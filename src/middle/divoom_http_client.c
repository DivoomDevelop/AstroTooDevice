#include "divoom.h"
#include <errno.h>

#include "curl/curl.h"
#include "curl/easy.h"

#define AES_BLOCK_SIZE 16
static volatile int g_divoom_http_abort = 0;

typedef struct {
    void *fp;
    uint32 offset;
    uint32 position;
    uint32 size;
    long status;
    int range_valid;
    int encoding_invalid;
    int protocol_error;
    int storage_error; /* 1: file I/O; 2: temporary memory/FD pressure. */
    int storage_errno;
    const char *path;
    divoom_http_client_down_file_callback *progress;
    void *user_data;
} divoom_http_client_resource_ctrl;

static size_t divoom_http_client_resource_header(char *data, size_t size, size_t count, void *user)
{
    divoom_http_client_resource_ctrl *ctrl = user;
    size_t length = size * count;
    char line[256];
    unsigned long first, last, total;
    if (length >= sizeof(line)) {
        return 0;
    }
    memcpy(line, data, length);
    line[length] = 0;
    if (!_strnicmp(line, "HTTP/", 5)) {
        sscanf(line, "HTTP/%*s %ld", &ctrl->status);
        ctrl->range_valid = 0;
        ctrl->encoding_invalid = 0;
    } else if (!_strnicmp(line, "Content-Range:", 14)) {
        ctrl->range_valid = sscanf(line + 14, " bytes %lu-%lu/%lu", &first, &last, &total) == 3 &&
            first == ctrl->offset && total == ctrl->size && last + 1 == total;
    } else if (!_strnicmp(line, "Content-Encoding:", 17)) {
        ctrl->encoding_invalid = _strnicmp(line + 17, " identity", 9) != 0;
    } else if (length == 2 && line[0] == '\r' && line[1] == '\n' && ctrl->status >= 200) {
        if (ctrl->status != 200 && ctrl->status != 206) {
            ctrl->protocol_error = ctrl->status == 416;
            return ctrl->status >= 300 && ctrl->status < 400 ? length : 0;
        }
        if (ctrl->encoding_invalid ||
            (ctrl->status == 206 && !ctrl->range_valid)) {
            ctrl->protocol_error = 1;
            return 0;
        }
        /* Range 被忽略的 200 必须从零覆盖，不能把完整响应追加到旧前缀。 */
        ctrl->position = ctrl->status == 206 ? ctrl->offset : 0;
        if (ctrl->progress) ctrl->progress(ctrl->user_data, ctrl->position, ctrl->size);
        errno = 0;
        ctrl->fp = divoom_port_file_open(ctrl->path, ctrl->position ? "r+b" : "wb");
        if (!ctrl->fp || (ctrl->position && divoom_port_file_seek(ctrl->fp, ctrl->position, DIVOOM_PORT_SEEK_SET) != 0)) {
            ctrl->storage_errno = errno ? errno : EIO;
            ctrl->storage_error = errno == ENOMEM || errno == EMFILE || errno == ENFILE ? 2 : 1;
            return 0;
        }
    }
    return length;
}

static size_t divoom_http_client_resource_write(char *data, size_t size, size_t count, void *user)
{
    divoom_http_client_resource_ctrl *ctrl = user;
    size_t length = size * count;
    if (!ctrl->fp || length > ctrl->size - ctrl->position) {
        ctrl->protocol_error = 1;
        return 0;
    }
    errno = 0;
    if (divoom_port_file_write(data, 1, length, ctrl->fp) != length) {
        ctrl->storage_errno = errno ? errno : EIO;
        ctrl->storage_error = errno == ENOMEM || errno == EMFILE || errno == ENFILE ? 2 : 1;
        return 0;
    }
    ctrl->position += (uint32)length;
    if (ctrl->progress) ctrl->progress(ctrl->user_data, ctrl->position, ctrl->size);
    return length;
}

static int divoom_http_client_resource_abort(void *user, curl_off_t total, curl_off_t current,
    curl_off_t upload_total, curl_off_t upload_current)
{
    (void)user;
    (void)total;
    (void)current;
    (void)upload_total;
    (void)upload_current;
    return g_divoom_http_abort;
}

int divoom_http_client_resource_part(const char *url, const char *path, uint32 offset, uint32 size, divoom_http_client_down_file_callback *progress, void *user_data)
{
    divoom_http_client_resource_ctrl ctrl = {0};
    CURL *curl = curl_easy_init();
    CURLcode rc;
    struct curl_slist *headers = NULL;
    char range[64];
    if (!curl) {
        return -1;
    }
    ctrl.path = path;
    ctrl.offset = offset;
    ctrl.size = size;
    ctrl.progress = progress;
    ctrl.user_data = user_data;
    headers = curl_slist_append(headers, "Accept-Encoding: identity");
    if (offset) {
        snprintf(range, sizeof(range), "Range: bytes=%u-", offset);
        headers = curl_slist_append(headers, range);
    }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 3L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
    /* 大文件允许持续下载超过半小时；连续 120 秒无有效数据才判断断流。 */
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 120L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, divoom_http_client_resource_header);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &ctrl);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, divoom_http_client_resource_write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctrl);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, divoom_http_client_resource_abort);
    rc = curl_easy_perform(curl);
    errno = 0;
    if (ctrl.fp && divoom_port_file_close(ctrl.fp) != 0 && !ctrl.storage_error) {
        ctrl.storage_errno = errno ? errno : EIO;
        ctrl.storage_error = errno == ENOMEM || errno == EMFILE || errno == ENFILE ? 2 : 1;
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (ctrl.storage_error) { errno = ctrl.storage_errno; return ctrl.storage_error == 2 ? -4 : -2; }
    if (ctrl.protocol_error) return -3;
    if (g_divoom_http_abort || rc == CURLE_ABORTED_BY_CALLBACK) return -5;
    /* Report this request's 404 directly; a concurrent request must not hide it. */
    if (ctrl.status == 404) return -6;
    if (ctrl.status == 408 || ctrl.status == 429 || ctrl.status >= 500 ||
        rc == CURLE_COULDNT_RESOLVE_HOST || rc == CURLE_COULDNT_CONNECT ||
        rc == CURLE_OPERATION_TIMEDOUT || rc == CURLE_RECV_ERROR ||
        rc == CURLE_SEND_ERROR || rc == CURLE_PARTIAL_FILE ||
        rc == CURLE_GOT_NOTHING || rc == CURLE_SSL_CONNECT_ERROR) return -4;
    return rc == CURLE_OK ? 0 : -1;
}

#define DIVOOM_PIC_RAW_SIZE 768
static void *divoom_lock_handle = NULL;

void divoom_http_client_request_abort(void)
{
    g_divoom_http_abort = 1;
}

void divoom_http_client_request_abort_clear(void)
{
    g_divoom_http_abort = 0;
}
void divoom_http_client_get_file_read_callback(void *user_data, uint8 *buf, int len, int tol);

void divoom_http_client_upload_err_flag(char *file_name, char *err_str)
{
    divoom_device_update_test_file_respone_info pinfo = {0};
    
    strcpy(pinfo.file_id, file_name);
    strcpy(pinfo.dest_info, err_str);
	divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_UPDATE_TEST_FILE, DIVOOM_SERVER_TOPIC, &pinfo);            
}

/**
 * NAME              divoom_http_client_upload_file
 * *  @brief        向服务器上传一个文件
 * *  @para         file_addr           本地文件地址
                    up_file_addr        服务器地址
 * *  @return       1：成功；0：失败
 * */

int divoom_http_client_upload_file(const char *file_addr, char *up_file_addr)
{
    (void)file_addr;
    if (up_file_addr != NULL) up_file_addr[0] = '\0';
    return 0;
}

#define OUT
/**
 * NAME              divoom_http_client_get_srv
 * *  @brief       	http璇锋眰鏈嶅姟鍣? * *  @param[in] 		none
 * *	 @author         aj               
 * *  @return       none
 * */

char *divoom_http_client_get_srv(char *url)
{
    return divoom_http_client_url_request(url, NULL, 0, NULL, NULL);
}

typedef struct _divoom_http_client_get_file_ctrl
{
    int file_size;
    int  tot_len;
    void *user_data;
    DIVOOM_HTTP_WIRTE_DATA *pwrite_callback;
}divoom_http_client_get_file_ctrl, *Pdivoom_http_client_get_file_ctrl;

static size_t divoom_http_client_dataCallback(void *ptr, size_t size, size_t nmemb, void *stream)
{
    Pdivoom_http_client_get_file_ctrl phandle = (Pdivoom_http_client_get_file_ctrl)stream;
    phandle->tot_len += size*nmemb;
	phandle->pwrite_callback(phandle->user_data, ptr, size*nmemb, phandle->file_size);
    return size*nmemb;
}

static int divoom_http_client_ascii_prefix_equal(const char *value,
                                                  size_t value_len,
                                                  const char *prefix,
                                                  size_t prefix_len)
{
    size_t index;

    if(value_len < prefix_len)
    {
        return 0;
    }
    for(index = 0; index < prefix_len; index++)
    {
        unsigned char left = (unsigned char)value[index];
        unsigned char right = (unsigned char)prefix[index];

        if(left >= 'A' && left <= 'Z')
        {
            left = (unsigned char)(left + ('a' - 'A'));
        }
        if(right >= 'A' && right <= 'Z')
        {
            right = (unsigned char)(right + ('a' - 'A'));
        }
        if(left != right)
        {
            return 0;
        }
    }
    return 1;
}
 
static size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    divoom_http_client_get_file_ctrl *pfile_info = (divoom_http_client_get_file_ctrl *)userdata;
    size_t total_size = size * nitems;
    
    // 查找Content-Length (支持大小写不敏感匹配)
    if (divoom_http_client_ascii_prefix_equal(buffer, total_size,
        "Content-Length:", sizeof("Content-Length:") - 1)) {
        char *ptr = buffer + 15;
        
        // 跳过空白字符
        while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == ':')) {
            ptr++;
        }
        
        // 解析数字
        if (*ptr >= '0' && *ptr <= '9') {
            int length = atoi(ptr);
            if (length > 0) {
                pfile_info->file_size = length;
                //divoom_print("Parsed Content-Length: %d bytes\n", length);
            }
        }
    }
    // 检测分块传输编码
    else if (divoom_http_client_ascii_prefix_equal(buffer, total_size,
        "Transfer-Encoding:", sizeof("Transfer-Encoding:") - 1)) {
        if (strstr(buffer, "chunked") != NULL) {
            divoom_print("Transfer-Encoding: chunked detected (Content-Length may not be available)\n");
            // 对于分块编码，file_size保持为0，表示长度未知
            pfile_info->file_size = -1; // 使用-1表示分块传输
        }
    }
    
    return total_size;
}

int divoom_http_client_get_net_tofile_sub(char *file_addr, DIVOOM_HTTP_WIRTE_DATA *pwrite_callback, void *user_data, Pdivoom_http_client_url_head_info phead_info, int head_num, DIVOOM_HTTP_EXIT_CALLBACK *pexit_callback, void *exit_handle)
{
    int i, still_running = 1, has_error = 0;
    struct curl_slist *headers = NULL;
	CURL *curl = NULL;    
    CURLM *multi_handle;
	CURLcode code = 0;    
	int ret = 0;
    divoom_http_client_get_file_ctrl pfile_info = {0};
	long response_code = -1;
    int retry_count = 0, divoom_flag = 0;
    const int max_retries = 3;

    divoom_flag = strstr(file_addr, "divoom-gz.com")!=NULL;

	curl = curl_easy_init();
	if(NULL == curl)
	{
		return -1;
	}
    multi_handle = curl_multi_init();
    if(!multi_handle)
    {
        curl_easy_cleanup(curl);
		return -1;
    }
    pfile_info.user_data = user_data;
    pfile_info.pwrite_callback = pwrite_callback;
    pfile_info.file_size = 0;
    pfile_info.tot_len = 0;  // 初始化下载长度计数器
    
	curl_easy_setopt(curl, CURLOPT_URL, file_addr);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, divoom_flag?300:10);				// 设置超时时间为10秒
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);  		// 增加连接超时到5秒
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 3L);           // 最大重定向次数
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &pfile_info);
    
    // 添加默认的User-Agent头，避免被服务器拒绝
    headers = curl_slist_append(headers, "User-Agent: DivoomApp/1.0 (compatible; curl/7.68.0)");
    
    // 设置请求头
    if(head_num)
    {
        char *head_buf = divoom_system_malloc(20*1024);
        if(head_buf)
        {
            for(i = 0 ; i < head_num; i++)
            {
                sprintf(head_buf, "%s: %s", phead_info[i].key, phead_info[i].word);
                headers = curl_slist_append(headers, head_buf);
            }
            divoom_system_free(head_buf);
        }
    }
    
    // 设置请求头
    if(headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    
    #ifdef DIVOOM_TEST_TUTV_FLAG
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    
    // 设置允许的密码套件（wolfSSL 格式）
    curl_easy_setopt(curl, CURLOPT_SSL_CIPHER_LIST, 
        "ECDHE-ECDSA-AES256-GCM-SHA384:"
        "ECDHE-RSA-AES256-GCM-SHA384:"
        "ECDHE-ECDSA-CHACHA20-POLY1305:"
        "ECDHE-RSA-CHACHA20-POLY1305");
    #endif
    
    // 设置数据回调函数
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &pfile_info);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, divoom_http_client_dataCallback);
    
    // 添加低速传输超时：如果30秒内传输速度低于1KB/s，则超时
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1024L);  // 1KB/s
    
    curl_multi_add_handle(multi_handle, curl);
    
    // 执行GET请求
    while(still_running)
    {
        CURLMcode mc = curl_multi_perform(multi_handle, &still_running);
        if (mc != CURLM_OK) {
            divoom_print("CURL multi error: %d (%s) at: %s\n", mc, curl_multi_strerror(mc), file_addr);
            has_error = 1;
            break;
        }
        
        // 添加短暂休眠
        divoom_port_delay_ms((1000) / 1000U);  // 1ms休眠
        
        if(g_divoom_http_abort || (pexit_callback && pexit_callback(exit_handle)))
        {
            divoom_print("system exit\n");
            has_error = 1;
            break;
        }
        CURLMsg *msg;
        int msgs_left;
        while ((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
            if (msg->msg == CURLMSG_DONE) {
                if (msg->data.result != CURLE_OK) {
                    if (msg->data.result == CURLE_OPERATION_TIMEDOUT) {
                        divoom_print("Timeout error: %d - Request(%s) timed out after 30 seconds\n", msg->data.result, file_addr);
                    } else {
                        divoom_print("CURL error: %d (%s)\n", msg->data.result, curl_easy_strerror(msg->data.result));
                    }
                    has_error = 1;
                } else {
                    // 获取HTTP响应码
                    long http_code = 0;
                    curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &http_code);
                    
                    // 检查HTTP状态码
                    if (http_code == 404) {
                        divoom_print("Error: File not found (404): %s\n", file_addr);
                        has_error = 1;
                    } else if (http_code == 405) {
                        divoom_print("Error: Method not allowed (405) - This API does not support the requested method\n");
                        has_error = 1;
                    } else if (http_code == 429) {
                        divoom_print("Error: HTTP 429 Too Many Requests\n");
                        has_error = 1;
                    } else if (http_code >= 400 && http_code != 401 && http_code != 403) {
                        divoom_print("Error: HTTP error %ld, %s\n", http_code, file_addr);
                        has_error = 1;
                    } else if (http_code == 200 || http_code == 401) {
                        //divoom_print("Success: HTTP %ld OK\n", http_code);
                        
                        // 获取CURL统计信息进行交叉验证
                        double download_size = 0;
                        curl_easy_getinfo(msg->easy_handle, CURLINFO_SIZE_DOWNLOAD, &download_size);
                        
                        // 下载完成后验证文件长度
                        if (pfile_info.file_size > 0) {
                            if (pfile_info.tot_len == pfile_info.file_size) {
                                //divoom_print("File length verification PASSED: downloaded %d bytes, expected %d bytes\n", 
                                //           pfile_info.tot_len, pfile_info.file_size);
                            } else {
                                divoom_print("File length verification FAILED: downloaded %d bytes, expected %d bytes\n", 
                                           pfile_info.tot_len, pfile_info.file_size);
                                has_error = 1;
                            }
                        } else if (pfile_info.file_size == -1) {
                            // 分块传输编码，无法预先知道文件大小
                            divoom_print("Download completed with chunked encoding: %d bytes received\n", 
                                       pfile_info.tot_len);
                        } else {
                            // 如果没有Content-Length头，只报告实际下载的字节数
                            divoom_print("Download completed: %d bytes received (Content-Length not available)\n", 
                                       pfile_info.tot_len);
                        }
                        
                        // 交叉验证CURL统计和我们的计数
                        if ((int)download_size != pfile_info.tot_len) {
                            divoom_print("Warning: CURL reports %.0f bytes, but callback counted %d bytes\n", 
                                       download_size, pfile_info.tot_len);
                        }
                    }
                }
                
                break;
            }
        }

        if (has_error) break;
        curl_multi_wait(multi_handle, NULL, 0, 50, NULL);
    }
    
    // 如果是429错误且还有重试次数，则等待后重试
    if (has_error) {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        if (http_code == 429 && retry_count < max_retries) {
            int delay_seconds = (1 << retry_count) * 2; // 指数退避：2, 4, 8秒
            divoom_print("Retrying after %d seconds (attempt %d/%d)\n", delay_seconds, retry_count + 1, max_retries);
            
            // 清理当前连接
            curl_multi_remove_handle(multi_handle, curl);
            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
            curl_multi_cleanup(multi_handle);
            
            // 等待指定时间
            for (int i = 0; i < delay_seconds; i++) {
                if (g_divoom_http_abort || (pexit_callback && pexit_callback(exit_handle))) {
                    return -1;
                }
                divoom_port_delay_ms((1000000) / 1000U); // 1秒
            }
            
            // 重新初始化连接
            retry_count++;
            curl = curl_easy_init();
            if (curl) {
                multi_handle = curl_multi_init();
                if (multi_handle) {
                    // 重新设置所有选项
                    curl_easy_setopt(curl, CURLOPT_URL, file_addr);
                    curl_easy_setopt(curl, CURLOPT_TIMEOUT, divoom_flag?300:10);
                    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
                    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 3L);
                    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
                    curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1);
                    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
                    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &pfile_info);
                    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &pfile_info);
                    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, divoom_http_client_dataCallback);
                    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L);
                    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1024L);
                    
                    // 重新设置请求头
                    headers = curl_slist_append(NULL, "User-Agent: DivoomApp/1.0 (compatible; curl/7.68.0)");
                    if (head_num) {
                        char *head_buf = divoom_system_malloc(20*1024);
                        if (head_buf) {
                            for (i = 0; i < head_num; i++) {
                                sprintf(head_buf, "%s: %s", phead_info[i].key, phead_info[i].word);
                                headers = curl_slist_append(headers, head_buf);
                            }
                            divoom_system_free(head_buf);
                        }
                    }
                    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
                    
                    #ifdef DIVOOM_TEST_TUTV_FLAG
                    curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
                    curl_easy_setopt(curl, CURLOPT_SSL_CIPHER_LIST, 
                        "ECDHE-ECDSA-AES256-GCM-SHA384:"
                        "ECDHE-RSA-AES256-GCM-SHA384:"
                        "ECDHE-ECDSA-CHACHA20-POLY1305:"
                        "ECDHE-RSA-CHACHA20-POLY1305");
                    #endif
                    
                    curl_multi_add_handle(multi_handle, curl);
                    has_error = 0;
                    still_running = 1;
                    
                    // 继续执行请求
                    goto retry_request;
                }
            }
        }
    }
    
    if(has_error)
    {
        curl_multi_remove_handle(multi_handle, curl);  // 必须先从multi handle中移除
        curl_easy_cleanup(curl);
        curl_multi_cleanup(multi_handle);
        curl_slist_free_all(headers);  // 确保清理请求头
		return -1;
    }

retry_request:
    // 执行重试请求
    while (still_running) {
        CURLMcode mc = curl_multi_perform(multi_handle, &still_running);
        if (mc != CURLM_OK) {
            divoom_print("Retry CURL multi error: %d (%s)\n", mc, curl_multi_strerror(mc));
            has_error = 1;
            break;
        }
        
        divoom_port_delay_ms((1000) / 1000U);
        
        if(g_divoom_http_abort || (pexit_callback && pexit_callback(exit_handle)))
        {
            divoom_print("system exit\n");
            has_error = 1;
            break;
        }
        
        CURLMsg *msg;
        int msgs_left;
        while ((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
            if (msg->msg == CURLMSG_DONE) {
                if (msg->data.result != CURLE_OK) {
                    divoom_print("Retry CURL error: %d (%s)\n", msg->data.result, curl_easy_strerror(msg->data.result));
                    has_error = 1;
                } else {
                    long http_code = 0;
                    curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &http_code);
                    
                    if (http_code == 200 || http_code == 401) {
                        divoom_print("Retry success: HTTP %ld OK\n", http_code);
                    } else {
                        divoom_print("Retry failed: HTTP %ld\n", http_code);
                        has_error = 1;
                    }
                }
                break;
            }
        }

        if (has_error) break;
        curl_multi_wait(multi_handle, NULL, 0, 50, NULL);
    }
    
    curl_multi_remove_handle(multi_handle, curl);
 
	
	curl_easy_cleanup(curl);
    curl_slist_free_all(headers);  // 确保清理请求头
    curl_multi_cleanup(multi_handle);
    if(has_error == 1)
    {
        return -1;
    }
	return ret;
    
}

int divoom_http_client_get_net_tofile(char *file_addr, DIVOOM_HTTP_WIRTE_DATA *pwrite_callback, void *user_data, Pdivoom_http_client_url_head_info phead_info, int head_num, DIVOOM_HTTP_EXIT_CALLBACK *pexit_callback, void *exit_handle)
{
	int ret = 0;

	if (divoom_lock_handle == NULL)
		divoom_lock_handle = divoom_system_create_lock();
	//divoom_system_get_lock(divoom_lock_handle);
    ret = divoom_http_client_get_net_tofile_sub(file_addr, pwrite_callback, user_data, phead_info, head_num, pexit_callback, exit_handle);
	//divoom_system_free_lock(divoom_lock_handle);
	return ret;
}


typedef struct _divoom_http_client_download_item
{
    int file_len;
    uint8 *file_buf;
}divoom_http_client_download_item, *Pdivoom_http_client_download_item;

typedef struct _divoom_http_client_download_info
{
    int file_size;
    int tol_len;
    divoom_http_client_down_file_callback *pcallback;
    void *user_data;
    void *file_queue;
}divoom_http_client_download_info, *Pdivoom_http_client_download_info;
void divoom_http_client_get_file_read_callback(void *user_data, uint8 *buf, int len, int tol)
{
    Pdivoom_http_client_download_info phandle = (Pdivoom_http_client_download_info)user_data;
    Pdivoom_http_client_download_item pitem = divoom_system_malloc(sizeof(divoom_http_client_download_item) + len + 1);
    if(pitem == NULL)
    {
        return;
    }
    pitem->file_buf = (uint8 *)&pitem[1];
    memcpy(pitem->file_buf, buf, len);
    pitem->file_len = len;
    divoom_queue_push(phandle->file_queue, pitem);
    phandle->tol_len += len;
    
}

char *divoom_http_client_get_file(char *file_addr, int *len, Pdivoom_http_client_url_head_info phead_info, int head_num, DIVOOM_HTTP_EXIT_CALLBACK *pexit_callback, void *exit_handle)
{
    int buf_off = 0, ret;
    uint8 *file_buf = NULL;
    divoom_http_client_download_info file_info;
    Pdivoom_http_client_download_item pitem;

    if(divoom_main_get_wifi_con_status() == 0 && memcmp(file_addr, DIVOOM_TEST_SERVER_HTTP_LIST_GUIDE_ADDR, 20))
    {
        return NULL;
    }

    file_info.file_queue = divoom_queue_init(10000, __func__, __LINE__);
    file_info.tol_len = 0;
    ret = divoom_http_client_get_net_tofile(file_addr, divoom_http_client_get_file_read_callback, &file_info, phead_info, head_num, pexit_callback, exit_handle);
    if(file_info.tol_len && ret == 0)
    {
        file_buf = divoom_system_malloc(file_info.tol_len + 1);
        if(file_buf)
        {
            while(divoom_queue_items(file_info.file_queue))
            {
                pitem = divoom_queue_pop(file_info.file_queue);
                if(pitem == NULL)
                {
                    break;
                }
                memcpy(&file_buf[buf_off], pitem->file_buf, pitem->file_len);
                buf_off += pitem->file_len;
                divoom_system_free(pitem);
            }
            file_buf[buf_off] = 0;
        }
        
    }
    divoom_queue_destroy(file_info.file_queue, NULL);
    if(len)
    {
        *len = file_info.tol_len;
    }
    return file_buf;
}


char *divoom_http_client_url_request(char *url_addr, Pdivoom_http_client_url_head_info phead_info, int head_num, DIVOOM_HTTP_EXIT_CALLBACK *pexit_callback, void *exit_handle)
{
    if(divoom_wifi_model_get_wifi_status() == 0)
    {
        return NULL;
    }
    return divoom_http_client_get_file(url_addr, NULL, phead_info, head_num, pexit_callback, exit_handle);
}

int divoom_http_client_last_http_status(void)
{
    return 0;
}

void divoom_http_client_set_skip_https_redirect(int skip)
{
    (void)skip;
}

int divoom_http_client_last_skipped_https_redirect(void)
{
    return 0;
}

int divoom_http_client_last_errno(void)
{
    return 0;
}

int divoom_http_client_last_is_auth_error(void)
{
    return 0;
}

int divoom_http_client_last_is_transport_error(void)
{
    return 0;
}

void divoom_http_client_down_file_read_callback(void *user_data, uint8 *buf, int len, int tol)
{
    Pdivoom_http_client_download_info phandle = (Pdivoom_http_client_download_info)user_data;

    divoom_port_file_write(buf, len, 1, phandle->file_queue);
    phandle->tol_len += len;
    if(phandle->pcallback)
    {
        phandle->pcallback(phandle->user_data, phandle->tol_len, tol);
    }
    phandle->file_size = tol;
    
}


static int divoom_http_client_build_download_url(char *url, size_t url_size,
    const char *server, const char *file_addr)
{
    size_t server_len = server ? strnlen(server, url_size) : 0;
    size_t file_addr_len;

    if(url == NULL || url_size == 0 || file_addr == NULL)
    {
        return 0;
    }
    file_addr_len = strnlen(file_addr, url_size);
    if(server_len >= url_size || file_addr_len >= url_size ||
        server_len + file_addr_len >= url_size)
    {
        divoom_print("download url is invalid or too long\n");
        return 0;
    }
    if(server_len)
    {
        memcpy(url, server, server_len);
    }
    memcpy(url + server_len, file_addr, file_addr_len);
    url[server_len + file_addr_len] = '\0';
    return 1;
}

int divoom_http_client_down_file(char *new_name, char *file_addr, int8 flag, int *file_size)
{
    return divoom_http_client_down_file_with_exit(new_name, file_addr, flag, file_size, NULL, NULL);
}
int divoom_http_client_down_file_with_exit(char *new_name, char *file_addr, int8 flag,
    int *file_size, DIVOOM_HTTP_EXIT_CALLBACK *exit_callback, void *exit_handle)
{	
    int ret, retry_flag = 0, tmp_flag = 0;
    divoom_http_client_download_info file_info  = {0};
	char url[256] = "";
    char tmp_file_name[256];

    if(new_name == NULL || strnlen(new_name, sizeof(tmp_file_name)) >= sizeof(tmp_file_name))
    {
        return -2;
    }
    if(strstr(new_name, DIVOOM_TMP_DIR "/"))
    {
        tmp_flag = 1;
        memcpy(tmp_file_name, new_name, strlen(new_name) + 1);
    }
    else
    {
        sprintf(tmp_file_name, "/userdata/%d.bin", divoom_main_get_tmp_file_index());
    }
    
    if(divoom_wifi_model_get_wifi_status() == 0)
    {
        return -2;
    }
    if(!divoom_http_client_build_download_url(url, sizeof(url),
        flag ? DIVOOM_SERVER_URL : NULL, file_addr))
    {
        return -2;
    }
    
    divoom_http_client_down_file_retry:
    if (exit_callback && exit_callback(exit_handle))
        return -1;
    divoom_print("[storage_io] http_begin via=%s dst=%s tmp=%s retry=%d free=%uKB\n",
        __func__, new_name ? new_name : "(null)", tmp_file_name, retry_flag,
        (uint32)divoom_port_fs_free_space(DIVOOM_USER_ROOT));
    file_info.file_queue = divoom_port_file_open(tmp_file_name, "wb");
    divoom_print("[storage_io] http_open via=%s tmp=%s ok=%d\n", __func__, tmp_file_name, file_info.file_queue != NULL);
    if(file_info.file_queue == NULL)
    {
        return -2;
    }
    
    ret = divoom_http_client_get_net_tofile(url, divoom_http_client_down_file_read_callback, &file_info, NULL, 0, exit_callback, exit_handle);
    divoom_print("[storage_io] http_received via=%s dst=%s ret=%d bytes=%d\n",
        __func__, new_name ? new_name : "(null)", ret, file_info.tol_len);
    if(file_size)
    {
        *file_size  = file_info.tol_len;
    }
    divoom_port_file_close(file_info.file_queue);
    if (exit_callback && exit_callback(exit_handle))
    {
        divoom_app_com_del_file(tmp_file_name);
        return -1;
    }
    if(ret < 0)
    {
        divoom_app_com_del_file(tmp_file_name);
        if (flag && retry_flag == 0)
        {
            if(!divoom_http_client_build_download_url(url, sizeof(url),
                divoom_main_get_file_other_server_addr(), file_addr))
            {
                return -2;
            }
            retry_flag = 1;
            goto divoom_http_client_down_file_retry;
        }
    }
    if(ret== 0 && tmp_flag == 0)
    {
        divoom_app_com_rename_file(tmp_file_name, new_name);
    }
    divoom_main_update_last_download_file();
    divoom_print("[storage_io] http_end dst=%s ret=%d bytes=%d free=%uKB\n",
        new_name, ret, file_info.tol_len, (uint32)divoom_port_fs_free_space(DIVOOM_USER_ROOT));
    return file_info.tol_len > 0 ? 1:-1;
}
int divoom_http_client_down_file_no_delay(char *new_name, char *file_addr, int8 flag, int *file_size, divoom_http_client_down_file_callback *pcallback, void *para)
{	
    int retry_flag = 0, ret;
    divoom_http_client_download_info file_info  = {0};
	char url[256] = "";
    char tmp_file_name[256];

    sprintf(tmp_file_name, "/userdata/%d.bin", divoom_main_get_tmp_file_index());
    
    if(divoom_wifi_model_get_wifi_status() == 0)
    {
        return 0;
    }
	if (flag)
		sprintf(url, "%s%s", DIVOOM_SERVER_URL, file_addr);
	else
		sprintf(url, "%s", file_addr);
    divoom_http_client_down_file_no_delay_retry:
    file_info.pcallback = pcallback;
    file_info.user_data = para;
    divoom_print("[storage_io] http_begin via=%s dst=%s tmp=%s retry=%d free=%uKB\n",
        __func__, new_name ? new_name : "(null)", tmp_file_name, retry_flag,
        (uint32)divoom_port_fs_free_space(DIVOOM_USER_ROOT));
    file_info.file_queue = divoom_port_file_open(tmp_file_name, "wb");
    divoom_print("[storage_io] http_open via=%s tmp=%s ok=%d\n", __func__, tmp_file_name, file_info.file_queue != NULL);
    if(file_info.file_queue == NULL)
    {
        return 0;
    }
    
    ret = divoom_http_client_get_net_tofile(url, divoom_http_client_down_file_read_callback, &file_info, NULL, 0, NULL, NULL);
    divoom_print("[storage_io] http_received via=%s dst=%s ret=%d bytes=%d\n",
        __func__, new_name ? new_name : "(null)", ret, file_info.tol_len);
    if(file_size)
    {
        *file_size  = file_info.tol_len;
    }
    divoom_port_file_close(file_info.file_queue);
    if(file_info.file_size == 0)
    {
        if(retry_flag == 0 && flag)
        {
            sprintf(url, "%s%s", divoom_main_get_file_other_server_addr(), file_addr);
            retry_flag = 1;
            goto divoom_http_client_down_file_no_delay_retry;
        }
        return 0;
    }
    if(ret == 0)
    {
        divoom_app_com_rename_file(tmp_file_name, new_name);
        divoom_main_update_last_download_file();
    }
    return file_info.tol_len == file_info.file_size;
}

int divoom_http_client_convert_file_undecode(char *file_addr)
{
	char *buffer = NULL;
	int ret = 1, read_len, tmp_len, net_file_len;
    uint8 image_type[4];    
	FILE *divoom_file_handle = NULL; 
    
    divoom_file_handle = divoom_port_file_open(file_addr, "rb");
    if(divoom_file_handle)
    {
        tmp_len = divoom_port_file_read(image_type,1, 4, divoom_file_handle);
        if(memcmp(image_type, "RIFF", 4) == 0)
        {
            //webp
        }
        else if(memcmp(image_type, "GIF", 3) == 0)
        {
            //gif
        }
        else if(image_type[0] == DIVOOM_NET_FILE_TYPE_SINGLE_AES||                
                image_type[0] == DIVOOM_NET_FILE_TYPE_GIF_AES||         
                image_type[0] == DIVOOM_NET_FILE_TYPE_PIC_SCAN||       
                image_type[0] == DIVOOM_NET_FILE_TYPE_NEW_MUL_SINGLE ||
                image_type[0] == DIVOOM_NET_FILE_TYPE_NEW_MUL_GIF)
        {
            divoom_port_file_seek(divoom_file_handle, 0, SEEK_END);
            net_file_len = divoom_port_file_tell(divoom_file_handle);
            divoom_port_file_seek(divoom_file_handle, 0, SEEK_SET);
            buffer = divoom_system_malloc(net_file_len);
            if(buffer)
            {
                tmp_len = divoom_port_file_read(buffer, 1, net_file_len, divoom_file_handle);
                divoom_port_file_close(divoom_file_handle);
                divoom_file_handle = divoom_port_file_open(file_addr, "wb");
                if(divoom_file_handle == NULL)
                {
                    divoom_system_free(buffer);
                    return 1;
                }
                read_len = 1;
                switch(buffer[0])
                {   
                    case DIVOOM_NET_FILE_TYPE_SINGLE_AES:
                        read_len = 1;
                        break;

                    case DIVOOM_NET_FILE_TYPE_GIF_AES:
                        read_len = 4;
                        break;
                     
                    case DIVOOM_NET_FILE_TYPE_PIC_SCAN:
                        read_len = 4;
                        break;
                        
                         
                    case DIVOOM_NET_FILE_TYPE_NEW_MUL_SINGLE:
                        read_len = 7;
                        break;
                        
                    case DIVOOM_NET_FILE_TYPE_NEW_MUL_GIF:
                        read_len = 6;
                        break;                
                }
                divoom_aes_do(&buffer[read_len], net_file_len - read_len);
                tmp_len = divoom_port_file_write(buffer, 1, net_file_len, divoom_file_handle);
            	divoom_free(buffer);	
            }
        }
        divoom_port_file_close(divoom_file_handle);
    }
    return ret;
}

int divoom_http_client_down_file_undecode(char *new_name, char *file_addr, int8 flag, int *file_size)
{	
	char *buffer = NULL;
	int ret = 1, read_len, tmp_len, net_file_len;
    uint8 image_type[4];    
	FILE *divoom_file_handle = NULL; 

    if(divoom_http_client_down_file(new_name, file_addr, flag, &net_file_len) == 1)
    {
        if(file_size)
        {
            *file_size = net_file_len;
        }
        divoom_file_handle = divoom_port_file_open(new_name, "rb");
        if(divoom_file_handle)
        {
            tmp_len = divoom_port_file_read(image_type,1, 4, divoom_file_handle);
            if(memcmp(image_type, "RIFF", 4) == 0)
            {
                //webp
            }
            else if(memcmp(image_type, "GIF", 3) == 0)
            {
                //gif
            }
            else if(image_type[0] == DIVOOM_NET_FILE_TYPE_SINGLE_AES||                
                    image_type[0] == DIVOOM_NET_FILE_TYPE_GIF_AES||         
                    image_type[0] == DIVOOM_NET_FILE_TYPE_PIC_SCAN||       
                    image_type[0] == DIVOOM_NET_FILE_TYPE_NEW_MUL_SINGLE ||
                    image_type[0] == DIVOOM_NET_FILE_TYPE_NEW_MUL_GIF)
            {
                divoom_port_file_seek(divoom_file_handle, 0, SEEK_SET);
                buffer = divoom_system_malloc(net_file_len);
                if(buffer)
                {
                    tmp_len = divoom_port_file_read(buffer, 1, net_file_len, divoom_file_handle);
                    divoom_port_file_close(divoom_file_handle);
                    divoom_file_handle = divoom_port_file_open(new_name, "wb");
                    if(divoom_file_handle == NULL)
                    {
                        divoom_system_free(buffer);
                        return 1;
                    }
                    read_len = 1;
                    switch(buffer[0])
                    {   
                        case DIVOOM_NET_FILE_TYPE_SINGLE_AES:
                            read_len = 1;
                            break;

                        case DIVOOM_NET_FILE_TYPE_GIF_AES:
                            read_len = 4;
                            break;
                         
                        case DIVOOM_NET_FILE_TYPE_PIC_SCAN:
                            read_len = 4;
                            break;
                            
                             
                        case DIVOOM_NET_FILE_TYPE_NEW_MUL_SINGLE:
                            read_len = 7;
                            break;
                            
                        case DIVOOM_NET_FILE_TYPE_NEW_MUL_GIF:
                            read_len = 6;
                            break;                
                    }
                    divoom_aes_do(&buffer[read_len], net_file_len - read_len);
                    tmp_len = divoom_port_file_write(buffer, 1, net_file_len, divoom_file_handle);
                	divoom_free(buffer);	
                }
            }
            divoom_port_file_close(divoom_file_handle);
            divoom_main_update_last_download_file();
        }
        return 1;
    }
    else
    {
        return 0;
    }
    
}


char *divoom_http_client_json_unpacket(char *string)
{
	divoom_cJSON *root = NULL, *ret_cmd = NULL;
	int ret_code = 0;
	char *file_id = divoom_malloc(128, DIVOOM_MODEL_NET, __LINE__, __FILE__);

	root = divoom_cJSON_Parse(string);
	if (!root)
	{
		return NULL;
	}	
	ret_code = divoom_cJSON_GetObjectInt(root, "ReturnCode");
	if (ret_code)
	{
		ret_cmd = divoom_cJSON_GetObjectItem(root, "ReturnMessage");
		divoom_print("%s:%d:the cmd fails at %s\n", __FILE__, __LINE__, ret_cmd->valuestring);
		return NULL;
	}
	else
	{
		divoom_cJSON_GetObjectString(root, "FileId", file_id);
	}
	return file_id;
}

char *divoom_http_client_deal_msg(char *hmsg)
{
	char *pcur = NULL, *pre = NULL;
	pcur = hmsg;
	pre = hmsg;
	char msg[1024] = {0};
	
	while(*pcur)
	{
		if (*pcur == '{')
		{
			pre = pcur;
			while (*pcur)
			{
				if (*pcur == '}')//结尾
				{					
					memset(msg, 0, sizeof(msg));
					strncpy(msg, pre, (pcur - pre + 1));							
					//divoom_print("DIVOOM_HTTP_DEBUG: read : %s\n", msg);
					return divoom_http_client_json_unpacket(msg);				
				}
				pcur++;
			}
		}
		pcur++;
	}
	return NULL;
}

char *divoom_http_send_data_sub(char *post_url, char *post_data, int flag, Pdivoom_http_client_url_head_info phead_info, int head_num)
{
    int ret = 0;
    long info = 0;
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers=NULL; 
    
    int buf_off = 0;
    uint8 *file_buf = NULL;
    divoom_http_client_download_info file_info;
    Pdivoom_http_client_download_item pitem;     
    divoom_http_client_get_file_ctrl pfile_info;
    long response_code = -1;
    int retry_count = 0;
    const int max_retries = 3;

    curl = curl_easy_init();
    if(NULL == curl)
    {
        return NULL;
    }
    file_info.file_queue = divoom_queue_init(10000, __func__, __LINE__);
    file_info.tol_len = 0;
    pfile_info.user_data = &file_info;
    pfile_info.tot_len = 0;
    pfile_info.pwrite_callback = divoom_http_client_get_file_read_callback;

    if (curl)
    {
        char *head_buf = NULL;

        // 添加默认的User-Agent头，避免被服务器拒绝
        headers = curl_slist_append(headers, "User-Agent: DivoomApp/1.0 (compatible; curl/7.68.0)");

        if(head_num)
        {
            head_buf = divoom_system_malloc(20*1024);
            if(head_buf)
            {
                for(int i = 0; i < head_num; i++)
                {
                    sprintf(head_buf, "%s: %s", phead_info[i].key, phead_info[i].word);
                    headers = curl_slist_append(headers, head_buf);
                }
                divoom_system_free(head_buf);
				headers = curl_slist_append(headers, "Content-Type: application/json"); // 设置适当的内容类型
            }
        }
        
        // 设置请求头
        if(headers) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }
        
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); //不要验证ssl证书
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

        curl_easy_setopt(curl, CURLOPT_URL, post_url);//设置post请求的url地址
        //curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1);//设置发送超时时间
        curl_easy_setopt(curl,	CURLOPT_WRITEFUNCTION,	divoom_http_client_dataCallback);
        curl_easy_setopt(curl,	CURLOPT_WRITEDATA,	(void	*)&pfile_info);
        #ifdef DIVOOM_TEST_TUTV_FLAG
        curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
        
        // 设置允许的密码套件（wolfSSL 格式）
        curl_easy_setopt(curl, CURLOPT_SSL_CIPHER_LIST, 
            "ECDHE-ECDSA-AES256-GCM-SHA384:"
            "ECDHE-RSA-AES256-GCM-SHA384:"
            "ECDHE-ECDSA-CHACHA20-POLY1305:"
            "ECDHE-RSA-CHACHA20-POLY1305");
        #endif
        
        if(post_data != NULL)
        {
            curl_easy_setopt(curl,CURLOPT_POSTFIELDS,post_data);
        }
        if(flag == 0)
        {
            curl_easy_setopt(curl,CURLOPT_HTTPGET,1);
        }
        else
        {
            curl_easy_setopt(curl, CURLOPT_POST, 1); 
        }
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);				// 设置总超时时间为30秒
//		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L); 		// 设置连接超时为5秒

retry_request:
        res = curl_easy_perform(curl);
        //curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE  , &info);

		 // 检查请求是否成功
        if(res != CURLE_OK) {
            // 打印错误信息
            divoom_print("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            // 获取HTTP响应码
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

            // 检查常见错误码
            if(response_code == 429 && retry_count < max_retries) {
                int delay_seconds = (1 << retry_count) * 2; // 指数退避：2, 4, 8秒
                divoom_print("HTTP 429 Too Many Requests, retrying after %d seconds (attempt %d/%d)\n", 
                           delay_seconds, retry_count + 1, max_retries);
                
                // 等待指定时间
                for (int i = 0; i < delay_seconds; i++) {
                    divoom_port_delay_ms((1000000) / 1000U); // 1秒
                }
                
                retry_count++;
                goto retry_request;
            } else if(response_code >= 400) {
                divoom_print("Error: Server returned HTTP code %ld\n", response_code);
            }
        }
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if(file_info.tol_len)
        {
            file_buf = divoom_system_malloc(file_info.tol_len + 1);
            if(file_buf)
            {
                while(divoom_queue_items(file_info.file_queue))
                {
                    pitem = divoom_queue_pop(file_info.file_queue);
                    if(pitem == NULL)
                    {
                        break;
                    }
                    memcpy(&file_buf[buf_off], pitem->file_buf, pitem->file_len);
                    buf_off += pitem->file_len;
                    divoom_system_free(pitem);
                }
                file_buf[buf_off] = 0;
            }
            
        }
        divoom_queue_destroy(file_info.file_queue, NULL);
        return file_buf;

    }
    return file_buf;

}

char *divoom_http_send_data(char *post_url, char *post_data, int flag, Pdivoom_http_client_url_head_info phead_info, int head_num)
{
	char *ret_data = NULL;

	if (divoom_lock_handle == NULL)
		divoom_lock_handle = divoom_system_create_lock();

	//divoom_system_get_lock(divoom_lock_handle);
	ret_data = divoom_http_send_data_sub(post_url, post_data, flag, phead_info, head_num);
	//divoom_system_free_lock(divoom_lock_handle);
	return ret_data;	
}

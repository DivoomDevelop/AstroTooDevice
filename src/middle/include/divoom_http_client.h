#ifndef _divoom_http_client_h_
#define _divoom_http_client_h_
#include "divoom_type.h"
#include <stdio.h>
#include <string.h>
#define DIVOOM_SERVER_PORT		9501
#define DIVOOM_FILE_SERVER_PORT	80


/**
 * NAME              divoom_http_client_url_request
 * *  @brief       	标准的HTTP GET请求
 * *  @param[in] 		url_addr 请求地址，参数也放在地址中
                    phead_info   请求头信息
                    head_num     请求头数目
 * *  @return        获取请求内容，直接返回
 * */
 

/* OAuth JWT（如 Outlook AccessToken）可达 3KB+，"Bearer " 前缀需额外空间 */
#define DIVOOM_HTTP_CLIENT_URL_HEAD_WORD_SIZE 4096

typedef struct _divoom_http_client_url_head_info
{
    char key[64];
    char word[DIVOOM_HTTP_CLIENT_URL_HEAD_WORD_SIZE];
}divoom_http_client_url_head_info, *Pdivoom_http_client_url_head_info;

static inline void divoom_http_client_head_set_bearer_auth(Pdivoom_http_client_url_head_info phead_info, const char *token)
{
    if (phead_info == NULL)
    {
        return;
    }
    strcpy(phead_info->key, "Authorization");
    if (token == NULL || token[0] == '\0')
    {
        phead_info->word[0] = '\0';
        return;
    }
    snprintf(phead_info->word, sizeof(phead_info->word), "Bearer %s", token);
}
typedef void DIVOOM_HTTP_WIRTE_DATA(void *user_data, uint8 *buf, int len, int tol);
typedef int DIVOOM_HTTP_EXIT_CALLBACK(void *user_data);

char *divoom_http_client_url_request(char *url_addr, Pdivoom_http_client_url_head_info phead_info, int head_num, DIVOOM_HTTP_EXIT_CALLBACK *pexit_callback, void *exit_handle);
void divoom_http_client_set_skip_https_redirect(int skip);
int divoom_http_client_last_skipped_https_redirect(void);

/**
 * NAME              divoom_http_client_get_file
 * *  @brief       	获取文件里的内容
 * *  @param[in] 		file_addr 文件地址， len 长度
 * *	 @author         wangft               
 * *  @return        获取到的文件内容
 * */

char *divoom_http_client_get_file(char *file_addr, int *len, Pdivoom_http_client_url_head_info phead_info, int head_num, DIVOOM_HTTP_EXIT_CALLBACK *pexit_callback, void *exit_handle);

/**
 * NAME              divoom_http_client_get_net_tofile
 * *  @brief       	获取文件里的内容传给回调
 * *  @param[in] 		file_addr 文件地址， len 长度
 * *	 @author         wangft               
 * *  @return        获取到的文件内容
 * */

int divoom_http_client_get_net_tofile(char *file_addr, DIVOOM_HTTP_WIRTE_DATA *pwrite_callback, void *user_data, Pdivoom_http_client_url_head_info phead_info, int head_num, DIVOOM_HTTP_EXIT_CALLBACK *pexit_callback, void *exit_handle);

/* Lightweight RTOS transport helpers for authenticated REST requests. */
/* 打断在途 HTTP（shutdown socket）。仅拆网编排调用；断网结束后立刻 abort_clear。 */
void divoom_http_client_request_abort(void);
void divoom_http_client_request_abort_clear(void);

int divoom_http_client_download_file_with_auth(const char *url, const char *file_name,
                                               const char *date,
                                               const char *authorization,
                                               int *file_size);
int divoom_http_client_head_with_auth(const char *url, const char *date,
                                      const char *authorization,
                                      int *content_length);

/**
 * NAME              divoom_http_client_down_file
 * *  @brief       	http获取文件
 * *  @param[in] 		new_name 保存新文件名， file_addr 获取文件的地址, flag 1表示获取divoom文件服务器 0表示获取其他文件服务器
 * *	 @author         wangft               
 * *  @return        <0 获取失败， 1 获取成功
 * */

int divoom_http_client_down_file(char *new_name, char *file_addr, int8 flag, int *file_size);
/* Per-request cancellation; never aborts unrelated HTTP transfers. */
int divoom_http_client_down_file_with_exit(char *new_name, char *file_addr, int8 flag,
    int *file_size, DIVOOM_HTTP_EXIT_CALLBACK *exit_callback, void *exit_handle);
typedef void DIVOOM_FTP_READ_LEN_CALLBACK(int read_len);
int divoom_ftp_client_down_file(char *new_name, char *file_addr, DIVOOM_FTP_READ_LEN_CALLBACK pcallback);
/**
 * NAME              divoom_http_client_down_file_no_delay
 * *  @brief       	http获取文件
 * *  @param[in] 		new_name 保存新文件名， file_addr 获取文件的地址, flag 1表示获取divoom文件服务器 0表示获取其他文件服务器
 * *	 @author         wangft               
 * *  @return        <0 获取失败， 1 获取成功
 * */

typedef void divoom_http_client_down_file_callback(void *user_data,uint32 read_len, uint32 tol_len);
int divoom_http_client_down_file_no_delay(char *new_name, char *file_addr, int8 flag, int *file_size, divoom_http_client_down_file_callback *pcallback, void *para);

/* RTOS OTA only: retain .tmp + .resume for the same URL/SHA1 across retries/reboots.
 * Verify the full SHA1 before publishing new_name. No SHA1 => full download only.
 * flag uses the same server-prefix convention as down_file_no_delay. Return 1/0.
 */
int divoom_http_client_down_ota_file(const char *new_name, const char *file_addr,
                                     int8 flag, const char *sha1_expect,
                                     divoom_http_client_down_file_callback *pcallback,
                                     void *para);

/**
 * NAME              divoom_http_client_down_file_undecode
 * *  @brief       	http获取文件,此接口一定是获取图像数据,并完成aes解码动作
 * *  @param[in] 		new_name 保存新文件名， file_addr 获取文件的地址, flag 1表示获取divoom文件服务器 0表示获取其他文件服务器
 * *	 @author         aijun              
 * *  @return        <0 获取失败， 1 获取成功
 * */

int divoom_http_client_down_file_undecode(char *new_name, char *file_addr, int8 flag, int *file_size);
int divoom_http_client_convert_file_undecode(char *file_addr);

#define DIVOOM_HTTP_CLIENT_BUF_SIZE (64*1024)
typedef struct _divoom_http_client_update_buffer_item
{
    int len;
    uint8 buf[DIVOOM_HTTP_CLIENT_BUF_SIZE];
}divoom_http_client_update_buffer_item, *Pdivoom_http_client_update_buffer_item;

typedef struct _divoom_http_client_update_buffer_info
{
    int ram_num;
    Pdivoom_http_client_update_buffer_item buf_list[20];
}divoom_http_client_update_buffer_info, *Pdivoom_http_client_update_buffer_info;
/**
 * NAME              divoom_http_client_upload_file
 * *  @brief       	http上传文件
 * *  @param[in] 		file_buf 上传的缓冲区
                        file_len 上传数据长度 
 * *	 @author         wangft               
 * *  @return       文件id
 * */

//char *divoom_http_client_upload_file(Pdivoom_http_client_update_buffer_info buf_info);

/*********************************************************
  * CREATED TIME	: 2020年 06月 17日 星期三 16:17:55 CST
  * FUNCTION		: divoom_httpclient_get_common_url
  * IUPUT			: char *url, uint32 ret_len
  * OUTPUT			: *
  * DESCRIPTION		: 用于通用的获取某些http返回,传入一个url地址
  *********************************************************/
char *divoom_httpclient_get_common_url(char *url, uint32 *ret_len);

/**
 * NAME              divoom_http_client_get_srv
 * *  @brief       	http请求服务器
 * *  @param[in] 		none
 * *	 @author         aj               
 * *  @return       字串
 * */

char *divoom_http_client_get_srv(char *url);

/**
 * NAME              divoom_http_client_upload_file
 * *  @brief        向服务器上传一个文件
 * *  @para         file_addr           本地文件地址
                    up_file_addr        服务器地址
 * *  @return       1：成功；0：失败
 * */

int divoom_http_client_upload_file(const char *file_addr, char *up_file_addr);
/* 表盘附件使用独立于系统日志的上传接口。 */
int divoom_http_client_upload_watchface_file(const char *file_addr, char *up_file_addr);

//建立连接,发送数据,post_url是url地址,post_ip是点分十进制地址,当post_ip存在的时候,忽略post_url
//不涉及301,302,401错误
char *divoom_http_send_data(char *post_url, char *post_data, int flag, Pdivoom_http_client_url_head_info phead_info, int head_num);
	
//return read len, <=0 fail
//return content_len
int divoom_http_recv_data(void *handle, char *buffer, int len, uint8 fetch_flag, int *content_len);
//int divoom_http_recv_data(void *handle, char *buffer, int len);
int divoom_http_client_get_err_download_fileszie(void);
/* Returns 1 verified, 2 HTTP 404 (skip), 0 failed/cancelled,
 * -1 local storage failed after the initial attempt and 3 retries.
 * retry receives this file's error count; 0 at start/completion; return 0 cancels. */
int divoom_http_client_down_resource_file(const char *path, const char *url, uint32 size, uint32 crc, int (*retry)(int errors), divoom_http_client_down_file_callback *progress, void *user_data);
/* Maintenance owns its content retry budget. Network/resource retries stay
 * unchanged; a completed bad-size/CRC payload is retained for deferred cleanup. */
#define DIVOOM_HTTP_RESOURCE_CONTENT_MISMATCH (-7)
int divoom_http_client_down_resource_file_checked(const char *path, const char *url, uint32 size, uint32 crc, int (*retry)(int errors), divoom_http_client_down_file_callback *progress, void *user_data);
/* Remove only this request's staging file and resume identity; returns 1 only
 * after removal is persisted; unrelated identities sharing a hash are kept. */
/* Validate the exact URL/path identity without creating files or issuing HTTP. */
int divoom_http_client_resource_file_supported(const char *path, const char *url, uint32 size, uint32 crc);
/* 1 exact request ownership (including a completed bad payload), 0 absent or
 * foreign identity, -1 metadata read/close failure. Does not modify files. */
int divoom_http_client_resource_file_owned(const char *path, const char *url, uint32 size, uint32 crc);
int divoom_http_client_cleanup_resource_file(const char *path, const char *url, uint32 size, uint32 crc);
/* resource_part: -6 HTTP 404 for this request; -5 interrupted request (retry callback decides cancellation);
 * -4 transient transport/HTTP error; -2 storage; -3 invalid protocol. */
int divoom_http_client_resource_part(const char *url, const char *path, uint32 offset, uint32 size, divoom_http_client_down_file_callback *progress, void *user_data);
int divoom_http_client_last_http_status(void);
//void *divoom_http_sse_url_request_connect(char *url_addr, Pdivoom_http_client_url_head_info phead_info, int head_num, http_event_handle_cb pcallback);
void divoom_http_sse_url_request_run(void *handle);
void divoom_http_sse_url_request_stop(void *handle);
void divoom_http_sse_url_request_destroy(void *handle);



#endif

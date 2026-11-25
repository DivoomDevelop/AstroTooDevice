#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef DIVOOM_SIMULATOR
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif
#include "divoom.h"
#include "include/divoom_http_server.h"
#include "divoom_http_server_port.h"
#ifdef DIVOOM_SIMULATOR
#include "sim_uhttpd_bridge.h"
#include "uhttpd/uhttpd.h"
#endif
#include <errno.h>
//#define DIVOOM_HTTPS_ENABLE

#include "divoom_net_command.h"
#include "divoom_net_command_json.h"
#include "divoom_net_app.h"
#include "divoom_game_runtime.h"
#include "divoom_watchface_local_api.h"
#include "divoom_lan_storage.h"
extern void divoom_lan_multipart(divoom_hs_conn_t *, int, int);
extern void divoom_lan_upload_abort(void *);

// SSL配置
#include "divoom_ssl_config.h"

// SSL相关头文件（真机 HTTPS；模拟器不编译 wolfssl）
#if defined(DIVOOM_HTTPS_ENABLE) && !defined(DIVOOM_SIMULATOR)
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/settings.h>
// 添加更多wolfSSL头文件
#include <wolfssl/options.h>
#endif

#define DVIOOM_HTTP_SERVER_PORT 9000
#define DVIOOM_HTTPS_SERVER_PORT 9443
#define DVIOOM_HTTP_SERVER_ROOT "/"

// SSL证书和密钥文件路径
#define DVIOOM_SSL_CERT_FILE DIVOOM_SSL_CERT_FILE
#define DVIOOM_SSL_KEY_FILE DIVOOM_SSL_KEY_FILE

#define FIFO_REQ DIVOOM_TMP_DIR "/fifo_req"
#define FIFO_RES DIVOOM_TMP_DIR "/fifo_res"
#define DIVOOM_HTTP_UPLOAD_FILE_BUFFER_SIZE (64 * 1024)
#define DIVOOM_HTTP_UPLOAD_FILE_BLOCK_SIZE  (32 * 1024)
#define DIVOOM_HTTP_UPLOAD_FILE_BLOCK_COUNT 2
/* PixelLocalHttp 上传进度阈值，单位 Byte；小文件不弹进度，大整页图片进入等待页。 */

typedef struct _divoom_http_server_item
{
    int cmd;
    void *sem_wait;
    char *ret_str;
}divoom_http_server_item, *Pdivoom_http_server_item;

typedef struct _divoom_http_server_con_item
{
    FILE *file_handle;
    int cur_file_size;
    int total_len;
    int accept_len;
    int file_len;
    uint8 *file_write_buffer;
    int file_write_buffer_len;
    int file_writer_buffer_len[DIVOOM_HTTP_UPLOAD_FILE_BLOCK_COUNT];
    int file_writer_fill_index;
    int file_writer_next_fill_index;
    int file_writer_next_write_index;
    volatile int file_writer_stop;
    volatile int file_writer_error;
    int file_writer_enabled;
    void *file_writer_ready_sem;
    void *file_writer_free_sem;
    void *file_writer_task;
    char upload_flag[128];
    char file_info[3072];
    char *json_str;
    void *conn_handle;
    uint8 pixel_upload_progress;
    unsigned char dial_bg_replace_upload;
    int dial_bg_replace_clock_id;
    /* multipart：第二段（或后续）写入磁盘后的完整路径，供 POST /create_local_clock 等复用 */
    char resolved_upload_path[512];
}divoom_http_server_con_item, *Pdivoom_http_server_con_item;

typedef struct _divoom_http_server_ctrl
{
    void *up_file_queue;
    void *cmd_queue;
#ifdef DIVOOM_HTTPS_ENABLE
    // SSL相关配置
    bool ssl_enabled;
    char *ssl_cert_file;
    char *ssl_key_file;
    void *ssl_ctx;  // wolfSSL context
#endif
}divoom_http_server_ctrl, *Pdivoom_http_server_ctrl;

/* PixelLocalHttp 上传进度由 HTTP 线程写入、表盘线程轮询。 */
typedef struct _divoom_http_server_pixel_upload_ctrl
{
    volatile int visible;
    volatile int percent;
    volatile int wait_page;
    void *owner;
}divoom_http_server_pixel_upload_ctrl;

Pdivoom_http_server_ctrl pdivoom_http_server_handle = NULL;
static divoom_http_server_pixel_upload_ctrl g_divoom_http_server_pixel_upload_ctrl;

int divoom_http_server_get_pixel_upload_progress(int *percent)
{
    int visible = g_divoom_http_server_pixel_upload_ctrl.visible;
    int value = g_divoom_http_server_pixel_upload_ctrl.percent;

    if(percent)
    {
        *percent = value;
    }
    return visible && value < 100;
}

int divoom_http_server_get_upload_wait_progress(int *percent)
{
    int active = g_divoom_http_server_pixel_upload_ctrl.visible &&
        g_divoom_http_server_pixel_upload_ctrl.wait_page;
    int value = g_divoom_http_server_pixel_upload_ctrl.percent;

    if(percent)
    {
        *percent = value;
    }
    return active;
}

void divoom_http_server_transfer_progress_begin(void *owner)
{
    if(owner == NULL)
    {
        return;
    }
    g_divoom_http_server_pixel_upload_ctrl.percent = 0;
    g_divoom_http_server_pixel_upload_ctrl.wait_page = 0;
    g_divoom_http_server_pixel_upload_ctrl.owner = owner;
    g_divoom_http_server_pixel_upload_ctrl.visible = 1;
}

static void divoom_http_server_upload_progress_begin(void *owner)
{
    divoom_http_server_transfer_progress_begin(owner);
    if(owner == g_divoom_http_server_pixel_upload_ctrl.owner)
    {
        g_divoom_http_server_pixel_upload_ctrl.wait_page = 1;
        divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP,
            DIVOOM_LIGHT_APP_EVENT_APP_SEND_PHOTO);
    }
}

void divoom_http_server_transfer_progress_update(void *owner, uint32 current,
                                                 uint32 total)
{
    int percent;

    if(owner != g_divoom_http_server_pixel_upload_ctrl.owner || total == 0U)
    {
        return;
    }
    percent = (int)(((T_U64)current * 100U) / total);
    if(percent >= 100)
    {
        percent = 100;
    }
    g_divoom_http_server_pixel_upload_ctrl.percent = percent;
    if(percent == 100 && g_divoom_http_server_pixel_upload_ctrl.wait_page == 0)
    {
        g_divoom_http_server_pixel_upload_ctrl.visible = 0;
        g_divoom_http_server_pixel_upload_ctrl.wait_page = 0;
        g_divoom_http_server_pixel_upload_ctrl.owner = NULL;
    }
}

void divoom_http_server_transfer_progress_finish(void *owner)
{
    if(owner == g_divoom_http_server_pixel_upload_ctrl.owner)
    {
        g_divoom_http_server_pixel_upload_ctrl.percent = 100;
        g_divoom_http_server_pixel_upload_ctrl.visible = 0;
        g_divoom_http_server_pixel_upload_ctrl.wait_page = 0;
        g_divoom_http_server_pixel_upload_ctrl.owner = NULL;
    }
}


int divoom_http_server_send_command_compare_handle(void *item1, void *item2)
{
    if(item1 == item2)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

int divoom_http_server_send_command_compare_id(void *item1, void *item2)
{
    Pdivoom_http_server_item pitem = (Pdivoom_http_server_item)item2;
    
    if(*(int *)item1 == pitem->cmd)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/*
  Function:       divoom_http_server_send_command
  Description:    向HTTP请求发送数据回civetweb
  Input:          cmd
                  cmd_string
  Return:         1：已经处理；0：未处理
*/

int divoom_http_server_send_command(int cmd,char *cmd_string)
{  
    Pdivoom_http_server_ctrl handle = __atomic_load_n(&pdivoom_http_server_handle, __ATOMIC_ACQUIRE);
    Pdivoom_http_server_item check_cmd;
    char *send_str = cmd_string;

    /* Unsolicited broadcasts can arrive before the optional HTTP service. */
    if(handle == NULL || handle->cmd_queue == NULL)
    {
        return 0;
    }
    check_cmd = divoom_queue_pop_cmp(handle->cmd_queue, &cmd, divoom_http_server_send_command_compare_id);
    if(check_cmd)
    {
        if(send_str == NULL)
        {
            send_str = "{\"ReturnCode\": 1, \"ReturnMessage\": \"Insufficient system resources\"}";
        }
        check_cmd->ret_str = divoom_app_com_copy_string(send_str, -1);
        divoom_system_put_sem(check_cmd->sem_wait);
        return 1;
    }
    else
    {
        divoom_print("pass the cmd: %d\n", cmd);
    }
            
    return 0;
    
}

char *divoom_http_server_filter_json_str(char  *file_name)
{   
    int len;
    char *ptr = file_name;

    while(*ptr && *ptr != '{')
    {
        ptr ++;
    }
    len = strlen(ptr) -1;
    while(len && ptr[len] != '}')
    {
        len --;
    }
    ptr[len + 1] = 0;

    return ptr;
    
}
void divoom_sigterm_handler(int sig);

#ifdef DIVOOM_HTTPS_ENABLE
/*
  Function:       divoom_http_server_ssl_init
  Description:    初始化SSL配置
  Input:          cert_file: 证书文件路径
                  key_file: 私钥文件路径
  Return:         0: 成功; -1: 失败
*/
int divoom_http_server_ssl_init(const char *cert_file, const char *key_file)
{
    if (!pdivoom_http_server_handle) {
        divoom_print("HTTP server handle not initialized\n");
        return -1;
    }

    // 检查证书和密钥文件是否存在
    if (!divoom_port_file_exists(cert_file)) {
        divoom_print("SSL certificate file not found: %s\n", cert_file);
        return -1;
    }
    
    if (!divoom_port_file_exists(key_file)) {
        divoom_print("SSL key file not found: %s\n", key_file);
        return -1;
    }

    // 初始化wolfSSL
    if (wolfSSL_Init() != SSL_SUCCESS) {
        divoom_print("Failed to initialize wolfSSL\n");
        return -1;
    }
    
    // 创建SSL上下文 - 使用默认方法
    WOLFSSL_CTX *ctx = wolfSSL_CTX_new(wolfSSLv23_server_method());
    if (!ctx) {
        divoom_print("Failed to create SSL context\n");
        return -1;
    }

    // 加载证书
    if (wolfSSL_CTX_use_certificate_file(ctx, cert_file, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        divoom_print("Failed to load SSL certificate: %s\n", cert_file);
        wolfSSL_CTX_free(ctx);
        return -1;
    }

    // 加载私钥
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        divoom_print("Failed to load SSL private key: %s\n", key_file);
        wolfSSL_CTX_free(ctx);
        return -1;
    }

    // 设置基本的SSL选项
    wolfSSL_CTX_set_options(ctx, WOLFSSL_OP_NO_SSLv2 | WOLFSSL_OP_NO_SSLv3);
    
    // 设置密码套件（使用简化版本）
    if (wolfSSL_CTX_set_cipher_list(ctx, DVIOOM_SSL_CIPHER_LIST) != SSL_SUCCESS) {
        divoom_print("Failed to set cipher list\n");
        wolfSSL_CTX_free(ctx);
        return -1;
    }

    pdivoom_http_server_handle->ssl_enabled = true;
    pdivoom_http_server_handle->ssl_cert_file = divoom_app_com_copy_string(cert_file, -1);
    pdivoom_http_server_handle->ssl_key_file = divoom_app_com_copy_string(key_file, -1);
    pdivoom_http_server_handle->ssl_ctx = ctx;

    divoom_print("SSL initialized successfully\n");
    return 0;
}

/*
  Function:       divoom_http_server_ssl_cleanup
  Description:    清理SSL资源
  Input:          void
  Return:         void
*/
void divoom_http_server_ssl_cleanup(void)
{
    if (pdivoom_http_server_handle && pdivoom_http_server_handle->ssl_ctx) {
        wolfSSL_CTX_free((WOLFSSL_CTX*)pdivoom_http_server_handle->ssl_ctx);
        pdivoom_http_server_handle->ssl_ctx = NULL;
        
        if (pdivoom_http_server_handle->ssl_cert_file) {
            divoom_system_free(pdivoom_http_server_handle->ssl_cert_file);
            pdivoom_http_server_handle->ssl_cert_file = NULL;
        }
        
        if (pdivoom_http_server_handle->ssl_key_file) {
            divoom_system_free(pdivoom_http_server_handle->ssl_key_file);
            pdivoom_http_server_handle->ssl_key_file = NULL;
        }
        
        pdivoom_http_server_handle->ssl_enabled = false;
        wolfSSL_Cleanup();
        divoom_print("SSL cleanup completed\n");
    }
}
#endif

static uint8 divoom_http_server_low_mem_busy(divoom_hs_conn_t *conn, int event)
{
    if(divoom_game_runtime_low_mem_guard_is_active() == 0)
    {
        return 0;
    }

    /*
     * Games 页面低内存保护态下尽早拒绝 HTTP 请求，尤其是上传接口，
     * 避免连接继续发送/服务端继续解析大 body。
     */
    if(event == DIVOOM_HS_EV_HEAD_COMPLETE || event == DIVOOM_HS_EV_COMPLETE)
    {
        divoom_hs_conn_send_head(conn, 503, -1,
            "Content-Type: application/json; charset=UTF-8\r\nConnection: close\r\n");
        divoom_hs_conn_chunk_printf(conn, "{\"ReturnCode\":1,\"ReturnMessage\":\"busy\"}");
        divoom_hs_conn_chunk_end(conn);
        divoom_hs_conn_done(conn);
    }

    return 1;
}

void divoom_http_server_default_handler_hs(divoom_hs_conn_t *conn, int event)
{
    if(divoom_http_server_low_mem_busy(conn, event))
    {
        return;
    }

    if (event != DIVOOM_HS_EV_COMPLETE)
    {
        return;
    }
    divoom_hs_conn_serve_file(conn);
}

void divoom_http_server_divoom_api_handler_hs(divoom_hs_conn_t *conn, int event)
{
    if(divoom_http_server_low_mem_busy(conn, event))
    {
        return;
    }

    //divoom_print("get event: %d\n", event);
    if (event == DIVOOM_HS_EV_COMPLETE) {
        Pdivoom_net_command pcmd_info;
        //divoom_hs_str_t path = conn->get_path(conn);
        //divoom_hs_str_t query = conn->get_query(conn);
        //divoom_hs_str_t ua = divoom_hs_conn_get_header(conn, "User-Agent");
        divoom_hs_str_t body = divoom_hs_conn_get_body(conn);
        char *json_str;
        json_str = divoom_system_malloc(body.len + 1);
        if(json_str == NULL)
        {
            divoom_hs_conn_send_head(conn, DIVOOM_HTTP_STATUS_OK, -1, "Content-Type: application/json; charset=UTF-8\r\n");
            divoom_hs_conn_chunk_printf(conn, "{\"ReturnCode\": 1, \"ReturnMessage\": \"Insufficient system resources\"}");
            divoom_hs_conn_chunk_end(conn);
            divoom_hs_conn_done(conn);
            return;
        }
        memcpy(json_str, body.p ,body.len);
        json_str[body.len] = 0;
        {
            int handled = 1;
            char *reply = body.len <= 65536 ? divoom_lan_dispatch(json_str, &handled) :
                divoom_watchface_local_http_error_json("", "JSON exceeds 65536 bytes");
            if(handled)
            {
                const char *response = reply ? reply : "{\"ReturnCode\":1,\"ReturnMessage\":\"oom\"}";
                divoom_system_free(json_str);
                divoom_hs_conn_send_head(conn, DIVOOM_HTTP_STATUS_OK, -1, "Content-Type: application/json; charset=UTF-8\r\n");
                divoom_hs_conn_chunk_send(conn, response, (int)strlen(response));
                if(reply) divoom_system_free(reply);
                divoom_hs_conn_chunk_end(conn);
                divoom_hs_conn_done(conn);
                return;
            }
        }
        divoom_print("we get http command: %s\n", json_str);
        pcmd_info = divoom_net_command_json_unpacket((char *)json_str, 1);
        
        divoom_hs_conn_send_head(conn, DIVOOM_HTTP_STATUS_OK, -1, "Content-Type: application/json; charset=UTF-8\r\n");
        
        if(pcmd_info)
        {
            Pdivoom_http_server_item cmd_info;
            int free_cmd_info = 1;
            
            pcmd_info->type  = DIVOOM_APP_TOPIC; 
            cmd_info = divoom_system_malloc(sizeof(divoom_http_server_item));
            if(cmd_info)
            {
                cmd_info->cmd    = pcmd_info->cmd;
                cmd_info->sem_wait = divoom_system_create_sem(1, 0);
                if(cmd_info->sem_wait == NULL ||
                    divoom_queue_push(pdivoom_http_server_handle->cmd_queue, cmd_info) == 0)
                {
                    divoom_print("the command is err!\n");
                    if(cmd_info->sem_wait)
                    {
                        divoom_system_destroy_sem(cmd_info->sem_wait);
                    }
                    divoom_system_free(cmd_info);
                    divoom_net_app_free_packet(pcmd_info);
                    divoom_hs_conn_chunk_printf(conn, "{\"ReturnCode\": 1, \"ReturnMessage\": \"Insufficient system resources\"}");
                    divoom_hs_conn_chunk_end(conn);
                    divoom_hs_conn_done(conn);
                    return;
                }
                divoom_print("wait command: %d\n", pcmd_info->cmd);
                if(pcmd_info->cmd == DIVOOM_NET_COMM_PHOTO_GET_LOCAL_PHOTO_LIST)
                {
                    divoom_net_app_push_quick_cmd(pcmd_info);
                }
                else
                {
                    divoom_net_app_push_cmd(pcmd_info);
                }
                if(divoom_system_get_sem_with_time(cmd_info->sem_wait, 5))
                {
                    if(cmd_info->ret_str)
                    {
                        divoom_hs_conn_chunk_send(conn, cmd_info->ret_str, strlen(cmd_info->ret_str));
                        divoom_print("we send http command: %s\n", cmd_info->ret_str);
                    }
                    else
                    {
                        divoom_print("the command is err! ret_str is NULL\n");
                        divoom_hs_conn_chunk_printf(conn, "{\"ReturnCode\": 1, \"ReturnMessage\": \"empty command response\"}");
                    }
                }
                else
                {
                    Pdivoom_http_server_item timeout_cmd;

                    timeout_cmd = divoom_queue_pop_cmp(pdivoom_http_server_handle->cmd_queue,
                        cmd_info, divoom_http_server_send_command_compare_handle);
                    if(timeout_cmd == NULL && divoom_system_get_sem_with_time(cmd_info->sem_wait, 1))
                    {
                        if(cmd_info->ret_str)
                        {
                            divoom_hs_conn_chunk_send(conn, cmd_info->ret_str, strlen(cmd_info->ret_str));
                            divoom_print("we send http command late: %s\n", cmd_info->ret_str);
                        }
                        else
                        {
                            divoom_print("the command is err! late ret_str is NULL\n");
                            divoom_hs_conn_chunk_printf(conn, "{\"ReturnCode\": 1, \"ReturnMessage\": \"empty command response\"}");
                        }
                    }
                    else
                    {
                        divoom_print("the command is err!\n");
                        divoom_hs_conn_chunk_printf(conn, "{\"ReturnCode\": 1, \"ReturnMessage\": \"command timeout\"}");
                        if(timeout_cmd == NULL)
                        {
                            free_cmd_info = 0;
                        }
                    }
                }
                if(free_cmd_info)
                {
                    divoom_system_destroy_sem(cmd_info->sem_wait);
                    if(cmd_info->ret_str)
                    {
                        divoom_system_free(cmd_info->ret_str);
                    }
                    divoom_system_free(cmd_info);
                }
            }
            else
            {
                divoom_print("the command is err!\n");
                divoom_hs_conn_chunk_printf(conn, "{\"ReturnCode\": 1, \"ReturnMessage\": \"Insufficient system resources\"}");
            }
        }
        else
        {
            divoom_print("pack command err!\n");
            divoom_hs_conn_chunk_printf(conn, "{\"ReturnCode\": 1, \"ReturnMessage\": \"Only accept JSON parameters\"}");
        }


        divoom_hs_conn_chunk_end(conn);
        divoom_hs_conn_done(conn);
    }
}
#if 1
static int divoom_http_server_upload_write_block(
    Pdivoom_http_server_con_item pup_file_handle, const uint8 *buffer, int write_len)
{
    size_t written;

    if(pup_file_handle == NULL || pup_file_handle->file_handle == NULL ||
       buffer == NULL || write_len <= 0)
    {
        return 0;
    }

    written = divoom_port_file_write(buffer, 1,
        (size_t)write_len, pup_file_handle->file_handle);

    if(written != (size_t)write_len)
    {
        divoom_print("[http_upload_write] failed request=%d written=%u file=%d/%d\n",
            write_len, (unsigned)written, pup_file_handle->file_len,
            pup_file_handle->cur_file_size);
        return -1;
    }
    return 0;
}

static void *divoom_http_server_upload_file_writer_task(void *param)
{
    Pdivoom_http_server_con_item pup_file_handle =
        (Pdivoom_http_server_con_item)param;

    for(;;)
    {
        int buffer_index;
        int write_len;

        divoom_system_get_sem(pup_file_handle->file_writer_ready_sem);
        if(pup_file_handle->file_writer_stop)
        {
            break;
        }

        buffer_index = pup_file_handle->file_writer_next_write_index;
        pup_file_handle->file_writer_next_write_index =
            (buffer_index + 1) % DIVOOM_HTTP_UPLOAD_FILE_BLOCK_COUNT;
        write_len = pup_file_handle->file_writer_buffer_len[buffer_index];
        if(divoom_http_server_upload_write_block(pup_file_handle,
            pup_file_handle->file_write_buffer +
                buffer_index * DIVOOM_HTTP_UPLOAD_FILE_BLOCK_SIZE,
            write_len) != 0)
        {
            pup_file_handle->file_writer_error = 1;
        }
        pup_file_handle->file_writer_buffer_len[buffer_index] = 0;
        divoom_system_put_sem(pup_file_handle->file_writer_free_sem);
    }
    return NULL;
}

static int divoom_http_server_upload_file_writer_start(
    Pdivoom_http_server_con_item pup_file_handle)
{
    pup_file_handle->file_writer_fill_index = -1;
    pup_file_handle->file_writer_next_fill_index = 0;
    pup_file_handle->file_writer_next_write_index = 0;
    pup_file_handle->file_writer_stop = 0;
    pup_file_handle->file_writer_error = 0;
    pup_file_handle->file_writer_buffer_len[0] = 0;
    pup_file_handle->file_writer_buffer_len[1] = 0;
    pup_file_handle->file_writer_ready_sem =
        divoom_system_create_sem(DIVOOM_HTTP_UPLOAD_FILE_BLOCK_COUNT + 1, 0);
    pup_file_handle->file_writer_free_sem =
        divoom_system_create_sem(DIVOOM_HTTP_UPLOAD_FILE_BLOCK_COUNT,
            DIVOOM_HTTP_UPLOAD_FILE_BLOCK_COUNT);
    if(pup_file_handle->file_writer_ready_sem == NULL ||
       pup_file_handle->file_writer_free_sem == NULL)
    {
        goto fail;
    }
    pup_file_handle->file_writer_task = divoom_system_create_joinable_task(
        divoom_http_server_upload_file_writer_task, "http_file_wr",
        pup_file_handle, DIVOOM_TASK_STACK_4K);
    if(pup_file_handle->file_writer_task == NULL)
    {
        goto fail;
    }
    pup_file_handle->file_writer_enabled = 1;
    return 0;

fail:
    if(pup_file_handle->file_writer_ready_sem)
    {
        divoom_system_destroy_sem(pup_file_handle->file_writer_ready_sem);
        pup_file_handle->file_writer_ready_sem = NULL;
    }
    if(pup_file_handle->file_writer_free_sem)
    {
        divoom_system_destroy_sem(pup_file_handle->file_writer_free_sem);
        pup_file_handle->file_writer_free_sem = NULL;
    }
    pup_file_handle->file_writer_task = NULL;
    pup_file_handle->file_writer_enabled = 0;
    pup_file_handle->file_writer_fill_index = 0;
    divoom_print("[http_upload_async] start failed, fallback synchronous\n");
    return -1;
}

static void divoom_http_server_upload_acquire_file_buffer(
    Pdivoom_http_server_con_item pup_file_handle)
{
    if(pup_file_handle->file_writer_fill_index >= 0)
    {
        return;
    }
    if(!pup_file_handle->file_writer_enabled)
    {
        pup_file_handle->file_writer_fill_index = 0;
        return;
    }

    divoom_system_get_sem(pup_file_handle->file_writer_free_sem);
    pup_file_handle->file_writer_fill_index =
        pup_file_handle->file_writer_next_fill_index;
    pup_file_handle->file_writer_next_fill_index =
        (pup_file_handle->file_writer_next_fill_index + 1) %
            DIVOOM_HTTP_UPLOAD_FILE_BLOCK_COUNT;
}

static int divoom_http_server_upload_flush_file_buffer(Pdivoom_http_server_con_item pup_file_handle)
{
    int buffer_index;
    int write_len;

    if(pup_file_handle == NULL || pup_file_handle->file_handle == NULL ||
       pup_file_handle->file_write_buffer == NULL ||
       pup_file_handle->file_write_buffer_len <= 0)
    {
        return 0;
    }

    divoom_http_server_upload_acquire_file_buffer(pup_file_handle);
    buffer_index = pup_file_handle->file_writer_fill_index;
    write_len = pup_file_handle->file_write_buffer_len;
    pup_file_handle->file_write_buffer_len = 0;
    if(!pup_file_handle->file_writer_enabled)
    {
        return divoom_http_server_upload_write_block(pup_file_handle,
            pup_file_handle->file_write_buffer +
                buffer_index * DIVOOM_HTTP_UPLOAD_FILE_BLOCK_SIZE,
            write_len);
    }

    pup_file_handle->file_writer_buffer_len[buffer_index] = write_len;
    pup_file_handle->file_writer_fill_index = -1;
    divoom_system_put_sem(pup_file_handle->file_writer_ready_sem);
    return 0;
}

static void divoom_http_server_upload_file_writer_finish(
    Pdivoom_http_server_con_item pup_file_handle, int flush_buffer)
{
    if(!pup_file_handle->file_writer_enabled)
    {
        return;
    }

    if(pup_file_handle->file_writer_fill_index >= 0)
    {
        if(flush_buffer && pup_file_handle->file_write_buffer_len > 0)
        {
            divoom_http_server_upload_flush_file_buffer(pup_file_handle);
        }
        else
        {
            pup_file_handle->file_write_buffer_len = 0;
            pup_file_handle->file_writer_fill_index = -1;
            divoom_system_put_sem(pup_file_handle->file_writer_free_sem);
        }
    }

    divoom_system_get_sem(pup_file_handle->file_writer_free_sem);
    divoom_system_get_sem(pup_file_handle->file_writer_free_sem);

    pup_file_handle->file_writer_stop = 1;
    divoom_system_put_sem(pup_file_handle->file_writer_ready_sem);
    divoom_system_join_task(pup_file_handle->file_writer_task);
    divoom_system_destroy_sem(pup_file_handle->file_writer_ready_sem);
    divoom_system_destroy_sem(pup_file_handle->file_writer_free_sem);
    pup_file_handle->file_writer_ready_sem = NULL;
    pup_file_handle->file_writer_free_sem = NULL;
    pup_file_handle->file_writer_task = NULL;
    pup_file_handle->file_writer_enabled = 0;
}

static void divoom_http_server_upload_buffer_file_data(
    Pdivoom_http_server_con_item pup_file_handle, const char *src_info, int data_len)
{
    int copy_len;
    int space_len;

    if(pup_file_handle == NULL || src_info == NULL || data_len <= 0 ||
       pup_file_handle->file_write_buffer == NULL)
    {
        return;
    }

    while(data_len > 0)
    {
        divoom_http_server_upload_acquire_file_buffer(pup_file_handle);
        space_len = DIVOOM_HTTP_UPLOAD_FILE_BLOCK_SIZE -
            pup_file_handle->file_write_buffer_len;
        copy_len = data_len < space_len ? data_len : space_len;
        memcpy(pup_file_handle->file_write_buffer +
            pup_file_handle->file_writer_fill_index *
                DIVOOM_HTTP_UPLOAD_FILE_BLOCK_SIZE +
            pup_file_handle->file_write_buffer_len, src_info, (size_t)copy_len);
        pup_file_handle->file_write_buffer_len += copy_len;
        src_info += copy_len;
        data_len -= copy_len;

        if(pup_file_handle->file_write_buffer_len ==
           DIVOOM_HTTP_UPLOAD_FILE_BLOCK_SIZE)
        {
            divoom_http_server_upload_flush_file_buffer(pup_file_handle);
        }
    }
}

static void divoom_http_server_upload_close_file(
    Pdivoom_http_server_con_item pup_file_handle, int flush_buffer)
{
    if(pup_file_handle == NULL)
    {
        return;
    }
    if(pup_file_handle->file_handle)
    {
        if(pup_file_handle->file_writer_enabled)
        {
            divoom_http_server_upload_file_writer_finish(pup_file_handle,
                flush_buffer);
        }
        else if(flush_buffer)
        {
            divoom_http_server_upload_flush_file_buffer(pup_file_handle);
        }
        divoom_port_file_close(pup_file_handle->file_handle);
        pup_file_handle->file_handle = NULL;
    }
    pup_file_handle->file_write_buffer_len = 0;
}

static void divoom_http_server_upload_free_file_buffer(
    Pdivoom_http_server_con_item pup_file_handle)
{
    if(pup_file_handle && pup_file_handle->file_write_buffer)
    {
        if(pup_file_handle->file_writer_enabled)
        {
            divoom_http_server_upload_file_writer_finish(pup_file_handle, 0);
        }
        divoom_system_free(pup_file_handle->file_write_buffer);
        pup_file_handle->file_write_buffer = NULL;
        pup_file_handle->file_write_buffer_len = 0;
    }
}

/* multipart 段头里 Content-Length（RFC 7230 字段名大小写不敏感） */
static char *divoom_http_multipart_find_content_length(char *src_info)
{
    char *p;

    for(p = src_info; p != NULL && *p != 0; )
    {
        if(strncasecmp(p, "content-length:", 15) == 0)
            return p;
        p = strchr(p, '\n');
        if(p == NULL)
            break;
        p++;
    }
    return NULL;
}

char * divoom_http_server_upload_get_file_info(Pdivoom_http_server_con_item pup_file_handle, char *src_info)
{
    int len, base_len;
    char *find_str = src_info, *end_str, file_name[512];

    find_str = strstr(src_info, "filename=\"");
    if(find_str == NULL)
    {
        divoom_print("[http_upload] multipart filename missing\n");
        return  NULL;
    }

    if(pup_file_handle->dial_bg_replace_upload)
    {
        divoom_watchface_replace_clock_dial_bg_cache_path(pup_file_handle->dial_bg_replace_clock_id, file_name, (int)sizeof file_name);
        if(file_name[0] == 0 || pup_file_handle->dial_bg_replace_clock_id <= 0)
        {
            divoom_print("replace dial bg: empty cache path for clock %d\n", pup_file_handle->dial_bg_replace_clock_id);
            return NULL;
        }
        divoom_app_com_check_and_mk_dir(file_name);
    }
    else
    {
        find_str += strlen("filename=\"");
        end_str = strchr(find_str, '\"');
        if(end_str == NULL)
        {
            divoom_print("[http_upload] malformed multipart filename\n");
            return  NULL;
        }
        len = (int)(end_str - find_str);
        strcpy(file_name, DIVOOM_SD_APP_PIC_DIR);
        strcat(file_name, "/");
        base_len = strlen(file_name);
        memcpy(&file_name[base_len], find_str, len);
        file_name[base_len + len] = 0;
        divoom_app_com_mk_dir(DIVOOM_SD_APP_PIC_DIR);
    }

    find_str = divoom_http_multipart_find_content_length(src_info);
    if(find_str == NULL)
    {
        divoom_print("[http_upload] multipart Content-Length missing\n");
        return NULL;
    }
    find_str += 15;
    while(*find_str && (*find_str < '0' || *find_str >'9'))
    {
        find_str ++;
    }
    pup_file_handle->cur_file_size  = atoi(find_str);

    divoom_print("[storage_io] upload_open via=%s path=%s bytes=%d free=%uKB\n",
        __func__, file_name, pup_file_handle->cur_file_size, (uint32)divoom_port_fs_free_space(DIVOOM_USER_ROOT));
    pup_file_handle->file_handle = divoom_port_file_open(file_name, "wb");
    if(pup_file_handle->file_handle == NULL)
    {
        divoom_print("fopen %s failed errno %d\n", file_name, errno);
        return  NULL;
    }
    if(pup_file_handle->file_write_buffer == NULL)
    {
        pup_file_handle->file_write_buffer =
            divoom_system_malloc_no_zero(DIVOOM_HTTP_UPLOAD_FILE_BUFFER_SIZE);
        if(pup_file_handle->file_write_buffer == NULL)
        {
            divoom_print("[http_upload_write] alloc %d failed\n",
                DIVOOM_HTTP_UPLOAD_FILE_BUFFER_SIZE);
            divoom_http_server_upload_close_file(pup_file_handle, 0);
            return NULL;
        }
    }
    pup_file_handle->file_write_buffer_len = 0;
    divoom_http_server_upload_file_writer_start(pup_file_handle);
    strncpy(pup_file_handle->resolved_upload_path, file_name, sizeof(pup_file_handle->resolved_upload_path) - 1);
    pup_file_handle->resolved_upload_path[sizeof(pup_file_handle->resolved_upload_path) - 1] = 0;

    find_str = strstr(find_str, "\r\n\r\n");
    if(find_str == NULL)
    {
        divoom_print("[http_upload] multipart file header incomplete\n");
        pup_file_handle->cur_file_size = 0;
        pup_file_handle->file_len = 0;
        divoom_http_server_upload_close_file(pup_file_handle, 0);
        return NULL;
    }
    pup_file_handle->file_len = 0;
    find_str += strlen("\r\n\r\n");
    return find_str;

}

void divoom_http_server_upload_do_file_data(Pdivoom_http_server_con_item pup_file_handle, char *src_info, int data_len)
{
    int tmp_len, off, file_data_len;
    char *next_content;

    if(data_len == 0)
    {
        return;
    }
    //
    if(pup_file_handle->file_handle == NULL)
    {
        //文件没有打开的话，将先取文件内容，如果内容足够，先取文件内容，不足够的话，将文件内容保持到下次使用
        tmp_len = strlen(pup_file_handle->file_info);
        if(tmp_len + data_len < sizeof(pup_file_handle->file_info))
        {
            memcpy(&pup_file_handle->file_info[tmp_len], src_info, data_len);
            pup_file_handle->file_info[tmp_len + data_len] = 0;
            off = data_len;
        }
        else
        {
            //假定文件头信息不会超过sizeof(pup_file_handle->file_info)
            memcpy(&pup_file_handle->file_info[tmp_len], src_info, sizeof(pup_file_handle->file_info) - tmp_len - 1);
            pup_file_handle->file_info[sizeof(pup_file_handle->file_info) - 1] = 0;
            off = sizeof(pup_file_handle->file_info) - tmp_len - 1;
        }
        next_content = divoom_http_server_upload_get_file_info(pup_file_handle, pup_file_handle->file_info);
        if(next_content)
        {
            //文件已经打开，将递归读取内容
            off = ((uint32)next_content - (uint32)pup_file_handle->file_info) - tmp_len;  //这个偏移是要减去用了的长度
            divoom_http_server_upload_do_file_data(pup_file_handle, &src_info[off], data_len - off);
        }
    }
    else
    {
        file_data_len = pup_file_handle->cur_file_size - pup_file_handle->file_len;
        if(file_data_len > data_len)
        {
            file_data_len = data_len;
        }
        if(file_data_len > 0)
        {
            divoom_http_server_upload_buffer_file_data(pup_file_handle,
                src_info, file_data_len);
            pup_file_handle->file_len += file_data_len;
            src_info += file_data_len;
            data_len -= file_data_len;
        }
        if(pup_file_handle->file_len >= pup_file_handle->cur_file_size)
        {
            //处理完文件后再递归调用这个函数处理其它文件
            divoom_http_server_upload_close_file(pup_file_handle, 1);
            pup_file_handle->file_info[0] = 0;
            if(data_len > 0)
            {
                divoom_http_server_upload_do_file_data(pup_file_handle, src_info, data_len);
            }
        }
    }
    

}

void divoom_http_server_upload_check_local_photo(char *json_str)
{
    Pdivoom_net_command pcmd_info;

    pcmd_info = divoom_net_command_json_unpacket(json_str, 0);
    if(pcmd_info)
    {
        if(pcmd_info->cmd == DIVOOM_NET_COMM_PHOTO_APP_ADD_PHOTO)
        {
            Pdivoom_photo_album_app_local_photo_info papp_info = (Pdivoom_photo_album_app_local_photo_info)pcmd_info->cmd_data;

            if(papp_info->photo_index == 0)
            {
                divoom_download_aliyun_file_set_app_last_info(papp_info->photo_total, papp_info->photo_index);
                divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_APP_SEND_PHOTO);
            }
        }
        divoom_net_app_free_packet(pcmd_info);
    }

}

char * divoom_http_server_upload_do_first_data(Pdivoom_http_server_con_item pup_file_handle, char *src_info, int data_len)
{
    int len, write_len;
    char *json_str = NULL, *find_str, *end_str, *content_ptr;
    char *start_flag = pup_file_handle->upload_flag;

    find_str = strchr(src_info, 0x0d);
    if(find_str == NULL)
    {
        divoom_print("[http_upload] multipart boundary missing\n");
        return NULL;
    }
    len = (int)(find_str - src_info);
    memcpy(start_flag, src_info, len);
    start_flag[len] = 0;
    find_str = strstr(find_str, "\r\n\r\n");
    if(find_str == NULL)
    {
        divoom_print("[http_upload] multipart JSON header incomplete\n");
        return NULL;
    }
    find_str += strlen("\r\n\r\n");

    find_str = strchr(find_str, '{');
    if(find_str == NULL)
    {
        divoom_print("[http_upload] multipart JSON body missing\n");
        return NULL;
    }
    end_str = strstr(find_str, start_flag);
    if(end_str == NULL)
    {
        divoom_print("[http_upload] multipart JSON boundary missing\n");
        return json_str;
    }
    while(*end_str != '}')
    {
        end_str --;
    }
    end_str ++;
    len = (int)(end_str - find_str);
    
    json_str = divoom_system_malloc(len  +1);
    memcpy(json_str, find_str, len);
    json_str[len] = 0;

    divoom_http_server_upload_check_local_photo(json_str);
    content_ptr = end_str + strlen(start_flag);
    write_len = (int)(((char *)src_info + data_len) - content_ptr);
    if(write_len > 0)
    {
        divoom_http_server_upload_do_file_data(pup_file_handle, content_ptr, write_len);
    }
    return json_str;
}

void divoom_http_server_upload_do_other_data(Pdivoom_http_server_con_item pup_file_handle, char *src_info, int data_len)
{    
    divoom_http_server_upload_do_file_data(pup_file_handle, src_info, data_len);
}

static char *divoom_http_server_replace_dial_bg_do_first_data(Pdivoom_http_server_con_item pup_file_handle, char *src_info, int data_len)
{
    int len;
    char *json_str = NULL, *find_str, *end_str;
    char *start_flag = pup_file_handle->upload_flag;
    Pdivoom_net_command pcmd;
    int cid;

    pup_file_handle->dial_bg_replace_upload = 0;
    pup_file_handle->dial_bg_replace_clock_id = -1;

    find_str = strchr(src_info, 0x0d);
    if(find_str == NULL)
    {
        divoom_print("replace dial bg: no cr\n");
        return NULL;
    }
    len = (int)(find_str - src_info);
    memcpy(start_flag, src_info, len);
    start_flag[len] = 0;
    find_str = strstr(find_str, "\r\n\r\n");
    if(find_str == NULL)
    {
        divoom_print("replace dial bg: no header end\n");
        return NULL;
    }
    find_str += strlen("\r\n\r\n");

    find_str = strchr(find_str, '{');
    if(find_str == NULL)
    {
        divoom_print("replace dial bg: no json\n");
        return NULL;
    }
    end_str = strstr(find_str, start_flag);
    if(end_str == NULL)
    {
        divoom_print("replace dial bg: no json end boundary\n");
        return json_str;
    }
    while(*end_str != '}')
    {
        end_str --;
    }
    end_str ++;
    len = (int)(end_str - find_str);

    json_str = divoom_system_malloc(len + 1);
    if(json_str == NULL)
        return NULL;
    memcpy(json_str, find_str, len);
    json_str[len] = 0;

    pcmd = divoom_net_command_json_unpacket(json_str, 0);
    divoom_system_free(json_str);
    json_str = NULL;
    if(pcmd == NULL)
        return NULL;
    if(pcmd->cmd != DIVOOM_NET_COMM_DEVICE_REPLACE_CLOCK_DIAL_BG_FILE || pcmd->cmd_data == NULL)
    {
        divoom_net_app_free_packet(pcmd);
        return NULL;
    }
    cid = divoom_watchface_replace_clock_dial_bg_resolve_clock_id((Pdivoom_clock_id_info)pcmd->cmd_data);
    divoom_net_app_free_packet(pcmd);
    if(cid <= 0)
    {
        divoom_print("replace dial bg: invalid clock id\n");
        return NULL;
    }
    pup_file_handle->dial_bg_replace_clock_id = cid;
    pup_file_handle->dial_bg_replace_upload = 1;

    {
        char *content_ptr = end_str + strlen(start_flag);
        int write_len = (int)(((char *)src_info + data_len) - content_ptr);

        if(write_len > 0)
        {
            divoom_http_server_upload_do_file_data(pup_file_handle, content_ptr, write_len);
        }
    }
    return NULL;
}
#endif

int divoom_http_server_upload_handler_compare_handle(void *item1, void *item2)
{
    Pdivoom_http_server_con_item pup_file_handle = (Pdivoom_http_server_con_item)item2;

    return item1 == pup_file_handle->conn_handle?0:1;
}

void divoom_http_server_upload_abort_hs(void *conn_handle)
{
    divoom_lan_upload_abort(conn_handle);
    Pdivoom_http_server_con_item pup_file_handle;

    if(pdivoom_http_server_handle == NULL ||
       pdivoom_http_server_handle->up_file_queue == NULL || conn_handle == NULL)
    {
        return;
    }

    pup_file_handle = divoom_queue_pop_cmp(pdivoom_http_server_handle->up_file_queue,
        conn_handle, divoom_http_server_upload_handler_compare_handle);
    if(pup_file_handle == NULL)
    {
        return;
    }

    divoom_http_server_transfer_progress_finish(conn_handle);
    divoom_download_aliyun_file_set_download_flag(0);
    if(pup_file_handle->json_str)
    {
        divoom_system_free(pup_file_handle->json_str);
    }
    if(pup_file_handle->file_handle)
    {
        divoom_http_server_upload_close_file(pup_file_handle, 0);
    }
    divoom_http_server_upload_free_file_buffer(pup_file_handle);
    divoom_system_free(pup_file_handle);
}

static int divoom_http_server_upload_is_pixel_local_http(const char *json_str)
{
    int matched = 0;
    Pdivoom_net_command pcmd_info;

    if(json_str == NULL)
    {
        return 0;
    }
    pcmd_info = divoom_net_command_json_unpacket((char *)json_str, 0);
    if(pcmd_info)
    {
        matched = pcmd_info->cmd == DIVOOM_NET_COMM_DRAWING_PIXEL_LOCAL_HTTP;
        divoom_net_app_free_packet(pcmd_info);
    }
    return matched;
}

void divoom_http_server_upload_handler_hs(divoom_hs_conn_t *conn, int event)
{
    Pdivoom_http_server_con_item pup_file_handle;
    void *conn_handle = divoom_hs_conn_handle(conn);

    if(divoom_http_server_low_mem_busy(conn, event))
    {
        return;
    }
    
    pup_file_handle = divoom_queue_get_cmp(pdivoom_http_server_handle->up_file_queue, conn_handle, divoom_http_server_upload_handler_compare_handle);
    if(pup_file_handle == NULL)
    {
        pup_file_handle  = divoom_system_malloc(sizeof(divoom_http_server_con_item));
        if(pup_file_handle == NULL)
        {
            return;
        }
        memset(pup_file_handle, 0, sizeof(divoom_http_server_con_item));
        pup_file_handle->dial_bg_replace_clock_id = -1;
        pup_file_handle->conn_handle = conn_handle;
        divoom_queue_push(pdivoom_http_server_handle->up_file_queue, pup_file_handle);
    }
    if(event == DIVOOM_HS_EV_HEAD_COMPLETE)
    {
        divoom_hs_str_t str = divoom_hs_conn_get_header(conn, "Content-Length");
        int content_length = 0;
        char buf[48];

        buf[0] = 0;
        if(str.p != NULL && str.len > 0 && str.len < (int)sizeof(buf))
        {
            memcpy(buf, str.p, (size_t)str.len);
            buf[str.len] = 0;
            content_length = atoi(buf);
        }

        pup_file_handle->accept_len = 0;
        pup_file_handle->file_len = 0;
        pup_file_handle->file_write_buffer_len = 0;
        pup_file_handle->pixel_upload_progress = 0;
        pup_file_handle->total_len = content_length;
        pup_file_handle->file_info[0] = 0;

        if(pup_file_handle->file_handle)
        {
            divoom_http_server_upload_close_file(pup_file_handle, 0);
        }

        /* 30 分钟约 1 Mbps 视频加封面仍可流式上传，单位：字节。 */
        if(content_length <= 0 || content_length > 512 * 1024 * 1024)
        {
            divoom_hs_conn_error(conn, DIVOOM_HTTP_STATUS_INTERNAL_SERVER_ERROR, "Too big");
            return;
        }
    }
    else if(event == DIVOOM_HS_EV_BODY)
    {
        divoom_hs_str_t body = divoom_hs_conn_extract_body(conn);

        #if 1
        if(pup_file_handle->accept_len  == 0)
        {
            pup_file_handle->json_str = divoom_http_server_upload_do_first_data(pup_file_handle, (char *)body.p, body.len);
            if(divoom_http_server_upload_is_pixel_local_http(pup_file_handle->json_str) &&
                pup_file_handle->cur_file_size >=
                    DIVOOM_HTTP_TRANSFER_PROGRESS_MIN_FILE_SIZE)
            {
                pup_file_handle->pixel_upload_progress = 1;
                if(pup_file_handle->cur_file_size >=
                    DIVOOM_HTTP_TRANSFER_PROGRESS_FULL_PAGE_FILE_SIZE)
                {
                    divoom_http_server_upload_progress_begin(conn_handle);
                }
                else
                {
                    divoom_http_server_transfer_progress_begin(conn_handle);
                }
            }
            divoom_download_aliyun_file_set_download_flag(1);
        }
        else
        {
            divoom_http_server_upload_do_other_data(pup_file_handle, (char *)body.p, body.len);
        }  
        #else
        {
            if(pup_file_handle->file_handle == NULL)
            {
                divoom_print("[storage_io] upload_open via=%s path=/userdata/http.bin free=%uKB\n",
                    __func__, (uint32)divoom_port_fs_free_space(DIVOOM_USER_ROOT));
                pup_file_handle->file_handle = divoom_port_file_open("/userdata/http.bin", "wb");
            }
            divoom_port_file_write(body.p, 1, body.len, pup_file_handle->file_handle);
            
        }
        #endif
        pup_file_handle->accept_len += body.len;
        if(pup_file_handle->pixel_upload_progress && pup_file_handle->cur_file_size > 0)
        {
            divoom_http_server_transfer_progress_update(conn_handle,
                (uint32)pup_file_handle->file_len,
                (uint32)pup_file_handle->cur_file_size);
        }
    }
    else if(event == DIVOOM_HS_EV_COMPLETE)
    {
        divoom_hs_conn_send_head(conn, DIVOOM_HTTP_STATUS_OK, -1, "Content-Type: application/json; charset=UTF-8\r\n");
        
        divoom_download_aliyun_file_set_download_flag(0);

        if(pup_file_handle->file_handle)
        {
            divoom_http_server_upload_close_file(pup_file_handle, 1);
        }
        if(pup_file_handle->json_str)
        {
            Pdivoom_net_command pcmd_info;

            pcmd_info = divoom_net_command_json_unpacket((char *)pup_file_handle->json_str, 0);
            if(pcmd_info)
            {
                divoom_net_app_push_cmd(pcmd_info);
            }
            divoom_system_free(pup_file_handle->json_str);
            pup_file_handle->json_str = NULL;
        }
        /* 文件写线程完成并且动画命令已入队后，等待页才进入完成态。 */
        divoom_http_server_transfer_progress_finish(conn_handle);
        pup_file_handle = divoom_queue_pop_cmp(pdivoom_http_server_handle->up_file_queue, divoom_hs_conn_handle(conn), divoom_http_server_upload_handler_compare_handle);
        if(pup_file_handle->json_str)
        {
            divoom_system_free(pup_file_handle->json_str);
        }
        if(pup_file_handle->file_handle)
        {
            divoom_http_server_upload_close_file(pup_file_handle, 1);
        }
        divoom_http_server_upload_free_file_buffer(pup_file_handle);
        divoom_system_free(pup_file_handle);

        divoom_hs_conn_chunk_printf(conn, "{\"ReturnCode\": 0, \"ReturnMessage\": \"\"}");
        divoom_hs_conn_chunk_end(conn);
        divoom_hs_conn_done(conn);
    }
}

void divoom_http_server_upload_local_asset_handler_hs(divoom_hs_conn_t *conn, int event)
{
    if(divoom_http_server_low_mem_busy(conn, event)) {
        divoom_lan_upload_abort(divoom_hs_conn_handle(conn));
        return;
    }
    if(event == DIVOOM_HS_EV_HEAD_COMPLETE || event == DIVOOM_HS_EV_COMPLETE)
    {
        divoom_print("[storage_io] lan_upload_event via=%s event=%d free=%uKB\n",
            __func__, event, (uint32)divoom_port_fs_free_space(DIVOOM_USER_ROOT));
    }
    divoom_lan_multipart(conn, event, 0);
}

/*
 * POST /create_local_clock — multipart/form-data：第一段 JSON（Command=Device/CreateLocalClock），第二段为
 * 单张底图（JPG/WebP，与 Replace 规则一致）或 **tar.gz** 包（内含 clock_bg.* 与可选元素图，见 API 文档）。
 * 行为见 docs Divoom_Watchface_Remote_Customization_Guide_* §4.x。
 */
void divoom_http_server_create_local_clock_handler_hs(divoom_hs_conn_t *conn, int event)
{
    if(divoom_http_server_low_mem_busy(conn, event)) {
        divoom_lan_upload_abort(divoom_hs_conn_handle(conn));
        return;
    }
    if(event == DIVOOM_HS_EV_HEAD_COMPLETE || event == DIVOOM_HS_EV_COMPLETE)
    {
        divoom_print("[storage_io] lan_upload_event via=%s event=%d free=%uKB\n",
            __func__, event, (uint32)divoom_port_fs_free_space(DIVOOM_USER_ROOT));
    }
    divoom_lan_multipart(conn, event, 1);
    return;
    Pdivoom_http_server_con_item pup_file_handle;

    if(divoom_http_server_low_mem_busy(conn, event))
    {
        return;
    }

    pup_file_handle = divoom_queue_get_cmp(pdivoom_http_server_handle->up_file_queue, divoom_hs_conn_handle(conn), divoom_http_server_upload_handler_compare_handle);
    if(pup_file_handle == NULL)
    {
        pup_file_handle  = divoom_system_malloc(sizeof(divoom_http_server_con_item));
        if(pup_file_handle == NULL)
        {
            return;
        }
        memset(pup_file_handle, 0, sizeof(divoom_http_server_con_item));
        pup_file_handle->dial_bg_replace_clock_id = -1;
        pup_file_handle->conn_handle = divoom_hs_conn_handle(conn);
        divoom_queue_push(pdivoom_http_server_handle->up_file_queue, pup_file_handle);
    }
    if(event == DIVOOM_HS_EV_HEAD_COMPLETE)
    {
        divoom_hs_str_t str = divoom_hs_conn_get_header(conn, "Content-Length");
        int content_length = 0;
        char buf[48];

        buf[0] = 0;
        if(str.p != NULL && str.len > 0 && str.len < (int)sizeof(buf))
        {
            memcpy(buf, str.p, (size_t)str.len);
            buf[str.len] = 0;
            content_length = atoi(buf);
        }

        pup_file_handle->accept_len = 0;
        pup_file_handle->file_len = 0;
        pup_file_handle->file_write_buffer_len = 0;
        pup_file_handle->total_len = content_length;
        pup_file_handle->file_info[0] = 0;

        if(pup_file_handle->file_handle)
        {
            divoom_http_server_upload_close_file(pup_file_handle, 0);
        }

        if(content_length > 100 * 1024 * 1024)
        {
            divoom_hs_conn_error(conn, DIVOOM_HTTP_STATUS_INTERNAL_SERVER_ERROR, "Too big");
            return;
        }
    }
    else if(event == DIVOOM_HS_EV_BODY)
    {
        divoom_hs_str_t body = divoom_hs_conn_extract_body(conn);

        if(pup_file_handle->accept_len  == 0)
        {
            pup_file_handle->json_str = divoom_http_server_upload_do_first_data(pup_file_handle, (char *)body.p, body.len);
            divoom_download_aliyun_file_set_download_flag(1);
        }
        else
        {
            divoom_http_server_upload_do_other_data(pup_file_handle, (char *)body.p, body.len);
        }
        pup_file_handle->accept_len += body.len;
    }
    else if(event == DIVOOM_HS_EV_COMPLETE)
    {
        char *rj = NULL;

        divoom_hs_conn_send_head(conn, DIVOOM_HTTP_STATUS_OK, -1, "Content-Type: application/json; charset=UTF-8\r\n");

        divoom_download_aliyun_file_set_download_flag(0);

        if(pup_file_handle->file_handle)
        {
            divoom_http_server_upload_close_file(pup_file_handle, 1);
        }
        if(pup_file_handle->json_str && pup_file_handle->resolved_upload_path[0] != 0
            && pup_file_handle->file_len > 0 && pup_file_handle->file_len == pup_file_handle->cur_file_size)
        {
            rj = divoom_watchface_local_http_create_local_clock(
                pup_file_handle->json_str,
                pup_file_handle->resolved_upload_path,
                pup_file_handle->file_len);
        }
        else if(pup_file_handle->json_str)
        {
            rj = divoom_watchface_local_http_error_json("Device/CreateLocalClock",
                "missing file part, invalid image/bundle, filename in multipart, size mismatch, or empty ItemList stage");
        }
        else
        {
            rj = divoom_watchface_local_http_error_json("Device/CreateLocalClock", "missing JSON part");
        }

        if(pup_file_handle->json_str)
        {
            divoom_system_free(pup_file_handle->json_str);
            pup_file_handle->json_str = NULL;
        }
        pup_file_handle = divoom_queue_pop_cmp(pdivoom_http_server_handle->up_file_queue, divoom_hs_conn_handle(conn), divoom_http_server_upload_handler_compare_handle);
        if(pup_file_handle->json_str)
        {
            divoom_system_free(pup_file_handle->json_str);
        }
        if(pup_file_handle->file_handle)
        {
            divoom_http_server_upload_close_file(pup_file_handle, 1);
        }

        if(rj != NULL)
        {
            divoom_hs_conn_chunk_printf(conn, "%s", rj);
            divoom_system_free(rj);
        }
        else
        {
            divoom_hs_conn_chunk_printf(conn, "{\"Command\":\"Device/CreateLocalClock\",\"ReturnCode\":1,\"ReturnMessage\":\"oom\"}");
        }

        divoom_http_server_upload_free_file_buffer(pup_file_handle);
        divoom_system_free(pup_file_handle);

        divoom_hs_conn_chunk_end(conn);
        divoom_hs_conn_done(conn);
    }
}

/*
 * POST /patch_local_clock — multipart/form-data：第一段为 Device/PatchLocalClockInfo Patch JSON，
 * 可选第二段为底图单文件或 tar.gz（DialAssets、ItemPatchList[].patch.bundle_image 规则同 POST /create_local_clock）。
 */
void divoom_http_server_patch_local_clock_handler_hs(divoom_hs_conn_t *conn, int event)
{
    if(divoom_http_server_low_mem_busy(conn, event)) {
        divoom_lan_upload_abort(divoom_hs_conn_handle(conn));
        return;
    }
    if(event == DIVOOM_HS_EV_HEAD_COMPLETE || event == DIVOOM_HS_EV_COMPLETE)
    {
        divoom_print("[storage_io] lan_upload_event via=%s event=%d free=%uKB\n",
            __func__, event, (uint32)divoom_port_fs_free_space(DIVOOM_USER_ROOT));
    }
    divoom_lan_multipart(conn, event, 2);
    return;
    Pdivoom_http_server_con_item pup_file_handle;

    if(divoom_http_server_low_mem_busy(conn, event))
    {
        return;
    }

    pup_file_handle = divoom_queue_get_cmp(pdivoom_http_server_handle->up_file_queue, divoom_hs_conn_handle(conn), divoom_http_server_upload_handler_compare_handle);
    if(pup_file_handle == NULL)
    {
        pup_file_handle  = divoom_system_malloc(sizeof(divoom_http_server_con_item));
        if(pup_file_handle == NULL)
        {
            return;
        }
        memset(pup_file_handle, 0, sizeof(divoom_http_server_con_item));
        pup_file_handle->dial_bg_replace_clock_id = -1;
        pup_file_handle->conn_handle = divoom_hs_conn_handle(conn);
        divoom_queue_push(pdivoom_http_server_handle->up_file_queue, pup_file_handle);
    }
    if(event == DIVOOM_HS_EV_HEAD_COMPLETE)
    {
        divoom_hs_str_t str = divoom_hs_conn_get_header(conn, "Content-Length");
        int content_length = 0;
        char buf[48];

        buf[0] = 0;
        if(str.p != NULL && str.len > 0 && str.len < (int)sizeof(buf))
        {
            memcpy(buf, str.p, (size_t)str.len);
            buf[str.len] = 0;
            content_length = atoi(buf);
        }

        pup_file_handle->accept_len = 0;
        pup_file_handle->file_len = 0;
        pup_file_handle->file_write_buffer_len = 0;
        pup_file_handle->total_len = content_length;
        pup_file_handle->file_info[0] = 0;

        if(pup_file_handle->file_handle)
        {
            divoom_http_server_upload_close_file(pup_file_handle, 0);
        }

        if(content_length > 100 * 1024 * 1024)
        {
            divoom_hs_conn_error(conn, DIVOOM_HTTP_STATUS_INTERNAL_SERVER_ERROR, "Too big");
            return;
        }
    }
    else if(event == DIVOOM_HS_EV_BODY)
    {
        divoom_hs_str_t body = divoom_hs_conn_extract_body(conn);

        if(pup_file_handle->accept_len  == 0)
        {
            pup_file_handle->json_str = divoom_http_server_upload_do_first_data(pup_file_handle, (char *)body.p, body.len);
            divoom_download_aliyun_file_set_download_flag(1);
        }
        else
        {
            divoom_http_server_upload_do_other_data(pup_file_handle, (char *)body.p, body.len);
        }
        pup_file_handle->accept_len += body.len;
    }
    else if(event == DIVOOM_HS_EV_COMPLETE)
    {
        char *rj = NULL;

        divoom_hs_conn_send_head(conn, DIVOOM_HTTP_STATUS_OK, -1, "Content-Type: application/json; charset=UTF-8\r\n");

        divoom_download_aliyun_file_set_download_flag(0);

        if(pup_file_handle->file_handle)
        {
            divoom_http_server_upload_close_file(pup_file_handle, 1);
        }

        if(pup_file_handle->json_str == NULL)
        {
            rj = divoom_watchface_local_http_error_json("Device/PatchLocalClockInfo", "missing JSON part");
        }
        else if(pup_file_handle->resolved_upload_path[0] != 0
            && pup_file_handle->file_len > 0 && pup_file_handle->file_len == pup_file_handle->cur_file_size)
        {
            rj = divoom_watchface_local_http_patch_local_clock_with_upload(
                pup_file_handle->json_str,
                pup_file_handle->resolved_upload_path,
                pup_file_handle->file_len,
                NULL, 0);
        }
        else if(pup_file_handle->resolved_upload_path[0] != 0
            || pup_file_handle->file_len > 0
            || pup_file_handle->cur_file_size > 0)
        {
            rj = divoom_watchface_local_http_error_json("Device/PatchLocalClockInfo",
                "multipart file part incomplete (size/path mismatch)");
        }
        else
        {
            rj = divoom_watchface_local_http_patch_local_clock_with_upload(
                pup_file_handle->json_str,
                NULL,
                0,
                NULL, 0);
        }

        if(pup_file_handle->json_str)
        {
            divoom_system_free(pup_file_handle->json_str);
            pup_file_handle->json_str = NULL;
        }
        pup_file_handle = divoom_queue_pop_cmp(pdivoom_http_server_handle->up_file_queue, divoom_hs_conn_handle(conn), divoom_http_server_upload_handler_compare_handle);
        if(pup_file_handle->json_str)
        {
            divoom_system_free(pup_file_handle->json_str);
        }
        if(pup_file_handle->file_handle)
        {
            divoom_http_server_upload_close_file(pup_file_handle, 1);
        }

        if(rj != NULL)
        {
            divoom_hs_conn_chunk_printf(conn, "%s", rj);
            divoom_system_free(rj);
        }
        else
        {
            divoom_hs_conn_chunk_printf(conn,
                "{\"Command\":\"Device/PatchLocalClockInfo\",\"ReturnCode\":1,\"ReturnMessage\":\"oom\"}");
        }

        divoom_http_server_upload_free_file_buffer(pup_file_handle);
        divoom_system_free(pup_file_handle);

        divoom_hs_conn_chunk_end(conn);
        divoom_hs_conn_done(conn);
    }
}


void divoom_http_server_replace_clock_dial_bg_handler_hs(divoom_hs_conn_t *conn, int event)
{
    if(divoom_http_server_low_mem_busy(conn, event)) {
        divoom_lan_upload_abort(divoom_hs_conn_handle(conn));
        return;
    }
    if(event == DIVOOM_HS_EV_HEAD_COMPLETE || event == DIVOOM_HS_EV_COMPLETE)
    {
        divoom_print("[storage_io] lan_upload_event via=%s event=%d free=%uKB\n",
            __func__, event, (uint32)divoom_port_fs_free_space(DIVOOM_USER_ROOT));
    }
    divoom_lan_multipart(conn, event, 3);
    return;
    Pdivoom_http_server_con_item pup_file_handle;

    if(divoom_http_server_low_mem_busy(conn, event))
    {
        return;
    }

    pup_file_handle = divoom_queue_get_cmp(pdivoom_http_server_handle->up_file_queue, divoom_hs_conn_handle(conn), divoom_http_server_upload_handler_compare_handle);
    if(pup_file_handle == NULL)
    {
        pup_file_handle  = divoom_system_malloc(sizeof(divoom_http_server_con_item));
        if(pup_file_handle == NULL)
        {
            return;
        }
        memset(pup_file_handle, 0, sizeof(divoom_http_server_con_item));
        pup_file_handle->dial_bg_replace_clock_id = -1;
        pup_file_handle->conn_handle = divoom_hs_conn_handle(conn);
        divoom_queue_push(pdivoom_http_server_handle->up_file_queue, pup_file_handle);
    }

    if(event == DIVOOM_HS_EV_HEAD_COMPLETE)
    {
        divoom_hs_str_t str = divoom_hs_conn_get_header(conn, "Content-Length");
        int content_length = 0;
        char buf[48];

        buf[0] = 0;
        if(str.p != NULL && str.len > 0 && str.len < (int)sizeof(buf))
        {
            memcpy(buf, str.p, (size_t)str.len);
            buf[str.len] = 0;
            content_length = atoi(buf);
        }

        pup_file_handle->accept_len = 0;
        pup_file_handle->file_len = 0;
        pup_file_handle->file_write_buffer_len = 0;
        pup_file_handle->total_len = content_length;
        pup_file_handle->file_info[0] = 0;

        if(pup_file_handle->file_handle)
        {
            divoom_http_server_upload_close_file(pup_file_handle, 0);
        }

        if(content_length > DIVOOM_REPLACE_DIAL_BG_MAX_HTTP_BODY)
        {
            divoom_hs_conn_error(conn, DIVOOM_HTTP_STATUS_INTERNAL_SERVER_ERROR, "Too big");
            return;
        }
    }
    else if(event == DIVOOM_HS_EV_BODY)
    {
        divoom_hs_str_t body = divoom_hs_conn_extract_body(conn);

        #if 1
        if(pup_file_handle->accept_len  == 0)
        {
            divoom_http_server_replace_dial_bg_do_first_data(pup_file_handle, (char *)body.p, body.len);
            divoom_download_aliyun_file_set_download_flag(1);
        }
        else
        {
            divoom_http_server_upload_do_other_data(pup_file_handle, (char *)body.p, body.len);
        }
        #endif
        pup_file_handle->accept_len += body.len;
    }
    else if(event == DIVOOM_HS_EV_COMPLETE)
    {
        int ok_cid = pup_file_handle->dial_bg_replace_clock_id;
        unsigned char ok_flg = pup_file_handle->dial_bg_replace_upload;
        int ok_csz = pup_file_handle->cur_file_size;
        int ok_flen = pup_file_handle->file_len;
        char *rj = NULL;

        divoom_hs_conn_send_head(conn, DIVOOM_HTTP_STATUS_OK, -1, "Content-Type: application/json; charset=UTF-8\r\n");

        divoom_download_aliyun_file_set_download_flag(0);

        if(pup_file_handle->file_handle)
        {
            divoom_http_server_upload_close_file(pup_file_handle, 1);
        }

        pup_file_handle = divoom_queue_pop_cmp(pdivoom_http_server_handle->up_file_queue, divoom_hs_conn_handle(conn), divoom_http_server_upload_handler_compare_handle);
        if(pup_file_handle->json_str)
        {
            divoom_system_free(pup_file_handle->json_str);
        }
        if(pup_file_handle->file_handle)
        {
            divoom_http_server_upload_close_file(pup_file_handle, 1);
        }

        if(ok_flg != 0 && ok_cid > 0 && ok_csz > 0 && ok_flen == ok_csz)
        {
            char bgpath[512];
            char valerr[224];

            bgpath[0] = 0;
            divoom_watchface_replace_clock_dial_bg_cache_path(ok_cid, bgpath, (int)sizeof(bgpath));
            if(divoom_watchface_replace_clock_dial_bg_validate_saved_file(bgpath, ok_flen, valerr, (int)sizeof(valerr)) != 0)
            {
                if(bgpath[0] != 0)
                    unlink(bgpath);
                rj = divoom_watchface_replace_clock_dial_bg_result_json(ok_cid, 1, valerr);
            }
            else
            {
                divoom_watchface_replace_clock_dial_bg_apply_saved_file(ok_cid);
                rj = divoom_watchface_replace_clock_dial_bg_result_json(ok_cid, 0, NULL);
            }
        }
        else
        {
            int report_cid = ok_cid > 0 ? ok_cid : -1;
            rj = divoom_watchface_replace_clock_dial_bg_result_json(report_cid, 1,
                "invalid multipart, clock id, size mismatch, or cannot write dial bg cache path");
        }

        if(rj != NULL)
        {
            divoom_hs_conn_chunk_printf(conn, "%s", rj);
            divoom_system_free(rj);
        }
        else
        {
            divoom_hs_conn_chunk_printf(conn, "{\"ReturnCode\":1,\"ReturnMessage\":\"oom\"}");
        }

        divoom_http_server_upload_free_file_buffer(pup_file_handle);
        divoom_system_free(pup_file_handle);

        divoom_hs_conn_chunk_end(conn);
        divoom_hs_conn_done(conn);
    }
}



#ifdef DIVOOM_SIMULATOR
void divoom_http_server_default_handler(struct uh_connection *conn, int event)
{
    divoom_http_server_default_handler_hs((divoom_hs_conn_t *)conn, event);
}

void divoom_http_server_divoom_api_handler(struct uh_connection *conn, int event)
{
    divoom_http_server_divoom_api_handler_hs((divoom_hs_conn_t *)conn, event);
}

void divoom_http_server_upload_handler(struct uh_connection *conn, int event)
{
    divoom_http_server_upload_handler_hs((divoom_hs_conn_t *)conn, event);
}

void divoom_http_server_create_local_clock_handler(struct uh_connection *conn, int event)
{
    divoom_http_server_create_local_clock_handler_hs((divoom_hs_conn_t *)conn, event);
}

void divoom_http_server_patch_local_clock_handler(struct uh_connection *conn, int event)
{
    divoom_http_server_patch_local_clock_handler_hs((divoom_hs_conn_t *)conn, event);
}

void divoom_http_server_replace_clock_dial_bg_handler(struct uh_connection *conn, int event)
{
    divoom_http_server_replace_clock_dial_bg_handler_hs((divoom_hs_conn_t *)conn, event);
}

#endif /* DIVOOM_SIMULATOR */
void* divoom_http_server_task_main(void *param)
{
#ifdef DIVOOM_SIMULATOR
    (void)param;
    divoom_print("HTTP server (sim) starting on port %d\n", DVIOOM_HTTP_SERVER_PORT);
    sim_uhttpd_run(DVIOOM_HTTP_SERVER_PORT);
    return NULL;
#else
    divoom_http_server_port_run_shttpd(param);
    return NULL;
#endif /* DIVOOM_SIMULATOR */
}



/*
  Function:       divoom_http_server_init
  Description:    http server 线程处理函数，与civetview通讯，这里只做数据处理，并不是网络通讯
  Input:          void
  Return:         NONE
*/
void divoom_http_server_init(void)
{
    Pdivoom_http_server_ctrl handle;

    if(pdivoom_http_server_handle)
    {
        return;
    }
    divoom_lan_storage_init();
    handle = divoom_system_malloc(sizeof(*handle));
    if(handle == NULL)
    {
        return;
    }
    memset(handle, 0, sizeof(*handle));
    handle->cmd_queue = divoom_queue_init(1000, __func__, __LINE__);
    handle->up_file_queue = divoom_queue_init(1000, __func__, __LINE__);
    if(handle->cmd_queue == NULL || handle->up_file_queue == NULL)
    {
        divoom_queue_destroy(handle->up_file_queue, NULL);
        divoom_queue_destroy(handle->cmd_queue, NULL);
        divoom_system_free(handle);
        return;
    }
    __atomic_store_n(&pdivoom_http_server_handle, handle, __ATOMIC_RELEASE);

#ifdef DIVOOM_HTTPS_ENABLE
    // 初始化SSL配置
    pdivoom_http_server_handle->ssl_enabled = false;
    pdivoom_http_server_handle->ssl_cert_file = NULL;
    pdivoom_http_server_handle->ssl_key_file = NULL;
    pdivoom_http_server_handle->ssl_ctx = NULL;
    
    // 尝试初始化SSL - 如果失败，只运行HTTP服务器
    if (divoom_http_server_ssl_init(DVIOOM_SSL_CERT_FILE, DVIOOM_SSL_KEY_FILE) == 0) {
        divoom_print("HTTPS enabled with SSL certificates\n");
    } else {
        divoom_print("HTTPS disabled - SSL certificates not found or initialization failed\n");
        divoom_print("HTTP server will run without HTTPS support\n");
    }
#endif

#ifdef DIVOOM_SIMULATOR
    sim_uhttpd_set_low_mem_guard_checker(divoom_game_runtime_low_mem_guard_is_active);
#endif

    #ifndef DIVOOM_TEST_TUTV_FLAG
    divoom_system_create_task(divoom_http_server_task_main, "http_server", handle, DIVOOM_TASK_STACK_HTTP_SERVER);
    #endif
}

/*
  Function:       divoom_http_server_cleanup
  Description:    清理HTTP服务器资源
  Input:          void
  Return:         void
*/
void divoom_http_server_cleanup(void)
{
    if (pdivoom_http_server_handle) {
        if (pdivoom_http_server_handle->cmd_queue) {
            divoom_queue_destroy(pdivoom_http_server_handle->cmd_queue, NULL);
        }
        if (pdivoom_http_server_handle->up_file_queue) {
            divoom_queue_destroy(pdivoom_http_server_handle->up_file_queue, NULL);
        }
#ifdef DIVOOM_HTTPS_ENABLE
        divoom_http_server_ssl_cleanup();
#endif
        divoom_system_free(pdivoom_http_server_handle);
        __atomic_store_n(&pdivoom_http_server_handle, NULL, __ATOMIC_RELEASE);
    }

#ifdef DIVOOM_SIMULATOR
    sim_uhttpd_set_low_mem_guard_checker(NULL);
#endif
}

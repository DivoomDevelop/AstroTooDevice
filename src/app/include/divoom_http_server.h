#ifndef _divoom_http_server_
#define _divoom_http_server_

#include "divoom_type.h"

/* <100 KiB 隐藏；100~199 KiB 内嵌显示；>=200 KiB 使用独立页面。 */
#define DIVOOM_HTTP_TRANSFER_PROGRESS_MIN_FILE_SIZE       (99U * 1024U)
#define DIVOOM_HTTP_TRANSFER_PROGRESS_FULL_PAGE_FILE_SIZE (100U * 1024U)

/*
  Function:       divoom_http_server_send_command
  Description:    向HTTP请求发送数据回civetweb
  Input:          cmd
                  cmd_string
  Return:         1：已经处理；0：未处理
*/

int divoom_http_server_send_command(int cmd,char *cmd_string);

/* 返回本地像素文件 HTTP 传输进度；1 表示进度条应显示，0 表示隐藏。 */
int divoom_http_server_get_pixel_upload_progress(int *percent);

/* 返回应由 APP_ADD_PHOTO 等待页显示的上传字节进度。 */
int divoom_http_server_get_upload_wait_progress(int *percent);

/*
 * 复用 DIVOOM_CLOCK_DISP_SUPPORT_HTTP_UPLOAD_PROGRESS 的单一显示槽。
 * 后开始的传输接管 owner，旧传输的延迟回调会因 owner 不匹配而被忽略。
 */
void divoom_http_server_transfer_progress_begin(void *owner);
void divoom_http_server_transfer_progress_update(void *owner, uint32 current,
                                                 uint32 total);
void divoom_http_server_transfer_progress_finish(void *owner);

/*
  Function:       divoom_http_server_init
  Description:    http server 线程处理函数，与civetview通讯，这里只做数据处理，并不是网络通讯
  Input:          void
  Return:         NONE
*/
void divoom_http_server_init(void);

#endif

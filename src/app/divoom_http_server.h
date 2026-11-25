#ifndef DIVOOM_HTTP_SERVER_H
#define DIVOOM_HTTP_SERVER_H

#include "divoom.h"

#ifdef __cplusplus
extern "C" {
#endif

// HTTP服务器初始化
void divoom_http_server_init(void);

// HTTP服务器清理
void divoom_http_server_cleanup(void);

// 发送命令到HTTP服务器
int divoom_http_server_send_command(int cmd, char *cmd_string);

#ifdef DIVOOM_HTTPS_ENABLE
// SSL相关函数
int divoom_http_server_ssl_init(const char *cert_file, const char *key_file);
void divoom_http_server_ssl_cleanup(void);
#endif

#ifdef __cplusplus
}
#endif

#endif // DIVOOM_HTTP_SERVER_H 
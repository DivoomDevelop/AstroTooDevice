/* 表盘附件上传在真机由 RTOS HTTP 客户端实现；仿真使用 libcurl。 */
#include "divoom.h"

#ifdef DIVOOM_SIMULATOR
#include <curl/curl.h>
#include <stdio.h>
#include <string.h>

typedef struct divoom_http_client_watchface_response {
    char data[4096];
    size_t length;
} divoom_http_client_watchface_response;



int divoom_http_client_upload_watchface_file(const char *file_addr, char *up_file_addr)
{
    (void)file_addr;
    if (up_file_addr != NULL) up_file_addr[0] = '\0';
    return 0;
}
#endif

/* HA-only bounded HTTP client for OAuth refresh; does not touch shared HTTP state. */
#include "divoom.h"
#include "divoom_ha_http.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#pragma push_macro("errno")
#undef errno
#include <sys/errno.h>
#pragma pop_macro("errno")
#include <unistd.h>
#include <sys/ioctl.h>
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#define DIVOOM_HA_HTTP_HEADER_MAX 4096
#define DIVOOM_HA_HTTP_RAW_MAX (DIVOOM_HA_HTTP_RESPONSE_MAX + DIVOOM_HA_HTTP_HEADER_MAX)

static int divoom_ha_http_wait_socket(int fd, int writing, unsigned timeout_ms)
{
    fd_set fds;
    struct timeval tv;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    return select(fd + 1, writing ? NULL : &fds, writing ? &fds : NULL, NULL, &tv);
}

static int divoom_ha_http_send(int fd, const char *data, size_t size)
{
    size_t offset = 0;
    unsigned start = divoom_port_tick_ms();
    while(offset < size)
    {
        int count;
        if(divoom_port_tick_ms() - start > 5000)
            return -1;
        {
            int ready = divoom_ha_http_wait_socket(fd, 1, 1000);
            if(ready < 0)
                return -1;
            if(!ready)
                continue;
        }
        count = send(fd, data + offset, size - offset, 0);
        if(count > 0)
            offset += (size_t)count;
        else if(count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
            continue;
        else
            return -1;
    }
    return 0;
}

static int divoom_ha_http_header_value(const char *headers, const char *name, char *value, size_t size)
{
    const char *line = strstr(headers, "\r\n");
    size_t name_len = strlen(name);
    while(line && line[2] && line[2] != '\r')
    {
        const char *start = line + 2;
        const char *end = strstr(start, "\r\n");
        const char *colon = end ? memchr(start, ':', (size_t)(end - start)) : NULL;
        if(colon && (size_t)(colon - start) == name_len && !strncasecmp(start, name, name_len))
        {
            const char *v = colon + 1;
            size_t n;
            while(*v == ' ' || *v == '\t')
                v++;
            n = (size_t)(end - v);
            if(n >= size)
                return -1;
            memcpy(value, v, n);
            value[n] = 0;
            return 1;
        }
        line = end;
    }
    return 0;
}

/*********************************************************************
  Function:        divoom_ha_http_post_form
  Description:     用独立 TCP 连接发送 HA OAuth 表单并读取有界响应。
  Input:           url：HTTP 完整 URL；form：已编码表单；http_status：HTTP 状态输出。
  Return:          成功返回由 divoom_system_free 释放的正文；失败返回 NULL。
  调用约束:         仅 HA 后台任务同步调用；5 秒超时；不支持 HTTPS 或重定向。
**********************************************************************/
char *divoom_ha_http_post_form(const char *url, const char *form, int *http_status)
{
    struct addrinfo hints, *addresses = NULL, *address;
    char host[256], authority[264], service[8], path[512], length_text[32];
    char *request = NULL, *raw = NULL, *response = NULL;
    size_t request_size = 0, request_len, used = 0, header_len = 0, body_len = 0, expected_len = 0;
    int has_length = 0, fd = -1, status, one = 1, result = -1;
    unsigned start;

    if(http_status)
        *http_status = 0;
    if(!url || !form || strncmp(url, "http://", 7))
        return NULL;
    url += 7;
    {
        const char *slash = strchr(url, '/');
        const char *colon = strchr(url, ':');
        size_t authority_len = slash ? (size_t)(slash - url) : strlen(url);
        size_t host_len = colon && (size_t)(colon - url) < authority_len ? (size_t)(colon - url) : authority_len;
        unsigned port = 80;
        if(!authority_len || authority_len >= sizeof(authority) || !host_len || host_len >= sizeof(host))
            goto DONE;
        memcpy(authority, url, authority_len);
        authority[authority_len] = 0;
        memcpy(host, url, host_len);
        host[host_len] = 0;
        if(colon && (size_t)(colon - url) < authority_len)
        {
            char *end;
            unsigned long number = strtoul(colon + 1, &end, 10);
            if(end == colon + 1 || end != url + authority_len || number == 0 || number > 65535)
                goto DONE;
            port = (unsigned)number;
        }
        snprintf(service, sizeof(service), "%u", port);
        snprintf(path, sizeof(path), "%s", slash ? slash : "/");
    }
    request_size = strlen(path) + strlen(authority) + strlen(form) + 160;
    request = divoom_system_malloc(request_size);
    raw = divoom_system_malloc(DIVOOM_HA_HTTP_RAW_MAX + 1);
    if(!request || !raw)
        goto DONE;
    request_len = (size_t)snprintf(request, request_size,
        "POST %s HTTP/1.1\r\nHost: %s\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %lu\r\nConnection: close\r\n\r\n%s",
        path, authority, (unsigned long)strlen(form), form);
    if(request_len >= request_size)
        goto DONE;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if(getaddrinfo(host, service, &hints, &addresses))
        goto DONE;
    for(address = addresses; address; address = address->ai_next)
    {
        int error = 0;
        socklen_t error_len = sizeof(error);
        fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if(fd < 0)
            continue;
        if(ioctl(fd, FIONBIO, &one) == 0 && (connect(fd, address->ai_addr, (socklen_t)address->ai_addrlen) == 0 || (errno == EINPROGRESS && divoom_ha_http_wait_socket(fd, 1, 5000) > 0 && getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *)&error, &error_len) == 0 && error == 0)))
            break;
        close(fd);
        fd = -1;
    }
    freeaddrinfo(addresses);
    addresses = NULL;
    if(fd < 0 || divoom_ha_http_send(fd, request, request_len))
        goto DONE;

    start = divoom_port_tick_ms();
    while(used < DIVOOM_HA_HTTP_RAW_MAX)
    {
        int count;
        if(divoom_port_tick_ms() - start > 5000)
            goto DONE;
        {
            int ready = divoom_ha_http_wait_socket(fd, 0, 1000);
            if(ready < 0)
                goto DONE;
            if(!ready)
                continue;
        }
        count = recv(fd, raw + used, DIVOOM_HA_HTTP_RAW_MAX - used, 0);
        if(count == 0)
            break;
        if(count < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
                continue;
            goto DONE;
        }
        used += (size_t)count;
        raw[used] = 0;
        if(!header_len)
        {
            char *separator = strstr(raw, "\r\n\r\n");
            char *status_start, *status_end;
            if(!separator)
            {
                if(used >= DIVOOM_HA_HTTP_HEADER_MAX)
                    goto DONE;
                continue;
            }
            header_len = (size_t)(separator + 4 - raw);
            if(header_len > DIVOOM_HA_HTTP_HEADER_MAX || strncmp(raw, "HTTP/", 5))
                goto DONE;
            status_start = strchr(raw, ' ');
            if(!status_start)
                goto DONE;
            status = (int)strtol(status_start + 1, &status_end, 10);
            if(status < 100 || status > 599 || status_end == status_start + 1)
                goto DONE;
            if(http_status)
                *http_status = status;
            if(divoom_ha_http_header_value(raw, "Content-Length", length_text, sizeof(length_text)) > 0)
            {
                char *end;
                unsigned long length = strtoul(length_text, &end, 10);
                if(*end || length > DIVOOM_HA_HTTP_RESPONSE_MAX)
                    goto DONE;
                has_length = 1;
                expected_len = (size_t)length;
            }
            if(has_length && used - header_len >= expected_len)
                break;
        }
        else if(has_length && used - header_len >= expected_len)
            break;
    }
    if(!header_len)
        goto DONE;
    body_len = used - header_len;
    if(has_length)
    {
        if(body_len < expected_len)
            goto DONE;
        body_len = expected_len;
    }
    if(body_len > DIVOOM_HA_HTTP_RESPONSE_MAX)
        goto DONE;
    response = divoom_system_malloc(body_len + 1);
    if(!response)
        goto DONE;
    memcpy(response, raw + header_len, body_len);
    response[body_len] = 0;
    result = 0;
DONE:
    if(addresses)
        freeaddrinfo(addresses);
    if(fd >= 0)
        close(fd);
    if(request)
    {
        memset(request, 0, request_size);
        divoom_system_free(request);
    }
    if(raw)
    {
        memset(raw, 0, DIVOOM_HA_HTTP_RAW_MAX + 1);
        divoom_system_free(raw);
    }
    if(result)
    {
        if(response)
            divoom_system_free(response);
        return NULL;
    }
    return response;
}
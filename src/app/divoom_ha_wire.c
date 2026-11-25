/* Bounded RFC6455 LAN client, independent of the cloud MQTT/HTTP locks. */
#include "divoom.h"
#include "divoom_ha_wire.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
/* App 的 include/lwip 路径会让 FreeRTOS errno.h 的 include_next 命中
 * lwip/errno.h，循环包含后缺少错误码。直接补入 Newlib 定义，保留
 * FreeRTOS 原有的 errno 绑定，避免改成另一份错误变量。 */
#pragma push_macro("errno")
#undef errno
#include <sys/errno.h>
#pragma pop_macro("errno")
#include <unistd.h>
#include <mbedtls/base64.h>
#include <mbedtls/sha1.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#if defined(CONFIG_DRIVERS_CE_0_6)
#include <hal/sunxi_hal_ce.h>
#endif
struct divoom_ha_wire
{
    int fd, fragmented;
    unsigned deadline;
    size_t used;
    unsigned char *message;
    mbedtls_ctr_drbg_context rng;
    mbedtls_entropy_context entropy;
};
/*********************************************************************
  Function:        divoom_ha_wire_get_entropy
  Description:     提供握手及客户端掩码所需随机熵，优先用 CE 硬件，否则用 mbedTLS 熵源。
  Input:           ctx：熵上下文；out：输出；size：随机字节数。
  Return:          0：成功；非 0：底层硬件或熵源失败。
  调用约束:         由传输随机数上下文调用。
**********************************************************************/
static int divoom_ha_wire_get_entropy(void *ctx, unsigned char *out, size_t size)
{
#if defined(CONFIG_DRIVERS_CE_0_6)
    crypto_rng_req_ctx_t req;
    int rc;
    (void)ctx;
    memset(&req, 0, sizeof(req));
    req.rng_buf = out;
    req.rng_len = size;
    if(sunxi_ce_init())
        return -1;
    rc = do_rng_gen(&req);
    sunxi_ce_uninit();
    return rc;
#else
    return mbedtls_entropy_func(ctx, out, size);
#endif
}
/*********************************************************************
  Function:        divoom_ha_wire_wait_socket
  Description:     用 select 等待 socket 可读或可写，避免非阻塞传输忙等。
  Input:           fd：socket；writing：非 0 等可写，否则可读；timeout_ms：等待时限。
  Return:          大于 0：就绪；0：等待超时；小于 0：select 错误。
  调用约束:         仅持有该连接的后台任务调用。
**********************************************************************/
static int divoom_ha_wire_wait_socket(int fd, int writing, unsigned timeout_ms)
{
    fd_set fds;
    struct timeval tv;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    return select(fd + 1, writing ? NULL : &fds, writing ? &fds : NULL, NULL, &tv);
}
/*********************************************************************
  Function:        divoom_ha_wire_transfer_data
  Description:     在有界超时内完整收发字节，处理部分传输、EAGAIN 和信号中断。
  Input:           w：连接；buf：缓冲区；n：长度；writing：非 0 发送，0 接收。
  Return:          0：完整传输成功；-1：关闭、错误或超时。
  调用约束:         仅后台任务调用，同时受单次时限和 w->deadline 约束。
**********************************************************************/
static int divoom_ha_wire_transfer_data(struct divoom_ha_wire *w, void *buf, size_t n, int writing)
{
    size_t off = 0;
    unsigned start = divoom_port_tick_ms();
    while(off < n)
    {
        int r;
        if(divoom_port_tick_ms() - start > 5000 || (int)(divoom_port_tick_ms() - w->deadline) >= 0 || divoom_ha_wire_wait_socket(w->fd, writing, 1000) < 0)
            return -1;
        r = writing ? send(w->fd, (char *)buf + off, n - off, 0) : recv(w->fd, (char *)buf + off, n - off, 0);
        if(r > 0)
            off += (size_t)r;
        else if(!r)
            return -1;
        else if(errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
            return -1;
    }
    return 0;
}
/*********************************************************************
  Function:        divoom_ha_wire_send_frame
  Description:     构造 FIN 帧，生成客户端掩码并分块发送负载。
  Input:           w：连接；op：帧类型；data：负载；n：长度，使用短帧或 16 位长度帧。
  Return:          0：发送完成；-1：长度、随机数、超时或 socket 失败。
  调用约束:         后台任务调用；当前业务控制消息应小于 65536 字节。
**********************************************************************/
static int divoom_ha_wire_send_frame(struct divoom_ha_wire *w, int op, const void *data, size_t n)
{
    unsigned char h[8], mask[4], chunk[512];
    size_t hn = 2, off, i;
    if(n > DIVOOM_HA_WIRE_MESSAGE_MAX || mbedtls_ctr_drbg_random(&w->rng, mask, 4))
        return -1;
    w->deadline = divoom_port_tick_ms() + 5000;
    h[0] = (unsigned char)(0x80 | op);
    if(n < 126)
        h[1] = (unsigned char)(0x80 | n);
    else
    {
        h[1] = 0x80 | 126;
        h[2] = (unsigned char)(n >> 8);
        h[3] = (unsigned char)n;
        hn = 4;
    }
    memcpy(h + hn, mask, 4);
    hn += 4;
    if(divoom_ha_wire_transfer_data(w, h, hn, 1))
        return -1;
    for(off = 0; off < n; off += i)
    {
        size_t count = n - off;
        if(count > sizeof(chunk))
            count = sizeof(chunk);
        for(i = 0; i < count; i++)
            chunk[i] = ((const unsigned char *)data)[off + i] ^ mask[(off + i) % 4];
        if(divoom_ha_wire_transfer_data(w, chunk, count, 1))
            return -1;
    }
    return 0;
}
/*********************************************************************
  Function:        divoom_ha_wire_read_http_line
  Description:     读取握手响应的一行，移除 CR/LF 并添加结束符。
  Input:           w：连接；buf：输出；cap：含结束符容量。
  Return:          0：完整一行；-1：传输失败或行超过容量。
  调用约束:         仅在 WebSocket 升级握手时使用。
**********************************************************************/
static int divoom_ha_wire_read_http_line(struct divoom_ha_wire *w, char *buf, size_t cap)
{
    size_t n = 0;
    char c;
    while(n + 1 < cap)
    {
        if(divoom_ha_wire_transfer_data(w, &c, 1, 0))
            return -1;
        if(c == '\n')
        {
            buf[n] = 0;
            return 0;
        }
        if(c != '\r')
            buf[n++] = c;
    }
    return -1;
}
/*********************************************************************
  Function:        divoom_ha_wire_header_starts_with
  Description:     不区分大小写地检查 HTTP 头字段前缀。
  Input:           s：头部文本；p：预期前缀。
  Return:          1：匹配；0：不匹配。
  调用约束:         不修改输入、不持有缓冲区。
**********************************************************************/
static int divoom_ha_wire_header_starts_with(const char *s, const char *p)
{
    while(*p)
        if(tolower((unsigned char)*s++) != tolower((unsigned char)*p++))
            return 0;
    return 1;
}
/*********************************************************************
  Function:        divoom_ha_wire_close
  Description:     关闭 socket，释放收包缓冲、随机数上下文和连接结构。
  Input:           w：连接，可为 NULL。
  Return:          无。
  调用约束:         仅连接所有者调用；返回后 w 及此前 text 指针均不能使用。
**********************************************************************/
void divoom_ha_wire_close(struct divoom_ha_wire *w)
{
    if(!w)
        return;
    if(w->fd >= 0)
        close(w->fd);
    mbedtls_ctr_drbg_free(&w->rng);
    mbedtls_entropy_free(&w->entropy);
    if(w->message)
        divoom_system_free(w->message);
    divoom_system_free(w);
}
/*********************************************************************
  Function:        divoom_ha_wire_open
  Description:     分配资源，建立局域网 TCP 连接并校验 WebSocket 升级握手。
  Input:           out：返回连接；host：主机；port：端口。
  Return:          0：握手成功；-1：分配、解析、连接或握手失败，*out 为 NULL。
  调用约束:         后台任务调用；仅 ws，不支持 WSS，不复用云 MQTT 连接。
**********************************************************************/
int divoom_ha_wire_open(struct divoom_ha_wire **out, const char *host, unsigned port)
{
    struct divoom_ha_wire *w;
    struct addrinfo hints, *list = NULL, *a;
    unsigned char nonce[16], sha[20];
    char service[8], key[32], expected[40], joined[96], line[512];
    size_t olen;
    int valid_accept = 0, valid_upgrade = 0, valid_connection = 0, total = 0;
    *out = NULL;
    w = divoom_system_malloc(sizeof(*w));
    if(!w)
        return -1;
    memset(w, 0, sizeof(*w));
    w->fd = -1;
    mbedtls_entropy_init(&w->entropy);
    mbedtls_ctr_drbg_init(&w->rng);
    if(mbedtls_ctr_drbg_seed(&w->rng, divoom_ha_wire_get_entropy, &w->entropy, (const unsigned char *)"astrotoo-ha", 11))
        goto fail;
    w->message = divoom_system_malloc(DIVOOM_HA_WIRE_MESSAGE_MAX + 1);
    if(!w->message)
        goto fail;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(service, sizeof(service), "%u", port);
    if(getaddrinfo(host, service, &hints, &list))
        goto fail;
    for(a = list; a; a = a->ai_next)
    {
        int one = 1, error = 0;
        socklen_t len = sizeof(error);
        w->fd = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
        if(w->fd < 0)
            continue;
        if(ioctl(w->fd, FIONBIO, &one) == 0 && (connect(w->fd, a->ai_addr, (socklen_t)a->ai_addrlen) == 0 || ((errno == EINPROGRESS || errno == EWOULDBLOCK) && divoom_ha_wire_wait_socket(w->fd, 1, 5000) > 0 && getsockopt(w->fd, SOL_SOCKET, SO_ERROR, (void *)&error, &len) == 0 && error == 0)))
            break;
        close(w->fd);
        w->fd = -1;
    }
    freeaddrinfo(list);
    list = NULL;
    if(w->fd < 0)
        goto fail;
    if(mbedtls_ctr_drbg_random(&w->rng, nonce, sizeof(nonce)) || mbedtls_base64_encode((unsigned char *)key, sizeof(key), &olen, nonce, sizeof(nonce)))
        goto fail;
    key[olen] = 0;
    snprintf(joined, sizeof(joined), "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", key);
    if(mbedtls_sha1_ret((unsigned char *)joined, strlen(joined), sha) || mbedtls_base64_encode((unsigned char *)expected, sizeof(expected), &olen, sha, sizeof(sha)))
        goto fail;
    expected[olen] = 0;
    w->deadline = divoom_port_tick_ms() + 5000;
    snprintf(line, sizeof(line), "GET /api/websocket HTTP/1.1\r\nHost: %s:%u\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: %s\r\n\r\n", host, port, key);
    if(divoom_ha_wire_transfer_data(w, line, strlen(line), 1) || divoom_ha_wire_read_http_line(w, line, sizeof(line)) || strncmp(line, "HTTP/1.1 101 ", 13))
        goto fail;
    while(1)
    {
        char *p;
        if(divoom_ha_wire_read_http_line(w, line, sizeof(line)))
            goto fail;
        total += (int)strlen(line) + 2;
        if(total > 4096)
            goto fail;
        if(!line[0])
            break;
        p = strchr(line, ':');
        if(!p)
            goto fail;
        p++;
        while(*p == ' ' || *p == '\t')
            p++;
        {
            char *end = p + strlen(p);
            while(end > p && isspace((unsigned char)end[-1]))
                *--end = 0;
        }
        if(divoom_ha_wire_header_starts_with(line, "Sec-WebSocket-Accept:"))
            valid_accept = !strcmp(p, expected);
        if(divoom_ha_wire_header_starts_with(line, "Upgrade:"))
            valid_upgrade = divoom_ha_wire_header_starts_with(p, "websocket") && strlen(p) == 9;
        if(divoom_ha_wire_header_starts_with(line, "Connection:"))
        {
            char *q = p;
            while(*q)
            {
                while(*q == ' ' || *q == ',' || *q == '\t')
                    q++;
                if(divoom_ha_wire_header_starts_with(q, "upgrade") && (!q[7] || q[7] == ',' || isspace((unsigned char)q[7])))
                    valid_connection = 1;
                while(*q && *q != ',')
                    q++;
            }
        }
    }
    if(!valid_accept || !valid_upgrade || !valid_connection)
        goto fail;
    *out = w;
    return 0;
fail:
    if(list)
        freeaddrinfo(list);
    divoom_ha_wire_close(w);
    return -1;
}
/*********************************************************************
  Function:        divoom_ha_wire_send_text
  Description:     将一条 JSON 文本作为 WebSocket 文本帧发送。
  Input:           w：连接；json：正确终止的 JSON 字符串。
  Return:          0：成功；-1：帧发送失败。
  调用约束:         后台任务调用；不接管 json，控制消息须小于 65536 字节。
**********************************************************************/
int divoom_ha_wire_send_text(struct divoom_ha_wire *w, const char *json)
{
    return divoom_ha_wire_send_frame(w, 1, json, strlen(json));
}
/*********************************************************************
  Function:        divoom_ha_wire_receive_text
  Description:     接收并拼接文本分片，处理 ping/pong，拒绝不支持或超限报文。
  Input:           w：连接；text：返回完整消息的内部缓冲指针。
  Return:          1：完整文本；0：无完整文本/已处理控制帧；-1：关闭、协议或超时错误；-2：消息超限。
  调用约束:         后台任务调用；text 不可释放，仅在下一次接收或关闭前有效。
**********************************************************************/
int divoom_ha_wire_receive_text(struct divoom_ha_wire *w, const char **text)
{
    unsigned char h[2], ext[8], control[125];
    size_t n;
    int op, fin, r, i;
    *text = NULL;
    r = divoom_ha_wire_wait_socket(w->fd, 0, 200);
    if(r <= 0)
        return r < 0 ? -1 : 0;
    w->deadline = divoom_port_tick_ms() + 5000;
    if(divoom_ha_wire_transfer_data(w, h, 2, 0))
        return -1;
    fin = h[0] & 0x80;
    op = h[0] & 15;
    n = h[1] & 127;
    if((h[0] & 0x70) || (h[1] & 0x80))
        return -1;
    if(n == 126)
    {
        if(divoom_ha_wire_transfer_data(w, ext, 2, 0))
            return -1;
        n = ((size_t)ext[0] << 8) | ext[1];
        if(n < 126)
            return -1;
    }
    else if(n == 127)
    {
        if(divoom_ha_wire_transfer_data(w, ext, 8, 0) || (ext[0] & 0x80))
            return -1;
        n = 0;
        for(i = 0; i < 8; i++)
        {
            if(n > DIVOOM_HA_WIRE_MESSAGE_MAX / 256)
                return -2;
            n = n * 256 + ext[i];
        }
        if(n < 65536)
            return -1;
    }
    if(op >= 8)
    {
        if(!fin || n > 125 || divoom_ha_wire_transfer_data(w, control, n, 0))
            return -1;
        if(op == 8)
            return -1;
        if(op == 9)
            return divoom_ha_wire_send_frame(w, 10, control, n) ? -1 : 0;
        return op == 10 ? 0 : -1;
    }
    if(op == 1)
    {
        if(w->fragmented)
            return -1;
        w->used = 0;
    }
    else if(op != 0 || !w->fragmented)
        return -1;
    if(n > DIVOOM_HA_WIRE_MESSAGE_MAX - w->used)
        return -2;
    if(divoom_ha_wire_transfer_data(w, w->message + w->used, n, 0))
        return -1;
    w->used += n;
    w->fragmented = !fin;
    if(!fin)
        return 0;
    if(memchr(w->message, 0, w->used))
        return -1;
    w->message[w->used] = 0;
    *text = (const char *)w->message;
    return 1;
}
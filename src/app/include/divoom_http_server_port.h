/**
 * @file divoom_http_server_port.h
 * HTTP server transport: libuhttpd (simulator) / SHTTPD (FreeRTOS).
 */
#ifndef DIVOOM_HTTP_SERVER_PORT_H
#define DIVOOM_HTTP_SERVER_PORT_H

#include <stdint.h>

typedef struct {
    const char *p;
    int len;
} divoom_hs_str_t;

#ifdef DIVOOM_SIMULATOR
#include "uhttpd/uhttpd.h"

typedef struct uh_connection divoom_hs_conn_t;

#define DIVOOM_HS_EV_HEAD_COMPLETE  UH_EV_HEAD_COMPLETE
#define DIVOOM_HS_EV_BODY           UH_EV_BODY
#define DIVOOM_HS_EV_COMPLETE       UH_EV_COMPLETE

#define DIVOOM_HTTP_STATUS_OK                    HTTP_STATUS_OK
#define DIVOOM_HTTP_STATUS_INTERNAL_SERVER_ERROR HTTP_STATUS_INTERNAL_SERVER_ERROR

static inline void *divoom_hs_conn_handle(divoom_hs_conn_t *conn)
{
    return (void *)conn;
}

static inline void divoom_hs_conn_serve_file(divoom_hs_conn_t *conn)
{
    conn->serve_file(conn);
}

static inline divoom_hs_str_t divoom_hs_conn_get_body(divoom_hs_conn_t *conn)
{
    struct uh_str s = conn->get_body(conn);
    divoom_hs_str_t r = { s.p, s.len };
    return r;
}

static inline divoom_hs_str_t divoom_hs_conn_extract_body(divoom_hs_conn_t *conn)
{
    struct uh_str s = conn->extract_body(conn);
    divoom_hs_str_t r = { s.p, s.len };
    return r;
}

static inline divoom_hs_str_t divoom_hs_conn_get_header(divoom_hs_conn_t *conn, const char *name)
{
    struct uh_str s = conn->get_header(conn, name);
    divoom_hs_str_t r = { s.p, s.len };
    return r;
}

static inline void divoom_hs_conn_send_head(divoom_hs_conn_t *conn, int status, int content_len,
                                            const char *extra_hdrs)
{
    conn->send_head(conn, status, content_len, extra_hdrs);
}

static inline void divoom_hs_conn_chunk_send(divoom_hs_conn_t *conn, const char *data, int len)
{
    conn->chunk_send(conn, data, len);
}

#define divoom_hs_conn_chunk_printf(conn, ...) ((conn)->chunk_printf((conn), __VA_ARGS__))

static inline void divoom_hs_conn_chunk_end(divoom_hs_conn_t *conn)
{
    conn->chunk_end(conn);
}

static inline void divoom_hs_conn_done(divoom_hs_conn_t *conn)
{
    conn->done(conn);
}

static inline void divoom_hs_conn_error(divoom_hs_conn_t *conn, int status, const char *msg)
{
    conn->error(conn, status, msg);
}

void divoom_http_server_port_run_uhttpd(int port);

#else /* FreeRTOS: SHTTPD */

#define DIVOOM_HS_EV_HEAD_COMPLETE  1
#define DIVOOM_HS_EV_BODY           2
#define DIVOOM_HS_EV_COMPLETE       3

#define DIVOOM_HTTP_STATUS_OK                    200
#define DIVOOM_HTTP_STATUS_INTERNAL_SERVER_ERROR 500

typedef struct divoom_hs_conn divoom_hs_conn_t;

void *divoom_hs_conn_handle(divoom_hs_conn_t *conn);
void divoom_hs_conn_serve_file(divoom_hs_conn_t *conn);
divoom_hs_str_t divoom_hs_conn_get_body(divoom_hs_conn_t *conn);
divoom_hs_str_t divoom_hs_conn_extract_body(divoom_hs_conn_t *conn);
divoom_hs_str_t divoom_hs_conn_get_header(divoom_hs_conn_t *conn, const char *name);
void divoom_hs_conn_send_head(divoom_hs_conn_t *conn, int status, int content_len,
                              const char *extra_hdrs);
void divoom_hs_conn_chunk_send(divoom_hs_conn_t *conn, const char *data, int len);
void divoom_hs_conn_chunk_printf(divoom_hs_conn_t *conn, const char *fmt, ...);
void divoom_hs_conn_chunk_end(divoom_hs_conn_t *conn);
void divoom_hs_conn_done(divoom_hs_conn_t *conn);
void divoom_hs_conn_error(divoom_hs_conn_t *conn, int status, const char *msg);

typedef void (*divoom_hs_event_handler_t)(divoom_hs_conn_t *conn, int event);

void divoom_http_server_port_register_shttpd(void *ctx);
void divoom_http_server_port_run_shttpd(void *param);

#endif /* DIVOOM_SIMULATOR */

#endif /* DIVOOM_HTTP_SERVER_PORT_H */

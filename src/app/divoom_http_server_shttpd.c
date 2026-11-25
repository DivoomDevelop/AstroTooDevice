/**
 * @file divoom_http_server_shttpd.c
 * SHTTPD transport for divoom_http_server (FreeRTOS only).
 */
#ifndef DIVOOM_SIMULATOR

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "divoom.h"
#include "divoom_http_server_port.h"
#include "compat_rtos.h"
#include "shttpd.h"

#define DIVOOM_SHTTPD_PORT_STR     "9000"
#define DIVOOM_SHTTPD_JSON_POST_MAX    (64 * 1024)
#define DIVOOM_SHTTPD_RESPONSE_MAX     (64 * 1024)
#define DIVOOM_SHTTPD_POST_GROW_STEP   (4 * 1024)
#define DIVOOM_SHTTPD_RESPONSE_HEADER_MAX  384
#define DIVOOM_SHTTPD_USERDATA_URI_PREFIX  "/userdata/"
#define DIVOOM_SHTTPD_USERDATA_URI_PATTERN "/userdata/*"
#define DIVOOM_SHTTPD_FILE_HEADER_MAX      256

extern void divoom_http_server_divoom_api_handler_hs(divoom_hs_conn_t *conn, int event);
extern void divoom_http_server_upload_handler_hs(divoom_hs_conn_t *conn, int event);
extern void divoom_http_server_upload_local_asset_handler_hs(divoom_hs_conn_t *conn, int event);
extern void divoom_http_server_upload_abort_hs(void *conn_handle);
extern void divoom_http_server_create_local_clock_handler_hs(divoom_hs_conn_t *conn, int event);
extern void divoom_http_server_patch_local_clock_handler_hs(divoom_hs_conn_t *conn, int event);
extern void divoom_http_server_replace_clock_dial_bg_handler_hs(divoom_hs_conn_t *conn, int event);

typedef struct {
    int status;
    char extra_hdrs[256];
    int content_length;
    int accept_len;
    int request_head_done;
    char *post_buf;
    int post_len;
    int post_cap;
    divoom_hs_str_t body_chunk;
    int complete;
    char *response_buf;
    int response_len;
    int response_cap;
    int response_sent;
    int response_done;
    int response_failed;
    char response_header[DIVOOM_SHTTPD_RESPONSE_HEADER_MAX];
    int response_header_len;
    int response_header_sent;
    int response_body_len;
} divoom_shttpd_sess_t;

typedef struct {
    void *file;
    long file_size;
    long body_sent;
    int head_only;
    int header_len;
    int header_sent;
    char header[DIVOOM_SHTTPD_FILE_HEADER_MAX];
} divoom_shttpd_file_state_t;

struct divoom_hs_conn {
    struct shttpd_arg *arg;
    divoom_shttpd_sess_t *sess;
};

static struct shttpd_ctx *g_shttpd_ctx;
static volatile int g_shttpd_exit;

static char g_shttpd_index_body[] =
    "<html><body>AstroToo HTTP server</body></html>";

static struct usr_file g_shttpd_local_files[] = {
    { "/userdata/", NULL },
    { "/userdata/index.html", g_shttpd_index_body },
};

static divoom_shttpd_sess_t *divoom_shttpd_sess_get(struct shttpd_arg *arg, int create)
{
    divoom_shttpd_sess_t *sess = (divoom_shttpd_sess_t *)arg->state;

    if(sess == NULL && create) {
        sess = (divoom_shttpd_sess_t *)divoom_system_malloc(sizeof(divoom_shttpd_sess_t));
        if(sess) {
            memset(sess, 0, sizeof(divoom_shttpd_sess_t));
            arg->state = sess;
            if(arg->priv == NULL) {
                arg->priv = arg;
            }
        }
    }
    return sess;
}

static void divoom_shttpd_sess_free(struct shttpd_arg *arg)
{
    divoom_shttpd_sess_t *sess = (divoom_shttpd_sess_t *)arg->state;

    if(sess) {
        divoom_system_free(sess->post_buf);
        divoom_system_free(sess->response_buf);
        divoom_system_free(sess);
        arg->state = NULL;
    }
}

static int divoom_shttpd_parse_content_length(divoom_hs_conn_t *conn)
{
    divoom_hs_str_t cl = divoom_hs_conn_get_header(conn, "Content-Length");
    char buf[48];

    if(cl.p == NULL || cl.len <= 0 || cl.len >= (int)sizeof(buf)) {
        return 0;
    }

    memcpy(buf, cl.p, (size_t)cl.len);
    buf[cl.len] = '\0';
    return atoi(buf);
}

static int divoom_shttpd_sess_append(divoom_shttpd_sess_t *sess, const char *data, int len, int max_len)
{
    int need;

    if(sess == NULL) {
        return -1;
    }
    if(data == NULL || len <= 0) {
        return 0;
    }
    if(max_len <= 0 || len > max_len || sess->post_len > max_len - len) {
        return -1;
    }
    need = sess->post_len + len + 1;
    if(need > sess->post_cap) {
        int cap = sess->post_cap ? sess->post_cap : DIVOOM_SHTTPD_POST_GROW_STEP;
        char *nb;

        while(cap < need) {
            int next_cap = cap + DIVOOM_SHTTPD_POST_GROW_STEP;

            cap = (next_cap > max_len + 1) ? need : next_cap;
        }
        nb = (char *)divoom_system_malloc((size_t)cap);
        if(nb == NULL) {
            return -1;
        }
        if(sess->post_buf && sess->post_len > 0) {
            memcpy(nb, sess->post_buf, (size_t)sess->post_len);
        }
        divoom_system_free(sess->post_buf);
        sess->post_buf = nb;
        sess->post_cap = cap;
    }
    memcpy(sess->post_buf + sess->post_len, data, (size_t)len);
    sess->post_len += len;
    sess->post_buf[sess->post_len] = '\0';
    return 0;
}

static int divoom_shttpd_response_append(divoom_shttpd_sess_t *sess,
                                         const char *data, int len)
{
    int need;

    if(sess == NULL || data == NULL || len <= 0) {
        return 0;
    }
    if(len > DIVOOM_SHTTPD_RESPONSE_MAX ||
       sess->response_len > DIVOOM_SHTTPD_RESPONSE_MAX - len) {
        return -1;
    }

    need = sess->response_len + len;
    if(need > sess->response_cap) {
        char *new_buf = (char *)divoom_system_malloc((size_t)need);

        if(new_buf == NULL) {
            return -1;
        }
        if(sess->response_buf && sess->response_len > 0) {
            memcpy(new_buf, sess->response_buf, (size_t)sess->response_len);
        }
        divoom_system_free(sess->response_buf);
        sess->response_buf = new_buf;
        sess->response_cap = need;
    }

    memcpy(sess->response_buf + sess->response_len, data, (size_t)len);
    sess->response_len += len;
    return 0;
}

static void divoom_shttpd_flush_response(struct shttpd_arg *arg,
                                         divoom_shttpd_sess_t *sess)
{
    static const char failure_body[] =
        "{\"ReturnCode\":1,\"ReturnMessage\":\"response too large or out of memory\"}";
    const char *body;
    int body_len;
    int room;

    if(arg == NULL || sess == NULL || !sess->response_done) {
        return;
    }

    body = sess->response_failed ? failure_body : sess->response_buf;

    if(sess->response_header_len == 0) {
        if(sess->response_failed) {
            sess->status = 500;
        }
        sess->response_body_len = sess->response_failed ?
            (int)sizeof(failure_body) - 1 : sess->response_len;
        sess->response_header_len = snprintf(
            sess->response_header, sizeof(sess->response_header),
            "HTTP/1.1 %d OK\r\n%sContent-Length: %d\r\nConnection: close\r\n\r\n",
            sess->status, sess->extra_hdrs, sess->response_body_len);
        if(sess->response_header_len <= 0 ||
           sess->response_header_len >= (int)sizeof(sess->response_header)) {
            sess->response_header_len = snprintf(
                sess->response_header, sizeof(sess->response_header),
                "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n"
                "Connection: close\r\n\r\n");
            sess->response_body_len = 0;
        }
    }
    body_len = sess->response_body_len;

    room = arg->out.len - arg->out.num_bytes;
    if(room > 0 && sess->response_header_sent < sess->response_header_len) {
        int left = sess->response_header_len - sess->response_header_sent;
        int copy_len = left < room ? left : room;

        memcpy(arg->out.buf + arg->out.num_bytes,
               sess->response_header + sess->response_header_sent,
               (size_t)copy_len);
        arg->out.num_bytes += copy_len;
        sess->response_header_sent += copy_len;
        room -= copy_len;
    }

    if(room > 0 && sess->response_header_sent >= sess->response_header_len &&
       sess->response_sent < body_len) {
        int left = body_len - sess->response_sent;
        int copy_len = left < room ? left : room;

        memcpy(arg->out.buf + arg->out.num_bytes,
               body + sess->response_sent, (size_t)copy_len);
        arg->out.num_bytes += copy_len;
        sess->response_sent += copy_len;
    }

    if(sess->response_header_sent >= sess->response_header_len &&
       sess->response_sent >= body_len) {
        divoom_shttpd_sess_free(arg);
        arg->flags |= SHTTPD_END_OF_OUTPUT;
    }
}

void *divoom_hs_conn_handle(divoom_hs_conn_t *conn)
{
    if(conn == NULL || conn->arg == NULL) {
        return NULL;
    }
    return conn->arg->priv;
}

void divoom_hs_conn_serve_file(divoom_hs_conn_t *conn)
{
    (void)conn;
}

divoom_hs_str_t divoom_hs_conn_get_body(divoom_hs_conn_t *conn)
{
    divoom_hs_str_t r = { NULL, 0 };

    if(conn && conn->sess && conn->sess->post_buf) {
        r.p = conn->sess->post_buf;
        r.len = conn->sess->post_len;
    }
    return r;
}

divoom_hs_str_t divoom_hs_conn_extract_body(divoom_hs_conn_t *conn)
{
    if(conn) {
        return conn->sess ? conn->sess->body_chunk : (divoom_hs_str_t){ NULL, 0 };
    }
    return (divoom_hs_str_t){ NULL, 0 };
}

divoom_hs_str_t divoom_hs_conn_get_header(divoom_hs_conn_t *conn, const char *name)
{
    divoom_hs_str_t r = { NULL, 0 };
    const char *hdr;

    if(conn == NULL || conn->arg == NULL || name == NULL) {
        return r;
    }
    hdr = shttpd_get_header(conn->arg, name);
    if(hdr) {
        r.p = hdr;
        r.len = (int)strlen(hdr);
    }
    return r;
}

void divoom_hs_conn_send_head(divoom_hs_conn_t *conn, int status, int content_len,
                              const char *extra_hdrs)
{
    (void)content_len;

    if(conn == NULL || conn->sess == NULL) {
        return;
    }
    conn->sess->status = status;
    if(extra_hdrs) {
        strncpy(conn->sess->extra_hdrs, extra_hdrs, sizeof(conn->sess->extra_hdrs) - 1);
        conn->sess->extra_hdrs[sizeof(conn->sess->extra_hdrs) - 1] = '\0';
    } else {
        conn->sess->extra_hdrs[0] = '\0';
    }
}

void divoom_hs_conn_chunk_send(divoom_hs_conn_t *conn, const char *data, int len)
{
    if(conn == NULL || conn->sess == NULL || data == NULL || len <= 0) {
        return;
    }
    if(divoom_shttpd_response_append(conn->sess, data, len) != 0) {
        conn->sess->response_failed = 1;
    }
}

void divoom_hs_conn_chunk_printf(divoom_hs_conn_t *conn, const char *fmt, ...)
{
    va_list ap;
    char buf[2048];
    int len;

    if(conn == NULL || conn->sess == NULL || fmt == NULL) {
        return;
    }
    va_start(ap, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if(len < 0) {
        conn->sess->response_failed = 1;
        return;
    }
    if(len >= (int)sizeof(buf)) {
        len = (int)sizeof(buf) - 1;
    }
    if(divoom_shttpd_response_append(conn->sess, buf, len) != 0) {
        conn->sess->response_failed = 1;
    }
}

void divoom_hs_conn_chunk_end(divoom_hs_conn_t *conn)
{
    (void)conn;
}

void divoom_hs_conn_done(divoom_hs_conn_t *conn)
{
    if(conn && conn->sess) {
        conn->sess->response_done = 1;
    }
}

void divoom_hs_conn_error(divoom_hs_conn_t *conn, int status, const char *msg)
{
    if(conn == NULL || conn->arg == NULL) {
        return;
    }
    shttpd_printf(conn->arg, "HTTP/1.1 %d Internal Server Error\r\n"
                              "Content-Type: text/plain\r\n\r\n%s",
                  status, msg ? msg : "error");
    conn->arg->flags |= SHTTPD_END_OF_OUTPUT;
}

static void divoom_shttpd_dispatch_multipart(struct shttpd_arg *arg,
                                             divoom_hs_event_handler_t handler)
{
    divoom_hs_conn_t conn;
    divoom_shttpd_sess_t *sess;
    int more = (arg->flags & SHTTPD_MORE_POST_DATA) ? 1 : 0;

    if(arg->flags & SHTTPD_CONNECTION_ERROR) {
        divoom_http_server_upload_abort_hs(arg->priv);
        divoom_shttpd_sess_free(arg);
        return;
    }

    sess = divoom_shttpd_sess_get(arg, 1);
    if(sess == NULL) {
        shttpd_printf(arg, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
        arg->flags |= SHTTPD_END_OF_OUTPUT;
        return;
    }

    conn.arg = arg;
    conn.sess = sess;

    if(sess->response_done) {
        divoom_shttpd_flush_response(arg, sess);
        return;
    }

    if(!sess->request_head_done) {
        sess->content_length = divoom_shttpd_parse_content_length(&conn);
        sess->request_head_done = 1;
        handler(&conn, DIVOOM_HS_EV_HEAD_COMPLETE);
        if(arg->flags & SHTTPD_END_OF_OUTPUT) {
            divoom_shttpd_sess_free(arg);
            return;
        }
        if(sess->response_done) {
            divoom_shttpd_flush_response(arg, sess);
            return;
        }
    }

    if(arg->in.buf && arg->in.len > 0) {
        sess->body_chunk.p = arg->in.buf;
        sess->body_chunk.len = arg->in.len;
        arg->in.num_bytes = arg->in.len;
        handler(&conn, DIVOOM_HS_EV_BODY);
        if(arg->flags & SHTTPD_END_OF_OUTPUT) {
            divoom_shttpd_sess_free(arg);
            return;
        }
        if(sess->response_done) {
            divoom_shttpd_flush_response(arg, sess);
            return;
        }
        sess->accept_len += arg->in.len;
    }

    if(!more && !sess->complete) {
        sess->complete = 1;
        handler(&conn, DIVOOM_HS_EV_COMPLETE);
        if(arg->flags & SHTTPD_END_OF_OUTPUT) {
            divoom_shttpd_sess_free(arg);
            return;
        }
        divoom_shttpd_flush_response(arg, sess);
    }
}

static void divoom_shttpd_dispatch_simple_post(struct shttpd_arg *arg,
                                              divoom_hs_event_handler_t handler)
{
    divoom_hs_conn_t conn;
    divoom_shttpd_sess_t *sess;
    int more = (arg->flags & SHTTPD_MORE_POST_DATA) ? 1 : 0;

    sess = divoom_shttpd_sess_get(arg, 1);
    if(sess == NULL) {
        shttpd_printf(arg, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
        arg->flags |= SHTTPD_END_OF_OUTPUT;
        return;
    }

    conn.arg = arg;
    conn.sess = sess;

    if(sess->response_done) {
        divoom_shttpd_flush_response(arg, sess);
        return;
    }

    if(!sess->request_head_done) {
        sess->content_length = divoom_shttpd_parse_content_length(&conn);
        sess->request_head_done = 1;
        if(sess->content_length > DIVOOM_SHTTPD_JSON_POST_MAX) {
            divoom_hs_conn_error(&conn, DIVOOM_HTTP_STATUS_INTERNAL_SERVER_ERROR, "Too big");
            divoom_shttpd_sess_free(arg);
            return;
        }
    }

    if(arg->in.buf && arg->in.len > 0) {
        if(divoom_shttpd_sess_append(sess, arg->in.buf, arg->in.len,
                                     DIVOOM_SHTTPD_JSON_POST_MAX) != 0) {
            divoom_hs_conn_error(&conn, DIVOOM_HTTP_STATUS_INTERNAL_SERVER_ERROR, "Too big");
            divoom_shttpd_sess_free(arg);
            return;
        }
        arg->in.num_bytes = arg->in.len;
    }

    if(!more && !sess->complete) {
        sess->complete = 1;
        handler(&conn, DIVOOM_HS_EV_COMPLETE);
        if(arg->flags & SHTTPD_END_OF_OUTPUT) {
            divoom_shttpd_sess_free(arg);
            return;
        }
        divoom_shttpd_flush_response(arg, sess);
    }
}

static void divoom_shttpd_cb_divoom_api(struct shttpd_arg *arg)
{
    if(arg->flags & SHTTPD_CONNECTION_ERROR) {
        divoom_shttpd_sess_free(arg);
        return;
    }
    divoom_shttpd_dispatch_simple_post(arg, divoom_http_server_divoom_api_handler_hs);
}

static void divoom_shttpd_cb_upload(struct shttpd_arg *arg)
{
    divoom_shttpd_dispatch_multipart(arg, divoom_http_server_upload_handler_hs);
}

static void divoom_shttpd_cb_upload_local_asset(struct shttpd_arg *arg)
{
    divoom_shttpd_dispatch_multipart(arg, divoom_http_server_upload_local_asset_handler_hs);
}

static void divoom_shttpd_cb_create_local_clock(struct shttpd_arg *arg)
{
    divoom_shttpd_dispatch_multipart(arg, divoom_http_server_create_local_clock_handler_hs);
}

static void divoom_shttpd_cb_patch_local_clock(struct shttpd_arg *arg)
{
    divoom_shttpd_dispatch_multipart(arg, divoom_http_server_patch_local_clock_handler_hs);
}

static void divoom_shttpd_cb_replace_dial_bg(struct shttpd_arg *arg)
{
    divoom_shttpd_dispatch_multipart(arg, divoom_http_server_replace_clock_dial_bg_handler_hs);
}

static int divoom_shttpd_ascii_equal_ignore_case(const char *left, const char *right)
{
    unsigned char a;
    unsigned char b;

    if(left == NULL || right == NULL) {
        return 0;
    }

    while(*left && *right) {
        a = (unsigned char)*left++;
        b = (unsigned char)*right++;
        if(a >= 'A' && a <= 'Z') {
            a = (unsigned char)(a - 'A' + 'a');
        }
        if(b >= 'A' && b <= 'Z') {
            b = (unsigned char)(b - 'A' + 'a');
        }
        if(a != b) {
            return 0;
        }
    }
    return *left == '\0' && *right == '\0';
}

static const char *divoom_shttpd_file_content_type(const char *path)
{
    const char *ext = path ? strrchr(path, '.') : NULL;

    if(ext == NULL) {
        return "application/octet-stream";
    }
    if(divoom_shttpd_ascii_equal_ignore_case(ext, ".webp")) {
        return "image/webp";
    }
    if(divoom_shttpd_ascii_equal_ignore_case(ext, ".jpg") ||
       divoom_shttpd_ascii_equal_ignore_case(ext, ".jpeg")) {
        return "image/jpeg";
    }
    if(divoom_shttpd_ascii_equal_ignore_case(ext, ".png")) {
        return "image/png";
    }
    if(divoom_shttpd_ascii_equal_ignore_case(ext, ".gif")) {
        return "image/gif";
    }
    if(divoom_shttpd_ascii_equal_ignore_case(ext, ".mp4")) {
        return "video/mp4";
    }
    return "application/octet-stream";
}

static int divoom_shttpd_userdata_uri_valid(const char *uri)
{
    const char *part;
    const char *segment;
    size_t prefix_len = strlen(DIVOOM_SHTTPD_USERDATA_URI_PREFIX);

    if(uri == NULL || strncmp(uri, DIVOOM_SHTTPD_USERDATA_URI_PREFIX, prefix_len) != 0) {
        return 0;
    }

    part = uri + prefix_len;
    if(*part == '\0') {
        return 0;
    }

    segment = part;
    for(;; part++) {
        unsigned char ch = (unsigned char)*part;

        if(ch == '\\' || ch == ':' || ch == '%' || ch == '#' ||
           (ch != '\0' && (ch < 0x20 || ch == 0x7f))) {
            return 0;
        }
        if(ch == '/' || ch == '\0') {
            size_t segment_len = (size_t)(part - segment);

            if(segment_len == 0 ||
               (segment_len == 1 && segment[0] == '.') ||
               (segment_len == 2 && segment[0] == '.' && segment[1] == '.')) {
                return 0;
            }
            if(ch == '\0') {
                break;
            }
            segment = part + 1;
        }
    }
    return 1;
}

static void divoom_shttpd_file_state_free(struct shttpd_arg *arg)
{
    divoom_shttpd_file_state_t *state;

    if(arg == NULL) {
        return;
    }

    state = (divoom_shttpd_file_state_t *)arg->state;
    if(state) {
        if(state->file) {
            divoom_port_file_close(state->file);
        }
        divoom_system_free(state);
        arg->state = NULL;
    }
}

static void divoom_shttpd_file_send_error(struct shttpd_arg *arg, int status,
                                          const char *reason)
{
    int body_len = reason ? (int)strlen(reason) : 0;

    shttpd_printf(arg,
                  "HTTP/1.1 %d %s\r\n"
                  "Content-Type: text/plain\r\n"
                  "Content-Length: %d\r\n"
                  "Connection: close\r\n\r\n%s",
                  status, reason ? reason : "Error", body_len,
                  reason ? reason : "");
    arg->flags |= SHTTPD_END_OF_OUTPUT;
}

static void divoom_shttpd_cb_userdata_file(struct shttpd_arg *arg)
{
    divoom_shttpd_file_state_t *state;
    const char *method;
    const char *uri;
    int room;
    size_t read_len;

    if(arg->flags & SHTTPD_CONNECTION_ERROR) {
        divoom_shttpd_file_state_free(arg);
        return;
    }

    state = (divoom_shttpd_file_state_t *)arg->state;
    if(state == NULL) {
        void *file;
        long file_size;

        method = shttpd_get_env(arg, "REQUEST_METHOD");
        uri = shttpd_get_env(arg, "REQUEST_URI");
        if(method == NULL ||
           (strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0)) {
            divoom_shttpd_file_send_error(arg, 405, "Method Not Allowed");
            return;
        }
        if(!divoom_shttpd_userdata_uri_valid(uri)) {
            divoom_print("SHTTPD reject file uri: %s\n", uri ? uri : "(null)");
            divoom_shttpd_file_send_error(arg, 403, "Forbidden");
            return;
        }

        file = divoom_port_file_open(uri, "rb");
        if(file == NULL) {
            divoom_print("SHTTPD file not found: %s\n", uri);
            divoom_shttpd_file_send_error(arg, 404, "Not Found");
            return;
        }
        if(divoom_port_file_seek(file, 0, SEEK_END) != 0 ||
           (file_size = divoom_port_file_tell(file)) < 0 ||
           divoom_port_file_seek(file, 0, SEEK_SET) != 0) {
            divoom_port_file_close(file);
            divoom_print("SHTTPD file seek failed: %s\n", uri);
            divoom_shttpd_file_send_error(arg, 500, "Internal Server Error");
            return;
        }

        state = (divoom_shttpd_file_state_t *)divoom_system_malloc(sizeof(*state));
        if(state == NULL) {
            divoom_port_file_close(file);
            divoom_shttpd_file_send_error(arg, 500, "Internal Server Error");
            return;
        }
        memset(state, 0, sizeof(*state));
        state->file = file;
        state->file_size = file_size;
        state->head_only = strcmp(method, "HEAD") == 0;
        state->header_len = snprintf(state->header, sizeof(state->header),
                                     "HTTP/1.1 200 OK\r\n"
                                     "Content-Type: %s\r\n"
                                     "Content-Length: %ld\r\n"
                                     "Connection: close\r\n"
                                     "Cache-Control: no-cache\r\n\r\n",
                                     divoom_shttpd_file_content_type(uri), file_size);
        if(state->header_len <= 0 ||
           state->header_len >= (int)sizeof(state->header)) {
            arg->state = state;
            divoom_shttpd_file_state_free(arg);
            divoom_shttpd_file_send_error(arg, 500, "Internal Server Error");
            return;
        }
        arg->state = state;
        divoom_print("SHTTPD serve file: %s, len: %ld\n", uri, file_size);
    }

    room = arg->out.len - arg->out.num_bytes;
    if(state->header_sent < state->header_len && room > 0) {
        int left = state->header_len - state->header_sent;
        int copy_len = left < room ? left : room;

        memcpy(arg->out.buf + arg->out.num_bytes,
               state->header + state->header_sent, (size_t)copy_len);
        arg->out.num_bytes += copy_len;
        state->header_sent += copy_len;
        room -= copy_len;
    }
    if(state->header_sent < state->header_len) {
        return;
    }

    if(state->head_only || state->file_size == 0) {
        divoom_shttpd_file_state_free(arg);
        arg->flags |= SHTTPD_END_OF_OUTPUT;
        return;
    }

    if(room > 0 && state->body_sent < state->file_size) {
        long remaining = state->file_size - state->body_sent;
        size_t request_len = (size_t)room;

        if(remaining < (long)request_len) {
            request_len = (size_t)remaining;
        }
        read_len = divoom_port_file_read(arg->out.buf + arg->out.num_bytes,
                                         1, request_len, state->file);
        arg->out.num_bytes += (int)read_len;
        state->body_sent += (long)read_len;
        if(read_len == 0 && state->body_sent < state->file_size) {
            divoom_print("SHTTPD file read stopped: %ld/%ld\n",
                         state->body_sent, state->file_size);
            divoom_shttpd_file_state_free(arg);
            arg->flags |= SHTTPD_END_OF_OUTPUT;
            return;
        }
    }

    if(state->body_sent >= state->file_size) {
        divoom_shttpd_file_state_free(arg);
        arg->flags |= SHTTPD_END_OF_OUTPUT;
    }
}

void divoom_http_server_port_register_shttpd(void *ctx)
{
    struct shttpd_ctx *sctx = (struct shttpd_ctx *)ctx;

    /* RTOS SHTTPD static files are memory-backed, so /userdata needs a file callback. */
    shttpd_register_uri(sctx, "/divoom_api", divoom_shttpd_cb_divoom_api, NULL);
    shttpd_register_uri(sctx, "/upload", divoom_shttpd_cb_upload, NULL);
    shttpd_register_uri(sctx, "/upload_local_asset", divoom_shttpd_cb_upload_local_asset, NULL);
    shttpd_register_uri(sctx, "/replace_clock_dial_bg", divoom_shttpd_cb_replace_dial_bg, NULL);
    shttpd_register_uri(sctx, "/create_local_clock", divoom_shttpd_cb_create_local_clock, NULL);
    shttpd_register_uri(sctx, "/patch_local_clock", divoom_shttpd_cb_patch_local_clock, NULL);
    shttpd_register_uri(sctx, DIVOOM_SHTTPD_USERDATA_URI_PATTERN,
                        divoom_shttpd_cb_userdata_file, NULL);
}

void divoom_http_server_port_run_shttpd(void *param)
{
    int listen_ok = 0, root_ok;
    uint32 last_attempt_ms = 0;
    unsigned long attempts = 0;

    (void)param;

    g_shttpd_exit = 0;
    _shttpd_init_local_file(g_shttpd_local_files,
                            sizeof(g_shttpd_local_files) / sizeof(g_shttpd_local_files[0]));
    while(!g_shttpd_exit) {
        uint32 now_ms = divoom_system_get_tickms();
        int ready = divoom_wifi_model_is_network_ready();
        int paused = divoom_wifi_model_network_services_paused();

        if(!listen_ok) {
            /* WiFi initializes lwIP lazily; do not create sockets before GOT_IP. */
            if(!ready || paused ||
               (attempts != 0 && (uint32)(now_ms - last_attempt_ms) < 5000)) {
                divoom_system_delay_ms(200);
                continue;
            }
            last_attempt_ms = now_ms;
            attempts++;
            g_shttpd_ctx = shttpd_init(0, NULL);
            if(g_shttpd_ctx == NULL) {
                divoom_print("[http_diag] init_failed errno=%d (%s) retry_ms=5000\n",
                    errno, strerror(errno));
                continue;
            }

            listen_ok = shttpd_set_option(g_shttpd_ctx, "ports", DIVOOM_SHTTPD_PORT_STR);
            root_ok = shttpd_set_option(g_shttpd_ctx, "root", "/userdata");
            if(!listen_ok || !root_ok) {
                divoom_print("[http_diag] configure_failed listen_ok=%d root_ok=%d retry_ms=5000\n",
                    listen_ok, root_ok);
                shttpd_fini(g_shttpd_ctx);
                g_shttpd_ctx = NULL;
                listen_ok = 0;
                continue;
            }
            divoom_http_server_port_register_shttpd(g_shttpd_ctx);
            divoom_print("SHTTPD started on port %s\n", DIVOOM_SHTTPD_PORT_STR);
        }
        shttpd_poll(g_shttpd_ctx, 200);
    }

    if(g_shttpd_ctx != NULL)
        shttpd_fini(g_shttpd_ctx);
    g_shttpd_ctx = NULL;
    divoom_print("SHTTPD stopped\n");
}

#endif /* !DIVOOM_SIMULATOR */

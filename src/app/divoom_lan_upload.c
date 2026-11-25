/* Strict two-part streaming multipart parser; file bytes never buffer in RAM. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "divoom.h"
#include "divoom_http_server_port.h"
#include "divoom_lan_storage.h"
#include "divoom_watchface_local_api.h"
#include "divoom_light_app.h"
typedef struct {
    void *owner;
    int kind, state, used, expected, received, error;
    char header[4096], boundary[128], path[192];
    char *json;
    FILE *file;
} lan_upload;
static lan_upload *active;
static unsigned sequence;
void divoom_lan_upload_abort(void *owner) {
    if(active && active->owner == owner) {
        if(active->file) fclose(active->file);
        if(active->path[0]) unlink(active->path);
        if(active->json) divoom_system_free(active->json);
        divoom_system_free(active);
        active = NULL;
    }
}
static void reply(divoom_hs_conn_t *c, const char *json) {
    const char *body = json ? json : "{\"ReturnCode\":1,\"ReturnMessage\":\"oom\"}";
    divoom_hs_conn_send_head(c, 200, -1, "Content-Type: application/json; charset=UTF-8\r\n");
    divoom_hs_conn_chunk_send(c, body, (int)strlen(body));
    divoom_hs_conn_chunk_end(c);
    divoom_hs_conn_done(c);
}
static int part_header(lan_upload *u) {
    char *p = strstr(u->header, "\r\nContent-Length:");
    char *end;
    long n;
    if(!p) return 0;
    n = strtol(p + strlen("\r\nContent-Length:"), &end, 10);
    if(end[0] != '\r' || end[1] != '\n' || n <= 0 ||
       n > (u->state == 0 ? 65536 : 6291456)) return 0;
    if(u->state == 0) {
        char *line = strstr(u->header, "\r\n");
        int len = line ? (int)(line - u->header) : 0;
        if(len < 3 || len >= sizeof u->boundary || strncmp(u->header, "--", 2)) return 0;
        memcpy(u->boundary, u->header, len);
        u->boundary[len] = 0;
        if(!strstr(u->header, "name=\"json\"")) return 0;
        u->json = divoom_system_malloc(n + 1);
        if(!u->json) return 0;
        u->state = 1;
    } else {
        char prefix[140];
        snprintf(prefix, sizeof prefix, "\r\n%s\r\n", u->boundary);
        if(strncmp(u->header, prefix, strlen(prefix)) || !strstr(u->header, "filename=\"")) return 0;
        divoom_app_com_mk_dir_for_file_path(u->path);
        u->file = divoom_port_file_open(u->path, "wb");
        if(!u->file) return 0;
        u->state = 3;
    }
    u->expected = (int)n;
    u->received = 0;
    u->used = 0;
    return 1;
}
static void feed(lan_upload *u, const char *p, int len) {
    while(len > 0 && !u->error) {
        if(u->state == 0 || u->state == 2 || u->state == 4) {
            if(u->used >= sizeof u->header - 1) { u->error = 1; break; }
            u->header[u->used++] = *p++; len--;
            u->header[u->used] = 0;
            if(u->state != 4 && u->used >= 4 &&
               !memcmp(u->header + u->used - 4, "\r\n\r\n", 4) && !part_header(u))
                u->error = 1;
        } else {
            int n = u->expected - u->received;
            if(n > len) n = len;
            if(u->state == 1) memcpy(u->json + u->received, p, n);
            else if(fwrite(p, 1, n, u->file) != (size_t)n) { u->error = 1; break; }
            p += n; len -= n; u->received += n;
            if(u->received == u->expected) {
                if(u->state == 1) { u->json[u->expected] = 0; u->state = 2; }
                else { if(fclose(u->file) != 0) u->error = 1; u->file = NULL; u->state = 4; }
                u->used = 0; u->header[0] = 0;
            }
        }
    }
}
void divoom_lan_multipart(divoom_hs_conn_t *conn, int event, int kind) {
    void *owner = divoom_hs_conn_handle(conn);
    lan_upload *u;
    if(event == DIVOOM_HS_EV_HEAD_COMPLETE) {
        if(active) {
            reply(conn, "{\"ReturnCode\":1,\"ReturnMessage\":\"another upload is active\"}");
            return;
        }
        active = divoom_system_malloc(sizeof *active);
        if(!active) { reply(conn, NULL); return; }
        memset(active, 0, sizeof *active);
        active->owner = owner; active->kind = kind;
        /* Never trust a client filename as a device path. */
        do {
            snprintf(active->path, sizeof active->path, DIVOOM_LAN_ROOT "/uploads/%lu-%u.bin",
                (unsigned long)time(NULL), ++sequence);
        } while(divoom_port_file_exists(active->path));
        return;
    }
    if(!active || active->owner != owner) return;
    u = active;
    if(event == DIVOOM_HS_EV_BODY) {
        divoom_hs_str_t body = divoom_hs_conn_extract_body(conn);
        feed(u, body.p, body.len);
        if(u->error) {
            reply(conn, "{\"ReturnCode\":1,\"ReturnMessage\":\"invalid multipart or storage failure\"}");
            divoom_lan_upload_abort(owner);
        }
    } else if(event == DIVOOM_HS_EV_COMPLETE) {
        char tail[144], *result = NULL;
        divoom_cJSON *meta;
        snprintf(tail, sizeof tail, "\r\n%s--\r\n", u->boundary);
        meta = u->json ? divoom_cJSON_Parse(u->json) : NULL;
        if(u->error || u->state != 4 || strcmp(tail, u->header) || !meta) {
            result = divoom_watchface_local_http_error_json("", "incomplete multipart");
        } else if(kind == 1) {
            result = divoom_watchface_local_http_create_local_clock(u->json, u->path, u->expected);
        } else if(kind == 2) {
            result = divoom_watchface_local_http_patch_local_clock_with_upload(u->json, u->path, u->expected, NULL, 0);
        } else if(kind == 3) {
            int cid = divoom_cJSON_GetObjectInt(meta, "ClockId");
            if(divoom_cJSON_GetObjectInt(meta, "UseCurrentDisplayClock") == 1 || cid <= 0)
                cid = divoom_light_app_get_cur_clock_id();
            result = divoom_watchface_local_http_replace_clock_dial_bg(cid, u->path, u->expected);
        } else {
            divoom_cJSON *command = divoom_cJSON_GetObjectItem(meta, "Command");
            if(command && command->valuestring && !strcmp(command->valuestring, "Device/UploadLocalAsset") &&
               divoom_lan_validate_image(u->path, u->expected, NULL, 0) == 0) {
                divoom_cJSON *out = divoom_cJSON_CreateObject();
                char ref[128];
                snprintf(ref, sizeof ref, "local://%s", strrchr(u->path, '/') + 1);
                divoom_cJSON_AddNumberToObject(out, "ReturnCode", 0);
                divoom_cJSON_AddStringToObject(out, "FileId", ref);
                result = divoom_cJSON_Print(out);
                divoom_cJSON_Delete(out);
                if(result) u->path[0] = 0; /* Keep staging until create/patch consumes it. */
            } else result = divoom_watchface_local_http_error_json("", "use Device/UploadLocalAsset with a valid image (<500KiB, at most 1024x1024)");
        }
        if(meta) divoom_cJSON_Delete(meta);
        reply(conn, result);
        if(result) divoom_system_free(result);
        divoom_lan_upload_abort(owner);
    }
}

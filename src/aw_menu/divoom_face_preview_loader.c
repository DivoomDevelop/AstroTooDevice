#include "divoom.h"
#include "divoom_pixel_format.h"
#include "divoom_face_preview_loader.h"

#define FACE_PREVIEW_IDLE_MS 200U
#define FACE_PREVIEW_PROBE_MS 500U
#define FACE_PREVIEW_RETRIES (FACE_PREVIEW_WINDOW * 4)
#ifndef FACE_PREVIEW_PERF
#define FACE_PREVIEW_PERF 0
#endif

typedef struct {
    unsigned generation;
    uint32 activity;
    int count, ids[FACE_PREVIEW_WINDOW];
    unsigned char ready[FACE_PREVIEW_WINDOW];
} face_preview_request;

typedef struct {
    int id, used, backoff;
    unsigned delivered;
    uint32 probe_at, touched;
} face_preview_retry;

struct divoom_face_preview_loader {
    void *lock, *exited;
    int running, started;
    face_preview_request request;
    divoom_face_preview_result *completed; /* Bounded, one-entry completion queue. */
    face_preview_retry retries[FACE_PREVIEW_RETRIES]; /* Worker only. */
    int cursor;
#if FACE_PREVIEW_PERF
    uint32 probes, decodes, decode_max, ui_lock_max;
#endif
};

static int face_preview_due(uint32 now, uint32 deadline)
{
    return (int32)(now - deadline) >= 0;
}

static face_preview_retry *face_preview_retry_for(divoom_face_preview_loader *loader, int id)
{
    int i, oldest = 0;
    uint32 now = divoom_system_get_tickms();
    for (i = 0; i < FACE_PREVIEW_RETRIES; i++)
    {
        if (loader->retries[i].used && loader->retries[i].id == id)
        {
            loader->retries[i].touched = now;
            return &loader->retries[i];
        }
        if (!loader->retries[i].used) oldest = i;
        else if (loader->retries[oldest].used &&
            (uint32)(now - loader->retries[i].touched) > (uint32)(now - loader->retries[oldest].touched)) oldest = i;
    }
    memset(&loader->retries[oldest], 0, sizeof(loader->retries[oldest]));
    loader->retries[oldest].used = 1;
    loader->retries[oldest].id = id;
    loader->retries[oldest].touched = now;
    loader->retries[oldest].probe_at = now;
    return &loader->retries[oldest];
}

static divoom_face_preview_result *face_preview_decode(int id, unsigned generation, char *path)
{
    int w = 0, h = 0, x, y;
    int album = id >= DIVOOM_PHOTO_START_CLOCK_ID;
    uint8 *src = divoom_rk_mpp_get_jpeg_info(path, &w, &h);
    divoom_face_preview_result *result;
    if (!src) return NULL;
    if (w <= 0 || h <= 0 || w > 4096 || h > 4096)
    {
        divoom_system_free(src);
        return NULL;
    }
    result = divoom_system_malloc(sizeof(*result));
    if (result)
    {
        uint16 *dst = (uint16 *)result->pixels;
        const uint16 *pixels = (const uint16 *)src;
        int draw_w = 60, draw_h = 60, left = 0, top = 0;
        /* Album covers are premultiplied BGRA WebPs, not RGB565 JPEGs.
         * Fit the entire cover inside 60x60, centered on the black thumb.
         * CPU scaling avoids sharing the UI's G2D engine. */
        if (album)
        {
            if (w > h) draw_h = h * 60 / w;
            else draw_w = w * 60 / h;
            if (draw_w < 1) draw_w = 1;
            if (draw_h < 1) draw_h = 1;
            left = (60 - draw_w) / 2;
            top = (60 - draw_h) / 2;
            memset(result->pixels, 0, sizeof(result->pixels));
        }
        for (y = 0; y < draw_h; y++)
            for (x = 0; x < draw_w; x++)
            {
                if (album)
                {
                    const uint8 *p = src + ((y * h / draw_h) * w + x * w / draw_w) * 4;
                    dst[(top + y) * 60 + left + x] = ((p[2] >> 3) << 11) | ((p[1] >> 2) << 5) | (p[0] >> 3);
                }
                else
                {
                    dst[y * 60 + x] = pixels[(y * h / 60) * w + x * w / 60];
                }
            }
        result->clock_id = id;
        result->generation = generation;
        divoom_lv_img_dsc_init_fmt(&result->image, 60, 60, LV_COLOR_FORMAT_RGB565, result->pixels);
    }
    divoom_system_free(src);
    return result;
}

static int face_preview_active(divoom_face_preview_loader *loader, unsigned generation)
{
    int active;
    divoom_system_get_lock(loader->lock);
    active = loader->running && loader->request.generation == generation && !loader->completed &&
        (uint32)(divoom_system_get_tickms() - loader->request.activity) >= FACE_PREVIEW_IDLE_MS;
    divoom_system_free_lock(loader->lock);
    return active;
}

static void face_preview_step(divoom_face_preview_loader *loader)
{
    face_preview_request request;
    int i;
    divoom_system_get_lock(loader->lock);
    request = loader->request;
    divoom_system_free_lock(loader->lock);
    if (request.count <= 0 || !face_preview_active(loader, request.generation)) return;
    for (i = 0; i < request.count; i++)
    {
        int slot = loader->cursor % request.count, id = request.ids[slot];
        uint32 now = divoom_system_get_tickms();
        face_preview_retry *retry;
        char path[128];
        divoom_face_preview_result *result;
        loader->cursor = (slot + 1) % request.count;
        if (!face_preview_active(loader, request.generation)) break;
        if (request.ready[slot] || id <= 0) continue;
        retry = face_preview_retry_for(loader, id);
        if (retry->delivered == request.generation || !face_preview_due(now, retry->probe_at)) continue;
        retry->probe_at = now + FACE_PREVIEW_PROBE_MS;
        if (id >= DIVOOM_PHOTO_START_CLOCK_ID)
            snprintf(path, sizeof(path), "%s/%d.webp", DIVOOM_ALBUM_COVER_INFO_DIR, id);
        else
            divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_REVIEW_PIC,
                DIVOOM_MANAGE_IMAGE_ID_BASE_TIME_DIAL_REVIEW_PIC + id, path);
#if FACE_PREVIEW_PERF
        loader->probes++;
#endif
        if (!divoom_port_file_exists(path))
        {
            /* Local-only loader: keep the default; another subsystem may supply the file later. */
            continue; /* Missing rows do not consume the single-decode budget. */
        }
#if FACE_PREVIEW_PERF
        loader->decodes++;
#endif
        result = face_preview_decode(id, request.generation, path);
#if FACE_PREVIEW_PERF
        now = (uint32)divoom_system_get_tickms() - now;
        if (now > loader->decode_max) loader->decode_max = now;
#endif
        if (result)
        {
            divoom_system_get_lock(loader->lock);
            if (loader->running && loader->request.generation == request.generation && !loader->completed)
            {
                loader->completed = result;
                retry->delivered = request.generation;
                result = NULL;
            }
            divoom_system_free_lock(loader->lock);
            divoom_system_free(result);
        }
        else
        {
            static const uint32 delay[] = {5000U, 10000U, 20000U, 30000U};
            retry->probe_at = (uint32)divoom_system_get_tickms() + delay[retry->backoff];
            if (retry->backoff < 3) retry->backoff++;
        }
        break;
    }
}

static void *face_preview_task(void *data)
{
    divoom_face_preview_loader *loader = data;
    for (;;)
    {
        int running;
        divoom_system_get_lock(loader->lock);
        running = loader->running;
        divoom_system_free_lock(loader->lock);
        if (!running) break;
        face_preview_step(loader);
        divoom_system_delay_ms(10);
    }
    divoom_system_put_sem(loader->exited);
    return NULL;
}

divoom_face_preview_loader *divoom_face_preview_create(void)
{
    divoom_face_preview_loader *loader = divoom_system_malloc(sizeof(*loader));
    if (!loader) return NULL;
    memset(loader, 0, sizeof(*loader));
    loader->lock = divoom_system_create_lock();
    loader->exited = divoom_system_create_sem(1, 0);
    if (!loader->lock || !loader->exited) { divoom_face_preview_destroy(loader); return NULL; }
    loader->running = 1;
    loader->request.generation = 1;
    loader->request.activity = divoom_system_get_tickms();
    loader->started = divoom_system_create_task(face_preview_task, "face_prev", loader,
        DIVOOM_TASK_STACK_LVGL_DIAL_PREV) != NULL;
    if (!loader->started) { divoom_face_preview_destroy(loader); return NULL; }
    return loader;
}

void divoom_face_preview_publish(divoom_face_preview_loader *loader, const int *ids,
    const unsigned char *ready, int count, int activity)
{
#if FACE_PREVIEW_PERF
    uint32 started = divoom_system_get_tickms();
#endif
    if (!loader) return;
    if (count < 0 || !ids || !ready) count = 0;
    if (count > FACE_PREVIEW_WINDOW) count = FACE_PREVIEW_WINDOW;
    divoom_system_get_lock(loader->lock);
#if FACE_PREVIEW_PERF
    {
        uint32 waited = (uint32)divoom_system_get_tickms() - started;
        if (waited > loader->ui_lock_max) loader->ui_lock_max = waited;
    }
#endif
    if (loader->request.count != count || (count && memcmp(loader->request.ids, ids, count * sizeof(*ids))))
    {
        if (++loader->request.generation == 0) loader->request.generation++;
        loader->request.count = count;
        if (count) memcpy(loader->request.ids, ids, count * sizeof(*ids));
    }
    if (count) memcpy(loader->request.ready, ready, count);
    if (activity) loader->request.activity = divoom_system_get_tickms();
    divoom_system_free_lock(loader->lock);
}

divoom_face_preview_result *divoom_face_preview_take(divoom_face_preview_loader *loader)
{
    divoom_face_preview_result *result;
    unsigned generation;
    if (!loader) return NULL;
    divoom_system_get_lock(loader->lock);
    result = loader->completed;
    loader->completed = NULL;
    generation = loader->request.generation;
    divoom_system_free_lock(loader->lock);
    if (result && result->generation != generation) { divoom_system_free(result); return NULL; }
    return result;
}

void divoom_face_preview_destroy(divoom_face_preview_loader *loader)
{
    if (!loader) return;
    if (loader->lock)
    {
        divoom_system_get_lock(loader->lock);
        loader->running = 0;
        divoom_system_free_lock(loader->lock);
    }
    if (loader->started) divoom_system_get_sem(loader->exited);
    divoom_system_free(loader->completed);
#if FACE_PREVIEW_PERF
    divoom_print("[face_preview] probes=%u decodes=%u decode_max_ms=%u ui_lock_max_ms=%u\n",
        loader->probes, loader->decodes, loader->decode_max, loader->ui_lock_max);
#endif
    if (loader->exited) divoom_system_destroy_sem(loader->exited);
    if (loader->lock) divoom_system_destroy_lock(loader->lock);
    divoom_system_free(loader);
}

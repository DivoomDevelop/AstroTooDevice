#ifndef DIVOOM_FACE_PREVIEW_LOADER_H
#define DIVOOM_FACE_PREVIEW_LOADER_H

#include "divoom_lvgl9_compat.h"

#define FACE_PREVIEW_WINDOW 8
typedef struct divoom_face_preview_loader divoom_face_preview_loader;
typedef struct {
    int clock_id;
    unsigned generation;
    lv_img_dsc_t image;
    unsigned char pixels[60 * 60 * 2];
} divoom_face_preview_result;

/* Local preview decoding only; missing files never request configs or downloads. */
divoom_face_preview_loader *divoom_face_preview_create(void);
/* Copies IDs/readiness only; no filesystem or decoder calls. UI thread owns this API. */
void divoom_face_preview_publish(divoom_face_preview_loader *loader, const int *ids,
    const unsigned char *ready, int count, int activity);
/* Transfers ownership to the UI; stale results are discarded before returning. */
divoom_face_preview_result *divoom_face_preview_take(divoom_face_preview_loader *loader);
/* UI must unbind/invalidate image before freeing a bound result. */
void divoom_face_preview_destroy(divoom_face_preview_loader *loader);

#endif

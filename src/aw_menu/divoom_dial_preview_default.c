#include "divoom.h"
#include "divoom_pixel_format.h"
#include "divoom_dial_preview_default.h"
#include "assets/dial_preview_default_jpg.h"

#include <string.h>

#define DIAL_PREVIEW_SIZE 60
#define DIAL_PREVIEW_BYTES (DIAL_PREVIEW_SIZE * DIAL_PREVIEW_SIZE * 2u)

/* One RGB565 copy per market page, shared by all of its rows. */
const lv_img_dsc_t *divoom_dial_preview_default_acquire(divoom_dial_preview_default *preview)
{
    int width = 0, height = 0;
    uint32 now = divoom_system_get_tickms();
    if (preview->pixels) return &preview->image;
    if (preview->attempted && (int32)(now - preview->retry_at) < 0) return NULL;
    preview->attempted = 1;
    preview->retry_at = now + 1000U;
    uint8_t *pixels = divoom_system_malloc(DIAL_PREVIEW_BYTES);
    if (!pixels) return NULL;
    if (!divoom_rk_mpp_decode_jpeg_rgb565_to_buf_info(
            (char *)s_dial_preview_default_jpg, sizeof(s_dial_preview_default_jpg),
            &width, &height, pixels, DIAL_PREVIEW_BYTES) ||
        width != DIAL_PREVIEW_SIZE || height != DIAL_PREVIEW_SIZE)
    {
        divoom_system_free(pixels);
        divoom_print("[dial_preview_default] decode failed\n");
        return NULL;
    }
    divoom_lv_img_dsc_init_fmt(&preview->image, DIAL_PREVIEW_SIZE, DIAL_PREVIEW_SIZE,
        LV_COLOR_FORMAT_RGB565, pixels);
    preview->pixels = pixels;
    return &preview->image;
}

void divoom_dial_preview_default_release(divoom_dial_preview_default *preview)
{
    if (preview->pixels)
    {
        lv_img_cache_invalidate_src(&preview->image);
        divoom_system_free(preview->pixels);
    }
    memset(preview, 0, sizeof(*preview));
}

const lv_img_dsc_t *divoom_dial_preview_default_image(void)
{
    static divoom_dial_preview_default legacy_preview;
    return divoom_dial_preview_default_acquire(&legacy_preview);
}

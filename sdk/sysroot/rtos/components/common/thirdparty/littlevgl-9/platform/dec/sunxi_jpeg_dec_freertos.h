#ifndef SUNXI_PNG_DEC_FREERTOS_H
#define SUNXI_PNG_DEC_FREERTOS_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(LV_USE_SUNXI_JPEG) && defined(OS_FREE_RTOS)
#include "lvgl.h"
#include "lvgl_private.h"



int32_t jpeg_decoder_init(void);
lv_draw_buf_t *jpeg_decoder_dec(lv_image_decoder_dsc_t *dsc, uint8_t *data, uint32_t size);
void jpeg_decoder_finish(lv_image_decoder_dsc_t *dsc);
void jpeg_decoder_deinit(void);

#endif


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

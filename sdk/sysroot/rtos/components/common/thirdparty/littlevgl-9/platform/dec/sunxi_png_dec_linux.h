#ifndef SUNXI_PNG_DEC_LINUX_H
#define SUNXI_PNG_DEC_LINUX_H


#ifdef __cplusplus
extern "C" {
#endif


#if defined(LV_USE_SUNXI_PNG) && defined(OS_LINUX)



int32_t png_decoder_init(void);
lv_draw_buf_t *png_decoder_dec(lv_image_decoder_dsc_t *dsc, uint8_t *data, uint32_t size);
void png_decoder_deinit(void);



#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

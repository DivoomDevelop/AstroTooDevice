#ifndef __DIVOOM_DECODE_WEBP_GIF_H__
#define __DIVOOM_DECODE_WEBP_GIF_H__

/*
 * DIVM-WebP 专用解码器：外层 DIVM 偏移表中的每个元素都是一个完整 WebP，
 * 每个 WebP 可以包含一帧或多帧，播放时展开为一条连续帧序列。
 */

/* 创建解码句柄，并返回所有内嵌 WebP 展开后的总帧数与画布尺寸。 */
void *divoom_decode_webp_gif_init(char *file_name, int *image_w,
    int *image_h, int *image_num);

/* 判断句柄是否由本模块创建，供 JPEG_GIF 公共接口做透明分发。 */
int divoom_decode_webp_gif_is_handle(void *handle);

/* 保留文件与索引信息，只关闭当前内层解码器并回到展开帧序列起点。 */
int divoom_decode_webp_gif_reset(void *handle);

/* 按展开后的连续帧序列跳过指定帧数，动画合成状态会同步推进。 */
void divoom_decode_webp_gif_pass_some_frame(void *handle, int frame_num);

/* 获取 BGRA 帧；malloc 版本由调用者释放，复用版本由解码句柄持有。 */
uint8 *divoom_decode_webp_gif_get_malloc_bgra_data(void *handle, int *speed,
    int *end_flag);
uint8 *divoom_decode_webp_gif_get_bgra_data(void *handle, int *speed,
    int *end_flag);

/* 获取 LVGL RGB565A8 平面格式帧；sys_buf 为空时由调用者释放返回值。 */
uint8 *divoom_decode_webp_gif_get_frame_rgb565a8_buffer(void *handle,
    int *speed, int *end_flag, uint8 *sys_buf);

/* 释放文件、当前内嵌 WebP 解码器和模块持有的帧缓存。 */
void divoom_decode_webp_gif_exit(void *handle);

#endif /* __DIVOOM_DECODE_WEBP_GIF_H__ */

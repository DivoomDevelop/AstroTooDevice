#ifndef __DIVOOM_DECODE_JPEG_GIF_H__
#define __DIVOOM_DECODE_JPEG_GIF_H__

/**
 * @brief 初始化JPEG/GIF解码器
 * @param file_name 文件名
 * @param image_w 输出参数：图像宽度
 * @param image_h 输出参数：图像高度
 * @param image_num 输出参数：图像帧数
 * @return 成功返回解码器句柄，失败返回NULL
 */
void *divoom_decode_jpeg_gif_init(char *file_name, int *image_w, int *image_h, int *image_num);
void *divoom_decode_jpeg_gif_init_rgb565a8(char *file_name, int *image_w, int *image_h, int *image_num);

/* 保留资源文件和索引信息，只把连续帧播放位置复位到开头。 */
int divoom_decode_jpeg_gif_reset(void *handle);

/**
 * @brief 跳过指定数量的帧
 * @param handle 解码器句柄
 * @param frame_num 要跳过的帧数
 */
void divoom_decode_jpeg_gif_pass_some_frame(void *handle, int frame_num);

/**
 * @brief 获取BGRA格式的图像数据（每次调用都会分配新内存）
 * @param handle 解码器句柄
 * @param speed 输出参数：帧速率
 * @param end_flag 输出参数：是否到达结尾
 * @return 返回BGRA格式的图像数据，调用者负责释放内存
 */
uint8 *divoom_decode_jpeg_gif_get_malloc_bgra_data(void *handle, int *speed, int *end_flag);
uint8 *divoom_decode_jpeg_gif_get_malloc_rgb565a8_data(void *handle, int *speed, int *end_flag);

/**
 * @brief 获取BGRA格式的图像数据（复用内存）
 * @param handle 解码器句柄
 * @param speed 输出参数：帧速率
 * @param end_flag 输出参数：是否到达结尾
 * @return 返回BGRA格式的图像数据，内存由解码器管理
 */
uint8 *divoom_decode_jpeg_gif_bgra_get_bgra_data(void *handle, int *speed, int *end_flag);
uint8 *divoom_decode_jpeg_gif_get_frame_rgb565a8_buffer(void *handle, int *speed, int *end_flag, uint8 *sys_buf);

/**
 * @brief 退出并清理解码器
 * @param handle 解码器句柄
 */
void divoom_decode_jpeg_gif_exit(void *handle);

#endif /* __DIVOOM_DECODE_JPEG_GIF_H__ */ 

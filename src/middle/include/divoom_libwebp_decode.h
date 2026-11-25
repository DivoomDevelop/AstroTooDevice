#ifndef _divoom_libwebp_decode_h_

#define _divoom_libwebp_decode_h_
#ifdef DIVOOM_SIMULATOR
typedef struct WebPAnimDecoder WebPAnimDecoder;
#else
#include <webp/decode.h>
#include <webp/encode.h>
#include "webp/demux.h"
#endif

typedef struct _divoom_libwebp_decode_ctrl
{
    int frame_id;
    int frame_num;
    int width;
    int high;
    int pre_frame_time;
    int file_len;
    char *file_name;
    uint8 *file_buf;
    void *image_handle;
    uint8 backend_type;
}divoom_libwebp_decode_ctrl, *Pdivoom_libwebp_decode_ctrl;

/*********************************************************************
  Function:        divoom_libwebp_decode_init
  Description:     创建WEBP的解码句柄
  Input:           file_name   图像文件名
  Return:          解码句柄
**********************************************************************/

void *divoom_libwebp_decode_init(char *file_name);

/*
 * 从大文件中的一个 WebP 区间创建流式解码句柄。压缩数据按固定小块读取，
 * 不会按 file_size 申请整文件缓存。
 */
void *divoom_libwebp_decode_init_file_region(char *file_name,
    uint32 file_offset, uint32 file_size);

uint8 *divoom_libwebp_decode_file_first_for_preview(char *file_name, int max_w, int max_h,
	int *out_w, int *out_h);
uint8 *divoom_libwebp_decode_file_first_for_preview_565a8(char *file_name,
    int max_w, int max_h, int *out_w, int *out_h);

/*
 * 不创建 WebPAnimDecoder，按原始尺寸解码静态 WebP 首帧。
 * 返回 1 表示静态图解码成功，0 表示动画 WebP，-1 表示静态图解码失败。
 */
int divoom_libwebp_decode_static_file_first_565a8(char *file_name,
    uint8 **out_buf, int *out_w, int *out_h);

/*********************************************************************
  Function:        divoom_libwebp_decode_get_frame_info
  Description:     获取动画的相关信息
  Input:           handle   解码句柄
                   width    图像宽度
                   high     图像高度
  Return:          返回图像帧数
**********************************************************************/

int divoom_libwebp_decode_get_frame_info(void *handle, int *width, int *high);

/*********************************************************************
  Function:        divoom_libwebp_decode_get_frame_buffer
  Description:     得到帧显示缓冲区，如果是动画可以循环调用此函数，它将自动循环，
                   注意这个返回的BUFFER，要外面进行释放，否则会有内存泄露
  Input:           handle   解码句柄
                   frame_time  这帧持续时间
  Return:          返回图像帧缓冲区
**********************************************************************/

uint8 *divoom_libwebp_decode_get_frame_buffer(void *handle,  int *frame_time, int *end_flag, uint8 *sys_buf);
uint8 *divoom_libwebp_decode_get_frame_rgb565a8_buffer(void *handle, int *frame_time, int *end_flag, uint8 *sys_buf);
uint8 *divoom_libwebp_decode_get_frame_rgb565_buffer(void *handle, int *frame_time, int *end_flag, uint8 *sys_buf);


/*********************************************************************
  Function:        divoom_libwebp_decode_exit
  Description:     释放解码句柄，释放所有资源
  Input:           handle   解码句柄
  Return:          none
**********************************************************************/

void divoom_libwebp_decode_exit(void *handle);

/*********************************************************************
  Function:        divoom_libwebp_decode_get_rgb_buffer
  Description:     得到帧显示缓冲区，如果是动画可以循环调用此函数，它将自动循环，
                   
  Input:           handle   解码句柄
                   frame_time  这帧持续时间
                   end_flag
                   rgb_buf
  Return:          成功返回1， 否则返回0
**********************************************************************/

int divoom_libwebp_decode_get_rgb_buffer(void *handle,  int *frame_time, int *end_flag, uint8 *rgb_buf);

/*********************************************************************
  Function:        divoom_libwebp_encode_image
  Description:     将图像缓冲区编码成webp格式的图像
  Input:           file_name   图像文件名
                   image_buf   图像缓冲区
                   image_width
                   image_high
  Return:          解码句柄
**********************************************************************/

int divoom_libwebp_encode_image(char *file_name, uint8 *image_buf, int image_width, int image_high);

/*********************************************************************
  Function:        divoom_libwebp_decode_image_buf
  Description:     对缓冲区数据进行解码，解码出一帧数据
  Input:           raw_buf   图像缓冲区
                   raw_size
  Return:          输出的RGB缓冲区
**********************************************************************/

void divoom_libwebp_decode_image_buf(uint8 *raw_buf, int raw_size, uint8 *rgb_buf, int image_size);
void divoom_libwebp_decode_image_buf_565(uint8 *raw_buf, int raw_size, uint8 *rgb_buf, int image_size);
void divoom_libwebp_decode_image_buf_565a8(uint8 *raw_buf, int raw_size, uint8 *rgb_buf, int image_size);

/*********************************************************************
  Function:        divoom_libwebp_decode_image_buf_width_info
  Description:     对缓冲区数据进行解码，解码出一帧数据
  Input:           raw_buf   图像缓冲区
                   raw_size
                   image_width      输出图像宽度
                   image_high       输出图像高度
  Return:          输出的RGB缓冲区
**********************************************************************/

uint8 *divoom_libwebp_decode_image_buf_width_info(uint8 *raw_buf, int raw_size, int *image_width, int *image_high);

/*********************************************************************
  Function:        divoom_libwebp_decode_pass_some_frame
  Description:     跳过一些帧
  Input:           handle   解码句柄
                   frame_num  跳过的帧数
  Return:          none
**********************************************************************/

void divoom_libwebp_decode_pass_some_frame(void *handle,  int frame_num);
/* 定位下一次要读取的帧，保留 WebP 前置帧的合成和时间戳。 */
int divoom_libwebp_decode_seek_frame(void *handle, int frame);

/*********************************************************************
  Function:        divoom_libwebp_decode_get_image_rgb_buf
  Description:     从文件中解码出一帧RGB数据出来
  Input:           file_name   图像文件名
  Return:          RGB缓冲区
**********************************************************************/

uint8 *divoom_libwebp_decode_get_image_rgb_buf(char *file_name, int *width, int *high);

#endif

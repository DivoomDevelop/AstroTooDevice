#ifndef _divoom_rk_mpp_h_
#define _divoom_rk_mpp_h_
#include "divoom_type.h"
#include "lvgl.h"
/*********************************************************************
  Function:        divoom_rk_mpp_get_stretch_image_width_high
  Description:     对图像进行拉伸，自动计算图像裁剪的后的大小
  Input:           src_w
                   src_h
                   dest_w
                   dest_h
                   mid_w
                   mid_h
  Return:          0为不需要裁剪； 1：需要裁剪
**********************************************************************/

int divoom_rk_mpp_get_stretch_image_width_high(int src_w, int src_h, int dest_w, int dest_h, int* mid_w, int* mid_h);

/*********************************************************************
  Function:        divoom_rk_mpp_stretch_image
  Description:     对图像进行拉伸
  Input:           src_rgb
                   src_w
                   src_h
                   dest_rgb
                   dest_w
                   dest_h
  Return:          0为成功，其它失败原因
**********************************************************************/


int divoom_rk_mpp_convert_color_type(int src_type);
int divoom_rk_mpp_stretch_image(uint8 *src_rgb, int src_w, int src_h, int src_type,uint8 *dest_rgb, int dest_w, int dest_h);
int divoom_rk_mpp_stretch_image_lvfmt(uint8 *src_rgb, int src_w, int src_h,
                                      int src_type, uint8 *dest_rgb,
                                      int dest_w, int dest_h,
                                      lv_color_format_t dest_cf);

/** BGRA 最近邻缩放（纯色块/小预览用，不重依赖 G2D） */
void divoom_rk_mpp_bgra_scale_nearest_cpu(uint8 *src, int sw, int sh, uint8 *dst, int dw, int dh);
/**
 * 当目标为小缩略时用 CPU 缩放，绕开 G2D 横纹；源可为全尺寸解码图。
 * 返回 1 已写入 dst；0 时请改调 divoom_rk_mpp_stretch_image。
 */
int divoom_rk_mpp_try_preview_bgra_scale_cpu(uint8 *src, int sw, int sh, uint8 *dst, int dw, int dh);
/*********************************************************************
  Function:        divoom_rk_mpp_stretch_crop_image
  Description:     对图像进行缩小,保持图像的比例,如果图像比原来小，就不放大，直接层中显示
  Input:           src_rgb
                   src_w
                   src_h
                   dest_rgb
                   dest_w
                   dest_h
  Return:          0为成功，其它失败原因
**********************************************************************/


int divoom_rk_mpp_stretch_crop_image(uint8 *src_rgb, int src_w, int src_h, uint8 *dest_rgb, int dest_w, int dest_h);

/*********************************************************************
  Function:        divoom_rk_mpp_save_jpeg_file
  Description:     将图像压缩成相关大小再保存到文件中
  Input:           file_name        //要保存的文件名
                   src_rgb          //原始图像的RGB BUF
                   src_w            //
                   src_h
                   dest_w
                   dest_h
  Return:          0为失败，其它成功
**********************************************************************/


int divoom_rk_mpp_save_jpeg_file(char *file_name, uint8 *src_rgb, int src_w, int src_h, int dest_w, int dest_h);

/*********************************************************************
  Function:        divoom_rk_mpp_get_jpeg_info
  Description:     从文件中读取图像数据；JPG/JPEG输出RGB565，其它格式保持8888缓冲区
  Input:           file_name        //要保存的文件名
                   image_w            //
                   image_h
  Return:          返回图像缓冲区
**********************************************************************/


uint8 *divoom_rk_mpp_get_jpeg_info(char *file_name, int *image_w, int *image_h);
    
/*********************************************************************
  Function:        divoom_rk_mpp_decode_jpeg_file
  Description:     从文件中读取JPEG文件的图像数据，输出RGB565缓冲区
  Input:           file_name        //要保存的文件名
                   image_w            //
                   image_h
  Return:          返回RGB565缓冲区
**********************************************************************/


uint8 *divoom_rk_mpp_decode_jpeg_file(char *file_name, int *image_w, int *image_h);

/*********************************************************************
  Function:        divoom_rk_mpp_get_pic_width_info
  Description:     从文件中读取JPEG文件的图像数据，得到宽高信息
  Input:           file_name        //要保存的文件名
                   image_w            //
                   image_h
  Return:          none
**********************************************************************/


void divoom_rk_mpp_get_pic_width_info(char *file_name, int *image_w, int *image_h);

uint8 *divoom_rk_mpp_decode_jpeg_rgb565_info(char *buf, int buf_len, int *image_w, int *image_h);
uint8 *divoom_rk_mpp_decode_jpeg_rgb565_to_buf_info(char *buf, int buf_len,
    int *image_w, int *image_h, uint8 *out_buf, uint32 out_size);
uint8 *divoom_rk_mpp_decode_jpeg_rgb565_to_buf(char *buf, int buf_len,
    int *image_w, int *image_h, uint8 *out_buf, uint32 out_size);

/*********************************************************************
  Function:        divoom_encode_rgb_to_jpeg
  Description:     将RGB数据保存在JPG文件中，rgb_buffer必须是RGB格式
  Input:           file_name        //要保存的文件名
                   rgb_buffer          //原始图像的RGB BUF
                   width            //
                   height
  Return:          none
**********************************************************************/


void divoom_encode_rgb_to_jpeg(char *file_name, unsigned char *rgb_buffer, int width, int height);


/*********************************************************************
  Function:        divoom_rk_mpp_rotate_image
  Description:     对图像进行旋转,如果是90 或270度的话，src_w, src_h会交换
  Input:           src_rgb
                   src_w
                   src_h
                   dest_rgb
                   age_value    //旋转角度
  Return:          0为成功，其它失败原因
**********************************************************************/


int divoom_rk_mpp_rotate_image(uint8 *src_rgb, int *src_w, int *src_h, int src_type,uint8 *dest_rgb, int age_value);

/*********************************************************************
  Function:        divoom_rk_mpp_mirror_image
  Description:     对图像进行镜像
  Input:           src_rgb
                   src_w
                   src_h
                   dest_rgb
  Return:          0为成功，其它失败原因
**********************************************************************/


int divoom_rk_mpp_mirror_image(uint8 *src_rgb, int src_w, int src_h, int src_type,uint8 *dest_rgb);

/*********************************************************************
  Function:        divoom_rk_mpp_stretch_image
  Description:     对图像进行拉伸， 要保持原始图像的比例
  Input:           src_rgb
                   src_w
                   src_h
                   dest_rgb
                   dest_w
                   dest_h
                   fill_flag    0:以dest_w为准， 1：以dest_h
  Return:          0为成功，其它失败原因
**********************************************************************/


int divoom_rk_mpp_get_disp_image_info(uint8 *src_rgb, int src_w, int src_h, int src_type,uint8 *dest_rgb, int *dest_w, int *dest_h, int dest_flag, int fill_flag);
/*********************************************************************
  Function:        divoom_rk_mpp_crop_image
  Description:     对图像进行裁剪
  Input:           src_rgb
                   src_w
                   src_h
                   src_type
                   dest_rgb
                   start_x    
                   start_y
                   size_x
                   size_y
  Return:          0为成功，其它失败原因
**********************************************************************/


int divoom_rk_mpp_crop_image(uint8 *src_rgb, int src_w, int src_h, int src_type,uint8 *dest_rgb, 
                                int start_x,int start_y,int size_x,int size_y);
/*********************************************************************
  Function:        divoom_rk_mpp_stretch_image
  Description:     对图像进行拉伸
  Input:           src_rgb
                   src_w
                   src_h
                   dest_rgb
                   dest_w
                   dest_h
  Return:          0为成功，其它失败原因
**********************************************************************/


int divoom_rk_mpp_stretch_image_direct(uint8 *src_rgb, int src_w, int src_h, int src_type,uint8 *dest_rgb, int dest_w, int dest_h, int dest_type);
/*********************************************************************
  Function:        divoom_rk_mpp_stretch_image_data
  Description:     对原始图像进行拉伸，按特定的比例进行输出，如果比例不合，将会拉伸一下
  Input:           src_rgb
                   src_w
                   src_h
                   dest_rgb
                   dest_w
                   dest_h
  Return:          0为成功，其它失败原因
**********************************************************************/


int divoom_rk_mpp_stretch_raw_picture_data(uint8 *src_rgb, int src_w, int src_h,uint8 *dest_rgb, int dest_w, int dest_h);

/*********************************************************************
  Function:        divoom_rk_mpp_crop_image
  Description:     对图像进行裁剪
  Input:           src_rgb
                   src_w
                   src_h
                   dest_rgb
                   dest_w
                   dest_h
                   
  Return:          0为成功，其它失败原因
**********************************************************************/


void divoom_rk_mpp_crop_image_to_size(uint8 *src_rgb, int src_w, int src_h, uint8 *dest_rgb, int dest_w,int dest_h);

/*********************************************************************
  Function:        divoom_rk_mpp_get_png_info
  Description:     读图像数据
  Input:           filename
                   image_w
                   image_h
                   
  Return:          图像缓冲区
**********************************************************************/

uint8 *divoom_rk_mpp_get_png_info(const char *filename, int *image_w, int *image_h) ;
uint8 *divoom_rk_mpp_get_png_lvfmt_info(const char *filename, int *image_w,
                                        int *image_h, lv_color_format_t cf);
uint8 *divoom_rk_mpp_get_png_lvfmt_to_buf_info(const uint8 *buf, int buf_len,
                                               int *image_w, int *image_h,
                                               lv_color_format_t cf,
                                               uint8 *out_buf,
                                               uint32 out_size);

/*********************************************************************
  Function:        divoom_rk_mpp_crop_stretch_image_direct
  Description:     对图像进行拉伸
  Input:           src_rgb
                   src_w
                   src_h
                   dest_rgb
                   dest_w
                   dest_h
  Return:          0为成功，其它失败原因
**********************************************************************/


int divoom_rk_mpp_crop_stretch_image_direct(int src_type, uint8 *src_rgb, int src_w, int src_h, int src_start_x, int src_start_y,
                                            int src_size_x, int src_size_y,uint8 *dest_rgb, int dest_w, int dest_h, 
                                            int dest_start_x, int dest_start_y,int dest_size_x, int dest_size_y);
int divoom_rk_mpp_crop_stretch_image_lvfmt(int src_type, uint8 *src_rgb,
                                           int src_w, int src_h,
                                           int src_start_x, int src_start_y,
                                           int src_size_x, int src_size_y,
                                           uint8 *dest_rgb, int dest_w,
                                           int dest_h, int dest_start_x,
                                           int dest_start_y, int dest_size_x,
                                           int dest_size_y,
                                           lv_color_format_t dest_cf);

/*********************************************************************
  Function:        divoom_rk_mpp_copy_stretch_image_direct
  Description:     对图像进行拷贝部分图像
  Input:           src_rgb
                   src_w
                   src_h
                   dest_rgb
                   dest_w
                   dest_h
                   dest_start_x
  Return:          0为成功，其它失败原因
**********************************************************************/


int divoom_rk_mpp_copy_stretch_image_direct(uint8 *src_rgb, int src_w, int src_h, 
                                            uint8 *dest_rgb, int dest_w, int dest_h, int dest_start_x);
/*********************************************************************
  Function:        divoom_decode_bmp_file
  Description:     从文件中读取BMP文件的图像数据，输出bgra缓冲区
  Input:           file_name        //要保存的文件名
                   image_w            //
                   image_h
                   image_data
  Return:          返回bgra缓冲区
**********************************************************************/

uint8* divoom_decode_bmp_file(const char* filename, int *image_w, int *image_h, uint8 *image_data);
uint8* divoom_decode_bmp_file_lvfmt(const char* filename, int *image_w,
                                    int *image_h, uint8 *image_data,
                                    lv_color_format_t cf);

#endif



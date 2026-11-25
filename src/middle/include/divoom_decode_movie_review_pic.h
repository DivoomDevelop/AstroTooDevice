#ifndef _divoom_decode_movie_review_pic_h_
#define _divoom_decode_movie_review_pic_h_

#include "divoom_type.h"
#include "lvgl.h"

/*********************************************************************
  Function:        divoom_decode_movie_review_pic_create
  Description:     生成视频的预览图
  Input:           movie_name      路径要带file://+本地路径
                   review_name     本地路径
                   image_w         控件宽度
                   image_h         控件高度
                   rotate_flag     旋转
                   mirror_flag     镜像
  Return:          none
**********************************************************************/

void divoom_decode_movie_review_pic_create(char *movie_name, char *review_name, int image_w, int image_h, int rotate_flag, int mirror_flag);

/*********************************************************************
  Function:        divoom_decode_movie_review_pic_create_delay_run
  Description:     生成视频的预览图的定时回调
                   para
  Return:          none
**********************************************************************/
int divoom_decode_movie_review_pic_create_delay_run(void *para);

/*********************************************************************
  Function:        divoom_decode_movie_review_pic_get_buf
  Description:     生成视频的预览图的缓冲区
  Input:           movie_name      路径要带file://+本地路径
                   review_name     本地路径
                   image_w         控件宽度
                   image_h         控件高度
                   rotate_flag     旋转
                   mirror_flag     镜像
  Return:          none
**********************************************************************/

void divoom_decode_movie_review_pic_get_buf(char *movie_name, uint8 *image_data, int image_w, int image_h, int rotate_flag, int mirror_flag);

void divoom_decode_movie_review_pic_get_lvfmt_buf(char *movie_name,
                                                  uint8 *image_data,
                                                  int image_w,
                                                  int image_h,
                                                  int rotate_flag,
                                                  int mirror_flag,
                                                  lv_color_format_t cf);

/*********************************************************************
  Function:        divoom_decode_movie_get_movie_first_frame_buf
  Description:     生成视频的预览图的第一帧图像缓冲区
  Input:           movie_name      路径要带file://+本地路径
                   image_w         控件宽度
                   image_h         控件高度
  Return:          第一帧缓冲区
**********************************************************************/

uint8 *divoom_decode_movie_get_movie_first_frame_buf(char *movie_name, int *image_w, int *image_h);

#endif


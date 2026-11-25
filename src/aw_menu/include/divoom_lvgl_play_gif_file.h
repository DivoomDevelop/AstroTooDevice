#ifndef _divoom_lvgl_play_gif_file_h_
#define _divoom_lvgl_play_gif_file_h_
/*********************************************************
  * FUNCTION		: divoom_lvgl_play_gif_file_destory_handle
  * 		        : 释放相关资源
  * IUPUT			handle
  * OUTPUT			none
  *********************************************************/

/* Caller holds LVGL. Stop callbacks before releasing LVGL to join/free. */
void divoom_lvgl_play_gif_file_retire_handle(void *handle);
void divoom_lvgl_play_gif_file_destory_handle(void *handle, int lock_flag);
/*********************************************************
  * FUNCTION		: divoom_lvgl_play_gif_file_create_handle
  * 		        : 创建一个文件播放，设计是按动画走的，目前只支持jpg,gif,webp,png
  * IUPUT			file_name
                    image_obj
                    image_data
                    width
                    high
  * OUTPUT			: handle 
  *********************************************************/

void *divoom_lvgl_play_gif_file_create_handle(char *file_name, void *image_obj, lv_img_dsc_t *image_data, int width, int high, int lock_flag, int single_flag);

/*********************************************************
  * FUNCTION		: divoom_lvgl_play_gif_file_start_play
  * 		        : 开始播放
  * IUPUT			handle
  * OUTPUT			none
  *********************************************************/

void divoom_lvgl_play_gif_file_start_play(void *handle);

/*********************************************************
  * FUNCTION		: divoom_lvgl_play_gif_file_create_handle_reuse_image_buf
  * 		        : GIF 动画；解码直写 image_data->data，不重复 malloc 解码缓冲
  *                  仅用于调用方已分配 lv_img_dsc.data 的小图场景（如 OOBE）
  * IUPUT			同 create_handle；image_data->data 须非空
  * OUTPUT			: handle
  *********************************************************/

void *divoom_lvgl_play_gif_file_create_handle_reuse_image_buf(char *file_name,
    void *image_obj, lv_img_dsc_t *image_data, int width, int high,
    int lock_flag, int single_flag);

/*********************************************************
  * FUNCTION		: divoom_lvgl_play_gif_file_destory_handle_reuse_image_buf
  * 		        : 释放 reuse 句柄；不释放 image_data->data
  * IUPUT			handle
  * OUTPUT			none
  *********************************************************/

void divoom_lvgl_play_gif_file_destory_handle_reuse_image_buf(void *handle, int lock_flag);

#endif



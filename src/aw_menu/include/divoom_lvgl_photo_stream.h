#ifndef _divoom_lvgl_photo_stream_h_
#define _divoom_lvgl_photo_stream_h_

void *divoom_lvgl_photo_stream_init(void *user_data, int menu_id);
void divoom_lvgl_photo_stream_disp(void *handle, void *active);
void divoom_lvgl_photo_stream_pause(void *handle, int exit_flag);
void divoom_lvgl_photo_stream_eixt(void *handle);

void *divoom_lvgl_photo_stream_get_lvgl_obj(void *handle);

void divoom_lvgl_photo_stream_update_time(void *handle, int play_time);

void divoom_lvgl_photo_stream_play_next(void *handle, int dir);

void divoom_lvgl_photo_stream_get_pic_info(void *handle, Pdivoom_lvgl_photo_view_para_info pic_info);
void divoom_lvgl_photo_stream_remove_notice(void *handle, uint32 *pic_list, int pic_num);

#endif



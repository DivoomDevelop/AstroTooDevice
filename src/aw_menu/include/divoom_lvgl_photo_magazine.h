#ifndef _divoom_lvgl_photo_magazine_h_
#define _divoom_lvgl_photo_magazine_h_

void *divoom_lvgl_photo_magazine_init(void *user_data, int menu_id);
void divoom_lvgl_photo_magazine_disp(void *handle, void *active_obj);
void divoom_lvgl_photo_magazine_pause(void *handle);
void divoom_lvgl_photo_magazine_exit(void *handle);

void *divoom_lvgl_photo_magazine_get_lvgl_obj(void *handle);
void divoom_lvgl_photo_magazine_update_time(void *handle, int play_time);

void divoom_lvgl_photo_magazine_play_next(void *handle, int dir);
void divoom_lvgl_photo_magazine_get_pic_info(void *handle, Pdivoom_lvgl_photo_view_para_info pic_info);

#endif



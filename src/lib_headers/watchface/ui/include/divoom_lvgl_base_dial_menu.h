#ifndef _divoom_lvgl_base_dial_menu_h_
#define _divoom_lvgl_base_dial_menu_h_

void *divoom_lvgl_base_dial_menu_init(void *user_data, int menu_id);
void divoom_lvgl_base_dial_menu_disp(void *handle, void *active);
void divoom_lvgl_base_dial_menu_pause(void *handle);
void divoom_lvgl_base_dial_menu_exit(void *handle);
void divoom_lvgl_base_dial_menu_update_clock_info(void *handle, void *data, int lock_flag);
int divoom_lvgl_base_dial_menu_is_play_photo_slide(void *handle);
int divoom_lvgl_base_dial_menu_is_empty_photo_album(void *handle);
void divoom_lvgl_base_dial_menu_change_photo_album(void *handle);

#endif


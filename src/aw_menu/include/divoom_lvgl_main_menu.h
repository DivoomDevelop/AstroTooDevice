#ifndef _divoom_lvgl_main_menu_h_
#define _divoom_lvgl_main_menu_h_

void *divoom_lvgl_main_menu_init(void *user_data, int menu_id);

void divoom_lvgl_main_menu_disp(void *handle, void *active_obj);

void divoom_lvgl_main_menu_exit(void *handle);

#endif


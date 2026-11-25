#ifndef _divoom_lvgl_demo_dial_menu_h_
#define _divoom_lvgl_demo_dial_menu_h_

void *divoom_lvgl_demo_dial_menu_init(void *user_data, int menu_id);
void divoom_lvgl_demo_dial_menu_disp(void *handle, void *active);
void divoom_lvgl_demo_dial_menu_pause(void *handle);
void divoom_lvgl_demo_dial_menu_exit(void *handle);

#endif


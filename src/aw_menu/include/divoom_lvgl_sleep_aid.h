#ifndef _divoom_lvgl_sleep_aid_h_
#define _divoom_lvgl_sleep_aid_h_

void *divoom_lvgl_sleep_aid_init(void *user_data, int menu_id);
void divoom_lvgl_sleep_aid_disp(void *handle, void *active);
void divoom_lvgl_sleep_aid_pause(void *handle);
void divoom_lvgl_sleep_aid_exit(void *handle);

#endif

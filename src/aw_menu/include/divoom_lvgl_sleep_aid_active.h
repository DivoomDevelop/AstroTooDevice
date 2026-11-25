#ifndef _divoom_lvgl_sleep_aid_active_h_
#define _divoom_lvgl_sleep_aid_active_h_

void *divoom_lvgl_sleep_aid_active_init(void *user_data, int menu_id);
void divoom_lvgl_sleep_aid_active_disp(void *handle, void *active_obj);
void divoom_lvgl_sleep_aid_active_pause(void *handle);
void divoom_lvgl_sleep_aid_active_exit(void *handle);

#endif

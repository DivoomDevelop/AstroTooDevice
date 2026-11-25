#ifndef _divoom_lvgl_system_setting_h_
#define  _divoom_lvgl_system_setting_h_

void *divoom_lvgl_system_setting_init(void *user_data, int menu_id);

void divoom_lvgl_system_setting_disp(void *handle, void *active_obj);

void divoom_lvgl_system_setting_pause(void *handle);
void divoom_lvgl_system_setting_exit(void *handle);

#endif


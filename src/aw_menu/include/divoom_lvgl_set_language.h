#ifndef _divoom_lvgl_set_language_h_
#define _divoom_lvgl_set_language_h_
#include "divoom_menu_control.h"

void *divoom_lvgl_setting_set_language_init(void *user_data, int menu_id);
void divoom_lvgl_setting_set_language_disp(void *handle, void *active_obj);
void divoom_lvgl_setting_set_language_exit(void *handle);

const divoom_select_language_menu_item *divoom_lvgl_set_language_get_list(void);
int divoom_lvgl_set_language_get_list_count(void);

#endif


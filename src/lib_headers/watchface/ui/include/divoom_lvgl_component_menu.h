#ifndef _divoom_lvgl_component_menu_
#define _divoom_lvgl_component_menu_ 

void *divoom_lvgl_component_menu_init(void *user_data, int menu_id);

void divoom_lvgl_component_menu_disp(void *handle, void *active_obj);

void divoom_lvgl_component_menu_exit(void *handle);

#endif

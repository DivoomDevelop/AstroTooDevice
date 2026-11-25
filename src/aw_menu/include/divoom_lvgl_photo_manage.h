#ifndef _divoom_lvgl_photo_manage_h_
#define _divoom_lvgl_photo_manage_h_

void *divoom_lvgl_photo_manage_init(void *user_data, int menu_id);
void divoom_lvgl_photo_manage_disp(void *handle, void *active_obj);
void divoom_lvgl_photo_manage_exit(void *handle);
void divoom_lvgl_photo_manage_pause(void *handle);
void divoom_lvgl_photo_manage_request_esc_return(void *handle);
/* 主线程 ESC：1=已触发 return_pre，0=留给 LVGL（操作菜单/确认框/多选） */
int divoom_lvgl_photo_manage_try_main_esc_return(void *handle);

#endif


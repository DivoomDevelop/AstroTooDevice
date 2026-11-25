#ifndef _divoom_lvgl_file_transfer_h_
#define _divoom_lvgl_file_transfer_h_

void *divoom_lvgl_file_transfer_init(void *user_data, int menu_id);
void divoom_lvgl_file_transfer_disp(void *handle, void *active_obj);
void divoom_lvgl_file_transfer_pause(void *handle);
void divoom_lvgl_file_transfer_exit(void *handle);

#endif

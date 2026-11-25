#ifndef _divoom_white_noise_menu_h_
#define _divoom_white_noise_menu_h_

void *divoom_white_noise_menu_init(void *user_data, int menu_id);
void divoom_white_noise_menu_disp(void *handle, void *active);
void divoom_white_noise_menu_clean(void *handle);
void *divoom_white_noise_menu_get_active(void *handle);
void *divoom_white_noise_menu_set_return_arrow_obj(void *obj);
void divoom_white_noise_menu_app_ctrl(uint8 ctrl, void *para);

#endif


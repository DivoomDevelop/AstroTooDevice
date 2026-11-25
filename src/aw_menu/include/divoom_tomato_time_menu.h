#ifndef _divoom_tomato_time_menu_
#define _divoom_tomato_time_menu_
#define DIVOOM_TOMATO_MENU_BACK_GROUD_COLOR   0XE4E4E4
#define DIVOOM_TOMATO_MENU_TEXT_COLOR 0X0
#define DIVOOM_TOMATO_MENU_BORDER_COLOR 0XC2C2C2

void divoom_tomato_time_menu_add_disp_gif(int model_id, int image_id);
void *divoom_tomato_time_menu_init(void *user_data, int menu_id);
void divoom_tomato_time_menu_anmi(void *handle);
void divoom_tomato_time_menu_disp(void *handle, void *active);
void divoom_tomato_time_menu_clean(void *handle);
Pdivoom_tomato_time_info divoom_tomato_time_menu_get_tomato_info(int index);
void *divoom_tomato_time_menu_set_return_arrow_obj(void *obj);
void *divoom_tomato_time_menu_set_label_time_obj(void *obj, int x, int y, int menu_id);
void divoom_tomato_time_menu_display_time_label(void *obj);
Pdivoom_tomato_time_info divoom_tomato_time_menu_get_tomato_info(int index);
void divoom_tomato_time_menu_control(int flag);
void divoom_tomato_time_menu_exit_disp(void);

#endif


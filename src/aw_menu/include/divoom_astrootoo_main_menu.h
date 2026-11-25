#ifndef _divoom_astrootoo_main_menu_h_
#define _divoom_astrootoo_main_menu_h_

void *divoom_astrootoo_main_menu_init(void *user_data, int menu_id);
void divoom_astrootoo_main_menu_disp(void *handle, void *active_obj);
void divoom_astrootoo_main_menu_clean(void *handle);
void divoom_astrootoo_main_menu_pause(void *handle);
int divoom_astrootoo_main_menu_whitebox_get_focus_index(void);

#endif

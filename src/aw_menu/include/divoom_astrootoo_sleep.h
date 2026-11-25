#ifndef _divoom_astrootoo_sleep_h_
#define _divoom_astrootoo_sleep_h_

void *divoom_astrootoo_sleep_init(void *user_data, int menu_id);
void divoom_astrootoo_sleep_disp(void *handle, void *active_obj);
void divoom_astrootoo_sleep_clean(void *handle);
void divoom_astrootoo_sleep_pause(void *handle);

int divoom_astrootoo_sleep_whitebox_get_active_tab(void);
int divoom_astrootoo_sleep_whitebox_get_focus_pane(void);
int divoom_astrootoo_sleep_whitebox_get_sub_focus(void);
int divoom_astrootoo_sleep_whitebox_get_sub_count(void);
int divoom_astrootoo_sleep_whitebox_get_sub_kind(void);

/** VOL 键改了 before/after_vol 后：设置页音量滑条跟新，并落盘。 */
void divoom_astrootoo_sleep_sync_hw_volume_sliders(void);

#endif

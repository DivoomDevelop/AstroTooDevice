#ifndef _divoom_astrootoo_alarm_h_
#define _divoom_astrootoo_alarm_h_

void *divoom_astrootoo_alarm_init(void *user_data, int menu_id);
void divoom_astrootoo_alarm_disp(void *handle, void *active_obj);
void divoom_astrootoo_alarm_clean(void *handle);
void divoom_astrootoo_alarm_pause(void *handle);

int divoom_astrootoo_alarm_whitebox_get_mode(void);
int divoom_astrootoo_alarm_whitebox_get_bell_phase(void);
int divoom_astrootoo_alarm_whitebox_get_bell_cat_focus(void);
int divoom_astrootoo_alarm_whitebox_get_edit_focus(void);
int divoom_astrootoo_alarm_whitebox_get_rec_focus(void);
int divoom_astrootoo_alarm_whitebox_get_rec_capture_active(void);

#endif

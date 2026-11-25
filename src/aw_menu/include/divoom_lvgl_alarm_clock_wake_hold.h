#ifndef _divoom_lvgl_alarm_clock_wake_hold_h_
#define _divoom_lvgl_alarm_clock_wake_hold_h_

#ifdef DIVOOM_TIME_FRAME_SPEAKER
void *divoom_lvgl_alarm_clock_wake_hold_init(void *user_data, int menu_id);
void divoom_lvgl_alarm_clock_wake_hold_disp(void *handle, void *active_obj);
void divoom_lvgl_alarm_clock_wake_hold_pause(void *handle);
void divoom_lvgl_alarm_clock_wake_hold_exit(void *handle);
#endif

#endif

#ifndef _divoom_disp_set_alarm_h_
#define _divoom_disp_set_alarm_h_
enum
{
    DIVOOM_DISP_SET_ALARM_UP,
    DIVOOM_DISP_SET_ALARM_DOWN,
    DIVOOM_DISP_SET_ALARM_LEFT,
    DIVOOM_DISP_SET_ALARM_RIGHT,
    DIVOOM_DISP_SET_ALARM_ENTER,
    DIVOOM_DISP_SET_ALARM_CANCEL,
};
void divoom_disp_set_alarm_key_handle(uint8 command);
void divoom_disp_set_alarm_init(void);
void divoom_disp_set_alarm_close(void);

#endif


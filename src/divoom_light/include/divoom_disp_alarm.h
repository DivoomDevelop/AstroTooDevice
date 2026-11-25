#ifndef _divoom_disp_alarm_h_
#define _divoom_disp_alarm_h_
#include "divoom_disp_com.h"
void divoom_disp_alarm_init(Pdivoom_disp_ctrl_alarm_info alarm_info);
void divoom_disp_alarm_update_disp(Pdivoom_disp_ctrl_alarm_info alarm_info);
void divoom_disp_alarm_play(uint8 status);
void divoom_disp_alarm_close(void);
uint8 divoom_disp_alarm_check_user_define_pic(uint8 mode, uint8 index);

#endif



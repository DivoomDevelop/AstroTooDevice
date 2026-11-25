#ifndef _divoom_disp_voice_ctrl_h_
#define _divoom_disp_voice_ctrl_h_

#define DIVOOM_VOICE_CTRL_START_TIME 3
void divoom_disp_voice_ctrl_init(T_BOOL flag);
T_BOOL divoom_disp_voice_ctrl_check_run(void);
void divoom_disp_voice_ctrl_close(void);
void divoom_disp_voice_ctrl_reset(void);
void divoom_disp_voice_ctrl_cancel(void);
T_BOOL divoom_disp_voice_ctrl_get_working(void);

#endif



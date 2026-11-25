#ifndef _divoom_show_mode_ctrl_h_
#define _divoom_show_mode_ctrl_h_

#define DIVOOM_SHOW_MODE_CTRL_IDLE_MS     (10 * 60 * 1000)
#define DIVOOM_SHOW_MODE_CTRL_TF_IDLE_MS  (5 * 60 * 1000)
#define DIVOOM_SHOW_MODE_CTRL_DISC_SEC    60
#define DIVOOM_SHOW_MODE_CTRL_TF_VOL      5
#define DIVOOM_SHOW_MODE_CTRL_ESC_MAX     5
#define DIVOOM_SHOW_MODE_CTRL_ESC_WAIT_MS 1200

void divoom_show_mode_ctrl_notice(void);
int divoom_show_mode_ctrl_idle_ready(void);
void divoom_show_mode_ctrl_tick(void);
int divoom_show_mode_ctrl_try_return_dial(void);

#endif

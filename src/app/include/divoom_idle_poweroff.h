#ifndef _divoom_idle_poweroff_h_
#define _divoom_idle_poweroff_h_

#define DIVOOM_IDLE_POWEROFF_30MIN_SEC  1800

#define DIVOOM_IDLE_POWEROFF_OPTION_CNT 6

extern const int divoom_idle_poweroff_secs[DIVOOM_IDLE_POWEROFF_OPTION_CNT];

void divoom_idle_poweroff_reset(void);
void divoom_idle_poweroff_tick(void);

#endif

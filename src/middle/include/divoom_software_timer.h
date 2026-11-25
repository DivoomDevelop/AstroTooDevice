#ifndef _divoom_software_timer_h_
#define _divoom_software_timer_h_

void divoom_software_timer_init(void);
typedef int DIVOOM_SOFTWARE_TIMER_CALLBACK(void *para);

void *divoom_software_timer_create_timer(int run_time, DIVOOM_SOFTWARE_TIMER_CALLBACK pcallback,void *para);
void divoom_software_timer_delete_timer(void *handle);

#ifdef DIVOOM_SIMULATOR
void divoom_software_timer_poll(void);
#endif

#endif

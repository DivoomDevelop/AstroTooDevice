#ifndef _divoom_light_plasma_h_
#define _divoom_light_plasma_h_


uint32 divoom_light_plasma_init(void);
void divoom_light_plasma_close(void);
void divoom_light_plasma_main(void);
uint32 divoom_light_plasma_get_refresh_time(void);
void divoom_light_plasma_set_refresh_time(void);
void divoom_light_plasma_get_taskid(uint32 id);

#endif







#ifndef _divoom_disp_base_clock_h_
#define _divoom_disp_base_clock_h_
void divoom_disp_base_clock_init(void *data, int menu_id);
void divoom_disp_base_clock_update(void *data, int flag);
void divoom_disp_base_clock_close(int flag); //1外部 0内部
void divoom_disp_base_clock_pause(void);
void divoom_disp_base_clock_resume(void);
/* The outer page owns intro playback before allocating lyric/photo resources. */
void divoom_disp_base_clock_set_outer_intro_owner(int enabled);

#endif


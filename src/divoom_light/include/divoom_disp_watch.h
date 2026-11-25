#ifndef _divoom_disp_watch_h_
#define _divoom_disp_watch_h_
//void divoom_disp_watch_init(void *data);
void divoom_disp_watch_init(void * data, DIVOOM_LIGHT_GET_TOOL_INFO get_info_func);
//void divoom_disp_watch_play(uint8 status);
void divoom_disp_watch_play(Pdivoom_disp_ctrl_watch_ctrl info);
void divoom_disp_watch_close(void);

#endif



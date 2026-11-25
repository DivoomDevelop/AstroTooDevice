#ifndef _divoom_disp_play_net_image_h_

#define _divoom_disp_play_net_image_h_


typedef struct _divoom_disp_Play_net_image_image_info
{
    int image_id;
    int event_id;
    int model_id;
    int play_time;
    DIVOOM_DISP_CTRL_OVER_CALL_END *pcallback;
}divoom_disp_Play_net_image_image_info, *Pdivoom_disp_Play_net_image_image_info;

void divoom_disp_Play_net_image_init(void *data);

void divoom_disp_Play_net_image_update(void *data);

void divoom_disp_Play_net_image_exit(void);

#endif


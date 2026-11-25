#ifndef _divoom_disp_time_manage_h_

typedef struct _divoom_disp_time_manage_disp_item
{
    T_U8 index;
    T_U8 disp_mode;  //0:表盘, 1:云图
    int clock_id;
    void *psubscribe_info;
    void *puser_define;
}divoom_disp_time_manage_disp_item, *Pdivoom_disp_time_manage_disp_item;
void divoom_disp_time_manange_init(void *data);
void divoom_disp_time_manange_update(void *data);
void divoom_disp_time_manange_close(void);
void divoom_disp_time_manange_play(uint8 status);

#endif



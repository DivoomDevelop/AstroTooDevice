#ifndef _DIVOOM_DISP_HIGH_PRI_EVENT_DIS_CTRL_H_
#define _DIVOOM_DISP_HIGH_PRI_EVENT_DIS_CTRL_H_

//#define DIVOOM_DISP_HIGH_PTI_EVENT_MAX_PIC 24
/*
typedef struct _divoom_disp_high_pri_event_
{
    uint8 on_time;       //显示时间
    uint8 period_time;   //显示一张图片的总时间:图片时间和不显示图片时间
    uint8 tol_pic;       //
    uint16 tol_time;      //总时间 100ms
    uint16 disp_id[DIVOOM_DISP_HIGH_PTI_EVENT_MAX_PIC];    //??ID
}divoom_disp_high_pri_event_struct, *Pdivoom_disp_high_pri_event_struct;*/

/*typedef struct _divoom_disp_high_pri_back_event_
{
	uint32 need_back_event;
	uint32 all_event;
}divoom_disp_high_pri_back_event, *Pdivoom_disp_high_pri_back_event;*/

//void divoom_disp_high_pri_event_dis_ctrl_main(void *data);

//需要还原的模式，图片信息
//void divoom_disp_high_pri_event_dis_ctrl_init(Pdivoom_disp_high_pri_back_event event_info, Pdivoom_disp_high_pri_event_struct pic_info);
void divoom_disp_high_pri_event_dis_ctrl_init(Pdivoom_disp_high_pri_event_struct pic_info);

void divoom_disp_high_pri_event_dis_ctrl_close(void);

#endif





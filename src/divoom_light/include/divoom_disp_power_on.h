#ifndef _divoom_disp_power_on_
#define _divoom_disp_power_on_



typedef struct _divoom_disp_power_on_disp_info
{
    int  power_down_flag;
    int  pic_len;
	uint8 *disp_buf;
    uint32 noice_id;      //结束通知标记
    DIVOOM_LIGHT_DISP_OVER_CALLBACK *pover_callback;
}divoom_disp_power_on_disp_info, *Pdivoom_disp_power_on_disp_info;



void divoom_disp_power_on_init(void * data);    //data == Pdivoom_disp_power_on_disp_info
void divoom_disp_power_on_play(uint8 play);
void divoom_disp_power_on_close(void);

#endif


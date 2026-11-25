#ifndef _divoom_disp_light_h_
#define _divoom_disp_light_h_


enum
{
	DIVOOM_DISP_LIGHT_SUN,
	DIVOOM_DISP_LIGHT_RAINBOW,
	DIVOOM_DISP_LIGHT_PLANT,
	DIVOOM_DISP_LIGHT_NO_MOSQUITO,
	DIVOOM_DISP_LIGHT_SLEEP,
	DIVOOM_DISP_LIGHT_END_MODE,
};


void divoom_disp_light_init(Pdivoom_disp_ctrl_light_color_info light_info);
void divoom_disp_light_set_attr(Pdivoom_disp_ctrl_light_color_info light_info);
void divoom_disp_light_auto_attr(Pdivoom_disp_ctrl_light_color_info light_info,T_BOOL arrow);
void divoom_disp_light_play(uint8 status);
void divoom_disp_light_close(void);

#endif



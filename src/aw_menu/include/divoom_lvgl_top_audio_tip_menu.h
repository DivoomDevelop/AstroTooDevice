#ifndef _divoom_lvgl_top_audio_tip_menu_h_
#define _divoom_lvgl_top_audio_tip_menu_h_

#define DIVOOM_AUDIO_TIP_TF_INSERTED		0
#define DIVOOM_AUDIO_TIP_TF_REMOVED		1
#define DIVOOM_AUDIO_TIP_AUX_INSERTED		2
#define DIVOOM_AUDIO_TIP_AUX_REMOVED		3
#define DIVOOM_AUDIO_TIP_UAC_INSERTED		4
#define DIVOOM_AUDIO_TIP_UAC_REMOVED		5
#define DIVOOM_AUDIO_TIP_BT_CONNECTED		6
#define DIVOOM_AUDIO_TIP_BT_DISCONNECTED	7
#define DIVOOM_AUDIO_TIP_TWS_CONNECTED_LEFT	8
#define DIVOOM_AUDIO_TIP_TWS_CONNECTED_RIGHT	9

void divoom_lvgl_top_audio_tip_menu_init(int audio_tip_flag);
void divoom_lvgl_top_audio_tip_menu_poll(void);

void divoom_lvgl_top_audio_tip_menu_exit(void);

#endif

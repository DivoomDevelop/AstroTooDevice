#ifndef _divoom_local_music_ctrl_tone_priv_h_
#define _divoom_local_music_ctrl_tone_priv_h_

#include "divoom.h"
#include "divoom_local_music_ctrl.h"

typedef struct _divoom_local_music_ctrl_tina_music_item {
	int music_status;
	void *lock;
	void *music_manage_sem;
	void *playhandle;
	int end_time;
	int play_time;
	char next_str[1024];
	char *play_str;
	void *play_queue;
	divoom_local_music_ctrl_music_play_get_next *pcallback;
} divoom_local_music_ctrl_tina_music_item, *Pdivoom_local_music_ctrl_tina_music_item;

typedef struct _divoom_local_music_ctrl_tina_ctrl {
	char tone_item_name[256];
	divoom_local_music_ctrl_tina_music_item music_item;
	divoom_local_music_ctrl_tina_music_item tone_item;
	int current_music_volume;
	volatile int music_stop_done_flag;
	int tone_pause_music_pending_resume;
	int tone_paused_mode;
	int tone_saved_sys_vol;
	int tone_saved_sys_vol_valid;
	int tone_cut_pending;
} divoom_local_music_ctrl_tina_ctrl, *Pdivoom_local_music_ctrl_tina_ctrl;

typedef struct _divoom_local_music_ctrl_tina_data_item {
	int type;
	char *music_name;
	int volume;
	divoom_local_music_ctrl_music_play_get_next *pnext_callback;
	int play_time;
} divoom_local_music_ctrl_tina_data_item, *Pdivoom_local_music_ctrl_tina_data_item;

Pdivoom_local_music_ctrl_tina_ctrl divoom_local_music_ctrl_tone_priv_get_ctrl(void);

void divoom_local_music_ctrl_tone_priv_close_sink_if_open(
	Pdivoom_local_music_ctrl_tina_music_item play_handle);
int divoom_local_music_ctrl_tone_priv_stop_requested(Pdivoom_local_music_ctrl_tina_music_item play_handle);
void divoom_local_music_ctrl_tone_priv_mute_jl_if_needed(void);
void divoom_local_music_ctrl_tone_priv_resume_after_tone(void);

#endif

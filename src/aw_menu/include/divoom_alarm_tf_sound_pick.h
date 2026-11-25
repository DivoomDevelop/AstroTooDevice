#ifndef _divoom_alarm_tf_sound_pick_h_
#define _divoom_alarm_tf_sound_pick_h_

#include "divoom.h"

#ifdef DIVOOM_TIME_FRAME_SPEAKER

#define DIVOOM_ALARM_TF_SOUND_MODE 4

typedef struct divoom_alarm_tf_sound_pick_ctrl divoom_alarm_tf_sound_pick_ctrl;

typedef void (*divoom_alarm_tf_sound_open_classic_cb)(void *user_data);
typedef void (*divoom_alarm_tf_sound_open_scene_cb)(void *user_data);
typedef void (*divoom_alarm_tf_sound_open_record_cb)(void *user_data);

typedef struct
{
	int menu_id;
	lv_obj_t *parent;
	T_DIVOOM_ALARM_ACTIVE_DO_INFO *alarm_info;
	lv_obj_t *sound_btn;
	lv_obj_t *music_label_obj;
	lv_obj_t *sound_music_label;
	divoom_alarm_tf_sound_open_classic_cb open_classic;
	divoom_alarm_tf_sound_open_scene_cb open_scene;
	divoom_alarm_tf_sound_open_record_cb open_record;
	void *user_data;
} divoom_alarm_tf_sound_pick_cfg;

divoom_alarm_tf_sound_pick_ctrl *divoom_alarm_tf_sound_pick_create(const divoom_alarm_tf_sound_pick_cfg *cfg);
void divoom_alarm_tf_sound_pick_destroy(divoom_alarm_tf_sound_pick_ctrl *ctrl);
void divoom_alarm_tf_sound_pick_open_select(divoom_alarm_tf_sound_pick_ctrl *ctrl);
void divoom_alarm_tf_sound_pick_refresh_label(divoom_alarm_tf_sound_pick_ctrl *ctrl);
void divoom_alarm_tf_sound_pick_apply_tf_state(divoom_alarm_tf_sound_pick_ctrl *ctrl);
int divoom_alarm_tf_sound_pick_is_subpage_visible(divoom_alarm_tf_sound_pick_ctrl *ctrl);
int divoom_alarm_tf_sound_card_inserted(void);

#endif

#endif

#include "divoom_lvgl_stream_cont.h"
#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_alarm_mode.h"
#ifdef DIVOOM_TIME_FRAME_SPEAKER
#include "divoom_alarm_tf_sound_pick.h"
#include "divoom_manage_image.h"
#include "divoom_local_music_ctrl.h"
#include "divoom_tf_music.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#endif


typedef struct _divoom_lvgl_alarm_edit_menu_ctrl
{
    int menu_id;
	lv_style_t backgroud_type;
	void *main_obj;
	void *bg_obj;

	int am_pm_flag;
	int hour;
	int minute;
	void *am_pm_disp_label;
	void *hour_disp_label;
	void *minute_disp_label;

	lv_style_t obj_style;

	int btn_status;	//0:未按下enter 1：按下enter
	void *first_active_obj;
	void *main_default_focus_obj;
	lv_style_t press_type;
	lv_style_t normal_type;
	lv_style_t select_press_type;
	void *pdirect_handle;
	
	//void *save_obj;
	//lv_style_t btn_normal_style;
	//lv_style_t btn_focus_style;
	void *save_btn;
	void *save_label;

	void *pdirect_repeat_handle;
	void *pdirect_music_handle;

	void *time_obj;
	int active_id;

	void *alarm_onoff_img;

	void *repeat_btn;
	void *cys_label_obj;		//重复标签对象
	void *week_list_obj;			
	void *weekday_btn_obj[7];	
	void *weekday_image_obj[7];	//多选重复

	void *sound_btn;
	void *music_label_obj;		//铃声标签对象
	void *bells_obj;
	void *bell_cont_obj[12];
	void *cur_active_obj;

#ifdef DIVOOM_TIME_FRAME_SPEAKER
	//铃声/显示/录音子页
	void *display_btn;
	void *display_label_obj;
	int display_index;
	void *display_select_obj;
	void *display_select_btn[7];
	void *pdirect_display_handle;

	void *sound_select_obj;
	void *sound_select_btn[4];
	void *sound_time_obj;
	void *sound_music_label;
	int sound_hour;
	int sound_minute;
	void *pdirect_sound_select_handle;

	void *scene_list_obj;
	void *scene_list_btn[6];
	void *scene_check_img[6];
	void *pdirect_scene_handle;

	void *record_obj;
	void *record_file_btn;
	void *record_file_title;
	void *record_file_dur;
	void *record_panel;
	void *record_panel_title;
	void *record_panel_icon;
	void *record_play_center_img;
	void *record_wave_img;
	void *record_time_lbl;
	void *record_as_bell_btn;
	void *pdirect_record_handle;
	lv_timer_t *record_refresh_timer;
	uint8 record_capture_active;
	uint8 record_playing;
	uint32 record_elapsed_sec;
	uint8 sub_pages_ready;
	lv_timer_t *sub_pages_defer_timer;

	void *tf_pick_obj;
	void *tf_left_panel;
	void *tf_right_panel;
	lv_obj_t *tf_left_folder_btn[DIVOOM_TF_MUSIC_MAX_ROOT + 2];
	void *tf_right_music_btn[30];
	void *tf_right_music_label[30];
	int tf_left_selected_index;
	int tf_left_item_count;
	divoom_tf_entry tf_hub[DIVOOM_TF_MUSIC_MAX_ROOT];
	int tf_hub_count;
	int tf_folder_id;
	divoom_tf_entry tf_page[30];
	int tf_page_n;
	int tf_music_count;
	int tf_right_first_index;
	int tf_right_focus_slot;
	lv_style_t tf_normal_type;
	lv_style_t tf_press_type;
	uint8 tf_styles_inited;
#endif
	void *vol_slide_obj;
	
	T_DIVOOM_ALARM_ACTIVE_DO_INFO *cur_alarm_info;
	T_DIVOOM_ALARM_ACTIVE_DO_INFO *edit_alarm_info;

	int cur_hour;
	int cur_minute;
	lv_timer_t *menu_bar_timer;
}divoom_lvgl_alarm_edit_menu_ctrl, *Pdivoom_lvgl_alarm_edit_menu_ctrl;

static const char *divoom_alarm_edit_bell_menu_img_list[] = {
    "49/49_bell_rain.png",
    "49/49_bell_wave.png",
    "49/49_bell_city.png",
    "49/49_bell_forest.png",
    "49/49_bell_dolphin.png",
    "49/49_bell_sun.png",
    "49/49_bell_bird.png",
    "49/49_bell_ring.png",
    "49/49_bell_clock.png",
    "49/49_bell_piano.png",
    "49/49_bell_clock2.png",
    "49/49_bell_music.png",
};

#ifdef DIVOOM_TIME_FRAME_SPEAKER
static const char *divoom_alarm_edit_bell_menu_scene_img_list[] = {
    "49/49_scene1.png",
    "49/49_scene2.png",
    "49/49_scene3.png",
    "49/49_scene4.png",
    "49/49_scene5.png",
    "49/49_scene6.png",
};

#define DIVOOM_ALARM_EDIT_DISPLAY_COUNT         7
#define DIVOOM_ALARM_EDIT_SCENE_COUNT           6
#define DIVOOM_ALARM_EDIT_SCENE_ROW_X           44
#define DIVOOM_ALARM_EDIT_SCENE_ROW_STEP        98
#define DIVOOM_ALARM_EDIT_SCENE_FIRST_Y         160
#define DIVOOM_ALARM_EDIT_SCENE_BTN_X           48
#define DIVOOM_ALARM_EDIT_SCENE_BTN_Y           164
#define DIVOOM_ALARM_EDIT_SCENE_BTN_W           712
#define DIVOOM_ALARM_EDIT_SCENE_BTN_H           90
#define DIVOOM_ALARM_EDIT_SCENE_BTN_RADIUS      24
#define DIVOOM_ALARM_EDIT_SCENE_LABEL_X         31
#define DIVOOM_ALARM_EDIT_SCENE_LABEL_Y         32
#define DIVOOM_ALARM_EDIT_SCENE_CHECK_X         644
#define DIVOOM_ALARM_EDIT_SCENE_CHECK_Y         27
#define DIVOOM_ALARM_EDIT_SCENE_CHECK_W         36
#define DIVOOM_ALARM_EDIT_SCENE_CHECK_H         36
#define DIVOOM_ALARM_EDIT_SCENE_FOCUS_BORDER    0x6D6D6D
#define DIVOOM_ALARM_EDIT_SOUND_ITEM_COUNT      4
#define DIVOOM_ALARM_EDIT_SOUND_FOCUS_COUNT     4
#define DIVOOM_ALARM_EDIT_TF_MODE               4
#define DIVOOM_ALARM_EDIT_TF_VISIBLE_ROWS       30
#define DIVOOM_ALARM_EDIT_TF_PANEL_H            1180
#define DIVOOM_ALARM_EDIT_RECORD_TMP_MP3        DIVOOM_TMP_DIR "/divoom_alarm_record_cap.mp3"
#define DIVOOM_ALARM_EDIT_RECORD_MIN_BYTES      128
#define DIVOOM_ALARM_EDIT_RECORD_MAX_SEC        (5u * 60u)
#define DIVOOM_ALARM_EDIT_RECORD_MP3_BPS        4000
#define DIVOOM_ALARM_EDIT_RECORD_PREVIEW_SEC    60
#define DIVOOM_ALARM_EDIT_RECORD_PANEL_X        40
#define DIVOOM_ALARM_EDIT_RECORD_PANEL_Y        160
#define DIVOOM_ALARM_EDIT_RECORD_PANEL_W        720
#define DIVOOM_ALARM_EDIT_RECORD_PANEL_H        392
#define DIVOOM_ALARM_EDIT_RECORD_FILE_ROW_Y     572
#define DIVOOM_ALARM_EDIT_RECORD_FILE_ROW_H     98
#define DIVOOM_ALARM_EDIT_RECORD_AS_BELL_Y      670
#define DIVOOM_ALARM_EDIT_RECORD_AS_BELL_H      98
#define DIVOOM_ALARM_EDIT_RECORD_FOCUS_COUNT    3
#define DIVOOM_ALARM_EDIT_RECORD_IMG_WAVE       "105/105_record2.png"
#define DIVOOM_ALARM_EDIT_RECORD_IMG_IDLE       "105/105_record_round1.png"
#define DIVOOM_ALARM_EDIT_RECORD_IMG_STOP       "105/105_record_round2.png"
#define DIVOOM_ALARM_EDIT_RECORD_IMG_PLAYING    "105/105_playing.png"
#define DIVOOM_ALARM_EDIT_RECORD_HINT_COLOR     0x797979
#define DIVOOM_ALARM_EDIT_PREVIEW_SEC           60

/* MasterGo 721:72418 row order: Skyline, Courtyard, Cityscape, Soft gradient, Misty meadow, Forest dawn */
static const uint8 divoom_alarm_edit_scene_display_order[DIVOOM_ALARM_EDIT_SCENE_COUNT] = {1, 4, 3, 0, 2, 5};

static void divoom_alarm_edit_display_index_load(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle);
static void divoom_alarm_edit_display_index_save(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle);
static void divoom_alarm_edit_display_label_refresh(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle);
static void divoom_alarm_edit_sound_label_refresh(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle);
static void divoom_alarm_edit_scene_apply_row_style(lv_obj_t *btn);
static int divoom_alarm_edit_scene_row_from_index(uint8 scene_index);
static int divoom_alarm_edit_scene_row_from_obj(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle, lv_obj_t *obj);
static void divoom_alarm_edit_scene_refresh_selection(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle);
static void divoom_alarm_edit_preview_stop_all(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle);
static void divoom_alarm_edit_preview_selected_sound(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle);
static void divoom_alarm_edit_scene_preview_stop(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle);
static void divoom_alarm_edit_scene_preview_play(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle, uint8 scene_index);
static void divoom_alarm_edit_record_apply_row_style(lv_obj_t *btn, lv_coord_t radius);
static void divoom_alarm_edit_record_build_clip_title(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle, char *buf, int buf_len);
static void divoom_alarm_edit_record_get_path(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle, char *path, int path_len);
static int divoom_alarm_edit_record_file_valid(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle);
static void divoom_alarm_edit_record_stop_capture(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle);
static int divoom_alarm_edit_record_start_capture(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle);
static int divoom_alarm_edit_record_commit(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle);
static void divoom_alarm_edit_record_stop_play(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle);
static void divoom_alarm_edit_record_start_play(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle);
static void divoom_alarm_edit_record_update_panel(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle);
static void divoom_alarm_edit_record_refresh_ui(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle);
static void divoom_alarm_edit_record_refresh_timer_cb(lv_timer_t *t);
static void divoom_alarm_edit_sound_select_menu_event_cb(lv_event_t *e);
static void divoom_alarm_edit_menu_create_display_page(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle, void *main_obj);
static void divoom_alarm_edit_menu_create_scene_list_page(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle, void *main_obj);
static void divoom_alarm_edit_menu_create_record_page(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle, void *main_obj);
static void divoom_alarm_edit_menu_create_tf_pick_page(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle, void *main_obj);
static void divoom_alarm_edit_menu_create_sub_pages(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle);
static void divoom_alarm_edit_menu_ensure_sub_pages(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle);
static void divoom_alarm_edit_menu_sub_pages_defer_cb(lv_timer_t *t);
static void divoom_alarm_edit_record_timer_start(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle);
static void divoom_alarm_edit_record_timer_stop(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle);
static void divoom_alarm_edit_tf_paint_left_selection(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle, int index);
static void divoom_alarm_edit_tf_update_music_list(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle);
static void divoom_alarm_edit_tf_rebuild_left(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle);
static lv_obj_t *divoom_alarm_edit_tf_create_folder_btn(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle,
                                                      lv_obj_t *parent, int y, const char *label_text);
static void divoom_alarm_edit_menu_restore_main_focus_if_main(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle);
static void divoom_alarm_edit_display_menu_event_cb(lv_event_t *e);
static void divoom_alarm_edit_scene_list_menu_event_cb(lv_event_t *e);
static void divoom_alarm_edit_record_menu_event_cb(lv_event_t *e);
static void divoom_alarm_edit_tf_pick_left_event_cb(lv_event_t *e);
static void divoom_alarm_edit_tf_pick_right_event_cb(lv_event_t *e);
static int divoom_alarm_edit_tf_card_available(void);
static void divoom_alarm_edit_tf_label_text(char *out, int out_size, const char *leave_url);
static void divoom_alarm_edit_sound_select_apply_tf_state(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle);
static int divoom_alarm_edit_menu_active_volume_id(void);
static int divoom_alarm_edit_menu_active_save_id(void);
#endif

void divoom_menu_alarm_refresh_edit_menu_bar(lv_timer_t *menu_bar_timer)
{
    int hour;
    Pdivoom_lvgl_alarm_edit_menu_ctrl phandle = (Pdivoom_lvgl_alarm_edit_menu_ctrl)lv_timer_get_user_data(menu_bar_timer);
	time_t time_now;
	struct tm p_now;
	
    time_now = time(NULL);
	localtime_r(&time_now, &p_now);
	hour = p_now.tm_hour;
    if(divoom_main_get_time_flag() == 0)
    {
        if(hour != 12)
        {
            hour = hour % 12;
            if(hour == 0)
            {
                hour = 12;
            }
        }
    }

    if(hour != phandle->cur_hour || p_now.tm_min != phandle->cur_minute)
    {
        phandle->cur_hour = hour;
        phandle->cur_minute = p_now.tm_min;
        lv_label_set_text_fmt(phandle->time_obj, "%02d:%02d", hour, phandle->cur_minute);
    }
}

int divoom_alarm_edit_menu_convert_to_twenty_four_hour(int hour, int am_pm_flag, int time_flag) {
    if (time_flag == 1) {
        // 已经是 24 小时制，直接返回
        return hour;
    } else {
        // 12 小时制转换为 24 小时制
        if (am_pm_flag == 0) { // AM
            if (hour == 12) {
                return 0; // 12 AM 转换为 0
            } else {
                return hour; // 其他 AM 时间保持不变
            }
        } else { // PM
            if (hour == 12) {
                return 12; // 12 PM 保持为 12
            } else {
                return hour + 12; // 其他 PM 时间加 12
            }
        }
    }
}

#ifdef DIVOOM_TIME_FRAME_SPEAKER
static int divoom_alarm_edit_menu_active_volume_id(void)
{
	return 7;
}

static int divoom_alarm_edit_menu_active_save_id(void)
{
	return 8;
}

static void divoom_alarm_edit_display_index_load(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	if(phandle == NULL || phandle->edit_alarm_info == NULL)
	{
		return;
	}
	if(strncmp(phandle->edit_alarm_info->image_data, "disp:", 5) == 0)
	{
		phandle->display_index = atoi(phandle->edit_alarm_info->image_data + 5);
	}
	else
	{
		phandle->display_index = 0;
	}
	if(phandle->display_index < 0 || phandle->display_index >= DIVOOM_ALARM_EDIT_DISPLAY_COUNT)
	{
		phandle->display_index = 0;
	}
}

static void divoom_alarm_edit_display_index_save(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	if(phandle == NULL || phandle->edit_alarm_info == NULL)
	{
		return;
	}
	snprintf(phandle->edit_alarm_info->image_data, sizeof(phandle->edit_alarm_info->image_data),
	         "disp:%d", phandle->display_index);
}

static void divoom_alarm_edit_display_label_refresh(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	char disp_str[64];

	if(phandle == NULL || phandle->display_label_obj == NULL)
	{
		return;
	}
	if(phandle->display_index <= 0)
	{
		lv_label_set_text(phandle->display_label_obj,
		                  divoom_menu_control_get_disp_str("AlarmDisplayNormal"));
	}
	else
	{
		snprintf(disp_str, sizeof(disp_str), "alarm_wake_scene%d", phandle->display_index);
		lv_label_set_text(phandle->display_label_obj, divoom_menu_control_get_disp_str(disp_str));
	}
}

static void divoom_alarm_edit_sound_label_refresh(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	char disp_str[64];

	if(phandle == NULL || phandle->music_label_obj == NULL)
	{
		return;
	}
	if(phandle->edit_alarm_info->mode == 4)
	{
		snprintf(disp_str, sizeof(disp_str), "alarm_wake_scene%d", phandle->edit_alarm_info->scene_index + 1);
		lv_label_set_text(phandle->music_label_obj, divoom_menu_control_get_disp_str(disp_str));
	}
	else if(phandle->edit_alarm_info->mode == 1)
	{
		if(divoom_alarm_edit_record_file_valid(phandle))
		{
			lv_label_set_text(phandle->music_label_obj, divoom_menu_control_get_disp_str("VoiceRecordHasClip"));
		}
		else
		{
			lv_label_set_text(phandle->music_label_obj, divoom_menu_control_get_disp_str("VoiceRecordNoClip"));
		}
	}
	else if(phandle->edit_alarm_info->mode == DIVOOM_ALARM_EDIT_TF_MODE)
	{
		char tf_label[64];

		divoom_alarm_edit_tf_label_text(tf_label, sizeof(tf_label), phandle->edit_alarm_info->leave_url);
		lv_label_set_text(phandle->music_label_obj, tf_label);
	}
	else
	{
		lv_label_set_text_fmt(phandle->music_label_obj, "Music %d", phandle->edit_alarm_info->tip_index + 1);
	}
	if(phandle->sound_music_label)
	{
		if(phandle->edit_alarm_info->mode == 4)
		{
			snprintf(disp_str, sizeof(disp_str), "alarm_wake_scene%d", phandle->edit_alarm_info->scene_index + 1);
			lv_label_set_text(phandle->sound_music_label, divoom_menu_control_get_disp_str(disp_str));
		}
		else if(phandle->edit_alarm_info->mode == 1)
		{
			lv_label_set_text(phandle->sound_music_label, divoom_menu_control_get_disp_str("SleepAidTabRecord"));
		}
		else if(phandle->edit_alarm_info->mode == DIVOOM_ALARM_EDIT_TF_MODE)
		{
			char tf_label[64];

			divoom_alarm_edit_tf_label_text(tf_label, sizeof(tf_label), phandle->edit_alarm_info->leave_url);
			lv_label_set_text(phandle->sound_music_label, tf_label);
		}
		else
		{
			lv_label_set_text_fmt(phandle->sound_music_label, "Music %d", phandle->edit_alarm_info->tip_index + 1);
		}
	}
}

static void divoom_alarm_edit_scene_apply_row_style(lv_obj_t *btn)
{
	lv_obj_set_style_pad_all(btn, 0, 0);
	lv_obj_set_style_bg_color(btn, lv_color_hex(0xF2F2F2), 0);
	lv_obj_set_style_radius(btn, DIVOOM_ALARM_EDIT_SCENE_BTN_RADIUS, 0);
	lv_obj_set_style_border_width(btn, 4, 0);
	lv_obj_set_style_outline_width(btn, 0, 0);
	lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_shadow_width(btn, 0, 0);
	lv_obj_set_style_border_color(btn, lv_color_hex(0xF2F2F2), 0);
	lv_obj_set_style_border_color(btn, lv_color_hex(DIVOOM_ALARM_EDIT_SCENE_FOCUS_BORDER),
	                              LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
}

static int divoom_alarm_edit_scene_row_from_index(uint8 scene_index)
{
	int i;

	for(i = 0; i < DIVOOM_ALARM_EDIT_SCENE_COUNT; i++)
	{
		if(divoom_alarm_edit_scene_display_order[i] == scene_index)
		{
			return i;
		}
	}
	return 0;
}

static void divoom_alarm_edit_scene_refresh_selection(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	int i;
	uint8 sel_index;

	if(phandle == NULL || phandle->edit_alarm_info == NULL)
	{
		return;
	}
	sel_index = (phandle->edit_alarm_info->mode == 4) ? phandle->edit_alarm_info->scene_index : 255;
	for(i = 0; i < DIVOOM_ALARM_EDIT_SCENE_COUNT; i++)
	{
		if(phandle->scene_check_img[i] == NULL)
		{
			continue;
		}
		if(sel_index != 255 && divoom_alarm_edit_scene_display_order[i] == sel_index)
		{
			lv_obj_clear_flag(phandle->scene_check_img[i], LV_OBJ_FLAG_HIDDEN);
		}
		else
		{
			lv_obj_add_flag(phandle->scene_check_img[i], LV_OBJ_FLAG_HIDDEN);
		}
	}
}

static int divoom_alarm_edit_scene_row_from_obj(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle, lv_obj_t *obj)
{
	int i;

	if(phandle == NULL || obj == NULL)
	{
		return -1;
	}
	for(i = 0; i < DIVOOM_ALARM_EDIT_SCENE_COUNT; i++)
	{
		if(obj == phandle->scene_list_btn[i])
		{
			return i;
		}
	}
	return -1;
}

static void divoom_alarm_edit_record_apply_row_style(lv_obj_t *btn, lv_coord_t radius)
{
	lv_obj_set_style_pad_all(btn, 0, 0);
	lv_obj_set_style_bg_color(btn, lv_color_hex(0xF2F2F2), 0);
	lv_obj_set_style_radius(btn, radius, 0);
	lv_obj_set_style_border_width(btn, 4, 0);
	lv_obj_set_style_outline_width(btn, 0, 0);
	lv_obj_set_style_shadow_width(btn, 0, 0);
	lv_obj_set_style_border_color(btn, lv_color_hex(0xF2F2F2), 0);
	lv_obj_set_style_border_color(btn, lv_color_hex(0xE89B00), LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
}

static void divoom_alarm_edit_record_build_clip_title(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle, char *buf, int buf_len)
{
	char path[256];
	divoom_port_stat_t st;
	struct tm lt;

	if(phandle == NULL || buf == NULL || buf_len <= 0)
	{
		return;
	}
	divoom_alarm_edit_record_get_path(phandle, path, (int)sizeof(path));
	if(divoom_port_stat(path, &st) == 0)
	{
		time_t mtime = (time_t)st.mtime;
		localtime_r(&mtime, &lt);
		snprintf(buf, (size_t)buf_len, "%02d-%02d%s", lt.tm_mon + 1, lt.tm_mday,
		         divoom_menu_control_get_disp_str("VoiceRecordClipSuffix"));
	}
	else
	{
		snprintf(buf, (size_t)buf_len, "%s", divoom_menu_control_get_disp_str("VoiceRecordHasClip"));
	}
}

static void divoom_alarm_edit_record_get_path(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle, char *path, int path_len)
{
	if(phandle == NULL || path == NULL || path_len <= 0 || phandle->edit_alarm_info == NULL)
	{
		return;
	}
	divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_ALARM_RECORD,
	                                         (uint32)phandle->edit_alarm_info->alarm_id, path);
}

static int divoom_alarm_edit_record_file_valid(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	char path[256];
	int sz;

	if(phandle == NULL)
	{
		return 0;
	}
	divoom_alarm_edit_record_get_path(phandle, path, (int)sizeof(path));
	if(!divoom_port_file_exists(path))
	{
		return 0;
	}
	sz = divoom_app_com_get_file_size(path);
	if(sz < DIVOOM_ALARM_EDIT_RECORD_MIN_BYTES)
	{
		return 0;
	}
	return 1;
}

static uint32 divoom_alarm_edit_record_duration_sec(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	char path[256];
	int sz;

	if(!divoom_alarm_edit_record_file_valid(phandle))
	{
		return 0;
	}
	divoom_alarm_edit_record_get_path(phandle, path, (int)sizeof(path));
	sz = divoom_app_com_get_file_size(path);
	if(sz <= 0)
	{
		return 0;
	}
	return (uint32)(sz / DIVOOM_ALARM_EDIT_RECORD_MP3_BPS);
}

static void divoom_alarm_edit_record_stop_capture(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	if(phandle == NULL)
	{
		return;
	}
	if(divoom_local_music_ctrl_voice_clip_record_is_active())
	{
		divoom_local_music_ctrl_voice_clip_record_stop_sync();
	}
	phandle->record_capture_active = 0;
}

static int divoom_alarm_edit_record_start_capture(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	if(phandle == NULL)
	{
		return -1;
	}
	divoom_alarm_edit_record_stop_play(phandle);
	divoom_alarm_edit_record_stop_capture(phandle);
	divoom_app_com_del_file((char *)DIVOOM_ALARM_EDIT_RECORD_TMP_MP3);
	if(divoom_local_music_ctrl_voice_clip_record_start(DIVOOM_ALARM_EDIT_RECORD_TMP_MP3, 0, NULL) != 0)
	{
		return -1;
	}
	phandle->record_capture_active = 1;
	phandle->record_elapsed_sec = 0;
	return 0;
}

static int divoom_alarm_edit_record_commit(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	char dest[256];
	char dirpath[256];
	int tmp_sz;
	int dest_sz;

	if(phandle == NULL)
	{
		return -1;
	}
	tmp_sz = divoom_app_com_get_file_size((char *)DIVOOM_ALARM_EDIT_RECORD_TMP_MP3);
	if(tmp_sz < DIVOOM_ALARM_EDIT_RECORD_MIN_BYTES)
	{
		divoom_app_com_del_file((char *)DIVOOM_ALARM_EDIT_RECORD_TMP_MP3);
		return -1;
	}
	divoom_manage_image_download_get_path_name(DIVOOM_MANAGE_IMAGE_ALARM_RECORD, 0, dirpath);
	divoom_app_com_check_and_mk_dir(dirpath);
	divoom_alarm_edit_record_get_path(phandle, dest, (int)sizeof(dest));
	divoom_app_com_copy_file((char *)DIVOOM_ALARM_EDIT_RECORD_TMP_MP3, dest);
	divoom_app_com_del_file((char *)DIVOOM_ALARM_EDIT_RECORD_TMP_MP3);
	if(!divoom_port_file_exists(dest))
	{
		return -1;
	}
	dest_sz = divoom_app_com_get_file_size(dest);
	if(dest_sz < DIVOOM_ALARM_EDIT_RECORD_MIN_BYTES)
	{
		divoom_app_com_del_file(dest);
		return -1;
	}
	phandle->edit_alarm_info->mode = 1;
	return 0;
}

static void divoom_alarm_edit_record_stop_play(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	if(phandle == NULL)
	{
		return;
	}
	divoom_local_music_ctrl_stop_tone();
	phandle->record_playing = 0;
	divoom_alarm_edit_record_update_panel(phandle);
}

static void divoom_alarm_edit_record_start_play(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	char path[256];
	uint32 sec;

	if(phandle == NULL || !divoom_alarm_edit_record_file_valid(phandle))
	{
		return;
	}
	divoom_alarm_edit_record_stop_capture(phandle);
	divoom_alarm_edit_record_get_path(phandle, path, (int)sizeof(path));
	sec = divoom_alarm_edit_record_duration_sec(phandle);
	if(sec <= 0)
	{
		sec = 1;
	}
	phandle->record_elapsed_sec = sec;
	divoom_local_music_ctrl_tone_play(path, phandle->edit_alarm_info->volume, NULL, (int)sec);
	phandle->record_playing = 1;
	divoom_alarm_edit_record_update_panel(phandle);
}

static void divoom_alarm_edit_preview_stop_all(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	divoom_alarm_ctrl_preview_stop();
#ifdef DIVOOM_TIMES_VIBE
	divoom_tf_music_pause_jl_play();
#endif
	if(phandle != NULL && phandle->record_playing)
	{
		phandle->record_playing = 0;
		divoom_alarm_edit_record_update_panel(phandle);
	}
}

static void divoom_alarm_edit_preview_selected_sound(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	char path[256];
	char file_name[128];
	uint32 sec;
	T_U8 vol;

	if(phandle == NULL || phandle->edit_alarm_info == NULL)
	{
		return;
	}
	vol = phandle->edit_alarm_info->volume;
	if(phandle->edit_alarm_info->mode == 4)
	{
		if(phandle->edit_alarm_info->scene_index >= DIVOOM_ALARM_EDIT_SCENE_COUNT)
		{
			return;
		}
		divoom_alarm_ctrl_try_listen_scene(phandle->edit_alarm_info->scene_index, vol, 1, 1);
	}
	else if(phandle->edit_alarm_info->mode == 1)
	{
		if(!divoom_alarm_edit_record_file_valid(phandle))
		{
			divoom_alarm_edit_preview_stop_all(phandle);
			return;
		}
		divoom_alarm_edit_record_get_path(phandle, path, (int)sizeof(path));
		sec = divoom_alarm_edit_record_duration_sec(phandle);
		if(sec <= 0)
		{
			sec = 1;
		}
		divoom_local_music_ctrl_tone_play(path, vol, NULL, (int)sec);
	}
	else if(phandle->edit_alarm_info->mode == DIVOOM_ALARM_EDIT_TF_MODE)
	{
		int music_id;

		music_id = divoom_tf_music_alarm_resolve_music_id(phandle->edit_alarm_info->leave_url);
		if(music_id <= 0)
		{
			divoom_alarm_edit_preview_stop_all(phandle);
			return;
		}
		divoom_alarm_edit_preview_stop_all(phandle);
#ifdef DIVOOM_TIMES_VIBE
		divoom_tf_music_play_music_id(music_id, vol);
#else
		{
			Pdivoom_tf_music_item tf_item;

			tf_item = divoom_tf_music_get_item_by_id(music_id);
			if(tf_item != NULL && tf_item->music_name != NULL && tf_item->music_name[0] != '\0')
			{
				snprintf(file_name, sizeof(file_name), "file://%s", tf_item->music_name);
				divoom_local_music_ctrl_tone_play(file_name, vol, NULL, DIVOOM_ALARM_EDIT_PREVIEW_SEC);
			}
		}
#endif
	}
	else
	{
		if(phandle->edit_alarm_info->tip_index >= 12)
		{
			return;
		}
		divoom_alarm_ctrl_try_listen((T_U8)phandle->edit_alarm_info->tip_index, vol, 1, 1);
	}
}

static void divoom_alarm_edit_scene_preview_stop(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	divoom_alarm_edit_preview_stop_all(phandle);
}

static void divoom_alarm_edit_scene_preview_play(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle, uint8 scene_index)
{
	if(phandle == NULL || phandle->edit_alarm_info == NULL || scene_index >= DIVOOM_ALARM_EDIT_SCENE_COUNT)
	{
		return;
	}
	divoom_alarm_ctrl_try_listen_scene(scene_index, phandle->edit_alarm_info->volume, 1, 1);
}

static void divoom_alarm_edit_record_update_panel(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	unsigned mm;
	unsigned ss;
	uint32 sec;
	const char *title_key;

	if(phandle == NULL || phandle->record_panel_title == NULL)
	{
		return;
	}
	if(phandle->record_playing)
	{
		title_key = "VoiceRecordPlaying";
	}
	else if(phandle->record_capture_active)
	{
		title_key = "SleepAidRecording";
	}
	else if(divoom_alarm_edit_record_file_valid(phandle))
	{
		title_key = "AlarmRecordTapReRecord";
	}
	else
	{
		title_key = "SleepAidRecordStart";
	}
	lv_label_set_text(phandle->record_panel_title, divoom_menu_control_get_disp_str(title_key));

	if(phandle->record_panel_icon)
	{
		if(phandle->record_playing)
		{
			lv_obj_add_flag(phandle->record_panel_icon, LV_OBJ_FLAG_HIDDEN);
		}
		else
		{
			lv_obj_clear_flag(phandle->record_panel_icon, LV_OBJ_FLAG_HIDDEN);
			divoom_menu_control_lv_img_set_src(phandle->record_panel_icon,
			                                   phandle->record_capture_active
			                                       ? DIVOOM_ALARM_EDIT_RECORD_IMG_STOP
			                                       : DIVOOM_ALARM_EDIT_RECORD_IMG_IDLE);
		}
	}
	if(phandle->record_play_center_img)
	{
		if(phandle->record_playing)
		{
			divoom_menu_control_lv_img_set_src(phandle->record_play_center_img,
			                                   DIVOOM_ALARM_EDIT_RECORD_IMG_PLAYING);
			lv_obj_clear_flag(phandle->record_play_center_img, LV_OBJ_FLAG_HIDDEN);
		}
		else
		{
			lv_obj_add_flag(phandle->record_play_center_img, LV_OBJ_FLAG_HIDDEN);
		}
	}
	if(phandle->record_wave_img)
	{
		if(phandle->record_capture_active || phandle->record_playing)
		{
			divoom_menu_control_lv_img_set_src(phandle->record_wave_img, DIVOOM_ALARM_EDIT_RECORD_IMG_WAVE);
			lv_obj_clear_flag(phandle->record_wave_img, LV_OBJ_FLAG_HIDDEN);
		}
		else
		{
			lv_obj_add_flag(phandle->record_wave_img, LV_OBJ_FLAG_HIDDEN);
		}
	}
	if(phandle->record_time_lbl)
	{
		sec = phandle->record_elapsed_sec;
		if(phandle->record_capture_active || phandle->record_playing || !divoom_alarm_edit_record_file_valid(phandle))
		{
			if(sec > DIVOOM_ALARM_EDIT_RECORD_MAX_SEC)
			{
				sec = DIVOOM_ALARM_EDIT_RECORD_MAX_SEC;
			}
			mm = (unsigned)(sec / 60u);
			ss = (unsigned)(sec % 60u);
			lv_label_set_text_fmt(phandle->record_time_lbl, "%02u:%02u", mm, ss);
			lv_obj_clear_flag(phandle->record_time_lbl, LV_OBJ_FLAG_HIDDEN);
		}
		else
		{
			lv_label_set_text(phandle->record_time_lbl, "00:00");
			lv_obj_clear_flag(phandle->record_time_lbl, LV_OBJ_FLAG_HIDDEN);
		}
	}
}

static void divoom_alarm_edit_record_refresh_ui(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	char title[64];
	char dur[16];
	uint32 sec;
	unsigned mm;
	unsigned ss;

	if(phandle == NULL)
	{
		return;
	}
	if(phandle->record_file_title)
	{
		if(divoom_alarm_edit_record_file_valid(phandle))
		{
			divoom_alarm_edit_record_build_clip_title(phandle, title, (int)sizeof(title));
			lv_label_set_text(phandle->record_file_title, title);
		}
		else
		{
			lv_label_set_text(phandle->record_file_title,
			                  divoom_menu_control_get_disp_str("VoiceRecordNoClip"));
		}
	}
	sec = divoom_alarm_edit_record_duration_sec(phandle);
	mm = (unsigned)(sec / 60u);
	ss = (unsigned)(sec % 60u);
	snprintf(dur, sizeof(dur), "%02u:%02u\"", mm, ss);
	if(phandle->record_file_dur)
	{
		if(divoom_alarm_edit_record_file_valid(phandle))
		{
			lv_label_set_text(phandle->record_file_dur, dur);
		}
		else
		{
			lv_label_set_text(phandle->record_file_dur, "00:00\"");
		}
	}
	divoom_alarm_edit_record_update_panel(phandle);
}

static void divoom_alarm_edit_record_refresh_timer_cb(lv_timer_t *t)
{
	Pdivoom_lvgl_alarm_edit_menu_ctrl phandle;
	uint8 was_busy;

	phandle = (Pdivoom_lvgl_alarm_edit_menu_ctrl)lv_timer_get_user_data(t);
	if(phandle == NULL || phandle->record_refresh_timer != t)
	{
		return;
	}
	was_busy = phandle->record_capture_active;
	if(phandle->record_capture_active || divoom_local_music_ctrl_voice_clip_record_is_active())
	{
		phandle->record_elapsed_sec++;
		if(phandle->record_elapsed_sec >= DIVOOM_ALARM_EDIT_RECORD_MAX_SEC)
		{
			divoom_alarm_edit_record_stop_capture(phandle);
			divoom_alarm_edit_record_commit(phandle);
		}
	}
	else if(was_busy)
	{
		divoom_alarm_edit_record_commit(phandle);
		phandle->record_elapsed_sec = 0;
	}
	phandle->record_capture_active = divoom_local_music_ctrl_voice_clip_record_is_active()
		? 1 : phandle->record_capture_active;
	divoom_alarm_edit_record_refresh_ui(phandle);
}

static void divoom_alarm_edit_record_timer_start(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	if(phandle == NULL)
	{
		return;
	}
	if(phandle->record_refresh_timer == NULL)
	{
		phandle->record_refresh_timer = lv_timer_create(divoom_alarm_edit_record_refresh_timer_cb, 1000, phandle);
	}
}

static void divoom_alarm_edit_record_timer_stop(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	if(phandle == NULL || phandle->record_refresh_timer == NULL)
	{
		return;
	}
	lv_timer_del(phandle->record_refresh_timer);
	phandle->record_refresh_timer = NULL;
}

static void divoom_alarm_edit_tf_label_text(char *out, int out_size, const char *leave_url)
{
	Pdivoom_tf_music_item tf_item;
	const char *name;
	int music_id;

	if(out == NULL || out_size <= 0)
	{
		return;
	}
	out[0] = '\0';
	music_id = divoom_tf_music_alarm_resolve_music_id(leave_url);
	tf_item = divoom_tf_music_get_item_by_id(music_id);
	if(tf_item != NULL && tf_item->music_name != NULL && tf_item->music_name[0] != '\0')
	{
		name = divoom_tf_music_get_disp_name(tf_item->music_name);
		strncpy(out, name, (size_t)out_size - 1);
		out[out_size - 1] = '\0';
		return;
	}
	if(leave_url != NULL && leave_url[0] != '\0')
	{
		name = divoom_tf_music_get_disp_name(leave_url);
		strncpy(out, name, (size_t)out_size - 1);
		out[out_size - 1] = '\0';
		return;
	}
	strncpy(out, divoom_menu_control_get_disp_str("TFCard"), (size_t)out_size - 1);
	out[out_size - 1] = '\0';
}

static int divoom_alarm_edit_tf_load_hub(divoom_tf_entry *out, int cap)
{
	int n;
	int i;
	int keep;

	if(out == NULL || cap <= 0)
	{
		return 0;
	}
	if(divoom_database_tf_is_rebuilding())
	{
		return 0;
	}
	n = divoom_database_tf_dir_page(0, 0, cap, out, cap);
	keep = 0;
	for(i = 0; i < n; i++)
	{
		if(!out[i].is_dir || out[i].id <= 0)
		{
			continue;
		}
		if(strcasecmp(out[i].name, "tf") == 0)
		{
			continue;
		}
		if(keep != i)
		{
			out[keep] = out[i];
		}
		keep++;
	}
	return keep;
}

static int divoom_alarm_edit_tf_hub_folder_id(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	if(phandle == NULL)
	{
		return 0;
	}
	if(phandle->tf_hub_count <= 0)
	{
		return 0;
	}
	if(phandle->tf_left_selected_index < 0 || phandle->tf_left_selected_index >= phandle->tf_hub_count)
	{
		return 0;
	}
	return phandle->tf_hub[phandle->tf_left_selected_index].id;
}

static void divoom_alarm_edit_tf_free_music_list(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	if(phandle == NULL)
	{
		return;
	}
	memset(phandle->tf_page, 0, sizeof(phandle->tf_page));
	phandle->tf_page_n = 0;
	phandle->tf_music_count = 0;
	phandle->tf_folder_id = 0;
}

static int divoom_alarm_edit_tf_right_visible_in_window(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	int left;

	if(phandle == NULL || phandle->tf_music_count <= 0)
	{
		return 0;
	}
	left = phandle->tf_music_count - phandle->tf_right_first_index;
	if(left > DIVOOM_ALARM_EDIT_TF_VISIBLE_ROWS)
	{
		return DIVOOM_ALARM_EDIT_TF_VISIBLE_ROWS;
	}
	return left;
}

static int divoom_alarm_edit_tf_get_global_index(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	int g;

	if(phandle == NULL)
	{
		return -1;
	}
	g = phandle->tf_right_first_index + phandle->tf_right_focus_slot;
	if(g < 0 || g >= phandle->tf_music_count)
	{
		return -1;
	}
	return g;
}

static void divoom_alarm_edit_tf_refresh_right_slots(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	int i;
	int max_slot;
	lv_obj_t *item_cont;

	if(phandle == NULL)
	{
		return;
	}
	if(phandle->tf_right_first_index < 0)
	{
		phandle->tf_right_first_index = 0;
	}
	memset(phandle->tf_page, 0, sizeof(phandle->tf_page));
	phandle->tf_page_n = 0;
	if(phandle->tf_music_count > 0 && !divoom_database_tf_is_rebuilding())
	{
		phandle->tf_page_n = divoom_database_tf_subtree_page(phandle->tf_folder_id,
		                                                    phandle->tf_right_first_index,
		                                                    DIVOOM_ALARM_EDIT_TF_VISIBLE_ROWS,
		                                                    phandle->tf_page,
		                                                    DIVOOM_ALARM_EDIT_TF_VISIBLE_ROWS);
	}
	for(i = 0; i < DIVOOM_ALARM_EDIT_TF_VISIBLE_ROWS; i++)
	{
		int gi = phandle->tf_right_first_index + i;

		item_cont = phandle->tf_right_music_btn[i];
		if(item_cont == NULL)
		{
			continue;
		}
		if(i >= phandle->tf_page_n || phandle->tf_page[i].id <= 0 || phandle->tf_page[i].is_dir
		   || gi < 0 || gi >= phandle->tf_music_count)
		{
			lv_obj_add_flag(item_cont, LV_OBJ_FLAG_HIDDEN);
			continue;
		}
		lv_obj_clear_flag(item_cont, LV_OBJ_FLAG_HIDDEN);
		if(phandle->tf_right_music_label[i] && phandle->tf_page[i].name[0] != '\0')
		{
			lv_label_set_text(phandle->tf_right_music_label[i],
			                  divoom_tf_music_get_disp_name(phandle->tf_page[i].name));
		}
	}

	max_slot = divoom_alarm_edit_tf_right_visible_in_window(phandle) - 1;
	if(max_slot < 0)
	{
		max_slot = 0;
	}
	if(phandle->tf_right_focus_slot < 0)
	{
		phandle->tf_right_focus_slot = 0;
	}
	if(phandle->tf_right_focus_slot > max_slot)
	{
		phandle->tf_right_focus_slot = max_slot;
	}
	if(phandle->tf_pick_obj
	   && !lv_obj_has_flag(phandle->tf_pick_obj, LV_OBJ_FLAG_HIDDEN)
	   && phandle->tf_right_music_btn[phandle->tf_right_focus_slot])
	{
		lv_group_focus_obj(phandle->tf_right_music_btn[phandle->tf_right_focus_slot]);
		lv_obj_scroll_to_view(phandle->tf_right_music_btn[phandle->tf_right_focus_slot], LV_ANIM_OFF);
	}
}

static void divoom_alarm_edit_tf_create_music_item(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle,
                                                   lv_obj_t *parent, const char *name, int index)
{
	lv_obj_t *item_cont;
	lv_obj_t *name_label;
	lv_obj_t *music_icon;

	item_cont = lv_btn_create(parent);
	lv_obj_set_size(item_cont, 489 + 6, 72 + 6);
	lv_obj_add_style(item_cont, &phandle->tf_normal_type, 0);
	lv_obj_add_style(item_cont, &phandle->tf_press_type, LV_STATE_FOCUSED);
	lv_obj_add_style(item_cont, &phandle->tf_press_type, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(item_cont, 0, 0);
	lv_obj_clear_flag(item_cont, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_event_cb(item_cont, divoom_alarm_edit_tf_pick_right_event_cb, LV_EVENT_KEY, phandle);

	music_icon = lv_img_create(item_cont);
	lv_obj_set_pos(music_icon, 15, 20);
	divoom_menu_control_lv_img_set_src(music_icon, "92/92_music.png");

	name_label = lv_label_create(item_cont);
	if(name && name[0] != '\0')
	{
		lv_label_set_text(name_label, divoom_tf_music_get_disp_name(name));
	}
	else
	{
		lv_label_set_text(name_label, divoom_menu_control_get_disp_str("UnknownMusicName"));
	}
	lv_obj_set_style_text_font(name_label, divoom_menu_control_get_font_info(20, phandle->menu_id), 0);
	lv_obj_set_style_text_color(name_label, lv_color_hex(0x0), 0);
	lv_obj_set_pos(name_label, 53, 24);
	lv_obj_set_width(name_label, 497 - 120);
	lv_obj_set_height(name_label, 52 - 32);
	lv_label_set_long_mode(name_label, LV_LABEL_LONG_DOT);

	phandle->tf_right_music_btn[index] = item_cont;
	phandle->tf_right_music_label[index] = name_label;
}

static void divoom_alarm_edit_tf_update_music_list(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	int i;

	if(phandle == NULL || !divoom_alarm_edit_tf_card_available())
	{
		return;
	}
	if(phandle->tf_right_panel)
	{
		lv_obj_clean(phandle->tf_right_panel);
	}
	for(i = 0; i < DIVOOM_ALARM_EDIT_TF_VISIBLE_ROWS; i++)
	{
		phandle->tf_right_music_btn[i] = NULL;
		phandle->tf_right_music_label[i] = NULL;
	}
	divoom_alarm_edit_tf_free_music_list(phandle);

	phandle->tf_folder_id = divoom_alarm_edit_tf_hub_folder_id(phandle);
	if(!divoom_database_tf_is_rebuilding())
	{
		phandle->tf_music_count = divoom_database_tf_subtree_count(phandle->tf_folder_id);
	}
	if(phandle->tf_music_count <= 0)
	{
		return;
	}

	phandle->tf_right_first_index = 0;
	phandle->tf_right_focus_slot = 0;
	for(i = 0; i < DIVOOM_ALARM_EDIT_TF_VISIBLE_ROWS; i++)
	{
		divoom_alarm_edit_tf_create_music_item(phandle, phandle->tf_right_panel, NULL, i);
	}
	divoom_alarm_edit_tf_refresh_right_slots(phandle);
}

static lv_obj_t *divoom_alarm_edit_tf_create_folder_btn(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle,
                                                      lv_obj_t *parent, int y, const char *label_text)
{
	lv_obj_t *btn;
	lv_obj_t *folder_img;
	lv_obj_t *label;

	btn = lv_btn_create(parent);
	lv_obj_set_size(btn, 245, 72);
	lv_obj_set_pos(btn, 14, y);
	lv_obj_set_style_pad_all(btn, 0, 0);
	lv_obj_set_style_border_width(btn, 3, 0);
	lv_obj_set_style_border_color(btn, lv_color_hex(0xD5D5D5), 0);
	lv_obj_set_style_border_opa(btn, LV_OPA_100, 0);
	lv_obj_set_style_border_color(btn, lv_color_hex(0x6D6D6D), LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_border_opa(btn, LV_OPA_100, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_radius(btn, 22, 0);
	lv_obj_set_style_shadow_width(btn, 0, 0);
	lv_obj_set_style_bg_opa(btn, 0, 0);
	lv_obj_set_style_outline_width(btn, 0, 0);
	lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_scrollbar_mode(btn, LV_SCROLLBAR_MODE_OFF);
	lv_obj_add_event_cb(btn, divoom_alarm_edit_tf_pick_left_event_cb, LV_EVENT_KEY, phandle);

	folder_img = lv_img_create(btn);
	divoom_menu_control_lv_img_set_src(folder_img, "92/92_folder.png");
	lv_obj_set_pos(folder_img, 10, 20);

	label = lv_label_create(btn);
	lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(20, phandle->menu_id), 0);
	lv_obj_set_style_text_color(label, lv_color_hex(0x0), 0);
	lv_obj_set_scrollbar_mode(label, LV_SCROLLBAR_MODE_OFF);
	lv_obj_set_size(label, 183, 52);
	lv_obj_set_pos(label, 52, 22);
	lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
	lv_label_set_text(label, label_text);
	return btn;
}

static void divoom_alarm_edit_tf_paint_left_selection(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle, int index)
{
	int k;

	if(phandle == NULL)
	{
		return;
	}
	for(k = 0; k < phandle->tf_left_item_count; k++)
	{
		if(phandle->tf_left_folder_btn[k])
		{
			lv_obj_set_style_bg_color(phandle->tf_left_folder_btn[k], lv_color_hex(0xD5D5D5), 0);
			lv_obj_set_style_bg_opa(phandle->tf_left_folder_btn[k], (k == index) ? LV_OPA_100 : 0, 0);
		}
	}
	if(phandle->tf_left_folder_btn[index])
	{
		lv_obj_set_style_bg_color(phandle->tf_left_folder_btn[index], lv_color_hex(0xE4E4E4), 0);
		lv_obj_set_style_bg_opa(phandle->tf_left_folder_btn[index], LV_OPA_100, 0);
	}
}

static void divoom_alarm_edit_tf_save_selection(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle, int global_index)
{
	int slot;
	int track_id;

	if(phandle == NULL || global_index < 0 || global_index >= phandle->tf_music_count)
	{
		return;
	}
	slot = global_index - phandle->tf_right_first_index;
	if(slot < 0 || slot >= phandle->tf_page_n || phandle->tf_page[slot].is_dir)
	{
		return;
	}
	track_id = phandle->tf_page[slot].id;
	if(track_id <= 0)
	{
		return;
	}
	phandle->edit_alarm_info->mode = DIVOOM_ALARM_EDIT_TF_MODE;
	snprintf(phandle->edit_alarm_info->leave_url, sizeof(phandle->edit_alarm_info->leave_url),
	         "%d", track_id);
	divoom_alarm_edit_sound_label_refresh(phandle);
	lv_obj_add_flag(phandle->tf_pick_obj, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(phandle->sound_select_obj, LV_OBJ_FLAG_HIDDEN);
	lv_group_focus_obj(phandle->sound_btn);
}

static void divoom_alarm_edit_tf_pick_right_event_cb(lv_event_t *e)
{
	int i;
	int slot;
	int g;
	int last_g;
	lv_obj_t *cur_obj;
	Pdivoom_lvgl_alarm_edit_menu_ctrl phandle = (Pdivoom_lvgl_alarm_edit_menu_ctrl)lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);

	cur_obj = lv_event_get_target(e);
	if(code != LV_EVENT_KEY || phandle == NULL)
	{
		return;
	}
	{
		char c = *((char *)lv_event_get_param(e));

		for(slot = -1, i = 0; i < DIVOOM_ALARM_EDIT_TF_VISIBLE_ROWS; i++)
		{
			if(phandle->tf_right_music_btn[i] == cur_obj)
			{
				slot = i;
				break;
			}
		}
		if(slot >= 0 && slot < DIVOOM_ALARM_EDIT_TF_VISIBLE_ROWS)
		{
			phandle->tf_right_focus_slot = slot;
		}

		if(c == DIVOOM_LVGL_KEY_ENTER && phandle->tf_page_n > 0 && phandle->tf_music_count > 0)
		{
			g = divoom_alarm_edit_tf_get_global_index(phandle);
			if(g >= 0)
			{
				divoom_alarm_edit_tf_save_selection(phandle, g);
			}
			return;
		}
		if(c == DIVOOM_LVGL_KEY_ESC)
		{
			if(phandle->tf_left_selected_index >= 0
			   && phandle->tf_left_selected_index < phandle->tf_left_item_count
			   && phandle->tf_left_folder_btn[phandle->tf_left_selected_index])
			{
				divoom_alarm_edit_tf_paint_left_selection(phandle, phandle->tf_left_selected_index);
				lv_group_focus_obj(phandle->tf_left_folder_btn[phandle->tf_left_selected_index]);
				lv_obj_scroll_to_view(phandle->tf_left_folder_btn[phandle->tf_left_selected_index], LV_ANIM_OFF);
			}
			return;
		}
		if(c == DIVOOM_LVGL_KEY_PRE)
		{
			if(phandle->tf_music_count <= 0)
			{
				return;
			}
			if(phandle->tf_right_focus_slot > 0)
			{
				phandle->tf_right_focus_slot--;
				if(phandle->tf_right_music_btn[phandle->tf_right_focus_slot])
				{
					lv_group_focus_obj(phandle->tf_right_music_btn[phandle->tf_right_focus_slot]);
					lv_obj_scroll_to_view(phandle->tf_right_music_btn[phandle->tf_right_focus_slot], LV_ANIM_ON);
				}
				return;
			}
			if(phandle->tf_right_first_index > 0)
			{
				phandle->tf_right_first_index--;
				divoom_alarm_edit_tf_refresh_right_slots(phandle);
				return;
			}
			if(phandle->tf_music_count > DIVOOM_ALARM_EDIT_TF_VISIBLE_ROWS)
			{
				phandle->tf_right_first_index = phandle->tf_music_count - DIVOOM_ALARM_EDIT_TF_VISIBLE_ROWS;
				phandle->tf_right_focus_slot = DIVOOM_ALARM_EDIT_TF_VISIBLE_ROWS - 1;
			}
			else
			{
				phandle->tf_right_first_index = 0;
				phandle->tf_right_focus_slot = phandle->tf_music_count - 1;
			}
			divoom_alarm_edit_tf_refresh_right_slots(phandle);
			return;
		}
		if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			if(phandle->tf_music_count <= 0)
			{
				return;
			}
			g = divoom_alarm_edit_tf_get_global_index(phandle);
			last_g = phandle->tf_music_count - 1;
			if(g >= 0 && g < last_g)
			{
				if(phandle->tf_right_focus_slot < DIVOOM_ALARM_EDIT_TF_VISIBLE_ROWS - 1
				   && phandle->tf_right_first_index + phandle->tf_right_focus_slot + 1 < phandle->tf_music_count)
				{
					phandle->tf_right_focus_slot++;
					if(phandle->tf_right_music_btn[phandle->tf_right_focus_slot])
					{
						lv_group_focus_obj(phandle->tf_right_music_btn[phandle->tf_right_focus_slot]);
						lv_obj_scroll_to_view(phandle->tf_right_music_btn[phandle->tf_right_focus_slot], LV_ANIM_ON);
					}
				}
				else
				{
					phandle->tf_right_first_index++;
					divoom_alarm_edit_tf_refresh_right_slots(phandle);
				}
				return;
			}
			phandle->tf_right_first_index = 0;
			phandle->tf_right_focus_slot = 0;
			divoom_alarm_edit_tf_refresh_right_slots(phandle);
		}
	}
}

static void divoom_alarm_edit_tf_pick_left_event_cb(lv_event_t *e)
{
	int i;
	int index = -1;
	lv_obj_t *cur_obj;
	Pdivoom_lvgl_alarm_edit_menu_ctrl phandle = (Pdivoom_lvgl_alarm_edit_menu_ctrl)lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);

	cur_obj = lv_event_get_target(e);
	if(code != LV_EVENT_KEY || phandle == NULL)
	{
		return;
	}
	{
		char c = *((char *)lv_event_get_param(e));

		for(i = 0; i < phandle->tf_left_item_count; i++)
		{
			if(phandle->tf_left_folder_btn[i] != NULL && cur_obj == phandle->tf_left_folder_btn[i])
			{
				index = i;
				break;
			}
		}
		if(index < 0)
		{
			return;
		}
		if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			index = (index + 1) % phandle->tf_left_item_count;
			lv_group_focus_obj(phandle->tf_left_folder_btn[index]);
			lv_obj_scroll_to_view(phandle->tf_left_folder_btn[index], LV_ANIM_ON);
		}
		else if(c == DIVOOM_LVGL_KEY_PRE)
		{
			index = (index - 1 + phandle->tf_left_item_count) % phandle->tf_left_item_count;
			lv_group_focus_obj(phandle->tf_left_folder_btn[index]);
			lv_obj_scroll_to_view(phandle->tf_left_folder_btn[index], LV_ANIM_ON);
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			phandle->tf_left_selected_index = index;
			divoom_alarm_edit_tf_paint_left_selection(phandle, index);
			divoom_alarm_edit_tf_update_music_list(phandle);
			if(phandle->tf_music_count > 0 && phandle->tf_right_music_btn[0])
			{
				lv_group_focus_obj(phandle->tf_right_music_btn[0]);
				lv_obj_scroll_to_view(phandle->tf_right_music_btn[0], LV_ANIM_OFF);
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_alarm_edit_sound_label_refresh(phandle);
			lv_obj_add_flag(phandle->tf_pick_obj, LV_OBJ_FLAG_HIDDEN);
			lv_obj_clear_flag(phandle->sound_select_obj, LV_OBJ_FLAG_HIDDEN);
			lv_group_focus_obj(phandle->sound_select_btn[3]);
			lv_obj_scroll_to_view(phandle->sound_select_btn[3], LV_ANIM_OFF);
		}
	}
}

static void divoom_alarm_edit_tf_rebuild_left(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	int i;
	int index;
	lv_obj_t *left_panel;

	if(phandle == NULL || phandle->tf_left_panel == NULL)
	{
		return;
	}
	left_panel = (lv_obj_t *)phandle->tf_left_panel;
	lv_obj_clean(left_panel);
	for(i = 0; i < DIVOOM_TF_MUSIC_MAX_ROOT + 2; i++)
	{
		phandle->tf_left_folder_btn[i] = NULL;
	}

	phandle->tf_hub_count = divoom_alarm_edit_tf_load_hub(phandle->tf_hub, DIVOOM_TF_MUSIC_MAX_ROOT);
	index = 0;
	if(phandle->tf_hub_count <= 0)
	{
		phandle->tf_left_folder_btn[index++] = divoom_alarm_edit_tf_create_folder_btn(
		    phandle, left_panel, 14, divoom_menu_control_get_disp_str("Allpath"));
		phandle->tf_left_item_count = 1;
	}
	else
	{
		for(i = 0; i < phandle->tf_hub_count && index < DIVOOM_TF_MUSIC_MAX_ROOT + 2; i++)
		{
			phandle->tf_left_folder_btn[index++] = divoom_alarm_edit_tf_create_folder_btn(
			    phandle, left_panel, 14 + 77 * i,
			    phandle->tf_hub[i].name[0] ? phandle->tf_hub[i].name : "");
		}
		phandle->tf_left_item_count = index;
	}
	if(phandle->tf_left_selected_index < 0 || phandle->tf_left_selected_index >= phandle->tf_left_item_count)
	{
		phandle->tf_left_selected_index = 0;
	}
	divoom_alarm_edit_tf_paint_left_selection(phandle, phandle->tf_left_selected_index);
}

static void divoom_alarm_edit_menu_create_tf_pick_page(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle, void *main_obj)
{
	lv_obj_t *left_panel;
	lv_obj_t *title;

	if(phandle == NULL || phandle->tf_pick_obj != NULL)
	{
		return;
	}
	if(!phandle->tf_styles_inited)
	{
		lv_style_init(&phandle->tf_normal_type);
		lv_style_set_bg_color(&phandle->tf_normal_type, lv_color_hex(0xE4E4E4));
		lv_style_set_pad_all(&phandle->tf_normal_type, 0);
		lv_style_set_border_width(&phandle->tf_normal_type, 4);
		lv_style_set_border_color(&phandle->tf_normal_type, lv_color_hex(0xE4E4E4));
		lv_style_set_radius(&phandle->tf_normal_type, 17);
		lv_style_set_shadow_width(&phandle->tf_normal_type, 0);
		lv_style_set_outline_width(&phandle->tf_normal_type, 0);

		lv_style_init(&phandle->tf_press_type);
		lv_style_set_bg_color(&phandle->tf_press_type, lv_color_hex(0xE4E4E4));
		lv_style_set_pad_all(&phandle->tf_press_type, 0);
		lv_style_set_border_width(&phandle->tf_press_type, 4);
		lv_style_set_border_color(&phandle->tf_press_type, lv_color_hex(0x6D6D6D));
		lv_style_set_radius(&phandle->tf_press_type, 17);
		lv_style_set_shadow_width(&phandle->tf_press_type, 0);
		lv_style_set_outline_width(&phandle->tf_press_type, 0);
		phandle->tf_styles_inited = 1;
	}

	phandle->tf_pick_obj = lv_obj_create(main_obj);
	lv_obj_set_style_bg_color(phandle->tf_pick_obj, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_border_width(phandle->tf_pick_obj, 0, 0);
	lv_obj_set_style_pad_all(phandle->tf_pick_obj, 0, 0);
	lv_obj_set_size(phandle->tf_pick_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(phandle->tf_pick_obj, 0, 0);
	lv_obj_add_flag(phandle->tf_pick_obj, LV_OBJ_FLAG_HIDDEN);

	title = lv_label_create(phandle->tf_pick_obj);
	lv_label_set_text(title, divoom_menu_control_get_disp_str("LocalMusic"));
	lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_obj_set_style_text_color(title, lv_color_hex(0x0), 0);
	lv_obj_set_size(title, 400, 60);
	lv_obj_set_pos(title, 200, 25);

	left_panel = lv_obj_create(phandle->tf_pick_obj);
	lv_obj_set_size(left_panel, 273, DIVOOM_ALARM_EDIT_TF_PANEL_H);
	lv_obj_set_pos(left_panel, 10, 90);
	lv_obj_set_style_bg_opa(left_panel, LV_OPA_100, 0);
	lv_obj_set_style_bg_color(left_panel, lv_color_hex(0xD5D5D5), 0);
	lv_obj_set_style_border_width(left_panel, 0, 0);
	lv_obj_set_style_pad_all(left_panel, 0, 0);
	lv_obj_set_style_radius(left_panel, 32, 0);

	phandle->tf_left_panel = left_panel;
	divoom_alarm_edit_tf_rebuild_left(phandle);

	phandle->tf_left_selected_index = 0;
	phandle->tf_right_first_index = 0;
	phandle->tf_right_focus_slot = 0;

	phandle->tf_right_panel = lv_obj_create(phandle->tf_pick_obj);
	lv_obj_set_size(phandle->tf_right_panel, 507, DIVOOM_ALARM_EDIT_TF_PANEL_H);
	lv_obj_set_pos(phandle->tf_right_panel, 293, 80);
	lv_obj_set_style_bg_opa(phandle->tf_right_panel, LV_OPA_100, 0);
	lv_obj_set_style_bg_color(phandle->tf_right_panel, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_border_width(phandle->tf_right_panel, 0, 0);
	lv_obj_set_style_radius(phandle->tf_right_panel, 0, 0);
	lv_obj_set_style_pad_all(phandle->tf_right_panel, 0, 0);
	lv_obj_set_style_pad_bottom(phandle->tf_right_panel, 20, 0);
	lv_obj_set_style_pad_top(phandle->tf_right_panel, 20, 0);
	lv_obj_set_scroll_dir(phandle->tf_right_panel, LV_DIR_VER);
	lv_obj_set_scrollbar_mode(phandle->tf_right_panel, LV_SCROLLBAR_MODE_OFF);
	lv_obj_set_flex_flow(phandle->tf_right_panel, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(phandle->tf_right_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

	divoom_alarm_edit_tf_paint_left_selection(phandle, 0);
}

static lv_obj_t *divoom_alarm_edit_menu_get_main_focus_btn(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	if(phandle == NULL)
	{
		return NULL;
	}
	return (lv_obj_t *)phandle->main_default_focus_obj;
}

static void divoom_alarm_edit_menu_restore_main_focus(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	lv_obj_t *focus_btn;

	focus_btn = divoom_alarm_edit_menu_get_main_focus_btn(phandle);
	if(focus_btn)
	{
		lv_group_focus_obj(focus_btn);
		lv_obj_scroll_to_view(focus_btn, LV_ANIM_OFF);
	}
}

static void divoom_alarm_edit_menu_restore_main_focus_if_main(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	if(phandle == NULL)
	{
		return;
	}
	if(phandle->sound_select_obj && !lv_obj_has_flag(phandle->sound_select_obj, LV_OBJ_FLAG_HIDDEN))
	{
		return;
	}
	if(phandle->tf_pick_obj && !lv_obj_has_flag(phandle->tf_pick_obj, LV_OBJ_FLAG_HIDDEN))
	{
		return;
	}
	if(phandle->bells_obj && !lv_obj_has_flag(phandle->bells_obj, LV_OBJ_FLAG_HIDDEN))
	{
		return;
	}
	if(phandle->scene_list_obj && !lv_obj_has_flag(phandle->scene_list_obj, LV_OBJ_FLAG_HIDDEN))
	{
		return;
	}
	if(phandle->record_obj && !lv_obj_has_flag(phandle->record_obj, LV_OBJ_FLAG_HIDDEN))
	{
		return;
	}
	if(phandle->display_select_obj && !lv_obj_has_flag(phandle->display_select_obj, LV_OBJ_FLAG_HIDDEN))
	{
		return;
	}
	divoom_alarm_edit_menu_restore_main_focus(phandle);
}
#endif


void divoom_alarm_edit_menu_set_event_cb(lv_event_t * e)
{
	int i, active_id;
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *cur_btn_obj = lv_event_get_target(e);
	Pdivoom_lvgl_alarm_edit_menu_ctrl phandle = (Pdivoom_lvgl_alarm_edit_menu_ctrl)lv_event_get_user_data(e);

	/*switch(code)
	{
		case LV_EVENT_CLICKED:
			if(cur_btn_obj == phandle->repeat_btn)
			{
				phandle->active_id = 2;
				lv_obj_clear_flag(phandle->bg_obj, LV_OBJ_FLAG_HIDDEN);
				lv_obj_clear_flag(phandle->week_list_obj, LV_OBJ_FLAG_HIDDEN);
			}
			else if(cur_btn_obj == phandle->sound_btn)
			{
				phandle->active_id = 3;
				lv_obj_clear_flag(phandle->bells_obj, LV_OBJ_FLAG_HIDDEN);
				lv_group_focus_obj(phandle->cur_active_obj);
			}
            break;
        default:
            break;
	}*/
	if(code == LV_EVENT_KEY)
	{
		char c = *((char *)lv_event_get_param(e));

		if(c == DIVOOM_LVGL_KEY_PRE)
		{
			if(phandle->btn_status == 0)
			{
				cur_btn_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_btn_obj, DIVOOM_MENU_DIRECT_RIGHT);
				lv_group_focus_obj(cur_btn_obj);
			}
			else
			{
				active_id = divoom_menu_direct_ctrl_get_pos(phandle->pdirect_handle, cur_btn_obj);
            	if(divoom_main_get_time_flag() != 0)
            	{
              		active_id ++;
            	}
				if(active_id == 0)//am pm
				{
					//消抖
					if(phandle->am_pm_flag)
                	{
                    	phandle->am_pm_flag = 0;
                    	lv_label_set_text(phandle->am_pm_disp_label, divoom_menu_control_get_disp_str("AM"));
                	}
                	else
                	{
                    	phandle->am_pm_flag = 1;
                    	lv_label_set_text(phandle->am_pm_disp_label, divoom_menu_control_get_disp_str("PM"));
                	}
				}
				else if(active_id == 1)//小时
				{
					if(divoom_main_get_time_flag() == 0)
                	{
                   		if(phandle->hour >= 12)
                    	{
                        	phandle->hour = 1;
                    	}
                    	else
                   		{
                        	phandle->hour ++;
                    	}
                	}
                	else
                	{
                    	if(phandle->hour >= 23)
                    	{
                        	phandle->hour = 0;
                    	}
                    	else
                    	{
                        	phandle->hour ++;
                    	}
                	}
                	lv_label_set_text_fmt(phandle->hour_disp_label, "%02d", phandle->hour);
				}
				else if(active_id == 2)//分钟
				{
					if(phandle->minute >= 59)
                	{
                   		phandle->minute = 0;
                	}
                	else
                	{
                    	phandle->minute ++;
                	}
                	lv_label_set_text_fmt(phandle->minute_disp_label, "%02d", phandle->minute);
				}
#ifdef DIVOOM_TIME_FRAME_SPEAKER
				else if(active_id == divoom_alarm_edit_menu_active_volume_id())
#else
				else if(active_id == 6)
#endif
				{
					int value = lv_slider_get_value(phandle->vol_slide_obj);
					value = (value < 95) ? (value + 5) : (100);
					lv_slider_set_value(phandle->vol_slide_obj, value, LV_ANIM_ON);
					phandle->edit_alarm_info->volume = value;
#ifdef DIVOOM_TIME_FRAME_SPEAKER
					divoom_alarm_edit_preview_selected_sound(phandle);
#endif
				}
			}
		}
		else if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			if(phandle->btn_status == 0)
			{
				cur_btn_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_btn_obj, DIVOOM_MENU_DIRECT_LEFT);
				lv_group_focus_obj(cur_btn_obj);
			}
			else
			{
				active_id = divoom_menu_direct_ctrl_get_pos(phandle->pdirect_handle, cur_btn_obj);
            	if(divoom_main_get_time_flag() != 0)
            	{
              	  	active_id ++;
            	}
				if(active_id == 0)//am pm
				{
					//消抖
					if(phandle->am_pm_flag)
                	{
                    	phandle->am_pm_flag = 0;
                    	lv_label_set_text(phandle->am_pm_disp_label, divoom_menu_control_get_disp_str("AM"));
                	}
                	else
                	{
                    	phandle->am_pm_flag = 1;
                    	lv_label_set_text(phandle->am_pm_disp_label, divoom_menu_control_get_disp_str("PM"));
                	}
				}
				else if(active_id == 1)//小时
				{
					if(divoom_main_get_time_flag() == 0)
               		{
                   		if(phandle->hour <= 1)
                    	{
                      		phandle->hour = 12;
                    	}
                    	else
                    	{
                    	    phandle->hour --;
                    	}
                	}
                	else
                	{
                   		if(phandle->hour <= 0)
                    	{
                   		   	phandle->hour = 23;
                    	}
                    	else
                    	{
                        	phandle->hour --;
                   	 	}
                	}	
                	lv_label_set_text_fmt(phandle->hour_disp_label, "%02d", phandle->hour);
				}
				else if(active_id == 2)//分钟
				{
					if(phandle->minute <= 0)
                	{
                    	phandle->minute = 59;
                	}
                	else
                	{
                	    phandle->minute --;
                	}
                	lv_label_set_text_fmt(phandle->minute_disp_label, "%02d", phandle->minute);
				}
#ifdef DIVOOM_TIME_FRAME_SPEAKER
				else if(active_id == divoom_alarm_edit_menu_active_volume_id())
#else
				else if(active_id == 6)
#endif
				{
					int value = lv_slider_get_value(phandle->vol_slide_obj);
					value = (value > 5) ? (value - 5) : (0);
					lv_slider_set_value(phandle->vol_slide_obj, value, LV_ANIM_ON);
					phandle->edit_alarm_info->volume = value;
#ifdef DIVOOM_TIME_FRAME_SPEAKER
					divoom_alarm_edit_preview_selected_sound(phandle);
#endif
				}
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			if(phandle->btn_status == 0)
			{
				active_id = divoom_menu_direct_ctrl_get_pos(phandle->pdirect_handle, cur_btn_obj);
				//phandle->btn_status = 1;
				if(divoom_main_get_time_flag() != 0)
            	{
              	  	active_id ++;
            	}
#ifdef DIVOOM_TIME_FRAME_SPEAKER
				if(active_id < 3 || active_id == divoom_alarm_edit_menu_active_volume_id())
#else
				if(active_id < 3 || active_id == 6)
#endif
				{
					phandle->btn_status = 1;
					lv_obj_add_style(cur_btn_obj, &phandle->select_press_type, LV_STATE_FOCUSED);
					lv_obj_add_style(cur_btn_obj, &phandle->select_press_type, LV_STATE_FOCUS_KEY);
#ifdef DIVOOM_TIME_FRAME_SPEAKER
					if(active_id == divoom_alarm_edit_menu_active_volume_id())
					{
						divoom_alarm_edit_preview_selected_sound(phandle);
					}
#endif
				}
				else if(active_id == 3)
				{
					if(phandle->edit_alarm_info->on_off)
					{
						phandle->edit_alarm_info->on_off = 0;
						divoom_menu_control_lv_img_set_src(phandle->alarm_onoff_img, "23/23_off.png");
					}
					else
					{
						phandle->edit_alarm_info->on_off = 1;
						divoom_menu_control_lv_img_set_src(phandle->alarm_onoff_img, "23/23_on.png");
					}
				}
				else if(active_id == 4)
				{
					lv_obj_clear_flag(phandle->bg_obj, LV_OBJ_FLAG_HIDDEN);
					lv_obj_clear_flag(phandle->week_list_obj, LV_OBJ_FLAG_HIDDEN);
					lv_group_focus_obj(phandle->weekday_btn_obj[0]);
				}
				else if(active_id == 5)
				{
#ifdef DIVOOM_TIME_FRAME_SPEAKER
					divoom_alarm_edit_menu_ensure_sub_pages(phandle);
					divoom_alarm_edit_sound_select_apply_tf_state(phandle);
					lv_obj_clear_flag(phandle->sound_select_obj, LV_OBJ_FLAG_HIDDEN);
					{
						lv_obj_t *focus_btn = phandle->sound_select_btn[0];

						if(phandle->edit_alarm_info->mode == 4)
						{
							focus_btn = phandle->sound_select_btn[1];
						}
						else if(phandle->edit_alarm_info->mode == 1)
						{
							focus_btn = phandle->sound_select_btn[2];
						}
						else if(phandle->edit_alarm_info->mode == DIVOOM_ALARM_EDIT_TF_MODE
						        && divoom_alarm_edit_tf_card_available())
						{
							focus_btn = phandle->sound_select_btn[3];
						}
						if(focus_btn)
						{
							lv_group_focus_obj(focus_btn);
							lv_obj_scroll_to_view(focus_btn, LV_ANIM_OFF);
						}
					}
#else
					lv_obj_clear_flag(phandle->bells_obj, LV_OBJ_FLAG_HIDDEN);
					lv_group_focus_obj(phandle->cur_active_obj);
#endif
				}
#ifdef DIVOOM_TIME_FRAME_SPEAKER
				else if(active_id == 6)
				{
					divoom_alarm_edit_menu_ensure_sub_pages(phandle);
					lv_obj_clear_flag(phandle->display_select_obj, LV_OBJ_FLAG_HIDDEN);
					if(phandle->display_select_btn[phandle->display_index])
					{
						lv_group_focus_obj(phandle->display_select_btn[phandle->display_index]);
						lv_obj_scroll_to_view(phandle->display_select_btn[phandle->display_index], LV_ANIM_OFF);
					}
				}
#endif
#ifdef DIVOOM_TIME_FRAME_SPEAKER
				else if(active_id == divoom_alarm_edit_menu_active_save_id())
#else
				else if(active_id == 7)
#endif
				{
					//保存
					phandle->edit_alarm_info->hour = phandle->hour;
					//if(divoom_main_get_time_flag() == 0)
					//{
					//	phandle->edit_alarm_info->hour = phandle->am_pm_flag ? (phandle->edit_alarm_info->hour + 12) : (phandle->edit_alarm_info->hour);
					//}
					phandle->edit_alarm_info->hour = divoom_alarm_edit_menu_convert_to_twenty_four_hour(phandle->edit_alarm_info->hour, phandle->am_pm_flag, divoom_main_get_time_flag());
					phandle->edit_alarm_info->minute = phandle->minute;
#ifdef DIVOOM_TIME_FRAME_SPEAKER
					divoom_alarm_edit_display_index_save(phandle);
#endif
					//phandle->edit_alarm_info->on_off = 1;//修改默认开启
					
					//divoom_print("hour = %d,minute = %d,cyc = 0x%x,tip_index = %d\n", phandle->edit_alarm_info->hour, phandle->edit_alarm_info->minute,phandle->edit_alarm_info->cyc_flag,phandle->edit_alarm_info->tip_index);
					memcpy(phandle->cur_alarm_info, phandle->edit_alarm_info, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
            		divoom_alarm_menu_update_alarm_item(phandle->cur_alarm_info);
					divoom_menu_control_return_pre();
				}
			}
			else
			{
				active_id = divoom_menu_direct_ctrl_get_pos(phandle->pdirect_handle, cur_btn_obj);
				if(divoom_main_get_time_flag() != 0)
            	{
              	  	active_id ++;
            	}
				if(active_id == 0)//am pm
				{
					if(phandle->am_pm_flag)
                	{
                    	phandle->am_pm_flag = 0;
                    	lv_label_set_text(phandle->am_pm_disp_label, divoom_menu_control_get_disp_str("AM"));
                	}
                	else
                	{
                    	phandle->am_pm_flag = 1;
                    	lv_label_set_text(phandle->am_pm_disp_label, divoom_menu_control_get_disp_str("PM"));
                	}
				}
				else if(active_id == 1)//小时
				{
					if(divoom_main_get_time_flag() == 0)
                	{
                   		if(phandle->hour >= 12)
                    	{
                        	phandle->hour = 1;
                    	}
                    	else
                   		{
                        	phandle->hour ++;
                    	}
                	}
                	else
                	{
                    	if(phandle->hour >= 23)
                    	{
                        	phandle->hour = 0;
                    	}
                    	else
                    	{
                        	phandle->hour ++;
                    	}
                	}
                	lv_label_set_text_fmt(phandle->hour_disp_label, "%02d", phandle->hour);
				}
				else if(active_id == 2)//分钟
				{
					if(phandle->minute >= 59)
                	{
                   		phandle->minute = 0;
                	}
                	else
                	{
                    	phandle->minute ++;
                	}
                	lv_label_set_text_fmt(phandle->minute_disp_label, "%02d", phandle->minute);
				}
#ifdef DIVOOM_TIME_FRAME_SPEAKER
				else if(active_id == divoom_alarm_edit_menu_active_volume_id())
#else
				else if(active_id == 6)//音量
#endif
				{
					int value = lv_slider_get_value(phandle->vol_slide_obj);
					value = (value < 95) ? (value + 5) : (100);
					lv_slider_set_value(phandle->vol_slide_obj, value, LV_ANIM_ON);
					phandle->edit_alarm_info->volume = value;
#ifdef DIVOOM_TIME_FRAME_SPEAKER
					divoom_alarm_edit_preview_selected_sound(phandle);
#endif
				}
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			if(phandle->btn_status == 0)
			{
				divoom_menu_control_return_pre();
			}
			else
			{
				phandle->btn_status = 0;
				lv_obj_add_style(cur_btn_obj, &phandle->press_type, LV_STATE_FOCUSED);
                lv_obj_add_style(cur_btn_obj, &phandle->press_type, LV_STATE_FOCUS_KEY);
#ifdef DIVOOM_TIME_FRAME_SPEAKER
				divoom_alarm_edit_preview_stop_all(phandle);
#endif
			}
		}
		else
		{
			char *key_info = (char *)lv_event_get_param(e);
			divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
			return;
		}
	}
}


void divoom_alarm_edit_menu_set_repeat_label_text(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	int i, index = 0;
	char disp_str[32];
	char weekday[7][5] = {"Mo","Tu","We","Th","Fr","Sa","Su"};

	for(i = 0;i < 7; i++)
	{
		if(phandle->edit_alarm_info->cyc_flag & (1 << ((i + 1)%7)))
        {
          	sprintf(disp_str + index, "%s ", weekday[i]);
			index += 3;
       	}
	}
	if(index > 0)
	{
		disp_str[index - 1] = '\0';
		lv_label_set_text(phandle->cys_label_obj, divoom_menu_control_get_disp_str(disp_str));
	}
	else
	{
		lv_label_set_text(phandle->cys_label_obj, divoom_menu_control_get_disp_str("Never"));
	}
	
}


void divoom_alarm_edit_menu_week_list_event_cb(lv_event_t * e)
{
	int i;
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *cur_btn_obj = lv_event_get_target(e);
	Pdivoom_lvgl_alarm_edit_menu_ctrl phandle = (Pdivoom_lvgl_alarm_edit_menu_ctrl)lv_event_get_user_data(e);

	if(code == LV_EVENT_KEY)
	{
		char c = *((char *)lv_event_get_param(e));

		if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			lv_group_focus_obj(divoom_menu_direct_ctrl_get(phandle->pdirect_repeat_handle, cur_btn_obj, DIVOOM_MENU_DIRECT_UP));
		}
		else if(c == DIVOOM_LVGL_KEY_PRE)
		{
			lv_group_focus_obj(divoom_menu_direct_ctrl_get(phandle->pdirect_repeat_handle, cur_btn_obj, DIVOOM_MENU_DIRECT_DOWN));
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			for(i = 0;i < 7; i++)
			{
				if(phandle->weekday_btn_obj[i] == cur_btn_obj)
				{
					if(phandle->edit_alarm_info->cyc_flag & (1 << ((i+1)%7)))
           			{
               			 phandle->edit_alarm_info->cyc_flag &= ~ (1 << ((i+1)%7));
               			 lv_obj_add_flag(phandle->weekday_image_obj[i], LV_OBJ_FLAG_HIDDEN);
            		}
            		else
            		{
                		phandle->edit_alarm_info->cyc_flag |= (1 << ((i+1)%7));
                		lv_obj_clear_flag(phandle->weekday_image_obj[i], LV_OBJ_FLAG_HIDDEN);
            		}
					break;
				}
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			lv_obj_add_flag(phandle->bg_obj, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(phandle->week_list_obj, LV_OBJ_FLAG_HIDDEN);
			divoom_alarm_edit_menu_set_repeat_label_text(phandle);
			lv_group_focus_obj(phandle->repeat_btn);
		}
	}
	
}


void divoom_alarm_edit_menu_create_week_list(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle, void *main_obj)
{
	int i;
	const char week_day[7][16] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};

	phandle->week_list_obj = lv_obj_create(main_obj);
	lv_obj_set_size(phandle->week_list_obj, 640, 822);
	lv_obj_set_style_pad_all(phandle->week_list_obj, 0, 0);
	lv_obj_set_style_radius(phandle->week_list_obj, 32, 0);
	lv_obj_set_style_border_width(phandle->week_list_obj, 0, 0);
	lv_obj_set_style_bg_color(phandle->week_list_obj, lv_color_hex(0xF2F2F2), 0);
	lv_obj_clear_flag(phandle->week_list_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_pos(phandle->week_list_obj, 80, 229);

	//lv_group_t *week_group = lv_group_create();
	for(i = 0;i < 7;i++)
	{
		lv_obj_t *weekday_btn = lv_btn_create(phandle->week_list_obj);
		lv_obj_remove_style_all(weekday_btn);
		lv_obj_set_style_pad_all(weekday_btn, 0, 0);
		lv_obj_set_style_bg_opa(weekday_btn, LV_OPA_0, 0);
		lv_obj_set_style_shadow_width(weekday_btn, 0, 0);
		lv_obj_set_style_border_width(weekday_btn, 4, 0);
		lv_obj_set_style_border_color(weekday_btn, lv_color_hex(0xF2F2F2), 0);
		lv_obj_set_style_border_color(weekday_btn, lv_color_hex(0x6D6D6D), LV_STATE_FOCUSED);
		lv_obj_set_style_border_color(weekday_btn, lv_color_hex(0x6D6D6D), LV_STATE_FOCUS_KEY);
		lv_obj_set_style_radius(weekday_btn, 25, 0);
		lv_obj_set_size(weekday_btn, 576, 98);
		lv_obj_set_pos(weekday_btn, 32, 32+i*110);
		divoom_menu_direct_ctrl_update(phandle->pdirect_repeat_handle, weekday_btn, 0, i);
		lv_obj_add_event_cb(weekday_btn, divoom_alarm_edit_menu_week_list_event_cb, LV_EVENT_KEY, phandle);
		phandle->weekday_btn_obj[i] = weekday_btn;
		//lv_group_add_obj(week_group, weekday_btn);

		lv_obj_t *disp_weekday = lv_label_create(weekday_btn);
		lv_obj_set_size(disp_weekday, 504, 68);
		lv_obj_set_pos(disp_weekday, 36, 15+10);
		lv_obj_set_style_text_color(disp_weekday, lv_color_hex(0x0), 0);
		lv_obj_set_style_text_font(disp_weekday, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
		lv_obj_set_style_text_align(disp_weekday,LV_TEXT_ALIGN_LEFT, 0);
		lv_label_set_text(disp_weekday, divoom_menu_control_get_disp_str(week_day[i]));

		lv_obj_t *weekday_img = lv_img_create(weekday_btn);
		divoom_menu_control_lv_img_set_src(weekday_img, "49/49_select.png");
		lv_obj_set_size(weekday_img, 36, 36);
		lv_obj_set_pos(weekday_img, 504, 31-5);

		if(phandle->edit_alarm_info->cyc_flag & (1 << ((i+1)%7)))
		{
			lv_obj_clear_flag(weekday_img, LV_OBJ_FLAG_HIDDEN);
		}
		else
		{
			lv_obj_add_flag(weekday_img, LV_OBJ_FLAG_HIDDEN);
		}
		phandle->weekday_image_obj[i] = weekday_img;
		
	}
	lv_obj_add_flag(phandle->week_list_obj, LV_OBJ_FLAG_HIDDEN);
}

static int divoom_alarm_edit_bell_tip_index_from_obj(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle, lv_obj_t *obj)
{
	int i;

	if(phandle == NULL || obj == NULL)
	{
		return -1;
	}
	for(i = 0; i < 12; i++)
	{
		if(obj == phandle->bell_cont_obj[i])
		{
			return i;
		}
	}
	return -1;
}

static void divoom_alarm_edit_bell_preview_stop(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
#ifdef DIVOOM_TIME_FRAME_SPEAKER
	divoom_alarm_edit_preview_stop_all(phandle);
#else
	divoom_alarm_ctrl_preview_stop();
	(void)phandle;
#endif
}

static void divoom_alarm_edit_bell_preview_play(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle, int tip_index)
{
	if(phandle == NULL || phandle->edit_alarm_info == NULL || tip_index < 0 || tip_index >= 12)
	{
		return;
	}
	divoom_alarm_ctrl_try_listen((T_U8)tip_index, phandle->edit_alarm_info->volume, 1, 1);
}

static void divoom_alarm_edit_bell_close_page(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	if(phandle == NULL)
	{
		return;
	}
	divoom_alarm_edit_bell_preview_stop(phandle);
	lv_obj_add_flag(phandle->bells_obj, LV_OBJ_FLAG_HIDDEN);
#ifdef DIVOOM_TIME_FRAME_SPEAKER
	divoom_alarm_edit_sound_label_refresh(phandle);
	divoom_alarm_edit_sound_select_apply_tf_state(phandle);
	lv_obj_clear_flag(phandle->sound_select_obj, LV_OBJ_FLAG_HIDDEN);
	lv_group_focus_obj(phandle->sound_select_btn[0]);
	lv_obj_scroll_to_view(phandle->sound_select_btn[0], LV_ANIM_OFF);
#else
	lv_group_focus_obj(phandle->sound_btn);
#endif
}

void divoom_alarm_edit_bell_menu_event_cb(lv_event_t * e)
{
	int tip_index;
	lv_obj_t *cur_obj;
	lv_obj_t *next_obj;
    Pdivoom_lvgl_alarm_edit_menu_ctrl phandle = (Pdivoom_lvgl_alarm_edit_menu_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target(e);
	
    if(code == LV_EVENT_KEY)
	{
		char c = *((char *)lv_event_get_param(e));

		if(c == DIVOOM_LVGL_KEY_PRE)
		{
			next_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_music_handle, cur_obj, DIVOOM_MENU_DIRECT_RIGHT);
			if(next_obj && next_obj != cur_obj)
			{
				lv_group_focus_obj(next_obj);
				lv_obj_scroll_to_view(next_obj, LV_ANIM_ON);
				divoom_alarm_edit_bell_preview_play(phandle,
				                                    divoom_alarm_edit_bell_tip_index_from_obj(phandle, next_obj));
			}
		}
		else if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			next_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_music_handle, cur_obj, DIVOOM_MENU_DIRECT_LEFT);
			if(next_obj && next_obj != cur_obj)
			{
				lv_group_focus_obj(next_obj);
				lv_obj_scroll_to_view(next_obj, LV_ANIM_ON);
				divoom_alarm_edit_bell_preview_play(phandle,
				                                    divoom_alarm_edit_bell_tip_index_from_obj(phandle, next_obj));
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_alarm_edit_bell_close_page(phandle);
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			tip_index = divoom_alarm_edit_bell_tip_index_from_obj(phandle, cur_obj);
			if(tip_index >= 0)
			{
				phandle->cur_active_obj = cur_obj;
				phandle->edit_alarm_info->tip_index = (T_U8)tip_index;
				phandle->edit_alarm_info->mode = 0;
				divoom_alarm_edit_bell_close_page(phandle);
			}
		}
		else
		{
			return;
		}
	}

}
#ifdef DIVOOM_TIME_FRAME_SPEAKER
static int divoom_alarm_edit_tf_card_available(void)
{
	return divoom_alarm_tf_sound_card_inserted();
}

static int divoom_alarm_edit_enter_tf_pick(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	lv_obj_t *focus_left;

	if(phandle == NULL || phandle->tf_pick_obj == NULL || !divoom_alarm_edit_tf_card_available())
	{
		return -1;
	}

	divoom_tf_music_send_get_music_count();
	phandle->tf_left_selected_index = 0;
	divoom_alarm_edit_tf_rebuild_left(phandle);
	divoom_alarm_edit_tf_update_music_list(phandle);
	lv_obj_add_flag(phandle->sound_select_obj, LV_OBJ_FLAG_HIDDEN);
	lv_obj_clear_flag(phandle->tf_pick_obj, LV_OBJ_FLAG_HIDDEN);
	lv_obj_move_foreground(phandle->tf_pick_obj);
	focus_left = phandle->tf_left_folder_btn[0];
	if(focus_left)
	{
		lv_group_focus_obj(focus_left);
		lv_obj_scroll_to_view(focus_left, LV_ANIM_OFF);
	}
	return 0;
}

static void divoom_alarm_edit_sound_select_apply_tf_state(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	int tf_ok;
	lv_obj_t *tf_btn;

	if(phandle == NULL)
	{
		return;
	}
	tf_ok = divoom_alarm_edit_tf_card_available();
	if(!tf_ok && phandle->tf_pick_obj && !lv_obj_has_flag(phandle->tf_pick_obj, LV_OBJ_FLAG_HIDDEN))
	{
		lv_obj_add_flag(phandle->tf_pick_obj, LV_OBJ_FLAG_HIDDEN);
		if(phandle->sound_select_obj)
		{
			lv_obj_clear_flag(phandle->sound_select_obj, LV_OBJ_FLAG_HIDDEN);
		}
	}
	tf_btn = phandle->sound_select_btn[3];
	if(tf_btn)
	{
		if(tf_ok)
		{
			lv_obj_clear_state(tf_btn, LV_STATE_DISABLED);
			lv_obj_set_style_opa(tf_btn, LV_OPA_COVER, 0);
			lv_obj_add_flag(tf_btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);
			lv_obj_remove_event_cb(tf_btn, divoom_alarm_edit_sound_select_menu_event_cb);
			lv_obj_add_event_cb(tf_btn, divoom_alarm_edit_sound_select_menu_event_cb, LV_EVENT_KEY, phandle);
		}
		else
		{
			lv_obj_add_state(tf_btn, LV_STATE_DISABLED);
			lv_obj_set_style_opa(tf_btn, LV_OPA_30, 0);
			lv_obj_clear_flag(tf_btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);
			lv_obj_remove_event_cb(tf_btn, divoom_alarm_edit_sound_select_menu_event_cb);
		}
	}
	if(phandle->pdirect_sound_select_handle)
	{
		divoom_menu_direct_ctrl_destory(phandle->pdirect_sound_select_handle);
		phandle->pdirect_sound_select_handle = NULL;
	}
	phandle->pdirect_sound_select_handle = divoom_menu_direct_ctrl_create(1, tf_ok ? 4 : 3);
	if(phandle->pdirect_sound_select_handle == NULL)
	{
		return;
	}
	if(phandle->sound_select_btn[0])
	{
		divoom_menu_direct_ctrl_update(phandle->pdirect_sound_select_handle, phandle->sound_select_btn[0], 0, 0);
	}
	if(phandle->sound_select_btn[1])
	{
		divoom_menu_direct_ctrl_update(phandle->pdirect_sound_select_handle, phandle->sound_select_btn[1], 0, 1);
	}
	if(phandle->sound_select_btn[2])
	{
		divoom_menu_direct_ctrl_update(phandle->pdirect_sound_select_handle, phandle->sound_select_btn[2], 0, 2);
	}
	if(tf_ok && phandle->sound_select_btn[3])
	{
		divoom_menu_direct_ctrl_update(phandle->pdirect_sound_select_handle, phandle->sound_select_btn[3], 0, 3);
	}
}

static lv_obj_t *divoom_alarm_edit_create_sound_list_btn(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle,
                                                        lv_obj_t *parent, int idx, int y,
                                                        const char *title_key, const char *desp_key,
                                                        uint8 disabled)
{
	lv_obj_t *btn;
	lv_obj_t *title;
	lv_obj_t *desp;
	lv_obj_t *arrow;

	btn = lv_btn_create(parent);
	lv_obj_set_style_pad_all(btn, 0, 0);
	lv_obj_set_style_pad_gap(btn, 0, 0);
	lv_obj_set_style_bg_color(btn, lv_color_hex(0xF2F2F2), 0);
	lv_obj_set_style_radius(btn, 17, 0);
	lv_obj_set_style_border_width(btn, 4, 0);
	lv_obj_set_style_outline_width(btn, 0, 0);
	lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_shadow_width(btn, 0, 0);
	lv_obj_set_style_border_color(btn, lv_color_hex(0xF2F2F2), 0);
	lv_obj_set_style_border_color(btn, lv_color_hex(0xE89B00), LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_size(btn, 712, 120);
	lv_obj_set_pos(btn, 44, y);
	if(disabled)
	{
		lv_obj_add_state(btn, LV_STATE_DISABLED);
		lv_obj_set_style_opa(btn, LV_OPA_30, 0);
		lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);
	}
	else
	{
		lv_obj_add_event_cb(btn, divoom_alarm_edit_sound_select_menu_event_cb, LV_EVENT_KEY, phandle);
	}
	phandle->sound_select_btn[idx] = btn;

	title = lv_label_create(btn);
	lv_obj_set_size(title, 400, 34);
	lv_obj_set_pos(title, 32, 28);
	lv_obj_set_style_text_color(title, lv_color_black(), 0);
	lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
	lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_LEFT, 0);
	lv_label_set_text(title, divoom_menu_control_get_disp_str(title_key));

	desp = lv_label_create(btn);
	lv_obj_set_size(desp, 560, 26);
	lv_obj_set_pos(desp, 32, 66);
	lv_obj_set_style_text_color(desp, lv_color_hex(0x5B5B5B), 0);
	lv_obj_set_style_text_font(desp, divoom_menu_control_get_font_info(20, phandle->menu_id), 0);
	lv_obj_set_style_text_align(desp, LV_TEXT_ALIGN_LEFT, 0);
	lv_label_set_text(desp, divoom_menu_control_get_disp_str(desp_key));

	if(!disabled)
	{
		arrow = lv_img_create(btn);
		lv_obj_set_pos(arrow, 644, 42);
		divoom_menu_control_lv_img_set_src(arrow, "49/49_right.png");
	}
	return btn;
}

void divoom_alarm_edit_sound_select_menu_event_cb(lv_event_t * e)
{
	int i;
	lv_obj_t *cur_obj;
	lv_obj_t *next_obj;
	Pdivoom_lvgl_alarm_edit_menu_ctrl phandle = (Pdivoom_lvgl_alarm_edit_menu_ctrl)lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);

	cur_obj = lv_event_get_target(e);
	if(code != LV_EVENT_KEY)
	{
		return;
	}
	{
		char c = *((char *)lv_event_get_param(e));

		if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			next_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_sound_select_handle, cur_obj, DIVOOM_MENU_DIRECT_UP);
			if(next_obj && next_obj != cur_obj)
			{
				lv_group_focus_obj(next_obj);
				lv_obj_scroll_to_view(next_obj, LV_ANIM_OFF);
			}
		}
		else if(c == DIVOOM_LVGL_KEY_PRE)
		{
			next_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_sound_select_handle, cur_obj, DIVOOM_MENU_DIRECT_DOWN);
			if(next_obj && next_obj != cur_obj)
			{
				lv_group_focus_obj(next_obj);
				lv_obj_scroll_to_view(next_obj, LV_ANIM_OFF);
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_alarm_edit_sound_label_refresh(phandle);
			lv_obj_add_flag(phandle->sound_select_obj, LV_OBJ_FLAG_HIDDEN);
			lv_group_focus_obj(phandle->sound_btn);
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			for(i = 0; i < DIVOOM_ALARM_EDIT_SOUND_ITEM_COUNT; i++)
			{
				if(cur_obj == phandle->sound_select_btn[i])
				{
					break;
				}
			}
			if(i == 3 && !divoom_alarm_edit_tf_card_available())
			{
				return;
			}
			if(i >= DIVOOM_ALARM_EDIT_SOUND_ITEM_COUNT)
			{
				return;
			}
			divoom_alarm_edit_menu_ensure_sub_pages(phandle);
			if(i == 0)
			{
				lv_obj_t *focus_bell;

				if(phandle->cur_active_obj == NULL || phandle->edit_alarm_info->tip_index < 0
				   || phandle->edit_alarm_info->tip_index >= 12)
				{
					phandle->cur_active_obj = phandle->bell_cont_obj[0];
				}
				focus_bell = phandle->cur_active_obj;
				if(focus_bell == NULL)
				{
					focus_bell = phandle->bell_cont_obj[0];
				}
				lv_obj_add_flag(phandle->sound_select_obj, LV_OBJ_FLAG_HIDDEN);
				lv_obj_clear_flag(phandle->bells_obj, LV_OBJ_FLAG_HIDDEN);
				lv_obj_move_foreground(phandle->bells_obj);
				if(focus_bell)
				{
					lv_group_focus_obj(focus_bell);
					lv_obj_scroll_to_view(focus_bell, LV_ANIM_OFF);
					divoom_alarm_edit_bell_preview_play(phandle,
					                                    divoom_alarm_edit_bell_tip_index_from_obj(phandle, focus_bell));
				}
			}
			else if(i == 1)
			{
				int focus_row;
				lv_obj_t *focus_scene;

				divoom_alarm_edit_scene_refresh_selection(phandle);
				lv_obj_add_flag(phandle->sound_select_obj, LV_OBJ_FLAG_HIDDEN);
				lv_obj_clear_flag(phandle->scene_list_obj, LV_OBJ_FLAG_HIDDEN);
				lv_obj_move_foreground(phandle->scene_list_obj);
				focus_row = divoom_alarm_edit_scene_row_from_index(phandle->edit_alarm_info->scene_index);
				focus_scene = phandle->scene_list_btn[focus_row];
				if(focus_scene == NULL)
				{
					focus_row = 0;
					focus_scene = phandle->scene_list_btn[0];
				}
				if(focus_scene)
				{
					lv_group_focus_obj(focus_scene);
					lv_obj_scroll_to_view(focus_scene, LV_ANIM_OFF);
					divoom_alarm_edit_scene_preview_play(phandle, divoom_alarm_edit_scene_display_order[focus_row]);
				}
			}
			else if(i == 2)
			{
				lv_obj_t *focus_obj;

				divoom_alarm_edit_record_refresh_ui(phandle);
				divoom_alarm_edit_record_timer_start(phandle);
				lv_obj_add_flag(phandle->sound_select_obj, LV_OBJ_FLAG_HIDDEN);
				lv_obj_clear_flag(phandle->record_obj, LV_OBJ_FLAG_HIDDEN);
				lv_obj_move_foreground(phandle->record_obj);
				if(divoom_alarm_edit_record_file_valid(phandle))
				{
					focus_obj = phandle->record_file_btn;
				}
				else
				{
					focus_obj = phandle->record_panel;
				}
				if(focus_obj)
				{
					lv_group_focus_obj(focus_obj);
					lv_obj_scroll_to_view(focus_obj, LV_ANIM_OFF);
				}
			}
			else if(i == 3)
			{
				divoom_alarm_edit_enter_tf_pick(phandle);
			}
		}
	}
}

void divoom_alarm_edit_display_menu_event_cb(lv_event_t * e)
{
	int i;
	lv_obj_t *cur_obj;
	lv_obj_t *next_obj;
	Pdivoom_lvgl_alarm_edit_menu_ctrl phandle = (Pdivoom_lvgl_alarm_edit_menu_ctrl)lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);

	cur_obj = lv_event_get_target(e);
	if(code != LV_EVENT_KEY)
	{
		return;
	}
	{
		char c = *((char *)lv_event_get_param(e));

		if(c == DIVOOM_LVGL_KEY_PRE)
		{
			next_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_display_handle, cur_obj, DIVOOM_MENU_DIRECT_RIGHT);
			if(next_obj && next_obj != cur_obj)
			{
				lv_group_focus_obj(next_obj);
				lv_obj_scroll_to_view(next_obj, LV_ANIM_ON);
			}
		}
		else if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			next_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_display_handle, cur_obj, DIVOOM_MENU_DIRECT_LEFT);
			if(next_obj && next_obj != cur_obj)
			{
				lv_group_focus_obj(next_obj);
				lv_obj_scroll_to_view(next_obj, LV_ANIM_ON);
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_alarm_edit_display_index_save(phandle);
			divoom_alarm_edit_display_label_refresh(phandle);
			lv_obj_add_flag(phandle->display_select_obj, LV_OBJ_FLAG_HIDDEN);
			lv_group_focus_obj(phandle->display_btn);
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			for(i = 0; i < DIVOOM_ALARM_EDIT_DISPLAY_COUNT; i++)
			{
				if(cur_obj == phandle->display_select_btn[i])
				{
					phandle->display_index = i;
					divoom_alarm_edit_display_index_save(phandle);
					divoom_alarm_edit_display_label_refresh(phandle);
					lv_obj_add_flag(phandle->display_select_obj, LV_OBJ_FLAG_HIDDEN);
					lv_group_focus_obj(phandle->display_btn);
					break;
				}
			}
		}
	}
}

void divoom_alarm_edit_scene_list_menu_event_cb(lv_event_t * e)
{
	int i;
	char disp_str[64];
	lv_obj_t *cur_obj;
	lv_obj_t *next_obj;
	Pdivoom_lvgl_alarm_edit_menu_ctrl phandle = (Pdivoom_lvgl_alarm_edit_menu_ctrl)lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);

	cur_obj = lv_event_get_target(e);
	if(code != LV_EVENT_KEY)
	{
		return;
	}
	{
		char c = *((char *)lv_event_get_param(e));

		if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			next_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_scene_handle, cur_obj, DIVOOM_MENU_DIRECT_UP);
			if(next_obj && next_obj != cur_obj)
			{
				int row;

				lv_group_focus_obj(next_obj);
				lv_obj_scroll_to_view(next_obj, LV_ANIM_ON);
				row = divoom_alarm_edit_scene_row_from_obj(phandle, next_obj);
				if(row >= 0)
				{
					divoom_alarm_edit_scene_preview_play(phandle, divoom_alarm_edit_scene_display_order[row]);
				}
			}
		}
		else if(c == DIVOOM_LVGL_KEY_PRE)
		{
			next_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_scene_handle, cur_obj, DIVOOM_MENU_DIRECT_DOWN);
			if(next_obj && next_obj != cur_obj)
			{
				int row;

				lv_group_focus_obj(next_obj);
				lv_obj_scroll_to_view(next_obj, LV_ANIM_ON);
				row = divoom_alarm_edit_scene_row_from_obj(phandle, next_obj);
				if(row >= 0)
				{
					divoom_alarm_edit_scene_preview_play(phandle, divoom_alarm_edit_scene_display_order[row]);
				}
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_alarm_edit_scene_preview_stop(phandle);
			divoom_alarm_edit_sound_label_refresh(phandle);
			lv_obj_add_flag(phandle->scene_list_obj, LV_OBJ_FLAG_HIDDEN);
			divoom_alarm_edit_sound_select_apply_tf_state(phandle);
			lv_obj_clear_flag(phandle->sound_select_obj, LV_OBJ_FLAG_HIDDEN);
			lv_group_focus_obj(phandle->sound_select_btn[1]);
			lv_obj_scroll_to_view(phandle->sound_select_btn[1], LV_ANIM_OFF);
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			for(i = 0; i < DIVOOM_ALARM_EDIT_SCENE_COUNT; i++)
			{
				if(cur_obj == phandle->scene_list_btn[i])
				{
					uint8 scene_index;

					scene_index = divoom_alarm_edit_scene_display_order[i];
					phandle->edit_alarm_info->mode = 4;
					phandle->edit_alarm_info->scene_index = scene_index;
					snprintf(disp_str, sizeof(disp_str), "alarm_wake_scene%d", scene_index + 1);
					lv_label_set_text(phandle->music_label_obj, divoom_menu_control_get_disp_str(disp_str));
					divoom_alarm_edit_scene_refresh_selection(phandle);
					divoom_alarm_edit_scene_preview_stop(phandle);
					divoom_alarm_edit_sound_label_refresh(phandle);
					lv_obj_add_flag(phandle->scene_list_obj, LV_OBJ_FLAG_HIDDEN);
					lv_obj_add_flag(phandle->sound_select_obj, LV_OBJ_FLAG_HIDDEN);
					lv_group_focus_obj(phandle->sound_btn);
					break;
				}
			}
		}
	}
}

void divoom_alarm_edit_record_menu_event_cb(lv_event_t * e)
{
	lv_obj_t *cur_obj;
	lv_obj_t *next_obj;
	Pdivoom_lvgl_alarm_edit_menu_ctrl phandle = (Pdivoom_lvgl_alarm_edit_menu_ctrl)lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);

	cur_obj = lv_event_get_target(e);
	if(code != LV_EVENT_KEY)
	{
		return;
	}
	{
		char c = *((char *)lv_event_get_param(e));

		if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			next_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_record_handle, cur_obj, DIVOOM_MENU_DIRECT_UP);
			if(next_obj && next_obj != cur_obj)
			{
				lv_group_focus_obj(next_obj);
				lv_obj_scroll_to_view(next_obj, LV_ANIM_ON);
			}
		}
		else if(c == DIVOOM_LVGL_KEY_PRE)
		{
			next_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_record_handle, cur_obj, DIVOOM_MENU_DIRECT_DOWN);
			if(next_obj && next_obj != cur_obj)
			{
				lv_group_focus_obj(next_obj);
				lv_obj_scroll_to_view(next_obj, LV_ANIM_ON);
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_alarm_edit_record_stop_play(phandle);
			divoom_alarm_edit_record_stop_capture(phandle);
			divoom_alarm_edit_record_timer_stop(phandle);
			divoom_alarm_edit_sound_label_refresh(phandle);
			lv_obj_add_flag(phandle->record_obj, LV_OBJ_FLAG_HIDDEN);
			divoom_alarm_edit_sound_select_apply_tf_state(phandle);
			lv_obj_clear_flag(phandle->sound_select_obj, LV_OBJ_FLAG_HIDDEN);
			lv_group_focus_obj(phandle->sound_select_btn[2]);
			lv_obj_scroll_to_view(phandle->sound_select_btn[2], LV_ANIM_OFF);
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			if(cur_obj == phandle->record_file_btn)
			{
				if(!divoom_alarm_edit_record_file_valid(phandle))
				{
					return;
				}
				if(phandle->record_playing)
				{
					divoom_alarm_edit_record_stop_play(phandle);
				}
				else
				{
					divoom_alarm_edit_record_start_play(phandle);
				}
			}
			else if(cur_obj == phandle->record_as_bell_btn)
			{
				if(divoom_alarm_edit_record_file_valid(phandle))
				{
					phandle->edit_alarm_info->mode = 1;
					divoom_alarm_edit_sound_label_refresh(phandle);
				}
			}
			else if(cur_obj == phandle->record_panel)
			{
				if(phandle->record_capture_active)
				{
					divoom_alarm_edit_record_stop_capture(phandle);
					divoom_alarm_edit_record_commit(phandle);
					divoom_alarm_edit_record_refresh_ui(phandle);
					divoom_alarm_edit_sound_label_refresh(phandle);
				}
				else
				{
					if(divoom_alarm_edit_record_start_capture(phandle) == 0)
					{
						divoom_alarm_edit_record_refresh_ui(phandle);
					}
				}
			}
		}
	}
}

void divoom_alarm_edit_menu_create_bell_select_page(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle, void *main_obj)
{
	int i;
	lv_obj_t *classic_btn;
	lv_obj_t *music_label;
	lv_obj_t *title;

	phandle->pdirect_sound_select_handle = divoom_menu_direct_ctrl_create(1, DIVOOM_ALARM_EDIT_SOUND_FOCUS_COUNT);

	lv_obj_t *bell_select_obj = lv_obj_create(main_obj);
	lv_obj_set_style_bg_color(bell_select_obj, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_border_width(bell_select_obj, 0, 0);
	lv_obj_set_style_pad_all(bell_select_obj, 0, 0);
	lv_obj_set_size(bell_select_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(bell_select_obj, 0, 0);
	lv_obj_add_flag(bell_select_obj, LV_OBJ_FLAG_HIDDEN);
	phandle->sound_select_obj = bell_select_obj;

	title = lv_label_create(bell_select_obj);
	lv_obj_set_size(title, 360, 56);
	lv_obj_set_pos(title, 220, 52);
	lv_obj_set_style_text_color(title, lv_color_black(), 0);
	lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_text(title, divoom_menu_control_get_disp_str("Bells"));

	classic_btn = divoom_alarm_edit_create_sound_list_btn(phandle, bell_select_obj, 0, 160,
	                                                      "classic", "classic_desp", 0);
	music_label = lv_label_create(classic_btn);
	lv_obj_set_size(music_label, 209, 34);
	lv_obj_set_pos(music_label, 427, 43);
	lv_obj_set_style_text_color(music_label, lv_color_hex(0x0), 0);
	lv_obj_set_style_text_font(music_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
	lv_obj_set_style_text_align(music_label, LV_TEXT_ALIGN_RIGHT, 0);
	lv_label_set_text_fmt(music_label, "Music %d", phandle->edit_alarm_info->tip_index + 1);
	phandle->sound_music_label = music_label;

	divoom_alarm_edit_create_sound_list_btn(phandle, bell_select_obj, 1, 298, "scene", "scene_desp", 0);
	divoom_alarm_edit_create_sound_list_btn(phandle, bell_select_obj, 2, 436,
	                                       "SleepAidTabRecord", "record_desp", 0);
	divoom_alarm_edit_create_sound_list_btn(phandle, bell_select_obj, 3, 574, "TFCard", "TFCard_desp",
	                                       divoom_alarm_edit_tf_card_available() ? 0 : 1);

	divoom_alarm_edit_menu_create_scene_list_page(phandle, main_obj);
	divoom_alarm_edit_menu_create_record_page(phandle, main_obj);
	divoom_alarm_edit_menu_create_tf_pick_page(phandle, main_obj);
	divoom_alarm_edit_sound_select_apply_tf_state(phandle);
}

void divoom_alarm_edit_menu_create_scene_list_page(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle, void *main_obj)
{
	int i;
	char disp_str[64];
	lv_obj_t *title;
	lv_obj_t *btn;
	lv_obj_t *label;
	lv_obj_t *check;

	phandle->pdirect_scene_handle = divoom_menu_direct_ctrl_create(1, DIVOOM_ALARM_EDIT_SCENE_COUNT);
	phandle->scene_list_obj = lv_obj_create(main_obj);
	lv_obj_set_style_bg_color(phandle->scene_list_obj, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_border_width(phandle->scene_list_obj, 0, 0);
	lv_obj_set_style_pad_all(phandle->scene_list_obj, 0, 0);
	lv_obj_set_size(phandle->scene_list_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(phandle->scene_list_obj, 0, 0);
	lv_obj_add_flag(phandle->scene_list_obj, LV_OBJ_FLAG_HIDDEN);

	title = lv_label_create(phandle->scene_list_obj);
	lv_obj_set_size(title, 360, 56);
	lv_obj_set_pos(title, 220, 52);
	lv_obj_set_style_text_color(title, lv_color_black(), 0);
	lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_text(title, divoom_menu_control_get_disp_str("scene"));

	for(i = 0; i < DIVOOM_ALARM_EDIT_SCENE_COUNT; i++)
	{
		uint8 scene_index;

		scene_index = divoom_alarm_edit_scene_display_order[i];
		btn = lv_btn_create(phandle->scene_list_obj);
		divoom_alarm_edit_scene_apply_row_style(btn);
		lv_obj_set_size(btn, DIVOOM_ALARM_EDIT_SCENE_BTN_W, DIVOOM_ALARM_EDIT_SCENE_BTN_H);
		lv_obj_set_pos(btn, DIVOOM_ALARM_EDIT_SCENE_BTN_X,
		               DIVOOM_ALARM_EDIT_SCENE_BTN_Y + i * DIVOOM_ALARM_EDIT_SCENE_ROW_STEP);
		lv_obj_add_event_cb(btn, divoom_alarm_edit_scene_list_menu_event_cb, LV_EVENT_KEY, phandle);
		phandle->scene_list_btn[i] = btn;
		divoom_menu_direct_ctrl_update(phandle->pdirect_scene_handle, btn, 0, i);

		label = lv_label_create(btn);
		lv_obj_set_size(label, 560, 34);
		lv_obj_set_pos(label, DIVOOM_ALARM_EDIT_SCENE_LABEL_X, DIVOOM_ALARM_EDIT_SCENE_LABEL_Y);
		lv_obj_set_style_text_color(label, lv_color_black(), 0);
		lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
		snprintf(disp_str, sizeof(disp_str), "alarm_wake_scene%d", scene_index + 1);
		lv_label_set_text(label, divoom_menu_control_get_disp_str(disp_str));

		check = lv_img_create(btn);
		divoom_menu_control_lv_img_set_src(check, "49/49_select.png");
		lv_obj_set_size(check, DIVOOM_ALARM_EDIT_SCENE_CHECK_W, DIVOOM_ALARM_EDIT_SCENE_CHECK_H);
		lv_obj_set_pos(check, DIVOOM_ALARM_EDIT_SCENE_CHECK_X, DIVOOM_ALARM_EDIT_SCENE_CHECK_Y);
		phandle->scene_check_img[i] = check;
	}
	divoom_alarm_edit_scene_refresh_selection(phandle);
}

void divoom_alarm_edit_menu_create_record_page(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle, void *main_obj)
{
	lv_obj_t *title;
	lv_obj_t *as_bell_label;

	phandle->pdirect_record_handle = divoom_menu_direct_ctrl_create(1, DIVOOM_ALARM_EDIT_RECORD_FOCUS_COUNT);
	phandle->record_obj = lv_obj_create(main_obj);
	lv_obj_set_style_bg_color(phandle->record_obj, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_border_width(phandle->record_obj, 0, 0);
	lv_obj_set_style_pad_all(phandle->record_obj, 0, 0);
	lv_obj_set_size(phandle->record_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(phandle->record_obj, 0, 0);
	lv_obj_add_flag(phandle->record_obj, LV_OBJ_FLAG_HIDDEN);

	title = lv_label_create(phandle->record_obj);
	lv_obj_set_size(title, 360, 56);
	lv_obj_set_pos(title, 220, 52);
	lv_obj_set_style_text_color(title, lv_color_black(), 0);
	lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_text(title, divoom_menu_control_get_disp_str("SleepAidTabRecord"));

	phandle->record_panel = lv_btn_create(phandle->record_obj);
	divoom_alarm_edit_record_apply_row_style(phandle->record_panel, 17);
	lv_obj_set_size(phandle->record_panel, DIVOOM_ALARM_EDIT_RECORD_PANEL_W, DIVOOM_ALARM_EDIT_RECORD_PANEL_H);
	lv_obj_set_pos(phandle->record_panel, DIVOOM_ALARM_EDIT_RECORD_PANEL_X, DIVOOM_ALARM_EDIT_RECORD_PANEL_Y);
	lv_obj_add_event_cb(phandle->record_panel, divoom_alarm_edit_record_menu_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_record_handle, phandle->record_panel, 0, 0);

	phandle->record_panel_title = lv_label_create(phandle->record_panel);
	lv_obj_set_style_text_font(phandle->record_panel_title, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
	lv_obj_set_style_text_color(phandle->record_panel_title, lv_color_hex(0x000000), 0);
	lv_obj_set_style_text_align(phandle->record_panel_title, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_size(phandle->record_panel_title, 560, 34);
	lv_obj_set_pos(phandle->record_panel_title, 80, 32);

	phandle->record_panel_icon = lv_img_create(phandle->record_panel);
	lv_obj_set_size(phandle->record_panel_icon, 72, 72);
	lv_obj_set_pos(phandle->record_panel_icon, 325, 103);
	divoom_menu_control_lv_img_set_src(phandle->record_panel_icon, DIVOOM_ALARM_EDIT_RECORD_IMG_IDLE);

	phandle->record_play_center_img = lv_img_create(phandle->record_panel);
	lv_obj_set_size(phandle->record_play_center_img, 72, 72);
	lv_obj_set_pos(phandle->record_play_center_img, 325, 103);
	lv_obj_add_flag(phandle->record_play_center_img, LV_OBJ_FLAG_HIDDEN);

	phandle->record_time_lbl = lv_label_create(phandle->record_panel);
	lv_obj_set_style_text_font(phandle->record_time_lbl, divoom_menu_control_get_font_info(40, phandle->menu_id), 0);
	lv_obj_set_style_text_color(phandle->record_time_lbl, lv_color_hex(0x000000), 0);
	lv_obj_set_style_text_align(phandle->record_time_lbl, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_size(phandle->record_time_lbl, 640, 54);
	lv_obj_set_pos(phandle->record_time_lbl, 40, 202);
	lv_label_set_text(phandle->record_time_lbl, "00:00");

	phandle->record_wave_img = lv_img_create(phandle->record_panel);
	lv_obj_set_size(phandle->record_wave_img, 640, 80);
	lv_obj_set_pos(phandle->record_wave_img, 40, 291);
	lv_obj_add_flag(phandle->record_wave_img, LV_OBJ_FLAG_HIDDEN);

	phandle->record_file_btn = lv_btn_create(phandle->record_obj);
	divoom_alarm_edit_record_apply_row_style(phandle->record_file_btn, 17);
	lv_obj_set_size(phandle->record_file_btn, DIVOOM_ALARM_EDIT_RECORD_PANEL_W, DIVOOM_ALARM_EDIT_RECORD_FILE_ROW_H);
	lv_obj_set_pos(phandle->record_file_btn, DIVOOM_ALARM_EDIT_RECORD_PANEL_X, DIVOOM_ALARM_EDIT_RECORD_FILE_ROW_Y);
	lv_obj_add_event_cb(phandle->record_file_btn, divoom_alarm_edit_record_menu_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_record_handle, phandle->record_file_btn, 0, 1);

	phandle->record_file_title = lv_label_create(phandle->record_file_btn);
	lv_obj_set_style_text_font(phandle->record_file_title, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
	lv_obj_set_style_text_color(phandle->record_file_title, lv_color_hex(0x000000), 0);
	lv_obj_set_size(phandle->record_file_title, 320, 34);
	lv_obj_set_pos(phandle->record_file_title, 40, 32);

	phandle->record_file_dur = lv_label_create(phandle->record_file_btn);
	lv_obj_set_style_text_font(phandle->record_file_dur, divoom_menu_control_get_font_info(20, phandle->menu_id), 0);
	lv_obj_set_style_text_color(phandle->record_file_dur, lv_color_hex(DIVOOM_ALARM_EDIT_RECORD_HINT_COLOR), 0);
	lv_obj_set_style_text_align(phandle->record_file_dur, LV_TEXT_ALIGN_RIGHT, 0);
	lv_obj_set_size(phandle->record_file_dur, 320, 37);
	lv_obj_set_pos(phandle->record_file_dur, 360, 28);

	phandle->record_as_bell_btn = lv_btn_create(phandle->record_obj);
	divoom_alarm_edit_record_apply_row_style(phandle->record_as_bell_btn, 24);
	lv_obj_set_size(phandle->record_as_bell_btn, DIVOOM_ALARM_EDIT_RECORD_PANEL_W, DIVOOM_ALARM_EDIT_RECORD_AS_BELL_H);
	lv_obj_set_pos(phandle->record_as_bell_btn, 44, DIVOOM_ALARM_EDIT_RECORD_AS_BELL_Y);
	lv_obj_add_event_cb(phandle->record_as_bell_btn, divoom_alarm_edit_record_menu_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_record_handle, phandle->record_as_bell_btn, 0, 2);

	as_bell_label = lv_label_create(phandle->record_as_bell_btn);
	lv_obj_set_style_text_font(as_bell_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
	lv_obj_set_style_text_color(as_bell_label, lv_color_hex(0x000000), 0);
	lv_obj_set_size(as_bell_label, 560, 34);
	lv_obj_set_pos(as_bell_label, 31, 32);
	lv_label_set_text(as_bell_label, divoom_menu_control_get_disp_str("AlarmRecordAsBell"));

	divoom_alarm_edit_record_refresh_ui(phandle);
}

void divoom_alarm_edit_menu_create_display_page(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle, void *main_obj)
{
	int i;
	int col;
	int row;
	char disp_str[64];
	lv_coord_t cell_w;
	lv_coord_t cell_h;
	lv_coord_t pad;
	lv_coord_t gap;
	lv_obj_t *scroll;
	lv_obj_t *btn;
	lv_obj_t *img;
	lv_obj_t *label;
	const char *img_src;

	phandle->pdirect_display_handle = divoom_menu_direct_ctrl_create(4, 2);
	phandle->display_select_obj = lv_obj_create(main_obj);
	lv_obj_set_style_bg_color(phandle->display_select_obj, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_border_width(phandle->display_select_obj, 0, 0);
	lv_obj_set_style_pad_all(phandle->display_select_obj, 0, 0);
	lv_obj_set_size(phandle->display_select_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(phandle->display_select_obj, 0, 0);
	lv_obj_add_flag(phandle->display_select_obj, LV_OBJ_FLAG_HIDDEN);

	scroll = lv_obj_create(phandle->display_select_obj);
	lv_obj_set_style_bg_color(scroll, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_border_width(scroll, 0, 0);
	lv_obj_set_style_pad_all(scroll, 0, 0);
	lv_obj_set_size(scroll, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH - 110);
	lv_obj_set_pos(scroll, 0, 110);
	lv_obj_set_scrollbar_mode(scroll, LV_SCROLLBAR_MODE_AUTO);

	pad = 18;
	gap = 10;
	cell_w = 175;
	cell_h = 268;
	for(i = 0; i < DIVOOM_ALARM_EDIT_DISPLAY_COUNT; i++)
	{
		col = i % 4;
		row = i / 4;
		btn = lv_btn_create(scroll);
		lv_obj_set_style_pad_all(btn, 0, 0);
		lv_obj_set_style_border_width(btn, 5, 0);
		lv_obj_set_style_radius(btn, 24, 0);
		lv_obj_set_style_outline_width(btn, 0, 0);
		lv_obj_set_style_border_color(btn, lv_color_hex(0xBEBEBE), 0);
		lv_obj_set_style_border_color(btn, lv_color_hex(0xE89B00), LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
		lv_obj_set_style_bg_color(btn, lv_color_hex(0xE4E4E4), 0);
		lv_obj_set_size(btn, cell_w, cell_h);
		lv_obj_set_pos(btn, pad + col * (cell_w + gap), pad + row * (cell_h + 48));
		lv_obj_add_event_cb(btn, divoom_alarm_edit_display_menu_event_cb, LV_EVENT_KEY, phandle);
		phandle->display_select_btn[i] = btn;
		divoom_menu_direct_ctrl_update(phandle->pdirect_display_handle, btn, col, row);

		img_src = (i <= 0) ? divoom_alarm_edit_bell_menu_scene_img_list[0]
		                   : divoom_alarm_edit_bell_menu_scene_img_list[i - 1];
		img = lv_img_create(btn);
		divoom_menu_control_lv_img_set_src(img, img_src);
		lv_obj_center(img);

		label = lv_label_create(scroll);
		lv_obj_set_size(label, cell_w, 28);
		lv_obj_set_pos(label, pad + col * (cell_w + gap), pad + row * (cell_h + 48) + cell_h + 8);
		lv_obj_set_style_text_color(label, lv_color_hex(0x333333), 0);
		lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(20, phandle->menu_id), 0);
		lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
		if(i <= 0)
		{
			lv_label_set_text(label, divoom_menu_control_get_disp_str("AlarmDisplayNormal"));
		}
		else
		{
			snprintf(disp_str, sizeof(disp_str), "alarm_wake_scene%d", i);
			lv_label_set_text(label, divoom_menu_control_get_disp_str(disp_str));
		}
	}
}

static void divoom_alarm_edit_menu_create_sub_pages(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	if(phandle == NULL || phandle->sub_pages_ready)
	{
		return;
	}
	divoom_alarm_edit_menu_create_bell_select_page(phandle, phandle->main_obj);
	divoom_alarm_edit_menu_create_display_page(phandle, phandle->main_obj);
	phandle->sub_pages_ready = 1;
}

static void divoom_alarm_edit_menu_ensure_sub_pages(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle)
{
	if(phandle == NULL)
	{
		return;
	}
	if(phandle->sub_pages_defer_timer)
	{
		lv_timer_del(phandle->sub_pages_defer_timer);
		phandle->sub_pages_defer_timer = NULL;
	}
	divoom_alarm_edit_menu_create_sub_pages(phandle);
}

static void divoom_alarm_edit_menu_sub_pages_defer_cb(lv_timer_t *t)
{
	Pdivoom_lvgl_alarm_edit_menu_ctrl phandle;

	phandle = (Pdivoom_lvgl_alarm_edit_menu_ctrl)lv_timer_get_user_data(t);
	if(phandle == NULL)
	{
		lv_timer_del(t);
		return;
	}
	phandle->sub_pages_defer_timer = NULL;
	divoom_alarm_edit_menu_create_sub_pages(phandle);
	divoom_alarm_edit_menu_restore_main_focus_if_main(phandle);
	lv_timer_del(t);
}
#endif

void divoom_alarm_edit_menu_create_bell_list(Pdivoom_lvgl_alarm_edit_menu_ctrl phandle, void *main_obj)
{
	int i, j;

	lv_obj_t *bells_obj = lv_obj_create(main_obj);
	lv_obj_set_style_bg_color(bells_obj, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_border_width(bells_obj, 0, 0);
	lv_obj_set_style_pad_all(bells_obj, 0, 0);
	lv_obj_set_size(bells_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(bells_obj, 0, 0);
	lv_obj_add_flag(bells_obj, LV_OBJ_FLAG_HIDDEN);
	phandle->bells_obj = bells_obj;

	lv_obj_t *bell_label = lv_label_create(bells_obj);
	lv_obj_set_size(bell_label, 360, 56);
	lv_obj_set_pos(bell_label, 220, 52);
	lv_obj_set_style_text_color(bell_label, lv_color_black(), 0);
	lv_obj_set_style_text_font(bell_label, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_obj_set_style_text_align(bell_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_text(bell_label, divoom_menu_control_get_disp_str("Bells"));

	//lv_group_t *group = lv_group_create();
	for(i = 0; i < 3; i++)
	{
		for(j = 0; j < 4; j++)
		{
			lv_obj_t *bell_btn = lv_btn_create(phandle->bells_obj);
			lv_obj_remove_style_all(bell_btn);
			lv_obj_set_style_pad_all(bell_btn, 0, 0);
			lv_obj_set_style_radius(bell_btn, 23, 0);
			lv_obj_set_style_border_width(bell_btn, 4, 0);
			lv_obj_set_style_bg_color(bell_btn, lv_color_hex(0xE2E2E2), 0);
			lv_obj_set_style_bg_opa(bell_btn, 7*LV_OPA_100/100, 0);
			lv_obj_set_style_shadow_width(bell_btn, 0, 0);

			lv_obj_set_style_border_color(bell_btn, lv_color_hex(0xE2E2E2), 0);
			lv_obj_set_style_border_opa(bell_btn, 7*LV_OPA_100/100, 0);
			lv_obj_set_style_border_color(bell_btn, lv_color_hex(0xE89B00), LV_STATE_FOCUSED);
			lv_obj_set_style_border_opa(bell_btn, LV_OPA_100, LV_STATE_FOCUSED);
			lv_obj_set_style_border_color(bell_btn, lv_color_hex(0xE89B00), LV_STATE_FOCUS_KEY);
			lv_obj_set_style_border_opa(bell_btn, LV_OPA_100, LV_STATE_FOCUS_KEY);
			
			lv_obj_set_size(bell_btn, 150, 150);
			lv_obj_set_pos(bell_btn, 40 + 190*j, 143 + 190*i);

			lv_obj_t *bell_img = lv_img_create(bell_btn);
			divoom_menu_control_lv_img_set_src(bell_img, divoom_alarm_edit_bell_menu_img_list[i*4+j]);
			lv_obj_center(bell_img);

			if(i*4+j == phandle->edit_alarm_info->tip_index)
			{
				phandle->cur_active_obj = bell_btn;
			}
			phandle->bell_cont_obj[i*4+j] = bell_btn;

			divoom_menu_direct_ctrl_update(phandle->pdirect_music_handle, bell_btn, j, i);
			//lv_group_add_obj(group, bell_btn);
			//lv_obj_add_event_cb(bell_btn, divoom_alarm_edit_bell_menu_event_cb, LV_EVENT_CLICKED, phandle);
			lv_obj_add_event_cb(bell_btn, divoom_alarm_edit_bell_menu_event_cb, LV_EVENT_KEY, phandle);
		}
	}

}

void *divoom_lvgl_alarm_edit_menu_init(void *user_data, int menu_id)
{
	int i, hour, active_id = 0;
	char disp_str[32];
	time_t time_now;
	struct tm p_now;
	Pdivoom_lvgl_alarm_edit_menu_ctrl phandle;
	
	phandle = (Pdivoom_lvgl_alarm_edit_menu_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_alarm_edit_menu_ctrl));
    phandle->menu_id = menu_id;
	phandle->edit_alarm_info = (T_DIVOOM_ALARM_ACTIVE_DO_INFO*)divoom_system_malloc(sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
	phandle->cur_alarm_info = (T_DIVOOM_ALARM_ACTIVE_DO_INFO*)user_data;
	memcpy(phandle->edit_alarm_info, phandle->cur_alarm_info, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
#ifdef DIVOOM_TIME_FRAME_SPEAKER
	phandle->pdirect_handle = divoom_menu_direct_ctrl_create(9, 1);
	divoom_alarm_edit_display_index_load(phandle);
#else
	phandle->pdirect_handle = divoom_menu_direct_ctrl_create(8, 1);
#endif
	phandle->pdirect_repeat_handle = divoom_menu_direct_ctrl_create(1, 7);
	phandle->pdirect_music_handle = divoom_menu_direct_ctrl_create(4, 3);
	phandle->first_active_obj = NULL;
	
	phandle->main_obj = lv_obj_create(NULL);
	lv_style_init(&phandle->backgroud_type);
    lv_style_set_bg_color(&phandle->backgroud_type, lv_color_hex(0xE4E4E4));
    lv_style_set_text_color(&phandle->backgroud_type, lv_color_hex(0x0));
    lv_obj_add_style(phandle->main_obj, &phandle->backgroud_type, 0);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	
	lv_obj_t *top = lv_obj_create(phandle->main_obj);
	lv_obj_set_size(top, 800, 78);
	lv_obj_set_style_border_width(top, 0, 0);
	lv_obj_set_style_pad_all(top, 0, 0);
	lv_obj_set_style_bg_color(top, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_pos(top, 0, 32);
	
	lv_obj_t *edit_alarm = lv_label_create(top);
	lv_obj_set_pos(edit_alarm, 333, 19);
    lv_obj_set_size(edit_alarm, 400, 40);
	lv_obj_set_style_text_font(edit_alarm, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_obj_set_style_text_color(edit_alarm, lv_color_hex(0x0), 0);
	lv_label_set_text(edit_alarm, divoom_menu_control_get_disp_str("EditAlarm"));
	
	lv_obj_t *time_label = lv_label_create(top);
	lv_obj_set_pos(time_label, 40, 19);
    lv_obj_set_size(time_label, 160, 40);
	lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_obj_set_style_text_color(time_label, lv_color_hex(0x0), 0);

	time_now = time(NULL);
	localtime_r(&time_now, &p_now);
	hour = p_now.tm_hour;
	if(divoom_main_get_time_flag() == 0)
    {
        if(hour != 12)
        {
            hour = hour % 12;
            if(hour == 0)
            {
                hour = 12;
            }
        }
    }
	sprintf(disp_str, "%02d:%02d", hour, p_now.tm_min);
	lv_label_set_text(time_label, disp_str);
	lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_LEFT, 0);
	phandle->menu_bar_timer = lv_timer_create(divoom_menu_alarm_refresh_edit_menu_bar, 1000, phandle);
	phandle->time_obj = time_label;

	lv_style_init(&phandle->obj_style);
	lv_style_set_pad_all(&phandle->obj_style, 0);
    lv_style_set_pad_gap(&phandle->obj_style, 0);
    lv_style_set_border_width(&phandle->obj_style, 0);
    lv_style_set_outline_width(&phandle->obj_style, 0);
    lv_style_set_border_side(&phandle->obj_style, LV_BORDER_SIDE_NONE);
    lv_style_set_bg_opa(&phandle->obj_style, LV_OPA_0);
	
	//divoom_menu_control_init_btn_stype(&phandle->press_type, NULL, 
    //                                        &phandle->normal_type, NULL, 
    //                                        &phandle->select_press_type, NULL, 17);

	lv_style_init(&phandle->normal_type);
	lv_style_set_bg_color(&phandle->normal_type, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->normal_type, 0);
	lv_style_set_border_width(&phandle->normal_type, 4);
	lv_style_set_border_color(&phandle->normal_type, lv_color_hex(0xE4E4E4));
	lv_style_set_radius(&phandle->normal_type, 17);
	lv_style_set_shadow_width(&phandle->normal_type, 0);
	lv_style_set_outline_width(&phandle->normal_type, 0);

	lv_style_init(&phandle->press_type);
	lv_style_set_bg_color(&phandle->press_type, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->press_type, 0);
	lv_style_set_border_width(&phandle->press_type, 4);
	lv_style_set_border_color(&phandle->press_type, lv_color_hex(0x6D6D6D));
	lv_style_set_radius(&phandle->press_type, 17);
	lv_style_set_shadow_width(&phandle->press_type, 0);
	lv_style_set_outline_width(&phandle->press_type, 0);

	lv_style_init(&phandle->select_press_type);
	lv_style_set_bg_color(&phandle->select_press_type, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->select_press_type, 0);
	lv_style_set_border_width(&phandle->select_press_type, 4);
	lv_style_set_border_color(&phandle->select_press_type, lv_color_hex(0xE89B00));
	lv_style_set_radius(&phandle->select_press_type, 17);
	lv_style_set_shadow_width(&phandle->select_press_type, 0);
	lv_style_set_outline_width(&phandle->select_press_type, 0);
	
	if(divoom_main_get_time_flag())//24小时
    {
    	for(i = 0;i < 2; i++)
		{
			/*lv_obj_t *obj = lv_obj_create(phandle->main_obj);
			lv_obj_add_style(obj, &phandle->obj_style, 0);
			lv_obj_set_size(obj, 720, 138);
			lv_obj_set_pos(obj, 40, 160+138*i);
			lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);*/
		
			lv_obj_t *btn  = lv_btn_create(phandle->main_obj);
			lv_obj_set_size(btn, 712, 138);
			lv_obj_set_pos(btn, 40, 160+138*i);
			lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
			if(phandle->first_active_obj == NULL)
        	{
            	phandle->first_active_obj = btn;
            	phandle->main_default_focus_obj = btn;
       	 	}
			lv_obj_add_style(btn, &phandle->normal_type, 0);
		
			lv_obj_add_style(btn, &phandle->press_type, LV_STATE_FOCUSED);
			lv_obj_add_style(btn, &phandle->press_type, LV_STATE_FOCUS_KEY);
			lv_obj_set_style_pad_all(btn, 0, 0);
			lv_obj_add_event_cb(btn, divoom_alarm_edit_menu_set_event_cb, LV_EVENT_KEY, phandle);
			
			divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn, active_id, 0);
			active_id ++;

			lv_obj_t *time_label = lv_label_create(btn);
			lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
			lv_obj_set_style_text_color(time_label, lv_color_hex(0x0), 0);
			lv_obj_set_size(time_label, 400, 34);
			lv_obj_set_pos(time_label, 32, 48);

			lv_obj_t *disp_obj = lv_obj_create(btn);
			lv_obj_set_style_pad_all(disp_obj, 0, 0);
			lv_obj_set_style_bg_color(disp_obj, lv_color_hex(0xFFFFFF), 0);
			lv_obj_set_style_border_color(disp_obj, lv_color_hex(0xA9A9A9), 0);
			lv_obj_set_style_border_width(disp_obj, 4, 0);
			lv_obj_set_style_radius(disp_obj, 14, 0);
			lv_obj_set_size(disp_obj, 180, 80);
			lv_obj_set_pos(disp_obj, 496, 25);
			lv_obj_clear_flag(disp_obj, LV_OBJ_FLAG_SCROLLABLE);

			lv_obj_t *disp_label = lv_label_create(disp_obj);
			lv_obj_set_style_text_font(disp_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
			lv_obj_set_style_text_color(disp_label, lv_color_hex(0x636363), 0);
			lv_obj_set_style_text_align(disp_label, LV_TEXT_ALIGN_CENTER, 0);
			lv_obj_set_size(disp_label, 180, 34);
			lv_obj_set_pos(disp_label, 0, 23);
			if(i == 0)//小时
			{
				phandle->hour = phandle->edit_alarm_info->hour;
				phandle->hour_disp_label = disp_label;
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Hour"));
        		lv_label_set_text_fmt(disp_label, "%02d", phandle->edit_alarm_info->hour);
			}
			else if(i == 1)//分钟
			{
				phandle->minute = phandle->edit_alarm_info->minute;
				phandle->minute_disp_label = disp_label;
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Minute"));
				lv_label_set_text_fmt(disp_label, "%02d", phandle->edit_alarm_info->minute);
			}
	
			lv_obj_t *up_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(up_img, "49/49_up.png");
			lv_obj_set_pos(up_img, 578, 4);
			lv_obj_t *down_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(down_img, "49/49_down.png");	
			lv_obj_set_pos(down_img, 578, 110);
		}
	}
	else
	{
		for(i = 0;i < 3; i++)
		{
			/*lv_obj_t *obj = lv_obj_create(phandle->main_obj);
			lv_obj_add_style(obj, &phandle->obj_style, 0);
			lv_obj_set_size(obj, 720, 138);
			lv_obj_set_pos(obj, 40, 160+138*i);
			lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);*/
		
			lv_obj_t *btn  = lv_btn_create(phandle->main_obj);
			lv_obj_set_size(btn, 712, 138);
			lv_obj_set_pos(btn, 40, 160+138*i);
			lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
			if(phandle->first_active_obj == NULL)
        	{
            	phandle->first_active_obj = btn;
            	phandle->main_default_focus_obj = btn;
       	 	}
			lv_obj_add_style(btn, &phandle->normal_type, 0);
		
			lv_obj_add_style(btn, &phandle->press_type, LV_STATE_FOCUSED);
			lv_obj_add_style(btn, &phandle->press_type, LV_STATE_FOCUS_KEY);
			lv_obj_set_style_pad_all(btn, 0, 0);
			lv_obj_add_event_cb(btn, divoom_alarm_edit_menu_set_event_cb, LV_EVENT_KEY, phandle);
			
			divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn, active_id, 0);
			active_id ++;

			lv_obj_t *time_label = lv_label_create(btn);
			lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
			lv_obj_set_style_text_color(time_label, lv_color_hex(0x0), 0);
			lv_obj_set_size(time_label, 400, 34);
			lv_obj_set_pos(time_label, 32, 48);

			lv_obj_t *disp_obj = lv_obj_create(btn);
			lv_obj_set_style_pad_all(disp_obj, 0, 0);
			lv_obj_set_style_bg_color(disp_obj, lv_color_hex(0xFFFFFF), 0);
			lv_obj_set_style_border_color(disp_obj, lv_color_hex(0xA9A9A9), 0);
			lv_obj_set_style_border_width(disp_obj, 4, 0);
			lv_obj_set_style_radius(disp_obj, 14, 0);
			lv_obj_set_size(disp_obj, 180, 80);
			lv_obj_set_pos(disp_obj, 496, 25);
			lv_obj_clear_flag(disp_obj, LV_OBJ_FLAG_SCROLLABLE);

			lv_obj_t *disp_label = lv_label_create(disp_obj);
			lv_obj_set_style_text_font(disp_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
			lv_obj_set_style_text_color(disp_label, lv_color_hex(0x636363), 0);
			lv_obj_set_style_text_align(disp_label, LV_TEXT_ALIGN_CENTER, 0);
			lv_obj_set_size(disp_label, 180, 34);
			lv_obj_set_pos(disp_label, 0, 23);
			if(i == 0)
			{
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("TimePeriod"));
				if(phandle->edit_alarm_info->hour < 12)
				{
					phandle->am_pm_flag = 0;
					lv_label_set_text(disp_label, divoom_menu_control_get_disp_str("AM"));
				}
				else
				{
					phandle->am_pm_flag = 1;
					lv_label_set_text(disp_label, divoom_menu_control_get_disp_str("PM"));
				}
				phandle->am_pm_disp_label = disp_label;
			}
			else if(i == 1)
			{
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Hour"));
				hour = phandle->edit_alarm_info->hour;
            	if(hour != 12)
            	{
               		hour = hour % 12;
                	if(hour == 0)
                	{
                    	hour = 12;
                	}
            	}
				phandle->hour = hour;
				phandle->hour_disp_label = disp_label;
				lv_label_set_text_fmt(disp_label, "%02d", hour);
			}
			else if(i == 2)
			{
				phandle->minute = phandle->edit_alarm_info->minute;
				phandle->minute_disp_label = disp_label;
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Minute"));
				lv_label_set_text_fmt(disp_label, "%02d", phandle->edit_alarm_info->minute);
			}
	
			lv_obj_t *up_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(up_img, "49/49_up.png");
			lv_obj_set_pos(up_img, 578, 4);
			lv_obj_t *down_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(down_img, "49/49_down.png");	
			lv_obj_set_pos(down_img, 578, 110);
		}
	}
	
#ifdef DIVOOM_TIME_FRAME_SPEAKER
	for(i = 0;i < 5; i++)
#else
	for(i = 0;i < 4; i++)
#endif
	{
		lv_obj_t *btn  = lv_btn_create(phandle->main_obj);
		lv_obj_set_size(btn, 712, 90);
		if(divoom_main_get_time_flag())
		{
			lv_obj_set_pos(btn, 40, 478+98*i);
		}
		else
		{
			lv_obj_set_pos(btn, 40, 618+98*i);
		}
		lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_add_style(btn, &phandle->normal_type, 0);
		
		lv_obj_add_style(btn, &phandle->press_type, LV_STATE_FOCUSED);
		lv_obj_add_style(btn, &phandle->press_type, LV_STATE_FOCUS_KEY);
		lv_obj_set_style_pad_all(btn, 0, 0);
		lv_obj_add_event_cb(btn, divoom_alarm_edit_menu_set_event_cb, LV_EVENT_KEY, phandle);
		
		divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn, active_id, 0);
		active_id ++;

		if(i == 0)
		{
			lv_obj_t *time_label = lv_label_create(btn);
			lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
			lv_obj_set_style_text_color(time_label, lv_color_hex(0X0), 0);
			lv_obj_set_size(time_label, 400, 34);
			lv_obj_set_pos(time_label, 32, 28);
			
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Alarm"));

			lv_obj_t *right_img = lv_img_create(btn);
			if(phandle->edit_alarm_info->on_off)
			{
				divoom_menu_control_lv_img_set_src(right_img, "23/23_on.png");
			}
			else
			{
				divoom_menu_control_lv_img_set_src(right_img, "23/23_off.png");
			}
			lv_obj_set_pos(right_img, 608, 23);
			phandle->alarm_onoff_img = right_img;
		}
		else if(i == 1)
		{
			lv_obj_t *repeat_label = lv_label_create(btn);
			lv_obj_set_style_text_font(repeat_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
			lv_obj_set_style_text_color(repeat_label, lv_color_hex(0x7A7A7A), 0);
			lv_obj_set_style_text_align(repeat_label, LV_TEXT_ALIGN_RIGHT, 0);
			lv_obj_set_size(repeat_label, 403, 34);
			lv_obj_set_pos(repeat_label, 333-100, 28);
			phandle->cys_label_obj = repeat_label;
			phandle->repeat_btn = btn;

			lv_obj_t *time_label = lv_label_create(btn);
			lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
			lv_obj_set_style_text_color(time_label, lv_color_hex(0X0), 0);
			lv_obj_set_size(time_label, 400, 34);
			lv_obj_set_pos(time_label, 32, 28);
			
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Repeat"));
			divoom_alarm_edit_menu_set_repeat_label_text(phandle);

			lv_obj_t *right_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(right_img, "49/49_right.png");
			lv_obj_set_pos(right_img, 644, 27);
		}
		else if(i == 2)
		{
			lv_obj_t *music_label = lv_label_create(btn);
			lv_obj_set_style_text_font(music_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
			lv_obj_set_style_text_color(music_label, lv_color_hex(0x7A7A7A), 0);
			lv_obj_set_style_text_align(music_label, LV_TEXT_ALIGN_RIGHT, 0);
			lv_obj_set_size(music_label, 150+150, 34);
			lv_obj_set_pos(music_label, 536-10-180, 28-2);
			phandle->music_label_obj = music_label;
			phandle->sound_btn = btn;

			lv_obj_t *time_label = lv_label_create(btn);
			lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
			lv_obj_set_style_text_color(time_label, lv_color_hex(0X0), 0);
			lv_obj_set_size(time_label, 400, 34);
			lv_obj_set_pos(time_label, 32, 28);
			
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Bells"));
#ifdef DIVOOM_TIME_FRAME_SPEAKER
			divoom_alarm_edit_sound_label_refresh(phandle);
#else
			if(phandle->edit_alarm_info->mode == 4)
			{
				sprintf(disp_str, "alarm_wake_scene%d", phandle->edit_alarm_info->scene_index + 1);
				lv_label_set_text(music_label, divoom_menu_control_get_disp_str(disp_str));
			}
			else
			{		
				lv_label_set_text_fmt(music_label, "Music %d", phandle->edit_alarm_info->tip_index + 1);
			}
#endif

			lv_obj_t *right_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(right_img, "49/49_right.png");
			lv_obj_set_pos(right_img, 644, 27);
		}
#ifdef DIVOOM_TIME_FRAME_SPEAKER
		else if(i == 3)
		{
			lv_obj_t *display_label = lv_label_create(btn);
			lv_obj_set_style_text_font(display_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
			lv_obj_set_style_text_color(display_label, lv_color_hex(0x7A7A7A), 0);
			lv_obj_set_style_text_align(display_label, LV_TEXT_ALIGN_RIGHT, 0);
			lv_obj_set_size(display_label, 150+150, 34);
			lv_obj_set_pos(display_label, 536-10-180, 28-2);
			phandle->display_label_obj = display_label;
			phandle->display_btn = btn;

			lv_obj_t *time_label = lv_label_create(btn);
			lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
			lv_obj_set_style_text_color(time_label, lv_color_hex(0X0), 0);
			lv_obj_set_size(time_label, 400, 34);
			lv_obj_set_pos(time_label, 32, 28);
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Display"));
			divoom_alarm_edit_display_label_refresh(phandle);

			lv_obj_t *right_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(right_img, "49/49_right.png");
			lv_obj_set_pos(right_img, 644, 27);
		}
		else if(i == 4)
#else
		else if(i == 3)
#endif
		{
			lv_obj_t *vol_min_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(vol_min_img, "49/49_vol_min.png");
			lv_obj_set_pos(vol_min_img, 28, 25);

			lv_obj_t *slide_obj = lv_slider_create(btn);
    		lv_slider_set_range(slide_obj, 0, 100);
    		lv_obj_set_pos(slide_obj, 104, 41);
    		lv_obj_set_size(slide_obj, 509, 4);
    		lv_slider_set_value(slide_obj, phandle->edit_alarm_info->volume, LV_ANIM_ON);
    		lv_obj_set_style_bg_color(slide_obj, lv_color_hex(0x707070), LV_PART_INDICATOR);
    		lv_obj_set_style_bg_color(slide_obj, lv_color_hex(0xB1B1B1), LV_PART_MAIN);
    		phandle->vol_slide_obj = slide_obj;
	
			lv_obj_t *vol_max_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(vol_max_img, "49/49_vol_max.png");
			lv_obj_set_pos(vol_max_img, 640, 25);
		}

		
	}
	
	//save
	/*lv_obj_t *save_obj = lv_obj_create(phandle->main_obj);
	lv_obj_set_x(save_obj, 40);
	lv_obj_set_y(save_obj, 890);
	lv_obj_set_width(save_obj, 704);
	lv_obj_set_height(save_obj, 120);
	lv_obj_set_style_pad_all(save_obj, 0, 0);
	lv_obj_set_style_border_width(save_obj, 6, 0);
	lv_obj_set_style_border_color(save_obj, lv_color_hex(0x0A2A12), 0);
	lv_obj_set_style_border_opa(save_obj, LV_OPA_100, 0);
	lv_obj_set_style_radius(save_obj, 75, 0);
	lv_obj_set_style_bg_color(save_obj, lv_color_hex(0x000000), 0);
	lv_obj_set_style_bg_opa(save_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	lv_obj_set_style_bg_opa(save_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_shadow_width(save_obj, 0, 0); //去掉边框底部的阴影
	lv_obj_set_style_border_color(save_obj, lv_color_hex(0x3AED3A), LV_STATE_FOCUSED);
	lv_obj_set_style_border_color(save_obj, lv_color_hex(0x3AED3A), LV_STATE_FOCUS_KEY);
	phandle->save_obj = save_obj;
	
	lv_obj_t *save_btn = lv_btn_create(save_obj);*/
	lv_obj_t *save_btn = lv_btn_create(phandle->main_obj);
	lv_obj_remove_style_all(save_btn);
	lv_obj_set_pos(save_btn, 54, 1108);
	lv_obj_set_width(save_btn, 692);
	lv_obj_set_height(save_btn, 108);
	lv_obj_set_style_pad_all(save_btn, 0, 0);
	lv_obj_set_style_border_width(save_btn, 6, 0);
	lv_obj_set_style_border_color(save_btn, lv_color_hex(0xE4E4E4), 0);
	//lv_obj_set_style_border_opa(save_btn, LV_OPA_0, 0);
	lv_obj_set_style_radius(save_btn, 75, 0);
	lv_obj_set_style_bg_color(save_btn, lv_color_hex(0x6BD96B), 0);
	lv_obj_set_style_bg_color(save_btn, lv_color_hex(0x17CD17), LV_STATE_FOCUSED);
	lv_obj_set_style_bg_color(save_btn, lv_color_hex(0x17CD17), LV_STATE_FOCUS_KEY);
	lv_obj_set_style_bg_opa(save_btn, LV_OPA_100, 0);
	lv_obj_set_style_bg_opa(save_btn, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	lv_obj_set_style_bg_opa(save_btn, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_shadow_width(save_btn, 0, 0); //去掉边框底部的阴影
	//lv_obj_set_align(save_btn, LV_ALIGN_CENTER);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, save_btn, active_id, 0);
	active_id ++;
	lv_obj_set_style_outline_width(save_btn, 6, 0);
	lv_obj_set_style_outline_color(save_btn, lv_color_hex(0x6BD96B), 0);
	lv_obj_set_style_outline_color(save_btn, lv_color_hex(0x198D19), LV_STATE_FOCUSED);
	lv_obj_set_style_outline_color(save_btn, lv_color_hex(0x198D19), LV_STATE_FOCUS_KEY);
	lv_obj_set_style_text_color(save_btn, lv_color_hex(0x2D712D), 0);
	lv_obj_set_style_text_color(save_btn, lv_color_hex(0x074F07), LV_STATE_FOCUSED);
	lv_obj_set_style_text_color(save_btn, lv_color_hex(0x074F07), LV_STATE_FOCUS_KEY);
	lv_obj_add_event_cb(save_btn, divoom_alarm_edit_menu_set_event_cb, LV_EVENT_KEY, phandle);
	phandle->save_btn = save_btn;
	
	lv_obj_t *save_label = lv_label_create(save_btn);
	//lv_obj_set_style_text_color(save_label, lv_color_hex(0x228B26), 0);
	lv_obj_set_style_text_font(save_label, divoom_menu_control_get_font_info(48, phandle->menu_id), 0);
	lv_label_set_text(save_label, divoom_menu_control_get_disp_str("Save"));
	lv_obj_center(save_label);
	phandle->save_label = save_label;

	//lv_obj_t *cont = lv_obj_create(phandle->main_obj);
	/*lv_obj_t *list = lv_list_create(phandle->main_obj);
	lv_obj_clear_flag(list, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_set_style_bg_color(list, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(list, LV_OPA_10, 0);
	lv_obj_set_style_pad_all(list, 0, 0);
	lv_obj_set_style_border_width(list, 0, 0);
	lv_obj_set_style_radius(list, 32, 0);
	lv_obj_set_size(list, 728, 172);
	lv_obj_set_pos(list, 36, 744);
	lv_obj_set_style_base_dir(list, LV_BASE_DIR_LTR, 0);
	lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
	
	
	for(i = 1;i < 3; i++)
	{
		if(i == 1)
		{
			//重复
			lv_obj_t *repeat_btn = lv_btn_create(list);
			lv_obj_set_size(repeat_btn, 728, 86);
			lv_obj_clear_flag(repeat_btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);
			lv_obj_add_event_cb(repeat_btn, divoom_alarm_edit_menu_set_event_cb, LV_EVENT_CLICKED, phandle);
			phandle->repeat_btn = repeat_btn;
			
			lv_obj_t *disp_repeat = lv_label_create(repeat_btn);
			lv_obj_set_size(disp_repeat, 210, 42);
			lv_obj_set_pos(disp_repeat, 40, 22);
			lv_obj_set_style_text_font(disp_repeat, divoom_menu_control_get_font_info(32), 0);
			lv_label_set_text(disp_repeat, divoom_menu_control_get_disp_str("Repeat"));
			lv_obj_t *cys_label = lv_label_create(repeat_btn);
			lv_obj_set_style_text_opa(cys_label, LV_OPA_60, 0);
			lv_obj_set_size(cys_label, 461, 37);
			lv_obj_set_pos(cys_label, 201, 25);
			lv_obj_set_style_text_font(cys_label, divoom_menu_control_get_font_info(28), 0);
			lv_obj_set_style_text_align(cys_label, LV_TEXT_ALIGN_RIGHT, 0);
			phandle->cys_label_obj = cys_label;
			divoom_alarm_edit_menu_set_repeat_label_text(phandle);

			lv_obj_t *repeat_img = lv_img_create(repeat_btn);
			divoom_menu_control_lv_img_set_src(repeat_img, "49/49_right.png");
			lv_obj_set_size(repeat_img, 36, 36);
			lv_obj_set_pos(repeat_img, 666, 25);
			lv_obj_set_style_img_opa(repeat_img, LV_OPA_100, 0);

			lv_obj_add_style(repeat_btn, &phandle->btn_normal_type, 0);
   			lv_obj_add_style(repeat_btn, &phandle->btn_press_type, LV_STATE_PRESSED);

			lv_obj_t *line_img2 = lv_img_create(phandle->main_obj);
			divoom_menu_control_lv_img_set_src(line_img2, "49/49_line.png");
			lv_obj_set_size(line_img2, 688, 2);
			lv_obj_set_pos(line_img2, 76, 830);
			lv_obj_set_style_img_opa(line_img2, LV_OPA_100, 0);
		 
		}
		if(i == 2)
		{
			//铃声
			lv_obj_t *sound_btn = lv_btn_create(list);
			lv_obj_set_size(sound_btn, 728, 86);
			lv_obj_clear_flag(sound_btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);
			phandle->sound_btn = sound_btn;
			lv_obj_add_event_cb(sound_btn, divoom_alarm_edit_menu_set_event_cb, LV_EVENT_CLICKED, phandle);
				
			lv_obj_t *disp_sound = lv_label_create(sound_btn);
			lv_obj_set_size(disp_sound, 210, 42);
			lv_obj_set_pos(disp_sound, 40, 22);
			lv_obj_set_style_text_font(disp_sound, divoom_menu_control_get_font_info(32), 0);
			lv_label_set_text(disp_sound, divoom_menu_control_get_disp_str("Bells"));
			lv_obj_t *music_label = lv_label_create(sound_btn);
			lv_obj_set_style_text_opa(music_label, LV_OPA_60, 0);
			lv_obj_set_size(music_label, 210, 37);
			lv_obj_set_pos(music_label, 452, 25);
			lv_obj_set_style_text_font(music_label, divoom_menu_control_get_font_info(28), 0);
			lv_label_set_text_fmt(music_label, "Music %d", phandle->edit_alarm_info->tip_index + 1);
			lv_obj_set_style_text_align(music_label, LV_TEXT_ALIGN_RIGHT, 0);
			phandle->music_label_obj = music_label;
			lv_obj_t *sound_img = lv_img_create(sound_btn);
			divoom_menu_control_lv_img_set_src(sound_img, "49/49_right.png");
			lv_obj_set_size(sound_img, 36, 36);
			lv_obj_set_pos(sound_img, 666, 25);
			lv_obj_set_style_img_opa(sound_img, LV_OPA_100, 0);
			lv_obj_add_style(sound_btn, &phandle->btn_normal_type, 0);
    		lv_obj_add_style(sound_btn, &phandle->btn_press_type, LV_STATE_PRESSED);

		}
		
	}
	*/
	
	lv_obj_t *bg_obj = lv_obj_create(phandle->main_obj);
	lv_obj_set_size(bg_obj, 800, 1280);
	lv_obj_set_pos(bg_obj, 0, 0);
	lv_obj_set_style_bg_opa(bg_obj, LV_OPA_0, 0);
	lv_obj_set_style_border_width(bg_obj, 0, 0);
	lv_obj_set_style_pad_all(bg_obj, 0, 0);
	lv_obj_clear_flag(bg_obj, LV_OBJ_FLAG_SCROLLABLE);
	phandle->bg_obj = bg_obj;
	
	lv_obj_t *bg_img = lv_img_create(bg_obj);
	divoom_menu_control_lv_img_set_src(bg_img, "49/49_bg_cover.png");
	lv_obj_add_flag(phandle->bg_obj, LV_OBJ_FLAG_HIDDEN);

	divoom_alarm_edit_menu_create_week_list(phandle, phandle->main_obj);
#ifdef DIVOOM_TIME_FRAME_SPEAKER
	phandle->sub_pages_defer_timer = lv_timer_create(divoom_alarm_edit_menu_sub_pages_defer_cb, 1, phandle);
	lv_timer_set_repeat_count(phandle->sub_pages_defer_timer, 1);
#endif
	divoom_alarm_edit_menu_create_bell_list(phandle, phandle->main_obj);

	return phandle;
}

void divoom_lvgl_alarm_edit_menu_disp(void *handle, void *active)
{
    Pdivoom_lvgl_alarm_edit_menu_ctrl phandle = (Pdivoom_lvgl_alarm_edit_menu_ctrl)handle;
    if(phandle->first_active_obj)
	{
		lv_group_focus_obj(phandle->first_active_obj);
		phandle->first_active_obj = NULL;
    }

	if(active)
	{
		lv_group_focus_obj(active);
	}
    lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_alarm_edit_menu_exit(void *handle)
{
	Pdivoom_lvgl_alarm_edit_menu_ctrl phandle = (Pdivoom_lvgl_alarm_edit_menu_ctrl)handle;
	
#ifdef DIVOOM_TIME_FRAME_SPEAKER
	divoom_alarm_edit_record_stop_play(phandle);
	divoom_alarm_edit_record_stop_capture(phandle);
	divoom_alarm_edit_record_timer_stop(phandle);
	divoom_alarm_edit_tf_free_music_list(phandle);
	if(phandle->tf_styles_inited)
	{
		lv_style_reset(&phandle->tf_normal_type);
		lv_style_reset(&phandle->tf_press_type);
		phandle->tf_styles_inited = 0;
	}
	if(phandle->sub_pages_defer_timer)
	{
		lv_timer_del(phandle->sub_pages_defer_timer);
		phandle->sub_pages_defer_timer = NULL;
	}
#endif
	lv_style_reset(&phandle->backgroud_type);
	lv_style_reset(&phandle->obj_style);
	lv_style_reset(&phandle->normal_type);
	lv_style_reset(&phandle->select_press_type);
	lv_style_reset(&phandle->press_type);
	divoom_menu_direct_ctrl_destory(phandle->pdirect_handle);
	divoom_menu_direct_ctrl_destory(phandle->pdirect_repeat_handle);
	divoom_menu_direct_ctrl_destory(phandle->pdirect_music_handle);
#ifdef DIVOOM_TIME_FRAME_SPEAKER
	divoom_menu_direct_ctrl_destory(phandle->pdirect_sound_select_handle);
	divoom_menu_direct_ctrl_destory(phandle->pdirect_scene_handle);
	divoom_menu_direct_ctrl_destory(phandle->pdirect_display_handle);
	divoom_menu_direct_ctrl_destory(phandle->pdirect_record_handle);
#endif
	if(phandle->menu_bar_timer)
	{
		lv_timer_del(phandle->menu_bar_timer);
	}
	lv_obj_del(phandle->main_obj);
	
	divoom_system_free(phandle->edit_alarm_info);
	divoom_system_free(phandle);

}







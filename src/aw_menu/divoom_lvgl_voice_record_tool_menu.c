#if 0 /* legacy: MasterGo voice_record_tool 全页；AstroToo menu 105 已走 astrootoo_tools MEMO，未挂本模块 */

#include "divoom.h"
#include "lvgl.h"
#include <string.h>
#include <time.h>
#include "divoom_menu_control.h"
#include "divoom_main.h"
#include "divoom_lvgl_message_box.h"
#include "divoom_lvgl_top_leave_msg_notice_menu.h"
#include "divoom_leave_msg_record.h"
#include "divoom_leave_msg_ctrl.h"
#include "divoom_voice_read.h"
#include "divoom_local_music_ctrl.h"
#include <unistd.h>

/* MasterGo layer 609:75655 DSL（温馨留言）像素对齐 */
#define VOICE_ROW_FULL_X    48
#define VOICE_ROW_FULL_W    704
#define VOICE_CARD_X        48
#define VOICE_CARD_Y        164
#define VOICE_CARD_W        704
#define VOICE_CARD_H        384
#define VOICE_TITLE_TOP_Y   61
#define VOICE_ROW_TEXT_PAD  40
#define VOICE_STATUS_H      98
#define VOICE_REROW_H       98
#define VOICE_SAVE_H        132
#define VOICE_CANCEL_H      132
/* 竖直留白：相邻带边框区块之间留出间隙（避免贴边） */
#define VOICE_GAP_AFTER_CARD        28
#define VOICE_GAP_STATUS_TO_REROW    8
#define VOICE_GAP_REROW_TO_SAVE    148
#define VOICE_GAP_SAVE_TO_CANCEL    18
/* 旋钮焦点：全页统一深黑描边（录音大卡 / 重新录音 / 保存取消外框） */
#define VOICE_FOCUS_BORDER_COLOR    0x000000
/* 保存/取消：最外灰框 + 浅环 + 浅灰胶囊 + 黑字（稿图双层/多圈层） */
#define VOICE_PILL_FRAME_PAD       10
#define VOICE_PILL_FRAME_BW         5
#define VOICE_PILL_FRAME_COLOR      0xC0C0C0
#define VOICE_PILL_INSET            4
#define VOICE_PILL_OUTER_BW         0
#define VOICE_PILL_OUTER_BORDER     0xFFFFFF
#define VOICE_PILL_RING_BG          0xEFEFEF
#define VOICE_PILL_BG               0xD6D6D6
#define VOICE_PILL_TEXT             0x1A1A1A
#define VOICE_PILL_BG_FOCUS         0xC8C8C8
#define VOICE_PILL_LABEL_FONT_PX    34
/* 波形区：贴大卡底部，与下方虚线占位同一带（避免显得「跑到上面」） */
#define VOICE_WAVE_BOTTOM_PAD       36
#define VOICE_RECORD_MIN_CLIP_BYTES  DIVOOM_LEAVE_MSG_MIN_CLIP_BYTES
#define VOICE_RECORD_MIN_WAV_BYTES   VOICE_RECORD_MIN_CLIP_BYTES
/* tone_play 异步启播：前几秒内 get_tone_status 仍可能为 0 */
#define VOICE_PLAY_ASYNC_GUARD_TICKS  8

static void divoom_lvgl_voice_record_tool_push_ctrl_msg(uint8 msg_id, uint8 lvgl_flag)
{
	divoom_sys_msg_info msg = {0};
	uint8 flag = lvgl_flag;

	msg.src_id = DIVOOM_MODEL_LEAVE_MSG;
	msg.dest_id = DIVOOM_MODEL_LEAVE_MSG;
	msg.msg_type = 1;
	msg.msg_id = msg_id;
	if (lvgl_flag) {
		msg.data = &flag;
		msg.data_len = 1;
	}
	divoom_sys_model_crl_msg_push(&msg);
}

typedef struct _divoom_lvgl_voice_record_tool_ctrl
{
	int menu_id;
	lv_style_t bg_style;
	lv_style_t card_style;
	lv_style_t card_focus_style;
	lv_style_t btn_norm_style;
	lv_style_t btn_focus_style;

	lv_obj_t *main_obj;
	lv_obj_t *card_obj;
	lv_obj_t *title_label;
	lv_obj_t *hint_label;
	lv_obj_t *record_btn;
	lv_obj_t *record_img;
	lv_obj_t *time_label;
	lv_obj_t *wave_img;
	lv_obj_t *wave_idle_line;
	lv_obj_t *status_bar;
	lv_obj_t *status_name_label;
	lv_obj_t *status_dur_label;
	lv_obj_t *rerow_btn;
	lv_obj_t *rerow_label;
	lv_obj_t *rerow_icon;

	lv_obj_t *save_frame;
	lv_obj_t *save_btn;
	lv_obj_t *save_label;
	lv_obj_t *cancel_frame;
	lv_obj_t *cancel_btn;
	lv_obj_t *cancel_label;

	void *pdirect_handle;
	lv_timer_t *sec_timer;
	lv_timer_t *wave_anim_timer;
	lv_coord_t wave_img_base_x;
	lv_coord_t wave_img_base_y;
	uint8 wave_phase;

	uint32 record_elapsed_sec;
	uint32 play_elapsed_sec;
	uint8 recording;
	uint8 playing;
	uint8 has_clip;
	uint8 play_async_guard_ticks;

	lv_obj_t *cur_active_obj;
	void *save_confirm_mbox;
	void *cancel_confirm_mbox;
} divoom_lvgl_voice_record_tool_ctrl, *Pdivoom_lvgl_voice_record_tool_ctrl;

static void divoom_lvgl_voice_record_tool_sync_has_clip(Pdivoom_lvgl_voice_record_tool_ctrl ph)
{
	int tmp_sz;
	uint8 on_disk;

	tmp_sz = divoom_leave_msg_record_temp_clip_bytes();
	on_disk = divoom_voice_read_file_is_exit((uint32)DIVOOM_VOICE_ID_LEAVE_MSG) ? (uint8)1 : (uint8)0;
	ph->has_clip = (tmp_sz >= VOICE_RECORD_MIN_WAV_BYTES || on_disk) ? (uint8)1 : (uint8)0;
}

static void divoom_lvgl_voice_record_tool_refresh_hint(Pdivoom_lvgl_voice_record_tool_ctrl ph)
{
	const char *key;

	if (ph->recording)
		key = "VoiceRecordPressToEnd";
	else if (ph->playing)
		key = "VoiceRecordPlaying";
	else if (ph->has_clip)
		key = "VoiceRecordPressToPlay";
	else
		key = "VoiceRecordPressToStart";

	lv_label_set_text(ph->hint_label, divoom_menu_control_get_disp_str(key));
}

static void divoom_lvgl_voice_record_tool_refresh_time(Pdivoom_lvgl_voice_record_tool_ctrl ph)
{
	unsigned mm, ss;
	uint32 sec = 0;

	if (ph->playing) {
		sec = ph->play_elapsed_sec;
	} else if (ph->recording) {
		sec = ph->record_elapsed_sec;
	} else {
		if (divoom_leave_msg_record_temp_clip_bytes() >= VOICE_RECORD_MIN_WAV_BYTES)
			sec = ph->record_elapsed_sec;
		else {
			uint32 ds;
			int mo, dd;

			if (divoom_leave_msg_record_read_last_saved_meta(&ds, &mo, &dd))
				sec = ds;
			else
				sec = ph->record_elapsed_sec;
		}
	}
	mm = (unsigned)(sec / 60u);
	ss = (unsigned)(sec % 60u);
	lv_label_set_text_fmt(ph->time_label, "%02u:%02u", mm, ss);
}

static void divoom_lvgl_voice_record_tool_wave_anim_tick(lv_timer_t *t)
{
	Pdivoom_lvgl_voice_record_tool_ctrl ph = (Pdivoom_lvgl_voice_record_tool_ctrl)lv_timer_get_user_data(t);
	lv_coord_t wobble;

	if ((!ph->recording && !ph->playing) || !ph->wave_img)
		return;
	ph->wave_phase = (uint8)(ph->wave_phase + 1u);
	/* 轻量横向摆动，呈现「动起来」观感（素材为 105_record2.png） */
	wobble = (lv_coord_t)((ph->wave_phase * 5) % 20) - 10;
	lv_obj_set_pos(ph->wave_img, ph->wave_img_base_x + wobble, ph->wave_img_base_y);
}

static void divoom_lvgl_voice_record_tool_build_clip_title(char *buf, size_t buflen, int mon, int mday)
{
	if (!buf || buflen < 8)
		return;
	snprintf(buf, buflen, "%02d-%02d%s", mon, mday, divoom_menu_control_get_disp_str("VoiceRecordClipSuffix"));
}

/** 临时 wav 可入库，或磁盘已有有效 240.bin（无临时亦可「保存」回到表盘+顶栏） */
static int divoom_lvgl_voice_record_tool_can_commit_leave_msg(void)
{
	char path[256];

	if (divoom_leave_msg_record_temp_clip_bytes() >= VOICE_RECORD_MIN_WAV_BYTES)
		return 1;
	if (!divoom_voice_read_file_is_exit((uint32)DIVOOM_VOICE_ID_LEAVE_MSG))
		return 0;
	if (!divoom_voice_read_get_filename((uint32)DIVOOM_VOICE_ID_LEAVE_MSG, path))
		return 0;
	if (!divoom_port_file_exists(path))
		return 0;
	return divoom_app_com_get_file_size(path) >= VOICE_RECORD_MIN_WAV_BYTES;
}

static void divoom_lvgl_voice_record_tool_focus_fix_save_disabled(Pdivoom_lvgl_voice_record_tool_ctrl ph)
{
	lv_group_t *g;
	lv_obj_t *focused;

	if (!ph->save_btn || !lv_obj_has_state(ph->save_btn, LV_STATE_DISABLED))
		return;
	g = lv_group_get_default();
	if (!g)
		return;
	focused = lv_group_get_focused(g);
	if (focused != ph->save_btn)
		return;
	/* lv_group_send_data 对 DISABLED 不派发 KEY — 必须把焦点挪开，否则旋钮会「卡住」 */
	if (ph->rerow_btn)
		lv_group_focus_obj(ph->rerow_btn);
	else if (ph->cancel_btn)
		lv_group_focus_obj(ph->cancel_btn);
	else if (ph->card_obj)
		lv_group_focus_obj(ph->card_obj);
}

/* 旋钮经 group 派发按键；DISABLED 的 save_btn 不接收 LV_EVENT_KEY，导航须跳过该行 */
static lv_obj_t *divoom_lvgl_voice_record_tool_focus_move_skip_disabled_save(
	Pdivoom_lvgl_voice_record_tool_ctrl ph, lv_obj_t *cur_btn, int direct_xy)
{
	lv_obj_t *next = cur_btn;
	int guard;

	for (guard = 0; guard < 8; guard++) {
		next = (lv_obj_t *)divoom_menu_direct_ctrl_get(ph->pdirect_handle, next, direct_xy);
		if (!next || next == cur_btn)
			break;
		cur_btn = next;
		if (!(ph->save_btn && next == ph->save_btn &&
				lv_obj_has_state(ph->save_btn, LV_STATE_DISABLED)))
			break;
	}
	return next ? next : cur_btn;
}

static void divoom_lvgl_voice_record_tool_refresh_save_enabled(Pdivoom_lvgl_voice_record_tool_ctrl ph)
{
	if (!ph->save_btn)
		return;
	if (divoom_lvgl_voice_record_tool_can_commit_leave_msg())
		lv_obj_clear_state(ph->save_btn, LV_STATE_DISABLED);
	else
		lv_obj_add_state(ph->save_btn, LV_STATE_DISABLED);

	divoom_lvgl_voice_record_tool_focus_fix_save_disabled(ph);
}

static void divoom_lvgl_voice_record_tool_refresh_status(Pdivoom_lvgl_voice_record_tool_ctrl ph)
{
	char title[96];
	time_t tnow;
	struct tm ltm;

	if (!ph->status_name_label || !ph->status_dur_label)
		return;

	divoom_lvgl_voice_record_tool_refresh_save_enabled(ph);

	/* 录音中：删除旧临时文件后 has_clip 可能为 0，信息条仍显示当天标题 + 已录时长 */
	if (ph->recording) {
		tnow = time(NULL);
		localtime_r(&tnow, &ltm);
		divoom_lvgl_voice_record_tool_build_clip_title(title, sizeof(title), ltm.tm_mon + 1, ltm.tm_mday);
		lv_label_set_text(ph->status_name_label, title);
		lv_label_set_text_fmt(ph->status_dur_label, "%02u:%02u''",
				(unsigned)(ph->record_elapsed_sec / 60u),
				(unsigned)(ph->record_elapsed_sec % 60u));
		divoom_lvgl_voice_record_tool_refresh_time(ph);
		return;
	}

	if (!ph->has_clip) {
		lv_label_set_text(ph->status_name_label, divoom_menu_control_get_disp_str("VoiceRecordNoClip"));
		lv_label_set_text(ph->status_dur_label, "");
		divoom_lvgl_voice_record_tool_refresh_time(ph);
		return;
	}

	if (divoom_leave_msg_record_temp_clip_bytes() >= VOICE_RECORD_MIN_WAV_BYTES) {
		tnow = time(NULL);
		localtime_r(&tnow, &ltm);
		divoom_lvgl_voice_record_tool_build_clip_title(title, sizeof(title), ltm.tm_mon + 1, ltm.tm_mday);
		lv_label_set_text(ph->status_name_label, title);
		lv_label_set_text_fmt(ph->status_dur_label, "%02u:%02u''",
				(unsigned)(ph->record_elapsed_sec / 60u),
				(unsigned)(ph->record_elapsed_sec % 60u));
	} else if (divoom_voice_read_file_is_exit((uint32)DIVOOM_VOICE_ID_LEAVE_MSG)) {
		uint32 dsec;
		int mon, day;

		if (divoom_leave_msg_record_read_last_saved_meta(&dsec, &mon, &day)) {
			divoom_lvgl_voice_record_tool_build_clip_title(title, sizeof(title), mon, day);
			lv_label_set_text(ph->status_name_label, title);
			lv_label_set_text_fmt(ph->status_dur_label, "%02u:%02u''",
					(unsigned)(dsec / 60u), (unsigned)(dsec % 60u));
		} else {
			lv_label_set_text(ph->status_name_label, divoom_menu_control_get_disp_str("VoiceRecordHasClip"));
			lv_label_set_text(ph->status_dur_label, "--:--''");
		}
	} else {
		lv_label_set_text(ph->status_name_label, divoom_menu_control_get_disp_str("VoiceRecordHasClip"));
		lv_label_set_text(ph->status_dur_label, "");
	}
	divoom_lvgl_voice_record_tool_refresh_time(ph);
}

static void divoom_lvgl_voice_record_tool_apply_card_visual(Pdivoom_lvgl_voice_record_tool_ctrl ph)
{
	if (ph->recording) {
		divoom_menu_control_lv_img_set_src(ph->record_img, "105/105_record_round2.png");
		lv_obj_add_flag(ph->wave_idle_line, LV_OBJ_FLAG_HIDDEN);
		if (ph->wave_img) {
			divoom_menu_control_lv_img_set_src(ph->wave_img, "105/105_record2.png");
			lv_obj_clear_flag(ph->wave_img, LV_OBJ_FLAG_HIDDEN);
		}
		if (ph->wave_anim_timer)
			lv_timer_resume(ph->wave_anim_timer);
	} else if (ph->playing) {
		divoom_menu_control_lv_img_set_src(ph->record_img, "105/105_playing.png");
		lv_obj_add_flag(ph->wave_idle_line, LV_OBJ_FLAG_HIDDEN);
		if (ph->wave_img) {
			divoom_menu_control_lv_img_set_src(ph->wave_img, "105/105_record2.png");
			lv_obj_clear_flag(ph->wave_img, LV_OBJ_FLAG_HIDDEN);
		}
		if (ph->wave_anim_timer)
			lv_timer_resume(ph->wave_anim_timer);
	} else {
		divoom_menu_control_lv_img_set_src(ph->record_img, "105/105_record_round1.png");
		lv_obj_clear_flag(ph->wave_idle_line, LV_OBJ_FLAG_HIDDEN);
		if (ph->wave_img) {
			lv_obj_add_flag(ph->wave_img, LV_OBJ_FLAG_HIDDEN);
			lv_obj_set_pos(ph->wave_img, ph->wave_img_base_x, ph->wave_img_base_y);
		}
		if (ph->wave_anim_timer)
			lv_timer_pause(ph->wave_anim_timer);
		ph->wave_phase = 0;
	}
	divoom_lvgl_voice_record_tool_refresh_hint(ph);
}

static void divoom_lvgl_voice_record_tool_sync_record_state(Pdivoom_lvgl_voice_record_tool_ctrl ph)
{
	int st = divoom_leave_msg_ctrl_get_status();

	if (st == DIVOOM_LEAVE_MSG_START_RECORD) {
		if (!ph->recording) {
			ph->recording = 1;
			ph->record_elapsed_sec = 0;
			divoom_lvgl_voice_record_tool_refresh_time(ph);
			divoom_lvgl_voice_record_tool_apply_card_visual(ph);
		}
		return;
	}
	if (ph->recording && st != DIVOOM_LEAVE_MSG_START_RECORD) {
		ph->recording = 0;
		divoom_lvgl_voice_record_tool_sync_has_clip(ph);
		divoom_lvgl_voice_record_tool_refresh_status(ph);
		divoom_lvgl_voice_record_tool_apply_card_visual(ph);
	}
}

static void divoom_lvgl_voice_record_tool_sec_tick(lv_timer_t *t)
{
	Pdivoom_lvgl_voice_record_tool_ctrl ph = (Pdivoom_lvgl_voice_record_tool_ctrl)lv_timer_get_user_data(t);

	divoom_lvgl_voice_record_tool_sync_record_state(ph);

	if (ph->recording) {
		ph->record_elapsed_sec++;
		divoom_lvgl_voice_record_tool_refresh_time(ph);
		return;
	}
	if (ph->playing) {
		if (divoom_local_music_ctrl_get_tone_status()) {
			ph->play_async_guard_ticks = 0;
			ph->play_elapsed_sec++;
			divoom_lvgl_voice_record_tool_refresh_time(ph);
			/* 播放进度达到录音时长则结束（与底层播放结束可并行） */
			if (ph->record_elapsed_sec > 0 && ph->play_elapsed_sec >= ph->record_elapsed_sec) {
				ph->playing = 0;
				ph->play_async_guard_ticks = 0;
				ph->play_elapsed_sec = 0;
				divoom_lvgl_voice_record_tool_refresh_time(ph);
				divoom_lvgl_voice_record_tool_apply_card_visual(ph);
			}
			return;
		}
		if (ph->play_async_guard_ticks > 0) {
			ph->play_async_guard_ticks--;
			return;
		}
		ph->playing = 0;
		ph->play_async_guard_ticks = 0;
		ph->play_elapsed_sec = 0;
		divoom_lvgl_voice_record_tool_refresh_time(ph);
		divoom_lvgl_voice_record_tool_apply_card_visual(ph);
		return;
	}
}

static void divoom_lvgl_voice_record_tool_toggle_record(Pdivoom_lvgl_voice_record_tool_ctrl ph)
{
	if (!ph->recording) {
		ph->playing = 0;
		ph->play_async_guard_ticks = 0;
		divoom_leave_msg_record_stop_preview_playback();
		divoom_lvgl_voice_record_tool_push_ctrl_msg(DIVOOM_LEAVE_MSG_CTRL_EVENT_LOCAL_RECORD, 1);
		ph->recording = 1;
		ph->record_elapsed_sec = 0;
		divoom_lvgl_voice_record_tool_refresh_time(ph);
		ph->has_clip = 0;
		divoom_lvgl_voice_record_tool_refresh_status(ph);
	} else {
		divoom_lvgl_voice_record_tool_push_ctrl_msg(DIVOOM_LEAVE_MSG_CTRL_EVENT_STOP_RECORD, 0);
		ph->recording = 0;
		divoom_lvgl_voice_record_tool_sync_has_clip(ph);
		divoom_lvgl_voice_record_tool_refresh_status(ph);
	}
	divoom_lvgl_voice_record_tool_apply_card_visual(ph);
}

static void divoom_lvgl_voice_record_tool_save_confirm_cb(void *handle, int ctrl_id)
{
	Pdivoom_lvgl_voice_record_tool_ctrl ph = (Pdivoom_lvgl_voice_record_tool_ctrl)handle;
	uint32 dur_sec;

	if (ph->save_confirm_mbox) {
		divoom_lvgl_message_box_destroy(ph->save_confirm_mbox, 0);
		ph->save_confirm_mbox = NULL;
	}
	if (ctrl_id != 1) {
		divoom_lvgl_voice_record_tool_refresh_save_enabled(ph);
		if (ph->save_btn && !lv_obj_has_state(ph->save_btn, LV_STATE_DISABLED))
			lv_group_focus_obj(ph->save_btn);
		else if (ph->rerow_btn)
			lv_group_focus_obj(ph->rerow_btn);
		return;
	}
	if (!divoom_lvgl_voice_record_tool_can_commit_leave_msg()) {
		divoom_lvgl_voice_record_tool_refresh_save_enabled(ph);
		if (ph->save_btn && !lv_obj_has_state(ph->save_btn, LV_STATE_DISABLED))
			lv_group_focus_obj(ph->save_btn);
		else if (ph->rerow_btn)
			lv_group_focus_obj(ph->rerow_btn);
		return;
	}
	dur_sec = ph->record_elapsed_sec;
	if (!divoom_leave_msg_record_commit_to_leave_msg_file(dur_sec)) {
		divoom_lvgl_voice_record_tool_sync_has_clip(ph);
		divoom_lvgl_voice_record_tool_refresh_status(ph);
		divoom_lvgl_voice_record_tool_refresh_save_enabled(ph);
		if (ph->save_btn && !lv_obj_has_state(ph->save_btn, LV_STATE_DISABLED))
			lv_group_focus_obj(ph->save_btn);
		else if (ph->rerow_btn)
			lv_group_focus_obj(ph->rerow_btn);
		return;
	}
	if (dur_sec == 0u) {
		uint32 ds;
		int mo, dd;

		if (divoom_leave_msg_record_read_last_saved_meta(&ds, &mo, &dd))
			dur_sec = ds;
	}
	divoom_lvgl_voice_record_tool_sync_has_clip(ph);
	divoom_lvgl_voice_record_tool_refresh_status(ph);
	/* 系统表盘 pinit 需要 clock_info，与 divoom_light_app DISP_BASE_LIGHT 分支一致 */
	{
		void *dial_ctr = divoom_light_app_system_dial_clearall_user_data();

		if (dial_ctr)
			divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID, dial_ctr,
					DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_KEY_LIGHT_EVENT);
		else
			divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP,
					DIVOOM_LIGHT_APP_EVENT_START_UP_DISP_DIAL);
	}
	/*
	 * 本回调在 lv_task_handler 内执行，外层已持有 lvgl_mutex。
	 * divoom_system_msg_notice_add 内部会再次 divoom_get_lvgl_lock()，非递归互斥会死锁。
	 */
	divoom_free_lvgl_lock();
	divoom_lvgl_top_leave_msg_notice_init(dur_sec);
	divoom_get_lvgl_lock();
}

static void divoom_lvgl_voice_record_tool_cancel_confirm_cb(void *handle, int ctrl_id)
{
	Pdivoom_lvgl_voice_record_tool_ctrl ph = (Pdivoom_lvgl_voice_record_tool_ctrl)handle;

	if (ph->cancel_confirm_mbox) {
		divoom_lvgl_message_box_destroy(ph->cancel_confirm_mbox, 0);
		ph->cancel_confirm_mbox = NULL;
	}
	if (ctrl_id != 1) {
		divoom_lvgl_voice_record_tool_refresh_save_enabled(ph);
		if (ph->cancel_btn)
			lv_group_focus_obj(ph->cancel_btn);
		return;
	}
	if (ph->playing) {
		divoom_leave_msg_record_stop_preview_playback();
		ph->playing = 0;
		ph->play_async_guard_ticks = 0;
		ph->play_elapsed_sec = 0;
	}
	ph->recording = 0;
	ph->record_elapsed_sec = 0;
	divoom_leave_msg_record_delete_temp_file();
	divoom_leave_msg_record_delete_saved_on_disk();
	divoom_lvgl_voice_record_tool_sync_has_clip(ph);
	divoom_lvgl_voice_record_tool_refresh_status(ph);
	divoom_lvgl_voice_record_tool_refresh_hint(ph);
	divoom_lvgl_voice_record_tool_refresh_time(ph);
	divoom_lvgl_voice_record_tool_apply_card_visual(ph);
	divoom_lvgl_voice_record_tool_refresh_save_enabled(ph);
	if (ph->card_obj)
		lv_group_focus_obj(ph->card_obj);
}

static void divoom_lvgl_voice_record_tool_show_cancel_confirm(Pdivoom_lvgl_voice_record_tool_ctrl ph)
{
	if (ph->cancel_confirm_mbox || ph->save_confirm_mbox)
		return;
	if (ph->recording) {
		ph->recording = 0;
		divoom_lvgl_voice_record_tool_push_ctrl_msg(DIVOOM_LEAVE_MSG_CTRL_EVENT_STOP_RECORD, 0);
		divoom_system_delay_ms(80);
		divoom_lvgl_voice_record_tool_sync_has_clip(ph);
		divoom_lvgl_voice_record_tool_refresh_status(ph);
		divoom_lvgl_voice_record_tool_apply_card_visual(ph);
	}
	if (ph->playing) {
		divoom_leave_msg_record_stop_preview_playback();
		ph->playing = 0;
		ph->play_async_guard_ticks = 0;
		ph->play_elapsed_sec = 0;
		divoom_lvgl_voice_record_tool_refresh_time(ph);
		divoom_lvgl_voice_record_tool_apply_card_visual(ph);
	}
	divoom_lvgl_voice_record_tool_sync_has_clip(ph);
	if (!ph->has_clip)
		return;

	ph->cancel_confirm_mbox = divoom_lvgl_message_box_init(ph->menu_id, ph->main_obj, ph,
			divoom_lvgl_voice_record_tool_cancel_confirm_cb, 80, 320, 640, 408,
			"2/2_confirm_bg.png", lv_color_hex(0xE4E4E4), 2);
	divoom_lvgl_message_box_add_label_item(ph->cancel_confirm_mbox,
			divoom_menu_control_get_disp_str("VoiceRecordCancelMsg"), -1, 34, lv_color_hex(0x1A1A1A),
			LV_TEXT_ALIGN_CENTER, 0, 40, 640, 56);
	divoom_lvgl_message_box_add_label_item(ph->cancel_confirm_mbox,
			divoom_menu_control_get_disp_str("VoiceRecordLeaveCancelConfirm"), -1, 26, lv_color_hex(0x333333),
			LV_TEXT_ALIGN_CENTER, 20, 104, 600, 180);
	divoom_lvgl_message_box_add_btn_item(ph->cancel_confirm_mbox, divoom_menu_control_get_disp_str("Cancel"), 30,
			lv_color_hex(0xE4E4E4), lv_color_hex(0xC8C8C8), lv_color_hex(0), 0, 120, 294, 180, 90);
	divoom_lvgl_message_box_add_btn_item(ph->cancel_confirm_mbox, divoom_menu_control_get_disp_str("Confirm"), 30,
			lv_color_hex(0xE4E4E4), lv_color_hex(0x000000), lv_color_hex(0), 1, 340, 294, 180, 90);
	divoom_lvgl_message_box_set_active_item(ph->cancel_confirm_mbox, 1);
}

static void divoom_lvgl_voice_record_tool_show_save_confirm(Pdivoom_lvgl_voice_record_tool_ctrl ph)
{
	if (ph->save_confirm_mbox || ph->cancel_confirm_mbox)
		return;
	if (ph->recording) {
		ph->recording = 0;
		divoom_lvgl_voice_record_tool_push_ctrl_msg(DIVOOM_LEAVE_MSG_CTRL_EVENT_STOP_RECORD, 0);
		divoom_system_delay_ms(80);
		divoom_lvgl_voice_record_tool_sync_has_clip(ph);
		divoom_lvgl_voice_record_tool_refresh_status(ph);
		divoom_lvgl_voice_record_tool_apply_card_visual(ph);
	}
	if (ph->playing) {
		divoom_leave_msg_record_stop_preview_playback();
		ph->playing = 0;
		ph->play_async_guard_ticks = 0;
		ph->play_elapsed_sec = 0;
		divoom_lvgl_voice_record_tool_refresh_time(ph);
		divoom_lvgl_voice_record_tool_apply_card_visual(ph);
	}
	if (!divoom_lvgl_voice_record_tool_can_commit_leave_msg())
		return;

	ph->save_confirm_mbox = divoom_lvgl_message_box_init(ph->menu_id, ph->main_obj, ph,
			divoom_lvgl_voice_record_tool_save_confirm_cb, 80, 320, 640, 408,
			"2/2_confirm_bg.png", lv_color_hex(0xE4E4E4), 2);

	/* 稿图：标题「保存留言」+ 正文区约 600×180 */
	divoom_lvgl_message_box_add_label_item(ph->save_confirm_mbox,
			divoom_menu_control_get_disp_str("VoiceRecordSaveMsg"), -1, 34, lv_color_hex(0x1A1A1A),
			LV_TEXT_ALIGN_CENTER, 0, 40, 640, 56);
	divoom_lvgl_message_box_add_label_item(ph->save_confirm_mbox,
			divoom_menu_control_get_disp_str("VoiceRecordSaveConfirm"), -1, 26, lv_color_hex(0x333333),
			LV_TEXT_ALIGN_CENTER, 20, 104, 600, 180);
	divoom_lvgl_message_box_add_btn_item(ph->save_confirm_mbox, divoom_menu_control_get_disp_str("Cancel"), 30,
			lv_color_hex(0xE4E4E4), lv_color_hex(0xC8C8C8), lv_color_hex(0), 0, 120, 294, 180, 90);
	divoom_lvgl_message_box_add_btn_item(ph->save_confirm_mbox, divoom_menu_control_get_disp_str("Save"), 30,
			lv_color_hex(0xE4E4E4), lv_color_hex(0x000000), lv_color_hex(0), 1, 340, 294, 180, 90);
	divoom_lvgl_message_box_set_active_item(ph->save_confirm_mbox, 1);
}

static void divoom_lvgl_voice_record_tool_do_rerecord(Pdivoom_lvgl_voice_record_tool_ctrl ph)
{
	divoom_leave_msg_record_stop_preview_playback();
	ph->playing = 0;
	ph->play_async_guard_ticks = 0;
	ph->play_elapsed_sec = 0;
	ph->recording = 0;
	divoom_leave_msg_record_delete_temp_file();
	divoom_leave_msg_record_delete_saved_on_disk();
	ph->record_elapsed_sec = 0;

	/* 稿图：清除当前录音后立即开始新一轮采集 */
	divoom_lvgl_voice_record_tool_push_ctrl_msg(DIVOOM_LEAVE_MSG_CTRL_EVENT_LOCAL_RECORD, 1);
	ph->recording = 1;
	ph->has_clip = 0;
	divoom_lvgl_voice_record_tool_refresh_time(ph);
	divoom_lvgl_voice_record_tool_refresh_status(ph);
	divoom_lvgl_voice_record_tool_refresh_hint(ph);
	divoom_lvgl_voice_record_tool_apply_card_visual(ph);
}

static void divoom_lvgl_voice_record_tool_do_save(Pdivoom_lvgl_voice_record_tool_ctrl ph)
{
	divoom_lvgl_voice_record_tool_show_save_confirm(ph);
}

static void divoom_lvgl_voice_record_tool_do_cancel(Pdivoom_lvgl_voice_record_tool_ctrl ph)
{
	if (ph->cancel_confirm_mbox || ph->save_confirm_mbox)
		return;
	divoom_lvgl_voice_record_tool_sync_has_clip(ph);
	if (!ph->has_clip) {
		if (ph->recording) {
			ph->recording = 0;
			divoom_lvgl_voice_record_tool_push_ctrl_msg(DIVOOM_LEAVE_MSG_CTRL_EVENT_STOP_RECORD, 0);
		}
		if (ph->playing) {
			divoom_leave_msg_record_stop_preview_playback();
			ph->playing = 0;
			ph->play_async_guard_ticks = 0;
			ph->play_elapsed_sec = 0;
		}
		divoom_leave_msg_record_delete_temp_file();
		divoom_menu_control_return_pre();
		return;
	}
	divoom_lvgl_voice_record_tool_show_cancel_confirm(ph);
}

static void divoom_lvgl_voice_record_tool_btn_style(lv_style_t *norm, lv_style_t *focus)
{
	lv_style_init(norm);
	lv_style_set_bg_color(norm, lv_color_hex(0xFFFFFF));
	lv_style_set_bg_opa(norm, LV_OPA_COVER);
	lv_style_set_pad_all(norm, 0);
	lv_style_set_border_width(norm, 2);
	lv_style_set_border_color(norm, lv_color_hex(0xD8D8D8));
	lv_style_set_radius(norm, 20);
	lv_style_set_shadow_width(norm, 0);
	lv_style_set_outline_width(norm, 0);

	lv_style_init(focus);
	lv_style_set_bg_color(focus, lv_color_hex(0xFFFFFF));
	lv_style_set_bg_opa(focus, LV_OPA_COVER);
	lv_style_set_pad_all(focus, 0);
	lv_style_set_border_width(focus, 3);
	lv_style_set_border_color(focus, lv_color_hex(VOICE_FOCUS_BORDER_COLOR));
	lv_style_set_radius(focus, 20);
	lv_style_set_shadow_width(focus, 0);
	lv_style_set_outline_width(focus, 0);
}

static void divoom_lvgl_voice_record_tool_attach_btn(lv_obj_t *btn, lv_style_t *norm, lv_style_t *focus)
{
	lv_obj_add_style(btn, norm, LV_STATE_DEFAULT);
	lv_obj_add_style(btn, focus, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
}

static void divoom_lvgl_voice_record_tool_key_cb(lv_event_t *e);

static void divoom_lvgl_voice_record_tool_card_action(Pdivoom_lvgl_voice_record_tool_ctrl ph)
{
	char path[256];

	if (ph->recording) {
		divoom_lvgl_voice_record_tool_toggle_record(ph);
		return;
	}
	if (ph->playing) {
		ph->playing = 0;
		ph->play_async_guard_ticks = 0;
		ph->play_elapsed_sec = 0;
		divoom_leave_msg_record_stop_preview_playback();
		divoom_lvgl_voice_record_tool_refresh_time(ph);
		divoom_lvgl_voice_record_tool_apply_card_visual(ph);
		return;
	}
	if (ph->has_clip) {
		if (!divoom_leave_msg_record_get_playback_path(path, sizeof(path)))
			return;
		/* 仅磁盘留言时 session 的 record_elapsed_sec 可能为 0，用 meta 时长供播放进度上限 */
		if (divoom_leave_msg_record_temp_clip_bytes() < VOICE_RECORD_MIN_WAV_BYTES) {
			uint32 ds;
			int mo, dd;

			if (divoom_leave_msg_record_read_last_saved_meta(&ds, &mo, &dd))
				ph->record_elapsed_sec = ds;
		}
		ph->playing = 1;
		ph->play_elapsed_sec = 0;
		ph->play_async_guard_ticks = VOICE_PLAY_ASYNC_GUARD_TICKS;
		/* play_time=0：留言不 loop，只播一次 */
		divoom_local_music_ctrl_tone_play(path, divoom_main_get_sys_volume(), NULL, 0);
		divoom_lvgl_voice_record_tool_refresh_time(ph);
		divoom_lvgl_voice_record_tool_apply_card_visual(ph);
		return;
	}
	divoom_lvgl_voice_record_tool_toggle_record(ph);
}

static void divoom_lvgl_voice_record_tool_card_clicked_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED)
		return;
	divoom_lvgl_voice_record_tool_card_action((Pdivoom_lvgl_voice_record_tool_ctrl)lv_event_get_user_data(e));
}

static void divoom_lvgl_voice_record_tool_bind_focus_cb(lv_event_t *e)
{
	Pdivoom_lvgl_voice_record_tool_ctrl ph = (Pdivoom_lvgl_voice_record_tool_ctrl)lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *t = lv_event_get_target(e);

	if (code == LV_EVENT_FOCUSED) {
		ph->cur_active_obj = t;
		if (t == ph->save_btn && ph->save_frame)
			lv_obj_set_style_border_color(ph->save_frame, lv_color_hex(VOICE_FOCUS_BORDER_COLOR), 0);
		if (t == ph->cancel_btn && ph->cancel_frame)
			lv_obj_set_style_border_color(ph->cancel_frame, lv_color_hex(VOICE_FOCUS_BORDER_COLOR), 0);
	} else if (code == LV_EVENT_DEFOCUSED) {
		if (t == ph->save_btn && ph->save_frame)
			lv_obj_set_style_border_color(ph->save_frame, lv_color_hex(VOICE_PILL_FRAME_COLOR), 0);
		if (t == ph->cancel_btn && ph->cancel_frame)
			lv_obj_set_style_border_color(ph->cancel_frame, lv_color_hex(VOICE_PILL_FRAME_COLOR), 0);
	}
}

static void divoom_lvgl_voice_record_tool_key_cb(lv_event_t *e)
{
	Pdivoom_lvgl_voice_record_tool_ctrl ph = (Pdivoom_lvgl_voice_record_tool_ctrl)lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *cur_btn = lv_event_get_target(e);

	if (code != LV_EVENT_KEY)
		return;

	{
		char c = *((char *)lv_event_get_param(e));

		/* 旋钮：PRE → 向下移动焦点；NEXT → 向上移动焦点（竖向 4 项循环） */
		if (c == DIVOOM_LVGL_KEY_PRE) {
			cur_btn = divoom_lvgl_voice_record_tool_focus_move_skip_disabled_save(ph, cur_btn,
					DIVOOM_MENU_DIRECT_DOWN);
			if (cur_btn)
				lv_group_focus_obj(cur_btn);
			return;
		}
		if (c == DIVOOM_LVGL_KEY_NEXT) {
			cur_btn = divoom_lvgl_voice_record_tool_focus_move_skip_disabled_save(ph, cur_btn,
					DIVOOM_MENU_DIRECT_UP);
			if (cur_btn)
				lv_group_focus_obj(cur_btn);
			return;
		}
		if (c == DIVOOM_LVGL_KEY_ENTER) {
			int pos = divoom_menu_direct_ctrl_get_pos(ph->pdirect_handle, cur_btn);

			if (pos == 0)
				divoom_lvgl_voice_record_tool_card_action(ph);
			else if (pos == 1)
				divoom_lvgl_voice_record_tool_do_rerecord(ph);
			else if (pos == 2)
				divoom_lvgl_voice_record_tool_do_save(ph);
			else if (pos == 3)
				divoom_lvgl_voice_record_tool_do_cancel(ph);
			return;
		}
		if (c == DIVOOM_LVGL_KEY_ESC) {
			divoom_lvgl_voice_record_tool_do_cancel(ph);
			return;
		}
		{
			char *key_info = (char *)lv_event_get_param(e);
			divoom_main_keypad_send_key_info((uint8)key_info[0], (uint16)key_info[1]);
		}
	}
}

void *divoom_lvgl_voice_record_tool_menu_init(void *user_data, int menu_id)
{
	lv_obj_t *main_obj;
	Pdivoom_lvgl_voice_record_tool_ctrl ph;

	ph = (Pdivoom_lvgl_voice_record_tool_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_voice_record_tool_ctrl));
	if (!ph)
		return NULL;
	memset(ph, 0, sizeof(*ph));
	ph->menu_id = menu_id;
	/*
	 * direct_ctrl：1 列 × 4 行 → 旋钮竖向切换焦点（顺序自上而下）
	 * y=0 录音大卡 y=1 重新录音 y=2 保存留言 y=3 取消留言（「暂无录音」不可聚焦）
	 */
	ph->pdirect_handle = divoom_menu_direct_ctrl_create(1, 4);
	ph->save_frame = NULL;
	ph->cancel_frame = NULL;

	main_obj = lv_obj_create(NULL);
	ph->main_obj = main_obj;
	lv_style_init(&ph->bg_style);
	/* 中性浅灰底，不透明，减轻屏上偏色（绿/青）透出 */
	lv_style_set_bg_color(&ph->bg_style, lv_color_hex(0xEBEBEB));
	lv_style_set_bg_opa(&ph->bg_style, LV_OPA_COVER);
	lv_style_set_text_color(&ph->bg_style, lv_color_hex(0x1A1A1A));
	lv_obj_add_style(main_obj, &ph->bg_style, 0);
	lv_obj_clear_flag(main_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(main_obj, 0, 0);

	ph->title_label = lv_label_create(main_obj);
	lv_obj_set_width(ph->title_label, DIVOOM_SCREEN_WIDTH - 80);
	lv_obj_align(ph->title_label, LV_ALIGN_TOP_MID, 0, VOICE_TITLE_TOP_Y);
	lv_obj_set_style_text_font(ph->title_label, divoom_menu_control_get_font_info(32, ph->menu_id), 0);
	lv_obj_set_style_text_color(ph->title_label, lv_color_hex(0x1A1A1A), 0);
	lv_obj_set_style_text_align(ph->title_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_text(ph->title_label, divoom_menu_control_get_disp_str("WarmMessage"));

	lv_style_init(&ph->card_style);
	lv_style_set_bg_color(&ph->card_style, lv_color_hex(0xFFFFFF));
	lv_style_set_bg_opa(&ph->card_style, LV_OPA_COVER);
	lv_style_set_radius(&ph->card_style, 28);
	lv_style_set_border_width(&ph->card_style, 1);
	lv_style_set_border_color(&ph->card_style, lv_color_hex(0xE4E4E4));
	lv_style_set_shadow_width(&ph->card_style, 28);
	lv_style_set_shadow_ofs_x(&ph->card_style, 0);
	lv_style_set_shadow_ofs_y(&ph->card_style, 6);
	lv_style_set_shadow_spread(&ph->card_style, 0);
	lv_style_set_shadow_opa(&ph->card_style, LV_OPA_20);
	lv_style_set_shadow_color(&ph->card_style, lv_color_hex(0x000000));
	lv_style_set_pad_all(&ph->card_style, 0);

	/* 聚焦：整块大卡外框加粗描边，颜色与其它可聚焦按钮一致（VOICE_FOCUS_BORDER_COLOR） */
	lv_style_init(&ph->card_focus_style);
	lv_style_set_bg_color(&ph->card_focus_style, lv_color_hex(0xFFFFFF));
	lv_style_set_bg_opa(&ph->card_focus_style, LV_OPA_COVER);
	lv_style_set_radius(&ph->card_focus_style, 28);
	lv_style_set_border_width(&ph->card_focus_style, 3);
	lv_style_set_border_color(&ph->card_focus_style, lv_color_hex(VOICE_FOCUS_BORDER_COLOR));
	lv_style_set_shadow_width(&ph->card_focus_style, 28);
	lv_style_set_shadow_ofs_x(&ph->card_focus_style, 0);
	lv_style_set_shadow_ofs_y(&ph->card_focus_style, 6);
	lv_style_set_shadow_spread(&ph->card_focus_style, 0);
	lv_style_set_shadow_opa(&ph->card_focus_style, LV_OPA_20);
	lv_style_set_shadow_color(&ph->card_focus_style, lv_color_hex(0x000000));
	lv_style_set_pad_all(&ph->card_focus_style, 0);
	lv_style_set_outline_width(&ph->card_focus_style, 0);

	/* 与工具列表页一致：用 lv_btn 才会自动加入 lv_group_get_default()，旋钮/按键才能聚焦 */
	ph->card_obj = lv_btn_create(main_obj);
	lv_obj_clear_flag(ph->card_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(ph->card_obj, VOICE_CARD_W, VOICE_CARD_H);
	lv_obj_set_pos(ph->card_obj, VOICE_CARD_X, VOICE_CARD_Y);
	lv_obj_add_style(ph->card_obj, &ph->card_style, LV_STATE_DEFAULT);
	lv_obj_add_style(ph->card_obj, &ph->card_focus_style, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_outline_width(ph->card_obj, 0, LV_STATE_DEFAULT);
	lv_obj_set_style_outline_width(ph->card_obj, 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);

	ph->hint_label = lv_label_create(ph->card_obj);
	lv_obj_set_width(ph->hint_label, 560);
	lv_obj_set_pos(ph->hint_label, (VOICE_CARD_W - 560) / 2, 28);
	lv_obj_set_style_text_font(ph->hint_label, divoom_menu_control_get_font_info(26, ph->menu_id), 0);
	lv_obj_set_style_text_color(ph->hint_label, lv_color_hex(0x666666), 0);
	lv_obj_set_style_text_align(ph->hint_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_long_mode(ph->hint_label, LV_LABEL_LONG_WRAP);
	divoom_lvgl_voice_record_tool_refresh_hint(ph);

	ph->record_btn = lv_obj_create(ph->card_obj);
	lv_obj_set_size(ph->record_btn, 72, 72);
	lv_obj_set_pos(ph->record_btn, (VOICE_CARD_W - 72) / 2, 99);
	lv_obj_set_style_bg_opa(ph->record_btn, LV_OPA_TRANSP, LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ph->record_btn, 0, LV_STATE_DEFAULT);
	lv_obj_set_style_outline_width(ph->record_btn, 0, LV_STATE_DEFAULT);
	lv_obj_clear_flag(ph->record_btn, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_clear_flag(ph->record_btn, LV_OBJ_FLAG_CLICKABLE);

	ph->record_img = lv_img_create(ph->record_btn);
	divoom_menu_control_lv_img_set_src(ph->record_img, "105/105_record_round1.png");
	lv_obj_center(ph->record_img);

	ph->time_label = lv_label_create(ph->card_obj);
	lv_obj_set_width(ph->time_label, 640);
	lv_obj_set_pos(ph->time_label, (VOICE_CARD_W - 640) / 2, 198);
	lv_obj_set_style_text_font(ph->time_label, divoom_menu_control_get_font_info(36, ph->menu_id), 0);
	lv_obj_set_style_text_color(ph->time_label, lv_color_hex(0x222222), 0);
	lv_obj_set_style_text_align(ph->time_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_text(ph->time_label, "00:00");

	ph->wave_img = lv_img_create(ph->card_obj);
	divoom_menu_control_lv_img_set_src(ph->wave_img, "105/105_record2.png");
	lv_obj_align(ph->wave_img, LV_ALIGN_BOTTOM_MID, 0, -VOICE_WAVE_BOTTOM_PAD);
	ph->wave_img_base_x = lv_obj_get_x(ph->wave_img);
	ph->wave_img_base_y = lv_obj_get_y(ph->wave_img);
	ph->wave_phase = 0;
	lv_obj_add_flag(ph->wave_img, LV_OBJ_FLAG_HIDDEN);

	/* 未录音：底部虚线占位；录音中：105_record2.png + 轻摆动 */
	/* 未录音虚线占位：与波形条同一水平带，便于状态切换时位置一致 */
	{
		static lv_point_t wave_idle_pts[] = {{0, 0}, {600, 0}};
		lv_coord_t wave_x = (VOICE_CARD_W - 600) / 2;
		/* 虚线约在波形图垂直中心略下，贴近底部留白 */
		lv_coord_t wave_line_y = VOICE_CARD_H - VOICE_WAVE_BOTTOM_PAD - 8;

		ph->wave_idle_line = lv_line_create(ph->card_obj);
		lv_obj_set_pos(ph->wave_idle_line, wave_x, wave_line_y);
		lv_line_set_points(ph->wave_idle_line, wave_idle_pts, 2);
		lv_obj_set_style_line_color(ph->wave_idle_line, lv_color_hex(0xAAAAAA), 0);
		lv_obj_set_style_line_width(ph->wave_idle_line, 2, 0);
		lv_obj_set_style_line_rounded(ph->wave_idle_line, 1, 0);
		lv_obj_set_style_line_dash_width(ph->wave_idle_line, 8, 0);
		lv_obj_set_style_line_dash_gap(ph->wave_idle_line, 6, 0);
	}

	divoom_lvgl_voice_record_tool_btn_style(&ph->btn_norm_style, &ph->btn_focus_style);

	ph->wave_anim_timer = lv_timer_create(divoom_lvgl_voice_record_tool_wave_anim_tick, 90, ph);
	lv_timer_pause(ph->wave_anim_timer);

	lv_obj_add_event_cb(ph->card_obj, divoom_lvgl_voice_record_tool_key_cb, LV_EVENT_KEY, ph);
	lv_obj_add_event_cb(ph->card_obj, divoom_lvgl_voice_record_tool_card_clicked_cb, LV_EVENT_CLICKED, ph);
	lv_obj_add_event_cb(ph->card_obj, divoom_lvgl_voice_record_tool_bind_focus_cb, LV_EVENT_FOCUSED, ph);
	lv_obj_add_event_cb(ph->card_obj, divoom_lvgl_voice_record_tool_bind_focus_cb, LV_EVENT_DEFOCUSED, ph);
	divoom_menu_direct_ctrl_update(ph->pdirect_handle, ph->card_obj, 0, 0);
	ph->cur_active_obj = ph->card_obj;

	{
		lv_coord_t y = VOICE_CARD_Y + VOICE_CARD_H + VOICE_GAP_AFTER_CARD;

		ph->status_bar = lv_obj_create(main_obj);
		lv_obj_set_size(ph->status_bar, VOICE_ROW_FULL_W, VOICE_STATUS_H);
		lv_obj_set_pos(ph->status_bar, VOICE_ROW_FULL_X, y);
		lv_obj_set_style_bg_color(ph->status_bar, lv_color_hex(0xFFFFFF), 0);
		lv_obj_set_style_bg_opa(ph->status_bar, LV_OPA_COVER, 0);
		/* 与「重新录音」条同款描边/圆角，保证文案左边线与按钮内标题对齐 */
		lv_obj_set_style_radius(ph->status_bar, 20, 0);
		lv_obj_set_style_border_width(ph->status_bar, 2, 0);
		lv_obj_set_style_border_color(ph->status_bar, lv_color_hex(0xD8D8D8), 0);
		lv_obj_set_style_pad_all(ph->status_bar, 0, 0);
		lv_obj_clear_flag(ph->status_bar, LV_OBJ_FLAG_SCROLLABLE);

		ph->status_name_label = lv_label_create(ph->status_bar);
		lv_obj_set_width(ph->status_name_label, 450);
		lv_obj_align(ph->status_name_label, LV_ALIGN_LEFT_MID, VOICE_ROW_TEXT_PAD, 0);
		lv_obj_set_style_text_align(ph->status_name_label, LV_TEXT_ALIGN_LEFT, 0);
		lv_obj_set_style_text_font(ph->status_name_label, divoom_menu_control_get_font_info(26, ph->menu_id), 0);
		lv_obj_set_style_text_color(ph->status_name_label, lv_color_hex(0x333333), 0);
		lv_label_set_long_mode(ph->status_name_label, LV_LABEL_LONG_DOT);

		ph->status_dur_label = lv_label_create(ph->status_bar);
		lv_obj_set_width(ph->status_dur_label, 200);
		lv_obj_align(ph->status_dur_label, LV_ALIGN_RIGHT_MID, -VOICE_ROW_TEXT_PAD, 0);
		lv_obj_set_style_text_align(ph->status_dur_label, LV_TEXT_ALIGN_RIGHT, 0);
		lv_obj_set_style_text_font(ph->status_dur_label, divoom_menu_control_get_font_info(26, ph->menu_id), 0);
		lv_obj_set_style_text_color(ph->status_dur_label, lv_color_hex(0x333333), 0);
		divoom_lvgl_voice_record_tool_refresh_status(ph);

		y += VOICE_STATUS_H + VOICE_GAP_STATUS_TO_REROW;
		ph->rerow_btn = lv_btn_create(main_obj);
		lv_obj_set_size(ph->rerow_btn, VOICE_ROW_FULL_W, VOICE_REROW_H);
		lv_obj_set_pos(ph->rerow_btn, VOICE_ROW_FULL_X, y);
		divoom_lvgl_voice_record_tool_attach_btn(ph->rerow_btn, &ph->btn_norm_style, &ph->btn_focus_style);

		ph->rerow_label = lv_label_create(ph->rerow_btn);
		lv_obj_set_style_text_font(ph->rerow_label, divoom_menu_control_get_font_info(26, ph->menu_id), 0);
		lv_obj_set_style_text_color(ph->rerow_label, lv_color_black(), 0);
		lv_label_set_text(ph->rerow_label, divoom_menu_control_get_disp_str("VoiceRecordReRecord"));
		lv_obj_align(ph->rerow_label, LV_ALIGN_LEFT_MID, VOICE_ROW_TEXT_PAD, 0);

		ph->rerow_icon = lv_img_create(ph->rerow_btn);
		divoom_menu_control_lv_img_set_src(ph->rerow_icon, "105/105_re_record.png");
		lv_obj_align(ph->rerow_icon, LV_ALIGN_RIGHT_MID, -VOICE_ROW_TEXT_PAD, 0);

		lv_obj_add_event_cb(ph->rerow_btn, divoom_lvgl_voice_record_tool_key_cb, LV_EVENT_KEY, ph);
		lv_obj_add_event_cb(ph->rerow_btn, divoom_lvgl_voice_record_tool_bind_focus_cb, LV_EVENT_FOCUSED, ph);
		lv_obj_add_event_cb(ph->rerow_btn, divoom_lvgl_voice_record_tool_bind_focus_cb, LV_EVENT_DEFOCUSED, ph);
		divoom_menu_direct_ctrl_update(ph->pdirect_handle, ph->rerow_btn, 0, 1);

		y += VOICE_REROW_H + VOICE_GAP_REROW_TO_SAVE;
		{
			lv_obj_t *save_outer;
			lv_coord_t ow = VOICE_ROW_FULL_W - VOICE_PILL_FRAME_PAD * 2;
			lv_coord_t oh = VOICE_SAVE_H - VOICE_PILL_FRAME_PAD * 2;
			lv_coord_t iw = ow - VOICE_PILL_INSET * 2;
			lv_coord_t ih = oh - VOICE_PILL_INSET * 2;
			lv_coord_t ir = ih / 2;

			ph->save_frame = lv_obj_create(main_obj);
			lv_obj_set_size(ph->save_frame, VOICE_ROW_FULL_W, VOICE_SAVE_H);
			lv_obj_set_pos(ph->save_frame, VOICE_ROW_FULL_X, y);
			lv_obj_clear_flag(ph->save_frame, LV_OBJ_FLAG_SCROLLABLE);
			lv_obj_set_style_bg_opa(ph->save_frame, LV_OPA_TRANSP, 0);
			lv_obj_set_style_border_width(ph->save_frame, VOICE_PILL_FRAME_BW, 0);
			lv_obj_set_style_border_color(ph->save_frame, lv_color_hex(VOICE_PILL_FRAME_COLOR), 0);
			lv_obj_set_style_radius(ph->save_frame, VOICE_SAVE_H / 2, 0);

			save_outer = lv_obj_create(ph->save_frame);
			lv_obj_set_size(save_outer, ow, oh);
			lv_obj_center(save_outer);
			lv_obj_clear_flag(save_outer, LV_OBJ_FLAG_SCROLLABLE);
			lv_obj_set_style_bg_color(save_outer, lv_color_hex(VOICE_PILL_RING_BG), 0);
			lv_obj_set_style_bg_opa(save_outer, LV_OPA_COVER, 0);
			lv_obj_set_style_border_width(save_outer, VOICE_PILL_OUTER_BW, 0);
			lv_obj_set_style_border_color(save_outer, lv_color_hex(VOICE_PILL_OUTER_BORDER), 0);
			lv_obj_set_style_radius(save_outer, oh / 2, 0);

			ph->save_btn = lv_btn_create(save_outer);
			lv_obj_set_size(ph->save_btn, iw, ih);
			lv_obj_center(ph->save_btn);
			divoom_lvgl_voice_record_tool_attach_btn(ph->save_btn, &ph->btn_norm_style, &ph->btn_focus_style);
			lv_obj_set_style_radius(ph->save_btn, ir, LV_STATE_DEFAULT);
			lv_obj_set_style_radius(ph->save_btn, ir, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
			lv_obj_set_style_border_width(ph->save_btn, 0, LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(ph->save_btn, 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
			lv_obj_set_style_bg_color(ph->save_btn, lv_color_hex(VOICE_PILL_BG), LV_STATE_DEFAULT);
			lv_obj_set_style_bg_color(ph->save_btn, lv_color_hex(VOICE_PILL_BG_FOCUS), LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
			lv_obj_set_style_outline_width(ph->save_btn, 0, LV_STATE_DEFAULT);
			lv_obj_set_style_outline_width(ph->save_btn, 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
			ph->save_label = lv_label_create(ph->save_btn);
			lv_obj_set_style_text_font(ph->save_label, divoom_menu_control_get_font_info(VOICE_PILL_LABEL_FONT_PX, ph->menu_id), 0);
			lv_obj_set_style_text_color(ph->save_label, lv_color_hex(VOICE_PILL_TEXT), 0);
			lv_label_set_text(ph->save_label, divoom_menu_control_get_disp_str("VoiceRecordSaveMsg"));
			lv_obj_center(ph->save_label);
			lv_obj_add_event_cb(ph->save_btn, divoom_lvgl_voice_record_tool_key_cb, LV_EVENT_KEY, ph);
			lv_obj_add_event_cb(ph->save_btn, divoom_lvgl_voice_record_tool_bind_focus_cb, LV_EVENT_FOCUSED, ph);
			lv_obj_add_event_cb(ph->save_btn, divoom_lvgl_voice_record_tool_bind_focus_cb, LV_EVENT_DEFOCUSED, ph);
			divoom_menu_direct_ctrl_update(ph->pdirect_handle, ph->save_btn, 0, 2);
		}

		y += VOICE_SAVE_H + VOICE_GAP_SAVE_TO_CANCEL;
		{
			lv_obj_t *cancel_outer;
			lv_coord_t ow = VOICE_ROW_FULL_W - VOICE_PILL_FRAME_PAD * 2;
			lv_coord_t oh = VOICE_CANCEL_H - VOICE_PILL_FRAME_PAD * 2;
			lv_coord_t iw = ow - VOICE_PILL_INSET * 2;
			lv_coord_t ih = oh - VOICE_PILL_INSET * 2;
			lv_coord_t ir = ih / 2;

			ph->cancel_frame = lv_obj_create(main_obj);
			lv_obj_set_size(ph->cancel_frame, VOICE_ROW_FULL_W, VOICE_CANCEL_H);
			lv_obj_set_pos(ph->cancel_frame, VOICE_ROW_FULL_X, y);
			lv_obj_clear_flag(ph->cancel_frame, LV_OBJ_FLAG_SCROLLABLE);
			lv_obj_set_style_bg_opa(ph->cancel_frame, LV_OPA_TRANSP, 0);
			lv_obj_set_style_border_width(ph->cancel_frame, VOICE_PILL_FRAME_BW, 0);
			lv_obj_set_style_border_color(ph->cancel_frame, lv_color_hex(VOICE_PILL_FRAME_COLOR), 0);
			lv_obj_set_style_radius(ph->cancel_frame, VOICE_CANCEL_H / 2, 0);

			cancel_outer = lv_obj_create(ph->cancel_frame);
			lv_obj_set_size(cancel_outer, ow, oh);
			lv_obj_center(cancel_outer);
			lv_obj_clear_flag(cancel_outer, LV_OBJ_FLAG_SCROLLABLE);
			lv_obj_set_style_bg_color(cancel_outer, lv_color_hex(VOICE_PILL_RING_BG), 0);
			lv_obj_set_style_bg_opa(cancel_outer, LV_OPA_COVER, 0);
			lv_obj_set_style_border_width(cancel_outer, VOICE_PILL_OUTER_BW, 0);
			lv_obj_set_style_border_color(cancel_outer, lv_color_hex(VOICE_PILL_OUTER_BORDER), 0);
			lv_obj_set_style_radius(cancel_outer, oh / 2, 0);

			ph->cancel_btn = lv_btn_create(cancel_outer);
			lv_obj_set_size(ph->cancel_btn, iw, ih);
			lv_obj_center(ph->cancel_btn);
			divoom_lvgl_voice_record_tool_attach_btn(ph->cancel_btn, &ph->btn_norm_style, &ph->btn_focus_style);
			lv_obj_set_style_radius(ph->cancel_btn, ir, LV_STATE_DEFAULT);
			lv_obj_set_style_radius(ph->cancel_btn, ir, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
			lv_obj_set_style_border_width(ph->cancel_btn, 0, LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(ph->cancel_btn, 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
			lv_obj_set_style_bg_color(ph->cancel_btn, lv_color_hex(VOICE_PILL_BG), LV_STATE_DEFAULT);
			lv_obj_set_style_bg_color(ph->cancel_btn, lv_color_hex(VOICE_PILL_BG_FOCUS), LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
			lv_obj_set_style_outline_width(ph->cancel_btn, 0, LV_STATE_DEFAULT);
			lv_obj_set_style_outline_width(ph->cancel_btn, 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
			ph->cancel_label = lv_label_create(ph->cancel_btn);
			lv_obj_set_style_text_font(ph->cancel_label, divoom_menu_control_get_font_info(VOICE_PILL_LABEL_FONT_PX, ph->menu_id), 0);
			lv_obj_set_style_text_color(ph->cancel_label, lv_color_hex(VOICE_PILL_TEXT), 0);
			lv_label_set_text(ph->cancel_label, divoom_menu_control_get_disp_str("VoiceRecordCancelMsg"));
			lv_obj_center(ph->cancel_label);
			lv_obj_add_event_cb(ph->cancel_btn, divoom_lvgl_voice_record_tool_key_cb, LV_EVENT_KEY, ph);
			lv_obj_add_event_cb(ph->cancel_btn, divoom_lvgl_voice_record_tool_bind_focus_cb, LV_EVENT_FOCUSED, ph);
			lv_obj_add_event_cb(ph->cancel_btn, divoom_lvgl_voice_record_tool_bind_focus_cb, LV_EVENT_DEFOCUSED, ph);
			divoom_menu_direct_ctrl_update(ph->pdirect_handle, ph->cancel_btn, 0, 3);
		}
	}

	divoom_lvgl_voice_record_tool_refresh_save_enabled(ph);

	ph->sec_timer = lv_timer_create(divoom_lvgl_voice_record_tool_sec_tick, 1000, ph);

	divoom_lvgl_voice_record_tool_apply_card_visual(ph);

	(void)user_data;

	return ph;
}

void divoom_lvgl_voice_record_tool_menu_disp(void *handle, void *active)
{
	Pdivoom_lvgl_voice_record_tool_ctrl ph = (Pdivoom_lvgl_voice_record_tool_ctrl)handle;
	lv_group_t *g = lv_group_get_default();

	lv_label_set_text(ph->title_label, divoom_menu_control_get_disp_str("WarmMessage"));
	divoom_lvgl_voice_record_tool_sync_has_clip(ph);
	divoom_lvgl_voice_record_tool_refresh_hint(ph);
	lv_label_set_text(ph->rerow_label, divoom_menu_control_get_disp_str("VoiceRecordReRecord"));
	lv_label_set_text(ph->save_label, divoom_menu_control_get_disp_str("VoiceRecordSaveMsg"));
	lv_label_set_text(ph->cancel_label, divoom_menu_control_get_disp_str("VoiceRecordCancelMsg"));
	divoom_lvgl_voice_record_tool_refresh_status(ph);

	lv_scr_load(ph->main_obj);

	/* 与 divoom_lvgl_tool_menu_disp 一致：切屏后再设焦点；两者都有时以外部 active 为准 */
	if (active) {
		lv_group_focus_obj((lv_obj_t *)active);
		ph->cur_active_obj = NULL;
	} else if (ph->cur_active_obj) {
		lv_group_focus_obj(ph->cur_active_obj);
		ph->cur_active_obj = NULL;
	} else if (g && ph->card_obj)
		lv_group_focus_obj(ph->card_obj);
}

void divoom_lvgl_voice_record_tool_menu_exit(void *handle)
{
	Pdivoom_lvgl_voice_record_tool_ctrl ph = (Pdivoom_lvgl_voice_record_tool_ctrl)handle;

	if (ph->save_confirm_mbox) {
		divoom_lvgl_message_box_destroy(ph->save_confirm_mbox, 0);
		ph->save_confirm_mbox = NULL;
	}
	if (ph->cancel_confirm_mbox) {
		divoom_lvgl_message_box_destroy(ph->cancel_confirm_mbox, 0);
		ph->cancel_confirm_mbox = NULL;
	}

	divoom_leave_msg_record_stop_preview_playback();
	divoom_lvgl_voice_record_tool_push_ctrl_msg(DIVOOM_LEAVE_MSG_CTRL_EVENT_LVGL_EXIT, 0);
	divoom_app_com_del_file((char *)DIVOOM_LEAVE_MSG_RECORD_TMP_MP3);

	if (ph->sec_timer)
		lv_timer_del(ph->sec_timer);
	ph->sec_timer = NULL;

	if (ph->wave_anim_timer) {
		lv_timer_del(ph->wave_anim_timer);
		ph->wave_anim_timer = NULL;
	}

	divoom_menu_direct_ctrl_destory(ph->pdirect_handle);
	ph->pdirect_handle = NULL;

	lv_style_reset(&ph->bg_style);
	lv_style_reset(&ph->card_style);
	lv_style_reset(&ph->card_focus_style);
	lv_style_reset(&ph->btn_norm_style);
	lv_style_reset(&ph->btn_focus_style);
	lv_obj_del(ph->main_obj);
	divoom_system_free(ph);
}

#endif /* legacy voice_record_tool */

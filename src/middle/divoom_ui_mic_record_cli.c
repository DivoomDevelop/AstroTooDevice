#include "divoom_finsh_autotest.h"
#if DIVOOM_FINSH_AUTOTEST

/*
 * Finsh: ui_mic_record_test — 通过 LVGL 按键消息走用户 UI 流程测助眠/闹钟录音+播放
 *
 * 导航依据 divoom_astrootoo_sleep.c / divoom_astrootoo_alarm.c 的真实焦点状态机；
 * 每步按键后读取白盒探针，未到目标 UI 则继续按键或报错退出。
 *
 *   ui_mic_record_test leave   — 表盘 → 主菜单 → 工具箱 → 温馨留言（astrootoo_tools MEMO）
 *   ui_mic_record_test sleep
 *   ui_mic_record_test alarm
 *   ui_mic_record_test all
 */
#include "divoom.h"
#include <console.h>
#include <stdio.h>
#include <string.h>

#include "divoom_astrootoo_alarm.h"
#include "divoom_astrootoo_main_menu.h"
#include "divoom_astrootoo_sleep.h"
#include "divoom_astrootoo_tools.h"
#include "divoom_leave_msg_ctrl.h"
#include "divoom_leave_msg_record.h"
#include "divoom_local_music_ctrl.h"
#include "divoom_menu_control.h"
#include "divoom_mic_record.h"
#include "divoom_sleep_aid_ctrl.h"
#include "divoom_user_key.h"

#define UI_MIC_REC_KEY_DELAY_MS      400
#define UI_MIC_REC_RECORD_SEC        3
#define UI_MIC_REC_MENU_WAIT_MS      10000
#define UI_MIC_REC_MENU_STABLE_MS    1200
#define UI_MIC_REC_SLEEP_SETTLE_MS   1200
#define UI_MIC_REC_ALARM_SETTLE_MS   2000
#define UI_MIC_REC_PLAY_WAIT_MS      10000
#define UI_MIC_REC_STEP_TIMEOUT_MS   6000
#define UI_MIC_REC_LEAVE_SETTLE_MS   800
#define UI_MIC_REC_LEAVE_INTERRUPT_ROUNDS  3
#define UI_MIC_REC_LEAVE_MIN_BYTES   DIVOOM_LEAVE_MSG_MIN_CLIP_BYTES
/* divoom_astrootoo_main_menu.c g_astrootoo_main_items[] "Tool Box" */
#define UI_LEAVE_MAIN_TOOL_IDX       4
/* divoom_astrootoo_tools.c g_tools_rows[] "VoiceRecordTool" */
#define UI_LEAVE_TOOLS_MEMO_ROW      4
#define UI_MIC_REC_ALARM_TMP_MP3     DIVOOM_TMP_DIR "/divoom_alarm_record_cap.mp3"
#define UI_MIC_REC_ALARM_MIN_BYTES   128

/* divoom_astrootoo_sleep.c */
#define UI_SLEEP_TAB_REC             5
#define UI_SLEEP_PANE_STRIP          0
#define UI_SLEEP_PANE_SUB            2
#define UI_SLEEP_SUB_KIND_REC_BTN    8
#define UI_SLEEP_SUB_KIND_REC_CLIP   9
#define UI_SLEEP_REC_SUB_COUNT       2

/* divoom_astrootoo_alarm.c */
#define UI_ALARM_MODE_LIST           0
#define UI_ALARM_MODE_EDIT           1
#define UI_ALARM_MODE_BELL           4
#define UI_ALARM_BELL_PHASE_RECORD   3
#define UI_ALARM_CAT_RECORD          2
#define UI_ALARM_EDIT_BELLS          2
#define UI_ALARM_REC_FOCUS_PANEL     0
#define UI_ALARM_REC_FOCUS_FILE      1

/*
 * AstroToo 物理键 → DIVOOM_KEY_*（divoom_user_key.h）
 *   ENTER = PLAY = DIVOOM_LVGL_KEY_ENTER（各页 is_enter）
 *   LEFT  = MUSIC_PREV = DIVOOM_LVGL_KEY_LEFT（主菜单/助眠顶栏横向）
 *   RIGHT = MUSIC_NEXT = DIVOOM_LVGL_KEY_RIGHT
 *   UP    = PREV = DIVOOM_LVGL_KEY_UP（工具列表纵向 is_up）
 *   DOWN  = NEXT = DIVOOM_LVGL_KEY_DOWN（工具列表纵向 is_down）
 */
#define UI_MIC_REC_KEY_ENTER         DIVOOM_KEY_PLAY
#define UI_MIC_REC_KEY_LEFT          DIVOOM_KEY_MUSIC_PREV
#define UI_MIC_REC_KEY_RIGHT         DIVOOM_KEY_MUSIC_NEXT
#define UI_MIC_REC_KEY_UP            DIVOOM_KEY_PREV
#define UI_MIC_REC_KEY_DOWN          DIVOOM_KEY_NEXT

static void ui_mic_rec_key(int key_id)
{
	divoom_user_key_send_key_info(key_id, DIVOOM_PRESS_SHORT);
	divoom_system_delay_ms(UI_MIC_REC_KEY_DELAY_MS);
}

static int ui_mic_rec_wait_menu_stable(int menu_id, int timeout_ms, int stable_ms)
{
	unsigned t0 = divoom_system_get_tickms();
	unsigned stable_start = 0;

	while (divoom_system_get_tickms() - t0 < (unsigned)timeout_ms)
	{
		if (divoom_menu_control_get_active_menu_id() == menu_id)
		{
			if (stable_start == 0)
			{
				stable_start = divoom_system_get_tickms();
			}
			else if (divoom_system_get_tickms() - stable_start >= (unsigned)stable_ms)
			{
				return 1;
			}
		}
		else
		{
			stable_start = 0;
		}
		divoom_system_delay_ms(50);
	}
	return 0;
}

static int ui_mic_rec_open_menu(int menu_id, int settle_ms)
{
	int attempt;

	for (attempt = 0; attempt < 5; attempt++)
	{
		if (attempt > 0)
		{
			divoom_system_delay_ms(800);
		}
		if (attempt == 0)
		{
			divoom_menu_control_load_next(menu_id, NULL, 0, 0);
		}
		else
		{
			divoom_menu_control_clearall_load(menu_id, NULL, 0, 0);
		}
		if (!ui_mic_rec_wait_menu_stable(menu_id, UI_MIC_REC_MENU_WAIT_MS, UI_MIC_REC_MENU_STABLE_MS))
		{
			printf("UI_MIC_RECORD_TEST: open menu=%d retry=%d active=%d\n",
			       menu_id, attempt, divoom_menu_control_get_active_menu_id());
			continue;
		}
		divoom_system_delay_ms((unsigned)settle_ms);
		if (divoom_menu_control_get_active_menu_id() == menu_id)
		{
			return 1;
		}
	}
	return 0;
}

static void ui_mic_rec_sleep_log(const char *tag)
{
	printf("UI_MIC_RECORD_TEST: sleep %s menu=%d tab=%d pane=%d sub=%d cnt=%d kind=%d busy=%d\n",
	       tag,
	       divoom_menu_control_get_active_menu_id(),
	       divoom_astrootoo_sleep_whitebox_get_active_tab(),
	       divoom_astrootoo_sleep_whitebox_get_focus_pane(),
	       divoom_astrootoo_sleep_whitebox_get_sub_focus(),
	       divoom_astrootoo_sleep_whitebox_get_sub_count(),
	       divoom_astrootoo_sleep_whitebox_get_sub_kind(),
	       divoom_sleep_aid_ctrl_record_busy());
}

static void ui_mic_rec_alarm_log(const char *tag)
{
	printf("UI_MIC_RECORD_TEST: alarm %s menu=%d mode=%d bell=%d cat=%d edit=%d recf=%d cap=%d\n",
	       tag,
	       divoom_menu_control_get_active_menu_id(),
	       divoom_astrootoo_alarm_whitebox_get_mode(),
	       divoom_astrootoo_alarm_whitebox_get_bell_phase(),
	       divoom_astrootoo_alarm_whitebox_get_bell_cat_focus(),
	       divoom_astrootoo_alarm_whitebox_get_edit_focus(),
	       divoom_astrootoo_alarm_whitebox_get_rec_focus(),
	       divoom_astrootoo_alarm_whitebox_get_rec_capture_active());
}

static int ui_mic_rec_wait_tone(unsigned timeout_ms)
{
	unsigned t0 = divoom_system_get_tickms();

	while (divoom_system_get_tickms() - t0 < timeout_ms)
	{
		if (divoom_local_music_ctrl_get_tone_status())
		{
			return 1;
		}
		divoom_system_delay_ms(20);
	}
	return 0;
}

static void ui_mic_rec_cleanup_audio(void)
{
	divoom_local_music_ctrl_stop_tone();
	divoom_local_music_ctrl_voice_clip_record_stop_sync();
	divoom_leave_msg_ctrl_abort_local_record();
	divoom_system_delay_ms(150);
}

static void ui_mic_rec_leave_log(const char *tag)
{
	printf("UI_MIC_RECORD_TEST: leave %s menu=%d main=%d page=%d list=%d memo_st=%d st=%d clip_s=%d tmp=%d mic=%d\n",
	       tag,
	       divoom_menu_control_get_active_menu_id(),
	       divoom_astrootoo_main_menu_whitebox_get_focus_index(),
	       divoom_astrootoo_tools_whitebox_get_page(),
	       divoom_astrootoo_tools_whitebox_get_list_focus(),
	       divoom_astrootoo_tools_whitebox_get_record_state(),
	       divoom_leave_msg_ctrl_get_status(),
	       divoom_astrootoo_tools_whitebox_get_record_clip_sec(),
	       divoom_leave_msg_record_temp_clip_bytes(),
	       divoom_local_music_ctrl_voice_clip_record_is_active());
}

static int ui_mic_rec_leave_wait_recording(unsigned timeout_ms)
{
	unsigned t0 = divoom_system_get_tickms();

	while (divoom_system_get_tickms() - t0 < timeout_ms) {
		if (divoom_leave_msg_ctrl_get_status() == DIVOOM_LEAVE_MSG_START_RECORD
			&& divoom_local_music_ctrl_voice_clip_record_is_active())
			return 1;
		divoom_system_delay_ms(50);
	}
	return 0;
}

static int ui_mic_rec_leave_is_idle(void)
{
	return !divoom_local_music_ctrl_voice_clip_record_is_active()
		&& divoom_leave_msg_ctrl_get_status() != DIVOOM_LEAVE_MSG_START_RECORD;
}

static int ui_mic_rec_leave_wait_idle(unsigned timeout_ms)
{
	unsigned t0 = divoom_system_get_tickms();

	while (divoom_system_get_tickms() - t0 < timeout_ms) {
		if (ui_mic_rec_leave_is_idle())
			return 1;
		divoom_system_delay_ms(50);
	}
	return 0;
}

static int ui_mic_rec_leave_discard_via_ui(unsigned timeout_ms)
{
	int i;

	for (i = 0; i < 3; i++) {
		ui_mic_rec_key(DIVOOM_KEY_ESC);
		if (ui_mic_rec_leave_wait_idle(timeout_ms / 3u))
			return 1;
		divoom_system_delay_ms(200);
	}
	if (!ui_mic_rec_leave_is_idle()) {
		divoom_leave_msg_ctrl_abort_local_record();
		divoom_leave_msg_record_delete_temp_file();
		divoom_astrootoo_tools_memo_sync_ui_idle();
	}
	return ui_mic_rec_leave_wait_idle(3000u);
}

/* 主菜单轮播：LEFT/RIGHT（is_prev/is_next 含 DIVOOM_LVGL_KEY_LEFT/RIGHT） */
static int ui_mic_rec_leave_goto_main_item(int target_idx)
{
	int guard;

	for (guard = 0; guard < 12; guard++)
	{
		int idx = divoom_astrootoo_main_menu_whitebox_get_focus_index();

		if (idx == target_idx)
		{
			ui_mic_rec_leave_log("main_tool_focus");
			return 1;
		}
		if (idx < 0)
		{
			break;
		}
		if (idx < target_idx)
		{
			ui_mic_rec_key(UI_MIC_REC_KEY_RIGHT);
		}
		else
		{
			ui_mic_rec_key(UI_MIC_REC_KEY_LEFT);
		}
	}
	ui_mic_rec_leave_log("err=main_focus");
	return 0;
}

static int ui_mic_rec_leave_enter_toolbox(void)
{
	ui_mic_rec_key(UI_MIC_REC_KEY_ENTER);
	if (!ui_mic_rec_wait_menu_stable(DIVOOM_MENU_CONTROL_ID_TOOL,
					UI_MIC_REC_MENU_WAIT_MS, UI_MIC_REC_MENU_STABLE_MS))
	{
		ui_mic_rec_leave_log("err=tool_menu");
		return 0;
	}
	if (divoom_astrootoo_tools_whitebox_get_page() != DIVOOM_ASTROTOO_TOOLS_PAGE_LIST)
	{
		ui_mic_rec_leave_log("err=tool_page");
		return 0;
	}
	ui_mic_rec_leave_log("tool_list_ready");
	return 1;
}

/* 工具列表：UP/DOWN（tools_list_key_cb is_up/is_down） */
static int ui_mic_rec_leave_goto_tools_row(int target_row)
{
	int guard;

	for (guard = 0; guard < 10; guard++)
	{
		int row = divoom_astrootoo_tools_whitebox_get_list_focus();

		if (row == target_row)
		{
			ui_mic_rec_leave_log("tool_row_focus");
			return 1;
		}
		if (row < 0)
		{
			break;
		}
		if (row < target_row)
		{
			ui_mic_rec_key(UI_MIC_REC_KEY_DOWN);
		}
		else
		{
			ui_mic_rec_key(UI_MIC_REC_KEY_UP);
		}
	}
	ui_mic_rec_leave_log("err=tool_row");
	return 0;
}

static int ui_mic_rec_leave_enter_memo(void)
{
	unsigned t0;

	ui_mic_rec_key(UI_MIC_REC_KEY_ENTER);
	t0 = divoom_system_get_tickms();
	while (divoom_system_get_tickms() - t0 < 12000u)
	{
		if (divoom_menu_control_get_active_menu_id() == DIVOOM_MENU_CONTROL_ID_VOICE_RECORD_TOOL_MENU
			&& divoom_astrootoo_tools_whitebox_get_page() == DIVOOM_ASTROTOO_TOOLS_PAGE_MEMO
			&& divoom_astrootoo_tools_whitebox_get_record_state() >= 0)
		{
			ui_mic_rec_leave_log("memo_ready");
			return 1;
		}
		divoom_system_delay_ms(50);
	}
	ui_mic_rec_leave_log("err=memo_enter");
	return 0;
}

static int ui_mic_rec_leave_wait_state(int want_state, unsigned timeout_ms)
{
	unsigned t0 = divoom_system_get_tickms();

	while (divoom_system_get_tickms() - t0 < timeout_ms) {
		if (divoom_astrootoo_tools_whitebox_get_record_state() == want_state)
			return 1;
		divoom_system_delay_ms(50);
	}
	return 0;
}

static int ui_mic_rec_leave_wait_clip(unsigned timeout_ms)
{
	unsigned t0 = divoom_system_get_tickms();

	while (divoom_system_get_tickms() - t0 < timeout_ms) {
		if (divoom_leave_msg_record_temp_clip_bytes() >= UI_MIC_REC_LEAVE_MIN_BYTES)
			return 1;
		if (divoom_astrootoo_tools_whitebox_get_record_clip_sec() > 0
			&& divoom_leave_msg_ctrl_get_duration_sec() > 0)
			return 1;
		divoom_system_delay_ms(50);
	}
	return 0;
}

/* 表盘：ENTER 进主菜单（divoom_lvgl_dial_menu.c DIVOOM_LVGL_KEY_ENTER → clearall_load MAIN） */
static int ui_mic_rec_leave_wait_dial(unsigned timeout_ms)
{
	unsigned t0 = divoom_system_get_tickms();

	while (divoom_system_get_tickms() - t0 < timeout_ms)
	{
		int mid = divoom_menu_control_get_active_menu_id();

		if (mid == DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID)
		{
			ui_mic_rec_leave_log("dial_ready");
			return 1;
		}
		divoom_system_delay_ms(100);
	}
	ui_mic_rec_leave_log("err=dial_wait");
	return 0;
}

static int ui_mic_rec_leave_enter_main_from_dial(void)
{
	ui_mic_rec_key(UI_MIC_REC_KEY_ENTER);
	if (!ui_mic_rec_wait_menu_stable(DIVOOM_MENU_CONTROL_ID_MAIN,
					UI_MIC_REC_MENU_WAIT_MS, UI_MIC_REC_MENU_STABLE_MS))
	{
		ui_mic_rec_leave_log("err=main_enter");
		return 0;
	}
	if (divoom_astrootoo_main_menu_whitebox_get_focus_index() < 0)
	{
		ui_mic_rec_leave_log("err=main_focus");
		return 0;
	}
	ui_mic_rec_leave_log("main_ready");
	divoom_system_delay_ms((unsigned)UI_MIC_REC_LEAVE_SETTLE_MS);
	return 1;
}

static int ui_mic_rec_test_leave(void)
{
	int interrupt_ok = 0;
	int clip_ok = 0;
	int round;
	int ok;

	printf("UI_MIC_RECORD_TEST: begin case=leave\n");
	ui_mic_rec_cleanup_audio();
	divoom_leave_msg_record_delete_temp_file();
	divoom_leave_msg_record_delete_saved_on_disk();

	if (!ui_mic_rec_leave_wait_dial(60000u))
	{
		return 0;
	}
	if (!ui_mic_rec_leave_enter_main_from_dial())
	{
		return 0;
	}
	if (!ui_mic_rec_leave_goto_main_item(UI_LEAVE_MAIN_TOOL_IDX))
	{
		return 0;
	}
	if (!ui_mic_rec_leave_enter_toolbox())
	{
		return 0;
	}
	if (!ui_mic_rec_leave_goto_tools_row(UI_LEAVE_TOOLS_MEMO_ROW))
	{
		return 0;
	}
	if (!ui_mic_rec_leave_enter_memo())
	{
		return 0;
	}
	divoom_system_delay_ms(500);

	for (round = 0; round < UI_MIC_REC_LEAVE_INTERRUPT_ROUNDS; round++)
	{
		ui_mic_rec_key(UI_MIC_REC_KEY_ENTER);
		if (!ui_mic_rec_leave_wait_recording(8000u))
		{
			ui_mic_rec_leave_log("err=rec_start");
			return 0;
		}
		ui_mic_rec_leave_log("recording");
		divoom_system_delay_ms(400);
		if (!ui_mic_rec_leave_discard_via_ui(8000u))
		{
			ui_mic_rec_leave_log("err=rec_discard");
			return 0;
		}
		interrupt_ok++;
		ui_mic_rec_leave_log("interrupt_ok");
		divoom_system_delay_ms(600);
	}

	ui_mic_rec_key(UI_MIC_REC_KEY_ENTER);
	if (!ui_mic_rec_leave_wait_recording(8000u))
	{
		ui_mic_rec_leave_log("err=final_start");
		return 0;
	}
	ui_mic_rec_leave_log("final_recording");
	divoom_system_delay_ms((unsigned)UI_MIC_REC_RECORD_SEC * 1000u);
	ui_mic_rec_key(UI_MIC_REC_KEY_ENTER);
	if (!ui_mic_rec_leave_wait_idle(8000u))
	{
		ui_mic_rec_leave_log("err=final_stop");
		return 0;
	}
	clip_ok = ui_mic_rec_leave_wait_clip(8000u);
	if (!clip_ok)
	{
		ui_mic_rec_leave_log("err=final_clip");
		return 0;
	}
	ui_mic_rec_leave_log("clip_ok");

	ok = (interrupt_ok == UI_MIC_REC_LEAVE_INTERRUPT_ROUNDS) && clip_ok;
	printf("UI_MIC_RECORD_TEST: case=leave ok=%d interrupt_ok=%d clip_ok=%d mic_active=%d\n",
	       ok, interrupt_ok, clip_ok, divoom_mic_record_is_active());
	return ok;
}

/* 顶栏 RIGHT 切 Tab；异步 build 完成后 sub_count 才反映当前 Tab 内容 */
static int ui_mic_rec_sleep_goto_tab(int target_tab)
{
	unsigned t0 = divoom_system_get_tickms();
	int guard;

	for (guard = 0; guard < 12; guard++)
	{
		int tab = divoom_astrootoo_sleep_whitebox_get_active_tab();

		if (tab == target_tab)
		{
			break;
		}
		if (divoom_astrootoo_sleep_whitebox_get_focus_pane() != UI_SLEEP_PANE_STRIP)
		{
			ui_mic_rec_key(DIVOOM_KEY_ESC);
		}
		ui_mic_rec_key(DIVOOM_KEY_MUSIC_NEXT);
	}
	t0 = divoom_system_get_tickms();
	while (divoom_system_get_tickms() - t0 < (unsigned)UI_MIC_REC_STEP_TIMEOUT_MS)
	{
		if (divoom_astrootoo_sleep_whitebox_get_active_tab() == target_tab
			&& divoom_astrootoo_sleep_whitebox_get_sub_count() == UI_SLEEP_REC_SUB_COUNT)
		{
			ui_mic_rec_sleep_log("tab_ready");
			return 1;
		}
		divoom_system_delay_ms(50);
	}
	ui_mic_rec_sleep_log("err=tab_timeout");
	return 0;
}

/* 顶栏 DOWN/ENTER -> 子页（sleep_enter_content） */
static int ui_mic_rec_sleep_enter_sub(void)
{
	unsigned t0 = divoom_system_get_tickms();
	int guard;

	for (guard = 0; guard < 4; guard++)
	{
		if (divoom_astrootoo_sleep_whitebox_get_focus_pane() == UI_SLEEP_PANE_SUB)
		{
			ui_mic_rec_sleep_log("sub_ready");
			return 1;
		}
		ui_mic_rec_key(DIVOOM_KEY_PLAY);
	}
	t0 = divoom_system_get_tickms();
	while (divoom_system_get_tickms() - t0 < (unsigned)UI_MIC_REC_STEP_TIMEOUT_MS)
	{
		if (divoom_astrootoo_sleep_whitebox_get_focus_pane() == UI_SLEEP_PANE_SUB)
		{
			ui_mic_rec_sleep_log("sub_ready");
			return 1;
		}
		divoom_system_delay_ms(50);
	}
	ui_mic_rec_sleep_log("err=sub_enter_timeout");
	return 0;
}

static int ui_mic_rec_sleep_goto_sub_kind(int kind)
{
	int guard;

	for (guard = 0; guard < 6; guard++)
	{
		if (divoom_astrootoo_sleep_whitebox_get_sub_kind() == kind)
		{
			ui_mic_rec_sleep_log(kind == UI_SLEEP_SUB_KIND_REC_BTN ? "at_rec_btn" : "at_rec_clip");
			return 1;
		}
		if (kind == UI_SLEEP_SUB_KIND_REC_BTN)
		{
			ui_mic_rec_key(DIVOOM_KEY_NEXT);
		}
		else
		{
			ui_mic_rec_key(DIVOOM_KEY_PREV);
		}
	}
	ui_mic_rec_sleep_log("err=sub_kind_timeout");
	return 0;
}

static int ui_mic_rec_test_sleep(void)
{
	int ok = 0;
	int clip_ok = 0;
	int play_ok = 0;
	int saw_busy = 0;

	printf("UI_MIC_RECORD_TEST: begin case=sleep\n");
	ui_mic_rec_cleanup_audio();

	if (!ui_mic_rec_open_menu(DIVOOM_MENU_CONTROL_ID_SLEEP_AID, UI_MIC_REC_SLEEP_SETTLE_MS))
	{
		printf("UI_MIC_RECORD_TEST: case=sleep ok=0 err=menu_timeout active=%d\n",
		       divoom_menu_control_get_active_menu_id());
		return 0;
	}
	ui_mic_rec_sleep_log("menu_ready");

	if (!ui_mic_rec_sleep_goto_tab(UI_SLEEP_TAB_REC))
	{
		return 0;
	}
	if (!ui_mic_rec_sleep_enter_sub())
	{
		return 0;
	}
	if (!ui_mic_rec_sleep_goto_sub_kind(UI_SLEEP_SUB_KIND_REC_BTN))
	{
		return 0;
	}

	ui_mic_rec_key(DIVOOM_KEY_PLAY);
	{
		unsigned t0 = divoom_system_get_tickms();
		while (divoom_system_get_tickms() - t0 < (unsigned)UI_MIC_REC_STEP_TIMEOUT_MS)
		{
			if (divoom_sleep_aid_ctrl_record_busy())
			{
				saw_busy = 1;
				break;
			}
			divoom_system_delay_ms(50);
		}
	}
	if (!saw_busy)
	{
		ui_mic_rec_sleep_log("err=rec_start");
		return 0;
	}
	ui_mic_rec_sleep_log("recording");

	divoom_system_delay_ms((unsigned)UI_MIC_REC_RECORD_SEC * 1000u);

	ui_mic_rec_key(DIVOOM_KEY_PLAY);
	{
		unsigned t0 = divoom_system_get_tickms();
		while (divoom_system_get_tickms() - t0 < 8000u)
		{
			if (divoom_sleep_aid_ctrl_record_busy())
			{
				saw_busy = 1;
			}
			if (saw_busy
				&& !divoom_sleep_aid_ctrl_record_busy()
				&& divoom_sleep_aid_ctrl_record_clip_count() > 0)
			{
				clip_ok = 1;
				break;
			}
			divoom_system_delay_ms(50);
		}
	}
	if (!clip_ok)
	{
		ui_mic_rec_sleep_log("err=rec_stop");
		return 0;
	}
	ui_mic_rec_sleep_log("clip_ok");

	/*
	 * 录音回放行：仅 UP 聚焦即可试听（sleep_apply_sub_voice -> select_voice -> tone_play）。
	 * 禁止 ENTER：会走 sleep_enter_play_from_sub_row -> enter_play_voice，
	 * 进入助眠播放页并尝试播不存在的视频，真机会死机。
	 */
	if (!ui_mic_rec_sleep_goto_sub_kind(UI_SLEEP_SUB_KIND_REC_CLIP))
	{
		return 0;
	}
	ui_mic_rec_sleep_log("focus_rec_clip_preview");
	play_ok = ui_mic_rec_wait_tone(UI_MIC_REC_PLAY_WAIT_MS);
	if (play_ok)
	{
		divoom_system_delay_ms(500);
		divoom_sleep_aid_ctrl_stop_play_voice();
	}
	ui_mic_rec_key(DIVOOM_KEY_ESC);

	ok = clip_ok && play_ok;
	printf("UI_MIC_RECORD_TEST: case=sleep ok=%d clip_ok=%d play_ok=%d clips=%d mic_active=%d\n",
	       ok, clip_ok, play_ok, divoom_sleep_aid_ctrl_record_clip_count(),
	       divoom_mic_record_is_active());
	return ok;
}

static int ui_mic_rec_alarm_wait_mode(int mode)
{
	unsigned t0 = divoom_system_get_tickms();

	while (divoom_system_get_tickms() - t0 < (unsigned)UI_MIC_REC_STEP_TIMEOUT_MS)
	{
		if (divoom_astrootoo_alarm_whitebox_get_mode() == mode)
		{
			return 1;
		}
		divoom_system_delay_ms(50);
	}
	return 0;
}

static int ui_mic_rec_alarm_goto_edit_bells(void)
{
	int guard;

	if (!ui_mic_rec_alarm_wait_mode(UI_ALARM_MODE_EDIT))
	{
		ui_mic_rec_alarm_log("err=not_edit");
		return 0;
	}
	for (guard = 0; guard < 6; guard++)
	{
		if (divoom_astrootoo_alarm_whitebox_get_edit_focus() == UI_ALARM_EDIT_BELLS)
		{
			break;
		}
		ui_mic_rec_key(DIVOOM_KEY_NEXT);
	}
	if (divoom_astrootoo_alarm_whitebox_get_edit_focus() != UI_ALARM_EDIT_BELLS)
	{
		ui_mic_rec_alarm_log("err=bells_row");
		return 0;
	}
	ui_mic_rec_key(DIVOOM_KEY_PLAY);
	if (!ui_mic_rec_alarm_wait_mode(UI_ALARM_MODE_BELL))
	{
		ui_mic_rec_alarm_log("err=bell_mode");
		return 0;
	}
	ui_mic_rec_alarm_log("at_bells");
	return 1;
}

static int ui_mic_rec_alarm_goto_record_page(void)
{
	int guard;

	if (divoom_astrootoo_alarm_whitebox_get_mode() != UI_ALARM_MODE_BELL)
	{
		ui_mic_rec_alarm_log("err=not_bell");
		return 0;
	}
	for (guard = 0; guard < 6; guard++)
	{
		if (divoom_astrootoo_alarm_whitebox_get_bell_phase() == UI_ALARM_BELL_PHASE_RECORD)
		{
			ui_mic_rec_alarm_log("at_record_page");
			return 1;
		}
		if (divoom_astrootoo_alarm_whitebox_get_bell_cat_focus() != UI_ALARM_CAT_RECORD)
		{
			ui_mic_rec_key(DIVOOM_KEY_NEXT);
			continue;
		}
		ui_mic_rec_key(DIVOOM_KEY_PLAY);
		divoom_system_delay_ms(300);
	}
	ui_mic_rec_alarm_log("err=record_page");
	return 0;
}

static int ui_mic_rec_alarm_goto_rec_file_row(void)
{
	int guard;

	for (guard = 0; guard < 5; guard++)
	{
		if (divoom_astrootoo_alarm_whitebox_get_rec_focus() == UI_ALARM_REC_FOCUS_FILE)
		{
			ui_mic_rec_alarm_log("at_rec_file");
			return 1;
		}
		ui_mic_rec_key(DIVOOM_KEY_NEXT);
	}
	ui_mic_rec_alarm_log("err=rec_file_row");
	return 0;
}

static int ui_mic_rec_test_alarm(void)
{
	int ok = 0;
	int clip_ok = 0;
	int play_ok = 0;
	int saw_busy = 0;

	printf("UI_MIC_RECORD_TEST: begin case=alarm\n");
	ui_mic_rec_cleanup_audio();

	if (!ui_mic_rec_open_menu(DIVOOM_MENU_CONTROL_ID_ALARM_LIST_MENU, UI_MIC_REC_ALARM_SETTLE_MS))
	{
		printf("UI_MIC_RECORD_TEST: case=alarm ok=0 err=menu_timeout active=%d\n",
		       divoom_menu_control_get_active_menu_id());
		return 0;
	}
	ui_mic_rec_alarm_log("menu_ready");

	if (!ui_mic_rec_alarm_wait_mode(UI_ALARM_MODE_LIST))
	{
		ui_mic_rec_alarm_log("err=not_list");
		return 0;
	}
	ui_mic_rec_key(DIVOOM_KEY_PREV);
	ui_mic_rec_key(DIVOOM_KEY_PLAY);
	if (!ui_mic_rec_alarm_wait_mode(UI_ALARM_MODE_EDIT))
	{
		ui_mic_rec_alarm_log("err=edit_mode");
		return 0;
	}
	ui_mic_rec_alarm_log("at_edit");

	if (!ui_mic_rec_alarm_goto_edit_bells())
	{
		return 0;
	}
	if (!ui_mic_rec_alarm_goto_record_page())
	{
		return 0;
	}
	if (divoom_astrootoo_alarm_whitebox_get_rec_focus() != UI_ALARM_REC_FOCUS_PANEL)
	{
		ui_mic_rec_key(DIVOOM_KEY_PREV);
	}

	ui_mic_rec_key(DIVOOM_KEY_PLAY);
	{
		unsigned t0 = divoom_system_get_tickms();
		while (divoom_system_get_tickms() - t0 < (unsigned)UI_MIC_REC_STEP_TIMEOUT_MS)
		{
			if (divoom_astrootoo_alarm_whitebox_get_rec_capture_active()
				|| divoom_local_music_ctrl_voice_clip_record_is_active()
				|| divoom_mic_record_is_active())
			{
				saw_busy = 1;
				break;
			}
			divoom_system_delay_ms(50);
		}
	}
	if (!saw_busy)
	{
		ui_mic_rec_alarm_log("err=rec_start");
		return 0;
	}
	ui_mic_rec_alarm_log("recording");

	divoom_system_delay_ms((unsigned)UI_MIC_REC_RECORD_SEC * 1000u);

	ui_mic_rec_key(DIVOOM_KEY_PLAY);
	{
		unsigned t0 = divoom_system_get_tickms();
		while (divoom_system_get_tickms() - t0 < 8000u)
		{
			if (divoom_local_music_ctrl_voice_clip_record_is_active()
				|| divoom_mic_record_is_active())
			{
				saw_busy = 1;
			}
			if (saw_busy
				&& !divoom_local_music_ctrl_voice_clip_record_is_active()
				&& !divoom_mic_record_is_active())
			{
				int sz = divoom_app_com_get_file_size((char *)UI_MIC_REC_ALARM_TMP_MP3);
				if (sz >= UI_MIC_REC_ALARM_MIN_BYTES)
				{
					clip_ok = 1;
					break;
				}
			}
			divoom_system_delay_ms(50);
		}
	}
	if (!clip_ok)
	{
		ui_mic_rec_alarm_log("err=rec_stop");
		return 0;
	}
	ui_mic_rec_alarm_log("clip_ok");

	if (!ui_mic_rec_alarm_goto_rec_file_row())
	{
		return 0;
	}
	ui_mic_rec_key(DIVOOM_KEY_PLAY);
	play_ok = ui_mic_rec_wait_tone(UI_MIC_REC_PLAY_WAIT_MS);
	if (play_ok)
	{
		divoom_system_delay_ms(500);
		divoom_local_music_ctrl_stop_tone();
	}

	ok = clip_ok && play_ok;
	printf("UI_MIC_RECORD_TEST: case=alarm ok=%d clip_ok=%d play_ok=%d mic_active=%d\n",
	       ok, clip_ok, play_ok, divoom_mic_record_is_active());
	return ok;
}

static int ui_mic_rec_test_cli(int argc, char **argv)
{
	const char *which = "all";
	int fail = 0;
	int ok_count = 0;

	if (argc >= 2)
	{
		which = argv[1];
	}

	printf("UI_MIC_RECORD_TEST: suite begin which=%s\n", which);

	if (strcmp(which, "sleep") == 0)
	{
		ok_count = ui_mic_rec_test_sleep();
		fail = ok_count ? 0 : 1;
	}
	else if (strcmp(which, "alarm") == 0)
	{
		ok_count = ui_mic_rec_test_alarm();
		fail = ok_count ? 0 : 1;
	}
	else if (strcmp(which, "leave") == 0)
	{
		ok_count = ui_mic_rec_test_leave();
		fail = ok_count ? 0 : 1;
	}
	else if (strcmp(which, "all") == 0)
	{
		if (ui_mic_rec_test_leave())
		{
			ok_count++;
		}
		else
		{
			fail++;
		}
		divoom_system_delay_ms(1000);
		ui_mic_rec_cleanup_audio();
		if (ui_mic_rec_test_sleep())
		{
			ok_count++;
		}
		else
		{
			fail++;
		}
		divoom_system_delay_ms(1000);
		ui_mic_rec_cleanup_audio();
		if (ui_mic_rec_test_alarm())
		{
			ok_count++;
		}
		else
		{
			fail++;
		}
	}
	else
	{
		printf("UI_MIC_RECORD_TEST: usage: ui_mic_record_test [leave|sleep|alarm|all]\n");
		return 1;
	}

	printf("UI_MIC_RECORD_TEST: done ok=%d fail=%d\n", ok_count, fail);
	return fail ? 1 : 0;
}

FINSH_FUNCTION_EXPORT_CMD(ui_mic_rec_test_cli, ui_mic_record_test,
			  leave/sleep/alarm UI key-flow mic record autotest);

#endif /* DIVOOM_FINSH_AUTOTEST */

#include "divoom.h"
#include "divoom_alarm.h"
#include "divoom_music_ctrl.h"
#if defined(_WIN32) || defined(CONFIG_DIVOOM_MPP_NET_AUDIO)
#include "divoom_radio.h"
#define DIVOOM_MUSIC_RADIO_SOURCE 1
#else
#define DIVOOM_MUSIC_RADIO_SOURCE 0
#endif
#ifdef CONFIG_DIVOOM_MPP_NET_AUDIO
#include "divoom_net_audio.h"
#endif
#if !defined(DIVOOM_OPENSOURCE_SWITCH) && defined(CONFIG_DIVOOM_AI_VOICE) && DIVOOM_AI_VOICE_DIAL_TEST_ENABLE
#include "divoom_ai_voice_dial_test.h"
#endif
#include "divoom_lvgl_top_source_switch_menu.h"
#include "divoom_lvgl_poweron.h"
#include "divoom_astrootoo_source.h"
#include "divoom_power_monitor.h"
#include "divoom_ota_server.h"
#if DIVOOM_BUILD_RTOS_DEVICE
#include "divoom_ota_jl.h"
#endif

/* 避免依赖 bt_manager.h 路径；与 divoom_bt_music.h 中声明一致 */
void divoom_bt_music_init(void);
void divoom_bt_music_deinit(void);
int divoom_bt_music_exit_bt_system(void);
int divoom_bt_music_start_classic_bt(void);
void divoom_bt_music_set_play_status(uint8 status);
void divoom_bt_music_set_play_next(void);
void divoom_bt_music_set_play_pre(void);
void divoom_bt_music_set_play_vol(uint8 value);
void divoom_bt_music_mark_sent_volume(int old_vol, int vol);
int divoom_bt_music_is_connected(void);
int divoom_bt_music_is_a2dp_playing(void);
#ifndef BTMG_AVRCP_PLAY
#define BTMG_AVRCP_PLAY   0
#define BTMG_AVRCP_PAUSE  1
#endif
#include "divoom_line_in.h"
#include "divoom_uac.h"

#ifdef DIVOOM_SIMULATOR
#include "middle/include/divoom_local_music_ctrl.h"
#endif
#if DIVOOM_SHOW_TF_NAND
#include "divoom_local_music_ctrl.h"
#include "divoom_audio_sink.h"
#endif

/*********************************************************************
 * 音乐模式管理模块
 * 
 * 管理多种音乐模式的互斥切换:
 *   1. 蓝牙音乐 (BT) - 通过管道与蓝牙管理进程通信
 *   2. TF卡音乐 (LOCAL) - 本地文件播放
 *   3. Line-In - ALSA音频直接输入输出
 *   4. UAC - USB音频类，从PC/手机USB输入播放
 * 
 * 同一时刻只能有一种模式在工作
 * 
 * 特性：当手机端在非蓝牙模式下开始播放时，会自动切换到蓝牙模式
 **********************************************************************/

/* 不定义则经典蓝牙常驻：切出蓝牙模式仅暂停+关音频，不退出 BT；手机端播放时自动切回蓝牙模式 */
//#define DIVOOM_CLASSIC_BT_SWITCH

typedef struct _divoom_music_ctrl_ctrl
{
    uint8 mode;                     // 当前音乐模式
    uint8 last_mode;                // 上一个模式（用于恢复）
    uint32 mode_generation;         /* 切源事务代号：调度回调期间的新请求使旧起播事务失效，包含ABA切换。 */
    uint8 source_transition;        /* 语音push期间只收集许可，随后一次性提交新音源。 */
    uint8 source_voice_changed;
    uint8 source_voice_allowed;
    uint8 playing;                  // 当前是否在播放
    uint8 transport_state;          // 已确认的播放/暂停/终止，供 AI 调度只读
    uint8 volume;                   // 当前音量 0-100（缓存，用于 BT/LOCAL 等无 get 接口的模式）
    uint8 hfp_call_level;           // HFP 通话档 0～15
    uint8 hfp_call_level_valid;    // 已收到 JL 0x6F 或本地设过
    uint8 bt_music_initialized;     // 蓝牙音乐模块是否已初始化
    uint8 line_in_initialized;      // Line-In模块是否已初始化
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
    uint8 uac_initialized;          // UAC模块是否已初始化
#endif
    void *source_lock; /* 串行切源及电台停止的语音占用提交；回调不取此锁。锁序source→music→radio。 */
    void *lock_handle;              // 线程锁
}divoom_music_ctrl_ctrl, *Pdivoom_music_ctrl_ctrl;

static Pdivoom_music_ctrl_ctrl pdivoom_music_ctrl_handle = NULL;

static uint8 divoom_music_ctrl_get_saved_volume_for_mode(uint8 mode);
static uint8 divoom_music_ctrl_low_power_cap_ui(uint8 vol);

/* 语音调度抢占时保存的播放状态，用于 pause/play 恢复 */
static uint8 s_music_voice_saved_playing;
static uint8 s_tone_overlay_saved_playing;
static uint8 s_music_ctrl_init_done;
static void divoom_music_ctrl_start_mode(uint8 mode, uint8 allow_voice_output,
					 uint8 jl_already_in_mode, uint8 send_jl_set, int32 radio_generation);
static void divoom_music_ctrl_switch_mode_ex(uint8 mode, uint8 send_jl_set, uint8 voice_owned, int32 radio_generation);

void divoom_music_ctrl_apply_mode(uint8 mode);

/* JL：等 SET 的 0x39 回包后再发 TF music_id；RX 里只延后 set_mode */
#define DIVOOM_MUSIC_CTRL_JL_WM_NONE  0xffu
static uint8 s_expect_jl_wm = DIVOOM_MUSIC_CTRL_JL_WM_NONE;
static uint8 s_jl_tf_ready;
static uint8 s_jl_sd_music_ready_seen;
static uint8 s_pending_tf_play;
static volatile uint8 s_jl_defer_mode;
static volatile uint8 s_jl_defer_pending;
static volatile uint8 s_jl_ui_refresh_pending;
static volatile uint8 s_jl_sync_pending;
static volatile uint8 s_jl_tf_verify_get;
static volatile uint8 s_jl_defer_task_run;
static uint32_t s_ignore_jl_apply_until_ms;
static uint8 s_jl_passive_work_mode;
static uint8 s_jl_passive_work_mode_valid;
static uint8 s_tf_file_update_session;
static uint8 s_jl_pre_sleep_work_mode = DIVOOM_MUSIC_CTRL_JL_WM_NONE;
static uint8 s_jl_sleep_mode_active;
static volatile uint8 s_jl_set_verify_task_run;
static uint8 s_jl_set_verify_retry;
static uint32_t s_jl_set_verify_until_ms;
#define DIVOOM_MUSIC_CTRL_JL_SET_VERIFY_MS  500u
static uint8 s_tf_pending_play_vol;
static uint8 s_tf_pending_play_vol_valid;
static uint32_t s_tf_insert_play_defer_until_ms;
static volatile uint8 s_tf_play_defer_task_run;
#define DIVOOM_MUSIC_CTRL_TF_READY_WAIT_MS  3000u
static uint32_t s_tf_ready_wait_deadline_ms;
static volatile uint8 s_tf_ready_wd_task_run;
static uint8 s_tf_ready_wd_retry;
static volatile uint8 s_tf_unplug_hold_local;
static volatile uint8 s_tf_unplug_follow_pending;
static divoom_music_ctrl_tf_play_info s_tf_play_info;

static int divoom_music_ctrl_ignore_jl_leave_tf(uint8 new_local)
{
#if DIVOOM_SHOW_TF_NAND
	if (pdivoom_music_ctrl_handle != NULL
	    && pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL
	    && new_local != DIVOOM_MUSIC_CTRL_MODE_LOCAL)
		return 1;
#endif
	if (!s_tf_unplug_hold_local)
		return 0;
	if (pdivoom_music_ctrl_handle == NULL)
		return 0;
	if (pdivoom_music_ctrl_handle->mode != DIVOOM_MUSIC_CTRL_MODE_LOCAL)
		return 0;
	if (new_local == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
		return 0;
	return 1;
}

static void divoom_music_ctrl_tf_session_leave(void);
static void divoom_music_ctrl_clear_jl_tf_state(void);
static void divoom_music_ctrl_jl_tf_mode_entered(void);
static void divoom_music_ctrl_jl_tf_ready_and_play(void);
static void divoom_music_ctrl_request_jl_tf_mode_for_play(uint8 voice_owned);
static void *divoom_music_ctrl_jl_defer_task(void *arg);
static void *divoom_music_ctrl_tf_play_defer_task(void *arg);
static void *divoom_music_ctrl_tf_ready_watchdog_task(void *arg);
static void divoom_music_ctrl_schedule_jl_work(void);
static void divoom_music_ctrl_schedule_tf_play_defer(void);
static void divoom_music_ctrl_clear_tf_ready_watchdog(void);
static void divoom_music_ctrl_schedule_tf_ready_watchdog(void);
static void divoom_music_ctrl_notify_alarm_tf_fallback(void);
static void divoom_music_ctrl_request_mode_ui_refresh(void);
static void divoom_music_ctrl_follow_jl_after_tf_unplug(void);
static uint8 divoom_music_ctrl_local_to_jl_work_mode(uint8 local_mode);
static void divoom_music_ctrl_jl_set_mode_ack(uint8 jl_work_mode, const char *via);
static void divoom_music_ctrl_schedule_jl_set_verify(void);
static void divoom_music_ctrl_jl_send_work_mode_set(uint8 jl_work_mode, uint8 jl_volume);
#define DIVOOM_MUSIC_CTRL_JL_WM_VOL_USE_39  0xFFu
static void *divoom_music_ctrl_jl_set_verify_task(void *arg);

static uint32 divoom_music_ctrl_schedule_voice_release(void)
{
	divoom_app_schedule_item_info info = {0};

	info.model_id = DIVOOM_MODEL_APP_MUSIC_CTRL;
	info.event_id = DIVOOM_MUSIC_SCHEDULE_ACTIVE;
	info.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
	info.resource_type = DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE;
	return divoom_app_schedule_cancel(&info);
}

static uint32 divoom_music_ctrl_schedule_voice_acquire(void)
{
	divoom_app_schedule_item_info info = {0};

	if (pdivoom_music_ctrl_handle == NULL)
		return 0;
	info.model_id = DIVOOM_MODEL_APP_MUSIC_CTRL;
	info.event_id = DIVOOM_MUSIC_SCHEDULE_ACTIVE;
	info.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
	info.resource_type = DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE;
	return divoom_app_schedule_push(&info);
}

static uint32 divoom_music_ctrl_sync_voice_schedule_locked(uint8 playing)
{
	if (playing)
		return divoom_music_ctrl_schedule_voice_acquire();
	return divoom_music_ctrl_schedule_voice_release();
}

static uint32 divoom_music_ctrl_sync_voice_schedule(uint8 play)
{
    uint32 result;
    if (!pdivoom_music_ctrl_handle || !pdivoom_music_ctrl_handle->source_lock)
    {
        return 0;
    }
    divoom_system_get_lock(pdivoom_music_ctrl_handle->source_lock);
    result = divoom_music_ctrl_sync_voice_schedule_locked(play);
    divoom_system_free_lock(pdivoom_music_ctrl_handle->source_lock);
    return result;
}


/* JL work_mode（SPP 枚举）→ 本机 music_ctrl 模式 */
static uint8 divoom_music_ctrl_btcore_work_mode_to_local(uint8 jl_work_mode)
{
	switch (jl_work_mode)
	{
		case 0: return DIVOOM_MUSIC_CTRL_MODE_BT;
		case 1: return DIVOOM_MUSIC_CTRL_MODE_NET_FM;
		case 2: return DIVOOM_MUSIC_CTRL_MODE_LINEIN;
		case 3: return DIVOOM_MUSIC_CTRL_MODE_LOCAL;
#if DIVOOM_SHOW_TF_NAND
		case DIVOOM_BTCORE_WORK_MODE_SHOW:
			return DIVOOM_MUSIC_CTRL_MODE_LOCAL;
#endif
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
		case 7: return DIVOOM_MUSIC_CTRL_MODE_UAC;
#endif
		case DIVOOM_MUSIC_CTRL_MODE_SLEEP:
			return DIVOOM_MUSIC_CTRL_MODE_SLEEP;
		default:
			return DIVOOM_MUSIC_CTRL_MODE_BT;
	}
}

static int divoom_music_ctrl_fetch_btcore_work_mode(uint8 *out_mode)
{
	uint8 cmd_data[2] = {DIVOOM_BTCORE_WORK_MODE_CTRL_GET, 0};
	uint8 reply = 0;
	uint16_t reply_len = 0;
	int ret;

	if (!out_mode || !divoom_btcore_uart_cmd_is_ready())
		return -1;

	ret = divoom_btcore_uart_cmd_send_wait(DM_BTCORE_WORK_MODE, cmd_data, 2,
					       &reply, 1, &reply_len);
	if (ret != 0 || reply_len < 1u)
		return -1;

	*out_mode = divoom_music_ctrl_btcore_work_mode_to_local(reply);
	divoom_print("music ctrl: jl work_mode=%u -> local mode=%u\n",
		     (unsigned)reply, (unsigned)*out_mode);
	return 0;
}

static int divoom_music_ctrl_fetch_btcore_work_mode_raw(uint8 *out_wm)
{
	uint8 cmd_data[2] = {DIVOOM_BTCORE_WORK_MODE_CTRL_GET, 0};
	uint8 reply = 0;
	uint16_t reply_len = 0;
	int ret;

	if (!out_wm || !divoom_btcore_uart_cmd_is_ready())
		return -1;

	ret = divoom_btcore_uart_cmd_send_wait(DM_BTCORE_WORK_MODE, cmd_data, 2,
					       &reply, 1, &reply_len);
	if (ret != 0 || reply_len < 1u)
		return -1;

	*out_wm = reply;
	return 0;
}

static uint8 divoom_music_ctrl_local_to_jl_work_mode(uint8 local_mode)
{
	switch (local_mode)
	{
		case DIVOOM_MUSIC_CTRL_MODE_BT:      return 0;
		case DIVOOM_MUSIC_CTRL_MODE_NET_FM:
#if DIVOOM_MUSIC_RADIO_SOURCE
        {
            /* 本机I2S解码，JL只用静音BT载体，不进入JL的FM模式。 */
            return 0;
        }
#else
        {
            return 1;
        }
#endif
		case DIVOOM_MUSIC_CTRL_MODE_LINEIN:  return 2;
		case DIVOOM_MUSIC_CTRL_MODE_LOCAL:
#if DIVOOM_SHOW_TF_NAND
			return DIVOOM_BTCORE_WORK_MODE_SHOW;
#else
			return 3;
#endif
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
		case DIVOOM_MUSIC_CTRL_MODE_UAC:     return 7;
#endif
		case DIVOOM_MUSIC_CTRL_MODE_SLEEP:
			return (uint8)DIVOOM_MUSIC_CTRL_MODE_SLEEP;
		default:                             return 0;
	}
}

static void divoom_music_ctrl_jl_send_work_mode_set(uint8 jl_work_mode, uint8 jl_volume)
{
	uint8 cmd;

	if (!divoom_btcore_uart_cmd_is_ready())
	{
		divoom_print("music ctrl: jl work_mode SET skip (uart not ready) wm=%u\n",
			     (unsigned)jl_work_mode);
		return;
	}

	s_expect_jl_wm = jl_work_mode;
	s_ignore_jl_apply_until_ms = divoom_system_get_tickms() + 800u;

	if (jl_volume == DIVOOM_MUSIC_CTRL_JL_WM_VOL_USE_39)
	{
		uint8 cmd_data[2];

		cmd = DM_BTCORE_WORK_MODE;
		cmd_data[0] = DIVOOM_BTCORE_WORK_MODE_CTRL_SET;
		cmd_data[1] = jl_work_mode;
		divoom_btcore_uart_cmd_send(cmd, cmd_data, 2);
		divoom_print("music ctrl: jl 0x39 SET work_mode=%u\n",
			     (unsigned)jl_work_mode);
	}
	else
	{
		uint8 cmd_data[2];

		cmd = DM_BTCORE_WORK_MODE_SET_VOL;
		cmd_data[0] = jl_work_mode;
		cmd_data[1] = jl_volume;
		divoom_btcore_uart_cmd_send(cmd, cmd_data, 2);
		divoom_print("music ctrl: jl 0x62 SET work_mode=%u vol=%u\n",
			     (unsigned)jl_work_mode, (unsigned)jl_volume);
	}
	divoom_music_ctrl_schedule_jl_set_verify();
}

void divoom_music_ctrl_enter_jl_sleep_mode(void)
{
	uint8 jl_wm;

	if (s_jl_sleep_mode_active)
		return;
	if (pdivoom_music_ctrl_handle == NULL)
		return;

	jl_wm = divoom_music_ctrl_local_to_jl_work_mode(pdivoom_music_ctrl_handle->mode);
	s_jl_pre_sleep_work_mode = jl_wm;
	s_jl_sleep_mode_active = 1;
	divoom_music_ctrl_jl_send_work_mode_set((uint8)DIVOOM_MUSIC_CTRL_MODE_SLEEP,
					      DIVOOM_MUSIC_CTRL_JL_WM_VOL_USE_39);
	divoom_print("music ctrl: enter jl sleep, saved wm=%u\n", (unsigned)jl_wm);
}

void divoom_music_ctrl_exit_jl_sleep_mode(void)
{
	uint8 restore_wm;
	uint8 restore_local;

	if (!s_jl_sleep_mode_active)
		return;

	s_jl_sleep_mode_active = 0;
	restore_wm = s_jl_pre_sleep_work_mode;
	s_jl_pre_sleep_work_mode = DIVOOM_MUSIC_CTRL_JL_WM_NONE;
	if (restore_wm == DIVOOM_MUSIC_CTRL_JL_WM_NONE)
		restore_wm = divoom_music_ctrl_local_to_jl_work_mode(DIVOOM_MUSIC_CTRL_MODE_BT);

	restore_local = divoom_music_ctrl_btcore_work_mode_to_local(restore_wm);
	if (!divoom_music_ctrl_is_mode_supported(restore_local))
	{
		divoom_print("music ctrl: exit jl sleep, wm=%u unsupported, fallback BT\n",
			     (unsigned)restore_wm);
		restore_wm = divoom_music_ctrl_local_to_jl_work_mode(DIVOOM_MUSIC_CTRL_MODE_BT);
		restore_local = DIVOOM_MUSIC_CTRL_MODE_BT;
	}

	{
		uint8 restore_vol = divoom_music_ctrl_get_saved_volume_for_mode(restore_local);

		if (pdivoom_music_ctrl_handle != NULL)
			pdivoom_music_ctrl_handle->volume = restore_vol;
		divoom_music_ctrl_jl_send_work_mode_set(restore_wm,
			(uint8)divoom_com_vol_ui_to_level(restore_vol));
	}
	divoom_print("music ctrl: exit jl sleep, restore wm=%u local=%u\n",
		     (unsigned)restore_wm, (unsigned)restore_local);

	/* 仅同步本地模式；勿 apply_mode/switch_mode（会 schedule_push，与 cancel 持锁 destroy 死锁） */
	if (pdivoom_music_ctrl_handle != NULL
	    && pdivoom_music_ctrl_handle->mode != restore_local)
	{
		if (pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL
		    && restore_local != DIVOOM_MUSIC_CTRL_MODE_LOCAL)
		{
			divoom_tf_music_stop();
			divoom_music_ctrl_clear_jl_tf_state();
		}
		pdivoom_music_ctrl_handle->last_mode = pdivoom_music_ctrl_handle->mode;
		pdivoom_music_ctrl_handle->mode = restore_local;
		__atomic_add_fetch(&pdivoom_music_ctrl_handle->mode_generation, 1, __ATOMIC_RELEASE);
		pdivoom_music_ctrl_handle->volume =
			divoom_music_ctrl_get_saved_volume_for_mode(restore_local);
	}
}

int divoom_music_ctrl_is_jl_sleep_mode_active(void)
{
	return s_jl_sleep_mode_active ? 1 : 0;
}

static void divoom_music_ctrl_clear_jl_tf_state(void)
{
	s_expect_jl_wm = DIVOOM_MUSIC_CTRL_JL_WM_NONE;
	s_jl_tf_ready = 0;
	s_jl_sd_music_ready_seen = 0;
	s_pending_tf_play = 0;
	divoom_music_ctrl_clear_tf_ready_watchdog();
}

/* JL 已切到 TF(work_mode)；等 0x5e SD_MUSIC_READY 后再发 music_id */
static void divoom_music_ctrl_jl_tf_mode_entered(void)
{
#if DIVOOM_SHOW_TF_NAND
	return;
#endif
	if (pdivoom_music_ctrl_handle == NULL
	    || pdivoom_music_ctrl_handle->mode != DIVOOM_MUSIC_CTRL_MODE_LOCAL)
	{
		return;
	}
	/* 0x5e 可能早于 0x39 到达，勿把已 ready 状态清掉 */
	if (s_jl_tf_ready)
	{
		divoom_print("music ctrl: jl TF mode ok, SD already ready\n");
		return;
	}
	if (s_jl_sd_music_ready_seen)
	{
		divoom_music_ctrl_jl_tf_ready_and_play();
		return;
	}
	s_jl_tf_ready = 0;
	s_pending_tf_play = 1;
	divoom_print("music ctrl: jl TF mode ok, wait SD_MUSIC_READY(0x5e)\n");
	divoom_music_ctrl_schedule_tf_ready_watchdog();
}

/* 收到 0x5e 或兼容路径：置 ready 并发送 set_music_id */
static void divoom_music_ctrl_jl_tf_ready_and_play(void)
{
#if DIVOOM_SHOW_TF_NAND
	return;
#endif
	if (pdivoom_music_ctrl_handle == NULL
	    || pdivoom_music_ctrl_handle->mode != DIVOOM_MUSIC_CTRL_MODE_LOCAL)
	{
		divoom_tf_music_clear_songlist_force_replay();
		return;
	}

	if (s_tf_insert_play_defer_until_ms != 0u
	    && divoom_system_get_tickms() < s_tf_insert_play_defer_until_ms)
	{
		divoom_print("music ctrl: defer TF play after hotplug\n");
		divoom_tf_music_on_jl_play_deferred_for_songlist();
		divoom_music_ctrl_schedule_tf_play_defer();
		return;
	}
	s_tf_insert_play_defer_until_ms = 0u;

	if (divoom_tf_music_defer_jl_play_for_songlist())
	{
		divoom_print("music ctrl: defer TF play, wait JL songlist\n");
		divoom_tf_music_on_jl_play_deferred_for_songlist();
		divoom_music_ctrl_schedule_tf_play_defer();
		return;
	}

	divoom_music_ctrl_jl_tf_mark_ready();
	s_expect_jl_wm = DIVOOM_MUSIC_CTRL_JL_WM_NONE;
	if (s_tf_pending_play_vol_valid)
	{
		divoom_tf_music_set_volume(s_tf_pending_play_vol);
		pdivoom_music_ctrl_handle->volume = s_tf_pending_play_vol;
		s_tf_pending_play_vol_valid = 0;
	}
	if (divoom_sleep_aid_ctrl_is_play_session_active())
	{
		divoom_print("music ctrl: jl TF ready, skip play (sleep aid active)\n");
		return;
	}
	/* 热插拔起播只走这里；吃掉 pending，避免 defer 任务再补一次 */
	(void)divoom_tf_music_take_jl_5e_pending_play();
	divoom_print("music ctrl: jl TF ready, play saved track\n");
	divoom_tf_music_play_saved_force_replay();
}

static void divoom_music_ctrl_request_jl_tf_mode_for_play(uint8 voice_owned)
{
	uint8 jl_wm;
	uint8 vol;

	if (pdivoom_music_ctrl_handle == NULL)
		return;

	divoom_print("music ctrl: local TF play needs JL TF SET (voice_owned=%u)\n",
		     (unsigned)voice_owned);
	/* 本地已是 LOCAL 时 switch_mode 会 early-return，须直接发 SET */
	if (pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
	{
		vol = pdivoom_music_ctrl_handle->volume;
		jl_wm = divoom_music_ctrl_local_to_jl_work_mode(DIVOOM_MUSIC_CTRL_MODE_LOCAL);
		divoom_music_ctrl_jl_send_work_mode_set(jl_wm,
			(uint8)divoom_com_vol_ui_to_level(vol));
		s_pending_tf_play = 1;
		divoom_music_ctrl_schedule_jl_set_verify();
		divoom_music_ctrl_schedule_tf_ready_watchdog();
		return;
	}
	divoom_music_ctrl_switch_mode_ex(DIVOOM_MUSIC_CTRL_MODE_LOCAL, 1, voice_owned, 0);
}

static void divoom_music_ctrl_schedule_jl_work(void)
{
	if (s_jl_defer_task_run)
		return;
	s_jl_defer_task_run = 1;
	divoom_system_create_task(divoom_music_ctrl_jl_defer_task, "music_jl_def", NULL, DIVOOM_TASK_STACK_MUSIC_JL_DEFER);
}

static void divoom_music_ctrl_schedule_tf_play_defer(void)
{
	if (s_tf_play_defer_task_run)
		return;
	s_tf_play_defer_task_run = 1;
	divoom_system_create_task(divoom_music_ctrl_tf_play_defer_task, "music_tf_def", NULL, DIVOOM_TASK_STACK_MUSIC_TF_DEFER);
}

static void divoom_music_ctrl_clear_tf_ready_watchdog(void)
{
	s_tf_ready_wait_deadline_ms = 0;
	s_tf_ready_wd_retry = 0;
}

static void divoom_music_ctrl_notify_alarm_tf_fallback(void)
{
	divoom_sys_msg_info msg = {0};

	msg.msg_type = 1;
	msg.src_id = DIVOOM_MODEL_APP_MUSIC_CTRL;
	msg.dest_id = DIVOOM_MODEL_ALARM;
	msg.msg_id = DIVOOM_ALARM_TF_FALLBACK_TONE;
	divoom_sys_model_crl_msg_push(&msg);
}

static void divoom_music_ctrl_schedule_tf_ready_watchdog(void)
{
	s_tf_ready_wd_retry = 0;
	s_tf_ready_wait_deadline_ms = divoom_system_get_tickms()
				     + DIVOOM_MUSIC_CTRL_TF_READY_WAIT_MS;
	if (s_tf_ready_wd_task_run)
		return;
	s_tf_ready_wd_task_run = 1;
	divoom_system_create_task(divoom_music_ctrl_tf_ready_watchdog_task,
				  "music_tf_wd", NULL, DIVOOM_TASK_STACK_MUSIC_TF_READY_WD);
}

static void *divoom_music_ctrl_tf_ready_watchdog_task(void *arg)
{
	uint32_t until;
	uint32_t now;
	uint8 local;
	uint8 jl_wm;
	uint8 vol;

	(void)arg;

	for (;;)
	{
		until = s_tf_ready_wait_deadline_ms;
		now = divoom_system_get_tickms();
		if (until == 0)
			break;
		if (until > now)
			divoom_system_delay_ms(until - now);

		if (s_tf_ready_wait_deadline_ms == 0 || s_jl_tf_ready || s_jl_sd_music_ready_seen)
			break;
		if (pdivoom_music_ctrl_handle == NULL
		    || pdivoom_music_ctrl_handle->mode != DIVOOM_MUSIC_CTRL_MODE_LOCAL)
			break;

		if (s_tf_ready_wd_retry == 0)
		{
			s_tf_ready_wd_retry = 1;
			vol = pdivoom_music_ctrl_handle->volume;
			jl_wm = divoom_music_ctrl_local_to_jl_work_mode(DIVOOM_MUSIC_CTRL_MODE_LOCAL);
			divoom_print("music ctrl: TF ready watchdog retry SET wm=%u\n",
				     (unsigned)jl_wm);
			divoom_music_ctrl_jl_send_work_mode_set(jl_wm,
				(uint8)divoom_com_vol_ui_to_level(vol));
			divoom_music_ctrl_schedule_jl_set_verify();
			s_tf_ready_wait_deadline_ms = divoom_system_get_tickms()
						     + DIVOOM_MUSIC_CTRL_TF_READY_WAIT_MS;
			continue;
		}

		if (divoom_music_ctrl_fetch_btcore_work_mode(&local) == 0)
		{
			divoom_print("music ctrl: TF ready watchdog GET local=%u no 0x5e\n",
				     (unsigned)local);
		}
		else
		{
			divoom_print("music ctrl: TF ready watchdog GET fail, give up wait\n");
		}
		s_pending_tf_play = 0;
		s_tf_pending_play_vol_valid = 0;
		s_tf_ready_wait_deadline_ms = 0;
		divoom_music_ctrl_notify_alarm_tf_fallback();
		break;
	}

	s_tf_ready_wd_task_run = 0;
	return NULL;
}

static void *divoom_music_ctrl_tf_play_defer_task(void *arg)
{
	uint32_t until;
	uint32_t now;
	int i;

	(void)arg;

	until = s_tf_insert_play_defer_until_ms;
	now = divoom_system_get_tickms();
	if (until > now)
		divoom_system_delay_ms(until - now);

	s_tf_insert_play_defer_until_ms = 0;

	for (i = 0; i < 40; i++)
	{
		if (pdivoom_music_ctrl_handle == NULL
		    || pdivoom_music_ctrl_handle->mode != DIVOOM_MUSIC_CTRL_MODE_LOCAL
		    || !s_jl_sd_music_ready_seen)
			break;
		if (!divoom_tf_music_defer_jl_play_for_songlist())
			break;
		divoom_system_delay_ms(300);
	}

	s_tf_play_defer_task_run = 0;

	if (pdivoom_music_ctrl_handle != NULL
	    && pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL
	    && s_jl_sd_music_ready_seen
	    && divoom_tf_music_take_jl_5e_pending_play())
		divoom_music_ctrl_jl_tf_ready_and_play();
	return NULL;
}

static void divoom_music_ctrl_request_mode_ui_refresh(void)
{
	s_jl_ui_refresh_pending = 1;
	divoom_music_ctrl_schedule_jl_work();
}

/* 0x62/0x39 SET 后收到 JL 0x39 回包或 GET 确认：处理 TF 后续 */
static void divoom_music_ctrl_jl_set_mode_ack(uint8 jl_work_mode, const char *via)
{
	uint8 local;

	if (pdivoom_music_ctrl_handle == NULL)
		return;
	if (s_expect_jl_wm == DIVOOM_MUSIC_CTRL_JL_WM_NONE
	    || jl_work_mode != s_expect_jl_wm)
		return;

	local = divoom_music_ctrl_btcore_work_mode_to_local(jl_work_mode);
	s_expect_jl_wm = DIVOOM_MUSIC_CTRL_JL_WM_NONE;
	s_ignore_jl_apply_until_ms = divoom_system_get_tickms() + 600u;

	if (local == pdivoom_music_ctrl_handle->mode
	    && !s_tf_pending_play_vol_valid)
	{
		divoom_print("music ctrl: jl mode ok (%s), mode %u vol=%u\n",
			     via, (unsigned)local,
			     (unsigned)pdivoom_music_ctrl_handle->volume);
	}
	if (local == DIVOOM_MUSIC_CTRL_MODE_LOCAL
	    && pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
		divoom_music_ctrl_jl_tf_mode_entered();
	divoom_music_ctrl_request_mode_ui_refresh();
}

static void divoom_music_ctrl_schedule_jl_set_verify(void)
{
	s_jl_set_verify_until_ms = divoom_system_get_tickms()
				   + DIVOOM_MUSIC_CTRL_JL_SET_VERIFY_MS;
	if (s_jl_set_verify_task_run)
		return;
	s_jl_set_verify_task_run = 1;
	divoom_system_create_task(divoom_music_ctrl_jl_set_verify_task, "music_jl_ver", NULL, DIVOOM_TASK_STACK_MUSIC_JL_VERIFY);
}

/* JL 已在目标模式时可能不回 0x39 通知；SET 后延时 GET 补确认并设音量 */
static void *divoom_music_ctrl_jl_set_verify_task(void *arg)
{
	uint8 expect_wm;
	uint8 cmd_data[2] = {DIVOOM_BTCORE_WORK_MODE_CTRL_GET, 0};
	uint8 reply = 0;
	uint16_t reply_len = 0;
	uint32_t until;
	uint32_t now;
	int ret;

	(void)arg;

	until = s_jl_set_verify_until_ms;
	now = divoom_system_get_tickms();
	if (until > now)
		divoom_system_delay_ms(until - now);

	s_jl_set_verify_task_run = 0;
	expect_wm = s_expect_jl_wm;
	if (expect_wm == DIVOOM_MUSIC_CTRL_JL_WM_NONE)
		return NULL;
	if (!divoom_music_ctrl_is_ready() || !divoom_btcore_uart_cmd_is_ready())
		return NULL;

	ret = divoom_btcore_uart_cmd_send_wait(DM_BTCORE_WORK_MODE, cmd_data, 2,
					       &reply, 1, &reply_len);
	if (ret != 0 || reply_len < 1u)
	{
		divoom_print("music ctrl: jl SET verify GET fail, expect wm %u\n",
			     (unsigned)expect_wm);
		return NULL;
	}
	if (reply != expect_wm)
	{
		uint8 cur_wm;
		uint8 vol;

		divoom_print("music ctrl: jl SET verify mismatch expect %u got %u\n",
			     (unsigned)expect_wm, (unsigned)reply);
		/* TF 还在等 0x5e 时会丢掉切到 UAC 的 0x62，杰理仍停在 3。补发一次。 */
		if (s_jl_set_verify_retry || pdivoom_music_ctrl_handle == NULL)
			return NULL;
		cur_wm = divoom_music_ctrl_local_to_jl_work_mode(pdivoom_music_ctrl_handle->mode);
		if (cur_wm != expect_wm || s_expect_jl_wm != expect_wm)
			return NULL;
		s_jl_set_verify_retry = 1;
		vol = divoom_music_ctrl_get_saved_volume_for_mode(pdivoom_music_ctrl_handle->mode);
		divoom_print("music ctrl: resend jl work_mode %u after verify mismatch\n",
			     (unsigned)cur_wm);
		divoom_music_ctrl_jl_send_work_mode_set(cur_wm,
			(uint8)divoom_com_vol_ui_to_level(vol));
		return NULL;
	}
	s_jl_set_verify_retry = 0;

	divoom_print("music ctrl: jl SET verify GET ok wm %u\n", (unsigned)reply);
	divoom_music_ctrl_jl_set_mode_ack(reply, "GET");
	return NULL;
}

/* 拆除结束后 follow 杰理当前源。禁止在 UART RX / tf_hotplug 栈上调用 apply。 */
static void divoom_music_ctrl_follow_jl_after_tf_unplug(void)
{
	uint8 wm;
	uint8 local;

	s_tf_unplug_follow_pending = 0;

#if DIVOOM_SHOW_TF_NAND
	s_tf_unplug_hold_local = 0;
	return;
#endif

	if (!s_tf_unplug_hold_local)
		return;
	if (divoom_main_get_audio_type_statu(2)) {
		s_tf_unplug_hold_local = 0;
		return;
	}
	if (pdivoom_music_ctrl_handle == NULL
	    || pdivoom_music_ctrl_handle->mode != DIVOOM_MUSIC_CTRL_MODE_LOCAL) {
		s_tf_unplug_hold_local = 0;
		return;
	}
	if (s_tf_file_update_session
#if DIVOOM_BUILD_RTOS_DEVICE
	    || divoom_ota_jl_is_upgrading()
#endif
	    ) {
		s_tf_unplug_hold_local = 0;
		divoom_print("music ctrl: skip follow after TF unplug (ota)\n");
		return;
	}
	if (s_expect_jl_wm != DIVOOM_MUSIC_CTRL_JL_WM_NONE) {
		s_tf_unplug_hold_local = 0;
		return;
	}
	if (!divoom_music_ctrl_is_ready()) {
		s_tf_unplug_hold_local = 0;
		return;
	}

	if (divoom_music_ctrl_fetch_btcore_work_mode_raw(&wm) != 0) {
		if (!s_jl_passive_work_mode_valid) {
			s_tf_unplug_hold_local = 0;
			divoom_print("music ctrl: TF unplug follow: no jl mode\n");
			return;
		}
		wm = s_jl_passive_work_mode;
	}

	if (wm == DIVOOM_BTCORE_WORK_MODE_FILE_UPDATE
	    || wm == (uint8)DIVOOM_MUSIC_CTRL_MODE_SLEEP) {
		s_tf_unplug_hold_local = 0;
		return;
	}

	local = divoom_music_ctrl_btcore_work_mode_to_local(wm);
	if (local == DIVOOM_MUSIC_CTRL_MODE_LOCAL
	    || !divoom_music_ctrl_is_mode_supported(local)) {
		s_tf_unplug_hold_local = 0;
		divoom_print("music ctrl: TF unplug follow skip wm=%u local=%u\n",
			     (unsigned)wm, (unsigned)local);
		return;
	}

	/* GET 等待期间可能已插回 / 人手切源，apply 前再判一次 */
	if (!s_tf_unplug_hold_local
	    || divoom_main_get_audio_type_statu(2)
	    || pdivoom_music_ctrl_handle == NULL
	    || pdivoom_music_ctrl_handle->mode != DIVOOM_MUSIC_CTRL_MODE_LOCAL) {
		s_tf_unplug_hold_local = 0;
		return;
	}

	divoom_print("music ctrl: TF unplug follow jl wm %u -> local %u\n",
		     (unsigned)wm, (unsigned)local);
	divoom_music_ctrl_apply_mode(local);
	s_tf_unplug_hold_local = 0;
}

static void *divoom_music_ctrl_jl_defer_task(void *arg)
{
	uint8 mode;
	uint8 local;
	uint8 need_ui = 0;

	(void)arg;

	if (divoom_product_test_is_active()) {
		s_jl_sync_pending = 0;
		s_jl_defer_pending = 0;
		s_jl_ui_refresh_pending = 0;
		s_jl_tf_verify_get = 0;
		s_tf_unplug_follow_pending = 0;
		s_jl_defer_task_run = 0;
		return NULL;
	}

	/* 禁止在 UART RX 里 send_wait / 抢 LVGL 锁 */
	if (s_jl_sync_pending)
	{
		s_jl_sync_pending = 0;
		if (divoom_music_ctrl_is_ready()
		    && divoom_music_ctrl_fetch_btcore_work_mode(&local) == 0
		    && pdivoom_music_ctrl_handle != NULL)
		{
			divoom_print("music ctrl sync: jl local mode=%u t113 mode=%u\n",
				     (unsigned)local, (unsigned)pdivoom_music_ctrl_handle->mode);
			if (pdivoom_music_ctrl_handle->mode != local
			    && divoom_music_ctrl_is_mode_supported(local)
			    && !divoom_music_ctrl_ignore_jl_leave_tf(local))
				divoom_music_ctrl_apply_mode(local);
			else if (pdivoom_music_ctrl_handle->mode == local
				 && local == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
				divoom_music_ctrl_jl_tf_mode_entered();
			else
				need_ui = 1;
		}
	}

	if (s_jl_tf_verify_get)
	{
		s_jl_tf_verify_get = 0;
		if (divoom_music_ctrl_is_ready()
		    && divoom_music_ctrl_fetch_btcore_work_mode(&local) == 0
		    && pdivoom_music_ctrl_handle != NULL
		    && local == DIVOOM_MUSIC_CTRL_MODE_LOCAL
		    && pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
			divoom_music_ctrl_jl_tf_mode_entered();
	}

	if (s_jl_defer_pending)
	{
		s_jl_defer_pending = 0;
		mode = s_jl_defer_mode;
		if (divoom_system_get_tickms() >= s_ignore_jl_apply_until_ms
		    && divoom_music_ctrl_is_ready() && pdivoom_music_ctrl_handle != NULL
		    && mode != pdivoom_music_ctrl_handle->mode
		    && divoom_music_ctrl_is_mode_supported(mode)
		    && !divoom_music_ctrl_ignore_jl_leave_tf(mode))
			divoom_music_ctrl_apply_mode(mode);
	}

	if (s_tf_unplug_follow_pending)
		divoom_music_ctrl_follow_jl_after_tf_unplug();

	if (s_jl_ui_refresh_pending || need_ui)
	{
		s_jl_ui_refresh_pending = 0;
		divoom_lvgl_top_audio_menu_refresh_select();
	}

	s_jl_defer_task_run = 0;
	if (s_jl_ui_refresh_pending || s_jl_sync_pending || s_jl_defer_pending
	    || s_jl_tf_verify_get || s_tf_unplug_follow_pending)
		divoom_music_ctrl_schedule_jl_work();
	return NULL;
}

void divoom_music_ctrl_jl_tf_mark_ready(void)
{
	s_jl_tf_ready = 1;
	s_pending_tf_play = 0;
	divoom_music_ctrl_clear_tf_ready_watchdog();
}

uint8 divoom_music_ctrl_tf_jl_ready_for_play(void)
{
	if (pdivoom_music_ctrl_handle == NULL
	    || pdivoom_music_ctrl_handle->mode != DIVOOM_MUSIC_CTRL_MODE_LOCAL)
		return 0;
	/* 等 SET 期间不准发 play。0x5e 可早于 0x39，seen 仍允许起播。 */
	if (s_expect_jl_wm != DIVOOM_MUSIC_CTRL_JL_WM_NONE
	    && !s_jl_sd_music_ready_seen)
		return 0;
	if (s_jl_tf_ready || s_jl_sd_music_ready_seen)
		return 1;
	return 0;
}

int divoom_music_ctrl_play_tf_track_id_ex(int music_id, uint8 volume, uint8 voice_owned)
{
	if (!divoom_music_ctrl_is_ready() || music_id <= 0)
		return -1;

#if !DIVOOM_SHOW_TF_NAND
	if (!divoom_main_get_audio_type_statu(2))
	{
		divoom_print("music ctrl: tf play skipped, no sd card (music_id=%d)\n", music_id);
		return -1;
	}
#endif

	s_tf_pending_play_vol = volume > 100u ? 100u : volume;
	s_tf_pending_play_vol_valid = 1;
	divoom_main_set_tf_music_id(music_id);

	if (pdivoom_music_ctrl_handle->mode != DIVOOM_MUSIC_CTRL_MODE_LOCAL)
	{
		divoom_music_ctrl_switch_mode_ex(DIVOOM_MUSIC_CTRL_MODE_LOCAL, 1, voice_owned, 0);
		return 1;
	}

	if (divoom_music_ctrl_tf_jl_ready_for_play())
	{
		divoom_tf_music_set_volume(s_tf_pending_play_vol);
		pdivoom_music_ctrl_handle->volume = s_tf_pending_play_vol;
		s_tf_pending_play_vol_valid = 0;
		divoom_tf_music_set_music_id(music_id);
		return 0;
	}

	divoom_music_ctrl_request_jl_tf_mode_for_play(voice_owned);
	return 1;
}

int divoom_music_ctrl_play_tf_track_id(int music_id, uint8 volume)
{
	return divoom_music_ctrl_play_tf_track_id_ex(music_id, volume, 0);
}

uint8 divoom_music_ctrl_is_ready(void)
{
	return (s_music_ctrl_init_done && pdivoom_music_ctrl_handle != NULL) ? 1 : 0;
}

/* 开机对齐 JL 模式：仅更新本地并 start_mode，不走 set_mode（避免持锁 UART 与 RX 重入） */
static void divoom_music_ctrl_boot_apply_jl_mode(uint8 jl_mode)
{
	if (pdivoom_music_ctrl_handle == NULL || jl_mode == pdivoom_music_ctrl_handle->mode)
		return;
	if (!divoom_music_ctrl_is_mode_supported(jl_mode))
		return;

	pdivoom_music_ctrl_handle->last_mode = pdivoom_music_ctrl_handle->mode;
	pdivoom_music_ctrl_handle->mode = jl_mode;
	/* 开机 GET 已确认 JL 当前模式，只同步本地，不向 JL 发 SET 0x39 */
	divoom_music_ctrl_start_mode(jl_mode, 1, 1, 0, 0);
	divoom_print("music ctrl boot apply jl mode %u\n", (unsigned)jl_mode);
}

static void divoom_music_ctrl_tf_session_leave(void)
{
	if (!s_tf_file_update_session)
		return;
	s_tf_file_update_session = 0;
	divoom_main_push_model_event_id(DIVOOM_MODEL_OTA,
					DIVOOM_OTA_SERVER_TF_SESSION_LEAVE);
}

void divoom_music_ctrl_on_jl_work_mode_notify(uint8 jl_work_mode)
{
	uint8 local;

	if (divoom_product_test_is_active())
		return;

	s_jl_passive_work_mode = jl_work_mode;
	s_jl_passive_work_mode_valid = 1;

	if (jl_work_mode == DIVOOM_BTCORE_WORK_MODE_FILE_UPDATE) {
		if (divoom_main_get_audio_type_statu(2)
#if DIVOOM_BUILD_RTOS_DEVICE
		    && !divoom_ota_jl_is_upgrading()
#endif
		    ) {
			s_tf_file_update_session = 1;
			divoom_main_push_model_event_id(DIVOOM_MODEL_OTA,
							DIVOOM_OTA_SERVER_TF_SESSION_ENTER);
		}
		return;
	}
	divoom_music_ctrl_tf_session_leave();

	if (!divoom_music_ctrl_is_ready() || pdivoom_music_ctrl_handle == NULL)
		return;

	local = divoom_music_ctrl_btcore_work_mode_to_local(jl_work_mode);

	/* T113 刚 SET 的模式：只认匹配的 0x39，其余忽略，避免旧状态误切 */
	if (s_expect_jl_wm != DIVOOM_MUSIC_CTRL_JL_WM_NONE)
	{
		if (jl_work_mode != s_expect_jl_wm)
			return;
		divoom_music_ctrl_jl_set_mode_ack(jl_work_mode, "RX");
		return;
	}

#if DIVOOM_MUSIC_RADIO_SOURCE
    /* 静音BT载体上报不表示离开电台；显式选源/TF、UAC热插拔照常切换。 */
    if (pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_NET_FM && local == DIVOOM_MUSIC_CTRL_MODE_BT)
    {
        return;
    }
#endif
	if (pdivoom_music_ctrl_handle->mode == local)
	{
		if (local == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
			divoom_music_ctrl_jl_tf_mode_entered();
		else
			divoom_music_ctrl_request_mode_ui_refresh();
		return;
	}
	if (!divoom_music_ctrl_is_mode_supported(local))
		return;
	if (divoom_system_get_tickms() < s_ignore_jl_apply_until_ms)
		return;
	if (divoom_music_ctrl_ignore_jl_leave_tf(local))
	{
		divoom_print("music ctrl: ignore jl leave TF after unplug (wm %u)\n",
			     (unsigned)jl_work_mode);
		return;
	}

	divoom_print("music ctrl: jl work_mode %u -> apply local %u\n",
		     (unsigned)jl_work_mode, (unsigned)local);
	s_jl_defer_mode = local;
	s_jl_defer_pending = 1;
	divoom_music_ctrl_schedule_jl_work();
}

int divoom_music_ctrl_sync_mode_from_jl(void)
{
	if (divoom_product_test_is_active())
		return -1;
	if (!divoom_music_ctrl_is_ready())
		return -1;
#if DIVOOM_MUSIC_RADIO_SOURCE
    /* 电台状态由本机播放器提供，不用JL查询结果覆盖。 */
    if (pdivoom_music_ctrl_handle && pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_NET_FM)
    {
        return 0;
    }
#endif
	s_jl_sync_pending = 1;
	divoom_music_ctrl_schedule_jl_work();
	return 0;
}

void divoom_music_ctrl_on_jl_sd_music_ready(void)
{
#if DIVOOM_SHOW_TF_NAND
	return;
#endif
	if (divoom_product_test_is_active())
		return;

	divoom_print("music ctrl: jl SD_MUSIC_READY(0x5e)\n");
	divoom_music_ctrl_clear_tf_ready_watchdog();
	if (pdivoom_music_ctrl_handle == NULL)
		return;
	if (pdivoom_music_ctrl_handle->mode != DIVOOM_MUSIC_CTRL_MODE_LOCAL)
	{
		uint8 cur_wm;
		uint8 vol;

		cur_wm = divoom_music_ctrl_local_to_jl_work_mode(pdivoom_music_ctrl_handle->mode);
		if (s_expect_jl_wm != DIVOOM_MUSIC_CTRL_JL_WM_NONE && cur_wm == s_expect_jl_wm)
		{
			/* 已经发出切走 TF 的 SET。这次 0x5e 是迟到的，不能留给下次进 TF。 */
			s_jl_sd_music_ready_seen = 0;
			vol = divoom_music_ctrl_get_saved_volume_for_mode(pdivoom_music_ctrl_handle->mode);
			divoom_print("music ctrl: resend jl work_mode %u after late 0x5e\n",
				     (unsigned)cur_wm);
			divoom_music_ctrl_jl_send_work_mode_set(cur_wm,
				(uint8)divoom_com_vol_ui_to_level(vol));
		}
		else
		{
			/* 还没切到 TF，0x5e 先到：留给随后的 start_mode 起播。 */
			s_jl_sd_music_ready_seen = 1;
		}
		return;
	}
	s_jl_sd_music_ready_seen = 1;
	divoom_music_ctrl_jl_tf_ready_and_play();
}

static void divoom_music_ctrl_sleep_clear_saved_restore_if(uint8 local_mode)
{
	if (!s_jl_sleep_mode_active)
		return;
	if (divoom_music_ctrl_btcore_work_mode_to_local(s_jl_pre_sleep_work_mode) != local_mode)
		return;
	s_jl_pre_sleep_work_mode =
		divoom_music_ctrl_local_to_jl_work_mode(DIVOOM_MUSIC_CTRL_MODE_BT);
	divoom_print("music ctrl: mode %u removed during sleep, saved restore -> BT\n",
		     (unsigned)local_mode);
}

void divoom_music_ctrl_on_tf_hotplug(int inserted)
{
	divoom_music_ctrl_clear_jl_tf_state();
	if (inserted)
	{
		s_tf_unplug_hold_local = 0;
		s_tf_unplug_follow_pending = 0;
		s_tf_insert_play_defer_until_ms = divoom_system_get_tickms() + 500u;
	}
	else
	{
		s_tf_unplug_hold_local = 1;
		s_tf_unplug_follow_pending = 0;
		s_tf_insert_play_defer_until_ms = 0u;
		divoom_music_ctrl_sleep_clear_saved_restore_if(DIVOOM_MUSIC_CTRL_MODE_LOCAL);
		memset(&s_tf_play_info, 0, sizeof(s_tf_play_info));
		divoom_music_ctrl_tf_session_leave();
	}
	divoom_print("music ctrl: tf hotplug %s, clear jl TF ready state\n",
		     inserted ? "insert" : "remove");
}

void divoom_music_ctrl_on_tf_unplug_teardown_done(void)
{
#if DIVOOM_SHOW_TF_NAND
	return;
#endif
	/* 连拔连插：插入已清 hold，或卡已在，则不 follow */
	if (!s_tf_unplug_hold_local)
		return;
	if (divoom_main_get_audio_type_statu(2))
		return;
	s_tf_unplug_follow_pending = 1;
	divoom_music_ctrl_schedule_jl_work();
}

void divoom_music_ctrl_on_linein_hotplug(int inserted)
{
	if (!inserted)
		divoom_music_ctrl_sleep_clear_saved_restore_if(DIVOOM_MUSIC_CTRL_MODE_LINEIN);
	divoom_print("music ctrl: linein hotplug %s\n", inserted ? "insert" : "remove");
}

static uint8 divoom_music_ctrl_voice_owner_blocks_track_change(uint8 voice_model)
{
	switch (voice_model)
	{
		case DIVOOM_MODEL_ALARM:
		case DIVOOM_MODEL_APP_WHITE_NOISE:
		case DIVOOM_MODEL_SLEEP_AID:
		case DIVOOM_MODEL_APP_TOMATO_TIME:
		case DIVOOM_MODEL_SLEEP:
			return 1;
		default:
			return 0;
	}
}

/** 闹钟/白噪音/助眠等占用语音轨时不允许切歌 */
static uint8 divoom_music_ctrl_voice_track_allows_track_change(void)
{
	uint8 ev = 0;
	uint8 m;
	uint8 mode;

	if (pdivoom_music_ctrl_handle == NULL)
		return 0;

	mode = pdivoom_music_ctrl_handle->mode;
	if (mode != DIVOOM_MUSIC_CTRL_MODE_BT && mode != DIVOOM_MUSIC_CTRL_MODE_LOCAL)
		return 1;

	m = divoom_app_schedule_get_active_model(&ev, DIVOOM_APP_SCHEDULE_RESOURCE_VOICE);
	if (divoom_music_ctrl_voice_owner_blocks_track_change(m))
		return 0;

	/* BT/TF：无更高优先级占用即可切歌（含暂停态、0x0e 后 JL 自动播但未占语音槽） */
	return 1;
}

/* 拔卡 hold / 歌单拆除中 / 无卡仍 LOCAL：杰理 MUSIC 流正在 stop，0x30 会撞
 * ASSERT stream->state != STREAM_STA_START。 */
static uint8 divoom_music_ctrl_jl_pp_unsafe(void)
{
#if DIVOOM_SHOW_TF_NAND
	return 0;
#else
	if (s_tf_unplug_hold_local || s_tf_unplug_follow_pending)
		return 1;
	if (pdivoom_music_ctrl_handle != NULL
	    && pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL
	    && !divoom_main_get_audio_type_statu(2))
		return 1;
	return 0;
#endif
}

/* 只暂停/恢复出声，不占用或释放语音调度槽位（闹钟等抢占时使用） */
static void divoom_music_ctrl_set_play_internal(uint8 play_status)
{
	if (pdivoom_music_ctrl_handle == NULL)
		return;

#if DIVOOM_MUSIC_RADIO_SOURCE
    if (pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_NET_FM)
    {
        /* music锁可调用电台短锁，电台worker反向调用前必须先释放radio锁。 */
        if (divoom_radio_is_playing() != (play_status != 0))
        {
            divoom_radio_set_voice_output(play_status);
        }
        pdivoom_music_ctrl_handle->playing = play_status ? 1 : 0;
        pdivoom_music_ctrl_handle->transport_state = play_status ?
            DIVOOM_MUSIC_TRANSPORT_PLAYING : DIVOOM_MUSIC_TRANSPORT_PAUSED;
        return;
    }
#endif
#if DIVOOM_SHOW_TF_NAND
	if (pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
	{
		if (pdivoom_music_ctrl_handle->playing == play_status)
			return;
		pdivoom_music_ctrl_handle->playing = play_status;
		pdivoom_music_ctrl_handle->transport_state = play_status ?
			DIVOOM_MUSIC_TRANSPORT_PLAYING : DIVOOM_MUSIC_TRANSPORT_PAUSED;
		divoom_local_music_ctrl_music_ctrl(play_status);
		return;
	}
#endif

	/* 闹钟/tone/调度：0/1 意图。先问杰理，已是目标态则不发，避免误翻转。
	 * 按键 PP 走 divoom_music_ctrl_toggle_play，不走这条。 */
	if (divoom_music_ctrl_sync_play_status_from_jl())
		return;

	if (pdivoom_music_ctrl_handle->playing == play_status)
		return;
	if (divoom_music_ctrl_jl_pp_unsafe())
		return;
	/* 线上只发 PP 翻转；playing/图标等杰理 0x32 */
	play_status = 2;
	divoom_btcore_uart_cmd_send(DM_BTCORE_PAUSE_RESUME_MUSIC, &play_status, 1);
}

/* 按键/音源页 PP：只发 0x30=2，不查 0x32、不写 playing */
static void divoom_music_ctrl_send_pp_toggle(void)
{
	uint8 jl_pp = 2;

	if (divoom_music_ctrl_jl_pp_unsafe())
	{
		divoom_print("music ctrl: drop PP during TF teardown\n");
		return;
	}
	divoom_btcore_uart_cmd_send(DM_BTCORE_PAUSE_RESUME_MUSIC, &jl_pp, 1);
}

/* 从系统配置中读取指定模式保存的音量 0-100 */
static uint8 divoom_music_ctrl_get_saved_volume_for_mode(uint8 mode)
{
	Pdivoom_sys_setting_info psys = divoom_main_get_system_config();
	uint8 vol = 50;
	if (!psys)
		return 50;
	switch (mode) {
		case DIVOOM_MUSIC_CTRL_MODE_BT:      vol = psys->music_volume_bt; break;
		case DIVOOM_MUSIC_CTRL_MODE_LOCAL:   vol = psys->music_volume_local; break;
		case DIVOOM_MUSIC_CTRL_MODE_LINEIN:  vol = psys->music_volume_linein; break;
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
		case DIVOOM_MUSIC_CTRL_MODE_UAC:     vol = psys->music_volume_uac; break;
#endif
		case DIVOOM_MUSIC_CTRL_MODE_NET_FM:  vol = psys->music_volume_netfm; break;
		default: break;
	}
	if (vol > 100)
	{
		vol = 100;
	}
	return divoom_com_vol_snap_ui(vol);
}

/* 将当前模式音量写入系统配置并保存 */
static void divoom_music_ctrl_save_volume_for_mode(uint8 mode, uint8 volume)
{
	Pdivoom_sys_setting_info psys = divoom_main_get_system_config();
	if (!psys)
		return;
	if (volume > 100)
		volume = 100;
	switch (mode) {
		case DIVOOM_MUSIC_CTRL_MODE_BT:      psys->music_volume_bt = volume; break;
		case DIVOOM_MUSIC_CTRL_MODE_LOCAL:   psys->music_volume_local = volume; break;
		case DIVOOM_MUSIC_CTRL_MODE_LINEIN:  psys->music_volume_linein = volume; break;
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
		case DIVOOM_MUSIC_CTRL_MODE_UAC:     psys->music_volume_uac = volume; break;
#endif
		case DIVOOM_MUSIC_CTRL_MODE_NET_FM:  psys->music_volume_netfm = volume; break;
		default: break;
	}
	divoom_main_save_config();
}

/* 低电 UI 封顶：与杰理 3/8 档一致；未低电原样返回 */
static uint8 divoom_music_ctrl_low_power_cap_ui(uint8 vol)
{
	uint8 cap;

	if (!PowerVolIsLowPower())
		return vol;
	cap = divoom_com_convert_vol(divoom_is_charge_sta() ? 8 : 3);
	return (vol > cap) ? cap : vol;
}

void divoom_music_ctrl_save_bt_volume(uint8 volume, uint8 flag)
{
	Pdivoom_sys_setting_info psys = divoom_main_get_system_config();
	if (!psys)
		return;
	if (volume > 100)
		volume = 100;
	volume = divoom_music_ctrl_low_power_cap_ui(volume);

	if (psys->music_volume_bt != volume && flag)
	{
		psys->music_volume_bt = volume;
		divoom_bt_music_set_play_vol_to_device(volume);
		/* 开机动画未结束：仍同步音量，但不弹 OSD（与连接 toast/提示音门禁一致） */
		if (divoom_lvgl_poweron_is_boot_pending())
			divoom_print("BT vol: skip osd during poweron vol=%u\n", (unsigned)volume);
		else
			divoom_lvgl_top_bright_volume_menu_init(DIVOOM_LVGL_TOP_BRIGHT_VOLUME_MENU_FORCE_MUSIC);
		divoom_main_save_config();
	}
}

/* 判断指定音乐模式当前是否支持：蓝牙始终可切换（与 A2DP 是否连上无关）；LOCAL/LINEIN/UAC 依赖设备插入状态 */
int divoom_music_ctrl_is_mode_supported(uint8 mode)
{
	switch (mode) 
	{
		case DIVOOM_MUSIC_CTRL_MODE_BT:
			return 1;
		case DIVOOM_MUSIC_CTRL_MODE_LOCAL:
#if DIVOOM_SHOW_TF_NAND
			return 1;
#else
			return divoom_main_get_audio_type_statu(2) ? 1 : 0;  /* TF 卡插入（TimesVibe 由 JL 上报/开机同步） */
#endif
#if DIVOOM_MUSIC_CTRL_LINEIN_ENABLE
		case DIVOOM_MUSIC_CTRL_MODE_LINEIN:
			return divoom_main_get_audio_type_statu(1) ? 1 : 0;  /* Line-In 插入 */
#endif
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
		case DIVOOM_MUSIC_CTRL_MODE_UAC:
			return divoom_main_get_audio_type_statu(0) ? 1 : 0;  /* UAC/USB 插入 */
#endif
#if DIVOOM_MUSIC_RADIO_SOURCE
        case DIVOOM_MUSIC_CTRL_MODE_NET_FM:
        {
            /* MODE 轮换与音源列表使用同一地区试用策略。 */
            return divoom_main_net_radio_available();
        }
#endif
		default:
			return 0;
	}
}

/*********************************************************************
  Function:        divoom_music_ctrl_set_next_mode
  Description:     切换到下一个音乐模式；若下一模式当前不支持则顺延，
                   直到找到当前支持的模式（蓝牙始终支持）。
  Input:           none
  Return:          NONE
**********************************************************************/
void divoom_music_ctrl_set_next_mode(void)
{
	int i, k, idx = -1;
	uint8 cur_mode, next_mode;
	uint8 audio_table[] =
	{
		DIVOOM_MUSIC_CTRL_MODE_BT,
		DIVOOM_MUSIC_CTRL_MODE_LOCAL,
#if DIVOOM_MUSIC_RADIO_SOURCE
        DIVOOM_MUSIC_CTRL_MODE_NET_FM,
#endif
#if DIVOOM_MUSIC_CTRL_LINEIN_ENABLE
		DIVOOM_MUSIC_CTRL_MODE_LINEIN,
#endif
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
		DIVOOM_MUSIC_CTRL_MODE_UAC,
#endif
	};
	const int audio_table_len = sizeof(audio_table) / sizeof(audio_table[0]);

	if (pdivoom_music_ctrl_handle == NULL) 
	{
		divoom_print("music ctrl not initialized\n");
		return;
	}

	cur_mode = pdivoom_music_ctrl_handle->mode;
	for (i = 0; i < audio_table_len; i++) 
	{
		if (audio_table[i] == cur_mode) 
		{
			idx = i;
			break;
		}
	}
	if (idx < 0)
		idx = -1;

	for (k = 1; k <= audio_table_len; k++) 
	{
		i = (idx + k) % audio_table_len;
		next_mode = audio_table[i];
		if (divoom_music_ctrl_is_mode_supported(next_mode)) 
		{
			divoom_music_ctrl_set_mode(next_mode);
			return;
		}
	}
	divoom_music_ctrl_set_mode(DIVOOM_MUSIC_CTRL_MODE_BT);
}

void divoom_music_ctrl_advance_mode_local(void)
{
	int i, k, idx = -1;
	uint8 cur_mode, next_mode;
	uint8 audio_table[] =
	{
		DIVOOM_MUSIC_CTRL_MODE_BT,
		DIVOOM_MUSIC_CTRL_MODE_LOCAL,
#if DIVOOM_MUSIC_RADIO_SOURCE
        DIVOOM_MUSIC_CTRL_MODE_NET_FM,
#endif
#if DIVOOM_MUSIC_CTRL_LINEIN_ENABLE
		DIVOOM_MUSIC_CTRL_MODE_LINEIN,
#endif
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
		DIVOOM_MUSIC_CTRL_MODE_UAC,
#endif
	};
	const int audio_table_len = sizeof(audio_table) / sizeof(audio_table[0]);

	if (pdivoom_music_ctrl_handle == NULL)
		return;

	cur_mode = pdivoom_music_ctrl_handle->mode;
	for (i = 0; i < audio_table_len; i++)
	{
		if (audio_table[i] == cur_mode)
		{
			idx = i;
			break;
		}
	}
	if (idx < 0)
		idx = -1;

	for (k = 1; k <= audio_table_len; k++)
	{
		i = (idx + k) % audio_table_len;
		next_mode = audio_table[i];
		if (divoom_music_ctrl_is_mode_supported(next_mode))
		{
			divoom_music_ctrl_apply_mode(next_mode);
			return;
		}
	}
	divoom_music_ctrl_apply_mode(DIVOOM_MUSIC_CTRL_MODE_BT);
}

/*********************************************************************
  Function:        divoom_music_ctrl_bt_play_request_handler
  Description:     蓝牙播放请求处理
                   当手机端在非蓝牙模式下开始播放时调用
                   自动切换到蓝牙模式
  Input:           none
  Return:          NONE 
**********************************************************************/
static void divoom_music_ctrl_bt_play_request_handler(void)
{
    divoom_print("BT play request received, auto switching to BT mode\n");
    /* 经典蓝牙常驻不退出，切到其他模式仅暂停；手机端点击播放时自动切回蓝牙模式并出声 */
    divoom_music_ctrl_apply_mode(DIVOOM_MUSIC_CTRL_MODE_BT);
}

//当模块在执行，被切换到后面时，将会调用这个函数，如果返回非0,将直接从队列中删除
uint8 divoom_music_ctrl_schedule_pause_callback(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type)
{
    if ((resouce_type & DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE) && pdivoom_music_ctrl_handle)
    {
        divoom_system_get_lock(pdivoom_music_ctrl_handle->lock_handle);
        if (pdivoom_music_ctrl_handle->source_transition)
        {
            pdivoom_music_ctrl_handle->source_voice_changed = 1;
            pdivoom_music_ctrl_handle->source_voice_allowed = 0;
            divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
            return 1;
        }
        divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
    }
#ifdef CONFIG_DIVOOM_MPP_NET_AUDIO
    /* 直播与UAC一样持续接收；提示音/闹钟的静音由NETWORK duck负责，
     * 不能为每个短提示反复断网。显式暂停和切换音源仍走set_play/stop。 */
    if ((resouce_type & DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE) && pdivoom_music_ctrl_handle &&
        pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_NET_FM)
    {
        return 1;
    }
#endif

	if ((resouce_type & DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE) && pdivoom_music_ctrl_handle)
	{
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
		/* UAC：PC 持续推流，静音靠 MEDIA_VOLUME duck；schedule 抢占勿发 PAUSE_RESUME */
		if (pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_UAC)
			return 1;
#endif
		/* APP 闹钟试听：与系统 tip 一样只 duck，不 0x30 暂停；槽仍占着以便 ESC 停试听 */
		if (active_model == DIVOOM_MODEL_ALARM
		    && active_event == DIVOOM_ALARM_APP_EVENT_LISTEN)
			return 1;
		/* 蓝牙铃声闹钟期间不发暂停。不记恢复，结束时由 stop_voice 停下，和手机直接播放一致。 */
		if (active_model == DIVOOM_MODEL_ALARM
		    && active_event == DIVOOM_ALARM_APP_EVENT_ALARM
		    && pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_BT
		    && divoom_alarm_get_current_alarm_mode() == 3)
		{
			s_music_voice_saved_playing = 0;
			return 1;
		}
		divoom_system_get_lock(pdivoom_music_ctrl_handle->lock_handle);
		s_music_voice_saved_playing = pdivoom_music_ctrl_handle->playing;
		/* 助眠等抢占语音轨：JL 可能在播但 playing 未同步，仍发暂停 */
		divoom_music_ctrl_set_play_internal(0);
		divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
	}
    return 1;
}
//当模块从后台被激活，将会调用这个函数
uint8 divoom_music_ctrl_schedule_play_callback(uint8 event_id, void *user_data, uint32 resouce_type)
{
    if ((resouce_type & DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE) && pdivoom_music_ctrl_handle)
    {
        divoom_system_get_lock(pdivoom_music_ctrl_handle->lock_handle);
        if (pdivoom_music_ctrl_handle->source_transition)
        {
            pdivoom_music_ctrl_handle->source_voice_changed = 1;
            pdivoom_music_ctrl_handle->source_voice_allowed = 1;
            divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
            return 1;
        }
        divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
    }
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
	if (pdivoom_music_ctrl_handle
	    && pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_UAC)
		return 1;
#endif
	if ((resouce_type & DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE) && pdivoom_music_ctrl_handle)
	{
		if (s_music_voice_saved_playing)
		{
			divoom_system_get_lock(pdivoom_music_ctrl_handle->lock_handle);
			s_music_voice_saved_playing = 0;
			divoom_music_ctrl_set_play_internal(1);
			divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
		}
	}
    return 1;
}
//当模块达到条件，将要卸载时，将会调用这个函数
uint8 divoom_music_ctrl_schedule_destory_callback(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type)
{
	s_music_voice_saved_playing = 0;
    return 1;
}

static divoom_music_ctrl_play_status_notify_fn s_play_status_notify;

void divoom_music_ctrl_set_play_status_notify(divoom_music_ctrl_play_status_notify_fn fn)
{
	s_play_status_notify = fn;
}

static void divoom_music_ctrl_notify_play_status_changed(void)
{
	if (s_play_status_notify)
		s_play_status_notify();
}

void divoom_music_ctrl_toggle_play_uac(void)
{
	uint8 toggle = 2;

	if (pdivoom_music_ctrl_handle == NULL || !divoom_btcore_uart_cmd_is_ready())
		return;

	divoom_btcore_uart_cmd_send(DM_BTCORE_PAUSE_RESUME_MUSIC, &toggle, 1);
	divoom_print("music ctrl: uac jl toggle pp (0x30)\n");
}

/** UAC：通知 JL 向 PC 发送 HID 上一曲/下一曲；payload 0=上一曲 1=下一曲 */
static void divoom_music_ctrl_uac_last_next(uint8 next)
{
	uint8 dir = next;

	if (pdivoom_music_ctrl_handle == NULL || !divoom_btcore_uart_cmd_is_ready())
		return;

	divoom_btcore_uart_cmd_send(DM_BTCORE_UAC_MUSIC_LAST_NEXT, &dir, 1);
	divoom_print("music ctrl: uac jl last/next (0x48) %u\n", (unsigned)dir);
}

//不模块处理激活时，系统有按键过来，将调用这个函数，如果返回０，这个按键信息将传递到系统，否则将不需要
// 处理音乐专用键：MUSIC_MODE(PD12)、MUSIC_PREV(PD13)、MUSIC_NEXT(PD14)、MUSIC_PLAY(PD10)、MUSIC_FAVORITE(PD11)
uint8 divoom_music_ctrl_schedule_key_callback(uint8 key_value, uint8 key_status, uint32 resouce_type)
{
	static uint8 s_fav_key_pressing_handled;

	switch (key_value)
	{
		case DIVOOM_KEY_MUSIC_MODE:    // KEY_F17, PD12 - 切换音乐模式
			divoom_print("wft test DIVOOM_KEY_MUSIC_MODE\n");
#ifdef DIVOOM_PRODUCT_ASTRO_TOO
			if (divoom_astrootoo_network_reset_confirm_is_open())
			{
				return 1;
			}
			if (key_status == DIVOOM_PRESS_LONG)
			{
				divoom_astrootoo_network_reset_confirm_arm_hold();
				return 1;
			}
			if (key_status == DIVOOM_PRESS_UP)
			{
				divoom_astrootoo_network_reset_confirm_disarm_hold();
				return 1;
			}
			if (key_status == DIVOOM_PRESSING)
			{
				divoom_astrootoo_network_reset_confirm_on_pressing();
				return 1;
			}
#endif
			if (key_status == DIVOOM_PRESS_LONG)
			{
#if 0
				if (!divoom_sleep_aid_ctrl_is_play_session_active())
				{
					if (divoom_bt_music_is_connected())
						divoom_bt_music_disconnect_bt();
					else
						divoom_bt_music_connect_bt();
				}
#endif
			}
			else if (key_status == DIVOOM_PRESS_SHORT /*|| key_status == DIVOOM_PRESSING*/)
			{
				uint8 prev_mode;
				uint8 notice_type;

				if (divoom_sleep_aid_ctrl_exit_play_on_music_mode_key(key_status))
					return 1;
#if 0
				if (divoom_lvgl_top_audio_menu_is_open()) {
					divoom_music_ctrl_set_next_mode();
					divoom_lvgl_top_audio_menu_refresh_select();
				} else {
					divoom_lvgl_top_audio_menu_init();
				}
#else
				prev_mode = divoom_music_ctrl_get_mode();
				divoom_music_ctrl_set_next_mode();
				/* 无其它可用音源时仍弹窗，文案只显示源名（蓝牙），不是「切换为」 */
				notice_type = (divoom_music_ctrl_get_mode() != prev_mode)
					? DIVOOM_SOURCE_SWITCH_OSD_SWITCH
					: DIVOOM_SOURCE_SWITCH_OSD_CURRENT;
				divoom_lvgl_top_source_switch_menu_init(divoom_music_ctrl_get_mode(),
					notice_type);
				divoom_astrootoo_source_goto_music_mode(divoom_music_ctrl_get_mode());
#endif
			}
			return 1;

		case DIVOOM_KEY_MUSIC_PREV:   // KEY_F18, PD13 - 上一首
			divoom_print("wft test DIVOOM_KEY_MUSIC_PREV\n");
// #ifdef DIVOOM_TIME_FRAME_SPEAKER
// 			divoom_lvgl_top_music_ctrl_hint_show(DIVOOM_KEY_MUSIC_PREV);
// #endif
			divoom_music_ctrl_play_pre();
			return 1;

		case DIVOOM_KEY_MUSIC_NEXT:   // KEY_F19, PD14 - 下一首
			divoom_print("wft test DIVOOM_KEY_MUSIC_NEXT\n");
// #ifdef DIVOOM_TIME_FRAME_SPEAKER
// 			divoom_lvgl_top_music_ctrl_hint_show(DIVOOM_KEY_MUSIC_NEXT);
// #endif
			divoom_music_ctrl_play_next();
			return 1;

		case DIVOOM_KEY_MUSIC_PLAY:   // KEY_F20, PD10 - 短按播放/暂停，长按切换按键灯效
#if !defined(DIVOOM_OPENSOURCE_SWITCH) && defined(CONFIG_DIVOOM_AI_VOICE) && DIVOOM_AI_VOICE_DIAL_TEST_ENABLE
			if (key_status == DIVOOM_PRESS_SHORT && divoom_ai_voice_dial_test_play_key())
				return 1;
#endif
			divoom_print("wft test DIVOOM_KEY_MUSIC_PLAY\n");
// #ifdef DIVOOM_TIME_FRAME_SPEAKER
//          divoom_lvgl_top_music_ctrl_hint_show(DIVOOM_KEY_MUSIC_PLAY);
// #endif
			if (key_status == DIVOOM_PRESS_LONG)
			{
				divoom_main_key_light_cycle_next();
				return 1;
			}
			if (key_status != DIVOOM_PRESS_SHORT)
				return 1;
			divoom_music_ctrl_toggle_play();
			return 1;

		case DIVOOM_KEY_MUSIC_FAVORITE:   // KEY_F21, PD11 - 助眠：短按播放 / 长按收藏列表
			if (key_status == DIVOOM_PRESS_LONG)
			{
				if (!divoom_sleep_aid_ctrl_is_play_session_active())
					divoom_sleep_aid_ctrl_open_fav_list_menu();
			}
			else if (key_status == DIVOOM_PRESS_SHORT || key_status == DIVOOM_PRESSING)
			{
				if (key_status == DIVOOM_PRESSING)
				{
					if (s_fav_key_pressing_handled)
						return 1;
					s_fav_key_pressing_handled = 1;
				}
				else if (s_fav_key_pressing_handled)
				{
					s_fav_key_pressing_handled = 0;
					return 1;
				}
				divoom_sleep_aid_ctrl_on_music_favorite_key();
			}
			else if (key_status == DIVOOM_PRESS_UP)
			{
				s_fav_key_pressing_handled = 0;
			}
			return 1;

		default:
			return 0;
	}
}

uint8 divoom_music_ctrl_event(Pdivoom_sys_msg_info msg)
{
    (void)msg;
    return 0;
}


/*********************************************************************
  Function:        divoom_music_ctrl_init
  Description:     初始化音乐控制模块
  Input:           none
  Return:          NONE 
**********************************************************************/

void divoom_music_ctrl_init(void)
{
    if(pdivoom_music_ctrl_handle != NULL)
    {
        divoom_print("music ctrl already initialized\n");
        return;
    }
    
    pdivoom_music_ctrl_handle = divoom_system_malloc(sizeof(divoom_music_ctrl_ctrl));
    if(pdivoom_music_ctrl_handle == NULL)
    {
        divoom_print("music ctrl malloc failed\n");
        return;
    }
    
    memset(pdivoom_music_ctrl_handle, 0, sizeof(divoom_music_ctrl_ctrl));
    pdivoom_music_ctrl_handle->mode = DIVOOM_MUSIC_CTRL_MODE_BT;  // 默认蓝牙模式
    pdivoom_music_ctrl_handle->last_mode = DIVOOM_MUSIC_CTRL_MODE_BT;
    pdivoom_music_ctrl_handle->playing = 0;
    pdivoom_music_ctrl_handle->bt_music_initialized = 0;
    pdivoom_music_ctrl_handle->line_in_initialized = 0;
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
    pdivoom_music_ctrl_handle->uac_initialized = 0;
#endif
    pdivoom_music_ctrl_handle->lock_handle = divoom_system_create_lock();
    pdivoom_music_ctrl_handle->source_lock = divoom_system_create_lock();
    if (!pdivoom_music_ctrl_handle->lock_handle || !pdivoom_music_ctrl_handle->source_lock)
    {
        if (pdivoom_music_ctrl_handle->lock_handle)
        {
            divoom_system_destroy_lock(pdivoom_music_ctrl_handle->lock_handle);
        }
        if (pdivoom_music_ctrl_handle->source_lock)
        {
            divoom_system_destroy_lock(pdivoom_music_ctrl_handle->source_lock);
        }
        divoom_system_free(pdivoom_music_ctrl_handle);
        pdivoom_music_ctrl_handle = NULL;
        return;
    }
    
    // 初始化蓝牙音乐模块（蓝牙模块需要常驻运行以接收A2DP连接）
    divoom_bt_music_init();
    pdivoom_music_ctrl_handle->bt_music_initialized = 1;

    if (divoom_bt_music_fetch_connected_phone_info_from_jl() != 0)
        divoom_print("music ctrl init: get connected phone info failed\n");
    
	divoom_sys_model_crl_register(DIVOOM_MODEL_APP_MUSIC_CTRL, divoom_music_ctrl_event);

    divoom_music_ctrl_clear_jl_tf_state();
    {
        uint8 jl_boot_mode = DIVOOM_MUSIC_CTRL_MODE_BT;
        int jl_boot_mode_ok = divoom_music_ctrl_fetch_btcore_work_mode(&jl_boot_mode);

        if (jl_boot_mode_ok != 0)
        {
            divoom_system_delay_ms(200);
            jl_boot_mode_ok = divoom_music_ctrl_fetch_btcore_work_mode(&jl_boot_mode);
        }
        if (jl_boot_mode_ok != 0 && s_jl_passive_work_mode_valid)
        {
            jl_boot_mode = divoom_music_ctrl_btcore_work_mode_to_local(
                s_jl_passive_work_mode);
            jl_boot_mode_ok = 0;
            divoom_print("music ctrl init: use passive jl work_mode %u -> local %u\n",
                         (unsigned)s_jl_passive_work_mode, (unsigned)jl_boot_mode);
        }

        if (jl_boot_mode_ok != 0)
        {
            divoom_print("music ctrl init: get jl work mode failed, use BT\n");
            divoom_music_ctrl_set_volume(
                divoom_music_ctrl_get_saved_volume_for_mode(pdivoom_music_ctrl_handle->mode));
        }
        else if (jl_boot_mode != pdivoom_music_ctrl_handle->mode)
        {
            divoom_music_ctrl_boot_apply_jl_mode(jl_boot_mode);
        }
        else
        {
            divoom_music_ctrl_set_volume(
                divoom_music_ctrl_get_saved_volume_for_mode(pdivoom_music_ctrl_handle->mode));
        }
    }
    s_music_ctrl_init_done = 1;

    divoom_music_ctrl_sync_play_status_from_jl();

    divoom_bt_music_sync_poweron_connect_tone_defer();

    divoom_print("music ctrl init done, mode: %u\n", (unsigned)pdivoom_music_ctrl_handle->mode);
}

/*********************************************************************
  Function:        divoom_music_ctrl_deinit
  Description:     反初始化音乐控制模块
  Input:           none
  Return:          NONE 
**********************************************************************/

void divoom_music_ctrl_deinit(void)
{
    if(pdivoom_music_ctrl_handle == NULL)
    {
        return;
    }

    s_music_ctrl_init_done = 0;
    
    // 停止当前播放
    divoom_music_ctrl_stop_play();
    
    // 反初始化各个模块
    if(pdivoom_music_ctrl_handle->bt_music_initialized)
    {
        divoom_bt_music_deinit();
        pdivoom_music_ctrl_handle->bt_music_initialized = 0;
    }
    
    if(pdivoom_music_ctrl_handle->line_in_initialized)
    {
        divoom_line_in_deinit();
        pdivoom_music_ctrl_handle->line_in_initialized = 0;
    }
    
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
    if(pdivoom_music_ctrl_handle->uac_initialized)
    {
        divoom_uac_deinit();
        pdivoom_music_ctrl_handle->uac_initialized = 0;
    }
#endif
    
    if(pdivoom_music_ctrl_handle->lock_handle)
    {
        divoom_system_destroy_lock(pdivoom_music_ctrl_handle->lock_handle);
    }
    
    if (pdivoom_music_ctrl_handle->source_lock)
    {
        divoom_system_destroy_lock(pdivoom_music_ctrl_handle->source_lock);
    }
    divoom_system_free(pdivoom_music_ctrl_handle);
    pdivoom_music_ctrl_handle = NULL;
    
    divoom_print("music ctrl deinit done\n");
}

/*********************************************************************
  Function:        divoom_music_ctrl_stop_current_mode
  Description:     停止当前模式的播放（内部函数）
  Input:           none
  Return:          NONE 
**********************************************************************/

static void divoom_music_ctrl_stop_current_mode(void)
{
    if(pdivoom_music_ctrl_handle == NULL)
    {
        return;
    }
    
	switch(pdivoom_music_ctrl_handle->mode)
    {
        case DIVOOM_MUSIC_CTRL_MODE_BT:      // 蓝牙模式
            divoom_print("BT music stopped\n");
            break;
            
        case DIVOOM_MUSIC_CTRL_MODE_LOCAL:   // 本地TF卡模式
            divoom_print("TF music stopped\n");
            divoom_tf_music_stop();
#if DIVOOM_SHOW_TF_NAND
            divoom_local_music_ctrl_stop_music();
#endif
            break;
            
        case DIVOOM_MUSIC_CTRL_MODE_NET_FM:  // 网络电台
            #if DIVOOM_MUSIC_RADIO_SOURCE
            divoom_radio_set_play(0);
#endif
            divoom_print("NET FM stopped\n");
            break;

        case DIVOOM_MUSIC_CTRL_MODE_LINEIN:  // Line-In模式
            divoom_print("Line-In stopped\n");
            break;

#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
        case DIVOOM_MUSIC_CTRL_MODE_UAC:    // UAC模式
            divoom_print("UAC stopped\n");
            break;
#endif
    }
    pdivoom_music_ctrl_handle->playing = 0;
    pdivoom_music_ctrl_handle->transport_state = DIVOOM_MUSIC_TRANSPORT_STOPPED;
}

/*********************************************************************
  Function:        divoom_music_ctrl_start_mode
  Description:     启动指定模式（内部函数）
  Input:           mode - 要启动的模式
                   allow_voice_output - 1 正常起播；0 仅拉起通路/解码但保持暂停（语音轨未占队首时用）
  Return:          NONE 
**********************************************************************/

static void divoom_music_ctrl_start_mode(uint8 mode, uint8 allow_voice_output,
					 uint8 jl_already_in_mode, uint8 send_jl_set, int32 radio_generation)
{
    uint8 saved_vol;
    if (pdivoom_music_ctrl_handle == NULL)
    {
        return;
    }
	uint8 jl_wm;

	if (mode != DIVOOM_MUSIC_CTRL_MODE_LOCAL)
		divoom_music_ctrl_clear_jl_tf_state();

	saved_vol = divoom_music_ctrl_get_saved_volume_for_mode(mode);
#if DIVOOM_MUSIC_RADIO_SOURCE
    if (mode == DIVOOM_MUSIC_CTRL_MODE_NET_FM)
    {
        /* 页面不拥有音源；JL退出TF/UAC并静音媒体，本机NETWORK独立出声。 */
        divoom_radio_start();
        if (!divoom_radio_prepare_resume(allow_voice_output, radio_generation))
        {
            return;
        }
        pdivoom_music_ctrl_handle->volume = saved_vol;
        /* NET_FM是本机虚拟音源，主动/被动切入都必须退出JL旧通路并静音载体。 */
        divoom_music_ctrl_jl_send_work_mode_set(0, 0);
#ifdef CONFIG_DIVOOM_MPP_NET_AUDIO
        divoom_net_audio_set_volume(saved_vol);
#endif
        /* prepare已原子提交出声许可；这里不能再次无条件set_play，避免复活后到的停止。 */
        pdivoom_music_ctrl_handle->playing = allow_voice_output ? 1 : 0;
        pdivoom_music_ctrl_handle->transport_state = allow_voice_output ?
            DIVOOM_MUSIC_TRANSPORT_PLAYING : DIVOOM_MUSIC_TRANSPORT_PAUSED;
        return;
    }
#endif
	if (send_jl_set)
		pdivoom_music_ctrl_handle->volume = saved_vol;
	else
		divoom_music_ctrl_set_volume(saved_vol);

	if (send_jl_set)
	{
		jl_wm = divoom_music_ctrl_local_to_jl_work_mode(mode);
		divoom_music_ctrl_jl_send_work_mode_set(jl_wm,
			(uint8)divoom_com_vol_ui_to_level(saved_vol));
		divoom_print("music ctrl: set jl work_mode %u (local %u)\n",
			     (unsigned)jl_wm, (unsigned)mode);
	}
	else
	{
		divoom_print("music ctrl: follow jl/local mode %u (no 0x39 SET)\n",
			     (unsigned)mode);
	}

	if (mode == DIVOOM_MUSIC_CTRL_MODE_BT)
	{
		/* 切到蓝牙不会让手机自动起播。M 键后 JL 会补 0x32=暂停；
		 * 拔 TF/USB 常只有 apply_mode，本地若沿用 TF/UAC 的 playing=1
		 * 就会把暂停条画成正在播。这里跟 AVRCP。 */
		pdivoom_music_ctrl_handle->playing =
			(allow_voice_output && divoom_bt_music_is_a2dp_playing()) ? 1 : 0;
	}
	else if (allow_voice_output)
	{
		pdivoom_music_ctrl_handle->playing = 1;
	}
	else
	{
		pdivoom_music_ctrl_handle->playing = 0;
	}
	
	if (mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
	{
#if DIVOOM_SHOW_TF_NAND
		divoom_tf_music_prepare();
		divoom_tf_music_set_volume(saved_vol);
		divoom_audio_sink_ch_set_volume(DIVOOM_AUDIO_SINK_CH_MUSIC, saved_vol);
		s_jl_tf_ready = 1;
		s_pending_tf_play = 0;
		if (allow_voice_output)
			divoom_tf_music_play_saved_force_replay();
		return;
#else
		divoom_tf_music_prepare();
		s_jl_tf_ready = 0;
		s_pending_tf_play = 1;
		if (!send_jl_set)
			s_expect_jl_wm = DIVOOM_MUSIC_CTRL_JL_WM_NONE;
		/* 插卡时 JL 可能先于 T113 切模式上报 0x5e，勿清零 seen */
		if (s_jl_sd_music_ready_seen)
		{
			divoom_print("music ctrl: TF mode start, SD already reported 0x5e\n");
			divoom_music_ctrl_jl_tf_ready_and_play();
		}
		else if (jl_already_in_mode)
		{
			divoom_print("music ctrl: jl already in TF, wait SD_MUSIC_READY(0x5e)\n");
			divoom_music_ctrl_schedule_tf_ready_watchdog();
		}
		else
		{
			/* JL 切模式后应上报 0x5e；不再用 GET 0x39 代替起播 */
			divoom_music_ctrl_schedule_jl_work();
			divoom_music_ctrl_schedule_tf_ready_watchdog();
		}
#endif
	}
}

/*********************************************************************
  Function:        divoom_music_ctrl_set_mode
  Description:     设置音乐模式（核心切换函数）
                   切换时会自动停止当前模式并启动新模式
  Input:           mode - 目标模式
  Return:          NONE 
**********************************************************************/

void divoom_music_ctrl_set_mode(uint8 mode)
{
	divoom_music_ctrl_switch_mode_ex(mode, 1, 0, 0);
}

void divoom_music_ctrl_apply_mode(uint8 mode)
{
	divoom_music_ctrl_switch_mode_ex(mode, 0, 0, 0);
}

static uint8 divoom_music_ctrl_switch_mode_locked(uint8 mode, uint8 send_jl_set, uint8 voice_owned, int32 radio_generation)
{
	uint32 schedule_ret = 0;
	uint32 generation;
    uint8 radio_request = radio_generation != 0;
	uint8 allow_voice_output = 1;

	if (divoom_product_test_is_active())
		return 0;

	if(pdivoom_music_ctrl_handle == NULL)
    {
        divoom_print("music ctrl not initialized\n");
        return 0;
    }

#if !DIVOOM_MUSIC_CTRL_UAC_ENABLE
    if (mode == DIVOOM_MUSIC_CTRL_MODE_UAC)
    {
        divoom_print("UAC mode disabled\n");
        return 0;
    }
#endif
    
    divoom_system_get_lock(pdivoom_music_ctrl_handle->lock_handle);
#if DIVOOM_MUSIC_RADIO_SOURCE
    if (mode == DIVOOM_MUSIC_CTRL_MODE_NET_FM && !radio_request)
    {
        if (!divoom_radio_start())
        {
            divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
            return 0;
        }
        radio_generation = divoom_radio_get_request_generation();
    }
    if (mode == DIVOOM_MUSIC_CTRL_MODE_NET_FM && !divoom_radio_request_current(radio_generation))
    {
        divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
        return 0;
    }
    if (mode != DIVOOM_MUSIC_CTRL_MODE_NET_FM)
    {
        /* 即使当前已是BT，显式选择BT也应撤销尚未消费的旧电台选源意图。 */
        divoom_radio_set_play(0);
    }
#endif
    if(pdivoom_music_ctrl_handle->mode == mode && !radio_request)
    {
        divoom_print("already in mode %d\n", mode);
        divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
        return 0;
    }
    
    divoom_print("switching music mode: %d -> %d (jl_set=%u voice_owned=%u)\n",
		pdivoom_music_ctrl_handle->mode, mode, (unsigned)send_jl_set, (unsigned)voice_owned);
    s_jl_set_verify_retry = 0;
    
    if (pdivoom_music_ctrl_handle->mode != mode)
    {
        divoom_music_ctrl_stop_current_mode();
        pdivoom_music_ctrl_handle->last_mode = pdivoom_music_ctrl_handle->mode;
        pdivoom_music_ctrl_handle->mode = mode;
    }
    generation = __atomic_add_fetch(&pdivoom_music_ctrl_handle->mode_generation, 1, __ATOMIC_RELEASE);
    /* push可能同步回调play；旧音源的待恢复标志不得提前启动尚未提交的新音源。 */
    s_music_voice_saved_playing = 0;
    s_tone_overlay_saved_playing = 0;
    pdivoom_music_ctrl_handle->source_transition = 1;
    pdivoom_music_ctrl_handle->source_voice_changed = 0;

	divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);

	if (voice_owned)
		schedule_ret = 1;
	else if (divoom_sleep_aid_ctrl_is_play_session_active())
		schedule_ret = 0;
	else
		schedule_ret = divoom_music_ctrl_sync_voice_schedule_locked(1);

	divoom_system_get_lock(pdivoom_music_ctrl_handle->lock_handle);
    pdivoom_music_ctrl_handle->source_transition = 0;

	/* 调度可能重入切源；只能由最后一次有效事务启动硬件/网络，旧事务不能复活电台。 */
	if (generation != __atomic_load_n(&pdivoom_music_ctrl_handle->mode_generation, __ATOMIC_ACQUIRE) ||
		pdivoom_music_ctrl_handle->mode != mode)
	{
		divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
		return 0;
	}
#if DIVOOM_MUSIC_RADIO_SOURCE
    if (mode == DIVOOM_MUSIC_CTRL_MODE_NET_FM && !divoom_radio_request_current(radio_generation))
    {
        divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
        return 0;
    }
#endif
	allow_voice_output = pdivoom_music_ctrl_handle->source_voice_changed ?
        pdivoom_music_ctrl_handle->source_voice_allowed : schedule_ret != 0;
	if (allow_voice_output)
		s_music_voice_saved_playing = 0;
	else
		s_music_voice_saved_playing = 1;
	divoom_music_ctrl_start_mode(mode, allow_voice_output, !send_jl_set, send_jl_set, radio_generation);
	divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);

    return 1;
}

/* 按键可能已持 LVGL 锁。必须先释放切源锁，再投递页面刷新。 */
static void divoom_music_ctrl_notify_mode_change(uint8 mode)
{
    if (divoom_music_ctrl_get_mode() != mode)
    {
        return;
    }

	if (!divoom_lvgl_top_audio_menu_is_open())
		divoom_music_ctrl_request_mode_ui_refresh();
	/* 插 TF / 插 USB→UAC 等被动切模式：音源菜单内跟到对应播放页。
	 * 插入 tip OSD 仍由 main 热插拔回调弹出，这里只补跳转。 */
	divoom_astrootoo_source_goto_music_mode(mode);
    divoom_print("music mode switched to: %d\n", mode);
}

/*********************************************************************
  Function:        divoom_music_ctrl_get_mode
  Description:     获取当前音乐模式
  Input:           none
  Return:          当前模式 
**********************************************************************/

/* source锁覆盖调度push/cancel，不能让旧停止晚于新音源获取共享语音槽。 */
static void divoom_music_ctrl_switch_mode_ex(uint8 mode, uint8 send_jl_set,
    uint8 voice_owned, int32 radio_generation)
{
    if (!pdivoom_music_ctrl_handle || !pdivoom_music_ctrl_handle->source_lock)
    {
        return;
    }
    divoom_system_get_lock(pdivoom_music_ctrl_handle->source_lock);
    uint8 changed = divoom_music_ctrl_switch_mode_locked(mode, send_jl_set, voice_owned, radio_generation);
    divoom_system_free_lock(pdivoom_music_ctrl_handle->source_lock);
    if (changed)
    {
        divoom_music_ctrl_notify_mode_change(mode);
    }
}

void divoom_music_ctrl_radio_source_request(int32 generation, uint8 active)
{
#if DIVOOM_MUSIC_RADIO_SOURCE
    uint8 cancel = 0;
    if (active)
    {
        divoom_music_ctrl_switch_mode_ex(DIVOOM_MUSIC_CTRL_MODE_NET_FM, 1, 0, generation);
        return;
    }
    if (!pdivoom_music_ctrl_handle || !pdivoom_music_ctrl_handle->source_lock)
    {
        return;
    }
    divoom_system_get_lock(pdivoom_music_ctrl_handle->source_lock);
    divoom_system_get_lock(pdivoom_music_ctrl_handle->lock_handle);
    if (pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_NET_FM &&
        divoom_radio_request_current(generation))
    {
        /* select_id(0)已原子停止radio；这里只提交缓存，不能再次停止更新的选台。 */
        pdivoom_music_ctrl_handle->playing = 0;
        pdivoom_music_ctrl_handle->transport_state = DIVOOM_MUSIC_TRANSPORT_PAUSED;
        s_music_voice_saved_playing = 0;
        cancel = 1;
    }
    divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
    if (cancel)
    {
        divoom_music_ctrl_sync_voice_schedule_locked(0);
    }
    divoom_system_free_lock(pdivoom_music_ctrl_handle->source_lock);
#else
    (void)generation;
    (void)active;
#endif
}

uint8 divoom_music_ctrl_get_mode(void)
{
    if(pdivoom_music_ctrl_handle == NULL)
    {
        return DIVOOM_MUSIC_CTRL_MODE_BT;
    }
    return pdivoom_music_ctrl_handle->mode;
}

/*********************************************************************
  Function:        divoom_music_ctrl_stop_play
  Description:     停止当前模式播放（不切换模式）
  Input:           none
  Return:          NONE 
**********************************************************************/

void divoom_music_ctrl_stop_play(void)
{
    if(pdivoom_music_ctrl_handle == NULL)
    {
        return;
    }
    
    divoom_system_get_lock(pdivoom_music_ctrl_handle->lock_handle);
    divoom_music_ctrl_stop_current_mode();
    divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
	divoom_music_ctrl_sync_voice_schedule(0);
}

/*********************************************************************
  Function:        divoom_music_ctrl_post_play_status
  Description:     JL上报播放状态
  Input:           play_status  1:播放 0:暂停
  Return:          NONE 
**********************************************************************/

void divoom_music_ctrl_post_play_status(uint8 play_status)
{
    if(pdivoom_music_ctrl_handle == NULL)
    {
        return;
    }

#if DIVOOM_MUSIC_RADIO_SOURCE
    /* JL状态只描述JL媒体，不能覆盖本机电台状态。 */
    if (pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_NET_FM)
    {
        return;
    }
#endif
	pdivoom_music_ctrl_handle->playing = play_status ? 1 : 0;
	divoom_music_ctrl_post_transport_state(play_status ?
        DIVOOM_MUSIC_TRANSPORT_PLAYING : DIVOOM_MUSIC_TRANSPORT_PAUSED);
	divoom_music_ctrl_notify_play_status_changed();
}


/*********************************************************************
  Function:        divoom_music_ctrl_set_play
  Description:     设置播放/暂停状态
  Input:           play_status  1:播放 0:暂停
  Return:          NONE 
**********************************************************************/

void divoom_music_ctrl_pause_output_only(void)
{
	if (pdivoom_music_ctrl_handle == NULL)
		return;

	divoom_system_get_lock(pdivoom_music_ctrl_handle->lock_handle);
	divoom_music_ctrl_set_play_internal(0);
	divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
}

void divoom_music_ctrl_resume_output_only(void)
{
	if (pdivoom_music_ctrl_handle == NULL)
		return;

	divoom_system_get_lock(pdivoom_music_ctrl_handle->lock_handle);
	divoom_music_ctrl_set_play_internal(1);
	divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
}

int divoom_music_ctrl_alarm_play_bt(uint8 alarm_vol_ui)
{
	uint8 jl_wm;
	uint8 jl_vol;
	uint8 prev_mode;

	if (!divoom_music_ctrl_is_ready())
		return -1;
	if (!divoom_btcore_uart_cmd_is_ready())
		return -1;
	if (!divoom_bt_music_is_connected())
		return -1;

	if (alarm_vol_ui > 100)
		alarm_vol_ui = 100;

	jl_wm = divoom_music_ctrl_local_to_jl_work_mode(DIVOOM_MUSIC_CTRL_MODE_BT);
	jl_vol = (uint8)divoom_com_vol_ui_to_level(alarm_vol_ui);

	divoom_system_get_lock(pdivoom_music_ctrl_handle->lock_handle);
	prev_mode = pdivoom_music_ctrl_handle->mode;
	if (prev_mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
		divoom_tf_music_stop();
	if (prev_mode != DIVOOM_MUSIC_CTRL_MODE_BT)
		divoom_music_ctrl_clear_jl_tf_state();
	pdivoom_music_ctrl_handle->last_mode = prev_mode;
#if DIVOOM_MUSIC_RADIO_SOURCE
    divoom_radio_set_play(0);
#endif
	pdivoom_music_ctrl_handle->mode = DIVOOM_MUSIC_CTRL_MODE_BT;
    __atomic_add_fetch(&pdivoom_music_ctrl_handle->mode_generation, 1, __ATOMIC_RELEASE);
	pdivoom_music_ctrl_handle->volume = alarm_vol_ui;
	divoom_music_ctrl_jl_send_work_mode_set(jl_wm, jl_vol);
	divoom_music_ctrl_set_play_internal(1);
	divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);

	divoom_print("music ctrl: alarm bt play vol=%u\n", (unsigned)alarm_vol_ui);
	return 0;
}

void divoom_music_ctrl_pause_for_tone(void)
{
	if (pdivoom_music_ctrl_handle == NULL)
		return;

#ifdef CONFIG_DIVOOM_MPP_NET_AUDIO
    /* 短提示仅压低NETWORK，不破坏直播连接和压缩储备。 */
    if (pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_NET_FM)
    {
        return;
    }
#endif
	divoom_system_get_lock(pdivoom_music_ctrl_handle->lock_handle);
	if (!pdivoom_music_ctrl_handle->playing)
	{
		divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
		return;
	}
	s_tone_overlay_saved_playing = 1;
	divoom_music_ctrl_set_play_internal(0);
	divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
}

void divoom_music_ctrl_resume_after_tone(void)
{
	if (pdivoom_music_ctrl_handle == NULL)
		return;
	if (!s_tone_overlay_saved_playing && !s_music_voice_saved_playing)
		return;

	divoom_system_get_lock(pdivoom_music_ctrl_handle->lock_handle);
	if (s_tone_overlay_saved_playing)
		divoom_music_ctrl_set_play_internal(1);
	else if (s_music_voice_saved_playing)
		divoom_music_ctrl_set_play_internal(1);
	s_tone_overlay_saved_playing = 0;
	s_music_voice_saved_playing = 0;
	divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
}

void divoom_music_ctrl_toggle_play(void)
{
	uint32 schedule_ret;

	if (pdivoom_music_ctrl_handle == NULL)
		return;

#if DIVOOM_MUSIC_RADIO_SOURCE
    if (pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_NET_FM)
    {
        divoom_music_ctrl_set_play(divoom_radio_is_playing() ? 0 : 1);
        return;
    }
#endif
	if (divoom_music_ctrl_jl_pp_unsafe())
	{
		divoom_print("music ctrl: drop PP during TF teardown\n");
		return;
	}

#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
	if (pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_UAC)
	{
		divoom_music_ctrl_toggle_play_uac();
		return;
	}
#endif
#if DIVOOM_SHOW_TF_NAND
	if (pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
	{
		divoom_music_ctrl_set_play(divoom_music_ctrl_is_playing() ? 0 : 1);
		return;
	}
#endif

	if (divoom_music_ctrl_is_playing())
	{
		divoom_system_get_lock(pdivoom_music_ctrl_handle->lock_handle);
		divoom_music_ctrl_send_pp_toggle();
		divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
		divoom_music_ctrl_sync_voice_schedule(0);
		s_music_voice_saved_playing = 0;
	}
	else
	{
		schedule_ret = divoom_music_ctrl_sync_voice_schedule(1);
		if (schedule_ret != 0)
		{
			divoom_system_get_lock(pdivoom_music_ctrl_handle->lock_handle);
			divoom_music_ctrl_send_pp_toggle();
			divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
			s_music_voice_saved_playing = 0;
		}
		else
		{
			divoom_system_get_lock(pdivoom_music_ctrl_handle->lock_handle);
			pdivoom_music_ctrl_handle->playing = 1;
			divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
			s_music_voice_saved_playing = 1;
		}
	}
}

static void divoom_music_ctrl_set_play_locked(uint8 play_status)
{
	uint32 schedule_ret;

    if(pdivoom_music_ctrl_handle == NULL)
    {
        return;
    }

	if (play_status)
	{
		schedule_ret = divoom_music_ctrl_sync_voice_schedule_locked(1);
		if (schedule_ret != 0)
		{
			divoom_system_get_lock(pdivoom_music_ctrl_handle->lock_handle);
			divoom_music_ctrl_set_play_internal(1);
			if (pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
				divoom_music_ctrl_jl_tf_mark_ready();
			divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
			s_music_voice_saved_playing = 0;
		}
		else
		{
			/* 未拿到语音队首，仅记录“待恢复播放”，待 schedule_play 回调触发真正出声 */
			divoom_system_get_lock(pdivoom_music_ctrl_handle->lock_handle);
			pdivoom_music_ctrl_handle->playing = 1;
			divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
			s_music_voice_saved_playing = 1;
		}
	}
	else
	{
		divoom_system_get_lock(pdivoom_music_ctrl_handle->lock_handle);
#if DIVOOM_MUSIC_RADIO_SOURCE
        if (pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_NET_FM)
        {
            divoom_radio_set_play(0);
        }
#endif
		divoom_music_ctrl_set_play_internal(0);
		divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
		divoom_music_ctrl_sync_voice_schedule_locked(0);
		s_music_voice_saved_playing = 0;
	}
}

void divoom_music_ctrl_set_play(uint8 play_status)
{
    if (!pdivoom_music_ctrl_handle || !pdivoom_music_ctrl_handle->source_lock)
    {
        return;
    }
    divoom_system_get_lock(pdivoom_music_ctrl_handle->source_lock);
    divoom_music_ctrl_set_play_locked(play_status);
    divoom_system_free_lock(pdivoom_music_ctrl_handle->source_lock);
}


#if !defined(DIVOOM_OPENSOURCE_SWITCH) && defined(CONFIG_DIVOOM_AI_VOICE) && DIVOOM_AI_VOICE_DIAL_TEST_ENABLE
int divoom_music_ctrl_pause_for_ai_voice(void)
{
	uint8 pause = 1;
	if (!pdivoom_music_ctrl_handle)
		return -3;
#if DIVOOM_SHOW_TF_NAND
	if (pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
	{
		divoom_music_ctrl_set_play(0);
	}
	else
#endif
	{
		/* Explicit pause: a toggle can start idle JL music. Do not depend on
		 * cached playing state or the existing sync-only set_play path. */
		if (!divoom_btcore_uart_cmd_is_ready() ||
		    divoom_btcore_uart_cmd_send(DM_BTCORE_PAUSE_RESUME_MUSIC, &pause, 1) != 0)
			return -6;
	}
	divoom_system_get_lock(pdivoom_music_ctrl_handle->lock_handle);
	pdivoom_music_ctrl_handle->playing = 0;
	pdivoom_music_ctrl_handle->transport_state = DIVOOM_MUSIC_TRANSPORT_PAUSED;
	s_music_voice_saved_playing = s_tone_overlay_saved_playing = 0;
	divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
	divoom_music_ctrl_sync_voice_schedule(0);
	divoom_print("ai_dial2080 music paused\n");
	return 0;
}
#endif

/*********************************************************************
  Function:        divoom_music_ctrl_set_bt_remote_play_state
  Description:     蓝牙管理进程上报的 AVRCP 状态：只更新本地 playing，
                   不调用 divoom_bt_music_set_play_status，避免与手机指令形成回环。
  Input:           play_status  1:播放 0:暂停/停止
  Return:          NONE
**********************************************************************/
void divoom_music_ctrl_set_bt_remote_play_state(uint8 play_status)
{
    uint32 schedule_ret = 0;
    uint8 paused_by_schedule = 0;

    if (pdivoom_music_ctrl_handle == NULL)
        return;

    if (play_status)
    {
        /* 手机端直接点播也要进入语音调度队列，否则闹钟/番茄无法抢占音乐 */
        schedule_ret = divoom_music_ctrl_sync_voice_schedule(1);

        divoom_system_get_lock(pdivoom_music_ctrl_handle->lock_handle);
        if (pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_BT)
        {
            if (schedule_ret != 0)
            {
                pdivoom_music_ctrl_handle->playing = 1;
                s_music_voice_saved_playing = 0;
            }
            else
            {
                /* 未拿到语音队首：保持待恢复，避免与高优先级语音并发 */
                pdivoom_music_ctrl_handle->playing = 1;
                s_music_voice_saved_playing = 1;
                divoom_bt_music_set_play_status(BTMG_AVRCP_PAUSE);
            }
        }
        divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);
    }
    else
    {
        divoom_system_get_lock(pdivoom_music_ctrl_handle->lock_handle);
        if (pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_BT)
        {
            pdivoom_music_ctrl_handle->playing = 0;
            paused_by_schedule = s_music_voice_saved_playing;
        }
        divoom_system_free_lock(pdivoom_music_ctrl_handle->lock_handle);

        /* 被调度抢占导致的 BT pause 仅更新状态，不能把音乐从语音队列 cancel 掉 */
        if (paused_by_schedule)
            return;

        divoom_music_ctrl_sync_voice_schedule(0);
        s_music_voice_saved_playing = 0;
    }
}

/*********************************************************************
  Function:        divoom_music_ctrl_play_next
  Description:     播放下一首（蓝牙/TF/UAC 有效；UAC 由 JL 通知 PC）
  Input:           none
  Return:          NONE 
**********************************************************************/

void divoom_music_ctrl_play_next(void)
{
	uint8 mode;

    if(pdivoom_music_ctrl_handle == NULL)
    {
        return;
    }

	mode = pdivoom_music_ctrl_handle->mode;
	/* 网络电台走 NET_RADIO 语音事件，队首未必是 MUSIC_CTRL，不切此项以免误判 */
	if (mode == DIVOOM_MUSIC_CTRL_MODE_BT || mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
	{
		if (!divoom_music_ctrl_voice_track_allows_track_change())
			return;
	}

    switch(mode)
    {
        case DIVOOM_MUSIC_CTRL_MODE_BT:      // 蓝牙模式
            divoom_bt_music_set_play_next();
            break;
            
        case DIVOOM_MUSIC_CTRL_MODE_LOCAL:   // 本地模式
            divoom_tf_music_music_play_next();
            break;
            
        case DIVOOM_MUSIC_CTRL_MODE_NET_FM:  // 网络电台
            #if DIVOOM_MUSIC_RADIO_SOURCE
            divoom_radio_step(1);
#endif
            break;

        case DIVOOM_MUSIC_CTRL_MODE_LINEIN:  // Line-In模式没有上下首概念
            divoom_print("Line-In mode: no next track\n");
            break;

#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
        case DIVOOM_MUSIC_CTRL_MODE_UAC:
            divoom_music_ctrl_uac_last_next(1);
            break;
#endif
    }
}

/*********************************************************************
  Function:        divoom_music_ctrl_play_pre
  Description:     播放上一首（蓝牙/TF/UAC 有效；UAC 由 JL 通知 PC）
  Input:           none
  Return:          NONE 
**********************************************************************/

void divoom_music_ctrl_play_pre(void)
{
	uint8 mode;

    if(pdivoom_music_ctrl_handle == NULL)
    {
        return;
    }

	mode = pdivoom_music_ctrl_handle->mode;
	/* 网络电台走 NET_RADIO 语音事件，队首未必是 MUSIC_CTRL，不切此项以免误判 */
	if (mode == DIVOOM_MUSIC_CTRL_MODE_BT || mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
	{
		if (!divoom_music_ctrl_voice_track_allows_track_change())
			return;
	}

    switch(mode)
    {
        case DIVOOM_MUSIC_CTRL_MODE_BT:      // 蓝牙模式
            divoom_bt_music_set_play_pre();
            break;
            
        case DIVOOM_MUSIC_CTRL_MODE_LOCAL:   // 本地模式
            divoom_tf_music_music_play_prev();
            break;
            
        case DIVOOM_MUSIC_CTRL_MODE_NET_FM:  // 网络电台
            #if DIVOOM_MUSIC_RADIO_SOURCE
            divoom_radio_step(-1);
#endif
            break;

        case DIVOOM_MUSIC_CTRL_MODE_LINEIN:  // Line-In模式没有上下首概念
            divoom_print("Line-In mode: no previous track\n");
            break;

#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
        case DIVOOM_MUSIC_CTRL_MODE_UAC:
            divoom_music_ctrl_uac_last_next(0);
            break;
#endif
    }
}

/*********************************************************************
  Function:        divoom_music_ctrl_set_volume
  Description:     设置音量
  Input:           volume  0-100
  Return:          NONE 
**********************************************************************/

void divoom_music_ctrl_set_volume(uint8 volume)
{
    if(pdivoom_music_ctrl_handle == NULL)
    {
        return;
    }

	volume = divoom_com_vol_snap_ui(volume);
	/* 相册与音乐同时播放时，旋钮/音量控制优先调音乐音量 */
	if (pdivoom_music_ctrl_handle->mode != DIVOOM_MUSIC_CTRL_MODE_NET_FM &&
        divoom_local_movie_play_is_album_slideshow() && !divoom_music_ctrl_is_playing())
	{
		Pdivoom_pic_phone_manage_slide_config pcfg = divoom_pic_phone_manage_get_slide_config();
		int play_vol;

		if (pcfg != NULL && pcfg->video_volume != volume)
		{
			pcfg->video_volume = volume;
			play_vol = divoom_pic_phone_manage_convert_video_volume(volume);
			divoom_print("album video vol set(music_ctrl): ui=%d jl=%d\n",
				     volume, play_vol);
			divoom_local_movie_play_set_movie_volume((uint8)play_vol);
			divoom_pic_phone_manage_save_slide_config(0);
		}
		pdivoom_music_ctrl_handle->volume = volume;
		return;
	}
	{
		uint8 apply_vol = divoom_music_ctrl_low_power_cap_ui(volume);
		uint8 cmd_data[2] = {0};
		uint8 lv_vol = (uint8)divoom_com_vol_ui_to_level(apply_vol);

#if DIVOOM_SHOW_TF_NAND
		if (pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
		{
			divoom_tf_music_set_volume(apply_vol);
			divoom_audio_sink_ch_set_volume(DIVOOM_AUDIO_SINK_CH_MUSIC, apply_vol);
		}
#endif
#if DIVOOM_MUSIC_RADIO_SOURCE
        if (pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_NET_FM)
        {
            /* 响度走数字 PCM，不发 JL MUSIC_VOLUME；IISIN EQ 按同一音量档跟着变。 */
#ifdef CONFIG_DIVOOM_MPP_NET_AUDIO
            divoom_net_audio_set_volume(apply_vol);
#endif
            pdivoom_music_ctrl_handle->volume = apply_vol;
            divoom_music_ctrl_save_volume_for_mode(DIVOOM_MUSIC_CTRL_MODE_NET_FM, apply_vol);
            return;
        }
#endif
		pdivoom_music_ctrl_handle->volume = apply_vol;
		cmd_data[0] = 1;
		cmd_data[1] = lv_vol;
		if (pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_BT)
			divoom_bt_music_mark_sent_volume(divoom_music_ctrl_get_volume(), apply_vol);
		divoom_btcore_uart_cmd_send(DM_BTCORE_MUSIC_VOLUME, cmd_data, 2);
		divoom_music_ctrl_save_volume_for_mode(pdivoom_music_ctrl_handle->mode, apply_vol);
	}
}

/*********************************************************************
  Function:        divoom_music_ctrl_get_volume
  Description:     获取当前音量
  Input:           none
  Return:          当前音量 0-100，未初始化返回 0
**********************************************************************/

uint8 divoom_music_ctrl_get_volume(void)
{
    uint8 vol;

    if(pdivoom_music_ctrl_handle == NULL)
        return 0;
    vol = divoom_music_ctrl_get_saved_volume_for_mode(pdivoom_music_ctrl_handle->mode);
    return divoom_music_ctrl_low_power_cap_ui(vol);
}

uint8 divoom_music_ctrl_is_phone_call(void)
{
	return (s_jl_passive_work_mode_valid
		&& s_jl_passive_work_mode == DIVOOM_BTCORE_WORK_MODE_PHONE) ? 1u : 0u;
}

uint8 divoom_music_ctrl_get_hfp_call_level(void)
{
	uint8 level;

	if (pdivoom_music_ctrl_handle == NULL)
		return 0;
	level = pdivoom_music_ctrl_handle->hfp_call_level;
	if (level > DIVOOM_BTCORE_HFP_CALL_VOL_MAX)
		level = (uint8)DIVOOM_BTCORE_HFP_CALL_VOL_MAX;
	return level;
}

uint8 divoom_music_ctrl_get_hfp_call_volume_ui(void)
{
	return divoom_com_hfp_call_level_to_ui(divoom_music_ctrl_get_hfp_call_level());
}

void divoom_music_ctrl_set_hfp_call_volume(uint8 level)
{
	uint8 cmd_data[2];

	if (pdivoom_music_ctrl_handle == NULL)
		return;
	if (level > DIVOOM_BTCORE_HFP_CALL_VOL_MAX)
		level = (uint8)DIVOOM_BTCORE_HFP_CALL_VOL_MAX;
	pdivoom_music_ctrl_handle->hfp_call_level = level;
	pdivoom_music_ctrl_handle->hfp_call_level_valid = 1;
	cmd_data[0] = (uint8)DIVOOM_BTCORE_HFP_CALL_VOL_OP_SET;
	cmd_data[1] = level;
	if (divoom_btcore_uart_cmd_is_ready())
		divoom_btcore_uart_cmd_send(DM_BTCORE_HFP_CALL_VOLUME, cmd_data, 2);
}

void divoom_music_ctrl_on_hfp_call_volume_notify(uint8 level)
{
	if (pdivoom_music_ctrl_handle == NULL)
		return;
	if (level > DIVOOM_BTCORE_HFP_CALL_VOL_MAX)
		level = (uint8)DIVOOM_BTCORE_HFP_CALL_VOL_MAX;
	pdivoom_music_ctrl_handle->hfp_call_level = level;
	pdivoom_music_ctrl_handle->hfp_call_level_valid = 1;
}

/*********************************************************************
  Function:        divoom_music_ctrl_sync_play_status_from_jl
  Description:     向 JL 查询当前播放/暂停状态，同步到 playing（App 重启后 JL 可能仍在播）
  Input:           none
  Return:          0 成功；-1 查询失败或未就绪
**********************************************************************/
int divoom_music_ctrl_sync_play_status_from_jl(void)
{
	uint8 reply = 0;
	uint16_t reply_len = 0;
	int ret;

#if DIVOOM_MUSIC_RADIO_SOURCE
    /* 电台状态由本机播放器提供，不用JL查询结果覆盖。 */
    if (pdivoom_music_ctrl_handle && pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_NET_FM)
    {
        return 0;
    }
#endif
	if (pdivoom_music_ctrl_handle == NULL || !divoom_btcore_uart_cmd_is_ready())
		return -1;

	ret = divoom_btcore_uart_cmd_send_wait(DM_BTCORE_MUSIC_STATUS, NULL, 0,
					       &reply, 1, &reply_len);
	if ((ret != 0 || reply_len < 1u)
	    && pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
	{
		ret = divoom_btcore_uart_cmd_send_wait(DM_BTCORE_SD_MUSIC_STATUS, NULL, 0,
						       &reply, 1, &reply_len);
	}

	if (ret != 0 || reply_len < 1u)
		return -1;

	{
		uint8 playing = reply ? 1 : 0;
		uint8 changed = (pdivoom_music_ctrl_handle->playing != playing);

		pdivoom_music_ctrl_handle->playing = playing;
		divoom_music_ctrl_post_transport_state(playing ?
			DIVOOM_MUSIC_TRANSPORT_PLAYING : DIVOOM_MUSIC_TRANSPORT_PAUSED);
		if (changed)
			divoom_music_ctrl_notify_play_status_changed();
		divoom_print("music ctrl: jl play status synced -> %u\n", (unsigned)reply);
	}
	return 0;
}

uint8 divoom_music_ctrl_is_playing(void)
{
    if(pdivoom_music_ctrl_handle == NULL)
    {
        return 0;
    }
#if DIVOOM_MUSIC_RADIO_SOURCE
    if (pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_NET_FM)
    {
        /* UI/AI 读取真实播放确认，连接中不冒充已播放；按键仍查询电台播放意图。 */
        return divoom_radio_get_play_state() == DIVOOM_RADIO_PLAYING;
    }
#endif
    return pdivoom_music_ctrl_handle->playing;
}

void divoom_music_ctrl_set_playing_flag(uint8 play_status)
{
	if (pdivoom_music_ctrl_handle == NULL)
		return;
	pdivoom_music_ctrl_handle->playing = play_status ? 1 : 0;
    /* 旧布尔接口没有暂停语义，不能由一次 0 猜测 PAUSE。 */
    divoom_music_ctrl_post_transport_state(play_status ?
        DIVOOM_MUSIC_TRANSPORT_PLAYING : DIVOOM_MUSIC_TRANSPORT_STOPPED);
}

void divoom_music_ctrl_post_transport_state(uint8 state)
{
    if (pdivoom_music_ctrl_handle != NULL && state <= DIVOOM_MUSIC_TRANSPORT_PAUSED)
    {
        pdivoom_music_ctrl_handle->transport_state = state;
    }
}

uint8 divoom_music_ctrl_get_transport_state(void)
{
    if (pdivoom_music_ctrl_handle == NULL)
    {
        return DIVOOM_MUSIC_TRANSPORT_STOPPED;
    }
#if DIVOOM_MUSIC_RADIO_SOURCE
    if (pdivoom_music_ctrl_handle->mode == DIVOOM_MUSIC_CTRL_MODE_NET_FM)
    {
        divoom_radio_play state = divoom_radio_get_play_state();
        if (state == DIVOOM_RADIO_PLAYING)
        {
            return DIVOOM_MUSIC_TRANSPORT_PLAYING;
        }
        /* 暂停保留原三态；首次连接、重连及失败均不能继续上报正在播放。 */
        if (state == DIVOOM_RADIO_STOPPED &&
            pdivoom_music_ctrl_handle->transport_state == DIVOOM_MUSIC_TRANSPORT_PAUSED)
        {
            return DIVOOM_MUSIC_TRANSPORT_PAUSED;
        }
        return DIVOOM_MUSIC_TRANSPORT_STOPPED;
    }
#endif
    if (pdivoom_music_ctrl_handle->playing)
    {
        return DIVOOM_MUSIC_TRANSPORT_PLAYING;
    }
    return pdivoom_music_ctrl_handle->transport_state == DIVOOM_MUSIC_TRANSPORT_PAUSED ?
        DIVOOM_MUSIC_TRANSPORT_PAUSED : DIVOOM_MUSIC_TRANSPORT_STOPPED;
}

/*********************************************************************
  Function:        divoom_music_ctrl_restore_last_mode
  Description:     恢复到上一个模式
  Input:           none
  Return:          NONE 
**********************************************************************/

void divoom_music_ctrl_restore_last_mode(void)
{
    if(pdivoom_music_ctrl_handle == NULL)
    {
        return;
    }
    
    divoom_print("restoring to last mode: %d\n", pdivoom_music_ctrl_handle->last_mode);
    divoom_music_ctrl_apply_mode(pdivoom_music_ctrl_handle->last_mode);
}

void divoom_music_ctrl_on_jl_sd_play_info(uint32 cur_time_sec, uint32 totle_time_sec, uint8 status)
{
#if DIVOOM_SHOW_TF_NAND
	(void)cur_time_sec;
	(void)totle_time_sec;
	(void)status;
	return;
#endif
	if (cur_time_sec == 0xFFFFu)
		cur_time_sec = 0;
	if (totle_time_sec == 0xFFFFu)
		totle_time_sec = 0;
	if (totle_time_sec > 0u && cur_time_sec > totle_time_sec)
		cur_time_sec = totle_time_sec;

	s_tf_play_info.cur_time = cur_time_sec;
	s_tf_play_info.totle_time = totle_time_sec;
	s_tf_play_info.status = status ? 1u : 0u;
}

Pdivoom_music_ctrl_tf_play_info divoom_music_ctrl_get_tf_info(void)
{
	if (pdivoom_music_ctrl_handle == NULL
	    || pdivoom_music_ctrl_handle->mode != DIVOOM_MUSIC_CTRL_MODE_LOCAL)
		return NULL;
#if DIVOOM_SHOW_TF_NAND
	s_tf_play_info.cur_time = (uint32)divoom_local_music_ctrl_get_music_position_sec();
	s_tf_play_info.totle_time = (uint32)divoom_local_music_ctrl_get_music_duration_sec();
#endif
	s_tf_play_info.status = divoom_music_ctrl_is_playing() ? 1 : 0;
	return &s_tf_play_info;
}

void divoom_music_ctrl_poll_tf_play_info(void)
{
	Pdivoom_music_ctrl_tf_play_info info = divoom_music_ctrl_get_tf_info();

	if (info)
	{
#if DIVOOM_SHOW_TF_NAND
		info->cur_time = (uint32)divoom_local_music_ctrl_get_music_position_sec();
		info->totle_time = (uint32)divoom_local_music_ctrl_get_music_duration_sec();
#endif
		info->status = divoom_music_ctrl_is_playing() ? 1 : 0;
	}
}

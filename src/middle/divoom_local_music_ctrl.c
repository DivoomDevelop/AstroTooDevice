#include "divoom.h"
#include "divoom_bt_music.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_poweron.h"
#include "divoom_local_movie_play.h"
#include "divoom_tone_lite_decode.h"
#include "divoom_audio_policy.h"
#include "divoom_audio_sink.h"
#include "divoom_audio_session.h"
#include "divoom_sleep_pcm_session.h"
#include "divoom_local_music_ctrl_tone_priv.h"
#include "divoom_tone_service.h"

#ifndef DIVOOM_TONE_I2S_RAMP_MS
#define DIVOOM_TONE_I2S_RAMP_MS  30
#endif

#ifndef DIVOOM_LOCAL_MUSIC_DEFAULT_VOL
#define DIVOOM_LOCAL_MUSIC_DEFAULT_VOL  DIVOOM_TPLAYER_SET_DEFAULT_VOL
#endif

#define DIVOOM_TONE_MIN_AUDIO_BYTES  1024

static int divoom_local_music_ctrl_tone_path_size(const char *path)
{
	const char *fs_path;

	if (!path || path[0] == '\0')
	{
		return -1;
	}
	fs_path = (strncmp(path, "file://", 7) == 0) ? (path + 7) : path;
	if (fs_path[0] == '\0')
	{
		return -1;
	}
	return divoom_app_com_get_file_size((char *)fs_path);
}

static int divoom_local_music_ctrl_tone_src_usable(const char *path)
{
	int sz;

	sz = divoom_local_music_ctrl_tone_path_size(path);
	if (sz < DIVOOM_TONE_MIN_AUDIO_BYTES)
	{
		divoom_print("tone skip bad file: %s size=%d\n", path, sz);
		return 0;
	}
	if (divoom_voice_clip_path_coding_type(path) == DIVOOM_TONE_CODING_NONE)
	{
		divoom_print("tone skip bad file: %s header not audio\n", path);
		return 0;
	}
	return 1;
}

static void divoom_local_music_ctrl_tone_fail_callback(
	divoom_local_music_ctrl_music_play_get_next *cb, const char *music_name)
{
	if (cb)
		cb(music_name ? music_name : (char *)"", 0);
}

#ifndef AudioPcmData
typedef struct AudioPcmData {
	void *pData;
	unsigned int nSize;
	int channels;
	int samplerate;
	int accuracy;
} AudioPcmData;
#endif

static void divoom_local_music_ctrl_tone_lite_notify(void *user, int event);

static void divoom_local_music_ctrl_tone_destroy_handle(
	Pdivoom_local_music_ctrl_tina_music_item play_handle)
{
	if (!play_handle || !play_handle->playhandle)
	{
		return;
	}
	divoom_tone_lite_destroy_abandon(play_handle->playhandle);
	play_handle->playhandle = NULL;
}

static int divoom_local_music_ctrl_tone_ensure_handle(
	Pdivoom_local_music_ctrl_tina_music_item play_handle)
{
	if (!play_handle)
	{
		return -1;
	}
	if (play_handle->playhandle)
	{
		return 0;
	}
	play_handle->playhandle = divoom_tone_lite_create(
		divoom_local_music_ctrl_tone_lite_notify, play_handle);
	return play_handle->playhandle ? 0 : -1;
}

static void divoom_local_music_ctrl_tone_request_stop(
	Pdivoom_local_music_ctrl_tina_music_item play_handle)
{
	if (!play_handle || !play_handle->playhandle)
	{
		return;
	}
	divoom_tone_lite_request_stop(play_handle->playhandle);
}

static void divoom_local_music_ctrl_tone_close_sink_if_open(
	Pdivoom_local_music_ctrl_tina_music_item play_handle)
{
	if (!play_handle || !play_handle->playhandle)
	{
		return;
	}
	divoom_tone_lite_close_sink_if_open(play_handle->playhandle);
}

static int divoom_local_music_ctrl_tone_is_playing(
	Pdivoom_local_music_ctrl_tina_music_item play_handle)
{
	if (!play_handle || !play_handle->playhandle)
	{
		return 0;
	}
	return divoom_tone_lite_is_playing(play_handle->playhandle);
}

static int divoom_local_music_ctrl_tone_stop_requested(
	Pdivoom_local_music_ctrl_tina_music_item play_handle)
{
	if (!play_handle || !play_handle->playhandle)
	{
		return 0;
	}
	return divoom_tone_lite_stop_requested(play_handle->playhandle);
}

static void divoom_local_music_ctrl_tone_set_volume(
	Pdivoom_local_music_ctrl_tina_music_item play_handle, int vol)
{
	if (!play_handle || !play_handle->playhandle)
	{
		return;
	}
	divoom_tone_lite_set_volume(play_handle->playhandle, vol);
}

static void divoom_local_music_ctrl_tone_set_jl_vol(
	Pdivoom_local_music_ctrl_tina_music_item play_handle, int vol)
{
	if (!play_handle || !play_handle->playhandle)
	{
		return;
	}
	divoom_tone_lite_set_jl_tone_vol(play_handle->playhandle, vol);
}

static int divoom_local_music_ctrl_tone_play_until(
	Pdivoom_local_music_ctrl_tina_music_item play_handle, const char *path, int loop,
	unsigned int end_ticks)
{
	if (!play_handle || !play_handle->playhandle)
	{
		return -1;
	}
	return divoom_tone_lite_play_until(play_handle->playhandle, path, loop, end_ticks);
}

static void divoom_local_music_ctrl_tone_play_run(
	Pdivoom_local_music_ctrl_tina_music_item play_handle)
{
	if (!play_handle || !play_handle->playhandle)
	{
		return;
	}
	divoom_tone_lite_play_run(play_handle->playhandle);
}


static Pdivoom_local_music_ctrl_tina_ctrl pdivoom_local_music_ctrl_tina_handle = NULL;

void divoom_local_music_ctrl_play(Pdivoom_local_music_ctrl_tina_music_item play_handle,int volume, char *music_name, divoom_local_music_ctrl_music_play_get_next *pnext_callback);
void divoom_local_music_ctrl_music_play_internal(Pdivoom_local_music_ctrl_tina_music_item play_handle,int volume, char *music_name, divoom_local_music_ctrl_music_play_get_next *pnext_callback);
static void divoom_local_music_ctrl_tone_play_internal(Pdivoom_local_music_ctrl_tina_music_item play_handle,int volume, char *music_name, divoom_local_music_ctrl_music_play_get_next *pnext_callback);

#define DIVOOM_TONE_PAUSED_MODE_NONE    0
#define DIVOOM_TONE_PAUSED_MODE_LOCAL   1
#define DIVOOM_TONE_PAUSED_MODE_BT      2
#define DIVOOM_TONE_PAUSED_MODE_LINEIN  3
#define DIVOOM_TONE_PAUSED_MODE_UAC     4

/* tip 已改走 MEDIA_VOLUME duck；保留 pause/resume 路径供其它调用方复用 */
static void __attribute__((unused))
divoom_local_music_ctrl_resume_music_after_tone_if_needed(void)
{
	Pdivoom_local_music_ctrl_tina_data_item music_item;

	if (!pdivoom_local_music_ctrl_tina_handle)
		return;
	if (!pdivoom_local_music_ctrl_tina_handle->tone_pause_music_pending_resume)
		return;

	pdivoom_local_music_ctrl_tina_handle->tone_pause_music_pending_resume = 0;
	music_item = (Pdivoom_local_music_ctrl_tina_data_item)divoom_system_malloc(sizeof(divoom_local_music_ctrl_tina_data_item));
	if (music_item)
	{
		music_item->type = DIVOOM_LOCAL_MUSIC_TYPE_RESUME;
		music_item->music_name = NULL;
		divoom_queue_push(pdivoom_local_music_ctrl_tina_handle->music_item.play_queue, music_item);
		divoom_system_put_sem(pdivoom_local_music_ctrl_tina_handle->music_item.music_manage_sem);
	}
}

static void __attribute__((unused))
divoom_local_music_ctrl_pause_active_mode_for_tone(void)
{
	uint8 mode;

	if (!pdivoom_local_music_ctrl_tina_handle)
		return;
	if (pdivoom_local_music_ctrl_tina_handle->tone_paused_mode != DIVOOM_TONE_PAUSED_MODE_NONE)
		return;
	if (!divoom_music_ctrl_is_playing())
		return;

#if defined(DIVOOM_TIMES_VIBE)
	/* BT/TF/UAC/LineIn 等均经 divoom_music_ctrl_set_play_internal → JL UART */
	divoom_music_ctrl_pause_for_tone();
	pdivoom_local_music_ctrl_tina_handle->tone_pause_music_pending_resume = 1;
	return;
#endif

	mode = divoom_music_ctrl_get_mode();
	if (mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
	{
		Pdivoom_local_music_ctrl_tina_data_item pause_item =
			(Pdivoom_local_music_ctrl_tina_data_item)divoom_system_malloc(sizeof(divoom_local_music_ctrl_tina_data_item));
		if (pause_item)
		{
			pause_item->type = DIVOOM_LOCAL_MUSIC_TYPE_PAUSE;
			pause_item->music_name = NULL;
			divoom_queue_push(pdivoom_local_music_ctrl_tina_handle->music_item.play_queue, pause_item);
			divoom_system_put_sem(pdivoom_local_music_ctrl_tina_handle->music_item.music_manage_sem);
			pdivoom_local_music_ctrl_tina_handle->tone_pause_music_pending_resume = 1;
			pdivoom_local_music_ctrl_tina_handle->tone_paused_mode = DIVOOM_TONE_PAUSED_MODE_LOCAL;
		}
		return;
	}

	divoom_music_ctrl_pause_for_tone();
	pdivoom_local_music_ctrl_tina_handle->tone_pause_music_pending_resume = 1;
	pdivoom_local_music_ctrl_tina_handle->tone_paused_mode = DIVOOM_TONE_PAUSED_MODE_BT;
}

/* 开机视频与提示音共用杰理门控，提示音结束勿误关功放 */
static int divoom_local_music_ctrl_should_keep_jl_volume_for_poweron_video(void)
{
	return divoom_local_movie_play_should_keep_jl_tone_volume();
}

static void divoom_local_music_ctrl_mute_jl_after_tone_if_needed(void)
{
	if (divoom_local_music_ctrl_should_keep_jl_volume_for_poweron_video())
	{
		return;
	}
	if (divoom_audio_policy_keep_jl_after_tone())
	{
		return;
	}
	/*
	 * EOF/切歌 keep：F101 I2S 仍开着时禁止 tip_release。
	 * JL 控的是 I2S 通路音量，keep 空窗里反复 100↔0 是头尾破音主因；
	 * 真正关 JL 只在 close_sink / release_kept（数字静音冲刷之后）。
	 */
	if (pdivoom_local_music_ctrl_tina_handle
	    && pdivoom_local_music_ctrl_tina_handle->tone_item.playhandle
	    && divoom_tone_lite_sink_is_open(
		       pdivoom_local_music_ctrl_tina_handle->tone_item.playhandle))
	{
		divoom_print("amp_gate: skip tip_release (tip i2s keep)\n");
		return;
	}
	divoom_audio_policy_amp_tip_release();
}

/* 开机视频同步提示音：仅放行 poweron.mp3，其它 tone 仍 defer/拦截 */
static int divoom_local_music_ctrl_is_poweron_boot_tone(const char *music_name)
{
	const char *base;

	if (!music_name || music_name[0] == '\0')
		return 0;
	base = strrchr(music_name, '/');
	base = base ? (base + 1) : music_name;
	return (strcmp(base, "poweron.mp3") == 0) ? 1 : 0;
}

static int divoom_local_music_ctrl_is_poweroff_tone(const char *music_name)
{
	const char *base;

	if (!music_name || music_name[0] == '\0')
		return 0;
	base = strrchr(music_name, '/');
	base = base ? (base + 1) : music_name;
	return (strcmp(base, "poweroff.mp3") == 0) ? 1 : 0;
}

/* 关机流程中仅允许 poweroff.mp3；其它 tone/stop 拒绝，避免 CUT 打断关机音 */
static int divoom_local_music_ctrl_reject_tone_during_poweroff(const char *music_name)
{
	if (!divoom_onoff_is_poweroff_active())
		return 0;
	if (divoom_local_music_ctrl_is_poweroff_tone(music_name))
		return 0;
	divoom_print("tone suppressed: poweroff active %s\n",
		     music_name ? music_name : "?");
	return 1;
}

static int divoom_local_music_ctrl_skip_tone_for_poweron_video(const char *music_name)
{
	if (divoom_product_test_is_active())
		return 0;
	if (divoom_local_music_ctrl_is_poweron_boot_tone(music_name))
		return 0;
	if (divoom_lvgl_poweron_is_video_phase())
	{
		divoom_print("skip tone: poweron video playing\n");
		return 1;
	}
	if (!divoom_menu_control_is_initialized())
		return 1;
	return 0;
}

static void divoom_local_music_ctrl_tone_flush_queue(void)
{
	Pdivoom_local_music_ctrl_tina_data_item tmp;

	if (!pdivoom_local_music_ctrl_tina_handle)
		return;
	while (divoom_queue_items(pdivoom_local_music_ctrl_tina_handle->tone_item.play_queue))
	{
		tmp = divoom_queue_pop(pdivoom_local_music_ctrl_tina_handle->tone_item.play_queue);
		if (tmp)
		{
			if (tmp->music_name)
				divoom_system_free(tmp->music_name);
			divoom_system_free(tmp);
		}
	}
}

static void divoom_local_music_ctrl_tone_free_queue_item(
	Pdivoom_local_music_ctrl_tina_data_item item)
{
	if (!item)
	{
		return;
	}
	if (item->music_name)
	{
		divoom_system_free(item->music_name);
	}
	divoom_system_free(item);
}

/* Step5a：合并待发 PLAY/VOL，保留 STOP/CUT */
static void divoom_local_music_ctrl_tone_flush_pending_plays(void)
{
	Pdivoom_local_music_ctrl_tina_data_item item;
	Pdivoom_local_music_ctrl_tina_data_item keep[40];
	int keep_n = 0;
	int n;
	int i;
	void *queue;

	if (!pdivoom_local_music_ctrl_tina_handle)
	{
		return;
	}
	queue = pdivoom_local_music_ctrl_tina_handle->tone_item.play_queue;
	n = (int)divoom_queue_items(queue);
	for (i = 0; i < n; i++)
	{
		item = (Pdivoom_local_music_ctrl_tina_data_item)divoom_queue_pop(queue);
		if (!item)
		{
			continue;
		}
		if (item->type == DIVOOM_LOCAL_MUSIC_TYPE_PLAY
		    || item->type == DIVOOM_LOCAL_MUSIC_TYPE_SET_VOLUME)
		{
			divoom_local_music_ctrl_tone_free_queue_item(item);
		}
		else if (keep_n < (int)(sizeof(keep) / sizeof(keep[0])))
		{
			keep[keep_n++] = item;
		}
		else
		{
			divoom_local_music_ctrl_tone_free_queue_item(item);
		}
	}
	for (i = 0; i < keep_n; i++)
	{
		divoom_queue_push(queue, keep[i]);
	}
}

/* 未在播时丢弃队列 STOP：避免挡在新 PLAY 前（cut=0 路径） */
static void divoom_local_music_ctrl_tone_drop_pending_stops(void)
{
	Pdivoom_local_music_ctrl_tina_data_item item;
	Pdivoom_local_music_ctrl_tina_data_item keep[40];
	int keep_n = 0;
	int n;
	int i;
	void *queue;

	if (!pdivoom_local_music_ctrl_tina_handle)
	{
		return;
	}
	queue = pdivoom_local_music_ctrl_tina_handle->tone_item.play_queue;
	n = (int)divoom_queue_items(queue);
	for (i = 0; i < n; i++)
	{
		item = (Pdivoom_local_music_ctrl_tina_data_item)divoom_queue_pop(queue);
		if (!item)
		{
			continue;
		}
		if (item->type == DIVOOM_LOCAL_MUSIC_TYPE_STOP)
		{
			divoom_local_music_ctrl_tone_free_queue_item(item);
		}
		else if (keep_n < (int)(sizeof(keep) / sizeof(keep[0])))
		{
			keep[keep_n++] = item;
		}
		else
		{
			divoom_local_music_ctrl_tone_free_queue_item(item);
		}
	}
	for (i = 0; i < keep_n; i++)
	{
		divoom_queue_push(queue, keep[i]);
	}
}

static int divoom_local_music_ctrl_tone_need_cut_for_play(void)
{
	Pdivoom_local_music_ctrl_tina_music_item play_handle;

	if (!pdivoom_local_music_ctrl_tina_handle)
	{
		return 0;
	}
	play_handle = &pdivoom_local_music_ctrl_tina_handle->tone_item;
	if (!play_handle->playhandle)
	{
		return 0;
	}
	if (divoom_local_music_ctrl_tone_is_playing(play_handle))
	{
		return 1;
	}
	return divoom_tone_lite_sink_is_open(play_handle->playhandle) ? 1 : 0;
}

static void divoom_local_music_ctrl_tone_apply_cut(void)
{
	Pdivoom_local_music_ctrl_tina_music_item play_handle;

	if (!pdivoom_local_music_ctrl_tina_handle)
	{
		return;
	}
	play_handle = &pdivoom_local_music_ctrl_tina_handle->tone_item;
	divoom_tone_service_mark_cut_pending();
	if (play_handle->playhandle && divoom_local_music_ctrl_tone_is_playing(play_handle))
	{
		divoom_print("tone cut req while playing tick=%u\n",
			     divoom_system_get_ticks());
		divoom_local_music_ctrl_tone_request_stop(play_handle);
	}
	else if (play_handle->playhandle
		 && divoom_tone_lite_sink_is_open(play_handle->playhandle))
	{
		divoom_print("tone cut keep-idle tick=%u\n", divoom_system_get_ticks());
	}
}

static void divoom_local_music_ctrl_tone_prepare_play_session(void)
{
	if (!pdivoom_local_music_ctrl_tina_handle)
	{
		return;
	}
	/* tip/sleep 期间只压 JL MEDIA_VOLUME，不 pause 音乐、不改 tip 播法 */
	divoom_audio_policy_media_duck_hold("tone");
	/* valid 只作 tip 会话闩，收尾 mute JL，不保存/还原 sys_vol */
	pdivoom_local_music_ctrl_tina_handle->tone_saved_sys_vol_valid = 1;
}

/*
 * Step5a：play_run 热路径轮询控制队列（仅 local_tone 线程调用）。
 * SET_VOLUME/CUT 立即弹出处理；STOP 只 request_stop，收尾留给主循环 STOP 项。
 */
typedef struct {
    int visited;
    int type;
} divoom_local_music_ctrl_tone_head_state;

/* pop_cmp holds the queue lock while inspecting and transferring the head. */
static int divoom_local_music_ctrl_tone_take_control_head(void *context, void *data)
{
    divoom_local_music_ctrl_tone_head_state *state = context;
    Pdivoom_local_music_ctrl_tina_data_item item = data;

    if(state->visited)
    {
        return 1;
    }
    state->visited = 1;
    state->type = item->type;
    return item->type != DIVOOM_LOCAL_MUSIC_TYPE_SET_VOLUME &&
        item->type != DIVOOM_LOCAL_MUSIC_TYPE_CUT;
}

static void divoom_local_music_ctrl_tone_poll_control(void *unused)
{
	Pdivoom_local_music_ctrl_tina_data_item item;
	Pdivoom_local_music_ctrl_tina_music_item play_handle;
	void *queue;

	(void)unused;
	if (!pdivoom_local_music_ctrl_tina_handle)
	{
		return;
	}
	queue = pdivoom_local_music_ctrl_tina_handle->tone_item.play_queue;
	play_handle = &pdivoom_local_music_ctrl_tina_handle->tone_item;
	for (;;)
	{
        divoom_local_music_ctrl_tone_head_state state = {0};

        item = divoom_queue_pop_cmp(queue, &state,
            divoom_local_music_ctrl_tone_take_control_head);
        if(item == NULL)
        {
            if(state.visited && state.type == DIVOOM_LOCAL_MUSIC_TYPE_STOP &&
                play_handle->playhandle)
            {
                divoom_local_music_ctrl_tone_request_stop(play_handle);
            }
            break;
        }
		if (item->type == DIVOOM_LOCAL_MUSIC_TYPE_SET_VOLUME)
		{
			if (play_handle->playhandle)
			{
				divoom_local_music_ctrl_tone_set_volume(play_handle, item->volume);
			}
			divoom_local_music_ctrl_tone_free_queue_item(item);
			continue;
		}
		if (item->type == DIVOOM_LOCAL_MUSIC_TYPE_CUT)
		{
			divoom_local_music_ctrl_tone_apply_cut();
			divoom_local_music_ctrl_tone_free_queue_item(item);
			continue;
		}
        break;
    }
}

static int divoom_local_music_ctrl_tone_enqueue(
	Pdivoom_local_music_ctrl_tina_data_item music_item)
{
	if (!pdivoom_local_music_ctrl_tina_handle || !music_item)
	{
		return 0;
	}
	if (!divoom_queue_push(pdivoom_local_music_ctrl_tina_handle->tone_item.play_queue,
			       music_item))
	{
		divoom_local_music_ctrl_tone_free_queue_item(music_item);
		return 0;
	}
	divoom_system_put_sem(pdivoom_local_music_ctrl_tina_handle->tone_item.music_manage_sem);
	return 1;
}

static void divoom_local_music_ctrl_resume_active_mode_after_tone(void)
{
	if (!pdivoom_local_music_ctrl_tina_handle)
		return;

	if (pdivoom_local_music_ctrl_tina_handle->tone_saved_sys_vol_valid)
	{
		/* 提示音退出保持 JL TONE 为 0；开机视频播放中则保留当前音量 */
		divoom_local_music_ctrl_mute_jl_after_tone_if_needed();
		pdivoom_local_music_ctrl_tina_handle->tone_saved_sys_vol_valid = 0;
	}

	/* 关机提示音已 mute JL，收尾勿 unmute；sleep 仍 hold 时 release 亦应保持 mute */
	if (!divoom_onoff_is_poweroff_active())
		divoom_audio_policy_media_duck_release("tone");
	pdivoom_local_music_ctrl_tina_handle->tone_paused_mode = DIVOOM_TONE_PAUSED_MODE_NONE;
	pdivoom_local_music_ctrl_tina_handle->tone_pause_music_pending_resume = 0;
}

/*
 * B1：见 divoom_tone_service_finish_after_run
 */
static void divoom_local_music_ctrl_tone_finish_after_run(
	Pdivoom_local_music_ctrl_tina_music_item play_handle)
{
	divoom_tone_service_finish_after_run(play_handle);
}

#if DIVOOM_SHOW_MODE
static volatile int s_show_drop_play;
#endif

#if DIVOOM_SHOW_TF_NAND
#define DIVOOM_SHOW_TF_SKIP_MAX 16

static int s_show_tf_skip_fails;

static void divoom_local_music_ctrl_show_enqueue_next(
	Pdivoom_local_music_ctrl_tina_music_item play_handle,
	divoom_local_music_ctrl_music_play_get_next *pnext_callback, int volume, int status)
{
	Pdivoom_local_music_ctrl_tina_data_item next_item;

	if (!play_handle || !pnext_callback || !pdivoom_local_music_ctrl_tina_handle)
		return;
	if (status == 0) {
		if (s_show_tf_skip_fails >= DIVOOM_SHOW_TF_SKIP_MAX) {
			divoom_print("show tf: stop auto skip after %d fails\n", s_show_tf_skip_fails);
			return;
		}
		s_show_tf_skip_fails++;
	} else {
		s_show_tf_skip_fails = 0;
	}
	play_handle->next_str[0] = '\0';
	if (!pnext_callback(play_handle->next_str, status) || !play_handle->next_str[0])
		return;
	next_item = (Pdivoom_local_music_ctrl_tina_data_item)divoom_system_malloc(
		sizeof(divoom_local_music_ctrl_tina_data_item));
	if (!next_item)
		return;
	next_item->type = DIVOOM_LOCAL_MUSIC_TYPE_PLAY;
	next_item->music_name = divoom_app_com_copy_string(play_handle->next_str, 0);
	next_item->volume = volume;
	next_item->pnext_callback = pnext_callback;
	next_item->play_time = 0;
	divoom_queue_push(pdivoom_local_music_ctrl_tina_handle->music_item.play_queue, next_item);
	divoom_system_put_sem(pdivoom_local_music_ctrl_tina_handle->music_item.music_manage_sem);
}

static void divoom_local_music_ctrl_show_poll_control(void *user)
{
	Pdivoom_local_music_ctrl_tina_data_item item;

	(void)user;
	if (!pdivoom_local_music_ctrl_tina_handle)
		return;
	while (divoom_queue_items(pdivoom_local_music_ctrl_tina_handle->music_item.play_queue))
	{
		item = divoom_queue_pop(pdivoom_local_music_ctrl_tina_handle->music_item.play_queue);
		if (!item)
			break;
		switch (item->type)
		{
		case DIVOOM_LOCAL_MUSIC_TYPE_SET_VOLUME:
			divoom_local_music_ctrl_tone_set_volume(
				&pdivoom_local_music_ctrl_tina_handle->music_item, item->volume);
			break;
		case DIVOOM_LOCAL_MUSIC_TYPE_PAUSE:
			divoom_tone_lite_set_paused(
				pdivoom_local_music_ctrl_tina_handle->music_item.playhandle, 1);
			pdivoom_local_music_ctrl_tina_handle->music_item.music_status =
				DIVOOM_LOCAL_MUSIC_TYPE_PAUSE;
#if DIVOOM_SHOW_MUSIC_FFT
			divoom_spectrum_set_local(0);
#endif
			break;
		case DIVOOM_LOCAL_MUSIC_TYPE_RESUME:
			divoom_tone_lite_set_paused(
				pdivoom_local_music_ctrl_tina_handle->music_item.playhandle, 0);
			pdivoom_local_music_ctrl_tina_handle->music_item.music_status =
				DIVOOM_LOCAL_MUSIC_TYPE_PLAY;
#if DIVOOM_SHOW_MUSIC_FFT
			divoom_spectrum_set_local(1);
#endif
			break;
		case DIVOOM_LOCAL_MUSIC_TYPE_STOP:
		case DIVOOM_LOCAL_MUSIC_TYPE_CUT:
			divoom_local_music_ctrl_tone_request_stop(
				&pdivoom_local_music_ctrl_tina_handle->music_item);
			break;
		case DIVOOM_LOCAL_MUSIC_TYPE_PLAY:
			if (s_show_drop_play)
			{
				if (item->music_name)
					divoom_system_free(item->music_name);
				divoom_system_free(item);
				break;
			}
			/*
			 * 切歌是 stop + play 连发。本钩子在 play_run 里排空队列，
			 * 若把 PLAY 当 default 丢掉，下一首起不来，再按一次才播，
			 * 表现为隔一首能播。PLAY 还回队列，等本次停完由主循环起播。
			 */
			divoom_queue_push_head(
				pdivoom_local_music_ctrl_tina_handle->music_item.play_queue, item);
			return;
		default:
			break;
		}
		if (item->music_name)
			divoom_system_free(item->music_name);
		divoom_system_free(item);
	}
}

static void divoom_local_music_ctrl_show_music_play_internal(
	Pdivoom_local_music_ctrl_tina_music_item play_handle, int volume, char *music_name,
	divoom_local_music_ctrl_music_play_get_next *pnext_callback)
{
	const char *path = music_name;

	if (path && strncmp(path, "file://", 7) == 0)
		path += 7;
	if (!path || path[0] == '\0' || !divoom_tone_lite_path_supported(path))
	{
		divoom_print("show tf: bad file %s\n", music_name ? music_name : "?");
		divoom_local_music_ctrl_show_enqueue_next(play_handle, pnext_callback, volume, 0);
		return;
	}
	if (divoom_local_music_ctrl_tone_ensure_handle(play_handle) != 0)
	{
		divoom_print("show tf: player handle fail\n");
		divoom_local_music_ctrl_show_enqueue_next(play_handle, pnext_callback, volume, 0);
		return;
	}
	divoom_tone_lite_set_sink_ch(play_handle->playhandle, DIVOOM_AUDIO_SINK_CH_MUSIC);
	divoom_tone_lite_set_play_mode(play_handle->playhandle, DIVOOM_TONE_LITE_MODE_COLD);
	divoom_local_music_ctrl_tone_set_volume(play_handle, volume);
	if (divoom_local_music_ctrl_tone_play_until(play_handle, path, 0, 0) != 0)
	{
		divoom_print("show tf: play_until fail %s\n", path);
		divoom_local_music_ctrl_show_enqueue_next(play_handle, pnext_callback, volume, 0);
		return;
	}
	s_show_tf_skip_fails = 0;
#if DIVOOM_SHOW_MUSIC_FFT
	divoom_spectrum_set_local(1);
#endif
	/*
	 * 连播只走 play_run 返回后的 enqueue。notify 若挂 pcallback 会先
	 * play_get_next 再清空，enqueue 被跳过：歌名变了但不播。
	 * 顺序/随机/单曲循环共用这条，选曲仍由 play_get_next。
	 */
	play_handle->pcallback = NULL;
	play_handle->music_status = DIVOOM_LOCAL_MUSIC_TYPE_PLAY;
	divoom_tone_lite_set_control_hook(play_handle->playhandle,
					  divoom_local_music_ctrl_show_poll_control, NULL);
	divoom_local_music_ctrl_tone_play_run(play_handle);
	if (divoom_local_music_ctrl_tone_stop_requested(play_handle) || s_show_drop_play)
		pdivoom_local_music_ctrl_tina_handle->music_stop_done_flag = 1;
	divoom_tone_lite_set_control_hook(play_handle->playhandle, NULL, NULL);
#if DIVOOM_SHOW_MUSIC_FFT
	divoom_spectrum_set_local(0);
#endif
	divoom_local_music_ctrl_tone_close_sink_if_open(play_handle);
	if (pnext_callback && !divoom_local_music_ctrl_tone_stop_requested(play_handle))
	{
		int vol = pdivoom_local_music_ctrl_tina_handle
				  ? pdivoom_local_music_ctrl_tina_handle->current_music_volume
				  : volume;

		divoom_local_music_ctrl_show_enqueue_next(play_handle, pnext_callback, vol, 1);
	}
}
#endif

void *divoom_local_music_ctrl_play_music_main_loop(void *param)
{
	Pdivoom_local_music_ctrl_tina_data_item music_item;

#if DIVOOM_SHOW_TF_NAND
	(void)param;
	while (1)
	{
		divoom_system_get_sem(pdivoom_local_music_ctrl_tina_handle->music_item.music_manage_sem);
		while (divoom_queue_items(pdivoom_local_music_ctrl_tina_handle->music_item.play_queue))
		{
			music_item = divoom_queue_pop(pdivoom_local_music_ctrl_tina_handle->music_item.play_queue);
			if (!music_item)
				continue;
			switch (music_item->type)
			{
			case DIVOOM_LOCAL_MUSIC_TYPE_PLAY:
				if (s_show_drop_play)
					break;
				divoom_local_music_ctrl_show_music_play_internal(
					&pdivoom_local_music_ctrl_tina_handle->music_item,
					music_item->volume, music_item->music_name,
					music_item->pnext_callback);
				pdivoom_local_music_ctrl_tina_handle->music_item.music_status =
					DIVOOM_LOCAL_MUSIC_TYPE_PLAY;
				break;
			case DIVOOM_LOCAL_MUSIC_TYPE_SET_VOLUME:
				divoom_local_music_ctrl_tone_set_volume(
					&pdivoom_local_music_ctrl_tina_handle->music_item,
					music_item->volume);
				break;
			case DIVOOM_LOCAL_MUSIC_TYPE_STOP:
			case DIVOOM_LOCAL_MUSIC_TYPE_CUT:
				divoom_local_music_ctrl_tone_request_stop(
					&pdivoom_local_music_ctrl_tina_handle->music_item);
				pdivoom_local_music_ctrl_tina_handle->music_stop_done_flag = 1;
				break;
			case DIVOOM_LOCAL_MUSIC_TYPE_PAUSE:
				divoom_tone_lite_set_paused(
					pdivoom_local_music_ctrl_tina_handle->music_item.playhandle, 1);
				pdivoom_local_music_ctrl_tina_handle->music_item.music_status =
					DIVOOM_LOCAL_MUSIC_TYPE_PAUSE;
#if DIVOOM_SHOW_MUSIC_FFT
				divoom_spectrum_set_local(0);
#endif
				break;
			case DIVOOM_LOCAL_MUSIC_TYPE_RESUME:
				divoom_tone_lite_set_paused(
					pdivoom_local_music_ctrl_tina_handle->music_item.playhandle, 0);
				pdivoom_local_music_ctrl_tina_handle->music_item.music_status =
					DIVOOM_LOCAL_MUSIC_TYPE_PLAY;
#if DIVOOM_SHOW_MUSIC_FFT
				divoom_spectrum_set_local(1);
#endif
				break;
			default:
				break;
			}
			if (music_item->music_name)
				divoom_system_free(music_item->music_name);
			divoom_system_free(music_item);
		}
	}
#elif 0 /* F101 暂无 tplayer.h，TF 本地播放入口待 rtplayer 适配后启用 */
    while(1)
    {
        divoom_system_get_sem(pdivoom_local_music_ctrl_tina_handle->music_item.music_manage_sem);

		while(divoom_queue_items(pdivoom_local_music_ctrl_tina_handle->music_item.play_queue))
		{
			music_item = divoom_queue_pop(pdivoom_local_music_ctrl_tina_handle->music_item.play_queue);
			if(music_item == NULL)
			{
				continue;
			}
			
			if(music_item)
			{
                divoom_print("music_item->type: %d!\n", music_item->type);
				switch(music_item->type)
				{
					case DIVOOM_LOCAL_MUSIC_TYPE_PLAY:
						pdivoom_local_music_ctrl_tina_handle->music_item.play_time = music_item->play_time;
   	 					pdivoom_local_music_ctrl_tina_handle->music_item.end_time = music_item->play_time  + divoom_system_get_ticks();
    					divoom_print("music play time: %d, %d\n", pdivoom_local_music_ctrl_tina_handle->music_item.end_time,  divoom_system_get_ticks());
						divoom_local_music_ctrl_music_play_internal(&pdivoom_local_music_ctrl_tina_handle->music_item, music_item->volume, music_item->music_name, music_item->pnext_callback);
						pdivoom_local_music_ctrl_tina_handle->music_item.music_status = DIVOOM_LOCAL_MUSIC_TYPE_PLAY;
						break;

					case DIVOOM_LOCAL_MUSIC_TYPE_SET_VOLUME:
						if(pdivoom_local_music_ctrl_tina_handle->music_item.playhandle)
    					{
#ifdef DIVOOM_TIME_FRAME_SPEAKER
							TPlayerSetVolume(pdivoom_local_music_ctrl_tina_handle->music_item.playhandle, DIVOOM_TPLAYER_SET_DEFAULT_VOL);
#else
							TPlayerSetVolume(pdivoom_local_music_ctrl_tina_handle->music_item.playhandle, music_item->volume*DIVOOM_TPLAYER_SET_DEFAULT_VOL/100);
#endif
						}
						break;

					case DIVOOM_LOCAL_MUSIC_TYPE_STOP:
						if(pdivoom_local_music_ctrl_tina_handle->music_item.playhandle)
    					{
                            divoom_print("stop music!\n");
        					/* 只 Destroy，内部会停播并释放，避免先 Stop 再 Destroy 导致底层对已停 player 再发 Stop/Reset 报错 */
        					TPlayerDestroy(pdivoom_local_music_ctrl_tina_handle->music_item.playhandle);
        					pdivoom_local_music_ctrl_tina_handle->music_item.playhandle = NULL;
        					/* 主动停止不回调 pcallback，避免“下一首”入队后在切到 LineIn 后仍被播导致抢声卡 */
						}
						/* 清空队列中未处理的播放请求，避免 STOP 之后残留的 PLAY 再起播 */
						while(divoom_queue_items(pdivoom_local_music_ctrl_tina_handle->music_item.play_queue))
						{
							Pdivoom_local_music_ctrl_tina_data_item tmp = divoom_queue_pop(pdivoom_local_music_ctrl_tina_handle->music_item.play_queue);
							if(tmp)
							{
								if(tmp->music_name) divoom_system_free(tmp->music_name);
								divoom_system_free(tmp);
							}
						}
						/* 通知 stop_music_and_wait：TPlayerDestroy 与底层 dlclose/destroy_Resample 已完成，可安全起 LineIn */
						pdivoom_local_music_ctrl_tina_handle->music_stop_done_flag = 1;
						pdivoom_local_music_ctrl_tina_handle->music_item.music_status = DIVOOM_LOCAL_MUSIC_TYPE_STOP;
						break;

					case DIVOOM_LOCAL_MUSIC_TYPE_PAUSE:
						if(pdivoom_local_music_ctrl_tina_handle->music_item.playhandle)
    					{
                            divoom_print("pause music!\n");
        					TPlayerPause(pdivoom_local_music_ctrl_tina_handle->music_item.playhandle);
							pdivoom_local_music_ctrl_tina_handle->music_item.music_status = DIVOOM_LOCAL_MUSIC_TYPE_PAUSE;
						}
						break;

					case DIVOOM_LOCAL_MUSIC_TYPE_RESUME:
						if(pdivoom_local_music_ctrl_tina_handle->music_item.playhandle)
    					{
                            divoom_print("resume music!\n");
							TPlayerStart(pdivoom_local_music_ctrl_tina_handle->music_item.playhandle);
							pdivoom_local_music_ctrl_tina_handle->music_item.music_status = DIVOOM_LOCAL_MUSIC_TYPE_PLAY;
						}
						break;

					default:
						break;
				}

				if(music_item->music_name)
				{
					divoom_system_free(music_item->music_name);
				}
				divoom_system_free(music_item);
			}
		}
    }
#endif
    
    return NULL;
}

void *divoom_local_music_ctrl_play_tone_main_loop(void *param)
{
	Pdivoom_local_music_ctrl_tina_data_item music_item;

	(void)param;
	while (1)
	{
		divoom_system_get_sem(pdivoom_local_music_ctrl_tina_handle->tone_item.music_manage_sem);

		while (divoom_queue_items(pdivoom_local_music_ctrl_tina_handle->tone_item.play_queue))
		{
			music_item = divoom_queue_pop(pdivoom_local_music_ctrl_tina_handle->tone_item.play_queue);
			if (music_item == NULL)
			{
				continue;
			}

			divoom_print("tone_item->type: %d!\n", music_item->type);
			switch (music_item->type)
			{
			case DIVOOM_LOCAL_MUSIC_TYPE_CUT:
				divoom_local_music_ctrl_tone_apply_cut();
				break;

			case DIVOOM_LOCAL_MUSIC_TYPE_PLAY:
				pdivoom_local_music_ctrl_tina_handle->tone_item.play_time = music_item->play_time;
				pdivoom_local_music_ctrl_tina_handle->tone_item.end_time =
					music_item->play_time + divoom_system_get_ticks();
				strcpy(pdivoom_local_music_ctrl_tina_handle->tone_item_name, music_item->music_name);
				divoom_print("play time: %d, %d, vol:%d\n",
					     pdivoom_local_music_ctrl_tina_handle->tone_item.end_time,
					     divoom_system_get_ticks(), music_item->volume);
				divoom_local_music_ctrl_tone_play_internal(
					&pdivoom_local_music_ctrl_tina_handle->tone_item,
					music_item->volume, music_item->music_name,
					music_item->pnext_callback);
				break;

			case DIVOOM_LOCAL_MUSIC_TYPE_SET_VOLUME:
				if (pdivoom_local_music_ctrl_tina_handle->tone_item.playhandle)
				{
					/* 响度走 audio_sink 数字域；amp_tip_hold 仅门控，不调 JL 分档 */
					divoom_local_music_ctrl_tone_set_volume(
						&pdivoom_local_music_ctrl_tina_handle->tone_item,
						music_item->volume);
				}
				break;

			case DIVOOM_LOCAL_MUSIC_TYPE_STOP:
				/*
				 * 睡眠里切提示音：CUT_HOLD/keep duck 之后队列里还常有 STOP+PLAY。
				 * 若这里 session_end，会把本地 TF MUSIC mute 松开一截，听感破一下。
				 */
				if (divoom_tone_service_queue_has_pending_play())
				{
					Pdivoom_local_music_ctrl_tina_music_item th =
						&pdivoom_local_music_ctrl_tina_handle->tone_item;

					divoom_print("stop tone: keep media_duck (pending play)\n");
					if (th->playhandle && divoom_local_music_ctrl_tone_is_playing(th))
						divoom_local_music_ctrl_tone_request_stop(th);
					break;
				}
				divoom_local_music_ctrl_mute_jl_after_tone_if_needed();
				if (pdivoom_local_music_ctrl_tina_handle->tone_item.playhandle)
				{
					Pdivoom_local_music_ctrl_tina_music_item th =
						&pdivoom_local_music_ctrl_tina_handle->tone_item;
					int live = divoom_local_music_ctrl_tone_is_playing(th)
						   || divoom_tone_lite_sink_is_open(th->playhandle);

					if (live)
					{
						divoom_print("stop tone!\n");
						divoom_local_music_ctrl_tone_request_stop(th);
						divoom_local_music_ctrl_tone_close_sink_if_open(th);
						divoom_tone_service_session_end(
							th, DIVOOM_TONE_END_STOP);
					}
					else
					{
						/* 已结束仍可能卡着 media_duck tip hold：补 session_end */
						divoom_print("stop tone: idle handle, force session_end\n");
						divoom_tone_service_session_end(
							th, DIVOOM_TONE_END_STOP);
						divoom_local_music_ctrl_tone_destroy_handle(th);
						pdivoom_local_music_ctrl_tina_handle->tone_item_name[0] = 0;
					}
				}
				break;

			default:
				break;
			}

			if (music_item->music_name)
			{
				divoom_system_free(music_item->music_name);
			}
			divoom_system_free(music_item);
		}
	}

	return NULL;
}

void divoom_local_music_ctrl_music_init(void)
{
    pdivoom_local_music_ctrl_tina_handle = divoom_system_malloc(sizeof(divoom_local_music_ctrl_tina_ctrl));

    pdivoom_local_music_ctrl_tina_handle->current_music_volume = DIVOOM_LOCAL_MUSIC_DEFAULT_VOL;
    pdivoom_local_music_ctrl_tina_handle->tone_pause_music_pending_resume = 0;
    pdivoom_local_music_ctrl_tina_handle->tone_paused_mode = DIVOOM_TONE_PAUSED_MODE_NONE;
    pdivoom_local_music_ctrl_tina_handle->tone_saved_sys_vol_valid = 0;
    pdivoom_local_music_ctrl_tina_handle->tone_cut_pending = 0;
    //初始化 music_item 后面要和tone_item一致
    pdivoom_local_music_ctrl_tina_handle->music_item.lock = divoom_system_create_lock();
    pdivoom_local_music_ctrl_tina_handle->music_item.music_manage_sem = divoom_system_create_sem(1, 0);
    pdivoom_local_music_ctrl_tina_handle->music_item.playhandle = NULL; 
	pdivoom_local_music_ctrl_tina_handle->music_item.play_queue = divoom_queue_init(40, __func__, __LINE__);
    pdivoom_local_music_ctrl_tina_handle->music_stop_done_flag = 0;
#if DIVOOM_SHOW_TF_NAND
    divoom_system_create_task(divoom_local_music_ctrl_play_music_main_loop, "local_music",
			      pdivoom_local_music_ctrl_tina_handle, DIVOOM_TASK_STACK_LOCAL_MUSIC);
#else
    //divoom_system_create_task(divoom_local_music_ctrl_play_music_main_loop, "local_music", pdivoom_local_music_ctrl_tina_handle, DIVOOM_TASK_STACK_LOCAL_MUSIC);
#endif

    //初始化 tone_item
    pdivoom_local_music_ctrl_tina_handle->tone_item.lock = divoom_system_create_lock();
    pdivoom_local_music_ctrl_tina_handle->tone_item.music_manage_sem = divoom_system_create_sem(1, 0);
    pdivoom_local_music_ctrl_tina_handle->tone_item.playhandle = NULL; 
	pdivoom_local_music_ctrl_tina_handle->tone_item.play_queue = divoom_queue_init(40, __func__, __LINE__);
    divoom_system_create_task(divoom_local_music_ctrl_play_tone_main_loop, "local_tone", pdivoom_local_music_ctrl_tina_handle, DIVOOM_TASK_STACK_LOCAL_TONE);
    //gst_init(NULL, NULL);
}

static void divoom_local_music_ctrl_tone_lite_notify(void *user, int event)
{
	Pdivoom_local_music_ctrl_tina_music_item play_handle =
		(Pdivoom_local_music_ctrl_tina_music_item)user;
	int ok = (event == DIVOOM_TONE_LITE_NOTIFY_COMPLETE) ? 1 : 0;

	if (play_handle == NULL)
		return;

	if (ok && play_handle->play_time &&
	    play_handle->end_time > divoom_system_get_ticks())
	{
		/* 定时循环未到期：适配层已按 end_ticks 结束，按完成处理 */
	}

	/* 仅通知上层；会话收尾（resume/清名）在 play_run 返回后 finish */
	if (play_handle->pcallback)
	{
		play_handle->pcallback(play_handle->next_str, ok);
		play_handle->pcallback = NULL;
	}
}

/* 提示音：divoom_mp3dec+AAC → 48k Speex → I2S；响度走数字通道，JL 仅门控 */
static void divoom_local_music_ctrl_tone_play_internal(
	Pdivoom_local_music_ctrl_tina_music_item play_handle, int volume, char *music_name,
	divoom_local_music_ctrl_music_play_get_next *pnext_callback)
{
	unsigned int end_ticks = 0;
	int loop = 0;

	divoom_print("play tone: %s: %d\n", music_name, volume);
	divoom_local_music_ctrl_tone_prepare_play_session();
	if (!divoom_local_music_ctrl_tone_src_usable(music_name))
	{
		divoom_print("tone play fail: bad file %s\n", music_name);
		divoom_local_music_ctrl_tone_fail_callback(pnext_callback, music_name);
		divoom_local_music_ctrl_resume_active_mode_after_tone();
		return;
	}
	if (!divoom_tone_lite_path_supported(music_name))
	{
		divoom_print("tone unsupported format: %s\n", music_name);
		divoom_local_music_ctrl_tone_fail_callback(pnext_callback, music_name);
		divoom_local_music_ctrl_resume_active_mode_after_tone();
		return;
	}
	if (divoom_local_music_ctrl_tone_ensure_handle(play_handle) != 0)
	{
		divoom_print("tone backend create fail\n");
		divoom_local_music_ctrl_tone_fail_callback(pnext_callback, music_name);
		divoom_local_music_ctrl_resume_active_mode_after_tone();
		return;
	}

	if (volume < 0)
		volume = 0;
	if (volume > 100)
		volume = 100;

	if (divoom_tone_service_before_play() != 0)
	{
		divoom_print("tone play fail: before_play %s\n", music_name);
		divoom_local_music_ctrl_tone_fail_callback(pnext_callback, music_name);
		divoom_local_music_ctrl_resume_active_mode_after_tone();
		return;
	}
	divoom_tone_lite_set_play_mode(play_handle->playhandle,
				       divoom_tone_service_get_play_mode());
	divoom_local_music_ctrl_tone_set_volume(play_handle, volume);

	if (play_handle->play_time)
	{
		loop = 1;
		end_ticks = (unsigned int)play_handle->end_time;
	}

	if (divoom_local_music_ctrl_tone_play_until(play_handle, music_name, loop, end_ticks) != 0)
	{
		divoom_print("tone play_until fail\n");
		divoom_tone_service_session_end(play_handle, DIVOOM_TONE_END_EOF);
		divoom_local_music_ctrl_tone_destroy_handle(play_handle);
		divoom_local_music_ctrl_tone_fail_callback(pnext_callback, music_name);
		return;
	}

#ifdef DIVOOM_TIME_FRAME_SPEAKER
#if !defined(DIVOOM_TIMES_VIBE)
	divoom_system_delay_ms(DIVOOM_TONE_I2S_RAMP_MS);
	divoom_main_set_sys_vol(volume);
#endif
	/* TimesVibe：JL 门控在 open_sink 冲静音后 tip_hold，勿在开流前先开功放 */
#endif
	play_handle->pcallback = pnext_callback;
	/* Step5a：play_run 期间 poll 控制队列；切歌/停播/调音量均由 local_tone 线程处理 */
	{
		unsigned int run_t0 = divoom_system_get_ticks();

		divoom_tone_lite_set_control_hook(play_handle->playhandle,
						  divoom_local_music_ctrl_tone_poll_control,
						  NULL);
		divoom_local_music_ctrl_tone_play_run(play_handle);
		divoom_tone_lite_set_control_hook(play_handle->playhandle, NULL, NULL);
		divoom_print("tone play_run done: %s ms=%u tick=%u\n",
			     music_name, divoom_system_get_ticks() - run_t0,
			     divoom_system_get_ticks());
		/* stop_req 切歌不调 notify；关机提示音被打断时仍须断电 */
		if (play_handle->pcallback && divoom_onoff_is_poweroff_active())
		{
			play_handle->pcallback(play_handle->next_str, 0);
			play_handle->pcallback = NULL;
		}
		/* B1：PCM 已关，在此按 CUT/STOP/EOF 收尾（勿再经 notify→stop_tone） */
		divoom_local_music_ctrl_tone_finish_after_run(play_handle);
	}
}

#if 0 /* F101 暂无 tplayer.h：TF 音乐 TPlayer 入口待后续适配 */

//* a callback for tplayer.
static int divoom_local_music_play_CallbackForTPlayer(void* pUserData, int msg, int param0, void* param1)
{
    divoom_local_music_ctrl_tina_music_item* play_handle = (divoom_local_music_ctrl_tina_music_item*)pUserData;

    switch(msg)
    {
        case TPLAYER_NOTIFY_PREPARED:
        {
            break;
        }
        case TPLAYER_NOTIFY_PLAYBACK_PRE_COMPLETE:
            if(play_handle->play_time)
            {
                divoom_print("TPLAYER_NOTIFY_PLAYBACK_PRE_COMPLETE: %d, %d\n", play_handle->end_time, divoom_system_get_ticks());
                if(play_handle->end_time > divoom_system_get_ticks())
                {
                }
                else
                {
                    if(play_handle->pcallback)
                    {
                        play_handle->pcallback(play_handle->next_str, 1);
                        play_handle->pcallback = NULL;
                    }
                	divoom_local_music_ctrl_stop_tone();
                }
            }
            //PowerManagerReleaseWakeLock("tplayerdemo");
            break;
            
        case TPLAYER_NOTIFY_PLAYBACK_COMPLETE:
        {
            divoom_print("TPLAYER_NOTIFY_PLAYBACK_COMPLETE: %d, %d\n", play_handle->end_time, divoom_system_get_ticks());
            if(play_handle->end_time > divoom_system_get_ticks())
            {
            }
            else
            {
                if(play_handle->pcallback)
                {
                    play_handle->pcallback(play_handle->next_str, 1);
                    play_handle->pcallback = NULL;
                }
            	divoom_local_music_ctrl_stop_tone();
            }
            //PowerManagerReleaseWakeLock("tplayerdemo");
            break;
        }

        case TPLAYER_NOTIFY_SEEK_COMPLETE:
        {
            divoom_print("TPLAYER_NOTIFY_SEEK_COMPLETE>>>>info: seek ok.\n");
            break;
        }

        case TPLAYER_NOTIFY_MEDIA_ERROR:
        {
            switch (param0)
            {
                case TPLAYER_MEDIA_ERROR_UNKNOWN:
                {
                    divoom_print("erro type:TPLAYER_MEDIA_ERROR_UNKNOWN\n");
                    break;
                }
                case TPLAYER_MEDIA_ERROR_UNSUPPORTED:
                {
                    divoom_print("erro type:TPLAYER_MEDIA_ERROR_UNSUPPORTED\n");
                    break;
                }
                case TPLAYER_MEDIA_ERROR_IO:
                {
                    divoom_print("erro type:TPLAYER_MEDIA_ERROR_IO\n");
                    break;
                }
            }
            if(play_handle->pcallback)
            {
                play_handle->pcallback(play_handle->next_str, 0);
                play_handle->pcallback = NULL;
            }
            //出错的话，也能出播放流程
            divoom_local_music_ctrl_stop_tone();
            divoom_print("error: open media source fail.\n");
            break;
        }

        case TPLAYER_NOTIFY_NOT_SEEKABLE:
        {
            divoom_print("info: media source is unseekable.\n");
            break;
        }

        case TPLAYER_NOTIFY_BUFFER_START:
        {
            divoom_print("have no enough data to play\n");
            break;
        }

        case TPLAYER_NOTIFY_BUFFER_END:
        {
            divoom_print("have enough data to play again\n");
            break;
        }

        case TPLAYER_NOTIFY_VIDEO_FRAME:
        {
            //divoom_print("get the decoded video frame\n");
            break;
        }

        case TPLAYER_NOTIFY_AUDIO_FRAME:
        {
            //divoom_print("get the decoded audio frame\n");
            break;
        }

        case TPLAYER_NOTIFY_SUBTITLE_FRAME:
        {
            //divoom_print("get the decoded subtitle frame\n");
            break;
        }
        case TPLAYER_NOTYFY_DECODED_VIDEO_SIZE:
            break;

        default:
        {
            divoom_print("warning: unknown callback from Tinaplayer.\n");
            break;
        }
    }
    return 0;
}

//* a callback for music player.
static int divoom_local_music_play_CallbackForMusicPlayer(void* pUserData, int msg, int param0, void* param1)
{
    divoom_local_music_ctrl_tina_music_item* play_handle = (divoom_local_music_ctrl_tina_music_item*)pUserData;

    switch(msg)
    {
        case TPLAYER_NOTIFY_PREPARED:
        {
            break;
        }
        case TPLAYER_NOTIFY_PLAYBACK_PRE_COMPLETE:
            divoom_print("MUSIC TPLAYER_NOTIFY_PLAYBACK_PRE_COMPLETE: %d, %d\n", play_handle->end_time, divoom_system_get_ticks());
            if(play_handle->play_time)
            {
                if(play_handle->end_time > divoom_system_get_ticks())
                {
                }
                else
                {
                    divoom_local_music_ctrl_stop_music();
                }
            }
            break;
            
        case TPLAYER_NOTIFY_PLAYBACK_COMPLETE:
        {
            divoom_print("MUSIC TPLAYER_NOTIFY_PLAYBACK_COMPLETE: %d, %d\n", play_handle->end_time, divoom_system_get_ticks());
            if(play_handle->play_time && play_handle->end_time > divoom_system_get_ticks())
            {
                /* 还没到时间，继续播放 */
            }
            else
            {
                /* 当前曲目播完：若有下一首则播下一首，否则停止 */
                if(play_handle->pcallback && play_handle->pcallback(play_handle->next_str, 1))
                {
                    Pdivoom_local_music_ctrl_tina_data_item music_item;
                    music_item = (Pdivoom_local_music_ctrl_tina_data_item)divoom_system_malloc(sizeof(divoom_local_music_ctrl_tina_data_item));
                    if(music_item)
                    {
                        music_item->type = DIVOOM_LOCAL_MUSIC_TYPE_PLAY;
                        music_item->music_name = divoom_app_com_copy_string(play_handle->next_str, 0);
						divoom_print("wft test music_name: %s\n", music_item->music_name);
                        music_item->volume = pdivoom_local_music_ctrl_tina_handle->current_music_volume;
                        music_item->pnext_callback = play_handle->pcallback;
                        music_item->play_time = 0;
                        divoom_queue_push(pdivoom_local_music_ctrl_tina_handle->music_item.play_queue, music_item);
                        divoom_system_put_sem(pdivoom_local_music_ctrl_tina_handle->music_item.music_manage_sem);
                    }
                    else
                    {
                        divoom_local_music_ctrl_stop_music();
                    }
                }
                else
                {
                    divoom_local_music_ctrl_stop_music();
                }
            }
            break;
        }

        case TPLAYER_NOTIFY_SEEK_COMPLETE:
        {
            divoom_print("MUSIC TPLAYER_NOTIFY_SEEK_COMPLETE>>>>info: seek ok.\n");
            break;
        }

        case TPLAYER_NOTIFY_MEDIA_ERROR:
        {
            switch (param0)
            {
                case TPLAYER_MEDIA_ERROR_UNKNOWN:
                {
                    divoom_print("music erro type:TPLAYER_MEDIA_ERROR_UNKNOWN\n");
                    break;
                }
                case TPLAYER_MEDIA_ERROR_UNSUPPORTED:
                {
                    divoom_print("music erro type:TPLAYER_MEDIA_ERROR_UNSUPPORTED\n");
                    break;
                }
                case TPLAYER_MEDIA_ERROR_IO:
                {
                    divoom_print("music erro type:TPLAYER_MEDIA_ERROR_IO\n");
                    break;
                }
            }
            //出错的话，也能出播放流程
            divoom_local_music_ctrl_stop_music();
            divoom_print("music error: open media source fail.\n");
            break;
        }

        case TPLAYER_NOTIFY_NOT_SEEKABLE:
        {
            divoom_print("music info: media source is unseekable.\n");
            break;
        }

        case TPLAYER_NOTIFY_BUFFER_START:
        {
            divoom_print("music have no enough data to play\n");
            break;
        }

        case TPLAYER_NOTIFY_BUFFER_END:
        {
            divoom_print("music have enough data to play again\n");
            break;
        }

        case TPLAYER_NOTIFY_VIDEO_FRAME:
        case TPLAYER_NOTIFY_SUBTITLE_FRAME:
        case TPLAYER_NOTYFY_DECODED_VIDEO_SIZE:
            break;

		case TPLAYER_NOTIFY_AUDIO_FRAME:
			break;

        default:
        {
            divoom_print("music warning: unknown callback from Tinaplayer.\n");
            break;
        }
    }
    return 0;
}

/*********************************************************************
  Function:        divoom_local_music_ctrl_music_play_internal
  Description:     内部函数，指定设备播放某个音频文件（用于music_item）
  Input:           play_handle
                   music_name
                   volume
                   pnext_callback ,播放完了触发回调通知上面,如果返回0，将结束播放，否则继续启动播放
  Return:          none
**********************************************************************/

void divoom_local_music_ctrl_music_play_internal(Pdivoom_local_music_ctrl_tina_music_item play_handle,int volume, char *music_name, divoom_local_music_ctrl_music_play_get_next *pnext_callback)
{
    if(play_handle->playhandle == NULL)
    {
        play_handle->playhandle = TPlayerCreate(AUDIO_PLAYER);
        TPlayerSetNotifyCallback(play_handle->playhandle,divoom_local_music_play_CallbackForMusicPlayer, (void*)play_handle);
    }
    else
    {
        TPlayerReset(play_handle->playhandle);
    }
    
    if (TPlayerSetDataSource(play_handle->playhandle,music_name,NULL)!= 0)
	{
	    divoom_print("Music TPlayerSetDataSource return fail.\n");
		if(pnext_callback && pnext_callback(play_handle->next_str, 0))
        {
            Pdivoom_local_music_ctrl_tina_data_item music_item;
            music_item = (Pdivoom_local_music_ctrl_tina_data_item)divoom_system_malloc(sizeof(divoom_local_music_ctrl_tina_data_item));
            if(music_item)
            {
                music_item->type = DIVOOM_LOCAL_MUSIC_TYPE_PLAY;
                music_item->music_name = divoom_app_com_copy_string(play_handle->next_str, 0);
                music_item->volume = volume;
                music_item->pnext_callback = play_handle->pcallback;
                music_item->play_time = 0;
                divoom_queue_push(pdivoom_local_music_ctrl_tina_handle->music_item.play_queue, music_item);
                divoom_system_put_sem(pdivoom_local_music_ctrl_tina_handle->music_item.music_manage_sem);
            }
        }
	    return ;
	}
	else
	{
	    divoom_print("Music setDataSource end\n");
	}
	if(TPlayerPrepare(play_handle->playhandle)!= 0)
	{
	    divoom_print("Music TPlayerPrepare return fail.\n");
	    return ;
	}
	else
	{
	    divoom_print("Music TPlayerPrepare end\n");
	}
    
	if(TPlayerStart(play_handle->playhandle) != 0)
	{
	    divoom_print("Music TPlayerStart() return fail.\n");
	    return ;
	}
	else
	{
	    divoom_print("Music started.\n");
	}
#ifdef DIVOOM_TIME_FRAME_SPEAKER
    TPlayerSetVolume(play_handle->playhandle, DIVOOM_TPLAYER_SET_DEFAULT_VOL);
#else
    TPlayerSetVolume(play_handle->playhandle, volume*DIVOOM_TPLAYER_SET_DEFAULT_VOL/100);
#endif
    if(play_handle->play_time)
    {
        TPlayerSetLooping(play_handle->playhandle, 1);
    }
    else
    {
        TPlayerSetLooping(play_handle->playhandle, 0);
    }
    play_handle->pcallback = pnext_callback;
}

/*********************************************************************
  Function:        divoom_local_music_ctrl_play
  Description:     指定设备播放某个音频文件
  Input:           play_handle
                   music_name
                   volume
                   pnext_callback ,播放完了触发回调通知上面,如果返回0，将结束播放，否则继续启动播放
  Return:          none
**********************************************************************/

void divoom_local_music_ctrl_play(Pdivoom_local_music_ctrl_tina_music_item play_handle,int volume, char *music_name, divoom_local_music_ctrl_music_play_get_next *pnext_callback)
{
    if(play_handle->playhandle == NULL)
    {
        play_handle->playhandle = TPlayerCreate(AUDIO_PLAYER);
        TPlayerSetNotifyCallback(play_handle->playhandle,divoom_local_music_play_CallbackForTPlayer, (void*)play_handle);
    }
    else
    {
        TPlayerReset(play_handle->playhandle);
    }
    
    if(TPlayerSetDataSource(play_handle->playhandle,music_name,NULL)!= 0)
	{
	    divoom_print("TPlayerSetDataSource return fail.\n");
	    return ;
	}else{
	    divoom_print("setDataSource end\n");
	}
	if(TPlayerPrepare(play_handle->playhandle)!= 0)
	{
	    divoom_print("TPlayerPrepare return fail.\n");
	    return ;
	}else{
	    divoom_print("TPlayerPrepare end\n");
	}
    
	if(TPlayerStart(play_handle->playhandle) != 0)
	{
	    divoom_print("TPlayerStart() return fail.\n");
	    return ;
	}else{
	    divoom_print("started.\n");
	}
#ifdef DIVOOM_TIME_FRAME_SPEAKER
    TPlayerSetVolume(play_handle->playhandle, DIVOOM_TPLAYER_SET_DEFAULT_VOL);
#else
    TPlayerSetVolume(play_handle->playhandle, volume*DIVOOM_TPLAYER_SET_DEFAULT_VOL/100);
#endif
    if(play_handle->play_time)
    {
        TPlayerSetLooping(play_handle->playhandle, 1);
    }
    else
    {
        TPlayerSetLooping(play_handle->playhandle, 0);
    }
    play_handle->pcallback = pnext_callback;
#ifdef DIVOOM_TIME_FRAME_SPEAKER
    /* 方案A：提示音不直接改全局 softvol，避免影响正在播放的 TF 音乐主音量 */
#endif
}

/* 提示音内部播放：仅按提示音参数设置本播放器增益，不改全局 softvol */
static void divoom_local_music_ctrl_tone_play_internal(Pdivoom_local_music_ctrl_tina_music_item play_handle,int volume, char *music_name, divoom_local_music_ctrl_music_play_get_next *pnext_callback)
{
    divoom_print("play music: %s: %d\n", music_name, volume);
    if(play_handle->playhandle == NULL)
    {
        play_handle->playhandle = TPlayerCreate(AUDIO_PLAYER);
        TPlayerSetNotifyCallback(play_handle->playhandle,divoom_local_music_play_CallbackForTPlayer, (void*)play_handle);
    }
    else
    {
        TPlayerReset(play_handle->playhandle);
    }

    if(TPlayerSetDataSource(play_handle->playhandle,music_name,NULL)!= 0)
	{
	    divoom_print("tone TPlayerSetDataSource return fail.\n");
	    return ;
	}else{
	    divoom_print("tone setDataSource end\n");
	}
	if(TPlayerPrepare(play_handle->playhandle)!= 0)
	{
	    divoom_print("tone TPlayerPrepare return fail.\n");
	    return ;
	}else{
	    divoom_print("tone TPlayerPrepare end\n");
	}

    if(volume < 0)
        volume = 0;
    if(volume > 100)
        volume = 100;

#ifdef DIVOOM_TIME_FRAME_SPEAKER
    /* Prepare 后先关 TPlayer 数字音量，避免 I2S 首帧满幅冲击 */
    TPlayerSetVolume(play_handle->playhandle, 0);
#if !defined(DIVOOM_TIMES_VIBE)
    divoom_main_set_sys_vol(volume);
#endif
#endif

	if(TPlayerStart(play_handle->playhandle) != 0)
	{
	    divoom_print("tone TPlayerStart() return fail.\n");
	    return ;
	}else{
	    divoom_print("tone started.\n");
	}

#if defined(DIVOOM_TIME_FRAME_SPEAKER) && !defined(DIVOOM_TIMES_VIBE)
    divoom_system_delay_ms(DIVOOM_TONE_I2S_RAMP_MS);
    /* 数字音量满幅，实际响度由 softvol 控制 */
    TPlayerSetVolume(play_handle->playhandle, DIVOOM_TPLAYER_SET_DEFAULT_VOL);
#else
    /* 响度走数字域，JL 的 I2S 档位固定不动 */
    TPlayerSetVolume(play_handle->playhandle, volume);
#endif
    if(play_handle->play_time)
    {
        TPlayerSetLooping(play_handle->playhandle, 1);
    }
    else
    {
        TPlayerSetLooping(play_handle->playhandle, 0);
    }
    play_handle->pcallback = pnext_callback;
}

#endif /* F101 tplayer 播放入口 */

/*********************************************************************
  Function:        divoom_local_music_ctrl_get_music_status
  Description:     获取当前播放状态
  Input:           
  Return:          1：播放；0：暂停
**********************************************************************/

int divoom_local_music_ctrl_get_music_status(void)
{
	if(pdivoom_local_music_ctrl_tina_handle->music_item.playhandle == NULL)
	{
		return 0;
	}

	if(pdivoom_local_music_ctrl_tina_handle->music_item.music_status == DIVOOM_LOCAL_MUSIC_TYPE_PLAY)
	{
		return 1;
	}
	else
	{
		return 0;
	}

    return 0;
}

/*********************************************************************
  Function:        divoom_local_music_ctrl_get_stone_status
  Description:     获取当前提示音播放状态
  Input:           
  Return:          1：播放；0：暂停
**********************************************************************/

int divoom_local_music_ctrl_get_tone_status(void)
{

	if (!pdivoom_local_music_ctrl_tina_handle)
	{
		return 0;
	}
	if (pdivoom_local_music_ctrl_tina_handle->tone_item.playhandle
	    && divoom_local_music_ctrl_tone_is_playing(
		    &pdivoom_local_music_ctrl_tina_handle->tone_item))
	{
		return 1;
	}
	return divoom_tone_service_queue_has_pending_play();
}

int divoom_local_music_ctrl_is_tone_playing_path(const char *music_name)
{
	if (pdivoom_local_music_ctrl_tina_handle == NULL || music_name == NULL)
	{
		return 0;
	}
	if (pdivoom_local_music_ctrl_tina_handle->tone_item.playhandle == NULL)
	{
		return 0;
	}
	if (!divoom_local_music_ctrl_tone_is_playing(
		    &pdivoom_local_music_ctrl_tina_handle->tone_item))
	{
		return 0;
	}
	return strcmp(pdivoom_local_music_ctrl_tina_handle->tone_item_name, music_name) == 0;
}

int divoom_local_music_ctrl_tone_idle_for_resume(void)
{

	if (!pdivoom_local_music_ctrl_tina_handle)
	{
		return 1;
	}
	if (divoom_local_music_ctrl_get_tone_status())
	{
		return 0;
	}
	return !divoom_tone_service_queue_has_pending_play();
}

/*********************************************************************
  Function:        divoom_local_music_ctrl_play
  Description:     指定设备播放某个音频文件
  Input:           music_name
                   pnext_callback ,播放完了触发回调通知上面,如果返回0，将结束播放，否则继续启动播放
  Return:          none
**********************************************************************/

void divoom_local_music_ctrl_music_play(char *music_name, uint8 vol, divoom_local_music_ctrl_music_play_get_next *pnext_callback)
{
	Pdivoom_local_music_ctrl_tina_data_item music_item;

	music_item = (Pdivoom_local_music_ctrl_tina_data_item)divoom_system_malloc(sizeof(divoom_local_music_ctrl_tina_data_item));
	if(music_item)
	{
		music_item->type = DIVOOM_LOCAL_MUSIC_TYPE_PLAY;
		music_item->music_name = divoom_app_com_copy_string(music_name, 0);
		divoom_print("wft test music_name: %s, vol: %d\n", music_item->music_name, vol);
		music_item->volume = vol;
		music_item->pnext_callback = pnext_callback;
		music_item->play_time = 0;
		divoom_queue_push(pdivoom_local_music_ctrl_tina_handle->music_item.play_queue, music_item);
		divoom_system_put_sem(pdivoom_local_music_ctrl_tina_handle->music_item.music_manage_sem);
	}
}

/*********************************************************************
  Function:        divoom_local_music_ctrl_set_music_volume
  Description:     设置提示音播放音量
  Input:           volume
  Return:          none
**********************************************************************/

void divoom_local_music_ctrl_set_music_volume(int volume)
{
	Pdivoom_local_music_ctrl_tina_data_item music_item;

    if(volume < 0)
    {
        volume = 0;
    }
    else if(volume > 100)
    {
        volume = 100;    
    }

    if (pdivoom_local_music_ctrl_tina_handle)
        pdivoom_local_music_ctrl_tina_handle->current_music_volume = volume;
#ifdef DIVOOM_TIME_FRAME_SPEAKER
	divoom_main_set_sys_vol(volume);
#else
	music_item = (Pdivoom_local_music_ctrl_tina_data_item)divoom_system_malloc(sizeof(divoom_local_music_ctrl_tina_data_item));
	if(music_item)
	{
		music_item->type = DIVOOM_LOCAL_MUSIC_TYPE_SET_VOLUME;
		music_item->volume = volume;
		music_item->music_name = NULL;
		divoom_queue_push(pdivoom_local_music_ctrl_tina_handle->music_item.play_queue, music_item); 
		divoom_system_put_sem(pdivoom_local_music_ctrl_tina_handle->music_item.music_manage_sem);
	}
#endif
}

/*********************************************************************
  Function:        divoom_local_music_ctrl_get_music_volume
  Description:     获取当前音乐音量（0-100）
  Input:           none
  Return:          当前音量，未初始化返回 0
**********************************************************************/
int divoom_local_music_ctrl_get_music_volume(void)
{
    if (!pdivoom_local_music_ctrl_tina_handle)
        return 0;
    return pdivoom_local_music_ctrl_tina_handle->current_music_volume;
}

int divoom_local_music_ctrl_get_music_position_sec(void)
{
    if (!pdivoom_local_music_ctrl_tina_handle
        || !pdivoom_local_music_ctrl_tina_handle->music_item.playhandle)
    {
        return 0;
    }
#if DIVOOM_SHOW_TF_NAND
    return divoom_tone_lite_get_position_sec(
	    pdivoom_local_music_ctrl_tina_handle->music_item.playhandle);
#else
    return 0;
#endif
}

int divoom_local_music_ctrl_get_music_duration_sec(void)
{
    if (!pdivoom_local_music_ctrl_tina_handle
        || !pdivoom_local_music_ctrl_tina_handle->music_item.playhandle)
    {
        return 0;
    }
#if DIVOOM_SHOW_TF_NAND
    return divoom_tone_lite_get_duration_sec(
	    pdivoom_local_music_ctrl_tina_handle->music_item.playhandle);
#else
    return 0;
#endif
}

int divoom_local_music_ctrl_is_music_active(void)
{
    if (!pdivoom_local_music_ctrl_tina_handle
        || !pdivoom_local_music_ctrl_tina_handle->music_item.playhandle)
    {
        return 0;
    }
    return divoom_local_music_ctrl_get_music_status() ? 1 : 0;
}

/*********************************************************************
  Function:        divoom_local_music_ctrl_stop_music
  Description:     停止播放音乐
  Input:           none
  Return:          none
**********************************************************************/


#if DIVOOM_SHOW_MODE
void divoom_local_music_ctrl_show_release_for_format(void)
{
	s_show_drop_play = 1;
	divoom_local_music_ctrl_stop_music();
	divoom_local_music_ctrl_stop_music_and_wait(3000);
}
#endif

void divoom_local_music_ctrl_stop_music(void)
{
	Pdivoom_local_music_ctrl_tina_data_item music_item;
	
    if(pdivoom_local_music_ctrl_tina_handle == NULL)
    {
        return;
    }
    pdivoom_local_music_ctrl_tina_handle->music_stop_done_flag = 0;
	
    music_item = (Pdivoom_local_music_ctrl_tina_data_item)divoom_system_malloc(sizeof(divoom_local_music_ctrl_tina_data_item));
	if(music_item)
	{
		music_item->type = DIVOOM_LOCAL_MUSIC_TYPE_STOP;
		music_item->music_name = NULL;
		divoom_queue_push(pdivoom_local_music_ctrl_tina_handle->music_item.play_queue, music_item);	

		divoom_system_put_sem(pdivoom_local_music_ctrl_tina_handle->music_item.music_manage_sem);
	}
}

/* 在已调用 divoom_local_music_ctrl_stop_music() 之后调用，阻塞直到 STOP 处理完（TPlayerDestroy 及底层释放完成）或超时。
 * 用于切到 LineIn/UAC 前确保 TF 播放器完全退出，避免 dlclose/destroy_Resample 在 LineIn 启动后才执行导致 LineIn 被拖垮。 */
void divoom_local_music_ctrl_stop_music_and_wait(unsigned int timeout_ms)
{
	unsigned int i, step_ms = 25;
	if (pdivoom_local_music_ctrl_tina_handle == NULL) return;
	for (i = 0; i < timeout_ms; i += step_ms) 
	{
		divoom_system_delay_ms(step_ms);
		if (pdivoom_local_music_ctrl_tina_handle->music_stop_done_flag) 
		{
			pdivoom_local_music_ctrl_tina_handle->music_stop_done_flag = 0;
			return;
		}
	}
}
/*********************************************************************
  Function:        divoom_local_music_ctrl_music_ctrl
  Description:     控制播放
  Input:           play     1：播放；0：暂停
                   none
  Return:          none
**********************************************************************/



void divoom_local_music_ctrl_music_ctrl(uint8 play)
{
	Pdivoom_local_music_ctrl_tina_data_item music_item;
	
    if(pdivoom_local_music_ctrl_tina_handle == NULL)
    {
        return;
    }
#if DIVOOM_SHOW_TF_NAND && DIVOOM_SHOW_MUSIC_FFT
	/* 按键暂停立刻放行 0x3b，不必等 play_run 排到队列。 */
	divoom_spectrum_set_local(play ? 1 : 0);
#endif
	
    music_item = (Pdivoom_local_music_ctrl_tina_data_item)divoom_system_malloc(sizeof(divoom_local_music_ctrl_tina_data_item));
	if(music_item)
	{
		music_item->type = play ? DIVOOM_LOCAL_MUSIC_TYPE_RESUME : DIVOOM_LOCAL_MUSIC_TYPE_PAUSE;
		music_item->music_name = NULL;
		divoom_queue_push(pdivoom_local_music_ctrl_tina_handle->music_item.play_queue, music_item);	

		divoom_system_put_sem(pdivoom_local_music_ctrl_tina_handle->music_item.music_manage_sem);
	}
}


/*********************************************************************
  Function:        divoom_local_music_ctrl_tone_play
  Description:     播放某个提示音文件，如果有音乐在播放，将音乐的音量降低一半，再播放提示音
  Input:           music_name
                   pnext_callback
                   play_time        播放多长时间结束
  Return:          none
**********************************************************************/

int divoom_local_music_ctrl_tone_play(char *music_name, int volume, divoom_local_music_ctrl_music_play_get_next *pnext_callback, int play_time)
{
	Pdivoom_local_music_ctrl_tina_data_item music_item;
	Pdivoom_local_music_ctrl_tina_data_item cut_item;
	int need_cut;

    if (pdivoom_local_music_ctrl_tina_handle == NULL)
    {
        return 0;
    }
	if (divoom_local_music_ctrl_reject_tone_during_poweroff(music_name))
	{
		return 0;
	}
	if (divoom_local_music_ctrl_skip_tone_for_poweron_video(music_name))
    {
        return 0;
    }
	{
		divoom_audio_policy_action_t action = divoom_audio_session_on_request_tone();

		if (action == DIVOOM_AUDIO_POLICY_DENY)
		{
			divoom_print("tone suppressed: policy deny %s\n", music_name);
			return 0;
		}
	}
    if (!divoom_local_music_ctrl_tone_src_usable(music_name))
    {
        return 0;
    }
    if (!divoom_tone_lite_path_supported(music_name))
    {
        divoom_print("tone unsupported format: %s\n", music_name);
        return 0;
    }
    if(strcmp(pdivoom_local_music_ctrl_tina_handle->tone_item_name, music_name) == 0)
    {
        divoom_local_music_ctrl_set_tone_volume(volume);
        return 1;
    }

    need_cut = divoom_local_music_ctrl_tone_need_cut_for_play();
    divoom_print("tone play req: %s vol=%d cut=%d tick=%u\n",
                 music_name, volume, need_cut, divoom_system_get_ticks());

    divoom_local_music_ctrl_tone_flush_pending_plays();
    /* idle 新播：丢掉积压 STOP，否则 STOP 会排在 PLAY 前把会话打穿 */
    if (!need_cut)
    {
	    divoom_local_music_ctrl_tone_drop_pending_stops();
    }

    if (need_cut)
    {
	cut_item = (Pdivoom_local_music_ctrl_tina_data_item)divoom_system_malloc(
		sizeof(divoom_local_music_ctrl_tina_data_item));
	if (!cut_item)
	{
		return 0;
	}
	cut_item->type = DIVOOM_LOCAL_MUSIC_TYPE_CUT;
	cut_item->music_name = NULL;
	cut_item->volume = 0;
	cut_item->pnext_callback = NULL;
	cut_item->play_time = 0;
	if (!divoom_local_music_ctrl_tone_enqueue(cut_item))
	{
		return 0;
	}
    }

	music_item = (Pdivoom_local_music_ctrl_tina_data_item)divoom_system_malloc(sizeof(divoom_local_music_ctrl_tina_data_item));
	if(music_item)
	{
		music_item->type = DIVOOM_LOCAL_MUSIC_TYPE_PLAY;
		music_item->music_name = divoom_app_com_copy_string(music_name, 0);
		music_item->volume = volume;
		music_item->pnext_callback = pnext_callback;
		music_item->play_time = play_time;
		return divoom_local_music_ctrl_tone_enqueue(music_item);
	}
	return 0;
}

/*********************************************************************
  Function:        divoom_local_music_ctrl_set_tone_volume
  Description:     设置提示音播放音量
  Input:           volume
  Return:          none
**********************************************************************/

void divoom_local_music_ctrl_set_tone_volume(int volume)
{
	Pdivoom_local_music_ctrl_tina_data_item music_item;

    if(volume < 0)
    {
        volume = 0;
    }
    else if(volume > 100)
    {
        volume = 100;    
    }

    if (!pdivoom_local_music_ctrl_tina_handle)
    {
        return;
    }

    music_item = (Pdivoom_local_music_ctrl_tina_data_item)divoom_system_malloc(sizeof(divoom_local_music_ctrl_tina_data_item));
	if(music_item)
	{
		music_item->type = DIVOOM_LOCAL_MUSIC_TYPE_SET_VOLUME;
		music_item->volume = volume;
		music_item->music_name = NULL;
		music_item->pnext_callback = NULL;
		music_item->play_time = 0;
		(void)divoom_local_music_ctrl_tone_enqueue(music_item);
	}
#ifdef DIVOOM_TIME_FRAME_SPEAKER
    /* TimesVibe：SET_VOLUME → tone_lite ch_set_volume；amp_tip_hold 仅会话门控 */
#if defined(DIVOOM_TIMES_VIBE)
	(void)0;
#else
    divoom_main_set_sys_vol(volume);
#endif
#endif
}

/*********************************************************************
  Function:        divoom_local_music_ctrl_stop_tone
  Description:     停止播放提示音
  Input:           none
  Return:          none
**********************************************************************/

void divoom_local_music_ctrl_stop_tone(void)
{
	Pdivoom_local_music_ctrl_tina_data_item music_item;

    if(pdivoom_local_music_ctrl_tina_handle == NULL)
    {
        return;
    }
	if (divoom_onoff_is_poweroff_active())
	{
		divoom_print("stop_tone suppressed: poweroff active\n");
		return;
	}

	/* Step5a：调用方只入队 STOP；request_stop / mute / session_end 由 local_tone 处理 */
	divoom_local_music_ctrl_tone_flush_pending_plays();

    music_item = (Pdivoom_local_music_ctrl_tina_data_item)divoom_system_malloc(sizeof(divoom_local_music_ctrl_tina_data_item));
	if(music_item)
	{
		music_item->type = DIVOOM_LOCAL_MUSIC_TYPE_STOP;
		music_item->music_name = NULL;
		music_item->volume = 0;
		music_item->pnext_callback = NULL;
		music_item->play_time = 0;
		if (!divoom_local_music_ctrl_tone_enqueue(music_item))
		{
			pdivoom_local_music_ctrl_tina_handle->tone_item_name[0] = 0;
			divoom_local_music_ctrl_resume_active_mode_after_tone();
		}
	}
	else
	{
		pdivoom_local_music_ctrl_tina_handle->tone_item_name[0] = 0;
		divoom_local_music_ctrl_resume_active_mode_after_tone();
	}
}

static divoom_mic_record_done_fn s_voice_clip_user_done;

static void divoom_local_music_ctrl_voice_clip_record_done_bridge(const char *path,
								  uint32 duration_sec,
								  int result)
{
	divoom_mic_record_done_fn cb = s_voice_clip_user_done;

	divoom_audio_policy_set_voice_clip_recording(0);
	s_voice_clip_user_done = NULL;
	if (cb)
		cb(path, duration_sec, result);
}

int divoom_local_music_ctrl_voice_clip_record_start(const char *path, uint32 duration_sec,
						    divoom_mic_record_done_fn done_cb)
{
	int ret;

	if (!path || !path[0])
		return -1;

	divoom_local_music_ctrl_voice_clip_record_stop_sync();
	/* 先 duck JL 总开关，再 stop_tone，避免 tip release 与 record hold 之间的漏音窗口 */
	divoom_audio_policy_set_voice_clip_recording(1);
	divoom_local_music_ctrl_stop_tone();

//	divoom_local_music_ctrl_stop_music();


	s_voice_clip_user_done = done_cb;
	ret = divoom_mic_record_start(path, duration_sec,
				      done_cb ? divoom_local_music_ctrl_voice_clip_record_done_bridge : NULL);
	if (ret != 0) {
		divoom_audio_policy_set_voice_clip_recording(0);
		s_voice_clip_user_done = NULL;
	}
	return ret;
}

void divoom_local_music_ctrl_voice_clip_record_stop(void)
{
	if (!divoom_mic_record_is_active())
		return;
	divoom_mic_record_stop();
}

void divoom_local_music_ctrl_voice_clip_record_stop_sync(void)
{
	if (!divoom_mic_record_is_active() && !divoom_audio_policy_is_voice_clip_recording())
		return;
	divoom_mic_record_stop_sync();
	divoom_audio_policy_set_voice_clip_recording(0);
	s_voice_clip_user_done = NULL;
}

int divoom_local_music_ctrl_voice_clip_record_is_active(void)
{
	return divoom_mic_record_is_active() ? 1 : 0;
}

static void divoom_local_music_ctrl_tone_count_queue_item(void *context, void *data, int pos)
{
    int *counts = context;
    Pdivoom_local_music_ctrl_tina_data_item item = data;

    (void)pos;
    if(item == NULL)
    {
        return;
    }
    switch(item->type)
    {
    case DIVOOM_LOCAL_MUSIC_TYPE_PLAY:
        counts[0]++;
        break;
    case DIVOOM_LOCAL_MUSIC_TYPE_SET_VOLUME:
        counts[1]++;
        break;
    case DIVOOM_LOCAL_MUSIC_TYPE_STOP:
        counts[2]++;
        break;
    case DIVOOM_LOCAL_MUSIC_TYPE_CUT:
        counts[3]++;
        break;
    default:
        break;
    }
}

void divoom_local_music_ctrl_tone_diag_print(void)
{
	Pdivoom_local_music_ctrl_tina_music_item play_handle;
	void *queue;
	int n;
	int counts[4] = {0};
	const char *path = "";
	const char *mode_str = "none";
	int playing = 0;
	int sink_open = 0;
	int play_mode = DIVOOM_TONE_LITE_MODE_COLD;

	if (!pdivoom_local_music_ctrl_tina_handle)
	{
		printf("  tone: (not init)\n");
		return;
	}
	play_handle = &pdivoom_local_music_ctrl_tina_handle->tone_item;
	queue = play_handle->play_queue;
    n = divoom_queue_get_all_items(queue, counts,
        divoom_local_music_ctrl_tone_count_queue_item);
	if (pdivoom_local_music_ctrl_tina_handle->tone_item_name[0] != '\0')
	{
		path = pdivoom_local_music_ctrl_tina_handle->tone_item_name;
	}
	if (play_handle->playhandle)
	{
		playing = divoom_local_music_ctrl_tone_is_playing(play_handle);
		sink_open = divoom_tone_lite_sink_is_open(play_handle->playhandle);
		play_mode = divoom_tone_lite_get_play_mode(play_handle->playhandle);
	}
	if (play_mode == DIVOOM_TONE_LITE_MODE_OVERLAY)
	{
		mode_str = "OVERLAY";
	}
	else if (play_handle->playhandle)
	{
		mode_str = "COLD";
	}
	printf("  tone: playing=%d status=%d path=%s cut_pending=%d\n",
	       playing, divoom_local_music_ctrl_get_tone_status(), path,
	       pdivoom_local_music_ctrl_tina_handle->tone_cut_pending);
	printf("  tone_q: total=%d play=%d vol=%d stop=%d cut=%d mode=%s sink_open=%d\n",
	       n, counts[0], counts[1], counts[2], counts[3], mode_str, sink_open);
}

Pdivoom_local_music_ctrl_tina_ctrl divoom_local_music_ctrl_tone_priv_get_ctrl(void)
{
	return pdivoom_local_music_ctrl_tina_handle;
}

void divoom_local_music_ctrl_tone_priv_close_sink_if_open(
	Pdivoom_local_music_ctrl_tina_music_item play_handle)
{
	if (!play_handle || !play_handle->playhandle)
	{
		return;
	}
	divoom_tone_lite_close_sink_if_open(play_handle->playhandle);
}

int divoom_local_music_ctrl_tone_priv_stop_requested(Pdivoom_local_music_ctrl_tina_music_item play_handle)
{
	return divoom_local_music_ctrl_tone_stop_requested(play_handle);
}

void divoom_local_music_ctrl_tone_priv_mute_jl_if_needed(void)
{
	divoom_local_music_ctrl_mute_jl_after_tone_if_needed();
}

void divoom_local_music_ctrl_tone_priv_resume_after_tone(void)
{
	divoom_local_music_ctrl_resume_active_mode_after_tone();
}


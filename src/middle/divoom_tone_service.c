#include "divoom_build_config.h"

#include "divoom_tone_service.h"
#include "divoom_tone_lite_decode.h"
#include "divoom_audio_session.h"
#include "divoom_audio_policy.h"
#include "divoom_audio_sink.h"
#include "divoom_sleep_pcm_session.h"
#include "divoom_print.h"
#include "divoom_queue.h"
#include "divoom_system.h"

#include <stdio.h>

#if defined(DIVOOM_TIMES_VIBE)
#include "divoom_local_movie_play.h"
#include "divoom_audio_policy.h"
#include "divoom_tone_lite_decode.h"
#endif

static const char *divoom_tone_service_end_reason_name(divoom_tone_end_reason_t reason)
{
	switch (reason)
	{
	case DIVOOM_TONE_END_EOF:
		return "EOF";
	case DIVOOM_TONE_END_STOP:
		return "STOP";
	case DIVOOM_TONE_END_CUT_HOLD:
		return "CUT_HOLD";
	case DIVOOM_TONE_END_CUT_DONE:
		return "CUT_DONE";
	default:
		return "?";
	}
}

static void divoom_tone_service_find_pending_play(void *result, void *data, int pos)
{
    Pdivoom_local_music_ctrl_tina_data_item item = data;
    int *pending = result;

    (void)pos;
    if(item && item->type == DIVOOM_LOCAL_MUSIC_TYPE_PLAY)
    {
        *pending = 1;
    }
}

int divoom_tone_service_queue_has_pending_play(void)
{
    Pdivoom_local_music_ctrl_tina_ctrl ctrl;
    int pending = 0;

    ctrl = divoom_local_music_ctrl_tone_priv_get_ctrl();
    if (!ctrl)
    {
        return 0;
    }
    divoom_queue_get_all_items(ctrl->tone_item.play_queue, &pending,
        divoom_tone_service_find_pending_play);
    return pending;
}

/*
 * 租约须在 tone_lite open_sink（audio_sink_start 之后）获取，勿在 before_play 提前 acquire：
 * 冷启动时 pool 尚未 init，会导致首次提示音被静默 suppress。
 */
int divoom_tone_service_before_play(void)
{
	if (!divoom_sleep_pcm_session_is_running()
	    && (divoom_sleep_pcm_session_get_mode() != DIVOOM_SLEEP_PCM_SESSION_NONE
		|| divoom_audio_policy_is_sleep_playback_active())
	    && divoom_audio_session_get_state() == DIVOOM_AUDIO_SESSION_STATE_IDLE)
	{
		divoom_audio_session_release_sleep_bus("tone_cold");
	}
	divoom_audio_session_on_tone_play_begin();
	return 0;
}

divoom_tone_lite_play_mode_t divoom_tone_service_get_play_mode(void)
{
	if (divoom_audio_session_is_overlay() && divoom_audio_sink_is_running())
	{
		return DIVOOM_TONE_LITE_MODE_OVERLAY;
	}
	return DIVOOM_TONE_LITE_MODE_COLD;
}

void divoom_tone_service_mark_cut_pending(void)
{
	Pdivoom_local_music_ctrl_tina_ctrl ctrl;

	ctrl = divoom_local_music_ctrl_tone_priv_get_ctrl();
	if (ctrl)
	{
		ctrl->tone_cut_pending = 1;
	}
}

int divoom_tone_service_should_keep_sink(int stop_req, int eof,
					 divoom_tone_lite_play_mode_t mode)
{
	Pdivoom_local_music_ctrl_tina_ctrl ctrl;
	int cut;

	ctrl = divoom_local_music_ctrl_tone_priv_get_ctrl();
	cut = (ctrl && ctrl->tone_cut_pending) ? 1 : 0;
	if (mode == DIVOOM_TONE_LITE_MODE_OVERLAY)
	{
		return (stop_req && cut) ? 1 : 0;
	}
	return ((stop_req && cut) || (!stop_req && eof)) ? 1 : 0;
}

void divoom_tone_service_release_tone_bus(Pdivoom_local_music_ctrl_tina_music_item play_handle)
{
	if (play_handle && play_handle->playhandle)
	{
		divoom_tone_lite_close_sink_if_open(play_handle->playhandle);
	}
	divoom_audio_sink_ch_disable(DIVOOM_AUDIO_SINK_CH_TONE);
	divoom_audio_sink_force_release_bus(DIVOOM_AUDIO_SINK_CH_TONE);
}

void divoom_tone_service_session_end(Pdivoom_local_music_ctrl_tina_music_item play_handle,
				     divoom_tone_end_reason_t reason)
{
	Pdivoom_local_music_ctrl_tina_ctrl ctrl;
	int sleep_running;

	ctrl = divoom_local_music_ctrl_tone_priv_get_ctrl();
	if (!ctrl || !play_handle)
	{
		return;
	}
	if (reason == DIVOOM_TONE_END_CUT_HOLD)
	{
		ctrl->tone_cut_pending = 0;
		divoom_print("tone_session_end: %s state=%d tick=%u\n",
			     divoom_tone_service_end_reason_name(reason),
			     (int)divoom_audio_session_get_state(),
			     divoom_system_get_ticks());
		return;
	}

	sleep_running = divoom_sleep_pcm_session_is_running();
	ctrl->tone_cut_pending = 0;
	if (reason == DIVOOM_TONE_END_STOP || reason == DIVOOM_TONE_END_EOF)
	{
		ctrl->tone_item_name[0] = 0;
	}
	divoom_print("tone_session_end: %s sleep=%d state=%d tick=%u\n",
		     divoom_tone_service_end_reason_name(reason),
		     sleep_running, (int)divoom_audio_session_get_state(),
		     divoom_system_get_ticks());
#if defined(DIVOOM_TIMES_VIBE)
	if (!sleep_running)
	{
		divoom_local_music_ctrl_tone_priv_mute_jl_if_needed();
	}
#endif
	divoom_audio_session_on_tone_play_end();
	divoom_tone_service_release_tone_bus(play_handle);
	/* 始终 release tone 的 MEDIA_VOLUME hold；sleep 仍活跃时由 refcount 保持 0 */
	divoom_local_music_ctrl_tone_priv_resume_after_tone();
}

void divoom_tone_service_finish_after_run(Pdivoom_local_music_ctrl_tina_music_item play_handle)
{
	Pdivoom_local_music_ctrl_tina_ctrl ctrl;
	int stopped;
	int cut;
	divoom_tone_end_reason_t reason;

	ctrl = divoom_local_music_ctrl_tone_priv_get_ctrl();
	if (!ctrl || !play_handle)
	{
		return;
	}

	stopped = divoom_local_music_ctrl_tone_priv_stop_requested(play_handle);
	cut = ctrl->tone_cut_pending;
	ctrl->tone_cut_pending = 0;

	if (cut)
	{
		reason = divoom_tone_service_queue_has_pending_play() ?
			 DIVOOM_TONE_END_CUT_HOLD : DIVOOM_TONE_END_CUT_DONE;
		divoom_tone_service_session_end(play_handle, reason);
		return;
	}
	if (stopped)
	{
		/*
		 * 旧逻辑 defer 到队列 STOP；但新 PLAY(cut=0) 会 drop_pending_stops，
		 * session_end/media_duck_release 永远不跑 → tip hold 卡死、音乐不恢复。
		 * 有后续 PLAY 则保持 duck；否则立即收尾。
		 */
		if (divoom_tone_service_queue_has_pending_play())
		{
			divoom_print("tone finish: STOP keep duck (pending play) tick=%u\n",
				     divoom_system_get_ticks());
			return;
		}
		divoom_print("tone finish: STOP session_end now tick=%u\n",
			     divoom_system_get_ticks());
		divoom_tone_service_session_end(play_handle, DIVOOM_TONE_END_STOP);
		return;
	}

	divoom_tone_service_session_end(play_handle, DIVOOM_TONE_END_EOF);
}

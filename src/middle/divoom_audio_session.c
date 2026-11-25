#include "divoom_build_config.h"

#include "divoom_audio_session.h"
#include "divoom_audio_policy.h"
#include "divoom_audio_sink.h"
#include "divoom_audio_session.h"
#include "divoom_audio_buf_pool.h"
#include "divoom_sleep_pcm_session.h"
#include "divoom_local_music_ctrl.h"
#include "divoom_mix_pcm_ctrl.h"
#include "divoom_print.h"
#include "divoom_system.h"

#include <stdio.h>

#if DIVOOM_BUILD_RTOS_DEVICE

#include <string.h>

#define DIVOOM_AUDIO_SESSION_SLEEP_STOP_WAIT_MS  3000u

static divoom_audio_session_state_t s_state = DIVOOM_AUDIO_SESSION_STATE_IDLE;
static divoom_audio_session_owner_t s_owner = DIVOOM_AUDIO_SESSION_OWNER_NONE;
static divoom_audio_session_sleep_stop_async_fn s_sleep_stop_async_fn;
static divoom_audio_session_sleep_stop_wait_fn s_sleep_stop_wait_fn;

static const char *divoom_audio_session_state_name(divoom_audio_session_state_t st)
{
	switch (st)
	{
	case DIVOOM_AUDIO_SESSION_STATE_IDLE:
		return "IDLE";
	case DIVOOM_AUDIO_SESSION_STATE_SLEEP:
		return "SLEEP";
	case DIVOOM_AUDIO_SESSION_STATE_TONE:
		return "TONE";
	case DIVOOM_AUDIO_SESSION_STATE_OVERLAY:
		return "OVERLAY";
	case DIVOOM_AUDIO_SESSION_STATE_HANDOFF:
		return "HANDOFF";
	default:
		return "?";
	}
}

static void divoom_audio_session_sync_owner(void)
{
	divoom_audio_session_owner_t owner = DIVOOM_AUDIO_SESSION_OWNER_NONE;

	switch (s_state)
	{
	case DIVOOM_AUDIO_SESSION_STATE_SLEEP:
	case DIVOOM_AUDIO_SESSION_STATE_OVERLAY:
		owner = DIVOOM_AUDIO_SESSION_OWNER_SLEEP;
		break;
	case DIVOOM_AUDIO_SESSION_STATE_TONE:
		owner = DIVOOM_AUDIO_SESSION_OWNER_TONE;
		break;
	case DIVOOM_AUDIO_SESSION_STATE_HANDOFF:
		owner = DIVOOM_AUDIO_SESSION_OWNER_HANDOFF;
		break;
	default:
		owner = DIVOOM_AUDIO_SESSION_OWNER_NONE;
		break;
	}
	if (s_owner != owner)
	{
		divoom_print("audio_session: owner %d -> %d (state=%s)\n",
			     (int)s_owner, (int)owner, divoom_audio_session_state_name(s_state));
	}
	s_owner = owner;
}

static void divoom_audio_session_set_state(divoom_audio_session_state_t st)
{
	if (s_state != st)
	{
		divoom_print("audio_session: state %s -> %s\n",
			     divoom_audio_session_state_name(s_state),
			     divoom_audio_session_state_name(st));
	}
	s_state = st;
	divoom_audio_session_sync_owner();
}

divoom_audio_session_owner_t divoom_audio_session_get_owner(void)
{
	return s_owner;
}

divoom_audio_session_state_t divoom_audio_session_get_state(void)
{
	return s_state;
}

int divoom_audio_session_is_overlay(void)
{
	return s_state == DIVOOM_AUDIO_SESSION_STATE_OVERLAY ? 1 : 0;
}

int divoom_audio_session_wait_tone_idle(unsigned int timeout_ms)
{
	unsigned int t0;

	if (timeout_ms < 100u)
	{
		timeout_ms = 100u;
	}
	if (timeout_ms > 3000u)
	{
		timeout_ms = 3000u;
	}
	divoom_local_music_ctrl_stop_tone();
	t0 = divoom_system_get_tickms();
	while (divoom_system_get_tickms() - t0 < timeout_ms)
	{
		if (!divoom_local_music_ctrl_get_tone_status()
		    && divoom_local_music_ctrl_tone_idle_for_resume())
		{
			if (!divoom_audio_sink_is_running()
			    || !divoom_audio_sink_ch_active(DIVOOM_AUDIO_SINK_CH_TONE))
			{
				return 0;
			}
		}
		divoom_system_delay_ms(10);
	}
	divoom_print("audio_session: wait_tone_idle timeout ms=%u\n", timeout_ms);
	return -1;
}

void divoom_audio_session_release_tone_bus(const char *tag)
{
	if (!divoom_audio_buf_pool_is_leased(DIVOOM_AUDIO_BUS_TONE))
	{
		return;
	}
	divoom_audio_sink_ch_disable(DIVOOM_AUDIO_SINK_CH_TONE);
	divoom_audio_sink_force_release_bus(DIVOOM_AUDIO_SINK_CH_TONE);
	divoom_print("audio_session: release TONE bus tag=%s\n", tag ? tag : "?");
}

void divoom_audio_session_release_sleep_bus(const char *tag)
{
	if (!divoom_audio_buf_pool_is_leased(DIVOOM_AUDIO_BUS_SLEEP))
	{
		return;
	}
	divoom_audio_sink_ch_disable(DIVOOM_AUDIO_SINK_CH_SLEEP);
	divoom_audio_sink_force_release_bus(DIVOOM_AUDIO_SINK_CH_SLEEP);
	divoom_print("audio_session: release SLEEP bus tag=%s\n", tag ? tag : "?");
}

int divoom_audio_session_prepare_sleep_from_tone(unsigned int timeout_ms)
{
	divoom_audio_session_set_state(DIVOOM_AUDIO_SESSION_STATE_HANDOFF);
	(void)divoom_audio_session_wait_tone_idle(timeout_ms);
	divoom_audio_session_release_tone_bus("sleep_prep");
	divoom_audio_session_set_state(DIVOOM_AUDIO_SESSION_STATE_SLEEP);
	return 0;
}

int divoom_audio_session_preempt_sleep_for_tone(void)
{
	if (s_state != DIVOOM_AUDIO_SESSION_STATE_SLEEP
	    && s_state != DIVOOM_AUDIO_SESSION_STATE_OVERLAY
	    && divoom_sleep_pcm_session_get_mode() == DIVOOM_SLEEP_PCM_SESSION_NONE
	    && !divoom_audio_policy_is_sleep_playback_active())
	{
		return 0;
	}
	divoom_print("audio_session: preempt sleep for tone\n");
	if (divoom_audio_session_sleep_stop_wait(DIVOOM_AUDIO_SESSION_SLEEP_STOP_WAIT_MS) != 0)
	{
		divoom_print("audio_session: preempt wait async stop timeout\n");
	}
	{
		divoom_audio_session_state_t prev = s_state;

		divoom_audio_session_set_state(DIVOOM_AUDIO_SESSION_STATE_HANDOFF);
		if (prev == DIVOOM_AUDIO_SESSION_STATE_OVERLAY)
		{
			divoom_audio_policy_on_tone_end();
			divoom_audio_sink_ch_disable(DIVOOM_AUDIO_SINK_CH_TONE);
			divoom_audio_sink_force_release_bus(DIVOOM_AUDIO_SINK_CH_TONE);
		}
	}
	divoom_sleep_pcm_session_stop();
	divoom_audio_session_release_sleep_bus("preempt");
	divoom_audio_session_set_state(DIVOOM_AUDIO_SESSION_STATE_IDLE);
	return 0;
}

divoom_audio_policy_action_t divoom_audio_session_on_request_tone(void)
{
	divoom_audio_policy_tone_ctx_t ctx;
	divoom_audio_policy_action_t action;

	memset(&ctx, 0, sizeof(ctx));
	action = divoom_audio_policy_evaluate_tone(s_state, &ctx);
	if (action == DIVOOM_AUDIO_POLICY_PREEMPT)
	{
		divoom_audio_session_preempt_sleep_for_tone();
		action = divoom_audio_policy_evaluate_tone(s_state, &ctx);
	}
	divoom_print("audio_session: request_tone state=%s action=%d\n",
		     divoom_audio_session_state_name(s_state), (int)action);
	return action;
}

void divoom_audio_session_on_tone_play_begin(void)
{
	switch (s_state)
	{
	case DIVOOM_AUDIO_SESSION_STATE_SLEEP:
		divoom_audio_session_set_state(DIVOOM_AUDIO_SESSION_STATE_OVERLAY);
		divoom_audio_policy_on_tone_begin(DIVOOM_AUDIO_POLICY_TONE_DUCK_SLEEP_PCT);
		break;
	case DIVOOM_AUDIO_SESSION_STATE_IDLE:
		divoom_audio_session_set_state(DIVOOM_AUDIO_SESSION_STATE_TONE);
		break;
	case DIVOOM_AUDIO_SESSION_STATE_OVERLAY:
	case DIVOOM_AUDIO_SESSION_STATE_TONE:
		break;
	default:
		break;
	}
}

void divoom_audio_session_on_tone_play_end(void)
{
	switch (s_state)
	{
	case DIVOOM_AUDIO_SESSION_STATE_OVERLAY:
		divoom_audio_policy_on_tone_end();
		divoom_audio_session_set_state(DIVOOM_AUDIO_SESSION_STATE_SLEEP);
		break;
	case DIVOOM_AUDIO_SESSION_STATE_TONE:
		divoom_audio_session_set_state(DIVOOM_AUDIO_SESSION_STATE_IDLE);
		break;
	default:
		break;
	}
}

void divoom_audio_session_on_sleep_pcm_start(void)
{
	if (s_state == DIVOOM_AUDIO_SESSION_STATE_IDLE
	    || s_state == DIVOOM_AUDIO_SESSION_STATE_HANDOFF)
	{
		divoom_audio_session_set_state(DIVOOM_AUDIO_SESSION_STATE_SLEEP);
	}
}

void divoom_audio_session_on_sleep_pcm_stop(void)
{
	if (s_state == DIVOOM_AUDIO_SESSION_STATE_OVERLAY)
	{
		divoom_audio_policy_on_tone_end();
		divoom_audio_sink_ch_disable(DIVOOM_AUDIO_SINK_CH_TONE);
		divoom_audio_sink_force_release_bus(DIVOOM_AUDIO_SINK_CH_TONE);
	}
	if (s_state != DIVOOM_AUDIO_SESSION_STATE_IDLE)
	{
		divoom_audio_session_set_state(DIVOOM_AUDIO_SESSION_STATE_IDLE);
	}
}

void divoom_audio_session_sleep_set_volume(int vol_percent)
{
	int mix_level;

	if (vol_percent < 0)
	{
		vol_percent = 0;
	}
	else if (vol_percent > 100)
	{
		vol_percent = 100;
	}
	mix_level = (vol_percent * 16) / 100;
	divoom_mix_pcm_set_vol(mix_level);
	divoom_audio_sink_ch_set_volume(DIVOOM_AUDIO_SINK_CH_SLEEP, (unsigned int)vol_percent);
}

void divoom_audio_session_set_sleep_stop_handlers(
	divoom_audio_session_sleep_stop_async_fn async_fn,
	divoom_audio_session_sleep_stop_wait_fn wait_fn)
{
	s_sleep_stop_async_fn = async_fn;
	s_sleep_stop_wait_fn = wait_fn;
}

void divoom_audio_session_sleep_stop_async(void)
{
	if (s_sleep_stop_async_fn)
	{
		s_sleep_stop_async_fn();
		return;
	}
	/* 助眠 ctrl 尚未注册时（极早路径）直接同步停，避免再挂独立线程 */
	divoom_sleep_pcm_session_stop();
}

int divoom_audio_session_sleep_stop_wait(unsigned int timeout_ms)
{
	if (s_sleep_stop_wait_fn)
	{
		return s_sleep_stop_wait_fn(timeout_ms);
	}
	return 0;
}

void divoom_audio_session_diag_print(void)
{
	printf("audio_diag:\n");
	printf("  fsm=%s owner=%d\n",
	       divoom_audio_session_state_name(s_state), (int)s_owner);
	printf("  lease: SLEEP=%d TONE=%d\n",
	       divoom_audio_buf_pool_is_leased(DIVOOM_AUDIO_BUS_SLEEP),
	       divoom_audio_buf_pool_is_leased(DIVOOM_AUDIO_BUS_TONE));
	printf("  pool: fail_acquire=%u\n", divoom_audio_buf_pool_lease_fail_count());
	divoom_audio_policy_diag_print();
	printf("  bus_force_release=%u\n", divoom_audio_sink_bus_force_release_count());
	divoom_local_music_ctrl_tone_diag_print();
	printf("  sink: running=%d sleep_act=%d tone_act=%d\n",
	       divoom_audio_sink_is_running(),
	       divoom_audio_sink_ch_active(DIVOOM_AUDIO_SINK_CH_SLEEP),
	       divoom_audio_sink_ch_active(DIVOOM_AUDIO_SINK_CH_TONE));
	printf("  sink_ring: sleep_bytes=%u tone_bytes=%u sleep_fill=%u%% tone_fill=%u%%\n",
	       divoom_audio_sink_ch_ring_bytes(DIVOOM_AUDIO_SINK_CH_SLEEP),
	       divoom_audio_sink_ch_ring_bytes(DIVOOM_AUDIO_SINK_CH_TONE),
	       divoom_audio_sink_ch_ring_fill_pct(DIVOOM_AUDIO_SINK_CH_SLEEP),
	       divoom_audio_sink_ch_ring_fill_pct(DIVOOM_AUDIO_SINK_CH_TONE));
	printf("  sink_stat: out_frames=%u silence_periods=%u underrun_sleep=%u underrun_tone=%u\n",
	       divoom_audio_sink_whitebox_out_frames(),
	       divoom_audio_sink_whitebox_silence_periods(),
	       divoom_audio_sink_whitebox_underruns(DIVOOM_AUDIO_SINK_CH_SLEEP),
	       divoom_audio_sink_whitebox_underruns(DIVOOM_AUDIO_SINK_CH_TONE));
	printf("  sleep_pcm: mode=%d running=%d policy_sleep=%d\n",
	       (int)divoom_sleep_pcm_session_get_mode(),
	       divoom_sleep_pcm_session_is_running(),
	       divoom_audio_policy_is_sleep_playback_active());
}

#else

divoom_audio_session_owner_t divoom_audio_session_get_owner(void)
{
	return DIVOOM_AUDIO_SESSION_OWNER_NONE;
}
divoom_audio_session_state_t divoom_audio_session_get_state(void)
{
	return DIVOOM_AUDIO_SESSION_STATE_IDLE;
}
int divoom_audio_session_is_overlay(void) { return 0; }
int divoom_audio_session_wait_tone_idle(unsigned int timeout_ms)
{
	(void)timeout_ms;
	return 0;
}
void divoom_audio_session_release_tone_bus(const char *tag) { (void)tag; }
void divoom_audio_session_release_sleep_bus(const char *tag) { (void)tag; }
int divoom_audio_session_prepare_sleep_from_tone(unsigned int timeout_ms)
{
	(void)timeout_ms;
	return 0;
}
int divoom_audio_session_preempt_sleep_for_tone(void) { return 0; }
divoom_audio_policy_action_t divoom_audio_session_on_request_tone(void)
{
	return DIVOOM_AUDIO_POLICY_ALLOW;
}
void divoom_audio_session_on_tone_play_begin(void) {}
void divoom_audio_session_on_tone_play_end(void) {}
void divoom_audio_session_on_sleep_pcm_start(void) {}
void divoom_audio_session_on_sleep_pcm_stop(void) {}
void divoom_audio_session_sleep_set_volume(int vol_percent) { (void)vol_percent; }
void divoom_audio_session_set_sleep_stop_handlers(
	divoom_audio_session_sleep_stop_async_fn async_fn,
	divoom_audio_session_sleep_stop_wait_fn wait_fn)
{
	(void)async_fn;
	(void)wait_fn;
}
void divoom_audio_session_sleep_stop_async(void) {}
int divoom_audio_session_sleep_stop_wait(unsigned int timeout_ms)
{
	(void)timeout_ms;
	return 0;
}
void divoom_audio_session_diag_print(void) {}

#endif

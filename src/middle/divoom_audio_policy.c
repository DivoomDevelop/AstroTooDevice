#include "divoom_build_config.h"
#include "divoom_audio_policy.h"
#include "divoom_audio_session.h"
#include "divoom_audio_sink.h"
#include "divoom_jl_amp_adapter.h"
#include "divoom_local_movie_play.h"
#include "divoom_print.h"
#include "divoom_system.h"
#include "divoom_type.h"
#include "divoom_app_com.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#if DIVOOM_BUILD_RTOS_DEVICE
#include <pthread.h>
/* Serializes only media duck state; sink callbacks read atomic hold bits. */
static pthread_mutex_t s_media_policy_lock = PTHREAD_MUTEX_INITIALIZER;
#define media_policy_lock() pthread_mutex_lock(&s_media_policy_lock)
#define media_policy_unlock() pthread_mutex_unlock(&s_media_policy_lock)
#define MEDIA_HOLD _Atomic uint8_t
#else
#define media_policy_lock() ((void)0)
#define media_policy_unlock() ((void)0)
#define MEDIA_HOLD uint8_t
#endif

int divoom_btcore_uart_cmd_set_iisin_eq(uint8_t level);
int divoom_mix_pcm_get_vol(void);

/* 未指定档位时的兜底电平 */
#define DIVOOM_AUDIO_POLICY_AMP_ON_LEVEL         100
#define DIVOOM_AUDIO_POLICY_AMP_SETTLE_MS        40

static uint8_t s_sleep_playback_active;
static uint8_t s_voice_clip_recording;
static uint8_t s_tone_duck_saved;
static uint8_t s_tone_duck_active;
#if DIVOOM_SHOW_TF_NAND
static uint8_t s_music_duck_saved;
static uint8_t s_music_duck_active;
#endif
static uint8_t s_amp_tip_held;
static uint8_t s_amp_sleep_held;
static uint8_t s_amp_gate_on;
/* s_amp_tip_level / s_amp_sleep_level：门控状态记录；JL UART 不再按此分档调响度 */
static uint8_t s_amp_tip_level = DIVOOM_AUDIO_POLICY_AMP_ON_LEVEL;
static uint8_t s_amp_sleep_level = DIVOOM_AUDIO_POLICY_AMP_ON_LEVEL;
static uint8_t s_amp_gate_level = 0;
/* 上次成功下发的 IISIN EQ 档；0=尚未发过，且协议约定不发 0 */
static uint8_t s_iisin_eq_last = 0;

/*
 * tip/sleep/record → MEDIA_VOLUME 音乐总开关；任意在则 ramp→0，全退 ramp→16。
 * 不改 tip/sleep 播法，也不动 MUSIC_VOLUME。
 */
static MEDIA_HOLD s_media_duck_tone_held;
static MEDIA_HOLD s_media_duck_sleep_held;
static MEDIA_HOLD s_media_duck_record_held;
#ifdef CONFIG_DIVOOM_MPP_NET_AUDIO
static MEDIA_HOLD s_media_duck_network_held = 0;
/* 电台 UI 音量 0–100，对应 IISIN EQ 用音量档 0–16，不另钉满档 */
static uint8_t s_network_vol_0_100 = 0;
int divoom_audio_policy_network_muted(void)
{
    return s_media_duck_tone_held || s_media_duck_sleep_held || s_media_duck_record_held;
}
#endif

static void divoom_audio_policy_media_duck_sync(const char *why)
{
	int want_mute = (s_media_duck_tone_held || s_media_duck_sleep_held
			 || s_media_duck_record_held) ? 1 : 0;
#ifdef CONFIG_DIVOOM_MPP_NET_AUDIO
    divoom_audio_sink_ch_set_duck(DIVOOM_AUDIO_SINK_CH_NETWORK, want_mute ? 0 : 100);
    want_mute = want_mute || s_media_duck_network_held;
#endif
	int target = want_mute ? 0 : DIVOOM_JL_MEDIA_VOL_MAX;

#if DIVOOM_SHOW_TF_NAND
	/*
	 * 展示机 TF 走本地 MUSIC sink。原先 MUSIC 在播就整段 return，
	 * 提示音既不压 JL 也不 mute 本机，两路叠在一起。
	 * 这里 mute/恢复 MUSIC；本地 TF 占用 I2S 时仍不 ramp JL MEDIA_VOLUME。
	 */
	if (want_mute) {
		if (!s_music_duck_active) {
			s_music_duck_saved = (uint8_t)divoom_audio_sink_ch_get_duck(
				DIVOOM_AUDIO_SINK_CH_MUSIC);
			if (s_music_duck_saved == 0)
				s_music_duck_saved = 100; /* 通道未起时 get_duck=0，恢复按满幅 */
			divoom_audio_sink_ch_set_duck(DIVOOM_AUDIO_SINK_CH_MUSIC, 0);
			s_music_duck_active = 1;
			divoom_print("media_duck: mute local MUSIC why=%s\n",
				     why ? why : "?");
		}
	} else if (s_music_duck_active) {
		divoom_audio_sink_ch_set_duck(DIVOOM_AUDIO_SINK_CH_MUSIC,
					      s_music_duck_saved);
		s_music_duck_active = 0;
		divoom_print("media_duck: restore local MUSIC %u why=%s\n",
			     (unsigned)s_music_duck_saved, why ? why : "?");
	}
	if (divoom_audio_sink_ch_active(DIVOOM_AUDIO_SINK_CH_MUSIC))
		return;
#endif
	divoom_print("media_duck: sync mute=%d tip=%u sleep=%u record=%u target=%d why=%s\n",
		     want_mute, (unsigned)s_media_duck_tone_held,
		     (unsigned)s_media_duck_sleep_held, (unsigned)s_media_duck_record_held,
		     target, why ? why : "?");
	divoom_jl_media_volume_ramp_to(target);
}

static int divoom_audio_policy_amp_keep_open(void)
{
	return divoom_local_movie_play_should_keep_jl_tone_volume() ? 1 : 0;
}

static void divoom_audio_policy_amp_soft_set(int level)
{
	if (level > 0)
	{
		divoom_jl_amp_adapter_prime_if_needed();
	}
}

static int divoom_audio_policy_amp_want_level(void)
{
	int level = 0;

	if (s_amp_tip_held && (int)s_amp_tip_level > level)
	{
		level = (int)s_amp_tip_level;
	}
	if (s_amp_sleep_held && (int)s_amp_sleep_level > level)
	{
		level = (int)s_amp_sleep_level;
	}
	return level;
}

static void divoom_audio_policy_amp_sync(void)
{
	int want_level;

	want_level = divoom_audio_policy_amp_want_level();
	if (s_amp_tip_held || s_amp_sleep_held)
	{
		if (!s_amp_gate_on || (int)s_amp_gate_level != want_level)
		{
			divoom_audio_policy_amp_soft_set(want_level);
			s_amp_gate_on = 1;
			s_amp_gate_level = (uint8_t)want_level;
			divoom_print("amp_gate: ON level=%d tip=%u/%u sleep=%u/%u\n",
				     want_level, (unsigned)s_amp_tip_held,
				     (unsigned)s_amp_tip_level, (unsigned)s_amp_sleep_held,
				     (unsigned)s_amp_sleep_level);
		}
		return;
	}
	if (s_amp_gate_on)
	{
		if (divoom_audio_policy_amp_keep_open())
		{
			divoom_print("amp_gate: skip OFF (poweron movie keep)\n");
			return;
		}
		divoom_audio_policy_amp_soft_set(0);
		s_amp_gate_on = 0;
		s_amp_gate_level = 0;
		divoom_print("amp_gate: OFF\n");
	}
}

static uint8_t divoom_audio_policy_iisin_eq_from_tip_100(int vol_0_100)
{
	int level;

	if (vol_0_100 <= 0)
	{
		return 0;
	}
	level = (vol_0_100 * 16 + 50) / 100;
	if (level > 16)
	{
		level = 16;
	}
	return (uint8_t)level;
}

static void divoom_audio_policy_iisin_eq_sync(void)
{
	int level = 0;
	int mix_vol = 0;

	if (s_amp_tip_held && s_amp_tip_level > 0)
	{
		level = (int)divoom_audio_policy_iisin_eq_from_tip_100((int)s_amp_tip_level);
	}
	else if (s_amp_sleep_held)
	{
		mix_vol = divoom_mix_pcm_get_vol();
		if (mix_vol > 16)
		{
			mix_vol = 16;
		}
		if (mix_vol > 0)
		{
			level = mix_vol;
		}
	}
#ifdef CONFIG_DIVOOM_MPP_NET_AUDIO
    else if (s_media_duck_network_held) {
        level = divoom_com_vol_ui_to_level(s_network_vol_0_100);
    }
#endif
	if (level <= 0 || level == (int)s_iisin_eq_last)
	{
		return;
	}
	if (divoom_btcore_uart_cmd_set_iisin_eq((uint8_t)level) != 0)
	{
		return;
	}
	s_iisin_eq_last = (uint8_t)level;
	divoom_print("iisin_eq: level=%d tip=%u/%u sleep=%u mix=%d net=%u\n",
		     level, (unsigned)s_amp_tip_held, (unsigned)s_amp_tip_level,
		     (unsigned)s_amp_sleep_held, mix_vol,
#ifdef CONFIG_DIVOOM_MPP_NET_AUDIO
		     (unsigned)s_network_vol_0_100
#else
		     0u
#endif
		     );
}

void divoom_audio_policy_iisin_eq_on_mix_vol(void)
{
	divoom_audio_policy_iisin_eq_sync();
}

/* 提示音会话占用 JL 门控；首次出声 prime JL，响度由 tone_lite 数字域承担 */
void divoom_audio_policy_amp_tip_hold(int jl_level_0_100)
{
	if (jl_level_0_100 < 0)
	{
		jl_level_0_100 = 0;
	}
	else if (jl_level_0_100 > 100)
	{
		jl_level_0_100 = 100;
	}
	if (!(s_amp_tip_held && s_amp_tip_level == (uint8_t)jl_level_0_100))
	{
		s_amp_tip_level = (uint8_t)jl_level_0_100;
		s_amp_tip_held = 1;
		divoom_audio_policy_amp_sync();
	}
	divoom_audio_policy_iisin_eq_sync();
}

void divoom_audio_policy_amp_tip_release(void)
{
	if (!s_amp_tip_held)
	{
		return;
	}
	s_amp_tip_held = 0;
	divoom_audio_policy_amp_sync();
	divoom_audio_policy_iisin_eq_sync();
}

/*
 * 曾用于开关 I2S 时钟前把 JL 拉 0。现在 audio_sink 的流常开不再启停，
 * 而拉 JL 本身就是一次会咔哒的跳变，所以这里不再碰 JL。
 */
void divoom_audio_policy_amp_prepare_i2s_clock(void)
{
}

/* 静音改由数字域完成，这里保留接口但不再改 JL 档位 */
void divoom_audio_policy_amp_force_mute(void)
{
}

void divoom_audio_policy_amp_restore(void)
{
	divoom_audio_policy_amp_sync();
}

void divoom_audio_policy_set_voice_clip_recording(int on)
{
	if (on) {
		if (s_voice_clip_recording)
			return;
		s_voice_clip_recording = 1;
		divoom_audio_policy_media_duck_hold("record");
		return;
	}
	if (!s_voice_clip_recording)
		return;
	s_voice_clip_recording = 0;
	divoom_audio_policy_media_duck_release("record");
}

int divoom_audio_policy_is_voice_clip_recording(void)
{
	return s_voice_clip_recording ? 1 : 0;
}

void divoom_audio_policy_set_sleep_playback_active(int on)
{
	s_sleep_playback_active = on ? 1u : 0u;
	if (on)
	{
		if (!s_amp_sleep_held)
		{
			s_amp_sleep_held = 1;
			divoom_audio_policy_amp_sync();
		}
		divoom_audio_policy_media_duck_hold("sleep");
	}
	else
	{
		if (s_amp_sleep_held)
		{
			s_amp_sleep_held = 0;
			divoom_audio_policy_amp_sync();
		}
		divoom_audio_policy_media_duck_release("sleep");
	}
	divoom_audio_policy_iisin_eq_sync();
}

static void divoom_audio_policy_media_duck_hold_locked(const char *tag)
{
	int changed = 0;

	if (tag && strcmp(tag, "sleep") == 0)
	{
		if (!s_media_duck_sleep_held)
		{
			s_media_duck_sleep_held = 1;
			changed = 1;
		}
	}
	else if (tag && strcmp(tag, "record") == 0)
	{
		if (!s_media_duck_record_held)
		{
			s_media_duck_record_held = 1;
			changed = 1;
		}
	}
	else if (!s_media_duck_tone_held)
	{
		s_media_duck_tone_held = 1;
		changed = 1;
	}
	if (changed)
	{
		divoom_audio_policy_media_duck_sync(tag);
	}
}

static void divoom_audio_policy_media_duck_release_locked(const char *tag)
{
	int changed = 0;

	if (tag && strcmp(tag, "sleep") == 0)
	{
		if (s_media_duck_sleep_held)
		{
			s_media_duck_sleep_held = 0;
			changed = 1;
		}
	}
	else if (tag && strcmp(tag, "record") == 0)
	{
		if (s_media_duck_record_held)
		{
			s_media_duck_record_held = 0;
			changed = 1;
		}
	}
	else if (s_media_duck_tone_held)
	{
		s_media_duck_tone_held = 0;
		changed = 1;
	}
	if (changed)
	{
		divoom_audio_policy_media_duck_sync(tag);
	}
}

int divoom_audio_policy_is_sleep_playback_active(void)
{
	return s_sleep_playback_active ? 1 : 0;
}

int divoom_audio_policy_tone_play_allowed(void)
{
	return s_voice_clip_recording ? 0 : 1;
}

divoom_audio_policy_action_t divoom_audio_policy_evaluate_tone(
	divoom_audio_session_state_t state, const divoom_audio_policy_tone_ctx_t *ctx)
{
	(void)ctx;

	if (s_voice_clip_recording)
	{
		return DIVOOM_AUDIO_POLICY_DENY;
	}
	switch (state)
	{
	case DIVOOM_AUDIO_SESSION_STATE_IDLE:
		return DIVOOM_AUDIO_POLICY_ALLOW;
	case DIVOOM_AUDIO_SESSION_STATE_SLEEP:
		return DIVOOM_AUDIO_POLICY_DUCK_OVERLAY;
	case DIVOOM_AUDIO_SESSION_STATE_TONE:
		return DIVOOM_AUDIO_POLICY_ALLOW;
	case DIVOOM_AUDIO_SESSION_STATE_OVERLAY:
		return DIVOOM_AUDIO_POLICY_DUCK_HOLD;
	case DIVOOM_AUDIO_SESSION_STATE_HANDOFF:
		return DIVOOM_AUDIO_POLICY_DENY;
	default:
		return DIVOOM_AUDIO_POLICY_DENY;
	}
}

int divoom_audio_policy_keep_jl_after_tone(void)
{
	divoom_audio_session_state_t st = divoom_audio_session_get_state();

#ifdef CONFIG_DIVOOM_MPP_NET_AUDIO
    if (s_media_duck_network_held) return 1;
#endif
	return (st == DIVOOM_AUDIO_SESSION_STATE_SLEEP
		|| st == DIVOOM_AUDIO_SESSION_STATE_OVERLAY
		|| divoom_audio_policy_is_sleep_playback_active()) ? 1 : 0;
}

#if DIVOOM_SHOW_TF_NAND
int divoom_audio_policy_show_music_muted(void)
{
	return s_music_duck_active ? 1 : 0;
}
#endif

static void divoom_audio_policy_on_tone_begin_locked(unsigned int sleep_duck_pct)
{
	if (sleep_duck_pct > 100)
	{
		sleep_duck_pct = 100;
	}
#if DIVOOM_SHOW_TF_NAND
	/* 助眠 overlay 时 media_duck 可能已 mute；未 hold 则这里补 mute，不要只压 25% */
	if (divoom_audio_sink_ch_active(DIVOOM_AUDIO_SINK_CH_MUSIC) && !s_music_duck_active)
	{
		s_music_duck_saved = (uint8_t)divoom_audio_sink_ch_get_duck(DIVOOM_AUDIO_SINK_CH_MUSIC);
		if (s_music_duck_saved == 0)
			s_music_duck_saved = 100;
		divoom_audio_sink_ch_set_duck(DIVOOM_AUDIO_SINK_CH_MUSIC, 0);
		s_music_duck_active = 1;
		divoom_print("audio_policy: mute local MUSIC (saved %u)\n",
			     (unsigned)s_music_duck_saved);
	}
#endif
	if (!divoom_audio_sink_ch_active(DIVOOM_AUDIO_SINK_CH_SLEEP))
	{
		return;
	}
	/* 切歌链：上一首 cut 未 end 时保持 duck，勿重复保存/覆盖 */
	if (s_tone_duck_active)
	{
		return;
	}
	s_tone_duck_saved = (uint8_t)divoom_audio_sink_ch_get_duck(DIVOOM_AUDIO_SINK_CH_SLEEP);
	divoom_audio_sink_ch_set_duck(DIVOOM_AUDIO_SINK_CH_SLEEP, sleep_duck_pct);
	s_tone_duck_active = 1;
	divoom_print("audio_policy: sleep duck %u%% (saved %u)\n",
		     sleep_duck_pct, (unsigned)s_tone_duck_saved);
}

static void divoom_audio_policy_on_tone_end_locked(void)
{
#if DIVOOM_SHOW_TF_NAND
	/* MUSIC mute 由 media_duck 收尾；此处仅在已无 hold 时兜底恢复 */
	if (s_music_duck_active
	    && !s_media_duck_tone_held
	    && !s_media_duck_sleep_held
	    && !s_media_duck_record_held
#ifdef CONFIG_DIVOOM_MPP_NET_AUDIO
	    && !s_media_duck_network_held
#endif
	    )
	{
		divoom_audio_sink_ch_set_duck(DIVOOM_AUDIO_SINK_CH_MUSIC, s_music_duck_saved);
		s_music_duck_active = 0;
		divoom_print("audio_policy: music duck restore %u\n", (unsigned)s_music_duck_saved);
	}
#endif
	if (!divoom_audio_sink_ch_active(DIVOOM_AUDIO_SINK_CH_SLEEP))
	{
		s_tone_duck_active = 0;
		return;
	}
	if (!s_tone_duck_active)
	{
		return;
	}
	divoom_audio_sink_ch_set_duck(DIVOOM_AUDIO_SINK_CH_SLEEP, s_tone_duck_saved);
	s_tone_duck_active = 0;
	divoom_print("audio_policy: sleep duck restore %u\n", (unsigned)s_tone_duck_saved);
}

void divoom_audio_policy_diag_print(void)
{
	printf("  policy: sleep_playback=%d voice_clip=%d\n",
	       s_sleep_playback_active ? 1 : 0, s_voice_clip_recording ? 1 : 0);
	printf("  duck: active=%u saved=%u current=%u\n",
	       (unsigned)s_tone_duck_active, (unsigned)s_tone_duck_saved,
	       (unsigned)divoom_audio_sink_ch_get_duck(DIVOOM_AUDIO_SINK_CH_SLEEP));
	printf("  media_duck: tip=%u sleep=%u record=%u level=%d\n",
	       (unsigned)s_media_duck_tone_held, (unsigned)s_media_duck_sleep_held,
	       (unsigned)s_media_duck_record_held, divoom_jl_media_volume_get_current());
	printf("  amp_gate: tip_held=%u sleep_held=%u gate_on=%u level=%u\n",
	       (unsigned)s_amp_tip_held, (unsigned)s_amp_sleep_held,
	       (unsigned)s_amp_gate_on, (unsigned)s_amp_gate_level);
	printf("  iisin_eq: last=%u mix=%d\n",
	       (unsigned)s_iisin_eq_last, divoom_mix_pcm_get_vol());
}

#ifdef CONFIG_DIVOOM_MPP_NET_AUDIO
void divoom_audio_policy_network_set_volume(unsigned int vol_0_100)
{
    if (vol_0_100 > 100u)
        vol_0_100 = 100u;
    s_network_vol_0_100 = (uint8_t)vol_0_100;
    if (s_media_duck_network_held)
        divoom_audio_policy_iisin_eq_sync();
}

void divoom_audio_policy_network_active(int on)
{
    media_policy_lock();
    s_media_duck_network_held = on ? 1 : 0;
    divoom_audio_policy_media_duck_sync("network");
    media_policy_unlock();
    if (on) {
        divoom_jl_amp_adapter_prime_if_needed();
        divoom_audio_policy_iisin_eq_sync();
    }
}
#endif

void divoom_audio_policy_media_duck_hold(const char *tag)
{
    media_policy_lock();
    divoom_audio_policy_media_duck_hold_locked(tag);
    media_policy_unlock();
}

void divoom_audio_policy_media_duck_release(const char *tag)
{
    media_policy_lock();
    divoom_audio_policy_media_duck_release_locked(tag);
    media_policy_unlock();
}

void divoom_audio_policy_on_tone_begin(unsigned int sleep_duck_pct)
{
    media_policy_lock();
    divoom_audio_policy_on_tone_begin_locked(sleep_duck_pct);
    media_policy_unlock();
}

void divoom_audio_policy_on_tone_end(void)
{
    media_policy_lock();
    divoom_audio_policy_on_tone_end_locked();
    media_policy_unlock();
}

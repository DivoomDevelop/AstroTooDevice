#include "divoom_build_config.h"

#include "divoom_sleep_pcm_session.h"
#include "divoom_audio_policy.h"
#include "divoom_audio_session.h"
#include "divoom.h"
#include "divoom_mix_pcm_ctrl.h"
#include "divoom_snd_pcm.h"

#include <string.h>

#if DIVOOM_BUILD_RTOS_DEVICE
#define sleep_pcm_session_pcm_alive()  divoom_snd_pcm_whitebox_is_running()
#else
#define sleep_pcm_session_pcm_alive()  divoom_mix_pcm_get_play_stat()
#endif

#define DIVOOM_SLEEP_PCM_SESSION_MPG_SWITCH_DEBOUNCE_MS  400u
#define DIVOOM_SLEEP_PCM_SESSION_MPG_VOL_PERCENT       100
#define DIVOOM_SLEEP_PCM_SESSION_MIN_MPG_BYTES          1024

static int sleep_pcm_session_mpg_file_ok(const char *mp3_path)
{
	int sz;

	if (!mp3_path || mp3_path[0] == '\0')
	{
		return 0;
	}
	sz = divoom_app_com_get_file_size((char *)mp3_path);
	if (sz < DIVOOM_SLEEP_PCM_SESSION_MIN_MPG_BYTES)
	{
		divoom_print("sleep_pcm_session: mpg file too small: %s (%d)\n", mp3_path, sz);
		return 0;
	}
	return 1;
}

static divoom_sleep_pcm_session_mode_t s_mode = DIVOOM_SLEEP_PCM_SESSION_NONE;
static char s_mpg_path[256];
static uint32_t s_mpg_switch_last_ms;

static void sleep_pcm_session_update_policy(void)
{
	divoom_audio_policy_set_sleep_playback_active(
		s_mode != DIVOOM_SLEEP_PCM_SESSION_NONE ? 1 : 0);
}

static void sleep_pcm_session_store_mpg_path(const char *mp3_path)
{
	if (!mp3_path)
	{
		s_mpg_path[0] = '\0';
		return;
	}
	strncpy(s_mpg_path, mp3_path, sizeof(s_mpg_path) - 1u);
	s_mpg_path[sizeof(s_mpg_path) - 1u] = '\0';
}

static void sleep_pcm_session_clear(void)
{
	s_mode = DIVOOM_SLEEP_PCM_SESSION_NONE;
	s_mpg_path[0] = '\0';
	sleep_pcm_session_update_policy();
	divoom_audio_session_on_sleep_pcm_stop();
}

static void sleep_pcm_session_close_pcm(void)
{
	/* 先停产流(mix_pcm)，再停消费(snd_pcm)，避免 ring 堆满把 close 卡死 */
	divoom_mix_pcm_ctrl_close();
	divoom_snd_pcm_close();
}

int divoom_sleep_pcm_session_start_diy(void)
{
	if (s_mode == DIVOOM_SLEEP_PCM_SESSION_DIY
	    && sleep_pcm_session_pcm_alive())
	{
		return 0;
	}

	if (s_mode != DIVOOM_SLEEP_PCM_SESSION_NONE)
	{
		sleep_pcm_session_close_pcm();
		sleep_pcm_session_clear();
	}

	divoom_mix_pcm_ctrl_init();
	divoom_snd_pcm_init();
	if (!sleep_pcm_session_pcm_alive())
	{
		divoom_mix_pcm_ctrl_close();
		return -1;
	}

	s_mode = DIVOOM_SLEEP_PCM_SESSION_DIY;
	sleep_pcm_session_update_policy();
	divoom_audio_session_on_sleep_pcm_start();
	return 0;
}

void divoom_sleep_pcm_session_stop(void)
{
	if (s_mode == DIVOOM_SLEEP_PCM_SESSION_NONE)
	{
		return;
	}
	sleep_pcm_session_close_pcm();
	sleep_pcm_session_clear();
}

void divoom_sleep_pcm_session_stop_async(void)
{
	divoom_audio_session_sleep_stop_async();
}

divoom_sleep_pcm_session_mode_t divoom_sleep_pcm_session_get_mode(void)
{
	return s_mode;
}

int divoom_sleep_pcm_session_is_running(void)
{
	return (s_mode != DIVOOM_SLEEP_PCM_SESSION_NONE
	        && sleep_pcm_session_pcm_alive()) ? 1 : 0;
}

int divoom_sleep_pcm_session_start_or_switch_mpg(const char *mp3_path, int vol_percent)
{
	(void)vol_percent;

	if (!mp3_path)
	{
		return -1;
	}
	if (!sleep_pcm_session_mpg_file_ok(mp3_path))
	{
		return -1;
	}

	if (s_mode == DIVOOM_SLEEP_PCM_SESSION_MPG)
	{
		if (strcmp(s_mpg_path, mp3_path) == 0)
		{
			return 0;
		}
		if (divoom_system_get_tickms() - s_mpg_switch_last_ms
		    < DIVOOM_SLEEP_PCM_SESSION_MPG_SWITCH_DEBOUNCE_MS)
		{
			return 0;
		}
		if (divoom_mix_pcm_ctrl_switch_mpg123(mp3_path,
						      DIVOOM_SLEEP_PCM_SESSION_MPG_VOL_PERCENT))
		{
			sleep_pcm_session_store_mpg_path(mp3_path);
			s_mpg_switch_last_ms = divoom_system_get_tickms();
			divoom_print("sleep_pcm_session mpg switch: %s\n", mp3_path);
			return 0;
		}
		divoom_print("sleep_pcm_session mpg switch fail, keep: %s\n", mp3_path);
		return 0;
	}

	if (s_mode == DIVOOM_SLEEP_PCM_SESSION_DIY)
	{
		if (divoom_mix_pcm_ctrl_switch_diy_to_mpg123(
			    mp3_path, DIVOOM_SLEEP_PCM_SESSION_MPG_VOL_PERCENT))
		{
			s_mode = DIVOOM_SLEEP_PCM_SESSION_MPG;
			sleep_pcm_session_store_mpg_path(mp3_path);
			sleep_pcm_session_update_policy();
			divoom_print("sleep_pcm_session mpg switch from diy: %s\n", mp3_path);
			return 0;
		}
		return -1;
	}

	divoom_mix_pcm_ctrl_init_mpg123(mp3_path, DIVOOM_SLEEP_PCM_SESSION_MPG_VOL_PERCENT);
	if (!divoom_mix_pcm_get_play_stat())
	{
		sleep_pcm_session_close_pcm();
		sleep_pcm_session_clear();
		return -1;
	}
	divoom_snd_pcm_init();
	s_mode = DIVOOM_SLEEP_PCM_SESSION_MPG;
	sleep_pcm_session_store_mpg_path(mp3_path);
	s_mpg_switch_last_ms = divoom_system_get_tickms();
	sleep_pcm_session_update_policy();
	divoom_audio_session_on_sleep_pcm_start();
	divoom_print("sleep_pcm_session mpg cold start: %s\n", mp3_path);
	return 0;
}

int divoom_sleep_pcm_session_switch_mpg_to_diy(void)
{
	if (s_mode != DIVOOM_SLEEP_PCM_SESSION_MPG)
	{
		return 0;
	}
	if (!divoom_mix_pcm_ctrl_switch_mpg123_to_diy())
	{
		return 0;
	}
	s_mode = DIVOOM_SLEEP_PCM_SESSION_DIY;
	s_mpg_path[0] = '\0';
	sleep_pcm_session_update_policy();
	return 1;
}

const char *divoom_sleep_pcm_session_mpg_path(void)
{
	return s_mpg_path;
}

void divoom_sleep_pcm_session_set_diy_mix_vol_level(int level_0_16)
{
	int pct;

	if (level_0_16 < 0)
	{
		level_0_16 = 0;
	}
	else if (level_0_16 > 16)
	{
		level_0_16 = 16;
	}
	pct = (level_0_16 * 100) / 16;
	divoom_audio_session_sleep_set_volume(pct);
}

void divoom_sleep_pcm_session_set_mpg_vol_percent(int vol_percent)
{
	divoom_audio_session_sleep_set_volume(vol_percent);
}

void divoom_sleep_pcm_session_prepare_i2s(unsigned int timeout_ms)
{
	divoom_audio_session_prepare_sleep_from_tone(timeout_ms);
}

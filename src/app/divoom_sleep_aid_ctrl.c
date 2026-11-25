#include "divoom.h"
#include "divoom_sleep_aid_ctrl.h"
#include "divoom_local_music_ctrl.h"
#include "divoom_manage_image.h"
#include "divoom_app_com.h"
#include "divoom_app_schedule.h"
#include "divoom_local_music_ctrl.h"
#include "divoom_menu_control.h"
#include "divoom_local_movie_play.h"
#include "divoom_voice_read.h"
#include "divoom_main.h"
#include "divoom_onoff.h"
#include "divoom_audio_policy.h"
#include "divoom_sleep_pcm_session.h"
#include "divoom_audio_sink.h"
#include "divoom_audio_session.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define DIVOOM_SLEEP_AID_RECORD_TMP_MP3 DIVOOM_TMP_DIR "/divoom_sleep_aid_cap.mp3"
#define DIVOOM_SLEEP_AID_RECORD_MP3_BITRATE_KBPS 32
#define DIVOOM_SLEEP_AID_RECORD_MAX_SLOTS 5
#define DIVOOM_SLEEP_AID_RECORD_MIN_BYTES 256

/** 列表/试听循环播放（秒），与 divoom_sleep 一致用大值表示持续播放 */
#define DIVOOM_SLEEP_AID_VOICE_PLAY_SEC 24*3600

#define SLEEP_AID_SETT_BRIGHT_MAX 100
#define SLEEP_AID_SETT_VOL_MAX    100

#define SLEEP_AID_PLAY_PHASE_NONE    0
#define SLEEP_AID_PLAY_PHASE_BEFORE  1
#define SLEEP_AID_PLAY_PHASE_AFTER   2

#define SLEEP_AID_AFTER_MUSIC_COUNT  3

/* 助眠定时关机联调：非 0 时睡前/入睡后 deadline 均固定为该秒数（忽略设置页分钟值）；测完改回 0 */
#define DIVOOM_SLEEP_AID_PHASE_TEST_SEC  0

static const int sleep_aid_after_voice_id_list[SLEEP_AID_AFTER_MUSIC_COUNT] =
{
	DIVOOM_VOICE_ID_AUTO_MASK,
	DIVOOM_VOICE_ID_RAIN_STEADY,
	DIVOOM_VOICE_ID_FAN_STANDY,
};

typedef struct _sleep_aid_msg_voice_pair
{
	int voice_type;
	int voice_id;
} sleep_aid_msg_voice_pair_t;

/* 助眠音频慢操作 job（实现见文件后部的串行 worker） */
#define SLEEP_AID_JOB_QUEUE_LEN  8
#define SLEEP_AID_JOB_TASK_PRIO  7  /* 低于 LVGL(10) 与 audio_sink(11)，高于 model async(4) */

enum
{
	SLEEP_AID_JOB_SELECT_VOICE = 0,
	SLEEP_AID_JOB_PLAY_VOICE_VIDEO,
	SLEEP_AID_JOB_STOP_VOICE,
	SLEEP_AID_JOB_VOICE_PREVIEW,
	SLEEP_AID_JOB_AFTER_MUSIC_PREVIEW,
	SLEEP_AID_JOB_AFTER_TONE_PLAY,
	SLEEP_AID_JOB_PCM_STOP,
	SLEEP_AID_JOB_CANCEL_PREVIEW_SCHEDULE,
};

typedef struct _sleep_aid_job
{
	int kind;
	int arg0;
	int arg1;
	int arg2;
	unsigned int play_generation;
} sleep_aid_job_t;

static divoom_sleep_aid_ctrl s_sleep_aid_ctrl;

static void sleep_aid_ctrl_ensure_settings_init(Pdivoom_sleep_aid_ctrl c);
static int sleep_aid_settings_save(Pdivoom_sleep_aid_ctrl c);
static void sleep_aid_play_phase_stop(Pdivoom_sleep_aid_ctrl c);
static void sleep_aid_play_phase_begin(Pdivoom_sleep_aid_ctrl c);
static void sleep_aid_play_phase_set_deadline_minutes(Pdivoom_sleep_aid_ctrl c, int minutes);
static void sleep_aid_play_phase_on_before_done(Pdivoom_sleep_aid_ctrl c);
static void sleep_aid_play_phase_on_after_done(Pdivoom_sleep_aid_ctrl c);
static int sleep_aid_record_times_load(Pdivoom_sleep_aid_ctrl c);
static int sleep_aid_record_times_save(Pdivoom_sleep_aid_ctrl c);
static int sleep_aid_record_temp_clip_bytes(void);
static void sleep_aid_record_sort_slots(Pdivoom_sleep_aid_ctrl c, int order[5]);
static int sleep_aid_voice_play_volume_percent(void);
static void sleep_aid_set_diy_mix_active(Pdivoom_sleep_aid_ctrl c, int on);
static void sleep_aid_set_mpg_loop_active(Pdivoom_sleep_aid_ctrl c, int on);
static void sleep_aid_sync_mpg_path_from_session(Pdivoom_sleep_aid_ctrl c);
static void sleep_aid_job_worker_start(void);
static void sleep_aid_job_post(int kind, int arg0, int arg1, int arg2);
static void sleep_aid_pcm_stop_async(void);
static int sleep_aid_pcm_stop_wait(unsigned int timeout_ms);

uint8 divoom_sleep_aid_ctrl_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type);

/** DIY 混合播放启停：PCM 生命周期由 divoom_sleep_pcm_session 统一管理。 */

static int sleep_aid_diy_mix_start(Pdivoom_sleep_aid_ctrl c)
{
	if (!c)
	{
		return -1;
	}
	if (divoom_sleep_pcm_session_start_diy() != 0)
	{
		divoom_print("sleep_aid: diy mix start fail (snd_pcm)\n");
		return -1;
	}
	{
		int vol = sleep_aid_voice_play_volume_percent();
		int level = (vol * 16 + 50) / 100;

		if (level > 16)
		{
			level = 16;
		}
		if (level < 0)
		{
			level = 0;
		}
		divoom_sleep_pcm_session_set_diy_mix_vol_level(level);
	}
	c->mpg_loop_src_path[0] = '\0';
	sleep_aid_set_diy_mix_active(c, 1);
	return 0;
}

static void sleep_aid_diy_mix_stop(Pdivoom_sleep_aid_ctrl c)
{
	if (!c || divoom_sleep_pcm_session_get_mode() != DIVOOM_SLEEP_PCM_SESSION_DIY)
	{
		return;
	}
	divoom_sleep_pcm_session_stop();
	sleep_aid_set_diy_mix_active(c, 0);
}

static void sleep_aid_mpg_loop_set_volume(int vol);
static void sleep_aid_voice_stop_playback(void);
static void sleep_aid_voice_stop_playback_async(void);

static void sleep_aid_set_diy_mix_active(Pdivoom_sleep_aid_ctrl c, int on)
{
	if (!c)
	{
		return;
	}
	c->diy_mix_active = on ? 1u : 0u;
}

static void sleep_aid_set_mpg_loop_active(Pdivoom_sleep_aid_ctrl c, int on)
{
	if (!c)
	{
		return;
	}
	c->mpg_loop_active = on ? 1u : 0u;
	if (!on)
	{
		c->mpg_loop_src_path[0] = '\0';
	}
}

static void sleep_aid_sync_mpg_path_from_session(Pdivoom_sleep_aid_ctrl c)
{
	const char *path;

	if (!c)
	{
		return;
	}
	path = divoom_sleep_pcm_session_mpg_path();
	if (!path || path[0] == '\0')
	{
		c->mpg_loop_src_path[0] = '\0';
		return;
	}
	strncpy(c->mpg_loop_src_path, path, sizeof(c->mpg_loop_src_path) - 1);
	c->mpg_loop_src_path[sizeof(c->mpg_loop_src_path) - 1] = '\0';
}

static void sleep_aid_mpg_loop_stop(Pdivoom_sleep_aid_ctrl c)
{
	if (!c || divoom_sleep_pcm_session_get_mode() != DIVOOM_SLEEP_PCM_SESSION_MPG)
	{
		return;
	}
	divoom_sleep_pcm_session_stop();
	sleep_aid_set_mpg_loop_active(c, 0);
}

static int sleep_aid_mpg_loop_start(const char *mp3_path, int vol)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();
	divoom_sleep_pcm_session_mode_t prev_mode;
	int r;

	if (!c || !mp3_path)
	{
		return -1;
	}
	if (divoom_sleep_pcm_session_get_mode() == DIVOOM_SLEEP_PCM_SESSION_MPG
	    && strcmp(c->mpg_loop_src_path, mp3_path) == 0)
	{
		sleep_aid_mpg_loop_set_volume(vol);
		return 0;
	}

	prev_mode = divoom_sleep_pcm_session_get_mode();
	if (prev_mode == DIVOOM_SLEEP_PCM_SESSION_NONE)
	{
		sleep_aid_voice_stop_playback();
	}

	r = divoom_sleep_pcm_session_start_or_switch_mpg(mp3_path, vol);
	if (r != 0 && prev_mode == DIVOOM_SLEEP_PCM_SESSION_DIY)
	{
		sleep_aid_voice_stop_playback();
		r = divoom_sleep_pcm_session_start_or_switch_mpg(mp3_path, vol);
	}
	if (r != 0)
	{
		return -1;
	}
	if (divoom_sleep_pcm_session_get_mode() != DIVOOM_SLEEP_PCM_SESSION_MPG)
	{
		return 0;
	}

	c->diy_mix_active = 0;
	sleep_aid_set_mpg_loop_active(c, 1);
	sleep_aid_sync_mpg_path_from_session(c);
	divoom_sleep_pcm_session_set_mpg_vol_percent(vol);
	divoom_print("sleep_aid mpg123 loop: %s vol=%d\n", mp3_path, vol);
	return 0;
}

static void sleep_aid_mpg_loop_set_volume(int vol)
{
	divoom_sleep_pcm_session_set_mpg_vol_percent(vol);
}

static void sleep_aid_voice_stop_playback_ex(int async)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();

	if (divoom_sleep_pcm_session_get_mode() != DIVOOM_SLEEP_PCM_SESSION_NONE)
	{
		if (async)
		{
			divoom_sleep_pcm_session_stop_async();
		}
		else
		{
			divoom_sleep_pcm_session_stop();
		}
		if (c)
		{
			c->diy_mix_active = 0;
			sleep_aid_set_mpg_loop_active(c, 0);
		}
		return;
	}

	int remain = 0;
	int st = divoom_alarm_get_alarm_active_status(&remain);
	if (st == DIVOOM_ALARM_STATUS_INTTRUPT
		|| st == DIVOOM_ALARM_STATUS_ACTIVE
		|| st == DIVOOM_ALARM_STATUS_SNOOZE)
	{
		divoom_print("sleep_aid: skip stop_tone, alarm owns tone\n");
		return;
	}
	divoom_local_music_ctrl_stop_tone();
}

static void sleep_aid_voice_stop_playback(void)
{
	sleep_aid_voice_stop_playback_ex(0);
}

static void sleep_aid_voice_stop_playback_async(void)
{
	sleep_aid_voice_stop_playback_ex(1);
}

static uint8 sleep_aid_slider_level_to_sys_brightness(int level_0_100)
{
	int v;

	if (level_0_100 < 0)
	{
		level_0_100 = 0;
	}
	if (level_0_100 > SLEEP_AID_SETT_BRIGHT_MAX)
	{
		level_0_100 = SLEEP_AID_SETT_BRIGHT_MAX;
	}
	v = level_0_100;
	if (level_0_100 > 0 && v < 1)
	{
		v = 1;
	}
	return (uint8)v;
}

static int sleep_aid_clamp_vol_level(int vol)
{
	if (vol < 0)
	{
		vol = 0;
	}
	if (vol > SLEEP_AID_SETT_VOL_MAX)
	{
		vol = SLEEP_AID_SETT_VOL_MAX;
	}
	return vol;
}

static void sleep_aid_save_sys_light(Pdivoom_sleep_aid_ctrl c)
{
	if (!c || c->saved_sys_light_valid)
	{
		return;
	}
	c->saved_sys_light = divoom_main_get_sys_light();
	c->saved_sys_light_valid = 1;
	divoom_print("sleep_aid_phase: save brightness=%u\n", (unsigned)c->saved_sys_light);
}

static void sleep_aid_restore_sys_light(Pdivoom_sleep_aid_ctrl c)
{
	if (!c || !c->saved_sys_light_valid)
	{
		return;
	}
	divoom_print("sleep_aid_phase: restore brightness=%u\n", (unsigned)c->saved_sys_light);
	divoom_main_set_sys_light(c->saved_sys_light);
	c->saved_sys_light_valid = 0;
}

static void sleep_aid_apply_sys_brightness_level(int level_0_100)
{
	uint8 bright;

	bright = sleep_aid_slider_level_to_sys_brightness(level_0_100);
	divoom_main_set_sleep_brightness(bright);
	divoom_print("sleep_aid_phase: brightness=%u (level=%d)\n", (unsigned)bright, level_0_100);
}

static int sleep_aid_voice_play_volume_percent(void)
{
	Pdivoom_sleep_aid_ctrl c;
	divoom_sleep_aid_info *inf;
	int vol;

	c = divoom_sleep_aid_ctrl_instance();
	if (c && c->settings_preview_vol_active)
	{
		return c->settings_preview_vol_percent;
	}
	sleep_aid_ctrl_ensure_settings_init(c);
	inf = &c->settings;
	if (c->play_phase == SLEEP_AID_PLAY_PHASE_AFTER && inf->after_on_off)
	{
		vol = inf->after_vol;
	}
	else
	{
		vol = inf->before_vol;
	}
	if (vol < 0)
	{
		vol = 0;
	}
	if (vol > SLEEP_AID_SETT_VOL_MAX)
	{
		vol = SLEEP_AID_SETT_VOL_MAX;
	}
	return vol;
}

static void sleep_aid_play_phase_stop(Pdivoom_sleep_aid_ctrl c)
{
	if (!c)
	{
		return;
	}
	c->play_phase_deadline = 0;
	c->play_phase = SLEEP_AID_PLAY_PHASE_NONE;
}

static void sleep_aid_play_phase_set_deadline_minutes(Pdivoom_sleep_aid_ctrl c, int minutes)
{
	time_t now;

	if (!c)
	{
		return;
	}
#if DIVOOM_SLEEP_AID_PHASE_TEST_SEC > 0
	time(&now);
	c->play_phase_deadline = (uint32)((unsigned long)now
					  + (unsigned long)DIVOOM_SLEEP_AID_PHASE_TEST_SEC);
	divoom_print("sleep_aid_phase: TEST deadline phase=%u in %d sec (utc=%u)\n",
		     (unsigned)c->play_phase, DIVOOM_SLEEP_AID_PHASE_TEST_SEC,
		     (unsigned)c->play_phase_deadline);
	return;
#endif
	if (minutes <= 0)
	{
		c->play_phase_deadline = 0;
		return;
	}
	time(&now);
	c->play_phase_deadline = (uint32)((unsigned long)now + (unsigned long)minutes * 60U);
	divoom_print("sleep_aid_phase: deadline phase=%u in %d min (utc=%u)\n",
		     (unsigned)c->play_phase, minutes, (unsigned)c->play_phase_deadline);
}

static int sleep_aid_resolve_after_music(int music_idx, int *voice_type, int *voice_id, int *video_type)
{
	static const int s_after_types[3] =
	{
		DIVOOM_SLEEP_AID_TYPE_NOISE,
		DIVOOM_SLEEP_AID_TYPE_WHITE,
		DIVOOM_SLEEP_AID_TYPE_WHITE,
	};
	divoom_sleep_aid_voice_list_item *list;
	int n;
	int pick;
	int type;

	if (!voice_type || !voice_id || !video_type)
	{
		return -1;
	}
	if (music_idx < 0 || music_idx > 2)
	{
		music_idx = 0;
	}
	type = s_after_types[music_idx];
	list = divoom_sleep_aid_voice_get_list(type, &n, 1);
	if (!list || n <= 0)
	{
		if (list)
		{
			divoom_system_free(list);
		}
		divoom_print("sleep_aid_phase: after_music list empty type=%d idx=%d\n", type, music_idx);
		return -1;
	}
	pick = 0;
	if (music_idx == 2 && n > 1)
	{
		pick = 1;
	}
	*voice_type = list[pick].audio_type;
	*voice_id = (int)list[pick].audio_id;
	*video_type = list[pick].video_type;
	divoom_system_free(list);
	divoom_print("sleep_aid_phase: after_music idx=%d -> type=%d id=%d vt=%d\n",
		     music_idx, *voice_type, *voice_id, *video_type);
	return 0;
}

static int sleep_aid_after_music_index(int music_idx)
{
	if (music_idx < 0 || music_idx >= SLEEP_AID_AFTER_MUSIC_COUNT)
	{
		music_idx = 0;
	}
	return music_idx;
}

static void sleep_aid_after_tone_play(int music_idx, int vol_level_0_100)
{
	char file_name[128];
	int idx;
	int vol;

	idx = sleep_aid_after_music_index(music_idx);
	vol = sleep_aid_clamp_vol_level(vol_level_0_100);
	if (divoom_voice_read_get_filename((uint32)sleep_aid_after_voice_id_list[idx], file_name))
	{
		sleep_aid_voice_stop_playback();
		divoom_local_movie_play_stop_movie();
		if (sleep_aid_mpg_loop_start(file_name, vol) != 0)
		{
			divoom_local_music_ctrl_tone_play(file_name, vol, NULL, DIVOOM_SLEEP_AID_VOICE_PLAY_SEC);
		}
	}
}

static void sleep_aid_settings_after_voice_preview(int music_idx, int vol_level_0_100)
{
	Pdivoom_sleep_aid_ctrl c;
	int pct;

	c = divoom_sleep_aid_ctrl_instance();
	if (!c)
	{
		return;
	}
	pct = sleep_aid_clamp_vol_level(vol_level_0_100);
	c->settings_preview_vol_is_after = 1;
	c->settings_preview_vol_active = 1;
	c->settings_preview_vol_percent = pct;
	sleep_aid_after_tone_play(music_idx, pct);
}

static void sleep_aid_play_phase_poweroff(Pdivoom_sleep_aid_ctrl c)
{
	if (!c)
	{
		return;
	}
	divoom_print("sleep_aid_phase: power off\n");
	divoom_app_schedule_set_suppress_resume(1);
	divoom_sleep_aid_ctrl_cancel_play_voice_wait(3000);
	divoom_app_schedule_set_suppress_resume(0);
	divoom_onoff_poweroff_no_tone();
}

static void sleep_aid_play_phase_on_before_done(Pdivoom_sleep_aid_ctrl c)
{
	divoom_sleep_aid_info *inf;

	if (!c)
	{
		return;
	}
	sleep_aid_ctrl_ensure_settings_init(c);
	inf = &c->settings;
	divoom_print("sleep_aid_phase: before done after_on_off=%d\n", inf->after_on_off);

	if (!inf->after_on_off)
	{
		sleep_aid_play_phase_poweroff(c);
		return;
	}

	/* main_loop 线程：起停 PCM 交给 job worker，避免与 UI 侧的起停并发操作同一会话 */
	sleep_aid_job_post(SLEEP_AID_JOB_AFTER_TONE_PLAY, inf->after_music, inf->after_vol, 0);
	sleep_aid_apply_sys_brightness_level(inf->after_bright);
	c->play_phase = SLEEP_AID_PLAY_PHASE_AFTER;
	sleep_aid_play_phase_set_deadline_minutes(c, inf->after_time);
}

static void sleep_aid_play_phase_on_after_done(Pdivoom_sleep_aid_ctrl c)
{
	sleep_aid_play_phase_poweroff(c);
}

static void sleep_aid_play_phase_begin(Pdivoom_sleep_aid_ctrl c)
{
	divoom_sleep_aid_info *inf;

	if (!c || !c->play_session_active)
	{
		return;
	}
	sleep_aid_ctrl_ensure_settings_init(c);
	inf = &c->settings;
	sleep_aid_play_phase_stop(c);
	sleep_aid_save_sys_light(c);
	sleep_aid_apply_sys_brightness_level(inf->before_bright);
	c->play_phase = SLEEP_AID_PLAY_PHASE_BEFORE;
	divoom_print("sleep_aid_phase: begin before_time=%d min after_on=%d after_time=%d\n",
		     inf->before_time, inf->after_on_off, inf->after_time);
	sleep_aid_play_phase_set_deadline_minutes(c, inf->before_time);
	if (inf->before_time <= 0)
	{
		divoom_print("sleep_aid_phase: before_time off, no auto phase switch\n");
	}
}

static int sleep_aid_voice_resolve_path(int voice_type, uint32 voice_id, char *path, int path_sz)
{
	uint32 record_id;
	Pdivoom_sleep_aid_ctrl c;

	if (!path || path_sz <= 0 || voice_id == 0)
	{
		return -1;
	}

	path[0] = '\0';

	if (voice_type == DIVOOM_SLEEP_AID_TYPE_RECORD
	    || (voice_id > DIVOOM_VOICE_ID_SLEEP_AID_RECORD_BASE
		&& voice_id < DIVOOM_VOICE_ID_END_SLEEP_AID_RECORD))
	{
		record_id = voice_id - DIVOOM_VOICE_ID_SLEEP_AID_RECORD_BASE;
		if (record_id == 0 || record_id > DIVOOM_SLEEP_AID_RECORD_MAX_SLOTS)
		{
			return -1;
		}

		c = divoom_sleep_aid_ctrl_instance();
		sleep_aid_record_times_load(c);
		if (c->record_time[record_id - 1] == 0)
		{
			return -1;
		}

		divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_SLEEP_AID_RECORD,
							   record_id, path);
		if (path[0] == '\0')
		{
			return -1;
		}
		if (!divoom_port_file_exists(path))
		{
			return -1;
		}
		if (divoom_app_com_get_file_size(path) < DIVOOM_SLEEP_AID_RECORD_MIN_BYTES)
		{
			return -1;
		}
		return 0;
	}

	if (voice_type == DIVOOM_SLEEP_AID_TYPE_DIY)
	{
		return -1;
	}

	divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_SLEEP_AID_PIC, voice_id, path);
	if (!divoom_port_file_exists(path))
	{
		return -1;
	}
	return 0;
}

static int sleep_aid_voice_pair_is_playable(int voice_type, int voice_id)
{
	char path[256];

	if (voice_id <= 0)
	{
		return 0;
	}
	if (voice_type == DIVOOM_SLEEP_AID_TYPE_DIY)
	{
		return 1;
	}
	return sleep_aid_voice_resolve_path(voice_type, (uint32)voice_id, path, (int)sizeof(path)) == 0 ? 1 : 0;
}

static void sleep_aid_persist_last_play_voice(Pdivoom_sleep_aid_ctrl c, int voice_type, int voice_id)
{
	if (!c || voice_id <= 0)
	{
		return;
	}
	sleep_aid_ctrl_ensure_settings_init(c);
	c->settings.last_voice_type = voice_type;
	c->settings.last_voice_id = voice_id;
	sleep_aid_settings_save(c);
}

static int sleep_aid_resolve_first_fav_voice(int *voice_type, int *voice_id)
{
	divoom_sleep_aid_fav_item *favs;
	int n;

	favs = divoom_sleep_aid_fav_get_list(&n);
	if (!favs || n <= 0)
	{
		if (favs)
		{
			divoom_system_free(favs);
		}
		return -1;
	}
	if (sleep_aid_voice_pair_is_playable(favs[0].audio_type, (int)favs[0].audio_id))
	{
		*voice_type = favs[0].audio_type;
		*voice_id = (int)favs[0].audio_id;
		divoom_system_free(favs);
		return 0;
	}
	divoom_system_free(favs);
	return -1;
}

static int sleep_aid_resolve_last_sleep_voice(int *voice_type, int *voice_id)
{
	Pdivoom_sleep_aid_ctrl c;
	int vt;
	int vid;
	int video_type;

	if (!voice_type || !voice_id)
	{
		return -1;
	}
	c = divoom_sleep_aid_ctrl_instance();
	sleep_aid_ctrl_ensure_settings_init(c);
	vt = c->settings.last_voice_type;
	vid = c->settings.last_voice_id;
	if (sleep_aid_voice_pair_is_playable(vt, vid))
	{
		*voice_type = vt;
		*voice_id = vid;
		return 0;
	}
	if (sleep_aid_resolve_first_fav_voice(voice_type, voice_id) == 0)
	{
		return 0;
	}
	if (sleep_aid_resolve_after_music(c->settings.after_music, voice_type, voice_id, &video_type) == 0)
	{
		return 0;
	}
	return -1;
}

static void sleep_aid_voice_set_path_ext(char *path, int path_sz, const char *ext)
{
	char *dot;
	int base_len;

	if (!path || path_sz <= 0 || !ext)
	{
		return;
	}
	dot = strrchr(path, '.');
	if (dot)
	{
		*dot = '\0';
	}
	base_len = (int)strlen(path);
	if (base_len + 1 + (int)strlen(ext) < path_sz)
	{
		snprintf(path + base_len, (unsigned)(path_sz - base_len), ".%s", ext);
	}
}

void sleep_aid_voice_start_play_video(int voice_type, uint32 voice_id, int video_type)
{
    #if 1
    int vt;
    char video_name[64];

/*
    1（1、2、7）、
    3、
    8（6、8、5、17）、
    11（10、11、16）、
    12（4、12、15、slvsw）、
    13（9、13、14）
*/
    if (video_type < 0)
    {
        vt = divoom_sleep_aid_voice_get_video_type(voice_type, voice_id);
    }
    else
    {
        vt = video_type;
    }
    if (vt < 0 || vt == 1|| vt == 2|| vt == 7)
    {
        vt = 1;
    }
    else if(vt == 3)
    {
        vt = 3;
    }
    else if(vt == 6|| vt == 8|| vt == 5|| vt == 17)
    {
        vt = 8;
    }
    else if(vt == 10|| vt == 11|| vt == 16)
    {
        vt = 11;
    }
    else if(vt == 4|| vt == 12|| vt == 15|| vt == 18)
    {
        vt = 12;
    }
    else if(vt == 9|| vt == 13|| vt == 14)
    {
        vt = 13;
    }
    else
    {
        vt = 1;
    }
    snprintf(video_name, sizeof(video_name), "/usr/share/divoom_app/sleep/slv%02d.mp4", vt);
	divoom_local_movie_play_play(video_name, 0xFF,
				      DIVOOM_SLEEP_AID_VOICE_PLAY_SEC, NULL, NULL);
    #else
	divoom_local_movie_play_play("/usr/share/divoom_app/sleep/1_particles.mp4", 0xFF,
				      DIVOOM_SLEEP_AID_VOICE_PLAY_SEC, NULL, NULL);
    #endif
}

static int sleep_aid_voice_start_playback(int voice_type, uint32 voice_id, uint32 play_tiime)
{
	char path[256];
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();

	if (divoom_sleep_aid_ctrl_record_busy())
	{
		return -1;
	}

	if (voice_type == DIVOOM_SLEEP_AID_TYPE_DIY)
	{
		if (!c)
		{
			return -1;
		}
		if (divoom_sleep_pcm_session_get_mode() == DIVOOM_SLEEP_PCM_SESSION_DIY)
		{
			if (!divoom_sleep_pcm_session_is_running())
			{
				divoom_print("%s: diy active but snd_pcm dead, restart\n", __func__);
				sleep_aid_diy_mix_stop(c);
			}
			else
			{
				int vol = sleep_aid_voice_play_volume_percent();
				int level = (vol * 16 + 50) / 100;

				if (level > 16)
				{
					level = 16;
				}
				if (level < 0)
				{
					level = 0;
				}
				divoom_sleep_pcm_session_set_diy_mix_vol_level(level);
				return 0;
			}
		}
	}

	sleep_aid_voice_stop_playback();

	if (voice_type == DIVOOM_SLEEP_AID_TYPE_DIY)
	{
		if (!c)
		{
			return -1;
		}
		divoom_sleep_pcm_session_prepare_i2s(2000);
		if (divoom_sleep_pcm_session_get_mode() == DIVOOM_SLEEP_PCM_SESSION_MPG
		    && divoom_sleep_pcm_session_switch_mpg_to_diy())
		{
			int vol = sleep_aid_voice_play_volume_percent();
			int level = (vol * 16 + 50) / 100;

			if (level > 16)
			{
				level = 16;
			}
			if (level < 0)
			{
				level = 0;
			}
			divoom_sleep_pcm_session_set_diy_mix_vol_level(level);
			sleep_aid_set_mpg_loop_active(c, 0);
			sleep_aid_set_diy_mix_active(c, 1);
			divoom_print("%s: diy mix switch from mpg id=%u vol_phase=%d\n", __func__,
				     (unsigned)voice_id, sleep_aid_voice_play_volume_percent());
			return 0;
		}
		sleep_aid_diy_mix_start(c);
		if (divoom_sleep_pcm_session_get_mode() != DIVOOM_SLEEP_PCM_SESSION_DIY)
		{
			divoom_print("%s: diy mix start failed id=%u\n", __func__, (unsigned)voice_id);
			return -1;
		}
		divoom_print("%s: diy mix start id=%u vol_phase=%d\n", __func__, (unsigned)voice_id,
			     sleep_aid_voice_play_volume_percent());
		return 0;
	}

	if (sleep_aid_voice_resolve_path(voice_type, voice_id, path, (int)sizeof(path)) != 0)
	{
		divoom_print("%s: no audio file type=%d id=%u\n", __func__, voice_type, (unsigned)voice_id);
		return -1;
	}

	divoom_local_music_ctrl_tone_play(path, sleep_aid_voice_play_volume_percent(), NULL, play_tiime);
	return 0;
}

static void sleep_aid_record_stop_capture(void)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();

	if (!c->record_capture_active && !divoom_local_music_ctrl_voice_clip_record_is_active())
	{
		return;
	}
	if (divoom_local_music_ctrl_voice_clip_record_is_active())
	{
		divoom_local_music_ctrl_voice_clip_record_stop_sync();
	}
	c->record_capture_active = 0;
}

static int sleep_aid_record_clip_bytes(uint32 record_id)
{
	char path[256];

	if (record_id == 0 || record_id > DIVOOM_SLEEP_AID_RECORD_MAX_SLOTS)
	{
		return 0;
	}
	divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_SLEEP_AID_RECORD, record_id, path);
	return divoom_app_com_get_file_size(path);
}

static int sleep_aid_record_clip_file_valid(uint32 record_id)
{
	if (sleep_aid_record_clip_bytes(record_id) < DIVOOM_SLEEP_AID_RECORD_MIN_BYTES)
	{
		return 0;
	}
	return 1;
}

static int sleep_aid_record_slot_file_valid(int slot)
{
	if (slot < 0 || slot >= DIVOOM_SLEEP_AID_RECORD_MAX_SLOTS)
	{
		return 0;
	}
	return sleep_aid_record_clip_file_valid((uint32)(slot + 1));
}

static void sleep_aid_record_ensure_index_dir(void)
{
	divoom_app_com_check_and_mk_dir((char *)DIVOOM_SD_SYSTEM_DIR);
}

static void sleep_aid_record_times_default(Pdivoom_sleep_aid_ctrl c)
{
	int i;

	if (!c)
	{
		return;
	}
	for (i = 0; i < DIVOOM_SLEEP_AID_RECORD_MAX_SLOTS; i++)
	{
		c->record_time[i] = 0;
	}
}

static int sleep_aid_record_times_prune(Pdivoom_sleep_aid_ctrl c)
{
	char path[256];
	int i;
	int changed;

	if (!c)
	{
		return 0;
	}
	changed = 0;
	for (i = 0; i < DIVOOM_SLEEP_AID_RECORD_MAX_SLOTS; i++)
	{
		if (c->record_time[i] == 0)
		{
			continue;
		}
		if (sleep_aid_record_slot_file_valid(i))
		{
			continue;
		}
		/* 仅当文件确实不存在时才剔除索引项，避免仿真/落盘瞬间 get_file_size 误 0 把刚提交的 clip 清掉 */
		divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_SLEEP_AID_RECORD,
							   (uint32)(i + 1), path);
		if (divoom_port_file_exists(path))
		{
			continue;
		}
		c->record_time[i] = 0;
		changed = 1;
	}
	return changed;
}

static int sleep_aid_record_times_rebuild_from_disk(Pdivoom_sleep_aid_ctrl c)
{
	char path[256];
	divoom_port_stat_t st;
	int i;
	int changed;

	if (!c)
	{
		return 0;
	}
	changed = 0;
	for (i = 0; i < DIVOOM_SLEEP_AID_RECORD_MAX_SLOTS; i++)
	{
		if (!sleep_aid_record_slot_file_valid(i))
		{
			if (c->record_time[i] != 0)
			{
				c->record_time[i] = 0;
				changed = 1;
			}
			continue;
		}
		if (c->record_time[i] != 0)
		{
			continue;
		}
		divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_SLEEP_AID_RECORD,
							   (uint32)(i + 1), path);
		if (divoom_port_stat(path, &st) == 0)
		{
			c->record_time[i] = (uint32)st.mtime;
			changed = 1;
		}
	}
	return changed;
}

static void sleep_aid_record_sort_slots(Pdivoom_sleep_aid_ctrl c, int order[5])
{
	int i;
	int j;

	if (!c || !order)
	{
		return;
	}
	for (i = 0; i < DIVOOM_SLEEP_AID_RECORD_MAX_SLOTS; i++)
	{
		order[i] = i;
	}
	for (i = 0; i < DIVOOM_SLEEP_AID_RECORD_MAX_SLOTS - 1; i++)
	{
		for (j = i + 1; j < DIVOOM_SLEEP_AID_RECORD_MAX_SLOTS; j++)
		{
			if (c->record_time[order[j]] > c->record_time[order[i]])
			{
				int tmp;

				tmp = order[i];
				order[i] = order[j];
				order[j] = tmp;
			}
		}
	}
}

static int sleep_aid_record_clip_count_internal(Pdivoom_sleep_aid_ctrl c)
{
	int i;
	int n;

	if (!c)
	{
		return 0;
	}
	n = 0;
	for (i = 0; i < DIVOOM_SLEEP_AID_RECORD_MAX_SLOTS; i++)
	{
		if (c->record_time[i] != 0 && sleep_aid_record_slot_file_valid(i))
		{
			n++;
		}
	}
	return n;
}

/** 选 record_time 最小的槽（相同时选索引小）；record_time[0] 对应 1.mp3 */
static int sleep_aid_record_pick_slot_min_time(Pdivoom_sleep_aid_ctrl c)
{
	int pick;
	int i;

	if (!c)
	{
		return 0;
	}
	pick = 0;
	for (i = 1; i < DIVOOM_SLEEP_AID_RECORD_MAX_SLOTS; i++)
	{
		if (c->record_time[i] < c->record_time[pick])
		{
			pick = i;
		}
	}
	return pick;
}

/* 完整读取主配置，失败时回退到同名出厂备份；调用方负责释放返回缓冲区。 */
static uint8 *sleep_aid_ctrl_read_file_with_backup(const char *path, const char *backup_path)
{
	uint8 *read_buf;

	read_buf = (uint8 *)divoom_port_file_read_all(path, NULL);
	if (!read_buf && backup_path)
	{
		read_buf = (uint8 *)divoom_port_file_read_all(backup_path, NULL);
	}
	return read_buf;
}

static int sleep_aid_record_times_load(Pdivoom_sleep_aid_ctrl c)
{
	uint8 *read_buf = NULL;
	divoom_cJSON *root = NULL;
	divoom_cJSON *arr = NULL;
	divoom_cJSON *it = NULL;
	int i;

	if (!c)
	{
		return -1;
	}
	if (c->record_times_loaded)
	{
		return 0;
	}

	c->record_times_loaded = 1;
	sleep_aid_record_times_default(c);

	read_buf = sleep_aid_ctrl_read_file_with_backup(DIVOOM_SLEEP_AID_RECORD_INDEX_PATH,
		DIVOOM_SLEEP_AID_RECORD_INDEX_BAK_PATH);
	if (!read_buf)
	{
		sleep_aid_record_times_rebuild_from_disk(c);
		return 0;
	}

	root = divoom_cJSON_Parse((char *)read_buf);
	divoom_system_free(read_buf);
	if (!root)
	{
		sleep_aid_record_times_rebuild_from_disk(c);
		return -1;
	}

	arr = divoom_cJSON_GetObjectItem(root, "record_time");
	if (!arr || !(arr->type & divoom_cJSON_Array))
	{
		divoom_cJSON_Delete(root);
		sleep_aid_record_times_rebuild_from_disk(c);
		return -1;
	}

	for (i = 0; i < DIVOOM_SLEEP_AID_RECORD_MAX_SLOTS; i++)
	{
		it = divoom_cJSON_GetArrayItem(arr, i);
		if (!it || !(it->type & divoom_cJSON_Number))
		{
			continue;
		}
		c->record_time[i] = (uint32)it->valueint;
	}

	divoom_cJSON_Delete(root);
	if (sleep_aid_record_times_prune(c))
	{
		sleep_aid_record_times_save(c);
	}
	return 0;
}

static int sleep_aid_record_times_save(Pdivoom_sleep_aid_ctrl c)
{
	divoom_cJSON *root;
	divoom_cJSON *arr;
	divoom_cJSON *num;
	char *txt;
	FILE *fp;
	int i;

	if (!c)
	{
		return -1;
	}

	root = divoom_cJSON_CreateObject();
	if (!root)
	{
		return -1;
	}
	arr = divoom_cJSON_CreateArray();
	if (!arr)
	{
		divoom_cJSON_Delete(root);
		return -1;
	}

	for (i = 0; i < DIVOOM_SLEEP_AID_RECORD_MAX_SLOTS; i++)
	{
		num = divoom_cJSON_CreateNumber((double)c->record_time[i]);
		if (!num)
		{
			divoom_cJSON_Delete(root);
			return -1;
		}
		divoom_cJSON_AddItemToArray(arr, num);
	}
	divoom_cJSON_AddItemToObject(root, "record_time", arr);

	txt = divoom_cJSON_Print(root);
	divoom_cJSON_Delete(root);
	if (!txt)
	{
		return -1;
	}

	sleep_aid_record_ensure_index_dir();
	fp = divoom_port_file_open(DIVOOM_SLEEP_AID_RECORD_INDEX_PATH, "w+");
	if (!fp)
	{
		divoom_system_free(txt);
		divoom_print("%s: open %s failed\n", __func__, DIVOOM_SLEEP_AID_RECORD_INDEX_PATH);
		return -1;
	}
	divoom_port_file_write(txt, strlen(txt), 1, fp);
	fflush(fp);
#if !defined(DIVOOM_SIMULATOR)
	(void)fsync(fileno(fp));
#endif
	divoom_port_file_close(fp);
	divoom_system_free(txt);
	return 0;
}

static int sleep_aid_record_temp_clip_bytes(void)
{
	return divoom_app_com_get_file_size((char *)DIVOOM_SLEEP_AID_RECORD_TMP_MP3);
}

static int sleep_aid_record_copy_file_bin(const char *src, const char *dest)
{
	FILE *fp_in;
	FILE *fp_out;
	uint8 *buf;
	size_t n;
	int ok;

	if (!src || !dest)
	{
		return -1;
	}

	ok = -1;
	buf = divoom_system_malloc(DIVOOM_PORT_WRITE_CHUNK);
	if (!buf)
	{
		return -1;
	}
	fp_in = divoom_port_file_open(src, "rb");
	if (!fp_in)
	{
		divoom_system_free(buf);
		return -1;
	}
	fp_out = divoom_port_file_open(dest, "wb");
	if (!fp_out)
	{
		divoom_port_file_close(fp_in);
		divoom_system_free(buf);
		return -1;
	}

	while (1)
	{
		n = divoom_port_file_read(buf, 1, DIVOOM_PORT_WRITE_CHUNK, fp_in);
		if (n == 0)
		{
			ok = 0;
			break;
		}
		if (divoom_port_file_write(buf, 1, n, fp_out) != n)
		{
			ok = -1;
			break;
		}
	}

	divoom_port_file_close(fp_in);
	divoom_port_file_close(fp_out);
	divoom_system_free(buf);
	return ok;
}

static int sleep_aid_record_commit_clip(Pdivoom_sleep_aid_ctrl c, int slot)
{
	char dirpath[160];
	char dest[256];
	uint32 file_id;
	int tmp_sz;
	int dest_sz;

	if (!c || slot < 0 || slot >= DIVOOM_SLEEP_AID_RECORD_MAX_SLOTS)
	{
		sleep_aid_record_stop_capture();
		divoom_app_com_del_file((char *)DIVOOM_SLEEP_AID_RECORD_TMP_MP3);
		return -1;
	}

	file_id = (uint32)(slot + 1);
	sleep_aid_record_stop_capture();

	tmp_sz = sleep_aid_record_temp_clip_bytes();
	if (tmp_sz < DIVOOM_SLEEP_AID_RECORD_MIN_BYTES)
	{
		divoom_print("%s: tmp too small slot=%d bytes=%d\n", __func__, slot, tmp_sz);
		divoom_app_com_del_file((char *)DIVOOM_SLEEP_AID_RECORD_TMP_MP3);
		return -1;
	}

	divoom_manage_image_download_get_path_name(DIVOOM_MANAGE_IMAGE_SLEEP_AID_RECORD, 0, dirpath);
	divoom_app_com_check_and_mk_dir(dirpath);
	divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_SLEEP_AID_RECORD, file_id, dest);

	if (sleep_aid_record_copy_file_bin((char *)DIVOOM_SLEEP_AID_RECORD_TMP_MP3, dest) != 0)
	{
		divoom_print("%s: copy failed tmp->%s\n", __func__, dest);
		divoom_app_com_del_file((char *)DIVOOM_SLEEP_AID_RECORD_TMP_MP3);
		return -1;
	}
	divoom_app_com_del_file((char *)DIVOOM_SLEEP_AID_RECORD_TMP_MP3);

	if (!divoom_port_file_exists(dest))
	{
		divoom_print("%s: dest missing %s\n", __func__, dest);
		return -1;
	}
	dest_sz = sleep_aid_record_clip_bytes(file_id);
	if (dest_sz < DIVOOM_SLEEP_AID_RECORD_MIN_BYTES)
	{
		divoom_print("%s: dest too small id=%u bytes=%d\n", __func__, (unsigned)file_id, dest_sz);
		divoom_app_com_del_file(dest);
		return -1;
	}

	c->record_time[slot] = (uint32)time(0);

	if (sleep_aid_record_times_save(c) != 0)
	{
		divoom_print("%s: index save failed slot=%d\n", __func__, slot);
		return -1;
	}

	divoom_print("%s: OK slot=%d file_id=%u bytes=%d\n", __func__, slot, (unsigned)file_id, dest_sz);
	c->record_times_loaded = 1;
	return 0;
}

static void sleep_aid_record_done_cb(const char *path, uint32 duration_sec, int result)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();

	(void)path;
	(void)result;
	if (!c)
	{
		return;
	}
	c->record_enc_duration_sec = duration_sec > 0u ? duration_sec : 1u;
}

static int sleep_aid_record_start_capture(void)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();

	if (c->record_pending_slot < 0
	    || c->record_pending_slot >= DIVOOM_SLEEP_AID_RECORD_MAX_SLOTS)
	{
		return -1;
	}

	divoom_local_music_ctrl_voice_clip_record_stop_sync();
	sleep_aid_record_stop_capture();
	divoom_app_com_del_file((char *)DIVOOM_SLEEP_AID_RECORD_TMP_MP3);

	c->record_enc_duration_sec = 0;
	if (divoom_local_music_ctrl_voice_clip_record_start(DIVOOM_SLEEP_AID_RECORD_TMP_MP3, 0,
							    sleep_aid_record_done_cb) != 0)
	{
		divoom_print("%s: voice_clip_record_start failed\n", __func__);
		return -1;
	}
	c->record_capture_active = 1;
	return 0;
}

static uint32 sleep_aid_schedule_push_voice(uint8 event_id, void *user_data)
{
	divoom_app_schedule_item_info ev = {0};
	uint32 res;

	ev.model_id = DIVOOM_MODEL_SLEEP_AID;
	ev.event_id = event_id;
	ev.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
	ev.resource_type = DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE;
	ev.user_data = user_data;
	res = divoom_app_schedule_push(&ev);
	if (res != 0)
	{
		if (divoom_sleep_aid_ctrl_app_schedule_play(event_id, user_data, res) == 0)
		{
			ev.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
			divoom_app_schedule_cancel(&ev);
			return 0;
		}
	}
	return res;
}

static uint32 sleep_aid_voice_schedule_request_play(void *user_data)
{
	return sleep_aid_schedule_push_voice(DIVOOM_SLEEP_AID_APP_EVENT_PLAY, user_data);
}

static void sleep_aid_voice_schedule_cancel_play(void)
{
	divoom_app_schedule_item_info ev = {0};

	ev.model_id = DIVOOM_MODEL_SLEEP_AID;
	ev.event_id = DIVOOM_SLEEP_AID_APP_EVENT_PLAY;
	ev.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
	ev.resource_type = DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE;
	divoom_app_schedule_cancel(&ev);
}

static uint32 sleep_aid_record_schedule_request(void)
{
	return sleep_aid_schedule_push_voice(DIVOOM_SLEEP_AID_APP_EVENT_RECORD, NULL);
}

static void sleep_aid_record_schedule_cancel(void)
{
	divoom_app_schedule_item_info ev = {0};

	ev.model_id = DIVOOM_MODEL_SLEEP_AID;
	ev.event_id = DIVOOM_SLEEP_AID_APP_EVENT_RECORD;
	ev.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
	ev.resource_type = DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE;
	divoom_app_schedule_cancel(&ev);
}

static uint32 sleep_aid_app_preview_schedule_begin(void)
{
	return sleep_aid_schedule_push_voice(DIVOOM_SLEEP_AID_APP_EVENT_TRY_LISTEN, NULL);
}

static void sleep_aid_app_preview_schedule_end(void)
{
	divoom_app_schedule_item_info ev = {0};

	ev.model_id = DIVOOM_MODEL_SLEEP_AID;
	ev.event_id = DIVOOM_SLEEP_AID_APP_EVENT_TRY_LISTEN;
	ev.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
	ev.resource_type = DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE;
	divoom_app_schedule_cancel(&ev);
}

/** @p cancel_schedule 为 0 时供 schedule destroy 使用，避免重入 cancel。 */
static void sleep_aid_app_preview_teardown(uint8 cancel_schedule)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();

	if (c)
	{
		c->app_aid_sleep_preview_active = 0;
	}
	if (cancel_schedule)
	{
		sleep_aid_app_preview_schedule_end();
	}
	sleep_aid_job_post(SLEEP_AID_JOB_STOP_VOICE, 0, 0, 0);
}

static int sleep_aid_record_ctrl_exec(int flag)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();
	int pending_slot;
	int sched_res;
	int ret;

	if (flag)
	{
		c->record_times_loaded = 0;
		sleep_aid_record_times_load(c);

		if (c->record_active)
		{
			return -2;
		}

		pending_slot = sleep_aid_record_pick_slot_min_time(c);
		c->record_pending_slot = pending_slot;
		c->record_schedule_armed = 1;
		sched_res = (int)sleep_aid_record_schedule_request();
		/* schedule push 内部 destroy 旧事件时会清 pending；此处恢复 */
		c->record_pending_slot = pending_slot;

		if (sched_res == 0)
		{
			c->record_schedule_armed = 0;
			c->record_pending_slot = -1;
			return -5;
		}

		if (!c->record_active && !c->record_capture_active)
		{
			c->record_schedule_armed = 1;
			c->record_pending_slot = pending_slot;
			if (sleep_aid_record_start_capture() != 0)
			{
				c->record_schedule_armed = 0;
				c->record_pending_slot = -1;
				sleep_aid_record_schedule_cancel();
				return -3;
			}
			c->record_active = 1;
			c->record_schedule_armed = 0;
		}

		return c->record_pending_slot;
	}

	if (!c->record_active && !c->record_capture_active
	    && !divoom_local_music_ctrl_voice_clip_record_is_active())
	{
		return -1;
	}

	pending_slot = c->record_pending_slot;
	if (pending_slot < 0 || pending_slot >= DIVOOM_SLEEP_AID_RECORD_MAX_SLOTS)
	{
		if (c->record_active || c->record_capture_active
		    || divoom_local_music_ctrl_voice_clip_record_is_active())
		{
			pending_slot = sleep_aid_record_pick_slot_min_time(c);
			c->record_pending_slot = pending_slot;
		}
	}

	if (pending_slot < 0 || pending_slot >= DIVOOM_SLEEP_AID_RECORD_MAX_SLOTS)
	{
		c->record_active = 0;
		c->record_pending_slot = -1;
		sleep_aid_record_stop_capture();
		divoom_app_com_del_file((char *)DIVOOM_SLEEP_AID_RECORD_TMP_MP3);
		sleep_aid_record_schedule_cancel();
		return -4;
	}

	ret = sleep_aid_record_commit_clip(c, pending_slot);
	c->record_active = 0;
	c->record_pending_slot = -1;

	if (ret != 0)
	{
		divoom_print("%s: commit slot=%d failed\n", __func__, pending_slot);
		sleep_aid_record_schedule_cancel();
		return -4;
	}

	sleep_aid_record_schedule_cancel();
	return 0;
}

uint8 divoom_sleep_aid_ctrl_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();
	Pdivoom_sleep_aid_active_enter ent = (Pdivoom_sleep_aid_active_enter)user_data;

	(void)resouce_type;

	switch (event_id)
	{
	case DIVOOM_SLEEP_AID_APP_EVENT_PLAY:
	{
		if (ent)
		{
			c->sel_voice_type = ent->voice_type;
			c->sel_voice_id = ent->voice_id;
			/* 先让 worker 停掉选择页试听；107 的 PLAY 任务随后入队。 */
			divoom_sleep_aid_ctrl_stop_play_voice();
			/* 播放页内存占用较大，进入前清空整条旧菜单栈。 */
			divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_SLEEP_AID_ACTIVE, ent, 0, 0);
		}
		c->play_session_active = 1;
		sleep_aid_play_phase_begin(c);
		return 1;
	}
	case DIVOOM_SLEEP_AID_APP_EVENT_RECORD:
	{
		if (c->record_schedule_armed)
		{
			if (sleep_aid_record_start_capture() != 0)
			{
				c->record_schedule_armed = 0;
				c->record_pending_slot = -1;
				return 0;
			}
			c->record_active = 1;
			c->record_schedule_armed = 0;
		}
		return 1;
	}
	case DIVOOM_SLEEP_AID_APP_EVENT_TRY_LISTEN:
	{
		if (c)
		{
			c->app_aid_sleep_preview_active = 1;
		}
		return 1;
	}
	default:
	{
		break;
	}
	}
	return 1;
}

uint8 divoom_sleep_aid_ctrl_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type)
{
	(void)user_data;
	(void)active_model;
	(void)active_event;
	(void)resouce_type;

	if (event_id == DIVOOM_SLEEP_AID_APP_EVENT_RECORD)
	{
		sleep_aid_record_stop_capture();
	}
	return 0;
}

uint8 divoom_sleep_aid_ctrl_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();

	(void)user_data;
	(void)status;
	(void)resouce_type;

	switch (event_id)
	{
	case DIVOOM_SLEEP_AID_APP_EVENT_PLAY:
	{
		sleep_aid_play_phase_stop(c);
		c->play_session_active = 0;
		sleep_aid_restore_sys_light(c);
		/* destroy 常由 UI 线程触发，关 mix_pcm/snd_pcm 不能就地做；
		 * session_stop 幂等，后续 app 抢音轨由 audio_session 兜底 */
		sleep_aid_job_post(SLEEP_AID_JOB_STOP_VOICE, 0, 0, 0);
		return 1;
	}
	case DIVOOM_SLEEP_AID_APP_EVENT_RECORD:
	{
		sleep_aid_record_stop_capture();
		/* 临时 MP3 由 commit_clip 删除；此处勿删，避免 cancel 与提交竞态 */
		c->record_active = 0;
		c->record_pending_slot = -1;
		c->record_schedule_armed = 0;
		return 1;
	}
	case DIVOOM_SLEEP_AID_APP_EVENT_TRY_LISTEN:
	{
		/* schedule 抢占销毁：勿 cancel TRY_LISTEN（框架正在 pop），须停播 */
		sleep_aid_app_preview_teardown(0);
		return 1;
	}
	default:
	{
		break;
	}
	}
	return 1;
}

uint8 divoom_sleep_aid_ctrl_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type)
{
	(void)key_type;
	(void)resouce_type;

	if (!divoom_sleep_aid_ctrl_app_preview_is_active())
	{
		return 0;
	}
	/* VOL± 由 divoom_lvgl_top_bright_volume_menu 调 sleep before_vol，不退出试听 */
	if (key_id == DIVOOM_KEY_LIGHT_LEFT || key_id == DIVOOM_KEY_LIGHT_RIGHT)
	{
		return 0;
	}
	divoom_sleep_aid_ctrl_app_preview_stop_by_user_key();
	return 1;
}

uint8 divoom_sleep_aid_ctrl_event(Pdivoom_sys_msg_info msg)
{
	if (!msg)
	{
		return 0;
	}

	switch (msg->msg_id)
	{
	case DIVOOM_SLEEP_AID_MSG_ENTER_PLAY_VOICE:
	{
		sleep_aid_msg_voice_pair_t *p;
		Pdivoom_sleep_aid_ctrl c;
		Pdivoom_sleep_aid_active_enter ent;

		if (!msg->data || msg->data_len < sizeof(sleep_aid_msg_voice_pair_t))
		{
			break;
		}
		p = (sleep_aid_msg_voice_pair_t *)msg->data;
		c = divoom_sleep_aid_ctrl_instance();
		c->sel_voice_type = p->voice_type;
		c->sel_voice_id = p->voice_id;
		sleep_aid_persist_last_play_voice(c, p->voice_type, p->voice_id);

		ent = (Pdivoom_sleep_aid_active_enter)divoom_system_malloc(sizeof(divoom_sleep_aid_active_enter));
		if (!ent)
		{
			break;
		}
		ent->voice_type = p->voice_type;
		ent->voice_id = p->voice_id;
		ent->video_type = divoom_sleep_aid_voice_get_video_type(p->voice_type, (uint32)p->voice_id);
		if (sleep_aid_voice_schedule_request_play(ent) == 0)
		{
			divoom_system_free(ent);
		}
		else
		{
			divoom_print("sleep_aid_phase: enter play scheduled type=%d id=%d\n",
				     p->voice_type, p->voice_id);
		}
		break;
	}
	case DIVOOM_SLEEP_AID_MSG_RECORD_CTRL:
	{
		int fl;

		if (!msg->data || msg->data_len < sizeof(int))
		{
			msg->result = -1;
			break;
		}
		fl = *(int *)msg->data;
		msg->result = sleep_aid_record_ctrl_exec(fl);
		break;
	}
	case DIVOOM_SLEEP_AID_MSG_RECORD_DELETE:
	{
		int idx;
		int slot;
		Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();
		uint32 rid;

		if (!msg->data || msg->data_len < sizeof(int))
		{
			msg->result = -1;
			break;
		}
		idx = *(int *)msg->data;
		sleep_aid_record_times_load(c);
		slot = divoom_sleep_aid_ctrl_record_sorted_slot(idx);
		if (slot < 0 || slot >= DIVOOM_SLEEP_AID_RECORD_MAX_SLOTS)
		{
			msg->result = -1;
			break;
		}

		rid = (uint32)(slot + 1);
		c->record_time[slot] = 0;
		divoom_manage_image_del(DIVOOM_MANAGE_IMAGE_SLEEP_AID_RECORD, &rid, 1, 0);

		if (sleep_aid_record_times_save(c) != 0)
		{
			msg->result = -1;
			break;
		}
		msg->result = 0;
		break;
	}
	default:
	{
		break;
	}
	}

	return 1;
}

void divoom_sleep_aid_ctrl_init(void)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();

	sleep_aid_ctrl_ensure_settings_init(c);
	c->record_pending_slot = -1;
	sleep_aid_record_times_load(c);
	sleep_aid_job_worker_start();
	divoom_audio_session_set_sleep_stop_handlers(sleep_aid_pcm_stop_async,
						     sleep_aid_pcm_stop_wait);
	divoom_sys_model_crl_register(DIVOOM_MODEL_SLEEP_AID, divoom_sleep_aid_ctrl_event);
}

void divoom_sleep_aid_ctrl_main_loop(void)
{
	Pdivoom_sleep_aid_ctrl c;
	time_t now;
	uint32 now_u;

	if (divoom_onoff_is_poweroff_active() || divoom_product_test_is_active())
	{
		return;
	}
	c = divoom_sleep_aid_ctrl_instance();
	if (!c || !c->play_session_active || c->play_phase_deadline == 0)
	{
		return;
	}

	time(&now);
	now_u = (uint32)(unsigned long)now;
	if (now_u < c->play_phase_deadline)
	{
		return;
	}

	c->play_phase_deadline = 0;
	divoom_print("sleep_aid_phase: main_loop deadline reached phase=%u\n", (unsigned)c->play_phase);

	if (c->play_phase == SLEEP_AID_PLAY_PHASE_BEFORE)
	{
		sleep_aid_play_phase_on_before_done(c);
	}
	else if (c->play_phase == SLEEP_AID_PLAY_PHASE_AFTER)
	{
		sleep_aid_play_phase_on_after_done(c);
	}
}

int divoom_sleep_aid_ctrl_get_play_time_phase(void)
{
	Pdivoom_sleep_aid_ctrl c;

	c = divoom_sleep_aid_ctrl_instance();
	if (!c || !c->play_session_active)
	{
		return DIVOOM_SLEEP_AID_PLAY_TIME_PHASE_NORMAL;
	}
	if (c->play_phase == SLEEP_AID_PLAY_PHASE_AFTER)
	{
		return DIVOOM_SLEEP_AID_PLAY_TIME_PHASE_AFTER;
	}
	if (c->play_phase == SLEEP_AID_PLAY_PHASE_BEFORE)
	{
		return DIVOOM_SLEEP_AID_PLAY_TIME_PHASE_BEFORE;
	}
	return DIVOOM_SLEEP_AID_PLAY_TIME_PHASE_NORMAL;
}

int divoom_sleep_aid_ctrl_get_phase_remain_sec(void)
{
	Pdivoom_sleep_aid_ctrl c;
	divoom_sleep_aid_info *inf;
	time_t now;
	uint32 now_u;

	c = divoom_sleep_aid_ctrl_instance();
	if (!c || !c->play_session_active)
	{
		return -1;
	}
	sleep_aid_ctrl_ensure_settings_init(c);
	inf = &c->settings;
	if (c->play_phase == SLEEP_AID_PLAY_PHASE_BEFORE)
	{
#if DIVOOM_SLEEP_AID_PHASE_TEST_SEC <= 0
		if (inf->before_time <= 0 || c->play_phase_deadline == 0)
		{
			return -1;
		}
#else
		if (c->play_phase_deadline == 0)
		{
			return -1;
		}
#endif
	}
	else if (c->play_phase == SLEEP_AID_PLAY_PHASE_AFTER)
	{
#if DIVOOM_SLEEP_AID_PHASE_TEST_SEC <= 0
		if (!inf->after_on_off || inf->after_time <= 0 || c->play_phase_deadline == 0)
		{
			return -1;
		}
#else
		if (!inf->after_on_off || c->play_phase_deadline == 0)
		{
			return -1;
		}
#endif
	}
	else
	{
		return -1;
	}
	time(&now);
	now_u = (uint32)(unsigned long)now;
	if (now_u >= c->play_phase_deadline)
	{
		return 0;
	}
	return (int)(c->play_phase_deadline - now_u);
}

void divoom_sleep_aid_ctrl_set_before_time_minutes(int minutes)
{
	Pdivoom_sleep_aid_ctrl c;
	divoom_sleep_aid_info *inf;

	c = divoom_sleep_aid_ctrl_instance();
	if (!c)
	{
		return;
	}
	if (minutes < 0)
	{
		minutes = 0;
	}
	sleep_aid_ctrl_ensure_settings_init(c);
	inf = &c->settings;
	inf->before_time = minutes;
	sleep_aid_settings_save(c);
	if (c->play_session_active && c->play_phase != SLEEP_AID_PLAY_PHASE_AFTER)
	{
		if (c->play_phase != SLEEP_AID_PLAY_PHASE_BEFORE)
		{
			c->play_phase = SLEEP_AID_PLAY_PHASE_BEFORE;
		}
		sleep_aid_play_phase_set_deadline_minutes(c, minutes);
	}
}

void divoom_sleep_aid_ctrl_set_after_time_minutes(int minutes)
{
	Pdivoom_sleep_aid_ctrl c;
	divoom_sleep_aid_info *inf;

	c = divoom_sleep_aid_ctrl_instance();
	if (!c)
	{
		return;
	}
	if (minutes < 0)
	{
		minutes = 0;
	}
	sleep_aid_ctrl_ensure_settings_init(c);
	inf = &c->settings;
	inf->after_time = minutes;
	sleep_aid_settings_save(c);
	if (c->play_session_active && c->play_phase == SLEEP_AID_PLAY_PHASE_AFTER)
	{
		sleep_aid_play_phase_set_deadline_minutes(c, minutes);
	}
}

void divoom_sleep_aid_ctrl_set_phase_timer_minutes(int minutes)
{
	if (divoom_sleep_aid_ctrl_get_play_time_phase() == DIVOOM_SLEEP_AID_PLAY_TIME_PHASE_AFTER)
	{
		divoom_sleep_aid_ctrl_set_after_time_minutes(minutes);
		return;
	}
	divoom_sleep_aid_ctrl_set_before_time_minutes(minutes);
}

static void sleep_aid_settings_apply_default(divoom_sleep_aid_info *inf)
{
	int i;

	memset(inf, 0, sizeof(*inf));
	inf->before_time = 30;
	inf->before_vol = 50;
	inf->before_bright = 17;
	inf->after_on_off = 0;
	inf->after_vol = 34;
	inf->after_music = 0;
	inf->after_time = 0;
	inf->after_bright = 0;
	for (i = 0; i < 8; i++)
	{
		inf->volume[i] = 0;
	}
	inf->last_voice_type = 0;
	inf->last_voice_id = 0;
}

static void sleep_aid_settings_clamp(divoom_sleep_aid_info *inf)
{
	int i;

	if (!inf)
	{
		return;
	}
	if (inf->before_vol < 0)
	{
		inf->before_vol = 0;
	}
	if (inf->before_vol > SLEEP_AID_SETT_VOL_MAX)
	{
		inf->before_vol = SLEEP_AID_SETT_VOL_MAX;
	}
	if (inf->before_bright < 0)
	{
		inf->before_bright = 0;
	}
	if (inf->before_bright > SLEEP_AID_SETT_BRIGHT_MAX)
	{
		inf->before_bright = SLEEP_AID_SETT_BRIGHT_MAX;
	}
	if (inf->after_vol < 0)
	{
		inf->after_vol = 0;
	}
	if (inf->after_vol > SLEEP_AID_SETT_VOL_MAX)
	{
		inf->after_vol = SLEEP_AID_SETT_VOL_MAX;
	}
	if (inf->after_bright < 0)
	{
		inf->after_bright = 0;
	}
	if (inf->after_bright > SLEEP_AID_SETT_BRIGHT_MAX)
	{
		inf->after_bright = SLEEP_AID_SETT_BRIGHT_MAX;
	}
	if (inf->after_on_off)
	{
		inf->after_on_off = 1;
	}
	else
	{
		inf->after_on_off = 0;
	}
	if (inf->after_music < 0)
	{
		inf->after_music = 0;
	}
	if (inf->after_music > 2)
	{
		inf->after_music = 2;
	}
	if (inf->before_time < 0)
	{
		inf->before_time = 0;
	}
	if (inf->after_time < 0)
	{
		inf->after_time = 0;
	}
	for (i = 0; i < 8; i++)
	{
		if (inf->volume[i] < 0)
		{
			inf->volume[i] = 0;
		}
		if (inf->volume[i] > SLEEP_AID_SETT_VOL_MAX)
		{
			inf->volume[i] = SLEEP_AID_SETT_VOL_MAX;
		}
	}
}

static int sleep_aid_settings_load(Pdivoom_sleep_aid_ctrl c)
{
	uint8 *read_buf;
	divoom_cJSON *root;
	divoom_cJSON *vol_arr;
	divoom_sleep_aid_info *inf;
	int i;
	int n;

	if (!c)
	{
		return -1;
	}
	inf = &c->settings;
	sleep_aid_settings_apply_default(inf);

	read_buf = sleep_aid_ctrl_read_file_with_backup(DIVOOM_SLEEP_AID_SETTINGS_PATH,
		DIVOOM_SLEEP_AID_SETTINGS_BAK_PATH);
	if (!read_buf)
	{
		return -1;
	}

	root = divoom_cJSON_Parse((char *)read_buf);
	divoom_system_free(read_buf);
	if (!root)
	{
		return -1;
	}

	inf->before_time = divoom_cJSON_GetObjectInt(root, "BeforeTime");
	inf->before_vol = divoom_cJSON_GetObjectInt(root, "BeforeVol");
	inf->before_bright = divoom_cJSON_GetObjectInt(root, "BeforeBright");
	inf->after_on_off = divoom_cJSON_GetObjectInt(root, "AfterOnOff");
	inf->after_vol = divoom_cJSON_GetObjectInt(root, "AfterVol");
	inf->after_music = divoom_cJSON_GetObjectInt(root, "AfterMusic");
	inf->after_time = divoom_cJSON_GetObjectInt(root, "AfterTime");
	inf->after_bright = divoom_cJSON_GetObjectInt(root, "AfterBright");
	inf->last_voice_type = divoom_cJSON_GetObjectInt(root, "LastVoiceType");
	inf->last_voice_id = divoom_cJSON_GetObjectInt(root, "LastVoiceId");

	vol_arr = divoom_cJSON_GetObjectItem(root, "Volume");
	if (vol_arr && (vol_arr->type & 0xFF) == divoom_cJSON_Array)
	{
		n = divoom_cJSON_GetArraySize(vol_arr);
		for (i = 0; i < n && i < 8; i++)
		{
			divoom_cJSON *it = divoom_cJSON_GetArrayItem(vol_arr, i);

			if (it)
			{
				inf->volume[i] = it->valueint;
			}
		}
	}

	divoom_cJSON_Delete(root);
	sleep_aid_settings_clamp(inf);
	return 0;
}

static int sleep_aid_settings_save(Pdivoom_sleep_aid_ctrl c)
{
	divoom_cJSON *root;
	divoom_cJSON *vol_arr;
	divoom_sleep_aid_info *inf;
	char *txt;
	FILE *fp;

	if (!c)
	{
		return -1;
	}
	inf = &c->settings;
	sleep_aid_settings_clamp(inf);

	root = divoom_cJSON_CreateObject();
	if (!root)
	{
		return -1;
	}

	divoom_cJSON_AddNumberToObject(root, "BeforeTime", inf->before_time);
	divoom_cJSON_AddNumberToObject(root, "BeforeVol", inf->before_vol);
	divoom_cJSON_AddNumberToObject(root, "BeforeBright", inf->before_bright);
	divoom_cJSON_AddNumberToObject(root, "AfterOnOff", inf->after_on_off);
	divoom_cJSON_AddNumberToObject(root, "AfterVol", inf->after_vol);
	divoom_cJSON_AddNumberToObject(root, "AfterMusic", inf->after_music);
	divoom_cJSON_AddNumberToObject(root, "AfterTime", inf->after_time);
	divoom_cJSON_AddNumberToObject(root, "AfterBright", inf->after_bright);
	divoom_cJSON_AddNumberToObject(root, "LastVoiceType", inf->last_voice_type);
	divoom_cJSON_AddNumberToObject(root, "LastVoiceId", inf->last_voice_id);
	vol_arr = divoom_cJSON_CreateIntArray(inf->volume, 8);
	if (!vol_arr)
	{
		divoom_cJSON_Delete(root);
		return -1;
	}
	divoom_cJSON_AddItemToObject(root, "Volume", vol_arr);

	txt = divoom_cJSON_Print(root);
	divoom_cJSON_Delete(root);
	if (!txt)
	{
		return -1;
	}

	fp = divoom_port_file_open(DIVOOM_SLEEP_AID_SETTINGS_PATH, "w+");
	if (!fp)
	{
		divoom_system_free(txt);
		divoom_print("%s: open %s failed\n", __func__, DIVOOM_SLEEP_AID_SETTINGS_PATH);
		return -1;
	}
	divoom_port_file_write(txt, strlen(txt), 1, fp);
	divoom_port_file_close(fp);
	divoom_system_free(txt);
	return 0;
}

static void sleep_aid_ctrl_ensure_settings_init(Pdivoom_sleep_aid_ctrl c)
{
	if (!c)
	{
		return;
	}
	if (!c->settings_inited)
	{
		if (sleep_aid_settings_load(c) != 0)
		{
			sleep_aid_settings_apply_default(&c->settings);
			c->settings_inited = 1;
			sleep_aid_settings_save(c);
		}
		else
		{
			c->settings_inited = 1;
		}
	}
}

int divoom_sleep_aid_ctrl_settings_save(void)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();

	sleep_aid_ctrl_ensure_settings_init(c);
	return sleep_aid_settings_save(c);
}

Pdivoom_sleep_aid_ctrl divoom_sleep_aid_ctrl_instance(void)
{
	return &s_sleep_aid_ctrl;
}

divoom_sleep_aid_info *divoom_sleep_aid_ctrl_get_settings(void)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();

	sleep_aid_ctrl_ensure_settings_init(c);
	return &c->settings;
}

void divoom_sleep_aid_ctrl_get_selected_voice(int *voice_type, int *voice_id)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();

	if (voice_type)
	{
		*voice_type = c->sel_voice_type;
	}
	if (voice_id)
	{
		*voice_id = c->sel_voice_id;
	}
}

void divoom_sleep_aid_ctrl_record_invalidate_manifest(void)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();

	c->record_times_loaded = 0;
}

int divoom_sleep_aid_ctrl_record_clip_count(void)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();
	int n;

	if (!c)
	{
		return 0;
	}

	sleep_aid_record_times_load(c);
	n = sleep_aid_record_clip_count_internal(c);
	if (n > 0)
	{
		return n;
	}

	/* commit 后若 manifest 被 reload/prune 误清，仍可从 mp3 文件恢复 */
	if (sleep_aid_record_times_rebuild_from_disk(c) != 0)
	{
		n = sleep_aid_record_clip_count_internal(c);
	}
#if defined(DIVOOM_SIMULATOR)
	if (n == 0)
	{
		divoom_print("sleep_aid clip_count=0 rt0=%u valid0=%d\n",
			     (unsigned)c->record_time[0],
			     sleep_aid_record_slot_file_valid(0));
	}
#endif
	return n;
}

int divoom_sleep_aid_ctrl_record_sorted_slot(int sorted_row)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();
	int order[DIVOOM_SLEEP_AID_RECORD_MAX_SLOTS];
	int i;
	int n;

	if (sorted_row < 0)
	{
		return -1;
	}

	sleep_aid_record_times_load(c);
	sleep_aid_record_sort_slots(c, order);
	n = 0;
	for (i = 0; i < DIVOOM_SLEEP_AID_RECORD_MAX_SLOTS; i++)
	{
		int slot;

		slot = order[i];
		if (c->record_time[slot] != 0 && sleep_aid_record_slot_file_valid(slot))
		{
			if (n == sorted_row)
			{
				return slot;
			}
			n++;
		}
	}
	return -1;
}

uint32 divoom_sleep_aid_ctrl_record_slot_utc(int slot)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();

	if (slot < 0 || slot >= DIVOOM_SLEEP_AID_RECORD_MAX_SLOTS)
	{
		return 0;
	}
	sleep_aid_record_times_load(c);
	return c->record_time[slot];
}

int divoom_sleep_aid_ctrl_record_clip_id(int sorted_row)
{
	int slot;

	slot = divoom_sleep_aid_ctrl_record_sorted_slot(sorted_row);
	if (slot < 0)
	{
		return -1;
	}
	return (int)(DIVOOM_VOICE_ID_SLEEP_AID_RECORD_BASE + slot + 1);
}

uint8 divoom_sleep_aid_ctrl_record_busy(void)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();

	if (!c)
	{
		return (uint8)0;
	}
	if (c->record_active || c->record_capture_active)
	{
		return (uint8)1;
	}
	if (divoom_local_music_ctrl_voice_clip_record_is_active())
	{
		return (uint8)1;
	}
	return (uint8)0;
}

uint32 divoom_sleep_aid_ctrl_record_enc_duration_sec(void)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();

	return c ? c->record_enc_duration_sec : 0u;
}

/*
 * 助眠音频慢操作串行 worker。
 *
 * 起停助眠音源要等提示音收尾（divoom_audio_session_wait_tone_idle 最长 2s），还要关掉
 * mix_pcm / snd_pcm，这些都不能在 LVGL 线程里同步做——整个等待期间界面既不刷新也不响应
 * 按键。系统 model 消息队列不适合承接：msg_type=0 是在调用者线程直接回调，msg_type=1 又
 * 由两个 worker 并发消费，起停会互相穿插。所以这里用单线程 + 单队列，保证串行且不占 UI。
 */
static sleep_aid_job_t s_sleep_aid_job_ring[SLEEP_AID_JOB_QUEUE_LEN];
static unsigned int s_sleep_aid_job_head;
static unsigned int s_sleep_aid_job_tail;
static void *s_sleep_aid_job_lock;
static void *s_sleep_aid_job_sem;
static void *s_sleep_aid_job_task;
static volatile int s_sleep_aid_pcm_stop_pending;
static volatile int s_sleep_aid_play_job_active;
static volatile unsigned int s_sleep_aid_play_generation;

static void sleep_aid_wait_play_video_stopped(unsigned int timeout_ms)
{
	unsigned int start_ms = divoom_system_get_tickms();
	unsigned int elapsed_ms;

	while (s_sleep_aid_play_job_active)
	{
		if (divoom_system_get_tickms() - start_ms >= timeout_ms)
		{
			divoom_print("sleep_aid: wait play job stopped timeout\n");
			return;
		}
		divoom_system_delay_ms(10);
	}
	elapsed_ms = divoom_system_get_tickms() - start_ms;
	if (elapsed_ms < timeout_ms)
	{
		(void)divoom_local_movie_play_wait_stopped(timeout_ms - elapsed_ms);
	}
}

static void sleep_aid_job_exec(const sleep_aid_job_t *job)
{
	Pdivoom_sleep_aid_ctrl c;

	if (!job)
	{
		return;
	}
	switch (job->kind)
	{
	case SLEEP_AID_JOB_SELECT_VOICE:
		c = divoom_sleep_aid_ctrl_instance();
		if (c)
		{
			c->settings_preview_vol_active = 0;
			c->settings_preview_vol_is_after = 0;
		}
		sleep_aid_voice_start_playback(job->arg0, (uint32)job->arg1,
					       DIVOOM_SLEEP_AID_VOICE_PLAY_SEC);
		break;
	case SLEEP_AID_JOB_PLAY_VOICE_VIDEO:
		s_sleep_aid_play_job_active = 1;
		(void)divoom_audio_session_wait_tone_idle(2000);
		c = divoom_sleep_aid_ctrl_instance();
		if (!c || !c->play_session_active
		    || job->play_generation != s_sleep_aid_play_generation)
		{
			s_sleep_aid_play_job_active = 0;
			break;
		}
		sleep_aid_voice_start_play_video(job->arg0, (uint32)job->arg1, job->arg2);
		(void)divoom_local_movie_play_wait_rendering(2000);
		if (!c->play_session_active
		    || job->play_generation != s_sleep_aid_play_generation)
		{
			divoom_local_movie_play_stop_movie();
			s_sleep_aid_play_job_active = 0;
			break;
		}
		sleep_aid_voice_start_playback(job->arg0, (uint32)job->arg1, U32_MAX / 2);
		s_sleep_aid_play_job_active = 0;
		break;
	case SLEEP_AID_JOB_STOP_VOICE:
		sleep_aid_voice_stop_playback();
		divoom_local_movie_play_stop_movie();
		s_sleep_aid_pcm_stop_pending = 0;
		break;
	case SLEEP_AID_JOB_VOICE_PREVIEW:
		sleep_aid_voice_stop_playback();
		divoom_local_movie_play_stop_movie();
		sleep_aid_voice_start_playback(job->arg0, (uint32)job->arg1,
					       DIVOOM_SLEEP_AID_VOICE_PLAY_SEC);
		divoom_sleep_aid_ctrl_set_voice_vol(job->arg2);
		s_sleep_aid_pcm_stop_pending = 0;
		break;
	case SLEEP_AID_JOB_AFTER_MUSIC_PREVIEW:
		sleep_aid_settings_after_voice_preview(job->arg0, job->arg1);
		break;
	case SLEEP_AID_JOB_AFTER_TONE_PLAY:
		sleep_aid_after_tone_play(job->arg0, job->arg1);
		break;
	case SLEEP_AID_JOB_PCM_STOP:
		divoom_sleep_pcm_session_stop();
		s_sleep_aid_pcm_stop_pending = 0;
		break;
	case SLEEP_AID_JOB_CANCEL_PREVIEW_SCHEDULE:
		sleep_aid_app_preview_schedule_end();
		break;
	default:
		break;
	}
}

static void *sleep_aid_job_worker(void *param)
{
	(void)param;

	for (;;)
	{
		sleep_aid_job_t job;
		int got = 0;

		divoom_system_get_sem(s_sleep_aid_job_sem);
		divoom_system_get_lock(s_sleep_aid_job_lock);
		if (s_sleep_aid_job_head != s_sleep_aid_job_tail)
		{
			job = s_sleep_aid_job_ring[s_sleep_aid_job_head % SLEEP_AID_JOB_QUEUE_LEN];
			s_sleep_aid_job_head++;
			got = 1;
		}
		divoom_system_free_lock(s_sleep_aid_job_lock);
		if (got)
		{
			sleep_aid_job_exec(&job);
		}
	}
	return NULL;
}

/* 只在 divoom_sleep_aid_ctrl_init 的单线程阶段调用：post 侧按需补建会让多个调用线程
 * 各建一个 worker，两个消费者一起跑就没有串行可言了。 */
static void sleep_aid_job_worker_start(void)
{
	if (s_sleep_aid_job_task)
	{
		return;
	}
	s_sleep_aid_job_lock = divoom_system_create_lock();
	s_sleep_aid_job_sem = divoom_system_create_sem(SLEEP_AID_JOB_QUEUE_LEN, 0);
	if (!s_sleep_aid_job_lock || !s_sleep_aid_job_sem)
	{
		divoom_print("sleep_aid: job queue init fail\n");
		return;
	}
	s_sleep_aid_job_task = divoom_system_create_task_prio(sleep_aid_job_worker, "sleep_aid_job",
							     NULL, DIVOOM_TASK_STACK_SLEEP_AID_JOB,
							     SLEEP_AID_JOB_TASK_PRIO);
	if (!s_sleep_aid_job_task)
	{
		divoom_print("sleep_aid: job worker create fail\n");
	}
}

static void sleep_aid_job_post(int kind, int arg0, int arg1, int arg2)
{
	sleep_aid_job_t *slot;
	int reused = 0;

	if (!s_sleep_aid_job_task)
	{
		/* worker 不可用时退回同步执行：宁可卡一下也不能让功能失效 */
		sleep_aid_job_t job;

		job.kind = kind;
		job.arg0 = arg0;
		job.arg1 = arg1;
		job.arg2 = arg2;
		job.play_generation = s_sleep_aid_play_generation;
		divoom_print("sleep_aid: job worker unavailable, run inline kind=%d\n", kind);
		sleep_aid_job_exec(&job);
		return;
	}

	divoom_system_get_lock(s_sleep_aid_job_lock);
	if (s_sleep_aid_job_tail - s_sleep_aid_job_head >= SLEEP_AID_JOB_QUEUE_LEN)
	{
		sleep_aid_job_t dropped;

		/* UI 连点挤满队列时丢最旧的：用户意图以最后一次为准，且不额外 put_sem */
		dropped = s_sleep_aid_job_ring[s_sleep_aid_job_head % SLEEP_AID_JOB_QUEUE_LEN];
		s_sleep_aid_job_head++;
		reused = 1;
		/* 被挤掉的 PCM_STOP 不会再执行，避免 wait 一直挂着 */
		if (dropped.kind == SLEEP_AID_JOB_PCM_STOP)
		{
			s_sleep_aid_pcm_stop_pending = 0;
		}
	}
	slot = &s_sleep_aid_job_ring[s_sleep_aid_job_tail % SLEEP_AID_JOB_QUEUE_LEN];
	slot->kind = kind;
	slot->arg0 = arg0;
	slot->arg1 = arg1;
	slot->arg2 = arg2;
	slot->play_generation = s_sleep_aid_play_generation;
	s_sleep_aid_job_tail++;
	divoom_system_free_lock(s_sleep_aid_job_lock);

	if (!reused)
	{
		divoom_system_put_sem(s_sleep_aid_job_sem);
	}
}

static void sleep_aid_pcm_stop_async(void)
{
	if (s_sleep_aid_pcm_stop_pending)
	{
		return;
	}
	s_sleep_aid_pcm_stop_pending = 1;
	sleep_aid_job_post(SLEEP_AID_JOB_PCM_STOP, 0, 0, 0);
}

static int sleep_aid_pcm_stop_wait(unsigned int timeout_ms)
{
	unsigned int t0 = divoom_system_get_tickms();

	while (s_sleep_aid_pcm_stop_pending)
	{
		if (divoom_system_get_tickms() - t0 >= timeout_ms)
		{
			return -1;
		}
		divoom_system_delay_ms(10);
	}
	return 0;
}

void divoom_sleep_aid_ctrl_set_voice_vol(int volume)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();
	int voice_type;
	int level;

	if (!c)
	{
		return;
	}
	if (volume < 0)
	{
		volume = 0;
	}
	if (volume > 100)
	{
		volume = 100;
	}
	if (c->settings_preview_vol_active)
	{
		c->settings_preview_vol_percent = volume;
		if (c->settings_preview_vol_is_after)
		{
			divoom_local_music_ctrl_set_tone_volume(volume);
			return;
		}
		voice_type = c->sel_voice_type;
	}
	else
	{
		voice_type = c->sel_voice_type;
	}
	if (voice_type == DIVOOM_SLEEP_AID_TYPE_DIY)
	{
		level = (volume * 16 + 50) / 100;

		if (level > 16)
		{
			level = 16;
		}
		if (level < 0)
		{
			level = 0;
		}
		divoom_sleep_pcm_session_set_diy_mix_vol_level(level);
		if (divoom_sleep_pcm_session_get_mode() == DIVOOM_SLEEP_PCM_SESSION_DIY)
		{
			divoom_audio_session_sleep_set_volume(volume);
		}
	}
	else if (divoom_sleep_pcm_session_get_mode() == DIVOOM_SLEEP_PCM_SESSION_MPG)
	{
		sleep_aid_mpg_loop_set_volume(volume);
	}
	else
	{
		divoom_local_music_ctrl_set_tone_volume(volume);
	}
}

void divoom_sleep_aid_ctrl_select_voice(int voice_type, int voice_id)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();

	if (!c)
	{
		return;
	}
	c->sel_voice_type = voice_type;
	c->sel_voice_id = voice_id;
	sleep_aid_job_post(SLEEP_AID_JOB_SELECT_VOICE, voice_type, voice_id, 0);
}

void divoom_sleep_aid_ctrl_stop_play_voice(void)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();

	if (c)
	{
		c->settings_preview_vol_active = 0;
		c->settings_preview_vol_is_after = 0;
		if (c->app_aid_sleep_preview_active)
		{
			/* schedule play 回调可能持 event_lock，勿同步 cancel TRY_LISTEN */
			sleep_aid_app_preview_teardown(0);
			sleep_aid_job_post(SLEEP_AID_JOB_CANCEL_PREVIEW_SCHEDULE, 0, 0, 0);
			return;
		}
	}
	sleep_aid_job_post(SLEEP_AID_JOB_STOP_VOICE, 0, 0, 0);
}

void divoom_sleep_aid_ctrl_app_preview_begin(void)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();

	if (!c)
	{
		return;
	}
	sleep_aid_app_preview_schedule_end();
	c->app_aid_sleep_preview_active = 1;
	if (sleep_aid_app_preview_schedule_begin() == 0)
	{
		divoom_print("%s: TRY_LISTEN schedule push failed\n", __func__);
	}
}

void divoom_sleep_aid_ctrl_app_preview_end(void)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();

	if (c)
	{
		c->app_aid_sleep_preview_active = 0;
	}
	sleep_aid_app_preview_schedule_end();
}

void divoom_sleep_aid_ctrl_app_preview_stop(void)
{
	sleep_aid_app_preview_teardown(1);
}

uint8 divoom_sleep_aid_ctrl_app_preview_is_active(void)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();

	return (c && c->app_aid_sleep_preview_active) ? (uint8)1 : (uint8)0;
}

void divoom_sleep_aid_ctrl_app_preview_stop_by_user_key(void)
{
	divoom_print("%s: user key exit app aid sleep preview\n", __func__);
	divoom_sleep_aid_ctrl_app_preview_stop();
}

void divoom_sleep_aid_ctrl_settings_volume_preview_start(int vol_level_0_100, uint8 is_after_vol)
{
	Pdivoom_sleep_aid_ctrl c;
	divoom_sleep_aid_info *inf;
	int voice_type;
	int voice_id;
	int pct;

	c = divoom_sleep_aid_ctrl_instance();
	if (!c)
	{
		return;
	}
	sleep_aid_ctrl_ensure_settings_init(c);
	inf = &c->settings;
	pct = sleep_aid_clamp_vol_level(vol_level_0_100);
	c->settings_preview_vol_is_after = is_after_vol ? (uint8)1 : (uint8)0;
	if (is_after_vol)
	{
		sleep_aid_job_post(SLEEP_AID_JOB_AFTER_MUSIC_PREVIEW, inf->after_music, pct, 0);
		return;
	}
	if (sleep_aid_resolve_last_sleep_voice(&voice_type, &voice_id) != 0)
	{
		return;
	}
	c->settings_preview_vol_active = 1;
	c->settings_preview_vol_percent = pct;
	c->sel_voice_type = voice_type;
	c->sel_voice_id = voice_id;
	sleep_aid_job_post(SLEEP_AID_JOB_VOICE_PREVIEW, voice_type, voice_id, pct);
}

void divoom_sleep_aid_ctrl_settings_after_music_preview(int music_idx, int vol_level_0_100)
{
	sleep_aid_job_post(SLEEP_AID_JOB_AFTER_MUSIC_PREVIEW, music_idx, vol_level_0_100, 0);
}

void divoom_sleep_aid_ctrl_settings_volume_preview_set_level(int vol_level_0_100)
{
	Pdivoom_sleep_aid_ctrl c;
	int pct;

	c = divoom_sleep_aid_ctrl_instance();
	if (!c || !c->settings_preview_vol_active)
	{
		return;
	}
	pct = sleep_aid_clamp_vol_level(vol_level_0_100);
	c->settings_preview_vol_percent = pct;
	divoom_sleep_aid_ctrl_set_voice_vol(pct);
}

void divoom_sleep_aid_ctrl_settings_volume_preview_stop(void)
{
	Pdivoom_sleep_aid_ctrl c;

	c = divoom_sleep_aid_ctrl_instance();
	if (!c || !c->settings_preview_vol_active)
	{
		return;
	}
	c->settings_preview_vol_active = 0;
	c->settings_preview_vol_is_after = 0;
	divoom_sleep_aid_ctrl_stop_play_voice();
}

void divoom_sleep_aid_ctrl_on_music_favorite_key(void)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();

	if (!c || c->play_session_active)
	{
		return;
	}
	divoom_sleep_aid_ctrl_enter_play_voice(c->sel_voice_type, c->sel_voice_id);
}

void divoom_sleep_aid_ctrl_enter_play_voice(int voice_type, int voice_id)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();
	divoom_sys_msg_info msg = {0};
	sleep_aid_msg_voice_pair_t pair;

	if (!c)
	{
		return;
	}
	c->sel_voice_type = voice_type;
	c->sel_voice_id = voice_id;
	pair.voice_type = voice_type;
	pair.voice_id = voice_id;
	msg.msg_type = 0;
	msg.src_id = DIVOOM_MODEL_MAIN;
	msg.dest_id = DIVOOM_MODEL_SLEEP_AID;
	msg.msg_id = DIVOOM_SLEEP_AID_MSG_ENTER_PLAY_VOICE;
	msg.data = (uint8 *)&pair;
	msg.data_len = (uint16)sizeof(pair);
	divoom_sys_model_crl_msg_push(&msg);
}

int divoom_sleep_aid_ctrl_should_suppress_system_tone(void)
{
	return divoom_audio_policy_tone_play_allowed() ? 0 : 1;
}

int divoom_sleep_aid_ctrl_should_keep_jl_after_tone(void)
{
	return divoom_audio_policy_keep_jl_after_tone();
}

void divoom_sleep_aid_ctrl_cancel_play_voice(void)
{
	Pdivoom_sleep_aid_ctrl c;

	c = divoom_sleep_aid_ctrl_instance();
	if (c)
	{
		c->play_session_active = 0;
	}
	s_sleep_aid_play_generation++;
	sleep_aid_play_phase_stop(c);
	sleep_aid_voice_schedule_cancel_play();
	sleep_aid_voice_stop_playback_async();
	divoom_local_movie_play_stop_movie();
	sleep_aid_restore_sys_light(c);
}

void divoom_sleep_aid_ctrl_cancel_play_voice_wait(unsigned int timeout_ms)
{
	divoom_sleep_aid_ctrl_cancel_play_voice();
	sleep_aid_wait_play_video_stopped(timeout_ms);
	(void)sleep_aid_pcm_stop_wait(timeout_ms);
}

void divoom_sleep_aid_ctrl_play_voice_with_video(int voice_type, int voice_id, int video_type)
{
	sleep_aid_job_post(SLEEP_AID_JOB_PLAY_VOICE_VIDEO, voice_type, voice_id, video_type);
}

void divoom_sleep_aid_ctrl_play_voice(int voice_type, int voice_id)
{
	divoom_sleep_aid_ctrl_play_voice_with_video(voice_type, voice_id, -1);
}

int divoom_sleep_aid_ctrl_record_delete_at(int idx)
{
	divoom_sys_msg_info msg = {0};
	int ix = idx;

	msg.msg_type = 0;
	msg.src_id = DIVOOM_MODEL_MAIN;
	msg.dest_id = DIVOOM_MODEL_SLEEP_AID;
	msg.msg_id = DIVOOM_SLEEP_AID_MSG_RECORD_DELETE;
	msg.data = (uint8 *)&ix;
	msg.data_len = (uint16)sizeof(ix);
	msg.result = -1;
	divoom_sys_model_crl_msg_push(&msg);
	return msg.result;
}

int divoom_sleep_aid_ctrl_record_ctrl(int flag)
{
	divoom_sys_msg_info msg = {0};
	int fl = flag;

	msg.msg_type = 0;
	msg.src_id = DIVOOM_MODEL_MAIN;
	msg.dest_id = DIVOOM_MODEL_SLEEP_AID;
	msg.msg_id = DIVOOM_SLEEP_AID_MSG_RECORD_CTRL;
	msg.data = (uint8 *)&fl;
	msg.data_len = (uint16)sizeof(fl);
	msg.result = -99;
	divoom_sys_model_crl_msg_push(&msg);

	return msg.result;
}

void divoom_sleep_aid_ctrl_open_fav_list_menu(void)
{
	int active;

	if (divoom_sleep_aid_ctrl_is_play_session_active())
		return;
	active = divoom_menu_control_get_active_menu_id();
	if (active == DIVOOM_MENU_CONTROL_ID_SLEEP_AID
	    || active == DIVOOM_MENU_CONTROL_ID_SLEEP_AID_ACTIVE)
		return;
	divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SLEEP_AID, NULL, 0, 0);
}

void divoom_sleep_aid_ctrl_active_esc_back(void)
{
	/* 播放页进入时已清空旧栈，ESC 明确重建助眠选择页。 */
	divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_SLEEP_AID, NULL, 0, 0);
}

uint8 divoom_sleep_aid_ctrl_exit_play_on_music_mode_key(uint8 key_status)
{
	if (key_status != DIVOOM_PRESS_SHORT && key_status != DIVOOM_PRESSING)
		return 0;
	if (!divoom_sleep_aid_ctrl_is_play_session_active())
		return 0;
	divoom_print("sleep_aid: MODE key exit play\n");
	divoom_sleep_aid_ctrl_active_esc_back();
	return 1;
}

uint8 divoom_sleep_aid_ctrl_is_play_session_active(void)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();

	return (c && c->play_session_active) ? 1 : 0;
}

uint8 divoom_sleep_aid_ctrl_is_sleep_voice_audible(void)
{
	Pdivoom_sleep_aid_ctrl c = divoom_sleep_aid_ctrl_instance();
	divoom_audio_session_state_t st;

	if (!c)
	{
		return 0;
	}
	if (c->app_aid_sleep_preview_active)
	{
		return 1;
	}
	if (c->play_session_active)
	{
		return 1;
	}
	if (c->settings_preview_vol_active)
	{
		return 1;
	}
	if (divoom_sleep_pcm_session_is_running())
	{
		return 1;
	}
	st = divoom_audio_session_get_state();
	if (st == DIVOOM_AUDIO_SESSION_STATE_SLEEP
	    || st == DIVOOM_AUDIO_SESSION_STATE_OVERLAY)
	{
		return 1;
	}
	return 0;
}

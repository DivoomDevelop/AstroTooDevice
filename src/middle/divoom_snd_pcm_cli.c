#include "divoom_finsh_autotest.h"
#if DIVOOM_FINSH_AUTOTEST

/*
 * Finsh: diy_pcm_test — DIY mix_pcm + snd_pcm 链路自动化探针
 *
 *   diy_pcm_test              # 默认 5s, vol=80
 *   diy_pcm_test 8 60         # 秒数, JL 音量 0..100
 */
#include "divoom.h"
#include <console.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "divoom_sleep_aid_ctrl.h"
#include "divoom_local_music_ctrl.h"
#include "divoom_snd_pcm.h"
#include "divoom_audio_sink.h"
#include "divoom_audio_policy.h"
#include "divoom_audio_session.h"
#include "divoom_audio_buf_pool.h"
#include "divoom_mix_pcm_ctrl.h"
#include "divoom_main.h"

#define DIY_TONE_ALT_A "/usr/share/divoom_app/tone/Clock_Normal1.mp3"
#define DIY_TONE_ALT_B "/usr/share/divoom_app/tone/Clock_Campanula.mp3"

static int diy_tone_alt_wait_tone_start(unsigned int timeout_ms)
{
	unsigned int t0 = divoom_system_get_tickms();

	while (divoom_system_get_tickms() - t0 < timeout_ms)
	{
		if (divoom_local_music_ctrl_get_tone_status())
		{
			return 0;
		}
		divoom_system_delay_ms(10);
	}
	return -1;
}

static int diy_tone_alt_wait_idle(unsigned int timeout_ms)
{
	unsigned int t0 = divoom_system_get_tickms();

	while (divoom_system_get_tickms() - t0 < timeout_ms)
	{
		if (!divoom_local_music_ctrl_get_tone_status())
		{
			return 0;
		}
		divoom_system_delay_ms(20);
	}
	return -1;
}

static int diy_tone_alt_wait_snd_pcm(unsigned int timeout_ms)
{
	unsigned int t0 = divoom_system_get_tickms();

	while (divoom_system_get_tickms() - t0 < timeout_ms)
	{
		if (divoom_snd_pcm_whitebox_is_running())
		{
			return 0;
		}
		divoom_system_delay_ms(20);
	}
	return -1;
}

static int diy_tone_alt_wait_snd_pcm_stop(unsigned int timeout_ms)
{
	unsigned int t0 = divoom_system_get_tickms();

	while (divoom_system_get_tickms() - t0 < timeout_ms)
	{
		if (!divoom_snd_pcm_whitebox_is_running())
		{
			return 0;
		}
		divoom_system_delay_ms(20);
	}
	return -1;
}

static int diy_tone_alt_wait_policy_sleep_idle(unsigned int timeout_ms)
{
	unsigned int t0 = divoom_system_get_tickms();

	while (divoom_system_get_tickms() - t0 < timeout_ms)
	{
		if (!divoom_audio_policy_is_sleep_playback_active()
		    && !divoom_snd_pcm_whitebox_is_running()
		    && divoom_audio_policy_tone_play_allowed())
		{
			return 0;
		}
		divoom_system_delay_ms(20);
	}
	return -1;
}

static int divoom_snd_pcm_diy_test_cli(int argc, char **argv)
{
	int sec = 5;
	int vol = 80;
	unsigned int out_frames_before;
	unsigned int out_frames_after;
	unsigned int underruns;
	int ring_bytes;
	int ok;

	if (argc >= 2)
	{
		sec = atoi(argv[1]);
	}
	if (argc >= 3)
	{
		vol = atoi(argv[2]);
	}
	if (sec < 1)
	{
		sec = 1;
	}
	if (sec > 120)
	{
		sec = 120;
	}
	if (vol < 0)
	{
		vol = 0;
	}
	if (vol > 100)
	{
		vol = 100;
	}

	printf("diy_pcm_test: start sec=%d vol=%d\n", sec, vol);

	divoom_local_music_ctrl_stop_tone();
	divoom_system_delay_ms(50);

#if defined(DIVOOM_TIME_FRAME_SPEAKER) && defined(DIVOOM_TIMES_VIBE)
	divoom_main_set_jl_mcu_tone_volume(vol);
#endif

	out_frames_before = divoom_snd_pcm_whitebox_out_frames();
	divoom_sleep_aid_ctrl_select_voice(DIVOOM_SLEEP_AID_TYPE_DIY, 1);
	divoom_system_delay_ms(200);

	if (!divoom_snd_pcm_whitebox_is_running())
	{
		printf("SND_PCM_TEST: ok=0 reason=snd_pcm_not_running sec=%d\n", sec);
		divoom_sleep_aid_ctrl_stop_play_voice();
		return 1;
	}

	divoom_system_delay_ms((unsigned int)sec * 1000u);

	out_frames_after = divoom_snd_pcm_whitebox_out_frames();
	underruns = divoom_snd_pcm_whitebox_underruns();
	ring_bytes = divoom_mix_pcm_ring_data_size();

	/* 48kHz 下约 sec*48000 帧；允许起播前后 0.5s 裕量 */
	ok = (out_frames_after > out_frames_before)
	     && ((out_frames_after - out_frames_before) >= (unsigned int)(sec * 40000));

	printf("SND_PCM_TEST: ok=%d sec=%d vol=%d out_frames=%u delta=%u underruns=%u ring_bytes=%d silence=%u running=%d\n",
	       ok, sec, vol, out_frames_after,
	       out_frames_after - out_frames_before,
	       underruns, ring_bytes,
	       divoom_snd_pcm_whitebox_silence_periods(),
	       divoom_snd_pcm_whitebox_is_running());

	divoom_sleep_aid_ctrl_stop_play_voice();
	divoom_system_delay_ms(100);

	printf("diy_pcm_test: done ok=%d\n", ok);
	return ok ? 0 : 1;
}

FINSH_FUNCTION_EXPORT_CMD(divoom_snd_pcm_diy_test_cli, diy_pcm_test,
			  DIY mix_pcm snd_pcm autotest probe);

static int divoom_snd_pcm_diy_tone_alt_cli(int argc, char **argv)
{
	int n = 10;
	int gap_ms = 400;
	int vol = 80;
	unsigned int i;
	unsigned int cycles_ok = 0;
	unsigned int tone_timeouts = 0;
	unsigned int resume_fail = 0;
	int ok;

	if (argc >= 2)
	{
		n = atoi(argv[1]);
	}
	if (argc >= 3)
	{
		gap_ms = atoi(argv[2]);
	}
	if (argc >= 4)
	{
		vol = atoi(argv[3]);
	}
	if (n < 1)
	{
		n = 1;
	}
	if (n > 100)
	{
		n = 100;
	}
	if (gap_ms < 100)
	{
		gap_ms = 100;
	}
	if (gap_ms > 5000)
	{
		gap_ms = 5000;
	}
	if (vol < 0)
	{
		vol = 0;
	}
	if (vol > 100)
	{
		vol = 100;
	}

	printf("diy_tone_alt_test: n=%d gap_ms=%d vol=%d\n", n, gap_ms, vol);

	divoom_local_music_ctrl_stop_tone();
	divoom_system_delay_ms(500);
	divoom_local_music_ctrl_stop_tone();
	divoom_system_delay_ms(500);

#if defined(DIVOOM_TIME_FRAME_SPEAKER) && defined(DIVOOM_TIMES_VIBE)
	divoom_main_set_jl_mcu_tone_volume(vol);
#endif

	divoom_sleep_aid_ctrl_select_voice(DIVOOM_SLEEP_AID_TYPE_DIY, 1);
	divoom_system_delay_ms(800);

	if (!divoom_snd_pcm_whitebox_is_running())
	{
		printf("DIY_TONE_ALT_TEST: ok=0 reason=diy_not_started n=%d tone_timeouts=%u resume_fail=%u cycles_ok=%u\n",
		       n, tone_timeouts, resume_fail, cycles_ok);
		return 1;
	}

	/* 等开机联网提示音结束，避免与压测抢 local_tone 队列 */
	divoom_system_delay_ms(3000);
	divoom_local_music_ctrl_stop_tone();
	divoom_system_delay_ms(200);

	for (i = 0; i < (unsigned int)n; i++)
	{
		const char *tone = (i & 1u) ? DIY_TONE_ALT_B : DIY_TONE_ALT_A;
		unsigned int frames_before;
		unsigned int frames_after;
		int tone_started = 0;
		int cycle_ok = 0;

		divoom_system_delay_ms((unsigned int)gap_ms / 2u);

		printf("diy_tone_alt_test: cycle #%u tone=%s\n", i + 1, tone);

		/* 策略层 DIY 活跃时会 suppress 提示音：先停 DIY 再播 tone，再恢复 DIY */
		divoom_sleep_aid_ctrl_stop_play_voice();
		divoom_system_delay_ms(200);
		(void)diy_tone_alt_wait_policy_sleep_idle(5000);

		if (divoom_local_music_ctrl_tone_play((char *)tone, vol, NULL, 5) == 1
		    && diy_tone_alt_wait_tone_start(5000) == 0)
		{
			tone_started = 1;
		}
		else
		{
			tone_timeouts++;
			printf("diy_tone_alt_test: tone start timeout cycle=%u\n", i + 1);
		}

		if (tone_started && diy_tone_alt_wait_idle(10000) != 0)
		{
			divoom_local_music_ctrl_stop_tone();
			divoom_system_delay_ms(200);
			if (diy_tone_alt_wait_idle(3000) != 0)
			{
				tone_timeouts++;
				printf("diy_tone_alt_test: tone end timeout cycle=%u\n", i + 1);
			}
		}

		divoom_local_music_ctrl_stop_tone();
		divoom_system_delay_ms(80);

		divoom_sleep_aid_ctrl_select_voice(DIVOOM_SLEEP_AID_TYPE_DIY, 1);
		divoom_system_delay_ms(300);
		if (diy_tone_alt_wait_snd_pcm(8000) != 0)
		{
			resume_fail++;
			printf("diy_tone_alt_test: snd_pcm resume fail cycle=%u\n", i + 1);
			continue;
		}

		frames_before = divoom_snd_pcm_whitebox_out_frames();
		divoom_system_delay_ms((unsigned int)gap_ms / 2u);
		frames_after = divoom_snd_pcm_whitebox_out_frames();

		if (divoom_snd_pcm_whitebox_is_running()
		    && frames_after > frames_before
		    && (frames_after - frames_before) >= 4000u)
		{
			cycle_ok = 1;
			cycles_ok++;
		}
		else
		{
			printf("diy_tone_alt_test: diy stall cycle=%u before=%u after=%u delta=%u running=%d\n",
			       i + 1, frames_before, frames_after,
			       (frames_after > frames_before) ? (frames_after - frames_before) : 0u,
			       divoom_snd_pcm_whitebox_is_running());
		}

		(void)cycle_ok;
	}

	ok = (cycles_ok == (unsigned int)n) && (resume_fail == 0);

	printf("DIY_TONE_ALT_TEST: ok=%d n=%d cycles_ok=%u tone_timeouts=%u resume_fail=%u underruns=%u running=%d\n",
	       ok, n, cycles_ok, tone_timeouts, resume_fail,
	       divoom_snd_pcm_whitebox_underruns(),
	       divoom_snd_pcm_whitebox_is_running());

	divoom_sleep_aid_ctrl_stop_play_voice();
	divoom_system_delay_ms(100);

	printf("diy_tone_alt_test: done ok=%d\n", ok);
	return ok ? 0 : 1;
}

FINSH_FUNCTION_EXPORT_CMD(divoom_snd_pcm_diy_tone_alt_cli, diy_tone_alt_test,
			  DIY mix_pcm <-> tone alternate autotest);

#define DIY_REENTRY_LIST_A "/usr/share/divoom_app/divoom/41/298.mp3"
#define DIY_REENTRY_LIST_B "/usr/share/divoom_app/divoom/41/422.mp3"

static int divoom_snd_pcm_diy_reentry_cli(int argc, char **argv)
{
	int vol = 50;
	int ok_first = 0;
	int ok_reentry = 0;
	int ok_frames = 0;
	unsigned int f_base;
	unsigned int f_after;
	int ok;

	if (argc >= 2)
	{
		vol = atoi(argv[1]);
	}
	if (vol < 0)
	{
		vol = 0;
	}
	if (vol > 100)
	{
		vol = 100;
	}

	printf("diy_reentry_test: vol=%d (DIY -> exit -> list cut -> DIY)\n", vol);

	divoom_local_music_ctrl_stop_tone();
	divoom_system_delay_ms(300);

	divoom_sleep_aid_ctrl_select_voice(DIVOOM_SLEEP_AID_TYPE_DIY, 1);
	divoom_system_delay_ms(800);
	if (diy_tone_alt_wait_snd_pcm(8000) == 0)
	{
		ok_first = 1;
	}
	divoom_system_delay_ms(300);

	divoom_sleep_aid_ctrl_stop_play_voice();
	divoom_system_delay_ms(500);

	divoom_local_music_ctrl_tone_play((char *)DIY_REENTRY_LIST_A, vol, NULL, 24 * 3600);
	if (diy_tone_alt_wait_tone_start(5000) != 0)
	{
		printf("diy_reentry_test: warn list A start timeout\n");
	}
	divoom_system_delay_ms(200);
	divoom_local_music_ctrl_tone_play((char *)DIY_REENTRY_LIST_B, vol, NULL, 24 * 3600);
	if (diy_tone_alt_wait_tone_start(5000) != 0)
	{
		printf("diy_reentry_test: warn list B start timeout\n");
	}
	divoom_system_delay_ms(500);

	divoom_sleep_aid_ctrl_select_voice(DIVOOM_SLEEP_AID_TYPE_DIY, 1);
	divoom_system_delay_ms(800);
	if (diy_tone_alt_wait_snd_pcm(8000) == 0)
	{
		ok_reentry = 1;
	}

	f_base = divoom_snd_pcm_whitebox_out_frames();
	divoom_system_delay_ms(400);
	f_after = divoom_snd_pcm_whitebox_out_frames();
	if (divoom_snd_pcm_whitebox_is_running() && f_after > f_base
	    && (f_after - f_base) >= 4000u)
	{
		ok_frames = 1;
	}

	ok = ok_first && ok_reentry && ok_frames;

	printf("DIY_REENTRY_TEST: ok=%d first_diy=%d reentry_diy=%d frames_ok=%d "
	       "frames=%u delta=%u running=%d underruns=%u\n",
	       ok, ok_first, ok_reentry, ok_frames, f_after,
	       (f_after > f_base) ? (f_after - f_base) : 0u,
	       divoom_snd_pcm_whitebox_is_running(),
	       divoom_snd_pcm_whitebox_underruns());

	divoom_sleep_aid_ctrl_stop_play_voice();
	divoom_system_delay_ms(100);

	printf("diy_reentry_test: done ok=%d\n", ok);
	return ok ? 0 : 1;
}

FINSH_FUNCTION_EXPORT_CMD(divoom_snd_pcm_diy_reentry_cli, diy_reentry_test,
			  DIY exit list-preview re-enter autotest);

#define BAD_FILE_DIY_TEST_PATH   "/usr/share/divoom_app/divoom/42/1.mp3"
#define BAD_FILE_DIY_TEST_MAX_SZ 1024

static int divoom_snd_pcm_bad_file_diy_cli(int argc, char **argv)
{
	int vol = 50;
	int file_sz;
	int ok_reject = 0;
	int ok_diy = 0;
	int ok_frames = 0;
	unsigned int f_base;
	unsigned int f_after;
	int ok;

	if (argc >= 2)
	{
		vol = atoi(argv[1]);
	}
	if (vol < 0)
	{
		vol = 0;
	}
	if (vol > 100)
	{
		vol = 100;
	}

	printf("bad_file_diy_test: path=%s vol=%d\n", BAD_FILE_DIY_TEST_PATH, vol);

	divoom_local_music_ctrl_stop_tone();
	divoom_system_delay_ms(300);

	file_sz = divoom_app_com_get_file_size((char *)BAD_FILE_DIY_TEST_PATH);
	if (file_sz < 0)
	{
		printf("BAD_FILE_DIY_TEST: ok=0 reason=file_missing sz=%d\n", file_sz);
		return 1;
	}
	if (file_sz >= BAD_FILE_DIY_TEST_MAX_SZ)
	{
		printf("BAD_FILE_DIY_TEST: ok=0 reason=file_not_small sz=%d (need <%d)\n",
		       file_sz, BAD_FILE_DIY_TEST_MAX_SZ);
		return 1;
	}

	divoom_local_music_ctrl_tone_play((char *)BAD_FILE_DIY_TEST_PATH, vol, NULL, 24 * 3600);
	/* 容错路径须在数百毫秒内返回，不可卡住 local_tone */
	divoom_system_delay_ms(800);
	if (!divoom_local_music_ctrl_get_tone_status())
	{
		ok_reject = 1;
	}
	else if (diy_tone_alt_wait_idle(3000) == 0)
	{
		ok_reject = 1;
	}
	else
	{
		printf("bad_file_diy_test: warn tone still active, force stop\n");
		divoom_local_music_ctrl_stop_tone();
		divoom_system_delay_ms(200);
	}

	divoom_local_music_ctrl_stop_tone();
	divoom_system_delay_ms(200);

	divoom_sleep_aid_ctrl_select_voice(DIVOOM_SLEEP_AID_TYPE_DIY, 1);
	divoom_system_delay_ms(800);
	if (diy_tone_alt_wait_snd_pcm(8000) == 0)
	{
		ok_diy = 1;
	}

	f_base = divoom_snd_pcm_whitebox_out_frames();
	divoom_system_delay_ms(2000);
	f_after = divoom_snd_pcm_whitebox_out_frames();
	if (divoom_snd_pcm_whitebox_is_running() && f_after > f_base
	    && (f_after - f_base) >= 4000u)
	{
		ok_frames = 1;
	}

	ok = ok_reject && ok_diy && ok_frames;

	printf("BAD_FILE_DIY_TEST: ok=%d reject=%d diy=%d frames_ok=%d file_sz=%d "
	       "frames=%u delta=%u running=%d underruns=%u\n",
	       ok, ok_reject, ok_diy, ok_frames, file_sz, f_after,
	       (f_after > f_base) ? (f_after - f_base) : 0u,
	       divoom_snd_pcm_whitebox_is_running(),
	       divoom_snd_pcm_whitebox_underruns());

	divoom_sleep_aid_ctrl_stop_play_voice();
	divoom_system_delay_ms(100);

	printf("bad_file_diy_test: done ok=%d\n", ok);
	return ok ? 0 : 1;
}

FINSH_FUNCTION_EXPORT_CMD(divoom_snd_pcm_bad_file_diy_cli, bad_file_diy_test,
			  bad MP3 then DIY mix fault tolerance autotest);

static int divoom_audio_sink_stat_cli(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	printf("audio_sink_stat: sink=%s out=%u silence=%u sleep_env=%u tone_env=%u sleep_fill=%u%% tone_fill=%u%% sleep_und=%u tone_und=%u\n",
	       divoom_audio_sink_is_running() ? "run" : "stop",
	       divoom_audio_sink_whitebox_out_frames(),
	       divoom_audio_sink_whitebox_silence_periods(),
	       divoom_audio_sink_ch_env_mul(DIVOOM_AUDIO_SINK_CH_SLEEP),
	       divoom_audio_sink_ch_env_mul(DIVOOM_AUDIO_SINK_CH_TONE),
	       divoom_audio_sink_ch_ring_fill_pct(DIVOOM_AUDIO_SINK_CH_SLEEP),
	       divoom_audio_sink_ch_ring_fill_pct(DIVOOM_AUDIO_SINK_CH_TONE),
	       divoom_audio_sink_whitebox_underruns(DIVOOM_AUDIO_SINK_CH_SLEEP),
	       divoom_audio_sink_whitebox_underruns(DIVOOM_AUDIO_SINK_CH_TONE));
	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(divoom_audio_sink_stat_cli, audio_sink_stat,
			  audio_sink status probe);

static int divoom_audio_diag_cli(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	divoom_audio_session_diag_print();
	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(divoom_audio_diag_cli, audio_diag,
			  FSM lease duck tone queue sink snapshot);

static int divoom_audio_sink_diag_cli(int argc, char **argv)
{
	int on = 1;

	if (argc >= 2)
	{
		on = atoi(argv[1]) ? 1 : 0;
	}
	divoom_audio_sink_diag_enable(on);
	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(divoom_audio_sink_diag_cli, audio_sink_diag,
			  audio_sink burst diag on/off);

static int divoom_audio_sink_diag_stat_cli(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	divoom_audio_sink_diag_print();
	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(divoom_audio_sink_diag_stat_cli, audio_sink_diag_stat,
			  audio_sink burst diag counters);

static int divoom_audio_sink_diag_clr_cli(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	divoom_audio_sink_diag_clear();
	printf("audio_sink_diag: cleared\n");
	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(divoom_audio_sink_diag_clr_cli, audio_sink_diag_clr,
			  audio_sink burst diag clear);

static int divoom_audio_sink_test_cli(int argc, char **argv)
{
	int mode_fade = 0;
	unsigned int env0;
	unsigned int env1;

	if (argc >= 2 && strcmp(argv[1], "fade") == 0)
	{
		mode_fade = 1;
	}

	if (!mode_fade)
	{
		printf("audio_sink_test: usage fade\n");
		return 1;
	}

	if (divoom_audio_sink_start() != 0)
	{
		printf("audio_sink_test: start fail\n");
		return 1;
	}
	/* 租约池后 fade/ensure_ring 必须先 acquire，否则 env 恒为 0 */
	if (divoom_audio_buf_pool_acquire(DIVOOM_AUDIO_BUS_TONE, "audio_sink_test") != 0)
	{
		printf("audio_sink_test: lease fail\n");
		return 1;
	}
	divoom_audio_sink_ch_set_volume(DIVOOM_AUDIO_SINK_CH_TONE, 80);
	divoom_audio_sink_ch_fade_in(DIVOOM_AUDIO_SINK_CH_TONE, 50, 0);
	divoom_system_delay_ms(60);
	env0 = divoom_audio_sink_ch_env_mul(DIVOOM_AUDIO_SINK_CH_TONE);
	divoom_audio_sink_ch_fade_out(DIVOOM_AUDIO_SINK_CH_TONE, 15);
	(void)divoom_audio_sink_ch_fade_out_wait(DIVOOM_AUDIO_SINK_CH_TONE, 100);
	env1 = divoom_audio_sink_ch_env_mul(DIVOOM_AUDIO_SINK_CH_TONE);
	divoom_audio_sink_ch_disable(DIVOOM_AUDIO_SINK_CH_TONE);
	divoom_audio_buf_pool_force_release(DIVOOM_AUDIO_BUS_TONE, "audio_sink_test");
	divoom_audio_sink_try_stop();

	if (env0 > 0 && env1 == 0)
	{
		printf("audio_sink_test: ok env0=%u env1=%u\n", env0, env1);
		return 0;
	}
	printf("audio_sink_test: fail env0=%u env1=%u\n", env0, env1);
	return 1;
}

FINSH_FUNCTION_EXPORT_CMD(divoom_audio_sink_test_cli, audio_sink_test,
			  audio_sink fade/volume autotest);

static int divoom_audio_robust_cleanup_cli(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	divoom_local_music_ctrl_stop_tone();
	divoom_system_delay_ms(300);
	divoom_sleep_aid_ctrl_stop_play_voice();
	divoom_system_delay_ms(300);
	divoom_local_music_ctrl_stop_tone();
	divoom_system_delay_ms(200);
	divoom_audio_sink_diag_clear();

	printf("AUDIO_ROBUST_CLEANUP: ok=1 sink=%s tone=%d snd_pcm=%d\n",
	       divoom_audio_sink_is_running() ? "run" : "stop",
	       divoom_local_music_ctrl_get_tone_status(),
	       divoom_snd_pcm_whitebox_is_running());
	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(divoom_audio_robust_cleanup_cli, audio_robust_cleanup,
			  stop tone/DIY and reset audio sink diag for autotest);

static int divoom_audio_robust_preflight_cli(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	printf("AUDIO_ROBUST_PREFLIGHT: sink=%s sleep_active=%d policy_tone_ok=%d "
	       "ch_sleep=%d ch_tone=%d snd_pcm=%d\n",
	       divoom_audio_sink_is_running() ? "run" : "stop",
	       divoom_audio_policy_is_sleep_playback_active(),
	       divoom_audio_policy_tone_play_allowed(),
	       divoom_audio_sink_ch_active(DIVOOM_AUDIO_SINK_CH_SLEEP),
	       divoom_audio_sink_ch_active(DIVOOM_AUDIO_SINK_CH_TONE),
	       divoom_snd_pcm_whitebox_is_running());
	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(divoom_audio_robust_preflight_cli, audio_robust_preflight,
			  audio robust suite preflight probe);

static int divoom_audio_policy_test_cli(int argc, char **argv)
{
	int vol = 50;
	int ok_suppress = 0;
	int ok_reject = 0;
	int ok_recover = 0;
	int ok;

	(void)argc;
	if (argc >= 2)
	{
		vol = atoi(argv[1]);
	}
	if (vol < 0)
	{
		vol = 0;
	}
	if (vol > 100)
	{
		vol = 100;
	}

	divoom_local_music_ctrl_stop_tone();
	divoom_system_delay_ms(200);
	divoom_sleep_aid_ctrl_stop_play_voice();
	divoom_system_delay_ms(200);

	/* 等开机联网提示音结束，避免与策略探针抢 local_tone */
	divoom_system_delay_ms(3000);
	divoom_local_music_ctrl_stop_tone();
	divoom_system_delay_ms(200);

#if defined(DIVOOM_TIME_FRAME_SPEAKER) && defined(DIVOOM_TIMES_VIBE)
	divoom_main_set_jl_mcu_tone_volume(vol);
#endif

	/* P01: DIY 活跃时系统提示音应 DUCK_OVERLAY（允许播 + sleep duck） */
	divoom_sleep_aid_ctrl_select_voice(DIVOOM_SLEEP_AID_TYPE_DIY, 1);
	divoom_system_delay_ms(800);
	(void)diy_tone_alt_wait_idle(3000);
	if (diy_tone_alt_wait_snd_pcm(8000) == 0
	    && divoom_audio_policy_is_sleep_playback_active()
	    && divoom_audio_policy_tone_play_allowed())
	{
		divoom_audio_policy_action_t act =
			divoom_audio_session_on_request_tone();
		unsigned int duck0 =
			divoom_audio_sink_ch_get_duck(DIVOOM_AUDIO_SINK_CH_SLEEP);

		if (act == DIVOOM_AUDIO_POLICY_DUCK_OVERLAY
		    && divoom_local_music_ctrl_tone_play((char *)DIY_TONE_ALT_A, vol, NULL, 5) == 1)
		{
			unsigned int t0 = divoom_system_get_tickms();
			unsigned int duck1;

			while (divoom_system_get_tickms() - t0 < 3000u)
			{
				duck1 = divoom_audio_sink_ch_get_duck(DIVOOM_AUDIO_SINK_CH_SLEEP);
				if (divoom_local_music_ctrl_get_tone_status()
				    && duck1 == DIVOOM_AUDIO_POLICY_TONE_DUCK_SLEEP_PCT
				    && duck1 <= duck0)
				{
					ok_suppress = 1; /* 字段名保留：语义=duck overlay ok */
					break;
				}
				divoom_system_delay_ms(20);
			}
		}
	}

	divoom_local_music_ctrl_stop_tone();
	divoom_sleep_aid_ctrl_stop_play_voice();
	divoom_system_delay_ms(500);
	(void)diy_tone_alt_wait_policy_sleep_idle(8000);

	/* P02: 坏文件应 reject，不拉起 worker */
	{
		int file_sz = divoom_app_com_get_file_size((char *)BAD_FILE_DIY_TEST_PATH);

		if (file_sz > 0 && file_sz < BAD_FILE_DIY_TEST_MAX_SZ)
		{
			divoom_local_music_ctrl_tone_play((char *)BAD_FILE_DIY_TEST_PATH, vol, NULL, 0);
			divoom_system_delay_ms(800);
			if (!divoom_local_music_ctrl_get_tone_status())
			{
				ok_reject = 1;
			}
			divoom_local_music_ctrl_stop_tone();
			divoom_system_delay_ms(100);
		}
	}

	/* P03: DIY 停后 tone 可恢复 */
	divoom_local_music_ctrl_stop_tone();
	divoom_system_delay_ms(300);
	(void)diy_tone_alt_wait_policy_sleep_idle(5000);
	if (divoom_audio_policy_tone_play_allowed()
	    && divoom_local_music_ctrl_tone_play((char *)DIY_TONE_ALT_A, vol, NULL, 5) == 1
	    && diy_tone_alt_wait_tone_start(12000) == 0)
	{
		ok_recover = 1;
	}
	divoom_local_music_ctrl_stop_tone();
	divoom_system_delay_ms(200);
	(void)diy_tone_alt_wait_idle(5000);

	ok = ok_suppress && ok_reject && ok_recover;

	/* suppress 字段保留兼容 host analyzer；语义=DIY 下 duck overlay 成功 */
	printf("AUDIO_POLICY_TEST: ok=%d duck=%d suppress=%d reject=%d recover=%d policy_tone_ok=%d\n",
	       ok, ok_suppress, ok_suppress, ok_reject, ok_recover,
	       divoom_audio_policy_tone_play_allowed());

	return ok ? 0 : 1;
}

FINSH_FUNCTION_EXPORT_CMD(divoom_audio_policy_test_cli, audio_policy_test,
			  audio policy duck/reject/recover autotest);

#define ALARM_TONE_TEST_URI "file:///usr/share/divoom_app/tone/Clock_Normal1.mp3"

static int divoom_alarm_tone_test_cli(int argc, char **argv)
{
	int vol = 60;
	int ok = 0;

	if (argc >= 2)
	{
		vol = atoi(argv[1]);
	}
	divoom_local_music_ctrl_stop_tone();
	divoom_system_delay_ms(100);
#if defined(DIVOOM_TIME_FRAME_SPEAKER) && defined(DIVOOM_TIMES_VIBE)
	divoom_main_set_jl_mcu_tone_volume(vol);
#endif
	if (divoom_local_music_ctrl_tone_play((char *)ALARM_TONE_TEST_URI, vol, NULL, 5) == 1
	    && diy_tone_alt_wait_tone_start(5000) == 0)
	{
		ok = 1;
	}
	printf("ALARM_TONE_TEST: ok=%d uri=%s tone_active=%d\n",
	       ok, ALARM_TONE_TEST_URI, divoom_local_music_ctrl_get_tone_status());
	divoom_system_delay_ms(2000);
	divoom_local_music_ctrl_stop_tone();
	return ok ? 0 : 1;
}

FINSH_FUNCTION_EXPORT_CMD(divoom_alarm_tone_test_cli, alarm_tone_test,
			  alarm preview tone file-uri autotest);

#endif /* DIVOOM_FINSH_AUTOTEST */

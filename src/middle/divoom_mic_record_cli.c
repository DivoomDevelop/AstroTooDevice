#include "divoom_finsh_autotest.h"
#if DIVOOM_FINSH_AUTOTEST

/*
 * Finsh: mic_record_test — 助眠/闹钟/留言 MIC 录音 + tone 试听自动化
 *
 *   mic_record_test leave 3
 *   mic_record_test sleep 3
 *   mic_record_test alarm 3
 *   mic_record_test all 3
 */
#include "divoom.h"
#include <console.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "divoom_app_com.h"
#include "divoom_leave_msg_record.h"
#include "divoom_local_music_ctrl.h"
#include "divoom_main.h"
#include "divoom_mic_record.h"

#define MIC_RECORD_TEST_LEAVE_TMP  DIVOOM_LEAVE_MSG_RECORD_TMP_MP3
#define MIC_RECORD_TEST_SLEEP_TMP  DIVOOM_TMP_DIR "/divoom_sleep_aid_cap.mp3"
#define MIC_RECORD_TEST_ALARM_TMP  DIVOOM_TMP_DIR "/divoom_alarm_record_cap.mp3"

#define MIC_RECORD_TEST_SLEEP_MIN_BYTES  256
#define MIC_RECORD_TEST_ALARM_MIN_BYTES  128
#define MIC_RECORD_TEST_DEFAULT_SEC      3
#define MIC_RECORD_TEST_MAX_SEC          15
#define MIC_RECORD_TEST_DEFAULT_VOL      60

typedef enum {
	MIC_RECORD_TEST_CASE_LEAVE = 0,
	MIC_RECORD_TEST_CASE_SLEEP,
	MIC_RECORD_TEST_CASE_ALARM,
	MIC_RECORD_TEST_CASE_COUNT
} mic_record_test_case_t;

static int mic_record_test_wait_tone_start(unsigned int timeout_ms)
{
	unsigned int t0 = divoom_system_get_tickms();

	while (divoom_system_get_tickms() - t0 < timeout_ms) {
		if (divoom_local_music_ctrl_get_tone_status())
			return 1;
		divoom_system_delay_ms(10);
	}
	return 0;
}

static const char *mic_record_test_case_name(mic_record_test_case_t c)
{
	switch (c) {
	case MIC_RECORD_TEST_CASE_LEAVE:
		return "leave";
	case MIC_RECORD_TEST_CASE_SLEEP:
		return "sleep";
	case MIC_RECORD_TEST_CASE_ALARM:
		return "alarm";
	default:
		return "unknown";
	}
}

static const char *mic_record_test_case_path(mic_record_test_case_t c)
{
	switch (c) {
	case MIC_RECORD_TEST_CASE_LEAVE:
		return MIC_RECORD_TEST_LEAVE_TMP;
	case MIC_RECORD_TEST_CASE_SLEEP:
		return MIC_RECORD_TEST_SLEEP_TMP;
	case MIC_RECORD_TEST_CASE_ALARM:
		return MIC_RECORD_TEST_ALARM_TMP;
	default:
		return NULL;
	}
}

static int mic_record_test_case_min_bytes(mic_record_test_case_t c)
{
	switch (c) {
	case MIC_RECORD_TEST_CASE_LEAVE:
		return (int)DIVOOM_LEAVE_MSG_MIN_CLIP_BYTES;
	case MIC_RECORD_TEST_CASE_SLEEP:
		return MIC_RECORD_TEST_SLEEP_MIN_BYTES;
	case MIC_RECORD_TEST_CASE_ALARM:
		return MIC_RECORD_TEST_ALARM_MIN_BYTES;
	default:
		return 256;
	}
}

static void mic_record_test_cleanup(void)
{
	divoom_local_music_ctrl_stop_tone();
	divoom_local_music_ctrl_voice_clip_record_stop_sync();
	divoom_system_delay_ms(100);
}

static int mic_record_test_one(mic_record_test_case_t c, int record_sec, int vol)
{
	const char *name = mic_record_test_case_name(c);
	const char *path = mic_record_test_case_path(c);
	int min_bytes = mic_record_test_case_min_bytes(c);
	int rec_bytes = 0;
	int play_started = 0;
	int ok = 0;
	int play_time;

	if (!path || record_sec < 1)
		return 0;

	printf("MIC_RECORD_TEST: begin case=%s sec=%d path=%s\n", name, record_sec, path);
	mic_record_test_cleanup();
	divoom_app_com_del_file((char *)path);

	if (divoom_local_music_ctrl_voice_clip_record_start(path, 0, NULL) != 0) {
		printf("MIC_RECORD_TEST: case=%s ok=0 err=start_fail mic_active=%d\n",
		       name, divoom_mic_record_is_active());
		goto out;
	}

	divoom_system_delay_ms((unsigned int)record_sec * 1000u);
	divoom_local_music_ctrl_voice_clip_record_stop_sync();
	rec_bytes = divoom_app_com_get_file_size((char *)path);
	if (rec_bytes < min_bytes) {
		printf("MIC_RECORD_TEST: case=%s ok=0 err=small_file rec_bytes=%d min=%d\n",
		       name, rec_bytes, min_bytes);
		goto out;
	}

	play_time = record_sec + 2;
	if (play_time < 2)
		play_time = 2;
#if defined(DIVOOM_TIME_FRAME_SPEAKER) && defined(DIVOOM_TIMES_VIBE)
	divoom_main_set_jl_mcu_tone_volume(vol);
#endif
	if (divoom_local_music_ctrl_tone_play((char *)path, vol, NULL, play_time) != 1) {
		printf("MIC_RECORD_TEST: case=%s ok=0 err=tone_play_fail rec_bytes=%d\n",
		       name, rec_bytes);
		goto out;
	}
	play_started = mic_record_test_wait_tone_start(8000u);
	divoom_system_delay_ms(800);
	divoom_local_music_ctrl_stop_tone();
	ok = play_started ? 1 : 0;

out:
	printf("MIC_RECORD_TEST: case=%s ok=%d rec_bytes=%d play_started=%d mic_active=%d\n",
	       name, ok, rec_bytes, play_started, divoom_mic_record_is_active());
	mic_record_test_cleanup();
	divoom_app_com_del_file((char *)path);
	return ok;
}

static int mic_record_test_cli(int argc, char **argv)
{
	const char *which = "all";
	int record_sec = MIC_RECORD_TEST_DEFAULT_SEC;
	int vol = MIC_RECORD_TEST_DEFAULT_VOL;
	int fail = 0;
	int ok_count = 0;
	int i;

	if (argc >= 2)
		which = argv[1];
	if (argc >= 3)
		record_sec = atoi(argv[2]);
	if (argc >= 4)
		vol = atoi(argv[3]);

	if (record_sec < 1)
		record_sec = 1;
	if (record_sec > MIC_RECORD_TEST_MAX_SEC)
		record_sec = MIC_RECORD_TEST_MAX_SEC;
	if (vol < 0)
		vol = 0;
	if (vol > 100)
		vol = 100;

	printf("MIC_RECORD_TEST: suite begin which=%s sec=%d vol=%d jl_mic=%d\n",
	       which, record_sec, vol, divoom_mic_record_is_active());

	if (strcmp(which, "leave") == 0) {
		ok_count = mic_record_test_one(MIC_RECORD_TEST_CASE_LEAVE, record_sec, vol);
		fail = ok_count ? 0 : 1;
	} else if (strcmp(which, "sleep") == 0) {
		ok_count = mic_record_test_one(MIC_RECORD_TEST_CASE_SLEEP, record_sec, vol);
		fail = ok_count ? 0 : 1;
	} else if (strcmp(which, "alarm") == 0) {
		ok_count = mic_record_test_one(MIC_RECORD_TEST_CASE_ALARM, record_sec, vol);
		fail = ok_count ? 0 : 1;
	} else if (strcmp(which, "all") == 0) {
		for (i = 0; i < MIC_RECORD_TEST_CASE_COUNT; i++) {
			if (mic_record_test_one((mic_record_test_case_t)i, record_sec, vol))
				ok_count++;
			else
				fail++;
			divoom_system_delay_ms(500);
		}
	} else {
		printf("MIC_RECORD_TEST: usage: mic_record_test [leave|sleep|alarm|all] [sec] [vol]\n");
		return 1;
	}

	printf("MIC_RECORD_TEST: done ok=%d fail=%d\n", ok_count, fail);
	return fail ? 1 : 0;
}

FINSH_FUNCTION_EXPORT_CMD(mic_record_test_cli, mic_record_test,
			  leave/sleep/alarm mic record + tone preview autotest);

#endif /* DIVOOM_FINSH_AUTOTEST */

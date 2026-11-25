#include "divoom_finsh_autotest.h"
#if DIVOOM_FINSH_AUTOTEST

/*
 * Finsh: tone_cut — 快速切提示音压测（定位破音 / PA_DIAG）
 *
 *   tone_cut              # 默认 20 次切歌, gap=400ms, vol=50
 *   tone_cut 30 300 60    # 次数 gap_ms volume
 */
#include "divoom.h"
#include <console.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "divoom_local_music_ctrl.h"

#define TONE_CUT_A "/usr/share/divoom_app/tone/Clock_Normal1.mp3"
#define TONE_CUT_B "/usr/share/divoom_app/tone/Clock_Campanula.mp3"
#define TONE_CUT_PLAY_SEC 60

static int divoom_tone_cut_cli(int argc, char **argv)
{
	int n = 20;
	int gap_ms = 400;
	int vol = 50;
	int i;
	char *path;

	if (argc >= 2)
		n = atoi(argv[1]);
	if (argc >= 3)
		gap_ms = atoi(argv[2]);
	if (argc >= 4)
		vol = atoi(argv[3]);

	if (n < 1)
		n = 1;
	if (n > 200)
		n = 200;
	if (gap_ms < 50)
		gap_ms = 50;
	if (gap_ms > 5000)
		gap_ms = 5000;
	if (vol < 0)
		vol = 0;
	if (vol > 100)
		vol = 100;

	printf("tone_cut: n=%d gap_ms=%d vol=%d\n", n, gap_ms, vol);
	printf("tone_cut: A=%s\n", TONE_CUT_A);
	printf("tone_cut: B=%s\n", TONE_CUT_B);

	for (i = 0; i < n; i++) {
		path = (i & 1) ? TONE_CUT_B : TONE_CUT_A;
		printf("tone_cut: #%d %s\n", i + 1, path);
		divoom_local_music_ctrl_tone_play(path, vol, NULL, TONE_CUT_PLAY_SEC);
		divoom_system_delay_ms((unsigned int)gap_ms);
	}

	divoom_system_delay_ms(200);
	divoom_local_music_ctrl_stop_tone();
	printf("tone_cut: done n=%d\n", n);
	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(divoom_tone_cut_cli, tone_cut,
			  Rapid tone cut stress for pop debug);

#endif /* DIVOOM_FINSH_AUTOTEST */

#ifndef _divoom_tone_service_h_
#define _divoom_tone_service_h_

#include "divoom_local_music_ctrl_tone_priv.h"
#include "divoom_tone_lite_decode.h"

typedef enum {
	DIVOOM_TONE_END_EOF = 0,
	DIVOOM_TONE_END_STOP,
	DIVOOM_TONE_END_CUT_HOLD,
	DIVOOM_TONE_END_CUT_DONE,
} divoom_tone_end_reason_t;

void divoom_tone_service_finish_after_run(Pdivoom_local_music_ctrl_tina_music_item play_handle);

int divoom_tone_service_before_play(void);

divoom_tone_lite_play_mode_t divoom_tone_service_get_play_mode(void);

void divoom_tone_service_mark_cut_pending(void);

int divoom_tone_service_queue_has_pending_play(void);

void divoom_tone_service_session_end(Pdivoom_local_music_ctrl_tina_music_item play_handle,
				     divoom_tone_end_reason_t reason);

/* 释放 CH_TONE ring 与 TONE 租约（duck 恢复由 session EV_TONE_ENDED 负责） */
void divoom_tone_service_release_tone_bus(Pdivoom_local_music_ctrl_tina_music_item play_handle);

/** P3：play_worker 结束时是否保留 TONE sink（原 tone_lite keep_sink 业务逻辑） */
int divoom_tone_service_should_keep_sink(int stop_req, int eof,
					 divoom_tone_lite_play_mode_t mode);

#endif

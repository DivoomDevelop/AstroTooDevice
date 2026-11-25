#include "divoom.h"
#include "divoom_build_config.h"
#include "divoom_jl_mic_record.h"

#if DIVOOM_BUILD_RTOS_DEVICE && defined(DIVOOM_TIMES_VIBE)

#include "divoom_btcore_rec_stream.h"

#define JL_MIC_RECORD_STOP_SYNC_MS  6000u
#define JL_MIC_RECORD_MP3_BPS       4000u

typedef struct {
	divoom_mic_record_done_fn user_cb;
	uint32 start_tick_ms;
	uint32 max_sec;
} divoom_jl_mic_record_ctx_t;

static divoom_jl_mic_record_ctx_t s_jl_mic;

static uint32 divoom_jl_mic_record_estimate_duration_sec(const divoom_btcore_rec_stream_end_t *end)
{
	uint32 sec;

	if (end && end->total_bytes > 0u)
		sec = (end->total_bytes + JL_MIC_RECORD_MP3_BPS - 1u) / JL_MIC_RECORD_MP3_BPS;
	else
		sec = 0u;
	if (s_jl_mic.start_tick_ms != 0u) {
		uint32 elapsed = (divoom_system_get_tickms() - s_jl_mic.start_tick_ms) / 1000u;

		if (elapsed > 0u && (sec == 0u || elapsed < sec))
			sec = elapsed;
	}
	if (s_jl_mic.max_sec > 0u && sec > s_jl_mic.max_sec)
		sec = s_jl_mic.max_sec;
	return sec;
}

static void divoom_jl_mic_record_btcore_done(const char *path,
					     const divoom_btcore_rec_stream_end_t *end,
					     int ok)
{
	divoom_mic_record_done_fn cb = s_jl_mic.user_cb;
	uint32 dur = divoom_jl_mic_record_estimate_duration_sec(end);
	int result = ok ? 0 : -1;

	s_jl_mic.user_cb = NULL;
	s_jl_mic.start_tick_ms = 0u;
	divoom_print("jl_mic_record: done ok=%d path=%s bytes=%u dur=%u\n",
		     ok, path ? path : "(null)",
		     end ? (unsigned)end->total_bytes : 0u, (unsigned)dur);
	if (cb)
		cb(path, dur, result);
}

static void divoom_jl_mic_record_wait_idle(void)
{
	uint32 waited = 0u;

	while (divoom_jl_mic_record_is_active() && waited < JL_MIC_RECORD_STOP_SYNC_MS) {
		divoom_system_delay_ms(25);
		waited += 25u;
	}
}

int divoom_jl_mic_record_start(const char *path, uint32 duration_sec,
			       divoom_mic_record_done_fn done_cb)
{
	if (!path || !path[0])
		return -1;

	divoom_jl_mic_record_stop_sync();

	if (divoom_btcore_rec_stream_set_save_path(path) != 0)
		return -1;

	s_jl_mic.user_cb = done_cb;
	s_jl_mic.start_tick_ms = divoom_system_get_tickms();
	s_jl_mic.max_sec = duration_sec;
	divoom_btcore_rec_stream_set_done_callback(divoom_jl_mic_record_btcore_done);

	if (divoom_btcore_rec_stream_send_record(1u, 0u) != 0) {
		s_jl_mic.user_cb = NULL;
		s_jl_mic.start_tick_ms = 0u;
		divoom_btcore_rec_stream_set_done_callback(NULL);
		return -1;
	}
	divoom_print("jl_mic_record: start path=%s max_sec=%u\n", path, (unsigned)duration_sec);
	return 0;
}

void divoom_jl_mic_record_stop(void)
{
	if (!divoom_jl_mic_record_is_active())
		return;
	divoom_btcore_rec_stream_send_record(0u, 0u);
}

void divoom_jl_mic_record_stop_sync(void)
{
	if (!divoom_jl_mic_record_is_active())
		return;
	divoom_jl_mic_record_stop();
	divoom_jl_mic_record_wait_idle();
}

int divoom_jl_mic_record_is_active(void)
{
	return divoom_btcore_rec_stream_is_active();
}

#elif DIVOOM_BUILD_RTOS_DEVICE

int divoom_jl_mic_record_start(const char *path, uint32 duration_sec,
			       divoom_mic_record_done_fn done_cb)
{
	(void)path;
	(void)duration_sec;
	(void)done_cb;
	return -1;
}

void divoom_jl_mic_record_stop(void)
{
}

void divoom_jl_mic_record_stop_sync(void)
{
}

int divoom_jl_mic_record_is_active(void)
{
	return 0;
}

#endif /* DIVOOM_BUILD_RTOS_DEVICE */

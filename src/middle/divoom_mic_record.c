#include "divoom.h"
#include "divoom_build_config.h"
#include "divoom_mic_record.h"
#include "divoom_pcm_record.h"
#include "divoom_jl_mic_record.h"
#include <string.h>
#include "divoom_port_os.h"

#ifndef DIVOOM_OPENSOURCE_SWITCH
static void *s_stream_lock;
static void *s_stream_control;
static unsigned s_record_generation, s_observer_generation;
static divoom_mic_record_stream_fn s_stream_cb;
int divoom_mic_record_set_stream_callback(divoom_mic_record_stream_fn cb)
{
    int rc=0;
    if (!s_stream_lock) s_stream_lock=divoom_port_mutex_create();
    if (!s_stream_control) s_stream_control=divoom_port_mutex_create();
    if (!s_stream_lock || !s_stream_control) return -1;
    divoom_port_mutex_lock(s_stream_control);
    divoom_port_mutex_lock(s_stream_lock);
    if (cb && s_stream_cb) rc=-1;
    else { s_stream_cb=cb; s_observer_generation=s_record_generation+1; }
    divoom_port_mutex_unlock(s_stream_lock);
    divoom_port_mutex_unlock(s_stream_control);
    return rc;
}
int divoom_mic_record_stream_is_current(divoom_mic_record_stream_fn cb)
{
    int current;
    if (!s_stream_lock) return 0;
    divoom_port_mutex_lock(s_stream_lock);
    current=s_stream_cb==cb && s_observer_generation==s_record_generation;
    divoom_port_mutex_unlock(s_stream_lock);
    return current;
}
void divoom_mic_record_stream_stop_sync(divoom_mic_record_stream_fn cb)
{
    if (!s_stream_control) return;
    divoom_port_mutex_lock(s_stream_control);
    if (divoom_mic_record_stream_is_current(cb)) divoom_mic_record_stop_sync();
    divoom_port_mutex_unlock(s_stream_control);
}
void divoom_mic_record_stream_data(const void *p,unsigned n,int format,unsigned rate)
{
    if (!s_stream_lock) return;
    divoom_port_mutex_lock(s_stream_lock);
    if (s_stream_cb && s_observer_generation==s_record_generation) s_stream_cb(p,n,format,rate);
    divoom_port_mutex_unlock(s_stream_lock);
}
#else
int divoom_mic_record_set_stream_callback(divoom_mic_record_stream_fn cb)
{ (void)cb; return -1; }
int divoom_mic_record_stream_is_current(divoom_mic_record_stream_fn cb) { (void)cb; return 0; }
void divoom_mic_record_stream_stop_sync(divoom_mic_record_stream_fn cb) { (void)cb; }
void divoom_mic_record_stream_data(const void *p,unsigned n,int f,unsigned r)
{ (void)p; (void)n; (void)f; (void)r; }
#endif

#define DIVOOM_MIC_RECORD_MP3_BITRATE_KBPS 32

static divoom_mic_record_done_fn s_mic_pcm_done_bridge;

static void divoom_mic_record_pcm_done_bridge(const char *file_path, uint32 duration_sec, int result)
{
	divoom_mic_record_done_fn cb = s_mic_pcm_done_bridge;

	s_mic_pcm_done_bridge = NULL;
	if (cb)
		cb(file_path, duration_sec, result);
}

int divoom_mic_record_start(const char *path, uint32 duration_sec,
			    divoom_mic_record_done_fn done_cb)
{
	int ret;
	if (!path || !path[0])
		return -1;

#ifndef DIVOOM_OPENSOURCE_SWITCH
    if (s_stream_control) divoom_port_mutex_lock(s_stream_control);
#endif
	divoom_mic_record_stop_sync();
#ifndef DIVOOM_OPENSOURCE_SWITCH
    if (s_stream_lock) divoom_port_mutex_lock(s_stream_lock);
    s_record_generation++;
    if (s_stream_lock) divoom_port_mutex_unlock(s_stream_lock);
#endif

#if DIVOOM_BUILD_SIMULATOR
	s_mic_pcm_done_bridge = done_cb;
	ret = divoom_pcm_record_start(path, duration_sec, DIVOOM_PCM_RECORD_FMT_MP3,
				       DIVOOM_MIC_RECORD_MP3_BITRATE_KBPS,
				       done_cb ? divoom_mic_record_pcm_done_bridge : NULL);
#elif DIVOOM_BUILD_RTOS_DEVICE
	/* DisableMic 仅关噪音/律动消费，不限制 JL 录音（留言/闹钟/助眠等） */
	ret = divoom_jl_mic_record_start(path, duration_sec, done_cb);
#else
	(void)duration_sec;
	(void)done_cb;
	ret = -1;
#endif
#ifndef DIVOOM_OPENSOURCE_SWITCH
    if (s_stream_control) divoom_port_mutex_unlock(s_stream_control);
#endif
	return ret;
}

void divoom_mic_record_stop(void)
{
#if DIVOOM_BUILD_SIMULATOR
	divoom_pcm_record_stop();
#elif DIVOOM_BUILD_RTOS_DEVICE
	divoom_jl_mic_record_stop();
#endif
}

void divoom_mic_record_stop_sync(void)
{
#if DIVOOM_BUILD_SIMULATOR
	divoom_pcm_record_stop_sync();
#elif DIVOOM_BUILD_RTOS_DEVICE
	divoom_jl_mic_record_stop_sync();
#endif
}

int divoom_mic_record_is_active(void)
{
#if DIVOOM_BUILD_SIMULATOR
	return divoom_pcm_record_is_active();
#elif DIVOOM_BUILD_RTOS_DEVICE
	return divoom_jl_mic_record_is_active();
#else
	return 0;
#endif
}

/* 对齐 jl701n divoom_os_com_music_file_get_duration_sec */
static const uint32 s_mic_adts_sample_rates[] = {
	96000u, 88200u, 64000u, 48000u,
	44100u, 32000u, 24000u, 22050u,
	16000u, 12000u, 11025u, 8000u, 7350u
};

static int divoom_mic_record_header_is_adts(const uint8 *header)
{
	int layer;

	if (!header || header[0] != 0xFFu || (header[1] & 0xF0u) != 0xF0u)
		return 0;
	layer = (int)((header[1] >> 1) & 0x03u);
	return layer == 0;
}

static uint32 divoom_mic_record_adts_duration_sec(void *fp, uint32 file_size, const uint8 *first_hdr)
{
	uint8 header[7];
	uint8 sr_idx;
	uint8 first_sr_idx;
	uint32 sample_rate;
	uint32 frame_len;
	uint32 pos;
	uint32 duration;
	uint32 tot_samples;

	memcpy(header, first_hdr, 7);
	sr_idx = (uint8)((header[2] >> 2) & 0x0Fu);
	if (sr_idx >= (uint8)(sizeof(s_mic_adts_sample_rates) / sizeof(s_mic_adts_sample_rates[0])))
		return 0u;
	sample_rate = s_mic_adts_sample_rates[sr_idx];
	if (sample_rate == 0u)
		return 0u;
	frame_len = ((uint32)(header[3] & 0x03u) << 11) | ((uint32)header[4] << 3)
		    | ((uint32)header[5] >> 5);
	if (frame_len < 7u || frame_len > file_size)
		return 0u;

	first_sr_idx = sr_idx;
	tot_samples = 1024u;
	pos = frame_len;
	while (pos + 7u <= file_size) {
		if (divoom_port_file_seek(fp, (long)pos, DIVOOM_PORT_SEEK_SET) != 0)
			break;
		if (divoom_port_file_read(header, 1, 7, fp) != 7)
			break;
		if (!divoom_mic_record_header_is_adts(header))
			break;
		sr_idx = (uint8)((header[2] >> 2) & 0x0Fu);
		if (sr_idx != first_sr_idx
		    || sr_idx >= (uint8)(sizeof(s_mic_adts_sample_rates) / sizeof(s_mic_adts_sample_rates[0])))
			break;
		if (s_mic_adts_sample_rates[sr_idx] == 0u)
			break;
		frame_len = ((uint32)(header[3] & 0x03u) << 11) | ((uint32)header[4] << 3)
			    | ((uint32)header[5] >> 5);
		if (frame_len < 7u || pos + frame_len > file_size || pos + frame_len < pos)
			break;
		if (tot_samples > (0xFFFFFFFFu - 1024u))
			break;
		tot_samples += 1024u;
		pos += frame_len;
	}

	duration = tot_samples / sample_rate;
	if (duration == 0u && tot_samples != 0u)
		duration = 1u;
	return duration;
}

uint32 divoom_mic_record_file_duration_sec(const char *path)
{
	void *fp;
	uint8 header[7];
	int sz;
	uint32 file_size;
	uint32 sec;

	if (!path || !path[0])
		return 0u;
	sz = divoom_app_com_get_file_size((char *)path);
	if (sz < 128)
		return 0u;
	file_size = (uint32)sz;
	fp = divoom_port_file_open((char *)path, "rb");
	if (!fp)
		return divoom_mic_record_mp3_cbr32_sec(sz);
	if (divoom_port_file_read(header, 1, 7, fp) != 7) {
		divoom_port_file_close(fp);
		return divoom_mic_record_mp3_cbr32_sec(sz);
	}
	if (divoom_mic_record_header_is_adts(header)) {
		sec = divoom_mic_record_adts_duration_sec(fp, file_size, header);
		divoom_port_file_close(fp);
		return sec;
	}
	divoom_port_file_close(fp);
	return divoom_mic_record_mp3_cbr32_sec(sz);
}

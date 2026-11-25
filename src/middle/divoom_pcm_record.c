/* divoom_pcm_record：仅模拟器/VM 录音后端；真机 MIC 走 divoom_mic_record → divoom_jl_mic_record。 */
#include "divoom.h"
#include "divoom_mic_record.h"
#include "divoom_pcm_record.h"

#if DIVOOM_PIXEL_EQ_ENABLE || DIVOOM_BUILD_SIMULATOR
#include "divoom_pcm_mp3_encode.h"
#include "divoom_app_com.h"
#include "divoom_read_pcm.h"
#include "divoom_eq_noise_mic_fft.h"

#include <stdio.h>
#include <string.h>

#define DIVOOM_PCM_RECORD_WAV_HDR_SIZE  44u
#define DIVOOM_PCM_RECORD_CHANNELS      1u
#define DIVOOM_PCM_RECORD_BITS          16u
#define DIVOOM_PCM_RECORD_PATH_MAX      256
#define DIVOOM_PCM_RECORD_MIN_MP3_BYTES 128u
/** 录音 PCM 数字增益（仅写入文件，不影响 FFT 噪声计） */
#define DIVOOM_PCM_RECORD_SW_GAIN       6

typedef struct _divoom_pcm_record_ctrl
{
	uint8 active;
	uint32 pcm_bytes;
	uint32 pcm_feed_count;
	uint8 finalize_pending;
	uint8 finalize_busy;
	divoom_pcm_record_format_e format;
	int mp3_bitrate_kbps;
	void *lock;
	FILE *fp;
	divoom_pcm_mp3_encoder_t *mp3_enc;
	char file_path[DIVOOM_PCM_RECORD_PATH_MAX];
	uint32 req_duration_sec;
	uint32 sample_rate;
	uint32 max_pcm_bytes;
	divoom_pcm_record_done_callback done_cb;
} divoom_pcm_record_ctrl, *Pdivoom_pcm_record_ctrl;

static divoom_pcm_record_ctrl s_pcm_record_ctrl;

static void divoom_pcm_record_put_u32_le(uint8 *p, uint32 v);
static void divoom_pcm_record_put_u16_le(uint8 *p, uint16 v);
static int divoom_pcm_record_write_wav_header(FILE *fp, uint32 sample_rate, uint16 channels,
					      uint16 bits, uint32 data_size);
static void divoom_pcm_record_ensure_parent_dir(const char *file_path);
static void divoom_pcm_record_restore_default_callback(void);
static void divoom_pcm_record_finalize_locked(Pdivoom_pcm_record_ctrl c, int invoke_done_cb);
static void divoom_pcm_record_request_finalize(Pdivoom_pcm_record_ctrl c);
static void divoom_pcm_record_pcm_data(uint16 *pcm_data, int len);
static int divoom_pcm_record_feed_pcm(Pdivoom_pcm_record_ctrl c, uint16 *pcm_data, int len);

static void divoom_pcm_record_put_u32_le(uint8 *p, uint32 v)
{
	p[0] = (uint8)(v & 0xFFu);
	p[1] = (uint8)((v >> 8) & 0xFFu);
	p[2] = (uint8)((v >> 16) & 0xFFu);
	p[3] = (uint8)((v >> 24) & 0xFFu);
}

static void divoom_pcm_record_put_u16_le(uint8 *p, uint16 v)
{
	p[0] = (uint8)(v & 0xFFu);
	p[1] = (uint8)((v >> 8) & 0xFFu);
}

static int divoom_pcm_record_write_wav_header(FILE *fp, uint32 sample_rate, uint16 channels,
					      uint16 bits, uint32 data_size)
{
	uint8 hdr[DIVOOM_PCM_RECORD_WAV_HDR_SIZE];
	uint32 byte_rate;
	uint16 block_align;
	uint32 riff_size;

	if (!fp)
	{
		return -1;
	}

	block_align = (uint16)(channels * bits / 8u);
	byte_rate = sample_rate * (uint32)block_align;
	riff_size = 36u + data_size;

	memcpy(hdr + 0, "RIFF", 4);
	divoom_pcm_record_put_u32_le(hdr + 4, riff_size);
	memcpy(hdr + 8, "WAVE", 4);
	memcpy(hdr + 12, "fmt ", 4);
	divoom_pcm_record_put_u32_le(hdr + 16, 16u);
	divoom_pcm_record_put_u16_le(hdr + 20, 1u);
	divoom_pcm_record_put_u16_le(hdr + 22, channels);
	divoom_pcm_record_put_u32_le(hdr + 24, sample_rate);
	divoom_pcm_record_put_u32_le(hdr + 28, byte_rate);
	divoom_pcm_record_put_u16_le(hdr + 32, block_align);
	divoom_pcm_record_put_u16_le(hdr + 34, bits);
	memcpy(hdr + 36, "data", 4);
	divoom_pcm_record_put_u32_le(hdr + 40, data_size);

	if (divoom_port_file_seek(fp, 0, SEEK_SET) != 0)
	{
		return -1;
	}
	if (divoom_port_file_write(hdr, 1, sizeof(hdr), fp) != sizeof(hdr))
	{
		return -1;
	}
	return 0;
}

static void divoom_pcm_record_ensure_parent_dir(const char *file_path)
{
	char dir[DIVOOM_PCM_RECORD_PATH_MAX];
	char *slash;

	if (!file_path || file_path[0] == '\0')
	{
		return;
	}
	strncpy(dir, file_path, sizeof(dir) - 1u);
	dir[sizeof(dir) - 1u] = '\0';
	slash = strrchr(dir, '/');
	if (slash)
	{
		*slash = '\0';
		divoom_app_com_check_and_mk_dir(dir);
	}
}

static void divoom_pcm_record_restore_default_callback(void)
{
	divoom_eq_noise_mic_fft_register_pcm_callback();
}

static void divoom_pcm_record_apply_sw_gain(uint16 *pcm_data, int len)
{
	int16_t *samples;
	int sample_count;
	int i;

	if (!pcm_data || len <= 0 || DIVOOM_PCM_RECORD_SW_GAIN <= 1)
	{
		return;
	}

	samples = (int16_t *)pcm_data;
	sample_count = len / (int)sizeof(int16_t);
	for (i = 0; i < sample_count; i++)
	{
		int32 v;

		v = (int32)samples[i] * DIVOOM_PCM_RECORD_SW_GAIN;
		if (v > 32767)
		{
			v = 32767;
		}
		else if (v < -32768)
		{
			v = -32768;
		}
		samples[i] = (int16_t)v;
	}
}

static int divoom_pcm_record_feed_pcm(Pdivoom_pcm_record_ctrl c, uint16 *pcm_data, int len)
{
	if (!c || !pcm_data || len <= 0)
	{
		return -1;
	}

	if (c->format == DIVOOM_PCM_RECORD_FMT_MP3)
	{
		int sample_count = len / (int)sizeof(int16_t);

		if (!c->mp3_enc)
		{
			return -1;
		}
		if (divoom_pcm_mp3_encode_feed(c->mp3_enc, (const int16_t *)pcm_data, sample_count) != 0)
		{
			return -1;
		}
	}
	else
	{
		if (!c->fp)
		{
			return -1;
		}
		if (divoom_port_file_write(pcm_data, 1, (size_t)len, c->fp) != (size_t)len)
		{
			return -1;
		}
	}

	c->pcm_bytes += (uint32)len;
	return 0;
}

static void divoom_pcm_record_finalize_locked(Pdivoom_pcm_record_ctrl c, int invoke_done_cb)
{
	char path[DIVOOM_PCM_RECORD_PATH_MAX];
	divoom_pcm_record_done_callback done_cb = NULL;
	uint32 req_duration_sec = 0;
	uint32 sample_rate = 0;
	uint32 pcm_bytes = 0;
	uint32 pcm_feed_count = 0;
	uint32 actual_duration_sec = 0;
	divoom_pcm_record_format_e format = DIVOOM_PCM_RECORD_FMT_WAV;
	int result = -1;
	FILE *fp = NULL;
	divoom_pcm_mp3_encoder_t *mp3_enc = NULL;

	if (!c || !c->active)
	{
		return;
	}

	c->active = 0;
	c->finalize_pending = 0;

	strncpy(path, c->file_path, sizeof(path) - 1u);
	path[sizeof(path) - 1u] = '\0';
	done_cb = c->done_cb;
	req_duration_sec = c->req_duration_sec;
	sample_rate = c->sample_rate;
	pcm_bytes = c->pcm_bytes;
	pcm_feed_count = c->pcm_feed_count;
	format = c->format;
	fp = c->fp;
	mp3_enc = c->mp3_enc;

	c->fp = NULL;
	c->mp3_enc = NULL;
	c->done_cb = NULL;
	c->file_path[0] = '\0';
	c->pcm_bytes = 0;
	c->pcm_feed_count = 0;
	c->max_pcm_bytes = 0;
	c->req_duration_sec = 0;
	c->format = DIVOOM_PCM_RECORD_FMT_WAV;
	c->mp3_bitrate_kbps = 0;

	divoom_pcm_record_restore_default_callback();

	if (format == DIVOOM_PCM_RECORD_FMT_MP3)
	{
		if (mp3_enc && pcm_bytes > 0u)
		{
			if (divoom_pcm_mp3_encode_close(&mp3_enc) == 0)
			{
				int file_sz = divoom_app_com_get_file_size(path);

				if (file_sz >= (int)DIVOOM_PCM_RECORD_MIN_MP3_BYTES)
				{
					result = 0;
				}
			}
		}
		else if (mp3_enc)
		{
			divoom_pcm_mp3_encode_close(&mp3_enc);
		}
		if (fp)
		{
			divoom_port_file_close(fp);
		}
	}
	else
	{
		if (fp)
		{
			if (pcm_bytes > 0u)
			{
				if (divoom_pcm_record_write_wav_header(fp, sample_rate,
								       DIVOOM_PCM_RECORD_CHANNELS,
								       DIVOOM_PCM_RECORD_BITS,
								       pcm_bytes) == 0)
				{
					result = 0;
				}
			}
			divoom_port_file_close(fp);
		}
	}

	if (sample_rate > 0u && pcm_bytes > 0u)
	{
		uint32 bytes_per_sec = sample_rate * DIVOOM_PCM_RECORD_CHANNELS
				       * (DIVOOM_PCM_RECORD_BITS / 8u);

		actual_duration_sec = pcm_bytes / bytes_per_sec;
		if (actual_duration_sec == 0u)
		{
			actual_duration_sec = 1u;
		}
	}
	else
	{
		actual_duration_sec = req_duration_sec;
	}

	if (result != 0)
	{
		divoom_print("divoom_pcm_record: finalize fail pcm_bytes=%u feeds=%u file_sz=%d path=%s\n",
			     (unsigned)pcm_bytes, (unsigned)pcm_feed_count,
			     divoom_app_com_get_file_size(path), path);
		divoom_app_com_del_file(path);
	}

	if (invoke_done_cb && done_cb)
	{
		done_cb(path, actual_duration_sec, result);
	}
}

static void *divoom_pcm_record_finalize_task(void *para)
{
	Pdivoom_pcm_record_ctrl c = (Pdivoom_pcm_record_ctrl)para;

	if (!c || !c->lock)
	{
		return NULL;
	}

	c->finalize_busy = 1;
	divoom_system_get_lock(c->lock);
	divoom_pcm_record_finalize_locked(c, 1);
	divoom_system_free_lock(c->lock);
	c->finalize_busy = 0;
	return NULL;
}

static void divoom_pcm_record_request_finalize(Pdivoom_pcm_record_ctrl c)
{
	if (!c || !c->lock)
	{
		return;
	}

	divoom_system_get_lock(c->lock);
	if (!c->active || c->finalize_pending)
	{
		divoom_system_free_lock(c->lock);
		return;
	}
	c->finalize_pending = 1;
	divoom_system_free_lock(c->lock);

	divoom_system_create_task(divoom_pcm_record_finalize_task, "pcm_finalize", c, DIVOOM_TASK_STACK_PCM_FINALIZE);
}

static void divoom_pcm_record_pcm_data(uint16 *pcm_data, int len)
{
	Pdivoom_pcm_record_ctrl c = &s_pcm_record_ctrl;

	if (!pcm_data || len <= 0)
	{
		return;
	}

	divoom_system_get_lock(c->lock);
	if (!c->active || c->finalize_pending)
	{
		divoom_system_free_lock(c->lock);
		return;
	}

	if (c->format == DIVOOM_PCM_RECORD_FMT_WAV && !c->fp)
	{
		divoom_system_free_lock(c->lock);
		return;
	}
	if (c->format == DIVOOM_PCM_RECORD_FMT_MP3 && !c->mp3_enc)
	{
		divoom_system_free_lock(c->lock);
		return;
	}

	divoom_pcm_record_apply_sw_gain(pcm_data, len);
	divoom_mic_record_stream_data(pcm_data, (unsigned)len, 0, c->sample_rate);

	if (divoom_pcm_record_feed_pcm(c, pcm_data, len) != 0)
	{
		divoom_system_free_lock(c->lock);
		divoom_pcm_record_request_finalize(c);
		return;
	}
	c->pcm_feed_count++;

	if (c->max_pcm_bytes > 0u && c->pcm_bytes >= c->max_pcm_bytes)
	{
		divoom_system_free_lock(c->lock);
		divoom_pcm_record_request_finalize(c);
		return;
	}

	divoom_system_free_lock(c->lock);
}

int divoom_pcm_record_start(const char *file_path, uint32 duration_sec,
			    divoom_pcm_record_format_e format, int mp3_bitrate_kbps,
			    divoom_pcm_record_done_callback done_cb)
{
	Pdivoom_pcm_record_ctrl c = &s_pcm_record_ctrl;
	uint32 sample_rate;
	uint32 max_pcm_bytes = 0;

	if (!file_path || file_path[0] == '\0')
	{
		return -1;
	}
	if (format != DIVOOM_PCM_RECORD_FMT_WAV && format != DIVOOM_PCM_RECORD_FMT_MP3)
	{
		return -1;
	}

	if (!c->lock)
	{
		c->lock = divoom_system_create_lock();
		if (!c->lock)
		{
			return -1;
		}
	}

	divoom_pcm_record_stop_sync();

	if (divoom_pcm_ensure_capture() != 0)
	{
		divoom_print("%s: divoom_pcm_ensure_capture failed\n", __func__);
		return -1;
	}

	divoom_pcm_apply_capture_gain();
	sample_rate = divoom_pcm_get_sample_rate();
	if (sample_rate == 0u)
	{
		divoom_print("%s: pcm sample_rate still 0 after ensure_capture\n", __func__);
		return -1;
	}

	if (duration_sec > 0u)
	{
		uint32 bytes_per_sec = sample_rate * DIVOOM_PCM_RECORD_CHANNELS
				       * (DIVOOM_PCM_RECORD_BITS / 8u);

		max_pcm_bytes = duration_sec * bytes_per_sec;
	}

	divoom_pcm_record_ensure_parent_dir(file_path);
	divoom_app_com_del_file((char *)file_path);

	divoom_system_get_lock(c->lock);
	c->fp = divoom_port_file_open(file_path, "wb");
	if (!c->fp)
	{
		divoom_system_free_lock(c->lock);
		divoom_print("%s: fopen failed %s\n", __func__, file_path);
		return -1;
	}

	if (format == DIVOOM_PCM_RECORD_FMT_MP3)
	{
		c->mp3_enc = divoom_pcm_mp3_encode_open(c->fp, sample_rate, mp3_bitrate_kbps);
		if (!c->mp3_enc)
		{
			divoom_port_file_close(c->fp);
			c->fp = NULL;
			divoom_system_free_lock(c->lock);
			divoom_print("%s: mp3 encoder open failed %s\n", __func__, file_path);
			return -1;
		}
	}
	else
	{
		if (divoom_pcm_record_write_wav_header(c->fp, sample_rate, DIVOOM_PCM_RECORD_CHANNELS,
						       DIVOOM_PCM_RECORD_BITS, 0) != 0)
		{
			divoom_port_file_close(c->fp);
			c->fp = NULL;
			divoom_system_free_lock(c->lock);
			divoom_print("%s: write wav header failed %s\n", __func__, file_path);
			return -1;
		}
	}

	strncpy(c->file_path, file_path, sizeof(c->file_path) - 1u);
	c->file_path[sizeof(c->file_path) - 1u] = '\0';
	c->req_duration_sec = duration_sec;
	c->sample_rate = sample_rate;
	c->pcm_bytes = 0;
	c->max_pcm_bytes = max_pcm_bytes;
	c->done_cb = done_cb;
	c->format = format;
	c->mp3_bitrate_kbps = mp3_bitrate_kbps;
	c->finalize_pending = 0;
	c->pcm_feed_count = 0;
	c->active = 1;
	divoom_system_free_lock(c->lock);

	divoom_pcm_set_callback(divoom_pcm_record_pcm_data);
	return 0;
}

void divoom_pcm_record_stop(void)
{
	Pdivoom_pcm_record_ctrl c = &s_pcm_record_ctrl;

	if (!c->lock)
	{
		return;
	}

	divoom_system_get_lock(c->lock);
	if (!c->active)
	{
		divoom_system_free_lock(c->lock);
		return;
	}
	divoom_system_free_lock(c->lock);

	divoom_pcm_record_request_finalize(c);
}

void divoom_pcm_record_wait_idle(void)
{
	Pdivoom_pcm_record_ctrl c = &s_pcm_record_ctrl;
	uint32 elapsed_ms = 0;

	while (elapsed_ms < 5000u)
	{
		uint8 busy = 0;

		if (!c->lock)
		{
			break;
		}
		divoom_system_get_lock(c->lock);
		if (c->active || c->finalize_pending || c->finalize_busy)
		{
			busy = 1;
		}
		divoom_system_free_lock(c->lock);
		if (!busy)
		{
			break;
		}
		divoom_system_delay_ms(20);
		elapsed_ms += 20u;
	}
}

void divoom_pcm_record_stop_sync(void)
{
	divoom_pcm_record_stop();
	divoom_pcm_record_wait_idle();
}

int divoom_pcm_record_is_active(void)
{
	Pdivoom_pcm_record_ctrl c = &s_pcm_record_ctrl;
	int active = 0;

	if (!c->lock)
	{
		return 0;
	}

	divoom_system_get_lock(c->lock);
	active = c->active ? 1 : 0;
	divoom_system_free_lock(c->lock);
	return active;
}
#else
int divoom_pcm_record_start(const char *file_path, uint32 duration_sec, divoom_pcm_record_format_e format, int mp3_bitrate_kbps, divoom_pcm_record_done_callback done_cb) { (void)file_path; (void)duration_sec; (void)format; (void)mp3_bitrate_kbps; (void)done_cb; return -1; }
void divoom_pcm_record_stop(void) {}
void divoom_pcm_record_stop_sync(void) {}
void divoom_pcm_record_wait_idle(void) {}
int divoom_pcm_record_is_active(void) { return 0; }
#endif

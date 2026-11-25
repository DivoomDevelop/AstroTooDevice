#include "divoom.h"
#include "divoom_pcm_mp3_encode.h"
#include "shine_mp3.h"

#include <string.h>

struct _divoom_pcm_mp3_encoder
{
	FILE *fp;
	shine_t shine;
	int samples_per_pass;
	int16_t *pending;
	int pending_cap;
	int pending_count;
};

static int divoom_pcm_mp3_encode_write_frame(divoom_pcm_mp3_encoder_t *enc, int16_t *pcm, int sample_count)
{
	unsigned char *mp3_data = NULL;
	int written = 0;
	int16_t *encode_buf = pcm;
	int16_t *pad_buf = NULL;

	if (!enc || !enc->shine || !enc->fp || !pcm || sample_count <= 0)
	{
		return -1;
	}

	if (sample_count < enc->samples_per_pass)
	{
		pad_buf = (int16_t *)divoom_system_malloc((uint32)enc->samples_per_pass * sizeof(int16_t));
		if (!pad_buf)
		{
			return -1;
		}
		memset(pad_buf, 0, (size_t)enc->samples_per_pass * sizeof(int16_t));
		memcpy(pad_buf, pcm, (size_t)sample_count * sizeof(int16_t));
		encode_buf = pad_buf;
	}

	mp3_data = shine_encode_buffer_interleaved(enc->shine, encode_buf, &written);
	if (pad_buf)
	{
		divoom_system_free(pad_buf);
	}
	if (written > 0 && mp3_data)
	{
		if (divoom_port_file_write(mp3_data, 1, (size_t)written, enc->fp) != (size_t)written)
		{
			return -1;
		}
	}
	return 0;
}

static int divoom_pcm_mp3_encode_drain_pending(divoom_pcm_mp3_encoder_t *enc)
{
	while (enc && enc->pending_count >= enc->samples_per_pass)
	{
		if (divoom_pcm_mp3_encode_write_frame(enc, enc->pending, enc->samples_per_pass) != 0)
		{
			return -1;
		}
		enc->pending_count -= enc->samples_per_pass;
		if (enc->pending_count > 0)
		{
			memmove(enc->pending, enc->pending + enc->samples_per_pass,
				(size_t)enc->pending_count * sizeof(int16_t));
		}
	}
	return 0;
}

divoom_pcm_mp3_encoder_t *divoom_pcm_mp3_encode_open(FILE *fp, uint32 sample_rate, int bitrate_kbps)
{
	divoom_pcm_mp3_encoder_t *enc = NULL;
	shine_config_t config;

	if (!fp || sample_rate == 0u)
	{
		return NULL;
	}

	if (bitrate_kbps <= 0)
	{
		bitrate_kbps = DIVOOM_PCM_MP3_ENCODE_BITRATE_DEFAULT;
	}

	enc = (divoom_pcm_mp3_encoder_t *)divoom_system_malloc(sizeof(divoom_pcm_mp3_encoder_t));
	if (!enc)
	{
		return NULL;
	}

	memset(enc, 0, sizeof(divoom_pcm_mp3_encoder_t));
	enc->fp = fp;

	shine_set_config_mpeg_defaults(&config.mpeg);
	config.wave.samplerate = (int)sample_rate;
	config.wave.channels = PCM_MONO;
	config.mpeg.mode = MONO;
	config.mpeg.bitr = bitrate_kbps;

	if (shine_check_config(config.wave.samplerate, config.mpeg.bitr) < 0)
	{
		divoom_print("%s: unsupported samplerate/bitrate %u/%d\n", __func__, sample_rate, bitrate_kbps);
		divoom_system_free(enc);
		return NULL;
	}

	enc->shine = shine_initialise(&config);
	if (!enc->shine)
	{
		divoom_system_free(enc);
		return NULL;
	}

	enc->samples_per_pass = shine_samples_per_pass(enc->shine);
	enc->pending_cap = enc->samples_per_pass * 2;
	enc->pending = (int16_t *)divoom_system_malloc((uint32)enc->pending_cap * sizeof(int16_t));
	if (!enc->pending)
	{
		divoom_pcm_mp3_encode_close(&enc);
		return NULL;
	}

	return enc;
}

int divoom_pcm_mp3_encode_feed(divoom_pcm_mp3_encoder_t *enc, const int16_t *pcm, int sample_count)
{
	int offset = 0;

	if (!enc || !enc->shine || !pcm || sample_count <= 0)
	{
		return -1;
	}

	while (offset < sample_count)
	{
		int copy_count;
		int room;

		if (divoom_pcm_mp3_encode_drain_pending(enc) != 0)
		{
			return -1;
		}

		room = enc->pending_cap - enc->pending_count;
		if (room <= 0)
		{
			divoom_print("%s: pending buffer full\n", __func__);
			return -1;
		}

		copy_count = sample_count - offset;
		if (copy_count > room)
		{
			copy_count = room;
		}

		memcpy(enc->pending + enc->pending_count, pcm + offset,
		       (size_t)copy_count * sizeof(int16_t));
		enc->pending_count += copy_count;
		offset += copy_count;

		if (divoom_pcm_mp3_encode_drain_pending(enc) != 0)
		{
			return -1;
		}
	}

	return 0;
}

int divoom_pcm_mp3_encode_close(divoom_pcm_mp3_encoder_t **enc_ptr)
{
	divoom_pcm_mp3_encoder_t *enc;
	unsigned char *mp3_data = NULL;
	int written = 0;

	if (!enc_ptr || !*enc_ptr)
	{
		return -1;
	}

	enc = *enc_ptr;
	*enc_ptr = NULL;

	if (enc->shine)
	{
		if (enc->pending_count > 0)
		{
			if (divoom_pcm_mp3_encode_write_frame(enc, enc->pending, enc->pending_count) != 0)
			{
				shine_close(enc->shine);
				enc->shine = NULL;
				if (enc->pending)
				{
					divoom_system_free(enc->pending);
				}
				divoom_system_free(enc);
				return -1;
			}
		}

		mp3_data = shine_flush(enc->shine, &written);
		if (written > 0 && mp3_data && enc->fp)
		{
			if (divoom_port_file_write(mp3_data, 1, (size_t)written, enc->fp) != (size_t)written)
			{
				shine_close(enc->shine);
				enc->shine = NULL;
				if (enc->pending)
				{
					divoom_system_free(enc->pending);
				}
				divoom_system_free(enc);
				return -1;
			}
		}
		shine_close(enc->shine);
		enc->shine = NULL;
	}

	if (enc->pending)
	{
		divoom_system_free(enc->pending);
	}
	divoom_system_free(enc);
	return 0;
}

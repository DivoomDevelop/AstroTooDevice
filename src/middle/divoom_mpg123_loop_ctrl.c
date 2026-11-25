#include "divoom.h"
#include "divoom_mpg123_loop_ctrl.h"
#include <string.h>

#ifndef DIVOOM_MPG123_LOOP_HAS_LIB
#if DIVOOM_BUILD_SIMULATOR && defined(__has_include)
#if __has_include(<mpg123.h>)
#define DIVOOM_MPG123_LOOP_HAS_LIB 1
#endif
#endif
#endif

#ifndef DIVOOM_MPG123_LOOP_HAS_LIB
#define DIVOOM_MPG123_LOOP_HAS_LIB 0
#endif

#if DIVOOM_BUILD_SIMULATOR && DIVOOM_MPG123_LOOP_HAS_LIB
#include <mpg123.h>

/** 循环接缝 crossfade：200ms @ 16kHz */
#define DIVOOM_MPG123_LOOP_CROSSFADE_SAMPLES 3200
/** 去掉 MP3 编解码首尾 padding（约 32ms） */
#define DIVOOM_MPG123_LOOP_TRIM_SAMPLES      512

typedef struct _divoom_mpg123_loop_item
{
	int16_t *pcm;
	int pcm_bytes;
	int loop_bytes;
	int read_pos;
	int vol_percent;
} divoom_mpg123_loop_item, *Pdivoom_mpg123_loop_item;

static int s_mpg123_lib_inited;

static void divoom_mpg123_loop_ensure_lib_init(void)
{
	if (s_mpg123_lib_inited)
	{
		return;
	}
	if (mpg123_init() == MPG123_OK)
	{
		s_mpg123_lib_inited = 1;
	}
}

static void divoom_mpg123_loop_apply_vol(uint8 *buf, int size, int vol_percent)
{
	int i;
	int n_samples;
	T_S16 *pcm;

	if (!buf || size <= 0 || vol_percent >= 100)
	{
		return;
	}
	if (vol_percent <= 0)
	{
		memset(buf, 0, (size_t)size);
		return;
	}
	pcm = (T_S16 *)buf;
	n_samples = size / (int)sizeof(T_S16);
	for (i = 0; i < n_samples; i++)
	{
		pcm[i] = (T_S16)(((T_S32)pcm[i] * vol_percent) / 100);
	}
}

static void divoom_mpg123_loop_seam_crossfade(int16_t *pcm, int n_samples)
{
	int i;

	if (!pcm || n_samples <= DIVOOM_MPG123_LOOP_CROSSFADE_SAMPLES * 2)
	{
		return;
	}
	for (i = 0; i < DIVOOM_MPG123_LOOP_CROSSFADE_SAMPLES; i++)
	{
		float t = (float)i / (float)DIVOOM_MPG123_LOOP_CROSSFADE_SAMPLES;
		float fade_in = t * t;
		float fade_out = (1.0f - t) * (1.0f - t);
		int32_t mixed;

		mixed = (int32_t)((float)pcm[n_samples - DIVOOM_MPG123_LOOP_CROSSFADE_SAMPLES + i] * fade_out
		                  + (float)pcm[i] * fade_in);
		if (mixed > 32767)
		{
			mixed = 32767;
		}
		else if (mixed < -32768)
		{
			mixed = -32768;
		}
		pcm[n_samples - DIVOOM_MPG123_LOOP_CROSSFADE_SAMPLES + i] = (int16_t)mixed;
		pcm[i] = (int16_t)mixed;
	}
}

static void divoom_mpg123_loop_trim_edges(int16_t *pcm, int *pcm_bytes)
{
	int trim_bytes;
	int n_samples;

	if (!pcm || !pcm_bytes || *pcm_bytes <= 0)
	{
		return;
	}
	trim_bytes = DIVOOM_MPG123_LOOP_TRIM_SAMPLES * (int)sizeof(int16_t);
	n_samples = *pcm_bytes / (int)sizeof(int16_t);
	if (n_samples <= DIVOOM_MPG123_LOOP_TRIM_SAMPLES * 2 + DIVOOM_MPG123_LOOP_CROSSFADE_SAMPLES * 2)
	{
		return;
	}
	memmove(pcm, pcm + DIVOOM_MPG123_LOOP_TRIM_SAMPLES,
	        (size_t)(n_samples - DIVOOM_MPG123_LOOP_TRIM_SAMPLES * 2) * sizeof(int16_t));
	*pcm_bytes -= trim_bytes * 2;
}

static int divoom_mpg123_loop_decode_all(mpg123_handle *mh, int16_t **out_pcm, int *out_bytes)
{
	int16_t *buf = NULL;
	int cap_bytes = 0;
	int len_bytes = 0;
	uint8 chunk[4096];
	size_t done;
	int ret;

	*out_pcm = NULL;
	*out_bytes = 0;
	if (!mh)
	{
		return -1;
	}

	for (;;)
	{
		done = 0;
		ret = mpg123_read(mh, chunk, sizeof(chunk), &done);
		if (ret == MPG123_NEW_FORMAT)
		{
			continue;
		}
		if (ret == MPG123_OK || ret == MPG123_NEED_MORE)
		{
			if (done == 0)
			{
				if (ret == MPG123_NEED_MORE)
				{
					continue;
				}
				break;
			}
			if (len_bytes + (int)done > cap_bytes)
			{
				int new_cap = cap_bytes ? cap_bytes * 2 : 65536;
				int16_t *new_buf;

				while (new_cap < len_bytes + (int)done)
				{
					new_cap *= 2;
				}
				new_buf = (int16_t *)divoom_system_malloc((size_t)new_cap);
				if (!new_buf)
				{
					divoom_system_free(buf);
					return -1;
				}
				if (buf && len_bytes > 0)
				{
					memcpy(new_buf, buf, (size_t)len_bytes);
					divoom_system_free(buf);
				}
				buf = new_buf;
				cap_bytes = new_cap;
			}
			memcpy((uint8 *)buf + (size_t)len_bytes, chunk, done);
			len_bytes += (int)done;
			continue;
		}
		if (ret == MPG123_DONE)
		{
			break;
		}
		divoom_print("mpg123 loop decode err: %s (code %d)\n", mpg123_strerror(mh), ret);
		divoom_system_free(buf);
		return -1;
	}

	if (len_bytes < (int)sizeof(int16_t))
	{
		divoom_system_free(buf);
		return -1;
	}
	*out_pcm = buf;
	*out_bytes = len_bytes;
	return 0;
}

void *divoom_mpg123_loop_open(const char *mp3_path, int vol_percent)
{
	Pdivoom_mpg123_loop_item item;
	mpg123_handle *mh = NULL;
	int err;

	if (!mp3_path || !mp3_path[0])
	{
		return NULL;
	}
	divoom_mpg123_loop_ensure_lib_init();
	if (!s_mpg123_lib_inited)
	{
		divoom_print("mpg123 loop: lib init fail\n");
		return NULL;
	}

	item = (Pdivoom_mpg123_loop_item)divoom_system_malloc(sizeof(divoom_mpg123_loop_item));
	if (!item)
	{
		return NULL;
	}
	memset(item, 0, sizeof(*item));

	mh = mpg123_new(NULL, &err);
	if (!mh)
	{
		divoom_print("mpg123 loop: mpg123_new fail err=%d\n", err);
		divoom_system_free(item);
		return NULL;
	}
	if (mpg123_open(mh, mp3_path) != MPG123_OK)
	{
		divoom_print("mpg123 loop: open fail: %s (%s)\n", mp3_path, mpg123_strerror(mh));
		mpg123_delete(mh);
		divoom_system_free(item);
		return NULL;
	}
	mpg123_format_none(mh);
	if (mpg123_format(mh, DIVOOM_MPG123_LOOP_OUT_RATE, MPG123_MONO, MPG123_ENC_SIGNED_16) != MPG123_OK)
	{
		divoom_print("mpg123 loop: format fail: %s\n", mpg123_strerror(mh));
		mpg123_close(mh);
		mpg123_delete(mh);
		divoom_system_free(item);
		return NULL;
	}

	if (divoom_mpg123_loop_decode_all(mh, &item->pcm, &item->pcm_bytes) != 0)
	{
		divoom_print("mpg123 loop: decode fail: %s\n", mp3_path);
		mpg123_close(mh);
		mpg123_delete(mh);
		divoom_system_free(item);
		return NULL;
	}
	mpg123_close(mh);
	mpg123_delete(mh);

	divoom_mpg123_loop_trim_edges(item->pcm, &item->pcm_bytes);
	divoom_mpg123_loop_seam_crossfade(item->pcm, item->pcm_bytes / (int)sizeof(int16_t));
	item->loop_bytes = item->pcm_bytes - DIVOOM_MPG123_LOOP_CROSSFADE_SAMPLES * (int)sizeof(int16_t);
	if (item->loop_bytes <= (int)sizeof(int16_t))
	{
		item->loop_bytes = item->pcm_bytes;
	}

	if (vol_percent < 0)
	{
		vol_percent = 0;
	}
	else if (vol_percent > 100)
	{
		vol_percent = 100;
	}
	item->vol_percent = vol_percent;
	item->read_pos = 0;
	divoom_print("mpg123 loop open: %s pcm_bytes=%d loop_bytes=%d vol=%d\n",
	             mp3_path, item->pcm_bytes, item->loop_bytes, vol_percent);
	return item;
}

int divoom_mpg123_loop_read(void *handle, uint8 *buf, int size)
{
	Pdivoom_mpg123_loop_item item = (Pdivoom_mpg123_loop_item)handle;
	int total;
	int copy_bytes;

	if (!item || !item->pcm || item->loop_bytes <= 0 || !buf || size <= 0)
	{
		return 0;
	}

	total = 0;
	while (total < size)
	{
		if (item->read_pos >= item->loop_bytes)
		{
			item->read_pos = 0;
		}
		copy_bytes = item->loop_bytes - item->read_pos;
		if (copy_bytes > size - total)
		{
			copy_bytes = size - total;
		}
		memcpy(buf + total, (uint8 *)item->pcm + (size_t)item->read_pos, (size_t)copy_bytes);
		item->read_pos += copy_bytes;
		total += copy_bytes;
	}

	divoom_mpg123_loop_apply_vol(buf, total, item->vol_percent);
	return total;
}

void divoom_mpg123_loop_set_vol(void *handle, int vol_percent)
{
	Pdivoom_mpg123_loop_item item = (Pdivoom_mpg123_loop_item)handle;

	if (!item)
	{
		return;
	}
	if (vol_percent < 0)
	{
		vol_percent = 0;
	}
	else if (vol_percent > 100)
	{
		vol_percent = 100;
	}
	item->vol_percent = vol_percent;
}

void divoom_mpg123_loop_close(void *handle)
{
	Pdivoom_mpg123_loop_item item = (Pdivoom_mpg123_loop_item)handle;

	if (!item)
	{
		return;
	}
	if (item->pcm)
	{
		divoom_system_free(item->pcm);
	}
	divoom_system_free(item);
}

#elif DIVOOM_BUILD_SIMULATOR || DIVOOM_BUILD_RTOS_DEVICE

/*
 * 当前构建未提供 libmpg123 时返回 NULL，上层 sleep-aid 逻辑会回退到 tone_play。
 * 后续接入 libmpg123 后，模拟器会自动启用上方真实循环解码分支。
 */
void *divoom_mpg123_loop_open(const char *mp3_path, int vol_percent)
{
	(void)mp3_path;
	(void)vol_percent;
	divoom_print("mpg123 loop: stub (libmpg123 not linked)\n");
	return NULL;
}

int divoom_mpg123_loop_read(void *handle, uint8 *buf, int size)
{
	(void)handle;
	(void)buf;
	(void)size;
	return 0;
}

void divoom_mpg123_loop_set_vol(void *handle, int vol_percent)
{
	(void)handle;
	(void)vol_percent;
}

void divoom_mpg123_loop_close(void *handle)
{
	(void)handle;
}

#endif

#include "divoom_build_config.h"
#if !DIVOOM_BUILD_RTOS_DEVICE
#error "RTOS device-only; exclude from simulator VS project."
#endif

#include "divoom.h"
#include "divoom_snd_pcm.h"
#include "divoom_audio_sink.h"
#include "divoom_audio_buf_pool.h"
#include "divoom_audio_session.h"

#define DIVOOM_SND_PCM_IN_RATE           16000u
#define DIVOOM_SND_PCM_OUT_RATE          48000u
#define DIVOOM_SND_PCM_OUT_CH            2u
#define DIVOOM_SND_PCM_UPSAMPLE          (DIVOOM_SND_PCM_OUT_RATE / DIVOOM_SND_PCM_IN_RATE) /* 3 */
#define DIVOOM_SND_PCM_RING_CHUNK_BYTES  2048u
#define DIVOOM_SND_PCM_IN_FRAMES         (DIVOOM_SND_PCM_RING_CHUNK_BYTES / sizeof(int16_t))
#define DIVOOM_SND_PCM_OUT_CAP_FRAMES    (DIVOOM_SND_PCM_IN_FRAMES * DIVOOM_SND_PCM_UPSAMPLE + 64u)

#define DIVOOM_SND_PCM_OUT_PERIOD_FRAMES   512u

#if (DIVOOM_SND_PCM_OUT_RATE % DIVOOM_SND_PCM_IN_RATE) != 0
#error snd_pcm: OUT_RATE must be integer multiple of IN_RATE
#endif
#if DIVOOM_SND_PCM_UPSAMPLE != 3
#error snd_pcm: expected 16k->48k (x3); update upsample loop if rates change
#endif

typedef struct _divoom_snd_pcm_ctrl
{
	unsigned int in_frames;
	int run_flag;
	void *sem;
	int16_t *mono_buf;
	int16_t *stereo_out;
	unsigned int frames_pushed;
	unsigned int ring_underruns;
} divoom_snd_pcm_ctrl, *Pdivoom_snd_pcm_ctrl;

static Pdivoom_snd_pcm_ctrl pdivoom_snd_pcm_handle;

void divoom_snd_pcm_mono_to_stereo(const int16_t *mono, int16_t *stereo, unsigned int frames)
{
	unsigned int i;

	if (!mono || !stereo)
	{
		return;
	}
	for (i = 0; i < frames; i++)
	{
		stereo[i * 2] = mono[i];
		stereo[i * 2 + 1] = mono[i];
	}
}

unsigned int divoom_snd_pcm_resample_out_frames(unsigned int in_frames)
{
	return in_frames * DIVOOM_SND_PCM_UPSAMPLE;
}

static int divoom_snd_pcm_push_frames(Pdivoom_snd_pcm_ctrl h, const int16_t *stereo, unsigned int frames)
{
	if (!h || !stereo || frames == 0)
	{
		return -1;
	}
	return divoom_audio_sink_push_all(DIVOOM_AUDIO_SINK_CH_SLEEP, stereo, frames);
}

static int divoom_snd_pcm_push_silence_period(Pdivoom_snd_pcm_ctrl h)
{
	if (!h)
	{
		return -1;
	}
	return divoom_audio_sink_push_silence(DIVOOM_AUDIO_SINK_CH_SLEEP,
					      DIVOOM_SND_PCM_OUT_PERIOD_FRAMES);
}

static int divoom_snd_pcm_process_chunk(Pdivoom_snd_pcm_ctrl h, const uint8 *mono_bytes, unsigned int bytes)
{
	unsigned int in_frames;
	unsigned int out_frames;
	unsigned int i;

	if (!h || !mono_bytes || bytes < sizeof(int16_t))
	{
		return -1;
	}
	if (!divoom_audio_sink_is_running())
	{
		return -1;
	}

	in_frames = bytes / (unsigned int)sizeof(int16_t);
	if (in_frames > DIVOOM_SND_PCM_IN_FRAMES)
	{
		in_frames = DIVOOM_SND_PCM_IN_FRAMES;
	}
	memcpy(h->mono_buf, mono_bytes, in_frames * sizeof(int16_t));

	/* 16k→48k 恰好 ×3：每样点复制 3 次并扩成立体声，避免每输出样点做除法 */
	out_frames = in_frames * DIVOOM_SND_PCM_UPSAMPLE;
	for (i = 0; i < in_frames; i++)
	{
		int16_t s = h->mono_buf[i];
		int16_t *o = &h->stereo_out[i * DIVOOM_SND_PCM_UPSAMPLE * DIVOOM_SND_PCM_OUT_CH];

		o[0] = s;
		o[1] = s;
		o[2] = s;
		o[3] = s;
		o[4] = s;
		o[5] = s;
	}
	if (divoom_snd_pcm_push_frames(h, h->stereo_out, out_frames) != 0)
	{
		return -1;
	}
	h->frames_pushed += out_frames;
	return 0;
}

static void divoom_snd_pcm_release_buffers(Pdivoom_snd_pcm_ctrl h)
{
	if (!h)
	{
		return;
	}
	if (h->mono_buf)
	{
		divoom_system_free(h->mono_buf);
		h->mono_buf = NULL;
	}
	if (h->stereo_out)
	{
		divoom_system_free(h->stereo_out);
		h->stereo_out = NULL;
	}
}

static int divoom_snd_pcm_alloc_buffers(Pdivoom_snd_pcm_ctrl h)
{
	if (!h)
	{
		return -1;
	}
	h->mono_buf = (int16_t *)divoom_system_malloc(DIVOOM_SND_PCM_IN_FRAMES * sizeof(int16_t));
	h->stereo_out = (int16_t *)divoom_system_malloc(DIVOOM_SND_PCM_OUT_CAP_FRAMES * DIVOOM_SND_PCM_OUT_CH * sizeof(int16_t));
	if (!h->mono_buf || !h->stereo_out)
	{
		divoom_snd_pcm_release_buffers(h);
		return -1;
	}
	return 0;
}

void *divoom_snd_pcm_task_main(void *data)
{
	Pdivoom_snd_pcm_ctrl h = pdivoom_snd_pcm_handle;
	uint8 chunk[DIVOOM_SND_PCM_RING_CHUNK_BYTES];
	int primed = 0;

	(void)data;

	while (h && h->run_flag)
	{
		if (divoom_mix_pcm_ring_get(chunk, DIVOOM_SND_PCM_RING_CHUNK_BYTES))
		{
			primed = 1;
			if (divoom_snd_pcm_process_chunk(h, chunk, DIVOOM_SND_PCM_RING_CHUNK_BYTES) != 0)
			{
				divoom_system_delay_ms(2);
			}
		}
		else
		{
			if (primed)
			{
				h->ring_underruns++;
				if (divoom_snd_pcm_push_silence_period(h) != 0)
				{
					divoom_system_delay_ms(2);
				}
			}
			else
			{
				divoom_system_delay_ms(2);
			}
		}
	}

	if (h)
	{
		divoom_system_put_sem(h->sem);
	}
	return NULL;
}

void divoom_snd_pcm_init(void)
{
	Pdivoom_snd_pcm_ctrl h;

	if (pdivoom_snd_pcm_handle)
	{
		return;
	}

	/*
	 * 复用全局那条常开的流，只启用自己的通道。
	 * 停流再建流会在 JL 接收端产生咔哒（JL 音量拉 0 也挡不住），
	 * 助眠进出和随后的提示音冷开都会响，所以这里绝不 stop。
	 */
	if (divoom_audio_sink_start() != 0)
	{
		divoom_print("snd_pcm: audio_sink start fail\n");
		return;
	}
	/* 复用保活流：先让出 TONE 租约，再租 SLEEP */
	divoom_audio_session_release_tone_bus("snd_pcm_init");
	if (divoom_audio_buf_pool_is_leased(DIVOOM_AUDIO_BUS_SLEEP))
	{
		divoom_print("snd_pcm: stale SLEEP lease, recover\n");
		divoom_audio_sink_force_release_bus(DIVOOM_AUDIO_SINK_CH_SLEEP);
	}
	if (divoom_audio_buf_pool_acquire(DIVOOM_AUDIO_BUS_SLEEP, "sleep") != 0)
	{
		divoom_print("snd_pcm: SLEEP bus lease fail\n");
		return;
	}
	divoom_audio_sink_ch_enable(DIVOOM_AUDIO_SINK_CH_SLEEP, 1);
	if (!divoom_audio_sink_ch_ring_bound(DIVOOM_AUDIO_SINK_CH_SLEEP))
	{
		divoom_audio_sink_force_release_bus(DIVOOM_AUDIO_SINK_CH_SLEEP);
		return;
	}
	divoom_audio_sink_ch_set_volume(DIVOOM_AUDIO_SINK_CH_SLEEP, 100);
	divoom_audio_sink_ch_fade_in(DIVOOM_AUDIO_SINK_CH_SLEEP, 0, 100);

	h = (Pdivoom_snd_pcm_ctrl)divoom_system_malloc(sizeof(divoom_snd_pcm_ctrl));
	if (!h)
	{
		divoom_audio_sink_ch_disable(DIVOOM_AUDIO_SINK_CH_SLEEP);
		divoom_audio_sink_force_release_bus(DIVOOM_AUDIO_SINK_CH_SLEEP);
		return;
	}
	memset(h, 0, sizeof(*h));
	h->in_frames = DIVOOM_SND_PCM_IN_FRAMES;
	if (divoom_snd_pcm_alloc_buffers(h) != 0)
	{
		divoom_audio_sink_ch_disable(DIVOOM_AUDIO_SINK_CH_SLEEP);
		divoom_audio_sink_force_release_bus(DIVOOM_AUDIO_SINK_CH_SLEEP);
		divoom_system_free(h);
		return;
	}

	h->run_flag = 1;
	h->sem = divoom_system_create_sem(1, 0);
	pdivoom_snd_pcm_handle = h;
	if (!h->sem ||
		!divoom_system_create_task(divoom_snd_pcm_task_main, "snd_pcm", NULL, DIVOOM_TASK_STACK_SND_PCM))
	{
		pdivoom_snd_pcm_handle = NULL;
		if (h->sem)
		{
			divoom_system_destroy_sem(h->sem);
		}
		divoom_snd_pcm_release_buffers(h);
		divoom_system_free(h);
		divoom_audio_sink_force_release_bus(DIVOOM_AUDIO_SINK_CH_SLEEP);
		return;
	}
	divoom_print("snd_pcm: init ok\n");
}

void divoom_snd_pcm_close(void)
{
	Pdivoom_snd_pcm_ctrl h;
	unsigned int frames_pushed;
	unsigned int underruns;

	h = pdivoom_snd_pcm_handle;
	if (!h)
	{
		return;
	}

	h->run_flag = 0;
	/* 已提交的一小段PCM可以完成；超时则关闭通道，等生产者确认退出。 */
	if (!divoom_system_get_sem_with_time(h->sem, 2))
	{
		divoom_print("snd_pcm: close wait timeout\n");
		divoom_audio_sink_ch_disable(DIVOOM_AUDIO_SINK_CH_SLEEP);
		divoom_system_get_sem(h->sem);
	}
	frames_pushed = h->frames_pushed;
	underruns = h->ring_underruns;
	pdivoom_snd_pcm_handle = NULL;
	divoom_system_destroy_sem(h->sem);
	divoom_snd_pcm_release_buffers(h);
	divoom_system_free(h);

	divoom_audio_sink_ch_disable(DIVOOM_AUDIO_SINK_CH_SLEEP);
	divoom_audio_sink_force_release_bus(DIVOOM_AUDIO_SINK_CH_SLEEP);
	/* 助眠 duck 叠播时 TONE 仍占用 BUS_TONE，勿 tip 拆 SLEEP 租约状态 */
	if (!divoom_audio_sink_ch_active(DIVOOM_AUDIO_SINK_CH_TONE))
	{
		divoom_audio_session_release_sleep_bus("snd_pcm_close");
	}

	divoom_print("snd_pcm: close ok pushed=%u underruns=%u sink_out=%u\n",
	             frames_pushed, underruns, divoom_audio_sink_whitebox_out_frames());
}

int divoom_snd_pcm_whitebox_is_running(void)
{
	return (pdivoom_snd_pcm_handle && pdivoom_snd_pcm_handle->run_flag) ? 1 : 0;
}

unsigned int divoom_snd_pcm_whitebox_out_frames(void)
{
	return divoom_audio_sink_whitebox_out_frames();
}

unsigned int divoom_snd_pcm_whitebox_underruns(void)
{
	if (!pdivoom_snd_pcm_handle)
	{
		return 0;
	}
	return pdivoom_snd_pcm_handle->ring_underruns
	       + divoom_audio_sink_whitebox_underruns(DIVOOM_AUDIO_SINK_CH_SLEEP);
}

unsigned int divoom_snd_pcm_whitebox_silence_periods(void)
{
	return divoom_audio_sink_whitebox_silence_periods();
}

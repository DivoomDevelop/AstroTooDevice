#include "divoom_build_config.h"

#if DIVOOM_BUILD_RTOS_DEVICE

#include "divoom.h"
#include "divoom_audio_sink.h"
#include "divoom_audio_buf_pool.h"
#include "divoom_audio_policy.h"
#include "divoom_ring_buffer.h"

#include <aw-tiny-sound-lib/sunxi_tiny_sound_pcm.h>
#include <sound_v2/sound_adf_ksound.h>
#include <sunxi_hal_sound.h>
#include <stdint.h>
#include <pthread.h>

#define DIVOOM_AUDIO_SINK_RATE           48000u
#define DIVOOM_AUDIO_SINK_CH             2u
#define DIVOOM_AUDIO_SINK_CARD_NAME      "soundi2s0"
#define DIVOOM_AUDIO_SINK_PERIOD_FRAMES  512u
#define DIVOOM_AUDIO_SINK_BUFFER_FRAMES  2048u
#define DIVOOM_AUDIO_SINK_FADE_MS        DIVOOM_AUDIO_SINK_FADE_MS_NORMAL
#define DIVOOM_AUDIO_SINK_CUT_FADE_MS    DIVOOM_AUDIO_SINK_FADE_MS_CUT
#define DIVOOM_AUDIO_SINK_DIAG_BURST_JUMP  8000
#define DIVOOM_AUDIO_SINK_WRITE_ERR_LIMIT  64u

#define DIVOOM_AUDIO_SINK_FADE_INTERLEAVED(ms) \
	(((DIVOOM_AUDIO_SINK_RATE * (ms)) / 1000u) * DIVOOM_AUDIO_SINK_CH)

typedef struct {
	uint8_t enabled;
	uint32_t period_cnt;
	uint32_t clip_cnt;
	uint32_t bound_jump_cnt;
	uint32_t starve_cnt;
	uint32_t starve_to_pcm_cnt;
	uint32_t fade_jump_cnt;
	uint32_t dual_mix_cnt;
	uint32_t large_in_cnt;
	uint32_t log_suppress;
	int16_t last_out_l;
	int16_t last_out_r;
	uint8_t prev_has_pcm[DIVOOM_AUDIO_SINK_CH_COUNT];
	uint8_t prev_env[DIVOOM_AUDIO_SINK_CH_COUNT];
} divoom_audio_sink_diag_t;

typedef struct {
	void *ring;
	uint32_t ring_capacity;
	uint32 prebuffer_bytes; /* 仅 NETWORK 使用，140毫秒 PCM；0 表示直接播放/EOF排空。 */
	uint8 buffering;
	uint8_t enabled;
	uint8_t vol_target;
	uint8_t duck_mul;
	uint8_t env_mul;
	uint32_t fade_in_rem;
	uint32_t fade_in_total;
	uint8_t fade_in_from;
	uint32_t fade_out_rem;
	uint32_t fade_out_total;
	uint8_t fade_out_from;
	uint8_t fade_out_active;
	uint32_t underruns;
	int ring_inited;
} divoom_audio_sink_ch_state_t;

typedef struct {
	struct sunxi_pcm *pcm;
	unsigned int card;
	int run_flag;
	int stop_req;
	void *sem;
	void *lock;
	unsigned int out_frames_written;
	unsigned int silence_periods;
	divoom_audio_sink_ch_state_t ch[DIVOOM_AUDIO_SINK_CH_COUNT];
	int16_t *ch_buf[DIVOOM_AUDIO_SINK_CH_COUNT];
	int16_t mix_buf[DIVOOM_AUDIO_SINK_PERIOD_FRAMES * DIVOOM_AUDIO_SINK_CH];
} divoom_audio_sink_ctrl_t;

static divoom_audio_sink_ctrl_t *s_sink;
static divoom_audio_sink_diag_t s_sink_diag;
static unsigned int s_bus_force_release_cnt;

static unsigned int divoom_audio_sink_fade_interleaved(unsigned int ms)
{
	if (ms == 0)
	{
		return 0;
	}
	return DIVOOM_AUDIO_SINK_FADE_INTERLEAVED(ms);
}

static unsigned int divoom_audio_sink_ch_ring_fill_pct_locked(const divoom_audio_sink_ch_state_t *c)
{
	if (!c || !c->ring || c->ring_capacity == 0)
	{
		return 0;
	}
	return (divoom_ring_buffer_getdatasize(c->ring) * 100u) / c->ring_capacity;
}

static int divoom_audio_sink_ch_needs_mix(const divoom_audio_sink_ch_state_t *c)
{
	if (!c)
	{
		return 0;
	}
	return c->enabled || c->fade_out_active
	       || (c->ring && divoom_ring_buffer_getdatasize(c->ring) > 0);
}

static int divoom_audio_sink_any_active_locked(const divoom_audio_sink_ctrl_t *h)
{
	unsigned int i;

	if (!h)
	{
		return 0;
	}
	for (i = 0; i < DIVOOM_AUDIO_SINK_CH_COUNT; i++)
	{
		if (divoom_audio_sink_ch_needs_mix(&h->ch[i]))
		{
			return 1;
		}
	}
	return 0;
}

static divoom_audio_bus_t divoom_audio_sink_ch_to_bus(divoom_audio_sink_ch_t ch_idx)
{
	if (ch_idx == DIVOOM_AUDIO_SINK_CH_SLEEP)
		return DIVOOM_AUDIO_BUS_SLEEP;
#if DIVOOM_SHOW_TF_NAND
	if (ch_idx == DIVOOM_AUDIO_SINK_CH_MUSIC)
		return DIVOOM_AUDIO_BUS_MUSIC;
#endif
#ifdef CONFIG_DIVOOM_MPP_NET_AUDIO
    if (ch_idx == DIVOOM_AUDIO_SINK_CH_NETWORK)
        return DIVOOM_AUDIO_BUS_NETWORK;
#endif
	return DIVOOM_AUDIO_BUS_TONE;
}

static int divoom_audio_sink_ch_ensure_ring(divoom_audio_sink_ch_state_t *c,
					    divoom_audio_sink_ch_t ch_idx)
{
	divoom_audio_bus_t bus;
	void *ring;

	if (!c)
	{
		return -1;
	}
	if (c->ring_inited)
	{
#ifdef CONFIG_DIVOOM_MPP_NET_AUDIO
        if (ch_idx == DIVOOM_AUDIO_SINK_CH_NETWORK) {
            c->duck_mul = divoom_audio_policy_network_muted() ? 0 : 100;
            return 0;
        }
#endif
		/* 复用 ring 时仍保证 duck 默认满幅（duck_mul 已参与 mix，勿留 0） */
#if DIVOOM_SHOW_TF_NAND
		if (ch_idx == DIVOOM_AUDIO_SINK_CH_MUSIC
		    && divoom_audio_policy_show_music_muted())
		{
			c->duck_mul = 0;
			return 0;
		}
#endif
		if (c->duck_mul == 0)
		{
			c->duck_mul = 100;
		}
		return 0;
	}
	bus = divoom_audio_sink_ch_to_bus(ch_idx);
	ring = divoom_audio_buf_pool_ring(bus);
	if (!ring)
	{
		divoom_print("audio_sink: ensure_ring no lease ch=%u bus=%u\n",
			     (unsigned int)ch_idx, (unsigned)bus);
		return -1;
	}
	c->ring = ring;
	c->ring_inited = 1;
	c->ring_capacity = DIVOOM_AUDIO_BUF_POOL_RING_BYTES;
	c->vol_target = 100;
	c->duck_mul = 100;
#if DIVOOM_SHOW_TF_NAND
	if (ch_idx == DIVOOM_AUDIO_SINK_CH_MUSIC)
	{
		c->ring_capacity = DIVOOM_AUDIO_BUF_POOL_MUSIC_RING_BYTES;
		if (divoom_audio_policy_show_music_muted())
		{
			c->duck_mul = 0;
		}
	}
#endif
#ifdef CONFIG_DIVOOM_MPP_NET_AUDIO
    if (ch_idx == DIVOOM_AUDIO_SINK_CH_NETWORK)
    {
        c->ring_capacity = DIVOOM_AUDIO_BUF_POOL_NETWORK_RING_BYTES;
        /* 实测冷启动首批解码间隔约130ms；先填140ms再开声，仍用原32KiB队列，
         * 只利用已有输出余量，网络抖动仍由压缩流缓存承担。 */
        c->prebuffer_bytes = 140u * 48000u * 2u * sizeof(int16_t) / 1000u;
        c->buffering = 1;
        c->duck_mul = divoom_audio_policy_network_muted() ? 0 : 100;
    }
#endif
	c->env_mul = 0;
	return 0;
}

static void divoom_audio_sink_ch_free_ring(divoom_audio_sink_ch_state_t *c)
{
	if (!c || !c->ring_inited)
	{
		return;
	}
	/* 数据区与控制块归 buf_pool 所有，此处仅拆通道绑定 */
	c->ring = NULL;
	c->ring_inited = 0;
	c->ring_capacity = 0;
	c->prebuffer_bytes = 0;
	c->buffering = 0;
}

static void divoom_audio_sink_ch_reset_fade_in(divoom_audio_sink_ch_state_t *c, unsigned int from_mul)
{
	unsigned int total;

	if (!c)
	{
		return;
	}
	if (from_mul > 100)
	{
		from_mul = 100;
	}
	total = divoom_audio_sink_fade_interleaved(DIVOOM_AUDIO_SINK_FADE_MS);
	c->fade_in_from = (uint8_t)from_mul;
	c->fade_in_total = total;
	c->fade_in_rem = total;
	c->fade_out_rem = 0;
	c->fade_out_total = 0;
	c->fade_out_active = 0;
	c->env_mul = (uint8_t)from_mul;
}

static void divoom_audio_sink_ch_start_fade_out(divoom_audio_sink_ch_state_t *c, unsigned int ms)
{
	unsigned int total;

	if (!c)
	{
		return;
	}
	total = divoom_audio_sink_fade_interleaved(ms);
	if (total == 0)
	{
		total = 1;
	}
	c->fade_in_rem = 0;
	c->fade_out_from = c->env_mul;
	if (c->fade_out_from > 100)
	{
		c->fade_out_from = 100;
	}
	c->fade_out_total = total;
	c->fade_out_rem = total;
	c->fade_out_active = 1;
}

static void divoom_audio_sink_ch_finish_fade_out(divoom_audio_sink_ch_state_t *c)
{
	if (!c)
	{
		return;
	}
	c->fade_out_active = 0;
	c->fade_out_rem = 0;
	c->env_mul = 0;
	c->enabled = 0;
	if (c->ring)
	{
		divoom_ring_buffer_clr(c->ring);
	}
}

static int divoom_audio_sink_open_device(divoom_audio_sink_ctrl_t *h)
{
	int ret;
	int card;
	sound_pcm_hw_params_t *hw = NULL;
	sound_pcm_sw_params_t *sw = NULL;
	sound_pcm_uframes_t period = DIVOOM_AUDIO_SINK_PERIOD_FRAMES;
	sound_pcm_uframes_t buffer = DIVOOM_AUDIO_SINK_BUFFER_FRAMES;

	card = ksound_card_index(DIVOOM_AUDIO_SINK_CARD_NAME);
	if (card < 0)
	{
		divoom_print("audio_sink: card %s not found (%d)\n", DIVOOM_AUDIO_SINK_CARD_NAME, card);
		return -1;
	}
	h->card = (unsigned int)card;

	ret = sunxi_pcm_open(&h->pcm, h->card, 0, SOUND_PCM_STREAM_PLAYBACK, 0);
	if (ret < 0 || !h->pcm)
	{
		divoom_print("audio_sink: pcm open fail %d\n", ret);
		h->pcm = NULL;
		return -1;
	}

	sound_pcm_hw_params_alloca(&hw);
	ret = sound_pcm_hw_params_any(h->pcm, hw);
	if (ret < 0)
	{
		goto fail;
	}
	ret = sound_pcm_hw_params_set_access(h->pcm, hw, SOUND_PCM_ACCESS_RW_INTERLEAVED);
	if (ret < 0)
	{
		goto fail;
	}
	ret = sound_pcm_hw_params_set_format(h->pcm, hw, SOUND_PCM_FORMAT_S16_LE);
	if (ret < 0)
	{
		goto fail;
	}
	ret = sound_pcm_hw_params_set_channels(h->pcm, hw, DIVOOM_AUDIO_SINK_CH);
	if (ret < 0)
	{
		goto fail;
	}
	ret = sound_pcm_hw_params_set_rate(h->pcm, hw, DIVOOM_AUDIO_SINK_RATE, 0);
	if (ret < 0)
	{
		goto fail;
	}
	ret = sound_pcm_hw_params_set_period_size(h->pcm, hw, period, 0);
	if (ret < 0)
	{
		goto fail;
	}
	ret = sound_pcm_hw_params_set_buffer_size(h->pcm, hw, buffer);
	if (ret < 0)
	{
		goto fail;
	}
	ret = sound_pcm_set_hw_params(h->pcm, hw);
	if (ret < 0)
	{
		goto fail;
	}

	sound_pcm_sw_params_alloca(&sw);
	sound_pcm_sw_params_current(h->pcm, sw);
	{
		sound_pcm_uframes_t boundary = 0;

		sound_pcm_sw_params_get_boundary(sw, &boundary);
		sound_pcm_sw_params_set_start_threshold(h->pcm, sw, buffer);
		sound_pcm_sw_params_set_silence_size(h->pcm, sw, boundary);
		/*
		 * stop_threshold=boundary 关闭 XRUN 停流：欠载时驱动补静音继续跑。
		 * 否则 avail>=stop_threshold 会 -EPIPE → recover→prepare 重启流，
		 * 而重启发生在 HAL 内部，绕过 JL 门控（此时 JL I2S 音量为 100）→ 破音。
		 */
		sound_pcm_sw_params_set_stop_threshold(h->pcm, sw, boundary);
		sound_pcm_sw_params_set_avail_min(h->pcm, sw, period);
	}
	ret = sound_pcm_set_sw_params(h->pcm, sw);
	if (ret < 0)
	{
		goto fail;
	}

	ret = sunxi_pcm_prepare(h->pcm);
	if (ret < 0)
	{
		goto fail;
	}

	divoom_print("audio_sink: open ok card=%u 48k/2ch\n", h->card);
	return 0;

fail:
	divoom_print("audio_sink: pcm config fail %d\n", ret);
	sunxi_pcm_close(h->pcm);
	h->pcm = NULL;
	return -1;
}

static void divoom_audio_sink_close_device(divoom_audio_sink_ctrl_t *h)
{
	if (!h || !h->pcm)
	{
		return;
	}
	sunxi_pcm_close(h->pcm);
	h->pcm = NULL;
}

static int divoom_audio_sink_write_frames(divoom_audio_sink_ctrl_t *h, const int16_t *interleaved,
					  unsigned int frames)
{
	unsigned int left = frames;
	const int16_t *p = interleaved;

	if (!h || !h->pcm || !interleaved || frames == 0)
	{
		return -1;
	}

	unsigned int err_guard = 0;

	while (left > 0 && h->run_flag)
	{
		sound_pcm_sframes_t wr;

		wr = sunxi_pcm_writei(h->pcm, p, left);
		if (wr < 0)
		{
			if (sunxi_pcm_recover(h->pcm, (int)wr, 1) < 0)
			{
				return -1;
			}
			/* 恢复后立刻重试会变成忙循环，把 UI 任务饿死 */
			if (++err_guard > DIVOOM_AUDIO_SINK_WRITE_ERR_LIMIT)
			{
				return -1;
			}
			divoom_system_delay_ms(1);
			continue;
		}
		err_guard = 0;
		if (wr == 0)
		{
			divoom_system_delay_ms(2);
			continue;
		}
		p += (unsigned int)wr * DIVOOM_AUDIO_SINK_CH;
		left -= (unsigned int)wr;
		h->out_frames_written += (unsigned int)wr;
	}
	return 0;
}

static void divoom_audio_sink_pull_channel(divoom_audio_sink_ch_state_t *c, int16_t *dst,
					   unsigned int interleaved_samples)
{
	unsigned int need_bytes;
	unsigned int avail;
	unsigned int got;

	if (!c || !dst || interleaved_samples == 0)
	{
		return;
	}
	need_bytes = interleaved_samples * sizeof(int16_t);
	if (!c->ring)
	{
		memset(dst, 0, need_bytes);
		return;
	}
	avail = divoom_ring_buffer_getdatasize(c->ring);
	/* sink锁内完成等待/恢复切换；只保留NETWORK的数据，其他通道与I2S时钟照常运行。
	 * 淡出优先于预缓冲，保证STOP在网络断流或尚未起播时也能及时完成。 */
	if (c->buffering && c->enabled && !c->fade_out_active)
	{
		if (avail < c->prebuffer_bytes)
		{
			memset(dst, 0, need_bytes);
			return;
		}
		c->buffering = 0;
		divoom_audio_sink_ch_reset_fade_in(c, 0);
	}
	if (avail < need_bytes && c->enabled && !c->fade_out_active && c->prebuffer_bytes)
	{
		/* 一次实际欠载记一次；等待期间不反复累加，诊断另报buffering状态。 */
		c->underruns++;
		c->buffering = 1;
		memset(dst, 0, need_bytes);
		return;
	}
	if (avail == 0)
	{
		if (c->enabled)
		{
			c->underruns++;
		}
		memset(dst, 0, need_bytes);
		return;
	}
	got = avail;
	if (got > need_bytes)
	{
		got = need_bytes;
	}
	divoom_ring_buffer_get(c->ring, dst, got);
	if (got < need_bytes)
	{
		memset((uint8_t *)dst + got, 0, need_bytes - got);
		if (c->enabled)
		{
			c->underruns++;
		}
	}
}

/*
 * vol/fade/duck 均为 0~100。须分步或 int64 混算：一次性 *duck/1e6 会在
 * 提示音满幅(vol=100)时 int32 溢出（改造前 /10000 无此问题）。
 */
static int divoom_audio_sink_mix_sample(const divoom_audio_sink_ch_state_t *c,
					int16_t sample, int fade_mul)
{
	int duck;
	int64_t v;

	if (!c)
	{
		return 0;
	}
	/* duck=0 就是 mute；勿再把 0 当成未初始化满幅 */
	duck = (int)c->duck_mul;
	if (duck <= 0)
	{
		return 0;
	}
	v = (int64_t)sample * (int64_t)c->vol_target * (int64_t)fade_mul;
	v /= 10000;
	if (duck != 100)
	{
		v = v * (int64_t)duck / 100;
	}
	return (int)v;
}

static void divoom_audio_sink_advance_env(divoom_audio_sink_ch_state_t *c, int *fade_mul_out)
{
	int fade_mul;

	if (!c || !fade_mul_out)
	{
		return;
	}

	/* 无进行中的 fade 时保持当前 env（支持 fade_in(0,0) 数字静音保持） */
	fade_mul = (int)c->env_mul;
	if (c->fade_in_rem > 0 && c->fade_in_total > 0)
	{
		unsigned int pos = c->fade_in_total - c->fade_in_rem;

		fade_mul = (int)(c->fade_in_from
				 + ((100u - (unsigned int)c->fade_in_from) * pos) / c->fade_in_total);
		c->fade_in_rem--;
		if (c->fade_in_rem == 0)
		{
			fade_mul = 100;
		}
	}
	if (c->fade_out_active)
	{
		if (c->fade_out_rem > 0 && c->fade_out_total > 0)
		{
			int fo = (int)((c->fade_out_from * c->fade_out_rem) / c->fade_out_total);

			fade_mul = fade_mul * fo / 100;
			c->fade_out_rem--;
		}
		else
		{
			fade_mul = 0;
		}
	}
	if (fade_mul < 0)
	{
		fade_mul = 0;
	}
	else if (fade_mul > 100)
	{
		fade_mul = 100;
	}
	c->env_mul = (uint8_t)fade_mul;
	*fade_mul_out = fade_mul;
}

/*
 * 混音热路径持锁：控制面（音量/duck/fade/enable）与 sink 任务共享 ch 状态。
 * 只包住 pull+mix（纯 CPU），不包 I2S write——避免把 DMA 阻塞拖进临界区。
 * 每 period 加锁一次；512 帧混算通常远短于 1ms。
 */
static void divoom_audio_sink_mix_period(divoom_audio_sink_ctrl_t *h)
{
	unsigned int i;
	unsigned int n = DIVOOM_AUDIO_SINK_PERIOD_FRAMES * DIVOOM_AUDIO_SINK_CH;
	unsigned int ch;
	uint8_t need_mix[DIVOOM_AUDIO_SINK_CH_COUNT];

	memset(h->mix_buf, 0, sizeof(h->mix_buf));
	memset(need_mix, 0, sizeof(need_mix));

	divoom_system_get_lock(h->lock);

	for (ch = 0; ch < DIVOOM_AUDIO_SINK_CH_COUNT; ch++)
	{
		divoom_audio_sink_ch_state_t *c = &h->ch[ch];

		if (!divoom_audio_sink_ch_needs_mix(c))
		{
			continue;
		}
		need_mix[ch] = 1;
		divoom_audio_sink_pull_channel(c, h->ch_buf[ch], n);
	}

	for (i = 0; i < n; i++)
	{
		int sum = 0;
		unsigned int ch_idx;

		for (ch_idx = 0; ch_idx < DIVOOM_AUDIO_SINK_CH_COUNT; ch_idx++)
		{
			divoom_audio_sink_ch_state_t *c = &h->ch[ch_idx];
			int fade_mul;
			int v;

			if (!need_mix[ch_idx])
			{
				continue;
			}
			divoom_audio_sink_advance_env(c, &fade_mul);
			v = divoom_audio_sink_mix_sample(c, h->ch_buf[ch_idx][i], fade_mul);
			sum += v;
		}
		if (sum > 32767)
		{
			sum = 32767;
		}
		else if (sum < -32768)
		{
			sum = -32768;
		}
		h->mix_buf[i] = (int16_t)sum;
	}

	for (ch = 0; ch < DIVOOM_AUDIO_SINK_CH_COUNT; ch++)
	{
		divoom_audio_sink_ch_state_t *c = &h->ch[ch];

		if (c->fade_out_active && c->fade_out_rem == 0)
		{
			divoom_audio_sink_ch_finish_fade_out(c);
		}
	}

	divoom_system_free_lock(h->lock);
}

static void *divoom_audio_sink_task_main(void *data)
{
	divoom_audio_sink_ctrl_t *h = s_sink;

	(void)data;

	while (h && h->run_flag)
	{
		int active;

		divoom_system_get_lock(h->lock);
		active = divoom_audio_sink_any_active_locked(h);
		divoom_system_free_lock(h->lock);

		if (!active)
		{
			if (h->stop_req)
			{
				break;
			}
			/*
			 * 空闲保活：继续按 period 节奏写静音，不让 I2S 停下来。
			 * 开关流会在 JL 接收端留下可闻的咔哒，JL 音量拉 0 也挡不住，
			 * 所以流必须一直跑。这里不能设空闲上限：一旦停止喂数据，
			 * DMA 就进入欠载状态，下一个音源复用这条流时 PCM 已经不健康。
			 * 写操作阻塞在 DMA 上，占用的 CPU 很小。
			 */
			memset(h->mix_buf, 0, sizeof(h->mix_buf));
			if (divoom_audio_sink_write_frames(h, h->mix_buf,
							   DIVOOM_AUDIO_SINK_PERIOD_FRAMES) != 0)
			{
				divoom_system_delay_ms(5);
			}
			h->silence_periods++;
			continue;
		}

		divoom_audio_sink_mix_period(h);
		if (!h->run_flag)
		{
			break;
		}
		if (divoom_audio_sink_write_frames(h, h->mix_buf, DIVOOM_AUDIO_SINK_PERIOD_FRAMES) != 0)
		{
			divoom_system_delay_ms(5);
		}

		divoom_system_get_lock(h->lock);
		if (!divoom_audio_sink_any_active_locked(h) && h->stop_req)
		{
			divoom_system_free_lock(h->lock);
			break;
		}
		divoom_system_free_lock(h->lock);
	}

	if (h)
	{
		divoom_system_put_sem(h->sem);
	}
	return NULL;
}

static pthread_mutex_t s_sink_lifecycle_lock = PTHREAD_MUTEX_INITIALIZER;

static int divoom_audio_sink_start_locked(void)
{
	divoom_audio_sink_ctrl_t *h;
	unsigned int ch;

	if (s_sink && s_sink->run_flag)
	{
		return 0;
	}

	if (divoom_audio_buf_pool_init() != 0)
	{
		return -1;
	}

	h = (divoom_audio_sink_ctrl_t *)divoom_system_malloc(sizeof(*h));
	if (!h)
	{
		return -1;
	}
	memset(h, 0, sizeof(*h));
	h->lock = divoom_system_create_lock();
	if (!h->lock)
	{
		divoom_system_free(h);
		return -1;
	}
	for (ch = 0; ch < DIVOOM_AUDIO_SINK_CH_COUNT; ch++)
	{
		h->ch_buf[ch] = (int16_t *)divoom_system_malloc(
			DIVOOM_AUDIO_SINK_PERIOD_FRAMES * DIVOOM_AUDIO_SINK_CH * sizeof(int16_t));
		if (!h->ch_buf[ch])
		{
			unsigned int j;

			for (j = 0; j < ch; j++)
			{
				divoom_system_free(h->ch_buf[j]);
			}
			divoom_system_destroy_lock(h->lock);
			divoom_system_free(h);
			return -1;
		}
	}
	if (divoom_audio_sink_open_device(h) != 0)
	{
		unsigned int j;

		for (j = 0; j < DIVOOM_AUDIO_SINK_CH_COUNT; j++)
		{
			if (h->ch_buf[j])
			{
				divoom_system_free(h->ch_buf[j]);
			}
		}
		divoom_system_destroy_lock(h->lock);
		divoom_system_free(h);
		return -1;
	}

	h->run_flag = 1;
	h->stop_req = 0;
	h->sem = divoom_system_create_sem(1, 0);
    if (!h->sem) goto start_failed;
	s_sink = h;
	if (!divoom_system_create_task_prio(divoom_audio_sink_task_main, "audio_sink", NULL,
					    DIVOOM_TASK_STACK_SND_PCM,
					    DIVOOM_AUDIO_SINK_TASK_PRIO))
	{
		divoom_print("audio_sink: create task fail\n");
		h->run_flag = 0;
		divoom_system_destroy_sem(h->sem);
		h->sem = NULL;
		s_sink = NULL;
        goto start_failed;
	}
	divoom_print("audio_sink: start ok\n");
	return 0;
start_failed:
    divoom_audio_sink_close_device(h);
    for (ch = 0; ch < DIVOOM_AUDIO_SINK_CH_COUNT; ch++)
        divoom_system_free(h->ch_buf[ch]);
    divoom_system_destroy_lock(h->lock);
    divoom_system_free(h);
    return -1;
}

int divoom_audio_sink_start(void)
{
    int ret;
    pthread_mutex_lock(&s_sink_lifecycle_lock);
    ret = divoom_audio_sink_start_locked();
    pthread_mutex_unlock(&s_sink_lifecycle_lock);
    return ret;
}

static void divoom_audio_sink_stop_locked(void)
{
	divoom_audio_sink_ctrl_t *h;
	unsigned int ch;

	h = s_sink;
	if (!h)
	{
		return;
	}
	if (!h->run_flag)
	{
		/* 任务已退出但未清 s_sink 的异常路径：仍做收尾 */
		s_sink = NULL;
		if (h->sem)
		{
			divoom_system_destroy_sem(h->sem);
			h->sem = NULL;
		}
		divoom_audio_sink_close_device(h);
		for (ch = 0; ch < DIVOOM_AUDIO_SINK_CH_COUNT; ch++)
		{
			divoom_audio_sink_ch_free_ring(&h->ch[ch]);
			if (h->ch_buf[ch])
			{
				divoom_system_free(h->ch_buf[ch]);
				h->ch_buf[ch] = NULL;
			}
		}
		for (ch = 0; ch < DIVOOM_AUDIO_BUS_COUNT; ch++)
            divoom_audio_buf_pool_force_release((divoom_audio_bus_t)ch, "sink_stop");
		if (h->lock)
		{
			divoom_system_destroy_lock(h->lock);
		}
		divoom_system_free(h);
		return;
	}

	/* 先停任务再拆 ring，避免 mix_period 与 free_ring 并发（切歌 keep 后释放尤易踩中） */
	h->stop_req = 1;
	divoom_system_get_lock(h->lock);
	for (ch = 0; ch < DIVOOM_AUDIO_SINK_CH_COUNT; ch++)
	{
		divoom_audio_sink_ch_finish_fade_out(&h->ch[ch]);
	}
	divoom_system_free_lock(h->lock);
	h->run_flag = 0;

	/* 先 stop PCM，解开可能阻塞的 writei，再等任务退出 */
	if (h->pcm)
	{
		(void)sunxi_pcm_stop(h->pcm);
	}

	if (!divoom_system_get_sem_with_time(h->sem, 2))
	{
		divoom_print("audio_sink: stop wait timeout\n");
		divoom_system_get_sem(h->sem);
	}

	s_sink = NULL;
	divoom_system_destroy_sem(h->sem);
	h->sem = NULL;
	divoom_audio_sink_close_device(h);
	for (ch = 0; ch < DIVOOM_AUDIO_SINK_CH_COUNT; ch++)
	{
		divoom_audio_sink_ch_free_ring(&h->ch[ch]);
		if (h->ch_buf[ch])
		{
			divoom_system_free(h->ch_buf[ch]);
			h->ch_buf[ch] = NULL;
		}
	}
	for (ch = 0; ch < DIVOOM_AUDIO_BUS_COUNT; ch++)
        divoom_audio_buf_pool_force_release((divoom_audio_bus_t)ch, "sink_stop");
	divoom_system_destroy_lock(h->lock);
	divoom_system_free(h);
	divoom_print("audio_sink: stop ok\n");
}

void divoom_audio_sink_stop(void)
{
    pthread_mutex_lock(&s_sink_lifecycle_lock);
    divoom_audio_sink_stop_locked();
    pthread_mutex_unlock(&s_sink_lifecycle_lock);
}

int divoom_audio_sink_is_running(void)
{
	return (s_sink && s_sink->run_flag) ? 1 : 0;
}

void divoom_audio_sink_ch_enable(divoom_audio_sink_ch_t ch, int on)
{
	divoom_audio_sink_ch_state_t *c;

	if (!s_sink || ch >= DIVOOM_AUDIO_SINK_CH_COUNT)
	{
		return;
	}
	c = &s_sink->ch[ch];
	divoom_system_get_lock(s_sink->lock);
	if (on)
	{
		if (divoom_audio_sink_ch_ensure_ring(c, ch) != 0)
		{
			divoom_print("audio_sink: enable fail ch=%u (ring alloc)\n",
				     (unsigned int)ch);
			divoom_system_free_lock(s_sink->lock);
			return;
		}
		c->enabled = 1;
		divoom_audio_sink_ch_reset_fade_in(c, 0);
	}
	else
	{
		if (c->enabled || c->fade_out_active)
		{
			divoom_audio_sink_ch_start_fade_out(c, DIVOOM_AUDIO_SINK_CUT_FADE_MS);
		}
	}
	divoom_system_free_lock(s_sink->lock);
}

/*
 * 用户档位 0–100 → 数字幅度 0–100（平方律，约 −40 dB～0 dB）。
 * 仅 0 为静音；非 0 至少幅度 1，避免整数截断把 1–7 档打成无声。
 */
static unsigned int divoom_audio_sink_ui_vol_to_amp(unsigned int ui)
{
	unsigned int amp;

	if (ui == 0)
	{
		return 0;
	}
	if (ui >= 100)
	{
		return 100;
	}
	amp = (ui * ui + 50u) / 100u;
	if (amp < 1u)
	{
		return 1;
	}
	return amp;
}

void divoom_audio_sink_ch_set_volume(divoom_audio_sink_ch_t ch, unsigned int vol_0_100)
{
	if (!s_sink || ch >= DIVOOM_AUDIO_SINK_CH_COUNT)
	{
		return;
	}
	if (vol_0_100 > 100)
	{
		vol_0_100 = 100;
	}
	divoom_system_get_lock(s_sink->lock);
	s_sink->ch[ch].vol_target = (uint8_t)divoom_audio_sink_ui_vol_to_amp(vol_0_100);
	divoom_system_free_lock(s_sink->lock);
}

void divoom_audio_sink_ch_set_duck(divoom_audio_sink_ch_t ch, unsigned int duck_0_100)
{
	if (!s_sink || ch >= DIVOOM_AUDIO_SINK_CH_COUNT)
	{
		return;
	}
	if (duck_0_100 > 100)
	{
		duck_0_100 = 100;
	}
	divoom_system_get_lock(s_sink->lock);
	s_sink->ch[ch].duck_mul = (uint8_t)duck_0_100;
	divoom_system_free_lock(s_sink->lock);
}

unsigned int divoom_audio_sink_ch_get_duck(divoom_audio_sink_ch_t ch)
{
	unsigned int duck;

	if (!s_sink || ch >= DIVOOM_AUDIO_SINK_CH_COUNT)
	{
		return 100;
	}
	divoom_system_get_lock(s_sink->lock);
	duck = (unsigned int)s_sink->ch[ch].duck_mul;
	divoom_system_free_lock(s_sink->lock);
	return duck;
}

void divoom_audio_sink_ch_fade_in(divoom_audio_sink_ch_t ch, unsigned int ms, unsigned int from_vol)
{
	divoom_audio_sink_ch_state_t *c;
	unsigned int total;

	if (!s_sink || ch >= DIVOOM_AUDIO_SINK_CH_COUNT)
	{
		return;
	}
	c = &s_sink->ch[ch];
	divoom_system_get_lock(s_sink->lock);
	if (divoom_audio_sink_ch_ensure_ring(c, ch) != 0)
	{
		divoom_system_free_lock(s_sink->lock);
		return;
	}
	if (from_vol > 100)
	{
		from_vol = 100;
	}
	total = divoom_audio_sink_fade_interleaved(ms);
	if (total == 0)
	{
		/* ms==0：按 from_vol 瞬时到位（fade_in(0,0)=数字静音），勿误用 vol_target */
		c->env_mul = (uint8_t)from_vol;
		c->fade_in_from = (uint8_t)from_vol;
		c->fade_in_total = 0;
		c->fade_in_rem = 0;
		c->fade_out_active = 0;
		c->fade_out_rem = 0;
	}
	else
	{
		c->fade_in_from = (uint8_t)from_vol;
		c->fade_in_total = total;
		c->fade_in_rem = total;
		c->fade_out_active = 0;
		c->fade_out_rem = 0;
		c->env_mul = (uint8_t)from_vol;
	}
	c->enabled = 1;
	divoom_system_free_lock(s_sink->lock);
}

/* 数字静音并丢弃待播数据；冲刷静音前用它，勿再让 fade_in 隐式清 ring */
void divoom_audio_sink_ch_mute_drop(divoom_audio_sink_ch_t ch)
{
	divoom_audio_sink_ch_state_t *c;

	if (!s_sink || ch >= DIVOOM_AUDIO_SINK_CH_COUNT)
	{
		return;
	}
	c = &s_sink->ch[ch];
	divoom_system_get_lock(s_sink->lock);
	if (divoom_audio_sink_ch_ensure_ring(c, ch) != 0)
	{
		divoom_system_free_lock(s_sink->lock);
		return;
	}
	c->env_mul = 0;
	c->fade_in_from = 0;
	c->fade_in_total = 0;
	c->fade_in_rem = 0;
	c->fade_out_active = 0;
	c->fade_out_rem = 0;
	c->fade_out_total = 0;
	c->enabled = 1;
	if (c->ring)
	{
		divoom_ring_buffer_clr(c->ring);
	}
	divoom_system_free_lock(s_sink->lock);
}

void divoom_audio_sink_ch_fade_out(divoom_audio_sink_ch_t ch, unsigned int ms)
{
	if (!s_sink || ch >= DIVOOM_AUDIO_SINK_CH_COUNT)
	{
		return;
	}
	divoom_system_get_lock(s_sink->lock);
	divoom_audio_sink_ch_start_fade_out(&s_sink->ch[ch], ms);
	divoom_system_free_lock(s_sink->lock);
}

int divoom_audio_sink_ch_fade_out_wait(divoom_audio_sink_ch_t ch, unsigned int timeout_ms)
{
	unsigned int t0;

	if (!s_sink || ch >= DIVOOM_AUDIO_SINK_CH_COUNT)
	{
		return -1;
	}
	t0 = divoom_system_get_tickms();
	while (divoom_system_get_tickms() - t0 < timeout_ms)
	{
		int active;

		divoom_system_get_lock(s_sink->lock);
		active = s_sink->ch[ch].fade_out_active;
		divoom_system_free_lock(s_sink->lock);
		if (!active)
		{
			return 0;
		}
		divoom_system_delay_ms(5);
	}
	return -1;
}

int divoom_audio_sink_push(divoom_audio_sink_ch_t ch, const int16_t *stereo, unsigned int frames)
{
	divoom_audio_sink_ch_state_t *c;
	uint32_t bytes;
	void *ring;
	int enabled_or_fade;
	uint32_t off = 0;

	if (!s_sink || !s_sink->run_flag || ch >= DIVOOM_AUDIO_SINK_CH_COUNT || !stereo || frames == 0)
	{
		return -1;
	}
	c = &s_sink->ch[ch];
	bytes = frames * DIVOOM_AUDIO_SINK_CH * (uint32_t)sizeof(int16_t);

	/* 与 mix/force_release 共享 ch 状态与 ring 指针；put 期间持锁避免 UAF */
	divoom_system_get_lock(s_sink->lock);
	enabled_or_fade = (c->enabled || c->fade_out_active) ? 1 : 0;
	ring = (c->ring_inited && c->ring) ? c->ring : NULL;
	if (!ring || !enabled_or_fade)
	{
		divoom_system_free_lock(s_sink->lock);
		return -1;
	}
	while (off < bytes)
	{
		uint32_t put;

		put = divoom_ring_buffer_put(ring, (uint8_t *)stereo + off, bytes - off);
		if (put == 0)
		{
			break;
		}
		off += put;
	}
	divoom_system_free_lock(s_sink->lock);

	if (off == 0)
	{
		return -1;
	}
	return (int)(off / (DIVOOM_AUDIO_SINK_CH * (uint32_t)sizeof(int16_t)));
}

int divoom_audio_sink_push_all(divoom_audio_sink_ch_t ch, const int16_t *stereo, unsigned int frames)
{
	unsigned int done = 0;
	int guard = 0;

	if (!stereo || frames == 0)
	{
		return -1;
	}
	while (done < frames)
	{
		unsigned int chunk = frames - done;
		int wr;

		if (chunk > DIVOOM_AUDIO_SINK_PERIOD_FRAMES)
		{
			chunk = DIVOOM_AUDIO_SINK_PERIOD_FRAMES;
		}
		wr = divoom_audio_sink_push(ch, stereo + done * DIVOOM_AUDIO_SINK_CH, chunk);
		if (wr <= 0)
		{
			if (++guard > 200)
			{
				return -1;
			}
			divoom_system_yield();
			divoom_system_delay_ms(1);
			continue;
		}
		done += (unsigned int)wr;
		guard = 0;
		if ((unsigned int)wr < chunk)
		{
			divoom_system_delay_ms(1);
		}
	}
	return 0;
}

int divoom_audio_sink_push_silence(divoom_audio_sink_ch_t ch, unsigned int frames)
{
	/* BSS 全零；分片复用，避免调用方在 6KB 栈任务里再开 2KB silence */
	static int16_t s_zero[DIVOOM_AUDIO_SINK_PERIOD_FRAMES * DIVOOM_AUDIO_SINK_CH];
	unsigned int done = 0;

	if (!s_sink || !s_sink->run_flag || ch >= DIVOOM_AUDIO_SINK_CH_COUNT || frames == 0)
	{
		return -1;
	}
	while (done < frames)
	{
		unsigned int chunk = frames - done;

		if (chunk > DIVOOM_AUDIO_SINK_PERIOD_FRAMES)
		{
			chunk = DIVOOM_AUDIO_SINK_PERIOD_FRAMES;
		}
		if (divoom_audio_sink_push_all(ch, s_zero, chunk) != 0)
		{
			return -1;
		}
		done += chunk;
	}
	return 0;
}

void divoom_audio_sink_ch_disable(divoom_audio_sink_ch_t ch)
{
	if (!s_sink || ch >= DIVOOM_AUDIO_SINK_CH_COUNT)
	{
		return;
	}
	divoom_system_get_lock(s_sink->lock);
	divoom_audio_sink_ch_finish_fade_out(&s_sink->ch[ch]);
	divoom_system_free_lock(s_sink->lock);
	divoom_audio_sink_try_stop();
}

/*
 * 通道空闲不再关闭 PCM：设备保持打开、任务持续写静音。
 * 每次提示音都 open/close 一次 I2S 是头尾咔哒的直接来源，
 * 真要释放设备（切音源、关机）显式调 divoom_audio_sink_stop()。
 */
void divoom_audio_sink_try_stop(void)
{
}

void divoom_audio_sink_force_release_bus(divoom_audio_sink_ch_t ch)
{
	divoom_audio_bus_t bus;

	if (ch >= DIVOOM_AUDIO_SINK_CH_COUNT)
	{
		return;
	}
	bus = divoom_audio_sink_ch_to_bus(ch);
	pthread_mutex_lock(&s_sink_lifecycle_lock);
	if (s_sink)
	{
		divoom_audio_sink_ch_state_t *c = &s_sink->ch[ch];
		divoom_system_get_lock(s_sink->lock);
		if (divoom_audio_buf_pool_is_leased(bus))
		{
			divoom_audio_sink_ch_finish_fade_out(c);
			divoom_audio_sink_ch_free_ring(c);
			/* 与绑定使用同一把通道锁，防止拆绑定后重新挂上待释放的ring。 */
			divoom_audio_buf_pool_force_release(bus, "force");
			s_bus_force_release_cnt++;
		}
		divoom_system_free_lock(s_sink->lock);
	}
	else
	{
		divoom_audio_buf_pool_force_release(bus, "force");
	}
	pthread_mutex_unlock(&s_sink_lifecycle_lock);
}

unsigned int divoom_audio_sink_bus_force_release_count(void)
{
	return s_bus_force_release_cnt;
}

int divoom_audio_sink_ch_active(divoom_audio_sink_ch_t ch)
{
	divoom_audio_sink_ch_state_t *c;
	int active;

	if (!s_sink || ch >= DIVOOM_AUDIO_SINK_CH_COUNT)
	{
		return 0;
	}
	c = &s_sink->ch[ch];
	divoom_system_get_lock(s_sink->lock);
	active = divoom_audio_sink_ch_needs_mix(c);
	divoom_system_free_lock(s_sink->lock);
	return active;
}

unsigned int divoom_audio_sink_whitebox_out_frames(void)
{
	if (!s_sink)
	{
		return 0;
	}
	return s_sink->out_frames_written;
}

unsigned int divoom_audio_sink_whitebox_underruns(divoom_audio_sink_ch_t ch)
{
	unsigned int n;

	if (!s_sink || ch >= DIVOOM_AUDIO_SINK_CH_COUNT)
	{
		return 0;
	}
	divoom_system_get_lock(s_sink->lock);
	n = s_sink->ch[ch].underruns;
	divoom_system_free_lock(s_sink->lock);
	return n;
}

unsigned int divoom_audio_sink_whitebox_silence_periods(void)
{
	if (!s_sink)
	{
		return 0;
	}
	return s_sink->silence_periods;
}

unsigned int divoom_audio_sink_ch_env_mul(divoom_audio_sink_ch_t ch)
{
	unsigned int env;

	if (!s_sink || ch >= DIVOOM_AUDIO_SINK_CH_COUNT)
	{
		return 0;
	}
	divoom_system_get_lock(s_sink->lock);
	env = s_sink->ch[ch].env_mul;
	divoom_system_free_lock(s_sink->lock);
	return env;
}

int divoom_audio_sink_ch_ring_bound(divoom_audio_sink_ch_t ch)
{
	int bound;

	if (!s_sink || ch >= DIVOOM_AUDIO_SINK_CH_COUNT)
	{
		return 0;
	}
	divoom_system_get_lock(s_sink->lock);
	bound = s_sink->ch[ch].ring_inited ? 1 : 0;
	divoom_system_free_lock(s_sink->lock);
	return bound;
}

/* 容量与水位分开读取，容量在一次租约内不变；不暴露ring内部指针。 */
unsigned int divoom_audio_sink_ch_ring_capacity(divoom_audio_sink_ch_t ch)
{
    unsigned int capacity;
    if (!s_sink || ch >= DIVOOM_AUDIO_SINK_CH_COUNT)
    {
        return 0;
    }
    divoom_system_get_lock(s_sink->lock);
    capacity = s_sink->ch[ch].ring_capacity;
    divoom_system_free_lock(s_sink->lock);
    return capacity;
}

int divoom_audio_sink_ch_buffering(divoom_audio_sink_ch_t ch)
{
    int buffering;
    if (!s_sink || ch >= DIVOOM_AUDIO_SINK_CH_COUNT)
    {
        return 0;
    }
    divoom_system_get_lock(s_sink->lock);
    buffering = s_sink->ch[ch].buffering;
    divoom_system_free_lock(s_sink->lock);
    return buffering;
}

void divoom_audio_sink_ch_drain(divoom_audio_sink_ch_t ch)
{
    if (!s_sink || ch >= DIVOOM_AUDIO_SINK_CH_COUNT)
    {
        return;
    }
    divoom_system_get_lock(s_sink->lock);
    s_sink->ch[ch].prebuffer_bytes = 0;
    if (s_sink->ch[ch].buffering)
    {
        s_sink->ch[ch].buffering = 0;
        divoom_audio_sink_ch_reset_fade_in(&s_sink->ch[ch], 0);
    }
    divoom_system_free_lock(s_sink->lock);
}

unsigned int divoom_audio_sink_ch_ring_bytes(divoom_audio_sink_ch_t ch)
{
	unsigned int bytes;

	if (!s_sink || ch >= DIVOOM_AUDIO_SINK_CH_COUNT)
	{
		return 0;
	}
	divoom_system_get_lock(s_sink->lock);
	if (!s_sink->ch[ch].ring)
	{
		divoom_system_free_lock(s_sink->lock);
		return 0;
	}
	bytes = divoom_ring_buffer_getdatasize(s_sink->ch[ch].ring);
	divoom_system_free_lock(s_sink->lock);
	return bytes;
}

unsigned int divoom_audio_sink_ch_ring_blank_bytes(divoom_audio_sink_ch_t ch)
{
	unsigned int blank;

	if (!s_sink || ch >= DIVOOM_AUDIO_SINK_CH_COUNT)
	{
		return 0;
	}
	divoom_system_get_lock(s_sink->lock);
	if (!s_sink->ch[ch].ring)
	{
		divoom_system_free_lock(s_sink->lock);
		return 0;
	}
	blank = divoom_ring_buffer_getblanksize(s_sink->ch[ch].ring);
	divoom_system_free_lock(s_sink->lock);
	return blank;
}

unsigned int divoom_audio_sink_ch_ring_fill_pct(divoom_audio_sink_ch_t ch)
{
	unsigned int pct;

	if (!s_sink || ch >= DIVOOM_AUDIO_SINK_CH_COUNT)
	{
		return 0;
	}
	divoom_system_get_lock(s_sink->lock);
	pct = divoom_audio_sink_ch_ring_fill_pct_locked(&s_sink->ch[ch]);
	divoom_system_free_lock(s_sink->lock);
	return pct;
}

void divoom_audio_sink_diag_enable(int on)
{
	memset(&s_sink_diag, 0, sizeof(s_sink_diag));
	s_sink_diag.enabled = on ? 1u : 0u;
	divoom_print("audio_sink_diag: %s\n", on ? "on" : "off");
}

void divoom_audio_sink_diag_clear(void)
{
	uint8_t on = s_sink_diag.enabled;
	int i;

	memset(&s_sink_diag, 0, sizeof(s_sink_diag));
	s_sink_diag.enabled = on;
	/* autotest cleanup：清通道 underrun，避免跨 case 累加误杀 */
	if (s_sink)
	{
		divoom_system_get_lock(s_sink->lock);
		for (i = 0; i < DIVOOM_AUDIO_SINK_CH_COUNT; i++)
		{
			s_sink->ch[i].underruns = 0;
		}
		s_sink->silence_periods = 0;
		divoom_system_free_lock(s_sink->lock);
	}
}

void divoom_audio_sink_diag_print(void)
{
	printf("audio_sink_diag: en=%u period=%u clip=%u bound_jump=%u starve=%u "
	       "starve_pcm=%u fade_jump=%u dual=%u large_in=%u\n",
	       s_sink_diag.enabled, s_sink_diag.period_cnt, s_sink_diag.clip_cnt,
	       s_sink_diag.bound_jump_cnt, s_sink_diag.starve_cnt,
	       s_sink_diag.starve_to_pcm_cnt, s_sink_diag.fade_jump_cnt,
	       s_sink_diag.dual_mix_cnt, s_sink_diag.large_in_cnt);
}

#elif DIVOOM_BUILD_SIMULATOR

#include "divoom_audio_sink.h"

int divoom_audio_sink_start(void) { return 0; }
void divoom_audio_sink_stop(void) {}
int divoom_audio_sink_is_running(void) { return 0; }
void divoom_audio_sink_ch_enable(divoom_audio_sink_ch_t ch, int on) { (void)ch; (void)on; }
void divoom_audio_sink_ch_set_volume(divoom_audio_sink_ch_t ch, unsigned int vol_0_100)
{
	(void)ch; (void)vol_0_100;
}
void divoom_audio_sink_ch_set_duck(divoom_audio_sink_ch_t ch, unsigned int duck_0_100)
{
	(void)ch; (void)duck_0_100;
}
unsigned int divoom_audio_sink_ch_get_duck(divoom_audio_sink_ch_t ch) { (void)ch; return 100u; }
void divoom_audio_sink_ch_fade_in(divoom_audio_sink_ch_t ch, unsigned int ms, unsigned int from_vol)
{
	(void)ch; (void)ms; (void)from_vol;
}
void divoom_audio_sink_ch_mute_drop(divoom_audio_sink_ch_t ch) { (void)ch; }
void divoom_audio_sink_ch_fade_out(divoom_audio_sink_ch_t ch, unsigned int ms) { (void)ch; (void)ms; }
int divoom_audio_sink_ch_fade_out_wait(divoom_audio_sink_ch_t ch, unsigned int timeout_ms)
{
	(void)ch; (void)timeout_ms;
	return 0;
}
int divoom_audio_sink_push(divoom_audio_sink_ch_t ch, const int16_t *stereo, unsigned int frames)
{
	(void)ch; (void)stereo;
	return (int)frames;
}
int divoom_audio_sink_push_all(divoom_audio_sink_ch_t ch, const int16_t *stereo, unsigned int frames)
{
	(void)ch; (void)stereo;
	return (int)frames;
}
int divoom_audio_sink_push_silence(divoom_audio_sink_ch_t ch, unsigned int frames)
{
	(void)ch;
	return (int)frames;
}
int divoom_audio_sink_ch_active(divoom_audio_sink_ch_t ch) { (void)ch; return 0; }
void divoom_audio_sink_ch_disable(divoom_audio_sink_ch_t ch) { (void)ch; }
void divoom_audio_sink_try_stop(void) {}
void divoom_audio_sink_force_release_bus(divoom_audio_sink_ch_t ch) { (void)ch; }
unsigned int divoom_audio_sink_bus_force_release_count(void) { return 0u; }
unsigned int divoom_audio_sink_whitebox_out_frames(void) { return 0u; }
unsigned int divoom_audio_sink_whitebox_underruns(divoom_audio_sink_ch_t ch) { (void)ch; return 0u; }
unsigned int divoom_audio_sink_whitebox_silence_periods(void) { return 0u; }
unsigned int divoom_audio_sink_ch_env_mul(divoom_audio_sink_ch_t ch) { (void)ch; return 0u; }
int divoom_audio_sink_ch_ring_bound(divoom_audio_sink_ch_t ch) { (void)ch; return 0; }
unsigned int divoom_audio_sink_ch_ring_capacity(divoom_audio_sink_ch_t ch) { (void)ch; return 0u; }
int divoom_audio_sink_ch_buffering(divoom_audio_sink_ch_t ch) { (void)ch; return 0; }
void divoom_audio_sink_ch_drain(divoom_audio_sink_ch_t ch) { (void)ch; }
unsigned int divoom_audio_sink_ch_ring_bytes(divoom_audio_sink_ch_t ch) { (void)ch; return 0u; }
unsigned int divoom_audio_sink_ch_ring_blank_bytes(divoom_audio_sink_ch_t ch) { (void)ch; return 0u; }
unsigned int divoom_audio_sink_ch_ring_fill_pct(divoom_audio_sink_ch_t ch) { (void)ch; return 0u; }
void divoom_audio_sink_diag_enable(int on) { (void)on; }
void divoom_audio_sink_diag_clear(void) {}
void divoom_audio_sink_diag_print(void) {}

#endif

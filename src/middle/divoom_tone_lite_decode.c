#include "divoom_build_config.h"

#if DIVOOM_BUILD_RTOS_DEVICE

#include "divoom.h"
#include "divoom_tone_lite_decode.h"
#include "divoom_audio_sink.h"
#include "divoom_audio_policy.h"
#include "divoom_audio_buf_pool.h"
#include "divoom_tone_service.h"
#include "divoom_port_fs.h"

#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

#ifdef CONFIG_COMPONENTS_SPEEXDSP
#include "speex_resampler.h"
#endif

#include "divoom_mp3dec.h"
#include "divoom_aacdec.h"

#define DIVOOM_TONE_LITE_OUT_RATE        48000u
#define DIVOOM_TONE_LITE_OUT_CH          2u
#define DIVOOM_TONE_LITE_PERIOD_FRAMES   512u
#define DIVOOM_TONE_LITE_MP3_READ_SAMPLES (DIVOOM_TONE_LITE_PERIOD_FRAMES * 8u)
#define DIVOOM_TONE_LITE_AAC_IO_BYTES    4096u
#define DIVOOM_TONE_LITE_FADE_MS         80u
/* 切歌 fade 过短时，JL mute 偶发未落地会听到硬切啪；略加长与 EOF 接近 */
#define DIVOOM_TONE_LITE_CUT_FADE_MS     50u
#define DIVOOM_TONE_LITE_I2S_SETTLE_MS   50
/* 对齐 plat_audio：覆盖 I2S buffer(~2048帧/~42ms) + 余量，冲刷 DMA 残留 */
#define DIVOOM_TONE_LITE_FLUSH_PERIODS   10u
#define DIVOOM_TONE_LITE_FLUSH_DUCK_PERIODS 0u /* 助眠 duck：I2S 常开，勿冲静音抢 SLEEP ring */
#define DIVOOM_TONE_LITE_JL_MUTE_SETTLE_MS 40u
/* 开 I2S 前预解码 PCM 量：覆盖 sink ring + I2S DMA，避免首段播完供数跟不上 */
#define DIVOOM_TONE_LITE_PREFILL_MIN_FRAMES  4096u  /* ~85ms @48k */
#define DIVOOM_TONE_LITE_PREFILL_MAX_FRAMES  24000u /* ~500ms @48k */
#define DIVOOM_TONE_LITE_PREFILL_DUCK_MAX_FRAMES 512u /* duck 叠播：流式送 sink，勿攒 4k+ 帧 */
#define DIVOOM_TONE_LITE_PREFILL_DUCK_MIN_FRAMES 0u
/* sink ring 背压：对齐 luban-lite divoom_audio_player 思路 */
#define DIVOOM_TONE_LITE_BP_LOW_RING_BYTES   2048u  /* 待播数据少 → 优先解码 */
#define DIVOOM_TONE_LITE_BP_MIN_BLANK_BYTES  2048u  /* 空闲不足 → 暂停解码 */
#define DIVOOM_TONE_LITE_BP_WAIT_MAX_MS      50u

typedef struct _divoom_tone_lite_handle {
	divoom_tone_lite_notify_fn notify;
	void *user;
	divoom_tone_lite_control_fn control_hook;
	void *control_hook_user;

	volatile int stop_req;
	volatile int playing;
	int play_mode; /* DIVOOM_TONE_LITE_MODE_*，由 tone_service 在 before_play 后注入 */
	int loop;
	unsigned int end_ticks;
	int vol_0_100;
	int jl_tone_vol;
	int jl_tone_vol_applied;

	char src_path[256];
	int coding;

	int sink_open;
	divoom_audio_sink_ch_t sink_ch;
	volatile int paused;
	unsigned int duration_sec;

	divoom_mp3dec_stream_t *mp3;
	void *mp3_fp;
	int mp3_open;
	unsigned int mp3_rate;
	unsigned int mp3_ch;
	int16_t *mp3_read_buf;

	unsigned int pending_pcm_frames;
	int pending_pcm_valid;

	divoom_aacdec_t *aac;
	int16_t *aac_pcm;
	void *aac_fp;
	uint8_t *aac_buf;
	unsigned int aac_buf_len;
	unsigned int aac_buf_pos;
	unsigned int aac_buf_cap;
	unsigned int aac_rate;
	unsigned int aac_ch;
	int aac_inited;

#ifdef CONFIG_COMPONENTS_SPEEXDSP
	SpeexResamplerState *resampler;
	unsigned int rsmp_ch; /* 实例声道数：mono=1 stereo=2，切歌同 rate 可复用 */
#endif
	unsigned int src_rate;
	unsigned int src_ch;
	int16_t *rsmp_buf;
	unsigned int rsmp_cap_samples;
	int16_t *mono_stereo_buf;
	unsigned int mono_stereo_cap;

	int16_t *prefill_pcm;
	unsigned int prefill_frames;
	unsigned int prefill_cap_frames;
} divoom_tone_lite_handle, *Pdivoom_tone_lite_handle;

static int16_t s_tone_lite_silence[DIVOOM_TONE_LITE_PERIOD_FRAMES * DIVOOM_TONE_LITE_OUT_CH];

static void divoom_tone_lite_flush_pcm_silence(Pdivoom_tone_lite_handle h);

static int divoom_tone_lite_wait_sink_backpressure(Pdivoom_tone_lite_handle h);
static unsigned int divoom_tone_lite_flush_periods_now(Pdivoom_tone_lite_handle h);
static unsigned int divoom_tone_lite_prefill_min_frames(Pdivoom_tone_lite_handle h);
static unsigned int divoom_tone_lite_prefill_max_frames(Pdivoom_tone_lite_handle h);
static int divoom_tone_lite_restart_decoder(Pdivoom_tone_lite_handle h);
static int divoom_tone_lite_decode_stream_loop(Pdivoom_tone_lite_handle h, int *eof_out,
					       int *notify_out);

static int divoom_tone_lite_normalize_path(const char *uri, char *out, unsigned int out_sz)
{
	const char *path = uri;

	if (!uri || !out || out_sz < 2)
	{
		return -1;
	}
	if (strstr(uri, "file://") == uri)
	{
		path = uri + 7;
	}
	strncpy(out, path, out_sz - 1);
	out[out_sz - 1] = '\0';
	return out[0] ? 0 : -1;
}

int divoom_tone_lite_path_supported(const char *path)
{
	int coding;

	if (!path || path[0] == '\0')
	{
		return 0;
	}
	coding = divoom_voice_clip_path_coding_type(path);
	return coding == DIVOOM_TONE_CODING_MP3 || coding == DIVOOM_TONE_CODING_AAC;
}

static void divoom_tone_lite_apply_jl_tone_vol_if_needed(Pdivoom_tone_lite_handle h)
{
	/* 保留钩子；JL 仅 policy prime 一次，响度见 divoom_tone_lite_digital_vol */
	(void)h;
}

static divoom_audio_bus_t divoom_tone_lite_bus(Pdivoom_tone_lite_handle h)
{
#if DIVOOM_SHOW_TF_NAND
	if (h && h->sink_ch == DIVOOM_AUDIO_SINK_CH_MUSIC)
		return DIVOOM_AUDIO_BUS_MUSIC;
#endif
	(void)h;
	return DIVOOM_AUDIO_BUS_TONE;
}

static const char *divoom_tone_lite_bus_owner(Pdivoom_tone_lite_handle h)
{
#if DIVOOM_SHOW_TF_NAND
	if (h && h->sink_ch == DIVOOM_AUDIO_SINK_CH_MUSIC)
		return "music";
#endif
	(void)h;
	return "tone";
}

/*
 * 响度走数字域：JL 的 I2S 档位固定不动（改它会咔哒且必须走 UART），
 * 音量与淡入淡出都由 audio_sink 逐样本完成。
 */
static unsigned int divoom_tone_lite_digital_vol(Pdivoom_tone_lite_handle h)
{
	return h ? (unsigned int)h->vol_0_100 : 100u;
}

/*
 * JL 门控落定后调用：丢掉护垫静音再起淡入。
 * 包络是混音时逐样本推进的，ring 里若还压着静音，淡入会被静音吃光，
 * 内容就变成满幅硬起。
 */
static void divoom_tone_lite_start_content_fade(Pdivoom_tone_lite_handle h)
{
	if (!h)
	{
		return;
	}
	divoom_audio_sink_ch_mute_drop(h->sink_ch);
	divoom_audio_sink_ch_set_volume(h->sink_ch,
					divoom_tone_lite_digital_vol(h));
	divoom_audio_sink_ch_fade_in(h->sink_ch, DIVOOM_TONE_LITE_FADE_MS, 0);
}

static int divoom_tone_lite_ensure_tone_bus(Pdivoom_tone_lite_handle h)
{
	divoom_audio_bus_t bus = divoom_tone_lite_bus(h);

	if (divoom_audio_buf_pool_is_leased(bus))
	{
		return 0;
	}
	if (divoom_audio_buf_pool_acquire(bus, divoom_tone_lite_bus_owner(h)) != 0)
	{
		divoom_print("tone_lite: bus lease fail ch=%u\n", (unsigned)h->sink_ch);
		return -1;
	}
	return 0;
}

static int divoom_tone_lite_mode_is_overlay(Pdivoom_tone_lite_handle h)
{
	return h && h->play_mode == DIVOOM_TONE_LITE_MODE_OVERLAY;
}

static void divoom_tone_lite_flush_pcm_silence_periods(Pdivoom_tone_lite_handle h,
						       unsigned int periods)
{
	unsigned int i;
	unsigned int ok = 0;

	if (!h || !h->sink_open || periods == 0)
	{
		return;
	}
	memset(s_tone_lite_silence, 0, sizeof(s_tone_lite_silence));
	divoom_audio_sink_ch_mute_drop(h->sink_ch);
	for (i = 0; i < periods; i++)
	{
		if (divoom_audio_sink_push_all(h->sink_ch, s_tone_lite_silence,
					       DIVOOM_TONE_LITE_PERIOD_FRAMES) != 0)
		{
			break;
		}
		ok++;
	}
	divoom_print("tone_lite: pcm flush silence periods=%u/%u overlay=%d\n",
		     ok, periods, divoom_tone_lite_mode_is_overlay(h) ? 1 : 0);
}

static int divoom_tone_lite_open_sink_overlay(Pdivoom_tone_lite_handle h)
{
	if (!h)
	{
		return -1;
	}
	if (divoom_tone_lite_ensure_tone_bus(h) != 0)
	{
		return -1;
	}
	if (!divoom_audio_sink_ch_ring_bound(h->sink_ch))
	{
		divoom_audio_sink_ch_enable(h->sink_ch, 1);
	}
	if (!divoom_audio_sink_ch_ring_bound(h->sink_ch))
	{
		divoom_print("tone_lite: duck overlay TONE ring bind fail\n");
		divoom_audio_sink_force_release_bus(h->sink_ch);
		return -1;
	}
	divoom_audio_sink_ch_set_volume(h->sink_ch,
					divoom_tone_lite_digital_vol(h));
	h->sink_open = 1;
	divoom_audio_policy_amp_tip_hold(h->vol_0_100);
	divoom_tone_lite_start_content_fade(h);
	divoom_print("tone_lite: sink overlay ok (sleep+TONE dual bus)\n");
	return 0;
}

static int divoom_tone_lite_open_sink(Pdivoom_tone_lite_handle h)
{
	if (!h)
	{
		return -1;
	}
	/* 切歌 / EOF idle：F101 I2S 已开；tip_hold 确保 JL 门控/prime，响度仍走数字域 */
	if (h->sink_open)
	{
		if (!divoom_audio_sink_ch_ring_bound(h->sink_ch))
		{
			divoom_print("tone_lite: sink reuse but ring gone, reopen\n");
			h->sink_open = 0;
		}
	}
	if (h->sink_open)
	{
		if (divoom_tone_lite_mode_is_overlay(h))
		{
			return divoom_tone_lite_open_sink_overlay(h);
		}
		if (divoom_tone_lite_ensure_tone_bus(h) != 0)
		{
			return -1;
		}
		divoom_audio_sink_ch_set_volume(h->sink_ch,
						divoom_tone_lite_digital_vol(h));
		divoom_tone_lite_flush_pcm_silence(h);
		divoom_audio_policy_amp_tip_hold(h->vol_0_100);
		divoom_tone_lite_start_content_fade(h);
		divoom_print("tone_lite: sink reuse (cut/eof handoff)\n");
		return 0;
	}

	/* 助眠叠播：sink 已由 SLEEP 常开，走轻量 open */
	if (divoom_tone_lite_mode_is_overlay(h))
	{
		return divoom_tone_lite_open_sink_overlay(h);
	}

	/*
	 * 冷开：audio_sink_start → 冲静音 → amp_tip_hold(prime JL 一次) → 数字 fade-in。
	 * I2S 常开保活后不再「先 JL=0 再开时钟」；amp_prepare_i2s_clock 保留为空实现。
	 */
	divoom_audio_policy_amp_prepare_i2s_clock();
	if (divoom_audio_sink_start() != 0)
	{
		divoom_print("tone_lite: audio_sink start fail\n");
		return -1;
	}
	if (divoom_tone_lite_ensure_tone_bus(h) != 0)
	{
		return -1;
	}
	divoom_audio_sink_ch_enable(h->sink_ch, 1);
	if (!divoom_audio_sink_ch_ring_bound(h->sink_ch))
	{
		divoom_print("tone_lite: TONE ring bind fail\n");
		divoom_audio_sink_force_release_bus(h->sink_ch);
		return -1;
	}
	divoom_audio_sink_ch_set_volume(h->sink_ch,
					divoom_tone_lite_digital_vol(h));
	h->sink_open = 1;
	divoom_tone_lite_flush_pcm_silence(h);
	divoom_audio_policy_amp_tip_hold(h->vol_0_100);
	divoom_tone_lite_start_content_fade(h);
	divoom_print("tone_lite: sink open ok 48k/2ch\n");
	return 0;
}

static void divoom_tone_lite_close_sink(Pdivoom_tone_lite_handle h)
{
	if (!h || !h->sink_open)
	{
		return;
	}
	divoom_tone_lite_flush_pcm_silence(h);
	divoom_audio_policy_amp_tip_release();
	divoom_tone_lite_flush_pcm_silence(h);
	divoom_audio_sink_ch_disable(h->sink_ch);
	divoom_audio_sink_try_stop();
	h->sink_open = 0;
}

static int divoom_tone_lite_write_pcm(Pdivoom_tone_lite_handle h, const int16_t *interleaved,
				      unsigned int frames)
{
	if (!h || !h->sink_open || !interleaved || frames == 0)
	{
		return -1;
	}
	if (h->stop_req)
	{
		return 0;
	}
	if (divoom_audio_sink_push_all(h->sink_ch, interleaved, frames) != 0)
	{
		return -1;
	}
#if DIVOOM_SHOW_TF_NAND
	if (h->sink_ch == DIVOOM_AUDIO_SINK_CH_MUSIC)
		divoom_spectrum_process(interleaved, (uint16)frames, 2, DIVOOM_TONE_LITE_OUT_RATE);
#endif
	divoom_tone_lite_apply_jl_tone_vol_if_needed(h);
	return 0;
}

static void divoom_tone_lite_flush_pcm_silence(Pdivoom_tone_lite_handle h)
{
	divoom_tone_lite_flush_pcm_silence_periods(h, divoom_tone_lite_flush_periods_now(h));
}

/* 关流收尾不再直接拧 JL：由 audio_policy amp tip/sleep 门控统一 On/Off。 */
static void divoom_tone_lite_mute_jl_before_close_ex(Pdivoom_tone_lite_handle h, int force)
{
	(void)force;
	if (!h)
	{
		return;
	}
	h->jl_tone_vol = 0;
	h->jl_tone_vol_applied = 1;
}

static void divoom_tone_lite_mute_jl_before_close(Pdivoom_tone_lite_handle h)
{
	divoom_tone_lite_mute_jl_before_close_ex(h, 0);
}

static int divoom_tone_lite_ensure_mono_stereo(Pdivoom_tone_lite_handle h, unsigned int frames)
{
	unsigned int need = frames * DIVOOM_TONE_LITE_OUT_CH;

	if (!h)
	{
		return -1;
	}
	if (h->mono_stereo_cap >= need)
	{
		return 0;
	}
	if (h->mono_stereo_buf)
	{
		divoom_system_free(h->mono_stereo_buf);
	}
	h->mono_stereo_buf = (int16_t *)divoom_system_malloc(need * sizeof(int16_t));
	if (!h->mono_stereo_buf)
	{
		h->mono_stereo_cap = 0;
		return -1;
	}
	h->mono_stereo_cap = need;
	return 0;
}

static int divoom_tone_lite_ensure_rsmp_buf(Pdivoom_tone_lite_handle h, unsigned int interleaved_samples)
{
	if (!h)
	{
		return -1;
	}
	if (h->rsmp_cap_samples >= interleaved_samples)
	{
		return 0;
	}
	if (h->rsmp_buf)
	{
		divoom_system_free(h->rsmp_buf);
	}
	h->rsmp_buf = (int16_t *)divoom_system_malloc(interleaved_samples * sizeof(int16_t));
	if (!h->rsmp_buf)
	{
		h->rsmp_cap_samples = 0;
		return -1;
	}
	h->rsmp_cap_samples = interleaved_samples;
	return 0;
}

static unsigned int divoom_tone_lite_rsmp_ch(unsigned int src_ch)
{
	return (src_ch >= 2u) ? DIVOOM_TONE_LITE_OUT_CH : 1u;
}

/* mono 变采样结果就地扩成立体声：必须从后往前，避免覆盖未读采样 */
static void divoom_tone_lite_expand_mono_to_stereo(int16_t *buf, unsigned int frames)
{
	unsigned int i;

	if (!buf || frames == 0)
	{
		return;
	}
	i = frames;
	while (i > 0)
	{
		int16_t s;

		i--;
		s = buf[i];
		buf[i * 2] = s;
		buf[i * 2 + 1] = s;
	}
}

static int divoom_tone_lite_setup_resampler(Pdivoom_tone_lite_handle h, unsigned int src_rate,
					    unsigned int src_ch)
{
	unsigned int want_ch;

	if (!h)
	{
		return -1;
	}
	h->src_rate = src_rate ? src_rate : DIVOOM_TONE_LITE_OUT_RATE;
	h->src_ch = src_ch ? src_ch : 1u;
	want_ch = divoom_tone_lite_rsmp_ch(h->src_ch);

#ifdef CONFIG_COMPONENTS_SPEEXDSP
	if (h->src_rate == DIVOOM_TONE_LITE_OUT_RATE)
	{
		if (h->resampler)
		{
			speex_resampler_destroy(h->resampler);
			h->resampler = NULL;
		}
		h->rsmp_ch = 0;
		return 0;
	}
	if (h->resampler && h->rsmp_ch == want_ch)
	{
		spx_uint32_t in_r = 0;
		spx_uint32_t out_r = 0;

		speex_resampler_get_rate(h->resampler, &in_r, &out_r);
		if (in_r == h->src_rate && out_r == DIVOOM_TONE_LITE_OUT_RATE)
		{
			speex_resampler_reset_mem(h->resampler);
			return 0;
		}
	}
	if (h->resampler)
	{
		speex_resampler_destroy(h->resampler);
		h->resampler = NULL;
	}
	{
		int err = 0;

		h->resampler = speex_resampler_init_frac(want_ch,
							 h->src_rate,
							 DIVOOM_TONE_LITE_OUT_RATE,
							 h->src_rate,
							 DIVOOM_TONE_LITE_OUT_RATE,
							 1, /* Q1：16 抽头；须与 RESAMPLE_FULL_SINC_TABLE 同开 */
							 &err);
		if (!h->resampler || err != 0)
		{
			h->rsmp_ch = 0;
			divoom_print("tone_lite: speex init fail err=%d\n", err);
			return -1;
		}
		h->rsmp_ch = want_ch;
		divoom_print("tone_lite: resample %u -> 48k ch=%u\n", h->src_rate, want_ch);
	}
#else
	if (h->src_rate != DIVOOM_TONE_LITE_OUT_RATE)
	{
		divoom_print("tone_lite: need SPEEXDSP for rate %u\n", h->src_rate);
		return -1;
	}
#endif
	return 0;
}

static int divoom_tone_lite_push_pcm_block(Pdivoom_tone_lite_handle h, const int16_t *pcm,
					  unsigned int ch, unsigned int frames, int to_sink)
{
	const int16_t *stereo_in;
	unsigned int i;

	if (!h || !pcm || frames == 0)
	{
		return 0;
	}

#ifdef CONFIG_COMPONENTS_SPEEXDSP
	if (h->src_rate != DIVOOM_TONE_LITE_OUT_RATE)
	{
		unsigned int want_ch = divoom_tone_lite_rsmp_ch(ch);

		if (!h->resampler || h->rsmp_ch != want_ch)
		{
			if (divoom_tone_lite_setup_resampler(h, h->src_rate, ch) != 0)
			{
				return -1;
			}
		}
	}
	if (h->resampler)
	{
		spx_uint32_t in_len = frames;
		spx_uint32_t out_len;
		unsigned int out_cap;
		int ret;

		out_cap = (unsigned int)(((unsigned long long)frames * DIVOOM_TONE_LITE_OUT_RATE) /
					 (h->src_rate ? h->src_rate : DIVOOM_TONE_LITE_OUT_RATE)) + 64u;
		if (divoom_tone_lite_ensure_rsmp_buf(h, out_cap * DIVOOM_TONE_LITE_OUT_CH) != 0)
		{
			return -1;
		}
		out_len = out_cap;
		if (ch == 1u)
		{
			ret = speex_resampler_process_int(h->resampler, 0, pcm, &in_len,
							  h->rsmp_buf, &out_len);
		}
		else
		{
			ret = speex_resampler_process_interleaved_int(h->resampler, pcm, &in_len,
								      h->rsmp_buf, &out_len);
		}
		if (ret != 0)
		{
			divoom_print("tone_lite: resample fail %d\n", ret);
			return -1;
		}
		if (ch == 1u)
		{
			divoom_tone_lite_expand_mono_to_stereo(h->rsmp_buf, out_len);
		}
		if (!to_sink)
		{
			h->pending_pcm_frames = out_len;
			h->pending_pcm_valid = 1;
			return 0;
		}
		return divoom_tone_lite_write_pcm(h, h->rsmp_buf, out_len);
	}
#endif
	if (ch == 1u)
	{
		if (divoom_tone_lite_ensure_mono_stereo(h, frames) != 0)
		{
			return -1;
		}
		for (i = 0; i < frames; i++)
		{
			h->mono_stereo_buf[i * 2] = pcm[i];
			h->mono_stereo_buf[i * 2 + 1] = pcm[i];
		}
		stereo_in = h->mono_stereo_buf;
	}
	else
	{
		stereo_in = pcm;
	}
	if (divoom_tone_lite_ensure_rsmp_buf(h, frames * DIVOOM_TONE_LITE_OUT_CH) != 0)
	{
		return -1;
	}
	memcpy(h->rsmp_buf, stereo_in, frames * DIVOOM_TONE_LITE_OUT_CH * sizeof(int16_t));
	if (!to_sink)
	{
		h->pending_pcm_frames = frames;
		h->pending_pcm_valid = 1;
		return 0;
	}
	return divoom_tone_lite_write_pcm(h, h->rsmp_buf, frames);
}

static int divoom_tone_lite_decode_once(Pdivoom_tone_lite_handle h, int to_sink);

static void divoom_tone_lite_reset_prefill(Pdivoom_tone_lite_handle h)
{
	if (!h)
	{
		return;
	}
	h->prefill_frames = 0;
}

static void divoom_tone_lite_release_all_scratch(Pdivoom_tone_lite_handle h)
{
	if (!h)
	{
		return;
	}
	if (h->prefill_pcm)
	{
		divoom_system_free(h->prefill_pcm);
		h->prefill_pcm = NULL;
	}
	h->prefill_frames = 0;
	h->prefill_cap_frames = 0;
	if (h->mp3_read_buf)
	{
		divoom_system_free(h->mp3_read_buf);
		h->mp3_read_buf = NULL;
	}
	if (h->aac_buf)
	{
		divoom_system_free(h->aac_buf);
		h->aac_buf = NULL;
	}
	h->aac_buf_len = 0;
	h->aac_buf_pos = 0;
	h->aac_buf_cap = 0;
	if (h->aac_pcm)
	{
		divoom_system_free(h->aac_pcm);
		h->aac_pcm = NULL;
	}
	if (h->rsmp_buf)
	{
		divoom_system_free(h->rsmp_buf);
		h->rsmp_buf = NULL;
		h->rsmp_cap_samples = 0;
	}
	if (h->mono_stereo_buf)
	{
		divoom_system_free(h->mono_stereo_buf);
		h->mono_stereo_buf = NULL;
		h->mono_stereo_cap = 0;
	}
#ifdef CONFIG_COMPONENTS_SPEEXDSP
	if (h->resampler)
	{
		speex_resampler_destroy(h->resampler);
		h->resampler = NULL;
	}
	h->rsmp_ch = 0;
#endif
}

static unsigned int divoom_tone_lite_prefill_min_frames(Pdivoom_tone_lite_handle h)
{
	if (divoom_tone_lite_mode_is_overlay(h))
	{
		return DIVOOM_TONE_LITE_PREFILL_DUCK_MIN_FRAMES;
	}
	return DIVOOM_TONE_LITE_PREFILL_MIN_FRAMES;
}

static unsigned int divoom_tone_lite_prefill_max_frames(Pdivoom_tone_lite_handle h)
{
	if (divoom_tone_lite_mode_is_overlay(h))
	{
		return DIVOOM_TONE_LITE_PREFILL_DUCK_MAX_FRAMES;
	}
	return DIVOOM_TONE_LITE_PREFILL_MAX_FRAMES;
}

static unsigned int divoom_tone_lite_flush_periods_now(Pdivoom_tone_lite_handle h)
{
	if (divoom_tone_lite_mode_is_overlay(h) && divoom_audio_sink_is_running())
	{
		return DIVOOM_TONE_LITE_FLUSH_DUCK_PERIODS;
	}
	return DIVOOM_TONE_LITE_FLUSH_PERIODS;
}

static void divoom_tone_lite_invoke_control(Pdivoom_tone_lite_handle h)
{
	if (h && h->control_hook)
	{
		h->control_hook(h->control_hook_user);
	}
}

static int divoom_tone_lite_decode_stream_loop(Pdivoom_tone_lite_handle h, int *eof_out, int *notify_out)
{
	int eof = 0;

	if (!h || !eof_out || !notify_out)
	{
		return -1;
	}
	*eof_out = 0;
	while (!h->stop_req)
	{
		int ret;

		while (h->paused && !h->stop_req)
		{
			divoom_tone_lite_invoke_control(h);
			divoom_system_delay_ms(20);
		}
		if (h->stop_req)
			break;

		if (divoom_tone_lite_wait_sink_backpressure(h) != 0)
		{
			break;
		}
		ret = divoom_tone_lite_decode_once(h, 1);
		divoom_system_yield();
		divoom_tone_lite_invoke_control(h);

		if (ret < 0)
		{
			*notify_out = DIVOOM_TONE_LITE_NOTIFY_ERROR;
			return -1;
		}
		if (ret > 0)
		{
			eof = 1;
			if (h->loop)
			{
				if (h->end_ticks != 0
				    && divoom_system_get_ticks() >= h->end_ticks)
				{
					break;
				}
				if (divoom_tone_lite_restart_decoder(h) != 0)
				{
					*notify_out = DIVOOM_TONE_LITE_NOTIFY_ERROR;
					return -1;
				}
				eof = 0;
				continue;
			}
			break;
		}
	}
	*eof_out = eof;
	return 0;
}

static int divoom_tone_lite_prefill_append_pending(Pdivoom_tone_lite_handle h)
{
	unsigned int n;
	unsigned int need;
	int16_t *nb;

	if (!h || !h->pending_pcm_valid || h->pending_pcm_frames == 0)
	{
		return 0;
	}
	n = h->pending_pcm_frames;
	need = h->prefill_frames + n;
	if (need > divoom_tone_lite_prefill_max_frames(h))
	{
		return -1;
	}
	if (need > h->prefill_cap_frames)
	{
		unsigned int new_cap = h->prefill_cap_frames ? h->prefill_cap_frames : n;
		unsigned int max_frames = divoom_tone_lite_prefill_max_frames(h);

		while (new_cap < need)
		{
			new_cap *= 2u;
		}
		if (new_cap > max_frames)
		{
			new_cap = max_frames;
		}
		nb = (int16_t *)divoom_system_malloc(new_cap * DIVOOM_TONE_LITE_OUT_CH * sizeof(int16_t));
		if (!nb)
		{
			return -1;
		}
		if (h->prefill_pcm && h->prefill_frames > 0)
		{
			memcpy(nb, h->prefill_pcm,
			       h->prefill_frames * DIVOOM_TONE_LITE_OUT_CH * sizeof(int16_t));
		}
		if (h->prefill_pcm)
		{
			divoom_system_free(h->prefill_pcm);
		}
		h->prefill_pcm = nb;
		h->prefill_cap_frames = new_cap;
	}
	memcpy(h->prefill_pcm + h->prefill_frames * DIVOOM_TONE_LITE_OUT_CH, h->rsmp_buf,
	       n * DIVOOM_TONE_LITE_OUT_CH * sizeof(int16_t));
	h->prefill_frames += n;
	h->pending_pcm_valid = 0;
	h->pending_pcm_frames = 0;
	return 0;
}

static int divoom_tone_lite_build_prefill(Pdivoom_tone_lite_handle h, int *all_eof)
{
	int ret;
	int eof = 0;
	unsigned int prefill_max = divoom_tone_lite_prefill_max_frames(h);

	if (!h || !all_eof)
	{
		return -1;
	}
	*all_eof = 0;
	divoom_tone_lite_reset_prefill(h);
	for (;;)
	{
		if (h->prefill_frames >= divoom_tone_lite_prefill_min_frames(h))
		{
			break;
		}
		if (h->prefill_frames >= prefill_max)
		{
			break;
		}
		ret = divoom_tone_lite_decode_once(h, 0);
		if (ret < 0)
		{
			return -1;
		}
		if (ret > 0)
		{
			if (h->pending_pcm_valid &&
			    divoom_tone_lite_prefill_append_pending(h) != 0)
			{
				return -1;
			}
			eof = 1;
			break;
		}
		if (!h->pending_pcm_valid)
		{
			continue;
		}
		if (divoom_tone_lite_prefill_append_pending(h) != 0)
		{
			return -1;
		}
		divoom_tone_lite_invoke_control(h);
	}
	if (h->prefill_frames == 0)
	{
		return -1;
	}
	*all_eof = eof;
	divoom_print("tone_lite: prefill ok frames=%u eof=%d\n", h->prefill_frames, eof);
	return 0;
}

static int divoom_tone_lite_play_prefill(Pdivoom_tone_lite_handle h)
{
	unsigned int off = 0;

	if (!h || !h->prefill_pcm || h->prefill_frames == 0)
	{
		return 0;
	}
	while (off < h->prefill_frames && !h->stop_req)
	{
		unsigned int chunk = h->prefill_frames - off;

		if (chunk > DIVOOM_TONE_LITE_PERIOD_FRAMES)
		{
			chunk = DIVOOM_TONE_LITE_PERIOD_FRAMES;
		}
		if (divoom_tone_lite_write_pcm(h, h->prefill_pcm + off * DIVOOM_TONE_LITE_OUT_CH,
					       chunk) != 0)
		{
			return -1;
		}
		off += chunk;
		divoom_tone_lite_invoke_control(h);
	}
	return 0;
}

static size_t divoom_tone_lite_mp3_io_read(void *buf, size_t size, void *user_data)
{
	void *fp = user_data;

	if (!fp || !buf || size == 0)
	{
		return 0;
	}
	return divoom_port_file_read(buf, 1, size, fp);
}

static int divoom_tone_lite_mp3_io_seek(uint32_t position, void *user_data)
{
	void *fp = user_data;

	if (!fp)
	{
		return -1;
	}
	return divoom_port_file_seek(fp, (long)position, DIVOOM_PORT_SEEK_SET);
}

static void divoom_tone_lite_teardown_decode(Pdivoom_tone_lite_handle h)
{
	if (!h)
	{
		return;
	}
	if (h->mp3_open)
	{
		divoom_mp3dec_stream_close(h->mp3);
		h->mp3 = NULL;
		h->mp3_open = 0;
	}
	if (h->mp3_fp)
	{
		divoom_port_file_close(h->mp3_fp);
		h->mp3_fp = NULL;
	}
	/* scratch 池：mp3_read_buf 等切歌保留，destroy 时 release_all_scratch */
	h->pending_pcm_valid = 0;
	h->pending_pcm_frames = 0;
	divoom_aacdec_destroy(h->aac);
	h->aac = NULL;
	/* aac_buf / aac_pcm 切歌保留，见 release_all_scratch */
	if (h->aac_fp)
	{
		divoom_port_file_close(h->aac_fp);
		h->aac_fp = NULL;
	}
	h->aac_buf_len = 0;
	h->aac_buf_pos = 0;
	h->aac_inited = 0;
	/* Speex 实例切歌保留，同 rate/声道在 setup 里复用；destroy 时 release_all_scratch */
	divoom_system_yield();
}

static int divoom_tone_lite_open_mp3(Pdivoom_tone_lite_handle h)
{
	int ret;
	long file_size = -1;
	divoom_mp3dec_stream_info_t info;

	if (!h)
	{
		return -1;
	}
	if (h->mp3_fp)
	{
		divoom_port_file_close(h->mp3_fp);
		h->mp3_fp = NULL;
	}
	h->mp3_fp = divoom_port_file_open(h->src_path, "rb");
	if (!h->mp3_fp)
	{
		divoom_print("tone_lite: mp3 open fail %s\n", h->src_path);
		return -1;
	}
	if (divoom_port_file_seek(h->mp3_fp, 0, DIVOOM_PORT_SEEK_END) != 0
	    || (file_size = divoom_port_file_tell(h->mp3_fp)) <= 0
	    || divoom_port_file_seek(h->mp3_fp, 0, DIVOOM_PORT_SEEK_SET) != 0)
	{
		divoom_print("tone_lite: mp3 bad size %s bytes=%ld\n", h->src_path, file_size);
		divoom_port_file_close(h->mp3_fp);
		h->mp3_fp = NULL;
		return -1;
	}
	/* 流式 open：仅 16KB IO 缓冲，不整文件 malloc */
	h->mp3 = divoom_mp3dec_stream_open(divoom_tone_lite_mp3_io_read, divoom_tone_lite_mp3_io_seek,
					   h->mp3_fp, &info);
	if (!h->mp3)
	{
		divoom_print("tone_lite: mp3 open fail %s\n", h->src_path);
		divoom_port_file_close(h->mp3_fp);
		h->mp3_fp = NULL;
		return -1;
	}
	h->mp3_open = 1;
	h->mp3_rate = info.hz;
	h->mp3_ch = info.channels;
	if (h->mp3_rate == 0)
	{
		h->mp3_rate = 44100u;
	}
	if (h->mp3_ch == 0)
	{
		h->mp3_ch = 2u;
	}
	h->duration_sec = 0;
	if (info.total_frames > 0)
		h->duration_sec = info.total_frames / h->mp3_rate;
	else if (file_size > 0 && info.bitrate_kbps > 0)
		h->duration_sec = (unsigned int)((file_size * 8L) / ((long)info.bitrate_kbps * 1000L));
	if (!h->mp3_read_buf)
	{
		h->mp3_read_buf = (int16_t *)divoom_system_malloc(
			DIVOOM_TONE_LITE_MP3_READ_SAMPLES * sizeof(int16_t));
		if (!h->mp3_read_buf)
		{
			divoom_mp3dec_stream_close(h->mp3);
			h->mp3 = NULL;
			h->mp3_open = 0;
			return -1;
		}
	}
	ret = divoom_tone_lite_setup_resampler(h, h->mp3_rate, h->mp3_ch);
	divoom_system_yield();
	return ret;
}

/* 0：读到数据或缓冲已满；1：文件没有更多数据。 */
static int divoom_tone_lite_aac_refill(Pdivoom_tone_lite_handle h)
{
	size_t nr;

	if (!h || !h->aac_fp || !h->aac_buf)
	{
		return -1;
	}
	if (h->aac_buf_pos > 0 && h->aac_buf_len >= h->aac_buf_pos)
	{
		memmove(h->aac_buf, h->aac_buf + h->aac_buf_pos, h->aac_buf_len - h->aac_buf_pos);
		h->aac_buf_len -= h->aac_buf_pos;
		h->aac_buf_pos = 0;
	}
	if (h->aac_buf_len >= h->aac_buf_cap)
	{
		return 0;
	}
	nr = divoom_port_file_read(h->aac_buf + h->aac_buf_len, 1,
				   h->aac_buf_cap - h->aac_buf_len, h->aac_fp);
	h->aac_buf_len += (unsigned int)nr;
	return nr ? 0 : 1;
}

static int divoom_tone_lite_open_aac(Pdivoom_tone_lite_handle h)
{
	divoom_aacdec_frame_info_t info;
	int ret;

	if (!h)
	{
		return -1;
	}
	h->aac_fp = divoom_port_file_open(h->src_path, "rb");
	if (!h->aac_fp)
	{
		divoom_print("tone_lite: aac open fail %s\n", h->src_path);
		return -1;
	}
	h->aac_buf_cap = DIVOOM_TONE_LITE_AAC_IO_BYTES * 4u;
	if (!h->aac_buf)
	{
		h->aac_buf = (uint8_t *)divoom_system_malloc(h->aac_buf_cap);
		if (!h->aac_buf)
		{
			goto fail;
		}
	}
	if (!h->aac_pcm)
	{
		h->aac_pcm = (int16_t *)divoom_system_malloc(
			(unsigned int)(DIVOOM_AACDEC_MAX_SAMPLES_PER_FRAME * sizeof(int16_t)));
		if (!h->aac_pcm)
		{
			goto fail;
		}
	}
	if (divoom_tone_lite_aac_refill(h) != 0)
	{
		divoom_print("tone_lite: aac empty %s\n", h->src_path);
		goto fail;
	}
	h->aac = divoom_aacdec_create();
	if (!h->aac)
	{
		divoom_print("tone_lite: aac init fail %s\n", h->src_path);
		goto fail;
	}
	/* 只验帧头。HE 的输出采样率要解出首帧才知道，重采样器在首帧再建。 */
	ret = divoom_aacdec_decode_frame(h->aac, h->aac_buf, (int)h->aac_buf_len, NULL, &info);
	if (ret == DIVOOM_AACDEC_ERR_UNSUPPORTED || !info.frame_bytes)
	{
		divoom_print("tone_lite: aac %s %s\n", ret ? "unsupported" : "no adts", h->src_path);
		goto fail;
	}
	h->aac_buf_pos = (unsigned int)info.frame_offset;
	h->aac_rate = 0;
	h->aac_ch = 0;
	h->aac_inited = 1;
	divoom_print("tone_lite: aac %s core=%d ch=%d\n", h->src_path, info.core_hz, info.channels);
	divoom_system_yield();
	return 0;

fail:
	divoom_tone_lite_teardown_decode(h);
	return -1;
}

static int divoom_tone_lite_restart_mp3(Pdivoom_tone_lite_handle h)
{
	if (!h || !h->mp3_open)
	{
		return -1;
	}
	if (divoom_mp3dec_stream_rewind(h->mp3) != 0)
	{
		divoom_mp3dec_stream_close(h->mp3);
		h->mp3 = NULL;
		h->mp3_open = 0;
		return divoom_tone_lite_open_mp3(h);
	}
#ifdef CONFIG_COMPONENTS_SPEEXDSP
	if (h->resampler)
	{
		speex_resampler_reset_mem(h->resampler);
	}
#endif
	return 0;
}

static int divoom_tone_lite_restart_aac(Pdivoom_tone_lite_handle h)
{
	if (!h)
	{
		return -1;
	}
	divoom_tone_lite_teardown_decode(h);
	return divoom_tone_lite_open_aac(h);
}

static int divoom_tone_lite_decode_mp3_once(Pdivoom_tone_lite_handle h, int to_sink)
{
	size_t got;
	unsigned int ch;
	unsigned int frames;

	if (!h || !h->mp3_open || !h->mp3_read_buf)
	{
		return -1;
	}
	got = divoom_mp3dec_stream_read(h->mp3, h->mp3_read_buf, DIVOOM_TONE_LITE_MP3_READ_SAMPLES);
	if (got == 0)
	{
		return 1;
	}
	ch = h->mp3_ch ? h->mp3_ch : 2u;
	frames = (unsigned int)(got / ch);
	if (frames > 0)
	{
		if (divoom_tone_lite_push_pcm_block(h, h->mp3_read_buf, ch, frames,
						    to_sink) != 0)
		{
			return -1;
		}
	}
	return 0;
}

static int divoom_tone_lite_decode_aac_once(Pdivoom_tone_lite_handle h, int to_sink)
{
	int eof = 0;

	if (!h || !h->aac || !h->aac_inited || !h->aac_pcm)
	{
		return -1;
	}
	for (;;)
	{
		divoom_aacdec_frame_info_t info;
		unsigned int avail = h->aac_buf_len - h->aac_buf_pos;
		unsigned int drop;
		int frames;

		/* ADTS 帧最长 8191 字节：剩余不到半个缓冲就补，保证总能凑齐一帧。 */
		if (!eof && avail < h->aac_buf_cap / 2u)
		{
			eof = divoom_tone_lite_aac_refill(h) != 0;
			avail = h->aac_buf_len - h->aac_buf_pos;
		}
		if (avail == 0)
		{
			return 1;
		}
		frames = divoom_aacdec_decode_frame(h->aac, h->aac_buf + h->aac_buf_pos, (int)avail,
						    h->aac_pcm, &info);
		if (frames == -1)
		{
			return -1;
		}
		drop = (unsigned int)(info.frame_bytes ? info.frame_bytes : info.frame_offset);
		if (drop > avail)
		{
			return -1;
		}
		h->aac_buf_pos += drop;
		if (!info.frame_bytes)
		{
			if (eof)
			{
				h->aac_buf_pos = h->aac_buf_len;
				return 1;
			}
			if (avail - drop >= h->aac_buf_cap / 2u)
			{
				h->aac_buf_pos++;
			}
			continue;
		}
		/* 坏帧或不支持的帧已被跳过，接着找下一帧。 */
		if (frames <= 0)
		{
			continue;
		}
		if ((unsigned int)info.hz != h->aac_rate || (unsigned int)info.channels != h->aac_ch)
		{
			h->aac_rate = (unsigned int)info.hz;
			h->aac_ch = (unsigned int)info.channels;
			if (divoom_tone_lite_setup_resampler(h, h->aac_rate, h->aac_ch) != 0)
			{
				return -1;
			}
		}
		return divoom_tone_lite_push_pcm_block(h, h->aac_pcm, h->aac_ch, (unsigned int)frames,
						       to_sink);
	}
}

static int divoom_tone_lite_open_decoder(Pdivoom_tone_lite_handle h)
{
	if (!h)
	{
		return -1;
	}
	if (h->coding == DIVOOM_TONE_CODING_MP3)
	{
		return divoom_tone_lite_open_mp3(h);
	}
	if (h->coding == DIVOOM_TONE_CODING_AAC)
	{
		return divoom_tone_lite_open_aac(h);
	}
	return -1;
}

static int divoom_tone_lite_decode_once(Pdivoom_tone_lite_handle h, int to_sink)
{
	if (!h)
	{
		return -1;
	}
	if (h->coding == DIVOOM_TONE_CODING_MP3)
	{
		return divoom_tone_lite_decode_mp3_once(h, to_sink);
	}
	if (h->coding == DIVOOM_TONE_CODING_AAC)
	{
		return divoom_tone_lite_decode_aac_once(h, to_sink);
	}
	return -1;
}

static int divoom_tone_lite_restart_decoder(Pdivoom_tone_lite_handle h)
{
	if (!h)
	{
		return -1;
	}
	if (h->coding == DIVOOM_TONE_CODING_MP3)
	{
		return divoom_tone_lite_restart_mp3(h);
	}
	if (h->coding == DIVOOM_TONE_CODING_AAC)
	{
		return divoom_tone_lite_restart_aac(h);
	}
	return -1;
}

static void divoom_tone_lite_drain_stop(Pdivoom_tone_lite_handle h)
{
	if (!h || !h->sink_open)
	{
		return;
	}
	/*
	 * 助眠 duck 叠播快速切歌：勿 fade+flush 阻塞 local_tone（APP 疯狂试听会卡 UI）。
	 * mute_drop 丢掉 TONE ring 残留即可，SLEEP 双 bus 继续产流。
	 */
	if (divoom_tone_lite_mode_is_overlay(h))
	{
		divoom_audio_sink_ch_mute_drop(h->sink_ch);
		divoom_print("tone_lite: overlay cut drain (mute_drop only)\n");
		return;
	}
	divoom_audio_sink_ch_fade_out(h->sink_ch, DIVOOM_TONE_LITE_CUT_FADE_MS);
	(void)divoom_audio_sink_ch_fade_out_wait(h->sink_ch, 200u);
	/* 切歌：仅数字 fade + 静音冲刷；同会话 JL 门控保持 On */
	divoom_tone_lite_flush_pcm_silence(h);
}

/* 自然 EOF：数字 fade + 静音冲刷；JL/关流在 close_sink（先 tip_release 再停 I2S） */
static void divoom_tone_lite_drain_eof(Pdivoom_tone_lite_handle h)
{
	if (!h || !h->sink_open)
	{
		return;
	}
	divoom_audio_sink_ch_fade_out(h->sink_ch, DIVOOM_TONE_LITE_FADE_MS);
	(void)divoom_audio_sink_ch_fade_out_wait(h->sink_ch, 500u);
	divoom_tone_lite_flush_pcm_silence(h);
	if (!divoom_tone_lite_mode_is_overlay(h))
	{
		divoom_system_delay_ms(DIVOOM_TONE_LITE_I2S_SETTLE_MS);
	}
}

/* ring 快满时让 I2S 消化；快空时立即返回继续解码 */
static int divoom_tone_lite_wait_sink_backpressure(Pdivoom_tone_lite_handle h)
{
	unsigned int waited_ms = 0;

	if (!h || !h->sink_open)
	{
		return 0;
	}
	while (!h->stop_req && waited_ms < DIVOOM_TONE_LITE_BP_WAIT_MAX_MS)
	{
		unsigned int ring_bytes =
			divoom_audio_sink_ch_ring_bytes(h->sink_ch);
		unsigned int blank_bytes =
			divoom_audio_sink_ch_ring_blank_bytes(h->sink_ch);

		if (ring_bytes < DIVOOM_TONE_LITE_BP_LOW_RING_BYTES)
		{
			return 0;
		}
		if (blank_bytes >= DIVOOM_TONE_LITE_BP_MIN_BLANK_BYTES)
		{
			return 0;
		}
		divoom_system_delay_ms(1);
		waited_ms++;
		divoom_tone_lite_invoke_control(h);
	}
	return h->stop_req ? -1 : 0;
}

static void divoom_tone_lite_play_worker(Pdivoom_tone_lite_handle h)
{
	int notify = DIVOOM_TONE_LITE_NOTIFY_COMPLETE;
	int got_any = 0;
	int eof = 0;
	int prefill_eof = 0;
	int tone_prio_saved = -1;
	int overlay_stream = 0;

	if (!h)
	{
		return;
	}
	overlay_stream = divoom_tone_lite_mode_is_overlay(h);
	tone_prio_saved = divoom_system_task_get_current_prio();
	/*
	 * OVERLAY：sink 已由助眠常开，勿提权到 5 以上抢 LVGL。
	 * COLD：prefill+push_all 临时提权抢供数。
	 */
	if (!overlay_stream)
	{
		divoom_system_task_set_current_prio(DIVOOM_LOCAL_TONE_PLAY_TASK_PRIO);
	}
	divoom_print("tone_lite: play prio %d->%d mode=%d sink=%d lvgl=%d\n",
		     tone_prio_saved,
		     overlay_stream ? tone_prio_saved : DIVOOM_LOCAL_TONE_PLAY_TASK_PRIO,
		     h->play_mode, DIVOOM_AUDIO_SINK_TASK_PRIO,
		     DIVOOM_LVGL_HANDLER_TASK_PRIO);
	h->playing = 1;
	h->paused = 0;
	h->pending_pcm_valid = 0;
	h->pending_pcm_frames = 0;
	divoom_tone_lite_reset_prefill(h);
	divoom_print("tone_lite: worker start %s coding=%d\n", h->src_path, h->coding);

	if (divoom_tone_lite_open_decoder(h) != 0)
	{
		notify = DIVOOM_TONE_LITE_NOTIFY_ERROR;
		goto done;
	}

	if (overlay_stream)
	{
		if (divoom_tone_lite_open_sink(h) != 0)
		{
			notify = DIVOOM_TONE_LITE_NOTIFY_ERROR;
			goto done;
		}
		got_any = 1;
		if (divoom_tone_lite_decode_stream_loop(h, &eof, &notify) != 0)
		{
			goto done;
		}
	}
	else
	{
		/* 冷启动：先预解码一段 PCM，再开 I2S；勿在真实 PCM 后插静音 keepalive */
		if (divoom_tone_lite_build_prefill(h, &prefill_eof) != 0)
		{
			divoom_print("tone_lite: prefill fail\n");
			notify = DIVOOM_TONE_LITE_NOTIFY_ERROR;
			goto done;
		}
		got_any = 1;
		if (divoom_tone_lite_open_sink(h) != 0)
		{
			notify = DIVOOM_TONE_LITE_NOTIFY_ERROR;
			goto done;
		}
		if (divoom_tone_lite_play_prefill(h) != 0)
		{
			notify = DIVOOM_TONE_LITE_NOTIFY_ERROR;
			goto done;
		}
		if (!prefill_eof)
		{
			if (divoom_tone_lite_decode_stream_loop(h, &eof, &notify) != 0)
			{
				goto done;
			}
		}
		else
		{
			eof = 1;
		}
	}

	if (h->stop_req)
	{
		divoom_tone_lite_drain_stop(h);
	}
	else if (got_any && h->sink_open)
	{
		divoom_tone_lite_drain_eof(h);
	}

done:
	{
		int would_keep = 0;

		if (tone_prio_saved >= 0)
		{
			divoom_system_task_set_current_prio(tone_prio_saved);
		}
		divoom_tone_lite_reset_prefill(h);
		divoom_tone_lite_teardown_decode(h);
#if DIVOOM_BUILD_RTOS_DEVICE
		would_keep = divoom_tone_service_should_keep_sink(
			h->stop_req, eof, (divoom_tone_lite_play_mode_t)h->play_mode);
#endif
		if (h->sink_open && would_keep)
		{
			divoom_print("tone_lite: sink keep mode=%d cut_stop=%d eof=%d\n",
				     h->play_mode, h->stop_req, eof);
		}
		else if (h->sink_open)
		{
			divoom_tone_lite_close_sink(h);
		}
		h->playing = 0;
		if (!h->stop_req && h->notify)
		{
			h->notify(h->user, notify);
		}
		divoom_print("tone_lite: worker exit notify=%d stop=%d eof=%d keep=%d stack_hwm=%u bytes\n",
			     notify, h->stop_req, eof, would_keep,
			     (unsigned int)uxTaskGetStackHighWaterMark2(NULL));
	}
}

void *divoom_tone_lite_create(divoom_tone_lite_notify_fn cb, void *user)
{
	Pdivoom_tone_lite_handle h;

	h = (Pdivoom_tone_lite_handle)divoom_system_malloc(sizeof(*h));
	if (!h)
	{
		return NULL;
	}
	memset(h, 0, sizeof(*h));
	h->notify = cb;
	h->user = user;
	h->vol_0_100 = 100;
	h->jl_tone_vol = 0;
	h->jl_tone_vol_applied = 1;
	h->play_mode = DIVOOM_TONE_LITE_MODE_COLD;
	h->sink_ch = DIVOOM_AUDIO_SINK_CH_TONE;
	return h;
}

#if DIVOOM_SHOW_TF_NAND
void divoom_tone_lite_set_sink_ch(void *handle, divoom_audio_sink_ch_t ch)
{
	Pdivoom_tone_lite_handle h = (Pdivoom_tone_lite_handle)handle;

	if (!h)
		return;
	h->sink_ch = ch;
}

void divoom_tone_lite_set_paused(void *handle, int paused)
{
	Pdivoom_tone_lite_handle h = (Pdivoom_tone_lite_handle)handle;

	if (!h)
		return;
	h->paused = paused ? 1 : 0;
}

int divoom_tone_lite_get_position_sec(void *handle)
{
	Pdivoom_tone_lite_handle h = (Pdivoom_tone_lite_handle)handle;
	unsigned int rate;

	if (!h || !h->mp3_open)
		return 0;
	rate = h->mp3_rate;
	if (rate == 0)
		return 0;
	return (int)(divoom_mp3dec_stream_position(h->mp3) / rate);
}

int divoom_tone_lite_get_duration_sec(void *handle)
{
	Pdivoom_tone_lite_handle h = (Pdivoom_tone_lite_handle)handle;

	if (!h)
		return 0;
	return (int)h->duration_sec;
}
#endif

void divoom_tone_lite_set_play_mode(void *handle, divoom_tone_lite_play_mode_t mode)
{
	Pdivoom_tone_lite_handle h = (Pdivoom_tone_lite_handle)handle;

	if (!h)
	{
		return;
	}
	h->play_mode = (int)mode;
}

void divoom_tone_lite_set_control_hook(void *handle, divoom_tone_lite_control_fn fn, void *user)
{
	Pdivoom_tone_lite_handle h = (Pdivoom_tone_lite_handle)handle;

	if (!h)
	{
		return;
	}
	h->control_hook = fn;
	h->control_hook_user = user;
}

int divoom_tone_lite_play_until(void *handle, const char *path, int loop, unsigned int end_ticks)
{
	Pdivoom_tone_lite_handle h = (Pdivoom_tone_lite_handle)handle;
	int coding;

	if (!h || !path)
	{
		return -1;
	}
	if (!divoom_tone_lite_path_supported(path))
	{
		return -1;
	}
	if (divoom_tone_lite_normalize_path(path, h->src_path, sizeof(h->src_path)) != 0)
	{
		return -1;
	}
	coding = divoom_voice_clip_path_coding_type(path);
	if (coding != DIVOOM_TONE_CODING_MP3 && coding != DIVOOM_TONE_CODING_AAC)
	{
		return -1;
	}
	h->coding = coding;
	h->loop = loop ? 1 : 0;
	h->end_ticks = end_ticks;
	h->stop_req = 0;
	return 0;
}

void divoom_tone_lite_play_run(void *handle)
{
	Pdivoom_tone_lite_handle h = (Pdivoom_tone_lite_handle)handle;

	if (!h || h->src_path[0] == '\0')
	{
		return;
	}
	divoom_tone_lite_play_worker(h);
}

void divoom_tone_lite_set_volume(void *handle, int vol_0_100)
{
	Pdivoom_tone_lite_handle h = (Pdivoom_tone_lite_handle)handle;

	if (!h)
	{
		return;
	}
	if (vol_0_100 < 0)
	{
		vol_0_100 = 0;
	}
	else if (vol_0_100 > 100)
	{
		vol_0_100 = 100;
	}
	h->vol_0_100 = vol_0_100;
	if (h->sink_open)
	{
#if DIVOOM_SHOW_TF_NAND
		if (h->sink_ch != DIVOOM_AUDIO_SINK_CH_MUSIC)
#endif
			divoom_audio_policy_amp_tip_hold(vol_0_100);
		divoom_audio_sink_ch_set_volume(h->sink_ch,
						divoom_tone_lite_digital_vol(h));
	}
}

void divoom_tone_lite_set_jl_tone_vol(void *handle, int vol_0_100)
{
	Pdivoom_tone_lite_handle h = (Pdivoom_tone_lite_handle)handle;

	if (!h)
	{
		return;
	}
	if (vol_0_100 < 0)
	{
		vol_0_100 = 0;
	}
	else if (vol_0_100 > 100)
	{
		vol_0_100 = 100;
	}
	h->jl_tone_vol = vol_0_100;
	h->jl_tone_vol_applied = 0;
}

void divoom_tone_lite_request_stop(void *handle)
{
	Pdivoom_tone_lite_handle h = (Pdivoom_tone_lite_handle)handle;

	if (!h)
	{
		return;
	}
	h->stop_req = 1;
}

void divoom_tone_lite_close_sink_if_open(void *handle)
{
	Pdivoom_tone_lite_handle h = (Pdivoom_tone_lite_handle)handle;

	if (!h || !h->sink_open)
	{
		return;
	}
	divoom_print("tone_lite: close sink if open\n");
	divoom_tone_lite_close_sink(h);
}

int divoom_tone_lite_is_playing(void *handle)
{
	Pdivoom_tone_lite_handle h = (Pdivoom_tone_lite_handle)handle;

	if (!h)
	{
		return 0;
	}
	return h->playing ? 1 : 0;
}

int divoom_tone_lite_sink_is_open(void *handle)
{
	Pdivoom_tone_lite_handle h = (Pdivoom_tone_lite_handle)handle;

	if (!h)
	{
		return 0;
	}
	return h->sink_open ? 1 : 0;
}

int divoom_tone_lite_stop_requested(void *handle)
{
	Pdivoom_tone_lite_handle h = (Pdivoom_tone_lite_handle)handle;

	if (!h)
	{
		return 0;
	}
	return h->stop_req ? 1 : 0;
}

int divoom_tone_lite_get_play_mode(void *handle)
{
	Pdivoom_tone_lite_handle h = (Pdivoom_tone_lite_handle)handle;

	if (!h)
	{
		return DIVOOM_TONE_LITE_MODE_COLD;
	}
	return h->play_mode;
}

void divoom_tone_lite_destroy_abandon(void *handle)
{
	Pdivoom_tone_lite_handle h = (Pdivoom_tone_lite_handle)handle;

	if (!h)
	{
		return;
	}
	h->stop_req = 1;
	divoom_tone_lite_reset_prefill(h);
	divoom_tone_lite_teardown_decode(h);
	divoom_tone_lite_close_sink(h);
	divoom_tone_lite_release_all_scratch(h);
	divoom_system_free(h);
}

#elif DIVOOM_BUILD_SIMULATOR

#include "divoom_tone_lite_decode.h"

int divoom_tone_lite_path_supported(const char *path)
{
	(void)path;
	return 0;
}

void *divoom_tone_lite_create(divoom_tone_lite_notify_fn cb, void *user)
{
	(void)cb;
	(void)user;
	return NULL;
}

#if DIVOOM_SHOW_TF_NAND
void divoom_tone_lite_set_sink_ch(void *handle, divoom_audio_sink_ch_t ch)
{
	(void)handle;
	(void)ch;
}

void divoom_tone_lite_set_paused(void *handle, int paused)
{
	(void)handle;
	(void)paused;
}

int divoom_tone_lite_get_position_sec(void *handle)
{
	(void)handle;
	return 0;
}

int divoom_tone_lite_get_duration_sec(void *handle)
{
	(void)handle;
	return 0;
}
#endif

int divoom_tone_lite_play_until(void *handle, const char *path, int loop, unsigned int end_ticks)
{
	(void)handle;
	(void)path;
	(void)loop;
	(void)end_ticks;
	return -1;
}

void divoom_tone_lite_play_run(void *handle)
{
	(void)handle;
}

void divoom_tone_lite_set_play_mode(void *handle, divoom_tone_lite_play_mode_t mode)
{
	(void)handle;
	(void)mode;
}

void divoom_tone_lite_set_control_hook(void *handle, divoom_tone_lite_control_fn fn, void *user)
{
	(void)handle;
	(void)fn;
	(void)user;
}

void divoom_tone_lite_set_volume(void *handle, int vol_0_100)
{
	(void)handle;
	(void)vol_0_100;
}

void divoom_tone_lite_set_jl_tone_vol(void *handle, int vol_0_100)
{
	(void)handle;
	(void)vol_0_100;
}

void divoom_tone_lite_request_stop(void *handle)
{
	(void)handle;
}

void divoom_tone_lite_close_sink_if_open(void *handle)
{
	(void)handle;
}

int divoom_tone_lite_is_playing(void *handle)
{
	(void)handle;
	return 0;
}

int divoom_tone_lite_sink_is_open(void *handle)
{
	(void)handle;
	return 0;
}

int divoom_tone_lite_stop_requested(void *handle)
{
	(void)handle;
	return 0;
}

int divoom_tone_lite_get_play_mode(void *handle)
{
	(void)handle;
	return DIVOOM_TONE_LITE_MODE_COLD;
}

void divoom_tone_lite_destroy_abandon(void *handle)
{
	(void)handle;
}

#endif

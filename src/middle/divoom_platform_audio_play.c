#include "divoom_build_config.h"

/*
 * LEGACY / NOT LINKED: F101 AstroToo 提示音主路径为 tone_lite + audio_sink。
 * 本文件依赖 eyesee-mpp DEMUX+ADEC，未列入 middle/Makefile；勿在新代码中引用。
 * 历史说明见 docs/F101_提示音与助眠混音音频架构.md
 */

#if DIVOOM_BUILD_RTOS_DEVICE

#include "divoom.h"
#include "divoom_platform_audio_play.h"
#include "divoom_audio_sink.h"

#if !defined(CONFIG_COMPONENTS_AW_EYESEE_MPP) || \
    !defined(CONFIG_COMPONENTS_AW_EYESEE_MPP_DEMUXER) || \
    !defined(CONFIG_COMPONENTS_AW_EYESEE_MPP_ADEC)
#error "divoom_platform_audio_play requires eyesee-mpp DEMUX+ADEC"
#endif

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <mpi_sys.h>
#include <mpi_demux.h>
#include <mpi_adec.h>

#include <aw-tiny-sound-lib/sunxi_tiny_sound_pcm.h>
#include <sound_v2/sound_adf_ksound.h>
#include <sunxi_hal_sound.h>

#ifdef CONFIG_COMPONENTS_SPEEXDSP
#include "speex_resampler.h"
#endif

extern void AwStreamInit(void);
extern void AwParserInit(void);

#define DIVOOM_PLAT_AUDIO_OUT_RATE       48000u
#define DIVOOM_PLAT_AUDIO_OUT_CH         2u
/* playback-cma=8 → buffer_bytes_max≈8192；2ch*16bit=4B/frame → buffer_frames≤2048 */
#define DIVOOM_PLAT_AUDIO_PERIOD_FRAMES  512u
#define DIVOOM_PLAT_AUDIO_BUFFER_FRAMES  2048u
#define DIVOOM_PLAT_AUDIO_CARD_NAME      "soundi2s0"
#define DIVOOM_PLAT_AUDIO_I2S_SETTLE_MS  50
/* GetFrame(40ms) 每次空返回计一次。看门狗不依赖 MPP 的 EOF 事件回调，
 * 避免解码失败/EOF 事件不送达时同步 worker 卡死 local_tone 线程 */
/* GetFrame 短超时(5~10ms)+keepalive：empty_streak 按轮询次数计，非旧 40ms 口径 */
#define DIVOOM_PLAT_AUDIO_STALL_EMPTY    200  /* 出过帧后连续 ~2s 无新帧 → 判定播完 */
#define DIVOOM_PLAT_AUDIO_NOSTART_EMPTY  1000 /* ~5s 仍未解出首帧 → 判定解码失败 */
#define DIVOOM_PLAT_AUDIO_FADE_MS        50u
#define DIVOOM_PLAT_AUDIO_FADE_SAMPLES   ((DIVOOM_PLAT_AUDIO_OUT_RATE * DIVOOM_PLAT_AUDIO_FADE_MS) / 1000u)
#define DIVOOM_PLAT_AUDIO_FADE_INTERLEAVED (DIVOOM_PLAT_AUDIO_FADE_SAMPLES * DIVOOM_PLAT_AUDIO_OUT_CH)
#define DIVOOM_PLAT_AUDIO_CUT_FADE_MS    15u
#define DIVOOM_PLAT_AUDIO_CUT_FADE_INTERLEAVED \
	((DIVOOM_PLAT_AUDIO_OUT_RATE * DIVOOM_PLAT_AUDIO_CUT_FADE_MS / 1000u) * DIVOOM_PLAT_AUDIO_OUT_CH)
#define DIVOOM_PLAT_AUDIO_FLUSH_PERIODS  8u
#define DIVOOM_PLAT_AUDIO_SILENCE_SAMPLES \
	(DIVOOM_PLAT_AUDIO_PERIOD_FRAMES * DIVOOM_PLAT_AUDIO_OUT_CH)

/* keepalive/flush 专用静音帧；仅 local_tone/plat_audio worker 调用，无并发写 */
static int16_t s_plat_audio_silence[DIVOOM_PLAT_AUDIO_SILENCE_SAMPLES];

/* 1=切歌/淡入淡出诊断打印（定位啪声） */
#define DIVOOM_PLAT_AUDIO_DIAG           0

enum {
	DIVOOM_PLAT_AUDIO_EXIT_NONE = 0,
	DIVOOM_PLAT_AUDIO_EXIT_FADE_DONE,
	DIVOOM_PLAT_AUDIO_EXIT_DRAIN_EOF,
	DIVOOM_PLAT_AUDIO_EXIT_DRAIN_STALL,
	DIVOOM_PLAT_AUDIO_EXIT_ERR_PROCESS,
	DIVOOM_PLAT_AUDIO_EXIT_ERR_NOSTART,
	DIVOOM_PLAT_AUDIO_EXIT_ERR_STALL,
	DIVOOM_PLAT_AUDIO_EXIT_DEMUX_FAIL,
	DIVOOM_PLAT_AUDIO_EXIT_CUT_IMMEDIATE,
	DIVOOM_PLAT_AUDIO_EXIT_CUT_FADE,
};

enum {
	DIVOOM_PLAT_AUDIO_UNUSED = 0,
};

#if DIVOOM_PLAT_AUDIO_DIAG
#define PA_DIAG(fmt, ...) \
	divoom_print("PA_DIAG: " fmt, ##__VA_ARGS__)
#else
#define PA_DIAG(fmt, ...) ((void)0)
#endif

static unsigned int s_pa_diag_seq;
static unsigned int s_pa_pcm_close_tick;
static unsigned int s_pa_pcm_gen;
static unsigned int s_pa_handoff_mul; /* 上一轨结束包络，跨曲淡入起点 */

typedef struct _divoom_plat_audio_handle {
	divoom_plat_audio_notify_fn notify;
	void *user;

	volatile int stop_req;
	volatile int playing;
	int loop;
	unsigned int end_ticks;
	int vol_0_100;
	int jl_tone_vol;        /* 首帧后再 ramp 开 JL，避免开流毛刺进功放 */
	int jl_tone_vol_applied;
	int pipeline_teardown;
	char src_uri[256];
	char src_path[256];

	void *lock;

	DEMUX_CHN dmx_chn;
	ADEC_CHN adec_chn;
	DEMUX_CHN_ATTR_S dmx_attr;
	ADEC_CHN_ATTR_S adec_attr;
	DEMUX_MEDIA_INFO_S media_info;
	int mpp_inited;
	volatile int adec_eof;
	volatile int demux_eof;

	int sink_open;
	unsigned int src_rate;
	unsigned int src_ch;

#ifdef CONFIG_COMPONENTS_SPEEXDSP
	SpeexResamplerState *resampler;
	unsigned int rsmp_ch; /* 实例声道数：mono=1 stereo=2，同 rate 可复用 */
#endif
	int16_t *rsmp_buf;
	unsigned int rsmp_cap_samples; /* interleaved samples capacity */
	int16_t *mono_stereo_buf;
	unsigned int mono_stereo_cap;

	unsigned int fade_in_rem;
	unsigned int fade_in_from_mul;  /* 0-100：淡入起点（承接上一轨 handoff） */
	unsigned int fade_out_rem;
	unsigned int fade_out_total;    /* 与 rem 同单位；切歌短淡出≠自然结束淡出 */
	unsigned int fade_out_from_mul; /* 0-100：淡出起点 */
	unsigned int cur_env_mul;       /* 当前包络 0-100，与 PCM 实际增益一致 */
	int fade_out_active;

#if DIVOOM_PLAT_AUDIO_DIAG
	unsigned int diag_seq;
	unsigned int worker_start_tick;
	unsigned int frames_written;
	unsigned int env_glitch_count;
	int exit_reason;
#endif
} divoom_plat_audio_handle, *Pdivoom_plat_audio_handle;

static int divoom_plat_audio_write_pcm(Pdivoom_plat_audio_handle h, const int16_t *interleaved,
				       unsigned int frames, int force);
static void divoom_plat_audio_flush_pcm_silence(Pdivoom_plat_audio_handle h);

/* 切歌复用 PCM 时，DEMUX/ADEC 重建期间须持续喂静音，否则 I2S underrun 破音 */
static void divoom_plat_audio_pcm_keepalive(Pdivoom_plat_audio_handle h, unsigned int periods)
{
	unsigned int i;

	if (!h || !h->sink_open || periods == 0)
		return;
	memset(s_plat_audio_silence, 0, sizeof(s_plat_audio_silence));
	for (i = 0; i < periods; i++)
		divoom_plat_audio_write_pcm(h, s_plat_audio_silence, DIVOOM_PLAT_AUDIO_PERIOD_FRAMES, 1);
}

#if defined(DIVOOM_TIME_FRAME_SPEAKER) && defined(DIVOOM_TIMES_VIBE)
static void divoom_plat_audio_apply_jl_tone_vol_if_needed(Pdivoom_plat_audio_handle h)
{
	if (!h || h->jl_tone_vol_applied || h->jl_tone_vol <= 0)
		return;
	h->jl_tone_vol_applied = 1;
	/* 响度已由 audio_sink 数字域承载，JL 的 I2S 档位固定不动 */
	PA_DIAG("jl tone ramp skipped seq=%u vol=%d tick=%u\n",
		h->diag_seq, h->jl_tone_vol, divoom_system_get_ticks());
}
#endif

static const char *divoom_plat_audio_diag_basename(const char *uri)
{
	const char *p = uri ? uri : "";
	const char *slash = strrchr(p, '/');

	return slash ? slash + 1 : p;
}

static const char *divoom_plat_audio_exit_reason_str(int reason)
{
	switch (reason) {
	case DIVOOM_PLAT_AUDIO_EXIT_FADE_DONE:
		return "fade_done";
	case DIVOOM_PLAT_AUDIO_EXIT_DRAIN_EOF:
		return "drain_eof";
	case DIVOOM_PLAT_AUDIO_EXIT_DRAIN_STALL:
		return "drain_stall";
	case DIVOOM_PLAT_AUDIO_EXIT_ERR_PROCESS:
		return "err_process";
	case DIVOOM_PLAT_AUDIO_EXIT_ERR_NOSTART:
		return "err_nostart";
	case DIVOOM_PLAT_AUDIO_EXIT_ERR_STALL:
		return "err_stall";
	case DIVOOM_PLAT_AUDIO_EXIT_DEMUX_FAIL:
		return "demux_fail";
	case DIVOOM_PLAT_AUDIO_EXIT_CUT_IMMEDIATE:
		return "cut_immediate";
	case DIVOOM_PLAT_AUDIO_EXIT_CUT_FADE:
		return "cut_fade";
	default:
		return "none";
	}
}

static int s_mpp_core_inited;
static int s_mpp_sys_ref;

static void divoom_plat_audio_mpp_core_init_once(void)
{
	if (s_mpp_core_inited)
		return;
	AwStreamInit();
	AwParserInit();
	s_mpp_core_inited = 1;
}

static void divoom_plat_audio_mpp_sys_acquire(Pdivoom_plat_audio_handle h)
{
	MPP_SYS_CONF_S conf;

	if (h->mpp_inited)
		return;
	memset(&conf, 0, sizeof(conf));
	conf.nAlignWidth = 32;
	if (s_mpp_sys_ref == 0) {
		AW_MPI_SYS_SetConf(&conf);
		AW_MPI_SYS_Init();
	}
	s_mpp_sys_ref++;
	h->mpp_inited = 1;
}

static void divoom_plat_audio_mpp_sys_release(Pdivoom_plat_audio_handle h)
{
	if (!h->mpp_inited)
		return;
	h->mpp_inited = 0;
	if (s_mpp_sys_ref > 0)
		s_mpp_sys_ref--;
	if (s_mpp_sys_ref == 0)
		AW_MPI_SYS_Exit();
}

static ERRORTYPE divoom_plat_audio_mpi_cb(void *cookie, MPP_CHN_S *p_chn,
					 MPP_EVENT_TYPE event, void *p_event_data)
{
	Pdivoom_plat_audio_handle h = (Pdivoom_plat_audio_handle)cookie;

	(void)p_event_data;
	if (!h || !p_chn)
		return SUCCESS;

	if (p_chn->mModId == MOD_ID_DEMUX && event == MPP_EVENT_NOTIFY_EOF) {
		h->demux_eof = 1;
		if (h->adec_chn >= 0)
			AW_MPI_ADEC_SetStreamEof(h->adec_chn, 1);
	} else if (p_chn->mModId == MOD_ID_ADEC && event == MPP_EVENT_NOTIFY_EOF) {
		h->adec_eof = 1;
	}
	return SUCCESS;
}

static int divoom_plat_audio_normalize_path(const char *uri, char *out, unsigned int out_sz)
{
	const char *path = uri;

	if (!uri || !out || out_sz < 2)
		return -1;
	if (strstr(uri, "file://") == uri)
		path = uri + 7;
	strncpy(out, path, out_sz - 1);
	out[out_sz - 1] = '\0';
	return out[0] ? 0 : -1;
}

static int divoom_plat_audio_store_uri(Pdivoom_plat_audio_handle h, const char *path)
{
	if (!h || !path || !path[0])
		return -1;
	if (strstr(path, "file://") == path) {
		strncpy(h->src_uri, path, sizeof(h->src_uri) - 1);
	} else {
		snprintf(h->src_uri, sizeof(h->src_uri), "file://%s", path);
	}
	h->src_uri[sizeof(h->src_uri) - 1] = '\0';
	return divoom_plat_audio_normalize_path(h->src_uri, h->src_path, sizeof(h->src_path));
}

static int divoom_plat_audio_open_pcm(Pdivoom_plat_audio_handle h)
{
	unsigned int from_mul;

	if (!h)
		return -1;
	if (h->sink_open)
		return 0;

	if (divoom_audio_sink_start() != 0) {
		divoom_print("plat_audio: audio_sink start fail\n");
		return -1;
	}
	from_mul = s_pa_handoff_mul;
	if (from_mul > 100)
		from_mul = 100;
	divoom_audio_sink_ch_set_volume(DIVOOM_AUDIO_SINK_CH_TONE, (unsigned int)h->vol_0_100);
	if (from_mul == 0)
		divoom_audio_sink_ch_fade_in(DIVOOM_AUDIO_SINK_CH_TONE, 0, 100);
	else
		divoom_audio_sink_ch_fade_in(DIVOOM_AUDIO_SINK_CH_TONE, DIVOOM_PLAT_AUDIO_FADE_MS, from_mul);
	h->sink_open = 1;
	s_pa_pcm_gen++;
	PA_DIAG("pcm open gen=%u tick=%u gap_ms=%u\n",
		s_pa_pcm_gen, divoom_system_get_ticks(),
		s_pa_pcm_close_tick ?
			(divoom_system_get_ticks() - s_pa_pcm_close_tick) : 0U);
	divoom_print("plat_audio: sink open ok ch=TONE 48k/2ch\n");
	return 0;
}

static void divoom_plat_audio_close_pcm(Pdivoom_plat_audio_handle h)
{
	if (!h || !h->sink_open)
		return;
	s_pa_pcm_close_tick = divoom_system_get_ticks();
	PA_DIAG("pcm close gen=%u tick=%u\n", s_pa_pcm_gen, s_pa_pcm_close_tick);
	divoom_audio_sink_ch_fade_out(DIVOOM_AUDIO_SINK_CH_TONE, DIVOOM_PLAT_AUDIO_CUT_FADE_MS);
	(void)divoom_audio_sink_ch_fade_out_wait(DIVOOM_AUDIO_SINK_CH_TONE, 100);
	h->sink_open = 0;
	divoom_audio_sink_ch_disable(DIVOOM_AUDIO_SINK_CH_TONE);
}

static void divoom_plat_audio_teardown_decode(Pdivoom_plat_audio_handle h)
{
	MPP_CHN_S dmx_chn;
	MPP_CHN_S adec_chn;

	if (!h)
		return;

	if (h->dmx_chn >= 0 && h->adec_chn >= 0) {
		dmx_chn.mModId = MOD_ID_DEMUX;
		dmx_chn.mDevId = 0;
		dmx_chn.mChnId = h->dmx_chn;
		adec_chn.mModId = MOD_ID_ADEC;
		adec_chn.mDevId = 0;
		adec_chn.mChnId = h->adec_chn;
		AW_MPI_SYS_UnBind(&dmx_chn, &adec_chn);
	}
	if (h->adec_chn >= 0) {
		AW_MPI_ADEC_DestroyChn(h->adec_chn);
		h->adec_chn = MM_INVALID_CHN;
	}
	if (h->dmx_chn >= 0) {
		AW_MPI_DEMUX_DestroyChn(h->dmx_chn);
		h->dmx_chn = MM_INVALID_CHN;
	}
	/* Speex 实例切歌保留，prepare 同 rate/声道复用；handle destroy 时释放 */
	if (h->rsmp_buf) {
		divoom_system_free(h->rsmp_buf);
		h->rsmp_buf = NULL;
		h->rsmp_cap_samples = 0;
	}
	if (h->mono_stereo_buf) {
		divoom_system_free(h->mono_stereo_buf);
		h->mono_stereo_buf = NULL;
		h->mono_stereo_cap = 0;
	}
}

static int divoom_plat_audio_create_demux(Pdivoom_plat_audio_handle h)
{
	ERRORTYPE ret;
	BOOL ok = FALSE;

	memset(&h->dmx_attr, 0, sizeof(h->dmx_attr));
	h->dmx_attr.mSourceType = SOURCETYPE_URL;
	h->dmx_attr.mSourceUrl = h->src_uri;
	h->dmx_attr.mFd = -1;
	h->dmx_attr.mDemuxDisableTrack = DEMUX_DISABLE_VIDEO_TRACK | DEMUX_DISABLE_SUBTITLE_TRACK;

	h->dmx_chn = 0;
	while (h->dmx_chn < DEMUX_MAX_CHN_NUM) {
		ret = AW_MPI_DEMUX_CreateChn(h->dmx_chn, &h->dmx_attr);
		if (ret == SUCCESS) {
			ok = TRUE;
			break;
		}
		if (ret == ERR_DEMUX_EXIST) {
			h->dmx_chn++;
			continue;
		}
		break;
	}
	if (!ok) {
		h->dmx_chn = MM_INVALID_CHN;
		return -1;
	}
	{
		MPPCallbackInfo cb;

		cb.cookie = (void *)h;
		cb.callback = (MPPCallbackFuncType)&divoom_plat_audio_mpi_cb;
		AW_MPI_DEMUX_RegisterCallback(h->dmx_chn, &cb);
	}
	return 0;
}

static int divoom_plat_audio_create_adec(Pdivoom_plat_audio_handle h)
{
	ERRORTYPE ret;
	BOOL ok = FALSE;
	DEMUX_AUDIO_STREAM_INFO_S *ainfo;

	if (h->media_info.mAudioNum <= 0)
		return -1;

	ainfo = &h->media_info.mAudioStreamInfo[h->media_info.mAudioIndex];
	memset(&h->adec_attr, 0, sizeof(h->adec_attr));
	h->adec_attr.mType = ainfo->mCodecType;
	h->adec_attr.sampleRate = ainfo->mSampleRate;
	h->adec_attr.channels = ainfo->mChannelNum;
	h->adec_attr.bitsPerSample = ainfo->mBitsPerSample;
	h->src_rate = (unsigned int)ainfo->mSampleRate;
	h->src_ch = (unsigned int)((ainfo->mChannelNum > 0) ? ainfo->mChannelNum : 1);

	h->adec_chn = 0;
	while (h->adec_chn < ADEC_MAX_CHN_NUM) {
		ret = AW_MPI_ADEC_CreateChn(h->adec_chn, &h->adec_attr);
		if (ret == SUCCESS) {
			ok = TRUE;
			break;
		}
		if (ret == ERR_ADEC_EXIST) {
			h->adec_chn++;
			continue;
		}
		break;
	}
	if (!ok) {
		h->adec_chn = MM_INVALID_CHN;
		return -1;
	}
	{
		MPPCallbackInfo cb;

		cb.cookie = (void *)h;
		cb.callback = (MPPCallbackFuncType)&divoom_plat_audio_mpi_cb;
		AW_MPI_ADEC_RegisterCallback(h->adec_chn, &cb);
	}
	return 0;
}

#define DIVOOM_PLAT_AUDIO_MIN_SRC_BYTES  1024u

static int divoom_plat_audio_src_file_ok(const char *path)
{
	struct stat st;

	if (!path || path[0] == '\0')
	{
		return 0;
	}
	if (stat(path, &st) != 0 || st.st_size < (off_t)DIVOOM_PLAT_AUDIO_MIN_SRC_BYTES)
	{
		return 0;
	}
	return 1;
}

static void divoom_plat_audio_teardown_pipeline_body(Pdivoom_plat_audio_handle h)
{
	if (!h)
	{
		return;
	}
	divoom_plat_audio_teardown_decode(h);
	divoom_plat_audio_close_pcm(h);
	divoom_plat_audio_mpp_sys_release(h);
}

/* prepare 失败时由已持锁的 play_until 调用，禁止再次 get_lock */
static void divoom_plat_audio_fail_pipeline(Pdivoom_plat_audio_handle h)
{
	if (!h || h->pipeline_teardown)
	{
		return;
	}
	h->pipeline_teardown = 1;
	divoom_plat_audio_teardown_pipeline_body(h);
}

static void divoom_plat_audio_destroy_pipeline(Pdivoom_plat_audio_handle h)
{
	if (!h)
	{
		return;
	}

	divoom_system_get_lock(h->lock);
	if (h->pipeline_teardown)
	{
		divoom_system_free_lock(h->lock);
		return;
	}
	h->pipeline_teardown = 1;
	divoom_system_free_lock(h->lock);

	divoom_plat_audio_teardown_pipeline_body(h);
}

#ifdef CONFIG_COMPONENTS_SPEEXDSP
static unsigned int divoom_plat_audio_rsmp_ch(unsigned int src_ch)
{
	return (src_ch >= 2u) ? DIVOOM_PLAT_AUDIO_OUT_CH : 1u;
}

static void divoom_plat_audio_expand_mono_to_stereo(int16_t *buf, unsigned int frames)
{
	unsigned int i;

	if (!buf || frames == 0)
		return;
	i = frames;
	while (i > 0) {
		int16_t s;

		i--;
		s = buf[i];
		buf[i * 2] = s;
		buf[i * 2 + 1] = s;
	}
}

static int divoom_plat_audio_setup_resampler(Pdivoom_plat_audio_handle h)
{
	unsigned int want_ch;

	if (!h)
		return -1;
	want_ch = divoom_plat_audio_rsmp_ch(h->src_ch);

	if (h->src_rate == DIVOOM_PLAT_AUDIO_OUT_RATE) {
		if (h->resampler) {
			speex_resampler_destroy(h->resampler);
			h->resampler = NULL;
		}
		h->rsmp_ch = 0;
		return 0;
	}
	if (h->resampler && h->rsmp_ch == want_ch) {
		spx_uint32_t in_r = 0;
		spx_uint32_t out_r = 0;

		speex_resampler_get_rate(h->resampler, &in_r, &out_r);
		if (in_r == h->src_rate && out_r == DIVOOM_PLAT_AUDIO_OUT_RATE) {
			speex_resampler_reset_mem(h->resampler);
			return 0;
		}
	}
	if (h->resampler) {
		speex_resampler_destroy(h->resampler);
		h->resampler = NULL;
	}
	{
		int err = 0;

		h->resampler = speex_resampler_init_frac(want_ch,
							h->src_rate,
							DIVOOM_PLAT_AUDIO_OUT_RATE,
							h->src_rate,
							DIVOOM_PLAT_AUDIO_OUT_RATE,
							1, /* Q1：16 抽头；须与 RESAMPLE_FULL_SINC_TABLE 同开 */
							&err);
		if (!h->resampler || err != 0) {
			h->rsmp_ch = 0;
			divoom_print("plat_audio: speex init fail err=%d\n", err);
			return -1;
		}
		h->rsmp_ch = want_ch;
		divoom_print("plat_audio: resample %u -> 48k ch=%u\n", h->src_rate, want_ch);
	}
	return 0;
}
#endif

static int divoom_plat_audio_prepare_pipeline(Pdivoom_plat_audio_handle h)
{
	MPP_CHN_S dmx_chn;
	MPP_CHN_S adec_chn;

	h->pipeline_teardown = 0;
	h->dmx_chn = MM_INVALID_CHN;
	h->adec_chn = MM_INVALID_CHN;

	divoom_plat_audio_mpp_core_init_once();
	divoom_plat_audio_mpp_sys_acquire(h);

	{
		int probe_fd = open(h->src_path, O_RDONLY);

		if (probe_fd < 0) {
			divoom_print("plat_audio: open fail %s\n", h->src_path);
			divoom_plat_audio_mpp_sys_release(h);
			return -1;
		}
		close(probe_fd);
	}

	if (!divoom_plat_audio_src_file_ok(h->src_path)) {
		divoom_print("plat_audio: src too small or missing %s\n", h->src_path);
		divoom_plat_audio_mpp_sys_release(h);
		return -1;
	}

	/* 切歌复用 PCM + DEMUX 解析期间 keepalive，避免 gap_ms=0 关开啪声与 xrun */
	if (!h->sink_open) {
		if (divoom_plat_audio_open_pcm(h) != 0)
			goto fail;
		PA_DIAG("pcm open before demux gen=%u tick=%u\n",
			s_pa_pcm_gen, divoom_system_get_ticks());
	} else {
		h->cur_env_mul = 0;
		divoom_plat_audio_flush_pcm_silence(h);
		PA_DIAG("pcm reuse gen=%u tick=%u\n",
			s_pa_pcm_gen, divoom_system_get_ticks());
	}
	divoom_plat_audio_pcm_keepalive(h, 2u);

	if (divoom_plat_audio_create_demux(h) != 0) {
		divoom_print("plat_audio: create demux fail\n");
		goto fail;
	}
	divoom_plat_audio_pcm_keepalive(h, 2u);
	if (AW_MPI_DEMUX_GetMediaInfo(h->dmx_chn, &h->media_info) != SUCCESS) {
		divoom_print("plat_audio: get media info fail\n");
		goto fail;
	}
	divoom_plat_audio_pcm_keepalive(h, 2u);
	if (h->media_info.mAudioNum <= 0) {
		divoom_print("plat_audio: no audio track\n");
		goto fail;
	}
	if (divoom_plat_audio_create_adec(h) != 0) {
		divoom_print("plat_audio: create adec fail\n");
		goto fail;
	}

	dmx_chn.mModId = MOD_ID_DEMUX;
	dmx_chn.mDevId = 0;
	dmx_chn.mChnId = h->dmx_chn;
	adec_chn.mModId = MOD_ID_ADEC;
	adec_chn.mDevId = 0;
	adec_chn.mChnId = h->adec_chn;
	if (AW_MPI_SYS_Bind(&dmx_chn, &adec_chn) != SUCCESS) {
		divoom_print("plat_audio: bind demux-adec fail\n");
		goto fail;
	}

#ifdef CONFIG_COMPONENTS_SPEEXDSP
	if (divoom_plat_audio_setup_resampler(h) != 0)
		goto fail;
#else
	if (h->src_rate != DIVOOM_PLAT_AUDIO_OUT_RATE) {
		divoom_print("plat_audio: need SPEEXDSP for rate %u\n", h->src_rate);
		goto fail;
	}
#endif

	divoom_print("plat_audio: prepare ok %s rate=%u ch=%u\n",
		     h->src_uri, h->src_rate, h->src_ch);
	return 0;

fail:
	divoom_plat_audio_fail_pipeline(h);
	return -1;
}

static void divoom_plat_audio_reset_fade_in(Pdivoom_plat_audio_handle h)
{
	unsigned int from_mul;

	if (!h)
		return;
	from_mul = s_pa_handoff_mul;
	if (from_mul > 100)
		from_mul = 100;
	h->fade_in_from_mul = from_mul;
	h->fade_in_rem = DIVOOM_PLAT_AUDIO_FADE_INTERLEAVED;
	if (h->sink_open)
		divoom_audio_sink_ch_fade_in(DIVOOM_AUDIO_SINK_CH_TONE, DIVOOM_PLAT_AUDIO_FADE_MS, from_mul);
}

static void divoom_plat_audio_reset_fade(Pdivoom_plat_audio_handle h)
{
	if (!h)
		return;
	divoom_plat_audio_reset_fade_in(h);
	h->fade_out_rem = 0;
	h->fade_out_total = DIVOOM_PLAT_AUDIO_FADE_INTERLEAVED;
	h->fade_out_from_mul = 100;
	h->fade_out_active = 0;
	h->cur_env_mul = h->fade_in_from_mul;
#if DIVOOM_PLAT_AUDIO_DIAG
	h->env_glitch_count = 0;
	h->exit_reason = DIVOOM_PLAT_AUDIO_EXIT_NONE;
#endif
}

static void divoom_plat_audio_note_env_mul(Pdivoom_plat_audio_handle h, unsigned int fade_mul,
					   const char *where)
{
	unsigned int prev;

	if (!h)
		return;
	prev = h->cur_env_mul;
	if (fade_mul > 100)
		fade_mul = 100;
#if DIVOOM_PLAT_AUDIO_DIAG
	if (prev <= 100) {
		int delta = (int)fade_mul - (int)prev;

		if (delta < 0)
			delta = -delta;
		if (delta > 20) {
			h->env_glitch_count++;
			PA_DIAG("env jump seq=%u %s %s prev=%u now=%u d=%d tick=%u\n",
				h->diag_seq, divoom_plat_audio_diag_basename(h->src_uri),
				where ? where : "?", prev, fade_mul, delta,
				divoom_system_get_ticks());
		}
	}
#endif
	h->cur_env_mul = fade_mul;
}

static void divoom_plat_audio_start_cut_fade_out(Pdivoom_plat_audio_handle h)
{
	if (!h || h->fade_out_active)
		return;
	h->fade_out_from_mul = h->cur_env_mul;
	if (h->fade_out_from_mul > 100)
		h->fade_out_from_mul = 100;
	h->fade_in_rem = 0;
	h->fade_out_active = 1;
	h->fade_out_total = DIVOOM_PLAT_AUDIO_CUT_FADE_INTERLEAVED;
	h->fade_out_rem = h->fade_out_total;
	if (h->sink_open)
		divoom_audio_sink_ch_fade_out(DIVOOM_AUDIO_SINK_CH_TONE, DIVOOM_PLAT_AUDIO_CUT_FADE_MS);
	PA_DIAG("cut_fade start seq=%u %s from=%u total=%u cur=%u tick=%u\n",
		h->diag_seq, divoom_plat_audio_diag_basename(h->src_uri),
		h->fade_out_from_mul, h->fade_out_total, h->cur_env_mul,
		divoom_system_get_ticks());
}

static void divoom_plat_audio_start_fade_out(Pdivoom_plat_audio_handle h)
{
	unsigned int fi_rem;
	unsigned int cur;

	if (!h || h->fade_out_active)
		return;
	fi_rem = h->fade_in_rem;
	cur = h->cur_env_mul;
	if (cur > 100)
		cur = 100;
	/* 以实际播放包络为淡出起点，避免跨轨残留 cur=0 而 from=100 上冲 */
	h->fade_out_from_mul = cur;
	h->fade_in_rem = 0;
	h->fade_out_active = 1;
	h->fade_out_total = DIVOOM_PLAT_AUDIO_FADE_INTERLEAVED;
	h->fade_out_rem = h->fade_out_total;
	if (h->sink_open)
		divoom_audio_sink_ch_fade_out(DIVOOM_AUDIO_SINK_CH_TONE, DIVOOM_PLAT_AUDIO_FADE_MS);
	PA_DIAG("fade_out start seq=%u %s from=%u fi_was=%u total=%u cur=%u stop=%d tick=%u\n",
		h->diag_seq, divoom_plat_audio_diag_basename(h->src_uri),
		h->fade_out_from_mul, fi_rem, h->fade_out_total, h->cur_env_mul,
		h->stop_req, divoom_system_get_ticks());
}

static int divoom_plat_audio_fade_out_done(Pdivoom_plat_audio_handle h)
{
	unsigned int env;

	if (!h || !h->fade_out_active)
		return 0;
	env = divoom_audio_sink_ch_env_mul(DIVOOM_AUDIO_SINK_CH_TONE);
	h->cur_env_mul = env;
	return env == 0;
}

static void divoom_plat_audio_apply_gain_fade(Pdivoom_plat_audio_handle h, int16_t *samples,
					      unsigned int n_interleaved)
{
	(void)samples;
	(void)n_interleaved;
	if (!h)
		return;
	h->cur_env_mul = divoom_audio_sink_ch_env_mul(DIVOOM_AUDIO_SINK_CH_TONE);
}

static void divoom_plat_audio_flush_pcm_silence(Pdivoom_plat_audio_handle h)
{
	unsigned int i;

	if (!h || !h->sink_open)
		return;
	memset(s_plat_audio_silence, 0, sizeof(s_plat_audio_silence));
	h->cur_env_mul = 0;
	divoom_audio_sink_ch_mute_drop(DIVOOM_AUDIO_SINK_CH_TONE);
	/* 冲刷 I2S/DAC 环形缓冲，包络保持 0 */
	for (i = 0; i < DIVOOM_PLAT_AUDIO_FLUSH_PERIODS; i++)
		divoom_plat_audio_write_pcm(h, s_plat_audio_silence, DIVOOM_PLAT_AUDIO_PERIOD_FRAMES, 1);
	PA_DIAG("pcm flush silence seq=%u periods=%u cur=%u tick=%u\n",
		h->diag_seq, DIVOOM_PLAT_AUDIO_FLUSH_PERIODS, h->cur_env_mul,
		divoom_system_get_ticks());
}

static void divoom_plat_audio_drain_fade_out_pcm(Pdivoom_plat_audio_handle h)
{
	if (!h || !h->fade_out_active)
		return;
	(void)divoom_audio_sink_ch_fade_out_wait(DIVOOM_AUDIO_SINK_CH_TONE, 200);
	h->fade_out_rem = 0;
	h->fade_out_active = 0;
	h->cur_env_mul = 0;
}

static int divoom_plat_audio_ensure_rsmp_buf(Pdivoom_plat_audio_handle h, unsigned int interleaved_samples)
{
	if (interleaved_samples <= h->rsmp_cap_samples)
		return 0;
	if (h->rsmp_buf)
		divoom_system_free(h->rsmp_buf);
	h->rsmp_buf = (int16_t *)divoom_system_malloc(interleaved_samples * sizeof(int16_t));
	if (!h->rsmp_buf) {
		h->rsmp_cap_samples = 0;
		return -1;
	}
	h->rsmp_cap_samples = interleaved_samples;
	return 0;
}

static int divoom_plat_audio_ensure_mono_stereo(Pdivoom_plat_audio_handle h, unsigned int frames)
{
	unsigned int need = frames * 2;

	if (need <= h->mono_stereo_cap)
		return 0;
	if (h->mono_stereo_buf)
		divoom_system_free(h->mono_stereo_buf);
	h->mono_stereo_buf = (int16_t *)divoom_system_malloc(need * sizeof(int16_t));
	if (!h->mono_stereo_buf) {
		h->mono_stereo_cap = 0;
		return -1;
	}
	h->mono_stereo_cap = need;
	return 0;
}

static int divoom_plat_audio_write_pcm(Pdivoom_plat_audio_handle h, const int16_t *interleaved,
				      unsigned int frames, int force)
{
	if (!h || !h->sink_open || !interleaved || frames == 0)
		return -1;
	if (!force && h->stop_req && !h->fade_out_active)
		return 0;
	if (divoom_audio_sink_push_all(DIVOOM_AUDIO_SINK_CH_TONE, interleaved, frames) != 0)
		return -1;
#if DIVOOM_PLAT_AUDIO_DIAG
	h->frames_written += frames;
#endif
	return 0;
}

static int divoom_plat_audio_process_frame(Pdivoom_plat_audio_handle h, AUDIO_FRAME_S *frm)
{
	unsigned int ch;
	unsigned int total_bytes;
	unsigned int in_frames;
	const int16_t *in_ptr;
	int16_t *stereo_in;
	unsigned int i;

	if (!frm || !frm->mpAddr || frm->mLen == 0)
		return 0;

	ch = (frm->mSoundmode == AUDIO_SOUND_MODE_STEREO) ? 2u : 1u;

	/* ADEC mLen is total interleaved PCM bytes (decoder mSize). */
	total_bytes = frm->mLen;
	if (total_bytes < 2u * ch)
		return 0;
	in_frames = total_bytes / (2u * ch);
	in_ptr = (const int16_t *)frm->mpAddr;

#ifdef CONFIG_COMPONENTS_SPEEXDSP
	if (h->src_rate != DIVOOM_PLAT_AUDIO_OUT_RATE) {
		unsigned int want_ch = divoom_plat_audio_rsmp_ch(ch);

		if (!h->resampler || h->rsmp_ch != want_ch) {
			h->src_ch = ch;
			if (divoom_plat_audio_setup_resampler(h) != 0)
				return -1;
		}
	}
	if (h->resampler) {
		spx_uint32_t in_len = in_frames;
		spx_uint32_t out_len;
		unsigned int out_cap;
		int ret;

		out_cap = (unsigned int)(((unsigned long long)in_frames * DIVOOM_PLAT_AUDIO_OUT_RATE) /
					 (h->src_rate ? h->src_rate : DIVOOM_PLAT_AUDIO_OUT_RATE)) + 64u;
		if (divoom_plat_audio_ensure_rsmp_buf(h, out_cap * DIVOOM_PLAT_AUDIO_OUT_CH) != 0)
			return -1;
		out_len = out_cap;
		if (ch == 1u)
			ret = speex_resampler_process_int(h->resampler, 0, in_ptr, &in_len,
							 h->rsmp_buf, &out_len);
		else
			ret = speex_resampler_process_interleaved_int(h->resampler, in_ptr, &in_len,
								     h->rsmp_buf, &out_len);
		if (ret != 0) {
			divoom_print("plat_audio: resample fail %d\n", ret);
			return -1;
		}
		if (ch == 1u)
			divoom_plat_audio_expand_mono_to_stereo(h->rsmp_buf, out_len);
		divoom_plat_audio_apply_gain_fade(h, h->rsmp_buf, out_len * DIVOOM_PLAT_AUDIO_OUT_CH);
		if (divoom_plat_audio_write_pcm(h, h->rsmp_buf, out_len, 0) != 0)
			return -1;
		divoom_plat_audio_apply_jl_tone_vol_if_needed(h);
		return 0;
	}
#endif
	if (ch == 1) {
		if (divoom_plat_audio_ensure_mono_stereo(h, in_frames) != 0)
			return -1;
		for (i = 0; i < in_frames; i++) {
			h->mono_stereo_buf[i * 2] = in_ptr[i];
			h->mono_stereo_buf[i * 2 + 1] = in_ptr[i];
		}
		stereo_in = h->mono_stereo_buf;
	} else {
		stereo_in = (int16_t *)(void *)in_ptr;
	}
	if (divoom_plat_audio_ensure_rsmp_buf(h, in_frames * DIVOOM_PLAT_AUDIO_OUT_CH) != 0)
		return -1;
	memcpy(h->rsmp_buf, stereo_in, in_frames * DIVOOM_PLAT_AUDIO_OUT_CH * sizeof(int16_t));
	divoom_plat_audio_apply_gain_fade(h, h->rsmp_buf, in_frames * DIVOOM_PLAT_AUDIO_OUT_CH);
	if (divoom_plat_audio_write_pcm(h, h->rsmp_buf, in_frames, 0) != 0)
		return -1;
	divoom_plat_audio_apply_jl_tone_vol_if_needed(h);
	return 0;
}

static int divoom_plat_audio_restart_loop(Pdivoom_plat_audio_handle h)
{
	AUDIO_FRAME_S drain_frm;
	int drain_guard = 0;

	/* loop 重播：与 stop_tone / request_stop 无关，勿在此处做停播逻辑 */
	h->adec_eof = 0;
	h->demux_eof = 0;

	/* GetFrame 取 PCM（non-tunnel）；seek 前须归还残留帧（mpi_adec.h） */
	if (h->adec_chn >= 0) {
		while (drain_guard++ < 64) {
			memset(&drain_frm, 0, sizeof(drain_frm));
			if (AW_MPI_ADEC_GetFrame(h->adec_chn, &drain_frm, 0) != SUCCESS)
				break;
			AW_MPI_ADEC_ReleaseFrame(h->adec_chn, &drain_frm);
		}
		AW_MPI_ADEC_Pause(h->adec_chn);
	}
	/* mpi_demux: Seek 仅 Idle/Pause；EOF 后 demux 仍 Executing，须 Stop→Seek→Start */
	if (h->dmx_chn >= 0) {
		AW_MPI_DEMUX_Stop(h->dmx_chn);
		AW_MPI_DEMUX_Seek(h->dmx_chn, 0);
	}
	/* mpi_adec tunnel seek: Pause→SetStreamEof(0)→Seek→StartRecvStream */
	if (h->adec_chn >= 0) {
		AW_MPI_ADEC_SetStreamEof(h->adec_chn, 0);
		AW_MPI_ADEC_Seek(h->adec_chn);
		AW_MPI_ADEC_StartRecvStream(h->adec_chn);
	}
	if (h->dmx_chn >= 0)
		AW_MPI_DEMUX_Start(h->dmx_chn);
#ifdef CONFIG_COMPONENTS_SPEEXDSP
	if (h->resampler)
		speex_resampler_reset_mem(h->resampler);
#endif
	divoom_plat_audio_reset_fade_in(h);
	h->fade_out_rem = 0;
	h->fade_out_total = DIVOOM_PLAT_AUDIO_FADE_INTERLEAVED;
	h->fade_out_from_mul = 100;
	h->fade_out_active = 0;
	h->fade_in_rem = 0;
	h->fade_in_from_mul = 100;
	h->cur_env_mul = 100;
	if (h->sink_open)
		divoom_audio_sink_ch_fade_in(DIVOOM_AUDIO_SINK_CH_TONE, 0, 100);
	return 0;
}

static void *divoom_plat_audio_worker(void *arg)
{
	Pdivoom_plat_audio_handle h = (Pdivoom_plat_audio_handle)arg;
	AUDIO_FRAME_S frm;
	ERRORTYPE ret;
	int notify = DIVOOM_PLAT_AUDIO_NOTIFY_COMPLETE;
	int empty_streak = 0;
	int got_any = 0;

#if DIVOOM_PLAT_AUDIO_DIAG
	h->worker_start_tick = divoom_system_get_ticks();
	h->frames_written = 0;
	h->exit_reason = DIVOOM_PLAT_AUDIO_EXIT_NONE;
#endif
	PA_DIAG("worker start seq=%u %s loop=%d stop=%d tick=%u pcm_gen=%u handoff=%u cur=%u\n",
		h->diag_seq, divoom_plat_audio_diag_basename(h->src_uri), h->loop,
		h->stop_req, h->worker_start_tick, s_pa_pcm_gen, s_pa_handoff_mul,
		h->cur_env_mul);
	divoom_print("plat_audio: worker start\n");

	/* 预填 < start_threshold，DMA 尚未启动；随后空窗 keepalive 接上 */
	if (h->sink_open)
		divoom_plat_audio_pcm_keepalive(h, 3u);

	AW_MPI_ADEC_StartRecvStream(h->adec_chn);
	if (AW_MPI_DEMUX_Start(h->dmx_chn) != SUCCESS) {
		divoom_print("plat_audio: demux start fail\n");
		notify = DIVOOM_PLAT_AUDIO_NOTIFY_ERROR;
#if DIVOOM_PLAT_AUDIO_DIAG
		h->exit_reason = DIVOOM_PLAT_AUDIO_EXIT_DEMUX_FAIL;
#endif
		goto done;
	}

	while (1) {
		int get_timeout_ms;

		/* 切歌：JL 侧已 mute（local_music_ctrl）。不再等 ADEC 帧做软件淡出——
		 * DMA 环缓里满幅样本消不掉，且 22k→48k 重采样写 PCM 易 xrun/-32 啪声。 */
		if (h->stop_req) {
			if (!h->fade_out_active)
				divoom_plat_audio_start_cut_fade_out(h);
#if DIVOOM_PLAT_AUDIO_DIAG
			h->exit_reason = DIVOOM_PLAT_AUDIO_EXIT_CUT_FADE;
#endif
			divoom_plat_audio_drain_fade_out_pcm(h);
			if (h->sink_open)
				divoom_plat_audio_flush_pcm_silence(h);
			break;
		}

		/* PCM buffer≈42ms：GetFrame 不可长时间阻塞，否则 I2S 必 underrun。
		 * 首帧前用短超时 + 静音 keepalive；已出帧后亦勿超过 ~1 period。 */
		get_timeout_ms = got_any ? 10 : 5;
		memset(&frm, 0, sizeof(frm));
		ret = AW_MPI_ADEC_GetFrame(h->adec_chn, &frm, get_timeout_ms);
		if (ret == SUCCESS) {
			empty_streak = 0;
			got_any = 1;
			if (divoom_plat_audio_process_frame(h, &frm) != 0) {
				AW_MPI_ADEC_ReleaseFrame(h->adec_chn, &frm);
				notify = DIVOOM_PLAT_AUDIO_NOTIFY_ERROR;
#if DIVOOM_PLAT_AUDIO_DIAG
				h->exit_reason = DIVOOM_PLAT_AUDIO_EXIT_ERR_PROCESS;
#endif
				break;
			}
			AW_MPI_ADEC_ReleaseFrame(h->adec_chn, &frm);
			if (divoom_plat_audio_fade_out_done(h)) {
#if DIVOOM_PLAT_AUDIO_DIAG
				if (h->stop_req)
					h->exit_reason = DIVOOM_PLAT_AUDIO_EXIT_CUT_FADE;
				else
					h->exit_reason = DIVOOM_PLAT_AUDIO_EXIT_FADE_DONE;
#endif
				break;
			}
			continue;
		}
		if (ret == ERR_ADEC_BUF_EMPTY) {
			empty_streak++;
			/* DEMUX/ADEC 尚未吐帧或短暂空窗：继续喂静音，撑住 I2S */
			if (h->sink_open && !h->fade_out_active)
				divoom_plat_audio_pcm_keepalive(h, got_any ? 1u : 2u);
			/* 快路径：DEMUX/ADEC EOF 事件 + 缓冲抽空 → 淡出后收尾 */
			if ((h->adec_eof || h->demux_eof) && empty_streak > 20) {
				if (h->loop && !h->stop_req) {
					if (h->end_ticks != 0 && divoom_system_get_ticks() >= h->end_ticks)
						break;
					divoom_plat_audio_restart_loop(h);
					empty_streak = 0;
					continue;
				}
				if (!got_any)
					notify = DIVOOM_PLAT_AUDIO_NOTIFY_ERROR;
				if (got_any && !h->fade_out_active)
					divoom_plat_audio_start_fade_out(h);
			} else if (got_any && empty_streak > DIVOOM_PLAT_AUDIO_STALL_EMPTY) {
				if (h->loop && !h->stop_req) {
					if (h->end_ticks != 0 &&
					    divoom_system_get_ticks() >= h->end_ticks)
						break;
					divoom_plat_audio_restart_loop(h);
					empty_streak = 0;
					continue;
				}
				divoom_print("plat_audio: drained, exit\n");
				if (!h->fade_out_active)
					divoom_plat_audio_start_fade_out(h);
			} else if (!got_any && empty_streak > DIVOOM_PLAT_AUDIO_NOSTART_EMPTY) {
				divoom_print("plat_audio: no frame decoded, exit\n");
				notify = DIVOOM_PLAT_AUDIO_NOTIFY_ERROR;
#if DIVOOM_PLAT_AUDIO_DIAG
				h->exit_reason = DIVOOM_PLAT_AUDIO_EXIT_ERR_NOSTART;
#endif
				break;
			}

			if (h->fade_out_active) {
#if DIVOOM_PLAT_AUDIO_DIAG
				if (h->exit_reason == DIVOOM_PLAT_AUDIO_EXIT_NONE)
					h->exit_reason = (h->adec_eof || h->demux_eof) ?
						DIVOOM_PLAT_AUDIO_EXIT_DRAIN_EOF :
						DIVOOM_PLAT_AUDIO_EXIT_DRAIN_STALL;
#endif
				divoom_plat_audio_drain_fade_out_pcm(h);
				break;
			}
			continue;
		}
		/* other errors: keep trying briefly unless eof */
		if (h->adec_eof || h->demux_eof) {
			if (h->loop && !h->stop_req) {
				if (h->end_ticks != 0 &&
				    divoom_system_get_ticks() >= h->end_ticks)
					break;
				divoom_plat_audio_restart_loop(h);
				empty_streak = 0;
				continue;
			}
			if (got_any && !h->fade_out_active)
				divoom_plat_audio_start_fade_out(h);
			if (h->fade_out_active) {
#if DIVOOM_PLAT_AUDIO_DIAG
				if (h->exit_reason == DIVOOM_PLAT_AUDIO_EXIT_NONE)
					h->exit_reason = DIVOOM_PLAT_AUDIO_EXIT_DRAIN_EOF;
#endif
				divoom_plat_audio_drain_fade_out_pcm(h);
				break;
			}
			break;
		}
		empty_streak++;
		if (h->sink_open && !h->fade_out_active)
			divoom_plat_audio_pcm_keepalive(h, 1u);
		if (empty_streak > 400) {
			notify = DIVOOM_PLAT_AUDIO_NOTIFY_ERROR;
#if DIVOOM_PLAT_AUDIO_DIAG
			h->exit_reason = DIVOOM_PLAT_AUDIO_EXIT_ERR_STALL;
#endif
			break;
		}
	}

	AW_MPI_DEMUX_Stop(h->dmx_chn);
	AW_MPI_ADEC_StopRecvStream(h->adec_chn);

done:
	/* 强停(stop_req)切歌时，ADEC 输出队列里可能仍堆着未取走的已解码帧。
	 * DestroyChn 的 Idle->Loaded 转换会阻塞等待所有输出帧节点归位
	 * (mIdleOutFrameList 满)，若不清空 mReadyOutFrameList 将永久死锁，
	 * 进而卡住 local_tone 线程，导致后续提示音全部无声。
	 * 这里主动把剩余 ready 帧取出并归还，确保节点复位后再拆管线。 */
	if (h->adec_chn >= 0) {
		AUDIO_FRAME_S drain_frm;
		int drain_guard = 0;
		unsigned int drain_cnt = 0;

		while (drain_guard++ < 2000) {
			memset(&drain_frm, 0, sizeof(drain_frm));
			if (AW_MPI_ADEC_GetFrame(h->adec_chn, &drain_frm, 0) != SUCCESS)
				break;
			AW_MPI_ADEC_ReleaseFrame(h->adec_chn, &drain_frm);
			drain_cnt++;
		}
		if (drain_cnt > 0)
			PA_DIAG("adec drain seq=%u leftover_frames=%u\n", h->diag_seq, drain_cnt);
	}
	if (h->sink_open)
		divoom_plat_audio_flush_pcm_silence(h);
	/* 切歌(stop_req)：保留 PCM，下首 prepare 复用；自然结束才关流 */
	divoom_plat_audio_teardown_decode(h);
	if (!h->stop_req)
		divoom_plat_audio_close_pcm(h);
	h->playing = 0;
	s_pa_handoff_mul = h->cur_env_mul;
	if (h->fade_out_active || h->stop_req)
		s_pa_handoff_mul = 0;
	if (!h->stop_req && h->notify)
		h->notify(h->user, notify);
#if DIVOOM_PLAT_AUDIO_DIAG
	{
		unsigned int now = divoom_system_get_ticks();
		unsigned int dur = now - h->worker_start_tick;

		PA_DIAG("worker exit seq=%u %s reason=%s stop=%d notify_cb=%d/%d "
			"dur_tick=%u frames=%u fade_from=%u cur=%u handoff=%u glitches=%u pcm_keep=%d\n",
			h->diag_seq, divoom_plat_audio_diag_basename(h->src_uri),
			divoom_plat_audio_exit_reason_str(h->exit_reason),
			h->stop_req, notify, (!h->stop_req && h->notify) ? 1 : 0,
			dur, h->frames_written, h->fade_out_from_mul, h->cur_env_mul,
			s_pa_handoff_mul, h->env_glitch_count, h->sink_open ? 1 : 0);
	}
#endif
	divoom_print("plat_audio: worker exit notify=%d stop=%d\n", notify, h->stop_req);
	return NULL;
}

/* 同步模型：worker 在 local_tone 线程内跑，返回即已退出。
 * 此处仅做防御性收尾（正常情况 playing 已为 0）。 */
static int divoom_plat_audio_wait_idle(Pdivoom_plat_audio_handle h)
{
	int guard;

	if (!h)
		return 0;
	if (!h->playing) {
		/* 切歌路径：worker 已退出，仍给 I2S 留一点排空时间 */
		if (h->stop_req) {
			PA_DIAG("wait_idle cut settle seq=%u tick=%u\n",
				h->diag_seq, divoom_system_get_ticks());
			divoom_system_delay_ms(DIVOOM_PLAT_AUDIO_I2S_SETTLE_MS / 5);
		}
		return 0;
	}

	PA_DIAG("wait_idle blocking seq=%u %s tick=%u\n",
		h->diag_seq, divoom_plat_audio_diag_basename(h->src_uri),
		divoom_system_get_ticks());
	h->stop_req = 1;
	{
		guard = 0;
		while (h->playing && guard < 400) {
			divoom_system_delay_ms(5);
			guard++;
		}
		if (h->playing) {
			divoom_print("plat_audio: wait_idle timeout\n");
			return -1;
		}
	}
	divoom_system_delay_ms(DIVOOM_PLAT_AUDIO_I2S_SETTLE_MS);
	h->stop_req = 0;
	return 0;
}

void *divoom_plat_audio_create(divoom_plat_audio_notify_fn cb, void *user)
{
	Pdivoom_plat_audio_handle h;

	h = (Pdivoom_plat_audio_handle)divoom_system_malloc(sizeof(*h));
	if (!h)
		return NULL;
	memset(h, 0, sizeof(*h));
	h->notify = cb;
	h->user = user;
	h->vol_0_100 = 100;
	h->jl_tone_vol = 0;
	h->jl_tone_vol_applied = 1;
	h->dmx_chn = MM_INVALID_CHN;
	h->adec_chn = MM_INVALID_CHN;
	h->lock = divoom_system_create_lock();
	h->cur_env_mul = 0;
	return h;
}

void divoom_plat_audio_set_volume(void *handle, int vol_0_100)
{
	Pdivoom_plat_audio_handle h = (Pdivoom_plat_audio_handle)handle;

	if (!h)
		return;
	if (vol_0_100 < 0)
		vol_0_100 = 0;
	else if (vol_0_100 > 100)
		vol_0_100 = 100;
	h->vol_0_100 = vol_0_100;
	if (h->sink_open)
		divoom_audio_sink_ch_set_volume(DIVOOM_AUDIO_SINK_CH_TONE, (unsigned int)vol_0_100);
}

void divoom_plat_audio_set_jl_tone_vol(void *handle, int vol_0_100)
{
	Pdivoom_plat_audio_handle h = (Pdivoom_plat_audio_handle)handle;

	if (!h)
		return;
	if (vol_0_100 < 0)
		vol_0_100 = 0;
	else if (vol_0_100 > 100)
		vol_0_100 = 100;
	h->jl_tone_vol = vol_0_100;
	h->jl_tone_vol_applied = 0;
}

int divoom_plat_audio_is_playing(void *handle)
{
	Pdivoom_plat_audio_handle h = (Pdivoom_plat_audio_handle)handle;

	if (!h)
		return 0;
	return h->playing ? 1 : 0;
}

void divoom_plat_audio_request_stop(void *handle)
{
	Pdivoom_plat_audio_handle h = (Pdivoom_plat_audio_handle)handle;

	if (!h)
		return;
	if (!h->stop_req) {
		PA_DIAG("request_stop seq=%u %s playing=%d fade_out=%d cur=%u tick=%u\n",
			h->diag_seq, divoom_plat_audio_diag_basename(h->src_uri),
			h->playing, h->fade_out_active, h->cur_env_mul,
			divoom_system_get_ticks());
	}
	h->stop_req = 1;
}

void divoom_plat_audio_release_pcm_for_external(void *handle)
{
	Pdivoom_plat_audio_handle h = (Pdivoom_plat_audio_handle)handle;
	int guard;

	if (!h)
		return;

	h->stop_req = 1;
	guard = 0;
	while (h->playing && guard < 400) {
		divoom_system_delay_ms(5);
		guard++;
	}
	if (h->sink_open) {
		divoom_audio_sink_ch_fade_out(DIVOOM_AUDIO_SINK_CH_TONE, DIVOOM_PLAT_AUDIO_CUT_FADE_MS);
		(void)divoom_audio_sink_ch_fade_out_wait(DIVOOM_AUDIO_SINK_CH_TONE, 200);
		h->sink_open = 0;
		divoom_audio_sink_ch_disable(DIVOOM_AUDIO_SINK_CH_TONE);
	}
	h->stop_req = 0;
}

void divoom_plat_audio_stop(void *handle)
{
	Pdivoom_plat_audio_handle h = (Pdivoom_plat_audio_handle)handle;

	if (!h)
		return;
	divoom_plat_audio_wait_idle(h);
}

int divoom_plat_audio_play(void *handle, const char *path, int loop)
{
	return divoom_plat_audio_play_until(handle, path, loop, 0);
}

int divoom_plat_audio_play_until(void *handle, const char *path, int loop, unsigned int end_ticks)
{
	Pdivoom_plat_audio_handle h = (Pdivoom_plat_audio_handle)handle;
	int was_cut = 0;
	unsigned int t0;

	if (!h || !path)
		return -1;

	was_cut = h->stop_req;
	t0 = divoom_system_get_ticks();
	if (divoom_plat_audio_wait_idle(h) != 0) {
		divoom_print("plat_audio: play_until wait_idle fail\n");
		return -1;
	}

#if DIVOOM_PLAT_AUDIO_DIAG
	h->diag_seq = ++s_pa_diag_seq;
	PA_DIAG("play_until #%u %s loop=%d was_cut=%d wait_ms=%u pcm=%d handoff=%u tick=%u\n",
		h->diag_seq, divoom_plat_audio_diag_basename(path), loop ? 1 : 0,
		was_cut, divoom_system_get_ticks() - t0, h->sink_open ? 1 : 0,
		s_pa_handoff_mul, divoom_system_get_ticks());
#endif

	divoom_system_get_lock(h->lock);
	if (divoom_plat_audio_store_uri(h, path) != 0) {
		divoom_system_free_lock(h->lock);
		return -1;
	}
	h->loop = loop ? 1 : 0;
	h->end_ticks = end_ticks;
	h->adec_eof = 0;
	h->demux_eof = 0;
	h->stop_req = 0;
	divoom_plat_audio_reset_fade(h);

	if (divoom_plat_audio_prepare_pipeline(h) != 0) {
		divoom_system_free_lock(h->lock);
		return -1;
	}

	h->playing = 1;
	divoom_system_free_lock(h->lock);
	return 0;
}

void divoom_plat_audio_play_run(void *handle)
{
	Pdivoom_plat_audio_handle h = (Pdivoom_plat_audio_handle)handle;

	if (!h || !h->playing)
		return;
	divoom_plat_audio_worker(h);
}

void divoom_plat_audio_destroy(void *handle)
{
	Pdivoom_plat_audio_handle h = (Pdivoom_plat_audio_handle)handle;

	if (!h)
		return;
	divoom_plat_audio_wait_idle(h);
	if (h->playing || h->sink_open || h->dmx_chn >= 0)
	{
		divoom_plat_audio_destroy_pipeline(h);
		h->playing = 0;
	}
#ifdef CONFIG_COMPONENTS_SPEEXDSP
	if (h->resampler) {
		speex_resampler_destroy(h->resampler);
		h->resampler = NULL;
	}
	h->rsmp_ch = 0;
#endif
	if (h->lock)
		divoom_system_destroy_lock(h->lock);
	divoom_system_free(h);
}

void divoom_plat_audio_destroy_abandon(void *handle)
{
	/* 同步模型下 worker 已随 play_run 返回退出，无僵尸 worker，直接 destroy */
	divoom_plat_audio_destroy(handle);
}

#elif DIVOOM_BUILD_SIMULATOR

#include "divoom_platform_audio_play.h"
#include "divoom.h"

typedef struct {
	divoom_plat_audio_notify_fn notify;
	void *user;
	int playing;
} divoom_plat_audio_sim_handle_t;

void *divoom_plat_audio_create(divoom_plat_audio_notify_fn cb, void *user)
{
	divoom_plat_audio_sim_handle_t *h;

	h = (divoom_plat_audio_sim_handle_t *)divoom_system_malloc(sizeof(*h));
	if (!h)
	{
		return NULL;
	}
	h->notify = cb;
	h->user = user;
	h->playing = 0;
	return h;
}

int divoom_plat_audio_play(void *handle, const char *path, int loop)
{
	return divoom_plat_audio_play_until(handle, path, loop, 0);
}

int divoom_plat_audio_play_until(void *handle, const char *path, int loop, unsigned int end_ticks)
{
	divoom_plat_audio_sim_handle_t *h = (divoom_plat_audio_sim_handle_t *)handle;

	(void)path;
	(void)loop;
	(void)end_ticks;
	if (!h)
	{
		return -1;
	}
	h->playing = 1;
	return 0;
}

void divoom_plat_audio_play_run(void *handle)
{
	divoom_plat_audio_sim_handle_t *h = (divoom_plat_audio_sim_handle_t *)handle;

	if (!h)
	{
		return;
	}
	h->playing = 0;
	if (h->notify)
	{
		h->notify(h->user, DIVOOM_PLAT_AUDIO_NOTIFY_COMPLETE);
	}
}

void divoom_plat_audio_set_volume(void *handle, int vol_0_100)
{
	(void)handle;
	(void)vol_0_100;
}

void divoom_plat_audio_set_jl_tone_vol(void *handle, int vol_0_100)
{
	(void)handle;
	(void)vol_0_100;
}

void divoom_plat_audio_stop(void *handle)
{
	divoom_plat_audio_sim_handle_t *h = (divoom_plat_audio_sim_handle_t *)handle;

	if (!h)
	{
		return;
	}
	h->playing = 0;
}

void divoom_plat_audio_request_stop(void *handle)
{
	divoom_plat_audio_stop(handle);
}

void divoom_plat_audio_release_pcm_for_external(void *handle)
{
	divoom_plat_audio_stop(handle);
}

int divoom_plat_audio_is_playing(void *handle)
{
	divoom_plat_audio_sim_handle_t *h = (divoom_plat_audio_sim_handle_t *)handle;

	return (h && h->playing) ? 1 : 0;
}

static void divoom_plat_audio_destroy_common(void *handle, int notify_error)
{
	divoom_plat_audio_sim_handle_t *h = (divoom_plat_audio_sim_handle_t *)handle;

	if (!h)
	{
		return;
	}
	h->playing = 0;
	if (notify_error && h->notify)
	{
		h->notify(h->user, DIVOOM_PLAT_AUDIO_NOTIFY_ERROR);
	}
	divoom_system_free(h);
}

void divoom_plat_audio_destroy(void *handle)
{
	divoom_plat_audio_destroy_common(handle, 0);
}

void divoom_plat_audio_destroy_abandon(void *handle)
{
	divoom_plat_audio_destroy_common(handle, 0);
}

#endif

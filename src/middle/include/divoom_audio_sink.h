#ifndef _divoom_audio_sink_h_
#define _divoom_audio_sink_h_

#include "divoom_build_config.h"
#include "divoom_type.h"

typedef enum {
	DIVOOM_AUDIO_SINK_CH_SLEEP = 0,
	DIVOOM_AUDIO_SINK_CH_TONE,
#if DIVOOM_SHOW_TF_NAND
	DIVOOM_AUDIO_SINK_CH_MUSIC,
#endif
#ifdef CONFIG_DIVOOM_MPP_NET_AUDIO
	DIVOOM_AUDIO_SINK_CH_NETWORK,
#endif
	DIVOOM_AUDIO_SINK_CH_COUNT
} divoom_audio_sink_ch_t;

#define DIVOOM_AUDIO_SINK_FADE_MS_NORMAL  50u
#define DIVOOM_AUDIO_SINK_FADE_MS_CUT     15u

int divoom_audio_sink_start(void);
void divoom_audio_sink_stop(void);
int divoom_audio_sink_is_running(void);

void divoom_audio_sink_ch_enable(divoom_audio_sink_ch_t ch, int on);
/* vol_0_100：用户档位。内部平方换成幅度再写 vol_target；duck/fade 仍为线性比例 */
void divoom_audio_sink_ch_set_volume(divoom_audio_sink_ch_t ch, unsigned int vol_0_100);
void divoom_audio_sink_ch_set_duck(divoom_audio_sink_ch_t ch, unsigned int duck_0_100);
unsigned int divoom_audio_sink_ch_get_duck(divoom_audio_sink_ch_t ch);
void divoom_audio_sink_ch_fade_in(divoom_audio_sink_ch_t ch, unsigned int ms, unsigned int from_vol);
/* 数字静音 + 丢弃 ring 待播数据；冲刷静音前调用（fade_in 不再隐式清 ring） */
void divoom_audio_sink_ch_mute_drop(divoom_audio_sink_ch_t ch);
void divoom_audio_sink_ch_fade_out(divoom_audio_sink_ch_t ch, unsigned int ms);
int divoom_audio_sink_ch_fade_out_wait(divoom_audio_sink_ch_t ch, unsigned int timeout_ms);

/* 返回本次写入的帧数；0 表示 ring 满未写入；-1 表示错误 */
int divoom_audio_sink_push(divoom_audio_sink_ch_t ch, const int16_t *stereo, unsigned int frames);
int divoom_audio_sink_push_all(divoom_audio_sink_ch_t ch, const int16_t *stereo, unsigned int frames);
/** 向通道写入 frames 帧立体声静音（BSS 零缓冲分片 push，调用方无需栈上大数组） */
int divoom_audio_sink_push_silence(divoom_audio_sink_ch_t ch, unsigned int frames);
int divoom_audio_sink_ch_active(divoom_audio_sink_ch_t ch);
void divoom_audio_sink_ch_disable(divoom_audio_sink_ch_t ch);
void divoom_audio_sink_try_stop(void);
/* 淡出结束并拆 ring，归还 buf_pool 租约（提示音结束 / snd_pcm close 等） */
void divoom_audio_sink_force_release_bus(divoom_audio_sink_ch_t ch);

/** force_release_bus 调用次数（audio_diag） */
unsigned int divoom_audio_sink_bus_force_release_count(void);

unsigned int divoom_audio_sink_whitebox_out_frames(void);
unsigned int divoom_audio_sink_whitebox_underruns(divoom_audio_sink_ch_t ch);
unsigned int divoom_audio_sink_whitebox_silence_periods(void);
unsigned int divoom_audio_sink_ch_env_mul(divoom_audio_sink_ch_t ch);
int divoom_audio_sink_ch_ring_bound(divoom_audio_sink_ch_t ch);
/* 当前租约绑定的PCM容量，单位字节，未绑定返回0。 */
unsigned int divoom_audio_sink_ch_ring_capacity(divoom_audio_sink_ch_t ch);
/* 电台起播/欠载后是否正在积累预缓冲；其他通道始终返回 0。 */
int divoom_audio_sink_ch_buffering(divoom_audio_sink_ch_t ch);
/* 正常 EOF 时释放不足预缓冲门限的尾音；STOP 仍走淡出并丢弃。 */
void divoom_audio_sink_ch_drain(divoom_audio_sink_ch_t ch);
unsigned int divoom_audio_sink_ch_ring_bytes(divoom_audio_sink_ch_t ch);
unsigned int divoom_audio_sink_ch_ring_blank_bytes(divoom_audio_sink_ch_t ch);
unsigned int divoom_audio_sink_ch_ring_fill_pct(divoom_audio_sink_ch_t ch);

void divoom_audio_sink_diag_enable(int on);
void divoom_audio_sink_diag_clear(void);
void divoom_audio_sink_diag_print(void);

#endif

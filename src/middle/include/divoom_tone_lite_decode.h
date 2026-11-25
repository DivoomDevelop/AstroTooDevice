#ifndef _divoom_tone_lite_decode_h_
#define _divoom_tone_lite_decode_h_

#include "divoom_build_config.h"
#include "divoom_audio_sink.h"

/**
 * 轻量提示音解码：divoom_mp3dec(MP3) + Helix/FAAD2(ADTS AAC) → Speex 重采样 48k → audio_sink。
 * 绕开 CedarX DEMUX/ADEC/MPI，供短提示音使用。
 */

enum {
	DIVOOM_TONE_LITE_NOTIFY_COMPLETE = 1,
	DIVOOM_TONE_LITE_NOTIFY_ERROR = 2,
};

typedef void (*divoom_tone_lite_notify_fn)(void *user, int event);
/** play_run 热路径轮询：处理 SET_VOLUME / STOP / CUT 等控制队列项 */
typedef void (*divoom_tone_lite_control_fn)(void *user);

typedef enum {
	DIVOOM_TONE_LITE_MODE_COLD = 0,
	DIVOOM_TONE_LITE_MODE_OVERLAY,
} divoom_tone_lite_play_mode_t;

/** MP3 或 ADTS-AAC 时返回 1 */
int divoom_tone_lite_path_supported(const char *path);

void *divoom_tone_lite_create(divoom_tone_lite_notify_fn cb, void *user);
#if DIVOOM_SHOW_TF_NAND
void divoom_tone_lite_set_sink_ch(void *handle, divoom_audio_sink_ch_t ch);
void divoom_tone_lite_set_paused(void *handle, int paused);
int divoom_tone_lite_get_position_sec(void *handle);
int divoom_tone_lite_get_duration_sec(void *handle);
#endif
int divoom_tone_lite_play_until(void *handle, const char *path, int loop, unsigned int end_ticks);
void divoom_tone_lite_set_play_mode(void *handle, divoom_tone_lite_play_mode_t mode);
/** play_run 期间周期性调用；传 NULL 清除 */
void divoom_tone_lite_set_control_hook(void *handle, divoom_tone_lite_control_fn fn, void *user);
void divoom_tone_lite_play_run(void *handle);
void divoom_tone_lite_set_volume(void *handle, int vol_0_100);
void divoom_tone_lite_set_jl_tone_vol(void *handle, int vol_0_100);
void divoom_tone_lite_request_stop(void *handle);
/** 若 sink 仍开则 close（session 释放 TONE bus 前调用） */
void divoom_tone_lite_close_sink_if_open(void *handle);
int divoom_tone_lite_is_playing(void *handle);
/** 1= audio_sink/I2S 仍开（含切歌 keep 空窗） */
int divoom_tone_lite_sink_is_open(void *handle);
/** play_run 返回后查询：1=被 request_stop 打断（切歌/外部 stop），0=自然结束或错误退出 */
int divoom_tone_lite_stop_requested(void *handle);
/** 白盒：当前 play_mode（DIVOOM_TONE_LITE_MODE_*） */
int divoom_tone_lite_get_play_mode(void *handle);
void divoom_tone_lite_destroy_abandon(void *handle);

#endif

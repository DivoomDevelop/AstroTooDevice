#ifndef _divoom_platform_audio_play_h_
#define _divoom_platform_audio_play_h_

/**
 * F101 真机平台音频播放适配层：DEMUX+ADEC → 48k Speex → sunxi_pcm(I2S0/JL7012)。
 * 替换原 TPlayer 提示音后端；模拟器不编译本实现。
 */

enum {
	DIVOOM_PLAT_AUDIO_NOTIFY_COMPLETE = 1,
	DIVOOM_PLAT_AUDIO_NOTIFY_ERROR = 2,
};

typedef void (*divoom_plat_audio_notify_fn)(void *user, int event);

void *divoom_plat_audio_create(divoom_plat_audio_notify_fn cb, void *user);

/** 开始播本地文件；path 可含 file:// 前缀。loop!=0 时 EOF 后 seek 循环。
 *  end_ticks!=0 时，循环前若系统 tick 已超过该值则结束并 COMPLETE。 */
int divoom_plat_audio_play(void *handle, const char *path, int loop);
int divoom_plat_audio_play_until(void *handle, const char *path, int loop, unsigned int end_ticks);

/** prepare 成功后由 local_tone 同步跑解码（阻塞至播完或 stop_req） */
void divoom_plat_audio_play_run(void *handle);

void divoom_plat_audio_set_volume(void *handle, int vol_0_100);

/** 首帧 PCM 写出后再 ramp 开 JL TONE 音量（切歌时 JL 已 mute） */
void divoom_plat_audio_set_jl_tone_vol(void *handle, int vol_0_100);

void divoom_plat_audio_stop(void *handle);

/** 仅置 stop_req，不阻塞；下一首 play_until 内 wait_idle 会等 worker 退出 */
void divoom_plat_audio_request_stop(void *handle);

/** 外部（snd_pcm/DIY）抢占 I2S：停 worker 并强制 close PCM（不复用切歌保留）。 */
void divoom_plat_audio_release_pcm_for_external(void *handle);

/** 1=worker 正在播/收尾，0=空闲（handle 可存在但不在播） */
int divoom_plat_audio_is_playing(void *handle);

void divoom_plat_audio_destroy(void *handle);

/** join 失败或强制收尾：不阻塞 wait_idle，释放 I2S/MPP 后销毁 handle */
void divoom_plat_audio_destroy_abandon(void *handle);

#endif

#ifndef _divoom_sleep_pcm_session_h_
#define _divoom_sleep_pcm_session_h_

typedef enum
{
	DIVOOM_SLEEP_PCM_SESSION_NONE = 0,
	DIVOOM_SLEEP_PCM_SESSION_DIY,
	DIVOOM_SLEEP_PCM_SESSION_MPG,
} divoom_sleep_pcm_session_mode_t;

/** DIY mix_pcm + snd_pcm；已在 DIY 且 snd_pcm 存活时返回 0。失败 -1。 */
int divoom_sleep_pcm_session_start_diy(void);

/** 停止 mix_pcm + snd_pcm，并清除 session 状态与 sleep playback policy。 */
void divoom_sleep_pcm_session_stop(void);

/** 异步 stop（不阻塞 UI 线程）；内部 audio_session worker。 */
void divoom_sleep_pcm_session_stop_async(void);

divoom_sleep_pcm_session_mode_t divoom_sleep_pcm_session_get_mode(void);

/** session 非 NONE 且 snd_pcm 线程在跑。 */
int divoom_sleep_pcm_session_is_running(void);

/**
 * MPG 冷启 / 同路径防抖切歌 / DIY→MPG 热切换。
 * 冷启前由上层先 release I2S 并 stop tone（本函数不调用 voice_stop）。
 * 成功 0；冷启 decode 失败 -1。
 */
int divoom_sleep_pcm_session_start_or_switch_mpg(const char *mp3_path, int vol_percent);

/** MPG→DIY 热切换；成功 1，无需切换或失败 0。 */
int divoom_sleep_pcm_session_switch_mpg_to_diy(void);

const char *divoom_sleep_pcm_session_mpg_path(void);

void divoom_sleep_pcm_session_set_diy_mix_vol_level(int level_0_16);

/** 非 JL 硬件路径的 mpg 音量（0~100）。 */
void divoom_sleep_pcm_session_set_mpg_vol_percent(int vol_percent);

void divoom_sleep_pcm_session_prepare_i2s(unsigned int timeout_ms);

#endif

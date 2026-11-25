#ifndef _divoom_eq_noise_mic_fft_H_
#define _divoom_eq_noise_mic_fft_H_

typedef enum
{
	DIVOOM_FFT_MIC_NOISE,
	DIVOOM_FFT_MIC_EQ,
}DIVOOM_FFT_MIC_TYPE;

void divoom_eq_noise_mic_fft_start(void *func, DIVOOM_FFT_MIC_TYPE type);

void divoom_eq_noise_mic_fft_start_clock(void *func, DIVOOM_FFT_MIC_TYPE type);

void divoom_eq_noise_mic_fft_stop(void);

void divoom_eq_noise_mic_fft_stop_clock(void);

void divoom_eq_noise_mic_fft_init(void);

/* Line-In 退出后若重新 divoom_pcm_init，需调用此函数恢复麦克风 FFT 回调 */
void divoom_eq_noise_mic_fft_register_pcm_callback(void);

void divoom_eq_noise_mic_fft_exit(void);

int divoom_eq_noise_mic_fft_get_noise_value(void);

#if defined(DIVOOM_PRODUCT_ASTRO_TOO)
/* 只提交解码后的 48kHz 双声道 S16 PCM，数据取自音量处理前。 */
void divoom_eq_noise_mic_fft_feed_radio_pcm(const int16 *pcm, int32 frames);
/* 清除换台、停止或重连前的输入及频谱。 */
void divoom_eq_noise_mic_fft_reset_radio(void);
/* 在电台控制任务计算并发布频谱。 */
void divoom_eq_noise_mic_fft_poll_radio(void);
/* 复制最近频带，返回 0 表示无新频谱；bands 可为 NULL，仅查询可用性。 */
int32 divoom_eq_noise_mic_fft_read_radio(int32 *bands, int32 max_age_ms);
#endif

#endif




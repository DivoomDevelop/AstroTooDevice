#ifndef DIVOOM_READ_PCM_H
#define DIVOOM_READ_PCM_H

// 回调函数类型定义
typedef void (*pcm_data_callback)(uint16 *data, int len);

// 模块初始化
int divoom_pcm_init(unsigned int sample_rate);

/* 打开再关闭 default 播放端，促使 softvol 等控件出现在 mixer；成功一次后内部不再打开设备 */
void divoom_pcm_try_prime_playback_default(void);

/** 拉高 audiocodec 采集增益（ADC/MIC）；录音前可重复调用 */
void divoom_pcm_apply_capture_gain(void);

// 设置数据回调函数
void divoom_pcm_set_callback(pcm_data_callback callback);

/** 当前 PCM 采样率（未 init 时返回 0） */
unsigned int divoom_pcm_get_sample_rate(void);

/** 确保麦克风采集线程已启动（不关闭/重开 ALSA 设备；设备仅 init/deinit 生命周期内 open 一次） */
int divoom_pcm_ensure_capture(void);

// 启动PCM读取
int divoom_pcm_start(void);

// 停止PCM读取
void divoom_pcm_stop(void);

// 清理资源
void divoom_pcm_deinit(void);

#endif // DIVOOM_READ_PCM_H 
#ifndef __DIVOOM_SPECTRUM_H__
#define __DIVOOM_SPECTRUM_H__

#include <math.h>
#include <stdint.h>
#include "divoom_type.h"

/*
 * 界面用的唯一一份频谱：64 柱，每柱 0~64。
 * 来源二选一：杰理 UART 0x3b（约 20ms 一包），或 F101 本机解码出声时自己算。
 * 本机算法与杰理 divoom_spectrum.c 音乐频谱同一套：1024 点、汉宁窗、抽 2、50% 重叠。
 */
#define DIVOOM_SPECTRUM_BAND_NUM            64

#if defined(DIVOOM_PRODUCT_ASTRO_TOO)
#define DIVOOM_SPECTRUM_RADIO_WINDOW_SAMPLES 1024
/* 将电台的 24kHz 采样窗口转换为与展示模式相同的 64 个频带。 */
int divoom_spectrum_calculate_radio_window(const int16 *samples, int32_t *bands);
#endif
#define DIVOOM_SPECTRUM_LEVEL_MAX           64
#define DIVOOM_SPECTRUM_SILENCE_MS_DEFAULT  500

void divoom_spectrum_init(void);
void divoom_spectrum_exit(void);

/* pcm 为 S16 交错，frames 为帧数。仅在 divoom_spectrum_set_local(1) 期间计算。 */
void divoom_spectrum_process(const int16 *pcm, uint16 frames, int channels, int sample_rate);

/* F101 本机解码出声时开/关。开：清缓存并丢弃杰理数据；关：频谱清零，恢复收杰理数据。 */
void divoom_spectrum_set_local(int on);

/* UART 0x3b 收包后调用；levels 为 NULL 或 n<=0 表示包无效，频谱判为无数据。本机开着时丢弃。 */
void divoom_spectrum_update_from_jl(const uint8 *levels, int n);

/* 清空频谱并标记从未更新（btcore 退出时调用） */
void divoom_spectrum_reset(void);

/*
 * 以下读取接口在“关了麦克风且没在播”时一律返回无数据/全 0。
 * silence_ms <= 0 时用 DIVOOM_SPECTRUM_SILENCE_MS_DEFAULT。
 */
/* 一次读取当前音源的有效频带，返回点数；out 可为空，仅查询点数。 */
int divoom_spectrum_read_levels(int32_t out[DIVOOM_SPECTRUM_BAND_NUM], int silence_ms);
/* silence_ms 内更新过且柱数 > 0 */
int divoom_spectrum_is_active(int silence_ms);
/* is_active 且不是全 0 */
int divoom_spectrum_has_signal(int silence_ms);
void divoom_spectrum_get_levels(int32_t out[DIVOOM_SPECTRUM_BAND_NUM]);
/* 最近一次更新的柱数 0~DIVOOM_SPECTRUM_BAND_NUM */
int divoom_spectrum_get_count(void);
/* 每隔 step 柱取一柱填 count 个，截到 0~max；返回 1=有效 */
int divoom_spectrum_fill(uint8 *out, int count, int step, int max);

#endif

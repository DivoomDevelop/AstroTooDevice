#ifndef __DIVOOM_FFT_H__
#define __DIVOOM_FFT_H__

#include "divoom_type.h"

/*
 * 原地实数 FFT。data_count 取 64~2048 的 2 的幂。
 * 输出按复数交错：x[2k] 为实部，x[2k+1] 为虚部；x[0]/x[1] 为直流与奈奎斯特。
 * scale_down 为 0 时先左移补偿逐级右移。返回 1 表示完成。
 */
int divoom_fft_rfft(int32 *x, int data_count, int scale_down);

#endif

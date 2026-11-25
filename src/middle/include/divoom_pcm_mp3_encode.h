#ifndef _divoom_pcm_mp3_encode_h_
#define _divoom_pcm_mp3_encode_h_

#include "divoom_type.h"
#include <stdio.h>

#define DIVOOM_PCM_MP3_ENCODE_BITRATE_DEFAULT 64

typedef struct _divoom_pcm_mp3_encoder divoom_pcm_mp3_encoder_t;

/**
 * 打开 Shine MP3 编码器并绑定输出文件。
 * @param bitrate_kbps  码率 kbps，0 使用默认 64
 */
divoom_pcm_mp3_encoder_t *divoom_pcm_mp3_encode_open(FILE *fp, uint32 sample_rate, int bitrate_kbps);

/** 写入 PCM（16-bit 单声道），内部按帧编码 */
int divoom_pcm_mp3_encode_feed(divoom_pcm_mp3_encoder_t *enc, const int16_t *pcm, int sample_count);

/** flush 并关闭编码器，释放资源 */
int divoom_pcm_mp3_encode_close(divoom_pcm_mp3_encoder_t **enc);

#endif

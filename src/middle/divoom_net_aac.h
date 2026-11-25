#ifndef DIVOOM_NET_AAC_H
#define DIVOOM_NET_AAC_H
#include "divoom_type.h"

/* 网络ADTS AAC解码（LC，开 SBR 时含 HE v1）；只保留压缩残帧和一帧PCM，坏帧跳过，
 * 输出沿用音源的背压与取消。 */
typedef struct divoom_net_aac divoom_net_aac;
typedef int32 (*divoom_net_aac_output)(void *user, const int16 *pcm,
    uint32 frames, uint32 rate, uint32 channels);
divoom_net_aac *divoom_net_aac_create(void);
void divoom_net_aac_destroy(divoom_net_aac *decoder);
/* 0成功（eof时丢弃尾部残帧）；-1输出错误；-2 ADIF/LATM，或编码超出LC双声道。 */
int32 divoom_net_aac_feed(divoom_net_aac *decoder, const uint8 *data, uint32 bytes,
    uint8 eof, divoom_net_aac_output output, void *user);
#endif

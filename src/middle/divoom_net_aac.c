/* 网络 ADTS AAC：只负责攒完整帧，解码与坏帧重同步在 divoom_aacdec。 */
#include "divoom.h"
#include "divoom_net_aac.h"
#include "divoom_aacdec.h"
#include <string.h>

struct divoom_net_aac
{
    divoom_aacdec_t *decoder;
    uint32 used;
    uint8 probed;
    uint8 playing;
    uint8 input[8192]; /* ADTS的13位帧长最大8191字节。 */
    int16 pcm[DIVOOM_AACDEC_MAX_SAMPLES_PER_FRAME];
};

divoom_net_aac *divoom_net_aac_create(void)
{
    divoom_net_aac *decoder = divoom_system_malloc(sizeof(*decoder));
    if (decoder)
    {
        memset(decoder, 0, sizeof(*decoder));
        decoder->decoder = divoom_aacdec_create();
        if (!decoder->decoder)
        {
            divoom_system_free(decoder);
            decoder = NULL;
        }
    }
    return decoder;
}

void divoom_net_aac_destroy(divoom_net_aac *decoder)
{
    if (decoder)
    {
        divoom_aacdec_destroy(decoder->decoder);
        divoom_system_free(decoder);
    }
}

static int32 divoom_net_aac_decode(divoom_net_aac *decoder, divoom_net_aac_output output, void *user)
{
    while (decoder->used)
    {
        divoom_aacdec_frame_info_t info;
        uint8 *p = decoder->input;
        int frames;
        uint32 drop;

        /* ADIF/LATM 不是 ADTS：只看流的开头，认出来交回上层，避免把整条流当垃圾扫。 */
        if (!decoder->probed)
        {
            if (decoder->used < 4)
            {
                break;
            }
            if (!memcmp(p, "ADIF", 4) || (p[0] == 0x56 && (p[1] & 0xe0) == 0xe0))
            {
                return -2;
            }
            decoder->probed = 1;
        }
        frames = divoom_aacdec_decode_frame(decoder->decoder, p, (int)decoder->used,
            decoder->pcm, &info);
        /* 已经出过声的流不会中途换编码；那时的“不支持”帧头是损坏数据，跳过即可。 */
        if (frames == DIVOOM_AACDEC_ERR_UNSUPPORTED)
        {
            if (!decoder->playing)
            {
                return -2;
            }
            frames = 0;
        }
        drop = (uint32)(info.frame_bytes ? info.frame_bytes : info.frame_offset);
        if (frames < 0 || drop > decoder->used)
        {
            return -1;
        }
        if (drop)
        {
            decoder->used -= drop;
            memmove(decoder->input, decoder->input + drop, decoder->used);
        }
        if (!info.frame_bytes)
        {
            break;
        }
        /* 坏帧只丢一个字节后重同步，不中断整条流。 */
        if (frames > 0)
        {
            decoder->playing = 1;
            if (output(user, decoder->pcm, (uint32)frames, (uint32)info.hz,
                (uint32)info.channels) != 0)
            {
                return -1;
            }
        }
    }
    return 0;
}

int32 divoom_net_aac_feed(divoom_net_aac *decoder, const uint8 *data, uint32 bytes,
    uint8 eof, divoom_net_aac_output output, void *user)
{
    if (!decoder || !output || (bytes && !data))
    {
        return -1;
    }
    while (bytes)
    {
        uint32 copy = sizeof(decoder->input) - decoder->used;
        if (!copy)
        {
            return -2;
        }
        if (copy > bytes)
        {
            copy = bytes;
        }
        memcpy(decoder->input + decoder->used, data, copy);
        decoder->used += copy;
        data += copy;
        bytes -= copy;
        int32 result = divoom_net_aac_decode(decoder, output, user);
        if (result)
        {
            return result;
        }
    }
    if (eof)
    {
        decoder->used = 0; /* 尾部残帧或标签不解码，与 net_mp3 一致 */
    }
    return 0;
}

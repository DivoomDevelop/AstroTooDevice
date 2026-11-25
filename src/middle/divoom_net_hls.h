#ifndef DIVOOM_NET_HLS_H
#define DIVOOM_NET_HLS_H
#include "divoom_type.h"

#define DIVOOM_NET_HLS_URL_BYTES 1024
#define DIVOOM_NET_HLS_RESET 0
#define DIVOOM_NET_HLS_AAC 1
#define DIVOOM_NET_HLS_MP3 2
#define DIVOOM_NET_HLS_SEGMENT 3 /* 无data的分片完成事件，按顺序统计，不重置解码器。 */

/* 同步生产压缩音频；调用者提供可取消HTTP、等待及有背压的输出。
 * emit(RESET,NULL,0)表示时间线/音轨改变，必须按顺序重置解码器。 */
typedef struct
{
    void *user;
    int32 (*open)(void *user, const char *url);
    int32 (*read)(void *user, uint8 *data, uint32 bytes);
    void (*close)(void *user);
    int32 (*wait)(void *user, uint32 milliseconds);
    int32 (*cancelled)(void *user);
    int32 (*emit)(void *user, uint8 codec, const uint8 *data, uint32 bytes);
} divoom_net_hls_io;

/* 支持未加密的主/媒体列表、MPEG-TS AAC-LC/MP3；0结束、-1传输/取消、-2不支持。 */
int32 divoom_net_hls_run(const char *url, const divoom_net_hls_io *io);
/* RFC3986中HTTP(S)相对路径合并；拒绝超长及其他协议，绝不截断。 */
int32 divoom_net_hls_resolve(const char *base, const char *relative, char *out, uint32 capacity);
#endif

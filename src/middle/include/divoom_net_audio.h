#ifndef DIVOOM_NET_AUDIO_H
#define DIVOOM_NET_AUDIO_H
#ifdef __cplusplus
extern "C" {
#endif
/* Final callback runs on the worker after releasing session ownership.
 * It may start a new URL; play returns -1 if another API caller owns a request.
 * Keep callbacks short. stop from a callback cancels without waiting. */
typedef enum {
    DIVOOM_NET_AUDIO_IDLE, DIVOOM_NET_AUDIO_CONNECTING,
    DIVOOM_NET_AUDIO_PLAYING, DIVOOM_NET_AUDIO_RECONNECTING,
    DIVOOM_NET_AUDIO_STOPPING,
    DIVOOM_NET_AUDIO_BUFFERING /* 正在积累直播压缩缓存，保留既有枚举数值。 */
} divoom_net_audio_state_t;
typedef enum {
    DIVOOM_NET_AUDIO_COMPLETE, DIVOOM_NET_AUDIO_ERROR, DIVOOM_NET_AUDIO_STOPPED
} divoom_net_audio_event_t;
typedef void (*divoom_net_audio_callback)(divoom_net_audio_event_t event, void *user);
int divoom_net_audio_play(const char *url, unsigned int volume,
                         divoom_net_audio_callback callback, void *user);
void divoom_net_audio_stop(void); /* cancel and wait, except in a callback */
void divoom_net_audio_set_volume(unsigned int volume); /* UI scale 0..100 */
divoom_net_audio_state_t divoom_net_audio_get_state(void);
#ifdef __cplusplus
}
#endif
#endif

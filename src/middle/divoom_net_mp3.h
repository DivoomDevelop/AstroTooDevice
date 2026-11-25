#ifndef DIVOOM_NET_MP3_H
#define DIVOOM_NET_MP3_H
#include <stddef.h>
#include <stdint.h>

/* Decoder state is private to each session; the MPEG audio decoder itself is
 * supplied once by libastrotoo_bundle (divoom_mp3dec, also used by tone_lite). */
typedef struct divoom_net_mp3 divoom_net_mp3_t;
typedef int (*divoom_net_mp3_output)(void *user, const int16_t *pcm,
                                   unsigned int frames, unsigned int rate,
                                   unsigned int channels);
divoom_net_mp3_t *divoom_net_mp3_create(void);
void divoom_net_mp3_destroy(divoom_net_mp3_t *d);
/* feed accepts arbitrary packet boundaries; output returning nonzero cancels
 * immediately. eof drains complete frames. */
int divoom_net_mp3_feed(divoom_net_mp3_t *d, const unsigned char *data,
                       size_t bytes, int eof, divoom_net_mp3_output output,
                       void *user);
#endif

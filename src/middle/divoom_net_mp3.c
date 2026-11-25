#include "divoom.h"
#include "divoom_net_mp3.h"
#include "divoom_mp3dec.h"
#include <string.h>

struct divoom_net_mp3 {
    divoom_mp3dec_t *decoder;
    unsigned char input[8192];
    size_t used;
    int16_t pcm[DIVOOM_MP3DEC_MAX_SAMPLES_PER_FRAME];
};

divoom_net_mp3_t *divoom_net_mp3_create(void)
{
    divoom_net_mp3_t *d = divoom_system_malloc(sizeof(*d));

    if (!d)
        return NULL;
    memset(d, 0, sizeof(*d));
    d->decoder = divoom_mp3dec_create();
    if (!d->decoder) {
        divoom_system_free(d);
        return NULL;
    }
    return d;
}

void divoom_net_mp3_destroy(divoom_net_mp3_t *d)
{
    if (!d)
        return;
    divoom_mp3dec_destroy(d->decoder);
    divoom_system_free(d);
}

static int decode_pending(divoom_net_mp3_t *d, divoom_net_mp3_output output,
                          void *user)
{
    while (d->used) {
        divoom_mp3dec_frame_info_t info;
        size_t drop;
        /* A partial frame at the end of a packet is kept, not dropped: the
         * decoder waits for the rest and keeps its bit reservoir. */
        int frames = divoom_mp3dec_decode_frame(d->decoder, d->input, (int)d->used,
                                                d->pcm, &info);

        drop = (size_t)(info.frame_bytes ? info.frame_bytes : info.frame_offset);
        if (drop > d->used || frames < 0 || frames > 1152)
            return -1;
        if (drop) {
            d->used -= drop;
            memmove(d->input, d->input + drop, d->used);
        }
        if (!info.frame_bytes)
            break;
        /* A frame needing an earlier bit reservoir may yield no samples;
         * consuming it still primes the decoder for the following frame. */
        if (frames && output(user, d->pcm, (unsigned int)frames,
                             (unsigned int)info.hz, (unsigned int)info.channels))
            return -1;
    }
    return 0;
}

int divoom_net_mp3_feed(divoom_net_mp3_t *d, const unsigned char *data,
                       size_t bytes, int eof, divoom_net_mp3_output output,
                       void *user)
{
    do {
        size_t n;
        if (decode_pending(d, output, user)) return -1;
        if (!bytes) break;
        if (d->used == sizeof(d->input)) {
            /* No sync in a full window: retain 7 KiB, enough for three
             * maximum-size free-format frames (2304 bytes each). */
            d->used -= 1024;
            memmove(d->input, d->input + 1024, d->used);
        }
        n = sizeof(d->input) - d->used;
        if (n > bytes) n = bytes;
        memcpy(d->input + d->used, data, n);
        d->used += n; data += n; bytes -= n;
    } while (1);
    if (eof) d->used = 0; /* incomplete trailing bytes are never decoded */
    return 0;
}

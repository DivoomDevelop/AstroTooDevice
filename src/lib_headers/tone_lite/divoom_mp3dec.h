#ifndef DIVOOM_MP3DEC_H
#define DIVOOM_MP3DEC_H
#include <stddef.h>
#include <stdint.h>

/* MPEG-1/2/2.5 audio Layer I/II/III decoder used by tone_lite, net audio and
 * AI voice. Backend is chosen in Kconfig: Helix MP3 + fixed-point Layer I/II
 * (default) or minimp3. Output is interleaved 16-bit PCM. */
#define DIVOOM_MP3DEC_MAX_SAMPLES_PER_FRAME (1152 * 2)

typedef struct {
    /* Bytes to drop from the input: leading junk plus the frame itself.
     * 0 when no complete frame is available yet. */
    int frame_bytes;
    /* Junk before the frame; without a complete frame, the bytes that can
     * safely be discarded while waiting for more input. */
    int frame_offset;
    int channels, hz, layer, bitrate_kbps;
} divoom_mp3dec_frame_info_t;

typedef struct divoom_mp3dec divoom_mp3dec_t;

divoom_mp3dec_t *divoom_mp3dec_create(void);
void divoom_mp3dec_destroy(divoom_mp3dec_t *d);
/* Forget sync, bit reservoir and synthesis history (e.g. after a seek). */
void divoom_mp3dec_reset(divoom_mp3dec_t *d);
/* Decodes the first complete frame in buf. Returns samples per channel; 0
 * with frame_bytes > 0 means the frame was consumed without output (bit
 * reservoir priming or a corrupt frame). pcm == NULL only probes the next
 * frame and leaves the decoder state untouched. */
int divoom_mp3dec_decode_frame(divoom_mp3dec_t *d, const uint8_t *buf, int bytes,
                               int16_t *pcm, divoom_mp3dec_frame_info_t *info);

/* File-style streaming: skips ID3v2, honours Xing/Info/LAME frame count and
 * encoder delay/padding, and strips a trailing ID3v1 tag. */
typedef size_t (*divoom_mp3dec_read_fn)(void *buf, size_t size, void *user);
typedef int (*divoom_mp3dec_seek_fn)(uint32_t position, void *user);

typedef struct {
    unsigned int hz, channels, layer, bitrate_kbps;
    /* Samples per channel from the Xing/Info tag, 0 if unknown. */
    uint32_t total_frames;
} divoom_mp3dec_stream_info_t;

typedef struct divoom_mp3dec_stream divoom_mp3dec_stream_t;

/* Returns NULL when no MPEG audio frame is found or memory is short. */
divoom_mp3dec_stream_t *divoom_mp3dec_stream_open(divoom_mp3dec_read_fn read,
                                                  divoom_mp3dec_seek_fn seek, void *user,
                                                  divoom_mp3dec_stream_info_t *info);
void divoom_mp3dec_stream_close(divoom_mp3dec_stream_t *s);
/* Reads up to samples interleaved samples; 0 at end of stream or on error.
 * Stops early when the stream changes sample rate, layer or channels. */
size_t divoom_mp3dec_stream_read(divoom_mp3dec_stream_t *s, int16_t *pcm, size_t samples);
/* Back to the first audio frame. */
int divoom_mp3dec_stream_rewind(divoom_mp3dec_stream_t *s);
/* Samples per channel returned since open / rewind. */
uint32_t divoom_mp3dec_stream_position(const divoom_mp3dec_stream_t *s);

#endif

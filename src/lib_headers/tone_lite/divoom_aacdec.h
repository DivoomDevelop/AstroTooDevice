/* ADTS AAC decoder shared by tone_lite and net audio. Backend is chosen in
 * Kconfig: Helix (default; LC, plus HE-AAC v1 with DIVOOM_AAC_HE_SBR) or
 * FAAD2 (LC). Output is interleaved 16-bit PCM, one or two channels. */
#ifndef DIVOOM_AACDEC_H
#define DIVOOM_AACDEC_H

#include <stdint.h>

#if defined(CONFIG_DIVOOM_AAC_HE_SBR)
#define DIVOOM_AACDEC_MAX_SAMPLES_PER_FRAME (2048 * 2)
#else
#define DIVOOM_AACDEC_MAX_SAMPLES_PER_FRAME (1024 * 2)
#endif

#define DIVOOM_AACDEC_ERR_UNSUPPORTED (-2)

typedef struct {
    /* Bytes to drop from the input: leading junk plus the frame. A frame that
     * fails to decode drops only its first byte, so the next call resyncs
     * inside it. 0 when no complete frame is available yet. */
    int frame_bytes;
    /* Junk before the frame; without a complete frame, the leading bytes
     * that can be discarded while waiting for more input. */
    int frame_offset;
    int channels;
    int hz;      /* output rate; twice core_hz while SBR is active */
    int core_hz; /* rate in the ADTS header */
    int sbr;
} divoom_aacdec_frame_info_t;

typedef struct divoom_aacdec divoom_aacdec_t;

divoom_aacdec_t *divoom_aacdec_create(void);
void divoom_aacdec_destroy(divoom_aacdec_t *d);
/* Drops decoder history, e.g. after a seek or a stream switch. */
void divoom_aacdec_reset(divoom_aacdec_t *d);
/* Decodes the first complete ADTS frame in buf into pcm (room for
 * DIVOOM_AACDEC_MAX_SAMPLES_PER_FRAME). Returns samples per channel; 0 with
 * frame_bytes > 0 means the bytes were consumed without output. pcm == NULL
 * only probes the header: channels and core_hz are filled, hz equals core_hz.
 * A header found after junk or a bad frame counts only once the next header
 * matches it, so resyncing may wait for one more frame of input.
 * DIVOOM_AACDEC_ERR_UNSUPPORTED: two matching ADTS headers that are not LC or
 * have more than two channels; frame_bytes still tells how to skip it. Frames
 * with several raw blocks are skipped like junk.
 * -1: bad arguments or out of memory. */
int divoom_aacdec_decode_frame(divoom_aacdec_t *d, const uint8_t *buf, int bytes,
                               int16_t *pcm, divoom_aacdec_frame_info_t *info);

#endif

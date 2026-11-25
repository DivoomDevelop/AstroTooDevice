#ifndef _divoom_snd_pcm_h_
#define _divoom_snd_pcm_h_

#include "divoom_type.h"

void divoom_snd_pcm_init(void);
void divoom_snd_pcm_close(void);

#if DIVOOM_BUILD_RTOS_DEVICE
void divoom_snd_pcm_mono_to_stereo(const int16_t *mono, int16_t *stereo, unsigned int frames);
unsigned int divoom_snd_pcm_resample_out_frames(unsigned int in_frames);
int divoom_snd_pcm_whitebox_is_running(void);
unsigned int divoom_snd_pcm_whitebox_out_frames(void);
unsigned int divoom_snd_pcm_whitebox_underruns(void);
unsigned int divoom_snd_pcm_whitebox_silence_periods(void);
#endif

#endif

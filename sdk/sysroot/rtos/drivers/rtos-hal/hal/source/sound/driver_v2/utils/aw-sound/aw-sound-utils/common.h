#ifndef __COMMON_H
#define __COMMON_H

#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
	sound_pcm_t *handle;
	sound_pcm_format_t format;
	unsigned int rate;
	unsigned int channels;
	unsigned int format_bits;
	sound_pcm_uframes_t period_size;
	sound_pcm_uframes_t buffer_size;

	sound_pcm_uframes_t frame_bytes;
	sound_pcm_uframes_t chunk_size;

	unsigned in_aborting;
	unsigned int capture_duration;
	int id;
} audio_mgr_t;

void xrun(sound_pcm_t *handle);
void do_pause(sound_pcm_t *handle);
void do_other_test(sound_pcm_t *handle);

audio_mgr_t *audio_mgr_create(void);
void audio_mgr_release(audio_mgr_t *mgr);
int set_param(sound_pcm_t *handle, sound_pcm_format_t format,
			unsigned int rate, unsigned int channels,
			sound_pcm_uframes_t period_size,
			sound_pcm_uframes_t buffer_size);

int pcm_write(sound_pcm_t *handle, char *data,
	      sound_pcm_uframes_t frames_total,
	      unsigned int frame_bytes);
int pcm_read(sound_pcm_t *handle, const char *data,
	     sound_pcm_uframes_t frames_total,
	     unsigned int frame_bytes);


#ifdef __cplusplus
};  // extern "C"
#endif
#endif /* __COMMON_H */

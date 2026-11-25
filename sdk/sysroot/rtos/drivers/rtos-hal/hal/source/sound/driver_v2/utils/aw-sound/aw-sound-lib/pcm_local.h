/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef __AW_ALSA_PCM_LOCAL_H
#define __AW_ALSA_PCM_LOCAL_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>
#include <aw-sound-lib/pcm.h>
#include <aw-sound-lib/pcm_config.h>
#include <sunxi_hal_sound.h>

#define ATTRIBUTE_UNUSED	__attribute__ ((__unused__))

#define SOUND_PCM_VERSION		"V1.7.3"

//#define SOUND_PCM_RATE_FIX_PERIOD_SIZE

#if 0
#define REFINE_DEBUG
#endif

typedef enum _sound_set_mode {
	SOUND_CHANGE,
	SOUND_TRY,
	SOUND_TEST,
} sound_set_mode_t;

typedef struct _sound_pcm_rbptr {
	sound_pcm_t *master;
	volatile sound_pcm_uframes_t *ptr;
	//int fd;
	off_t offset;
	int link_dst_count;
	sound_pcm_t **link_dst;
	//void *private_data;
	//void (*changed)(sound_pcm_t *pcm, sound_pcm_t *src);
} sound_pcm_rbptr_t;

typedef struct {
	int (*close)(sound_pcm_t *pcm);
	//int (*nonblock)(sound_pcm_t *pcm, int nonblock);
	//int (*async)(sound_pcm_t *pcm, int sig, pid_t pid);
	//int (*info)(sound_pcm_t *pcm, sound_pcm_info_t *info);
	int (*hw_refine)(sound_pcm_t *pcm, sound_pcm_hw_params_t *params);
	int (*hw_params)(sound_pcm_t *pcm, sound_pcm_hw_params_t *params);
	int (*hw_free)(sound_pcm_t *pcm);
	int (*sw_params)(sound_pcm_t *pcm, sound_pcm_sw_params_t *params); /* always locked */
	int (*channel_info)(sound_pcm_t *pcm, sound_pcm_channel_info_t *info);
	void (*dump)(sound_pcm_t *pcm);
	int (*mmap)(sound_pcm_t *pcm);
	int (*munmap)(sound_pcm_t *pcm);
	sound_pcm_chmap_query_t **(*query_chmaps)(sound_pcm_t *pcm);
	sound_pcm_chmap_t *(*get_chmap)(sound_pcm_t *pcm);
	int (*set_chmap)(sound_pcm_t *pcm, const sound_pcm_chmap_t *map);
} sound_pcm_ops_t;

typedef struct {
	//int (*status)(sound_pcm_t *pcm, sound_pcm_status_t *status);
	int (*prepare)(sound_pcm_t *pcm);
	int (*reset)(sound_pcm_t *pcm);
	int (*start)(sound_pcm_t *pcm);
	int (*drop)(sound_pcm_t *pcm);
	int (*drain)(sound_pcm_t *pcm);
	int (*pause)(sound_pcm_t *pcm, int enable);
	sound_pcm_state_t (*state)(sound_pcm_t *pcm);
	int (*hwsync)(sound_pcm_t *pcm);
	int (*delay)(sound_pcm_t *pcm, sound_pcm_sframes_t *delayp);
	int (*resume)(sound_pcm_t *pcm);
	int (*link)(sound_pcm_t *pcm1, sound_pcm_t *pcm2);
	//int (*link_slaves)(sound_pcm_t *pcm, sound_pcm_t *master);
	int (*unlink)(sound_pcm_t *pcm);
	//sound_pcm_sframes_t (*rewindable)(sound_pcm_t *pcm);
	sound_pcm_sframes_t (*rewind)(sound_pcm_t *pcm, sound_pcm_uframes_t frames);
	//sound_pcm_sframes_t (*forwardable)(sound_pcm_t *pcm);
	//sound_pcm_sframes_t (*forward)(sound_pcm_t *pcm, sound_pcm_uframes_t frames);
	sound_pcm_sframes_t (*writei)(sound_pcm_t *pcm, const void *buffer, sound_pcm_uframes_t size);
	//sound_pcm_sframes_t (*writen)(sound_pcm_t *pcm, void **bufs, sound_pcm_uframes_t size);
	sound_pcm_sframes_t (*readi)(sound_pcm_t *pcm, void *buffer, sound_pcm_uframes_t size);
	//sound_pcm_sframes_t (*readn)(sound_pcm_t *pcm, void **bufs, sound_pcm_uframes_t size);
	sound_pcm_sframes_t (*avail_update)(sound_pcm_t *pcm);
	sound_pcm_sframes_t (*mmap_commit)(sound_pcm_t *pcm, sound_pcm_uframes_t offset, sound_pcm_uframes_t size);
	//int (*htimestamp)(sound_pcm_t *pcm, sound_pcm_uframes_t *avail, sound_htimestamp_t *tstamp);
	//int (*poll_descriptors_count)(sound_pcm_t *pcm);
	//int (*poll_descriptors)(sound_pcm_t *pcm, struct pollfd *pfds, unsigned int space);
	//int (*poll_revents)(sound_pcm_t *pcm, struct pollfd *pfds, unsigned int nfds, unsigned short *revents);
	int (*may_wait_for_avail_min)(sound_pcm_t *pcm, sound_pcm_uframes_t avail);
	int (*wait)(sound_pcm_t *pcm, int timeout);
	void (*cache_update)(sound_pcm_t *pcm, sound_pcm_uframes_t offset, sound_pcm_uframes_t size);
} sound_pcm_fast_ops_t;

struct _sound_pcm {
	char *name;
	sound_pcm_type_t type;
	sound_pcm_stream_t stream;
	int mode;
	void *private_data;

	int setup;
	int donot_close;	/* don't close this PCM */

	sound_pcm_access_t access;
	sound_pcm_format_t format;
	unsigned int channels;
	unsigned int rate;
	sound_pcm_uframes_t period_size;
	unsigned int period_time;
	unsigned int periods;
	sound_pcm_uframes_t avail_min;
	sound_pcm_uframes_t start_threshold;
	sound_pcm_uframes_t stop_threshold;
	sound_pcm_uframes_t silence_size;
	sound_pcm_uframes_t boundary;

	sound_pcm_uframes_t buffer_size;
	unsigned int buffer_time;
	unsigned int sample_bits;
	unsigned int frame_bits;

	sound_pcm_rbptr_t appl;
	sound_pcm_rbptr_t hw;

	unsigned int mmap_rw;	/* use always mmapped buffer */
	unsigned int mmap_shadow;	/* don't call actual mmap, use the mmaped buffer of the slave */
	sound_pcm_channel_info_t *mmap_channels;
	sound_pcm_channel_area_t *running_areas;
	sound_pcm_channel_area_t *stopped_areas;

	const sound_pcm_ops_t *ops;
	const sound_pcm_fast_ops_t *fast_ops;
	sound_pcm_t *op_arg;
	sound_pcm_t *fast_op_arg;

	pthread_mutex_t lock;
};

int sound_pcm_new(sound_pcm_t **pcmp, sound_pcm_type_t type, const char *name, sound_pcm_stream_t stream, int mode);
int sound_pcm_free(sound_pcm_t *pcm);

int sound_pcm_wait_nocheck(sound_pcm_t *pcm, int timeout);

void sound_pcm_areas_from_buf(sound_pcm_t *pcm, sound_pcm_channel_area_t *areas, void *buf);
void sound_pcm_areas_from_bufs(sound_pcm_t *pcm, sound_pcm_channel_area_t *areas, void **bufs);

int sound_pcm_mmap(sound_pcm_t *pcm);
int sound_pcm_munmap(sound_pcm_t *pcm);
void sound_pcm_set_hw_ptr(sound_pcm_t *pcm, volatile sound_pcm_uframes_t *hw_ptr, int fd, off_t offset);
void sound_pcm_set_appl_ptr(sound_pcm_t *pcm, volatile sound_pcm_uframes_t *appl_ptr, int fd, off_t offset);
void sound_pcm_link_hw_ptr(sound_pcm_t *pcm, sound_pcm_t *slave);
void sound_pcm_link_appl_ptr(sound_pcm_t *pcm, sound_pcm_t *slave);
void sound_pcm_unlink_hw_ptr(sound_pcm_t *pcm, sound_pcm_t *slave);
void sound_pcm_unlink_appl_ptr(sound_pcm_t *pcm, sound_pcm_t *slave);
void sound_pcm_mmap_appl_backward(sound_pcm_t *pcm, sound_pcm_uframes_t frames);
void sound_pcm_mmap_appl_forward(sound_pcm_t *pcm, sound_pcm_uframes_t frames);
void sound_pcm_mmap_hw_backward(sound_pcm_t *pcm, sound_pcm_uframes_t frames);
void sound_pcm_mmap_hw_forward(sound_pcm_t *pcm, sound_pcm_uframes_t frames);

typedef sound_pcm_sframes_t (*sound_pcm_xfer_areas_func_t)(sound_pcm_t *pcm,
						       const sound_pcm_channel_area_t *areas,
						       sound_pcm_uframes_t offset,
						       sound_pcm_uframes_t size);

sound_pcm_sframes_t sound_pcm_read_areas(sound_pcm_t *pcm, const sound_pcm_channel_area_t *areas,
				     sound_pcm_uframes_t offset, sound_pcm_uframes_t size,
				     sound_pcm_xfer_areas_func_t func);
sound_pcm_sframes_t sound_pcm_write_areas(sound_pcm_t *pcm, const sound_pcm_channel_area_t *areas,
				      sound_pcm_uframes_t offset, sound_pcm_uframes_t size,
				      sound_pcm_xfer_areas_func_t func);
static inline int sound_pcm_channel_info(sound_pcm_t *pcm, sound_pcm_channel_info_t *info)
{
	return pcm->ops->channel_info(pcm, info);
}
int sound_pcm_channel_info_shm(sound_pcm_t *pcm, sound_pcm_channel_info_t *info, int shmid);

/* return true if the PCM stream may wait to get avail_min space */
static inline int sound_pcm_may_wait_for_avail_min(sound_pcm_t *pcm, sound_pcm_uframes_t avail)
{
	if (avail >= pcm->avail_min)
		return 0;
	if (pcm->fast_ops->may_wait_for_avail_min)
		return pcm->fast_ops->may_wait_for_avail_min(pcm, avail);
	return 1;
}

static inline void sound_pcm_lock(sound_pcm_t *pcm)
{
	pthread_mutex_lock(&pcm->lock);
}

static inline void sound_pcm_unlock(sound_pcm_t *pcm)
{
	pthread_mutex_unlock(&pcm->lock);
}


/* locked versions */
int __sound_pcm_mmap_begin(sound_pcm_t *pcm, const sound_pcm_channel_area_t **areas,
			 sound_pcm_uframes_t *offset, sound_pcm_uframes_t *frames);
sound_pcm_sframes_t __sound_pcm_mmap_commit(sound_pcm_t *pcm,
					sound_pcm_uframes_t offset,
					sound_pcm_uframes_t frames);
int __sound_pcm_wait_in_lock(sound_pcm_t *pcm, int timeout);

static inline sound_pcm_sframes_t __sound_pcm_avail_update(sound_pcm_t *pcm)
{
	return pcm->fast_ops->avail_update(pcm->fast_op_arg);
}

static inline int __sound_pcm_start(sound_pcm_t *pcm)
{
	return pcm->fast_ops->start(pcm->fast_op_arg);
}

static inline sound_pcm_state_t __sound_pcm_state(sound_pcm_t *pcm)
{
	return pcm->fast_ops->state(pcm->fast_op_arg);
}

static inline int __sound_pcm_hwsync(sound_pcm_t *pcm)
{
	return pcm->fast_ops->hwsync(pcm->fast_op_arg);
}

/* handle special error cases */
static inline int sound_pcm_check_error(sound_pcm_t *pcm, int err)
{
	if (err == -EINTR) {
		switch (__sound_pcm_state(pcm)) {
		case SOUNDRV_PCM_STATE_XRUN:
			return -EPIPE;
		case SOUNDRV_PCM_STATE_SUSPENDED:
			return -ESTRPIPE;
		case SOUNDRV_PCM_STATE_DISCONNECTED:
			return -ENODEV;
		default:
			break;
		}
	}
	return err;
}

static inline sound_pcm_uframes_t sound_pcm_mmap_playback_avail(sound_pcm_t *pcm)
{
	sound_pcm_sframes_t avail;
	avail = *pcm->hw.ptr + pcm->buffer_size - *pcm->appl.ptr;
	if (avail < 0)
		avail += pcm->boundary;
	else if ((sound_pcm_uframes_t) avail >= pcm->boundary)
		avail -= pcm->boundary;
	return avail;
}

static inline sound_pcm_uframes_t sound_pcm_mmap_capture_avail(sound_pcm_t *pcm)
{
	sound_pcm_sframes_t avail;
	avail = *pcm->hw.ptr - *pcm->appl.ptr;
	if (avail < 0)
		avail += pcm->boundary;
	return avail;
}

static inline sound_pcm_uframes_t sound_pcm_mmap_avail(sound_pcm_t *pcm)
{
	if (pcm->stream == SOUND_PCM_STREAM_PLAYBACK)
		return sound_pcm_mmap_playback_avail(pcm);
	else
		return sound_pcm_mmap_capture_avail(pcm);
}

static inline sound_pcm_sframes_t sound_pcm_mmap_playback_hw_avail(sound_pcm_t *pcm)
{
	return pcm->buffer_size - sound_pcm_mmap_playback_avail(pcm);
}

static inline sound_pcm_sframes_t sound_pcm_mmap_capture_hw_avail(sound_pcm_t *pcm)
{
	return pcm->buffer_size - sound_pcm_mmap_capture_avail(pcm);
}

static inline const sound_pcm_channel_area_t *sound_pcm_mmap_areas(sound_pcm_t *pcm)
{
	if (pcm->stopped_areas && __sound_pcm_state(pcm) != SOUNDRV_PCM_STATE_RUNNING)
		return pcm->stopped_areas;
	return pcm->running_areas;
}

static inline sound_pcm_uframes_t sound_pcm_mmap_offset(sound_pcm_t *pcm)
{
	assert(pcm);
	return *pcm->appl.ptr % pcm->buffer_size;
}

static inline sound_pcm_uframes_t sound_pcm_mmap_hw_offset(sound_pcm_t *pcm)
{
	assert(pcm);
	return *pcm->hw.ptr % pcm->buffer_size;
}

static inline void *sound_pcm_channel_area_addr(const sound_pcm_channel_area_t *area, sound_pcm_uframes_t offset)
{
	unsigned int bitofs = area->first + area->step * offset;
	assert(bitofs % 8 == 0);
	return (char *) area->addr + bitofs / 8;
}

static inline unsigned int sound_pcm_channel_area_step(const sound_pcm_channel_area_t *area)
{
	assert(area->step % 8 == 0);
	return area->step / 8;
}

static inline sound_pcm_sframes_t _sound_pcm_writei(sound_pcm_t *pcm, const void *buffer, sound_pcm_uframes_t size)
{
	/* lock handled in the callback */
	return pcm->fast_ops->writei(pcm->fast_op_arg, buffer, size);
}

static inline sound_pcm_sframes_t _sound_pcm_readi(sound_pcm_t *pcm, void *buffer, sound_pcm_uframes_t size)
{
	/* lock handled in the callback */
	return pcm->fast_ops->readi(pcm->fast_op_arg, buffer, size);
}



static inline int muldiv(int a, int b, int c, int *r)
{
	int64_t n = (int64_t)a * b;
	int64_t v = n / c;
	if (v > INT_MAX) {
		*r = 0;
		return INT_MAX;
	}
	if (v < INT_MIN) {
		*r = 0;
		return INT_MIN;
	}
	*r = n % c;
	return v;
}

static inline int muldiv_down(int a, int b, int c)
{
	int64_t v = (int64_t)a * b / c;
	if (v > INT_MAX) {
		return INT_MAX;
	}
	if (v < INT_MIN) {
		return INT_MIN;
	}
	return v;
}

static inline int muldiv_near(int a, int b, int c)
{
	int r;
	int n = muldiv(a, b, c, &r);
	if (r >= (c + 1) / 2)
		n++;
	return n;
}

#define SOUND_PCM_HW_PARBIT_ACCESS	(1U << SOUND_PCM_HW_PARAM_ACCESS)
#define SOUND_PCM_HW_PARBIT_FORMAT	(1U << SOUND_PCM_HW_PARAM_FORMAT)
#define SOUND_PCM_HW_PARBIT_CHANNELS	(1U << SOUND_PCM_HW_PARAM_CHANNELS)
#define SOUND_PCM_HW_PARBIT_RATE		(1U << SOUND_PCM_HW_PARAM_RATE)
#define SOUND_PCM_HW_PARBIT_PERIOD_TIME	(1U << SOUND_PCM_HW_PARAM_PERIOD_TIME)
#define SOUND_PCM_HW_PARBIT_PERIOD_SIZE	(1U << SOUND_PCM_HW_PARAM_PERIOD_SIZE)
#define SOUND_PCM_HW_PARBIT_PERIODS	(1U << SOUND_PCM_HW_PARAM_PERIODS)
#define SOUND_PCM_HW_PARBIT_BUFFER_TIME	(1U << SOUND_PCM_HW_PARAM_BUFFER_TIME)
#define SOUND_PCM_HW_PARBIT_BUFFER_SIZE	(1U << SOUND_PCM_HW_PARAM_BUFFER_SIZE)
#define SOUND_PCM_HW_PARBIT_SAMPLE_BITS	(1U << SOUND_PCM_HW_PARAM_SAMPLE_BITS)
#define SOUND_PCM_HW_PARBIT_FRAME_BITS	(1U << SOUND_PCM_HW_PARAM_FRAME_BITS)
#define SOUND_PCM_HW_PARBIT_PERIOD_BYTES	(1U << SOUND_PCM_HW_PARAM_PERIOD_BYTES)
#define SOUND_PCM_HW_PARBIT_BUFFER_BYTES	(1U << SOUND_PCM_HW_PARAM_BUFFER_BYTES)

#define SOUND_PCM_ACCBIT_MMAP  ((1U << SOUND_PCM_ACCESS_MMAP_INTERLEAVED) | \
			      (1U << SOUND_PCM_ACCESS_MMAP_NONINTERLEAVED) | \
			      (1U << SOUND_PCM_ACCESS_MMAP_COMPLEX))
#define SOUND_PCM_ACCBIT_MMAPI (1U << SOUND_PCM_ACCESS_MMAP_INTERLEAVED)
#define SOUND_PCM_ACCBIT_MMAPN (1U << SOUND_PCM_ACCESS_MMAP_NONINTERLEAVED)
#define SOUND_PCM_ACCBIT_MMAPC (1U << SOUND_PCM_ACCESS_MMAP_COMPLEX)

#define SOUND_PCM_ACCBIT_SHM ((1U << SOUND_PCM_ACCESS_MMAP_INTERLEAVED) | \
			    (1U << SOUND_PCM_ACCESS_RW_INTERLEAVED) | \
			    (1U << SOUND_PCM_ACCESS_MMAP_NONINTERLEAVED) | \
			    (1U << SOUND_PCM_ACCESS_RW_NONINTERLEAVED))
#define SOUND_PCM_ACCBIT_SHMI ((1U << SOUND_PCM_ACCESS_MMAP_INTERLEAVED) | \
			     (1U << SOUND_PCM_ACCESS_RW_INTERLEAVED))
#define SOUND_PCM_ACCBIT_SHMN ((1U << SOUND_PCM_ACCESS_MMAP_NONINTERLEAVED) | \
			     (1U << SOUND_PCM_ACCESS_RW_NONINTERLEAVED))

#define SOUND_PCM_FMTBIT_LINEAR \
	((1U << SOUND_PCM_FORMAT_S8) | \
	 (1U << SOUND_PCM_FORMAT_U8) | \
	 (1U << SOUND_PCM_FORMAT_S16_LE) | \
	 (1U << SOUND_PCM_FORMAT_S16_BE) | \
	 (1U << SOUND_PCM_FORMAT_U16_LE) | \
	 (1U << SOUND_PCM_FORMAT_U16_BE) | \
	 (1U << SOUND_PCM_FORMAT_S24_LE) | \
	 (1U << SOUND_PCM_FORMAT_S24_BE) | \
	 (1U << SOUND_PCM_FORMAT_U24_LE) | \
	 (1U << SOUND_PCM_FORMAT_U24_BE) | \
	 (1U << SOUND_PCM_FORMAT_S32_LE) | \
	 (1U << SOUND_PCM_FORMAT_S32_BE) | \
	 (1U << SOUND_PCM_FORMAT_U32_LE) | \
	 (1U << SOUND_PCM_FORMAT_U32_BE))

void sound_pcm_hw_param_dump(const sound_pcm_hw_params_t *params, sound_pcm_hw_param_t var);

int sound_pcm_hw_param_empty(const sound_pcm_hw_params_t *params,
			   sound_pcm_hw_param_t var);
int sound_pcm_hw_param_always_eq(const sound_pcm_hw_params_t *params,
			       sound_pcm_hw_param_t var,
			       const sound_pcm_hw_params_t *params1);
int sound_pcm_hw_param_never_eq(const sound_pcm_hw_params_t *params,
			      sound_pcm_hw_param_t var,
			      const sound_pcm_hw_params_t *params1);
int sound_pcm_hw_param_get(const sound_pcm_hw_params_t *params, sound_pcm_hw_param_t var,
			 unsigned int *val, int *dir);
int sound_pcm_hw_param_get_min(const sound_pcm_hw_params_t *params,
			     sound_pcm_hw_param_t var,
			     unsigned int *val, int *dir);
int sound_pcm_hw_param_get_max(const sound_pcm_hw_params_t *params,
			     sound_pcm_hw_param_t var,
			     unsigned int *val, int *dir);

void _sound_pcm_hw_param_any(sound_pcm_hw_params_t *params, sound_pcm_hw_param_t var);
void _sound_pcm_hw_params_any(sound_pcm_hw_params_t *params);

int _sound_pcm_hw_param_set_mask(sound_pcm_hw_params_t *params,
			       sound_pcm_hw_param_t var, const sound_interval_t *mask);
int _sound_pcm_hw_param_set_range(sound_pcm_hw_params_t *params,
			        sound_pcm_hw_param_t var, const sound_interval_t *val);

int _sound_pcm_hw_param_set(sound_pcm_hw_params_t *param,
			  sound_pcm_hw_param_t var, unsigned int val, int dir);

int _sound_pcm_hw_param_set_min(sound_pcm_hw_params_t *params,
			      sound_pcm_hw_param_t var, unsigned int val, int dir);
int _sound_pcm_hw_param_set_max(sound_pcm_hw_params_t *params,
			      sound_pcm_hw_param_t var, unsigned int val, int dir);
int _sound_pcm_hw_param_set_minmax(sound_pcm_hw_params_t *params,
				 sound_pcm_hw_param_t var,
				 unsigned int min, int mindir,
				 unsigned int max, int maxdir);
int _sound_pcm_hw_param_refine(sound_pcm_hw_params_t *params,
			     sound_pcm_hw_param_t var,
			     const sound_pcm_hw_params_t *src);
int _sound_pcm_hw_params_refine(sound_pcm_hw_params_t *params,
			      unsigned int vars,
			      const sound_pcm_hw_params_t *src);

int sound_pcm_hw_param_refine_near(sound_pcm_t *pcm,
				 sound_pcm_hw_params_t *params,
				 sound_pcm_hw_param_t var,
				 const sound_pcm_hw_params_t *src);
int sound_pcm_hw_param_refine_multiple(sound_pcm_t *pcm,
				     sound_pcm_hw_params_t *params,
				     sound_pcm_hw_param_t var,
				     const sound_pcm_hw_params_t *src);

int sound_pcm_hw_refine(sound_pcm_t *pcm, sound_pcm_hw_params_t *params);
int sound_pcm_hw_refine_soft(sound_pcm_t *pcm, sound_pcm_hw_params_t *params);
int sound_pcm_hw_refine_slave(sound_pcm_t *pcm, sound_pcm_hw_params_t *params,
			    int (*cprepare)(sound_pcm_t *pcm,
					    sound_pcm_hw_params_t *params),
			    int (*cchange)(sound_pcm_t *pcm,
					   sound_pcm_hw_params_t *params,
					   sound_pcm_hw_params_t *sparams),
			    int (*sprepare)(sound_pcm_t *pcm,
					    sound_pcm_hw_params_t *params),
			    int (*schange)(sound_pcm_t *pcm,
					   sound_pcm_hw_params_t *params,
					   sound_pcm_hw_params_t *sparams),
			    int (*srefine)(sound_pcm_t *pcm,
					   sound_pcm_hw_params_t *sparams));
int sound_pcm_hw_params_slave(sound_pcm_t *pcm, sound_pcm_hw_params_t *params,
			    int (*cchange)(sound_pcm_t *pcm,
					   sound_pcm_hw_params_t *params,
					   sound_pcm_hw_params_t *sparams),
			    int (*sprepare)(sound_pcm_t *pcm,
					    sound_pcm_hw_params_t *params),
			    int (*schange)(sound_pcm_t *pcm,
					   sound_pcm_hw_params_t *params,
					   sound_pcm_hw_params_t *sparams),
			    int (*sparams)(sound_pcm_t *pcm,
					   sound_pcm_hw_params_t *sparams));

int sound_pcm_hw_param_set(sound_pcm_t *pcm, sound_pcm_hw_params_t *params,
			 sound_set_mode_t mode,
			 sound_pcm_hw_param_t var, unsigned int val, int dir);
int sound_pcm_hw_param_set_first(sound_pcm_t *pcm, sound_pcm_hw_params_t *params,
			       sound_pcm_hw_param_t var, unsigned int *rval, int *dir);
int sound_pcm_hw_param_set_last(sound_pcm_t *pcm, sound_pcm_hw_params_t *params,
			      sound_pcm_hw_param_t var, unsigned int *rval, int *dir);
int sound_pcm_hw_param_set_min(sound_pcm_t *pcm, sound_pcm_hw_params_t *params,
			     sound_set_mode_t mode,
			     sound_pcm_hw_param_t var, unsigned int *val, int *dir);
int sound_pcm_hw_param_set_max(sound_pcm_t *pcm, sound_pcm_hw_params_t *params,
			     sound_set_mode_t mode,
			     sound_pcm_hw_param_t var, unsigned int *val, int *dir);
int sound_pcm_hw_param_set_minmax(sound_pcm_t *pcm, sound_pcm_hw_params_t *params,
				sound_set_mode_t mode,
				sound_pcm_hw_param_t var,
				unsigned int *min, int *mindir,
				unsigned int *max, int *maxdir);
int sound_pcm_hw_param_set_near(sound_pcm_t *pcm, sound_pcm_hw_params_t *params,
			      sound_pcm_hw_param_t var,
			      unsigned int *val, int *dir);
int sound_pcm_hw_param_set_mask(sound_pcm_t *pcm, sound_pcm_hw_params_t *params,
			      sound_set_mode_t mode,
			      sound_pcm_hw_param_t var, const sound_interval_t *val);

int _sound_pcm_hw_params_internal(sound_pcm_t *pcm, sound_pcm_hw_params_t *params);

static inline int sound_pcm_hw_param_can_changed(
		sound_pcm_hw_params_t *params, sound_pcm_hw_param_t var)
{
#if 0
	return params->cmask & (1 << var);
#endif
	return 1;
}

static inline int sound_pcm_hw_param_change(
		sound_pcm_hw_params_t *params, sound_pcm_hw_param_t var)
{
#if 0
	params->cmask |= (1 << var);
#endif
	return 0;
}


#define SOUND_PCM_APPEND  (1<<8)

int sound_pcm_open_config(sound_pcm_t **pcmp, const sound_pcm_config_t *pcm_config,
		sound_pcm_stream_t stream, int mode);


/* PCM Helper Functions */
int sound_pcm_format_signed(sound_pcm_format_t format);
int sound_pcm_format_unsigned(sound_pcm_format_t format);
int sound_pcm_format_linear(sound_pcm_format_t format);
int sound_pcm_format_little_endian(sound_pcm_format_t format);
int sound_pcm_format_big_endian(sound_pcm_format_t format);
int sound_pcm_format_width(sound_pcm_format_t format);
sound_pcm_format_t sound_pcm_build_linear_format(int width, int pwidth, int unsignd, int big_endian);

#if 0
static inline sound_interval_t *hw_param_interval(sound_pcm_hw_params_t *params,
                                                sound_pcm_hw_param_t var)
{
        return &params->intervals[var];
}

int sound_interval_refine(sound_interval_t *i, const sound_interval_t *v);
#endif
sound_pcm_chmap_query_t **_sound_pcm_make_single_query_chmaps(const sound_pcm_chmap_t *src);

/* pcm semaphore Functions */
#if defined(CONFIG_KERNEL_FREERTOS)
typedef struct sound_semaphore {
	SemaphoreHandle_t handle;
} sound_sem_t;
#elif defined(CONFIG_RTTKERNEL)
typedef struct sound_semaphore {
	rt_sem_t handle;
} sound_sem_t;
#endif
int sound_pcm_sem_init(sound_sem_t *sem, uint32_t initCount, uint32_t maxCount);
void sound_pcm_sem_destroy(sound_sem_t *sem);
int sound_pcm_sem_wait(sound_sem_t *sem);
int sound_pcm_sem_post(sound_sem_t *sem);

#ifdef __cplusplus
}
#endif
#endif /* __AW_ALSA_PCM_LOCAL_H */

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
#ifndef __AW_ALSA_PCM_H
#define __AW_ALSA_PCM_H
#ifdef __cplusplus
extern "C" {
#endif

#include <sound_v2/sound_pcm_common.h>
#include <alloca.h>
#include <string.h>
#include <sys/types.h>
#include <hal_mutex.h>

#ifndef sound_malloc
#define sound_malloc(size)	calloc(1, size)
#endif

#ifndef sound_free
#define sound_free(ptr)		free(ptr)
#endif


/* debug option */
#define AW_ALSA_LOG_COLOR_NONE		"\e[0m"
#define AW_ALSA_LOG_COLOR_RED		"\e[31m"
#define AW_ALSA_LOG_COLOR_GREEN		"\e[32m"
#define AW_ALSA_LOG_COLOR_YELLOW	"\e[33m"
#define AW_ALSA_LOG_COLOR_BLUE		"\e[34m"

//#define AW_ALSA_DEBUG
#ifdef AW_ALSA_DEBUG
#define awalsa_debug(fmt, args...) \
	printf(AW_ALSA_LOG_COLOR_GREEN "[AWALSA_DEBUG][%s:%u]" fmt \
		AW_ALSA_LOG_COLOR_NONE, __FUNCTION__, __LINE__, ##args)
#else
#define awalsa_debug(fmt, args...)
#endif

//#define AW_ALSA_PRINT
#ifdef AW_ALSA_PRINT
#define awalsa_print(fmt, args...) \
	printf("[AWALSA_PRINT][%s:%u]" fmt, \
		__FUNCTION__, __LINE__, ##args)
#else
#define awalsa_print(fmt, args...)
#endif

#define awalsa_info(fmt, args...) \
	printf(AW_ALSA_LOG_COLOR_BLUE "[AWALSA_INFO][%s:%u]" fmt \
		AW_ALSA_LOG_COLOR_NONE, __FUNCTION__, __LINE__, ##args)

#define awalsa_err(fmt, args...) \
	printf(AW_ALSA_LOG_COLOR_RED "[AWALSA_ERR][%s:%u]" fmt \
		AW_ALSA_LOG_COLOR_NONE, __FUNCTION__, __LINE__, ##args)
/* end debug option */

/** Non blocking mode (flag for open mode) \hideinitializer */
//#define SOUND_PCM_NONBLOCK		0x00000001
/** Async notification (flag for open mode) \hideinitializer */
//#define SOUND_PCM_ASYNC			0x00000002
/** In an abort state (internal, not allowed for open) */
//#define SOUND_PCM_ABORT			0x00008000
/** Disable automatic (but not forced!) rate resamplinig */
#define SOUND_PCM_NO_AUTO_RESAMPLE	0x00010000
/** Disable automatic (but not forced!) channel conversion */
#define SOUND_PCM_NO_AUTO_CHANNELS	0x00020000
/** Disable automatic (but not forced!) format conversion */
#define SOUND_PCM_NO_AUTO_FORMAT		0x00040000
/** Disable soft volume control */
//#define SOUND_PCM_NO_SOFTVOL		0x00080000

typedef struct _sound_pcm sound_pcm_t;

enum _sound_pcm_type {
	/** Kernel level PCM */
	SOUND_PCM_TYPE_HW = 0,
	/** One or more linked PCM with exclusive access to selected
	    channels */
	SOUND_PCM_TYPE_MULTI,
	/** File writing plugin */
	SOUND_PCM_TYPE_FILE,
	/** Linear format conversion PCM */
	SOUND_PCM_TYPE_LINEAR,
	/** Rate conversion PCM */
	SOUND_PCM_TYPE_RATE,
	/** Attenuated static route PCM */
	SOUND_PCM_TYPE_ROUTE,
	/** Format adjusted PCM */
	SOUND_PCM_TYPE_PLUG,
	/** Direct Mixing plugin */
	SOUND_PCM_TYPE_DMIX,
	/** Direct Snooping plugin */
	SOUND_PCM_TYPE_DSNOOP,
	/** Soft volume plugin */
	SOUND_PCM_TYPE_SOFTVOL,
	/** External filter plugin */
	SOUND_PCM_TYPE_EXTPLUG,
	SOUND_PCM_TYPE_LAST = SOUND_PCM_TYPE_EXTPLUG
};

/** PCM type */
typedef enum _sound_pcm_type sound_pcm_type_t;

/** PCM stream (direction) */
// typedef enum _sound_pcm_stream {
//         /** Playback stream */
//         SOUND_PCM_STREAM_PLAYBACK = 0,
//         /** Capture stream */
//         SOUND_PCM_STREAM_CAPTURE,
//         SOUND_PCM_STREAM_LAST = SOUND_PCM_STREAM_CAPTURE
// } sound_pcm_stream_t;


/** PCM area specification */
typedef struct _sound_pcm_channel_area {
	/** base address of channel samples */
	void *addr;
	/** offset to first sample in bits */
	unsigned int first;
	/** samples distance in bits */
	unsigned int step;
} sound_pcm_channel_area_t;


int sound_pcm_open(sound_pcm_t **pcm, const char *name,
		sound_pcm_stream_t stream, int mode);
int sound_pcm_close(sound_pcm_t *pcm);

int sound_pcm_hw_params_malloc(sound_pcm_hw_params_t **ptr);
void sound_pcm_hw_params_free(sound_pcm_hw_params_t *obj);
int sound_pcm_hw_refine(sound_pcm_t *pcm, sound_pcm_hw_params_t *params);
int sound_pcm_hw_params(sound_pcm_t *pcm, sound_pcm_hw_params_t *params);
int sound_pcm_hw_free(sound_pcm_t *pcm);
int sound_pcm_reset(sound_pcm_t *pcm);
int sound_pcm_start(sound_pcm_t *pcm);
int sound_pcm_prepare(sound_pcm_t *pcm);
int sound_pcm_drop(sound_pcm_t *pcm);
int sound_pcm_drain(sound_pcm_t *pcm);
int sound_pcm_pause(sound_pcm_t *pcm, int enable);
int sound_pcm_recover(sound_pcm_t *pcm, int err, int silent);
int sound_pcm_resume(sound_pcm_t *pcm);
sound_pcm_state_t sound_pcm_state(sound_pcm_t *pcm);
sound_pcm_stream_t sound_pcm_stream(sound_pcm_t *pcm);
int sound_pcm_hwsync(sound_pcm_t *pcm);
int sound_pcm_delay(sound_pcm_t *pcm, sound_pcm_sframes_t *delayp);
sound_pcm_sframes_t sound_pcm_avail_update(sound_pcm_t *pcm);
int sound_pcm_hw_params_can_pause(const sound_pcm_hw_params_t *params);
sound_pcm_sframes_t sound_pcm_rewind(sound_pcm_t *pcm, sound_pcm_uframes_t frames);
int sound_pcm_link(sound_pcm_t *pcm1, sound_pcm_t *pcm2);
int sound_pcm_unlink(sound_pcm_t *pcm);

sound_pcm_sframes_t sound_pcm_writei(sound_pcm_t *pcm, const void *buffer, sound_pcm_uframes_t size);
sound_pcm_sframes_t sound_pcm_readi(sound_pcm_t *pcm, void *buffer, sound_pcm_uframes_t size);

int sound_pcm_wait(sound_pcm_t *pcm, int timeout);

int sound_pcm_dump(sound_pcm_t *pcm);
int sound_pcm_dump_setup(sound_pcm_t *pcm);
int sound_pcm_dump_hw_setup(sound_pcm_t *pcm);
int sound_pcm_dump_sw_setup(sound_pcm_t *pcm);
int sound_pcm_hw_params_dump(sound_pcm_hw_params_t *params);

/** channel positions */
enum sound_pcm_chmap_position {
	SOUND_CHMAP_UNKNOWN = 0,	/**< unspecified */
	SOUND_CHMAP_NA,		/**< N/A, silent */
	SOUND_CHMAP_MONO,		/**< mono stream */
	SOUND_CHMAP_FL,		/**< front left */
	SOUND_CHMAP_FR,		/**< front right */
	SOUND_CHMAP_RL,		/**< rear left */
	SOUND_CHMAP_RR,		/**< rear right */
	SOUND_CHMAP_FC,		/**< front center */
	SOUND_CHMAP_LFE,		/**< LFE */
	SOUND_CHMAP_SL,		/**< side left */
	SOUND_CHMAP_SR,		/**< side right */
	SOUND_CHMAP_RC,		/**< rear center */
	SOUND_CHMAP_FLC,		/**< front left center */
	SOUND_CHMAP_FRC,		/**< front right center */
	SOUND_CHMAP_RLC,		/**< rear left center */
	SOUND_CHMAP_RRC,		/**< rear right center */
	SOUND_CHMAP_FLW,		/**< front left wide */
	SOUND_CHMAP_FRW,		/**< front right wide */
	SOUND_CHMAP_FLH,		/**< front left high */
	SOUND_CHMAP_FCH,		/**< front center high */
	SOUND_CHMAP_FRH,		/**< front right high */
	SOUND_CHMAP_TC,		/**< top center */
	SOUND_CHMAP_TFL,		/**< top front left */
	SOUND_CHMAP_TFR,		/**< top front right */
	SOUND_CHMAP_TFC,		/**< top front center */
	SOUND_CHMAP_TRL,		/**< top rear left */
	SOUND_CHMAP_TRR,		/**< top rear right */
	SOUND_CHMAP_TRC,		/**< top rear center */
	SOUND_CHMAP_TFLC,		/**< top front left center */
	SOUND_CHMAP_TFRC,		/**< top front right center */
	SOUND_CHMAP_TSL,		/**< top side left */
	SOUND_CHMAP_TSR,		/**< top side right */
	SOUND_CHMAP_LLFE,		/**< left LFE */
	SOUND_CHMAP_RLFE,		/**< right LFE */
	SOUND_CHMAP_BC,		/**< bottom center */
	SOUND_CHMAP_BLC,		/**< bottom left center */
	SOUND_CHMAP_BRC,		/**< bottom right center */
	SOUND_CHMAP_LAST = SOUND_CHMAP_BRC,
};

/** channel map list type */
enum sound_pcm_chmap_type {
	SOUND_CHMAP_TYPE_NONE = 0,/**< unspecified channel position */
	SOUND_CHMAP_TYPE_FIXED,	/**< fixed channel position */
	SOUND_CHMAP_TYPE_VAR,	/**< freely swappable channel position */
	SOUND_CHMAP_TYPE_PAIRED,	/**< pair-wise swappable channel position */
	SOUND_CHMAP_TYPE_LAST = SOUND_CHMAP_TYPE_PAIRED, /**< last entry */
};

/** the channel map header */
typedef struct sound_pcm_chmap {
	unsigned int channels;	/**< number of channels */
	unsigned int pos[0];	/**< channel position array */
} sound_pcm_chmap_t;

/** the header of array items returned from sound_pcm_query_chmaps() */
typedef struct sound_pcm_chmap_query {
	enum sound_pcm_chmap_type type;	/**< channel map type */
	sound_pcm_chmap_t map;		/**< available channel map */
} sound_pcm_chmap_query_t;

sound_pcm_chmap_query_t **sound_pcm_query_chmaps(sound_pcm_t *pcm);
void sound_pcm_free_chmaps(sound_pcm_chmap_query_t **maps);
sound_pcm_chmap_t *sound_pcm_get_chmap(sound_pcm_t *pcm);
int sound_pcm_set_chmap(sound_pcm_t *pcm, const sound_pcm_chmap_t *map);

/* for pthread mutex lock */
typedef hal_mutex_t sound_pcm_mutex_t;
sound_pcm_mutex_t sound_thread_mutex_init(void);
int sound_thread_mutex_lock_timeout(sound_pcm_mutex_t mutex, long ms);
int sound_thread_mutex_lock(sound_pcm_mutex_t mutex);
int sound_thread_mutex_unlock(sound_pcm_mutex_t mutex);
void sound_thread_mutex_destroy(sound_pcm_mutex_t mutex);

size_t sound_pcm_hw_params_sizeof(void);
size_t sound_pcm_sw_params_sizeof(void);

#define __sound_alloca(ptr,type) do { *ptr = (type##_t *) alloca(type##_sizeof()); memset(*ptr, 0, type##_sizeof()); } while (0)
#define sound_pcm_hw_params_alloca(ptr) __sound_alloca(ptr, sound_pcm_hw_params)

int sound_pcm_hw_params_any(sound_pcm_t *pcm, sound_pcm_hw_params_t *params);

int sound_pcm_hw_params_set_format(sound_pcm_t *pcm, sound_pcm_hw_params_t *params, sound_pcm_format_t val);
int sound_pcm_hw_params_get_format(const sound_pcm_hw_params_t *params, sound_pcm_format_t *format);

int sound_pcm_hw_params_set_channels(sound_pcm_t *pcm, sound_pcm_hw_params_t *params, unsigned int val);
int sound_pcm_hw_params_get_channels(const sound_pcm_hw_params_t *params, unsigned int *val);

int sound_pcm_hw_params_set_rate(sound_pcm_t *pcm, sound_pcm_hw_params_t *params, unsigned int val, int dir);
int sound_pcm_hw_params_get_rate(const sound_pcm_hw_params_t *params, unsigned int *val, int *dir);

int sound_pcm_hw_params_set_period_time(sound_pcm_t *pcm, sound_pcm_hw_params_t *params, unsigned int us, int dir);
int sound_pcm_hw_params_set_period_time_near(sound_pcm_t *pcm, sound_pcm_hw_params_t *params, unsigned int *val, int *dir);
int sound_pcm_hw_params_get_period_time(const sound_pcm_hw_params_t *params, unsigned int *val, int *dir);

int sound_pcm_hw_params_set_period_size(sound_pcm_t *pcm, sound_pcm_hw_params_t *params, sound_pcm_uframes_t val, int dir);
int sound_pcm_hw_params_set_period_size_near(sound_pcm_t *pcm, sound_pcm_hw_params_t *params, sound_pcm_uframes_t *val, int *dir);
int sound_pcm_hw_params_get_period_size(const sound_pcm_hw_params_t *params, sound_pcm_uframes_t *val, int *dir);

int sound_pcm_hw_params_set_periods(sound_pcm_t *pcm, sound_pcm_hw_params_t *params, unsigned int val, int dir);
int sound_pcm_hw_params_set_periods_near(sound_pcm_t *pcm, sound_pcm_hw_params_t *params, unsigned int *val, int *dir);
int sound_pcm_hw_params_get_periods(const sound_pcm_hw_params_t *params, unsigned int *val, int *dir);

int sound_pcm_hw_params_set_buffer_size(sound_pcm_t *pcm, sound_pcm_hw_params_t *params, sound_pcm_uframes_t val);
int sound_pcm_hw_params_set_buffer_size_near(sound_pcm_t *pcm, sound_pcm_hw_params_t *params, sound_pcm_uframes_t *val);
int sound_pcm_hw_params_get_buffer_size(const sound_pcm_hw_params_t *params, sound_pcm_uframes_t *val);

int sound_pcm_hw_params_set_buffer_time(sound_pcm_t *pcm, sound_pcm_hw_params_t *params, unsigned int us);
int sound_pcm_hw_params_set_buffer_time_near(sound_pcm_t *pcm, sound_pcm_hw_params_t *params, unsigned int *val, int *dir);

int sound_pcm_hw_params_set_access(sound_pcm_t *pcm, sound_pcm_hw_params_t *params, sound_pcm_access_t access);
int sound_pcm_hw_params_get_access(const sound_pcm_hw_params_t *params, sound_pcm_access_t *access);

#define sound_pcm_sw_params_alloca(ptr) __sound_alloca(ptr, sound_pcm_sw_params)

int sound_pcm_sw_params_current(sound_pcm_t *pcm, sound_pcm_sw_params_t *params);
int sound_pcm_sw_params_set_start_threshold(sound_pcm_t *pcm, sound_pcm_sw_params_t *params, sound_pcm_uframes_t val);
int sound_pcm_sw_params_get_start_threshold(sound_pcm_sw_params_t *params, sound_pcm_uframes_t *val);
int sound_pcm_sw_params_set_stop_threshold(sound_pcm_t *pcm, sound_pcm_sw_params_t *params, sound_pcm_uframes_t val);
int sound_pcm_sw_params_get_stop_threshold(sound_pcm_sw_params_t *params, sound_pcm_uframes_t *val);
int sound_pcm_sw_params_set_silence_size(sound_pcm_t *pcm, sound_pcm_sw_params_t *params, sound_pcm_uframes_t val);
int sound_pcm_sw_params_get_silence_size(sound_pcm_sw_params_t *params, sound_pcm_uframes_t *val);
int sound_pcm_sw_params_set_avail_min(sound_pcm_t *pcm, sound_pcm_sw_params_t *params, sound_pcm_uframes_t val);
int sound_pcm_sw_params_get_avail_min(const sound_pcm_sw_params_t *params, sound_pcm_uframes_t *val);
int sound_pcm_sw_params_get_boundary(const sound_pcm_sw_params_t *params, sound_pcm_uframes_t *val);
int sound_pcm_sw_params(sound_pcm_t *pcm, sound_pcm_sw_params_t *params);


ssize_t sound_pcm_format_size(sound_pcm_format_t format, size_t samples);
sound_pcm_sframes_t sound_pcm_bytes_to_frames(sound_pcm_t *pcm, ssize_t bytes);
ssize_t sound_pcm_frames_to_bytes(sound_pcm_t *pcm, sound_pcm_sframes_t frames);

sound_pcm_type_t sound_pcm_type(sound_pcm_t *pcm);

const char *sound_pcm_type_name(sound_pcm_type_t type);
const char *sound_pcm_stream_name(sound_pcm_stream_t stream);
const char *sound_pcm_access_name(const sound_pcm_access_t _access);
const char *sound_pcm_format_name(const sound_pcm_format_t format);
const char *sound_pcm_state_name(const sound_pcm_state_t state);

/* Direct Access (MMAP) functions */
int sound_pcm_mmap_begin(sound_pcm_t *pcm,
		       const sound_pcm_channel_area_t **areas,
		       sound_pcm_uframes_t *offset,
		       sound_pcm_uframes_t *frames);
sound_pcm_sframes_t sound_pcm_mmap_commit(sound_pcm_t *pcm,
				      sound_pcm_uframes_t offset,
				      sound_pcm_uframes_t frames);
sound_pcm_sframes_t sound_pcm_mmap_writei(sound_pcm_t *pcm, const void *buffer, sound_pcm_uframes_t size);
sound_pcm_sframes_t sound_pcm_mmap_readi(sound_pcm_t *pcm, void *buffer, sound_pcm_uframes_t size);

int sound_pcm_area_silence(const sound_pcm_channel_area_t *dst_channel, sound_pcm_uframes_t dst_offset,
			 unsigned int samples, sound_pcm_format_t format);
int sound_pcm_areas_silence(const sound_pcm_channel_area_t *dst_channels, sound_pcm_uframes_t dst_offset,
			  unsigned int channels, sound_pcm_uframes_t frames, sound_pcm_format_t format);
int sound_pcm_area_copy(const sound_pcm_channel_area_t *dst_channel, sound_pcm_uframes_t dst_offset,
		      const sound_pcm_channel_area_t *src_channel, sound_pcm_uframes_t src_offset,
		      unsigned int samples, sound_pcm_format_t format);
int sound_pcm_areas_copy(const sound_pcm_channel_area_t *dst_channels, sound_pcm_uframes_t dst_offset,
		       const sound_pcm_channel_area_t *src_channels, sound_pcm_uframes_t src_offset,
		       unsigned int channels, sound_pcm_uframes_t frames, sound_pcm_format_t format);

#ifdef __cplusplus
}
#endif

#endif /* __AW_ALSA_PCM_H */

/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
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

#ifndef _SUNXI_HAL_SOUND_H_
#define _SUNXI_HAL_SOUND_H_

#include<stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SOUND_DEV_APPEND  (1<<8)
#define SOUND_DEV_NONBLOCK  (1<<9)

#define _SOUNDIOC(type,nr)   ((type << 16)|(nr))

#define SOUND_CTRL_CMD       (0x01000) /* control I/O ioctl commands */
#define SOUND_PCM_CMD        (0x01100) /* pcm driver ioctl commands */

#define sound_malloc(size)		calloc(1, size)
#define sound_calloc(n, size)	calloc(n, size)
#define sound_free(ptr)			free((void *)ptr)

enum sound_platform_type {
	SOUND_PLATFORM_TYPE_AAUDIO = 0,
	SOUND_PLATFORM_TYPE_I2S0,
	SOUND_PLATFORM_TYPE_I2S1,
	SOUND_PLATFORM_TYPE_I2S2,
	SOUND_PLATFORM_TYPE_I2S3,
	SOUND_PLATFORM_TYPE_DMIC,
	SOUND_PLATFORM_TYPE_OWA,
	SOUND_PLATFORM_TYPE_MAX,
};

#define SOUNDRV_LOG_COLOR_NONE		"\e[0m"
#define SOUNDRV_LOG_COLOR_RED		"\e[31m"
#define SOUNDRV_LOG_COLOR_GREEN	"\e[32m"
#define SOUNDRV_LOG_COLOR_YELLOW		"\e[33m"
#define SOUNDRV_LOG_COLOR_BLUE		"\e[34m"

#ifdef SOUNDRV_DEBUG
#define sound_debug(fmt, args...) \
	printf(SOUNDRV_LOG_COLOR_GREEN "[SOUND_DEBUG][%s:%d]" fmt \
		SOUNDRV_LOG_COLOR_NONE, __FUNCTION__, __LINE__, ##args)
#else
#define sound_debug(fmt, args...)
#endif

#if 0
#define sound_info(fmt, args...) \
	printf(SOUNDRV_LOG_COLOR_BLUE "[SOUND_INFO][%s:%d]" fmt \
		SOUNDRV_LOG_COLOR_NONE, __FUNCTION__, __LINE__, ##args)
#else
#define sound_info(fmt, args...)
#endif

#define sound_err(fmt, args...) \
	printf(SOUNDRV_LOG_COLOR_RED "[SOUND_ERR][%s:%d]" fmt \
		SOUNDRV_LOG_COLOR_NONE, __FUNCTION__, __LINE__, ##args)
#if 0
#define sound_lock_debug(fmt, args...) \
	printf(SOUNDRV_LOG_COLOR_RED "[SOUND_LOCK_DEBUG][%s:%u]" fmt \
		SOUNDRV_LOG_COLOR_NONE, __FUNCTION__, __LINE__, ##args)
#else
#define sound_lock_debug(fmt, args...)
#endif
/*****************************************************************************
 *																			 *
 *			   Digital Audio (PCM) interface - Sunxipcm??				 *
 *																			 *
 *****************************************************************************/

typedef unsigned long sound_pcm_uframes_t;
typedef signed long sound_pcm_sframes_t;

typedef union sound_interval sound_interval_t;
union sound_interval {
	struct {
		uint32_t min;
		uint32_t max;
		int openmin;	/* whether the interval is left-open */
		int openmax;	/* whether the interval is right-open */
		int integer;	/* whether the value is integer or not */
		int empty;
	} range;
	uint32_t mask;
};

typedef long sound_pcm_sframes_t;
typedef unsigned long sound_pcm_uframes_t;

/** PCM stream (direction) */
typedef enum _sound_pcm_stream {
    /** Playback stream */
    SOUND_PCM_STREAM_PLAYBACK = 0,
    /** Capture stream */
    SOUND_PCM_STREAM_CAPTURE,
    SOUND_PCM_STREAM_LAST = SOUND_PCM_STREAM_CAPTURE
} sound_pcm_stream_t;

/** PCM access type */
typedef enum _sound_pcm_access {
	/** mmap access with simple interleaved channels */
	SOUND_PCM_ACCESS_MMAP_INTERLEAVED = 0,
	/** mmap access with simple non interleaved channels */
	SOUND_PCM_ACCESS_MMAP_NONINTERLEAVED,
	/** mmap access with complex placement */
	SOUND_PCM_ACCESS_MMAP_COMPLEX,
	/** sound_pcm_readi/sound_pcm_writei access */
	SOUND_PCM_ACCESS_RW_INTERLEAVED,
	/** sound_pcm_readn/sound_pcm_writen access */
	SOUND_PCM_ACCESS_RW_NONINTERLEAVED,
	SOUND_PCM_ACCESS_LAST = SOUND_PCM_ACCESS_RW_NONINTERLEAVED
} sound_pcm_access_t;

/** PCM sample format */
typedef enum _sound_pcm_format {
	/** Unknown */
	SOUND_PCM_FORMAT_UNKNOWN = -1,
	/** Signed 8 bit */
	SOUND_PCM_FORMAT_S8 = 0,
	/** Unsigned 8 bit */
	SOUND_PCM_FORMAT_U8,
	/** Signed 16 bit Little Endian */
	SOUND_PCM_FORMAT_S16_LE,
	/** Signed 16 bit Big Endian */
	SOUND_PCM_FORMAT_S16_BE,
	/** Unsigned 16 bit Little Endian */
	SOUND_PCM_FORMAT_U16_LE,
	/** Unsigned 16 bit Big Endian */
	SOUND_PCM_FORMAT_U16_BE,
	/** Signed 24 bit Little Endian using low three bytes in 32-bit word */
	SOUND_PCM_FORMAT_S24_LE,
	/** Signed 24 bit Big Endian using low three bytes in 32-bit word */
	SOUND_PCM_FORMAT_S24_BE,
	/** Unsigned 24 bit Little Endian using low three bytes in 32-bit word */
	SOUND_PCM_FORMAT_U24_LE,
	/** Unsigned 24 bit Big Endian using low three bytes in 32-bit word */
	SOUND_PCM_FORMAT_U24_BE,
	/** Signed 32 bit Little Endian */
	SOUND_PCM_FORMAT_S32_LE,
	/** Signed 32 bit Big Endian */
	SOUND_PCM_FORMAT_S32_BE,
	/** Unsigned 32 bit Little Endian */
	SOUND_PCM_FORMAT_U32_LE,
	/** Unsigned 32 bit Big Endian */
	SOUND_PCM_FORMAT_U32_BE,
	/** Float 32 bit Little Endian, Range -1.0 to 1.0 */
	SOUND_PCM_FORMAT_FLOAT_LE,
	/** Float 32 bit Big Endian, Range -1.0 to 1.0 */
	SOUND_PCM_FORMAT_FLOAT_BE,
	/** Float 64 bit Little Endian, Range -1.0 to 1.0 */
	SOUND_PCM_FORMAT_FLOAT64_LE,
	/** Float 64 bit Big Endian, Range -1.0 to 1.0 */
	SOUND_PCM_FORMAT_FLOAT64_BE,
	/** IEC-958 Little Endian */
	SOUND_PCM_FORMAT_IEC958_SUBFRAME_LE,
	/** IEC-958 Big Endian */
	SOUND_PCM_FORMAT_IEC958_SUBFRAME_BE,
	/** Mu-Law */
	SOUND_PCM_FORMAT_MU_LAW,
	/** A-Law */
	SOUND_PCM_FORMAT_A_LAW,
	/** Ima-ADPCM */
	SOUND_PCM_FORMAT_IMA_ADPCM,
	/** MPEG */
	SOUND_PCM_FORMAT_MPEG,
	/** GSM */
	SOUND_PCM_FORMAT_GSM,
	/** Signed 20bit Little Endian in 4bytes format, LSB justified */
	SOUND_PCM_FORMAT_S20_LE,
	/** Signed 20bit Big Endian in 4bytes format, LSB justified */
	SOUND_PCM_FORMAT_S20_BE,
	/** Unsigned 20bit Little Endian in 4bytes format, LSB justified */
	SOUND_PCM_FORMAT_U20_LE,
	/** Unsigned 20bit Big Endian in 4bytes format, LSB justified */
	SOUND_PCM_FORMAT_U20_BE,
	/** Special */
	SOUND_PCM_FORMAT_SPECIAL = 31,
	/** Signed 24bit Little Endian in 3bytes format */
	SOUND_PCM_FORMAT_S24_3LE = 32,
	/** Signed 24bit Big Endian in 3bytes format */
	SOUND_PCM_FORMAT_S24_3BE,
	/** Unsigned 24bit Little Endian in 3bytes format */
	SOUND_PCM_FORMAT_U24_3LE,
	/** Unsigned 24bit Big Endian in 3bytes format */
	SOUND_PCM_FORMAT_U24_3BE,
	/** Signed 20bit Little Endian in 3bytes format */
	SOUND_PCM_FORMAT_S20_3LE,
	/** Signed 20bit Big Endian in 3bytes format */
	SOUND_PCM_FORMAT_S20_3BE,
	/** Unsigned 20bit Little Endian in 3bytes format */
	SOUND_PCM_FORMAT_U20_3LE,
	/** Unsigned 20bit Big Endian in 3bytes format */
	SOUND_PCM_FORMAT_U20_3BE,
	/** Signed 18bit Little Endian in 3bytes format */
	SOUND_PCM_FORMAT_S18_3LE,
	/** Signed 18bit Big Endian in 3bytes format */
	SOUND_PCM_FORMAT_S18_3BE,
	/** Unsigned 18bit Little Endian in 3bytes format */
	SOUND_PCM_FORMAT_U18_3LE,
	/** Unsigned 18bit Big Endian in 3bytes format */
	SOUND_PCM_FORMAT_U18_3BE,
	/* G.723 (ADPCM) 24 kbit/s, 8 samples in 3 bytes */
	SOUND_PCM_FORMAT_G723_24,
	/* G.723 (ADPCM) 24 kbit/s, 1 sample in 1 byte */
	SOUND_PCM_FORMAT_G723_24_1B,
	/* G.723 (ADPCM) 40 kbit/s, 8 samples in 3 bytes */
	SOUND_PCM_FORMAT_G723_40,
	/* G.723 (ADPCM) 40 kbit/s, 1 sample in 1 byte */
	SOUND_PCM_FORMAT_G723_40_1B,
	/* Direct Stream Digital (DSD) in 1-byte samples (x8) */
	SOUND_PCM_FORMAT_DSD_U8,
	/* Direct Stream Digital (DSD) in 2-byte samples (x16) */
	SOUND_PCM_FORMAT_DSD_U16_LE,
	/* Direct Stream Digital (DSD) in 4-byte samples (x32) */
	SOUND_PCM_FORMAT_DSD_U32_LE,
	/* Direct Stream Digital (DSD) in 2-byte samples (x16) */
	SOUND_PCM_FORMAT_DSD_U16_BE,
	/* Direct Stream Digital (DSD) in 4-byte samples (x32) */
	SOUND_PCM_FORMAT_DSD_U32_BE,
	SOUND_PCM_FORMAT_LAST = SOUND_PCM_FORMAT_DSD_U32_BE,
/* only support little endian */
/** Signed 16 bit CPU endian */
	SOUND_PCM_FORMAT_S16 = SOUND_PCM_FORMAT_S16_LE,
	/** Unsigned 16 bit CPU endian */
	SOUND_PCM_FORMAT_U16 = SOUND_PCM_FORMAT_U16_LE,
	/** Signed 24 bit CPU endian */
	SOUND_PCM_FORMAT_S24 = SOUND_PCM_FORMAT_S24_LE,
	/** Unsigned 24 bit CPU endian */
	SOUND_PCM_FORMAT_U24 = SOUND_PCM_FORMAT_U24_LE,
	/** Signed 32 bit CPU endian */
	SOUND_PCM_FORMAT_S32 = SOUND_PCM_FORMAT_S32_LE,
	/** Unsigned 32 bit CPU endian */
	SOUND_PCM_FORMAT_U32 = SOUND_PCM_FORMAT_U32_LE,
} sound_pcm_format_t;

/** PCM state */
typedef enum _sound_pcm_state {
	/** Open */
	SOUNDRV_PCM_STATE_OPEN = 0,
	/** Setup installed */
	SOUNDRV_PCM_STATE_SETUP,
	/** Ready to start */
	SOUNDRV_PCM_STATE_PREPARED,
	/** Running */
	SOUNDRV_PCM_STATE_RUNNING,
	/** Stopped: underrun (playback) or overrun (capture) detected */
	SOUNDRV_PCM_STATE_XRUN,
	/** Draining: running (playback) or stopped (capture) */
	SOUNDRV_PCM_STATE_DRAINING,
	/** Paused */
	SOUNDRV_PCM_STATE_PAUSED,
	/** Hardware is suspended */
	SOUNDRV_PCM_STATE_SUSPENDED,
	/** Hardware is disconnected */
	SOUNDRV_PCM_STATE_DISCONNECTED,
	SOUNDRV_PCM_STATE_LAST = SOUNDRV_PCM_STATE_DISCONNECTED
} sound_pcm_state_t;

struct sound_pcm_info {
	unsigned int device;		/* RO/WR (control): device number */
	unsigned int subdevice;		/* RO/WR (control): subdevice number */
	int stream;			/* RO/WR (control): stream direction */
	int card;			/* R: card number */
	unsigned char id[64];		/* ID (user selectable) */
	unsigned char name[64];		/* name of this device */
	unsigned char subname[32];	/* subdevice name */
	unsigned int subdevices_count;
	unsigned int subdevices_avail;
};

typedef int sound_pcm_hw_param_t;
#define SOUND_PCM_HW_PARAM_ACCESS		0
#define SOUND_PCM_HW_PARAM_FORMAT		1
#define SOUND_PCM_HW_PARAM_FIRST_MASK	SOUND_PCM_HW_PARAM_ACCESS
#define SOUND_PCM_HW_PARAM_LAST_MASK	SOUND_PCM_HW_PARAM_FORMAT
#define SOUND_PCM_HW_PARAM_SAMPLE_BITS	2
#define SOUND_PCM_HW_PARAM_FRAME_BITS	3
#define SOUND_PCM_HW_PARAM_CHANNELS	4
#define SOUND_PCM_HW_PARAM_RATE		5
#define SOUND_PCM_HW_PARAM_PERIOD_TIME	6
#define SOUND_PCM_HW_PARAM_PERIOD_SIZE	7
#define SOUND_PCM_HW_PARAM_PERIOD_BYTES	8
#define SOUND_PCM_HW_PARAM_PERIODS	9
#define SOUND_PCM_HW_PARAM_BUFFER_TIME	10
#define SOUND_PCM_HW_PARAM_BUFFER_SIZE	11
#define SOUND_PCM_HW_PARAM_BUFFER_BYTES	12
#define SOUND_PCM_HW_PARAM_FIRST_RANGE SOUND_PCM_HW_PARAM_SAMPLE_BITS
#define SOUND_PCM_HW_PARAM_LAST_RANGE SOUND_PCM_HW_PARAM_BUFFER_BYTES
#define SOUND_PCM_HW_PARAM_FIRST_INTERVAL SOUND_PCM_HW_PARAM_ACCESS
#define SOUND_PCM_HW_PARAM_LAST_INTERVAL SOUND_PCM_HW_PARAM_BUFFER_BYTES

typedef struct sound_pcm_hw_params {
	union sound_interval intervals[SOUND_PCM_HW_PARAM_LAST_INTERVAL -
				SOUND_PCM_HW_PARAM_FIRST_INTERVAL + 1];
	unsigned int can_paused;

	uint32_t rmask; 	/* W: requested masks */
	uint32_t cmask; 	/* R: changed masks */
} sound_pcm_hw_params_t;

typedef struct sound_pcm_sw_params {
	sound_pcm_uframes_t avail_min;			/* min avail frames for wakeup */
	sound_pcm_uframes_t start_threshold;		/* min hw_avail frames for automatic start */
	sound_pcm_uframes_t stop_threshold;		/* min avail frames for automatic stop */
	sound_pcm_uframes_t silence_size; 		/* silence block size */
	sound_pcm_uframes_t boundary; 			/* pointers wrap point */
} sound_pcm_sw_params_t;

typedef struct _sound_pcm_channel_info {
	unsigned int channel;
	void *addr; 		/* base address of channel samples */
	unsigned int first; 	/* offset to first sample in bits */
	unsigned int step;		/* samples distance in bits */
	enum { SOUND_PCM_AREA_MMAP, SOUND_PCM_AREA_LOCAL } type;
} sound_pcm_channel_info_t;

struct sound_pcm_mmap_status {
	sound_pcm_state_t state;
	sound_pcm_uframes_t hw_ptr;
	sound_pcm_state_t state_suspend;
};

struct sound_pcm_mmap_control {
	sound_pcm_uframes_t appl_ptr;
	sound_pcm_uframes_t avail_min;
};

#define SOUNDRV_PCM_SYNC_PTR_HWSYNC       (1<<0)  /* execute hwsync */
#define SOUNDRV_PCM_SYNC_PTR_APPL         (1<<1)  /* get appl_ptr from driver (r/w op) */
#define SOUNDRV_PCM_SYNC_PTR_AVAIL_MIN    (1<<2)  /* get avail_min from driver */

struct sound_pcm_sync_ptr {
	unsigned int flags;
	union {
		struct sound_pcm_mmap_status status;
		unsigned char reserved[64];
	} s;
	union {
		struct sound_pcm_mmap_control control;
		unsigned char reserved[64];
	} c;
};


#define SOUND_PCM(nr)						_SOUNDIOC(SOUND_PCM_CMD,(nr))

#define SOUND_CTRL_PCM_INFO				SOUND_PCM(0)
#define SOUND_CTRL_PCM_HW_REFINE			SOUND_PCM(1)
#define SOUND_CTRL_PCM_HW_PARAMS			SOUND_PCM(2)
#define SOUND_CTRL_PCM_HW_FREE			SOUND_PCM(3)
#define SOUND_CTRL_PCM_SW_PARAMS			SOUND_PCM(4)
#define SOUND_CTRL_PCM_PREPARE			SOUND_PCM(5)
#define SOUND_CTRL_PCM_RESET				SOUND_PCM(6)
#define SOUND_CTRL_PCM_START				SOUND_PCM(7)
#define SOUND_CTRL_PCM_RESUME				SOUND_PCM(8)
#define SOUND_CTRL_PCM_HWSYNC				SOUND_PCM(9)
#define SOUND_CTRL_PCM_DELAY				SOUND_PCM(10)
#define SOUND_CTRL_PCM_SYNC_PTR			SOUND_PCM(11)
#define SOUND_CTRL_PCM_DRAIN				SOUND_PCM(12)
#define SOUND_CTRL_PCM_DROP				SOUND_PCM(13)
#define SOUND_CTRL_PCM_PAUSE				SOUND_PCM(14)
#define SOUND_CTRL_PCM_WAIT				SOUND_PCM(15)
#define SOUND_CTRL_PCM_CHANNEL_INFO		SOUND_PCM(16)
#define SOUND_CTRL_PCM_MMAP_DCACHE_UPDATE SOUND_PCM(17)
#define SOUND_CTRL_PCM_DSLEEP_INIT		SOUND_PCM(18)
#define SOUND_CTRL_PCM_DSLEEP_RELEASE		SOUND_PCM(19)
#define SOUND_CTRL_PCM_REWIND				SOUND_PCM(20)

/****************************************************************************
 *																			*
 *		  Section for driver control interface - control?			*
 *																			*
 ****************************************************************************/

struct sound_control_card_info {
	int card;			/* card number */
	char name[32];		/* Short name of soundcard */
};

typedef int sound_control_part_type_t;

#define SOUND_CTRL_PART_TYPE_INTEGER ((sound_control_part_type_t)0)
#define SOUND_CTRL_PART_TYPE_ENUMERATED ((sound_control_part_type_t)1)
#define SOUND_CTRL_PART_TYPE_LAST SOUND_CTRL_PART_TYPE_ENUMERATED

struct sound_control_part_list {
	unsigned int offset;	/* W: first element ID to get */
	unsigned int space;		/* W: count of element IDs to get */
	unsigned int count;		/* R: count of all elements */
	unsigned int *numid; 	/* R: IDs */
};

struct sound_control_info {
	unsigned int id;
	sound_control_part_type_t type;
	const char *name;
	unsigned long value;
	unsigned long *values;
	int min;
	int max;
	int count;
	/* for enum */
	unsigned int items;
	const char *const *texts;
	unsigned long *private_data;
};
typedef struct sound_control_info sound_control_info_t;

#define SOUND_CTRL(nr)			_SOUNDIOC(SOUND_CTRL_CMD,(nr))
#define SOUND_CTRL_CARD_INFO		SOUND_CTRL(0)
#define SOUND_CTRL_PART_LIST		SOUND_CTRL(1)
#define SOUND_CTRL_PART_INFO		SOUND_CTRL(2)
#define SOUND_CTRL_PART_READ		SOUND_CTRL(3)
#define SOUND_CTRL_PART_WRITE		SOUND_CTRL(4)
#define SOUND_CTRL_PART_ADD		SOUND_CTRL(5)
#define SOUND_CTRL_PART_REMOVE	SOUND_CTRL(6)

typedef struct sound_sys_device sound_device_t;

int sound_device_open(sound_device_t **sound_dev, const char *devname, int mode);
int sound_device_close(sound_device_t *sound_dev);
int sound_device_control(sound_device_t *sound_dev, unsigned int cmd, unsigned long arg);
int sound_device_write(sound_device_t *sound_dev, const void *buffer, sound_pcm_uframes_t size);
int sound_device_read(sound_device_t *sound_dev, void *buffer, sound_pcm_uframes_t size);

struct sound_info_buffer {
	char *buffer;		/* pointer to begin of buffer */
	unsigned int len;	/* total length of buffer */
	int error;		/* error code */
};

void sound_cards_info(struct sound_info_buffer *buf);
void sound_pcm_read(struct sound_info_buffer *buf);
int sound_dataflow_info_read(int card_num, int device, int subdevice, int stream,
							 struct sound_info_buffer *buf);

#ifdef __cplusplus
}
#endif

#endif /* _SUNXI_HAL_SOUND_H_ */


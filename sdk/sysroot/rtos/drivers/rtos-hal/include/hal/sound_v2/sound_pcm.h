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

#ifndef __SOUND_PCM_H
#define __SOUND_PCM_H

#include <unistd.h>
#include <hal_sem.h>
#include <hal_mutex.h>
#include "hal_atomic.h"
#include <sunxi_hal_common.h>
#include "aw_list.h"

#include <sunxi_hal_sound.h>
#include <sound_v2/sound_core.h>
#include <sound_v2/sound_pcm_common.h>

struct sound_pcm_hardware {
	uint32_t info;
	uint64_t formats;
	uint32_t rates;
	uint32_t rate_min;
	uint32_t rate_max;
	uint32_t channels_min;
	uint32_t channels_max;
	uint32_t buffer_bytes_max;
	uint32_t period_bytes_min;
	uint32_t period_bytes_max;
	uint32_t periods_min;
	uint32_t periods_max;
};

struct sound_pcm;
struct sound_pcm_data;
struct sound_pcm_dataflow;
typedef void* __iomem;
typedef __iomem dma_addr_t;

#define SOUND_CMA_BLOCK_BYTES	1024
#define SOUND_CMA_MAX_KBYTES	128
#define SOUND_CMA_MIN_KBYTES	4
#define SOUND_CMA_MAX_BYTES	(SOUND_CMA_BLOCK_BYTES * SOUND_CMA_MAX_KBYTES)
#define SOUND_CMA_MIN_BYTES	(SOUND_CMA_BLOCK_BYTES * SOUND_CMA_MIN_KBYTES)

#define SOUND_FIFO_SIZE		128

struct sunxi_dma_params {
	char *name;
	dma_addr_t dma_addr;
	uint32_t src_maxburst;
	uint32_t dst_maxburst;
	uint8_t dma_drq_type_num;

	/* max buffer set (value must be (2^n)Kbyte) */
	size_t cma_kbytes;
};

struct sound_dma_buffer {
	dma_addr_t addr;
	size_t bytes;
};

struct sound_pcm_hw_constrains {
	union sound_interval intervals[SOUND_PCM_HW_PARAM_LAST_INTERVAL - SOUND_PCM_HW_PARAM_FIRST_INTERVAL + 1];
	unsigned int rules_num;
	unsigned int rules_all;
	struct sound_pcm_hw_rule *rules;
};

static inline sound_interval_t *constrs_interval(struct sound_pcm_hw_constrains *constrs,
		sound_pcm_hw_param_t var)
{
	return &constrs->intervals[var - SOUND_PCM_HW_PARAM_FIRST_INTERVAL];
}

#define is_range_of_hw_constrains(param, cons, type) \
((param->intervals[type].range.min >= cons->intervals[type].range.min) && \
(param->intervals[type].range.min <= cons->intervals[type].range.max))

struct sound_pcm_ops {
	int (*open)(struct sound_pcm_dataflow *dataflow);
	int (*close)(struct sound_pcm_dataflow *dataflow);
	int (*ioctl)(struct sound_pcm_dataflow * dataflow,
		     unsigned int cmd, void *arg);
	int (*hw_params)(struct sound_pcm_dataflow *dataflow,
			struct sound_pcm_hw_params *params);
	int (*hw_free)(struct sound_pcm_dataflow *dataflow);
	int (*prepare)(struct sound_pcm_dataflow *dataflow);
	int (*trigger)(struct sound_pcm_dataflow *dataflow, int cmd);
	sound_pcm_uframes_t (*pointer)(struct sound_pcm_dataflow *dataflow);
	int (*copy)(struct sound_pcm_dataflow *dataflow, int channel,
			sound_pcm_uframes_t pos,
			void *buf, sound_pcm_uframes_t count);
};

#define SOUND_SOC_DAIFMT_I2S              1 /* I2S mode */
#define SOUND_SOC_DAIFMT_RIGHT_J          2 /* Right Justified mode */
#define SOUND_SOC_DAIFMT_LEFT_J           3 /* Left Justified mode */
#define SOUND_SOC_DAIFMT_DSP_A            4 /* L data MSB after FRM LRC */
#define SOUND_SOC_DAIFMT_DSP_B            5 /* L data MSB during FRM LRC */
#define SOUND_SOC_DAIFMT_AC97             6 /* AC97 */
#define SOUND_SOC_DAIFMT_PDM              7 /* Pulse density modulation */

										 /* left and right justified also known as MSB and LSB respectively */
#define SOUND_SOC_DAIFMT_MSB              SOUND_SOC_DAIFMT_LEFT_J
#define SOUND_SOC_DAIFMT_LSB              SOUND_SOC_DAIFMT_RIGHT_J

#define	SOUND_SOC_DAIFMT_SIG_SHIFT		8
#define	SOUND_SOC_DAIFMT_MASTER_SHIFT		12

#define SOUND_SOC_DAIFMT_NB_NF            (1 << 8) /* normal bit clock + frame */
#define SOUND_SOC_DAIFMT_NB_IF            (2 << 8) /* normal BCLK + inv FRM */
#define SOUND_SOC_DAIFMT_IB_NF            (3 << 8) /* invert BCLK + nor FRM */
#define SOUND_SOC_DAIFMT_IB_IF            (4 << 8) /* invert BCLK + FRM */

#define SOUND_SOC_DAIFMT_CBM_CFM          (1 << 12) /* codec clk & FRM master */
#define SOUND_SOC_DAIFMT_CBS_CFM          (2 << 12) /* codec clk slave & FRM master */
#define SOUND_SOC_DAIFMT_CBM_CFS          (3 << 12) /* codec clk master & frame slave */
#define SOUND_SOC_DAIFMT_CBS_CFS          (4 << 12) /* codec clk & FRM slave */

#define SOUND_SOC_DAIFMT_FORMAT_MASK      0x000f
#define SOUND_SOC_DAIFMT_CLOCK_MASK       0x00f0
#define SOUND_SOC_DAIFMT_INV_MASK         0x0f00
#define SOUND_SOC_DAIFMT_MASTER_MASK      0xf000


#define SOUNDRV_PCM_IOCTL1_RESET		0
										 /* 1 is absent slot. */
#define SOUNDRV_PCM_IOCTL1_CHANNEL_INFO	2
										 /* 3 is absent slot. */
#define SOUNDRV_PCM_IOCTL1_FIFO_SIZE	4

#define SOUNDRV_PCM_TRIGGER_STOP          0
#define SOUNDRV_PCM_TRIGGER_START         1
#define SOUNDRV_PCM_TRIGGER_PAUSE_PUSH    3
#define SOUNDRV_PCM_TRIGGER_PAUSE_RELEASE 4
#define SOUNDRV_PCM_TRIGGER_SUSPEND       5
#define SOUNDRV_PCM_TRIGGER_RESUME        6
#define SOUNDRV_PCM_TRIGGER_DRAIN         7

#define SOUNDRV_PCM_RATE_5512             (1<<0)          /* 5512Hz */
#define SOUNDRV_PCM_RATE_8000             (1<<1)          /* 8000Hz */
#define SOUNDRV_PCM_RATE_11025            (1<<2)          /* 11025Hz */
#define SOUNDRV_PCM_RATE_16000            (1<<3)          /* 16000Hz */
#define SOUNDRV_PCM_RATE_22050            (1<<4)          /* 22050Hz */
#define SOUNDRV_PCM_RATE_32000            (1<<5)          /* 32000Hz */
#define SOUNDRV_PCM_RATE_44100            (1<<6)          /* 44100Hz */
#define SOUNDRV_PCM_RATE_48000            (1<<7)          /* 48000Hz */
#define SOUNDRV_PCM_RATE_64000            (1<<8)          /* 64000Hz */
#define SOUNDRV_PCM_RATE_88200            (1<<9)          /* 88200Hz */
#define SOUNDRV_PCM_RATE_96000            (1<<10)         /* 96000Hz */
#define SOUNDRV_PCM_RATE_176400           (1<<11)         /* 176400Hz */
#define SOUNDRV_PCM_RATE_192000           (1<<12)         /* 192000Hz */

#define SOUNDRV_PCM_RATE_CONTINUOUS       (1<<30)         /* continuous range */
#define SOUNDRV_PCM_RATE_KNOT             (1<<31)         /* supports more non-continuos rates */

#define SOUNDRV_PCM_RATE_8000_44100       (SOUNDRV_PCM_RATE_8000|SOUNDRV_PCM_RATE_11025|\
                                         SOUNDRV_PCM_RATE_16000|SOUNDRV_PCM_RATE_22050|\
                                         SOUNDRV_PCM_RATE_32000|SOUNDRV_PCM_RATE_44100)
#define SOUNDRV_PCM_RATE_8000_48000       (SOUNDRV_PCM_RATE_8000_44100|SOUNDRV_PCM_RATE_48000)
#define SOUNDRV_PCM_RATE_8000_96000       (SOUNDRV_PCM_RATE_8000_48000|SOUNDRV_PCM_RATE_64000|\
                                         SOUNDRV_PCM_RATE_88200|SOUNDRV_PCM_RATE_96000)
#define SOUNDRV_PCM_RATE_8000_192000      (SOUNDRV_PCM_RATE_8000_96000|SOUNDRV_PCM_RATE_176400|\
                                         SOUNDRV_PCM_RATE_192000)

#define	SOUNDRV_PCM_FORMAT_S8	((sound_pcm_format_t) 0)
#define	SOUNDRV_PCM_FORMAT_U8	((sound_pcm_format_t) 1)
#define	SOUNDRV_PCM_FORMAT_S16_LE	((sound_pcm_format_t) 2)
#define	SOUNDRV_PCM_FORMAT_S16_BE	((sound_pcm_format_t) 3)
#define	SOUNDRV_PCM_FORMAT_U16_LE	((sound_pcm_format_t) 4)
#define	SOUNDRV_PCM_FORMAT_U16_BE	((sound_pcm_format_t) 5)
#define	SOUNDRV_PCM_FORMAT_S24_LE	((sound_pcm_format_t) 6)
#define	SOUNDRV_PCM_FORMAT_S24_BE	((sound_pcm_format_t) 7)
#define	SOUNDRV_PCM_FORMAT_U24_LE	((sound_pcm_format_t) 8)
#define	SOUNDRV_PCM_FORMAT_U24_BE	((sound_pcm_format_t) 9)
#define	SOUNDRV_PCM_FORMAT_S32_LE	((sound_pcm_format_t) 10)
#define	SOUNDRV_PCM_FORMAT_S32_BE	((sound_pcm_format_t) 11)
#define	SOUNDRV_PCM_FORMAT_U32_LE	((sound_pcm_format_t) 12)
#define	SOUNDRV_PCM_FORMAT_U32_BE	((sound_pcm_format_t) 13)
#define	SOUNDRV_PCM_FORMAT_FLOAT_LE	((sound_pcm_format_t) 14) /* 4-byte float, IEEE-754 32-bit, range -1.0 to 1.0 */
#define	SOUNDRV_PCM_FORMAT_FLOAT_BE	((sound_pcm_format_t) 15) /* 4-byte float, IEEE-754 32-bit, range -1.0 to 1.0 */
#define	SOUNDRV_PCM_FORMAT_FLOAT64_LE	((sound_pcm_format_t) 16) /* 8-byte float, IEEE-754 64-bit, range -1.0 to 1.0 */
#define	SOUNDRV_PCM_FORMAT_FLOAT64_BE	((sound_pcm_format_t) 17) /* 8-byte float, IEEE-754 64-bit, range -1.0 to 1.0 */
#define	SOUNDRV_PCM_FORMAT_IEC958_SUBFRAME_LE ((sound_pcm_format_t) 18) /* IEC-958 subframe, Little Endian */
#define	SOUNDRV_PCM_FORMAT_IEC958_SUBFRAME_BE ((sound_pcm_format_t) 19) /* IEC-958 subframe, Big Endian */
#define	SOUNDRV_PCM_FORMAT_MU_LAW		((sound_pcm_format_t) 20)
#define	SOUNDRV_PCM_FORMAT_A_LAW		((sound_pcm_format_t) 21)
#define	SOUNDRV_PCM_FORMAT_IMA_ADPCM	((sound_pcm_format_t) 22)
#define	SOUNDRV_PCM_FORMAT_MPEG		((sound_pcm_format_t) 23)
#define	SOUNDRV_PCM_FORMAT_GSM		((sound_pcm_format_t) 24)
#define	SOUNDRV_PCM_FORMAT_S20_LE	((sound_pcm_format_t) 25) /* in four bytes, LSB justified */
#define	SOUNDRV_PCM_FORMAT_S20_BE	((sound_pcm_format_t) 26) /* in four bytes, LSB justified */
#define	SOUNDRV_PCM_FORMAT_U20_LE	((sound_pcm_format_t) 27) /* in four bytes, LSB justified */
#define	SOUNDRV_PCM_FORMAT_U20_BE	((sound_pcm_format_t) 28) /* in four bytes, LSB justified */
/* gap in the numbering for a future standard linear format */
#define	SOUNDRV_PCM_FORMAT_SPECIAL	((sound_pcm_format_t) 31)
#define	SOUNDRV_PCM_FORMAT_S24_3LE	((sound_pcm_format_t) 32)	/* in three bytes */
#define	SOUNDRV_PCM_FORMAT_S24_3BE	((sound_pcm_format_t) 33)	/* in three bytes */
#define	SOUNDRV_PCM_FORMAT_U24_3LE	((sound_pcm_format_t) 34)	/* in three bytes */
#define	SOUNDRV_PCM_FORMAT_U24_3BE	((sound_pcm_format_t) 35)	/* in three bytes */
#define	SOUNDRV_PCM_FORMAT_S20_3LE	((sound_pcm_format_t) 36)	/* in three bytes */
#define	SOUNDRV_PCM_FORMAT_S20_3BE	((sound_pcm_format_t) 37)	/* in three bytes */
#define	SOUNDRV_PCM_FORMAT_U20_3LE	((sound_pcm_format_t) 38)	/* in three bytes */
#define	SOUNDRV_PCM_FORMAT_U20_3BE	((sound_pcm_format_t) 39)	/* in three bytes */
#define	SOUNDRV_PCM_FORMAT_S18_3LE	((sound_pcm_format_t) 40)	/* in three bytes */
#define	SOUNDRV_PCM_FORMAT_S18_3BE	((sound_pcm_format_t) 41)	/* in three bytes */
#define	SOUNDRV_PCM_FORMAT_U18_3LE	((sound_pcm_format_t) 42)	/* in three bytes */
#define	SOUNDRV_PCM_FORMAT_U18_3BE	((sound_pcm_format_t) 43)	/* in three bytes */
#define	SOUNDRV_PCM_FORMAT_G723_24	((sound_pcm_format_t) 44) /* 8 samples in 3 bytes */
#define	SOUNDRV_PCM_FORMAT_G723_24_1B	((sound_pcm_format_t) 45) /* 1 sample in 1 byte */
#define	SOUNDRV_PCM_FORMAT_G723_40	((sound_pcm_format_t) 46) /* 8 Samples in 5 bytes */
#define	SOUNDRV_PCM_FORMAT_G723_40_1B	((sound_pcm_format_t) 47) /* 1 sample in 1 byte */
#define	SOUNDRV_PCM_FORMAT_DSD_U8		((sound_pcm_format_t) 48) /* DSD, 1-byte samples DSD (x8) */
#define	SOUNDRV_PCM_FORMAT_DSD_U16_LE	((sound_pcm_format_t) 49) /* DSD, 2-byte samples DSD (x16), little endian */
#define	SOUNDRV_PCM_FORMAT_DSD_U32_LE	((sound_pcm_format_t) 50) /* DSD, 4-byte samples DSD (x32), little endian */
#define	SOUNDRV_PCM_FORMAT_DSD_U16_BE	((sound_pcm_format_t) 51) /* DSD, 2-byte samples DSD (x16), big endian */
#define	SOUNDRV_PCM_FORMAT_DSD_U32_BE	((sound_pcm_format_t) 52) /* DSD, 4-byte samples DSD (x32), big endian */
#define	SOUNDRV_PCM_FORMAT_LAST		SOUNDRV_PCM_FORMAT_DSD_U32_BE
#define	SOUNDRV_PCM_FORMAT_FIRST		SOUNDRV_PCM_FORMAT_S8

#define _SOUNDRV_PCM_FMTBIT(fmt)          (1ULL << (int)SOUNDRV_PCM_FORMAT_##fmt)
#define SOUNDRV_PCM_FMTBIT_S8             _SOUNDRV_PCM_FMTBIT(S8)
#define SOUNDRV_PCM_FMTBIT_U8             _SOUNDRV_PCM_FMTBIT(U8)
#define SOUNDRV_PCM_FMTBIT_S16_LE         _SOUNDRV_PCM_FMTBIT(S16_LE)
#define SOUNDRV_PCM_FMTBIT_S16_BE         _SOUNDRV_PCM_FMTBIT(S16_BE)
#define SOUNDRV_PCM_FMTBIT_U16_LE         _SOUNDRV_PCM_FMTBIT(U16_LE)
#define SOUNDRV_PCM_FMTBIT_U16_BE         _SOUNDRV_PCM_FMTBIT(U16_BE)
#define SOUNDRV_PCM_FMTBIT_S24_LE         _SOUNDRV_PCM_FMTBIT(S24_LE)
#define SOUNDRV_PCM_FMTBIT_S24_BE         _SOUNDRV_PCM_FMTBIT(S24_BE)
#define SOUNDRV_PCM_FMTBIT_U24_LE         _SOUNDRV_PCM_FMTBIT(U24_LE)
#define SOUNDRV_PCM_FMTBIT_U24_BE         _SOUNDRV_PCM_FMTBIT(U24_BE)
#define SOUNDRV_PCM_FMTBIT_S32_LE         _SOUNDRV_PCM_FMTBIT(S32_LE)
#define SOUNDRV_PCM_FMTBIT_S32_BE         _SOUNDRV_PCM_FMTBIT(S32_BE)
#define SOUNDRV_PCM_FMTBIT_U32_LE         _SOUNDRV_PCM_FMTBIT(U32_LE)
#define SOUNDRV_PCM_FMTBIT_U32_BE         _SOUNDRV_PCM_FMTBIT(U32_BE)
#define SOUNDRV_PCM_FMTBIT_U32_LE		_SOUNDRV_PCM_FMTBIT(U32_LE)
#define SOUNDRV_PCM_FMTBIT_U32_BE		_SOUNDRV_PCM_FMTBIT(U32_BE)
#define SOUNDRV_PCM_FMTBIT_FLOAT_LE	_SOUNDRV_PCM_FMTBIT(FLOAT_LE)
#define SOUNDRV_PCM_FMTBIT_FLOAT_BE	_SOUNDRV_PCM_FMTBIT(FLOAT_BE)
#define SOUNDRV_PCM_FMTBIT_FLOAT64_LE	_SOUNDRV_PCM_FMTBIT(FLOAT64_LE)
#define SOUNDRV_PCM_FMTBIT_FLOAT64_BE	_SOUNDRV_PCM_FMTBIT(FLOAT64_BE)
#define SOUNDRV_PCM_FMTBIT_IEC958_SUBFRAME_LE _SOUNDRV_PCM_FMTBIT(IEC958_SUBFRAME_LE)
#define SOUNDRV_PCM_FMTBIT_IEC958_SUBFRAME_BE _SOUNDRV_PCM_FMTBIT(IEC958_SUBFRAME_BE)
#define SOUNDRV_PCM_FMTBIT_MU_LAW		_SOUNDRV_PCM_FMTBIT(MU_LAW)
#define SOUNDRV_PCM_FMTBIT_A_LAW		_SOUNDRV_PCM_FMTBIT(A_LAW)
#define SOUNDRV_PCM_FMTBIT_IMA_ADPCM	_SOUNDRV_PCM_FMTBIT(IMA_ADPCM)
#define SOUNDRV_PCM_FMTBIT_MPEG		_SOUNDRV_PCM_FMTBIT(MPEG)
#define SOUNDRV_PCM_FMTBIT_GSM		_SOUNDRV_PCM_FMTBIT(GSM)
#define SOUNDRV_PCM_FMTBIT_S20_LE	_SOUNDRV_PCM_FMTBIT(S20_LE)
#define SOUNDRV_PCM_FMTBIT_U20_LE	_SOUNDRV_PCM_FMTBIT(U20_LE)
#define SOUNDRV_PCM_FMTBIT_S20_BE	_SOUNDRV_PCM_FMTBIT(S20_BE)
#define SOUNDRV_PCM_FMTBIT_U20_BE	_SOUNDRV_PCM_FMTBIT(U20_BE)
#define SOUNDRV_PCM_FMTBIT_SPECIAL	_SOUNDRV_PCM_FMTBIT(SPECIAL)
#define SOUNDRV_PCM_FMTBIT_S24_3LE	_SOUNDRV_PCM_FMTBIT(S24_3LE)
#define SOUNDRV_PCM_FMTBIT_U24_3LE	_SOUNDRV_PCM_FMTBIT(U24_3LE)
#define SOUNDRV_PCM_FMTBIT_S24_3BE	_SOUNDRV_PCM_FMTBIT(S24_3BE)
#define SOUNDRV_PCM_FMTBIT_U24_3BE	_SOUNDRV_PCM_FMTBIT(U24_3BE)
#define SOUNDRV_PCM_FMTBIT_S20_3LE	_SOUNDRV_PCM_FMTBIT(S20_3LE)
#define SOUNDRV_PCM_FMTBIT_U20_3LE	_SOUNDRV_PCM_FMTBIT(U20_3LE)
#define SOUNDRV_PCM_FMTBIT_S20_3BE	_SOUNDRV_PCM_FMTBIT(S20_3BE)
#define SOUNDRV_PCM_FMTBIT_U20_3BE	_SOUNDRV_PCM_FMTBIT(U20_3BE)
#define SOUNDRV_PCM_FMTBIT_S18_3LE	_SOUNDRV_PCM_FMTBIT(S18_3LE)
#define SOUNDRV_PCM_FMTBIT_U18_3LE	_SOUNDRV_PCM_FMTBIT(U18_3LE)
#define SOUNDRV_PCM_FMTBIT_S18_3BE	_SOUNDRV_PCM_FMTBIT(S18_3BE)
#define SOUNDRV_PCM_FMTBIT_U18_3BE	_SOUNDRV_PCM_FMTBIT(U18_3BE)
#define SOUNDRV_PCM_FMTBIT_G723_24	_SOUNDRV_PCM_FMTBIT(G723_24)
#define SOUNDRV_PCM_FMTBIT_G723_24_1B	_SOUNDRV_PCM_FMTBIT(G723_24_1B)
#define SOUNDRV_PCM_FMTBIT_G723_40	_SOUNDRV_PCM_FMTBIT(G723_40)
#define SOUNDRV_PCM_FMTBIT_G723_40_1B	_SOUNDRV_PCM_FMTBIT(G723_40_1B)
#define SOUNDRV_PCM_FMTBIT_DSD_U8		_SOUNDRV_PCM_FMTBIT(DSD_U8)
#define SOUNDRV_PCM_FMTBIT_DSD_U16_LE	_SOUNDRV_PCM_FMTBIT(DSD_U16_LE)
#define SOUNDRV_PCM_FMTBIT_DSD_U32_LE	_SOUNDRV_PCM_FMTBIT(DSD_U32_LE)
#define SOUNDRV_PCM_FMTBIT_DSD_U16_BE	_SOUNDRV_PCM_FMTBIT(DSD_U16_BE)
#define SOUNDRV_PCM_FMTBIT_DSD_U32_BE	_SOUNDRV_PCM_FMTBIT(DSD_U32_BE)

struct sound_pcm_running_param {
	sound_pcm_uframes_t hw_ptr_base;
	sound_pcm_access_t access;
	sound_pcm_format_t format;
	unsigned int rate;
	unsigned int channels;
	unsigned int can_paused;
	sound_pcm_uframes_t period_size;
	unsigned int periods;
	sound_pcm_uframes_t buffer_size;
	sound_pcm_uframes_t min_align;		/* Min alignment for the format, frame align */
	unsigned int frame_bits;
	unsigned int sample_bits;
	unsigned int no_period_wakeup: 1;

	dma_addr_t dma_addr;
	size_t dma_bytes;

	struct sound_dma_buffer *dma_buffer_p;

	/* private section */
	void *private_data;

	struct sound_pcm_mmap_status *status;
	struct sound_pcm_mmap_control *control;

	/* HW constrains */
	struct sound_pcm_hardware hw;
	struct sound_pcm_hw_constrains hw_constrains;

	/* Other params */
	sound_pcm_uframes_t start_threshold;
	sound_pcm_uframes_t stop_threshold;
	sound_pcm_uframes_t silence_size;

	sound_pcm_uframes_t silence_start;
	sound_pcm_uframes_t silence_filled;

	sound_pcm_uframes_t boundary;

	/* locking / scheduling */
	sound_pcm_uframes_t twake;
	sound_schd_t tsleep;   /* transfer sleep */
	sound_schd_t sleep;    /* poll sleep (drain...) */
	sound_schd_t dsleep;	/* direct access sleep */

	sound_schd_t dsleep_list[32];

	unsigned int xrun_cnt;
};

struct sound_pcm_dataflow {
	struct sound_pcm *pcm;
	struct sound_pcm_data *pcm_data;
	void *priv_data;		/* copied from pcm->priv_data */
	char name[32];
	int stream;
	int number;
	struct sound_pcm_running_param *pcm_running;
	const struct sound_pcm_ops *ops;
	struct sound_dma_buffer dma_buffer;
	hal_spinlock_t lock;
	hal_irq_state_t irqstate;
	sound_mutex_t mutex;
	/* -- next dataflow -- */
	struct sound_pcm_dataflow *next;
	int ref_count;
	unsigned int mode;
	int dapm_state;
	int hw_opened;
};

struct sound_pcm_data {
	int stream;
	struct sound_pcm *pcm;
	unsigned int dataflow_count;
	unsigned int dataflow_opened;
	struct sound_pcm_dataflow *dataflow;
	struct sound_sys_device pcm_dev;
};

struct sound_pcm {
	struct sound_card *card;
	char name[48];
	char id[64];
	int device_num;
	struct sound_pcm_data data[2];
	struct list_head list;
	void *priv_data;
	void (*priv_free) (struct sound_pcm *pcm);
	sound_mutex_t open_mutex;
	bool nonatomic; /* whole PCM operations are in non-atomic context */
	bool nonatomic_irqsave; /* whole PCM operations are in non-atomic irq context */
};

int sound_pcm_action_start(void *dataflow_handle);
int sound_pcm_action_stop(struct sound_pcm_dataflow *dataflow, sound_pcm_state_t state);


int sound_new_pcm(struct sound_card *card, const char *id, int device,
		int playback_count, int capture_count, struct sound_pcm **rpcm);

void sound_pcm_stream_lock_irq(struct sound_pcm_dataflow *dataflow);
void sound_pcm_stream_unlock_irq(struct sound_pcm_dataflow *dataflow);

hal_irq_state_t _sound_pcm_stream_lock_irqsave(struct sound_pcm_dataflow *dataflow);
void sound_pcm_stream_unlock_irqrestore(struct sound_pcm_dataflow *dataflow,
					unsigned long flags);

#define sound_pcm_stream_lock_irqsave(dataflow, flags) \
do { \
	flags = _sound_pcm_stream_lock_irqsave(dataflow); \
} while (0)

static inline int sound_pcm_running(struct sound_pcm_dataflow *dataflow)
{
	return (dataflow->pcm_running->status->state == SOUNDRV_PCM_STATE_RUNNING ||
		(dataflow->pcm_running->status->state == SOUNDRV_PCM_STATE_DRAINING &&
		 dataflow->stream == SOUNDRV_PCM_STREAM_PLAYBACK));
}

/**
 * bytes_to_samples - Unit conversion of the size from bytes to samples
 * @runtime: PCM runtime instance
 * @size: size in bytes
 */
static inline ssize_t bytes_to_samples(struct sound_pcm_running_param *runtime, ssize_t size)
{
	return size * 8 / runtime->sample_bits;
}

static inline sound_pcm_sframes_t bytes_to_frames(struct sound_pcm_running_param *runtime, ssize_t size)
{
	return size * 8 / runtime->frame_bits;
}

/**
 * samples_to_bytes - Unit conversion of the size from samples to bytes
 * @runtime: PCM runtime instance
 * @size: size in samples
 */
static inline ssize_t samples_to_bytes(struct sound_pcm_running_param *runtime, ssize_t size)
{
	return size * runtime->sample_bits / 8;
}

static inline ssize_t frames_to_bytes(struct sound_pcm_running_param *runtime, sound_pcm_sframes_t size)
{
	return size * runtime->frame_bits / 8;
}

/* Get the available(readable) space for capture */
static inline sound_pcm_uframes_t sound_pcm_capture_avail(struct sound_pcm_running_param *pcm_running)
{
	sound_pcm_sframes_t avail = pcm_running->status->hw_ptr - pcm_running->control->appl_ptr;
	if (avail < 0)
		avail += pcm_running->boundary;
	return avail;
}

static inline size_t sound_pcm_lib_buffer_bytes(struct sound_pcm_dataflow *dataflow)
{
	struct sound_pcm_running_param *pcm_running = dataflow->pcm_running;
	return frames_to_bytes(pcm_running, pcm_running->buffer_size);
}

static inline size_t sound_pcm_lib_period_bytes(struct sound_pcm_dataflow *dataflow)
{
	struct sound_pcm_running_param *pcm_running = dataflow->pcm_running;
	return frames_to_bytes(pcm_running, pcm_running->period_size);
}

/* Get the available(writeable) space for playback */
static inline sound_pcm_uframes_t sound_pcm_playback_avail(struct sound_pcm_running_param *pcm_running)
{
	sound_pcm_sframes_t avail = pcm_running->status->hw_ptr + pcm_running->buffer_size - pcm_running->control->appl_ptr;

	if (avail < 0)
		avail += pcm_running->boundary;
	else if ((sound_pcm_uframes_t) avail >= pcm_running->boundary)
		avail -= pcm_running->boundary;

	return avail;
}

/* Get the queued space(has been written) for playback */
static inline sound_pcm_sframes_t sound_pcm_playback_hw_avail(struct sound_pcm_running_param *pcm_running)
{
	return pcm_running->buffer_size - sound_pcm_playback_avail(pcm_running);
}

/* Get the free space for capture */
static inline sound_pcm_sframes_t sound_pcm_capture_hw_avail(struct sound_pcm_running_param *pcm_running)
{
	return pcm_running->buffer_size - sound_pcm_capture_avail(pcm_running);
}

static inline int sound_pcm_playback_data(struct sound_pcm_dataflow *dataflow)
{
	struct sound_pcm_running_param *pcm_running = dataflow->pcm_running;
/*
	sound_debug("st %lu b %lu hw_p %lu bz %lu ap %lu av %lu\n", pcm_running->stop_threshold, pcm_running->boundary,
	    pcm_running->status->hw_ptr, pcm_running->buffer_size, pcm_running->control->appl_ptr, sound_pcm_playback_avail(pcm_running));
*/
	if (pcm_running->stop_threshold >= pcm_running->boundary)
		return 1;

	return sound_pcm_playback_avail(pcm_running) < pcm_running->buffer_size;
}

static inline int sound_pcm_playback_empty(struct sound_pcm_dataflow *dataflow)
{
	struct sound_pcm_running_param *pcm_running = dataflow->pcm_running;
	return sound_pcm_playback_avail(pcm_running) >= pcm_running->buffer_size;
}


void sound_set_pcm_ops(struct sound_pcm *pcm, int direction,
				const struct sound_pcm_ops *ops);

int sound_pcm_lib_ioctl(struct sound_pcm_dataflow *dataflow,
				unsigned int cmd, void *arg);
void sound_pcm_period_elapsed(struct sound_pcm_dataflow *dataflow);
sound_pcm_sframes_t __sound_pcm_lib_xfer(struct sound_pcm_dataflow *dataflow,
				     void *data, int interleaved,
				     sound_pcm_uframes_t size);


static inline sound_pcm_sframes_t
sound_pcm_lib_write(struct sound_pcm_dataflow *dataflow,
		  const void  *buf, sound_pcm_uframes_t frames)
{
	return __sound_pcm_lib_xfer(dataflow, (void *)buf, true, frames);
}

static inline sound_pcm_sframes_t
sound_pcm_lib_read(struct sound_pcm_dataflow *dataflow,
		 void *buf, sound_pcm_uframes_t frames)
{
	return __sound_pcm_lib_xfer(dataflow, (void *)buf, true, frames);
}


static inline void sound_pcm_set_runtime_buffer(struct sound_pcm_dataflow *dataflow,
						struct sound_dma_buffer *bufp)
{
	struct sound_pcm_running_param *pcm_running = dataflow->pcm_running;

	if (bufp) {
		pcm_running->dma_buffer_p = bufp;
		pcm_running->dma_addr = bufp->addr;
		pcm_running->dma_bytes = bufp->bytes;
	} else {
		pcm_running->dma_buffer_p = NULL;
		pcm_running->dma_addr = 0;
		pcm_running->dma_bytes = 0;
	}

	sound_debug("runtime dma buffer addr %p size %u\n", pcm_running->dma_addr, pcm_running->dma_bytes);
}

#define for_each_pcm_dataflow(pcm, str, dataflow) \
	for ((str) = 0; (str) < 2; (str)++) \
		for ((dataflow) = (pcm)->data[str].dataflow; (dataflow); \
		     (dataflow) = (dataflow)->next)


int sound_pcm_suspend_total(struct sound_pcm *pcm);
int sound_pcm_resume_total(struct sound_pcm *pcm);

#endif /* __SOUND_PCM_H */

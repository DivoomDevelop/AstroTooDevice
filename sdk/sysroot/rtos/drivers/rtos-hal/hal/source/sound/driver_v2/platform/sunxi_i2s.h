/*
 * sound\sound_sunxi_rxsync.h
 * (C) Copyright 2025-2029
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
 * zhouxijing <zhouxijing@allwinnertech.com>
 * xudongpdc <xudongpdc@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef	__SUNXI_I2S_H_
#define	__SUNXI_I2S_H_

#include <aw_common.h>
#include <hal_clk.h>
#include <hal_reset.h>
#include <hal_gpio.h>

/* I2S register definition */
#define	SUNXI_I2S_CTL		0x00
#define	SUNXI_I2S_FMT0		0x04
#define	SUNXI_I2S_FMT1		0x08
#define	SUNXI_I2S_INTSTA		0x0C
#define	SUNXI_I2S_RXFIFO		0x10
#define	SUNXI_I2S_FIFOCTL		0x14
#define	SUNXI_I2S_FIFOSTA		0x18
#define	SUNXI_I2S_INTCTL		0x1C
#define	SUNXI_I2S_TXFIFO		0x20
#define	SUNXI_I2S_CLKDIV		0x24
#define	SUNXI_I2S_TXCNT		0x28
#define	SUNXI_I2S_RXCNT		0x2C
#define	SUNXI_I2S_CHCFG		0x30
#define	SUNXI_I2S_TX0CHSEL		0x34
#define	SUNXI_I2S_TX1CHSEL		0x38
#define	SUNXI_I2S_TX2CHSEL		0x3C
#define	SUNXI_I2S_TX3CHSEL		0x40

#define	SUNXI_I2S_TX0CHMAP0		0x44
#define	SUNXI_I2S_TX0CHMAP1		0x48
#define	SUNXI_I2S_TX1CHMAP0		0x4C
#define	SUNXI_I2S_TX1CHMAP1		0x50
#define	SUNXI_I2S_TX2CHMAP0		0x54
#define	SUNXI_I2S_TX2CHMAP1		0x58
#define	SUNXI_I2S_TX3CHMAP0		0x5C
#define	SUNXI_I2S_TX3CHMAP1		0x60
#define	SUNXI_I2S_RXCHSEL		0x64
#define	SUNXI_I2S_RXCHMAP0		0x68
#define	SUNXI_I2S_RXCHMAP1		0x6C
#define	SUNXI_I2S_RXCHMAP2		0x70
#define	SUNXI_I2S_RXCHMAP3		0x74
#define	SUNXI_I2S_DEBUG		0x78
#define	SUNXI_I2S_REV		0x7C

#define SUNXI_I2S_ASRC_MCLKCFG       0x80
#define SUNXI_I2S_ASRC_FSOUTCFG      0x84
#define SUNXI_I2S_ASRC_FSIN_EXTCFG   0x88
#define SUNXI_I2S_ASRC_ASRCEN        0x8C
#define SUNXI_I2S_ASRC_MANCFG        0x90
#define SUNXI_I2S_ASRC_RATIOSTAT     0x94
#define SUNXI_I2S_ASRC_FIFOSTAT      0x98
#define SUNXI_I2S_ASRC_MBISTCFG      0x9C
#define SUNXI_I2S_ASRC_MBISTSTA      0xA0

/* SUNXI_I2S_CTL:0x00 */
#define RX_SYNC_EN_STA			21
#define RX_SYNC_EN			20
#define	BCLK_OUT			18
#define	LRCK_OUT			17
#define	LRCKR_CTL			16
#define	SDO3_EN				11
#define	SDO2_EN				10
#define	SDO1_EN				9
#define	SDO0_EN				8
#define	MUTE_CTL			6
#define	MODE_SEL			4
#define	LOOP_EN				3
#define	CTL_TXEN			2
#define	CTL_RXEN			1
#define	GLOBAL_EN			0

/* SUNXI_I2S_FMT0:0x04 */
#define	SDI_SYNC_SEL			31
#define	LRCK_WIDTH			30
#define	LRCKR_PERIOD			20
#define	LRCK_POLARITY			19
#define	LRCK_PERIOD			8
#define	BCLK_POLARITY			7
#define	I2S_SAMPLE_RESOLUTION	4
#define	EDGE_TRANSFER			3
#define	SLOT_WIDTH			0

/* SUNXI_I2S_FMT1:0x08 */
#define	RX_MLS				7
#define	TX_MLS				6
#define	SEXT				4
#define	RX_PDM				2
#define	TX_PDM				0

/* SUNXI_I2S_INTSTA:0x0C */
#define	TXU_INT				6
#define	TXO_INT				5
#define	TXE_INT				4
#define	RXU_INT				2
#define RXO_INT				1
#define	RXA_INT				0

/* SUNXI_I2S_FIFOCTL:0x14 */
#define	HUB_EN				31
#define	FIFO_CTL_FTX			25
#define	FIFO_CTL_FRX			24
#define	TXTL				12
#define	RXTL				4
#define	TXIM				2
#define	RXOM				0

/* SUNXI_I2S_FIFOSTA:0x18 */
#define	FIFO_TXE			28
#define	FIFO_TX_CNT			16
#define	FIFO_RXA			8
#define	FIFO_RX_CNT			0

/* SUNXI_I2S_INTCTL:0x1C */
#define	TXDRQEN				7
#define	TXUI_EN				6
#define	TXOI_EN				5
#define	TXEI_EN				4
#define	RXDRQEN				3
#define	RXUIEN				2
#define	RXOIEN				1
#define	RXAIEN				0

/* SUNXI_I2S_CLKDIV:0x24 */
#define	MCLKOUT_EN			8
#define	BCLK_DIV			4
#define	MCLK_DIV			0

/* SUNXI_I2S_CHCFG:0x30 */
#define	TX_SLOT_HIZ			9
#define	TX_STATE			8
#define	RX_SLOT_NUM			4
#define	TX_SLOT_NUM			0

/* SUNXI_I2S_TXnCHSEL:0X34+n*0x04 */
#define	TX_OFFSET			20
#define	TX_CHSEL			16
#define	TX_CHEN				0

/* SUNXI_I2S_RXCHSEL */
#define	RX_OFFSET			20
#define	RX_CHSEL			16

/* SUNXI_I2S_ASRC_MCLKCFG:0x80 */
#define I2S_ASRC_MCLK_GATE           16
#define I2S_ASRC_MCLK_RATIO          0

/* SUNXI_I2S_ASRC_FSOUTCFG:0x84 */
#define I2S_ASRC_FSOUT_GATE          20
#define I2S_ASRC_FSOUT_CLKSRC        16
#define I2S_ASRC_FSOUT_CLKDIV1       4
#define I2S_ASRC_FSOUT_CLKDIV2       0

/* SUNXI_I2S_ASRC_FSIN_EXTCFG:0x88 */
#define I2S_ASRC_FSIN_EXTEN          16
#define I2S_ASRC_FSIN_EXTCYCLE       0

/* SUNXI_I2S_ASRC_ASRCEN:0x8C */
#define I2S_ASRC_ASRCEN              0

/* DRQ Type */
#ifndef DRQDST_I2S0_TX
#define DRQDST_I2S0_TX	3
#endif
#ifndef DRQDST_I2S1_TX
#define DRQDST_I2S1_TX	4
#endif
#ifndef DRQDST_I2S2_TX
#define DRQDST_I2S2_TX	5
#endif
#ifndef DRQDST_I2S3_TX
#define DRQDST_I2S3_TX	6
#endif

#ifndef DRQSRC_I2S0_RX
#define DRQSRC_I2S0_RX	3
#endif
#ifndef DRQSRC_I2S1_RX
#define DRQSRC_I2S1_RX	4
#endif
#ifndef DRQSRC_I2S2_RX
#define DRQSRC_I2S2_RX	5
#endif
#ifndef DRQSRC_I2S3_RX
#define DRQSRC_I2S3_RX	6
#endif

#define	SUNXI_I2S_RATES	(SOUNDRV_PCM_RATE_8000_192000 \
				| SOUNDRV_PCM_RATE_KNOT)

#define SUNXI_I2S_DRQDST(sunxi_i2s, x)			\
	((sunxi_i2s)->playback_dma_param.dma_drq_type_num =	\
				DRQDST_I2S##x##_TX)
#define SUNXI_I2S_DRQSRC(sunxi_i2s, x)			\
	((sunxi_i2s)->capture_dma_param.dma_drq_type_num =	\
				DRQSRC_I2S##x##_RX)

struct sunxi_i2s_param {
	unsigned int tdm_num;
	/* pllclk */
	unsigned int pllclk_freq;

	/* dma */
	size_t playback_cma;
	size_t capture_cma;

	/* pin mapping */
	bool tx_pin[4];
	uint32_t tx_pin_chmap[4][16];
	uint32_t rxfifo_pinmap[16];
	uint32_t rxfifo_chmap[16];

	/* rx sync */
	bool rx_sync_ctl;
	int rx_sync_id;
	rx_sync_domain_t rx_sync_domain;

	unsigned int clk_en_post_delay;
	unsigned int clk_keep;

	bool spk_en;
	bool is_playing;
};

#if defined(CONFIG_ARCH_SUN251IW1)
#include "platforms/i2s-sun251iw1.h"
#elif defined (CONFIG_ARCH_SUN55IW6)
#include "platforms/i2s-sun55iw6.h"
#elif defined (CONFIG_ARCH_SUN252IW2)
#include "platforms/i2s-sun252iw2.h"
#endif

struct sunxi_i2s_clk_sta {
	hal_mutex_t clk_mutex;
	unsigned int old_rate;
	unsigned int p_work;
	unsigned int c_work;
};

struct sunxi_i2s {
	struct sunxi_i2s_clk *clk;
	struct sunxi_i2s_clk_sta clk_sta;
	struct sunxi_sound_pinctrl *pinctrl;
	unsigned int pinctrl_num;
	struct sunxi_pacfg *pa_cfg;
	uint32_t pa_pin_max;
	int asrc_en;

	struct sunxi_i2s_param param;
	struct sound_sunxi_dai_fmt dai_fmt;
	struct sunxi_dma_params playback_dma_param;
	struct sunxi_dma_params capture_dma_param;
};

#endif	/* __SUNXI_I2S_H_ */

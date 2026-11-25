/*
 * sound\sound_sunxi_rxsync.h
 * (C) Copyright 2025-2029
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
 * zhouxijing <zhouxijing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SOUND_SUN252IW2_CODEC_H
#define __SOUND_SUN252IW2_CODEC_H

#include <hal_clk.h>
#include "sunxi_sound_rxsync.h"

#define CODEC		"codec"

#define SUNXI_CODEC_BASE_ADDR		0x02030000

/* REG-Digital */
#define SUNXI_DAC_DPC			0x00
#define SUNXI_DAC_VOL_CTL		0x04
#define SUNXI_DAC_FIFO_CTL		0x10
#define SUNXI_DAC_FIFO_STA		0x14
#define SUNXI_DAC_TXDATA		0X20
#define SUNXI_DAC_CNT			0x24
#define SUNXI_DAC_DEBUG			0x28
#define SUNXI_ADC_FIFO_CTL		0x30
#define SUNXI_ADC_VOL_CTL1		0x34
#define SUNXI_ADC_FIFO_STA		0x38
#define SUNXI_ADC_RXDATA		0x40
#define SUNXI_ADC_CNT			0x44
#define SUNXI_ADC_DIG_CTL		0x50
#define SUNXI_VAR1SPEEDUP_DOWN_CTL	0x54
#define SUNXI_DAC_DAP_CTL		0xF0
#define SUNXI_ADC_DAP_CTL		0xF8

#define SUNXI_DAC_DRC_HHPFC	0x100
#define SUNXI_DAC_DRC_LHPFC	0x104
#define SUNXI_DAC_DRC_CTRL	0x108
#define SUNXI_DAC_DRC_LPFHAT	0x10C
#define SUNXI_DAC_DRC_LPFLAT	0x110
#define SUNXI_DAC_DRC_RPFHAT	0x114
#define SUNXI_DAC_DRC_RPFLAT	0x118
#define SUNXI_DAC_DRC_LPFHRT	0x11C
#define SUNXI_DAC_DRC_LPFLRT	0x120
#define SUNXI_DAC_DRC_RPFHRT	0x124
#define SUNXI_DAC_DRC_RPFLRT	0x128
#define SUNXI_DAC_DRC_LRMSHAT	0x12C
#define SUNXI_DAC_DRC_LRMSLAT	0x130
#define SUNXI_DAC_DRC_RRMSHAT	0x134
#define SUNXI_DAC_DRC_RRMSLAT	0x138
#define SUNXI_DAC_DRC_HCT	0x13C
#define SUNXI_DAC_DRC_LCT	0x140
#define SUNXI_DAC_DRC_HKC	0x144
#define SUNXI_DAC_DRC_LKC	0x148
#define SUNXI_DAC_DRC_HOPC	0x14C
#define SUNXI_DAC_DRC_LOPC	0x150
#define SUNXI_DAC_DRC_HLT	0x154
#define SUNXI_DAC_DRC_LLT	0x158
#define SUNXI_DAC_DRC_HKI	0x15C
#define SUNXI_DAC_DRC_LKI	0x160
#define SUNXI_DAC_DRC_HOPL	0x164
#define SUNXI_DAC_DRC_LOPL	0x168
#define SUNXI_DAC_DRC_HET	0x16C
#define SUNXI_DAC_DRC_LET	0x170
#define SUNXI_DAC_DRC_HKE	0x174
#define SUNXI_DAC_DRC_LKE	0x178
#define SUNXI_DAC_DRC_HOPE	0x17C
#define SUNXI_DAC_DRC_LOPE	0x180
#define SUNXI_DAC_DRC_HKN	0x184
#define SUNXI_DAC_DRC_LKN	0x188
#define SUNXI_DAC_DRC_SFHAT	0x18C
#define SUNXI_DAC_DRC_SFLAT	0x190
#define SUNXI_DAC_DRC_SFHRT	0x194
#define SUNXI_DAC_DRC_SFLRT	0x198
#define SUNXI_DAC_DRC_MXGHS	0x19C
#define SUNXI_DAC_DRC_MXGLS	0x1A0
#define SUNXI_DAC_DRC_MNGHS	0x1A4
#define SUNXI_DAC_DRC_MNGLS	0x1A8
#define SUNXI_DAC_DRC_EPSHC	0x1AC
#define SUNXI_DAC_DRC_EPSLC	0x1B0
#define SUNXI_DAC_DRC_OPT	0x1B4
#define SUNXI_DAC_DRC_HPFHGAIN	0x1B8
#define SUNXI_DAC_DRC_HPFLGAIN	0x1BC

#define SUNXI_ADC_DRC_HHPFC	0x200
#define SUNXI_ADC_DRC_LHPFC	0x204
#define SUNXI_ADC_DRC_CTRL	0x208
#define SUNXI_ADC_DRC_LPFHAT	0x20C
#define SUNXI_ADC_DRC_LPFLAT	0x210
#define SUNXI_ADC_DRC_RPFHAT	0x214
#define SUNXI_ADC_DRC_RPFLAT	0x218
#define SUNXI_ADC_DRC_LPFHRT	0x21C
#define SUNXI_ADC_DRC_LPFLRT	0x220
#define SUNXI_ADC_DRC_RPFHRT	0x224
#define SUNXI_ADC_DRC_RPFLRT	0x228
#define SUNXI_ADC_DRC_LRMSHAT	0x22C
#define SUNXI_ADC_DRC_LRMSLAT	0x230
#define SUNXI_ADC_DRC_HCT	0x23C
#define SUNXI_ADC_DRC_LCT	0x240
#define SUNXI_ADC_DRC_HKC	0x244
#define SUNXI_ADC_DRC_LKC	0x248
#define SUNXI_ADC_DRC_HOPC	0x24C
#define SUNXI_ADC_DRC_LOPC	0x250
#define SUNXI_ADC_DRC_HLT	0x254
#define SUNXI_ADC_DRC_LLT	0x258
#define SUNXI_ADC_DRC_HKI	0x25C
#define SUNXI_ADC_DRC_LKI	0x260
#define SUNXI_ADC_DRC_HOPL	0x264
#define SUNXI_ADC_DRC_LOPL	0x268
#define SUNXI_ADC_DRC_HET	0x26C
#define SUNXI_ADC_DRC_LET	0x270
#define SUNXI_ADC_DRC_HKE	0x274
#define SUNXI_ADC_DRC_LKE	0x278
#define SUNXI_ADC_DRC_HOPE	0x27C
#define SUNXI_ADC_DRC_LOPE	0x280
#define SUNXI_ADC_DRC_HKN	0x284
#define SUNXI_ADC_DRC_LKN	0x288
#define SUNXI_ADC_DRC_SFHAT	0x28C
#define SUNXI_ADC_DRC_SFLAT	0x290
#define SUNXI_ADC_DRC_SFHRT	0x294
#define SUNXI_ADC_DRC_SFLRT	0x298
#define SUNXI_ADC_DRC_MXGHS	0x29C
#define SUNXI_ADC_DRC_MXGLS	0x2A0
#define SUNXI_ADC_DRC_MNGHS	0x2A4
#define SUNXI_ADC_DRC_MNGLS	0x2A8
#define SUNXI_ADC_DRC_EPSHC	0x2AC
#define SUNXI_ADC_DRC_EPSLC	0x2B0
#define SUNXI_ADC_DRC_OPT	0x2B4
#define SUNXI_ADC_DRC_HPFHGAIN	0x2B8
#define SUNXI_ADC_DRC_HPFLGAIN	0x2BC

#define SUNXI_VERSION			0x2C0

/* REG-Analog */
#define SUNXI_ADC1_AN_REG		0x300
#define SUNXI_DAC_AN_REG		0x310
#define SUNXI_RAMP_REG			0x31C
#define SUNXI_BIAS_REG			0x320
#define SUNXI_HP2_REG			0x340
#define SUNXI_POWER_REG			0x348
#define SUNXI_ADC_CUR_REG		0x34C
#define SUNXI_AUDIO_MAX_REG		SUNXI_ADC_CUR_REG

/* BITS */
/* SUNXI_DAC_DPC:0x00 */
#define DAC_DIG_EN			31
#define MODQU				25
#define DWA_EN				24
#define HPF_EN				18
#define DVOL				12
#define DITHER_SGM			8
#define DITHER_SFT			4
#define DITHER_EN			1
#define HUB_EN				0
/* SUNXI_DAC_VOL_CTL:0x04 */
#define DAC_VOL_SEL			16
#define DAC_VOL_L			8
#define DAC_VOL_R			0
/* SUNXI_DAC_FIFO_CTL:0x10 */
#define DAC_FS				29
#define FIR_VER				28
#define SEND_LASAT			26
#define DAC_FIFO_MODE			24
#define DAC_DRQ_CLR_CNT			21
#define TX_TRIG_LEVEL			8
#define DAC_MONO_EN			6
#define TX_SAMPLE_BITS			5
#define DAC_DRQ_EN			4
#define DAC_IRQ_EN			3
#define DAC_FIFO_UNDERRUN_IRQ_EN	2
#define DAC_FIFO_OVERRUN_IRQ_EN		1
#define DAC_FIFO_FLUSH			0
/* SUNXI_DAC_FIFO_STA:0x14 */
#define	DAC_TX_EMPTY			23
#define	DAC_TXE_CNT			8
#define	DAC_TXE_INT			3
#define	DAC_TXU_INT			2
#define	DAC_TXO_INT			1
/* SUNXI_DAC_DEBUG:0x28 */
#define	DAC_MODU_SEL			11
#define	DAC_PATTERN_SEL			9
#define	CODEC_CLK_SEL			8
#define	DA_SWP				6
#define	ADDA_LOOP_MODE			0
/* SUNXI_ADC_FIFO_CTL:0x30 */
#define	ADC_FS				29
#define	EN_AD				28
#define	ADCFDT				26
#define	ADCDFEN				25
#define	RX_FIFO_MODE			24
#define	RX_SYNC_EN_START		21
#define	RX_SYNC_EN			20
#define	RX_SAMPLE_BITS			16
#define	RX_FIFO_TRG_LEVEL		4
#define	ADC_DRQ_EN			3
#define	ADC_IRQ_EN			2
#define	ADC_OVERRUN_IRQ_EN		1
#define	ADC_FIFO_FLUSH			0
/* SUNXI_ADC_VOL_CTL1:0x34 */
#define	PLACE_HOLDER_0			24
#define	PLACE_HOLDER_1			16
#define	ADC1_VOL			0
/* SUNXI_ADC_FIFO_STA:0x38 */
#define	RXA				23
#define	RXA_CNT				8
#define	RXA_INT				3
#define	RXO_INT				1
#define	PLACE_HOLDER			0
/* SUNXI_ADC_DIG_CTL:0x50 */
#define	ADC1_VOL_EN			16
#define ADC1_CHANNEL_EN			0
/* SUNXI_VAR1SPEEDUP_DOWN_CTL:0x54 */
#define VRA1SPEEDUP_DOWN_STATE		4
#define VRA1SPEEDUP_DOWN_CTL		1
#define VRA1SPEEDUP_DOWN_RST_CTL	0
/* SUNXI_DAC_DAP_CTL:0xF0 */
#define DDAP_EN				31
#define DDAP_DRC_EN			29
#define DDAP_HPF_EN			28
/* SUNXI_ADC_DAP_CTL:0xF8 */
#define ADC_DAP_EN				31
#define ADC_DRC_EN			29
#define ADC_HPF_EN			28
/* SUNXI_DAC_DRC_HHPFC : 0x100*/
#define DAC_HHPF_CONF		0
/* SUNXI_DAC_DRC_LHPFC : 0x104*/
#define DAC_LHPF_CONF		0
/* SUNXI_DAC_DRC_CTRL : 0x108*/
#define DAC_DRC_DELAY_OUT_STATE	15
#define DAC_DRC_SIGNAL_DELAY	8
#define DAC_DRC_DELAY_BUF_EN	7
#define DAC_DRC_GAIN_MAX_EN	6
#define DAC_DRC_GAIN_MIN_EN	5
#define DAC_DRC_NOISE_DET_EN	4
#define DAC_DRC_SIGNAL_SEL	3
#define DAC_DRC_DELAY_EN	2
#define DAC_DRC_LT_EN		1
#define DAC_DRC_ET_EN		0
/* SUNXI_ADC_DRC_HHPFC : 0x200*/
#define ADC_HHPF_CONF		0
/* SUNXI_ADC_DRC_LHPFC : 0x204*/
#define ADC_LHPF_CONF		0
/* SUNXI_ADC_DRC_CTRL : 0x208*/
#define ADC_DRC_DELAY_OUT_STATE	15
#define ADC_DRC_SIGNAL_DELAY	8
#define ADC_DRC_DELAY_BUF_EN	7
#define ADC_DRC_GAIN_MAX_EN	6
#define ADC_DRC_GAIN_MIN_EN	5
#define ADC_DRC_NOISE_DET_EN	4
#define ADC_DRC_SIGNAL_SEL	3
#define ADC_DRC_DELAY_EN	2
#define ADC_DRC_LT_EN		1
#define ADC_DRC_ER_EN		0
/* SUNXI_ADC1_AN_REG:0x300 */
#define ADC1_EN				31
#define MIC1_PGA_EN			30
#define ADC1_DITHER_CTL			29
#define FMINLEN				27
#define DSM_DITHER_LVL			24
#define LINEINLEN			23
#define IOPAAF2S			20
#define LINEINL_GAIN_CTL		18
#define FML_GAIN_CTL			16
#define DSM_RESET_CTL			13
#define MIC_START_CTL			12
#define ADC1_PGA_GAIN_CTL		8
#define ADC1_IOPAAF			6
#define ADC1_IOPSDM1			4
#define ADC1_IOPSDM2			2
#define PGA1_OP1_CHOPPER_EN		1
#define PGA1_OP2_CHOPPER_EN		0
/* SUNXI_DAC_AN_REG:0x310 */
#define DAC_CHOPPER_EN			31
#define DAC_CHOPPER_NOL_EN		30
#define DAC_CHOPPER_CKSET		28
#define DAC_CHOPPER_DLYSET		26
#define DAC_CHOPPER_NOLDLY_SET		24
#define DACL_EN				15
#define DACR_EN				14
#define LMUTE				12
#define RMUTE				10
#define CKDAC_DLYSET			8
/* SUNXI_RAMP_REG:0x31C */
#define RAMP_RISE_INT_EN		31
#define RAMP_RISE_INT			30
#define RAMP_FALL_INT_EN		29
#define RAMP_FALL_INT			28
#define RAMP_SOFT_RESET			24
#define RK_OPT_EN			21
#define RAMP_CLK_DIV_M			16
#define HP_PULL_OUT_EN			15
#define RAMP_HOLD_STEP			12
#define GAP_STEP			8
#define RAMP_STEP			4
#define RMD_EN				3
#define RMU_EN				2
#define RMC_EN				1
#define RAMP_DIG_EN			0
/* SUNXI_BIAS_REG:0x320 */
#define CURRENT_TEST_SEL		20
#define RESERVED_BIT			18
#define IOPVRS				16
#define IOPMIXS				14
#define IOPDACS				12
#define IOPADC				10
#define IOPHP				8
#define BIAS_CURRENT_TRIM		0
/* SUNXI_HP2_REG:0x340 */
#define HPFB_BUF_EN			31
#define HP_GAIN				28
#define HPFB_RES			26
#define OPDRV_CUR			24
#define HP_OFFSET_ADJ			22
#define HP_DRVEN			21
#define HP_VO_OFFSET_EN			20
#define RSWITCH				19
#define RAMPEN				18
#define HPFB_IN_EN			17
#define RAMP_FINAL_CTL			16
#define RAMP_OUT_EN			15
#define RAMP_FILTER			8
#define HP_CHOPPER_EN			7
#define HP_CHOPPER_NOL_EN		6
#define HP_CHOPPER_CKS			4
#define HP_CHOPPER_DLYSET		2
#define HP_CHOPPER_NOLDLY_SET		0
/* SUNXI_POWER_REG:0x348 */
#define VAR1SPEEDUP_DOWN_FURTHER_CTL	29
#define AVCCPOR				16
#define BG_BUFFER_DISABLE   15
#define ALDO_TRMMING        4
#define BG_TRMMING          0
/* SUNXI_ADC_CUR_REG:0x34C */
#define ADC1_PGA_CHOPPER_DLYSET		6
#define ADC1_PGA_CHOPPER_EN		5
#define ADC1_PGA_CHOPPER_NOL_EN		4
#define ADC1_PGA_CHOPPER_NOLDLYSET	2
#define ADC1_PGA_CHOPPER_FREQ_SET	0
/* priv param */
#define SOUND_CTL_ENUM_NONE_MASK		3

enum adc_src {
	ADC_SRC_NULL = 0,
	ADC_SRC_MICIN,
	ADC_SRC_FMIN,
	ADC_SRC_LINEIN,
};

struct sunxi_codec_clk {
	/* parent */
	hal_clk_t clk_pll_audio1;
	hal_clk_t clk_pll_audio1_div5;
	hal_clk_t clk_pll_audio1_div2;
	/* module */
	hal_clk_t clk_audio_dac;
	hal_clk_t clk_audio_adc;
	/* bus & reset */
	hal_clk_t clk_bus;
	struct reset_control *clk_rst;
};

struct sunxi_codec_param {
	/* volume & gain */
	uint16_t dac_vol;
	uint16_t dacl_vol;
	uint16_t dacr_vol;
	uint16_t hpout_gain;
	uint16_t adc1_vol;
	uint16_t adc1_gain;
	uint16_t lineinl_gain;
	uint16_t fminl_gain;

	/* switch */
	enum adc_src adc1_src;
	bool hpoutl_en;
	bool hpoutr_en;
	bool spk_en;

	/* hw DRC & HPF */
	uint8_t adcdrc_en;
	uint8_t adchpf_en;
	uint8_t dacdrc_en;
	uint8_t dachpf_en;

	/* tx_hub */
	bool tx_hub_en;

	/* rx_sync */
	bool rx_sync_en;
	bool rx_sync_ctl;
	int16_t rx_sync_id;
	rx_sync_domain_t rx_sync_domain;

	/* dapm */
	bool is_playing;
	bool is_recording;
};

struct sunxi_codec {
	void *codec_base_addr;
	struct sunxi_codec_clk clk;
	struct sunxi_codec_param param;
	struct sunxi_pacfg *pa_cfg;
	uint32_t pa_pin_max;

	hal_mutex_t mutex;
};

#endif /* __SOUND_SUN252IW2_CODEC_H */

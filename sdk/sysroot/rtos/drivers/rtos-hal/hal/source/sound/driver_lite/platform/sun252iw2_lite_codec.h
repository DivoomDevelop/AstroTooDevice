#ifndef __SUN252IW2_LITE_CODEC_H
#define __SUN252IW2_LITE_CODEC_H

#include <hal_clk.h>

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
#define SUNXI_VAR1SPEEDUP_DOWN_CTL	0x54

/* REG-Analog */
#define SUNXI_DAC_AN_REG		0x310
#define SUNXI_RAMP_REG			0x31C
#define SUNXI_BIAS_REG			0x320
#define SUNXI_HP2_REG			0x340
#define SUNXI_POWER_REG			0x348

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

/******* pa config *******/
struct lite_pa_cfg {
	gpio_pin_t gpio_pin;
	gpio_data_t data;

	short msleep_0;
	short msleep_1;
};

/******* fex param *******/
struct lite_fex_params {
	short pa_pin_max;
	struct lite_pa_cfg *pa_cfg;

	/* volume & gain */
	uint16_t dac_vol;
	uint16_t dacl_vol;
	uint16_t hpout_gain;

	/* switch */
	bool hpoutl_en;
	bool spk_en;
	bool is_playing;
};

/******* clk *******/
struct sunxi_codec_clk {
	/* parent */
	hal_clk_t clk_pll_audio1;
	hal_clk_t clk_pll_audio1_div5;
	hal_clk_t clk_pll_audio1_div2;
	/* module */
	hal_clk_t clk_audio_dac;
	/* bus & reset */
	hal_clk_t clk_bus;
	struct reset_control *clk_rst;
};

/******* core data struct *******/
struct sunxi_codec {
	void *addr_base;
	struct lite_fex_params fex_params;
	struct sunxi_codec_clk clk;

	dma_addr_t play_dma_addr;
	size_t play_dma_bytes;
	struct sunxi_dma_chan *play_chan;
};

struct sample_rate {
	unsigned int samplerate;
	unsigned int rate_bit;
};

#if defined(CONFIG_COMPONENTS_PM) || defined(CONFIG_STANDBY)
int sunxi_codec_suspend(void);
int sunxi_codec_resume(void);
#endif

#endif /* __SUN252IW2_LITE_CODEC_H */

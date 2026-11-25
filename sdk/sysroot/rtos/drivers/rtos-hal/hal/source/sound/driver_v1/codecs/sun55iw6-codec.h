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

#ifndef __SND_SUN55IW6_CODEC_H
#define __SND_SUN55IW6_CODEC_H

#include <hal_clk.h>
#include <sound/driver_v1/common/snd_sunxi_common.h>

#define CODEC		"audiocodec"

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

#define SUNXI_ANA_DEBUG_REG2	0x5c

#define SUNXI_DAC_DAP_CTL		0xF0
#define SUNXI_DAC_DRC_CTL		0x108

#define SUNXI_VERSION			0x2C0

/* REG-Analog */
#define SUNXI_DAC_AN_REG		0x310

#define SUNXI_RAMP_REG			0x31c
#define SUNXI_BIAS_AN_CTL		0x320
#define SUNXI_AUDIO_MAX_REG		SUNXI_BIAS_AN_CTL

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
/* SUNXI_VAR1SPEEDUP_DOWN_CTL:0x54 */
#define VRA1SPEEDUP_DOWN_STATE		4
#define VRA1SPEEDUP_DOWN_CTL		1
#define VRA1SPEEDUP_DOWN_RST_CTL	0
/* SUNXI_DAC_DAP_CTL:0xF0 */
#define DDAP_EN				31
#define DDAP_DRC_EN			29
#define DDAP_HPF_EN			28
/* SUNXI_DAC_AN_REG:0x310 */
#define CURRENT_TEST_SEL		31
#define VRA1SPEEDUP_DIS_MANUAL	22
#define IOPVRS				20
#define ILINEOUTAMPS			18
#define IOPDACS				16
#define DACL_EN				15
#define LINEOUTL_EN			13
#define LMUTE				12
#define OPVR_OI_CTRL			7
#define LINEOUT_DIFFEN			6
#define LINEOUT_GAIN			0
/* SUNXI_RAMP_REG:0x31C */
#define RMC_EN				1
#define RD_EN				0
/* SUNXI_BIAS_AN_CTL:0x320 */
#define BIASDATA			0

#define DACDRC_SHIFT			1
#define DACHPF_SHIFT			2

/* priv param */
#define SND_CTL_ENUM_LINEOUTL_MASK	1
#define SND_CTL_ENUM_SPK_MASK		1

struct sunxi_codec_clk {
	/* parent */
	hal_clk_t clk_pll_audio0_4x;
	hal_clk_t clk_pll_audio1_4x;
	/* module */
	hal_clk_t clk_audio_dac;
	/* bus & reset */
	hal_clk_t clk_bus;
	struct reset_control *clk_rst;
}; //unset

struct sunxi_codec_param {
	/* volume & gain */
	uint8_t dac_vol;
	uint8_t dacl_vol;
	uint8_t lineout_gain;

	bool lineoutl_en;
	bool spk_en;

	/* tx_hub */
	bool tx_hub_en;

	bool playback_dapm;
};

struct sunxi_codec_info {
	struct snd_codec *codec;

	void *codec_base_addr;
	struct sunxi_codec_clk clk;
	struct sunxi_codec_param param;
	struct sunxi_pa_config pa_cfg;
};

#endif /* __SND_SUN55IW6_CODEC_H */

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
#ifndef	__SUN251IW1_DAUDIO_H_
#define	__SUN251IW1_DAUDIO_H_

#define	SUNXI_DAUDIO_BASE (0x02032000)

#define DAUDIO_NUM_MAX	3

#define reg_write(addr, value)	hal_writel(value, addr)
#define reg_read(addr, value)	hal_readl(value, addr)

/*------------------------ PIN CONFIG FOR NORMAL ---------------------------*/
#define DAUDIO2_PIN_MCLK \
{.gpio_pin = GPIOF(6), .mux = 3, .driv_level = GPIO_DRIVING_LEVEL3}
#define DAUDIO2_PIN_BCLK \
{.gpio_pin = GPIOF(3), .mux = 3, .driv_level = GPIO_DRIVING_LEVEL3}
#define DAUDIO2_PIN_LRCK \
{.gpio_pin = GPIOF(5), .mux = 3, .driv_level = GPIO_DRIVING_LEVEL3}
#define DAUDIO2_PIN_DIN0 \
{.gpio_pin = GPIOF(1), .mux = 5, .driv_level = GPIO_DRIVING_LEVEL3}
#define DAUDIO2_PIN_DOUT0 \
{.gpio_pin = GPIOF(0), .mux = 3, .driv_level = GPIO_DRIVING_LEVEL3}

/*
 * Daudio Params Setting
 *
 *daudio_master:
 *	1: SND_SOC_DAIFMT_CBM_CFM(codec clk & FRM master)		use
 *	2: SND_SOC_DAIFMT_CBS_CFM(codec clk slave & FRM master)		not use
 *	3: SND_SOC_DAIFMT_CBM_CFS(codec clk master & frame slave)	not use
 *	4: SND_SOC_DAIFMT_CBS_CFS(codec clk & FRM slave)		use
 *tdm_config:
 *	0 is pcm; 1 is i2s
 *audio_format:
 *	1:SND_SOC_DAIFMT_I2S(standard i2s format). use
 *	2:SND_SOC_DAIFMT_RIGHT_J(right justfied format).
 *	3:SND_SOC_DAIFMT_LEFT_J(left justfied format)
 *	4:SND_SOC_DAIFMT_DSP_A(pcm. MSB is available on 2nd BCLK rising edge after LRC rising edge). use
 *	5:SND_SOC_DAIFMT_DSP_B(pcm. MSB is available on 1nd BCLK rising edge after LRC rising edge)
 *signal_inversion:
 *	1:SND_SOC_DAIFMT_NB_NF(normal bit clock + frame)  use
 *	2:SND_SOC_DAIFMT_NB_IF(normal BCLK + inv FRM)
 *	3:SND_SOC_DAIFMT_IB_NF(invert BCLK + nor FRM)  use
 *	4:SND_SOC_DAIFMT_IB_IF(invert BCLK + FRM)
 *pcm_lrck_period	:16/32/64/128/256
 *msb_lsb_first		:0: msb first; 1: lsb first
 *sign_extend		:0: zero pending; 1: sign extend
 *slot_width_select	:8 bit width / 16 bit width / 32 bit width
 *frametype		:0: short frame = 1 clock width;  1: long frame = 2 clock width
 *mclk_div		:0: not output(normal setting this);
 *			:1/2/4/6/8/12/16/24/32/48/64/96/128/176/192:
 *			setting mclk as input clock to external codec,
 *			freq is pll_audio/mclk_div
 *tx_data_mode		:0: 16bit linear PCM; (use) 1: reserved;
 *			:2: 8bit u-law; (no use) 3: 8bit a-law (no use)
 *rx_data_mode		:0: 16bit linear PCM; (use) 1: reserved;
 *			:2: 8bit u-law; (no use) 3: 8bit a-law (no use)
 */

#define DAUDIO0_PARAMS \
{.tdm_num = 0, \
.daudio_master = 4, .audio_format = 1, .signal_inversion = 1, \
.pcm_lrck_period = 64, .slot_width_select = 32, \
.msb_lsb_first	= 0, .frametype = 0, \
.tx_data_mode = 0, .rx_data_mode = 0, \
.tdm_config = 1, .mclk_div = 1,\
.rx_sync_en = false,\
.rx_sync_ctl = false,\
}

#define DAUDIO1_PARAMS \
{.tdm_num = 1, \
.daudio_master = 4, .audio_format = 1, .signal_inversion = 1, \
.pcm_lrck_period = 64, .slot_width_select = 32, \
.msb_lsb_first	= 0, .frametype = 0, \
.tx_data_mode = 0, .rx_data_mode = 0, \
.tdm_config = 1, .mclk_div = 1,\
.rx_sync_en = false,\
.rx_sync_ctl = false,\
}

#define DAUDIO2_PARAMS \
{.tdm_num = 2, \
.daudio_master = 4, .audio_format = 1, .signal_inversion = 1, \
.pcm_lrck_period = 64, .slot_width_select = 32, \
.msb_lsb_first	= 0, .frametype = 0, \
.tx_data_mode = 0, .rx_data_mode = 0, \
.tdm_config = 1, .mclk_div = 1,\
.rx_sync_en = false,\
.rx_sync_ctl = false,\
}

struct sunxi_daudio_param daudio_param[] = {
	DAUDIO2_PARAMS,
	//DAUDIO1_PARAMS,
	//DAUDIO0_PARAMS,
};

struct daudio_pinctrl daudio0_pinctrl[] = {
	/*
	 * DAUDIO0_PIN_MCLK,
	 * DAUDIO0_PIN_BCLK,
	 * DAUDIO0_PIN_LRCK,
	 * DAUDIO0_PIN_DOUT0,
	 * DAUDIO0_PIN_DIN0,
	 */
};

struct daudio_pinctrl daudio1_pinctrl[] = {
	/*
	 * DAUDIO1_PIN_MCLK,
	 * DAUDIO1_PIN_BCLK,
	 * DAUDIO1_PIN_LRCK,
	 * DAUDIO1_PIN_DOUT0,
	 * DAUDIO1_PIN_DIN0,
	 */
};

struct daudio_pinctrl daudio2_pinctrl[] = {
	DAUDIO2_PIN_MCLK,
	DAUDIO2_PIN_BCLK,
	DAUDIO2_PIN_LRCK,
	DAUDIO2_PIN_DOUT0,
	DAUDIO2_PIN_DIN0,
};

struct pa_config daudio_pa_cfg[] = {
};

struct sunxi_daudio_clk {
	/* parent clk */
	hal_clk_t clk_pll_audio0_1x;
	hal_clk_t clk_pll_audio1_div5;
	/* module clk */
	hal_clk_t clk_i2s;

	hal_clk_t clk_bus;
	struct reset_control *clk_rst;
};

static inline int snd_sunxi_daudio_clk_enable(struct sunxi_daudio_clk *clk, uint8_t tdm_num)
{
	snd_print("\n");

	if (hal_reset_control_deassert(clk->clk_rst)) {
		snd_err("daudio%u clk_rst deassert failed\n", tdm_num);
		goto err_deassert_rst;
	}

	if (hal_clock_enable(clk->clk_bus)) {
		snd_err("daudio%u clk_bus enable failed\n", tdm_num);
		goto err_enable_clk_bus;
	}

	if (hal_clock_enable(clk->clk_pll_audio0_1x)) {
		snd_err("daudio%u clk_pll_audio0_1x enable failed\n", tdm_num);
		goto err_enable_clk_pll_audio0_1x;
	}
	
	if (hal_clock_enable(clk->clk_pll_audio1_div5)) {
		snd_err("daudio%u clk_pll_audio1_div5 enable failed\n", tdm_num);
		goto err_enable_clk_pll_audio1_div5;
	}

	if (hal_clock_enable(clk->clk_i2s)) {
		snd_err("daudio%u clk_i2s enable failed\n", tdm_num);
		goto err_enable_clk_i2s;
	}

	return HAL_CLK_STATUS_OK;

err_enable_clk_i2s:
	hal_clock_disable(clk->clk_pll_audio1_div5);
err_enable_clk_pll_audio1_div5:
	hal_clock_disable(clk->clk_pll_audio0_1x);
err_enable_clk_pll_audio0_1x:
	hal_clock_disable(clk->clk_bus);
err_enable_clk_bus:
	hal_reset_control_assert(clk->clk_rst);
err_deassert_rst:
	return HAL_CLK_STATUS_ERROR;
}

static inline void snd_sunxi_daudio_clk_disable(struct sunxi_daudio_clk *clk)
{
	snd_print("\n");

	hal_clock_disable(clk->clk_i2s);
	hal_clock_disable(clk->clk_pll_audio1_div5);
	hal_clock_disable(clk->clk_pll_audio0_1x);
	hal_clock_disable(clk->clk_bus);
	hal_reset_control_assert(clk->clk_rst);

	return;
}

static inline int snd_sunxi_daudio_clk_init(struct sunxi_daudio_clk *clk, uint8_t tdm_num)
{
	int ret = 0;

	snd_print("\n");

	switch (tdm_num) {
	case 0:
		clk->clk_rst = hal_reset_control_get(HAL_SUNXI_RESET, RST_APP_BUS_I2S0);
		clk->clk_bus = hal_clock_get(HAL_SUNXI_CCU, CLK_APP_BUS_I2S0);
		clk->clk_i2s = hal_clock_get(HAL_SUNXI_CCU, CLK_APP_I2S0);
		break;
#if DAUDIO_NUM_MAX > 1
	case 1:
		clk->clk_rst = hal_reset_control_get(HAL_SUNXI_RESET, RST_APP_BUS_I2S1);
		clk->clk_bus = hal_clock_get(HAL_SUNXI_CCU, CLK_APP_BUS_I2S1);
		clk->clk_i2s = hal_clock_get(HAL_SUNXI_CCU, CLK_APP_I2S1);
		break;
#endif
#if DAUDIO_NUM_MAX > 2
	case 2:
		clk->clk_rst = hal_reset_control_get(HAL_SUNXI_RESET, RST_APP_BUS_I2S2);
		clk->clk_bus = hal_clock_get(HAL_SUNXI_CCU, CLK_APP_BUS_I2S2);
		clk->clk_i2s = hal_clock_get(HAL_SUNXI_CCU, CLK_APP_I2S2);
		break;
#endif
	default:
		snd_err("tdm_num:%u overflow\n", tdm_num);
		ret = -EFAULT;
		goto err_daudio_get_clk;
	}

	/* get parent clk */
	clk->clk_pll_audio0_1x = hal_clock_get(HAL_SUNXI_CCU, CLK_APP_AUDIO0_PLL_1X_FIXED_DIV);
	if (!clk->clk_pll_audio0_1x) {
		snd_err("daudio%u clk_pll_audio0_4x get failed\n", tdm_num);
		goto err_get_clk_pll_audio0_1x;
	}

	clk->clk_pll_audio1_div5 = hal_clock_get(HAL_SUNXI_CCU, CLK_APP_AUDIO1_PLL_DIV5);
	if (!clk->clk_pll_audio1_div5) {
		snd_err("daudio%u clk_pll_audio1_div5 get failed\n", tdm_num);
		goto err_get_clk_pll_audio1_div5;
	}

	ret = snd_sunxi_daudio_clk_enable(clk, tdm_num);
	if (ret) {
		snd_err("daudio%u clk enable failed\n", tdm_num);
		ret = -EINVAL;
		goto err_clk_enable;
	}

	return HAL_CLK_STATUS_OK;

err_clk_enable:
	hal_clock_put(clk->clk_pll_audio1_div5);
err_get_clk_pll_audio1_div5:
	hal_clock_put(clk->clk_pll_audio0_1x);
err_get_clk_pll_audio0_1x:
	hal_clock_put(clk->clk_i2s);
	hal_clock_put(clk->clk_bus);
err_daudio_get_clk:
	return HAL_CLK_STATUS_ERROR;
}

static inline void snd_sunxi_daudio_clk_exit(struct sunxi_daudio_clk *clk)
{
	snd_print("\n");

	snd_sunxi_daudio_clk_disable(clk);
	hal_clock_put(clk->clk_i2s);
	hal_clock_put(clk->clk_pll_audio1_div5);
	hal_clock_put(clk->clk_pll_audio0_1x);
	hal_clock_put(clk->clk_bus);
	hal_reset_control_put(clk->clk_rst);

	return;
}

static inline int snd_sunxi_daudio_clk_set_rate(struct sunxi_daudio_clk *clk, int stream,
						unsigned int freq_in, unsigned int freq_out)
{
	snd_print("\n");

	if (freq_in % 24576000 == 0) {
		if (hal_clk_set_parent(clk->clk_i2s, clk->clk_pll_audio1_div5)) {
			snd_err("set i2s parent clk failed\n");
			return -EINVAL;
		}
	} else {
		if (hal_clk_set_parent(clk->clk_i2s, clk->clk_pll_audio0_1x)) {
			snd_err("set i2s parent clk failed\n");
			return -EINVAL;
		}
	}

	if (hal_clk_set_rate(clk->clk_i2s, freq_out)) {
		snd_err("freq : %u module clk unsupport\n", freq_out);
		return -EINVAL;
	}

	return HAL_CLK_STATUS_OK;
}

#endif	/* __SUN251IW1_DAUDIO_H_ */

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
#ifndef	__SUN251IW1_OWA_H_
#define	__SUN251IW1_OWA_H_

#define	SUNXI_OWA_MEMBASE (0x02036000)

/*add this to surport the owa receive IEC-61937 data */
#define CONFIG_SND_SUNXI_OWA_RX_IEC61937

/*------------------OWA EXP register definition--------------------*/
#define	SUNXI_OWA_EXP_CTL	0x40
#define	SUNXI_OWA_EXP_ISTA	0x44
#define	SUNXI_OWA_EXP_INFO0	0x48
#define	SUNXI_OWA_EXP_INFO1	0x4C
#define	SUNXI_OWA_EXP_DBG0	0x50
#define	SUNXI_OWA_EXP_DBG1	0x54
#define	SUNXI_OWA_EXP_VER	0x58

/* SUNXI_OWA_EXP_CTL register */
#define INSET_DET_NUM		0
#define INSET_DET_EN		8
#define SYNCW_BIT_EN		9
#define DATA_TYPE_BIT_EN	10
#define DATA_LEG_BIT_EN		11
#define AUDIO_DATA_BIT_EN	12
#define RX_MODE			13
#define RX_MODE_MAN		14
#define UNIT_SEL		15
#define RPOTBF_NUM		16
#define BURST_DATA_OUT_SEL	30

/* SUNXI_OWA_EXP_ISTA register */
#define INSET_INT		0
#define PAPB_CAP_INT		1
#define PCPD_CAP_INT		2
#define RPDB_ERR_INT		3
#define PC_DTYOE_CH_INT		4
#define PC_ERR_FLAG_INT		5
#define PC_BIT_CH_INT		6
#define PC_PAUSE_STOP_INT	7
#define PD_CHAN_INT		8
#define INSET_INT_EN		16
#define PAPB_CAP_INT_EN		17
#define PCPD_CAP_INT_EN		18
#define RPDB_ERR_INT_EN		19
#define PC_DTYOE_CH_INT_EN	20
#define PC_ERR_FLAG_INT_EN	21
#define PC_BIT_CH_INT_EN	22
#define PC_PAUSE_STOP_INT_EN	23
#define PD_CHAN_INT_EN		24

/* SUNXI_OWA_EXP_INFO0 register */
#define PD_DATA_INFO		0
#define PC_DATA_INFO		16

/* SUNXI_OWA_EXP_INFO1 register */
#define SAMPLE_RATE_VAL		0
#define RPOTBF_VAL		16

/* SUNXI_OWA_EXP_DBG0 register */
#define RE_DATA_COUNT_VAL	0
#define DATA_CAP_STA_MACHE	16

/* SUNXI_OWA_EXP_DBG1 register */
#define SAMPLE_RATE_COUNT	0
#define RPOTBF_COUNT		16

/* SUNXI_OWA_EXP_VER register */
#define MOD_VER			0

/*------------------------ PIN CONFIG FOR NORMAL ---------------------------*/
owa_gpio_t g_owa_gpio = {
	/* .clk	= {GPIOB(12), 3}, */
	//.out	= {GPIOA(17), 3},	/* use for uart0 rx */
	//.in	= {GPIOA(16), 3},	/* use for uart0 tx */
	.out	= {GPIOD(22), 2},
	//.in		= {GPIOF(2), 6},
};

/*------------------------ CLK CONFIG FOR SUN251IW1 ---------------------------*/
struct sunxi_owa_clk {
	/* parent clk */
	hal_clk_t clk_pll_audio0_1x;
	hal_clk_t clk_pll_audio1_div5;
	/* module clk */
	hal_clk_t clk_owa_tx;
	hal_clk_t clk_owa_rx;

	hal_clk_t clk_bus;
	struct reset_control *clk_rst;
};

static inline int snd_sunxi_owa_clk_enable(struct sunxi_owa_clk *clk)
{
	snd_print("\n");

	/* rst & bus */
	if (hal_reset_control_deassert(clk->clk_rst)) {
		snd_err("clk_rst deassert failed\n");
		goto err_deassert_rst;
	}

	if (hal_clock_enable(clk->clk_bus)) {
		snd_err("clk_bus enable failed\n");
		goto err_enable_clk_bus;
	}

	if (hal_clock_enable(clk->clk_pll_audio0_1x)) {
		snd_err("clk_pll_audio0_1x enable failed\n");
		goto err_enable_clk_pll_audio0_1x;
	}

	if (hal_clock_enable(clk->clk_pll_audio1_div5)) {
		snd_err("clk_pll_audio1_div5 enable failed\n");
		goto err_enable_clk_pll_audio1_div5;
	}

	if (hal_clock_enable(clk->clk_owa_tx)) {
		snd_err("clk_owa_tx enable failed\n");
		goto err_enable_clk_owa_tx;
	}

	if (hal_clock_enable(clk->clk_owa_rx)) {
		snd_err("clk_owa_rx enable failed\n");
		goto err_enable_clk_owa_rx;
	}

	return HAL_CLK_STATUS_OK;

err_enable_clk_owa_rx:
	hal_clock_disable(clk->clk_owa_tx);
err_enable_clk_owa_tx:
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

static inline void snd_sunxi_owa_clk_disable(struct sunxi_owa_clk *clk)
{
	snd_print("\n");

	hal_clock_disable(clk->clk_owa_rx);
	hal_clock_disable(clk->clk_owa_tx);
	hal_clock_disable(clk->clk_pll_audio1_div5);
	hal_clock_disable(clk->clk_pll_audio0_1x);
	hal_clock_disable(clk->clk_bus);
	hal_reset_control_assert(clk->clk_rst);

	return;
}

static inline int snd_sunxi_owa_clk_init(struct sunxi_owa_clk *clk)
{
	int ret  = 0;

	/* get rst clk */
	clk->clk_rst = hal_reset_control_get(HAL_SUNXI_RESET, RST_APP_BUS_OWA);
	if (!clk->clk_rst) {
		snd_err("codec clk_rst hal_reset_control_get failed\n");
		goto err_get_clk_rst;
	}

	/* get bus clk */
	clk->clk_bus = hal_clock_get(HAL_SUNXI_CCU, CLK_APP_BUS_OWA);
	if (!clk->clk_bus) {
		snd_err("clk_bus get failed\n");
		goto err_get_clk_bus;
	}

	/* get parent clk */
	clk->clk_pll_audio0_1x = hal_clock_get(HAL_SUNXI_CCU, CLK_APP_AUDIO0_PLL_1X_FIXED_DIV);
	if (!clk->clk_pll_audio0_1x) {
		snd_err("clk_pll_audio0_1x get failed\n");
		goto err_get_clk_pll_audio0_1x;
	}

	clk->clk_pll_audio1_div5 = hal_clock_get(HAL_SUNXI_CCU, CLK_APP_AUDIO1_PLL_DIV5);
	if (!clk->clk_pll_audio1_div5) {
		snd_err("clk_pll_audio1_div5 get failed\n");
		goto err_get_clk_pll_audio1_div5;
	}

	/* get i2s clk */
	clk->clk_owa_tx = hal_clock_get(HAL_SUNXI_CCU, CLK_APP_OWA_TX);
	if (!clk->clk_owa_tx) {
		snd_err("clk_owa_tx get failed\n");
		goto err_get_clk_owa_tx;
	}
	clk->clk_owa_rx = hal_clock_get(HAL_SUNXI_CCU, CLK_APP_OWA_RX);
	if (!clk->clk_owa_rx) {
		snd_err("clk_owa_rx get failed\n");
		goto err_get_clk_owa_rx;
	}

	ret = snd_sunxi_owa_clk_enable(clk);
	if (ret) {
		snd_err("clk enable failed\n");
		ret = -EINVAL;
		goto err_clk_enable;
	}

	return HAL_CLK_STATUS_OK;

err_clk_enable:
	hal_clock_put(clk->clk_owa_rx);
err_get_clk_owa_rx:
	hal_clock_put(clk->clk_owa_tx);
err_get_clk_owa_tx:
	hal_clock_put(clk->clk_pll_audio1_div5);
err_get_clk_pll_audio1_div5:
	hal_clock_put(clk->clk_pll_audio0_1x);
err_get_clk_pll_audio0_1x:
	hal_clock_put(clk->clk_bus);
err_get_clk_bus:
err_get_clk_rst:
	return HAL_CLK_STATUS_ERROR;
}

static inline void snd_sunxi_owa_clk_exit(struct sunxi_owa_clk *clk)
{
	snd_print("\n");

	snd_sunxi_owa_clk_disable(clk);
	hal_clock_put(clk->clk_owa_rx);
	hal_clock_put(clk->clk_owa_tx);
	hal_clock_put(clk->clk_pll_audio1_div5);
	hal_clock_put(clk->clk_pll_audio0_1x);
	hal_clock_put(clk->clk_bus);
	hal_reset_control_put(clk->clk_rst);

	return;
}

static inline int snd_sunxi_owa_clk_set_rate(struct sunxi_owa_clk *clk, int stream,
					       unsigned int freq_in, unsigned int freq_out)
{
	if (freq_in % 24576000 == 0) {
		if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
			if (hal_clk_set_parent(clk->clk_owa_tx, clk->clk_pll_audio1_div5)) {
				snd_err("set owa tx parent clk failed\n");
				return -EINVAL;
			}
			if (hal_clk_set_rate(clk->clk_owa_tx, freq_out)) {
				snd_err("clk owa tx set rate failed\n");
				return -EINVAL;
			}
		} else {
			if (hal_clk_set_parent(clk->clk_owa_rx, clk->clk_pll_audio1_div5)) {
				snd_err("set owa rx parent clk failed\n");
				return -EINVAL;
			}
			if (hal_clk_set_rate(clk->clk_owa_rx, freq_out)) {
				snd_err("clk owa tx set rate failed\n");
				return -EINVAL;
			}
		}
	} else {
		if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
			if (hal_clk_set_parent(clk->clk_owa_tx, clk->clk_pll_audio0_1x)) {
				snd_err("set owa tx parent clk failed\n");
				return -EINVAL;
			}
			if (hal_clk_set_rate(clk->clk_owa_tx, freq_out)) {
				snd_err("clk owa tx set rate failed\n");
				return -EINVAL;
			}
		} else {
			if (hal_clk_set_parent(clk->clk_owa_rx, clk->clk_pll_audio0_1x)) {
				snd_err("set owa rx parent clk failed\n");
				return -EINVAL;
			}
			if (hal_clk_set_rate(clk->clk_owa_rx, freq_out)) {
				snd_err("clk owa tx set rate failed\n");
				return -EINVAL;
			}
		}
	}

	return HAL_CLK_STATUS_OK;
}

#endif /* __SUN251IW1_OWA_H_ */

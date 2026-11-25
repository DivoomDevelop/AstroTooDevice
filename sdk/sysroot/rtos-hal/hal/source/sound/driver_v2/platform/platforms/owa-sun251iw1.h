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

#ifndef	__SUN251IW1_OWA_H_
#define	__SUN251IW1_OWA_H_

#define	SUNXI_OWA_MEMBASE (0x02036000)

/*add this to surport the owa loopback */
#define CONFIG_SOUND_SUNXI_OWA_LOOPBACK

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

static inline struct sunxi_owa_clk *sunxi_owa_clk_init(void)
{
	int ret  = 0;
	struct sunxi_owa_clk *clk = NULL;

	SOUND_LOG_DEBUG("\n");

	clk = sound_malloc(sizeof(*clk));
	if (!clk) {
		SOUND_LOG_ERR("can't allocate sunxi_owa_clk memory\n");
		return NULL;
	}

	/* get rst clk */
	clk->clk_rst = hal_reset_control_get(HAL_SUNXI_RESET, RST_APP_BUS_OWA);
	if (!clk->clk_rst) {
		SOUND_LOG_ERR("codec clk_rst hal_reset_control_get failed\n");
		goto err_get_clk_rst;
	}

	/* get bus clk */
	clk->clk_bus = hal_clock_get(HAL_SUNXI_CCU, CLK_APP_BUS_OWA);
	if (!clk->clk_bus) {
		SOUND_LOG_ERR("clk_bus get failed\n");
		goto err_get_clk_bus;
	}

	/* get parent clk */
	clk->clk_pll_audio0_1x = hal_clock_get(HAL_SUNXI_CCU, CLK_APP_AUDIO0_PLL_1X_FIXED_DIV);
	if (!clk->clk_pll_audio0_1x) {
		SOUND_LOG_ERR("clk_pll_audio0_1x get failed\n");
		goto err_get_clk_pll_audio0_1x;
	}

	clk->clk_pll_audio1_div5 = hal_clock_get(HAL_SUNXI_CCU, CLK_APP_AUDIO1_PLL_DIV5);
	if (!clk->clk_pll_audio1_div5) {
		SOUND_LOG_ERR("clk_pll_audio1_div5 get failed\n");
		goto err_get_clk_pll_audio1_div5;
	}

	/* get i2s clk */
	clk->clk_owa_tx = hal_clock_get(HAL_SUNXI_CCU, CLK_APP_OWA_TX);
	if (!clk->clk_owa_tx) {
		SOUND_LOG_ERR("clk_owa_tx get failed\n");
		goto err_get_clk_owa_tx;
	}
	clk->clk_owa_rx = hal_clock_get(HAL_SUNXI_CCU, CLK_APP_OWA_RX);
	if (!clk->clk_owa_rx) {
		SOUND_LOG_ERR("clk_owa_rx get failed\n");
		goto err_get_clk_owa_rx;
	}

	return clk;

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
	sound_free(clk);
	return NULL;
}

static inline void sunxi_owa_clk_exit(struct sunxi_owa_clk *clk)
{
	SOUND_LOG_DEBUG("\n");

	hal_clock_put(clk->clk_owa_rx);
	hal_clock_put(clk->clk_owa_tx);
	hal_clock_put(clk->clk_pll_audio1_div5);
	hal_clock_put(clk->clk_pll_audio0_1x);
	hal_clock_put(clk->clk_bus);
	hal_reset_control_put(clk->clk_rst);

	sound_free(clk);

	return;
}

static inline int sunxi_owa_clk_bus_enable(struct sunxi_owa_clk *clk)
{
	SOUND_LOG_DEBUG("\n");

	/* rst & bus */
	if (hal_reset_control_deassert(clk->clk_rst)) {
		SOUND_LOG_ERR("clk_rst deassert failed\n");
		goto err_deassert_rst;
	}

	if (hal_clock_enable(clk->clk_bus)) {
		SOUND_LOG_ERR("clk_bus enable failed\n");
		goto err_enable_clk_bus;
	}

	return HAL_CLK_STATUS_OK;

err_enable_clk_bus:
	hal_reset_control_assert(clk->clk_rst);
err_deassert_rst:
	return HAL_CLK_STATUS_ERROR;
}

static inline int sunxi_owa_clk_enable(struct sunxi_owa_clk *clk)
{
	SOUND_LOG_DEBUG("\n");

	if (hal_clock_enable(clk->clk_pll_audio0_1x)) {
		SOUND_LOG_ERR("clk_pll_audio0_1x enable failed\n");
		goto err_enable_clk_pll_audio0_1x;
	}

	if (hal_clock_enable(clk->clk_pll_audio1_div5)) {
		SOUND_LOG_ERR("clk_pll_audio1_div5 enable failed\n");
		goto err_enable_clk_pll_audio1_div5;
	}

	if (hal_clock_enable(clk->clk_owa_tx)) {
		SOUND_LOG_ERR("clk_owa_tx enable failed\n");
		goto err_enable_clk_owa_tx;
	}

	if (hal_clock_enable(clk->clk_owa_rx)) {
		SOUND_LOG_ERR("clk_owa_rx enable failed\n");
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
	return HAL_CLK_STATUS_ERROR;
}

static inline void sunxi_owa_clk_bus_disable(struct sunxi_owa_clk *clk)
{
	SOUND_LOG_DEBUG("\n");

	hal_clock_disable(clk->clk_bus);
	hal_reset_control_assert(clk->clk_rst);

	return;
}

static inline void sunxi_owa_clk_disable(struct sunxi_owa_clk *clk)
{
	SOUND_LOG_DEBUG("\n");

	hal_clock_disable(clk->clk_owa_rx);
	hal_clock_disable(clk->clk_owa_tx);
	hal_clock_disable(clk->clk_pll_audio1_div5);
	hal_clock_disable(clk->clk_pll_audio0_1x);

	return;
}

static inline int sunxi_owa_clk_set_rate(struct sunxi_owa_clk *clk, int stream,
					       unsigned int freq_in, unsigned int freq_out)
{
	if (freq_in % 24576000 == 0) {
		if (stream == SOUNDRV_PCM_STREAM_PLAYBACK) {
			if (hal_clk_set_parent(clk->clk_owa_tx, clk->clk_pll_audio1_div5)) {
				SOUND_LOG_ERR("set owa tx parent clk failed\n");
				return -EINVAL;
			}
			if (hal_clk_set_rate(clk->clk_owa_tx, freq_out)) {
				SOUND_LOG_ERR("clk owa tx set rate failed\n");
				return -EINVAL;
			}
		} else {
			if (hal_clk_set_parent(clk->clk_owa_rx, clk->clk_pll_audio1_div5)) {
				SOUND_LOG_ERR("set owa rx parent clk failed\n");
				return -EINVAL;
			}
			if (hal_clk_set_rate(clk->clk_owa_rx, freq_out)) {
				SOUND_LOG_ERR("clk owa tx set rate failed\n");
				return -EINVAL;
			}
		}
	} else {
		if (stream == SOUNDRV_PCM_STREAM_PLAYBACK) {
			if (hal_clk_set_parent(clk->clk_owa_tx, clk->clk_pll_audio0_1x)) {
				SOUND_LOG_ERR("set owa tx parent clk failed\n");
				return -EINVAL;
			}
			if (hal_clk_set_rate(clk->clk_owa_tx, freq_out)) {
				SOUND_LOG_ERR("clk owa tx set rate failed\n");
				return -EINVAL;
			}
		} else {
			if (hal_clk_set_parent(clk->clk_owa_rx, clk->clk_pll_audio0_1x)) {
				SOUND_LOG_ERR("set owa rx parent clk failed\n");
				return -EINVAL;
			}
			if (hal_clk_set_rate(clk->clk_owa_rx, freq_out)) {
				SOUND_LOG_ERR("clk owa tx set rate failed\n");
				return -EINVAL;
			}
		}
	}

	return HAL_CLK_STATUS_OK;
}

#endif /* __SUN251IW1_OWA_H_ */

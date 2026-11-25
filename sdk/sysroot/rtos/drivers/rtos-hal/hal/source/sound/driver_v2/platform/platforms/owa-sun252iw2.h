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

#ifndef	__SUN252IW2_OWA_H_
#define	__SUN252IW2_OWA_H_

#define	SUNXI_OWA_MEMBASE (0x02036000)

/*------------------------ CLK CONFIG FOR SUN251IW1 ---------------------------*/
struct sunxi_owa_clk {
	/* parent clk */
	hal_clk_t clk_pll_audio1_div5;
	/* module clk */
	hal_clk_t clk_owa_tx;

	hal_clk_t clk_bus;
	struct reset_control *clk_rst;
};

static inline struct sunxi_owa_clk *sunxi_owa_clk_init(void)
{
	struct sunxi_owa_clk *clk = NULL;

	SOUND_LOG_DEBUG("\n");

	clk = sound_malloc(sizeof(*clk));
	if (!clk) {
		SOUND_LOG_ERR("can't allocate sunxi_owa_clk memory\n");
		return NULL;
	}

	/* get rst clk */
	clk->clk_rst = hal_reset_control_get(HAL_SUNXI_RESET, RST_BUS_OWA);
	if (!clk->clk_rst) {
		SOUND_LOG_ERR("codec clk_rst hal_reset_control_get failed\n");
		goto err_get_clk_rst;
	}

	/* get bus clk */
	clk->clk_bus = hal_clock_get(HAL_SUNXI_CCU, CLK_BUS_OWA);
	if (!clk->clk_bus) {
		SOUND_LOG_ERR("clk_bus get failed\n");
		goto err_get_clk_bus;
	}

	/* get parent clk */
	clk->clk_pll_audio1_div5 = hal_clock_get(HAL_SUNXI_CCU, CLK_PLL_AUDIO1_DIV5);
	if (!clk->clk_pll_audio1_div5) {
		SOUND_LOG_ERR("clk_pll_audio1_div5 get failed\n");
		goto err_get_clk_pll_audio1_div5;
	}

	/* get i2s clk */
	clk->clk_owa_tx = hal_clock_get(HAL_SUNXI_CCU, CLK_OWA_TX);
	if (!clk->clk_owa_tx) {
		SOUND_LOG_ERR("clk_owa_tx get failed\n");
		goto err_get_clk_owa_tx;
	}

	return clk;

err_get_clk_owa_tx:
	hal_clock_put(clk->clk_pll_audio1_div5);
err_get_clk_pll_audio1_div5:
	hal_clock_put(clk->clk_bus);
err_get_clk_bus:
    hal_reset_control_put(clk->clk_rst);
err_get_clk_rst:
	sound_free(clk);
	return NULL;
}

static inline void sunxi_owa_clk_exit(struct sunxi_owa_clk *clk)
{
	SOUND_LOG_DEBUG("\n");

	hal_clock_put(clk->clk_owa_tx);
	hal_clock_put(clk->clk_pll_audio1_div5);
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

	if (hal_clock_enable(clk->clk_pll_audio1_div5)) {
		SOUND_LOG_ERR("clk_pll_audio1_div5 enable failed\n");
		goto err_enable_clk_pll_audio1_div5;
	}

	if (hal_clock_enable(clk->clk_owa_tx)) {
		SOUND_LOG_ERR("clk_owa_tx enable failed\n");
		goto err_enable_clk_owa_tx;
	}

	return HAL_CLK_STATUS_OK;

err_enable_clk_owa_tx:
	hal_clock_disable(clk->clk_pll_audio1_div5);
err_enable_clk_pll_audio1_div5:
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

	hal_clock_disable(clk->clk_owa_tx);
	hal_clock_disable(clk->clk_pll_audio1_div5);

	return;
}

static inline int sunxi_owa_clk_set_rate(struct sunxi_owa_clk *clk, int stream,
					       unsigned int freq_in, unsigned int freq_out)
{
	SOUND_LOG_DEBUG("\n");

	if (freq_in % 24576000 != 0) {
		SOUND_LOG_ERR("sun252iw2 platform only support 48k series sample rate.\n");
		return HAL_CLK_STATUS_ERROR;
	}

	if (hal_clk_set_parent(clk->clk_owa_tx, clk->clk_pll_audio1_div5)) {
		SOUND_LOG_ERR("set owa tx parent clk failed\n");
		return -HAL_CLK_STATUS_ERROR;
	}
	if (hal_clk_set_rate(clk->clk_owa_tx, freq_out)) {
		SOUND_LOG_ERR("clk owa tx set rate failed\n");
		return -HAL_CLK_STATUS_ERROR;
	}

	return HAL_CLK_STATUS_OK;
}

#endif /* __SUN252IW2_OWA_H_ */

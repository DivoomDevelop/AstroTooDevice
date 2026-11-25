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

#ifndef	__SUN55IW6_DMIC_H_
#define	__SUN55IW6_DMIC_H_

#define	SUNXI_DMIC_MEMBASE (0x02031000)

struct sunxi_dmic_clk {
	/* parent clk */
	hal_clk_t clk_pll_audio0_4x;
    hal_clk_t clk_pll_audio1_4x;
	/* module clk */
	hal_clk_t clk_dmic;

	hal_clk_t clk_bus;
	struct reset_control *clk_rst;
};

static inline struct sunxi_dmic_clk *sunxi_dmic_clk_init(void)
{
	struct sunxi_dmic_clk *clk = NULL;

	SOUND_LOG_DEBUG("\n");

	clk = sound_malloc(sizeof(*clk));
	if (!clk) {
		SOUND_LOG_ERR("can't allocate sunxi_dmic_clk memory\n");
		return NULL;
	}

	/* get rst clk */
	clk->clk_rst = hal_reset_control_get(HAL_SUNXI_RESET, RST_BUS_DMIC);
	if (!clk->clk_rst) {
		SOUND_LOG_ERR("dmic clk rst get failed\n");
		goto err_get_clk_rst;
	}

	/* get bus clk */
	clk->clk_bus = hal_clock_get(HAL_SUNXI_CCU, CLK_BUS_DMIC);
	if (!clk->clk_bus) {
		SOUND_LOG_ERR("clk_bus get failed\n");
		goto err_get_clk_bus;
	}

	/* get parent clk */
	clk->clk_pll_audio0_4x = hal_clock_get(HAL_SUNXI_CCU, CLK_PLL_AUDIO0_4X);
	if (!clk->clk_pll_audio0_4x) {
		SOUND_LOG_ERR("clk_pll_audio0_4x get failed\n");
		goto err_get_clk_pll_audio0_4x;
	}
	clk->clk_pll_audio1_4x = hal_clock_get(HAL_SUNXI_CCU, CLK_PLL_AUDIO1_4X);
	if (!clk->clk_pll_audio1_4x) {
		SOUND_LOG_ERR("clk_pll_audio1_4x get failed\n");
		goto err_get_clk_pll_audio1_4x;
	}

	/* get dmic clk */
	clk->clk_dmic = hal_clock_get(HAL_SUNXI_CCU, CLK_DMIC);
	if (!clk->clk_dmic) {
		SOUND_LOG_ERR("clk_dmic get failed\n");
		goto err_get_clk_dmic;
	}

	return clk;

err_get_clk_dmic:
	hal_clock_put(clk->clk_pll_audio1_4x);
err_get_clk_pll_audio1_4x:
    hal_clock_put(clk->clk_pll_audio0_4x);
err_get_clk_pll_audio0_4x:
	hal_clock_put(clk->clk_bus);
err_get_clk_bus:
err_get_clk_rst:
	return NULL;
}

static inline void sunxi_dmic_clk_exit(struct sunxi_dmic_clk *clk)
{
	SOUND_LOG_DEBUG("\n");

	hal_clock_put(clk->clk_dmic);
	hal_clock_put(clk->clk_pll_audio0_4x);
    hal_clock_put(clk->clk_pll_audio1_4x);
	hal_clock_put(clk->clk_bus);
	hal_reset_control_put(clk->clk_rst);

	return;
}

static inline int sunxi_dmic_clk_bus_enable(struct sunxi_dmic_clk *clk)
{
	SOUND_LOG_DEBUG("\n");

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

static inline int sunxi_dmic_clk_enable(struct sunxi_dmic_clk *clk)
{
	SOUND_LOG_DEBUG("\n");

	if (hal_clock_enable(clk->clk_pll_audio0_4x)) {
		SOUND_LOG_ERR("clk_pll_audio0_4x enable failed\n");
		goto err_enable_clk_pll_audio0_4x;
	}
	if (hal_clock_enable(clk->clk_pll_audio1_4x)) {
		SOUND_LOG_ERR("clk_pll_audio0_4x enable failed\n");
		goto err_enable_clk_pll_audio1_4x;
	}

	if (hal_clock_enable(clk->clk_dmic)) {
		SOUND_LOG_ERR("clk_dmic enable failed\n");
		goto err_enable_clk_dmic;
	}

	return HAL_CLK_STATUS_OK;

err_enable_clk_dmic:
	hal_clock_disable(clk->clk_pll_audio1_4x);
err_enable_clk_pll_audio1_4x:
    hal_clock_disable(clk->clk_pll_audio0_4x);
err_enable_clk_pll_audio0_4x:
	return HAL_CLK_STATUS_ERROR;
}

static inline void sunxi_dmic_clk_bus_disable(struct sunxi_dmic_clk *clk)
{
	SOUND_LOG_DEBUG("\n");

	hal_clock_disable(clk->clk_bus);
	hal_reset_control_assert(clk->clk_rst);

	return;
}

static inline void sunxi_dmic_clk_disable(struct sunxi_dmic_clk *clk)
{
	SOUND_LOG_DEBUG("\n");

	hal_clock_disable(clk->clk_dmic);
	hal_clock_disable(clk->clk_pll_audio0_4x);
    hal_clock_disable(clk->clk_pll_audio1_4x);

	return;
}

static inline int sunxi_dmic_clk_set_rate(struct sunxi_dmic_clk *clk, int stream,
					      unsigned int freq_in, unsigned int freq_out)
{
	if (freq_in % 24576000 == 0) {
		if (hal_clk_set_parent(clk->clk_dmic, clk->clk_pll_audio0_4x)) {
			SOUND_LOG_ERR("set dmic parent clk failed\n");
			return -EINVAL;
		}
	} else {
		if (hal_clk_set_parent(clk->clk_dmic, clk->clk_pll_audio1_4x)) {
			SOUND_LOG_ERR("set dmic parent clk failed\n");
			return -EINVAL;
		}
	}

	if (hal_clk_set_rate(clk->clk_dmic, freq_out)) {
		SOUND_LOG_ERR("set clk_dmic rate failed, rate: %u\n", freq_out);
		return -EINVAL;
	}

	return HAL_CLK_STATUS_OK;
}

#endif /* __SUN55IW6_DMIC_H_ */

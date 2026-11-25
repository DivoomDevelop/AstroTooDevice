#ifndef __IRRX_SUN252IW2_H__
#define __IRRX_SUN252IW2_H__

#include <hal_interrupt.h>
#include <hal_clk.h>

/* CPUS IRRX config */
#define IRRX_CPUS_BASE		0x02087000

#define IRRX_CPUS_IRQ		38


static struct sunxi_irrx_params_t g_sunxi_irrx_params[] = {
	/* s_irrx */
	{	.port = 0,
		.reg_base = IRRX_CPUS_BASE, .irq_num = IRRX_CPUS_IRQ,
		.clk_type = HAL_SUNXI_CCU, .bus_clk_id = CLK_BUS_IRRX,
		.mclk_id = CLK_IRRX, .pclk_type = AW_SRC_CCU,
		.pclk_id = CLK_SRC_HOSC, .reset_type = HAL_SUNXI_RESET,
		.reset_id = RST_BUS_IRRX, .gpio_name = GPIOD(7),
		.enable_mux = 3, .disable_mux = 0,
	},
};

#define CIR_MAX_NUM ARRAY_SIZE(g_sunxi_irrx_params)

#endif /* __IRRX_SUN55IW6_H__ */

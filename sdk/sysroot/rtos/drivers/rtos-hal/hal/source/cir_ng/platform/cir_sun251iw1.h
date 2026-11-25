#ifndef __IRRX_SUN251IW1_H__
#define __IRRX_SUN251IW1_H__

#include <hal_interrupt.h>
#include <hal_clk.h>

/* CPUS IRRX config */
#define IRRX_CPUS_BASE		0x07040000

#define IRRX_CPUS_IRQ		167


static struct sunxi_irrx_params_t g_sunxi_irrx_params[] = {
	/* s_irrx */
	{	.port = 0,
		.reg_base = IRRX_CPUS_BASE, .irq_num = IRRX_CPUS_IRQ,
		.clk_type = AW_PRCM_CCU, .bus_clk_id = CLK_PRCM_R_APB0_BUS_IRRX,
		.mclk_id = CLK_PRCM_R_APB0_IRRX, .pclk_type = AW_SRC_CCU,
		.pclk_id = CLK_SRC_HOSC, .reset_type = AW_PRCM_CCU,
		.reset_id = RST_PRCM_BUS_R_IRRX, .gpio_name = GPIOB(7),
		.enable_mux = 5, .disable_mux = 0,
	},
};

#define CIR_MAX_NUM ARRAY_SIZE(g_sunxi_irrx_params)

#endif /* __IRRX_SUN55IW6_H__ */

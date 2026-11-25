#ifndef __IRRX_SUN55IW6_H__
#define __IRRX_SUN55IW6_H__

#include <hal_interrupt.h>
#include <hal_clk.h>

/* CPUX IRRX config */
#define IRRX0_CPUX_BASE		0x02081000
#if defined(CONFIG_ARCH_RISCV)
#define IRRX0_CPUX_IRQ		MAKE_IRQn(43, 0)
#endif

#define IRRX1_CPUX_BASE		0x02082000
#if defined(CONFIG_ARCH_RISCV)
#define IRRX1_CPUX_IRQ		MAKE_IRQn(133, 0)
#endif

#define IRRX2_CPUX_BASE		0x02083000
#if defined(CONFIG_ARCH_RISCV)
#define IRRX2_CPUX_IRQ		MAKE_IRQn(134, 0)
#endif

#define IRRX3_CPUX_BASE		0x02084000
#if defined(CONFIG_ARCH_RISCV)
#define IRRX3_CPUX_IRQ		MAKE_IRQn(135, 0)
#endif

/* CPUS IRRX config */
#define IRRX_CPUS_BASE		0x07040000
#if defined(CONFIG_ARCH_RISCV)
#define IRRX_CPUS_IRQ		MAKE_IRQn(226, 0)
#endif

static struct sunxi_irrx_params_t g_sunxi_irrx_params[] = {
	/* irrx0 */
	{	.port = 0,
		.reg_base = IRRX0_CPUX_BASE, .irq_num = IRRX0_CPUX_IRQ,
		.clk_type = HAL_SUNXI_CCU, .bus_clk_id = CLK_BUS_IRRX0,
		.mclk_id = CLK_IRRX0, .pclk_type = HAL_SUNXI_FIXED_CCU,
		.pclk_id = CLK_SRC_HOSC24M, .reset_type = HAL_SUNXI_RESET,
		.reset_id = RST_BUS_IRRX0, .gpio_name = GPIOH(15),
		.enable_mux = 2, .disable_mux = 0,
	},

	/* irrx1 */
	{	.port = 1,
		.reg_base = IRRX1_CPUX_BASE, .irq_num = IRRX1_CPUX_IRQ,
		.clk_type = HAL_SUNXI_CCU, .bus_clk_id = CLK_BUS_IRRX1,
		.mclk_id = CLK_IRRX1, .pclk_type = HAL_SUNXI_FIXED_CCU,
		.pclk_id = CLK_SRC_HOSC24M, .reset_type = HAL_SUNXI_RESET,
		.reset_id = RST_BUS_IRRX1, .gpio_name = GPIOH(12),
		.enable_mux = 2, .disable_mux = 0,
	},

	/* irrx2 */
	{	.port = 2,
		.reg_base = IRRX2_CPUX_BASE, .irq_num = IRRX2_CPUX_IRQ,
		.clk_type = HAL_SUNXI_CCU, .bus_clk_id = CLK_BUS_IRRX2,
		.mclk_id = CLK_IRRX2, .pclk_type = HAL_SUNXI_FIXED_CCU,
		.pclk_id = CLK_SRC_HOSC24M, .reset_type = HAL_SUNXI_RESET,
		.reset_id = RST_BUS_IRRX2, .gpio_name = GPIOH(13),
		.enable_mux = 2, .disable_mux = 0,
	},

	/* irrx3 */
	{	.port = 3,
		.reg_base = IRRX3_CPUX_BASE, .irq_num = IRRX3_CPUX_IRQ,
		.clk_type = HAL_SUNXI_CCU, .bus_clk_id = CLK_BUS_IRRX3,
		.mclk_id = CLK_IRRX3, .pclk_type = HAL_SUNXI_FIXED_CCU,
		.pclk_id = CLK_SRC_HOSC24M, .reset_type = HAL_SUNXI_RESET,
		.reset_id = RST_BUS_IRRX3, .gpio_name = GPIOH(11),
		.enable_mux = 2, .disable_mux = 0,
	},

	/* s_irrx */
	{	.port = 4,
		.reg_base = IRRX_CPUS_BASE, .irq_num = IRRX_CPUS_IRQ,
		.clk_type = HAL_SUNXI_R_CCU, .bus_clk_id = CLK_R_BUS_IRRX,
		.mclk_id = CLK_R_IRRX, .pclk_type = HAL_SUNXI_FIXED_CCU,
		.pclk_id = CLK_SRC_HOSC24M, .reset_type = HAL_SUNXI_R_RESET,
		.reset_id = RST_BUS_R_IRRX, .gpio_name = GPIOL(11),
		.enable_mux = 2, .disable_mux = 0,
	},
};

#define CIR_MAX_NUM ARRAY_SIZE(g_sunxi_irrx_params)

#endif /* __IRRX_SUN55IW6_H__ */

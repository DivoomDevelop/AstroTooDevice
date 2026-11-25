/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY��S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS��SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY��S TECHNOLOGY.
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

#ifndef __SPI_SUN251IW1_H__
#define __SPI_SUN251IW1_H__

#include <hal_clk.h>

#define SUNXI_SPI0_PBASE		(0x04025000ul) /* 4K */
#define SUNXI_SPI1_PBASE		(0x04026000ul) /* 4K */

#ifdef CONFIG_ARCH_RISCV
#define SUNXI_IRQ_SPI0		31
#define SUNXI_IRQ_SPI1		32
#endif

__attribute__((__unused__)) static struct sunxi_spi_params_t g_sunxi_spi_params[] = {
	/* SPI0 */
	{	.port = 0,
		.reg_base = SUNXI_SPI0_PBASE, .irq_num = SUNXI_IRQ_SPI0, .gpio_num = 6,
		.pclk_pll_type = AW_APP_CCU, .pclk_pll_id = CLK_APP_PERI_PLL_1X_FIXED_DIV,
		.pclk_hosc_type = AW_SRC_CCU, .pclk_hosc_id = CLK_SRC_HOSC,
		.bus_type = AW_APP_CCU, .bus_id = CLK_APP_BUS_SPI0,
		.mclk_type = AW_APP_CCU, .mclk_id = CLK_APP_SPI0,
		.reset_type = AW_APP_CCU, .reset_id = RST_APP_BUS_SPI0,
		.gpio_clk = GPIOC(0), .gpio_mosi = GPIOC(3), .gpio_miso = GPIOC(2),
		.gpio_cs0 = GPIOC(1), .gpio_wp = GPIOC(4), .gpio_hold = GPIOC(5),
		.mux = 2, .driv_level = GPIO_DRIVING_LEVEL2,
	#if defined(CONFIG_DRIVERS_DMA)
		.drq_tx = DRQDST_SPI0_TX, .drq_rx = DRQSRC_SPI0_RX,
	#endif
		.rx_fifosize = 64, .tx_fifosize = 64, .dma_force_fixed = false,
	},
	/* SPI1 */
	{	.port = 1,
		.reg_base = SUNXI_SPI1_PBASE, .irq_num = SUNXI_IRQ_SPI1, .gpio_num = 6,
		.pclk_pll_type = AW_APP_CCU, .pclk_pll_id = CLK_APP_PERI_PLL_1X_FIXED_DIV,
		.pclk_hosc_type = AW_SRC_CCU, .pclk_hosc_id = CLK_SRC_HOSC,
		.bus_type = AW_APP_CCU, .bus_id = CLK_APP_BUS_SPI1,
		.mclk_type = AW_APP_CCU, .mclk_id = CLK_APP_SPI1,
		.reset_type = AW_APP_CCU, .reset_id = RST_APP_BUS_SPI1,
		.gpio_clk = GPIOD(11), .gpio_mosi = GPIOD(12), .gpio_miso = GPIOD(13),
		.gpio_cs0 = GPIOD(10), .gpio_wp = GPIOD(15), .gpio_hold = GPIOD(14),
		.mux = 4, .driv_level = GPIO_DRIVING_LEVEL2,
	#if defined(CONFIG_DRIVERS_DMA)
		.drq_tx = DRQDST_SPI1_TX, .drq_rx = DRQSRC_SPI1_RX,
	#endif
		.rx_fifosize = 64, .tx_fifosize = 64, .dma_force_fixed = false,
	},
};

#define SPI_MAX_NUM ARRAY_SIZE(g_sunxi_spi_params)

#endif /* __SPI_SUN251IW1_H__ */

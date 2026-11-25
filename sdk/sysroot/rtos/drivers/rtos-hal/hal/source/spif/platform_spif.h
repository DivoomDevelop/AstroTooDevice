/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
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

#ifndef __PLATFORM_SPIF_H__
#define __PLATFORM_SPIF_H__

#include <hal_gpio.h>
#include <hal_clk.h>
#include <hal_reset.h>

#define SPIF_MIN_GPIO_NUM (4)
#define SPIF_MAX_GPIO_NUM (10)

struct sunxi_spif_params_t {
	uint8_t port;
	unsigned long reg_base;
	uint32_t irq_num;
	/* clock */
	hal_clk_type_t pclk_pll_type;
	hal_clk_id_t pclk_pll_id;
	hal_clk_type_t pclk_hosc_type;
	hal_clk_id_t pclk_hosc_id;
	hal_clk_type_t bus_type;
	hal_clk_id_t bus_id;
	hal_clk_type_t mclk_type;
	hal_clk_id_t mclk_id;
	hal_reset_type_t reset_type;
	u32 reset_id;
	/* gpio */
	uint8_t gpio_num;
	gpio_pin_t gpio_clk;
	gpio_pin_t gpio_mosi;
	gpio_pin_t gpio_miso;
	gpio_pin_t gpio_cs0;
	gpio_pin_t gpio_wp;
	gpio_pin_t gpio_hold;
	gpio_muxsel_t mux;
	gpio_driving_level_t driv_level;
	/* fifo size */
	uint8_t rx_fifosize;
	uint8_t tx_fifosize;
	/* mode */
	u32 mode;
#define SPIF_TX_BYTE     BIT(1)                  // transmit with 1 wire byte
#define SPIF_TX_DUAL     BIT(2)                  // transmit with 2 wires
#define SPIF_TX_QUAD     BIT(3)                 // transmit with 4 wires
#define SPIF_RX_SLOW     BIT(4)                 // receive with 1 wire slow
#define SPIF_RX_DUAL     BIT(5)                 // receive with 2 wires
#define SPIF_RX_QUAD     BIT(6)                 // receive with 4 wires
#define SPIF_TX_OCTAL    BIT(7)                 // transmit with 8 wires
#define SPIF_RX_OCTAL    BIT(8)                 // receive with 8 wires */
#define SPIF_DTR_MODE    BIT(9)                 /* dtr mode */
#define SPIF_IO_MODE     BIT(10)                 /* 1-x-x & x-x-x mode */

	/* frequency */
	u32 clock_frequency;
};

#if defined(CONFIG_ARCH_SUN252IW2)
#include "platform/spi_sun252iw2.h"
#endif

#endif /* __PLATFORM_SPI_H__ */

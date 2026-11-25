/*
* Allwinner GMAC driver.
*
* Copyright(c) 2022-2027 Allwinnertech Co., Ltd.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#ifndef __GMAC_SUN251W1_H__
#define __GMAC_SUN251W1_H__

#include <hal_gpio.h>

#define SUNXI_ETH0_PBASE	0x04500000UL
#define SUNXI_ETH0_SYSCFG	0x03000030UL
#define SUNXI_ETH0_IRQ		62

#define SUNXI_ETH1_PBASE	0x04510000UL
#define SUNXI_ETH1_SYSCFG	0x03000034UL
#define SUNXI_ETH1_IRQ		55

__attribute__((__unused__)) static const struct sunxi_eth_platform sunxi_eth_plat[] = {
	{
		.port = 0, .type = SUNXI_ETH_TYPE_GMAC,
		.io_base = SUNXI_ETH0_PBASE, .syscfg_base = SUNXI_ETH0_SYSCFG, .irq_num = SUNXI_ETH0_IRQ,
		.gpio = GPIOE(0), .gpio_num = 11, .mux = 8, .driv_level = GPIO_DRIVING_LEVEL3,
		.phy_rst = GPIOE(11), .phy_rst_delay = { 0, 10000, 15000 },
		.eth_clk_type = AW_APP_CCU, .eth_clk_id = CLK_APP_BUS_GMAC0,
		.phy_clk_type = AW_APP_CCU, .phy_clk_id = CLK_APP_GMAC0_PHY,
		.ahb_rst_type = AW_APP_CCU, .ahb_rst_id = RST_APP_BUS_GMAC0,
		.interface = PHY_INTERFACE_MODE_RMII, .speed = SPEED_100, .duplex = DUPLEX_FULL,
		.tx_delay = 0, .rx_delay = 0,
		.soc_phy_clk_en = true,
	},
	{
		.port = 1, .type = SUNXI_ETH_TYPE_GMAC,
		.io_base = SUNXI_ETH1_PBASE, .syscfg_base = SUNXI_ETH1_SYSCFG, .irq_num = SUNXI_ETH1_IRQ,
		.gpio = GPIOG(0), .gpio_num = 11, .mux = 4, .driv_level = GPIO_DRIVING_LEVEL3,
		.phy_rst = GPIOG(11), .phy_rst_delay = { 0, 10000, 15000 },
		.eth_clk_type = AW_APP_CCU, .eth_clk_id = CLK_APP_BUS_GMAC1,
		.phy_clk_type = AW_APP_CCU, .phy_clk_id = CLK_APP_GMAC1_PHY,
		.ahb_rst_type = AW_APP_CCU, .ahb_rst_id = RST_APP_BUS_GMAC1,
		.interface = PHY_INTERFACE_MODE_RMII, .speed = SPEED_100, .duplex = DUPLEX_FULL,
		.tx_delay = 0, .rx_delay = 0,
		.soc_phy_clk_en = true,
	},
};

#define SUNXI_ETH_MAX_NUM ARRAY_SIZE(sunxi_eth_plat)

#endif /* __GMAC_SUN251W1_H__ */

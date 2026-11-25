/*
* Allwinner GMAC driver.
*
* Copyright(c) 2022-2027 Allwinnertech Co., Ltd.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#ifndef __GMAC_SUN8IW20_H__
#define __GMAC_SUN8IW20_H__

#include <hal_gpio.h>

#define SUNXI_ETH0_PBASE	0x04500000
#define SUNXI_ETH0_SYSCFG	0x03000030
#define SUNXI_ETH0_IRQ	62

__attribute__((__unused__)) static const struct sunxi_eth_platform sunxi_eth_plat[] = {
	{
		.port = 0, .type = SUNXI_ETH_TYPE_GMAC,
		.io_base = SUNXI_ETH0_PBASE, .syscfg_base = SUNXI_ETH0_SYSCFG, .irq_num = SUNXI_ETH0_IRQ,
		.gpio = GPIOE(0), .gpio_num = 10, .mux = 8, .driv_level = GPIO_DRIVING_LEVEL3,
		.phy_rst = GPIOE(13), .phy_rst_delay = { 0, 10000, 15000 },
		.eth_clk_type = HAL_SUNXI_CCU, .eth_clk_id = CLK_BUS_EMAC0,
		.phy_clk_type = HAL_SUNXI_CCU, .phy_clk_id = CLK_EMAC0_25M,
		.ahb_rst_type = HAL_SUNXI_RESET, .ahb_rst_id = RST_BUS_EMAC0,
		.interface = PHY_INTERFACE_MODE_RMII, .speed = SPEED_100, .duplex = DUPLEX_FULL,
		.tx_delay = 0, .rx_delay = 0,
		.soc_phy_clk_en = true,
	},
};

#define SUNXI_ETH_MAX_NUM ARRAY_SIZE(sunxi_eth_plat)

#endif /* __GMAC_SUN8IW20_H__ */

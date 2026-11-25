/*
* Allwinner GMAC driver.
*
* Copyright(c) 2022-2027 Allwinnertech Co., Ltd.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#ifndef __GMAC_SUN55IW6_H__
#define __GMAC_SUN55IW6_H__

#define SUNXI_ETH0_PBASE	(0x04500000ul)
#define SUNXI_ETH0_SYSCFG	(0x04508000ul)
#define SUNXI_ETH0_IRQ		(46)

#define SUNXI_ETH1_PBASE	(0x04510000ul)
#define SUNXI_ETH1_SYSCFG	(0x04518000ul)
#define SUNXI_ETH1_IRQ		(47)

__attribute__((__unused__)) static const struct sunxi_eth_platform sunxi_eth_plat[] = {
	{
		.port = 0, .type = SUNXI_ETH_TYPE_DWMAC,
		.io_base = SUNXI_ETH0_PBASE, .syscfg_base = SUNXI_ETH0_SYSCFG, .irq_num = SUNXI_ETH0_IRQ,
		.gpio = GPIOJ(0), .gpio_num = 16, .mux = 5, .driv_level = GPIO_DRIVING_LEVEL3,
		.phy_rst = GPIOB(7), .phy_rst_delay = { 0, 10000, 15000 },
		.eth_clk_type = HAL_SUNXI_CCU, .eth_clk_id = CLK_GMAC0,
		.bus_clk_type = HAL_SUNXI_CCU, .bus_clk_id = CLK_MBUS_GMAC0_GATE,
		.phy_clk_type = HAL_SUNXI_CCU, .phy_clk_id = CLK_GMAC0_PHY,
		.nsi_clk_type = HAL_SUNXI_CCU, .nsi_clk_id = CLK_GMAC_NSI,
		.nsi_src_type = HAL_SUNXI_CCU, .nsi_src_id = CLK_PLL_PERI0_480M,
		.ahb_rst_type = HAL_SUNXI_RESET, .ahb_rst_id = RST_BUS_GMAC0,
		.axi_rst_type = HAL_SUNXI_RESET, .axi_rst_id = RST_BUS_GMAC0_AXI,
		.flags = SUNXI_DWMAC_NSI_CLK_GATE,
		.interface = PHY_INTERFACE_MODE_RGMII, .speed = SPEED_1000, .duplex = DUPLEX_FULL,
		.tx_delay = 10, .rx_delay = 1,
		.soc_phy_clk_en = true,
	},
	{
		.port = 1, .type = SUNXI_ETH_TYPE_DWMAC,
		.io_base = SUNXI_ETH1_PBASE, .syscfg_base = SUNXI_ETH1_SYSCFG, .irq_num = SUNXI_ETH1_IRQ,
		.gpio = GPIOJ(16), .gpio_num = 16, .mux = 5, .driv_level = GPIO_DRIVING_LEVEL3,
		.phy_rst = GPIOB(8), .phy_rst_delay = { 0, 10000, 15000 },
		.eth_clk_type = HAL_SUNXI_CCU, .eth_clk_id = CLK_GMAC1,
		.bus_clk_type = HAL_SUNXI_CCU, .bus_clk_id = CLK_MBUS_GMAC1_GATE,
		.phy_clk_type = HAL_SUNXI_CCU, .phy_clk_id = CLK_GMAC1_PHY,
		.nsi_clk_type = HAL_SUNXI_CCU, .nsi_clk_id = CLK_GMAC_NSI,
		.nsi_src_type = HAL_SUNXI_CCU, .nsi_src_id = CLK_PLL_PERI0_480M,
		.ahb_rst_type = HAL_SUNXI_RESET, .ahb_rst_id = RST_BUS_GMAC1,
		.axi_rst_type = HAL_SUNXI_RESET, .axi_rst_id = RST_BUS_GMAC1_AXI,
		.flags = SUNXI_DWMAC_NSI_CLK_GATE,
		.interface = PHY_INTERFACE_MODE_RGMII, .speed = SPEED_1000, .duplex = DUPLEX_FULL,
		.tx_delay = 10, .rx_delay = 3,
		.soc_phy_clk_en = true,
	},
};

#define SUNXI_ETH_MAX_NUM ARRAY_SIZE(sunxi_eth_plat)

#endif /* __GMAC_SUN55IW6_H__ */

/*
* Allwinner Ethernet driver.
*
* Copyright(c) 2022-2027 Allwinnertech Co., Ltd.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <hal_gpio.h>
#include <hal_clk.h>
#include <hal_reset.h>
#include <hal_phy.h>

#define SUNXI_DWMAC_NSI_CLK_GATE	BIT(0)
#define SUNXI_DWMAC_HSI_CLK_GATE	BIT(1)

enum sunxi_eth_type {
	SUNXI_ETH_TYPE_GMAC,
	SUNXI_ETH_TYPE_DWMAC,
};

struct sunxi_eth_platform {
	int port;
	enum sunxi_eth_type type;
	unsigned long io_base;
	unsigned long syscfg_base;
	u32 irq_num;
	int gpio_num;
	gpio_pin_t gpio;
	gpio_muxsel_t mux;
	gpio_driving_level_t driv_level;
	gpio_pin_t phy_rst;
	int phy_rst_delay[3];
	/* for GMAC/DWMAC */
	hal_clk_type_t eth_clk_type;
	hal_clk_id_t eth_clk_id;
	hal_clk_type_t phy_clk_type;
	hal_clk_id_t phy_clk_id;
	hal_clk_type_t ahb_rst_type;
	hal_reset_id_t ahb_rst_id;
	/* for DMWAC only */
	hal_clk_type_t bus_clk_type;
	hal_clk_id_t bus_clk_id;
	hal_clk_type_t nsi_clk_type;
	hal_clk_id_t nsi_clk_id;
	hal_clk_type_t nsi_src_type;
	hal_clk_id_t nsi_src_id;
	hal_clk_type_t axi_rst_type;
	hal_reset_id_t axi_rst_id;
	u32 flags;
	int interface;
	int speed;
	int duplex;
	int tx_delay;
	int rx_delay;
	bool soc_phy_clk_en;
};

#if defined(CONFIG_ARCH_SUN8IW20)
#include "platform/gmac_sun8iw20.h"
#include "arch/riscv/barrier.h"
#endif

#if defined(CONFIG_ARCH_SUN55IW6)
#include "platform/gmac_sun55iw6.h"
#endif

#if defined(CONFIG_ARCH_SUN251IW1)
#include "platform/gmac_sun251iw1.h"
#endif

#endif /* __PLATFORM_H__ */
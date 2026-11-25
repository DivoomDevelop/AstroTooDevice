/*
* Allwinner DWMAC driver.
*
* Copyright(c) 2024 Allwinnertech Co., Ltd.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#ifndef __HAL_DWMAC_H__
#define __HAL_DWMAC_H__

#include "platform.h"

#define setbits(reg, set) \
	writel((readl(reg) | (set)), reg);

#define clrbits(reg, clr) \
	writel((readl(reg) & ~(clr)), reg);

#define clrsetbits(reg, clr, set) \
	writel(((readl(reg) & ~(clr)) | set), reg);

#define SUNXI_DWC_MAC_REGS_BASE 0x000
struct sunxi_dwc_mac_regs {
	u32 configuration;				/* 0x000 */
	u32 unused_004[(0x070 - 0x004) / 4];	/* 0x004 */
	u32 q0_tx_flow_ctrl;			/* 0x070 */
	u32 unused_070[(0x090 - 0x074) / 4];	/* 0x074 */
	u32 rx_flow_ctrl;				/* 0x090 */
	u32 unused_094;				/* 0x094 */
	u32 txq_prty_map0;				/* 0x098 */
	u32 unused_09c;				/* 0x09c */
	u32 rxq_ctrl0;				/* 0x0a0 */
	u32 unused_0a4;				/* 0x0a4 */
	u32 rxq_ctrl2;				/* 0x0a8 */
	u32 unused_0ac[(0x0dc - 0x0ac) / 4];	/* 0x0ac */
	u32 us_tic_counter;			/* 0x0dc */
	u32 unused_0e0[(0x11c - 0x0e0) / 4];	/* 0x0e0 */
	u32 hw_feature0;				/* 0x11c */
	u32 hw_feature1;				/* 0x120 */
	u32 hw_feature2;				/* 0x124 */
	u32 unused_128[(0x200 - 0x128) / 4];	/* 0x128 */
	u32 mdio_address;				/* 0x200 */
	u32 mdio_data;				/* 0x204 */
	u32 unused_208[(0x300 - 0x208) / 4];	/* 0x208 */
	u32 address0_high;				/* 0x300 */
	u32 address0_low;				/* 0x304 */
};

#define SUNXI_DWC_MAC_CONFIGURATION_GPSLCE		BIT(23)
#define SUNXI_DWC_MAC_CONFIGURATION_CST			BIT(21)
#define SUNXI_DWC_MAC_CONFIGURATION_ACS			BIT(20)
#define SUNXI_DWC_MAC_CONFIGURATION_WD			BIT(19)
#define SUNXI_DWC_MAC_CONFIGURATION_JD			BIT(17)
#define SUNXI_DWC_MAC_CONFIGURATION_JE			BIT(16)
#define SUNXI_DWC_MAC_CONFIGURATION_PS			BIT(15)
#define SUNXI_DWC_MAC_CONFIGURATION_FES			BIT(14)
#define SUNXI_DWC_MAC_CONFIGURATION_DM			BIT(13)
#define SUNXI_DWC_MAC_CONFIGURATION_LM			BIT(12)
#define SUNXI_DWC_MAC_CONFIGURATION_TE			BIT(1)
#define SUNXI_DWC_MAC_CONFIGURATION_RE			BIT(0)

#define SUNXI_DWC_MAC_Q0_TX_FLOW_CTRL_PT_SHIFT		16
#define SUNXI_DWC_MAC_Q0_TX_FLOW_CTRL_PT_MASK		0xffff
#define SUNXI_DWC_MAC_Q0_TX_FLOW_CTRL_TFE			BIT(1)

#define SUNXI_DWC_MAC_RX_FLOW_CTRL_RFE			BIT(0)

#define SUNXI_DWC_MAC_TXQ_PRTY_MAP0_PSTQ0_SHIFT		0
#define SUNXI_DWC_MAC_TXQ_PRTY_MAP0_PSTQ0_MASK		0xff

#define SUNXI_DWC_MAC_RXQ_CTRL0_RXQ0EN_SHIFT		0
#define SUNXI_DWC_MAC_RXQ_CTRL0_RXQ0EN_MASK			3
#define SUNXI_DWC_MAC_RXQ_CTRL0_RXQ0EN_NOT_ENABLED	0
#define SUNXI_DWC_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_DCB	2
#define SUNXI_DWC_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_AV	1

#define SUNXI_DWC_MAC_RXQ_CTRL2_PSRQ0_SHIFT			0
#define SUNXI_DWC_MAC_RXQ_CTRL2_PSRQ0_MASK			0xff

#define SUNXI_DWC_MAC_HW_FEATURE0_MMCSEL_SHIFT		8
#define SUNXI_DWC_MAC_HW_FEATURE0_HDSEL_SHIFT		2
#define SUNXI_DWC_MAC_HW_FEATURE0_GMIISEL_SHIFT		1
#define SUNXI_DWC_MAC_HW_FEATURE0_MIISEL_SHIFT		0

#define SUNXI_DWC_MAC_HW_FEATURE1_TXFIFOSIZE_SHIFT	6
#define SUNXI_DWC_MAC_HW_FEATURE1_TXFIFOSIZE_MASK	0x1f
#define SUNXI_DWC_MAC_HW_FEATURE1_RXFIFOSIZE_SHIFT	0
#define SUNXI_DWC_MAC_HW_FEATURE1_RXFIFOSIZE_MASK	0x1f

#define SUNXI_DWC_MAC_HW_FEATURE3_ASP_SHIFT			28
#define SUNXI_DWC_MAC_HW_FEATURE3_ASP_MASK			0x3

#define SUNXI_DWC_MAC_MDIO_ADDRESS_PA_SHIFT			21
#define SUNXI_DWC_MAC_MDIO_ADDRESS_RDA_SHIFT		16
#define SUNXI_DWC_MAC_MDIO_ADDRESS_CR_SHIFT			8
#define SUNXI_DWC_MAC_MDIO_ADDRESS_CR_20_35			2
#define SUNXI_DWC_MAC_MDIO_ADDRESS_CR_150_250		4
#define SUNXI_DWC_MAC_MDIO_ADDRESS_CR_250_300		5
#define SUNXI_DWC_MAC_MDIO_ADDRESS_SKAP				BIT(4)
#define SUNXI_DWC_MAC_MDIO_ADDRESS_GOC_SHIFT		2
#define SUNXI_DWC_MAC_MDIO_ADDRESS_GOC_READ			3
#define SUNXI_DWC_MAC_MDIO_ADDRESS_GOC_WRITE		1
#define SUNXI_DWC_MAC_MDIO_ADDRESS_C45E				BIT(1)
#define SUNXI_DWC_MAC_MDIO_ADDRESS_GB				BIT(0)
#define SUNXI_DWC_MAC_MDIO_DATA_GD_MASK				0xffff

#define SUNXI_DWC_MTL_REGS_BASE 0xd00
struct sunxi_dwc_mtl_regs {
	u32 txq0_operation_mode;			/* 0xd00 */
	u32 unused_d04;				/* 0xd04 */
	u32 txq0_debug;				/* 0xd08 */
	u32 unused_d0c[(0xd18 - 0xd0c) / 4];	/* 0xd0c */
	u32 txq0_quantum_weight;			/* 0xd18 */
	u32 unused_d1c[(0xd30 - 0xd1c) / 4];	/* 0xd1c */
	u32 rxq0_operation_mode;			/* 0xd30 */
	u32 unused_d34;				/* 0xd34 */
	u32 rxq0_debug;				/* 0xd38 */
};

#define SUNXI_DWC_MTL_TXQ0_OPERATION_MODE_TQS_SHIFT		16
#define SUNXI_DWC_MTL_TXQ0_OPERATION_MODE_TQS_MASK		0x1ff
#define SUNXI_DWC_MTL_TXQ0_OPERATION_MODE_TXQEN_SHIFT	2
#define SUNXI_DWC_MTL_TXQ0_OPERATION_MODE_TXQEN_MASK	3
#define SUNXI_DWC_MTL_TXQ0_OPERATION_MODE_TXQEN_ENABLED	2
#define SUNXI_DWC_MTL_TXQ0_OPERATION_MODE_TSF			BIT(1)
#define SUNXI_DWC_MTL_TXQ0_OPERATION_MODE_FTQ			BIT(0)

#define SUNXI_DWC_MTL_TXQ0_DEBUG_TXQSTS				BIT(4)
#define SUNXI_DWC_MTL_TXQ0_DEBUG_TRCSTS_SHIFT		1
#define SUNXI_DWC_MTL_TXQ0_DEBUG_TRCSTS_MASK		3

#define SUNXI_DWC_MTL_RXQ0_OPERATION_MODE_RQS_SHIFT		20
#define SUNXI_DWC_MTL_RXQ0_OPERATION_MODE_RQS_MASK		0x3ff
#define SUNXI_DWC_MTL_RXQ0_OPERATION_MODE_RFD_SHIFT		14
#define SUNXI_DWC_MTL_RXQ0_OPERATION_MODE_RFD_MASK		0x3f
#define SUNXI_DWC_MTL_RXQ0_OPERATION_MODE_RFA_SHIFT		8
#define SUNXI_DWC_MTL_RXQ0_OPERATION_MODE_RFA_MASK		0x3f
#define SUNXI_DWC_MTL_RXQ0_OPERATION_MODE_EHFC			BIT(7)
#define SUNXI_DWC_MTL_RXQ0_OPERATION_MODE_RSF			BIT(5)

#define SUNXI_DWC_MTL_RXQ0_DEBUG_PRXQ_SHIFT			16
#define SUNXI_DWC_MTL_RXQ0_DEBUG_PRXQ_MASK			0x7fff
#define SUNXI_DWC_MTL_RXQ0_DEBUG_RXQSTS_SHIFT		4
#define SUNXI_DWC_MTL_RXQ0_DEBUG_RXQSTS_MASK		3

#define SUNXI_DWC_DMA_REGS_BASE 0x1000
struct sunxi_dwc_dma_regs {
	u32 mode;					/* 0x1000 */
	u32 sysbus_mode;				/* 0x1004 */
	u32 unused_1008[(0x1100 - 0x1008) / 4];	/* 0x1008 */
	u32 ch0_control;				/* 0x1100 */
	u32 ch0_tx_control;			/* 0x1104 */
	u32 ch0_rx_control;			/* 0x1108 */
	u32 unused_110c;				/* 0x110c */
	u32 ch0_txdesc_list_haddress;		/* 0x1110 */
	u32 ch0_txdesc_list_address;		/* 0x1114 */
	u32 ch0_rxdesc_list_haddress;		/* 0x1118 */
	u32 ch0_rxdesc_list_address;		/* 0x111c */
	u32 ch0_txdesc_tail_pointer;		/* 0x1120 */
	u32 unused_1124;				/* 0x1124 */
	u32 ch0_rxdesc_tail_pointer;		/* 0x1128 */
	u32 ch0_txdesc_ring_length;		/* 0x112c */
	u32 ch0_rxdesc_ring_length;		/* 0x1130 */
};

#define SUNXI_DWC_DMA_MODE_SWR				BIT(0)

#define SUNXI_DWC_DMA_SYSBUS_MODE_RD_OSR_LMT_SHIFT		16
#define SUNXI_DWC_DMA_SYSBUS_MODE_RD_OSR_LMT_MASK		0xf
#define SUNXI_DWC_DMA_SYSBUS_MODE_EAME					BIT(11)
#define SUNXI_DWC_DMA_SYSBUS_MODE_BLEN16				BIT(3)
#define SUNXI_DWC_DMA_SYSBUS_MODE_BLEN8					BIT(2)
#define SUNXI_DWC_DMA_SYSBUS_MODE_BLEN4					BIT(1)

#define SUNXI_DWC_DMA_CH0_CONTROL_DSL_SHIFT			18
#define SUNXI_DWC_DMA_CH0_CONTROL_DSL_MASK			0x7
#define SUNXI_DWC_DMA_CH0_CONTROL_PBLX8				BIT(16)

#define SUNXI_DWC_DMA_CH0_TX_CONTROL_TXPBL_SHIFT	16
#define SUNXI_DWC_DMA_CH0_TX_CONTROL_TXPBL_MASK		0x3f
#define SUNXI_DWC_DMA_CH0_TX_CONTROL_OSP			BIT(4)
#define SUNXI_DWC_DMA_CH0_TX_CONTROL_ST				BIT(0)

#define SUNXI_DWC_DMA_CH0_RX_CONTROL_RXPBL_SHIFT	16
#define SUNXI_DWC_DMA_CH0_RX_CONTROL_RXPBL_MASK		0x3f
#define SUNXI_DWC_DMA_CH0_RX_CONTROL_RBSZ_SHIFT		1
#define SUNXI_DWC_DMA_CH0_RX_CONTROL_RBSZ_MASK		0x3fff
#define SUNXI_DWC_DMA_CH0_RX_CONTROL_SR				BIT(0)

/* Descriptors */
#define SUNXI_DWC_DESCRIPTORS_TX	64
#define SUNXI_DWC_DESCRIPTORS_RX	64
#define SUNXI_DWC_DESCRIPTORS_NUM	(SUNXI_DWC_DESCRIPTORS_TX + SUNXI_DWC_DESCRIPTORS_RX)
#define SUNXI_DWC_BUFFER_ALIGN		CACHELINE_LEN
#define SUNXI_DWC_MAX_PACKET_SIZE	ALIGN_UP(1568, CACHELINE_LEN)
#define SUNXI_DWC_TX_BUFFER_SIZE	(SUNXI_DWC_DESCRIPTORS_TX * SUNXI_DWC_MAX_PACKET_SIZE)
#define SUNXI_DWC_RX_BUFFER_SIZE	(SUNXI_DWC_DESCRIPTORS_RX * SUNXI_DWC_MAX_PACKET_SIZE)

struct sunxi_dwc_desc {
	u32 des0;
	u32 des1;
	u32 des2;
	u32 des3;
};

#define SUNXI_DWC_DESC3_OWN		BIT(31)
#define SUNXI_DWC_DESC3_FD		BIT(29)
#define SUNXI_DWC_DESC3_LD		BIT(28)
#define SUNXI_DWC_DESC3_BUF1V	BIT(24)

#define SUNXI_DWC_AXI_WIDTH_32	4
#define SUNXI_DWC_AXI_WIDTH_64	8
#define SUNXI_DWC_AXI_WIDTH_128	16

/* DWMAC Configuration Register */
#define SUNXI_DWMAC_CFG_REG	(0x00)
	#define SUNXI_DWMAC_CFG_ETXDC_H		GENMASK(17, 16)
	#define SUNXI_DWMAC_CFG_PHY_SEL		BIT(15)
	#define SUNXI_DWMAC_CFG_ENDIAN_MODE	BIT(14)
	#define SUNXI_DWMAC_CFG_RMII_EN		BIT(13)
	#define SUNXI_DWMAC_CFG_ETXDC_L		GENMASK(12, 10)
	#define SUNXI_DWMAC_CFG_ERXDC		GENMASK(9, 5)
	#define SUNXI_DWMAC_CFG_ERXIE		BIT(4)
	#define SUNXI_DWMAC_CFG_ETXIE		BIT(3)
	#define SUNXI_DWMAC_CFG_EPIT		BIT(2)
	#define SUNXI_DWMAC_CFG_ETCS		GENMASK(1, 0)

#define SUNXI_DWMAC_ETCS_MII		0x0
#define SUNXI_DWMAC_ETCS_EXT_GMII	0x1
#define SUNXI_DWMAC_ETCS_INT_GMII	0x2

struct sunxi_dwmac {
	struct ethernetif *ethernetif;

	unsigned long io_base;
	unsigned long syscfg_base;

	struct sunxi_dwc_mac_regs *mac_regs;
	struct sunxi_dwc_mtl_regs *mtl_regs;
	struct sunxi_dwc_dma_regs *dma_regs;

	hal_clk_t eth_clk;
	hal_clk_t bus_clk;
	hal_clk_t phy_clk;
	hal_clk_t nsi_clk;
	struct reset_control *ahb_rst;
	struct reset_control *axi_rst;

	void *tx_descs;
	void *rx_descs;
	int tx_desc_idx, rx_desc_idx;
	unsigned int desc_size;
	unsigned int desc_per_cacheline;
	void *tx_dma_buf;
	void *rx_dma_buf;
	bool started;

	const struct sunxi_eth_platform *plat;
	int speed;
	int duplex;
};

#endif /* __HAL_DWMAC_H__ */

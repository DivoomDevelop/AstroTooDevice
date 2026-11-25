/*
 * drivers/spi/spif-sunxi.h
 *
 * Copyright (C) 2021 - 2024 Reuuimlla Limited
 * Lu Jian Liang <lujianliang@reuuimllatech.com>
 *
 * SUNXI SPIF Register Definition
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * 2021.12.1 <lujianliang@allwinnertech.com>
 *    Adapt to support sun8iw21~future of Allwinner.
 */

#ifndef _SUNXI_SPIF_H_
#define _SUNXI_SPIF_H_

#include <hal_clk.h>
#include <hal_sem.h>
#include <hal_mutex.h>
#include <spif/platform_spif.h>

/* For debug */
//#define SPIF_DEBUG
#ifdef SPIF_DEBUG
#define SPIF_INFO(fmt, arg...) printf("%s %s()%d - " fmt, "[I]", __func__, __LINE__, ##arg)
#define SPIF_ERR(fmt, arg...)  printf("%s %s()%d - " fmt, "[E]", __func__, __LINE__, ##arg)
#define SPIF_DBG(fmt, arg...) printf("%s %s()%d - " fmt, "[W]", __func__, __LINE__, ##arg)
#else
#define SPIF_INFO(fmt, arg...) printf("%s %s()%d - " fmt, "[I]", __func__, __LINE__, ##arg)
#define SPIF_ERR(fmt, arg...) printf("%s %s()%d - " fmt, "[E]", __func__, __LINE__, ##arg)
#define SPIF_DBG(fmt, arg...)                                                                                                             \
	do {                                                                                                                               \
	} while (0)
#endif

#define XFER_TIMEOUT	(5000)
#define SUNXI_SPIF_OK   0
#define SUNXI_SPIF_FAIL -1

/* SPIF Registers offsets from peripheral base address */
#define SPIF_VER_REG		(0x00)	/* version number register */
#define SPIF_GC_REG		(0x04)	/* global control register */
#define SPIF_GCA_REG		(0x08)	/* global additional control register */
#define SPIF_TC_REG		(0x0C)	/* timing control register */
#define SPIF_TDS_REG		(0x10)	/* timing delay state register */
#define SPIF_INT_EN_REG		(0x14)	/* interrupt enable register */
#define SPIF_INT_STA_REG	(0x18)	/* interrupt status register */
#define SPIF_CSD_REG		(0x1C)	/* chipselect delay register */
#define SPIF_PHC_REG		(0x20)	/* trans phase configure register */
#define SPIF_TCF_REG		(0x24)	/* trans configure1 register */
#define SPIF_TCS_REG		(0x28)	/* trans configure2 register */
#define SPIF_TNM_REG		(0x2C)	/* trans number register */
#define SPIF_PS_REG		(0x30)	/* prefetch state register */
#define SPIF_PSA_REG		(0x34)	/* prefetch start address register */
#define SPIF_PEA_REG		(0x38)	/* prefetch end address register */
#define SPIF_PMA_REG		(0x3C)	/* prefetch map address register */
#define SPIF_DMA_CTL_REG	(0x40)	/* DMA control register */
#define SPIF_DSC_REG		(0x44)	/* DMA descriptor start address register */
#define SPIF_DFT_REG		(0x48)	/* DQS FIFO trigger level register */
#define SPIF_CFT_REG		(0x4C)	/* CDC FIFO trigger level register */
#define SPIF_CFS_REG		(0x50)	/* CDC FIFO status register */
#define SPIF_BAT_REG		(0x54)	/* Bit-Aligned tansfer configure register */
#define SPIF_BAC_REG		(0x58)	/* Bit-Aligned clock configuration register */
#define SPIF_TB_REG		(0x5C)	/* TX Bit register */
#define SPIF_RB_REG		(0x60)	/* RX Bit register */
#define SPIF_ST_REG		(0x60)	/* SPI status register */
#define SPIF_RXDATA_REG		(0x200)	/* prefetch RX data register */

/* SPIF global control register bit fields & masks,default value:0x0000_0100 */
#define SPIF_GC_CFG_MODE	(0x1 << 0)
#define SPIF_GC_DMA_MODE	(1)
#define SPIF_GC_CPU_MODE	(0)
#define SPIF_GC_ADDR_MAP_MODE	(0x1 << 1)
#define SPIF_GC_NMODE_EN	(0x1 << 2)
#define SPIF_GC_PMODE_EN	(0x1 << 3)
#define SPIF_GC_CPHA		(0x1 << 4)
#define SPIF_GC_CPOL		(0x1 << 5)
#define SPIF_GC_SS_MASK		(0x3 << 6) /* SPIF chip select:00-SPI_SS0;01-SPI_SS1;10-SPI_SS2;11-SPI_SS3*/
#define SPIF_GC_CS_POL		(0x1 << 8)
#define SPIF_GC_DUMMY_BIT_POL	(0x1 << 9)
#define SPIF_GC_DQS_RX_EN	(0x1 << 10)
#define SPIF_GC_HOLD_POL	(0x1 << 12)
#define SPIF_GC_HOLD_EN		(0x1 << 13)
#define SPIF_GC_WP_POL		(0x1 << 14)
#define SPIF_GC_WP_EN		(0x1 << 15)
#define SPIF_GC_DTR_EN		(0x1 << 16)
#define SPIF_GC_RX_CFG_FBS	(0x1 << 17)
#define SPIF_GC_TX_CFG_FBS	(0x1 << 18)
#define SPIF_GC_SS_BIT_POS	(6)
#define MSB_FIRST		(0)
#define LSB_FIRST		(1)

/*0x14*/
#define SPIF_RF_RDY_INT_EN		(0x1 << 0)	/* read fifo is ready */
#define SPIF_RF_ENPTY_INT_EN		(0x1 << 1)
#define SPIF_RF_FULL_INT_EN		(0x1 << 2)
#define SPIF_WF_RDY_INT_EN		(0x1 << 4)	/* write fifo is ready */
#define SPIF_WF_ENPTY_INT_EN		(0x1 << 5)
#define SPIF_WF_FULL_INT_EN		(0x1 << 6)
#define SPIF_RF_OVF_INT_EN		(0x1 << 8)
#define SPIF_RF_UDF_INT_EN		(0x1 << 9)
#define SPIF_WF_OVF_INT_EN		(0x1 << 10)
#define SPIF_WF_UDF_INT_EN		(0x1 << 11)
#define SPIF_DMA_TRANS_DONE_EN		(0x1 << 24)
#define SPIF_PREFETCH_READ_EN		(0x1 << 25)
#define SPIF_INT_STA_READY_EN		(SPIF_RF_RDY_INT_EN | SPIF_WF_RDY_INT_EN)
#define SPIF_INT_STA_TC_EN		(SPIF_DMA_TRANS_DONE_EN | SPIF_PREFETCH_READ_EN)
#define	SPIF_INT_STA_ERR_EN		(SPIF_RF_OVF_INT_EN | SPIF_RF_UDF_INT_EN | SPIF_WF_OVF_INT_EN)

/* 0x18 */
#define SPIF_RF_RDY                     BIT(0)
#define SPIF_RF_EMPTY                   BIT(1)
#define SPIF_RF_FULL                    BIT(2)
#define SPIF_WF_RDY                     BIT(4)
#define SPIF_WF_EMPTY                   BIT(5)
#define SPIF_WF_FULL                    BIT(6)
#define SPIF_RF_OVF                     BIT(8)
#define SPIF_RF_UDF                     BIT(9)
#define SPIF_WF_OVF                     BIT(10)
#define SPIF_WF_UDF                     BIT(11)
#define SPIF_DMA_TRANS_DONE             BIT(24)
#define SPIF_PREFETCH_READ_BACK         BIT(25)
#define SPIF_INT_STA_READY              (SPIF_RF_READY | SPIF_WF_READY)
#define SPIF_INT_STA_TC                 (SPIF_DMA_TRANS_DONE | SPIF_PREFETCH_READ_BACK)
#define SPIF_INT_STA_ERR                (SPIF_RF_OVF | SPIF_RF_UDF | SPIF_WF_OVF)

//SPIF_GCR:SPIF_MODE
#define SPIF_MODE0		(0U << 4)
#define SPIF_MODE1		(SPIF_GC_CPHA)
#define SPIF_MODE2		(SPIF_GC_CPOL)
#define SPIF_MODE3		(SPIF_GC_CPOL | SPIF_GC_CPHA)
#define SPIF_MASK		(3U << 4)

#define SPIF_SCKT_DELAY_MODE	(1U << 21)
#define SPIF_DIGITAL_ANALOG_EN	(1U << 20)
#define SPIF_DIGITAL_DELAY	(16)
#define SPIF_DIGITAL_DELAY_MASK	(7U << 16)
#define SPIF_ANALOG_DL_SW_RX_EN	(1U << 6)
#define SPIF_ANALOG_DELAY	(0)
#define SPIF_ANALOG_DELAY_MASK	(0x3F << 0)

#define SPIF_RF_SRST		(0x1 << 0)
#define SPIF_WF_SRST		(0x1 << 1)
#define SPIF_DQS_RF_SRST	(0x1 << 2)
#define SPIF_SOFT_SRST		(0x1 << 3)
#define SPIF_DMA_END		(0x1 << 4)
#define SPIF_FIFO_SRST		(SPIF_RF_SRST | SPIF_WF_SRST)
#define SPIF_DFT_DQS		(0x6400)
#define SPIF_CFT_CDC		(0x64106410)
#define SPIF_CSDA		(5)
#define SPIF_CSEOT		(6)
#define	SPIF_CSSOT		(6)
#define SPIF_CSD_DEF		((SPIF_CSDA << 16) | (SPIF_CSEOT << 8) | SPIF_CSSOT)

/* SPIF Timing Configure Register Bit Fields & Masks,default value:0x0000_0000 */
#define SPIF_SAMP_DL_VAL_TX_POS (8)
#define SPIF_SAMP_DL_VAL_RX_POS (0)
#define SPIF_SCKR_DL_MODE_SEL	(0x1 << 20)
#define SPIF_CLK_SCKOUT_SRC_SEL	(0x1 << 26)

/* SPIF Trans Phase Configure Register Bit Fields & Masks,default value:0x0000_0000 */
#define SPIF_RX_TRANS_EN        (0x1 << 8)
#define SPIF_TX_TRANS_EN        (0x1 << 12)
#define SPIF_DUMMY_TRANS_EN     (0x1 << 16)
#define SPIF_MODE_TRANS_EN      (0x1 << 20)
#define SPIF_ADDR_TRANS_EN      (0x1 << 24)
#define SPIF_CMD_TRANS_EN       (0x1 << 28)

/* SPIF Trans Configure2 Register Bit Fields & Masks,default value:0x0000_0000 */
#define SPIF_DATA_TRANS_POS     (0)
#ifdef CONFIG_MACH_SUN8IW21
#define SPIF_MODE_TRANS_POS     (4)
#define SPIF_ADDR_TRANS_POS     (8)
#define SPIF_CMD_TRANS_POS      (12)
#else
#define SPIF_MODE_TRANS_POS     (2)
#define SPIF_ADDR_TRANS_POS     (4)
#define SPIF_CMD_TRANS_POS      (6)
#endif
#define SPIF_MODE_OPCODE_POS    (16)
#define SPIF_CMD_OPCODE_POS     (24)

/* SPIF Trans Number Register Bit Fields & Masks,default value:0x0000_0000 */
#define SPIF_ADDR_SIZE_24BIT	(0x0 << 24)
#define SPIF_ADDR_SIZE_32BIT	(0x1 << 24)
#define SPIF_ADDR_SIZE_MASK	(0x1 << 24)
/* V1.2 update CMD/ADDR_LENGTH */
#define SPIF_ADDR_SIZE_8BIT_V2  (0x0 << 24)
#define SPIF_ADDR_SIZE_16BIT_V2 (0x1 << 24)
#define SPIF_ADDR_SIZE_24BIT_V2 (0x2 << 24)
#define SPIF_ADDR_SIZE_32BIT_V2 (0x3 << 24)
#define SPIF_ADDR_SIZE_MASK_V2  (0x3 << 24)
#define SPIF_CMD_LEN_1BTYE	(0x0 << 26)
#define SPIF_CMD_LEN_2BTYE	(0x1 << 26)
#define SPIF_CMD_LEN_MASK	(0x1 << 26)

#define SPIF_DUMMY_NUM_POS      (16)
#define SPIF_DATA_NUM_POS	(0)

/* SPIF DMA Control Register Bit Fields & Masks,default value:0x0000_0000 */
#define CFG_DMA_START		(1 << 0)
#define DMA_DESCRIPTOR_LEN	(32 << 4)

/* DMA descriptor0 Bit Fields & Masks */
#define HBURST_SINGLE_TYPE	(0x0 << 4)
#define HBURST_INCR4_TYPE	(0x3 << 4)
#define HBURST_INCR8_TYPE	(0x5 << 4)
#define HBURST_INCR16_TYPE	(0x7 << 4)
#define HBURST_TYPE		(0x7 << 4)
#define DMA_RW_PROCESS		(0x1 << 1) /* 0:Read  1:Write */
#define DMA_FINISH_FLASG	(0x1 << 0) /* The Last One Descriptor */

/* DMA descriptor1 Bit Fields & Masks */
#ifdef CONFIG_MACH_SUN8IW21
#define DMA_BLK_LEN_8B		(0x0 << 16)
#define DMA_BLK_LEN_16B		(0x1 << 16)
#define DMA_BLK_LEN_32B		(0x2 << 16)
#define DMA_BLK_LEN_64B		(0x3 << 16)
#define DMA_BLK_LEN		(0xffff << 16)
#define DMA_DATA_LEN		(0xffff)
#define SPIF_MAX_TRANS_NUM	(4096)
#else
#define DMA_BLK_LEN_8B		(0x0 << 24)
#define DMA_BLK_LEN_16B		(0x1 << 24)
#define DMA_BLK_LEN_32B		(0x2 << 24)
#define DMA_BLK_LEN_64B		(0x3 << 24)
#define DMA_BLK_LEN		(0xff << 24)
#define DMA_DATA_LEN		(0x1ffff)
#define SPIF_MAX_TRANS_NUM	(65536)
#endif

#define DMA_TRANS_NUM_16BIT	(1 << 31)
#define SPIF_MIN_TRANS_NUM	(8)
#define DMA_TRANS_NUM		(0xffff)
#define DMA_DATA_LEN_POS	(0)

/* DMA descriptor7 Bit Fields & Masks */

#define SPIF_DES_NORMAL_EN	(0x1 << 28)

#define SPIF_OCTAL_MODE		(8)
#define SPIF_QUAD_MODE		(4)
#define SPIF_DUEL_MODE		(2)
#define SPIF_SINGLE_MODE	(1)

#define SUNXI_SPIF_DEFAULT_FREQUENCY	(25000000)	/* Low-frequency reading of ID */
#define SUNXI_SPIF_MAX_FREQUENCY	(150000000)	/* SPI Controller support max freq 150Mhz */
#define SUNXI_SPIF_MIN_FREQUENCY	(187500)	/* SPI Controller support min freq 187.5Khz(24M/8/16) */

#define CONFIG_SYS_CACHELINE_SIZE (64)
#define ROUND_UP(a, b)		  (((a) + (b)-1) & ~((b)-1))
#define ROUND_DOWN(a, b)	  (((a) - (b) + 1) & ~((b)-1))
#define CHECK_IS_NOALIGN(x)	  ((x) % CONFIG_SYS_CACHELINE_SIZE)

#define min_t(type, a, b)			\
	({                                      \
		type __a = (a);			\
		type __b = (b);                 \
		__a < __b ? __a : __b;		\
	})

#define max_t(type, a, b)			\
	({                                      \
		type __a = (a);			\
		type __b = (b);                 \
		__a > __b ? __a : __b;          \
	})

#define SAMP_MODE_DL_DEFAULT	0xaaaaffff

typedef enum
{
	HAL_SPIF_MASTER_ERROR = -6,			/* SPI master function error occurred */
	HAL_SPIF_MASTER_ERROR_NOMEM = -5,	/* SPI master request mem failed */
	HAL_SPIF_MASTER_ERROR_TIMEOUT = -4,	/* SPI master xfer timeout */
	HAL_SPIF_MASTER_ERROR_BUSY = -3,		/* SPI master is busy */
	HAL_SPIF_MASTER_ERROR_PORT = -2,		/* SPI master invalid port */
	HAL_SPIF_MASTER_INVALID_PARAMETER = -1,	/* SPI master invalid input parameter */
	HAL_SPIF_MASTER_OK = 0,				/* SPI master operation completed successfully */
} hal_spif_master_status_t;

struct sunxi_spif {
	void			*base_addr;
	hal_clk_t		pclk; /* PLL clock */
	hal_clk_t		bus_clk; /* BUS clock */
	hal_clk_t		mclk; /* spi module clock */
	struct reset_control	*reset;
	/* SPIF don't support customize gpio as cs */
	u32			bus_num;	/* the total num of spi chip select */
	u32			chip_select;
	u32			bus_freq;

	uint32_t		speed_hz;
	uint32_t		mode;
	unsigned int		sample_delay;
	unsigned int		sample_mode;
	unsigned int		rx_dtr_en;
	unsigned int		tx_dtr_en;
	unsigned int		xip_en;
	hal_sem_t		done;
	hal_mutex_t		mutex;
	int8_t result : 2;
#define SPIF_XFER_READY 0
#define SPIF_XFER_OK 1
#define SPIF_XFER_FAILED -1

	struct sunxi_spif_params_t *para;
};

struct xip_param {
	unsigned int xip_addr;
	unsigned int xip_len;
};

struct spif_descriptor_op {
	u32 hburst_rw_flag;
	u32 block_data_len;
	u32 data_addr;
	u32 next_des_addr;
	u32 trans_phase;//0x20
	u32 flash_addr;//0x24
	u32 cmd_mode_buswidth;//0x28
	u32 addr_dummy_data_count;//0x2c
};
struct nor_flash;

struct sunxi_spif *get_sspif(void);
int spif_xfer(struct spif_descriptor_op *spif_op, unsigned int data_len);

extern void spif_init_dtr(u32 flags);
extern uint32_t sunxi_spif_get_version(void);
extern hal_spif_master_status_t sunxi_spif_probe(struct sunxi_spif *sspi);
extern hal_spif_master_status_t hal_spif_deinit(void);
extern int spif_prefetch_enable(struct spif_descriptor_op *spif_op, unsigned int data_len);
extern void spif_perfetch_disable(void);
extern int spif_set_right_delay_para(struct nor_flash *nor, struct sunxi_spif *sspi);
#ifdef CONFIG_DRIVERS_SPINOR
extern int nor_wait_ready(int ms, int times);
#endif

#endif

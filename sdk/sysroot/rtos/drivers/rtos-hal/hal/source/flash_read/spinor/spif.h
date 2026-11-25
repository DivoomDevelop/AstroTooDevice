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

#ifndef __SPIF_H__
#define __SPIF_H__

#include <stdio.h>

/* porting defination */

//#define SPIF_DEBUG
//#define SPIF_LOG
#ifdef SPIF_DEBUG
#define SPIF_EXIT()	      printf("%s()%d - %s\n", __func__, __LINE__, "Exit")
#define SPIF_ENTER()	      printf("%s()%d - %s\n", __func__, __LINE__, "Enter ...")
#define SPIF_DBG(fmt, arg...) printf("%s %s()%d - " fmt, "[D]", __func__, __LINE__, ##arg)
#else
#define SPIF_EXIT()
#define SPIF_ENTER()
#define SPIF_DBG(fmt, arg...)                                                                                                              \
	do {                                                                                                                               \
	} while (0)
#endif

#ifdef SPIF_LOG
#define SPIF_INFO(fmt, arg...) printf("%s %s()%d - " fmt, "[I]", __func__, __LINE__, ##arg)
#define SPIF_ERR(fmt, arg...)  printf("%s %s()%d - " fmt, "[E]", __func__, __LINE__, ##arg)
#define SPIF_WARN(fmt, arg...) printf("%s %s()%d - " fmt, "[W]", __func__, __LINE__, ##arg)
#else
#define SPIF_INFO(fmt, arg...)                                                                                                             \
	do {                                                                                                                               \
	} while (0)
#define SPIF_ERR(fmt, arg...) printf("%s %s()%d - " fmt, "[E]", __func__, __LINE__, ##arg)
#define SPIF_WARN(fmt, arg...)                                                                                                             \
	do {                                                                                                                               \
	} while (0)
#endif

#define SUNXI_SPIF_OK	0
#define SUNXI_SPIF_FAIL -1

#if defined(CONFIG_ARCH_SUN300IW1)
#define SUNXI_SPIF_BASE (0x44F00000)
#elif defined(CONFIG_ARCH_SUN252IW1)
#define SUNXI_SPIF_BASE (0x04F00000)
#endif

/*
#define SPI_MODULE_NUM		(4)
#define SPI_FIFO_DEPTH		(128)
#define MAX_FIFU		64
#define BULK_DATA_BOUNDARY	64
#define SPI_MAX_FREQUENCY	100000000
*/
/* SPIF Global (Additional) Control Register Bit Fields & Masks,default value:0x0000_0080 */

/* SPIF Registers offsets from peripheral base address */
#define SPIF_VER_REG	 (0x00) /* version number register */
#define SPIF_GC_REG	 (0x04) /* global control register */
#define SPIF_GCA_REG	 (0x08) /* global additional control register */
#define SPIF_TC_REG	 (0x0C) /* timing control register */
#define SPIF_TDS_REG	 (0x10) /* timing delay state register */
#define SPIF_INT_EN_REG	 (0x14) /* interrupt enable register */
#define SPIF_INT_STA_REG (0x18) /* interrupt status register */
#define SPIF_CSD_REG	 (0x1C) /* chipselect delay register */
#define SPIF_PHC_REG	 (0x20) /* trans phase configure register */
#define SPIF_TCF_REG	 (0x24) /* trans configure1 register */
#define SPIF_TCS_REG	 (0x28) /* trans configure2 register */
#define SPIF_TNM_REG	 (0x2C) /* trans number register */
#define SPIF_PS_REG	 (0x30) /* prefetch state register */
#define SPIF_PSA_REG	 (0x34) /* prefetch start address register */
#define SPIF_PEA_REG	 (0x38) /* prefetch end address register */
#define SPIF_PMA_REG	 (0x3C) /* prefetch map address register */
#define SPIF_DMA_CTL_REG (0x40) /* DMA control register */
#define SPIF_DSC_REG	 (0x44) /* DMA descriptor start address register */
#define SPIF_DFT_REG	 (0x48) /* DQS FIFO trigger level register */
#define SPIF_CFT_REG	 (0x4C) /* CDC FIFO trigger level register */
#define SPIF_CFS_REG	 (0x50) /* CDC FIFO status register */
#define SPIF_BAT_REG	 (0x54) /* Bit-Aligned tansfer configure register */
#define SPIF_BAC_REG	 (0x58) /* Bit-Aligned clock configuration register */
#define SPIF_TB_REG	 (0x5C) /* TX Bit register */
#define SPIF_RB_REG	 (0x60) /* RX Bit register */
#define SPIF_RXDATA_REG	 (0x200) /* prefetch RX data register */

/* SPIF global control register bit fields & masks,default value:0x0000_0100 */
#define SPIF_GC_CFG_MODE      (0x1 << 0)
#define SPIF_GC_DMA_MODE      (1)
#define SPIF_GC_CPU_MODE      (0)
#define SPIF_GC_ADDR_MAP_MODE (0x1 << 1)
#define SPIF_GC_NMODE_EN      (0x1 << 2)
#define SPIF_GC_PMODE_EN      (0x1 << 3)
#define SPIF_GC_CPHA	      (0x1 << 4)
#define SPIF_GC_CPOL	      (0x1 << 5)
#define SPIF_GC_SS_MASK	      (0x3 << 6) /* SPIF chip select:00-SPI_SS0;01-SPI_SS1;10-SPI_SS2;11-SPI_SS3*/
#define SPIF_GC_CS_POL	      (0x1 << 8)
#define SPIF_GC_DUMMY_BIT_POL (0x1 << 9)
#define SPIF_GC_DQS_RX_EN     (0x1 << 10)
#define SPIF_GC_HOLD_POL      (0x1 << 12)
#define SPIF_GC_HOLD_EN	      (0x1 << 13)
#define SPIF_GC_WP_POL	      (0x1 << 14)
#define SPIF_GC_WP_EN	      (0x1 << 15)
#define SPIF_GC_DTR_EN	      (0x1 << 16)
#define SPIF_GC_RX_CFG_FBS    (0x1 << 17)
#define SPIF_GC_TX_CFG_FBS    (0x1 << 18)
#define SPIF_GC_SS_BIT_POS    (6)
#define MSB_FIRST	      (0)
#define LSB_FIRST	      (1)

//SPIF_GCR:SPIF_MODE
#define SPIF_MODE0 (0U << 4)
#define SPIF_MODE1 (SPIF_GC_CPHA)
#define SPIF_MODE2 (SPIF_GC_CPOL)
#define SPIF_MODE3 (SPIF_GC_CPOL | SPIF_GC_CPHA)
#define SPIF_MASK  (3U << 4)

#define SPIF_SCKT_DELAY_MODE	(1U << 21)
#define SPIF_DIGITAL_ANALOG_EN	(1U << 20)
#define SPIF_DIGITAL_DELAY	(16)
#define SPIF_DIGITAL_DELAY_MASK (7U << 16)
#define SPIF_ANALOG_DL_SW_RX_EN (1U << 6)
#define SPIF_ANALOG_DELAY	(0)
#define SPIF_ANALOG_DELAY_MASK	(0x3F << 0)

#define SPIF_GCA_SRST  (0xf << 0)
#define SPIF_FIFO_SRST (0x3 << 0)
#define SPIF_DFT_DQS   (0x6400)
#define SPIF_CFT_CDC   (0x64106410)
#define SPIF_CSDA      (5)
#define SPIF_CSEOT     (6)
#define SPIF_CSSOT     (6)
#define SPIF_CSD_DEF   ((SPIF_CSDA << 16) | (SPIF_CSEOT << 8) | SPIF_CSSOT)

/* SPIF Timing Configure Register Bit Fields & Masks,default value:0x0000_0000 */
#define SPIF_SAMP_DL_VAL_TX_POS (8)
#define SPIF_SAMP_DL_VAL_RX_POS (0)
#define SPIF_SCKR_DL_MODE_SEL	(0x1 << 20)
#define SPIF_CLK_SCKOUT_SRC_SEL (0x1 << 26)

/* SPIF Interrupt status Register Bit Fields & Masks,default value:0x0000_0000 */
#define DMA_TRANS_DONE_INT (0x1 << 24)

/* SPIF Trans Phase Configure Register Bit Fields & Masks,default value:0x0000_0000 */
#define SPIF_RX_TRANS_EN    (0x1 << 8)
#define SPIF_TX_TRANS_EN    (0x1 << 12)
#define SPIF_DUMMY_TRANS_EN (0x1 << 16)
#define SPIF_MODE_TRANS_EN  (0x1 << 20)
#define SPIF_ADDR_TRANS_EN  (0x1 << 24)
#define SPIF_CMD_TRANS_EN   (0x1 << 28)

/* SPIF Trans Configure2 Register Bit Fields & Masks,default value:0x0000_0000 */
#define SPIF_DATA_TRANS_POS  (0)
#define SPIF_MODE_TRANS_POS  (2)
#define SPIF_ADDR_TRANS_POS  (4)
#define SPIF_CMD_TRANS_POS   (6)
#define SPIF_MODE_OPCODE_POS (16)
#define SPIF_CMD_OPCODE_POS  (24)

/* SPIF Trans Number Register Bit Fields & Masks,default value:0x0000_0000 */
#define SPIF_ADDR_SIZE_24BIT	(0x0 << 24)
#define SPIF_ADDR_SIZE_32BIT	(0x1 << 24)
#define SPIF_ADDR_SIZE_MASK	(0x1 << 24)
/* V1.2 update CMD/ADDR_LENGTH */
#define SPIF_ADDR_SIZE_8BIT_V2	(0x0 << 24)
#define SPIF_ADDR_SIZE_16BIT_V2 (0x1 << 24)
#define SPIF_ADDR_SIZE_24BIT_V2 (0x2 << 24)
#define SPIF_ADDR_SIZE_32BIT_V2 (0x3 << 24)
#define SPIF_ADDR_SIZE_MASK_V2	(0x3 << 24)
#define SPIF_CMD_LEN_1BTYE	(0x0 << 26)
#define SPIF_CMD_LEN_2BTYE	(0x1 << 26)
#define SPIF_CMD_LEN_MASK	(0x1 << 26)

#define SPIF_DUMMY_NUM_POS (16)
#define SPIF_DATA_NUM_POS  (0)

/* SPIF DMA Control Register Bit Fields & Masks,default value:0x0000_0000 */
#define CFG_DMA_START	   (1 << 0)
#define DMA_DESCRIPTOR_LEN (32 << 4)

/* DMA descriptor0 Bit Fields & Masks */
#define HBURST_SINGLE_TYPE (0x0 << 4)
#define HBURST_INCR4_TYPE  (0x3 << 4)
#define HBURST_INCR8_TYPE  (0x5 << 4)
#define HBURST_INCR16_TYPE (0x7 << 4)
#define HBURST_TYPE	   (0x7 << 4)
#define DMA_RW_PROCESS	   (0x1 << 1) /* 0:Read  1:Write */
#define DMA_FINISH_FLASG   (0x1 << 0) /* The Last One Descriptor */

/* DMA descriptor1 Bit Fields & Masks */
#define DMA_BLK_LEN_8B	   (0x0 << 24)
#define DMA_BLK_LEN_16B	   (0x1 << 24)
#define DMA_BLK_LEN_32B	   (0x2 << 24)
#define DMA_BLK_LEN_64B	   (0x3 << 24)
#define DMA_BLK_LEN	   (0xff << 24)
#define DMA_DATA_LEN	   (0x1ffff)
#define SPIF_MAX_TRANS_NUM (65536)

#define DMA_TRANS_NUM_16BIT (1 << 31)
#define SPIF_MIN_TRANS_NUM  (8)
#define DMA_TRANS_NUM	    (0xffff)
#define DMA_DATA_LEN_POS    (0)

/* DMA descriptor7 Bit Fields & Masks */

#define SPIF_DES_NORMAL_EN (0x1 << 28)

#define SPIF_OCTAL_MODE	 (8)
#define SPIF_QUAD_MODE	 (4)
#define SPIF_DUEL_MODE	 (2)
#define SPIF_SINGLE_MODE (1)

#define NOR_PAGE_SIZE (256)

#define CONFIG_SYS_CACHELINE_SIZE 32
#define ROUND_UP(a, b)		  (((a) + (b)-1) & ~((b)-1))
#define ROUND_DOWN(a, b)	  (((a) - (b) + 1) & ~((b)-1))
#define CHECK_IS_NOALIGN(x)	  ((x) % CONFIG_SYS_CACHELINE_SIZE)

#define SAMP_MODE_DL_DEFAULT 0xaaaaffff
#define SAMP_MODE_0	     0 // delay  0 clk
#define SAMP_MODE_1	     1 // delay  0.5 clk
#define SAMP_MODE_2	     2 // delay  1 clk
#define SAMP_MODE_3	     3 // delay  1.5 clk
#define SAMP_MODE_4	     4 // delay  2 clk
#define SAMP_MODE_5	     5 // delay  2.5 clk
#define SAMP_MODE_6	     6 // delay  3 clk
#define SAMP_MODE_7	     7 // delay  3.5 clk

#define min_t(type, a, b)                                                                                                                  \
	({                                                                                                                                 \
		type __a = (a);                                                                                                            \
		type __b = (b);                                                                                                            \
		__a < __b ? __a : __b;                                                                                                     \
	})

#define max_t(type, a, b)                                                                                                                  \
	({                                                                                                                                 \
		type __a = (a);                                                                                                            \
		type __b = (b);                                                                                                            \
		__a > __b ? __a : __b;                                                                                                     \
	})

struct sunxi_spif_slave {
	uint32_t max_hz;
	uint32_t mode;
	int cs_bitmap; /* cs0- 0x1; cs1-0x2, cs0&cs1-0x3. */
	uint32_t base_addr;
	unsigned int right_sample_delay;
	unsigned int right_sample_mode;
	unsigned int rx_dtr_en;
	unsigned int tx_dtr_en;
};

struct spif_descriptor_op {
	uint32_t hburst_rw_flag;
	uint32_t block_data_len;
	uint32_t data_addr;
	uint32_t next_des_addr;
	uint32_t trans_phase; //0x20
	uint32_t flash_addr; //0x24
	uint32_t cmd_mode_buswidth; //0x28
	uint32_t addr_dummy_data_count; //0x2c
};

/**
 * @brief Retrieves the SPI Flash slave structure.
 *
 * @return struct sunxi_spif_slave* Pointer to the SPI Flash slave structure.
 */
struct sunxi_spif_slave *get_sspif(void);

/**
 * @brief Transfers data via SPI Flash.
 *
 * @param spif_op A pointer to the SPI Flash operation descriptor.
 * @param data_len The length of data to be transferred.
 *
 * @return int Returns 0 if successful, non-zero if failed.
 */
int spif_xfer(struct spif_descriptor_op *spif_op, unsigned int data_len);

/**
 * @brief Gets the version of the SPI Flash interface.
 *
 * @return uint32_t The version of the SPI Flash interface.
 */
uint32_t sunxi_spif_get_version(void);

/**
 * @brief Initializes the SPI Flash interface.
 *
 * @return int Returns 0 if successful, non-zero if failed.
 */
int spif_init(void);

/**
 * @brief Deinitializes the SPI Flash interface.
 *
 * This function releases any resources or settings used by the SPI Flash interface.
 */
void spif_exit(void);

/**
 * @brief Sets the clock for the SPI Flash interface.
 *
 * @return int Returns 0 if successful, non-zero if failed.
 */
int set_spif_clk(void);

/**
 * @brief Configures the sample mode for the SPI Flash interface.
 *
 * @param base_addr Base address of the SPI Flash controller.
 * @param status The status value to set for sample mode.
 */
void spif_samp_mode(void *base_addr, unsigned int status);

/**
 * @brief Configures the sample delay for SPI Flash RX status.
 *
 * @param base_addr Base address of the SPI Flash controller.
 * @param status The status value to set for sample delay.
 */
void spif_samp_dl_sw_rx_status(void *base_addr, unsigned int status);

/**
 * @brief Sets the sample mode for the SPI Flash interface.
 *
 * @param base_addr Base address of the SPI Flash controller.
 * @param mode The sample mode to set.
 */
void spif_set_sample_mode(void *base_addr, unsigned int mode);

/**
 * @brief Sets the sample delay for the SPI Flash interface.
 *
 * @param base_addr Base address of the SPI Flash controller.
 * @param sample_delay The sample delay value to set.
 */
void spif_set_sample_delay(void *base_addr, unsigned int sample_delay);

/**
 * @brief Locks the SPI Flash memory for exclusive access.
 *
 * @return int Returns 0 if successful, non-zero if failed.
 */
int spif_flash_lock(void);

/**
 * @brief Unlocks the SPI Flash memory, allowing access.
 *
 * This function should be called after the memory is no longer in exclusive use.
 */
void spif_flash_unlock(void);

#endif /* __SPIF_H__ */

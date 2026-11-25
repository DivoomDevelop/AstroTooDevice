/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY'S TECHNOLOGY.
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

#ifndef __DMA_SUN251IW1_H__
#define __DMA_SUN251IW1_H__

#define DMA_IRQ_NUM		           (66)	
#define DMA_SECURE_IRQ_NUM	       (67)	
#define NR_MAX_CHAN		           16	/* total of channels */
#define START_CHAN_OFFSET          0
#define SUNXI_CLK_DMA              CLK_APP_BUS_DMA
#define SUNXI_CLK_MBUS_DMA         CLK_APP_BUS_DMA_M
#define SUNXI_RST_DMA              RST_APP_BUS_DMA
#define HAL_SUNXI_RESET            AW_APP_CCU
#define HAL_SUNXI_CCU              AW_APP_CCU
#define SUNXI_DMAC_PBASE	       (0x03002000ul)

/* F135 supports BMode */
#define SUNXI_HAS_TIMEOUT

/*
 * The source DRQ type and port corresponding relation
 */
#define DRQSRC_SDRAM               0
#define DRQSRC_LSPSRAM             1
#define DRQSRC_OWA                 2 
#define DRQSRC_I2S0_RX             3
#define DRQSRC_I2S1_RX             4 
#define DRQSRC_I2S2_RX             5
#define DRQSRC_AUDIO_CODEC         7
#define DRQSRC_DMIC                8
#define DRQSRC_GPADC               12
#define DRQSRC_TPADC               13
#define DRQSRC_UART0_RX            14
#define DRQSRC_UART1_RX            15
#define DRQSRC_UART2_RX            16
#define DRQSRC_UART3_RX            17
#define DRQSRC_UART4_RX            18
#define DRQSRC_UART5_RX            19
#define DRQSRC_SPI0_RX             22
#define DRQSRC_SPI1_RX             23
#define DRQSRC_OTG_EP1             30
#define DRQSRC_OTG_EP2             31
#define DRQSRC_OTG_EP3             32
#define DRQSRC_OTG_EP4             33
#define DRQSRC_OTG_EP5             34
#define DRQSRC_TWI0_RX             43
#define DRQSRC_TWI1_RX             44
#define DRQSRC_TWI2_RX             45
#define DRQSRC_TWI3_RX             46

/*
 * The destination DRQ type and port corresponding relation
 */
#define DRQDST_SDRAM               0
#define DRQDST_LSPSRAM             1
#define DRQDST_OWA                 2 
#define DRQDST_I2S0_TX             3
#define DRQDST_I2S1_TX             4 
#define DRQDST_I2S2_TX             5
#define DRQDST_AUDIO_CODEC         7
#define DRQDST_IR_TX               13
#define DRQDST_UART0_TX            14
#define DRQDST_UART1_TX            15
#define DRQDST_UART2_TX            16
#define DRQDST_UART3_TX            17
#define DRQDST_UART4_TX            18
#define DRQDST_UART5_TX            19
#define DRQDST_SPI0_TX             22
#define DRQDST_SPI1_TX             23
#define DRQDST_OTG_EP1             30
#define DRQDST_OTG_EP2             31
#define DRQDST_OTG_EP3             32
#define DRQDST_OTG_EP4             33
#define DRQDST_OTG_EP5             34
#define DRQDST_LEDC                42
#define DRQDST_TWI0_TX             43
#define DRQDST_TWI1_TX             44
#define DRQDST_TWI2_TX             45
#define DRQDST_TWI3_TX             46

#endif /*__DMA_SUN251IW1_H__  */

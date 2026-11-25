/* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
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

#ifndef _CCU_SUN300IW1_APP_H_
#define _CCU_SUN300IW1_APP_H_

#define SUN300IW1_APP_CCU_BASE      0x42001000  /* CCU_APP */

#define CLK_DRAM        0
#define CLK_E907_TS     1
#define CLK_AL27_MT     2
#define CLK_SMHC0       3
#define CLK_SS          4
#define CLK_SPI         5
#define CLK_SPIF        6
#define CLK_MCSI        7
#define CLK_CSI_MASTER0     8
#define CLK_CSI_MASTER1     9
#define CLK_SPI2        10
#define CLK_TCONLCD     11
#define CLK_DE          12
#define CLK_G2D         13
#define CLK_GPADC0_24M      14
#define CLK_VE          15
#define CLK_AUDIO_DAC       16
#define CLK_AUDIO_ADC       17
#define CLK_DMIC        18
#define CLK_I2S0        19
#define CLK_I2S1        20
#define CLK_SMHC1       21
#define CLK_PLL_AUDIO_4X    22
#define CLK_PLL_AUDIO_1X    23
#define CLK_SPI1        24
#define CLK_GMAC_FANOUT     25
#define CLK_A27_MSGBOX      26
#define CLK_WIEGAND_24M     27
#define CLK_USB_24M     28
#define CLK_USB_12M     29
#define CLK_USB_48M     30
#define CLK_AVS         31
#define CLK_GMAC_25M        32
#define CLK_BUS_DPSS_TOP    33
#define CLK_BUS_GPIO        34
#define CLK_BUS_WKT     35
#define CLK_MCSI_AHB        36
#define CLK_MBUS_MCSI       37
#define CLK_VID_OUT     38
#define CLK_MBUS_VID_OUT    39
#define CLK_HBUS_GMAC       40
#define CLK_USBOHCI     41
#define CLK_USBEHCI     42
#define CLK_USBOTG      43
#define CLK_USB         44
#define CLK_UART3       45
#define CLK_UART2       46
#define CLK_UART1       47
#define CLK_UART0       48
#define CLK_TWI0        49
#define CLK_PWM         50
#define CLK_TRNG        51
#define CLK_TIMER       52
#define CLK_SG_DMA      53
#define CLK_DMA         54
#define CLK_SYSCRTL     55
#define CLK_CE          56
#define CLK_HSTIMER     57
#define CLK_SPLOCK      58
#define CLK_BUS_DRAM        59
#define CLK_RV_MSGBOX   60
#define CLK_RISCV       61
#define CLK_MBUS_G2D        62
#define CLK_BUS_G2D     63
#define CLK_G2D_HB      64
#define CLK_TWI2        65
#define CLK_TWI1        66
#define CLK_BUS_SPI2        67
#define CLK_BUS_GMAC        68
#define CLK_BUS_SMHC1       69
#define CLK_BUS_SMHC0       70
#define CLK_BUS_SPI1        71
#define CLK_GBGSYS      72
#define CLK_MBUS_GMAC       73
#define CLK_MBUS_SMHC1      74
#define CLK_MBUS_SMHC0      75
#define CLK_MBUS_USB        76
#define CLK_MBUS_DMA        77
#define CLK_BUS_I2S1        78
#define CLK_BUS_I2S0        79
#define CLK_BUS_DMIC        80
#define CLK_BUS_ADDA        81
#define CLK_BUS_SPIF        82
#define CLK_BUS_SPI     83
#define CLK_BUS_VE_AHB      84
#define CLK_MBUS_VE     85
#define CLK_BUS_THS     86
#define CLK_GPA         87
#define CLK_HBUS_MCSI       88
#define CLK_SBUS_MCSI       89
#define CLK_BUS_MISP        90
#define CLK_SD_MONITOR      91
#define CLK_AHB_MONITOR     92
#define CLK_SBUS_VE     93
#define CLK_HBUS_VE     94
#define CLK_BUS_TCON        95
#define CLK_SGDMA       96
#define CLK_BUS_DE      97
#define CLK_DE_HB       98

#define CLK_NUMBER	(CLK_DE_HB + 1)

int sunxi_ccu_app_init(void);

#endif /* _CCU_SUN300IW1_APP_H_ */

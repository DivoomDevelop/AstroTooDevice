/* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.

 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.

 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PART'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNER'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PART'S TECHNOLOGY.


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
#ifndef __SUNXI_HCI_H__
#define __SUNXI_HCI_H__

#include "uhc_inner.h"
#include "sunxi_hal_common.h"
#include "ehci-driver.h"

#define USBC_Readb(reg) (*((volatile unsigned char *)(long)(reg)))
#define USBC_Readw(reg) (*((volatile unsigned short *)(reg)))
#define USBC_Readl(reg) (*((volatile unsigned int *)(long)(reg)))

#define USBC_Writeb(value, reg) (*((volatile unsigned char *)(long)(reg)) = (unsigned char)(value))
#define USBC_Writew(value, reg) (*((volatile unsigned short *)(reg)) = (unsigned short)(value))
#define USBC_Writel(value, reg) (*((volatile unsigned int *)(long)(reg)) = (unsigned int)(value))

#define USBC_REG_test_bit_b(bp, reg) (USBC_Readb(reg) & (1 << (bp)))
#define USBC_REG_test_bit_w(bp, reg) (USBC_Readw(reg) & (1 << (bp)))
#define USBC_REG_test_bit_l(bp, reg) (USBC_Readl(reg) & (1 << (bp)))

#define USBC_REG_set_bit_b(bp, reg) (USBC_Writeb((USBC_Readb(reg) | (1 << (bp))), (reg)))
#define USBC_REG_set_bit_w(bp, reg) (USBC_Writew((USBC_Readw(reg) | (1 << (bp))), (reg)))
#define USBC_REG_set_bit_l(bp, reg) (USBC_Writel((USBC_Readl(reg) | (1 << (bp))), (reg)))

#define USBC_REG_clear_bit_b(bp, reg) (USBC_Writeb((USBC_Readb(reg) & (~(1 << (bp)))), (reg)))
#define USBC_REG_clear_bit_w(bp, reg) (USBC_Writew((USBC_Readw(reg) & (~(1 << (bp)))), (reg)))
#define USBC_REG_clear_bit_l(bp, reg) (USBC_Writel((USBC_Readl(reg) & (~(1 << (bp)))), (reg)))

#define HCI0_USBC_NO 0
#define HCI1_USBC_NO 1
#define HCI2_USBC_NO 2
#define HCI3_USBC_NO 3

#define SUNXI_USB_EHCI_BASE_OFFSET 0x00
#define SUNXI_USB_OHCI_BASE_OFFSET 0x400
#define SUNXI_USB_PHY_BASE_OFFSET 0x800
#define SUNXI_USB_EHCI_LEN 0x58
#define SUNXI_USB_OHCI_LEN 0x58

#define SUNXI_USB_EHCI_TIME_INT 0x30
#define SUNXI_USB_EHCI_STANDBY_IRQ_STATUS 1
#define SUNXI_USB_EHCI_STANDBY_IRQ 2

#define SUNXI_USB_PMU_IRQ_ENABLE 0x800
#define SUNXI_HCI_CTRL_3 0X808
#define SUNXI_HCI_PHY_CTRL 0x810
#define SUNXI_HCI_PHY_TUNE 0x818
#define SUNXI_HCI_UTMI_PHY_STATUS 0x824
#define SUNXI_HCI_CTRL_3_REMOTE_WAKEUP 3
#define SUNXI_HCI_RC16M_CLK_ENBALE 2
#if defined(CONFIG_ARCH_SUN20IW2)
#define SUNXI_HCI_PHY_CTRL_SIDDQ 1
#else
#define SUNXI_HCI_PHY_CTRL_SIDDQ 3
#endif
#if defined(CONFIG_ARCH_SUN20IW2)
#define SUNXI_GPRCM_BASE (0x40050000)
#define USB_BIAS_CTRL (0x0064)
#define USB_BIAS_CTRL_EN (0x0001)
#endif

#define SUNXI_OTG_PHY_CTRL 0x410
#define SUNXI_OTG_PHY_CFG 0x420
#define SUNXI_OTG_PHY_STATUS 0x424
#define SUNXI_USBC_REG_INTUSBE 0x0050

#define EHCI_CAP_OFFSET (0x00)
#define EHCI_CAP_LEN (0x10)

#define EHCI_CAP_CAPLEN (EHCI_CAP_OFFSET + 0x00)
#define EHCI_CAP_HCIVER (EHCI_CAP_OFFSET + 0x00)
#define EHCI_CAP_HCSPAR (EHCI_CAP_OFFSET + 0x04)
#define EHCI_CAP_HCCPAR (EHCI_CAP_OFFSET + 0x08)
#define EHCI_CAP_COMPRD (EHCI_CAP_OFFSET + 0x0c)

#define EHCI_OPR_OFFSET (EHCI_CAP_OFFSET + EHCI_CAP_LEN)

#define EHCI_OPR_USBCMD (EHCI_OPR_OFFSET + 0x00)
#define EHCI_OPR_USBSTS (EHCI_OPR_OFFSET + 0x04)
#define EHCI_OPR_USBINTR (EHCI_OPR_OFFSET + 0x08)
#define EHCI_OPR_FRINDEX (EHCI_OPR_OFFSET + 0x0c)
#define EHCI_OPR_CRTLDSS (EHCI_OPR_OFFSET + 0x10)
#define EHCI_OPR_PDLIST (EHCI_OPR_OFFSET + 0x14)
#define EHCI_OPR_ASLIST (EHCI_OPR_OFFSET + 0x18)
#define EHCI_OPR_CFGFLAG (EHCI_OPR_OFFSET + 0x40)
#define EHCI_OPR_PORTSC (EHCI_OPR_OFFSET + 0x44)

/**
 * PORT Control and Status Register
 * port_no is 0 based, 0, 1, 2, .....
 *
 * Reg EHCI_OPR_PORTSC
 */
/* Port Test Control bits */
//#define EHCI_PORTSC_PTC_MASK	            (0xf<<16)
#define EHCI_PORTSC_PTC_DIS (0x0 << 16)
#define EHCI_PORTSC_PTC_J (0x1 << 16)
#define EHCI_PORTSC_PTC_K (0x2 << 16)
//#define EHCI_PORTSC_PTC_SE0NAK	        (0x3<<16)
//#define EHCI_PORTSC_PTC_PACKET	            (0x4<<16)
#define EHCI_PORTSC_PTC_FORCE (0x5 << 16)

//#define EHCI_PORTSC_OWNER	                (0x1<<13)
#define EHCI_PORTSC_POWER (0x1 << 12)

#define EHCI_PORTSC_LS_MASK (0x3 << 10)
#define EHCI_PORTSC_LS_SE0 (0x0 << 10)
#define EHCI_PORTSC_LS_J (0x2 << 10)
#define EHCI_PORTSC_LS_K (0x1 << 10)
#define EHCI_PORTSC_LS_UDF (0x3 << 10)

//#define EHCI_PORTSC_RESET	                (0x1<<8)
//#define EHCI_PORTSC_SUSPEND	                (0x1<<7)
//#define EHCI_PORTSC_RESUME	                (0x1<<6)
//#define EHCI_PORTSC_OCC		                (0x1<<5)
#define EHCI_PORTSC_OC (0x1 << 4)
//#define EHCI_PORTSC_PEC		                (0x1<<3)
//#define EHCI_PORTSC_PE		                (0x1<<2)
//#define EHCI_PORTSC_CSC		                (0x1<<1)
//#define EHCI_PORTSC_CCS		                (0x1<<0)

#define EHCI_PORTSC_CHANGE (EHCI_PORTSC_OCC | EHCI_PORTSC_PEC | EHCI_PORTSC_CSC)

#define SUNXI_USB_HCI_DEBUG

#define KEY_USB_ENABLE "usb_used"
#define KEY_USB_DRVVBUS_TYPE "usb_drv_vbus_type"
#define KEY_USB_DRVVBUS_GPIO "usb_drv_vbus_gpio"
#define KEY_USB_REGULATOR_IO "usb_regulator_io"
#define KEY_USB_REGULATOR_IO_VOL "usb_regulator_vol"
#define KEY_USB_WAKEUP_SUSPEND "usb_wakeup_suspend"
#define KEY_USB_HSIC_USBED "usb_hsic_used"
#define KEY_USB_HSIC_CTRL "usb_hsic_ctrl"
#define KEY_USB_HSIC_RDY_GPIO "usb_hsic_rdy_gpio"
#define KEY_USB_HSIC_REGULATOR_IO "usb_hsic_regulator_io"
#define KEY_WAKEUP_SOURCE "wakeup-source"
#define KEY_USB_PORT_TYPE "usb_port_type"
#define KEY_USB_DRIVER_LEVEL "usbh_driver_level"
#define KEY_USB_IRQ_FLAG "usbh_irq_flag"

/* xHCI */
#define XHCI_RESOURCES_NUM 2
#define XHCI_REGS_START 0x0
#define XHCI_REGS_END 0x7fff

/* xHCI Operational Registers */
#define XHCI_OP_REGS_HCUSBCMD 0X0020
#define XHCI_OP_REGS_HCUSBSTS 0X0024
#define XHCI_OP_REGS_HCPORT1SC 0X0420
#define XHCI_OP_REGS_HCPORT1PMSC 0X0424

#define SUNXI_GLOBALS_REGS_START 0xc100
#define SUNXI_GLOBALS_REGS_END 0xc6ff

/* Global Registers */
#define SUNXI_GLOBALS_REGS_GCTL 0xc110
#define SUNXI_GUSB2PHYCFG(n) (0xc200 + (n * 0x04))
#define SUNXI_GUSB3PIPECTL(n) (0xc2c0 + (n * 0x04))

/* Interface Status and Control Register */
#define SUNXI_APP 0x10000
#define SUNXI_PIPE_CLOCK_CONTROL 0x10014
#define SUNXI_PHY_TUNE_LOW 0x10018
#define SUNXI_PHY_TUNE_HIGH 0x1001c
#define SUNXI_PHY_EXTERNAL_CONTROL 0x10020

/* Bit fields */

/* Global Configuration Register */
#define SUNXI_GCTL_PRTCAPDIR(n) ((n) << 12)
#define SUNXI_GCTL_PRTCAP_HOST 1
#define SUNXI_GCTL_PRTCAP_DEVICE 2
#define SUNXI_GCTL_PRTCAP_OTG 3
#define SUNXI_GCTL_SOFITPSYNC (0x01 << 10)
#define SUNXI_GCTL_CORESOFTRESET (1 << 11)

/* Global USB2 PHY Configuration Register n */
#define SUNXI_USB2PHYCFG_SUSPHY (0x01 << 6)
#define SUNXI_USB2PHYCFG_PHYSOFTRST (1 << 31)

/* Global USB3 PIPE Control Register */
#define SUNXI_USB3PIPECTL_PHYSOFTRST (1 << 31)

/* USB2.0 Interface Status and Control Register */
#define SUNXI_APP_FOCE_VBUS (0x03 << 12)

/* PIPE Clock Control Register */
#define SUNXI_PPC_PIPE_CLK_OPEN (0x01 << 6)

/* PHY External Control Register */
#define SUNXI_PEC_EXTERN_VBUS (0x03 << 1)
#define SUNXI_PEC_SSC_EN (0x01 << 24)
#define SUNXI_PEC_REF_SSP_EN (0x01 << 26)

/* PHY Tune High Register */
#define SUNXI_TX_DEEMPH_3P5DB(n) ((n) << 19)
#define SUNXI_TX_DEEMPH_6DB(n) ((n) << 13)
#define SUNXI_TX_SWING_FULL(n) ((n) << 6)
#define SUNXI_LOS_BIAS(n) ((n) << 3)
#define SUNXI_TXVBOOSTLVL(n) ((n) << 0)

/* HCI UTMI PHY TUNE */
#define SUNXI_TX_VREF_TUNE_OFFSET 8
#define SUNXI_TX_RISE_TUNE_OFFSET 4
#define SUNXI_TX_RES_TUNE_OFFSET 2
#define SUNXI_TX_PREEMPAMP_TUNE_OFFSET 0
#define SUNXI_TX_VREF_TUNE (0xf << SUNXI_TX_VREF_TUNE_OFFSET)
#define SUNXI_TX_RISE_TUNE (0x3 << SUNXI_TX_RISE_TUNE_OFFSET)
#define SUNXI_TX_RES_TUNE (0x3 << SUNXI_TX_RES_TUNE_OFFSET)
#define SUNXI_TX_PREEMPAMP_TUNE (0x3 << SUNXI_TX_PREEMPAMP_TUNE_OFFSET)

enum sunxi_usbc_used {
	SUNXI_USB_DISABLE = 0,
	SUNXI_USB_ENABLE,
};

enum sunxi_usbc_type {
	SUNXI_USB_UNKNOWN = 0,
	SUNXI_USB_EHCI,
	SUNXI_USB_OHCI,
	SUNXI_USB_XHCI,
};

enum usb_drv_vbus_type {
	USB_DRV_VBUS_TYPE_NULL = 0,
	USB_DRV_VBUS_TYPE_GIPO,
	USB_DRV_VBUS_TYPE_AXP,
};

enum usb_wakeup_source_type {
	SUPER_STANDBY = 0,
	USB_STANDBY,
	NORMAL_STANDBY,
};

struct uhc_board {
	int drv_vbus_type;
	unsigned int drv_vbus_gpio_valid;
	int drv_vbus_gpio_set;
	int usb_driver_level;
	int usb_irq_flag;
};

struct uhc_platform {
	hal_clk_t bus_clk;
	hal_clk_t phy_clk;
	hal_clk_t ohci_clk;

	struct reset_control *reset_hci;
	struct reset_control *reset_phy;
};

struct sunxi_hci {
	int usbcx_num; /* usb controller number */
	char name[32]; /* hci name */
	unsigned int status;
#define SUNXI_HCI_STATUS_INIT (0)
#define sunxi_hci_status_check_init(sunxi_hci) \
	(!!((sunxi_hci)->status & (0x1 << SUNXI_HCI_STATUS_INIT)))
#define sunxi_hci_status_set_init(sunxi_hci) ((sunxi_hci)->status |= (0x1 << SUNXI_HCI_STATUS_INIT))
#define sunxi_hci_status_clear_init(sunxi_hci) \
	((sunxi_hci)->status &= ~(0x1 << SUNXI_HCI_STATUS_INIT))

#define SUNXI_HCI_STATUS_START (1)
#define sunxi_hci_status_check_start(sunxi_hci) \
	(!!((sunxi_hci)->status & (0x1 << SUNXI_HCI_STATUS_START)))
#define sunxi_hci_status_set_start(sunxi_hci) \
	((sunxi_hci)->status |= (0x1 << SUNXI_HCI_STATUS_START))
#define sunxi_hci_status_clear_start(sunxi_hci) \
	((sunxi_hci)->status &= ~(0x1 << SUNXI_HCI_STATUS_START))

	struct platform_usb_config hwinfo; /* usbcx hardware information */
	struct platform_usb_config otginfo;
	struct uhc_board board_config; /* board configuration */
	struct uhc_platform plat; /* platform handler */

	unsigned int hci_base;
	unsigned int hci_reg_length;

	int (*open_clock)(struct sunxi_hci *sx_hci);
	int (*close_clock)(struct sunxi_hci *sx_hci);
	int (*set_power)(struct sunxi_hci *sx_hci, int ison);
	int (*port_configure)(struct sunxi_hci *sx_hci, u32 enable);
	int (*usb_passby)(struct sunxi_hci *sx_hci, u32 enable);
	int (*phy_ctrl)(struct sunxi_hci *sx_hci, u32 enable);

	struct hcd hcd;
};

static inline struct sunxi_hci *hcd_to_sunxi_hci(struct hcd *hcd)
{
	return container_of((void *)hcd, struct sunxi_hci, hcd);
}

/* board */
int uhc_board_check_usbcx_is_enable(int usbhwc_num);
int uhc_board_hci_get_config_param(struct sunxi_hci *sx_hci);
/* platform */
int uhc_platform_open_clk(struct uhc_platform *plat);
int uhc_platform_close_clk(struct uhc_platform *plat);
/* hci */
int sunxi_hci_open_clock(struct sunxi_hci *sx_hci);
int sunxi_hci_close_clock(struct sunxi_hci *sx_hci);
int sunxi_hci_set_vbus(struct sunxi_hci *sx_hci, int is_on);
int sunxi_hci_usb_passby(struct sunxi_hci *sx_hci, u32 enable);

#endif
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
#ifndef __LINUX_USB_EHCI_DEF_H
#define __LINUX_USB_EHCI_DEF_H

#include "uhc_inner.h"

#define ehci_big_endian_mmio(e) 0
#define ehci_big_endian_capbase(e) 0

/* EHCI register interface, corresponds to EHCI Revision 0.95 specification */

/* Section 2.2 Host Controller Capability Registers */
struct ehci_caps {
	/* these fields are specified as 8 and 16 bit registers,
	 * but some hosts can't perform 8 or 16 bit PCI accesses.
	 * some hosts treat caplength and hciversion as parts of a 32-bit
	 * register, others treat them as two separate registers, this
	 * affects the memory map for big endian controllers.
	 */
	uint32_t hc_capbase;
#define HC_LENGTH(ehci, p) \
	(0x00ff & ((p) >> /* bits 7:0 / offset 00h */ \
		   (ehci_big_endian_capbase(ehci) ? 24 : 0)))
#define HC_VERSION(ehci, p) \
	(0xffff & ((p) >> /* bits 31:16 / offset 02h */ \
		   (ehci_big_endian_capbase(ehci) ? 0 : 16)))
	uint32_t hcs_params; /* HCSPARAMS - offset 0x4 */
#define HCS_DEBUG_PORT(p) (((p) >> 20) & 0xf) /* bits 23:20, debug port? */
#define HCS_INDICATOR(p) ((p) & (1 << 16)) /* true: has port indicators */
#define HCS_N_CC(p) (((p) >> 12) & 0xf) /* bits 15:12, #companion HCs */
#define HCS_N_PCC(p) (((p) >> 8) & 0xf) /* bits 11:8, ports per CC */
#define HCS_PORTROUTED(p) ((p) & (1 << 7)) /* true: port routing */
#define HCS_PPC(p) ((p) & (1 << 4)) /* true: port power control */
#define HCS_N_PORTS(p) (((p) >> 0) & 0xf) /* bits 3:0, ports on HC */
#define HCS_N_PORTS_MAX 15 /* N_PORTS valid 0x1-0xF */

	uint32_t hcc_params; /* HCCPARAMS - offset 0x8 */
/* EHCI 1.1 addendum */
#define HCC_32FRAME_PERIODIC_LIST(p) ((p) & (1 << 19))
#define HCC_PER_PORT_CHANGE_EVENT(p) ((p) & (1 << 18))
#define HCC_LPM(p) ((p) & (1 << 17))
#define HCC_HW_PREFETCH(p) ((p) & (1 << 16))

#define HCC_EXT_CAPS(p) (((p) >> 8) & 0xff) /* for pci extended caps */
#define HCC_ISOC_CACHE(p) ((p) & (1 << 7)) /* true: can cache isoc frame */
#define HCC_ISOC_THRES(p) (((p) >> 4) & 0x7) /* bits 6:4, uframes cached */
#define HCC_CANPARK(p) ((p) & (1 << 2)) /* true: can park on async qh */
#define HCC_PGM_FRAMELISTLEN(p) ((p) & (1 << 1)) /* true: periodic_size changes*/
#define HCC_64BIT_ADDR(p) ((p) & (1)) /* true: can use 64-bit addr */
	uint8_t portroute[8]; /* nibbles for routing - offset 0xC */
} __attribute__((packed, aligned(4)));

/* Section 2.3 Host Controller Operational Registers */
struct ehci_regs {
	/* USBCMD: offset 0x00 */
	uint32_t command;

/* EHCI 1.1 addendum */
#define CMD_HIRD (0xf << 24) /* host initiated resume duration */
#define CMD_PPCEE (1 << 15) /* per port change event enable */
#define CMD_FSP (1 << 14) /* fully synchronized prefetch */
#define CMD_ASPE (1 << 13) /* async schedule prefetch enable */
#define CMD_PSPE (1 << 12) /* periodic schedule prefetch enable */
/* 23:16 is r/w intr rate, in microframes; default "8" == 1/msec */
#define CMD_PARK (1 << 11) /* enable "park" on async qh */
#define CMD_PARK_CNT(c) (((c) >> 8) & 3) /* how many transfers to park for */
#define CMD_LRESET (1 << 7) /* partial reset (no ports, etc) */
#define CMD_IAAD (1 << 6) /* "doorbell" interrupt async advance */
#define CMD_ASE (1 << 5) /* async schedule enable */
#define CMD_PSE (1 << 4) /* periodic schedule enable */
/* 3:2 is periodic frame list size */
#define CMD_RESET (1 << 1) /* reset HC not bus */
#define CMD_RUN (1 << 0) /* start/stop HC */

	/* USBSTS: offset 0x04 */
	uint32_t status;
#define STS_PPCE_MASK (0xff << 16) /* Per-Port change event 1-16 */
#define STS_ASS (1 << 15) /* Async Schedule Status */
#define STS_PSS (1 << 14) /* Periodic Schedule Status */
#define STS_RECL (1 << 13) /* Reclamation */
#define STS_HALT (1 << 12) /* Not running (any reason) */
	/* some bits reserved */
	/* these STS_* flags are also intr_enable bits (USBINTR) */
#define STS_IAA (1 << 5) /* Interrupted on async advance */
#define STS_FATAL (1 << 4) /* such as some PCI access errors */
#define STS_FLR (1 << 3) /* frame list rolled over */
#define STS_PCD (1 << 2) /* port change detect */
#define STS_ERR (1 << 1) /* "error" completion (overflow, ...) */
#define STS_INT (1 << 0) /* "normal" completion (short, ...) */

	/* USBINTR: offset 0x08 */
	uint32_t intr_enable;

	/* FRINDEX: offset 0x0C */
	uint32_t frame_index; /* current microframe number */
	/* CTRLDSSEGMENT: offset 0x10 */
	uint32_t segment; /* address bits 63:32 if needed */
	/* PERIODICLISTBASE: offset 0x14 */
	uint32_t frame_list; /* points to periodic list */
	/* ASYNCLISTADDR: offset 0x18 */
	uint32_t async_next; /* address of next async queue head */

	uint32_t reserved1[2];

	/* TXFILLTUNING: offset 0x24 */
	uint32_t txfill_tuning; /* TX FIFO Tuning register */
#define TXFIFO_DEFAULT (8 << 16) /* FIFO burst threshold 8 */

	uint32_t reserved2[6];

	/* CONFIGFLAG: offset 0x40 */
	uint32_t configured_flag;
#define FLAG_CF (1 << 0) /* true: we'll support "high speed" */

	union {
		/* PORTSC: offset 0x44 */
		uint32_t port_status[HCS_N_PORTS_MAX]; /* up to N_PORTS */
/* EHCI 1.1 addendum */
#define PORTSC_SUSPEND_STS_ACK 0
#define PORTSC_SUSPEND_STS_NYET 1
#define PORTSC_SUSPEND_STS_STALL 2
#define PORTSC_SUSPEND_STS_ERR 3

#define PORT_DEV_ADDR (0x7f << 25) /* device address */
#define PORT_SSTS (0x3 << 23) /* suspend status */
/* 31:23 reserved */
#define PORT_WKOC_E (1 << 22) /* wake on overcurrent (enable) */
#define PORT_WKDISC_E (1 << 21) /* wake on disconnect (enable) */
#define PORT_WKCONN_E (1 << 20) /* wake on connect (enable) */
/* 19:16 for port testing */
#define PORT_TEST(x) (((x)&0xf) << 16) /* Port Test Control */
#define PORT_TEST_PKT PORT_TEST(0x4) /* Port Test Control - packet test */
#define PORT_TEST_FORCE PORT_TEST(0x5) /* Port Test Control - force enable */
#define PORT_LED_OFF (0 << 14)
#define PORT_LED_AMBER (1 << 14)
#define PORT_LED_GREEN (2 << 14)
#define PORT_LED_MASK (3 << 14)
#define PORT_OWNER (1 << 13) /* true: companion hc owns this port */
#define PORT_POWER (1 << 12) /* true: has power (see PPC) */
#define PORT_USB11(x) (((x) & (3 << 10)) == (1 << 10)) /* USB 1.1 device */
/* 11:10 for detecting lowspeed devices (reset vs release ownership) */
/* 9 reserved */
#define PORT_LPM (1 << 9) /* LPM transaction */
#define PORT_RESET (1 << 8) /* reset port */
#define PORT_SUSPEND (1 << 7) /* suspend port */
#define PORT_RESUME (1 << 6) /* resume it */
#define PORT_OCC (1 << 5) /* over current change */
#define PORT_OC (1 << 4) /* over current active */
#define PORT_PEC (1 << 3) /* port enable change */
#define PORT_PE (1 << 2) /* port enable */
#define PORT_CSC (1 << 1) /* connect status change */
#define PORT_CONNECT (1 << 0) /* device connected */
#define PORT_RWC_BITS (PORT_CSC | PORT_PEC | PORT_OCC)
		struct {
			uint32_t reserved3[9];
			/* USBMODE: offset 0x68 */
			uint32_t usbmode; /* USB Device mode */
		};
#define USBMODE_SDIS (1 << 3) /* Stream disable */
#define USBMODE_BE (1 << 2) /* BE/LE endianness select */
#define USBMODE_CM_HC (3 << 0) /* host controller mode */
#define USBMODE_CM_IDLE (0 << 0) /* idle state */
	};
	uint32_t reserved4;

	/* Moorestown has some non-standard registers, partially due to the fact that
	 * its EHCI controller has both TT and LPM support. HOSTPCx are extensions to
	 * PORTSCx
	 */
	/* HOSTPC: offset 0x84 */
	uint32_t hostpc[HCS_N_PORTS_MAX]; /* HOSTPC extension */
#define HOSTPC_PHCD (1 << 22) /* Phy clock disable */
#define HOSTPC_PSPD (3 << 25) /* Port speed detection */

	uint32_t reserved5[2];

	/* USBMODE_EX: offset 0xc8 */
	uint32_t usbmode_ex; /* USB Device mode extension */
#define USBMODE_EX_VBPS (1 << 5) /* VBus Power Select On */
#define USBMODE_EX_HC (3 << 0) /* host controller mode */
} __attribute__((packed, aligned(4)));

#endif /* __LINUX_USB_EHCI_DEF_H */

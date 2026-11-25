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
#ifndef __UHC_INNER_H__
#define __UHC_INNER_H__

#include "usb_os_platform.h"
#include "usb_log.h"
#include "stdio.h"
#include "stdarg.h"
#include "aw_list.h"
#include "hal_mutex.h"
#include "hal_atomic.h"
#include "usb_def.h"
#include "usb_hc.h"
#include "awusb_pm.h"
#include "usb_osal.h"

/* debug */
#define uhc_debug(f, ...) USB_LOG_DBG("%s %d: " f, __func__, __LINE__, ##__VA_ARGS__)
#define uhc_info(f, ...) USB_LOG_INFO("%s %d: " f, __func__, __LINE__, ##__VA_ARGS__)
#define uhc_warn(f, ...) USB_LOG_WRN("%s %d: " f, __func__, __LINE__, ##__VA_ARGS__)
#define uhc_err(f, ...) USB_LOG_ERR("%s %d: " f, __func__, __LINE__, ##__VA_ARGS__)
#define uhc_printf(f, ...) printf("%s %d: " f, __func__, __LINE__, ##__VA_ARGS__)

#define hcd_debug(hcd, f, ...)                                                              \
	USB_LOG_DBG("{%s}%s %d: " f, (hcd)->name ? (hcd)->name : (unsigned char *)"NULL",       \
		    __func__, __LINE__, ##__VA_ARGS__)
#define hcd_info(hcd, f, ...)                                                               \
	USB_LOG_INFO("{%s}%s %d: " f, (hcd)->name ? (hcd)->name : (unsigned char *)"NULL",      \
		     __func__, __LINE__, ##__VA_ARGS__)
#define hcd_warn(hcd, f, ...)                                                               \
	USB_LOG_WRN("{%s}%s %d: " f, (hcd)->name ? (hcd)->name : (unsigned char *)"NULL",       \
		    __func__, __LINE__, ##__VA_ARGS__)
#define hcd_err(hcd, f, ...)                                                                \
	USB_LOG_ERR("{%s}%s %d: " f, (hcd)->name ? (hcd)->name : (unsigned char *)"NULL",       \
		    __func__, __LINE__, ##__VA_ARGS__)
#define hcd_printf(hcd, f, ...)                                                             \
	printf("{%s}%s %d: " f, (hcd)->name ? (hcd)->name : (unsigned char *)"NULL", __func__,  \
	       __LINE__, ##__VA_ARGS__)

#define hci_debug(hci, f, ...)                                                              \
	USB_LOG_DBG("{%s}%s %d: " f, (hci)->name ? (hci)->name : (char *)"NULL", __func__,      \
		    __LINE__, ##__VA_ARGS__)
#define hci_info(hci, f, ...)                                                               \
	USB_LOG_INFO("{%s}%s %d: " f, (hci)->name ? (hci)->name : (char *)"NULL", __func__,     \
		     __LINE__, ##__VA_ARGS__)
#define hci_warn(hci, f, ...)                                                               \
	USB_LOG_WRN("{%s}%s %d: " f, (hci)->name ? (hci)->name : (char *)"NULL", __func__,      \
		    __LINE__, ##__VA_ARGS__)
#define hci_err(hci, f, ...)                                                                \
	USB_LOG_ERR("{%s}%s %d: " f, (hci)->name ? (hci)->name : (char *)"NULL", __func__,      \
		    __LINE__, ##__VA_ARGS__)
#define hci_printf(hci, f, ...)                                                             \
	printf("{%s}%s %d: " f, (hci)->name ? (hci)->name : (char *)"NULL", __func__, __LINE__, \
	       ##__VA_ARGS__)

#define uhc_container_of(ptr, type, member) \
	((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

#define call_hc_driver_void_ops(hcd, op, args...) ({ \
	if ((hcd) && ((hcd)->driver) && ((hcd)->driver->op)) \
		(hcd)->driver->op((hcd), args); \
})

#define call_hc_driver_int_ops(hcd, op, args...) ({ \
	int ret = 0;\
	if ((hcd) && ((hcd)->driver) && ((hcd)->driver->op)) { \
		ret = (hcd)->driver->op((hcd), args); \
	} \
	ret; \
})

struct usb_hc;
struct hcd;

struct hc_driver {
	unsigned char *descriptor;
	unsigned char name[32];
	unsigned int hcd_priv_size;

	unsigned short (*get_frame_number)(struct hcd *hcd);
	int (*rh_control)(struct hcd *hcd, struct usb_setup_packet *setup, uint8_t *buf);
	int (*usb_submit_urb)(struct hcd *hcd, struct usbh_urb *urb);
	int (*usb_kill_urb)(struct hcd *hcd, struct usbh_urb *urb);

	void (*irq_handler)(struct hcd *hcd);
	int (*reset)(struct hcd *hcd);
	int (*start)(struct hcd *hcd);
	int (*stop)(struct hcd *hcd);

	int (*port_power)(struct hcd *hcd, int portnum, bool enable);

	int (*suspend)(struct hcd *hcd, int mode);
	int (*resume)(struct hcd *hcd, int mode);
};

struct hcd {
	struct list_head entry;
	unsigned char name[32];
	int portcnt;
	int region_start;
	int region_size;
	unsigned int status;
#define USB_HCD_STATUS_INIT (0)
#define usb_hcd_status_check_init(hcd) (!!((hcd)->status & (0x1 << USB_HCD_STATUS_INIT)))
#define usb_hcd_status_set_init(hcd) ((hcd)->status |= (0x1 << USB_HCD_STATUS_INIT))
#define usb_hcd_status_clear_init(hcd) ((hcd)->status &= ~(0x1 << USB_HCD_STATUS_INIT))

	const struct hc_driver *driver;
	struct usb_hc *usbhc;

	void *hcd_priv;
};

static inline unsigned short hcd_get_frame_number(struct hcd *hcd)
{
	if (hcd && hcd->driver && hcd->driver->get_frame_number)
		return hcd->driver->get_frame_number(hcd);
	hcd_err(hcd, "get_frame_number is NULL\n");
	return 0;
}

static inline int hcd_rh_control(struct hcd *hcd, struct usb_setup_packet *setup, uint8_t *buf)
{
	if (hcd && hcd->driver && hcd->driver->rh_control)
		return hcd->driver->rh_control(hcd, setup, buf);
	hcd_err(hcd, "rh_control is NULL\n");
	return -1;
}

static inline int hcd_usb_submit_urb(struct hcd *hcd, struct usbh_urb *urb)
{
	if (hcd && hcd->driver && hcd->driver->usb_submit_urb)
		return hcd->driver->usb_submit_urb(hcd, urb);
	hcd_err(hcd, "usb_submit_urb is NULL\n");
	return -1;
}

static inline int hcd_usb_kill_urb(struct hcd *hcd, struct usbh_urb *urb)
{
	if (hcd && hcd->driver && hcd->driver->usb_kill_urb)
		return hcd->driver->usb_kill_urb(hcd, urb);
	hcd_err(hcd, "usb_kill_urb is NULL\n");
	return -1;
}

static inline int hcd_usb_port_power(struct hcd *hcd, int portnum, bool enable)
{
	if (hcd && hcd->driver && hcd->driver->port_power)
		return hcd->driver->port_power(hcd, portnum, enable);
	hcd_err(hcd, "port_power is NULL\n");
	return -1;
}

struct usb_hc {
	struct list_head hcd_list;
	hal_spinlock_t list_lock;
	hal_spinlock_t bitops_lock;
	struct hal_mutex hdl_lock;

	struct awusb_pmhdl pm;
	struct usbh_bus *bus;

	usb_osal_mq_t work_mq;
	usb_osal_thread_t work_thr;
	usb_osal_mutex_t work_lock;

	unsigned int virtual_portid_map;
	unsigned int flags;
#define USB_HC_STATUS_INIT (0)
#define usb_hc_status_check_init(hc) (!!((hc)->flags & (0x1 << USB_HC_STATUS_INIT)))
#define usb_hc_status_set_init(hc) ((hc)->flags |= (0x1 << USB_HC_STATUS_INIT))
#define usb_hc_status_clear_init(hc) ((hc)->flags &= ~(0x1 << USB_HC_STATUS_INIT))

	/* ops */
	int (*hcd_init)(struct usb_hc *usbhc, struct hcd *hcd, struct hc_driver *driver);
	int (*hcd_deinit)(struct usb_hc *usbhc, struct hcd *hcd);
	int (*hcd_add)(struct usb_hc *usbhc, struct hcd *hcd);
	int (*hcd_remove)(struct usb_hc *usbhc, struct hcd *hcd);
#define usbhc_hcd_init(usbhc, hcd, hc_driver) (usbhc)->hcd_init(usbhc, hcd, hc_driver)
#define usbhc_hcd_deinit(usbhc, hcd) (usbhc)->hcd_deinit(usbhc, hcd)
#define usbhc_hcd_add(usbhc, hcd) (usbhc)->hcd_add(usbhc, hcd)
#define usbhc_hcd_remove(usbhc, hcd) (usbhc)->hcd_remove(usbhc, hcd)
};

/* uhc */
struct hcd *usbh_hcd_match_by_virtual_portid(unsigned char virtual_portid);
unsigned char usbh_virtiual_portid_switch_to_portnum(struct hcd *hcd,
	unsigned char virtual_portid);
unsigned char usbh_portnum_switch_to_virtual_portid(struct hcd *hcd,
	unsigned char portnum);
int usb_hc_portx_died(struct usbh_bus *bus, int portnum, uint32_t lastword);
struct usb_hc *usb_hc_get_usbhc_object(void);
static inline int usb_hc_check_core_is_enable(void)
{
	struct usb_hc *usbhc = usb_hc_get_usbhc_object();
	return usb_hc_status_check_init(usbhc);
}

static inline int usb_hc_check_hcd_list_is_empty(void)
{
	struct usb_hc *usbhc = usb_hc_get_usbhc_object();
	return list_empty(&usbhc->hcd_list);
}

/* hal */
int hal_uhc_initial(struct usb_hc *usbhc);
#define hal_uhc_initial_with_usbhc_object() hal_uhc_initial(usb_hc_get_usbhc_object())

int hal_uhc_deinitial(struct usb_hc *usbhc);
#define hal_uhc_deinitial_with_usbhc_object() hal_uhc_deinitial(usb_hc_get_usbhc_object())

int hal_uhc_hcd_init(struct usb_hc *usbhc, int usbhwc_num, unsigned int hci_support);
#define hal_uhc_hcd_init_with_usbhc_object(usbhwc_num, hci_support) \
	hal_uhc_hcd_init(usb_hc_get_usbhc_object(), usbhwc_num, hci_support)

int hal_uhc_hcd_deinit(struct usb_hc *usbhc, int usbhwc_num, unsigned int hci_support);
#define hal_uhc_hcd_deinit_with_usbhc_object(usbhwc_num, hci_support) \
	hal_uhc_hcd_deinit(usb_hc_get_usbhc_object(), usbhwc_num, hci_support)

int usbh_hcd_rh_thread_wakeup(struct hcd *hcd, unsigned char port);

void usbh_setup_packet_dump(struct usb_setup_packet *setup);

#if defined(CONFIG_ARCH_SUNXI_HOST_OHCI)
int sunxi_ohci_hcd_init(struct usb_hc *usbhc, int usbhwc_num);
int sunxi_ohci_hcd_deinit(struct usb_hc *usbhc, int usbhwc_num);
#else
static inline int sunxi_ohci_hcd_init(struct usb_hc *usbhc, int usbhwc_num)
{
	return -1;
}
static inline int sunxi_ohci_hcd_deinit(struct usb_hc *usbhc, int usbhwc_num)
{
	return -1;
}
#endif

#if defined(CONFIG_ARCH_SUNXI_HOST_EHCI)
int sunxi_ehci_hcd_init(struct usb_hc *usbhc, int usbhwc_num);
int sunxi_ehci_hcd_deinit(struct usb_hc *usbhc, int usbhwc_num);
#else
static inline int sunxi_ehci_hcd_init(struct usb_hc *usbhc, int usbhwc_num)
{
	return -1;
}
static inline int sunxi_ehci_hcd_deinit(struct usb_hc *usbhc, int usbhwc_num)
{
	return -1;
}
#endif

/* pm */
#if defined(CONFIG_CHERRYUSB_COMMON_FRAMEWORK_PM)
int usb_hc_power_manager_initial(struct usb_hc *usbhc);
int usb_hc_power_manager_deinit(struct usb_hc *usbhc);
#else
static inline int usb_hc_power_manager_initial(struct usb_hc *usbhc)
{
	return 0;
}
static inline int usb_hc_power_manager_deinit(struct usb_hc *usbhc)
{
	return 0;
}
#endif
#define usb_hc_pm_initial_with_usbhc_object() \
	usb_hc_power_manager_initial(usb_hc_get_usbhc_object())
#define usb_hc_pm_deinit_with_usbhc_object() \
	usb_hc_power_manager_deinit(usb_hc_get_usbhc_object())

/* work */
/* offset */
#define USBHC_WORK_CMD_OFFSET (16)
#define USBHC_WORK_IDX_OFFSET (8)
#define USBHC_WORK_VAL_OFFSET (0)
/* mask */
#define USBHC_WORK_CMD_MASK (0xffff)
#define USBHC_WORK_IDX_MASK (0xff)
#define USBHC_WORK_VAL_MASK (0xff)
/* get */
#define USBHC_WORK_GET_CMD(work) (((work) >> USBHC_WORK_CMD_OFFSET) & USBHC_WORK_CMD_MASK)
#define USBHC_WORK_GET_IDX(work) (((work) >> USBHC_WORK_IDX_OFFSET) & USBHC_WORK_IDX_MASK)
#define USBHC_WORK_GET_VAL(work) (((work) >> USBHC_WORK_VAL_OFFSET) & USBHC_WORK_VAL_MASK)
/* construct */
#define USBHC_WORK_CONSTRUCT(cmd, idx, val) \
	((uint32_t)(((cmd & USBHC_WORK_CMD_MASK) << USBHC_WORK_CMD_OFFSET) \
			  | ((idx & USBHC_WORK_IDX_MASK) << USBHC_WORK_IDX_OFFSET) \
			  | ((val & USBHC_WORK_VAL_MASK) << USBHC_WORK_VAL_OFFSET)))

/* workcmd */
#define USBHC_WORKCMD_PORTX_RELAUNCH ((uint16_t)'w' + 0)	/* relaunch the port */

static inline int usb_hc_worker_queue_work(struct usb_hc *usbhc, unsigned int work)
{
	return usb_osal_mq_send(usbhc->work_mq, (uintptr_t)work);
}

int usb_hc_worker_initial(struct usb_hc *usbhc);
int usb_hc_worker_deinit(struct usb_hc *usbhc);

#define usb_hc_worker_initial_with_usbhc_object() \
	usb_hc_worker_initial(usb_hc_get_usbhc_object())
#define usb_hc_worker_deinit_with_usbhc_object() \
	usb_hc_worker_deinit(usb_hc_get_usbhc_object())
#define usb_hc_worker_queue_work_with_usbhc_object(work) \
	usb_hc_worker_queue_work(usb_hc_get_usbhc_object(), work)

#endif
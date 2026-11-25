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
#ifndef __UDC_INNER_H__
#define __UDC_INNER_H__

#include "usb_os_platform.h"
#include "usb_log.h"
#include "stdio.h"

/* debug */
#define udc_debug   USB_LOG_DBG
#define udc_info    USB_LOG_INFO
#define udc_warn    USB_LOG_WRN
#define udc_err     USB_LOG_ERR
#define udc_printf  printf

/**
 * board level
 */
typedef void *udc_board_t;

/* board support init */
udc_board_t udc_board_init(void);

/**
 * platform level
 */
typedef void *udc_platform_t;

/* platform support init */
udc_platform_t udc_platform_init(void);

/* platform support deinit */
int udc_platform_deinit(udc_platform_t udc_platform);

/* get platform usb otg interrupt id */
int udc_platform_get_interrupt_id(udc_platform_t udc_platform);

/* driver level */
int udc_driver_check_is_already_startup(void);

/* pm */
#if defined(CONFIG_CHERRYUSB_COMMON_FRAMEWORK_PM)
int usb_dc_power_manager_initial(void);
int usb_dc_power_manager_deinit(void);
#else
static inline int usb_dc_power_manager_initial(void)
{
    return 0;
}

static inline int usb_dc_power_manager_deinit(void)
{
    return 0;
}
#endif
#endif
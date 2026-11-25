/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * UDC log buffer system - transparent buffer logging for IRQ path.
 *
 * Enable:  define USB_LOG_DUMP_ENABLE (or add -DUDC_LOG_BUFFER_ENABLED)
 * Disable: leave USB_LOG_DUMP_ENABLE undefined (default)
 *
 * When enabled, udc_debug/info/warn/err are redirected to ring buffer
 * (zero IO overhead in IRQ), dump via MSH command "udc_log_cmd".
 *
 * When disabled, udc_debug/info/warn/err fall through to original
 * USB_LOG_DBG/INFO/WRN/ERR (direct serial output).
 *
 * In both cases, driver code always uses udc_debug/info/warn/err —
 * no udc_log_xxx API exposed to drivers.
 */

#ifndef UDC_LOG_H
#define UDC_LOG_H

#include "udc_sunxi.h"
#include "udc_inner.h"

#ifdef USB_LOG_DUMP_ENABLE

#include "usb_irq_log.h"

/* Declare extern prototype; definition is in udc_sunxi.c */
USB_IRQ_LOG_DECLARE(udc)

/* Redirect original macros to buffer */
#undef udc_debug
#undef udc_info
#undef udc_warn
#undef udc_err

#define udc_debug(...)  udc_log_write(0, __VA_ARGS__)
#define udc_info(...)   udc_log_write(1, __VA_ARGS__)
#define udc_warn(...)   udc_log_write(2, __VA_ARGS__)
#define udc_err(...)    udc_log_write(3, __VA_ARGS__)

#endif /* USB_LOG_DUMP_ENABLE */

/* UDC-specific helper (always available) */
static inline const char *ep0_state_name(uint8_t state)
{
	switch (state) {
	case USB_EP0_STATE_SETUP:      return "SETUP";
	case USB_EP0_STATE_IN_DATA:    return "IN_DATA";
	case USB_EP0_STATE_OUT_DATA:   return "OUT_DATA";
	case USB_EP0_STATE_IN_STATUS:  return "IN_STATUS";
	case USB_EP0_STATE_OUT_STATUS: return "OUT_STATUS";
	case USB_EP0_STATE_IN_ZLP:     return "IN_ZLP";
	case USB_EP0_STATE_STALL:      return "STALL";
	default:                       return "UNKNOWN";
	}
}

#endif /* UDC_LOG_H */

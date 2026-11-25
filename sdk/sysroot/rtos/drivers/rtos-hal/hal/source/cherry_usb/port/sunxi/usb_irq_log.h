/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Generic IRQ log buffer system - low-overhead ring buffer logging for
 * interrupt context where direct printk/printf would affect timing.
 *
 * Usage:
 *
 * 1. In ONE .c file (e.g. udc_sunxi.c), define the instance:
 *      #include "usb_irq_log.h"
 *      USB_IRQ_LOG_DEFINE(udc, 128, 96);
 *
 * 2. In a header (e.g. udc_log.h), declare the shared prototype:
 *      #include "usb_irq_log.h"
 *      USB_IRQ_LOG_DECLARE(udc);
 *
 * 3. Any .c file that includes the header can call:
 *      udc_log_write(0, "SETUP %02x %02x", bmRequestType, bRequest);
 *
 * 4. Dump to console via shell command (only in DEFINE translation unit):
 *      msh /> udc_log_cmd
 *
 * Another module instance:
 *      USB_IRQ_LOG_DEFINE(hcd, 256, 80);   // in hcd_xxx.c
 *      USB_IRQ_LOG_DECLARE(hcd);           // in hcd_log.h
 *      hcd_log_write(1, "port %d connected", port);
 *      msh /> hcd_log_cmd
 *
 * Platform: selected by CONFIG_OS_MELIS / CONFIG_KERNEL_FREERTOS
 *
 * Level: 0=DBG 1=INF 2=WRN 3=ERR
 */

#ifndef USB_IRQ_LOG_H
#define USB_IRQ_LOG_H

#include <stdarg.h>
#include <string.h>

/* ==================== Platform adaptation ==================== */

#if defined(CONFIG_OS_MELIS)

#include <hal_time.h>
#include <rtthread.h>
#include <hal_cmd.h>
#define USB_LOG_GET_TIME_NS()  hal_gettime_ns()
#define USB_LOG_PRINTF         rt_kprintf
#define USB_LOG_CMD_EXPORT     FINSH_FUNCTION_EXPORT_CMD

#elif defined(CONFIG_KERNEL_FREERTOS)

#include <hal_time.h>
#include <hal_cmd.h>

#define USB_LOG_GET_TIME_NS()  hal_gettime_ns()
#define USB_LOG_PRINTF         printf
#define USB_LOG_CMD_EXPORT     FINSH_FUNCTION_EXPORT_CMD

#else
#error "usb_irq_log.h: unsupported platform, define CONFIG_OS_MELIS or CONFIG_KERNEL_FREERTOS"
#endif

/* ==================== End of platform adaptation ==================== */

/* token pasting helpers */
#define _USB_LOG_CAT2(a, b)  a##b
#define _USB_LOG_CAT(a, b)   _USB_LOG_CAT2(a, b)
#define _USB_LOG_STR2(s)     #s
#define _USB_LOG_STR(s)      _USB_LOG_STR2(s)
#define _USB_LOG_CMD_EXPORT(func, cmd, desc)  USB_LOG_CMD_EXPORT(func, cmd, desc)

/*
 * USB_IRQ_LOG_DECLARE(name)
 *
 * Declares extern function prototype for use in header files.
 * Multiple translation units can include this without conflict.
 * Pair with USB_IRQ_LOG_DEFINE() in exactly ONE .c file.
 */
#define USB_IRQ_LOG_DECLARE(name) \
void _USB_LOG_CAT(name, _log_write)(uint8_t level, const char *fmt, ...);

/*
 * USB_IRQ_LOG_DEFINE(name, buf_size, msg_size)
 *
 * Defines a complete log instance. Use in exactly ONE .c file.
 * Produces:
 *   - static struct <name>_log_entry and <name>_log_buf (internal)
 *   - <name>_log_write()  (extern, callable from other .c files)
 *   - <name>_log_cmd shell command (for console dump)
 */
#define USB_IRQ_LOG_DEFINE(name, buf_size, msg_size) \
\
struct _USB_LOG_CAT(name, _log_entry) { \
	uint64_t ts_ns; \
	uint8_t  level; \
	char     msg[msg_size]; \
}; \
\
static struct _USB_LOG_CAT(name, _log_entry) _USB_LOG_CAT(name, _log_buf)[buf_size]; \
static volatile uint32_t _USB_LOG_CAT(name, _log_head) = 0; \
\
void _USB_LOG_CAT(name, _log_write)(uint8_t level, const char *fmt, ...) \
{ \
	uint32_t idx; \
	va_list args; \
\
	if (_USB_LOG_CAT(name, _log_head) == 0xFFFFFFFF) \
		return; \
\
	idx = _USB_LOG_CAT(name, _log_head) % buf_size; \
	_USB_LOG_CAT(name, _log_buf)[idx].ts_ns = USB_LOG_GET_TIME_NS(); \
	_USB_LOG_CAT(name, _log_buf)[idx].level = level; \
	va_start(args, fmt); \
	vsnprintf(_USB_LOG_CAT(name, _log_buf)[idx].msg, msg_size, fmt, args); \
	va_end(args); \
	_USB_LOG_CAT(name, _log_head)++; \
} \
\
static int _USB_LOG_CAT(name, _log_cmd)(int argc, char **argv) \
{ \
	uint32_t count, start, i; \
	static const char * const lv[] = {"DBG", "INF", "WRN", "ERR"}; \
\
	count = (_USB_LOG_CAT(name, _log_head) > buf_size) ? buf_size \
		: _USB_LOG_CAT(name, _log_head); \
	start = _USB_LOG_CAT(name, _log_head) - count; \
\
	USB_LOG_PRINTF("=== %s log (total %lu, showing %lu) ===\n", \
		_USB_LOG_STR(name), (unsigned long)_USB_LOG_CAT(name, _log_head), \
		(unsigned long)count); \
\
	for (i = 0; i < count; i++) { \
		uint32_t idx = (start + i) % buf_size; \
		USB_LOG_PRINTF("[%9llu us][%s] %s\n", \
			_USB_LOG_CAT(name, _log_buf)[idx].ts_ns / 1000, \
			lv[_USB_LOG_CAT(name, _log_buf)[idx].level], \
			_USB_LOG_CAT(name, _log_buf)[idx].msg); \
	} \
\
	USB_LOG_PRINTF("=== %s log end ===\n", _USB_LOG_STR(name)); \
	return 0; \
} \
_USB_LOG_CMD_EXPORT(_USB_LOG_CAT(name, _log_cmd), _USB_LOG_CAT(name, _log_cmd),\
	dump name log buffer);

#endif /* USB_IRQ_LOG_H */

/*
 * Allwinner SoCs ksc driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _KSC_SYS_INTF_H_
#define _KSC_SYS_INTF_H_

#include "ksc.h"
#include "soc/soc_base_data.h"

#define RTOS_SYS_CONFIG_USE 1

#define ksc_read_poll_timeout(op, val, cond, sleep_us, timeout_us, \
				sleep_before_read, args) \
({ \
	uint64_t __timeout_us = (timeout_us); \
	uint64_t __sleep_us = (sleep_us); \
	uint64_t timestamp_us = hal_get_timestamp(); \
	uint64_t __timeout = timestamp_us + timeout_us; \
	if (sleep_before_read && __sleep_us) \
		hal_usleep(__sleep_us); \
	for (;;) { \
		(val) = op(args); \
		if (cond) \
			break; \
		if (hal_get_timestamp() > __timeout) { \
			break; \
		} \
		if (__sleep_us) \
			hal_usleep(__sleep_us); \
	} \
	(cond) ? 0 : -ETIMEDOUT; \
})

enum proerty_type {
	PROPERTY_UNDEFINED = 0,
	PROPERTY_INTGER,
	PROPERTY_STRING,
	/*for the pins which function is GPIO(IN or OUT)*/
	PROPERTY_GPIO,
	/*for the pins which function is not GPIO*/
	PROPERTY_PIN,
	/*for axp power*/
	PROPERTY_POWER,
};

/**
 * pro
 */
struct property_t {
	char name[40];
	enum proerty_type type;
	union value_t {
		u32 value;
		char str[40];
	} v;
};

#define KSC_BYTE_ALIGN(x) (((x + (4*1024-1)) >> 12) << 12)

uintptr_t ksc_getprop_regbase(char *main_name, u32 index);

u32 ksc_getprop_irq(char *main_name, u32 index);

u32 ksc_getprop_clk_id(char *main_name);

u32 ksc_getprop_rst_id(char *main_name);

u32 ksc_getprop_clk_rate(char *main_name);

s32 ksc_sys_clk_set_rate(hal_clk_id_t clk_id, u32 rate);

u32 ksc_sys_clk_get_rate(hal_clk_id_t clk_id);

s32 ksc_sys_clk_enable(hal_clk_id_t clk_id);

s32 ksc_sys_clk_disable(hal_clk_id_t clk_id);

s32 ksc_sys_rst_deassert(hal_reset_id_t rst_id);

s32 ksc_sys_rst_assert(hal_reset_id_t rst_id);

s32 ksc_sys_rst_get_status(hal_reset_id_t rst_id);

bool ksc_clock_is_enabled(hal_clk_id_t clk_id);

s32 ksc_sys_clk_set_parent(hal_clk_id_t clk, hal_clk_id_t parent);

hal_clk_id_t ksc_sys_clk_get_parent(hal_clk_id_t clk);

int ksc_sys_register_irq(u32 IrqNo, u32 Flags, void *Handler, void *pArg,
			  u32 DataSize, u32 Prio);

void ksc_sys_unregister_irq(u32 IrqNo, void *Handler, void *pArg);

void ksc_sys_disable_irq(u32 IrqNo);

void ksc_sys_enable_irq(u32 IrqNo);

/* returns: 0:invalid, 1: int; 2:str, 3: gpio */
s32 ksc_sys_script_get_item(char *main_name, char *sub_name, s32 value[],
			     s32 type);

s32 ksc_delay_us(u32 us);

s32 ksc_delay_ms(u32 ms);

u32 ksc_getprop_irq(char *main_name, u32 index);

void *ksc_sys_malloc(u32 size);
void ksc_sys_free(void *ptr);
int ksc_sys_mutex_init(hal_sem_t *lock);
int ksc_sys_mutex_unlock(hal_sem_t *sem);
int ksc_sys_mutex_lock(hal_sem_t *sem);

#endif


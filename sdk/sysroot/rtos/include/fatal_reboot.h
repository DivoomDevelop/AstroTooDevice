/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Fatal-error reboot helper.
 *
 * A fatal error (configASSERT, hal_assert, BUG(), malloc-failed hook,
 * stack-overflow hook, unhandled CPU exception) must not leave a product
 * spinning forever in a "print then while(1)" loop.  When
 * CONFIG_FATAL_REBOOT is enabled the fatal paths call fatal_reboot_now(),
 * which flushes the log and then resets the SoC through the hardware
 * watchdog.
 *
 * The function never returns.  When CONFIG_FATAL_REBOOT is disabled it is
 * not compiled at all, so callers must keep their original halt loop.
 */

#ifndef __FATAL_REBOOT_H__
#define __FATAL_REBOOT_H__

/*
 * tag: short reason string, printed and used for the boot-reason record.
 * Never returns when CONFIG_FATAL_REBOOT is enabled.
 */
/* Disable interrupts and arm reset before potentially blocking diagnostics. */
void fatal_reboot_arm(void);
void fatal_reboot_now(const char *tag);
/* Supply the software failure site when it is available. */
void fatal_reboot_at(const char *tag, const char *file, unsigned int line);

#endif /* __FATAL_REBOOT_H__ */

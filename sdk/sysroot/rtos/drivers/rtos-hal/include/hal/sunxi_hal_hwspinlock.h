#ifndef HAL_HWSPINLOCK_H
#define HAL_HWSPINLOCK_H

#include <sunxi_hal_common.h>
#include <hal_hwspinlock.h>
#include <hal_clk.h>
#include <hal_reset.h>
#include <hal_log.h>
#include <interrupt.h>

#include <hwspinlock/hwspinlock.h>
/* Possible values of SPINLOCK_LOCK_REG */
#define SPINLOCK_NOTTAKEN               (0)     /* free */
#define SPINLOCK_TAKEN                  (1)     /* locked */
#define SPINLOCK_NUM                    (32)    /* max lock num */
#define SPINLOCK_MAX_UDELAY             (200)
#define SPINLOCK_IRQ_ALL_ENABLE         (0xffffffff)

#endif

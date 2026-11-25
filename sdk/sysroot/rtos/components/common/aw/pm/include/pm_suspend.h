
#ifndef _PM_SUSPEND_H_
#define _PM_SUSPEND_H_

#ifdef CONFIG_KERNEL_FREERTOS
#include <FreeRTOS.h>
#else
#error "PM do not support the RTOS!!"
#endif

#include <task.h>
#include <queue.h>

#include "pm_base.h"
#include "pm_state.h"

#include <hal_thread.h>
#include <hal_queue.h>

struct pm_suspend_thread_t {
	hal_thread_t thread;
	hal_queue_t queue;
};

int pm_suspend_init(void);
int pm_suspend_exit(void);
void pm_suspend_mode_change(suspend_mode_t mode);
void pm_set_standby_mode(pm_standby_mode_t mode);
void pm_record_stage(uint32_t val, uint8_t cover);
suspend_mode_t pm_get_suspend_mode(void);
pm_standby_mode_t pm_get_standby_mode(void);
int pm_suspend_request(suspend_mode_t mode);

#endif


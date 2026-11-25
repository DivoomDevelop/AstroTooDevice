#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

#include <backtrace.h>
#ifdef CONFIG_FATAL_REBOOT
#include <fatal_reboot.h>
#endif

int cur_cpu_id(void);
#ifndef CONFIG_FATAL_REBOOT
extern int cmd_reboot(int argc, char **argv);
#endif

void vAssertCalled(const char *pcFile, unsigned long ulLine)
{
#ifdef CONFIG_FATAL_REBOOT
    /* Arm the watchdog before diagnostics: printf or unwinding can hang. */
    fatal_reboot_at("configASSERT", pcFile, ulLine);
#else
    TaskHandle_t task_handle = xTaskGetCurrentTaskHandle();

    printf("cpu%d: pcFile %s, uLine %lu, task handle %p, task name %s.\n",
        cur_cpu_id(), pcFile, ulLine, task_handle, pcTaskGetName(task_handle));
#ifdef CONFIG_DEBUG_BACKTRACE
    backtrace(NULL, NULL, 0, 0, printf);
#endif
    /* 断言信息先送出串口，再由硬件看门狗重启，避免设备永久停在死循环。 */
    printf("assert: rebooting\n");
    fflush(stdout);
    cmd_reboot(1, NULL);
#endif
    while (1)
    {
    }
}

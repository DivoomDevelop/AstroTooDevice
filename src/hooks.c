#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <divoom_crash_log.h>
#ifdef CONFIG_FATAL_REBOOT
#include <fatal_reboot.h>
#endif

void vApplicationMallocFailedHook(void)
{
    printf("Malloc failed\n");
#ifdef CONFIG_FATAL_REBOOT
    fatal_reboot_now("malloc failed");
#endif
    for (;;);
}

void vApplicationStackOverflowHook(xTaskHandle pxTask, char *pcTaskName)
{
#ifdef CONFIG_DIVOOM_CRASH_LOG
    (void) pcTaskName; /* May point at a damaged TCB; snapshot validates it. */
    divoom_crash_log_overflow_task(pxTask);
    /* Enter the normal trap capture path. These are detection-site registers. */
    __asm__ volatile("ebreak" ::: "memory");
#ifdef CONFIG_FATAL_REBOOT
    fatal_reboot_now("stack overflow");
#endif
    for (;;);
#else
    (void) pcTaskName;
    (void) pxTask;

    printf("Stack overflow in task (handle %p, name '%s')\n", pxTask, pcTaskName);
#ifdef CONFIG_FATAL_REBOOT
    fatal_reboot_now("stack overflow");
#endif
    for (;;);
#endif
}

void vApplicationIdleHook(void)
{
    __asm__ volatile("wfi":::"memory", "cc");
}

void vApplicationTickHook(void)
{
}

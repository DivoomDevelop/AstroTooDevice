#ifndef __PM_PLAT__
#define __PM_PLAT__

#include <pm_base.h>

#define PM_PLAT_INF_EN      (0)
#define PM_PLAT_LOG_EN      (1)
#define PM_PLAT_ERR_EN      (1)
#if PM_PLAT_INF_EN
#define plat_inf(fmt,...)   printf("[PLAT_INF]" fmt,##__VA_ARGS__)
#else
#define plat_inf(fmt,...)
#endif
#if PM_PLAT_LOG_EN
#define plat_log(fmt,...)   printf("[PLAT_LOG]" fmt,##__VA_ARGS__)
#else
#define plat_log(fmt,...)
#endif
#if PM_PLAT_ERR_EN
#define plat_err(fmt,...)   printf("[PLAT_ERR]" fmt,##__VA_ARGS__)
#else
#define plat_err(fmt,...)
#endif

int pm_plat_platops_init(void);
int pm_plat_platops_deinit(void);
void pm_systeminit(void);
int pm_trigger_suspend(suspend_mode_t mode);
void plat_suspend(void);
void plat_resume(void);

#endif

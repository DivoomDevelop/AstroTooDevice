
#ifndef _PM_WAKECNT_H_
#define _PM_WAKECNT_H_
#if defined(CONFIG_ARCH_SUN20IW2) || defined(CONFIG_ARCH_SUN252IW2)
#include "pm_wakesrc.h"

/* int pm_wakecnt_init(void); */
int pm_wakecnt_exit(void);
uint32_t pm_wakecnt_stash(void);
uint32_t pm_wakecnt_subcnt_update(void);

int pm_wakecnt_check(void);
int pm_wakecnt_has_changed(void);
int pm_wakecnt_in_progress(void);

void pm_wakecnt_inc(int irq);
void pm_stay_awake(int irq);
void pm_relax(int irq, pm_relax_type_t wakeup);

int pm_wakecnt_subcnt_has_changed(void);
void pm_wakecnt_subcnt_inc(int irq);
#endif

#endif

#ifndef __OS_STBY_H__
#define __OS_STBY_H__

#if ((defined CONFIG_ARCH_SUN300IW1) || (defined CONFIG_DRIVER_V821)) && (defined CONFIG_PM_STANDBY_MEMORY)
#include "pm_mem.h"
#define xr_unsaved_bss                 __standby_unsaved_bss
#define xr_unsaved_data                __standby_unsaved_data
#else
#define xr_unsaved_bss
#define xr_unsaved_data
#endif
#endif

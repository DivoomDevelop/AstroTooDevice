#ifndef DIVOOM_FINSH_AUTOTEST_H
#define DIVOOM_FINSH_AUTOTEST_H

#include "divoom_build_config.h"

#if DIVOOM_BUILD_SIMULATOR
#error "Finsh autotest CLI is RTOS device-only; exclude from simulator VS project."
#endif

/* DIVOOM_FINSH_AUTOTEST from divoom_build_config.h ← CONFIG_DIVOOM_FINSH_AUTOTEST (menuconfig) */

#endif /* DIVOOM_FINSH_AUTOTEST_H */

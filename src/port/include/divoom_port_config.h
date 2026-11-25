#ifndef DIVOOM_PORT_CONFIG_H
#define DIVOOM_PORT_CONFIG_H

#include "divoom_build_config.h"

/*
 * Port-layer capability flags (phase 1: OS + FS only).
 * Business code should prefer these over DIVOOM_SIMULATOR where applicable.
 */
#define DIVOOM_PORT_HAS_STATVFS  DIVOOM_BUILD_RTOS_DEVICE

#endif /* DIVOOM_PORT_CONFIG_H */

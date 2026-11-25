#include "divoom_build_config.h"
#if !DIVOOM_BUILD_RTOS_DEVICE
#error "RTOS device-only stub; exclude from simulator VS project."
#endif

#if defined(CONFIG_DIVOOM_SQLITE) && CONFIG_DIVOOM_SQLITE

#define DIVOOM_DATABASE_MANAGE_INCLUDED_FROM_RTOS_STUB 1
#include "divoom_database_manage.c"

#else /* !CONFIG_DIVOOM_SQLITE */

#include "divoom.h"
#include <string.h>
#include "divoom_database_disabled.inc"

#endif /* CONFIG_DIVOOM_SQLITE */

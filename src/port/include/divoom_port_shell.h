#ifndef DIVOOM_PORT_SHELL_H
#define DIVOOM_PORT_SHELL_H

#include "divoom_build_config.h"

#if DIVOOM_BUILD_RTOS_DEVICE
#include <console.h>
#define DIVOOM_PORT_SHELL_CMD(name, cmd, desc) \
	FINSH_FUNCTION_EXPORT_CMD(name, cmd, desc)
#define DIVOOM_PORT_SHELL_CMD_ALIAS(name, alias, desc) \
	FINSH_FUNCTION_EXPORT_ALIAS(name, alias, desc)
#else
#define DIVOOM_PORT_SHELL_CMD(name, cmd, desc)
#define DIVOOM_PORT_SHELL_CMD_ALIAS(name, alias, desc)
#endif

#endif /* DIVOOM_PORT_SHELL_H */

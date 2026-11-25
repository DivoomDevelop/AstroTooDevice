#ifndef DIVOOM_PORT_TIME_H
#define DIVOOM_PORT_TIME_H

#include "divoom_build_config.h"
#include <time.h>

/*
 * Portable time parsing for RTOS newlib (no libc strptime).
 * divoom_port_strptime follows POSIX strptime semantics on supported specifiers.
 */
char *divoom_port_strptime(const char *s, const char *fmt, struct tm *tm);

#if DIVOOM_BUILD_RTOS_DEVICE
char *strptime(const char *s, const char *fmt, struct tm *tm);
#endif

#endif /* DIVOOM_PORT_TIME_H */

#ifndef _BT_LOG_H_STUB_
#define _BT_LOG_H_STUB_

#include "divoom_print.h"

#ifndef divoom_debug
#define divoom_debug(fmt, ...) divoom_print(fmt, ##__VA_ARGS__)
#endif

#endif

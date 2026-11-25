#ifndef __HID_DEBUG_H__
#define __HID_DEBUG_H__

#include "stdio.h"
#include "stdarg.h"

#define HID_DEBUG (1)

#define hid_err(f, ...)     printf("{err  } %s %d: " f, __func__, __LINE__, ##__VA_ARGS__)
#define hid_warn(f, ...)    printf("{warn } %s %d: " f, __func__, __LINE__, ##__VA_ARGS__)
#define hid_info(f, ...)    printf("{info } %s %d: " f, __func__, __LINE__, ##__VA_ARGS__)
#if HID_DEBUG
#define hid_debug(f, ...)   printf("{debug} %s %d: " f, __func__, __LINE__, ##__VA_ARGS__)
#else
#define hid_debug(f, ...)
#endif

#define hidf_err(hidf, f, ...) \
    printf("{err  } %s %d hid-f%d: " f, __func__, __LINE__, (hidf)->id, ##__VA_ARGS__)
#define hidf_warn(hidf, f, ...) \
    printf("{warn } %s %d hid-f%d: " f, __func__, __LINE__, (hidf)->id, ##__VA_ARGS__)
#define hidf_info(hidf, f, ...) \
    printf("{info } %s %d hid-f%d: " f, __func__, __LINE__, (hidf)->id, ##__VA_ARGS__)
#if HID_DEBUG
#define hidf_debug(hidf, f, ...) \
    printf("{debug} %s %d hid-f%d: " f, __func__, __LINE__, (hidf)->id, ##__VA_ARGS__)
#else
#define hidf_debug(hidf, f, ...)
#endif

#endif
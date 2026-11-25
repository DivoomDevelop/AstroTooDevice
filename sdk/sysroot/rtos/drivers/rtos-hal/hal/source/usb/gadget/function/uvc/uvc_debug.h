#ifndef __UVC_DEBUG_H__
#define __UVC_DEBUG_H__

#include "stdio.h"
#include "stdarg.h"
#include "uvc_drv.h"

#define UVC_DEBUG (0)

#define uvc_err(f, ...)     printf("{err  } %s %d: " f, __func__, __LINE__, ##__VA_ARGS__)
#define uvc_warn(f, ...)    printf("{warn } %s %d: " f, __func__, __LINE__, ##__VA_ARGS__)
#define uvc_info(f, ...)    printf("{info } %s %d: " f, __func__, __LINE__, ##__VA_ARGS__)
#if UVC_DEBUG
#define uvc_debug(f, ...)   printf("{debug} %s %d: " f, __func__, __LINE__, ##__VA_ARGS__)
#else
#define uvc_debug(f, ...)
#endif

#define vchn_err(video, f, ...) \
    printf("{err  } %s %d video-ch%d: " f, __func__, __LINE__, (video)->id, ##__VA_ARGS__)
#define vchn_warn(video, f, ...) \
    printf("{warn } %s %d video-ch%d: " f, __func__, __LINE__, (video)->id, ##__VA_ARGS__)
#define vchn_info(video, f, ...) \
    printf("{info } %s %d video-ch%d: " f, __func__, __LINE__, (video)->id, ##__VA_ARGS__)
#if UVC_DEBUG
#define vchn_debug(video, f, ...) \
    printf("{debug} %s %d video-ch%d: " f, __func__, __LINE__, (video)->id, ##__VA_ARGS__)
#else
#define vchn_debug(video, f, ...)
#endif

#endif

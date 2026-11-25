/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : CdxTime.h
 * Description : Time
 * History :
 *
 */

#ifndef CDX_TIME_H
#define CDX_TIME_H

#include <CdxTypes.h>
#include <time.h>
#ifdef CONFIG_KERNEL_FREERTOS
/* Provided by components/aw/newlibc/time.c; this newlib header hides the
 * POSIX prototype unless additional feature macros are enabled. */
CDX_API int clock_gettime(clockid_t clock_id, struct timespec *tp);
#endif

/* Wall Clock: it's not monotonic and may have discontinuities */
CDX_API cdx_int64 CdxGetNowUs(void);

/* Monotonic clock */
CDX_API cdx_int64 CdxMonoTimeUs(void);

#endif

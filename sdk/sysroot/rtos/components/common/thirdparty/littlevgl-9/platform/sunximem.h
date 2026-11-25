/**
 * @file sunximem.h
 *
 */

#ifndef SUNXIMEM_H
#define SUNXIMEM_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#ifndef USE_SUNXIMEM
#define USE_SUNXIMEM   0
#endif

#if USE_SUNXIMEM
#ifdef LV_USE_SUNXIFB_DEBUG_TIMER
#include <sys/time.h>
#endif
/*********************
 *      DEFINES
 *********************/
#ifdef CONFIG_SOC_SUN252IW2
#define SUNXIMEM_ALIGN(x)   ((((x) + (1 << 5) - 1) >> 5) << 5)
#else
#define SUNXIMEM_ALIGN(x)   ((((x) + (1 << 4) - 1) >> 4) << 4)
#endif
#define SUNXIMEM_ALIGN_32B(x)   ((((x) + (1 << 5) - 1) >> 5) << 5)
#define SUNXIMEM_ALIGN_16B(x)   ((((x) + (1 << 4) - 1) >> 4) << 4)
/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
int sunxifb_mem_init(void);
void sunxifb_mem_deinit(void);
void* sunxifb_mem_alloc(size_t size, char *label);
void sunxifb_mem_free(void **data, char *label);
int sunxifb_mem_get_fd(void *data);
void* sunxifb_mem_get_phyaddr(void *data);
void sunxifb_mem_flush_cache(void *data, size_t size);
void sunxifb_mem_invalidate_cache(void *data, size_t size);
#ifdef LV_USE_SUNXIFB_DEBUG_TIMER
void sunxifb_mem_start_timer(struct timeval* start);
long long sunxifb_mem_end_timer(const struct timeval* start, const char* desc);
#endif /* LV_USE_SUNXIFB_DEBUG_TIME */

/**********************
 *      MACROS
 **********************/

#endif  /*USE_SUNXIFB_G2D*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*SUNXIMEM_H*/

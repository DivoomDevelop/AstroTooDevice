/**
 * @file lv_os_melis.h
 *
 */

#ifndef LV_OS_MELIS_H
#define LV_OS_MELIS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#if LV_USE_OS == LV_OS_CUSTOM
#include <stdint.h>
#include <stdbool.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    uint32_t thread;
} lv_thread_t;

typedef struct {
    void *mutex;
} lv_mutex_t;

typedef struct {
    void *sem;
} lv_thread_sync_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_OS == LV_OS_CUSTOM*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_OS_MELIS_H*/

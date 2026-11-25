
#ifndef __IPCLINUX_LOG_H__
#define __IPCLINUX_LOG_H__

#include <stdio.h>

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#define OPTION_LOG_STYLE_PRINTF     0
#define OPTION_LOG_STYLE_GLOG       1

#ifdef USE_STD_LOG
#define CONFIG_LOG_STYLE OPTION_LOG_STYLE_PRINTF
#else
#define CONFIG_LOG_STYLE OPTION_LOG_STYLE_GLOG
#endif

// option for debug level.
#define OPTION_LOG_LEVEL_CLOSE      0
#define OPTION_LOG_LEVEL_ERROR      1
#define OPTION_LOG_LEVEL_WARN       2
#define OPTION_LOG_LEVEL_DEBUG      3
#define OPTION_LOG_LEVEL_VERBOSE    4

#define COLOR_LOG                   1
#define FORMAT_LOG                  1

// MPP_LOG_LEVEL takes priority over CONFIG_LOG_LEVEL to avoid conflict with system config
#ifdef MPP_LOG_LEVEL
  #undef CONFIG_LOG_LEVEL
  #define CONFIG_LOG_LEVEL MPP_LOG_LEVEL
#else
  #ifndef CONFIG_LOG_LEVEL
    #ifdef LOG_NDEBUG
      #if(LOG_NDEBUG == 0)
        #define CONFIG_LOG_LEVEL    OPTION_LOG_LEVEL_VERBOSE
      #endif
    #endif
  #endif

  #ifndef CONFIG_LOG_LEVEL
  #define CONFIG_LOG_LEVEL    OPTION_LOG_LEVEL_DEBUG
  #endif
#endif

#ifndef LOG_TAG
#define LOG_TAG ""
#endif

#define StripFileName(s) (strrchr(s, '/')?(strrchr(s, '/')+1):s)

extern int MPP_GLOBAL_LOG_LEVEL; //OPTION_LOG_LEVEL_DEBUG

#if (CONFIG_LOG_STYLE == OPTION_LOG_STYLE_GLOG)
    // #include <glogCWrapper/log.h>

    #define PLAT_LOG_LEVEL_INFO   0
    #define PLAT_LOG_LEVEL_WARN   1
    #define PLAT_LOG_LEVEL_ERROR  2
    #define PLAT_LOG_LEVEL_FATAL  3

#define PLATLOG(mppLogLevel, level, fmt, arg...)  \
        do { \
            if (mppLogLevel <= MPP_GLOBAL_LOG_LEVEL) \
            { \
                printf("%s: " fmt "\n", LOG_TAG, ##arg); \
            } \
        } while (0)

#if CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_CLOSE

#define aloge(fmt, arg...)
#define alogw(fmt, arg...)
#define alogd(fmt, arg...)
#define alogv(fmt, arg...)

#elif CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_ERROR

#define aloge(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_ERROR, PLAT_LOG_LEVEL_ERROR, fmt, ##arg)
#define alogw(fmt, arg...)
#define alogd(fmt, arg...)
#define alogv(fmt, arg...)

#elif CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_WARN

#define aloge(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_ERROR, PLAT_LOG_LEVEL_ERROR, fmt, ##arg)
#define alogw(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_WARN, PLAT_LOG_LEVEL_WARN, fmt, ##arg)
#define alogd(fmt, arg...)
#define alogv(fmt, arg...)

#elif CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_DEBUG

#define aloge(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_ERROR, PLAT_LOG_LEVEL_ERROR, fmt, ##arg)
#define alogw(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_WARN, PLAT_LOG_LEVEL_WARN, fmt, ##arg)
#define alogd(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_DEBUG, PLAT_LOG_LEVEL_INFO, fmt, ##arg)
#define alogv(fmt, arg...)

#elif CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_VERBOSE

#define aloge(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_ERROR, PLAT_LOG_LEVEL_ERROR, fmt, ##arg)
#define alogw(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_WARN, PLAT_LOG_LEVEL_WARN, fmt, ##arg)
#define alogd(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_DEBUG, PLAT_LOG_LEVEL_INFO, fmt, ##arg)
#define alogv(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_VERBOSE, PLAT_LOG_LEVEL_INFO, fmt, ##arg)

#endif

#elif (CONFIG_LOG_STYLE == OPTION_LOG_STYLE_PRINTF)
    #include <stdio.h>
    #include <string.h>
    #include <log/std_log.h>

    #define PLAT_LOG_LEVEL_ERROR     FMT_COLOR_BG(BLACK, RED) " E " FMT_DEFAULT
    #define PLAT_LOG_LEVEL_WARN      FMT_COLOR_BG(BLACK, YELLOW) " W " FMT_DEFAULT
    #define PLAT_LOG_LEVEL_DEBUG     FMT_COLOR_BG(BLACK, BLUE) " D " FMT_DEFAULT
    #define PLAT_LOG_LEVEL_VERBOSE   FMT_COLOR_BG(BLACK, WRITE) " V " FMT_DEFAULT

    #define PLATLOG(mppLogLevel, level, fmt, arg...) \
        do { \
            if(mppLogLevel <= MPP_GLOBAL_LOG_LEVEL) \
            { \
                printf("\t%s %s " fmt "\n", StripFileName(__FILE__), XPOSTO(50) level, __FUNCTION__, __LINE__, ##arg); \
            } \
        } while(0)

#if CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_CLOSE

#define aloge(fmt, arg...)
#define alogw(fmt, arg...)
#define alogd(fmt, arg...)
#define alogv(fmt, arg...)

#elif CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_ERROR

#define aloge(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_ERROR, PLAT_LOG_LEVEL_ERROR, FMT_COLOR_FG(LIGHT, RED) "<%s:%d> " XPOSTO(90) FMT_DEFAULT fmt, ##arg)
#define alogw(fmt, arg...)
#define alogd(fmt, arg...)
#define alogv(fmt, arg...)

#elif CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_WARN

#define aloge(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_ERROR, PLAT_LOG_LEVEL_ERROR, FMT_COLOR_FG(LIGHT, RED) "<%s:%d> " XPOSTO(90) FMT_DEFAULT fmt, ##arg)
#define alogw(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_WARN, PLAT_LOG_LEVEL_WARN, FMT_COLOR_FG(LIGHT, YELLOW) "<%s:%d> " XPOSTO(90) FMT_DEFAULT fmt, ##arg)
#define alogd(fmt, arg...)
#define alogv(fmt, arg...)

#elif CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_DEBUG

#define aloge(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_ERROR, PLAT_LOG_LEVEL_ERROR, FMT_COLOR_FG(LIGHT, RED) "<%s:%d> " XPOSTO(90) FMT_DEFAULT fmt, ##arg)
#define alogw(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_WARN, PLAT_LOG_LEVEL_WARN, FMT_COLOR_FG(LIGHT, YELLOW) "<%s:%d> " XPOSTO(90) FMT_DEFAULT fmt, ##arg)
#define alogd(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_DEBUG, PLAT_LOG_LEVEL_DEBUG, FMT_COLOR_FG(LIGHT, BLUE) "<%s:%d> " XPOSTO(90) FMT_DEFAULT fmt, ##arg)
#define alogv(fmt, arg...)

#elif CONFIG_LOG_LEVEL == OPTION_LOG_LEVEL_VERBOSE

#define aloge(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_ERROR, PLAT_LOG_LEVEL_ERROR, FMT_COLOR_FG(LIGHT, RED) "<%s:%d> " XPOSTO(90) FMT_DEFAULT fmt, ##arg)
#define alogw(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_WARN, PLAT_LOG_LEVEL_WARN, FMT_COLOR_FG(LIGHT, YELLOW) "<%s:%d> " XPOSTO(90) FMT_DEFAULT fmt, ##arg)
#define alogd(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_DEBUG, PLAT_LOG_LEVEL_DEBUG, FMT_COLOR_FG(LIGHT, BLUE) "<%s:%d> " XPOSTO(90) FMT_DEFAULT fmt, ##arg)
#define alogv(fmt, arg...) PLATLOG(OPTION_LOG_LEVEL_VERBOSE, PLAT_LOG_LEVEL_VERBOSE, FMT_COLOR_FG(LIGHT, WRITE) "<%s:%d> " XPOSTO(90) FMT_DEFAULT fmt, ##arg)

#endif

#endif

#ifdef __cplusplus
}
#endif

// 确保日志函数总是可用
#ifndef aloge
#define aloge(fmt, arg...) printf("[ERROR] %s: " fmt "\n", LOG_TAG, ##arg)
#endif

#ifndef alogw
#define alogw(fmt, arg...) printf("[WARN] %s: " fmt "\n", LOG_TAG, ##arg)
#endif

#ifndef alogd
#define alogd(fmt, arg...) printf("[DEBUG] %s: " fmt "\n", LOG_TAG, ##arg)
#endif

#ifndef alogv
#define alogv(fmt, arg...) printf("[VERBOSE] %s: " fmt "\n", LOG_TAG, ##arg)
#endif

#endif


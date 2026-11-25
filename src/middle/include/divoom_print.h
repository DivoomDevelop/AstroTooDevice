#ifndef _divoom_print_h_
#define _divoom_print_h_
//#define DIVOOM_TAG "divoom_debug"
#define DIVOOM_TAG __func__
void divoom_log_start(void);
void divoom_log_refresh(void);
void divoom_log_close(void);
unsigned char *divoom_log_get_buffer(int *buf_size);
void divoom_log_write(const char* tag, const char* fmt, ...);

#define DIVOOM_LOG_LEVEL_ERROR 0
#define DIVOOM_LOG_LEVEL_STATE 1
#define DIVOOM_LOG_LEVEL_DEBUG 2

#ifndef CONFIG_DIVOOM_LOG_LEVEL
#define CONFIG_DIVOOM_LOG_LEVEL DIVOOM_LOG_LEVEL_DEBUG
#endif

#define DIVOOM_LOG_STATE_IMPL(fmt,...) do { \
        char tag[128]; \
        sprintf(tag, "%u:%s--%d:  ", \
                (unsigned)divoom_system_get_tickms(), __func__, __LINE__); \
        printf("%s", tag); \
        printf(fmt, ##__VA_ARGS__); \
        divoom_log_write(tag, fmt, ##__VA_ARGS__); \
    } while (0)

#if CONFIG_DIVOOM_LOG_LEVEL >= DIVOOM_LOG_LEVEL_STATE
#define divoom_log_state(fmt,...) DIVOOM_LOG_STATE_IMPL(fmt, ##__VA_ARGS__)
#else
#define divoom_log_state(fmt,...) do { } while (0)
#endif

/* Existing business/state logs stay source compatible. */
#define divoom_print(fmt,...) divoom_log_state(fmt, ##__VA_ARGS__)

#define dm_print printf 

#if CONFIG_DIVOOM_LOG_LEVEL >= DIVOOM_LOG_LEVEL_DEBUG
#define divoom_debug(format,...) do { \
        printf("%s,LINE:%d:" format "\n", __func__, __LINE__, ##__VA_ARGS__); \
    } while (0)
#else
#define divoom_debug(format,...) do { } while (0)
#endif


#define divoom_print_err(fmt,...) do { \
        char tag[128]; \
        sprintf(tag, "%s--%d", __func__, __LINE__); \
        printf("%s", tag); \
        printf(fmt, ##__VA_ARGS__); \
    } while (0)


void divoom_print_buf(unsigned char *data, unsigned short len);

#endif

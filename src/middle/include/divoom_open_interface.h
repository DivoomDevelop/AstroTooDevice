#ifndef DIVOOM_OPEN_INTERFACE_H
#define DIVOOM_OPEN_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Returns a library-owned topic prefix; callers must not modify or free it. */
const char *divoom_open_get_topic(void);
    void divoom_open_update_dead_lock_info(char *info);

#ifdef __cplusplus
}
#endif

#endif

/**
 * @file prctl.h
 * @brief Placeholder header for process control operations
 *
 * This is a placeholder header for RTOS porting.
 * Real implementation should provide process control API.
 */

#ifndef _SYS_PRCTL_H
#define _SYS_PRCTL_H
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

// prctl option definitions (simplified for RTOS)
#define PR_SET_NAME    1
#define PR_GET_NAME    2
#define PR_SET_PDEATHSIG 3
#define PR_GET_PDEATHSIG 4
#define PR_SET_DUMPABLE 5
#define PR_GET_DUMPABLE 6
#define PR_SET_TIMING   7
#define PR_GET_TIMING   8

// Placeholder function for RTOS
static inline int prctl(int option, unsigned long arg2, unsigned long arg3,
                        unsigned long arg4, unsigned long arg5) {
    (void)option; (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    if (option != PR_SET_NAME) {
        printf("[%s] line:%d \n", __func__, __LINE__);
        return 0;
    }
    pthread_setname_np(pthread_self(), arg2);
    return 0; // Always succeed for RTOS
}

#ifdef __cplusplus
}
#endif

#endif /* _SYS_PRCTL_H */

#ifndef STORAGE_STRESS_TEST_H
#define STORAGE_STRESS_TEST_H

/* Set to 1 in a diagnostic build to start the destructive storage stress test. */
#ifndef DIVOOM_STORAGE_STRESS_TEST
#define DIVOOM_STORAGE_STRESS_TEST 0
#endif

#if DIVOOM_STORAGE_STRESS_TEST
#include <stdint.h>
#define STORAGE_STRESS_CYCLES 2000U
enum storage_stress_phase {
    STORAGE_STRESS_WAIT, STORAGE_STRESS_WRITE, STORAGE_STRESS_VERIFY,
    STORAGE_STRESS_DELETE, STORAGE_STRESS_CHECK, STORAGE_STRESS_DONE,
    STORAGE_STRESS_ERROR
};
typedef struct {
    uint32_t cycle, phase, files[2], done, total, free_kib;
    int error;
} storage_stress_status;
void storage_stress_test_start(void);
void storage_stress_get_status(storage_stress_status *status);
#endif

#endif

#ifndef DIVOOM_PRODUCT_TEST_USR_CHECK_H
#define DIVOOM_PRODUCT_TEST_USR_CHECK_H

typedef enum {
    DIVOOM_PT_USR_CHECK_IDLE,
    DIVOOM_PT_USR_CHECK_RUNNING,
    DIVOOM_PT_USR_CHECK_DONE,
    DIVOOM_PT_USR_CHECK_FAILED,
} divoom_product_test_usr_check_phase;

typedef struct {
    divoom_product_test_usr_check_phase phase;
    int done, total, error_files, error_code;
    int fs_check_target; /* 0 userdata, 1 system; stage zero means CRC scan. */
    unsigned fs_check_stage, fs_damage_flags;
    int maintenance_required; /* Online screening never authorizes disk writes. */
} divoom_product_test_usr_check_status;

/* Once per factory session. Caller reserves /usr against format and OTA first.
 * Read-only: never repair, download or change the local CRC/version files. */
void divoom_product_test_usr_check_start(void);
/* Settings can run a fresh scan after completion; never restarts a live worker. */
int divoom_product_test_usr_check_start_settings(void);
void divoom_product_test_usr_check_get_status(divoom_product_test_usr_check_status *status);

#endif

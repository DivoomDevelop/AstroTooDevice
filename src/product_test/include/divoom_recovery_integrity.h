#ifndef DIVOOM_RECOVERY_INTEGRITY_H
#define DIVOOM_RECOVERY_INTEGRITY_H
#include "divoom_product_test_format.h"
/* Snapshot belongs to the single maintenance worker; read it immediately after
 * an operation. Stage strings have static lifetime. target_usr=-1 is not a disk. */
typedef enum {
 DIVOOM_RECOVERY_FAILURE_NONE, DIVOOM_RECOVERY_FAILURE_VOLUME,
 DIVOOM_RECOVERY_FAILURE_RESOURCE, DIVOOM_RECOVERY_FAILURE_JOURNAL,
 DIVOOM_RECOVERY_FAILURE_FIRMWARE, DIVOOM_RECOVERY_FAILURE_STATE
} divoom_recovery_failure_kind;
typedef struct {
 divoom_recovery_failure_kind kind;
 int target_usr, result, error, raw_error; /* error supplies a fallback when errno was zero. */
 const char *stage;
} divoom_recovery_integrity_failure;
void divoom_recovery_integrity_get_failure(divoom_recovery_integrity_failure *out);
void divoom_recovery_integrity_clear_failure(void);
/* Both volumes, before persisting any repair intent. */
int divoom_recovery_integrity_preview(divoom_product_test_repair_plan *plan);
/* 1 both structures clean (continue CRC); 2 format userdata; 3 format usr;
 * 4 userdata-only recovery ready for configuration restore and completion;
 * -1 temporary/read-only retry; -2 completed format still unhealthy. */
int divoom_recovery_integrity_run(int manual_usr_format);
int divoom_recovery_integrity_before_format(int target_usr);
/* Enable writes only on a checked peer when a firmware backup must be made. */
int divoom_recovery_integrity_protect_wifi(int target_usr);
/* Caller has exhausted explicitly classified local I/O retries. */
int divoom_recovery_integrity_request_format(int target_usr);
int divoom_recovery_integrity_after_format(int target_usr);
int divoom_recovery_integrity_format_failed(int target_usr);
int divoom_recovery_integrity_complete(int target_usr);
#endif

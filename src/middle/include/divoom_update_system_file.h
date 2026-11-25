#ifndef _divoom_update_system_file_h_
#define _divoom_update_system_file_h_
#include <stdint.h>
void divoom_update_system_file_init(void);
int divoom_update_system_file_get_file_version(void);
/* Called only by SET_ALL_MODE before its request; returns 0 while an updater/OTA owns the round. */
int divoom_update_system_file_begin_settings_check(void);
/* Consume one response from that request; late or duplicate responses cannot reopen a round. */
void divoom_update_system_file_add_file(void *para);
int divoom_update_system_file_is_downloading(void);
/* Resource download/install page ownership; excludes firmware-only queues and cancellation. */
int divoom_update_system_file_is_resource_updating(void);
/* Cancel only this resource round; safe from synchronous schedule callbacks. */
void divoom_update_system_file_cancel_current(void);
int divoom_update_system_file_is_busy(void); /* Maintenance: includes read-only scans. */
/* Local read-only scan: reserve against resource writes using the updater lock. */
int divoom_update_system_file_begin_health_check(void);
void divoom_update_system_file_end_health_check(void);
/* Failed Settings scan: retain the gate across the asynchronous main handoff.
 * Only a finished reader is ready; active readers/installers cannot be claimed. */
int divoom_update_system_file_hold_health_check_for_repair(void);
int divoom_update_system_file_health_check_repair_ready(void);
void divoom_update_system_file_cancel_health_check_repair(void);
typedef enum {
    DIVOOM_SYSTEM_FILE_STAGE_IDLE = 0,
    DIVOOM_SYSTEM_FILE_STAGE_PREPARE,
    DIVOOM_SYSTEM_FILE_STAGE_DOWNLOAD,
    DIVOOM_SYSTEM_FILE_STAGE_INSTALL,
    DIVOOM_SYSTEM_FILE_STAGE_COMMIT,
    DIVOOM_SYSTEM_FILE_STAGE_DONE,
    DIVOOM_SYSTEM_FILE_STAGE_RETRY,
    DIVOOM_SYSTEM_FILE_STAGE_FAILED,
} divoom_system_file_stage;
/* Atomic worker snapshot; local installation/commit do not depend on network activity. */
divoom_system_file_stage divoom_update_system_file_get_progress(int *current, int *total);
void divoom_update_system_file_stop(void);
int divoom_update_system_file_start_pending(void);
int divoom_update_system_file_update_local_net_version(int version);
void divoom_update_system_file_save_sys_file_version(int version);
/* Real firmware upgrades cancel the resource round; no-update payloads wait without cancellation.
 * On 1 the updater owns firmware_info; on 0 the caller still owns it. */
int divoom_update_system_file_start_firmware(void *firmware_info, int ota_event);

/* 产测格式化只恢复资源：清单先落 /userdata，恢复不读取旧版本标记。 */
#define DIVOOM_SYSTEM_FORMAT_MANIFEST "/userdata/.format_file_crc.bin"
/* file_percent: 0..100 for the current file, -1 when no file is active. */
typedef void (*divoom_system_file_restore_progress)(int done, int total, int file_percent);
/* retry receives the current file's error count (0 at start/completion); 0 return cancels. */
/* Validate root, paths, nonempty resource list and reserved CRC metadata. */
int divoom_update_system_file_validate_format_manifest(const char *manifest);
int divoom_update_system_file_restore_path_is_reserved(const char *relative);
int divoom_update_system_file_prepare_format(int (*retry)(int errors));
int divoom_update_system_file_ensure_format_manifest(int (*retry)(int errors));
/* Missing local ledger only: recover the original BASE guide, never LAST or a version.
 * Existing ledgers are untouched; transport errors retry every 10s.
 * Returns -1 after 4 local I/O failures, 0 cancelled/invalid, 1 verified. */
int divoom_update_system_file_ensure_original_ledger(int (*retry)(int errors));
#define DIVOOM_SYSTEM_FILE_RESTORE_SKIPPED 2
#define DIVOOM_SYSTEM_FILE_RESTORE_NEEDS_FORMAT (-1)
/* Persistent staging-directory/file failure belongs to userdata, not usr. */
#define DIVOOM_SYSTEM_FILE_RESTORE_NEEDS_USERDATA_FORMAT (-3)
#define DIVOOM_SYSTEM_FILE_RESTORE_USERDATA_RETRY (-4)
/* Confirmed local preparation/final-validation I/O; caller owns retry budget. */
#define DIVOOM_SYSTEM_FILE_RESTORE_LOCAL_RETRY (-2)
/* Returns 1 installed/verified, 2 skipped HTTP 404, 0 failed/cancelled,
 * -1 local creation/write/install/CRC/deletion failure requiring format. */
int divoom_update_system_file_restore_one(const char *relative, uint64_t size, uint32_t crc,
    int done, int total, divoom_system_file_restore_progress progress, int (*retry)(int));
int divoom_update_system_file_restore_format(divoom_system_file_restore_progress progress, int (*retry)(int errors));
/* Current maintenance pass only: 0 download, 1 alternate-name rescue,
 * 2 failed-file cleanup, 3 complete. Counts remain available until next pass. */
void divoom_update_system_file_restore_status(int *stage, int *failed, int *skipped);
/* Preserve the trusted peer backup if this pass could not restore the driver. */
int divoom_update_system_file_restore_driver_skipped(void);
/* Explicit white-box entry; operates only on dedicated test fixtures. */
int divoom_update_system_file_restore_whitebox(int scenario);

#endif



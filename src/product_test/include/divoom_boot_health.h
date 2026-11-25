#ifndef DIVOOM_BOOT_HEALTH_H
#define DIVOOM_BOOT_HEALTH_H
/* 0: normal boot; 1: recovery required; -1: state could not be saved. */
int divoom_boot_health_begin(void);
/* Consecutive abnormal restarts recorded by the boot-health check. */
int divoom_boot_health_restart_count(void);
int divoom_boot_health_clean(void);
void divoom_boot_health_tick(void);
/* Drain health I/O before userdata mkfs. End after a successful remount or a
 * pre-erase cancellation; failed formatting keeps I/O paused until retry/reboot. */
int divoom_boot_health_userdata_format_begin(void);
void divoom_boot_health_userdata_format_end(void);
int divoom_boot_health_mount_failed(void);
int divoom_boot_health_userdata_mount_failed(void);
int divoom_boot_health_mount_reason(int target_usr);
int divoom_boot_health_mount_storage_failure(int target_usr);
void divoom_boot_health_set_mount_failed(int target_usr, int failed);
#endif

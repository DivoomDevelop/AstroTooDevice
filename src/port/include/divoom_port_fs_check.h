#ifndef DIVOOM_PORT_FS_CHECK_H
#define DIVOOM_PORT_FS_CHECK_H
#include "ffcheck.h"
#ifdef __cplusplus
extern "C" {
#endif
int divoom_port_fs_check_volume(const char *devpath, const char *mount_path,
    ff_check_result *result, ff_check_entry *entries, uint32_t capacity,
    ff_check_progress progress, void *context);
int divoom_port_fs_delete_corrupt_batch(const char *devpath, const char *mount_path,
    const ff_check_geometry *geometry, const ff_check_entry *entries, uint32_t count,
    ff_check_commit commit, void *context);
int divoom_port_fs_reclaim_orphans(const char *devpath, const char *mount_path,
    const ff_check_geometry *geometry, int resume, ff_check_result *result,
    ff_check_progress progress, ff_check_commit commit, void *context);
/* Per-devnode, preserved while the volume is unmounted and remounted. */
int divoom_port_fs_screen_volume(const char *devpath, const char *mount_path,
    ff_check_result *result, ff_check_progress progress, void *context);
int divoom_port_fs_write_block(const char *devpath, int blocked);
int divoom_port_fs_remount_volume(const char *devpath, const char *mount_path);
#ifdef __cplusplus
}
#endif
#endif

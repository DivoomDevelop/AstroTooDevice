#ifndef FATFS_CHECK_H
#define FATFS_CHECK_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define FF_CHECK_NO_LFN UINT32_C(0xffffffff)
/* A successful mkfs may still need a checked remount before recovery. */
enum { FF_CHECK_CLEAN=0, FF_CHECK_STRUCTURAL=1, FF_CHECK_FORMATTED_UNMOUNTED=2, FF_CHECK_IO=-1,
       FF_CHECK_NOMEM=-2, FF_CHECK_BUSY=-3, FF_CHECK_UNSUPPORTED=-4 };
enum { FF_CHECK_CROSS_LINK=1U, FF_CHECK_CYCLE=2U, FF_CHECK_INVALID_LINK=4U,
       FF_CHECK_ORPHAN=8U, FF_CHECK_DIRECTORY=16U, FF_CHECK_MIRROR=32U,
       FF_CHECK_FILE_SIZE=64U, FF_CHECK_LFN=128U };
enum { FF_CHECK_STAGE_FAT=1, FF_CHECK_STAGE_DIRECTORY=2, FF_CHECK_STAGE_OWNERS=3,
       FF_CHECK_STAGE_VERIFY=4 };
enum { FF_CHECK_DETACH_PENDING=1, FF_CHECK_FREE_PENDING=2, FF_CHECK_ORPHAN_PENDING=3 };

typedef struct {
    uint32_t serial, fat_type, sector_size, cluster_sectors;
    uint32_t fat_entries, fat_sectors, volume_base, fat_base, directory_base, data_base;
} ff_check_geometry;
/* All offsets are within the parent directory stream, never raw partition
 * offsets. The 32-byte original SFN entry binds the identity across reboot. */
typedef struct {
    uint32_t parent_cluster, entry_offset, lfn_offset, reason;
    uint8_t original[32];
} ff_check_entry;
typedef struct {
    ff_check_geometry geometry;
    uint32_t damage_flags, files, directories, affected_files, orphan_clusters;
    uint32_t cross_links, cycles, complete, unsafe, plan_complete, fs_error;
    uint32_t screen_only; /* Online result never authorizes a deletion plan. */
} ff_check_result;
typedef void (*ff_check_progress)(uint32_t stage, uint32_t done, uint32_t total, void *context);
/* Callback must only commit a raw, independent journal; never call VFS/FatFs.
 * Return 1 only after durable phase commit, 0 to stop without that phase. */
typedef int (*ff_check_commit)(int phase, void *context);

int f_check_volume(const char *drive, ff_check_result *result,
    ff_check_entry *entries, uint32_t capacity, ff_check_progress progress, void *context);
int f_screen_volume(const char *drive, ff_check_result *result,
    ff_check_progress progress, void *context);
/* Maintenance-only, after full ownership validation. The commit callback must
 * durably retain geometry and ORPHAN_PENDING before any FAT write. On reboot,
 * keep the volume write barrier and pass resume=1 before ordinary mirror checks.
 * Only unreferenced FAT entries may differ during resume. No file/dir data is
 * changed. On success orphan_clusters reports the number of entries reclaimed;
 * the caller must recheck the volume before retiring the journal phase. */
int f_reclaim_orphans(const char *drive, const ff_check_geometry *geometry, int resume,
    ff_check_result *result, ff_check_progress progress, ff_check_commit commit, void *context);
/* VFS calls this only when disposing an FD after failed f_close. */
void f_check_forget_file(void *file);
/* Does not mount/read the medium; checks all FIL/FF_DIR lifetimes under lock. */
int f_check_volume_idle(const char *drive);
/* Maintenance only: atomically require idle and reject new FIL/FF_DIR opens. */
int f_check_volume_quiesce(const char *drive, int enable);
int f_delete_corrupt_batch(const char *drive, const ff_check_geometry *geometry,
    const ff_check_entry *entries, uint32_t count, ff_check_commit commit, void *context);
/* Board hook: normal filesystem mutations are blocked when it returns 0.
 * Native batch deletion and explicit mkfs are maintenance-only exceptions. */
int ff_check_write_allowed(uint8_t physical_drive);
#ifdef __cplusplus
}
#endif
#endif

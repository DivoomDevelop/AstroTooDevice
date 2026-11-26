#ifndef ELMFAT_BOOT_REPAIR_H
#define ELMFAT_BOOT_REPAIR_H

#include <stdbool.h>

enum { ELMFAT_BOOT_USERDATA = 1U, ELMFAT_BOOT_USR = 2U };

struct elmfat_boot_repair_result {
    unsigned blocked;  /* An outstanding mutation still owns this volume. */
    unsigned repaired;
    unsigned damaged;  /* Confirmed damage beyond reclaimable orphan clusters. */
};

/* Before normal mounts and workers, with no open FAT handles. */
struct elmfat_boot_repair_result elmfat_boot_repair(bool due, bool (*usr_busy)(void));
int elmfat_remount_checked(const char *device, const char *mount_path);

#endif

#include "divoom_port_fs_check.h"
#include <string.h>
static const char *check_device(const char *path)
{
    if (!path)
        return 0;
    return !strncmp(path, "/dev/", 5) ? path + 5 : path;
}
int divoom_port_fs_check_volume(const char *dev, const char *mount, ff_check_result *out,
                                ff_check_entry *entries, uint32_t capacity, ff_check_progress progress,
                                void *context)
{
    extern int elmfat_check_volume(const char *, ff_check_result *, ff_check_entry *, uint32_t,
                                   ff_check_progress, void *);
    (void)mount;
    dev = check_device(dev);
    if (!dev)
        return FF_CHECK_UNSUPPORTED;
    return elmfat_check_volume(dev, out, entries, capacity, progress, context);
}
int divoom_port_fs_delete_corrupt_batch(const char *dev, const char *mount, const ff_check_geometry *geometry,
                                        const ff_check_entry *entries, uint32_t count, ff_check_commit commit,
                                        void *context)
{
    extern int elmfat_delete_corrupt_batch(const char *, const ff_check_geometry *, const ff_check_entry *,
                                           uint32_t, ff_check_commit, void *);
    (void)mount;
    dev = check_device(dev);
    if (!dev)
        return FF_CHECK_UNSUPPORTED;
    return elmfat_delete_corrupt_batch(dev, geometry, entries, count, commit, context);
}
int divoom_port_fs_reclaim_orphans(const char *dev, const char *mount,
    const ff_check_geometry *geometry, int resume, ff_check_result *out,
    ff_check_progress progress, ff_check_commit commit, void *context)
{
    extern int elmfat_reclaim_orphans(const char *, const ff_check_geometry *, int,
        ff_check_result *, ff_check_progress, ff_check_commit, void *);
    (void)mount;
    dev = check_device(dev);
    return dev ? elmfat_reclaim_orphans(dev, geometry, resume, out, progress, commit, context) :
        FF_CHECK_UNSUPPORTED;
}
int divoom_port_fs_write_block(const char *dev, int blocked)
{
    extern int elmfat_check_write_block(const char *, int);
    dev = check_device(dev);
    return dev ? elmfat_check_write_block(dev, blocked) : -1;
}
int divoom_port_fs_remount_volume(const char *dev, const char *mount)
{
    extern int elmfat_remount_checked(const char *, const char *);
    dev = check_device(dev);
    return dev && mount ? elmfat_remount_checked(dev, mount) : -1;
}

int divoom_port_fs_screen_volume(const char *dev, const char *mount, ff_check_result *out,
                                 ff_check_progress progress, void *context)
{
    extern int elmfat_screen_volume(const char *, ff_check_result *, ff_check_progress, void *);
    (void)mount;
    dev = check_device(dev);
    if (!dev)
        return FF_CHECK_UNSUPPORTED;
    return elmfat_screen_volume(dev, out, progress, context);
}

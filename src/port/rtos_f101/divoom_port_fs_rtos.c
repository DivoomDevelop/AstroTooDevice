#include <sys/errno.h>
#include "divoom_build_config.h"
#if !DIVOOM_BUILD_RTOS_DEVICE
#error "divoom_port_fs_rtos.c is RTOS device-only."
#endif

#include "divoom_port_fs.h"
#include "divoom_device_error.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

const char *divoom_port_path_resolve(const char *path)
{
	return path;
}

int divoom_port_file_exists(const char *path)
{
	const char *resolved;

	if (!path || !path[0])
		return 0;
	resolved = divoom_port_path_resolve(path);
	return access(resolved, F_OK) == 0;
}

int divoom_port_remove_file(const char *path)
{
	const char *resolved;

	if (!path)
		return -1;
	resolved = divoom_port_path_resolve(path);
	return remove(resolved);
}

int divoom_port_rename_file(const char *old_path, const char *new_path)
{
	const char *resolved_old;
	const char *resolved_new;

	if (!old_path || !new_path)
		return -1;
	resolved_old = divoom_port_path_resolve(old_path);
	resolved_new = divoom_port_path_resolve(new_path);
	return rename(resolved_old, resolved_new);
}

int divoom_port_fs_format_volume(const char *devpath, const char *mount_path)
{
#ifdef CONFIG_DRIVERS_SDMMC_DEVFS
	extern int sdmmc_format_devpath(const char *devpath, const char *mount_path);

	if (!devpath || !devpath[0] || !mount_path || !mount_path[0])
		return -1;
    int userdata = !strcmp(devpath, "/dev/UDISK") || !strcmp(mount_path, "/userdata");
    if (userdata) divoom_device_error_userdata_format_begin();
	divoom_port_sync();
    errno = 0;
    int result = sdmmc_format_devpath(devpath, mount_path);
    int format_errno = errno;
    if (userdata) divoom_device_error_userdata_format_end();
    errno = format_errno;
    return result;
#else
	(void)devpath;
	(void)mount_path;
	return -1;
#endif
}

int divoom_port_fs_format_userdata(void)
{
	return divoom_port_fs_format_volume("/dev/UDISK", "/userdata");
}

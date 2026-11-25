#include "divoom_build_config.h"
#if !DIVOOM_BUILD_SIMULATOR
#error "divoom_port_hw_sim.c is simulator-only."
#endif

#include "divoom_port_hw.h"
#include "divoom_port_fs.h"
#include <string.h>

int divoom_port_hw_sysfs_read(const char *path, char *buf, size_t buf_len)
{
	void *fp;
	size_t n;

	if (!path || !buf || buf_len == 0)
		return -1;
	fp = divoom_port_file_open(path, "r");
	if (!fp)
		return -1;
	n = divoom_port_file_read(buf, 1, buf_len - 1, fp);
	divoom_port_file_close(fp);
	if (n == 0)
		return -1;
	buf[n] = '\0';
	return (int)n;
}

int divoom_port_hw_sysfs_write(const char *path, const char *value)
{
	void *fp;
	size_t len;

	if (!path || !value)
		return -1;
	fp = divoom_port_file_open(path, "w");
	if (!fp)
		return -1;
	len = strlen(value);
	if (divoom_port_file_write(value, 1, len, fp) != len) {
		divoom_port_file_close(fp);
		return -1;
	}
	divoom_port_file_close(fp);
	return 0;
}

int divoom_port_hw_usb_set_config(const char *config)
{
	(void)config;
	return 0;
}

int divoom_port_hw_reboot(void)
{
	return 0;
}

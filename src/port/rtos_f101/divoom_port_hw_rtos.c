#include "divoom_build_config.h"
#if !DIVOOM_BUILD_RTOS_DEVICE
#error "divoom_port_hw_rtos.c is RTOS device-only."
#endif

#include "divoom_port_hw.h"
#include "divoom_port_fs.h"
#ifdef CONFIG_DRIVERS_WATCHDOG
#include <sunxi_hal_watchdog.h>
#endif
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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
	/* F101 USB mode switch: hook Allwinner USB HAL when available */
	return 0;
}

int divoom_port_hw_reboot(void)
{
	divoom_port_sync();
#ifdef CONFIG_DRIVERS_WATCHDOG
	hal_watchdog_restart();
#endif
	for (;;)
		;
	return -1;
}

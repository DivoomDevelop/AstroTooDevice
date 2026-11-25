#ifndef DIVOOM_PORT_HW_H
#define DIVOOM_PORT_HW_H

#include <stddef.h>

/* Sysfs text read/write (GPIO, power_supply, gpadc, etc.) */
int divoom_port_hw_sysfs_read(const char *path, char *buf, size_t buf_len);
int divoom_port_hw_sysfs_write(const char *path, const char *value);

/* USB gadget mode (replaces system("setusbconfig ...") on RTOS) */
int divoom_port_hw_usb_set_config(const char *config);

/* Whole-system reset. RTOS uses watchdog and does not return. */
int divoom_port_hw_reboot(void);

#endif /* DIVOOM_PORT_HW_H */

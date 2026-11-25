#include "divoom_build_config.h"
#if !DIVOOM_BUILD_RTOS_DEVICE
#error "RTOS device-only stub; exclude from simulator VS project."
#endif

#include "divoom_ip6103.h"

int divoom_ip6103_open(void) { return -1; }
void divoom_ip6103_close(int fd) { (void)fd; }
int divoom_ip6103_get_wake_source(int fd, int *out_source)
{
	(void)fd;
	if (out_source)
		*out_source = 0;
	return -1;
}
int divoom_ip6103_get_reg(int fd, struct divoom_ip6103_reg_config *reg_cfg)
{
	(void)fd;
	(void)reg_cfg;
	return -1;
}
int divoom_ip6103_set_reg(int fd, struct divoom_ip6103_reg_config *reg_cfg)
{
	(void)fd;
	(void)reg_cfg;
	return -1;
}
int divoom_ip6103_get_calendar(int fd, struct divoom_ip6103_calendar *cal)
{
	(void)fd;
	(void)cal;
	return -1;
}
int divoom_ip6103_set_calendar(int fd, const struct divoom_ip6103_calendar *cal)
{
	(void)fd;
	(void)cal;
	return -1;
}
int divoom_ip6103_get_alarm(int fd, struct divoom_ip6103_alarm *alarm)
{
	(void)fd;
	(void)alarm;
	return -1;
}
int divoom_ip6103_set_alarm(int fd, const struct divoom_ip6103_alarm *alarm)
{
	(void)fd;
	(void)alarm;
	return -1;
}
int divoom_ip6103_shutdown(int fd) { (void)fd; return -1; }
int divoom_ip6103_cancel_hold_restart(int fd) { (void)fd; return -1; }
int divoom_ip6103_reboot(int fd) { (void)fd; return -1; }
int divoom_ip6103_get_key_state(int fd, int *is_long)
{
	(void)fd;
	if (is_long)
		*is_long = 0;
	return -1;
}
int divoom_ip6103_enable_async_key(int fd) { (void)fd; return -1; }
int divoom_ip6103_set_ldo2_enable(int fd, int enable)
{
	(void)fd;
	(void)enable;
	return -1;
}
int divoom_ip6103_set_ldo2_voltage_mv(int fd, int voltage_mv)
{
	(void)fd;
	(void)voltage_mv;
	return -1;
}

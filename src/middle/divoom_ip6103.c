/**
 * IP6103 PMIC/RTC middle 层实现
 * 设备节点：/dev/IP61030
 */

#include "divoom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "divoom_ip6103.h"

#define IP6103_DEV_PATH   "/dev/IP61030"

/* 与内核驱动一致的 ioctl 定义 */
#define MY_IOCTL_MAGIC    'k'
#define IP6103_GET_ROUSE         _IOR(MY_IOCTL_MAGIC, 1, int)
#define IP6103_SET_REG           _IOW(MY_IOCTL_MAGIC, 3, int)
#define IP6103_GET_REG           _IOR(MY_IOCTL_MAGIC, 4, int)
#define IP6103_GET_CALENDAR      _IOR(MY_IOCTL_MAGIC, 5, int)
#define IP6103_SET_CALENDAR      _IOW(MY_IOCTL_MAGIC, 6, int)
#define IP6103_SET_ALARM_CLOCK   _IOW(MY_IOCTL_MAGIC, 7, int)
#define IP6103_GET_ALARM_CLOCK   _IOR(MY_IOCTL_MAGIC, 2, int)
#define IP6103_SHUT_DOWN         _IOR(MY_IOCTL_MAGIC, 8, int)
#define IP6103_REBOOT            _IOR(MY_IOCTL_MAGIC, 9, int)
#define IP6103_GET_KEY_STATE     _IOR(MY_IOCTL_MAGIC, 10, int)
#define IP6103_CANCEL_HOLD_RESTART _IOR(MY_IOCTL_MAGIC, 11, int)

int divoom_ip6103_open(void)
{
	int fd = open(IP6103_DEV_PATH, O_RDWR);
	return fd;
}

void divoom_ip6103_close(int fd)
{
	if (fd >= 0)
		close(fd);
}

int divoom_ip6103_get_wake_source(int fd, int *out_source)
{
	int ret;
	if (fd < 0 || !out_source)
		return -1;
	ret = ioctl(fd, IP6103_GET_ROUSE);
	if (ret < 0)
		return -1;
	*out_source = ret;
	return 0;
}

int divoom_ip6103_get_reg(int fd, struct divoom_ip6103_reg_config *reg_cfg)
{
	if (fd < 0 || !reg_cfg)
		return -1;
	if (ioctl(fd, IP6103_GET_REG, (unsigned long)reg_cfg) < 0)
		return -1;
	return 0;
}

int divoom_ip6103_set_reg(int fd, struct divoom_ip6103_reg_config *reg_cfg)
{
	if (fd < 0 || !reg_cfg)
		return -1;
	if (ioctl(fd, IP6103_SET_REG, (unsigned long)reg_cfg) < 0)
		return -1;
	return 0;
}

int divoom_ip6103_get_calendar(int fd, struct divoom_ip6103_calendar *cal)
{
	if (fd < 0 || !cal)
		return -1;
	if (ioctl(fd, IP6103_GET_CALENDAR, (unsigned long)cal) < 0)
		return -1;
	return 0;
}

int divoom_ip6103_set_calendar(int fd, const struct divoom_ip6103_calendar *cal)
{
	if (fd < 0 || !cal)
		return -1;
	if (ioctl(fd, IP6103_SET_CALENDAR, (unsigned long)cal) < 0)
		return -1;
	return 0;
}

int divoom_ip6103_get_alarm(int fd, struct divoom_ip6103_alarm *alarm)
{
	if (fd < 0 || !alarm)
		return -1;
	if (ioctl(fd, IP6103_GET_ALARM_CLOCK, (unsigned long)alarm) < 0)
		return -1;
	return 0;
}

int divoom_ip6103_set_alarm(int fd, const struct divoom_ip6103_alarm *alarm)
{
	if (fd < 0 || !alarm)
		return -1;
	if (ioctl(fd, IP6103_SET_ALARM_CLOCK, (unsigned long)alarm) < 0)
		return -1;
	return 0;
}

int divoom_ip6103_shutdown(int fd)
{
	if (fd < 0)
		return -1;
	if (ioctl(fd, IP6103_SHUT_DOWN) < 0)
		return -1;
	return 0;
}

int divoom_ip6103_cancel_hold_restart(int fd)
{
	if (fd < 0)
		return -1;
	if (ioctl(fd, IP6103_CANCEL_HOLD_RESTART) < 0)
		return -1;
	return 0;
}

int divoom_ip6103_reboot(int fd)
{
	if (fd < 0)
		return -1;
	if (ioctl(fd, IP6103_REBOOT) < 0)
		return -1;
	return 0;
}

int divoom_ip6103_get_key_state(int fd, int *is_long)
{
	int ret;
	if (fd < 0 || !is_long)
		return -1;
	ret = ioctl(fd, IP6103_GET_KEY_STATE);
	if (ret < 0)
		return -1;
	*is_long = ret ? 1 : 0;
	return 0;
}

int divoom_ip6103_enable_async_key(int fd)
{
	int flags;
	if (fd < 0)
		return -1;
	if (fcntl(fd, F_SETOWN, getpid()) == -1)
		return -1;
	flags = fcntl(fd, F_GETFL);
	if (flags == -1)
		return -1;
	if (fcntl(fd, F_SETFL, flags | FASYNC) == -1)
		return -1;
	return 0;
}

/* 0x41: LDO 使能寄存器，bit2 = LDO2EN (1=ENABLE, 0=DISABLE) */
#define IP6103_REG_SW_LDO_CTL2  0x41
#define IP6103_LDO2EN_BIT       (1u << 2)

/* 0x46: LDO2 电压调节，bit[6:0] 可写，0.7V~3.4V 步进 25mV，bit7 只读 */
#define IP6103_REG_LDO2         0x46
#define IP6103_LDO2_VOLT_MIN_MV 700
#define IP6103_LDO2_VOLT_MAX_MV 3400
#define IP6103_LDO2_VOLT_STEP_MV 25

int divoom_ip6103_set_ldo2_enable(int fd, int enable)
{
	struct divoom_ip6103_reg_config rcfg;

	if (fd < 0)
		return -1;
	memset(&rcfg, 0, sizeof(rcfg));
	rcfg.reg[0] = IP6103_REG_SW_LDO_CTL2;
	rcfg.len = 1;
	if (divoom_ip6103_get_reg(fd, &rcfg) != 0)
		return -1;
	if (enable)
		rcfg.data[0] |= IP6103_LDO2EN_BIT;
	else
		rcfg.data[0] &= (unsigned char)~IP6103_LDO2EN_BIT;
	return divoom_ip6103_set_reg(fd, &rcfg);
}

int divoom_ip6103_set_ldo2_voltage_mv(int fd, int voltage_mv)
{
	struct divoom_ip6103_reg_config rcfg;
	int code;

	if (fd < 0)
		return -1;
	if (voltage_mv < IP6103_LDO2_VOLT_MIN_MV)
		voltage_mv = IP6103_LDO2_VOLT_MIN_MV;
	if (voltage_mv > IP6103_LDO2_VOLT_MAX_MV)
		voltage_mv = IP6103_LDO2_VOLT_MAX_MV;
	code = (voltage_mv - IP6103_LDO2_VOLT_MIN_MV) / IP6103_LDO2_VOLT_STEP_MV;
	if (code > 0x7F)
		code = 0x7F;

	memset(&rcfg, 0, sizeof(rcfg));
	rcfg.reg[0] = IP6103_REG_LDO2;
	rcfg.len = 1;
	if (divoom_ip6103_get_reg(fd, &rcfg) != 0)
		return -1;
	rcfg.data[0] = (unsigned char)((rcfg.data[0] & 0x80) | (code & 0x7F));
	return divoom_ip6103_set_reg(fd, &rcfg);
}

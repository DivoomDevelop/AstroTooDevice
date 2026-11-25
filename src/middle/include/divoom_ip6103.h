/**
 * IP6103 PMIC/RTC middle 层
 * 设备节点：/dev/IP61030
 * 功能：唤醒源、寄存器读写、日历、闹钟、关机/重启、按键状态（短按/长按）
 */

#ifndef _DIVOOM_IP6103_H_
#define _DIVOOM_IP6103_H_

#ifdef __cplusplus
extern "C" {
#endif

/* 唤醒源（与驱动一致） */
enum divoom_ip6103_wake_source {
	DIVOOM_IP6103_WAKE_EXTERN,
	DIVOOM_IP6103_WAKE_ALARM,
	DIVOOM_IP6103_WAKE_VIN,
	DIVOOM_IP6103_WAKE_REBOOT,
	DIVOOM_IP6103_WAKE_ONOFF_SHORT,
	DIVOOM_IP6103_WAKE_ONOFF_LONG,
	DIVOOM_IP6103_WAKE_ONOFF_REST,
};

/* 寄存器读写（与驱动一致） */
struct divoom_ip6103_reg_config {
	unsigned char reg[128];
	unsigned char data[128];
	unsigned char len;
};

struct divoom_ip6103_alarm {
	unsigned char sec;
	unsigned char min;
	unsigned char hour;
	unsigned char day;
	unsigned char mon;
	unsigned char year;
};

struct divoom_ip6103_calendar {
	unsigned char sec;
	unsigned char min;
	unsigned char hour;
	unsigned char day;
	unsigned char week;
	unsigned char mon;
	unsigned char year;
	unsigned char is_leap_year;
};

/**
 * 打开 IP6103 设备
 * @return >=0 为 fd，<0 失败
 */
int divoom_ip6103_open(void);

/**
 * 关闭设备
 * @param fd  divoom_ip6103_open 返回的 fd，-1 则忽略
 */
void divoom_ip6103_close(int fd);

/**
 * 获取唤醒源
 * @param fd  设备 fd
 * @param out_source  输出唤醒源（见 enum divoom_ip6103_wake_source）
 * @return 0 成功，-1 失败
 */
int divoom_ip6103_get_wake_source(int fd, int *out_source);

/**
 * 读寄存器
 * @param fd  设备 fd
 * @param reg_cfg  reg[] 填寄存器地址，len 为个数；返回时 data[] 为读到的值
 * @return 0 成功，-1 失败
 */
int divoom_ip6103_get_reg(int fd, struct divoom_ip6103_reg_config *reg_cfg);

/**
 * 写寄存器
 * @param fd  设备 fd
 * @param reg_cfg  reg[]/data[]/len 为要写的内容
 * @return 0 成功，-1 失败
 */
int divoom_ip6103_set_reg(int fd, struct divoom_ip6103_reg_config *reg_cfg);

/**
 * 读日历
 * @param fd  设备 fd
 * @param cal  输出当前 RTC 日历
 * @return 0 成功，-1 失败
 */
int divoom_ip6103_get_calendar(int fd, struct divoom_ip6103_calendar *cal);

/**
 * 写日历
 * @param fd  设备 fd
 * @param cal  要设置的日历
 * @return 0 成功，-1 失败
 */
int divoom_ip6103_set_calendar(int fd, const struct divoom_ip6103_calendar *cal);

/**
 * 读闹钟
 * @param fd  设备 fd
 * @param alarm  输出当前闹钟时间
 * @return 0 成功，-1 失败
 */
int divoom_ip6103_get_alarm(int fd, struct divoom_ip6103_alarm *alarm);

/**
 * 写闹钟
 * @param fd  设备 fd
 * @param alarm  要设置的闹钟
 * @return 0 成功，-1 失败
 */
int divoom_ip6103_set_alarm(int fd, const struct divoom_ip6103_alarm *alarm);

/**
 * 关机
 * @param fd  设备 fd
 * @return 0 成功，-1 失败（成功时可能不会返回）
 */
int divoom_ip6103_shutdown(int fd);

/**
 * 取消长按 IRQ 时内核排队的延时重启（不写入休眠寄存器）。
 * 应在长按关机流程尽早调用，避免 main_exit/关机音耗时超过 10s 时先被 kernel_restart。
 * @param fd  设备 fd
 * @return 0 成功，-1 失败
 */
int divoom_ip6103_cancel_hold_restart(int fd);

/**
 * 重启
 * @param fd  设备 fd
 * @return 0 成功，-1 失败（成功时可能不会返回）
 */
int divoom_ip6103_reboot(int fd);

/**
 * 获取按键状态（在 SIGIO 或 poll 等通知到有按键后调用）
 * @param fd  设备 fd
 * @param is_long  输出：1 长按，0 短按
 * @return 0 成功，-1 失败
 */
int divoom_ip6103_get_key_state(int fd, int *is_long);

/**
 * 启用异步按键通知（SIGIO）
 * @param fd  设备 fd
 * @return 0 成功，-1 失败
 */
int divoom_ip6103_enable_async_key(int fd);

/**
 * 设置 LDO2 使能（寄存器 0x41 bit2 LDO2EN，datasheet：1=ENABLE，0=DISABLE）
 * @param fd    设备 fd
 * @param enable  0=关闭 LDO2，非0=打开 LDO2
 * @return 0 成功，-1 失败
 */
int divoom_ip6103_set_ldo2_enable(int fd, int enable);

/**
 * 设置 LDO2 输出电压（寄存器 0x46，datasheet：0.7V~3.4V，步进 25mV）
 * @param fd    设备 fd
 * @param voltage_mv  输出电压毫伏数，有效范围 700~3400，超出则钳位
 * @return 0 成功，-1 失败
 */
int divoom_ip6103_set_ldo2_voltage_mv(int fd, int voltage_mv);

#ifdef __cplusplus
}
#endif

#endif /* _DIVOOM_IP6103_H_ */

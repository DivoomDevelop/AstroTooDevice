/**
 * 电池电压检测 middle 层
 * 通过 GPADC0 分压读取：BAT -> R31 -> GPADC0 -> R32 -> GND
 * 电阻值以原理图为准；内核：/sys/class/gpadc/data 或 IIO in_voltage0_raw
 */

#include "divoom.h"
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

/* 选通道后等待转换完成再读，避免读到旧值（单位 ms） */
#ifndef DIVOOM_GPADC_READ_DELAY_MS
#define DIVOOM_GPADC_READ_DELAY_MS  30
#endif
/* 多次采样取中值，过滤偶发 stale/噪声；设为 1 则只读一次 */
#ifndef DIVOOM_GPADC_SAMPLE_COUNT
#define DIVOOM_GPADC_SAMPLE_COUNT  3
#endif
#define GPADC_SAMPLE_MAX  5

/* 分压电阻 (Ω)：与原理图一致 R31=910K, R32=220K。分压比 220/1130≈0.195，8.2V→约1.60V，不超1.8V量程。 */
#define R31_OHM  910000
#define R32_OHM  220000

/* GPADC 量程：12bit，参考 1.8V（与内核 VOL_RANGE 一致） */
#define GPADC_REF_MV   1800
#define GPADC_MAX_RAW 4095
/* 内核通道未使能时返回 VOL_RANGE+1，应视为无效 */
#define GPADC_CHANNEL_DISABLED_VAL  1800001

#define GPADC_SYSFS_DATA   "/sys/class/gpadc/data"

/** 通过 /sys/class/gpadc/data 读指定通道（0~3），返回 raw 或 mV 取决于 BSP */
static int read_gpadc_channel_sysfs(int channel, int *out_val)
{
	char buf[32];
	char ch_str[2];

	if (channel < 0 || channel > 3)
		return -1;

	ch_str[0] = (char)('0' + channel);
	ch_str[1] = '\0';
	if (divoom_port_hw_sysfs_write(GPADC_SYSFS_DATA, ch_str) != 0)
		return -1;
#if DIVOOM_GPADC_READ_DELAY_MS > 0
	divoom_port_delay_ms(DIVOOM_GPADC_READ_DELAY_MS);
#endif
	if (divoom_port_hw_sysfs_read(GPADC_SYSFS_DATA, buf, sizeof(buf)) <= 0)
		return -1;

	*out_val = atoi(buf);
	return 0;
}

/** 从 IIO 读取 in_voltage0 的数值（单位 mV） */
static int read_gpadc0_mv_iio(int *out_mv)
{
	char path[256];
	char buf[32];
	DIR *dir;
	const divoom_port_dirent_t *ent;
	int found = 0, val;

	dir = divoom_port_dir_open("/sys/bus/iio/devices");
	if (!dir)
		return -1;

	while ((ent = divoom_port_dir_read(dir)) != NULL) {
		if (strncmp(ent->name, "iio:device", 10) != 0)
			continue;
		snprintf(path, sizeof(path),
			 "/sys/bus/iio/devices/%s/in_voltage0_raw", ent->name);
		if (divoom_port_hw_sysfs_read(path, buf, sizeof(buf)) > 0) {
			val = atoi(buf);
			found = 1;
			break;
		}
	}
	divoom_port_dir_close(dir);

	if (!found)
		return -1;

	*out_mv = val;
	return 0;
}

/**
 * 部分 BSP 的 /sys/class/gpadc/data 返回的是 ADC 原始值 (0~4095) 而非 mV。
 * 若设为 0：认为 data 是 raw，按 1.8V 参考换算成 mV（读值一直 5xxx 时用此）。
 * 若设为 1：认为 data 已是 mV，不换算。
 */
#ifndef DIVOOM_GPADC_DATA_IS_MV
#define DIVOOM_GPADC_DATA_IS_MV  0
#endif
/* 校准：当前 BSP 的 raw（1525~1532）与万用表实测 GPADC0 引脚 1.5V 不符（理论 raw 应约 3417）。
 * 启用后按「实测电压/按 1.8V 参考算出的电压」比例修正，使显示电池电压与真实一致。
 * 编译时加 -DDIVOOM_GPADC0_NO_CALIB 可关闭校准。 */
#ifndef DIVOOM_GPADC0_NO_CALIB
#define DIVOOM_GPADC0_USE_CALIB  1
#endif
#ifdef DIVOOM_GPADC0_USE_CALIB
/* 万用表在 GPADC0 脚实测电压 (mV)；该校准时刻对应的 raw。换算 7858、实际 7780，故略降使显示≈7780 */
#ifndef DIVOOM_GPADC0_CALIB_REAL_MV
#define DIVOOM_GPADC0_CALIB_REAL_MV  1524
#endif
#ifndef DIVOOM_GPADC0_CALIB_RAW_AT_REAL
#define DIVOOM_GPADC0_CALIB_RAW_AT_REAL  1528
#endif
#endif

/** 将 ADC 原始值 (0~4095) 转为 GPADC 引脚电压 (mV)，1.8V 参考 */
static int raw_to_gpadc0_mv(int raw)
{
	if (raw < 0 || raw > GPADC_MAX_RAW)
		return raw;
	return (int)((unsigned long)raw * GPADC_REF_MV / (GPADC_MAX_RAW + 1));
}

/** 读到的值是否有效：通道未使能时内核返回 1800001，应视为无效 */
static int gpadc_value_valid(int val)
{
	if (val < 0)
		return 0;
	if (val == GPADC_CHANNEL_DISABLED_VAL)
		return 0;
	if (val > GPADC_MAX_RAW && val > 2000)
		return 0; /* 非 raw 且非合理 mV，视为无效 */
	return 1;
}

/** 简单排序取中值（小数组） */
static int median_int(int *arr, int n)
{
	int i, j, t;
	if (n <= 0)
		return 0;
	for (i = 0; i < n - 1; i++)
		for (j = i + 1; j < n; j++)
			if (arr[j] < arr[i]) {
				t = arr[i];
				arr[i] = arr[j];
				arr[j] = t;
			}
	return arr[n / 2];
}

/** 内部：读指定通道并统一为 mV，支持多次采样取中值 */
static int read_gpadc_channel_mv(int channel, int *out_mv)
{
	int val, n, i;
	int samples[GPADC_SAMPLE_MAX];
	int sample_count = DIVOOM_GPADC_SAMPLE_COUNT;

	if (sample_count > GPADC_SAMPLE_MAX)
		sample_count = GPADC_SAMPLE_MAX;
	if (sample_count < 1)
		sample_count = 1;

	if (channel == 0 && read_gpadc0_mv_iio(&val) == 0) {
		if (!gpadc_value_valid(val))
			return -1;
		*out_mv = DIVOOM_GPADC_DATA_IS_MV ? val : raw_to_gpadc0_mv(val);
#ifdef DIVOOM_GPADC0_USE_CALIB
		if (!DIVOOM_GPADC_DATA_IS_MV && *out_mv > 0) {
			int ref_mv = raw_to_gpadc0_mv(DIVOOM_GPADC0_CALIB_RAW_AT_REAL);
			if (ref_mv > 0)
				*out_mv = (int)((unsigned long)*out_mv * DIVOOM_GPADC0_CALIB_REAL_MV / ref_mv);
		}
#endif
		return 0;
	}

	/* sysfs：多次采样取中值，减少 stale/噪声 */
	n = 0;
	for (i = 0; i < sample_count; i++) {
		if (read_gpadc_channel_sysfs(channel, &val) != 0)
			break;
		if (!gpadc_value_valid(val))
			continue;
		samples[n++] = DIVOOM_GPADC_DATA_IS_MV ? val : raw_to_gpadc0_mv(val);
		if (i < sample_count - 1)
			divoom_port_delay_ms((20000) / 1000U); /* 20ms 间隔再读 */
	}
	if (n == 0)
		return -1;
	*out_mv = (n == 1) ? samples[0] : median_int(samples, n);
#ifdef DIVOOM_GPADC0_USE_CALIB
	if (channel == 0 && !DIVOOM_GPADC_DATA_IS_MV && *out_mv > 0) {
		int ref_mv = raw_to_gpadc0_mv(DIVOOM_GPADC0_CALIB_RAW_AT_REAL);
		if (ref_mv > 0)
			*out_mv = (int)((unsigned long)*out_mv * DIVOOM_GPADC0_CALIB_REAL_MV / ref_mv);
	}
#endif
	return 0;
}

/** V_BAT = V_GPADC0 * (R31 + R32) / R32 */
static int gpadc0_mv_to_battery_mv(int gpadc0_mv)
{
	unsigned long long bat_mv;

	bat_mv = (unsigned long long)gpadc0_mv * (R31_OHM + R32_OHM) / R32_OHM;
	return (int)bat_mv;
}

int divoom_battery_voltage_get_gpadc0_mv(int *out_mv)
{
	if (!out_mv)
		return -1;
	return read_gpadc_channel_mv(0, out_mv);
}

/** 读指定 GPADC 通道的引脚电压 (mV)，channel 0~3 */
int divoom_battery_voltage_get_channel_mv(int channel, int *out_mv)
{
	if (!out_mv || channel < 0 || channel > 3)
		return -1;
	return read_gpadc_channel_mv(channel, out_mv);
}

int divoom_battery_voltage_get_mv(int *out_mv)
{
	int gpadc0_mv;

	if (!out_mv)
		return -1;
	if (read_gpadc_channel_mv(0, &gpadc0_mv) != 0)
		return -1;
	*out_mv = gpadc0_mv_to_battery_mv(gpadc0_mv);
	return 0;
}

/**
 * 用指定通道按分压比换算为“电池电压”(mV)。用于排查电池实际接在哪个通道。
 * 若 channel 0 一直不变，可轮询 1/2/3 看哪个随电量变化。
 */
int divoom_battery_voltage_get_mv_by_channel(int channel, int *out_mv)
{
	int ch_mv;

	if (!out_mv || channel < 0 || channel > 3)
		return -1;
	if (read_gpadc_channel_mv(channel, &ch_mv) != 0)
		return -1;
	*out_mv = gpadc0_mv_to_battery_mv(ch_mv);
	return 0;
}

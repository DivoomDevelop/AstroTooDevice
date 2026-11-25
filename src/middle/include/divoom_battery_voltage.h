#ifndef _DIVOOM_BATTERY_VOLTAGE_H_
#define _DIVOOM_BATTERY_VOLTAGE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 电池电压检测（通过 GPADC0 分压）
 * 硬件：BAT -> R31(910K) -> GPADC0 -> R32(220K) -> GND
 * 内核接口：/sys/class/gpadc/data 或 IIO in_voltage0_raw
 */

/**
 * 读取电池电压（单位 mV）
 * @param out_mv  输出：电池电压毫伏值
 * @return 0 成功，-1 失败（GPADC 不可用或读错）
 */
int divoom_battery_voltage_get_mv(int *out_mv);

/**
 * 读取 GPADC0 引脚电压（分压后的电压，单位 mV）
 * @param out_mv  输出：GPADC0 电压毫伏值
 * @return 0 成功，-1 失败
 */
int divoom_battery_voltage_get_gpadc0_mv(int *out_mv);

/**
 * 读取指定 GPADC 通道的引脚电压 (mV)，channel 0~3
 * @param channel 通道号 0~3
 * @param out_mv  输出：该通道电压毫伏值
 * @return 0 成功，-1 失败
 */
int divoom_battery_voltage_get_channel_mv(int channel, int *out_mv);

/**
 * 用指定通道按分压比换算为“电池电压”(mV)。用于排查电池接在哪个通道。
 * 若通道 0 一直不变，可轮询 1/2/3 看哪个随电量变化。
 * @param channel 通道号 0~3
 * @param out_mv  输出：按分压比换算的电池电压毫伏值
 * @return 0 成功，-1 失败
 */
int divoom_battery_voltage_get_mv_by_channel(int channel, int *out_mv);

#ifdef __cplusplus
}
#endif

#endif /* _DIVOOM_BATTERY_VOLTAGE_H_ */

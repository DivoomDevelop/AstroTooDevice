/**
 * divoom_power_monitor.h
 * DIVOOM 电池/电源监控模块 - 对外接口与类型定义
 * Copyright (c) Mountain View Silicon Tech. Inc. All rights reserved.
 */

#ifndef _DIVOOM_POWER_MONITOR_H_
#define _DIVOOM_POWER_MONITOR_H_

#include "divoom_type.h"

#ifndef CFG_FUNC_POWER_MONITOR_EN
#define CFG_FUNC_POWER_MONITOR_EN  1
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 事件码：与 key_event_deal / 应用层一致，用于限音量、关机等 */
#define DIVOOM_MSG_LOW_POWER      0   /* 进入/持续低电：通知应用、限制音量 */
#define DIVOOM_MSG_LOW_POWER_OFF  1   /* 欠压关机 */

/* 电池状态（PowerMonitor 返回值 / 内部状态） */
#define PWR_MNT_HIGH_V  0   /* 正常/高电 */
#define PWR_MNT_OFF_V   1   /* 欠压，需关机 */
#define PWR_MNT_LOW_V   2   /* 低电告警 */

/* 电量等级枚举（内部档位与对外 0～9 级，共 10 档） */
typedef enum _PWR_LEVEL {
    PWR_LEVEL_0 = 0,
    PWR_LEVEL_1,
    PWR_LEVEL_2,
    PWR_LEVEL_3,
    PWR_LEVEL_4,
    PWR_LEVEL_5,
    PWR_LEVEL_6,
    PWR_LEVEL_7,
    PWR_LEVEL_8,
    PWR_LEVEL_9,
    PWR_LEVEL_FULL = PWR_LEVEL_9  /* 满电，与 PWR_LEVEL_9 等同 */
} PWR_LEVEL;

/**
 * 上电初始化；电量档位等待 JL BtCore 0x55 推送后更新。
 */
void PowerMonitorInit(void);

/**
 * 周期调用；返回 JL 缓存的电池状态（不再本地 GPADC 采样）。
 * @return 当前电池状态：PWR_MNT_HIGH_V / PWR_MNT_LOW_V / PWR_MNT_OFF_V
 */
uint8 PowerMonitor(void);

/**
 * 获取当前电量档位（PWR_LEVEL_0～PWR_LEVEL_FULL）
 */
PWR_LEVEL PowerLevelGet(void);

/**
 * 是否为低电状态（用于限音量、UI 等）
 * @return 非 0 表示低电
 */
uint8 PowerVolIsLowPower(void);

/**
 * 当前平均电压值（mV），来自 JL 0x34 推送缓存
 */
uint16 PowerVolGet(void);

/**
 * 供 LED 显示用的档位（经防抖后的档位）
 */
uint8 PowerLevelLedGet(void);

/**
 * 对外 0～9 级电量（JL 0x55 推送缓存）
 * 语义：N → (N+1)×10%，9=100%；放电 0=1红/降功率或关机显示，1=1绿
 */
int divoom_get_cur_battery_level(void);

/** 主菜单 5 格：phone 0/1→1 … 8/9→5（与格 = phone/2+1 一致，0 也是 1 格） */
int divoom_power_phone_to_ui_bar(int phone);

/**
 * 放电 phone1 没有单独绿图，复用充电 1 格图。
 * @return 非 0 表示用充电 1～5 格素材
 */
int divoom_power_use_charge_battery_art(int phone, int charging);

void divoom_power_set_charge(uint8 statu);

void divoom_power_jl_set_usb_status(uint8 statu);


/**
 * 是否插入充电（JL 0x35 USB insert，杰理侧 GPIO 5V 检测）
 */
int divoom_is_in_charge(void);

/**
 * JL→F101 DM_BTCORE_BAT_VOLT_LEVE(0x55) 推送：更新电量档位缓存（0~9）。
 */
void divoom_power_set_level(uint8 level);

PWR_LEVEL divoom_power_get_level(void);

/**
 * 向 JL 查询电池电压等级（DM_BTCORE_BAT_VOLT_LEVE，0~9 级）并更新本地档位。
 * @return 0 成功，-1 失败
 */
int divoom_power_fetch_bat_volt_level_from_jl(void);

/**
 * JL→F101 DM_BTCORE_BAT_VOLT_AVR(0x34) 推送：更新电池/USB 电压缓存（mV）。
 */
void divoom_power_on_jl_bat_volt_avr(uint16 bat_mv, uint16 usb_mv);

/** JL 上报的 USB 实时电压（mV）；未收到 0x34 前为 0 */
uint16 divoom_power_get_jl_usb_volt(void);

/**
 * 当前充电状态
 */
uint8 divoom_is_charge_sta(void);

/**
 * 主循环：推进软定时器 + 调用 PowerMonitor()，并根据状态上报低电/关机事件。
 * 应在系统主循环中周期调用（如每 10ms～100ms）。
 */
void divoom_power_monitor_mainloop(void);

/**
 * 关机流程中调用，在关机态下继续监测充电与电量（如充电灯）。
 */
uint8 Poweroff_charger_PowerMonitor(void);

/**
 * 将 DIVOOM_MSG_LOW_POWER / DIVOOM_MSG_LOW_POWER_OFF 转给系统（限音量、关机等）
 */
void divoom_power_monitor_event_to_user(uint16 event);

/** 记录开机时刻，启动 early boot 欠压关机保护 */
void divoom_power_monitor_boot_guard_init(void);

/** 开机显示完成（POWER_ON_OVER / 开机动画结束）后调用 */
void divoom_power_monitor_set_boot_complete(void);

#ifdef __cplusplus
}
#endif

#endif /* _DIVOOM_POWER_MONITOR_H_ */

/**
 * divoom_power_monitor.c
 * AstroToo 双 MCU：电池电量由杰理 ADC 采样并经 BtCore 推送，F101 只缓存本地值。
 *   0x55 DM_BTCORE_BAT_VOLT_LEVE — 电量档位 0~9（JL 每秒推送）
 *   0x34 DM_BTCORE_BAT_VOLT_AVR  — 电池/USB 平均电压 mV（JL 每秒推送）
 *   0x33 DM_BTCORE_REMIND_BAT_LOW_POWER — 低电/关机提醒（由 JL 判定）
 * Copyright (c) Mountain View Silicon Tech. Inc. All rights reserved.
 */

#include "divoom.h"
#include "divoom_power_monitor.h"
#include "divoom_board_io_config.h"
#include "divoom_lvgl_top_battery_menu.h"
#include "divoom_lvgl_poweron.h"
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

/* early boot 期间忽略 JL 0x33 欠压关机，直至 POWER_ON_OVER 或超时 */
#define DIVOOM_EARLY_BOOT_GUARD_MS          10000

/* 软定时器（不依赖具体 RTOS） */
typedef struct __dm_timer_ {
    uint32 TimeOutVal;
    uint32 TickValCache;
    uint8  IsTimeOut;
} dm_timer;

static dm_timer PowerMonitorTimer;

/* JL 上报缓存 */
static uint16  LdoinLevelAverage;
static PWR_LEVEL PowerLevel = PWR_LEVEL_FULL;
static uint8   divoom_powerled_level = PWR_LEVEL_FULL;
static uint8   divoom_powerled_level_pre = PWR_LEVEL_FULL;
static int     divoom_phone_pwr_level = PWR_LEVEL_FULL;
static uint8   DivoomBatteryStatus = PWR_MNT_HIGH_V;
static uint8   s_jl_bat_level_valid;

static uint8   divoom_jl_usb_insert;
static uint8   divoom_in_charge_sta;
static uint8   divoom_in_charge_full;
static uint16  s_jl_usb_volt_mv;
static uint8   s_jl_usb_first_statu_seen;
static uint8   s_jl_usb_suppress_until_unplug; /* 插着开机：拔线前不弹充电提示 */
static uint8   s_jl_charge_tip_shown;

static uint32  s_boot_start_ms;
static uint8   s_boot_power_on_over;
// static uint8   s_low_batt_dimmed; /* 低电自动降亮度已关 */

static void divoom_power_update_battery_status_from_level(uint8 level);

static void divoom_power_post_battery_event(uint8 stat)
{
	divoom_sys_msg_info model_msg = {0};
	uint8 *bat_stat;

	bat_stat = (uint8 *)divoom_system_malloc(1);
	if (!bat_stat)
		return;
	*bat_stat = stat;

	model_msg.msg_type = 1;
	model_msg.src_id = DIVOOM_MODEL_MAIN;
	model_msg.dest_id = DIVOOM_MODEL_MAIN;
	model_msg.msg_id = DIVOOM_MAIN_EVENT_BATTERY;
	model_msg.data = bat_stat;
	model_msg.data_len = 1;
	divoom_sys_model_crl_msg_push(&model_msg);
}

/* 与 0x33 低电提醒同路：DIVOOM_MAIN_EVENT_BATTERY → divoom_main_disp_low_battery */
static void divoom_power_post_low_battery_warn(void)
{
	if (!PowerVolIsLowPower())
		return;

	divoom_power_post_battery_event(BATTERY_STAT_LOW_WARN);

	// s_low_batt_dimmed = 1;
	/* 钳位在 music_ctrl（JL 3/8 档）；提醒时同步封顶音量到 JL，覆盖拔线后 cap 从 50%→18% 等场景 */
	divoom_music_ctrl_set_volume(divoom_music_ctrl_get_volume());
}

/* 低电把背光打到 30。放电过程电压只会往下走，格数弹回不算恢复。
 * 插上充电立刻拉回用户亮度，不看是否已经离开 LOW。 */
static void divoom_power_restore_brightness_on_charge(void)
{
	// if (!s_low_batt_dimmed)
	// 	return;
	// s_low_batt_dimmed = 0;
	// divoom_main_apply_sys_light();
}

static void divoom_power_set_charge_sta(uint8 charge_sta)
{
	divoom_in_charge_sta = charge_sta;
	if (!charge_sta) {
		divoom_in_charge_full = 0;
		divoom_power_update_battery_status_from_level((uint8)PowerLevel);
		divoom_power_post_low_battery_warn();
		return;
	}
	// divoom_power_restore_brightness_on_charge();
	/* 插上后立刻按电压决定是否继续封 8 档，避免 OSD 先放开、杰理还在限 */
	divoom_power_update_battery_status_from_level((uint8)PowerLevel);
}

static void divoom_power_try_show_charge_tip(void)
{
	if (s_jl_usb_suppress_until_unplug || s_jl_charge_tip_shown)
		return;
	if (!divoom_jl_usb_insert)
		return;
	divoom_lvgl_top_battery_menu_init(1);
	s_jl_charge_tip_shown = 1;
}

static uint32 get_system_tick_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32)((tv.tv_sec * 1000UL) + (tv.tv_usec / 1000));
}

static void TimeOutSet(dm_timer *timer, uint32 ms)
{
    timer->TimeOutVal = ms;
    timer->TickValCache = get_system_tick_ms();
    timer->IsTimeOut = 0;
}

static void Time_mainloop(dm_timer *timer, uint32 step_ms)
{
    uint32 now = get_system_tick_ms();
    (void)step_ms;
    if (now - timer->TickValCache >= timer->TimeOutVal) {
        timer->IsTimeOut = 1;
        timer->TickValCache = now;
    }
}

static void divoom_power_update_battery_status_from_level(uint8 level)
{
	uint8 old_sta = DivoomBatteryStatus;

	/*
	 * 关机只信 0x33 OFF，不要用 0x55 phone0 当关机。
	 * 放电进低电由 0x33 置位；phone1 已是 20%，不能 phone<=1 当低电。
	 * 充电限音跟 0x55 是否还是 10%（杰理：进低电仍 6.90；已 10% 后再充要未补偿>7.20 才离开）。
	 * 不要用 0x34 比 6.90/7.20。没进过 10% 时充电 6.90～7.27 是 20%，本侧不会限音。
	 */
	if (old_sta == PWR_MNT_OFF_V) {
		if (divoom_in_charge_sta) {
			if (!s_jl_bat_level_valid)
				return;
			if (divoom_phone_pwr_level >= 1)
				DivoomBatteryStatus = PWR_MNT_HIGH_V;
		}
		if (DivoomBatteryStatus == PWR_MNT_OFF_V)
			return;
	}

	if (divoom_in_charge_sta) {
		if (!s_jl_bat_level_valid)
			return;
		if (divoom_phone_pwr_level <= 0)
			DivoomBatteryStatus = PWR_MNT_LOW_V;
		else
			DivoomBatteryStatus = PWR_MNT_HIGH_V;
	} else if (level == 0)
		DivoomBatteryStatus = PWR_MNT_LOW_V;
	else
		DivoomBatteryStatus = PWR_MNT_HIGH_V;

	if (divoom_in_charge_sta && old_sta != DivoomBatteryStatus) {
		divoom_music_ctrl_set_volume(divoom_music_ctrl_get_volume());
		/* 充电中离开低电：音量解开时把背光一并拉回（不依赖再次插电） */
		// if ((old_sta == PWR_MNT_LOW_V || old_sta == PWR_MNT_OFF_V) &&
		//     DivoomBatteryStatus == PWR_MNT_HIGH_V) {
		// 	s_low_batt_dimmed = 0;
		// 	divoom_main_apply_sys_light();
		// }
	}
}

static void divoom_power_apply_level(uint8 level)
{
	if (level > (uint8)PWR_LEVEL_FULL)
		level = (uint8)PWR_LEVEL_FULL;

	PowerLevel = (PWR_LEVEL)level;
	divoom_phone_pwr_level = (int)level;
	s_jl_bat_level_valid = 1;

	divoom_powerled_level = level;
	divoom_powerled_level_pre = level;

	divoom_power_update_battery_status_from_level(level);
}

void divoom_power_set_level(uint8 level)
{
	divoom_power_apply_level(level);
}

PWR_LEVEL divoom_power_get_level(void)
{
    return PowerLevel;
}

int divoom_power_fetch_bat_volt_level_from_jl(void)
{
	uint8 reply = 0;
	uint16_t reply_len = 0;
	int ret;

	if (!divoom_btcore_uart_cmd_is_ready())
		return -1;

	ret = divoom_btcore_uart_cmd_send_wait(DM_BTCORE_BAT_VOLT_LEVE, NULL, 0,
					       &reply, 1, &reply_len);
	if (ret != 0 || reply_len < 1u)
		return -1;

	divoom_power_apply_level(reply);
	return 0;
}

void divoom_power_jl_set_usb_status(uint8 statu)
{
	uint8 prev_insert = divoom_jl_usb_insert;

	if (!s_jl_usb_first_statu_seen) {
		s_jl_usb_first_statu_seen = 1;
		if (statu)
			s_jl_usb_suppress_until_unplug = 1;
	}

	divoom_jl_usb_insert = statu;

	if (!statu) {
		s_jl_usb_suppress_until_unplug = 0;
		s_jl_charge_tip_shown = 0;
		divoom_power_set_charge_sta(0);
		return;
	}

	divoom_power_set_charge_sta(1);
	if (statu && !prev_insert)
		divoom_power_try_show_charge_tip();
}

void divoom_power_set_charge(uint8 statu)
{
	divoom_power_set_charge_sta(statu);
}

int divoom_is_in_charge(void)
{
    return divoom_in_charge_sta;
}

uint8 divoom_is_charge_sta(void)
{
    return divoom_in_charge_sta;
}

void divoom_power_chg_full_cb(int statu)
{
	static int divoom_power_chg_full_frist = 1;

	if (divoom_in_charge_sta) {
		divoom_in_charge_full = statu;
		if (statu == 1 && divoom_power_chg_full_frist == 0)
			divoom_lvgl_top_battery_menu_init(2);
		divoom_power_chg_full_frist = 0;
		return;
	}
	divoom_in_charge_full = 0;
	divoom_power_chg_full_frist = 0;
}

#ifndef DIVOOM_PRODUCT_ASTRO_TOO
/* TimesVibe：PF5/USB12V-DET，inserted=0 表示 12V 快充口接入 */
void divoom_power_fast_chg_det_cb(int inserted)
{
	static int divoom_fast_chg_frist = 1;

	divoom_power_set_charge_sta(inserted ? 0 : 1);

	if (divoom_fast_chg_frist == 0 && divoom_in_charge_sta)
		divoom_power_try_show_charge_tip();
	if (divoom_fast_chg_frist == 1 && divoom_in_charge_sta)
		s_jl_usb_suppress_until_unplug = 1;
	divoom_fast_chg_frist = 0;
}
#endif

void divoom_power_on_jl_bat_volt_avr(uint16 bat_mv, uint16 usb_mv)
{
	s_jl_usb_volt_mv = usb_mv;
	if (bat_mv > 0)
		LdoinLevelAverage = bat_mv;
	if (divoom_in_charge_sta)
		divoom_power_update_battery_status_from_level((uint8)PowerLevel);
}

uint16 divoom_power_get_jl_usb_volt(void)
{
	return s_jl_usb_volt_mv;
}

void divoom_power_monitor_boot_guard_init(void)
{
    s_boot_start_ms = get_system_tick_ms();
    s_boot_power_on_over = 0;
}

void divoom_power_monitor_set_boot_complete(void)
{
    s_boot_power_on_over = 1;
}

static int divoom_power_should_defer_low_power_off(void)
{
    uint32 now;

    if (divoom_lvgl_poweron_should_defer_connect_tone())
        return 1;

    if (s_boot_power_on_over)
        return 0;

    if (s_boot_start_ms == 0)
        divoom_power_monitor_boot_guard_init();

    now = get_system_tick_ms();
    if ((now - s_boot_start_ms) < DIVOOM_EARLY_BOOT_GUARD_MS)
        return 1;

    return 0;
}

void PowerMonitorInit(void)
{
    TimeOutSet(&PowerMonitorTimer, 0);
    divoom_power_monitor_boot_guard_init();
    DivoomBatteryStatus = PWR_MNT_HIGH_V;
    PowerLevel = PWR_LEVEL_FULL;
    divoom_phone_pwr_level = PWR_LEVEL_FULL;
    divoom_powerled_level = PWR_LEVEL_FULL;
    divoom_powerled_level_pre = PWR_LEVEL_FULL;
    s_jl_bat_level_valid = 0;
    // s_low_batt_dimmed = 0;

	divoom_chg_full_init(divoom_power_chg_full_cb);
#ifndef DIVOOM_PRODUCT_ASTRO_TOO
	divoom_fast_chg_hotplug_init(divoom_power_fast_chg_det_cb);
#endif
}

uint8 PowerMonitor(void)
{
    if (PowerMonitorTimer.IsTimeOut)
        TimeOutSet(&PowerMonitorTimer, 1000);
    return DivoomBatteryStatus;
}

uint8 PowerVolIsLowPower(void)
{
    return (DivoomBatteryStatus == PWR_MNT_LOW_V || DivoomBatteryStatus == PWR_MNT_OFF_V) ? 1 : 0;
}

uint16 PowerVolGet(void)
{
    return LdoinLevelAverage;
}

uint8 PowerLevelLedGet(void)
{
    return divoom_powerled_level;
}

int divoom_get_cur_battery_level(void)
{
	/* 未收到 0x55 前不要默认满电，否则开机先画出满格 */
	if (!s_jl_bat_level_valid)
		return 0;
    return divoom_phone_pwr_level;
}

int divoom_power_phone_to_ui_bar(int phone)
{
	if (phone < 0)
		phone = 0;
	if (phone > 9)
		phone = 9;
	return phone / 2 + 1;
}

int divoom_power_use_charge_battery_art(int phone, int charging)
{
	if (charging)
		return 1;
	/* 放电 1 绿：素材只有充电 1 格是绿的 */
	return (phone == 1) ? 1 : 0;
}

void divoom_power_monitor_event_to_user(uint16 event)
{
    if (event != DIVOOM_MSG_LOW_POWER_OFF && event != DIVOOM_MSG_LOW_POWER)
        return;

    /* 开机动画期间忽略欠压关机，JL 后续会再发 0x33 */
    if (event == DIVOOM_MSG_LOW_POWER_OFF &&
        divoom_power_should_defer_low_power_off())
    {
        divoom_print("power: ignore low power off during early boot\n");
        return;
    }

    /* 充电中仍 skip 低电弹窗；欠压关机不 skip（电脑 USB 充不进也会掉到 6600） */
    if (event == DIVOOM_MSG_LOW_POWER_OFF)
        DivoomBatteryStatus = PWR_MNT_OFF_V;
    else
        DivoomBatteryStatus = PWR_MNT_LOW_V;
    if (divoom_in_charge_sta && event != DIVOOM_MSG_LOW_POWER_OFF)
        return;

    if (event == DIVOOM_MSG_LOW_POWER_OFF) {
        divoom_power_post_battery_event(BATTERY_STAT_LOW_SHUTDOWN);
        return;
    }

    divoom_power_post_low_battery_warn();
}

void divoom_power_monitor_mainloop(void)
{
    Time_mainloop(&PowerMonitorTimer, 10);
    (void)PowerMonitor();
    /* 低电/关机事件由 JL 经 DM_BTCORE_REMIND_BAT_LOW_POWER(0x33) 推送 */
}

uint8 Poweroff_charger_PowerMonitor(void)
{
    return divoom_in_charge_sta ? 1 : 0;
}

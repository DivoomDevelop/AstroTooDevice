#ifndef __divoom_onoff_h
#define __divoom_onoff_h

/** 开机源：失败或尚未 check_poweron 为 UNKNOWN；RTC_WAKE 表示 RTC 到点唤醒（用户闹钟或定时开机再看 wake_kind） */
#define DIVOOM_ONOFF_BOOT_UNKNOWN	(-1)
#define DIVOOM_ONOFF_BOOT_NORMAL	0
#define DIVOOM_ONOFF_BOOT_RTC_WAKE	1

/** 请求关机：走静默关机（存配置、写唤醒、发 0x19），供 main 收到 KEY_POWEROFF 时调用 */
void divoom_onoff_request_shutdown(void);

/** 用当前系统时间同步 RTC（NTP/MQTT 校时后调用）。@return 0 成功，-1 失败 */
int divoom_onoff_sync_rtc_from_system(void);

/** InitV3 拿到服务器时间后调用：本机次只向 JL SET 一次；失败则下次 InitV3 再试 */
int divoom_onoff_sync_rtc_after_server_time(void);

/** 设置里手改日期/时间：视为权威时间并立即推给 JL */
void divoom_onoff_on_user_set_time(void);

/** 时区已加载（或确认无 tzcode）；可把缓存的 JL 本地日历 mktime 进系统钟 */
void divoom_onoff_on_timezone_ready(void);

/** JL 在 CHECK_LINK 后主动推的 0x36 本地日历；未权威校时时缓存/采用 */
void divoom_onoff_on_jl_system_time(uint16 year, uint8 month, uint8 day,
				    uint8 hour, uint8 minute, uint8 second);

/** 关机写最近 RTC 唤醒；权威或有唤醒时整次关机只 SET 一次 0x36（产测不走此路径） */
void divoom_onoff_program_earliest_rtc_wake(void);

/** 本次启动若为定时开机 RTC 唤醒，返回 power_on_info 槽位索引，否则 -1 */
int divoom_onoff_get_power_schedule_wake_slot_index(void);
void divoom_onoff_clear_power_schedule_wake_slot_index(void);

void divoom_onoff_set_next_alarm_wakeup_timer(void);
void divooom_onoff_set_power(uint8 power_status);
void divoom_onoff_set_sleep(void);
int divoom_onoff_check_poweron(void);

/** 经 BtCore 查询 JL 是否产测开机；返回 DIVOOM_ONOFF_MODE_NORMAL 或 DIVOOM_ONOFF_MODE_TEST */
int divoom_onoff_query_jl_product_test_boot(void);

/* Prog 刷写前写入一次性产测入口；重启时读取并删除。 */
int divoom_onoff_arm_upgrade_aging_boot(void);
int divoom_onoff_arm_product_test_menu_boot(void);
void divoom_onoff_clear_upgrade_aging_boot(void);
int divoom_onoff_take_product_test_boot(uint8 *boot_mode);

/** 最近一次 divoom_onoff_check_poweron 的开机源，见 DIVOOM_ONOFF_BOOT_* */
int divoom_onoff_get_last_boot_source(void);

/*****************************************************************************
 Prototype    : divoom_onoff_poweroff
 Description  : 关机处理
 Input        : uint8 mode  0：按键关机
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2020/8/12
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_onoff_poweroff(uint8 mode);

/** 不播关机提示音：正常关机 main_exit 后发 JL 0x19；产测只发 0x19 */
void divoom_onoff_poweroff_no_tone(void);

/** 关机流程进行中（按键/JL 0x19 或助眠静默关机） */
int divoom_onoff_is_poweroff_active(void);

/** 关机提示音未能正常播完（入队失败、开播失败、被 stop 打断等），仍执行断电 */
void divoom_onoff_poweroff_tone_aborted(void);

#endif


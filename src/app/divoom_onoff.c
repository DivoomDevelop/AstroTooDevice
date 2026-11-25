#include "divoom.h"
#include <time.h>
#include <string.h>
#if defined(_WIN32)
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif
#include "divoom_alarm.h"
#include "divoom_power_control.h"
#include "divoom_screen_driver.h"
#include "divoom_port_fs.h"
#include "divoom_boot_health.h"
static int s_divoom_power_schedule_wake_index = -1;

static const uint8 s_product_test_boot_magic[8] = {
	'D', 'V', 'A', 'G', 1, 0, 0, 0
};

static int divoom_onoff_arm_product_test_boot(uint8 boot_mode)
{
	void *fp;
	uint8 marker[sizeof(s_product_test_boot_magic)];
	size_t written;

	memcpy(marker, s_product_test_boot_magic, sizeof(marker));
	marker[4] = boot_mode;
	divoom_port_remove_file(DIVOOM_PRODUCT_TEST_AGING_BOOT_FLAG_TMP);
	fp = divoom_port_file_open(DIVOOM_PRODUCT_TEST_AGING_BOOT_FLAG_TMP, "wb");
	if (!fp)
		return 0;
	written = divoom_port_file_write(marker, 1, sizeof(marker), fp);
	divoom_port_file_close(fp);
	if (written != sizeof(marker)
		|| divoom_port_sync() != 0
		|| divoom_port_rename_file(DIVOOM_PRODUCT_TEST_AGING_BOOT_FLAG_TMP,
			DIVOOM_PRODUCT_TEST_AGING_BOOT_FLAG) != 0
		|| divoom_port_sync() != 0) {
		divoom_port_remove_file(DIVOOM_PRODUCT_TEST_AGING_BOOT_FLAG_TMP);
		return 0;
	}
	divoom_print("product_test: boot marker armed mode=%u: %s\n",
		(unsigned)boot_mode, DIVOOM_PRODUCT_TEST_AGING_BOOT_FLAG);
	return 1;
}

int divoom_onoff_arm_upgrade_aging_boot(void)
{
	return divoom_onoff_arm_product_test_boot(
		DIVOOM_PRODUCT_TEST_BOOT_UPGRADE_AGING);
}

int divoom_onoff_arm_product_test_menu_boot(void)
{
	return divoom_onoff_arm_product_test_boot(DIVOOM_PRODUCT_TEST_BOOT_NORMAL);
}

void divoom_onoff_clear_upgrade_aging_boot(void)
{
	divoom_port_remove_file(DIVOOM_PRODUCT_TEST_AGING_BOOT_FLAG_TMP);
	divoom_port_remove_file(DIVOOM_PRODUCT_TEST_AGING_BOOT_FLAG);
	(void)divoom_port_sync();
}

int divoom_onoff_take_product_test_boot(uint8 *boot_mode)
{
	void *fp;
	uint8 marker[sizeof(s_product_test_boot_magic)];
	uint8 extra;
	size_t read_len;
	int valid;

	fp = divoom_port_file_open(DIVOOM_PRODUCT_TEST_AGING_BOOT_FLAG, "rb");
	if (!fp)
		return 0;
	read_len = divoom_port_file_read(marker, 1, sizeof(marker), fp);
	valid = read_len == sizeof(marker)
		&& divoom_port_file_read(&extra, 1, 1, fp) == 0
		&& memcmp(marker, s_product_test_boot_magic, 4) == 0
		&& (marker[4] == DIVOOM_PRODUCT_TEST_BOOT_NORMAL
		    || marker[4] == DIVOOM_PRODUCT_TEST_BOOT_UPGRADE_AGING)
		&& marker[5] == 0 && marker[6] == 0 && marker[7] == 0;
	divoom_port_file_close(fp);
	divoom_onoff_clear_upgrade_aging_boot();
	if (!valid)
		divoom_print("product_test: invalid boot marker removed\n");
	else {
		if (boot_mode)
			*boot_mode = marker[4];
		divoom_print("product_test: boot marker consumed mode=%u\n",
			(unsigned)marker[4]);
	}
	return valid;
}

static void divoom_onoff_persist_wake_kind(char kind, int power_index)
{
	void *fp;
	unsigned char buf[2];

	if (kind != 'A' && kind != 'P') {
		divoom_port_remove_file(DIVOOM_WAKE_KIND_PATH);
		return;
	}
	fp = divoom_port_file_open(DIVOOM_WAKE_KIND_PATH, "wb");
	if (!fp)
		return;
	buf[0] = (unsigned char)kind;
	if (kind == 'P' && power_index >= 0 && power_index < DIVOOM_POWER_CTRL_NUM)
		buf[1] = (unsigned char)power_index;
	else
		buf[1] = 0;
	(void)divoom_port_file_write(buf, 1, 2, fp);
	divoom_port_file_close(fp);
	(void)divoom_port_sync();
}

#define DIVOOM_ONOFF_TAG					"DIVOOM_ONOFF"
#define DIVOOM_ONOFF_PRINT			divoom_print

//触发电平
#define		DIVOOM_ONOFF_CHECK_PIN_LEV			(0)

static int s_divoom_last_boot_source = DIVOOM_ONOFF_BOOT_UNKNOWN;
static volatile uint8 s_poweroff_active = 0;
static volatile uint8 s_poweroff_done = 0;

/* 本机次与 JL 对时：JL 推本地日历作离线种子，服务器/手改为权威 */
typedef struct _divoom_onoff_jl_time_sync {
	uint8 tz_ready;
	uint8 authoritative;
	uint8 server_pushed_to_jl;
	uint8 jl_pending;
	uint16 year;
	uint8 month;
	uint8 day;
	uint8 hour;
	uint8 minute;
	uint8 second;
} divoom_onoff_jl_time_sync;
static divoom_onoff_jl_time_sync s_jl_time_sync;

int divoom_onoff_is_poweroff_active(void)
{
	return s_poweroff_active ? 1 : 0;
}

static int divoom_onoff_poweroff_try_begin(void)
{
	if (s_poweroff_active)
		return 0;
	s_poweroff_active = 1;
	return 1;
}

static void divoom_onoff_shutdown_hardware(void)
{
	/* 发杰理 0x19 前走 close_flow；已关过则内部直接返回 */
    if (divoom_port_sync() == 0) (void)divoom_boot_health_clean();
    divoom_os_com_lcd_disable();
    divoom_btcore_uart_cmd_send(DM_BTCORE_SYSTEM_POWER_OFF, NULL, 0);
}

static void divoom_onoff_poweroff_complete(void)
{
	if (!s_poweroff_active || s_poweroff_done)
		return;
	s_poweroff_done = 1;
	divoom_onoff_shutdown_hardware();
}

void divoom_onoff_poweroff_tone_aborted(void)
{
	divoom_print("Onoff: poweroff tone lost, shutdown anyway\n");
	divoom_onoff_poweroff_complete();
}

static int divoom_onoff_poweroff_tone_cb(char *music_name, int status)
{
	(void)music_name;
	divoom_print("Onoff: poweroff tone over status=%d\n", status);
	divoom_onoff_poweroff_complete();
	return 0;
}

static void divoom_onoff_poweroff_play_tone(void)
{
	char file_name[128] = {0};
    int volume = divoom_main_get_tone_volume();
#ifdef DIVOOM_POWEROFF_TONE_VOL_PERCENT
	volume = volume * DIVOOM_POWEROFF_TONE_VOL_PERCENT / 100;
#endif
	/* 视觉灭屏即可；close_flow 与 I2S 重叠会把关机音卡成两截 */
	divoom_screen_bright(0);
	divoom_voice_read_get_filename(DIVOOM_VOICE_ID_POWEROFF, file_name);
	if (!divoom_local_music_ctrl_tone_play(file_name, volume,divoom_onoff_poweroff_tone_cb, 0)) {
		divoom_onoff_poweroff_tone_aborted();
		return;
	}
}

void divoom_onoff_poweroff_no_tone(void)
{
	if (!divoom_onoff_poweroff_try_begin())
		return;
	divoom_print("DIVOOM_POWER_OFF (no tone)\n");
	/* 产测关机：不播提示音、不保存任何配置，只通知杰理断电 */
	if (divoom_product_test_is_active()) {
		divoom_onoff_shutdown_hardware();
		return;
	}
	divoom_main_exit();
	divoom_onoff_poweroff_complete();
}

void divoom_onoff_poweroff(uint8 mode)
{
	(void)mode;
	if (!divoom_onoff_poweroff_try_begin())
		return;
	/*
	 * 真正断电在提示音结束后的 divoom_onoff_shutdown_hardware
	 * （先 close_flow 再发 0x19）。产测不走 main_exit。
	 */
	divoom_print("DIVOOM_POWER_OFF\n");
	if (divoom_product_test_is_active()) {
		divoom_btcore_uart_cmd_send_disable_alarm_time();
		divoom_onoff_poweroff_play_tone();
		return;
	}
	/* JL 按键关机：先把关屏标记改为开屏，再保存供下次开机恢复。 */
	if (divoom_light_app_get_screen_status(0xFF) == 0)
	{
		divoom_light_app_get_screen_status(1);
		divoom_light_app_load_save();
	}
	divoom_main_exit();
	divoom_onoff_poweroff_play_tone();
}

/**
 * 请求关机：走静默关机（存配置、写唤醒、发 0x19）。
 * 供 main 收到 DIVOOM_MAIN_EVENT_KEY_POWEROFF 时调用（低电关机等）。
 */
void divoom_onoff_request_shutdown(void)
{
	divoom_onoff_poweroff_no_tone();
}

/**
 * 用当前系统时间同步 RTC（解决 RTC 时间不对问题）。
 * 应在系统时间已校准时调用（如 NTP/MQTT 校时后）。
 * @return 0 成功，-1 失败
 */
int divoom_onoff_sync_rtc_from_system(void)
{
	if (divoom_btcore_uart_cmd_send_set_system_time_now() != 0) {
		DIVOOM_ONOFF_PRINT("[RTC] sync from system to btcore failed\n");
		return -1;
	}
	DIVOOM_ONOFF_PRINT("[RTC] sync from system to btcore ok\n");
	return 0;
}

#define DIVOOM_ONOFF_JL_TIME_YEAR_MIN  2020u
#define DIVOOM_ONOFF_JL_TIME_YEAR_MAX  2037u

static int divoom_onoff_jl_cal_valid(uint16 year, uint8 month, uint8 day,
				     uint8 hour, uint8 minute, uint8 second)
{
	if (year < DIVOOM_ONOFF_JL_TIME_YEAR_MIN || year > DIVOOM_ONOFF_JL_TIME_YEAR_MAX)
		return 0;
	if (month < 1 || month > 12)
		return 0;
	if (day < 1 || day > 31)
		return 0;
	if (hour > 23 || minute > 59 || second > 59)
		return 0;
	return 1;
}

static int divoom_onoff_apply_jl_local_cal(void)
{
	struct tm tm_info;
	time_t sec;
	struct timeval tv;

	memset(&tm_info, 0, sizeof(tm_info));
	tm_info.tm_year = (int)s_jl_time_sync.year - 1900;
	tm_info.tm_mon = (int)s_jl_time_sync.month - 1;
	tm_info.tm_mday = (int)s_jl_time_sync.day;
	tm_info.tm_hour = (int)s_jl_time_sync.hour;
	tm_info.tm_min = (int)s_jl_time_sync.minute;
	tm_info.tm_sec = (int)s_jl_time_sync.second;
	tm_info.tm_isdst = -1;
	sec = mktime(&tm_info);
	if (sec == (time_t)-1)
		return -1;
	tv.tv_sec = sec;
	tv.tv_usec = 0;
	if (settimeofday(&tv, NULL) != 0)
		return -1;
	divoom_system_init_system_time();
	return 0;
}

static void divoom_onoff_try_apply_jl_time(void)
{
	if (!s_jl_time_sync.jl_pending || !s_jl_time_sync.tz_ready
	    || s_jl_time_sync.authoritative)
		return;
	if (divoom_onoff_apply_jl_local_cal() != 0) {
		DIVOOM_ONOFF_PRINT("[RTC] apply JL time failed %04u-%02u-%02u %02u:%02u:%02u\n",
				   (unsigned)s_jl_time_sync.year,
				   (unsigned)s_jl_time_sync.month,
				   (unsigned)s_jl_time_sync.day,
				   (unsigned)s_jl_time_sync.hour,
				   (unsigned)s_jl_time_sync.minute,
				   (unsigned)s_jl_time_sync.second);
		return;
	}
	s_jl_time_sync.jl_pending = 0;
	DIVOOM_ONOFF_PRINT("[RTC] apply JL time %04u-%02u-%02u %02u:%02u:%02u\n",
			   (unsigned)s_jl_time_sync.year,
			   (unsigned)s_jl_time_sync.month,
			   (unsigned)s_jl_time_sync.day,
			   (unsigned)s_jl_time_sync.hour,
			   (unsigned)s_jl_time_sync.minute,
			   (unsigned)s_jl_time_sync.second);
}

int divoom_onoff_sync_rtc_after_server_time(void)
{
	s_jl_time_sync.authoritative = 1;
	s_jl_time_sync.jl_pending = 0;
	if (s_jl_time_sync.server_pushed_to_jl)
		return 0;
	if (divoom_onoff_sync_rtc_from_system() != 0)
		return -1;
	s_jl_time_sync.server_pushed_to_jl = 1;
	return 0;
}

void divoom_onoff_on_user_set_time(void)
{
	s_jl_time_sync.authoritative = 1;
	s_jl_time_sync.jl_pending = 0;
	(void)divoom_onoff_sync_rtc_from_system();
}

void divoom_onoff_on_timezone_ready(void)
{
	s_jl_time_sync.tz_ready = 1;
	divoom_onoff_try_apply_jl_time();
}

void divoom_onoff_on_jl_system_time(uint16 year, uint8 month, uint8 day,
				    uint8 hour, uint8 minute, uint8 second)
{
	if (divoom_product_test_is_active())
		return;
	if (!divoom_onoff_jl_cal_valid(year, month, day, hour, minute, second)) {
		DIVOOM_ONOFF_PRINT("[RTC] ignore invalid JL time %04u-%02u-%02u %02u:%02u:%02u\n",
				   (unsigned)year, (unsigned)month, (unsigned)day,
				   (unsigned)hour, (unsigned)minute, (unsigned)second);
		return;
	}
	if (s_jl_time_sync.authoritative) {
		DIVOOM_ONOFF_PRINT("[RTC] ignore JL time (authoritative already)\n");
		return;
	}
	s_jl_time_sync.year = year;
	s_jl_time_sync.month = month;
	s_jl_time_sync.day = day;
	s_jl_time_sync.hour = hour;
	s_jl_time_sync.minute = minute;
	s_jl_time_sync.second = second;
	s_jl_time_sync.jl_pending = 1;
	DIVOOM_ONOFF_PRINT("[RTC] cache JL time %04u-%02u-%02u %02u:%02u:%02u tz_ready=%u\n",
			   (unsigned)year, (unsigned)month, (unsigned)day,
			   (unsigned)hour, (unsigned)minute, (unsigned)second,
			   (unsigned)s_jl_time_sync.tz_ready);
	divoom_onoff_try_apply_jl_time();
}

static int divoom_onoff_write_rtc_wake(time_t t_hw)
{
	struct tm tm_a;
	divoom_btcore_alarm_time_t alarm_time;
	int full_year;
	unsigned char mon;

	if (localtime_r(&t_hw, &tm_a) == NULL)
		return -1;

	mon = (unsigned char)(tm_a.tm_mon + 1);
	if (mon > 12)
		mon = 12;
	full_year = tm_a.tm_year + 1900;

	alarm_time.year = (uint16_t)full_year;
	alarm_time.month = mon;
	alarm_time.day = (unsigned char)tm_a.tm_mday;
	alarm_time.hour = (unsigned char)(tm_a.tm_hour <= 23 ? tm_a.tm_hour : 0);
	alarm_time.minute = (unsigned char)(tm_a.tm_min <= 59 ? tm_a.tm_min : 0);
	alarm_time.second = (unsigned char)(tm_a.tm_sec <= 59 ? tm_a.tm_sec : 0);
	if (divoom_btcore_uart_cmd_send_set_alarm_time(&alarm_time) != 0) {
		DIVOOM_ONOFF_PRINT("%s: btcore set alarm failed\n", __func__);
		return -1;
	}
	DIVOOM_ONOFF_PRINT("%s: btcore wake %04d-%02d-%02d %02d:%02d:%02d\n", __func__,
		full_year, (int)alarm_time.month, (int)alarm_time.day,
		(int)alarm_time.hour, (int)alarm_time.minute, (int)alarm_time.second);
	return 0;
}

void divoom_onoff_program_earliest_rtc_wake(void)
{
	time_t now, t_best, ta, tp;
	int tp_idx = -1;
	int best_is_alarm = 0;

	ta = divoom_alarm_get_next_rtc_wake_unix();
	tp = (time_t)divoom_power_control_get_next_power_on_unix_and_index(&tp_idx);

	now = time(NULL);
	t_best = 0;
	if (ta > now && (t_best == 0 || ta < t_best)) {
		t_best = ta;
		best_is_alarm = 1;
	}
	if (tp > now && (t_best == 0 || tp < t_best)) {
		t_best = tp;
		best_is_alarm = 0;
	}

	/* 权威必须写回；有唤醒也要先对表再写闹钟。整次关机只 SET 一次。产测不走 main_exit。 */
	if (s_jl_time_sync.authoritative || t_best != 0) {
		if (divoom_onoff_sync_rtc_from_system() != 0 && t_best != 0) {
			DIVOOM_ONOFF_PRINT("%s: SET JL time failed, skip rtc wake\n", __func__);
			divoom_onoff_persist_wake_kind(0, -1);
			divoom_btcore_uart_cmd_send_disable_alarm_time();
			return;
		}
	}

	if (t_best == 0) {
		DIVOOM_ONOFF_PRINT("%s: no future wake (alarm=%lld power_on=%lld)\n", __func__,
			(long long)ta, (long long)tp);
		divoom_onoff_persist_wake_kind(0, -1);
		divoom_btcore_uart_cmd_send_disable_alarm_time();
		return;
	}
	divoom_onoff_write_rtc_wake(t_best);

	/* 'A' 用户闹钟，'P' 定时开机（RTC 只有一个唤醒槽，开机后再读此标记） */
	if (best_is_alarm)
		divoom_onoff_persist_wake_kind('A', -1);
	else
		divoom_onoff_persist_wake_kind('P', tp_idx);
}

int divoom_onoff_get_power_schedule_wake_slot_index(void)
{
	return s_divoom_power_schedule_wake_index;
}

void divoom_onoff_clear_power_schedule_wake_slot_index(void)
{
	s_divoom_power_schedule_wake_index = -1;
}

/**
 * 获取开机源：经 DM_BTCORE_GET_BOOT_SOURCE 向 JL 查询，0=正常开机，1=闹钟唤醒。
 * @return 0 成功，-1 失败
 */
static int divoom_onoff_get_boot_source(int *out_source)
{
	int ret;
	uint8_t recv = 0;
	uint16_t ret_len = 0;

	if (out_source == NULL)
		return -1;
	*out_source = -1;
	if (!divoom_btcore_uart_cmd_is_ready())
		return -1;

	/* UART 刚 init 时 JL 可能尚未就绪，稍等再查 */
	divoom_port_delay_ms((200 * 1000) / 1000U);

	ret = divoom_btcore_uart_cmd_send_wait(DM_BTCORE_GET_BOOT_SOURCE, NULL, 0,
					      &recv, 1, &ret_len);
	if (ret != 0 || ret_len < 1u)
		return -1;

	*out_source = (int)recv;
	return 0;
}


/**
 * *  NAME          divoom_onoff_check_wakeup_from_timer
 * *  @brief       	判断是否闹钟开机
 * *  @param[in] 	none 
 * *  @author       wangft                                   
 * *  @return       none
 * */
uint8 divoom_onoff_check_wakeup_from_timer(void)
{
	return 0;
}

/**
 * *  NAME          divoom_onoff_set_timer_wakeup
 * *  @brief       	为什么要设置nvs，因为每次开机之后上次设置的开机闹钟就失效了，但是此时设备要进入睡眠模式，闹钟模块并没有初始化，不能从闹钟模块
 					直接读取下次闹钟时间
 * *  @param[in] 	none 
 * *  @author       wangft                                   
 * *  @return       none
 * */
void divoom_onoff_set_timer_wakeup(void)
{
}

/**
 * *  NAME          divoom_onoff_set_next_alarm_wakeup_timer
 * *  @brief       	设置关机闹钟，先获取下一个闹钟时间，再写进nvs，最后设置闹钟
 					直接读取下次闹钟时间
					从alarm获取下一个的闹钟的utc时间，减去当前utc时间（time(0)），得到时间差值
					如果取到的值小于等于当前time(0),需要重新取
					需要根据alarm模块进行调整
					这个函数在什么时候调用呢？按键关机时调用？
 * *  @param[in] 	none 
 * *  @author       wangft                                   
 * *  @return       none
 * */
void divoom_onoff_set_next_alarm_wakeup_timer(void)
{
	//这个部分的逻辑还要调整,先获取，然后存到nvs，再返回需要的值
	uint32 next_alarm = 0;
	divoom_sys_msg_info model_msg = {0};
	model_msg.src_id = DIVOOM_MODEL_ONOFF;
	model_msg.dest_id = DIVOOM_MODEL_ALARM;
	model_msg.msg_id = DIVOOM_ALARM_GET_NEXT_ALARM_TIME;
	divoom_sys_model_crl_msg_push(&model_msg);
	next_alarm = model_msg.result;

	if (next_alarm != 0)
	{
		next_alarm = next_alarm - time(0);
	}
    divoom_print("device will wake up after %ds, cur time : %d\n", next_alarm, (int)time(0));
	//divoom_onoff_set_timer_wakeup();
}

void divooom_onoff_set_power(uint8 power_status)
{
}



//设置ext1唤醒源的rtc gpio
//只能是rtc gpio,否则运行出错:红色Not an RTC IO: GPIO错误信息,但是编译不会出错
void divoom_onoff_set_rtcgpio_wakeup(uint32 key_gpio)
{
	//esp_sleep_enable_ext1_wakeup((1ULL << key_gpio), (DIVOOM_ONOFF_CHECK_PIN_LEV)?ESP_EXT1_WAKEUP_ANY_HIGH:ESP_EXT1_WAKEUP_ALL_LOW);
}


void divoom_onoff_start_sleep(void)
{
}

void divoom_onoff_set_sleep(void)
{
}



static T_BOOL poweron_check_syskey(T_U8 type)
{
    return AK_TRUE;
}


int divoom_onoff_check_powerkey_is_poweron(void)
{
    return 1;
}


/*****************************************************************************
 Prototype    : divoom_onoff_checkkey_set_sleep
 Description  : 开机检测开机键长按时间不够后调用此函数进sleep
 Input        : void  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2020/10/12
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_onoff_checkkey_set_sleep(void)
{
}



int divoom_onoff_query_jl_product_test_boot(void)
{
	uint8_t boot_mode = DIVOOM_BTCORE_BOOT_MODE_NORMAL;

	if (!divoom_btcore_uart_cmd_is_ready()) {
		if (divoom_btcore_uart_cmd_init(NULL) != 0)
			return DIVOOM_ONOFF_MODE_NORMAL;
	}

	divoom_port_delay_ms(200);

	if (divoom_btcore_uart_cmd_query_boot_mode(&boot_mode,
			DIVOOM_BTCORE_BOOT_MODE_QUERY_TIMEOUT_S) != 0)
		return DIVOOM_ONOFF_MODE_NORMAL;

	if (boot_mode == DIVOOM_BTCORE_BOOT_MODE_PRODUCT_TEST) {
		DIVOOM_ONOFF_PRINT("jl boot mode: product test\n");
		return DIVOOM_ONOFF_MODE_TEST;
	}
	return DIVOOM_ONOFF_MODE_NORMAL;
}

//等待检测够3秒
//默认返回 DIVOOM_ONOFF_MODE_NORMAL；RTC 闹钟唤醒时根据关机前写入文件区分 divoom_alarm / 定时开机
//需要增加开机检测按键是否按下开机
int divoom_onoff_check_poweron(void)
{
	int boot_source = 0;
	int ret_mode = DIVOOM_ONOFF_MODE_NORMAL;

	divoom_app_com_mk_dir(DIVOOM_CONFIG_DIR);
	divoom_app_com_mk_dir(DIVOOM_SD_SYSTEM_DIR);
	s_divoom_power_schedule_wake_index = -1;

	if (divoom_onoff_get_boot_source(&boot_source) != 0)
		boot_source = -1;
	if (boot_source == DIVOOM_BTCORE_BOOT_SOURCE_NORMAL)
		s_divoom_last_boot_source = DIVOOM_ONOFF_BOOT_NORMAL;
	else if (boot_source == DIVOOM_BTCORE_BOOT_SOURCE_ALARM)
		s_divoom_last_boot_source = DIVOOM_ONOFF_BOOT_RTC_WAKE;
	else
		s_divoom_last_boot_source = DIVOOM_ONOFF_BOOT_UNKNOWN;

	switch (boot_source) {
		case DIVOOM_BTCORE_BOOT_SOURCE_NORMAL:
			DIVOOM_ONOFF_PRINT("boot_source: normal power-on\n");
			break;
		case DIVOOM_BTCORE_BOOT_SOURCE_ALARM:
			DIVOOM_ONOFF_PRINT("boot_source: rtc wake-up\n");
			break;
		default:
			DIVOOM_ONOFF_PRINT("boot_source: %d / unknown\n", boot_source);
			break;
	}

	if (boot_source == DIVOOM_BTCORE_BOOT_SOURCE_ALARM) {
		void *fp = divoom_port_file_open(DIVOOM_WAKE_KIND_PATH, "rb");
		char buf[2] = {0};
		size_t n = 0;

		if (fp) {
			n = divoom_port_file_read(buf, 1, 2, fp);
			divoom_port_file_close(fp);
		}
		divoom_port_remove_file(DIVOOM_WAKE_KIND_PATH);
		if (n >= 1 && buf[0] == 'A') {
			ret_mode = DIVOOM_ONOFF_MODE_CLOCK;
			DIVOOM_ONOFF_PRINT("wake kind: divoom_alarm (CLOCK)\n");
		} else if (n >= 1 && buf[0] == 'P') {
			ret_mode = DIVOOM_ONOFF_MODE_POWER_SCHEDULE;
			if (n >= 2)
				s_divoom_power_schedule_wake_index = (unsigned char)buf[1];
			DIVOOM_ONOFF_PRINT("wake kind: power_schedule slot %d\n", s_divoom_power_schedule_wake_index);
		} else {
			ret_mode = DIVOOM_ONOFF_MODE_NORMAL;
			DIVOOM_ONOFF_PRINT("wake kind: unknown marker, default NORMAL\n");
		}
	}

	return ret_mode;
}

int divoom_onoff_get_last_boot_source(void)
{
	return s_divoom_last_boot_source;
}

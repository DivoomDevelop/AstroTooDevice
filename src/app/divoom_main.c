#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "divoom.h"
#include "divoom_lyric_demand.h"
#include "divoom_port_os.h"
#include "divoom_product_test_format.h"
#if DIVOOM_BUILD_RTOS_DEVICE
#include "divoom_ota_rtos.h"
#include <env.h>
#endif
#include "divoom_lan_storage.h"
#include "divoom_boot_health.h"
#include "divoom_device_error.h"
#if DIVOOM_BUILD_RTOS_DEVICE && defined(CONFIG_DRIVERS_AIC8800D80)
#include "divoom_recovery_integrity.h"
#endif
#if DIVOOM_BUILD_RTOS_DEVICE
#include <divoom_crash_log.h>
#ifdef CONFIG_DIVOOM_CRASH_LOG
const uint32_t divoom_crash_software_version = DIVOOM_SOFTWARE_VERSION;
#endif
#endif
void divoom_main_upoad_err_file(void);
#include "cJSON.h"
#include "divoom_store_clock_prefetch.h"
#include "divoom_mqtt_model.h"
#include "divoom_http_server.h"
#include "divoom_onoff.h"
#include "divoom_idle_poweroff.h"
#include "divoom_show_mode_ctrl.h"
#include "divoom_clock_component_index.h"
#ifdef DIVOOM_TIME_FRAME_SPEAKER
#include "divoom_power_control.h"
#endif
#include "divoom_bt_music.h"
#include "divoom_radio.h"
#ifdef DIVOOM_USE_RK_IC
#include "rk_mpi.h"
#include <turbojpeg.h>
#include "jpeglib.h"
#include <rga/im2d.h>
#include <rga/rga.h>
#include <rga/RgaApi.h>
#include "utility.h"
#else
#ifdef DIVOOM_SIMULATOR
#include "sim_aic_wifi.h"
#else
#include "aic_wifi.h"
#endif
#include <setjmp.h>
#include <time.h>
#include <signal.h>
#if defined(__has_include)
#  if __has_include(<execinfo.h>)
#    include <execinfo.h>
#    define DIVOOM_HAVE_EXECINFO 1
#  endif
#endif
#ifndef DIVOOM_HAVE_EXECINFO
#  define DIVOOM_HAVE_EXECINFO 0
#endif
#endif
#include <malloc.h>    
#include <stdlib.h>
#include <math.h>
#include "divoom_menu_control.h" 
#include "divoom_product_test_ui.h"
#include "divoom_lvgl_poweron.h"
#include "divoom_jl_amp_adapter.h"
#include "divoom_lvgl_top_main_menu.h"
#include "divoom_lvgl_base_dial_menu.h"
#include "divoom_lvgl_top_ambient_menu.h"
#include "divoom_lvgl_wifi_disconnect.h"
#include "divoom_lvgl_top_bright_volume_menu.h"
#include "divoom_lvgl_top_reached_home_menu.h"
#include "divoom_lvgl_power_off_disp_menu.h"
#include "divoom_lvgl_top_anniversary_reminder_menu.h"
#include "divoom_lvgl_top_danmaku_menu.h"
#include "divoom_lvgl_top_social_reminder_menu.h"
#include "divoom_ble_accept_data.h"
#include "divoom_lvgl_top_audio_tip_menu.h"
#include "divoom_lvgl_top_battery_menu.h"
#include "divoom_white_noise_player.h"
#include "divoom_lyric_play_ctrl.h"
#include "divoom_clock_favorite.h"
#include "divoom_sleep_aid_ctrl.h"
#include "divoom_game_runtime.h"
#include "divoom_sls_log.h"
#include "divoom_astrootoo_settings.h"
#include "divoom_astrootoo_settings_wifi.h"
#include "divoom_light_app.h"

#ifdef DIVOOM_SIMULATOR
#include "sim_pal.h"
void sim_movie_process_pending_callback(void);
#endif

#if defined(__has_include)
#  if __has_include(<curl/curl.h>)
#    include <curl/curl.h>
#    define DIVOOM_HAVE_LIBCURL 1
#  endif
#endif
#ifndef DIVOOM_HAVE_LIBCURL
#  define DIVOOM_HAVE_LIBCURL 0
#endif


//


#define DIVOOM_MAIN_SEND_APP_TRY_TIME     2

#ifdef DIVOOM_TIME_FRAME_SPEAKER
/*
 * 软音量：与 /etc/asound.conf 里 softvol 的 control.name 一致；声卡号随枚举变化。
 * 若 amixer 报「Cannot find the given element」：在设备上执行
 *   for i in 0 1 2 3; do echo "=== card $i"; amixer -c $i controls | grep -i soft; done
 * 用查到的 card 与 name 通过编译选项覆盖，例如：
 *   -DDIVOOM_ALSA_SOFTVOL_CARD=0 -DDIVOOM_ALSA_SOFTVOL_ELEM=\"PCM\"
 * 「Operation not permitted」多为权限或该 control 只读，需 root/udev 或换可写控件。
 */
#ifndef DIVOOM_ALSA_SOFTVOL_CARD
#define DIVOOM_ALSA_SOFTVOL_CARD 2
#endif
#ifndef DIVOOM_ALSA_SOFTVOL_ELEM
#define DIVOOM_ALSA_SOFTVOL_ELEM  "Soft Volume Master"
#endif
//#define DIVOOM_AMP_ACM8625P
#endif

/*
 * Temporary soak test: once Wi-Fi is connected, wait 10 seconds and then
 * inject one physical-equivalent DOWN short press every second.
 * Set to 0 after the dial-switch crash has been located.
 */
#ifndef DIVOOM_DIAL_SWITCH_AUTOTEST
#define DIVOOM_DIAL_SWITCH_AUTOTEST 0
#endif
#define DIVOOM_DIAL_SWITCH_AUTOTEST_START_DELAY_SEC 10U
#define DIVOOM_DIAL_SWITCH_AUTOTEST_PERIOD_SEC      1U

#define DIVOOM_VISUAL_CROSSING_WEATHER_URL \
    "https://weather.visualcrossing.com/VisualCrossingWebServices/rest/services/timeline/%s,%s" \
    "?unitGroup=metric&include=current,days,hours&contentType=json" \
    "&elements=datetime,datetimeEpoch,temp,tempmax,tempmin,humidity,pressure,visibility,windspeed,icon,sunriseEpoch,sunsetEpoch"
/* Open-Meteo 备用来源，风速使用 m/s。 */
#define DIVOOM_OPEN_METEO_WEATHER_URL "https://api.open-meteo.com/v1/forecast?latitude=%s&longitude=%s&current=temperature_2m,relative_humidity_2m,weather_code,surface_pressure,wind_speed_10m,is_day&hourly=visibility,temperature_2m,weather_code&wind_speed_unit=ms&daily=sunrise,sunset,weather_code,temperature_2m_max,temperature_2m_min&timezone=auto"
/* OpenWeather 备用 API (当 Open-Meteo 失败时使用) */
#define DIVOOM_DISP_WEATHER_URL_INFO "http://wea.divoom-gz.com/OpenWeatherMapCache.php?Lang=zh&Units=metric&Query[lon]=%s&Query[lat]=%s"
#define DIVOOM_DISP_WEATHER_WEEK_URL_INFO "http://wea.divoom-gz.com/OpenWeatherMapForecastCache.php?Lang=zh&Units=metric&Query[lon]=%s&Query[lat]=%s"

enum
{
    DIVOOM_MAIN_AUTO_POWER_OFF_NEVER,
    DIVOOM_MAIN_AUTO_POWER_OFF_HALF_HOUR,
    DIVOOM_MAIN_AUTO_POWER_OFF_ONE_HOUR,
    DIVOOM_MAIN_AUTO_POWER_OFF_THREE_HOUR,
    DIVOOM_MAIN_AUTO_POWER_OFF_SIX_HOUR,
    DIVOOM_MAIN_AUTO_POWER_OFF_TWELVE_HOUR,
};
static Pdivoom_sys_ctrl_handle pDivoomHandle = NULL;
static int s_boot_ab_recovery_needed;

#if DIVOOM_BUILD_RTOS_DEVICE
static void divoom_main_retry_boot_ab_recovery(void)
{
    static pthread_mutex_t retry_lock = PTHREAD_MUTEX_INITIALIZER;
    static uint32 last_request_ms;
    static int requested;
    uint32 now;
    int send_request = 0;

    if (!s_boot_ab_recovery_needed || !divoom_net_mqtt_get_net_status() ||
        !divoom_net_app_net_is_ok() || divoom_ota_server_get_update_status())
        return;
    now = divoom_system_get_tickms();
    pthread_mutex_lock(&retry_lock);
    if (!requested || (uint32)(now - last_request_ms) >= 60000U) {
        requested = 1;
        last_request_ms = now;
        send_request = 1;
    }
    pthread_mutex_unlock(&retry_lock);
    if (send_request) {
        divoom_print("[BOOT AB] requesting firmware for recovery\n");
        divoom_main_check_deivce_update_version();
    }
}
#endif

int divoom_main_get_boot_ab_recovery_needed(void)
{
    return s_boot_ab_recovery_needed;
}

static void divoom_main_read_boot_ab_recovery(void)
{
#if DIVOOM_BUILD_RTOS_DEVICE
    const char *value;
    int failed_slot = 0, running_slot = 0, clear_written = 0;
    extern int divoom_boot_ab_recovery_pending(void);
    extern int divoom_boot_ab_current_verified(void);
    extern const char *divoom_boot_ab_running_partition(void);

    s_boot_ab_recovery_needed = divoom_boot_ab_recovery_pending();
    if (fw_env_open() != 0) {
        divoom_print("[BOOT AB] cannot read recovery marker\n");
        return;
    }
    value = fw_getenv("rtos_ab_failure");
    if (value && !strcmp(value, "bootA")) failed_slot = 1;
    else if (value && !strcmp(value, "bootB")) failed_slot = 2;
    s_boot_ab_recovery_needed |= failed_slot != 0;
    if (failed_slot) {
        value = divoom_boot_ab_running_partition();
        if (value && !strcmp(value, "bootA")) running_slot = 1;
        else if (value && !strcmp(value, "bootB")) running_slot = 2;
        /* Clear only after the other slot actually reached divoom_main. */
        if (running_slot && running_slot != failed_slot &&
            divoom_boot_ab_current_verified() && !divoom_boot_ab_recovery_pending()) {
            clear_written = fw_env_write("rtos_ab_failure", "0") == 0 &&
                            fw_env_flush() == 0;
        }
    }
    fw_env_close();
    if (clear_written && fw_env_open() == 0) {
        value = fw_getenv("rtos_ab_failure");
        if (value && !strcmp(value, "0"))
            s_boot_ab_recovery_needed = 0;
        fw_env_close();
    }
    if (failed_slot && running_slot && running_slot != failed_slot &&
        s_boot_ab_recovery_needed && !divoom_boot_ab_recovery_pending())
        divoom_print("[BOOT AB] cannot verify recovery marker clear\n");
    if (s_boot_ab_recovery_needed)
        divoom_print("[BOOT AB] unverified startup, firmware recovery pending\n");
#endif
}
/* Only system_config.cfg and the local maintenance lifecycle share this lock.
 * Never hold it while stopping tasks, dispatching messages, or formatting. */
static pthread_mutex_t s_userdata_config_lock = PTHREAD_MUTEX_INITIALIZER;
static enum {
    DIVOOM_USERDATA_READY,
    DIVOOM_USERDATA_FORMATTING,
    DIVOOM_USERDATA_WAIT_REBOOT
} s_userdata_maintenance;

static int divoom_main_begin_userdata_maintenance(void)
{
    if (pthread_mutex_lock(&s_userdata_config_lock) != 0)
        return 0;
    if (s_userdata_maintenance != DIVOOM_USERDATA_READY)
    {
        pthread_mutex_unlock(&s_userdata_config_lock);
        return 0;
    }
    /* Acquiring the same lock drains an already running config write/close. */
    s_userdata_maintenance = DIVOOM_USERDATA_FORMATTING;
    pthread_mutex_unlock(&s_userdata_config_lock);
    return 1;
}

static void divoom_main_userdata_maintenance_returned(void)
{
    pthread_mutex_lock(&s_userdata_config_lock);
    /* A failed mkfs can already have unmounted userdata. Old RAM settings must
     * not recreate config after mkfs/fallback or an unexpectedly returning reboot. */
    s_userdata_maintenance = DIVOOM_USERDATA_WAIT_REBOOT;
    pthread_mutex_unlock(&s_userdata_config_lock);
    divoom_print("userdata maintenance returned; keep config/events paused until reboot\n");
}

static int divoom_main_ignore_userdata_event(int event_id)
{
    int busy;

    switch (event_id)
    {
        case DIVOOM_MAIN_EVENT_WIFI_CON:
        case DIVOOM_MAIN_EVENT_WIFI_DISCON:
        case DIVOOM_DEVICE_WIFI_CON_ID:
        case DIVOOM_DEVICE_WIFI_DISCON_ID:
        case DIVOOM_MAIN_EVENT_SRV_MQTT_CON:
        case DIVOOM_MAIN_EVENT_SRV_MQTT_DISCON:
        case DIVOOM_MAIN_EVENT_RETRY_MQTT_CONNECT:
        case DIVOOM_MAIN_NOTICE_RESET_APP_MQTT_CON:
        case DIVOOM_MAIN_EVENT_CONFIG_NET_WORK:
        case DIVOOM_MAIN_EVENT_CONFIG_NET_WORK_OVER:
        case DIVOOM_MAIN_EVENT_CONFIG_NET_WORK_CANCLE:
        case DIVOOM_APP_CONFIG_SAVE_EVENT_ID:
        case DIVOOM_MAIN_EVENT_SET_BETA_FEATURES:
        case DIVOOM_MAIN_EVENT_BETA_FEATURES_SAVED:
        case DIVOOM_MAIN_EVENT_NOTICE_UPDATE_DEVICE:
        case DIVOOM_MAIN_EVENT_CHECK_SYSTEM_FILES:
        case DIVOOM_MAIN_EVENT_FORMAT_USERDATA:
        case DIVOOM_MAIN_EVENT_RESET_FACTORY_SETTING:
        case DIVOOM_DEVICE_SYSTEM_RESET_ALL:
            break;
        default:
            return 0; /* Preserve shutdown/deinit acknowledgements and cleanup. */
    }
    if (pthread_mutex_lock(&s_userdata_config_lock) != 0)
        return 1;
    busy = s_userdata_maintenance != DIVOOM_USERDATA_READY;
    pthread_mutex_unlock(&s_userdata_config_lock);
    return busy;
}

static uint8 s_jl_get_update_pending;
static uint8 s_wifi_add_network_route_pending;
static volatile uint8 s_startup_dial_event_pending;
#ifdef DIVOOM_SIMULATOR
static uint8 s_sim_pending_wifi_mqtt_connect;
static uint32 s_sim_startup_dial_request_count;
#endif

static int divoom_main_apply_posix_timezone(const char *tzcode)
{
    if (tzcode == NULL || tzcode[0] == '\0')
        return -1;

#if defined(_WIN32) && defined(_MSC_VER)
    if (_putenv_s("TZ", tzcode) != 0)
        return -1;
    _tzset();
#else
    if (setenv("TZ", tzcode, 1) != 0)
        return -1;
    tzset();
#endif
    return 0;
}

void divoom_main_push_model_event_id(int model_id, uint8 event_id);

#ifdef DIVOOM_TIME_FRAME_SPEAKER
/* 闹钟唤醒在响铃前无页面时：按键取消等待并切表盘；已在响铃页则交给闹钟 UI */
int divoom_main_try_dismiss_clock_black_screen(void)
{
	divoom_app_schedule_item_info alarm_wait_cancel = {0};

	if (!pDivoomHandle || pDivoomHandle->poweron_mode != DIVOOM_ONOFF_MODE_CLOCK)
		return 0;
	if (divoom_menu_control_get_active_menu_id() == DIVOOM_MENU_CONTROL_ID_ALARM_ACTIVE_MENU)
		return 0;
	if (divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_ID_ALARM_ACTIVE_MENU)
	{
		alarm_wait_cancel.model_id = DIVOOM_MODEL_ALARM;
		alarm_wait_cancel.event_id = DIVOOM_ALARM_APP_EVENT_ALARM;
		alarm_wait_cancel.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
		alarm_wait_cancel.resource_type = (uint8)(DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE | DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE);
		divoom_app_schedule_cancel(&alarm_wait_cancel);
	}
	divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_START_UP_DISP_DIAL);
	divoom_alarm_clear_rtc_clock_wake_pending();
	divoom_main_finish_clock_wake_poweron();
	divoom_jl_release_boot_audio_gates_once();
	return 1;
}
#endif

extern uint8 divoom_ble_is_run(void);
void divoom_main_give_wait_download_msg(void);
void divoom_main_update_format_file(char *file_name, int file_type, int del_flag);

void divoom_main_save_config_local(void);

//暂停一些该暂停的模块的task,比如alarm等
void divoom_main_deinit_model(void);

void divoom_main_init_model(void);
int hex_char_to_int(char hex_char);

//获取IP地址与MAC地址
int divoom_main_get_device_mac_addr_ifconfig(uint8 *mac_addr, uint8 *ip4_addr)
{
#ifdef DIVOOM_SIMULATOR
    uint8 mac_tmp[6];
    uint8 ip_tmp[4];
    if (sim_net_get_mac_ip(mac_addr ? mac_addr : mac_tmp, ip4_addr ? ip4_addr : ip_tmp) == 0) {
        if (mac_addr) {
            divoom_print("macadd: %02x:%02x:%02x:%02x:%02x:%02x\n",
                mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        }
        if (ip4_addr) {
            divoom_print("ipaddr: %d.%d.%d.%d", ip4_addr[0], ip4_addr[1], ip4_addr[2], ip4_addr[3]);
        }
        return 1;
    }
    return 0;
#else
    int ret = -1;
    uint32 ip = 0;

    if (mac_addr)
    {
        memset(mac_addr, 0, AW_WIFI_MAC_LEN);
        if (aw_wifi_get_mac(AW_WIFI_IF_STA, mac_addr) == 0)
        {
            ret = 1;
        }
        else
        {
            divoom_print("it fails to get mac!\n");
        }
    }

    if (ip4_addr)
    {
        memset(ip4_addr, 0, 4);
        if (aw_wifi_get_ip(AW_WIFI_IF_STA, &ip, NULL, NULL) == 0)
        {
            ip4_addr[0] = (uint8)(ip & 0xff);
            ip4_addr[1] = (uint8)((ip >> 8) & 0xff);
            ip4_addr[2] = (uint8)((ip >> 16) & 0xff);
            ip4_addr[3] = (uint8)((ip >> 24) & 0xff);
            ret = 1;
        }
    }

    return ret;
#endif
}

/**
 * NAME         divoom_main_get_device_mac_addr
 * @BRIEF	    获取MAC 地址
 * @PARAM	    mac_addr 返回六个字节
                
                    
 * @RETURN	1：成功获取到，0：没有获取到
 * @RETVAL	
 */

int divoom_main_get_device_mac_addr(uint8 *mac_addr)
{
    return divoom_main_get_device_mac_addr_ifconfig(mac_addr, NULL);

}

char *divoom_main_get_blue_name(void)
{
	return pDivoomHandle->blue_info.blue_name;
}

int bt_status_test = 0, bt_device_num;
//蓝牙连接状态，1：非等待连接状态；0：等待连接
uint8 divoom_main_get_blue_con_status(void)
{
	return bt_status_test;
}

//蓝牙连接，1：连接；0：断开连接
void divoom_main_ctrl_blue_connect(uint8 con_flag)
{
    bt_status_test = con_flag;
    if(con_flag)
    {
        bt_device_num = 1;
    }
}

//清除所有蓝牙配对信息
void divoom_main_unpair_blue_device(void)
{
    bt_device_num = 0;
}

//有连接过蓝牙信息为1，没有被连接过为0
uint8 divoom_main_get_blue_device_num(void)
{
	return bt_device_num;
}

/* JL 名字段 26 字节含 '\0'，有效最长 25 */
#define DIVOOM_BT_NAME_WIRE_MAX 25
uint16 divoom_main_cut_blue_name(char *pInput, uint16 max);

static unsigned char divoom_main_bt_name_lower(unsigned char c)
{
	if (c >= 'A' && c <= 'Z')
		return (unsigned char)(c + 32);
	return c;
}

static int divoom_main_bt_name_starts_brand(const char *s, uint16 brand_len)
{
	uint16 i;
	const char *brand = DIVOOM_BLUE_BLE_DEVICE_NAME;

	if (s == NULL)
		return 0;
	for (i = 0; i < brand_len; i++)
	{
		if (s[i] == '\0')
			return 0;
		if (divoom_main_bt_name_lower((unsigned char)s[i]) !=
		    divoom_main_bt_name_lower((unsigned char)brand[i]))
			return 0;
	}
	return 1;
}

static void divoom_main_bt_name_trim_copy(const char *in, uint16 in_len, char *out, uint16 out_size)
{
	uint16 start = 0;
	uint16 end;
	uint16 n;

	if (out == NULL || out_size == 0)
		return;
	out[0] = '\0';
	if (in == NULL || in_len == 0)
		return;
	while (start < in_len && in[start] != '\0' && (unsigned char)in[start] <= ' ')
		start++;
	end = start;
	while (end < in_len && in[end] != '\0')
		end++;
	while (end > start && (unsigned char)in[end - 1] <= ' ')
		end--;
	n = (uint16)(end - start);
	if (n >= out_size)
		n = (uint16)(out_size - 1);
	if (n > 0)
		memcpy(out, in + start, n);
	out[n] = '\0';
}

/* 成品只允许 AstroToo 或 AstroToo-后缀，与 JL 25 字节上限对齐 */
static void divoom_main_normalize_blue_name(const char *in, uint16 in_len, char *out, uint16 out_size)
{
	char src[BT_NAME_MAX_LEN];
	char built[BT_NAME_MAX_LEN];
	const char *brand = DIVOOM_BLUE_BLE_DEVICE_NAME;
	const char *suffix;
	uint16 brand_len;
	uint16 wire_max;
	uint16 suffix_max;
	uint16 suffix_len;

	if (out == NULL || out_size == 0)
		return;

	memset(out, 0, out_size);
	brand_len = (uint16)strlen(brand);
	wire_max = DIVOOM_BT_NAME_WIRE_MAX;
	if ((uint16)(out_size - 1) < wire_max)
		wire_max = (uint16)(out_size - 1);

	memset(src, 0, sizeof(src));
	memset(built, 0, sizeof(built));
	divoom_main_bt_name_trim_copy(in, in_len, src, (uint16)sizeof(src));

	if (src[0] == '\0' || !divoom_main_bt_name_starts_brand(src, brand_len))
	{
		if (src[0] == '\0')
		{
			strncpy(out, brand, out_size - 1);
			return;
		}
		memcpy(built, brand, brand_len);
		built[brand_len] = '-';
		suffix_max = (uint16)(wire_max - brand_len - 1);
		suffix_len = divoom_main_cut_blue_name(src, suffix_max);
		memcpy(built + brand_len + 1, src, suffix_len);
		built[brand_len + 1 + suffix_len] = '\0';
	}
	else if (src[brand_len] == '\0')
	{
		strncpy(out, brand, out_size - 1);
		return;
	}
	else if (src[brand_len] == '-')
	{
		suffix = src + brand_len + 1;
		if (suffix[0] == '\0')
		{
			strncpy(out, brand, out_size - 1);
			return;
		}
		memcpy(built, brand, brand_len);
		built[brand_len] = '-';
		suffix_max = (uint16)(wire_max - brand_len - 1);
		suffix_len = divoom_main_cut_blue_name((char *)suffix, suffix_max);
		memcpy(built + brand_len + 1, suffix, suffix_len);
		built[brand_len + 1 + suffix_len] = '\0';
	}
	else
	{
		suffix = src + brand_len;
		memcpy(built, brand, brand_len);
		built[brand_len] = '-';
		suffix_max = (uint16)(wire_max - brand_len - 1);
		suffix_len = divoom_main_cut_blue_name((char *)suffix, suffix_max);
		memcpy(built + brand_len + 1, suffix, suffix_len);
		built[brand_len + 1 + suffix_len] = '\0';
	}

	if (strlen(built) == (size_t)brand_len + 1u && built[brand_len] == '-')
	{
		strncpy(out, brand, out_size - 1);
		return;
	}
	strncpy(out, built, out_size - 1);
}

uint16 divoom_main_cut_blue_name(char *pInput, uint16 max)
{
    int outputSize = 0, add_len;

    if (pInput == NULL || max == 0)
        return 0;

    while (*pInput && (outputSize < max))
    {
    	/*utf8 single char ,utf8 */
        add_len = 1;
        if (*pInput > 0x00 && *pInput <= 0x7F)
        {
        }
		/*utf8 double char ,utf8 */
        else if (((*pInput) & 0xE0) == 0xC0)
        {
            char low;
            
            pInput++;
            add_len = 2;
            low = *pInput;
			/*check if it is illegal*/
            if ((low & 0xC0) != 0x80)
            {
                return outputSize;
            }

        }
		/*utf8 triple char ,utf8 */
        else if (((*pInput) & 0xF0) == 0xE0)
        {
            pInput++;
            pInput++;
            add_len = 3;
         
        }
        else
        {
            return outputSize;
        }
        if(add_len + outputSize > max)
        {
            return outputSize;
        }
		pInput ++;
        outputSize += add_len;
    }
    return outputSize;
}

void divoom_main_set_blue_name(uint8 len, char *user_name)
{
	divoom_main_normalize_blue_name(user_name, len,
		pDivoomHandle->blue_info.blue_name,
		(uint16)sizeof(pDivoomHandle->blue_info.blue_name));
}

/* Weather state only. Do not hold this mutex during HTTP or MQTT dispatch. */
static pthread_mutex_t s_weather_lock = PTHREAD_MUTEX_INITIALIZER;
static uint32 s_weather_position_generation;
static uint8 s_weather_request_active;
static uint8 s_weather_mqtt_pending;
#define DIVOOM_WEATHER_MQTT_ALL_PENDING ((1U << DIVOOM_DISP_WEATHER_TODAY_INFO) | \
                                        (1U << DIVOOM_DISP_WEATHER_FURTHER_INFO))

uint8 divoom_main_set_device_position(char *lot, char *lat)
{
	if (strlen(lot) == 0 || strlen(lat) == 0)
	{
		return 0;
	}

    pthread_mutex_lock(&s_weather_lock);
    if(strcmp(pDivoomHandle->sys_setting.lot, lot) == 0 && strcmp(pDivoomHandle->sys_setting.lat, lat) == 0)
    {
        pthread_mutex_unlock(&s_weather_lock);
        return 0;
    }
    ++s_weather_position_generation;
    s_weather_mqtt_pending = 0;
    pDivoomHandle->update_flag = 1;
    pDivoomHandle->read_weather_time = 0;
    pDivoomHandle->weather_info.update_real_time = 0;
    pDivoomHandle->weather_info.update_future_time = 0;
    pDivoomHandle->sunrise_second = 0;
    pDivoomHandle->sunset_second = 0;
    strcpy(pDivoomHandle->sys_setting.lot, lot);
    strcpy(pDivoomHandle->sys_setting.lat, lat);
    pthread_mutex_unlock(&s_weather_lock);
    return 1;
}

void divoom_main_get_device_position(char *lot, char *lat)
{
    pthread_mutex_lock(&s_weather_lock);
    strcpy(lot, pDivoomHandle->sys_setting.lot);
    strcpy(lat, pDivoomHandle->sys_setting.lat);
    pthread_mutex_unlock(&s_weather_lock);
}

/*
https://openweathermap.org/weather-conditions

Day icon	Night icon	Description
01d.png  	01n.png  	clear sky
02d.png  	02n.png  	few clouds
03d.png  	03n.png  	scattered clouds
04d.png  	04n.png  	broken clouds
09d.png  	09n.png  	shower rain
10d.png  	10n.png  	rain
11d.png  	11n.png  	thunderstorm
13d.png  	13n.png  	snow
50d.png  	50n.png  	mist

*/

/* Open-Meteo WMO 天气代码映射到 DIVOOM 天气图标, is_day: 1=白天 0=夜晚 */
static T_U16 divoom_main_openmeteo_wmo_to_weather_pic(int wmo_code, int is_day)
{
    int day_suffix = is_day ? DIVOOM_DISP_CLOCK_WEATHER_IMAGE_SUNNY_DAY : DIVOOM_DISP_CLOCK_WEATHER_IMAGE_SUNNY_NIGHT;
    int night_suffix = is_day ? DIVOOM_DISP_CLOCK_WEATHER_IMAGE_CLOUDY_DAY : DIVOOM_DISP_CLOCK_WEATHER_IMAGE_CLOUDY_NIGHT;
    int rain_suffix = is_day ? DIVOOM_DISP_CLOCK_WEATHER_IMAGE_RAINY_DAY : DIVOOM_DISP_CLOCK_WEATHER_IMAGE_RAINY_NIGHT;
    int snow_suffix = is_day ? DIVOOM_DISP_CLOCK_WEATHER_IMAGE_SNOW_DAY : DIVOOM_DISP_CLOCK_WEATHER_IMAGE_SNOW_NIGHT;
    int fog_suffix = is_day ? DIVOOM_DISP_CLOCK_WEATHER_IMAGE_FROG_DAY : DIVOOM_DISP_CLOCK_WEATHER_IMAGE_FROG_NIGHT;

    if(wmo_code == 0) return day_suffix;
    if(wmo_code >= 1 && wmo_code <= 3) return night_suffix;  /* Mainly clear, partly cloudy, overcast */
    if(wmo_code == 45 || wmo_code == 48) return fog_suffix;   /* Fog */
    if((wmo_code >= 51 && wmo_code <= 67) || (wmo_code >= 80 && wmo_code <= 82)) return rain_suffix;  /* Rain/Drizzle */
    if((wmo_code >= 71 && wmo_code <= 77) || (wmo_code >= 85 && wmo_code <= 86)) return snow_suffix;  /* Snow */
    if(wmo_code >= 95 && wmo_code <= 99) return rain_suffix; /* Thunderstorm */
    return night_suffix;
}

/* 解析 Open-Meteo ISO8601 时间 "2026-02-28T06:54" 为 unix 时间戳 */
static int divoom_main_parse_iso8601_to_unix(const char *iso_str)
{
    int y, mo, d, hour = 0, min = 0;
    struct tm t = {0};
    if(!iso_str || sscanf(iso_str, "%d-%d-%dT%d:%d", &y, &mo, &d, &hour, &min) < 5) return 0;
    t.tm_year = y - 1900;
    t.tm_mon = mo - 1;
    t.tm_mday = d;
    t.tm_hour = hour;
    t.tm_min = min;
    t.tm_sec = 0;
    return (int)mktime(&t);
}

/* 解析 Open-Meteo 日期 "2026-02-28" 为 unix 时间戳(当日0点) */
static int divoom_main_parse_iso8601_date_to_unix(const char *date_str)
{
    int y, mo, d;
    struct tm t = {0};
    if(!date_str || sscanf(date_str, "%d-%d-%d", &y, &mo, &d) < 3) return 0;
    t.tm_year = y - 1900;
    t.tm_mon = mo - 1;
    t.tm_mday = d;
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;
    return (int)mktime(&t);
}

void divoom_main_get_weather_info(Pdivoom_disp_weather_info weather_info)
{
    pthread_mutex_lock(&s_weather_lock);
    memcpy(weather_info, &pDivoomHandle->weather_info, sizeof(divoom_disp_weather_info));
    pthread_mutex_unlock(&s_weather_lock);
}

void divoom_main_get_http_weather_info(void *data)
{
    Pdivoom_device_get_weather_info weather_info = (Pdivoom_device_get_weather_info)data;
    
    pthread_mutex_lock(&s_weather_lock);
    strcpy(weather_info->Weather, pDivoomHandle->weather_info.weather_str);
    weather_info->CurTemp = (float)DIVOOM_WEATHER_ROUND_TEMP(pDivoomHandle->weather_info.temp);
    weather_info->MinTemp = (float)DIVOOM_WEATHER_ROUND_TEMP(pDivoomHandle->weather_info.min_temp);
    weather_info->MaxTemp = (float)DIVOOM_WEATHER_ROUND_TEMP(pDivoomHandle->weather_info.max_temp);
    weather_info->Pressure = pDivoomHandle->weather_info.pressure;
    weather_info->Humidity = pDivoomHandle->weather_info.humidity;
    weather_info->Visibility = pDivoomHandle->weather_info.visibility;
    weather_info->WindSpeed = pDivoomHandle->weather_info.wind_speed;
    pthread_mutex_unlock(&s_weather_lock);
}

int divoom_main_get_temp_flag(void)
{
    return pDivoomHandle->sys_setting.temp_c_f_flag;
}

int divoom_main_get_battery_level(void)
{
#if CFG_FUNC_POWER_MONITOR_EN
	return divoom_get_cur_battery_level();
#else
    return 0;
#endif
}

int divoom_main_get_show_grid_flag(void)
{
	return pDivoomHandle->sys_setting.show_grid_flag;
}

void divoom_main_set_show_grid_flag(int flag)
{
	if(pDivoomHandle->sys_setting.show_grid_flag != flag)
    {
        pDivoomHandle->sys_setting.show_grid_flag = flag;
    }
}

int divoom_main_set_wifi_config_status(int mode)
{
    if(mode != -1)
    {
        pDivoomHandle->sys_setting.wifi_config_flag = mode;
        divoom_main_save_config_local();
    }
    return  pDivoomHandle->sys_setting.wifi_config_flag;
}

int divoom_main_set_lang_flag(int lang)
{
    if(lang != -1)
    {
        pDivoomHandle->sys_setting.lang = pDivoomHandle->device_china_flag
            ? DIVOOM_LVGL_MENU_LANG_SC : lang;
        divoom_main_save_config_local();
    }
    return pDivoomHandle->sys_setting.lang;
}

int divoom_main_get_dial_demo_flag(void)
{
	if(pDivoomHandle)
	{
		return pDivoomHandle->sys_setting.dial_demo_flag;
	}

	return 0;
}

void divoom_main_set_temp_flag(int mode)
{
    if(pDivoomHandle->sys_setting.temp_c_f_flag != mode)
    {
        pDivoomHandle->sys_setting.temp_c_f_flag = mode;
        divoom_main_save_notice_dis();
    }
}

void divoom_main_set_screen_dir(int mode)
{
    if(pDivoomHandle->sys_setting.screen_dir != mode)
    {
        pDivoomHandle->sys_setting.screen_dir = mode;
        divoom_screen_set_direction(pDivoomHandle->sys_setting.screen_dir);
    }
}

void divoom_main_set_mirror_flag(int mode)
{
    if(pDivoomHandle->sys_setting.mirror_flag != mode)
    {
        pDivoomHandle->sys_setting.mirror_flag = mode;
        divoom_screen_set_mirror_image(pDivoomHandle->sys_setting.mirror_flag);
    }
    
}

uint8 divoom_main_get_screen_switch(void)
{
    return pDivoomHandle->sys_setting.screen_switch;    
}
void divoom_main_save_config(void);

void divoom_main_set_screen_switch(int mode)
{
    if(pDivoomHandle->sys_setting.screen_switch != mode)
    {
        pDivoomHandle->sys_setting.screen_switch = mode;
        pDivoomHandle->update_flag = 1;
		divoom_main_save_config();
    }
    
}

void divoom_main_set_high_light_mode(int mode)
{
    divoom_screen_set_power_mode(mode);    
}

void divoom_main_play_buzzer(Pdivoom_devcice_play_buzzer_respone para)
{
}

void divoom_main_set_white_banlance(divoom_screen_color_info *para)
{
    
}

char *divoom_main_get_disp_title(uint8 type)
{
    return NULL;
}

void divoom_main_set_text_dir(void)
{
}

void divoom_main_out_disp_str(char *data,uint8 cyc_flag,void *callback)
{
}

void divoom_main_out_disp_str_remove_title(char *data,uint8 cyc_flag,void *callback)
{
}

/* 获取 cJSON 数组元素的字符串值 */
static const char* divoom_cjson_array_string(divoom_cJSON *arr, int idx)
{
    divoom_cJSON *item = divoom_cJSON_GetArrayItem(arr, idx);
    return (item && item->valuestring) ? item->valuestring : NULL;
}

static int divoom_main_weather_number(divoom_cJSON *object, const char *key,
    double minimum, double maximum, double *result)
{
    divoom_cJSON *item = divoom_cJSON_GetObjectItem(object, key);

    if (!item || (item->type & 0xff) != divoom_cJSON_Number ||
        !(item->valuedouble >= minimum && item->valuedouble <= maximum)) {
        return 0;
    }
    *result = item->valuedouble;
    return 1;
}

/* Open-Meteo 日出日落解析 - cJSON 数组元素直接是字符串 */
static void divoom_main_openmeteo_parse_sunrise_sunset(divoom_cJSON *root, divoom_disp_weather_info *weather)
{
    divoom_cJSON *daily = divoom_cJSON_GetObjectItem(root, "daily");
    if(!daily) return;
    divoom_cJSON *sunrise_arr = divoom_cJSON_GetObjectItem(daily, "sunrise");
    divoom_cJSON *sunset_arr = divoom_cJSON_GetObjectItem(daily, "sunset");
    if(sunrise_arr) {
        const char *s = divoom_cjson_array_string(sunrise_arr, 0);
        if(s) weather->sunrise_time = divoom_main_parse_iso8601_to_unix(s);
    }
    if(sunset_arr) {
        const char *s = divoom_cjson_array_string(sunset_arr, 0);
        if(s) weather->sunset_time = divoom_main_parse_iso8601_to_unix(s);
    }
}

/* 从 hourly 中按当前时间匹配能见度，current.time 与 hourly.time 对齐到小时 */
static void divoom_main_openmeteo_parse_visibility_from_hourly(divoom_cJSON *root, divoom_disp_weather_info *weather)
{
    divoom_cJSON *cur, *hourly, *time_arr, *vis_arr, *temp_arr, *weather_pic_arr;
    divoom_cJSON *cur_time_item;
    const char *cur_time;
    int i, n, is_day;

    cur = divoom_cJSON_GetObjectItem(root, "current");
    hourly = divoom_cJSON_GetObjectItem(root, "hourly");
    if(!cur || !hourly) return;
    cur_time_item = divoom_cJSON_GetObjectItem(cur, "time");
	is_day = divoom_cJSON_GetObjectInt(cur, "is_day");
    time_arr = divoom_cJSON_GetObjectItem(hourly, "time");
    vis_arr = divoom_cJSON_GetObjectItem(hourly, "visibility");
	temp_arr = divoom_cJSON_GetObjectItem(hourly, "temperature_2m");
	weather_pic_arr = divoom_cJSON_GetObjectItem(hourly, "weather_code");

    if(!cur_time_item || !cur_time_item->valuestring || !time_arr || !vis_arr || !temp_arr || !weather_pic_arr) return;

    cur_time = cur_time_item->valuestring;
    n = divoom_cJSON_GetArraySize(time_arr);

	weather->visibility = 10000;  /* 未匹配时默认 10km */
    /* 按当前时间匹配：比较 date+hour 部分(前13字符 "2026-02-28T02")，取对应小时的能见度 */
    for(i = 0; i < n; i++) {
        const char *hour_time = divoom_cjson_array_string(time_arr, i);
        if(hour_time && strncmp(hour_time, cur_time, 13) == 0) {
            divoom_cJSON *vis_item = divoom_cJSON_GetArrayItem(vis_arr, i);
            if(vis_item) {
                weather->visibility = (int)(vis_item->valuedouble + 0.5);
                //return;
				break;
            }
        }
    }

	for(i = 0; i < n && i < 48; i++) {
       divoom_cJSON *temp_item = divoom_cJSON_GetArrayItem(temp_arr, i);
       if(temp_item) {
           weather->hour_table[i].temp = (int)(temp_item->valuedouble + 0.5);
       }
    }

	for(i = 0; i < n && i < 48; i++) {
       divoom_cJSON *weather_pic_item = divoom_cJSON_GetArrayItem(weather_pic_arr, i);
       if(weather_pic_item) {
           weather->hour_table[i].weather_pic_id = divoom_main_openmeteo_wmo_to_weather_pic(weather_pic_item->valueint , is_day);
       }
    }
}

/* Open-Meteo 当前天气解析，成功返回1，失败返回0(将触发 DIVOOM 备用)，风速已用 wind_speed_unit=ms 获取 m/s */
static int divoom_main_parse_openmeteo_current(divoom_cJSON *root, divoom_disp_weather_info *weather)
{
    divoom_cJSON *cur = divoom_cJSON_GetObjectItem(root, "current");
    double humidity, pressure, code, daytime;
    int wmo_code, is_day;

    if (!divoom_main_weather_number(cur, "temperature_2m", -150, 100, &weather->temp) ||
        !divoom_main_weather_number(cur, "relative_humidity_2m", 0, 100, &humidity) ||
        !divoom_main_weather_number(cur, "surface_pressure", 1, 1200, &pressure) ||
        !divoom_main_weather_number(cur, "wind_speed_10m", 0, 300, &weather->wind_speed) ||
        !divoom_main_weather_number(cur, "weather_code", 0, 99, &code) ||
        !divoom_main_weather_number(cur, "is_day", 0, 1, &daytime)) {
        return 0;
    }
    weather->humidity = (int)humidity;
    weather->pressure = DIVOOM_WEATHER_ROUND_TEMP(pressure);
    wmo_code = (int)code;
    is_day = (int)daytime;

    weather->weather_pic_id = divoom_main_openmeteo_wmo_to_weather_pic(wmo_code, is_day);
    weather->min_temp = weather->temp;
    weather->max_temp = weather->temp;
    divoom_main_openmeteo_parse_visibility_from_hourly(root, weather);

    if(wmo_code == 0) strcpy(weather->weather_str, "Sunny");
    else if(wmo_code >= 1 && wmo_code <= 3) strcpy(weather->weather_str, "Cloudy");
    else if(wmo_code == 45 || wmo_code == 48) strcpy(weather->weather_str, "Fog");
    else if((wmo_code >= 51 && wmo_code <= 67) || (wmo_code >= 80 && wmo_code <= 82) || (wmo_code >= 95 && wmo_code <= 99)) strcpy(weather->weather_str, "Rainy");
    else if((wmo_code >= 71 && wmo_code <= 77) || (wmo_code >= 85 && wmo_code <= 86)) strcpy(weather->weather_str, "Snow");
    else strcpy(weather->weather_str, "Cloudy");

    divoom_main_openmeteo_parse_sunrise_sunset(root, weather);
    return 1;
}

/* Open-Meteo 周预报解析，成功返回1，失败返回0 */
static int divoom_main_parse_openmeteo_weekly(divoom_cJSON *root, divoom_disp_weather_info *weather)
{
    divoom_cJSON *daily, *time_arr, *code_arr, *max_arr, *min_arr;
    int i, number;
    const char *date_str;

    daily = divoom_cJSON_GetObjectItem(root, "daily");
    if(!daily) return 0;
    time_arr = divoom_cJSON_GetObjectItem(daily, "time");
    code_arr = divoom_cJSON_GetObjectItem(daily, "weather_code");
    max_arr = divoom_cJSON_GetObjectItem(daily, "temperature_2m_max");
    min_arr = divoom_cJSON_GetObjectItem(daily, "temperature_2m_min");
    if(!time_arr || !code_arr || !max_arr || !min_arr) return 0;

    number = divoom_cJSON_GetArraySize(time_arr);
    if(number <= 0) return 0;
    if(number > 40) number = 40;

    for(i = 0; i < number; i++) {
        divoom_disp_day_weather_info *forecast = &weather->futrue_table[i];
        divoom_cJSON *code_item = divoom_cJSON_GetArrayItem(code_arr, i);
        divoom_cJSON *max_item = divoom_cJSON_GetArrayItem(max_arr, i);
        divoom_cJSON *min_item = divoom_cJSON_GetArrayItem(min_arr, i);

        date_str = divoom_cjson_array_string(time_arr, i);
        forecast->utc_time = divoom_main_parse_iso8601_date_to_unix(date_str);
        if (forecast->utc_time <= 0 ||
            !code_item || (code_item->type & 0xff) != divoom_cJSON_Number ||
            !max_item || (max_item->type & 0xff) != divoom_cJSON_Number ||
            !min_item || (min_item->type & 0xff) != divoom_cJSON_Number ||
            max_item->valuedouble < min_item->valuedouble) {
            return 0;
        }
        forecast->weather_pic_id = divoom_main_openmeteo_wmo_to_weather_pic(code_item->valueint, 1);
        forecast->max_temp = max_item->valuedouble;
        forecast->min_temp = min_item->valuedouble;
    }
    return 1;
}

typedef struct
{
    divoom_weather_position_info position;
    uint32 generation;
    uint8 openmeteo_retried;
} divoom_weather_request;

static const char *divoom_main_weather_string(divoom_cJSON *object, const char *key)
{
    divoom_cJSON *item = divoom_cJSON_GetObjectItem(object, key);

    return item ? item->valuestring : NULL;
}

static int divoom_main_visual_weather_icon(divoom_cJSON *object, int is_day, const char **label)
{
    const char *icon = divoom_main_weather_string(object, "icon");
    int wmo;

    if (!icon || !*icon) {
        return -1;
    }
    if (strcmp(icon, "clear-day") == 0 || strcmp(icon, "clear-night") == 0) {
        wmo = 0;
        *label = "Sunny";
    } else if (strcmp(icon, "fog") == 0) {
        wmo = 45;
        *label = "Fog";
    } else if (strncmp(icon, "snow", 4) == 0 || strcmp(icon, "sleet") == 0) {
        wmo = 71;
        *label = "Snow";
    } else if (strstr(icon, "rain") || strstr(icon, "showers") ||
               strncmp(icon, "thunder", 7) == 0 || strcmp(icon, "hail") == 0) {
        wmo = 61;
        *label = "Rainy";
    } else {
        wmo = 3;
        *label = "Cloudy";
    }

    if (strstr(icon, "-night")) {
        is_day = 0;
    } else if (strstr(icon, "-day")) {
        is_day = 1;
    }
    return divoom_main_openmeteo_wmo_to_weather_pic(wmo, is_day);
}

static int divoom_main_visual_weather_hour(divoom_cJSON *object)
{
    const char *text = divoom_main_weather_string(object, "datetime");
    int hour, minute, second;

    if (!text || sscanf(text, "%2d:%2d:%2d", &hour, &minute, &second) != 3 ||
        hour < 0 || hour >= 24 || minute < 0 || minute >= 60 || second < 0 || second >= 60) {
        return -1;
    }
    return hour;
}

static int divoom_main_visual_weather_epoch(divoom_cJSON *object, const char *key)
{
    double epoch;

    return divoom_main_weather_number(object, key, 1, 2147483647, &epoch) ? (int)epoch : 0;
}

static int divoom_main_visual_weather_is_day(divoom_cJSON *object, int hour, int sunrise, int sunset)
{
    int epoch = divoom_main_visual_weather_epoch(object, "datetimeEpoch");

    if (epoch && sunrise && sunset > sunrise) {
        return epoch >= sunrise && epoch < sunset;
    }
    return hour >= 6 && hour < 18;
}

static int divoom_main_parse_visual_weather(divoom_cJSON *root, divoom_disp_weather_info *weather)
{
    divoom_cJSON *current = divoom_cJSON_GetObjectItem(root, "currentConditions");
    divoom_cJSON *days = divoom_cJSON_GetObjectItem(root, "days");
    uint8 hour_present[48] = {0};
    double humidity, pressure, visibility, speed;
    const char *label;
    int now_hour, icon, i;

    if (!current || !days || divoom_cJSON_GetArraySize(days) < 7 ||
        !divoom_main_weather_number(current, "temp", -150, 100, &weather->temp) ||
        !divoom_main_weather_number(current, "humidity", 0, 100, &humidity) ||
        !divoom_main_weather_number(current, "pressure", 1, 1200, &pressure) ||
        !divoom_main_weather_number(current, "visibility", 0, 1000, &visibility) ||
        !divoom_main_weather_number(current, "windspeed", 0, 1000, &speed)) {
        return 0;
    }

    now_hour = divoom_main_visual_weather_hour(current);
    if (now_hour < 0) {
        return 0;
    }
    weather->humidity = DIVOOM_WEATHER_ROUND_TEMP(humidity);
    weather->pressure = DIVOOM_WEATHER_ROUND_TEMP(pressure);
    weather->visibility = DIVOOM_WEATHER_ROUND_TEMP(visibility * 1000.0);
    weather->wind_speed = speed / 3.6;

    for (i = 0; i < 7; ++i) {
        divoom_cJSON *day = divoom_cJSON_GetArrayItem(days, i);
        divoom_disp_day_weather_info *forecast = &weather->futrue_table[i];
        double minimum, maximum;

        forecast->utc_time = divoom_main_parse_iso8601_date_to_unix(
            divoom_main_weather_string(day, "datetime"));
        icon = divoom_main_visual_weather_icon(day, 1, &label);
        if (forecast->utc_time <= 0 || (i > 0 && forecast[-1].utc_time >= forecast->utc_time) ||
            icon < 0 || !divoom_main_weather_number(day, "tempmin", -150, 100, &minimum) ||
            !divoom_main_weather_number(day, "tempmax", minimum, 100, &maximum)) {
            return 0;
        }
        forecast->min_temp = (float)minimum;
        forecast->max_temp = (float)maximum;
        forecast->weather_pic_id = icon;

        if (i < 2) {
            divoom_cJSON *hours = divoom_cJSON_GetObjectItem(day, "hours");
            divoom_cJSON *hour;
            int sunrise = divoom_main_visual_weather_epoch(day, "sunriseEpoch");
            int sunset = divoom_main_visual_weather_epoch(day, "sunsetEpoch");

            if (i == 0) {
                weather->min_temp = minimum;
                weather->max_temp = maximum;
                weather->sunrise_time = sunrise;
                weather->sunset_time = sunset;
            }
            /* The watchface looks up hours by local hour plus a day offset. */
            for (hour = hours ? hours->child : NULL; hour; hour = hour->next) {
                int slot = divoom_main_visual_weather_hour(hour);
                double temperature;

                icon = divoom_main_visual_weather_icon(hour,
                    divoom_main_visual_weather_is_day(hour, slot, sunrise, sunset), &label);
                if (slot < 0 || icon < 0 ||
                    !divoom_main_weather_number(hour, "temp", -150, 100, &temperature)) {
                    continue;
                }
                slot += i * 24;
                weather->hour_table[slot].temp = (float)temperature;
                weather->hour_table[slot].weather_pic_id = icon;
                hour_present[slot] = 1;
            }
        }
    }
    for (i = now_hour; i <= now_hour + 4; ++i) {
        if (!hour_present[i]) {
            return 0;
        }
    }

    icon = divoom_main_visual_weather_icon(current,
        divoom_main_visual_weather_is_day(current, now_hour, weather->sunrise_time, weather->sunset_time), &label);
    if (icon < 0) {
        return 0;
    }
    weather->weather_pic_id = (T_U16)icon;
    strcpy(weather->weather_str, label);
    return 1;
}

static int divoom_main_weather_request_current(const divoom_weather_request *request)
{
    int current;

    pthread_mutex_lock(&s_weather_lock);
    current = request->generation == s_weather_position_generation &&
              pDivoomHandle->device_wifi_status;
    pthread_mutex_unlock(&s_weather_lock);
    return current;
}

static int divoom_main_weather_finish(divoom_weather_request *request,
    divoom_disp_weather_info *weather, int use_mqtt)
{
    int adopted = 0;

    pthread_mutex_lock(&s_weather_lock);
    if (request->generation == s_weather_position_generation && pDivoomHandle->device_wifi_status) {
        if (weather) {
            weather->request_time = time(0);
            weather->update_real_time = weather->request_time + DIVOOM_MAIN_WEATHER_UPDATE_TIME;
            weather->update_future_time = weather->update_real_time;
            pDivoomHandle->weather_info = *weather;
            pDivoomHandle->sunrise_second = weather->sunrise_time;
            pDivoomHandle->sunset_second = weather->sunset_time;
            adopted = 1;
        } else if (use_mqtt) {
            pDivoomHandle->weather_info.update_real_time = time(0) + 60;
            pDivoomHandle->weather_info.update_future_time = pDivoomHandle->weather_info.update_real_time;
        }
    }
    pthread_mutex_unlock(&s_weather_lock);

    pthread_mutex_lock(&s_weather_lock);
    s_weather_request_active = 0;
    pthread_mutex_unlock(&s_weather_lock);
    divoom_system_free(request);
    return adopted;
}

static int divoom_main_queue_weather(divoom_weather_request *request, int visual_crossing);

static void divoom_main_weather_http_result(Pdivoom_web_get_msg_info msg,
    divoom_weather_request *request, int visual_crossing)
{
    divoom_disp_weather_info weather = {0};
    divoom_cJSON *root;
    int ok = 0;

    if (!divoom_main_weather_request_current(request)) {
        divoom_main_weather_finish(request, NULL, 0);
        return;
    }

    if (msg && msg->data && msg->len) {
        root = divoom_cJSON_ParseWithOpts(msg->data, NULL, 1);
        if (root && !divoom_cJSON_GetObjectItem(root, "error")) {
            if (visual_crossing) {
                ok = divoom_main_parse_visual_weather(root, &weather);
            } else {
                ok = divoom_main_parse_openmeteo_current(root, &weather) &&
                     divoom_main_parse_openmeteo_weekly(root, &weather);
            }
        }
        divoom_cJSON_Delete(root);
    }

    if (ok) {
        if (divoom_main_weather_finish(request, &weather, 0)) {
            divoom_print("Weather source: %s\n", visual_crossing ? "Visual Crossing" : "Open-Meteo");
        }
        return;
    }

    if (visual_crossing) {
        divoom_print("Visual Crossing failed; trying Open-Meteo\n");
        if (divoom_main_queue_weather(request, 0)) {
            return;
        }
    } else if ((!msg || !msg->data || !msg->len) && !request->openmeteo_retried) {
        request->openmeteo_retried = 1;
        if (divoom_main_queue_weather(request, 0)) {
            return;
        }
    }
    divoom_main_weather_finish(request, NULL, 1);
}

static void divoom_main_visual_weather_callback(Pdivoom_web_get_msg_info msg, void *para)
{
    divoom_main_weather_http_result(msg, para, 1);
}

static void divoom_main_openmeteo_weather_callback(Pdivoom_web_get_msg_info msg, void *para)
{
    divoom_main_weather_http_result(msg, para, 0);
}

static int divoom_main_queue_weather(divoom_weather_request *request, int visual_crossing)
{
    divoom_web_get_request_info url_info = {0};
    int written;

    if (!divoom_main_weather_request_current(request)) {
        return 0;
    }
    written = snprintf(url_info.url, sizeof(url_info.url),
        visual_crossing ? DIVOOM_VISUAL_CROSSING_WEATHER_URL : DIVOOM_OPEN_METEO_WEATHER_URL,
        request->position.lat, request->position.lot);
    if (written < 0 || (size_t)written >= sizeof(url_info.url)) {
        return 0;
    }
    url_info.para = request;
    url_info.mode = visual_crossing ? DIVOOM_WEB_GET_TYPE_GZIP_TEXT_BUF : DIVOOM_WEB_GET_TYPE_TEXT_BUF;
    url_info.pcallback = visual_crossing ? divoom_main_visual_weather_callback : divoom_main_openmeteo_weather_callback;
    return divoom_web_get_push(&url_info);
}

#if 1
void divoom_main_get_week_weather_info_callback(Pdivoom_web_get_msg_info pmsg, void *para)
{
    int i, number;
	divoom_cJSON *root = NULL, *list, *item, *tmp_obj;
    char url_addr[64];
    
    if(pmsg == NULL || pmsg->len == 0 || pmsg->data == NULL)
    {
        divoom_print("fails to read data:\n");
        return;
    }
    
	root = divoom_cJSON_Parse(pmsg->data);
	if (!root)
	{
        divoom_print("read data: ERR JSON\n");
		return ;
	}
    number = divoom_cJSON_GetObjectInt(root, "cnt");
    if(number <= 0)
    {
    	divoom_cJSON_Delete(root);
        return ;
    }
    if(number > 40)
    {
        number = 40;
    }
    
    list = divoom_cJSON_GetObjectItem(root, "list");
    if(list == NULL)
    {
        divoom_cJSON_Delete(root);
        return;
    }
    pDivoomHandle->weather_info.request_time = time(0);
	for(i = 0; i < number; i++)
	{
        item = divoom_cJSON_GetArrayItem(list, i);
        if(item == NULL)
        {
            break;
        }
        pDivoomHandle->weather_info.futrue_table[i].utc_time = divoom_cJSON_GetObjectInt(item, "dt");
        tmp_obj = divoom_cJSON_GetObjectItem(item, "main");
        if(tmp_obj )
        {
            pDivoomHandle->weather_info.futrue_table[i].min_temp = divoom_cJSON_GetObjectdouble(tmp_obj, "temp_min");
            pDivoomHandle->weather_info.futrue_table[i].max_temp = divoom_cJSON_GetObjectdouble(tmp_obj, "temp_max");
        }
        tmp_obj = divoom_cJSON_GetObjectItem(item, "weather");
        if(tmp_obj)
        {
            tmp_obj = divoom_cJSON_GetArrayItem(tmp_obj, 0);
            if(tmp_obj)
            {
                divoom_cJSON_GetObjectString(tmp_obj, "icon",url_addr);
                
                if(strcmp(url_addr, "01d") == 0)
                {
                    pDivoomHandle->weather_info.futrue_table[i].weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_SUNNY_DAY;
                }
                else if(strcmp(url_addr, "01n") == 0)
                {
                    pDivoomHandle->weather_info.futrue_table[i].weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_SUNNY_NIGHT;
                }
                else if(strcmp(url_addr, "02d") == 0)
                {
                    pDivoomHandle->weather_info.futrue_table[i].weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_CLOUDY_DAY;
                }
                else if(strcmp(url_addr, "02n") == 0)
                {
                    pDivoomHandle->weather_info.futrue_table[i].weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_CLOUDY_NIGHT;
                }
                else if(strcmp(url_addr, "03d") == 0)
                {
                    pDivoomHandle->weather_info.futrue_table[i].weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_CLOUDY_DAY;
                }
                else if(strcmp(url_addr, "03n") == 0)
                {
                    pDivoomHandle->weather_info.futrue_table[i].weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_CLOUDY_NIGHT;
                }
                else if(strcmp(url_addr, "04d") == 0)
                {
                    pDivoomHandle->weather_info.futrue_table[i].weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_CLOUDY_DAY;
                }
                else if(strcmp(url_addr, "04n") == 0)
                {
                    pDivoomHandle->weather_info.futrue_table[i].weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_CLOUDY_NIGHT;
                }
                else if(strcmp(url_addr, "09d") == 0)
                {
                    pDivoomHandle->weather_info.futrue_table[i].weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_RAINY_DAY;
                }
                else if(strcmp(url_addr, "09n") == 0)
                {
                    pDivoomHandle->weather_info.futrue_table[i].weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_RAINY_NIGHT;
                }
                else if(strcmp(url_addr, "10d") == 0)
                {
                    pDivoomHandle->weather_info.futrue_table[i].weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_RAINY_DAY;
                }
                else if(strcmp(url_addr, "10n") == 0)
                {
                    pDivoomHandle->weather_info.futrue_table[i].weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_RAINY_NIGHT;
                }
                else if(strcmp(url_addr, "11d") == 0)
                {
                    pDivoomHandle->weather_info.futrue_table[i].weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_RAINY_DAY;
                }
                else if(strcmp(url_addr, "11n") == 0)
                {
                    pDivoomHandle->weather_info.futrue_table[i].weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_RAINY_NIGHT;
                }
                else if(strcmp(url_addr, "13d") == 0)
                {
                    pDivoomHandle->weather_info.futrue_table[i].weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_SNOW_DAY;
                }
                else if(strcmp(url_addr, "13n") == 0)
                {
                    pDivoomHandle->weather_info.futrue_table[i].weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_SNOW_NIGHT;
                }
                else if(strcmp(url_addr, "50d") == 0)
                {
                    pDivoomHandle->weather_info.futrue_table[i].weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_FROG_DAY;
                }
                else if(strcmp(url_addr, "50n") == 0)
                {
                    pDivoomHandle->weather_info.futrue_table[i].weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_FROG_NIGHT;
                }
                else
                {
                    pDivoomHandle->weather_info.futrue_table[i].weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_SUNNY_DAY;
                }

            }
        }
        
	}

    pDivoomHandle->weather_info.request_weather_flag &= ~(1 << DIVOOM_DISP_WEATHER_FURTHER_INFO);

	divoom_cJSON_Delete(root);
    pDivoomHandle->weather_info.update_future_time = time(0) + DIVOOM_MAIN_WEATHER_UPDATE_TIME*2;
}
#endif
/*
01d.png  	01n.png  	clear sky       sunny
02d.png  	02n.png  	few clouds      Cloudy
03d.png  	03n.png  	scattered clouds Cloudy
04d.png  	04n.png  	broken clouds    Cloudy
09d.png  	09n.png  	shower rain      Rainy
10d.png  	10n.png  	rain             Rainy
11d.png  	11n.png  	thunderstorm     Rainy
13d.png  	13n.png  	snow             snow
50d.png  	50n.png  	mist             Frog
晴 多云 雨 雪 雾

*/

#if 1
void divoom_main_get_weather_info_callback(Pdivoom_web_get_msg_info pmsg, void *para)
{
	divoom_cJSON *root = NULL, *item;
    char *net_data; 
    char url_addr[64];
    
    if(pmsg == NULL || pmsg->len == 0 || pmsg->data == NULL)
    {
        divoom_print("fails to read data:\n");
        return;
    }
    net_data = pmsg->data;
    
    divoom_debug("read data:%s", net_data);
	root = divoom_cJSON_Parse(net_data);
	if (!root)
	{
        divoom_print("read data: ERR JSON\n");
		return ;
	}
    item = divoom_cJSON_GetObjectItem(root, "sys");
    if(item == NULL)
    {
        divoom_cJSON_Delete(root);
        return;
    }
    pDivoomHandle->sunrise_second = divoom_cJSON_GetObjectInt(item , "sunrise");
    pDivoomHandle->sunset_second = divoom_cJSON_GetObjectInt(item , "sunset");
    item = divoom_cJSON_GetObjectItem(root, "weather");
    if(item == NULL)
    {
        divoom_cJSON_Delete(root);
        return;
    }
    item = divoom_cJSON_GetArrayItem(item, 0);
    if(item == NULL)
    {
        divoom_cJSON_Delete(root);
        return;
    }
    divoom_cJSON_GetObjectString(item , "icon",url_addr);
    //将天气转换为
    if(strcmp(url_addr, "01d") == 0)
    {
        pDivoomHandle->weather_info.weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_SUNNY_DAY;
        strcpy(pDivoomHandle->weather_info.weather_str, "Sunny");
    }
    else if(strcmp(url_addr, "01n") == 0)
    {
        pDivoomHandle->weather_info.weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_SUNNY_NIGHT;
        strcpy(pDivoomHandle->weather_info.weather_str, "Sunny");
    }
    else if(strcmp(url_addr, "02d") == 0)
    {
        pDivoomHandle->weather_info.weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_CLOUDY_DAY;
        strcpy(pDivoomHandle->weather_info.weather_str, "Cloudy");
    }
    else if(strcmp(url_addr, "02n") == 0)
    {
        pDivoomHandle->weather_info.weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_CLOUDY_NIGHT;
        strcpy(pDivoomHandle->weather_info.weather_str, "Cloudy");
    }
    else if(strcmp(url_addr, "03d") == 0)
    {
        pDivoomHandle->weather_info.weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_CLOUDY_DAY;
        strcpy(pDivoomHandle->weather_info.weather_str, "Cloudy");
    }
    else if(strcmp(url_addr, "03n") == 0)
    {
        pDivoomHandle->weather_info.weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_CLOUDY_NIGHT;
        strcpy(pDivoomHandle->weather_info.weather_str, "Cloudy");
    }
    else if(strcmp(url_addr, "04d") == 0)
    {
        pDivoomHandle->weather_info.weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_CLOUDY_DAY;
        strcpy(pDivoomHandle->weather_info.weather_str, "Cloudy");
    }
    else if(strcmp(url_addr, "04n") == 0)
    {
        pDivoomHandle->weather_info.weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_CLOUDY_NIGHT;
        strcpy(pDivoomHandle->weather_info.weather_str, "Cloudy");
    }
    else if(strcmp(url_addr, "09d") == 0)
    {
        pDivoomHandle->weather_info.weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_RAINY_DAY;
        strcpy(pDivoomHandle->weather_info.weather_str, "Rainy");
    }
    else if(strcmp(url_addr, "09n") == 0)
    {
        pDivoomHandle->weather_info.weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_RAINY_NIGHT;
        strcpy(pDivoomHandle->weather_info.weather_str, "Rainy");
    }
    else if(strcmp(url_addr, "10d") == 0)
    {
        pDivoomHandle->weather_info.weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_RAINY_DAY;
        strcpy(pDivoomHandle->weather_info.weather_str, "Rainy");
    }
    else if(strcmp(url_addr, "10n") == 0)
    {
        pDivoomHandle->weather_info.weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_RAINY_NIGHT;
        strcpy(pDivoomHandle->weather_info.weather_str, "Rainy");
    }
    else if(strcmp(url_addr, "11d") == 0)
    {
        pDivoomHandle->weather_info.weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_RAINY_DAY;
        strcpy(pDivoomHandle->weather_info.weather_str, "Rainy");
    }
    else if(strcmp(url_addr, "11n") == 0)
    {
        pDivoomHandle->weather_info.weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_RAINY_NIGHT;
        strcpy(pDivoomHandle->weather_info.weather_str, "Rainy");
    }
    else if(strcmp(url_addr, "13d") == 0)
    {
        pDivoomHandle->weather_info.weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_SNOW_DAY;
        strcpy(pDivoomHandle->weather_info.weather_str, "Snow");
    }
    else if(strcmp(url_addr, "13n") == 0)
    {
        pDivoomHandle->weather_info.weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_SNOW_NIGHT;
        strcpy(pDivoomHandle->weather_info.weather_str, "Snow");
    }
    else if(strcmp(url_addr, "50d") == 0)
    {
        pDivoomHandle->weather_info.weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_FROG_DAY;
        strcpy(pDivoomHandle->weather_info.weather_str, "Fog");
    }
    else if(strcmp(url_addr, "50n") == 0)
    {
        pDivoomHandle->weather_info.weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_FROG_NIGHT;
        strcpy(pDivoomHandle->weather_info.weather_str, "Fog");
    }
    else
    {
        pDivoomHandle->weather_info.weather_pic_id = DIVOOM_DISP_CLOCK_WEATHER_IMAGE_SUNNY_DAY;
        strcpy(pDivoomHandle->weather_info.weather_str, "Sunny");
    }

    
    item = divoom_cJSON_GetObjectItem(root, "main");
    if(item == NULL)
    {
        divoom_cJSON_Delete(root);
        return;
    }
    pDivoomHandle->weather_info.temp = divoom_cJSON_GetObjectdouble(item, "temp");
    pDivoomHandle->weather_info.min_temp = divoom_cJSON_GetObjectdouble(item, "temp_min");
    pDivoomHandle->weather_info.max_temp = divoom_cJSON_GetObjectdouble(item, "temp_max");
    pDivoomHandle->weather_info.pressure = divoom_cJSON_GetObjectInt(item, "pressure");
    pDivoomHandle->weather_info.humidity = divoom_cJSON_GetObjectInt(item, "humidity");
    pDivoomHandle->weather_info.visibility = divoom_cJSON_GetObjectInt(root, "visibility");
    item = divoom_cJSON_GetObjectItem(root, "wind");
    if(item == NULL)
    {
        divoom_cJSON_Delete(root);
        return;
    }
    pDivoomHandle->weather_info.wind_speed = divoom_cJSON_GetObjectdouble(item, "speed");
    pDivoomHandle->weather_info.request_weather_flag &=  ~(1 << DIVOOM_DISP_WEATHER_TODAY_INFO);
	divoom_cJSON_Delete(root);
    pDivoomHandle->weather_info.update_real_time = time(0) + DIVOOM_MAIN_WEATHER_UPDATE_TIME*2;
}
#endif

/**
 * NAME         divoom_main_update_weather_info
 * @BRIEF	更新天气相关信息
 * @PARAM	flag
                    
 * @RETURN	none
 * @RETVAL	
 */


void divoom_main_update_weather_info(uint8 flag)
{
    divoom_weather_request *request = NULL;
    time_t now = time(0);

    pthread_mutex_lock(&s_weather_lock);
    if (pDivoomHandle->device_wifi_status && pDivoomHandle->sys_setting.lat[0] &&
        pDivoomHandle->sys_setting.lot[0] && !s_weather_request_active &&
        (flag || (pDivoomHandle->read_weather_time <= now &&
                  (pDivoomHandle->weather_info.update_real_time < now ||
                   pDivoomHandle->weather_info.update_future_time < now)))) {
        pDivoomHandle->read_weather_time = now + 10;
        request = divoom_system_malloc(sizeof(*request));
        if (request) {
            strcpy(request->position.lat, pDivoomHandle->sys_setting.lat);
            strcpy(request->position.lot, pDivoomHandle->sys_setting.lot);
            request->generation = s_weather_position_generation;
            request->openmeteo_retried = 0;
            s_weather_request_active = 1;
            s_weather_mqtt_pending = 0;
        }
    }
    pthread_mutex_unlock(&s_weather_lock);

    if (!request) {
        return;
    }
    if (!divoom_main_queue_weather(request, 1) && !divoom_main_queue_weather(request, 0)) {
        divoom_main_weather_finish(request, NULL, 1);
    }
}

/**
 * NAME         divoom_main_get_ftp_server_ip
 * @BRIEF	返回服务器FTP地址
 * @PARAM	
                    
 * @RETURN	
 * @RETVAL	
 */

char *divoom_main_get_ftp_server_ip(void)
{
    if(pDivoomHandle->sys_setting.server_flag == 0)
    {
        return DIVOOM_FTP_CHINA_IP;
    }
    else
    {
        return DIVOOM_FTP_USA_IP;
    }
}

/**
 * NAME         divoom_main_get_https_server_ip
 * @BRIEF	返回服务器https地址
 * @PARAM	
                    
 * @RETURN	
 * @RETVAL	
 */

char *divoom_main_get_https_server_ip(void)
{
    if(pDivoomHandle->sys_setting.server_flag == 0)
    {
        return DIVOOM_SERVER_CHINA_HTTPS_IP;
    }
    else
    {
        return DIVOOM_SERVER_USA_HTTPS_IP;
    }
}

/**
 * NAME         divoom_main_get_http_server_addr
 * @BRIEF	返回服务器命令地址
 * @PARAM	
                    
 * @RETURN	1:国外服务器，0：国内服务器
 * @RETVAL	
 */

char *divoom_main_get_http_server_addr(void)
{
    if(pDivoomHandle->use_ip_flag == 0)
    {
        if(pDivoomHandle->sys_setting.server_flag == 2)
        {
            return pDivoomHandle->use_https_flag?DIVOOM_SERVER_HTTPS_NORMAL_URL:DIVOOM_SERVER_HTTP_NORMAL_URL;
        }
        else if(pDivoomHandle->sys_setting.server_flag)
        {
            return pDivoomHandle->use_https_flag?DIVOOM_SERVER_HTTPS_USA_URL:DIVOOM_SERVER_HTTP_USA_URL;
        }
        else
        {
            return pDivoomHandle->use_https_flag?DIVOOM_SERVER_HTTPS_CHINA_URL:DIVOOM_SERVER_HTTP_CHINA_URL;
        }
    }
    else
    {
        if(pDivoomHandle->sys_setting.server_flag == 2)
        {
            return DIVOOM_SERVER_HTTP_NORMAL_IP;
        }
        else if(pDivoomHandle->sys_setting.server_flag)
        {
            return DIVOOM_SERVER_HTTP_USA_IP;
        }
        else
        {
            return DIVOOM_SERVER_HTTP_CHINA_IP;
        }
    }
}

/**
 * NAME         divoom_main_get_file_server_addr
 * @BRIEF	返回服务器文件地址
 * @PARAM	
                    
 * @RETURN	1:国外服务器，0：国内服务器
 * @RETVAL	
 */

char *divoom_main_get_file_server_addr(void)
{
    /* Factory test mode downloads resources before the main app exists. */
    if (!pDivoomHandle)
        return DIVOOM_SERVER_DEFAULT_URL;
    if(pDivoomHandle->use_ip_flag == 0)
    {
        if(pDivoomHandle->sys_setting.server_flag == 2)
        {
            return pDivoomHandle->use_https_flag?DIVOOM_SERVER_DEFAULT_URLS:DIVOOM_SERVER_DEFAULT_URL;
        }
        else if(pDivoomHandle->sys_setting.server_flag)
        {
            return pDivoomHandle->use_https_flag?DIVOOM_SERVER_USA_URLS:DIVOOM_SERVER_USA_URL;
        }
        else
        {
            return pDivoomHandle->use_https_flag?DIVOOM_SERVER_CHINA_URLS:DIVOOM_SERVER_CHINA_URL;
        }
    }
    else
    {
        if(pDivoomHandle->sys_setting.server_flag == 2)
        {
            return DIVOOM_SERVER_DEFAULT_IP;
        }
        else if(pDivoomHandle->sys_setting.server_flag)
        {
            return DIVOOM_SERVER_USA_IP;
        }
        else
        {
            return DIVOOM_SERVER_CHINA_IP;
        }
    }
}


/**
 * NAME         divoom_main_get_file_server_addr
 * @BRIEF	返回服务器文件地址
 * @PARAM	
                    
 * @RETURN	1:国外服务器，0：国内服务器
 * @RETVAL	
 */

char *divoom_main_get_file_other_server_addr(void)
{
    if(pDivoomHandle->use_ip_flag == 0)
    {
        if(pDivoomHandle->sys_setting.server_flag == 2)
        {
            return pDivoomHandle->use_https_flag?DIVOOM_SERVER_DEFAULT_URLS:DIVOOM_SERVER_DEFAULT_URL;
        }
        else if(pDivoomHandle->sys_setting.server_flag)
        {
            return pDivoomHandle->use_https_flag?DIVOOM_SERVER_CHINA_URLS:DIVOOM_SERVER_CHINA_URL;
        }
        else
        {
            return pDivoomHandle->use_https_flag?DIVOOM_SERVER_USA_URLS:DIVOOM_SERVER_USA_URL;
        }
    }
    else
    {
        if(pDivoomHandle->sys_setting.server_flag == 2)
        {
            return DIVOOM_SERVER_DEFAULT_IP;
        }
        else if(pDivoomHandle->sys_setting.server_flag)
        {
            return DIVOOM_SERVER_CHINA_IP;
        }
        else
        {
            return DIVOOM_SERVER_USA_IP;
        }
    }
}


/**
 * NAME         divoom_main_update_server_type
 * @BRIEF	更新服务器的地址
 * @PARAM	cur_tm
                    
 * @RETURN	1:国外，0：国内
 * @RETVAL	
 */

void divoom_main_update_server_type(int type)
{
    divoom_print_err("server type : %d\n", type);
    if(type==  0 || type == 1)
    {
        pDivoomHandle->sys_setting.server_flag = type;
    }
}

/**
 * NAME         divoom_main_get_server_type
 * @BRIEF	返回服务器类型
 * @PARAM	
                    
 * @RETURN	1:国外服务器，0：国内服务器
 * @RETVAL	
 */

uint8 divoom_main_get_server_type(void)
{
    return pDivoomHandle->sys_setting.server_flag != 0;
}

uint8 divoom_main_net_radio_available(void)
{
    return pDivoomHandle && (!divoom_main_get_server_type() || pDivoomHandle->sys_setting.beta_features);
}

/* MAIN 串行上报最新的本地选择。开机和重连只补发未确认的上报。 */
static void divoom_main_report_beta_features(void)
{
    if (divoom_main_get_server_type() && pDivoomHandle->divoom_connect_srv &&
        pDivoomHandle->sys_setting.beta_test_flag_pending)
    {
        int32 test_flag = pDivoomHandle->sys_setting.beta_features;
        pDivoomHandle->beta_test_flag_packet = divoom_net_app_send_packet_tracked(
            DIVOOM_NET_COMM_SAVE_DEVICE_TEST_FLAG, DIVOOM_SERVER_TOPIC, &test_flag);
    }
}

void divoom_main_set_beta_features(uint8 enabled)
{
    if (pDivoomHandle && divoom_main_get_server_type())
    {
        /* 由 MAIN 线程处理配置落盘，按键回调只投递切换请求。 */
        divoom_sys_msg_info msg = {0};
        msg.msg_type = 1;
        msg.src_id = DIVOOM_MODEL_MAIN;
        msg.dest_id = DIVOOM_MODEL_MAIN;
        msg.msg_id = DIVOOM_MAIN_EVENT_SET_BETA_FEATURES;
        msg.attr = enabled != 0;
        divoom_sys_model_crl_msg_push(&msg);
    }
}
int divoom_main_update_china_data_update_time(void)
{
    return pDivoomHandle->china_day_info_update_time;
}

/**
 * NAME         divoom_main_update_china_data_info
 * @BRIEF	更新当天的农历信息
 * @PARAM	cur_tm
                    
 * @RETURN	none
 * @RETVAL	
 */

void divoom_main_update_china_data_info(struct tm *cur_tm)
{
    static int get_day_time_info = -1;

    #if 0
    if(pDivoomHandle->server_flag == 1)
    {
        //只有国内的设备才获取
        return;
    }
    #endif
    
    if(cur_tm && get_day_time_info == cur_tm->tm_mday)
    {
        //一天才获取一次
        return;
    }
    
    Pdivoom_net_command pcmd_info;
    
    pcmd_info = divoom_net_app_send_packet_wait(DIVOOM_NET_COMM_DEVICE_REQUEST_CHINA_DAY, DIVOOM_SERVER_TOPIC, NULL);
    if(pcmd_info && pcmd_info->cmd_data)
    {
        Pdivoom_device_china_day_info day_info = (divoom_device_china_day_info *)pcmd_info->cmd_data;
        if(strcmp(pDivoomHandle->china_day_info.LunarDateTime ,day_info->LunarDateTime))
        {
            memcpy(&pDivoomHandle->china_day_info, pcmd_info->cmd_data, sizeof(divoom_device_china_day_info));
            pDivoomHandle->china_day_info_update_time = time(0);
        }
        if(cur_tm)
        {
            get_day_time_info = cur_tm->tm_mday;
        }
    }
    divoom_net_app_free_packet(pcmd_info);
}

Pdivoom_device_china_day_info divoom_main_get_china_data_info(void)
{
    if(strlen(pDivoomHandle->china_day_info.LunarDateTime) == 0)
    {
        divoom_main_update_china_data_info(NULL);
    }
    return &pDivoomHandle->china_day_info;
}

int divoom_main_get_time_zone(void)
{
    return pDivoomHandle->device_info.time_zone;
}

uint8 *divoom_main_get_phone_blue_addr(void)
{
    return pDivoomHandle->phone_mac_addr;
}

int divoom_main_get_device_id(void)
{
    return pDivoomHandle->device_info.device_id;
}

// 产品测试状态可能在音频任务启动前读取，控制结构未创建时按非测试模式处理。
uint8 divoom_main_get_protest_flag(void)
{
    if (pDivoomHandle != NULL)
    {
        return pDivoomHandle->product_test_flag;
    }
    return 0;
}

//设备是否在自升级状态
int divoom_main_get_device_self_update_flag(void)
{
    return pDivoomHandle->request_update_flag;
}



//获取设备最新版本
int divoom_main_get_device_last_version(void)
{
    Pdivoom_general_json_get_update_file_info_respone pupdate_info = (Pdivoom_general_json_get_update_file_info_respone)pDivoomHandle->pupdate_info;
    return pupdate_info->version;
}

//获取设备最新版本的升级信息
void divoom_main_get_device_last_version_update_info(char *save_update_info)
{
	if(pDivoomHandle)
	{
		Pdivoom_general_json_get_update_file_info_respone pupdate_info = (Pdivoom_general_json_get_update_file_info_respone)pDivoomHandle->pupdate_info;
		memcpy(save_update_info, pupdate_info->update_info, sizeof(pupdate_info->update_info));
	}
    
    return;
}


//用户启动设备升级
int divoom_main_user_start_update_device(void)
{
    Pdivoom_general_json_get_update_file_info_respone update_info =
        (Pdivoom_general_json_get_update_file_info_respone)divoom_malloc(
            sizeof(divoom_general_json_get_update_file_info_respone), DIVOOM_MODEL_NET, __LINE__, __FILE__);
    if (!update_info)
        return -1;
    memcpy(update_info, pDivoomHandle->pupdate_info, sizeof(*update_info));
    /* Cancel/drain resource work already in flight; About never starts a file update. */
    if (!divoom_update_system_file_start_firmware(update_info, DIVOOM_OTA_SERVER_SRV_UPDATE))
    {
        divoom_system_free(update_info);
        return -1;
    }
    return 0;
}

void *divoom_main_set_device_update_info(void *pupdate_info)
{
    if(pupdate_info != NULL)
    {
       memcpy(pDivoomHandle->pupdate_info, pupdate_info, sizeof(divoom_general_json_get_update_file_info_respone));
    }
    return pDivoomHandle->pupdate_info;
}

void divoom_main_set_mast_flag(int flag)
{
	pDivoomHandle->master_flag = flag;
}

#ifdef DIVOOM_TIME_FRAME_SPEAKER
/*
 * UI 滑块 0~100（线性）→ 传给 softvol 的百分比。
 * Soft Volume 在 asound 里按 dB 映射（如 -51~0dB），线性 50% 对应约 -25dB，听感明显偏小；
 * 用 sqrt 抬高中档：50→≈71%、25→≈50%，0 与 100 不变。
 */
static int divoom_main_ui_vol_to_softvol_pct(int ui_vol)
{
	float x;

	if (ui_vol <= 0)
		return 0;
	if (ui_vol >= 100)
		return 100;
	x = (float)ui_vol / 100.0f;
	return (int)(100.0f * sqrtf(x) + 0.5f);
}

#define DIVOOM_MAIN_SOFTVOL_RETRY_MAX  35
#define DIVOOM_MAIN_SOFTVOL_RETRY_US   120000

static int divoom_main_amixer_softvol_cmd_ok(const char *cmd)
{
	int st;

	st = system(cmd);
	if (st == -1)
		return 0;
	if (WIFEXITED(st) && WEXITSTATUS(st) == 0)
		return 1;
	return 0;
}

static void divoom_main_amixer_softvol_set_pct(int pct)
{
	char cmd_softvol[160];
	int i;

	if (pct < 0)
		pct = 0;
	if (pct > 100)
		pct = 100;
	snprintf(cmd_softvol, sizeof(cmd_softvol), "amixer -c %d cset name='%s' %d%%", DIVOOM_ALSA_SOFTVOL_CARD, DIVOOM_ALSA_SOFTVOL_ELEM, pct);

	divoom_debug("softvol:%s", cmd_softvol);
	divoom_pcm_try_prime_playback_default();

	for (i = 0; i < DIVOOM_MAIN_SOFTVOL_RETRY_MAX; i++) 
	{
		if (divoom_main_amixer_softvol_cmd_ok(cmd_softvol)) 
		{
			if (i > 0)
				divoom_debug("softvol ok after %d retries", i);
			return;
		}
		if ((i % 6) == 5)
			divoom_pcm_try_prime_playback_default();
		divoom_port_delay_ms((DIVOOM_MAIN_SOFTVOL_RETRY_US) / 1000U);
	}
	divoom_print("softvol FAILED (%d tries): %s\n", DIVOOM_MAIN_SOFTVOL_RETRY_MAX, cmd_softvol);
}
#endif

void divoom_main_init_sys_vol(void) 
{
#ifdef DIVOOM_TIME_FRAME_SPEAKER
#ifdef DIVOOM_TIMES_VIBE
	divoom_pcm_apply_capture_gain();
	return;
#endif

	/* daudio hub/loopback 仍在 card 1；软音量在 DIVOOM_ALSA_SOFTVOL_CARD（见宏说明） */
	divoom_port_delay_ms((200000) / 1000U);
	exec_command_system("amixer -c 1 cset numid=1 1");   /* sunxi daudio audio hub mode，1 使能 */
	exec_command_system("amixer -c 1 cset numid=2 0");   /* sunxi daudio loopback debug，0 关闭环回 */
	divoom_main_amixer_softvol_set_pct(50);
	pDivoomHandle->sys_vol = 50;
	divoom_print("divoom_main_init_sys_vol: card1 hub/loopback + softvol card %d elem \"%s\"\n",
		     DIVOOM_ALSA_SOFTVOL_CARD, DIVOOM_ALSA_SOFTVOL_ELEM);
#else
	exec_command_system("amixer cset numid=15");
	exec_command_system("amixer cset numid=4 63");
	exec_command_system("amixer cset numid=17 7");
	exec_command_system("amixer cset numid=5 160");
	exec_command_system("amixer cset numid=11 25");
	pDivoomHandle->sys_vol = 100;

#endif
}

//设置或获取系统音量（播放走 default→softvol；控件名见 DIVOOM_ALSA_SOFTVOL_ELEM）
int divoom_main_set_sys_vol(int vol)
{
	if (!pDivoomHandle)
		return 0;

#ifdef DIVOOM_TIME_FRAME_SPEAKER
#ifdef DIVOOM_TIMES_VIBE
	return pDivoomHandle->sys_vol;
#endif

    if(vol != -1)
    {
		int ui_vol;
		int softvol_pct;

        pDivoomHandle->sys_vol = vol;
        ui_vol = vol;
        if (ui_vol > 100)
            ui_vol = 100;
        if (ui_vol < 0)
            ui_vol = 0;
		softvol_pct = divoom_main_ui_vol_to_softvol_pct(ui_vol);
		divoom_debug("sys_vol ui:%d softvol:%d%% card:%d '%s'", ui_vol, softvol_pct, DIVOOM_ALSA_SOFTVOL_CARD, DIVOOM_ALSA_SOFTVOL_ELEM);
		divoom_main_amixer_softvol_set_pct(softvol_pct);
    }
#else
	int vol_table[] = {0, 40, 44, 48, 53,55,57,59,61,62,63};
    if(vol != -1)
    {
		char cmd_str[128] = {0};
		char vol_str[20] = {0};
        pDivoomHandle->sys_vol = vol;
		strcpy(cmd_str, "cset numid=4 ");
		sprintf(vol_str, "%d", vol_table[vol/10]);
		strcat(cmd_str, vol_str);
		exec_command_system(cmd_str);
    }
#endif

    return pDivoomHandle->sys_vol;
}

#if DIVOOM_BUILD_RTOS_DEVICE
/* 外部 set/mute 递增；ramp 用 raw 发送并检测 seq 变化以中止，避免切歌 mute 与 ramp 交叉啪声 */
static volatile unsigned int s_jl_tone_vol_seq;

static void divoom_main_jl_mcu_tone_volume_send(int vol_0_100, const char *from)
{
	uint8_t payload[3] = {0};

	if (vol_0_100 < 0)
		vol_0_100 = 0;
	else if (vol_0_100 > 100)
		vol_0_100 = 100;
	if (vol_0_100 == 0 && divoom_local_movie_play_should_keep_jl_tone_volume())
	{
		divoom_print("jl_tone_vol: block mute caller=%s keep=poweron_movie\n", from);
		return;
	}
	divoom_debug("jl_tone_vol caller:%s vol:%d", from, vol_0_100);
	payload[0] = 1;
	payload[1] = 0;
	payload[2] = (uint8_t)vol_0_100;
	if (divoom_btcore_uart_cmd_is_ready())
	{
		if (divoom_btcore_uart_cmd_send(DM_BTCORE_TONE_VOLUME, payload, 3) != 0)
			divoom_print("jl_tone_vol: send fail caller=%s vol=%u\n", from, (unsigned)payload[2]);
	}
	else
		divoom_print("jl_tone_vol: btcore not ready caller=%s vol=%d\n", from, vol_0_100);
}

void divoom_main_set_jl_mcu_tone_volume_tag(int vol_0_100, const char *caller)
{
	const char *from = caller ? caller : "?";

	s_jl_tone_vol_seq++;
	divoom_main_jl_mcu_tone_volume_send(vol_0_100, from);
}

#define DIVOOM_JL_TONE_VOL_RAMP_STEPS 10
#define DIVOOM_JL_TONE_VOL_RAMP_STEP_MS 8

void divoom_main_ramp_jl_mcu_tone_volume(int vol_0_100)
{
	int i;
	const int steps = DIVOOM_JL_TONE_VOL_RAMP_STEPS;
	unsigned int seq;

	if (vol_0_100 <= 0)
		return;
	seq = s_jl_tone_vol_seq;
	for (i = 1; i <= steps; i++)
	{
		int v;

		if (s_jl_tone_vol_seq != seq)
		{
			divoom_print("jl_tone_vol: ramp abort vol=%d step=%d/%d\n",
				     vol_0_100, i, steps);
			return;
		}
		v = (vol_0_100 * i + steps - 1) / steps;
		divoom_main_jl_mcu_tone_volume_send(v, "ramp");
		/* 阶梯间短延时，避免 UART 连发导致功放台阶啪声 */
		if (i < steps)
			divoom_system_delay_ms(DIVOOM_JL_TONE_VOL_RAMP_STEP_MS);
	}
}

#else
void divoom_main_set_jl_mcu_tone_volume_tag(int vol_0_100, const char *caller)
{
	(void)vol_0_100;
	(void)caller;
}
void divoom_main_ramp_jl_mcu_tone_volume(int vol_0_100)
{
	(void)vol_0_100;
}
#if !defined(DIVOOM_PRODUCT_ASTRO_TOO)
/* AstroToo 仿真已经编入 divoom_btcore_uart_cmd.c，旧 fallback 仅留给未接入该模块的产品。 */
int divoom_btcore_uart_cmd_is_ready(void)
{
	return 1;
}
int divoom_btcore_uart_cmd_send(uint8_t cmd, const uint8_t *cmd_data, uint16_t cmd_len)
{
	(void)cmd;
	(void)cmd_data;
	return cmd_len;
}
#endif
#endif

uint8 divoom_main_get_sys_light(void)
{
    return pDivoomHandle->sys_setting.brightness;
}

/* 用户亮度 0 仍保持开屏最低档，与系统亮度一致；灭屏只用 divoom_screen_bright(0)。 */
static uint8 divoom_main_user_bright_to_hw(uint8 light)
{
    if (light == 0)
    {
        return DIVOOM_SYS_BRIGHT_ON_FLOOR;
    }
    return light;
}

uint8 divoom_main_get_sys_light_hw(void)
{
    return divoom_main_user_bright_to_hw(divoom_main_get_sys_light());
}

void divoom_main_apply_sys_light(void)
{
    if (pDivoomHandle == NULL)
    {
        return;
    }
    /* 关屏只关背光：滑条/云端调亮度只改配置，PWM 保持 0，开屏再按配置恢复。 */
    if (divoom_light_app_get_screen_status(0xFF) == 0)
    {
        divoom_screen_bright(0);
        return;
    }
    if (pDivoomHandle->sleep_bright_override)
    {
        divoom_screen_bright(pDivoomHandle->sleep_bright);
        return;
    }
    if (pDivoomHandle->night_bright_override)
    {
        divoom_screen_bright(divoom_main_user_bright_to_hw(pDivoomHandle->night_bright));
        return;
    }
    divoom_screen_bright(divoom_main_get_sys_light_hw());
}

void divoom_main_set_sys_light(uint8 value)
{
    pDivoomHandle->sleep_bright_override = 0;
    pDivoomHandle->sys_setting.brightness  = value;
    divoom_main_apply_sys_light();
    divoom_main_save_config_local();
}

void divoom_main_set_sleep_brightness(uint8 value)
{
    /* 只改助眠覆盖档，不改系统配置。关屏时 PWM 保持 0，开屏由 apply_sys_light 再用本档。 */
    if (pDivoomHandle == NULL)
    {
        return;
    }
    pDivoomHandle->sleep_bright_override = 1;
    pDivoomHandle->sleep_bright = value;
    if (divoom_light_app_get_screen_status(0xFF) == 0)
    {
        divoom_screen_bright(0);
        return;
    }
    divoom_screen_bright(value);
}

uint8 divoom_main_get_sleep_brightness(void)
{
    if (pDivoomHandle == NULL)
    {
        return 0;
    }
    if (pDivoomHandle->sleep_bright_override)
    {
        return pDivoomHandle->sleep_bright;
    }
    return divoom_main_get_sys_light();
}

void divoom_main_set_night_brightness(uint8 value)
{
    /* 夜览只覆盖 PWM，不改系统配置；0 与系统最低开档相同。退出夜览再 clear + apply_sys_light。 */
    if (pDivoomHandle == NULL)
    {
        return;
    }
    pDivoomHandle->night_bright_override = 1;
    pDivoomHandle->night_bright = value;
    if (divoom_light_app_get_screen_status(0xFF) == 0)
    {
        divoom_screen_bright(0);
        return;
    }
    divoom_screen_bright(divoom_main_user_bright_to_hw(value));
}

void divoom_main_clear_night_brightness(void)
{
    if (pDivoomHandle == NULL)
    {
        return;
    }
    pDivoomHandle->night_bright_override = 0;
}

void divoom_main_upload_sys_brightness(uint8 value)
{
	divoom_system_setting_birghtness_conf brightness;

	brightness.light = value;
	divoom_net_app_send_packet(DIVOOM_NET_COMM_SET_SYS_BRIGHTNESS, DIVOOM_SERVER_TOPIC, &brightness);
}

void divoom_main_set_audio_volume(uint8 value)
{
    pDivoomHandle->sys_setting.audio_volume = value;
    divoom_main_save_config_local();
}

	
int divoom_main_get_color_temp(void)
{
	if(pDivoomHandle)
    {
		return pDivoomHandle->sys_setting.color_temp;
	}

	return 0;
}

void divoom_main_set_color_temp_inner(int value)
{
	char cmd[256];

	sprintf(cmd, "echo %d > /sys/class/disp/disp/attr/color_temperature", value);
	exec_command_system(cmd);
}

void divoom_main_set_color_temp(int value)
{
	if(value > 256 || value < -256)
	{
		return;
	}
	
    pDivoomHandle->sys_setting.color_temp = value;
    divoom_main_set_color_temp_inner(value);
    divoom_main_save_config_local();
}


uint8 divoom_main_get_sys_volume(void)
{
	return pDivoomHandle->sys_setting.volume;
}

uint8 divoom_main_get_time_flag(void)
{
	return pDivoomHandle->sys_setting.hour12_24;
}


uint8 divoom_main_get_wind_speed_flag(void)
{
	if(pDivoomHandle)
	{
		return pDivoomHandle->sys_setting.wind_speed_flag;
	}
	return 0;
}

void divoom_main_set_wind_speed_flag(uint8 flag)
{
	if(pDivoomHandle->sys_setting.wind_speed_flag != flag)
	{
		pDivoomHandle->sys_setting.wind_speed_flag = flag;
	}
}


uint8 divoom_main_get_time_format_type(void)
{
    
    if(pDivoomHandle->sys_setting.time_format >= DIVOOM_DISP_TIME_FORMAT_END_TYPE)
    {
        return 0;
    }
	return pDivoomHandle->sys_setting.time_format;
}

void divoom_main_set_time_format_type(uint8 type)
{
    pDivoomHandle->sys_setting.time_format = type;
}

int divoom_main_set_tf_music_order(int order)
{
    if(order != -1)
    {
        pDivoomHandle->sys_setting.tf_music_order = order;
        divoom_tf_music_set_order(order);
        divoom_main_save_config_local();
    }
    return pDivoomHandle->sys_setting.tf_music_order;
}

int divoom_main_set_tf_music_mode(int mode)
{
    if(mode != -1)
    {
        pDivoomHandle->sys_setting.tf_music_mode = mode;
        divoom_tf_music_set_mode(mode);
        divoom_main_save_config_local();
    }
    return pDivoomHandle->sys_setting.tf_music_mode;
}

int divoom_main_set_tf_music_bg(int id)
{
    if(id != -1)
    {
        pDivoomHandle->sys_setting.tf_music_bg = id;
        divoom_main_save_config_local();
    }
    return pDivoomHandle->sys_setting.tf_music_bg;
}

int divoom_main_set_tf_music_id(int id)
{
    if(id != -1)
    {
        pDivoomHandle->sys_setting.tf_music_id = id;
        divoom_main_save_config_local();
    }
    return pDivoomHandle->sys_setting.tf_music_id;
}

int divoom_main_set_system_file_update_time(int system_update_time)
{
    if(pDivoomHandle->system_file_update_time != system_update_time && system_update_time != -1)
    {
        pDivoomHandle->system_file_update_time = system_update_time;
        divoom_main_save_config_local();
    }
    return pDivoomHandle->system_file_update_time;
}


void divoom_main_set_time_flag(int mode)
{
    if(pDivoomHandle->sys_setting.hour12_24 != mode)
    {
    	pDivoomHandle->sys_setting.hour12_24 = mode;
        divoom_main_save_notice_dis();
    }
}


Pdivoom_screen_color_info divoom_main_get_time_ptr(void)
{
	return NULL;
//	return &gdivoom_main_handle->light_info.env_info.color;
}

void divoom_main_disp_cacnel_wifi_notice(void)
{
    divoom_sys_msg_info model_msg = {0};
    Pdivoom_disp_event_extend_info info ;

    info = divoom_system_malloc(sizeof(divoom_disp_event_extend_info));
    info->model_id = DIVOOM_MODEL_MAIN;
    info->event_id = DIVOOM_MAIN_PLAY_WIFI_NOTICE_EVENT;
    model_msg.src_id = DIVOOM_MODEL_MAIN;
    model_msg.dest_id = DIVOOM_MODEL_MAIN;  
    model_msg.msg_id = DIVOOM_MAIN_EVENT_SCHEDULE_CANCEL;
    model_msg.msg_type = 1;
    model_msg.data = (uint8 *)info;
    divoom_sys_model_crl_msg_push(&model_msg);
}

void divoom_main_disp_cacnel_sd_free_warning(void)
{
    divoom_sys_msg_info model_msg = {0};
    Pdivoom_disp_event_extend_info info ;

    info = divoom_system_malloc(sizeof(divoom_disp_event_extend_info));
    info->model_id = DIVOOM_MODEL_MAIN;
    info->event_id = DIVOOM_MAIN_PLAY_SD_FREE_WARNING;
    model_msg.src_id = DIVOOM_MODEL_MAIN;
    model_msg.dest_id = DIVOOM_MODEL_MAIN;  
    model_msg.msg_id = DIVOOM_MAIN_EVENT_SCHEDULE_CANCEL;
    model_msg.msg_type = 1;
    model_msg.data = (uint8 *)info;
    divoom_sys_model_crl_msg_push(&model_msg);
}

void divoom_main_disp_cacnel_sd_format(void)
{
    divoom_sys_msg_info model_msg = {0};
    Pdivoom_disp_event_extend_info info ;

    info = divoom_system_malloc(sizeof(divoom_disp_event_extend_info));
    info->model_id = DIVOOM_MODEL_MAIN;
    info->event_id = DIVOOM_MAIN_PLAY_SD_ERR_EVENT;
    model_msg.src_id = DIVOOM_MODEL_MAIN;
    model_msg.dest_id = DIVOOM_MODEL_MAIN;  
    model_msg.msg_id = DIVOOM_MAIN_EVENT_SCHEDULE_CANCEL;
    model_msg.msg_type = 1;
    model_msg.data = (uint8 *)info;
    divoom_sys_model_crl_msg_push(&model_msg);
}
uint8 divoom_main_set_power_flag(uint8 flag);

void divoom_main_clear_power_flag(void)
{
    divoom_main_set_power_flag(0);
}

int divoom_main_disp_power_over_notice(uint8 mode)
{
    //发异步通知开机事件完成
    divoom_sys_msg_info model_msg = {0};
    uint8 *user_data;
    
    divoom_debug("power pic finished:%d", mode);
    if(mode == DIVOOM_MAIN_PLAY_POWER_ON_EVENT)
    {
        divoom_main_clear_power_flag();
    }
    
    if(pDivoomHandle->divoom_connect_srv == 0 && divoom_power_on_manage_get_power_mode() == DIVOOM_POWER_ON_WIFI_CONFIG_MODE)
    {

        model_msg.src_id = DIVOOM_MODEL_MAIN;
        model_msg.dest_id = DIVOOM_MODEL_MAIN;  
        model_msg.msg_id = DIVOOM_MAIN_NOTICE_WIFI_NOTICE_MESSAGE;
        model_msg.msg_type = 1;
        divoom_sys_model_crl_msg_push(&model_msg);
        return 1;
    }
    else if(pDivoomHandle->sys_finish_init_ok)
    {
        user_data = divoom_system_malloc(sizeof(uint8));
        *user_data = mode;
        
        if(mode == DIVOOM_MAIN_PLAY_WIFI_NOTICE_EVENT && pDivoomHandle->divoom_connect_srv == 0)
        {
            //divoom_light_app_play_hot_info();
        }

        model_msg.src_id = DIVOOM_MODEL_MAIN;
        model_msg.dest_id = DIVOOM_MODEL_MAIN;  
        model_msg.msg_id = DIVOOM_MAIN_NOTICE_OVER_MESSAGE;
        model_msg.data = user_data;
        model_msg.msg_type = 1;
        divoom_sys_model_crl_msg_push(&model_msg);
        return 1;
    }
    return 0;
    
}

void divoom_main_disp_power_on(int poweron_mode)
{
    #if 0
	divoom_app_schedule_item_info push_info = {0};

    if(poweron_mode == DIVOOM_ONOFF_MODE_CLOCK)
    {
        divoom_print("we will skip power on pic!\n");
        return;
    }
	push_info.event_id = DIVOOM_MAIN_PLAY_POWER_ON_EVENT;
	push_info.model_id = DIVOOM_MODEL_MAIN;
    push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
	if(divoom_app_schedule_push(&push_info))
	{
        divoom_disp_power_on_disp_info disp_info = {0};
        
        disp_info.power_down_flag = 0;
        disp_info.noice_id = DIVOOM_MAIN_PLAY_POWER_ON_EVENT;
        disp_info.pover_callback = divoom_main_disp_power_over_notice;
        divoom_disp_ctrl_set_mode(DIVOOM_DISP_POWER_ON, &disp_info);
	}
	else
	{
		divoom_print("Here Can't get Scheduel Priority!");
	}
	return;
    #endif
}

int divoom_main_disp_blue_notice(uint8 mode)
{
    //发异步通知开机事件完成
    divoom_sys_msg_info model_msg = {0};
    uint8 *user_data;
    
    user_data = divoom_system_malloc(sizeof(uint8));
    *user_data = mode;

    model_msg.src_id = DIVOOM_MODEL_MAIN;
    model_msg.dest_id = DIVOOM_MODEL_MAIN;  
    model_msg.msg_id = DIVOOM_MAIN_NOTICE_OVER_MESSAGE;
    model_msg.data = user_data;
    model_msg.msg_type = 1;
    divoom_sys_model_crl_msg_push(&model_msg);
    return 1;
}

void divoom_main_disp_blue_dis_connect(void)
{
    #if 0
	divoom_app_schedule_item_info push_info = {0};
	divoom_disp_event_disp_info notice_info = {0};

	notice_info.disp_id[0] = DIVOOM_DISP_BLUE_DISCONNECT;
	notice_info.on_time = 1;
	notice_info.period_time = 1;
	notice_info.tot_pic = 1;
	notice_info.tot_time = 10; // 10 *100ms
	notice_info.noice_id = DIVOOM_MAIN_PLAY_BLUE_NOTICE_EVENT;
	notice_info.pover_callback = divoom_main_disp_blue_notice;

	push_info.event_id = DIVOOM_MAIN_PLAY_BLUE_NOTICE_EVENT;
	push_info.model_id = DIVOOM_MODEL_MAIN;
	push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
	if(divoom_app_schedule_push(&push_info))
	{
		divoom_disp_ctrl_set_mode(DIVOOM_DISP_EVENT_DIS_CTRL, (void *)&notice_info);
	}
	else
	{
		divoom_print("Here Can't get Scheduel Priority!");
	}
    #endif
}


void divoom_main_disp_blue_connect_ok(void)
{
    #if 0
	divoom_app_schedule_item_info push_info = {0};
	divoom_disp_event_disp_info notice_info = {0};

	notice_info.disp_id[0] = DIVOOM_DISP_BLUE_CONNECT_00;
	notice_info.disp_id[1] = DIVOOM_DISP_BLUE_CONNECT_01;
	notice_info.disp_id[2] = DIVOOM_DISP_BLUE_CONNECT_02;
	notice_info.on_time = 1;
	notice_info.period_time = 3;
	notice_info.tot_pic = 3;
	notice_info.tot_time = 10; // 10 *100ms
	notice_info.noice_id = DIVOOM_MAIN_PLAY_BLUE_NOTICE_EVENT;
	notice_info.pover_callback = divoom_main_disp_blue_notice;

	push_info.event_id = DIVOOM_MAIN_PLAY_BLUE_NOTICE_EVENT;
	push_info.model_id = DIVOOM_MODEL_MAIN;
	push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
	if(divoom_app_schedule_push(&push_info))
	{
		divoom_disp_ctrl_set_mode(DIVOOM_DISP_EVENT_DIS_CTRL, (void *)&notice_info);
	}
	else
	{
		divoom_print("Here Can't get Scheduel Priority!");
	}
    #endif
}

void divoom_main_disp_blue_connect(int time)
{
    #if 0
	divoom_app_schedule_item_info push_info = {0};
	divoom_disp_event_disp_info notice_info = {0};

	notice_info.disp_id[0] = DIVOOM_DISP_BLUE_MODE_00;
	notice_info.disp_id[1] = DIVOOM_DISP_BLUE_MODE_01;
	notice_info.disp_id[2] = DIVOOM_DISP_BLUE_MODE_01;
	notice_info.on_time = 1;
	notice_info.period_time = 1;
	notice_info.tot_pic = 3;
	notice_info.tot_time = time; // 10 *100ms
	notice_info.noice_id = DIVOOM_MAIN_PLAY_BLUE_NOTICE_EVENT;
	notice_info.pover_callback = divoom_main_disp_blue_notice;

	push_info.event_id = DIVOOM_MAIN_PLAY_BLUE_NOTICE_EVENT;
	push_info.model_id = DIVOOM_MODEL_MAIN;
	push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
	if(divoom_app_schedule_push(&push_info))
	{
		divoom_disp_ctrl_set_mode(DIVOOM_DISP_EVENT_DIS_CTRL, (void *)&notice_info);
	}
	else
	{
		divoom_print("Here Can't get Scheduel Priority!");
	}
    #endif
}


T_VOID divoom_main_disp_low_battery(T_VOID)
{
    #if 0
	divoom_app_schedule_item_info push_info = {0};
    divoom_disp_event_disp_info notice_info = {0};

//    notice_info.disp_id[0] = DIVOOM_DISP_LOW_POWER_PIC;
//	notice_info.on_time = 5;
//	notice_info.period_time = 10;
//	notice_info.tot_pic = 1;
//	notice_info.tot_time = 80;
//    notice_info.noice_id = DIVOOM_MAIN_PLAY_LOW_POWER_EVENT;
//    notice_info.pover_callback = divoom_main_disp_power_over_notice;
//    notice_info.disp_title = divoom_main_get_disp_title(DIVOOM_NOTICE_LANUAGE_BAT_LOW_MSG);
    
	push_info.event_id = DIVOOM_MAIN_PLAY_LOW_POWER_EVENT;
	push_info.model_id = DIVOOM_MODEL_MAIN;
	push_info.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
	push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;	

	if(divoom_app_schedule_push(&push_info))
	{
		divoom_main_play_voice_tip(DIVOOM_MAIN_PLAY_LOW_POWER_EVENT, NULL);
        divoom_disp_ctrl_set_mode(DIVOOM_DISP_EVENT_DIS_CTRL, (void *)&notice_info);
	}
	else
	{
		divoom_print("Here Can't get Scheduel Priority!");
	}
    #endif
	divoom_lvgl_top_battery_menu_init(0);
	divoom_main_play_voice_tip(DIVOOM_MAIN_PLAY_LOW_POWER_EVENT, NULL);
	/* 已在充电则不打 30：避免 0x33 入队后插电已恢复、主循环才把背光压暗 */
	// if (!divoom_is_charge_sta())
	// 	divoom_disp_ctrl_set_light(30);
}


int divoom_main_low_battery_poweroff_play_over_notic(uint8 status)
{
	divoom_sys_msg_info model_msg = {0};
	model_msg.msg_type = 1;
	model_msg.msg_id = DIVOOM_MAIN_EVENT_KEY_POWEROFF;
	model_msg.src_id = DIVOOM_MODEL_MAIN;
	model_msg.dest_id = DIVOOM_MODEL_MAIN;
	divoom_sys_model_crl_msg_push(&model_msg);
    return 1;
}

static void *s_voice_tip_over_param;
static int divoom_main_play_voice_tip_over_bridge(char *music_name, int status)
{
	typedef void (*voice_tip_over_fn)(uint8);
	voice_tip_over_fn fn;
	(void)music_name;
	fn = (voice_tip_over_fn)s_voice_tip_over_param;
	s_voice_tip_over_param = NULL;
	if (fn != NULL)
		fn((uint8)status);
	return 0;
}

void divoom_main_play_voice_tip(int event, void *param)
{
	char file_name[128];
	uint32 voice_id = (uint32)-1;
	int vol = divoom_main_get_tone_volume();
	if (vol <= 0)
		vol = 80;

	switch (event) {
	case DIVOOM_MAIN_PLAY_LOW_POWER_EVENT:
		voice_id = DIVOOM_VOICE_ID_LOWPOWER;
		break;
	case DIVOOM_MAIN_PLAY_POWER_OFF_EVENT:
		voice_id = DIVOOM_VOICE_ID_POWEROFF;
		break;
	case DIVOOM_MAIN_PLAY_WIFI_CONNECT_EVENT:
	case DIVOOM_MAIN_PLAY_BLUE_NOTICE_EVENT:
		voice_id = DIVOOM_VOICE_ID_CONNECTED;
		break;
	case DIVOOM_MAIN_PLAY_CHECK_POWER_EVENT:
		voice_id = DIVOOM_VOICE_ID_LOWPOWER;
		break;
	default:
		divoom_print("[%s] unknown event: %d\n", __func__, event);
		return;
	}

	if (voice_id == (uint32)-1)
		return;

	if (divoom_voice_read_get_filename(voice_id, file_name) != 0) 
	{
		divoom_print("[%s] get filename fail for voice_id %u\n", __func__, (unsigned)voice_id);
		return;
	}

	s_voice_tip_over_param = param;
	divoom_local_music_ctrl_tone_play(file_name, vol, param ? divoom_main_play_voice_tip_over_bridge : NULL, 0);
}

T_VOID divoom_main_disp_low_battery_poweroff(T_VOID)
{
    #if 0
	divoom_app_schedule_item_info push_info = {0};
    divoom_disp_event_disp_info notice_info = {0};

    notice_info.disp_id[0] = DIVOOM_DISP_LOW_POWER_PIC;
	notice_info.on_time = 5;
	notice_info.period_time = 10;
	notice_info.tot_pic = 1;
	notice_info.tot_time = 50;
    notice_info.noice_id = DIVOOM_MAIN_PLAY_LOW_POWER_EVENT;
    notice_info.pover_callback = divoom_main_low_battery_poweroff_play_over_notic;
    notice_info.disp_title = divoom_main_get_disp_title(DIVOOM_NOTICE_LANUAGE_BAT_LOW_MSG);
    
	push_info.event_id = DIVOOM_MAIN_PLAY_LOW_POWER_EVENT;
	push_info.model_id = DIVOOM_MODEL_MAIN;
	push_info.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
	push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;	

	if(divoom_app_schedule_push(&push_info))
	{
		divoom_main_play_voice_tip(DIVOOM_MAIN_PLAY_LOW_POWER_EVENT, divoom_main_low_battery_poweroff_play_over_notic);
        divoom_disp_ctrl_set_mode(DIVOOM_DISP_EVENT_DIS_CTRL, (void *)&notice_info);
		divoom_sys_model_crl_send_all_model(DIVOOM_APP_CONFIG_SAVE_EVENT_ID);
	}
	else
	{
		divoom_print("Here Can't get Scheduel Priority!");
	}
    #endif
	divoom_main_play_voice_tip(DIVOOM_MAIN_PLAY_LOW_POWER_EVENT, divoom_main_low_battery_poweroff_play_over_notic);
//	divoom_lvgl_top_battery_menu_init(0);

	divoom_sys_model_crl_send_all_model(DIVOOM_APP_CONFIG_SAVE_EVENT_ID);
	// divoom_disp_ctrl_set_light(30);	//调整亮度
}


T_VOID divoom_main_play_ifft_voice(char *file_name)
{
    #if 0
	divoom_app_schedule_item_info push_info = {0};
    
	push_info.event_id = DIVOOM_MAIN_PLAY_IFFT_VOICE_EVENT;
	push_info.model_id = DIVOOM_MODEL_MAIN;
    push_info.resource_type = DIVOOM_APP_SCHEDULE_RESOURCCE_RGB;
	if(divoom_app_schedule_push(&push_info))
	{
        divoom_main_play_notice_voice(file_name, divoom_main_play_ifft_voice_over_callback, 0);
	}
	else
	{
		divoom_print("Here Can't get Scheduel Priority!");
	}
    #endif
}

T_VOID divoom_main_disp_wifi_connecting(int request_flag, int time)
{
    #if 0
	divoom_app_schedule_item_info push_info = {0};
    divoom_disp_event_disp_info notice_info = {0};

    notice_info.disp_id[0] = DIVOOM_DISP_WIFI_CONNECTING_00;
    notice_info.disp_id[1] = DIVOOM_DISP_WIFI_CONNECTING_01;
    notice_info.disp_id[2] = DIVOOM_DISP_WIFI_CONNECTING_02;
    notice_info.disp_id[3] = DIVOOM_DISP_WIFI_CONNECTING_03;
    notice_info.disp_id[4] = DIVOOM_DISP_WIFI_CONNECTING_04;
	notice_info.on_time = 1;
	notice_info.period_time = 1;
	notice_info.tot_pic = 5;
	notice_info.tot_time = time;
    notice_info.noice_id = DIVOOM_MAIN_PLAY_WIFI_NOTICE_EVENT;
    notice_info.pover_callback = divoom_main_disp_power_over_notice;
    notice_info.disp_title = divoom_main_get_disp_title(DIVOOM_NOTICE_LANUAGE_CONNECT_WIFI_MSG);
    
	push_info.event_id = DIVOOM_MAIN_PLAY_WIFI_NOTICE_EVENT;
	push_info.model_id = DIVOOM_MODEL_MAIN;
    if(request_flag)
    {
        push_info.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
    }
    push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
	if(request_flag == 0 || divoom_app_schedule_push(&push_info))
	{
        divoom_disp_ctrl_set_mode(DIVOOM_DISP_EVENT_DIS_CTRL, (void *)&notice_info);
        //divoom_main_play_voice_tip(DIVOOM_MAIN_PLAY_WIFI_CONNECT_EVENT, 1);
	}
	else
	{
		divoom_print("Here Can't get Scheduel Priority!");
	}
    #endif
}
void divoom_main_manage_dis_all_app_con(void);

/*
  Function:       divoom_main_disp_device_reset
  Description:	   设备reset时的图案
  Input:          time
  Return:         NONE
*/

void divoom_main_disp_device_reset(void)
{
	divoom_app_schedule_item_info push_info = {0};
    divoom_disp_event_disp_info notice_info = {0};

    
    divoom_manage_image_up_netstatus(0);

    notice_info.disp_id[0] = DIVOOM_DISP_LOADING_00;
    notice_info.disp_id[1] = DIVOOM_DISP_LOADING_01;
    notice_info.disp_id[2] = DIVOOM_DISP_LOADING_02;
    notice_info.disp_id[3] = DIVOOM_DISP_LOADING_03;
    notice_info.disp_id[4] = DIVOOM_DISP_LOADING_04;
    notice_info.disp_id[5] = DIVOOM_DISP_LOADING_05;
    notice_info.disp_id[6] = DIVOOM_DISP_LOADING_06;
    notice_info.disp_id[7] = DIVOOM_DISP_LOADING_07;
	notice_info.on_time = 1;
	notice_info.period_time = 1;
	notice_info.tot_pic = 8;
	notice_info.tot_time = 1000;
    notice_info.noice_id = DIVOOM_MAIN_PLAY_DEVICE_UPDATE_EVENT;
    notice_info.pover_callback = divoom_main_disp_power_over_notice;
    
	push_info.event_id = DIVOOM_MAIN_PLAY_DEVICE_UPDATE_EVENT;
	push_info.model_id = DIVOOM_MODEL_MAIN;
    push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
	if(divoom_app_schedule_push(&push_info))
	{
        divoom_disp_ctrl_set_mode(DIVOOM_DISP_EVENT_DIS_CTRL, (void *)&notice_info);
        //divoom_main_play_voice_tip(DIVOOM_MAIN_PLAY_WIFI_CONNECT_EVENT, 1);
	}
    divoom_main_manage_dis_all_app_con();
}

/*
  Function:       divoom_main_disp_wifi_config
  Description:	   WIFI 配网过程中
  Input:          time
  Return:         NONE
*/

T_VOID divoom_main_disp_wifi_config(int request_flag, int time)
{
    #if 0
	divoom_app_schedule_item_info push_info = {0};
    divoom_disp_event_disp_info notice_info = {0};

    if(pDivoomHandle->pass_wifi_discon_notice)
    {
        return;
    }

    notice_info.disp_id[0] = DIVOOM_DISP_WIFI_CONFIG_00;
    notice_info.disp_id[1] = DIVOOM_DISP_WIFI_CONFIG_01;
    notice_info.disp_id[2] = DIVOOM_DISP_WIFI_CONFIG_02;
	notice_info.on_time = 1;
	notice_info.period_time = 1;
	notice_info.tot_pic = 3;
	notice_info.tot_time = time;
    notice_info.noice_id = DIVOOM_MAIN_PLAY_WIFI_NOTICE_EVENT;
    notice_info.pover_callback = divoom_main_disp_power_over_notice;
    
	push_info.event_id = DIVOOM_MAIN_PLAY_WIFI_NOTICE_EVENT;
	push_info.model_id = DIVOOM_MODEL_MAIN;
    push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
    push_info.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
	if(request_flag == 0 || divoom_app_schedule_push(&push_info))
	{
        divoom_disp_ctrl_set_mode(DIVOOM_DISP_EVENT_DIS_CTRL, (void *)&notice_info);
        //divoom_main_play_voice_tip(DIVOOM_MAIN_PLAY_WIFI_CONNECT_EVENT, 1);
	}
	else
	{
		divoom_print("Here Can't get Scheduel Priority!");
	}
    #endif
}

/*
  Function:       divoom_main_disp_wifi_config
  Description:	   WIFI 配网过程中
  Input:          time
  Return:         NONE
*/

T_VOID divoom_main_notice_sd_free_warning(void)
{
	divoom_app_schedule_item_info push_info = {0};


	push_info.event_id = DIVOOM_MAIN_PLAY_SD_FREE_WARNING;
	push_info.model_id = DIVOOM_MODEL_MAIN;
    push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
	if(divoom_app_schedule_push(&push_info))
	{
        divoom_disp_Play_net_image_image_info notice_info = {0};

        notice_info.model_id = DIVOOM_MANAGE_IMAGE_NOTICE_NET_GIF;
        notice_info.event_id = DIVOOM_MAIN_PLAY_ALL_NOTICE_EVENT;
        notice_info.image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_NOTICE_NET_PIC + DIVOOM_lOCAL_USE_PIC_NOTICE_SD_ROOM_WARNING_MSG;
        divoom_disp_ctrl_set_mode(DIVOOM_DISP_PLAY_NET_IMAGE_CTRL, (void *)&notice_info);
	}
	else
	{
		divoom_print("Here Can't get Scheduel Priority!");
	}
}



T_VOID divoom_main_disp_wifi_connect_ok(int time)
{
    #if 0
	divoom_app_schedule_item_info push_info = {0};
    divoom_disp_event_disp_info notice_info = {0};
    

    notice_info.disp_id[0] = DIVOOM_DISP_WIFI_OK;
	notice_info.on_time = 10;
	notice_info.period_time = 10;
	notice_info.tot_pic = 1;
	notice_info.tot_time = time;
    notice_info.noice_id = DIVOOM_MAIN_PLAY_WIFI_NOTICE_EVENT;
    notice_info.pover_callback = divoom_main_disp_power_over_notice;
    notice_info.disp_title = divoom_main_get_disp_title(DIVOOM_NOTICE_LANUAGE_CON_OK_WIFI_MSG);
    
	push_info.event_id = DIVOOM_MAIN_PLAY_WIFI_NOTICE_EVENT;
	push_info.model_id = DIVOOM_MODEL_MAIN;
    push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
	if(divoom_app_schedule_push(&push_info))
	{
        divoom_disp_ctrl_set_mode(DIVOOM_DISP_EVENT_DIS_CTRL, (void *)&notice_info);
        //divoom_main_play_voice_tip(DIVOOM_MAIN_PLAY_WIFI_CONNECT_EVENT, 1);
	}
	else
	{
		divoom_print("Here Can't get Scheduel Priority!");
	}
    #endif
}


T_VOID divoom_main_disp_wifi_dis_con(void)
{
    #if 0
	divoom_app_schedule_item_info push_info = {0};
    divoom_disp_event_disp_info notice_info = {0};

    notice_info.disp_id[0] = DIVOOM_DISP_WIFI_FAIL;
	notice_info.on_time = 5;
	notice_info.period_time = 10;
	notice_info.tot_pic = 1;
	notice_info.tot_time = T_U16_MAX/2;
    notice_info.noice_id = DIVOOM_MAIN_PLAY_WIFI_NOTICE_EVENT;
    notice_info.pover_callback = divoom_main_disp_power_over_notice;
    notice_info.disp_title = divoom_main_get_disp_title(DIVOOM_NOTICE_LANUAGE_DISCON_WIFI_MSG);
    
	push_info.event_id = DIVOOM_MAIN_PLAY_WIFI_NOTICE_EVENT;
	push_info.model_id = DIVOOM_MODEL_MAIN;
    push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
	if(divoom_app_schedule_push(&push_info))
	{
        divoom_disp_ctrl_set_mode(DIVOOM_DISP_EVENT_DIS_CTRL, (void *)&notice_info);
	}
	else
	{
		divoom_print("Here Can't get Scheduel Priority!");
	}
    #endif
}
    
void divoom_main_disp_wifi_dis_time_out(void)
{
    #if 0
	divoom_app_schedule_item_info push_info = {0};
    divoom_disp_event_disp_info notice_info = {0};

    if(pDivoomHandle->pass_wifi_discon_notice)
    {
        return;
    }
    notice_info.disp_id[0] = DIVOOM_DISP_WIFI_TIMEOUT;
	notice_info.on_time = 5;
	notice_info.period_time = 10;
	notice_info.tot_pic = 1;
	notice_info.tot_time = 30;
    notice_info.noice_id = DIVOOM_MAIN_PLAY_WIFI_NOTICE_EVENT;
    notice_info.pover_callback = divoom_main_disp_power_over_notice;
    notice_info.disp_title = divoom_main_get_disp_title(DIVOOM_NOTICE_LANUAGE_DISCON_WIFI_MSG);
    
	push_info.event_id = DIVOOM_MAIN_PLAY_WIFI_NOTICE_EVENT;
	push_info.model_id = DIVOOM_MODEL_MAIN;
    push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
	if(divoom_app_schedule_push(&push_info))
	{
        divoom_disp_ctrl_set_mode(DIVOOM_DISP_EVENT_DIS_CTRL, (void *)&notice_info);
	}
	else
	{
		divoom_print("Here Can't get Scheduel Priority!");
	}
    #endif
}
    

void divoom_main_save_notice_dis(void)
{
    #if 0
	divoom_app_schedule_item_info push_info = {0};
    
	push_info.event_id = DIVOOM_MAIN_PLAY_ALL_NOTICE_EVENT;
	push_info.model_id = DIVOOM_MODEL_MAIN;
    push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
	if(divoom_app_schedule_push(&push_info))
	{        
        divoom_disp_Play_net_image_image_info notice_info = {0};

        notice_info.model_id = DIVOOM_MANAGE_IMAGE_NOTICE_NET_GIF;
        notice_info.event_id = DIVOOM_MAIN_PLAY_ALL_NOTICE_EVENT;
        notice_info.play_time = 1;
        notice_info.pcallback = divoom_main_disp_power_over_notice;
        notice_info.image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_NOTICE_NET_PIC + DIVOOM_lOCAL_USE_PIC_NOTICE_SAVE_CONFIG_MSG;
        divoom_disp_ctrl_set_mode(DIVOOM_DISP_PLAY_NET_IMAGE_CTRL, (void *)&notice_info);
	}
	else
	{
		divoom_print("Here Can't get Scheduel Priority!");
	}
    #endif
}

Pdivoom_system_setting_json_night_view divoom_main_get_night_view_info(void)
{
    return &pDivoomHandle->night_view;
}

void divoom_main_send_sys_settings_config(void)
{
	divoom_system_setting_json_system_cfg sys_config;

    sys_config.Time24Flag = pDivoomHandle->sys_setting.hour12_24;
	sys_config.TemperatureMode = pDivoomHandle->sys_setting.temp_c_f_flag;
	sys_config.time_format = pDivoomHandle->sys_setting.time_format;
	sys_config.Language = pDivoomHandle->sys_setting.lang;
	sys_config.SoundEffectMode = pDivoomHandle->sys_setting.audio_effect;
	//sys_config.SProt = pDivoomHandle->sys_setting.screen_save_mode;
	//sys_config.MirrorFlag = pDivoomHandle->sys_setting.mirror_flag;
	sys_config.AutoUpdateFlag = pDivoomHandle->sys_setting.auto_update;
	sys_config.NotificationSound = pDivoomHandle->sys_setting.volume;
	sys_config.ShowGridflag = pDivoomHandle->sys_setting.show_grid_flag;
	sys_config.ColorTemp = pDivoomHandle->sys_setting.color_temp;
	sys_config.WindSpeed_flag = pDivoomHandle->sys_setting.wind_speed_flag;

	divoom_net_app_send_packet(DIVOOM_NET_COMM_SET_SYS_CONF, DIVOOM_SERVER_TOPIC, &sys_config);
}

//系统音量
int divoom_main_get_tone_volume(void)
{
	if (pDivoomHandle == NULL) {
		if (divoom_product_test_is_active())
			return divoom_product_test_get_volume();
		return 0;
	}
	#ifdef DIVOOM_TONE_VOL_FLOOR
	if (pDivoomHandle->sys_setting.volume < DIVOOM_TONE_VOL_FLOOR && pDivoomHandle->sys_setting.volume != 0)
		return DIVOOM_TONE_VOL_FLOOR;
	#endif
    return pDivoomHandle->sys_setting.volume;
}

int divoom_main_get_poweron_mode(void)
{
    if (pDivoomHandle == NULL)
        return 0;
    return (int)pDivoomHandle->poweron_mode;
}

void divoom_main_finish_clock_wake_poweron(void)
{
	if (pDivoomHandle && pDivoomHandle->poweron_mode == DIVOOM_ONOFF_MODE_CLOCK)
		pDivoomHandle->poweron_mode = DIVOOM_ONOFF_MODE_NORMAL;
}

//客户产品代码
int divoom_main_get_custom_product_flag(void)
{
    if (pDivoomHandle == NULL)
        return 0;
    return pDivoomHandle->custom_product_flag;
}

//系统音量
void divoom_main_set_tone_volume(int volume)
{
	if (pDivoomHandle == NULL)
		return;
    pDivoomHandle->sys_setting.volume = volume;
}

Pdivoom_sys_setting_info divoom_main_get_system_config(void)
{
    return &pDivoomHandle->sys_setting;
}

void divoom_main_get_all_config(char *buf)
{
    divoom_channel_channel_cfg_info pconfig_info;
    
    divoom_light_app_get_change_clock_config(&pconfig_info);
    
    sprintf(buf, "{\"error_code\": 0, \"Brightness\":%d, \"RotationFlag\": %d, \"ClockTime\": %d,\"GalleryTime\": %d,\"SingleGalleyTime\": %d,\"PowerOnChannelId\": %d,\"GalleryShowTimeFlag\": %d,\"CurClockId\": %d,\"Time24Flag\": %d,\"TemperatureMode\": %d,\"GyrateAngle\": %d,\"MirrorFlag\": %d,\"LightSwitch\": %d}",
        pDivoomHandle->sys_setting.brightness, pconfig_info.RotationFlag,pconfig_info.ClockTime,pconfig_info.GalleryTime,
        pconfig_info.SingleGalleyTime,pconfig_info.ChannelIndex,pDivoomHandle->sys_setting.show_time_flag, divoom_light_app_get_cur_clock_id(),pDivoomHandle->sys_setting.hour12_24, pDivoomHandle->sys_setting.temp_c_f_flag, pDivoomHandle->sys_setting.screen_dir,pDivoomHandle->sys_setting.mirror_flag, divoom_light_app_get_screen_status(0xFF));
}

void divoom_main_load_product_flag(void)
{
    char *json_info;
	divoom_cJSON *root = NULL;

    json_info = divoom_app_com_read_file(DIVOOM_PRODUCT_FLAG_SAVE_PATH, NULL);
    if(json_info == NULL)
    {
        pDivoomHandle->custom_product_flag = 0;
        return;
    }
	root = divoom_cJSON_Parse(json_info);
    divoom_system_free(json_info);
    pDivoomHandle->custom_product_flag  = divoom_cJSON_GetObjectInt(root, "CustomFlag");
    divoom_cJSON_Delete(root);

    
}

void divoom_config_load_china_config(void)
{
    char *cmd_string;
    divoom_cJSON *root;

    pDivoomHandle->device_china_flag = 0;
    cmd_string = divoom_app_com_read_file(DIVOOM_NOT_SUPPORT_LANG_PATH, NULL);
    if(cmd_string == NULL)
    {
        return;
    }


	root = divoom_cJSON_Parse(cmd_string);
    divoom_system_free(cmd_string);
	if(!root)
	{
		return ;
	}
    pDivoomHandle->device_china_flag =
        (divoom_cJSON_GetObjectInt(root, "device_china_flag") == 1);
    
	divoom_cJSON_Delete(root);
    divoom_print("gdivoom_config_handle->device_china_flag: %d\n", pDivoomHandle->device_china_flag);
}

int divoom_config_set_china_config(int flag)
{
    if(flag > 0)
    {
        if(flag == 1)
        {
            if(pDivoomHandle->device_china_flag == 1)
            {
                pDivoomHandle->device_china_flag = 0;
                divoom_app_com_del_file(DIVOOM_NOT_SUPPORT_LANG_PATH);
                divoom_net_app_send_packet(DIVOOM_NET_COMM_APP_NOT_SUPPORT_SELECT_LANUAGE, DIVOOM_SERVER_TOPIC, &pDivoomHandle->device_china_flag);
            }
        }
        else
        {
            divoom_net_app_send_packet(DIVOOM_NET_COMM_APP_NOT_SUPPORT_SELECT_LANUAGE, DIVOOM_SERVER_TOPIC, &pDivoomHandle->device_china_flag);
        }
    }
    return pDivoomHandle->device_china_flag;
}

void divoom_main_get_key_light(uint8 *mode, uint8 *on_off, uint8 *brightness)
{
	if (pDivoomHandle == NULL)
		return;
	if (mode != NULL)
		*mode = pDivoomHandle->sys_setting.key_light_mode;
	if (on_off != NULL)
		*on_off = pDivoomHandle->sys_setting.key_light_on_off;
	if (brightness != NULL)
		*brightness = pDivoomHandle->sys_setting.key_light_brightness;
}

#ifdef DIVOOM_PRODUCT_ASTRO_TOO
static void divoom_main_key_light_fill_app_rgb(Pdivoom_channel_set_rgb_info rgb_info)
{
	uint8 mode;
	uint8 on_off;
	uint8 bright;

	if (rgb_info == NULL || pDivoomHandle == NULL)
	{
		return;
	}
	memset(rgb_info, 0, sizeof(*rgb_info));
	divoom_main_get_key_light(&mode, &on_off, &bright);
	rgb_info->on_off = on_off;
	rgb_info->KeyOnOff = on_off;
	rgb_info->brightness = bright;
	if (on_off && mode < (uint8)DIVOOM_BTCORE_KEY_LIGHT_MODE_MAX)
	{
		rgb_info->rgb_light_list[0].select_effect = (int)mode;
	}
}

void divoom_main_key_light_apply_from_app_rgb(Pdivoom_channel_set_rgb_info rgb_info)
{
	uint8 mode;
	uint8 on_off;
	uint8 bright;
	int effect;

	if (rgb_info == NULL || pDivoomHandle == NULL)
	{
		return;
	}

	bright = rgb_info->brightness;
	if (bright > DIVOOM_BTCORE_KEY_LIGHT_BRIGHT_MAX)
	{
		bright = DIVOOM_BTCORE_KEY_LIGHT_BRIGHT_MAX;
	}
	on_off = (rgb_info->on_off && rgb_info->KeyOnOff) ? 1u : 0u;
	effect = rgb_info->rgb_light_list[0].select_effect;
	if (effect < 0 || effect >= (int)DIVOOM_BTCORE_KEY_LIGHT_MODE_MAX)
	{
		effect = (int)DIVOOM_BTCORE_KEY_LIGHT_MODE_DEFAULT;
	}
	mode = (uint8)effect;

	if (on_off)
	{
		divoom_main_set_key_light(mode, 1u, bright);
		divoom_btcore_key_light_set_full(mode, 1u, bright);
	}
	else
	{
		divoom_main_get_key_light(&mode, NULL, &bright);
		divoom_main_set_key_light(mode, 0u, bright);
		divoom_btcore_key_light_off();
	}
}
#endif /* DIVOOM_PRODUCT_ASTRO_TOO */

int divoom_main_set_key_light(uint8 mode, uint8 on_off, uint8 brightness)
{
    if (pDivoomHandle == NULL)
		return -1;

    pDivoomHandle->sys_setting.key_light_mode = mode;
	pDivoomHandle->sys_setting.key_light_on_off = on_off;
	pDivoomHandle->sys_setting.key_light_brightness = brightness;
	divoom_main_save_config_local();
	return 0;
}

void divoom_main_key_light_cycle_next(void)
{
#ifdef DIVOOM_TIMES_VIBE
	uint8 mode;
	uint8 on_off;
	uint8 bright;

	if (pDivoomHandle == NULL)
		return;

	divoom_main_get_key_light(&mode, &on_off, &bright);
	if (mode >= (uint8)DIVOOM_BTCORE_KEY_LIGHT_MODE_MAX)
	{
		mode = 0;
	}
	if (!on_off)
	{
		/* 灭灯后再次长按：从灯效 1 开始 */
		mode = 0;
		on_off = 1;
	}
	else if (mode + 1u >= (uint8)DIVOOM_BTCORE_KEY_LIGHT_MODE_MAX)
	{
		/* 最后一档灯效后再切：灭灯（保留 mode 供设置页与下次恢复） */
		on_off = 0;
	}
	else
	{
		mode = (uint8)(mode + 1u);
	}
	divoom_main_set_key_light(mode, on_off, bright);
	if (on_off)
	{
		divoom_btcore_key_light_set(mode, on_off);
	}
	else
	{
		divoom_btcore_key_light_off();
	}
#else
	(void)0;
#endif
}


void divoom_main_load_config(void)
{

    char *json_info;
	divoom_cJSON *root = NULL, *list,*item;



    divoom_config_load_china_config();
    pDivoomHandle->weather_info.temp = 28;
    pDivoomHandle->weather_info.pressure = 10589;
    pDivoomHandle->weather_info.humidity = 75;
    pDivoomHandle->weather_info.visibility = 3200;
    pDivoomHandle->weather_info.wind_speed = 3.56;

    divoom_main_load_product_flag();

    json_info = divoom_app_com_read_file(DIVOOM_SYS_CFG_SAVE_PATH, NULL);
    if(json_info == NULL)
    {
        goto divoom_main_load_config_default_table;
    }
	root = divoom_cJSON_Parse(json_info);
    divoom_system_free(json_info);

    if(root == NULL)
    {
        divoom_main_load_config_default_table:
        divoom_app_com_del_file(DIVOOM_SYS_CFG_SAVE_PATH);
        memset(pDivoomHandle->server_type, 0, sizeof(pDivoomHandle->server_type));
        memset(&pDivoomHandle->sys_setting, 0, sizeof(pDivoomHandle->sys_setting));
		memset(&pDivoomHandle->blue_info, 0, sizeof(pDivoomHandle->blue_info));
		memset(&pDivoomHandle->device_info, 0, sizeof(pDivoomHandle->device_info));
        pDivoomHandle->blue_info.auto_connect_flag = 1;
        pDivoomHandle->blue_info.spp_switch = 0;
        strcpy(pDivoomHandle->blue_info.spp_secret,"0000");
        strcpy(pDivoomHandle->blue_info.blue_name, DIVOOM_BLUE_BLE_DEVICE_NAME);
        pDivoomHandle->sys_setting.bt_auto_connect = 1;
        pDivoomHandle->sys_setting.bt_secret_switch = 0;
        strcpy(pDivoomHandle->sys_setting.bt_password, "0000");
    	pDivoomHandle->sys_setting.brightness = 90;
    	pDivoomHandle->sys_setting.screen_switch = 1;
    	pDivoomHandle->sys_setting.temp_c_f_flag = 0;
    	pDivoomHandle->sys_setting.lang = pDivoomHandle->device_china_flag?DIVOOM_LVGL_MENU_LANG_SC:-1;        
    	pDivoomHandle->sys_setting.wifi_config_flag = 0;
		pDivoomHandle->device_wifi_status = 0;
        pDivoomHandle->sys_setting.mic_swtich = 0;
        pDivoomHandle->new_server_mode = 1;
        pDivoomHandle->sys_setting.server_flag = 2;
        pDivoomHandle->sys_setting.volume = 80;
        pDivoomHandle->sys_setting.music_volume_bt = 50;
        pDivoomHandle->sys_setting.music_volume_local = 50;
        pDivoomHandle->sys_setting.music_volume_linein = 50;
        pDivoomHandle->sys_setting.music_volume_uac = 50;
        pDivoomHandle->sys_setting.music_volume_netfm = 50;
        pDivoomHandle->sys_setting.beta_features = 0;
        pDivoomHandle->sys_setting.beta_test_flag_pending = 0;
        pDivoomHandle->sys_setting.volume_memory = 1;
        pDivoomHandle->sys_setting.key_light_mode = DIVOOM_BTCORE_KEY_LIGHT_MODE_DEFAULT;
        pDivoomHandle->sys_setting.key_light_on_off = 1;
        pDivoomHandle->sys_setting.key_light_brightness = DIVOOM_BTCORE_KEY_LIGHT_BRIGHT_DEFAULT;
        if(divoom_app_com_check_file_exist(DIVOOM_LIGHT_APP_BASE_LIGHT_SAVE_PATH) == 0)
        {
            divoom_print("we will reset config!  and set new device\n");
            pDivoomHandle->sys_setting.reset_flag = 1;
        }
        divoom_print("we will load default config!\n");
        return;
    }
    
    pDivoomHandle->local_clock_load_flag  = divoom_cJSON_GetObjectInt(root, "local_clock_load_flag");
    pDivoomHandle->new_server_mode  = divoom_cJSON_GetObjectInt(root, "new_server_mode");
    pDivoomHandle->update_firmware_flag = divoom_cJSON_GetObjectInt(root, "update_firmware_flag");
    pDivoomHandle->gif_mode = divoom_cJSON_GetObjectInt(root, "gif_mode");
    pDivoomHandle->system_file_update_time = divoom_cJSON_GetObjectInt(root, "system_file_update_time");
    item = divoom_cJSON_GetObjectItem(root, "sys_setting");
    pDivoomHandle->sys_setting.wifi_config_flag = divoom_cJSON_GetObjectInt(item, "wifi_config_flag");
#if DIVOOM_SHOW_MODE
    pDivoomHandle->sys_setting.show_store_sched_flag = divoom_cJSON_GetObjectInt(item, "show_store_sched_flag");
#endif
    pDivoomHandle->sys_setting.mic_swtich = divoom_cJSON_GetObjectInt(item, "mic_swtich");
    pDivoomHandle->sys_setting.tf_music_mode = divoom_cJSON_GetObjectInt(item, "tf_music_mode");
    pDivoomHandle->sys_setting.tf_music_order = divoom_cJSON_GetObjectInt(item, "tf_music_order");
    pDivoomHandle->sys_setting.tf_music_id = divoom_cJSON_GetObjectInt(item, "tf_music_id");
    pDivoomHandle->sys_setting.tf_music_bg = divoom_cJSON_GetObjectInt(item, "tf_music_bg");
    pDivoomHandle->sys_setting.server_flag = divoom_cJSON_GetObjectInt(item, "server_flag");
    pDivoomHandle->sys_setting.reset_flag = divoom_cJSON_GetObjectInt(item, "reset_flag");
    if(pDivoomHandle->device_china_flag)
    {
    	pDivoomHandle->sys_setting.lang = DIVOOM_LVGL_MENU_LANG_SC;
    }
    else
    {
        pDivoomHandle->sys_setting.lang = divoom_cJSON_GetObjectInt(item, "lang");
    }
    pDivoomHandle->sys_setting.user_lock = divoom_cJSON_GetObjectInt(item, "user_lock");
    pDivoomHandle->sys_setting.log_level = divoom_cJSON_GetObjectInt(item, "log_level");
    pDivoomHandle->sys_setting.brightness = divoom_cJSON_GetObjectInt(item, "brightness");
	pDivoomHandle->sys_setting.theme_flag = divoom_cJSON_GetObjectInt(item, "theme_flag");
	pDivoomHandle->sys_setting.audio_volume = divoom_cJSON_GetObjectInt(item, "audio_volume");
	pDivoomHandle->sys_setting.audio_effect = divoom_cJSON_GetObjectInt(item, "audio_effect");
	pDivoomHandle->sys_setting.volume_switch = divoom_cJSON_GetObjectInt(item, "volume_switch");
    pDivoomHandle->sys_setting.volume = divoom_cJSON_GetObjectInt(item, "volume");
    pDivoomHandle->sys_setting.volume_memory = divoom_cJSON_GetObjectInt(item, "volume_memory");
    pDivoomHandle->sys_setting.music_volume_bt = divoom_cJSON_GetObjectInt(item, "music_volume_bt");
    pDivoomHandle->sys_setting.music_volume_local = divoom_cJSON_GetObjectInt(item, "music_volume_local");
    pDivoomHandle->sys_setting.music_volume_linein = divoom_cJSON_GetObjectInt(item, "music_volume_linein");
    pDivoomHandle->sys_setting.music_volume_uac = divoom_cJSON_GetObjectInt(item, "music_volume_uac");
    pDivoomHandle->sys_setting.music_volume_netfm = divoom_cJSON_GetObjectInt(item, "music_volume_netfm");
    pDivoomHandle->sys_setting.beta_features = divoom_cJSON_GetObjectInt(item, "beta_features") == 1;
    pDivoomHandle->sys_setting.beta_test_flag_pending = divoom_cJSON_GetObjectInt(item, "beta_test_flag_pending") == 1;
    pDivoomHandle->sys_setting.auto_update = divoom_cJSON_GetObjectInt(item, "auto_update");
    pDivoomHandle->sys_setting.mirror_flag = divoom_cJSON_GetObjectInt(item, "mirror_flag");
    pDivoomHandle->sys_setting.screen_switch = divoom_cJSON_GetObjectInt(item, "screen_switch");
	pDivoomHandle->sys_setting.wind_speed_flag = divoom_cJSON_GetObjectInt(item, "wind_speed_flag");
	pDivoomHandle->sys_setting.temp_c_f_flag = divoom_cJSON_GetObjectInt(item, "temp_c_f_flag");
    pDivoomHandle->sys_setting.hour12_24 = divoom_cJSON_GetObjectInt(item, "hour12_24");
    pDivoomHandle->sys_setting.time_format = divoom_cJSON_GetObjectInt(item, "time_format");
    pDivoomHandle->sys_setting.screen_dir = divoom_cJSON_GetObjectInt(item, "screen_dir");
    pDivoomHandle->sys_setting.show_time_flag = divoom_cJSON_GetObjectInt(item, "show_time_flag");
    pDivoomHandle->sys_setting.screen_save_mode = divoom_cJSON_GetObjectInt(item, "screen_save_mode");
	pDivoomHandle->sys_setting.show_grid_flag = divoom_cJSON_GetObjectInt(item, "show_grid_flag");
	pDivoomHandle->sys_setting.dial_demo_flag = divoom_cJSON_GetObjectInt(item, "dial_demo_flag");
    pDivoomHandle->sys_setting.idle_poweroff_time = divoom_cJSON_GetObjectInt(item, "idle_poweroff_time");
    pDivoomHandle->device_info.device_id = divoom_cJSON_GetObjectInt(item, "device_id");
	pDivoomHandle->sys_setting.color_temp = divoom_cJSON_GetObjectInt(item, "color_temp");
    pDivoomHandle->sys_setting.key_light_mode = divoom_cJSON_GetObjectInt(item, "key_light_mode");
    pDivoomHandle->sys_setting.key_light_on_off = divoom_cJSON_GetObjectInt(item, "key_light_on_off");
    pDivoomHandle->sys_setting.key_light_brightness = divoom_cJSON_GetObjectInt(item, "key_light_brightness");

    divoom_cJSON_GetObjectString(item, "power_on_id",pDivoomHandle->sys_setting.power_on_id);
    divoom_cJSON_GetObjectString(item, "lat",pDivoomHandle->sys_setting.lat);
    divoom_cJSON_GetObjectString(item, "lot",pDivoomHandle->sys_setting.lot);
    divoom_cJSON_GetObjectString(item, "time_zone", pDivoomHandle->sys_setting.tzcode);
    
    item = divoom_cJSON_GetObjectItem(root, "blue_info");

    pDivoomHandle->blue_info.auto_connect_flag = divoom_cJSON_GetObjectInt(item, "auto_connect_flag");
    pDivoomHandle->blue_info.spp_switch = divoom_cJSON_GetObjectInt(item, "spp_switch");
    divoom_cJSON_GetObjectString(item, "spp_secret",pDivoomHandle->blue_info.spp_secret);
    divoom_cJSON_GetObjectString(item, "blue_name",pDivoomHandle->blue_info.blue_name);
    pDivoomHandle->sys_setting.bt_auto_connect = pDivoomHandle->blue_info.auto_connect_flag ? 1 : 0;
    pDivoomHandle->sys_setting.bt_secret_switch = pDivoomHandle->blue_info.spp_switch ? 1 : 0;
    strncpy(pDivoomHandle->sys_setting.bt_password, pDivoomHandle->blue_info.spp_secret, 4);
    pDivoomHandle->sys_setting.bt_password[4] = '\0';

	list = divoom_cJSON_GetObjectItem(root, "PowerOnInfo");
	if(list)
	{
		for(int i = 0; i < sizeof(pDivoomHandle->power_on_info)/sizeof(pDivoomHandle->power_on_info[0]); i++)
		{
			item = divoom_cJSON_GetArrayItem(list, i);

		    if(item != NULL)
		    {
		        pDivoomHandle->power_on_info[i].on_off = divoom_cJSON_GetObjectInt(item,"on_off");
		        pDivoomHandle->power_on_info[i].pow_on_id = divoom_cJSON_GetObjectInt(item,"pow_on_id");
		        pDivoomHandle->power_on_info[i].mode = divoom_cJSON_GetObjectInt(item,"mode");
		        pDivoomHandle->power_on_info[i].hour = divoom_cJSON_GetObjectInt(item,"hour");
		        pDivoomHandle->power_on_info[i].minute = divoom_cJSON_GetObjectInt(item,"minute");
		        pDivoomHandle->power_on_info[i].week = divoom_cJSON_GetObjectInt(item,"week");
		    }
		}
	}

    
    divoom_cJSON_Delete(root);

    /* 音量记忆固定开启，设置页不提供开关 */
    pDivoomHandle->sys_setting.volume_memory = 1;
}

//得到一个可以用的用户级的表盘（已分配 ID 记在 user_clock_id_list.json 的 ClockIdList 数组中）

/*
 * 读取 ClockIdList（文件按升序保存），在 [DIVOOM_PC_START_CLOCK_ID, DIVOOM_CLOUD_START_CLOCK_ID) 中取最小未占用 ID；
 * 无文件或无列表时返回 DIVOOM_PC_START_CLOCK_ID，60000 保留给 MCP。
 */
static int divoom_main_user_clock_id_next_available(void)
{
    const int lo = DIVOOM_PC_START_CLOCK_ID;
    const int hi = DIVOOM_CLOUD_START_CLOCK_ID;
    char *json_info;
    divoom_cJSON *root, *arr, *el;
    int n_arr, i, k, cand;

    json_info = divoom_app_com_read_file(DIVOOM_USER_CLOCK_ID_LIST_SAVE_PATH, NULL);
    if(json_info == NULL)
        return lo;

    root = divoom_cJSON_Parse(json_info);
    divoom_system_free(json_info);
    if(root == NULL)
        return lo;

    arr = divoom_cJSON_GetObjectItem(root, "ClockIdList");
    if(arr == NULL || !(arr->type & divoom_cJSON_Array) || divoom_cJSON_GetArraySize(arr) == 0)
    {
        divoom_cJSON_Delete(root);
        return lo;
    }

    n_arr = divoom_cJSON_GetArraySize(arr);
    cand = lo;
    for(i = 0; i < n_arr; i++)
    {
        el = divoom_cJSON_GetArrayItem(arr, i);
        if(el == NULL || !(el->type & divoom_cJSON_Number))
            continue;
        k = el->valueint;
        if(k < lo || k >= hi)
            continue;
        if(k < cand)
            continue;
        if(k > cand)
            break;
        cand++;
    }
    divoom_cJSON_Delete(root);

    if(cand >= hi)
        return -1;
    return cand;
}

static int divoom_main_user_clock_id_list_append_save(int clock_id, int local)
{
    const int lo = DIVOOM_USER_START_CLOCK_ID;
    const int hi = DIVOOM_CLOUD_START_CLOCK_ID;
    divoom_cJSON *root, *arr, *new_arr, *el;
    char *json_info;
    int *buf = NULL;
    int n_arr, i, nbuf, j, nf;
    int k;

    json_info = divoom_app_com_read_file(DIVOOM_USER_CLOCK_ID_LIST_SAVE_PATH, NULL);
    if(json_info != NULL)
    {
        root = divoom_cJSON_Parse(json_info);
        divoom_system_free(json_info);
    }
    else
        root = NULL;

    if(root == NULL)
        root = divoom_cJSON_CreateObject();
    if(root == NULL)
        return 0;

    arr = divoom_cJSON_GetObjectItem(root, "ClockIdList");
    n_arr = (arr != NULL && (arr->type & divoom_cJSON_Array)) ? divoom_cJSON_GetArraySize(arr) : 0;

    buf = (int *)divoom_system_malloc((size_t)(n_arr + 1) * sizeof(int));
    if(buf == NULL)
    {
        divoom_cJSON_Delete(root);
        return 0;
    }

    nf = 0;
    for(i = 0; i < n_arr; i++)
    {
        el = divoom_cJSON_GetArrayItem(arr, i);
        if(el == NULL || !(el->type & divoom_cJSON_Number))
            continue;
        k = el->valueint;
        if(k >= lo && k < hi)
            buf[nf++] = k;
    }
    /* 读出的 ID 已为升序，仅将新号插入有序位置 */
    k = clock_id;
    buf[nf] = k;
    for(i = nf - 1; i >= 0 && buf[i] > k; i--)
        buf[i + 1] = buf[i];
    buf[i + 1] = k;
    nbuf = nf + 1;

    j = 0;
    for(i = 0; i < nbuf; i++)
    {
        if(j > 0 && buf[i] == buf[j - 1])
            continue;
        buf[j++] = buf[i];
    }
    nbuf = j;

    divoom_cJSON_DeleteItemFromObject(root, "ClockIdList");
    new_arr = divoom_cJSON_CreateArray();
    if(new_arr == NULL)
    {
        divoom_system_free(buf);
        divoom_cJSON_Delete(root);
        return 0;
    }
    for(i = 0; i < nbuf; i++)
        divoom_cJSON_AddItemToArray(new_arr, divoom_cJSON_CreateNumber(buf[i]));
    divoom_cJSON_AddItemToObject(root, "ClockIdList", new_arr);
    divoom_system_free(buf);

    json_info = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);
    if(json_info != NULL)
    {
        int ok = 1;
        if(local) ok = divoom_lan_write(DIVOOM_USER_CLOCK_ID_LIST_SAVE_PATH, json_info, strlen(json_info));
        else divoom_app_com_write_file(DIVOOM_USER_CLOCK_ID_LIST_SAVE_PATH, json_info, strlen(json_info));
        divoom_system_free(json_info);
        return ok;
    }
    return 0;
}

/* Called only inside the LAN transaction; allocation is rolled back with assets/config. */
int divoom_main_get_new_local_clock_id(void)
{
    char *json = divoom_app_com_read_file(DIVOOM_USER_CLOCK_ID_LIST_SAVE_PATH, NULL);
    divoom_cJSON *root = json ? divoom_cJSON_Parse(json) : NULL;
    divoom_cJSON *arr = root ? divoom_cJSON_GetObjectItem(root, "ClockIdList") : NULL;
    int id, i, occupied;
    if(json) divoom_system_free(json);
    for(id = DIVOOM_PC_START_CLOCK_ID; id < DIVOOM_CLOUD_START_CLOCK_ID; id++) {
        occupied = divoom_lan_clock_exists(id);
        for(i = 0; !occupied && arr && i < divoom_cJSON_GetArraySize(arr); i++) {
            divoom_cJSON *entry = divoom_cJSON_GetArrayItem(arr, i);
            if(entry && entry->valueint == id) occupied = 1;
        }
        if(!occupied) break;
    }
    if(root) divoom_cJSON_Delete(root);
    if(id >= DIVOOM_CLOUD_START_CLOCK_ID || !divoom_main_user_clock_id_list_append_save(id, 1))
        return -1;
    return id;
}

int divoom_main_get_new_custon_clock_id(void)
{
    int clock_id;

    clock_id = divoom_main_user_clock_id_next_available();
    if(clock_id < 0)
    {
        divoom_print("divoom_main_get_new_custon_clock_id: pool exhausted\n");
        return -1;
    }

    divoom_main_user_clock_id_list_append_save(clock_id, 0);

    return clock_id;
    
}

void divoom_main_remove_custom_clcok(int *clock_id)
{
    if(clock_id== NULL  || *clock_id==0)
    {
        return;
    }

    divoom_clock_manage_del_clock(*clock_id);
    divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_REMOVE_LOCAL_CLOCK, DIVOOM_APP_TOPIC, clock_id);
}

void divoom_main_save_custom_clock_list(void *para )
{
    const int lo = DIVOOM_USER_START_CLOCK_ID;
    const int hi = DIVOOM_CLOUD_START_CLOCK_ID;
    Pdivoom_device_get_loacl_clock_list pclock_list = (Pdivoom_device_get_loacl_clock_list)para;
    divoom_cJSON *root, *new_arr;
    char *json_info, *out;
    int *buf = NULL;
    int i, j, nbuf, k;

    pDivoomHandle->local_clock_load_flag  = 1;
    divoom_main_save_config();

    if(pclock_list == NULL || pclock_list->plist == NULL || pclock_list->num <= 0)
    {
        return;
    }

    buf = (int *)divoom_system_malloc((size_t)pclock_list->num * sizeof(int));
    if(buf == NULL)
    {
        return;
    }

    nbuf = 0;
    for(i = 0; i < pclock_list->num; i++)
    {
        k = pclock_list->plist[i];
        if(k >= lo && k < hi)
        {
            buf[nbuf++] = k;
        }
    }

    for(i = 1; i < nbuf; i++)
    {
        k = buf[i];
        j = i - 1;
        while(j >= 0 && buf[j] > k)
        {
            buf[j + 1] = buf[j];
            j--;
        }
        buf[j + 1] = k;
    }

    j = 0;
    for(i = 0; i < nbuf; i++)
    {
        if(j > 0 && buf[i] == buf[j - 1])
        {
            continue;
        }
        buf[j++] = buf[i];
    }
    nbuf = j;

    json_info = divoom_app_com_read_file(DIVOOM_USER_CLOCK_ID_LIST_SAVE_PATH, NULL);
    if(json_info != NULL)
    {
        root = divoom_cJSON_Parse(json_info);
        divoom_system_free(json_info);
    }
    else
    {
        root = NULL;
    }
    if(root == NULL)
    {
        root = divoom_cJSON_CreateObject();
    }
    if(root == NULL)
    {
        divoom_system_free(buf);
        return;
    }

    divoom_cJSON_DeleteItemFromObject(root, "ClockIdList");
    new_arr = divoom_cJSON_CreateArray();
    if(new_arr == NULL)
    {
        divoom_system_free(buf);
        divoom_cJSON_Delete(root);
        return;
    }
    for(i = 0; i < nbuf; i++)
    {
        divoom_cJSON_AddItemToArray(new_arr, divoom_cJSON_CreateNumber(buf[i]));
    }
    divoom_cJSON_AddItemToObject(root, "ClockIdList", new_arr);
    divoom_system_free(buf);

    out = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);
    if(out != NULL)
    {
        divoom_app_com_write_file(DIVOOM_USER_CLOCK_ID_LIST_SAVE_PATH, out, strlen(out));
        divoom_system_free(out);
    }
}

void divoom_main_request_custon_clock_list(void)
{

    if(pDivoomHandle->local_clock_load_flag == 0)
    {
        divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_REMOVE_LOCAL_CLOCK, DIVOOM_SERVER_TOPIC, NULL);
    }
    
}

void divoom_main_del_config(void)
{
    char *json_info;
    divoom_cJSON *root, *setting, *device_id;
    int registered;

    /* Called before divoom_main(): inspect persisted state, not its handle.
     * A nonzero device_id is saved after a successful server response.
     * Language shares this configuration, so keep it for registered devices. */
    json_info = divoom_app_com_read_file(DIVOOM_SYS_CFG_SAVE_PATH, NULL);
    if (json_info == NULL)
    {
        /* Do not erase a user's settings on a transient read/allocation error. */
        return;
    }
    root = divoom_cJSON_Parse(json_info);
    divoom_system_free(json_info);
    if (root == NULL)
    {
        divoom_print("[product_test] config unreadable; preserve for normal boot\n");
        return;
    }
    setting = divoom_cJSON_GetObjectItem(root, "sys_setting");
    device_id = divoom_cJSON_GetObjectItem(setting, "device_id");
    registered = device_id != NULL && device_id->type == divoom_cJSON_Number
        && device_id->valueint != 0;
    divoom_cJSON_Delete(root);
    if (registered)
    {
        divoom_print("[product_test] registered device: preserve system config/language\n");
        return;
    }
    divoom_app_com_del_file(DIVOOM_SYS_CFG_SAVE_PATH);
}

void divoom_main_save_config_local(void)
{
    char *json_string;;
    divoom_cJSON *root, *item, *list;

    /* 产测用 minimal handle（memset），勿覆盖用户 system_config.cfg */
    if (divoom_product_test_is_active())
    {
        return;
	}
    if (pthread_mutex_lock(&s_userdata_config_lock) != 0)
        return;
    if (s_userdata_maintenance != DIVOOM_USERDATA_READY)
    {
        pthread_mutex_unlock(&s_userdata_config_lock);
        return;
    }
    pDivoomHandle->blue_info.auto_connect_flag = pDivoomHandle->sys_setting.bt_auto_connect ? 1 : 0;
    pDivoomHandle->blue_info.spp_switch = pDivoomHandle->sys_setting.bt_secret_switch ? 1 : 0;
    strncpy(pDivoomHandle->blue_info.spp_secret, pDivoomHandle->sys_setting.bt_password, 4);
    pDivoomHandle->blue_info.spp_secret[4] = '\0';
    
    root = divoom_cJSON_CreateObject();
    if(root == NULL)
    {
        pthread_mutex_unlock(&s_userdata_config_lock);
        return;
    }


    divoom_cJSON_AddNumberToObject(root, "local_clock_load_flag", pDivoomHandle->local_clock_load_flag);
    divoom_cJSON_AddNumberToObject(root, "gif_mode", pDivoomHandle->gif_mode);
    divoom_cJSON_AddNumberToObject(root, "new_server_mode", pDivoomHandle->new_server_mode);
    divoom_cJSON_AddNumberToObject(root, "update_firmware_flag", pDivoomHandle->update_firmware_flag);
    divoom_cJSON_AddNumberToObject(root, "system_file_update_time", pDivoomHandle->system_file_update_time);
	divoom_cJSON_AddItemToObject(root, "sys_setting", item = divoom_cJSON_CreateObject());
    if(pDivoomHandle->gif_mode)
    {
        divoom_cJSON_AddNumberToObject(item, "wifi_config_flag", 0);
        divoom_cJSON_AddNumberToObject(item, "lang", -1);
    }
    else
    {
        divoom_cJSON_AddNumberToObject(item, "wifi_config_flag", pDivoomHandle->sys_setting.wifi_config_flag);
        divoom_cJSON_AddNumberToObject(item, "lang", pDivoomHandle->sys_setting.lang);
    }
#if DIVOOM_SHOW_MODE
    divoom_cJSON_AddNumberToObject(item, "show_store_sched_flag", pDivoomHandle->sys_setting.show_store_sched_flag);
#endif
    divoom_cJSON_AddNumberToObject(item, "reset_flag", pDivoomHandle->sys_setting.reset_flag);    
    divoom_cJSON_AddNumberToObject(item, "mic_swtich", pDivoomHandle->sys_setting.mic_swtich);    
    divoom_cJSON_AddNumberToObject(item, "tf_music_mode", pDivoomHandle->sys_setting.tf_music_mode);
    divoom_cJSON_AddNumberToObject(item, "tf_music_order", pDivoomHandle->sys_setting.tf_music_order);
    divoom_cJSON_AddNumberToObject(item, "tf_music_id", pDivoomHandle->sys_setting.tf_music_id);
    divoom_cJSON_AddNumberToObject(item, "tf_music_bg", pDivoomHandle->sys_setting.tf_music_bg);
    divoom_cJSON_AddNumberToObject(item, "server_flag", pDivoomHandle->sys_setting.server_flag);
    divoom_cJSON_AddNumberToObject(item, "user_lock", pDivoomHandle->sys_setting.user_lock);
    divoom_cJSON_AddNumberToObject(item, "log_level", pDivoomHandle->sys_setting.log_level);
    divoom_cJSON_AddNumberToObject(item, "brightness", pDivoomHandle->sys_setting.brightness);
	divoom_cJSON_AddNumberToObject(item, "theme_flag", pDivoomHandle->sys_setting.theme_flag);
	divoom_cJSON_AddNumberToObject(item, "audio_volume", pDivoomHandle->sys_setting.audio_volume);
	divoom_cJSON_AddNumberToObject(item, "audio_effect", pDivoomHandle->sys_setting.audio_effect);
	divoom_cJSON_AddNumberToObject(item, "volume_switch", pDivoomHandle->sys_setting.volume_switch);
    divoom_cJSON_AddNumberToObject(item, "volume", pDivoomHandle->sys_setting.volume);
    divoom_cJSON_AddNumberToObject(item, "volume_memory", pDivoomHandle->sys_setting.volume_memory);
    divoom_cJSON_AddNumberToObject(item, "music_volume_bt", pDivoomHandle->sys_setting.music_volume_bt);
    divoom_cJSON_AddNumberToObject(item, "music_volume_local", pDivoomHandle->sys_setting.music_volume_local);
    divoom_cJSON_AddNumberToObject(item, "music_volume_linein", pDivoomHandle->sys_setting.music_volume_linein);
    divoom_cJSON_AddNumberToObject(item, "music_volume_uac", pDivoomHandle->sys_setting.music_volume_uac);
    divoom_cJSON_AddNumberToObject(item, "music_volume_netfm", pDivoomHandle->sys_setting.music_volume_netfm);
    divoom_cJSON_AddNumberToObject(item, "beta_features", pDivoomHandle->sys_setting.beta_features);
    divoom_cJSON_AddNumberToObject(item, "beta_test_flag_pending", pDivoomHandle->sys_setting.beta_test_flag_pending);
    divoom_cJSON_AddNumberToObject(item, "auto_update", pDivoomHandle->sys_setting.auto_update);
    divoom_cJSON_AddNumberToObject(item, "mirror_flag", pDivoomHandle->sys_setting.mirror_flag);
    divoom_cJSON_AddNumberToObject(item, "screen_switch", pDivoomHandle->sys_setting.screen_switch);
	divoom_cJSON_AddNumberToObject(item, "wind_speed_flag", pDivoomHandle->sys_setting.wind_speed_flag);    
	divoom_cJSON_AddNumberToObject(item, "temp_c_f_flag", pDivoomHandle->sys_setting.temp_c_f_flag);
    divoom_cJSON_AddNumberToObject(item, "hour12_24", pDivoomHandle->sys_setting.hour12_24);
    divoom_cJSON_AddNumberToObject(item, "time_format", pDivoomHandle->sys_setting.time_format);
    divoom_cJSON_AddNumberToObject(item, "screen_dir", pDivoomHandle->sys_setting.screen_dir);
    divoom_cJSON_AddNumberToObject(item, "show_time_flag", pDivoomHandle->sys_setting.show_time_flag);
    divoom_cJSON_AddNumberToObject(item, "screen_save_mode", pDivoomHandle->sys_setting.screen_save_mode);
	divoom_cJSON_AddNumberToObject(item, "show_grid_flag", pDivoomHandle->sys_setting.show_grid_flag);
	divoom_cJSON_AddNumberToObject(item, "dial_demo_flag", pDivoomHandle->sys_setting.dial_demo_flag);
    divoom_cJSON_AddNumberToObject(item, "idle_poweroff_time", pDivoomHandle->sys_setting.idle_poweroff_time);
    divoom_cJSON_AddNumberToObject(item, "device_id", pDivoomHandle->device_info.device_id);
	divoom_cJSON_AddNumberToObject(item, "color_temp", pDivoomHandle->sys_setting.color_temp);
	divoom_cJSON_AddNumberToObject(item, "key_light_mode", pDivoomHandle->sys_setting.key_light_mode);
	divoom_cJSON_AddNumberToObject(item, "key_light_on_off", pDivoomHandle->sys_setting.key_light_on_off);
	divoom_cJSON_AddNumberToObject(item, "key_light_brightness", pDivoomHandle->sys_setting.key_light_brightness);

	divoom_cJSON_AddStringToObject(item, "power_on_id", pDivoomHandle->sys_setting.power_on_id);
    divoom_cJSON_AddStringToObject(item, "lat", pDivoomHandle->sys_setting.lat);
    divoom_cJSON_AddStringToObject(item, "lot", pDivoomHandle->sys_setting.lot);
    divoom_cJSON_AddStringToObject(item, "time_zone", pDivoomHandle->sys_setting.tzcode);

    
	divoom_cJSON_AddItemToObject(root, "blue_info", item = divoom_cJSON_CreateObject());
    divoom_cJSON_AddNumberToObject(item, "auto_connect_flag", pDivoomHandle->blue_info.auto_connect_flag);
    divoom_cJSON_AddNumberToObject(item, "spp_switch", pDivoomHandle->blue_info.spp_switch);
    divoom_cJSON_AddStringToObject(item, "spp_secret", pDivoomHandle->blue_info.spp_secret);
    divoom_cJSON_AddStringToObject(item, "blue_name", pDivoomHandle->blue_info.blue_name);


	divoom_cJSON_AddItemToObject(root, "PowerOnInfo", list = divoom_cJSON_CreateArray());
    for(int i = 0; i < sizeof(pDivoomHandle->power_on_info)/sizeof(pDivoomHandle->power_on_info[0]); i++)
	{
		divoom_cJSON_AddItemToArray(list, item = divoom_cJSON_CreateObject());
		if(item)
		{
			divoom_cJSON_AddNumberToObject(item, "on_off", pDivoomHandle->power_on_info[i].on_off);
			divoom_cJSON_AddNumberToObject(item, "pow_on_id", pDivoomHandle->power_on_info[i].pow_on_id);
		    divoom_cJSON_AddNumberToObject(item, "mode", pDivoomHandle->power_on_info[i].mode);
			divoom_cJSON_AddNumberToObject(item, "hour", pDivoomHandle->power_on_info[i].hour);
			divoom_cJSON_AddNumberToObject(item, "minute", pDivoomHandle->power_on_info[i].minute);
			divoom_cJSON_AddNumberToObject(item, "week", pDivoomHandle->power_on_info[i].week);
		}
    }

    json_string = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);
    if (json_string != NULL)
    {
	    divoom_app_com_write_file(DIVOOM_SYS_CFG_SAVE_PATH,json_string, strlen(json_string));
        divoom_system_free(json_string);
    }
    pthread_mutex_unlock(&s_userdata_config_lock);

}

void divoom_main_save_config(void)
{
    divoom_main_save_config_local();
}

/*********************************************************************
  Function:         divoom_main_mqtt_del_discon_info
  Description:      清除某个ＩＰ的连接的信息
  Input:            discon_ip       
                    
  Return:           NONE
  Author:           albert
**********************************************************************/

void divoom_main_mqtt_del_discon_info(char *discon_ip)
{
    int i;
	divoom_system_get_lock(pDivoomHandle->app_list_lock);    
    if(pDivoomHandle->app_list)
    {
        //divoom_print("divoom_main_mqtt_notice: %d\n",pDivoomHandle->app_list->num);
        for(i = 0; i < pDivoomHandle->app_list->num; i++)
        {
            if(strcmp(discon_ip, pDivoomHandle->app_list->list[i].ip_addr) == 0)
            {
				memset(&pDivoomHandle->app_list->list[i], 0, sizeof(divoom_device_app_id_item));
				//pDivoomHandle->app_list->num--;
				//divoom_system_free(&pDivoomHandle->app_list->list[i]);
				divoom_system_free_lock(pDivoomHandle->app_list_lock);
                return;
            }
        }
    }
	divoom_system_free_lock(pDivoomHandle->app_list_lock);
}


/*********************************************************************
  Function:         divoom_main_mqtt_connect_app_mqtt_ctrl
  Description:      对列表中的ＭＱＴＴ连接进行管理
  Input:            index     
                    on_off
                    
  Return:           NONE
  Author:           albert
**********************************************************************/

void  divoom_main_mqtt_connect_app_mqtt_ctrl(int index, int on_off)
{
    
}


/*********************************************************************
  Function:         divoom_main_mqtt_remove_app_mqtt_connect
  Description:      通知ＭＱＴＴ模块，断开某个ＩＰ的连接
  Input:            discon_ip       
                    
  Return:           NONE
  Author:           albert
**********************************************************************/

void  divoom_main_mqtt_remove_app_mqtt_connect(char *ip_addr)
{
    
}
#define DIVOOM_MAIN_MQTT_RESET_TIMES 3

void  divoom_main_mqtt_notice(char *con_ip, uint8 flag)
{

}


uint8  divoom_main_mqtt_check_app_mqtt_reset(void)
{
	return 0;
}

/*********************************************************************
  Function:         divoom_main_manage_app_con
  Description:      开机后更新一连接,先断开所有连接，再启动所有连接
  Input:            discon_ip       
                    
  Return:           NONE
  Author:           albert
**********************************************************************/

void  divoom_main_manage_app_con(Pdivoom_device_app_id_list app_list)
{
}

/*********************************************************************
  Function:         divoom_main_manage_update_app_con
  Description:      更新某个的连接信息
  Input:            discon_ip       
                    
  Return:           NONE
  Author:           albert
**********************************************************************/

void  divoom_main_manage_update_app_con(Pdivoom_device_app_id_list app_list)
{
    
}

//断开所有的APP的连接
void divoom_main_manage_dis_all_app_con(void)
{
}

//APP心跳包
void divoom_main_manage_accept_app_hearbeat(int user_id)
{
}

//APP心跳包
void divoom_main_manage_send_connect_app(int user_id)
{
}

//没有收到心跳包重新连接所有的APP的连接
void divoom_main_manage_reconnect_all_app_con(void)
{
}

int divoom_main_update_log_level(int log_level)
{
    if(pDivoomHandle == NULL)
    {
        return 0;
    }
    if(log_level >= 0)
    {
        if(pDivoomHandle->sys_setting.log_level != log_level)
        {
            pDivoomHandle->sys_setting.log_level = log_level;
            divoom_main_save_config();
            if(log_level)
            {
                divoom_log_start();
            }
        }
    }
    return pDivoomHandle->sys_setting.log_level;
}


void divoom_main_set_wifi_err_status(int status)
{
    if(pDivoomHandle->wifi_err_status == -1)
    {
        pDivoomHandle->wifi_err_status = status;
    }
}

void divoom_main_check_dns_type(void)
{
    int i;
    char *net_data, *url_info;

#if defined(DIVOOM_SIMULATOR) && !SIM_THREAD_REAL_DEVICE
    /* 模拟器 poll 模式默认不探测外网 DNS/IP，避免在主线程 poll 里多次阻塞 curl */
    pDivoomHandle->use_https_flag = 0;
    pDivoomHandle->use_ip_flag = 1;
    divoom_print("sim: skip dns probe, use_ip_flag=1\n");
    return;
#endif

    //第一次先使用域名请求
    url_info = divoom_system_malloc(256);
    pDivoomHandle->use_https_flag = 0;
    #if 1
    if(pDivoomHandle->sys_setting.server_flag == 2)
    {
        strcpy(url_info, DIVOOM_SERVER_HTTP_NORMAL_URL);
    }
    else if(pDivoomHandle->sys_setting.server_flag)
    {
        strcpy(url_info, DIVOOM_SERVER_HTTP_USA_URL);
    }
    else
    {
        strcpy(url_info, DIVOOM_SERVER_HTTP_CHINA_URL);
    }
    strcat(url_info, "/Test/GetIP");
    for(i = 0; i < 3; i ++)
    {
        net_data = divoom_http_client_url_request(url_info, NULL, 0, NULL, NULL);
        if(net_data)
        {
            divoom_print("we will use dns!: %s\n", url_info);
            pDivoomHandle->use_ip_flag = 0;
            divoom_system_free(url_info);
            divoom_system_free(net_data);
            return;
        }
    }
    #endif

    if(pDivoomHandle->sys_setting.server_flag == 2)
    {
        strcpy(url_info, DIVOOM_SERVER_HTTPS_NORMAL_URL);
    }
    else if(pDivoomHandle->sys_setting.server_flag)
    {
        strcpy(url_info, DIVOOM_SERVER_HTTPS_USA_URL);
    }
    else
    {
        strcpy(url_info, DIVOOM_SERVER_HTTPS_CHINA_URL);
    }
    strcat(url_info, "/Test/GetIP");
    for(i = 0; i < 3; i ++)
    {
        net_data = divoom_http_client_url_request(url_info, NULL, 0,NULL, NULL);
        if(net_data)
        {
            divoom_print("we will use dns!\n");
            pDivoomHandle->use_ip_flag = 0;
            pDivoomHandle->use_https_flag = 1;
            divoom_system_free(url_info);
            divoom_system_free(net_data);
            return;
        }
    }

    //域名请求失败，将使用IP方式
    if(pDivoomHandle->sys_setting.server_flag == 2)
    {
        strcpy(url_info, DIVOOM_SERVER_HTTP_NORMAL_IP);
    }
    else if(pDivoomHandle->sys_setting.server_flag)
    {
        strcpy(url_info, DIVOOM_SERVER_HTTP_USA_IP);
    }
    else
    {
        strcpy(url_info, DIVOOM_SERVER_HTTP_CHINA_IP);
    }
    strcat(url_info, "/Test/GetIP");
    for(i = 0; i < 3; i ++)
    {
        net_data = divoom_http_client_url_request(url_info, NULL, 0, NULL, NULL);
        if(net_data)
        {
            divoom_system_free(net_data);
            break;
        }
    }

    if(i < 3)
    {
        //IP请求失败后就返回
        divoom_sls_log_upload("check_dns", "it fail https!\n");    
        divoom_print("we will use ip! (all probes failed)\n");
        pDivoomHandle->use_ip_flag = 1;
        divoom_system_free(url_info);
        return;
    }
#if 0
    //IP请求成功后再试一下域名请求能否成功
    if(pDivoomHandle->sys_setting.server_flag == 2)
    {
        strcpy(url_info, DIVOOM_SERVER_HTTP_NORMAL_URL);
    }
    else if(pDivoomHandle->sys_setting.server_flag)
    {
        strcpy(url_info, DIVOOM_SERVER_HTTP_USA_URL);
    }
    else
    {
        strcpy(url_info, DIVOOM_SERVER_HTTP_CHINA_URL);
    }
    strcat(url_info, "/Test/GetIP");
    for(i = 0; i < 3; i ++)
    {
        net_data = divoom_http_client_url_request(url_info, NULL, 0, NULL, NULL);
        if(net_data)
        {
            divoom_print("we will use dns! %s\n", url_info);
            pDivoomHandle->use_ip_flag = 0;
            divoom_system_free(url_info);
            divoom_system_free(net_data);
            return;
        }
    }
#endif
    divoom_system_free(url_info);
    divoom_print("we will use ip!\n");
    pDivoomHandle->use_ip_flag = 1;
    divoom_sls_log_upload("check_dns", "it fail all!\n");

}
//300000003-1721439418852-5.webp

void divoom_main_test_aliyun_download_file(void)
{
    divoom_download_aliyun_file_item file_info = {0};

    strcpy(file_info.os_name, "300000003-1721439418852-5.webp");
    strcpy(file_info.photo_title , "this is test!\n");
    divoom_download_aliyun_file_add_item(&file_info, 1, NULL,0);
}

static void divoom_main_route_after_add_network_success(void)
{
    uint32 host_id;

    host_id = divoom_main_get_host_id();
    divoom_print("[WIFI_CFG] route after add network host_id=%u\n",
        (unsigned int)host_id);
    if (host_id == 0)
    {
        /* MQTT worker 只投递标记，弹窗由添加网络页的 LVGL timer 创建。 */
        if (!divoom_astrootoo_settings_wifi_request_app_pairing_prompt())
        {
            divoom_print("[WIFI_CFG] pairing prompt skipped: add network page inactive\n");
        }
        return;
    }

    /* 已绑定用户仅在「添加网络」页完成分流时回表盘，设置 Wi-Fi 状态页不切走。 */
    if (divoom_menu_control_get_active_menu_id()
        != DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_WIFI_ADD)
    {
        divoom_print("[WIFI_CFG] skip dial: not on add network page\n");
        return;
    }
    divoom_main_return_startup_dial();
}

void divoom_main_return_startup_dial(void)
{
    void *dial_user_data;

    /* 先显示缓存表盘，避免菜单栈底为主菜单时出现中间页；表盘事件待页面切换完成后再投递。 */
    dial_user_data = divoom_light_app_get_current_clock_info();
    if (dial_user_data == NULL)
    {
        divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP,
            DIVOOM_LIGHT_APP_EVENT_START_UP_DISP_DIAL);
        return;
    }

    s_startup_dial_event_pending = 1;
    divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID,
        dial_user_data, DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_KEY_LIGHT_EVENT);
}

static void divoom_main_poll_startup_dial_event(void)
{
    if (!s_startup_dial_event_pending
        || divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID)
    {
        return;
    }

    /* 表盘已经可见后再通知 Light APP 刷新资源，避免与首次建页并发。 */
    s_startup_dial_event_pending = 0;
    divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP,
        DIVOOM_LIGHT_APP_EVENT_START_UP_DISP_DIAL);
}

/* 开机落到表盘后若磁盘仍有留言则弹顶栏；本启动只尝试一次。 */
static void divoom_main_poll_boot_leave_msg_notice(void)
{
    static uint8 s_boot_leave_notice_done;
    static uint8 s_boot_leave_notice_dial_stable;
    int alarm_remain = 0;
    int alarm_st;

    if (s_boot_leave_notice_done)
        return;
    if (!divoom_main_get_finish_start_up())
        return;
    if (divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID) {
        s_boot_leave_notice_dial_stable = 0;
        return;
    }
    /* 闹钟开机：表盘可能先于响铃页出现，须等关掉闹钟后 mode 回到 NORMAL。 */
    if (divoom_main_get_poweron_mode() == DIVOOM_ONOFF_MODE_CLOCK) {
        s_boot_leave_notice_dial_stable = 0;
        return;
    }
    alarm_st = divoom_alarm_get_alarm_active_status(&alarm_remain);
    if (alarm_st == DIVOOM_ALARM_STATUS_ACTIVE
        || alarm_st == DIVOOM_ALARM_STATUS_SNOOZE
        || alarm_st == DIVOOM_ALARM_STATUS_INTTRUPT) {
        s_boot_leave_notice_dial_stable = 0;
        return;
    }
    /* 再等一拍 main_loop（约 100ms），避开表盘 CLEAR_ALL_LOAD/pinit 同拍 add_top。 */
    if (!s_boot_leave_notice_dial_stable) {
        s_boot_leave_notice_dial_stable = 1;
        return;
    }
    s_boot_leave_notice_done = 1;
    divoom_leave_msg_ctrl_notice_if_clip_on_disk();
}

#ifdef DIVOOM_SIMULATOR
void divoom_main_sim_route_after_add_network_success(uint32 host_id)
{
    if (pDivoomHandle == NULL)
    {
        return;
    }
    pDivoomHandle->host_id = host_id;
    divoom_main_route_after_add_network_success();
}
#endif

int32 divoom_main_check_ip_region(void);

int divoom_main_get_ip_region(void)
{
    return pDivoomHandle->ip_region;
}

static volatile int s_net_lyric_source = DIVOOM_LYRIC_SOURCE_OFF;

int divoom_main_net_lyric_source(void)
{
    return pDivoomHandle && divoom_wifi_model_get_wifi_status() ?
        s_net_lyric_source : DIVOOM_LYRIC_SOURCE_OFF;
}

int divoom_main_net_lyric_region_off(void)
{
    return divoom_main_net_lyric_source() == DIVOOM_LYRIC_SOURCE_OFF;
}

/* A new region decision invalidates requests, including same-provider reconnects. */
static void divoom_main_set_lyric_country(const char *code)
{
    divoom_port_critical_enter();
    s_net_lyric_source = divoom_lyric_source_for_country(code);
    if (pDivoomHandle)
        pDivoomHandle->net_lyric_region_off = code ?
            (s_net_lyric_source == DIVOOM_LYRIC_SOURCE_OFF) : 0xFF;
    divoom_port_critical_exit();
    divoom_lyric_demand_region_changed();
}

int32 divoom_main_check_ip_region(void);

uint8 divoom_main_manage_connect_service_mqtt(void)
{
    divoom_device_ser_mqtt_info_json_request device_info;
	uint8 mac[6], try_times = 0;
    divoom_sys_msg_info model_msg = {0};  
    Pdivoom_net_command ret_data = NULL;

    if (pDivoomHandle == NULL || divoom_product_test_is_active())
    {
        return 0;
    }
    /* 拆网 abort 若漏清，initv2 会被直接打回。这里再清一次无副作用。 */
    divoom_http_client_request_abort_clear();

    divoom_sls_log_init();

#if defined(DIVOOM_SIMULATOR) && !SIM_THREAD_REAL_DEVICE
    /* poll 模式：HTTP worker 未创建时不走同步 initv2 */
    if (!divoom_net_http_manage_sim_worker_active())
    {
        divoom_print("sim: skip connect_service_mqtt (enable http worker for cloud)\n");
        return 0;
    }
#endif
    
    int ret = divoom_main_get_device_mac_addr(mac);

	if (ret <= 0)
	{
		return 0;
	}
	
    device_info.mqtt_con_time = pDivoomHandle->mqtt_con_time;
    device_info.reset_all_flag = pDivoomHandle->wifi_config_reset_all_flag;
    device_info.hot_user_id = pDivoomHandle->app_id;
    device_info.bond_flag = pDivoomHandle->bond_flag;
    if(pDivoomHandle->sys_setting.lang == -1 && pDivoomHandle->back_lang != -1)
    {
        pDivoomHandle->sys_setting.lang = pDivoomHandle->back_lang;
    }
    device_info.language = pDivoomHandle->sys_setting.lang;
    device_info.HardWare = DIVOOM_HARDWARE_VERSION;
    device_info.device_id = divoom_main_get_device_id();
    device_info.product_flag = pDivoomHandle->custom_product_flag;
    
    pDivoomHandle->wifi_config_reset_all_flag = 0;
    #if 1
    if(device_info.bond_flag == 0)
    {
        if(pDivoomHandle->new_server_mode == 0)
        {
            pDivoomHandle->sys_setting.server_flag = 2;
        }
    }
    pDivoomHandle->ip_region = divoom_main_check_ip_region();
    divoom_print("servertype : %d, %d\n", pDivoomHandle->sys_setting.server_flag, pDivoomHandle->china_region);
    if(pDivoomHandle->sys_setting.server_flag == 0 && pDivoomHandle->china_region == 0)
    {
        pDivoomHandle->sys_setting.server_flag = 1;
    }
    device_info.server_type = pDivoomHandle->sys_setting.server_flag;
    #endif
    divoom_main_check_dns_type();
    divoom_print("use_ip_flag is : %d\n", pDivoomHandle->use_ip_flag);
	sprintf(device_info.device_name, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    while(try_times < 20)
    {
	    ret_data = divoom_net_app_send_packet_http(DIVOOM_NET_COMM_GET_SER_MQTT_INFO, &device_info, 1);
        if(ret_data)
        {
            break;
        }
        pDivoomHandle->use_ip_flag = 1;
        divoom_print("we will try again get initv2!\n");
        try_times ++;
        divoom_system_delay_ms(500);
    }
	if (ret_data)
	{
		Pdivoom_device_ser_mqtt_info_json mqtt_info = (Pdivoom_device_ser_mqtt_info_json)ret_data->cmd_data;

        if (mqtt_info == NULL)
        {
            divoom_net_app_free_packet(ret_data);
            return 0;
        }
		divoom_print("dev_id: %d dev_ip: %s  dev_token: %s: new: %d, port: %d\n", mqtt_info->dev_id, mqtt_info->dev_ip, mqtt_info->dev_token, mqtt_info->new_device, mqtt_info->mqtt_port);

		Pdivoom_server_msg_struct srv_info = divoom_system_malloc(sizeof(divoom_server_msg_struct));
        struct timeval utc_time_info = {0};
        if (srv_info == NULL)
        {
            divoom_net_app_free_packet(ret_data);
            return 0;
        }

        gettimeofday(&utc_time_info, NULL);
 		if(utc_time_info.tv_sec != mqtt_info->utc_time)
        {
            int update_flag = abs(utc_time_info.tv_sec - mqtt_info->utc_time) > 60;
            utc_time_info.tv_sec = mqtt_info->utc_time;
            srand(mqtt_info->utc_time);
            settimeofday(&utc_time_info, NULL);
            if(update_flag)
            {
                divoom_system_init_system_time();
            }
        }

		/* 先根据服务器时间校准系统时间，再同步到杰理 RTC（本机次只 SET 一次） */
		divoom_onoff_sync_rtc_after_server_time();

		srv_info->port = 8883;
        
        pDivoomHandle->bond_flag = 0;
        pDivoomHandle->mqtt_con_time ++;
		strcpy(srv_info->clinet_id, device_info.device_name);
		strcpy(srv_info->username, device_info.device_name);
        if(mqtt_info->new_device)
        {
            pDivoomHandle->sys_setting.reset_flag = 0;
        }
        if(pDivoomHandle->use_ip_flag == 0)
        {
            strcpy(srv_info->host, "open.divoom-gz.com");
        }
        else
        {
            strcpy(srv_info->host, mqtt_info->dev_ip);
        }
		strcpy(srv_info->password, mqtt_info->dev_token);
        if(device_info.device_id  == 0 && DIVOOM_CUSTOME_CLOCK_ID != 0)
        {
            mqtt_info->last_clock_id = DIVOOM_CUSTOME_CLOCK_ID;
        }
		srv_info->device_id = mqtt_info->dev_id;
        pDivoomHandle->device_info.device_id = mqtt_info->dev_id;
		divoom_net_http_manage_update_device_id(mqtt_info->dev_id);
		divoom_net_mqtt_manage_update_device_id(mqtt_info->dev_id);
        if(device_info.device_id == 0)
        {
            Pdivoom_net_command clear_cmd = NULL;
            
            clear_cmd = divoom_net_app_send_packet_http(DIVOOM_NET_COMM_DEVICE_CLEAR_USER_ALBUM, NULL, 1);
            divoom_net_app_free_packet(clear_cmd);
        }
        if (mqtt_info->user_id == 100)
        {
            pDivoomHandle->host_id = 0;
        }
        else
        {
            pDivoomHandle->host_id = mqtt_info->user_id;

        }

        if(pDivoomHandle->host_id == 100)
        {
            pDivoomHandle->host_id = 0;
        }
        pDivoomHandle->reset_flag = mqtt_info->reset_flag;
        pDivoomHandle->new_server_mode = 1;        
        pDivoomHandle->sys_setting.server_flag = mqtt_info->server_type;
        if(pDivoomHandle->sys_setting.server_flag != mqtt_info->server_type)
    	{
            pDivoomHandle->update_flag = 1;
            pDivoomHandle->sys_setting.server_flag = mqtt_info->server_type;
        }
        if(mqtt_info->last_clock_id && pDivoomHandle->sys_setting.reset_flag == 0)
        {
            divoom_light_app_update_last_clock_id(mqtt_info->last_clock_id);
        }
        
        if(pDivoomHandle->sys_setting.log_level != mqtt_info->log_level)
        {
		    pDivoomHandle->sys_setting.log_level = mqtt_info->log_level;
            //divoom_main_save_config();
        }
        if(pDivoomHandle->sys_setting.log_level)
        {
            divoom_log_start();
        }
		divoom_main_save_config();
		divoom_main_set_device_position(mqtt_info->lot_str, mqtt_info->lat_str);
		time_t cur_time;
		struct tm local_info;
		time(&cur_time);
		localtime_r(&cur_time, &local_info);
        pDivoomHandle->init_time_flag = 1;
		divoom_print("set time: %d, %d, %d\n",mqtt_info->utc_time, local_info.tm_hour, local_info.tm_min);
		divoom_sys_model_crl_send_asyn_all_model(DIVOOM_DEVICE_EVENT_TIME_INIT_OK);
        pDivoomHandle->wifi_err_status = -1;

        srand(cur_time + divoom_system_get_tickms());
        pDivoomHandle->first_request_flag = 0;
        pDivoomHandle->divoom_first_connect = 1;
        
		model_msg.msg_type = 1;
		model_msg.src_id = DIVOOM_MODEL_MAIN;
		model_msg.dest_id = DIVOOM_MODEL_SER_CON;		   
		model_msg.msg_id = DIVOOM_SERVER_MSG_CONNECT_DIVOOM; 
		model_msg.data = (uint8 *)srv_info;
		divoom_sys_model_crl_msg_push(&model_msg);
		divoom_net_app_free_packet(ret_data);
	        divoom_clock_schedule_init();
	        divoom_ble_set_wifi_status(DIVOOM_WIFI_CONNECTED_SER);

            if (s_wifi_add_network_route_pending)
            {
                /* 完成时读取最新标记，避免已由连接码页消费后旧请求再次切到表盘。 */
                s_wifi_add_network_route_pending = 0;
                divoom_main_route_after_add_network_success();
            }

			return 1;
	}
	return 0;
}

#ifdef DIVOOM_SIMULATOR
void divoom_main_sim_poll(void)
{
	uint8 ret;

	divoom_main_poll_startup_dial_event();
	divoom_main_poll_boot_leave_msg_notice();
	if (!s_sim_pending_wifi_mqtt_connect)
	{
		return;
	}
	s_sim_pending_wifi_mqtt_connect = 0;
	divoom_print("[sim_main] deferred connect_service_mqtt on main_loop\n");
	ret = divoom_main_manage_connect_service_mqtt();
	if (pDivoomHandle)
	{
		if (ret)
		{
			pDivoomHandle->http_request_ok = 1;
		}
		else
		{
			pDivoomHandle->http_request_ok = 0;
		}
		pDivoomHandle->http_request_frist = 1;
	}
}
#endif

void divoom_main_get_http_request(uint32 tick)
{
	if (pDivoomHandle == NULL)
		return;
	if (pDivoomHandle->http_request_frist == 1 && pDivoomHandle->http_request_ok == 0 && tick % 1000 == 0)
	{
#if DIVOOM_BUILD_RTOS_DEVICE
		divoom_sys_msg_info msg = {0};

		msg.msg_id = DIVOOM_MAIN_EVENT_RETRY_MQTT_CONNECT;
		msg.src_id = DIVOOM_MODEL_MAIN;
		msg.dest_id = DIVOOM_MODEL_MAIN;
		msg.msg_type = 1;
		divoom_sys_model_crl_msg_push(&msg);
#else
		uint8 ret = divoom_main_manage_connect_service_mqtt();
		if (ret)
			pDivoomHandle->http_request_ok = 1;
		else
			pDivoomHandle->http_request_ok = 0;
#endif
	}
}

void divoom_main_close_screen(uint8 power_off_flag)
{
    divoom_sys_msg_info model_msg = {0};  
    
    model_msg.src_id = DIVOOM_MODEL_MAIN;
    model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;
	//根据是16*16,还是16*48决定使用 DIVOOM_LIGHT_APP_EVENT_SMALL_DRAW 或者 DIVOOM_LIGHT_APP_EVENT_BIG_PLAY;
    //model_msg.msg_id = DIVOOM_DISP_APP_POWER_OFF_MODE;
    //model_msg.attr = DIVOOM_LIGHT_APP_CTRL_NORMAL;
	model_msg.data = &power_off_flag;
    divoom_sys_model_crl_msg_push(&model_msg);
}

//实现保存全部的数据,然后断电关机
//这个函数是主动关机,不通过任何按键控制关机
void divoom_main_power_off(uint8 voice_flag, uint8 power_off_flag)
{
    divoom_sys_model_crl_send_all_model(DIVOOM_APP_CONFIG_SAVE_EVENT_ID);
    //if(voice_flag)
    //{
    //    divoom_voice_info_play_default_voice(DIVOOM_VOICE_ID_POWEROFF, 100, NULL, 0);
    //}
    divoom_main_close_screen(power_off_flag==0?2:1);

#if 0
	//保留逻辑，后面整好闹钟从新启用
    divoom_sys_msg_info model_msg = {0};

    model_msg.src_id = DIVOOM_MODEL_MAIN;
    model_msg.dest_id = DIVOOM_MODEL_ONOFF;
    model_msg.msg_id = DIVOOM_APP_ONOFF_SOFT_POWEROFF;
    divoom_sys_model_crl_msg_push(&model_msg);
#endif
}

void divoom_main_disp_battery_info(uint8 flag)
{
    #if 0
    //divoom_battery_get_bat_level
    divoom_disp_event_disp_info notice_info = {0};
	divoom_sys_msg_info model_msg = {0};
	model_msg.src_id = DIVOOM_MODEL_MAIN;
    notice_info.disp_id[0] = DIVOOM_DISP_BATTERY_PIC;
    
    if(flag)
    {
    	notice_info.on_time = 1;
    	notice_info.period_time = 1;
    	notice_info.tot_pic = 1;
    	notice_info.tot_time = 10;
    }
    else
    {
    	notice_info.on_time = 10;
    	notice_info.period_time = 10;
    	notice_info.tot_pic = 1;
    	notice_info.tot_time = 10;
    }
	model_msg.msg_type = 0;			
	model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
	//根据是16*16,还是16*48决定使用 DIVOOM_LIGHT_APP_EVENT_SMALL_DRAW 或者 DIVOOM_LIGHT_APP_EVENT_BIG_PLAY;
	//model_msg.msg_id = DIVOOM_DISP_EVENT_DIS_CTRL;			
	model_msg.data = (uint8 *)&notice_info;
	divoom_sys_model_crl_msg_push(&model_msg);
    #endif
}

void divoom_main_disp_vol_info(uint8 mode, uint8 dir, uint8 vol)
{
#if 0
    divoom_disp_event_disp_info disp_info = {0};
	divoom_sys_msg_info model_msg = {0};
    int pic_len;
    uint8 *disp_buf = NULL, i, j;
    divoom_screen_color_info fill_color = {0xff,0xff,0xff}, back_color = {50,50,50};
    /*if(divoom_battery_get_lowpower_flag())
    {
        if(vol >= 6)
        {
			divoom_main_disp_low_battery();
            return;
        }
    }*/
    disp_info.on_time = 15;
    disp_info.period_time = 15;
    disp_info.tot_time = 15;
    disp_info.tot_pic = 1;
    if(vol == 0)
    {
        disp_info.disp_id[0] = DIVOOM_DISP_VOLUME_MUTE_PIC;
    }
    else if(vol >= DIVOOM_MAX_VOL_VALUE)
    {
        disp_info.disp_id[0] = DIVOOM_DISP_VOLUME_MAX_PIC;
    }
    else 
    {
        disp_buf = divoom_system_malloc(SCREEN_BUF_SIZE);
        if(disp_buf == AK_NULL)
        {
            divoom_print("%s:%d: it fails to malloc!\n",__FILE__, __LINE__);
            return ;
        }
        if(dir)
        {
            divoom_disp_picture_get_rgb_data(DIVOOM_DISP_VOLUME_ADD_PIC, disp_buf);
        }
        else
        {
            divoom_disp_picture_get_rgb_data(DIVOOM_DISP_VOLUME_SUB_PIC, disp_buf);
        }
        for(i = 13; i < 15; i ++)
        {
            for(j = 0; j < SCREEN_WIDTH; j ++)
            {
                if(vol > j)
                {
                    divoom_disp_ctrl_set_data(i *SCREEN_WIDTH + j ,&fill_color, disp_buf);
                }
                else
                {
                    divoom_disp_ctrl_set_data(i *SCREEN_WIDTH + j ,&back_color, disp_buf);
                }
            }
        }
        disp_info.tot_pic = 0;
		disp_info.pic_data = divoom_image_encode_encode_mul_pic(disp_buf, 1, &pic_len, 100);
        disp_info.pic_len = pic_len;
    }
    
    model_msg.src_id = mode;
    model_msg.msg_type = 0;         
    model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
	//根据是16*16,还是16*48决定使用 DIVOOM_LIGHT_APP_EVENT_SMALL_DRAW 或者 DIVOOM_LIGHT_APP_EVENT_BIG_PLAY;
    //model_msg.msg_id = DIVOOM_DISP_EVENT_DIS_CTRL;          
    model_msg.data = (uint8 *)&disp_info;
    divoom_sys_model_crl_msg_push(&model_msg);

    
	if(disp_info.pic_data)
	{
		Fwl_Free(disp_info.pic_data);
	}
#endif
}

void divoom_main_test_url_info(void)
{
    char *net_data;

    divoom_print("test url11111111111111!\n");
    net_data = divoom_http_client_url_request("https://fortnite-api.com/v1/stats/br/v2?name=BH%20nixxxay&accountType=epic", NULL, 0, NULL, NULL);

    if(net_data)
    {
        divoom_print("request data: %s\n", net_data);
        divoom_system_free(net_data);
    }
    else
    {
        divoom_print("request data: NULL\n");
    }

    
    divoom_print("test url22222222222222!\n");
    net_data = divoom_http_client_url_request("https://fortnite-api.com/v1/stats/br/v2?name=BH%20nixxxay&accountType=epic", NULL, 0, NULL, NULL);

    if(net_data)
    {
        divoom_print("request data: %s\n", net_data);
        divoom_system_free(net_data);
    }
    else
    {
        divoom_print("request data: NULL\n");
    }
    
    divoom_print("test url3333333!\n");
    net_data = divoom_http_client_url_request("https://fortnite-api.com/v1/stats/br/v2?name=BH%20nixxxay&accountType=epic", NULL, 0, NULL, NULL);

    if(net_data)
    {
        divoom_print("request data: %s\n", net_data);
        divoom_system_free(net_data);
    }
    else
    {
        divoom_print("request data: NULL\n");
    }
}

typedef struct
{
    uint32 start_ms;
    uint32 request_ms;
} divoom_main_ip_region_query;

/* 仅取消本次请求，不影响其他 HTTP 业务；耗时以传输层轮询取消为准。 */
static int divoom_main_ip_region_query_exit(void *user_data)
{
    divoom_main_ip_region_query *query = user_data;
    uint32 now = divoom_system_get_tickms();

    return divoom_wifi_model_get_wifi_status() == 0 ||
        (uint32)(now - query->start_ms) >= 10000U ||
        (uint32)(now - query->request_ms) >= 5000U;
}

static int divoom_main_ip_region_parse(const char *response, const char *field,
    char code[3])
{
    divoom_cJSON *root;
    divoom_cJSON *success;
    divoom_cJSON *error;
    divoom_cJSON *country_code;
    int valid = 0;
    int i;

    root = divoom_cJSON_ParseWithOpts(response, NULL, 1);
    if (root == NULL)
    {
        return 0;
    }
    success = divoom_cJSON_GetObjectItem(root, "success");
    error = divoom_cJSON_GetObjectItem(root, "error");
    country_code = divoom_cJSON_GetObjectItem(root, field);
    /* 有 success 时必须为 true；不把限流/错误响应中的地区字段当作成功。 */
    if ((root->type & 0xFF) == divoom_cJSON_Object &&
        (success == NULL || (success->type & 0xFF) == divoom_cJSON_True) &&
        (error == NULL || (error->type & 0xFF) == divoom_cJSON_False) &&
        country_code != NULL && (country_code->type & 0xFF) == divoom_cJSON_String &&
        country_code->valuestring != NULL && strlen(country_code->valuestring) == 2)
    {
        for (i = 0; i < 2; ++i)
        {
            char ch = country_code->valuestring[i];
            code[i] = ch >= 'a' && ch <= 'z' ? ch - 'a' + 'A' : ch;
        }
        code[2] = '\0';
        valid = code[0] >= 'A' && code[0] <= 'Z' &&
            code[1] >= 'A' && code[1] <= 'Z' &&
            strcmp(code, "ZZ") != 0 && strcmp(code, "XX") != 0;
    }
    divoom_cJSON_Delete(root);
    return valid;
}

int32 divoom_main_check_ip_region(void)
{
    static const struct
    {
        char *url;
        const char *field;
    } providers[] = {
        {"https://api.country.is/", "country"},
        {"https://free.freeipapi.com/api/v1/json/", "countryCode"}
    };
    divoom_main_ip_region_query query;
    char code[3];
    unsigned int i;

    pDivoomHandle->china_region = 0xff;
    divoom_main_set_lyric_country(NULL);
    query.start_ms = divoom_system_get_tickms();
    for (i = 0; i < sizeof(providers) / sizeof(providers[0]); ++i)
    {
        char *response;
        int valid = 0;

        query.request_ms = divoom_system_get_tickms();
        if (divoom_main_ip_region_query_exit(&query))
        {
            break;
        }
        response = divoom_http_client_url_request(providers[i].url, NULL, 0,
            divoom_main_ip_region_query_exit, &query);
        if (response != NULL)
        {
            if (!divoom_main_ip_region_query_exit(&query))
            {
                valid = divoom_main_ip_region_parse(response, providers[i].field,
                    code);
            }
            divoom_system_free(response);
        }
        if (!valid)
        {
            divoom_print("ip region: provider %u failed\n", i + 1);
            continue;
        }
        divoom_main_set_lyric_country(code);
        divoom_print("ip region: provider %u country=%s\n", i + 1, code);
        pDivoomHandle->china_region = (strcmp(code, "CN") == 0);
        /* 保持原有规则；CN 仅匹配中国大陆，HK/MO 单独处理。 */
        return strcmp(code, "JP") == 0 || strcmp(code, "TW") == 0 ||
            strcmp(code, "CN") == 0 || strcmp(code, "MY") == 0 ? 0 : 1;
    }
    return -1;
}


void divoom_main_poweroff_play_over_voice(uint8 status)
{
	if(pDivoomHandle->power_onoff_flag[1])
	{
		divoom_sys_msg_info model_msg = {0};
		model_msg.msg_type = 1;
		model_msg.msg_id = DIVOOM_MAIN_EVENT_KEY_POWEROFF;
		model_msg.src_id = DIVOOM_MODEL_MAIN;
		model_msg.dest_id = DIVOOM_MODEL_MAIN;
		divoom_sys_model_crl_msg_push(&model_msg);

//		divoom_onoff_set_sleep();
	}
    pDivoomHandle->power_onoff_flag[0] = 0xbb;
}

void divoom_main_lowpower_play_over_voice(uint8 status)
{
//    pDivoomHandle->play_over_flag = 1;
    divoom_sys_msg_info model_msg = {0};  

	model_msg.msg_type = 1;
    model_msg.src_id = DIVOOM_MODEL_MAIN;
    model_msg.dest_id = DIVOOM_MODEL_MAIN;          
    model_msg.msg_id = DIVOOM_MAIN_EVENT_END_PLAY_VOIC_LOWPOWER;
	divoom_sys_model_crl_msg_push(&model_msg);
}

void divoom_main_bt_connect_play_over_voice(uint8 status)
{
    divoom_print("%s: play over!\n", __func__);
//    pDivoomHandle->play_over_flag = 1;
    divoom_sys_msg_info model_msg = {0};  

	model_msg.msg_type = 1;
    model_msg.src_id = DIVOOM_MODEL_MAIN;
    model_msg.dest_id = DIVOOM_MODEL_MAIN;          
    model_msg.msg_id = DIVOOM_MAIN_EVENT_END_PLAY_VOIC_BT_CONNECT;
	divoom_sys_model_crl_msg_push(&model_msg);
}



/*
   400000006--|
   400000007--|
   400000012--|-->达人图片
   400000015--|
   400006376--|

   DIVOOM_DISP_APP_SET_LIST_CATEGORY_LIST
category: 1-->Default 4-->表情 7-->符号

   400001941-->空图库，看是否能给清除掉
   */

/*eq:"group1/M00/05/6B/eEwpPVw0QBCENHuKAAAAADN7MsM2620395"

  "group1/M00/05/6A/eEwpPVwz7GaEdOcxAAAAACIKu7k8984123"

  "group1/M00/05/21/L1ghblwzz8iEcp_NAAAAAFsE0Os7009936"

  "group1/M00/05/6A/eEwpPVwzumCERyifAAAAAIRLMeY3349620"

  "group1/M00/05/21/L1ghblwzuNKEUnQVAAAAAKXp7KA8581631"

  "group1/M00/05/6A/eEwpPVwzuDeETziHAAAAAO985iw7101382"

  "group1/M00/05/6A/eEwpPVwzto-EBTS7AAAAAOPxAjc8318166"

  "group1/M00/05/21/L1ghblwztIyEViqSAAAAACexkUg9469078"

  "group1/M00/05/68/eEwpPVwzL9KEBfrcAAAAAPHxpME9477537"

  "group1/M00/05/1F/L1ghblwzLyqEFTduAAAAAF0uBy83660415"

  "group1/M00/05/68/eEwpPVwzLvKES7iCAAAAAIIML5Q6854582"

  "group1/M00/05/1F/L1ghblwzLjiELFtVAAAAAIHQ67g7746178"

  "group1/M00/05/1F/L1ghblwzLF6EdMQmAAAAAPK8Sok9450375"

  "group1/M00/05/1F/L1ghblwzKuuEGMV6AAAAAF71U_03598919"

  gif
  "group1/M00/05/1F/L1ghblwzKkCEVq4ZAAAAAGB88V83385140"
  "group1/M00/06/7E/L1ghblykb8eEMf8WAAAAAKBbd0k.422821"
  "group1/M00/06/7E/L1ghblykbBWENDI9AAAAAMbS2-8.092151"
  "group1/M00/06/7E/L1ghblykayiEWXP2AAAAALDCgzc8878128"
  "group1/M00/06/C6/eEwpPVykZPGEFV4KAAAAAH6fLng9016197"
  "group1/M00/06/7E/L1ghblykZNKETPXJAAAAALKXRAs.670889"
  "group1/M00/06/C6/eEwpPVykYiKEGeHhAAAAACKor5A.273982"
  "group1/M00/06/7D/L1ghblykV-yENtnSAAAAANbsH7k2830789"
  "group1/M00/06/7D/L1ghblykToWEDGSRAAAAAJTqVZI.778040"
  "group1/M00/06/C6/eEwpPVykQlmELVfZAAAAAAY11Es.297025"
  "group1/M00/06/C5/eEwpPVykPdmEExZKAAAAAPeL49I.041275"
  "group1/M00/06/C5/eEwpPVykOyGEHGASAAAAAM8TeBU9190180"
  "group1/M00/06/C5/eEwpPVykNpWEdxEKAAAAAHVQC2s.714446"
  "group1/M00/06/C5/eEwpPVykNKaET22sAAAAANmWHBo.672536"
  "group1/M00/06/7D/L1ghblykNGuEfcKRAAAAAJcwuuk8491092"
  "group1/M00/06/C5/eEwpPVykMGqEe2EOAAAAAEG685U0099098"
  "group1/M00/06/7D/L1ghblykMAyEeE39AAAAAGjZ5u0.186155"
  pic
  "group1/M00/06/C6/eEwpPVykapGEfFUkAAAAAD5wGZo4774002"
  "group1/M00/06/C6/eEwpPVykYx2EF0S5AAAAAPi8Tiw6603373"
  "group1/M00/06/7D/L1ghblykXh-EXeehAAAAAHtvXY00761211"
  "group1/M00/06/7D/L1ghblykXXOEHQUDAAAAAF5zd2Q1295363"
  "group1/M00/06/C5/eEwpPVykBpeEKflGAAAAAD7Dk2Y1650186"
  "group1/M00/06/C5/eEwpPVykBO-EGRABAAAAAGsKOWw6836720"
  "group1/M00/06/7D/L1ghblykAQWEPk8hAAAAAFXL3r02971610"
  "group1/M00/06/7D/L1ghblyj7fCENhODAAAAABlM1R09995153"
  "group1/M00/06/C5/eEwpPVyjxlqEZo95AAAAAIMKHJA.227853"
  "group1/M00/06/7C/L1ghblyjlj-ELLD0AAAAAPQDEec4715454"
  "group1/M00/06/7C/L1ghblyjlZKEEj6uAAAAAFnb6Ek2548919"
  "group1/M00/06/C4/eEwpPVyjlSeEHTlaAAAAAIdY0Gw1504747"
  "group1/M00/06/C4/eEwpPVyjkdKEKs24AAAAALiWSPk0747681"
  "group1/M00/06/C4/eEwpPVyjguiECi7nAAAAAPpkms48911387"
  "group1/M00/06/7B/L1ghblyjgs2EU-heAAAAAJ4k8q83236440"
  "group1/M00/06/C4/eEwpPVyjgk6EW2P_AAAAADv19lc8766260"
  */

/*void divoom_shaco_test_userdefine(uint8 flag)
{
	static int time = 100;
	uint32 i = 0, rand_num = 0;
	char temp[64];
	divoom_sys_msg_info model_msg = {0};
	divoom_light_user_info frame_info;
	//divoom_light_user_item frame_need[15]; 
	Pdivoom_light_user_item frame_need = NULL;
	uint8 all_num = 0;
	if(flag)
	{
		all_num = 5;
		frame_need = divoom_system_malloc(sizeof(divoom_light_user_item)*5);
		strcpy((char *)frame_need[0].frame_addr, "group1/M00/05/1F/L1ghblwzKkCEVq4ZAAAAAGB88V83385140");
		strcpy((char *)frame_need[1].frame_addr, "group1/M00/06/7E/L1ghblykb8eEMf8WAAAAAKBbd0k.422821");
		strcpy((char *)frame_need[2].frame_addr, "group1/M00/06/7E/L1ghblykbBWENDI9AAAAAMbS2-8.092151");
		strcpy((char *)frame_need[3].frame_addr, "group1/M00/06/7E/L1ghblykayiEWXP2AAAAALDCgzc8878128");
		strcpy((char *)frame_need[4].frame_addr, "group1/M00/06/C6/eEwpPVykZPGEFV4KAAAAAH6fLng9016197");
		//strcpy((char *)frame_need[5].frame_addr, "group1/M00/06/7E/L1ghblykZNKETPXJAAAAALKXRAs.670889");
		//strcpy((char *)frame_need[6].frame_addr, "group1/M00/06/C6/eEwpPVykYiKEGeHhAAAAACKor5A.273982");
		//strcpy((char *)frame_need[7].frame_addr, "group1/M00/06/7D/L1ghblykV-yENtnSAAAAANbsH7k2830789");
		//strcpy((char *)frame_need[8].frame_addr, "group1/M00/06/7D/L1ghblykToWEDGSRAAAAAJTqVZI.778040");
		//strcpy((char *)frame_need[9].frame_addr, "group1/M00/06/C6/eEwpPVykQlmELVfZAAAAAAY11Es.297025");
		//strcpy((char *)frame_need[10].frame_addr, "group1/M00/06/C6/eEwpPVykapGEfFUkAAAAAD5wGZo4774002");
		//strcpy((char *)frame_need[11].frame_addr, "group1/M00/06/C6/eEwpPVykYx2EF0S5AAAAAPi8Tiw6603373");
		//strcpy((char *)frame_need[12].frame_addr, "group1/M00/06/7D/L1ghblykXh-EXeehAAAAAHtvXY00761211");
		//strcpy((char *)frame_need[13].frame_addr, "group1/M00/06/7D/L1ghblykXXOEHQUDAAAAAF5zd2Q1295363");
		//strcpy((char *)frame_need[14].frame_addr, "group1/M00/06/C5/eEwpPVykBpeEKflGAAAAAD7Dk2Y1650186");
		//frame_info.frame_list_num = 15;
	}
	else
	{
		all_num = 3;
		frame_need = divoom_system_malloc(sizeof(divoom_light_user_item)*3);
		strcpy((char *)frame_need[0].frame_addr, "group1/M00/06/7D/L1ghblykMAyEeE39AAAAAGjZ5u0.186155");
		strcpy((char *)frame_need[1].frame_addr, "group1/M00/06/C6/eEwpPVykapGEfFUkAAAAAD5wGZo4774002");
		strcpy((char *)frame_need[2].frame_addr, "group1/M00/06/7C/L1ghblyjlZKEEj6uAAAAAFnb6Ek2548919");
		//strcpy((char *)frame_need[5].frame_addr, "group1/M00/06/7E/L1ghblykZNKETPXJAAAAALKXRAs.670889");
		//strcpy((char *)frame_need[6].frame_addr, "group1/M00/06/C6/eEwpPVykYiKEGeHhAAAAACKor5A.273982");
		//strcpy((char *)frame_need[7].frame_addr, "group1/M00/06/7D/L1ghblykV-yENtnSAAAAANbsH7k2830789");
		//strcpy((char *)frame_need[8].frame_addr, "group1/M00/06/7D/L1ghblykToWEDGSRAAAAAJTqVZI.778040");
		//strcpy((char *)frame_need[9].frame_addr, "group1/M00/06/C6/eEwpPVykQlmELVfZAAAAAAY11Es.297025");
		//strcpy((char *)frame_need[10].frame_addr, "group1/M00/06/C6/eEwpPVykapGEfFUkAAAAAD5wGZo4774002");
		//strcpy((char *)frame_need[11].frame_addr, "group1/M00/06/C6/eEwpPVykYx2EF0S5AAAAAPi8Tiw6603373");
		//strcpy((char *)frame_need[12].frame_addr, "group1/M00/06/7D/L1ghblykXh-EXeehAAAAAHtvXY00761211");
		//strcpy((char *)frame_need[13].frame_addr, "group1/M00/06/7D/L1ghblykXXOEHQUDAAAAAF5zd2Q1295363");
		//strcpy((char *)frame_need[14].frame_addr, "group1/M00/06/C5/eEwpPVykBpeEKflGAAAAAD7Dk2Y1650186");
		//frame_info.frame_list_num = 15;
	}
	for(i=0;i<all_num;i++)
	{
		divoom_print("Old Url--%d--:%s", i, frame_need[i].frame_addr);
	}
	frame_info.frame_list_num = all_num;
	frame_info.frame_list = frame_need;
	frame_info.update_time = time;
	time += 100;
	frame_info.Is_set_mode = 0;
	frame_info.Is_set_info = 1;
    model_msg.src_id = DIVOOM_MODEL_MAIN;
    model_msg.dest_id = DIVOOM_MODEL_LIGHT_CONTROL;
	model_msg.msg_id = DIVOOM_LIGHT_CONTROL_SET_USER_DEFINE_INFO;
	model_msg.data = (uint8 *)&frame_info;

	divoom_sys_model_crl_msg_push(&model_msg);
	divoom_free(frame_need);
#if 0
	vTaskDelay(500 / portTICK_PERIOD_MS);

	for(i=0;i<7;i++)
	{
		rand_num = esp_random()%14;
		strcpy(temp, (char *)frame_need[rand_num].frame_addr);
		temp[33] = 0;
		temp[34] = 0;
		temp[35] = 0;
		temp[36] = 0;
		strcpy((char *)frame_need[rand_num].frame_addr, (char *)frame_need[14-rand_num].frame_addr);
		strcpy((char *)frame_need[14-rand_num].frame_addr, temp);
	}
	for(i=0;i<14;i++)
	{
		strcpy((char *)frame_need[rand_num].frame_addr, (char *)frame_need[14-rand_num].frame_addr);
		strcpy((char *)frame_need[14-rand_num].frame_addr, temp);
	}
	for(i=0;i<14;i++)
	{
		divoom_print("2--%d---old strig:%s", i, frame_need[i].frame_addr);
	}
	frame_info.update_time = time;
	frame_info.Is_set_mode = 0;
	frame_info.Is_set_info = 1;
	frame_info.frame_list = frame_need;
    model_msg.src_id = DIVOOM_MODEL_MAIN;
    model_msg.dest_id = DIVOOM_MODEL_LIGHT_CONTROL;
	model_msg.msg_id = DIVOOM_LIGHT_CONTROL_SET_USER_DEFINE_INFO;
	model_msg.data = (uint8 *)&frame_info;
	divoom_sys_model_crl_msg_push(&model_msg);
#endif
}

void divoom_shaco_test_hot(void)
{
	divoom_sys_msg_info model_msg = {0};
	divoom_light_hot_ctrl hot_info;
	Pdivoom_light_hot_info hot_data = divoom_system_malloc(sizeof(divoom_light_hot_info)*2);
	hot_info.Is_set_info = 1;
	hot_info.Is_set_mode = 0;
	hot_info.all_hot_num = 2;
	hot_info.hot_info = hot_data;
	hot_data[0].user_id = 30001;
	hot_data[0].version = 1000;
	hot_data[0].all_num = 15;
	hot_data[0].addr_info = divoom_system_malloc(sizeof(divoom_light_hot_addr)*15);
	strcpy(hot_data[0].addr_info[0].url_list, "group1/M00/05/6B/eEwpPVw0QBCENHuKAAAAADN7MsM2620395");
	strcpy(hot_data[0].addr_info[1].url_list, "group1/M00/05/6A/eEwpPVwz7GaEdOcxAAAAACIKu7k8984123");
	strcpy(hot_data[0].addr_info[2].url_list, "group1/M00/05/21/L1ghblwzz8iEcp_NAAAAAFsE0Os7009936");
	strcpy(hot_data[0].addr_info[3].url_list, "group1/M00/05/6A/eEwpPVwzumCERyifAAAAAIRLMeY3349620");
	strcpy(hot_data[0].addr_info[4].url_list, "group1/M00/05/21/L1ghblwzuNKEUnQVAAAAAKXp7KA8581631");
	strcpy(hot_data[0].addr_info[5].url_list, "group1/M00/05/6A/eEwpPVwzuDeETziHAAAAAO985iw7101382");
	strcpy(hot_data[0].addr_info[6].url_list, "group1/M00/05/6A/eEwpPVwzto-EBTS7AAAAAOPxAjc8318166");
	strcpy(hot_data[0].addr_info[7].url_list, "group1/M00/05/21/L1ghblwztIyEViqSAAAAACexkUg9469078");
	strcpy(hot_data[0].addr_info[8].url_list, "group1/M00/05/68/eEwpPVwzL9KEBfrcAAAAAPHxpME9477537");
	strcpy(hot_data[0].addr_info[9].url_list, "group1/M00/05/1F/L1ghblwzLyqEFTduAAAAAF0uBy83660415");
	strcpy(hot_data[0].addr_info[10].url_list, "group1/M00/05/68/eEwpPVwzLvKES7iCAAAAAIIML5Q6854582");
	strcpy(hot_data[0].addr_info[11].url_list, "group1/M00/05/1F/L1ghblwzLjiELFtVAAAAAIHQ67g7746178");
	strcpy(hot_data[0].addr_info[12].url_list, "group1/M00/05/1F/L1ghblwzLF6EdMQmAAAAAPK8Sok9450375");
	strcpy(hot_data[0].addr_info[13].url_list, "group1/M00/05/1F/L1ghblwzKuuEGMV6AAAAAF71U_03598919");
	strcpy(hot_data[0].addr_info[14].url_list, "group1/M00/05/1F/L1ghblwzKkCEVq4ZAAAAAGB88V83385140");

	hot_data[1].user_id = 30003;
	hot_data[1].version = 1022;
	hot_data[1].all_num = 15;
	hot_data[1].addr_info = divoom_system_malloc(sizeof(divoom_light_hot_addr)*15);
	strcpy(hot_data[1].addr_info[0].url_list, "group1/M00/05/6A/eEwpPVwzuDeETziHAAAAAO985iw7101382");
	strcpy(hot_data[1].addr_info[1].url_list, "group1/M00/05/6A/eEwpPVwzto-EBTS7AAAAAOPxAjc8318166");
	strcpy(hot_data[1].addr_info[2].url_list, "group1/M00/05/21/L1ghblwztIyEViqSAAAAACexkUg9469078");
	strcpy(hot_data[1].addr_info[3].url_list, "group1/M00/05/68/eEwpPVwzL9KEBfrcAAAAAPHxpME9477537");
	strcpy(hot_data[1].addr_info[4].url_list, "group1/M00/05/1F/L1ghblwzLyqEFTduAAAAAF0uBy83660415");
	strcpy(hot_data[1].addr_info[5].url_list, "group1/M00/05/68/eEwpPVwzLvKES7iCAAAAAIIML5Q6854582");
	strcpy(hot_data[1].addr_info[6].url_list, "group1/M00/05/1F/L1ghblwzLjiELFtVAAAAAIHQ67g7746178");
	strcpy(hot_data[1].addr_info[7].url_list, "group1/M00/05/6B/eEwpPVw0QBCENHuKAAAAADN7MsM2620395");
	strcpy(hot_data[1].addr_info[8].url_list, "group1/M00/05/6A/eEwpPVwz7GaEdOcxAAAAACIKu7k8984123");
	strcpy(hot_data[1].addr_info[9].url_list, "group1/M00/05/21/L1ghblwzz8iEcp_NAAAAAFsE0Os7009936");
	strcpy(hot_data[1].addr_info[10].url_list, "group1/M00/05/6A/eEwpPVwzumCERyifAAAAAIRLMeY3349620");
	strcpy(hot_data[1].addr_info[11].url_list, "group1/M00/05/21/L1ghblwzuNKEUnQVAAAAAKXp7KA8581631");
	strcpy(hot_data[1].addr_info[12].url_list, "group1/M00/05/1F/L1ghblwzLF6EdMQmAAAAAPK8Sok9450375");
	strcpy(hot_data[1].addr_info[13].url_list, "group1/M00/05/1F/L1ghblwzKuuEGMV6AAAAAF71U_03598919");
	strcpy(hot_data[1].addr_info[14].url_list, "group1/M00/05/1F/L1ghblwzKkCEVq4ZAAAAAGB88V83385140");

    model_msg.src_id = DIVOOM_MODEL_MAIN;
    model_msg.dest_id = DIVOOM_MODEL_LIGHT_CONTROL;
    model_msg.attr = DIVOOM_LIGHT_APP_CTRL_NORMAL;
	model_msg.msg_id = DIVOOM_LIGHT_CONTROL_SET_ALL_HOT_INFO;
	model_msg.data = (uint8 *)&hot_info;

	divoom_sys_model_crl_msg_push(&model_msg);
	divoom_free(hot_data[0].addr_info);
	divoom_free(hot_data[1].addr_info);
	divoom_free(hot_data);
}

void divoom_shaco_test_one_eq_file(void)
{
	divoom_sys_msg_info model_msg = {0};
	divoom_light_eq_ctrl info;
	divoom_light_eq_info eq_info[] = {
		{13, "group1/M00/05/6A/eEwpPVwz7GaEdOcxAAAAACIKu7k8984123", "aaaaaaa"},
	};
	info.all_eq_num = 1;
	info.eq_command = DIVOOM_LIGHT_CONTROL_ADD_ONE;
	info.eq_info = eq_info;
	info.Is_set_info = 1;
	info.Is_set_mode = 0;

    model_msg.src_id = DIVOOM_MODEL_MAIN;
    model_msg.dest_id = DIVOOM_MODEL_LIGHT_CONTROL;
    model_msg.attr = DIVOOM_LIGHT_APP_CTRL_NORMAL;
	model_msg.msg_id = DIVOOM_LIGHT_CONTROL_SET_EQ_INFO;
	model_msg.data = (uint8 *)&info;

	divoom_sys_model_crl_msg_push(&model_msg);
}

void divoom_shaco_test_eq_file(void)
{
	divoom_sys_msg_info model_msg = {0};
	divoom_light_eq_ctrl info;
	divoom_light_eq_info eq_info[] = {
		{12, "group1/M00/05/6B/eEwpPVw0QBCENHuKAAAAADN7MsM2620395", "aaaaaaa"},
		{14, "group1/M00/05/21/L1ghblwztIyEViqSAAAAACexkUg9469078", "aaaaaaa"},
		{15, "group1/M00/05/6A/eEwpPVwzumCERyifAAAAAIRLMeY3349620", "aaaaaaa"},
		{16, "group1/M00/05/21/L1ghblwzuNKEUnQVAAAAAKXp7KA8581631", "aaaaaaa"},
		{17, "group1/M00/05/6A/eEwpPVwzuDeETziHAAAAAO985iw7101382", "aaaaaaa"},
		{18, "group1/M00/05/6A/eEwpPVwzto-EBTS7AAAAAOPxAjc8318166", "aaaaaaa"},
		//{19, "group1/M00/05/21/L1ghblwztIyEViqSAAAAACexkUg9469078", "aaaaaaa"},
		//{20, "group1/M00/05/68/eEwpPVwzL9KEBfrcAAAAAPHxpME9477537", "aaaaaaa"},
		//{21, "group1/M00/05/1F/L1ghblwzLyqEFTduAAAAAF0uBy83660415", "aaaaaaa"},
		//{22, "group1/M00/05/68/eEwpPVwzLvKES7iCAAAAAIIML5Q6854582", "aaaaaaa"},
		//{23, "group1/M00/05/1F/L1ghblwzLjiELFtVAAAAAIHQ67g7746178", "aaaaaaa"},
		//{24, "group1/M00/05/1F/L1ghblwzLF6EdMQmAAAAAPK8Sok9450375", "aaaaaaa"},
		//{25, "group1/M00/05/1F/L1ghblwzKuuEGMV6AAAAAF71U_03598919", "aaaaaaa"},
		//{26, "group1/M00/05/1F/L1ghblwzKkCEVq4ZAAAAAGB88V83385140", "aaaaaaa"},
	};
	info.all_eq_num = 7;
	info.eq_command = DIVOOM_LIGHT_CONTROL_ADD_ALL;
	info.eq_info = eq_info;
	info.Is_set_info = 1;
	info.Is_set_mode = 0;

    model_msg.src_id = DIVOOM_MODEL_MAIN;
    model_msg.dest_id = DIVOOM_MODEL_LIGHT_CONTROL;
    model_msg.attr = DIVOOM_LIGHT_APP_CTRL_NORMAL;
	model_msg.msg_id = DIVOOM_LIGHT_CONTROL_SET_EQ_INFO;
	model_msg.data = (uint8 *)&info;

	divoom_sys_model_crl_msg_push(&model_msg);
}

void divoom_shaco_test_del_pic(void)
{
	divoom_sys_msg_info model_msg = {0};
	divoom_light_expect_info user_info = {0};
	divoom_light_expect_ctrl user_list[] = {
										{DIVOOM_DISP_APP_SET_LIST_OTHER, 400000006}, 
										{DIVOOM_DISP_APP_SET_LIST_OTHER, 400000012}, 
										{DIVOOM_DISP_APP_SET_LIST_CATEGORY_LIST, 3}, 
										{DIVOOM_DISP_APP_SET_LIST_CATEGORY_LIST, 7},};

    model_msg.src_id = DIVOOM_MODEL_MAIN;
    model_msg.dest_id = DIVOOM_MODEL_LIGHT_CONTROL;
    model_msg.attr = DIVOOM_LIGHT_APP_CTRL_NORMAL;
	model_msg.data = (uint8 *)&user_info;
	model_msg.msg_id = DIVOOM_LIGHT_CONTROL_SET_EXPECT_INFO;


	user_info.all_num = 4;
	user_info.Is_rand_disp = 1;
	user_info.Is_set_info = 1;
	user_info.Is_set_mode = 0;
	user_info.command = DIVOOM_LIGHT_CONTROL_EXPECT_CANCEL_SUB;
	user_info.user_list = user_list;

	divoom_sys_model_crl_msg_push(&model_msg);
}

void divoom_shaco_test_load_pic(void)
{
	divoom_sys_msg_info model_msg = {0};
	divoom_light_expect_info user_info = {0};
	divoom_light_expect_ctrl user_list[] = {
										{DIVOOM_DISP_APP_SET_LIST_OTHER, 400000006}, 
										{DIVOOM_DISP_APP_SET_LIST_OTHER, 400000007}, 
										{DIVOOM_DISP_APP_SET_LIST_OTHER, 400000012}, 
										{DIVOOM_DISP_APP_SET_LIST_OTHER, 400000015}, 
										{DIVOOM_DISP_APP_SET_LIST_OTHER, 400006376}, 
										{DIVOOM_DISP_APP_SET_LIST_OTHER, 400001941}, 
										{DIVOOM_DISP_APP_SET_LIST_CATEGORY_LIST, 3}, 
										{DIVOOM_DISP_APP_SET_LIST_CATEGORY_LIST, 5}, 
										{DIVOOM_DISP_APP_SET_LIST_CATEGORY_LIST, 7},};

    model_msg.src_id = DIVOOM_MODEL_MAIN;
    model_msg.dest_id = DIVOOM_MODEL_LIGHT_CONTROL;
    model_msg.attr = DIVOOM_LIGHT_APP_CTRL_NORMAL;
	model_msg.data = (uint8 *)&user_info;
	model_msg.msg_id = DIVOOM_LIGHT_CONTROL_SET_EXPECT_INFO;


	user_info.all_num = 9;
	user_info.Is_rand_disp = 1;
	user_info.Is_set_info = 1;
	user_info.Is_set_mode = 0;
	user_info.command = DIVOOM_LIGHT_CONTROL_EXPECT_SUB;
	user_info.user_list = user_list;

	divoom_sys_model_crl_msg_push(&model_msg);
}*/

int divoom_main_get_wifi_con_status(void)
{
#ifdef DIVOOM_SIMULATOR
    return 1;
#else
	if(pDivoomHandle)
	{
		return pDivoomHandle->device_wifi_status;
	}
	return 0;
    #endif
}

int divoom_main_get_server_con_status(void)
{
#ifdef DIVOOM_SIMULATOR
    return 1;
#else
	if(pDivoomHandle)
	{
		return pDivoomHandle->divoom_connect_srv;
	}
	return 0;
    #endif
}

//通过url获取ip地址,可以减少每次使用url造成的内存泄漏
//>0 success,<=0 fail
//ip_host_len要大于等于24,加上http://
int divoom_main_get_ip_from_url(char *url, char *ip_host, uint8 ip_host_len, DIVOOM_PROTOCOL protocol)
{
	(void)url;
	(void)ip_host;
	(void)ip_host_len;
	(void)protocol;
	return -1;
}

T_VOID divoom_main_disp_sd_out(T_VOID)
{
	return;
	divoom_app_schedule_item_info push_info = {0};
    divoom_disp_Play_net_image_image_info notice_info = {0};

	push_info.event_id = DIVOOM_MAIN_PLAY_SD_OUT_EVENT;
	push_info.model_id = DIVOOM_MODEL_MAIN;
	push_info.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
	push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;	
	if(divoom_app_schedule_push(&push_info))
	{
        notice_info.model_id = DIVOOM_MANAGE_IMAGE_NOTICE_NET_GIF;
        notice_info.image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + DIVOOM_DISP_DIVOOM_SD_OUT;
        divoom_disp_ctrl_set_mode(DIVOOM_DISP_PLAY_NET_IMAGE_CTRL, (void *)&notice_info);
	}
	else
	{
		divoom_print("Here Can't get Scheduel Priority!");
	}
	push_info.event_id = DIVOOM_MAIN_PLAY_SD_FREE_WARNING;
    divoom_app_schedule_cancel(&push_info);
}
T_VOID divoom_main_start_format_sd(T_VOID)
{
//    uint8 buzzer_music = DIVOOM_BUZZER_ONLY_DI;
    divoom_disp_Play_net_image_image_info notice_info = {0};
    
    notice_info.model_id = DIVOOM_MANAGE_IMAGE_NOTICE_NET_GIF;
    notice_info.image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_NOTICE_NET_PIC + DIVOOM_lOCAL_USE_PIC_NOTICE_SD_FORMAT_MSG;
    pDivoomHandle->device_wifi_status = 0;
    pDivoomHandle->divoom_connect_srv = 0;
    divoom_disp_ctrl_set_mode(DIVOOM_DISP_PLAY_NET_IMAGE_CTRL, (void *)&notice_info);       

    divoom_manage_image_download_exit();
    
}
T_VOID divoom_main_power_on_err_sd(T_VOID)
{
    pDivoomHandle->device_wifi_status = 0;
    pDivoomHandle->divoom_connect_srv = 0;

    divoom_manage_image_download_exit();
    divoom_disp_event_disp_info notice_info = {0};
	divoom_app_schedule_item_info push_info = {0};

	push_info.event_id = DIVOOM_MAIN_PLAY_POWER_ON_EVENT;
	push_info.model_id = DIVOOM_MODEL_MAIN;
    push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
	divoom_app_schedule_push(&push_info);

    notice_info.disp_id[0] = DIVOOM_DISP_DIVOOM_SD_ERR;
	notice_info.on_time = 1;
	notice_info.period_time = 2;
	notice_info.tot_pic = 1;
	notice_info.tot_time = 60000;
    notice_info.noice_id = DIVOOM_MAIN_PLAY_WIFI_NOTICE_EVENT;
    notice_info.pover_callback = NULL;
    
    divoom_disp_ctrl_set_mode(DIVOOM_DISP_EVENT_DIS_CTRL, (void *)&notice_info);
    divoom_system_delay_ms(5000);

}
T_VOID divoom_main_disp_sd_err_type(T_U8 flag)
{
	divoom_app_schedule_item_info push_info = {0};
    divoom_disp_Play_net_image_image_info notice_info = {0};
    

	push_info.event_id = DIVOOM_MAIN_PLAY_SD_ERR_EVENT;
	push_info.model_id = DIVOOM_MODEL_MAIN;
	push_info.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
	push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;	
	if(divoom_app_schedule_push(&push_info))
	{
		uint8 buzzer_music = DIVOOM_BUZZER_ONLY_DI;
        
        
        if(0) //(flag == 0)
        {
            notice_info.model_id = DIVOOM_MANAGE_IMAGE_NOTICE_NET_GIF;
            notice_info.image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + DIVOOM_DISP_DIVOOM_SD_ERR;
        }
        else
        {
            notice_info.model_id = DIVOOM_MANAGE_IMAGE_NOTICE_NET_GIF;
            notice_info.image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_NOTICE_NET_PIC + DIVOOM_lOCAL_USE_PIC_CONFIRM_SD_FORMAT_MSG;
        }
        divoom_disp_ctrl_set_mode(DIVOOM_DISP_PLAY_NET_IMAGE_CTRL, (void *)&notice_info);
        
        
    	push_info.model_id = DIVOOM_MODEL_MAIN;
    	push_info.event_id = DIVOOM_MAIN_PLAY_SD_OUT_EVENT;
    	push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
    	divoom_app_schedule_cancel(&push_info);
	}
	else
	{
		divoom_print("Here Can't get Scheduel Priority!");
	}
}

T_VOID divoom_main_disp_sd_in(T_VOID)
{
	//先取消sd卡拔出的提示
	if(pDivoomHandle->new_sd_flag)
	{
        divoom_main_give_wait_download_msg();
	}
	divoom_app_schedule_item_info event_info = {0};
	event_info.model_id = DIVOOM_MODEL_MAIN;
	event_info.event_id = DIVOOM_MAIN_PLAY_SD_OUT_EVENT;
	event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
	divoom_app_schedule_cancel(&event_info);
    if(pDivoomHandle->new_sd_flag)
    {
        divoom_disp_ctrl_set_mode(DIVOOM_DISP_DOWNLOAD_FILE_MODE, NULL);
    }
}

enum{
    DIVOOM_MAIN_KEY_OVER_MODE,
    DIVOOM_MAIN_KEY_MOD_VOLUMN_MODE,
    DIVOOM_MAIN_KEY_MOD_FRONT_LIGHT_MODE,
    DIVOOM_MAIN_KEY_MOD_BACK_LIGHT_MODE,
    DIVOOM_MAIN_KEY_MOD_COLOR_EFFECT_MODE,
};
    
/**
 * NAME         divoom_main_disp_key_change_status_over_callback
 * @BRIEF	    按键功能显示播放完成事件
 * @PARAM	
                    
 * @RETURN	none
 * @RETVAL	
 */
void divoom_main_disp_key_change_status_over_callback(unsigned char mode)
{
    divoom_sys_msg_info model_msg = {0};
    
    divoom_print("%s:  key func play is over!\n",__func__);
    model_msg.msg_type = 1;
    model_msg.src_id = DIVOOM_MODEL_MAIN;
    model_msg.dest_id = DIVOOM_MODEL_MAIN;
    model_msg.msg_id = DIVOOM_MAIN_KEY_FUNC_PLAY_OVER;  
    divoom_sys_model_crl_msg_push(&model_msg);
}

/**
 * NAME         divoom_main_disp_key_change_status
 * @BRIEF	    按键功能显示更新
 * @PARAM	
                    
 * @RETURN	none
 * @RETVAL	
 */

void divoom_main_disp_key_change_status(uint8 mode, uint8 value, uint8 max_value, uint8 add_flag)
{
#if 0
    //DIVOOM_DISP_VOLUME_MUTE_PIC
    
	divoom_app_schedule_item_info push_info = {0};
    divoom_disp_ctrl_speed_bar_info disp_info;

    
    disp_info.value = value;
    disp_info.max_value = max_value;
    divoom_print("%s: mode: %d, value: %d, max: %d, flag: %d\n",__func__, mode, value, max_value, add_flag);
    if(mode == DIVOOM_MAIN_KEY_MOD_VOLUMN_MODE)
    {
        if(value == 0)
        {
            disp_info.pic_id = DIVOOM_DISP_VOLUME_MUTE_PIC;
        }
        else if(value == max_value)
        {
            disp_info.pic_id = DIVOOM_DISP_VOLUME_MAX_PIC;
        }
        else if(add_flag)
        {
            disp_info.pic_id = DIVOOM_DISP_VOLUME_ADD_PIC;
        }
        else
        {
            disp_info.pic_id = DIVOOM_DISP_VOLUME_SUB_PIC;
        }
    }
    else if(mode == DIVOOM_MAIN_KEY_MOD_FRONT_LIGHT_MODE)
    {
        if(add_flag)
        {
            disp_info.pic_id = DIVOOM_DISP_SELFFRONTLIGHTADD;
        }
        else
        {
            disp_info.pic_id = DIVOOM_DISP_SELFFRONTLIGHTSUB;
        }
    }
    else if(mode == DIVOOM_MAIN_KEY_MOD_BACK_LIGHT_MODE)
    {
        if(add_flag)
        {
            disp_info.pic_id = DIVOOM_DISP_SELFLIGHTADD;
        }
        else
        {
            disp_info.pic_id = DIVOOM_DISP_SELFLIGHTSUB;
        }
    }
    else
    {
        if(pDivoomHandle->back_light_off == 0)
        {
            disp_info.pic_id = DIVOOM_DISP_BT_DISCONNECTED_PIC;
        }
        else
        {
            disp_info.pic_id = DIVOOM_DISP_SPECIAL_LIGHT_MODE_01_PIC;
        }
    }
	push_info.event_id = DIVOOM_MAIN_PLAY_KEY_FUNC_EVENT;
	push_info.model_id = DIVOOM_MODEL_MAIN;
    push_info.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
    push_info.resource_type = DIVOOM_APP_SCHEDULE_RESOURCCE_RGB;
	if(divoom_app_schedule_push(&push_info))
	{
        disp_info.pover_callback = divoom_main_disp_key_change_status_over_callback;
        divoom_disp_ctrl_set_mode(DIVOOM_DISP_SPEED_BAR_MODE, (void *)&disp_info);
	}
	else
	{
        pDivoomHandle->key_func = DIVOOM_MAIN_KEY_OVER_MODE;
        pDivoomHandle->key_op_time = 0;
		divoom_print("Here Can't get Scheduel Priority!");
	}
#endif
}

/**
 * NAME         divoom_main_disp_key_change_over
 * @BRIEF	    取消按键功能显示
 * @PARAM	
                    
 * @RETURN	none
 * @RETVAL	
 */

void divoom_main_disp_key_change_over(void)
{
	divoom_app_schedule_item_info push_info = {0};

    divoom_print("%s: we will cancel key func!\n",__func__);
	push_info.event_id = DIVOOM_MAIN_PLAY_KEY_FUNC_EVENT;
	push_info.model_id = DIVOOM_MODEL_MAIN;
    push_info.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
    push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
	divoom_app_schedule_cancel(&push_info);
    pDivoomHandle->key_func = DIVOOM_MAIN_KEY_OVER_MODE;
    pDivoomHandle->key_op_time = 0;
}

/**
 * NAME         divoom_main_do_with_other_key_pre
 * @BRIEF	    检查是否是在复合按键处理逻辑,并处理相关逻辑
 * @PARAM	
                    
 * @RETURN	none
 * @RETVAL	
 */

uint8 divoom_main_do_with_other_key_pre(uint8 key_value, uint8 key_type)
{
    uint8 add_flag = 0, max_value = 0, value = 0, flag = 0;
    
    if((pDivoomHandle->key_func != DIVOOM_MAIN_KEY_OVER_MODE))
    {
        if(pDivoomHandle->key_op_time && (pDivoomHandle->key_op_time + DIVOOM_KEY_COMBINE_TIME_OUT > divoom_system_get_tickms()))
        {
            if((key_value == kbPRE || key_value == kbNEXT) && pDivoomHandle->key_func != DIVOOM_MAIN_KEY_OVER_MODE)
            {
                divoom_print("%s: value: %d, type: %d, mdoe: %d\n",__func__, key_value, key_type, pDivoomHandle->key_func);
                flag = 1;
                switch(pDivoomHandle->key_func)
                {
                    case DIVOOM_MAIN_KEY_MOD_VOLUMN_MODE:
                        break;
                        
                    case DIVOOM_MAIN_KEY_MOD_FRONT_LIGHT_MODE:
                        max_value = 100;
                        if(key_value == kbPRE)
                        {
#ifdef DIVOOM_TIME_FRAME_SUPPORT_LOW_LIGHT
                            if(pDivoomHandle->sys_setting.brightness > DIVOOM_SYS_BRIGHT_ON_FLOOR + 5)
                            {
                                pDivoomHandle->sys_setting.brightness -= 5;
                            }
                            else if(pDivoomHandle->sys_setting.brightness > DIVOOM_SYS_BRIGHT_ON_FLOOR)
                            {
                                pDivoomHandle->sys_setting.brightness = DIVOOM_SYS_BRIGHT_ON_FLOOR;
                            }
                            else
                            {
                                pDivoomHandle->sys_setting.brightness = DIVOOM_SYS_BRIGHT_ON_FLOOR;
                            }
#else
                            if(pDivoomHandle->sys_setting.brightness > 5)
                            {
                                pDivoomHandle->sys_setting.brightness -= 5;
                            }
                            else
                            {
                                pDivoomHandle->sys_setting.brightness = 0;
                            }
#endif
                        }
                        else
                        {
#ifdef DIVOOM_TIME_FRAME_SUPPORT_LOW_LIGHT
                            if(pDivoomHandle->sys_setting.brightness == 0)
                            {
                                pDivoomHandle->sys_setting.brightness = DIVOOM_SYS_BRIGHT_ON_FLOOR;
                            }
                            else if(pDivoomHandle->sys_setting.brightness == DIVOOM_SYS_BRIGHT_ON_FLOOR)
                            {
                                pDivoomHandle->sys_setting.brightness = DIVOOM_SYS_BRIGHT_ON_FLOOR + 5;
                            }
                            else if(pDivoomHandle->sys_setting.brightness > 95)
                            {
                                pDivoomHandle->sys_setting.brightness = 100;
                            }
                            else
                            {
                                pDivoomHandle->sys_setting.brightness += 5;
                            }
#else
                            if(pDivoomHandle->sys_setting.brightness > 95)
                            {
                                pDivoomHandle->sys_setting.brightness = 100;
                            }
                            else
                            {
                                pDivoomHandle->sys_setting.brightness += 5;
                            }
#endif
                            add_flag = 1;
                        }
                        value = pDivoomHandle->sys_setting.brightness;
                        pDivoomHandle->sleep_bright_override = 0;
                        divoom_main_apply_sys_light();
                        break;
                        
                    case DIVOOM_MAIN_KEY_MOD_BACK_LIGHT_MODE:
                        max_value = 100;
                        break;
                        
                    case DIVOOM_MAIN_KEY_MOD_COLOR_EFFECT_MODE:
                        add_flag = 1;
                        if(key_type == PRESS_SHORT)
                        {
//                            divoom_mix_screen_oneline_led_switch(1);
                            if(key_value == kbPRE)
                            {
//                                divoom_mix_screen_oneline_led_set_effect_dir(0);
                            }
                            else
                            {
//                                divoom_mix_screen_oneline_led_set_effect_dir(1);
                            }
                            pDivoomHandle->back_light_off = 1;
                        }
                        else if(key_type == PRESS_LONG)
                        {
                            if(key_value == kbPRE)
                            {
//                                divoom_mix_screen_oneline_led_switch(0);
                                pDivoomHandle->back_light_off = 0;
                                add_flag = 0;
                            }
                            else
                            {
//                                divoom_mix_screen_oneline_led_switch(1);
                                pDivoomHandle->back_light_off = 1;
                            }
                        }
//                        value = divoom_mix_screen_oneline_get_mode(&max_value);
                        flag = 1;
                        break;
                        
                }
                pDivoomHandle->key_op_time = divoom_system_get_tickms();
                if(flag)
                {
                    divoom_main_disp_key_change_status(pDivoomHandle->key_func, value, max_value, add_flag);
                }
                return  1;
            }
            return  0;
        }
        else
        {
            //超时，将退出
            
            divoom_print("%s:  key func time out!%d,%d, %d\n",__func__,pDivoomHandle->key_op_time,pDivoomHandle->key_op_time + DIVOOM_KEY_COMBINE_TIME_OUT, divoom_system_get_tickms());
            divoom_main_disp_key_change_over();
        }
    }
	else 
	{
//		if (key_value == DIVOOM_KEY_LIGHT_RIGHT || key_value == DIVOOM_KEY_LIGHT_LEFT)
//		{
//			if(divoom_main_get_rgb_light() == 0)
//			{
//				divoom_main_set_rgb_light(100);
//			}
//			if (key_value == DIVOOM_KEY_LIGHT_RIGHT)
//				divoom_disp_ambient_ctrl_set_disp_mode(1);
//			else
//				divoom_disp_ambient_ctrl_set_disp_mode(0);
//			Pdivoom_channel_set_rgb_info rgb_info = divoom_disp_ambient_ctrl_get_disp_info();
//			divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_GET_RGB_INFO, DIVOOM_APP_TOPIC, (void *)rgb_info);
//			divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_SET_RGB_INFO, DIVOOM_SERVER_TOPIC, (void *)rgb_info);
//		}
#if 0
		if (key_value == DIVOOM_KEY_LIGHT_RIGHT || key_value == DIVOOM_KEY_LIGHT_LEFT)
		{
			/* 音乐播放中优先调音乐音量；仅相册幻灯且无音乐时调相册视频音量 */
			if (divoom_music_ctrl_is_playing())
			{
				divoom_lvgl_top_bright_volume_menu_init(1);
			}
			else if (divoom_local_movie_play_is_album_slideshow())
			{
				divoom_lvgl_top_bright_volume_menu_init(2);
			}
			else if (divoom_local_movie_play_is_playing())
			{
				divoom_lvgl_top_bright_volume_menu_init(1);
			}
			else if (divoom_local_music_ctrl_get_tone_status())
			{
				divoom_lvgl_top_bright_volume_menu_init(3);
			}
			else
			{
				divoom_lvgl_top_bright_volume_menu_init(1);
			}
			return 1;
		}
#endif
	}
    return 0; 
}

/**
 * NAME         divoom_main_do_with_other_key
 * @BRIEF	    启动复合按键相关功能
 * @PARAM	
                    
 * @RETURN	none
 * @RETVAL	
 */

void divoom_main_do_with_other_key(uint8 key_value, uint8 key_type)
{
}

/**
 * NAME         divoom_main_do_with_other_key
 * @BRIEF	    启动复合按键相关功能
 * @PARAM	
                    
 * @RETURN	none
 * @RETVAL	
 */

void divoom_main_enter_stanby_mode(void)
{
    int ret;
    divoom_screen_bright(0);
	divoom_disp_ambient_ctrl_set_rgb_on_off(0);
    
    ret = system("pgrep -f run_check.sh | xargs kill");
    ret = system("killall divoom_bt_manage");
    ret = system("killall wifi_daemon");
    ret = system("killall wpa_supplicant");
    ret = system("rmmod ATBM6x6x_wifi_sdio");
    ret = system("echo disp0 > /sys/kernel/debug/dispdbg/name");
    ret = system("echo blank > /sys/kernel/debug/dispdbg/command");
    ret = system("echo 1 > /sys/kernel/debug/dispdbg/param");
    ret = system("echo 1 > /sys/kernel/debug/dispdbg/start");
}



void divoom_main_set_timezone(Pdivoom_wifi_get_tz ptz)
{
	if(NULL == ptz)
		return;
	if(strcmp(pDivoomHandle->sys_setting.tzcode,ptz->tzcode))
	{
        pDivoomHandle->update_flag = 1;
        strcpy(pDivoomHandle->sys_setting.tzcode, ptz->tzcode);
		//divoom_time_set_tz(ptz);
	}
}

void divoom_main_load_timezone_config(void)
{
    if(strlen(pDivoomHandle->sys_setting.tzcode))
    {
        if (divoom_main_apply_posix_timezone(pDivoomHandle->sys_setting.tzcode) == 0)
            divoom_print("time zone: %s\n",pDivoomHandle->sys_setting.tzcode);
        else
            divoom_print("time zone err: %s\n",pDivoomHandle->sys_setting.tzcode);
    }
    else
    {
        divoom_print("time zone pass empty!\n");
    }
	//divoom_time_set_tz(&tz);
	divoom_onoff_on_timezone_ready();
}

void divoom_main_ledseg_update_time(void)
{
	time_t cur_time;
	struct tm cur_tm;
//获取时间	
	cur_time = time(0);
	localtime_r((time_t *)&cur_time, &cur_tm);
//	divoom_mix_screen_led_seg_set_time(cur_tm.tm_hour, cur_tm.tm_min);
}

uint8 divoom_main_get_mic_switch(void)
{
	if(divoom_product_test_is_active())
	{
		return 0;
	}
	
    return pDivoomHandle->sys_setting.mic_swtich;
}

void divoom_main_ledseg_time_init(void)
{
	divoom_main_load_timezone_config();
//	divoom_mix_screen_ledseg_set_brightness(pDivoomHandle->sys_setting.brightness);
	pDivoomHandle->ledseg_update_flag = 1;
}

void divoom_main_send_lwt_msg(int flag)
{
	uint8 mac[6];
	divoom_device_public_ser_mqtt_lwt_request_info device_mac;
    
    divoom_main_get_device_mac_addr(mac);
	sprintf(device_mac.mac_addr, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    device_mac.flag = flag;
	divoom_net_app_send_packet_lwt(DIVOOM_NET_COMM_PUBLIC_SER_MQTT_LWT, DIVOOM_SERVER_TOPIC, (void *)&device_mac);
}

static void divoom_main_ensure_system_clock_dirs(void)
{
	divoom_app_com_mk_dir(DIVOOM_SD_SYSTEM_DIR);
	if (!divoom_port_file_exists(DIVOOM_SD_SYSTEM_APP_CLOCKS_DIR))
		divoom_port_copy_tree("/usr/share/divoom_app/clockapp", DIVOOM_SD_SYSTEM_APP_CLOCKS_DIR);
	if (!divoom_port_file_exists(DIVOOM_SD_SYSTEM_SYS_CLOCKS_DIR))
		divoom_port_copy_tree("/usr/share/divoom_app/clocksys", DIVOOM_SD_SYSTEM_SYS_CLOCKS_DIR);
}

static int divoom_main_ensure_userdata_seed(void)
{
	const char *seed_root = "/usr/userdata_seed";

	if (divoom_port_file_exists(DIVOOM_THUMB_DATA_BASE_FILE_NAME))
		return 0;
	divoom_print("userdata: seed from %s\n", seed_root);
	divoom_port_copy_tree(seed_root, DIVOOM_USER_ROOT);
    return 1;
}

/* OTA 暂存目录：开机确保存在，并清掉上次未完成/已刷写后的残留包 */
static void divoom_main_ensure_ota_staging_dir(void)
{
	void *dir;
	const divoom_port_dirent_t *ent;
	char path[280];

	divoom_port_mkdir_p(DIVOOM_OTA_RTOS_STAGING_DIR);
	dir = divoom_port_dir_open(DIVOOM_OTA_RTOS_STAGING_DIR);
	if (!dir)
		return;
	while ((ent = divoom_port_dir_read(dir)) != NULL) {
		if (ent->name[0] == '.')
			continue;
		snprintf(path, sizeof(path), "%s/%s", DIVOOM_OTA_RTOS_STAGING_DIR, ent->name);
		if (ent->is_dir)
			divoom_port_remove_tree(path);
		else
			divoom_port_remove_file(path);
	}
	divoom_port_dir_close(dir);
}

/* Boot-time /userdata directory tree; call before any file under /userdata is opened. */
static void divoom_main_ensure_userdata_dirs(void)
{
    char path_name[128];
	divoom_app_com_mk_dir(DIVOOM_TMP_DIR);
	divoom_app_com_mk_dir(DIVOOM_USER_WIFI_DIR);
	divoom_app_com_mk_dir(DIVOOM_USER_BT_DIR);
	divoom_app_com_mk_dir(DIVOOM_USER_SSL_CERTS_DIR);
	divoom_app_com_mk_dir(DIVOOM_USER_SSL_PRIVATE_DIR);
	divoom_app_com_mk_dir(DIVOOM_SD_SYSTEM_DIR);
	divoom_main_ensure_system_clock_dirs();
	divoom_app_com_mk_dir(DIVOOM_SD_APP_PIC_DIR);
	divoom_app_com_mk_dir(DIVOOM_SD_APP_PARA_DIR);
	divoom_app_com_mk_dir(DIVOOM_SD_APP_API_DIR);
	divoom_app_com_mk_dir(DIVOOM_SD_SYSTEM_SYS_CLOCKS_DIR);
	divoom_app_com_mk_dir(DIVOOM_SD_SYSTEM_APP_CLOCKS_DIR);
	divoom_app_com_mk_dir(DIVOOM_SD_SYSTEM_COMPONENT_DIR);
	divoom_app_com_mk_dir(DIVOOM_THUMB_INFO_DIR);
	divoom_app_com_mk_dir(DIVOOM_ALBUM_COVER_INFO_DIR);
	divoom_app_com_mk_dir(DIVOOM_SD_UPDATE_SYS_FILE_DIR);
	divoom_app_com_mk_dir(DIVOOM_VISION_INFO_DIR);
	divoom_app_com_mk_dir(DIVOOM_CLOUD_PHOTO_INFO_DIR);
	divoom_app_com_mk_dir(DIVOOM_USER_RECOMMEND_DIR);
	divoom_app_com_mk_dir(DIVOOM_AI_TALK_CACHE_DIR);
#if DIVOOM_SHOW_TF_NAND
	divoom_app_com_mk_dir(DIVOOM_TF_MUSIC_NAND_DIR);
#endif

    for(int i = 1; i < DIVOOM_MANAGE_IMAGE_END_ID; i++)
    {
        sprintf(path_name, "%s/%d", DIVOOM_USER_RECOMMEND_DIR, i);
	    divoom_app_com_mk_dir(path_name);
    }
	divoom_main_ensure_ota_staging_dir();
}

static void divoom_main_clear_userdata_except_system(void)
{
	char cmd[768];

	sprintf(cmd,
		"rm -rf %s %s %s %s %s %s %s %s %s %s %s",
		DIVOOM_SD_APP_PIC_DIR,
		DIVOOM_SD_APP_PARA_DIR,
		DIVOOM_SD_APP_API_DIR,
		DIVOOM_SD_UPDATE_SYS_FILE_DIR,
		DIVOOM_VISION_INFO_DIR,
		DIVOOM_CLOUD_PHOTO_INFO_DIR,
		DIVOOM_THUMB_INFO_DIR,
		DIVOOM_GAMES_ROOT_USER,
		DIVOOM_DEBUG_FILE_NAME,
		DIVOOM_DEBUG_BAKE_FILE_NAME,
		DIVOOM_THUMB_DATA_BASE_FILE_NAME);
	exec_command_system(cmd);
	exec_command_system("rm -f /userdata/divoom_run.log /userdata/divoom_fail.log "
			    "/userdata/tmp.tar.gz /userdata/divoom_app_new "
			    "/userdata/jl_firmware.bin " DIVOOM_OTA_JL_FW_TMP_PATH " "
			    "/userdata/http.bin /userdata/customize.tar.gz");
}

static int divoom_main_mkfs_udisk_partition_active(void)
{
	int ret;

#if DIVOOM_BUILD_RTOS_DEVICE && defined(CONFIG_DRIVERS_AIC8800D80)
	if (!divoom_recovery_integrity_protect_wifi(0)) {
		divoom_print("mkfs UDISK denied: no verified Wi-Fi firmware on /usr\n");
		return -1;
	}
#endif
	divoom_print("mkfs UDISK: unmount /userdata, f_mkfs /dev/UDISK, remount\n");
	divoom_database_manage_close(NULL);
	divoom_port_sync();
	ret = divoom_port_fs_format_userdata();
	if (ret != 0)
		divoom_print("mkfs UDISK failed (%d)\n", ret);
	return ret;
}

int divoom_main_mkfs_udisk_partition(void)
{
    int ret;

    if (!divoom_main_begin_userdata_maintenance())
        return -1;
    ret = divoom_main_mkfs_udisk_partition_active();
    divoom_main_userdata_maintenance_returned();
    return ret;
}

static void divoom_main_format_divoom_config_partition_active(void);

static void divoom_main_format_udisk_partition_active(void)
{
	int mkfs_ok;

	mkfs_ok = (divoom_main_mkfs_udisk_partition_active() == 0);
	if (mkfs_ok) {
		divoom_main_ensure_userdata_seed();
		divoom_main_ensure_userdata_dirs();
	} else {
		divoom_print("mkfs UDISK failed, fallback to rm\n");
		divoom_main_format_divoom_config_partition_active();
		divoom_main_clear_userdata_except_system();
	}
	exec_command_system("sync");
	exec_command_system("reboot");
}

void divoom_main_format_udisk_partition(void)
{
    if (!divoom_main_begin_userdata_maintenance())
        return;
    divoom_main_format_udisk_partition_active();
    divoom_main_userdata_maintenance_returned();
}

static void divoom_main_format_divoom_config_partition_active(void)
{
	char cmd[256];

	sprintf(cmd, "rm -rf %s", DIVOOM_SD_SYSTEM_DIR);
	exec_command_system(cmd);
    divoom_main_ensure_userdata_seed();
	divoom_main_ensure_system_clock_dirs();
}

void divoom_main_format_divoom_config_partition(void)
{
    if (!divoom_main_begin_userdata_maintenance())
        return;
    divoom_main_format_divoom_config_partition_active();
    divoom_main_userdata_maintenance_returned();
}

//设备切换到GIF模式
int divoom_main_change_to_gif_mode(int flag)
{
    if(!pDivoomHandle)
        return 0;
    if(flag != -1)
    {
        if(pDivoomHandle->gif_mode != flag)
        {
            pDivoomHandle->gif_mode = flag;
            divoom_main_save_config();
        }
    }
    return pDivoomHandle->gif_mode;
}

//设备设置升级标记
int divoom_main_set_update_fireware_flag(int flag)
{
    if(!pDivoomHandle)
        return 0;
    if(flag != -1)
    {
        if(pDivoomHandle->update_firmware_flag != flag)
        {
            pDivoomHandle->update_firmware_flag = flag;
            divoom_main_save_config();
        }
    }
    return pDivoomHandle->update_firmware_flag;
}

//删除配置，重新配网
void divoom_main_format_device(uint8 sd_format)
{
	(void)sd_format;

	divoom_main_format_udisk_partition();
}

//获取设备主人
uint32 divoom_main_get_host_id(void)
{
    return pDivoomHandle->host_id;
}

//设置设备主人
void divoom_main_set_host_id(int host_id)
{
    /* 首次绑定必须允许 host_id 从 0 更新为服务器下发的用户 ID。 */
    if (pDivoomHandle != NULL && pDivoomHandle->host_id == 0)
    {
        pDivoomHandle->host_id = host_id;
    }
}

void divoom_main_request_device_share_code(void)
{
    if (pDivoomHandle == NULL)
    {
        return;
    }
    divoom_print("[SHARE_CODE] request Device/GetShareCodeV2\n");
    divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_SHARE_CODE,
        DIVOOM_SERVER_TOPIC, NULL);
}

void divoom_main_update_device_share_code(uint32 share_code, uint32 expire_time)
{
    if (pDivoomHandle == NULL || share_code > 99999999u)
    {
        return;
    }
    pDivoomHandle->device_share_code = share_code;
    pDivoomHandle->device_share_code_expire_time = expire_time;
    pDivoomHandle->device_share_code_valid = 1;
    divoom_print("[SHARE_CODE] response ShareCode=%08u ExpireTime=%u\n",
        (unsigned int)share_code, (unsigned int)expire_time);
}

int divoom_main_get_device_share_code(uint32 *share_code, uint32 *expire_time)
{
    if (pDivoomHandle == NULL || pDivoomHandle->device_share_code_valid == 0)
    {
        return 0;
    }
    if(pDivoomHandle->device_share_code == 0)
    {
        Pdivoom_net_command ret_data = NULL;
	    ret_data = divoom_net_app_send_packet_http(DIVOOM_NET_COMM_DEVICE_GET_SHARE_CODE, NULL, 1);
        if(ret_data)
        {
            Pdivoom_device_share_code_info pshare_code;
            
            pshare_code = (Pdivoom_device_share_code_info)(Pdivoom_device_ser_mqtt_info_json)ret_data->cmd_data;
            if(pshare_code)
            {
                pDivoomHandle->device_share_code = pshare_code->share_code;
                pDivoomHandle->device_share_code_expire_time = pshare_code->expire_time;
                pDivoomHandle->device_share_code_valid = 1;
            }
            divoom_net_app_free_packet(ret_data);
        }
    }
    if (share_code != NULL)
    {
        *share_code = pDivoomHandle->device_share_code;
    }
    if (expire_time != NULL)
    {
        *expire_time = pDivoomHandle->device_share_code_expire_time;
    }
    return 1;
}

uint8 divoom_main_set_on_off_work_status(uint8 state)
{
    if(state != 0xFF)
    {
        if(state == 2 && pDivoomHandle->on_off_config.work_status == 0)
        {
            return pDivoomHandle->on_off_config.work_status;
        }
        pDivoomHandle->on_off_config.work_status = state;
    }
    return pDivoomHandle->on_off_config.work_status;
}

uint8 divoom_main_set_night_view_work_status(uint8 state)
{
    if(state != 0xFF)
    {
        if(state == 2 && pDivoomHandle->night_view.work_status == 0)
        {
            return pDivoomHandle->night_view.work_status;
        }
        pDivoomHandle->night_view.work_status = state;
    }
    return pDivoomHandle->night_view.work_status;
}

Pdivoom_sys_tidal_moment_info divoom_main_get_tidal_moment_data(void)
{
    return &pDivoomHandle->tidal_mement_info;
}

//取消定时开关机
void divoom_main_cancel_on_off(void)
{
	divoom_sys_msg_info model_msg = {0};

    if(pDivoomHandle->on_off_config.work_status == 1)
    {
        pDivoomHandle->on_off_config.work_status = 2;
		
		model_msg.dest_id = DIVOOM_MODEL_MAIN; 
        model_msg.msg_id = DIVOOM_MAIN_EVENT_POWER_OFF_DISP_MENU_CANCLE;
        divoom_sys_model_crl_msg_push(&model_msg);
    }
	
    divoom_light_app_open_screen();
    divoom_disp_ambient_ctrl_set_rgb_on_off(1);
}

int divoom_main_get_time_active_status(int start, int end)
{	
    time_t cur_time;
	struct tm cur_tm;   
    uint16 cur_work_status = 0, cur_min;


    
    cur_time = time(0);
    localtime_r((time_t *)&cur_time, &cur_tm);
    cur_min = cur_tm.tm_hour * 60 + cur_tm.tm_min; 
    
    if(start < end)
    {
        if(cur_min >= start && end > cur_min)
        {
            cur_work_status = 1;
        }
    }
    else
    {
        if(cur_min >= start || cur_min < end)
        {
            cur_work_status = 1;
        }
    }
    return cur_work_status;
}

void* divoom_main_get_power_on_off_config(void)
{
	if(pDivoomHandle)
	{
    	return &pDivoomHandle->on_off_config;
	}
	return NULL;
}


/**
 * NAME         divoom_main_push_model_event_id
 * @BRIEF	    向某个模块抛异步事件
 * @PARAM	    model_id
                event_id
                    
 * @RETURN	none
 * @RETVAL	
 */

void divoom_main_push_model_event_id(int model_id, uint8 event_id)
{
    divoom_sys_msg_info model_msg= {0};

#ifdef DIVOOM_SIMULATOR
    if (model_id == DIVOOM_MODEL_LIGHT_APP
        && event_id == DIVOOM_LIGHT_APP_EVENT_START_UP_DISP_DIAL)
    {
        s_sim_startup_dial_request_count++;
    }
#endif
    model_msg.msg_type = 1;
	model_msg.src_id = DIVOOM_MODEL_MAIN; 
	model_msg.dest_id = model_id; 
	model_msg.msg_id = event_id;
	divoom_sys_model_crl_msg_push(&model_msg);
}

#ifdef DIVOOM_SIMULATOR
void divoom_main_sim_reset_startup_dial_request_count(void)
{
    s_sim_startup_dial_request_count = 0;
}

uint32 divoom_main_sim_get_startup_dial_request_count(void)
{
    return s_sim_startup_dial_request_count;
}
#endif

//设置全局的图像句柄，为了方便显示

void divoom_main_set_global_image_handle(int model_id, int image_id, int voice_flag)
{
    pDivoomHandle->pglobal_image_model_id  = model_id;
    pDivoomHandle->pglobal_image_image_id  = image_id;
    pDivoomHandle->pglobal_image_voice_flag  = voice_flag;
}

//获取全局的图像句柄，为了方便显示

void divoom_main_get_global_image_handle(int *model_id, int *image_id, int *voice_flag)
{
    *model_id = pDivoomHandle->pglobal_image_model_id;
    *image_id = pDivoomHandle->pglobal_image_image_id;
    *voice_flag = pDivoomHandle->pglobal_image_voice_flag;
}


extern void divoom_bt_ctrl_init(DIVOOM_BT_STATUS_CB callback);

void divoom_main_clear_all_user_data(void)
{
    divoom_clock_manage_reset_all_app_config();
    divoom_album_manage_reset();
    divoom_favorite_manage_clear_all();
    divoom_user_define_ctrl_reset();
    divoom_expert_manage_reset();
    divoom_clock_schedule_clear_all();
}
int divoom_main_set_wifi_disconnect_notice_flag(int flag)
{
    if(!pDivoomHandle)
        return 0;
    if(flag >= 0)
    {
        pDivoomHandle->divoom_wifi_dis_connect_flag = flag;
    }
    return pDivoomHandle->divoom_wifi_dis_connect_flag;
}

int divoom_main_model_night_view_callback(void *para)
{
    divoom_main_clear_night_brightness();
    divoom_main_apply_sys_light();
    pDivoomHandle->pnight_view_time_handle = NULL;
    return 0;
}

void* divoom_main_model_get_power_on_item(void)
{
	if(pDivoomHandle)
		return &pDivoomHandle->power_on_info;
	
	return NULL;
}

#if DIVOOM_SHOW_MODE
#define DIVOOM_SHOW_STORE_POWER_WEEK  0x7F

static void divoom_main_show_fill_store_power_slots(Pdivoom_power_control_item items, uint32 id0, uint32 id1)
{
	memset(&items[0], 0, sizeof(items[0]));
	items[0].pow_on_id = id0;
	items[0].on_off = 1;
	items[0].mode = 0;
	items[0].hour = 9;
	items[0].minute = 0;
	items[0].week = DIVOOM_SHOW_STORE_POWER_WEEK;
	memset(&items[1], 0, sizeof(items[1]));
	items[1].pow_on_id = id1;
	items[1].on_off = 1;
	items[1].mode = 1;
	items[1].hour = 22;
	items[1].minute = 0;
	items[1].week = DIVOOM_SHOW_STORE_POWER_WEEK;
}

/* upload=0：GOT_IP 只写本地。upload=1：GetList 之后带云端 id 做 SET，避免旧列表盖掉营业时间 */
static void divoom_main_show_seed_store_power_schedule(int upload)
{
	Pdivoom_power_control_item items;
	int i;
	int has_enabled;
	uint32 id0;
	uint32 id1;

	if(pDivoomHandle == NULL)
	{
		return;
	}
	if(pDivoomHandle->sys_setting.show_store_sched_flag)
	{
		return;
	}
	items = (Pdivoom_power_control_item)divoom_main_model_get_power_on_item();
	if(items == NULL)
	{
		return;
	}

	if(upload)
	{
		id0 = items[0].pow_on_id;
		id1 = items[1].pow_on_id;
		divoom_main_show_fill_store_power_slots(items, id0, id1);
		divoom_power_control_send(&items[0]);
		divoom_power_control_send(&items[1]);
		divoom_power_control_update(NULL, 2);
		pDivoomHandle->sys_setting.show_store_sched_flag = 1;
		divoom_main_save_config();
		divoom_print("show store schedule: 09:00 on / 22:00 off (cloud)\n");
		return;
	}

	has_enabled = 0;
	for(i = 0; i < DIVOOM_POWER_CTRL_NUM; i++)
	{
		if(items[i].on_off)
		{
			has_enabled = 1;
			break;
		}
	}
	if(has_enabled == 0)
	{
		divoom_main_show_fill_store_power_slots(items, 0, 0);
		divoom_power_control_update(NULL, 2);
		divoom_main_save_config();
		divoom_print("show store schedule: 09:00 on / 22:00 off\n");
	}
}
#endif

/* 与闹钟相同：云端 RepeatArray[0..5]=周一到周六、[6]=周日 → 本地 bit1…bit6 / bit0 */
static uint8 divoom_main_power_week_from_cyc_flag(const int *cyc_flag)
{
	uint8 week;
	int j;

	week = 0;
	if(cyc_flag == NULL)
	{
		return 0;
	}
	if(cyc_flag[6])
	{
		week = 1;
	}
	for(j = 0; j < 6; j++)
	{
		if(cyc_flag[j])
		{
			week |= (uint8)(1 << (j + 1));
		}
	}
	return week;
}

void divoom_main_model_set_power_on_item_info(Pdivoom_device_power_control_item power_on_item)
{
	int find_index = -1;
	int i;

	if(power_on_item == NULL)
	{
		return;
	}
	
	for(i = 0; i < DIVOOM_POWER_CTRL_NUM; i++)
	{
		if(pDivoomHandle->power_on_info[i].pow_on_id == power_on_item->power_on_id)
		{
			find_index = i;
			break;
		}
	}
	
	if(find_index == -1)
	{
		for(i = 0; i < DIVOOM_POWER_CTRL_NUM; i++)
		{
			if(pDivoomHandle->power_on_info[i].pow_on_id == 0)
			{
				pDivoomHandle->power_on_info[i].pow_on_id = power_on_item->power_on_id;

				pDivoomHandle->power_on_info[i].on_off = power_on_item->on_off; 
				pDivoomHandle->power_on_info[i].hour = power_on_item->time/3600;
				pDivoomHandle->power_on_info[i].minute = power_on_item->time/60%60;
				pDivoomHandle->power_on_info[i].mode = power_on_item->power_on_flag;

				pDivoomHandle->power_on_info[i].week = divoom_main_power_week_from_cyc_flag(power_on_item->cyc_flag);

				break;
			}
		}
	
		if(i >= DIVOOM_POWER_CTRL_NUM)
		{
			//设备和app的pow_on_id对应不上，重新获取list
			divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_POWER_ON_LIST, DIVOOM_SERVER_TOPIC,NULL);
		}
	}
	else if((find_index >= 0) && (find_index < DIVOOM_POWER_CTRL_NUM))
	{
		pDivoomHandle->power_on_info[find_index].on_off = power_on_item->on_off; 

		pDivoomHandle->power_on_info[find_index].hour = power_on_item->time/3600;
		pDivoomHandle->power_on_info[find_index].minute = power_on_item->time/60%60;
		pDivoomHandle->power_on_info[find_index].mode = power_on_item->power_on_flag;

		pDivoomHandle->power_on_info[find_index].week = divoom_main_power_week_from_cyc_flag(power_on_item->cyc_flag);

	}
	
}

void* divoom_main_model_get_danmaku_config(void)
{
	if(pDivoomHandle)
		return &pDivoomHandle->danmaku_config;
	
	return NULL;
}

void divoom_main_model_enter_debug_mode(void)
{
	char kill_aactd_cmd[256] = "for pid in $(ps | awk '$5==\"aactd\" {print $1}'); do kill -9 $pid;done";
	char kill_adbd_cmd[256] = "for pid in $(ps | awk '$5==\"/bin/adbd\" {print $1}'); do kill -9 $pid;done";
			
	exec_command_system(kill_aactd_cmd);	
	exec_command_system(kill_adbd_cmd);
}

uint8 divoom_main_model_event(Pdivoom_sys_msg_info msg)
{
    divoom_sys_msg_info model_msg = {0};

    model_msg.src_id = DIVOOM_MODEL_MAIN;
    model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;  

    divoom_print("%s:event id: %d\n", __func__, msg->msg_id);
    if (divoom_main_ignore_userdata_event(msg->msg_id))
    {
        divoom_print("userdata maintenance: ignore MAIN event %d\n", msg->msg_id);
        return 1; /* The controller retains ownership and frees message data. */
    }
    switch(msg->msg_id)
    {
        case DIVOOM_MAIN_EVENT_SET_BETA_FEATURES:
        {
            if (divoom_main_get_server_type() && pDivoomHandle->sys_setting.beta_features != msg->attr)
            {
                pDivoomHandle->sys_setting.beta_features = msg->attr != 0;
                pDivoomHandle->sys_setting.beta_test_flag_pending = 1;
                pDivoomHandle->update_flag = 1;
                pDivoomHandle->beta_test_flag_packet = 0;
                divoom_main_report_beta_features();
            }
            break;
        }
        case DIVOOM_MAIN_EVENT_BETA_FEATURES_SAVED:
        {
            /* 连续修改时，较早的确认无法清掉新选择的待同步标志。 */
            if (divoom_main_get_server_type() && pDivoomHandle->divoom_connect_srv &&
                pDivoomHandle->sys_setting.beta_test_flag_pending &&
                pDivoomHandle->beta_test_flag_packet &&
                pDivoomHandle->beta_test_flag_packet == (uint32)msg->result)
            {
                pDivoomHandle->beta_test_flag_packet = 0;
                pDivoomHandle->sys_setting.beta_test_flag_pending = 0;
                pDivoomHandle->update_flag = 1;
            }
            break;
        }

		case DIVOOM_MAIN_EVENT_SEND_DANMAKU_FACE:
		{
			int face_index = *(int*)msg->data;

			divoom_lvgl_top_danmaku_menu_add_danmaku_image_item(0, 1, face_index);
		}
			break;

		case DIVOOM_MAIN_EVENT_GET_DEBUG_FLAG:
		{
			int debug_flag = *(int*)msg->data;

			if(debug_flag == 1)
			{
				divoom_main_model_enter_debug_mode();
			}
		}
			break;

		case DIVOOM_MAIN_EVENT_SEND_DANMAKU_TEXT:
		{
			Pdivoom_device_danmaku_text_item pdanmaku_text = (Pdivoom_device_danmaku_text_item)msg->data;

			divoom_lvgl_top_danmaku_menu_add_danmaku_label_item(pdanmaku_text->disp_text, pdanmaku_text->text_color, 1, pdanmaku_text->user_id);
		}
			break;

		case DIVOOM_MAIN_EVENT_SEND_RANDOM_DANMAKU_FACE:
		{
			divoom_lvgl_top_danmaku_menu_add_danmaku_image_item(0, 1, -1);
		}
			break;

		case DIVOOM_MAIN_EVENT_GET_DANMAKU_CONFIG:
		{
			Pdivoom_device_danmaku_config_item pdanmaku_config = (Pdivoom_device_danmaku_config_item)msg->data;

			pDivoomHandle->danmaku_config.disp_area_flag = pdanmaku_config->disp_area_flag;
			pDivoomHandle->danmaku_config.speed = pdanmaku_config->speed;
			
		}
			break;

		case DIVOOM_MAIN_EVENT_SET_DANMAKU_CONFIG:
		{
			Pdivoom_device_danmaku_config_item pdanmaku_config = (Pdivoom_device_danmaku_config_item)msg->data;

			pDivoomHandle->danmaku_config.disp_area_flag = pdanmaku_config->disp_area_flag;
			pDivoomHandle->danmaku_config.speed = pdanmaku_config->speed;

		}
			break;
	
		case DIVOOM_MAIN_EVENT_SET_POWER_ON_MODE:
		{
			Pdivoom_device_power_control_item power_on_item = (Pdivoom_device_power_control_item)msg->data;

			divoom_main_model_set_power_on_item_info(power_on_item);
			divoom_power_control_update(NULL, 3);
			divoom_main_save_config();
		}
			break;
			
		case DIVOOM_MAIN_EVENT_GET_POWER_ON_LIST:
		{
			int i;
			Pdivoom_device_power_control_list power_on_list = (Pdivoom_device_power_control_list)msg->data;    
			
			for(i = 0; i < power_on_list->power_on_list_num; i++)
			{
				if(i < DIVOOM_POWER_CTRL_NUM)
				{
					int on_off = power_on_list->power_on_list[i].on_off;

					pDivoomHandle->power_on_info[i].pow_on_id = power_on_list->power_on_list[i].power_on_id;
					if (divoom_power_control_should_force_disable_pow_on_id(
							power_on_list->power_on_list[i].power_on_id))
						on_off = 0;
					pDivoomHandle->power_on_info[i].on_off = on_off;
					pDivoomHandle->power_on_info[i].hour = power_on_list->power_on_list[i].time/3600;
					pDivoomHandle->power_on_info[i].minute = power_on_list->power_on_list[i].time/60%60;
					pDivoomHandle->power_on_info[i].mode = power_on_list->power_on_list[i].power_on_flag;

					pDivoomHandle->power_on_info[i].week = divoom_main_power_week_from_cyc_flag(power_on_list->power_on_list[i].cyc_flag);
				}
			}

			divoom_power_control_on_get_list_sync();
#if DIVOOM_SHOW_MODE
			divoom_main_show_seed_store_power_schedule(1);
#endif
			divoom_power_control_update(NULL, 2);
			divoom_main_save_config();
		}
			break;
	
        case DIVOOM_MAIN_EVENT_CREATE_MOIVE_REVIEW_PIC:
            divoom_software_timer_create_timer(4, divoom_decode_movie_review_pic_create_delay_run, msg->data);
            msg->data = NULL;
            break;
            
    	case DIVOOM_MAIN_EVENT_SET_NIGHT_VIEW_MODE:
            if(pDivoomHandle->pnight_view_time_handle)
            {
                divoom_software_timer_delete_timer(pDivoomHandle->pnight_view_time_handle);
                pDivoomHandle->pnight_view_time_handle = NULL;
            }
            divoom_main_set_night_brightness((uint8)(*(int *)msg->data));
            pDivoomHandle->pnight_view_time_handle = divoom_software_timer_create_timer(5, divoom_main_model_night_view_callback, NULL);
            break;
            
    	case DIVOOM_MAIN_EVENT_EXIT_NIGHT_VIEW_MODE:
            if(pDivoomHandle->pnight_view_time_handle)
            {
                divoom_software_timer_delete_timer(pDivoomHandle->pnight_view_time_handle);
                pDivoomHandle->pnight_view_time_handle = NULL;
            }
            divoom_main_clear_night_brightness();
            divoom_main_apply_sys_light();
            
            break;
            
		case DIVOOM_MAIN_EVENT_POWER_OFF_DISP_MENU_ENTER:
    	{
			divoom_app_schedule_item_info push_info = {0};

        	push_info.event_id = DIVOOM_MAIN_PLAY_ENTER_POWER_OFF_DISP_MENU_EVENT;
        	push_info.model_id = DIVOOM_MODEL_MAIN;
            push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
        	if(divoom_app_schedule_push(&push_info))
        	{
        		//divoom_lvgl_top_power_off_disp_menu_init();
        		divoom_system_msg_notice_clear_all(1);
                divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_POWER_OFF_DISP_MENU, NULL, DIVOOM_MODEL_MAIN, DIVOOM_MAIN_PLAY_ENTER_POWER_OFF_DISP_MENU_EVENT);
        	}
		}
			break;

		case DIVOOM_MAIN_EVENT_POWER_OFF_DISP_MENU_CANCLE:
		{
			divoom_app_schedule_item_info push_info = {0};

			divoom_light_app_open_screen();
			divoom_disp_ambient_ctrl_set_rgb_on_off(1);
			//voom_lvgl_top_power_off_disp_menu_exit();

        	push_info.event_id = DIVOOM_MAIN_PLAY_ENTER_POWER_OFF_DISP_MENU_EVENT;
        	push_info.model_id = DIVOOM_MODEL_MAIN;
        	push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
       		divoom_app_schedule_cancel(&push_info);
			if (divoom_menu_control_get_active_menu_id() == DIVOOM_MENU_CONTROL_ID_POWER_OFF_DISP_MENU)
			{
				/* 与主菜单 ESC 回表盘相同：表盘是 ROOT 不压栈，栈空则 clearall 重建 */
				if (divoom_menu_control_get_stack_depth() == 0)
					divoom_main_return_startup_dial();
				else
					divoom_menu_control_return_pre();
			}
			else
				divoom_menu_control_redisp_current();
		}	
			break;
	
		case DIVOOM_MAIN_EVENT_BRIGHT_MENU_ENTER:
    	{
			divoom_lvgl_top_bright_volume_menu_init(1);
		}
			break;

		case DIVOOM_MAIN_EVENT_VOLUME_MENU_ENTER:
    	{
			divoom_lvgl_top_bright_volume_menu_init(2);
		}
			break;	
		
    	case DIVOOM_MAIN_EVENT_WIFI_NOTICE_MENU_ENTER:
    	{
        	divoom_app_schedule_item_info push_info = {0};

            divoom_lvgl_wifi_disconnect_exit();
        	push_info.event_id = DIVOOM_MAIN_PLAY_WIFI_CONFIG_EVENT;
        	push_info.model_id = DIVOOM_MODEL_MAIN;
            push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
            pDivoomHandle->sys_setting.wifi_config_flag = 1;
            pDivoomHandle->divoom_wifi_dis_connect_flag = 1;
            if(divoom_app_schedule_push(&push_info))
            {
                /* 勿压 CONFIG_WIFI：扫描时会 EXIT_CONFIG，破坏返回栈；仅进添加网络扫描页。 */
                divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_WIFI_ADD,
                    NULL, DIVOOM_MODEL_MAIN, DIVOOM_MAIN_PLAY_POWER_ON_EVENT);
            }
    	}
            break;
            
    	case DIVOOM_MAIN_EVENT_WIFI_NOTICE_MENU_CANCLE:
            divoom_lvgl_wifi_disconnect_exit();
            pDivoomHandle->divoom_wifi_dis_connect_flag = 1;
            break;
            
        case DIVOOM_MAIN_EVENT_CLEAR_USER_PHOTO:
            if(divoom_menu_control_get_active_menu_id() == DIVOOM_MENU_CONTROL_ID_PHOTO_MANAGE || (divoom_menu_control_get_active_menu_id() == DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID && divoom_menu_control_get_clock_mode_menu_handle()))
            {
                if(divoom_lvgl_base_dial_menu_is_play_photo_slide(divoom_menu_control_get_clock_mode_menu_handle()))
                {
                    int clock_id = divoom_light_app_check_clock_exist(0);
                    model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SET_CLOCK_INDEX;
                    model_msg.data = (uint8 *)&clock_id;                    
        			divoom_sys_model_crl_msg_push(&model_msg);
                }
            }
            divoom_pic_phone_manage_remove_all_pic();
            break;
            
        case DIVOOM_MAIN_EVENT_RAM_GARBAGE:
            divoom_print("divoom collect ram garbage!\n");
            divoom_system_delay_ms(100);
            malloc_trim(0);
            break;
            
        case DIVOOM_MAIN_EVENT_EXIT_MENU_SETTING:
        {
            divoom_app_schedule_item_info push_info = {0};
            
            push_info.event_id = DIVOOM_MAIN_PLAY_ENTER_SYSTEM_MENU_SETTING;
            push_info.model_id = DIVOOM_MODEL_MAIN;
            push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
            divoom_app_schedule_cancel(&push_info);
        }
            break;
        case DIVOOM_MAIN_EVENT_ENTER_MENU_SETTING:
        {
        	divoom_app_schedule_item_info push_info = {0};

        	push_info.event_id = DIVOOM_MAIN_PLAY_ENTER_SYSTEM_MENU_SETTING;
        	push_info.model_id = DIVOOM_MODEL_MAIN;
            push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
            pDivoomHandle->sys_setting.wifi_config_flag = 1;
        	if(divoom_app_schedule_push(&push_info))
        	{	
                divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETING_MENU, NULL, DIVOOM_MODEL_MAIN, DIVOOM_MAIN_PLAY_ENTER_SYSTEM_MENU_SETTING);
        	}
            divoom_main_save_config();
        }
            break;
            
        case DIVOOM_MAIN_EVENT_WIFI_BT_START_EVENT:
            break;
        
        case DIVOOM_MAIN_EVENT_BT_ON_EVENT:
            break;
            
        case DIVOOM_MAIN_EVENT_CONFIG_NET_WORK_OVER:
        case DIVOOM_MAIN_EVENT_CONFIG_NET_WORK_CANCLE:
        {           
            uint8 event_id, model_id;
            
            model_id = divoom_app_schedule_get_active_model(&event_id, DIVOOM_APP_SCHEDULE_RESOURCCE_RGB);
            if(model_id == DIVOOM_MODEL_MAIN && event_id == DIVOOM_MAIN_PLAY_WIFI_CONFIG_EVENT)
            {
                divoom_app_schedule_item_info push_info = {0};
                
                push_info.event_id = DIVOOM_MAIN_PLAY_WIFI_CONFIG_EVENT;
                push_info.model_id = DIVOOM_MODEL_MAIN;
                push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
                divoom_app_schedule_cancel(&push_info);
            }
            else
            {
				if(msg->msg_id == DIVOOM_MAIN_EVENT_CONFIG_NET_WORK_CANCLE)
				{
					if(divoom_wifi_model_get_wifi_status() == 0)
					{                        
						divoom_wifi_connect_other_ssid();
					}
				}

				divoom_menu_control_return_pre();
            }
        }
            break;
            
    	case DIVOOM_MAIN_EVENT_CONFIG_NET_WORK:            //配置网络   
    	{
        	divoom_app_schedule_item_info push_info = {0};

        	push_info.event_id = DIVOOM_MAIN_PLAY_WIFI_CONFIG_EVENT;
        	push_info.model_id = DIVOOM_MODEL_MAIN;
            push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
            /* 重置网络只清配网/绑定，保留本机语言。写成 -1 会进 OOBE 语言页，
             * 且 GetConf 容易被云端默认英语覆盖。back_lang 给当次重连识别用。 */
            pDivoomHandle->back_lang = pDivoomHandle->sys_setting.lang;
            pDivoomHandle->sys_setting.wifi_config_flag = 0;
    		if(divoom_net_app_net_is_ok() == 0)
    		{
                pDivoomHandle->sys_setting.reset_flag = 1;
    		}
            divoom_main_save_config();
        	if(divoom_app_schedule_push(&push_info))
        	{	
                //pDivoomHandle->wifi_config_reset_all_flag = 1;
        		//divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_RESET_ALL, DIVOOM_SERVER_TOPIC, NULL);
				divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_RESET_ALL, DIVOOM_SERVER_TOPIC, NULL);
                divoom_system_delay_ms(500);
                /*
                 * 先停图片队列和在途 HTTP，再停 MQTT、断 WiFi。
                 * MQTT 还占着 TCP 时 aw_wifi_disconnect→fhost_wpa_end
                 * 会等到 keepalive 超时（约 60s）。abort 只在本拆网段调用，
                 * 断网返回后立刻 clear，避免再配网时 initv2 被挡。
                 * 不用 server_msg_manage_deinit / image_download_manage_stop：
                 * 重置网络后设备继续跑。
                 */
                divoom_manage_image_up_netstatus(0);
                divoom_http_client_request_abort();
                divoom_mqtt_model_remove_server_connect();
                pDivoomHandle->divoom_wifi_dis_connect_flag = 1;
                divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_CONFIG_WIFI, NULL, DIVOOM_MODEL_MAIN, DIVOOM_MAIN_PLAY_POWER_ON_EVENT);
                divoom_wifi_clear_all_wifi();
                divoom_http_client_request_abort_clear();
        	}
            divoom_sys_model_crl_send_all_model(DIVOOM_DEVICE_SYSTEM_RESET_ALL);
        }
    	    break;
            
        case DIVOOM_MAIN_EVENT_CHECK_SYSTEM_FILES:
            /* Keep queued status while configuration/marker I/O is in progress.
             * A rejection is terminal: the About UI then permits returning. */
            if (divoom_ota_server_get_update_status() || !divoom_update_system_file_health_check_repair_ready()) {
                divoom_update_system_file_cancel_health_check_repair();
                divoom_product_test_format_settings_request_set_status(-1);
                break;
            }
#if DIVOOM_BUILD_RTOS_DEVICE
            if (divoom_ota_rtos_is_running() || divoom_ota_rtos_flash_has_started()) {
                divoom_update_system_file_cancel_health_check_repair();
                divoom_product_test_format_settings_request_set_status(-1);
                break;
            }
#endif
            divoom_main_save_config(); /* Persist the selected language before the maintenance gate. */
            if (!divoom_product_test_format_arm_settings_check() ||
                !divoom_main_begin_userdata_maintenance()) {
                divoom_update_system_file_cancel_health_check_repair();
                divoom_product_test_format_settings_request_set_status(-1);
                break;
            }
            divoom_product_test_format_settings_request_set_status(1);
            /* Run teardown in main's existing worker, never under the LVGL lock. */
            divoom_manage_image_up_netstatus(0);
            divoom_http_client_request_abort();
            divoom_main_exit();
            if (divoom_port_sync() == 0) (void)divoom_boot_health_clean();
            divoom_system_reboot();
            divoom_main_userdata_maintenance_returned();
            break;

    	case DIVOOM_MAIN_EVENT_FORMAT_USERDATA:            //格式化内部储存
    	    break;
            
    	case DIVOOM_MAIN_EVENT_RESET_FACTORY_SETTING:            //恢复出厂设置
    	{
            /* 云端等非 UI 入口：先切等待页，由等待页刷完后再 push 本事件 */
            if (divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_ID_RESETING_DEVICE_MENU)
            {
                divoom_print("factory reset: not on wait page, load UI first\n");
                divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_RESETING_DEVICE_MENU, NULL, 0, 0);
                break;
            }
            if (!divoom_main_begin_userdata_maintenance())
                break;
			divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_CLEAR_USER_ALBUM, DIVOOM_SERVER_TOPIC, NULL);
			divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_RESET_ALL, DIVOOM_SERVER_TOPIC, NULL);
            divoom_system_delay_ms(500);
            /*
             * mkfs 会强行 unmount /userdata，在途的 FILE* 立即失效，故先让下载/服务器
             * 任务收尾。等待必须在断网之前：curl 超时 300s，网断了 stop() 的 5s 等待必然
             * 超时返回，句柄却还在写。维护状态会忽略并发 WIFI_DISCON 等冲突事件，
             * 所以必须在这里主动完成下载/服务器收尾，且等待期间不持有配置锁。
             */
            divoom_manage_image_up_netstatus(0);
            divoom_http_client_request_abort();
            divoom_image_download_manage_stop();
            divoom_server_msg_manage_deinit();
            divoom_print("factory reset: download/server stopped\n");
            divoom_mqtt_model_remove_server_connect();
            divoom_print("factory reset: restore default bt name\n");
            divoom_main_set_blue_name((uint8)strlen(DIVOOM_BLUE_BLE_DEVICE_NAME),
                (char *)DIVOOM_BLUE_BLE_DEVICE_NAME);
            if (divoom_bt_music_set_jl_adapter_name(divoom_main_get_blue_name()) != 0)
                divoom_print("factory reset: set jl bt name failed\n");
            divoom_print("factory reset: MQTT stopped, clear Wi-Fi\n");
            divoom_wifi_clear_all_wifi();
#if DIVOOM_SHOW_MODE
            divoom_print("factory reset: show mode release tf music\n");
            divoom_local_music_ctrl_show_release_for_format();
            divoom_show_tf_music_fetch_cancel();
#endif
            divoom_http_client_request_abort_clear();
            divoom_print("factory reset: format udisk\n");
            divoom_main_format_udisk_partition_active();
            divoom_main_userdata_maintenance_returned();
    	}
    	    break;
            
        case DIVOOM_MAIN_NOTICE_INIT_ON_OFF_TIME:
        {
            Pdivoom_device_on_off_config_para pon_off_config = (Pdivoom_device_on_off_config_para)msg->data;   
            
            pDivoomHandle->on_off_config.on_off = pon_off_config->on_off;
            pDivoomHandle->on_off_config.start = pon_off_config->start;
            pDivoomHandle->on_off_config.end = pon_off_config->end;
            pDivoomHandle->on_off_config.work_status = 0;
            #if 0
            if(pDivoomHandle->on_off_config.on_off && pDivoomHandle->on_off_config.work_status == 0)
            {
                if(divoom_main_get_time_active_status(pDivoomHandle->on_off_config.start, pDivoomHandle->on_off_config.end))
                {
                    //开机这一次直接取消
                    divoom_print("we will pass\n");
                    pDivoomHandle->on_off_config.work_status = 2;
                }
            }
            #endif
            divoom_print("we will init on off with %d, %d,%d, %d!\n", pDivoomHandle->on_off_config.on_off, pDivoomHandle->on_off_config.work_status, pDivoomHandle->on_off_config.start, pDivoomHandle->on_off_config.end);
            
        }
            break;
                
        case DIVOOM_MAIN_NOTICE_UPDATE_ON_OFF_TIME:
        {
            Pdivoom_device_on_off_config_para pon_off_config = (Pdivoom_device_on_off_config_para)msg->data;            
            uint16 cur_work_status = 0, cur_min;
            time_t cur_time;
        	struct tm cur_tm;  

            if(pon_off_config->on_off)
            {  
                //先统计一下当前是否在工作状态
                cur_time = time(0);
    	        localtime_r((time_t *)&cur_time, &cur_tm);
                cur_min = cur_tm.tm_hour * 60 + cur_tm.tm_min;
                if(pon_off_config->start < pon_off_config->end)
                {
                    if(cur_min >= pon_off_config->start && pon_off_config->end > cur_min)
                    {
                        cur_work_status = 1;
                    }
                }
                else
                {
                    if(cur_min >= pon_off_config->start || cur_min < pon_off_config->end)
                    {
                        cur_work_status = 1;
                    }
                }
            }
            if(pDivoomHandle->on_off_config.work_status == 1)
            {
                if(cur_work_status == 0)
                {
                    //将关闭,恢复亮度
                    //int on_off_switch = 1;
                    divoom_print("we will resotre the bright!\n");
        			//model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
        			//model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SCREEN_CTRL;
                    //model_msg.data = (uint8 *)&on_off_switch;
        			//divoom_sys_model_crl_msg_push(&model_msg);

					//divoom_disp_ambient_ctrl_set_rgb_on_off(1);
					
                    model_msg.dest_id = DIVOOM_MODEL_MAIN; 
        			model_msg.msg_id = DIVOOM_MAIN_EVENT_POWER_OFF_DISP_MENU_CANCLE;
        			divoom_sys_model_crl_msg_push(&model_msg);
					
                    pDivoomHandle->on_off_config.work_status = 0;
                }
            }
            else if(pDivoomHandle->on_off_config.work_status == 2)
            {
                pDivoomHandle->on_off_config.work_status = 0;
            }
            pDivoomHandle->on_off_config.on_off = pon_off_config->on_off;
            pDivoomHandle->on_off_config.start = pon_off_config->start;
            pDivoomHandle->on_off_config.end = pon_off_config->end;
            divoom_print("we will set on off with %d, %d,%d, %d!\n", pDivoomHandle->on_off_config.on_off, pDivoomHandle->on_off_config.work_status, pDivoomHandle->on_off_config.start, pDivoomHandle->on_off_config.end);
        }
            break;
         
        case DIVOOM_MAIN_EVENT_INIT_DEVICE_NIGHT_VIEW:
        {
            Pdivoom_system_setting_json_night_view pngiht_view = (Pdivoom_system_setting_json_night_view)msg->data;   
            
            pDivoomHandle->night_view.bright = pngiht_view->bright;
            pDivoomHandle->night_view.on_off = pngiht_view->on_off;
            pDivoomHandle->night_view.start = pngiht_view->start;
            pDivoomHandle->night_view.end = pngiht_view->end;
            pDivoomHandle->night_view.work_status = 0;
            #if 0
            if(pDivoomHandle->night_view.on_off && pDivoomHandle->night_view.work_status == 0)
            {
                if(divoom_main_get_time_active_status(pDivoomHandle->night_view.start, pDivoomHandle->night_view.end))
                {
                    //开机这一次直接取消
                    pDivoomHandle->night_view.work_status = 1;
                }
            }
            #endif
            divoom_print("we will set nightview with %d, %d!\n", pDivoomHandle->night_view.on_off, pDivoomHandle->night_view.bright);
        }
            break;
               
        case DIVOOM_MAIN_EVENT_SET_DEVICE_NIGHT_VIEW:
        {
            Pdivoom_system_setting_json_night_view pngiht_view = (Pdivoom_system_setting_json_night_view)msg->data;            
            uint16 cur_work_status = 0, cur_min;
            time_t cur_time;
        	struct tm cur_tm;

            if(pngiht_view->on_off)
            {  
                //先统计一下当前是否在工作状态
                cur_time = time(0);
    	        localtime_r((time_t *)&cur_time, &cur_tm);
                cur_min = cur_tm.tm_hour * 60 + cur_tm.tm_min;
                if(pngiht_view->start < pngiht_view->end)
                {
                    if(cur_min >= pngiht_view->start && pngiht_view->end > cur_min)
                    {
                        cur_work_status = 1;
                    }
                }
                else
                {
                    if(cur_min >= pngiht_view->start || cur_min < pngiht_view->end)
                    {
                        cur_work_status = 1;
                    }
                }
            }
            //if(pDivoomHandle->night_view.on_off && pDivoomHandle->night_view.work_status && pngiht_view->on_off == 0)
            if(pDivoomHandle->night_view.work_status && (pngiht_view->on_off == 0 || cur_work_status == 0))
            {
                //将关闭,恢复亮度
                divoom_print("we will resotre the bright!\n");
                if(divoom_light_app_get_screen_status(0xFF))
                {
                    divoom_main_clear_night_brightness();
                    divoom_main_apply_sys_light();
                    //divoom_disp_ambient_ctrl_set_brightness(NULL, 1);
                }
                
                pDivoomHandle->night_view.work_status = 0;
            }
            memcpy(&pDivoomHandle->night_view, pngiht_view, sizeof(pDivoomHandle->night_view));
            if (pDivoomHandle->night_view.on_off && cur_work_status
                && divoom_light_app_get_screen_status(0xFF))
            {
                pDivoomHandle->night_view.work_status = 1;
                divoom_main_set_night_brightness(pDivoomHandle->night_view.bright);
            }
			divoom_print("we will set nightview with %d, %d!\n", pDivoomHandle->night_view.on_off, pDivoomHandle->night_view.bright);
        }
            break;
            
        case DIVOOM_MAIN_EVENT_SD_FREE_ROOM_NOT_ENOUGH:
            divoom_main_notice_sd_free_warning();
            break;
            
        case DIVOOM_MAIN_NOTICE_RESET_APP_MQTT_CON:
            if(pDivoomHandle->divoom_connect_srv)
            {
                divoom_manage_image_up_netstatus(1);    //开启下载
            }
            divoom_main_manage_reconnect_all_app_con();
            break;
            
        case DIVOOM_MAIN_NOTICE_WIFI_NOTICE_MESSAGE:
        {
            divoom_app_schedule_item_info push_info = {0};
            
            //取消开机动画
            divoom_main_disp_wifi_config(1, 10000);
            push_info.event_id = DIVOOM_MAIN_PLAY_POWER_ON_EVENT;
            push_info.model_id = DIVOOM_MODEL_MAIN;
            push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
            divoom_app_schedule_cancel(&push_info);
            break;
        }
        
        case DIVOOM_DEVICE_SYSTEM_RESET_ALL:
            //先删除开机动画，再向系统获取相关配置
            {
                divoom_main_clear_all_user_data();
            }
            break;
        case DIVOOM_MAIN_EVENT_FORMAT_SD_OK:
        {
        }
            break;
            
        case DIVOOM_MAIN_EVENT_NEW_SD_NOTICE: 
            if(pDivoomHandle->divoom_connect_srv)
            {
                divoom_main_give_wait_download_msg();
            }
            else
            {
                divoom_print("pass the event!\n");
            }
            
            break;
            
        case DIVOOM_MAIN_EVENT_DOWNLOAD_ALL_DATA:
        {
            Pdivoom_disp_ctrl_download_file_info file_info = (Pdivoom_disp_ctrl_download_file_info)msg->data;

            divoom_print("file num: %d, %d\n", file_info->down_num, file_info->wait_down_num);
            if(file_info->wait_down_num == 0)
            {
                divoom_app_schedule_item_info push_info = {0};

                pDivoomHandle->new_sd_flag = 0;
                push_info.event_id = DIVOOM_MAIN_PLAY_DOWNLOAD_OK_EVENT;
                push_info.model_id = DIVOOM_MODEL_MAIN;
                push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;

                divoom_app_schedule_cancel(&push_info);
                push_info.event_id = DIVOOM_MAIN_PLAY_SD_ERR_EVENT;
                divoom_app_schedule_cancel(&push_info);
                push_info.event_id = DIVOOM_MAIN_PLAY_POWER_ON_EVENT;
                divoom_app_schedule_cancel(&push_info);
                
                model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;
    			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SET_CHANNEL_INDEX;
                model_msg.data = NULL;
    			divoom_sys_model_crl_msg_push(&model_msg);
            }
            else
            {
                if(divoom_disp_ctrl_get_mode() == DIVOOM_DISP_DOWNLOAD_FILE_MODE)
                {
                    divoom_disp_ctrl_set_mode(DIVOOM_DISP_DOWNLOAD_FILE_MODE, file_info);
                }
                
            }
            

            
        }
            break;
            
		case DIVOOM_MAIN_EVENT_LEDSEG_UPDATE_FLAG:
			{
				//断码屏时间更新
				uint8 *pflag = (uint8 *)msg->data;
				if(pflag)
				{
					if(pflag[0])
					{
						if(0 == pDivoomHandle->ledseg_update_flag)
						{
//							第一次时先执行一次更新，不然最长可能要等待1分钟才生效
							divoom_main_ledseg_update_time();
						}
						pDivoomHandle->ledseg_update_flag = 1;
					}
					else
					{
						pDivoomHandle->ledseg_update_flag = 0;
					}
				}
			}
			break;

		case DIVOOM_MAIN_EVENT_SET_TIMEZONE:
			{
				divoom_wifi_get_tz ptz = {0};

                strcpy(ptz.tzcode, (char *)msg->data);
				divoom_main_set_timezone(&ptz);
			}
			break;

		case DIVOOM_MAIN_EVENT_SET_TIME:
		{
			uint32 utc_time = *(uint32 *)msg->data;

			divoom_main_load_timezone_config();
			//divoom_time_set(utc_time, NULL);
			break;
		}

        case DIVOOM_MAIN_EVENT_SET_CURRENT_WEATHER:
        {
            const divoom_disp_weather_info *received = (const divoom_disp_weather_info *)msg->data;
            divoom_disp_weather_info *weather = &pDivoomHandle->weather_info;
            const uint8 pending = 1U << DIVOOM_DISP_WEATHER_TODAY_INFO;
            int first_adopted = 0;

            pthread_mutex_lock(&s_weather_lock);
            if (received && (s_weather_mqtt_pending & pending)) {
                first_adopted = s_weather_mqtt_pending == DIVOOM_WEATHER_MQTT_ALL_PENDING;
                s_weather_mqtt_pending &= ~pending;
                weather->request_weather_flag &= ~pending;
                weather->weather_pic_id = received->weather_pic_id;
                weather->pressure = received->pressure;
                weather->humidity = received->humidity;
                weather->visibility = received->visibility;
                weather->sunrise_time = received->sunrise_time;
                weather->sunset_time = received->sunset_time;
                weather->wind_speed = received->wind_speed;
                weather->temp = received->temp;
                weather->min_temp = received->min_temp;
                weather->max_temp = received->max_temp;
                strcpy(weather->weather_str, received->weather_str);
                weather->request_time = time(0);
                weather->update_real_time = weather->request_time + DIVOOM_MAIN_WEATHER_UPDATE_TIME;
                pDivoomHandle->sunrise_second = weather->sunrise_time;
                pDivoomHandle->sunset_second = weather->sunset_time;
            }
            pthread_mutex_unlock(&s_weather_lock);
            if (first_adopted) {
                divoom_print("Weather source: DIVOOM MQTT\n");
            }
            break;
        }

        case DIVOOM_MAIN_EVENT_SET_FUTURE_WEATHER:
        {
            const divoom_disp_weather_info *received = (const divoom_disp_weather_info *)msg->data;
            divoom_disp_weather_info *weather = &pDivoomHandle->weather_info;
            const uint8 pending = 1U << DIVOOM_DISP_WEATHER_FURTHER_INFO;
            int first_adopted = 0;

            pthread_mutex_lock(&s_weather_lock);
            if (received && (s_weather_mqtt_pending & pending)) {
                first_adopted = s_weather_mqtt_pending == DIVOOM_WEATHER_MQTT_ALL_PENDING;
                s_weather_mqtt_pending &= ~pending;
                weather->request_weather_flag &= ~pending;
                memcpy(weather->futrue_table, received->futrue_table, sizeof(weather->futrue_table));
                weather->request_time = time(0);
                weather->update_future_time = weather->request_time + DIVOOM_MAIN_WEATHER_UPDATE_TIME;
            }
            pthread_mutex_unlock(&s_weather_lock);
            if (first_adopted) {
                divoom_print("Weather source: DIVOOM MQTT\n");
            }
            break;
        }
		case DIVOOM_MAIN_EVENT_SET_BLUE_NAME:
		{
			char *name = (char *)msg->data;
			uint8 name_len = 0;

			if (name != NULL)
				name_len = (uint8)strlen(name);
			/* 先规整成本地名（AstroToo / AstroToo-xxx），再同一串下发 JL */
			divoom_main_set_blue_name(name_len, name);
			if (divoom_bt_music_set_jl_adapter_name(divoom_main_get_blue_name()) != 0)
				divoom_print("main: set jl bt name failed\n");
			pDivoomHandle->update_flag = 1;
			divoom_main_save_config_local();
		}
			break;

		case DIVOOM_MAIN_EVENT_GET_BLUE_NAME:
			divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_BLUE_NAME, DIVOOM_APP_TOPIC, divoom_main_get_blue_name());
			break;

		case DIVOOM_MAIN_EVENT_GET_DEVICE_ID:
		{
			int device_id = divoom_main_get_device_id();
			divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_DEVICE_ID, DIVOOM_APP_TOPIC, &device_id);
			break;
		}
		case DIVOOM_MAIN_EVENT_BATTERY:
			{
				uint8_t *bat_info = (uint8_t *)msg->data;
				if(*bat_info == BATTERY_STAT_LOW_SHUTDOWN)
				{
					/* 对标 T113：收到 0x33(1) 立刻关机，不把断电挂在低电提示音回调上 */
					divoom_print("low battery: shutdown now\n");
					divoom_lvgl_top_battery_menu_init(0);
					// divoom_disp_ctrl_set_light(30);
					divoom_onoff_poweroff_no_tone();
				}
				else if(*bat_info == BATTERY_STAT_LOW_WARN)
				{
					//显示低电提醒图案,及音乐
					divoom_main_disp_low_battery();
				}
			}
			break;

            
        case DIVOOM_MAIN_KEY_FUNC_PLAY_OVER:
            divoom_main_disp_key_change_over();
            break;
        
        case DIVOOM_MAIN_NOTICE_OVER_MESSAGE:
        {
            divoom_app_schedule_item_info push_info = {0};
            
            push_info.event_id = *msg->data;
            push_info.model_id = DIVOOM_MODEL_MAIN;
            push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
            divoom_print("mode : %d, %d\n", push_info.event_id, pDivoomHandle->divoom_connect_srv);
            if(pDivoomHandle->new_sd_flag)
            {
                //取消开机动画
                divoom_app_schedule_cancel(&push_info);
                push_info.event_id = DIVOOM_MAIN_PLAY_POWER_ON_EVENT;
                push_info.model_id = DIVOOM_MODEL_MAIN;
                push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
                divoom_app_schedule_cancel(&push_info);
                divoom_main_give_wait_download_msg();
            }
            else if(push_info.event_id == DIVOOM_MAIN_PLAY_POWER_ON_EVENT)
            {
                if(pDivoomHandle->divoom_connect_srv == 0)
                {
                    divoom_main_disp_wifi_connecting(1, 1800);
                }
                else
                {
				    divoom_main_disp_wifi_connect_ok(10);
                }
                divoom_app_schedule_cancel(&push_info);
            }
            else
            {
                divoom_app_schedule_cancel(&push_info);
            }
        }
            break;
        case DIVOOM_MAIN_EVENT_SCHEDULE_CANCEL:
			if(msg->data)
			{
				//没有数据就不是提醒退出,需要到schedule 主动退出当前的schedule
	        	divoom_app_schedule_item_info push_info = {0};
				Pdivoom_disp_event_extend_info info = (Pdivoom_disp_event_extend_info)msg->data;
	        	push_info.event_id = info->event_id;
	        	push_info.model_id = info->model_id;
	            push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
	            divoom_app_schedule_cancel(&push_info);
			}
            break;
        
        case DIVOOM_MAIN_EVENT_POWER_ON_OVER:
        {
        	divoom_app_schedule_item_info push_info = {0};

        	divoom_power_monitor_set_boot_complete();
        	push_info.event_id = DIVOOM_MAIN_PLAY_POWER_ON_EVENT;
        	push_info.model_id = DIVOOM_MODEL_MAIN;
            push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
            divoom_app_schedule_cancel(&push_info);
        }
            break;
            
		case DIVOOM_MAIN_EVENT_APP_UNBIND:
            divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_APP_ID, DIVOOM_SERVER_TOPIC, NULL);
            pDivoomHandle->host_id = 0;
			break;
            
        case DIVOOM_MAIN_EVENT_PUSH_APP_ID:     
            #ifdef DIVOOM_APP_USE_MQTT_CONNECT
            divoom_main_manage_app_con((Pdivoom_device_app_id_list)msg->data);
            #endif
            break;
        case DIVOOM_MAIN_EVENT_UPDATE_APP_ID:
            
#ifdef DIVOOM_APP_USE_MQTT_CONNECT
            divoom_main_manage_update_app_con((Pdivoom_device_app_id_list)msg->data);
            
#endif
            break;
			//主动断开所有APP的连接,是为了在作为从机之后的操作
		case DIVOOM_MAIN_EVENT_DELETE_APP_ID:
#ifdef DIVOOM_APP_USE_MQTT_CONNECT
			{
				divoom_main_manage_dis_all_app_con();
			}
#endif
			break;
            
            
        case DIVOOM_MAIN_EVENT_RESTORE_SYS_LIGHT:
//            divoom_mix_screen_ledseg_set_brightness(pDivoomHandle->sys_setting.brightness);
            pDivoomHandle->sleep_bright_override = 0;
            divoom_main_apply_sys_light();
            break;

        case DIVOOM_MAIN_EVENT_SET_SYS_LIGHT:
            divoom_main_cancel_on_off();
            pDivoomHandle->sys_setting.brightness = *(int *)msg->data;
#ifndef DIVOOM_TIME_FRAME_SUPPORT_LOW_LIGHT
			if(pDivoomHandle->sys_setting.brightness > 0 && pDivoomHandle->sys_setting.brightness < 10)
			{
				pDivoomHandle->sys_setting.brightness = 10;
			}	
#endif
            pDivoomHandle->sleep_bright_override = 0;
            divoom_main_apply_sys_light();
    		pDivoomHandle->update_flag = 1;
            divoom_main_save_config();
            break;

            
        case DIVOOM_MAIN_EVENT_ADD_SYS_LIGHT:
			pDivoomHandle->sys_setting.brightness += 10;
            if(pDivoomHandle->sys_setting.brightness > 100)
            {
                pDivoomHandle->sys_setting.brightness = 100;
            }
            
            pDivoomHandle->sleep_bright_override = 0;
            divoom_main_apply_sys_light();
			break;
			
		case DIVOOM_MAIN_EVENT_DEC_SYS_LIGHT:
#ifdef DIVOOM_TIME_FRAME_SUPPORT_LOW_LIGHT
			if(pDivoomHandle->sys_setting.brightness > DIVOOM_SYS_BRIGHT_ON_FLOOR + 10)
			{
				pDivoomHandle->sys_setting.brightness -= 10;
			}
			else if(pDivoomHandle->sys_setting.brightness > DIVOOM_SYS_BRIGHT_ON_FLOOR)
			{
				pDivoomHandle->sys_setting.brightness = DIVOOM_SYS_BRIGHT_ON_FLOOR;
			}
#else
			if (pDivoomHandle->sys_setting.brightness > 10)
			{
				pDivoomHandle->sys_setting.brightness -= 10;
			}
            else
            {
                pDivoomHandle->sys_setting.brightness = 0;
            }
#endif
            pDivoomHandle->sleep_bright_override = 0;
            divoom_main_apply_sys_light();
			break;

		

        case DIVOOM_MAIN_EVENT_POWER_ON_PRODUCT_TEST_UPDATE:
            /*
        {
            Pdivoom_update_poweron_cfg_info pinfo = (Pdivoom_update_poweron_cfg_info)msg->data;            
			pDivoomHandle->sys_setting.user_lock = pinfo->user_lock;
            strcpy(pDivoomHandle->sys_setting.power_on_text, (char *)pinfo->power_on_text);
            pDivoomHandle->update_flag = 1;
            divoom_main_save_config();
        }
        */
            break;

            
        case DIVOOM_MAIN_EVENT_SET_DEVICE_LOCATION:
        {
            Pdivoom_system_setting_device_location location = (Pdivoom_system_setting_device_location)msg->data;
            
            pDivoomHandle->update_flag = divoom_main_set_device_position(location->Longi, location->Lati);
            divoom_main_save_config();
        }
            break;
            
        case DIVOOM_MAIN_EVENT_POWER_ON_PIC:
        {
            Pdivoom_system_setting_device_poweron_pic pic_info = (Pdivoom_system_setting_device_poweron_pic)msg->data;
            
            if(strcmp(pic_info->pic, pDivoomHandle->sys_setting.power_on_id) && (0 == pDivoomHandle->sys_setting.user_lock))
            {
                pDivoomHandle->update_flag = 1;
                divoom_main_save_notice_dis();
                strcpy(pDivoomHandle->sys_setting.power_on_id, pic_info->pic);                         
				divoom_main_save_config();       
                divoom_http_client_down_file_undecode(DIVOOM_DEIVCE_POWERON_PIC_SAVE_PATH, pic_info->pic, 1, NULL);

            }
        }
            break;


        case DIVOOM_MAIN_EVENT_SET_ALL_MODE:
        {
            uint8 update_flag = 0, pass_save = 0, refresh_ui_flag = 0, lang_flag = 0;
            char tmp_str[20];
            Pdivoom_system_setting_json_system_cfg setting = (Pdivoom_system_setting_json_system_cfg)msg->data;
            
            if(pDivoomHandle->sys_setting.mic_swtich != setting->DisableMic)
            {
                pDivoomHandle->sys_setting.mic_swtich = setting->DisableMic;
                update_flag = 1;
            }
            #if 0
            if(strlen(setting->startup_logo)
				 && (0 ==pDivoomHandle->sys_setting.user_lock))
            {

                if(strcmp(pDivoomHandle->sys_setting.power_on_id, setting->startup_logo))
                {
                    strcpy(pDivoomHandle->sys_setting.power_on_id, setting->startup_logo);
                    pass_save = 1;
                    update_flag = 1;
                    divoom_http_client_down_file_undecode(DIVOOM_DEIVCE_POWERON_PIC_SAVE_PATH, setting->startup_logo, 1, NULL);
                    divoom_main_save_notice_dis();
                }
                else
                {
                    FILE *file_handle;
                    file_handle = divoom_port_file_open(DIVOOM_DEIVCE_POWERON_PIC_SAVE_PATH, "rb");
                    if(file_handle == NULL)
                    {
                        divoom_http_client_down_file_undecode(DIVOOM_DEIVCE_POWERON_PIC_SAVE_PATH, setting->startup_logo, 1, NULL);
                    }
                    else
                    {
                        divoom_port_file_close(file_handle);
                    }
                }
            }                  
            #endif

            if(!pDivoomHandle->device_china_flag
                && pDivoomHandle->sys_setting.lang != setting->Language)
            {
                int first_menu_id = divoom_menu_control_menu_get_first_menu_id();

                /* r43404：开机语言页首次云配置不覆盖本机已选语言。
                 * 重置网络后同样保留：当次 back_lang，或重启后首屏为 OOBE 配网页。 */
                if (pDivoomHandle->first_request_flag == 0
                    && pDivoomHandle->sys_setting.lang >= 0
                    && (first_menu_id == DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_LANGUGE
                        || first_menu_id == DIVOOM_MENU_CONTROL_ID_ASTROTOO_OOBE_CONFIG_WIFI
                        || pDivoomHandle->back_lang >= 0))
                {
                    lang_flag = 1;
                }
                else
                {
                    pDivoomHandle->sys_setting.lang = setting->Language;
                    divoom_menu_control_set_language(pDivoomHandle->sys_setting.lang);
                    divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_DISP_CLOCK_DIAL);
                    update_flag = 1;
                }
            }

            if(pDivoomHandle->sys_setting.hour12_24 != setting->Time24Flag)
            {
                pDivoomHandle->sys_setting.hour12_24 = setting->Time24Flag;
                update_flag = 1;
                refresh_ui_flag = 1;
            }
            if(pDivoomHandle->sys_setting.volume != setting->NotificationSound)
            {
                pDivoomHandle->sys_setting.volume = setting->NotificationSound;
                update_flag = 1;
            }
            if (setting->SoundEffectMode >= 0 && setting->SoundEffectMode < 5
                && pDivoomHandle->sys_setting.audio_effect != (uint8)setting->SoundEffectMode)
            {
                pDivoomHandle->sys_setting.audio_effect = (uint8)setting->SoundEffectMode;
                divoom_btcore_uart_cmd_apply_sound_effect(pDivoomHandle->sys_setting.audio_effect);
                update_flag = 1;
            }
			if(pDivoomHandle->sys_setting.wind_speed_flag != setting->WindSpeed_flag)
            {
                pDivoomHandle->sys_setting.wind_speed_flag = setting->WindSpeed_flag;
                update_flag = 1;
				refresh_ui_flag = 1;
            }
            if(pDivoomHandle->sys_setting.time_format != setting->time_format)
            {
                pDivoomHandle->sys_setting.time_format = setting->time_format;
                update_flag = 1;
                refresh_ui_flag = 1;
            }
            if(setting->ShowTime_flag > 1)
            {
                setting->ShowTime_flag = 0;
            }

            
            if(pDivoomHandle->sys_setting.temp_c_f_flag != setting->TemperatureMode)
            {
                pDivoomHandle->sys_setting.temp_c_f_flag = setting->TemperatureMode;
                update_flag = 1;
                refresh_ui_flag = 1;
            }

            if(pDivoomHandle->sys_setting.screen_dir != setting->GyrateAngle)
            {
                pDivoomHandle->sys_setting.screen_dir = setting->GyrateAngle;
                update_flag = 1;                
                divoom_screen_set_direction(pDivoomHandle->sys_setting.screen_dir);
            }

			if(pDivoomHandle->sys_setting.color_temp != setting->ColorTemp)
			{
				pDivoomHandle->sys_setting.color_temp = setting->ColorTemp;
                update_flag = 1;
				divoom_main_set_color_temp_inner(pDivoomHandle->sys_setting.color_temp);
			}

            divoom_app_com_float_to_str(tmp_str, sizeof(tmp_str), setting->Longitude, 2);
            if(strcmp(tmp_str, pDivoomHandle->sys_setting.lot))
            {
                strcpy(pDivoomHandle->sys_setting.lot, tmp_str);
                update_flag = 1;
                pDivoomHandle->read_weather_time = 0;
                pDivoomHandle->weather_info.update_real_time = 0;
                pDivoomHandle->weather_info.update_future_time = 0;
                pass_save = 1;
            }
            
            divoom_app_com_float_to_str(tmp_str, sizeof(tmp_str), setting->Latitude, 2);
            if(strcmp(tmp_str, pDivoomHandle->sys_setting.lat))
            {
                strcpy(pDivoomHandle->sys_setting.lat, tmp_str);
                update_flag = 1;
                pDivoomHandle->read_weather_time = 0;
                pDivoomHandle->weather_info.update_real_time = 0;
                pDivoomHandle->weather_info.update_future_time = 0;
                pass_save = 1;
            }

            
            if(strcmp(setting->TimeZoneValue, pDivoomHandle->sys_setting.tzcode))
            {
                strcpy(pDivoomHandle->sys_setting.tzcode, setting->TimeZoneValue);
                divoom_main_load_timezone_config();                
                divoom_sys_model_crl_send_asyn_all_model(DIVOOM_DEVICE_EVENT_TIME_INIT_OK);
                update_flag = 1;
                refresh_ui_flag = 1;
            }
            
            
            if(setting->WhiteBalanceR < 0)
            {
                setting->WhiteBalanceR = 100;
            }
            if(setting->WhiteBalanceG < 0)
            {
                setting->WhiteBalanceG = 100;
            }
            if(setting->WhiteBalanceB < 0)
            {
                setting->WhiteBalanceB = 100;
            }
            if(setting->Brightness != -1)
            {
                if(setting->Brightness != pDivoomHandle->sys_setting.brightness)
                {
                    pDivoomHandle->sys_setting.brightness = setting->Brightness;
                    pDivoomHandle->update_flag = 1;
                    pDivoomHandle->sleep_bright_override = 0;
                    divoom_main_apply_sys_light();
                    pass_save = 1;
                }
            }
            
            divoom_screen_set_power_mode(setting->PowerMode);
            divoom_screen_update_write_brance(setting->WhiteBalanceR, setting->WhiteBalanceG, setting->WhiteBalanceB);
            
            if(pDivoomHandle->sys_setting.auto_update != setting->AutoUpdateFlag)
            {
                pDivoomHandle->sys_setting.auto_update = setting->AutoUpdateFlag;
                pass_save = 1;
                update_flag = 1;
            }
            
            if(pDivoomHandle->sys_setting.mirror_flag != setting->MirrorFlag)
            {
                pDivoomHandle->sys_setting.mirror_flag = setting->MirrorFlag;
                divoom_screen_set_mirror_image(pDivoomHandle->sys_setting.mirror_flag);
                update_flag = 1;
            }

			if(pDivoomHandle->sys_setting.show_grid_flag != setting->ShowGridflag)
			{
				pDivoomHandle->sys_setting.show_grid_flag = setting->ShowGridflag;
				update_flag = 1;
			}
            
            if(pDivoomHandle->first_request_flag == 0)
            {
                int update_time;
                
                update_time = divoom_eq_image_manage_get_update_time();
                divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_GET_EQ_LIST, DIVOOM_SERVER_TOPIC,&update_time);
                divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_GOOGLE_CALENDAR_CONFIG, DIVOOM_SERVER_TOPIC,NULL);
                divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_OUTLOOK_CALENDAR_CONFIG, DIVOOM_SERVER_TOPIC,NULL);
                update_time = divoom_system_font_get_update_time();
                divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_TIME_DIAL_FONT_LIST, DIVOOM_SERVER_TOPIC,&update_time);
                if(pDivoomHandle->device_china_flag)
                {
                    divoom_net_app_send_packet(DIVOOM_NET_COMM_APP_NOT_SUPPORT_SELECT_LANUAGE, DIVOOM_SERVER_TOPIC, &pDivoomHandle->device_china_flag);
                    divoom_net_app_send_packet(DIVOOM_NET_COMM_APP_GET_NO_BAN_FLAG, DIVOOM_SERVER_TOPIC,NULL);
                }
                divoom_main_upoad_err_file();
				
            }
            #if DIVOOM_BUILD_RTOS_DEVICE
            divoom_main_retry_boot_ab_recovery();
            /* Every settings sync can retry a resource download cancelled by firmware OTA. */
            if (divoom_update_system_file_begin_settings_check())
            {
                int update_time = divoom_update_system_file_get_file_version();
                divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_DEVICE_UPDATE_FILE_LIST, DIVOOM_SERVER_TOPIC, &update_time);
            }
            #endif
            pDivoomHandle->divoom_srv_respone = 1;
            
            if(update_flag)
            {
                pDivoomHandle->update_flag = 1;
                divoom_main_save_config_local();    //这里马上保存
                if(pDivoomHandle->first_request_flag && pass_save == 0)
                {
                    divoom_main_save_notice_dis();
                }
            }
            pDivoomHandle->first_request_flag = 1;
            pDivoomHandle->divoom_first_connect = 0;
            divoom_device_error_report_all();
            if(refresh_ui_flag)
            {
                divoom_disp_clock_main_refresh_all();
            }
            if (lang_flag)
            {
                /* 设备语言与云端不一致，以设备为准并回写 */
                divoom_main_send_sys_settings_config();
            }
            //divoom_main_test_url_info();
            //divoom_main_test_url_info();
            //divoom_main_test_url_info();
        }
            break;

		case DIVOOM_MAIN_SET_POWERON_MODE:
			{
				//暂时不考虑读取这个模式命令,后面有需要的话再加:收到包之后spi发送给gd,gd返回在包发出去
				//通过spi发送当前开机模式到gd
				//divoom_screen_handle_send(DIVOOM_SCREEN_CMD_FOR_CMD, (void *)msg->data, 1);
				//divoom_disp_ctrl_app_send_cmd(DIVOOM_SCREEN_CMD_FOR_CMD, (void *)msg->data, 1);
			}
			break;
            
        case DIVOOM_APP_CONFIG_SAVE_EVENT_ID:
			//增加关机前发送notice给服务器
			//divoom_main_mqtt_notice_all(pDivoomHandle->app_list, 0);
            //divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_DISCONNECT, DIVOOM_SERVER_TOPIC, NULL);
            divoom_main_save_config_local();
            //divoom_wifi_model_deinit();
			//divoom_main_exit();
            //divoom_image_download_manage_save();
            break;

        case DIVOOM_DEVICE_WIFI_CON_ID:
            //divoom_main_disp_power_over_notice(DIVOOM_MAIN_PLAY_ALL_NOTICE_EVENT);            
            if(pDivoomHandle->divoom_connect_srv == 0 && pDivoomHandle->divoom_first_connect)
            {
                int status;
                uint8 mac[6], ip_addr[64];
                divoom_devcice_con_json device_info = {0};
				divoom_bond_user_para bond_info = {0};
                divoom_main_get_device_mac_addr_ifconfig(mac, ip_addr);
                device_info.custom_id = DIVOOM_CUSTOM_ID;
                device_info.soft_version = DIVOOM_SOFTWARE_VERSION;
#ifdef DIVOOM_TIMES_VIBE
				{
					uint32_t jl_ver = divoom_ota_jl_get_local_version();

					/* 杰理未上报有效版本时不上抛 Software2，避免服务器记到 0/乱码 */
					device_info.soft_version2 =
						divoom_ota_jl_version_is_valid(jl_ver) ? (int)jl_ver : 0;
				}
#else
				device_info.soft_version2 = 0;
#endif
                device_info.hd_version = DIVOOM_HARDWARE_VERSION;
                sprintf(device_info.mac_addr, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                sprintf(device_info.dev_ip,"%d.%d.%d.%d", ip_addr[0], ip_addr[1],ip_addr[2],ip_addr[3]);
                //strcpy(device_info.dev_ip, wifi_info.ip_addr);
                //strcpy(device_info.ssid, "Divoom-3g");
#ifdef DIVOOM_SIMULATOR
                strcpy(device_info.ssid, DIVOOM_PRODUCT_DEBUG_WIFI_SSID);
#else
                strcpy(device_info.ssid, pDivoomHandle->ssid);
#endif
                strcpy(device_info.net_ip, pDivoomHandle->net_ip);

				divoom_net_image_cache_update_net_status(1);
                divoom_manage_image_up_netstatus(1);
                pDivoomHandle->divoom_connect_srv = 1;
                pDivoomHandle->first_request_flag = 0;
                pDivoomHandle->divoom_srv_respone = 0;
                pDivoomHandle->connect_serve_time = time(0);
				if(pDivoomHandle->app_id)
    			{
                    pDivoomHandle->sys_setting.wifi_config_flag = 1;
					bond_info.app_id = pDivoomHandle->app_id;
					bond_info.lang = pDivoomHandle->sys_setting.lang;
    				//divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_BOND, DIVOOM_SERVER_TOPIC, &pDivoomHandle->app_id);
					divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_BOND, DIVOOM_SERVER_TOPIC, (void*)&bond_info);
					//divoom_power_on_manage_erase_app_id();
    			}
                else
                {
                    //divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_APP_ID, DIVOOM_SERVER_TOPIC,NULL);
                }
                if(pDivoomHandle->sys_setting.reset_flag)
                {                    
                    divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_RESET_SYSTEM, DIVOOM_SERVER_TOPIC, &pDivoomHandle->sys_setting.lang);
                    pDivoomHandle->sys_setting.reset_flag = 0;
                    divoom_main_save_config_local();
                }
                if(pDivoomHandle->reset_flag)
                {
                    Pdivoom_net_command pcmd_info;
                    
                    pDivoomHandle->reset_flag = 0;
                    pcmd_info = divoom_net_app_send_packet_wait(DIVOOM_NET_COMM_DEVICE_CLEAR_RESET_FLAG, DIVOOM_SERVER_TOPIC,NULL);
                    divoom_net_app_free_packet(pcmd_info);
                    
                    divoom_sys_model_crl_send_all_model(DIVOOM_DEVICE_SYSTEM_RESET_ALL);            
                }
                divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_SHARE_CODE,DIVOOM_SERVER_TOPIC, NULL);
				
                divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_CON_SRV, DIVOOM_SERVER_TOPIC,(void *)&device_info);
                divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_SYS_CONF, DIVOOM_SERVER_TOPIC,&pDivoomHandle->app_id);
                divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_SYS_NIGHT_VIEW, DIVOOM_SERVER_TOPIC,NULL);
				divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_POWER_ON_LIST, DIVOOM_SERVER_TOPIC,NULL);
				//divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_DEBUG_FALG, DIVOOM_SERVER_TOPIC,NULL);
                status = divoom_light_app_get_screen_status(0xFF);
                divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_SCREEN_CTRL, DIVOOM_SERVER_TOPIC, &status);
                divoom_net_app_send_packet(DIVOOM_NET_COMM_PHOTO_GET_ALBUM_LIST, DIVOOM_SERVER_TOPIC, NULL);
                divoom_net_app_send_packet(DIVOOM_NET_COMM_PHOTO_GET_ALBUM_CONFIG, DIVOOM_SERVER_TOPIC, NULL);
                divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_ALI_PHOTO_LIST, DIVOOM_SERVER_TOPIC,NULL);
				divoom_light_app_get_my_clock_list();
				divoom_light_app_get_my_clock_recent_list();
				divoom_light_app_get_my_clock_favorite_list();
				divoom_tomato_time_get_tomato_list();
				divoom_exchange_pic_get_all_list();
				divoom_nba_pic_get_all_list();
				divoom_store_clock_prefetch_on_server_connected();

                if(pDivoomHandle->new_sd_flag)
                {
                    divoom_main_give_wait_download_msg();
                }
                else
                {
				    divoom_main_disp_wifi_connect_ok(10);
                }
                //重新加载自定义
                divoom_user_define_ctrl_reload();
                divoom_sys_model_crl_send_asyn_all_model(DIVOOM_DEVICE_WIFI_CON_SRV);
                pDivoomHandle->first_request_flag = 0;
                pDivoomHandle->send_server_heart_time = time(0);
            }
            else
            {
				divoom_net_image_cache_update_net_status(1);
                divoom_manage_image_up_netstatus(1);
                pDivoomHandle->divoom_connect_srv = 1;
                pDivoomHandle->first_request_flag = 1;
                pDivoomHandle->divoom_srv_respone = 1;
                pDivoomHandle->connect_serve_time = time(0);
                divoom_main_disp_cacnel_wifi_notice();
            }
            divoom_main_report_beta_features();
			/* 复用下载前的表盘空间清理入口，完成开机首次存储状态上报。 */
			divoom_print("[storage_gc] call source=%s line=%d\n", __func__, __LINE__);
			divoom_clock_manage_clear_removed_clock_data_if_needed();
			divoom_main_send_lwt_msg(1);
            pDivoomHandle->device_wifi_status = 1;
            divoom_net_app_send_packet_http(DIVOOM_NET_COMM_DEVICE_HTTP_UPDATE_IP, NULL, 0);
            break;

        case DIVOOM_MAIN_EVENT_WIFI_CON:
			{
				divoom_print("[WIFI_CFG] MAIN_EVENT_WIFI_CON (GOT_IP), start MQTT\n");
				//需要记录一下当前WiFi的连接状态,在没有连上WiFi之前,不响应按键事件
				//ssid至少有两个字节
				#ifdef DIVOOM_USR_RK_IC
                RK_WIFI_INFO_Connection_s wif_conf_info;
                
                RK_wifi_running_getConnectionInfo(&wif_conf_info);
                #else     
                
                #endif
  
                pDivoomHandle->pass_wifi_discon_notice = 0;
                divoom_wifi_get_connect_ssid_info(pDivoomHandle->ssid);
                if(msg->data)
                {
                    pDivoomHandle->app_id = *(int *)msg->data;
                    pDivoomHandle->bond_flag = 1;
                }
				divoom_print("pDivoomHandle->ssid:%s---pDivoomHandle->app_id = %d\n", pDivoomHandle->ssid, pDivoomHandle->app_id);
				//这是连上WiFi后的第一入口,先获取服务器的IP地址
					
				pDivoomHandle->device_wifi_status = 1;
#if DIVOOM_SHOW_MODE
				divoom_main_show_seed_store_power_schedule(0);
				divoom_show_tf_music_fetch_if_needed();
#endif
#if defined(DIVOOM_SIMULATOR) && !SIM_THREAD_REAL_DEVICE
				s_sim_pending_wifi_mqtt_connect = 1;
#else
				uint8 ret = divoom_main_manage_connect_service_mqtt();
				if (ret)
				{
					pDivoomHandle->http_request_ok = 1;
				}
				else
				{
					pDivoomHandle->http_request_ok = 0;
				}
				pDivoomHandle->http_request_frist = 1;
#endif
				//mqtt连接并且订阅成功之后再进行wifi通知
				//divoom_sys_model_crl_send_asyn_all_model(DIVOOM_DEVICE_WIFI_CON_ID);
			}
            break;

		case DIVOOM_MAIN_EVENT_RETRY_MQTT_CONNECT:
		{
			uint8 ret;

			divoom_print("[WIFI_CFG] retry MQTT connect on sys_model thread\n");
			ret = divoom_main_manage_connect_service_mqtt();
			if (ret)
				pDivoomHandle->http_request_ok = 1;
			else
				pDivoomHandle->http_request_ok = 0;
			break;
		}

		case DIVOOM_MAIN_EVENT_SRV_MQTT_CON:
            if(pDivoomHandle->divoom_connect_srv == 0)
            {
                #if 0
                model_msg.src_id = DIVOOM_MODEL_MAIN;
                model_msg.dest_id = DIVOOM_MODEL_MAIN;  
                model_msg.msg_id = DIVOOM_DEVICE_WIFI_CON_ID;  
                model_msg.msg_type = 1;  
                divoom_sys_model_crl_msg_push(&model_msg);
                #endif
            }
            break;

        case DIVOOM_MAIN_EVENT_SRV_MQTT_DISCON:
            pDivoomHandle->beta_test_flag_packet = 0;
            if(pDivoomHandle->divoom_connect_srv)
            {
                pDivoomHandle->divoom_connect_srv = 0;
                divoom_net_image_cache_update_net_status(0);
                divoom_manage_image_up_netstatus(0);
            }
            divoom_store_clock_prefetch_on_net_lost();
            break;
            
        case DIVOOM_MAIN_EVENT_WIFI_DISCON:
            divoom_main_set_lyric_country(NULL);
            if(pDivoomHandle->pass_wifi_discon_notice == 0)
            {
                pDivoomHandle->divoom_connect_srv = 0;
                pDivoomHandle->divoom_srv_respone = 0;
                pDivoomHandle->first_request_flag = 0;
    			if(pDivoomHandle->device_wifi_status == 1)
    			{
    				pDivoomHandle->device_wifi_status = 0;
    				divoom_sys_model_crl_send_asyn_all_model(DIVOOM_DEVICE_WIFI_DISCON_ID);
    				divoom_main_disp_wifi_dis_con();
    			}
                divoom_net_image_cache_update_net_status(0);
                divoom_manage_image_up_netstatus(0);
            }
            divoom_store_clock_prefetch_on_net_lost();
            break;
        
//VJ需要一个关机,先发关屏命令,再发送关机命令(或者告诉GD是关机状态,伪关机命令)
		case DIVOOM_MAIN_EVENT_KEY_POWEROFF:
		{
            divoom_system_delay_ms(200);
#ifdef DIVOOM_TIME_FRAME_SPEAKER
			/* 执行实际断电 */
			divoom_onoff_request_shutdown();
#endif
			break;
		}
        
		case DIVOOM_MAIN_EVENT_SOFT_RESTART:
			{
			//需要重启，只需要保存配置就可以了
				/*uint8 *cur_mode = (uint8_t *)msg->data;
				if(cur_mode)
				{
					if(*cur_mode == DIVOOM_APP_ONOFF_BT_MODE)
						divoom_bt_clear_mix_flag();
				}*/
//                divoom_screen_close();
		        divoom_sys_model_crl_save_config();
				break;
			}
            
			
		case DIVOOM_MAIN_EVENT_STOP_CON_APP:
			{
				//向servermanage模块发送DIVOOM_SERVER_MSG_DEL_SERVER断开连接
				Pdivoom_server_msg_destory_struct info = (Pdivoom_server_msg_destory_struct)msg->data;
				divoom_sys_msg_info model_msg = {0};

				model_msg.src_id = DIVOOM_MODEL_MAIN;
				model_msg.dest_id = DIVOOM_MODEL_SER_CON;
				model_msg.msg_id = DIVOOM_SERVER_MSG_DEL_SERVER;
				model_msg.data = (void *)info;
				divoom_sys_model_crl_msg_push(&model_msg);
			}
			break;

		case DIVOOM_MAIN_EVENT_APP_MQTT_CON:
			{
				divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_CON_APP, DIVOOM_APP_TOPIC, NULL);
			}
			break;		
		/*
		//事件保留,但是可能没有需要在BLE退出之后再初始化的了
		case DIVOOM_MAIN_EVENT_EXIT_BLE:
			{
			}
			break;*/
		case DIVOOM_MAIN_EVENT_KEY_PRESS_EVENT:
			//收到按键,发送DIVOOM_APP_KEY_EVENT_ID事件,发给当前激活的模块
			//新增没有连接上WiFi,不处理按键切换,这样就没有可以打断闪WiFi图标的情况了
			{
				Pdivoom_key_info key_info = (Pdivoom_key_info)msg->data;
				divoom_print("key_info->key_value:0x%x, key_info->press_type = %d\n", key_info->value, key_info->type);
				if(key_info->value == kbPOWER)
				{	
					//开机5秒内不处理		
					if(divoom_system_get_tickms() < 5000)
					{
						break;
					}
					if(key_info->type == PRESS_SHORT)
					{

					}
					else if(key_info->type == PRESS_LONG)
					{
						divoom_app_schedule_item_info event_info = {0};
//						divoom_mix_screen_poweroff_clear();
						event_info.model_id = DIVOOM_MODEL_MAIN;
						event_info.event_id = DIVOOM_MAIN_PLAY_POWER_OFF_EVENT;
						event_info.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
						event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;			
						if(divoom_app_schedule_push(&event_info))
						{
							pDivoomHandle->power_onoff_flag[0] = 0xaa;
//							divoom_main_play_voice_tip(DIVOOM_MAIN_PLAY_POWER_OFF_EVENT, divoom_main_poweroff_play_over_voice);
							divoom_disp_ctrl_set_mode(DIVOOM_DISP_NO_DISP_MODE, NULL);
							//保存配置
							divoom_sys_model_crl_send_all_model(DIVOOM_APP_CONFIG_SAVE_EVENT_ID);

							divoom_sys_msg_info model_msg = {0};
							model_msg.msg_type = 1;
							model_msg.msg_id = DIVOOM_MAIN_EVENT_KEY_POWEROFF;
							model_msg.src_id = DIVOOM_MODEL_MAIN;
							model_msg.dest_id = DIVOOM_MODEL_MAIN;
							divoom_sys_model_crl_msg_push(&model_msg);

							divoom_print("Here Is Poweroff And Down Wifi & Ble!\n");
						}
						else
						{
							divoom_print("cancel poweroff!\n");
						}
						break;
					}				
					else
					{
						break;
					}
				}
#ifdef DIVOOM_TIME_FRAME_SPEAKER
				/* RTC 唤醒黑屏：任意键（短按/长按/连按等）先脱黑屏；已成功则吞掉本键，避免与后续菜单逻辑叠加 */
				if (pDivoomHandle && pDivoomHandle->poweron_mode == DIVOOM_ONOFF_MODE_CLOCK)
				{
					if (divoom_main_try_dismiss_clock_black_screen())
						break;
				}
#endif
                if(divoom_main_do_with_other_key_pre(key_info->value, key_info->type))
                {
                    break;
                }
#ifdef DIVOOM_PRODUCT_ASTRO_TOO
                /* APP AidSleep 试听：除 VOL± 外任意键退出（VOL 走下方 bright_volume sleep 音量） */
                if (divoom_sleep_aid_ctrl_app_preview_is_active()
                    && key_info->value != DIVOOM_KEY_LIGHT_LEFT
                    && key_info->value != DIVOOM_KEY_LIGHT_RIGHT)
                {
                    divoom_sleep_aid_ctrl_app_preview_stop_by_user_key();
                    break;
                }
                /* VOL± 是 LIGHT_LEFT/RIGHT，与游戏方向键 LEFT/RIGHT 不是同一组实体键。
                 * 局内也走全局音量 OSD；方向键继续交给 Lua。 */
                if ((key_info->value == DIVOOM_KEY_LIGHT_LEFT || key_info->value == DIVOOM_KEY_LIGHT_RIGHT)
                    && divoom_lvgl_top_bright_volume_menu_handle_key(key_info->value, key_info->type))
                {
                    break;
                }
#endif
				if(key_info->value == DIVOOM_CONFIG_BOND_KEY && key_info->type == PRESS_LONG_LONG)
				{
					model_msg.msg_type = 1;
                    pDivoomHandle->divoom_connect_srv = 0;
					model_msg.msg_id = DIVOOM_POWER_ON_WIFI_CONFIG_MODE;
					model_msg.dest_id = DIVOOM_MODEL_POWER_ON;
					divoom_sys_model_crl_msg_push(&model_msg);
                    
					break;
				}
#if 1
				if (key_info->value == DIVOOM_WORK_MODE_SWITCH && key_info->type == PRESS_LONG_LONG)
				{
                    divoom_main_disp_sd_err_type(1);
                    divoom_print("we will start format sd!\n");
					break;
				}
#endif			
				if(key_info->value == DIVOOM_LVGL_KEY_HOME && key_info->type == DIVOOM_PRESS_LONG_LONG)
				{
                    char file_name[128];
                    
                    divoom_voice_read_get_filename(DIVOOM_VOICE_ID_TIME_MANAGE, file_name);
                    divoom_local_music_ctrl_tone_play(file_name, 50, NULL, 0);
                    
					divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_CONFIG_WIFI, NULL, 0, 0);
					break;
				}

				if((key_info->value == DIVOOM_LVGL_KEY_HOME || key_info->value == DIVOOM_LVGL_KEY_ENTER)
					&& (key_info->type == DIVOOM_PRESS_SHORT || key_info->type == DIVOOM_PRESSING))
				{
#ifdef DIVOOM_PRODUCT_ASTRO_TOO
					if (divoom_menu_control_get_active_menu_id() == DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID)
					{
						/* main 消息线程无 lvgl 锁，走 event 队列由 menu 线程切屏 */
						divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_MAIN, NULL, 0, 0);
					}
#else
					if (key_info->value == DIVOOM_LVGL_KEY_HOME)
					{
						/* 顶层主菜单（Times 等 legacy 产品） */
						divoom_lvgl_top_main_menu_init();
					}
#endif
				}

				if(key_info->value == DIVOOM_KEY_LIGHT_ENTER && (key_info->type == DIVOOM_PRESS_SHORT || key_info->type == DIVOOM_PRESSING))
				{
					#ifdef DIVOOM_TIME_FRAME_SPEAKER
						divoom_lvgl_top_bright_volume_menu_init(0);
					#else
						#ifdef DIVOOM_TIME_FRAME_NO_RGB_LIGHT
						if(divoom_light_app_get_screen_status(0xFF))
    					{
        					//divoom_screen_bright(pDivoomHandle->sys_setting.brightness);
							divoom_app_schedule_item_info push_info = {0};

    						push_info.event_id = DIVOOM_MAIN_PLAY_ENTER_POWER_OFF_DISP_MENU_EVENT;
    						push_info.model_id = DIVOOM_MODEL_MAIN;
        					push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
    						if(divoom_app_schedule_push(&push_info))
    						{	
            					divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_POWER_OFF_DISP_MENU, NULL, DIVOOM_MODEL_MAIN, DIVOOM_MAIN_PLAY_ENTER_POWER_OFF_DISP_MENU_EVENT);
    						}
    					}
    					else
    					{
							divoom_sys_msg_info model_msg= {0};

    						model_msg.msg_type = 1;
							model_msg.src_id = DIVOOM_MODEL_MAIN; 
							model_msg.dest_id = DIVOOM_MODEL_MAIN; 
							model_msg.msg_id = DIVOOM_MAIN_EVENT_POWER_OFF_DISP_MENU_CANCLE;
							divoom_sys_model_crl_msg_push(&model_msg);
    					}
						#else
							divoom_lvgl_top_ambient_menu_init();
						#endif
					#endif
				}

				if(key_info->value == DIVOOM_LVGL_KEY_ESC && (key_info->type == DIVOOM_PRESS_SHORT || key_info->type == DIVOOM_PRESSING))
				{
					if(pDivoomHandle->sys_setting.dial_demo_flag)
					{
						if(divoom_menu_control_get_active_menu_id() == DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID)
						{
							divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP ,DIVOOM_LIGHT_APP_EVENT_DISP_DEMO_DIAL);
						}
						else if(divoom_menu_control_get_active_menu_id() == DIVOOM_MENU_CONTROL_ID_DEMO_DIAL_MENU)
						{
							divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_DISP_CLOCK_DIAL);
						}
					}
					divoom_lvgl_top_reached_home_menu_init();
				}
#ifdef DIVOOM_TIME_FRAME_SPEAKER
				/* 助眠白噪音后台播放时，播放/暂停键优先停止助眠，避免误切蓝牙音乐 */
				if (divoom_white_noise_player_try_consume_play_key(key_info->value, key_info->type))
				{
					break;
				}
				// 先交给音乐模块处理（音量/播放/上下曲等），若已处理则不再下发调度
				if (divoom_music_ctrl_schedule_key_callback(key_info->value, key_info->type, DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE))
				{
					break;
				}
#endif
				//调用各个模块进行按键分发
				if(divoom_app_schedule_key_handle_event(key_info->value, key_info->type) == 0)
				{
					//通用处理,激活的模块都没有进行处理,将要做组合键处理
					//divoom_main_key_handle((Pdivoom_key_info)msg->data);
					//divoom_print("we will pass the key : %d, %d!\n",key_info->value, key_info->type);
	                //将处理按键不同状态
	                divoom_main_do_with_other_key(key_info->value, key_info->type);
				}
			}
			break;
            
		case DIVOOM_MAIN_STOP_ALL_SERVER_CON:
			{
				//发送给服务器,断开所有的APP了
				divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_DISCONNECT, DIVOOM_SERVER_TOPIC, NULL);
				//主动断开服务器,APP,释放资源
				divoom_net_mqtt_manage_exit();
				divoom_net_http_manage_deinit();
			}
			break;
        case DIVOOM_MAIN_SET_SD_ERR:
        case DIVOOM_MAIN_SET_SD_UNMOUNT:
			divoom_sys_model_crl_send_asyn_all_model(DIVOOM_APP_SD_STATUS_OUT_ID);
            break;
        
		case DIVOOM_MAIN_SET_SD_MOUNT_OK_DO:
			{
				//这里是异步消息,所以延时5ms,不然有时候会readdir失败
				divoom_system_delay_ms(50);
				divoom_sys_model_crl_send_asyn_all_model(DIVOOM_APP_SD_STATUS_IN_ID);
			}
			break;

		case DIVOOM_MAIN_EVENT_SET_RGB_INFO:
		{
			Pdivoom_channel_set_rgb_info rgb_info = (Pdivoom_channel_set_rgb_info)msg->data;
            int *status = divoom_system_malloc(sizeof(int));
            
            *status = 1;
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_EVENT_POWER_OFF_DISP_MENU_CANCLE;
			divoom_sys_model_crl_msg_push(&model_msg);
			
			divoom_main_cancel_on_off();
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SCREEN_CTRL;
            model_msg.msg_type = 1;
            model_msg.data = (uint8 *)status;
			divoom_sys_model_crl_msg_push(&model_msg);
#ifdef DIVOOM_PRODUCT_ASTRO_TOO
			divoom_main_key_light_apply_from_app_rgb(rgb_info);
#else
			divoom_disp_ambient_ctrl_set_disp_info(rgb_info, 1);
#endif
			break;
		}

		case DIVOOM_MAIN_EVENT_SET_RGB_INIT_INFO:
		{
			Pdivoom_channel_set_rgb_info rgb_info = (Pdivoom_channel_set_rgb_info)msg->data;
#ifdef DIVOOM_PRODUCT_ASTRO_TOO
			divoom_main_key_light_apply_from_app_rgb(rgb_info);
#else
			divoom_disp_ambient_ctrl_set_disp_info(rgb_info, /*divoom_light_app_get_screen_status(0xFF)*/divoom_light_app_get_screen_status(0xFF));
#endif
			break;
		}
		
		case DIVOOM_MAIN_EVENT_GET_RGB_INFO:
		{
#ifdef DIVOOM_PRODUCT_ASTRO_TOO
			divoom_channel_set_rgb_info rgb_info;

			divoom_main_key_light_fill_app_rgb(&rgb_info);
			divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_GET_RGB_INFO, DIVOOM_APP_TOPIC, (void *)&rgb_info);
#else
			Pdivoom_channel_set_rgb_info rgb_info = divoom_disp_ambient_ctrl_get_disp_info();

			divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_GET_RGB_INFO, DIVOOM_APP_TOPIC, (void *)rgb_info);
#endif
			break;
		}
    }
    if(pDivoomHandle->update_flag)
    {
        divoom_main_save_config();
    }
    return 1;
}

void divoom_main_manage_ble_mix_music_cb(uint8_t cmd, void *data)
{
    divoom_sys_msg_info model_msg = {0};    
    // divoom_print("%s\n",__func__);
	switch(cmd)
	{
		case DIVOOM_BLE_GET_MIX_MUSIC:
			{
                uint8_t *cmd_data = (uint8_t *)data;
                divoom_print("BLE_MIXER:%d,%d,%d\n",cmd_data[0],cmd_data[1],cmd_data[2]);
                model_msg.msg_type = 0;
                model_msg.src_id = DIVOOM_MODEL_NET_MUSIC;
                model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;
                model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_MIXER_SET;
                model_msg.data = data;
                divoom_sys_model_crl_msg_push(&model_msg);
                if(cmd_data[0] == 0 && cmd_data[2] == 0)
                {   
                    /* 旧协议退出时仍通知 MAIN 停止混音音频，显示状态已在上方同步退出。 */
                    model_msg.dest_id = DIVOOM_MODEL_MAIN;
                    model_msg.msg_id = DIVOOM_MAIN_EVENT_END_MUSIC_MIXER;
                    divoom_sys_model_crl_msg_push(&model_msg);
                }
			}
			break;
        case DIVOOM_BLE_GET_MUSICAL_INSTRUMENTS:
            {
                uint8 *cmd_data = (uint8 *)data;
                divoom_print("MUSICAL_INSTRUMENTS:%d,%d\n",cmd_data[0],cmd_data[1]);
                model_msg.msg_type = 0;
                model_msg.src_id = DIVOOM_MODEL_NET_MUSIC;
                model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;
                model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_MIXER_SET_NEW;
                model_msg.data = data;
                divoom_sys_model_crl_msg_push(&model_msg);                
            }
            break;            
		default:
			break;
	}
}

//基本上以后所有的模块都可以这样操作
//被打断激活的时候,只需要设置 DIVOOM_LIGHT_APP_EVENT_SMALL_DRAW 就可以了, 16*48 和 voice会自动找到第一个可以播放的,如果没有就不播放
uint8 divoom_main_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type)
{
	divoom_print("[%s:%d]", __func__, __LINE__);
	if(event_id == DIVOOM_MAIN_PLAY_DOWNLOAD_OK_EVENT)
	{
        if(pDivoomHandle->new_sd_flag == 1)
        {
            divoom_disp_ctrl_set_mode(DIVOOM_DISP_DOWNLOAD_FILE_MODE, NULL);
            pDivoomHandle->new_sd_flag = 2;
        }
		return true;
	}
    else if (event_id == DIVOOM_MAIN_PLAY_WIFI_NOTICE_EVENT)
    {
        if(DIVOOM_POWER_ON_WIFI_CONFIG_MODE == divoom_power_on_manage_get_power_mode())
        {
            divoom_main_disp_wifi_config(0, 10000);
        }
        else
        {
            divoom_disp_event_disp_info notice_info = {0};

            notice_info.disp_id[0] = DIVOOM_DISP_WIFI_CONNECTING_00;
            notice_info.disp_id[1] = DIVOOM_DISP_WIFI_CONNECTING_01;
            notice_info.disp_id[2] = DIVOOM_DISP_WIFI_CONNECTING_02;
            notice_info.disp_id[3] = DIVOOM_DISP_WIFI_CONNECTING_03;
            notice_info.disp_id[4] = DIVOOM_DISP_WIFI_CONNECTING_04;
        	notice_info.on_time = 1;
        	notice_info.period_time = 1;
        	notice_info.tot_pic = 5;
        	notice_info.tot_time = 1800;
            notice_info.noice_id = DIVOOM_MAIN_PLAY_WIFI_NOTICE_EVENT;
            notice_info.pover_callback = divoom_main_disp_power_over_notice;
            notice_info.disp_title = divoom_main_get_disp_title(DIVOOM_NOTICE_LANUAGE_CONNECT_WIFI_MSG);
            
            divoom_disp_ctrl_set_mode(DIVOOM_DISP_EVENT_DIS_CTRL, (void *)&notice_info);
        }
    }
	else if(event_id == DIVOOM_MAIN_PLAY_ENTER_POWER_OFF_DISP_MENU_EVENT)
	{
		divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_POWER_OFF_DISP_MENU, NULL, DIVOOM_MODEL_MAIN, DIVOOM_MAIN_PLAY_ENTER_POWER_OFF_DISP_MENU_EVENT);
	}
	
	return true;
}

//不同事件可以被打断,被打断不做任何操作
uint8 divoom_main_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type)
{
	divoom_print("[%s:%d]", __func__, __LINE__);
    //divoom_voice_info_stop_all();
	if(event_id == DIVOOM_MAIN_PLAY_POWER_ON_EVENT)
	{
		return false;
	}
	else if(event_id == DIVOOM_MAIN_PLAY_DEFAULT_TIME_EVENT)
	{
		return true;
	}
	else if(event_id == DIVOOM_MAIN_PLAY_ENTER_SYSTEM_MENU_SETTING)
	{
		return false;
	}
	else if(event_id == DIVOOM_MAIN_PLAY_ENTER_POWER_OFF_DISP_MENU_EVENT)
	{
		return true;
	}
	return false;
}

//需要把所有的资源全部释放了,16*16的不需要,会被别的模式直接退出
uint8 divoom_main_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type)
{
	divoom_print("event_id %d\n", event_id);
	return true;
}

uint8 divoom_main_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type)
{
	//TODO
	uint8 event_id;

    divoom_app_schedule_get_active_model(&event_id, DIVOOM_APP_SCHEDULE_RESOURCCE_RGB);
    divoom_print("event id: %d, key_id: %d\n", event_id, key_id);
    if(event_id == DIVOOM_MAIN_PLAY_WIFI_NOTICE_EVENT)
    {
        pDivoomHandle->pass_wifi_discon_notice = 1;
        divoom_main_disp_cacnel_wifi_notice();
        return 1;
    }
    else if(event_id == DIVOOM_MAIN_PLAY_SD_FREE_WARNING)
    {
        divoom_main_disp_cacnel_sd_free_warning();
        return 1;
    }
    else if(event_id == DIVOOM_MAIN_PLAY_SD_ERR_EVENT)
    {
        if(key_id == kbPOWER)
        {
            divoom_sys_msg_info model_msg = {0};
            
			model_msg.dest_id = DIVOOM_MODEL_POWER_ON; 
			model_msg.msg_id = DIVOOM_POWER_ON_FORMAT_SD_MODE;
			divoom_sys_model_crl_msg_push(&model_msg); 
        }
        else if(key_id == DIVOOM_WORK_MODE_SWITCH && key_type == PRESS_SHORT)
        {
            divoom_print("we cancel formated the sd!\n");
            divoom_main_disp_cacnel_sd_format();
        }
    }
	return 0;
}

uint8 divoom_main_get_charge_status(void)
{
	//if(pDivoomHandle && pDivoomHandle->charge_status)
		//return (pDivoomHandle->charge_status == DIVOOM_IO_UNACTIVE)?0:1;
	return 0;
}

uint8 divoom_main_get_new_sd_status(void)
{
    return 0; //pDivoomHandle->new_sd_flag;
}
//当前ＳＤ卡是一张新卡
void divoom_main_new_sd_notice(void)
{
	divoom_sys_msg_info model_msg = {0};

    if(divoom_power_on_manage_get_power_mode()==DIVOOM_POWER_ON_WIFI_CONFIG_MODE)
    {
        return;
    }
    divoom_print("we will download all files!\n");       
    pDivoomHandle->new_sd_flag = 1;
    if(pDivoomHandle->sys_finish_init_ok)
    {
        divoom_main_disp_sd_in();   
    }
	model_msg.msg_type = 1;
	model_msg.src_id = DIVOOM_MODEL_MAIN;
	model_msg.dest_id = DIVOOM_MODEL_MAIN;
	model_msg.msg_id = DIVOOM_MAIN_EVENT_NEW_SD_NOTICE;
	divoom_sys_model_crl_msg_push(&model_msg);
    
}

void divoom_main_give_wait_download_msg(void)
{
    divoom_print("sd flag: %d!\n",pDivoomHandle->new_sd_flag);
    if(pDivoomHandle->new_sd_flag == 1)
    {
    	divoom_app_schedule_item_info push_info = {0};
    	push_info.event_id = DIVOOM_MAIN_PLAY_DOWNLOAD_OK_EVENT;
    	push_info.model_id = DIVOOM_MODEL_MAIN;
    	push_info.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
        push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
    	if(divoom_app_schedule_push(&push_info))
    	{
            divoom_disp_ctrl_set_mode(DIVOOM_DISP_DOWNLOAD_FILE_MODE, NULL);
    	}
        pDivoomHandle->new_sd_flag = 2;
    }
    else
    {
        divoom_disp_ctrl_set_mode(DIVOOM_DISP_DOWNLOAD_FILE_MODE, NULL);
    }
    
    
}

//通知系统必要数据已经下载完成，可以启动显示了
int divoom_main_download_data_notice(int no_cache_num, int cache_num)
{
	divoom_app_schedule_item_info push_info = {0};
	push_info.event_id = DIVOOM_MAIN_PLAY_DOWNLOAD_OK_EVENT;
	push_info.model_id = DIVOOM_MODEL_MAIN;
	push_info.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
    push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;

    if(pDivoomHandle->new_sd_flag)
    {
        push_info.event_id = DIVOOM_MAIN_PLAY_POWER_ON_EVENT;
        push_info.model_id = DIVOOM_MODEL_MAIN;
        push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
        divoom_app_schedule_cancel(&push_info);

        
    	push_info.event_id = DIVOOM_MAIN_PLAY_DOWNLOAD_OK_EVENT;
    	push_info.model_id = DIVOOM_MODEL_MAIN;
    	push_info.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
        push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
    	if(divoom_app_schedule_push(&push_info))
    	{
	        divoom_sys_msg_info model_msg = {0};
            Pdivoom_disp_ctrl_download_file_info msg = divoom_system_malloc(sizeof(divoom_disp_ctrl_download_file_info));

            divoom_print("no cache num: %d, cache num; %d\n", no_cache_num, cache_num);
            msg->wait_down_num = no_cache_num;
            msg->down_num = cache_num;
        	model_msg.msg_type = 1;
        	model_msg.src_id = DIVOOM_MODEL_MAIN;
        	model_msg.dest_id = DIVOOM_MODEL_MAIN;
        	model_msg.msg_id = DIVOOM_MAIN_EVENT_DOWNLOAD_ALL_DATA;
            model_msg.data = (uint8 *)msg;
        	divoom_sys_model_crl_msg_push(&model_msg);
            return 1;
    	}
    }
    return 0;
	//divoom_print("%s:%d:Send key info!", __func__, __LINE__);
}

//SD卡空间不够提醒
void divoom_main_sd_free_room_not_enough(void)
{
	divoom_sys_msg_info model_msg = {0};

    divoom_print("send sd warning event!\n");
	model_msg.msg_type = 1;
	model_msg.src_id = DIVOOM_MODEL_MAIN;
	model_msg.dest_id = DIVOOM_MODEL_MAIN;
	model_msg.msg_id = DIVOOM_MAIN_EVENT_SD_FREE_ROOM_NOT_ENOUGH;
	divoom_sys_model_crl_msg_push(&model_msg);
}

void divoom_main_update_storage_status(uint32 free_space)
{
	uint8 full_flag;
	int report_full;

	if(pDivoomHandle == NULL)
	{
		return;
	}

	full_flag = free_space < DIVOOM_ROOM_WARNING_MIN;
	if(pDivoomHandle->storage_status_valid == 0 ||
		pDivoomHandle->storage_full_flag != full_flag)
	{
		/* 状态变化后先记为待上报；断网期间不会丢失这次变化。 */
		pDivoomHandle->storage_full_flag = full_flag;
		pDivoomHandle->storage_status_valid = 1;
		pDivoomHandle->storage_status_reported = 0;
	}
	if(pDivoomHandle->storage_status_reported)
	{
		return;
	}
	if(divoom_net_mqtt_get_net_status() == 0 || divoom_main_get_server_con_status() == 0)
	{
		return;
	}

	report_full = full_flag;
	divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_SET_STORAGE_STATUS,
		DIVOOM_SERVER_TOPIC, &report_full);
	pDivoomHandle->storage_status_reported = 1;
}


//发送按键信息,直接发到main模块处理,先发给当前激活的模块,不处理的话再用全局功能
void divoom_main_keypad_send_key_info(uint8 key_value, uint16 press_type)
{
	Pdivoom_key_info send_key_info = NULL;
	divoom_sys_msg_info model_msg = {0};
#if DIVOOM_SHOW_MODE
	divoom_light_app_show_clock_rotate_pause();
#endif
	send_key_info = (Pdivoom_key_info)divoom_system_malloc(sizeof(divoom_key_info));
//	DIVOOM_MEM_CHECK(DIVOOM_APP_ONOFF_TAG, send_key_info, return);
	send_key_info->value = key_value;
	send_key_info->type = press_type;
    
#ifdef DIVOOM_LOG_SAVE_SD_FLAG
    if(key_value == kbLIGHT)
    {
        divoom_log_close();
    }
#endif

	model_msg.msg_type = 1;
	model_msg.src_id = DIVOOM_MODEL_MAIN;
	model_msg.dest_id = DIVOOM_MODEL_MAIN;
	model_msg.msg_id = DIVOOM_MAIN_EVENT_KEY_PRESS_EVENT;
	model_msg.data = (void *)send_key_info;
	divoom_sys_model_crl_msg_push(&model_msg);
	//divoom_print("%s:%d:Send key info!", __func__, __LINE__);
}

uint32 divoom_main_onoff_get_next_active_alarm_time(void)
{
	void *dialy_alarm_queue = divoom_queue_init(100, __func__, __LINE__);
	uint32 time = 0;

	pDivoomHandle = (Pdivoom_sys_ctrl_handle)divoom_malloc(sizeof(divoom_sys_ctrl_handle), DIVOOM_MODEL_MAIN, __LINE__, __FILE__);

	memset(pDivoomHandle, 0, sizeof(divoom_sys_ctrl_handle));
    pDivoomHandle->app_list = divoom_system_malloc(sizeof(divoom_device_app_id_list));
    pDivoomHandle->app_list->num = 0;
	pDivoomHandle->app_list_lock = divoom_system_create_lock();
    //pDivoomHandle->charge_status = DIVOOM_IO_UNACTIVE;
	//上电默认开机状态
	pDivoomHandle->master_flag = 1;
    pDivoomHandle->back_light_off = 1;
    
//校正时区
	divoom_main_load_config();
	divoom_main_load_timezone_config();
	divoom_alarm_poweron_get_all_alarm_info(dialy_alarm_queue);
	time = divoom_alarm_poweron_lib_get_next_active_alarm_time(dialy_alarm_queue);
	return time;
}

void divoom_main_copy_config_to_sd(void)
{
}


//当前ＳＤ卡是一张新卡
void divoom_main_clock_change_notice(uint32 data)
{
	divoom_sys_msg_info model_msg = {0};

	model_msg.msg_type = 1;
	model_msg.src_id = DIVOOM_MODEL_MAIN;
	model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;
	model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_NOTICE_CLOCK_CHANGE;
	divoom_sys_model_crl_msg_push(&model_msg);
    
}

uint8 divoom_main_set_power_flag(uint8 flag)
{
    if(flag == 0)
    {
        divoom_app_com_del_file(DIVOOM_DEIVCE_POWER_FLAG_SAVE_PATH);
        divoom_print("we will del power flag!\n");
        return 0;
    }
    else
    {
        uint32 times = 0;
        FILE *sys_config = divoom_port_file_open(DIVOOM_DEIVCE_POWER_FLAG_SAVE_PATH, "rb");
        if(sys_config != NULL)
        {
            if( 0 == divoom_port_file_read(&times, sizeof(times), 1, sys_config))
            {
                times = 0;
            }
            divoom_port_file_close(sys_config);
        }
        
        sys_config = divoom_port_file_open(DIVOOM_DEIVCE_POWER_FLAG_SAVE_PATH, "wb");
        times ++;
        divoom_port_file_write(&times, sizeof(times), 1, sys_config);
        divoom_port_file_close(sys_config);
        divoom_print("we will update power flag!(%d)\n", times);
        return times -1;
    }
}

void divoom_main_format_key_info(uint8 key_value, uint16 press_type)
{
    static int key_times = 0;
    Pdivoom_key_info key_info;

    
    if(press_type != PRESS_SHORT)
    {
        return;
    }
    key_times ++;
    if(key_times == 1)
    {
        return;
    }
    divoom_print("key: %d, %d\n", key_value, press_type);
    key_info = divoom_system_malloc(sizeof(divoom_key_info));
    if(key_info)
    {
        key_info->value = key_value;
        key_info->type = press_type;
        divoom_queue_push(pDivoomHandle->key_queue, key_info);
    }
}

void divoom_main_format_mount_ok_sb_func(uint8 status, uint8 flag)
{
}

void *divoom_main_format_loop(void *data)
{
    divoom_disp_Play_net_image_image_info notice_info = {0};
    divoom_disp_event_disp_info event_info = {0};
    Pdivoom_key_info key_info;
    

    divoom_print_err("start format loop!\n");
    notice_info.image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + DIVOOM_DISP_PIXOO64_FORMAT_RESET;
    notice_info.model_id = DIVOOM_MANAGE_IMAGE_NOTICE_NET_GIF;
    notice_info.play_time = 0xFFFFF;
    notice_info.pcallback = NULL;


    
    event_info.disp_id[0] = DIVOOM_DISP_LOADING_00;
    event_info.disp_id[1] = DIVOOM_DISP_LOADING_01;
    event_info.disp_id[2] = DIVOOM_DISP_LOADING_02;
    event_info.disp_id[3] = DIVOOM_DISP_LOADING_03;
    event_info.disp_id[4] = DIVOOM_DISP_LOADING_04;
    event_info.disp_id[5] = DIVOOM_DISP_LOADING_05;
    event_info.disp_id[6] = DIVOOM_DISP_LOADING_06;
    event_info.disp_id[7] = DIVOOM_DISP_LOADING_07;
	event_info.on_time = 1;
	event_info.period_time = 1;
	event_info.tot_pic = 8;
	event_info.tot_time = 50000;
    event_info.noice_id = DIVOOM_MAIN_PLAY_DEVICE_UPDATE_EVENT;
    event_info.pover_callback = divoom_main_disp_power_over_notice;
    
    divoom_disp_ctrl_set_mode(DIVOOM_DISP_PLAY_NET_IMAGE_CTRL, (void *)&notice_info);
    while(1)
    {
        key_info = divoom_queue_pop(pDivoomHandle->key_queue);
        if(key_info)
        {
            if(key_info->value == kbPOWER)
            {
                
            }
            else if(key_info->value == kbLIGHT)
            {
                divoom_print_err("start reset system!\n");
            }
        }
        divoom_system_delay_ms(50);
    }

    return NULL;
}

float divoom_main_get_wind_speed(void)
{
    float value;

    pthread_mutex_lock(&s_weather_lock);
    value = pDivoomHandle->weather_info.wind_speed;
    pthread_mutex_unlock(&s_weather_lock);
    return value;
}

int divoom_main_get_today_visibility(void)
{
    int value;

    pthread_mutex_lock(&s_weather_lock);
    value = pDivoomHandle->weather_info.visibility;
    pthread_mutex_unlock(&s_weather_lock);
    return value;
}


int divoom_main_get_sunrise_time(void)
{
    int value;

    pthread_mutex_lock(&s_weather_lock);
    value = pDivoomHandle->sunrise_second;
    pthread_mutex_unlock(&s_weather_lock);
    return value;
}

int divoom_main_get_sunset_time(void)
{
    int value;

    pthread_mutex_lock(&s_weather_lock);
    value = pDivoomHandle->sunset_second;
    pthread_mutex_unlock(&s_weather_lock);
    return value;
}

//得到当前播放的歌名
char *divoom_main_get_music_name(void)
{
	static const char s_idle_music_title[] = "Divoom";
	uint8 mode;
	char *radio_name;
	char *tf_name;

	/* 电台连接、暂停和后台播放均展示当前选台，不读取残留的蓝牙元数据。 */
	mode = divoom_music_ctrl_get_mode();
	if (mode == DIVOOM_MUSIC_CTRL_MODE_NET_FM)
	{
        radio_name = divoom_radio_copy_selected_name();
		if (radio_name)
		{
			return radio_name;
		}
        return divoom_menu_control_get_disp_str("UnknownMusicName");
	}

	if (!divoom_music_ctrl_is_playing())
		return (char *)s_idle_music_title;

	if (mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
	{
		tf_name = divoom_tf_music_get_playing_disp_name();
		if (tf_name == NULL || tf_name[0] == '\0')
			return divoom_menu_control_get_disp_str("UnknownMusicName");
		return tf_name;
	}
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
	if (mode == DIVOOM_MUSIC_CTRL_MODE_UAC)
		return divoom_menu_control_get_disp_str("SourceUAC");
#endif
	return (char *)divoom_bt_music_get_song_name();
}

//得到当前播放的歌手
char *divoom_main_get_music_singer(void)
{
	uint8 mode;
	char *radio_name;

	/* 电台连接、暂停和后台播放均展示当前选台，不读取残留的蓝牙元数据。 */
	mode = divoom_music_ctrl_get_mode();
	if (mode == DIVOOM_MUSIC_CTRL_MODE_NET_FM)
	{
        radio_name = divoom_radio_copy_selected_name();
		if (radio_name)
		{
			return radio_name;
		}
        return divoom_menu_control_get_disp_str("UnknownMusicName");
	}

	if (mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
		return (char *)"";
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
	if (mode == DIVOOM_MUSIC_CTRL_MODE_UAC)
		return (char *)"";
#endif
	return (char *)divoom_bt_music_get_song_singer();
}

//得到当前播放的歌词
char *divoom_main_get_music_lyric(void)
{
	static const char s_idle_music_lyric[] = "Spark your life.";
	uint8 mode;
	char *radio_name;

	/* 电台连接、暂停和后台播放均展示当前选台，不读取残留的蓝牙元数据。 */
	mode = divoom_music_ctrl_get_mode();
	if (mode == DIVOOM_MUSIC_CTRL_MODE_NET_FM)
	{
        radio_name = divoom_radio_copy_selected_name();
		if (radio_name)
		{
			return radio_name;
		}
        return divoom_menu_control_get_disp_str("UnknownMusicName");
	}

	if (!divoom_music_ctrl_is_playing())
		return (char *)s_idle_music_lyric;

	if (mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
		return (char *)"";
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
	if (mode == DIVOOM_MUSIC_CTRL_MODE_UAC)
		return (char *)"";
#endif
	return (char *)divoom_bt_music_get_song_lyric();
}

//得到上一句歌词
char *divoom_main_get_music_prev_lyric(void)
{
	uint8 mode;

	mode = divoom_music_ctrl_get_mode();
	if (mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
		return (char *)"";
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
	if (mode == DIVOOM_MUSIC_CTRL_MODE_UAC)
		return (char *)"";
#endif
	return (char *)divoom_bt_music_get_song_prev_lyric();
}

//得到当前播放的状态
int divoom_main_get_play_music_status(void)
{
    /* 与 AI 表盘及系统音乐状态一致，包含后台电台，不再返回固定占位值。 */
    return divoom_music_ctrl_is_playing() ? 1 : 0;
}


int8 divoom_main_get_wifi_rssi(void)
{
	int8 rssi;

	if (divoom_wifi_model_get_sta_rssi(&rssi) == 0)
	{
		return rssi;
	}
	return 0;
}

char *g_divoom_dbg_file_name = "test";
int  g_divoom_dbg_file_line;
void divoom_main_set_dbg_info(char *file_name, int line)
{
    g_divoom_dbg_file_name = file_name;
    g_divoom_dbg_file_line = line;
}
void divoom_main_dbg(void *rar)
{
    while(1)
    {
        divoom_print("test: %s; %d\n", g_divoom_dbg_file_name, g_divoom_dbg_file_line);        
        divoom_system_delay_ms(2000);
    }
}

void divoom_main_test_strech(void)
{
    int image_w, image_h, dest_w, dest_h;
    uint8 *in_buf, *out_buf;

    image_w = 800;
    image_h = 1280;
    in_buf = divoom_system_malloc(image_w *image_h * 4);
    
    
    dest_w = 236;
    dest_h = 376;
    out_buf = divoom_system_malloc(dest_w *dest_h * 4);

    for(int i = 0; i < 100; i++)
    {
        divoom_print("aj test!: %d\n", i);
        //divoom_rk_mpp_stretch_image_direct_test(in_buf, out_buf);
        //divoom_system_delay_ms(1000);
        
        divoom_rk_mpp_stretch_image_direct(in_buf, image_w, image_h, RK_FORMAT_BGRA_8888, out_buf, dest_w, dest_h, RK_FORMAT_BGRA_8888);
    }
    exit(0);
}

void divoom_main_copy_last_log_file(int log_file_id)
{
    FILE *log_file_handle;
    uint8 *file_buf;
    int buf_size, file_size;

    file_buf = divoom_log_get_buffer(&buf_size);
    if(file_buf == NULL)
    {
        return;
    }
    log_file_handle = divoom_port_file_open(DIVOOM_DEBUG_FILE_NAME, "rt");
    if(log_file_handle == NULL)
    {
        return;
    }
    divoom_port_file_seek(log_file_handle, 0, SEEK_END);
    file_size = divoom_port_file_tell(log_file_handle);
    if(file_size > buf_size)
    {
        divoom_port_file_seek(log_file_handle, file_size - buf_size, SEEK_SET);        
    }
    else
    {
        divoom_port_file_seek(log_file_handle, 0, SEEK_SET);        
        buf_size = file_size;
    }
    buf_size = divoom_port_file_read(file_buf, 1, buf_size, log_file_handle);
    divoom_port_file_close(log_file_handle);
    buf_size = write(log_file_id, file_buf, buf_size);    
}

void divoom_sigterm_handler(int sig)
{
    int ret;
    pthread_t tid = pthread_self();
    
    printf("we get sig: %d, tid:%lu\n", sig, tid);
    
#if defined(SIGABRT) && defined(SIGSEGV)
    if(sig == SIGABRT || sig == SIGSEGV)
    {
        int fd = open(DIVOOM_TMP_DIR "/divoom_start", O_WRONLY | O_CREAT | O_APPEND, 0644);
        if(fd >= 0) {
            ret = write(fd, "1", 1);
            close(fd);
        }
    }
#endif
    printf("start check!\n");
    if(sig == SIGINT)
    {
        //JSON参数
        exit(1);
    }
    else if(sig == SIGTERM
#if defined(SIGPWR)
            || sig == SIGPWR
#endif
            )
    {
        exit(1);
    }
#if defined(SIGABRT) && defined(SIGSEGV)
    else if(sig == SIGABRT || sig == SIGSEGV)
    {
#if DIVOOM_HAVE_EXECINFO
        /* 代码异常：在提供 execinfo 的 libc 上打印 backtrace */
        int divoom_log;
        char file_name[64], **debug_ptr;
        void *array[10];
        size_t size;

        size = backtrace(array, 10);
        if(size <= 2)
        {
            exit(1);
        }

        divoom_print("we get sig: %d, tid:%lu, version: %d\n", sig, tid, DIVOOM_SOFTWARE_VERSION);

        fprintf(stderr,"Error: signalas %d:%d\n", sig, size);

        divoom_log_close();

        // 打印堆栈帧地址
        sprintf(file_name, "%s/divoom_err_log%d.txt", DIVOOM_USER_DATA_DIR, pDivoomHandle->system_err_log);
        divoom_log  = open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if(divoom_log  <= 0)
        {
            divoom_log = STDERR_FILENO;
        }
        divoom_main_copy_last_log_file(divoom_log);
        ret = write(divoom_log, "\r\n\r\n", strlen("\r\n\r\n"));
        ret = write(divoom_log, file_name, strlen(file_name));
        ret = write(divoom_log, "\r\n", strlen("\r\n"));
        // 获取堆栈帧地址
        debug_ptr = backtrace_symbols(array, size);
        if(debug_ptr)
        {
            for(int i = 0; i < size; i ++)
            {
                if(debug_ptr[i])
                {
                    ret = write(divoom_log, debug_ptr[i], strlen(debug_ptr[i]));
                    ret = write(divoom_log, "\r\n", strlen("\r\n"));
                    fprintf(stderr,"we get debug_ptr: %s\n", debug_ptr[i]);
                }
            }
        }
        if(divoom_log != STDERR_FILENO)
        {
            close(divoom_log);
        }

        {
        int fd = open(DIVOOM_TMP_DIR "/divoom_end", O_WRONLY | O_CREAT | O_APPEND, 0644);
        if(fd >= 0) {
            ret = write(fd, "1", 1);
            close(fd);
        }
        }

        exit(1);
#else
        divoom_print("fatal signal %d (no execinfo/backtrace)\n", sig);
        exit(1);
#endif
    }
#endif /* SIGABRT && SIGSEGV */
#if defined(SIGPIPE)
    else if(sig < SIGPIPE)
    {
        exit(1);
    }
#endif
}

/*********************************************************************
  Function:        divoom_main_upoad_err_file
  Description:     联网读取异常、合并上传并清理已上报文件
                    
  Para:             para
  Return:           
**********************************************************************/

#if DIVOOM_BUILD_RTOS_DEVICE && defined(CONFIG_DIVOOM_CRASH_LOG)
static void *divoom_main_upload_crash_task(void *arg)
{
    (void)arg;
    return NULL;
}
#endif

void divoom_main_upoad_err_file(void)
{
#if DIVOOM_BUILD_RTOS_DEVICE && defined(CONFIG_DIVOOM_CRASH_LOG)
    /* 崩溃包上传可能等待网络；网络命令线程必须继续处理切表盘请求。 */
    if (divoom_net_app_net_is_ok() &&
        divoom_system_create_task(divoom_main_upload_crash_task, "crash_upload", NULL,
            DIVOOM_TASK_STACK_MAIN_TEST_URL) == NULL)
    {
        divoom_print("[crash] upload task create failed; bundle retained\n");
    }
#endif
}

/*********************************************************************
  Function:        divoom_main_update_test_file
  Description:     上传测试文件
                    
  Para:             para
  Return:           
**********************************************************************/

void divoom_main_update_test_file(void *para)
{
    (void)para;
}

/*********************************************************************
  Function:        divoom_main_update_dead_lock_file
  Description:     上传死锁文件,上传完了，设备将自启动
                    
  Para:             para
  Return:           
**********************************************************************/

void divoom_main_update_dead_lock_file(char *lock_info)
{    
    divoom_open_update_dead_lock_info(lock_info);
}

/*********************************************************************
  Function:        divoom_main_set_divoom_app_fail_type
  Description:     设置应用不断重启的失败类型次数，第一次删除divoom_config,第二次删除rootfs_data,第三次删除user_data
                    
  Para:             fail_type
  Return:           
**********************************************************************/

void divoom_main_set_divoom_app_fail_type(int fail_type)
{
    char *file_buf;
	divoom_cJSON *root = NULL;
    if(fail_type == 0)
    {
        //正常运行，清除异常记录
        divoom_app_com_del_file(DIVOOM_RUN_FAIL_LOG_FILE_PATH);
        return;
    }
    root = divoom_cJSON_CreateObject();
    if(root == NULL)
    {
        return;
    }
    
    divoom_cJSON_AddNumberToObject(root, "FailType", fail_type);

    file_buf = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);
    divoom_app_com_write_file(DIVOOM_RUN_FAIL_LOG_FILE_PATH,file_buf, strlen(file_buf));
    divoom_system_free(file_buf);
}

/*********************************************************************
  Function:        divoom_main_get_divoom_app_fail_type
  Description:     获取应用不断重启的失败类型次数
                    
  Para:             fail_type
  Return:           
**********************************************************************/

int divoom_main_get_divoom_app_fail_type(void)
{
    int fail_type =0;
    char *file_buf;
	divoom_cJSON *root = NULL;

    
    file_buf  = (uint8 *)divoom_app_com_read_file(DIVOOM_RUN_FAIL_LOG_FILE_PATH, NULL);
    if(file_buf == NULL)
    {
        return 0;
    }
	root = divoom_cJSON_Parse(file_buf);
    divoom_system_free(file_buf);
    if(root)
    {
        fail_type = divoom_cJSON_GetObjectInt(root, "FailType");
        divoom_cJSON_Delete(root);
    }
    return fail_type;
    
}

void divoom_main_save_format_info_file(int restart_time, int fail_type, int format_flag)
{
	char *format_json = NULL;
	divoom_cJSON *format_info = NULL;
	char file_name[64];
	char cmd[128];

	sprintf(file_name, "%s/divoom_format_log.txt", DIVOOM_USER_DATA_DIR);

	format_info = divoom_cJSON_CreateObject();
	
	if(format_info != NULL)
	{
		divoom_cJSON_AddNumberToObject(format_info, "RestartTimes", restart_time);
		divoom_cJSON_AddNumberToObject(format_info, "FailType", fail_type);
		divoom_cJSON_AddNumberToObject(format_info, "FormatFlag", format_flag);
		format_json = divoom_cJSON_Print(format_info);
		divoom_cJSON_Delete(format_info);
		
		divoom_app_com_write_file(file_name, format_json, strlen(format_json));
		divoom_system_free(format_json);
		
		sprintf(cmd, "chmod +x %s", file_name);
		divoom_exec_cmd(cmd, NULL, 0);
		
		//divoom_main_update_format_file(file_name, 1, 1);
	}
}

int divoom_system_err_log;
/*
此文件系统运行十分钟后就将计数设置为0，每次运行，都将计数加一，初始值为1
文件格式为json格式：
{
    "RestartTimes": 10
}
*/
void divoom_main_check_run_log(int flag)
{
    int restart_time = 0;
	divoom_cJSON *root = NULL;
    char *file_buf;

    
#ifdef DIVOOM_SIMULATOR
    return;
#endif
    
    
    if(flag)
    {
        if(divoom_port_file_exists(DIVOOM_RUN_LOG_FILE_PATH))
        {
           divoom_system_err_log = 1;
           file_buf  = (uint8 *)divoom_app_com_read_file(DIVOOM_RUN_LOG_FILE_PATH, NULL);

            if(file_buf)
            {
            	root = divoom_cJSON_Parse(file_buf);
                divoom_system_free(file_buf);
                if(root)
                {
                    restart_time = divoom_cJSON_GetObjectInt(root, "RestartTimes");
                    restart_time ++;
                    divoom_system_err_log = restart_time;
                    divoom_cJSON_Delete(root);
                }
            }
        }
        else
        {
            divoom_system_err_log = 0;
        }
    }
    else
    {
        restart_time = 0;
        divoom_main_set_divoom_app_fail_type(0);
    }
    
    root = divoom_cJSON_CreateObject();
    if(root == NULL)
    {
        return;
    }
    
    divoom_cJSON_AddNumberToObject(root, "RestartTimes", restart_time);

    file_buf = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);
    divoom_app_com_write_file(DIVOOM_RUN_LOG_FILE_PATH,file_buf, strlen(file_buf));
    divoom_system_free(file_buf);

    divoom_system_err_log = restart_time;
    divoom_print("divoom_app run times: %d\n", restart_time);
}

void divoom_main_test_video(void)
{
    divoom_decode_movie_review_pic_create("/userdata/app_pic/20255/300000003-1748328494516.mp4", "/userdata/test.webp", 724, 1280, 0, 0);
}
void *divoom_main_test_url_request_main(void *data)
{
    char *net_data;
    while(1)
    {
        net_data = divoom_http_client_url_request("https://qt.gtimg.cn/q=fuZN,fuZF,fuZT", NULL, 0, NULL, NULL);
        if(net_data)
        {
            divoom_system_free(net_data);
        }
        divoom_system_delay_ms(1000);
    }
    return NULL;
}

void divoom_main_test_url_request(void)
{
    for(int i = 0; i < 20; i ++)
    {
        divoom_system_create_task(divoom_main_test_url_request_main, "test_url", NULL, DIVOOM_TASK_STACK_MAIN_TEST_URL);
    }
}

void divoom_light_music_global_init(void);

//是否是系统异常退出的
int divoom_main_is_app_exit_flag(void)
{
    return divoom_system_err_log;
}

long divoom_main_get_ram_free_size(void)
{
    return divoom_system_get_ram_free_size();
}

//检查系统可用内存是否小于35M
int divoom_main_check_ram_garbage(void)
{
    long mem_available = divoom_main_get_ram_free_size();

    divoom_print("system ram : %ld kb\n", mem_available);
#if DIVOOM_BUILD_RTOS_DEVICE
    {
        long total_kb = divoom_system_get_ram_total_size();

        if (total_kb <= 0) {
            total_kb = 1;
        }
        if (mem_available < total_kb / 10) {
            divoom_print("system ram : %ld kb\n", mem_available);
            return 2;
        }
        if (mem_available < total_kb / 5) {
            divoom_print("system ram : %ld kb\n", mem_available);
            return 1;
        }
        return 0;
    }
#else
    if(mem_available < 30*1024)
    {
        divoom_print("system ram : %ld kb\n", mem_available);
        return 2;
    }
    else if(mem_available < 35*1024)
    {
        divoom_print("system ram : %ld kb\n", mem_available);
        return 1;
    }
    else
    {
        return 0;
    }
#endif
}
#if 0
void divoom_convret_colck_info(void)
{
    int clock_id;
    DIR *clock_dir_handle;
    Pdivoom_clock_manage_item pclock_info;
    

    //查找目录下面的所有图片文件
    clock_dir_handle = divoom_port_dir_open(DIVOOM_SD_SYSTEM_SYS_CLOCKS_DIR);
    if(clock_dir_handle)
    {
        const divoom_port_dirent_t *dirp;
        int file_num = 0;

        while((dirp = divoom_port_dir_read(clock_dir_handle)) != NULL)
        {
            divoom_print("find: type: %d, %s\n", dirp->is_dir, dirp->name);
            if(dirp->is_dir == 0)
            {
                sscanf(dirp->name, "clock%d.cfg", &clock_id);
                if(clock_id)
                {
                    pclock_info = divoom_clock_manage_get_clock_info(clock_id, 0, 0);
                    if(pclock_info)
                    {
                        divoom_print("it ok to create clock: %d:%d\n",file_num, clock_id);
                        divoom_clock_manage_save_sys_clock_info(pclock_info);
                        divoom_clock_manage_free_clock(pclock_info);
                        file_num ++;
                    }
                    else
                    {
                        divoom_print("it fails to create clock: %d\n", clock_id);
                    }
                }
            }
        }
        
        divoom_port_dir_close(clock_dir_handle);
    }
    else
    {
        return 0;
    }
}
#endif

int divoom_main_get_tmp_file_index(void)
{
    int file_index;

    divoom_system_get_lock(pDivoomHandle->tmp_file_lock);
    pDivoomHandle->tmp_file_index ++;
    file_index = pDivoomHandle->tmp_file_index;
    divoom_system_free_lock(pDivoomHandle->tmp_file_lock);
    return file_index;
}


void divoom_main_update_last_download_file(void)
{
    if(!pDivoomHandle)
        return;
    pDivoomHandle->last_download_file_time = time(0);
}


#define DIVOOM_WIFI_CONF_MAX_NET  3

/**
 * @brief 裁剪 JSON 格式 wifi.conf，最多保留前 DIVOOM_WIFI_CONF_MAX_NET 组网络
 */
static int divoom_main_trim_wifi_conf_json(const char *file_path)
{
    char *file_buf;
    int file_len;
    int i, num;
    divoom_cJSON *root;
    divoom_cJSON *list;
    divoom_cJSON *new_list;
    divoom_cJSON *item;
    char *json_string;

    file_buf = divoom_app_com_read_file((char *)file_path, &file_len);
    if (file_buf == NULL)
    {
        return 0;
    }

    for (i = 0; file_buf[i] != '\0'; i++)
    {
        if (file_buf[i] != ' ' && file_buf[i] != '\t' &&
            file_buf[i] != '\r' && file_buf[i] != '\n')
        {
            if (file_buf[i] != '{')
            {
                divoom_system_free(file_buf);
                divoom_print("[WIFI_CFG] trim skip, legacy non-JSON: %s\n", file_path);
                return 0;
            }
            break;
        }
    }

    root = divoom_cJSON_Parse(file_buf);
    divoom_system_free(file_buf);
    if (root == NULL)
    {
        divoom_print("[WIFI_CFG] trim skip, json parse failed: %s\n", file_path);
        return -1;
    }

    list = divoom_cJSON_GetObjectItem(root, "wifi");
    if (list == NULL || (list->type & divoom_cJSON_Array) == 0)
    {
        divoom_cJSON_Delete(root);
        return 0;
    }

    num = divoom_cJSON_GetArraySize(list);
    if (num <= DIVOOM_WIFI_CONF_MAX_NET)
    {
        divoom_cJSON_Delete(root);
        return 0;
    }

    new_list = divoom_cJSON_CreateArray();
    if (new_list == NULL)
    {
        divoom_cJSON_Delete(root);
        return -1;
    }

    for (i = 0; i < DIVOOM_WIFI_CONF_MAX_NET; i++)
    {
        item = divoom_cJSON_GetArrayItem(list, i);
        if (item != NULL)
        {
            divoom_cJSON_AddItemToArray(new_list, divoom_cJSON_Duplicate(item, 1));
        }
    }

    divoom_cJSON_DeleteItemFromObject(root, "wifi");
    divoom_cJSON_AddItemToObject(root, "wifi", new_list);

    json_string = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);
    if (json_string == NULL)
    {
        return -1;
    }

    divoom_app_com_write_file((char *)file_path, json_string, strlen(json_string));
    divoom_system_free(json_string);
    divoom_print("[WIFI_CFG] trim wifi.conf: %d -> %d networks\n", num, DIVOOM_WIFI_CONF_MAX_NET);
    return 0;
}

/**
 * @brief 检查并处理 wifi.conf（RTOS 为 JSON；模拟器保留 wpa 文本裁剪）
 * @param file_path 配置文件路径
 * @return 0成功，-1失败
 */
int divoom_main_trim_wpa_config(const char *file_path)
{
#if DIVOOM_BUILD_RTOS_DEVICE
    return divoom_main_trim_wifi_conf_json(file_path);
#else
    #define MAX_LINE_LEN 1024
    #define MAX_NETWORKS 3
    void *fp_read = NULL;
    void *fp_write = NULL;
    char line[MAX_LINE_LEN];
    T_S32 file_size = 0;
    T_S32 network_count = 0;
    T_S32 brace_depth = 0;
    T_S32 in_network_block = 0;
    T_S32 skip_mode = 0;
    char *output_buf = NULL;
    T_U32 output_len = 0;
    T_U32 output_capacity = 0;
    T_S32 i;
    char first_non_ws = 0;
    
    fp_read = divoom_port_file_open(file_path, "r");
    if (fp_read == NULL)
    {
        divoom_print("无法打开文件: %s\n", file_path);
        return -1;
    }

    /* AstroToo RTOS: wifi.conf is JSON written by divoom_wifi_con, not wpa_supplicant text. */
    while (divoom_port_file_gets(line, (T_S32)sizeof(line), fp_read) != NULL)
    {
        for (i = 0; line[i] != '\0'; i++)
        {
            if (line[i] == ' ' || line[i] == '\t' || line[i] == '\r' || line[i] == '\n')
            {
                continue;
            }
            first_non_ws = line[i];
            break;
        }
        if (first_non_ws != 0)
        {
            break;
        }
    }
    divoom_port_file_close(fp_read);
    if (first_non_ws == '{')
    {
        divoom_print("skip trim for JSON wifi config: %s\n", file_path);
        return 0;
    }
    fp_read = divoom_port_file_open(file_path, "r");
    if (fp_read == NULL)
    {
        return -1;
    }
    
    divoom_port_file_seek(fp_read, 0, DIVOOM_PORT_SEEK_END);
    file_size = (T_S32)divoom_port_file_tell(fp_read);
    divoom_port_file_seek(fp_read, 0, DIVOOM_PORT_SEEK_SET);
    
    output_capacity = (T_U32)file_size + 1U;
    output_buf = (char *)divoom_system_malloc(output_capacity);
    if (output_buf == NULL)
    {
        divoom_print("内存分配失败\n");
        divoom_port_file_close(fp_read);
        return -1;
    }
    output_buf[0] = '\0';
    output_len = 0;
    
    while (divoom_port_file_gets(line, (T_S32)sizeof(line), fp_read) != NULL)
    {
        if (strstr(line, "network=") != NULL || strstr(line, "network =") != NULL)
        {
            network_count++;
            in_network_block = 1;
            brace_depth = 0;
            
            if (network_count > MAX_NETWORKS)
            {
                skip_mode = 1;
                divoom_print("发现第 %d 个network配置，将被删除\n", network_count);
            }
        }
        
        if (in_network_block)
        {
            for (i = 0; line[i] != '\0'; i++)
            {
                if (line[i] == '{')
                {
                    brace_depth++;
                }
                else if (line[i] == '}')
                {
                    brace_depth--;
                    if (brace_depth == 0)
                    {
                        if (!skip_mode)
                        {
                            strcat(output_buf, line);
                            output_len += (T_U32)strlen(line);
                        }
                        in_network_block = 0;
                        skip_mode = (network_count >= MAX_NETWORKS) ? 1 : 0;
                        goto next_line;
                    }
                }
            }
        }
        
        if (!skip_mode)
        {
            strcat(output_buf, line);
            output_len += (T_U32)strlen(line);
        }
        
next_line:
        continue;
    }
    
    divoom_port_file_close(fp_read);
    
    printf("共发现 %d 个network配置\n", network_count);
    
    if (network_count > MAX_NETWORKS)
    {
        fp_write = divoom_port_file_open(file_path, "w");
        if (fp_write == NULL)
        {
            divoom_print("无法写入文件: %s\n", file_path);
            divoom_system_free(output_buf);
            return -1;
        }
        
        divoom_port_file_puts(output_buf, fp_write);
        divoom_port_file_close(fp_write);
        
        divoom_print("已删除多余的 %d 个network配置，保留前 %d 个\n", 
               network_count - MAX_NETWORKS, MAX_NETWORKS);
    }
    else
    {
        divoom_print("network配置数量(%d)未超过 %d 个，无需处理\n", 
               network_count, 3);
    }
    
    divoom_system_free(output_buf);
    return 0;
#endif /* !DIVOOM_BUILD_RTOS_DEVICE */
}


void divoom_main_check_wifi_daemon_file(void)
{
#if DIVOOM_BUILD_RTOS_DEVICE
    /* AstroToo RTOS：无 overlay/wifi_daemon/wifimg，仅维护 JSON wifi.conf */
    divoom_main_trim_wpa_config(DIVOOM_WPA_SUPPLICANT_CONF);
#else
    if(divoom_app_com_check_file_exist("/overlay/upper/bin/wifi_daemon"))
    {
        divoom_print("we will restore wifi driver !\n\n");
        exec_command_system("rm -rf /overlay/upper/lib");
        exec_command_system("rm -rf /overlay/upper/bin");
        exec_command_system("rm -rf /overlay/upper/etc/wifi/wifimg.config");
        exec_command_system("rm -rf /overlay/upper/etc/wifi/wifimg.def");
        exec_command_system("rm -rf /overlay/upper/usr/bin/divoom_bt_manage");
        exec_command_system("rm -rf /overlay/upper/usr/bin/divoom_restart");
        exec_command_system("sync");
        divoom_system_delay_ms(1000);
        exec_command_system("reboot");
    }
    divoom_main_trim_wpa_config(DIVOOM_WPA_SUPPLICANT_CONF);
#endif
}

void divoom_main_check_md5_wpa_file(void)
{
#if DIVOOM_BUILD_RTOS_DEVICE
    /* AstroToo RTOS：无 wpad / overlay CGI，WiFi 由 aic_wifi 管理 */
    return;
#elif defined(DIVOOM_SIMULATOR)
    /* Windows 仿真跳过 sha1sum/reboot 校验 */
    return;
#else
    char  sha1sum_info[128];

    
    if(divoom_app_com_check_file_exist("/usr/share/divoom_app/cgi-bin/run_check.sh"))
    {
        divoom_app_com_get_file_sha1sum("/usr/share/divoom_app/cgi-bin/run_check.sh",sha1sum_info);
    }
    else
    {
        sha1sum_info[0] = 0;
    }
    if(!(strcmp(sha1sum_info, "5640b41b0187abcee269cb4362e2b19c6acd4633") == 0 || strcmp(sha1sum_info, "4f16f5322a8384ba692a0f8e22e0c047aa243769") == 0))
    {
        divoom_print("we will restore /usr/share/divoom_app/cgi-bin/run_check.sh !\n\n");
        exec_command_system("cp /rom/usr/share/divoom_app/cgi-bin/run_check.sh /usr/share/divoom_app/cgi-bin/run_check.sh");
        exec_command_system("chmod +x /usr/share/divoom_app/cgi-bin/run_check.sh");
    }

    divoom_app_com_get_file_sha1sum("/usr/sbin/wpad",sha1sum_info);
    if(strcmp(sha1sum_info, "0b32c2790b38cf1ee221489bda8431e23261e2d2") == 0 || strcmp(sha1sum_info, "3b435a8ac8cd4d8a287bff2fe06425886da68bd9") == 0)
    {
        //文件校验正常
        exec_command_system("chmod +x /usr/sbin/wpad");
        divoom_print("/usr/sbin/wpad is ok!: %s\n\n", sha1sum_info);
        return;
    }
    divoom_print("we will restore /usr/sbin/wpad !\n\n");
    exec_command_system("cp /rom/usr/sbin/wpad /usr/sbin/wpad");
    exec_command_system("chmod +x /usr/sbin/wpad");
    divoom_update_system_file_save_sys_file_version(66);
    exec_command_system("sync");
    exec_command_system("reboot");
#endif
}

uint8 divoom_main_get_audio_type_statu(uint8 types)
{
	if (types == 0) //uac
		return pDivoomHandle->audio_det_info.uac_statu;
	else if (types == 1) //linein
		return pDivoomHandle->audio_det_info.linein_statu;
	else if (types == 2) //tf
		return pDivoomHandle->audio_det_info.tf_statu;
	else if (types == 3)
		return divoom_bt_music_is_connected();
	return 0;
}

/* 音乐模式：蓝牙、UAC、TF卡、LineIn。插入TF卡切到TF模式；拔出时若当前为TF则切到下一模式，否则仅提示。
 * 开机时会上报一次当前状态，仅更新状态，不弹提示、不切模式。 */
void divoom_main_sdcard_det_cb(DIVOOM_SD_CARD_STATE status)
{
	divoom_print("wft test sdcard status: %d\n", status);
	if (status == DIVOOM_SD_CARD_STATE_MOUNTED)
		pDivoomHandle->audio_det_info.tf_statu = 1;
	else
		pDivoomHandle->audio_det_info.tf_statu = 0;

#ifndef DIVOOM_TIMES_VIBE
	if (status == DIVOOM_SD_CARD_STATE_MOUNTED)
	{
		/* 插入TF卡：切换到TF卡模式 */
		divoom_music_ctrl_set_mode(DIVOOM_MUSIC_CTRL_MODE_LOCAL);
	}
	else
	{
		/* 拔出TF卡 */
		if (divoom_music_ctrl_get_mode() == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
		{
			divoom_music_ctrl_set_next_mode();
		}
	}
#endif
	/* TimesVibe：TF 插拔切模式由 JL 上报 0x06/divoom_main_tf_det_cb + 0x39 驱动，T113 仅 follow */
}

#ifdef DIVOOM_TIMES_VIBE
static int divoom_main_query_btcore_insert_status(uint8 cmd, uint8 *out_status)
{
	uint8 reply = 0;
	uint16_t reply_len = 0;
	int ret;

	if (!out_status || !divoom_btcore_uart_cmd_is_ready())
		return -1;

	ret = divoom_btcore_uart_cmd_send_wait(cmd, NULL, 0, &reply, 1, &reply_len);
	if (ret != 0 || reply_len < 1u)
		return -1;

	*out_status = (reply != 0) ? 1 : 0;
	return 0;
}

static void divoom_main_sync_btcore_insert_status(void)
{
	uint8 status;

	if (pDivoomHandle == NULL)
		return;

	if (divoom_btcore_uart_cmd_send_check_link() != 0)
		divoom_print("btcore startup: check link failed, continue sync\n");
	/* 杰理版本由 BOOT_DONE(0x5f) 后主动上报 0x50 缓存，此处不再主动查询 */
	if (divoom_main_query_btcore_insert_status(DM_BTCORE_SD_INSERT_STATUS, &status) == 0)
	{
		pDivoomHandle->audio_det_info.tf_statu = status;
		divoom_print("btcore tf insert: %u\n", (unsigned)status);
	}
	else
	{
		divoom_print("btcore tf insert query failed\n");
	}

	if (divoom_main_query_btcore_insert_status(DM_BTCORE_LINEIN_INSERT_STATUS, &status) == 0)
	{
		pDivoomHandle->audio_det_info.linein_statu = status;
		divoom_print("btcore linein insert: %u\n", (unsigned)status);
	}
	else
	{
		divoom_print("btcore linein insert query failed\n");
	}

#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
	if (divoom_main_query_btcore_insert_status(DM_BTCORE_UAC_INSERT_STATUS, &status) == 0)
	{
		pDivoomHandle->audio_det_info.uac_statu = status;
		divoom_print("btcore uac insert: %u\n", (unsigned)status);
	}
	else
	{
		divoom_print("btcore uac insert query failed\n");
	}
#endif

	if (divoom_main_query_btcore_insert_status(DM_BTCORE_SERVICE_A2DP_STATUS, &status) == 0)
	{
		/* 0x12=0 只说明 A2DP 通道未开，不能覆盖 0x49 已置上的 ACL 已连接 */
		if (status != 0 || !divoom_bt_music_is_connected())
			divoom_bt_music_handle_connect_status(status);
		divoom_print("btcore a2dp status: %u\n", (unsigned)status);
	}
	else
	{
		divoom_print("btcore a2dp status query failed\n");
	}
}
#endif

void divoom_main_tf_det_cb(int inserted)
{
	static uint8 s_tf_first_statu_seen;
	static uint8 s_tf_suppress_until_unplug; /* 插着开机：拔卡前不弹插入提示 */
	static uint8 s_tf_insert_tip_shown;
	static uint8 s_tf_prev_inserted;

	divoom_print("wft test tf inserted: %d\n", inserted);
	if (pDivoomHandle == NULL)
		return;

	if (divoom_product_test_is_active()) {
		pDivoomHandle->audio_det_info.tf_statu = inserted;
		s_tf_prev_inserted = inserted;
		return;
	}

	if (!s_tf_first_statu_seen) {
		s_tf_first_statu_seen = 1;
		if (inserted)
			s_tf_suppress_until_unplug = 1;
	}

	pDivoomHandle->audio_det_info.tf_statu = inserted;

	if (!inserted) {
		s_tf_suppress_until_unplug = 0;
		s_tf_insert_tip_shown = 0;
		if (s_tf_prev_inserted)
			divoom_lvgl_top_audio_tip_menu_init(DIVOOM_AUDIO_TIP_TF_REMOVED);
		divoom_print("wft test tf removed\n");
		divoom_music_ctrl_on_tf_hotplug(0);
		divoom_tf_music_on_tf_hotplug(0);
	} else {
		if (!s_tf_prev_inserted) {
			if (!s_tf_suppress_until_unplug && !s_tf_insert_tip_shown) {
				divoom_lvgl_top_audio_tip_menu_init(DIVOOM_AUDIO_TIP_TF_INSERTED);
				s_tf_insert_tip_shown = 1;
			}
#ifdef DIVOOM_TIMES_VIBE
			if (!s_tf_suppress_until_unplug) {
				divoom_music_ctrl_on_tf_hotplug(1);
				divoom_tf_music_on_tf_hotplug(1);
#if !DIVOOM_SHOW_TF_NAND
				divoom_music_ctrl_sync_mode_from_jl();
#endif
			}
#endif
		}
	}

	s_tf_prev_inserted = inserted;
}

void divoom_main_linein_det_cb(int inserted)
{
	static uint8 s_linein_first_statu_seen;
	static uint8 s_linein_suppress_until_unplug; /* 插着开机：拔线前不弹插入提示 */
	static uint8 s_linein_insert_tip_shown;
	static uint8 s_linein_prev_inserted;

#ifndef DIVOOM_TIMES_VIBE
	inserted = inserted ? 0 : 1;  /* 本板极性：回调 0=插入 1=拔出，取反为 1=插入 0=拔出 */
#endif
	divoom_print("wft test linein inserted: %d\n", inserted);
	if (pDivoomHandle == NULL)
		return;

	if (divoom_product_test_is_active()) {
		pDivoomHandle->audio_det_info.linein_statu = inserted;
		s_linein_prev_inserted = inserted;
		return;
	}

	if (!s_linein_first_statu_seen) {
		s_linein_first_statu_seen = 1;
		if (inserted)
			s_linein_suppress_until_unplug = 1;
	}

	pDivoomHandle->audio_det_info.linein_statu = inserted;

	if (!inserted) {
		s_linein_suppress_until_unplug = 0;
		s_linein_insert_tip_shown = 0;
		if (s_linein_prev_inserted) {
			divoom_lvgl_top_audio_tip_menu_init(DIVOOM_AUDIO_TIP_AUX_REMOVED);
			if (divoom_music_ctrl_get_mode() == DIVOOM_MUSIC_CTRL_MODE_LINEIN)
				divoom_music_ctrl_set_next_mode();
			else
				divoom_main_disp_sd_out();
		}
	} else if (!s_linein_prev_inserted) {
		if (!s_linein_suppress_until_unplug && !s_linein_insert_tip_shown) {
			divoom_lvgl_top_audio_tip_menu_init(DIVOOM_AUDIO_TIP_AUX_INSERTED);
			s_linein_insert_tip_shown = 1;
			divoom_music_ctrl_set_mode(DIVOOM_MUSIC_CTRL_MODE_LINEIN);
		}
	}

	s_linein_prev_inserted = inserted;
}

static uint8 s_uac_first_statu_seen;
static uint8 s_uac_suppress_until_unplug; /* 插着开机：拔线前不弹 UAC 插入提示 */
static uint8 s_uac_insert_tip_shown;
static uint8 s_uac_prev_inserted;

static void divoom_main_uac_try_show_insert(void)
{
	if (divoom_product_test_is_active())
		return;
	if (s_uac_suppress_until_unplug || s_uac_insert_tip_shown)
		return;
	divoom_lvgl_top_audio_tip_menu_init(DIVOOM_AUDIO_TIP_UAC_INSERTED);
	s_uac_insert_tip_shown = 1;
	divoom_music_ctrl_set_mode(DIVOOM_MUSIC_CTRL_MODE_UAC);
}

/* USB 接 PC 时内核会发 USB_STATE=CONFIGURED，接充电器不会；据此判断是否进入 UAC。插入后延时轮询 state，检测到 CONFIGURED 则视为接 PC。 */
static void *divoom_main_usb_configured_check_task(void *arg)
{
	int i;
	(void)arg;

	if (pDivoomHandle == NULL)
		return NULL;
	divoom_system_delay_ms(400);
	for (i = 0; i < 8; i++) 
	{
		if (divoom_usb_hotplug_get_state() != 1) /* 已拔出 */
			break;
		if (divoom_usb_is_configured()) 
		{
			pDivoomHandle->audio_det_info.uac_statu = 1;
			divoom_print("usb connected to PC (CONFIGURED), switch to UAC\n");
			divoom_main_uac_try_show_insert();
			return NULL;
		}
		divoom_system_delay_ms(300);
	}
	/* 超时或中途拔出：视为仅充电，不切 UAC */
	pDivoomHandle->audio_det_info.uac_statu = 0;
	divoom_print("usb not configured (charger only), uac_statu=0\n");
	return NULL;
}

/* 与 TF 卡一致：接 PC 时切到 UAC；拔出时若当前为 UAC 则切下一模式，否则仅提示。
 * 开机时会上报一次当前状态，仅更新状态，不弹提示、不切模式、不启动 CONFIGURED 检测。 */
void divoom_main_usb_det_cb(int inserted)
{
#ifdef DIVOOM_TIMES_VIBE
	divoom_print("wft test usb inserted: %d\n", inserted);
	if (pDivoomHandle == NULL)
		return;

	if (divoom_product_test_is_active()) {
		pDivoomHandle->audio_det_info.uac_statu = inserted ? 1 : 0;
		s_uac_prev_inserted = inserted ? 1 : 0;
		return;
	}

	if (!s_uac_first_statu_seen) {
		s_uac_first_statu_seen = 1;
		if (inserted)
			s_uac_suppress_until_unplug = 1;
	}

	pDivoomHandle->audio_det_info.uac_statu = inserted ? 1 : 0;

	if (!inserted) {
		s_uac_suppress_until_unplug = 0;
		s_uac_insert_tip_shown = 0;
		if (s_uac_prev_inserted) {
			if (divoom_music_ctrl_get_mode() == DIVOOM_MUSIC_CTRL_MODE_UAC)
				divoom_music_ctrl_set_next_mode();
			divoom_lvgl_top_audio_tip_menu_init(DIVOOM_AUDIO_TIP_UAC_REMOVED);
		}
	} else if (!s_uac_prev_inserted) {
		divoom_main_uac_try_show_insert();
	}

	s_uac_prev_inserted = inserted;
	return;
#else
	divoom_print("wft test usb inserted: %d\n", inserted);
	if (pDivoomHandle == NULL)
		return;

	if (divoom_product_test_is_active()) {
		pDivoomHandle->audio_det_info.uac_statu = inserted ? 1 : 0;
		s_uac_prev_inserted = inserted ? 1 : 0;
		return;
	}

	if (!s_uac_first_statu_seen) {
		s_uac_first_statu_seen = 1;
		if (inserted)
			s_uac_suppress_until_unplug = 1;
	}

	pDivoomHandle->audio_det_info.uac_statu = 0;  /* 插入时先置 0，CONFIGURED 后再置 1 */

	if (!inserted) {
		s_uac_suppress_until_unplug = 0;
		s_uac_insert_tip_shown = 0;
		if (s_uac_prev_inserted) {
			if (divoom_music_ctrl_get_mode() == DIVOOM_MUSIC_CTRL_MODE_UAC)
				divoom_music_ctrl_set_next_mode();
			divoom_lvgl_top_audio_tip_menu_init(DIVOOM_AUDIO_TIP_UAC_REMOVED);
		}
		s_uac_prev_inserted = 0;
		return;
	}

	if (!s_uac_prev_inserted)
		divoom_system_create_task(divoom_main_usb_configured_check_task, "usb_check", NULL, DIVOOM_TASK_STACK_MAIN_USB_CHECK);
	s_uac_prev_inserted = inserted;
#endif
}


//返回系统是否完成启动

int divoom_main_get_finish_start_up(void)
{
    return pDivoomHandle ? pDivoomHandle->sys_finish_init_ok : 0;
}

int divoom_main_is_btcore_poweroff_ready(void)
{
	if (!pDivoomHandle)
		return 0;

	/* 产测：须已发 0x5f（通常等开机电源键松手），与 JL boot_done 门禁对齐 */
	if (divoom_product_test_is_active()) {
		if (!divoom_btcore_uart_cmd_is_boot_done_notified())
			return 0;
		return 1;
	}

	/* 开机动画未完成时勿处理 0x19：prepare 已跑但提示音被 skip，回调不到 finalize/硬件关机 */
	if (divoom_lvgl_poweron_should_defer_connect_tone())
		return 0;

	return 1;
}


void divoom_main_print_ram(char *tag, int line)
{
    printf("%s:%d ram:%d\n", tag, line , divoom_main_get_ram_free_size());
}
static void divoom_main_check_storage_on_boot(void)
{
    uint32 free_space = (uint32)divoom_port_fs_free_space(DIVOOM_USER_ROOT);

    if(free_space < DIVOOM_ROOM_WARNING_MIN)
    {
        divoom_clock_manage_clear_all_removed_clock_data();
        free_space = (uint32)divoom_port_fs_free_space(DIVOOM_USER_ROOT);
        if(free_space <= DIVOOM_ROOM_WARNING_MIN)
        {
            divoom_hot_manage_clear_files_on_boot();
            free_space = (uint32)divoom_port_fs_free_space(DIVOOM_USER_ROOT);
        }
    }
    divoom_main_update_storage_status(free_space);
}

void divoom_main(void)
{
    uint8 mac[6];
    //char device_id[32];
    int work_mode = 0, first_flag = 0;
	int poweron_mode = 0;
    //divoom_system_setting_spirit_conf spirit_conf;

#if DIVOOM_BUILD_RTOS_DEVICE
    divoom_system_err_log = divoom_boot_health_restart_count();
#endif

#ifdef DIVOOM_SIMULATOR
    divoom_log_start();
#endif
    pthread_t tid = pthread_self();

    first_flag = divoom_main_ensure_userdata_seed();//把从只读盘中读取数据到读写盘
    divoom_main_ensure_userdata_dirs();

    divoom_check_system_lock_init();
    divoom_print("**************main thread:%p***********************\n", (void *)tid);
    divoom_main_read_boot_ab_recovery();

    divoom_print("***************************************************\n");
    divoom_print("***************************************************\n");
    divoom_print("********divoom version: %d; custom id: %d******\n", DIVOOM_SOFTWARE_VERSION, DIVOOM_CUSTOM_ID);
    divoom_print("***Date: %s, Time: %s, Timestamp: %s ***\n", __DATE__,__TIME__,__TIMESTAMP__); //显示编译日期
    divoom_print("********************divoom_system_err_log:%d*******************************\n", divoom_system_err_log);
    divoom_print("***************************************************\n");
    divoom_system_init_system_time();
    divoom_memory_pool_init(DIVOOM_RAM_POOL_TOTAL_RAM_NUM * DIVOOM_RAM_POOL_BLOCK_RAM_SIZE, DIVOOM_RAM_POOL_BLOCK_RAM_SIZE,DIVOOM_RAM_POOL_TOTAL_MAX_RAM_NUM * DIVOOM_RAM_POOL_BLOCK_MAX_RAM_SIZE, DIVOOM_RAM_POOL_BLOCK_MAX_RAM_SIZE);

#ifndef DIVOOM_SIMULATOR
#ifdef DIVOOM_TIMES_VIBE
    if (!divoom_btcore_uart_cmd_is_ready()) {
        divoom_btcore_uart_cmd_init(NULL);
    }
#endif
#endif
    //sys_reset_status =  || (rtc_get_reset_reason(0) == 12) || (rtc_get_reset_reason(1) == 12);
    //sys_reset_status = 0;
	//这里是开机检测，先注释掉，暂时用不上
	poweron_mode = divoom_onoff_check_poweron();
#if DIVOOM_HAVE_LIBCURL
    curl_global_init(CURL_GLOBAL_ALL);
#endif
    //divoom_main_test_url_request();
    //return;
    //divoom_main_test_video();
    for(int i = 1; i < DIVOOM_MANAGE_IMAGE_END_ID; i++)
    {
        char file_name[128];
        divoom_manage_image_download_get_path_name(1, 0, file_name);
        divoom_app_com_mk_dir(file_name);
    }
    int database_ready = divoom_database_manage_init(DIVOOM_THUMB_DATA_BASE_FILE_NAME);
#if DIVOOM_BUILD_RTOS_DEVICE && defined(CONFIG_DIVOOM_SQLITE) && CONFIG_DIVOOM_SQLITE
    if (!database_ready) {
        divoom_print("[DB] initialization failed after recovery; rebooting\n");
        divoom_system_reboot();
        return;
    }
#else
    (void)database_ready;
#endif
    divoom_local_music_ctrl_music_init();
    divoom_image_font_cache_init();
	pDivoomHandle = (Pdivoom_sys_ctrl_handle)divoom_malloc(sizeof(divoom_sys_ctrl_handle), DIVOOM_MODEL_MAIN, __LINE__, __FILE__);

	memset(pDivoomHandle, 0, sizeof(divoom_sys_ctrl_handle));
    pDivoomHandle->net_lyric_region_off = 0xFF;
    pDivoomHandle->tmp_file_lock = divoom_system_create_lock();
    pDivoomHandle->tmp_file_index = 1234567;
    pDivoomHandle->last_download_file_time = -1;
    pDivoomHandle->system_err_log = divoom_system_err_log;
	pDivoomHandle->poweron_mode = poweron_mode;
    pDivoomHandle->app_list = divoom_system_malloc(sizeof(divoom_device_app_id_list));
    pDivoomHandle->app_list->num = 0;
	pDivoomHandle->app_list_lock = divoom_system_create_lock();
    pDivoomHandle->wifi_err_status = 0;
    pDivoomHandle->back_lang = -1;
    //pDivoomHandle->charge_status = DIVOOM_IO_UNACTIVE;
	//上电默认开机状态
	pDivoomHandle->master_flag = 1;
    pDivoomHandle->back_light_off = 1;
    pDivoomHandle->sys_setting.server_flag = 2; //默认国外服务器
    pDivoomHandle->weather_info.request_weather_flag = (1 << DIVOOM_DISP_WEATHER_TODAY_INFO) | (1 << DIVOOM_DISP_WEATHER_FURTHER_INFO);
    //divoom_main_test_strech();
    pDivoomHandle->pupdate_info = divoom_system_malloc(sizeof(divoom_general_json_get_update_file_info_respone));
    Pdivoom_general_json_get_update_file_info_respone pupdate_info = (Pdivoom_general_json_get_update_file_info_respone)pDivoomHandle->pupdate_info;
    pupdate_info->version = DIVOOM_SOFTWARE_VERSION;
    pDivoomHandle->pjl_update_info = divoom_system_malloc(sizeof(divoom_general_json_get_update_file_info_respone));
    if (pDivoomHandle->pjl_update_info)
    {
	    Pdivoom_general_json_get_update_file_info_respone pjl_info = (Pdivoom_general_json_get_update_file_info_respone)pDivoomHandle->pjl_update_info;
	    pjl_info->version = divoom_ota_jl_get_local_version();
    }
    s_jl_get_update_pending = 0;
    //divoom_main_test_strech();
    if(poweron_mode == DIVOOM_ONOFF_MODE_FORMAT_CONFIG)
    {
        divoom_sys_setting_info sys_setting = {0};
        
    	divoom_app_schedule_init();
    	divoom_sys_model_crl_init();
    	divoom_microtask_init();
        divoom_decode_pic_init();
        sys_setting.brightness = 100;
	    divoom_disp_ctrl_app_init(&sys_setting);
        divoom_screen_bright(100);
        pDivoomHandle->key_queue = divoom_queue_init(1000, __func__, __LINE__);
	    //divoom_keypad_set_msgsend_func(divoom_main_format_key_info);        
        divoom_system_create_task(divoom_main_format_loop, "format_loop", NULL, DIVOOM_TASK_STACK_MAIN_FORMAT); 
        return;
        
    }

	divoom_app_schedule_init();
	divoom_sys_model_crl_init();
    divoom_sys_model_crl_register(DIVOOM_MODEL_MAIN,divoom_main_model_event);
	divoom_sleep_aid_ctrl_init();
	divoom_leave_msg_ctrl_init();
    divoom_cache_clock_data_init();
    divoom_software_timer_init();
    divoom_system_msg_notice_init();

//	divoom_image_download_manage_init();
//	divoom_net_image_cache_init();
	divoom_main_load_config();
    if(first_flag == 1)
    {
        pDivoomHandle->sys_setting.reset_flag = 1;
        divoom_main_save_config();
    }
    divoom_app_config_wifi_init();
	divoom_power_on_manage_init();
    divoom_update_system_file_init();
    work_mode = divoom_power_on_manage_get_power_mode();

	/*
	 * 须先于 divoom_music_ctrl_init：后者会 divoom_bt_music_set_play_vol → divoom_main_set_sys_vol，
	 * 且 BT 连接事件里会立刻播提示音并 set_sys_vol。若 hub/loopback/softvol 尚未由 init_sys_vol 配好，
	 * 刚开机第一次 amixer 往往无效，提示音会特别大或不受控。
	 * 500 系麦克风采集须在 hub/loopback 配置完成后再 divoom_pcm_init。
	 */
#ifdef DIVOOM_TIME_FRAME_SPEAKER
	divoom_main_init_sys_vol();
	if (divoom_pcm_init(32000) != 0)
	{
		divoom_print("divoom_main: divoom_pcm_init failed\n");
	}
#else
    divoom_pcm_init(32000);
	divoom_main_init_sys_vol();
#endif

#ifdef DIVOOM_TIME_FRAME_SPEAKER
#ifndef DIVOOM_TIMES_VIBE
#ifndef DIVOOM_AMP_ACM8625P
	divoom_acm8625_init();
#else
	divoom_acm8625p_amp_init();
#endif
	divoom_linein_hotplug_init(divoom_main_linein_det_cb);
	divoom_usb_hotplug_init(divoom_main_usb_det_cb);
	divoom_tf_hotplug_init(divoom_main_tf_det_cb);
	PowerMonitorInit();
#else
	divoom_main_sync_btcore_insert_status();
	divoom_tf_music_bootstrap_at_boot();
	divoom_power_monitor_boot_guard_init();
#endif
	divoom_music_ctrl_init();
	divoom_lyric_play_init();
	divoom_white_noise_player_init();
#endif

#if defined(DIVOOM_SIMULATOR) || defined(_WIN32)
	/* AstroToo 真机由 JL 协处理器管理音乐子系统（未定义 DIVOOM_TIME_FRAME_SPEAKER，
	 * 上面的初始化被跳过）。仿真下显式初始化音乐控制，使音源各子页有可控的播放状态、
	 * 并让蓝牙预置示例歌曲信息可被读取。UAC/AUX/TF 默认视为已插入，便于验收子页。 */
	divoom_music_ctrl_init();
	divoom_tf_music_bootstrap_at_boot();
	if (pDivoomHandle != NULL)
	{
		pDivoomHandle->audio_det_info.uac_statu = 1;
		pDivoomHandle->audio_det_info.linein_statu = 1;
		pDivoomHandle->audio_det_info.tf_statu = 1;
	}
#endif

    divoom_main_load_timezone_config();
    //divoom_led_control_init();
    if(pDivoomHandle->sys_setting.server_flag)
    {
        pDivoomHandle->sys_setting.server_flag = pDivoomHandle->sys_setting.server_flag - 1;
        if(pDivoomHandle->sys_setting.server_flag != 0 || pDivoomHandle->sys_setting.server_flag != 1)
        {
            pDivoomHandle->sys_setting.server_flag = 2;
        }
    }
	//蓝牙初始化
    divoom_minilzo_init();
	divoom_microtask_init();
    divoom_decode_pic_init();
    divoom_save_all_config_init();
	divoom_mic_ctrl_init();
	divoom_eq_noise_mic_fft_init();
    divoom_lvgl_top_control_init();
    //divoom_user_define_schedule_init();
    divoom_pic_phone_manage_init(DIVOOM_USER_DATA_DIR, 0);
    //设置时区信息
    //rm /etc/localtime
    //ln -s /usr/share/zoneinfo/Asia/Shanghai /etc/localtime
    //divoom_clock_schedule_init();
//    divoom_mix_screen_init();
    //divoom_leave_msg_init();
	//divoom_spi_init();	//初始化spi,图片数据通过spi传到gd
	//divoom_screen_handle_init();
//    divoom_image_disp_init();
	//divoom_local_music_init();
	//下载模块
	pDivoomHandle->bond_flag = 0;
	divoom_web_get_init();
	divoom_disp_ctrl_app_init(&pDivoomHandle->sys_setting);
	//divoom_sd_init(divoom_main_sd_mount_ok_sb_func, 0);
    divoom_gif_ctrl_init();
    divoom_user_head_manage_init();
    /* Publish clock dependencies before the light model can consume boot events. */
    if(!divoom_clock_manage_init())
    {
        divoom_print("[CLOCK] initialization failed, rebooting\n");
        divoom_system_reboot();
        return;
    }
	divoom_light_app_init(divoom_system_err_log);
	divoom_clock_favorite_init();
#ifdef DIVOOM_TIME_FRAME_SPEAKER
	/* 仅本次有效的 RTC 唤醒可亮屏；JL 的开机来源在主控重启后可能仍是旧值。 */
	if (poweron_mode == DIVOOM_ONOFF_MODE_CLOCK
		|| poweron_mode == DIVOOM_ONOFF_MODE_POWER_SCHEDULE)
	{
		divoom_light_app_get_screen_status(1);
		/* 唤醒后同步保存，后续升级/异常重启才能恢复当前亮屏状态。 */
		divoom_light_app_load_save();
	}
#endif
    divoom_user_head_ctrl_init();
    divoom_commit_clock_config_init();
	divoom_component_manage_init();
    divoom_charge_init();
    divoom_light_music_global_init();
    divoom_menu_control_init();
#ifndef DIVOOM_TIME_FRAME_SPEAKER
	divoom_disp_ambient_ctrl_init(divoom_light_app_get_screen_status(0xFF));
#endif
    divoom_custom_photo_init();
	divoom_clock_image_manage_init();

    //先显示开机画面，再做其它模块初始化
    if(divoom_light_app_get_screen_status(0xFF))
    {
        divoom_main_apply_sys_light();
    }
    else
    {
        divoom_screen_bright(0);
    }
    divoom_screen_set_mirror_image(pDivoomHandle->sys_setting.mirror_flag);
	divoom_main_set_color_temp_inner(pDivoomHandle->sys_setting.color_temp);
	if(DIVOOM_ONOFF_MODE_TEST != poweron_mode)
	{
        int cur_menu_id = divoom_menu_control_get_active_menu_id();
        if(cur_menu_id == DIVOOM_MENU_CONTROL_ID_CONFIG_WIFI || cur_menu_id == DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_LANGUGE)
        {
        	divoom_app_schedule_item_info push_info = {0};

        	push_info.event_id = DIVOOM_MAIN_PLAY_WIFI_CONFIG_EVENT;
        	push_info.model_id = DIVOOM_MODEL_MAIN;
            push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
        	divoom_app_schedule_push(&push_info);
        }
	}

    if (!divoom_light_app_get_screen_status(0xFF))
        divoom_btcore_key_light_follow_screen(0);
    divoom_btcore_key_light_set_full(pDivoomHandle->sys_setting.key_light_mode, 
        pDivoomHandle->sys_setting.key_light_on_off, pDivoomHandle->sys_setting.key_light_brightness);
    divoom_btcore_uart_cmd_apply_sound_effect(pDivoomHandle->sys_setting.audio_effect);
    
    #if 0
    if(DIVOOM_POWER_ON_WIFI_MODE == work_mode)
    {
        if(pDivoomHandle->ext_info.ext_str.power_down_flag == 0)
        {
            
            pDivoomHandle->ext_info.ext_str.power_down_flag = 1;
        }
        else
        {
            pDivoomHandle->ext_info.ext_str.power_down_flag = 0;
        }
        pDivoomHandle->update_flag = 1;
        divoom_main_save_config_local();
    }
    else
    {
        //如果在配网模式，就不显示
        if(pDivoomHandle->ext_info.ext_str.power_down_flag == 1)
        {
            pDivoomHandle->ext_info.ext_str.power_down_flag = 0;
        }
        pDivoomHandle->update_flag = 1;
        divoom_main_save_config_local();
    }
    #endif
    
//    divoom_main_check_power_pic();
	divoom_ota_server_init();
    
    
	divoom_manage_image_init();

    /*
    if(strlen(pDivoomHandle->sys_setting.power_on_id))
    {
        divoom_manage_image_item_info pimage_info = {0};

        strcpy(pimage_info.file_addr, pDivoomHandle->sys_setting.power_on_id);
        pimage_info.flag = 1;
        pimage_info.image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_POWER_ON;
        pimage_info.pcallback = divoom_app_com_finishcallback;      //不关心ＦＯＮＴ，只是提高优先级
        divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_PowerOnGif, &pimage_info, 1); 
    } 
    */
    divoom_local_use_pic_load();
    divoom_system_font_init();
//    divoom_app_pic_manage_init();
    divoom_download_aliyun_file_init();
	divoom_disp_clock_global_init();
    divoom_clock_control_global_init();
    divoom_vision_ctrl_init();
    if(pDivoomHandle->custom_product_flag)
    {
	    divoom_cloud_photo_ctrl_init();
    }


    //divoom_disp_ctrl_set_bright(pDivoomHandle->sys_setting.brightness);
#ifdef DIVOOM_ONOFF_NO_POWER_CONTROL
    divoom_print("divoom_main:  work_mode: %d: poweron_mode: %d\n", work_mode, poweron_mode);
	//要求是开机检查有没有sd卡,有没有相关的自定义文件,有的话切换到自定义模式;没有的话就闪WiFi图标,结束之后进行时间显示
	if(DIVOOM_POWER_ON_WIFI_MODE == work_mode)
	{
		//开机显示流程,显示开机画面,退出开机画面之后,开始进入默认模式(达人订阅模式),等到收到sd卡初始化OK之后,检查是否有sd本地显示,有的话进入sd卡本地显示
		//divoom_main_disp_power_on();
	}
	else
	{
		//如果没连接WiFi就去显示WiFi没连接,有sd卡本地文件,就显示sd卡本地显示
		divoom_main_disp_wifi_config(10000);
	}
#else
    divoom_print("divoom_main: poweron_mode: %d; work_mode: %d:\n",poweron_mode, work_mode);

	//alarm需要用别的处理了
		divoom_alarm_init(poweron_mode);
		//divoom_app_file_manage_init();
		//divoom_sleep_init();
		divoom_time_plan_init();
        divoom_clock_play_plan_init();
        divoom_hot_manage_init();
        divoom_expert_manage_init();
        divoom_favorite_manage_init(mac);
        divoom_album_manage_init();
        divoom_eq_image_manage_init();
        //divoom_clock_manage_check_user_pic_config();
        divoom_google_calendar_init(NULL);
        divoom_outlook_calendar_init();
        divoom_user_define_ctrl_init(mac);
        /* 有效索引直接复用，只有首次升级或文件损坏时才全量自愈。 */
        (void)divoom_clock_component_index_bootstrap();
        //divoom_exchange_pic_init();
		divoom_app_tool_init();
        divoom_subscribe_manage_init();
        divoom_main_check_storage_on_boot();
		divoom_tomato_time_init();
		divoom_power_control_init(1);
		//divoom_white_noise_init();
		divoom_anniversary_clock_reminder_init();
		divoom_social_clock_reminder_init();
        #ifdef DIVOOM_SIMULATOR
        if(pDivoomHandle->sys_setting.log_level)
        {
            divoom_log_start();
        }
        #endif
		divoom_net_app_init();
		divoom_ha_init();
#ifdef DIVOOM_TIME_FRAME_SPEAKER
		if (poweron_mode == DIVOOM_ONOFF_MODE_POWER_SCHEDULE)
			divoom_power_control_after_rtc_schedule_wakeup();
#endif
		if (work_mode != DIVOOM_POWER_ON_BLUE_MODE)
		{
			divoom_net_mqtt_manage_init(pDivoomHandle->device_info.device_id);
			divoom_net_http_manage_init(pDivoomHandle->device_info.device_id);
			divoom_ai_talk_init();
	        divoom_http_server_init();
		}
    pDivoomHandle->sys_finish_init_ok = 1;
#endif
    if(divoom_menu_control_menu_get_first_menu_id() == DIVOOM_MENU_CONTROL_ID_MAIN && divoom_system_err_log)
    {
        divoom_light_app_system_reset_up();
        divoom_bt_music_play_deferred_connect_tone_if_any();
    }
    divoom_system_create_task(divoom_main_loop, "main_loop", NULL, DIVOOM_TASK_STACK_MAIN_LOOP);
    //divoom_system_create_task_pro(divoom_main_dbg, "divoom_main", NULL, 8*1024, DIVOOM_SYSTEM_TASK_DEFAULT_PRIO, 0, 0); 
    //divoom_main_loop(NULL);
}

//清除服务器心跳包的时间，只要收到命令就认为有心跳

void divoom_main_reset_check_mqtt_server_heart(void)
{
    pDivoomHandle->divoom_srv_respone = 1;
    pDivoomHandle->check_server_heart_time = time(0);
}

//将检查设备与服务器的连接，如果多次连续请求失败，将重启设备
void divoom_main_check_mqtt_server(time_t cur_time)
{
    static int fails_request_time = 0;
	divoom_net_command *pcmd_info;
    
    if(pDivoomHandle->divoom_connect_srv && pDivoomHandle->divoom_srv_respone && pDivoomHandle->check_server_heart_time)
    {
        //十分钟检测一次，或者，如果失败了连续检测时间为３Ｓ
        
        if((cur_time > pDivoomHandle->check_server_heart_time + 120) || ((cur_time >pDivoomHandle->check_server_heart_time + 3) && fails_request_time))
        {            
            divoom_print("we will send hearbeat to server!: %d\n", fails_request_time);
            pcmd_info = divoom_net_app_send_packet_wait(DIVOOM_NET_COMM_DEVICE_APP_HEARBEAT, DIVOOM_SERVER_TOPIC, &fails_request_time);
            if(pcmd_info == NULL)
            {
                fails_request_time ++;
                if(fails_request_time > 2)
                {
                    divoom_sys_msg_info model_msg = {0};
            		model_msg.msg_type = 1;
            		model_msg.src_id = DIVOOM_MODEL_MAIN;
            		model_msg.dest_id = DIVOOM_MODEL_SER_CON;		   
            		model_msg.msg_id = DIVOOM_SERVER_MSG_RESET_DIVOOM; 
            		divoom_sys_model_crl_msg_push(&model_msg);
                    fails_request_time = 0;
                }
                divoom_print("it fails to request server, and will reset: %d!\n", fails_request_time);
            }
            else
            {
                fails_request_time = 0;
                divoom_net_app_free_packet(pcmd_info);
            }
            pDivoomHandle->check_server_heart_time = cur_time;
        }

    }
    else
    {
        fails_request_time = 0;
    }
}
//取得设备最新版本
void divoom_main_check_deivce_update_version(void)
{
    
    divoom_general_json_get_update_file_info_request device_info;
    
    device_info.hardware = DIVOOM_HARDWARE_VERSION;
    device_info.test_flag = 0;
    divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_UPDATE_FILE_INFO, DIVOOM_SERVER_TOPIC, &device_info);
}



void divoom_main_check_jl_update_version(void)
{
	divoom_general_json_get_update_file_info_request device_info;
    
	device_info.hardware = DIVOOM_JL_HARDWARE_VERSION;
	device_info.test_flag = 0;
	divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_UPDATE_FILE_INFO, DIVOOM_SERVER_TOPIC, &device_info);
}


#ifdef DIVOOM_SIMULATOR

int divoom_ota_jl_version_is_valid(uint32_t jl_version)
{
	return 1;
}
#endif

#if defined(DIVOOM_TIMES_VIBE) && !defined(DIVOOM_PRODUCT_ASTRO_TOO)
int divoom_main_is_jl_get_update_pending(void)
{
	uint8 pending = s_jl_get_update_pending;

	s_jl_get_update_pending = 0;
	return pending ? 1 : 0;
}

void *divoom_main_set_jl_device_update_info(void *pupdate_info)
{
	if (pDivoomHandle == NULL || pupdate_info == NULL || pDivoomHandle->pjl_update_info == NULL)
		return NULL;
	memcpy(pDivoomHandle->pjl_update_info, pupdate_info,
	       sizeof(divoom_general_json_get_update_file_info_respone));
	return pDivoomHandle->pjl_update_info;
}
void divoom_main_check_jl_update_version(void)
{
	divoom_general_json_get_update_file_info_request device_info;
	uint32_t jl_ver = divoom_ota_jl_get_local_version();
    #ifndef DIVOOM_SIMULATOR

	if (!divoom_ota_jl_version_is_valid(jl_ver)) {
		if (divoom_btcore_uart_cmd_get_jl_device_version(&jl_ver) != 0 ||
		    !divoom_ota_jl_version_is_valid(jl_ver)) {
			divoom_print("check jl update: no valid jl version, skip\n");
			return;
		}
	}
    #endif

	device_info.hardware = (int)(jl_ver / 1000u);
	device_info.test_flag = 0;
	s_jl_get_update_pending = 1;
	divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_UPDATE_FILE_INFO, DIVOOM_SERVER_TOPIC, &device_info);
}

int divoom_main_user_start_jl_update_device(void)
{
	divoom_sys_msg_info model_msg = {0};
	Pdivoom_general_json_get_update_file_info_respone update_info;

	if (pDivoomHandle == NULL || pDivoomHandle->pjl_update_info == NULL)
		return -1;

	update_info = (Pdivoom_general_json_get_update_file_info_respone)
		divoom_malloc(sizeof(divoom_general_json_get_update_file_info_respone),
			      DIVOOM_MODEL_NET, __LINE__, __FILE__);
	if (!update_info)
		return -1;

	memcpy(update_info, pDivoomHandle->pjl_update_info,
	       sizeof(divoom_general_json_get_update_file_info_respone));
	model_msg.msg_type = 1;
	model_msg.dest_id = DIVOOM_MODEL_OTA;
	model_msg.msg_id = DIVOOM_OTA_SERVER_JL_SRV_UPDATE;
	model_msg.data = (uint8 *)update_info;
	divoom_sys_model_crl_msg_push(&model_msg);
	return 0;
}
#endif

//获取是否已配网标志
int divoom_main_get_wifi_config_flag(void)
{
	if(pDivoomHandle)
		return pDivoomHandle->sys_setting.wifi_config_flag;
	return 1;
}

void divoom_main_set_wifi_add_network_route_pending(uint8 pending)
{
    s_wifi_add_network_route_pending = pending ? 1 : 0;
}

#if DIVOOM_DIAL_SWITCH_AUTOTEST
static void divoom_main_dial_switch_autotest_poll(void)
{
    static uint8 wait_started;
    static uint8 switch_started;
    static uint32 next_tick;
    static uint32 send_count;
    uint32 now_tick;

    now_tick = divoom_system_get_ticks();
    if (!wait_started)
    {
        if (!divoom_main_get_wifi_con_status())
        {
            return;
        }
        wait_started = 1;
        next_tick = now_tick + DIVOOM_DIAL_SWITCH_AUTOTEST_START_DELAY_SEC;
        divoom_print("[dial_switch_test] wifi connected tick=%u, start after %u sec\n",
            now_tick, DIVOOM_DIAL_SWITCH_AUTOTEST_START_DELAY_SEC);
        return;
    }

    if ((int32)(now_tick - next_tick) < 0)
    {
        return;
    }

    if (!switch_started)
    {
        switch_started = 1;
        divoom_print("[dial_switch_test] begin tick=%u key=DOWN period=%u sec\n",
            now_tick, DIVOOM_DIAL_SWITCH_AUTOTEST_PERIOD_SEC);
    }

    send_count++;
    divoom_print("[dial_switch_test] play next clock count=%u tick=%u active_menu=%d\n",
        send_count, now_tick, divoom_menu_control_get_active_menu_id());
    divoom_light_app_play_next_clock(1);
    next_tick = now_tick + DIVOOM_DIAL_SWITCH_AUTOTEST_PERIOD_SEC;
}
#endif

void *divoom_main_loop(void * data)
{	
    static int reset_second;
    static int subscribe_reload_mday = -1;
    static int subscribe_reload_time;
    static uint8 subscribe_reload_flag;
    uint8  wifi_signal_weak_flag = 0;
    uint32 divoom_tick = 0, divoom_10s_tick = 0, wifi_weak_times =0, wifi_strong_times =0;
    time_t cur_time, sync_refresh_time = 0;
	struct tm cur_tm;    

	pDivoomHandle->request_update_time = rand() % 7000;
	while (1)
	{
        divoom_main_poll_startup_dial_event();
        divoom_main_poll_boot_leave_msg_notice();
#if DIVOOM_BUILD_RTOS_DEVICE
        if (pDivoomHandle->divoom_srv_respone)
            divoom_main_retry_boot_ab_recovery();
#endif
#if DIVOOM_DIAL_SWITCH_AUTOTEST
        divoom_main_dial_switch_autotest_poll();
#endif
        if((divoom_tick % 10)== 0 && pDivoomHandle->master_flag)
        {
            divoom_10s_tick ++;
            divoom_wifi_model_refresh_sta_link();
            //显示时间
            cur_time = time(0);
            if(pDivoomHandle->send_server_heart_time && pDivoomHandle->send_server_heart_time + 60 < cur_time)
            {
                divoom_print("send server heart !\n");
                divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_APP_HEARBEAT, DIVOOM_SERVER_TOPIC, NULL);
                pDivoomHandle->send_server_heart_time = cur_time;
            }
            if(pDivoomHandle->divoom_connect_srv == 1 && pDivoomHandle->connect_serve_time && pDivoomHandle->connect_serve_time + 10 < cur_time && pDivoomHandle->divoom_srv_respone == 0)
            {
            	divoom_sys_msg_info  msg = {0};
            	msg.msg_id = DIVOOM_DEVICE_WIFI_CON_ID;
            	msg.src_id = DIVOOM_MODEL_MAIN;
            	msg.dest_id = DIVOOM_MODEL_MAIN;
                pDivoomHandle->divoom_connect_srv = 0;
                divoom_sys_model_crl_msg_push(&msg);
            }
            if(sync_refresh_time == 0)
            {
                sync_refresh_time = cur_time;
            }
            if(cur_time > sync_refresh_time + 10)
            {
                //10秒定时刷新一下磁盘
                if(pDivoomHandle->last_download_file_time == -1 || cur_time > pDivoomHandle->last_download_file_time + 10)
                {
                    divoom_print("sync!\n");
#if defined(__linux__) || defined(__GLIBC__)
                    sync();
#endif
                    pDivoomHandle->last_download_file_time = -1;
                    sync_refresh_time = cur_time;
                }
            }
            #if 0
			if(DIVOOM_POWER_ON_WIFI_MODE == divoom_power_on_manage_get_power_mode())
			{
                if(pDivoomHandle->ext_info.ext_str.power_down_flag && pDivoomHandle->connect_serve_time && pDivoomHandle->connect_serve_time + 5 < cur_time)
                {
                    divoom_print("we will clear power down flag!\n");
                    pDivoomHandle->ext_info.ext_str.power_down_flag = 0;
                    pDivoomHandle->update_flag = 1;
                    divoom_main_save_config_local();
                }
			}
            else 
            {
                if(start_time + 30 < cur_time)
                {
                    divoom_print("we will clear power down flag!\n");
                    pDivoomHandle->ext_info.ext_str.power_down_flag = 0;
                    pDivoomHandle->update_flag = 1;
                    divoom_main_save_config_local();
                }
            }
            #endif
	        localtime_r((time_t *)&cur_time, &cur_tm);
            if(divoom_net_mqtt_get_net_status() && pDivoomHandle->divoom_connect_srv && pDivoomHandle->divoom_srv_respone)
            {
                divoom_main_update_china_data_info(&cur_tm);
            }
			divoom_time_plan_main();
			divoom_clock_play_plan_main();
			divoom_sleep_main();
        	divoom_anniversary_clock_reminder_main();
			divoom_lvgl_top_anniversary_reminder_menu_main_loop();
			divoom_lvgl_top_social_reminder_menu_main_loop();
		    divoom_clock_schedule_main();
            //divoom_user_define_schedule_main();
            if(divoom_10s_tick == 5)
            {
                int8 wifi_rssi;

                divoom_wifi_model_refresh_sta_rssi();
                if (divoom_wifi_model_get_sta_rssi(&wifi_rssi) == 0)
                {
                    pDivoomHandle->rssi = wifi_rssi;
                }
                else
                {
                    pDivoomHandle->rssi = 0;
                }

                int free_ram_size = divoom_main_get_ram_free_size();
				int voltage = 0;
#ifdef DIVOOM_TIME_FRAME_SPEAKER
#ifndef DIVOOM_TIMES_VIBE
				divoom_battery_voltage_get_mv(&voltage);
#endif
#endif
                divoom_10s_tick = 0;
                divoom_print_err("system: %d-%d-%d, %d:%d:%d; ram=%d, voltage = %d, size=%d\n",cur_tm.tm_year+1900,cur_tm.tm_mon + 1,cur_tm.tm_mday,cur_tm.tm_hour,cur_tm.tm_min,cur_tm.tm_sec, free_ram_size, voltage,(int)divoom_port_fs_free_space(DIVOOM_USER_ROOT));
                divoom_memory_pool_info();
            }
            
            if(pDivoomHandle->init_time_flag && pDivoomHandle->night_view.on_off == 1 && pDivoomHandle->on_off_config.work_status != 1)
            {
                uint16 cur_work_status = 0, cur_min;

                cur_min = cur_tm.tm_hour * 60 + cur_tm.tm_min;
                if(pDivoomHandle->night_view.start < pDivoomHandle->night_view.end)
                {
                    if(cur_min >= pDivoomHandle->night_view.start && pDivoomHandle->night_view.end > cur_min)
                    {
                        cur_work_status = 1;
                    }
                }
                else
                {
                    if(cur_min >= pDivoomHandle->night_view.start || cur_min < pDivoomHandle->night_view.end)
                    {
                        cur_work_status = 1;
                    }
                }
                //divoom_print(" night view info : %d, %d, %d, %d, %d!\n", cur_work_status, pDivoomHandle->night_view.work_status, cur_min ,pDivoomHandle->night_view.start ,pDivoomHandle->night_view.end);

                //工作状态不一样，将改变亮度
                if(cur_work_status != pDivoomHandle->night_view.work_status)
                {
                    //int brightness = pDivoomHandle->sys_setting.brightness;
                    pDivoomHandle->night_view.work_status = cur_work_status;
                    if(cur_work_status)
                    {
                        divoom_print("we will start night view!\n");
                        if(divoom_light_app_get_screen_status(0xFF))
                        {
                            //brightness = pDivoomHandle->night_view.bright;
                            //divoom_net_app_send_packet(DIVOOM_NET_COMM_SET_SYS_BRIGHTNESS, DIVOOM_SERVER_TOPIC, (void *)&brightness);
                            divoom_main_set_night_brightness(pDivoomHandle->night_view.bright);
                            //divoom_disp_ambient_ctrl_set_temp_brightness(pDivoomHandle->night_view.bright);
                        }
                    }
                    else
                    {
                        divoom_print("we will end night view!\n");
                        if(divoom_light_app_get_screen_status(0xFF))
                        {
                            //brightness = pDivoomHandle->sys_setting.brightness;
                            //divoom_net_app_send_packet(DIVOOM_NET_COMM_SET_SYS_BRIGHTNESS, DIVOOM_SERVER_TOPIC, (void *)&brightness);
                            divoom_main_clear_night_brightness();
                            divoom_main_apply_sys_light();
                            //divoom_disp_ambient_ctrl_set_brightness(NULL, 1);
                        }
                    }
                }
            }
            #if 0
            if(pDivoomHandle->init_time_flag && pDivoomHandle->on_off_config.on_off == 1)
            {
                int cur_work_status = 0, cur_min;

                cur_min = cur_tm.tm_hour * 60 + cur_tm.tm_min;
                if(pDivoomHandle->on_off_config.start < pDivoomHandle->on_off_config.end)
                {
                    if(cur_min >= pDivoomHandle->on_off_config.start && pDivoomHandle->on_off_config.end > cur_min)
                    {
                        cur_work_status = 1;
                    }
                }
                else
                {
                    if(cur_min >= pDivoomHandle->on_off_config.start || cur_min < pDivoomHandle->on_off_config.end)
                    {
                        cur_work_status = 1;
                    }
                }

                //工作状态不一样，将改变工作模式
                if(cur_work_status != pDivoomHandle->on_off_config.work_status)
                {
                    if(pDivoomHandle->on_off_config.work_status == 2)
                    {
                        if(cur_work_status == 0)
                        {
                            pDivoomHandle->on_off_config.work_status = 0;
                        }
                    }
                    else
                    {
                        divoom_sys_msg_info model_msg = {0};
                        
                        pDivoomHandle->on_off_config.work_status = cur_work_status;
                        //工作状态不一样，将改变亮度
                        divoom_print("we will control on off : %d!\n", cur_work_status);
                        cur_work_status = cur_work_status?0:1;  //工作时，发关屏命令，非工作时，发开屏命令
            			//model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
            			//model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SCREEN_CTRL;
                        //model_msg.data = (uint8 *)&cur_work_status;
            			//divoom_sys_model_crl_msg_push(&model_msg);

						//divoom_disp_ambient_ctrl_set_rgb_on_off((uint8)cur_work_status);

						if(cur_work_status == 0)	//关屏命令
						{
							model_msg.dest_id = DIVOOM_MODEL_MAIN; 
        					model_msg.msg_id = DIVOOM_MAIN_EVENT_POWER_OFF_DISP_MENU_ENTER;
        					divoom_sys_model_crl_msg_push(&model_msg);
						}
						else	//开屏命令
						{
							model_msg.dest_id = DIVOOM_MODEL_MAIN; 
        					model_msg.msg_id = DIVOOM_MAIN_EVENT_POWER_OFF_DISP_MENU_CANCLE;
        					divoom_sys_model_crl_msg_push(&model_msg);
						}
                    }
                }
            }
            #endif
            if(pDivoomHandle->sys_setting.auto_update)
            {
                if(cur_tm.tm_hour == 2 || cur_tm.tm_hour == 3)
                {
                    if(pDivoomHandle->request_update_flag == 0)
                    {
                        if(divoom_net_mqtt_get_net_status() && pDivoomHandle->divoom_connect_srv && pDivoomHandle->divoom_srv_respone)
                        {
                            int off_second = (cur_tm.tm_hour - 2) * 3600 + cur_tm.tm_min * 60 + cur_tm.tm_sec;

                            if(pDivoomHandle->request_update_time < off_second)
                            {
                                divoom_print("we will self-update the device!\n");    
                                pDivoomHandle->request_update_flag = 1;
                                divoom_main_check_deivce_update_version();
                                pDivoomHandle->request_update_time = rand() % 7000;
                            }
                        }
                    }
                }
                else
                {
                    pDivoomHandle->request_update_flag = 0;
                }
            }
            if(cur_tm.tm_hour >= 2 && cur_tm.tm_hour <= 5)
            {
                int off_second = (cur_tm.tm_hour - 2) * 3600 +
                    cur_tm.tm_min * 60 + cur_tm.tm_sec;

                if(subscribe_reload_mday != cur_tm.tm_mday)
                {
                    subscribe_reload_mday = cur_tm.tm_mday;
                    subscribe_reload_time = rand() % (4 * 3600);
                    subscribe_reload_flag = 0;
                    divoom_print("subscribe reload time: %02d:%02d:%02d\n",
                        2 + subscribe_reload_time / 3600,
                        subscribe_reload_time % 3600 / 60,
                        subscribe_reload_time % 60);
                }
                if(subscribe_reload_flag == 0 &&
                    off_second >= subscribe_reload_time &&
                    divoom_net_mqtt_get_net_status() &&
                    pDivoomHandle->divoom_connect_srv &&
                    pDivoomHandle->divoom_srv_respone)
                {
                    subscribe_reload_flag =
                        divoom_subscribe_manage_reload_more_gif(
                        cur_tm.tm_mday) ? 1 : 0;
                }
            }
            divoom_main_check_mqtt_server(cur_time);
            if(pDivoomHandle->divoom_srv_respone)
            {
                divoom_clock_notice_main_loop(cur_time);
            }
            //divoom_main_test_func();
		    divoom_app_tool_main();
            divoom_power_control_main();
            divoom_alarm_main(NULL);
            divoom_idle_poweroff_tick();
#ifndef DIVOOM_SIMULATOR
            divoom_show_mode_ctrl_tick();
#endif
            divoom_sleep_aid_ctrl_main_loop();
        }
        divoom_light_ctrl_main_loop();
#ifdef DIVOOM_TIME_FRAME_SPEAKER
#ifndef DIVOOM_TIMES_VIBE
		divoom_power_monitor_mainloop();
		//divoom_acm8625p_amp_monitor_once_loop(0, 0);
#endif
#if 1 //屏蔽内核打印
		static int pritk_num = 0;
		if (pritk_num < 3)
		{
			pritk_num++;
			exec_command_system("echo 3 > /proc/sys/kernel/printk");//屏蔽内核打印，因为导致uac播放卡顿
		}
#endif
#endif
		divoom_tomato_time_loop();
        if(pDivoomHandle->device_wifi_status)
        {
        	divoom_main_get_http_request(divoom_tick);
        }
        divoom_main_update_weather_info(0);
        if(pDivoomHandle->divoom_connect_srv && pDivoomHandle->divoom_srv_respone)
        {
            divoom_clock_manage_update_loop();
			divoom_component_manage_update_loop();
        }
        divoom_tick ++;
        divoom_boot_health_tick();
        //divoom_sys_model_crl_asyn_main(NULL);
        divoom_pic_phone_manage_delete_pic_task_main(NULL);
        //divoom_gif_ctrl_wite_task(NULL);
        divoom_light_app_check_font_task(NULL);
        divoom_manage_image_del_file_task(NULL);
        divoom_download_aliyun_create_thumb_pic_info(NULL);
        divoom_download_aliyun_file_task(NULL);
        //divoom_net_channel_main();
		divoom_system_delay_ms(100);
	}

    return NULL;
}


//需要退出的模块和需要注销的函数
void divoom_main_enter_power_shutdown_display(void)
{
	int i;

	if (!divoom_onoff_is_poweroff_active() || divoom_product_test_is_active())
		return;

	divoom_menu_control_clearall_load(
		DIVOOM_MENU_CONTROL_ID_POWER_SHUTDOWN, NULL, 0, 0);

	for (i = 0; i < 100; i++) {
		if (divoom_menu_control_get_active_menu_id()
		    == DIVOOM_MENU_CONTROL_ID_POWER_SHUTDOWN)
			break;
		divoom_system_delay_ms(5);
	}
}

void divoom_main_exit(void)
{

    divoom_main_send_lwt_msg(0);

	divoom_main_enter_power_shutdown_display();
    //divoom_net_radio_stop();
    //divoom_channel_control_run_ctrl(AK_FALSE);
	divoom_mqtt_model_remove_server_connect();
    divoom_download_aliyun_file_stop();
    divoom_update_system_file_stop();
    divoom_image_download_manage_stop();
	divoom_sys_model_crl_save_config();
	divoom_music_ctrl_set_play(0);
	divoom_onoff_program_earliest_rtc_wake();
	divoom_server_msg_manage_deinit();
	divoom_database_manage_close(NULL);
}

#if defined(DIVOOM_TIMES_VIBE) && defined(DIVOOM_PRODUCT_ASTRO_TOO)
int divoom_main_is_jl_get_update_pending(void)
{
	return 0;
}

void *divoom_main_set_jl_device_update_info(void *pupdate_info)
{
	(void)pupdate_info;
	return NULL;
}
#endif

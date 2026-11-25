#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <pthread.h>
#include "divoom.h"
#include "divoom_build_config.h"

/* 白盒切网测试仅供现场定位使用，默认不编译进正式固件。 */
#ifndef DIVOOM_WIFI_WHITEBOX_TEST_ENABLE
#define DIVOOM_WIFI_WHITEBOX_TEST_ENABLE 0
#endif

/* 详细状态快照仅在专项定位时开启，正式固件保留必要业务日志。 */
#ifndef DIVOOM_WIFI_DIAG_LOG_ENABLE
#define DIVOOM_WIFI_DIAG_LOG_ENABLE 0
#endif

#if DIVOOM_BUILD_RTOS_DEVICE
#include <console.h>
#include <stdlib.h>
#endif
#include "divoom_ble_accept_data.h"
#include "divoom_menu_control.h" 
#include "divoom_lvgl_wifi_disconnect.h"
#ifdef DIVOOM_SIMULATOR
#include "sim_aic_wifi.h"
#else
#include "aic_wifi.h"
#endif
#include "cJSON.h"

#if 1
#define DIVOOM_WIFI_WAIT_IP			(5000)
#define DIVOOM_WIFI_RECONNECT_DELAY_MS  2000
#define DIVOOM_WIFI_DHCP_WATCH_INTERVAL_MS  15000
#define DIVOOM_WIFI_DHCP_WATCH_RETRY_MAX    3
#define DIVOOM_WIFI_DHCP_RECONNECT_MAX      1
#define DIVOOM_WIFI_DHCP_WATCH_TASK_STACK   2048

#define WIFI_CFG_LOG(...) divoom_print("[WIFI_CFG] " __VA_ARGS__)
#if DIVOOM_WIFI_DIAG_LOG_ENABLE
#define WIFI_DIAG_LOG(...) WIFI_CFG_LOG("[DIAG] " __VA_ARGS__)
#else
#define WIFI_DIAG_LOG(...) ((void)0)
#endif

#define DIVOOM_WIFI_BLE_TAG			"DIVOOM_WIFI_BLE_CON"
#define DIVOOM_WIFI_NVS_NAME		"DIVOOM_WIFI_NVS"
#define DIVOOM_WIFI_SSID_NVS		"WIFI_SSID"
#define DIVOOM_WIFI_PSWD_NVS		"WIFI_PSWD"
#define DIVOOM_WIFI_APP_ID			"APP_ID"
#define DIVOOM_WIFI_CON_FLAG_NVS	"WIFI_CON_FLAG"
#define DIVOOM_WIFI_TASK_NAME		"wifi_task"
#define DIVOOM_WIFI_TASK_STACK		3072


#define DIVOOM_WIFI_NEED_CON (0XAA)
#define DIVOOM_WIFI_NOTNEED_CON (0XBB)
#define IPSTR "%d.%d.%d.%d"

#define DIVOOM_WIFI_SSID_MAX_NUM  5
//void divoom_wifi_model_confirm_cur_ssid(void);
typedef struct _divoom_wifi_config_item{
	char ssid[32];
	char pswd[64];
}divoom_wifi_config_item, *Pdivoom_wifi_config_item;

typedef struct _divoom_wifi_config_info{
    uint8 num;
    divoom_wifi_config_item list[DIVOOM_WIFI_SSID_MAX_NUM];
}divoom_wifi_config_info, *Pdivoom_wifi_config_info;

typedef struct _divoom_wifi_msg_
{
	uint8 msg_id;
    uint32 sta_session; /* Used by queued DHCP success events. */
}divoom_wifi_msg,*Pdivoom_wifi_msg;


typedef struct _divoom_wifi_con_ssid_info{  
	uint8 ssid[33]; /* Include terminators for maximum-length credentials. */
	uint8 pswd[65];
}divoom_wifi_con_ssid_info, *Pdivoom_wifi_con_ssid_info;

typedef struct _divoom_wifi_struct{  
    int    connect_wifi_id;
    uint32 wifi_disconnect_time;
    uint8 ssid_exist_flag;
    uint8 start_connect_wifi;
    uint8 cancle_connect_wifi;
    uint8 send_password_err;
    uint8 send_ssid_err;
    /* LOST_IP 时即使射频仍关联，也必须强制断开后重新走 join + DHCP。 */
    uint8 reconnect_force_disconnect;
    /* 标记用户退出扫描页后发起的恢复重连，完成或无可用网络时清除。 */
    uint8 reconnect_user_resume;
    /* 普通断网首次切换其它已保存网络时不等待，失败后的轮询仍保留节流。 */
    uint8 reconnect_immediate;
    /* 配网流程中禁止后台已保存网络重连，普通运行态不受 BLE 常驻状态影响。 */
    uint8 provisioning_active;
    uint8 run_flag;
	uint8 product_run_flag;
    uint8 connect_status;
    volatile uint8 network_ready; /* Set by GOT_IP, cleared immediately on link/IP loss. */
    uint8 connect_time;
    uint8 ssid_index;
    uint8 ssid_len;
    uint8 pswd_len;
	uint8 ssid[33];
	uint8 cur_ssid[33];
	uint8 pswd[65];
	uint8 wifi_start_flag;
	//uint8_t config_flag;//0XAA 需要配网，0XBB不需要
	uint8 wifi_status;
	bool tz_info_flag;//0->no get 1,get
	uint32 app_id;
    void *divoom_wifi_sem;
	void *queue;
    void *wifi_ssid_queue;
	DIVOOM_WIFI_BLE_CB callback;
}divoom_wifi_struct, *Pdivoom_wifi_struct;

#ifndef DIVOOM_WIFI_STA_RSSI_UNKNOWN
#define DIVOOM_WIFI_STA_RSSI_UNKNOWN  (-100)
#endif

typedef struct _divoom_wifi_sta_link_info{
	uint8 link_valid;
	uint8 link_connected;
	uint8 rssi_valid;
	int8 rssi;
}divoom_wifi_sta_link_info, *Pdivoom_wifi_sta_link_info;

Pdivoom_wifi_struct divoom_wifi_handle = NULL;

//void divoom_wifi_model_config(DIVOOM_WIFI_BLE_CB divoom_callback);
void divoom_wifi_con_get_ble_msg(uint8 cmd, void *data);
void divoom_ble_wifi_status_notify(uint8_t command, uint32_t size, uint8_t *data);
int divoom_wifi_connect_ssid_in(char *ssid, char *password);
int divoom_wifi_get_wifi_number(void);
void divoom_wifi_connect_other_ssid(void);
void divoom_wifi_put_msg(divoom_wifi_msg *pmsg);
extern uint8 divoom_ble_is_run(void);

static void divoom_wifi_event_handle(aw_wifi_event_id_t id,
                                     const aw_wifi_event_info_t *info,
                                     void *arg);
static int divoom_wifi_con_save_all_wifi_password(void);

static const char *divoom_wifi_cfg_aw_event_name(aw_wifi_event_id_t id)
{
    switch (id)
    {
    case AW_WIFI_EVENT_WIFI_READY: return "WIFI_READY";
    case AW_WIFI_EVENT_SCAN_DONE: return "SCAN_DONE";
    case AW_WIFI_EVENT_STA_START: return "STA_START";
    case AW_WIFI_EVENT_STA_STOP: return "STA_STOP";
    case AW_WIFI_EVENT_STA_CONNECTED: return "STA_CONNECTED";
    case AW_WIFI_EVENT_STA_DISCONNECTED: return "STA_DISCONNECTED";
    case AW_IP_EVENT_STA_GOT_IP: return "GOT_IP";
    case AW_IP_EVENT_STA_LOST_IP: return "LOST_IP";
    default: return "OTHER";
    }
}

static const char *divoom_wifi_cfg_msg_name(int msg_id)
{
    switch (msg_id)
    {
    case DIVOOM_WIFI_EVENT_INIT: return "INIT";
    case DIVOOM_WIFI_EVENT_ENTER_CONFIG: return "ENTER_CONFIG";
    case DIVOOM_WIFI_EVENT_CONNECT_WIFI: return "CONNECT_WIFI";
    case DIVOOM_WIFI_EVENT_EXIT_CONFIG: return "EXIT_CONFIG";
    case DIVOOM_WIFI_EVENT_RECONNECT: return "RECONNECT";
    case DIVOOM_WIFI_EVENT_WIFI_PASSWORD_ERR: return "PASSWORD_ERR";
    case DIVOOM_WIFI_EVENT_RESET_BLE: return "RESET_BLE";
    case DIVOOM_WIFI_EVENT_CONNECTED: return "CONNECTED";
    case DIVOOM_WIFI_EVENT_DHCP_SUCCESS: return "DHCP_SUCCESS";
    case DIVOOM_WIFI_EVENT_STOP_CONNECT: return "STOP_CONNECT";
    case DIVOOM_WIFI_EVENT_STOP_MQTT:
    {
        return "STOP_MQTT";
    }
    case DIVOOM_WIFI_EVENT_DHCP_TIMEOUT: return "DHCP_TIMEOUT";
    default: return "UNKNOWN";
    }
}

#if DIVOOM_WIFI_DIAG_LOG_ENABLE || DIVOOM_WIFI_WHITEBOX_TEST_ENABLE
static const char *divoom_wifi_cfg_error_name(int error_reason)
{
    switch (error_reason)
    {
    case AW_WIFI_STA_CONNECT_OK: return "OK";
    case AW_WIFI_STA_CONNECT_ERR_PASSWORD_TOO_SHORT: return "PASSWORD_TOO_SHORT";
    case AW_WIFI_STA_CONNECT_ERR_VIF_CONFIG: return "VIF_CONFIG";
    case AW_WIFI_STA_CONNECT_ERR_NETIF_NOT_FOUND: return "NETIF_NOT_FOUND";
    case AW_WIFI_STA_CONNECT_ERR_NET_INIT: return "NET_INIT";
    case AW_WIFI_STA_CONNECT_ERR_AUTH: return "AUTH";
    case AW_WIFI_STA_CONNECT_ERR_DHCP: return "DHCP";
    case AW_WIFI_STA_CONNECT_ERR_INVALID_SSID: return "INVALID_SSID";
    case AW_WIFI_STA_CONNECT_ERR_NETIF_ADD: return "NETIF_ADD";
    case AW_WIFI_STA_CONNECT_ERR_CONTROL_LINK: return "CONTROL_LINK";
    case AW_WIFI_STA_CONNECT_ERR_CONNECT: return "CONNECT_TIMEOUT";
    case AW_WIFI_STA_CONNECT_ERR_INVALID_PASSWORD: return "INVALID_PASSWORD";
    case AW_WIFI_STA_CONNECT_ERR_SCAN_LIMIT_REACHED: return "SCAN_LIMIT";
    case AW_WIFI_STA_CONNECT_ERR_WRONG_KEY: return "WRONG_KEY";
    case AW_WIFI_STA_CONNECT_ERR_MISC: return "MISC";
    default: return "UNKNOWN";
    }
}
#endif

int divoom_wifi_ensure_sta_started(void)
{
    int ret;

    WIFI_DIAG_LOG("ensure_sta begin tick=%u\n", divoom_system_get_ticks());
    ret = aw_wifi_init(NULL);
    if(ret != 0)
    {
        WIFI_DIAG_LOG("ensure_sta aw_wifi_init failed ret=%d\n", ret);
        return -1;
    }

    aw_wifi_set_event_cb(divoom_wifi_event_handle, divoom_wifi_handle);
    ret = aw_wifi_start_mode(AW_WIFI_MODE_STA);
    if(ret != 0)
    {
        WIFI_DIAG_LOG("ensure_sta start_mode failed ret=%d\n", ret);
        return -1;
    }

    WIFI_DIAG_LOG("ensure_sta end radio=%d\n",
        aw_wifi_is_connected() ? 1 : 0);
    return 0;
}

int divoom_wifi_insert_ssid_compare(void *item1, void *item2)
{
    Pdivoom_wifi_con_ssid_info pssid_info = (Pdivoom_wifi_con_ssid_info)item1;

    return strcmp(pssid_info->ssid, item2);
}

void divoom_wifi_set_wifi_time_out_flag(int status_flag)
{
    return;
}

static DIVOOM_WIFI_STATUS_CB g_divoom_wifi_status_cb = NULL;
static volatile int g_divoom_wifi_manual_connect_result = DIVOOM_WIFI_DISCONNECTED;
static volatile int g_divoom_wifi_manual_connect_error_reason = 0;
static volatile uint8 g_divoom_wifi_manual_connect_pending = 0;
static volatile uint8 g_divoom_wifi_switch_disconnect_pending = 0;
static volatile uint8 g_divoom_wifi_switch_services_paused = 0;
static volatile uint8 g_divoom_wifi_stop_connect_pending = 0;
static volatile uint32 g_divoom_wifi_dhcp_watch_generation = 0;
static volatile uint32 g_divoom_wifi_dhcp_timeout_generation = 0;
static uint8 g_divoom_wifi_dhcp_reconnect_count = 0;
#if DIVOOM_WIFI_WHITEBOX_TEST_ENABLE
static volatile uint8 g_divoom_wifi_whitebox_active = 0;
#else
#define g_divoom_wifi_whitebox_active 0
#endif
/* 仅用于复现旧 AP 消失后的切网问题，序号把扫描、重建和异步事件串成一次连接。 */
static uint32 g_divoom_wifi_diag_connect_seq = 0;
static uint32 g_divoom_wifi_diag_connect_start_tick = 0;
static divoom_wifi_sta_link_info g_divoom_wifi_sta_link_info =
{
	0, 0, 0, DIVOOM_WIFI_STA_RSSI_UNKNOWN,
};

#if DIVOOM_WIFI_DIAG_LOG_ENABLE
static void divoom_wifi_diag_log_state(const char *stage)
{
    aw_wifi_mode_t mode = AW_WIFI_MODE_NULL;
    int mode_ret;

    if (divoom_wifi_handle == NULL)
    {
        WIFI_DIAG_LOG("stage=%s handle=NULL\n", stage ? stage : "NULL");
        return;
    }
    mode_ret = aw_wifi_get_mode(&mode);
    WIFI_DIAG_LOG("stage=%s seq=%u tick=%u mode_ret=%d mode=%d "
        "radio=%d status=%d event=%d ready=%d start=%d cancel=%d manual=%d "
        "stop_pending=%d switch_pending=%d services_paused=%d "
        "provisioning=%d cur_ssid=%s request_ssid=%s\n",
        stage ? stage : "NULL", g_divoom_wifi_diag_connect_seq,
        divoom_system_get_ticks(), mode_ret, mode, aw_wifi_is_connected() ? 1 : 0,
        divoom_wifi_handle->connect_status, divoom_wifi_handle->wifi_status,
        divoom_wifi_handle->network_ready, divoom_wifi_handle->start_connect_wifi,
        divoom_wifi_handle->cancle_connect_wifi, g_divoom_wifi_manual_connect_pending,
        g_divoom_wifi_stop_connect_pending, g_divoom_wifi_switch_disconnect_pending,
        g_divoom_wifi_switch_services_paused,
        divoom_wifi_handle->provisioning_active, divoom_wifi_handle->cur_ssid,
        divoom_wifi_handle->ssid);
}
#else
#define divoom_wifi_diag_log_state(stage) ((void)0)
#endif

static int divoom_wifi_cancel_connect_for_scan(void)
{
    int ret;

    divoom_wifi_diag_log_state("cancel_connect_for_scan_begin");
    /* AIC 层负责结束旧连接任务并清理 WPA/IP，STA 模式保持可扫描。 */
    ret = aw_wifi_disconnect();
    WIFI_DIAG_LOG("cancel connect for scan ret=%d\n", ret);
    divoom_wifi_diag_log_state("cancel_connect_for_scan_end");
    return ret;
}

static void divoom_wifi_manual_connect_finish(int status)
{
    if (g_divoom_wifi_manual_connect_pending)
    {
        WIFI_DIAG_LOG("manual_finish seq=%u elapsed=%us old_result=%d "
            "new_result=%d reason=%d(%s) ssid=%s\n",
            g_divoom_wifi_diag_connect_seq,
            g_divoom_wifi_diag_connect_start_tick == 0
                || divoom_system_get_ticks() < g_divoom_wifi_diag_connect_start_tick ? 0
                : divoom_system_get_ticks() - g_divoom_wifi_diag_connect_start_tick,
            g_divoom_wifi_manual_connect_result, status,
            g_divoom_wifi_manual_connect_error_reason,
            divoom_wifi_cfg_error_name(g_divoom_wifi_manual_connect_error_reason),
            divoom_wifi_handle ? (char *)divoom_wifi_handle->ssid : "");
        g_divoom_wifi_manual_connect_result = status;
        g_divoom_wifi_manual_connect_pending = 0;
    }
    else
    {
        WIFI_DIAG_LOG("manual_finish ignored seq=%u status=%d pending=0\n",
            g_divoom_wifi_diag_connect_seq, status);
    }
}

int divoom_wifi_get_manual_connect_result(void)
{
    return g_divoom_wifi_manual_connect_result;
}

int divoom_wifi_get_manual_connect_error_reason(void)
{
    return g_divoom_wifi_manual_connect_error_reason;
}

static void divoom_wifi_resume_network_services(void)
{
    if (!g_divoom_wifi_switch_services_paused)
    {
        return;
    }

    divoom_http_client_request_abort_clear();
    g_divoom_wifi_switch_services_paused = 0;
}

static void divoom_wifi_stop_old_network_services(void)
{
    if (g_divoom_wifi_switch_services_paused)
    {
        return;
    }

    /*
     * 先终止旧链路上的 HTTP，再等待 MQTT 线程确认退出。这里运行在 WiFi
     * 工作线程；只有收到 MQTT disconnect_sem 后才允许 AIC 清理旧 STA，
     * 避免 TCP 仍占用旧链路时 fhost_wpa_end 等到 keepalive 超时。
     */
    g_divoom_wifi_switch_services_paused = 1;
    divoom_http_client_request_abort();
    divoom_mqtt_model_remove_server_connect();
    WIFI_CFG_LOG("old network services disconnected, start WiFi switch\n");
}

static int divoom_wifi_wait_before_reconnect(void)
{
    int ri;

    if (divoom_wifi_handle == NULL)
    {
        return 0;
    }

    WIFI_CFG_LOG("delay %dms before reconnect\n", DIVOOM_WIFI_RECONNECT_DELAY_MS);
    for (ri = 0; ri < (DIVOOM_WIFI_RECONNECT_DELAY_MS / 100);
         ri++)
    {
        if (divoom_wifi_handle->cancle_connect_wifi)
        {
            WIFI_CFG_LOG("reconnect delay canceled for scan\n");
            return 0;
        }
        divoom_system_delay_ms(100);
    }
    return 1;
}

static void divoom_wifi_try_show_disconnect_toast(void)
{
    if (divoom_wifi_handle == NULL || divoom_wifi_handle->wifi_disconnect_time == 0)
    {
        return;
    }
    if (divoom_wifi_handle->wifi_disconnect_time + 30 >= divoom_system_get_ticks())
    {
        return;
    }
    if (divoom_main_set_wifi_disconnect_notice_flag(-1) == 0
        && divoom_menu_control_is_enter_config_wifi()
        && divoom_main_get_wifi_config_flag())
    {
        divoom_lvgl_wifi_disconnect_init();
    }
}

static void divoom_wifi_aw_notify_disconnect(void)
{
    uint8 notify_disconnect;

    if (divoom_wifi_handle == NULL)
    {
        return;
    }

    notify_disconnect = divoom_wifi_handle->connect_status != DIVOOM_WIFI_DISCONNECTED;
    divoom_wifi_handle->connect_status = DIVOOM_WIFI_DISCONNECTED;
    if (divoom_wifi_handle->wifi_disconnect_time == 0)
    {
        divoom_wifi_handle->wifi_disconnect_time = divoom_system_get_ticks();
    }
    divoom_print("disconnect :%d, %d \n", divoom_wifi_handle->wifi_disconnect_time, divoom_system_get_ticks());
    divoom_wifi_try_show_disconnect_toast();
    if (notify_disconnect && divoom_wifi_handle->callback)
    {
        divoom_wifi_handle->callback(DIVOOM_WIFI_DISCONNECTED, NULL);
    }
    if(divoom_wifi_handle->provisioning_active == 0
        && divoom_wifi_handle->start_connect_wifi == 0
        && divoom_wifi_handle->cancle_connect_wifi == 0)
    {
        divoom_wifi_handle->reconnect_immediate = 1;
        divoom_wifi_connect_other_ssid();
    }
}

static void divoom_wifi_aw_notify_connect_fail(int error_reason)
{
    uint8 send_status = DIVOOM_WIFI_OTHER_ERR;
    uint8 manual_connect_failed;

    if (divoom_wifi_handle == NULL)
    {
        return;
    }
    if (divoom_wifi_handle->cancle_connect_wifi)
    {
        /* 扫描页主动取消产生的失败事件不属于连接失败，不能继续派生重试。 */
        divoom_wifi_handle->start_connect_wifi = 0;
        g_divoom_wifi_manual_connect_pending = 0;
        g_divoom_wifi_manual_connect_result = DIVOOM_WIFI_DISCONNECTED;
        g_divoom_wifi_manual_connect_error_reason = 0;
        divoom_wifi_resume_network_services();
        WIFI_DIAG_LOG("ignore failure seq=%u reason=%d(%s) while scan paused\n",
            g_divoom_wifi_diag_connect_seq, error_reason,
            divoom_wifi_cfg_error_name(error_reason));
        divoom_wifi_diag_log_state("connect_fail_ignored");
        return;
    }

    WIFI_DIAG_LOG("connect_fail seq=%u reason=%d(%s) manual=%d "
        "start=%d active_menu=%d\n",
        g_divoom_wifi_diag_connect_seq, error_reason,
        divoom_wifi_cfg_error_name(error_reason),
        g_divoom_wifi_manual_connect_pending,
        divoom_wifi_handle->start_connect_wifi,
        divoom_menu_control_get_active_menu_id());
    divoom_wifi_diag_log_state("connect_fail_begin");
    divoom_wifi_try_show_disconnect_toast();
    manual_connect_failed = g_divoom_wifi_manual_connect_pending;
    g_divoom_wifi_manual_connect_error_reason = error_reason;
    divoom_wifi_manual_connect_finish(DIVOOM_WIFI_OTHER_ERR);
    divoom_wifi_resume_network_services();
    if(divoom_wifi_handle->callback)
    {
        divoom_wifi_handle->callback(DIVOOM_WIFI_OTHER_ERR, NULL);
    }
    divoom_wifi_handle->connect_status = DIVOOM_WIFI_OTHER_ERR;
    divoom_ble_set_wifi_status(DIVOOM_WIFI_OTHER_ERR);
    if(divoom_wifi_handle->start_connect_wifi && divoom_wifi_handle->send_password_err == 0)
    {
        divoom_ble_wifi_status_notify(WIFI_STATUS_BLE_COMMAND, 1, &send_status);
        divoom_wifi_handle->send_password_err = 1;
    }
    divoom_wifi_handle->start_connect_wifi = 0;
    /* 手动选网失败由列表页完成三次重试；后台重连失败统一回到带间隔的 RECONNECT。 */
    if(!manual_connect_failed && divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_ID_CONFIG_WIFI)
    {
        divoom_wifi_connect_other_ssid();
    }
    divoom_wifi_diag_log_state("connect_fail_end");
}

#if DIVOOM_BUILD_RTOS_DEVICE
static void divoom_wifi_dhcp_recovery_request(uint32 generation)
{
    divoom_wifi_msg wifi_msg;

    if (generation != g_divoom_wifi_dhcp_watch_generation)
    {
        return;
    }

    /* 看门狗只投递恢复请求，WPA 清理和重连统一由 WiFi 工作线程串行执行。 */
    g_divoom_wifi_dhcp_timeout_generation = generation;
    wifi_msg.msg_id = DIVOOM_WIFI_EVENT_DHCP_TIMEOUT;
    divoom_wifi_put_msg(&wifi_msg);
}

typedef struct {
    uint32 generation;
    uint32 session;
} divoom_wifi_dhcp_watch_context;

static void *divoom_wifi_dhcp_watchdog_task(void *param)
{
    divoom_wifi_dhcp_watch_context *ctx = (divoom_wifi_dhcp_watch_context *)param;
    uint32 generation = ctx->generation;
    uint32 session = ctx->session;
    uint8 retry_count;

    divoom_system_free(ctx);

    for (retry_count = 0; retry_count <= DIVOOM_WIFI_DHCP_WATCH_RETRY_MAX;
        retry_count++)
    {
#if DIVOOM_WIFI_DIAG_LOG_ENABLE
        int stop_ret;
#endif
        int start_ret;

        divoom_system_delay_ms(DIVOOM_WIFI_DHCP_WATCH_INTERVAL_MS);
        if (generation != g_divoom_wifi_dhcp_watch_generation
            || divoom_wifi_handle == NULL
            || divoom_wifi_handle->network_ready
            || !divoom_wifi_handle->start_connect_wifi)
        {
            return NULL;
        }

        if (retry_count >= DIVOOM_WIFI_DHCP_WATCH_RETRY_MAX)
        {
            WIFI_DIAG_LOG("DHCP watchdog timeout seq=%u generation=%u "
                "ssid=%s retries=%u\n",
                g_divoom_wifi_diag_connect_seq, generation,
                divoom_wifi_handle->ssid, retry_count);
            divoom_wifi_dhcp_recovery_request(generation);
            return NULL;
        }

        /* DHCP 恢复期间底层可能短暂断开后自行重新关联，保留看门狗直到总超时。 */
        if (!aw_wifi_is_connected()
            || divoom_wifi_handle->wifi_status != AW_WIFI_EVENT_STA_CONNECTED)
        {
            WIFI_DIAG_LOG("DHCP watchdog wait link recovery retry=%u/%u "
                "seq=%u generation=%u status=%d radio=%d ssid=%s\n",
                retry_count + 1, DIVOOM_WIFI_DHCP_WATCH_RETRY_MAX,
                g_divoom_wifi_diag_connect_seq, generation,
                divoom_wifi_handle->wifi_status,
                aw_wifi_is_connected() ? 1 : 0,
                divoom_wifi_handle->ssid);
            continue;
        }

        /* AIC 的 DHCP 监控线程 10 秒无地址会静默退出，上层主动重建 DHCP 会话。 */
#if DIVOOM_WIFI_DIAG_LOG_ENABLE
        stop_ret = aw_wifi_sta_dhcpc_stop_for_session(session);
        if (stop_ret != 0)
            return NULL;
#else
        if (aw_wifi_sta_dhcpc_stop_for_session(session) != 0)
            return NULL;
#endif
        if (generation != g_divoom_wifi_dhcp_watch_generation
            || divoom_wifi_handle == NULL
            || divoom_wifi_handle->network_ready)
        {
            return NULL;
        }
        start_ret = aw_wifi_sta_dhcpc_start_for_session(session);
        WIFI_DIAG_LOG("DHCP watchdog retry=%u/%u seq=%u generation=%u "
            "stop_ret=%d start_ret=%d ssid=%s\n",
            retry_count + 1, DIVOOM_WIFI_DHCP_WATCH_RETRY_MAX,
            g_divoom_wifi_diag_connect_seq, generation,
            stop_ret, start_ret, divoom_wifi_handle->ssid);
        if (start_ret != 0)
        {
            divoom_wifi_dhcp_recovery_request(generation);
            return NULL;
        }
    }

    return NULL;
}

static void divoom_wifi_dhcp_watchdog_start(uint32 session)
{
    uint32 generation = ++g_divoom_wifi_dhcp_watch_generation;
    divoom_wifi_dhcp_watch_context *ctx = divoom_system_malloc(sizeof(*ctx));

    if (ctx == NULL) {
        WIFI_DIAG_LOG("create DHCP watchdog context failed\n");
        return;
    }
    ctx->generation = generation;
    ctx->session = session;
    if (divoom_system_create_task(divoom_wifi_dhcp_watchdog_task,
        "wifi_dhcp", ctx, DIVOOM_WIFI_DHCP_WATCH_TASK_STACK) == NULL)
    {
        divoom_system_free(ctx);
        WIFI_DIAG_LOG("create DHCP watchdog failed seq=%u generation=%u\n",
            g_divoom_wifi_diag_connect_seq, generation);
    }
}
#endif

#if DIVOOM_BUILD_RTOS_DEVICE
/* Invoked under the AIC session guard; only update local state here. */
static void divoom_wifi_commit_session_event(void *arg)
{
    aw_wifi_event_id_t id = *(aw_wifi_event_id_t *)arg;
    divoom_wifi_handle->wifi_status = id;
    divoom_wifi_handle->network_ready = (id == AW_IP_EVENT_STA_GOT_IP);
}
#endif

static void divoom_wifi_event_handle(aw_wifi_event_id_t id,
                                     const aw_wifi_event_info_t *info,
                                     void *arg)
{
    divoom_wifi_msg wifi_msg;
    int event_reason;
    uint32 event_tick;
    uint32 elapsed_tick;
#if DIVOOM_WIFI_DIAG_LOG_ENABLE
    const uint8 *ip_bytes;
#endif

    (void)arg;

    if(divoom_wifi_handle == NULL)
    {
        return;
    }

#if DIVOOM_BUILD_RTOS_DEVICE
    if (id == AW_WIFI_EVENT_STA_CONNECTED || id == AW_IP_EVENT_STA_GOT_IP) {
        if (!info || !aw_wifi_sta_session_update(info->sta_session,
                                                  divoom_wifi_commit_session_event, &id))
            return;
    } else
#endif
    {
        divoom_wifi_handle->wifi_status = id;
        if (id == AW_IP_EVENT_STA_GOT_IP) {
            divoom_wifi_handle->network_ready = 1;
        } else if (id == AW_IP_EVENT_STA_LOST_IP ||
                   id == AW_WIFI_EVENT_STA_DISCONNECTED ||
                   id == AW_WIFI_EVENT_STA_STOP ||
                   id == AW_WIFI_EVENT_STA_START ||
                   id == AW_WIFI_EVENT_STA_CONNECTED) {
            divoom_wifi_handle->network_ready = 0;
        }
    }
    if (id == AW_WIFI_EVENT_STA_DISCONNECTED || id == AW_IP_EVENT_STA_LOST_IP)
    {
        /* 先排入 MQTT 停止事件，再处理 WiFi 重连；底层回调不能等待 MQTT 退出。 */
        wifi_msg.msg_id = DIVOOM_WIFI_EVENT_STOP_MQTT;
        divoom_wifi_put_msg(&wifi_msg);
    }
    event_reason = info ? (int)(int8)info->reason : 0;
    event_tick = divoom_system_get_ticks();
    elapsed_tick = g_divoom_wifi_diag_connect_start_tick == 0
        || event_tick < g_divoom_wifi_diag_connect_start_tick ? 0
        : event_tick - g_divoom_wifi_diag_connect_start_tick;
    WIFI_CFG_LOG("aw event: %d(%s) reason=%d seq=%u elapsed=%us "
        "radio=%d start_connect=%d status=%d ready=%d\n",
        id, divoom_wifi_cfg_aw_event_name(id),
        event_reason, g_divoom_wifi_diag_connect_seq, elapsed_tick,
        aw_wifi_is_connected() ? 1 : 0,
        divoom_wifi_handle->start_connect_wifi,
        divoom_wifi_handle->connect_status,
        divoom_wifi_handle->network_ready);
    if (id == AW_IP_EVENT_STA_GOT_IP && info)
    {
#if DIVOOM_WIFI_DIAG_LOG_ENABLE
        ip_bytes = (const uint8 *)&info->ip;
        WIFI_DIAG_LOG("got_ip seq=%u ip=" IPSTR " gateway=0x%08x "
            "netmask=0x%08x\n", g_divoom_wifi_diag_connect_seq,
            ip_bytes[0], ip_bytes[1], ip_bytes[2], ip_bytes[3],
            info->gateway, info->netmask);
#endif
    }

    if (g_divoom_wifi_status_cb)
    {
        g_divoom_wifi_status_cb((void *)&id);
    }

    switch(id)
    {
        case AW_WIFI_EVENT_WIFI_READY:
        case AW_WIFI_EVENT_STA_START:
            divoom_print("wifi start \n");
            /* 扫描也可能再次上报 READY，不能覆盖仍然有效的已连接状态。 */
            if (!aw_wifi_is_connected())
            {
                divoom_wifi_handle->connect_status = DIVOOM_WIFI_START;
                if (divoom_wifi_handle->callback)
                {
                    divoom_wifi_handle->callback(DIVOOM_WIFI_START, NULL);
                }
            }
            else
            {
                WIFI_CFG_LOG("ignore start event while STA is connected\n");
            }
            break;

        case AW_WIFI_EVENT_SCAN_DONE:
            divoom_print("scan results \n");
            divoom_wifi_set_wifi_time_out_flag(2);
            break;

        case AW_WIFI_EVENT_STA_CONNECTED:
            /* aw_wifi_connect 只负责异步关联，DHCP 也由事件链继续，禁止连接请求阻塞 UI。 */
            g_divoom_wifi_switch_disconnect_pending = 0;
            WIFI_DIAG_LOG("associated seq=%u start DHCP ssid=%s\n",
                g_divoom_wifi_diag_connect_seq, divoom_wifi_handle->ssid);
#if DIVOOM_BUILD_RTOS_DEVICE
            if (aw_wifi_sta_dhcpc_start_for_session(info->sta_session) != 0)
#else
            if (aw_wifi_sta_dhcpc_start() != 0)
#endif
            {
#if DIVOOM_BUILD_RTOS_DEVICE
                if (info->sta_session != aw_wifi_sta_session_token())
                    return;
#endif
                WIFI_CFG_LOG("start DHCP failed\n");
#if DIVOOM_BUILD_RTOS_DEVICE
                g_divoom_wifi_dhcp_watch_generation++;
                divoom_wifi_dhcp_recovery_request(
                    g_divoom_wifi_dhcp_watch_generation);
#else
                divoom_wifi_aw_notify_connect_fail(AW_WIFI_STA_CONNECT_ERR_DHCP);
#endif
            }
            else
            {
                WIFI_CFG_LOG("STA associated, DHCP started\n");
#if DIVOOM_BUILD_RTOS_DEVICE
                divoom_wifi_dhcp_watchdog_start(info->sta_session);
#endif
            }
            break;

        case AW_IP_EVENT_STA_GOT_IP:
#if DIVOOM_BUILD_RTOS_DEVICE
            if (info->sta_session != aw_wifi_sta_session_token())
                return;
            g_divoom_wifi_dhcp_watch_generation++;
            g_divoom_wifi_dhcp_timeout_generation = 0;
            g_divoom_wifi_dhcp_reconnect_count = 0;
#endif
            divoom_print("dhcp success \n");
            divoom_wifi_resume_network_services();
#ifdef DIVOOM_SIMULATOR
            divoom_print("[sim_discon] free\n");
#endif
            divoom_wifi_handle->start_connect_wifi = 0;
            wifi_msg.msg_id = DIVOOM_WIFI_EVENT_CONNECTED;
#if DIVOOM_BUILD_RTOS_DEVICE
            wifi_msg.sta_session = info->sta_session;
#else
            wifi_msg.sta_session = 0;
#endif
            divoom_wifi_put_msg(&wifi_msg);
            wifi_msg.msg_id = DIVOOM_WIFI_EVENT_DHCP_SUCCESS;
            divoom_wifi_put_msg(&wifi_msg);
            break;

        case AW_IP_EVENT_STA_LOST_IP:
#if DIVOOM_BUILD_RTOS_DEVICE
            g_divoom_wifi_dhcp_watch_generation++;
#endif
            divoom_print("dhcp timeout \n");
            /* LOST_IP 不一定继续上报 STA_DISCONNECTED，主动进入同一重连状态机。 */
            if (!g_divoom_wifi_switch_disconnect_pending)
            {
                divoom_wifi_handle->reconnect_force_disconnect = 1;
                divoom_wifi_aw_notify_disconnect();
            }
            break;

        case AW_WIFI_EVENT_STA_DISCONNECTED:
#if DIVOOM_BUILD_RTOS_DEVICE
            /* reason=0 可能是 DHCP 恢复触发的瞬时链路切换，继续由当前看门狗兜底。 */
            if (!((info == NULL || info->reason == 0)
                && divoom_wifi_handle->start_connect_wifi
                && g_divoom_wifi_manual_connect_pending))
            {
                g_divoom_wifi_dhcp_watch_generation++;
            }
#endif
            WIFI_DIAG_LOG("disconnected seq=%u reason=%d(%s) "
                "intentional=%d start=%d manual=%d cancel=%d\n",
                g_divoom_wifi_diag_connect_seq, event_reason,
                divoom_wifi_cfg_error_name(event_reason),
                g_divoom_wifi_switch_disconnect_pending,
                divoom_wifi_handle->start_connect_wifi,
                g_divoom_wifi_manual_connect_pending,
                divoom_wifi_handle->cancle_connect_wifi);
            if (g_divoom_wifi_switch_disconnect_pending
                && (info == NULL || info->reason == 0))
            {
                g_divoom_wifi_switch_disconnect_pending = 0;
                WIFI_CFG_LOG("ignore intentional disconnect for network switch\n");
            }
            else if ((info == NULL || info->reason == 0)
                && divoom_wifi_handle->start_connect_wifi
                && g_divoom_wifi_manual_connect_pending)
            {
                WIFI_DIAG_LOG("keep manual connect pending for transient "
                    "reason=0 disconnect seq=%u\n",
                    g_divoom_wifi_diag_connect_seq);
            }
            else
            {
                g_divoom_wifi_switch_disconnect_pending = 0;
                if (divoom_wifi_handle->start_connect_wifi || g_divoom_wifi_manual_connect_pending)
                {
                    divoom_wifi_aw_notify_connect_fail(
                        info ? (int)(int8)info->reason : AW_WIFI_STA_CONNECT_ERR_CONNECT);
                }
                else
                {
                    divoom_wifi_aw_notify_disconnect();
                }
            }
            break;

        default:
            break;
    }
}



int divoom_wifi_put_msg_compare(void *item1, void *item2)
{
    divoom_wifi_msg *msg1 = (divoom_wifi_msg *)item1;
    divoom_wifi_msg *msg2 = (divoom_wifi_msg *)item2;

    if (msg1->msg_id != msg2->msg_id)
        return 1;
    if (msg1->msg_id == DIVOOM_WIFI_EVENT_CONNECTED ||
        msg1->msg_id == DIVOOM_WIFI_EVENT_DHCP_SUCCESS)
        return msg1->sta_session == msg2->sta_session ? 0 : 1;
    return 0;
}

void divoom_wifi_put_msg(divoom_wifi_msg *pmsg)
{
    divoom_wifi_msg *task_queue_handle;
#if DIVOOM_WIFI_DIAG_LOG_ENABLE
	int queue_count;
#endif
	if(NULL == divoom_wifi_handle || NULL == pmsg)
	{
		return;
	}
#if DIVOOM_WIFI_DIAG_LOG_ENABLE
    queue_count = divoom_queue_items(divoom_wifi_handle->queue);
#endif
    task_queue_handle = divoom_queue_get_cmp(divoom_wifi_handle->queue,pmsg, divoom_wifi_put_msg_compare);
    if(task_queue_handle != NULL)
    {
        WIFI_DIAG_LOG("queue drop duplicate msg=%s(%d) depth=%d seq=%u\n",
            divoom_wifi_cfg_msg_name(pmsg->msg_id), pmsg->msg_id,
            queue_count, g_divoom_wifi_diag_connect_seq);
        return;
    }
    task_queue_handle = divoom_system_malloc(sizeof(divoom_wifi_msg));
    if(task_queue_handle)
    {
        memcpy(task_queue_handle, pmsg, sizeof(divoom_wifi_msg));
        divoom_queue_push(divoom_wifi_handle->queue,task_queue_handle);
		WIFI_DIAG_LOG("queue push msg=%s(%d) depth=%d->%d seq=%u\n",
            divoom_wifi_cfg_msg_name(pmsg->msg_id), pmsg->msg_id,
            queue_count, divoom_queue_items(divoom_wifi_handle->queue),
            g_divoom_wifi_diag_connect_seq);
		divoom_system_put_sem(divoom_wifi_handle->divoom_wifi_sem);
    }
    else
    {
        WIFI_DIAG_LOG("queue alloc failed msg=%s(%d) depth=%d\n",
            divoom_wifi_cfg_msg_name(pmsg->msg_id), pmsg->msg_id, queue_count);
    }
}

//获取最后连接的SSID
int divoom_wifi_get_connect_ssid_info(char *ssid)
{
    strcpy(ssid, divoom_wifi_handle->cur_ssid);
    return 0;
}


int divoom_wifi_get_exist_wifi_info(char *ssid, char *password)
{
    int save_wifi_num = 0, connect_id;
    Pdivoom_wifi_con_ssid_info pssid_info;

    if (divoom_wifi_handle == NULL || ssid == NULL || password == NULL
        || divoom_wifi_handle->wifi_ssid_queue == NULL)
    {
        return -1;
    }
    ssid[0] = 0;
    password[0] = 0;
    save_wifi_num = divoom_queue_items(divoom_wifi_handle->wifi_ssid_queue);
    if(save_wifi_num <= 0)
    {
        return -1;
    }

    if (divoom_wifi_handle->connect_wifi_id < 0
        || divoom_wifi_handle->connect_wifi_id >= save_wifi_num)
    {
        divoom_wifi_handle->connect_wifi_id = save_wifi_num -1;
    }
    connect_id = divoom_wifi_handle->connect_wifi_id;
    pssid_info = divoom_queue_get_index_item(divoom_wifi_handle->wifi_ssid_queue, connect_id);
    if(pssid_info == NULL)
    {
        return -1;
    }
    strcpy(ssid, pssid_info->ssid);
    strcpy(password, pssid_info->pswd);
    /* 当前断开的网络先尝试；失败后再从保存列表向前轮换。 */
    divoom_wifi_handle->connect_wifi_id = connect_id == 0
        ? save_wifi_num - 1 : connect_id - 1;
    divoom_debug("find existing wifi: %s, password_len:%d, id:%d", ssid,
        (int)strlen(password), connect_id);
    return 0;
}

/* 连接成功后把下次自动重连起点放到其它保存网络；仅有一项时才回到当前项。 */
static void divoom_wifi_set_reconnect_start_from_ssid(const char *ssid)
{
    int i;
    int save_wifi_num;
    Pdivoom_wifi_con_ssid_info pssid_info;

    if (divoom_wifi_handle == NULL || ssid == NULL || ssid[0] == 0
        || divoom_wifi_handle->wifi_ssid_queue == NULL)
    {
        return;
    }
    save_wifi_num = divoom_queue_items(divoom_wifi_handle->wifi_ssid_queue);
    for (i = 0; i < save_wifi_num; i++)
    {
        pssid_info = divoom_queue_get_index_item(divoom_wifi_handle->wifi_ssid_queue, i);
        if (pssid_info && strcmp((char *)pssid_info->ssid, ssid) == 0)
        {
            divoom_wifi_handle->connect_wifi_id = save_wifi_num > 1
                ? (i == 0 ? save_wifi_num - 1 : i - 1) : i;
            return;
        }
    }
}

int divoom_wifi_power_on_connect_wifi(void)
{
    int save_wifi_num;

    if (divoom_product_test_is_active())
    {
        return 0;
    }
    if (divoom_wifi_handle == NULL || divoom_wifi_handle->wifi_ssid_queue == NULL)
    {
        return 0;
    }
    save_wifi_num = divoom_queue_items(divoom_wifi_handle->wifi_ssid_queue);
    if (save_wifi_num <= 0)
    {
        return 0;
    }

    /* 保存文件尾部是最近保存的网络；首次失败后从前一项继续轮询。 */
    divoom_wifi_handle->connect_wifi_id = save_wifi_num - 1;
    if (divoom_wifi_get_exist_wifi_info((char *)divoom_wifi_handle->ssid,
        (char *)divoom_wifi_handle->pswd) != 0)
    {
        return 0;
    }
    if (divoom_wifi_connect_ssid_in((char *)divoom_wifi_handle->ssid,
        (char *)divoom_wifi_handle->pswd) == 0)
    {
        return 0;
    }
    WIFI_CFG_LOG("power-on connect started: %s\n", divoom_wifi_handle->ssid);
    return 1;
}

/* Called only by wifi_con: keep queue mutation and file persistence on its
 * existing worker, including recovery-mode DHCP success. */
static void *s_wifi_config_lock;
static int divoom_wifi_config_lock_take(void);
static int divoom_wifi_con_save_all_wifi_password_unlocked(void);
static int s_wifi_userdata_formatting;

static void divoom_wifi_remember_current_network(void)
{
    Pdivoom_wifi_con_ssid_info saved;
    if (!divoom_wifi_handle || !divoom_wifi_handle->wifi_ssid_queue || !divoom_wifi_handle->ssid[0])
        return;
    if (!divoom_wifi_config_lock_take()) return;
    saved = divoom_queue_get_cmp(divoom_wifi_handle->wifi_ssid_queue,
        divoom_wifi_handle->ssid, divoom_wifi_insert_ssid_compare);
    if (!saved) {
        saved = divoom_system_malloc(sizeof(*saved));
        if (!saved) { divoom_system_free_lock_direct(s_wifi_config_lock); return; }
        memset(saved, 0, sizeof(*saved));
        snprintf((char *)saved->ssid, sizeof(saved->ssid), "%s", divoom_wifi_handle->ssid);
        snprintf((char *)saved->pswd, sizeof(saved->pswd), "%s", divoom_wifi_handle->pswd);
        if (!divoom_queue_push(divoom_wifi_handle->wifi_ssid_queue, saved)) {
            divoom_system_free(saved); divoom_system_free_lock_direct(s_wifi_config_lock); return;
        }
    } else {
        /* Empty is meaningful when an AP changes from secured to open. */
        snprintf((char *)saved->pswd, sizeof(saved->pswd), "%s", divoom_wifi_handle->pswd);
    }
    if (!s_wifi_userdata_formatting && divoom_wifi_con_save_all_wifi_password_unlocked() != 0)
        WIFI_CFG_LOG("save wifi.conf failed ssid=%s\n", divoom_wifi_handle->ssid);
    divoom_wifi_set_reconnect_start_from_ssid((char *)divoom_wifi_handle->ssid);
    divoom_system_free_lock_direct(s_wifi_config_lock);
}

static void divoom_wifi_con_dispatch_msg(Pdivoom_wifi_struct divoom_task_handle, divoom_wifi_msg *task_queue_handle)
{
    if (!divoom_task_handle || !task_queue_handle)
    {
        return;
    }
#if DIVOOM_BUILD_RTOS_DEVICE
    if ((task_queue_handle->msg_id == DIVOOM_WIFI_EVENT_CONNECTED ||
         task_queue_handle->msg_id == DIVOOM_WIFI_EVENT_DHCP_SUCCESS) &&
        task_queue_handle->sta_session != aw_wifi_sta_session_token())
        return;
#endif
    divoom_debug("event: %d", task_queue_handle->msg_id);
    WIFI_CFG_LOG("task msg: %s depth_after_pop=%d seq=%u\n",
        divoom_wifi_cfg_msg_name(task_queue_handle->msg_id),
        divoom_queue_items(divoom_task_handle->queue),
        g_divoom_wifi_diag_connect_seq);
    switch (task_queue_handle->msg_id)
    {
    case DIVOOM_WIFI_EVENT_RESET_BLE:
    {
        if (divoom_wifi_model_get_wifi_status() == 0)
        {
            divoom_bt_ctrl_reset_ble();
        }
        break;
    }
    case DIVOOM_WIFI_EVENT_WIFI_PASSWORD_ERR:
    {
        /* 兼容队列中可能残留的旧事件，也统一经过 RECONNECT 的 2 秒节流。 */
        divoom_wifi_connect_other_ssid();
        break;
    }
    case DIVOOM_WIFI_EVENT_RECONNECT:
    {
        uint8 reconnect_immediate;

        reconnect_immediate = divoom_wifi_handle->reconnect_immediate;
        divoom_wifi_handle->reconnect_immediate = 0;
        if ((!reconnect_immediate && !divoom_wifi_wait_before_reconnect())
            || divoom_wifi_handle->cancle_connect_wifi
            || divoom_wifi_handle->start_connect_wifi
            || g_divoom_wifi_manual_connect_pending
            || divoom_wifi_handle->provisioning_active)
        {
            WIFI_DIAG_LOG("reconnect skipped immediate=%d cancel=%d start=%d "
                "manual=%d provisioning=%d saved=%d\n",
                reconnect_immediate, divoom_wifi_handle->cancle_connect_wifi,
                divoom_wifi_handle->start_connect_wifi,
                g_divoom_wifi_manual_connect_pending,
                divoom_wifi_handle->provisioning_active,
                divoom_wifi_handle->wifi_ssid_queue
                    ? divoom_queue_items(divoom_wifi_handle->wifi_ssid_queue) : -1);
            break;
        }
        if (divoom_wifi_handle->reconnect_force_disconnect)
        {
            /* aw_wifi_connect 的驱动任务会清理旧 STA 会话，不在消息线程同步断开。 */
            divoom_wifi_handle->reconnect_force_disconnect = 0;
        }
        if (divoom_wifi_get_exist_wifi_info(divoom_task_handle->ssid,
            divoom_task_handle->pswd) != 0)
        {
            divoom_wifi_handle->reconnect_user_resume = 0;
            WIFI_CFG_LOG("stop reconnect: no saved WiFi\n");
            break;
        }
        if (divoom_wifi_connect_ssid_in(divoom_task_handle->ssid,
            divoom_task_handle->pswd) == 0
            && divoom_wifi_handle->cancle_connect_wifi == 0
            && divoom_wifi_handle->provisioning_active == 0)
        {
            /* 同步失败也要尝试下一个保存网络，下一条 RECONNECT 自带 2 秒间隔。 */
            divoom_wifi_connect_other_ssid();
        }
        break;
    }
    case DIVOOM_WIFI_EVENT_CONNECT_WIFI:
    {
#if DIVOOM_BUILD_RTOS_DEVICE
        g_divoom_wifi_dhcp_watch_generation++;
        g_divoom_wifi_dhcp_timeout_generation = 0;
        g_divoom_wifi_dhcp_reconnect_count = 0;
#endif
        g_divoom_wifi_diag_connect_seq++;
        g_divoom_wifi_diag_connect_start_tick = divoom_system_get_ticks();
        WIFI_CFG_LOG("CONNECT_WIFI seq=%u ssid=%s pswd_len=%d\n",
            g_divoom_wifi_diag_connect_seq,
            divoom_task_handle->ssid,
            (int)strlen((char *)divoom_task_handle->pswd));
        divoom_wifi_diag_log_state("connect_dispatch_begin");
        /* 连接替换、旧 WPA 清理和异步任务串行化统一由 AIC 层负责。 */
        g_divoom_wifi_manual_connect_result = DIVOOM_WIFI_CONNECTING;
        g_divoom_wifi_manual_connect_error_reason = 0;
        g_divoom_wifi_manual_connect_pending = 1;
        divoom_wifi_handle->cancle_connect_wifi = 0;
        divoom_wifi_handle->reconnect_user_resume = 0;
        divoom_wifi_handle->send_ssid_err = 0;
        divoom_wifi_handle->send_password_err = 0;
        divoom_ble_set_wifi_status(DIVOOM_WIFI_CONNECTING);
        divoom_wifi_connect_ssid_in(divoom_task_handle->ssid, divoom_task_handle->pswd);
        break;
    }
    case DIVOOM_WIFI_EVENT_DHCP_TIMEOUT:
    {
#if DIVOOM_BUILD_RTOS_DEVICE
        int ret;
        uint32 timeout_generation = g_divoom_wifi_dhcp_timeout_generation;

        if (timeout_generation == 0
            || timeout_generation != g_divoom_wifi_dhcp_watch_generation
            || divoom_wifi_handle->network_ready
            || !divoom_wifi_handle->start_connect_wifi)
        {
            WIFI_CFG_LOG("ignore stale DHCP timeout generation=%u current=%u "
                "ready=%d start=%d\n", timeout_generation,
                g_divoom_wifi_dhcp_watch_generation,
                divoom_wifi_handle->network_ready,
                divoom_wifi_handle->start_connect_wifi);
            break;
        }

        g_divoom_wifi_dhcp_watch_generation++;
        g_divoom_wifi_dhcp_timeout_generation = 0;
        g_divoom_wifi_switch_disconnect_pending = 1;
        WIFI_CFG_LOG("DHCP timeout, rebuild STA session ssid=%s recovery=%u/%u\n",
            divoom_wifi_handle->ssid,
            g_divoom_wifi_dhcp_reconnect_count + 1,
            DIVOOM_WIFI_DHCP_RECONNECT_MAX);
        ret = aw_wifi_disconnect();
        if (ret != 0)
        {
            g_divoom_wifi_switch_disconnect_pending = 0;
            WIFI_CFG_LOG("DHCP recovery disconnect failed ret=%d ssid=%s\n",
                ret, divoom_wifi_handle->ssid);
            divoom_wifi_aw_notify_connect_fail(AW_WIFI_STA_CONNECT_ERR_DHCP);
            break;
        }

        if (g_divoom_wifi_dhcp_reconnect_count
            < DIVOOM_WIFI_DHCP_RECONNECT_MAX
            && !divoom_wifi_handle->cancle_connect_wifi)
        {
            g_divoom_wifi_dhcp_reconnect_count++;
            /* 保留本轮手动连接状态，对同一配置完整重建 STA、WPA 和 DHCP。 */
            if (divoom_wifi_connect_ssid_in((char *)divoom_wifi_handle->ssid,
                (char *)divoom_wifi_handle->pswd) == 0
                && g_divoom_wifi_manual_connect_pending)
            {
                divoom_wifi_aw_notify_connect_fail(AW_WIFI_STA_CONNECT_ERR_DHCP);
            }
        }
        else
        {
            divoom_wifi_aw_notify_connect_fail(AW_WIFI_STA_CONNECT_ERR_DHCP);
        }
#endif
        break;
    }
    case DIVOOM_WIFI_EVENT_STOP_CONNECT:
    {
        int ret;

        /* 离开扫描页后，排队的停止请求不得再打断恢复的连接。 */
        if (!divoom_wifi_handle->cancle_connect_wifi)
        {
            g_divoom_wifi_stop_connect_pending = 0;
            WIFI_CFG_LOG("skip stale scan STA prepare request\n");
            break;
        }
        divoom_wifi_diag_log_state("stop_connect_begin");
        divoom_wifi_handle->start_connect_wifi = 0;
        ret = divoom_wifi_cancel_connect_for_scan();
        /* 2 表示清理失败，页面可重试准备；非零时仍禁止开始扫描。 */
        g_divoom_wifi_stop_connect_pending =
            (ret != 0 && divoom_wifi_handle->cancle_connect_wifi) ? 2 : 0;
        WIFI_CFG_LOG("scan STA prepare ret=%d pending=%d\n", ret,
            g_divoom_wifi_stop_connect_pending);
        divoom_wifi_diag_log_state("stop_connect_end");
        break;
    }
    case DIVOOM_WIFI_EVENT_STOP_MQTT:
    {
        /* 快速恢复 IP 时丢弃过期停止请求，恢复连接仍由 DHCP_SUCCESS 通知主业务。 */
        if (divoom_wifi_handle->wifi_status != AW_IP_EVENT_STA_GOT_IP)
        {
            divoom_mqtt_model_remove_server_connect();
            WIFI_CFG_LOG("WiFi offline, MQTT server reconnect stopped\n");
        }
        break;
    }
    case DIVOOM_WIFI_EVENT_ENTER_CONFIG:
    {
        WIFI_CFG_LOG("enter config mode, start BLE provisioning\n");
        divoom_wifi_handle->provisioning_active = 1;
        divoom_wifi_model_config();
        divoom_mqtt_model_remove_server_connect();
        if (divoom_wifi_handle->connect_status == DIVOOM_WIFI_CONNECTED)
        {
            divoom_wifi_set_wifi_time_out_flag(1);
            aw_wifi_disconnect();
            divoom_wifi_set_wifi_time_out_flag(2);
        }
        divoom_ble_set_wifi_status(DIVOOM_WIFI_NOT_CONFIG);
        divoom_ble_set_wifi_status(DIVOOM_WIFI_NOT_CONFIG);
        divoom_bt_ctrl_set_wifi_config(1);
        divoom_wifi_handle->cancle_connect_wifi = 0;
        divoom_wifi_handle->reconnect_user_resume = 0;
        break;
    }
    case DIVOOM_WIFI_EVENT_EXIT_CONFIG:
    {
        WIFI_CFG_LOG("exit config mode, stop BLE\n");
        divoom_wifi_handle->provisioning_active = 0;
        divoom_ble_stop(0);
        divoom_bt_ctrl_set_wifi_config(0);
        break;
    }
    case DIVOOM_WIFI_EVENT_INIT:
    {
        int saved = divoom_queue_items(divoom_wifi_handle->wifi_ssid_queue);

        if (divoom_wifi_handle->cancle_connect_wifi)
        {
            WIFI_CFG_LOG("skip boot reconnect while reconnect is paused\n");
            break;
        }
        if (divoom_product_test_is_active()) {
            WIFI_CFG_LOG("boot init, skip saved wifi (product test) saved=%d\n", saved);
            break;
        }
        WIFI_CFG_LOG("boot init, saved_wifi=%d\n", saved);
        if (saved > 0 && divoom_wifi_power_on_connect_wifi() == 0)
        {
            /* STA 启动或 join 同步失败时仍需进入统一的延时重连流程。 */
            divoom_wifi_connect_other_ssid();
        }
        break;
    }
    case DIVOOM_WIFI_EVENT_CONNECTED:
    {
        divoom_print("connected \n");
        /* In production recovery, association alone is insufficient: save
         * after DHCP so a failed connection cannot replace saved credentials. */
        if (!divoom_product_test_is_active())
            divoom_wifi_remember_current_network();
        divoom_lvgl_wifi_disconnect_exit();
        divoom_main_set_wifi_disconnect_notice_flag(0);
        divoom_wifi_handle->wifi_disconnect_time = 0;
        divoom_wifi_handle->reconnect_force_disconnect = 0;
        divoom_wifi_handle->reconnect_user_resume = 0;
        divoom_wifi_handle->reconnect_immediate = 0;
        divoom_wifi_handle->connect_status = DIVOOM_WIFI_CONNECTED;
        if (divoom_product_test_is_active() == 1)
            divoom_product_test_post(DIVOOM_PT_MSG_WIFI_UP, 0);
        divoom_ble_set_wifi_status(DIVOOM_WIFI_CONNECTED);
        divoom_ble_wifi_status_notify(WIFI_STATUS_BLE_COMMAND, 1, &divoom_wifi_handle->connect_status);
        if (!divoom_product_test_is_active() && divoom_wifi_handle->callback)
            divoom_wifi_handle->callback(DIVOOM_WIFI_CONNECTED, NULL);
        break;
    }
    case DIVOOM_WIFI_EVENT_DHCP_SUCCESS:
    {
        WIFI_CFG_LOG("DHCP success, ssid=%s\n", divoom_wifi_handle->ssid);
        if (divoom_product_test_is_active())
            divoom_wifi_remember_current_network();
#ifdef DIVOOM_SIMULATOR
        divoom_print("[sim_discon] free\n");
#endif
        divoom_lvgl_wifi_disconnect_exit();
        divoom_main_set_wifi_disconnect_notice_flag(0);
        divoom_print("dhcp success \n");
        divoom_wifi_manual_connect_finish(DIVOOM_WIFI_CONNECTED);
        if (divoom_wifi_handle->connect_status != DIVOOM_WIFI_CONNECTED)
        {
            divoom_wifi_handle->connect_status = DIVOOM_WIFI_CONNECTED;
            divoom_ble_set_wifi_status(DIVOOM_WIFI_CONNECTED);
            divoom_ble_wifi_status_notify(WIFI_STATUS_BLE_COMMAND, 1, &divoom_wifi_handle->connect_status);
            if (divoom_product_test_is_active() == 1)
                divoom_product_test_post(DIVOOM_PT_MSG_WIFI_UP, 0);
            if (!divoom_product_test_is_active() && divoom_wifi_handle->callback)
                divoom_wifi_handle->callback(DIVOOM_WIFI_CONNECTED, NULL);
        }
        if (divoom_product_test_is_active() == 1)
            divoom_product_test_post(DIVOOM_PT_MSG_START_OTA, 0);
        if (!divoom_product_test_is_active() && divoom_wifi_handle->callback)
        {
            if (g_divoom_wifi_whitebox_active)
            {
                WIFI_CFG_LOG("[WB] defer GOT_IP business notification seq=%u\n",
                    g_divoom_wifi_diag_connect_seq);
            }
            else
            {
                divoom_wifi_handle->callback(DIVOOM_WIFI_GOT_IP, NULL);
            }
        }
        break;
    }
    default:
        break;
    }
}

#ifdef DIVOOM_SIMULATOR
static uint8 s_wifi_con_use_poll;

void divoom_wifi_con_poll(void)
{
    divoom_wifi_msg *task_queue_handle;

    if (!s_wifi_con_use_poll || !divoom_wifi_handle || !divoom_wifi_handle->run_flag)
    {
        return;
    }
    while (divoom_queue_items(divoom_wifi_handle->queue))
    {
        task_queue_handle = divoom_queue_pop(divoom_wifi_handle->queue);
        if (task_queue_handle == NULL)
        {
            break;
        }
        divoom_wifi_con_dispatch_msg(divoom_wifi_handle, task_queue_handle);
        divoom_system_free(task_queue_handle);
    }
}
#endif /* DIVOOM_SIMULATOR */

void *divoom_wifi_con_task_main(void *arg)
{
    divoom_wifi_msg *task_queue_handle;
    Pdivoom_wifi_struct divoom_task_handle = (Pdivoom_wifi_struct)arg;


    while(divoom_task_handle->run_flag)
    {
        divoom_system_get_sem(divoom_task_handle->divoom_wifi_sem);

		if(divoom_task_handle->run_flag == 0)
		{
			break;
		}
		while(divoom_queue_items(divoom_task_handle->queue))
		{
			task_queue_handle = divoom_queue_pop(divoom_task_handle->queue);
			if(task_queue_handle == NULL)
			{
				break;
			}
            #ifndef DIVOOM_WIFI_RF_TEST
            divoom_wifi_con_dispatch_msg(divoom_task_handle, task_queue_handle);
            #endif
			divoom_system_free(task_queue_handle);

		}

    }
	return NULL;
}


/**
 * NAME         divoom_wifi_connect_ssid
 * @BRIEF	    连接WIFI，如果是同一个网络将不进行重连，否则直接使用新 aw_wifi 接口发起连接
 * @PARAM	    model_id
                event_id
                    
 * @RETURN	none
 * @RETVAL	
 */

int divoom_wifi_connect_ssid_in(char *ssid, char *password)
{    
    int ret;
#if DIVOOM_WIFI_DIAG_LOG_ENABLE
    int config_ret;
#endif
    uint8 switching_network;
    aw_wifi_config_t wifi_config;
#if DIVOOM_WIFI_DIAG_LOG_ENABLE
    aw_wifi_config_t readback_config;
#endif

    if(strlen(ssid) == 0)
    {
        return 0;
    }
    divoom_debug("start connect ssid:%s, pswd_len:%d", ssid, password ? (int)strlen(password) : 0);
    divoom_wifi_diag_log_state("connect_in_begin");
    if(divoom_wifi_ensure_sta_started() != 0)
    {
        g_divoom_wifi_manual_connect_error_reason = AW_WIFI_STA_CONNECT_ERR_VIF_CONFIG;
        divoom_wifi_manual_connect_finish(DIVOOM_WIFI_OTHER_ERR);
        divoom_wifi_resume_network_services();
        return 0;
    }
    if (divoom_wifi_handle->cancle_connect_wifi)
    {
        WIFI_CFG_LOG("skip join while reconnect is paused\n");
        return 0;
    }

    switching_network = aw_wifi_is_connected() ? 1 : 0;
    if(switching_network)
    {
        divoom_debug("connected sta ssid:%s, requested ssid:%s", divoom_wifi_handle->cur_ssid, ssid);
        if (!strcmp((char *)divoom_wifi_handle->cur_ssid, ssid)
            && divoom_wifi_handle->connect_status == DIVOOM_WIFI_CONNECTED)
        {
            divoom_print("==Wi-Fi connect successful, wifi already connected and not need to change.==\n");
            divoom_wifi_manual_connect_finish(DIVOOM_WIFI_CONNECTED);
            if (divoom_wifi_handle->callback)
            {
                divoom_wifi_handle->callback(DIVOOM_WIFI_CONNECTED, NULL);
                if (g_divoom_wifi_whitebox_active)
                {
                    WIFI_CFG_LOG("[WB] defer same-network GOT_IP notification seq=%u\n",
                        g_divoom_wifi_diag_connect_seq);
                }
                else
                {
                    divoom_wifi_handle->callback(DIVOOM_WIFI_GOT_IP, NULL);
                }
            }
            return 1;
        }
        divoom_print("wifi already connected but now need to change.==\n");
        /* 业务连接退出后，再由 AIC 异步连接任务清理旧 STA 会话。 */
        divoom_wifi_stop_old_network_services();
        g_divoom_wifi_switch_disconnect_pending = 1;
    }

    /* 新连接必须等待本轮 DHCP，不能沿用上一个网络残留的 ready 状态。 */
    divoom_wifi_handle->network_ready = 0;
    strcpy(divoom_wifi_handle->cur_ssid, ssid);
    divoom_wifi_handle->ssid_exist_flag = 0;
    divoom_wifi_handle->start_connect_wifi = 1;
    memset(&wifi_config, 0, sizeof(wifi_config));
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (password != NULL)
    {
        strncpy((char *)wifi_config.sta.password, password,
            sizeof(wifi_config.sta.password) - 1);
    }
    wifi_config.sta.authmode = (password != NULL && password[0] != 0)
        ? AW_WIFI_AUTH_WPA2_PSK : AW_WIFI_AUTH_OPEN;
    wifi_config.sta.channel = 0;
    ret = aw_wifi_set_config(AW_WIFI_IF_STA, &wifi_config);
    WIFI_DIAG_LOG("set_config seq=%u ret=%d auth=%d channel=%d ssid=%s\n",
        g_divoom_wifi_diag_connect_seq, ret, wifi_config.sta.authmode,
        wifi_config.sta.channel, ssid);
#if DIVOOM_WIFI_DIAG_LOG_ENABLE
    memset(&readback_config, 0, sizeof(readback_config));
    config_ret = aw_wifi_get_config(AW_WIFI_IF_STA, &readback_config);
    WIFI_DIAG_LOG("get_config seq=%u ret=%d ssid=%s pswd_len=%d "
        "auth=%d band=%d channel=%d bssid_set=%d\n",
        g_divoom_wifi_diag_connect_seq, config_ret,
        config_ret == 0 ? (char *)readback_config.sta.ssid : "",
        config_ret == 0 ? (int)strlen((char *)readback_config.sta.password) : -1,
        config_ret == 0 ? readback_config.sta.authmode : -1,
        config_ret == 0 ? readback_config.sta.band : -1,
        config_ret == 0 ? readback_config.sta.channel : -1,
        config_ret == 0 ? readback_config.sta.bssid_set : -1);
#endif
    if (ret == 0)
    {
        divoom_debug("aw_wifi_connect start ssid:%s pswd_len:%d", ssid,
            password ? (int)strlen(password) : 0);
        ret = aw_wifi_connect();
    }
    else
    {
        /* 配置同步失败属于 STA 状态异常，不能误报为密码错误。 */
        WIFI_CFG_LOG("aw_wifi_set_config failed ret=%d ssid=%s\n", ret, ssid);
        ret = AW_WIFI_STA_CONNECT_ERR_VIF_CONFIG;
    }

    if(ret != 0)
    {
        uint8 send_status = DIVOOM_WIFI_OTHER_ERR;

        if (divoom_wifi_handle->cancle_connect_wifi)
        {
            /* 扫描页主动中止 join，不得上报密码错误或触发下一轮重连。 */
            divoom_wifi_handle->start_connect_wifi = 0;
            g_divoom_wifi_manual_connect_error_reason = 0;
            divoom_wifi_resume_network_services();
            WIFI_CFG_LOG("aw_wifi_connect canceled for scan ret=%d ssid=%s\n", ret, ssid);
            return 0;
        }
        if(ret == AW_WIFI_STA_CONNECT_ERR_SCAN_LIMIT_REACHED)
        {
            send_status = DIVOOM_WIFI_NO_SSID;
        }
        g_divoom_wifi_manual_connect_error_reason = ret;
        divoom_wifi_manual_connect_finish(send_status);
        divoom_wifi_resume_network_services();
        g_divoom_wifi_switch_disconnect_pending = 0;
        WIFI_CFG_LOG("aw_wifi_connect start failed ret=%d ssid=%s\n", ret, ssid);
        divoom_print("aw_wifi_connect: fails: %d\n", ret);
        if(divoom_wifi_handle->callback)
        {
            divoom_wifi_handle->callback(send_status, NULL);
        }
        if(divoom_wifi_handle->send_password_err == 0)
        {
            divoom_wifi_handle->send_password_err = 1;
            divoom_ble_wifi_status_notify(WIFI_STATUS_BLE_COMMAND, 1, &send_status);
        }
        divoom_wifi_handle->start_connect_wifi = 0;
        return 0;
    }

    WIFI_CFG_LOG("aw_wifi_connect accepted seq=%u (async), wait STA_CONNECTED/GOT_IP\n",
        g_divoom_wifi_diag_connect_seq);
    divoom_wifi_diag_log_state("connect_in_accepted");
    divoom_debug("connect started:%s", ssid);
    return 1;
}

/**
 * NAME         divoom_wifi_connect_other_ssid
 * @BRIEF	    网络断开，连接其它网络
 * @PARAM	    model_id
                event_id
                    
 * @RETURN	none
 * @RETVAL	
 */

void divoom_wifi_connect_other_ssid(void)
{  
	divoom_wifi_msg msg;

    if (divoom_wifi_handle == NULL
        || divoom_wifi_handle->cancle_connect_wifi
        || divoom_wifi_handle->provisioning_active
        || divoom_wifi_handle->wifi_ssid_queue == NULL
        || divoom_queue_items(divoom_wifi_handle->wifi_ssid_queue) <= 0)
    {
        WIFI_DIAG_LOG("skip queue reconnect handle=%d cancel=%d provisioning=%d "
            "saved=%d start=%d manual=%d\n",
            divoom_wifi_handle ? 1 : 0,
            divoom_wifi_handle ? divoom_wifi_handle->cancle_connect_wifi : -1,
            divoom_wifi_handle ? divoom_wifi_handle->provisioning_active : -1,
            divoom_wifi_handle && divoom_wifi_handle->wifi_ssid_queue
                ? divoom_queue_items(divoom_wifi_handle->wifi_ssid_queue) : -1,
            divoom_wifi_handle ? divoom_wifi_handle->start_connect_wifi : -1,
            g_divoom_wifi_manual_connect_pending);
        return;
    }

    msg.msg_id = DIVOOM_WIFI_EVENT_RECONNECT;
    divoom_wifi_put_msg(&msg);

}

void divoom_wifi_stop_connect_wifi(void)
{
    uint8 join_active;
    uint8 prepare_scan;
    divoom_wifi_msg msg;

    if (divoom_wifi_handle == NULL)
    {
        return;
    }

    /* 扫描期间禁止已保存网络重连；正在 join/DHCP 时主动取消，避免 AIC 拒绝扫描。 */
    divoom_wifi_handle->cancle_connect_wifi = 1;
    divoom_wifi_handle->reconnect_force_disconnect = 0;
    divoom_wifi_handle->reconnect_user_resume = 0;
    divoom_wifi_handle->reconnect_immediate = 0;
    join_active = divoom_wifi_handle->start_connect_wifi;
    /* 仅清理正在执行或已明确失败的 join；正常离线状态保持 STA 不动。 */
    prepare_scan = (join_active
        || divoom_wifi_handle->connect_status == DIVOOM_WIFI_OTHER_ERR) ? 1 : 0;
    WIFI_DIAG_LOG("scan pause decision join=%d connect_status=%d event=%d "
        "radio=%d ready=%d prepare=%d queue=%d\n",
        join_active, divoom_wifi_handle->connect_status,
        divoom_wifi_handle->wifi_status, aw_wifi_is_connected() ? 1 : 0,
        divoom_wifi_handle->network_ready, prepare_scan,
        divoom_queue_items(divoom_wifi_handle->queue));
    g_divoom_wifi_manual_connect_pending = 0;
    g_divoom_wifi_manual_connect_result = DIVOOM_WIFI_DISCONNECTED;
    g_divoom_wifi_manual_connect_error_reason = 0;
    divoom_wifi_resume_network_services();
    if ((prepare_scan || g_divoom_wifi_stop_connect_pending == 2)
        && g_divoom_wifi_stop_connect_pending != 1)
    {
        g_divoom_wifi_stop_connect_pending = 1;
        msg.msg_id = DIVOOM_WIFI_EVENT_STOP_CONNECT;
        divoom_wifi_put_msg(&msg);
    }
    WIFI_CFG_LOG("saved WiFi reconnect paused for scan, join_active=%d prepare=%d\n",
        join_active, prepare_scan);
    divoom_wifi_diag_log_state("scan_pause_requested");
}

uint8 divoom_wifi_is_stop_connect_pending(void)
{
    return g_divoom_wifi_stop_connect_pending;
}

void divoom_wifi_allow_saved_reconnect(void)
{
    if (divoom_wifi_handle == NULL) return;
    /* Formatting owns the network; factory provisioning must not block reconnect. */
    divoom_wifi_handle->provisioning_active = 0;
    divoom_wifi_handle->cancle_connect_wifi = 0;
    WIFI_CFG_LOG("saved WiFi reconnect enabled\n");
}

void divoom_wifi_connect_wifi_in_nvs(void)
{
    uint8 radio_connected;

    if (divoom_wifi_handle == NULL)
    {
        return;
    }

    divoom_wifi_handle->cancle_connect_wifi = 0;
    radio_connected = aw_wifi_is_connected() ? 1 : 0;
    if (!radio_connected
        || divoom_wifi_handle->connect_status != DIVOOM_WIFI_CONNECTED)
    {
        /* 射频仍关联但业务层已丢 IP 时，先断开再重新 join，才能重新触发 DHCP。 */
        divoom_wifi_handle->reconnect_force_disconnect = radio_connected;
        divoom_wifi_handle->reconnect_user_resume = 1;
        WIFI_CFG_LOG("resume saved WiFi reconnect, radio=%d status=%d\n",
            radio_connected, divoom_wifi_handle->connect_status);
        divoom_wifi_connect_other_ssid();
    }
}

/**
 * NAME         divoom_wifi_connect_ssid
 * @BRIEF	    排队连接指定 WiFi，底层负责顺序替换正在执行的旧连接请求
 * @PARAM	    model_id
                event_id
                    
 * @RETURN	none
 * @RETVAL	
 */

void divoom_wifi_connect_ssid(char *ssid, char *password)
{  
	divoom_wifi_msg msg;
	
    if(divoom_wifi_handle == NULL)
    {
        g_divoom_wifi_manual_connect_result = DIVOOM_WIFI_OTHER_ERR;
        g_divoom_wifi_manual_connect_pending = 0;
        return;
    }
    if(strlen(ssid))
    {
        const char *connect_password = password ? password : "";

        if (g_divoom_wifi_manual_connect_pending
            && g_divoom_wifi_manual_connect_result == DIVOOM_WIFI_CONNECTING
            && strcmp((char *)divoom_wifi_handle->ssid, ssid) == 0
            && strcmp((char *)divoom_wifi_handle->pswd, connect_password) == 0)
        {
            /* 页面超时只延长等待，不得把仍在执行的同一连接再次提交给 AIC。 */
            WIFI_DIAG_LOG("keep waiting seq=%u ssid=%s elapsed=%us "
                "result=%d reason=%d(%s)\n",
                g_divoom_wifi_diag_connect_seq, ssid,
                g_divoom_wifi_diag_connect_start_tick == 0
                    || divoom_system_get_ticks() < g_divoom_wifi_diag_connect_start_tick ? 0
                    : divoom_system_get_ticks() - g_divoom_wifi_diag_connect_start_tick,
                g_divoom_wifi_manual_connect_result,
                g_divoom_wifi_manual_connect_error_reason,
                divoom_wifi_cfg_error_name(g_divoom_wifi_manual_connect_error_reason));
            return;
        }
        g_divoom_wifi_manual_connect_result = DIVOOM_WIFI_CONNECTING;
        g_divoom_wifi_manual_connect_error_reason = 0;
        g_divoom_wifi_manual_connect_pending = 1;
        WIFI_CFG_LOG("queue connect ssid=%s pswd_len=%d radio=%d status=%d "
            "event=%d cancel=%d manual=%d\n",
            ssid, password ? (int)strlen(password) : 0,
            aw_wifi_is_connected() ? 1 : 0, divoom_wifi_handle->connect_status,
            divoom_wifi_handle->wifi_status, divoom_wifi_handle->cancle_connect_wifi,
            g_divoom_wifi_manual_connect_pending);
        Pdivoom_wifi_con_ssid_info pssid_info;
        strcpy(divoom_wifi_handle->ssid, ssid);
        strcpy(divoom_wifi_handle->pswd, connect_password);
    	msg.msg_id = DIVOOM_WIFI_EVENT_CONNECT_WIFI;
    	divoom_wifi_put_msg(&msg);

		divoom_wifi_handle->product_run_flag = 0;
    }
}

Pdivoom_wifi_menu_wifi_info divoom_wifi_menu_get_all_wifi_info(int *wifi_num)
{
	int num;
	Pdivoom_wifi_con_ssid_info pwifi_info;
	Pdivoom_wifi_menu_wifi_info pwifi_list;

	if(wifi_num == NULL)
	{
		return NULL;
	}
	(*wifi_num) = 0;
	if(divoom_wifi_handle == NULL || divoom_wifi_handle->wifi_ssid_queue == NULL)
	{
		return NULL;
	}
	num = divoom_queue_items(divoom_wifi_handle->wifi_ssid_queue);
	if(num <= 0)
	{
		return NULL;
	}
	pwifi_list = (Pdivoom_wifi_menu_wifi_info)divoom_system_malloc(
		num * sizeof(divoom_wifi_menu_wifi_info));
	if(pwifi_list == NULL)
	{
		return NULL;
	}
	
	for(int i = 0;i < num; i++)
	{
		pwifi_info = divoom_queue_get_index_item(divoom_wifi_handle->wifi_ssid_queue, i);

		if(pwifi_info == NULL)
		{
			break;
		}

		memcpy(pwifi_list[*wifi_num].ssid, pwifi_info->ssid, sizeof(pwifi_list[*wifi_num].ssid));
		memcpy(pwifi_list[*wifi_num].pswd, pwifi_info->pswd, sizeof(pwifi_list[*wifi_num].pswd));
		
		(*wifi_num)++;
	}

	return pwifi_list;
}

Pdivoom_wifi_menu_wifi_info divoom_wifi_userdata_format_snapshot(int *count)
{
    if (!count) return NULL;
    *count = -1;
    if (!divoom_wifi_config_lock_take()) return NULL;
    if (!s_wifi_userdata_formatting) {
        divoom_system_free_lock_direct(s_wifi_config_lock); return NULL;
    }
    int expected = divoom_wifi_handle && divoom_wifi_handle->wifi_ssid_queue ?
        divoom_queue_items(divoom_wifi_handle->wifi_ssid_queue) : 0;
    Pdivoom_wifi_menu_wifi_info records = divoom_wifi_menu_get_all_wifi_info(count);
    if (expected > 0 && (!records || *count != expected)) {
        divoom_system_free(records); records = NULL; *count = -1;
    }
    divoom_system_free_lock_direct(s_wifi_config_lock);
    return records;
}

static int divoom_wifi_con_add_json_wifi_item(divoom_cJSON *item)
{
    divoom_cJSON *ssid_item;
    divoom_cJSON *password_item;
    Pdivoom_wifi_con_ssid_info pssid_info;

    if(item == NULL || (item->type & divoom_cJSON_Object) == 0)
    {
        return 0;
    }

    ssid_item = divoom_cJSON_GetObjectItem(item, "ssid");
    password_item = divoom_cJSON_GetObjectItem(item, "password");
    if(ssid_item == NULL || password_item == NULL ||
       (ssid_item->type & divoom_cJSON_String) == 0 ||
       (password_item->type & divoom_cJSON_String) == 0 ||
       ssid_item->valuestring == NULL || ssid_item->valuestring[0] == 0)
    {
        return 0;
    }

    pssid_info = divoom_system_malloc(sizeof(divoom_wifi_con_ssid_info));
    if(pssid_info == NULL)
    {
        return 0;
    }

    memset(pssid_info, 0, sizeof(divoom_wifi_con_ssid_info));
    strncpy((char *)pssid_info->ssid, ssid_item->valuestring, sizeof(pssid_info->ssid) - 1);
    strncpy((char *)pssid_info->pswd, password_item->valuestring, sizeof(pssid_info->pswd) - 1);
    divoom_queue_push(divoom_wifi_handle->wifi_ssid_queue, pssid_info);
    divoom_debug("find wifi json:%s", pssid_info->ssid);
    WIFI_CFG_LOG("load json item: ssid=%s pswd_len=%d\n",
        pssid_info->ssid, (int)strlen((char *)pssid_info->pswd));
    return 1;
}

/* Local maintenance window: only wifi.conf persistence is gated. Network
 * notifications and the existing Wi-Fi worker keep running. No driver calls,
 * user callbacks, or format lock are held while taking this lock. */
static int divoom_wifi_config_lock_take(void)
{
    void *lock = __atomic_load_n(&s_wifi_config_lock, __ATOMIC_ACQUIRE);
    if (!lock) {
        void *created = divoom_system_create_lock();
        if (!created) return 0;
        lock = __sync_val_compare_and_swap(&s_wifi_config_lock, NULL, created);
        if (lock) divoom_system_destroy_lock(created);
        else lock = created;
    }
    divoom_system_get_lock_direct(lock);
    return 1;
}

int divoom_wifi_userdata_format_begin(void)
{
    if (!divoom_wifi_config_lock_take()) return 0;
    /* Acquiring the same lock waits for every in-flight save to close its FD. */
    s_wifi_userdata_formatting = 1;
    divoom_system_free_lock_direct(s_wifi_config_lock);
    return 1;
}

void divoom_wifi_userdata_format_end(void)
{
    if (!divoom_wifi_config_lock_take()) return;
    s_wifi_userdata_formatting = 0;
    divoom_system_free_lock_direct(s_wifi_config_lock);
}

static int divoom_wifi_con_save_all_wifi_password_unlocked(void);
static int divoom_wifi_con_save_all_wifi_password(void)
{
    int rc = 0;
    if (!divoom_wifi_config_lock_take()) return -1;
    /* Late DHCP must neither reopen the old volume nor recreate erased data.
     * Keep the gate closed after erase starts, including failures, until reboot. */
    if (!s_wifi_userdata_formatting)
        rc = divoom_wifi_con_save_all_wifi_password_unlocked();
    divoom_system_free_lock_direct(s_wifi_config_lock);
    return rc;
}

static int divoom_wifi_con_save_all_wifi_password_unlocked(void)
{
    divoom_cJSON *root;
    divoom_cJSON *list;
    char *json_string;
    int i, num;

    if(divoom_wifi_handle == NULL || divoom_wifi_handle->wifi_ssid_queue == NULL)
    {
        return -1;
    }

    root = divoom_cJSON_CreateObject();
    list = divoom_cJSON_CreateArray();
    if(root == NULL || list == NULL)
    {
        if(root)
        {
            divoom_cJSON_Delete(root);
        }
        if(list)
        {
            divoom_cJSON_Delete(list);
        }
        return -1;
    }

    divoom_cJSON_AddItemToObject(root, "wifi", list);
    num = divoom_queue_items(divoom_wifi_handle->wifi_ssid_queue);
    for(i = 0; i < num; i++)
    {
        Pdivoom_wifi_con_ssid_info pssid_info = divoom_queue_get_index_item(divoom_wifi_handle->wifi_ssid_queue, i);
        divoom_cJSON *item;

        if(pssid_info == NULL || pssid_info->ssid[0] == 0)
        {
            continue;
        }

        item = divoom_cJSON_CreateObject();
        if(item == NULL)
        {
            continue;
        }
        divoom_cJSON_AddStringToObject(item, "ssid", (char *)pssid_info->ssid);
        divoom_cJSON_AddStringToObject(item, "password", (char *)pssid_info->pswd);
        divoom_cJSON_AddItemToArray(list, item);
    }

    json_string = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);
    if(json_string == NULL)
    {
        return -1;
    }

    void *file = NULL;
    int ok = 0;
    size_t length = strlen(json_string);
    if (divoom_port_mkdir_p(DIVOOM_USER_SYSTEM_DIR) == 0)
        file = divoom_port_file_open(DIVOOM_WPA_SUPPLICANT_CONF, "wb");
    if (file) {
        ok = divoom_port_file_write(json_string, 1, length, file) == length &&
            divoom_port_file_sync(file) == 0;
        if (divoom_port_file_close(file) != 0) ok = 0;
    }
    WIFI_CFG_LOG("save wifi.conf %s, networks=%d path=%s\n",
        ok ? "ok" : "failed", num, DIVOOM_WPA_SUPPLICANT_CONF);
    divoom_system_free(json_string);
    return ok ? 0 : -1;
}

int divoom_wifi_con_load_all_wifi_password(void)
{
    char *file_buf;
    int file_len;
    int i, load_num = 0;
    divoom_cJSON *root;
    divoom_cJSON *list;

    file_buf = divoom_app_com_read_file(DIVOOM_WPA_SUPPLICANT_CONF, &file_len);
    if(file_buf == NULL)
    {
        WIFI_CFG_LOG("load wifi.conf: file missing %s\n", DIVOOM_WPA_SUPPLICANT_CONF);
        return 0;
    }

    root = divoom_cJSON_Parse(file_buf);
    divoom_system_free(file_buf);
    if(root == NULL)
    {
        WIFI_CFG_LOG("load wifi.conf: json parse failed %s\n", DIVOOM_WPA_SUPPLICANT_CONF);
        divoom_print("wifi json parse failed: %s\n", DIVOOM_WPA_SUPPLICANT_CONF);
        return 0;
    }

    if((root->type & divoom_cJSON_Object) == 0)
    {
        divoom_cJSON_Delete(root);
        return 0;
    }

    list = divoom_cJSON_GetObjectItem(root, "wifi");
    if(list && (list->type & divoom_cJSON_Array))
    {
        for(i = 0; i < divoom_cJSON_GetArraySize(list); i++)
        {
            load_num += divoom_wifi_con_add_json_wifi_item(divoom_cJSON_GetArrayItem(list, i));
        }
    }

    divoom_cJSON_Delete(root);
    WIFI_CFG_LOG("load wifi.conf done, loaded=%d from %s\n",
        load_num, DIVOOM_WPA_SUPPLICANT_CONF);
    if (divoom_product_test_is_active())
        return load_num; /* Read credentials without normal business startup. */
    if(divoom_main_set_update_fireware_flag(-1) == 0)
    {
        if(divoom_main_change_to_gif_mode(-1) == 1)
        {
            divoom_wifi_clear_all_wifi();
        }
    }
    else
    {
        divoom_main_set_update_fireware_flag(0);
    }
    return load_num;
}


/*********************************************************
  * CREATED TIME	: 2018年 05月 24日 星期四 14:41:21 CST
  * FUNCTION		: divoom_wifi_model_init
  * IUPUT			: DIVOOM_WIFI_BLE_CB divoom_callback
  * OUTPUT			: void 
  * DESCRIPTION		: 如果配置过WiFi测开始连接，没有配置过，上报DIVOOM_WIFI_NOT_CONFIG = 5,     //没有配置过WiFi,等待外面调用配置WiFi接口
  *********************************************************/
uint8 divoom_wifi_model_init(DIVOOM_WIFI_BLE_CB divoom_callback, uint8 is_config_wifi)
{
	int ret = 0;
	WIFI_CFG_LOG("model_init is_config_wifi=%d\n", is_config_wifi);
	divoom_wifi_handle = (Pdivoom_wifi_struct)divoom_malloc(sizeof(divoom_wifi_struct), DIVOOM_MODEL_MAIN, __LINE__, __FILE__);
	if (divoom_wifi_handle == NULL)
	{
		divoom_print("Can't malloc memory for wifi!\n");
		return 0;
	}
	memset(divoom_wifi_handle, 0, sizeof(divoom_wifi_struct));
	divoom_wifi_handle->provisioning_active = is_config_wifi ? 1 : 0;
	if (divoom_callback)
		divoom_wifi_handle->callback = divoom_callback;
    
    #ifndef DIVOOM_SIMULATOR

	if(divoom_main_get_wifi_config_flag() == 0)
	{	
		divoom_wifi_handle->product_run_flag = 1;
	}
    #endif
	
	divoom_wifi_handle->run_flag = 1;
	divoom_wifi_handle->queue = divoom_queue_init(100, __func__, __LINE__);
    divoom_wifi_handle->wifi_ssid_queue = divoom_queue_init(100, __func__, __LINE__);
	if(divoom_wifi_handle->queue == NULL)
	{
		divoom_print("Queue creat fail!\n");
//		goto WRONG_EXIT;
	}
	divoom_wifi_con_load_all_wifi_password();

	divoom_wifi_handle->divoom_wifi_sem = divoom_system_create_sem(1, 0);
	if(divoom_wifi_handle->divoom_wifi_sem == NULL)
	{
		divoom_print("Queue creat fail!\n");
//		goto WRONG_EXIT;
	}
#ifdef DIVOOM_SIMULATOR
	/* 仿真不创建 wifi_con 线程，主循环必须消费联网事件队列，否则 MQTT 收不到 GOT_IP。 */
	s_wifi_con_use_poll = 1;
#else
	divoom_system_create_task(divoom_wifi_con_task_main, "wifi_con", divoom_wifi_handle, DIVOOM_TASK_STACK_WIFI_CON);
#endif

#ifdef DIVOOM_WIFI_RF_TEST
    return 1;
#endif
    aw_wifi_set_event_cb(divoom_wifi_event_handle, divoom_wifi_handle);

	divoom_wifi_msg msg;
	msg.msg_id = DIVOOM_WIFI_EVENT_INIT;
	divoom_wifi_put_msg(&msg);

	//每次都要看一下是不是需要配网，配网的话不需要链接以前的WiFi
	//但是nvs保存的信息不用清除
	//是否需要配网都要读取nvs里面的appid

#if 0
//TEST:直连路由器
	{
		char *ssid = "Divoom_Ota_Server";
		char *pswd = "divoom2020";

		memset(divoom_wifi_handle->ssid,0,sizeof(divoom_wifi_handle->ssid));
		memset(divoom_wifi_handle->pswd,0,sizeof(divoom_wifi_handle->pswd));
		strcpy((char *)divoom_wifi_handle->ssid, ssid);
		strcpy((char *)divoom_wifi_handle->pswd, pswd);
		divoom_wifi_handle->ssid_len = strlen(ssid)+1;
		divoom_wifi_handle->pswd_len = strlen(pswd)+1;

		return 1;
	}
#endif

	if (!is_config_wifi) 
	{
		//不用配网,获取ssid直接联网
#if 0	//已经直接用配置文件连接了，这里不再用去读配置去连接
		divoom_wifi_handle->ssid_len = sizeof(divoom_wifi_handle->ssid);
		divoom_wifi_handle->pswd_len = sizeof(divoom_wifi_handle->pswd);
		ret = divoom_power_on_manage_get_wifi_info((char *)divoom_wifi_handle->ssid, &divoom_wifi_handle->ssid_len, (char *)divoom_wifi_handle->pswd, &divoom_wifi_handle->pswd_len);
		if (ret == 0)
		{
			divoom_wifi_handle->callback(DIVOOM_WIFI_NOT_CONFIG, NULL);
			divoom_print("get ssid fail");
			return 0;
		}
#endif
		divoom_wifi_handle->wifi_start_flag = 1;
	}
    else
    {
        if (divoom_wifi_handle->callback)
            divoom_wifi_handle->callback(DIVOOM_WIFI_RECONFIG, NULL);
    }
	return 1;
}

/*********************************************************
  * CREATED TIME	: 2018年 06月 07日 星期四 20:21:53 CST
  * FUNCTION		: divoom_wifi_con_get_ble_msg
  * IUPUT			: uint8_t *ssid, uint8_t *pswd, uint8_t ssid_len, uint8_t pswd_len, Pdivoom_wifi_get_tz tz, Pdivoom_wifi_get_country_info country_info, uint8_t flag
  * OUTPUT			: void 
  * DESCRIPTION		: //传给BLE的callback,主要负责解析处理BLE解析出来的命令数据
  *********************************************************/
void divoom_wifi_con_get_ble_msg(uint8_t cmd, void *data)
{
	if(divoom_wifi_handle == NULL)
	{
		divoom_print("divoom_wifi_handle IS NULL!\n");
		return;
	}
	divoom_debug("command:%d", cmd);
	switch(cmd)
	{
		case DIVOOM_BLE_NO_DATA:
			break;
		case DIVOOM_BLE_CHECK_OK:
			break;
		case DIVOOM_BLE_GET_CONFIG:
			//if(divoom_wifi_handle && divoom_wifi_handle->config_flag != DIVOOM_WIFI_NOTNEED_CON)
			{
				Pdivoom_wifi_info_struct wifi_info = (Pdivoom_wifi_info_struct)data;
                WIFI_CFG_LOG("BLE GET_CONFIG ssid=%s pswd_len=%u app_id=%u\n",
                    wifi_info ? (char *)wifi_info->ssid : "(null)",
                    wifi_info ? (unsigned)wifi_info->pswd_len : 0u,
                    wifi_info ? (unsigned)wifi_info->app_id : 0u);
                
                strncpy( (char *)divoom_wifi_handle->ssid, (char *)wifi_info->ssid,wifi_info->ssid_len);
                strncpy( (char *)divoom_wifi_handle->pswd, (char *)wifi_info->pswd,wifi_info->pswd_len);
                divoom_wifi_handle->pswd_len = wifi_info->ssid_len;
                divoom_wifi_handle->pswd_len = wifi_info->pswd_len;

                divoom_wifi_connect_ssid(wifi_info->ssid, wifi_info->pswd);
				divoom_ble_set_wifi_status(DIVOOM_WIFI_CONNECTING);
				divoom_wifi_handle->wifi_start_flag = 1;
                divoom_wifi_handle->connect_status = DIVOOM_WIFI_CONNECTING;
                //divoom_main_disp_wifi_connecting(0, 10000);
				if(wifi_info->ssid)
				{
					divoom_free_line(wifi_info->ssid, __FILE__, __LINE__);
				}
				if(wifi_info->pswd)
				{
					divoom_free_line(wifi_info->pswd, __FILE__, __LINE__);
				}
                if (divoom_wifi_handle->callback)
                    divoom_wifi_handle->callback(DIVOOM_WIFI_RECONFIG, &wifi_info->app_id);
				divoom_free_line(wifi_info, __FILE__, __LINE__);
			}
			break;
		case DIVOOM_BLE_AP_STA_CONFIG:
		{
			divoom_wifi_ssid_pswd ssid_pswd = {0};
            divoom_debug("");
			// divoom_wifi_con_get_wifi_msg(&ssid_pswd);            
            divoom_wifi_connect_ssid(ssid_pswd.ssid, ssid_pswd.pswd);
			divoom_wifi_handle->wifi_start_flag = 1;
		}
			break;
		default:
			break;
	}
}

/*********************************************************************
  Function:        divoom_wifi_clear_all_wifi
  Description:     删除所有的本地网络，并断开所有链接
  Input:           none
  Return:          none
**********************************************************************/

void divoom_wifi_clear_all_wifi(void)
{
    Pdivoom_wifi_con_ssid_info pssid_info;

    while(divoom_queue_items(divoom_wifi_handle->wifi_ssid_queue))
    {
        pssid_info = divoom_queue_pop(divoom_wifi_handle->wifi_ssid_queue);
        if(pssid_info == NULL)
        {
            break;
        }
        divoom_debug("remove ssid:%s", pssid_info->ssid);
        divoom_system_free(pssid_info);
    }

    //断开网络，停止连接
    divoom_wifi_set_wifi_time_out_flag(1);
    aw_wifi_disconnect();
    divoom_wifi_set_wifi_time_out_flag(2);
    divoom_wifi_con_save_all_wifi_password();
}

//配网函数不需要调用这个函数，已经做好
//这个地方不用记录回复的句柄，因为蓝牙状态，BLE一直工作
void divoom_wifi_model_ble_init(DIVOOM_BLE_CALLBACK callback)
{
	if(divoom_wifi_handle == NULL)
	{
		divoom_print( "Must config wifi after wifi init!\n");
		return;
	}
	if(callback)
		divoom_ble_start(callback, divoom_wifi_handle->app_id);
}


//divoom_system_create_task_pro(divoom_ble_data_do, DIVOOM_BLE_TASK_NAME, divoom_ble_handle, DIVOOM_BLE_TASK_STACK*3,  0, 1, 0);


void divoom_wifi_reset_ble(void)
{
	divoom_wifi_msg msg;

	msg.msg_id = DIVOOM_WIFI_EVENT_RESET_BLE;
	divoom_wifi_put_msg(&msg);
}

int divoom_wifi_is_ready(void)
{
	return divoom_wifi_handle != NULL;
}

void divoom_wifi_set_config(int onoff)
{
	divoom_wifi_msg msg;

	if (divoom_wifi_handle == NULL)
	{
		divoom_print("wifi not ready, skip set_config(%d)\n", onoff);
		return;
	}

	if(onoff)
	{
        WIFI_CFG_LOG("set_config on=%d (enter provisioning)\n", onoff);
        if(onoff != 2)
        {
        	divoom_queue_pop_all(divoom_wifi_handle->wifi_ssid_queue, NULL);
        }
		divoom_wifi_handle->cancle_connect_wifi = 1;
		msg.msg_id = DIVOOM_WIFI_EVENT_ENTER_CONFIG;

	}
	else
	{
        WIFI_CFG_LOG("set_config on=0 (exit provisioning)\n");
		msg.msg_id = DIVOOM_WIFI_EVENT_EXIT_CONFIG;
	}
	divoom_wifi_put_msg(&msg);
}


/*********************************************************
  * CREATED TIME	: 2018年 05月 24日 星期四 16:49:38 CST
  * FUNCTION		: divoom_wifi_model_config
  * IUPUT			: DIVOOM_WIFI_BLE_CB divoom_callback
  * OUTPUT			: void 
  * DESCRIPTION		: 调用这个函数开始配网
  *********************************************************/
void divoom_wifi_model_config(void)
{
	if(divoom_wifi_handle == NULL)
	{
		WIFI_CFG_LOG("model_config: wifi handle NULL\n");
		divoom_print("Must config wifi after wifi init!\n");
		return;
	}
	WIFI_CFG_LOG("model_config: ble_start app_id=%u\n", (unsigned)divoom_wifi_handle->app_id);
	divoom_ble_start(divoom_wifi_con_get_ble_msg, divoom_wifi_handle->app_id);
}
//stop WiFi,stop ble(if init), ble not release mem, maybe no config wifi
int divoom_wifi_model_network_services_paused(void)
{
    return g_divoom_wifi_switch_services_paused != 0;
}

int divoom_wifi_model_is_network_ready(void)
{
    return divoom_wifi_handle != NULL && divoom_wifi_handle->network_ready;
}

int divoom_wifi_model_get_wifi_status(void)
{
#ifdef DIVOOM_SIMULATOR
    return 1;
#else
    if(divoom_wifi_handle == NULL)
    {
        return 1;
    }
    return divoom_wifi_handle->connect_status == DIVOOM_WIFI_CONNECTED || divoom_wifi_handle->connect_status == DIVOOM_WIFI_CONNECTED_SER;
    #endif
}

static void divoom_wifi_sta_link_clear_rssi(void)
{
	g_divoom_wifi_sta_link_info.rssi_valid = 0;
	g_divoom_wifi_sta_link_info.rssi = DIVOOM_WIFI_STA_RSSI_UNKNOWN;
}

#ifndef DIVOOM_SIMULATOR
int aw_wifi_sta_get_rssi(int8_t *rssi);
#endif

int divoom_wifi_model_get_sta_link_connected(void)
{
#ifdef DIVOOM_SIMULATOR
    return 0;
#else
    if (!g_divoom_wifi_sta_link_info.link_valid)
    {
        return 0;
    }
    return g_divoom_wifi_sta_link_info.link_connected ? 1 : 0;
#endif
}

int divoom_wifi_model_get_sta_rssi(int8 *rssi)
{
    if (rssi == NULL)
    {
        return -1;
    }

#ifdef DIVOOM_SIMULATOR
    return -1;
#else
    if (!g_divoom_wifi_sta_link_info.link_valid
        || !g_divoom_wifi_sta_link_info.link_connected
        || !g_divoom_wifi_sta_link_info.rssi_valid)
    {
        return -1;
    }

    *rssi = g_divoom_wifi_sta_link_info.rssi;
    return 0;
#endif
}

void divoom_wifi_model_refresh_sta_link(void)
{
#ifdef DIVOOM_SIMULATOR
    g_divoom_wifi_sta_link_info.link_valid = 0;
    g_divoom_wifi_sta_link_info.link_connected = 0;
    divoom_wifi_sta_link_clear_rssi();
#else
    int link_up = aw_wifi_is_connected() ? 1 : 0;

    g_divoom_wifi_sta_link_info.link_valid = 1;
    g_divoom_wifi_sta_link_info.link_connected = (uint8)link_up;
    if (!link_up)
    {
        divoom_wifi_sta_link_clear_rssi();
    }
#endif
}

void divoom_wifi_model_refresh_sta_rssi(void)
{
#ifdef DIVOOM_SIMULATOR
    return;
#else
    int8_t rssi;

    if (!g_divoom_wifi_sta_link_info.link_valid
        || !g_divoom_wifi_sta_link_info.link_connected)
    {
        return;
    }

    if (aw_wifi_sta_get_rssi(&rssi) != 0)
    {
        divoom_wifi_sta_link_clear_rssi();
        return;
    }

    g_divoom_wifi_sta_link_info.rssi = rssi;
    g_divoom_wifi_sta_link_info.rssi_valid = 1;
#endif
}

//stop WiFi,stop ble(if init), ble not release mem, maybe no config wifi
void divoom_wifi_model_deinit(void)
{
	if(divoom_wifi_handle)
	{
        divoom_ble_stop(0);
        divoom_bt_ctrl_stop_ble();
		//esp_event_loop_deinit();
		divoom_free(divoom_wifi_handle);
		divoom_wifi_handle = NULL;

		//不考虑刚好正在配网正在连接的情况,只要BLE还在运行,就不释放内存
		//if(divoom_ble_is_run())
			//divoom_ble_stop(0); //蓝牙初始化和停止需要在同一个cpu核上调用，否则会死机
	}
}




//测试模式给定SSID,PSWD直接连接WIFI
int divoom_wifi_model_test_mode_config_init(char *ssid,char *pswd, DIVOOM_WIFI_STATUS_CB divoom_callback)
{
    g_divoom_wifi_status_cb = divoom_callback;
    aw_wifi_set_event_cb(divoom_wifi_event_handle, divoom_wifi_handle);
    divoom_wifi_connect_ssid(ssid, pswd);
	return 1;
}




void divoom_wifi_save_config(void)
{
	if(NULL == divoom_wifi_handle)
		return;
#if 0
	//1.存wifi信息
	uint8 ret = divoom_power_on_manage_save_wifi_info((char *)divoom_wifi_handle->ssid, (char *)divoom_wifi_handle->pswd, divoom_wifi_handle->app_id);
	if (!ret)
	{
		divoom_print("divoom nvs save wifi error");
		return;
	}
#endif

	//2.设置下一次的开机模式
	divoom_power_on_manage_set_power_mode(DIVOOM_POWER_ON_WIFI_MODE, 0);
	divoom_print("[WIFI_CFG] save_config: next boot mode=WIFI_MODE\n");
}

#if DIVOOM_BUILD_RTOS_DEVICE && DIVOOM_WIFI_WHITEBOX_TEST_ENABLE

#define DIVOOM_WIFI_WB_STAGE_IDLE             0
#define DIVOOM_WIFI_WB_STAGE_CONNECT_A        1
#define DIVOOM_WIFI_WB_STAGE_INJECT_TO_B      2
#define DIVOOM_WIFI_WB_STAGE_INJECT_TO_A      3
#define DIVOOM_WIFI_WB_STAGE_DONE             4
#define DIVOOM_WIFI_WB_STAGE_FAILED           5
#define DIVOOM_WIFI_WB_STAGE_STOPPED          6
#define DIVOOM_WIFI_WB_CONNECT_TIMEOUT_SEC    135
#define DIVOOM_WIFI_WB_INJECT_TIMEOUT_SEC     15
#define DIVOOM_WIFI_WB_INJECT_DWELL_MS        1000
#define DIVOOM_WIFI_WB_POLL_MS                200
#define DIVOOM_WIFI_WB_TASK_STACK             4096
#define DIVOOM_WIFI_WB_MAX_CYCLES             65535

typedef struct _divoom_wifi_whitebox_state
{
    volatile uint8 running;
    volatile uint8 stop_requested;
    volatile uint8 stage;
    uint16 cycles;
    uint16 current_cycle;
    uint16 pass_count;
    uint16 fail_count;
} divoom_wifi_whitebox_state;

static divoom_wifi_whitebox_state g_divoom_wifi_whitebox_state;

/* 实验室白盒测试账号只编入真机固件；运行日志不得输出密码内容。 */
static const char g_divoom_wifi_wb_ssid_a[] = "Divoom_N_Ota_Server";
static const char g_divoom_wifi_wb_password_a[] = "divoom2020";
static const char g_divoom_wifi_wb_ssid_b[] = "divoom";
static const char g_divoom_wifi_wb_password_b[] = "Divoom603";
static const char g_divoom_wifi_wb_invalid_ssid[] = "DIVOOM_WB_OFFLINE_AP";
static const char g_divoom_wifi_wb_invalid_password[] = "aaaaaaaa";

static const char *divoom_wifi_whitebox_stage_name(uint8 stage)
{
    switch (stage)
    {
    case DIVOOM_WIFI_WB_STAGE_CONNECT_A:
    {
        return "CONNECT_A";
    }
    case DIVOOM_WIFI_WB_STAGE_INJECT_TO_B:
    {
        return "INVALID_TO_B";
    }
    case DIVOOM_WIFI_WB_STAGE_INJECT_TO_A:
    {
        return "INVALID_TO_A";
    }
    case DIVOOM_WIFI_WB_STAGE_DONE:
    {
        return "DONE";
    }
    case DIVOOM_WIFI_WB_STAGE_FAILED:
    {
        return "FAILED";
    }
    case DIVOOM_WIFI_WB_STAGE_STOPPED:
    {
        return "STOPPED";
    }
    default:
    {
        return "IDLE";
    }
    }
}

static void divoom_wifi_whitebox_log_state(const char *state_label,
    const char *target_ssid,
    uint32 elapsed_sec)
{
    WIFI_CFG_LOG("[WB] %s cycle=%u/%u stage=%s target=%s elapsed=%us "
        "seq=%u result=%d reason=%d(%s) radio=%d status=%d event=%d "
        "ready=%d start=%d manual=%d cancel=%d cur=%s\n",
        state_label,
        g_divoom_wifi_whitebox_state.current_cycle,
        g_divoom_wifi_whitebox_state.cycles,
        divoom_wifi_whitebox_stage_name(g_divoom_wifi_whitebox_state.stage),
        target_ssid ? target_ssid : "",
        elapsed_sec,
        g_divoom_wifi_diag_connect_seq,
        g_divoom_wifi_manual_connect_result,
        g_divoom_wifi_manual_connect_error_reason,
        divoom_wifi_cfg_error_name(g_divoom_wifi_manual_connect_error_reason),
        aw_wifi_is_connected() ? 1 : 0,
        divoom_wifi_handle ? divoom_wifi_handle->connect_status : -1,
        divoom_wifi_handle ? divoom_wifi_handle->wifi_status : -1,
        divoom_wifi_handle ? divoom_wifi_handle->network_ready : -1,
        divoom_wifi_handle ? divoom_wifi_handle->start_connect_wifi : -1,
        g_divoom_wifi_manual_connect_pending,
        divoom_wifi_handle ? divoom_wifi_handle->cancle_connect_wifi : -1,
        divoom_wifi_handle ? (char *)divoom_wifi_handle->cur_ssid : "");
}

static int divoom_wifi_whitebox_wait_connected(const char *target_ssid)
{
    uint32 start_tick;
    uint32 now_tick;
    uint32 elapsed_sec;
    uint32 next_log_sec = 5;
    int last_result = -1;
    int last_reason = -1;

    start_tick = divoom_system_get_ticks();
    while (!g_divoom_wifi_whitebox_state.stop_requested)
    {
        int result = divoom_wifi_get_manual_connect_result();
        int reason = divoom_wifi_get_manual_connect_error_reason();

        now_tick = divoom_system_get_ticks();
        elapsed_sec = now_tick >= start_tick ? now_tick - start_tick : 0;
        if (result == DIVOOM_WIFI_CONNECTED
            && divoom_wifi_handle
            && divoom_wifi_handle->network_ready
            && aw_wifi_is_connected()
            && strcmp((char *)divoom_wifi_handle->cur_ssid, target_ssid) == 0)
        {
            g_divoom_wifi_whitebox_state.pass_count++;
            divoom_wifi_whitebox_log_state("PASS", target_ssid, elapsed_sec);
            return 1;
        }
        if (result != last_result || reason != last_reason)
        {
            divoom_wifi_whitebox_log_state("TRANSITION", target_ssid, elapsed_sec);
            last_result = result;
            last_reason = reason;
        }
        if ((result == DIVOOM_WIFI_NO_SSID
            || result == DIVOOM_WIFI_OTHER_ERR)
            && !g_divoom_wifi_manual_connect_pending)
        {
            g_divoom_wifi_whitebox_state.fail_count++;
            divoom_wifi_whitebox_log_state("FAIL_RESULT", target_ssid, elapsed_sec);
            divoom_wifi_diag_log_state("whitebox_connect_failed");
            return 0;
        }
        if (elapsed_sec >= next_log_sec)
        {
            divoom_wifi_whitebox_log_state("HEARTBEAT", target_ssid, elapsed_sec);
            next_log_sec += 5;
        }
        if (elapsed_sec >= DIVOOM_WIFI_WB_CONNECT_TIMEOUT_SEC)
        {
            g_divoom_wifi_whitebox_state.fail_count++;
            divoom_wifi_whitebox_log_state("FAIL_TIMEOUT", target_ssid, elapsed_sec);
            divoom_wifi_diag_log_state("whitebox_connect_timeout");
            return 0;
        }
        divoom_system_delay_ms(DIVOOM_WIFI_WB_POLL_MS);
    }
    return -1;
}

static int divoom_wifi_whitebox_connect_and_wait(const char *ssid, const char *password)
{
    WIFI_CFG_LOG("[WB] REQUEST stage=%s ssid=%s pswd_len=%d\n",
        divoom_wifi_whitebox_stage_name(g_divoom_wifi_whitebox_state.stage),
        ssid, (int)strlen(password));
    divoom_wifi_connect_ssid((char *)ssid, (char *)password);
    return divoom_wifi_whitebox_wait_connected(ssid);
}

static int divoom_wifi_whitebox_inject_then_connect(const char *target_ssid,
    const char *target_password)
{
    uint32 seq_before;
    uint32 start_tick;
    uint32 elapsed_sec = 0;
    uint8 inject_started = 0;

    seq_before = g_divoom_wifi_diag_connect_seq;
    WIFI_CFG_LOG("[WB] INJECT_REQUEST invalid_ssid=%s pswd_len=%d seq_before=%u\n",
        g_divoom_wifi_wb_invalid_ssid,
        (int)strlen(g_divoom_wifi_wb_invalid_password), seq_before);
    divoom_wifi_connect_ssid((char *)g_divoom_wifi_wb_invalid_ssid,
        (char *)g_divoom_wifi_wb_invalid_password);
    start_tick = divoom_system_get_ticks();
    while (!g_divoom_wifi_whitebox_state.stop_requested)
    {
        uint32 now_tick = divoom_system_get_ticks();

        elapsed_sec = now_tick >= start_tick ? now_tick - start_tick : 0;
        if (divoom_wifi_handle
            && g_divoom_wifi_diag_connect_seq != seq_before
            && divoom_wifi_handle->start_connect_wifi
            && g_divoom_wifi_manual_connect_pending
            && g_divoom_wifi_manual_connect_result == DIVOOM_WIFI_CONNECTING
            && strcmp((char *)divoom_wifi_handle->cur_ssid,
                g_divoom_wifi_wb_invalid_ssid) == 0)
        {
            inject_started = 1;
            break;
        }
        if (elapsed_sec >= DIVOOM_WIFI_WB_INJECT_TIMEOUT_SEC)
        {
            break;
        }
        divoom_system_delay_ms(DIVOOM_WIFI_WB_POLL_MS);
    }
    if (g_divoom_wifi_whitebox_state.stop_requested)
    {
        return -1;
    }
    if (!inject_started)
    {
        g_divoom_wifi_whitebox_state.fail_count++;
        divoom_wifi_whitebox_log_state("FAIL_INJECT_NOT_STARTED",
            g_divoom_wifi_wb_invalid_ssid, elapsed_sec);
        divoom_wifi_diag_log_state("whitebox_inject_not_started");
        return 0;
    }

    divoom_wifi_whitebox_log_state("INJECT_ACTIVE",
        g_divoom_wifi_wb_invalid_ssid, elapsed_sec);
    divoom_system_delay_ms(DIVOOM_WIFI_WB_INJECT_DWELL_MS);
    WIFI_CFG_LOG("[WB] RACE_REQUEST target=%s while_invalid_seq=%u active\n",
        target_ssid, g_divoom_wifi_diag_connect_seq);
    divoom_wifi_connect_ssid((char *)target_ssid, (char *)target_password);
    return divoom_wifi_whitebox_wait_connected(target_ssid);
}

static void *divoom_wifi_whitebox_task(void *param)
{
    uint16 cycle;
    int ret = 1;

    (void)param;
    WIFI_CFG_LOG("[WB] START cycles=%u A=%s B=%s invalid=%s\n",
        g_divoom_wifi_whitebox_state.cycles,
        g_divoom_wifi_wb_ssid_a, g_divoom_wifi_wb_ssid_b,
        g_divoom_wifi_wb_invalid_ssid);
    for (cycle = 1; cycle <= g_divoom_wifi_whitebox_state.cycles; cycle++)
    {
        g_divoom_wifi_whitebox_state.current_cycle = cycle;
        g_divoom_wifi_whitebox_state.stage = DIVOOM_WIFI_WB_STAGE_CONNECT_A;
        ret = divoom_wifi_whitebox_connect_and_wait(g_divoom_wifi_wb_ssid_a,
            g_divoom_wifi_wb_password_a);
        if (ret != 1)
        {
            break;
        }

        g_divoom_wifi_whitebox_state.stage = DIVOOM_WIFI_WB_STAGE_INJECT_TO_B;
        ret = divoom_wifi_whitebox_inject_then_connect(g_divoom_wifi_wb_ssid_b,
            g_divoom_wifi_wb_password_b);
        if (ret != 1)
        {
            break;
        }

        g_divoom_wifi_whitebox_state.stage = DIVOOM_WIFI_WB_STAGE_INJECT_TO_A;
        ret = divoom_wifi_whitebox_inject_then_connect(g_divoom_wifi_wb_ssid_a,
            g_divoom_wifi_wb_password_a);
        if (ret != 1)
        {
            break;
        }
    }

    if (g_divoom_wifi_whitebox_state.stop_requested || ret < 0)
    {
        g_divoom_wifi_whitebox_state.stage = DIVOOM_WIFI_WB_STAGE_STOPPED;
    }
    else if (ret == 1)
    {
        g_divoom_wifi_whitebox_state.stage = DIVOOM_WIFI_WB_STAGE_DONE;
    }
    else
    {
        g_divoom_wifi_whitebox_state.stage = DIVOOM_WIFI_WB_STAGE_FAILED;
    }
    divoom_wifi_whitebox_log_state("SUMMARY", NULL, 0);
    WIFI_CFG_LOG("[WB] SUMMARY result=%s pass=%u fail=%u cycles=%u/%u\n",
        divoom_wifi_whitebox_stage_name(g_divoom_wifi_whitebox_state.stage),
        g_divoom_wifi_whitebox_state.pass_count,
        g_divoom_wifi_whitebox_state.fail_count,
        g_divoom_wifi_whitebox_state.current_cycle,
        g_divoom_wifi_whitebox_state.cycles);
    g_divoom_wifi_whitebox_active = 0;
    g_divoom_wifi_whitebox_state.running = 0;
    if (g_divoom_wifi_manual_connect_result == DIVOOM_WIFI_CONNECTED
        && divoom_wifi_handle && divoom_wifi_handle->network_ready
        && aw_wifi_is_connected()
        && !divoom_product_test_is_active() && divoom_wifi_handle->callback)
    {
        WIFI_CFG_LOG("[WB] resume GOT_IP business notification after test\n");
        divoom_wifi_handle->callback(DIVOOM_WIFI_GOT_IP, NULL);
    }
    return NULL;
}

static void divoom_wifi_whitebox_print_help(void)
{
    divoom_print("wifi_wb [start] [cycles] : A -> invalid -> B -> invalid -> A\n");
    divoom_print("cycles range: 1..%d\n", DIVOOM_WIFI_WB_MAX_CYCLES);
    divoom_print("wifi_wb status           : print current whitebox state\n");
    divoom_print("wifi_wb stop             : stop test polling\n");
}

static int divoom_wifi_whitebox_first_arg_index(int argc, char **argv)
{
    if (argc > 0 && argv[0] && strcmp(argv[0], "wifi_wb") == 0)
    {
        return 1;
    }
    return 0;
}

static int divoom_wifi_whitebox_cli(int argc, char **argv)
{
    int cycles = 1;
    int arg_index;
    const char *action;
    const char *cycles_arg = NULL;
    void *task_handle;

    arg_index = divoom_wifi_whitebox_first_arg_index(argc, argv);
    action = argc > arg_index ? argv[arg_index] : NULL;
    WIFI_CFG_LOG("[WB] CLI argc=%d arg0=%s arg1=%s arg2=%s\n",
        argc,
        argc > 0 && argv[0] ? argv[0] : "",
        argc > 1 && argv[1] ? argv[1] : "",
        argc > 2 && argv[2] ? argv[2] : "");
    if (action && strcmp(action, "status") == 0)
    {
        divoom_wifi_whitebox_log_state("STATUS", NULL, 0);
        return 0;
    }
    if (action && strcmp(action, "stop") == 0)
    {
        g_divoom_wifi_whitebox_state.stop_requested = 1;
        WIFI_CFG_LOG("[WB] stop requested\n");
        return 0;
    }
    if (action && strcmp(action, "start") == 0)
    {
        if (argc > arg_index + 1)
        {
            cycles_arg = argv[arg_index + 1];
        }
    }
    else
    {
        cycles_arg = action;
    }
    if (cycles_arg)
    {
        cycles = atoi(cycles_arg);
    }
    if (cycles < 1 || cycles > DIVOOM_WIFI_WB_MAX_CYCLES)
    {
        WIFI_CFG_LOG("[WB] invalid cycles=%d\n", cycles);
        divoom_wifi_whitebox_print_help();
        return -1;
    }
    if (divoom_wifi_handle == NULL)
    {
        WIFI_CFG_LOG("[WB] WiFi module is not initialized\n");
        return -1;
    }
    if (g_divoom_wifi_whitebox_state.running)
    {
        WIFI_CFG_LOG("[WB] test is already running\n");
        divoom_wifi_whitebox_log_state("STATUS", NULL, 0);
        return -1;
    }

    memset(&g_divoom_wifi_whitebox_state, 0,
        sizeof(g_divoom_wifi_whitebox_state));
    g_divoom_wifi_whitebox_active = 1;
    g_divoom_wifi_whitebox_state.running = 1;
    g_divoom_wifi_whitebox_state.cycles = (uint16)cycles;
    WIFI_CFG_LOG("[WB] parsed cycles=%d\n", cycles);
    task_handle = divoom_system_create_task(divoom_wifi_whitebox_task,
        "wifi_wb", NULL, DIVOOM_WIFI_WB_TASK_STACK);
    if (task_handle == NULL)
    {
        g_divoom_wifi_whitebox_active = 0;
        g_divoom_wifi_whitebox_state.running = 0;
        WIFI_CFG_LOG("[WB] create task failed\n");
        return -1;
    }
    return 0;
}

FINSH_FUNCTION_EXPORT_CMD(divoom_wifi_whitebox_cli, wifi_wb,
    WiFi invalid SSID switch whitebox test);

#endif /* DIVOOM_BUILD_RTOS_DEVICE && DIVOOM_WIFI_WHITEBOX_TEST_ENABLE */




#endif

#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_menu_direct_ctrl.h"
#include "divoom_main.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_astrootoo_settings_wifi.h"
#include "divoom_astrootoo_theme.h"
#include "divoom_wifi_con.h"
#include "divoom_lvgl_play_gif_file.h"
#include "divoom_pixel_format.h"
#include <stdio.h>
#include <string.h>
#ifdef DIVOOM_SIMULATOR
#include "sim_aic_wifi.h"
/* 仿真 LVGL fork 保留旧图像 API，枢轴坐标语义与真机接口相同。 */
#define lv_image_set_pivot lv_img_set_pivot
#else
#include "aic_wifi.h"
#endif
#ifndef DIVOOM_USE_RK_IC
#include "wifimg.h"
#endif

#define DIVOOM_ASTROTOO_SETTINGS_UI_DIR     "26/"
#define DIVOOM_ASTROTOO_SETTINGS_HEADER     DIVOOM_ASTROTOO_SETTINGS_UI_DIR "at_settings_header.png"
#define DIVOOM_ASTROTOO_SETTINGS_CHEVRON    DIVOOM_ASTROTOO_SETTINGS_UI_DIR "at_settings_chevron.png"

/*
 * Wi-Fi 二级页
 * 真源: AstroTooUI DeviceSettingsWifiScreen.tsx + DeviceSettings.module.css
 *   (SettingsValueRow / SettingsModuleRow / SettingsDeviceRow)
 */

#define DIVOOM_ASTRO_SET_WIFI_CHECK_ICON    "23/at_repeat_check.png"
#define DIVOOM_ASTRO_SET_WIFI_SAVED_ICON    "126/at_wifi_saved.png"
#define DIVOOM_ASTRO_SET_WIFI_RSSI_UNKNOWN  0
#define DIVOOM_ASTRO_SET_WIFI_ROW_W         444
#define DIVOOM_ASTRO_SET_WIFI_ROW_H         72
#define DIVOOM_ASTRO_SET_WIFI_ROW_GAP       6
#define DIVOOM_ASTRO_SET_WIFI_INFO_GAP      6
#define DIVOOM_ASTRO_SET_WIFI_ROW_X         ((DIVOOM_SCREEN_WIDTH - DIVOOM_ASTRO_SET_WIFI_ROW_W) / 2)
#define DIVOOM_ASTRO_SET_WIFI_TOP_GAP       8
#define DIVOOM_ASTRO_SET_WIFI_INFO_VALUE_W  260
#define DIVOOM_ASTRO_SET_WIFI_INFO_VALUE_H  32
#define DIVOOM_ASTRO_SET_WIFI_SECTION_H     38
#define DIVOOM_ASTRO_SET_WIFI_SEPARATOR_H   3
#define DIVOOM_ASTRO_SET_WIFI_SEPARATOR_PAD 6
#define DIVOOM_ASTRO_SET_WIFI_INFO_ROWS     4
/* 根页固定行：添加网络 + 设备连接码（可聚焦）；已保存网络与信息区在 rebuild 中追加。 */
#define DIVOOM_ASTRO_SET_WIFI_ROOT_BASE_ROWS 2
#define DIVOOM_ASTRO_SET_WIFI_SCROLL_SLACK_PX 2

#define DIVOOM_ASTRO_WIFI_PASSWORD_MIN_LEN   8
#define DIVOOM_ASTRO_WIFI_PASSWORD_MAX_LEN   63
#define DIVOOM_ASTRO_WIFI_PASSWORD_TITLE_Y   16
#define DIVOOM_ASTRO_WIFI_PASSWORD_FIELD_X   6
#define DIVOOM_ASTRO_WIFI_PASSWORD_FIELD_Y   58
#define DIVOOM_ASTRO_WIFI_PASSWORD_FIELD_W   336
#define DIVOOM_ASTRO_WIFI_PASSWORD_FIELD_H   62
#define DIVOOM_ASTRO_WIFI_PASSWORD_TOOL_W    58
#define DIVOOM_ASTRO_WIFI_PASSWORD_TOOL_H    62
#define DIVOOM_ASTRO_WIFI_PASSWORD_CONFIRM_X 350
#define DIVOOM_ASTRO_WIFI_PASSWORD_CLEAR_X   416
#define DIVOOM_ASTRO_WIFI_PASSWORD_TOOL_COUNT 2
#define DIVOOM_ASTRO_WIFI_PASSWORD_ZONE_KEYBOARD 0
#define DIVOOM_ASTRO_WIFI_PASSWORD_ZONE_TOOLBAR  1
#define DIVOOM_ASTRO_WIFI_PASSWORD_CONFIRM_ICON "129/at_wifi_confirm.png"
#define DIVOOM_ASTRO_WIFI_PASSWORD_CLEAR_ICON   "129/at_wifi_clear.png"
#define DIVOOM_ASTRO_WIFI_PASSWORD_BACKSPACE_ICON "129/at_wifi_backspace.png"
#define DIVOOM_ASTRO_WIFI_PASSWORD_SHIFT_ICON     "129/at_wifi_shift.png"
#define DIVOOM_ASTRO_WIFI_PASSWORD_SPACE_ICON     "129/at_wifi_space.png"
#define DIVOOM_ASTRO_WIFI_ERROR_ICON               "126/at_wifi_password_error.png"
#define DIVOOM_ASTRO_WIFI_LOADING_GIF              "126/at_wifi_loading.gif"
#define DIVOOM_ASTRO_WIFI_LOADING_W                64
#define DIVOOM_ASTRO_WIFI_LOADING_H                64
#define DIVOOM_ASTRO_WIFI_LOADING_X                128
#define DIVOOM_ASTRO_WIFI_LOADING_Y                22
#define DIVOOM_ASTRO_WIFI_LOADING_BUF_BYTES        (DIVOOM_ASTRO_WIFI_LOADING_W * DIVOOM_ASTRO_WIFI_LOADING_H * 3)
#define DIVOOM_ASTRO_WIFI_SUCCESS_ICON             "126/at_wifi_connect_success.png"
#define DIVOOM_ASTRO_WIFI_CONNECT_TIMEOUT_MS       60000U
#define DIVOOM_ASTRO_WIFI_STATUS_AUTO_DISMISS_MS   2200U
#ifdef DIVOOM_SIMULATOR
/* 仿真错误密码固定等待 5 秒后直接显示失败，真机仍按需求重试三次。 */
#define DIVOOM_ASTRO_WIFI_CONNECT_MAX_ATTEMPTS     1
#else
#define DIVOOM_ASTRO_WIFI_CONNECT_MAX_ATTEMPTS     3
#endif
#define DIVOOM_ASTRO_WIFI_SCAN_TASK_STACK           (4 * 1024)
#define DIVOOM_ASTRO_WIFI_SCAN_NOTICE_DELAY_MS       300U
#define DIVOOM_ASTRO_WIFI_SCAN_RETRY_MS             1000U
#define DIVOOM_ASTRO_WIFI_SCAN_ROUNDS                   3
#define DIVOOM_ASTRO_WIFI_SCAN_ROUND_GAP_MS             150U
#define DIVOOM_ASTRO_WIFI_SCAN_TIMEOUT_MS          60000U
#define DIVOOM_ASTRO_WIFI_MAC_RETRY_MS               1000U
/* 连接等待期间每 5 秒输出一次状态快照，避免 100ms UI timer 淹没串口。 */
#define DIVOOM_ASTRO_WIFI_DIAG_HEARTBEAT_MS           5000U
#define DIVOOM_ASTRO_WIFI_STATUS_CARD_X             80
#define DIVOOM_ASTRO_WIFI_STATUS_CARD_Y             150
#define DIVOOM_ASTRO_WIFI_STATUS_CARD_W             320
#define DIVOOM_ASTRO_WIFI_STATUS_CARD_H             180
#define DIVOOM_ASTRO_WIFI_CONFIRM_APP_PAIRING       (-2)

enum
{
    DIVOOM_ASTRO_SET_WIFI_KIND_INFO = 0,
    DIVOOM_ASTRO_SET_WIFI_KIND_ADD,
    DIVOOM_ASTRO_SET_WIFI_KIND_CONNECTION_CODE,
    DIVOOM_ASTRO_SET_WIFI_KIND_NETWORK,
};

enum
{
    DIVOOM_ASTRO_WIFI_KEY_CHAR = 0,
    DIVOOM_ASTRO_WIFI_KEY_BACKSPACE,
    DIVOOM_ASTRO_WIFI_KEY_SHIFT,
    DIVOOM_ASTRO_WIFI_KEY_SPACE,
};

typedef struct
{
    const char *text;
    int16 x;
    int16 y;
    int16 w;
    int16 h;
    uint8 row;
    uint8 col;
    uint8 col_span;
    uint8 kind;
} divoom_astrootoo_wifi_password_key_t;

typedef struct
{
    char ssid[AW_WIFI_SSID_LEN + 1];
    char password[DIVOOM_ASTRO_WIFI_PASSWORD_MAX_LEN + 1];
    uint8 authmode;
    uint8 connect_state;
    uint8 connect_attempt;
    uint8 first_connect;
    uint8 had_failure;
    uint8 awaiting_route;
    int8 focus_net_idx;
    uint32 connect_started_ms;
    uint32 result_started_ms;
} divoom_astrootoo_wifi_flow_t;

typedef struct
{
    volatile uint8 status;
    int count;
    uint32 started_ms;
    char ssid[DIVOOM_ASTRO_SET_WIFI_MAX_ROW][AW_WIFI_SSID_LEN + 1];
    uint8 authmode[DIVOOM_ASTRO_SET_WIFI_MAX_ROW];
    int8 rssi[DIVOOM_ASTRO_SET_WIFI_MAX_ROW];
} divoom_astrootoo_wifi_scan_cache_t;

enum
{
    DIVOOM_ASTRO_WIFI_SCAN_IDLE = 0,
    DIVOOM_ASTRO_WIFI_SCAN_RUNNING,
    DIVOOM_ASTRO_WIFI_SCAN_READY,
    DIVOOM_ASTRO_WIFI_SCAN_FAILED,
};

enum
{
    DIVOOM_ASTRO_WIFI_CONNECT_IDLE = 0,
    DIVOOM_ASTRO_WIFI_CONNECT_RUNNING,
    DIVOOM_ASTRO_WIFI_CONNECT_FAILED,
    DIVOOM_ASTRO_WIFI_CONNECT_SUCCESS,
};

enum
{
    DIVOOM_ASTRO_WIFI_NOTICE_SCANNING = 0,
    DIVOOM_ASTRO_WIFI_NOTICE_CONNECTING,
    DIVOOM_ASTRO_WIFI_NOTICE_SUCCESS,
    DIVOOM_ASTRO_WIFI_NOTICE_ERROR,
    DIVOOM_ASTRO_WIFI_NOTICE_SCAN_ERROR,
};

/* 坐标逐项抄自 AstroTooUI flowtooWifiPasswordKeyboardLayout.ts。 */
static const divoom_astrootoo_wifi_password_key_t g_wifi_password_keys[] =
{
    { "a", 6, 130, 38, 44, 0, 0, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "b", 48, 130, 38, 44, 0, 1, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "c", 90, 130, 38, 44, 0, 2, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "d", 132, 130, 38, 44, 0, 3, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "e", 174, 130, 38, 44, 0, 4, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "f", 216, 130, 38, 44, 0, 5, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "g", 258, 130, 38, 44, 0, 6, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { NULL, 300, 130, 44, 44, 0, 7, 1, DIVOOM_ASTRO_WIFI_KEY_BACKSPACE },
    { "1", 348, 130, 38, 44, 0, 8, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "2", 392, 130, 38, 44, 0, 9, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "3", 436, 130, 38, 44, 0, 10, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "h", 6, 180, 38, 44, 1, 0, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "i", 48, 180, 38, 44, 1, 1, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "j", 90, 180, 38, 44, 1, 2, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "k", 132, 180, 38, 44, 1, 3, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "l", 174, 180, 38, 44, 1, 4, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "m", 216, 180, 38, 44, 1, 5, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "n", 258, 180, 38, 44, 1, 6, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "4", 348, 180, 38, 44, 1, 8, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "5", 392, 180, 38, 44, 1, 9, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "6", 436, 180, 38, 44, 1, 10, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "o", 6, 230, 38, 44, 2, 0, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "p", 48, 230, 38, 44, 2, 1, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "q", 90, 230, 38, 44, 2, 2, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "r", 132, 230, 38, 44, 2, 3, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "s", 174, 230, 38, 44, 2, 4, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "t", 216, 230, 38, 44, 2, 5, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "u", 258, 230, 38, 44, 2, 6, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "7", 348, 230, 38, 44, 2, 8, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "8", 392, 230, 38, 44, 2, 9, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "9", 436, 230, 38, 44, 2, 10, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "v", 6, 280, 38, 44, 3, 0, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "w", 48, 280, 38, 44, 3, 1, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "x", 90, 280, 38, 44, 3, 2, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "y", 132, 280, 38, 44, 3, 3, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "z", 174, 280, 38, 44, 3, 4, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { NULL, 216, 280, 82, 44, 3, 5, 2, DIVOOM_ASTRO_WIFI_KEY_SHIFT },
    { "0", 348, 280, 82, 44, 3, 8, 2, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { ".", 436, 280, 38, 44, 3, 10, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "-", 6, 330, 38, 44, 4, 0, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "=", 49, 330, 38, 44, 4, 1, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "%", 92, 330, 38, 44, 4, 2, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "*", 135, 330, 38, 44, 4, 3, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "/", 178, 330, 38, 44, 4, 4, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "\\", 221, 330, 38, 44, 4, 5, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "@", 264, 330, 38, 44, 4, 6, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "#", 307, 330, 38, 44, 4, 7, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "$", 350, 330, 38, 44, 4, 8, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "&", 393, 330, 38, 44, 4, 9, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "~", 436, 330, 38, 44, 4, 10, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "(", 6, 380, 38, 44, 5, 0, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { ")", 49, 380, 38, 44, 5, 1, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "?", 92, 380, 38, 44, 5, 2, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "[", 135, 380, 38, 44, 5, 3, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "]", 178, 380, 38, 44, 5, 4, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "{", 221, 380, 38, 44, 5, 5, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "}", 264, 380, 38, 44, 5, 6, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "|", 307, 380, 38, 44, 5, 7, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "!", 350, 380, 38, 44, 5, 8, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { ",", 393, 380, 38, 44, 5, 9, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { ";", 436, 380, 38, 44, 5, 10, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "<", 6, 430, 38, 44, 6, 0, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { ">", 49, 430, 38, 44, 6, 1, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { NULL, 92, 430, 166, 44, 6, 2, 4, DIVOOM_ASTRO_WIFI_KEY_SPACE },
    { "\"", 264, 430, 38, 44, 6, 6, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { ":", 307, 430, 38, 44, 6, 7, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "'", 350, 430, 38, 44, 6, 8, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "+", 393, 430, 38, 44, 6, 9, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
    { "-", 436, 430, 38, 44, 6, 10, 1, DIVOOM_ASTRO_WIFI_KEY_CHAR },
};

static divoom_astrootoo_wifi_flow_t g_wifi_flow;
static divoom_astrootoo_wifi_scan_cache_t g_wifi_scan_cache;
static aw_wifi_ap_record_t g_wifi_scan_live_records[DIVOOM_ASTRO_SET_WIFI_MAX_ROW];
static volatile uint8 g_wifi_scan_cancel_requested;
static uint8 g_wifi_scan_timeout_reported;
static volatile uint8 g_wifi_app_pairing_prompt_pending;
static volatile uint8 g_wifi_app_pairing_prompt_open;
static volatile uint8 g_wifi_status_refresh_after_network_change;
/* 诊断序号只用于把多轮扫描和页面重试串起来，不参与业务判断。 */
static volatile uint32 g_wifi_diag_scan_seq;
static uint32 g_wifi_diag_connect_heartbeat_ms;

static void divoom_astrootoo_settings_wifi_wait_notice_create(
    Pdivoom_astrootoo_settings_ctrl phandle);
static int divoom_astrootoo_settings_wifi_ensure_loading_buf(
    Pdivoom_astrootoo_settings_ctrl phandle);
static void divoom_astrootoo_settings_wifi_loading_gif_stop(
    Pdivoom_astrootoo_settings_ctrl phandle);
static void divoom_astrootoo_settings_wifi_loading_gif_start(
    Pdivoom_astrootoo_settings_ctrl phandle);
static void divoom_astrootoo_settings_wifi_connect_notice_hide(
    Pdivoom_astrootoo_settings_ctrl phandle);
static void divoom_astrootoo_settings_wifi_connect_notice_show(
    Pdivoom_astrootoo_settings_ctrl phandle, uint8 variant);
static void divoom_astrootoo_settings_wifi_connect_begin(
    Pdivoom_astrootoo_settings_ctrl phandle, int net_idx, const char *password,
    uint8 first_connect);
static void divoom_astrootoo_settings_wifi_scan_start(void);
static void divoom_astrootoo_settings_wifi_scan_apply(
    Pdivoom_astrootoo_settings_ctrl phandle);
static void divoom_astrootoo_settings_wifi_scan_refresh(
    Pdivoom_astrootoo_settings_ctrl phandle);
static void divoom_astrootoo_settings_wifi_scan_spinner_show(
    Pdivoom_astrootoo_settings_ctrl phandle);
static void divoom_astrootoo_settings_wifi_scan_cancel_ui(
    Pdivoom_astrootoo_settings_ctrl phandle);
static void divoom_astrootoo_settings_wifi_connect_process(
    Pdivoom_astrootoo_settings_ctrl phandle);
static void divoom_astrootoo_settings_wifi_toast_hide(
    Pdivoom_astrootoo_settings_ctrl phandle);
static void divoom_astrootoo_settings_wifi_toast_show(
    Pdivoom_astrootoo_settings_ctrl phandle, const char *text, uint32 color);
static int divoom_astrootoo_settings_wifi_get_connected_ssid(
    char *ssid, int ssid_len);
static void divoom_astrootoo_settings_wifi_disconnect_modal_show(
    Pdivoom_astrootoo_settings_ctrl phandle, int net_idx);
static void divoom_astrootoo_settings_wifi_app_pairing_modal_show(
    Pdivoom_astrootoo_settings_ctrl phandle);

static int divoom_astrootoo_settings_wifi_row_focusable(uint8 kind)
{
    return (kind == DIVOOM_ASTRO_SET_WIFI_KIND_ADD
        || kind == DIVOOM_ASTRO_SET_WIFI_KIND_CONNECTION_CODE
        || kind == DIVOOM_ASTRO_SET_WIFI_KIND_NETWORK);
}

static void divoom_astrootoo_settings_wifi_rebuild_direct_focus(
    Pdivoom_astrootoo_settings_ctrl phandle)
{
    int i;

    if (!phandle)
    {
        return;
    }
    if (phandle->direct_handle)
    {
        divoom_menu_direct_ctrl_destory(phandle->direct_handle);
    }
    phandle->direct_handle = divoom_menu_direct_ctrl_create(1,
        DIVOOM_ASTRO_SET_WIFI_MAX_ROW);
    phandle->wifi_focus = 0;
    for (i = 0; i < phandle->wifi_row_count; i++)
    {
        if (phandle->wifi_row[i]
            && divoom_astrootoo_settings_wifi_row_focusable(
                phandle->wifi_row_kind[i]))
        {
            divoom_menu_direct_ctrl_update(phandle->direct_handle,
                phandle->wifi_row[i], 0, phandle->wifi_focus++);
        }
    }
}

static int divoom_astrootoo_settings_wifi_content_can_scroll_further(
    lv_obj_t *content)
{
    if (!content)
    {
        return 0;
    }
    return lv_obj_get_scroll_bottom(content) > DIVOOM_ASTRO_SET_WIFI_SCROLL_SLACK_PX;
}

static void divoom_astrootoo_settings_wifi_content_scroll_to_top(lv_obj_t *content)
{
    if (content)
    {
        lv_obj_scroll_to_y(content, 0, LV_ANIM_ON);
    }
}

static void divoom_astrootoo_settings_wifi_content_scroll_to_bottom(lv_obj_t *content)
{
    lv_coord_t max_y;

    if (!content)
    {
        return;
    }
    max_y = lv_obj_get_scroll_y(content) + lv_obj_get_scroll_bottom(content);
    lv_obj_scroll_to_y(content, max_y, LV_ANIM_ON);
}

/* 真源 DeviceSettingsWifiScreen moveWifiRootFocus + ArrowUp/ArrowDown */
static void divoom_astrootoo_settings_wifi_root_nav(
    Pdivoom_astrootoo_settings_ctrl phandle, lv_obj_t *target, int down)
{
    lv_obj_t *next;
    int pos;
    int last_focusable;

    if (!phandle || !phandle->direct_handle || !target
        || phandle->wifi_focus <= 0)
    {
        return;
    }
    pos = divoom_menu_direct_ctrl_get_pos(phandle->direct_handle, target);
    last_focusable = phandle->wifi_focus - 1;
    if (!down)
    {
        if (pos == 0 && phandle->content
            && lv_obj_get_scroll_y(phandle->content) > 0)
        {
            divoom_astrootoo_settings_wifi_content_scroll_to_top(phandle->content);
            return;
        }
        next = (lv_obj_t *)divoom_menu_direct_ctrl_get(phandle->direct_handle,
            target, DIVOOM_MENU_DIRECT_UP);
        if (next && next != target)
        {
            lv_group_focus_obj(next);
            divoom_astrootoo_ui_focus_scroll(next);
            if (divoom_menu_direct_ctrl_get_pos(phandle->direct_handle, next) == 0)
            {
                divoom_astrootoo_settings_wifi_content_scroll_to_top(phandle->content);
            }
        }
        return;
    }
    if (pos == last_focusable
        && divoom_astrootoo_settings_wifi_content_can_scroll_further(phandle->content))
    {
        divoom_astrootoo_settings_wifi_content_scroll_to_bottom(phandle->content);
        return;
    }
    next = (lv_obj_t *)divoom_menu_direct_ctrl_get(phandle->direct_handle,
        target, DIVOOM_MENU_DIRECT_DOWN);
    if (next && next != target)
    {
        lv_group_focus_obj(next);
        divoom_astrootoo_ui_focus_scroll(next);
        if (divoom_menu_direct_ctrl_get_pos(phandle->direct_handle, next) == 0)
        {
            divoom_astrootoo_settings_wifi_content_scroll_to_top(phandle->content);
        }
    }
}

static int divoom_astrootoo_settings_wifi_read_sta_info(
    wifi_sta_info_t *info, uint8 read_ip)
{
    aw_wifi_config_t config;
    wifi_sta_info_t fallback;
    uint32 ip;
    int connected;
    int ret;

    if (!info)
    {
        return 0;
    }
    memset(info, 0, sizeof(*info));
    memset(&config, 0, sizeof(config));
    ip = 0;
    connected = aw_wifi_is_connected() ? 1 : 0;

    /* AIC 链路和 IP 是连接状态的唯一依据，旧接口只补齐缺失的 SSID/MAC。 */
    if (connected)
    {
        ret = aw_wifi_get_config(AW_WIFI_IF_STA, &config);
        if (ret == 0 && config.sta.ssid[0] != '\0')
        {
            strncpy((char *)info->ssid, (const char *)config.sta.ssid,
                sizeof(info->ssid) - 1u);
        }
    }
    if (connected && read_ip)
    {
        ret = aw_wifi_get_ip(AW_WIFI_IF_STA, &ip, NULL, NULL);
        if (ret == 0 && ip != 0)
        {
            info->ip_addr[0] = (uint8)(ip & 0xff);
            info->ip_addr[1] = (uint8)((ip >> 8) & 0xff);
            info->ip_addr[2] = (uint8)((ip >> 16) & 0xff);
            info->ip_addr[3] = (uint8)((ip >> 24) & 0xff);
        }
        else
        {
            connected = 0;
        }
    }

    if (info->ssid[0] == '\0')
    {
        memset(&fallback, 0, sizeof(fallback));
        ret = wifi_sta_get_info(&fallback);
        if (ret == 0)
        {
            if (connected && fallback.ssid[0] != '\0')
            {
                strncpy((char *)info->ssid, (const char *)fallback.ssid,
                    sizeof(info->ssid) - 1u);
            }
            memcpy(info->mac_addr, fallback.mac_addr, sizeof(info->mac_addr));
        }
    }
    connected = connected && info->ssid[0] != '\0';
    return connected;
}

static void divoom_astrootoo_settings_wifi_format_mac(char *buf, int buf_len,
    const uint8 *mac)
{
    if (!buf || buf_len < 18 || !mac)
    {
        if (buf && buf_len > 0)
        {
            buf[0] = '\0';
        }
        return;
    }
    snprintf(buf, (size_t)buf_len, "%02x:%02x:%02x:%02x:%02x:%02x",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static int divoom_astrootoo_settings_wifi_mac_is_valid(const uint8 *mac)
{
    int has_nonzero;
    int has_non_ff;
    int i;

    if (!mac)
    {
        return 0;
    }
    has_nonzero = 0;
    has_non_ff = 0;
    for (i = 0; i < 6; i++)
    {
        if (mac[i] != 0)
        {
            has_nonzero = 1;
        }
        if (mac[i] != 0xff)
        {
            has_non_ff = 1;
        }
    }
    return has_nonzero && has_non_ff;
}

static int divoom_astrootoo_settings_wifi_ip_is_valid(const uint8 *ip)
{
    return ip && (ip[0] != 0 || ip[1] != 0 || ip[2] != 0 || ip[3] != 0);
}

static void divoom_astrootoo_settings_wifi_refresh_device_mac(
    Pdivoom_astrootoo_settings_ctrl phandle, const uint8 *sta_mac)
{
    uint8 mac[6];
    uint32 now;
    int ret;

    if (!phandle || phandle->wifi_device_mac_ready)
    {
        return;
    }
    now = (uint32)lv_tick_get();
    if (phandle->wifi_device_mac_last_try_ms != 0
        && (uint32)(now - phandle->wifi_device_mac_last_try_ms)
            < DIVOOM_ASTRO_WIFI_MAC_RETRY_MS)
    {
        return;
    }
    phandle->wifi_device_mac_last_try_ms = now ? now : 1;
    memset(mac, 0, sizeof(mac));

    /* MAC 与连接状态无关，状态页首次进入就从 STA 接口读取并缓存。 */
    ret = divoom_main_get_device_mac_addr_ifconfig(mac, NULL);
    if (ret == 1 && divoom_astrootoo_settings_wifi_mac_is_valid(mac))
    {
        memcpy(phandle->wifi_device_mac, mac, sizeof(mac));
        phandle->wifi_device_mac_ready = 1;
    }
    else if (divoom_astrootoo_settings_wifi_mac_is_valid(sta_mac))
    {
        memcpy(phandle->wifi_device_mac, sta_mac,
            sizeof(phandle->wifi_device_mac));
        phandle->wifi_device_mac_ready = 1;
    }
}

static void divoom_astrootoo_settings_wifi_request_status_refresh(
    Pdivoom_astrootoo_settings_ctrl phandle, uint8 network_changed)
{
    if (!phandle)
    {
        return;
    }
    if (network_changed)
    {
        /* 网络切换后只让状态页重新读取一轮，成功后继续使用缓存。 */
        memset(phandle->wifi_device_ip, 0, sizeof(phandle->wifi_device_ip));
        phandle->wifi_device_ip_ready = 0;
        memset(phandle->wifi_device_mac, 0, sizeof(phandle->wifi_device_mac));
        phandle->wifi_device_mac_ready = 0;
        phandle->wifi_device_mac_last_try_ms = 0;
    }
    phandle->wifi_status_refresh_pending = 1;
}

static void divoom_astrootoo_settings_wifi_format_ip(char *buf, int buf_len,
    const uint8 *ip)
{
    if (!buf || buf_len < 16 || !ip)
    {
        if (buf && buf_len > 0)
        {
            buf[0] = '\0';
        }
        return;
    }
    snprintf(buf, (size_t)buf_len, "%d.%d.%d.%d",
        ip[0], ip[1], ip[2], ip[3]);
}

static int divoom_astrootoo_settings_wifi_net_index(Pdivoom_astrootoo_settings_ctrl phandle,
    const char *ssid)
{
    int i;

    if (!phandle || !ssid || !ssid[0])
    {
        return -1;
    }
    for (i = 0; i < phandle->wifi_net_count; i++)
    {
        if (strcmp(phandle->wifi_ssid[i], ssid) == 0)
        {
            return i;
        }
    }
    return -1;
}

static int divoom_astrootoo_settings_wifi_row_index_from_net(
    Pdivoom_astrootoo_settings_ctrl phandle, int net_idx)
{
    int i;

    if (!phandle || net_idx < 0)
    {
        return -1;
    }
    for (i = 0; i < phandle->wifi_row_count; i++)
    {
        if (phandle->wifi_row_kind[i] == DIVOOM_ASTRO_SET_WIFI_KIND_NETWORK
            && phandle->wifi_row_net_idx[i] == net_idx)
        {
            return i;
        }
    }
    return -1;
}

static void divoom_astrootoo_settings_wifi_append_network(Pdivoom_astrootoo_settings_ctrl phandle,
    const char *ssid, const char *pswd)
{
    int idx;

    if (!phandle || !ssid || !ssid[0] || phandle->wifi_net_count >= DIVOOM_ASTRO_SET_WIFI_MAX_ROW)
    {
        return;
    }
    idx = divoom_astrootoo_settings_wifi_net_index(phandle, ssid);
    if (idx >= 0)
    {
        if (pswd && pswd[0])
        {
            strncpy(phandle->wifi_pswd[idx], pswd, sizeof(phandle->wifi_pswd[0]) - 1);
        }
        return;
    }
    strncpy(phandle->wifi_ssid[phandle->wifi_net_count], ssid, sizeof(phandle->wifi_ssid[0]) - 1);
    if (pswd)
    {
        strncpy(phandle->wifi_pswd[phandle->wifi_net_count], pswd, sizeof(phandle->wifi_pswd[0]) - 1);
    }
    else
    {
        phandle->wifi_pswd[phandle->wifi_net_count][0] = '\0';
    }
    phandle->wifi_net_count++;
}

static int divoom_astrootoo_settings_wifi_saved_index(
    Pdivoom_wifi_menu_wifi_info saved_list, int saved_num, const char *ssid)
{
    int i;

    if (!saved_list || !ssid || !ssid[0])
    {
        return -1;
    }
    for (i = 0; i < saved_num; i++)
    {
        if (strcmp((const char *)saved_list[i].ssid, ssid) == 0)
        {
            return i;
        }
    }
    return -1;
}

static void divoom_astrootoo_settings_wifi_load_saved_networks(
    Pdivoom_astrootoo_settings_ctrl phandle)
{
    char connected_ssid[AW_WIFI_SSID_LEN + 1];
    Pdivoom_wifi_menu_wifi_info saved_list;
    int saved_num;
    int pass;
    int i;

    if (!phandle)
    {
        return;
    }
    saved_num = 0;
    saved_list = divoom_wifi_menu_get_all_wifi_info(&saved_num);
    connected_ssid[0] = '\0';
    divoom_astrootoo_settings_wifi_get_connected_ssid(connected_ssid,
        (int)sizeof(connected_ssid));

    /* 状态页保存列表按当前连接优先，其余保持配置保存顺序。 */
    for (pass = 0; pass < 2; pass++)
    {
        for (i = 0; i < saved_num; i++)
        {
            int is_connected;
            int net_idx;

            is_connected = connected_ssid[0]
                && strcmp(connected_ssid, (const char *)saved_list[i].ssid) == 0;
            if ((pass == 0 && !is_connected) || (pass == 1 && is_connected))
            {
                continue;
            }
            if (phandle->wifi_net_count
                >= DIVOOM_ASTRO_SET_WIFI_MAX_ROW
                    - DIVOOM_ASTRO_SET_WIFI_ROOT_BASE_ROWS
                    - DIVOOM_ASTRO_SET_WIFI_INFO_ROWS)
            {
                break;
            }
            net_idx = phandle->wifi_net_count;
            divoom_astrootoo_settings_wifi_append_network(phandle,
                (const char *)saved_list[i].ssid, (const char *)saved_list[i].pswd);
            if (phandle->wifi_net_count <= net_idx)
            {
                continue;
            }
            phandle->wifi_saved[net_idx] = 1;
            phandle->wifi_authmode[net_idx] = saved_list[i].pswd[0]
                ? AW_WIFI_AUTH_WPA2_PSK : AW_WIFI_AUTH_OPEN;
            phandle->wifi_rssi[net_idx] = DIVOOM_ASTRO_SET_WIFI_RSSI_UNKNOWN;
        }
    }
    if (saved_list)
    {
        divoom_system_free(saved_list);
    }
}

static void divoom_astrootoo_settings_wifi_refresh_labels(Pdivoom_astrootoo_settings_ctrl phandle)
{
    wifi_sta_info_t info;
    char buf[64];
    int connected;
    int status_page;
    int i;

    if (!phandle || phandle->wifi_row_count <= 0)
    {
        return;
    }

    status_page = phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI;
    connected = divoom_astrootoo_settings_wifi_read_sta_info(&info,
        (uint8)(status_page && !phandle->wifi_device_ip_ready));
    if (status_page)
    {
        divoom_astrootoo_settings_wifi_refresh_device_mac(phandle, info.mac_addr);
        if (!phandle->wifi_device_ip_ready
            && divoom_astrootoo_settings_wifi_ip_is_valid(info.ip_addr))
        {
            memcpy(phandle->wifi_device_ip, info.ip_addr,
                sizeof(phandle->wifi_device_ip));
            phandle->wifi_device_ip_ready = 1;
        }
    }

    if (status_page)
    {
        int info_idx;

        info_idx = 0;
        for (i = 0; i < phandle->wifi_row_count; i++)
        {
            if (phandle->wifi_row_kind[i] != DIVOOM_ASTRO_SET_WIFI_KIND_INFO
                || !phandle->wifi_row_val[i])
            {
                continue;
            }
            if (info_idx == 0)
            {
                if (connected && info.ssid[0])
                {
                    lv_label_set_text(phandle->wifi_row_val[i],
                        (const char *)info.ssid);
                }
                else
                {
                    lv_label_set_text(phandle->wifi_row_val[i],
                        divoom_menu_control_get_disp_str("WifiDisconnected"));
                }
            }
            else if (info_idx == 1)
            {
                lv_label_set_text(phandle->wifi_row_val[i],
                    divoom_menu_control_get_disp_str(
                        connected ? "Connected" : "WifiDisconnected"));
            }
            else if (info_idx == 2)
            {
                if (connected && phandle->wifi_device_ip_ready)
                {
                    divoom_astrootoo_settings_wifi_format_ip(buf,
                        (int)sizeof(buf), phandle->wifi_device_ip);
                    lv_label_set_text(phandle->wifi_row_val[i], buf);
                }
                else
                {
                    lv_label_set_text(phandle->wifi_row_val[i], "—");
                }
            }
            else if (info_idx == 3)
            {
                if (phandle->wifi_device_mac_ready)
                {
                    divoom_astrootoo_settings_wifi_format_mac(buf,
                        (int)sizeof(buf), phandle->wifi_device_mac);
                    lv_label_set_text(phandle->wifi_row_val[i], buf);
                }
                else
                {
                    lv_label_set_text(phandle->wifi_row_val[i], "—");
                }
            }
            info_idx++;
        }
    }
    else
    {
        if (phandle->wifi_row_val[0])
        {
            if (connected && info.ssid[0])
            {
                lv_label_set_text(phandle->wifi_row_val[0], (const char *)info.ssid);
            }
            else
            {
                lv_label_set_text(phandle->wifi_row_val[0],
                    divoom_menu_control_get_disp_str("WifiDisconnected"));
            }
        }
        if (phandle->wifi_row_val[1])
        {
            lv_label_set_text(phandle->wifi_row_val[1],
                divoom_menu_control_get_disp_str(connected ? "Connected" : "WifiDisconnected"));
        }
        if (phandle->wifi_row_val[2])
        {
            if (connected && phandle->wifi_device_ip_ready)
            {
                divoom_astrootoo_settings_wifi_format_ip(buf, (int)sizeof(buf),
                    phandle->wifi_device_ip);
                lv_label_set_text(phandle->wifi_row_val[2], buf);
            }
            else
            {
                lv_label_set_text(phandle->wifi_row_val[2], "—");
            }
        }
        if (phandle->wifi_row_val[3])
        {
            if (phandle->wifi_device_mac_ready)
            {
                divoom_astrootoo_settings_wifi_format_mac(buf, (int)sizeof(buf),
                    phandle->wifi_device_mac);
                lv_label_set_text(phandle->wifi_row_val[3], buf);
            }
            else
            {
                lv_label_set_text(phandle->wifi_row_val[3], "—");
            }
        }
    }

    for (i = 0; i < phandle->wifi_row_count; i++)
    {
        if (phandle->wifi_row_kind[i] == DIVOOM_ASTRO_SET_WIFI_KIND_INFO
            && phandle->wifi_row_val[i])
        {
            divoom_astrootoo_ui_list_label_marquee_always(phandle->wifi_row_val[i]);
        }
    }

    for (i = 0; i < phandle->wifi_row_count; i++)
    {
        if (phandle->wifi_row_kind[i] != DIVOOM_ASTRO_SET_WIFI_KIND_NETWORK)
        {
            continue;
        }
        if (phandle->wifi_row_chk[i]
            && phandle->wifi_row_net_idx[i] >= 0
            && phandle->wifi_row_net_idx[i] < phandle->wifi_net_count)
        {
            int net_idx;

            net_idx = phandle->wifi_row_net_idx[i];
            if (connected && phandle->wifi_row_net_idx[i] >= 0
                && strcmp((const char *)info.ssid,
                    phandle->wifi_ssid[net_idx]) == 0)
            {
                divoom_menu_control_lv_img_set_src(phandle->wifi_row_chk[i],
                    DIVOOM_ASTRO_SET_WIFI_CHECK_ICON);
                lv_obj_clear_flag(phandle->wifi_row_chk[i], LV_OBJ_FLAG_HIDDEN);
            }
            else if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI
                && phandle->wifi_saved[net_idx])
            {
                divoom_menu_control_lv_img_set_src(phandle->wifi_row_chk[i],
                    DIVOOM_ASTRO_SET_WIFI_SAVED_ICON);
                lv_obj_clear_flag(phandle->wifi_row_chk[i], LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_obj_add_flag(phandle->wifi_row_chk[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
    if (status_page)
    {
        /* 只重试尚未取得的值；MAC 与已联网时的 IP 都有效后停止状态轮询。 */
        phandle->wifi_status_refresh_pending =
            (!phandle->wifi_device_mac_ready
                || (connected && !phandle->wifi_device_ip_ready)) ? 1 : 0;
    }
}

static void divoom_astrootoo_settings_wifi_timer_cb(lv_timer_t *t)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    uint32 scan_elapsed_ms;

    phandle = (Pdivoom_astrootoo_settings_ctrl)lv_timer_get_user_data(t);
    if (!phandle || !phandle->root_obj || !lv_obj_is_valid(phandle->root_obj))
    {
        return;
    }
    if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_ADD)
    {
        scan_elapsed_ms = (uint32)(lv_tick_get() - phandle->wifi_scan_begin_ms);
        /* 每 100ms 读取已完整提交的 AP，不等整轮 DONE。只有主线程追加 LVGL 行。 */
        if (g_wifi_scan_cache.status == DIVOOM_ASTRO_WIFI_SCAN_RUNNING
            && !g_wifi_scan_cancel_requested
            && g_wifi_flow.connect_state == DIVOOM_ASTRO_WIFI_CONNECT_IDLE)
        {
            divoom_astrootoo_settings_wifi_scan_refresh(phandle);
        }
        if (!phandle->wifi_scan_finished
            && !divoom_wifi_is_stop_connect_pending()
            && g_wifi_scan_cache.status == DIVOOM_ASTRO_WIFI_SCAN_READY)
        {
            divoom_astrootoo_settings_wifi_scan_apply(phandle);
            phandle->wifi_scan_finished = 1;
        }
        else if (!phandle->wifi_scan_finished
            && (g_wifi_scan_cache.status == DIVOOM_ASTRO_WIFI_SCAN_FAILED
                || scan_elapsed_ms >= DIVOOM_ASTRO_WIFI_SCAN_TIMEOUT_MS))
        {
            if (phandle->wifi_row_count > 0)
            {
                /* 终态失败或超时不覆盖已经显示的部分结果。 */
                if (g_wifi_scan_cache.status != DIVOOM_ASTRO_WIFI_SCAN_RUNNING)
                {
                    phandle->wifi_scan_finished = 1;
                    if (g_wifi_flow.connect_state == DIVOOM_ASTRO_WIFI_CONNECT_IDLE)
                        divoom_astrootoo_settings_wifi_connect_notice_hide(phandle);
                }
            }
            else if (!g_wifi_scan_timeout_reported)
            {
                g_wifi_scan_timeout_reported = 1;
                divoom_print("[WIFI_UI_DIAG] scan unavailable seq=%u elapsed_ms=%u "
                    "status=%d count=%d stop_pending=%d\n", g_wifi_diag_scan_seq,
                    scan_elapsed_ms, g_wifi_scan_cache.status, g_wifi_scan_cache.count,
                    divoom_wifi_is_stop_connect_pending());
                divoom_astrootoo_settings_wifi_connect_notice_show(phandle,
                    DIVOOM_ASTRO_WIFI_NOTICE_SCAN_ERROR);
            }
        }
        else if (!phandle->wifi_scan_applied
            && divoom_wifi_is_stop_connect_pending() == 2
            && (uint32)(lv_tick_get() - g_wifi_scan_cache.started_ms)
                >= DIVOOM_ASTRO_WIFI_SCAN_RETRY_MS)
        {
            g_wifi_scan_cache.started_ms = lv_tick_get();
            divoom_print("[WIFI_UI_DIAG] retry failed scan STA prepare\n");
            divoom_wifi_stop_connect_wifi();
        }
        else if (!phandle->wifi_scan_applied
            && !divoom_wifi_is_stop_connect_pending()
            && g_wifi_scan_cache.status == DIVOOM_ASTRO_WIFI_SCAN_IDLE)
        {
            divoom_astrootoo_settings_wifi_scan_start();
        }
        else if (!phandle->wifi_scan_applied
            && scan_elapsed_ms >= DIVOOM_ASTRO_WIFI_SCAN_NOTICE_DELAY_MS
            && phandle->wifi_connect_notice
            && lv_obj_has_flag(phandle->wifi_connect_notice, LV_OBJ_FLAG_HIDDEN))
        {
            divoom_astrootoo_settings_wifi_connect_notice_show(phandle,
                DIVOOM_ASTRO_WIFI_NOTICE_SCANNING);
        }
        divoom_astrootoo_settings_wifi_connect_process(phandle);
        if (g_wifi_app_pairing_prompt_pending
            && g_wifi_flow.connect_state == DIVOOM_ASTRO_WIFI_CONNECT_IDLE)
        {
            lv_group_t *group;

            /* 配网成功但设备未绑定时直接进入连接码页，不再显示二次确认框。 */
            divoom_main_set_wifi_add_network_route_pending(0);
            g_wifi_app_pairing_prompt_pending = 0;
            g_wifi_flow.awaiting_route = 0;
            memset(&g_wifi_flow, 0, sizeof(g_wifi_flow));
            group = lv_group_get_default();
            if (group)
            {
                lv_group_remove_all_objs(group);
            }
            divoom_menu_control_load_next_pop_cur(
                DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_CONNECTION_CODE,
                NULL, 0, 0);
        }
    }
    else
    {
        divoom_astrootoo_settings_wifi_connect_process(phandle);
        if (phandle->wifi_status_refresh_pending)
        {
            divoom_astrootoo_settings_wifi_refresh_labels(phandle);
        }
    }
}

static void divoom_astrootoo_settings_wifi_bind_info_row(lv_obj_t *card, lv_obj_t *lbl,
    lv_obj_t *val, int status_page)
{
    lv_coord_t key_w;
    lv_coord_t val_w;
    lv_coord_t inset_r;

    if (!card || !lbl || !val)
    {
        return;
    }
    inset_r = status_page ? DIVOOM_ASTRO_SET_WIFI_STATUS_INSET_R
        : DIVOOM_ASTRO_SET_WIFI_LABEL_INSET_R;
    lv_obj_update_layout(lbl);
    key_w = lv_obj_get_width(lbl);
    val_w = DIVOOM_ASTRO_SET_WIFI_ROW_W - DIVOOM_ASTRO_SET_WIFI_LABEL_INSET_L
        - inset_r - key_w - DIVOOM_ASTRO_LIST_LABEL_GAP;
    if (val_w > DIVOOM_ASTRO_SET_WIFI_INFO_VALUE_W)
    {
        val_w = DIVOOM_ASTRO_SET_WIFI_INFO_VALUE_W;
    }
    if (val_w < 80)
    {
        val_w = 80;
    }
    lv_obj_set_size(val, val_w, DIVOOM_ASTRO_SET_WIFI_INFO_VALUE_H);
    divoom_astrootoo_ui_list_label_prepare(val, val_w);
    lv_obj_set_style_text_align(val, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(val, LV_ALIGN_RIGHT_MID, status_page ? -12 : -8, status_page ? 3 : 0);
    divoom_astrootoo_ui_list_label_marquee_always(val);
}

static lv_obj_t *divoom_astrootoo_settings_wifi_add_row(Pdivoom_astrootoo_settings_ctrl phandle,
    int idx, lv_coord_t y, uint8 kind, const char *left_key, const char *right_text,
    int focusable)
{
    lv_obj_t *card;
    lv_obj_t *lbl;
    lv_obj_t *val;
    int status_page;

    status_page = phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI;

    card = lv_btn_create(phandle->content);
    lv_obj_add_style(card, &phandle->ui.card_item_style, 0);
    lv_obj_add_style(card, &phandle->ui.card_item_focus_style, LV_STATE_FOCUSED);
    lv_obj_add_style(card, &phandle->ui.card_item_focus_style, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(card, lv_color_hex(0xB8B0A8), LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(card, lv_color_hex(0xB8B0A8), LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_size(card, DIVOOM_ASTRO_SET_WIFI_ROW_W, DIVOOM_ASTRO_SET_WIFI_ROW_H);
    lv_obj_set_pos(card, DIVOOM_ASTRO_SET_WIFI_ROW_X, y);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lbl = lv_label_create(card);
    lv_label_set_text(lbl, divoom_menu_control_get_disp_str(left_key));
    lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(
        status_page ? 24 : 28, phandle->menu_id), 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 12,
        status_page ? 3 : 0);

    if (kind == DIVOOM_ASTRO_SET_WIFI_KIND_ADD
        || kind == DIVOOM_ASTRO_SET_WIFI_KIND_CONNECTION_CODE)
    {
        lv_obj_t *chev;
        lv_coord_t inset_r;

        inset_r = status_page ? DIVOOM_ASTRO_SET_WIFI_STATUS_INSET_R
            : DIVOOM_ASTRO_SET_WIFI_LABEL_INSET_R;
        divoom_astrootoo_ui_list_label_prepare(lbl,
            divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_SET_WIFI_ROW_W,
                DIVOOM_ASTRO_SET_WIFI_LABEL_INSET_L, inset_r,
                DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W));
        divoom_astrootoo_ui_list_row_bind_marquee(card, lbl);

        chev = lv_img_create(card);
        divoom_menu_control_lv_img_set_src(chev, DIVOOM_ASTROTOO_SETTINGS_CHEVRON);
        divoom_astrootoo_ui_apply_sublevel_chevron_size(chev);
        lv_obj_align(chev, LV_ALIGN_RIGHT_MID, status_page ? -14 : -8, 0);
    }
    else if (right_text && kind != DIVOOM_ASTRO_SET_WIFI_KIND_NETWORK)
    {
        val = lv_label_create(card);
        lv_label_set_text(val, right_text);
        lv_obj_set_style_text_color(val, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
        lv_obj_set_style_text_font(val, divoom_menu_control_get_font_info(
            status_page && kind == DIVOOM_ASTRO_SET_WIFI_KIND_INFO ? 20 : 24,
            phandle->menu_id), 0);
        if (kind == DIVOOM_ASTRO_SET_WIFI_KIND_INFO)
        {
            divoom_astrootoo_settings_wifi_bind_info_row(card, lbl, val, status_page);
        }
        else
        {
            lv_obj_set_size(val, 200, DIVOOM_ASTRO_SET_WIFI_ROW_H);
            lv_label_set_long_mode(val, LV_LABEL_LONG_DOT);
            lv_obj_set_style_text_align(val, LV_TEXT_ALIGN_RIGHT, 0);
            lv_obj_align(val, LV_ALIGN_RIGHT_MID, status_page ? -12 : -8,
                status_page ? 3 : 0);
        }
        phandle->wifi_row_val[idx] = val;
    }

    if (kind == DIVOOM_ASTRO_SET_WIFI_KIND_NETWORK)
    {
        lv_obj_t *chk;

        chk = lv_img_create(card);
        divoom_menu_control_lv_img_set_src(chk, DIVOOM_ASTRO_SET_WIFI_CHECK_ICON);
        lv_obj_align(chk, LV_ALIGN_RIGHT_MID, -8, 0);
        lv_obj_add_flag(chk, LV_OBJ_FLAG_HIDDEN);
        phandle->wifi_row_chk[idx] = chk;
        lv_label_set_text(lbl, right_text ? right_text : "");
        divoom_astrootoo_ui_list_label_prepare(lbl,
            divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_SET_WIFI_ROW_W,
                DIVOOM_ASTRO_SET_WIFI_LABEL_INSET_L, DIVOOM_ASTRO_SET_WIFI_LABEL_INSET_R,
                DIVOOM_ASTRO_LIST_CHK_TRAIL_CARD));
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 12, status_page ? 3 : 0);
        divoom_astrootoo_ui_list_row_bind_marquee(card, lbl);
    }

    if (focusable)
    {
        divoom_menu_direct_ctrl_update(phandle->direct_handle, card, 0, phandle->wifi_focus);
        phandle->wifi_focus++;
    }

    phandle->wifi_row[idx] = card;
    phandle->wifi_row_kind[idx] = kind;
    phandle->wifi_row_net_idx[idx] = -1;
    return card;
}

static lv_obj_t *divoom_astrootoo_settings_wifi_add_section_title(
    Pdivoom_astrootoo_settings_ctrl phandle, lv_coord_t y, const char *key)
{
    lv_obj_t *title;

    title = lv_label_create(phandle->content);
    lv_label_set_text(title, divoom_menu_control_get_disp_str(key));
    lv_obj_set_size(title, DIVOOM_ASTRO_SET_WIFI_ROW_W,
        DIVOOM_ASTRO_SET_WIFI_SECTION_H);
    lv_obj_set_pos(title, DIVOOM_ASTRO_SET_WIFI_ROW_X, y);
    lv_obj_set_style_pad_left(title, 14, 0);
    lv_obj_set_style_pad_top(title, 6, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(title,
        divoom_menu_control_get_font_info(20, phandle->menu_id), 0);
    lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
    return title;
}

static int divoom_astrootoo_settings_wifi_get_connected_ssid(char *ssid, int ssid_len)
{
    aw_wifi_config_t config;
    int ret;

    if (!ssid || ssid_len <= 0)
    {
        return 0;
    }
    ssid[0] = '\0';
    ret = aw_wifi_is_connected() ? 1 : 0;
    if (!ret)
    {
        return 0;
    }
    memset(&config, 0, sizeof(config));
    ret = aw_wifi_get_config(AW_WIFI_IF_STA, &config);
    if (ret != 0 || config.sta.ssid[0] == '\0')
    {
        return 0;
    }
    strncpy(ssid, (const char *)config.sta.ssid, (size_t)ssid_len - 1u);
    ssid[ssid_len - 1] = '\0';
    return 1;
}

static lv_obj_t *divoom_astrootoo_settings_wifi_overlay_create(
    lv_obj_t *parent, lv_coord_t x, lv_coord_t width, lv_coord_t height)
{
    lv_obj_t *overlay;

    overlay = lv_obj_create(parent);
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, width, height);
    lv_obj_set_pos(overlay, x, 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(overlay, 0, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    /* 真机不绘制遮罩背景，避免 RGB565 将任意透明度错误渲染为纯黑。 */
    return overlay;
}

static void divoom_astrootoo_settings_wifi_disconnect_modal_style(
    Pdivoom_astrootoo_settings_ctrl phandle)
{
    lv_obj_t *cancel_lbl;
    lv_obj_t *ok_lbl;
    int ok_focus;
    int app_pairing;

    if (!phandle || !phandle->confirm_cancel_btn || !phandle->confirm_ok_btn)
    {
        return;
    }
    ok_focus = phandle->confirm_focus == 1;
    app_pairing = phandle->confirm_action == DIVOOM_ASTRO_WIFI_CONFIRM_APP_PAIRING;

    lv_obj_set_style_bg_color(phandle->confirm_cancel_btn,
        lv_color_hex(ok_focus ? DIVOOM_ASTRO_SURFACE_IDLE : DIVOOM_ASTRO_SURFACE_FOCUS), 0);
    if (app_pairing)
    {
        lv_obj_set_style_border_width(phandle->confirm_cancel_btn, 2, 0);
        lv_obj_set_style_border_color(phandle->confirm_cancel_btn,
            lv_color_hex(ok_focus ? DIVOOM_ASTRO_STROKE : DIVOOM_ASTRO_SURFACE_FOCUS), 0);
    }
    else
    {
        lv_obj_set_style_border_width(phandle->confirm_cancel_btn, 0, 0);
    }

    if (app_pairing)
    {
        lv_obj_set_style_bg_opa(phandle->confirm_ok_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(phandle->confirm_ok_btn,
            lv_color_hex(ok_focus ? DIVOOM_ASTRO_SURFACE_FOCUS : DIVOOM_ASTRO_SURFACE_IDLE), 0);
        lv_obj_set_style_border_width(phandle->confirm_ok_btn, 2, 0);
        lv_obj_set_style_border_color(phandle->confirm_ok_btn,
            lv_color_hex(ok_focus ? DIVOOM_ASTRO_SURFACE_FOCUS : DIVOOM_ASTRO_STROKE), 0);
    }
    else if (ok_focus)
    {
        lv_obj_set_style_bg_opa(phandle->confirm_ok_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(phandle->confirm_ok_btn, lv_color_hex(DIVOOM_ASTRO_DANGER), 0);
        lv_obj_set_style_border_width(phandle->confirm_ok_btn, 0, 0);
    }
    else
    {
        lv_obj_set_style_bg_opa(phandle->confirm_ok_btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(phandle->confirm_ok_btn, 2, 0);
        lv_obj_set_style_border_color(phandle->confirm_ok_btn, lv_color_hex(DIVOOM_ASTRO_DANGER), 0);
    }

    cancel_lbl = lv_obj_get_child(phandle->confirm_cancel_btn, 0);
    ok_lbl = lv_obj_get_child(phandle->confirm_ok_btn, 0);
    if (cancel_lbl)
    {
        lv_obj_set_style_text_color(cancel_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    }
    if (ok_lbl)
    {
        lv_obj_set_style_text_color(ok_lbl,
            lv_color_hex(app_pairing ? DIVOOM_ASTRO_INK
                : (ok_focus ? DIVOOM_ASTRO_BG : DIVOOM_ASTRO_DANGER)), 0);
    }
    lv_group_focus_obj(ok_focus ? phandle->confirm_ok_btn : phandle->confirm_cancel_btn);
    divoom_astrootoo_ui_confirm_modal_sync_btn_labels(phandle->confirm_cancel_btn,
        phandle->confirm_ok_btn, ok_focus);
}

static void divoom_astrootoo_settings_wifi_disconnect_modal_close(
    Pdivoom_astrootoo_settings_ctrl phandle)
{
    lv_obj_t *restore;
    int net_idx;
    int app_pairing;

    if (!phandle || !phandle->confirm_overlay)
    {
        return;
    }
    app_pairing = phandle->confirm_action == DIVOOM_ASTRO_WIFI_CONFIRM_APP_PAIRING;
    restore = phandle->confirm_focus_restore;
    if (!restore && !app_pairing)
    {
        net_idx = divoom_astrootoo_settings_wifi_row_index_from_net(phandle,
            phandle->confirm_action);
        restore = (net_idx >= 0 && net_idx < phandle->wifi_row_count)
            ? phandle->wifi_row[net_idx] : NULL;
    }
    if (restore)
    {
        lv_obj_clear_state(restore, LV_STATE_FOCUSED);
    }
    lv_obj_del(phandle->confirm_overlay);
    phandle->confirm_overlay = NULL;
    phandle->confirm_cancel_btn = NULL;
    phandle->confirm_ok_btn = NULL;
    phandle->confirm_focus_restore = NULL;
    if (app_pairing)
    {
        g_wifi_app_pairing_prompt_open = 0;
    }
    else
    {
        divoom_astrootoo_settings_wifi_group_bind(phandle, restore);
    }
}

static void divoom_astrootoo_settings_wifi_add_back(void)
{
    int stack_depth;

    g_wifi_scan_cancel_requested = 1;
    stack_depth = divoom_menu_control_get_stack_depth();

    if (g_wifi_flow.awaiting_route)
    {
        divoom_main_set_wifi_add_network_route_pending(0);
        g_wifi_app_pairing_prompt_pending = 0;
        memset(&g_wifi_flow, 0, sizeof(g_wifi_flow));
        if (stack_depth == 0)
        {
            divoom_main_return_startup_dial();
        }
        else
        {
            divoom_menu_control_return_pre();
        }
        divoom_wifi_connect_wifi_in_nvs();
        return;
    }
    if (g_wifi_flow.had_failure)
    {
        divoom_main_set_wifi_add_network_route_pending(0);
        memset(&g_wifi_flow, 0, sizeof(g_wifi_flow));
        if (stack_depth == 0)
        {
            divoom_main_return_startup_dial();
        }
        else
        {
            divoom_menu_control_return_pre();
        }
        divoom_wifi_connect_wifi_in_nvs();
        return;
    }
    /* 扫描页无论是否发生连接都曾暂停后台重连；正常返回也必须解除暂停。 */
    if (stack_depth == 0)
    {
        divoom_main_return_startup_dial();
        divoom_wifi_connect_wifi_in_nvs();
        return;
    }
    divoom_menu_control_return_pre();
    divoom_wifi_connect_wifi_in_nvs();
}

static void divoom_astrootoo_settings_wifi_app_pairing_modal_finish(
    Pdivoom_astrootoo_settings_ctrl phandle, uint8 confirm)
{
    lv_group_t *group;

    divoom_main_set_wifi_add_network_route_pending(0);
    g_wifi_app_pairing_prompt_pending = 0;
    memset(&g_wifi_flow, 0, sizeof(g_wifi_flow));
    if (confirm)
    {
        /* 菜单切换通过队列异步执行。保留弹窗直到新页面完成创建，避免中间露出列表。 */
        group = lv_group_get_default();
        if (group)
        {
            lv_group_remove_all_objs(group);
        }
        divoom_menu_control_load_next_pop_cur(
            DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_CONNECTION_CODE,
            NULL, 0, 0);
        return;
    }
    divoom_astrootoo_settings_wifi_disconnect_modal_close(phandle);
    divoom_menu_control_return_pre();
}

static void divoom_astrootoo_settings_wifi_disconnect_modal_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    uint8 key_code;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    if (!divoom_astrootoo_ui_key_is_menu_press(e, key_code))
    {
        return;
    }
    phandle = (Pdivoom_astrootoo_settings_ctrl)lv_event_get_user_data(e);
    if (!phandle)
    {
        return;
    }
    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        if (phandle->confirm_action == DIVOOM_ASTRO_WIFI_CONFIRM_APP_PAIRING)
        {
            divoom_astrootoo_settings_wifi_app_pairing_modal_finish(phandle, 0);
            return;
        }
        divoom_astrootoo_settings_wifi_disconnect_modal_close(phandle);
        return;
    }
    if (divoom_astrootoo_ui_key_is_left(key_code)
        || divoom_astrootoo_ui_key_is_right(key_code)
        || divoom_astrootoo_ui_key_is_prev(key_code)
        || divoom_astrootoo_ui_key_is_next(key_code))
    {
        phandle->confirm_focus = phandle->confirm_focus == 0 ? 1 : 0;
        divoom_astrootoo_settings_wifi_disconnect_modal_style(phandle);
        return;
    }
    if (!divoom_astrootoo_ui_key_is_enter(key_code))
    {
        return;
    }
    if (phandle->confirm_focus == 0)
    {
        if (phandle->confirm_action == DIVOOM_ASTRO_WIFI_CONFIRM_APP_PAIRING)
        {
            divoom_astrootoo_settings_wifi_app_pairing_modal_finish(phandle, 0);
            return;
        }
        divoom_astrootoo_settings_wifi_disconnect_modal_close(phandle);
        return;
    }

    if (phandle->confirm_action == DIVOOM_ASTRO_WIFI_CONFIRM_APP_PAIRING)
    {
        divoom_astrootoo_settings_wifi_app_pairing_modal_finish(phandle, 1);
        return;
    }

    /* 断开但不删除保存密码，列表随后把该行降为“已保存”状态。 */
    divoom_wifi_set_wifi_time_out_flag(1);
    aw_wifi_disconnect();
    divoom_wifi_set_wifi_time_out_flag(2);
    divoom_astrootoo_settings_wifi_disconnect_modal_close(phandle);
    divoom_astrootoo_settings_wifi_request_status_refresh(phandle, 1);
    divoom_astrootoo_settings_wifi_refresh_labels(phandle);
}

static lv_obj_t *divoom_astrootoo_settings_wifi_confirm_make_btn(
    Pdivoom_astrootoo_settings_ctrl phandle, lv_obj_t *parent, const char *label)
{
    return divoom_astrootoo_ui_confirm_modal_make_btn(parent, label, phandle->menu_id, 0,
        divoom_astrootoo_settings_wifi_disconnect_modal_key_cb, phandle, 0);
}

/* FlowtooConfirmModal：透明 overlay + shell/frame 双层底色（真源 modalShellBg / modalBg） */
static void divoom_astrootoo_settings_wifi_confirm_modal_show(
    Pdivoom_astrootoo_settings_ctrl phandle, int action_id,
    const char *title, const char *body,
    const char *cancel_label, const char *confirm_label)
{
    lv_obj_t *frame;
    lv_obj_t *body_lbl;
    lv_obj_t *actions;
    lv_group_t *group;

    if (!phandle || !body || !cancel_label || !confirm_label)
    {
        return;
    }

    phandle->confirm_action = action_id;
    phandle->confirm_focus = 0;

    phandle->confirm_overlay = divoom_astrootoo_ui_confirm_modal_create(
        phandle->root_obj, &frame);
    if (!phandle->confirm_overlay || !frame)
    {
        return;
    }

    if (title && title[0] != '\0')
    {
        lv_obj_t *title_lbl;

        title_lbl = lv_label_create(frame);
        lv_label_set_long_mode(title_lbl, LV_LABEL_LONG_WRAP);
        lv_label_set_text(title_lbl, title);
        lv_obj_set_width(title_lbl, LV_PCT(100));
        lv_obj_set_style_text_align(title_lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(title_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
        lv_obj_set_style_text_font(title_lbl,
            divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
        lv_obj_set_style_pad_bottom(title_lbl, 10, 0);
        lv_obj_set_style_border_side(title_lbl, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_width(title_lbl, 2, 0);
        lv_obj_set_style_border_color(title_lbl, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
    }

    body_lbl = lv_label_create(frame);
    lv_label_set_long_mode(body_lbl, LV_LABEL_LONG_WRAP);
    lv_label_set_text(body_lbl, body);
    lv_obj_set_width(body_lbl, LV_PCT(100));
    lv_obj_set_style_text_align(body_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(body_lbl, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(body_lbl, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);

    actions = lv_obj_create(frame);
    lv_obj_remove_style_all(actions);
    lv_obj_set_width(actions, LV_PCT(100));
    lv_obj_set_height(actions, LV_SIZE_CONTENT);
    lv_obj_clear_flag(actions, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(actions, 8, 0);

    phandle->confirm_cancel_btn = divoom_astrootoo_settings_wifi_confirm_make_btn(
        phandle, actions, cancel_label);
    phandle->confirm_ok_btn = divoom_astrootoo_settings_wifi_confirm_make_btn(
        phandle, actions, confirm_label);

    group = lv_group_get_default();
    if (group)
    {
        lv_group_remove_all_objs(group);
        lv_group_add_obj(group, phandle->confirm_cancel_btn);
        lv_group_add_obj(group, phandle->confirm_ok_btn);
    }
    divoom_astrootoo_ui_confirm_modal_layout(frame);
    divoom_astrootoo_settings_wifi_disconnect_modal_style(phandle);
}

static void divoom_astrootoo_settings_wifi_disconnect_modal_show(
    Pdivoom_astrootoo_settings_ctrl phandle, int net_idx)
{
    char text[192];

    if (!phandle || phandle->confirm_overlay || net_idx < 0
        || net_idx >= phandle->wifi_net_count)
    {
        return;
    }
    snprintf(text, sizeof(text),
        divoom_menu_control_get_disp_str("WifiDisconnectConfirm"),
        phandle->wifi_ssid[net_idx]);
    phandle->confirm_focus_restore = NULL;
    divoom_astrootoo_settings_wifi_confirm_modal_show(phandle, net_idx, NULL, text,
        divoom_menu_control_get_disp_str("Cancel"),
        divoom_menu_control_get_disp_str("Confirm"));
    /* React 真源弹窗打开时仍保留父列表选中态，LVGL 需显式补回。 */
    {
        int row_idx;

        row_idx = divoom_astrootoo_settings_wifi_row_index_from_net(phandle, net_idx);
        if (row_idx >= 0)
        {
            phandle->confirm_focus_restore = phandle->wifi_row[row_idx];
            lv_obj_add_state(phandle->wifi_row[row_idx], LV_STATE_FOCUSED);
        }
    }
}

static void divoom_astrootoo_settings_wifi_app_pairing_modal_show(
    Pdivoom_astrootoo_settings_ctrl phandle)
{
    lv_group_t *group;

    if (!phandle || phandle->confirm_overlay
        || phandle->page != DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_ADD)
    {
        return;
    }
    group = lv_group_get_default();
    phandle->confirm_focus_restore = group ? lv_group_get_focused(group) : NULL;
    divoom_astrootoo_settings_wifi_confirm_modal_show(phandle,
        DIVOOM_ASTRO_WIFI_CONFIRM_APP_PAIRING,
        divoom_menu_control_get_disp_str("WifiAppPairingPromptTitle"),
        divoom_menu_control_get_disp_str("WifiAppPairingPromptBody"),
        divoom_menu_control_get_disp_str("WifiAppPairingPromptCancel"),
        divoom_menu_control_get_disp_str("WifiAppPairingPromptConfirm"));
    if (phandle->confirm_focus_restore
        && lv_obj_is_valid(phandle->confirm_focus_restore))
    {
        lv_obj_add_state(phandle->confirm_focus_restore, LV_STATE_FOCUSED);
    }
    g_wifi_app_pairing_prompt_pending = 0;
    g_wifi_app_pairing_prompt_open = 1;
}

static void divoom_astrootoo_settings_wifi_drop_stale_add_route(void)
{
    g_wifi_flow.awaiting_route = 0;
    divoom_main_set_wifi_add_network_route_pending(0);
    if (g_wifi_flow.connect_state == DIVOOM_ASTRO_WIFI_CONNECT_SUCCESS)
    {
        g_wifi_flow.connect_state = DIVOOM_ASTRO_WIFI_CONNECT_IDLE;
    }
}

static void divoom_astrootoo_settings_wifi_route_after_manual_success(void)
{
    if (!g_wifi_flow.awaiting_route)
    {
        return;
    }
    /* 分流只属于「添加网络」页；设置 Wi-Fi 状态页点进来不得回表盘。 */
    if (divoom_menu_control_get_active_menu_id()
        != DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_WIFI_ADD)
    {
        divoom_astrootoo_settings_wifi_drop_stale_add_route();
        return;
    }
    if (divoom_main_get_host_id() == 0)
    {
        if (!divoom_astrootoo_settings_wifi_request_app_pairing_prompt())
        {
            return;
        }
    }
    else
    {
        divoom_main_return_startup_dial();
    }
    /* 本地已完成分流，禁止稍后的 MQTT 回调重复弹框或重复切页。 */
    g_wifi_flow.awaiting_route = 0;
    divoom_main_set_wifi_add_network_route_pending(0);
}

static void divoom_astrootoo_settings_wifi_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    lv_obj_t *target;
    lv_obj_t *next;
    uint8_t key_code;
    int pos;
    int idx;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    phandle = (Pdivoom_astrootoo_settings_ctrl)lv_event_get_user_data(e);
    if (!phandle)
    {
        return;
    }
    target = lv_event_get_target(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    /* 有扫描转圈时，ESC/ENTER 只结束扫描；吞掉本次按键，不退出或选中网络。 */
    if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_ADD
        && !phandle->wifi_scan_finished && phandle->wifi_loading_gif_handle
        && (g_wifi_flow.connect_state == DIVOOM_ASTRO_WIFI_CONNECT_IDLE
            || g_wifi_scan_cache.status == DIVOOM_ASTRO_WIFI_SCAN_RUNNING)
        && (divoom_astrootoo_ui_key_is_esc(key_code)
            || divoom_astrootoo_ui_key_is_enter(key_code)))
    {
        divoom_astrootoo_settings_wifi_scan_cancel_ui(phandle);
        return;
    }

    if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_ADD
        || phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI)
    {
        if (g_wifi_flow.connect_state == DIVOOM_ASTRO_WIFI_CONNECT_RUNNING)
        {
            return;
        }
        if (g_wifi_flow.connect_state == DIVOOM_ASTRO_WIFI_CONNECT_SUCCESS)
        {
            g_wifi_flow.connect_state = DIVOOM_ASTRO_WIFI_CONNECT_IDLE;
            divoom_astrootoo_settings_wifi_connect_notice_hide(phandle);
            if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_ADD)
            {
                /* 添加页：用户提前收起成功卡时完成本次配网分流。 */
                divoom_astrootoo_settings_wifi_route_after_manual_success();
            }
            else
            {
                divoom_astrootoo_settings_wifi_drop_stale_add_route();
            }
            return;
        }
        if (g_wifi_flow.connect_state == DIVOOM_ASTRO_WIFI_CONNECT_FAILED)
        {
            int net_idx;

            divoom_main_set_wifi_add_network_route_pending(0);
            net_idx = g_wifi_flow.focus_net_idx;
            if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_ADD
                && net_idx >= 0 && net_idx < phandle->wifi_net_count)
            {
                phandle->wifi_saved[net_idx] = 0;
                phandle->wifi_pswd[net_idx][0] = '\0';
            }
            g_wifi_flow.connect_state = DIVOOM_ASTRO_WIFI_CONNECT_IDLE;
            divoom_astrootoo_settings_wifi_connect_notice_hide(phandle);
            divoom_astrootoo_settings_wifi_refresh_labels(phandle);
            net_idx = divoom_astrootoo_settings_wifi_row_index_from_net(phandle, net_idx);
            divoom_astrootoo_settings_wifi_group_bind(phandle,
                net_idx >= 0 ? phandle->wifi_row[net_idx] : NULL);
            return;
        }
    }

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_ADD)
        {
            divoom_astrootoo_settings_wifi_add_back();
            return;
        }
        divoom_menu_control_return_pre();
        return;
    }
    if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_ADD
        && !phandle->wifi_scan_applied)
    {
        return;
    }
    if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code))
    {
        if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI)
        {
            divoom_astrootoo_settings_wifi_root_nav(phandle, target,
                divoom_astrootoo_ui_key_is_down(key_code));
            return;
        }
        next = (lv_obj_t *)divoom_menu_direct_ctrl_get(phandle->direct_handle,
            target, divoom_astrootoo_ui_key_is_up(key_code)
                ? DIVOOM_MENU_DIRECT_UP : DIVOOM_MENU_DIRECT_DOWN);
        if (next && next != target)
        {
            lv_group_focus_obj(next);
            divoom_astrootoo_ui_focus_scroll(next);
        }
        return;
    }
    if (!divoom_astrootoo_ui_key_is_enter(key_code))
    {
        return;
    }

    pos = divoom_menu_direct_ctrl_get_pos(phandle->direct_handle, target);
    if (pos < 0)
    {
        return;
    }
    for (idx = 0; idx < phandle->wifi_row_count; idx++)
    {
        if (phandle->wifi_row[idx] != target)
        {
            continue;
        }
        if (phandle->wifi_row_kind[idx] == DIVOOM_ASTRO_SET_WIFI_KIND_ADD)
        {
            /* 设置内新增独立扫描流程；OOBE/二维码配网页保持原样。 */
            memset(&g_wifi_flow, 0, sizeof(g_wifi_flow));
            /* ENTER 时立即停止后台重连，不等待下一级页面完成构造。 */
            divoom_wifi_stop_connect_wifi();
            if (g_wifi_scan_cache.status != DIVOOM_ASTRO_WIFI_SCAN_RUNNING)
            {
                g_wifi_scan_cache.status = DIVOOM_ASTRO_WIFI_SCAN_IDLE;
            }
            divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_WIFI_ADD,
                NULL, 0, 0);
        }
        else if (phandle->wifi_row_kind[idx]
            == DIVOOM_ASTRO_SET_WIFI_KIND_CONNECTION_CODE)
        {
            divoom_menu_control_load_next(
                DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_CONNECTION_CODE,
                NULL, 0, 0);
        }
        else if (phandle->wifi_row_kind[idx] == DIVOOM_ASTRO_SET_WIFI_KIND_NETWORK
            && phandle->wifi_row_net_idx[idx] >= 0
            && phandle->wifi_row_net_idx[idx] < phandle->wifi_net_count)
        {
            int net_idx;

            net_idx = phandle->wifi_row_net_idx[idx];
            if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_ADD)
            {
                /* 选中部分结果后停止后续更新/重试，本轮由 worker 安全收尾。 */
                g_wifi_scan_cancel_requested = 1;
                divoom_astrootoo_settings_wifi_connect_notice_hide(phandle);
                /* 复现旧 AP 消失后的切网失败时，记录用户实际选中的扫描项。 */
                divoom_print("[WIFI_CFG_DIAG] select scan ssid=%s auth=%d rssi=%d saved=%d\n",
                    phandle->wifi_ssid[net_idx], phandle->wifi_authmode[net_idx],
                    phandle->wifi_rssi[net_idx], phandle->wifi_saved[net_idx]);
                strncpy(g_wifi_flow.ssid, phandle->wifi_ssid[net_idx], sizeof(g_wifi_flow.ssid) - 1u);
                g_wifi_flow.ssid[sizeof(g_wifi_flow.ssid) - 1u] = '\0';
                g_wifi_flow.authmode = phandle->wifi_authmode[net_idx];
                g_wifi_flow.password[0] = '\0';
                g_wifi_flow.connect_state = DIVOOM_ASTRO_WIFI_CONNECT_IDLE;
                g_wifi_flow.focus_net_idx = (int8)net_idx;
                /* 添加页所有扫描结果都按新 SSID+密码处理，不复用保存密码。 */
                g_wifi_flow.first_connect = 1;
                divoom_menu_control_load_next(
                    DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_WIFI_PASSWORD, NULL, 0, 0);
            }
            else
            {
                char connected_ssid[AW_WIFI_SSID_LEN + 1];

                connected_ssid[0] = '\0';
                divoom_astrootoo_settings_wifi_get_connected_ssid(connected_ssid,
                    (int)sizeof(connected_ssid));
                if (connected_ssid[0]
                    && strcmp(connected_ssid, phandle->wifi_ssid[net_idx]) == 0)
                {
                    /* 已连接网络的确认键不执行操作，也不弹出断开对话框。 */
                    return;
                }
                else
                {
                    divoom_astrootoo_settings_wifi_connect_begin(phandle, net_idx,
                        phandle->wifi_pswd[net_idx], 0);
                }
            }
        }
        break;
    }
    (void)pos;
}

static int divoom_astrootoo_settings_wifi_scan_cache_index(
    const divoom_astrootoo_wifi_scan_cache_t *cache, const char *ssid)
{
    int i;

    for (i = 0; i < cache->count; i++)
    {
        if (strcmp(cache->ssid[i], ssid) == 0)
        {
            return i;
        }
    }
    return -1;
}

static void *divoom_astrootoo_settings_wifi_scan_task(void *arg)
{
    divoom_astrootoo_wifi_scan_cache_t next_cache;
    aw_wifi_ap_record_t *records;
    uint16 scan_count;
    uint16 actual_count = 0;
    uint32 scan_seq;
    uint32 scan_started_tick;
    int ret;
    int i;
    int round;
    int successful_rounds = 0;
    char connected_ssid[AW_WIFI_SSID_LEN + 1];

    (void)arg;
    scan_seq = g_wifi_diag_scan_seq;
    scan_started_tick = divoom_system_get_ticks();
    memset(&next_cache, 0, sizeof(next_cache));
    divoom_print("[WIFI_UI_DIAG] scan worker begin seq=%u tick=%u\n",
        scan_seq, scan_started_tick);
    connected_ssid[0] = '\0';
    divoom_astrootoo_settings_wifi_get_connected_ssid(connected_ssid,
        (int)sizeof(connected_ssid));
    for (round = 0; round < DIVOOM_ASTRO_WIFI_SCAN_ROUNDS; round++)
    {
        if (round > 0)
        {
            divoom_system_delay_ms(DIVOOM_ASTRO_WIFI_SCAN_ROUND_GAP_MS);
        }
        if (g_wifi_scan_cancel_requested)
        {
            goto scan_canceled;
        }
        /* 失败或没有可显示网络才重试；成功后立即发布列表。 */
        ret = divoom_wifi_ensure_sta_started();
        if (ret != 0)
        {
            divoom_print("[WIFI_UI_DIAG] scan ensure STA failed seq=%u round=%d ret=%d\n",
                scan_seq, round + 1, ret);
            continue;
        }
        divoom_print("[WIFI_UI_DIAG] scan API begin seq=%u round=%d/%d blocking=1\n",
            scan_seq, round + 1, DIVOOM_ASTRO_WIFI_SCAN_ROUNDS);
        ret = aw_wifi_scan_start(NULL, true);
        divoom_print("[WIFI_UI_DIAG] scan API end seq=%u ret=%d elapsed=%us\n",
            scan_seq, ret, divoom_system_get_ticks() - scan_started_tick);
        if (ret != 0)
        {
            continue;
        }
        scan_count = 0;
        ret = aw_wifi_scan_get_ap_num(&scan_count);
        divoom_print("[WIFI_UI_DIAG] scan count seq=%u ret=%d count=%u\n",
            scan_seq, ret, scan_count);
        if (ret != 0 || scan_count == 0)
        {
            continue;
        }
    
        records = (aw_wifi_ap_record_t *)divoom_system_malloc(
            (size_t)scan_count * sizeof(aw_wifi_ap_record_t));
        if (!records)
        {
            divoom_print("[WIFI_UI_DIAG] scan alloc failed seq=%u count=%u bytes=%u\n",
                scan_seq, scan_count,
                (uint32)((size_t)scan_count * sizeof(aw_wifi_ap_record_t)));
            continue;
        }
        actual_count = scan_count;
        ret = aw_wifi_scan_get_ap_records(&actual_count, records);
        divoom_print("[WIFI_UI_DIAG] scan records seq=%u ret=%d requested=%u actual=%u\n",
            scan_seq, ret, scan_count, actual_count);
        if (ret != 0)
        {
            divoom_system_free(records);
            continue;
        }
    
        /* 单次结果按 SSID 去重；排除当前连接后有可显示网络就结束。 */
        successful_rounds++;
        for (i = 0; i < (int)actual_count; i++)
        {
            char ssid[AW_WIFI_SSID_LEN + 1];
            int net_idx;
    
            memcpy(ssid, records[i].ssid, AW_WIFI_SSID_LEN);
            ssid[AW_WIFI_SSID_LEN] = '\0';
            divoom_print("[WIFI_UI_DIAG] scan AP seq=%u idx=%d ssid=%s auth=%d "
                "rssi=%d band=%d channel=%d bssid=%02x:%02x:%02x:%02x:%02x:%02x\n",
                scan_seq, i, ssid, records[i].authmode, records[i].rssi,
                records[i].band, records[i].channel,
                records[i].bssid[0], records[i].bssid[1], records[i].bssid[2],
                records[i].bssid[3], records[i].bssid[4], records[i].bssid[5]);
            if (ssid[0] == '\0'
                || (connected_ssid[0] && strcmp(ssid, connected_ssid) == 0))
            {
                continue;
            }
            net_idx = divoom_astrootoo_settings_wifi_scan_cache_index(&next_cache, ssid);
            if (net_idx >= 0)
            {
                if (records[i].rssi > next_cache.rssi[net_idx])
                {
                    next_cache.rssi[net_idx] = (int8)records[i].rssi;
                    next_cache.authmode[net_idx] = (uint8)records[i].authmode;
                }
                continue;
            }
            if (next_cache.count >= DIVOOM_ASTRO_SET_WIFI_MAX_ROW)
            {
                continue;
            }
            net_idx = next_cache.count++;
            strncpy(next_cache.ssid[net_idx], ssid, sizeof(next_cache.ssid[net_idx]) - 1u);
            next_cache.ssid[net_idx][sizeof(next_cache.ssid[net_idx]) - 1u] = '\0';
            next_cache.authmode[net_idx] = (uint8)records[i].authmode;
            next_cache.rssi[net_idx] = (int8)records[i].rssi;
        }
        divoom_system_free(records);
        divoom_print("[WIFI_UI_DIAG] scan round seq=%u round=%d/%d raw=%u merged=%d\n",
            scan_seq, round + 1, DIVOOM_ASTRO_WIFI_SCAN_ROUNDS,
            actual_count, next_cache.count);
        if (next_cache.count > 0)
        {
            break;
        }
    }
    if (g_wifi_scan_cancel_requested)
    {
        goto scan_canceled;
    }

    /* status 最后发布，timer 看到 READY 时缓存内容已经完整。 */
    g_wifi_scan_cache.count = next_cache.count;
    memcpy(g_wifi_scan_cache.ssid, next_cache.ssid, sizeof(g_wifi_scan_cache.ssid));
    memcpy(g_wifi_scan_cache.authmode, next_cache.authmode,
        sizeof(g_wifi_scan_cache.authmode));
    memcpy(g_wifi_scan_cache.rssi, next_cache.rssi, sizeof(g_wifi_scan_cache.rssi));
    g_wifi_scan_cache.status = next_cache.count > 0
        ? DIVOOM_ASTRO_WIFI_SCAN_READY : DIVOOM_ASTRO_WIFI_SCAN_FAILED;
    divoom_print("[WIFI_UI_DIAG] scan worker end seq=%u raw=%u unique=%d "
        "status=%d elapsed=%us successful_rounds=%d\n", scan_seq, actual_count, next_cache.count,
        g_wifi_scan_cache.status, divoom_system_get_ticks() - scan_started_tick,
        successful_rounds);
    return NULL;

scan_canceled:
    divoom_print("[WIFI_UI_DIAG] scan canceled seq=%u merged=%d\n",
        scan_seq, next_cache.count);
    g_wifi_scan_cache.count = 0;
    g_wifi_scan_cache.status = DIVOOM_ASTRO_WIFI_SCAN_IDLE;
    return NULL;
}

static void divoom_astrootoo_settings_wifi_scan_start(void)
{
    if (g_wifi_scan_cache.status == DIVOOM_ASTRO_WIFI_SCAN_RUNNING)
    {
        return;
    }
    g_wifi_scan_cancel_requested = 0;
    g_wifi_diag_scan_seq++;
    g_wifi_scan_cache.count = 0;
    g_wifi_scan_cache.started_ms = lv_tick_get();
    g_wifi_scan_cache.status = DIVOOM_ASTRO_WIFI_SCAN_RUNNING;
    divoom_print("[WIFI_UI_DIAG] scan start seq=%u lv_tick=%u stop_pending=%d\n",
        g_wifi_diag_scan_seq, g_wifi_scan_cache.started_ms,
        divoom_wifi_is_stop_connect_pending());
    if (!divoom_system_create_task(divoom_astrootoo_settings_wifi_scan_task,
        "wifi_scan", NULL, DIVOOM_ASTRO_WIFI_SCAN_TASK_STACK))
    {
        divoom_print("[WIFI_UI_DIAG] scan task create failed seq=%u\n",
            g_wifi_diag_scan_seq);
        g_wifi_scan_cache.status = DIVOOM_ASTRO_WIFI_SCAN_FAILED;
    }
}

static void divoom_astrootoo_settings_wifi_scan_append(
    Pdivoom_astrootoo_settings_ctrl phandle,
    const aw_wifi_ap_record_t *records, uint16 count)
{
    Pdivoom_wifi_menu_wifi_info saved_list;
    char connected_ssid[AW_WIFI_SSID_LEN + 1];
    int saved_num = 0;
    int saved_loaded = 0;
    int old_rows = phandle->wifi_row_count;
    int i;
    lv_group_t *group;

    if (!count)
        return;
    connected_ssid[0] = '\0';
    divoom_astrootoo_settings_wifi_get_connected_ssid(connected_ssid, sizeof(connected_ssid));
    saved_list = NULL;
    group = lv_group_get_default();
    for (i = 0; i < count; i++)
    {
        char ssid[AW_WIFI_SSID_LEN + 1];
        int net_idx, saved_idx, row;
        lv_obj_t *card;
        memcpy(ssid, records[i].ssid, AW_WIFI_SSID_LEN);
        ssid[AW_WIFI_SSID_LEN] = '\0';
        if (!ssid[0] || (connected_ssid[0] && strcmp(ssid, connected_ssid) == 0))
            continue;
        net_idx = divoom_astrootoo_settings_wifi_net_index(phandle, ssid);
        if (net_idx >= 0)
        {
            if (records[i].rssi >= phandle->wifi_rssi[net_idx])
            {
                phandle->wifi_rssi[net_idx] = (int8)records[i].rssi;
                phandle->wifi_authmode[net_idx] = (uint8)records[i].authmode;
            }
            continue;
        }
        if (phandle->wifi_net_count >= DIVOOM_ASTRO_SET_WIFI_MAX_ROW
            || phandle->wifi_row_count >= DIVOOM_ASTRO_SET_WIFI_MAX_ROW)
            break;
        net_idx = phandle->wifi_net_count;
        if (!saved_loaded)
        {
            saved_list = divoom_wifi_menu_get_all_wifi_info(&saved_num);
            saved_loaded = 1;
        }
        saved_idx = divoom_astrootoo_settings_wifi_saved_index(saved_list, saved_num, ssid);
        divoom_astrootoo_settings_wifi_append_network(phandle, ssid,
            saved_idx >= 0 ? (const char *)saved_list[saved_idx].pswd : NULL);
        phandle->wifi_authmode[net_idx] = (uint8)records[i].authmode;
        phandle->wifi_saved[net_idx] = saved_idx >= 0;
        phandle->wifi_rssi[net_idx] = (int8)records[i].rssi;
        row = phandle->wifi_row_count;
        card = divoom_astrootoo_settings_wifi_add_row(phandle, row,
            DIVOOM_ASTRO_SET_WIFI_TOP_GAP
                + row * (DIVOOM_ASTRO_SET_WIFI_ROW_H + DIVOOM_ASTRO_SET_WIFI_ROW_GAP),
            DIVOOM_ASTRO_SET_WIFI_KIND_NETWORK, "Wifi", phandle->wifi_ssid[net_idx], 1);
        if (!card)
        {
            phandle->wifi_net_count--;
            continue;
        }
        phandle->wifi_row_net_idx[row] = (int8)net_idx;
        phandle->wifi_row_count++;
        lv_obj_add_event_cb(card, divoom_astrootoo_settings_wifi_key_cb, LV_EVENT_KEY, phandle);
        if (group)
        {
            if (row == 0 && phandle->wifi_scan_key_btn)
                lv_group_remove_obj(phandle->wifi_scan_key_btn);
            lv_group_add_obj(group, card);
            if (row == 0)
                lv_group_focus_obj(card);
        }
        divoom_print("[WIFI_UI_DIAG] scan live append seq=%u row=%d ssid=%s rssi=%d status=%d\n",
            g_wifi_diag_scan_seq, row, ssid, records[i].rssi, g_wifi_scan_cache.status);
    }
    if (saved_list)
        divoom_system_free(saved_list);
    if (phandle->wifi_row_count > old_rows)
    {
        phandle->wifi_scan_applied = 1;
        divoom_astrootoo_settings_wifi_refresh_labels(phandle);
        if (g_wifi_flow.connect_state == DIVOOM_ASTRO_WIFI_CONNECT_IDLE)
        {
            if (g_wifi_scan_cache.status == DIVOOM_ASTRO_WIFI_SCAN_RUNNING
                && !g_wifi_scan_cancel_requested)
                divoom_astrootoo_settings_wifi_scan_spinner_show(phandle);
            else
                divoom_astrootoo_settings_wifi_connect_notice_hide(phandle);
        }
    }
}

static void divoom_astrootoo_settings_wifi_scan_refresh(
    Pdivoom_astrootoo_settings_ctrl phandle)
{
    uint16 count = DIVOOM_ASTRO_SET_WIFI_MAX_ROW;
    if (aw_wifi_scan_get_ap_progress(&count, g_wifi_scan_live_records) == 0 && count > 0)
        divoom_astrootoo_settings_wifi_scan_append(phandle, g_wifi_scan_live_records, count);
}

static void divoom_astrootoo_settings_wifi_scan_apply(
    Pdivoom_astrootoo_settings_ctrl phandle)
{
    int i;
    /* DONE only fills missing rows; it must not recreate/reorder live rows. */
    for (i = 0; i < g_wifi_scan_cache.count; i++)
    {
        memset(&g_wifi_scan_live_records[i], 0, sizeof(g_wifi_scan_live_records[i]));
        memcpy(g_wifi_scan_live_records[i].ssid, g_wifi_scan_cache.ssid[i], AW_WIFI_SSID_LEN);
        g_wifi_scan_live_records[i].authmode = g_wifi_scan_cache.authmode[i];
        g_wifi_scan_live_records[i].rssi = g_wifi_scan_cache.rssi[i];
    }
    divoom_astrootoo_settings_wifi_scan_append(phandle, g_wifi_scan_live_records,
        (uint16)g_wifi_scan_cache.count);
    divoom_print("[WIFI_UI_DIAG] scan apply complete seq=%u rows=%d\n",
        g_wifi_diag_scan_seq, phandle->wifi_row_count);
    if (phandle->wifi_row_count == 0)
        divoom_astrootoo_settings_wifi_connect_notice_show(phandle, DIVOOM_ASTRO_WIFI_NOTICE_SCAN_ERROR);
    else if (g_wifi_flow.connect_state == DIVOOM_ASTRO_WIFI_CONNECT_IDLE)
        divoom_astrootoo_settings_wifi_connect_notice_hide(phandle);
    else if (g_wifi_flow.connect_state == DIVOOM_ASTRO_WIFI_CONNECT_RUNNING)
        divoom_astrootoo_settings_wifi_connect_notice_show(phandle, DIVOOM_ASTRO_WIFI_NOTICE_CONNECTING);
    else if (g_wifi_flow.connect_state == DIVOOM_ASTRO_WIFI_CONNECT_FAILED)
        divoom_astrootoo_settings_wifi_connect_notice_show(phandle, DIVOOM_ASTRO_WIFI_NOTICE_ERROR);
    else if (g_wifi_flow.connect_state == DIVOOM_ASTRO_WIFI_CONNECT_SUCCESS)
        divoom_astrootoo_settings_wifi_connect_notice_show(phandle, DIVOOM_ASTRO_WIFI_NOTICE_SUCCESS);
}

void divoom_astrootoo_settings_build_wifi_add(Pdivoom_astrootoo_settings_ctrl phandle)
{
    phandle->wifi_row_count = 0;
    phandle->wifi_focus = 0;
    phandle->wifi_net_count = 0;
    phandle->wifi_scan_applied = 0;
    phandle->wifi_scan_finished = 0;
    phandle->wifi_scan_begin_ms = lv_tick_get();
    g_wifi_scan_timeout_reported = 0;
    memset(phandle->wifi_row, 0, sizeof(phandle->wifi_row));
    memset(phandle->wifi_row_chk, 0, sizeof(phandle->wifi_row_chk));
    memset(phandle->wifi_row_kind, 0, sizeof(phandle->wifi_row_kind));
    memset(phandle->wifi_authmode, 0, sizeof(phandle->wifi_authmode));
    memset(phandle->wifi_saved, 0, sizeof(phandle->wifi_saved));
    memset(phandle->wifi_rssi, DIVOOM_ASTRO_SET_WIFI_RSSI_UNKNOWN,
        sizeof(phandle->wifi_rssi));
    memset(phandle->wifi_row_net_idx, -1, sizeof(phandle->wifi_row_net_idx));
    memset(phandle->wifi_device_mac, 0, sizeof(phandle->wifi_device_mac));
    phandle->wifi_device_mac_ready = 0;
    phandle->wifi_device_mac_last_try_ms = 0;
    divoom_print("[WIFI_UI_DIAG] build add page lv_tick=%u flow_state=%d attempt=%d "
        "scan_status=%d scan_seq=%u stop_pending=%d\n",
        phandle->wifi_scan_begin_ms, g_wifi_flow.connect_state,
        g_wifi_flow.connect_attempt, g_wifi_scan_cache.status,
        g_wifi_diag_scan_seq, divoom_wifi_is_stop_connect_pending());

    phandle->direct_handle = divoom_menu_direct_ctrl_create(1,
        DIVOOM_ASTRO_SET_WIFI_MAX_ROW);
    divoom_astrootoo_ui_create_page_header_tab(phandle->root_obj, &phandle->ui,
        "AddWifi", DIVOOM_ASTROTOO_SETTINGS_HEADER, phandle->menu_id);
    phandle->content = divoom_astrootoo_ui_create_scroll_content(phandle->root_obj, &phandle->ui);
    lv_obj_set_scrollbar_mode(phandle->content, LV_SCROLLBAR_MODE_OFF);
    phandle->wifi_scan_key_btn = divoom_astrootoo_ui_create_key_capture_btn(
        phandle->root_obj, &phandle->ui, divoom_astrootoo_settings_wifi_key_cb, phandle);
    divoom_astrootoo_settings_wifi_wait_notice_create(phandle);

    if (g_wifi_flow.connect_state == DIVOOM_ASTRO_WIFI_CONNECT_IDLE)
    {
        /* 初次进入列表先终止后台重连，保证 AIC 扫描不被连接过程占用。 */
        divoom_wifi_stop_connect_wifi();
    }
    if (g_wifi_scan_cache.status == DIVOOM_ASTRO_WIFI_SCAN_READY
        && g_wifi_flow.connect_state != DIVOOM_ASTRO_WIFI_CONNECT_IDLE)
    {
        divoom_astrootoo_settings_wifi_scan_apply(phandle);
    }
    else if (g_wifi_scan_cache.status != DIVOOM_ASTRO_WIFI_SCAN_RUNNING
        && !divoom_wifi_is_stop_connect_pending())
    {
        divoom_astrootoo_settings_wifi_scan_start();
    }

    phandle->wifi_timer = lv_timer_create(divoom_astrootoo_settings_wifi_timer_cb, 100, phandle);
    if (phandle->wifi_timer)
    {
        lv_timer_set_repeat_count(phandle->wifi_timer, -1);
    }
    divoom_astrootoo_settings_wifi_group_bind(phandle, NULL);
}

static void divoom_astrootoo_settings_wifi_password_style_button(lv_obj_t *btn,
    uint32 bg_color, uint32 border_color, int radius)
{
    lv_obj_remove_style_all(btn);
    lv_obj_set_style_bg_color(btn, lv_color_hex(bg_color), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(border_color), 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn, radius, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
}

static const char *divoom_astrootoo_settings_wifi_password_key_icon(uint8 kind)
{
    if (kind == DIVOOM_ASTRO_WIFI_KEY_BACKSPACE)
    {
        return DIVOOM_ASTRO_WIFI_PASSWORD_BACKSPACE_ICON;
    }
    if (kind == DIVOOM_ASTRO_WIFI_KEY_SHIFT)
    {
        return DIVOOM_ASTRO_WIFI_PASSWORD_SHIFT_ICON;
    }
    if (kind == DIVOOM_ASTRO_WIFI_KEY_SPACE)
    {
        return DIVOOM_ASTRO_WIFI_PASSWORD_SPACE_ICON;
    }
    return NULL;
}

static void divoom_astrootoo_settings_wifi_password_refresh(Pdivoom_astrootoo_settings_ctrl phandle)
{
    int key_count;
    int i;

    key_count = (int)(sizeof(g_wifi_password_keys) / sizeof(g_wifi_password_keys[0]));
    if (phandle->wifi_password_field_lbl)
    {
        lv_label_set_text(phandle->wifi_password_field_lbl, phandle->wifi_password);
    }
    for (i = 0; i < DIVOOM_ASTRO_WIFI_PASSWORD_TOOL_COUNT; i++)
    {
        int focused;

        focused = (phandle->wifi_password_focus_zone == DIVOOM_ASTRO_WIFI_PASSWORD_ZONE_TOOLBAR
            && phandle->wifi_password_toolbar_focus == i);
        if (phandle->wifi_password_toolbar[i])
        {
            lv_obj_set_style_bg_color(phandle->wifi_password_toolbar[i],
                lv_color_hex(focused ? 0xF8B634 : 0xDDD8D0), 0);
            lv_obj_set_style_border_color(phandle->wifi_password_toolbar[i],
                lv_color_hex(focused ? 0x2C2A27 : 0xD4CEC6), 0);
            lv_obj_set_style_border_width(phandle->wifi_password_toolbar[i], focused ? 2 : 1, 0);
        }
    }
    for (i = 0; i < key_count; i++)
    {
        lv_obj_t *key_obj;
        uint32 bg_color;

        key_obj = phandle->wifi_password_key[i];
        if (!key_obj)
        {
            continue;
        }
        bg_color = 0xFAF7F3;
        if (g_wifi_password_keys[i].kind == DIVOOM_ASTRO_WIFI_KEY_SHIFT
            && phandle->wifi_password_uppercase)
        {
            bg_color = 0xF8E3B7;
        }
        if (phandle->wifi_password_focus_zone == DIVOOM_ASTRO_WIFI_PASSWORD_ZONE_KEYBOARD
            && phandle->wifi_password_key_focus == i)
        {
            bg_color = 0xF8B634;
        }
        lv_obj_set_style_bg_color(key_obj, lv_color_hex(bg_color), 0);

        if (g_wifi_password_keys[i].kind == DIVOOM_ASTRO_WIFI_KEY_CHAR
            && g_wifi_password_keys[i].text
            && g_wifi_password_keys[i].text[0] >= 'a'
            && g_wifi_password_keys[i].text[0] <= 'z'
            && g_wifi_password_keys[i].text[1] == '\0')
        {
            lv_obj_t *label;
            char text[2];

            label = lv_obj_get_child(key_obj, 0);
            text[0] = g_wifi_password_keys[i].text[0];
            if (phandle->wifi_password_uppercase)
            {
                text[0] = (char)(text[0] - 'a' + 'A');
            }
            text[1] = '\0';
            lv_label_set_text(label, text);
        }
    }
}

static int divoom_astrootoo_settings_wifi_password_find_at(int row, int col)
{
    int key_count;
    int i;

    key_count = (int)(sizeof(g_wifi_password_keys) / sizeof(g_wifi_password_keys[0]));
    for (i = 0; i < key_count; i++)
    {
        if ((int)g_wifi_password_keys[i].row == row
            && col >= (int)g_wifi_password_keys[i].col
            && col < (int)g_wifi_password_keys[i].col + (int)g_wifi_password_keys[i].col_span)
        {
            return i;
        }
    }
    return -1;
}

static int divoom_astrootoo_settings_wifi_password_move_horizontal(int current, int delta)
{
    int key_count;
    int first;
    int last;
    int i;
    uint8 row;

    key_count = (int)(sizeof(g_wifi_password_keys) / sizeof(g_wifi_password_keys[0]));
    row = g_wifi_password_keys[current].row;
    first = -1;
    last = -1;
    for (i = 0; i < key_count; i++)
    {
        if (g_wifi_password_keys[i].row != row)
        {
            continue;
        }
        if (first < 0)
        {
            first = i;
        }
        last = i;
        if (delta > 0 && i > current)
        {
            return i;
        }
    }
    if (delta < 0)
    {
        for (i = current - 1; i >= first; i--)
        {
            if (g_wifi_password_keys[i].row == row)
            {
                return i;
            }
        }
        return last;
    }
    return first;
}

static int divoom_astrootoo_settings_wifi_password_move_vertical(int current, int delta)
{
    int row;
    int col;
    int i;
    int found;

    row = (int)g_wifi_password_keys[current].row + delta;
    if (row < 0)
    {
        row = 0;
    }
    if (row > 6)
    {
        row = 6;
    }
    col = g_wifi_password_keys[current].col;
    for (i = col; i <= 10; i++)
    {
        found = divoom_astrootoo_settings_wifi_password_find_at(row, i);
        if (found >= 0)
        {
            return found;
        }
    }
    for (i = col - 1; i >= 0; i--)
    {
        found = divoom_astrootoo_settings_wifi_password_find_at(row, i);
        if (found >= 0)
        {
            return found;
        }
    }
    return current;
}

static void divoom_astrootoo_settings_wifi_password_append(Pdivoom_astrootoo_settings_ctrl phandle,
    char ch)
{
    if (phandle->wifi_password_len >= DIVOOM_ASTRO_WIFI_PASSWORD_MAX_LEN)
    {
        return;
    }
    phandle->wifi_password[phandle->wifi_password_len++] = ch;
    phandle->wifi_password[phandle->wifi_password_len] = '\0';
}

static int divoom_astrootoo_settings_wifi_ensure_loading_buf(
    Pdivoom_astrootoo_settings_ctrl phandle)
{
    if (!phandle)
    {
        return 0;
    }
    if (!phandle->wifi_loading_buf)
    {
        phandle->wifi_loading_buf = (uint8 *)divoom_system_malloc(
            DIVOOM_ASTRO_WIFI_LOADING_BUF_BYTES);
    }
    return phandle->wifi_loading_buf != NULL;
}

static void divoom_astrootoo_settings_wifi_loading_gif_stop(
    Pdivoom_astrootoo_settings_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }
    if (phandle->wifi_loading_gif_handle)
    {
        divoom_lvgl_play_gif_file_destory_handle_reuse_image_buf(
            phandle->wifi_loading_gif_handle, 0);
        phandle->wifi_loading_gif_handle = NULL;
    }
    if (phandle->wifi_connect_spinner)
    {
        lv_obj_add_flag(phandle->wifi_connect_spinner, LV_OBJ_FLAG_HIDDEN);
    }
}

static void divoom_astrootoo_settings_wifi_loading_gif_start(
    Pdivoom_astrootoo_settings_ctrl phandle)
{
    char gif_path[128];

    if (!phandle || !phandle->wifi_connect_card || !phandle->wifi_connect_spinner
        || !divoom_astrootoo_settings_wifi_ensure_loading_buf(phandle))
    {
        return;
    }

    divoom_astrootoo_settings_wifi_loading_gif_stop(phandle);
    divoom_menu_control_get_img_path2(DIVOOM_ASTRO_WIFI_LOADING_GIF, gif_path);
    if (divoom_app_com_check_file_exist(gif_path) == 0)
    {
        return;
    }

    lv_obj_set_parent(phandle->wifi_connect_spinner, phandle->wifi_connect_card);
    lv_image_set_pivot(phandle->wifi_connect_spinner, 0, 0);
    lv_img_set_zoom(phandle->wifi_connect_spinner, 256);
    lv_obj_set_pos(phandle->wifi_connect_spinner, DIVOOM_ASTRO_WIFI_LOADING_X,
        DIVOOM_ASTRO_WIFI_LOADING_Y);
    lv_obj_set_size(phandle->wifi_connect_spinner, DIVOOM_ASTRO_WIFI_LOADING_W,
        DIVOOM_ASTRO_WIFI_LOADING_H);
    lv_obj_clear_flag(phandle->wifi_connect_spinner, LV_OBJ_FLAG_HIDDEN);

    divoom_lv_img_dsc_init_fmt(&phandle->wifi_loading_dsc,
        DIVOOM_ASTRO_WIFI_LOADING_W, DIVOOM_ASTRO_WIFI_LOADING_H,
        LV_COLOR_FORMAT_RGB565A8, phandle->wifi_loading_buf);
    phandle->wifi_loading_gif_handle =
        divoom_lvgl_play_gif_file_create_handle_reuse_image_buf(gif_path,
            phandle->wifi_connect_spinner, &phandle->wifi_loading_dsc,
            DIVOOM_ASTRO_WIFI_LOADING_W, DIVOOM_ASTRO_WIFI_LOADING_H, 0, 0);
    if (phandle->wifi_loading_gif_handle)
    {
        divoom_lvgl_play_gif_file_start_play(phandle->wifi_loading_gif_handle);
    }
    lv_img_set_src(phandle->wifi_connect_spinner, &phandle->wifi_loading_dsc);
    lv_obj_invalidate(phandle->wifi_connect_spinner);
}

static void divoom_astrootoo_settings_wifi_connect_notice_hide(
    Pdivoom_astrootoo_settings_ctrl phandle)
{
    if (!phandle || !phandle->wifi_connect_notice)
    {
        return;
    }
    divoom_astrootoo_settings_wifi_loading_gif_stop(phandle);
    lv_obj_add_flag(phandle->wifi_connect_notice, LV_OBJ_FLAG_HIDDEN);
}

static void divoom_astrootoo_settings_wifi_scan_spinner_show(
    Pdivoom_astrootoo_settings_ctrl phandle)
{
    if (!phandle || !phandle->wifi_connect_spinner || !phandle->wifi_connect_notice)
        return;
    if (!phandle->wifi_loading_gif_handle)
        divoom_astrootoo_settings_wifi_loading_gif_start(phandle);
    /* 复用原 GIF，保持原来的屏幕位置和大小；仅隐藏弹窗以显示实时列表。 */
    lv_obj_set_parent(phandle->wifi_connect_spinner, phandle->root_obj);
    lv_image_set_pivot(phandle->wifi_connect_spinner, 0, 0);
    lv_img_set_zoom(phandle->wifi_connect_spinner, 256);
    lv_obj_set_pos(phandle->wifi_connect_spinner,
        DIVOOM_ASTRO_WIFI_STATUS_CARD_X + DIVOOM_ASTRO_WIFI_LOADING_X,
        DIVOOM_ASTRO_WIFI_STATUS_CARD_Y + DIVOOM_ASTRO_WIFI_LOADING_Y);
    lv_obj_set_size(phandle->wifi_connect_spinner, DIVOOM_ASTRO_WIFI_LOADING_W,
        DIVOOM_ASTRO_WIFI_LOADING_H);
    lv_obj_clear_flag(phandle->wifi_connect_spinner, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(phandle->wifi_connect_spinner, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(phandle->wifi_connect_notice, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(phandle->wifi_connect_spinner);
}

static void divoom_astrootoo_settings_wifi_scan_cancel_ui(
    Pdivoom_astrootoo_settings_ctrl phandle)
{
    g_wifi_scan_cancel_requested = 1;
    phandle->wifi_scan_finished = 1;
    phandle->wifi_scan_applied = 1;
    divoom_astrootoo_settings_wifi_connect_notice_hide(phandle);
    /* 只取消尚在等扫描收尾的连接，已经发出的 join 保持原交互。 */
    if (g_wifi_flow.connect_state == DIVOOM_ASTRO_WIFI_CONNECT_RUNNING
        && g_wifi_flow.connect_attempt == 0)
    {
        divoom_main_set_wifi_add_network_route_pending(0);
        memset(&g_wifi_flow, 0, sizeof(g_wifi_flow));
    }
    divoom_print("[WIFI_UI_DIAG] scan canceled by key seq=%u rows=%d worker_status=%d\n",
        g_wifi_diag_scan_seq, phandle->wifi_row_count, g_wifi_scan_cache.status);
}

static void divoom_astrootoo_settings_wifi_wait_notice_create(
    Pdivoom_astrootoo_settings_ctrl phandle)
{
    lv_obj_t *notice;
    lv_obj_t *card;

    notice = divoom_astrootoo_settings_wifi_overlay_create(phandle->root_obj,
        0, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    phandle->wifi_connect_notice = notice;

    card = lv_obj_create(notice);
    lv_obj_remove_style_all(card);
    lv_obj_set_size(card, DIVOOM_ASTRO_WIFI_STATUS_CARD_W,
        DIVOOM_ASTRO_WIFI_STATUS_CARD_H);
    lv_obj_set_pos(card, DIVOOM_ASTRO_WIFI_STATUS_CARD_X,
        DIVOOM_ASTRO_WIFI_STATUS_CARD_Y);
    lv_obj_set_style_bg_color(card, lv_color_hex(DIVOOM_ASTRO_MODAL_BG), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    phandle->wifi_connect_card = card;

    if (!divoom_astrootoo_settings_wifi_ensure_loading_buf(phandle))
    {
        return;
    }
    phandle->wifi_connect_spinner = lv_img_create(card);
    lv_obj_set_pos(phandle->wifi_connect_spinner, DIVOOM_ASTRO_WIFI_LOADING_X,
        DIVOOM_ASTRO_WIFI_LOADING_Y);
    lv_obj_set_size(phandle->wifi_connect_spinner, DIVOOM_ASTRO_WIFI_LOADING_W,
        DIVOOM_ASTRO_WIFI_LOADING_H);
    lv_obj_add_flag(phandle->wifi_connect_spinner, LV_OBJ_FLAG_HIDDEN);

    phandle->wifi_connect_icon = lv_img_create(card);
    divoom_menu_control_lv_img_set_src(phandle->wifi_connect_icon,
        DIVOOM_ASTRO_WIFI_SUCCESS_ICON);
    lv_obj_set_pos(phandle->wifi_connect_icon, 128, 32);
    lv_obj_add_flag(phandle->wifi_connect_icon, LV_OBJ_FLAG_HIDDEN);

    phandle->wifi_connect_divider = lv_obj_create(card);
    lv_obj_remove_style_all(phandle->wifi_connect_divider);
    lv_obj_set_size(phandle->wifi_connect_divider, 260, 2);
    lv_obj_set_pos(phandle->wifi_connect_divider, 30, 100);
    lv_obj_set_style_bg_color(phandle->wifi_connect_divider,
        lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
    lv_obj_set_style_bg_opa(phandle->wifi_connect_divider, LV_OPA_COVER, 0);
    lv_obj_clear_flag(phandle->wifi_connect_divider, LV_OBJ_FLAG_SCROLLABLE);

    phandle->wifi_connect_notice_lbl = lv_label_create(card);
    lv_obj_set_size(phandle->wifi_connect_notice_lbl, 300, 32);
    lv_obj_set_pos(phandle->wifi_connect_notice_lbl, 10, 104);
    lv_label_set_long_mode(phandle->wifi_connect_notice_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(phandle->wifi_connect_notice_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(phandle->wifi_connect_notice_lbl,
        divoom_menu_control_get_font_info(24, phandle->menu_id), 0);

    phandle->wifi_connect_subtitle_lbl = lv_label_create(card);
    lv_obj_set_size(phandle->wifi_connect_subtitle_lbl, 300, 32);
    lv_obj_set_pos(phandle->wifi_connect_subtitle_lbl, 10, 138);
    lv_label_set_long_mode(phandle->wifi_connect_subtitle_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(phandle->wifi_connect_subtitle_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(phandle->wifi_connect_subtitle_lbl,
        lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(phandle->wifi_connect_subtitle_lbl,
        divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
    lv_obj_add_flag(notice, LV_OBJ_FLAG_HIDDEN);
}

static void divoom_astrootoo_settings_wifi_connect_notice_show(
    Pdivoom_astrootoo_settings_ctrl phandle, uint8 variant)
{
    const char *text;
    int loading;

    if (!phandle->wifi_connect_notice || !phandle->wifi_connect_notice_lbl)
    {
        return;
    }
    divoom_astrootoo_settings_wifi_toast_hide(phandle);
    loading = variant == DIVOOM_ASTRO_WIFI_NOTICE_SCANNING
        || variant == DIVOOM_ASTRO_WIFI_NOTICE_CONNECTING;
    if (variant == DIVOOM_ASTRO_WIFI_NOTICE_SCANNING)
    {
        text = divoom_menu_control_get_disp_str("WifiScanning");
    }
    else if (variant == DIVOOM_ASTRO_WIFI_NOTICE_CONNECTING)
    {
        text = divoom_menu_control_get_disp_str("WifiConnectingShort");
    }
    else if (variant == DIVOOM_ASTRO_WIFI_NOTICE_SUCCESS)
    {
        text = divoom_menu_control_get_disp_str("WifiConnectSuccess");
    }
    else if (variant == DIVOOM_ASTRO_WIFI_NOTICE_ERROR)
    {
        text = divoom_menu_control_get_disp_str("WifiWrongPassword");
    }
    else if (variant == DIVOOM_ASTRO_WIFI_NOTICE_SCAN_ERROR)
    {
        text = divoom_menu_control_get_disp_str("NetworkConnectFail");
    }
    else
    {
        text = divoom_menu_control_get_disp_str("WifiDisconnected");
    }
    lv_label_set_text(phandle->wifi_connect_notice_lbl, text);
    lv_obj_set_style_text_color(phandle->wifi_connect_notice_lbl,
        lv_color_hex((variant == DIVOOM_ASTRO_WIFI_NOTICE_ERROR
            || variant == DIVOOM_ASTRO_WIFI_NOTICE_SCAN_ERROR)
            ? DIVOOM_ASTRO_DANGER : DIVOOM_ASTRO_INK), 0);
    if (loading)
    {
        divoom_astrootoo_settings_wifi_loading_gif_start(phandle);
        lv_obj_add_flag(phandle->wifi_connect_icon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(phandle->wifi_connect_divider, 30, 100);
        lv_obj_set_pos(phandle->wifi_connect_notice_lbl, 10, 104);
        lv_obj_set_pos(phandle->wifi_connect_subtitle_lbl, 10, 138);
        lv_label_set_text(phandle->wifi_connect_subtitle_lbl,
            variant == DIVOOM_ASTRO_WIFI_NOTICE_CONNECTING ? g_wifi_flow.ssid : "");
    }
    else
    {
        divoom_astrootoo_settings_wifi_loading_gif_stop(phandle);
        divoom_menu_control_lv_img_set_src(phandle->wifi_connect_icon,
            variant == DIVOOM_ASTRO_WIFI_NOTICE_SUCCESS
                ? DIVOOM_ASTRO_WIFI_SUCCESS_ICON : DIVOOM_ASTRO_WIFI_ERROR_ICON);
        lv_obj_clear_flag(phandle->wifi_connect_icon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(phandle->wifi_connect_divider, 30, 114);
        lv_obj_set_pos(phandle->wifi_connect_notice_lbl, 10, 132);
        lv_label_set_text(phandle->wifi_connect_subtitle_lbl, "");
    }
    lv_obj_clear_flag(phandle->wifi_connect_notice, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(phandle->wifi_connect_notice);
}

static void divoom_astrootoo_settings_wifi_connect_start_attempt(void)
{
    g_wifi_flow.connect_attempt++;
    g_wifi_flow.connect_started_ms = lv_tick_get();
    g_wifi_diag_connect_heartbeat_ms = g_wifi_flow.connect_started_ms;
    divoom_print("[WIFI_UI_DIAG] connect attempt=%d/%d ssid=%s auth=%d "
        "pswd_len=%d first=%d page_tick=%u\n",
        g_wifi_flow.connect_attempt, DIVOOM_ASTRO_WIFI_CONNECT_MAX_ATTEMPTS,
        g_wifi_flow.ssid, g_wifi_flow.authmode,
        (int)strlen(g_wifi_flow.password), g_wifi_flow.first_connect,
        g_wifi_flow.connect_started_ms);
    divoom_wifi_connect_ssid(g_wifi_flow.ssid, g_wifi_flow.password);
}

static uint8 divoom_astrootoo_settings_wifi_is_password_error(int reason)
{
    return reason == AW_WIFI_STA_CONNECT_ERR_PASSWORD_TOO_SHORT
        || reason == AW_WIFI_STA_CONNECT_ERR_AUTH
        || reason == AW_WIFI_STA_CONNECT_ERR_INVALID_PASSWORD
        || reason == AW_WIFI_STA_CONNECT_ERR_WRONG_KEY;
}

static void divoom_astrootoo_settings_wifi_connect_begin(
    Pdivoom_astrootoo_settings_ctrl phandle, int net_idx, const char *password,
    uint8 first_connect)
{
    if (!phandle || net_idx < 0 || net_idx >= phandle->wifi_net_count)
    {
        return;
    }
    strncpy(g_wifi_flow.ssid, phandle->wifi_ssid[net_idx], sizeof(g_wifi_flow.ssid) - 1u);
    g_wifi_flow.ssid[sizeof(g_wifi_flow.ssid) - 1u] = '\0';
    strncpy(g_wifi_flow.password, password ? password : "",
        sizeof(g_wifi_flow.password) - 1u);
    g_wifi_flow.password[sizeof(g_wifi_flow.password) - 1u] = '\0';
    g_wifi_flow.authmode = phandle->wifi_authmode[net_idx];
    g_wifi_flow.first_connect = first_connect;
    g_wifi_flow.focus_net_idx = (int8)net_idx;
    g_wifi_flow.connect_state = DIVOOM_ASTRO_WIFI_CONNECT_RUNNING;
    g_wifi_flow.connect_attempt = 0;
    g_wifi_diag_connect_heartbeat_ms = 0;
    divoom_print("[WIFI_UI_DIAG] connect begin net_idx=%d ssid=%s saved=%d "
        "auth=%d rssi=%d pswd_len=%d first=%d\n",
        net_idx, g_wifi_flow.ssid, phandle->wifi_saved[net_idx],
        g_wifi_flow.authmode, phandle->wifi_rssi[net_idx],
        (int)strlen(g_wifi_flow.password), first_connect);
    /* 先让 LVGL 绘制连接卡；实际连接由下一次 100ms timer 回调发起。 */
    divoom_astrootoo_settings_wifi_connect_notice_show(phandle,
        DIVOOM_ASTRO_WIFI_NOTICE_CONNECTING);
}

static void divoom_astrootoo_settings_wifi_promote_connected(
    Pdivoom_astrootoo_settings_ctrl phandle, int net_idx)
{
    char ssid[AW_WIFI_SSID_LEN + 1];
    char password[DIVOOM_ASTRO_WIFI_PASSWORD_MAX_LEN + 1];
    uint8 authmode;
    uint8 saved;
    int8 rssi;
    int i;

    if (!phandle || net_idx < 0 || net_idx >= phandle->wifi_net_count)
    {
        return;
    }
    if (net_idx > 0)
    {
        strncpy(ssid, phandle->wifi_ssid[net_idx], sizeof(ssid));
        strncpy(password, phandle->wifi_pswd[net_idx], sizeof(password));
        authmode = phandle->wifi_authmode[net_idx];
        saved = phandle->wifi_saved[net_idx];
        rssi = phandle->wifi_rssi[net_idx];
        for (i = net_idx; i > 0; i--)
        {
            memcpy(phandle->wifi_ssid[i], phandle->wifi_ssid[i - 1],
                sizeof(phandle->wifi_ssid[i]));
            memcpy(phandle->wifi_pswd[i], phandle->wifi_pswd[i - 1],
                sizeof(phandle->wifi_pswd[i]));
            phandle->wifi_authmode[i] = phandle->wifi_authmode[i - 1];
            phandle->wifi_saved[i] = phandle->wifi_saved[i - 1];
            phandle->wifi_rssi[i] = phandle->wifi_rssi[i - 1];
        }
        memcpy(phandle->wifi_ssid[0], ssid, sizeof(phandle->wifi_ssid[0]));
        memcpy(phandle->wifi_pswd[0], password, sizeof(phandle->wifi_pswd[0]));
        phandle->wifi_authmode[0] = authmode;
        phandle->wifi_saved[0] = saved;
        phandle->wifi_rssi[0] = rssi;
    }

    /* 真源在连接状态变化后重排数据。 */
    {
        int network_row;

        network_row = 0;
        for (i = 0; i < phandle->wifi_row_count; i++)
        {
            if (phandle->wifi_row_kind[i] == DIVOOM_ASTRO_SET_WIFI_KIND_NETWORK)
            {
                lv_obj_t *label;

                label = lv_obj_get_child(phandle->wifi_row[i], 0);
                if (label && network_row < phandle->wifi_net_count)
                {
                    lv_label_set_text(label, phandle->wifi_ssid[network_row]);
                    divoom_astrootoo_ui_list_label_refresh_marquee(phandle->wifi_row[i], label);
                }
                phandle->wifi_row_net_idx[i] = (int8)network_row;
                network_row++;
            }
        }
    }
    divoom_astrootoo_settings_wifi_refresh_labels(phandle);
    if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI)
    {
        int connected_row;

        /* 数据提升后按网络身份重新聚焦，不能继续停在已换内容的物理行。 */
        connected_row = divoom_astrootoo_settings_wifi_row_index_from_net(phandle, 0);
        divoom_astrootoo_settings_wifi_group_bind(phandle,
            connected_row >= 0 ? phandle->wifi_row[connected_row] : NULL);
    }
}

static void divoom_astrootoo_settings_wifi_connect_process(
    Pdivoom_astrootoo_settings_ctrl phandle)
{
    int result;
    int error_reason;
    uint32 now_ms;
    uint32 elapsed_ms;

    if (g_wifi_flow.connect_state == DIVOOM_ASTRO_WIFI_CONNECT_SUCCESS)
    {
        if ((uint32)(lv_tick_get() - g_wifi_flow.result_started_ms)
            >= DIVOOM_ASTRO_WIFI_STATUS_AUTO_DISMISS_MS)
        {
            /* MQTT 沿用旧连接时不会再次回调，成功卡结束后必须本地完成分流。 */
            g_wifi_flow.connect_state = DIVOOM_ASTRO_WIFI_CONNECT_IDLE;
            divoom_astrootoo_settings_wifi_connect_notice_hide(phandle);
            if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_ADD)
                divoom_astrootoo_settings_wifi_route_after_manual_success();
            else
                divoom_astrootoo_settings_wifi_drop_stale_add_route();
        }
        return;
    }
    if (g_wifi_flow.connect_state != DIVOOM_ASTRO_WIFI_CONNECT_RUNNING)
    {
        return;
    }
    /* A selection can arrive before DONE. Wait for the scan worker to release STA. */
    if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_ADD
        && g_wifi_scan_cache.status == DIVOOM_ASTRO_WIFI_SCAN_RUNNING)
        return;
    if (g_wifi_flow.connect_attempt == 0)
    {
        divoom_astrootoo_settings_wifi_connect_start_attempt();
        return;
    }
    result = divoom_wifi_get_manual_connect_result();
    error_reason = divoom_wifi_get_manual_connect_error_reason();
    now_ms = lv_tick_get();
    elapsed_ms = (uint32)(now_ms - g_wifi_flow.connect_started_ms);
    if ((uint32)(now_ms - g_wifi_diag_connect_heartbeat_ms)
        >= DIVOOM_ASTRO_WIFI_DIAG_HEARTBEAT_MS)
    {
        g_wifi_diag_connect_heartbeat_ms = now_ms;
        divoom_print("[WIFI_UI_DIAG] connect heartbeat attempt=%d/%d ssid=%s "
            "elapsed_ms=%u result=%d reason=%d flow=%d page=%d awaiting_route=%d\n",
            g_wifi_flow.connect_attempt, DIVOOM_ASTRO_WIFI_CONNECT_MAX_ATTEMPTS,
            g_wifi_flow.ssid, elapsed_ms, result, error_reason,
            g_wifi_flow.connect_state, phandle ? phandle->page : -1,
            g_wifi_flow.awaiting_route);
    }
    if (result == DIVOOM_WIFI_CONNECTED)
    {
        int net_idx;

        divoom_print("[WIFI_UI_DIAG] connect success attempt=%d ssid=%s "
            "elapsed_ms=%u first=%d\n", g_wifi_flow.connect_attempt,
            g_wifi_flow.ssid, elapsed_ms, g_wifi_flow.first_connect);

        g_wifi_status_refresh_after_network_change = 1;
        if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI)
        {
            divoom_astrootoo_settings_wifi_request_status_refresh(phandle, 1);
            g_wifi_status_refresh_after_network_change = 0;
        }
        net_idx = divoom_astrootoo_settings_wifi_net_index(phandle, g_wifi_flow.ssid);
        if (net_idx >= 0)
        {
            phandle->wifi_saved[net_idx] = 1;
            strncpy(phandle->wifi_pswd[net_idx], g_wifi_flow.password,
                sizeof(phandle->wifi_pswd[net_idx]) - 1u);
            phandle->wifi_pswd[net_idx][sizeof(phandle->wifi_pswd[net_idx]) - 1u] = '\0';
            divoom_astrootoo_settings_wifi_promote_connected(phandle, net_idx);
        }
        if (g_wifi_flow.first_connect)
        {
            g_wifi_flow.connect_state = DIVOOM_ASTRO_WIFI_CONNECT_SUCCESS;
            g_wifi_flow.result_started_ms = lv_tick_get();
            divoom_astrootoo_settings_wifi_connect_notice_show(phandle,
                DIVOOM_ASTRO_WIFI_NOTICE_SUCCESS);
        }
        else
        {
            memset(&g_wifi_flow, 0, sizeof(g_wifi_flow));
            divoom_astrootoo_settings_wifi_connect_notice_hide(phandle);
            if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_ADD)
            {
                /* 添加页保存网络重连成功不显示成功卡，直接返回状态页。 */
                divoom_menu_control_return_pre();
            }
            else
            {
                /* 状态页切网成功：与模拟器 reconnected 一致，顶栏 toast「已连接该网络」。 */
                divoom_astrootoo_settings_wifi_refresh_labels(phandle);
                divoom_astrootoo_settings_wifi_toast_show(phandle,
                    divoom_menu_control_get_disp_str("WifiConnectedHint"),
                    DIVOOM_ASTRO_INK);
            }
        }
        return;
    }
    if (result == DIVOOM_WIFI_NO_SSID || result == DIVOOM_WIFI_OTHER_ERR
        || elapsed_ms >= DIVOOM_ASTRO_WIFI_CONNECT_TIMEOUT_MS)
    {
        divoom_print("[WIFI_UI_DIAG] connect attempt done attempt=%d ssid=%s "
            "elapsed_ms=%u result=%d reason=%d timeout=%d\n",
            g_wifi_flow.connect_attempt, g_wifi_flow.ssid, elapsed_ms,
            result, error_reason,
            elapsed_ms >= DIVOOM_ASTRO_WIFI_CONNECT_TIMEOUT_MS);
        if (g_wifi_flow.connect_attempt < DIVOOM_ASTRO_WIFI_CONNECT_MAX_ATTEMPTS)
        {
            divoom_print("[WIFI_UI_DIAG] connect retry next_attempt=%d ssid=%s\n",
                g_wifi_flow.connect_attempt + 1, g_wifi_flow.ssid);
            divoom_astrootoo_settings_wifi_connect_start_attempt();
        }
        else
        {
            uint8 notice_variant;

            g_wifi_flow.connect_state = DIVOOM_ASTRO_WIFI_CONNECT_FAILED;
            g_wifi_flow.had_failure = 1;
            g_wifi_flow.result_started_ms = lv_tick_get();
            notice_variant = divoom_astrootoo_settings_wifi_is_password_error(
                error_reason)
                ? DIVOOM_ASTRO_WIFI_NOTICE_ERROR
                : DIVOOM_ASTRO_WIFI_NOTICE_SCAN_ERROR;
            divoom_print("[WIFI_UI_DIAG] connect final failure attempts=%d ssid=%s "
                "result=%d reason=%d notice=%d\n",
                g_wifi_flow.connect_attempt, g_wifi_flow.ssid,
                result, error_reason, notice_variant);
            divoom_astrootoo_settings_wifi_connect_notice_show(phandle,
                notice_variant);
        }
    }
}

static void divoom_astrootoo_settings_wifi_toast_hide(
    Pdivoom_astrootoo_settings_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }
    if (phandle->wifi_toast_timer)
    {
        lv_timer_del(phandle->wifi_toast_timer);
        phandle->wifi_toast_timer = NULL;
    }
    if (phandle->wifi_password_toast)
    {
        lv_obj_del(phandle->wifi_password_toast);
        phandle->wifi_password_toast = NULL;
    }
}

static void divoom_astrootoo_settings_wifi_password_toast_timer_cb(lv_timer_t *timer)
{
    Pdivoom_astrootoo_settings_ctrl phandle;

    phandle = (Pdivoom_astrootoo_settings_ctrl)lv_timer_get_user_data(timer);
    if (phandle)
    {
        if (phandle->wifi_toast_timer == timer)
        {
            phandle->wifi_toast_timer = NULL;
        }
        if (phandle->wifi_password_toast)
        {
            lv_obj_del(phandle->wifi_password_toast);
            phandle->wifi_password_toast = NULL;
        }
    }
    lv_timer_del(timer);
}

static void divoom_astrootoo_settings_wifi_toast_show(
    Pdivoom_astrootoo_settings_ctrl phandle, const char *text, uint32 color)
{
    lv_obj_t *toast;
    lv_obj_t *label;

    if (!phandle || !phandle->root_obj || !text)
    {
        return;
    }
    divoom_astrootoo_settings_wifi_toast_hide(phandle);
    toast = lv_obj_create(phandle->root_obj);
    lv_obj_remove_style_all(toast);
    lv_obj_add_flag(toast, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(toast, DIVOOM_ASTRO_TOP_TOAST_W, DIVOOM_ASTRO_TOP_TOAST_MIN_H);
    lv_obj_set_pos(toast, DIVOOM_ASTRO_TOP_TOAST_X, DIVOOM_ASTRO_TOP_TOAST_Y);
    lv_obj_set_style_bg_color(toast, lv_color_hex(DIVOOM_ASTRO_BG), 0);
    lv_obj_set_style_bg_opa(toast, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(toast, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
    lv_obj_set_style_border_width(toast, 2, 0);
    lv_obj_set_style_radius(toast, DIVOOM_ASTRO_RADIUS_SM, 0);
    lv_obj_set_style_pad_all(toast, 0, 0);
    lv_obj_clear_flag(toast, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(toast, LV_OBJ_FLAG_CLICKABLE);
    label = lv_label_create(toast);
    lv_label_set_text(label, text);
    lv_obj_set_width(label, DIVOOM_ASTRO_TOP_TOAST_W - DIVOOM_ASTRO_TOP_TOAST_PAD_X * 2);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_text_font(label,
        divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
    lv_obj_move_foreground(toast);
    phandle->wifi_password_toast = toast;
    phandle->wifi_toast_timer = lv_timer_create(
        divoom_astrootoo_settings_wifi_password_toast_timer_cb,
        DIVOOM_ASTRO_WIFI_STATUS_AUTO_DISMISS_MS, phandle);
}

static void divoom_astrootoo_settings_wifi_password_invalid_show(
    Pdivoom_astrootoo_settings_ctrl phandle)
{
    divoom_astrootoo_settings_wifi_toast_show(phandle,
        divoom_menu_control_get_disp_str("WifiPasswordInvalidLength"),
        DIVOOM_ASTRO_DANGER);
}

static void divoom_astrootoo_settings_wifi_password_activate(Pdivoom_astrootoo_settings_ctrl phandle)
{
    const divoom_astrootoo_wifi_password_key_t *key;
    char ch;

    if (phandle->wifi_password_focus_zone == DIVOOM_ASTRO_WIFI_PASSWORD_ZONE_TOOLBAR)
    {
        if (phandle->wifi_password_toolbar_focus == 1)
        {
            phandle->wifi_password_len = 0;
            phandle->wifi_password[0] = '\0';
        }
        else if (g_wifi_flow.ssid[0] != '\0')
        {
            if (g_wifi_flow.authmode != AW_WIFI_AUTH_OPEN
                && (phandle->wifi_password_len < DIVOOM_ASTRO_WIFI_PASSWORD_MIN_LEN
                    || phandle->wifi_password_len > DIVOOM_ASTRO_WIFI_PASSWORD_MAX_LEN))
            {
                divoom_astrootoo_settings_wifi_password_invalid_show(phandle);
                return;
            }
            strncpy(g_wifi_flow.password, phandle->wifi_password,
                sizeof(g_wifi_flow.password) - 1u);
            g_wifi_flow.password[sizeof(g_wifi_flow.password) - 1u] = '\0';
            divoom_print("[WIFI_CFG_DIAG] submit ssid=%s auth=%d pswd_len=%d\n",
                g_wifi_flow.ssid, g_wifi_flow.authmode,
                (int)strlen(g_wifi_flow.password));
            g_wifi_flow.connect_state = DIVOOM_ASTRO_WIFI_CONNECT_RUNNING;
            g_wifi_flow.connect_attempt = 0;
            g_wifi_flow.awaiting_route = 1;
            divoom_main_set_wifi_add_network_route_pending(1);
            /* 先返回列表绘制连接提示，下一次列表 timer 再发起可能阻塞的切网。 */
            divoom_menu_control_return_pre();
            return;
        }
        divoom_astrootoo_settings_wifi_password_refresh(phandle);
        return;
    }

    key = &g_wifi_password_keys[phandle->wifi_password_key_focus];
    if (key->kind == DIVOOM_ASTRO_WIFI_KEY_BACKSPACE)
    {
        if (phandle->wifi_password_len > 0)
        {
            phandle->wifi_password[--phandle->wifi_password_len] = '\0';
        }
    }
    else if (key->kind == DIVOOM_ASTRO_WIFI_KEY_SHIFT)
    {
        phandle->wifi_password_uppercase = phandle->wifi_password_uppercase ? 0 : 1;
    }
    else if (key->kind == DIVOOM_ASTRO_WIFI_KEY_SPACE)
    {
        divoom_astrootoo_settings_wifi_password_append(phandle, ' ');
    }
    else if (key->text && key->text[0])
    {
        ch = key->text[0];
        if (phandle->wifi_password_uppercase && ch >= 'a' && ch <= 'z')
        {
            ch = (char)(ch - 'a' + 'A');
        }
        divoom_astrootoo_settings_wifi_password_append(phandle, ch);
    }
    divoom_astrootoo_settings_wifi_password_refresh(phandle);
}

static void divoom_astrootoo_settings_wifi_password_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    uint8 key_code;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    if (!divoom_astrootoo_ui_key_is_menu_press(e, key_code))
    {
        return;
    }
    phandle = (Pdivoom_astrootoo_settings_ctrl)lv_event_get_user_data(e);
    if (!phandle)
    {
        return;
    }
    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        g_wifi_flow.ssid[0] = '\0';
        divoom_menu_control_return_pre();
        return;
    }
    if (divoom_astrootoo_ui_key_is_up(key_code))
    {
        if (phandle->wifi_password_focus_zone == DIVOOM_ASTRO_WIFI_PASSWORD_ZONE_KEYBOARD
            && g_wifi_password_keys[phandle->wifi_password_key_focus].row == 0)
        {
            phandle->wifi_password_focus_zone = DIVOOM_ASTRO_WIFI_PASSWORD_ZONE_TOOLBAR;
            phandle->wifi_password_toolbar_focus = 0;
        }
        else if (phandle->wifi_password_focus_zone == DIVOOM_ASTRO_WIFI_PASSWORD_ZONE_KEYBOARD)
        {
            phandle->wifi_password_key_focus = divoom_astrootoo_settings_wifi_password_move_vertical(
                phandle->wifi_password_key_focus, -1);
        }
    }
    else if (divoom_astrootoo_ui_key_is_down(key_code))
    {
        if (phandle->wifi_password_focus_zone == DIVOOM_ASTRO_WIFI_PASSWORD_ZONE_TOOLBAR)
        {
            phandle->wifi_password_focus_zone = DIVOOM_ASTRO_WIFI_PASSWORD_ZONE_KEYBOARD;
            phandle->wifi_password_key_focus = 0;
        }
        else
        {
            phandle->wifi_password_key_focus = divoom_astrootoo_settings_wifi_password_move_vertical(
                phandle->wifi_password_key_focus, 1);
        }
    }
    else if (divoom_astrootoo_ui_key_is_left(key_code)
        || divoom_astrootoo_ui_key_is_right(key_code))
    {
        int delta;

        delta = divoom_astrootoo_ui_key_is_left(key_code) ? -1 : 1;
        if (phandle->wifi_password_focus_zone == DIVOOM_ASTRO_WIFI_PASSWORD_ZONE_TOOLBAR)
        {
            phandle->wifi_password_toolbar_focus =
                (phandle->wifi_password_toolbar_focus + delta
                    + DIVOOM_ASTRO_WIFI_PASSWORD_TOOL_COUNT)
                % DIVOOM_ASTRO_WIFI_PASSWORD_TOOL_COUNT;
        }
        else
        {
            phandle->wifi_password_key_focus = divoom_astrootoo_settings_wifi_password_move_horizontal(
                phandle->wifi_password_key_focus, delta);
        }
    }
    else if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        divoom_astrootoo_settings_wifi_password_activate(phandle);
        return;
    }
    else
    {
        return;
    }
    divoom_astrootoo_settings_wifi_password_refresh(phandle);
}

void divoom_astrootoo_settings_build_wifi_password(Pdivoom_astrootoo_settings_ctrl phandle)
{
    lv_obj_t *title;
    lv_obj_t *field;
    int key_count;
    int i;

    phandle->wifi_password_focus_zone = DIVOOM_ASTRO_WIFI_PASSWORD_ZONE_KEYBOARD;
    phandle->wifi_password_toolbar_focus = 0;
    phandle->wifi_password_key_focus = 0;
    phandle->wifi_password_len = 0;
    phandle->wifi_password_uppercase = 0;
    phandle->wifi_password[0] = '\0';

    title = lv_label_create(phandle->root_obj);
    lv_label_set_text(title, g_wifi_flow.ssid);
    lv_obj_set_size(title, DIVOOM_SCREEN_WIDTH, 32);
    lv_obj_set_pos(title, 0, DIVOOM_ASTRO_WIFI_PASSWORD_TITLE_Y);
    lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);

    field = lv_obj_create(phandle->root_obj);
    lv_obj_remove_style_all(field);
    lv_obj_set_size(field, DIVOOM_ASTRO_WIFI_PASSWORD_FIELD_W,
        DIVOOM_ASTRO_WIFI_PASSWORD_FIELD_H);
    lv_obj_set_pos(field, DIVOOM_ASTRO_WIFI_PASSWORD_FIELD_X,
        DIVOOM_ASTRO_WIFI_PASSWORD_FIELD_Y);
    lv_obj_set_style_bg_color(field, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(field, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(field, lv_color_hex(0xD4CEC6), 0);
    lv_obj_set_style_border_width(field, 2, 0);
    lv_obj_set_style_radius(field, 12, 0);
    lv_obj_set_style_pad_hor(field, 12, 0);
    lv_obj_set_style_pad_ver(field, 0, 0);
    lv_obj_clear_flag(field, LV_OBJ_FLAG_SCROLLABLE);
    phandle->wifi_password_field_lbl = lv_label_create(field);
    lv_obj_set_width(phandle->wifi_password_field_lbl,
        DIVOOM_ASTRO_WIFI_PASSWORD_FIELD_W - 28);
    lv_label_set_long_mode(phandle->wifi_password_field_lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_color(phandle->wifi_password_field_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(phandle->wifi_password_field_lbl,
        divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
    lv_obj_align(phandle->wifi_password_field_lbl, LV_ALIGN_LEFT_MID, 0, 0);

    for (i = 0; i < DIVOOM_ASTRO_WIFI_PASSWORD_TOOL_COUNT; i++)
    {
        lv_obj_t *icon;

        phandle->wifi_password_toolbar[i] = lv_btn_create(phandle->root_obj);
        divoom_astrootoo_settings_wifi_password_style_button(phandle->wifi_password_toolbar[i],
            0xDDD8D0, 0xD4CEC6, 12);
        lv_obj_set_size(phandle->wifi_password_toolbar[i], DIVOOM_ASTRO_WIFI_PASSWORD_TOOL_W,
            DIVOOM_ASTRO_WIFI_PASSWORD_TOOL_H);
        lv_obj_set_pos(phandle->wifi_password_toolbar[i],
            i == 0 ? DIVOOM_ASTRO_WIFI_PASSWORD_CONFIRM_X : DIVOOM_ASTRO_WIFI_PASSWORD_CLEAR_X,
            DIVOOM_ASTRO_WIFI_PASSWORD_FIELD_Y);
        icon = lv_img_create(phandle->wifi_password_toolbar[i]);
        divoom_menu_control_lv_img_set_src(icon,
            i == 0 ? DIVOOM_ASTRO_WIFI_PASSWORD_CONFIRM_ICON : DIVOOM_ASTRO_WIFI_PASSWORD_CLEAR_ICON);
        lv_obj_center(icon);
    }

    key_count = (int)(sizeof(g_wifi_password_keys) / sizeof(g_wifi_password_keys[0]));
    for (i = 0; i < key_count && i < DIVOOM_ASTRO_SET_WIFI_KEY_COUNT; i++)
    {
        const char *icon_path;
        lv_obj_t *key_obj;

        key_obj = lv_btn_create(phandle->root_obj);
        phandle->wifi_password_key[i] = key_obj;
        divoom_astrootoo_settings_wifi_password_style_button(key_obj, 0xFAF7F3, 0xD4CEC6, 5);
        lv_obj_set_size(key_obj, g_wifi_password_keys[i].w, g_wifi_password_keys[i].h);
        lv_obj_set_pos(key_obj, g_wifi_password_keys[i].x, g_wifi_password_keys[i].y);
        icon_path = divoom_astrootoo_settings_wifi_password_key_icon(g_wifi_password_keys[i].kind);
        if (icon_path)
        {
            lv_obj_t *icon;

            icon = lv_img_create(key_obj);
            divoom_menu_control_lv_img_set_src(icon, icon_path);
            lv_obj_center(icon);
        }
        else
        {
            lv_obj_t *label;

            label = lv_label_create(key_obj);
            lv_label_set_text(label, g_wifi_password_keys[i].text ? g_wifi_password_keys[i].text : "");
            lv_obj_set_style_text_color(label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
            lv_obj_set_style_text_font(label,
                divoom_menu_control_get_font_info(18, phandle->menu_id), 0);
            lv_obj_center(label);
        }
    }

    phandle->wifi_password_key_btn = divoom_astrootoo_ui_create_key_capture_btn(
        phandle->root_obj, &phandle->ui, divoom_astrootoo_settings_wifi_password_key_cb, phandle);

    divoom_astrootoo_settings_wifi_password_refresh(phandle);
    divoom_astrootoo_settings_wifi_group_bind(phandle, phandle->wifi_password_key_btn);
}

static void divoom_astrootoo_settings_wifi_build_info_tail(
    Pdivoom_astrootoo_settings_ctrl phandle, lv_coord_t y, int *idx_io)
{
    const char *info_keys[] = { "CurrentWifi", "ConnectionStatus",
        "IP Address", "Mac Address" };
    int i;
    int idx;

    if (!phandle || !idx_io)
    {
        return;
    }
    idx = *idx_io;
    if (phandle->wifi_net_count > 0)
    {
        if (phandle->wifi_info_separator
            && lv_obj_is_valid(phandle->wifi_info_separator))
        {
            lv_obj_del(phandle->wifi_info_separator);
        }
        phandle->wifi_info_separator = lv_obj_create(phandle->content);
        lv_obj_remove_style_all(phandle->wifi_info_separator);
        lv_obj_set_size(phandle->wifi_info_separator, DIVOOM_ASTRO_SET_WIFI_ROW_W,
            DIVOOM_ASTRO_SET_WIFI_SEPARATOR_H);
        lv_obj_set_pos(phandle->wifi_info_separator, DIVOOM_ASTRO_SET_WIFI_ROW_X, y);
        lv_obj_set_style_bg_color(phandle->wifi_info_separator,
            lv_color_hex(0xB8B0A8), 0);
        lv_obj_set_style_bg_opa(phandle->wifi_info_separator, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(phandle->wifi_info_separator, 0, 0);
        lv_obj_set_style_radius(phandle->wifi_info_separator, 0, 0);
        lv_obj_clear_flag(phandle->wifi_info_separator,
            LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        y += DIVOOM_ASTRO_SET_WIFI_SEPARATOR_H + DIVOOM_ASTRO_SET_WIFI_SEPARATOR_PAD;
    }
    else if (phandle->wifi_info_separator
        && lv_obj_is_valid(phandle->wifi_info_separator))
    {
        lv_obj_del(phandle->wifi_info_separator);
        phandle->wifi_info_separator = NULL;
    }
    for (i = 0; i < DIVOOM_ASTRO_SET_WIFI_INFO_ROWS
        && idx < DIVOOM_ASTRO_SET_WIFI_MAX_ROW; i++)
    {
        divoom_astrootoo_settings_wifi_add_row(phandle, idx++, y,
            DIVOOM_ASTRO_SET_WIFI_KIND_INFO, info_keys[i], "—", 0);
        y += DIVOOM_ASTRO_SET_WIFI_ROW_H + DIVOOM_ASTRO_SET_WIFI_INFO_GAP;
    }
    *idx_io = idx;
}

static void divoom_astrootoo_settings_wifi_rebuild_saved_rows(
    Pdivoom_astrootoo_settings_ctrl phandle)
{
    lv_coord_t y;
    int idx;
    int i;

    if (!phandle || phandle->page != DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI)
    {
        return;
    }
    if (phandle->wifi_saved_title && lv_obj_is_valid(phandle->wifi_saved_title))
    {
        lv_obj_del(phandle->wifi_saved_title);
    }
    phandle->wifi_saved_title = NULL;
    if (phandle->wifi_info_separator
        && lv_obj_is_valid(phandle->wifi_info_separator))
    {
        lv_obj_del(phandle->wifi_info_separator);
    }
    phandle->wifi_info_separator = NULL;
    for (i = DIVOOM_ASTRO_SET_WIFI_ROOT_BASE_ROWS; i < phandle->wifi_row_count; i++)
    {
        if (phandle->wifi_row[i] && lv_obj_is_valid(phandle->wifi_row[i]))
        {
            lv_group_t *g;

            g = lv_group_get_default();
            if (g && divoom_astrootoo_settings_wifi_row_focusable(
                    phandle->wifi_row_kind[i]))
            {
                lv_group_remove_obj(phandle->wifi_row[i]);
            }
            lv_obj_del(phandle->wifi_row[i]);
        }
        phandle->wifi_row[i] = NULL;
        phandle->wifi_row_val[i] = NULL;
        phandle->wifi_row_chk[i] = NULL;
        phandle->wifi_row_kind[i] = DIVOOM_ASTRO_SET_WIFI_KIND_INFO;
        phandle->wifi_row_net_idx[i] = -1;
    }
    memset(phandle->wifi_ssid, 0, sizeof(phandle->wifi_ssid));
    memset(phandle->wifi_pswd, 0, sizeof(phandle->wifi_pswd));
    memset(phandle->wifi_authmode, 0, sizeof(phandle->wifi_authmode));
    memset(phandle->wifi_saved, 0, sizeof(phandle->wifi_saved));
    memset(phandle->wifi_rssi, DIVOOM_ASTRO_SET_WIFI_RSSI_UNKNOWN,
        sizeof(phandle->wifi_rssi));
    phandle->wifi_net_count = 0;
    phandle->wifi_row_count = DIVOOM_ASTRO_SET_WIFI_ROOT_BASE_ROWS;
    divoom_astrootoo_settings_wifi_load_saved_networks(phandle);

    if (phandle->wifi_net_count <= 0)
    {
        y = DIVOOM_ASTRO_SET_WIFI_TOP_GAP
            + DIVOOM_ASTRO_SET_WIFI_ROOT_BASE_ROWS
                * (DIVOOM_ASTRO_SET_WIFI_ROW_H + DIVOOM_ASTRO_SET_WIFI_ROW_GAP);
        idx = DIVOOM_ASTRO_SET_WIFI_ROOT_BASE_ROWS;
        divoom_astrootoo_settings_wifi_build_info_tail(phandle, y, &idx);
        phandle->wifi_row_count = idx;
        divoom_astrootoo_settings_wifi_rebuild_direct_focus(phandle);
        divoom_astrootoo_settings_wifi_refresh_labels(phandle);
        return;
    }
    y = DIVOOM_ASTRO_SET_WIFI_TOP_GAP
        + DIVOOM_ASTRO_SET_WIFI_ROOT_BASE_ROWS
            * (DIVOOM_ASTRO_SET_WIFI_ROW_H + DIVOOM_ASTRO_SET_WIFI_ROW_GAP);
    phandle->wifi_saved_title = divoom_astrootoo_settings_wifi_add_section_title(
        phandle, y, "WifiSavedNetworks");
    y += DIVOOM_ASTRO_SET_WIFI_SECTION_H + DIVOOM_ASTRO_SET_WIFI_ROW_GAP;
    idx = DIVOOM_ASTRO_SET_WIFI_ROOT_BASE_ROWS;
    for (i = 0; i < phandle->wifi_net_count
        && idx < DIVOOM_ASTRO_SET_WIFI_MAX_ROW; i++)
    {
        lv_obj_t *card;

        card = divoom_astrootoo_settings_wifi_add_row(phandle, idx, y,
            DIVOOM_ASTRO_SET_WIFI_KIND_NETWORK, "Wifi", phandle->wifi_ssid[i], 1);
        phandle->wifi_row_net_idx[idx] = (int8)i;
        if (card)
        {
            lv_obj_add_event_cb(card, divoom_astrootoo_settings_wifi_key_cb,
                LV_EVENT_KEY, phandle);
        }
        idx++;
        y += DIVOOM_ASTRO_SET_WIFI_ROW_H + DIVOOM_ASTRO_SET_WIFI_ROW_GAP;
    }
    divoom_astrootoo_settings_wifi_build_info_tail(phandle, y, &idx);
    phandle->wifi_row_count = idx;
    divoom_astrootoo_settings_wifi_rebuild_direct_focus(phandle);
    divoom_astrootoo_settings_wifi_refresh_labels(phandle);
}

void divoom_astrootoo_settings_build_wifi(Pdivoom_astrootoo_settings_ctrl phandle)
{
    lv_obj_t *card;
    lv_coord_t y;
    int idx;

    divoom_main_request_device_share_code();
    phandle->wifi_row_count = 0;
    phandle->wifi_focus = 0;
    phandle->wifi_net_count = 0;
    phandle->wifi_saved_title = NULL;
    phandle->wifi_info_separator = NULL;
    memset(phandle->wifi_row, 0, sizeof(phandle->wifi_row));
    memset(phandle->wifi_row_val, 0, sizeof(phandle->wifi_row_val));
    memset(phandle->wifi_row_chk, 0, sizeof(phandle->wifi_row_chk));
    memset(phandle->wifi_row_kind, 0, sizeof(phandle->wifi_row_kind));
    memset(phandle->wifi_authmode, 0, sizeof(phandle->wifi_authmode));
    memset(phandle->wifi_saved, 0, sizeof(phandle->wifi_saved));
    memset(phandle->wifi_rssi, DIVOOM_ASTRO_SET_WIFI_RSSI_UNKNOWN,
        sizeof(phandle->wifi_rssi));
    memset(phandle->wifi_row_net_idx, -1, sizeof(phandle->wifi_row_net_idx));
    divoom_astrootoo_settings_wifi_request_status_refresh(phandle, 1);
    g_wifi_status_refresh_after_network_change = 0;
    /* 进状态页丢掉添加网络分流残留，避免定时器/按键误回表盘。 */
    divoom_astrootoo_settings_wifi_drop_stale_add_route();

    phandle->direct_handle = divoom_menu_direct_ctrl_create(1,
        DIVOOM_ASTRO_SET_WIFI_MAX_ROW);
    divoom_astrootoo_ui_create_page_header_tab(phandle->root_obj, &phandle->ui,
        "Wi-Fi", DIVOOM_ASTROTOO_SETTINGS_HEADER, phandle->menu_id);
    phandle->content = divoom_astrootoo_ui_create_scroll_content(phandle->root_obj, &phandle->ui);
    lv_obj_set_height(phandle->content, DIVOOM_ASTRO_CONTENT_H - 8);
    lv_obj_set_scrollbar_mode(phandle->content, LV_SCROLLBAR_MODE_OFF);

    y = DIVOOM_ASTRO_SET_WIFI_TOP_GAP;
    idx = 0;

    card = divoom_astrootoo_settings_wifi_add_row(phandle, idx++, y,
        DIVOOM_ASTRO_SET_WIFI_KIND_ADD, "AddWifi", NULL, 1);
    if (card)
    {
        lv_obj_add_event_cb(card, divoom_astrootoo_settings_wifi_key_cb,
            LV_EVENT_KEY, phandle);
    }
    y += DIVOOM_ASTRO_SET_WIFI_ROW_H + DIVOOM_ASTRO_SET_WIFI_ROW_GAP;
    card = divoom_astrootoo_settings_wifi_add_row(phandle, idx++, y,
        DIVOOM_ASTRO_SET_WIFI_KIND_CONNECTION_CODE, "DeviceConnectionCode", NULL, 1);
    if (card)
    {
        lv_obj_add_event_cb(card, divoom_astrootoo_settings_wifi_key_cb,
            LV_EVENT_KEY, phandle);
    }
    phandle->wifi_row_count = idx;
    divoom_astrootoo_settings_wifi_rebuild_saved_rows(phandle);
    divoom_astrootoo_settings_wifi_wait_notice_create(phandle);

    divoom_astrootoo_settings_wifi_refresh_labels(phandle);
    phandle->wifi_timer = lv_timer_create(divoom_astrootoo_settings_wifi_timer_cb, 100, phandle);
    if (phandle->wifi_timer)
    {
        lv_timer_set_repeat_count(phandle->wifi_timer, -1);
    }
    divoom_astrootoo_settings_wifi_group_bind(phandle, phandle->wifi_row[0]);
}

void divoom_astrootoo_settings_wifi_group_detach(Pdivoom_astrootoo_settings_ctrl phandle)
{
    lv_group_t *g;
    int j;

    if (!phandle)
    {
        return;
    }
    g = lv_group_get_default();
    if (!g)
    {
        return;
    }
    for (j = 0; j < phandle->wifi_row_count; j++)
    {
        if (phandle->wifi_row[j])
        {
            lv_group_remove_obj(phandle->wifi_row[j]);
        }
    }
    if (phandle->wifi_password_key_btn)
    {
        lv_group_remove_obj(phandle->wifi_password_key_btn);
    }
    if (phandle->wifi_scan_key_btn)
    {
        lv_group_remove_obj(phandle->wifi_scan_key_btn);
    }
}

void divoom_astrootoo_settings_wifi_group_bind(Pdivoom_astrootoo_settings_ctrl phandle,
    lv_obj_t *focus_target)
{
    lv_group_t *g;
    int i;
    lv_obj_t *focus;

    g = lv_group_get_default();
    if (!g)
    {
        return;
    }

    if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI
        && g_wifi_status_refresh_after_network_change)
    {
        divoom_astrootoo_settings_wifi_request_status_refresh(phandle, 1);
        g_wifi_status_refresh_after_network_change = 0;
    }

    /* 子页返回时 pause 已停 timer。重建保存列表后必须立刻刷右侧状态/图标，
     * 不能只看 pending：首次进页后 MAC/IP 已缓存时 pending 为 0，否则会停在「—」且对勾隐藏。 */
    if (!phandle->wifi_timer)
    {
        if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_ADD)
        {
            phandle->wifi_timer = lv_timer_create(
                divoom_astrootoo_settings_wifi_timer_cb, 100, phandle);
        }
        else if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI)
        {
            divoom_astrootoo_settings_wifi_rebuild_saved_rows(phandle);
            phandle->wifi_timer = lv_timer_create(
                divoom_astrootoo_settings_wifi_timer_cb, 100, phandle);
        }
        if (phandle->wifi_timer)
        {
            lv_timer_set_repeat_count(phandle->wifi_timer, -1);
        }
    }
    if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_ADD
        && g_wifi_flow.connect_state == DIVOOM_ASTRO_WIFI_CONNECT_RUNNING)
    {
        /* 密码页返回时复用已隐藏提示的扫描页，恢复连接等待层并置于列表之上。 */
        divoom_astrootoo_settings_wifi_connect_notice_show(phandle,
            DIVOOM_ASTRO_WIFI_NOTICE_CONNECTING);
    }
    lv_group_remove_all_objs(g);
    if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_PASSWORD)
    {
        if (phandle->wifi_password_key_btn)
        {
            lv_group_add_obj(g, phandle->wifi_password_key_btn);
            lv_group_focus_obj(phandle->wifi_password_key_btn);
        }
        return;
    }
    if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_ADD
        && phandle->wifi_row_count == 0)
    {
        if (phandle->wifi_scan_key_btn)
        {
            lv_group_add_obj(g, phandle->wifi_scan_key_btn);
            lv_group_focus_obj(phandle->wifi_scan_key_btn);
        }
        return;
    }
    for (i = 0; i < phandle->wifi_row_count; i++)
    {
        if (!phandle->wifi_row[i])
        {
            continue;
        }
        if (!divoom_astrootoo_settings_wifi_row_focusable(phandle->wifi_row_kind[i]))
        {
            continue;
        }
        lv_group_add_obj(g, phandle->wifi_row[i]);
    }
    focus = NULL;
    if (focus_target && lv_obj_is_valid(focus_target))
    {
        int j;

        for (j = 0; j < phandle->wifi_row_count; j++)
        {
            if (phandle->wifi_row[j] == focus_target
                && divoom_astrootoo_settings_wifi_row_focusable(phandle->wifi_row_kind[j]))
            {
                focus = focus_target;
                break;
            }
        }
    }
    if (!focus)
    {
        for (i = 0; i < phandle->wifi_row_count; i++)
        {
            if (phandle->wifi_row[i]
                && divoom_astrootoo_settings_wifi_row_focusable(phandle->wifi_row_kind[i]))
            {
                focus = phandle->wifi_row[i];
                break;
            }
        }
    }
    if (focus)
    {
        lv_group_focus_obj(focus);
        divoom_astrootoo_ui_focus_scroll(focus);
    }
}

uint8 divoom_astrootoo_settings_wifi_request_app_pairing_prompt(void)
{
    if (divoom_menu_control_get_active_menu_id()
        != DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_WIFI_ADD)
    {
        return 0;
    }
    g_wifi_app_pairing_prompt_pending = 1;
    return 1;
}

#ifdef DIVOOM_SIMULATOR
int divoom_astrootoo_settings_wifi_sim_app_pairing_prompt_is_open(void)
{
    return g_wifi_app_pairing_prompt_open ? 1 : 0;
}
#endif

void divoom_astrootoo_settings_wifi_clean(Pdivoom_astrootoo_settings_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }
    if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_ADD)
    {
        g_wifi_scan_cancel_requested = 1;
        g_wifi_app_pairing_prompt_pending = 0;
        g_wifi_app_pairing_prompt_open = 0;
    }
    if (phandle->wifi_timer)
    {
        lv_timer_del(phandle->wifi_timer);
        phandle->wifi_timer = NULL;
    }
    divoom_astrootoo_settings_wifi_loading_gif_stop(phandle);
    if (phandle->wifi_loading_buf)
    {
        divoom_system_free(phandle->wifi_loading_buf);
        phandle->wifi_loading_buf = NULL;
    }
    divoom_print("[WIFI_UI_DIAG] clean page=%d flow_state=%d attempt=%d ssid=%s "
        "scan_status=%d scan_seq=%u\n", phandle->page,
        g_wifi_flow.connect_state, g_wifi_flow.connect_attempt,
        g_wifi_flow.ssid, g_wifi_scan_cache.status, g_wifi_diag_scan_seq);
    if (phandle->wifi_toast_timer)
    {
        lv_timer_del(phandle->wifi_toast_timer);
        phandle->wifi_toast_timer = NULL;
    }
    phandle->wifi_password_toast = NULL;
}

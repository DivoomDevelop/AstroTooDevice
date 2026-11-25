#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_menu_direct_ctrl.h"
#include "divoom_main.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_astrootoo_settings.h"
#include "divoom_astrootoo_settings_internal.h"
#include "divoom_astrootoo_settings_ex.h"
#include "divoom_astrootoo_settings_wifi.h"
#include "divoom_astrootoo_settings_slideshow.h"
#include "divoom_astrootoo_time_pick.h"
#include "divoom_bt_music.h"
#include "divoom_tws_host.h"
#include "divoom_net_app.h"
#include "divoom_system_setting_json.h"
#include "divoom_system.h"
#include "divoom_astrootoo_theme.h"
#include "divoom_power_monitor.h"
#include "divoom_lvgl_set_language.h"
#include "divoom_btcore_key_light.h"
#include "divoom_idle_poweroff.h"
#include "divoom_paths.h"
#include "divoom_port_fs.h"
#include "divoom_product_test_format_i18n.h"
#include "divoom_product_test_usr_check.h"
#include "divoom_product_test_format.h"

#ifdef DIVOOM_TIMES_VIBE
#include "divoom_ota_jl.h"
#endif
#include "divoom_onoff.h"
#include "divoom_pixel_format.h"
#include <time.h>
#if defined(_WIN32)
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif

/*
 * AstroToo 系统设置（单页滚动）
 * 真源: AstroTooUI flowtoo DeviceSettingsRootScreen.tsx + golden settings.png
 *   顶栏(#243746) + 顶部亮度/音量行内滑条 + 分组(显示/媒体与音效/连接/偏好/系统管理)
 */

#define DIVOOM_ASTROTOO_SETTINGS_UI_DIR     "26/"
#define DIVOOM_ASTROTOO_SETTINGS_HEADER     DIVOOM_ASTROTOO_SETTINGS_UI_DIR "at_settings_header.png"
#define DIVOOM_ASTROTOO_SETTINGS_CHEVRON    DIVOOM_ASTROTOO_SETTINGS_UI_DIR "at_settings_chevron.png"
#define DIVOOM_ASTROTOO_SETTINGS_BRIGHT_ICO DIVOOM_ASTROTOO_SETTINGS_UI_DIR "at_settings_brightness.png"
#define DIVOOM_ASTROTOO_SETTINGS_VOLUME_ICO DIVOOM_ASTROTOO_SETTINGS_UI_DIR "at_settings_volume.png"
#define DIVOOM_ASTROTOO_SETTINGS_SWITCH_ON  DIVOOM_ASTROTOO_SETTINGS_UI_DIR "at_settings_switch_on.png"
#define DIVOOM_ASTROTOO_SETTINGS_SWITCH_OFF DIVOOM_ASTROTOO_SETTINGS_UI_DIR "at_settings_switch_off.png"
#define DIVOOM_ASTROTOO_SETTINGS_CHEVRON_UP DIVOOM_ASTROTOO_SETTINGS_UI_DIR "arrow_up.png"
#define DIVOOM_ASTROTOO_SETTINGS_CHEVRON_DN  DIVOOM_ASTROTOO_SETTINGS_UI_DIR "arrow_down.png"
/* 设置行尾电池：接近 toast 充电图标外形（无闪电）。宽仍约 43，高按 40×40 图里电池比例 */
#define DIVOOM_ASTROTOO_SETTINGS_BATTERY_W  43
#define DIVOOM_ASTROTOO_SETTINGS_BATTERY_H  24
#define DIVOOM_ASTROTOO_SETTINGS_BAT_FRAME_COLOR  0x666666
#define DIVOOM_ASTROTOO_SETTINGS_BAT_RED_COLOR    0xBD5252
#define DIVOOM_ASTROTOO_SETTINGS_BAT_GREEN_COLOR  0x52BD66
#define DIVOOM_ASTROTOO_SETTINGS_BAT_BODY_X       0
#define DIVOOM_ASTROTOO_SETTINGS_BAT_BODY_Y       2
#define DIVOOM_ASTROTOO_SETTINGS_BAT_BODY_W       37
#define DIVOOM_ASTROTOO_SETTINGS_BAT_BODY_H       20
#define DIVOOM_ASTROTOO_SETTINGS_BAT_BODY_RADIUS  3
#define DIVOOM_ASTROTOO_SETTINGS_BAT_BODY_BORDER  2
/* toast：极柱 3×8、与壳体空 1px，上下略窄；不要叠在壳体上 */
#define DIVOOM_ASTROTOO_SETTINGS_BAT_NIPPLE_X     38
#define DIVOOM_ASTROTOO_SETTINGS_BAT_NIPPLE_Y     8
#define DIVOOM_ASTROTOO_SETTINGS_BAT_NIPPLE_W     3
#define DIVOOM_ASTROTOO_SETTINGS_BAT_NIPPLE_H     8
#define DIVOOM_ASTROTOO_SETTINGS_BAT_NIPPLE_RAD   2
#define DIVOOM_ASTROTOO_SETTINGS_BAT_FILL_X       3
#define DIVOOM_ASTROTOO_SETTINGS_BAT_FILL_Y       5
#define DIVOOM_ASTROTOO_SETTINGS_BAT_FILL_W       31
#define DIVOOM_ASTROTOO_SETTINGS_BAT_FILL_H       14
#define DIVOOM_ASTROTOO_SETTINGS_BAT_FILL_RADIUS  2
#define DIVOOM_ASTROTOO_SETTINGS_BAT_LEVEL_MAX    9

/* 夜览 L3（真源 DeviceSettingsNightModeScreen.module.css） */
#define DIVOOM_ASTRO_NIGHT_SCROLL_GAP       4
#define DIVOOM_ASTRO_NIGHT_SEC_GAP          2
#define DIVOOM_ASTRO_NIGHT_SEC_TITLE_H      26
#define DIVOOM_ASTRO_NIGHT_SEC_TITLE_Y      2    /* sectionTitleTight padding-top */
#define DIVOOM_ASTRO_NIGHT_MENU_FONT        24   /* PixelText variant=menu → FONT_PX.menu */
#define DIVOOM_ASTRO_NIGHT_ROW_INSET_L      20   /* FlowtooListCardRowV1 + rootRowGrid v1 */
#define DIVOOM_ASTRO_NIGHT_ROW_INSET_R      20
#define DIVOOM_ASTRO_NIGHT_CARD_BORDER      DIVOOM_ASTRO_STROKE
#define DIVOOM_ASTRO_NIGHT_CARD_FOCUS_BORDER 0xB8B0A8  /* --astro-tab-ink */
#define DIVOOM_ASTRO_NIGHT_ROW_LABEL_W      52   /* timeRowLabel width */
#define DIVOOM_ASTRO_NIGHT_TIME_ROW_PAD_H     20   /* timeListRow padding-left/right */
#define DIVOOM_ASTRO_NIGHT_TIME_ROW_GAP       8    /* timeListRow gap label↔pair */
#define DIVOOM_ASTRO_NIGHT_TIME_PAIR_GAP      6    /* timePairWrap gap */
#define DIVOOM_ASTRO_NIGHT_TIME_CHEV_W        12   /* editRowChevron 显示宽 */
#define DIVOOM_ASTRO_NIGHT_SEC_TITLE_PAD_X    20   /* sectionTitleTight padding-left */
#define DIVOOM_ASTRO_NIGHT_SEC_DIM          107   /* ~42% opacity */
#define DIVOOM_ASTRO_NIGHT_MODE_MAIN        0
#define DIVOOM_ASTRO_NIGHT_MODE_TIME_PICK   1
#define DIVOOM_ASTRO_NIGHT_TIME_START       0
#define DIVOOM_ASTRO_NIGHT_TIME_END         1

/* 行布局（真源 flowtooV1CardListLayout：444×72，gap 8） */
#define DIVOOM_ASTRO_SET_ROW_W      444
#define DIVOOM_ASTRO_SET_ROW_H      72
#define DIVOOM_ASTRO_SET_ROW_GAP    8
#define DIVOOM_ASTRO_SET_ROW_X      ((DIVOOM_SCREEN_WIDTH - DIVOOM_ASTRO_SET_ROW_W) / 2)
#define DIVOOM_ASTRO_SET_TOP_GAP    8
/* 设置根页 H2：DeviceSettings.module.css rootGroupTitleV1 / rootGroupBodyV1MainH2 */
#define DIVOOM_ASTRO_SET_ROOT_ROW_GAP       6
#define DIVOOM_ASTRO_SET_ROOT_GROUP_GAP     8
#define DIVOOM_ASTRO_SET_SEC_TITLE_PAD_X    20
#define DIVOOM_ASTRO_SET_SEC_TITLE_PAD_Y    8
#define DIVOOM_ASTRO_SET_SEC_TITLE_PAD_B    4
#define DIVOOM_ASTRO_SET_SEC_BODY_GAP       4
/* SEC_H 仅作参考；布局以 create_section 实测 label 高度为准 */
#define DIVOOM_ASTRO_SET_SEC_H      (DIVOOM_ASTRO_SET_SEC_TITLE_PAD_Y + 24 \
    + DIVOOM_ASTRO_SET_SEC_TITLE_PAD_B + DIVOOM_ASTRO_SET_SEC_BODY_GAP)
/* 蓝牙左右文案分栏（对照 WiFi INFO / NETWORK 行，避免长设备名叠字） */
#define DIVOOM_ASTRO_SET_BT_PAD_L       8
#define DIVOOM_ASTRO_SET_BT_PAD_R       8
#define DIVOOM_ASTRO_SET_BT_GAP         8
#define DIVOOM_ASTRO_SET_BT_VALUE_H     32
#define DIVOOM_ASTRO_SET_BT_VALUE_MIN_W 120
#define DIVOOM_ASTRO_SET_BT_LABEL_MIN_W 48

/* 滑条 pixelRail 绿轨（真源 FlowtooSlider pixelRail，与闹钟一致） */
#define DIVOOM_ASTRO_SET_FILL_TOP   0x90FF78
#define DIVOOM_ASTRO_SET_FILL_BOT   0x28B028
#define DIVOOM_ASTRO_SET_TRACK_TOP  0xA8A8A8
#define DIVOOM_ASTRO_SET_TRACK_BOT  0x585858
/* 根页快捷行 UI：48px 图标 + 滑条撑满（仅布局，不改步进逻辑） */
/* DIVOOM_ASTRO_SET_CARD_PAD_H 见 divoom_astrootoo_settings_internal.h */
#define DIVOOM_ASTRO_SET_QUICK_ICON_INSET     8
#define DIVOOM_ASTRO_SET_QUICK_ICON_SZ        48
#define DIVOOM_ASTRO_SET_QUICK_SLIDER_INSET_R 16
#define DIVOOM_ASTRO_SET_SLIDER_W (DIVOOM_ASTRO_SET_ROW_W - DIVOOM_ASTRO_SET_CARD_PAD_H * 2 \
    - DIVOOM_ASTRO_SET_QUICK_ICON_INSET - DIVOOM_ASTRO_SET_QUICK_ICON_SZ \
    - DIVOOM_ASTRO_SET_QUICK_SLIDER_INSET_R)
#define DIVOOM_ASTRO_SET_KNOB_COLOR 0x8A8A8A

#define DIVOOM_ASTRO_SET_MAX_FOCUS  24
#define DIVOOM_ASTRO_SET_PICKER_MAX 10
#define DIVOOM_ASTRO_SET_ACTION_RESET_NET    1
#define DIVOOM_ASTRO_SET_ACTION_CLEAR_STORAGE 2
#define DIVOOM_ASTRO_SET_ACTION_UPGRADE       3
#define DIVOOM_ASTRO_SET_ACTION_CONFIRM_UPDATE 4
#define DIVOOM_ASTRO_SET_ACTION_START_UPDATE   5
#define DIVOOM_ASTRO_SET_ACTION_BETA_FEATURES 6
#define DIVOOM_ASTRO_ABOUT_UPDATE_POLL_MS     800  /* 真源 refreshAboutUpdateCheck 周期：800ms */
#define DIVOOM_ASTRO_ABOUT_UPDATE_DOT_SIZE    8
#define DIVOOM_ASTRO_ABOUT_UPDATE_TRAIL_GAP   6
#define DIVOOM_ASTRO_SET_BT_MAX_ROW 28
#define DIVOOM_ASTRO_SET_CHECK_ICON "23/at_repeat_check.png"
#define DIVOOM_ASTRO_SET_BT_ICON_DISCONNECT_LINKED \
    DIVOOM_ASTROTOO_SETTINGS_UI_DIR "at_bt_disconnect_on.png"
#define DIVOOM_ASTRO_SET_BT_ICON_DISCONNECT_IDLE \
    DIVOOM_ASTROTOO_SETTINGS_UI_DIR "at_bt_disconnect_off.png"
#define DIVOOM_ASTRO_SET_BT_ICON_UNPAIR_LINKED \
    DIVOOM_ASTROTOO_SETTINGS_UI_DIR "at_bt_unpair_on.png"
#define DIVOOM_ASTRO_SET_BT_ICON_UNPAIR_IDLE \
    DIVOOM_ASTROTOO_SETTINGS_UI_DIR "at_bt_unpair_off.png"
#define DIVOOM_ASTRO_SET_BT_ICON_SZ           36
#define DIVOOM_ASTRO_SET_BT_SIG_INVALID       0xFF
#define DIVOOM_ASTRO_SET_BT_CHECKED_ROW_NONE     (-1)   // 无勾选
#define DIVOOM_ASTRO_SET_BT_CHECKED_ROW_INVALID  (-2)   // 未初始化
#define DIVOOM_ASTRO_SET_BT_SWITCH_WAIT_MAX      75     /* 400ms * 75 = 30s */
/* 固定行 0..6（含「已连接设备」分区标题），动态设备行从 7 起 */
#define DIVOOM_ASTRO_SET_BT_DEVICE_START         7

/* 设备连接码（真源 DeviceSettingsConnectionCodeScreen.module.css） */
#define DIVOOM_ASTRO_SHARE_QR_FRAME_SIZE  200
#define DIVOOM_ASTRO_SHARE_QR_SIZE        168
#define DIVOOM_ASTRO_SHARE_QR_X           140
#define DIVOOM_ASTRO_SHARE_QR_Y           34
#define DIVOOM_ASTRO_SHARE_CODE_Y         262
#define DIVOOM_ASTRO_SHARE_HINT_Y         328
#define DIVOOM_ASTRO_SHARE_GOLD           0xF8B634
#define DIVOOM_ASTRO_SHARE_CODE_COLOR     0x3D3D3D
#define DIVOOM_ASTRO_SHARE_HINT_COLOR     0x9A938B
#define DIVOOM_ASTRO_SHARE_SKIP_COLOR     0xB8B0A8
#define DIVOOM_ASTRO_SHARE_SKIP_PAD_B     12

/* 按键灯效 L3：首项熄灯，其后 8 款 mode(0~7)；picker idx 1~8 ↔ mode 0~7 */
#define DIVOOM_ASTRO_KEY_LIGHT_PICKER_OFF_IDX  0
#define DIVOOM_ASTRO_KEY_LIGHT_PICKER_CNT      ((int)DIVOOM_BTCORE_KEY_LIGHT_MODE_MAX + 1)
#define DIVOOM_ASTRO_KEY_LIGHT_ROW_CNT         (1 + DIVOOM_ASTRO_KEY_LIGHT_PICKER_CNT)

/* OOBE 首启语言步（真源 flowtooOobeLayout.ts） */
#define DIVOOM_ASTRO_OOBE_LANG_ROW_W      448
#define DIVOOM_ASTRO_OOBE_LANG_ROW_H      72
#define DIVOOM_ASTRO_OOBE_LANG_ROW_GAP    8
#define DIVOOM_ASTRO_OOBE_LANG_ROW_X        ((DIVOOM_SCREEN_WIDTH - DIVOOM_ASTRO_OOBE_LANG_ROW_W) / 2)
#define DIVOOM_ASTRO_OOBE_LANG_ICON       "53/oobe_language_icon.png"

/* 蓝牙子页行类型 */
enum
{
    DIVOOM_ASTRO_SET_BT_KIND_TOGGLE = 0,
    DIVOOM_ASTRO_SET_BT_KIND_NAME,
    DIVOOM_ASTRO_SET_BT_KIND_STATUS,
    DIVOOM_ASTRO_SET_BT_KIND_DISCONNECT,
    DIVOOM_ASTRO_SET_BT_KIND_UNPAIR,
    DIVOOM_ASTRO_SET_BT_KIND_SECTION,
    DIVOOM_ASTRO_SET_BT_KIND_EMPTY,
    DIVOOM_ASTRO_SET_BT_KIND_DEVICE,
    DIVOOM_ASTRO_SET_BT_KIND_LINK_PASSWORD,
};

static lv_obj_t *divoom_astrootoo_settings_bt_add_row(Pdivoom_astrootoo_settings_ctrl phandle,
    int idx, lv_coord_t y, uint8 kind, const char *left_key, const char *right_text,
    int focusable);
static void divoom_astrootoo_settings_bt_key_cb(lv_event_t *e);

/* 偏好分区开关 id（编码在 item.menu_id 上） */
#define DIVOOM_ASTRO_SET_TOGGLE_PIXELGRID 2

/* 行类型 */
enum
{
    DIVOOM_ASTRO_SET_KIND_SECTION = 0,
    DIVOOM_ASTRO_SET_KIND_BRIGHT,
    DIVOOM_ASTRO_SET_KIND_VOLUME,
    DIVOOM_ASTRO_SET_KIND_LINK,
    DIVOOM_ASTRO_SET_KIND_TOGGLE,
    DIVOOM_ASTRO_SET_KIND_ACTION,
};

typedef struct
{
    int kind;
    const char *key;
    int menu_id;
} divoom_astrootoo_settings_item_t;

/* 单页设置内容（真源 DeviceSettingsRootScreen buildSettingsLayout 的可见子集） */
static const divoom_astrootoo_settings_item_t g_settings_items[] =
{
    { DIVOOM_ASTRO_SET_KIND_BRIGHT,  NULL,              0 },
    { DIVOOM_ASTRO_SET_KIND_VOLUME,  NULL,              0 },
    { DIVOOM_ASTRO_SET_KIND_SECTION, "Display",         0 },
    { DIVOOM_ASTRO_SET_KIND_LINK,    "SlideEffect",     DIVOOM_MENU_CONTROL_ID_SLIDE_SETING_MENU },
    { DIVOOM_ASTRO_SET_KIND_LINK,    "NightViewMode",   DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_NIGHTVIEW },
    { DIVOOM_ASTRO_SET_KIND_SECTION, "MediaAndEffects", 0 },
    { DIVOOM_ASTRO_SET_KIND_LINK,    "Equalizer",       DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_EQUALIZER },
    { DIVOOM_ASTRO_SET_KIND_SECTION, "Connection",      0 },
    { DIVOOM_ASTRO_SET_KIND_LINK,    "Bluetooth",       DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_BLUETOOTH },
    { DIVOOM_ASTRO_SET_KIND_LINK,    "StereoPair",      DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_TWS },
    { DIVOOM_ASTRO_SET_KIND_LINK,    "Wi-Fi",           DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_WIFI },
    { DIVOOM_ASTRO_SET_KIND_SECTION, "Preferences",     0 },
    { DIVOOM_ASTRO_SET_KIND_LINK,    "ScheduledPower",  DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_POWER_OFF_TIME },
    { DIVOOM_ASTRO_SET_KIND_LINK,    "IdleShutdown",    DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_IDLE_POWER_OFF_TIME },
    { DIVOOM_ASTRO_SET_KIND_LINK,    "DateTime",        DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_DATE_TIME },
    { DIVOOM_ASTRO_SET_KIND_LINK,    "WindUnit",        DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_WIND_UNIT },
    { DIVOOM_ASTRO_SET_KIND_LINK,    "Temperature",     DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_TEMPERATURE },
    { DIVOOM_ASTRO_SET_KIND_LINK,    "Language",        DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_LANGUGE },
    { DIVOOM_ASTRO_SET_KIND_LINK,    "KeyLightEffects", DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_KEY_LIGHT_EFFECTS },
    { DIVOOM_ASTRO_SET_KIND_TOGGLE,  "PixelGrid",       DIVOOM_ASTRO_SET_TOGGLE_PIXELGRID },
    { DIVOOM_ASTRO_SET_KIND_LINK,    "PcWatchfaceMcp",  DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_MCP_TOOL },
    { DIVOOM_ASTRO_SET_KIND_SECTION, "SystemManage",                 0 },
    { DIVOOM_ASTRO_SET_KIND_LINK,    "About",                        DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_ABOUT },
    { DIVOOM_ASTRO_SET_KIND_ACTION,  "Reset all network and settings", DIVOOM_ASTRO_SET_ACTION_RESET_NET },
    { DIVOOM_ASTRO_SET_KIND_ACTION,  "Factory Restore",              DIVOOM_ASTRO_SET_ACTION_CLEAR_STORAGE },
    { DIVOOM_ASTRO_SET_KIND_LINK,    "Battery",                      0 },
};

#define DIVOOM_ASTRO_SET_ITEM_CNT (int)(sizeof(g_settings_items) / sizeof(g_settings_items[0]))

enum
{
    DIVOOM_ASTRO_SET_DT_KIND_HOUR24 = 1,
    DIVOOM_ASTRO_SET_DT_KIND_FORMAT,
#if DIVOOM_ASTRO_MANUAL_DATETIME_SET_ENABLE
    DIVOOM_ASTRO_SET_DT_KIND_AUTO,
    DIVOOM_ASTRO_SET_DT_KIND_TIME,
    DIVOOM_ASTRO_SET_DT_KIND_DATE,
#endif
    DIVOOM_ASTRO_SET_ABOUT_KIND_SPACE = 10,
    DIVOOM_ASTRO_SET_ABOUT_KIND_AUTO_UPDATE,
    DIVOOM_ASTRO_SET_ABOUT_KIND_NEW_VERSION,
    DIVOOM_ASTRO_SET_ABOUT_KIND_CHECK_FILES,
    DIVOOM_ASTRO_SET_ABOUT_KIND_BETA_FEATURES,
};

#if DIVOOM_ASTRO_MANUAL_DATETIME_SET_ENABLE
static int g_dt_auto_set = 1;
#endif

#define DIVOOM_ASTRO_DT_DATE_FIELDS     3
#define DIVOOM_ASTRO_DT_SEG_TIME_W      150
#define DIVOOM_ASTRO_DT_SEG_TIME_AMPW   82
#define DIVOOM_ASTRO_DT_SEG_TIME_UNITW  140
#define DIVOOM_ASTRO_DT_SEG_TIME_H      120
#define DIVOOM_ASTRO_DT_SEG_DATE_W      132
#define DIVOOM_ASTRO_DT_SEG_DATE_H      120

static void divoom_astrootoo_settings_root_confirm(Pdivoom_astrootoo_settings_ctrl phandle, int action_id);

static divoom_astrootoo_settings_page_t divoom_astrootoo_settings_page_from_menu_id(int menu_id)
{
    switch (menu_id)
    {
    case DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_BRIGHTNESS:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_BRIGHTNESS;
    }
    case DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_VOLUME:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_VOLUME;
    }
    case DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_NIGHTVIEW:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_NIGHTMODE;
    }
    case DIVOOM_MENU_CONTROL_ID_SLIDE_SETING_MENU:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_SLIDESHOW;
    }
    case DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_SLIDESHOW_ORDER:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_SLIDESHOW_PHOTO_ORDER;
    }
    case DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_LANGUGE:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_LANGUAGE;
    }
    case DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_BLUETOOTH:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_BLUETOOTH;
    }
    case DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_WIFI:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI;
    }
    case DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_WIFI_ADD:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_ADD;
    }
    case DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_WIFI_PASSWORD:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_PASSWORD;
    }
    case DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_EQUALIZER:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_EQUALIZER;
    }
    case DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_WIND_UNIT:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_WIND_UNIT;
    }
    case DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_KEY_LIGHT_EFFECTS:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_KEY_LIGHT_EFFECTS;
    }
    case DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_IDLE_POWER_OFF_TIME:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_IDLE_SHUTDOWN;
    }
    case DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_POWER_OFF_TIME:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_SCHEDULED_POWER;
    }
    case DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_DATE_TIME:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_TIME;
    }
    case DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_TEMPERATURE:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_TEMP_UNIT;
    }
    case DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_DATE_FORMAT:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_FORMAT;
    }
    case DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_DATE_TIME_TIME:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_TIME_TIME;
    }
    case DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_DATE_TIME_DATE:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_TIME_DATE;
    }
    case DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_BT_SET_SECRET:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_BT_PASSWORD;
    }
    case DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_MCP_TOOL:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_MCP;
    }
    case DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_ABOUT:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_ABOUT;
    }
    case DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_CONNECTION_CODE:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_CONNECTION_CODE;
    }
    default:
    {
        return DIVOOM_ASTROTOO_SETTINGS_PAGE_ROOT;
    }
    }
}

/* ====================================================================== */
/* 共用：深色顶栏 / 绿轨滑条                                                */
/* ====================================================================== */
static void divoom_astrootoo_settings_create_header(Pdivoom_astrootoo_settings_ctrl phandle, const char *title_key)
{
    divoom_astrootoo_ui_create_page_header_tab(phandle->root_obj, &phandle->ui,
        title_key, DIVOOM_ASTROTOO_SETTINGS_HEADER, phandle->menu_id);
}

/* 仅从 group 摘除本页可聚焦对象（勿 remove_all，避免误清已切到父页的 group） */
static void divoom_astrootoo_settings_group_remove_obj_safe(lv_obj_t *obj)
{
    lv_group_t *g;

    if (!obj || !lv_obj_is_valid(obj))
    {
        return;
    }
    g = lv_group_get_default();
    if (!g)
    {
        return;
    }
    lv_group_remove_obj(obj);
}

static void divoom_astrootoo_settings_group_detach_page(Pdivoom_astrootoo_settings_ctrl phandle)
{
    uint32_t i;
    uint32_t n;
    lv_obj_t *ch;
    int j;

    if (!phandle)
    {
        return;
    }

    switch (phandle->page)
    {
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_NIGHTMODE:
    {
        divoom_astrootoo_settings_group_remove_obj_safe(phandle->night_toggle_card);
        divoom_astrootoo_settings_group_remove_obj_safe(phandle->night_bright_card);
        divoom_astrootoo_settings_group_remove_obj_safe(phandle->night_start_card);
        divoom_astrootoo_settings_group_remove_obj_safe(phandle->night_end_card);
        divoom_astrootoo_settings_group_remove_obj_safe(phandle->night_key_btn);
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_ROOT:
    {
        if (!phandle->content || !lv_obj_is_valid(phandle->content))
        {
            break;
        }
        n = lv_obj_get_child_cnt(phandle->content);
        for (i = 0; i < n; i++)
        {
            ch = lv_obj_get_child(phandle->content, i);
            if (ch && lv_obj_check_type(ch, &lv_btn_class))
            {
                divoom_astrootoo_settings_group_remove_obj_safe(ch);
            }
        }
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_BRIGHTNESS:
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_VOLUME:
    {
        divoom_astrootoo_settings_group_remove_obj_safe(phandle->slider);
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_SLIDESHOW:
    {
        divoom_astrootoo_settings_slideshow_group_detach(phandle);
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_LANGUAGE:
    {
        for (j = 0; j < DIVOOM_ASTRO_SET_LANG_COUNT; j++)
        {
            divoom_astrootoo_settings_group_remove_obj_safe(phandle->lang_row[j]);
        }
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_BLUETOOTH:
    {
        for (j = 0; j < phandle->bt_row_count; j++)
        {
            divoom_astrootoo_settings_group_remove_obj_safe(phandle->bt_row[j]);
        }
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI:
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_ADD:
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_PASSWORD:
    {
        divoom_astrootoo_settings_wifi_group_detach(phandle);
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_EQUALIZER:
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_IDLE_SHUTDOWN:
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_WIND_UNIT:
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_TEMP_UNIT:
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_FORMAT:
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_SLIDESHOW_PHOTO_ORDER:
    {
        for (j = 0; j < phandle->picker_cnt; j++)
        {
            if (phandle->picker_row[j])
            {
                divoom_astrootoo_settings_group_remove_obj_safe(phandle->picker_row[j]);
            }
        }
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_KEY_LIGHT_EFFECTS:
    {
        divoom_astrootoo_settings_group_remove_obj_safe(phandle->key_light_bright_card);
        for (j = 0; j < phandle->picker_cnt; j++)
        {
            if (phandle->picker_row[j])
            {
                divoom_astrootoo_settings_group_remove_obj_safe(phandle->picker_row[j]);
            }
        }
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_TIME_TIME:
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_TIME_DATE:
    {
        divoom_astrootoo_settings_group_remove_obj_safe(phandle->dt_key_btn);
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_CONNECTION_CODE:
    {
        divoom_astrootoo_settings_group_remove_obj_safe(phandle->connection_code_key_btn);
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_TIME:
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_ABOUT:
    {
        for (j = 0; j < phandle->list_row_count; j++)
        {
            divoom_astrootoo_settings_group_remove_obj_safe(phandle->list_row[j]);
        }
        break;
    }
    default:
    {
        break;
    }
    }
}

static void divoom_astrootoo_settings_group_bind_night(Pdivoom_astrootoo_settings_ctrl phandle,
    lv_obj_t *focus_target);
static void divoom_astrootoo_settings_night_refresh(Pdivoom_astrootoo_settings_ctrl phandle);

static void divoom_astrootoo_settings_group_bind_night(Pdivoom_astrootoo_settings_ctrl phandle,
    lv_obj_t *focus_target)
{
    lv_group_t *g;
    lv_obj_t *ch;

    if (!phandle)
    {
        return;
    }
    g = lv_group_get_default();
    lv_group_remove_all_objs(g);
    if (phandle->night_screen_mode == DIVOOM_ASTRO_NIGHT_MODE_TIME_PICK)
    {
        if (phandle->night_key_btn && lv_obj_is_valid(phandle->night_key_btn))
        {
            lv_group_add_obj(g, phandle->night_key_btn);
            lv_group_focus_obj(phandle->night_key_btn);
        }
        return;
    }
    if (phandle->night_toggle_card && lv_obj_is_valid(phandle->night_toggle_card))
    {
        lv_group_add_obj(g, phandle->night_toggle_card);
    }
    if (phandle->night_bright_card && lv_obj_is_valid(phandle->night_bright_card))
    {
        lv_group_add_obj(g, phandle->night_bright_card);
    }
    if (phandle->night_start_card && lv_obj_is_valid(phandle->night_start_card))
    {
        lv_group_add_obj(g, phandle->night_start_card);
    }
    if (phandle->night_end_card && lv_obj_is_valid(phandle->night_end_card))
    {
        lv_group_add_obj(g, phandle->night_end_card);
    }
    if (focus_target && lv_obj_is_valid(focus_target))
    {
        lv_group_focus_obj(focus_target);
    }
    else if (phandle->night_toggle_card)
    {
        lv_group_focus_obj(phandle->night_toggle_card);
    }
    ch = lv_group_get_focused(g);
    if (ch)
    {
        divoom_astrootoo_ui_focus_scroll(ch);
    }
    divoom_astrootoo_settings_night_refresh(phandle);
}

static void divoom_astrootoo_settings_group_bind_content(Pdivoom_astrootoo_settings_ctrl phandle,
    lv_obj_t *focus_target)
{
    lv_group_t *g;
    uint32_t i;
    uint32_t n;
    lv_obj_t *first_btn;
    lv_obj_t *ch;

    if (!phandle || !phandle->content)
    {
        return;
    }
    g = lv_group_get_default();
    lv_group_remove_all_objs(g);
    first_btn = NULL;
    n = lv_obj_get_child_cnt(phandle->content);
    for (i = 0; i < n; i++)
    {
        ch = lv_obj_get_child(phandle->content, i);
        if (!ch || !lv_obj_check_type(ch, &lv_btn_class))
        {
            continue;
        }
        lv_group_add_obj(g, ch);
        if (!first_btn)
        {
            first_btn = ch;
        }
    }
    if (focus_target && lv_obj_is_valid(focus_target))
    {
        lv_group_focus_obj(focus_target);
    }
    else if (first_btn)
    {
        lv_group_focus_obj(first_btn);
    }
    ch = lv_group_get_focused(g);
    if (ch)
    {
        divoom_astrootoo_ui_focus_scroll_keep_section_title(ch);
    }
}

static void divoom_astrootoo_settings_group_bind_slider(Pdivoom_astrootoo_settings_ctrl phandle,
    lv_obj_t *focus_target)
{
    lv_group_t *g;

    if (!phandle || !phandle->slider)
    {
        return;
    }
    g = lv_group_get_default();
    lv_group_remove_all_objs(g);
    lv_group_add_obj(g, phandle->slider);
    if (focus_target && lv_obj_is_valid(focus_target))
    {
        lv_group_focus_obj(focus_target);
    }
    else
    {
        lv_group_focus_obj(phandle->slider);
    }
}

static void divoom_astrootoo_settings_group_bind_lang(Pdivoom_astrootoo_settings_ctrl phandle,
    lv_obj_t *focus_target)
{
    lv_group_t *g;
    int i;
    lv_obj_t *focus;

    g = lv_group_get_default();
    lv_group_remove_all_objs(g);
    for (i = 0; i < DIVOOM_ASTRO_SET_LANG_COUNT; i++)
    {
        if (phandle->lang_row[i])
        {
            lv_group_add_obj(g, phandle->lang_row[i]);
        }
    }
    focus = NULL;
    if (focus_target && lv_obj_is_valid(focus_target))
    {
        focus = focus_target;
    }
    else if (phandle->lang_focus >= 0 && phandle->lang_focus < DIVOOM_ASTRO_SET_LANG_COUNT
        && phandle->lang_row[phandle->lang_focus])
    {
        focus = phandle->lang_row[phandle->lang_focus];
    }
    else if (phandle->lang_row[0])
    {
        focus = phandle->lang_row[0];
    }
    if (focus)
    {
        lv_group_focus_obj(focus);
        divoom_astrootoo_ui_focus_scroll(focus);
    }
}

static void divoom_astrootoo_settings_group_bind_bt(Pdivoom_astrootoo_settings_ctrl phandle,
    lv_obj_t *focus_target)
{
    lv_group_t *g;
    int i;
    lv_obj_t *focus;

    g = lv_group_get_default();
    lv_group_remove_all_objs(g);
    for (i = 0; i < phandle->bt_row_count; i++)
    {
        uint8 kind;

        if (!phandle->bt_row[i])
        {
            continue;
        }
        kind = phandle->bt_row_kind[i];
        if (kind == DIVOOM_ASTRO_SET_BT_KIND_SECTION
            || kind == DIVOOM_ASTRO_SET_BT_KIND_NAME
            || kind == DIVOOM_ASTRO_SET_BT_KIND_STATUS
            || kind == DIVOOM_ASTRO_SET_BT_KIND_EMPTY)
        {
            continue;
        }
        lv_group_add_obj(g, phandle->bt_row[i]);
    }
    focus = NULL;
    if (focus_target && lv_obj_is_valid(focus_target))
    {
        focus = focus_target;
    }
    else if (phandle->bt_focus >= 0 && phandle->bt_focus < phandle->bt_row_count
        && phandle->bt_row[phandle->bt_focus])
    {
        focus = phandle->bt_row[phandle->bt_focus];
    }
    else if (phandle->bt_row[0])
    {
        focus = phandle->bt_row[0];
    }
    if (focus)
    {
        lv_group_focus_obj(focus);
        divoom_astrootoo_ui_focus_scroll(focus);
    }
}

static void divoom_astrootoo_settings_style_slider(lv_obj_t *slider)
{
    lv_obj_clear_flag(slider, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(slider, lv_color_hex(DIVOOM_ASTRO_SET_TRACK_TOP), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(slider, lv_color_hex(DIVOOM_ASTRO_SET_TRACK_BOT), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(slider, LV_GRAD_DIR_VER, LV_PART_MAIN);
    lv_obj_set_style_radius(slider, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(slider, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, lv_color_hex(DIVOOM_ASTRO_SET_FILL_TOP), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(slider, lv_color_hex(DIVOOM_ASTRO_SET_FILL_BOT), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_dir(slider, LV_GRAD_DIR_VER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(slider, 0, LV_PART_INDICATOR);
    /* pixelRail 滑块：灰底 #8A8A8A + 白边（真源 FlowtooSlider pixelRail SVG knob） */
    lv_obj_set_style_bg_color(slider, lv_color_hex(DIVOOM_ASTRO_SET_KNOB_COLOR), LV_PART_KNOB);
    lv_obj_set_style_radius(slider, 0, LV_PART_KNOB);
    lv_obj_set_style_border_color(slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
    lv_obj_set_style_border_width(slider, 1, LV_PART_KNOB);
    lv_obj_set_style_pad_hor(slider, 3, LV_PART_KNOB);
    lv_obj_set_style_pad_ver(slider, 6, LV_PART_KNOB);
    lv_obj_set_style_outline_width(slider, 0, LV_PART_KNOB);
    /* 去掉聚焦态默认蓝色外圈/边框/阴影（见 lvgl-border-outline-focus 规则） */
    lv_obj_set_style_outline_width(slider, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(slider, 0, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(slider, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(slider, 0, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(slider, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_shadow_width(slider, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(slider, 0, LV_PART_KNOB);
    divoom_menu_control_slider_clear_border_outline_shadow(slider);
}

/* ====================================================================== */
/* 单页根设置                                                              */
/* ====================================================================== */
static void divoom_astrootoo_settings_apply_brightness(int value)
{
    if (value < 0)
    {
        value = 0;
    }
    else if (value > 100)
    {
        value = 100;
    }

    divoom_main_set_sys_light((uint8)value);
    divoom_main_upload_sys_brightness((uint8)value);
}

static void divoom_astrootoo_settings_sync_sys_conf(void)
{
    divoom_main_send_sys_settings_config();
    divoom_main_save_config();
}

static void divoom_astrootoo_settings_apply_tone_volume(int value)
{
    if (value < 0)
    {
        value = 0;
    }
    else if (value > 100)
    {
        value = 100;
    }

    divoom_main_set_tone_volume(value);
    divoom_local_music_ctrl_set_tone_volume(value);
    divoom_main_send_sys_settings_config();
    divoom_main_save_config();
}

static void divoom_astrootoo_settings_apply_focus_value(Pdivoom_astrootoo_settings_ctrl phandle, lv_obj_t *card, int delta)
{
    lv_obj_t *slider;
    int v;

    if (card == phandle->bright_card)
    {
        slider = phandle->bright_slider;
    }
    else if (card == phandle->vol_card)
    {
        slider = phandle->vol_slider;
    }
    else
    {
        return;
    }

    v = (int)lv_slider_get_value(slider) + delta * 5;
    if (v < 0)
    {
        v = 0;
    }
    if (v > 100)
    {
        v = 100;
    }
    lv_slider_set_value(slider, v, LV_ANIM_OFF);
    if (card == phandle->bright_card)
    {
        divoom_astrootoo_settings_apply_brightness(v);
    }
    else
    {
        divoom_astrootoo_settings_apply_tone_volume(v);
    }
}

static void divoom_astrootoo_settings_root_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    lv_obj_t *target;
    lv_obj_t *next;
    uint8_t key_code;
    int pos;

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

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_menu_control_return_pre();
        return;
    }

    if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code))
    {
        int direct;

        direct = divoom_astrootoo_ui_key_is_up(key_code) ? DIVOOM_MENU_DIRECT_UP : DIVOOM_MENU_DIRECT_DOWN;
        next = divoom_menu_direct_ctrl_get(phandle->direct_handle, target, direct);
        if (next && next != target)
        {
            lv_group_focus_obj(next);
            divoom_astrootoo_ui_focus_scroll_keep_section_title(next);
        }
        return;
    }

    /* 亮度/音量行：左右即时调节 */
    if (target == phandle->bright_card || target == phandle->vol_card)
    {
        if (divoom_astrootoo_ui_key_is_left(key_code))
        {
            divoom_astrootoo_settings_apply_focus_value(phandle, target, -1);
            return;
        }
        if (divoom_astrootoo_ui_key_is_right(key_code))
        {
            divoom_astrootoo_settings_apply_focus_value(phandle, target, 1);
            return;
        }
    }

    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        pos = divoom_menu_direct_ctrl_get_pos(phandle->direct_handle, target);
        if (pos >= 0 && pos < phandle->focus_cnt && phandle->focus_items[pos])
        {
            const divoom_astrootoo_settings_item_t *it;

            it = phandle->focus_items[pos];
            if (it->kind == DIVOOM_ASTRO_SET_KIND_TOGGLE)
            {
                phandle->pref_pixelgrid_on = !phandle->pref_pixelgrid_on;
                divoom_main_set_show_grid_flag(phandle->pref_pixelgrid_on ? 1 : 0);
                divoom_astrootoo_settings_sync_sys_conf();
                if (phandle->pixelgrid_img)
                {
                    divoom_menu_control_lv_img_set_src(phandle->pixelgrid_img,
                        phandle->pref_pixelgrid_on ? DIVOOM_ASTROTOO_SETTINGS_SWITCH_ON : DIVOOM_ASTROTOO_SETTINGS_SWITCH_OFF);
                }
                return;
            }
            if (it->kind == DIVOOM_ASTRO_SET_KIND_ACTION)
            {
                divoom_astrootoo_settings_root_confirm(phandle, it->menu_id);
                return;
            }
            if (it->menu_id != 0)
            {
                divoom_menu_control_load_next(it->menu_id, NULL, 0, 0);
            }
        }
    }
}

static lv_coord_t divoom_astrootoo_settings_create_section(Pdivoom_astrootoo_settings_ctrl phandle,
    const char *key, lv_coord_t y)
{
    lv_obj_t *lbl;
    lv_coord_t text_h;

    lbl = lv_label_create(phandle->content);
    lv_label_set_text(lbl, divoom_menu_control_get_disp_str(key));
    lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
    lv_obj_set_width(lbl, DIVOOM_ASTRO_SET_ROW_W - DIVOOM_ASTRO_SET_SEC_TITLE_PAD_X * 2);
    lv_obj_set_style_pad_bottom(lbl, DIVOOM_ASTRO_SET_SEC_TITLE_PAD_B, 0);
    lv_obj_set_pos(lbl, DIVOOM_ASTRO_SET_ROW_X + DIVOOM_ASTRO_SET_SEC_TITLE_PAD_X,
        y + DIVOOM_ASTRO_SET_SEC_TITLE_PAD_Y);
    lv_obj_update_layout(lbl);
    text_h = (lv_coord_t)lv_obj_get_height(lbl);
    return DIVOOM_ASTRO_SET_SEC_TITLE_PAD_Y + text_h + DIVOOM_ASTRO_SET_SEC_BODY_GAP;
}

static lv_obj_t *divoom_astrootoo_settings_create_card(Pdivoom_astrootoo_settings_ctrl phandle, int focus_idx, lv_coord_t y)
{
    lv_obj_t *card;

    card = lv_btn_create(phandle->content);
    lv_obj_add_style(card, &phandle->ui.card_item_style, 0);
    lv_obj_add_style(card, &phandle->ui.card_item_focus_style, LV_STATE_FOCUSED);
    lv_obj_add_style(card, &phandle->ui.card_item_focus_style, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_size(card, DIVOOM_ASTRO_SET_ROW_W, DIVOOM_ASTRO_SET_ROW_H);
    lv_obj_set_pos(card, DIVOOM_ASTRO_SET_ROW_X, y);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    divoom_menu_direct_ctrl_update(phandle->direct_handle, card, 0, focus_idx);
    lv_obj_add_event_cb(card, divoom_astrootoo_settings_root_key_cb, LV_EVENT_KEY, phandle);
    return card;
}

static void divoom_astrootoo_settings_fill_slider_row(lv_obj_t *card, const char *icon_file,
    int initial, lv_obj_t **slider_out)
{
    lv_obj_t *icon;
    lv_obj_t *slider;

    if (!card)
    {
        return;
    }
    icon = lv_img_create(card);
    divoom_menu_control_lv_img_set_src(icon, icon_file);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, DIVOOM_ASTRO_SET_QUICK_ICON_INSET, 0);

    slider = lv_slider_create(card);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, initial, LV_ANIM_OFF);
    lv_obj_set_size(slider, DIVOOM_ASTRO_SET_SLIDER_W, 8);
    lv_obj_align(slider, LV_ALIGN_LEFT_MID,
        DIVOOM_ASTRO_SET_QUICK_ICON_INSET + DIVOOM_ASTRO_SET_QUICK_ICON_SZ, 0);
    divoom_astrootoo_settings_style_slider(slider);
    if (slider_out)
    {
        *slider_out = slider;
    }
}

static lv_obj_t *divoom_astrootoo_settings_create_slider_row(Pdivoom_astrootoo_settings_ctrl phandle,
    int focus_idx, lv_coord_t y, const char *icon_file, int initial)
{
    lv_obj_t *card;
    lv_obj_t *slider;

    card = divoom_astrootoo_settings_create_card(phandle, focus_idx, y);

    divoom_astrootoo_settings_fill_slider_row(card, icon_file, initial, &slider);

    if (icon_file == DIVOOM_ASTROTOO_SETTINGS_BRIGHT_ICO)
    {
        phandle->bright_card = card;
        phandle->bright_slider = slider;
    }
    else
    {
        phandle->vol_card = card;
        phandle->vol_slider = slider;
    }
    return card;
}

static uint8 divoom_astrootoo_settings_battery_icon_sig(void)
{
    int level;
    uint8 charge;

    level = divoom_get_cur_battery_level();
    if (level < 0)
    {
        level = 0;
    }
    if (level > 9)
    {
        level = 9;
    }
    charge = divoom_is_charge_sta() ? 1u : 0u;
    return (uint8)(((uint8)level << 1) | charge);
}

static void divoom_astrootoo_settings_fill_bat_rect(lv_layer_t *layer, const lv_area_t *base,
    lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h, uint32 color, lv_coord_t radius)
{
    lv_draw_rect_dsc_t dsc;
    lv_area_t area;

    if (!layer || !base || w <= 0 || h <= 0)
    {
        return;
    }

    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = lv_color_hex(color);
    dsc.bg_opa = LV_OPA_COVER;
    dsc.border_width = 0;
    dsc.radius = radius;
    area.x1 = base->x1 + x;
    area.y1 = base->y1 + y;
    area.x2 = area.x1 + w - 1;
    area.y2 = area.y1 + h - 1;
    lv_draw_rect(layer, &dsc, &area);
}

static void divoom_astrootoo_settings_stroke_bat_rect(lv_layer_t *layer, const lv_area_t *base,
    lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h, uint32 color,
    lv_coord_t radius, lv_coord_t border_w)
{
    lv_draw_rect_dsc_t dsc;
    lv_area_t area;

    if (!layer || !base || w <= 0 || h <= 0 || border_w <= 0)
    {
        return;
    }

    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_opa = LV_OPA_TRANSP;
    dsc.border_width = border_w;
    dsc.border_opa = LV_OPA_COVER;
    dsc.border_color = lv_color_hex(color);
    dsc.radius = radius;
    area.x1 = base->x1 + x;
    area.y1 = base->y1 + y;
    area.x2 = area.x1 + w - 1;
    area.y2 = area.y1 + h - 1;
    lv_draw_rect(layer, &dsc, &area);
}

/* toast 式圆角电池，无闪电；色块十级。绿只表示充电。 */
static void divoom_astrootoo_settings_battery_draw_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    lv_layer_t *layer;
    lv_obj_t *obj;
    lv_area_t coords;
    uint8 sig;
    int level;
    int fill_w;
    uint32 fill;

    if (lv_event_get_code(e) != LV_EVENT_DRAW_MAIN)
    {
        return;
    }

    phandle = (Pdivoom_astrootoo_settings_ctrl)lv_event_get_user_data(e);
    obj = lv_event_get_target(e);
    layer = lv_event_get_layer(e);
    if (!phandle || !obj || !layer)
    {
        return;
    }

    lv_obj_get_coords(obj, &coords);
    sig = phandle->battery_icon_sig;
    if (sig == 0xFF)
    {
        sig = divoom_astrootoo_settings_battery_icon_sig();
    }
    level = (int)(sig >> 1);
    if (level < 0)
    {
        level = 0;
    }
    if (level > DIVOOM_ASTROTOO_SETTINGS_BAT_LEVEL_MAX)
    {
        level = DIVOOM_ASTROTOO_SETTINGS_BAT_LEVEL_MAX;
    }
    fill_w = (level + 1) * DIVOOM_ASTROTOO_SETTINGS_BAT_FILL_W
        / (DIVOOM_ASTROTOO_SETTINGS_BAT_LEVEL_MAX + 1);

    if (sig & 1)
    {
        fill = DIVOOM_ASTROTOO_SETTINGS_BAT_GREEN_COLOR;
    }
    else if (level == 0)
    {
        fill = DIVOOM_ASTROTOO_SETTINGS_BAT_RED_COLOR;
    }
    else
    {
        fill = DIVOOM_ASTROTOO_SETTINGS_BAT_FRAME_COLOR;
    }

    divoom_astrootoo_settings_stroke_bat_rect(layer, &coords,
        DIVOOM_ASTROTOO_SETTINGS_BAT_BODY_X, DIVOOM_ASTROTOO_SETTINGS_BAT_BODY_Y,
        DIVOOM_ASTROTOO_SETTINGS_BAT_BODY_W, DIVOOM_ASTROTOO_SETTINGS_BAT_BODY_H,
        DIVOOM_ASTROTOO_SETTINGS_BAT_FRAME_COLOR,
        DIVOOM_ASTROTOO_SETTINGS_BAT_BODY_RADIUS, DIVOOM_ASTROTOO_SETTINGS_BAT_BODY_BORDER);
    divoom_astrootoo_settings_fill_bat_rect(layer, &coords,
        DIVOOM_ASTROTOO_SETTINGS_BAT_NIPPLE_X, DIVOOM_ASTROTOO_SETTINGS_BAT_NIPPLE_Y,
        DIVOOM_ASTROTOO_SETTINGS_BAT_NIPPLE_W, DIVOOM_ASTROTOO_SETTINGS_BAT_NIPPLE_H,
        DIVOOM_ASTROTOO_SETTINGS_BAT_FRAME_COLOR, DIVOOM_ASTROTOO_SETTINGS_BAT_NIPPLE_RAD);
    divoom_astrootoo_settings_fill_bat_rect(layer, &coords,
        DIVOOM_ASTROTOO_SETTINGS_BAT_FILL_X, DIVOOM_ASTROTOO_SETTINGS_BAT_FILL_Y,
        fill_w, DIVOOM_ASTROTOO_SETTINGS_BAT_FILL_H, fill,
        DIVOOM_ASTROTOO_SETTINGS_BAT_FILL_RADIUS);
}

static void divoom_astrootoo_settings_battery_refresh(Pdivoom_astrootoo_settings_ctrl phandle)
{
    uint8 sig;

    if (!phandle || !phandle->battery_img || !lv_obj_is_valid(phandle->battery_img))
    {
        return;
    }

    sig = divoom_astrootoo_settings_battery_icon_sig();
    if (sig == phandle->battery_icon_sig)
    {
        return;
    }
    phandle->battery_icon_sig = sig;
    lv_obj_invalidate(phandle->battery_img);
}

static int divoom_astrootoo_settings_menu_needs_brightness_sync(int menu_id)
{
    return menu_id == DIVOOM_MENU_CONTROL_ID_SYSTEM_SETING_MENU
        || menu_id == DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_BRIGHTNESS;
}

static void divoom_astrootoo_settings_sync_brightness_ui(Pdivoom_astrootoo_settings_ctrl phandle)
{
    int sys_bright;
    int cur;

    if (!phandle)
    {
        return;
    }

    sys_bright = (int)divoom_main_get_sys_light();
    if (sys_bright < 0)
    {
        sys_bright = 0;
    }
    else if (sys_bright > 100)
    {
        sys_bright = 100;
    }

    if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_ROOT
        && phandle->bright_slider && lv_obj_is_valid(phandle->bright_slider))
    {
        cur = (int)lv_slider_get_value(phandle->bright_slider);
        if (cur != sys_bright)
        {
            lv_slider_set_value(phandle->bright_slider, sys_bright, LV_ANIM_OFF);
        }
    }
    else if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_BRIGHTNESS
        && phandle->slider && lv_obj_is_valid(phandle->slider))
    {
        cur = (int)lv_slider_get_value(phandle->slider);
        if (cur != sys_bright)
        {
            lv_slider_set_value(phandle->slider, sys_bright, LV_ANIM_OFF);
            if (phandle->slider_pct_lbl && lv_obj_is_valid(phandle->slider_pct_lbl))
            {
                char buf[16];

                lv_snprintf(buf, sizeof(buf), "%d %%", sys_bright);
                lv_label_set_text(phandle->slider_pct_lbl, buf);
            }
        }
    }
}

void divoom_astrootoo_settings_notify_sys_brightness(void)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    int menu_id;

    menu_id = divoom_menu_control_get_active_menu_id();
    if (!divoom_astrootoo_settings_menu_needs_brightness_sync(menu_id))
    {
        return;
    }

    phandle = (Pdivoom_astrootoo_settings_ctrl)divoom_menu_control_get_clock_mode_menu_handle();
    if (!phandle || !phandle->root_obj || !lv_obj_is_valid(phandle->root_obj))
    {
        return;
    }

    divoom_astrootoo_settings_sync_brightness_ui(phandle);
}

static void divoom_astrootoo_settings_battery_timer_cb(lv_timer_t *t)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    static Pdivoom_astrootoo_settings_ctrl s_battery_phandle;
    static uint32_t s_battery_last_ms;

    phandle = (Pdivoom_astrootoo_settings_ctrl)lv_timer_get_user_data(t);
    if (!phandle || !phandle->root_obj || !lv_obj_is_valid(phandle->root_obj))
    {
        return;
    }

    if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_ROOT
        || phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_BRIGHTNESS)
    {
        divoom_astrootoo_settings_sync_brightness_ui(phandle);
    }

    if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_ROOT && phandle->battery_img)
    {
        uint32_t now_ms;

        now_ms = lv_tick_get();
        if (s_battery_phandle != phandle)
        {
            s_battery_phandle = phandle;
            s_battery_last_ms = now_ms;
        }
        if (now_ms - s_battery_last_ms >= 1000u)
        {
            s_battery_last_ms = now_ms;
            divoom_astrootoo_settings_battery_refresh(phandle);
        }
    }
}

static void divoom_astrootoo_settings_battery_timer_stop(Pdivoom_astrootoo_settings_ctrl phandle)
{
    if (!phandle || !phandle->battery_timer)
    {
        return;
    }
    lv_timer_del(phandle->battery_timer);
    phandle->battery_timer = NULL;
}

static void divoom_astrootoo_settings_battery_timer_start(Pdivoom_astrootoo_settings_ctrl phandle)
{
    if (!phandle || !phandle->battery_img)
    {
        return;
    }
    if (!phandle->battery_timer)
    {
        phandle->battery_timer = lv_timer_create(divoom_astrootoo_settings_battery_timer_cb, 250, phandle);
        if (phandle->battery_timer)
        {
            lv_timer_set_repeat_count(phandle->battery_timer, -1);
        }
    }
    phandle->battery_icon_sig = 0xFF;
    divoom_astrootoo_settings_battery_refresh(phandle);
}

static void divoom_astrootoo_settings_create_link_row(Pdivoom_astrootoo_settings_ctrl phandle,
    int focus_idx, lv_coord_t y, const char *key, int is_battery)
{
    lv_obj_t *card;
    lv_obj_t *lbl;
    lv_obj_t *trail;

    card = divoom_astrootoo_settings_create_card(phandle, focus_idx, y);

    lbl = lv_label_create(card);
    lv_label_set_text(lbl, divoom_menu_control_get_disp_str(key));
    lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 8, 0);
    divoom_astrootoo_ui_list_label_prepare(lbl,
        divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_SET_ROW_W,
            DIVOOM_ASTRO_SET_CARD_LABEL_INSET_L, DIVOOM_ASTRO_SET_CARD_LABEL_INSET_R,
            is_battery ? DIVOOM_ASTRO_LIST_TOGGLE_W : DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W));
    divoom_astrootoo_ui_list_row_bind_marquee(card, lbl);

    if (is_battery)
    {
        /* 电池行：代码绘制十级色块，不显示箭头 */
        trail = lv_obj_create(card);
        lv_obj_remove_style_all(trail);
        lv_obj_set_size(trail,
            DIVOOM_ASTROTOO_SETTINGS_BATTERY_W, DIVOOM_ASTROTOO_SETTINGS_BATTERY_H);
        lv_obj_set_style_bg_opa(trail, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(trail, 0, 0);
        lv_obj_set_style_pad_all(trail, 0, 0);
        lv_obj_clear_flag(trail, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(trail, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
        lv_obj_add_event(trail, divoom_astrootoo_settings_battery_draw_cb,
            LV_EVENT_DRAW_MAIN, phandle);
        phandle->battery_img = trail;
        divoom_astrootoo_settings_battery_refresh(phandle);
    }
    else
    {
        trail = lv_img_create(card);
        divoom_menu_control_lv_img_set_src(trail, DIVOOM_ASTROTOO_SETTINGS_CHEVRON);
        divoom_astrootoo_ui_apply_sublevel_chevron_size(trail);
    }
    lv_obj_align(trail, LV_ALIGN_RIGHT_MID, -8, 0);
}

/* 偏好分区开关行：左文案 + 右侧开关图标（真源 SettingsToggleRow + FlowtooToggle） */
static void divoom_astrootoo_settings_create_toggle_row(Pdivoom_astrootoo_settings_ctrl phandle,
    int focus_idx, lv_coord_t y, const char *key, int toggle_id)
{
    lv_obj_t *card;
    lv_obj_t *lbl;
    lv_obj_t *sw;
    int on;

    (void)toggle_id;

    card = divoom_astrootoo_settings_create_card(phandle, focus_idx, y);

    lbl = lv_label_create(card);
    lv_label_set_text(lbl, divoom_menu_control_get_disp_str(key));
    lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 8, 0);
    divoom_astrootoo_ui_list_label_prepare(lbl,
        divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_SET_ROW_W,
            DIVOOM_ASTRO_SET_CARD_LABEL_INSET_L, DIVOOM_ASTRO_SET_CARD_LABEL_INSET_R,
            DIVOOM_ASTRO_LIST_TOGGLE_W));
    divoom_astrootoo_ui_list_row_bind_marquee(card, lbl);

    on = phandle->pref_pixelgrid_on;

    sw = lv_img_create(card);
    divoom_menu_control_lv_img_set_src(sw,
        on ? DIVOOM_ASTROTOO_SETTINGS_SWITCH_ON : DIVOOM_ASTROTOO_SETTINGS_SWITCH_OFF);
    lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -8, 0);
    phandle->pixelgrid_img = sw;
}

static int divoom_astrootoo_settings_count_focus(void)
{
    int i;
    int n;

    n = 0;
    for (i = 0; i < DIVOOM_ASTRO_SET_ITEM_CNT; i++)
    {
        if (g_settings_items[i].kind != DIVOOM_ASTRO_SET_KIND_SECTION
            && !(g_settings_items[i].menu_id == DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_LANGUGE
                && divoom_config_set_china_config(-1) == 1))
        {
            n++;
        }
    }
    return n;
}

static void divoom_astrootoo_settings_build_root(Pdivoom_astrootoo_settings_ctrl phandle)
{
    int i;
    int fi;
    lv_coord_t y;
    Pdivoom_sys_setting_info cfg;
    int bright;
    int vol;

    cfg = divoom_main_get_system_config();
    bright = (cfg) ? cfg->brightness : 80;
    vol = (cfg) ? cfg->volume : 50;
    phandle->pref_pixelgrid_on = (cfg && cfg->show_grid_flag) ? 1 : 0;

    phandle->focus_cnt = divoom_astrootoo_settings_count_focus();
    phandle->direct_handle = divoom_menu_direct_ctrl_create(1, phandle->focus_cnt);

    divoom_astrootoo_settings_create_header(phandle, "SystemSetting");
    phandle->content = divoom_astrootoo_ui_create_scroll_content(phandle->root_obj, &phandle->ui);

    y = DIVOOM_ASTRO_SET_TOP_GAP;
    y += divoom_astrootoo_settings_create_section(phandle, "BrightnessAndVolume", y);
    fi = 0;
    for (i = 0; i < DIVOOM_ASTRO_SET_ITEM_CNT; i++)
    {
        const divoom_astrootoo_settings_item_t *it;
        int prev_was_row;

        it = &g_settings_items[i];
        if (it->menu_id == DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_LANGUGE
            && divoom_config_set_china_config(-1) == 1)
        {
            continue;
        }
        prev_was_row = (i > 0 && g_settings_items[i - 1].kind != DIVOOM_ASTRO_SET_KIND_SECTION);
        if (it->kind == DIVOOM_ASTRO_SET_KIND_SECTION)
        {
            if (prev_was_row)
            {
                y += DIVOOM_ASTRO_SET_ROOT_GROUP_GAP - DIVOOM_ASTRO_SET_ROOT_ROW_GAP;
            }
            y += divoom_astrootoo_settings_create_section(phandle, it->key, y);
            continue;
        }

        if (fi < DIVOOM_ASTRO_SET_MAX_FOCUS)
        {
            phandle->focus_items[fi] = it;
        }

        if (it->kind == DIVOOM_ASTRO_SET_KIND_BRIGHT)
        {
            divoom_astrootoo_settings_create_slider_row(phandle, fi, y, DIVOOM_ASTROTOO_SETTINGS_BRIGHT_ICO, bright);
        }
        else if (it->kind == DIVOOM_ASTRO_SET_KIND_VOLUME)
        {
            divoom_astrootoo_settings_create_slider_row(phandle, fi, y, DIVOOM_ASTROTOO_SETTINGS_VOLUME_ICO, vol);
        }
        else if (it->kind == DIVOOM_ASTRO_SET_KIND_TOGGLE)
        {
            divoom_astrootoo_settings_create_toggle_row(phandle, fi, y, it->key, it->menu_id);
        }
        else
        {
            int is_battery = (it->kind == DIVOOM_ASTRO_SET_KIND_LINK && it->menu_id == 0
                && it->key && strcmp(it->key, "Battery") == 0);
            divoom_astrootoo_settings_create_link_row(phandle, fi, y, it->key, is_battery);
        }
        fi++;
        y += DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROOT_ROW_GAP;
    }

    divoom_astrootoo_settings_group_bind_content(phandle, NULL);
    divoom_astrootoo_settings_battery_timer_start(phandle);
}

/* ====================================================================== */
/* 子页：亮度/音量独立滑条页（真源 settings__display__brightness 等）        */
/* ====================================================================== */
static void divoom_astrootoo_settings_slider_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    int value;

    phandle = (Pdivoom_astrootoo_settings_ctrl)lv_event_get_user_data(e);
    if (!phandle || !phandle->slider)
    {
        return;
    }

    value = (int)lv_slider_get_value(phandle->slider);
    if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_BRIGHTNESS)
    {
        divoom_astrootoo_settings_apply_brightness(value);
    }
    else if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_VOLUME)
    {
        divoom_astrootoo_settings_apply_tone_volume(value);
    }

    if (phandle->slider_pct_lbl)
    {
        char buf[16];

        lv_snprintf(buf, sizeof(buf), "%d %%", value);
        lv_label_set_text(phandle->slider_pct_lbl, buf);
    }
}

static void divoom_astrootoo_settings_slider_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    uint8_t key_code;
    int v;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    phandle = (Pdivoom_astrootoo_settings_ctrl)lv_event_get_user_data(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_menu_control_return_pre();
        return;
    }
    if (!phandle || !phandle->slider)
    {
        return;
    }
    v = (int)lv_slider_get_value(phandle->slider);
    if (divoom_astrootoo_ui_key_is_left(key_code))
    {
        v -= 5;
    }
    else if (divoom_astrootoo_ui_key_is_right(key_code))
    {
        v += 5;
    }
    else
    {
        return;
    }
    if (v < 0)
    {
        v = 0;
    }
    if (v > 100)
    {
        v = 100;
    }
    lv_slider_set_value(phandle->slider, v, LV_ANIM_OFF);
    lv_obj_send_event(phandle->slider, LV_EVENT_VALUE_CHANGED, NULL);
}

static void divoom_astrootoo_settings_build_slider(Pdivoom_astrootoo_settings_ctrl phandle, const char *title_key, int initial)
{
    Pdivoom_sys_setting_info cfg;

    phandle->title_key = title_key;
    cfg = divoom_main_get_system_config();
    if (cfg && phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_BRIGHTNESS)
    {
        initial = cfg->brightness;
    }
    else if (cfg && phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_VOLUME)
    {
        initial = (int)cfg->volume;
    }

    divoom_astrootoo_settings_create_header(phandle, title_key);

    /* 大号百分比（真源 settings__display__brightness：100 %） */
    phandle->slider_pct_lbl = lv_label_create(phandle->root_obj);
    lv_obj_set_style_text_color(phandle->slider_pct_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(phandle->slider_pct_lbl, divoom_menu_control_get_font_info(48, phandle->menu_id), 0);
    lv_obj_set_width(phandle->slider_pct_lbl, DIVOOM_SCREEN_WIDTH);
    lv_obj_set_style_text_align(phandle->slider_pct_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(phandle->slider_pct_lbl, 0, 88);

    phandle->slider = lv_slider_create(phandle->root_obj);
    lv_slider_set_range(phandle->slider, 0, 100);
    lv_slider_set_value(phandle->slider, initial, LV_ANIM_OFF);
    lv_obj_set_size(phandle->slider, 440, 10);
    lv_obj_set_pos(phandle->slider, (DIVOOM_SCREEN_WIDTH - 440) / 2, 152);
    divoom_astrootoo_settings_style_slider(phandle->slider);
    lv_obj_add_event_cb(phandle->slider, divoom_astrootoo_settings_slider_cb, LV_EVENT_VALUE_CHANGED, phandle);
    lv_obj_add_event_cb(phandle->slider, divoom_astrootoo_settings_slider_key_cb, LV_EVENT_KEY, phandle);
    divoom_astrootoo_settings_group_bind_slider(phandle, phandle->slider);

    /* 底部操作提示 */
    {
        lv_obj_t *hint;

        hint = lv_label_create(phandle->root_obj);
        lv_label_set_text(hint, divoom_menu_control_get_disp_str("SliderHint"));
        lv_obj_set_style_text_color(hint, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
        lv_obj_set_style_text_font(hint, divoom_menu_control_get_font_info(20, phandle->menu_id), 0);
        lv_obj_set_width(hint, DIVOOM_SCREEN_WIDTH);
        lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_pos(hint, 0, 450);
    }

    {
        char buf[16];

        lv_snprintf(buf, sizeof(buf), "%d %%", initial);
        lv_label_set_text(phandle->slider_pct_lbl, buf);
    }
}

/* ====================================================================== */
/* 子页：夜览模式（真源 DeviceSettingsNightModeScreen.tsx）                  */
/* ====================================================================== */
static lv_timer_t *g_settings_night_save_timer;

static int divoom_astrootoo_settings_night_in_schedule(Pdivoom_astrootoo_settings_ctrl phandle)
{
    time_t cur_time;
    struct tm cur_tm;
    uint16 cur_min;
    uint16 start;
    uint16 end;

    if (!phandle || !phandle->night_on)
    {
        return 0;
    }
    start = (uint16)(phandle->night_start_h * 60 + phandle->night_start_m);
    end = (uint16)(phandle->night_end_h * 60 + phandle->night_end_m);

    cur_time = time(0);
    localtime_r((time_t *)&cur_time, &cur_tm);
    cur_min = (uint16)(cur_tm.tm_hour * 60 + cur_tm.tm_min);

    if (start < end)
    {
        if (cur_min >= start && end > cur_min)
        {
            return 1;
        }
    }
    else if (cur_min >= start || cur_min < end)
    {
        return 1;
    }
    return 0;
}

static void divoom_astrootoo_settings_night_preview_apply(int bright)
{
    if (!divoom_light_app_get_screen_status(0xFF))
    {
        return;
    }
    divoom_main_set_night_brightness((uint8)bright);
}

/* 开关为开时立刻用夜览亮度。时段外记为预览，离开页面再恢复系统亮度。 */
static void divoom_astrootoo_settings_night_preview_if_on(Pdivoom_astrootoo_settings_ctrl phandle)
{
    if (!phandle || !phandle->night_on)
    {
        return;
    }
    divoom_astrootoo_settings_night_preview_apply(phandle->night_bright);
    if (!divoom_astrootoo_settings_night_in_schedule(phandle))
    {
        phandle->night_preview_active = 1;
    }
}

static void divoom_astrootoo_settings_night_preview_restore(void)
{
    if (!divoom_light_app_get_screen_status(0xFF))
    {
        return;
    }
    divoom_main_clear_night_brightness();
    divoom_main_apply_sys_light();
}

static void divoom_astrootoo_settings_night_preview_stop(Pdivoom_astrootoo_settings_ctrl phandle)
{
    if (!phandle || !phandle->night_preview_active)
    {
        return;
    }
    phandle->night_preview_active = 0;
    if (!divoom_astrootoo_settings_night_in_schedule(phandle))
    {
        divoom_astrootoo_settings_night_preview_restore();
    }
}

static void divoom_astrootoo_settings_night_push_event(void)
{
    Pdivoom_system_setting_json_night_view pnv;
    Pdivoom_system_setting_json_night_view pcopy;
    divoom_sys_msg_info model_msg;
    divoom_system_setting_json_night_view night_view;

    pnv = divoom_main_get_night_view_info();
    if (!pnv)
    {
        return;
    }
    pcopy = (Pdivoom_system_setting_json_night_view)divoom_system_malloc_out(
        sizeof(divoom_system_setting_json_night_view));
    if (!pcopy)
    {
        return;
    }
    memcpy(pcopy, pnv, sizeof(divoom_system_setting_json_night_view));

    memset(&model_msg, 0, sizeof(model_msg));
    model_msg.src_id = DIVOOM_MODEL_APP_TOOL;
    model_msg.dest_id = DIVOOM_MODEL_MAIN;
    model_msg.msg_type = 1;
    model_msg.msg_id = DIVOOM_MAIN_EVENT_SET_DEVICE_NIGHT_VIEW;
    model_msg.data = (uint8 *)pcopy;
    divoom_sys_model_crl_msg_push(&model_msg);

    night_view.on_off = pcopy->on_off;
    night_view.bright = pcopy->bright;
    night_view.start = pcopy->start;
    night_view.end = pcopy->end;
    night_view.work_status = pcopy->work_status;
    divoom_net_app_send_packet(DIVOOM_NET_COMM_SET_SYS_NIGHT_VIEW, DIVOOM_SERVER_TOPIC, &night_view);
}

static void divoom_astrootoo_settings_night_save_timer_cb(lv_timer_t *t)
{
    (void)t;
    g_settings_night_save_timer = NULL;
    divoom_astrootoo_settings_night_push_event();
}

static void divoom_astrootoo_settings_night_schedule_push(void)
{
    if (g_settings_night_save_timer)
    {
        lv_timer_reset(g_settings_night_save_timer);
        return;
    }
    g_settings_night_save_timer = lv_timer_create(
        divoom_astrootoo_settings_night_save_timer_cb, 80, NULL);
    if (g_settings_night_save_timer)
    {
        lv_timer_set_repeat_count(g_settings_night_save_timer, 1);
    }
}

static void divoom_astrootoo_settings_night_load(Pdivoom_astrootoo_settings_ctrl phandle)
{
    Pdivoom_system_setting_json_night_view nv;

    nv = divoom_main_get_night_view_info();
    phandle->night_screen_mode = DIVOOM_ASTRO_NIGHT_MODE_MAIN;
    phandle->night_dirty = 0;
    phandle->night_preview_active = 0;
    if (!nv)
    {
        phandle->night_on = 0;
        phandle->night_bright = 50;
        phandle->night_start_h = 22;
        phandle->night_start_m = 0;
        phandle->night_end_h = 7;
        phandle->night_end_m = 0;
        return;
    }
    phandle->night_on = nv->on_off ? 1 : 0;
    phandle->night_bright = nv->bright;
    phandle->night_start_h = (int)(nv->start / 60);
    phandle->night_start_m = (int)(nv->start % 60);
    phandle->night_end_h = (int)(nv->end / 60);
    phandle->night_end_m = (int)(nv->end % 60);
}

static void divoom_astrootoo_settings_night_save(Pdivoom_astrootoo_settings_ctrl phandle)
{
    Pdivoom_system_setting_json_night_view pnv;

    pnv = divoom_main_get_night_view_info();
    if (!pnv || !phandle || !phandle->night_dirty)
    {
        return;
    }
    if (phandle->night_bright_slider && lv_obj_is_valid(phandle->night_bright_slider))
    {
        phandle->night_bright = (int)lv_slider_get_value(phandle->night_bright_slider);
    }
    pnv->on_off = (uint8)phandle->night_on;
    pnv->bright = (uint8)phandle->night_bright;
    pnv->start = (uint16)(phandle->night_start_h * 60 + phandle->night_start_m);
    pnv->end = (uint16)(phandle->night_end_h * 60 + phandle->night_end_m);
    phandle->night_dirty = 0;

    /* 延后推送事件 74：msg.data 须独立堆块；且避免 clean/ESC 与表盘背光线程竞态 */
    divoom_astrootoo_settings_night_schedule_push();
}

static void divoom_astrootoo_settings_night_apply_card_idle(lv_obj_t *card)
{
    if (!card)
    {
        return;
    }
    /* .card / FlowtooListCardRowV1.wrap：bg-muted + stroke 2px + radius 12 */
    lv_obj_set_style_bg_color(card, lv_color_hex(DIVOOM_ASTRO_BG_MUTED), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(DIVOOM_ASTRO_NIGHT_CARD_BORDER), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, DIVOOM_ASTRO_RADIUS_SM, 0);
    lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUS_KEY);
}

static void divoom_astrootoo_settings_night_apply_card_focused(lv_obj_t *card, int focused)
{
    if (!card)
    {
        return;
    }
    if (focused)
    {
        /* .cardFocus / FlowtooListCardRowV1.wrapFocus */
        lv_obj_set_style_bg_color(card, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
        lv_obj_set_style_border_color(card, lv_color_hex(DIVOOM_ASTRO_NIGHT_CARD_FOCUS_BORDER), 0);
    }
    else
    {
        divoom_astrootoo_settings_night_apply_card_idle(card);
    }
    lv_obj_set_style_border_width(card, 2, 0);
}

static void divoom_astrootoo_settings_night_apply_time_row_style(lv_obj_t *card, int focused)
{
    divoom_astrootoo_settings_night_apply_card_focused(card, focused);
}

void divoom_astrootoo_settings_apply_time_pick_segment(lv_obj_t *obj, int focused)
{
    if (!obj)
    {
        return;
    }
    lv_obj_set_style_radius(obj, DIVOOM_ASTRO_TP_DIGIT_RADIUS, 0);
    lv_obj_set_style_border_width(obj, 2, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 0, 0);
    if (focused)
    {
        lv_obj_set_style_bg_color(obj, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
        lv_obj_set_style_border_color(obj, lv_color_hex(DIVOOM_ASTRO_TP_FOCUS_BORDER), 0);
    }
    else
    {
        lv_obj_set_style_bg_color(obj, lv_color_hex(DIVOOM_ASTRO_SURFACE_IDLE), 0);
        lv_obj_set_style_border_color(obj, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
    }
}

void divoom_astrootoo_settings_apply_time_pick_slot(lv_obj_t *obj, int focused)
{
    divoom_astrootoo_settings_apply_time_pick_segment(obj, focused);
    if (focused)
    {
        lv_obj_add_state(obj, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    }
    else
    {
        lv_obj_clear_state(obj, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    }
}

lv_coord_t divoom_astrootoo_settings_time_pick_stack_y(lv_coord_t area_h)
{
    lv_coord_t y;

    y = (area_h - DIVOOM_ASTRO_DTTP_STACK_H) / 2 - 4;
    if (y < 0)
    {
        y = 0;
    }
    return y;
}

static void divoom_astrootoo_settings_time_pick_create_box(int menu_id, lv_obj_t *parent,
    lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h, int font,
    lv_obj_t **box_out, lv_obj_t **lbl_out)
{
    lv_obj_t *box;
    lv_obj_t *lbl;

    box = lv_obj_create(parent);
    divoom_astrootoo_settings_apply_time_pick_slot(box, 0);
    lv_obj_set_size(box, w, h);
    lv_obj_set_pos(box, x, y);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    lbl = lv_label_create(box);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(font, menu_id), 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_label_set_text(lbl, "00");
    lv_obj_center(lbl);

    if (box_out)
    {
        *box_out = box;
    }
    if (lbl_out)
    {
        *lbl_out = lbl;
    }
}

static lv_obj_t *divoom_astrootoo_settings_time_pick_create_cap(int menu_id, lv_obj_t *parent,
    const char *key, lv_coord_t x, lv_coord_t w, lv_coord_t y)
{
    lv_obj_t *cap;

    cap = lv_label_create(parent);
    lv_label_set_text(cap, divoom_menu_control_get_disp_str(key));
    lv_obj_set_style_text_font(cap,
        divoom_menu_control_get_font_info(DIVOOM_ASTRO_DTTP_CAP_FONT, menu_id), 0);
    lv_obj_set_style_text_color(cap, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_width(cap, w);
    lv_obj_set_style_text_align(cap, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(cap, x, y);
    return cap;
}

static void divoom_astrootoo_settings_time_pick_create_bar_btn(int menu_id, lv_obj_t *parent,
    lv_coord_t x, lv_coord_t y, const char *key, lv_obj_t **out)
{
    lv_obj_t *btn;
    lv_obj_t *lbl;

    btn = lv_obj_create(parent);
    divoom_astrootoo_settings_apply_time_pick_segment(btn, 0);
    lv_obj_set_size(btn, DIVOOM_ASTRO_DTTP_BAR_HALF_W, DIVOOM_ASTRO_DTTP_BAR_H);
    lv_obj_set_pos(btn, x, y);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lbl = lv_label_create(btn);
    lv_label_set_text(lbl, divoom_menu_control_get_disp_str(key));
    lv_obj_set_style_text_font(lbl,
        divoom_menu_control_get_font_info(DIVOOM_ASTRO_TP_MENU_FONT, menu_id), 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_center(lbl);
    if (out)
    {
        *out = btn;
    }
}

void divoom_astrootoo_settings_create_time_pick_stack(lv_obj_t *parent, int menu_id,
    const lv_style_t *content_style, lv_coord_t stack_y,
    divoom_astrootoo_time_pick_widgets_t *widgets)
{
    int use_hour24;
    int i;
    lv_obj_t *tray;
    lv_obj_t *dot;
    lv_coord_t content_w;
    lv_coord_t x0;
    lv_coord_t hour_x;
    lv_coord_t colon_x;
    lv_coord_t min_x;
    lv_coord_t unit_w;
    lv_coord_t gap;
    lv_coord_t row_w;
    lv_coord_t box_y;
    lv_coord_t cap_y;
    lv_coord_t colon_y;
    lv_coord_t tray_x;
    lv_coord_t bar_x;
    lv_coord_t bar_y;

    if (!parent || !widgets || !content_style)
    {
        return;
    }

    use_hour24 = divoom_astrootoo_time_pick_use_hour24();
    memset(widgets, 0, sizeof(*widgets));
    widgets->use_hour24 = use_hour24;

    tray_x = (DIVOOM_SCREEN_WIDTH - DIVOOM_ASTRO_DTTP_TRAY_W) / 2;

    tray = lv_obj_create(parent);
    lv_obj_add_style(tray, content_style, 0);
    lv_obj_set_size(tray, DIVOOM_ASTRO_DTTP_TRAY_W, DIVOOM_ASTRO_DTTP_TRAY_H);
    lv_obj_set_pos(tray, tray_x, stack_y);
    lv_obj_set_style_bg_color(tray, lv_color_hex(DIVOOM_ASTRO_BG_MUTED), 0);
    lv_obj_set_style_bg_opa(tray, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(tray, 2, 0);
    lv_obj_set_style_border_color(tray, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
    lv_obj_set_style_radius(tray, DIVOOM_ASTRO_RADIUS_MD, 0);
    lv_obj_set_style_pad_all(tray, 0, 0);
    lv_obj_clear_flag(tray, LV_OBJ_FLAG_SCROLLABLE);
    widgets->tray = tray;

    content_w = DIVOOM_ASTRO_DTTP_TRAY_W - 2 * DIVOOM_ASTRO_DTTP_PAD_X;
    box_y = DIVOOM_ASTRO_DTTP_PAD_TOP;
    cap_y = box_y + DIVOOM_ASTRO_DTTP_BOX_H + DIVOOM_ASTRO_DTTP_CAP_GAP;
    colon_y = box_y + (DIVOOM_ASTRO_DTTP_BOX_H
        - (DIVOOM_ASTRO_DTTP_COLON_DOT * 2 + DIVOOM_ASTRO_DTTP_COLON_GAP)) / 2;

    if (!use_hour24)
    {
        unit_w = DIVOOM_ASTRO_DTTP_UNIT_W;
        gap = DIVOOM_ASTRO_DTTP_GAP_12H;
        row_w = DIVOOM_ASTRO_DTTP_AMPM_W + gap + unit_w + gap
            + DIVOOM_ASTRO_DTTP_COLON_W + gap + unit_w;
        x0 = DIVOOM_ASTRO_DTTP_PAD_X + (content_w - row_w) / 2;
        hour_x = x0 + DIVOOM_ASTRO_DTTP_AMPM_W + gap;
        colon_x = hour_x + unit_w + gap;
        min_x = colon_x + DIVOOM_ASTRO_DTTP_COLON_W + gap;

        divoom_astrootoo_settings_time_pick_create_box(menu_id, tray, x0, box_y,
            DIVOOM_ASTRO_DTTP_AMPM_W, DIVOOM_ASTRO_DTTP_BOX_H, DIVOOM_ASTRO_DTTP_AMPM_FONT,
            &widgets->ampm_box, &widgets->ampm_lbl);
    }
    else
    {
        unit_w = DIVOOM_ASTRO_DTTP_UNIT24_W;
        gap = DIVOOM_ASTRO_DTTP_GAP_24H;
        row_w = unit_w + gap + DIVOOM_ASTRO_DTTP_COLON_W + gap + unit_w;
        x0 = DIVOOM_ASTRO_DTTP_PAD_X + (content_w - row_w) / 2;
        hour_x = x0;
        colon_x = hour_x + unit_w + gap;
        min_x = colon_x + DIVOOM_ASTRO_DTTP_COLON_W + gap;
    }

    divoom_astrootoo_settings_time_pick_create_box(menu_id, tray, hour_x, box_y,
        unit_w, DIVOOM_ASTRO_DTTP_BOX_H, DIVOOM_ASTRO_DTTP_DIGIT_FONT,
        &widgets->h_box, &widgets->h_lbl);
    widgets->h_cap = divoom_astrootoo_settings_time_pick_create_cap(menu_id, tray,
        "TimeFieldHour", hour_x, unit_w, cap_y);

    for (i = 0; i < 2; i++)
    {
        dot = lv_obj_create(tray);
        lv_obj_set_size(dot, DIVOOM_ASTRO_DTTP_COLON_DOT, DIVOOM_ASTRO_DTTP_COLON_DOT);
        lv_obj_set_pos(dot, colon_x,
            colon_y + i * (DIVOOM_ASTRO_DTTP_COLON_DOT + DIVOOM_ASTRO_DTTP_COLON_GAP));
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, lv_color_hex(DIVOOM_ASTRO_INK), 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_set_style_pad_all(dot, 0, 0);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
    }

    divoom_astrootoo_settings_time_pick_create_box(menu_id, tray, min_x, box_y,
        unit_w, DIVOOM_ASTRO_DTTP_BOX_H, DIVOOM_ASTRO_DTTP_DIGIT_FONT,
        &widgets->m_box, &widgets->m_lbl);
    widgets->m_cap = divoom_astrootoo_settings_time_pick_create_cap(menu_id, tray,
        "TimeFieldMinute", min_x, unit_w, cap_y);

    bar_x = (DIVOOM_SCREEN_WIDTH - DIVOOM_ASTRO_DTTP_BAR_W) / 2;
    bar_y = stack_y + DIVOOM_ASTRO_DTTP_TRAY_H + DIVOOM_ASTRO_DTTP_STACK_GAP;
    divoom_astrootoo_settings_time_pick_create_bar_btn(menu_id, parent, bar_x, bar_y,
        "Cancel", &widgets->cancel);
    divoom_astrootoo_settings_time_pick_create_bar_btn(menu_id, parent,
        bar_x + DIVOOM_ASTRO_DTTP_BAR_HALF_W + DIVOOM_ASTRO_DTTP_BAR_GAP, bar_y,
        "Confirm", &widgets->confirm);
}

static void divoom_astrootoo_settings_night_format_time(char *buf, int len, int hour, int minute)
{
    char tbuf[16];
    const char *ap;

    if (!buf || len <= 0)
    {
        return;
    }
    /* 夜览列表行：12h 为「时刻 + 上午/下午」（真源 FlowtooClockTimeLabel） */
    divoom_astrootoo_time_pick_format_clock_label(tbuf, (int)sizeof(tbuf), hour, minute);
    if (!divoom_astrootoo_time_pick_use_hour24())
    {
        ap = divoom_menu_control_get_disp_str(
            divoom_astrootoo_time_pick_is_pm(hour) ? "PM" : "AM");
        if (ap && ap[0] != '\0')
        {
            snprintf(buf, (size_t)len, "%s %s", tbuf, ap);
            return;
        }
    }
    snprintf(buf, (size_t)len, "%s", tbuf);
}

static void divoom_astrootoo_settings_night_set_header(Pdivoom_astrootoo_settings_ctrl phandle, const char *key)
{
    if (phandle && phandle->night_header_lbl)
    {
        lv_label_set_text(phandle->night_header_lbl, divoom_menu_control_get_disp_str(key));
    }
}

static void divoom_astrootoo_settings_night_show_mode(Pdivoom_astrootoo_settings_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }
    if (phandle->night_screen_mode == DIVOOM_ASTRO_NIGHT_MODE_TIME_PICK)
    {
        if (phandle->content)
        {
            lv_obj_add_flag(phandle->content, LV_OBJ_FLAG_HIDDEN);
        }
        if (phandle->night_time_cont)
        {
            lv_obj_clear_flag(phandle->night_time_cont, LV_OBJ_FLAG_HIDDEN);
        }
        divoom_astrootoo_settings_night_set_header(phandle, "Time");
    }
    else
    {
        if (phandle->content)
        {
            lv_obj_clear_flag(phandle->content, LV_OBJ_FLAG_HIDDEN);
        }
        if (phandle->night_time_cont)
        {
            lv_obj_add_flag(phandle->night_time_cont, LV_OBJ_FLAG_HIDDEN);
        }
        divoom_astrootoo_settings_night_set_header(phandle, "NightViewMode");
    }
}

static void divoom_astrootoo_settings_night_refresh(Pdivoom_astrootoo_settings_ctrl phandle);

static void divoom_astrootoo_settings_night_refresh_time_pick(Pdivoom_astrootoo_settings_ctrl phandle)
{
    divoom_astrootoo_time_pick_widgets_t widgets;

    if (!phandle)
    {
        return;
    }
    widgets.ampm_box = phandle->night_time_ampm_box;
    widgets.ampm_lbl = phandle->night_time_ampm_lbl;
    widgets.h_box = phandle->night_time_h_box;
    widgets.m_box = phandle->night_time_m_box;
    widgets.h_lbl = phandle->night_time_h_lbl;
    widgets.m_lbl = phandle->night_time_m_lbl;
    widgets.h_cap = phandle->night_time_h_cap;
    widgets.m_cap = phandle->night_time_m_cap;
    widgets.cancel = phandle->night_time_cancel;
    widgets.confirm = phandle->night_time_confirm;
    widgets.use_hour24 = divoom_astrootoo_time_pick_use_hour24();
    divoom_astrootoo_time_pick_refresh_widgets(&widgets, phandle->night_pick_field,
        phandle->night_pick_hour, phandle->night_pick_min, widgets.use_hour24);
}

static void divoom_astrootoo_settings_night_exit_time_pick(Pdivoom_astrootoo_settings_ctrl phandle, int apply)
{
    lv_obj_t *restore;

    if (!phandle)
    {
        return;
    }
    if (apply)
    {
        if (phandle->night_time_target == DIVOOM_ASTRO_NIGHT_TIME_START)
        {
            phandle->night_start_h = phandle->night_pick_hour;
            phandle->night_start_m = phandle->night_pick_min;
        }
        else
        {
            phandle->night_end_h = phandle->night_pick_hour;
            phandle->night_end_m = phandle->night_pick_min;
        }
        phandle->night_dirty = 1;
    }
    phandle->night_screen_mode = DIVOOM_ASTRO_NIGHT_MODE_MAIN;
    divoom_astrootoo_settings_night_show_mode(phandle);
    divoom_astrootoo_settings_night_refresh(phandle);
    restore = (phandle->night_time_target == DIVOOM_ASTRO_NIGHT_TIME_START)
        ? phandle->night_start_card : phandle->night_end_card;
    divoom_astrootoo_settings_group_bind_night(phandle, restore);
}

static void divoom_astrootoo_settings_night_open_time_pick(Pdivoom_astrootoo_settings_ctrl phandle, int target)
{
    if (!phandle || !phandle->night_on)
    {
        return;
    }
    phandle->night_time_target = target;
    if (target == DIVOOM_ASTRO_NIGHT_TIME_START)
    {
        phandle->night_pick_base_h = phandle->night_start_h;
        phandle->night_pick_base_m = phandle->night_start_m;
        phandle->night_pick_hour = phandle->night_start_h;
        phandle->night_pick_min = phandle->night_start_m;
    }
    else
    {
        phandle->night_pick_base_h = phandle->night_end_h;
        phandle->night_pick_base_m = phandle->night_end_m;
        phandle->night_pick_hour = phandle->night_end_h;
        phandle->night_pick_min = phandle->night_end_m;
    }
    phandle->night_pick_field = divoom_astrootoo_time_pick_initial_field(
        divoom_astrootoo_time_pick_use_hour24());
    phandle->night_screen_mode = DIVOOM_ASTRO_NIGHT_MODE_TIME_PICK;
    divoom_astrootoo_settings_night_show_mode(phandle);
    divoom_astrootoo_settings_night_refresh_time_pick(phandle);
    divoom_astrootoo_settings_group_bind_night(phandle, NULL);
}

static void divoom_astrootoo_settings_night_refresh(Pdivoom_astrootoo_settings_ctrl phandle)
{
    char buf[32];
    lv_opa_t sec_opa;
    lv_obj_t *focused;
    int start_outline;
    int end_outline;

    sec_opa = phandle->night_on ? LV_OPA_COVER : DIVOOM_ASTRO_NIGHT_SEC_DIM;
    if (phandle->night_toggle_img)
    {
        divoom_menu_control_lv_img_set_src(phandle->night_toggle_img,
            phandle->night_on ? DIVOOM_ASTROTOO_SETTINGS_SWITCH_ON : DIVOOM_ASTROTOO_SETTINGS_SWITCH_OFF);
    }
    if (phandle->night_sec_bright)
    {
        lv_obj_set_style_opa(phandle->night_sec_bright, sec_opa, 0);
    }
    if (phandle->night_sec_period)
    {
        lv_obj_set_style_opa(phandle->night_sec_period, sec_opa, 0);
    }
    if (phandle->night_bright_slider)
    {
        lv_slider_set_value(phandle->night_bright_slider, phandle->night_bright, LV_ANIM_OFF);
    }
    divoom_astrootoo_settings_night_format_time(buf, (int)sizeof(buf),
        phandle->night_start_h, phandle->night_start_m);
    if (phandle->night_start_val_lbl)
    {
        lv_label_set_text(phandle->night_start_val_lbl, buf);
        divoom_astrootoo_ui_list_label_refresh_marquee(phandle->night_start_card,
            phandle->night_start_val_lbl);
    }
    divoom_astrootoo_settings_night_format_time(buf, (int)sizeof(buf),
        phandle->night_end_h, phandle->night_end_m);
    if (phandle->night_end_val_lbl)
    {
        lv_label_set_text(phandle->night_end_val_lbl, buf);
        divoom_astrootoo_ui_list_label_refresh_marquee(phandle->night_end_card,
            phandle->night_end_val_lbl);
    }

    focused = lv_group_get_focused(lv_group_get_default());
    start_outline = (phandle->night_on && focused == phandle->night_start_card
        && phandle->night_screen_mode == DIVOOM_ASTRO_NIGHT_MODE_MAIN);
    end_outline = (phandle->night_on && focused == phandle->night_end_card
        && phandle->night_screen_mode == DIVOOM_ASTRO_NIGHT_MODE_MAIN);
    divoom_astrootoo_settings_night_apply_card_focused(phandle->night_toggle_card,
        focused == phandle->night_toggle_card && phandle->night_screen_mode == DIVOOM_ASTRO_NIGHT_MODE_MAIN);
    divoom_astrootoo_settings_night_apply_card_focused(phandle->night_bright_card,
        phandle->night_on && focused == phandle->night_bright_card
        && phandle->night_screen_mode == DIVOOM_ASTRO_NIGHT_MODE_MAIN);
    divoom_astrootoo_settings_night_apply_time_row_style(phandle->night_start_card, start_outline);
    divoom_astrootoo_settings_night_apply_time_row_style(phandle->night_end_card, end_outline);
}

static void divoom_astrootoo_settings_night_key_cb(lv_event_t *e);

static lv_obj_t *divoom_astrootoo_settings_night_card_on(lv_obj_t *parent,
    Pdivoom_astrootoo_settings_ctrl phandle, int focus_idx, lv_coord_t x, lv_coord_t y,
    lv_coord_t h, int use_orange_focus)
{
    lv_obj_t *card;

    (void)use_orange_focus;
    card = lv_btn_create(parent);
    divoom_astrootoo_settings_night_apply_card_idle(card);
    lv_obj_set_size(card, DIVOOM_ASTRO_SET_ROW_W, h);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    divoom_menu_direct_ctrl_update(phandle->direct_handle, card, 0, focus_idx);
    lv_obj_add_event_cb(card, divoom_astrootoo_settings_night_key_cb, LV_EVENT_KEY, phandle);
    return card;
}

static void divoom_astrootoo_settings_night_apply_time_row_pad(lv_obj_t *card)
{
    if (!card)
    {
        return;
    }
    /* timeListRow: padding 0 20px（与 brightnessCard 的 4px 12px 不同） */
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_set_style_pad_left(card, DIVOOM_ASTRO_NIGHT_TIME_ROW_PAD_H, 0);
    lv_obj_set_style_pad_right(card, DIVOOM_ASTRO_NIGHT_TIME_ROW_PAD_H, 0);
}

static void divoom_astrootoo_settings_night_section_title(Pdivoom_astrootoo_settings_ctrl phandle,
    lv_obj_t *parent, const char *key, lv_coord_t y)
{
    lv_obj_t *lbl;

    lbl = lv_label_create(parent);
    lv_label_set_text(lbl, divoom_menu_control_get_disp_str(key));
    lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(DIVOOM_ASTRO_NIGHT_MENU_FONT, phandle->menu_id), 0);
    lv_obj_set_pos(lbl, DIVOOM_ASTRO_NIGHT_SEC_TITLE_PAD_X, y);
}

static void divoom_astrootoo_settings_night_apply_toggle_row_pad(lv_obj_t *card)
{
    if (!card)
    {
        return;
    }
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_set_style_pad_left(card, DIVOOM_ASTRO_NIGHT_ROW_INSET_L, 0);
    lv_obj_set_style_pad_right(card, DIVOOM_ASTRO_NIGHT_ROW_INSET_R, 0);
}

static void divoom_astrootoo_settings_night_apply_brightness_row_pad(lv_obj_t *card)
{
    if (!card)
    {
        return;
    }
    /* brightnessCard: padding 4px 12px */
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_set_style_pad_top(card, 4, 0);
    lv_obj_set_style_pad_bottom(card, 4, 0);
    lv_obj_set_style_pad_left(card, 12, 0);
    lv_obj_set_style_pad_right(card, 12, 0);
}

static void divoom_astrootoo_settings_night_bind_time_row(lv_obj_t *card,
    lv_obj_t *row_lbl, lv_obj_t *val_lbl, lv_obj_t *chev, lv_obj_t *wrap)
{
    lv_coord_t inner_w;
    lv_coord_t key_w;
    lv_coord_t pair_w;
    lv_coord_t val_w;

    if (!card || !row_lbl || !val_lbl || !chev || !wrap)
    {
        return;
    }
    inner_w = DIVOOM_ASTRO_SET_ROW_W - DIVOOM_ASTRO_NIGHT_TIME_ROW_PAD_H * 2;
    lv_obj_update_layout(row_lbl);
    key_w = lv_obj_get_width(row_lbl);
    pair_w = inner_w - key_w - DIVOOM_ASTRO_NIGHT_TIME_ROW_GAP;
    if (pair_w < 72)
    {
        pair_w = 72;
    }
    lv_obj_set_size(wrap, pair_w, DIVOOM_ASTRO_SET_ROW_H);
    lv_obj_align(wrap, LV_ALIGN_RIGHT_MID, 0, 0);

    val_w = pair_w - DIVOOM_ASTRO_NIGHT_TIME_CHEV_W - DIVOOM_ASTRO_NIGHT_TIME_PAIR_GAP;
    if (val_w < 48)
    {
        val_w = 48;
    }
    divoom_astrootoo_ui_list_label_prepare(val_lbl, val_w);
    lv_obj_set_style_text_align(val_lbl, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align_to(val_lbl, chev, LV_ALIGN_OUT_LEFT_MID, -DIVOOM_ASTRO_NIGHT_TIME_PAIR_GAP, 0);
    divoom_astrootoo_ui_list_row_bind_marquee(card, val_lbl);
}

static void divoom_astrootoo_settings_bind_kv_row(lv_obj_t *card, lv_obj_t *key_lbl,
    lv_obj_t *val_lbl, int has_chevron)
{
    lv_coord_t trail_w;

    if (!card || !key_lbl || !val_lbl)
    {
        return;
    }
    trail_w = has_chevron ? DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W : 0;
    divoom_astrootoo_ui_list_kv_bind_row(key_lbl, val_lbl,
        DIVOOM_ASTRO_SET_ROW_W, DIVOOM_ASTRO_SET_CARD_LABEL_INSET_L, DIVOOM_ASTRO_SET_CARD_LABEL_INSET_R,
        trail_w,
        has_chevron ? -(DIVOOM_ASTRO_SET_CARD_LABEL_INSET_R + trail_w + DIVOOM_ASTRO_LIST_LABEL_GAP)
            : -DIVOOM_ASTRO_SET_CARD_LABEL_INSET_R,
        80);
    divoom_astrootoo_ui_list_row_bind_marquee(card, val_lbl);
}

static void divoom_astrootoo_settings_night_fill_time_row(Pdivoom_astrootoo_settings_ctrl phandle,
    lv_obj_t *card, const char *row_key, lv_obj_t **row_lbl,
    lv_obj_t **val_lbl, lv_obj_t **chev)
{
    lv_obj_t *wrap;

    divoom_astrootoo_settings_night_apply_time_row_pad(card);

    *row_lbl = lv_label_create(card);
    lv_label_set_text(*row_lbl, divoom_menu_control_get_disp_str(row_key));
    lv_obj_set_style_text_color(*row_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(*row_lbl, divoom_menu_control_get_font_info(DIVOOM_ASTRO_NIGHT_MENU_FONT, phandle->menu_id), 0);
    lv_obj_align(*row_lbl, LV_ALIGN_LEFT_MID, 0, 0);

    wrap = lv_obj_create(card);
    lv_obj_set_style_bg_opa(wrap, LV_OPA_TRANSP, 0);
    lv_obj_add_style(wrap, &phandle->ui.content_style, 0);
    lv_obj_clear_flag(wrap, LV_OBJ_FLAG_SCROLLABLE);

    *chev = lv_img_create(wrap);
    divoom_menu_control_lv_img_set_src(*chev, DIVOOM_ASTROTOO_SETTINGS_CHEVRON);
    divoom_astrootoo_ui_apply_sublevel_chevron_size(*chev);
    lv_obj_align(*chev, LV_ALIGN_RIGHT_MID, 0, 0);

    *val_lbl = lv_label_create(wrap);
    lv_label_set_text(*val_lbl, "--:--");
    lv_obj_set_style_text_color(*val_lbl, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(*val_lbl, divoom_menu_control_get_font_info(DIVOOM_ASTRO_NIGHT_MENU_FONT, phandle->menu_id), 0);

    divoom_astrootoo_settings_night_bind_time_row(card, *row_lbl, *val_lbl, *chev, wrap);
    divoom_astrootoo_settings_night_apply_time_row_style(card, 0);
}

static void divoom_astrootoo_settings_night_create_time_pick(Pdivoom_astrootoo_settings_ctrl phandle)
{
    divoom_astrootoo_time_pick_widgets_t widgets;
    lv_coord_t stack_y;

    phandle->night_time_cont = lv_obj_create(phandle->root_obj);
    lv_obj_add_flag(phandle->night_time_cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_style(phandle->night_time_cont, &phandle->ui.content_style, 0);
    lv_obj_set_size(phandle->night_time_cont, DIVOOM_SCREEN_WIDTH, DIVOOM_ASTRO_CONTENT_H);
    lv_obj_set_pos(phandle->night_time_cont, 0, DIVOOM_ASTRO_CONTENT_Y);
    lv_obj_set_style_bg_opa(phandle->night_time_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(phandle->night_time_cont, 0, 0);
    lv_obj_clear_flag(phandle->night_time_cont, LV_OBJ_FLAG_SCROLLABLE);

    stack_y = divoom_astrootoo_settings_time_pick_stack_y(DIVOOM_ASTRO_CONTENT_H);
    divoom_astrootoo_settings_create_time_pick_stack(phandle->night_time_cont, phandle->menu_id,
        &phandle->ui.content_style, stack_y, &widgets);
    phandle->night_time_ampm_box = widgets.ampm_box;
    phandle->night_time_ampm_lbl = widgets.ampm_lbl;
    phandle->night_time_h_box = widgets.h_box;
    phandle->night_time_m_box = widgets.m_box;
    phandle->night_time_h_lbl = widgets.h_lbl;
    phandle->night_time_m_lbl = widgets.m_lbl;
    phandle->night_time_h_cap = widgets.h_cap;
    phandle->night_time_m_cap = widgets.m_cap;
    phandle->night_time_cancel = widgets.cancel;
    phandle->night_time_confirm = widgets.confirm;
}

static void divoom_astrootoo_settings_night_time_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    uint8_t key_code;
    divoom_astrootoo_time_pick_state_t st;
    divoom_astrootoo_time_pick_key_action_t action;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    phandle = (Pdivoom_astrootoo_settings_ctrl)lv_event_get_user_data(e);
    if (!phandle || phandle->night_screen_mode != DIVOOM_ASTRO_NIGHT_MODE_TIME_PICK)
    {
        return;
    }
    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    st.use_hour24 = divoom_astrootoo_time_pick_use_hour24();
    st.field = phandle->night_pick_field;
    st.hour24 = phandle->night_pick_hour;
    st.minute = phandle->night_pick_min;
    action = divoom_astrootoo_time_pick_handle_key(&st, key_code, 0);
    phandle->night_pick_field = st.field;
    phandle->night_pick_hour = st.hour24;
    phandle->night_pick_min = st.minute;
    if (action == DIVOOM_ASTRO_TP_KEY_CANCEL)
    {
        divoom_astrootoo_settings_night_exit_time_pick(phandle, 0);
        return;
    }
    if (action == DIVOOM_ASTRO_TP_KEY_CONFIRM)
    {
        divoom_astrootoo_settings_night_exit_time_pick(phandle, 1);
        return;
    }
    divoom_astrootoo_settings_night_refresh_time_pick(phandle);
}

static void divoom_astrootoo_settings_night_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    lv_obj_t *target;
    lv_obj_t *next;
    uint8_t key_code;
    int delta;
    int direct;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    phandle = (Pdivoom_astrootoo_settings_ctrl)lv_event_get_user_data(e);
    if (!phandle || phandle->night_screen_mode != DIVOOM_ASTRO_NIGHT_MODE_MAIN)
    {
        return;
    }
    target = lv_event_get_target(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_menu_control_return_pre();
        return;
    }

    if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code))
    {
        direct = divoom_astrootoo_ui_key_is_up(key_code) ? DIVOOM_MENU_DIRECT_UP : DIVOOM_MENU_DIRECT_DOWN;
        next = divoom_menu_direct_ctrl_get(phandle->direct_handle, target, direct);
        if (!phandle->night_on && next && next != phandle->night_toggle_card)
        {
            next = NULL;
        }
        if (next && next != target)
        {
            lv_group_focus_obj(next);
            divoom_astrootoo_ui_focus_scroll(next);
            divoom_astrootoo_settings_night_refresh(phandle);
        }
        return;
    }

    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        if (target == phandle->night_toggle_card)
        {
            phandle->night_on = !phandle->night_on;
            phandle->night_dirty = 1;
            if (!phandle->night_on)
            {
                divoom_astrootoo_settings_night_preview_stop(phandle);
                lv_group_focus_obj(phandle->night_toggle_card);
            }
            else
            {
                divoom_astrootoo_settings_night_preview_if_on(phandle);
            }
            divoom_astrootoo_settings_night_refresh(phandle);
            return;
        }
        if (phandle->night_on && target == phandle->night_start_card)
        {
            divoom_astrootoo_settings_night_open_time_pick(phandle, DIVOOM_ASTRO_NIGHT_TIME_START);
            return;
        }
        if (phandle->night_on && target == phandle->night_end_card)
        {
            divoom_astrootoo_settings_night_open_time_pick(phandle, DIVOOM_ASTRO_NIGHT_TIME_END);
            return;
        }
        return;
    }

    delta = 0;
    if (divoom_astrootoo_ui_key_is_left(key_code))
    {
        delta = -1;
    }
    else if (divoom_astrootoo_ui_key_is_right(key_code))
    {
        delta = 1;
    }
    if (delta == 0)
    {
        return;
    }

    if (phandle->night_on && target == phandle->night_bright_card)
    {
        int v;

        v = (int)lv_slider_get_value(phandle->night_bright_slider) + delta * 5;
        if (v < 0)
        {
            v = 0;
        }
        if (v > 100)
        {
            v = 100;
        }
        phandle->night_bright = v;
        phandle->night_dirty = 1;
        lv_slider_set_value(phandle->night_bright_slider, v, LV_ANIM_OFF);
        divoom_astrootoo_settings_night_preview_apply(v);
        if (!divoom_astrootoo_settings_night_in_schedule(phandle))
        {
            phandle->night_preview_active = 1;
        }
    }
}

static void divoom_astrootoo_settings_build_nightmode(Pdivoom_astrootoo_settings_ctrl phandle)
{
    lv_obj_t *lbl;
    lv_coord_t y;
    lv_coord_t sy;

    divoom_astrootoo_settings_night_load(phandle);
    divoom_astrootoo_settings_night_preview_if_on(phandle);
    phandle->direct_handle = divoom_menu_direct_ctrl_create(1, 4);
    phandle->night_header = divoom_astrootoo_ui_create_page_header_tab(phandle->root_obj, &phandle->ui,
        "NightViewMode", DIVOOM_ASTROTOO_SETTINGS_HEADER, phandle->menu_id);
    phandle->night_header_lbl = lv_obj_get_child(phandle->night_header, -1);
    phandle->content = divoom_astrootoo_ui_create_scroll_content(phandle->root_obj, &phandle->ui);

    y = DIVOOM_ASTRO_NIGHT_SCROLL_GAP;
    phandle->night_toggle_card = divoom_astrootoo_settings_night_card_on(phandle->content, phandle,
        0, DIVOOM_ASTRO_SET_ROW_X, y, DIVOOM_ASTRO_SET_ROW_H, 1);
    divoom_astrootoo_settings_night_apply_toggle_row_pad(phandle->night_toggle_card);
    lbl = lv_label_create(phandle->night_toggle_card);
    lv_label_set_text(lbl, divoom_menu_control_get_disp_str("NightViewMode"));
    lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(DIVOOM_ASTRO_NIGHT_MENU_FONT, phandle->menu_id), 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 0, 0);
    divoom_astrootoo_ui_list_label_prepare(lbl,
        divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_SET_ROW_W,
            DIVOOM_ASTRO_NIGHT_ROW_INSET_L, DIVOOM_ASTRO_NIGHT_ROW_INSET_R,
            DIVOOM_ASTRO_LIST_TOGGLE_W));
    divoom_astrootoo_ui_list_row_bind_marquee(phandle->night_toggle_card, lbl);
    phandle->night_toggle_img = lv_img_create(phandle->night_toggle_card);
    lv_obj_align(phandle->night_toggle_img, LV_ALIGN_RIGHT_MID, 0, 0);
    y += DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_NIGHT_SCROLL_GAP;

    phandle->night_sec_bright = lv_obj_create(phandle->content);
    lv_obj_add_style(phandle->night_sec_bright, &phandle->ui.content_style, 0);
    lv_obj_set_size(phandle->night_sec_bright, DIVOOM_ASTRO_SET_ROW_W,
        DIVOOM_ASTRO_NIGHT_SEC_TITLE_Y + DIVOOM_ASTRO_NIGHT_SEC_TITLE_H
        + DIVOOM_ASTRO_NIGHT_SEC_GAP + DIVOOM_ASTRO_SET_ROW_H + 10);
    lv_obj_set_pos(phandle->night_sec_bright, DIVOOM_ASTRO_SET_ROW_X, y);
    lv_obj_clear_flag(phandle->night_sec_bright, LV_OBJ_FLAG_SCROLLABLE);
    divoom_astrootoo_settings_night_section_title(phandle, phandle->night_sec_bright, "Brightness",
        DIVOOM_ASTRO_NIGHT_SEC_TITLE_Y);
    sy = DIVOOM_ASTRO_NIGHT_SEC_TITLE_Y + DIVOOM_ASTRO_NIGHT_SEC_TITLE_H + DIVOOM_ASTRO_NIGHT_SEC_GAP + 6;
    phandle->night_bright_card = divoom_astrootoo_settings_night_card_on(phandle->night_sec_bright,
        phandle, 1, 0, sy, DIVOOM_ASTRO_SET_ROW_H, 1);
    divoom_astrootoo_settings_night_apply_brightness_row_pad(phandle->night_bright_card);
    divoom_astrootoo_settings_fill_slider_row(phandle->night_bright_card,
        DIVOOM_ASTROTOO_SETTINGS_BRIGHT_ICO, phandle->night_bright, &phandle->night_bright_slider);
    y += DIVOOM_ASTRO_NIGHT_SEC_TITLE_Y + DIVOOM_ASTRO_NIGHT_SEC_TITLE_H + DIVOOM_ASTRO_NIGHT_SEC_GAP
        + DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_NIGHT_SCROLL_GAP + 12;

    phandle->night_sec_period = lv_obj_create(phandle->content);
    lv_obj_add_style(phandle->night_sec_period, &phandle->ui.content_style, 0);
    lv_obj_set_size(phandle->night_sec_period, DIVOOM_ASTRO_SET_ROW_W,
        DIVOOM_ASTRO_NIGHT_SEC_TITLE_Y + DIVOOM_ASTRO_NIGHT_SEC_TITLE_H + DIVOOM_ASTRO_NIGHT_SEC_GAP
        + DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_NIGHT_SCROLL_GAP + DIVOOM_ASTRO_SET_ROW_H + 12);
    lv_obj_set_pos(phandle->night_sec_period, DIVOOM_ASTRO_SET_ROW_X, y);
    lv_obj_clear_flag(phandle->night_sec_period, LV_OBJ_FLAG_SCROLLABLE);
    divoom_astrootoo_settings_night_section_title(phandle, phandle->night_sec_period,
        "NightModeSchedule", DIVOOM_ASTRO_NIGHT_SEC_TITLE_Y);
    sy = DIVOOM_ASTRO_NIGHT_SEC_TITLE_Y + DIVOOM_ASTRO_NIGHT_SEC_TITLE_H + DIVOOM_ASTRO_NIGHT_SEC_GAP + 6;
    phandle->night_start_card = divoom_astrootoo_settings_night_card_on(phandle->night_sec_period,
        phandle, 2, 0, sy, DIVOOM_ASTRO_SET_ROW_H, 1);
    divoom_astrootoo_settings_night_fill_time_row(phandle, phandle->night_start_card, "NightModeStart",
        &phandle->night_start_row_lbl, &phandle->night_start_val_lbl, &phandle->night_start_chev);
    sy += DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_NIGHT_SCROLL_GAP + 6;
    phandle->night_end_card = divoom_astrootoo_settings_night_card_on(phandle->night_sec_period,
        phandle, 3, 0, sy, DIVOOM_ASTRO_SET_ROW_H, 1);
    divoom_astrootoo_settings_night_fill_time_row(phandle, phandle->night_end_card, "NightModeEnd",
        &phandle->night_end_row_lbl, &phandle->night_end_val_lbl, &phandle->night_end_chev);

    divoom_astrootoo_settings_night_create_time_pick(phandle);
    phandle->night_key_btn = divoom_astrootoo_ui_create_key_capture_btn(phandle->root_obj, &phandle->ui,
        divoom_astrootoo_settings_night_time_key_cb, phandle);

    divoom_astrootoo_settings_night_refresh(phandle);
    divoom_astrootoo_settings_group_bind_night(phandle, phandle->night_toggle_card);
}
/* ====================================================================== */
/* 子页：设备语言（列表与 pdivoom_lang_list / DIVOOM_LVGL_MENU_LANG_END 一致） */
/* ====================================================================== */
static int divoom_astrootoo_settings_lang_committed_idx(void)
{
    Pdivoom_sys_setting_info cfg;
    const divoom_select_language_menu_item *lang_list;
    int i;

    cfg = divoom_main_get_system_config();
    if (!cfg)
    {
        return 0;
    }
    lang_list = divoom_lvgl_set_language_get_list();
    for (i = 0; i < DIVOOM_ASTRO_SET_LANG_COUNT; i++)
    {
        if (lang_list[i].lang_id == cfg->lang)
        {
            return i;
        }
    }
    return 0;
}

static void divoom_astrootoo_settings_lang_refresh_checks(Pdivoom_astrootoo_settings_ctrl phandle)
{
    int committed;
    int i;

    committed = divoom_astrootoo_settings_lang_committed_idx();
    for (i = 0; i < DIVOOM_ASTRO_SET_LANG_COUNT; i++)
    {
        if (!phandle->lang_check[i])
        {
            continue;
        }
        if (i == committed)
        {
            lv_obj_clear_flag(phandle->lang_check[i], LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(phandle->lang_check[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static int divoom_astrootoo_settings_is_oobe_language_boot(void)
{
    return (divoom_main_set_wifi_config_status(-1) == 0
        && divoom_menu_control_menu_items() <= 1);
}

enum
{
    DIVOOM_ASTRO_SET_LANG_APPLY_FAILED = -1,
    DIVOOM_ASTRO_SET_LANG_APPLY_UNCHANGED = 0,
    DIVOOM_ASTRO_SET_LANG_APPLY_CHANGED = 1,
};

/*
  Function:         divoom_astrootoo_settings_lang_apply
  Description:      常规设置通过菜单事件切换语言并重建路由；OOBE 保持原有就地提交流程
  Input:            phandle, idx, rebuild_route
  Return:           -1:失败，0:语言未变化，1:已切换或切换事件已入队
*/
static int divoom_astrootoo_settings_lang_apply(Pdivoom_astrootoo_settings_ctrl phandle,
    int idx, int rebuild_route)
{
    Pdivoom_sys_setting_info cfg;
    const divoom_select_language_menu_item *lang_list;
    int lang_id;

    if (divoom_config_set_china_config(-1) == 1)
    {
        return DIVOOM_ASTRO_SET_LANG_APPLY_UNCHANGED;
    }
    if (idx < 0 || idx >= DIVOOM_ASTRO_SET_LANG_COUNT)
    {
        return DIVOOM_ASTRO_SET_LANG_APPLY_FAILED;
    }
    lang_list = divoom_lvgl_set_language_get_list();
    cfg = divoom_main_get_system_config();
    if (!cfg)
    {
        return DIVOOM_ASTRO_SET_LANG_APPLY_FAILED;
    }
    lang_id = lang_list[idx].lang_id;
    if (cfg->lang == lang_id)
    {
        divoom_astrootoo_settings_lang_refresh_checks(phandle);
        return DIVOOM_ASTRO_SET_LANG_APPLY_UNCHANGED;
    }

    if (rebuild_route)
    {
        if (!divoom_menu_control_switch_language(lang_id,
            DIVOOM_MENU_CONTROL_ID_MAIN, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETING_MENU))
        {
            divoom_print("settings: switch language event enqueue failed lang=%d\n", lang_id);
            divoom_astrootoo_settings_lang_refresh_checks(phandle);
            return DIVOOM_ASTRO_SET_LANG_APPLY_FAILED;
        }
        /* 只有路由重建事件成功入队后才提交配置，失败时仍可留在当前语言页重试。 */
        cfg->lang = lang_id;
        divoom_main_send_sys_settings_config();
        if (divoom_main_get_wifi_config_flag())
        {
            divoom_main_save_config();
        }
        return DIVOOM_ASTRO_SET_LANG_APPLY_CHANGED;
    }

    cfg->lang = lang_id;
    divoom_main_send_sys_settings_config();
    if (divoom_main_get_wifi_config_flag())
    {
        divoom_main_save_config();
    }
    divoom_menu_control_set_language(cfg->lang);
    divoom_astrootoo_settings_lang_refresh_checks(phandle);
    return DIVOOM_ASTRO_SET_LANG_APPLY_CHANGED;
}

static void divoom_astrootoo_settings_lang_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    lv_obj_t *target;
    lv_obj_t *next;
    uint8_t key_code;
    int pos;
    int is_oobe;
    int wifi_configured;
    int apply_result;

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

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        if (!divoom_astrootoo_settings_is_oobe_language_boot())
        {
            divoom_menu_control_return_pre();
        }
        return;
    }
    if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code))
    {
        int direct;

        direct = divoom_astrootoo_ui_key_is_up(key_code) ? DIVOOM_MENU_DIRECT_UP : DIVOOM_MENU_DIRECT_DOWN;
        next = divoom_menu_direct_ctrl_get(phandle->direct_handle, target, direct);
        if (next && next != target)
        {
            lv_group_focus_obj(next);
            divoom_astrootoo_ui_focus_scroll(next);
        }
        return;
    }
    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        pos = divoom_menu_direct_ctrl_get_pos(phandle->direct_handle, target);
        if (pos >= 0 && pos < DIVOOM_ASTRO_SET_LANG_COUNT)
        {
            is_oobe = divoom_astrootoo_settings_is_oobe_language_boot();
            wifi_configured = divoom_main_set_wifi_config_status(-1);
            apply_result = divoom_astrootoo_settings_lang_apply(phandle, pos,
                !is_oobe && wifi_configured);
            if (apply_result == DIVOOM_ASTRO_SET_LANG_APPLY_FAILED)
            {
                return;
            }

            if (is_oobe)
            {
                divoom_menu_control_load_next_pop_cur(DIVOOM_MENU_CONTROL_ID_ASTROTOO_OOBE_WELCOME, NULL, 0, 0);
            }
            else if (wifi_configured)
            {
                if (apply_result == DIVOOM_ASTRO_SET_LANG_APPLY_UNCHANGED)
                {
                    divoom_menu_control_return_pre();
                }
            }
            else
            {
                divoom_menu_control_load_next_pop_cur(DIVOOM_MENU_CONTROL_ID_CONFIG_WIFI, NULL, 0, 0);
            }
        }
    }
}

static void divoom_astrootoo_settings_build_language_oobe(Pdivoom_astrootoo_settings_ctrl phandle)
{
    lv_obj_t *icon;
    lv_obj_t *card;
    lv_obj_t *lbl;
    lv_coord_t y;
    int i;
    int committed;
    const divoom_select_language_menu_item *lang_list;

    lang_list = divoom_lvgl_set_language_get_list();
    committed = divoom_astrootoo_settings_lang_committed_idx();
    phandle->lang_focus = committed;
    phandle->direct_handle = divoom_menu_direct_ctrl_create(1, DIVOOM_ASTRO_SET_LANG_COUNT);

    icon = lv_img_create(phandle->root_obj);
    divoom_menu_control_lv_img_set_src(icon, DIVOOM_ASTRO_OOBE_LANG_ICON);
    lv_obj_set_size(icon, 36, 36);
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 28);

    phandle->content = divoom_astrootoo_ui_create_scroll_content(phandle->root_obj, &phandle->ui);
    lv_obj_align(phandle->content, LV_ALIGN_TOP_MID, 0, 72);

    y = 0;
    for (i = 0; i < DIVOOM_ASTRO_SET_LANG_COUNT; i++)
    {
        card = lv_btn_create(phandle->content);
        lv_obj_add_style(card, &phandle->ui.card_item_style, 0);
        lv_obj_add_style(card, &phandle->ui.card_item_focus_style, LV_STATE_FOCUSED);
        lv_obj_add_style(card, &phandle->ui.card_item_focus_style, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUS_KEY);
        lv_obj_set_size(card, DIVOOM_ASTRO_OOBE_LANG_ROW_W, DIVOOM_ASTRO_OOBE_LANG_ROW_H);
        lv_obj_set_pos(card, DIVOOM_ASTRO_OOBE_LANG_ROW_X, y);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        divoom_menu_direct_ctrl_update(phandle->direct_handle, card, 0, i);
        lv_obj_add_event_cb(card, divoom_astrootoo_settings_lang_key_cb, LV_EVENT_KEY, phandle);

        lbl = lv_label_create(card);
        lv_label_set_text(lbl, divoom_menu_control_get_disp_str(lang_list[i].lang_info));
        lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
        lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 20, 0);
        divoom_astrootoo_ui_list_label_prepare(lbl,
            divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_OOBE_LANG_ROW_W, 20, 20, 0));
        divoom_astrootoo_ui_list_row_bind_marquee(card, lbl);

        phandle->lang_row[i] = card;
        phandle->lang_check[i] = NULL;
        y += DIVOOM_ASTRO_OOBE_LANG_ROW_H + DIVOOM_ASTRO_OOBE_LANG_ROW_GAP;
    }

    divoom_astrootoo_settings_group_bind_lang(phandle,
        phandle->lang_row[committed] ? phandle->lang_row[committed] : NULL);
}

static void divoom_astrootoo_settings_build_language(Pdivoom_astrootoo_settings_ctrl phandle)
{
    lv_obj_t *card;
    lv_obj_t *lbl;
    lv_obj_t *chk;
    lv_coord_t y;
    int i;
    int committed;
    const divoom_select_language_menu_item *lang_list;

    if (divoom_astrootoo_settings_is_oobe_language_boot())
    {
        divoom_astrootoo_settings_build_language_oobe(phandle);
        return;
    }

    lang_list = divoom_lvgl_set_language_get_list();
    committed = divoom_astrootoo_settings_lang_committed_idx();
    phandle->lang_focus = committed;
    phandle->direct_handle = divoom_menu_direct_ctrl_create(1, DIVOOM_ASTRO_SET_LANG_COUNT);
    divoom_astrootoo_settings_create_header(phandle, "Language");
    phandle->content = divoom_astrootoo_ui_create_scroll_content(phandle->root_obj, &phandle->ui);

    y = DIVOOM_ASTRO_SET_TOP_GAP;
    for (i = 0; i < DIVOOM_ASTRO_SET_LANG_COUNT; i++)
    {
        card = lv_btn_create(phandle->content);
        lv_obj_add_style(card, &phandle->ui.card_item_style, 0);
        lv_obj_add_style(card, &phandle->ui.card_item_focus_style, LV_STATE_FOCUSED);
        lv_obj_add_style(card, &phandle->ui.card_item_focus_style, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUS_KEY);
        lv_obj_set_size(card, DIVOOM_ASTRO_SET_ROW_W, DIVOOM_ASTRO_SET_ROW_H);
        lv_obj_set_pos(card, DIVOOM_ASTRO_SET_ROW_X, y);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        divoom_menu_direct_ctrl_update(phandle->direct_handle, card, 0, i);
        lv_obj_add_event_cb(card, divoom_astrootoo_settings_lang_key_cb, LV_EVENT_KEY, phandle);

        lbl = lv_label_create(card);
        lv_label_set_text(lbl, divoom_menu_control_get_disp_str(lang_list[i].lang_info));
        lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
        lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 8, 0);
        divoom_astrootoo_ui_list_label_prepare(lbl,
            divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_SET_ROW_W,
                DIVOOM_ASTRO_SET_CARD_LABEL_INSET_L, DIVOOM_ASTRO_SET_CARD_LABEL_INSET_R,
                DIVOOM_ASTRO_LIST_CHK_TRAIL_CARD));
        divoom_astrootoo_ui_list_row_bind_marquee(card, lbl);

        chk = lv_img_create(card);
        divoom_menu_control_lv_img_set_src(chk, DIVOOM_ASTRO_SET_CHECK_ICON);
        lv_obj_align(chk, LV_ALIGN_RIGHT_MID, -16, 0);
        lv_obj_add_flag(chk, LV_OBJ_FLAG_HIDDEN);

        phandle->lang_row[i] = card;
        phandle->lang_check[i] = chk;
        y += DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROW_GAP;
    }

    divoom_astrootoo_settings_lang_refresh_checks(phandle);
    divoom_astrootoo_settings_group_bind_lang(phandle,
        phandle->lang_row[committed] ? phandle->lang_row[committed] : NULL);
}

/* ====================================================================== */
/* 子页：蓝牙（真源 DeviceSettingsBluetoothScreen + divoom_bt_music）      */
/* ====================================================================== */
static void divoom_astrootoo_settings_bt_set_label_if_changed(lv_obj_t *row, lv_obj_t *lbl,
    const char *text, int marquee_always)
{
    const char *cur;

    if (!lbl || !text)
    {
        return;
    }
    cur = lv_label_get_text(lbl);
    if (cur && strcmp(cur, text) == 0)
    {
        return;
    }
    lv_label_set_text(lbl, text);
    if (marquee_always)
    {
        divoom_astrootoo_ui_list_label_marquee_always(lbl);
    }
    else
    {
        divoom_astrootoo_ui_list_label_refresh_marquee(row, lbl);
    }
}

static void divoom_astrootoo_settings_bt_switch_reset(Pdivoom_astrootoo_settings_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }
    phandle->bt_switch_stage = 0;
    phandle->bt_switch_wait_tick = 0;
    phandle->bt_switch_target_addr[0] = '\0';
}

static void divoom_astrootoo_settings_bt_ui_sig_reset(Pdivoom_astrootoo_settings_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }
    phandle->bt_sw_on_sig = DIVOOM_ASTRO_SET_BT_SIG_INVALID;
    phandle->bt_linked_sig = DIVOOM_ASTRO_SET_BT_SIG_INVALID;
    phandle->bt_device_list_sig = DIVOOM_ASTRO_SET_BT_SIG_INVALID;
    phandle->bt_checked_row = DIVOOM_ASTRO_SET_BT_CHECKED_ROW_INVALID;
    divoom_astrootoo_settings_bt_switch_reset(phandle);
}

static uint8 divoom_astrootoo_settings_bt_device_list_sig(const DeviceInfo *list, int cnt)
{
    uint8 sig;
    int i;
    int j;

    sig = (uint8)(cnt & 0xFF);
    if (!list || cnt <= 0)
    {
        return sig;
    }
    for (i = 0; i < cnt; i++)
    {
        for (j = 0; list[i].bt_addr[j] != '\0' && j < 17; j++)
        {
            sig ^= (uint8)list[i].bt_addr[j];
        }
        sig ^= (uint8)list[i].device_name[0];
    }
    return sig;
}

static lv_coord_t divoom_astrootoo_settings_bt_device_area_y(void)
{
    return DIVOOM_ASTRO_SET_TOP_GAP
        + 6 * (DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROW_GAP)
        + DIVOOM_ASTRO_SET_SEC_H;
}

static void divoom_astrootoo_settings_bt_reregister_direct(Pdivoom_astrootoo_settings_ctrl phandle)
{
    int i;
    int cnt;
    int fi;
    uint8 kind;

    if (!phandle)
    {
        return;
    }
    cnt = 0;
    divoom_bt_music_get_device_list(&cnt);
    if (cnt < 0)
    {
        cnt = 0;
    }
    fi = 4 + cnt;
    if (fi > DIVOOM_ASTRO_SET_BT_MAX_ROW)
    {
        fi = DIVOOM_ASTRO_SET_BT_MAX_ROW;
    }
    if (phandle->direct_handle)
    {
        divoom_menu_direct_ctrl_destory(phandle->direct_handle);
    }
    phandle->direct_handle = divoom_menu_direct_ctrl_create(1, fi);
    phandle->bt_focus = 0;
    for (i = 0; i < phandle->bt_row_count; i++)
    {
        if (!phandle->bt_row[i])
        {
            continue;
        }
        kind = phandle->bt_row_kind[i];
        if (kind == DIVOOM_ASTRO_SET_BT_KIND_SECTION
            || kind == DIVOOM_ASTRO_SET_BT_KIND_NAME
            || kind == DIVOOM_ASTRO_SET_BT_KIND_STATUS
            || kind == DIVOOM_ASTRO_SET_BT_KIND_EMPTY)
        {
            continue;
        }
        divoom_menu_direct_ctrl_update(phandle->direct_handle, phandle->bt_row[i],
            0, phandle->bt_focus++);
    }
}

static void divoom_astrootoo_settings_bt_refresh_labels(Pdivoom_astrootoo_settings_ctrl phandle);

static void divoom_astrootoo_settings_bt_rebuild_device_rows(Pdivoom_astrootoo_settings_ctrl phandle)
{
    lv_coord_t y;
    int cnt;
    int i;
    int idx;
    lv_obj_t *card;
    lv_group_t *g;
    lv_obj_t *prev_focus;
    const DeviceInfo *list;

    if (!phandle || phandle->page != DIVOOM_ASTROTOO_SETTINGS_PAGE_BLUETOOTH
        || !phandle->content || !lv_obj_is_valid(phandle->content))
    {
        return;
    }
    if (phandle->bt_row_count < DIVOOM_ASTRO_SET_BT_DEVICE_START)
    {
        return;
    }

    g = lv_group_get_default();
    prev_focus = g ? lv_group_get_focused(g) : NULL;

    for (i = DIVOOM_ASTRO_SET_BT_DEVICE_START; i < phandle->bt_row_count; i++)
    {
        if (phandle->bt_row[i] && lv_obj_is_valid(phandle->bt_row[i]))
        {
            divoom_astrootoo_settings_group_remove_obj_safe(phandle->bt_row[i]);
            lv_obj_del(phandle->bt_row[i]);
        }
        phandle->bt_row[i] = NULL;
        phandle->bt_row_lbl[i] = NULL;
        phandle->bt_row_val[i] = NULL;
        phandle->bt_row_trail[i] = NULL;
        phandle->bt_row_kind[i] = 0;
        phandle->bt_row_dev_idx[i] = -1;
    }
    phandle->bt_row_count = DIVOOM_ASTRO_SET_BT_DEVICE_START;
    phandle->bt_checked_row = DIVOOM_ASTRO_SET_BT_CHECKED_ROW_INVALID;

    list = divoom_bt_music_get_device_list(&cnt);
    if (cnt < 0)
    {
        cnt = 0;
    }
    if (cnt > MAX_BT_DEVICE_COUNT)
    {
        cnt = MAX_BT_DEVICE_COUNT;
    }
    phandle->bt_device_list_sig = divoom_astrootoo_settings_bt_device_list_sig(list, cnt);

    y = divoom_astrootoo_settings_bt_device_area_y();
    idx = DIVOOM_ASTRO_SET_BT_DEVICE_START;
    if (cnt <= 0)
    {
        card = divoom_astrootoo_settings_bt_add_row(phandle, idx++, y,
            DIVOOM_ASTRO_SET_BT_KIND_EMPTY, "NoConnectedDevices", NULL, 0);
        (void)card;
        y += DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROW_GAP;
    }
    else
    {
        for (i = 0; i < cnt && idx < DIVOOM_ASTRO_SET_BT_MAX_ROW; i++)
        {
            card = divoom_astrootoo_settings_bt_add_row(phandle, idx, y,
                DIVOOM_ASTRO_SET_BT_KIND_DEVICE, "Bluetooth", list[i].device_name, 0);
            if (card)
            {
                phandle->bt_row_dev_idx[idx] = i;
                lv_obj_add_event_cb(card, divoom_astrootoo_settings_bt_key_cb,
                    LV_EVENT_KEY, phandle);
            }
            idx++;
            y += DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROW_GAP;
        }
    }
    phandle->bt_row_count = idx;

    divoom_astrootoo_settings_bt_reregister_direct(phandle);
    phandle->bt_linked_sig = DIVOOM_ASTRO_SET_BT_SIG_INVALID;
    divoom_astrootoo_settings_bt_refresh_labels(phandle);
    lv_obj_update_layout(phandle->content);
    divoom_astrootoo_settings_group_bind_bt(phandle, prev_focus);
}

static int divoom_astrootoo_settings_bt_device_is_connected(const DeviceInfo *dev)
{
    const DeviceInfo *cur;

    if (!dev || !divoom_bt_music_is_connected())
    {
        return 0;
    }
    cur = divoom_bt_music_get_current_device_info();
    if (!cur)
    {
        return 0;
    }
    if (dev->bt_addr[0] != '\0' && cur->bt_addr[0] != '\0'
        && strcmp(dev->bt_addr, cur->bt_addr) == 0)
    {
        return 1;
    }
    if (dev->device_name[0] != '\0' && cur->device_name[0] != '\0'
        && strcmp(dev->device_name, cur->device_name) == 0)
    {
        return 1;
    }
    return 0;
}

static int divoom_astrootoo_settings_bt_find_checked_row(Pdivoom_astrootoo_settings_ctrl phandle,
    const DeviceInfo *list, int cnt)
{
    int i;

    if (!phandle || !list || cnt <= 0 || !divoom_bt_music_is_connected())
    {
        return DIVOOM_ASTRO_SET_BT_CHECKED_ROW_NONE;
    }
    for (i = 0; i < phandle->bt_row_count; i++)
    {
        int dev_idx;

        if (phandle->bt_row_kind[i] != DIVOOM_ASTRO_SET_BT_KIND_DEVICE)
        {
            continue;
        }
        dev_idx = phandle->bt_row_dev_idx[i];
        if (dev_idx >= 0 && dev_idx < cnt
            && divoom_astrootoo_settings_bt_device_is_connected(&list[dev_idx]))
        {
            return i;
        }
    }
    return DIVOOM_ASTRO_SET_BT_CHECKED_ROW_NONE;
}

static void divoom_astrootoo_settings_bt_refresh_device_check(Pdivoom_astrootoo_settings_ctrl phandle,
    const DeviceInfo *list, int cnt)
{
    int want_row;
    int prev_row;

    want_row = divoom_astrootoo_settings_bt_find_checked_row(phandle, list, cnt);
    if (want_row == phandle->bt_checked_row)
    {
        return;
    }

    prev_row = phandle->bt_checked_row;
    phandle->bt_checked_row = (int8)want_row;

    if (prev_row >= 0 && prev_row < phandle->bt_row_count && phandle->bt_row_trail[prev_row])
    {
        lv_obj_add_flag(phandle->bt_row_trail[prev_row], LV_OBJ_FLAG_HIDDEN);
    }
    if (want_row >= 0 && want_row < phandle->bt_row_count && phandle->bt_row_trail[want_row])
    {
        divoom_menu_control_lv_img_set_src(phandle->bt_row_trail[want_row],
            DIVOOM_ASTRO_SET_CHECK_ICON);
        lv_obj_clear_flag(phandle->bt_row_trail[want_row], LV_OBJ_FLAG_HIDDEN);
    }
}

static void divoom_astrootoo_settings_bt_refresh_trail(Pdivoom_astrootoo_settings_ctrl phandle)
{
    int linked;
    int i;
    int cnt;
    const DeviceInfo *list;

    if (!phandle)
    {
        return;
    }
    linked = divoom_bt_music_is_connected() ? 1 : 0;
    if ((uint8)linked != phandle->bt_linked_sig)
    {
        phandle->bt_linked_sig = (uint8)linked;
        for (i = 0; i < phandle->bt_row_count; i++)
        {
            if (!phandle->bt_row_trail[i])
            {
                continue;
            }
            if (phandle->bt_row_kind[i] == DIVOOM_ASTRO_SET_BT_KIND_DISCONNECT)
            {
                divoom_menu_control_lv_img_set_src(phandle->bt_row_trail[i],
                    linked ? DIVOOM_ASTRO_SET_BT_ICON_DISCONNECT_LINKED
                    : DIVOOM_ASTRO_SET_BT_ICON_DISCONNECT_IDLE);
                lv_obj_clear_flag(phandle->bt_row_trail[i], LV_OBJ_FLAG_HIDDEN);
            }
            else if (phandle->bt_row_kind[i] == DIVOOM_ASTRO_SET_BT_KIND_UNPAIR)
            {
                divoom_menu_control_lv_img_set_src(phandle->bt_row_trail[i],
                    linked ? DIVOOM_ASTRO_SET_BT_ICON_UNPAIR_LINKED
                    : DIVOOM_ASTRO_SET_BT_ICON_UNPAIR_IDLE);
                lv_obj_clear_flag(phandle->bt_row_trail[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }

    list = divoom_bt_music_get_device_list(&cnt);
    divoom_astrootoo_settings_bt_refresh_device_check(phandle, list, cnt);
}

static void divoom_astrootoo_settings_bt_refresh_labels(Pdivoom_astrootoo_settings_ctrl phandle)
{
    Pdivoom_sys_setting_info cfg;
    const DeviceInfo *cur;
    const char *local_name;
    int auto_on;
    int i;

    if (!phandle || phandle->bt_row_count <= 0)
    {
        return;
    }
    cfg = divoom_main_get_system_config();
    auto_on = (cfg && cfg->bt_auto_connect) ? 1 : 0;
    if (phandle->bt_sw_img && (uint8)auto_on != phandle->bt_sw_on_sig)
    {
        phandle->bt_sw_on_sig = (uint8)auto_on;
        divoom_menu_control_lv_img_set_src(phandle->bt_sw_img,
            auto_on ? DIVOOM_ASTROTOO_SETTINGS_SWITCH_ON : DIVOOM_ASTROTOO_SETTINGS_SWITCH_OFF);
    }
    local_name = divoom_bt_music_get_local_adapter_name();
    if (phandle->bt_row_val[1])
    {
        divoom_astrootoo_settings_bt_set_label_if_changed(phandle->bt_row[1], phandle->bt_row_val[1],
            (local_name && local_name[0]) ? local_name : "—", 1);
    }
    cur = divoom_bt_music_get_current_device_info();
    if (phandle->bt_row_val[2])
    {
        if (divoom_bt_music_is_connected() && cur && cur->device_name[0])
        {
            divoom_astrootoo_settings_bt_set_label_if_changed(phandle->bt_row[2], phandle->bt_row_val[2],
                cur->device_name, 1);
        }
        else
        {
            divoom_astrootoo_settings_bt_set_label_if_changed(phandle->bt_row[2], phandle->bt_row_val[2],
                divoom_menu_control_get_disp_str("BTDisconnectedTip"), 1);
        }
    }
    for (i = 0; i < phandle->bt_row_count; i++)
    {
        if (phandle->bt_row_kind[i] != DIVOOM_ASTRO_SET_BT_KIND_DEVICE)
        {
            continue;
        }
        if (phandle->bt_row_dev_idx[i] >= 0 && phandle->bt_row_lbl[i])
        {
            int cnt;
            const DeviceInfo *list;

            list = divoom_bt_music_get_device_list(&cnt);
            if (list && phandle->bt_row_dev_idx[i] < cnt)
            {
                divoom_astrootoo_settings_bt_set_label_if_changed(phandle->bt_row[i], phandle->bt_row_lbl[i],
                    list[phandle->bt_row_dev_idx[i]].device_name, 0);
            }
        }
    }
    divoom_astrootoo_settings_bt_refresh_trail(phandle);
    (void)cur;
}

static int divoom_astrootoo_settings_bt_same_device(const DeviceInfo *a, const DeviceInfo *b)
{
    if (!a || !b)
    {
        return 0;
    }
    if (a->bt_addr[0] != '\0' && b->bt_addr[0] != '\0'
        && strcmp(a->bt_addr, b->bt_addr) == 0)
    {
        return 1;
    }
    if (a->device_name[0] != '\0' && b->device_name[0] != '\0'
        && strcmp(a->device_name, b->device_name) == 0)
    {
        return 1;
    }
    return 0;
}

static void divoom_astrootoo_settings_bt_begin_connect(Pdivoom_astrootoo_settings_ctrl phandle,
    const char *bt_addr)
{
    if (!phandle || !bt_addr || bt_addr[0] == '\0')
    {
        return;
    }
    strncpy(phandle->bt_switch_target_addr, bt_addr,
        sizeof(phandle->bt_switch_target_addr) - 1);
    phandle->bt_switch_target_addr[sizeof(phandle->bt_switch_target_addr) - 1] = '\0';
    if (divoom_bt_music_connect_device(bt_addr) == 0)
    {
        phandle->bt_switch_stage = 2;
        phandle->bt_switch_wait_tick = 0;
    }
    else
    {
        divoom_astrootoo_settings_bt_switch_reset(phandle);
    }
}

static void divoom_astrootoo_settings_bt_request_connect_device(Pdivoom_astrootoo_settings_ctrl phandle,
    const DeviceInfo *target)
{
    const DeviceInfo *cur;

    if (!phandle || !target || target->bt_addr[0] == '\0')
    {
        return;
    }
    if (divoom_astrootoo_settings_bt_device_is_connected(target))
    {
        return;
    }
    if (phandle->bt_switch_stage != 0
        && strcmp(phandle->bt_switch_target_addr, target->bt_addr) == 0)
    {
        return;
    }

    cur = divoom_bt_music_get_current_device_info();
    if (divoom_bt_music_is_connected() && cur != NULL
        && cur->bt_addr[0] != '\0'
        && !divoom_astrootoo_settings_bt_same_device(target, cur))
    {
        strncpy(phandle->bt_switch_target_addr, target->bt_addr,
            sizeof(phandle->bt_switch_target_addr) - 1);
        phandle->bt_switch_target_addr[sizeof(phandle->bt_switch_target_addr) - 1] = '\0';
        divoom_bt_music_disconnect_bt();
        phandle->bt_switch_stage = 1;
        phandle->bt_switch_wait_tick = 0;
    }
    else
    {
        divoom_astrootoo_settings_bt_begin_connect(phandle, target->bt_addr);
    }
}

static void divoom_astrootoo_settings_bt_switch_poll(Pdivoom_astrootoo_settings_ctrl phandle)
{
    const DeviceInfo *cur;
    DeviceInfo target;

    if (!phandle || phandle->bt_switch_stage == 0)
    {
        return;
    }

    phandle->bt_switch_wait_tick++;
    if (phandle->bt_switch_wait_tick >= DIVOOM_ASTRO_SET_BT_SWITCH_WAIT_MAX)
    {
        divoom_astrootoo_settings_bt_switch_reset(phandle);
        divoom_astrootoo_settings_bt_refresh_labels(phandle);
        return;
    }

    memset(&target, 0, sizeof(target));
    strncpy(target.bt_addr, phandle->bt_switch_target_addr, sizeof(target.bt_addr) - 1);
    target.bt_addr[sizeof(target.bt_addr) - 1] = '\0';
    cur = divoom_bt_music_get_current_device_info();

    if (phandle->bt_switch_stage == 1)
    {
        if (divoom_bt_music_is_connected() && cur != NULL
            && divoom_astrootoo_settings_bt_same_device(&target, cur))
        {
            divoom_astrootoo_settings_bt_switch_reset(phandle);
            divoom_astrootoo_settings_bt_refresh_labels(phandle);
            return;
        }
        if (!divoom_bt_music_is_connected())
        {
            divoom_astrootoo_settings_bt_begin_connect(phandle, phandle->bt_switch_target_addr);
        }
        return;
    }

    if (phandle->bt_switch_stage == 2 && divoom_bt_music_is_connected() && cur != NULL
        && divoom_astrootoo_settings_bt_same_device(&target, cur))
    {
        divoom_astrootoo_settings_bt_switch_reset(phandle);
        divoom_astrootoo_settings_bt_refresh_labels(phandle);
    }
}

static void divoom_astrootoo_settings_bt_timer_cb(lv_timer_t *t)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    int cnt;
    uint8 sig;
    const DeviceInfo *list;

    phandle = (Pdivoom_astrootoo_settings_ctrl)lv_timer_get_user_data(t);
    if (!phandle || !phandle->root_obj || !lv_obj_is_valid(phandle->root_obj))
    {
        return;
    }
    divoom_astrootoo_settings_bt_switch_poll(phandle);
    list = divoom_bt_music_get_device_list(&cnt);
    if (cnt < 0)
    {
        cnt = 0;
    }
    sig = divoom_astrootoo_settings_bt_device_list_sig(list, cnt);
    if (sig != phandle->bt_device_list_sig)
    {
        divoom_astrootoo_settings_bt_rebuild_device_rows(phandle);
        return;
    }
    divoom_astrootoo_settings_bt_refresh_labels(phandle);
}

static void divoom_astrootoo_settings_stop_page_timers(Pdivoom_astrootoo_settings_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }
    if (phandle->bt_timer)
    {
        lv_timer_del(phandle->bt_timer);
        phandle->bt_timer = NULL;
    }
    divoom_astrootoo_settings_battery_timer_stop(phandle);
    if (phandle->connection_code_timer)
    {
        lv_timer_del(phandle->connection_code_timer);
        phandle->connection_code_timer = NULL;
    }
    if (phandle->about_update_timer)
    {
        lv_timer_del(phandle->about_update_timer);
        phandle->about_update_timer = NULL;
    }
    /* The read-only worker owns its updater reservation until it finishes. */
    divoom_astrootoo_settings_wifi_clean(phandle);
}

static lv_obj_t *divoom_astrootoo_settings_bt_add_row(Pdivoom_astrootoo_settings_ctrl phandle,
    int idx, lv_coord_t y, uint8 kind, const char *left_key, const char *right_text, int focusable)
{
    lv_obj_t *card;
    lv_obj_t *lbl;
    lv_obj_t *val;

    if (kind == DIVOOM_ASTRO_SET_BT_KIND_SECTION)
    {
        lbl = lv_label_create(phandle->content);
        lv_label_set_text(lbl, divoom_menu_control_get_disp_str(left_key));
        lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
        lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
        lv_obj_set_pos(lbl, DIVOOM_ASTRO_SET_ROW_X + 4, y + 8);
        phandle->bt_row_kind[idx] = kind;
        phandle->bt_row_lbl[idx] = lbl;
        return NULL;
    }

    card = lv_btn_create(phandle->content);
    lv_obj_add_style(card, &phandle->ui.card_item_style, 0);
    lv_obj_add_style(card, &phandle->ui.card_item_focus_style, LV_STATE_FOCUSED);
    lv_obj_add_style(card, &phandle->ui.card_item_focus_style, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_size(card, DIVOOM_ASTRO_SET_ROW_W, DIVOOM_ASTRO_SET_ROW_H);
    lv_obj_set_pos(card, DIVOOM_ASTRO_SET_ROW_X, y);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    if (focusable)
    {
        divoom_menu_direct_ctrl_update(phandle->direct_handle, card, 0, phandle->bt_focus);
        phandle->bt_focus++;
    }

    lbl = lv_label_create(card);
    lv_label_set_text(lbl, divoom_menu_control_get_disp_str(left_key));
    lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, DIVOOM_ASTRO_SET_BT_PAD_L, 0);
    phandle->bt_row_val[idx] = NULL;
    phandle->bt_row_trail[idx] = NULL;

    if (kind == DIVOOM_ASTRO_SET_BT_KIND_TOGGLE)
    {
        divoom_astrootoo_ui_list_label_prepare(lbl,
            divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_SET_ROW_W,
                DIVOOM_ASTRO_SET_CARD_LABEL_INSET_L, DIVOOM_ASTRO_SET_CARD_LABEL_INSET_R,
                DIVOOM_ASTRO_LIST_TOGGLE_W));
        divoom_astrootoo_ui_list_row_bind_marquee(card, lbl);

        phandle->bt_sw_img = lv_img_create(card);
        divoom_menu_control_lv_img_set_src(phandle->bt_sw_img, DIVOOM_ASTROTOO_SETTINGS_SWITCH_OFF);
        lv_obj_align(phandle->bt_sw_img, LV_ALIGN_RIGHT_MID, -DIVOOM_ASTRO_SET_BT_PAD_R, 0);
    }
    else if (kind == DIVOOM_ASTRO_SET_BT_KIND_LINK_PASSWORD)
    {
        divoom_astrootoo_ui_list_label_prepare(lbl,
            divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_SET_ROW_W,
                DIVOOM_ASTRO_SET_CARD_LABEL_INSET_L, DIVOOM_ASTRO_SET_CARD_LABEL_INSET_R,
                DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W));
        divoom_astrootoo_ui_list_row_bind_marquee(card, lbl);

        lv_obj_t *chev;

        chev = lv_img_create(card);
        divoom_menu_control_lv_img_set_src(chev, DIVOOM_ASTROTOO_SETTINGS_CHEVRON);
        divoom_astrootoo_ui_apply_sublevel_chevron_size(chev);
        lv_obj_align(chev, LV_ALIGN_RIGHT_MID, -DIVOOM_ASTRO_SET_BT_PAD_R, 0);
    }
    else if (kind == DIVOOM_ASTRO_SET_BT_KIND_DISCONNECT
        || kind == DIVOOM_ASTRO_SET_BT_KIND_UNPAIR)
    {
        lv_obj_t *trail;

        trail = lv_img_create(card);
        lv_obj_set_size(trail, DIVOOM_ASTRO_SET_BT_ICON_SZ, DIVOOM_ASTRO_SET_BT_ICON_SZ);
        lv_obj_align(trail, LV_ALIGN_RIGHT_MID, -DIVOOM_ASTRO_SET_BT_PAD_R, 0);
        lv_obj_add_flag(trail, LV_OBJ_FLAG_HIDDEN);
        phandle->bt_row_trail[idx] = trail;
    }
    else if (kind == DIVOOM_ASTRO_SET_BT_KIND_DEVICE)
    {
        lv_obj_t *chk;

        /* 真源 SettingsDeviceRow：只画设备名，连接中设备右侧勾选 */
        lv_label_set_text(lbl, (right_text && right_text[0]) ? right_text : "");
        divoom_astrootoo_ui_list_label_prepare(lbl,
            divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_SET_ROW_W,
                DIVOOM_ASTRO_SET_CARD_LABEL_INSET_L, DIVOOM_ASTRO_SET_CARD_LABEL_INSET_R,
                DIVOOM_ASTRO_LIST_CHK_TRAIL_CARD));
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, DIVOOM_ASTRO_SET_BT_PAD_L, 0);
        divoom_astrootoo_ui_list_row_bind_marquee(card, lbl);

        chk = lv_img_create(card);
        lv_obj_align(chk, LV_ALIGN_RIGHT_MID, -DIVOOM_ASTRO_SET_BT_PAD_R, 0);
        lv_obj_add_flag(chk, LV_OBJ_FLAG_HIDDEN);
        phandle->bt_row_trail[idx] = chk;
    }
    else if (right_text)
    {
        lv_coord_t left_w;
        lv_coord_t value_w;

        val = lv_label_create(card);
        lv_label_set_text(val, right_text);
        lv_obj_set_style_text_color(val, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
        lv_obj_set_style_text_font(val, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
        lv_obj_update_layout(card);
        left_w = lv_obj_get_width(lbl);
        value_w = DIVOOM_ASTRO_SET_ROW_W - DIVOOM_ASTRO_SET_CARD_PAD_H * 2
            - DIVOOM_ASTRO_SET_BT_PAD_L - DIVOOM_ASTRO_SET_BT_PAD_R - left_w
            - DIVOOM_ASTRO_SET_BT_GAP - DIVOOM_ASTRO_LIST_LABEL_GAP;
        if (value_w < DIVOOM_ASTRO_SET_BT_VALUE_MIN_W)
        {
            left_w = DIVOOM_ASTRO_SET_ROW_W - DIVOOM_ASTRO_SET_CARD_PAD_H * 2
                - DIVOOM_ASTRO_SET_BT_PAD_L - DIVOOM_ASTRO_SET_BT_PAD_R
                - DIVOOM_ASTRO_SET_BT_GAP - DIVOOM_ASTRO_SET_BT_VALUE_MIN_W
                - DIVOOM_ASTRO_LIST_LABEL_GAP;
            if (left_w < DIVOOM_ASTRO_SET_BT_LABEL_MIN_W)
            {
                left_w = DIVOOM_ASTRO_SET_BT_LABEL_MIN_W;
            }
            lv_obj_set_width(lbl, left_w);
            lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
            lv_obj_align(lbl, LV_ALIGN_LEFT_MID, DIVOOM_ASTRO_SET_BT_PAD_L, 0);
            value_w = DIVOOM_ASTRO_SET_BT_VALUE_MIN_W;
        }
        lv_obj_set_size(val, value_w, DIVOOM_ASTRO_SET_BT_VALUE_H);
        divoom_astrootoo_ui_list_label_prepare(val, value_w);
        lv_obj_set_style_text_align(val, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_align(val, LV_ALIGN_RIGHT_MID, -DIVOOM_ASTRO_SET_BT_PAD_R, 0);
        if (focusable)
        {
            divoom_astrootoo_ui_list_row_bind_marquee(card, val);
        }
        else
        {
            divoom_astrootoo_ui_list_label_marquee_always(val);
        }
        phandle->bt_row_val[idx] = val;
    }

    phandle->bt_row[idx] = card;
    phandle->bt_row_lbl[idx] = lbl;
    phandle->bt_row_kind[idx] = kind;
    phandle->bt_row_dev_idx[idx] = -1;
    return card;
}

static void divoom_astrootoo_settings_bt_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    lv_obj_t *target;
    lv_obj_t *next;
    uint8_t key_code;
    int pos;
    int idx;
    Pdivoom_sys_setting_info cfg;

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

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_main_save_config();
        divoom_menu_control_return_pre();
        return;
    }
    if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code))
    {
        int direct;

        direct = divoom_astrootoo_ui_key_is_up(key_code) ? DIVOOM_MENU_DIRECT_UP : DIVOOM_MENU_DIRECT_DOWN;
        next = divoom_menu_direct_ctrl_get(phandle->direct_handle, target, direct);
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
    for (idx = 0; idx < phandle->bt_row_count; idx++)
    {
        if (phandle->bt_row[idx] != target)
        {
            continue;
        }
        cfg = divoom_main_get_system_config();
        if (phandle->bt_row_kind[idx] == DIVOOM_ASTRO_SET_BT_KIND_LINK_PASSWORD)
        {
            divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_BT_SET_SECRET, NULL, 0, 0);
        }
        else if (phandle->bt_row_kind[idx] == DIVOOM_ASTRO_SET_BT_KIND_TOGGLE && cfg)
        {
            if (cfg->bt_auto_connect)
            {
                if (divoom_bt_music_disable_boot_connect(1) == 0)
                {
                    cfg->bt_auto_connect = 0;
                    divoom_main_save_config();
                }
            }
            else
            {
                if (divoom_bt_music_disable_boot_connect(0) == 0)
                {
                    cfg->bt_auto_connect = 1;
                    divoom_main_save_config();
                }
            }
            divoom_astrootoo_settings_bt_refresh_labels(phandle);
        }
        else if (phandle->bt_row_kind[idx] == DIVOOM_ASTRO_SET_BT_KIND_DISCONNECT)
        {
            divoom_bt_music_disconnect_bt();
            divoom_astrootoo_settings_bt_switch_reset(phandle);
            divoom_astrootoo_settings_bt_refresh_labels(phandle);
        }
        else if (phandle->bt_row_kind[idx] == DIVOOM_ASTRO_SET_BT_KIND_UNPAIR)
        {
            divoom_bt_music_unpair_all_devices();
            divoom_astrootoo_settings_bt_switch_reset(phandle);
            divoom_astrootoo_settings_bt_rebuild_device_rows(phandle);
        }
        else if (phandle->bt_row_kind[idx] == DIVOOM_ASTRO_SET_BT_KIND_DEVICE
            && phandle->bt_row_dev_idx[idx] >= 0)
        {
            int cnt;
            const DeviceInfo *list;

            list = divoom_bt_music_get_device_list(&cnt);
            if (list && phandle->bt_row_dev_idx[idx] < cnt)
            {
                divoom_astrootoo_settings_bt_request_connect_device(phandle,
                    &list[phandle->bt_row_dev_idx[idx]]);
            }
            divoom_astrootoo_settings_bt_refresh_labels(phandle);
        }
        break;
    }
    (void)pos;
}

static void divoom_astrootoo_settings_build_bluetooth(Pdivoom_astrootoo_settings_ctrl phandle)
{
    lv_obj_t *card;
    lv_coord_t y;
    int cnt;
    int idx;
    int fi;
    int right_linked;

    phandle->bt_row_count = 0;
    phandle->bt_focus = 0;
    divoom_astrootoo_settings_bt_ui_sig_reset(phandle);
    divoom_bt_music_get_device_list(&cnt);
    if (cnt < 0)
    {
        cnt = 0;
    }
    if (cnt > MAX_BT_DEVICE_COUNT)
    {
        cnt = MAX_BT_DEVICE_COUNT;
    }
    right_linked = divoom_tws_host_is_right_linked();
    /* 右声道不操作手机：不放断开、取消配对。自动连接、名称、状态、密码和设备列表仍在。 */
    fi = (right_linked ? 2 : 4) + cnt;
    if (fi > DIVOOM_ASTRO_SET_BT_MAX_ROW)
    {
        fi = DIVOOM_ASTRO_SET_BT_MAX_ROW;
    }
    phandle->direct_handle = divoom_menu_direct_ctrl_create(1, fi);
    divoom_astrootoo_settings_create_header(phandle, "Bluetooth");
    phandle->content = divoom_astrootoo_ui_create_scroll_content(phandle->root_obj, &phandle->ui);

    y = DIVOOM_ASTRO_SET_TOP_GAP;
    idx = 0;
    card = divoom_astrootoo_settings_bt_add_row(phandle, idx++, y, DIVOOM_ASTRO_SET_BT_KIND_TOGGLE,
        "BTAutoConnect", NULL, 1);
    if (card)
    {
        lv_obj_add_event_cb(card, divoom_astrootoo_settings_bt_key_cb, LV_EVENT_KEY, phandle);
    }
    y += DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROW_GAP;

    card = divoom_astrootoo_settings_bt_add_row(phandle, idx++, y, DIVOOM_ASTRO_SET_BT_KIND_NAME,
        "BTDeviceName", "—", 0);
    y += DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROW_GAP;

    card = divoom_astrootoo_settings_bt_add_row(phandle, idx++, y, DIVOOM_ASTRO_SET_BT_KIND_STATUS,
        "ConnectionStatus", "—", 0);
    y += DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROW_GAP;

    card = divoom_astrootoo_settings_bt_add_row(phandle, idx++, y, DIVOOM_ASTRO_SET_BT_KIND_LINK_PASSWORD,
        "BTPassword", NULL, 1);
    if (card)
    {
        lv_obj_add_event_cb(card, divoom_astrootoo_settings_bt_key_cb, LV_EVENT_KEY, phandle);
    }
    y += DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROW_GAP;

    if (!right_linked)
    {
        card = divoom_astrootoo_settings_bt_add_row(phandle, idx++, y, DIVOOM_ASTRO_SET_BT_KIND_DISCONNECT,
            "DisConnectBT", NULL, 1);
        if (card)
        {
            lv_obj_add_event_cb(card, divoom_astrootoo_settings_bt_key_cb, LV_EVENT_KEY, phandle);
        }
        y += DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROW_GAP;

        card = divoom_astrootoo_settings_bt_add_row(phandle, idx++, y, DIVOOM_ASTRO_SET_BT_KIND_UNPAIR,
            "UnpairBT", NULL, 1);
        if (card)
        {
            lv_obj_add_event_cb(card, divoom_astrootoo_settings_bt_key_cb, LV_EVENT_KEY, phandle);
        }
        y += DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROW_GAP;
    }

    divoom_astrootoo_settings_bt_add_row(phandle, idx++, y, DIVOOM_ASTRO_SET_BT_KIND_SECTION,
        "ConnectedDevices", NULL, 0);
    y += DIVOOM_ASTRO_SET_SEC_H;

    phandle->bt_row_count = idx;
    divoom_astrootoo_settings_bt_rebuild_device_rows(phandle);
    phandle->bt_timer = lv_timer_create(divoom_astrootoo_settings_bt_timer_cb, 400, phandle);
    if (phandle->bt_timer)
    {
        lv_timer_set_repeat_count(phandle->bt_timer, -1);
    }
    divoom_astrootoo_settings_group_bind_bt(phandle, phandle->bt_row[0]);
}

/* ====================================================================== */
/* 通用 picker 子页 + 根页确认框                                            */
/* ====================================================================== */
static int g_eq_preset_idx;

/* 关闭确认弹窗：删除遮罩（连同卡片/按钮），并把焦点还给列表卡片 */
static void divoom_astrootoo_settings_confirm_close(Pdivoom_astrootoo_settings_ctrl phandle)
{
    if (!phandle || !phandle->confirm_overlay)
    {
        return;
    }
    divoom_astrootoo_settings_group_remove_obj_safe(phandle->confirm_cancel_btn);
    divoom_astrootoo_settings_group_remove_obj_safe(phandle->confirm_ok_btn);
    lv_obj_del(phandle->confirm_overlay);
    phandle->confirm_overlay = NULL;
    phandle->confirm_beta_body = NULL;
    phandle->confirm_cancel_btn = NULL;
    phandle->confirm_ok_btn = NULL;
    if (phandle->confirm_focus_restore)
    {
        lv_group_focus_obj(phandle->confirm_focus_restore);
        phandle->confirm_focus_restore = NULL;
    }
}

/* 刷新双钮外观（真源 FlowtooConfirmModal：取消聚焦琥珀，确定危险红） */
static void divoom_astrootoo_settings_confirm_refresh(Pdivoom_astrootoo_settings_ctrl phandle)
{
    int ok_focus;
    int ordinary_action;
    lv_obj_t *ok_lbl;

    if (!phandle->confirm_cancel_btn || !phandle->confirm_ok_btn)
    {
        return;
    }
    ok_focus = (phandle->confirm_focus == 1);
    ordinary_action = phandle->confirm_action == DIVOOM_ASTRO_SET_ACTION_UPGRADE ||
        phandle->confirm_action == DIVOOM_ASTRO_SET_ACTION_BETA_FEATURES;

    /* 取消钮：聚焦=琥珀底，否则浅灰底；墨色字 */
    lv_obj_set_style_bg_color(phandle->confirm_cancel_btn,
        lv_color_hex(ok_focus ? DIVOOM_ASTRO_SURFACE_IDLE : DIVOOM_ASTRO_SURFACE_FOCUS), 0);

    /* 升级是普通确认；重置/清储存才使用危险红色。 */
    if (ordinary_action)
    {
        lv_obj_set_style_bg_opa(phandle->confirm_ok_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(phandle->confirm_ok_btn,
            lv_color_hex(ok_focus ? DIVOOM_ASTRO_SURFACE_FOCUS : DIVOOM_ASTRO_SURFACE_IDLE), 0);
        lv_obj_set_style_border_width(phandle->confirm_ok_btn, 2, 0);
        lv_obj_set_style_border_color(phandle->confirm_ok_btn,
            lv_color_hex(ok_focus ? DIVOOM_ASTRO_TAB_INK : DIVOOM_ASTRO_STROKE), 0);
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
    ok_lbl = lv_obj_get_child(phandle->confirm_ok_btn, 0);
    if (ok_lbl)
    {
        lv_obj_set_style_text_color(ok_lbl,
            lv_color_hex(ordinary_action ? DIVOOM_ASTRO_INK
                : (ok_focus ? DIVOOM_ASTRO_BG : DIVOOM_ASTRO_DANGER)), 0);
    }

    lv_group_focus_obj(ok_focus ? phandle->confirm_ok_btn : phandle->confirm_cancel_btn);
    divoom_astrootoo_ui_confirm_modal_sync_btn_labels(phandle->confirm_cancel_btn,
        phandle->confirm_ok_btn, ok_focus);
}

/* 确认（确定钮）：先关弹窗，再下发对应主控事件 */
static void divoom_astrootoo_settings_confirm_commit(Pdivoom_astrootoo_settings_ctrl phandle)
{
    int action;

    action = phandle->confirm_action;
    /* 确定后回到列表无需还原焦点高亮（操作会触发重启/重置），但仍安全关闭 */
    divoom_astrootoo_settings_confirm_close(phandle);
    if (action == DIVOOM_ASTRO_SET_ACTION_RESET_NET)
    {
        divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_CONFIG_NET_WORK);
    }
    else if (action == DIVOOM_ASTRO_SET_ACTION_CLEAR_STORAGE)
    {
        /* 先上等待页，页刷完后再 push RESET_FACTORY 做 mkfs */
        divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_RESETING_DEVICE_MENU, NULL, 0, 0);
    }
    else if (action == DIVOOM_ASTRO_SET_ACTION_BETA_FEATURES)
    {
        divoom_main_set_beta_features(1);
    }
    else if (action == DIVOOM_ASTRO_SET_ACTION_UPGRADE)
    {
        /* 统一升级确认：timer 先启动文件更新，成功后再由 worker 发起固件升级。 */
        phandle->about_pending_action = DIVOOM_ASTRO_SET_ACTION_START_UPDATE;
        lv_timer_ready(phandle->about_update_timer);
    }
}

/* 弹窗按键：←→/上下 切焦点，Enter 触发当前钮，Esc 取消（真源 FlowtooConfirmModal keydown） */
static void divoom_astrootoo_settings_confirm_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    uint8_t key_code;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    phandle = (Pdivoom_astrootoo_settings_ctrl)lv_event_get_user_data(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_astrootoo_settings_confirm_close(phandle);
        return;
    }
    if (phandle->confirm_beta_body &&
        (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code)))
    {
        int32 offset = divoom_astrootoo_ui_key_is_up(key_code) ? -64 : 64;
        lv_obj_scroll_to_y(phandle->confirm_beta_body,
            lv_obj_get_scroll_y(phandle->confirm_beta_body) + offset, LV_ANIM_OFF);
        return;
    }
    if (divoom_astrootoo_ui_key_is_left(key_code) || divoom_astrootoo_ui_key_is_right(key_code)
        || divoom_astrootoo_ui_key_is_prev(key_code) || divoom_astrootoo_ui_key_is_next(key_code))
    {
        phandle->confirm_focus = (phandle->confirm_focus == 0) ? 1 : 0;
        divoom_astrootoo_settings_confirm_refresh(phandle);
        return;
    }
    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        if (phandle->confirm_focus == 1)
        {
            divoom_astrootoo_settings_confirm_commit(phandle);
        }
        else
        {
            divoom_astrootoo_settings_confirm_close(phandle);
        }
    }
}

static lv_obj_t *divoom_astrootoo_settings_confirm_make_btn(Pdivoom_astrootoo_settings_ctrl phandle,
    lv_obj_t *parent, const char *label_key)
{
    return divoom_astrootoo_ui_confirm_modal_make_btn(parent,
        divoom_menu_control_get_disp_str(label_key), phandle->menu_id, 0,
        divoom_astrootoo_settings_confirm_key_cb, phandle, 1);
}

/* 新版本确认：真源 FlowtooConfirmModal layout=fullBleed（暖色满屏 + H2 顶栏） */
static void divoom_astrootoo_settings_root_confirm_full(
    Pdivoom_astrootoo_settings_ctrl phandle, lv_obj_t *frame, const char *body_text)
{
    lv_obj_t *body_cont;
    lv_obj_t *body;
    lv_obj_t *actions;
    lv_obj_t *cancel_lbl;
    lv_coord_t actions_y;
    lv_coord_t body_h;
    lv_coord_t body_w;
    lv_coord_t line_space;

    actions_y = DIVOOM_SCREEN_HIGH - DIVOOM_ASTRO_CONFIRM_MODAL_FULL_BLEED_ACTIONS_BOTTOM
        - DIVOOM_ASTRO_CONFIRM_MODAL_BTN_MIN_H;
    body_w = DIVOOM_SCREEN_WIDTH - DIVOOM_ASTRO_CONFIRM_MODAL_FULL_BLEED_PAD_X * 2;
    if (phandle->confirm_action == DIVOOM_ASTRO_SET_ACTION_BETA_FEATURES)
    {
        /* Beta 说明复用满屏确认框，正文可滚动，左右键仍选择取消或确认。 */
        divoom_astrootoo_ui_create_page_header_tab(frame, &phandle->ui, "BetaFeatures",
            DIVOOM_ASTRO_SUBPAGE_HEADER_IMG, phandle->menu_id);
        body_cont = lv_obj_create(frame);
        lv_obj_remove_style_all(body_cont);
        lv_obj_set_style_border_width(body_cont, 0, 0);
        lv_obj_set_style_pad_all(body_cont, 0, 0);
        lv_obj_set_pos(body_cont, 24, DIVOOM_ASTRO_HEADER_H + 24);
        lv_obj_set_size(body_cont, DIVOOM_SCREEN_WIDTH - 48, actions_y - DIVOOM_ASTRO_HEADER_H - 40);
        lv_obj_add_flag(body_cont, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scroll_dir(body_cont, LV_DIR_VER);
        lv_obj_set_scrollbar_mode(body_cont, LV_SCROLLBAR_MODE_AUTO);
        phandle->confirm_beta_body = body_cont;

        body = lv_label_create(body_cont);
        lv_obj_set_width(body, LV_PCT(100));
        lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
        const lv_font_t *font = divoom_menu_control_get_font_info(20, phandle->menu_id);
        lv_obj_set_style_text_font(body, font, 0);
        lv_obj_set_style_text_color(body, lv_color_hex(DIVOOM_ASTRO_INK), 0);
        lv_obj_set_style_text_line_space(body, 32 - lv_font_get_line_height(font), 0);
        lv_label_set_text(body, divoom_menu_control_get_disp_str("BetaFeaturesConfirmBody"));
    }
    else
    {
        divoom_astrootoo_ui_create_page_header_tab(frame, &phandle->ui, "DeviceUpdate",
            DIVOOM_ASTRO_SUBPAGE_HEADER_IMG, phandle->menu_id);
        body_h = actions_y - DIVOOM_ASTRO_HEADER_H - DIVOOM_ASTRO_CONFIRM_MODAL_FULL_BLEED_CONTENT_GAP;
        line_space = DIVOOM_ASTRO_CONFIRM_MODAL_FULL_BLEED_BODY_LINE_H
            - DIVOOM_ASTRO_CONFIRM_MODAL_FULL_BLEED_BODY_FONT;
        if (line_space < 0)
        {
            line_space = 0;
        }

        body_cont = lv_obj_create(frame);
        lv_obj_remove_style_all(body_cont);
        lv_obj_set_style_border_width(body_cont, 0, 0);
        lv_obj_set_style_pad_all(body_cont, 0, 0);
        lv_obj_set_pos(body_cont, DIVOOM_ASTRO_CONFIRM_MODAL_FULL_BLEED_PAD_X, DIVOOM_ASTRO_HEADER_H);
        lv_obj_set_size(body_cont, body_w, body_h);
        lv_obj_set_style_bg_opa(body_cont, LV_OPA_TRANSP, 0);
        lv_obj_set_flex_flow(body_cont, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(body_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(body_cont, DIVOOM_ASTRO_CONFIRM_MODAL_FULL_BLEED_BODY_GAP, 0);
        lv_obj_set_scrollbar_mode(body_cont, LV_SCROLLBAR_MODE_AUTO);

        body = lv_label_create(body_cont);
        lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
        lv_label_set_text(body, body_text ? body_text : "");
        lv_obj_set_width(body, body_w);
        lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(body, lv_color_hex(DIVOOM_ASTRO_INK), 0);
        lv_obj_set_style_text_font(body,
            divoom_menu_control_get_font_info(DIVOOM_ASTRO_CONFIRM_MODAL_FULL_BLEED_BODY_FONT,
                phandle->menu_id), 0);
        lv_obj_set_style_text_line_space(body, line_space, 0);
    }

    actions = lv_obj_create(frame);
    lv_obj_remove_style_all(actions);
    lv_obj_set_style_border_width(actions, 0, 0);
    lv_obj_set_style_pad_all(actions, 0, 0);
    lv_obj_set_pos(actions, DIVOOM_ASTRO_CONFIRM_MODAL_FULL_BLEED_PAD_X, actions_y);
    lv_obj_set_size(actions, body_w, DIVOOM_ASTRO_CONFIRM_MODAL_BTN_MIN_H);
    lv_obj_clear_flag(actions, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(actions, DIVOOM_ASTRO_CONFIRM_MODAL_FULL_BLEED_ACTIONS_GAP, 0);

    phandle->confirm_cancel_btn = divoom_astrootoo_settings_confirm_make_btn(phandle, actions, "Cancel");
    phandle->confirm_ok_btn = divoom_astrootoo_settings_confirm_make_btn(phandle, actions, "Confirm");

    cancel_lbl = lv_obj_get_child(phandle->confirm_cancel_btn, 0);
    if (cancel_lbl)
    {
        lv_obj_set_style_text_color(cancel_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    }
}

/* 系统管理确认弹窗（真源 FlowtooConfirmModal + flowtooModalOverlay/Alert CSS）：
   透明 overlay + shell/frame 双层卡片（真源 FlowtooConfirmModal；F101 不做 ink dim）
   升级走 fullBleed 满屏，其它动作仍用居中卡片。 */
static void divoom_astrootoo_settings_root_confirm(Pdivoom_astrootoo_settings_ctrl phandle, int action_id)
{
    lv_obj_t *frame;
    lv_obj_t *title;
    lv_obj_t *body;
    lv_obj_t *actions;
    lv_obj_t *cancel_lbl;
    const char *title_key;
    const char *body_key;
    char update_body[512];

    if (!phandle || phandle->confirm_overlay)
    {
        return;
    }
    phandle->confirm_action = action_id;
    phandle->confirm_focus = 0;
    phandle->confirm_focus_restore = lv_group_get_focused(lv_group_get_default());

    if (action_id == DIVOOM_ASTRO_SET_ACTION_BETA_FEATURES)
    {
        phandle->confirm_overlay = divoom_astrootoo_ui_confirm_modal_create_full_bleed(phandle->root_obj, &frame);
        if (phandle->confirm_overlay && frame)
        {
            divoom_astrootoo_settings_root_confirm_full(phandle, frame, NULL);
            divoom_astrootoo_settings_confirm_refresh(phandle);
        }
        return;
    }
    if (action_id == DIVOOM_ASTRO_SET_ACTION_UPGRADE)
    {
        update_body[0] = '\0';
        if (phandle->about_firmware_version > DIVOOM_SOFTWARE_VERSION)
        {
            divoom_main_get_device_last_version_update_info(update_body);
            update_body[sizeof(update_body) - 1] = '\0';
        }
        if (update_body[0] == '\0')
        {
            snprintf(update_body, sizeof(update_body), "%s",
                divoom_menu_control_get_disp_str("DownloadUpdate"));
        }
        phandle->confirm_overlay = divoom_astrootoo_ui_confirm_modal_create_full_bleed(
            phandle->root_obj, &frame);
        if (!phandle->confirm_overlay || !frame)
        {
            return;
        }
        divoom_astrootoo_settings_root_confirm_full(phandle, frame, update_body);
        divoom_astrootoo_settings_confirm_refresh(phandle);
        return;
    }

    phandle->confirm_overlay = divoom_astrootoo_ui_confirm_modal_create(
        phandle->root_obj, &frame);
    if (!phandle->confirm_overlay || !frame)
    {
        return;
    }

    if (action_id == DIVOOM_ASTRO_SET_ACTION_RESET_NET)
    {
        title_key = "ResetWifiConfig";
        body_key = "ResetNetworkConfirmBody";
    }
    else
    {
        title_key = "ClearStorageConfirmTitle";
        body_key = "ClearStorageConfirmBody";
    }

    /* 标题 + 琥珀下划线 */
    title = lv_label_create(frame);
    lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP);
    lv_label_set_text(title, divoom_menu_control_get_disp_str(title_key));
    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(30, phandle->menu_id), 0);
    lv_obj_set_style_pad_bottom(title, 10, 0);
    lv_obj_set_style_border_side(title, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(title, 2, 0);
    lv_obj_set_style_border_color(title, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);

    /* 正文（自动换行，灰字） */
    body = lv_label_create(frame);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_label_set_text(body, divoom_menu_control_get_disp_str(body_key));
    lv_obj_set_width(body, LV_PCT(100));
    lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(body, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(body, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);

    /* 双钮行：取消(左) / 确定(右) */
    actions = lv_obj_create(frame);
    lv_obj_remove_style_all(actions);
    lv_obj_set_width(actions, LV_PCT(100));
    lv_obj_set_height(actions, LV_SIZE_CONTENT);
    lv_obj_clear_flag(actions, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(actions, 8, 0);

    phandle->confirm_cancel_btn = divoom_astrootoo_settings_confirm_make_btn(phandle, actions, "Cancel");
    phandle->confirm_ok_btn = divoom_astrootoo_settings_confirm_make_btn(phandle, actions, "Confirm");

    cancel_lbl = lv_obj_get_child(phandle->confirm_cancel_btn, 0);
    if (cancel_lbl)
    {
        lv_obj_set_style_text_color(cancel_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    }

    divoom_astrootoo_ui_confirm_modal_layout(frame);
    divoom_astrootoo_settings_confirm_refresh(phandle);
}

static void divoom_astrootoo_settings_picker_refresh(Pdivoom_astrootoo_settings_ctrl phandle)
{
    int i;

    for (i = 0; i < phandle->picker_cnt; i++)
    {
        if (!phandle->picker_check[i])
        {
            continue;
        }
        if (i == phandle->picker_committed)
        {
            lv_obj_clear_flag(phandle->picker_check[i], LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(phandle->picker_check[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static int divoom_astrootoo_settings_key_light_picker_committed(void)
{
    uint8 mode;
    uint8 on_off;
    uint8 bright;

    divoom_main_get_key_light(&mode, &on_off, &bright);
    if (!on_off)
    {
        return DIVOOM_ASTRO_KEY_LIGHT_PICKER_OFF_IDX;
    }
    if (mode >= DIVOOM_BTCORE_KEY_LIGHT_MODE_MAX)
    {
        return DIVOOM_ASTRO_KEY_LIGHT_PICKER_OFF_IDX + 1;
    }
    return (int)mode + 1;
}

static void divoom_astrootoo_settings_key_light_apply_brightness(int bright)
{
    uint8 mode;
    uint8 on_off;
    uint8 cur_bright;

    if (bright < 0)
    {
        bright = 0;
    }
    else if (bright > 100)
    {
        bright = 100;
    }
    divoom_main_get_key_light(&mode, &on_off, &cur_bright);
    divoom_main_set_key_light(mode, on_off, (uint8)bright);
    divoom_btcore_key_light_set_brightness((uint8)bright);
}

static void divoom_astrootoo_settings_key_light_apply_effect(int pos)
{
    uint8 mode;
    uint8 on_off;
    uint8 bright;

    if (pos < 0 || pos >= DIVOOM_ASTRO_KEY_LIGHT_PICKER_CNT)
    {
        return;
    }
    divoom_main_get_key_light(&mode, &on_off, &bright);
    if (pos == DIVOOM_ASTRO_KEY_LIGHT_PICKER_OFF_IDX)
    {
        divoom_main_set_key_light(mode, 0u, bright);
        divoom_btcore_key_light_off();
        return;
    }
    mode = (uint8)(pos - 1);
    if (mode >= DIVOOM_BTCORE_KEY_LIGHT_MODE_MAX)
    {
        return;
    }
    divoom_main_set_key_light(mode, 1u, bright);
    divoom_btcore_key_light_set_full(mode, 1u, bright);
}

static void divoom_astrootoo_settings_key_light_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    lv_obj_t *target;
    lv_obj_t *next;
    uint8_t key_code;
    int pos;
    int v;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    phandle = (Pdivoom_astrootoo_settings_ctrl)lv_event_get_user_data(e);
    target = lv_event_get_target(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_menu_control_return_pre();
        return;
    }
    if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code))
    {
        int direct;

        direct = divoom_astrootoo_ui_key_is_up(key_code) ? DIVOOM_MENU_DIRECT_UP : DIVOOM_MENU_DIRECT_DOWN;
        next = divoom_menu_direct_ctrl_get(phandle->direct_handle, target, direct);
        if (next && next != target)
        {
            lv_group_focus_obj(next);
            divoom_astrootoo_ui_focus_scroll_keep_section_title(next);
        }
        return;
    }
    if (target == phandle->key_light_bright_card)
    {
        if (divoom_astrootoo_ui_key_is_left(key_code))
        {
            v = phandle->key_light_bright - 5;
            if (v < 0)
            {
                v = 0;
            }
            phandle->key_light_bright = v;
            divoom_astrootoo_settings_key_light_apply_brightness(v);
            if (phandle->key_light_bright_slider)
            {
                lv_slider_set_value(phandle->key_light_bright_slider, v, LV_ANIM_OFF);
            }
            return;
        }
        if (divoom_astrootoo_ui_key_is_right(key_code))
        {
            v = phandle->key_light_bright + 5;
            if (v > 100)
            {
                v = 100;
            }
            phandle->key_light_bright = v;
            divoom_astrootoo_settings_key_light_apply_brightness(v);
            if (phandle->key_light_bright_slider)
            {
                lv_slider_set_value(phandle->key_light_bright_slider, v, LV_ANIM_OFF);
            }
            return;
        }
        if (divoom_astrootoo_ui_key_is_enter(key_code))
        {
            return;
        }
    }
    if (!divoom_astrootoo_ui_key_is_enter(key_code))
    {
        return;
    }
    pos = divoom_menu_direct_ctrl_get_pos(phandle->direct_handle, target);
    if (pos <= 0 || pos > phandle->picker_cnt)
    {
        return;
    }
    divoom_astrootoo_settings_key_light_apply_effect(pos - 1);
    phandle->picker_committed = pos - 1;
    divoom_astrootoo_settings_picker_refresh(phandle);
}

static void divoom_astrootoo_settings_picker_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    lv_obj_t *target;
    lv_obj_t *next;
    uint8_t key_code;
    int pos;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    phandle = (Pdivoom_astrootoo_settings_ctrl)lv_event_get_user_data(e);
    target = lv_event_get_target(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_menu_control_return_pre();
        return;
    }
    if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code))
    {
        int direct;

        direct = divoom_astrootoo_ui_key_is_up(key_code) ? DIVOOM_MENU_DIRECT_UP : DIVOOM_MENU_DIRECT_DOWN;
        next = divoom_menu_direct_ctrl_get(phandle->direct_handle, target, direct);
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
    if (pos < 0 || pos >= phandle->picker_cnt)
    {
        return;
    }
    switch (phandle->page)
    {
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_EQUALIZER:
    {
        Pdivoom_sys_setting_info cfg;

        g_eq_preset_idx = pos;
        cfg = divoom_main_get_system_config();
        if (cfg)
        {
            cfg->audio_effect = (uint8)pos;
            divoom_btcore_uart_cmd_apply_sound_effect(cfg->audio_effect);
            divoom_astrootoo_settings_sync_sys_conf();
        }
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_IDLE_SHUTDOWN:
    {
        Pdivoom_sys_setting_info cfg;

        cfg = divoom_main_get_system_config();
        if (cfg && pos >= 0 && pos < DIVOOM_IDLE_POWEROFF_OPTION_CNT)
        {
            cfg->idle_poweroff_time = divoom_idle_poweroff_secs[pos];
            divoom_main_save_config();
        }
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_WIND_UNIT:
    {
        divoom_main_set_wind_speed_flag((uint8)pos);
        divoom_astrootoo_settings_sync_sys_conf();
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_TEMP_UNIT:
    {
        divoom_main_set_temp_flag(pos);
        divoom_astrootoo_settings_sync_sys_conf();
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_FORMAT:
    {
        divoom_main_set_time_format_type((uint8)pos);
        divoom_astrootoo_settings_sync_sys_conf();
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_SLIDESHOW_PHOTO_ORDER:
    {
        divoom_astrootoo_settings_slideshow_picker_apply(phandle, pos);
        break;
    }
    default:
    {
        break;
    }
    }
    phandle->picker_committed = pos;
    divoom_astrootoo_settings_picker_refresh(phandle);
    if (phandle->page != DIVOOM_ASTROTOO_SETTINGS_PAGE_EQUALIZER)
    {
        divoom_menu_control_return_pre();
    }
}

static int divoom_astrootoo_settings_idle_committed(void)
{
    Pdivoom_sys_setting_info cfg;
    int i;

    cfg = divoom_main_get_system_config();
    if (!cfg)
    {
        return DIVOOM_IDLE_POWEROFF_OPTION_CNT - 1;
    }
    for (i = 0; i < DIVOOM_IDLE_POWEROFF_OPTION_CNT; i++)
    {
        if (cfg->idle_poweroff_time == divoom_idle_poweroff_secs[i])
        {
            return i;
        }
    }
    return DIVOOM_IDLE_POWEROFF_OPTION_CNT - 1;
}

static void divoom_astrootoo_settings_build_picker(Pdivoom_astrootoo_settings_ctrl phandle,
    const char *title_key, const char *const *option_keys, int count, int committed)
{
    lv_obj_t *card;
    lv_obj_t *lbl;
    lv_obj_t *chk;
    lv_coord_t y;
    int i;

    if (count > DIVOOM_ASTRO_SET_PICKER_MAX)
    {
        count = DIVOOM_ASTRO_SET_PICKER_MAX;
    }
    phandle->picker_cnt = count;
    phandle->picker_committed = committed;
    phandle->direct_handle = divoom_menu_direct_ctrl_create(1, count);
    divoom_astrootoo_settings_create_header(phandle, title_key);
    phandle->content = divoom_astrootoo_ui_create_scroll_content(phandle->root_obj, &phandle->ui);

    y = DIVOOM_ASTRO_SET_TOP_GAP;
    for (i = 0; i < count; i++)
    {
        card = lv_btn_create(phandle->content);
        lv_obj_add_style(card, &phandle->ui.card_item_style, 0);
        lv_obj_add_style(card, &phandle->ui.card_item_focus_style, LV_STATE_FOCUSED);
        lv_obj_add_style(card, &phandle->ui.card_item_focus_style, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUS_KEY);
        lv_obj_set_size(card, DIVOOM_ASTRO_SET_ROW_W, DIVOOM_ASTRO_SET_ROW_H);
        lv_obj_set_pos(card, DIVOOM_ASTRO_SET_ROW_X, y);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        divoom_menu_direct_ctrl_update(phandle->direct_handle, card, 0, i);
        lv_obj_add_event_cb(card, divoom_astrootoo_settings_picker_key_cb, LV_EVENT_KEY, phandle);

        lbl = lv_label_create(card);
        lv_label_set_text(lbl, divoom_menu_control_get_disp_str(option_keys[i]));
        lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
        lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 8, 0);
        divoom_astrootoo_ui_list_label_prepare(lbl,
            divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_SET_ROW_W,
                DIVOOM_ASTRO_SET_CARD_LABEL_INSET_L, DIVOOM_ASTRO_SET_CARD_LABEL_INSET_R,
                DIVOOM_ASTRO_LIST_CHK_TRAIL_CARD));
        divoom_astrootoo_ui_list_row_bind_marquee(card, lbl);

        chk = lv_img_create(card);
        divoom_menu_control_lv_img_set_src(chk, DIVOOM_ASTRO_SET_CHECK_ICON);
        lv_obj_align(chk, LV_ALIGN_RIGHT_MID, -16, 0);
        lv_obj_add_flag(chk, LV_OBJ_FLAG_HIDDEN);

        phandle->picker_row[i] = card;
        phandle->picker_check[i] = chk;
        y += DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROW_GAP;
    }
    divoom_astrootoo_settings_picker_refresh(phandle);
}

static void divoom_astrootoo_settings_build_key_light_effects(Pdivoom_astrootoo_settings_ctrl phandle)
{
    static const char *key_light_keys[] = {
        "KeyLightEffectNone",
        "KeyLightEffect1", "KeyLightEffect2", "KeyLightEffect3", "KeyLightEffect4",
        "KeyLightEffect5", "KeyLightEffect6", "KeyLightEffect7", "KeyLightEffect8",
    };
    lv_obj_t *card;
    lv_obj_t *lbl;
    lv_obj_t *chk;
    lv_coord_t y;
    lv_coord_t sy;
    int i;
    int count;
    int committed;
    uint8 mode;
    uint8 on_off;
    uint8 bright;

    count = DIVOOM_ASTRO_KEY_LIGHT_PICKER_CNT;
    if (count > DIVOOM_ASTRO_SET_PICKER_MAX)
    {
        count = DIVOOM_ASTRO_SET_PICKER_MAX;
    }
    committed = divoom_astrootoo_settings_key_light_picker_committed();
    divoom_main_get_key_light(&mode, &on_off, &bright);
    phandle->key_light_bright = (int)bright;
    phandle->picker_cnt = count;
    phandle->picker_committed = committed;
    phandle->direct_handle = divoom_menu_direct_ctrl_create(1, DIVOOM_ASTRO_KEY_LIGHT_ROW_CNT);
    divoom_astrootoo_settings_create_header(phandle, "KeyLightEffects");
    phandle->content = divoom_astrootoo_ui_create_scroll_content(phandle->root_obj, &phandle->ui);

    y = DIVOOM_ASTRO_SET_TOP_GAP;
    phandle->key_light_sec_bright = lv_obj_create(phandle->content);
    lv_obj_add_style(phandle->key_light_sec_bright, &phandle->ui.content_style, 0);
    lv_obj_set_size(phandle->key_light_sec_bright, DIVOOM_ASTRO_SET_ROW_W,
        DIVOOM_ASTRO_NIGHT_SEC_TITLE_Y + DIVOOM_ASTRO_NIGHT_SEC_TITLE_H
        + DIVOOM_ASTRO_NIGHT_SEC_GAP + DIVOOM_ASTRO_SET_ROW_H + 10);
    lv_obj_set_pos(phandle->key_light_sec_bright, DIVOOM_ASTRO_SET_ROW_X, y);
    lv_obj_clear_flag(phandle->key_light_sec_bright, LV_OBJ_FLAG_SCROLLABLE);
    divoom_astrootoo_settings_night_section_title(phandle, phandle->key_light_sec_bright,
        "KeyLightBrightness", DIVOOM_ASTRO_NIGHT_SEC_TITLE_Y);
    sy = DIVOOM_ASTRO_NIGHT_SEC_TITLE_Y + DIVOOM_ASTRO_NIGHT_SEC_TITLE_H + DIVOOM_ASTRO_NIGHT_SEC_GAP + 6;
    card = lv_btn_create(phandle->key_light_sec_bright);
    lv_obj_add_style(card, &phandle->ui.card_item_style, 0);
    lv_obj_add_style(card, &phandle->ui.card_item_focus_style, LV_STATE_FOCUSED);
    lv_obj_add_style(card, &phandle->ui.card_item_focus_style, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_size(card, DIVOOM_ASTRO_SET_ROW_W, DIVOOM_ASTRO_SET_ROW_H);
    lv_obj_set_pos(card, 0, sy);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    divoom_menu_direct_ctrl_update(phandle->direct_handle, card, 0, 0);
    lv_obj_add_event_cb(card, divoom_astrootoo_settings_key_light_key_cb, LV_EVENT_KEY, phandle);
    divoom_astrootoo_settings_fill_slider_row(card, DIVOOM_ASTROTOO_SETTINGS_BRIGHT_ICO,
        phandle->key_light_bright, &phandle->key_light_bright_slider);
    phandle->key_light_bright_card = card;
    y += DIVOOM_ASTRO_NIGHT_SEC_TITLE_Y + DIVOOM_ASTRO_NIGHT_SEC_TITLE_H + DIVOOM_ASTRO_NIGHT_SEC_GAP
        + DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROW_GAP + 12;

    phandle->key_light_sec_effects = lv_obj_create(phandle->content);
    lv_obj_add_style(phandle->key_light_sec_effects, &phandle->ui.content_style, 0);
    lv_obj_set_size(phandle->key_light_sec_effects, DIVOOM_ASTRO_SET_ROW_W,
        DIVOOM_ASTRO_NIGHT_SEC_TITLE_Y + DIVOOM_ASTRO_NIGHT_SEC_TITLE_H + DIVOOM_ASTRO_NIGHT_SEC_GAP
        + (lv_coord_t)count * (DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROW_GAP) + 10);
    lv_obj_set_pos(phandle->key_light_sec_effects, DIVOOM_ASTRO_SET_ROW_X, y);
    lv_obj_clear_flag(phandle->key_light_sec_effects, LV_OBJ_FLAG_SCROLLABLE);
    divoom_astrootoo_settings_night_section_title(phandle, phandle->key_light_sec_effects,
        "KeyLightEffectsSection", DIVOOM_ASTRO_NIGHT_SEC_TITLE_Y);
    sy = DIVOOM_ASTRO_NIGHT_SEC_TITLE_Y + DIVOOM_ASTRO_NIGHT_SEC_TITLE_H + DIVOOM_ASTRO_NIGHT_SEC_GAP + 6;
    for (i = 0; i < count; i++)
    {
        card = lv_btn_create(phandle->key_light_sec_effects);
        lv_obj_add_style(card, &phandle->ui.card_item_style, 0);
        lv_obj_add_style(card, &phandle->ui.card_item_focus_style, LV_STATE_FOCUSED);
        lv_obj_add_style(card, &phandle->ui.card_item_focus_style, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUS_KEY);
        lv_obj_set_size(card, DIVOOM_ASTRO_SET_ROW_W, DIVOOM_ASTRO_SET_ROW_H);
        lv_obj_set_pos(card, 0, sy);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        divoom_menu_direct_ctrl_update(phandle->direct_handle, card, 0, i + 1);
        lv_obj_add_event_cb(card, divoom_astrootoo_settings_key_light_key_cb, LV_EVENT_KEY, phandle);

        lbl = lv_label_create(card);
        lv_label_set_text(lbl, divoom_menu_control_get_disp_str(key_light_keys[i]));
        lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
        lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 8, 0);
        divoom_astrootoo_ui_list_label_prepare(lbl,
            divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_SET_ROW_W,
                DIVOOM_ASTRO_SET_CARD_LABEL_INSET_L, DIVOOM_ASTRO_SET_CARD_LABEL_INSET_R,
                DIVOOM_ASTRO_LIST_CHK_TRAIL_CARD));
        divoom_astrootoo_ui_list_row_bind_marquee(card, lbl);

        chk = lv_img_create(card);
        divoom_menu_control_lv_img_set_src(chk, DIVOOM_ASTRO_SET_CHECK_ICON);
        lv_obj_align(chk, LV_ALIGN_RIGHT_MID, -16, 0);
        lv_obj_add_flag(chk, LV_OBJ_FLAG_HIDDEN);

        phandle->picker_row[i] = card;
        phandle->picker_check[i] = chk;
        sy += DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROW_GAP;
    }
    divoom_astrootoo_settings_picker_refresh(phandle);
}

static void divoom_astrootoo_settings_group_bind_key_light(Pdivoom_astrootoo_settings_ctrl phandle,
    lv_obj_t *focus_target)
{
    lv_group_t *g;
    int i;
    lv_obj_t *focus;

    if (!phandle)
    {
        return;
    }
    g = lv_group_get_default();
    lv_group_remove_all_objs(g);
    if (phandle->key_light_bright_card)
    {
        lv_group_add_obj(g, phandle->key_light_bright_card);
    }
    for (i = 0; i < phandle->picker_cnt; i++)
    {
        if (phandle->picker_row[i])
        {
            lv_group_add_obj(g, phandle->picker_row[i]);
        }
    }
    focus = NULL;
    if (focus_target && lv_obj_is_valid(focus_target))
    {
        focus = focus_target;
    }
    else if (phandle->picker_committed >= 0 && phandle->picker_committed < phandle->picker_cnt
        && phandle->picker_row[phandle->picker_committed])
    {
        focus = phandle->picker_row[phandle->picker_committed];
    }
    else if (phandle->key_light_bright_card)
    {
        focus = phandle->key_light_bright_card;
    }
    if (focus)
    {
        lv_group_focus_obj(focus);
        divoom_astrootoo_ui_focus_scroll_keep_section_title(focus);
    }
}

static void divoom_astrootoo_settings_group_bind_picker(Pdivoom_astrootoo_settings_ctrl phandle,
    lv_obj_t *focus_target)
{
    lv_group_t *g;
    int i;
    lv_obj_t *focus;

    if (!phandle || phandle->picker_cnt <= 0)
    {
        return;
    }
    g = lv_group_get_default();
    lv_group_remove_all_objs(g);
    for (i = 0; i < phandle->picker_cnt; i++)
    {
        if (phandle->picker_row[i])
        {
            lv_group_add_obj(g, phandle->picker_row[i]);
        }
    }
    focus = NULL;
    if (focus_target && lv_obj_is_valid(focus_target))
    {
        focus = focus_target;
    }
    else if (phandle->picker_committed >= 0 && phandle->picker_committed < phandle->picker_cnt
        && phandle->picker_row[phandle->picker_committed])
    {
        focus = phandle->picker_row[phandle->picker_committed];
    }
    else if (phandle->picker_row[0])
    {
        focus = phandle->picker_row[0];
    }
    if (focus)
    {
        lv_group_focus_obj(focus);
        divoom_astrootoo_ui_focus_scroll(focus);
    }
}

#if DIVOOM_ASTRO_MANUAL_DATETIME_SET_ENABLE
static int divoom_astrootoo_settings_dt_row_focusable(uint8 kind)
{
    if (kind == DIVOOM_ASTRO_SET_DT_KIND_TIME || kind == DIVOOM_ASTRO_SET_DT_KIND_DATE)
    {
        return g_dt_auto_set ? 0 : 1;
    }
    return 1;
}
#endif

static void divoom_astrootoo_settings_dt_read_clock(Pdivoom_astrootoo_settings_ctrl phandle)
{
    time_t now;
    struct tm tm_info;

    if (!phandle)
    {
        return;
    }
    now = time(NULL);
#if defined(_WIN32)
    if (localtime_s(&tm_info, &now) != 0)
    {
        return;
    }
#else
    if (localtime_r(&now, &tm_info) == NULL)
    {
        return;
    }
#endif
    phandle->dt_hour = tm_info.tm_hour;
    phandle->dt_minute = tm_info.tm_min;
    phandle->dt_year = tm_info.tm_year + 1900;
    phandle->dt_month = tm_info.tm_mon + 1;
    phandle->dt_day = tm_info.tm_mday;
}

static int divoom_astrootoo_settings_dt_days_in_month(int year, int month)
{
    static const int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int n;

    if (month < 1 || month > 12)
    {
        return 31;
    }
    n = days[month - 1];
    if (month == 2)
    {
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
        {
            n = 29;
        }
    }
    return n;
}

static void divoom_astrootoo_settings_dt_apply_clock(Pdivoom_astrootoo_settings_ctrl phandle, int time_only, int date_only)
{
    struct tm tm_info;
    time_t sec;
    struct timeval tv;

    if (!phandle)
    {
        return;
    }
    memset(&tm_info, 0, sizeof(tm_info));
    sec = time(NULL);
#if defined(_WIN32)
    localtime_s(&tm_info, &sec);
#else
    localtime_r(&sec, &tm_info);
#endif
    if (!date_only)
    {
        tm_info.tm_hour = phandle->dt_hour;
        tm_info.tm_min = phandle->dt_minute;
        tm_info.tm_sec = 0;
    }
    if (!time_only)
    {
        tm_info.tm_year = phandle->dt_year - 1900;
        tm_info.tm_mon = phandle->dt_month - 1;
        tm_info.tm_mday = phandle->dt_day;
    }
    sec = mktime(&tm_info);
    if (sec == (time_t)-1)
    {
        return;
    }
    tv.tv_sec = sec;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
    divoom_system_init_system_time();
    divoom_onoff_on_user_set_time();
}

static void divoom_astrootoo_settings_dt_apply_seg_style(Pdivoom_astrootoo_settings_ctrl phandle,
    lv_obj_t *box, int focused)
{
    if (!box || !phandle)
    {
        return;
    }
    if (focused)
    {
        lv_obj_set_style_bg_color(box, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
        lv_obj_set_style_border_color(box, lv_color_hex(0xB8B0A8), 0);
    }
    else
    {
        lv_obj_set_style_bg_color(box, lv_color_hex(DIVOOM_ASTRO_BG_MUTED), 0);
        lv_obj_set_style_border_color(box, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
    }
    lv_obj_set_style_border_width(box, 2, 0);
    lv_obj_set_style_radius(box, 12, 0);
    lv_obj_set_style_pad_all(box, 0, 0);
}

static void divoom_astrootoo_settings_dt_refresh_pick(Pdivoom_astrootoo_settings_ctrl phandle)
{
    char buf[16];
    int i;
    int seg_cnt;
    int focused;
    int use_hour24;

    if (!phandle)
    {
        return;
    }
    use_hour24 = divoom_astrootoo_time_pick_use_hour24();
    if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_TIME_TIME)
    {
        divoom_astrootoo_time_pick_refresh_widgets(&phandle->dt_tp_widgets,
            phandle->dt_pick_field, phandle->dt_hour, phandle->dt_minute, use_hour24);
        return;
    }
    seg_cnt = DIVOOM_ASTRO_DT_DATE_FIELDS;
    focused = phandle->dt_pick_field;
    if (focused < 0 || focused >= seg_cnt)
    {
        focused = 0;
        phandle->dt_pick_field = 0;
    }
    for (i = 0; i < seg_cnt; i++)
    {
        if (!phandle->dt_seg_box[i])
        {
            continue;
        }
        divoom_astrootoo_settings_dt_apply_seg_style(phandle, phandle->dt_seg_box[i], focused == i);
        if (!phandle->dt_seg_lbl[i])
        {
            continue;
        }
        if (i == 0)
        {
            lv_snprintf(buf, sizeof(buf), "%04d", phandle->dt_year);
            lv_label_set_text(phandle->dt_seg_lbl[i], buf);
        }
        else if (i == 1)
        {
            lv_snprintf(buf, sizeof(buf), "%02d", phandle->dt_month);
            lv_label_set_text(phandle->dt_seg_lbl[i], buf);
        }
        else
        {
            lv_snprintf(buf, sizeof(buf), "%02d", phandle->dt_day);
            lv_label_set_text(phandle->dt_seg_lbl[i], buf);
        }
        if (phandle->dt_seg_cap[i] && !lv_obj_has_flag(phandle->dt_seg_cap[i], LV_OBJ_FLAG_HIDDEN))
        {
            lv_obj_set_style_text_color(phandle->dt_seg_cap[i],
                lv_color_hex(focused == i ? DIVOOM_ASTRO_SURFACE_FOCUS : DIVOOM_ASTRO_INK_MUTED), 0);
        }
    }
}

static void divoom_astrootoo_settings_dt_pick_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    uint8_t key_code;
    int max_day;
    divoom_astrootoo_time_pick_state_t st;

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
    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_TIME_TIME)
    {
        divoom_astrootoo_time_pick_key_action_t action;

        st.use_hour24 = divoom_astrootoo_time_pick_use_hour24();
        st.field = phandle->dt_pick_field;
        st.hour24 = phandle->dt_hour;
        st.minute = phandle->dt_minute;
        action = divoom_astrootoo_time_pick_handle_key(&st, key_code, 0);
        phandle->dt_pick_field = st.field;
        phandle->dt_hour = st.hour24;
        phandle->dt_minute = st.minute;
        if (action == DIVOOM_ASTRO_TP_KEY_CANCEL)
        {
            divoom_menu_control_return_pre();
            return;
        }
        if (action == DIVOOM_ASTRO_TP_KEY_CONFIRM)
        {
            divoom_astrootoo_settings_dt_apply_clock(phandle, 1, 0);
            divoom_menu_control_return_pre();
            return;
        }
        divoom_astrootoo_settings_dt_refresh_pick(phandle);
        return;
    }

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_menu_control_return_pre();
        return;
    }
    if (divoom_astrootoo_ui_key_is_left(key_code))
    {
        phandle->dt_pick_field = (phandle->dt_pick_field + DIVOOM_ASTRO_DT_DATE_FIELDS - 1)
            % DIVOOM_ASTRO_DT_DATE_FIELDS;
        divoom_astrootoo_settings_dt_refresh_pick(phandle);
        return;
    }
    if (divoom_astrootoo_ui_key_is_right(key_code))
    {
        phandle->dt_pick_field = (phandle->dt_pick_field + 1) % DIVOOM_ASTRO_DT_DATE_FIELDS;
        divoom_astrootoo_settings_dt_refresh_pick(phandle);
        return;
    }
    if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code))
    {
        int delta;

        delta = divoom_astrootoo_ui_key_is_up(key_code) ? 1 : -1;
        if (phandle->dt_pick_field == 0)
        {
            phandle->dt_year += delta;
            if (phandle->dt_year < 2000)
            {
                phandle->dt_year = 2000;
            }
            if (phandle->dt_year > 2099)
            {
                phandle->dt_year = 2099;
            }
        }
        else if (phandle->dt_pick_field == 1)
        {
            phandle->dt_month = ((phandle->dt_month - 1 + delta + 12) % 12) + 1;
        }
        else
        {
            max_day = divoom_astrootoo_settings_dt_days_in_month(phandle->dt_year, phandle->dt_month);
            phandle->dt_day = ((phandle->dt_day - 1 + delta + max_day) % max_day) + 1;
        }
        max_day = divoom_astrootoo_settings_dt_days_in_month(phandle->dt_year, phandle->dt_month);
        if (phandle->dt_day > max_day)
        {
            phandle->dt_day = max_day;
        }
        divoom_astrootoo_settings_dt_refresh_pick(phandle);
        return;
    }
    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        divoom_astrootoo_settings_dt_apply_clock(phandle, 0, 1);
        divoom_menu_control_return_pre();
    }
}

static lv_obj_t *divoom_astrootoo_settings_dt_create_segment(Pdivoom_astrootoo_settings_ctrl phandle,
    lv_obj_t *parent, const char *cap_key, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h,
    int seg_idx, int font_digit)
{
    lv_obj_t *wrap;
    lv_obj_t *box;
    lv_obj_t *val;
    lv_obj_t *cap;

    wrap = lv_obj_create(parent);
    lv_obj_add_style(wrap, &phandle->ui.content_style, 0);
    lv_obj_set_style_bg_opa(wrap, LV_OPA_TRANSP, 0);
    lv_obj_set_size(wrap, w, h + 36);
    lv_obj_set_pos(wrap, x, y);
    lv_obj_clear_flag(wrap, LV_OBJ_FLAG_SCROLLABLE);

    box = lv_obj_create(wrap);
    divoom_astrootoo_settings_dt_apply_seg_style(phandle, box, 0);
    lv_obj_set_size(box, w, h);
    lv_obj_set_pos(box, 0, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    val = lv_label_create(box);
    lv_obj_set_style_text_font(val, divoom_menu_control_get_font_info(font_digit, phandle->menu_id), 0);
    lv_obj_set_style_text_color(val, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_label_set_text(val, "00");
    lv_obj_center(val);

    cap = lv_label_create(wrap);
    if (cap_key)
    {
        lv_label_set_text(cap, divoom_menu_control_get_disp_str(cap_key));
        lv_obj_set_style_text_font(cap, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
        lv_obj_set_style_text_color(cap, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
        lv_obj_align(cap, LV_ALIGN_BOTTOM_MID, 0, 0);
    }
    else
    {
        lv_obj_add_flag(cap, LV_OBJ_FLAG_HIDDEN);
    }

    phandle->dt_seg_box[seg_idx] = box;
    phandle->dt_seg_lbl[seg_idx] = val;
    phandle->dt_seg_cap[seg_idx] = cap;
    return wrap;
}

static void divoom_astrootoo_settings_build_dt_time_pick(Pdivoom_astrootoo_settings_ctrl phandle)
{
    int use_hour24;
    lv_coord_t stack_y;

    divoom_astrootoo_settings_dt_read_clock(phandle);
    use_hour24 = divoom_astrootoo_time_pick_use_hour24();
    phandle->dt_pick_field = divoom_astrootoo_time_pick_initial_field(use_hour24);
    memset(&phandle->dt_tp_widgets, 0, sizeof(phandle->dt_tp_widgets));

    divoom_astrootoo_settings_create_header(phandle, "Time");
    phandle->content = divoom_astrootoo_ui_create_scroll_content(phandle->root_obj, &phandle->ui);
    lv_obj_set_scrollbar_mode(phandle->content, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(phandle->content, LV_OBJ_FLAG_SCROLLABLE);

    stack_y = divoom_astrootoo_settings_time_pick_stack_y(DIVOOM_ASTRO_CONTENT_H);
    divoom_astrootoo_settings_create_time_pick_stack(phandle->content, phandle->menu_id,
        &phandle->ui.content_style, stack_y, &phandle->dt_tp_widgets);
    phandle->dt_pick_tray = phandle->dt_tp_widgets.tray;

    phandle->dt_key_btn = divoom_astrootoo_ui_create_key_capture_btn(phandle->root_obj, &phandle->ui,
        divoom_astrootoo_settings_dt_pick_key_cb, phandle);
    divoom_astrootoo_settings_dt_refresh_pick(phandle);
}

static void divoom_astrootoo_settings_build_dt_date_pick(Pdivoom_astrootoo_settings_ctrl phandle)
{
    divoom_astrootoo_settings_dt_read_clock(phandle);
    phandle->dt_pick_field = 0;
    divoom_astrootoo_settings_create_header(phandle, "Date");
    phandle->content = divoom_astrootoo_ui_create_scroll_content(phandle->root_obj, &phandle->ui);
    lv_obj_set_scrollbar_mode(phandle->content, LV_SCROLLBAR_MODE_OFF);

    phandle->dt_pick_tray = lv_obj_create(phandle->content);
    lv_obj_add_style(phandle->dt_pick_tray, &phandle->ui.card_item_style, 0);
    lv_obj_set_size(phandle->dt_pick_tray, 448, 220);
    lv_obj_align(phandle->dt_pick_tray, LV_ALIGN_TOP_MID, 0, 55);
    lv_obj_clear_flag(phandle->dt_pick_tray, LV_OBJ_FLAG_SCROLLABLE);

    divoom_astrootoo_settings_dt_create_segment(phandle, phandle->dt_pick_tray, "DateFieldYear",
        8, 16, DIVOOM_ASTRO_DT_SEG_DATE_W, DIVOOM_ASTRO_DT_SEG_DATE_H, 0, 48);
    divoom_astrootoo_settings_dt_create_segment(phandle, phandle->dt_pick_tray, "DateFieldMonth",
        154, 16, DIVOOM_ASTRO_DT_SEG_DATE_W, DIVOOM_ASTRO_DT_SEG_DATE_H, 1, 48);
    divoom_astrootoo_settings_dt_create_segment(phandle, phandle->dt_pick_tray, "DateFieldDay",
        300, 16, DIVOOM_ASTRO_DT_SEG_DATE_W, DIVOOM_ASTRO_DT_SEG_DATE_H, 2, 48);

    phandle->dt_key_btn = divoom_astrootoo_ui_create_key_capture_btn(phandle->root_obj, &phandle->ui,
        divoom_astrootoo_settings_dt_pick_key_cb, phandle);
    divoom_astrootoo_settings_dt_refresh_pick(phandle);
}

#if DIVOOM_ASTRO_MANUAL_DATETIME_SET_ENABLE
static lv_obj_t *divoom_astrootoo_settings_dt_next_list_row(Pdivoom_astrootoo_settings_ctrl phandle,
    int from_idx, int delta)
{
    int i;
    int idx;

    if (!phandle || phandle->list_row_count <= 0)
    {
        return NULL;
    }
    idx = from_idx;
    for (i = 0; i < phandle->list_row_count; i++)
    {
        idx = (idx + delta + phandle->list_row_count) % phandle->list_row_count;
        if (divoom_astrootoo_settings_dt_row_focusable(phandle->list_row_kind[idx]))
        {
            return phandle->list_row[idx];
        }
    }
    return phandle->list_row[from_idx];
}

static void divoom_astrootoo_settings_dt_normalize_list_focus(Pdivoom_astrootoo_settings_ctrl phandle)
{
    lv_obj_t *focused;
    int i;

    if (!phandle || phandle->page != DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_TIME)
    {
        return;
    }
    focused = lv_group_get_focused(lv_group_get_default());
    for (i = 0; i < phandle->list_row_count; i++)
    {
        if (phandle->list_row[i] == focused
            && !divoom_astrootoo_settings_dt_row_focusable(phandle->list_row_kind[i]))
        {
            focused = divoom_astrootoo_settings_dt_next_list_row(phandle, i, 1);
            if (focused)
            {
                lv_group_focus_obj(focused);
                divoom_astrootoo_ui_focus_scroll(focused);
            }
            return;
        }
    }
}
#endif

static void divoom_astrootoo_settings_list_refresh_dt(Pdivoom_astrootoo_settings_ctrl phandle)
{
    Pdivoom_sys_setting_info cfg;
    int i;
#if DIVOOM_ASTRO_MANUAL_DATETIME_SET_ENABLE
    lv_opa_t row_opa;
    int disabled;
#endif

    cfg = divoom_main_get_system_config();
    for (i = 0; i < phandle->list_row_count; i++)
    {
        if (!phandle->list_row[i])
        {
            continue;
        }
        if (phandle->list_row_kind[i] == DIVOOM_ASTRO_SET_DT_KIND_HOUR24 && phandle->list_row_sw[i] && cfg)
        {
            divoom_menu_control_lv_img_set_src(phandle->list_row_sw[i],
                cfg->hour12_24 ? DIVOOM_ASTROTOO_SETTINGS_SWITCH_ON : DIVOOM_ASTROTOO_SETTINGS_SWITCH_OFF);
        }
#if DIVOOM_ASTRO_MANUAL_DATETIME_SET_ENABLE
        else if (phandle->list_row_kind[i] == DIVOOM_ASTRO_SET_DT_KIND_AUTO && phandle->list_row_sw[i])
        {
            divoom_menu_control_lv_img_set_src(phandle->list_row_sw[i],
                g_dt_auto_set ? DIVOOM_ASTROTOO_SETTINGS_SWITCH_ON : DIVOOM_ASTROTOO_SETTINGS_SWITCH_OFF);
        }
#endif
#if DIVOOM_ASTRO_MANUAL_DATETIME_SET_ENABLE
        disabled = (phandle->list_row_kind[i] == DIVOOM_ASTRO_SET_DT_KIND_TIME
            || phandle->list_row_kind[i] == DIVOOM_ASTRO_SET_DT_KIND_DATE) ? g_dt_auto_set : 0;
        row_opa = disabled ? LV_OPA_40 : LV_OPA_COVER;
        if (disabled)
        {
            lv_obj_clear_state(phandle->list_row[i], LV_STATE_FOCUSED);
            lv_obj_clear_state(phandle->list_row[i], LV_STATE_FOCUS_KEY);
        }
        lv_obj_set_style_opa(phandle->list_row[i], row_opa, 0);
        if (phandle->list_row_chev[i])
        {
            lv_obj_set_style_img_opa(phandle->list_row_chev[i], row_opa, 0);
        }
#endif
    }
#if DIVOOM_ASTRO_MANUAL_DATETIME_SET_ENABLE
    divoom_astrootoo_settings_dt_normalize_list_focus(phandle);
#endif
}

static void divoom_astrootoo_settings_group_bind_list(Pdivoom_astrootoo_settings_ctrl phandle, lv_obj_t *focus_target);

static void divoom_astrootoo_settings_system_check_key(lv_event_t *e)
{
    Pdivoom_astrootoo_settings_ctrl phandle = lv_event_get_user_data(e);
    if (lv_event_get_code(e) != LV_EVENT_KEY || !phandle->system_check_finished ||
        phandle->system_check_finished == 2 ||
        !divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e))) return;
    lv_obj_del(phandle->system_check_overlay);
    phandle->system_check_overlay = NULL;
    phandle->system_check_finished = 0;
    divoom_main_check_deivce_update_version();
    divoom_astrootoo_settings_group_bind_list(phandle, phandle->list_row[3]);
}

static void divoom_astrootoo_settings_system_check_poll(lv_timer_t *timer)
{
    Pdivoom_astrootoo_settings_ctrl phandle = lv_timer_get_user_data(timer);
    divoom_product_test_usr_check_status status;
    if (!phandle->system_check_overlay) return;
    int lang = divoom_main_get_system_config()->lang;
    if (phandle->system_check_start_failed ||
        (phandle->system_check_finished == 2 && divoom_product_test_format_settings_request_status() < 0)) {
        /* The normal updater may briefly own its files. Requeue on this
         * existing poller; never require an error-page Retry key. */
        phandle->system_check_finished = 0;
        phandle->system_check_start_failed = !divoom_product_test_usr_check_start_settings();
        if (phandle->system_check_start_failed) {
            lv_label_set_text(phandle->system_check_state,
                divoom_product_test_integrity_text(lang, "Waiting for storage"));
            lv_label_set_text(phandle->system_check_hint,
                divoom_product_test_integrity_text(lang, "Recovery continues automatically"));
            return;
        }
    }
    if (phandle->system_check_finished) return;
    divoom_product_test_usr_check_get_status(&status);
    char errors[96];
    snprintf(errors, sizeof(errors), divoom_product_test_integrity_text(lang, "Error files: %d"), status.error_files);
    lv_label_set_text_fmt(phandle->system_check_progress, "%d/%d\n%s", status.done, status.total, errors);
    lv_bar_set_value(phandle->system_check_bar, status.total > 0 ?
        (int)((uint64_t)status.done * 100 / status.total) : 0, LV_ANIM_OFF);
    if (status.phase == DIVOOM_PT_USR_CHECK_RUNNING) {
        lv_label_set_text(phandle->system_check_state, divoom_product_test_integrity_text(lang,
            divoom_product_test_integrity_stage_key(status.fs_check_stage)));
        lv_label_set_text(phandle->system_check_hint, status.fs_check_stage ?
            divoom_product_test_integrity_text(lang, status.fs_check_target ? "System Partition" : "User Partition") : "");
        return;
    }
    if (status.phase == DIVOOM_PT_USR_CHECK_DONE && !status.error_files) {
        phandle->system_check_finished = 1;
        lv_label_set_text(phandle->system_check_state, divoom_product_test_integrity_text(lang, "Check passed"));
        lv_obj_set_style_text_color(phandle->system_check_state, lv_color_hex(0x61D69D), 0);
        lv_label_set_text(phandle->system_check_hint, divoom_product_test_integrity_text(lang, "Press any key to return"));
    } else {
        /* The completed checker already queued the request with its gate held.
         * Do not enqueue it again or reset an already rejected request. */
        phandle->system_check_finished = 2;
        lv_label_set_text(phandle->system_check_state, divoom_product_test_integrity_text(lang, "Starting repair"));
    }
}

static void divoom_astrootoo_settings_system_check_show(Pdivoom_astrootoo_settings_ctrl phandle)
{
    if (phandle->system_check_overlay) return;
    lv_obj_t *panel = lv_obj_create(phandle->root_obj);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_pos(panel, 0, 0);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x151E2D), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    phandle->system_check_overlay = panel;
    lv_obj_t *labels[4];
    const int ys[] = {48, 142, 212, 376};
    for (int i = 0; i < 4; ++i) {
        labels[i] = lv_label_create(panel);
        lv_obj_set_pos(labels[i], 28, ys[i]);
        lv_obj_set_width(labels[i], DIVOOM_SCREEN_WIDTH - 56);
        lv_obj_set_style_text_font(labels[i], divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
        lv_obj_set_style_text_color(labels[i], lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_align(labels[i], LV_TEXT_ALIGN_CENTER, 0);
    }
    lv_label_set_text(labels[0], divoom_product_test_system_check_text(divoom_main_get_system_config()->lang));
    phandle->system_check_state = labels[1];
    phandle->system_check_progress = labels[2];
    phandle->system_check_hint = labels[3];
    int lang = divoom_main_get_system_config()->lang;
    lv_label_set_text(labels[1], divoom_product_test_integrity_text(lang, "Checking file system"));
    char errors[96];
    snprintf(errors, sizeof(errors), divoom_product_test_integrity_text(lang, "Error files: %d"), 0);
    lv_label_set_text_fmt(labels[2], "0/0\n%s", errors);
    lv_label_set_text(labels[3], "");
    phandle->system_check_bar = lv_bar_create(panel);
    lv_obj_set_pos(phandle->system_check_bar, 32, 308);
    lv_obj_set_size(phandle->system_check_bar, DIVOOM_SCREEN_WIDTH - 64, 12);
    lv_bar_set_range(phandle->system_check_bar, 0, 100);
    lv_obj_set_style_bg_color(phandle->system_check_bar, lv_color_hex(0x27CEE0), LV_PART_INDICATOR);
    lv_obj_add_event_cb(panel, divoom_astrootoo_settings_system_check_key, LV_EVENT_KEY, phandle);
    lv_group_add_obj(lv_group_get_default(), panel);
    lv_group_focus_obj(panel);
    phandle->system_check_finished = 0;
    phandle->about_pending_action = 0;
    /* Reuse About's poller and fonts; no screen switch or font preparation. */
    phandle->system_check_start_failed = !phandle->about_update_timer || !divoom_product_test_usr_check_start_settings();
    if (phandle->system_check_start_failed) {
        /* Without a poller there is no automatic retry. Keep the page dismissible. */
        phandle->system_check_finished = phandle->about_update_timer ? 0 : 1;
        lv_label_set_text(labels[1], divoom_product_test_integrity_text(divoom_main_get_system_config()->lang, "Waiting for storage"));
        lv_label_set_text(labels[3], divoom_product_test_integrity_text(divoom_main_get_system_config()->lang,
            phandle->about_update_timer ? "Recovery continues automatically" : "Press any key to return"));
    }
}

static void divoom_astrootoo_settings_list_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    lv_obj_t *target;
    lv_obj_t *next;
    uint8_t key_code;
    int pos;
    int i;
    Pdivoom_sys_setting_info cfg;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    phandle = (Pdivoom_astrootoo_settings_ctrl)lv_event_get_user_data(e);
    target = lv_event_get_target(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    cfg = divoom_main_get_system_config();

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_menu_control_return_pre();
        return;
    }
    if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code))
    {
#if DIVOOM_ASTRO_MANUAL_DATETIME_SET_ENABLE
        if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_TIME)
        {
            int pos_cur;
            int delta;

            pos_cur = divoom_menu_direct_ctrl_get_pos(phandle->direct_handle, target);
            delta = divoom_astrootoo_ui_key_is_up(key_code) ? -1 : 1;
            next = divoom_astrootoo_settings_dt_next_list_row(phandle, pos_cur, delta);
        }
        else
#endif
        {
            int direct;

            direct = divoom_astrootoo_ui_key_is_up(key_code) ? DIVOOM_MENU_DIRECT_UP : DIVOOM_MENU_DIRECT_DOWN;
            next = divoom_menu_direct_ctrl_get(phandle->direct_handle, target, direct);
        }
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
    for (i = 0; i < phandle->list_row_count; i++)
    {
        if (phandle->list_row[i] != target)
        {
            continue;
        }
        if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_TIME)
        {
            if (phandle->list_row_kind[i] == DIVOOM_ASTRO_SET_DT_KIND_HOUR24 && cfg)
            {
                cfg->hour12_24 = cfg->hour12_24 ? 0 : 1;
                divoom_astrootoo_settings_sync_sys_conf();
                divoom_astrootoo_settings_list_refresh_dt(phandle);
            }
#if DIVOOM_ASTRO_MANUAL_DATETIME_SET_ENABLE
            else if (phandle->list_row_kind[i] == DIVOOM_ASTRO_SET_DT_KIND_AUTO)
            {
                g_dt_auto_set = g_dt_auto_set ? 0 : 1;
                divoom_astrootoo_settings_list_refresh_dt(phandle);
            }
#endif
            else if (phandle->list_row_kind[i] == DIVOOM_ASTRO_SET_DT_KIND_FORMAT)
            {
                divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_DATE_FORMAT, NULL, 0, 0);
            }
#if DIVOOM_ASTRO_MANUAL_DATETIME_SET_ENABLE
            else if (phandle->list_row_kind[i] == DIVOOM_ASTRO_SET_DT_KIND_TIME)
            {
                if (!g_dt_auto_set)
                {
                    divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_DATE_TIME_TIME,
                        NULL, 0, 0);
                }
            }
            else if (phandle->list_row_kind[i] == DIVOOM_ASTRO_SET_DT_KIND_DATE)
            {
                if (!g_dt_auto_set)
                {
                    divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_DATE_TIME_DATE,
                        NULL, 0, 0);
                }
            }
#endif
        }
        else if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_ABOUT)
        {
            if (phandle->list_row_kind[i] == DIVOOM_ASTRO_SET_ABOUT_KIND_AUTO_UPDATE && cfg)
            {
                cfg->auto_update = cfg->auto_update ? 0 : 1;
                divoom_astrootoo_settings_sync_sys_conf();
                if (phandle->list_row_sw[i])
                {
                    divoom_menu_control_lv_img_set_src(phandle->list_row_sw[i],
                        cfg->auto_update ? DIVOOM_ASTROTOO_SETTINGS_SWITCH_ON : DIVOOM_ASTROTOO_SETTINGS_SWITCH_OFF);
                }
            }
            else if (phandle->list_row_kind[i] == DIVOOM_ASTRO_SET_ABOUT_KIND_BETA_FEATURES && cfg &&
                divoom_main_get_server_type())
            {
                if (cfg->beta_features)
                {
                    divoom_main_set_beta_features(0);
                }
                else
                {
                    /* 确认页搭建交给 About timer，默认聚焦取消。 */
                    phandle->about_pending_action = DIVOOM_ASTRO_SET_ACTION_BETA_FEATURES;
                    lv_timer_ready(phandle->about_update_timer);
                }
            }
            else if (phandle->list_row_kind[i] == DIVOOM_ASTRO_SET_ABOUT_KIND_CHECK_FILES)
            {
                divoom_astrootoo_settings_system_check_show(phandle);
            }
            else if (phandle->list_row_kind[i] == DIVOOM_ASTRO_SET_ABOUT_KIND_NEW_VERSION)
            {
                /* 固件/文件共用此入口；确认页创建与候选快照都由 timer 完成。 */
                phandle->about_pending_action = DIVOOM_ASTRO_SET_ACTION_CONFIRM_UPDATE;
                lv_timer_ready(phandle->about_update_timer);
            }
        }
        break;
    }
}

static lv_obj_t *divoom_astrootoo_settings_list_add_row(Pdivoom_astrootoo_settings_ctrl phandle,
    lv_coord_t y, uint8 kind, const char *label_key, const char *val_key, int has_chevron)
{
    lv_obj_t *card;
    lv_obj_t *lbl;
    lv_obj_t *val;
    lv_obj_t *sw;
    lv_obj_t *chev;
    int idx;

    if (phandle->list_row_count >= 8)
    {
        return NULL;
    }
    idx = phandle->list_row_count;
    card = lv_btn_create(phandle->content);
    lv_obj_add_style(card, &phandle->ui.card_item_style, 0);
    lv_obj_add_style(card, &phandle->ui.card_item_focus_style, LV_STATE_FOCUSED);
    lv_obj_add_style(card, &phandle->ui.card_item_focus_style, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_size(card, DIVOOM_ASTRO_SET_ROW_W, DIVOOM_ASTRO_SET_ROW_H);
    lv_obj_set_pos(card, DIVOOM_ASTRO_SET_ROW_X, y);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    divoom_menu_direct_ctrl_update(phandle->direct_handle, card, 0, idx);
    lv_obj_add_event_cb(card, divoom_astrootoo_settings_list_key_cb, LV_EVENT_KEY, phandle);

    lbl = lv_label_create(card);
    lv_label_set_text(lbl, !strcmp(label_key, "Check System File") ?
        divoom_product_test_system_check_text(divoom_main_get_system_config()->lang) :
        divoom_menu_control_get_disp_str(label_key));
    lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(
        phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_ABOUT ? 24 : 28, phandle->menu_id), 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID,
        phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_ABOUT
            ? DIVOOM_ASTRO_SET_CARD_LABEL_INSET_L : 8, 0);

    phandle->list_row_kind[idx] = kind;
    phandle->list_row[idx] = card;

    if (kind == DIVOOM_ASTRO_SET_DT_KIND_HOUR24
#if DIVOOM_ASTRO_MANUAL_DATETIME_SET_ENABLE
        || kind == DIVOOM_ASTRO_SET_DT_KIND_AUTO
#endif
        || kind == DIVOOM_ASTRO_SET_ABOUT_KIND_AUTO_UPDATE
        || kind == DIVOOM_ASTRO_SET_ABOUT_KIND_BETA_FEATURES)
    {
        divoom_astrootoo_ui_list_label_prepare(lbl,
            divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_SET_ROW_W,
                DIVOOM_ASTRO_SET_CARD_LABEL_INSET_L, DIVOOM_ASTRO_SET_CARD_LABEL_INSET_R,
                DIVOOM_ASTRO_LIST_TOGGLE_W));
        divoom_astrootoo_ui_list_row_bind_marquee(card, lbl);

        sw = lv_img_create(card);
        divoom_menu_control_lv_img_set_src(sw, DIVOOM_ASTROTOO_SETTINGS_SWITCH_OFF);
        lv_obj_align(sw, LV_ALIGN_RIGHT_MID,
            phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_ABOUT
                ? -DIVOOM_ASTRO_SET_CARD_LABEL_INSET_R : -8, 0);
        phandle->list_row_sw[idx] = sw;
    }
    else if (val_key || kind == DIVOOM_ASTRO_SET_ABOUT_KIND_SPACE)
    {
        val = lv_label_create(card);
        if (val_key)
        {
            lv_label_set_text(val, divoom_menu_control_get_disp_str(val_key));
        }
        else
        {
            lv_label_set_text(val, "—");
        }
        lv_obj_set_style_text_color(val, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
        lv_obj_set_style_text_font(val, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
        divoom_astrootoo_settings_bind_kv_row(card, lbl, val, has_chevron);
        phandle->list_row_val[idx] = val;
        if (has_chevron)
        {
            chev = lv_img_create(card);
            divoom_menu_control_lv_img_set_src(chev, DIVOOM_ASTROTOO_SETTINGS_CHEVRON);
            divoom_astrootoo_ui_apply_sublevel_chevron_size(chev);
            lv_obj_align(chev, LV_ALIGN_RIGHT_MID,
                phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_ABOUT
                    ? -DIVOOM_ASTRO_SET_CARD_LABEL_INSET_R : -8, 0);
            phandle->list_row_chev[idx] = chev;
        }
    }
    else if (has_chevron)
    {
        divoom_astrootoo_ui_list_label_prepare(lbl,
            divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_SET_ROW_W,
                DIVOOM_ASTRO_SET_CARD_LABEL_INSET_L, DIVOOM_ASTRO_SET_CARD_LABEL_INSET_R,
                DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W));
        divoom_astrootoo_ui_list_row_bind_marquee(card, lbl);

        chev = lv_img_create(card);
        divoom_menu_control_lv_img_set_src(chev, DIVOOM_ASTROTOO_SETTINGS_CHEVRON);
        divoom_astrootoo_ui_apply_sublevel_chevron_size(chev);
        lv_obj_align(chev, LV_ALIGN_RIGHT_MID,
            phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_ABOUT
                ? -DIVOOM_ASTRO_SET_CARD_LABEL_INSET_R : -8, 0);
        phandle->list_row_chev[idx] = chev;
    }

    phandle->list_row_count++;
    return card;
}

static void divoom_astrootoo_settings_group_bind_list(Pdivoom_astrootoo_settings_ctrl phandle,
    lv_obj_t *focus_target)
{
    lv_group_t *g;
    int i;
    int visible = 0;
    lv_obj_t *focus;

    if (!phandle || phandle->list_row_count <= 0)
    {
        return;
    }
    if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_ABOUT)
    {
        divoom_menu_direct_ctrl_destory(phandle->direct_handle);
        phandle->direct_handle = divoom_menu_direct_ctrl_create(1, phandle->list_row_count);
        if (!phandle->direct_handle)
        {
            return;
        }
    }
    g = lv_group_get_default();
    lv_group_remove_all_objs(g);
    for (i = 0; i < phandle->list_row_count; i++)
    {
        if (phandle->list_row[i] && !lv_obj_has_flag(phandle->list_row[i], LV_OBJ_FLAG_HIDDEN))
        {
            lv_group_add_obj(g, phandle->list_row[i]);
            if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_ABOUT)
            {
                divoom_menu_direct_ctrl_update(phandle->direct_handle, phandle->list_row[i], 0, visible++);
            }
        }
    }
    if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_ABOUT)
    {
        divoom_menu_direct_ctrl_update_row_col(phandle->direct_handle, 1, visible);
    }
    focus = NULL;
    if (focus_target && lv_obj_is_valid(focus_target)
        && !lv_obj_has_flag(focus_target, LV_OBJ_FLAG_HIDDEN))
    {
        focus = focus_target;
    }
    else if (phandle->list_row[0])
    {
        focus = phandle->list_row[0];
    }
    if (focus)
    {
        lv_group_focus_obj(focus);
        divoom_astrootoo_ui_focus_scroll(focus);
    }
}

static int divoom_astrootoo_settings_about_update_available(void)
{
    return divoom_main_get_device_last_version() > DIVOOM_SOFTWARE_VERSION;
}

static lv_obj_t *divoom_astrootoo_settings_about_add_update_row(
    Pdivoom_astrootoo_settings_ctrl phandle, uint8 kind, const char *label_key)
{
    lv_obj_t *card;
    lv_obj_t *dot;
    lv_coord_t y;

    if (!phandle)
    {
        return NULL;
    }

    y = DIVOOM_ASTRO_SET_TOP_GAP
        + 2 * (DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROW_GAP);
    card = divoom_astrootoo_settings_list_add_row(phandle, y,
        kind, label_key, NULL, 1);
    if (!card)
    {
        return NULL;
    }

    /* 真源 updateTrail：8px 危险色提示点，位于子级箭头左侧 6px。 */
    dot = lv_obj_create(card);
    lv_obj_remove_style_all(dot);
    lv_obj_set_size(dot, DIVOOM_ASTRO_ABOUT_UPDATE_DOT_SIZE,
        DIVOOM_ASTRO_ABOUT_UPDATE_DOT_SIZE);
    lv_obj_set_style_bg_color(dot, lv_color_hex(DIVOOM_ASTRO_DANGER), 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(dot, 0, 0);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(dot, LV_ALIGN_RIGHT_MID,
        -(DIVOOM_ASTRO_SET_CARD_LABEL_INSET_R + DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W
            + DIVOOM_ASTRO_ABOUT_UPDATE_TRAIL_GAP), 0);

    phandle->about_update_dot = dot;
    phandle->about_update_row_ready = 1;
    return card;
}

/* 可见行变化才重建导航，弹窗期间不抢走按钮焦点。 */
/* 新版本行和地区开关可以独立变化，版本文字始终排在可见卡片下方。 */
static void divoom_astrootoo_settings_about_place_beta(Pdivoom_astrootoo_settings_ctrl phandle,
    uint8 show_update, uint8 show_beta)
{
    lv_obj_t *beta = phandle->list_row[4];
    if (show_beta)
    {
        lv_obj_clear_flag(beta, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_y(beta, DIVOOM_ASTRO_SET_TOP_GAP +
            (show_update ? 4 : 3) * (DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROW_GAP));
    }
    else
    {
        lv_obj_add_flag(beta, LV_OBJ_FLAG_HIDDEN);
    }
    if (show_update && show_beta)
    {
        lv_obj_align_to(phandle->about_ver_lbl, beta, LV_ALIGN_OUT_BOTTOM_MID, 0, 12);
    }
    else
    {
        lv_obj_align(phandle->about_ver_lbl, LV_ALIGN_BOTTOM_MID, 0, -12);
    }
}

static void divoom_astrootoo_settings_about_update_timer_cb(lv_timer_t *timer)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    lv_obj_t *focus;
    int firmware_version;
    int show_update;
    int action;

    phandle = (Pdivoom_astrootoo_settings_ctrl)lv_timer_get_user_data(timer);
    if (!phandle || phandle->page != DIVOOM_ASTROTOO_SETTINGS_PAGE_ABOUT
        || !phandle->root_obj || !lv_obj_is_valid(phandle->root_obj))
    {
        return;
    }
    if (phandle->system_check_overlay) {
        divoom_astrootoo_settings_system_check_poll(timer);
        return;
    }
    action = phandle->about_pending_action;
    phandle->about_pending_action = 0;
    if (action == DIVOOM_ASTRO_SET_ACTION_BETA_FEATURES && divoom_main_get_server_type())
    {
        divoom_astrootoo_settings_root_confirm(phandle, action);
    }
    firmware_version = divoom_main_get_device_last_version();
    if (action == DIVOOM_ASTRO_SET_ACTION_START_UPDATE &&
        phandle->about_firmware_version > DIVOOM_SOFTWARE_VERSION &&
        firmware_version == phandle->about_firmware_version)
    {
        divoom_main_user_start_update_device();
    }
    if (action == DIVOOM_ASTRO_SET_ACTION_CONFIRM_UPDATE &&
        firmware_version > DIVOOM_SOFTWARE_VERSION)
    {
        phandle->about_firmware_version = firmware_version;
        divoom_astrootoo_settings_root_confirm(phandle, DIVOOM_ASTRO_SET_ACTION_UPGRADE);
    }
    if (phandle->confirm_overlay)
    {
        return;
    }
    const Pdivoom_sys_setting_info cfg = divoom_main_get_system_config();
    uint8 beta_enabled = cfg && cfg->beta_features;
    if (beta_enabled != phandle->about_beta_features)
    {
        phandle->about_beta_features = beta_enabled;
        divoom_menu_control_lv_img_set_src(phandle->list_row_sw[4],
            beta_enabled ? DIVOOM_ASTROTOO_SETTINGS_SWITCH_ON : DIVOOM_ASTROTOO_SETTINGS_SWITCH_OFF);
    }
    uint8 show_beta = divoom_main_get_server_type();
    show_update = divoom_astrootoo_settings_about_update_available();
    if (show_update != lv_obj_has_flag(phandle->list_row[2], LV_OBJ_FLAG_HIDDEN) &&
        show_beta != lv_obj_has_flag(phandle->list_row[4], LV_OBJ_FLAG_HIDDEN))
    {
        return;
    }
    focus = lv_group_get_focused(lv_group_get_default());
    if (show_update)
    {
        lv_obj_clear_flag(phandle->list_row[2], LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(phandle->list_row[2], LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_set_y(phandle->list_row[3], DIVOOM_ASTRO_SET_TOP_GAP +
        (show_update ? 3 : 2) * (DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROW_GAP));
    divoom_astrootoo_settings_about_place_beta(phandle, show_update, show_beta);
    divoom_astrootoo_settings_group_bind_list(phandle, focus);
}

static void divoom_astrootoo_settings_about_request_update(
    Pdivoom_astrootoo_settings_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }
    /* 关于页仅检查程序版本并确认程序升级。 */
    if (!phandle->system_check_overlay) {
        divoom_main_check_deivce_update_version();
    }
    if (!phandle->about_update_timer)
    {
        phandle->about_update_timer = lv_timer_create(
            divoom_astrootoo_settings_about_update_timer_cb,
            DIVOOM_ASTRO_ABOUT_UPDATE_POLL_MS, phandle);
    }
}

static void divoom_astrootoo_settings_build_datetime(Pdivoom_astrootoo_settings_ctrl phandle)
{
    lv_coord_t y;

    phandle->list_row_count = 0;
#if DIVOOM_ASTRO_MANUAL_DATETIME_SET_ENABLE
    phandle->direct_handle = divoom_menu_direct_ctrl_create(1, 5);
#else
    phandle->direct_handle = divoom_menu_direct_ctrl_create(1, 2);
#endif
    divoom_astrootoo_settings_create_header(phandle, "DateTime");
    phandle->content = divoom_astrootoo_ui_create_scroll_content(phandle->root_obj, &phandle->ui);

    y = DIVOOM_ASTRO_SET_TOP_GAP;
    divoom_astrootoo_settings_list_add_row(phandle, y, DIVOOM_ASTRO_SET_DT_KIND_HOUR24, "Hour24", NULL, 0);
    y += DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROW_GAP;
    divoom_astrootoo_settings_list_add_row(phandle, y, DIVOOM_ASTRO_SET_DT_KIND_FORMAT, "DateFormatLabel", NULL, 1);
#if DIVOOM_ASTRO_MANUAL_DATETIME_SET_ENABLE
    y += DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROW_GAP;
    divoom_astrootoo_settings_list_add_row(phandle, y, DIVOOM_ASTRO_SET_DT_KIND_AUTO, "AutoSet", NULL, 0);
    y += DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROW_GAP;
    divoom_astrootoo_settings_list_add_row(phandle, y, DIVOOM_ASTRO_SET_DT_KIND_TIME, "Time", NULL, 1);
    y += DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROW_GAP;
    divoom_astrootoo_settings_list_add_row(phandle, y, DIVOOM_ASTRO_SET_DT_KIND_DATE, "Date", NULL, 1);
#endif

    divoom_astrootoo_settings_list_refresh_dt(phandle);
}

/* 480-main aboutScreen.versionTpl: "版本：{{v1}}|{{v2}}" — v1=F101, v2=JL（仅缓存，无则 0） */
static void divoom_astrootoo_settings_format_about_version(char *buf, size_t buf_len)
{
    uint32_t jl_ver = 0;

    if (!buf || buf_len == 0)
        return;

#ifdef DIVOOM_TIMES_VIBE
    jl_ver = divoom_ota_jl_get_local_version();
    if (!divoom_ota_jl_version_is_valid(jl_ver))
        jl_ver = 0;
#endif

    snprintf(buf, buf_len, "%s:%u|%u",
        divoom_menu_control_get_disp_str("DeviceVersion"),
        (unsigned)DIVOOM_SOFTWARE_VERSION, (unsigned)jl_ver);
}

static void divoom_astrootoo_settings_build_about(Pdivoom_astrootoo_settings_ctrl phandle)
{
    Pdivoom_sys_setting_info cfg;
    uint32_t free_kb;
    lv_coord_t y;
    char buf[96];

    cfg = divoom_main_get_system_config();
    phandle->list_row_count = 0;
    phandle->about_update_dot = NULL;
    phandle->about_update_row_ready = 0;
    phandle->direct_handle = divoom_menu_direct_ctrl_create(1, 5);
    divoom_astrootoo_settings_create_header(phandle, "About");
    phandle->content = divoom_astrootoo_ui_create_scroll_content(phandle->root_obj, &phandle->ui);

    y = DIVOOM_ASTRO_SET_TOP_GAP;
    divoom_astrootoo_settings_list_add_row(phandle, y, DIVOOM_ASTRO_SET_ABOUT_KIND_SPACE,
        "AvailableSpace", NULL, 0);
    if (phandle->list_row_val[0])
    {
        free_kb = divoom_port_fs_free_space(DIVOOM_USER_ROOT);
        if (free_kb >= 1024U * 1024U)
            snprintf(buf, sizeof(buf), "%u.%uG", (unsigned)(free_kb / (1024U * 1024U)),
                (unsigned)((free_kb % (1024U * 1024U)) * 10U / (1024U * 1024U)));
        else if (free_kb >= 1024U)
            snprintf(buf, sizeof(buf), "%u.%uM", (unsigned)(free_kb / 1024U),
                (unsigned)((free_kb % 1024U) * 10U / 1024U));
        else
            snprintf(buf, sizeof(buf), "%uK", (unsigned)free_kb);
        lv_label_set_text(phandle->list_row_val[0], buf);
    }
    y += DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROW_GAP;
    divoom_astrootoo_settings_list_add_row(phandle, y, DIVOOM_ASTRO_SET_ABOUT_KIND_AUTO_UPDATE,
        "AutoUpdate", NULL, 0);
    if (cfg && phandle->list_row_sw[1])
    {
        divoom_menu_control_lv_img_set_src(phandle->list_row_sw[1],
            cfg->auto_update ? DIVOOM_ASTROTOO_SETTINGS_SWITCH_ON : DIVOOM_ASTROTOO_SETTINGS_SWITCH_OFF);
    }
    /* 仅预建一个升级入口，固件或允许目录内文件有更新时显示。 */
    divoom_astrootoo_settings_about_add_update_row(phandle,
        DIVOOM_ASTRO_SET_ABOUT_KIND_NEW_VERSION, "NewVersion");
    if (!divoom_astrootoo_settings_about_update_available())
    {
        lv_obj_add_flag(phandle->list_row[2], LV_OBJ_FLAG_HIDDEN);
    }

    y = DIVOOM_ASTRO_SET_TOP_GAP + (lv_obj_has_flag(phandle->list_row[2], LV_OBJ_FLAG_HIDDEN) ? 2 : 3) *
        (DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROW_GAP);
    divoom_astrootoo_settings_list_add_row(phandle, y, DIVOOM_ASTRO_SET_ABOUT_KIND_CHECK_FILES,
        "Check System File", NULL, 1);
    y += DIVOOM_ASTRO_SET_ROW_H + DIVOOM_ASTRO_SET_ROW_GAP;
    divoom_astrootoo_settings_list_add_row(phandle, y, DIVOOM_ASTRO_SET_ABOUT_KIND_BETA_FEATURES,
        "BetaFeatures", NULL, 0);
    phandle->about_beta_features = cfg && cfg->beta_features;
    if (phandle->about_beta_features)
    {
        divoom_menu_control_lv_img_set_src(phandle->list_row_sw[4], DIVOOM_ASTROTOO_SETTINGS_SWITCH_ON);
    }

    phandle->about_ver_lbl = lv_label_create(phandle->content);
    divoom_astrootoo_settings_format_about_version(buf, sizeof(buf));
    lv_label_set_text(phandle->about_ver_lbl, buf);
    lv_obj_set_style_text_color(phandle->about_ver_lbl, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(phandle->about_ver_lbl, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
    divoom_astrootoo_settings_about_place_beta(phandle,
        !lv_obj_has_flag(phandle->list_row[2], LV_OBJ_FLAG_HIDDEN), divoom_main_get_server_type());
}

static void divoom_astrootoo_settings_connection_code_release_qr(
    Pdivoom_astrootoo_settings_ctrl phandle)
{
    if (!phandle || !phandle->connection_code_qr_buf)
    {
        return;
    }

    lv_img_cache_invalidate_src(&phandle->connection_code_qr_dsc);
    divoom_system_free(phandle->connection_code_qr_buf);
    phandle->connection_code_qr_buf = NULL;
    memset(&phandle->connection_code_qr_dsc, 0,
        sizeof(phandle->connection_code_qr_dsc));
}

static int divoom_astrootoo_settings_connection_code_set_qr(
    Pdivoom_astrootoo_settings_ctrl phandle, char *raw_code)
{
    Pdivoom_qrencode_data qrencode_data;
    uint8 *qr_buf;
    uint32 dst_offset;
    uint32 src_offset;
    uint32 src_x;
    uint32 src_y;
    uint32 render_start_ms;
    uint32 x;
    uint32 y;
    uint16 rgb565;

    if (!phandle || !phandle->connection_code_qr || !raw_code)
    {
        return 0;
    }

    render_start_ms = lv_tick_get();
    qrencode_data = divoom_qrencode_string(raw_code);
    if (!qrencode_data || !qrencode_data->rgb_data
        || qrencode_data->pic_width == 0
        || qrencode_data->pic_width != qrencode_data->pic_high)
    {
        divoom_system_free(qrencode_data);
        return 0;
    }

    qr_buf = (uint8 *)divoom_system_malloc(
        (uint32)DIVOOM_ASTRO_SHARE_QR_SIZE * (uint32)DIVOOM_ASTRO_SHARE_QR_SIZE
        * DIVOOM_APP_RGB565_BYTES_PER_PX);
    if (!qr_buf)
    {
        divoom_system_free(qrencode_data);
        return 0;
    }

    /* 先在缓冲区做最近邻展开，避开仿真 LVGL transform 对小图放大时的裁剪问题。 */
    for (y = 0; y < DIVOOM_ASTRO_SHARE_QR_SIZE; y++)
    {
        src_y = y * (uint32)qrencode_data->pic_high
            / (uint32)DIVOOM_ASTRO_SHARE_QR_SIZE;
        for (x = 0; x < DIVOOM_ASTRO_SHARE_QR_SIZE; x++)
        {
            src_x = x * (uint32)qrencode_data->pic_width
                / (uint32)DIVOOM_ASTRO_SHARE_QR_SIZE;
            src_offset = (src_y * (uint32)qrencode_data->pic_width + src_x) * 3U;
            dst_offset = (y * (uint32)DIVOOM_ASTRO_SHARE_QR_SIZE + x)
                * DIVOOM_APP_RGB565_BYTES_PER_PX;
            rgb565 = divoom_rgb888_to_rgb565(
                qrencode_data->rgb_data[src_offset],
                qrencode_data->rgb_data[src_offset + 1U],
                qrencode_data->rgb_data[src_offset + 2U]);
            divoom_store_rgb565_le(qr_buf + dst_offset, rgb565);
        }
    }

    divoom_astrootoo_settings_connection_code_release_qr(phandle);
    phandle->connection_code_qr_buf = qr_buf;
    divoom_lv_img_dsc_init_fmt(&phandle->connection_code_qr_dsc,
        DIVOOM_ASTRO_SHARE_QR_SIZE, DIVOOM_ASTRO_SHARE_QR_SIZE,
        LV_COLOR_FORMAT_RGB565, qr_buf);

    lv_img_set_src(phandle->connection_code_qr,
        &phandle->connection_code_qr_dsc);
    lv_img_set_size_mode(phandle->connection_code_qr, LV_IMG_SIZE_MODE_VIRTUAL);
    lv_img_set_antialias(phandle->connection_code_qr, 0);
    lv_img_set_zoom(phandle->connection_code_qr, LV_IMG_ZOOM_NONE);
    lv_obj_set_size(phandle->connection_code_qr,
        DIVOOM_ASTRO_SHARE_QR_SIZE, DIVOOM_ASTRO_SHARE_QR_SIZE);
    lv_obj_center(phandle->connection_code_qr);

    divoom_print("[settings_share_code] qr source=%ux%u display=%ux%u render=%ums\n",
        qrencode_data->pic_width, qrencode_data->pic_high,
        DIVOOM_ASTRO_SHARE_QR_SIZE, DIVOOM_ASTRO_SHARE_QR_SIZE,
        lv_tick_elaps(render_start_ms));
    divoom_system_free(qrencode_data);
    return 1;
}

static void divoom_astrootoo_settings_connection_code_refresh(
    Pdivoom_astrootoo_settings_ctrl phandle)
{
    uint32 share_code;
    uint32 expire_time;
    char raw_code[9];
    char display_code[10];

    if (!phandle || phandle->connection_code_ready
        || !divoom_main_get_device_share_code(&share_code, &expire_time))
    {
        return;
    }

    snprintf(raw_code, sizeof(raw_code), "%08u", (unsigned int)share_code);
    snprintf(display_code, sizeof(display_code), "%.4s-%.4s", raw_code, raw_code + 4);
    if (!divoom_astrootoo_settings_connection_code_set_qr(phandle, raw_code))
    {
        return;
    }
    lv_label_set_text(phandle->connection_code_value_lbl, display_code);
    lv_obj_clear_flag(phandle->connection_code_frame, LV_OBJ_FLAG_HIDDEN);
    phandle->connection_code_displayed = share_code;
    phandle->connection_code_ready = 1;
    (void)expire_time;

    if (phandle->connection_code_timer && !phandle->connection_code_wait_binding)
    {
        lv_timer_del(phandle->connection_code_timer);
        phandle->connection_code_timer = NULL;
    }
}

static void divoom_astrootoo_settings_connection_code_timer_cb(lv_timer_t *timer)
{
    Pdivoom_astrootoo_settings_ctrl phandle;

    phandle = (Pdivoom_astrootoo_settings_ctrl)lv_timer_get_user_data(timer);
    if (!phandle || phandle->page != DIVOOM_ASTROTOO_SETTINGS_PAGE_CONNECTION_CODE
        || !phandle->root_obj || !lv_obj_is_valid(phandle->root_obj))
    {
        return;
    }
    if (phandle->connection_code_wait_binding && divoom_main_get_host_id() != 0
        && divoom_menu_control_get_active_menu_id() == phandle->menu_id)
    {
        /* 先停止定时器再排队返回，避免页面切换前重复投递退出事件。 */
        phandle->connection_code_wait_binding = 0;
        lv_timer_del(phandle->connection_code_timer);
        phandle->connection_code_timer = NULL;
        divoom_menu_control_return_pre();
        return;
    }
    divoom_astrootoo_settings_connection_code_refresh(phandle);
}

static void divoom_astrootoo_settings_connection_code_key_cb(lv_event_t *event)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    uint8 key_code;

    if (lv_event_get_code(event) != LV_EVENT_KEY)
    {
        return;
    }
    key_code = divoom_astrootoo_ui_key_code_from_event(event);
    if (!divoom_astrootoo_ui_key_is_menu_press(event, key_code))
    {
        return;
    }

    phandle = (Pdivoom_astrootoo_settings_ctrl)lv_event_get_user_data(event);
    if (!phandle)
    {
        return;
    }
    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_print("[WIFI_CFG] share code ESC return Wi-Fi page\n");
        divoom_menu_control_return_pre();
        return;
    }
    if (divoom_astrootoo_ui_key_is_enter(key_code) && !phandle->connection_code_ready)
    {
        divoom_main_request_device_share_code();
        divoom_astrootoo_settings_connection_code_refresh(phandle);
    }
}

static void divoom_astrootoo_settings_build_connection_code(
    Pdivoom_astrootoo_settings_ctrl phandle)
{
    phandle->connection_code_wait_binding = divoom_main_get_host_id() == 0;
    divoom_astrootoo_settings_create_header(phandle, "DeviceConnectionCode");
    phandle->content = divoom_astrootoo_ui_create_scroll_content(phandle->root_obj, &phandle->ui);
    lv_obj_set_scrollbar_mode(phandle->content, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(phandle->content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_height(phandle->content, DIVOOM_ASTRO_CONTENT_H);

    phandle->connection_code_frame = lv_obj_create(phandle->content);
    lv_obj_add_style(phandle->connection_code_frame, &phandle->ui.content_style, 0);
    lv_obj_set_size(phandle->connection_code_frame, DIVOOM_ASTRO_SHARE_QR_FRAME_SIZE,
        DIVOOM_ASTRO_SHARE_QR_FRAME_SIZE);
    lv_obj_set_pos(phandle->connection_code_frame, DIVOOM_ASTRO_SHARE_QR_X,
        DIVOOM_ASTRO_SHARE_QR_Y);
    lv_obj_set_style_bg_color(phandle->connection_code_frame, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(phandle->connection_code_frame, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(phandle->connection_code_frame,
        lv_color_hex(DIVOOM_ASTRO_SHARE_GOLD), 0);
    lv_obj_set_style_border_width(phandle->connection_code_frame, 4, 0);
    lv_obj_set_style_radius(phandle->connection_code_frame, 8, 0);
    lv_obj_set_style_pad_all(phandle->connection_code_frame, 0, 0);
    lv_obj_clear_flag(phandle->connection_code_frame, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(phandle->connection_code_frame, LV_OBJ_FLAG_HIDDEN);

    phandle->connection_code_qr = lv_img_create(phandle->connection_code_frame);
    lv_obj_clear_flag(phandle->connection_code_qr, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(phandle->connection_code_qr);

    phandle->connection_code_value_lbl = lv_label_create(phandle->content);
    lv_label_set_text(phandle->connection_code_value_lbl, "---- ----");
    lv_obj_set_width(phandle->connection_code_value_lbl, DIVOOM_SCREEN_WIDTH);
    lv_obj_set_pos(phandle->connection_code_value_lbl, 0, DIVOOM_ASTRO_SHARE_CODE_Y);
    lv_obj_set_style_text_align(phandle->connection_code_value_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(phandle->connection_code_value_lbl,
        lv_color_hex(DIVOOM_ASTRO_SHARE_CODE_COLOR), 0);
    lv_obj_set_style_text_font(phandle->connection_code_value_lbl,
        divoom_menu_control_get_font_info(32, phandle->menu_id), 0);

    phandle->connection_code_hint_lbl = lv_label_create(phandle->content);
    lv_label_set_text(phandle->connection_code_hint_lbl,
        divoom_menu_control_get_disp_str("DeviceConnectionCodeHint"));
    lv_obj_set_width(phandle->connection_code_hint_lbl, 448);
    lv_obj_set_pos(phandle->connection_code_hint_lbl, 16, DIVOOM_ASTRO_SHARE_HINT_Y);
    lv_obj_set_style_text_align(phandle->connection_code_hint_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(phandle->connection_code_hint_lbl,
        lv_color_hex(DIVOOM_ASTRO_SHARE_HINT_COLOR), 0);
    lv_obj_set_style_text_font(phandle->connection_code_hint_lbl,
        divoom_menu_control_get_font_info(16, phandle->menu_id), 0);
    lv_label_set_long_mode(phandle->connection_code_hint_lbl, LV_LABEL_LONG_WRAP);

    phandle->connection_code_skip_lbl = lv_label_create(phandle->content);
    lv_label_set_text(phandle->connection_code_skip_lbl,
        divoom_menu_control_get_disp_str("DeviceConnectionCodeSkipHint"));
    lv_obj_set_width(phandle->connection_code_skip_lbl, 448);
    lv_obj_set_style_text_align(phandle->connection_code_skip_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(phandle->connection_code_skip_lbl,
        lv_color_hex(DIVOOM_ASTRO_SHARE_SKIP_COLOR), 0);
    lv_obj_set_style_text_font(phandle->connection_code_skip_lbl,
        divoom_menu_control_get_font_info(16, phandle->menu_id), 0);
    lv_label_set_long_mode(phandle->connection_code_skip_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_align(phandle->connection_code_skip_lbl, LV_ALIGN_BOTTOM_MID, 0,
        -DIVOOM_ASTRO_SHARE_SKIP_PAD_B);

    phandle->connection_code_key_btn = divoom_astrootoo_ui_create_key_capture_btn(
        phandle->root_obj, &phandle->ui, divoom_astrootoo_settings_connection_code_key_cb, phandle);

    if (!divoom_main_get_device_share_code(NULL, NULL))
    {
        divoom_main_request_device_share_code();
    }
    divoom_astrootoo_settings_connection_code_refresh(phandle);
    if (!phandle->connection_code_ready || phandle->connection_code_wait_binding)
    {
        phandle->connection_code_timer = lv_timer_create(
            divoom_astrootoo_settings_connection_code_timer_cb, 100, phandle);
    }
}

static void divoom_astrootoo_settings_build_subpage(Pdivoom_astrootoo_settings_ctrl phandle)
{
    static const char *eq_keys[] = {
        [DIVOOM_AUDIO_EFFECT_ORIGINAL] = "EqPresetOriginal", 
        [DIVOOM_AUDIO_EFFECT_GAMING] = "EqPresetGaming", 
        [DIVOOM_AUDIO_EFFECT_HIFI] = "EqPresetHifi",
        [DIVOOM_AUDIO_EFFECT_OUTDOOR] = "EqPresetOutdoor", 
        [DIVOOM_AUDIO_EFFECT_RETRO] = "EqPresetRetro"
    };
    static const char *idle_keys[] = {
        "IdleShutdown30m", "IdleShutdown1h", "IdleShutdown3h",
        "IdleShutdown6h", "IdleShutdown12h", "Never"
    };
    static const char *wind_keys[] = {
        "WindUnitMs", "WindUnitKmh", "WindUnitMph", "WindUnitKt"
    };
    static const char *temp_keys[] = { "TempUnitCelsius", "TempUnitFahrenheit" };
    switch (phandle->page)
    {
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_EQUALIZER:
    {
        Pdivoom_sys_setting_info cfg;

        g_eq_preset_idx = 0;
        cfg = divoom_main_get_system_config();
        if (cfg)
        {
            g_eq_preset_idx = (int)cfg->audio_effect;
        }
        if (g_eq_preset_idx < 0 || g_eq_preset_idx >= 5)
        {
            g_eq_preset_idx = 0;
        }
        divoom_astrootoo_settings_build_picker(phandle, "Equalizer", eq_keys, 5, g_eq_preset_idx);
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_IDLE_SHUTDOWN:
    {
        divoom_astrootoo_settings_build_picker(phandle, "IdleShutdown",
            idle_keys, 6, divoom_astrootoo_settings_idle_committed());
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_WIND_UNIT:
    {
        int c;

        c = (int)divoom_main_get_wind_speed_flag();
        if (c < 0 || c > 3)
        {
            c = 0;
        }
        divoom_astrootoo_settings_build_picker(phandle, "WindUnit", wind_keys, 4, c);
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_TEMP_UNIT:
    {
        int c;

        c = divoom_main_get_temp_flag();
        if (c < 0 || c > 1)
        {
            c = 0;
        }
        divoom_astrootoo_settings_build_picker(phandle, "Temperature", temp_keys, 2, c);
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_FORMAT:
    {
        static const char *fmt_keys[] = {
            "DateFmtYmd", "DateFmtDmy", "DateFmtMdy",
            "DateFmtYmdDot", "DateFmtDmyDot", "DateFmtMdyDot",
            "DateFmtYmdSlash", "DateFmtDmySlash", "DateFmtMdySlash"
        };
        int tf;

        tf = (int)divoom_main_get_time_format_type();
        if (tf < 0 || tf >= 9)
        {
            tf = 0;
        }
        divoom_astrootoo_settings_build_picker(phandle, "DateFormatLabel", fmt_keys, 9, tf);
        break;
    }
case DIVOOM_ASTROTOO_SETTINGS_PAGE_SLIDESHOW_PHOTO_ORDER:
    {
        Pdivoom_pic_phone_manage_slide_config cfg;
        static const char *order_keys[] = { "OrderByAcceptTime", "OrderByPhotoTime", "OrderByRandom" };
        int c;

        cfg = divoom_pic_phone_manage_get_slide_config();
        c = cfg ? (int)cfg->disp_order : 0;
        if (c < 0 || c > 2)
        {
            c = 0;
        }
        divoom_astrootoo_settings_build_picker(phandle, "DisplayOrder", order_keys, 3, c);
        break;
    }
    default:
    {
        break;
    }
    }
}

void *divoom_astrootoo_settings_init(void *user_data, int menu_id)
{
    Pdivoom_astrootoo_settings_ctrl phandle;

    if (menu_id == DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_CONNECTION_CODE
        && user_data != NULL)
    {
        divoom_system_free(user_data);
    }

    phandle = divoom_system_malloc_out(sizeof(divoom_astrootoo_settings_ctrl));
    if (!phandle)
    {
        return NULL;
    }

    phandle->system_check_overlay = NULL;
    phandle->system_check_finished = 0;
    phandle->system_check_start_failed = 0;
    phandle->page = divoom_astrootoo_settings_page_from_menu_id(menu_id);
    phandle->menu_id = menu_id;
    phandle->root_obj = divoom_astrootoo_ui_create_root(&phandle->ui);

    if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_BRIGHTNESS)
    {
        divoom_astrootoo_settings_build_slider(phandle, "Brightness", 50);
    }
    else if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_VOLUME)
    {
        divoom_astrootoo_settings_build_slider(phandle, "Volume", 50);
    }
    else if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_NIGHTMODE)
    {
        divoom_astrootoo_settings_build_nightmode(phandle);
    }
    else if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_SLIDESHOW)
    {
        divoom_astrootoo_settings_slideshow_build(phandle);
    }
    else if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_LANGUAGE)
    {
        divoom_astrootoo_settings_build_language(phandle);
    }
    else if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_BLUETOOTH)
    {
        divoom_astrootoo_settings_build_bluetooth(phandle);
    }
    else if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI)
    {
        divoom_astrootoo_settings_build_wifi(phandle);
    }
    else if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_ADD)
    {
        divoom_astrootoo_settings_build_wifi_add(phandle);
    }
    else if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_PASSWORD)
    {
        divoom_astrootoo_settings_build_wifi_password(phandle);
    }
    else if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_TIME)
    {
        divoom_astrootoo_settings_build_datetime(phandle);
    }
    else if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_TIME_TIME)
    {
        divoom_astrootoo_settings_build_dt_time_pick(phandle);
    }
    else if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_TIME_DATE)
    {
        divoom_astrootoo_settings_build_dt_date_pick(phandle);
    }
    else if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_ABOUT)
    {
        divoom_astrootoo_settings_build_about(phandle);
    }
    else if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_CONNECTION_CODE)
    {
        divoom_astrootoo_settings_build_connection_code(phandle);
    }
    else if (divoom_astrootoo_settings_ex_is_page(phandle->page))
    {
        divoom_astrootoo_settings_ex_build(phandle);
    }
    else if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_KEY_LIGHT_EFFECTS)
    {
        divoom_astrootoo_settings_build_key_light_effects(phandle);
    }
    else if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_SLIDESHOW_PHOTO_ORDER
        || (phandle->page >= DIVOOM_ASTROTOO_SETTINGS_PAGE_EQUALIZER
            && phandle->page <= DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_FORMAT))
    {
        divoom_astrootoo_settings_build_subpage(phandle);
    }
    else
    {
        divoom_astrootoo_settings_build_root(phandle);
    }

    return phandle;
}

void divoom_astrootoo_settings_disp(void *handle, void *active_obj)
{
    Pdivoom_astrootoo_settings_ctrl phandle;

    phandle = (Pdivoom_astrootoo_settings_ctrl)handle;
    if (!phandle)
    {
        return;
    }

    lv_scr_load(phandle->root_obj);
    if (phandle->system_check_overlay) {
        divoom_astrootoo_settings_about_request_update(phandle);
        lv_group_add_obj(lv_group_get_default(), phandle->system_check_overlay);
        lv_group_focus_obj(phandle->system_check_overlay);
        return;
    }

    switch (phandle->page)
    {
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_ROOT:
    {
        divoom_astrootoo_settings_group_bind_content(phandle, active_obj);
        divoom_astrootoo_settings_battery_timer_start(phandle);
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_NIGHTMODE:
    {
        divoom_astrootoo_settings_group_bind_night(phandle, active_obj);
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_SLIDESHOW:
    {
        divoom_astrootoo_settings_slideshow_group_bind(phandle, active_obj);
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_BRIGHTNESS:
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_VOLUME:
    {
        divoom_astrootoo_settings_group_bind_slider(phandle, active_obj);
        if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_BRIGHTNESS)
        {
            divoom_astrootoo_settings_battery_timer_start(phandle);
            divoom_astrootoo_settings_sync_brightness_ui(phandle);
        }
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_LANGUAGE:
    {
        divoom_astrootoo_settings_group_bind_lang(phandle, active_obj);
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_BLUETOOTH:
    {
        divoom_astrootoo_settings_group_bind_bt(phandle, active_obj);
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI:
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_ADD:
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_PASSWORD:
    {
        divoom_astrootoo_settings_wifi_group_bind(phandle, active_obj);
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_EQUALIZER:
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_IDLE_SHUTDOWN:
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_WIND_UNIT:
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_TEMP_UNIT:
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_FORMAT:
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_SLIDESHOW_PHOTO_ORDER:
    {
        divoom_astrootoo_settings_group_bind_picker(phandle, active_obj);
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_KEY_LIGHT_EFFECTS:
    {
        divoom_astrootoo_settings_group_bind_key_light(phandle, active_obj);
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_SCHEDULED_POWER:
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_BT_PASSWORD:
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_MCP:
    {
        divoom_astrootoo_settings_ex_group_bind(phandle, active_obj);
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_TIME:
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_ABOUT:
    {
        if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_TIME)
        {
            divoom_astrootoo_settings_list_refresh_dt(phandle);
        }
        divoom_astrootoo_settings_group_bind_list(phandle, active_obj);
        if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_ABOUT)
        {
            divoom_astrootoo_settings_about_request_update(phandle);
        }
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_TIME_TIME:
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_TIME_DATE:
    {
        if (phandle->dt_key_btn)
        {
            lv_group_remove_all_objs(lv_group_get_default());
            lv_group_add_obj(lv_group_get_default(), phandle->dt_key_btn);
            lv_group_focus_obj(phandle->dt_key_btn);
        }
        break;
    }
    case DIVOOM_ASTROTOO_SETTINGS_PAGE_CONNECTION_CODE:
    {
        if (phandle->connection_code_key_btn)
        {
            lv_group_remove_all_objs(lv_group_get_default());
            lv_group_add_obj(lv_group_get_default(), phandle->connection_code_key_btn);
            lv_group_focus_obj(phandle->connection_code_key_btn);
        }
        break;
    }
    default:
    {
        break;
    }
    }
}

void divoom_astrootoo_settings_clean(void *handle)
{
    Pdivoom_astrootoo_settings_ctrl phandle;

    phandle = (Pdivoom_astrootoo_settings_ctrl)handle;
    if (!phandle)
    {
        return;
    }

    /* 先从 group 摘除本页控件，避免 lv_obj_del 时 group 仍持有悬空指针 */
    divoom_astrootoo_settings_group_detach_page(phandle);

    if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_NIGHTMODE)
    {
        divoom_astrootoo_settings_night_preview_stop(phandle);
        divoom_astrootoo_settings_night_save(phandle);
    }
    if (divoom_astrootoo_settings_slideshow_is_page(phandle->page))
    {
        divoom_astrootoo_settings_slideshow_save(phandle);
    }

    divoom_astrootoo_settings_stop_page_timers(phandle);

    /* 确认弹窗为 root_obj 子对象，随 root_obj 删除自动释放，这里仅清指针 */
    phandle->confirm_overlay = NULL;
    phandle->confirm_beta_body = NULL;
    phandle->confirm_cancel_btn = NULL;
    phandle->confirm_ok_btn = NULL;
    phandle->confirm_focus_restore = NULL;
    phandle->confirm_box = NULL;

    if (phandle->direct_handle)
    {
        divoom_menu_direct_ctrl_destory(phandle->direct_handle);
        phandle->direct_handle = NULL;
    }

    divoom_astrootoo_settings_ex_release(phandle);
    if (phandle->root_obj)
    {
        lv_obj_del(phandle->root_obj);
        phandle->root_obj = NULL;
    }
    divoom_astrootoo_settings_connection_code_release_qr(phandle);

    divoom_astrootoo_ui_base_reset_styles(&phandle->ui);
    divoom_system_free(phandle);
}

void divoom_astrootoo_settings_pause(void *handle)
{
    Pdivoom_astrootoo_settings_ctrl phandle;

    phandle = (Pdivoom_astrootoo_settings_ctrl)handle;
    if (!phandle)
    {
        return;
    }
    /* return_pre 压栈：停子页 timer，避免 pause 后 clean 前回调访问已释放 phandle */
    divoom_astrootoo_settings_stop_page_timers(phandle);
    /* 仅摘除本页 group 成员，勿清空整个 group（父页 disp 尚未执行） */
    divoom_astrootoo_settings_group_detach_page(phandle);
}

/* ====================================================================== */
/* 白盒压测：按 focus_items 顺序遍历设置根页全部子项                         */
/* ====================================================================== */

#if defined(DIVOOM_SIMULATOR)

#include "sim_keypad.h"

#define WB_TOUR_KEY_INTERVAL_MS    450u
#define WB_TOUR_FOCUS_PREV_CNT     8
#define WB_TOUR_ESC_MAX            8

enum
{
    WB_TOUR_ST_IDLE = 0,
    WB_TOUR_ST_ENSURE_ROOT,
    WB_TOUR_ST_FOCUS,
    WB_TOUR_ST_ROOT_ACT,
    WB_TOUR_ST_WAIT_SUB,
    WB_TOUR_ST_SUB_ACT,
    WB_TOUR_ST_WAIT_NESTED,
    WB_TOUR_ST_ESC_ROOT,
    WB_TOUR_ST_GAP,
};

typedef struct
{
    int active;
    int done;
    int fail;
    char fail_detail[200];
    int focus_idx;
    int focus_total;
    int phase;
    int item_kind;
    int item_menu_id;
    int key_step;
    int focus_step;
    int esc_try;
    unsigned tour_start_ms;
    unsigned item_start_ms;
    unsigned phase_start_ms;
    unsigned last_key_ms;
    unsigned timeout_ms;
    unsigned item_timeout_ms;
} settings_wb_tour_t;

static settings_wb_tour_t g_settings_wb_tour;

static int settings_wb_tour_get_item(int focus_idx, int *kind, int *menu_id)
{
    int i;
    int fi;

    fi = 0;
    for (i = 0; i < DIVOOM_ASTRO_SET_ITEM_CNT; i++)
    {
        if (g_settings_items[i].kind == DIVOOM_ASTRO_SET_KIND_SECTION)
        {
            continue;
        }
        if (fi == focus_idx)
        {
            if (kind)
            {
                *kind = g_settings_items[i].kind;
            }
            if (menu_id)
            {
                *menu_id = g_settings_items[i].menu_id;
            }
            return 1;
        }
        fi++;
    }
    return 0;
}

static int settings_wb_tour_focus_direct(int focus_idx)
{
    Pdivoom_astrootoo_settings_ctrl ph;
    lv_obj_t *card;
    lv_obj_t *next;
    int i;

    ph = (Pdivoom_astrootoo_settings_ctrl)divoom_menu_control_get_clock_mode_menu_handle();
    if (!ph || ph->page != DIVOOM_ASTROTOO_SETTINGS_PAGE_ROOT || !ph->direct_handle)
    {
        return 0;
    }
    if (focus_idx < 0 || focus_idx >= ph->focus_cnt)
    {
        return 0;
    }
    card = ph->bright_card;
    if (!card || !lv_obj_is_valid(card))
    {
        return 0;
    }
    for (i = 0; i < focus_idx; i++)
    {
        next = (lv_obj_t *)divoom_menu_direct_ctrl_get(ph->direct_handle, card, DIVOOM_MENU_DIRECT_DOWN);
        if (!next || next == card)
        {
            return 0;
        }
        card = next;
    }
    lv_group_focus_obj(card);
    divoom_astrootoo_ui_focus_scroll_keep_section_title(card);
    return 1;
}

static void settings_wb_tour_set_fail(const char *msg)
{
    g_settings_wb_tour.fail = 1;
    g_settings_wb_tour.active = 0;
    if (msg)
    {
        strncpy(g_settings_wb_tour.fail_detail, msg, sizeof(g_settings_wb_tour.fail_detail) - 1);
        g_settings_wb_tour.fail_detail[sizeof(g_settings_wb_tour.fail_detail) - 1] = '\0';
    }
}

static int settings_wb_tour_can_inject(unsigned now_ms)
{
    if (g_settings_wb_tour.last_key_ms == 0
        || now_ms - g_settings_wb_tour.last_key_ms >= WB_TOUR_KEY_INTERVAL_MS)
    {
        return 1;
    }
    return 0;
}

static void settings_wb_tour_inject(int key_id, unsigned now_ms)
{
    sim_keypad_inject_divoom_key(key_id, DIVOOM_PRESS_SHORT);
    g_settings_wb_tour.last_key_ms = now_ms;
}

static int settings_wb_tour_item_timeout(unsigned now_ms)
{
    if (g_settings_wb_tour.item_timeout_ms == 0)
    {
        return 0;
    }
    if (g_settings_wb_tour.phase == WB_TOUR_ST_IDLE
        || g_settings_wb_tour.phase == WB_TOUR_ST_ENSURE_ROOT
        || g_settings_wb_tour.phase == WB_TOUR_ST_FOCUS
        || g_settings_wb_tour.phase == WB_TOUR_ST_GAP)
    {
        return 0;
    }
    if (now_ms - g_settings_wb_tour.item_start_ms >= g_settings_wb_tour.item_timeout_ms)
    {
        return 1;
    }
    return 0;
}

static int settings_wb_tour_total_timeout(unsigned now_ms)
{
    if (g_settings_wb_tour.timeout_ms == 0)
    {
        return 0;
    }
    if (now_ms - g_settings_wb_tour.tour_start_ms >= g_settings_wb_tour.timeout_ms)
    {
        return 1;
    }
    return 0;
}

static void settings_wb_tour_begin_item(unsigned now_ms)
{
    g_settings_wb_tour.item_start_ms = now_ms;
    g_settings_wb_tour.phase_start_ms = now_ms;
    g_settings_wb_tour.key_step = 0;
    g_settings_wb_tour.focus_step = 0;
    g_settings_wb_tour.esc_try = 0;
    g_settings_wb_tour.last_key_ms = 0;
    if (!settings_wb_tour_get_item(g_settings_wb_tour.focus_idx,
        &g_settings_wb_tour.item_kind, &g_settings_wb_tour.item_menu_id))
    {
        snprintf(g_settings_wb_tour.fail_detail, sizeof(g_settings_wb_tour.fail_detail),
            "tour: bad focus_idx %d", g_settings_wb_tour.focus_idx);
        settings_wb_tour_set_fail(g_settings_wb_tour.fail_detail);
        return;
    }
    g_settings_wb_tour.phase = WB_TOUR_ST_ENSURE_ROOT;
}

static int settings_wb_tour_datetime_nested_active(int menu_id)
{
    Pdivoom_astrootoo_settings_ctrl ph;

#if DIVOOM_ASTRO_MANUAL_DATETIME_SET_ENABLE
    if (menu_id == DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_DATE_TIME_TIME
        || menu_id == DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_DATE_TIME_DATE
        || menu_id == DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_DATE_FORMAT)
#else
    if (menu_id == DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_DATE_FORMAT)
#endif
    {
        return 1;
    }
    ph = (Pdivoom_astrootoo_settings_ctrl)divoom_menu_control_get_clock_mode_menu_handle();
    if (!ph)
    {
        return 0;
    }
#if DIVOOM_ASTRO_MANUAL_DATETIME_SET_ENABLE
    if (ph->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_TIME_TIME
        || ph->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_TIME_DATE
        || ph->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_FORMAT)
#else
    if (ph->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_FORMAT)
#endif
    {
        return 1;
    }
    return 0;
}

static int settings_wb_tour_sub_key(int sub_menu_id, int step)
{
    switch (sub_menu_id)
    {
    case DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_LANGUGE:
    {
        /* 仅上下键探测，不 ENTER 避免触发 set_language/字体缓存竞态 */
        if (step == 0)
        {
            return DIVOOM_KEY_NEXT;
        }
        if (step == 1)
        {
            return DIVOOM_KEY_PREV;
        }
        break;
    }
    case DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_DATE_TIME:
    {
        /* 首行 Hour24 为开关；下移一格到 DateFormat 再 ENTER 进入嵌套子页 */
        if (step == 0)
        {
            return DIVOOM_KEY_NEXT;
        }
        if (step == 1)
        {
            return DIVOOM_KEY_PLAY;
        }
        break;
    }
    case DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_WIFI:
    {
        if (step == 0 || step == 1)
        {
            return DIVOOM_KEY_NEXT;
        }
        break;
    }
    case DIVOOM_MENU_CONTROL_ID_SLIDE_SETING_MENU:
    {
        /* 不按键：DOWN 会进入相簿选择(15)并触发重资源加载导致崩溃 */
        break;
    }
    default:
    {
        if (step == 0)
        {
            return DIVOOM_KEY_NEXT;
        }
        if (step == 1)
        {
            return DIVOOM_KEY_PREV;
        }
        break;
    }
    }
    return -1;
}

static int settings_wb_tour_sub_steps(int sub_menu_id)
{
    switch (sub_menu_id)
    {
    case DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_LANGUGE:
    {
        return 2;
    }
    case DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_DATE_TIME:
    {
        return 2;
    }
    case DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_WIFI:
    {
        return 2;
    }
    case DIVOOM_MENU_CONTROL_ID_SLIDE_SETING_MENU:
    {
        return 0;
    }
    default:
    {
        return 2;
    }
    }
}

void divoom_astrootoo_settings_whitebox_tour_reset(void)
{
    memset(&g_settings_wb_tour, 0, sizeof(g_settings_wb_tour));
    g_settings_wb_tour.active = 1;
    g_settings_wb_tour.focus_total = divoom_astrootoo_settings_count_focus();
    g_settings_wb_tour.timeout_ms = 240000u;
    g_settings_wb_tour.item_timeout_ms = 8000u;
}

void divoom_astrootoo_settings_whitebox_tour_configure(unsigned timeout_ms, unsigned item_timeout_ms)
{
    if (timeout_ms > 0)
    {
        g_settings_wb_tour.timeout_ms = timeout_ms;
    }
    if (item_timeout_ms > 0)
    {
        g_settings_wb_tour.item_timeout_ms = item_timeout_ms;
    }
}

const char *divoom_astrootoo_settings_whitebox_tour_fail_detail(void)
{
    return g_settings_wb_tour.fail_detail;
}

int divoom_astrootoo_settings_whitebox_tour_get_index(void)
{
    return g_settings_wb_tour.focus_idx;
}

int divoom_astrootoo_settings_whitebox_tour_is_done(void)
{
    return g_settings_wb_tour.done ? 1 : 0;
}

int divoom_astrootoo_settings_whitebox_tour_tick(unsigned now_ms)
{
    int menu_id;
    int key;
    int sub_steps;

    if (g_settings_wb_tour.fail)
    {
        return -1;
    }
    if (g_settings_wb_tour.tour_start_ms == 0)
    {
        g_settings_wb_tour.tour_start_ms = now_ms;
    }
    if (settings_wb_tour_total_timeout(now_ms))
    {
        snprintf(g_settings_wb_tour.fail_detail, sizeof(g_settings_wb_tour.fail_detail),
            "tour: total timeout at idx %d menu %d",
            g_settings_wb_tour.focus_idx, divoom_menu_control_get_active_menu_id());
        settings_wb_tour_set_fail(g_settings_wb_tour.fail_detail);
        return -1;
    }
    if (g_settings_wb_tour.focus_idx >= g_settings_wb_tour.focus_total)
    {
        g_settings_wb_tour.done = 1;
        g_settings_wb_tour.active = 0;
        return 1;
    }

    menu_id = divoom_menu_control_get_active_menu_id();

    if (settings_wb_tour_item_timeout(now_ms)
        && g_settings_wb_tour.phase != WB_TOUR_ST_GAP)
    {
        snprintf(g_settings_wb_tour.fail_detail, sizeof(g_settings_wb_tour.fail_detail),
            "tour: item timeout idx %d kind %d item_menu %d cur %d phase %d step %d",
            g_settings_wb_tour.focus_idx, g_settings_wb_tour.item_kind,
            g_settings_wb_tour.item_menu_id, menu_id,
            g_settings_wb_tour.phase, g_settings_wb_tour.key_step);
        settings_wb_tour_set_fail(g_settings_wb_tour.fail_detail);
        return -1;
    }

    if (g_settings_wb_tour.phase == WB_TOUR_ST_IDLE)
    {
        settings_wb_tour_begin_item(now_ms);
        return 0;
    }

    if (!settings_wb_tour_can_inject(now_ms))
    {
        return 0;
    }

    switch (g_settings_wb_tour.phase)
    {
    case WB_TOUR_ST_ENSURE_ROOT:
    {
        if (menu_id == DIVOOM_MENU_CONTROL_ID_ACCEPT_APP_PHOTO_VIEW)
        {
            settings_wb_tour_inject(DIVOOM_KEY_ESC, now_ms);
            break;
        }
        if (menu_id == DIVOOM_MENU_CONTROL_ID_CONFIG_WIFI)
        {
            settings_wb_tour_inject(DIVOOM_KEY_ESC, now_ms);
            break;
        }
        if (menu_id != DIVOOM_MENU_CONTROL_ID_SYSTEM_SETING_MENU)
        {
            if (g_settings_wb_tour.esc_try < WB_TOUR_ESC_MAX)
            {
                g_settings_wb_tour.esc_try++;
                settings_wb_tour_inject(DIVOOM_KEY_ESC, now_ms);
            }
            else
            {
                snprintf(g_settings_wb_tour.fail_detail, sizeof(g_settings_wb_tour.fail_detail),
                    "tour: cannot reach settings root idx %d got %d",
                    g_settings_wb_tour.focus_idx, menu_id);
                settings_wb_tour_set_fail(g_settings_wb_tour.fail_detail);
            }
            break;
        }
        if (g_settings_wb_tour.focus_idx == 0)
        {
            (void)settings_wb_tour_focus_direct(0);
            g_settings_wb_tour.phase = WB_TOUR_ST_ROOT_ACT;
            g_settings_wb_tour.key_step = 0;
            g_settings_wb_tour.item_start_ms = now_ms;
            g_settings_wb_tour.phase_start_ms = now_ms;
        }
        else
        {
            g_settings_wb_tour.phase = WB_TOUR_ST_FOCUS;
            g_settings_wb_tour.phase_start_ms = now_ms;
        }
        break;
    }
    case WB_TOUR_ST_FOCUS:
    {
        if (menu_id != DIVOOM_MENU_CONTROL_ID_SYSTEM_SETING_MENU)
        {
            g_settings_wb_tour.phase = WB_TOUR_ST_ENSURE_ROOT;
            break;
        }
        if (settings_wb_tour_focus_direct(g_settings_wb_tour.focus_idx))
        {
            g_settings_wb_tour.phase = WB_TOUR_ST_ROOT_ACT;
            g_settings_wb_tour.key_step = 0;
            g_settings_wb_tour.item_start_ms = now_ms;
            g_settings_wb_tour.phase_start_ms = now_ms;
        }
        else if (g_settings_wb_tour.focus_step < WB_TOUR_FOCUS_PREV_CNT)
        {
            settings_wb_tour_inject(DIVOOM_KEY_PREV, now_ms);
            g_settings_wb_tour.focus_step++;
        }
        else if (g_settings_wb_tour.focus_step < WB_TOUR_FOCUS_PREV_CNT + g_settings_wb_tour.focus_idx)
        {
            settings_wb_tour_inject(DIVOOM_KEY_NEXT, now_ms);
            g_settings_wb_tour.focus_step++;
        }
        else
        {
            g_settings_wb_tour.phase = WB_TOUR_ST_ROOT_ACT;
            g_settings_wb_tour.key_step = 0;
            g_settings_wb_tour.item_start_ms = now_ms;
            g_settings_wb_tour.phase_start_ms = now_ms;
        }
        break;
    }
    case WB_TOUR_ST_ROOT_ACT:
    {
        if (menu_id != DIVOOM_MENU_CONTROL_ID_SYSTEM_SETING_MENU)
        {
            g_settings_wb_tour.phase = WB_TOUR_ST_ENSURE_ROOT;
            break;
        }
        if (g_settings_wb_tour.item_kind == DIVOOM_ASTRO_SET_KIND_BRIGHT
            || g_settings_wb_tour.item_kind == DIVOOM_ASTRO_SET_KIND_VOLUME)
        {
            if (g_settings_wb_tour.key_step == 0)
            {
                settings_wb_tour_inject(DIVOOM_KEY_LIGHT_LEFT, now_ms);
                g_settings_wb_tour.key_step = 1;
            }
            else
            {
                settings_wb_tour_inject(DIVOOM_KEY_LIGHT_RIGHT, now_ms);
                g_settings_wb_tour.focus_idx++;
                g_settings_wb_tour.phase = WB_TOUR_ST_GAP;
                g_settings_wb_tour.phase_start_ms = now_ms;
            }
            break;
        }
        if (g_settings_wb_tour.item_kind == DIVOOM_ASTRO_SET_KIND_TOGGLE)
        {
            if (g_settings_wb_tour.key_step == 0)
            {
                settings_wb_tour_inject(DIVOOM_KEY_NEXT, now_ms);
                g_settings_wb_tour.key_step = 1;
            }
            else
            {
                settings_wb_tour_inject(DIVOOM_KEY_PREV, now_ms);
                g_settings_wb_tour.focus_idx++;
                g_settings_wb_tour.phase = WB_TOUR_ST_GAP;
                g_settings_wb_tour.phase_start_ms = now_ms;
            }
            break;
        }
        if (g_settings_wb_tour.item_kind == DIVOOM_ASTRO_SET_KIND_ACTION)
        {
            /* 仅上下键经过，不 ENTER 确认框（避免 overlay/group 与快速切页竞态） */
            if (g_settings_wb_tour.key_step == 0)
            {
                settings_wb_tour_inject(DIVOOM_KEY_NEXT, now_ms);
                g_settings_wb_tour.key_step = 1;
            }
            else
            {
                settings_wb_tour_inject(DIVOOM_KEY_PREV, now_ms);
                g_settings_wb_tour.focus_idx++;
                g_settings_wb_tour.phase = WB_TOUR_ST_GAP;
                g_settings_wb_tour.phase_start_ms = now_ms;
            }
            break;
        }
        if (g_settings_wb_tour.item_kind == DIVOOM_ASTRO_SET_KIND_LINK && g_settings_wb_tour.item_menu_id == 0)
        {
            if (g_settings_wb_tour.key_step == 0)
            {
                settings_wb_tour_inject(DIVOOM_KEY_NEXT, now_ms);
                g_settings_wb_tour.key_step = 1;
            }
            else
            {
                settings_wb_tour_inject(DIVOOM_KEY_PREV, now_ms);
                g_settings_wb_tour.focus_idx++;
                g_settings_wb_tour.phase = WB_TOUR_ST_GAP;
                g_settings_wb_tour.phase_start_ms = now_ms;
            }
            break;
        }
        if (g_settings_wb_tour.item_kind == DIVOOM_ASTRO_SET_KIND_LINK && g_settings_wb_tour.item_menu_id != 0)
        {
            settings_wb_tour_inject(DIVOOM_KEY_PLAY, now_ms);
            g_settings_wb_tour.phase = WB_TOUR_ST_WAIT_SUB;
            g_settings_wb_tour.phase_start_ms = now_ms;
            break;
        }
        g_settings_wb_tour.focus_idx++;
        g_settings_wb_tour.phase = WB_TOUR_ST_GAP;
        break;
    }
    case WB_TOUR_ST_WAIT_SUB:
    {
        if (menu_id == g_settings_wb_tour.item_menu_id)
        {
            g_settings_wb_tour.key_step = 0;
            g_settings_wb_tour.phase = WB_TOUR_ST_SUB_ACT;
            g_settings_wb_tour.phase_start_ms = now_ms;
        }
        else if (now_ms - g_settings_wb_tour.phase_start_ms >= 6000u)
        {
            snprintf(g_settings_wb_tour.fail_detail, sizeof(g_settings_wb_tour.fail_detail),
                "tour: wait sub idx %d exp %d got %d",
                g_settings_wb_tour.focus_idx, g_settings_wb_tour.item_menu_id, menu_id);
            settings_wb_tour_set_fail(g_settings_wb_tour.fail_detail);
        }
        break;
    }
    case WB_TOUR_ST_SUB_ACT:
    {
        if (g_settings_wb_tour.item_menu_id == DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_WIFI
            && menu_id == DIVOOM_MENU_CONTROL_ID_CONFIG_WIFI)
        {
            settings_wb_tour_inject(DIVOOM_KEY_ESC, now_ms);
            break;
        }
        sub_steps = settings_wb_tour_sub_steps(g_settings_wb_tour.item_menu_id);
        if (g_settings_wb_tour.key_step >= sub_steps)
        {
            if (g_settings_wb_tour.item_menu_id == DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_LANGUGE)
            {
                g_settings_wb_tour.phase = WB_TOUR_ST_ESC_ROOT;
            }
            else
            {
                g_settings_wb_tour.phase = WB_TOUR_ST_ESC_ROOT;
            }
            g_settings_wb_tour.esc_try = 0;
            g_settings_wb_tour.phase_start_ms = now_ms;
            break;
        }
        key = settings_wb_tour_sub_key(g_settings_wb_tour.item_menu_id, g_settings_wb_tour.key_step);
        if (key < 0)
        {
            g_settings_wb_tour.phase = WB_TOUR_ST_ESC_ROOT;
            break;
        }
        settings_wb_tour_inject(key, now_ms);
        if (g_settings_wb_tour.item_menu_id == DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_DATE_TIME
            && g_settings_wb_tour.key_step == 1)
        {
            g_settings_wb_tour.phase = WB_TOUR_ST_WAIT_NESTED;
            g_settings_wb_tour.phase_start_ms = now_ms;
        }
        g_settings_wb_tour.key_step++;
        break;
    }
    case WB_TOUR_ST_WAIT_NESTED:
    {
        if (settings_wb_tour_datetime_nested_active(menu_id))
        {
            settings_wb_tour_inject(DIVOOM_KEY_ESC, now_ms);
            g_settings_wb_tour.phase = WB_TOUR_ST_ESC_ROOT;
            g_settings_wb_tour.esc_try = 0;
            g_settings_wb_tour.phase_start_ms = now_ms;
        }
        else if (now_ms - g_settings_wb_tour.phase_start_ms >= 6000u)
        {
            snprintf(g_settings_wb_tour.fail_detail, sizeof(g_settings_wb_tour.fail_detail),
                "tour: wait nested datetime idx %d got %d",
                g_settings_wb_tour.focus_idx, menu_id);
            settings_wb_tour_set_fail(g_settings_wb_tour.fail_detail);
        }
        break;
    }
    case WB_TOUR_ST_ESC_ROOT:
    {
        if (menu_id == DIVOOM_MENU_CONTROL_ID_SYSTEM_SETING_MENU)
        {
            g_settings_wb_tour.focus_idx++;
            g_settings_wb_tour.phase = WB_TOUR_ST_GAP;
            g_settings_wb_tour.phase_start_ms = now_ms;
            break;
        }
        if (g_settings_wb_tour.esc_try < WB_TOUR_ESC_MAX)
        {
            g_settings_wb_tour.esc_try++;
            settings_wb_tour_inject(DIVOOM_KEY_ESC, now_ms);
        }
        else
        {
            snprintf(g_settings_wb_tour.fail_detail, sizeof(g_settings_wb_tour.fail_detail),
                "tour: ESC to root failed idx %d got %d",
                g_settings_wb_tour.focus_idx, menu_id);
            settings_wb_tour_set_fail(g_settings_wb_tour.fail_detail);
        }
        break;
    }
    case WB_TOUR_ST_GAP:
    {
        if (now_ms - g_settings_wb_tour.phase_start_ms >= 500u)
        {
            settings_wb_tour_begin_item(now_ms);
        }
        break;
    }
    default:
    {
        g_settings_wb_tour.phase = WB_TOUR_ST_IDLE;
        break;
    }
    }

    if (g_settings_wb_tour.fail)
    {
        return -1;
    }
    if (g_settings_wb_tour.done)
    {
        return 1;
    }
    return 0;
}

#else /* !DIVOOM_SIMULATOR */

void divoom_astrootoo_settings_whitebox_tour_reset(void)
{
}

void divoom_astrootoo_settings_whitebox_tour_configure(unsigned timeout_ms, unsigned item_timeout_ms)
{
    (void)timeout_ms;
    (void)item_timeout_ms;
}

const char *divoom_astrootoo_settings_whitebox_tour_fail_detail(void)
{
    return "";
}

int divoom_astrootoo_settings_whitebox_tour_get_index(void)
{
    return 0;
}

int divoom_astrootoo_settings_whitebox_tour_is_done(void)
{
    return 1;
}

int divoom_astrootoo_settings_whitebox_tour_tick(unsigned now_ms)
{
    (void)now_ms;
    return 1;
}

#endif /* DIVOOM_SIMULATOR */

#ifndef _divoom_astrootoo_settings_internal_h_
#define _divoom_astrootoo_settings_internal_h_

#include "lvgl.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_astrootoo_settings.h"
#include "divoom_menu_control.h"

/* card_item_style pad_all=8；列表宽度公式须计入 pad + 内容区 align */
#define DIVOOM_ASTRO_SET_CARD_PAD_H           8
#define DIVOOM_ASTRO_SET_CARD_LABEL_INSET_L   (8 + DIVOOM_ASTRO_SET_CARD_PAD_H)
#define DIVOOM_ASTRO_SET_CARD_LABEL_INSET_R   (8 + DIVOOM_ASTRO_SET_CARD_PAD_H)
#define DIVOOM_ASTRO_SET_WIFI_LABEL_INSET_L   (12 + DIVOOM_ASTRO_SET_CARD_PAD_H)
#define DIVOOM_ASTRO_SET_WIFI_LABEL_INSET_R   (8 + DIVOOM_ASTRO_SET_CARD_PAD_H)
#define DIVOOM_ASTRO_SET_WIFI_STATUS_INSET_R  (14 + DIVOOM_ASTRO_SET_CARD_PAD_H)

#define DIVOOM_ASTRO_SET_MAX_FOCUS      24
#define DIVOOM_ASTRO_SET_LANG_COUNT     DIVOOM_LVGL_MENU_LANG_END
#define DIVOOM_ASTRO_SET_PICKER_MAX     10
#define DIVOOM_ASTRO_SET_BT_MAX_ROW     28
#define DIVOOM_ASTRO_SET_WIFI_MAX_ROW   20
#define DIVOOM_ASTRO_SET_WIFI_KEY_COUNT 69
#define DIVOOM_ASTRO_SP_SCHEDULE_COUNT  10
#define DIVOOM_ASTRO_SP_REPEAT_ROWS     8
#define DIVOOM_ASTRO_SP_EDIT_FIELDS     3

/* 时间编辑子页（AlarmScreen --time-pick-*，夜览/定时开关机共用） */
#define DIVOOM_ASTRO_TP_WIDE_W            416
#define DIVOOM_ASTRO_TP_HALF_W            204
#define DIVOOM_ASTRO_TP_CELL_H            56
#define DIVOOM_ASTRO_TP_ROW_GAP           16
#define DIVOOM_ASTRO_TP_COLON_W           20
#define DIVOOM_ASTRO_TP_SEG_GAP           10
#define DIVOOM_ASTRO_TP_SEG_W             ((DIVOOM_ASTRO_TP_WIDE_W - DIVOOM_ASTRO_TP_COLON_W \
    - DIVOOM_ASTRO_TP_SEG_GAP * 2) / 2)
#define DIVOOM_ASTRO_TP_SEG_H             132
#define DIVOOM_ASTRO_TP_DIGIT_W           168
#define DIVOOM_ASTRO_TP_DIGIT_H           124
#define DIVOOM_ASTRO_TP_DIGIT_RADIUS      16
#define DIVOOM_ASTRO_TP_ACTION_GAP        8
#define DIVOOM_ASTRO_TP_MENU_FONT         24
#define DIVOOM_ASTRO_TP_FOCUS_BORDER      0xB8B0A8
#define DIVOOM_ASTRO_TP_CHEV_W            12
#define DIVOOM_ASTRO_TP_CHEV_H            8
#define DIVOOM_ASTRO_TP_COL_GAP           10
#define DIVOOM_ASTRO_TP_COL_H             (DIVOOM_ASTRO_TP_CHEV_H + DIVOOM_ASTRO_TP_COL_GAP \
    + DIVOOM_ASTRO_TP_DIGIT_H + DIVOOM_ASTRO_TP_COL_GAP + DIVOOM_ASTRO_TP_CHEV_H)
#define DIVOOM_ASTRO_TP_TRAY_H            (DIVOOM_ASTRO_TP_COL_H + DIVOOM_ASTRO_TP_ROW_GAP \
    + DIVOOM_ASTRO_TP_CELL_H)
#define DIVOOM_ASTRO_TP_X                 ((DIVOOM_SCREEN_WIDTH - DIVOOM_ASTRO_TP_WIDE_W) / 2)
#define DIVOOM_ASTRO_TP_Y                 (135-72)
#define DIVOOM_ASTRO_TP_CHEV_UP           "26/arrow_up.png"
#define DIVOOM_ASTRO_TP_CHEV_DN           "26/arrow_down.png"
/* 12h · AM/PM 82px + 时/分各 140px（真源 AlarmScreen.module.css） */
#define DIVOOM_ASTRO_TP_AMPM_W            82
#define DIVOOM_ASTRO_TP_UNIT_W            140
#define DIVOOM_ASTRO_TP_UNIT_DIGIT_W      120
#define DIVOOM_ASTRO_TP_12H_ROW_W         (DIVOOM_ASTRO_TP_AMPM_W + DIVOOM_ASTRO_TP_SEG_GAP \
    + DIVOOM_ASTRO_TP_UNIT_W + DIVOOM_ASTRO_TP_COLON_W + DIVOOM_ASTRO_TP_SEG_GAP \
    + DIVOOM_ASTRO_TP_UNIT_W)

/* 系统时间编辑子页（真源 DeviceSettingsDateTimePick.module.css，带外框容器 pickTray） */
#define DIVOOM_ASTRO_DTTP_TRAY_W          432   /* .pickTray max-width */
#define DIVOOM_ASTRO_DTTP_PAD_TOP         22    /* padding-top */
#define DIVOOM_ASTRO_DTTP_PAD_X           14    /* padding 左右 */
#define DIVOOM_ASTRO_DTTP_PAD_BOT         26    /* padding-bottom */
#define DIVOOM_ASTRO_DTTP_BOX_H           176   /* --pick-box-h */
#define DIVOOM_ASTRO_DTTP_CAP_GAP         12    /* .segment column gap（box->caption） */
#define DIVOOM_ASTRO_DTTP_CAP_H           34    /* caption 行高 */
#define DIVOOM_ASTRO_DTTP_AMPM_W          82    /* .segmentAmPm */
#define DIVOOM_ASTRO_DTTP_UNIT_W          140   /* .segmentRow12h .segmentUnit box */
#define DIVOOM_ASTRO_DTTP_UNIT24_W        184   /* 24h box flex max-width 184 */
#define DIVOOM_ASTRO_DTTP_GAP_12H         8     /* .segmentRow12h gap */
#define DIVOOM_ASTRO_DTTP_GAP_24H         12    /* .segmentRow gap */
#define DIVOOM_ASTRO_DTTP_COLON_W         8     /* .colonDot width */
#define DIVOOM_ASTRO_DTTP_COLON_DOT       8
#define DIVOOM_ASTRO_DTTP_COLON_GAP       16    /* .colon gap */
#define DIVOOM_ASTRO_DTTP_DIGIT_FONT      80    /* BitmapText size24 scale4 ≈ 大像素数字 */
#define DIVOOM_ASTRO_DTTP_AMPM_FONT       28
#define DIVOOM_ASTRO_DTTP_CAP_FONT        24
#define DIVOOM_ASTRO_DTTP_BAR_W           416   /* --time-pick-wide-w */
#define DIVOOM_ASTRO_DTTP_BAR_HALF_W      204   /* --time-pick-half-w */
#define DIVOOM_ASTRO_DTTP_BAR_H           56    /* --time-pick-cell-h */
#define DIVOOM_ASTRO_DTTP_BAR_GAP         8     /* .timePickActions gap */
#define DIVOOM_ASTRO_DTTP_STACK_GAP       12    /* .pickStack gap（tray->bar） */
#define DIVOOM_ASTRO_DTTP_TRAY_H          (DIVOOM_ASTRO_DTTP_PAD_TOP + DIVOOM_ASTRO_DTTP_BOX_H \
    + DIVOOM_ASTRO_DTTP_CAP_GAP + DIVOOM_ASTRO_DTTP_CAP_H + DIVOOM_ASTRO_DTTP_PAD_BOT)
#define DIVOOM_ASTRO_DTTP_STACK_H         (DIVOOM_ASTRO_DTTP_TRAY_H + DIVOOM_ASTRO_DTTP_STACK_GAP \
    + DIVOOM_ASTRO_DTTP_BAR_H)

typedef struct
{
    lv_obj_t *tray;
    lv_obj_t *ampm_box;
    lv_obj_t *ampm_lbl;
    lv_obj_t *h_box;
    lv_obj_t *m_box;
    lv_obj_t *h_lbl;
    lv_obj_t *m_lbl;
    lv_obj_t *h_cap;
    lv_obj_t *m_cap;
    lv_obj_t *cancel;
    lv_obj_t *confirm;
    int use_hour24;
} divoom_astrootoo_time_pick_widgets_t;

typedef struct _divoom_astrootoo_settings_ctrl divoom_astrootoo_settings_ctrl, *Pdivoom_astrootoo_settings_ctrl;

void divoom_astrootoo_settings_apply_time_pick_segment(lv_obj_t *obj, int focused);
void divoom_astrootoo_settings_apply_time_pick_slot(lv_obj_t *obj, int focused);
lv_coord_t divoom_astrootoo_settings_time_pick_stack_y(lv_coord_t area_h);
void divoom_astrootoo_settings_create_time_pick_stack(lv_obj_t *parent, int menu_id,
    const lv_style_t *content_style, lv_coord_t stack_y,
    divoom_astrootoo_time_pick_widgets_t *widgets);

typedef struct _divoom_astrootoo_settings_sp_item
{
    uint8 hour;
    uint8 minute;
    uint8 enabled;
    uint8 weekday_mask;
    uint8 action;
} divoom_astrootoo_settings_sp_item_t;

typedef struct
{
    int kind;
    const char *key;
    int menu_id;
} divoom_astrootoo_settings_item_ref_t;

typedef struct _divoom_astrootoo_settings_ctrl
{
    divoom_astrootoo_ui_base ui;
    divoom_astrootoo_settings_page_t page;
    int menu_id;
    lv_obj_t *root_obj;
    lv_obj_t *content;
    void *direct_handle;

    const divoom_astrootoo_settings_item_ref_t *focus_items[DIVOOM_ASTRO_SET_MAX_FOCUS];
    int focus_cnt;
    lv_obj_t *bright_slider;
    lv_obj_t *vol_slider;
    lv_obj_t *bright_card;
    lv_obj_t *vol_card;

    int pref_pixelgrid_on;
    lv_obj_t *pixelgrid_img;
    lv_obj_t *battery_img;
    lv_timer_t *battery_timer;
    uint8 battery_icon_sig;

    lv_obj_t *slider;
    lv_obj_t *slider_pct_lbl;
    const char *title_key;

    int night_on;
    int night_bright;
    int night_start_h;
    int night_start_m;
    int night_end_h;
    int night_end_m;
    int night_screen_mode;
    int night_time_target;
    int night_pick_hour;
    int night_pick_min;
    int night_pick_field;
    int night_pick_base_h;
    int night_pick_base_m;
    uint8 night_dirty;
    uint8 night_preview_active;
    lv_obj_t *night_header;
    lv_obj_t *night_header_lbl;
    lv_obj_t *night_toggle_card;
    lv_obj_t *night_toggle_img;
    lv_obj_t *night_sec_bright;
    lv_obj_t *night_sec_period;
    lv_obj_t *night_bright_card;
    lv_obj_t *night_bright_slider;
    int key_light_bright;
    lv_obj_t *key_light_sec_bright;
    lv_obj_t *key_light_sec_effects;
    lv_obj_t *key_light_bright_card;
    lv_obj_t *key_light_bright_slider;
    lv_obj_t *night_start_card;
    lv_obj_t *night_end_card;
    lv_obj_t *night_start_row_lbl;
    lv_obj_t *night_end_row_lbl;
    lv_obj_t *night_start_val_lbl;
    lv_obj_t *night_end_val_lbl;
    lv_obj_t *night_start_chev;
    lv_obj_t *night_end_chev;
    lv_obj_t *night_time_cont;
    lv_obj_t *night_time_ampm_box;
    lv_obj_t *night_time_ampm_lbl;
    lv_obj_t *night_time_h_box;
    lv_obj_t *night_time_m_box;
    lv_obj_t *night_time_h_lbl;
    lv_obj_t *night_time_m_lbl;
    lv_obj_t *night_time_h_cap;
    lv_obj_t *night_time_m_cap;
    lv_obj_t *night_time_cancel;
    lv_obj_t *night_time_confirm;
    lv_obj_t *night_key_btn;

    int lang_focus;
    lv_obj_t *lang_row[DIVOOM_ASTRO_SET_LANG_COUNT];
    lv_obj_t *lang_check[DIVOOM_ASTRO_SET_LANG_COUNT];

    int bt_row_count;
    int bt_focus;
    lv_obj_t *bt_row[DIVOOM_ASTRO_SET_BT_MAX_ROW];
    lv_obj_t *bt_row_lbl[DIVOOM_ASTRO_SET_BT_MAX_ROW];
    lv_obj_t *bt_row_val[DIVOOM_ASTRO_SET_BT_MAX_ROW];
    lv_obj_t *bt_row_trail[DIVOOM_ASTRO_SET_BT_MAX_ROW];
    lv_obj_t *bt_sw_img;
    uint8 bt_row_kind[DIVOOM_ASTRO_SET_BT_MAX_ROW];
    int8 bt_row_dev_idx[DIVOOM_ASTRO_SET_BT_MAX_ROW];
    uint8 bt_sw_on_sig;
    uint8 bt_linked_sig;
    uint8 bt_device_list_sig;
    int8 bt_checked_row;
    uint8 bt_switch_stage; /* 0=无 1=已发断开等待断开 2=已发连接等待连上目标 */
    uint16 bt_switch_wait_tick;
    char bt_switch_target_addr[18];
    lv_timer_t *bt_timer;

    int wifi_row_count;
    int wifi_focus;
    int wifi_net_count;
    char wifi_ssid[DIVOOM_ASTRO_SET_WIFI_MAX_ROW][128];
    char wifi_pswd[DIVOOM_ASTRO_SET_WIFI_MAX_ROW][128];
    lv_obj_t *wifi_row[DIVOOM_ASTRO_SET_WIFI_MAX_ROW];
    lv_obj_t *wifi_row_val[DIVOOM_ASTRO_SET_WIFI_MAX_ROW];
    lv_obj_t *wifi_row_chk[DIVOOM_ASTRO_SET_WIFI_MAX_ROW];
    lv_obj_t *wifi_saved_title;
    lv_obj_t *wifi_info_separator;
    uint8 wifi_row_kind[DIVOOM_ASTRO_SET_WIFI_MAX_ROW];
    uint8 wifi_authmode[DIVOOM_ASTRO_SET_WIFI_MAX_ROW];
    uint8 wifi_saved[DIVOOM_ASTRO_SET_WIFI_MAX_ROW];
    int8 wifi_rssi[DIVOOM_ASTRO_SET_WIFI_MAX_ROW];
    int8 wifi_row_net_idx[DIVOOM_ASTRO_SET_WIFI_MAX_ROW];
    uint8 wifi_device_mac[6];
    uint8 wifi_device_mac_ready;
    uint32 wifi_device_mac_last_try_ms;
    uint8 wifi_device_ip[4];
    uint8 wifi_device_ip_ready;
    uint8 wifi_status_refresh_pending;
    lv_timer_t *wifi_timer;
    lv_timer_t *wifi_toast_timer;
    lv_obj_t *wifi_hint_lbl;
    lv_obj_t *wifi_scan_key_btn;
    lv_obj_t *wifi_connect_spinner;
    lv_obj_t *wifi_password_toast;
    lv_img_dsc_t wifi_loading_dsc;
    uint8 *wifi_loading_buf;
    void *wifi_loading_gif_handle;
    uint8 wifi_scan_applied;
    uint8 wifi_scan_finished;
    uint32 wifi_scan_begin_ms;
    lv_obj_t *wifi_password_field_lbl;
    lv_obj_t *wifi_password_toolbar[2];
    lv_obj_t *wifi_password_key[DIVOOM_ASTRO_SET_WIFI_KEY_COUNT];
    lv_obj_t *wifi_password_key_btn;
    int wifi_password_focus_zone;
    int wifi_password_toolbar_focus;
    int wifi_password_key_focus;
    int wifi_password_len;
    uint8 wifi_password_uppercase;
    char wifi_password[65];
    lv_obj_t *wifi_connect_notice;
    lv_obj_t *wifi_connect_card;
    lv_obj_t *wifi_connect_icon;
    lv_obj_t *wifi_connect_divider;
    lv_obj_t *wifi_connect_notice_lbl;
    lv_obj_t *wifi_connect_subtitle_lbl;

    int picker_cnt;
    int picker_committed;
    lv_obj_t *picker_row[DIVOOM_ASTRO_SET_PICKER_MAX];
    lv_obj_t *picker_check[DIVOOM_ASTRO_SET_PICKER_MAX];
    void *confirm_box;
    int confirm_action;
    /* 系统管理确认弹窗（真源 FlowtooConfirmModal：遮罩 + 暖色卡片 + 取消/确定双钮） */
    lv_obj_t *confirm_overlay;
    lv_obj_t *confirm_beta_body;
    lv_obj_t *confirm_cancel_btn;
    lv_obj_t *confirm_ok_btn;
    lv_obj_t *confirm_focus_restore;
    int confirm_focus;

    int list_row_count;
    lv_obj_t *list_row[8];
    lv_obj_t *list_row_val[8];
    lv_obj_t *list_row_sw[8];
    lv_obj_t *list_row_chev[8];
    uint8 list_row_kind[8];
    lv_obj_t *about_ver_lbl;
    /* 关于页 OTA 状态：网络响应写入主模块缓存，LVGL timer 只负责把新版本行同步到界面。 */
    lv_obj_t *about_update_dot;
    lv_timer_t *about_update_timer;
    uint8 about_update_row_ready;
    uint8 about_beta_features;
    /* 程序升级确认快照及按键交给 timer 的轻量动作。 */
    int about_firmware_version; /* 同一次确认绑定的固件版本，0 表示没有固件。 */
    int about_pending_action;
    lv_obj_t *system_check_overlay, *system_check_state, *system_check_progress;
    lv_obj_t *system_check_bar, *system_check_hint;
    int system_check_start_failed, system_check_finished;

    /* 设备连接码：分享码由主模块首次联网异步缓存，页面仅动态生成 QR。 */
    lv_obj_t *connection_code_key_btn;
    lv_obj_t *connection_code_frame;
    lv_obj_t *connection_code_qr;
    lv_img_dsc_t connection_code_qr_dsc;
    uint8 *connection_code_qr_buf;
    lv_obj_t *connection_code_value_lbl;
    lv_obj_t *connection_code_hint_lbl;
    lv_obj_t *connection_code_skip_lbl;
    lv_timer_t *connection_code_timer;
    uint32 connection_code_displayed;
    uint8 connection_code_ready;
    uint8 connection_code_wait_binding; /* 进入时未绑定，绑定完成后自动返回。 */

    /* 时间/日期手动调节子页 */
    int dt_pick_field;
    int dt_hour;
    int dt_minute;
    int dt_year;
    int dt_month;
    int dt_day;
    lv_obj_t *dt_key_btn;
    lv_obj_t *dt_pick_tray;
    lv_obj_t *dt_seg_box[3];
    lv_obj_t *dt_seg_lbl[3];
    lv_obj_t *dt_seg_cap[3];
    divoom_astrootoo_time_pick_widgets_t dt_tp_widgets;

    /* MCP（真源 DeviceSettingsPcWatchfaceMcpScreen.tsx） */
    int mcp_qr_index;
    lv_obj_t *mcp_key_btn;
    lv_obj_t *mcp_title_lbl;
    lv_obj_t *mcp_qr_img;
    lv_img_dsc_t mcp_qr_dsc;
    uint8 *mcp_qr_buf;
    int mcp_qr_rendered_index;
    lv_obj_t *mcp_hint_lbl;
    lv_obj_t *mcp_url_lbl[3];
    lv_obj_t *mcp_dot[2];

    /* 蓝牙密码（真源 BluetoothPasswordScreen.tsx） */
    int bt_pw_focus;
    int bt_pw_draft_enabled;
    int8 bt_pw_digits[4];
    int bt_pw_baseline_enabled;
    int8 bt_pw_baseline_digits[4];
    lv_obj_t *bt_pw_key_btn;
    lv_obj_t *bt_pw_toggle_card;
    lv_obj_t *bt_pw_toggle_sw;
    lv_obj_t *bt_pw_digit_btn[4];
    lv_obj_t *bt_pw_digit_lbl[4];
    lv_obj_t *bt_pw_save_btn;
    lv_obj_t *bt_pw_save_lbl;

    /* 定时开关机（真源 DeviceSettingsScheduledPowerScreen.tsx） */
    int sp_mode;
    int sp_list_focus;
    int sp_list_col;
    int sp_edit_field;
    int sp_editing_index;
    uint8 sp_edit_touched;
    divoom_astrootoo_settings_sp_item_t sp_items[DIVOOM_ASTRO_SP_SCHEDULE_COUNT];
    divoom_astrootoo_settings_sp_item_t sp_draft;
    int sp_time_hour;
    int sp_time_min;
    int sp_time_field;
    int sp_repeat_mask;
    int sp_repeat_focus;
    int sp_action_focus;
    lv_obj_t *sp_key_btn;
    lv_obj_t *sp_header;
    lv_obj_t *sp_header_lbl;
    lv_obj_t *sp_list_cont;
    lv_obj_t *sp_row_btn[DIVOOM_ASTRO_SP_SCHEDULE_COUNT];
    lv_obj_t *sp_row_body[DIVOOM_ASTRO_SP_SCHEDULE_COUNT];
    lv_obj_t *sp_row_icon[DIVOOM_ASTRO_SP_SCHEDULE_COUNT];
    lv_obj_t *sp_row_time[DIVOOM_ASTRO_SP_SCHEDULE_COUNT];
    lv_obj_t *sp_row_meridiem[DIVOOM_ASTRO_SP_SCHEDULE_COUNT];
    lv_obj_t *sp_row_sub[DIVOOM_ASTRO_SP_SCHEDULE_COUNT];
    lv_obj_t *sp_row_toggle[DIVOOM_ASTRO_SP_SCHEDULE_COUNT];
    lv_obj_t *sp_row_sw[DIVOOM_ASTRO_SP_SCHEDULE_COUNT];
    lv_obj_t *sp_edit_cont;
    lv_obj_t *sp_edit_row[DIVOOM_ASTRO_SP_EDIT_FIELDS];
    lv_obj_t *sp_edit_lbl[DIVOOM_ASTRO_SP_EDIT_FIELDS];
    lv_obj_t *sp_edit_val[DIVOOM_ASTRO_SP_EDIT_FIELDS];
    lv_obj_t *sp_edit_action_icon;
    lv_obj_t *sp_pick_cont;
    lv_obj_t *sp_pick_row[DIVOOM_ASTRO_SP_REPEAT_ROWS];
    lv_obj_t *sp_pick_icon[DIVOOM_ASTRO_SP_REPEAT_ROWS];
    lv_obj_t *sp_pick_lbl[DIVOOM_ASTRO_SP_REPEAT_ROWS];
    lv_obj_t *sp_pick_chk[DIVOOM_ASTRO_SP_REPEAT_ROWS];
    lv_obj_t *sp_time_cont;
    lv_obj_t *sp_time_ampm_box;
    lv_obj_t *sp_time_ampm_lbl;
    lv_obj_t *sp_time_h_box;
    lv_obj_t *sp_time_m_box;
    lv_obj_t *sp_time_h_lbl;
    lv_obj_t *sp_time_m_lbl;
    lv_obj_t *sp_time_h_cap;
    lv_obj_t *sp_time_m_cap;
    lv_obj_t *sp_time_cancel;
    lv_obj_t *sp_time_confirm;

    /* 幻灯片设置（真源 DeviceSettingsSlideshowScreen.tsx） */
    int slide_playback_pct;
    int slide_row_count;
    lv_obj_t *slide_row[10];
    lv_obj_t *slide_row_val[10];
    lv_obj_t *slide_row_sw[10];
    lv_obj_t *slide_row_slider[1];
    lv_obj_t *slide_reverse_lbl;
    lv_obj_t *slide_reverse_hint;
    uint8 slide_row_kind[10];
} divoom_astrootoo_settings_ctrl, *Pdivoom_astrootoo_settings_ctrl;

#endif

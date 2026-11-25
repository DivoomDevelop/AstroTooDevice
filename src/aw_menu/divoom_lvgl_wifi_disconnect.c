#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_astrootoo_theme.h"
#include "divoom_lvgl_top_control.h"

#define DIVOOM_WIFI_NOTICE_ACTION_W          80
#define DIVOOM_WIFI_NOTICE_ACTION_H          36
#define DIVOOM_WIFI_NOTICE_ACTION_RIGHT      14
#define DIVOOM_WIFI_NOTICE_AUTO_CLOSE_MS     6000
#define DIVOOM_WIFI_NOTICE_ICON_FILE         "82/at_toast_wifi_disconnected.png"

typedef struct _divoom_lvgl_wifi_disconnect_ctrl
{
    lv_obj_t *top_disp_obj;
    lv_timer_t *dismiss_timer;
    lv_style_t top_disp_style;
    lv_style_t action_style;
    uint8 closing;
} divoom_lvgl_wifi_disconnect_ctrl, *Pdivoom_lvgl_wifi_disconnect_ctrl;

static Pdivoom_lvgl_wifi_disconnect_ctrl pdivoom_lvgl_wifi_disconnect_handle = NULL;

static void divoom_lvgl_wifi_disconnect_request_close(
    Pdivoom_lvgl_wifi_disconnect_ctrl phandle, uint8 enter_wifi)
{
    if (!phandle || phandle != pdivoom_lvgl_wifi_disconnect_handle || phandle->closing)
    {
        return;
    }
    phandle->closing = 1;
    divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN,
        enter_wifi ? DIVOOM_MAIN_EVENT_WIFI_NOTICE_MENU_ENTER
                   : DIVOOM_MAIN_EVENT_WIFI_NOTICE_MENU_CANCLE);
}

static void divoom_lvgl_wifi_disconnect_timer_cb(lv_timer_t *timer)
{
    Pdivoom_lvgl_wifi_disconnect_ctrl phandle;

    phandle = (Pdivoom_lvgl_wifi_disconnect_ctrl)lv_timer_get_user_data(timer);
    if (!phandle || phandle != pdivoom_lvgl_wifi_disconnect_handle)
    {
        return;
    }
    /* repeat_count=1 时 LVGL 会在回调结束后自动删除 timer，避免 exit 二次 lv_timer_del */
    phandle->dismiss_timer = NULL;
    divoom_lvgl_wifi_disconnect_request_close(phandle, 0);
}

static void divoom_lvgl_wifi_disconnect_event_cb(lv_event_t *e)
{
    Pdivoom_lvgl_wifi_disconnect_ctrl phandle;
    uint8 *key_code;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    phandle = (Pdivoom_lvgl_wifi_disconnect_ctrl)lv_event_get_user_data(e);
    if (!phandle || phandle != pdivoom_lvgl_wifi_disconnect_handle)
    {
        return;
    }
    key_code = (uint8 *)lv_event_get_param(e);
    /* 顶层离线提示只保留一个动作：确认进入扫描页，其余任意键关闭。 */
    divoom_lvgl_wifi_disconnect_request_close(phandle,
        key_code && key_code[0] == DIVOOM_LVGL_KEY_ENTER);
}

void divoom_lvgl_wifi_disconnect_init(void)
{
    Pdivoom_lvgl_wifi_disconnect_ctrl phandle;
    lv_obj_t *icon;
    lv_obj_t *label;
    lv_obj_t *action;
    lv_obj_t *action_label;
    int active_menu_id;

    active_menu_id = divoom_menu_control_get_active_menu_id();
    if (active_menu_id == DIVOOM_MENU_CONTROL_ID_CONFIG_WIFI
        || active_menu_id == DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_WIFI_ADD
        || active_menu_id == DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_WIFI_PASSWORD)
    {
        return;
    }

    divoom_get_lvgl_lock();
    if (pdivoom_lvgl_wifi_disconnect_handle)
    {
        divoom_free_lvgl_lock();
        return;
    }
    phandle = divoom_system_malloc(sizeof(divoom_lvgl_wifi_disconnect_ctrl));
    if (!phandle)
    {
        divoom_free_lvgl_lock();
        return;
    }
    memset(phandle, 0, sizeof(*phandle));
    pdivoom_lvgl_wifi_disconnect_handle = phandle;

    lv_style_init(&phandle->top_disp_style);
    lv_style_set_bg_color(&phandle->top_disp_style, lv_color_hex(DIVOOM_ASTRO_BG));
    lv_style_set_bg_opa(&phandle->top_disp_style, LV_OPA_COVER);
    lv_style_set_pad_all(&phandle->top_disp_style, 0);
    lv_style_set_border_width(&phandle->top_disp_style, 2);
    lv_style_set_border_color(&phandle->top_disp_style, lv_color_hex(DIVOOM_ASTRO_STROKE));
    lv_style_set_outline_width(&phandle->top_disp_style, 0);
    lv_style_set_radius(&phandle->top_disp_style, DIVOOM_ASTRO_RADIUS_SM);
    lv_style_set_shadow_width(&phandle->top_disp_style, 12);
    lv_style_set_shadow_color(&phandle->top_disp_style, lv_color_hex(DIVOOM_ASTRO_INK));
    lv_style_set_shadow_opa(&phandle->top_disp_style, LV_OPA_10);
    lv_style_set_shadow_ofs_y(&phandle->top_disp_style, 4);

    lv_style_init(&phandle->action_style);
    lv_style_set_bg_color(&phandle->action_style, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS));
    lv_style_set_bg_opa(&phandle->action_style, LV_OPA_COVER);
    lv_style_set_pad_all(&phandle->action_style, 0);
    lv_style_set_border_width(&phandle->action_style, 2);
    lv_style_set_border_color(&phandle->action_style, lv_color_hex(DIVOOM_ASTRO_STROKE));
    lv_style_set_outline_width(&phandle->action_style, 0);
    lv_style_set_radius(&phandle->action_style, 16);
    lv_style_set_shadow_width(&phandle->action_style, 0);

    phandle->top_disp_obj = lv_btn_create(lv_layer_top());
    lv_obj_add_style(phandle->top_disp_obj, &phandle->top_disp_style, 0);
    lv_obj_set_pos(phandle->top_disp_obj, DIVOOM_ASTRO_TOP_TOAST_X,
        DIVOOM_ASTRO_TOP_TOAST_Y);
    lv_obj_set_size(phandle->top_disp_obj, DIVOOM_ASTRO_TOP_TOAST_W,
        DIVOOM_ASTRO_TOP_TOAST_H);
    lv_obj_set_style_outline_width(phandle->top_disp_obj, 0,
        LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_clear_flag(phandle->top_disp_obj, LV_OBJ_FLAG_SCROLLABLE);

    icon = lv_img_create(phandle->top_disp_obj);
    divoom_menu_control_lv_img_set_src(icon, DIVOOM_WIFI_NOTICE_ICON_FILE);
    lv_obj_set_pos(icon, DIVOOM_ASTRO_TOP_TOAST_PAD_X,
        (DIVOOM_ASTRO_TOP_TOAST_H - DIVOOM_ASTRO_TOP_TOAST_ICON) / 2);
    lv_obj_set_size(icon, DIVOOM_ASTRO_TOP_TOAST_ICON, DIVOOM_ASTRO_TOP_TOAST_ICON);

    action = lv_obj_create(phandle->top_disp_obj);
    lv_obj_add_style(action, &phandle->action_style, 0);
    lv_obj_set_size(action, DIVOOM_WIFI_NOTICE_ACTION_W, DIVOOM_WIFI_NOTICE_ACTION_H);
    lv_obj_set_pos(action,
        DIVOOM_ASTRO_TOP_TOAST_W - DIVOOM_WIFI_NOTICE_ACTION_RIGHT
            - DIVOOM_WIFI_NOTICE_ACTION_W,
        (DIVOOM_ASTRO_TOP_TOAST_H - DIVOOM_WIFI_NOTICE_ACTION_H) / 2);
    lv_obj_clear_flag(action, LV_OBJ_FLAG_SCROLLABLE);

    action_label = lv_label_create(action);
    lv_label_set_text(action_label, divoom_menu_control_get_disp_str("EnterWifiConfig"));
    lv_label_set_long_mode(action_label, LV_LABEL_LONG_DOT);
    lv_obj_set_width(action_label, DIVOOM_WIFI_NOTICE_ACTION_W - 12);
    lv_obj_set_style_text_align(action_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(action_label,
        divoom_menu_control_get_font_info(16, DIVOOM_MENU_CONTROL_ID_TOP_WIFI_NOTICE_MENU), 0);
    lv_obj_set_style_text_color(action_label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_center(action_label);

    label = lv_label_create(phandle->top_disp_obj);
    lv_label_set_text(label, divoom_menu_control_get_disp_str("WifiDisconnectTip"));
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_obj_set_pos(label,
        DIVOOM_ASTRO_TOP_TOAST_PAD_X + DIVOOM_ASTRO_TOP_TOAST_ICON
            + DIVOOM_ASTRO_TOP_TOAST_GAP,
        0);
    lv_obj_set_size(label,
        DIVOOM_ASTRO_TOP_TOAST_W - DIVOOM_ASTRO_TOP_TOAST_PAD_X
            - DIVOOM_ASTRO_TOP_TOAST_ICON - DIVOOM_ASTRO_TOP_TOAST_GAP
            - DIVOOM_WIFI_NOTICE_ACTION_W - DIVOOM_WIFI_NOTICE_ACTION_RIGHT
            - DIVOOM_ASTRO_TOP_TOAST_GAP,
        DIVOOM_ASTRO_TOP_TOAST_H);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label,
        divoom_menu_control_get_font_info(22, DIVOOM_MENU_CONTROL_ID_TOP_WIFI_NOTICE_MENU), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(DIVOOM_ASTRO_INK), 0);

    lv_obj_add_event_cb(phandle->top_disp_obj,
        divoom_lvgl_wifi_disconnect_event_cb, LV_EVENT_KEY, phandle);
    lv_group_add_obj(lv_group_get_default(), phandle->top_disp_obj);
    divoom_lvgl_top_control_add_top(phandle, phandle->top_disp_obj);
    phandle->dismiss_timer = lv_timer_create(divoom_lvgl_wifi_disconnect_timer_cb,
        DIVOOM_WIFI_NOTICE_AUTO_CLOSE_MS, phandle);
    if (phandle->dismiss_timer)
    {
        lv_timer_set_repeat_count(phandle->dismiss_timer, 1);
    }
    divoom_lvgl_layer_top_raise(phandle->top_disp_obj);
    divoom_free_lvgl_lock();
}

void divoom_lvgl_wifi_disconnect_exit(void)
{
    Pdivoom_lvgl_wifi_disconnect_ctrl phandle;

    divoom_get_lvgl_lock();
    phandle = pdivoom_lvgl_wifi_disconnect_handle;
    if (!phandle)
    {
        divoom_free_lvgl_lock();
        return;
    }
    pdivoom_lvgl_wifi_disconnect_handle = NULL;
    if (phandle->dismiss_timer)
    {
        lv_timer_del(phandle->dismiss_timer);
        phandle->dismiss_timer = NULL;
    }
    divoom_lvgl_top_control_remove_top(phandle);
    if (phandle->top_disp_obj)
    {
        lv_obj_del(phandle->top_disp_obj);
        phandle->top_disp_obj = NULL;
    }
    lv_style_reset(&phandle->action_style);
    lv_style_reset(&phandle->top_disp_style);
    lv_obj_invalidate(lv_scr_act());
    divoom_system_free(phandle);
    divoom_free_lvgl_lock();
}

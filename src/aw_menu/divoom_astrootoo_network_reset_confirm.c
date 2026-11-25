#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_main.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_astrootoo_network_reset_confirm.h"

typedef struct _divoom_astrootoo_network_reset_confirm_ctrl
{
    lv_obj_t *overlay;
    lv_obj_t *cancel_btn;
    lv_obj_t *ok_btn;
    lv_obj_t *focus_restore;
    int confirm_focus;
    lv_timer_t *hold_timer;
} divoom_astrootoo_network_reset_confirm_ctrl;

static divoom_astrootoo_network_reset_confirm_ctrl g_network_reset_confirm;

/* 主线程可读写，勿碰 LVGL */
static uint8 g_hold_armed_main;
static uint8 g_hold_shown_main;
static uint32 g_long_tick_ms;
static uint8 g_confirm_open;

static int divoom_astrootoo_network_reset_confirm_can_open(void)
{
    int menu_id;

    if (g_confirm_open)
    {
        return 0;
    }
    menu_id = divoom_menu_control_get_active_menu_id();
    if (menu_id == DIVOOM_MENU_CONTROL_ID_CONFIG_WIFI
        || menu_id == DIVOOM_MENU_CONTROL_ID_ASTROTOO_OOBE_CONFIG_WIFI
        || menu_id == DIVOOM_MENU_CONTROL_ID_ASTROTOO_OOBE_WELCOME)
    {
        return 0;
    }
    return 1;
}

static void divoom_astrootoo_network_reset_confirm_group_remove_safe(lv_obj_t *obj)
{
    lv_group_t *group;

    if (!obj)
    {
        return;
    }
    group = lv_obj_get_group(obj);
    if (group)
    {
        lv_group_remove_obj(obj);
    }
}

static void divoom_astrootoo_network_reset_confirm_refresh(void)
{
    int ok_focus;
    lv_obj_t *ok_lbl;
    int menu_id;

    if (!g_network_reset_confirm.cancel_btn || !g_network_reset_confirm.ok_btn)
    {
        return;
    }
    menu_id = DIVOOM_MENU_CONTROL_ID_CONFIG_WIFI;
    ok_focus = (g_network_reset_confirm.confirm_focus == 1);

    lv_obj_set_style_bg_color(g_network_reset_confirm.cancel_btn,
        lv_color_hex(ok_focus ? DIVOOM_ASTRO_SURFACE_IDLE : DIVOOM_ASTRO_SURFACE_FOCUS), 0);

    if (ok_focus)
    {
        lv_obj_set_style_bg_opa(g_network_reset_confirm.ok_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(g_network_reset_confirm.ok_btn, lv_color_hex(DIVOOM_ASTRO_DANGER), 0);
        lv_obj_set_style_border_width(g_network_reset_confirm.ok_btn, 0, 0);
    }
    else
    {
        lv_obj_set_style_bg_opa(g_network_reset_confirm.ok_btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(g_network_reset_confirm.ok_btn, 2, 0);
        lv_obj_set_style_border_color(g_network_reset_confirm.ok_btn, lv_color_hex(DIVOOM_ASTRO_DANGER), 0);
    }
    ok_lbl = lv_obj_get_child(g_network_reset_confirm.ok_btn, 0);
    if (ok_lbl)
    {
        lv_obj_set_style_text_color(ok_lbl,
            lv_color_hex(ok_focus ? DIVOOM_ASTRO_BG : DIVOOM_ASTRO_DANGER), 0);
    }

    lv_group_focus_obj(ok_focus ? g_network_reset_confirm.ok_btn : g_network_reset_confirm.cancel_btn);
    divoom_astrootoo_ui_confirm_modal_sync_btn_labels(g_network_reset_confirm.cancel_btn,
        g_network_reset_confirm.ok_btn, ok_focus);
}

static void divoom_astrootoo_network_reset_confirm_close_inner(int restore_focus)
{
    if (!g_network_reset_confirm.overlay)
    {
        g_confirm_open = 0;
        return;
    }
    divoom_astrootoo_network_reset_confirm_group_remove_safe(g_network_reset_confirm.cancel_btn);
    divoom_astrootoo_network_reset_confirm_group_remove_safe(g_network_reset_confirm.ok_btn);
    lv_obj_del(g_network_reset_confirm.overlay);
    g_network_reset_confirm.overlay = NULL;
    g_network_reset_confirm.cancel_btn = NULL;
    g_network_reset_confirm.ok_btn = NULL;
    g_confirm_open = 0;
    if (restore_focus && g_network_reset_confirm.focus_restore)
    {
        lv_group_focus_obj(g_network_reset_confirm.focus_restore);
    }
    g_network_reset_confirm.focus_restore = NULL;
}

static void divoom_astrootoo_network_reset_confirm_close(void)
{
    divoom_astrootoo_network_reset_confirm_close_inner(1);
}

static void divoom_astrootoo_network_reset_confirm_commit(void)
{
    /* 即将切全屏配网，勿把焦点还给即将销毁的表盘/菜单对象 */
    divoom_astrootoo_network_reset_confirm_close_inner(0);
    divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_CONFIG_NET_WORK);
}

static void divoom_astrootoo_network_reset_confirm_key_cb(lv_event_t *e)
{
    uint8_t key_code;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_astrootoo_network_reset_confirm_close();
        return;
    }
    if (divoom_astrootoo_ui_key_is_left(key_code) || divoom_astrootoo_ui_key_is_right(key_code)
        || divoom_astrootoo_ui_key_is_prev(key_code) || divoom_astrootoo_ui_key_is_next(key_code))
    {
        g_network_reset_confirm.confirm_focus =
            (g_network_reset_confirm.confirm_focus == 0) ? 1 : 0;
        divoom_astrootoo_network_reset_confirm_refresh();
        return;
    }
    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        if (g_network_reset_confirm.confirm_focus == 1)
        {
            divoom_astrootoo_network_reset_confirm_commit();
        }
        else
        {
            divoom_astrootoo_network_reset_confirm_close();
        }
    }
}

static lv_obj_t *divoom_astrootoo_network_reset_confirm_make_btn(lv_obj_t *parent,
    const char *label_key)
{
    return divoom_astrootoo_ui_confirm_modal_make_btn(parent,
        divoom_menu_control_get_disp_str(label_key), DIVOOM_MENU_CONTROL_ID_CONFIG_WIFI, 0,
        divoom_astrootoo_network_reset_confirm_key_cb, NULL, 1);
}

/* 须在 LVGL / menu 定时器线程调用 */
static void divoom_astrootoo_network_reset_confirm_show_inner(void)
{
    lv_obj_t *frame;
    lv_obj_t *title;
    lv_obj_t *body;
    lv_obj_t *actions;
    lv_obj_t *cancel_lbl;
    int menu_id;

    if (!divoom_astrootoo_network_reset_confirm_can_open())
    {
        return;
    }

    g_network_reset_confirm.confirm_focus = 0;
    g_network_reset_confirm.focus_restore = lv_group_get_focused(lv_group_get_default());
    menu_id = DIVOOM_MENU_CONTROL_ID_CONFIG_WIFI;

    g_network_reset_confirm.overlay = divoom_astrootoo_ui_confirm_modal_create(
        lv_layer_top(), &frame);
    if (!g_network_reset_confirm.overlay || !frame)
    {
        return;
    }

    title = lv_label_create(frame);
    lv_label_set_text(title, divoom_menu_control_get_disp_str("ResetWifiConfig"));
    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(30, menu_id), 0);
    lv_obj_set_style_pad_bottom(title, 10, 0);
    lv_obj_set_style_border_side(title, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(title, 2, 0);
    lv_obj_set_style_border_color(title, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);

    body = lv_label_create(frame);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_label_set_text(body, divoom_menu_control_get_disp_str("ResetNetworkConfirmBody"));
    lv_obj_set_width(body, LV_PCT(100));
    lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(body, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(body, divoom_menu_control_get_font_info(24, menu_id), 0);

    actions = lv_obj_create(frame);
    lv_obj_remove_style_all(actions);
    lv_obj_set_width(actions, LV_PCT(100));
    lv_obj_set_height(actions, LV_SIZE_CONTENT);
    lv_obj_clear_flag(actions, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(actions, 8, 0);

    g_network_reset_confirm.cancel_btn =
        divoom_astrootoo_network_reset_confirm_make_btn(actions, "Cancel");
    g_network_reset_confirm.ok_btn =
        divoom_astrootoo_network_reset_confirm_make_btn(actions, "Confirm");

    cancel_lbl = lv_obj_get_child(g_network_reset_confirm.cancel_btn, 0);
    if (cancel_lbl)
    {
        lv_obj_set_style_text_color(cancel_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    }

    g_confirm_open = 1;
    divoom_astrootoo_ui_confirm_modal_layout(frame);
    divoom_astrootoo_network_reset_confirm_refresh();
}

static void divoom_astrootoo_network_reset_confirm_cancel_hold_timer_inner(void)
{
    if (g_network_reset_confirm.hold_timer)
    {
        lv_timer_del(g_network_reset_confirm.hold_timer);
        g_network_reset_confirm.hold_timer = NULL;
    }
}

static void divoom_astrootoo_network_reset_confirm_hold_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    g_network_reset_confirm.hold_timer = NULL;
    if (!g_hold_armed_main || g_hold_shown_main)
    {
        return;
    }
    g_hold_shown_main = 1;
    g_hold_armed_main = 0;
    divoom_astrootoo_network_reset_confirm_show_inner();
}

static void divoom_astrootoo_network_reset_confirm_lvgl_arm_hold(void *user_data)
{
    (void)user_data;
    divoom_astrootoo_network_reset_confirm_cancel_hold_timer_inner();
    if (!g_hold_armed_main || g_hold_shown_main || g_confirm_open)
    {
        return;
    }
    g_network_reset_confirm.hold_timer = lv_timer_create(
        divoom_astrootoo_network_reset_confirm_hold_timer_cb,
        ASTRO_NETWORK_RESET_HOLD_DELAY_MS, NULL);
    if (g_network_reset_confirm.hold_timer)
    {
        lv_timer_set_repeat_count(g_network_reset_confirm.hold_timer, 1);
    }
}

static void divoom_astrootoo_network_reset_confirm_lvgl_disarm_hold(void *user_data)
{
    (void)user_data;
    divoom_astrootoo_network_reset_confirm_cancel_hold_timer_inner();
}

static void divoom_astrootoo_network_reset_confirm_lvgl_show(void *user_data)
{
    (void)user_data;
    divoom_astrootoo_network_reset_confirm_cancel_hold_timer_inner();
    divoom_astrootoo_network_reset_confirm_show_inner();
}

static void divoom_astrootoo_network_reset_confirm_request_show(void)
{
    g_hold_shown_main = 1;
    g_hold_armed_main = 0;
    divoom_menu_control_lvgl_ctrl(divoom_astrootoo_network_reset_confirm_lvgl_show, NULL);
}

int divoom_astrootoo_network_reset_confirm_is_open(void)
{
    return g_confirm_open;
}

void divoom_astrootoo_network_reset_confirm_arm_hold(void)
{
    if (g_confirm_open)
    {
        return;
    }
    g_hold_armed_main = 1;
    g_hold_shown_main = 0;
    g_long_tick_ms = divoom_system_get_tickms();
    divoom_menu_control_lvgl_ctrl(divoom_astrootoo_network_reset_confirm_lvgl_arm_hold, NULL);
}

void divoom_astrootoo_network_reset_confirm_disarm_hold(void)
{
    g_hold_armed_main = 0;
    divoom_menu_control_lvgl_ctrl(divoom_astrootoo_network_reset_confirm_lvgl_disarm_hold, NULL);
}

void divoom_astrootoo_network_reset_confirm_on_pressing(void)
{
    if (!g_hold_armed_main || g_hold_shown_main || g_confirm_open)
    {
        return;
    }
    /* LONG 约在 2s 触发；再持满 HOLD_DELAY_MS 。勿用 PRESSING 次数提前弹出。 */
    if (divoom_system_get_tickms() - g_long_tick_ms >= ASTRO_NETWORK_RESET_HOLD_DELAY_MS)
    {
        divoom_astrootoo_network_reset_confirm_request_show();
    }
}

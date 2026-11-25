#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_menu_direct_ctrl.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_astrootoo_theme.h"
#include "divoom_btcore_uart_cmd.h"
#include "divoom_tws_host.h"
#include <stdio.h>
#include <string.h>

#define DIVOOM_ASTRO_TWS_UI      "26/"
#define DIVOOM_ASTRO_TWS_ROW_W   444
#define DIVOOM_ASTRO_TWS_ROW_H   72
#define DIVOOM_ASTRO_TWS_ROW_GAP 8
#define DIVOOM_ASTRO_TWS_ROW_X   ((DIVOOM_SCREEN_WIDTH - DIVOOM_ASTRO_TWS_ROW_W) / 2)
#define DIVOOM_ASTRO_TWS_TOP     8
#define DIVOOM_ASTRO_TWS_PAD     8
#define DIVOOM_ASTRO_TWS_ICON    36

#define DIVOOM_ASTRO_TWS_Y_ACTION  0
#define DIVOOM_ASTRO_TWS_Y_UNPAIR  1

typedef struct _divoom_astrootoo_tws_ctrl
{
    divoom_astrootoo_ui_base ui;
    int menu_id;
    lv_obj_t *root_obj;
    lv_obj_t *content;
    lv_obj_t *status_row;
    lv_obj_t *status_val;
    lv_obj_t *hint_box;
    lv_obj_t *hint_lbl;
    lv_obj_t *action_btn;
    lv_obj_t *action_lbl;
    lv_obj_t *unpair_btn;
    lv_obj_t *unpair_icon;
    lv_obj_t *confirm_overlay;
    lv_obj_t *confirm_cancel_btn;
    lv_obj_t *confirm_ok_btn;
    lv_obj_t *confirm_focus_restore;
    void *direct_handle;
    lv_timer_t *timer;
    uint8 action_func;
    uint8 action_arg;
    uint8 confirm_focus;
} divoom_astrootoo_tws_ctrl, *Pdivoom_astrootoo_tws_ctrl;

static void divoom_astrootoo_tws_refresh(Pdivoom_astrootoo_tws_ctrl phandle);

static const char *divoom_astrootoo_tws_fail_key(uint8 reason)
{
    switch (reason)
    {
    case DIVOOM_BTCORE_TWS_FAIL_NOT_FOUND:
        return "StereoPairNotFound";
    case DIVOOM_BTCORE_TWS_FAIL_PHONE_WAIT:
        return "StereoPairPhoneWait";
    case DIVOOM_BTCORE_TWS_FAIL_IN_CALL:
        return "PhoneCallActiveTip";
    default:
        return NULL;
    }
}

static void divoom_astrootoo_tws_show(lv_obj_t *obj, int show)
{
    if (!obj)
    {
        return;
    }
    if (show)
    {
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

static lv_obj_t *divoom_astrootoo_tws_make_card(Pdivoom_astrootoo_tws_ctrl phandle,
    const char *left_key, int focus_y, lv_obj_t **out_lbl)
{
    lv_obj_t *card;
    lv_obj_t *lbl;

    card = lv_btn_create(phandle->content);
    lv_obj_add_style(card, &phandle->ui.card_item_style, 0);
    lv_obj_add_style(card, &phandle->ui.card_item_focus_style, LV_STATE_FOCUSED);
    lv_obj_add_style(card, &phandle->ui.card_item_focus_style, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_size(card, DIVOOM_ASTRO_TWS_ROW_W, DIVOOM_ASTRO_TWS_ROW_H);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lbl = lv_label_create(card);
    lv_label_set_text(lbl, divoom_menu_control_get_disp_str(left_key));
    lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, DIVOOM_ASTRO_TWS_PAD, 0);
    if (out_lbl)
    {
        *out_lbl = lbl;
    }
    if (focus_y >= 0)
    {
        divoom_menu_direct_ctrl_update(phandle->direct_handle, card, 0, focus_y);
    }
    return card;
}

static void divoom_astrootoo_tws_group_remove(lv_obj_t *obj)
{
    if (obj && lv_obj_get_group(obj))
    {
        lv_group_remove_obj(obj);
    }
}

static void divoom_astrootoo_tws_confirm_close(Pdivoom_astrootoo_tws_ctrl phandle)
{
    lv_obj_t *restore;

    if (!phandle || !phandle->confirm_overlay)
    {
        return;
    }
    divoom_astrootoo_tws_group_remove(phandle->confirm_cancel_btn);
    divoom_astrootoo_tws_group_remove(phandle->confirm_ok_btn);
    lv_obj_del(phandle->confirm_overlay);
    phandle->confirm_overlay = NULL;
    phandle->confirm_cancel_btn = NULL;
    phandle->confirm_ok_btn = NULL;
    restore = phandle->confirm_focus_restore;
    phandle->confirm_focus_restore = NULL;
    if (restore && lv_obj_is_valid(restore))
    {
        lv_group_focus_obj(restore);
    }
}

static void divoom_astrootoo_tws_confirm_refresh(Pdivoom_astrootoo_tws_ctrl phandle)
{
    int ok_focus;
    lv_obj_t *ok_lbl;

    if (!phandle || !phandle->confirm_cancel_btn || !phandle->confirm_ok_btn)
    {
        return;
    }
    ok_focus = (phandle->confirm_focus == 1);
    lv_obj_set_style_bg_color(phandle->confirm_cancel_btn,
        lv_color_hex(ok_focus ? DIVOOM_ASTRO_SURFACE_IDLE : DIVOOM_ASTRO_SURFACE_FOCUS), 0);
    if (ok_focus)
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
            lv_color_hex(ok_focus ? DIVOOM_ASTRO_BG : DIVOOM_ASTRO_DANGER), 0);
    }
    lv_group_focus_obj(ok_focus ? phandle->confirm_ok_btn : phandle->confirm_cancel_btn);
    divoom_astrootoo_ui_confirm_modal_sync_btn_labels(phandle->confirm_cancel_btn,
        phandle->confirm_ok_btn, ok_focus);
}

static void divoom_astrootoo_tws_confirm_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_tws_ctrl phandle;
    uint8_t key_code;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    phandle = (Pdivoom_astrootoo_tws_ctrl)lv_event_get_user_data(e);
    if (!phandle || !phandle->confirm_overlay)
    {
        return;
    }
    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_astrootoo_tws_confirm_close(phandle);
        return;
    }
    if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code)
        || divoom_astrootoo_ui_key_is_left(key_code) || divoom_astrootoo_ui_key_is_right(key_code)
        || divoom_astrootoo_ui_key_is_prev(key_code) || divoom_astrootoo_ui_key_is_next(key_code))
    {
        phandle->confirm_focus = (phandle->confirm_focus == 0) ? 1 : 0;
        divoom_astrootoo_tws_confirm_refresh(phandle);
        return;
    }
    if (!divoom_astrootoo_ui_key_is_enter(key_code))
    {
        return;
    }
    if (phandle->confirm_focus == 1)
    {
        divoom_astrootoo_tws_confirm_close(phandle);
        divoom_tws_host_send(DIVOOM_BTCORE_TWS_FUNC_UNPAIR, DIVOOM_BTCORE_TWS_ARG_OFF);
        divoom_astrootoo_tws_refresh(phandle);
    }
    else
    {
        divoom_astrootoo_tws_confirm_close(phandle);
    }
}

static void divoom_astrootoo_tws_confirm_open(Pdivoom_astrootoo_tws_ctrl phandle)
{
    lv_obj_t *frame;
    lv_obj_t *title;
    lv_obj_t *actions;
    lv_obj_t *cancel_lbl;

    if (!phandle || phandle->confirm_overlay)
    {
        return;
    }
    phandle->confirm_focus = 0;
    phandle->confirm_focus_restore = lv_group_get_focused(lv_group_get_default());
    phandle->confirm_overlay = divoom_astrootoo_ui_confirm_modal_create(phandle->root_obj, &frame);
    if (!phandle->confirm_overlay || !frame)
    {
        phandle->confirm_overlay = NULL;
        phandle->confirm_focus_restore = NULL;
        return;
    }

    title = lv_label_create(frame);
    lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP);
    lv_label_set_text(title, divoom_menu_control_get_disp_str("UnpairBT"));
    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(30, phandle->menu_id), 0);
    lv_obj_set_style_pad_bottom(title, 10, 0);
    lv_obj_set_style_border_side(title, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(title, 2, 0);
    lv_obj_set_style_border_color(title, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);

    actions = lv_obj_create(frame);
    lv_obj_remove_style_all(actions);
    lv_obj_set_width(actions, LV_PCT(100));
    lv_obj_set_height(actions, LV_SIZE_CONTENT);
    lv_obj_clear_flag(actions, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(actions, 8, 0);
    phandle->confirm_cancel_btn = divoom_astrootoo_ui_confirm_modal_make_btn(actions,
        divoom_menu_control_get_disp_str("Cancel"), phandle->menu_id, 0,
        divoom_astrootoo_tws_confirm_key_cb, phandle, 1);
    phandle->confirm_ok_btn = divoom_astrootoo_ui_confirm_modal_make_btn(actions,
        divoom_menu_control_get_disp_str("Confirm"), phandle->menu_id, 0,
        divoom_astrootoo_tws_confirm_key_cb, phandle, 1);
    cancel_lbl = lv_obj_get_child(phandle->confirm_cancel_btn, 0);
    if (cancel_lbl)
    {
        lv_obj_set_style_text_color(cancel_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    }
    divoom_astrootoo_ui_confirm_modal_layout(frame);
    divoom_astrootoo_tws_confirm_refresh(phandle);
}

static lv_obj_t *divoom_astrootoo_tws_focus_home(Pdivoom_astrootoo_tws_ctrl phandle)
{
    if (phandle->action_btn
        && !lv_obj_has_flag(phandle->action_btn, LV_OBJ_FLAG_HIDDEN))
    {
        return phandle->action_btn;
    }
    if (phandle->unpair_btn
        && !lv_obj_has_flag(phandle->unpair_btn, LV_OBJ_FLAG_HIDDEN))
    {
        return phandle->unpair_btn;
    }
    return phandle->status_row;
}

static void divoom_astrootoo_tws_refresh(Pdivoom_astrootoo_tws_ctrl phandle)
{
    divoom_tws_snap_t snap;
    char status[160];
    const char *fail_key;
    const char *hint_key;
    const char *action_key;
    int paired;
    int linked;
    int show_action;
    int show_unpair;
    int waiting;
    int in_call;
    lv_coord_t y;
    lv_group_t *g;

    if (!phandle || !phandle->status_row)
    {
        return;
    }
    divoom_tws_host_get_snap(&snap);
    paired = (snap.valid && (snap.flags & DIVOOM_BTCORE_TWS_F_PAIRED)) ? 1 : 0;
    linked = (snap.state == DIVOOM_BTCORE_TWS_ST_CONNECTED) ? 1 : 0;

    show_action = 0;
    show_unpair = 0;
    /* 未配对、等待都显示未配对。等待是进页自己做的，按钮才是开始配对。 */
    waiting = (snap.state == DIVOOM_BTCORE_TWS_ST_UNPAIRED
        || snap.state == DIVOOM_BTCORE_TWS_ST_LISTEN) ? 1 : 0;
    in_call = (snap.valid && (snap.flags & DIVOOM_BTCORE_TWS_F_CALL)) ? 1 : 0;
    action_key = "StartPairing";
    phandle->action_func = DIVOOM_BTCORE_TWS_FUNC_PAIR;
    phandle->action_arg = DIVOOM_BTCORE_TWS_ARG_ON;
    if (!in_call && snap.state == DIVOOM_BTCORE_TWS_ST_SEARCH)
    {
        show_action = 1;
        action_key = "CancelSearch";
        phandle->action_func = DIVOOM_BTCORE_TWS_FUNC_PAIR;
        phandle->action_arg = DIVOOM_BTCORE_TWS_ARG_OFF;
    }
    else if (!in_call && waiting)
    {
        show_action = 1;
    }
    else if (!in_call && paired)
    {
        show_unpair = 1;
    }

    y = DIVOOM_ASTRO_TWS_TOP;
    divoom_astrootoo_tws_show(phandle->status_row, 1);
    lv_obj_set_pos(phandle->status_row, DIVOOM_ASTRO_TWS_ROW_X, y);
    y += DIVOOM_ASTRO_TWS_ROW_H + DIVOOM_ASTRO_TWS_ROW_GAP;

    if (snap.state == DIVOOM_BTCORE_TWS_ST_SEARCH)
    {
        snprintf(status, sizeof(status), "%s %u",
            divoom_menu_control_get_disp_str("Searching"),
            (unsigned)snap.remain_sec);
    }
    else if (linked && snap.role == DIVOOM_BTCORE_TWS_ROLE_LEFT)
    {
        snprintf(status, sizeof(status), "%s · %s",
            divoom_menu_control_get_disp_str("Connected"),
            divoom_menu_control_get_disp_str("LeftChannel"));
    }
    else if (linked && snap.role == DIVOOM_BTCORE_TWS_ROLE_RIGHT)
    {
        snprintf(status, sizeof(status), "%s · %s",
            divoom_menu_control_get_disp_str("Connected"),
            divoom_menu_control_get_disp_str("RightChannel"));
    }
    else if (snap.state == DIVOOM_BTCORE_TWS_ST_RECONNECT)
    {
        snprintf(status, sizeof(status), "%s",
            divoom_menu_control_get_disp_str("Reconnecting"));
    }
    else
    {
        snprintf(status, sizeof(status), "%s",
            divoom_menu_control_get_disp_str("Unpaired"));
    }
    lv_label_set_text(phandle->status_val, status);
    divoom_astrootoo_ui_list_kv_bind_row(lv_obj_get_child(phandle->status_row, 0),
        phandle->status_val, DIVOOM_ASTRO_TWS_ROW_W, 16, 16, 0, -8, 120);

    fail_key = divoom_astrootoo_tws_fail_key(snap.fail_reason);
    if (fail_key)
    {
        hint_key = fail_key;
    }
    else if (in_call)
    {
        hint_key = "PhoneCallActiveTip";
    }
    else if (snap.role == DIVOOM_BTCORE_TWS_ROLE_RIGHT
        && (linked || snap.state == DIVOOM_BTCORE_TWS_ST_RECONNECT))
    {
        hint_key = "StereoPairPhoneOnLeft";
    }
    else if (waiting || snap.state == DIVOOM_BTCORE_TWS_ST_SEARCH)
    {
        hint_key = "StereoPairGuide";
    }
    else
    {
        hint_key = "";
    }
    divoom_astrootoo_tws_show(phandle->action_btn, show_action);
    if (show_action)
    {
        lv_label_set_text(phandle->action_lbl, divoom_menu_control_get_disp_str(action_key));
        lv_obj_set_pos(phandle->action_btn, DIVOOM_ASTRO_TWS_ROW_X, y);
        y += DIVOOM_ASTRO_TWS_ROW_H + DIVOOM_ASTRO_TWS_ROW_GAP;
    }

    divoom_astrootoo_tws_show(phandle->unpair_btn, show_unpair);
    if (show_unpair)
    {
        lv_obj_set_pos(phandle->unpair_btn, DIVOOM_ASTRO_TWS_ROW_X, y);
        divoom_menu_control_lv_img_set_src(phandle->unpair_icon,
            linked ? DIVOOM_ASTRO_TWS_UI "at_bt_unpair_on.png"
            : DIVOOM_ASTRO_TWS_UI "at_bt_unpair_off.png");
        y += DIVOOM_ASTRO_TWS_ROW_H + DIVOOM_ASTRO_TWS_ROW_GAP;
    }

    if (hint_key[0])
    {
        lv_coord_t box_h;

        box_h = DIVOOM_ASTRO_CONTENT_H - y - DIVOOM_ASTRO_TWS_TOP;
        if (box_h < DIVOOM_ASTRO_TWS_ROW_H)
        {
            box_h = DIVOOM_ASTRO_TWS_ROW_H;
        }
        lv_label_set_text(phandle->hint_lbl, divoom_menu_control_get_disp_str(hint_key));
        lv_obj_set_pos(phandle->hint_box, DIVOOM_ASTRO_TWS_ROW_X, y);
        lv_obj_set_size(phandle->hint_box, DIVOOM_ASTRO_TWS_ROW_W, box_h);
        divoom_astrootoo_tws_show(phandle->hint_box, 1);
    }
    else
    {
        divoom_astrootoo_tws_show(phandle->hint_box, 0);
    }

    if (phandle->confirm_overlay)
    {
        if (!paired)
        {
            divoom_astrootoo_tws_confirm_close(phandle);
        }
        else
        {
            divoom_tws_host_page_poll();
            return;
        }
    }
    g = lv_group_get_default();
    if (g && (lv_obj_get_group(phandle->status_row) == g
        || lv_obj_get_group(phandle->action_btn) == g
        || lv_obj_get_group(phandle->unpair_btn) == g))
    {
        int action_in;
        int unpair_in;
        int status_in;
        lv_obj_t *focused;
        lv_obj_t *home;

        action_in = (lv_obj_get_group(phandle->action_btn) == g);
        unpair_in = (lv_obj_get_group(phandle->unpair_btn) == g);
        status_in = (lv_obj_get_group(phandle->status_row) == g);
        if (show_action && !action_in)
        {
            lv_group_add_obj(g, phandle->action_btn);
        }
        else if (!show_action && action_in)
        {
            lv_group_remove_obj(phandle->action_btn);
        }
        if (show_unpair && !unpair_in)
        {
            lv_group_add_obj(g, phandle->unpair_btn);
        }
        else if (!show_unpair && unpair_in)
        {
            lv_group_remove_obj(phandle->unpair_btn);
        }
        home = divoom_astrootoo_tws_focus_home(phandle);
        if (home == phandle->status_row && !status_in)
        {
            lv_group_add_obj(g, phandle->status_row);
        }
        else if (home != phandle->status_row && status_in)
        {
            lv_group_remove_obj(phandle->status_row);
        }
        focused = lv_group_get_focused(g);
        if (lv_obj_get_group(home) != g)
        {
            lv_group_add_obj(g, home);
        }
        if (!focused || lv_obj_get_group(focused) != g
            || lv_obj_has_flag(focused, LV_OBJ_FLAG_HIDDEN))
        {
            lv_group_focus_obj(home);
        }
    }
    divoom_tws_host_page_poll();
}

static void divoom_astrootoo_tws_timer_cb(lv_timer_t *timer)
{
    divoom_astrootoo_tws_refresh((Pdivoom_astrootoo_tws_ctrl)lv_timer_get_user_data(timer));
}

static void divoom_astrootoo_tws_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_tws_ctrl phandle;
    lv_obj_t *target;
    lv_obj_t *next;
    uint8_t key_code;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    phandle = (Pdivoom_astrootoo_tws_ctrl)lv_event_get_user_data(e);
    if (!phandle)
    {
        return;
    }
    target = lv_event_get_target(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    if (phandle->confirm_overlay)
    {
        return;
    }
    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_menu_control_return_pre();
        return;
    }
    if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code))
    {
        int direct;
        int step;
        lv_obj_t *cursor;

        direct = divoom_astrootoo_ui_key_is_up(key_code)
            ? DIVOOM_MENU_DIRECT_UP : DIVOOM_MENU_DIRECT_DOWN;
        /* 开始配对和取消配对不会同时出现，藏起来的那一格要跳过，否则向下停在空位上。 */
        cursor = target;
        for (step = 0; step < 3; step++)
        {
            next = divoom_menu_direct_ctrl_get(phandle->direct_handle, cursor, direct);
            if (!next || next == target)
            {
                break;
            }
            if (lv_obj_has_flag(next, LV_OBJ_FLAG_HIDDEN))
            {
                cursor = next;
                continue;
            }
            lv_group_focus_obj(next);
            divoom_astrootoo_ui_focus_scroll(next);
            break;
        }
        return;
    }
    if (!divoom_astrootoo_ui_key_is_enter(key_code))
    {
        return;
    }
    if (target == phandle->action_btn)
    {
        divoom_tws_host_send(phandle->action_func, phandle->action_arg);
    }
    else if (target == phandle->unpair_btn)
    {
        divoom_astrootoo_tws_confirm_open(phandle);
        return;
    }
    divoom_astrootoo_tws_refresh(phandle);
}

static void divoom_astrootoo_tws_bind(Pdivoom_astrootoo_tws_ctrl phandle)
{
    lv_group_t *g;

    g = lv_group_get_default();
    if (!g || !phandle)
    {
        return;
    }
    lv_group_remove_all_objs(g);
    if (phandle->action_btn && !lv_obj_has_flag(phandle->action_btn, LV_OBJ_FLAG_HIDDEN))
    {
        lv_group_add_obj(g, phandle->action_btn);
    }
    if (phandle->unpair_btn && !lv_obj_has_flag(phandle->unpair_btn, LV_OBJ_FLAG_HIDDEN))
    {
        lv_group_add_obj(g, phandle->unpair_btn);
    }
    if (lv_group_get_obj_count(g) == 0)
    {
        lv_group_add_obj(g, phandle->status_row);
    }
    lv_group_focus_obj(divoom_astrootoo_tws_focus_home(phandle));
}

void *divoom_astrootoo_settings_tws_init(void *user_data, int menu_id)
{
    Pdivoom_astrootoo_tws_ctrl phandle;
    lv_obj_t *lbl;

    (void)user_data;
    phandle = divoom_system_malloc(sizeof(*phandle));
    if (!phandle)
    {
        return NULL;
    }
    memset(phandle, 0, sizeof(*phandle));
    phandle->menu_id = menu_id;
    phandle->root_obj = divoom_astrootoo_ui_create_root(&phandle->ui);
    divoom_astrootoo_ui_create_page_header_tab(phandle->root_obj, &phandle->ui,
        "StereoPair", "26/at_settings_header.png", menu_id);
    phandle->content = divoom_astrootoo_ui_create_scroll_content(phandle->root_obj, &phandle->ui);
    phandle->direct_handle = divoom_menu_direct_ctrl_create(1, 2);

    phandle->status_row = divoom_astrootoo_tws_make_card(phandle, "ConnectionStatus", -1, NULL);
    lv_obj_add_event_cb(phandle->status_row, divoom_astrootoo_tws_key_cb, LV_EVENT_KEY, phandle);
    phandle->status_val = lv_label_create(phandle->status_row);
    lv_label_set_text(phandle->status_val, "");
    lv_obj_set_style_text_color(phandle->status_val, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(phandle->status_val,
        divoom_menu_control_get_font_info(24, menu_id), 0);

    phandle->hint_box = lv_obj_create(phandle->content);
    lv_obj_remove_style_all(phandle->hint_box);
    lv_obj_add_style(phandle->hint_box, &phandle->ui.card_item_style, 0);
    lv_obj_clear_flag(phandle->hint_box, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    phandle->hint_lbl = lv_label_create(phandle->hint_box);
    lv_label_set_long_mode(phandle->hint_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(phandle->hint_lbl, DIVOOM_ASTRO_TWS_ROW_W - 32);
    lv_obj_set_pos(phandle->hint_lbl, 8, 8);
    lv_obj_set_style_text_color(phandle->hint_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(phandle->hint_lbl, divoom_menu_control_get_font_info(22, menu_id), 0);

    phandle->action_btn = divoom_astrootoo_tws_make_card(phandle, "StartPairing",
        DIVOOM_ASTRO_TWS_Y_ACTION, &phandle->action_lbl);
    divoom_astrootoo_ui_list_label_prepare(phandle->action_lbl,
        divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_TWS_ROW_W, 16, 16, 0));
    divoom_astrootoo_ui_list_row_bind_marquee(phandle->action_btn, phandle->action_lbl);
    lv_obj_add_event_cb(phandle->action_btn, divoom_astrootoo_tws_key_cb, LV_EVENT_KEY, phandle);

    phandle->unpair_btn = divoom_astrootoo_tws_make_card(phandle, "UnpairBT",
        DIVOOM_ASTRO_TWS_Y_UNPAIR, &lbl);
    divoom_astrootoo_ui_list_label_prepare(lbl,
        divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_TWS_ROW_W, 16, 16,
            DIVOOM_ASTRO_TWS_ICON));
    divoom_astrootoo_ui_list_row_bind_marquee(phandle->unpair_btn, lbl);
    phandle->unpair_icon = lv_img_create(phandle->unpair_btn);
    lv_obj_set_size(phandle->unpair_icon, DIVOOM_ASTRO_TWS_ICON, DIVOOM_ASTRO_TWS_ICON);
    lv_obj_align(phandle->unpair_icon, LV_ALIGN_RIGHT_MID, -DIVOOM_ASTRO_TWS_PAD, 0);
    divoom_menu_control_lv_img_set_src(phandle->unpair_icon,
        DIVOOM_ASTRO_TWS_UI "at_bt_unpair_off.png");
    lv_obj_add_event_cb(phandle->unpair_btn, divoom_astrootoo_tws_key_cb, LV_EVENT_KEY, phandle);

    divoom_tws_host_on_page_enter();
    divoom_astrootoo_tws_refresh(phandle);
    phandle->timer = lv_timer_create(divoom_astrootoo_tws_timer_cb, 300, phandle);
    if (phandle->timer)
    {
        lv_timer_set_repeat_count(phandle->timer, -1);
    }
    return phandle;
}

void divoom_astrootoo_settings_tws_disp(void *handle, void *active_obj)
{
    Pdivoom_astrootoo_tws_ctrl phandle;

    (void)active_obj;
    phandle = (Pdivoom_astrootoo_tws_ctrl)handle;
    if (!phandle)
    {
        return;
    }
    lv_scr_load(phandle->root_obj);
    divoom_astrootoo_tws_bind(phandle);
    divoom_astrootoo_tws_refresh(phandle);
}

void divoom_astrootoo_settings_tws_clean(void *handle)
{
    Pdivoom_astrootoo_tws_ctrl phandle;

    phandle = (Pdivoom_astrootoo_tws_ctrl)handle;
    if (!phandle)
    {
        return;
    }
    if (phandle->timer)
    {
        lv_timer_del(phandle->timer);
        phandle->timer = NULL;
    }
    divoom_astrootoo_tws_confirm_close(phandle);
    divoom_tws_host_on_page_leave();
    if (phandle->direct_handle)
    {
        divoom_menu_direct_ctrl_destory(phandle->direct_handle);
        phandle->direct_handle = NULL;
    }
    if (phandle->root_obj)
    {
        lv_obj_del(phandle->root_obj);
        phandle->root_obj = NULL;
    }
    divoom_astrootoo_ui_base_reset_styles(&phandle->ui);
    divoom_system_free(phandle);
}

void divoom_astrootoo_settings_tws_pause(void *handle)
{
    (void)handle;
}

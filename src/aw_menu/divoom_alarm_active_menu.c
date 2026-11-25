#include "divoom_alarm_active_menu.h"
#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_astrootoo_theme.h"
#include "divoom_alarm.h"

enum
{
    DIVOOM_ALARM_ACTIVE_FOCUS_SNOOZE = 0,
    DIVOOM_ALARM_ACTIVE_FOCUS_DISMISS = 1,
};

typedef struct _divoom_alarm_active_menu_ctrl
{
    int menu_id;
    int alarm_status;
    int remain_time;
    int remain_time_last_disp;
    uint8 user_snoozed;
    uint8 button_focus;
    uint8 leaving;
    uint8 styles_initialized;

    divoom_astrootoo_ui_base ui;
    lv_style_t root_style;
    lv_style_t btn_style;
    lv_style_t btn_focus_style;

    lv_obj_t *main_obj;
    lv_obj_t *badge_row;
    lv_obj_t *badge_icon;
    lv_obj_t *badge_label;
    lv_obj_t *time_label;
    lv_obj_t *snooze_btn;
    lv_obj_t *snooze_label;
    lv_obj_t *dismiss_btn;
    lv_obj_t *dismiss_label;

    lv_timer_t *refresh_timer;
} divoom_alarm_active_menu_ctrl, *Pdivoom_alarm_active_menu_ctrl;

static lv_coord_t divoom_alarm_active_menu_actions_h(int show_snooze)
{
    if (show_snooze)
    {
        return DIVOOM_ASTRO_ALARM_RING_BTN_H * 2 + DIVOOM_ASTRO_ALARM_RING_BTN_GAP;
    }
    return DIVOOM_ASTRO_ALARM_RING_BTN_H;
}

static int divoom_alarm_active_menu_use_accent(Pdivoom_alarm_active_menu_ctrl phandle)
{
    if (!phandle)
    {
        return 0;
    }
    if (phandle->alarm_status == DIVOOM_ALARM_STATUS_SNOOZE)
    {
        return 1;
    }
    if (phandle->user_snoozed)
    {
        return 1;
    }
    return 0;
}

static int divoom_alarm_active_menu_is_snooze_waiting(Pdivoom_alarm_active_menu_ctrl phandle)
{
    if (!phandle)
    {
        return 0;
    }
    if (phandle->alarm_status == DIVOOM_ALARM_STATUS_SNOOZE)
    {
        return 1;
    }
    if (phandle->user_snoozed)
    {
        return 1;
    }
    return 0;
}

static int divoom_alarm_active_menu_show_snooze(Pdivoom_alarm_active_menu_ctrl phandle)
{
    if (!phandle)
    {
        return 0;
    }
    if (phandle->user_snoozed)
    {
        return 0;
    }
    if (phandle->alarm_status == DIVOOM_ALARM_STATUS_SNOOZE)
    {
        return 0;
    }
    return 1;
}

static void divoom_alarm_active_menu_apply_btn_style(lv_obj_t *btn, lv_obj_t *label, int focused, int use_accent)
{
    lv_color_t border_color;
    lv_color_t text_color;
    lv_opa_t bg_opa;

    (void)use_accent;
    if (focused)
    {
        border_color = lv_color_hex(DIVOOM_ASTRO_ALARM_RING_ACCENT);
        text_color = lv_color_hex(DIVOOM_ASTRO_ALARM_RING_ACCENT);
        bg_opa = DIVOOM_ASTRO_ALARM_RING_FOCUS_FILL_OPA;
    }
    else
    {
        border_color = lv_color_hex(DIVOOM_ASTRO_ALARM_RING_MUTED);
        text_color = lv_color_hex(DIVOOM_ASTRO_ALARM_RING_MUTED);
        bg_opa = LV_OPA_TRANSP;
    }

    lv_obj_set_style_border_color(btn, border_color, 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(DIVOOM_ASTRO_ALARM_RING_FOCUS_FILL), 0);
    lv_obj_set_style_bg_opa(btn, bg_opa, 0);
    lv_obj_set_style_text_color(label, text_color, 0);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_shadow_width(btn, 0, 0);
}

static void divoom_alarm_active_menu_layout_actions(Pdivoom_alarm_active_menu_ctrl phandle)
{
    int show_snooze;
    lv_coord_t actions_h;
    lv_coord_t btn_w;
    lv_coord_t dismiss_y;
    lv_coord_t snooze_y;

    if (!phandle)
    {
        return;
    }

    show_snooze = divoom_alarm_active_menu_show_snooze(phandle);
    actions_h = divoom_alarm_active_menu_actions_h(show_snooze);
    btn_w = DIVOOM_SCREEN_WIDTH - DIVOOM_ASTRO_ALARM_RING_ACTIONS_INSET_X * 2;
    dismiss_y = DIVOOM_SCREEN_HIGH - DIVOOM_ASTRO_ALARM_RING_ACTIONS_BOTTOM - DIVOOM_ASTRO_ALARM_RING_BTN_H;
    snooze_y = dismiss_y - DIVOOM_ASTRO_ALARM_RING_BTN_GAP - DIVOOM_ASTRO_ALARM_RING_BTN_H;

    lv_obj_set_y(phandle->dismiss_btn, dismiss_y);
    lv_obj_set_x(phandle->dismiss_btn, DIVOOM_ASTRO_ALARM_RING_ACTIONS_INSET_X);
    lv_obj_set_width(phandle->dismiss_btn, btn_w);
    lv_obj_set_height(phandle->dismiss_btn, DIVOOM_ASTRO_ALARM_RING_BTN_H);

    if (show_snooze)
    {
        lv_obj_clear_flag(phandle->snooze_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_y(phandle->snooze_btn, snooze_y);
        lv_obj_set_x(phandle->snooze_btn, DIVOOM_ASTRO_ALARM_RING_ACTIONS_INSET_X);
        lv_obj_set_width(phandle->snooze_btn, btn_w);
        lv_obj_set_height(phandle->snooze_btn, DIVOOM_ASTRO_ALARM_RING_BTN_H);
    }
    else
    {
        lv_obj_add_flag(phandle->snooze_btn, LV_OBJ_FLAG_HIDDEN);
        if (phandle->button_focus == DIVOOM_ALARM_ACTIVE_FOCUS_SNOOZE)
        {
            phandle->button_focus = DIVOOM_ALARM_ACTIVE_FOCUS_DISMISS;
        }
    }

    if (phandle->time_label)
    {
        lv_coord_t time_zone_bottom;
        lv_coord_t time_zone_h;
        lv_coord_t time_y;

        time_zone_bottom = DIVOOM_ASTRO_ALARM_RING_ACTIONS_BOTTOM + actions_h;
        time_zone_h = DIVOOM_SCREEN_HIGH - time_zone_bottom;
        time_y = (time_zone_h / 2) + DIVOOM_ASTRO_ALARM_RING_TIME_OFFSET_Y - 48;
        lv_obj_set_y(phandle->time_label, time_y);
    }
}

static void divoom_alarm_active_menu_refresh_palette(Pdivoom_alarm_active_menu_ctrl phandle)
{
    int use_accent;
    const char *badge_img;

    if (!phandle)
    {
        return;
    }

    use_accent = divoom_alarm_active_menu_use_accent(phandle);
    badge_img = use_accent
        ? DIVOOM_ASTRO_ALARM_RING_UI_DIR "alarm_active_badge_accent.png"
        : DIVOOM_ASTRO_ALARM_RING_UI_DIR "alarm_active_badge_muted.png";
    divoom_menu_control_lv_img_set_src(phandle->badge_icon, badge_img);

    if (phandle->badge_label)
    {
        lv_obj_set_style_text_color(phandle->badge_label,
            lv_color_hex(use_accent ? DIVOOM_ASTRO_ALARM_RING_ACCENT : DIVOOM_ASTRO_ALARM_RING_MUTED), 0);
    }
    if (phandle->time_label)
    {
        lv_obj_set_style_text_color(phandle->time_label,
            lv_color_hex(use_accent ? DIVOOM_ASTRO_ALARM_RING_ACCENT : DIVOOM_ASTRO_ALARM_RING_MUTED), 0);
    }

    divoom_alarm_active_menu_apply_btn_style(phandle->snooze_btn, phandle->snooze_label,
        phandle->button_focus == DIVOOM_ALARM_ACTIVE_FOCUS_SNOOZE, use_accent);
    divoom_alarm_active_menu_apply_btn_style(phandle->dismiss_btn, phandle->dismiss_label,
        phandle->button_focus == DIVOOM_ALARM_ACTIVE_FOCUS_DISMISS, use_accent);
}

static void divoom_alarm_active_menu_layout_badge_row(Pdivoom_alarm_active_menu_ctrl phandle)
{
    lv_coord_t badge_x;

    if (!phandle || !phandle->badge_label || !phandle->badge_icon)
    {
        return;
    }

    lv_obj_update_layout(phandle->badge_label);
    badge_x = (DIVOOM_SCREEN_WIDTH
        - DIVOOM_ASTRO_ALARM_RING_BADGE_ICON_W
        - DIVOOM_ASTRO_ALARM_RING_BADGE_GAP
        - lv_obj_get_width(phandle->badge_label)) / 2;
    lv_obj_set_pos(phandle->badge_icon, badge_x, 0);
    lv_obj_set_pos(phandle->badge_label,
        badge_x + DIVOOM_ASTRO_ALARM_RING_BADGE_ICON_W + DIVOOM_ASTRO_ALARM_RING_BADGE_GAP,
        (DIVOOM_ASTRO_ALARM_RING_BADGE_ICON_H - lv_obj_get_height(phandle->badge_label)) / 2);
}

static void divoom_alarm_active_menu_refresh_badge(Pdivoom_alarm_active_menu_ctrl phandle)
{
    const char *badge_key;

    if (!phandle || !phandle->badge_label)
    {
        return;
    }

    badge_key = divoom_alarm_active_menu_is_snooze_waiting(phandle) ? "Snooze" : "Alarm";
    lv_label_set_text(phandle->badge_label, divoom_menu_control_get_disp_str(badge_key));
    divoom_alarm_active_menu_layout_badge_row(phandle);
}

static void divoom_alarm_active_menu_refresh_time(Pdivoom_alarm_active_menu_ctrl phandle)
{
    char alarm_time_str[40];

    if (!phandle || !phandle->time_label)
    {
        return;
    }

    if (divoom_alarm_active_menu_is_snooze_waiting(phandle))
    {
        if (phandle->remain_time == phandle->remain_time_last_disp)
        {
            return;
        }
        phandle->remain_time_last_disp = phandle->remain_time;
        if (phandle->remain_time > 0 && phandle->remain_time < 3600)
        {
            lv_label_set_text_fmt(phandle->time_label, "%02d:%02d",
                phandle->remain_time / 60, phandle->remain_time % 60);
        }
        else
        {
            lv_label_set_text(phandle->time_label, "--:--");
        }
        return;
    }

    divoom_alarm_get_current_alarm_time_str(alarm_time_str);
    if (alarm_time_str[0] == '\0')
    {
        /* 闹钟已清空：保留最后一次有效时刻，避免闪成假时间 */
        return;
    }
    lv_label_set_text(phandle->time_label, alarm_time_str);
}

static void divoom_alarm_active_menu_leave(Pdivoom_alarm_active_menu_ctrl phandle, int cancel_alarm)
{
    if (!phandle || phandle->leaving)
    {
        return;
    }
    phandle->leaving = 1;
    if (phandle->refresh_timer)
    {
        lv_timer_pause(phandle->refresh_timer);
    }
    if (cancel_alarm)
    {
        divoom_alarm_process_alarm_menu_btn(0);
    }
    divoom_main_finish_clock_wake_poweron();
    divoom_menu_control_return_pre();
}

static void divoom_alarm_active_menu_sync_status(Pdivoom_alarm_active_menu_ctrl phandle)
{
    int alarm_status;
    int remain_time;

    if (!phandle)
    {
        return;
    }

    alarm_status = divoom_alarm_get_alarm_active_status(&remain_time);
    if (alarm_status != phandle->alarm_status)
    {
        if (alarm_status == DIVOOM_ALARM_STATUS_SNOOZE)
        {
            phandle->user_snoozed = 1;
            phandle->button_focus = DIVOOM_ALARM_ACTIVE_FOCUS_DISMISS;
            phandle->remain_time_last_disp = -1;
        }
        else if (alarm_status == DIVOOM_ALARM_STATUS_ACTIVE)
        {
            /* 贪睡倒计时结束再次响铃：恢复完整响铃 UI（与 disp 入口一致） */
            phandle->user_snoozed = 0;
            phandle->button_focus = DIVOOM_ALARM_ACTIVE_FOCUS_SNOOZE;
            phandle->remain_time_last_disp = -1;
        }
        phandle->alarm_status = alarm_status;
    }
    phandle->remain_time = remain_time;

    divoom_alarm_active_menu_layout_actions(phandle);
    divoom_alarm_active_menu_refresh_badge(phandle);
    divoom_alarm_active_menu_refresh_time(phandle);
    divoom_alarm_active_menu_refresh_palette(phandle);
}

static void divoom_alarm_active_menu_refresh_timer_cb(lv_timer_t *timer)
{
    Pdivoom_alarm_active_menu_ctrl phandle;

    phandle = (Pdivoom_alarm_active_menu_ctrl)lv_timer_get_user_data(timer);
    if (!phandle || phandle->leaving)
    {
        return;
    }
    divoom_alarm_active_menu_sync_status(phandle);
    if (phandle->alarm_status == DIVOOM_ALARM_STATUS_WAIT)
    {
        /* 响铃已结束（超时/被取消）但页还在：直接退出，不要停在无闹钟状态 */
        divoom_alarm_active_menu_leave(phandle, 0);
    }
}

static void divoom_alarm_active_menu_focus_btn(Pdivoom_alarm_active_menu_ctrl phandle, lv_obj_t *btn)
{
    lv_group_t *grp;

    if (!phandle || !btn)
    {
        return;
    }

    grp = lv_group_get_default();
    if (grp)
    {
        lv_group_focus_obj(btn);
    }
    divoom_astrootoo_ui_focus_scroll(btn);
    divoom_alarm_active_menu_refresh_palette(phandle);
}

static void divoom_alarm_active_menu_move_focus(Pdivoom_alarm_active_menu_ctrl phandle, int dir)
{
    int order[2];
    int idx;
    int next_idx;
    lv_obj_t *next_btn;

    if (!phandle || !divoom_alarm_active_menu_show_snooze(phandle))
    {
        return;
    }

    order[0] = DIVOOM_ALARM_ACTIVE_FOCUS_SNOOZE;
    order[1] = DIVOOM_ALARM_ACTIVE_FOCUS_DISMISS;
    idx = (phandle->button_focus == DIVOOM_ALARM_ACTIVE_FOCUS_SNOOZE) ? 0 : 1;
    next_idx = (idx + dir + 2) % 2;
    phandle->button_focus = order[next_idx];
    next_btn = (phandle->button_focus == DIVOOM_ALARM_ACTIVE_FOCUS_SNOOZE)
        ? phandle->snooze_btn
        : phandle->dismiss_btn;
    divoom_alarm_active_menu_focus_btn(phandle, next_btn);
}

static void divoom_alarm_active_menu_activate_focus(Pdivoom_alarm_active_menu_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }

    if (phandle->button_focus == DIVOOM_ALARM_ACTIVE_FOCUS_SNOOZE
        && divoom_alarm_active_menu_show_snooze(phandle))
    {
        divoom_alarm_process_alarm_menu_btn(1);
        phandle->user_snoozed = 1;
        phandle->button_focus = DIVOOM_ALARM_ACTIVE_FOCUS_DISMISS;
        divoom_alarm_active_menu_sync_status(phandle);
        divoom_alarm_active_menu_focus_btn(phandle, phandle->dismiss_btn);
        return;
    }

    divoom_alarm_active_menu_leave(phandle, 1);
}

static void divoom_alarm_active_menu_event_cb(lv_event_t *e)
{
    Pdivoom_alarm_active_menu_ctrl phandle;
    lv_event_code_t code;
    uint8_t key_code;

    phandle = (Pdivoom_alarm_active_menu_ctrl)lv_event_get_user_data(e);
    code = lv_event_get_code(e);
    if (!phandle || code != LV_EVENT_KEY)
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
        divoom_alarm_active_menu_leave(phandle, 1);
        return;
    }

    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        divoom_alarm_active_menu_activate_focus(phandle);
        return;
    }

    if (divoom_astrootoo_ui_key_is_up(key_code)
        || divoom_astrootoo_ui_key_is_down(key_code)
        || divoom_astrootoo_ui_key_is_left(key_code)
        || divoom_astrootoo_ui_key_is_right(key_code))
    {
        int dir = (divoom_astrootoo_ui_key_is_down(key_code)
            || divoom_astrootoo_ui_key_is_right(key_code)) ? 1 : -1;
        divoom_alarm_active_menu_move_focus(phandle, dir);
    }
}

static lv_obj_t *divoom_alarm_active_menu_create_action_btn(Pdivoom_alarm_active_menu_ctrl phandle,
    lv_obj_t *parent, const char *label_key, lv_event_cb_t cb)
{
    lv_obj_t *btn;
    lv_obj_t *label;

    btn = lv_btn_create(parent);
    lv_obj_remove_style_all(btn);
    lv_obj_add_style(btn, &phandle->btn_style, 0);
    lv_obj_add_style(btn, &phandle->btn_focus_style, LV_STATE_FOCUSED);
    lv_obj_add_style(btn, &phandle->btn_focus_style, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_radius(btn, DIVOOM_ASTRO_RADIUS_MD, 0);
    lv_obj_set_style_pad_hor(btn, 20, 0);
    lv_obj_set_style_pad_ver(btn, 0, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_KEY, phandle);

    label = lv_label_create(btn);
    lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(label, divoom_menu_control_get_disp_str(label_key));
    lv_obj_center(label);

    return btn;
}

static void divoom_alarm_active_menu_init_styles(Pdivoom_alarm_active_menu_ctrl phandle)
{
    if (!phandle || phandle->styles_initialized)
    {
        return;
    }

    lv_style_init(&phandle->root_style);
    lv_style_set_bg_color(&phandle->root_style, lv_color_hex(DIVOOM_ASTRO_ALARM_RING_BG));
    lv_style_set_border_width(&phandle->root_style, 0);
    lv_style_set_pad_all(&phandle->root_style, 0);
    lv_style_set_radius(&phandle->root_style, 0);

    lv_style_init(&phandle->btn_style);
    lv_style_set_bg_opa(&phandle->btn_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&phandle->btn_style, 2);
    lv_style_set_border_color(&phandle->btn_style, lv_color_hex(DIVOOM_ASTRO_ALARM_RING_MUTED));
    lv_style_set_radius(&phandle->btn_style, DIVOOM_ASTRO_RADIUS_MD);
    lv_style_set_pad_all(&phandle->btn_style, 0);

    lv_style_init(&phandle->btn_focus_style);
    lv_style_set_outline_width(&phandle->btn_focus_style, 0);

    phandle->styles_initialized = 1;
}

static void divoom_alarm_active_menu_reset_styles(Pdivoom_alarm_active_menu_ctrl phandle)
{
    if (!phandle || !phandle->styles_initialized)
    {
        return;
    }

    lv_style_reset(&phandle->root_style);
    lv_style_reset(&phandle->btn_style);
    lv_style_reset(&phandle->btn_focus_style);
    phandle->styles_initialized = 0;
}

static void divoom_alarm_active_menu_build_ui(Pdivoom_alarm_active_menu_ctrl phandle)
{
    lv_obj_t *badge_icon;
    lv_obj_t *badge_label;

    divoom_alarm_active_menu_init_styles(phandle);

    phandle->main_obj = lv_obj_create(NULL);
    lv_obj_add_style(phandle->main_obj, &phandle->root_style, 0);
    lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_SCROLLABLE);

    phandle->badge_row = lv_obj_create(phandle->main_obj);
    lv_obj_add_style(phandle->badge_row, &phandle->root_style, 0);
    lv_obj_set_size(phandle->badge_row, DIVOOM_SCREEN_WIDTH, DIVOOM_ASTRO_ALARM_RING_BADGE_ICON_H);
    lv_obj_set_pos(phandle->badge_row, 0, DIVOOM_ASTRO_ALARM_RING_BADGE_TOP_Y);
    lv_obj_clear_flag(phandle->badge_row, LV_OBJ_FLAG_SCROLLABLE);

    badge_icon = lv_img_create(phandle->badge_row);
    divoom_menu_control_lv_img_set_src(badge_icon,
        DIVOOM_ASTRO_ALARM_RING_UI_DIR "alarm_active_badge_muted.png");
    phandle->badge_icon = badge_icon;

    badge_label = lv_label_create(phandle->badge_row);
    lv_obj_set_style_text_font(badge_label, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
    lv_obj_set_style_text_color(badge_label, lv_color_hex(DIVOOM_ASTRO_ALARM_RING_MUTED), 0);
    lv_label_set_text(badge_label, divoom_menu_control_get_disp_str("Alarm"));
    phandle->badge_label = badge_label;

    lv_obj_update_layout(badge_label);
    divoom_alarm_active_menu_layout_badge_row(phandle);

    phandle->time_label = lv_label_create(phandle->main_obj);
    lv_obj_set_width(phandle->time_label, DIVOOM_SCREEN_WIDTH);
    lv_obj_set_style_text_font(phandle->time_label, divoom_menu_control_get_font_info(96, phandle->menu_id), 0);
    lv_obj_set_style_text_color(phandle->time_label, lv_color_hex(DIVOOM_ASTRO_ALARM_RING_MUTED), 0);
    lv_obj_set_style_text_align(phandle->time_label, LV_TEXT_ALIGN_CENTER, 0);

    phandle->snooze_btn = divoom_alarm_active_menu_create_action_btn(phandle, phandle->main_obj,
        "ringingSnooze", divoom_alarm_active_menu_event_cb);
    phandle->snooze_label = lv_obj_get_child(phandle->snooze_btn, 0);

    phandle->dismiss_btn = divoom_alarm_active_menu_create_action_btn(phandle, phandle->main_obj,
        "ringingDismiss", divoom_alarm_active_menu_event_cb);
    phandle->dismiss_label = lv_obj_get_child(phandle->dismiss_btn, 0);

    divoom_alarm_active_menu_sync_status(phandle);
    phandle->refresh_timer = lv_timer_create(divoom_alarm_active_menu_refresh_timer_cb, 200, phandle);
}

void *divoom_alarm_active_menu_init(void *user_data, int menu_id)
{
    Pdivoom_alarm_active_menu_ctrl phandle;

    (void)user_data;
    phandle = divoom_system_malloc(sizeof(divoom_alarm_active_menu_ctrl));
    if (!phandle)
    {
        return NULL;
    }

    phandle->menu_id = menu_id;
    phandle->alarm_status = DIVOOM_ALARM_STATUS_ACTIVE;
    phandle->remain_time = 0;
    phandle->remain_time_last_disp = -1;
    phandle->user_snoozed = 0;
    phandle->button_focus = DIVOOM_ALARM_ACTIVE_FOCUS_SNOOZE;
    phandle->leaving = 0;
    phandle->styles_initialized = 0;
    phandle->refresh_timer = NULL;

    divoom_alarm_active_menu_build_ui(phandle);
    return phandle;
}

void divoom_alarm_active_menu_disp(void *handle, void *active)
{
    Pdivoom_alarm_active_menu_ctrl phandle;
    lv_group_t *grp;
    lv_obj_t *focus_btn;

    (void)active;
    phandle = (Pdivoom_alarm_active_menu_ctrl)handle;
    if (!phandle)
    {
        return;
    }

    phandle->user_snoozed = 0;
    phandle->button_focus = DIVOOM_ALARM_ACTIVE_FOCUS_SNOOZE;
    divoom_alarm_active_menu_sync_status(phandle);

    lv_scr_load(phandle->main_obj);
    divoom_menu_control_change_disp_mode(phandle->main_obj, 0);

    grp = lv_group_get_default();
    if (grp)
    {
        lv_group_remove_all_objs(grp);
        lv_group_add_obj(grp, phandle->snooze_btn);
        lv_group_add_obj(grp, phandle->dismiss_btn);
    }

    focus_btn = divoom_alarm_active_menu_show_snooze(phandle)
        ? phandle->snooze_btn
        : phandle->dismiss_btn;
    divoom_alarm_active_menu_focus_btn(phandle, focus_btn);
}

void divoom_alarm_active_menu_exit(void *handle)
{
    Pdivoom_alarm_active_menu_ctrl phandle;

    phandle = (Pdivoom_alarm_active_menu_ctrl)handle;
    if (!phandle)
    {
        return;
    }

    if (phandle->refresh_timer)
    {
        lv_timer_del(phandle->refresh_timer);
        phandle->refresh_timer = NULL;
    }

    divoom_alarm_active_menu_reset_styles(phandle);
    if (phandle->main_obj)
    {
        lv_obj_del(phandle->main_obj);
        phandle->main_obj = NULL;
    }
    divoom_system_free(phandle);
}

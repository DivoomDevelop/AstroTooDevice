#include "divoom_astrootoo_time_pick.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom.h"
#include "divoom_main.h"
#include "divoom_menu_control.h"
#include <stdio.h>

static const divoom_astrootoo_time_pick_field_t g_tp_order_24[] = {
    DIVOOM_ASTRO_TP_FIELD_HOUR,
    DIVOOM_ASTRO_TP_FIELD_MINUTE,
    DIVOOM_ASTRO_TP_FIELD_CANCEL,
    DIVOOM_ASTRO_TP_FIELD_CONFIRM,
};

static const divoom_astrootoo_time_pick_field_t g_tp_order_12[] = {
    DIVOOM_ASTRO_TP_FIELD_AMPM,
    DIVOOM_ASTRO_TP_FIELD_HOUR,
    DIVOOM_ASTRO_TP_FIELD_MINUTE,
    DIVOOM_ASTRO_TP_FIELD_CANCEL,
    DIVOOM_ASTRO_TP_FIELD_CONFIRM,
};

int divoom_astrootoo_time_pick_use_hour24(void)
{
    return divoom_main_get_time_flag() ? 1 : 0;
}

int divoom_astrootoo_time_pick_is_pm(int hour24)
{
    int h24;

    h24 = ((hour24 % 24) + 24) % 24;
    return h24 >= 12 ? 1 : 0;
}

static const divoom_astrootoo_time_pick_field_t *divoom_astrootoo_time_pick_order(int use_hour24, int *cnt)
{
    if (use_hour24)
    {
        if (cnt)
        {
            *cnt = (int)(sizeof(g_tp_order_24) / sizeof(g_tp_order_24[0]));
        }
        return g_tp_order_24;
    }
    if (cnt)
    {
        *cnt = (int)(sizeof(g_tp_order_12) / sizeof(g_tp_order_12[0]));
    }
    return g_tp_order_12;
}

static int divoom_astrootoo_time_pick_order_index(int field, int use_hour24)
{
    const divoom_astrootoo_time_pick_field_t *order;
    int cnt;
    int i;

    order = divoom_astrootoo_time_pick_order(use_hour24, &cnt);
    for (i = 0; i < cnt; i++)
    {
        if (order[i] == (divoom_astrootoo_time_pick_field_t)field)
        {
            return i;
        }
    }
    return 0;
}

int divoom_astrootoo_time_pick_initial_field(int use_hour24)
{
    const divoom_astrootoo_time_pick_field_t *order;
    int cnt;

    order = divoom_astrootoo_time_pick_order(use_hour24, &cnt);
    return (int)order[0];
}

int divoom_astrootoo_time_pick_move_field(int field, int dir, int use_hour24)
{
    const divoom_astrootoo_time_pick_field_t *order;
    int cnt;
    int idx;

    order = divoom_astrootoo_time_pick_order(use_hour24, &cnt);
    idx = divoom_astrootoo_time_pick_order_index(field, use_hour24);
    idx = (idx + dir + cnt) % cnt;
    return (int)order[idx];
}

static const divoom_astrootoo_time_pick_field_t g_tp_value_order_24[] = {
    DIVOOM_ASTRO_TP_FIELD_HOUR,
    DIVOOM_ASTRO_TP_FIELD_MINUTE,
};

static const divoom_astrootoo_time_pick_field_t g_tp_value_order_12[] = {
    DIVOOM_ASTRO_TP_FIELD_AMPM,
    DIVOOM_ASTRO_TP_FIELD_HOUR,
    DIVOOM_ASTRO_TP_FIELD_MINUTE,
};

int divoom_astrootoo_time_pick_move_value_field(int field, int dir, int use_hour24)
{
    const divoom_astrootoo_time_pick_field_t *order;
    int cnt;
    int idx;
    int i;

    if (use_hour24)
    {
        order = g_tp_value_order_24;
        cnt = (int)(sizeof(g_tp_value_order_24) / sizeof(g_tp_value_order_24[0]));
    }
    else
    {
        order = g_tp_value_order_12;
        cnt = (int)(sizeof(g_tp_value_order_12) / sizeof(g_tp_value_order_12[0]));
    }
    idx = 0;
    for (i = 0; i < cnt; i++)
    {
        if (order[i] == (divoom_astrootoo_time_pick_field_t)field)
        {
            idx = i;
            break;
        }
    }
    idx = (idx + dir + cnt) % cnt;
    return (int)order[idx];
}

void divoom_astrootoo_time_pick_format_hour(char *buf, int len, int hour24, int use_hour24)
{
    int h24;

    if (!buf || len <= 0)
    {
        return;
    }
    h24 = ((hour24 % 24) + 24) % 24;
    if (use_hour24)
    {
        snprintf(buf, (size_t)len, "%02d", h24);
        return;
    }
    snprintf(buf, (size_t)len, "%d", h24 % 12 ? h24 % 12 : 12);
}

void divoom_astrootoo_time_pick_format_clock_label(char *buf, int len, int hour24, int minute)
{
    int h24;
    int min;
    int use_hour24;

    if (!buf || len <= 0)
    {
        return;
    }
    h24 = ((hour24 % 24) + 24) % 24;
    min = ((minute % 60) + 60) % 60;
    use_hour24 = divoom_astrootoo_time_pick_use_hour24();
    if (use_hour24)
    {
        snprintf(buf, (size_t)len, "%02d:%02d", h24, min);
        return;
    }
    {
        int h12;

        h12 = h24 % 12;
        if (h12 == 0)
        {
            h12 = 12;
        }
        snprintf(buf, (size_t)len, "%d:%02d", h12, min);
    }
}

static void divoom_astrootoo_time_pick_bump_ampm(divoom_astrootoo_time_pick_state_t *st)
{
    int h24;

    h24 = ((st->hour24 % 24) + 24) % 24;
    if (h24 < 12)
    {
        st->hour24 = h24 + 12;
    }
    else
    {
        st->hour24 = h24 - 12;
    }
}

static void divoom_astrootoo_time_pick_bump_hour12(divoom_astrootoo_time_pick_state_t *st, int dir)
{
    int h24;
    int is_pm;
    int h12;
    int new_h24;

    h24 = ((st->hour24 % 24) + 24) % 24;
    is_pm = h24 >= 12 ? 1 : 0;
    h12 = h24 % 12;
    if (h12 == 0)
    {
        h12 = 12;
    }
    h12 = ((h12 - 1 + dir) % 12 + 12) % 12 + 1;
    if (h12 == 12)
    {
        new_h24 = is_pm ? 12 : 0;
    }
    else
    {
        new_h24 = is_pm ? h12 + 12 : h12;
    }
    st->hour24 = new_h24;
}

void divoom_astrootoo_time_pick_bump(divoom_astrootoo_time_pick_state_t *st, int dir)
{
    if (!st)
    {
        return;
    }
    if (st->field == DIVOOM_ASTRO_TP_FIELD_MINUTE)
    {
        st->minute = (st->minute + dir + 60) % 60;
        return;
    }
    if (st->field == DIVOOM_ASTRO_TP_FIELD_AMPM)
    {
        divoom_astrootoo_time_pick_bump_ampm(st);
        return;
    }
    if (st->field == DIVOOM_ASTRO_TP_FIELD_HOUR)
    {
        if (st->use_hour24)
        {
            st->hour24 = ((st->hour24 + dir) % 24 + 24) % 24;
        }
        else
        {
            divoom_astrootoo_time_pick_bump_hour12(st, dir);
        }
    }
}

divoom_astrootoo_time_pick_key_action_t divoom_astrootoo_time_pick_handle_key(
    divoom_astrootoo_time_pick_state_t *st, uint8 key_code, int esc_confirms)
{
    if (!st)
    {
        return DIVOOM_ASTRO_TP_KEY_HANDLED;
    }

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        if (esc_confirms)
        {
            return DIVOOM_ASTRO_TP_KEY_CONFIRM;
        }
        return DIVOOM_ASTRO_TP_KEY_CANCEL;
    }

    if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code))
    {
        int dir;

        dir = divoom_astrootoo_ui_key_is_up(key_code) ? 1 : -1;
        if (st->field == DIVOOM_ASTRO_TP_FIELD_HOUR
            || st->field == DIVOOM_ASTRO_TP_FIELD_MINUTE
            || st->field == DIVOOM_ASTRO_TP_FIELD_AMPM)
        {
            divoom_astrootoo_time_pick_bump(st, dir);
        }
        return DIVOOM_ASTRO_TP_KEY_HANDLED;
    }

    if (divoom_astrootoo_ui_key_is_left(key_code) || divoom_astrootoo_ui_key_is_right(key_code))
    {
        int dir;

        dir = divoom_astrootoo_ui_key_is_left(key_code) ? -1 : 1;
        st->field = divoom_astrootoo_time_pick_move_field(st->field, dir, st->use_hour24);
        return DIVOOM_ASTRO_TP_KEY_HANDLED;
    }

    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        if (st->field == DIVOOM_ASTRO_TP_FIELD_CANCEL)
        {
            return DIVOOM_ASTRO_TP_KEY_CANCEL;
        }
        if (st->field == DIVOOM_ASTRO_TP_FIELD_CONFIRM)
        {
            return DIVOOM_ASTRO_TP_KEY_CONFIRM;
        }
        if (st->field == DIVOOM_ASTRO_TP_FIELD_HOUR
            || st->field == DIVOOM_ASTRO_TP_FIELD_MINUTE
            || st->field == DIVOOM_ASTRO_TP_FIELD_AMPM)
        {
            st->field = DIVOOM_ASTRO_TP_FIELD_CONFIRM;
        }
        return DIVOOM_ASTRO_TP_KEY_HANDLED;
    }

    return DIVOOM_ASTRO_TP_KEY_HANDLED;
}

void divoom_astrootoo_time_pick_refresh_widgets(const divoom_astrootoo_time_pick_widgets_t *widgets,
    int field, int hour24, int minute, int use_hour24)
{
    char buf[16];
    const char *ap;

    if (!widgets)
    {
        return;
    }

    if (widgets->ampm_box)
    {
        if (use_hour24)
        {
            lv_obj_add_flag(widgets->ampm_box, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_clear_flag(widgets->ampm_box, LV_OBJ_FLAG_HIDDEN);
            ap = divoom_menu_control_get_disp_str(
                divoom_astrootoo_time_pick_is_pm(hour24) ? "PM" : "AM");
            if (widgets->ampm_lbl && ap)
            {
                lv_label_set_text(widgets->ampm_lbl, ap);
            }
            divoom_astrootoo_settings_apply_time_pick_slot(widgets->ampm_box,
                field == DIVOOM_ASTRO_TP_FIELD_AMPM);
        }
    }

    divoom_astrootoo_time_pick_format_hour(buf, (int)sizeof(buf), hour24, use_hour24);
    if (widgets->h_lbl)
    {
        lv_label_set_text(widgets->h_lbl, buf);
    }
    snprintf(buf, sizeof(buf), "%02d", minute);
    if (widgets->m_lbl)
    {
        lv_label_set_text(widgets->m_lbl, buf);
    }
    if (widgets->h_box)
    {
        divoom_astrootoo_settings_apply_time_pick_slot(widgets->h_box,
            field == DIVOOM_ASTRO_TP_FIELD_HOUR);
    }
    if (widgets->m_box)
    {
        divoom_astrootoo_settings_apply_time_pick_slot(widgets->m_box,
            field == DIVOOM_ASTRO_TP_FIELD_MINUTE);
    }
    if (widgets->cancel)
    {
        divoom_astrootoo_settings_apply_time_pick_slot(widgets->cancel,
            field == DIVOOM_ASTRO_TP_FIELD_CANCEL);
    }
    if (widgets->confirm)
    {
        divoom_astrootoo_settings_apply_time_pick_slot(widgets->confirm,
            field == DIVOOM_ASTRO_TP_FIELD_CONFIRM);
    }
    if (widgets->h_cap)
    {
        lv_obj_set_style_text_color(widgets->h_cap,
            lv_color_hex(field == DIVOOM_ASTRO_TP_FIELD_HOUR
                ? DIVOOM_ASTRO_INK : DIVOOM_ASTRO_INK_MUTED), 0);
    }
    if (widgets->m_cap)
    {
        lv_obj_set_style_text_color(widgets->m_cap,
            lv_color_hex(field == DIVOOM_ASTRO_TP_FIELD_MINUTE
                ? DIVOOM_ASTRO_INK : DIVOOM_ASTRO_INK_MUTED), 0);
    }
}

lv_obj_t *divoom_astrootoo_time_pick_focus_obj(const divoom_astrootoo_time_pick_widgets_t *widgets,
    int field)
{
    if (!widgets)
    {
        return NULL;
    }
    switch (field)
    {
    case DIVOOM_ASTRO_TP_FIELD_AMPM:
    {
        return widgets->ampm_box;
    }
    case DIVOOM_ASTRO_TP_FIELD_HOUR:
    {
        return widgets->h_box;
    }
    case DIVOOM_ASTRO_TP_FIELD_MINUTE:
    {
        return widgets->m_box;
    }
    case DIVOOM_ASTRO_TP_FIELD_CANCEL:
    {
        return widgets->cancel;
    }
    case DIVOOM_ASTRO_TP_FIELD_CONFIRM:
    {
        return widgets->confirm;
    }
    default:
    {
        break;
    }
    }
    return widgets->h_box;
}

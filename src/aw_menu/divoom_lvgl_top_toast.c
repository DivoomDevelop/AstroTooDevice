#include "divoom.h"
#include <string.h>
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_astrootoo_theme.h"
#include "divoom_lvgl_font_manage.h"
#include "divoom_lvgl_top_control.h"
#include "divoom_lvgl_top_toast.h"
#include "divoom_tf_music.h"

enum {
    DIVOOM_ASTRO_UI_TOAST_NONE = 0,
    DIVOOM_ASTRO_UI_TOAST_SCAN,
    DIVOOM_ASTRO_UI_TOAST_TIMED,
};

static lv_obj_t *s_astro_toast;
static lv_obj_t *s_astro_toast_label;
static lv_timer_t *s_astro_toast_timer;
static uint8 s_astro_toast_kind;

static void divoom_lvgl_top_toast_drop_timer(void)
{
    if (s_astro_toast_timer)
    {
        lv_timer_del(s_astro_toast_timer);
        s_astro_toast_timer = NULL;
    }
}

static void divoom_lvgl_top_toast_sync(void)
{
    if (s_astro_toast && !lv_obj_is_valid(s_astro_toast))
    {
        s_astro_toast = NULL;
        s_astro_toast_label = NULL;
        s_astro_toast_kind = DIVOOM_ASTRO_UI_TOAST_NONE;
    }
}

void divoom_lvgl_top_toast_hide(void)
{
    divoom_lvgl_top_toast_drop_timer();
    divoom_lvgl_top_toast_sync();
    if (s_astro_toast)
    {
        lv_obj_del(s_astro_toast);
        s_astro_toast = NULL;
        s_astro_toast_label = NULL;
    }
    s_astro_toast_kind = DIVOOM_ASTRO_UI_TOAST_NONE;
}

void divoom_lvgl_top_toast_hide_timed(void)
{
    if (s_astro_toast_kind == DIVOOM_ASTRO_UI_TOAST_TIMED)
        divoom_lvgl_top_toast_hide();
}

static void divoom_lvgl_top_toast_timer_cb(lv_timer_t *timer)
{
    if (s_astro_toast_timer == timer)
        s_astro_toast_timer = NULL;
    lv_timer_del(timer);
    if (s_astro_toast_kind == DIVOOM_ASTRO_UI_TOAST_TIMED)
        divoom_lvgl_top_toast_hide();
}

void divoom_lvgl_top_toast_show(const char *text, uint32_t duration_ms)
{
    lv_obj_t *toast;
    lv_obj_t *label;

    if (!text)
        return;

    divoom_lvgl_top_toast_drop_timer();
    divoom_lvgl_top_toast_sync();

    if (s_astro_toast && s_astro_toast_label)
    {
        lv_label_set_text(s_astro_toast_label, text);
        divoom_lvgl_layer_top_raise(s_astro_toast);
    }
    else
    {
        toast = lv_obj_create(lv_layer_top());
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
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
        lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(22, 0), 0);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

        divoom_lvgl_layer_top_raise(toast);
        s_astro_toast = toast;
        s_astro_toast_label = label;
    }

    s_astro_toast_kind = duration_ms ? DIVOOM_ASTRO_UI_TOAST_TIMED : DIVOOM_ASTRO_UI_TOAST_SCAN;
    if (duration_ms > 0)
    {
        s_astro_toast_timer = lv_timer_create(divoom_lvgl_top_toast_timer_cb,
            duration_ms, NULL);
    }
}

void divoom_lvgl_top_toast_poll(void)
{
    const char *title;

    if (!divoom_lvgl_font_manage_is_inited())
        return;

    if (divoom_menu_control_get_active_menu_id() == DIVOOM_MENU_CONTROL_ID_SYSTEM_POWER_ON)
    {
        divoom_lvgl_top_toast_hide();
        return;
    }

    if (divoom_tf_music_is_songlist_refreshing())
    {
        if (s_astro_toast_kind != DIVOOM_ASTRO_UI_TOAST_SCAN)
        {
            title = divoom_menu_control_get_disp_str("SourceTfScanningTitle");
            divoom_lvgl_top_toast_show(title, 0);
        }
        return;
    }

    if (s_astro_toast_kind == DIVOOM_ASTRO_UI_TOAST_SCAN)
        divoom_lvgl_top_toast_hide();
}

#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_main.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_astrootoo_oobe_welcome.h"

#define ASTRO_OOBE_UI_DIR               "53/"
#define ASTRO_OOBE_WELCOME_GRAPHICS     ASTRO_OOBE_UI_DIR "oobe_welcome_graphics.png"

#define OOBE_TOP_Y                      22
#define OOBE_BADGE_SIZE                 32
#define OOBE_BADGE_X                    22
#define OOBE_BADGE_BG                   0xFBF6F4
#define OOBE_BADGE_BORDER               0xF8B634
#define OOBE_BADGE_INK                  0xF8B634
#define OOBE_STEP_INK                   0x3D3D3D
#define OOBE_SKIP_INK                   0x9A938B
#define OOBE_TOP_BAR_W                  (DIVOOM_SCREEN_WIDTH - OOBE_BADGE_X - 16)
#define OOBE_TOP_BAR_SKIP_MAX_W         220
#define OOBE_TOP_BAR_COL_PAD            8
#define OOBE_WELCOME_SUB_INK            0x6D6B6A
#define OOBE_WELCOME_SUB_Y              385
#define OOBE_WELCOME_GFX_X              78
#define OOBE_WELCOME_GFX_Y              101
#define OOBE_WELCOME_GFX_W              324
#define OOBE_WELCOME_GFX_H              230

typedef struct _divoom_astrootoo_oobe_welcome_ctrl
{
    divoom_astrootoo_ui_base ui;
    int menu_id;
    lv_timer_t *timer_handle;
    lv_obj_t *root_obj;
    lv_obj_t *key_btn;
} divoom_astrootoo_oobe_welcome_ctrl, *Pdivoom_astrootoo_oobe_welcome_ctrl;

static void divoom_astrootoo_oobe_welcome_advance(Pdivoom_astrootoo_oobe_welcome_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }
    if (phandle->timer_handle)
    {
        lv_timer_del(phandle->timer_handle);
        phandle->timer_handle = NULL;
    }
    divoom_menu_control_load_next_pop_cur(DIVOOM_MENU_CONTROL_ID_ASTROTOO_OOBE_CONFIG_WIFI, NULL, 0, 0);
}

static void divoom_astrootoo_oobe_welcome_timer_cb(lv_timer_t *timer)
{
    divoom_astrootoo_oobe_welcome_advance((Pdivoom_astrootoo_oobe_welcome_ctrl)lv_timer_get_user_data(timer));
}

static void divoom_astrootoo_oobe_welcome_key_cb(lv_event_t *e)
{
    (void)e;
}

#define OOBE_WELCOME_STEP_FONT          20
#define OOBE_WELCOME_SKIP_FONT          18

static void divoom_astrootoo_oobe_welcome_style_top_bar_label(lv_obj_t *lbl,
    lv_coord_t max_w, lv_text_align_t align)
{
    if (!lbl)
    {
        return;
    }

    lv_obj_set_width(lbl, max_w);
    lv_obj_set_height(lbl, LV_SIZE_CONTENT);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(lbl, align, 0);
    lv_obj_add_flag(lbl, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
}

static void divoom_astrootoo_oobe_welcome_update_top_bar_layout(lv_obj_t *bar,
    lv_obj_t *step_lbl, lv_obj_t *skip_lbl)
{
    lv_obj_t *left;
    lv_coord_t bar_h;
    lv_coord_t step_h;
    lv_coord_t skip_h;
    lv_coord_t step_w;

    if (!bar || !step_lbl || !skip_lbl)
    {
        return;
    }

    step_w = OOBE_TOP_BAR_W - OOBE_BADGE_SIZE - OOBE_TOP_BAR_COL_PAD - OOBE_TOP_BAR_SKIP_MAX_W;
    divoom_astrootoo_oobe_welcome_style_top_bar_label(step_lbl, step_w, LV_TEXT_ALIGN_LEFT);
    divoom_astrootoo_oobe_welcome_style_top_bar_label(skip_lbl, OOBE_TOP_BAR_SKIP_MAX_W,
        LV_TEXT_ALIGN_RIGHT);

    lv_obj_update_layout(step_lbl);
    step_h = lv_obj_get_height(step_lbl);
    lv_obj_update_layout(skip_lbl);
    skip_h = lv_obj_get_height(skip_lbl);

    bar_h = step_h > skip_h ? step_h : skip_h;
    if (bar_h < OOBE_BADGE_SIZE)
    {
        bar_h = OOBE_BADGE_SIZE;
    }

    left = lv_obj_get_parent(step_lbl);
    lv_obj_set_height(bar, bar_h);
    if (left)
    {
        lv_obj_set_height(left, bar_h);
    }
}

static lv_obj_t *divoom_astrootoo_oobe_welcome_create_top_bar(lv_obj_t *parent, int menu_id,
    const char *step_no, const char *step_key, const char *skip_key)
{
    lv_obj_t *bar;
    lv_obj_t *left;
    lv_obj_t *badge;
    lv_obj_t *badge_lbl;
    lv_obj_t *step_lbl;
    lv_obj_t *skip_lbl;
    lv_coord_t bar_h;

    bar_h = OOBE_BADGE_SIZE;

    bar = lv_obj_create(parent);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, OOBE_TOP_BAR_W, bar_h);
    lv_obj_set_pos(bar, OOBE_BADGE_X, OOBE_TOP_Y);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(bar, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    left = lv_obj_create(bar);
    lv_obj_remove_style_all(left);
    lv_obj_set_flex_grow(left, 1);
    lv_obj_set_width(left, 1);
    lv_obj_set_height(left, bar_h);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(left, OOBE_TOP_BAR_COL_PAD, 0);
    lv_obj_add_flag(left, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

    badge = lv_obj_create(left);
    lv_obj_remove_style_all(badge);
    lv_obj_set_size(badge, OOBE_BADGE_SIZE, OOBE_BADGE_SIZE);
    lv_obj_set_style_bg_color(badge, lv_color_hex(OOBE_BADGE_BG), 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(badge, lv_color_hex(OOBE_BADGE_BORDER), 0);
    lv_obj_set_style_border_width(badge, 2, 0);
    lv_obj_set_style_radius(badge, 8, 0);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

    badge_lbl = lv_label_create(badge);
    lv_label_set_text(badge_lbl, step_no);
    lv_obj_set_style_text_color(badge_lbl, lv_color_hex(OOBE_BADGE_INK), 0);
    lv_obj_set_style_text_font(badge_lbl, divoom_menu_control_get_font_info(20, menu_id), 0);
    lv_obj_center(badge_lbl);

    step_lbl = lv_label_create(left);
    lv_label_set_text(step_lbl, divoom_menu_control_get_disp_str(step_key));
    lv_obj_set_style_text_color(step_lbl, lv_color_hex(OOBE_STEP_INK), 0);
    lv_obj_set_style_text_font(step_lbl, divoom_menu_control_get_font_info(OOBE_WELCOME_STEP_FONT, menu_id), 0);

    skip_lbl = lv_label_create(bar);
    lv_label_set_text(skip_lbl, divoom_menu_control_get_disp_str(skip_key));
    lv_obj_set_style_text_color(skip_lbl, lv_color_hex(OOBE_SKIP_INK), 0);
    lv_obj_set_style_text_font(skip_lbl, divoom_menu_control_get_font_info(OOBE_WELCOME_SKIP_FONT, menu_id), 0);

    divoom_astrootoo_oobe_welcome_update_top_bar_layout(bar, step_lbl, skip_lbl);

    return bar;
}

void *divoom_astrootoo_oobe_welcome_init(void *user_data, int menu_id)
{
    Pdivoom_astrootoo_oobe_welcome_ctrl phandle;
    lv_obj_t *gfx;
    lv_obj_t *top_bar;
    lv_obj_t *sub1;
    const char *sub;

    (void)user_data;

    phandle = divoom_system_malloc_out(sizeof(divoom_astrootoo_oobe_welcome_ctrl));
    if (!phandle)
    {
        return NULL;
    }

    phandle->menu_id = menu_id;
    phandle->timer_handle = NULL;
    phandle->root_obj = divoom_astrootoo_ui_create_root(&phandle->ui);

    gfx = lv_img_create(phandle->root_obj);
    divoom_menu_control_lv_img_set_src(gfx, ASTRO_OOBE_WELCOME_GRAPHICS);
    lv_obj_set_pos(gfx, OOBE_WELCOME_GFX_X, OOBE_WELCOME_GFX_Y);
    lv_obj_set_size(gfx, OOBE_WELCOME_GFX_W, OOBE_WELCOME_GFX_H);

    top_bar = divoom_astrootoo_oobe_welcome_create_top_bar(phandle->root_obj, menu_id,
        "1", "OobeWelcomeStepName", "OobeSkipHint");

    sub = divoom_menu_control_get_disp_str("OobeWelcomeSubtitle");
    sub1 = lv_label_create(phandle->root_obj);
    lv_obj_set_width(sub1, DIVOOM_SCREEN_WIDTH - 32);
    lv_label_set_long_mode(sub1, LV_LABEL_LONG_WRAP);
    lv_label_set_text(sub1, sub);
    lv_obj_set_style_text_color(sub1, lv_color_hex(OOBE_WELCOME_SUB_INK), 0);
    lv_obj_set_style_text_align(sub1, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(sub1, divoom_menu_control_get_font_info(24, menu_id), 0);
    lv_obj_align(sub1, LV_ALIGN_TOP_MID, 0, OOBE_WELCOME_SUB_Y);

    lv_obj_move_foreground(top_bar);
    lv_obj_move_foreground(sub1);

    phandle->key_btn = divoom_astrootoo_ui_create_key_capture_btn(phandle->root_obj, &phandle->ui,
        divoom_astrootoo_oobe_welcome_key_cb, phandle);
    lv_obj_move_foreground(phandle->key_btn);
    lv_group_add_obj(lv_group_get_default(), phandle->key_btn);

    phandle->timer_handle = lv_timer_create(divoom_astrootoo_oobe_welcome_timer_cb, 2000, phandle);
    lv_timer_set_repeat_count(phandle->timer_handle, 1);

    return phandle;
}

void divoom_astrootoo_oobe_welcome_disp(void *handle, void *active_obj)
{
    Pdivoom_astrootoo_oobe_welcome_ctrl phandle;

    (void)active_obj;
    phandle = (Pdivoom_astrootoo_oobe_welcome_ctrl)handle;
    if (!phandle)
    {
        return;
    }

    lv_scr_load(phandle->root_obj);
    if (phandle->key_btn)
    {
        lv_group_focus_obj(phandle->key_btn);
    }
}

void divoom_astrootoo_oobe_welcome_pause(void *handle)
{
    (void)handle;
}

void divoom_astrootoo_oobe_welcome_exit(void *handle)
{
    Pdivoom_astrootoo_oobe_welcome_ctrl phandle;

    phandle = (Pdivoom_astrootoo_oobe_welcome_ctrl)handle;
    if (!phandle)
    {
        return;
    }

    if (phandle->timer_handle)
    {
        lv_timer_del(phandle->timer_handle);
    }
    if (phandle->root_obj)
    {
        lv_obj_del(phandle->root_obj);
    }
    divoom_astrootoo_ui_base_reset_styles(&phandle->ui);
    divoom_system_free(phandle);
}

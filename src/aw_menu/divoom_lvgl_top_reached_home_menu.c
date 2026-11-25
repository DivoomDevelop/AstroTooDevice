#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_top_reached_home_menu.h"
#include "divoom_astrootoo_theme.h"
#include "divoom_lvgl_top_control.h"

/* AstroTooUI FlowtooSystemNotice.toastTop + atTopStack（480×480） */
/* 真源：AstroTooUI public/flowtoo/icons/at-top-stack.png（28×27），居中垫至 32×32 后 1:1 显示 */
#define DIVOOM_ASTRO_TOP_TOAST_UI_DIR       "82/"
#define DIVOOM_ASTRO_TOP_TOAST_ICON_FILE    DIVOOM_ASTRO_TOP_TOAST_UI_DIR "at_top_stack.png"

typedef struct _divoom_lvgl_top_reached_home_menu_ctrl
{
    lv_obj_t *main_obj;
    lv_timer_t *menu_exit_timer;
    lv_style_t panel_style;
    lv_style_t cont_style;
    int exit_time;
    uint8 styles_initialized;
} divoom_lvgl_top_reached_home_menu_ctrl, *Pdivoom_lvgl_top_reached_home_menu_ctrl;

static divoom_lvgl_top_reached_home_menu_ctrl g_top_reached_home_ctrl;
static Pdivoom_lvgl_top_reached_home_menu_ctrl pdivoom_lvgl_top_reached_home_menu_handle = NULL;

static void divoom_lvgl_top_reached_home_menu_reset_styles(Pdivoom_lvgl_top_reached_home_menu_ctrl phandle)
{
    if (phandle && phandle->styles_initialized)
    {
        lv_style_reset(&phandle->panel_style);
        lv_style_reset(&phandle->cont_style);
        phandle->styles_initialized = 0;
    }
}

static void divoom_lvgl_top_reached_home_menu_del_menu(void)
{
    if (pdivoom_lvgl_top_reached_home_menu_handle == NULL)
    {
        return;
    }

    if (pdivoom_lvgl_top_reached_home_menu_handle->menu_exit_timer)
    {
        lv_timer_del(pdivoom_lvgl_top_reached_home_menu_handle->menu_exit_timer);
        pdivoom_lvgl_top_reached_home_menu_handle->menu_exit_timer = NULL;
    }

    if (pdivoom_lvgl_top_reached_home_menu_handle->main_obj)
    {
        lv_obj_del_async(pdivoom_lvgl_top_reached_home_menu_handle->main_obj);
        pdivoom_lvgl_top_reached_home_menu_handle->main_obj = NULL;
    }

    divoom_lvgl_top_reached_home_menu_reset_styles(pdivoom_lvgl_top_reached_home_menu_handle);
    pdivoom_lvgl_top_reached_home_menu_handle = NULL;
}

static void divoom_lvgl_top_reached_home_menu_time_handle(lv_timer_t *exit_timer)
{
    (void)exit_timer;

    if (pdivoom_lvgl_top_reached_home_menu_handle == NULL)
    {
        return;
    }

    pdivoom_lvgl_top_reached_home_menu_handle->exit_time++;
    if (pdivoom_lvgl_top_reached_home_menu_handle->exit_time >= DIVOOM_ASTRO_TOP_TOAST_DISMISS_TICKS)
    {
        divoom_lvgl_top_reached_home_menu_del_menu();
    }
}

static void divoom_lvgl_top_reached_home_menu_create_ui(Pdivoom_lvgl_top_reached_home_menu_ctrl phandle)
{
    lv_obj_t *icon;
    lv_obj_t *text_cont;
    lv_obj_t *label;
    lv_obj_t *spacer;
    lv_coord_t text_w;

    phandle->exit_time = 0;
    phandle->styles_initialized = 0;

    lv_style_init(&phandle->panel_style);
    lv_style_set_bg_color(&phandle->panel_style, lv_color_hex(DIVOOM_ASTRO_BG));
    lv_style_set_bg_opa(&phandle->panel_style, LV_OPA_COVER);
    lv_style_set_border_width(&phandle->panel_style, 2);
    lv_style_set_border_color(&phandle->panel_style, lv_color_hex(DIVOOM_ASTRO_STROKE));
    lv_style_set_radius(&phandle->panel_style, DIVOOM_ASTRO_RADIUS_SM);
    lv_style_set_pad_top(&phandle->panel_style, DIVOOM_ASTRO_TOP_TOAST_PAD_Y);
    lv_style_set_pad_bottom(&phandle->panel_style, DIVOOM_ASTRO_TOP_TOAST_PAD_Y);
    lv_style_set_pad_left(&phandle->panel_style, DIVOOM_ASTRO_TOP_TOAST_PAD_X);
    lv_style_set_pad_right(&phandle->panel_style, DIVOOM_ASTRO_TOP_TOAST_PAD_X);
    lv_style_set_pad_column(&phandle->panel_style, DIVOOM_ASTRO_TOP_TOAST_GAP);
    lv_style_set_shadow_width(&phandle->panel_style, 0);
    lv_style_set_outline_width(&phandle->panel_style, 0);

    lv_style_init(&phandle->cont_style);
    lv_style_set_bg_opa(&phandle->cont_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&phandle->cont_style, 0);
    lv_style_set_pad_all(&phandle->cont_style, 0);
    lv_style_set_pad_gap(&phandle->cont_style, 0);
    lv_style_set_radius(&phandle->cont_style, 0);
    phandle->styles_initialized = 1;

    phandle->main_obj = lv_obj_create(lv_layer_top());
    lv_obj_add_style(phandle->main_obj, &phandle->panel_style, 0);
    lv_obj_set_pos(phandle->main_obj, DIVOOM_ASTRO_TOP_TOAST_X, DIVOOM_ASTRO_TOP_TOAST_Y);
    lv_obj_set_size(phandle->main_obj, DIVOOM_ASTRO_TOP_TOAST_W, DIVOOM_ASTRO_TOP_TOAST_MIN_H);
    lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_flex_flow(phandle->main_obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(phandle->main_obj, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    icon = lv_img_create(phandle->main_obj);
    divoom_menu_control_lv_img_set_src(icon, DIVOOM_ASTRO_TOP_TOAST_ICON_FILE);
    lv_obj_set_size(icon, DIVOOM_ASTRO_TOP_TOAST_ICON, DIVOOM_ASTRO_TOP_TOAST_ICON);

    text_w = DIVOOM_ASTRO_TOP_TOAST_W
        - DIVOOM_ASTRO_TOP_TOAST_PAD_X * 2
        - DIVOOM_ASTRO_TOP_TOAST_ICON * 2
        - DIVOOM_ASTRO_TOP_TOAST_GAP * 2;
    if (text_w < 120)
    {
        text_w = 120;
    }

    text_cont = lv_obj_create(phandle->main_obj);
    lv_obj_add_style(text_cont, &phandle->cont_style, 0);
    lv_obj_set_size(text_cont, text_w, LV_SIZE_CONTENT);
    lv_obj_clear_flag(text_cont, LV_OBJ_FLAG_SCROLLABLE);

    label = lv_label_create(text_cont);
    lv_obj_set_width(label, text_w);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(24, DIVOOM_MENU_CONTROL_ID_TOP_REACHED_HOME_MENU), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_label_set_text(label, divoom_menu_control_get_disp_str("ReachedHome"));
    lv_obj_center(label);

    spacer = lv_obj_create(phandle->main_obj);
    lv_obj_add_style(spacer, &phandle->cont_style, 0);
    lv_obj_set_size(spacer, DIVOOM_ASTRO_TOP_TOAST_ICON, DIVOOM_ASTRO_TOP_TOAST_ICON);
    lv_obj_clear_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);

    phandle->menu_exit_timer = lv_timer_create(divoom_lvgl_top_reached_home_menu_time_handle, 200, NULL);
    divoom_lvgl_layer_top_raise(phandle->main_obj);
}

void divoom_lvgl_top_reached_home_menu_init(void)
{
    if (divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID)
    {
        return;
    }

    divoom_get_lvgl_lock();

    if (pdivoom_lvgl_top_reached_home_menu_handle != NULL)
    {
        pdivoom_lvgl_top_reached_home_menu_handle->exit_time = 0;
        divoom_lvgl_layer_top_raise(pdivoom_lvgl_top_reached_home_menu_handle->main_obj);
        divoom_free_lvgl_lock();
        return;
    }

    memset(&g_top_reached_home_ctrl, 0, sizeof(g_top_reached_home_ctrl));
    pdivoom_lvgl_top_reached_home_menu_handle = &g_top_reached_home_ctrl;
    divoom_lvgl_top_reached_home_menu_create_ui(pdivoom_lvgl_top_reached_home_menu_handle);

    divoom_free_lvgl_lock();
}

void divoom_lvgl_top_reached_home_menu_exit(void)
{
    divoom_get_lvgl_lock();
    divoom_lvgl_top_reached_home_menu_del_menu();
    divoom_free_lvgl_lock();
}

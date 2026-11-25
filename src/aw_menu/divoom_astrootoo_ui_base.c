#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_menu_direct_ctrl.h"
#include "divoom_astrootoo_ui_base.h"

void divoom_astrootoo_ui_base_init_styles(Pdivoom_astrootoo_ui_base phandle)
{
    if (!phandle || phandle->styles_initialized)
    {
        return;
    }

    lv_style_init(&phandle->root_style);
    /* golden 实测背景为接近 flat 的暖米色（非渐变），用 flat bg 即可匹配 */
    lv_style_set_bg_color(&phandle->root_style, lv_color_hex(DIVOOM_ASTRO_BG));
    lv_style_set_border_width(&phandle->root_style, 0);
    lv_style_set_pad_all(&phandle->root_style, 0);
    lv_style_set_radius(&phandle->root_style, 0);

    lv_style_init(&phandle->content_style);
    lv_style_set_bg_color(&phandle->content_style, lv_color_hex(DIVOOM_ASTRO_BG));
    lv_style_set_border_width(&phandle->content_style, 0);
    lv_style_set_pad_all(&phandle->content_style, 0);
    lv_style_set_radius(&phandle->content_style, DIVOOM_ASTRO_RADIUS_SM);

    lv_style_init(&phandle->grid_cont_style);
    lv_style_set_bg_color(&phandle->grid_cont_style, lv_color_hex(DIVOOM_ASTRO_BG));
    lv_style_set_border_width(&phandle->grid_cont_style, 0);
    lv_style_set_pad_all(&phandle->grid_cont_style, 0);
    lv_style_set_pad_gap(&phandle->grid_cont_style, 0);
    lv_style_set_radius(&phandle->grid_cont_style, DIVOOM_ASTRO_RADIUS_SM);

    lv_style_init(&phandle->list_item_style);
    lv_style_set_bg_color(&phandle->list_item_style, lv_color_hex(DIVOOM_ASTRO_SURFACE_IDLE));
    lv_style_set_border_color(&phandle->list_item_style, lv_color_hex(DIVOOM_ASTRO_ITEM_BORDER_INACTIVE));
    lv_style_set_border_width(&phandle->list_item_style, DIVOOM_ASTRO_ITEM_BORDER_W_INACTIVE);
    lv_style_set_radius(&phandle->list_item_style, DIVOOM_ASTRO_RADIUS_SM);
    lv_style_set_pad_all(&phandle->list_item_style, 0);

    lv_style_init(&phandle->list_item_focus_style);
    lv_style_set_bg_color(&phandle->list_item_focus_style, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS));
    lv_style_set_border_color(&phandle->list_item_focus_style, lv_color_hex(DIVOOM_ASTRO_ITEM_BORDER_ACTIVE));
    lv_style_set_border_width(&phandle->list_item_focus_style, DIVOOM_ASTRO_ITEM_BORDER_W_ACTIVE);
    lv_style_set_radius(&phandle->list_item_focus_style, DIVOOM_ASTRO_RADIUS_SM);
    lv_style_set_pad_all(&phandle->list_item_focus_style, 0);

    lv_style_init(&phandle->list_item_border_focus_style);
    lv_style_set_border_color(&phandle->list_item_border_focus_style, lv_color_hex(DIVOOM_ASTRO_ITEM_BORDER_ACTIVE));
    lv_style_set_border_width(&phandle->list_item_border_focus_style, DIVOOM_ASTRO_ITEM_BORDER_W_ACTIVE);

    lv_style_init(&phandle->card_item_style);
    lv_style_set_bg_color(&phandle->card_item_style, lv_color_hex(DIVOOM_ASTRO_SURFACE_IDLE));
    lv_style_set_border_color(&phandle->card_item_style, lv_color_hex(DIVOOM_ASTRO_STROKE));
    lv_style_set_border_width(&phandle->card_item_style, 1);
    lv_style_set_radius(&phandle->card_item_style, DIVOOM_ASTRO_RADIUS_SM);
    lv_style_set_pad_all(&phandle->card_item_style, 8);

    lv_style_init(&phandle->card_item_focus_style);
    lv_style_set_bg_color(&phandle->card_item_focus_style, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS));
    lv_style_set_border_color(&phandle->card_item_focus_style, lv_color_hex(DIVOOM_ASTRO_ITEM_BORDER_ACTIVE));
    lv_style_set_border_width(&phandle->card_item_focus_style, DIVOOM_ASTRO_ITEM_BORDER_W_ACTIVE);
    lv_style_set_radius(&phandle->card_item_focus_style, DIVOOM_ASTRO_RADIUS_SM);
    lv_style_set_pad_all(&phandle->card_item_focus_style, 8);

    phandle->styles_initialized = 1;
}

void divoom_astrootoo_ui_base_reset_styles(Pdivoom_astrootoo_ui_base phandle)
{
    if (!phandle || !phandle->styles_initialized)
    {
        return;
    }

    lv_style_reset(&phandle->root_style);
    lv_style_reset(&phandle->content_style);
    lv_style_reset(&phandle->grid_cont_style);
    lv_style_reset(&phandle->list_item_style);
    lv_style_reset(&phandle->list_item_focus_style);
    lv_style_reset(&phandle->list_item_border_focus_style);
    lv_style_reset(&phandle->card_item_style);
    lv_style_reset(&phandle->card_item_focus_style);
    phandle->styles_initialized = 0;
}

lv_obj_t *divoom_astrootoo_ui_create_root(Pdivoom_astrootoo_ui_base phandle)
{
    lv_obj_t *root;

    divoom_astrootoo_ui_base_init_styles(phandle);
    root = lv_obj_create(NULL);
    lv_obj_add_style(root, &phandle->root_style, 0);
    lv_obj_set_size(root, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    return root;
}

lv_obj_t *divoom_astrootoo_ui_create_header(lv_obj_t *parent, Pdivoom_astrootoo_ui_base phandle,
    const char *title_key, int menu_id)
{
    lv_obj_t *header;
    lv_obj_t *label;

    header = lv_obj_create(parent);
    lv_obj_add_style(header, &phandle->content_style, 0);
    lv_obj_set_size(header, DIVOOM_SCREEN_WIDTH, DIVOOM_ASTRO_HEADER_H);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(DIVOOM_ASTRO_HEADER_V1), 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    label = lv_label_create(header);
    lv_label_set_text(label, divoom_menu_control_get_disp_str(title_key));
    lv_obj_set_style_text_color(label, lv_color_hex(DIVOOM_ASTRO_HEADER_INK), 0);
    lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(32, menu_id), 0);
    lv_obj_center(label);
    return header;
}

lv_obj_t *divoom_astrootoo_ui_create_page_header_img(lv_obj_t *parent, Pdivoom_astrootoo_ui_base phandle,
    const char *title_text, const char *bg_img_file, int menu_id)
{
    lv_obj_t *header;
    lv_obj_t *bg_img;
    lv_obj_t *label;

    header = lv_obj_create(parent);
    lv_obj_add_style(header, &phandle->content_style, 0);
    lv_obj_set_size(header, DIVOOM_SCREEN_WIDTH, DIVOOM_ASTRO_HEADER_H);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    if (bg_img_file)
    {
        bg_img = lv_img_create(header);
        lv_obj_set_size(bg_img, DIVOOM_SCREEN_WIDTH, DIVOOM_ASTRO_HEADER_H);
        divoom_menu_control_lv_img_set_src(bg_img, bg_img_file);
    }

    label = lv_label_create(header);
    lv_label_set_text(label, title_text ? title_text : "");
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(32, menu_id), 0);
    lv_obj_center(label);
    return header;
}

lv_obj_t *divoom_astrootoo_ui_create_page_header_tab(lv_obj_t *parent, Pdivoom_astrootoo_ui_base phandle,
    const char *title_key, const char *bg_img_file, int menu_id)
{
    return divoom_astrootoo_ui_create_page_header_img(parent, phandle,
        divoom_menu_control_get_disp_str(title_key), bg_img_file, menu_id);
}

lv_obj_t *divoom_astrootoo_ui_create_subpage_header(lv_obj_t *parent, Pdivoom_astrootoo_ui_base phandle,
    const char *title_key, int menu_id)
{
    return divoom_astrootoo_ui_create_page_header_tab(parent, phandle, title_key,
        DIVOOM_ASTRO_SUBPAGE_HEADER_IMG, menu_id);
}

lv_obj_t *divoom_astrootoo_ui_create_subpage_header_text(lv_obj_t *parent, Pdivoom_astrootoo_ui_base phandle,
    const char *title_text, int menu_id)
{
    return divoom_astrootoo_ui_create_page_header_img(parent, phandle, title_text,
        DIVOOM_ASTRO_SUBPAGE_HEADER_IMG, menu_id);
}

lv_obj_t *divoom_astrootoo_ui_create_scroll_content(lv_obj_t *parent, Pdivoom_astrootoo_ui_base phandle)
{
    lv_obj_t *cont;

    cont = lv_obj_create(parent);
    lv_obj_add_style(cont, &phandle->grid_cont_style, 0);
    lv_obj_set_size(cont, DIVOOM_SCREEN_WIDTH, DIVOOM_ASTRO_CONTENT_H);
    lv_obj_set_pos(cont, 0, DIVOOM_ASTRO_CONTENT_Y);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_AUTO);
    return cont;
}

static void divoom_astrootoo_ui_place_row(lv_obj_t *btn, int row_index, int row_h, int gap)
{
    lv_coord_t y;

    y = gap + row_index * (row_h + gap);
    lv_obj_set_pos(btn, DIVOOM_ASTRO_LIST_PAD_X, y);
}

lv_obj_t *divoom_astrootoo_ui_create_repeat_row(lv_obj_t *parent, Pdivoom_astrootoo_ui_base phandle,
    const char *label_key, int row_index, int row_count, void *direct_handle)
{
    lv_obj_t *btn;
    lv_obj_t *label;

    btn = lv_btn_create(parent);
    lv_obj_add_style(btn, &phandle->list_item_style, 0);
    lv_obj_add_style(btn, &phandle->list_item_focus_style, LV_STATE_FOCUSED);
    lv_obj_add_style(btn, &phandle->list_item_focus_style, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn, &phandle->list_item_border_focus_style, LV_STATE_FOCUSED);
    lv_obj_add_style(btn, &phandle->list_item_border_focus_style, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_size(btn, DIVOOM_ASTRO_REPEAT_ROW_W, DIVOOM_ASTRO_REPEAT_ROW_H);
    divoom_astrootoo_ui_place_row(btn, row_index, DIVOOM_ASTRO_REPEAT_ROW_H, DIVOOM_ASTRO_REPEAT_ROW_GAP);

    label = lv_label_create(btn);
    lv_label_set_text(label, divoom_menu_control_get_disp_str(label_key));
    lv_obj_set_style_text_color(label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 12, 0);
    divoom_astrootoo_ui_list_label_prepare(label,
        divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_REPEAT_ROW_W, 12, 12, 0));
    divoom_astrootoo_ui_list_row_bind_marquee(btn, label);

    if (direct_handle)
    {
        divoom_menu_direct_ctrl_update(direct_handle, btn, 0, row_index);
    }

    return btn;
}

lv_obj_t *divoom_astrootoo_ui_create_card_row(lv_obj_t *parent, Pdivoom_astrootoo_ui_base phandle,
    const char *label_key, int row_index, int row_count, void *direct_handle, int menu_id,
    int bind_left_marquee)
{
    lv_obj_t *btn;
    lv_obj_t *label;
    lv_coord_t x;

    x = (DIVOOM_SCREEN_WIDTH - DIVOOM_ASTRO_CARD_ROW_W) / 2;
    btn = lv_btn_create(parent);
    lv_obj_add_style(btn, &phandle->card_item_style, 0);
    lv_obj_add_style(btn, &phandle->card_item_focus_style, LV_STATE_FOCUSED);
    lv_obj_add_style(btn, &phandle->card_item_focus_style, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_size(btn, DIVOOM_ASTRO_CARD_ROW_W, DIVOOM_ASTRO_CARD_ROW_H);
    lv_obj_set_pos(btn, x, DIVOOM_ASTRO_CARD_ROW_GAP + row_index * (DIVOOM_ASTRO_CARD_ROW_H + DIVOOM_ASTRO_CARD_ROW_GAP));

    label = lv_label_create(btn);
    lv_label_set_text(label, divoom_menu_control_get_disp_str(label_key));
    lv_obj_set_style_text_color(label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(28, menu_id), 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 12, 0);
    divoom_astrootoo_ui_list_label_prepare(label,
        divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_CARD_ROW_W, 12, 12, 0));
    if (bind_left_marquee)
    {
        divoom_astrootoo_ui_list_row_bind_marquee(btn, label);
    }

    if (direct_handle)
    {
        divoom_menu_direct_ctrl_update(direct_handle, btn, 0, row_index);
    }

    return btn;
}

void divoom_astrootoo_ui_apply_sublevel_chevron_size(lv_obj_t *chevron)
{
    if (!chevron)
    {
        return;
    }
    lv_obj_set_size(chevron, DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W, DIVOOM_ASTRO_SUBLEVEL_CHEVRON_H);
}

void divoom_astrootoo_ui_tf_label_apply_focus(lv_obj_t *label, int focused)
{
    const char *txt;
    const lv_font_t *font;
    lv_point_t size;
    lv_coord_t box_w;
    lv_coord_t line_h;
    lv_coord_t letter_space;
    int overflow_x;

    if (!label)
    {
        return;
    }

    font = lv_obj_get_style_text_font(label, 0);
    line_h = font ? (lv_coord_t)lv_font_get_line_height(font) : DIVOOM_ASTRO_TF_LABEL_H;
    if (line_h < DIVOOM_ASTRO_TF_LABEL_H)
    {
        line_h = DIVOOM_ASTRO_TF_LABEL_H;
    }
    /* 高度至少一行，避免 LVGL 在未横向超出时走纵向循环滚动 */
    lv_obj_set_height(label, line_h);
    lv_obj_set_style_max_height(label, line_h, 0);

    overflow_x = 0;
    txt = lv_label_get_text(label);
    if (focused && txt && txt[0] && font)
    {
        lv_obj_update_layout(label);
        box_w = lv_obj_get_content_width(label);
        if (box_w <= 0)
        {
            box_w = lv_obj_get_width(label);
        }
        letter_space = lv_obj_get_style_text_letter_space(label, 0);
        lv_txt_get_size(&size, txt, font, letter_space, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        if (size.x > box_w)
        {
            overflow_x = 1;
        }
    }

    if (overflow_x)
    {
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        if (txt)
        {
            lv_label_set_text(label, txt);
        }
    }
    else
    {
        lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
        txt = lv_label_get_text(label);
        if (txt)
        {
            lv_label_set_text(label, txt);
        }
    }
}

void divoom_astrootoo_ui_tf_label_refresh_long_mode(lv_obj_t *label)
{
    divoom_astrootoo_ui_list_label_refresh_marquee(NULL, label);
}

void divoom_astrootoo_ui_list_label_prepare(lv_obj_t *label, lv_coord_t width)
{
    const lv_font_t *font;
    lv_coord_t line_h;

    if (!label)
    {
        return;
    }
    font = lv_obj_get_style_text_font(label, 0);
    line_h = font ? (lv_coord_t)lv_font_get_line_height(font) : DIVOOM_ASTRO_TF_LABEL_H;
    if (line_h < DIVOOM_ASTRO_TF_LABEL_H)
    {
        line_h = DIVOOM_ASTRO_TF_LABEL_H;
    }
    if (width > 0)
    {
        lv_obj_set_width(label, width);
    }
    lv_obj_set_height(label, line_h);
    lv_obj_set_style_max_height(label, line_h, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
}

static void divoom_astrootoo_ui_list_row_marquee_focus_cb(lv_event_t *e)
{
    lv_event_code_t code;
    lv_obj_t *label;

    code = lv_event_get_code(e);
    if (code != LV_EVENT_FOCUSED && code != LV_EVENT_DEFOCUSED)
    {
        return;
    }
    label = (lv_obj_t *)lv_event_get_user_data(e);
    divoom_astrootoo_ui_tf_label_apply_focus(label, code == LV_EVENT_FOCUSED);
}

void divoom_astrootoo_ui_list_row_bind_marquee(lv_obj_t *row, lv_obj_t *label)
{
    if (!row || !label)
    {
        return;
    }
    lv_obj_add_event_cb(row, divoom_astrootoo_ui_list_row_marquee_focus_cb, LV_EVENT_FOCUSED, label);
    lv_obj_add_event_cb(row, divoom_astrootoo_ui_list_row_marquee_focus_cb, LV_EVENT_DEFOCUSED, label);
}

void divoom_astrootoo_ui_list_label_refresh_marquee(lv_obj_t *row, lv_obj_t *label)
{
    int focused;

    if (!label)
    {
        return;
    }
    if (!row)
    {
        row = lv_obj_get_parent(label);
    }
    focused = (row && (lv_obj_has_state(row, LV_STATE_FOCUSED)
        || lv_obj_has_state(row, LV_STATE_FOCUS_KEY))) ? 1 : 0;
    divoom_astrootoo_ui_tf_label_apply_focus(label, focused);
}

void divoom_astrootoo_ui_list_label_marquee_always(lv_obj_t *label)
{
    if (!label)
    {
        return;
    }
    divoom_astrootoo_ui_tf_label_apply_focus(label, 1);
}

lv_coord_t divoom_astrootoo_ui_list_label_width(lv_coord_t row_w,
    lv_coord_t inset_l, lv_coord_t inset_r, lv_coord_t trail_w)
{
    lv_coord_t w;

    w = row_w - inset_l - inset_r - trail_w - DIVOOM_ASTRO_LIST_LABEL_GAP;
    return (w > 0) ? w : 0;
}

lv_coord_t divoom_astrootoo_ui_list_kv_value_width(lv_coord_t row_w,
    lv_coord_t inset_l, lv_coord_t inset_r, lv_coord_t key_w,
    lv_coord_t key_val_gap, lv_coord_t trail_w)
{
    lv_coord_t w;

    w = row_w - inset_l - inset_r - key_w - key_val_gap - trail_w - DIVOOM_ASTRO_LIST_LABEL_GAP;
    return (w > 0) ? w : 0;
}

lv_coord_t divoom_astrootoo_ui_list_kv_key_width(lv_coord_t row_w,
    lv_coord_t inset_l, lv_coord_t inset_r, lv_coord_t val_w,
    lv_coord_t key_val_gap, lv_coord_t trail_w)
{
    lv_coord_t w;

    w = row_w - inset_l - inset_r - val_w - key_val_gap - trail_w - DIVOOM_ASTRO_LIST_LABEL_GAP;
    return (w > 0) ? w : 0;
}

void divoom_astrootoo_ui_list_kv_bind_row(lv_obj_t *key_lbl, lv_obj_t *val_lbl,
    lv_coord_t row_w, lv_coord_t inset_l, lv_coord_t inset_r,
    lv_coord_t trail_w, lv_coord_t val_align_ofs, lv_coord_t val_min_w)
{
    lv_coord_t key_nat;
    lv_coord_t key_max;
    lv_coord_t val_w;

    if (!key_lbl || !val_lbl)
    {
        return;
    }

    lv_obj_set_width(key_lbl, LV_SIZE_CONTENT);
    lv_obj_update_layout(key_lbl);
    key_nat = lv_obj_get_width(key_lbl);

    val_w = divoom_astrootoo_ui_list_kv_value_width(row_w, inset_l, inset_r,
        key_nat, DIVOOM_ASTRO_LIST_KV_GAP, trail_w);
    if (val_min_w > 0 && val_w < val_min_w)
    {
        val_w = val_min_w;
    }

    key_max = divoom_astrootoo_ui_list_kv_key_width(row_w, inset_l, inset_r,
        val_w, DIVOOM_ASTRO_LIST_KV_GAP, trail_w);
    if (key_max <= 0)
    {
        key_max = key_nat;
    }

    divoom_astrootoo_ui_list_label_prepare(key_lbl, key_max);
    divoom_astrootoo_ui_list_label_prepare(val_lbl, val_w);
    lv_obj_set_style_text_align(val_lbl, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(val_lbl, LV_ALIGN_RIGHT_MID, val_align_ofs, 0);
}

static lv_obj_t *divoom_astrootoo_ui_tf_row_find_label(lv_obj_t *row)
{
    uint32_t i;
    uint32_t cnt;
    lv_obj_t *child;

    if (!row)
    {
        return NULL;
    }
    cnt = lv_obj_get_child_cnt(row);
    for (i = 0; i < cnt; i++)
    {
        child = lv_obj_get_child(row, (int32_t)i);
        if (child && lv_obj_check_type(child, &lv_label_class))
        {
            return child;
        }
    }
    return NULL;
}

static void divoom_astrootoo_ui_tf_row_focus_cb(lv_event_t *e)
{
    lv_event_code_t code;
    lv_obj_t *row;

    code = lv_event_get_code(e);
    if (code != LV_EVENT_FOCUSED && code != LV_EVENT_DEFOCUSED)
    {
        return;
    }
    row = lv_event_get_target(e);
    divoom_astrootoo_ui_tf_label_apply_focus(divoom_astrootoo_ui_tf_row_find_label(row),
        code == LV_EVENT_FOCUSED);
}

lv_obj_t *divoom_astrootoo_ui_create_tf_list_row(lv_obj_t *parent, Pdivoom_astrootoo_ui_base phandle,
    int row_kind, int row_index, lv_coord_t row_h, lv_coord_t row_gap, int menu_id,
    lv_obj_t **out_icon, lv_obj_t **out_label, lv_obj_t **out_chevron, lv_obj_t **out_check)
{
    lv_obj_t *btn;
    lv_obj_t *icon;
    lv_obj_t *label;
    lv_obj_t *chevron;
    lv_obj_t *mark;
    const char *icon_path;

    btn = lv_btn_create(parent);
    lv_obj_add_style(btn, &phandle->list_item_style, 0);
    lv_obj_add_style(btn, &phandle->list_item_focus_style, LV_STATE_FOCUSED);
    lv_obj_add_style(btn, &phandle->list_item_focus_style, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn, &phandle->list_item_border_focus_style, LV_STATE_FOCUSED);
    lv_obj_add_style(btn, &phandle->list_item_border_focus_style, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_size(btn, DIVOOM_ASTRO_REPEAT_ROW_W, row_h);
    lv_obj_set_pos(btn, DIVOOM_ASTRO_LIST_PAD_X, row_gap + row_index * (row_h + DIVOOM_ASTRO_REPEAT_ROW_GAP));
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    if (row_kind == DIVOOM_ASTROTOO_TF_ROW_FOLDER)
    {
        icon_path = "23/at_tf_folder.png";
    }
    else
    {
        icon_path = "23/at_tf_music_note.png";
    }

    icon = lv_img_create(btn);
    lv_obj_set_size(icon, DIVOOM_ASTRO_TF_ICON_W, DIVOOM_ASTRO_TF_ICON_H);
    divoom_menu_control_lv_img_set_src(icon, icon_path);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, 20, 0);

    label = lv_label_create(btn);
    lv_obj_set_style_text_color(label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(24, menu_id), 0);
    lv_obj_set_size(label, DIVOOM_ASTRO_REPEAT_ROW_W - 120, DIVOOM_ASTRO_TF_LABEL_H);
    lv_obj_set_style_max_height(label, DIVOOM_ASTRO_TF_LABEL_H, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 68, 0);
    lv_obj_add_event_cb(btn, divoom_astrootoo_ui_tf_row_focus_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(btn, divoom_astrootoo_ui_tf_row_focus_cb, LV_EVENT_DEFOCUSED, NULL);

    chevron = lv_img_create(btn);
    divoom_astrootoo_ui_apply_sublevel_chevron_size(chevron);
    divoom_menu_control_lv_img_set_src(chevron, "23/at_sublevel_chevron.png");
    lv_obj_align(chevron, LV_ALIGN_RIGHT_MID, -20, 0);
    if (row_kind != DIVOOM_ASTROTOO_TF_ROW_FOLDER)
    {
        lv_obj_add_flag(chevron, LV_OBJ_FLAG_HIDDEN);
    }

    mark = lv_img_create(btn);
    lv_obj_set_size(mark, 24, 24);
    divoom_menu_control_lv_img_set_src(mark, "23/at_repeat_check.png");
    lv_obj_align(mark, LV_ALIGN_RIGHT_MID, -20, 0);
    lv_obj_add_flag(mark, LV_OBJ_FLAG_HIDDEN);

    if (out_icon)
    {
        *out_icon = icon;
    }
    if (out_label)
    {
        *out_label = label;
    }
    if (out_chevron)
    {
        *out_chevron = chevron;
    }
    if (out_check)
    {
        *out_check = mark;
    }

    return btn;
}

void divoom_astrootoo_ui_focus_scroll(lv_obj_t *obj)
{
    if (obj)
    {
        lv_obj_scroll_to_view(obj, LV_ANIM_ON);
    }
}

static lv_obj_t *divoom_astrootoo_ui_scrollable_parent(lv_obj_t *obj)
{
    lv_obj_t *scrl;

    scrl = obj ? lv_obj_get_parent(obj) : NULL;
    while (scrl && !lv_obj_has_flag(scrl, LV_OBJ_FLAG_SCROLLABLE))
    {
        scrl = lv_obj_get_parent(scrl);
    }
    return scrl;
}

static lv_obj_t *divoom_astrootoo_ui_first_btn_in(lv_obj_t *obj)
{
    uint32_t i;
    uint32_t n;
    lv_obj_t *found;

    if (!obj)
    {
        return NULL;
    }
    if (lv_obj_check_type(obj, &lv_btn_class))
    {
        return obj;
    }
    n = lv_obj_get_child_cnt(obj);
    for (i = 0; i < n; i++)
    {
        found = divoom_astrootoo_ui_first_btn_in(lv_obj_get_child(obj, (int32_t)i));
        if (found)
        {
            return found;
        }
    }
    return NULL;
}

void divoom_astrootoo_ui_focus_scroll_keep_section_title(lv_obj_t *row)
{
    lv_obj_t *parent;
    lv_obj_t *scrl;
    lv_obj_t *title;
    lv_obj_t *ch;
    uint32_t i;
    uint32_t n;

    if (!row)
    {
        return;
    }
    parent = lv_obj_get_parent(row);
    scrl = divoom_astrootoo_ui_scrollable_parent(row);
    if (!parent || !scrl)
    {
        divoom_astrootoo_ui_focus_scroll(row);
        return;
    }

    title = NULL;
    n = lv_obj_get_child_cnt(parent);
    for (i = 0; i < n; i++)
    {
        ch = lv_obj_get_child(parent, (int32_t)i);
        if (!ch)
        {
            continue;
        }
        if (ch == row)
        {
            break;
        }
        if (lv_obj_check_type(ch, &lv_btn_class))
        {
            title = NULL;
        }
        else
        {
            title = ch;
        }
    }
    if (!title)
    {
        divoom_astrootoo_ui_focus_scroll(row);
        return;
    }
    if (divoom_astrootoo_ui_first_btn_in(scrl) == row)
    {
        lv_obj_scroll_to_y(scrl, 0, LV_ANIM_ON);
        return;
    }
    if (parent == scrl)
    {
        lv_obj_scroll_to_y(scrl, lv_obj_get_y(title), LV_ANIM_ON);
        return;
    }
    lv_obj_scroll_to_y(scrl, lv_obj_get_y(parent), LV_ANIM_ON);
}

uint32_t divoom_astrootoo_ui_key_raw_from_event(lv_event_t *e)
{
    uint32_t *para;

    if (!e || lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return 0;
    }

    para = (uint32_t *)lv_event_get_param(e);
    if (!para)
    {
        return 0;
    }

    return *para;
}

uint8_t divoom_astrootoo_ui_key_code_from_event(lv_event_t *e)
{
    return (uint8_t)(divoom_astrootoo_ui_key_raw_from_event(e) & 0xff);
}

uint8_t divoom_astrootoo_ui_key_press_from_event(lv_event_t *e)
{
    return (uint8_t)((divoom_astrootoo_ui_key_raw_from_event(e) >> 8) & 0xff);
}

int divoom_astrootoo_ui_key_is_short_press(lv_event_t *e)
{
    uint8_t press;

    press = divoom_astrootoo_ui_key_press_from_event(e);
    return (press == DIVOOM_PRESS_SHORT || press == DIVOOM_NO_PRESS);
}

int divoom_astrootoo_ui_key_is_nav_key(uint8_t key_code)
{
    return divoom_astrootoo_ui_key_is_up(key_code)
        || divoom_astrootoo_ui_key_is_down(key_code)
        || divoom_astrootoo_ui_key_is_left(key_code)
        || divoom_astrootoo_ui_key_is_right(key_code);
}

int divoom_astrootoo_ui_key_is_nav_press(lv_event_t *e)
{
    uint8_t press;

    if (!e || lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return 0;
    }

    press = divoom_astrootoo_ui_key_press_from_event(e);
    switch (press)
    {
    case DIVOOM_PRESS_SHORT:
    case DIVOOM_PRESS_LONG:
    case DIVOOM_PRESSING:
    case DIVOOM_NO_PRESS:
        return 1;
    default:
        return 0;
    }
}

int divoom_astrootoo_ui_key_is_menu_press(lv_event_t *e, uint8_t key_code)
{
    if (divoom_astrootoo_ui_key_is_nav_key(key_code)
        || divoom_astrootoo_ui_key_is_prev(key_code)
        || divoom_astrootoo_ui_key_is_next(key_code))
    {
        return divoom_astrootoo_ui_key_is_nav_press(e);
    }

    return divoom_astrootoo_ui_key_is_short_press(e);
}

static int divoom_astrootoo_ui_key_to_direct(uint8_t key_code)
{
    if (divoom_astrootoo_ui_key_is_up(key_code))
    {
        return DIVOOM_MENU_DIRECT_UP;
    }
    if (divoom_astrootoo_ui_key_is_down(key_code))
    {
        return DIVOOM_MENU_DIRECT_DOWN;
    }
    if (divoom_astrootoo_ui_key_is_left(key_code))
    {
        return DIVOOM_MENU_DIRECT_LEFT;
    }
    if (divoom_astrootoo_ui_key_is_right(key_code))
    {
        return DIVOOM_MENU_DIRECT_RIGHT;
    }

    return -1;
}

int divoom_astrootoo_ui_direct_nav_step(void *direct_handle, lv_obj_t *target,
    uint8_t key_code, lv_obj_t **out_next)
{
    lv_obj_t *next;
    int direct;

    if (!direct_handle || !target)
    {
        return 0;
    }

    direct = divoom_astrootoo_ui_key_to_direct(key_code);
    if (direct < 0)
    {
        return 0;
    }

    next = (lv_obj_t *)divoom_menu_direct_ctrl_get(direct_handle, target, direct);
    if (!next || next == target)
    {
        return 0;
    }

    lv_group_focus_obj(next);
    divoom_astrootoo_ui_focus_scroll(next);
    if (out_next)
    {
        *out_next = next;
    }
    return 1;
}

int divoom_astrootoo_ui_key_is_prev(uint8_t key_code)
{
    if (key_code == DIVOOM_LVGL_KEY_PRE || key_code == DIVOOM_LVGL_KEY_LEFT
        || key_code == DIVOOM_KEY_LIGHT_LEFT || key_code == LV_KEY_LEFT || key_code == LV_KEY_PREV)
    {
        return 1;
    }

    return 0;
}

int divoom_astrootoo_ui_key_is_next(uint8_t key_code)
{
    if (key_code == DIVOOM_LVGL_KEY_NEXT || key_code == DIVOOM_LVGL_KEY_RIGHT
        || key_code == DIVOOM_KEY_LIGHT_RIGHT || key_code == LV_KEY_RIGHT || key_code == LV_KEY_NEXT)
    {
        return 1;
    }

    return 0;
}

int divoom_astrootoo_ui_key_is_enter(uint8_t key_code)
{
    if (key_code == DIVOOM_LVGL_KEY_ENTER || key_code == DIVOOM_KEY_PLAY
        || key_code == DIVOOM_KEY_LIGHT_ENTER || key_code == LV_KEY_ENTER)
    {
        return 1;
    }

    return 0;
}

int divoom_astrootoo_ui_key_is_home(uint8_t key_code)
{
    return (key_code == DIVOOM_LVGL_KEY_HOME || key_code == DIVOOM_KEY_MENU
        || key_code == LV_KEY_HOME) ? 1 : 0;
}

int divoom_astrootoo_ui_key_is_esc(uint8_t key_code)
{
    if (key_code == DIVOOM_LVGL_KEY_ESC || key_code == LV_KEY_ESC)
    {
        return 1;
    }

    return 0;
}

/* 二维导航：上 = PREV（PageUp/Up），与 is_prev 的 LIGHT_LEFT 区分开 */
int divoom_astrootoo_ui_key_is_up(uint8_t key_code)
{
    if (key_code == DIVOOM_LVGL_KEY_UP || key_code == DIVOOM_LVGL_KEY_PRE
        || key_code == LV_KEY_UP || key_code == LV_KEY_PREV)
    {
        return 1;
    }

    return 0;
}

/* 二维导航：下 = NEXT（PageDown/Down），与 is_next 的 LIGHT_RIGHT 区分开 */
int divoom_astrootoo_ui_key_is_down(uint8_t key_code)
{
    if (key_code == DIVOOM_LVGL_KEY_DOWN || key_code == DIVOOM_LVGL_KEY_NEXT
        || key_code == LV_KEY_DOWN || key_code == LV_KEY_NEXT)
    {
        return 1;
    }

    return 0;
}

/* 二维导航：左 = 实体方向左（不含 VOL- / LIGHT_LEFT） */
int divoom_astrootoo_ui_key_is_left(uint8_t key_code)
{
    if (key_code == DIVOOM_LVGL_KEY_LEFT || key_code == LV_KEY_LEFT)
    {
        return 1;
    }

    return 0;
}

/* 二维导航：右 = 实体方向右（不含 VOL+ / LIGHT_RIGHT） */
int divoom_astrootoo_ui_key_is_right(uint8_t key_code)
{
    if (key_code == DIVOOM_LVGL_KEY_RIGHT || key_code == LV_KEY_RIGHT)
    {
        return 1;
    }

    return 0;
}

lv_obj_t *divoom_astrootoo_ui_create_key_capture_btn(lv_obj_t *parent, Pdivoom_astrootoo_ui_base phandle,
    lv_event_cb_t key_cb, void *user_data)
{
    lv_obj_t *btn;

    btn = lv_btn_create(parent);
    lv_obj_add_style(btn, &phandle->root_style, 0);
    lv_obj_set_size(btn, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_pos(btn, 0, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    if (key_cb)
    {
        lv_obj_add_event_cb(btn, key_cb, LV_EVENT_KEY, user_data);
    }

    return btn;
}

lv_obj_t *divoom_astrootoo_ui_create_status_bar(lv_obj_t *parent, Pdivoom_astrootoo_ui_base phandle,
    lv_obj_t **out_time_label)
{
    lv_obj_t *bar;
    lv_obj_t *time_label;
    lv_obj_t *bt_label;
    lv_obj_t *right_label;

    bar = lv_obj_create(parent);
    lv_obj_add_style(bar, &phandle->content_style, 0);
    lv_obj_set_size(bar, DIVOOM_SCREEN_WIDTH, DIVOOM_ASTRO_STATUS_BAR_H);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    /* 左：蓝牙 */
    bt_label = lv_label_create(bar);
    lv_label_set_text(bt_label, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_color(bt_label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_align(bt_label, LV_ALIGN_LEFT_MID, 16, 0);

    /* 中：时间 */
    time_label = lv_label_create(bar);
    lv_label_set_text(time_label, "--:--");
    lv_obj_set_style_text_color(time_label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_center(time_label);

    /* 右：WiFi + 电量 */
    right_label = lv_label_create(bar);
    lv_label_set_text(right_label, LV_SYMBOL_WIFI "  " LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(right_label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_align(right_label, LV_ALIGN_RIGHT_MID, -16, 0);

    if (out_time_label)
    {
        *out_time_label = time_label;
    }

    return bar;
}

lv_obj_t *divoom_astrootoo_ui_create_main_menu_header(lv_obj_t *parent, Pdivoom_astrootoo_ui_base phandle,
    int menu_id, lv_obj_t **out_time_label, lv_obj_t **out_source_img, lv_obj_t **out_wifi_img,
    lv_obj_t **out_battery_img)
{
    lv_obj_t *bar;
    lv_obj_t *time_label;
    lv_obj_t *source_img;
    lv_obj_t *wifi_img;
    lv_obj_t *battery_img;

    bar = lv_obj_create(parent);
    lv_obj_add_style(bar, &phandle->content_style, 0);
    lv_obj_set_size(bar, DIVOOM_SCREEN_WIDTH, DIVOOM_ASTRO_MAIN_MENU_HEADER_H);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    source_img = lv_img_create(bar);
    lv_obj_set_size(source_img,
        DIVOOM_ASTRO_MAIN_MENU_HEADER_SOURCE_W, DIVOOM_ASTRO_MAIN_MENU_HEADER_SOURCE_H);
    lv_obj_align(source_img, LV_ALIGN_LEFT_MID, 16, 0);

    time_label = lv_label_create(bar);
    lv_label_set_text(time_label, "--:--");
    lv_obj_set_style_text_color(time_label, lv_color_hex(DIVOOM_ASTRO_MAIN_MENU_HEADER_FG), 0);
    lv_obj_set_style_text_font(time_label,
        divoom_menu_control_get_font_info(DIVOOM_ASTRO_MAIN_MENU_HEADER_TIME_FONT_PX, menu_id), 0);
    lv_obj_center(time_label);

    /* 电量外壳改为代码绘制，占位仍是 25×14，与原 PNG 同尺寸。 */
    battery_img = lv_obj_create(bar);
    lv_obj_remove_style_all(battery_img);
    lv_obj_set_size(battery_img,
        DIVOOM_ASTRO_MAIN_MENU_HEADER_BATTERY_W, DIVOOM_ASTRO_MAIN_MENU_HEADER_BATTERY_H);
    lv_obj_set_style_bg_opa(battery_img, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(battery_img, 0, 0);
    lv_obj_set_style_pad_all(battery_img, 0, 0);
    lv_obj_clear_flag(battery_img, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(battery_img, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_align(battery_img, LV_ALIGN_RIGHT_MID, -16, 0);

    wifi_img = lv_img_create(bar);
    lv_obj_set_size(wifi_img,
        DIVOOM_ASTRO_MAIN_MENU_HEADER_SOURCE_W, DIVOOM_ASTRO_MAIN_MENU_HEADER_SOURCE_H);
    lv_obj_align(wifi_img, LV_ALIGN_RIGHT_MID,
        -(16 + DIVOOM_ASTRO_MAIN_MENU_HEADER_BATTERY_W + DIVOOM_ASTRO_MAIN_MENU_HEADER_STATUS_GAP), 0);

    if (out_time_label)
    {
        *out_time_label = time_label;
    }
    if (out_source_img)
    {
        *out_source_img = source_img;
    }
    if (out_wifi_img)
    {
        *out_wifi_img = wifi_img;
    }
    if (out_battery_img)
    {
        *out_battery_img = battery_img;
    }

    return bar;
}

lv_obj_t *divoom_astrootoo_ui_confirm_modal_create(lv_obj_t *parent, lv_obj_t **out_frame)
{
    lv_obj_t *overlay;
    lv_obj_t *shell;
    lv_obj_t *frame;

    if (!parent)
    {
        return NULL;
    }

    overlay = lv_obj_create(parent);
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(overlay, 0, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

    shell = lv_obj_create(overlay);
    lv_obj_remove_style_all(shell);
    lv_obj_set_width(shell, DIVOOM_ASTRO_CONFIRM_MODAL_SHELL_W);
    lv_obj_set_height(shell, LV_SIZE_CONTENT);
    lv_obj_center(shell);
    lv_obj_set_style_bg_color(shell, lv_color_hex(DIVOOM_ASTRO_MODAL_SHELL_BG), 0);
    lv_obj_set_style_bg_opa(shell, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(shell, 2, 0);
    lv_obj_set_style_border_color(shell, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
    lv_obj_set_style_radius(shell, DIVOOM_ASTRO_RADIUS_MD, 0);
    lv_obj_set_style_pad_all(shell, 2, 0);
    lv_obj_set_style_clip_corner(shell, true, 0);
    lv_obj_clear_flag(shell, LV_OBJ_FLAG_SCROLLABLE);

    frame = lv_obj_create(shell);
    lv_obj_remove_style_all(frame);
    lv_obj_set_width(frame, LV_PCT(100));
    lv_obj_set_height(frame, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(frame, lv_color_hex(DIVOOM_ASTRO_MODAL_BG), 0);
    lv_obj_set_style_bg_opa(frame, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(frame, DIVOOM_ASTRO_RADIUS_MD - 2, 0);
    lv_obj_set_style_clip_corner(frame, true, 0);
    lv_obj_set_style_pad_top(frame, 18, 0);
    lv_obj_set_style_pad_bottom(frame, 14, 0);
    lv_obj_set_style_pad_left(frame, 16, 0);
    lv_obj_set_style_pad_right(frame, 16, 0);
    lv_obj_set_style_pad_row(frame, 14, 0);
    lv_obj_clear_flag(frame, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(frame, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(frame, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_START);

    lv_obj_move_foreground(overlay);
    if (out_frame)
    {
        *out_frame = frame;
    }
    return overlay;
}

lv_obj_t *divoom_astrootoo_ui_confirm_modal_create_full_bleed(lv_obj_t *parent, lv_obj_t **out_frame)
{
    lv_obj_t *overlay;
    lv_obj_t *shell;
    lv_obj_t *frame;

    if (!parent)
    {
        return NULL;
    }

    overlay = lv_obj_create(parent);
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(DIVOOM_ASTRO_BG), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(overlay, 0, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

    shell = lv_obj_create(overlay);
    lv_obj_remove_style_all(shell);
    lv_obj_set_size(shell, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_pos(shell, 0, 0);
    lv_obj_set_style_bg_color(shell, lv_color_hex(DIVOOM_ASTRO_BG), 0);
    lv_obj_set_style_bg_opa(shell, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(shell, 0, 0);
    lv_obj_set_style_radius(shell, 0, 0);
    lv_obj_set_style_pad_all(shell, 0, 0);
    lv_obj_clear_flag(shell, LV_OBJ_FLAG_SCROLLABLE);

    frame = lv_obj_create(shell);
    lv_obj_remove_style_all(frame);
    lv_obj_set_size(frame, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_pos(frame, 0, 0);
    lv_obj_set_style_bg_color(frame, lv_color_hex(DIVOOM_ASTRO_BG), 0);
    lv_obj_set_style_bg_opa(frame, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(frame, 0, 0);
    lv_obj_set_style_pad_all(frame, 0, 0);
    lv_obj_clear_flag(frame, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_move_foreground(overlay);
    if (out_frame)
    {
        *out_frame = frame;
    }
    return overlay;
}

void divoom_astrootoo_ui_confirm_modal_layout(lv_obj_t *frame)
{
    lv_obj_t *shell;

    if (!frame)
    {
        return;
    }
    shell = lv_obj_get_parent(frame);
    lv_obj_update_layout(frame);
    lv_obj_set_height(frame, LV_SIZE_CONTENT);
    lv_obj_update_layout(frame);
    if (shell)
    {
        lv_obj_set_height(shell, LV_SIZE_CONTENT);
        lv_obj_update_layout(shell);
    }
}

static void divoom_astrootoo_ui_confirm_modal_btn_apply_label_focus(lv_obj_t *btn, int focused)
{
    lv_obj_t *lbl;
    const char *txt;

    if (!btn)
    {
        return;
    }
    lbl = lv_obj_get_child(btn, 0);
    if (!lbl)
    {
        return;
    }
    txt = lv_label_get_text(lbl);
    if (focused)
    {
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    }
    else
    {
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
        if (txt)
        {
            lv_label_set_text(lbl, txt);
        }
    }
}

static void divoom_astrootoo_ui_confirm_modal_btn_focus_cb(lv_event_t *e)
{
    lv_event_code_t code;
    lv_obj_t *btn;

    code = lv_event_get_code(e);
    if (code != LV_EVENT_FOCUSED && code != LV_EVENT_DEFOCUSED)
    {
        return;
    }
    btn = lv_event_get_target(e);
    divoom_astrootoo_ui_confirm_modal_btn_apply_label_focus(btn, code == LV_EVENT_FOCUSED);
}

void divoom_astrootoo_ui_confirm_modal_sync_btn_labels(lv_obj_t *cancel_btn, lv_obj_t *ok_btn,
    int ok_focus)
{
    divoom_astrootoo_ui_confirm_modal_btn_apply_label_focus(cancel_btn, ok_focus == 0);
    divoom_astrootoo_ui_confirm_modal_btn_apply_label_focus(ok_btn, ok_focus != 0);
}

lv_obj_t *divoom_astrootoo_ui_confirm_modal_make_btn(lv_obj_t *parent, const char *label,
    int menu_id, int font_size, lv_event_cb_t key_cb, void *user_data, int add_to_group)
{
    lv_obj_t *btn;
    lv_obj_t *lbl;
    int label_font;

    if (!parent || !label)
    {
        return NULL;
    }
    label_font = (font_size > 0) ? font_size : DIVOOM_ASTRO_CONFIRM_MODAL_BTN_FONT;

    btn = lv_btn_create(parent);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_set_height(btn, DIVOOM_ASTRO_CONFIRM_MODAL_BTN_MIN_H);
    lv_obj_set_style_radius(btn, 16, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_pad_top(btn, DIVOOM_ASTRO_CONFIRM_MODAL_BTN_PAD_Y, 0);
    lv_obj_set_style_pad_bottom(btn, DIVOOM_ASTRO_CONFIRM_MODAL_BTN_PAD_Y, 0);
    lv_obj_set_style_pad_left(btn, DIVOOM_ASTRO_CONFIRM_MODAL_BTN_PAD_X, 0);
    lv_obj_set_style_pad_right(btn, DIVOOM_ASTRO_CONFIRM_MODAL_BTN_PAD_X, 0);
    lv_obj_set_style_clip_corner(btn, true, 0);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    lbl = lv_label_create(btn);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
    lv_label_set_text(lbl, label);
    lv_obj_set_width(lbl, LV_PCT(100));
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(label_font, menu_id), 0);
    lv_obj_center(lbl);

    lv_obj_add_event_cb(btn, divoom_astrootoo_ui_confirm_modal_btn_focus_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(btn, divoom_astrootoo_ui_confirm_modal_btn_focus_cb, LV_EVENT_DEFOCUSED, NULL);
    if (key_cb)
    {
        lv_obj_add_event_cb(btn, key_cb, LV_EVENT_KEY, user_data);
    }
    if (add_to_group)
    {
        lv_group_add_obj(lv_group_get_default(), btn);
    }
    return btn;
}

lv_obj_t *divoom_astrootoo_ui_alert_modal_create(lv_obj_t *parent, lv_obj_t **out_frame)
{
    return divoom_astrootoo_ui_confirm_modal_create(parent, out_frame);
}

lv_obj_t *divoom_astrootoo_ui_alert_modal_add_title(lv_obj_t *frame, const char *text, int menu_id)
{
    lv_obj_t *title;

    if (!frame || !text)
    {
        return NULL;
    }

    title = lv_label_create(frame);
    lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP);
    lv_label_set_text(title, text);
    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(30, menu_id), 0);
    lv_obj_set_style_pad_bottom(title, 10, 0);
    lv_obj_set_style_border_side(title, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(title, 2, 0);
    lv_obj_set_style_border_color(title, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
    return title;
}

lv_obj_t *divoom_astrootoo_ui_alert_modal_add_body(lv_obj_t *frame, const char *text, int menu_id)
{
    lv_obj_t *body;

    if (!frame || !text)
    {
        return NULL;
    }

    body = lv_label_create(frame);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_label_set_text(body, text);
    lv_obj_set_width(body, LV_PCT(100));
    lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(body, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(body, divoom_menu_control_get_font_info(24, menu_id), 0);
    return body;
}

lv_obj_t *divoom_astrootoo_ui_alert_modal_make_ok_btn(lv_obj_t *parent, const char *label,
    int menu_id, lv_event_cb_t key_cb, void *user_data)
{
    lv_obj_t *btn;
    lv_obj_t *lbl;

    if (!parent || !label)
    {
        return NULL;
    }

    btn = lv_btn_create(parent);
    lv_obj_set_width(btn, DIVOOM_ASTRO_ALERT_MODAL_BTN_SINGLE_W);
    lv_obj_set_height(btn, DIVOOM_ASTRO_CONFIRM_MODAL_BTN_MIN_H);
    lv_obj_set_style_radius(btn, 16, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_top(btn, DIVOOM_ASTRO_CONFIRM_MODAL_BTN_PAD_Y, 0);
    lv_obj_set_style_pad_bottom(btn, DIVOOM_ASTRO_CONFIRM_MODAL_BTN_PAD_Y, 0);
    lv_obj_set_style_pad_left(btn, DIVOOM_ASTRO_CONFIRM_MODAL_BTN_PAD_X, 0);
    lv_obj_set_style_pad_right(btn, DIVOOM_ASTRO_CONFIRM_MODAL_BTN_PAD_X, 0);
    lv_obj_set_style_clip_corner(btn, true, 0);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    lbl = lv_label_create(btn);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
    lv_label_set_text(lbl, label);
    lv_obj_set_width(lbl, LV_PCT(100));
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(lbl,
        divoom_menu_control_get_font_info(DIVOOM_ASTRO_CONFIRM_MODAL_BTN_FONT, menu_id), 0);
    lv_obj_center(lbl);

    lv_obj_add_event_cb(btn, divoom_astrootoo_ui_confirm_modal_btn_focus_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(btn, divoom_astrootoo_ui_confirm_modal_btn_focus_cb, LV_EVENT_DEFOCUSED, NULL);
    if (key_cb)
    {
        lv_obj_add_event_cb(btn, key_cb, LV_EVENT_KEY, user_data);
    }
    lv_group_add_obj(lv_group_get_default(), btn);
    lv_group_focus_obj(btn);
    divoom_astrootoo_ui_confirm_modal_btn_apply_label_focus(btn, 1);
    return btn;
}

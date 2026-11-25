#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_menu_direct_ctrl.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_astrootoo_theme.h"
#include "divoom_astrootoo_settings_slideshow.h"
#include "divoom_astrootoo_settings_internal.h"
#include "divoom_pic_phone_manage.h"
#include "divoom_pic_album_manage.h"

#define DIVOOM_ASTRO_SLIDE_UI_DIR           "26/"
#define DIVOOM_ASTRO_SLIDE_HEADER           DIVOOM_ASTRO_SLIDE_UI_DIR "at_settings_header.png"
#define DIVOOM_ASTRO_SLIDE_SWITCH_ON        DIVOOM_ASTRO_SLIDE_UI_DIR "at_settings_switch_on.png"
#define DIVOOM_ASTRO_SLIDE_SWITCH_OFF       DIVOOM_ASTRO_SLIDE_UI_DIR "at_settings_switch_off.png"
#define DIVOOM_ASTRO_SLIDE_CHEVRON          DIVOOM_ASTRO_SLIDE_UI_DIR "at_settings_chevron.png"
#define DIVOOM_ASTRO_SLIDE_SPEED_SLOW       DIVOOM_ASTRO_SLIDE_UI_DIR "at_slideshow_speed_slow.png"
#define DIVOOM_ASTRO_SLIDE_SPEED_FAST       DIVOOM_ASTRO_SLIDE_UI_DIR "at_slideshow_speed_fast.png"

#define DIVOOM_ASTRO_SLIDE_ROW_W            444
#define DIVOOM_ASTRO_SLIDE_ROW_H            72
#define DIVOOM_ASTRO_SLIDE_ROW_GAP          8
#define DIVOOM_ASTRO_SLIDE_ROW_X            ((DIVOOM_SCREEN_WIDTH - DIVOOM_ASTRO_SLIDE_ROW_W) / 2)
#define DIVOOM_ASTRO_SLIDE_TOP_GAP          8
#define DIVOOM_ASTRO_SLIDE_SEC_H            40
#define DIVOOM_ASTRO_SLIDE_CARD_PAD_H       8
#define DIVOOM_ASTRO_SLIDE_ENDPOINT_SZ      32
#define DIVOOM_ASTRO_SLIDE_ENDPOINT_GAP     8
/* 内容区 = 行宽 - 卡片左右 pad；再减两端图标与间距 */
#define DIVOOM_ASTRO_SLIDE_SLIDER_W (DIVOOM_ASTRO_SLIDE_ROW_W - DIVOOM_ASTRO_SLIDE_CARD_PAD_H * 4 \
    - DIVOOM_ASTRO_SLIDE_ENDPOINT_SZ * 2 - DIVOOM_ASTRO_SLIDE_ENDPOINT_GAP * 2)
#define DIVOOM_ASTRO_SLIDE_KV_VAL_W         148
#define DIVOOM_ASTRO_SLIDE_SLIDER_STEP      5

#define DIVOOM_ASTRO_SLIDE_FILL_TOP         0x90FF78
#define DIVOOM_ASTRO_SLIDE_FILL_BOT         0x28B028
#define DIVOOM_ASTRO_SLIDE_TRACK_TOP        0xA8A8A8
#define DIVOOM_ASTRO_SLIDE_TRACK_BOT        0x585858
#define DIVOOM_ASTRO_SLIDE_KNOB_COLOR       0x8A8A8A

#define DIVOOM_ASTRO_SLIDE_ROW_CNT          5

enum
{
    DIVOOM_ASTRO_SLIDE_ROW_PLAYBACK = 0,
    DIVOOM_ASTRO_SLIDE_ROW_PHOTO_ORDER,
    DIVOOM_ASTRO_SLIDE_ROW_REVERSE,
    DIVOOM_ASTRO_SLIDE_ROW_SHOW_CLOCK,
    DIVOOM_ASTRO_SLIDE_ROW_SHOW_WEATHER,
};

static void divoom_astrootoo_settings_slideshow_style_slider(lv_obj_t *slider)
{
    lv_obj_clear_flag(slider, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(slider, lv_color_hex(DIVOOM_ASTRO_SLIDE_TRACK_TOP), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(slider, lv_color_hex(DIVOOM_ASTRO_SLIDE_TRACK_BOT), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(slider, LV_GRAD_DIR_VER, LV_PART_MAIN);
    lv_obj_set_style_radius(slider, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(slider, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, lv_color_hex(DIVOOM_ASTRO_SLIDE_FILL_TOP), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(slider, lv_color_hex(DIVOOM_ASTRO_SLIDE_FILL_BOT), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_dir(slider, LV_GRAD_DIR_VER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(slider, 0, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_hex(DIVOOM_ASTRO_SLIDE_KNOB_COLOR), LV_PART_KNOB);
    lv_obj_set_style_radius(slider, 0, LV_PART_KNOB);
    lv_obj_set_style_border_color(slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
    lv_obj_set_style_border_width(slider, 1, LV_PART_KNOB);
    lv_obj_set_style_pad_hor(slider, 3, LV_PART_KNOB);
    lv_obj_set_style_pad_ver(slider, 6, LV_PART_KNOB);
    lv_obj_set_style_outline_width(slider, 0, LV_PART_KNOB);
    lv_obj_set_style_outline_width(slider, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(slider, 0, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(slider, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    divoom_menu_control_slider_clear_border_outline_shadow(slider);
}

static void divoom_astrootoo_settings_slideshow_create_header(Pdivoom_astrootoo_settings_ctrl phandle)
{
    divoom_astrootoo_ui_create_page_header_tab(phandle->root_obj, &phandle->ui,
        "SlideEffect", DIVOOM_ASTRO_SLIDE_HEADER, phandle->menu_id);
}

static lv_obj_t *divoom_astrootoo_settings_slideshow_section_title(Pdivoom_astrootoo_settings_ctrl phandle,
    lv_coord_t y, const char *key)
{
    lv_obj_t *lbl;

    lbl = lv_label_create(phandle->content);
    lv_label_set_text(lbl, divoom_menu_control_get_disp_str(key));
    lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
    lv_obj_set_pos(lbl, DIVOOM_ASTRO_SLIDE_ROW_X + 8, y);
    return lbl;
}

static const char *divoom_astrootoo_settings_slideshow_order_key(uint8 disp_order)
{
    if (disp_order == 0)
    {
        return "OrderByAcceptTime";
    }
    if (disp_order == 1)
    {
        return "OrderByPhotoTime";
    }
    return "OrderByRandom";
}

static void divoom_astrootoo_settings_slideshow_sync_from_ui(Pdivoom_astrootoo_settings_ctrl phandle)
{
    Pdivoom_pic_phone_manage_slide_config cfg;

    cfg = divoom_pic_phone_manage_get_slide_config();
    if (!cfg || !phandle)
    {
        return;
    }
    if (phandle->slide_row_slider[0])
    {
        phandle->slide_playback_pct = (int)lv_slider_get_value(phandle->slide_row_slider[0]);
    }
    cfg->play_time = (uint32)divoom_pic_phone_manage_convert_percent_to_slide_speed(phandle->slide_playback_pct);
}

void divoom_astrootoo_settings_slideshow_save(Pdivoom_astrootoo_settings_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }
    divoom_astrootoo_settings_slideshow_sync_from_ui(phandle);
    divoom_pic_phone_manage_save_slide_config(1);
}

void divoom_astrootoo_settings_slideshow_picker_apply(Pdivoom_astrootoo_settings_ctrl phandle, int pos)
{
    Pdivoom_pic_phone_manage_slide_config cfg;

    cfg = divoom_pic_phone_manage_get_slide_config();
    if (!cfg || !phandle || pos < 0)
    {
        return;
    }
    if (phandle->page == DIVOOM_ASTROTOO_SETTINGS_PAGE_SLIDESHOW_PHOTO_ORDER)
    {
        cfg->disp_order = (uint8)pos;
        divoom_pic_album_manage_set_refresh_album_info();
    }
    divoom_pic_phone_manage_save_slide_config(0);
}

static void divoom_astrootoo_settings_slideshow_refresh(Pdivoom_astrootoo_settings_ctrl phandle)
{
    Pdivoom_pic_phone_manage_slide_config cfg;
    char buf[16];
    int i;

    cfg = divoom_pic_phone_manage_get_slide_config();
    if (!cfg || !phandle)
    {
        return;
    }
    if (phandle->slide_row_slider[0])
    {
        lv_slider_set_value(phandle->slide_row_slider[0], phandle->slide_playback_pct, LV_ANIM_OFF);
    }
    if (phandle->slide_reverse_hint)
    {
        lv_label_set_text(phandle->slide_reverse_hint,
            divoom_menu_control_get_disp_str(cfg->disp_reverse ? "DisplayReverseDesc1" : "DisplayReverseDesc0"));
        divoom_astrootoo_ui_list_label_refresh_marquee(
            phandle->slide_row[DIVOOM_ASTRO_SLIDE_ROW_REVERSE], phandle->slide_reverse_hint);
    }
    if (phandle->slide_reverse_lbl)
    {
        divoom_astrootoo_ui_list_label_refresh_marquee(
            phandle->slide_row[DIVOOM_ASTRO_SLIDE_ROW_REVERSE], phandle->slide_reverse_lbl);
    }
    for (i = DIVOOM_ASTRO_SLIDE_ROW_REVERSE; i <= DIVOOM_ASTRO_SLIDE_ROW_SHOW_WEATHER; i++)
    {
        int on;

        if (!phandle->slide_row_sw[i])
        {
            continue;
        }
        on = 0;
        if (i == DIVOOM_ASTRO_SLIDE_ROW_REVERSE)
        {
            on = cfg->disp_reverse ? 1 : 0;
        }
        else if (i == DIVOOM_ASTRO_SLIDE_ROW_SHOW_CLOCK)
        {
            on = cfg->clock_flag ? 1 : 0;
        }
        else if (i == DIVOOM_ASTRO_SLIDE_ROW_SHOW_WEATHER)
        {
            on = cfg->weather_flag ? 1 : 0;
        }
        divoom_menu_control_lv_img_set_src(phandle->slide_row_sw[i],
            on ? DIVOOM_ASTRO_SLIDE_SWITCH_ON : DIVOOM_ASTRO_SLIDE_SWITCH_OFF);
    }
    lv_snprintf(buf, sizeof(buf), "%d%%", phandle->slide_playback_pct);
    if (phandle->slide_row_val[DIVOOM_ASTRO_SLIDE_ROW_PLAYBACK])
    {
        lv_label_set_text(phandle->slide_row_val[DIVOOM_ASTRO_SLIDE_ROW_PLAYBACK], buf);
    }
}

static lv_obj_t *divoom_astrootoo_settings_slideshow_add_row(Pdivoom_astrootoo_settings_ctrl phandle,
    lv_coord_t y, int row_idx)
{
    lv_obj_t *card;

    card = lv_btn_create(phandle->content);
    lv_obj_add_style(card, &phandle->ui.card_item_style, 0);
    lv_obj_add_style(card, &phandle->ui.card_item_focus_style, LV_STATE_FOCUSED);
    lv_obj_add_style(card, &phandle->ui.card_item_focus_style, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_size(card, DIVOOM_ASTRO_SLIDE_ROW_W, DIVOOM_ASTRO_SLIDE_ROW_H);
    lv_obj_set_pos(card, DIVOOM_ASTRO_SLIDE_ROW_X, y);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    divoom_menu_direct_ctrl_update(phandle->direct_handle, card, 0, row_idx);
    phandle->slide_row[row_idx] = card;
    phandle->slide_row_kind[row_idx] = (uint8)row_idx;
    return card;
}

static void divoom_astrootoo_settings_slideshow_bump_slider(Pdivoom_astrootoo_settings_ctrl phandle,
    int delta)
{
    lv_obj_t *slider;
    int v;

    slider = phandle->slide_row_slider[0];
    if (!slider)
    {
        return;
    }
    v = (int)lv_slider_get_value(slider) + delta;
    if (v < 0)
    {
        v = 0;
    }
    if (v > 100)
    {
        v = 100;
    }
    lv_slider_set_value(slider, v, LV_ANIM_OFF);
    phandle->slide_playback_pct = v;
    divoom_astrootoo_settings_slideshow_refresh(phandle);
}

static void divoom_astrootoo_settings_slideshow_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_settings_ctrl phandle;
    lv_obj_t *target;
    lv_obj_t *next;
    uint8_t key_code;
    int pos;
    Pdivoom_pic_phone_manage_slide_config cfg;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    phandle = (Pdivoom_astrootoo_settings_ctrl)lv_event_get_user_data(e);
    target = lv_event_get_target(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    cfg = divoom_pic_phone_manage_get_slide_config();

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_menu_control_return_pre();
        return;
    }
    if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code))
    {
        int direct;

        direct = divoom_astrootoo_ui_key_is_up(key_code) ? DIVOOM_MENU_DIRECT_UP : DIVOOM_MENU_DIRECT_DOWN;
        next = divoom_menu_direct_ctrl_get(phandle->direct_handle, target, direct);
        if (next && next != target)
        {
            lv_group_focus_obj(next);
            divoom_astrootoo_ui_focus_scroll_keep_section_title(next);
        }
        return;
    }
    pos = divoom_menu_direct_ctrl_get_pos(phandle->direct_handle, target);
    if (pos < 0 || pos >= DIVOOM_ASTRO_SLIDE_ROW_CNT)
    {
        return;
    }
    if (divoom_astrootoo_ui_key_is_left(key_code))
    {
        if (pos == DIVOOM_ASTRO_SLIDE_ROW_PLAYBACK)
        {
            divoom_astrootoo_settings_slideshow_bump_slider(phandle, -DIVOOM_ASTRO_SLIDE_SLIDER_STEP);
        }
        return;
    }
    if (divoom_astrootoo_ui_key_is_right(key_code))
    {
        if (pos == DIVOOM_ASTRO_SLIDE_ROW_PLAYBACK)
        {
            divoom_astrootoo_settings_slideshow_bump_slider(phandle, DIVOOM_ASTRO_SLIDE_SLIDER_STEP);
        }
        return;
    }
    if (!divoom_astrootoo_ui_key_is_enter(key_code) || !cfg)
    {
        return;
    }
    switch (pos)
    {
    case DIVOOM_ASTRO_SLIDE_ROW_PHOTO_ORDER:
    {
        divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_SLIDESHOW_ORDER, NULL, 0, 0);
        break;
    }
    case DIVOOM_ASTRO_SLIDE_ROW_REVERSE:
    {
        cfg->disp_reverse = cfg->disp_reverse ? 0 : 1;
        divoom_astrootoo_settings_slideshow_refresh(phandle);
        break;
    }
    case DIVOOM_ASTRO_SLIDE_ROW_SHOW_CLOCK:
    {
        cfg->clock_flag = cfg->clock_flag ? 0 : 1;
        divoom_astrootoo_settings_slideshow_refresh(phandle);
        break;
    }
    case DIVOOM_ASTRO_SLIDE_ROW_SHOW_WEATHER:
    {
        cfg->weather_flag = cfg->weather_flag ? 0 : 1;
        divoom_astrootoo_settings_slideshow_refresh(phandle);
        break;
    }
    default:
    {
        break;
    }
    }
}

int divoom_astrootoo_settings_slideshow_is_page(divoom_astrootoo_settings_page_t page)
{
    if (page == DIVOOM_ASTROTOO_SETTINGS_PAGE_SLIDESHOW
        || page == DIVOOM_ASTROTOO_SETTINGS_PAGE_SLIDESHOW_PHOTO_ORDER)
    {
        return 1;
    }
    return 0;
}

void divoom_astrootoo_settings_slideshow_build(Pdivoom_astrootoo_settings_ctrl phandle)
{
    Pdivoom_pic_phone_manage_slide_config cfg;
    lv_obj_t *card;
    lv_obj_t *lbl;
    lv_obj_t *slider;
    lv_obj_t *trail;
    lv_obj_t *speed_slow;
    lv_obj_t *speed_fast;
    lv_coord_t y;

    cfg = divoom_pic_phone_manage_get_slide_config();
    if (!cfg || !phandle)
    {
        return;
    }
    phandle->slide_row_count = DIVOOM_ASTRO_SLIDE_ROW_CNT;
    phandle->slide_playback_pct = divoom_pic_phone_manage_convert_slide_speed_to_percent((int)cfg->play_time);
    phandle->direct_handle = divoom_menu_direct_ctrl_create(1, DIVOOM_ASTRO_SLIDE_ROW_CNT);
    divoom_astrootoo_settings_slideshow_create_header(phandle);
    phandle->content = divoom_astrootoo_ui_create_scroll_content(phandle->root_obj, &phandle->ui);

    y = DIVOOM_ASTRO_SLIDE_TOP_GAP;
    divoom_astrootoo_settings_slideshow_section_title(phandle, y, "PlaySpeed");
    y += DIVOOM_ASTRO_SLIDE_SEC_H;

    card = divoom_astrootoo_settings_slideshow_add_row(phandle, y, DIVOOM_ASTRO_SLIDE_ROW_PLAYBACK);
    lv_obj_add_event_cb(card, divoom_astrootoo_settings_slideshow_key_cb, LV_EVENT_KEY, phandle);
    speed_slow = lv_img_create(card);
    divoom_menu_control_lv_img_set_src(speed_slow, DIVOOM_ASTRO_SLIDE_SPEED_SLOW);
    lv_obj_align(speed_slow, LV_ALIGN_LEFT_MID, DIVOOM_ASTRO_SLIDE_CARD_PAD_H, 0);
    speed_fast = lv_img_create(card);
    divoom_menu_control_lv_img_set_src(speed_fast, DIVOOM_ASTRO_SLIDE_SPEED_FAST);
    lv_obj_align(speed_fast, LV_ALIGN_RIGHT_MID, -DIVOOM_ASTRO_SLIDE_CARD_PAD_H, 0);
    slider = lv_slider_create(card);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, phandle->slide_playback_pct, LV_ANIM_OFF);
    lv_obj_set_size(slider, DIVOOM_ASTRO_SLIDE_SLIDER_W, 8);
    lv_obj_align(slider, LV_ALIGN_LEFT_MID,
        DIVOOM_ASTRO_SLIDE_CARD_PAD_H + DIVOOM_ASTRO_SLIDE_ENDPOINT_SZ + DIVOOM_ASTRO_SLIDE_ENDPOINT_GAP, 0);
    divoom_astrootoo_settings_slideshow_style_slider(slider);
    phandle->slide_row_slider[0] = slider;
    y += DIVOOM_ASTRO_SLIDE_ROW_H + DIVOOM_ASTRO_SLIDE_ROW_GAP;

    divoom_astrootoo_settings_slideshow_section_title(phandle, y, "DisplayPreferences");
    y += DIVOOM_ASTRO_SLIDE_SEC_H;

    card = divoom_astrootoo_settings_slideshow_add_row(phandle, y, DIVOOM_ASTRO_SLIDE_ROW_PHOTO_ORDER);
    lv_obj_add_event_cb(card, divoom_astrootoo_settings_slideshow_key_cb, LV_EVENT_KEY, phandle);
    lbl = lv_label_create(card);
    lv_label_set_text(lbl, divoom_menu_control_get_disp_str("DisplayOrder"));
    lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 8, 0);
    divoom_astrootoo_ui_list_label_prepare(lbl,
        divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_SLIDE_ROW_W,
            DIVOOM_ASTRO_SET_CARD_LABEL_INSET_L, DIVOOM_ASTRO_SET_CARD_LABEL_INSET_R,
            DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W));
    divoom_astrootoo_ui_list_row_bind_marquee(card, lbl);
    phandle->slide_row_val[DIVOOM_ASTRO_SLIDE_ROW_PHOTO_ORDER] = NULL;
    trail = lv_img_create(card);
    divoom_menu_control_lv_img_set_src(trail, DIVOOM_ASTRO_SLIDE_CHEVRON);
    divoom_astrootoo_ui_apply_sublevel_chevron_size(trail);
    lv_obj_align(trail, LV_ALIGN_RIGHT_MID, -8, 0);
    y += DIVOOM_ASTRO_SLIDE_ROW_H + DIVOOM_ASTRO_SLIDE_ROW_GAP;

    card = divoom_astrootoo_settings_slideshow_add_row(phandle, y, DIVOOM_ASTRO_SLIDE_ROW_REVERSE);
    lv_obj_add_event_cb(card, divoom_astrootoo_settings_slideshow_key_cb, LV_EVENT_KEY, phandle);
    phandle->slide_reverse_lbl = lv_label_create(card);
    lv_label_set_text(phandle->slide_reverse_lbl, divoom_menu_control_get_disp_str("DisplayReverse"));
    lv_obj_set_style_text_color(phandle->slide_reverse_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(phandle->slide_reverse_lbl, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
    lv_obj_align(phandle->slide_reverse_lbl, LV_ALIGN_LEFT_MID, 8, -14);
    divoom_astrootoo_ui_list_label_prepare(phandle->slide_reverse_lbl,
        divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_SLIDE_ROW_W,
            DIVOOM_ASTRO_SET_CARD_LABEL_INSET_L, DIVOOM_ASTRO_SET_CARD_LABEL_INSET_R,
            DIVOOM_ASTRO_LIST_TOGGLE_W));
    divoom_astrootoo_ui_list_row_bind_marquee(card, phandle->slide_reverse_lbl);
    phandle->slide_reverse_hint = lv_label_create(card);
    lv_obj_set_style_text_color(phandle->slide_reverse_hint, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(phandle->slide_reverse_hint, divoom_menu_control_get_font_info(20, phandle->menu_id), 0);
    lv_obj_align(phandle->slide_reverse_hint, LV_ALIGN_LEFT_MID, 8, 16);
    divoom_astrootoo_ui_list_label_prepare(phandle->slide_reverse_hint,
        divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_SLIDE_ROW_W,
            DIVOOM_ASTRO_SET_CARD_LABEL_INSET_L, DIVOOM_ASTRO_SET_CARD_LABEL_INSET_R,
            DIVOOM_ASTRO_LIST_TOGGLE_W));
    divoom_astrootoo_ui_list_row_bind_marquee(card, phandle->slide_reverse_hint);
    trail = lv_img_create(card);
    phandle->slide_row_sw[DIVOOM_ASTRO_SLIDE_ROW_REVERSE] = trail;
    lv_obj_align(trail, LV_ALIGN_RIGHT_MID, -8, 0);
    y += DIVOOM_ASTRO_SLIDE_ROW_H + DIVOOM_ASTRO_SLIDE_ROW_GAP;

    divoom_astrootoo_settings_slideshow_section_title(phandle, y, "SmallComponent");
    y += DIVOOM_ASTRO_SLIDE_SEC_H;

    card = divoom_astrootoo_settings_slideshow_add_row(phandle, y, DIVOOM_ASTRO_SLIDE_ROW_SHOW_CLOCK);
    lv_obj_add_event_cb(card, divoom_astrootoo_settings_slideshow_key_cb, LV_EVENT_KEY, phandle);
    lbl = lv_label_create(card);
    lv_label_set_text(lbl, divoom_menu_control_get_disp_str("DisplayTime"));
    lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 8, 0);
    divoom_astrootoo_ui_list_label_prepare(lbl,
        divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_SLIDE_ROW_W,
            DIVOOM_ASTRO_SET_CARD_LABEL_INSET_L, DIVOOM_ASTRO_SET_CARD_LABEL_INSET_R,
            DIVOOM_ASTRO_LIST_TOGGLE_W));
    divoom_astrootoo_ui_list_row_bind_marquee(card, lbl);
    trail = lv_img_create(card);
    phandle->slide_row_sw[DIVOOM_ASTRO_SLIDE_ROW_SHOW_CLOCK] = trail;
    lv_obj_align(trail, LV_ALIGN_RIGHT_MID, -8, 0);
    y += DIVOOM_ASTRO_SLIDE_ROW_H + DIVOOM_ASTRO_SLIDE_ROW_GAP;

    card = divoom_astrootoo_settings_slideshow_add_row(phandle, y, DIVOOM_ASTRO_SLIDE_ROW_SHOW_WEATHER);
    lv_obj_add_event_cb(card, divoom_astrootoo_settings_slideshow_key_cb, LV_EVENT_KEY, phandle);
    lbl = lv_label_create(card);
    lv_label_set_text(lbl, divoom_menu_control_get_disp_str("DisplayWeather"));
    lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 8, 0);
    divoom_astrootoo_ui_list_label_prepare(lbl,
        divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_SLIDE_ROW_W,
            DIVOOM_ASTRO_SET_CARD_LABEL_INSET_L, DIVOOM_ASTRO_SET_CARD_LABEL_INSET_R,
            DIVOOM_ASTRO_LIST_TOGGLE_W));
    divoom_astrootoo_ui_list_row_bind_marquee(card, lbl);
    trail = lv_img_create(card);
    phandle->slide_row_sw[DIVOOM_ASTRO_SLIDE_ROW_SHOW_WEATHER] = trail;
    lv_obj_align(trail, LV_ALIGN_RIGHT_MID, -8, 0);

    divoom_astrootoo_settings_slideshow_refresh(phandle);
}

void divoom_astrootoo_settings_slideshow_group_detach(Pdivoom_astrootoo_settings_ctrl phandle)
{
    int i;

    if (!phandle)
    {
        return;
    }
    for (i = 0; i < phandle->slide_row_count; i++)
    {
        if (phandle->slide_row[i] && lv_obj_is_valid(phandle->slide_row[i]))
        {
            lv_group_remove_obj(phandle->slide_row[i]);
        }
    }
}

void divoom_astrootoo_settings_slideshow_group_bind(Pdivoom_astrootoo_settings_ctrl phandle,
    lv_obj_t *focus_target)
{
    lv_group_t *g;
    int i;
    lv_obj_t *focus;

    if (!phandle || phandle->slide_row_count <= 0)
    {
        return;
    }
    g = lv_group_get_default();
    lv_group_remove_all_objs(g);
    for (i = 0; i < phandle->slide_row_count; i++)
    {
        if (phandle->slide_row[i])
        {
            lv_group_add_obj(g, phandle->slide_row[i]);
        }
    }
    focus = NULL;
    if (focus_target && lv_obj_is_valid(focus_target))
    {
        focus = focus_target;
    }
    else if (phandle->slide_row[0])
    {
        focus = phandle->slide_row[0];
    }
    if (focus)
    {
        lv_group_focus_obj(focus);
        divoom_astrootoo_ui_focus_scroll_keep_section_title(focus);
    }
}

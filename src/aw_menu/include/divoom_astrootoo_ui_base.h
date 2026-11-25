#ifndef _divoom_astrootoo_ui_base_h_
#define _divoom_astrootoo_ui_base_h_

#include "lvgl.h"
#include "divoom_astrootoo_theme.h"
#include "divoom_user_key.h"

typedef struct _divoom_astrootoo_ui_base
{
    lv_style_t root_style;
    lv_style_t content_style;
    lv_style_t grid_cont_style;
    lv_style_t list_item_style;
    lv_style_t list_item_focus_style;
    lv_style_t list_item_border_focus_style;
    lv_style_t card_item_style;
    lv_style_t card_item_focus_style;
    uint8 styles_initialized;
} divoom_astrootoo_ui_base, *Pdivoom_astrootoo_ui_base;

void divoom_astrootoo_ui_base_init_styles(Pdivoom_astrootoo_ui_base phandle);
void divoom_astrootoo_ui_base_reset_styles(Pdivoom_astrootoo_ui_base phandle);

lv_obj_t *divoom_astrootoo_ui_create_root(Pdivoom_astrootoo_ui_base phandle);
lv_obj_t *divoom_astrootoo_ui_create_header(lv_obj_t *parent, Pdivoom_astrootoo_ui_base phandle,
    const char *title_key, int menu_id);
lv_obj_t *divoom_astrootoo_ui_create_page_header_tab(lv_obj_t *parent, Pdivoom_astrootoo_ui_base phandle,
    const char *title_key, const char *bg_img_file, int menu_id);
lv_obj_t *divoom_astrootoo_ui_create_page_header_img(lv_obj_t *parent, Pdivoom_astrootoo_ui_base phandle,
    const char *title_text, const char *bg_img_file, int menu_id);
lv_obj_t *divoom_astrootoo_ui_create_subpage_header(lv_obj_t *parent, Pdivoom_astrootoo_ui_base phandle,
    const char *title_key, int menu_id);
lv_obj_t *divoom_astrootoo_ui_create_subpage_header_text(lv_obj_t *parent, Pdivoom_astrootoo_ui_base phandle,
    const char *title_text, int menu_id);
lv_obj_t *divoom_astrootoo_ui_create_scroll_content(lv_obj_t *parent, Pdivoom_astrootoo_ui_base phandle);

lv_obj_t *divoom_astrootoo_ui_create_repeat_row(lv_obj_t *parent, Pdivoom_astrootoo_ui_base phandle,
    const char *label_key, int row_index, int row_count, void *direct_handle);

lv_obj_t *divoom_astrootoo_ui_create_card_row(lv_obj_t *parent, Pdivoom_astrootoo_ui_base phandle,
    const char *label_key, int row_index, int row_count, void *direct_handle, int menu_id,
    int bind_left_marquee);

enum
{
    DIVOOM_ASTROTOO_TF_ROW_FOLDER = 0,
    DIVOOM_ASTROTOO_TF_ROW_TRACK,
};

#define DIVOOM_ASTRO_TF_LIST_ROW_H      66
#define DIVOOM_ASTRO_TF_ICON_W          36
#define DIVOOM_ASTRO_TF_ICON_H          36
#define DIVOOM_ASTRO_TF_LABEL_H         32
#define DIVOOM_ASTRO_TF_CHEVRON_W       DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W
#define DIVOOM_ASTRO_TF_CHEVRON_H       DIVOOM_ASTRO_SUBLEVEL_CHEVRON_H

void divoom_astrootoo_ui_apply_sublevel_chevron_size(lv_obj_t *chevron);

lv_obj_t *divoom_astrootoo_ui_create_tf_list_row(lv_obj_t *parent, Pdivoom_astrootoo_ui_base phandle,
    int row_kind, int row_index, lv_coord_t row_h, lv_coord_t row_gap, int menu_id,
    lv_obj_t **out_icon, lv_obj_t **out_label, lv_obj_t **out_chevron, lv_obj_t **out_check);

/* 超长文件夹/歌名：单行；焦点滚动，非焦点裁切（不换行、不加点） */
void divoom_astrootoo_ui_tf_label_apply_focus(lv_obj_t *label, int focused);
void divoom_astrootoo_ui_tf_label_refresh_long_mode(lv_obj_t *label);

/* 列表行：限宽单行 + 焦点时按溢出决定是否横向滚动（键值行请只绑「值」侧 label） */
void divoom_astrootoo_ui_list_label_prepare(lv_obj_t *label, lv_coord_t width);
void divoom_astrootoo_ui_list_row_bind_marquee(lv_obj_t *row, lv_obj_t *label);
void divoom_astrootoo_ui_list_label_refresh_marquee(lv_obj_t *row, lv_obj_t *label);
/* 不可聚焦行：不依赖 row 焦点，超长即滚动 */
void divoom_astrootoo_ui_list_label_marquee_always(lv_obj_t *label);

lv_coord_t divoom_astrootoo_ui_list_label_width(lv_coord_t row_w,
    lv_coord_t inset_l, lv_coord_t inset_r, lv_coord_t trail_w);
lv_coord_t divoom_astrootoo_ui_list_kv_value_width(lv_coord_t row_w,
    lv_coord_t inset_l, lv_coord_t inset_r, lv_coord_t key_w,
    lv_coord_t key_val_gap, lv_coord_t trail_w);
lv_coord_t divoom_astrootoo_ui_list_kv_key_width(lv_coord_t row_w,
    lv_coord_t inset_l, lv_coord_t inset_r, lv_coord_t val_w,
    lv_coord_t key_val_gap, lv_coord_t trail_w);
/* 键值行：左标题限宽 + 右值宽度/右对齐；key_val 间距用 DIVOOM_ASTRO_LIST_KV_GAP */
void divoom_astrootoo_ui_list_kv_bind_row(lv_obj_t *key_lbl, lv_obj_t *val_lbl,
    lv_coord_t row_w, lv_coord_t inset_l, lv_coord_t inset_r,
    lv_coord_t trail_w, lv_coord_t val_align_ofs, lv_coord_t val_min_w);

void divoom_astrootoo_ui_focus_scroll(lv_obj_t *obj);
/* 分组标题不是焦点。scroll_to_view(卡片) 会把卡片顶到视口上沿并裁掉标题。
 * 内容区第一项滚回 y=0；其后带前置标题的分组把标题（或标题所在分组）滚到视口顶。 */
void divoom_astrootoo_ui_focus_scroll_keep_section_title(lv_obj_t *row);

uint32_t divoom_astrootoo_ui_key_raw_from_event(lv_event_t *e);
uint8_t divoom_astrootoo_ui_key_code_from_event(lv_event_t *e);
uint8_t divoom_astrootoo_ui_key_press_from_event(lv_event_t *e);
int divoom_astrootoo_ui_key_is_short_press(lv_event_t *e);
int divoom_astrootoo_ui_key_is_nav_key(uint8_t key_code);
int divoom_astrootoo_ui_key_is_nav_press(lv_event_t *e);
int divoom_astrootoo_ui_key_is_menu_press(lv_event_t *e, uint8_t key_code);
int divoom_astrootoo_ui_direct_nav_step(void *direct_handle, lv_obj_t *target,
    uint8_t key_code, lv_obj_t **out_next);
int divoom_astrootoo_ui_key_is_prev(uint8_t key_code);
int divoom_astrootoo_ui_key_is_next(uint8_t key_code);
int divoom_astrootoo_ui_key_is_enter(uint8_t key_code);
int divoom_astrootoo_ui_key_is_home(uint8_t key_code);
int divoom_astrootoo_ui_key_is_esc(uint8_t key_code);
/* 二维列表/网格专用：上/下用 PREV/NEXT，左/右用实体方向键。
   勿用 is_prev/is_next 做二维导航——二者会把"上=左、下=右"混为一谈。
   VOL± 是 LIGHT_LEFT/RIGHT，不算左右。 */
int divoom_astrootoo_ui_key_is_up(uint8_t key_code);
int divoom_astrootoo_ui_key_is_down(uint8_t key_code);
int divoom_astrootoo_ui_key_is_left(uint8_t key_code);
int divoom_astrootoo_ui_key_is_right(uint8_t key_code);

lv_obj_t *divoom_astrootoo_ui_create_key_capture_btn(lv_obj_t *parent, Pdivoom_astrootoo_ui_base phandle,
    lv_event_cb_t key_cb, void *user_data);
lv_obj_t *divoom_astrootoo_ui_create_status_bar(lv_obj_t *parent, Pdivoom_astrootoo_ui_base phandle,
    lv_obj_t **out_time_label);
lv_obj_t *divoom_astrootoo_ui_create_main_menu_header(lv_obj_t *parent, Pdivoom_astrootoo_ui_base phandle,
    int menu_id, lv_obj_t **out_time_label, lv_obj_t **out_source_img, lv_obj_t **out_wifi_img,
    lv_obj_t **out_battery_img);

/* FlowtooConfirmModal：F101 用透明 overlay + shell/frame（勿用 ink 半透明 dim） */
lv_obj_t *divoom_astrootoo_ui_confirm_modal_create(lv_obj_t *parent, lv_obj_t **out_frame);
/* layout=fullBleed：480×480 暖色实底，无居中卡片；勿再调 confirm_modal_layout */
lv_obj_t *divoom_astrootoo_ui_confirm_modal_create_full_bleed(lv_obj_t *parent, lv_obj_t **out_frame);
void divoom_astrootoo_ui_confirm_modal_layout(lv_obj_t *frame);
lv_obj_t *divoom_astrootoo_ui_confirm_modal_make_btn(lv_obj_t *parent, const char *label,
    int menu_id, int font_size, lv_event_cb_t key_cb, void *user_data, int add_to_group);
void divoom_astrootoo_ui_confirm_modal_sync_btn_labels(lv_obj_t *cancel_btn, lv_obj_t *ok_btn,
    int ok_focus);

/* FlowtooAlertModal：shell/frame 与 confirm 相同；单钮琥珀 OK，勿清空 lv_group */
lv_obj_t *divoom_astrootoo_ui_alert_modal_create(lv_obj_t *parent, lv_obj_t **out_frame);
lv_obj_t *divoom_astrootoo_ui_alert_modal_add_title(lv_obj_t *frame, const char *text, int menu_id);
lv_obj_t *divoom_astrootoo_ui_alert_modal_add_body(lv_obj_t *frame, const char *text, int menu_id);
lv_obj_t *divoom_astrootoo_ui_alert_modal_make_ok_btn(lv_obj_t *parent, const char *label,
    int menu_id, lv_event_cb_t key_cb, void *user_data);

#endif

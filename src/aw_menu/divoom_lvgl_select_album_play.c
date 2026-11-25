#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_menu_direct_ctrl.h"
#include "divoom_lvgl_select_album_play.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_astrootoo_theme.h"
#include "divoom_lvgl_photo_manage.h"
#include "divoom_pixel_format.h"

/* AstroToo gallery 相册列表：3 列 × 144px（真源 GalleryScreens.module.css · albumGrid） */
#define DIVOOM_LVGL_SELECT_ALBUM_DEFER_MS           50
#define DIVOOM_LVGL_SELECT_ALBUM_COLS           3
#define DIVOOM_LVGL_SELECT_ALBUM_CELL_W         144
#define DIVOOM_LVGL_SELECT_ALBUM_GAP            4
#define DIVOOM_LVGL_SELECT_ALBUM_GRID_W         \
    (DIVOOM_LVGL_SELECT_ALBUM_COLS * DIVOOM_LVGL_SELECT_ALBUM_CELL_W \
     + (DIVOOM_LVGL_SELECT_ALBUM_COLS - 1) * DIVOOM_LVGL_SELECT_ALBUM_GAP)
#define DIVOOM_LVGL_SELECT_ALBUM_PAD_X          ((DIVOOM_SCREEN_WIDTH - DIVOOM_LVGL_SELECT_ALBUM_GRID_W) / 2)
#define DIVOOM_LVGL_SELECT_ALBUM_PAD_TOP         8
#define DIVOOM_LVGL_SELECT_ALBUM_COVER_SIZE     140
#define DIVOOM_LVGL_SELECT_ALBUM_CELL_BORDER   2   /* albumCell · border 2px（box-sizing 内绘） */
#define DIVOOM_LVGL_SELECT_ALBUM_CELL_GAP_IN     6
#define DIVOOM_LVGL_SELECT_ALBUM_NAME_H          30   /* 24px 字 + 行高余量，避免底部裁切 */
#define DIVOOM_LVGL_SELECT_ALBUM_COUNT_H         18
#define DIVOOM_LVGL_SELECT_ALBUM_CELL_PAD_B      4
#define DIVOOM_LVGL_SELECT_ALBUM_CELL_H         \
    (DIVOOM_LVGL_SELECT_ALBUM_CELL_BORDER * 2 \
     + DIVOOM_LVGL_SELECT_ALBUM_COVER_SIZE \
     + DIVOOM_LVGL_SELECT_ALBUM_CELL_GAP_IN * 2 \
     + DIVOOM_LVGL_SELECT_ALBUM_NAME_H + DIVOOM_LVGL_SELECT_ALBUM_COUNT_H \
     + DIVOOM_LVGL_SELECT_ALBUM_CELL_PAD_B)
/* albumCellActive: color-mix(surfaceFocus 12%, astroBg) on #F0E4D2 */
#define DIVOOM_LVGL_SELECT_ALBUM_CELL_FOCUS_BG  0xF1DEBF
#define DIVOOM_LVGL_SELECT_ALBUM_COVER_BUF_SIZE \
    (DIVOOM_LVGL_SELECT_ALBUM_COVER_SIZE * DIVOOM_LVGL_SELECT_ALBUM_COVER_SIZE * DIVOOM_APP_RGB565A8_BYTES_PER_PX)
#define DIVOOM_LVGL_SELECT_ALBUM_COVER_LOAD_BATCH 2
#define DIVOOM_LVGL_SELECT_ALBUM_SCROLL_PAD_B   12

typedef struct _divoom_lvgl_select_album_play_ctrl
{
    divoom_astrootoo_ui_base ui;
    int menu_id;
    int first_flag;
    int select_items;
    int start_album_num;
    int cover_load_index;
    uint8 cover_loaded[DIVOOM_PHOTO_MAXALBUM + 1];
    void *img_tbl_obj[DIVOOM_PHOTO_MAXALBUM + 1];
    void *btn_tbl_obj[DIVOOM_PHOTO_MAXALBUM + 1];
    void *album_name_label_obj[DIVOOM_PHOTO_MAXALBUM + 1];
    void *album_total_label_obj[DIVOOM_PHOTO_MAXALBUM + 1];
    int album_include_pic_num[DIVOOM_PHOTO_MAXALBUM + 1];
    uint8 restore_focus_light;
    lv_obj_t *root_obj;
    lv_obj_t *scroll_obj;
    lv_obj_t *grid_obj;
    void *pdirect_menu_ctrl;
    lv_obj_t *alert_overlay;
    lv_obj_t *alert_btn;
    void *last_active_obj;
    lv_obj_t *restore_focus_obj;
    lv_obj_t *key_btn;              /* 无相册项时捕获按键，保证 ESC 可返回 */
    lv_timer_t *restore_input_timer;
    lv_timer_t *dialog_close_timer;
    lv_timer_t *cover_load_timer;
    lv_timer_t *grid_build_timer;
    uint8 grid_built;
    divoom_lvgl_select_album_play_para para_info;
    lv_img_dsc_t *image_data_table;
    Pdivoom_pic_album_manage_head_data palbum_info;
} divoom_lvgl_select_album_play_ctrl, *Pdivoom_lvgl_select_album_play_ctrl;

static void divoom_lvgl_select_album_play_key_cb(lv_event_t *e);
static void divoom_lvgl_select_album_play_create_notice_dialog(Pdivoom_lvgl_select_album_play_ctrl phandle);
static int divoom_lvgl_select_album_play_get_first_pic_data_from_album(Pdivoom_lvgl_select_album_play_ctrl phandle, int index);
static void divoom_lvgl_select_album_play_format_count(Pdivoom_lvgl_select_album_play_ctrl phandle, int index, char *buf, int buf_len);
static void divoom_lvgl_select_album_play_cover_load_timer_cb(lv_timer_t *timer);
static void divoom_lvgl_select_album_play_schedule_cover_load(Pdivoom_lvgl_select_album_play_ctrl phandle, int start_index);
static void divoom_lvgl_select_album_play_deferred_focus_cb(lv_timer_t *timer);
static void divoom_lvgl_select_album_play_deferred_grid_cb(lv_timer_t *timer);
static void divoom_lvgl_select_album_play_build_grid(Pdivoom_lvgl_select_album_play_ctrl phandle);
static void divoom_lvgl_select_album_play_focus_scroll(Pdivoom_lvgl_select_album_play_ctrl phandle, lv_obj_t *obj);
static void divoom_lvgl_select_album_play_on_focus_cell(Pdivoom_lvgl_select_album_play_ctrl phandle, lv_obj_t *cur_obj);
static void divoom_lvgl_select_album_play_sync_input(Pdivoom_lvgl_select_album_play_ctrl phandle);
static void divoom_lvgl_select_album_play_create_key_btn(Pdivoom_lvgl_select_album_play_ctrl phandle);
static int divoom_lvgl_select_album_play_obj_is_album_btn(Pdivoom_lvgl_select_album_play_ctrl phandle, lv_obj_t *obj);

/*
 * 相册封面统一走 get_album_disp_rgba_buf：webp 仅在中间层短暂解码，LVGL 只绑 RGB565A8 缓冲。
 * 空相册无 userdata 封面时，由 divoom_pic_album_manage 回退 DIVOOM_EMPTY_ALBUM_PHOTO_NAME。
 * 勿对 lv_img 直接 lv_img_set_src(.webp)，避免 LVGL 解码器常驻缓存、额外占内存。
 */
static int divoom_lvgl_select_album_play_apply_album_cover(Pdivoom_lvgl_select_album_play_ctrl phandle, int index, int album_id)
{
    int image_w = DIVOOM_LVGL_SELECT_ALBUM_COVER_SIZE;
    int image_h = DIVOOM_LVGL_SELECT_ALBUM_COVER_SIZE;

    DIVOOM_LV_IMG_HEADER_INIT(&phandle->image_data_table[index].header);
    phandle->image_data_table[index].header.cf = LV_COLOR_FORMAT_RGB565A8;
    phandle->image_data_table[index].header.w = (lv_coord_t)image_w;
    phandle->image_data_table[index].header.h = (lv_coord_t)image_h;
    phandle->image_data_table[index].data_size = DIVOOM_LVGL_SELECT_ALBUM_COVER_BUF_SIZE;
    if (phandle->image_data_table[index].data == NULL)
    {
        phandle->image_data_table[index].data = divoom_system_malloc(DIVOOM_LVGL_SELECT_ALBUM_COVER_BUF_SIZE);
        if (phandle->image_data_table[index].data == NULL)
        {
            return 0;
        }
    }
    if (!divoom_pic_album_manage_get_album_disp_rgba_buf(album_id, image_w, image_h,
            (uint8 *)phandle->image_data_table[index].data))
    {
        return 0;
    }
    lv_img_set_src(phandle->img_tbl_obj[index], &phandle->image_data_table[index]);
    return 1;
}

/* GalleryAlbumListScreen · albumCell：失焦透明；聚焦描边 + 12% focus 混色底（实色近似） */
static void divoom_lvgl_select_album_play_apply_cell_style(lv_obj_t *btn)
{
    lv_obj_remove_style_all(btn);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn, DIVOOM_LVGL_SELECT_ALBUM_CELL_BORDER, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
    lv_obj_set_style_border_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(btn, DIVOOM_ASTRO_RADIUS_SM, 0);
    lv_obj_set_style_clip_corner(btn, false, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_style_pad_row(btn, DIVOOM_LVGL_SELECT_ALBUM_CELL_GAP_IN, 0);
    lv_obj_set_style_pad_bottom(btn, DIVOOM_LVGL_SELECT_ALBUM_CELL_PAD_B, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_bg_color(btn, lv_color_hex(DIVOOM_LVGL_SELECT_ALBUM_CELL_FOCUS_BG), LV_STATE_FOCUSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_FOCUSED);
    lv_obj_set_style_bg_color(btn, lv_color_hex(DIVOOM_LVGL_SELECT_ALBUM_CELL_FOCUS_BG), LV_STATE_FOCUS_KEY);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_opa(btn, LV_OPA_COVER, LV_STATE_FOCUSED);
    lv_obj_set_style_border_opa(btn, LV_OPA_COVER, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(btn, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(btn, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), LV_STATE_FOCUS_KEY);
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
    /* 主轴 START：避免 CENTER 在固定高度内挤压 label 导致首帧底部裁切 */
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
}

static void divoom_lvgl_select_album_play_deferred_focus_cb(lv_timer_t *timer)
{
    Pdivoom_lvgl_select_album_play_ctrl phandle;
    lv_obj_t *focus_obj;

    phandle = (Pdivoom_lvgl_select_album_play_ctrl)lv_timer_get_user_data(timer);
    if (!phandle)
    {
        return;
    }
    phandle->restore_input_timer = NULL;
    focus_obj = phandle->restore_focus_obj;
    phandle->restore_focus_obj = NULL;
    if (phandle->restore_focus_light)
    {
        phandle->restore_focus_light = 0;
        if (focus_obj && lv_obj_is_valid(focus_obj))
        {
            lv_group_focus_obj(focus_obj);
            lv_obj_scroll_to_view(focus_obj, LV_ANIM_ON);
        }
        else if (phandle->last_active_obj && lv_obj_is_valid(phandle->last_active_obj))
        {
            lv_group_focus_obj(phandle->last_active_obj);
            lv_obj_scroll_to_view(phandle->last_active_obj, LV_ANIM_ON);
        }
        divoom_menu_control_add_key_event();
        return;
    }
    if (focus_obj && lv_obj_is_valid(focus_obj))
    {
        divoom_lvgl_select_album_play_focus_scroll(phandle, focus_obj);
        divoom_lvgl_select_album_play_on_focus_cell(phandle, focus_obj);
    }
    else if (phandle->last_active_obj && lv_obj_is_valid(phandle->last_active_obj))
    {
        divoom_lvgl_select_album_play_focus_scroll(phandle, phandle->last_active_obj);
        divoom_lvgl_select_album_play_on_focus_cell(phandle, phandle->last_active_obj);
    }
    else if (phandle->select_items > 0 && phandle->btn_tbl_obj[0])
    {
        divoom_lvgl_select_album_play_focus_scroll(phandle, phandle->btn_tbl_obj[0]);
        divoom_lvgl_select_album_play_on_focus_cell(phandle, phandle->btn_tbl_obj[0]);
    }
    divoom_menu_control_add_key_event();
}

static int divoom_lvgl_select_album_play_album_index(Pdivoom_lvgl_select_album_play_ctrl phandle, int ui_index)
{
    return phandle->palbum_info->album_num - (phandle->start_album_num + ui_index) - 1;
}

static void divoom_lvgl_select_album_play_format_count(Pdivoom_lvgl_select_album_play_ctrl phandle, int index, char *buf, int buf_len)
{
    const char *tpl;
    int n;
    int off;
    const char *p;

    tpl = divoom_menu_control_get_disp_str("galleryPhotoCount");
    if (!tpl || !tpl[0])
    {
        tpl = "%d photos";
    }
    n = phandle->album_include_pic_num[index];
    if (strstr(tpl, "{{n}}"))
    {
        off = 0;
        p = tpl;
        while (*p && off < buf_len - 1)
        {
            if (p[0] == '{' && p[1] == '{' && p[2] == 'n' && p[3] == '}' && p[4] == '}')
            {
                off += snprintf(buf + off, (size_t)(buf_len - off), "%d", n);
                p += 5;
            }
            else
            {
                buf[off++] = *p++;
            }
        }
        buf[off] = '\0';
    }
    else
    {
        snprintf(buf, (size_t)buf_len, tpl, n);
    }
}

static int divoom_lvgl_select_album_play_get_first_pic_data_from_album(Pdivoom_lvgl_select_album_play_ctrl phandle, int index)
{
    int total;
    int first_pic_id;
    int album_index;

    if (!phandle || index < 0 || index >= phandle->select_items)
    {
        return 0;
    }

    album_index = divoom_lvgl_select_album_play_album_index(phandle, index);
    total = divoom_pic_phone_manage_get_total_pic_num(phandle->palbum_info->palbum_list[album_index].id);
    phandle->album_include_pic_num[index] = total;

    if (total > 0
        && divoom_pic_album_manage_check_album_cover_exist(phandle->palbum_info->palbum_list[album_index].id) == 0)
    {
        first_pic_id = divoom_pic_phone_manage_get_first_pic_id(phandle->palbum_info->palbum_list[album_index].id);
        if (first_pic_id > 0)
        {
            divoom_pic_album_manage_set_album_cover(phandle->palbum_info->palbum_list[album_index].id, first_pic_id, 1, 0);
        }
    }

    divoom_lvgl_select_album_play_apply_album_cover(phandle, index,
        phandle->palbum_info->palbum_list[album_index].id);
    phandle->cover_loaded[index] = 1;
    return total;
}

static void divoom_lvgl_select_album_play_cover_load_timer_cb(lv_timer_t *timer)
{
    Pdivoom_lvgl_select_album_play_ctrl phandle;
    int loaded;
    int i;
    char buf[64];

    phandle = (Pdivoom_lvgl_select_album_play_ctrl)lv_timer_get_user_data(timer);
    if (!phandle)
    {
        return;
    }

    loaded = 0;
    for (i = 0; i < DIVOOM_LVGL_SELECT_ALBUM_COVER_LOAD_BATCH && phandle->cover_load_index < phandle->select_items; i++)
    {
        if (!phandle->cover_loaded[phandle->cover_load_index])
        {
            divoom_lvgl_select_album_play_get_first_pic_data_from_album(phandle, phandle->cover_load_index);
            divoom_lvgl_select_album_play_format_count(phandle, phandle->cover_load_index, buf, (int)sizeof(buf));
            lv_label_set_text(phandle->album_total_label_obj[phandle->cover_load_index], buf);
            loaded++;
        }
        phandle->cover_load_index++;
    }

    if (phandle->cover_load_index >= phandle->select_items)
    {
        lv_timer_del(phandle->cover_load_timer);
        phandle->cover_load_timer = NULL;
    }
    else if (loaded == 0)
    {
        phandle->cover_load_index++;
    }
}

static void divoom_lvgl_select_album_play_schedule_cover_load(Pdivoom_lvgl_select_album_play_ctrl phandle, int start_index)
{
    if (!phandle)
    {
        return;
    }
    phandle->cover_load_index = start_index;
    if (phandle->cover_load_timer)
    {
        lv_timer_del(phandle->cover_load_timer);
    }
    phandle->cover_load_timer = lv_timer_create(divoom_lvgl_select_album_play_cover_load_timer_cb, 16, phandle);
    lv_timer_set_repeat_count(phandle->cover_load_timer, -1);
}

static void divoom_lvgl_select_album_play_alert_close(Pdivoom_lvgl_select_album_play_ctrl phandle);
static void divoom_lvgl_select_album_play_alert_restore_focus(Pdivoom_lvgl_select_album_play_ctrl phandle);
static void divoom_lvgl_select_album_play_schedule_alert_close(Pdivoom_lvgl_select_album_play_ctrl phandle);
static void divoom_lvgl_select_album_play_dialog_close_timer_cb(lv_timer_t *timer);
static void divoom_lvgl_select_album_play_notice_open_timer_cb(lv_timer_t *timer);

static void divoom_lvgl_select_album_play_alert_close(Pdivoom_lvgl_select_album_play_ctrl phandle)
{
    if (!phandle || !phandle->alert_overlay)
    {
        return;
    }
    if (phandle->alert_btn)
    {
        lv_group_remove_obj(phandle->alert_btn);
    }
    lv_obj_del(phandle->alert_overlay);
    phandle->alert_overlay = NULL;
    phandle->alert_btn = NULL;
}

static void divoom_lvgl_select_album_play_alert_restore_focus(Pdivoom_lvgl_select_album_play_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }
    if (phandle->restore_input_timer)
    {
        lv_timer_del(phandle->restore_input_timer);
        phandle->restore_input_timer = NULL;
    }
    phandle->restore_focus_light = 1;
    phandle->restore_focus_obj = phandle->last_active_obj;
    phandle->restore_input_timer = lv_timer_create(divoom_lvgl_select_album_play_deferred_focus_cb,
        DIVOOM_LVGL_SELECT_ALBUM_DEFER_MS, phandle);
    if (phandle->restore_input_timer)
    {
        lv_timer_set_repeat_count(phandle->restore_input_timer, 1);
    }
}

static void divoom_lvgl_select_album_play_dialog_close_timer_cb(lv_timer_t *timer)
{
    Pdivoom_lvgl_select_album_play_ctrl phandle;

    phandle = (Pdivoom_lvgl_select_album_play_ctrl)lv_timer_get_user_data(timer);
    if (phandle)
    {
        phandle->dialog_close_timer = NULL;
    }
    lv_timer_del(timer);
    if (!phandle || !phandle->alert_overlay)
    {
        return;
    }
    divoom_lvgl_select_album_play_alert_close(phandle);
    divoom_lvgl_select_album_play_alert_restore_focus(phandle);
}

static void divoom_lvgl_select_album_play_schedule_alert_close(Pdivoom_lvgl_select_album_play_ctrl phandle)
{
    lv_timer_t *timer;

    if (!phandle || !phandle->alert_overlay || phandle->dialog_close_timer)
    {
        return;
    }
    timer = lv_timer_create(divoom_lvgl_select_album_play_dialog_close_timer_cb,
        DIVOOM_LVGL_SELECT_ALBUM_DEFER_MS, phandle);
    if (timer)
    {
        phandle->dialog_close_timer = timer;
        lv_timer_set_repeat_count(timer, 1);
    }
}

static void divoom_lvgl_select_album_play_alert_key_cb(lv_event_t *e)
{
    Pdivoom_lvgl_select_album_play_ctrl phandle;
    uint8_t key_code;

    phandle = (Pdivoom_lvgl_select_album_play_ctrl)lv_event_get_user_data(e);
    if (!phandle || lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    if (divoom_astrootoo_ui_key_is_enter(key_code) || divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_lvgl_select_album_play_schedule_alert_close(phandle);
    }
}

static void divoom_lvgl_select_album_play_notice_open_timer_cb(lv_timer_t *timer)
{
    Pdivoom_lvgl_select_album_play_ctrl phandle;

    phandle = (Pdivoom_lvgl_select_album_play_ctrl)lv_timer_get_user_data(timer);
    lv_timer_del(timer);
    if (!phandle)
    {
        return;
    }
    divoom_lvgl_select_album_play_create_notice_dialog(phandle);
}

static void divoom_lvgl_select_album_play_create_notice_dialog(Pdivoom_lvgl_select_album_play_ctrl phandle)
{
    lv_obj_t *frame;
    lv_obj_t *actions;
    lv_group_t *grp;

    if (!phandle || !phandle->root_obj)
    {
        return;
    }

    divoom_lvgl_select_album_play_alert_close(phandle);

    grp = lv_group_get_default();
    phandle->last_active_obj = lv_group_get_focused(grp);

    phandle->alert_overlay = divoom_astrootoo_ui_alert_modal_create(phandle->root_obj, &frame);
    if (!phandle->alert_overlay || !frame)
    {
        return;
    }

    divoom_astrootoo_ui_alert_modal_add_title(frame,
        divoom_menu_control_get_disp_str("NoPictureInAlbum"), phandle->menu_id);
    divoom_astrootoo_ui_alert_modal_add_body(frame,
        divoom_menu_control_get_disp_str("NoPictureInAlbumDesc"), phandle->menu_id);

    actions = lv_obj_create(frame);
    lv_obj_remove_style_all(actions);
    lv_obj_set_width(actions, LV_PCT(100));
    lv_obj_set_height(actions, LV_SIZE_CONTENT);
    lv_obj_clear_flag(actions, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    phandle->alert_btn = divoom_astrootoo_ui_alert_modal_make_ok_btn(actions,
        divoom_menu_control_get_disp_str("Cancel"), phandle->menu_id,
        divoom_lvgl_select_album_play_alert_key_cb, phandle);

    divoom_astrootoo_ui_confirm_modal_layout(frame);
}

static void divoom_lvgl_select_album_play_focus_scroll(Pdivoom_lvgl_select_album_play_ctrl phandle, lv_obj_t *obj)
{
    if (!phandle || !obj)
    {
        return;
    }
    lv_group_focus_obj(obj);
    lv_obj_scroll_to_view(obj, LV_ANIM_ON);
}

static int divoom_lvgl_select_album_play_obj_is_album_btn(Pdivoom_lvgl_select_album_play_ctrl phandle, lv_obj_t *obj)
{
    int i;

    if (!phandle || !obj || !lv_obj_is_valid(obj))
    {
        return 0;
    }
    for (i = 0; i < phandle->select_items; i++)
    {
        if (obj == phandle->btn_tbl_obj[i])
        {
            return 1;
        }
    }
    return 0;
}

static void divoom_lvgl_select_album_play_create_key_btn(Pdivoom_lvgl_select_album_play_ctrl phandle)
{
    if (!phandle || !phandle->root_obj || phandle->key_btn)
    {
        return;
    }

    phandle->key_btn = divoom_astrootoo_ui_create_key_capture_btn(phandle->root_obj, &phandle->ui,
        divoom_lvgl_select_album_play_key_cb, phandle);
    lv_obj_add_flag(phandle->key_btn, LV_OBJ_FLAG_HIDDEN);
}

static void divoom_lvgl_select_album_play_sync_input(Pdivoom_lvgl_select_album_play_ctrl phandle)
{
    lv_group_t *grp;
    int i;
    int vis = 0;

    if (!phandle)
    {
        return;
    }
    grp = lv_group_get_default();
    if (!grp)
    {
        return;
    }

    if (phandle->key_btn)
    {
        lv_obj_add_flag(phandle->key_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(phandle->key_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_group_remove_obj(phandle->key_btn);
        lv_obj_move_background(phandle->key_btn);
    }

    /* 清掉共享 key group 里的残留对象（如刚销毁的照片网格按钮），只保留相册按钮，
     * 避免返回相册后焦点停在已失效对象上导致按键失效 */
    lv_group_remove_all_objs(grp);
    for (i = 0; i < phandle->select_items; i++)
    {
        lv_obj_t *btn = phandle->btn_tbl_obj[i];

        if (btn && lv_obj_is_valid(btn))
        {
            lv_group_add_obj(grp, btn);
            vis++;
        }
    }

    /* 未配网/相册列表为空时没有可聚焦格子，用透明按键捕获 ESC（对齐闹钟 TF 空列表） */
    if (vis == 0 && phandle->key_btn)
    {
        lv_obj_clear_flag(phandle->key_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->key_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_move_foreground(phandle->key_btn);
        lv_group_add_obj(grp, phandle->key_btn);
        lv_group_focus_obj(phandle->key_btn);
    }
}

static void divoom_lvgl_select_album_play_on_focus_cell(Pdivoom_lvgl_select_album_play_ctrl phandle, lv_obj_t *cur_obj)
{
    int i;
    char buf[64];

    for (i = 0; i < phandle->select_items; i++)
    {
        if (cur_obj == phandle->btn_tbl_obj[i])
        {
            if (!phandle->cover_loaded[i])
            {
                divoom_lvgl_select_album_play_get_first_pic_data_from_album(phandle, i);
                divoom_lvgl_select_album_play_format_count(phandle, i, buf, (int)sizeof(buf));
                lv_label_set_text(phandle->album_total_label_obj[i], buf);
            }
            break;
        }
    }
}

static void divoom_lvgl_select_album_play_key_cb(lv_event_t *e)
{
    Pdivoom_lvgl_select_album_play_ctrl phandle;
    lv_obj_t *cur_obj;
    lv_obj_t *next_obj;
    uint8_t key_code;
    int i;
    int album_index;

    phandle = (Pdivoom_lvgl_select_album_play_ctrl)lv_event_get_user_data(e);
    if (!phandle || lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }

    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    if (phandle->alert_overlay)
    {
        return;
    }

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_menu_control_return_pre();
        return;
    }

    cur_obj = lv_group_get_focused(lv_group_get_default());
    if (!cur_obj)
    {
        return;
    }

    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        for (i = 0; i < phandle->select_items; i++)
        {
            if (cur_obj != phandle->btn_tbl_obj[i])
            {
                continue;
            }
            album_index = divoom_lvgl_select_album_play_album_index(phandle, i);
            if (phandle->para_info.type == 1)
            {
                divoom_pic_album_manage_add_list_to_name(phandle->palbum_info->palbum_list[album_index].id,
                    phandle->para_info.pic_list, phandle->para_info.pic_num);
                divoom_menu_control_return_pre();
            }
            else if (phandle->para_info.type == 0 && phandle->album_include_pic_num[i])
            {
                divoom_lvgl_photo_manage_para *para_info;

                para_info = divoom_system_malloc(sizeof(divoom_lvgl_photo_manage_para));
                para_info->operate_type = DIVOOM_LVGL_PHOTO_MANAGE_VIEW_PICTURE;
                para_info->album_id = phandle->palbum_info->palbum_list[album_index].id;
                phandle->para_info.album_id = para_info->album_id;
                divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_PHOTO_MANAGE, para_info, 0, 0);
            }
            else
            {
                lv_timer_t *timer;

                phandle->last_active_obj = cur_obj;
                timer = lv_timer_create(divoom_lvgl_select_album_play_notice_open_timer_cb,
                    DIVOOM_LVGL_SELECT_ALBUM_DEFER_MS, phandle);
                if (timer)
                {
                    lv_timer_set_repeat_count(timer, 1);
                }
            }
            return;
        }
        return;
    }

    next_obj = NULL;
    if (divoom_astrootoo_ui_key_is_left(key_code))
    {
        next_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_menu_ctrl, cur_obj, DIVOOM_MENU_DIRECT_LEFT);
    }
    else if (divoom_astrootoo_ui_key_is_right(key_code))
    {
        next_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_menu_ctrl, cur_obj, DIVOOM_MENU_DIRECT_RIGHT);
    }
    else if (divoom_astrootoo_ui_key_is_up(key_code))
    {
        next_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_menu_ctrl, cur_obj, DIVOOM_MENU_DIRECT_UP);
    }
    else if (divoom_astrootoo_ui_key_is_down(key_code))
    {
        next_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_menu_ctrl, cur_obj, DIVOOM_MENU_DIRECT_DOWN);
    }

    if (next_obj && next_obj != cur_obj)
    {
        divoom_lvgl_select_album_play_focus_scroll(phandle, next_obj);
        divoom_lvgl_select_album_play_on_focus_cell(phandle, next_obj);
    }
}

static void divoom_lvgl_select_album_play_build_grid(Pdivoom_lvgl_select_album_play_ctrl phandle)
{
    Pdivoom_pic_album_manage_head_data palbum_info;
    lv_obj_t *btn_obj;
    lv_obj_t *cout_obj;
    lv_obj_t *img_obj;
    lv_obj_t *name_lab;
    lv_obj_t *count_lab;
    char buf[64];
    int i;
    int col;
    int row;

    if (!phandle || phandle->grid_built || !phandle->grid_obj)
    {
        return;
    }
    palbum_info = phandle->palbum_info;
    if (!palbum_info || phandle->select_items <= 0)
    {
        phandle->grid_built = 1;
        return;
    }

    for (i = 0; i < phandle->select_items; i++)
    {
        col = i % DIVOOM_LVGL_SELECT_ALBUM_COLS;
        row = i / DIVOOM_LVGL_SELECT_ALBUM_COLS;

        btn_obj = lv_btn_create(phandle->grid_obj);
        divoom_lvgl_select_album_play_apply_cell_style(btn_obj);
        lv_obj_set_size(btn_obj, DIVOOM_LVGL_SELECT_ALBUM_CELL_W, DIVOOM_LVGL_SELECT_ALBUM_CELL_H);
        lv_obj_set_pos(btn_obj,
            DIVOOM_LVGL_SELECT_ALBUM_PAD_X + col * (DIVOOM_LVGL_SELECT_ALBUM_CELL_W + DIVOOM_LVGL_SELECT_ALBUM_GAP),
            row * (DIVOOM_LVGL_SELECT_ALBUM_CELL_H + DIVOOM_LVGL_SELECT_ALBUM_GAP));
        phandle->btn_tbl_obj[i] = btn_obj;
        divoom_menu_direct_ctrl_update(phandle->pdirect_menu_ctrl, btn_obj, col, row);
        lv_obj_add_event_cb(btn_obj, divoom_lvgl_select_album_play_key_cb, LV_EVENT_KEY, phandle);
        lv_group_add_obj(lv_group_get_default(), btn_obj);

        cout_obj = lv_obj_create(btn_obj);
        lv_obj_add_style(cout_obj, &phandle->ui.content_style, 0);
        lv_obj_set_size(cout_obj, DIVOOM_LVGL_SELECT_ALBUM_COVER_SIZE, DIVOOM_LVGL_SELECT_ALBUM_COVER_SIZE);
        lv_obj_set_style_radius(cout_obj, 8, 0);
        lv_obj_set_style_clip_corner(cout_obj, true, 0);
        lv_obj_set_style_pad_all(cout_obj, 0, 0);
        lv_obj_clear_flag(cout_obj, LV_OBJ_FLAG_SCROLLABLE);

        img_obj = lv_img_create(cout_obj);
        phandle->img_tbl_obj[i] = img_obj;
        lv_obj_set_size(img_obj, DIVOOM_LVGL_SELECT_ALBUM_COVER_SIZE, DIVOOM_LVGL_SELECT_ALBUM_COVER_SIZE);
        lv_obj_center(img_obj);
        /* 封面由 cover_load_timer → apply_album_cover 统一解码绑定，建格不设 webp 直读 */

        name_lab = lv_label_create(btn_obj);
        phandle->album_name_label_obj[i] = name_lab;
        lv_obj_set_width(name_lab, DIVOOM_LVGL_SELECT_ALBUM_CELL_W - 8);
        lv_obj_set_height(name_lab, DIVOOM_LVGL_SELECT_ALBUM_NAME_H);
        lv_label_set_long_mode(name_lab, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(name_lab, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(name_lab, lv_color_hex(DIVOOM_ASTRO_INK), 0);
        lv_obj_set_style_text_font(name_lab, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
        lv_obj_add_flag(name_lab, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
        lv_label_set_text(name_lab, divoom_pic_album_manage_get_static_album_name(
            palbum_info->palbum_list[divoom_lvgl_select_album_play_album_index(phandle, i)].id));

        count_lab = lv_label_create(btn_obj);
        phandle->album_total_label_obj[i] = count_lab;
        lv_obj_set_width(count_lab, DIVOOM_LVGL_SELECT_ALBUM_CELL_W - 8);
        lv_obj_set_height(count_lab, DIVOOM_LVGL_SELECT_ALBUM_COUNT_H);
        lv_obj_set_style_text_align(count_lab, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(count_lab, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
        lv_obj_set_style_text_font(count_lab, divoom_menu_control_get_font_info(16, phandle->menu_id), 0);
        lv_obj_add_flag(count_lab, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

        phandle->cover_loaded[i] = 0;
        phandle->album_include_pic_num[i] = divoom_pic_phone_manage_get_total_pic_num(
            palbum_info->palbum_list[divoom_lvgl_select_album_play_album_index(phandle, i)].id);
        divoom_lvgl_select_album_play_format_count(phandle, i, buf, (int)sizeof(buf));
        lv_label_set_text(count_lab, buf);
    }

    phandle->grid_built = 1;
    lv_obj_update_layout(phandle->grid_obj);
    divoom_lvgl_select_album_play_schedule_cover_load(phandle, 0);
}

static void divoom_lvgl_select_album_play_deferred_grid_cb(lv_timer_t *timer)
{
    Pdivoom_lvgl_select_album_play_ctrl phandle;
    lv_obj_t *focus_obj;

    phandle = (Pdivoom_lvgl_select_album_play_ctrl)lv_timer_get_user_data(timer);
    if (!phandle)
    {
        return;
    }
    phandle->grid_build_timer = NULL;
    divoom_lvgl_select_album_play_build_grid(phandle);
    divoom_lvgl_select_album_play_sync_input(phandle);

    focus_obj = phandle->restore_focus_obj;
    phandle->restore_focus_obj = NULL;
    if (focus_obj && lv_obj_is_valid(focus_obj)
        && divoom_lvgl_select_album_play_obj_is_album_btn(phandle, focus_obj))
    {
        phandle->last_active_obj = focus_obj;
        lv_group_focus_obj(focus_obj);
        lv_obj_scroll_to_view(focus_obj, LV_ANIM_OFF);
        divoom_lvgl_select_album_play_on_focus_cell(phandle, focus_obj);
    }
    else if (phandle->select_items > 0 && phandle->btn_tbl_obj[0])
    {
        phandle->last_active_obj = phandle->btn_tbl_obj[0];
        lv_group_focus_obj(phandle->btn_tbl_obj[0]);
        lv_obj_scroll_to_view(phandle->btn_tbl_obj[0], LV_ANIM_OFF);
        divoom_lvgl_select_album_play_on_focus_cell(phandle, phandle->btn_tbl_obj[0]);
    }
    divoom_menu_control_add_key_event();
}

void *divoom_lvgl_select_album_play_init(void *user_data, int menu_id)
{
    Pdivoom_lvgl_select_album_play_ctrl phandle;
    Pdivoom_pic_album_manage_head_data palbum_info;
    lv_obj_t *root;
    lv_coord_t content_h;
    lv_coord_t grid_h;
    int rows;
    const char *title_key;

    phandle = divoom_system_malloc(sizeof(divoom_lvgl_select_album_play_ctrl));
    phandle->menu_id = menu_id;
    phandle->first_flag = 1;
    phandle->cover_load_index = 0;
    phandle->cover_load_timer = NULL;
    phandle->grid_build_timer = NULL;
    phandle->grid_built = 0;

    if (user_data)
    {
        memcpy(&phandle->para_info, user_data, sizeof(phandle->para_info));
    }
    else
    {
        phandle->para_info.type = 0;
    }

    divoom_thumb_cache_set_dest_pic_size(DIVOOM_LVGL_SELECT_ALBUM_COVER_SIZE, DIVOOM_LVGL_SELECT_ALBUM_COVER_SIZE);
    palbum_info = divoom_pic_album_manage_get_all_album_info();
    phandle->palbum_info = palbum_info;
    if (!palbum_info)
    {
        phandle->select_items = 0;
        phandle->grid_built = 1;
        root = divoom_astrootoo_ui_create_root(&phandle->ui);
        phandle->root_obj = root;
        divoom_astrootoo_ui_create_subpage_header(root, &phandle->ui, "ALBUMS", menu_id);
        divoom_lvgl_select_album_play_create_key_btn(phandle);
        divoom_lvgl_select_album_play_sync_input(phandle);
        return phandle;
    }

    if (phandle->para_info.type == 1)
    {
        phandle->start_album_num = 1;
        title_key = "AddPicToAlbum";
    }
    else
    {
        phandle->start_album_num = 0;
        title_key = "ALBUMS";
    }

    phandle->select_items = palbum_info->album_num - phandle->start_album_num;
    if (phandle->select_items < 0)
    {
        phandle->select_items = 0;
    }
    if (phandle->select_items > DIVOOM_PHOTO_MAXALBUM)
    {
        phandle->select_items = DIVOOM_PHOTO_MAXALBUM;
    }

    rows = (phandle->select_items + DIVOOM_LVGL_SELECT_ALBUM_COLS - 1) / DIVOOM_LVGL_SELECT_ALBUM_COLS;
    if (rows < 1)
    {
        rows = 1;
    }

    root = divoom_astrootoo_ui_create_root(&phandle->ui);
    phandle->root_obj = root;
    divoom_astrootoo_ui_create_subpage_header(root, &phandle->ui, title_key, menu_id);

    content_h = (lv_coord_t)(DIVOOM_ASTRO_CONTENT_H - DIVOOM_ASTRO_PAGE_PAD);
    grid_h = (lv_coord_t)(rows * DIVOOM_LVGL_SELECT_ALBUM_CELL_H
        + (rows > 1 ? (rows - 1) * DIVOOM_LVGL_SELECT_ALBUM_GAP : 0));

    phandle->scroll_obj = divoom_astrootoo_ui_create_scroll_content(root, &phandle->ui);
    lv_obj_set_pos(phandle->scroll_obj, 0, DIVOOM_ASTRO_CONTENT_Y);
    lv_obj_set_size(phandle->scroll_obj, DIVOOM_SCREEN_WIDTH, content_h);
    lv_obj_set_scrollbar_mode(phandle->scroll_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_top(phandle->scroll_obj, DIVOOM_LVGL_SELECT_ALBUM_PAD_TOP, 0);
    lv_obj_set_style_pad_left(phandle->scroll_obj, 4, 0);
    lv_obj_set_style_pad_right(phandle->scroll_obj, 4, 0);
    lv_obj_set_style_pad_bottom(phandle->scroll_obj, DIVOOM_LVGL_SELECT_ALBUM_SCROLL_PAD_B, 0);

    phandle->grid_obj = lv_obj_create(phandle->scroll_obj);
    lv_obj_add_style(phandle->grid_obj, &phandle->ui.content_style, 0);
    lv_obj_set_size(phandle->grid_obj, DIVOOM_SCREEN_WIDTH, grid_h);
    lv_obj_set_pos(phandle->grid_obj, 0, 0);
    lv_obj_clear_flag(phandle->grid_obj, LV_OBJ_FLAG_SCROLLABLE);

    phandle->pdirect_menu_ctrl = divoom_menu_direct_ctrl_create(DIVOOM_LVGL_SELECT_ALBUM_COLS, rows);
    phandle->image_data_table = divoom_system_malloc(sizeof(lv_img_dsc_t) * DIVOOM_PHOTO_MAXALBUM);

    divoom_lvgl_select_album_play_create_key_btn(phandle);

    phandle->grid_build_timer = lv_timer_create(divoom_lvgl_select_album_play_deferred_grid_cb, 0, phandle);
    if (phandle->grid_build_timer)
    {
        lv_timer_set_repeat_count(phandle->grid_build_timer, 1);
    }
    else if (phandle->select_items <= 0)
    {
        divoom_lvgl_select_album_play_sync_input(phandle);
    }
    return phandle;
}

void divoom_lvgl_select_album_play_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_select_album_play_ctrl phandle;
    int i;
    int total;
    char buf[64];
    lv_obj_t *restore_obj;

    phandle = (Pdivoom_lvgl_select_album_play_ctrl)handle;
    lv_scr_load(phandle->root_obj);

    if (phandle->dialog_close_timer)
    {
        lv_timer_del(phandle->dialog_close_timer);
        phandle->dialog_close_timer = NULL;
    }
    divoom_lvgl_select_album_play_alert_close(phandle);

    divoom_lvgl_select_album_play_sync_input(phandle);

    /* 选定要聚焦的相册按钮：优先恢复返回前的焦点，否则上次焦点，否则首项 */
    restore_obj = (lv_obj_t *)active_obj;
    if (phandle->first_flag || !divoom_lvgl_select_album_play_obj_is_album_btn(phandle, restore_obj))
    {
        restore_obj = phandle->last_active_obj;
    }
    if (!divoom_lvgl_select_album_play_obj_is_album_btn(phandle, restore_obj))
    {
        restore_obj = (phandle->select_items > 0) ? phandle->btn_tbl_obj[0] : NULL;
    }

    if (phandle->restore_input_timer)
    {
        lv_timer_del(phandle->restore_input_timer);
        phandle->restore_input_timer = NULL;
    }
    phandle->restore_focus_light = 0;

    if (!phandle->grid_built)
    {
        phandle->restore_focus_obj = restore_obj;
        phandle->first_flag = 0;
        return;
    }

    for (i = 0; i < phandle->select_items; i++)
    {
        total = divoom_pic_phone_manage_get_total_pic_num(
            phandle->palbum_info->palbum_list[divoom_lvgl_select_album_play_album_index(phandle, i)].id);
        if (phandle->album_include_pic_num[i] != total)
        {
            phandle->album_include_pic_num[i] = total;
            divoom_lvgl_select_album_play_format_count(phandle, i, buf, (int)sizeof(buf));
            lv_label_set_text(phandle->album_total_label_obj[i], buf);
            if (phandle->cover_loaded[i])
            {
                phandle->cover_loaded[i] = 0;
                divoom_lvgl_select_album_play_get_first_pic_data_from_album(phandle, i);
            }
        }
    }

    phandle->restore_focus_obj = NULL;

    /* 同步聚焦到有效相册按钮：返回即焦点稳定，按键立即可用（不再延迟到 timer） */
    if (restore_obj && lv_obj_is_valid(restore_obj))
    {
        phandle->last_active_obj = restore_obj;
        lv_group_focus_obj(restore_obj);
        lv_obj_scroll_to_view(restore_obj, LV_ANIM_OFF);
        divoom_lvgl_select_album_play_on_focus_cell(phandle, restore_obj);
    }
    else if (phandle->select_items <= 0)
    {
        divoom_lvgl_select_album_play_sync_input(phandle);
    }
    phandle->first_flag = 0;
}

void divoom_lvgl_select_album_play_pause(void *handle)
{
    Pdivoom_lvgl_select_album_play_ctrl phandle;
    lv_obj_t *focused;

    phandle = (Pdivoom_lvgl_select_album_play_ctrl)handle;
    if (!phandle)
    {
        return;
    }
    if (phandle->cover_load_timer)
    {
        lv_timer_del(phandle->cover_load_timer);
        phandle->cover_load_timer = NULL;
    }
    focused = lv_group_get_focused(lv_group_get_default());
    if (divoom_lvgl_select_album_play_obj_is_album_btn(phandle, focused))
    {
        phandle->last_active_obj = focused;
    }
}

void divoom_lvgl_select_album_play_exit(void *handle)
{
    Pdivoom_lvgl_select_album_play_ctrl phandle;
    int i;

    phandle = (Pdivoom_lvgl_select_album_play_ctrl)handle;
    if (!phandle)
    {
        return;
    }

    if (phandle->cover_load_timer)
    {
        lv_timer_del(phandle->cover_load_timer);
        phandle->cover_load_timer = NULL;
    }
    if (phandle->grid_build_timer)
    {
        lv_timer_del(phandle->grid_build_timer);
        phandle->grid_build_timer = NULL;
    }
    if (phandle->restore_input_timer)
    {
        lv_timer_del(phandle->restore_input_timer);
        phandle->restore_input_timer = NULL;
    }
    if (phandle->dialog_close_timer)
    {
        lv_timer_del(phandle->dialog_close_timer);
        phandle->dialog_close_timer = NULL;
    }
    phandle->restore_focus_obj = NULL;

    divoom_lvgl_select_album_play_alert_close(phandle);

    if (phandle->image_data_table)
    {
        for (i = 0; i < phandle->select_items; i++)
        {
            if (phandle->image_data_table[i].data)
            {
                divoom_system_free((uint8 *)phandle->image_data_table[i].data);
            }
        }
        divoom_system_free(phandle->image_data_table);
    }

    if (phandle->para_info.type == 1 && phandle->para_info.pic_list)
    {
        divoom_system_free(phandle->para_info.pic_list);
    }

    if (phandle->root_obj)
    {
        lv_obj_del(phandle->root_obj);
    }

    divoom_menu_direct_ctrl_destory(phandle->pdirect_menu_ctrl);
    divoom_astrootoo_ui_base_reset_styles(&phandle->ui);
    divoom_system_free(handle);
}

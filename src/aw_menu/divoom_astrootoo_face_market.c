#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_menu_direct_ctrl.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_astrootoo_theme.h"
#include "divoom_astrootoo_face_market.h"
#include "divoom_light_app.h"
#include "divoom_sys_model_ctrl.h"
#include "divoom_net_app.h"
#include "divoom_clock_manage.h"
#include "divoom_dial_preview_default.h"
#include "divoom_face_preview_loader.h"

/*
 * AstroToo 表盘应用市场（480x480，对照 AstroTooUI WatchFaceManageScreen）。
 * 顶部：三固定 tab（最近使用 / 收藏 / 全部）。
 * 下方：左栏（60 预览 + 名称）· 中收藏 · 右删除。对照模拟器 WatchFaceManage。
 * 数据：全部=我的表盘 clock_history；收藏/最近均为我的表盘子集。
 */

#define FM_STRIP_BACKDROP_H       74
#define FM_TAB_ROW_W              472
#define FM_TAB_GAP                4
#define FM_TAB_H                  56
#define FM_TAB_COUNT              DIVOOM_ASTRO_FACE_MARKET_TAB_COUNT
#define FM_CONTENT_TOP            (FM_STRIP_BACKDROP_H + 6)
#define FM_CONTENT_H              (DIVOOM_SCREEN_HIGH - FM_CONTENT_TOP)
#define FM_ROW_W                  472
#define FM_ROW_H                  72
#define FM_ROW_GAP                4
#define FM_ACTION_W               64
#define FM_COL_GAP                4
#define FM_TEXT_W                 (FM_ROW_W - FM_ACTION_W - FM_ACTION_W - FM_COL_GAP - FM_COL_GAP)
#define FM_LIST_PAD_X             ((DIVOOM_SCREEN_WIDTH - FM_ROW_W) / 2)
#define FM_STRIP_BG               0x243746
#define FM_TAB_IDLE_BG            0x414D56
#define FM_ACT_ICON_SZ            24
#define FM_THUMB_SZ               60
#define FM_THUMB_PAD              6
#define FM_THUMB_GAP              8
#define FM_LABEL_X                (FM_THUMB_PAD + FM_THUMB_SZ + FM_THUMB_GAP)
#define FM_LABEL_RIGHT_PAD        12
#define FM_ROW_LABEL_W            (FM_TEXT_W - FM_LABEL_X - FM_ACT_ICON_SZ - DIVOOM_ASTRO_LIST_LABEL_GAP - FM_LABEL_RIGHT_PAD)

#define FM_STAR_SZ                28
#define FM_DELETE_ICON            "106/sleep_delete.png"
#define FM_IN_USE_ICON            "23/at_repeat_check.png"

#define FM_REFRESH_MS             3000
/* 连续操作时推迟本地预览解码，停稳后接收后台结果。 */
#define FM_THUMB_SETTLE_MS        200
#define FM_THUMB_BIND_MS          40
#ifndef FACE_PREVIEW_PERF
#define FACE_PREVIEW_PERF 0
#endif
#define FM_LIST_MAX               500
#define FM_RECENT_MAX             20
#define FM_VISIBLE_MAX            8
#define FM_VISIBLE_FOCUS_BEFORE   3
#define FM_NAME_MAX               64
#define FM_SCAN_BATCH             FM_VISIBLE_MAX
#define FM_SCAN_PERIOD_MS         20

enum
{
    FM_MODAL_NONE = 0,
    FM_MODAL_KEEP_MIN_ONE,
};

enum
{
    FM_PANE_STRIP = 0,
    FM_PANE_LIST,
};

enum
{
    FM_COL_TEXT = 0,
    FM_COL_FAVORITE,
    FM_COL_DELETE,
};

typedef struct _fm_row
{
    lv_obj_t *btn;
    lv_obj_t *text_cont;
    lv_obj_t *fav_cont;
    lv_obj_t *delete_cont;
    lv_obj_t *thumb;
    lv_obj_t *thumb_img;
    lv_obj_t *name_label;
    lv_obj_t *in_use_icon;
    lv_obj_t *fav_icon;
    lv_obj_t *delete_icon;
    int clock_id;
    uint8 is_favorite;
    uint8 thumb_ready;
    divoom_face_preview_result *decoded; /* UI-owned; survives row rotation. */
} fm_row;

typedef struct {
    int clock_id;
    char name[FM_NAME_MAX];
} fm_name_entry;

typedef struct _divoom_astrootoo_face_market_ctrl
{
    divoom_astrootoo_ui_base ui;
    int menu_id;
    lv_obj_t *root_obj;

    lv_obj_t *strip_cont;
    lv_obj_t *strip_track;
    lv_obj_t *tab_btn[FM_TAB_COUNT];
    lv_obj_t *tab_label[FM_TAB_COUNT];
    int active_tab;
    int focus_pane;

    lv_obj_t *content;
    lv_obj_t *empty_label;
    /* 原始列表用于刷新比对；clock_ids 仅在加载完成后保留有名表盘。 */
    int *source_clock_ids;
    int *clock_ids;
    uint8 *named_mask;
    int scan_idx;
    uint8 load_busy;
    fm_row *rows;
    int row_slots; /* 已创建的行控件。切 tab 时留下，只改绑定。 */
    uint8 rows_rebind; /* 列表 id 已换，下次出表必须整窗重绑。 */
    int row_count;
    int visible_start;
    int visible_count;
    int list_focus;
    int col_focus;
    void *list_direct;

    int built_for_tab;
    int built_count;

    lv_obj_t *modal_overlay;
    lv_obj_t *modal_btn;
    int modal_kind;

    lv_obj_t *confirm_overlay;
    lv_obj_t *confirm_cancel_btn;
    lv_obj_t *confirm_ok_btn;
    int confirm_focus;
    int confirm_delete_row;

    lv_timer_t *build_timer;
    lv_timer_t *scan_timer;
    lv_timer_t *refresh_timer;
    /* 只在 LVGL 线程使用。到时后回调里清空指针，离开页面时若还在则删除。 */
    lv_timer_t *thumb_settle_timer;
    divoom_face_preview_loader *preview_loader;
    divoom_dial_preview_default default_preview;
    fm_name_entry *name_cache;
    int name_cache_count;
    int name_cache_next;
    int name_cache_lang;
#if FACE_PREVIEW_PERF
    uint32 focus_count, focus_max_ms, focus_hist[7], thumb_binds;
#endif
} divoom_astrootoo_face_market_ctrl, *Pdivoom_astrootoo_face_market_ctrl;

/* 仅由 LVGL 线程使用；后台解码器只接收 ID 快照，不访问页面。 */
static Pdivoom_astrootoo_face_market_ctrl s_fm_page;

static const char *fm_tab_label_keys[FM_TAB_COUNT] =
{
    "WatchFaceTabRecent",
    "WatchFaceTabFavorite",
    "WatchFaceTabAll",
};

static void fm_strip_key_cb(lv_event_t *e);
static void fm_list_key_cb(lv_event_t *e);
static void fm_build_list(Pdivoom_astrootoo_face_market_ctrl ph);
static void fm_request_build(Pdivoom_astrootoo_face_market_ctrl ph);
static void fm_clear_visible_rows(Pdivoom_astrootoo_face_market_ctrl ph);
static void fm_hide_kept_rows(Pdivoom_astrootoo_face_market_ctrl ph);
static void fm_drop_empty_label(Pdivoom_astrootoo_face_market_ctrl ph);
static void fm_release_list_data(Pdivoom_astrootoo_face_market_ctrl ph);
static void fm_stop_scan(Pdivoom_astrootoo_face_market_ctrl ph);
static void fm_commit_list(Pdivoom_astrootoo_face_market_ctrl ph);
static void fm_arm_thumb_settle(Pdivoom_astrootoo_face_market_ctrl ph);
static void fm_row_set_thumb(Pdivoom_astrootoo_face_market_ctrl ph, fm_row *r);
static void fm_row_release_decoded(fm_row *r);
static void fm_publish_thumb_window(Pdivoom_astrootoo_face_market_ctrl ph, int activity);
static void fm_scan_cb(lv_timer_t *t);
static void fm_focus_strip(Pdivoom_astrootoo_face_market_ctrl ph);
static void fm_rebuild_visible_rows(Pdivoom_astrootoo_face_market_ctrl ph);
static void fm_show_empty(Pdivoom_astrootoo_face_market_ctrl ph);
static int fm_visible_index(Pdivoom_astrootoo_face_market_ctrl ph, int row);
static void fm_hide_modal(Pdivoom_astrootoo_face_market_ctrl ph);
static void fm_show_modal(Pdivoom_astrootoo_face_market_ctrl ph, int kind);
static void fm_modal_key_cb(lv_event_t *e);
static void fm_confirm_close(Pdivoom_astrootoo_face_market_ctrl ph);
static void fm_show_delete_confirm(Pdivoom_astrootoo_face_market_ctrl ph, int row);
static void fm_delete_row_commit(Pdivoom_astrootoo_face_market_ctrl ph, int row);
static int fm_dialog_blocks_keys(Pdivoom_astrootoo_face_market_ctrl ph);
static void fm_play_clock(Pdivoom_astrootoo_face_market_ctrl ph, int row);
static void fm_switch_tab(Pdivoom_astrootoo_face_market_ctrl ph, int tab);

static int fm_my_clock_count(void)
{
    int n;

    n = divoom_light_app_get_my_clock_num();
    if (n < 0)
    {
        n = 0;
    }
    return n;
}

static int fm_clock_in_history(int clock_id)
{
    int n;
    int i;
    int buf[DIVOOM_MAX_CLOCK_NUM];

    if (clock_id <= 0)
    {
        return 0;
    }
    n = divoom_light_app_copy_my_clock_list_limited(buf, DIVOOM_MAX_CLOCK_NUM);
    for (i = 0; i < n; i++)
    {
        if (buf[i] == clock_id)
        {
            return 1;
        }
    }
    return 0;
}

static int fm_tab_empty_label_key(int tab)
{
    if (tab == DIVOOM_ASTRO_FACE_MARKET_TAB_FAVORITE)
    {
        return 1;
    }
    if (tab == DIVOOM_ASTRO_FACE_MARKET_TAB_RECENT)
    {
        return 2;
    }
    return 0;
}

static const char *fm_empty_label_for_tab(int tab)
{
    switch (fm_tab_empty_label_key(tab))
    {
    case 1:
        return "WatchFaceFavoriteEmpty";
    case 2:
        return "WatchFaceRecentEmpty";
    default:
        return "No items";
    }
}

static int fm_clock_id_exists(const int *ids, int n, int clock_id)
{
    int i;

    if (!ids || clock_id <= 0)
    {
        return 0;
    }

    for (i = 0; i < n; i++)
    {
        if (ids[i] == clock_id)
        {
            return 1;
        }
    }
    return 0;
}

static void fm_append_unique_clock_id(int *ids, int *n, int max_n, int clock_id)
{
    if (!ids || !n || *n >= max_n || clock_id <= 0)
    {
        return;
    }

    /* 底层最近/收藏/全部数组可能残留重复 id，列表展示前统一去重。 */
    if (fm_clock_id_exists(ids, *n, clock_id))
    {
        return;
    }

    ids[*n] = clock_id;
    (*n)++;
}

static int fm_collect_tab_clock_ids(Pdivoom_astrootoo_face_market_ctrl ph, int *ids, int max_n)
{
    int tab;
    int n;
    int i;
    int all_buf[FM_LIST_MAX];
    int all_n;
    int recent_buf[DIVOOM_RECENT_USED_CLOCK_MAX_NUM];
    int recent_n;

    if (!ph || !ids || max_n <= 0)
    {
        return 0;
    }

    tab = ph->active_tab;
    n = 0;
    all_n = divoom_light_app_copy_my_clock_list_limited(all_buf, FM_LIST_MAX);
    recent_n = divoom_light_app_copy_recent_clock_list_limited(recent_buf,
        DIVOOM_RECENT_USED_CLOCK_MAX_NUM);

    if (tab == DIVOOM_ASTRO_FACE_MARKET_TAB_ALL)
    {
        for (i = 0; i < all_n && n < max_n; i++)
        {
            fm_append_unique_clock_id(ids, &n, max_n, all_buf[i]);
        }
        return n;
    }

    if (tab == DIVOOM_ASTRO_FACE_MARKET_TAB_FAVORITE)
    {
        for (i = 0; i < recent_n && n < max_n; i++)
        {
            if (fm_clock_in_history(recent_buf[i])
                && divoom_light_app_is_fav_clock(recent_buf[i]))
            {
                fm_append_unique_clock_id(ids, &n, max_n, recent_buf[i]);
            }
        }

        for (i = 0; i < all_n && n < max_n; i++)
        {
            if (divoom_light_app_is_fav_clock(all_buf[i]))
            {
                fm_append_unique_clock_id(ids, &n, max_n, all_buf[i]);
            }
        }
        return n;
    }

    {
        for (i = 0; i < recent_n && n < max_n && n < FM_RECENT_MAX; i++)
        {
            if (fm_clock_in_history(recent_buf[i]))
            {
                fm_append_unique_clock_id(ids, &n, max_n, recent_buf[i]);
            }
        }
        return n;
    }
}

/* 仅本地 cfg / 相册名。不 wait 网络、不用商店名，避免切 tab 卡死。 */
static void fm_get_clock_name(int clock_id, char *buf, int buf_sz)
{
    Pdivoom_clock_manage_item pclock_info;
    Pdivoom_astrootoo_face_market_ctrl ph = s_fm_page;
    int lang = divoom_main_set_lang_flag(-1), i;

    if (!buf || buf_sz <= 0)
    {
        return;
    }
    buf[0] = '\0';
    if (clock_id <= 0)
    {
        return;
    }

    if (clock_id >= DIVOOM_PHOTO_START_CLOCK_ID)
    {
        char *name = divoom_pic_album_manage_get_static_album_name(clock_id);

        if (name && name[0] != '\0')
        {
            strncpy(buf, name, (size_t)buf_sz - (size_t)1);
            buf[buf_sz - 1] = '\0';
        }
        return;
    }

    if (ph && ph->name_cache)
    {
        if (ph->name_cache_lang != lang)
        {
            ph->name_cache_lang = lang;
            ph->name_cache_count = ph->name_cache_next = 0;
        }
        for (i = 0; i < ph->name_cache_count; i++)
        {
            if (ph->name_cache[i].clock_id == clock_id)
            {
                strncpy(buf, ph->name_cache[i].name, (size_t)buf_sz - 1);
                buf[buf_sz - 1] = '\0';
                return;
            }
        }
    }
    /* Reuse names captured by the initial scan instead of copying configs on each key. */
    pclock_info = divoom_clock_manage_get_raw_clock_info(clock_id, 0, 0);
    if (!pclock_info)
    {
        return;
    }
    if (divoom_main_set_lang_flag(-1) == 1 && pclock_info->name_cn[0] != '\0')
    {
        strncpy(buf, pclock_info->name_cn, (size_t)buf_sz - (size_t)1);
    }
    else if (pclock_info->name_en[0] != '\0')
    {
        strncpy(buf, pclock_info->name_en, (size_t)buf_sz - (size_t)1);
    }
    else if (pclock_info->name_cn[0] != '\0')
    {
        strncpy(buf, pclock_info->name_cn, (size_t)buf_sz - (size_t)1);
    }
    buf[buf_sz - 1] = '\0';
    divoom_clock_manage_free_clock(pclock_info);
    if (ph && ph->name_cache && buf[0])
    {
        fm_name_entry *entry = &ph->name_cache[ph->name_cache_next];
        entry->clock_id = clock_id;
        strncpy(entry->name, buf, sizeof(entry->name) - 1);
        entry->name[sizeof(entry->name) - 1] = '\0';
        ph->name_cache_next = (ph->name_cache_next + 1) % FM_LIST_MAX;
        if (ph->name_cache_count < FM_LIST_MAX) ph->name_cache_count++;
    }
}

static void fm_apply_slot(lv_obj_t *o, int focus)
{
    lv_obj_set_style_bg_color(o,
        lv_color_hex(focus ? DIVOOM_ASTRO_SURFACE_FOCUS : DIVOOM_ASTRO_SURFACE_IDLE), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(o,
        lv_color_hex(focus ? DIVOOM_ASTRO_ITEM_BORDER_ACTIVE : DIVOOM_ASTRO_STROKE), 0);
    lv_obj_set_style_border_width(o, focus ? DIVOOM_ASTRO_ITEM_BORDER_W_ACTIVE
        : DIVOOM_ASTRO_ITEM_BORDER_W_INACTIVE, 0);
    lv_obj_set_style_radius(o, DIVOOM_ASTRO_RADIUS_SM, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
}

static void fm_apply_tab_style(lv_obj_t *btn, int selected, int list_pane)
{
    if (!btn)
    {
        return;
    }
    if (selected && list_pane)
    {
        lv_obj_set_style_bg_color(btn, lv_color_hex(FM_TAB_IDLE_BG), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
        lv_obj_set_style_border_width(btn, 2, 0);
    }
    else if (selected)
    {
        lv_obj_set_style_bg_color(btn, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
    }
    else
    {
        lv_obj_set_style_bg_color(btn, lv_color_hex(FM_TAB_IDLE_BG), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
    }
}

static void fm_paint_strip(Pdivoom_astrootoo_face_market_ctrl ph)
{
    int i;

    for (i = 0; i < FM_TAB_COUNT; i++)
    {
        int selected = (i == ph->active_tab);
        int list_pane = (ph->focus_pane == FM_PANE_LIST);

        fm_apply_tab_style(ph->tab_btn[i], selected, selected && list_pane);
        if (ph->tab_label[i])
        {
            lv_obj_set_style_text_color(ph->tab_label[i],
                lv_color_hex((selected && !list_pane) ? 0x1F2A33 : 0xA2A2A2), 0);
        }
    }
}

static void fm_paint_row(Pdivoom_astrootoo_face_market_ctrl ph, int row)
{
    fm_row *r;
    int vis;
    int text_focus;
    int fav_focus;
    int delete_focus;

    vis = fm_visible_index(ph, row);
    if (vis < 0 || !ph->rows)
    {
        return;
    }
    r = &ph->rows[vis];
    text_focus = (ph->focus_pane == FM_PANE_LIST && row == ph->list_focus
        && ph->col_focus == FM_COL_TEXT);
    fav_focus = (ph->focus_pane == FM_PANE_LIST && row == ph->list_focus
        && ph->col_focus == FM_COL_FAVORITE);
    delete_focus = (ph->focus_pane == FM_PANE_LIST && row == ph->list_focus
        && ph->col_focus == FM_COL_DELETE);
    fm_apply_slot(r->text_cont, text_focus);
    fm_apply_slot(r->fav_cont, fav_focus);
    fm_apply_slot(r->delete_cont, delete_focus);
}

/*
 * 收藏星，28×28，对照 图片资源/收藏。
 * 实心 392 字节：每字两像素，高半字节在左。
 *   0 透明，1/2 两档黑边，3–14 不透明黑到 #FFD23F，15 高光。
 * 空心 98 字节：1 bit，高位在左，有点即不透明黑。
 */
static const uint32 fm_star_pal[16] = {
    0x000000, 0x000000, 0x000000, 0x000000,
    0x171306, 0x2E260B, 0x463911, 0x5D4C17,
    0x745F1D, 0x8B7322, 0xA28628, 0xB9992E,
    0xD1AC34, 0xE8BF39, 0xFFD23F, 0xFFF8E6,
};

static const uint8 fm_star_opa[16] = {
    0, 32, 224, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
};

static const uint8 fm_star_fill_bits[FM_STAR_SZ * (FM_STAR_SZ / 2)] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x22, 0x22, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x33, 0x33, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x43, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x23, 0x4C, 0xC4, 0x32, 0x10, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x23, 0x5D, 0xD5, 0x32, 0x10, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0xCE, 0xEC, 0x43, 0x21, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x12, 0x35, 0xDE, 0xED, 0x53, 0x21, 0x00, 0x00, 0x00, 0x00,
    0x11, 0x11, 0x11, 0x11, 0x23, 0x4C, 0xEE, 0xEE, 0xC4, 0x32, 0x11, 0x11, 0x11, 0x11,
    0x22, 0x22, 0x22, 0x22, 0x33, 0x5F, 0xFE, 0xEE, 0xD5, 0x33, 0x22, 0x22, 0x22, 0x22,
    0x33, 0x34, 0x44, 0x44, 0x45, 0xDF, 0xFE, 0xEE, 0xEC, 0x54, 0x44, 0x44, 0x43, 0x33,
    0x33, 0x4C, 0xDD, 0xDD, 0xDD, 0xFF, 0xEE, 0xEE, 0xEE, 0xDD, 0xDD, 0xDD, 0xC4, 0x33,
    0x23, 0x35, 0xDE, 0xEE, 0xEE, 0xFF, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xED, 0x53, 0x32,
    0x12, 0x33, 0x5C, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xC5, 0x33, 0x21,
    0x01, 0x23, 0x35, 0xCE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEC, 0x53, 0x32, 0x10,
    0x00, 0x12, 0x33, 0x5D, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xD5, 0x33, 0x21, 0x00,
    0x00, 0x01, 0x23, 0x4D, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xD4, 0x32, 0x10, 0x00,
    0x00, 0x00, 0x12, 0x4D, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xD4, 0x21, 0x00, 0x00,
    0x00, 0x00, 0x12, 0x4D, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xD4, 0x21, 0x00, 0x00,
    0x00, 0x00, 0x32, 0x4D, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xD4, 0x23, 0x00, 0x00,
    0x00, 0x01, 0x23, 0xED, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xDE, 0x32, 0x10, 0x00,
    0x00, 0x03, 0x2E, 0xED, 0xEE, 0xDD, 0xDD, 0xDD, 0xDD, 0xEE, 0xDE, 0xE2, 0x30, 0x00,
    0x00, 0x03, 0x2E, 0xEE, 0xDC, 0x54, 0x44, 0x44, 0x45, 0xCD, 0xEE, 0xE2, 0x30, 0x00,
    0x00, 0x03, 0x2E, 0xCC, 0xE4, 0x33, 0x22, 0x22, 0x33, 0x4E, 0xCC, 0xE2, 0x30, 0x00,
    0x00, 0x12, 0x3E, 0xE4, 0x33, 0x22, 0x11, 0x11, 0x22, 0x33, 0x4E, 0xE3, 0x21, 0x00,
    0x00, 0x12, 0x33, 0x33, 0x22, 0x11, 0x00, 0x00, 0x11, 0x22, 0x33, 0x33, 0x21, 0x00,
    0x00, 0x01, 0x23, 0x22, 0x11, 0x00, 0x00, 0x00, 0x00, 0x11, 0x22, 0x32, 0x10, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8 fm_star_hollow_bits[(FM_STAR_SZ * FM_STAR_SZ + 7) / 8] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x1F, 0x80, 0x00, 0x01, 0xF8, 0x00,
    0x00, 0x39, 0xC0, 0x00, 0x03, 0x9C, 0x00, 0x00, 0x70, 0xE0, 0x00, 0x07, 0x0E, 0x00,
    0x00, 0xE0, 0x70, 0x0F, 0xFE, 0x07, 0xFF, 0xFF, 0xC0, 0x3F, 0xFE, 0x00, 0x00, 0x07,
    0xF0, 0x00, 0x00, 0xF7, 0x80, 0x00, 0x1E, 0x3C, 0x00, 0x03, 0xC1, 0xE0, 0x00, 0x78,
    0x0E, 0x00, 0x07, 0x00, 0x60, 0x00, 0x60, 0x06, 0x00, 0x06, 0x00, 0xE0, 0x00, 0x70,
    0x0C, 0x00, 0x03, 0x01, 0x80, 0x00, 0x18, 0x18, 0x3F, 0xC1, 0x81, 0x87, 0xFE, 0x18,
    0x19, 0xF0, 0xF9, 0x81, 0xFC, 0x03, 0xF8, 0x0F, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00,
};

static void fm_star_span(lv_layer_t *layer, int x, int y, int w, uint32 color, uint8 opa)
{
    lv_draw_rect_dsc_t dsc;
    lv_area_t area;

    if (!layer || w <= 0 || opa == 0)
    {
        return;
    }
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = lv_color_hex(color);
    dsc.bg_opa = opa;
    dsc.border_width = 0;
    dsc.radius = 0;
    area.x1 = (lv_coord_t)x;
    area.y1 = (lv_coord_t)y;
    area.x2 = (lv_coord_t)(x + w - 1);
    area.y2 = (lv_coord_t)y;
    lv_draw_rect(layer, &dsc, &area);
}

static void fm_star_draw_fill(lv_layer_t *layer, int ox, int oy)
{
    int y;

    for (y = 0; y < FM_STAR_SZ; y++)
    {
        const uint8 *row = fm_star_fill_bits + y * (FM_STAR_SZ / 2);
        int x;
        int run_x = 0;
        int run = 0;
        uint8 prev = 0;

        for (x = 0; x < FM_STAR_SZ; x++)
        {
            uint8 packed = row[x >> 1];
            uint8 idx = (x & 1) ? (uint8)(packed & 0x0F) : (uint8)(packed >> 4);

            if (idx != 0 && idx == prev)
            {
                run++;
                continue;
            }
            if (prev != 0)
            {
                fm_star_span(layer, ox + run_x, oy + y, run,
                    fm_star_pal[prev], fm_star_opa[prev]);
            }
            prev = idx;
            run_x = x;
            run = 1;
        }
        if (prev != 0)
        {
            fm_star_span(layer, ox + run_x, oy + y, run,
                fm_star_pal[prev], fm_star_opa[prev]);
        }
    }
}

static void fm_star_draw_hollow(lv_layer_t *layer, int ox, int oy)
{
    int y;

    for (y = 0; y < FM_STAR_SZ; y++)
    {
        int x;
        int run_x = 0;
        int run = 0;

        for (x = 0; x < FM_STAR_SZ; x++)
        {
            int bit = y * FM_STAR_SZ + x;
            int on = (fm_star_hollow_bits[bit >> 3] & (uint8)(0x80 >> (bit & 7))) != 0;

            if (on)
            {
                if (run == 0)
                {
                    run_x = x;
                }
                run++;
                continue;
            }
            if (run > 0)
            {
                fm_star_span(layer, ox + run_x, oy + y, run, 0x000000, LV_OPA_COVER);
                run = 0;
            }
        }
        if (run > 0)
        {
            fm_star_span(layer, ox + run_x, oy + y, run, 0x000000, LV_OPA_COVER);
        }
    }
}

static void fm_star_draw_cb(lv_event_t *e)
{
    lv_obj_t *obj;
    lv_layer_t *layer;
    lv_area_t coords;

    if (lv_event_get_code(e) != LV_EVENT_DRAW_MAIN)
    {
        return;
    }
    obj = lv_event_get_target(e);
    layer = lv_event_get_layer(e);
    if (!obj || !layer)
    {
        return;
    }
    lv_obj_get_coords(obj, &coords);
    if (lv_obj_get_user_data(obj) != NULL)
    {
        fm_star_draw_fill(layer, coords.x1, coords.y1);
    }
    else
    {
        fm_star_draw_hollow(layer, coords.x1, coords.y1);
    }
}

static void fm_row_set_fav_icon(fm_row *r)
{
    if (!r || !r->fav_icon)
    {
        return;
    }
    lv_obj_set_user_data(r->fav_icon, r->is_favorite ? (void *)1 : NULL);
    lv_obj_invalidate(r->fav_icon);
}

static void fm_hide_modal(Pdivoom_astrootoo_face_market_ctrl ph)
{
    if (!ph)
    {
        return;
    }
    fm_confirm_close(ph);
    ph->modal_kind = FM_MODAL_NONE;
    if (ph->modal_overlay)
    {
        lv_obj_del(ph->modal_overlay);
        ph->modal_overlay = NULL;
        ph->modal_btn = NULL;
    }
    if (ph->focus_pane == FM_PANE_LIST && ph->row_count > 0)
    {
        int vis = fm_visible_index(ph, ph->list_focus);

        if (vis >= 0 && ph->rows && ph->rows[vis].btn)
        {
            lv_group_focus_obj(ph->rows[vis].btn);
            return;
        }
    }
    fm_focus_strip(ph);
}

static void fm_modal_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_face_market_ctrl ph;
    uint8_t key_code;

    ph = (Pdivoom_astrootoo_face_market_ctrl)lv_event_get_user_data(e);
    if (!ph || lv_event_get_code(e) != LV_EVENT_KEY)
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
        fm_hide_modal(ph);
    }
}

static void fm_show_modal(Pdivoom_astrootoo_face_market_ctrl ph, int kind)
{
    lv_obj_t *frame;
    lv_obj_t *actions;

    if (!ph || !ph->root_obj || kind == FM_MODAL_NONE)
    {
        return;
    }

    fm_hide_modal(ph);
    ph->modal_kind = kind;

    ph->modal_overlay = divoom_astrootoo_ui_alert_modal_create(ph->root_obj, &frame);
    if (!ph->modal_overlay || !frame)
    {
        ph->modal_kind = FM_MODAL_NONE;
        return;
    }

    divoom_astrootoo_ui_alert_modal_add_title(frame,
        divoom_menu_control_get_disp_str("WatchFaceKeepMinOneTitle"), ph->menu_id);
    divoom_astrootoo_ui_alert_modal_add_body(frame,
        divoom_menu_control_get_disp_str("WatchFaceKeepMinOneBody"), ph->menu_id);

    actions = lv_obj_create(frame);
    lv_obj_remove_style_all(actions);
    lv_obj_set_width(actions, LV_PCT(100));
    lv_obj_set_height(actions, LV_SIZE_CONTENT);
    lv_obj_clear_flag(actions, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    ph->modal_btn = divoom_astrootoo_ui_alert_modal_make_ok_btn(actions,
        divoom_menu_control_get_disp_str("WatchFaceStorageFullOk"), ph->menu_id,
        fm_modal_key_cb, ph);

    divoom_astrootoo_ui_confirm_modal_layout(frame);
}

static int fm_dialog_blocks_keys(Pdivoom_astrootoo_face_market_ctrl ph)
{
    return ph && (ph->modal_kind != FM_MODAL_NONE || ph->confirm_overlay);
}

static void fm_confirm_restore_list_focus(Pdivoom_astrootoo_face_market_ctrl ph)
{
    lv_group_t *g;
    int i;
    int vis;

    if (!ph)
    {
        return;
    }
    g = lv_group_get_default();
    if (g)
    {
        for (i = 0; i < FM_TAB_COUNT; i++)
        {
            if (ph->tab_btn[i] && lv_obj_get_group(ph->tab_btn[i]) == NULL)
            {
                lv_group_add_obj(g, ph->tab_btn[i]);
            }
        }
    }
    if (ph->focus_pane == FM_PANE_LIST && ph->rows && ph->visible_count > 0)
    {
        for (i = 0; i < ph->visible_count; i++)
        {
            if (ph->rows[i].btn && g && lv_obj_get_group(ph->rows[i].btn) == NULL)
            {
                lv_group_add_obj(g, ph->rows[i].btn);
            }
        }
        vis = fm_visible_index(ph, ph->list_focus);
        if (vis >= 0 && ph->rows[vis].btn)
        {
            lv_group_focus_obj(ph->rows[vis].btn);
            return;
        }
    }
    fm_focus_strip(ph);
}

static void fm_confirm_close(Pdivoom_astrootoo_face_market_ctrl ph)
{
    if (!ph || !ph->confirm_overlay)
    {
        return;
    }
    if (ph->confirm_cancel_btn)
    {
        lv_group_remove_obj(ph->confirm_cancel_btn);
    }
    if (ph->confirm_ok_btn)
    {
        lv_group_remove_obj(ph->confirm_ok_btn);
    }
    lv_obj_del(ph->confirm_overlay);
    ph->confirm_overlay = NULL;
    ph->confirm_cancel_btn = NULL;
    ph->confirm_ok_btn = NULL;
    ph->confirm_delete_row = -1;
    fm_confirm_restore_list_focus(ph);
}

static void fm_confirm_refresh(Pdivoom_astrootoo_face_market_ctrl ph)
{
    int ok_focus;
    lv_obj_t *cancel_lbl;
    lv_obj_t *ok_lbl;

    if (!ph || !ph->confirm_cancel_btn || !ph->confirm_ok_btn)
    {
        return;
    }
    ok_focus = (ph->confirm_focus == 1);

    lv_obj_set_style_bg_opa(ph->confirm_cancel_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(ph->confirm_cancel_btn,
        lv_color_hex(ok_focus ? DIVOOM_ASTRO_SURFACE_IDLE : DIVOOM_ASTRO_SURFACE_FOCUS), 0);
    lv_obj_set_style_border_width(ph->confirm_cancel_btn, 0, 0);

    if (ok_focus)
    {
        lv_obj_set_style_bg_opa(ph->confirm_ok_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(ph->confirm_ok_btn, lv_color_hex(DIVOOM_ASTRO_DANGER), 0);
        lv_obj_set_style_border_width(ph->confirm_ok_btn, 0, 0);
    }
    else
    {
        lv_obj_set_style_bg_opa(ph->confirm_ok_btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(ph->confirm_ok_btn, 2, 0);
        lv_obj_set_style_border_color(ph->confirm_ok_btn, lv_color_hex(DIVOOM_ASTRO_DANGER), 0);
    }

    cancel_lbl = lv_obj_get_child(ph->confirm_cancel_btn, 0);
    ok_lbl = lv_obj_get_child(ph->confirm_ok_btn, 0);
    if (cancel_lbl)
    {
        lv_obj_set_style_text_color(cancel_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    }
    if (ok_lbl)
    {
        lv_obj_set_style_text_color(ok_lbl,
            lv_color_hex(ok_focus ? DIVOOM_ASTRO_BG : DIVOOM_ASTRO_DANGER), 0);
    }
    lv_group_focus_obj(ok_focus ? ph->confirm_ok_btn : ph->confirm_cancel_btn);
    divoom_astrootoo_ui_confirm_modal_sync_btn_labels(ph->confirm_cancel_btn,
        ph->confirm_ok_btn, ok_focus);
}

static void fm_confirm_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_face_market_ctrl ph;
    uint8_t key_code;
    int row;
    int do_commit;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    ph = (Pdivoom_astrootoo_face_market_ctrl)lv_event_get_user_data(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    if (!ph || !ph->confirm_overlay)
    {
        return;
    }

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        fm_confirm_close(ph);
        return;
    }
    if (divoom_astrootoo_ui_key_is_left(key_code) || divoom_astrootoo_ui_key_is_right(key_code)
        || divoom_astrootoo_ui_key_is_prev(key_code) || divoom_astrootoo_ui_key_is_next(key_code))
    {
        ph->confirm_focus = (ph->confirm_focus == 0) ? 1 : 0;
        fm_confirm_refresh(ph);
        return;
    }
    if (!divoom_astrootoo_ui_key_is_enter(key_code))
    {
        return;
    }

    row = ph->confirm_delete_row;
    do_commit = (ph->confirm_focus == 1);
    fm_confirm_close(ph);
    if (do_commit)
    {
        fm_delete_row_commit(ph, row);
    }
}

static lv_obj_t *fm_confirm_make_btn(Pdivoom_astrootoo_face_market_ctrl ph, lv_obj_t *parent,
    const char *label_key)
{
    return divoom_astrootoo_ui_confirm_modal_make_btn(parent,
        divoom_menu_control_get_disp_str(label_key), ph->menu_id, 0,
        fm_confirm_key_cb, ph, 1);
}

static void fm_show_delete_confirm(Pdivoom_astrootoo_face_market_ctrl ph, int row)
{
    lv_obj_t *frame;
    lv_obj_t *title;
    lv_obj_t *body;
    lv_obj_t *actions;

    if (!ph || !ph->root_obj || row < 0 || row >= ph->row_count)
    {
        return;
    }

    fm_confirm_close(ph);
    ph->confirm_delete_row = row;
    ph->confirm_focus = 0;
    {
        lv_group_t *g = lv_group_get_default();

        if (g)
        {
            lv_group_remove_all_objs(g);
        }
    }

    ph->confirm_overlay = divoom_astrootoo_ui_confirm_modal_create(ph->root_obj, &frame);
    if (!ph->confirm_overlay || !frame)
    {
        ph->confirm_delete_row = -1;
        return;
    }

    title = lv_label_create(frame);
    lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP);
    lv_label_set_text(title, divoom_menu_control_get_disp_str("WatchFaceDeleteConfirmTitle"));
    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(30, ph->menu_id), 0);
    lv_obj_set_style_pad_bottom(title, 10, 0);
    lv_obj_set_style_border_side(title, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(title, 2, 0);
    lv_obj_set_style_border_color(title, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);

    body = lv_label_create(frame);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_label_set_text(body, divoom_menu_control_get_disp_str("WatchFaceDeleteConfirmBody"));
    lv_obj_set_width(body, LV_PCT(100));
    lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(body, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(body, divoom_menu_control_get_font_info(24, ph->menu_id), 0);

    actions = lv_obj_create(frame);
    lv_obj_remove_style_all(actions);
    lv_obj_set_width(actions, LV_PCT(100));
    lv_obj_set_height(actions, LV_SIZE_CONTENT);
    lv_obj_clear_flag(actions, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(actions, 8, 0);

    ph->confirm_cancel_btn = fm_confirm_make_btn(ph, actions, "Cancel");
    ph->confirm_ok_btn = fm_confirm_make_btn(ph, actions, "Remove");
    divoom_astrootoo_ui_confirm_modal_layout(frame);
    fm_confirm_refresh(ph);
}

static int fm_modal_blocks_keys(Pdivoom_astrootoo_face_market_ctrl ph)
{
    return fm_dialog_blocks_keys(ph);
}

static void fm_stop_scan(Pdivoom_astrootoo_face_market_ctrl ph)
{
    if (!ph || !ph->scan_timer)
    {
        return;
    }
    lv_timer_del(ph->scan_timer);
    ph->scan_timer = NULL;
}

static void fm_compact_named(Pdivoom_astrootoo_face_market_ctrl ph)
{
    int i;
    int n;
    int out;

    if (!ph || !ph->source_clock_ids || !ph->clock_ids || !ph->named_mask)
    {
        if (ph)
        {
            ph->row_count = 0;
        }
        return;
    }

    n = ph->built_count;
    if (n < 0)
    {
        n = 0;
    }
    if (n > FM_LIST_MAX)
    {
        n = FM_LIST_MAX;
    }

    out = 0;
    for (i = 0; i < n; i++)
    {
        if (ph->named_mask[i] == 0)
        {
            continue;
        }
        ph->clock_ids[out] = ph->source_clock_ids[i];
        out++;
    }
    ph->row_count = out;
}

static void fm_sync_visible_local(Pdivoom_astrootoo_face_market_ctrl ph)
{
    int need;

    if (!ph || ph->row_count <= 0)
    {
        return;
    }
    need = ph->row_count;
    if (need > FM_VISIBLE_MAX)
    {
        need = FM_VISIBLE_MAX;
    }
    if (ph->rows_rebind || !ph->rows || ph->visible_count < need)
    {
        fm_rebuild_visible_rows(ph);
    }
}

static void fm_scan_named_batch(Pdivoom_astrootoo_face_market_ctrl ph, int max_n)
{
    int n;
    int end;
    char name[FM_NAME_MAX];

    if (!ph || !ph->source_clock_ids || !ph->named_mask || max_n <= 0)
    {
        return;
    }

    n = ph->built_count;
    if (n < 0)
    {
        n = 0;
    }
    if (ph->scan_idx < 0)
    {
        ph->scan_idx = 0;
    }
    if (ph->scan_idx >= n)
    {
        return;
    }

    end = ph->scan_idx + max_n;
    if (end > n)
    {
        end = n;
    }
    for (; ph->scan_idx < end; ph->scan_idx++)
    {
        int cid = ph->source_clock_ids[ph->scan_idx];

        name[0] = '\0';
        fm_get_clock_name(cid, name, (int)sizeof(name));
        if (name[0] != '\0')
        {
            ph->named_mask[ph->scan_idx] = 1;
        }
        else
        {
            ph->named_mask[ph->scan_idx] = 0;
        }
    }
}

static void fm_cancel_thumb_settle(Pdivoom_astrootoo_face_market_ctrl ph)
{
    if (!ph)
    {
        return;
    }
    if (ph->thumb_settle_timer)
    {
        lv_timer_del(ph->thumb_settle_timer);
        ph->thumb_settle_timer = NULL;
    }
    divoom_face_preview_publish(ph->preview_loader, NULL, NULL, 0, 1);
}

static void fm_thumb_settle_cb(lv_timer_t *t)
{
    Pdivoom_astrootoo_face_market_ctrl ph = lv_timer_get_user_data(t);
    divoom_face_preview_result *result;
    int i;
    if (!ph) return;
    ph->thumb_settle_timer = NULL;
    if (!ph->root_obj || !ph->clock_ids || !ph->rows || ph->visible_count <= 0) return;
    if (ph->load_busy || ph->scan_timer) { fm_arm_thumb_settle(ph); return; }
    result = divoom_face_preview_take(ph->preview_loader);
    if (result)
    {
        for (i = 0; i < ph->visible_count; i++)
        {
            fm_row *r = &ph->rows[i];
            if (r->clock_id != result->clock_id || r->thumb_ready || !r->thumb_img) continue;
            lv_img_set_src(r->thumb_img, &result->image);
            fm_row_release_decoded(r);
            r->decoded = result;
#if FACE_PREVIEW_PERF
            ph->thumb_binds++;
#endif
            r->thumb_ready = 1;
            lv_obj_clear_flag(r->thumb_img, LV_OBJ_FLAG_HIDDEN);
            result = NULL;
            break;
        }
        divoom_system_free(result);
    }
    fm_publish_thumb_window(ph, 0);
    ph->thumb_settle_timer = lv_timer_create(fm_thumb_settle_cb, FM_THUMB_BIND_MS, ph);
    if (ph->thumb_settle_timer) lv_timer_set_repeat_count(ph->thumb_settle_timer, 1);
}


static void fm_publish_thumb_window(Pdivoom_astrootoo_face_market_ctrl ph, int activity)
{
    int i, count = 0, ids[FACE_PREVIEW_WINDOW];
    unsigned char ready[FACE_PREVIEW_WINDOW];
    if (ph->rows && ph->clock_ids && !ph->load_busy && !ph->scan_timer)
    {
        count = ph->visible_count;
        if (count > FACE_PREVIEW_WINDOW) count = FACE_PREVIEW_WINDOW;
        for (i = 0; i < count; i++)
        {
            ids[i] = ph->clock_ids[ph->visible_start + i];
            ready[i] = ph->rows[i].thumb_ready;
        }
    }
    divoom_face_preview_publish(ph->preview_loader, ids, ready, count, activity);
}

static void fm_row_release_decoded(fm_row *r)
{
    if (!r->decoded) return;
    lv_img_cache_invalidate_src(&r->decoded->image);
    divoom_system_free(r->decoded);
    r->decoded = NULL;
}

static void fm_arm_thumb_settle(Pdivoom_astrootoo_face_market_ctrl ph)
{
    if (!ph || ph->root_obj == NULL || ph->row_count <= 0 || !ph->rows)
    {
        return;
    }
    fm_publish_thumb_window(ph, 1);
    if (ph->thumb_settle_timer)
    {
        lv_timer_del(ph->thumb_settle_timer);
        ph->thumb_settle_timer = NULL;
    }
    ph->thumb_settle_timer = lv_timer_create(fm_thumb_settle_cb, FM_THUMB_SETTLE_MS, ph);
    if (ph->thumb_settle_timer)
    {
        lv_timer_set_repeat_count(ph->thumb_settle_timer, 1);
    }
}

static void fm_commit_list(Pdivoom_astrootoo_face_market_ctrl ph)
{
    if (!ph)
    {
        return;
    }

    fm_stop_scan(ph);
    ph->load_busy = 0;

    fm_compact_named(ph);
    if (ph->named_mask)
    {
        divoom_system_free(ph->named_mask);
        ph->named_mask = NULL;
    }

    if (ph->row_count <= 0)
    {
        if (ph->clock_ids)
        {
            divoom_system_free(ph->clock_ids);
            ph->clock_ids = NULL;
        }
        fm_show_empty(ph);
        if (ph->thumb_settle_timer)
        {
            lv_timer_del(ph->thumb_settle_timer);
            ph->thumb_settle_timer = NULL;
        }
        return;
    }

    if (ph->rows_rebind || !ph->rows || ph->visible_count <= 0)
    {
        fm_rebuild_visible_rows(ph);
    }
    fm_arm_thumb_settle(ph);
}

static void fm_scan_advance(Pdivoom_astrootoo_face_market_ctrl ph)
{
    int n;
    if (!ph) return;
    n = ph->built_count;
    if (n < 0) n = 0;
    if (ph->scan_idx < n) fm_scan_named_batch(ph, FM_SCAN_BATCH);
    /* Build from local names only; missing configs do not start a network wait/retry. */
    fm_compact_named(ph);
    fm_sync_visible_local(ph);
    if (ph->scan_idx >= n) fm_commit_list(ph);
}

static void fm_scan_cb(lv_timer_t *t)
{
    Pdivoom_astrootoo_face_market_ctrl ph;

    ph = (Pdivoom_astrootoo_face_market_ctrl)lv_timer_get_user_data(t);
    if (!ph || ph->root_obj == NULL)
    {
        return;
    }
    fm_scan_advance(ph);
}

static void fm_start_scan(Pdivoom_astrootoo_face_market_ctrl ph)
{
    if (!ph || ph->scan_timer)
    {
        return;
    }
    ph->scan_timer = lv_timer_create(fm_scan_cb, FM_SCAN_PERIOD_MS, ph);
}

static void fm_stop_timers(Pdivoom_astrootoo_face_market_ctrl ph)
{
    if (!ph)
    {
        return;
    }
    if (ph->build_timer)
    {
        lv_timer_del(ph->build_timer);
        ph->build_timer = NULL;
    }
    fm_stop_scan(ph);
    if (ph->refresh_timer)
    {
        lv_timer_del(ph->refresh_timer);
        ph->refresh_timer = NULL;
    }
    if (ph->thumb_settle_timer)
    {
        lv_timer_del(ph->thumb_settle_timer);
        ph->thumb_settle_timer = NULL;
    }
}

static void fm_clear_visible_rows(Pdivoom_astrootoo_face_market_ctrl ph)
{
    int i;
    lv_group_t *grp;

    if (!ph)
    {
        return;
    }
    if (ph->list_direct)
    {
        divoom_menu_direct_ctrl_destory(ph->list_direct);
        ph->list_direct = NULL;
    }
    grp = lv_group_get_default();
    if (ph->rows && ph->row_slots > 0)
    {
        for (i = 0; i < ph->row_slots; i++)
        {
            if (grp && ph->rows[i].btn)
            {
                lv_group_remove_obj(ph->rows[i].btn);
            }
            /* 先解开相册共用缓冲和预览 jpg，再删控件。 */
            if (ph->rows[i].thumb_img)
            {
                lv_img_set_src(ph->rows[i].thumb_img, NULL);
            }
            fm_row_release_decoded(&ph->rows[i]);
        }
    }
    if (ph->content)
    {
        lv_obj_scroll_to_y(ph->content, 0, LV_ANIM_OFF);
        lv_obj_clean(ph->content);
    }
    ph->empty_label = NULL;
    if (ph->rows)
    {
        divoom_system_free(ph->rows);
        ph->rows = NULL;
    }
    ph->visible_start = 0;
    ph->visible_count = 0;
    ph->row_slots = 0;
    ph->rows_rebind = 0;
}

static void fm_release_list_data(Pdivoom_astrootoo_face_market_ctrl ph)
{
    if (!ph)
    {
        return;
    }

    fm_stop_scan(ph);
    fm_cancel_thumb_settle(ph);
    if (ph->clock_ids)
    {
        divoom_system_free(ph->clock_ids);
        ph->clock_ids = NULL;
    }
    if (ph->source_clock_ids)
    {
        divoom_system_free(ph->source_clock_ids);
        ph->source_clock_ids = NULL;
    }
    if (ph->named_mask)
    {
        divoom_system_free(ph->named_mask);
        ph->named_mask = NULL;
    }
    ph->row_count = 0;
    ph->scan_idx = 0;
    ph->load_busy = 0;
    ph->list_focus = 0;
    ph->col_focus = FM_COL_TEXT;
    ph->visible_start = 0;
    if (ph->rows)
    {
        ph->rows_rebind = 1;
    }
    if (ph->content)
    {
        lv_obj_scroll_to_y(ph->content, 0, LV_ANIM_OFF);
    }
}

static void fm_free_list(Pdivoom_astrootoo_face_market_ctrl ph)
{
    if (!ph)
    {
        return;
    }

    fm_release_list_data(ph);
    fm_clear_visible_rows(ph);
}

static void fm_show_empty(Pdivoom_astrootoo_face_market_ctrl ph)
{
    lv_obj_t *lab;
    const char *text;

    if (!ph || !ph->content)
    {
        return;
    }
    fm_hide_kept_rows(ph);
    text = divoom_menu_control_get_disp_str(fm_empty_label_for_tab(ph->active_tab));
    if (ph->empty_label)
    {
        lv_label_set_text(ph->empty_label, text);
        lv_obj_clear_flag(ph->empty_label, LV_OBJ_FLAG_HIDDEN);
        fm_focus_strip(ph);
        return;
    }

    lab = lv_label_create(ph->content);
    lv_label_set_text(lab, text);
    lv_obj_set_style_text_color(lab, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(lab, divoom_menu_control_get_font_info(20, ph->menu_id), 0);
    lv_obj_align(lab, LV_ALIGN_TOP_MID, 0, 24);
    ph->empty_label = lab;
    /* 无列表行可聚焦（含收藏 Tab 内取消最后一项收藏）；统一回到 Tab 条 */
    fm_focus_strip(ph);
}

/* Native 60x60 default, with no file/config reads in the row-recycle/key path. */
static void fm_row_set_thumb(Pdivoom_astrootoo_face_market_ctrl ph, fm_row *r)
{
    const lv_img_dsc_t *image;
    if (!r || !r->thumb_img) return;
    r->thumb_ready = 0;
    image = divoom_dial_preview_default_acquire(&ph->default_preview);
    if (image)
    {
        if (lv_img_get_src(r->thumb_img) != image) lv_img_set_src(r->thumb_img, image);
        lv_obj_clear_flag(r->thumb_img, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_img_set_src(r->thumb_img, NULL);
        lv_obj_add_flag(r->thumb_img, LV_OBJ_FLAG_HIDDEN);
    }
    fm_row_release_decoded(r);
}

/* 截图 76 高里两行顶到顶是 26 像素，列表行高 72，按比例约 25。 */
#define FM_NAME_FONT_SZ           22
#define FM_NAME_LINE_PITCH        25

/* 22 号。单行放得下就一行并垂直居中；超出行宽则折成两行，不再横向轮动。 */
static void fm_apply_name_label(lv_obj_t *label, int menu_id)
{
    const lv_font_t *font;
    const char *txt;
    lv_point_t size;
    lv_coord_t line_h;
    lv_coord_t line_space;
    lv_coord_t box_h;
    lv_coord_t letter_space;

    if (!label)
    {
        return;
    }
    lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(FM_NAME_FONT_SZ, menu_id), 0);
    font = lv_obj_get_style_text_font(label, 0);
    line_h = font ? (lv_coord_t)lv_font_get_line_height(font) : FM_NAME_FONT_SZ;
    /* 步进 = line_height + line_space。字体行高比 25 大时往上收。 */
    line_space = 0;
    if (line_h > FM_NAME_LINE_PITCH)
    {
        line_space = (lv_coord_t)(FM_NAME_LINE_PITCH - line_h);
    }
    lv_obj_set_style_text_line_space(label, line_space, 0);
    letter_space = lv_obj_get_style_text_letter_space(label, 0);
    txt = lv_label_get_text(label);
    lv_obj_set_width(label, FM_ROW_LABEL_W);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    box_h = line_h;
    if (txt && txt[0] && font)
    {
        lv_txt_get_size(&size, txt, font, letter_space, line_space, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        if (size.x > FM_ROW_LABEL_W)
        {
            box_h = (lv_coord_t)(line_h * 2 + line_space);
            if (box_h > FM_ROW_H)
            {
                box_h = FM_ROW_H;
            }
        }
    }
    lv_obj_set_height(label, box_h);
    lv_obj_set_style_max_height(label, box_h, 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, FM_LABEL_X, 0);
}

static void fm_create_row(Pdivoom_astrootoo_face_market_ctrl ph, int visible_idx, int global_idx, int clock_id)
{
    fm_row *r;
    lv_obj_t *btn;
    lv_coord_t y;
    lv_coord_t x_fav;
    lv_coord_t x_del;
    char buf[64];
    int cur_clock_id;

    r = &ph->rows[visible_idx];
    r->decoded = NULL;
    y = visible_idx * (FM_ROW_H + FM_ROW_GAP);
    x_fav = FM_TEXT_W + FM_COL_GAP;
    x_del = x_fav + FM_ACTION_W + FM_COL_GAP;
    cur_clock_id = divoom_light_app_get_cur_clock_id();

    btn = lv_btn_create(ph->content);
    lv_obj_add_style(btn, &ph->ui.content_style, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_size(btn, FM_ROW_W, FM_ROW_H);
    lv_obj_set_pos(btn, FM_LIST_PAD_X, y);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    r->text_cont = lv_obj_create(btn);
    fm_apply_slot(r->text_cont, 0);
    lv_obj_set_size(r->text_cont, FM_TEXT_W, FM_ROW_H);
    lv_obj_set_pos(r->text_cont, 0, 0);
    lv_obj_clear_flag(r->text_cont, LV_OBJ_FLAG_SCROLLABLE);

    r->thumb = lv_obj_create(r->text_cont);
    lv_obj_set_size(r->thumb, FM_THUMB_SZ, FM_THUMB_SZ);
    lv_obj_align(r->thumb, LV_ALIGN_LEFT_MID, FM_THUMB_PAD, 0);
    lv_obj_set_style_bg_color(r->thumb, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(r->thumb, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(r->thumb, 0, 0);
    lv_obj_set_style_radius(r->thumb, 8, 0);
    lv_obj_set_style_pad_all(r->thumb, 0, 0);
    lv_obj_set_style_clip_corner(r->thumb, true, 0);
    lv_obj_clear_flag(r->thumb, LV_OBJ_FLAG_SCROLLABLE);
    r->thumb_img = lv_img_create(r->thumb);
    lv_obj_set_pos(r->thumb_img, 0, 0);
    lv_obj_add_flag(r->thumb_img, LV_OBJ_FLAG_HIDDEN);

    fm_get_clock_name(clock_id, buf, (int)sizeof(buf));
    r->name_label = lv_label_create(r->text_cont);
    lv_label_set_text(r->name_label, buf);
    lv_obj_set_style_text_color(r->name_label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    fm_apply_name_label(r->name_label, ph->menu_id);

    r->in_use_icon = lv_img_create(r->text_cont);
    lv_obj_set_size(r->in_use_icon, FM_ACT_ICON_SZ, FM_ACT_ICON_SZ);
    divoom_menu_control_lv_img_set_src(r->in_use_icon, FM_IN_USE_ICON);
    lv_obj_align(r->in_use_icon, LV_ALIGN_RIGHT_MID, -FM_LABEL_RIGHT_PAD, 0);
    if (clock_id != cur_clock_id)
    {
        lv_obj_add_flag(r->in_use_icon, LV_OBJ_FLAG_HIDDEN);
    }

    r->fav_cont = lv_obj_create(btn);
    fm_apply_slot(r->fav_cont, 0);
    lv_obj_set_size(r->fav_cont, FM_ACTION_W, FM_ROW_H);
    lv_obj_set_pos(r->fav_cont, x_fav, 0);
    lv_obj_clear_flag(r->fav_cont, LV_OBJ_FLAG_SCROLLABLE);

    r->fav_icon = lv_obj_create(r->fav_cont);
    lv_obj_remove_style_all(r->fav_icon);
    lv_obj_set_size(r->fav_icon, FM_STAR_SZ, FM_STAR_SZ);
    lv_obj_set_style_bg_opa(r->fav_icon, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(r->fav_icon, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(r->fav_icon, fm_star_draw_cb, LV_EVENT_DRAW_MAIN, NULL);
    lv_obj_center(r->fav_icon);

    r->delete_cont = lv_obj_create(btn);
    fm_apply_slot(r->delete_cont, 0);
    lv_obj_set_size(r->delete_cont, FM_ACTION_W, FM_ROW_H);
    lv_obj_set_pos(r->delete_cont, x_del, 0);
    lv_obj_clear_flag(r->delete_cont, LV_OBJ_FLAG_SCROLLABLE);

    r->delete_icon = lv_img_create(r->delete_cont);
    lv_obj_set_size(r->delete_icon, 28, 28);
    divoom_menu_control_lv_img_set_src(r->delete_icon, FM_DELETE_ICON);
    lv_obj_center(r->delete_icon);

    r->clock_id = clock_id;
    r->is_favorite = (uint8)divoom_light_app_is_fav_clock(clock_id);
    fm_row_set_thumb(ph, r);
    fm_row_set_fav_icon(r);

    lv_obj_add_event_cb(btn, fm_list_key_cb, LV_EVENT_KEY, ph);
    lv_group_add_obj(lv_group_get_default(), btn);
    r->btn = btn;
    (void)global_idx;
}

static void fm_place_row(const fm_row *r, int vis)
{
    if (!r || !r->btn)
    {
        return;
    }
    lv_obj_set_pos(r->btn, FM_LIST_PAD_X, vis * (FM_ROW_H + FM_ROW_GAP));
}

static void fm_sync_list_direct(Pdivoom_astrootoo_face_market_ctrl ph)
{
    int i;

    if (!ph || !ph->list_direct || !ph->rows)
    {
        return;
    }
    for (i = 0; i < ph->visible_count; i++)
    {
        divoom_menu_direct_ctrl_update(ph->list_direct, ph->rows[i].btn, 0, i);
    }
}

/* 窗口挪 1 行时，整行控件跟着走，只把滚出的那一行留到另一头再重绑。 */
static void fm_rotate_rows(Pdivoom_astrootoo_face_market_ctrl ph, int delta)
{
    fm_row saved;
    int i;
    int n;

    if (!ph || !ph->rows)
    {
        return;
    }
    n = ph->visible_count;
    if (n <= 1)
    {
        return;
    }
    if (delta == 1)
    {
        saved = ph->rows[0];
        for (i = 0; i < n - 1; i++)
        {
            ph->rows[i] = ph->rows[i + 1];
        }
        ph->rows[n - 1] = saved;
    }
    else
    {
        saved = ph->rows[n - 1];
        for (i = n - 1; i > 0; i--)
        {
            ph->rows[i] = ph->rows[i - 1];
        }
        ph->rows[0] = saved;
    }
    for (i = 0; i < n; i++)
    {
        fm_place_row(&ph->rows[i], i);
    }
    fm_sync_list_direct(ph);
}

static void fm_bind_row_content(Pdivoom_astrootoo_face_market_ctrl ph, int vis, int clock_id)
{
    fm_row *r;
    char buf[64];
    int cur_clock_id;

    if (!ph || !ph->rows || vis < 0 || vis >= ph->visible_count)
    {
        return;
    }
    r = &ph->rows[vis];
    if (!r->name_label)
    {
        return;
    }

    if (r->clock_id != clock_id)
    {
        buf[0] = '\0';
        fm_get_clock_name(clock_id, buf, (int)sizeof(buf));
        lv_label_set_text(r->name_label, buf);
        fm_apply_name_label(r->name_label, ph->menu_id);
        r->clock_id = clock_id;
        fm_row_set_thumb(ph, r);
    }

    r->is_favorite = (uint8)divoom_light_app_is_fav_clock(clock_id);
    fm_row_set_fav_icon(r);
    cur_clock_id = divoom_light_app_get_cur_clock_id();
    if (r->in_use_icon)
    {
        if (clock_id == cur_clock_id)
        {
            lv_obj_clear_flag(r->in_use_icon, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(r->in_use_icon, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/* 窗口只挪 1 行时复用 8 个控件，只取滚入那条的名；跳窗则原地换内容。 */
static int fm_try_reuse_visible_rows(Pdivoom_astrootoo_face_market_ctrl ph, int new_start, int new_count)
{
    int i;
    int delta;

    if (!ph || !ph->rows || !ph->clock_ids || new_count <= 0)
    {
        return 0;
    }
    if (ph->visible_count != new_count)
    {
        return 0;
    }

    delta = new_start - ph->visible_start;
    if (delta == 0)
    {
        return 1;
    }
    if (delta == 1 || delta == -1)
    {
        fm_rotate_rows(ph, delta);
        ph->visible_start = new_start;
        if (delta == 1)
        {
            fm_bind_row_content(ph, new_count - 1, ph->clock_ids[new_start + new_count - 1]);
        }
        else
        {
            fm_bind_row_content(ph, 0, ph->clock_ids[new_start]);
        }
        return 1;
    }

    ph->visible_start = new_start;
    for (i = 0; i < new_count; i++)
    {
        fm_bind_row_content(ph, i, ph->clock_ids[new_start + i]);
    }
    return 1;
}

static int fm_visible_index(Pdivoom_astrootoo_face_market_ctrl ph, int row)
{
    if (!ph || row < ph->visible_start || row >= ph->visible_start + ph->visible_count)
    {
        return -1;
    }
    return row - ph->visible_start;
}

static int fm_calc_visible_start(Pdivoom_astrootoo_face_market_ctrl ph, int focus)
{
    int max_start;
    int start;

    if (!ph || ph->row_count <= FM_VISIBLE_MAX)
    {
        return 0;
    }

    start = focus - FM_VISIBLE_FOCUS_BEFORE;
    max_start = ph->row_count - FM_VISIBLE_MAX;
    if (start < 0)
    {
        start = 0;
    }
    if (start > max_start)
    {
        start = max_start;
    }
    return start;
}

static void fm_drop_empty_label(Pdivoom_astrootoo_face_market_ctrl ph)
{
    if (!ph || !ph->empty_label)
    {
        return;
    }
    lv_obj_del(ph->empty_label);
    ph->empty_label = NULL;
}

static void fm_hide_one_row(fm_row *r)
{
    lv_group_t *grp;

    if (!r || !r->btn)
    {
        return;
    }
    grp = lv_group_get_default();
    if (grp && lv_obj_get_group(r->btn) == grp)
    {
        lv_group_remove_obj(r->btn);
    }
    lv_obj_add_flag(r->btn, LV_OBJ_FLAG_HIDDEN);
}

static void fm_show_one_row(fm_row *r)
{
    lv_group_t *grp;

    if (!r || !r->btn)
    {
        return;
    }
    lv_obj_clear_flag(r->btn, LV_OBJ_FLAG_HIDDEN);
    grp = lv_group_get_default();
    if (grp && lv_obj_get_group(r->btn) == NULL)
    {
        lv_group_add_obj(grp, r->btn);
    }
}

static void fm_hide_kept_rows(Pdivoom_astrootoo_face_market_ctrl ph)
{
    int i;

    if (!ph || !ph->rows)
    {
        return;
    }
    for (i = 0; i < ph->row_slots; i++)
    {
        fm_hide_one_row(&ph->rows[i]);
    }
    ph->visible_count = 0;
    if (ph->list_direct)
    {
        divoom_menu_direct_ctrl_destory(ph->list_direct);
        ph->list_direct = NULL;
    }
}

/* 切 tab 或可见行数变化时复用已有控件，只改文字、收藏标记和位置。 */
static void fm_rebind_kept_rows(Pdivoom_astrootoo_face_market_ctrl ph, int start, int count)
{
    int i;
    int vis;

    if (!ph || !ph->rows || !ph->clock_ids || count <= 0 || ph->row_slots < count)
    {
        return;
    }

    fm_drop_empty_label(ph);
    if (ph->list_direct)
    {
        divoom_menu_direct_ctrl_destory(ph->list_direct);
        ph->list_direct = NULL;
    }
    for (i = 0; i < ph->row_slots; i++)
    {
        if (i < count)
        {
            fm_show_one_row(&ph->rows[i]);
            fm_place_row(&ph->rows[i], i);
        }
        else
        {
            fm_hide_one_row(&ph->rows[i]);
        }
    }
    ph->visible_start = start;
    ph->visible_count = count;
    ph->list_direct = divoom_menu_direct_ctrl_create(1, ph->visible_count);
    for (i = 0; i < count; i++)
    {
        fm_bind_row_content(ph, i, ph->clock_ids[start + i]);
        divoom_menu_direct_ctrl_update(ph->list_direct, ph->rows[i].btn, 0, i);
        fm_paint_row(ph, start + i);
    }
    ph->rows_rebind = 0;
    vis = fm_visible_index(ph, ph->list_focus);
    if (ph->focus_pane == FM_PANE_LIST && vis >= 0 && ph->rows[vis].btn)
    {
        lv_group_focus_obj(ph->rows[vis].btn);
    }
    else if (ph->focus_pane == FM_PANE_STRIP)
    {
        fm_focus_strip(ph);
    }
}

static void fm_rebuild_visible_rows(Pdivoom_astrootoo_face_market_ctrl ph)
{
    int i;
    int start;
    int count;
    int vis;

    if (!ph)
    {
        return;
    }

    if (!ph->clock_ids || ph->row_count <= 0)
    {
        if (ph->rows)
        {
            fm_hide_kept_rows(ph);
        }
        else
        {
            fm_clear_visible_rows(ph);
        }
        return;
    }

    start = fm_calc_visible_start(ph, ph->list_focus);
    count = ph->row_count - start;
    if (count > FM_VISIBLE_MAX)
    {
        count = FM_VISIBLE_MAX;
    }
    if (count <= 0)
    {
        if (ph->rows)
        {
            fm_hide_kept_rows(ph);
        }
        else
        {
            fm_clear_visible_rows(ph);
        }
        return;
    }

    if (!ph->rows_rebind && fm_try_reuse_visible_rows(ph, start, count))
    {
        vis = fm_visible_index(ph, ph->list_focus);
        for (i = 0; i < ph->visible_count; i++)
        {
            fm_paint_row(ph, ph->visible_start + i);
        }
        if (ph->focus_pane == FM_PANE_LIST && vis >= 0 && ph->rows[vis].btn)
        {
            lv_group_focus_obj(ph->rows[vis].btn);
        }
        else if (ph->focus_pane == FM_PANE_STRIP)
        {
            fm_focus_strip(ph);
        }
        return;
    }

    if (ph->rows && ph->row_slots >= count)
    {
        fm_rebind_kept_rows(ph, start, count);
        return;
    }

    fm_clear_visible_rows(ph);

    ph->rows = (fm_row *)divoom_system_malloc((unsigned)count * sizeof(fm_row));
    if (!ph->rows)
    {
        ph->visible_start = 0;
        ph->visible_count = 0;
        return;
    }

    ph->visible_start = start;
    ph->visible_count = count;
    ph->row_slots = count;
    ph->rows_rebind = 0;
    for (i = 0; i < count; i++)
    {
        fm_create_row(ph, i, start + i, ph->clock_ids[start + i]);
    }

    ph->list_direct = divoom_menu_direct_ctrl_create(1, ph->visible_count);
    for (i = 0; i < ph->visible_count; i++)
    {
        divoom_menu_direct_ctrl_update(ph->list_direct, ph->rows[i].btn, 0, i);
    }

    vis = fm_visible_index(ph, ph->list_focus);
    if (ph->focus_pane == FM_PANE_LIST && vis >= 0)
    {
        fm_paint_row(ph, ph->list_focus);
        lv_group_focus_obj(ph->rows[vis].btn);
    }
    else if (ph->focus_pane == FM_PANE_STRIP)
    {
        fm_focus_strip(ph);
    }
}

static void fm_build_list(Pdivoom_astrootoo_face_market_ctrl ph)
{
    int n;
    int i;
    int *ids;

    if (!ph)
    {
        return;
    }

    fm_release_list_data(ph);
    ph->built_for_tab = ph->active_tab;

    ids = (int *)divoom_system_malloc((unsigned)FM_LIST_MAX * sizeof(int));
    if (!ids)
    {
        fm_show_empty(ph);
        ph->built_count = 0;
        return;
    }

    n = fm_collect_tab_clock_ids(ph, ids, FM_LIST_MAX);
    ph->built_count = n;
    if (n <= 0)
    {
        divoom_system_free(ids);
        fm_show_empty(ph);
        return;
    }

    if (n > FM_LIST_MAX)
    {
        n = FM_LIST_MAX;
        ph->built_count = n;
    }

    ph->source_clock_ids = (int *)divoom_system_malloc((unsigned)n * sizeof(int));
    if (!ph->source_clock_ids)
    {
        divoom_system_free(ids);
        ph->built_count = -1;
        fm_show_empty(ph);
        return;
    }

    ph->clock_ids = (int *)divoom_system_malloc((unsigned)n * sizeof(int));
    ph->named_mask = (uint8 *)divoom_system_malloc((unsigned)n);
    if (!ph->clock_ids || !ph->named_mask)
    {
        divoom_system_free(ids);
        ph->built_count = -1;
        fm_free_list(ph);
        fm_show_empty(ph);
        return;
    }

    for (i = 0; i < n; i++)
    {
        ph->source_clock_ids[i] = ids[i];
        ph->named_mask[i] = 0;
    }
    divoom_system_free(ids);

    ph->scan_idx = 0;
    ph->load_busy = 1;
    ph->list_focus = 0;
    ph->col_focus = FM_COL_TEXT;
    ph->row_count = 0;

    /* 分批读取本地名称，尽早显示已有内容。 */
    fm_scan_advance(ph);
    if (ph->load_busy)
    {
        fm_start_scan(ph);
    }
}

static void fm_build_cb(lv_timer_t *t)
{
    Pdivoom_astrootoo_face_market_ctrl ph;

    ph = (Pdivoom_astrootoo_face_market_ctrl)lv_timer_get_user_data(t);
    if (ph)
    {
        ph->build_timer = NULL;
    }
    lv_timer_del(t);
    if (!ph || ph->root_obj == NULL)
    {
        return;
    }
    fm_build_list(ph);
}

static void fm_cancel_pending_build(Pdivoom_astrootoo_face_market_ctrl ph)
{
    if (!ph || !ph->build_timer)
    {
        return;
    }

    lv_timer_del(ph->build_timer);
    ph->build_timer = NULL;
}

static void fm_request_build(Pdivoom_astrootoo_face_market_ctrl ph)
{
    if (!ph || ph->build_timer)
    {
        return;
    }
    ph->build_timer = lv_timer_create(fm_build_cb, 0, ph);
    if (ph->build_timer)
    {
        lv_timer_set_repeat_count(ph->build_timer, 1);
    }
}

static void fm_set_list_focus(Pdivoom_astrootoo_face_market_ctrl ph, int row, int col)
{
    int old_row;
    int old_start;
    int vis;
    int rebuilt;
#if FACE_PREVIEW_PERF
    uint32 started = divoom_system_get_tickms();
#endif

    if (ph->row_count <= 0)
    {
        return;
    }
    if (row < 0)
    {
        row = 0;
    }
    if (row >= ph->row_count)
    {
        row = ph->row_count - 1;
    }
    if (col < FM_COL_TEXT)
    {
        col = FM_COL_TEXT;
    }
    if (col > FM_COL_DELETE)
    {
        col = FM_COL_DELETE;
    }

    old_row = ph->list_focus;
    old_start = ph->visible_start;
    ph->list_focus = row;
    ph->col_focus = col;
    ph->focus_pane = FM_PANE_LIST;
    rebuilt = 0;

    /*
     * 全部表盘可能很多，只保留焦点附近一段 LVGL 对象。
     * 焦点跨出当前段时重建可见段，避免一次性创建大量行导致按键滞后。
     */
    if (fm_calc_visible_start(ph, row) != old_start || fm_visible_index(ph, row) < 0)
    {
        fm_rebuild_visible_rows(ph);
        rebuilt = 1;
    }
    if (old_row != row)
    {
        /* 更新窗口快照；连续切换时继续推迟解码。 */
        fm_arm_thumb_settle(ph);
    }

    if (!rebuilt && old_row != row)
    {
        fm_paint_row(ph, old_row);
    }
    fm_paint_row(ph, row);
    fm_paint_strip(ph);

    vis = fm_visible_index(ph, row);
    if (vis >= 0 && ph->rows && ph->rows[vis].btn)
    {
        lv_group_focus_obj(ph->rows[vis].btn);
        lv_obj_scroll_to_view(ph->rows[vis].btn, LV_ANIM_OFF);
    }
#if FACE_PREVIEW_PERF
    {
        static const uint32 limits[] = {1, 5, 10, 20, 50, 100};
        uint32 elapsed = (uint32)divoom_system_get_tickms() - started;
        int bucket;
        for (bucket = 0; bucket < 6 && elapsed > limits[bucket]; bucket++) {}
        ph->focus_hist[bucket]++;
        ph->focus_count++;
        if (elapsed > ph->focus_max_ms) ph->focus_max_ms = elapsed;
    }
#endif
}

static void fm_focus_strip(Pdivoom_astrootoo_face_market_ctrl ph)
{
    lv_group_t *g;
    int i;

    ph->focus_pane = FM_PANE_STRIP;
    if (ph->row_count > 0)
    {
        fm_paint_row(ph, ph->list_focus);
    }
    fm_paint_strip(ph);
    g = lv_group_get_default();
    if (g)
    {
        for (i = 0; i < FM_TAB_COUNT; i++)
        {
            if (ph->tab_btn[i] && lv_obj_get_group(ph->tab_btn[i]) == NULL)
            {
                lv_group_add_obj(g, ph->tab_btn[i]);
            }
        }
    }
    if (ph->tab_btn[ph->active_tab])
    {
        lv_group_focus_obj(ph->tab_btn[ph->active_tab]);
    }
}

static void fm_switch_tab(Pdivoom_astrootoo_face_market_ctrl ph, int tab)
{
    if (!ph)
    {
        return;
    }

    if (tab < 0)
    {
        tab += FM_TAB_COUNT;
    }
    tab %= FM_TAB_COUNT;

    ph->active_tab = tab;
    ph->focus_pane = FM_PANE_STRIP;
    ph->list_focus = 0;
    ph->col_focus = FM_COL_TEXT;

    /* 切换 tab 时立即回到列表顶部，避免继承上一页滚动偏移导致底部行贴边或像卡住。 */
    if (ph->content)
    {
        lv_obj_scroll_to_y(ph->content, 0, LV_ANIM_OFF);
    }

    fm_paint_strip(ph);
    if (ph->tab_btn[ph->active_tab])
    {
        lv_group_focus_obj(ph->tab_btn[ph->active_tab]);
    }
    fm_cancel_pending_build(ph);
    fm_request_build(ph);
}

static void fm_enter_list(Pdivoom_astrootoo_face_market_ctrl ph)
{
    if (ph->row_count > 0)
    {
        fm_set_list_focus(ph, 0, FM_COL_TEXT);
    }
}

static void fm_toggle_favorite(Pdivoom_astrootoo_face_market_ctrl ph, int row)
{
    fm_row *r;
    int cid;
    int vis;
    int old_fav;

    if (row < 0 || row >= ph->row_count || !ph->clock_ids)
    {
        return;
    }
    cid = ph->clock_ids[row];
    if (cid <= 0)
    {
        return;
    }

    old_fav = divoom_light_app_is_fav_clock(cid);
    divoom_light_app_add_remove_fav_clock(cid, old_fav ? 0 : 1);
    divoom_light_app_report_set_favorite(cid, old_fav ? 0 : 1);
    vis = fm_visible_index(ph, row);
    if (vis >= 0 && ph->rows)
    {
        r = &ph->rows[vis];
        r->is_favorite = (uint8)(!old_fav);
        fm_row_set_fav_icon(r);
    }

    if (ph->active_tab == DIVOOM_ASTRO_FACE_MARKET_TAB_FAVORITE)
    {
        fm_request_build(ph);
    }
}

static void fm_delete_row_commit(Pdivoom_astrootoo_face_market_ctrl ph, int row)
{
    int cid;

    if (!ph || row < 0 || row >= ph->row_count || !ph->clock_ids)
    {
        return;
    }
    cid = ph->clock_ids[row];
    if (cid <= 0)
    {
        return;
    }

    divoom_light_app_add_remove_fav_clock(cid, 0);
    divoom_light_app_remove_recent_clock(cid);
    divoom_light_app_delete_clock_id(cid);
    divoom_net_app_send_packet(DIVOOM_NET_COMM_REMOVE_MY_CLOCK, DIVOOM_SERVER_TOPIC, &cid);
    fm_request_build(ph);
}

static void fm_delete_row(Pdivoom_astrootoo_face_market_ctrl ph, int row)
{
    int cid;

    if (!ph || row < 0 || row >= ph->row_count || !ph->clock_ids)
    {
        return;
    }
    cid = ph->clock_ids[row];
    if (cid <= 0)
    {
        return;
    }

    if (fm_my_clock_count() <= 1)
    {
        fm_show_modal(ph, FM_MODAL_KEEP_MIN_ONE);
        return;
    }

    fm_show_delete_confirm(ph, row);
}

static void fm_play_clock(Pdivoom_astrootoo_face_market_ctrl ph, int row)
{
    int cid;
    int *send_clock_id;
    divoom_sys_msg_info model_msg = {0};

    if (row < 0 || row >= ph->row_count || !ph->clock_ids)
    {
        return;
    }
    cid = ph->clock_ids[row];
    if (cid <= 0)
    {
        return;
    }

    send_clock_id = (int *)divoom_system_malloc(sizeof(int));
    if (!send_clock_id)
    {
        return;
    }
    *send_clock_id = cid;
    model_msg.msg_type = 1;
    model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;
    model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SET_CLOCK_INDEX;
    model_msg.data = (uint8 *)send_clock_id;
    divoom_sys_model_crl_msg_push(&model_msg);
}

static void fm_create_strip(Pdivoom_astrootoo_face_market_ctrl ph)
{
    int i;
    lv_coord_t tab_w;

    tab_w = (FM_TAB_ROW_W - FM_TAB_GAP * (FM_TAB_COUNT - 1)) / FM_TAB_COUNT;

    ph->strip_cont = lv_obj_create(ph->root_obj);
    lv_obj_add_style(ph->strip_cont, &ph->ui.content_style, 0);
    lv_obj_set_size(ph->strip_cont, DIVOOM_SCREEN_WIDTH, FM_STRIP_BACKDROP_H);
    lv_obj_set_pos(ph->strip_cont, 0, 0);
    lv_obj_set_style_bg_color(ph->strip_cont, lv_color_hex(FM_STRIP_BG), 0);
    lv_obj_set_style_bg_opa(ph->strip_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ph->strip_cont, 0, 0);
    lv_obj_clear_flag(ph->strip_cont, LV_OBJ_FLAG_SCROLLABLE);

    ph->strip_track = lv_obj_create(ph->strip_cont);
    lv_obj_add_style(ph->strip_track, &ph->ui.content_style, 0);
    lv_obj_set_style_bg_opa(ph->strip_track, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ph->strip_track, 0, 0);
    lv_obj_set_style_pad_all(ph->strip_track, 0, 0);
    lv_obj_set_size(ph->strip_track, FM_TAB_ROW_W, FM_TAB_H);
    lv_obj_set_pos(ph->strip_track, FM_LIST_PAD_X, 6);
    lv_obj_clear_flag(ph->strip_track, LV_OBJ_FLAG_SCROLLABLE);

    for (i = 0; i < FM_TAB_COUNT; i++)
    {
        lv_obj_t *btn;
        lv_obj_t *lab;

        btn = lv_btn_create(ph->strip_track);
        lv_obj_add_style(btn, &ph->ui.content_style, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_radius(btn, 8, 0);
        lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_pad_all(btn, 0, 0);
        lv_obj_set_size(btn, tab_w, FM_TAB_H);
        lv_obj_set_pos(btn, i * (tab_w + FM_TAB_GAP), 0);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

        lab = lv_label_create(btn);
        lv_label_set_text(lab, divoom_menu_control_get_disp_str(fm_tab_label_keys[i]));
        lv_obj_set_style_text_font(lab, divoom_menu_control_get_font_info(20, ph->menu_id), 0);
        lv_label_set_long_mode(lab, LV_LABEL_LONG_CLIP);
        lv_obj_set_width(lab, tab_w - 8);
        lv_obj_set_style_text_align(lab, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(lab);

        lv_obj_add_event_cb(btn, fm_strip_key_cb, LV_EVENT_KEY, ph);
        lv_group_add_obj(lv_group_get_default(), btn);

        ph->tab_btn[i] = btn;
        ph->tab_label[i] = lab;
    }
    fm_paint_strip(ph);
}

static void fm_handle_strip_key(Pdivoom_astrootoo_face_market_ctrl ph, uint8_t key_code)
{
    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_menu_control_return_pre();
        return;
    }

    if (divoom_astrootoo_ui_key_is_left(key_code))
    {
        fm_switch_tab(ph, ph->active_tab + FM_TAB_COUNT - 1);
        return;
    }

    if (divoom_astrootoo_ui_key_is_right(key_code))
    {
        fm_switch_tab(ph, ph->active_tab + 1);
        return;
    }

    if (divoom_astrootoo_ui_key_is_down(key_code) || divoom_astrootoo_ui_key_is_enter(key_code))
    {
        fm_enter_list(ph);
        return;
    }
}

static void fm_handle_list_key(Pdivoom_astrootoo_face_market_ctrl ph, uint8_t key_code)
{
    if (!ph || ph->row_count <= 0)
    {
        return;
    }

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        fm_focus_strip(ph);
        return;
    }

    if (divoom_astrootoo_ui_key_is_left(key_code))
    {
        if (ph->col_focus > FM_COL_TEXT)
        {
            fm_set_list_focus(ph, ph->list_focus, ph->col_focus - 1);
        }
        return;
    }
    if (divoom_astrootoo_ui_key_is_right(key_code))
    {
        if (ph->col_focus < FM_COL_DELETE)
        {
            fm_set_list_focus(ph, ph->list_focus, ph->col_focus + 1);
        }
        return;
    }

    if (divoom_astrootoo_ui_key_is_up(key_code))
    {
        lv_obj_t *next;
        int idx;
        int vis;

        if (ph->list_focus <= 0)
        {
            fm_focus_strip(ph);
            return;
        }
        vis = fm_visible_index(ph, ph->list_focus);
        idx = -1;
        if (vis > 0 && ph->list_direct && ph->rows && ph->rows[vis].btn)
        {
            next = (lv_obj_t *)divoom_menu_direct_ctrl_get(ph->list_direct,
                ph->rows[vis].btn, DIVOOM_MENU_DIRECT_UP);
            idx = next ? divoom_menu_direct_ctrl_get_pos(ph->list_direct, next) : -1;
            if (idx >= 0 && idx < ph->visible_count)
            {
                idx += ph->visible_start;
            }
        }
        if (idx < 0 || idx >= ph->row_count)
        {
            idx = ph->list_focus - 1;
        }
        fm_set_list_focus(ph, idx, ph->col_focus);
        return;
    }

    if (divoom_astrootoo_ui_key_is_down(key_code))
    {
        lv_obj_t *next;
        int idx;
        int vis;

        if (ph->list_focus >= ph->row_count - 1)
        {
            fm_set_list_focus(ph, 0, ph->col_focus);
            return;
        }
        vis = fm_visible_index(ph, ph->list_focus);
        idx = -1;
        if (vis >= 0 && vis < ph->visible_count - 1 && ph->list_direct
            && ph->rows && ph->rows[vis].btn)
        {
            next = (lv_obj_t *)divoom_menu_direct_ctrl_get(ph->list_direct,
                ph->rows[vis].btn, DIVOOM_MENU_DIRECT_DOWN);
            idx = next ? divoom_menu_direct_ctrl_get_pos(ph->list_direct, next) : -1;
            if (idx >= 0 && idx < ph->visible_count)
            {
                idx += ph->visible_start;
            }
        }
        if (idx < 0 || idx >= ph->row_count)
        {
            idx = ph->list_focus + 1;
        }
        fm_set_list_focus(ph, idx, ph->col_focus);
        return;
    }

    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        if (ph->col_focus == FM_COL_TEXT)
        {
            fm_play_clock(ph, ph->list_focus);
        }
        else if (ph->col_focus == FM_COL_FAVORITE)
        {
            fm_toggle_favorite(ph, ph->list_focus);
        }
        else
        {
            fm_delete_row(ph, ph->list_focus);
        }
        return;
    }
}

static void fm_strip_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_face_market_ctrl ph;
    uint8_t key_code;

    ph = (Pdivoom_astrootoo_face_market_ctrl)lv_event_get_user_data(e);
    if (!ph || lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    if (!divoom_astrootoo_ui_key_is_menu_press(e, key_code))
    {
        return;
    }
    fm_arm_thumb_settle(ph);
    if (fm_modal_blocks_keys(ph))
    {
        return;
    }

    /* LVGL 事件对象偶尔仍落在 tab，上下键按页面状态路由，避免看似列表获焦却无响应。 */
    if (ph->focus_pane == FM_PANE_LIST)
    {
        fm_handle_list_key(ph, key_code);
        return;
    }
    fm_handle_strip_key(ph, key_code);
}

static void fm_list_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_face_market_ctrl ph;
    uint8_t key_code;

    ph = (Pdivoom_astrootoo_face_market_ctrl)lv_event_get_user_data(e);
    if (!ph || lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    if (!divoom_astrootoo_ui_key_is_menu_press(e, key_code))
    {
        return;
    }
    fm_arm_thumb_settle(ph);
    if (fm_modal_blocks_keys(ph))
    {
        return;
    }

    if (ph->focus_pane == FM_PANE_STRIP)
    {
        fm_handle_strip_key(ph, key_code);
        return;
    }
    fm_handle_list_key(ph, key_code);
}

static void fm_refresh_cb(lv_timer_t *t)
{
    Pdivoom_astrootoo_face_market_ctrl ph;
    int ids[FM_LIST_MAX];
    int n;
    int i;
    int cur_clock_id;

    ph = (Pdivoom_astrootoo_face_market_ctrl)lv_timer_get_user_data(t);
    if (!ph || ph->root_obj == NULL)
    {
        return;
    }

    if (ph->load_busy || ph->build_timer)
    {
        return;
    }

    n = fm_collect_tab_clock_ids(ph, ids, FM_LIST_MAX);
    if (n != ph->built_count || ph->built_for_tab != ph->active_tab
        || (n > 0 && !ph->source_clock_ids))
    {
        fm_request_build(ph);
        return;
    }
    for (i = 0; i < n; i++)
    {
        if (ph->source_clock_ids[i] != ids[i])
        {
            fm_request_build(ph);
            return;
        }
    }

    if (!ph->rows || ph->visible_count <= 0)
    {
        return;
    }

    cur_clock_id = divoom_light_app_get_cur_clock_id();
    for (i = 0; i < ph->visible_count; i++)
    {
        int now_fav = divoom_light_app_is_fav_clock(ph->rows[i].clock_id);

        if ((int)ph->rows[i].is_favorite != now_fav)
        {
            ph->rows[i].is_favorite = (uint8)now_fav;
            fm_row_set_fav_icon(&ph->rows[i]);
        }
        if (ph->rows[i].in_use_icon)
        {
            if (ph->rows[i].clock_id == cur_clock_id)
            {
                lv_obj_clear_flag(ph->rows[i].in_use_icon, LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_obj_add_flag(ph->rows[i].in_use_icon, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

void *divoom_astrootoo_face_market_init(void *user_data, int menu_id)
{
    Pdivoom_astrootoo_face_market_ctrl ph;

    (void)user_data;

    ph = divoom_system_malloc_out(sizeof(divoom_astrootoo_face_market_ctrl));
    if (!ph)
    {
        return NULL;
    }
    ph->confirm_delete_row = -1;

    ph->menu_id = menu_id;
    ph->active_tab = DIVOOM_ASTRO_FACE_MARKET_TAB_RECENT;
    ph->focus_pane = FM_PANE_STRIP;
    ph->col_focus = FM_COL_TEXT;
    ph->built_for_tab = -1;
    ph->built_count = -1;
    ph->thumb_settle_timer = NULL;
    ph->row_slots = 0;
    ph->rows_rebind = 0;
    ph->rows = NULL;
    memset(&ph->default_preview, 0, sizeof(ph->default_preview));
    ph->preview_loader = divoom_face_preview_create();
    ph->name_cache = divoom_system_malloc(sizeof(fm_name_entry) * FM_LIST_MAX);
    ph->name_cache_count = ph->name_cache_next = 0;
    ph->name_cache_lang = divoom_main_set_lang_flag(-1);
#if FACE_PREVIEW_PERF
    ph->focus_count = ph->focus_max_ms = ph->thumb_binds = 0;
    memset(ph->focus_hist, 0, sizeof(ph->focus_hist));
#endif
    s_fm_page = ph;

    ph->root_obj = divoom_astrootoo_ui_create_root(&ph->ui);

    fm_create_strip(ph);

    ph->content = lv_obj_create(ph->root_obj);
    lv_obj_add_style(ph->content, &ph->ui.grid_cont_style, 0);
    lv_obj_set_size(ph->content, DIVOOM_SCREEN_WIDTH, FM_CONTENT_H);
    lv_obj_set_pos(ph->content, 0, FM_CONTENT_TOP);
    lv_obj_set_style_pad_all(ph->content, 0, 0);
    lv_obj_set_scrollbar_mode(ph->content, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(ph->content, LV_DIR_VER);

    fm_build_list(ph);

    ph->refresh_timer = lv_timer_create(fm_refresh_cb, FM_REFRESH_MS, ph);

    return ph;
}

void divoom_astrootoo_face_market_disp(void *handle, void *active_obj)
{
    Pdivoom_astrootoo_face_market_ctrl ph;

    (void)active_obj;

    ph = (Pdivoom_astrootoo_face_market_ctrl)handle;
    if (!ph)
    {
        return;
    }

    if (!ph->preview_loader) ph->preview_loader = divoom_face_preview_create();
    fm_arm_thumb_settle(ph);
    if (!ph->refresh_timer)
    {
        ph->refresh_timer = lv_timer_create(fm_refresh_cb, FM_REFRESH_MS, ph);
    }
    if (ph->load_busy && !ph->scan_timer)
    {
        fm_start_scan(ph);
    }

    lv_scr_load(ph->root_obj);
    fm_focus_strip(ph);
}

void divoom_astrootoo_face_market_clean(void *handle)
{
    Pdivoom_astrootoo_face_market_ctrl ph;

    ph = (Pdivoom_astrootoo_face_market_ctrl)handle;
    if (!ph)
    {
        return;
    }
    if (s_fm_page == ph)
    {
        s_fm_page = NULL;
    }

    fm_stop_timers(ph);
    divoom_face_preview_destroy(ph->preview_loader);
    ph->preview_loader = NULL;
    fm_hide_modal(ph);
    fm_free_list(ph);
    divoom_system_free(ph->name_cache);
    ph->name_cache = NULL;
    if (ph->root_obj)
    {
        lv_obj_del(ph->root_obj);
        ph->root_obj = NULL;
    }

    divoom_dial_preview_default_release(&ph->default_preview);
    divoom_astrootoo_ui_base_reset_styles(&ph->ui);
    divoom_system_free(ph);
}

void divoom_astrootoo_face_market_pause(void *handle)
{
    Pdivoom_astrootoo_face_market_ctrl ph;

    ph = (Pdivoom_astrootoo_face_market_ctrl)handle;
    if (!ph)
    {
        return;
    }
    fm_stop_timers(ph);
    divoom_face_preview_destroy(ph->preview_loader);
    ph->preview_loader = NULL;
#if FACE_PREVIEW_PERF
    {
        static const uint32 limits[] = {1, 5, 10, 20, 50, 100, 0xffffffffU};
        uint32 total = 0;
        int i;
        for (i = 0; i < 6; i++)
        {
            total += ph->focus_hist[i];
            if (total >= (ph->focus_count * 95U + 99U) / 100U) break;
        }
        divoom_print("[face_list] focus_count=%u p95_bucket_ms=%u focus_max_ms=%u binds=%u cached_names=%d\n",
            ph->focus_count, limits[i], ph->focus_max_ms, ph->thumb_binds, ph->name_cache_count);
    }
#endif
}

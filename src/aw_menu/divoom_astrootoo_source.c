#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_menu_direct_ctrl.h"
#include "divoom_main.h"
#include "divoom_music_ctrl.h"
#include "divoom_bt_music.h"
#include "divoom_tws_host.h"
#include "divoom_bt_bip.h"
#include "divoom_pixel_format.h"
#include "divoom_rk_mpp.h"
#include "divoom_jpeg_decode.h"
#include "divoom_tf_music.h"
#include "divoom_radio.h"
#include "divoom_radio_menu.h"
#include "divoom_wifi_con.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_lvgl_top_toast.h"
#include "divoom_astrootoo_source.h"
#include "divoom_lvgl_font_manage.h"
#include "divoom_astrootoo_theme.h"
#include "divoom_lvgl_top_source_switch_menu.h"
#include <string.h>
#include <strings.h>
#include <stdlib.h>

#define DIVOOM_ASTROTOO_SOURCE_UI_DIR       "91/"
#define DIVOOM_ASTRO_SOURCE_HEADER          DIVOOM_ASTROTOO_SOURCE_UI_DIR "source_header.png"
#define DIVOOM_ASTRO_SOURCE_LIST_ROW_H      66
#define DIVOOM_ASTRO_SOURCE_LIST_ROW_GAP    6
#define DIVOOM_ASTRO_SOURCE_LIST_ICON       36
#define DIVOOM_ASTRO_SOURCE_LIST_ROW_W       448
#define DIVOOM_ASTRO_SOURCE_PAGE_STACK_MAX  8
/* TF 播放几何见 divoom_astrootoo_theme.h（暖纸主卡 + 卡内坐标） */
#define DIVOOM_ASTRO_SOURCE_NET_TAB_CNT      3
#define DIVOOM_ASTRO_SOURCE_NET_STATION_CNT  DIVOOM_RADIO_MAX
#define DIVOOM_ASTRO_SOURCE_TF_BROWSE_NAME_LEN 64
#define DIVOOM_ASTRO_SOURCE_TF_SLOT_CNT        8
#define DIVOOM_ASTRO_SOURCE_TF_FOLDER_STACK    16
#define DIVOOM_ASTRO_SOURCE_TF_ROW_STRIDE \
    (DIVOOM_ASTRO_SOURCE_LIST_ROW_H + DIVOOM_ASTRO_SOURCE_LIST_ROW_GAP)

#define DIVOOM_ASTRO_SOURCE_TF_REFRESH_MS       250
#define DIVOOM_ASTRO_SOURCE_TF_CATALOG_MS       500
#define DIVOOM_ASTRO_SOURCE_PLAYBACK_REFRESH_MS 500

typedef enum
{
    DIVOOM_ASTRO_SOURCE_PAGE_LIST = 0,
    DIVOOM_ASTRO_SOURCE_PAGE_BT,
    DIVOOM_ASTRO_SOURCE_PAGE_TF,
    DIVOOM_ASTRO_SOURCE_PAGE_UAC,
#if DIVOOM_MUSIC_CTRL_LINEIN_ENABLE
    DIVOOM_ASTRO_SOURCE_PAGE_AUX,
#endif
    DIVOOM_ASTRO_SOURCE_PAGE_NET_RADIO,
    DIVOOM_ASTRO_SOURCE_PAGE_TF_HUB,
    DIVOOM_ASTRO_SOURCE_PAGE_TF_TRACKS,
    DIVOOM_ASTRO_SOURCE_PAGE_TF_SCOPE,
    DIVOOM_ASTRO_SOURCE_PAGE_TF_ORDER,
    DIVOOM_ASTRO_SOURCE_PAGE_TF_EQ,
    DIVOOM_ASTRO_SOURCE_PAGE_TF_FAVORITES,
} divoom_astrootoo_source_page_t;

#define DIVOOM_ASTRO_SOURCE_MAIN_SLOTS \
    (3 + (DIVOOM_MUSIC_CTRL_UAC_ENABLE ? 1 : 0) + (DIVOOM_MUSIC_CTRL_LINEIN_ENABLE ? 1 : 0))

typedef enum
{
    DIVOOM_ASTRO_SOURCE_TRANSPORT_PREV = 0,
    DIVOOM_ASTRO_SOURCE_TRANSPORT_PLAY,
    DIVOOM_ASTRO_SOURCE_TRANSPORT_NEXT,
} divoom_astrootoo_source_transport_t;

typedef enum
{
    DIVOOM_ASTRO_SOURCE_TF_SEC_REPEAT = 0,
    DIVOOM_ASTRO_SOURCE_TF_SEC_LIST,
    DIVOOM_ASTRO_SOURCE_TF_SEC_REFRESH,
} divoom_astrootoo_source_tf_secondary_t;

typedef enum
{
    DIVOOM_ASTRO_SOURCE_NET_ROAM_TABS = 0,
    DIVOOM_ASTRO_SOURCE_NET_ROAM_LIST,
    DIVOOM_ASTRO_SOURCE_NET_ROAM_DOCK,
} divoom_astrootoo_source_net_roam_t;

typedef enum
{
    DIVOOM_ASTRO_SOURCE_NET_PANE_NONE = 0,
    DIVOOM_ASTRO_SOURCE_NET_PANE_TABS,
    DIVOOM_ASTRO_SOURCE_NET_PANE_LIST,
    DIVOOM_ASTRO_SOURCE_NET_PANE_DOCK,
} divoom_astrootoo_source_net_pane_t;

typedef struct _divoom_astrootoo_source_ctrl
{
    divoom_astrootoo_ui_base ui;
    int menu_id;
    divoom_astrootoo_source_page_t page;
    divoom_astrootoo_source_page_t page_stack[DIVOOM_ASTRO_SOURCE_PAGE_STACK_MAX];
    int page_stack_depth;

    lv_obj_t *root_obj;
    lv_obj_t *header;
    lv_obj_t *content;
    lv_obj_t *key_btn;
    void *direct_handle;
    lv_timer_t *refresh_timer;

    /* 主列表 */
    lv_obj_t *list_rows[DIVOOM_ASTRO_SOURCE_MAIN_SLOTS];

    /* 卡片播放（蓝牙/AUX/UAC） */
    lv_obj_t *card_cont;
    lv_obj_t *card_line1_lbl;
    lv_obj_t *card_line2_lbl;
    lv_obj_t *card_lyric_plate;
    lv_obj_t *card_divider;
    lv_obj_t *uac_wave[2];
    lv_obj_t *card_hint_lbl;
    lv_obj_t *transport_btns[3];
    lv_obj_t *transport_imgs[3];
    lv_obj_t *mode_hint_lbl;
    divoom_astrootoo_source_transport_t transport_focus;
    uint8 playback_source; /* DIVOOM_MUSIC_CTRL_MODE_* */
    char bt_line1[384];
    char bt_line2[384];
    uint8 bt_line1_dirty;
    uint8 bt_line2_dirty;
    uint8 playback_ui_dirty;
    /* 蓝牙封面。有 JPG 时盖住黑胶，没有时藏起图片露出黑胶 */
    lv_obj_t *bt_cover_img;
    lv_img_dsc_t bt_cover_dsc;
    uint8_t *bt_cover_buf;
    uint32_t bt_cover_gen;

    /* TF 播放 */
    lv_obj_t *tf_meta_cont;
    lv_obj_t *tf_vinyl_disc;
    lv_timer_t *tf_vinyl_timer;
    int tf_vinyl_angle;
    lv_obj_t *tf_title_lbl;
    lv_obj_t *tf_progress_bar;
    lv_obj_t *tf_progress_thumb;
    lv_obj_t *tf_time_pos_lbl;
    lv_obj_t *tf_time_dur_lbl;
    lv_obj_t *tf_transport_bar;
    lv_obj_t *tf_secondary_bar;
    lv_obj_t *tf_transport_btns[3];
    lv_obj_t *tf_transport_imgs[3];
    lv_obj_t *tf_secondary_btns[3];
    lv_obj_t *tf_secondary_imgs[3];
    divoom_astrootoo_source_transport_t tf_transport_focus;
    divoom_astrootoo_source_tf_secondary_t tf_secondary_focus;
    uint8 tf_focus_row; /* 0 transport 1 secondary */
    int tf_track_focus;
    int tf_track_count;
    int tf_folder_id;
    int tf_folder_stack[DIVOOM_ASTRO_SOURCE_TF_FOLDER_STACK];
    int tf_folder_depth;
    int tf_generation;
    divoom_tf_entry tf_slots[DIVOOM_ASTRO_SOURCE_TF_SLOT_CNT];
    int tf_slot_count;
    int tf_dir_total;
    int tf_root_play_index;
    lv_obj_t *tf_slot_row[DIVOOM_ASTRO_SOURCE_TF_SLOT_CNT];
    lv_obj_t *tf_slot_icon[DIVOOM_ASTRO_SOURCE_TF_SLOT_CNT];
    lv_obj_t *tf_slot_label[DIVOOM_ASTRO_SOURCE_TF_SLOT_CNT];
    lv_obj_t *tf_slot_chevron[DIVOOM_ASTRO_SOURCE_TF_SLOT_CNT];
    lv_obj_t *tf_slot_check[DIVOOM_ASTRO_SOURCE_TF_SLOT_CNT];
    int tf_window_start;
    int tf_logical_focus;
    int tf_resume_focus; /* 从子目录返回时保留焦点，不 reset */

    /* 分类页只持有5条可见行；浏览记忆独立于可释放的页面对象。 */
    void *net_menu;
    divoom_radio_menu_memory net_memory;

    /* picker 共用 */
    int picker_committed;
    int picker_focus;
    const char **picker_keys;
    int picker_count;
} divoom_astrootoo_source_ctrl, *Pdivoom_astrootoo_source_ctrl;

static Pdivoom_astrootoo_source_ctrl g_astrootoo_source_handle;

static const struct
{
    const char *label_key;
    const char *icon_file;
    uint8 music_mode;
    divoom_astrootoo_source_page_t page;
} g_source_main_rows[] =
{
    { "Bluetooth", DIVOOM_ASTROTOO_SOURCE_UI_DIR "source_bluetooth.png", DIVOOM_MUSIC_CTRL_MODE_BT, DIVOOM_ASTRO_SOURCE_PAGE_BT },
    { "TFCard", DIVOOM_ASTROTOO_SOURCE_UI_DIR "source_tf.png", DIVOOM_MUSIC_CTRL_MODE_LOCAL, DIVOOM_ASTRO_SOURCE_PAGE_TF },
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
    { "SourceUAC", DIVOOM_ASTROTOO_SOURCE_UI_DIR "source_uac.png", DIVOOM_MUSIC_CTRL_MODE_UAC, DIVOOM_ASTRO_SOURCE_PAGE_UAC },
#endif
#if DIVOOM_MUSIC_CTRL_LINEIN_ENABLE
    { "SourceAUX", DIVOOM_ASTROTOO_SOURCE_UI_DIR "source_aux.png", DIVOOM_MUSIC_CTRL_MODE_LINEIN, DIVOOM_ASTRO_SOURCE_PAGE_AUX },
#endif
    { "SourceNetRadio", DIVOOM_ASTROTOO_SOURCE_UI_DIR "source_net_radio.png", DIVOOM_MUSIC_CTRL_MODE_NET_FM, DIVOOM_ASTRO_SOURCE_PAGE_NET_RADIO },
};

#define DIVOOM_ASTRO_SOURCE_MAIN_CNT \
    ((int)(sizeof(g_source_main_rows) / sizeof(g_source_main_rows[0])))

static const char *g_eq_keys[] =
{
    "EqPresetOriginal",
    "EqPresetGaming",
    "EqPresetHifi",
    "EqPresetOutdoor",
    "EqPresetRetro",
};

static const char *g_scope_keys[] =
{
    "SourceScopeAll",
    "SourceScopeFolder",
    "SourceScopeFavorites",
};

static const char *g_order_keys[] =
{
    "SequentialPlay",
    "SinglePlay",
    "RandomPlay",
};

/* AstroTooUI TfPlayOrder：sequential → singleLoop → listLoop(shuffle) */
static const char *divoom_astrootoo_source_tf_play_mode_icon(int mode)
{
    static const char *icons[3] =
    {
        DIVOOM_ASTROTOO_SOURCE_UI_DIR "source_tf_repeat.png",
        DIVOOM_ASTROTOO_SOURCE_UI_DIR "source_tf_single_loop.png",
        DIVOOM_ASTROTOO_SOURCE_UI_DIR "source_tf_list_loop.png",
    };

    if (mode < DIVOOM_FT_MUSIC_MODE_LIST || mode > DIVOOM_FT_MUSIC_MODE_RANDOM)
    {
        mode = DIVOOM_FT_MUSIC_MODE_LIST;
    }
    return icons[mode];
}

static int divoom_astrootoo_source_cycle_tf_play_mode(void)
{
    int mode;

    mode = divoom_main_set_tf_music_mode(-1);
    mode++;
    if (mode > DIVOOM_FT_MUSIC_MODE_RANDOM)
    {
        mode = DIVOOM_FT_MUSIC_MODE_LIST;
    }
    divoom_main_set_tf_music_mode(mode);
    return mode;
}

static const char *divoom_astrootoo_source_tf_play_mode_label(int mode)
{
    if (mode == DIVOOM_FT_MUSIC_MODE_ONE)
    {
        return divoom_menu_control_get_disp_str("SinglePlay");
    }
    if (mode == DIVOOM_FT_MUSIC_MODE_RANDOM)
    {
        return divoom_menu_control_get_disp_str("RandomPlay");
    }
    return divoom_menu_control_get_disp_str("SequentialPlay");
}

static const char *g_net_station_names[6] =
{
    "NetRadioStation1",
    "NetRadioStation2",
    "NetRadioStation3",
    "NetRadioStation4",
    "NetRadioStation5",
    "NetRadioStation6",
};

static void divoom_astrootoo_source_push_page(Pdivoom_astrootoo_source_ctrl phandle, divoom_astrootoo_source_page_t page);
static void divoom_astrootoo_source_pop_page(Pdivoom_astrootoo_source_ctrl phandle);
static void divoom_astrootoo_source_show_page(Pdivoom_astrootoo_source_ctrl phandle, divoom_astrootoo_source_page_t page);
static void divoom_astrootoo_source_clear_content(Pdivoom_astrootoo_source_ctrl phandle,
    divoom_astrootoo_source_page_t next_page);
static void divoom_astrootoo_source_group_bind(Pdivoom_astrootoo_source_ctrl phandle);
static void divoom_astrootoo_source_refresh_playback_ui(Pdivoom_astrootoo_source_ctrl phandle);
static int divoom_astrootoo_source_is_card_playback_page(divoom_astrootoo_source_page_t page);
static void divoom_astrootoo_source_refresh_net_radio_ui(Pdivoom_astrootoo_source_ctrl phandle);
static void divoom_astrootoo_source_key_cb(lv_event_t *e);
static void divoom_astrootoo_source_timer_cb(lv_timer_t *t);
static const char *divoom_astrootoo_source_path_basename(const char *path);
static void divoom_astrootoo_source_tf_browse_go_up(Pdivoom_astrootoo_source_ctrl phandle);
static void divoom_astrootoo_source_collect_tf_browse_entries(Pdivoom_astrootoo_source_ctrl phandle);
static void divoom_astrootoo_source_tf_browse_reset_nav(Pdivoom_astrootoo_source_ctrl phandle);
static void divoom_astrootoo_source_build_tf_slot_list(Pdivoom_astrootoo_source_ctrl phandle);
static void divoom_astrootoo_source_tf_list_paint(Pdivoom_astrootoo_source_ctrl phandle);
static void divoom_astrootoo_source_tf_list_apply_focus(Pdivoom_astrootoo_source_ctrl phandle,
    lv_anim_enable_t anim);
static int divoom_astrootoo_source_tf_list_nav(Pdivoom_astrootoo_source_ctrl phandle, int go_up,
    lv_anim_enable_t anim);
static void divoom_astrootoo_source_refresh_tf_ui(Pdivoom_astrootoo_source_ctrl phandle);
static void divoom_astrootoo_source_tf_vinyl_set_spin(Pdivoom_astrootoo_source_ctrl phandle, int playing);
static void divoom_astrootoo_source_tf_vinyl_build(Pdivoom_astrootoo_source_ctrl phandle, lv_obj_t *meta,
    int draw_rim);
static void divoom_astrootoo_source_tf_refresh_bar_focus(Pdivoom_astrootoo_source_ctrl phandle);
static void divoom_astrootoo_source_tf_focus_btn(Pdivoom_astrootoo_source_ctrl phandle);
static void divoom_astrootoo_source_tf_toast_hide(void);
static void divoom_astrootoo_source_tf_toast_show(const char *text, uint32_t duration_ms);
static void divoom_astrootoo_source_stop_refresh_timer(Pdivoom_astrootoo_source_ctrl phandle);
static void divoom_astrootoo_source_start_refresh_timer(Pdivoom_astrootoo_source_ctrl phandle,
    divoom_astrootoo_source_page_t page);

/* 为可聚焦按钮挂上统一按键回调（焦点对象才会收到 LV_EVENT_KEY） */
static void divoom_astrootoo_source_attach_key(Pdivoom_astrootoo_source_ctrl phandle, lv_obj_t *btn)
{
    if (phandle && btn)
    {
        lv_obj_add_event_cb(btn, divoom_astrootoo_source_key_cb, LV_EVENT_KEY, phandle);
    }
}

/* 统一设置含 CJK 的字体，避免中文显示成方块 */
static void divoom_astrootoo_source_label_font(lv_obj_t *lbl, int size, int menu_id)
{
    if (lbl)
    {
        lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(size, menu_id), 0);
    }
}

static void divoom_astrootoo_source_tf_format_row_label(char *dst, int dst_sz, const char *src, int is_dir)
{
    char *dot;

    if (dst == NULL || dst_sz <= 0 || src == NULL)
    {
        return;
    }
    lv_snprintf(dst, (size_t)dst_sz, "%s", src);
    if (!is_dir)
    {
        dot = strrchr(dst, '.');
        if (dot != NULL && strcasecmp(dot, ".mp3") == 0)
        {
            *dot = '\0';
        }
    }
}

static void divoom_astrootoo_source_tf_row_label_set(Pdivoom_astrootoo_source_ctrl phandle, lv_obj_t *lab,
    const char *text)
{
    void *font;

    if (lab == NULL || text == NULL)
    {
        return;
    }
    font = divoom_lvgl_font_manage_load_font(divoom_menu_control_get_system_font_id(), 24,
        (char *)text, DIVOOM_ASTRO_REPEAT_ROW_W - 120, 10, 24, NULL, phandle->menu_id, NULL);
    if (font != NULL)
    {
        lv_obj_set_style_text_font(lab, font, 0);
    }
    else
    {
        divoom_astrootoo_source_label_font(lab, 24, phandle->menu_id);
    }
    lv_label_set_text(lab, text);
    divoom_astrootoo_ui_tf_label_refresh_long_mode(lab);
}

static void divoom_astrootoo_source_bt_lrc_cb(void *para, char *singer, char *song_name, char *lyric, int play_time)
{
    Pdivoom_astrootoo_source_ctrl phandle;
    char buf[384];

    (void)play_time;
    phandle = (Pdivoom_astrootoo_source_ctrl)para;
    if (!phandle)
    {
        return;
    }

    buf[0] = '\0';
    if (song_name && song_name[0])
    {
        lv_snprintf(buf, sizeof(buf), "%s", song_name);
    }
    if (singer && singer[0])
    {
        if (buf[0])
        {
            lv_snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), " \xC2\xB7 %s", singer);
        }
        else
        {
            lv_snprintf(buf, sizeof(buf), "%s", singer);
        }
    }
    if (strcmp(phandle->bt_line1, buf) != 0)
    {
        lv_snprintf(phandle->bt_line1, sizeof(phandle->bt_line1), "%s", buf);
        phandle->bt_line1_dirty = 1;
    }

    if (!lyric)
    {
        lyric = "";
    }
    if (strcmp(phandle->bt_line2, lyric) != 0)
    {
        lv_snprintf(phandle->bt_line2, sizeof(phandle->bt_line2), "%s", lyric);
        phandle->bt_line2_dirty = 1;
    }
}

static void divoom_astrootoo_source_fmt_mmss(char *buf, int n, int total_sec)
{
    int m;
    int s;

    if (total_sec < 0)
    {
        total_sec = 0;
    }
    m = total_sec / 60;
    s = total_sec % 60;
    lv_snprintf(buf, n, "%02d:%02d", m, s);
}

/* 路径最后一段：11111/22222/444.mp3 -> 444.mp3 */
static const char *divoom_astrootoo_source_path_basename(const char *path)
{
    const char *p;
    const char *last;

    if (!path || !path[0])
    {
        return "";
    }
    last = path;
    for (p = path; *p; p++)
    {
        if (*p == '/' || *p == '\\')
        {
            if (p[1] != '\0')
            {
                last = p + 1;
            }
        }
    }
    return last;
}

static int divoom_astrootoo_source_tf_entry_is_mount(const divoom_tf_entry *entry)
{
    return (entry != NULL && entry->is_dir
        && divoom_tf_music_root_is_mount_label(entry->name));
}

static void divoom_astrootoo_source_tf_browse_reset_nav(Pdivoom_astrootoo_source_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }
    phandle->tf_folder_id = 0;
    phandle->tf_folder_depth = 0;
    memset(phandle->tf_folder_stack, 0, sizeof(phandle->tf_folder_stack));
    phandle->tf_window_start = 0;
    phandle->tf_logical_focus = 0;
    phandle->tf_resume_focus = 0;
    phandle->tf_root_play_index = -1;
    phandle->tf_slot_count = 0;
    phandle->tf_dir_total = 0;
    phandle->tf_track_count = 0;
    memset(phandle->tf_slots, 0, sizeof(phandle->tf_slots));
}

static int divoom_astrootoo_source_tf_find_visible_index(int folder_id, int id, int is_dir)
{
    divoom_tf_entry entry;
    int raw;
    int off;
    int vis;

    if (id <= 0 || divoom_database_tf_is_rebuilding())
    {
        return -1;
    }
    raw = divoom_database_tf_dir_total(folder_id);
    vis = 0;
    off = 0;
    while (off < raw)
    {
        if (divoom_database_tf_dir_page(folder_id, off, 1, &entry, 1) <= 0)
        {
            break;
        }
        if (folder_id == 0 && divoom_astrootoo_source_tf_entry_is_mount(&entry))
        {
            off++;
            continue;
        }
        if (entry.id == id && ((is_dir && entry.is_dir) || (!is_dir && !entry.is_dir)))
        {
            return vis;
        }
        vis++;
        off++;
    }
    return -1;
}

static int divoom_astrootoo_source_tf_root_play_index_of(int folder_id)
{
    divoom_tf_entry entry;
    int off;
    int idx;

    off = 0;
    idx = 0;
    while (divoom_database_tf_dir_page(0, off, 1, &entry, 1) > 0)
    {
        if (!entry.is_dir)
        {
            break;
        }
        if (divoom_tf_music_root_is_mount_label(entry.name))
        {
            off++;
            continue;
        }
        if (entry.id == folder_id)
        {
            return idx;
        }
        idx++;
        off++;
    }
    return 0;
}

static void divoom_astrootoo_source_tf_clear_slots(Pdivoom_astrootoo_source_ctrl phandle)
{
    phandle->tf_slot_count = 0;
    phandle->tf_dir_total = 0;
    phandle->tf_track_count = 0;
    memset(phandle->tf_slots, 0, sizeof(phandle->tf_slots));
}

static void divoom_astrootoo_source_collect_tf_browse_entries(Pdivoom_astrootoo_source_ctrl phandle)
{
    divoom_tf_entry entry;
    uint32 gen;
    int raw_total;
    int mount_cnt;
    int skip;
    int filled;
    int off;

    if (!phandle)
    {
        return;
    }

    gen = 0;
    if (divoom_database_tf_meta_get_u32("generation", &gen))
    {
        if (phandle->tf_generation < 0)
        {
            phandle->tf_generation = (int)gen;
        }
        else if ((int)gen != phandle->tf_generation)
        {
            phandle->tf_generation = (int)gen;
            divoom_astrootoo_source_tf_browse_reset_nav(phandle);
        }
    }

    if (divoom_database_tf_is_rebuilding())
    {
        divoom_astrootoo_source_tf_clear_slots(phandle);
        return;
    }

    if (phandle->tf_folder_id == 0)
    {
        raw_total = divoom_database_tf_dir_total(0);
        mount_cnt = 0;
        off = 0;
        while (off < raw_total)
        {
            if (divoom_database_tf_dir_page(0, off, 1, &entry, 1) <= 0)
            {
                break;
            }
            if (!entry.is_dir)
            {
                break;
            }
            if (divoom_tf_music_root_is_mount_label(entry.name))
            {
                mount_cnt++;
            }
            off++;
        }
        phandle->tf_dir_total = raw_total - mount_cnt;
        if (phandle->tf_dir_total < 0)
        {
            phandle->tf_dir_total = 0;
        }

        skip = phandle->tf_window_start;
        filled = 0;
        off = 0;
        memset(phandle->tf_slots, 0, sizeof(phandle->tf_slots));
        while (off < raw_total && filled < DIVOOM_ASTRO_SOURCE_TF_SLOT_CNT)
        {
            if (divoom_database_tf_dir_page(0, off, 1, &entry, 1) <= 0)
            {
                break;
            }
            if (divoom_astrootoo_source_tf_entry_is_mount(&entry))
            {
                off++;
                continue;
            }
            if (skip > 0)
            {
                skip--;
                off++;
                continue;
            }
            phandle->tf_slots[filled++] = entry;
            off++;
        }
        phandle->tf_slot_count = filled;
    }
    else
    {
        phandle->tf_dir_total = divoom_database_tf_dir_total(phandle->tf_folder_id);
        memset(phandle->tf_slots, 0, sizeof(phandle->tf_slots));
        phandle->tf_slot_count = divoom_database_tf_dir_page(phandle->tf_folder_id,
            phandle->tf_window_start, DIVOOM_ASTRO_SOURCE_TF_SLOT_CNT,
            phandle->tf_slots, DIVOOM_ASTRO_SOURCE_TF_SLOT_CNT);
    }

    phandle->tf_track_count = phandle->tf_dir_total;
}

static void divoom_astrootoo_source_tf_browse_go_up(Pdivoom_astrootoo_source_ctrl phandle)
{
    int left_id;
    int idx;

    if (!phandle)
    {
        return;
    }
    if (phandle->tf_folder_depth <= 0)
    {
        divoom_astrootoo_source_pop_page(phandle);
        return;
    }
    left_id = phandle->tf_folder_id;
    phandle->tf_folder_depth--;
    phandle->tf_folder_id = phandle->tf_folder_stack[phandle->tf_folder_depth];
    phandle->tf_window_start = 0;
    phandle->tf_logical_focus = 0;
    idx = divoom_astrootoo_source_tf_find_visible_index(phandle->tf_folder_id, left_id, 1);
    if (idx >= 0)
    {
        phandle->tf_logical_focus = idx;
    }
    phandle->tf_resume_focus = 1;
    if (phandle->tf_folder_id == 0)
    {
        divoom_astrootoo_source_pop_page(phandle);
        return;
    }
    divoom_astrootoo_source_show_page(phandle, DIVOOM_ASTRO_SOURCE_PAGE_TF_TRACKS);
}

static const char *divoom_astrootoo_source_tf_browse_title(Pdivoom_astrootoo_source_ctrl phandle)
{
    static char title_buf[DIVOOM_ASTRO_SOURCE_TF_BROWSE_NAME_LEN];
    divoom_tf_folder_info *info;

    if (!phandle || phandle->tf_folder_id <= 0)
    {
        return divoom_menu_control_get_disp_str("FolderBrowsing");
    }
    info = (divoom_tf_folder_info *)divoom_system_malloc(sizeof(*info));
    if (!info)
    {
        return divoom_menu_control_get_disp_str("FolderBrowsing");
    }
    if (divoom_database_tf_folder_info(phandle->tf_folder_id, info)
        && info->name[0]
        && !divoom_tf_music_root_is_mount_label(info->name))
    {
        lv_snprintf(title_buf, sizeof(title_buf), "%s", info->name);
        divoom_system_free(info);
        return title_buf;
    }
    divoom_system_free(info);
    return divoom_menu_control_get_disp_str("FolderBrowsing");
}

static lv_obj_t *divoom_astrootoo_source_create_list_row_icon(Pdivoom_astrootoo_source_ctrl phandle,
    lv_obj_t *parent, const char *label_key, const char *icon_file, int row_index, void *direct_handle)
{
    lv_obj_t *btn;
    lv_obj_t *icon;
    lv_obj_t *label;
    lv_obj_t *chevron;
    lv_coord_t y;

    /* golden 首行顶部留白 4px（行间隙仍为 GAP=6，行高 66、节距 72），与 source.png 对齐 */
    y = (DIVOOM_ASTRO_SOURCE_LIST_ROW_GAP - 2) + row_index * (DIVOOM_ASTRO_SOURCE_LIST_ROW_H + DIVOOM_ASTRO_SOURCE_LIST_ROW_GAP);
    btn = lv_btn_create(parent);
    lv_obj_add_style(btn, &phandle->ui.list_item_style, 0);
    lv_obj_add_style(btn, &phandle->ui.list_item_focus_style, LV_STATE_FOCUSED);
    lv_obj_add_style(btn, &phandle->ui.list_item_focus_style, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn, &phandle->ui.list_item_border_focus_style, LV_STATE_FOCUSED);
    lv_obj_add_style(btn, &phandle->ui.list_item_border_focus_style, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_size(btn, DIVOOM_ASTRO_SOURCE_LIST_ROW_W, DIVOOM_ASTRO_SOURCE_LIST_ROW_H);
    lv_obj_set_pos(btn, DIVOOM_ASTRO_LIST_PAD_X, y);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    icon = lv_img_create(btn);
    lv_obj_set_size(icon, DIVOOM_ASTRO_SOURCE_LIST_ICON, DIVOOM_ASTRO_SOURCE_LIST_ICON);
    divoom_menu_control_lv_img_set_src(icon, icon_file);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, 8, 0);

    label = lv_label_create(btn);
    lv_label_set_text(label, divoom_menu_control_get_disp_str(label_key));
    lv_obj_set_style_text_color(label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    divoom_astrootoo_source_label_font(label, 24, phandle->menu_id);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 8 + DIVOOM_ASTRO_SOURCE_LIST_ICON + 8, 0);

    chevron = lv_img_create(btn);
    divoom_menu_control_lv_img_set_src(chevron, "23/at_sublevel_chevron.png");
    divoom_astrootoo_ui_apply_sublevel_chevron_size(chevron);
    lv_obj_align(chevron, LV_ALIGN_RIGHT_MID, -8, 0);

    divoom_astrootoo_source_attach_key(phandle, btn);
    if (direct_handle)
    {
        divoom_menu_direct_ctrl_update(direct_handle, btn, 0, row_index);
    }
    return btn;
}

static lv_obj_t *divoom_astrootoo_source_create_picker_row(Pdivoom_astrootoo_source_ctrl phandle,
    lv_obj_t *parent, const char *label_key, int row_index, int committed_index, void *direct_handle)
{
    lv_obj_t *btn;
    lv_obj_t *label;
    lv_obj_t *check;

    btn = divoom_astrootoo_ui_create_repeat_row(parent, &phandle->ui, label_key, row_index,
        phandle->picker_count, direct_handle);
    label = lv_obj_get_child(btn, 0);
    if (label)
    {
        divoom_astrootoo_source_label_font(label, 24, phandle->menu_id);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 12, 0);
    }
    if (row_index == committed_index)
    {
        check = lv_img_create(btn);
        divoom_menu_control_lv_img_set_src(check, "23/at_repeat_check.png");
        lv_obj_align(check, LV_ALIGN_RIGHT_MID, -12, 0);
    }
    divoom_astrootoo_source_attach_key(phandle, btn);
    return btn;
}

static void divoom_astrootoo_source_set_transport_img(Pdivoom_astrootoo_source_ctrl phandle,
    lv_obj_t *img, int playing, int focused)
{
    const char *name;

    (void)phandle;
    (void)focused;
    if (playing)
    {
        name = DIVOOM_ASTROTOO_SOURCE_UI_DIR "source_transport_pause.png";
    }
    else
    {
        name = DIVOOM_ASTROTOO_SOURCE_UI_DIR "source_transport_play.png";
    }
    divoom_menu_control_lv_img_set_src(img, name);
}

static lv_style_t g_astro_card_transport_style;
static lv_style_t g_astro_card_transport_focus_style;
static uint8 g_astro_card_transport_style_ready;

static void divoom_astrootoo_source_init_card_transport_styles(void)
{
    if (g_astro_card_transport_style_ready)
    {
        return;
    }
    lv_style_init(&g_astro_card_transport_style);
    lv_style_set_bg_color(&g_astro_card_transport_style, lv_color_hex(DIVOOM_ASTRO_SURFACE_IDLE));
    lv_style_set_bg_opa(&g_astro_card_transport_style, LV_OPA_COVER);
    lv_style_set_border_color(&g_astro_card_transport_style, lv_color_hex(DIVOOM_ASTRO_STROKE));
    lv_style_set_border_width(&g_astro_card_transport_style, 2);
    lv_style_set_radius(&g_astro_card_transport_style, DIVOOM_ASTRO_RADIUS_SM);
    lv_style_set_pad_all(&g_astro_card_transport_style, 0);
    lv_style_set_shadow_width(&g_astro_card_transport_style, 0);
    lv_style_set_outline_width(&g_astro_card_transport_style, 0);

    lv_style_init(&g_astro_card_transport_focus_style);
    lv_style_set_bg_color(&g_astro_card_transport_focus_style, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS));
    lv_style_set_bg_opa(&g_astro_card_transport_focus_style, LV_OPA_COVER);
    lv_style_set_border_color(&g_astro_card_transport_focus_style, lv_color_hex(DIVOOM_ASTRO_BT_TRANSPORT_TAB_INK));
    lv_style_set_border_width(&g_astro_card_transport_focus_style, 2);
    lv_style_set_radius(&g_astro_card_transport_focus_style, DIVOOM_ASTRO_RADIUS_SM);
    lv_style_set_pad_all(&g_astro_card_transport_focus_style, 0);
    lv_style_set_shadow_width(&g_astro_card_transport_focus_style, 0);
    lv_style_set_outline_width(&g_astro_card_transport_focus_style, 0);

    g_astro_card_transport_style_ready = 1;
}

static void divoom_astrootoo_source_apply_card_transport_btn_style(lv_obj_t *btn)
{
    divoom_astrootoo_source_init_card_transport_styles();
    lv_obj_remove_style_all(btn);
    lv_obj_add_style(btn, &g_astro_card_transport_style, LV_PART_MAIN);
    lv_obj_add_style(btn, &g_astro_card_transport_focus_style, LV_STATE_FOCUSED);
    lv_obj_add_style(btn, &g_astro_card_transport_focus_style, LV_STATE_FOCUS_KEY);
}

/* 底栏 MODE 提示条（蓝牙/TF 等共用：440×40 #DFDCD6 + 圆角 + M 徽标） */
static void divoom_astrootoo_source_create_mode_hint_bar(Pdivoom_astrootoo_source_ctrl phandle,
    const char *hint_key, uint8 show_m_badge, uint32 bg_color)
{
    lv_obj_t *bar;
    lv_obj_t *badge;
    lv_obj_t *badge_lbl;
    lv_coord_t card_w;
    lv_coord_t card_x;

    card_w = DIVOOM_ASTRO_BT_CARD_W;
    card_x = (DIVOOM_SCREEN_WIDTH - card_w) / 2;

    bar = lv_obj_create(phandle->content);
    lv_obj_set_size(bar, card_w, DIVOOM_ASTRO_BT_MODE_BAR_H);
    lv_obj_set_pos(bar, card_x, DIVOOM_ASTRO_BT_MODE_BAR_CONTENT_Y);
    lv_obj_set_style_bg_color(bar, lv_color_hex(bg_color), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_radius(bar, DIVOOM_ASTRO_RADIUS_SM, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    if (show_m_badge)
    {
        badge = lv_obj_create(bar);
        lv_obj_set_size(badge, 24, 24);
        lv_obj_align(badge, LV_ALIGN_LEFT_MID, 12, 0);
        lv_obj_set_style_bg_color(badge, lv_color_hex(DIVOOM_ASTRO_INK), 0);
        lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(badge, 0, 0);
        lv_obj_set_style_radius(badge, 6, 0);
        lv_obj_set_style_pad_all(badge, 0, 0);
        lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
        badge_lbl = lv_label_create(badge);
        lv_label_set_text(badge_lbl, "M");
        lv_obj_set_style_text_color(badge_lbl, lv_color_hex(DIVOOM_ASTRO_BG), 0);
        lv_obj_set_style_text_font(badge_lbl, divoom_menu_control_get_font_info(16, phandle->menu_id), 0);
        lv_obj_center(badge_lbl);
    }

    phandle->mode_hint_lbl = lv_label_create(bar);
    lv_label_set_text(phandle->mode_hint_lbl, divoom_menu_control_get_disp_str(hint_key));
    lv_obj_set_style_text_color(phandle->mode_hint_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    divoom_astrootoo_source_label_font(phandle->mode_hint_lbl, 16, phandle->menu_id);
    if (show_m_badge)
    {
        lv_obj_align(phandle->mode_hint_lbl, LV_ALIGN_LEFT_MID, 12 + 24 + 8, 0);
    }
    else
    {
        lv_obj_align(phandle->mode_hint_lbl, LV_ALIGN_CENTER, 0, 0);
    }
}

static void divoom_astrootoo_source_tf_apply_bar_style(lv_obj_t *bar, int active)
{
    if (!bar)
    {
        return;
    }
    lv_obj_set_style_bg_color(bar,
        lv_color_hex(active ? DIVOOM_ASTRO_TF_BAR_ACTIVE_BG : DIVOOM_ASTRO_SURFACE_IDLE), 0);
    lv_obj_set_style_border_color(bar,
        lv_color_hex(active ? DIVOOM_ASTRO_TAB_INK : DIVOOM_ASTRO_STROKE), 0);
}

static void divoom_astrootoo_source_tf_apply_icon_tone(lv_obj_t *img, int selected)
{
    if (!img)
    {
        return;
    }
    lv_obj_set_style_img_recolor(img,
        lv_color_hex(selected ? DIVOOM_ASTRO_INK : DIVOOM_ASTRO_TF_BAR_ICON_MUTED), 0);
    lv_obj_set_style_img_recolor_opa(img, LV_OPA_COVER, 0);
}

static void divoom_astrootoo_source_tf_refresh_bar_focus(Pdivoom_astrootoo_source_ctrl phandle)
{
    int i;
    int row0;

    if (!phandle)
    {
        return;
    }
    row0 = (phandle->tf_focus_row == 0) ? 1 : 0;
    divoom_astrootoo_source_tf_apply_bar_style(phandle->tf_transport_bar, row0);
    divoom_astrootoo_source_tf_apply_bar_style(phandle->tf_secondary_bar, !row0);
    for (i = 0; i < 3; i++)
    {
        divoom_astrootoo_source_tf_apply_icon_tone(phandle->tf_transport_imgs[i],
            row0 && ((int)phandle->tf_transport_focus == i));
        divoom_astrootoo_source_tf_apply_icon_tone(phandle->tf_secondary_imgs[i],
            !row0 && ((int)phandle->tf_secondary_focus == i));
    }
}

static void divoom_astrootoo_source_tf_focus_btn(Pdivoom_astrootoo_source_ctrl phandle)
{
    lv_obj_t *obj;

    if (!phandle)
    {
        return;
    }
    if (phandle->tf_focus_row == 0)
    {
        obj = phandle->tf_transport_btns[phandle->tf_transport_focus];
    }
    else
    {
        obj = phandle->tf_secondary_btns[phandle->tf_secondary_focus];
    }
    if (obj)
    {
        lv_group_focus_obj(obj);
    }
    divoom_astrootoo_source_tf_refresh_bar_focus(phandle);
}

static void divoom_astrootoo_source_tf_style_bar_btn(lv_obj_t *btn)
{
    lv_obj_remove_style_all(btn);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_STATE_FOCUSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_radius(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
}

static lv_obj_t *divoom_astrootoo_source_tf_create_bar(lv_obj_t *parent, lv_coord_t y)
{
    lv_obj_t *bar;

    bar = lv_obj_create(parent);
    lv_obj_set_size(bar, DIVOOM_ASTRO_TF_BAR_W, DIVOOM_ASTRO_TF_TRANSPORT_BTN_H);
    lv_obj_set_pos(bar, DIVOOM_ASTRO_TF_BAR_X, y);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 2, 0);
    lv_obj_set_style_radius(bar, DIVOOM_ASTRO_RADIUS_SM, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_set_style_pad_left(bar, DIVOOM_ASTRO_TF_BAR_PAD_X, 0);
    lv_obj_set_style_pad_right(bar, DIVOOM_ASTRO_TF_BAR_PAD_X, 0);
    lv_obj_set_style_pad_column(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_clip_corner(bar, true, 0);
    return bar;
}

static void divoom_astrootoo_source_tf_toast_hide(void)
{
    divoom_lvgl_top_toast_hide_timed();
}

static void divoom_astrootoo_source_tf_toast_show(const char *text, uint32_t duration_ms)
{
    divoom_lvgl_top_toast_show(text, duration_ms);
}

static void divoom_astrootoo_source_push_page(Pdivoom_astrootoo_source_ctrl phandle, divoom_astrootoo_source_page_t page)
{
    if (page == DIVOOM_ASTRO_SOURCE_PAGE_NET_RADIO && !divoom_main_net_radio_available())
    {
        return;
    }
    if (phandle->page_stack_depth < DIVOOM_ASTRO_SOURCE_PAGE_STACK_MAX)
    {
        phandle->page_stack[phandle->page_stack_depth] = phandle->page;
        phandle->page_stack_depth++;
    }
    divoom_astrootoo_source_show_page(phandle, page);
}

static void divoom_astrootoo_source_pop_page(Pdivoom_astrootoo_source_ctrl phandle)
{
    divoom_astrootoo_source_page_t prev;

    if (phandle->page_stack_depth > 0)
    {
        phandle->page_stack_depth--;
        prev = phandle->page_stack[phandle->page_stack_depth];
        divoom_astrootoo_source_show_page(phandle, prev);
        return;
    }
    divoom_menu_control_return_pre();
}

static void divoom_astrootoo_source_stop_refresh_timer(Pdivoom_astrootoo_source_ctrl phandle)
{
    if (phandle && phandle->refresh_timer)
    {
        lv_timer_del(phandle->refresh_timer);
        phandle->refresh_timer = NULL;
    }
}

static int divoom_astrootoo_source_is_card_playback_page(divoom_astrootoo_source_page_t page)
{
    if (page == DIVOOM_ASTRO_SOURCE_PAGE_BT)
    {
        return 1;
    }
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
    if (page == DIVOOM_ASTRO_SOURCE_PAGE_UAC)
    {
        return 1;
    }
#endif
#if DIVOOM_MUSIC_CTRL_LINEIN_ENABLE
    if (page == DIVOOM_ASTRO_SOURCE_PAGE_AUX)
    {
        return 1;
    }
#endif
    return 0;
}

static int divoom_astrootoo_source_page_for_mode(uint8 mode, divoom_astrootoo_source_page_t *out_page)
{
    if (!out_page)
    {
        return 0;
    }
    switch (mode)
    {
    case DIVOOM_MUSIC_CTRL_MODE_NET_FM:
    {
        *out_page = DIVOOM_ASTRO_SOURCE_PAGE_NET_RADIO;
        return divoom_main_net_radio_available();
    }
    case DIVOOM_MUSIC_CTRL_MODE_BT:
        *out_page = DIVOOM_ASTRO_SOURCE_PAGE_BT;
        return 1;
    case DIVOOM_MUSIC_CTRL_MODE_LOCAL:
        *out_page = DIVOOM_ASTRO_SOURCE_PAGE_TF;
        return 1;
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
    case DIVOOM_MUSIC_CTRL_MODE_UAC:
        *out_page = DIVOOM_ASTRO_SOURCE_PAGE_UAC;
        return 1;
#endif
#if DIVOOM_MUSIC_CTRL_LINEIN_ENABLE
    case DIVOOM_MUSIC_CTRL_MODE_LINEIN:
        *out_page = DIVOOM_ASTRO_SOURCE_PAGE_AUX;
        return 1;
#endif
    default:
        return 0;
    }
}

static int divoom_astrootoo_source_list_row_for_mode(uint8 mode)
{
    int i;

    for (i = 0; i < DIVOOM_ASTRO_SOURCE_MAIN_CNT; i++)
    {
        if (g_source_main_rows[i].music_mode == mode)
        {
            return i;
        }
    }
    return 0;
}

static void divoom_astrootoo_source_drop_tf_catalog(Pdivoom_astrootoo_source_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }
    divoom_astrootoo_source_tf_browse_reset_nav(phandle);
}

static void divoom_astrootoo_source_start_refresh_timer(Pdivoom_astrootoo_source_ctrl phandle,
    divoom_astrootoo_source_page_t page)
{
    uint32_t period_ms;

    if (!phandle)
    {
        return;
    }
    divoom_astrootoo_source_stop_refresh_timer(phandle);
    switch (page)
    {
    case DIVOOM_ASTRO_SOURCE_PAGE_NET_RADIO:
    {
        period_ms = 100;
        break;
    }
    case DIVOOM_ASTRO_SOURCE_PAGE_TF:
    {
        period_ms = DIVOOM_ASTRO_SOURCE_TF_REFRESH_MS;
        break;
    }
    case DIVOOM_ASTRO_SOURCE_PAGE_TF_HUB:
    case DIVOOM_ASTRO_SOURCE_PAGE_TF_TRACKS:
    {
        period_ms = DIVOOM_ASTRO_SOURCE_TF_CATALOG_MS;
        break;
    }
    default:
    {
        if (!divoom_astrootoo_source_is_card_playback_page(page))
        {
            return;
        }
        period_ms = DIVOOM_ASTRO_SOURCE_PLAYBACK_REFRESH_MS;
        break;
    }
    }
    phandle->refresh_timer = lv_timer_create(divoom_astrootoo_source_timer_cb, period_ms, phandle);
}

static void divoom_astrootoo_source_clear_content(Pdivoom_astrootoo_source_ctrl phandle,
    divoom_astrootoo_source_page_t next_page)
{
    int i;

    divoom_astrootoo_source_tf_toast_hide();
    if (phandle->page == DIVOOM_ASTRO_SOURCE_PAGE_NET_RADIO)
    {
        divoom_radio_set_menu_active(0);
        divoom_radio_menu_destroy(phandle->net_menu);
        phandle->net_menu = NULL;
    }

    if (phandle->refresh_timer)
    {
        divoom_astrootoo_source_stop_refresh_timer(phandle);
    }
    if (phandle->tf_vinyl_timer)
    {
        lv_timer_del(phandle->tf_vinyl_timer);
        phandle->tf_vinyl_timer = NULL;
    }
    phandle->tf_vinyl_disc = NULL;
    if (phandle->page == DIVOOM_ASTRO_SOURCE_PAGE_BT)
    {
        divoom_bt_music_lrc_set_callback(NULL, NULL);
        if (next_page != DIVOOM_ASTRO_SOURCE_PAGE_BT)
        {
            divoom_bt_bip_set_store(0);
        }
    }
    if (phandle->direct_handle)
    {
        divoom_menu_direct_ctrl_destory(phandle->direct_handle);
        phandle->direct_handle = NULL;
    }
    if (next_page != DIVOOM_ASTRO_SOURCE_PAGE_TF_HUB
        && next_page != DIVOOM_ASTRO_SOURCE_PAGE_TF_TRACKS)
    {
        divoom_astrootoo_source_tf_browse_reset_nav(phandle);
    }
    /* 先解开图片对缓冲的引用，再释放。缓冲只属于这一页 */
    if (phandle->bt_cover_img)
    {
        lv_img_set_src(phandle->bt_cover_img, NULL);
        phandle->bt_cover_img = NULL;
    }
    if (phandle->bt_cover_buf)
    {
        divoom_system_free(phandle->bt_cover_buf);
        phandle->bt_cover_buf = NULL;
    }
    phandle->bt_cover_gen = 0;
    if (phandle->content)
    {
        lv_obj_clean(phandle->content);
    }
    memset(phandle->tf_slot_row, 0, sizeof(phandle->tf_slot_row));
    memset(phandle->tf_slot_icon, 0, sizeof(phandle->tf_slot_icon));
    memset(phandle->tf_slot_label, 0, sizeof(phandle->tf_slot_label));
    memset(phandle->tf_slot_chevron, 0, sizeof(phandle->tf_slot_chevron));
    memset(phandle->tf_slot_check, 0, sizeof(phandle->tf_slot_check));
    for (i = 0; i < DIVOOM_ASTRO_SOURCE_MAIN_CNT; i++)
    {
        phandle->list_rows[i] = NULL;
    }
    phandle->card_cont = NULL;
    phandle->card_line1_lbl = NULL;
    phandle->card_line2_lbl = NULL;
    phandle->card_lyric_plate = NULL;
    phandle->card_divider = NULL;
    phandle->uac_wave[0] = NULL;
    phandle->uac_wave[1] = NULL;
    phandle->card_hint_lbl = NULL;
    phandle->mode_hint_lbl = NULL;
    memset(phandle->transport_btns, 0, sizeof(phandle->transport_btns));
    memset(phandle->transport_imgs, 0, sizeof(phandle->transport_imgs));
    phandle->tf_meta_cont = NULL;
    phandle->tf_title_lbl = NULL;
    phandle->tf_progress_bar = NULL;
    phandle->tf_progress_thumb = NULL;
    phandle->tf_time_pos_lbl = NULL;
    phandle->tf_time_dur_lbl = NULL;
    phandle->tf_transport_bar = NULL;
    phandle->tf_secondary_bar = NULL;
    memset(phandle->tf_transport_btns, 0, sizeof(phandle->tf_transport_btns));
    memset(phandle->tf_transport_imgs, 0, sizeof(phandle->tf_transport_imgs));
    memset(phandle->tf_secondary_btns, 0, sizeof(phandle->tf_secondary_btns));
    memset(phandle->tf_secondary_imgs, 0, sizeof(phandle->tf_secondary_imgs));


}

static const char *divoom_astrootoo_source_page_title_key(divoom_astrootoo_source_page_t page)
{
    switch (page)
    {
    case DIVOOM_ASTRO_SOURCE_PAGE_LIST:
    {
        return "AudioSource2";
    }
    case DIVOOM_ASTRO_SOURCE_PAGE_BT:
    {
        return "BluetoothPlaybackTitle";
    }
    case DIVOOM_ASTRO_SOURCE_PAGE_TF:
    {
        return "TFCard";
    }
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
    case DIVOOM_ASTRO_SOURCE_PAGE_UAC:
    {
        return "SourceUAC";
    }
#endif
#if DIVOOM_MUSIC_CTRL_LINEIN_ENABLE
    case DIVOOM_ASTRO_SOURCE_PAGE_AUX:
    {
        return "SourceAUX";
    }
#endif
    case DIVOOM_ASTRO_SOURCE_PAGE_NET_RADIO:
    {
        return "SourceNetRadio";
    }
    case DIVOOM_ASTRO_SOURCE_PAGE_TF_HUB:
    {
        return "LocalMusic";
    }
    case DIVOOM_ASTRO_SOURCE_PAGE_TF_TRACKS:
    {
        return "FolderBrowsing";
    }
    case DIVOOM_ASTRO_SOURCE_PAGE_TF_SCOPE:
    {
        return "NowPlaying";
    }
    case DIVOOM_ASTRO_SOURCE_PAGE_TF_ORDER:
    {
        return "PlaybackOrder";
    }
    case DIVOOM_ASTRO_SOURCE_PAGE_TF_EQ:
    {
        return "AudioEffects";
    }
    case DIVOOM_ASTRO_SOURCE_PAGE_TF_FAVORITES:
    {
        return "Favorites";
    }
    default:
    {
        return "AudioSource";
    }
    }
}

static void divoom_astrootoo_source_build_list(Pdivoom_astrootoo_source_ctrl phandle)
{
    int i;

    phandle->direct_handle = divoom_menu_direct_ctrl_create(1, DIVOOM_ASTRO_SOURCE_MAIN_CNT);
    for (i = 0; i < DIVOOM_ASTRO_SOURCE_MAIN_CNT; i++)
    {
        /* 电台是最后一行，关闭时保留现有音源的行坐标和导航索引。 */
        if (g_source_main_rows[i].page == DIVOOM_ASTRO_SOURCE_PAGE_NET_RADIO && !divoom_main_net_radio_available())
        {
            break;
        }
        phandle->list_rows[i] = divoom_astrootoo_source_create_list_row_icon(phandle, phandle->content,
            g_source_main_rows[i].label_key, g_source_main_rows[i].icon_file, i, phandle->direct_handle);
        lv_obj_add_flag(phandle->list_rows[i], LV_OBJ_FLAG_CLICK_FOCUSABLE);
    }
}

/* 蓝牙 / UAC 共用：面板外三键，等宽、高 64、间距 12 */
static void divoom_astrootoo_source_create_playback_transport(Pdivoom_astrootoo_source_ctrl phandle,
    lv_coord_t card_x, lv_coord_t card_w)
{
    lv_obj_t *bar;
    lv_obj_t *btn;
    lv_obj_t *img;
    int i;

    bar = lv_obj_create(phandle->content);
    lv_obj_set_size(bar, card_w, DIVOOM_ASTRO_PLAYBACK_TRANSPORT_H);
    lv_obj_set_pos(bar, card_x, DIVOOM_ASTRO_PLAYBACK_TRANSPORT_Y);
    lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_set_style_pad_column(bar, DIVOOM_ASTRO_PLAYBACK_TRANSPORT_GAP, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    for (i = 0; i < 3; i++)
    {
        btn = lv_btn_create(bar);
        divoom_astrootoo_source_apply_card_transport_btn_style(btn);
        lv_obj_set_height(btn, DIVOOM_ASTRO_PLAYBACK_TRANSPORT_H);
        lv_obj_set_flex_grow(btn, 1);
        phandle->transport_btns[i] = btn;
        img = lv_img_create(btn);
        phandle->transport_imgs[i] = img;
        if (i == 0)
        {
            divoom_menu_control_lv_img_set_src(img, DIVOOM_ASTROTOO_SOURCE_UI_DIR "source_transport_prev.png");
        }
        else if (i == 2)
        {
            divoom_menu_control_lv_img_set_src(img, DIVOOM_ASTROTOO_SOURCE_UI_DIR "source_transport_next.png");
        }
        lv_obj_center(img);
        divoom_astrootoo_source_attach_key(phandle, btn);
    }
}

static lv_obj_t *divoom_astrootoo_source_uac_box(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
    lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *obj;

    obj = lv_obj_create(parent);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    return obj;
}

static void divoom_astrootoo_source_uac_mark(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
    lv_coord_t w, lv_coord_t h, int outline, int radius)
{
    lv_obj_t *obj;

    obj = lv_obj_create(parent);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    if (outline)
    {
        lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(obj, lv_color_hex(DIVOOM_ASTRO_INK), 0);
        lv_obj_set_style_border_width(obj, 2, 0);
    }
    else
    {
        lv_obj_set_style_bg_color(obj, lv_color_hex(DIVOOM_ASTRO_INK), 0);
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(obj, 0, 0);
    }
}

static lv_obj_t *divoom_astrootoo_source_uac_wave(lv_obj_t *parent, lv_coord_t x, const int *counts)
{
    lv_obj_t *wave;
    int col;
    int i;

    wave = divoom_astrootoo_source_uac_box(parent, x, 0, 28, DIVOOM_ASTRO_UAC_HERO_H);
    for (col = 0; col < 4; col++)
    {
        int n = counts[col];
        int col_h = n * 4 + (n - 1) * 2;
        int y0 = (DIVOOM_ASTRO_UAC_HERO_H - col_h) / 2;

        for (i = 0; i < n; i++)
        {
            lv_obj_t *dot;

            dot = lv_obj_create(wave);
            lv_obj_set_size(dot, 4, 4);
            lv_obj_set_pos(dot, col * 8, y0 + i * 6);
            lv_obj_set_style_radius(dot, 0, 0);
            lv_obj_set_style_pad_all(dot, 0, 0);
            lv_obj_set_style_border_width(dot, 0, 0);
            lv_obj_set_style_bg_color(dot, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
            lv_obj_set_style_bg_opa(dot, LV_OPA_80, 0);
            lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE);
        }
    }
    return wave;
}

static void divoom_astrootoo_source_uac_build_hero(Pdivoom_astrootoo_source_ctrl phandle,
    lv_obj_t *hero, lv_coord_t card_w)
{
    lv_obj_t *halo;
    lv_obj_t *ring;
    lv_obj_t *usb;
    lv_coord_t origin;
    static const int wave_l[4] = { 2, 4, 6, 4 };
    static const int wave_r[4] = { 4, 6, 4, 2 };

    /* 左声波 28 + 间距 20 + 光圈 128 + 间距 20 + 右声波 28 */
    origin = (card_w - 224) / 2;
    phandle->uac_wave[0] = divoom_astrootoo_source_uac_wave(hero, origin, wave_l);
    phandle->uac_wave[1] = divoom_astrootoo_source_uac_wave(hero, origin + 28 + 20 + 128 + 20, wave_r);

    halo = divoom_astrootoo_source_uac_box(hero, origin + 28 + 20, 0, 128, 128);
    ring = lv_obj_create(halo);
    lv_obj_set_size(ring, 128, 128);
    lv_obj_center(ring);
    lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ring, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
    lv_obj_set_style_bg_opa(ring, (lv_opa_t)26, 0);
    lv_obj_set_style_border_color(ring, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
    lv_obj_set_style_border_opa(ring, (lv_opa_t)56, 0);
    lv_obj_set_style_border_width(ring, 2, 0);
    lv_obj_set_style_pad_all(ring, 0, 0);
    lv_obj_clear_flag(ring, LV_OBJ_FLAG_SCROLLABLE);
    ring = lv_obj_create(halo);
    lv_obj_set_size(ring, 96, 96);
    lv_obj_center(ring);
    lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ring, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
    lv_obj_set_style_bg_opa(ring, (lv_opa_t)36, 0);
    lv_obj_set_style_border_color(ring, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
    lv_obj_set_style_border_opa(ring, (lv_opa_t)77, 0);
    lv_obj_set_style_border_width(ring, 2, 0);
    lv_obj_set_style_pad_all(ring, 0, 0);
    lv_obj_clear_flag(ring, LV_OBJ_FLAG_SCROLLABLE);

    usb = divoom_astrootoo_source_uac_box(halo, 40, 32, 48, 64);
    divoom_astrootoo_source_uac_mark(usb, 15, 4, 18, 16, 1, 0);
    divoom_astrootoo_source_uac_mark(usb, 8, 20, 32, 40, 1, 4);
    divoom_astrootoo_source_uac_mark(usb, 19, 9, 4, 5, 0, 0);
    divoom_astrootoo_source_uac_mark(usb, 25, 9, 4, 5, 0, 0);
    divoom_astrootoo_source_uac_mark(usb, 22, 28, 4, 22, 0, 0);
    divoom_astrootoo_source_uac_mark(usb, 20, 30, 2, 2, 0, 0);
    divoom_astrootoo_source_uac_mark(usb, 26, 30, 2, 2, 0, 0);
    divoom_astrootoo_source_uac_mark(usb, 14, 36, 4, 6, 0, 0);
    divoom_astrootoo_source_uac_mark(usb, 14, 42, 10, 3, 0, 0);
    divoom_astrootoo_source_uac_mark(usb, 30, 34, 4, 6, 0, 0);
    divoom_astrootoo_source_uac_mark(usb, 24, 40, 10, 3, 0, 0);
    divoom_astrootoo_source_uac_mark(usb, 20, 50, 8, 4, 0, 0);
}

static void divoom_astrootoo_source_build_uac_playback(Pdivoom_astrootoo_source_ctrl phandle)
{
    lv_obj_t *card;
    lv_obj_t *hero;
    lv_obj_t *badge;
    lv_coord_t card_w;
    lv_coord_t card_x;

    phandle->playback_source = DIVOOM_MUSIC_CTRL_MODE_UAC;
    phandle->transport_focus = DIVOOM_ASTRO_SOURCE_TRANSPORT_PLAY;

    card_w = DIVOOM_ASTRO_BT_CARD_W;
    card_x = (DIVOOM_SCREEN_WIDTH - card_w) / 2;

    card = lv_obj_create(phandle->content);
    lv_obj_set_size(card, card_w, DIVOOM_ASTRO_UAC_CARD_H);
    lv_obj_set_pos(card, card_x, DIVOOM_ASTRO_BT_CARD_CONTENT_Y);
    lv_obj_set_style_bg_color(card, lv_color_hex(DIVOOM_ASTRO_UAC_CARD_BG), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(DIVOOM_ASTRO_UAC_CARD_BORDER), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, DIVOOM_ASTRO_RADIUS_MD, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    phandle->card_cont = card;

    hero = divoom_astrootoo_source_uac_box(card, 0, DIVOOM_ASTRO_UAC_HERO_Y, card_w, DIVOOM_ASTRO_UAC_HERO_H);
    divoom_astrootoo_source_uac_build_hero(phandle, hero, card_w);

    phandle->card_line1_lbl = lv_label_create(card);
    lv_obj_set_width(phandle->card_line1_lbl, card_w - 48);
    lv_obj_set_pos(phandle->card_line1_lbl, 24, DIVOOM_ASTRO_UAC_TITLE_Y);
    lv_obj_set_style_text_align(phandle->card_line1_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(phandle->card_line1_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(phandle->card_line1_lbl,
        divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
    lv_label_set_long_mode(phandle->card_line1_lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(phandle->card_line1_lbl, divoom_menu_control_get_disp_str("SourceUacIdleCaption"));

    badge = lv_obj_create(card);
    lv_obj_set_height(badge, DIVOOM_ASTRO_UAC_BADGE_H);
    lv_obj_set_width(badge, LV_SIZE_CONTENT);
    lv_obj_set_style_min_width(badge, DIVOOM_ASTRO_UAC_BADGE_MIN_W, 0);
    lv_obj_set_style_pad_hor(badge, DIVOOM_ASTRO_UAC_BADGE_PAD_X, 0);
    lv_obj_align(badge, LV_ALIGN_TOP_MID, 0, DIVOOM_ASTRO_UAC_BADGE_Y);
    lv_obj_set_style_bg_color(badge, lv_color_hex(DIVOOM_ASTRO_BG_MUTED), 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(badge, 0, 0);
    lv_obj_set_style_radius(badge, DIVOOM_ASTRO_RADIUS_SM, 0);
    lv_obj_set_style_pad_all(badge, 0, 0);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
    phandle->card_lyric_plate = badge;

    phandle->card_line2_lbl = lv_label_create(badge);
    lv_obj_set_style_text_color(phandle->card_line2_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    divoom_astrootoo_source_label_font(phandle->card_line2_lbl, 16, phandle->menu_id);
    lv_label_set_text(phandle->card_line2_lbl, divoom_menu_control_get_disp_str("ConnectedDevices"));
    lv_obj_center(phandle->card_line2_lbl);

    divoom_astrootoo_source_create_playback_transport(phandle, card_x, card_w);

    divoom_astrootoo_source_create_mode_hint_bar(phandle, "SourceUacModeHint", 1,
        DIVOOM_ASTRO_BT_MODE_BAR_BG);
#if defined(DIVOOM_TIMES_VIBE)
    (void)divoom_music_ctrl_sync_play_status_from_jl();
#endif
    divoom_astrootoo_source_refresh_playback_ui(phandle);
}

/*
 * 蓝牙封面 JPEG 走 divoom_jpeg_decode 软解（TimesGate 同款，RGB888），再转 RGB565。
 * 其它 JPEG 仍走全志硬解。解码后按宽高比缩进封面格，多出来的边用封面底色填上。
 */
static int divoom_astrootoo_source_bt_cover_sof(const uint8_t *jpeg, uint32_t jpeg_len,
    int *width, int *height)
{
    uint32_t i;

    if (!jpeg || jpeg_len < 4u || !width || !height)
    {
        return 0;
    }
    if (jpeg[0] != 0xFFu || jpeg[1] != 0xD8u)
    {
        return 0;
    }
    i = 2u;
    while (i + 8u < jpeg_len)
    {
        uint8_t marker;
        uint32_t seglen;

        if (jpeg[i] != 0xFFu)
        {
            return 0;
        }
        while (i < jpeg_len && jpeg[i] == 0xFFu)
        {
            i++;
        }
        if (i >= jpeg_len)
        {
            return 0;
        }
        marker = jpeg[i++];
        if (marker == 0xD8u || marker == 0xD9u || (marker >= 0xD0u && marker <= 0xD7u))
        {
            continue;
        }
        if (marker == 0xDAu || i + 1u >= jpeg_len)
        {
            return 0;
        }
        seglen = ((uint32_t)jpeg[i] << 8) | jpeg[i + 1u];
        if (seglen < 2u || i + seglen > jpeg_len)
        {
            return 0;
        }
        /* SOF0 / SOF2 */
        if ((marker == 0xC0u || marker == 0xC2u) && seglen >= 7u)
        {
            *height = ((int)jpeg[i + 3u] << 8) | jpeg[i + 4u];
            *width = ((int)jpeg[i + 5u] << 8) | jpeg[i + 6u];
            return (*width > 0 && *height > 0);
        }
        i += seglen;
    }
    return 0;
}

/* 整张缩进 96 方格，宽高比保持。用邻近采样，不走 lv_img_set_zoom */
static void divoom_astrootoo_source_bt_cover_fit(uint8_t *dst, const uint8_t *src,
    int sw, int sh)
{
    int dw;
    int dh;
    int ox;
    int oy;
    int y;
    int x;

    if (sw >= sh)
    {
        dw = DIVOOM_ASTRO_BT_COVER_SZ;
        dh = sh * DIVOOM_ASTRO_BT_COVER_SZ / sw;
    }
    else
    {
        dh = DIVOOM_ASTRO_BT_COVER_SZ;
        dw = sw * DIVOOM_ASTRO_BT_COVER_SZ / sh;
    }
    if (dw < 1)
    {
        dw = 1;
    }
    if (dh < 1)
    {
        dh = 1;
    }
    ox = (DIVOOM_ASTRO_BT_COVER_SZ - dw) / 2;
    oy = (DIVOOM_ASTRO_BT_COVER_SZ - dh) / 2;
    for (y = 0; y < dh; y++)
    {
        int sy = y * sh / dh;

        if (sy >= sh)
        {
            sy = sh - 1;
        }
        for (x = 0; x < dw; x++)
        {
            int sx = x * sw / dw;
            const uint8_t *sp;
            uint8_t *dp;

            if (sx >= sw)
            {
                sx = sw - 1;
            }
            sp = src + ((uint32_t)sy * (uint32_t)sw + (uint32_t)sx) * DIVOOM_APP_RGB565_BYTES_PER_PX;
            dp = dst + ((uint32_t)(oy + y) * DIVOOM_ASTRO_BT_COVER_SZ + (uint32_t)(ox + x))
                * DIVOOM_APP_RGB565_BYTES_PER_PX;
            dp[0] = sp[0];
            dp[1] = sp[1];
        }
    }
}

static uint8_t *divoom_astrootoo_source_bt_cover_decode(const uint8_t *jpeg, uint32_t jpeg_len)
{
    int width;
    int height;
    int i;
    uint8_t *rgb888;
    uint8_t *src;
    uint8_t *out;
    uint32_t out_bytes;
    uint32_t rgb888_len;
    uint32_t decoded_len;
    uint16_t bg;

    if (!divoom_astrootoo_source_bt_cover_sof(jpeg, jpeg_len, &width, &height))
    {
        return NULL;
    }
    /* 软解 img_width/img_height 为 uint8，超过 255 解不了；过大也放弃继续显示黑胶 */
    if (width > 255 || height > 255)
    {
        return NULL;
    }

    rgb888_len = (uint32_t)width * (uint32_t)height * 3u;
    rgb888 = (uint8_t *)divoom_system_malloc(rgb888_len);
    if (!rgb888)
    {
        return NULL;
    }
    decoded_len = 0u;
    if (!divoom_jpeg_decompress((uint8 *)jpeg, jpeg_len, rgb888, &decoded_len))
    {
        divoom_system_free(rgb888);
        return NULL;
    }

    src = (uint8_t *)divoom_system_malloc(
        divoom_pixel_format_size(width, height, LV_COLOR_FORMAT_RGB565));
    if (!src)
    {
        divoom_system_free(rgb888);
        return NULL;
    }
    divoom_rgb888_to_rgb565_buf(rgb888, src, (uint32_t)width * (uint32_t)height);
    divoom_system_free(rgb888);

    out_bytes = divoom_pixel_format_size(DIVOOM_ASTRO_BT_COVER_SZ, DIVOOM_ASTRO_BT_COVER_SZ,
        LV_COLOR_FORMAT_RGB565);
    if (out_bytes == 0u)
    {
        divoom_system_free(src);
        return NULL;
    }
    out = (uint8_t *)divoom_system_malloc(out_bytes);
    if (!out)
    {
        divoom_system_free(src);
        return NULL;
    }
    bg = divoom_rgb888_to_rgb565(0xA1, 0xA1, 0xA1);
    for (i = 0; i < DIVOOM_ASTRO_BT_COVER_SZ * DIVOOM_ASTRO_BT_COVER_SZ; i++)
    {
        divoom_store_rgb565_le(out + (uint32_t)i * 2u, bg);
    }
    divoom_astrootoo_source_bt_cover_fit(out, src, width, height);
    divoom_system_free(src);
    return out;
}

static void divoom_astrootoo_source_bt_cover_vinyl(Pdivoom_astrootoo_source_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }
    if (phandle->bt_cover_img)
    {
        lv_img_set_src(phandle->bt_cover_img, NULL);
        lv_obj_add_flag(phandle->bt_cover_img, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->bt_cover_buf)
    {
        divoom_system_free(phandle->bt_cover_buf);
        phandle->bt_cover_buf = NULL;
    }
}

static void divoom_astrootoo_source_bt_cover_apply(Pdivoom_astrootoo_source_ctrl phandle)
{
    uint8_t *jpeg;
    uint8_t *pixels;
    uint32_t jpeg_len;
    uint32_t gen;
    int kind;

    if (!phandle || !phandle->bt_cover_img)
    {
        return;
    }
    jpeg = NULL;
    jpeg_len = 0;
    gen = 0;
    kind = divoom_bt_bip_copy(phandle->bt_cover_gen, &jpeg, &jpeg_len, &gen);
    if (kind == DIVOOM_BT_BIP_UI_UNCHANGED)
    {
        return;
    }
    if (kind == DIVOOM_BT_BIP_UI_VINYL)
    {
        divoom_astrootoo_source_bt_cover_vinyl(phandle);
        phandle->bt_cover_gen = gen;
        return;
    }

    pixels = divoom_astrootoo_source_bt_cover_decode(jpeg, jpeg_len);
    divoom_system_free(jpeg);
    phandle->bt_cover_gen = gen;
    if (!pixels)
    {
        divoom_astrootoo_source_bt_cover_vinyl(phandle);
        return;
    }
    lv_img_set_src(phandle->bt_cover_img, NULL);
    if (phandle->bt_cover_buf)
    {
        divoom_system_free(phandle->bt_cover_buf);
    }
    phandle->bt_cover_buf = pixels;
    divoom_lv_img_dsc_init_fmt(&phandle->bt_cover_dsc, DIVOOM_ASTRO_BT_COVER_SZ,
        DIVOOM_ASTRO_BT_COVER_SZ, LV_COLOR_FORMAT_RGB565, pixels);
    lv_img_set_src(phandle->bt_cover_img, &phandle->bt_cover_dsc);
    lv_obj_clear_flag(phandle->bt_cover_img, LV_OBJ_FLAG_HIDDEN);
}

/* TF 黑胶整组是 136。缩进封面格，唱臂不单独重画。 */
static void divoom_astrootoo_source_bt_place_vinyl(Pdivoom_astrootoo_source_ctrl phandle, lv_obj_t *cover)
{
    lv_obj_t *slot;
    int16_t zoom;

    divoom_astrootoo_source_tf_vinyl_build(phandle, cover, 0);
    if (!phandle->tf_vinyl_disc)
    {
        return;
    }
    slot = lv_obj_get_parent(phandle->tf_vinyl_disc);
    if (!slot)
    {
        return;
    }
    zoom = (int16_t)((DIVOOM_ASTRO_BT_COVER_SZ * 256) / DIVOOM_ASTRO_TF_VINYL_PX);
    lv_obj_set_pos(slot,
        (DIVOOM_ASTRO_BT_COVER_SZ - DIVOOM_ASTRO_TF_VINYL_PX) / 2,
        (DIVOOM_ASTRO_BT_COVER_SZ - DIVOOM_ASTRO_TF_VINYL_PX) / 2);
    lv_obj_set_style_transform_pivot_x(slot, DIVOOM_ASTRO_TF_VINYL_PX / 2, 0);
    lv_obj_set_style_transform_pivot_y(slot, DIVOOM_ASTRO_TF_VINYL_PX / 2, 0);
    lv_obj_set_style_transform_zoom(slot, zoom, 0);
}

static void divoom_astrootoo_source_build_bt_playback(Pdivoom_astrootoo_source_ctrl phandle)
{
    lv_obj_t *card;
    lv_obj_t *cover;
    lv_obj_t *lyric_box;
    lv_coord_t card_w;
    lv_coord_t card_x;

    phandle->playback_source = DIVOOM_MUSIC_CTRL_MODE_BT;
    phandle->transport_focus = DIVOOM_ASTRO_SOURCE_TRANSPORT_PLAY;

    card_w = DIVOOM_ASTRO_BT_CARD_W;
    card_x = (DIVOOM_SCREEN_WIDTH - card_w) / 2;

    card = lv_obj_create(phandle->content);
    lv_obj_set_size(card, card_w, DIVOOM_ASTRO_PLAYBACK_CARD_H);
    lv_obj_set_pos(card, card_x, DIVOOM_ASTRO_BT_CARD_CONTENT_Y);
    lv_obj_set_style_bg_color(card, lv_color_hex(DIVOOM_ASTRO_BT_CARD_BG), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(DIVOOM_ASTRO_PLAYBACK_CARD_BORDER), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, DIVOOM_ASTRO_RADIUS_MD, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    phandle->card_cont = card;

    cover = lv_obj_create(card);
    lv_obj_set_size(cover, DIVOOM_ASTRO_BT_COVER_SZ, DIVOOM_ASTRO_BT_COVER_SZ);
    lv_obj_align(cover, LV_ALIGN_TOP_MID, 0, DIVOOM_ASTRO_BT_COVER_Y);
    lv_obj_set_style_bg_color(cover, lv_color_hex(DIVOOM_ASTRO_BT_COVER_BG), 0);
    lv_obj_set_style_bg_opa(cover, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(cover, 0, 0);
    lv_obj_set_style_radius(cover, DIVOOM_ASTRO_BT_COVER_R, 0);
    lv_obj_set_style_pad_all(cover, 0, 0);
    lv_obj_set_style_clip_corner(cover, true, 0);
    lv_obj_clear_flag(cover, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(cover, LV_OBJ_FLAG_CLICKABLE);
    divoom_astrootoo_source_bt_place_vinyl(phandle, cover);

    phandle->bt_cover_img = lv_img_create(cover);
    lv_obj_set_size(phandle->bt_cover_img, DIVOOM_ASTRO_BT_COVER_SZ, DIVOOM_ASTRO_BT_COVER_SZ);
    lv_obj_center(phandle->bt_cover_img);
    lv_obj_clear_flag(phandle->bt_cover_img, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(phandle->bt_cover_img, LV_OBJ_FLAG_HIDDEN);

    phandle->card_line1_lbl = lv_label_create(card);
    lv_obj_set_width(phandle->card_line1_lbl, card_w - 48);
    lv_obj_set_pos(phandle->card_line1_lbl, 24, DIVOOM_ASTRO_BT_SONG_ONLY_Y);
    lv_obj_set_style_text_align(phandle->card_line1_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(phandle->card_line1_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(phandle->card_line1_lbl,
        divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
    lv_label_set_long_mode(phandle->card_line1_lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    phandle->card_divider = NULL;

    lyric_box = lv_obj_create(card);
    lv_obj_set_size(lyric_box, card_w - 40, DIVOOM_ASTRO_BT_LYRIC_H);
    lv_obj_set_pos(lyric_box, 20, DIVOOM_ASTRO_BT_LYRIC_Y);
    lv_obj_set_style_bg_opa(lyric_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(lyric_box, 0, 0);
    lv_obj_set_style_pad_all(lyric_box, 0, 0);
    lv_obj_clear_flag(lyric_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(lyric_box, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_flow(lyric_box, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(lyric_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    phandle->card_line2_lbl = lv_label_create(lyric_box);
    lv_obj_set_width(phandle->card_line2_lbl, card_w - 40);
    lv_label_set_long_mode(phandle->card_line2_lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(phandle->card_line2_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(phandle->card_line2_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(phandle->card_line2_lbl,
        divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
    lv_obj_add_flag(phandle->card_line2_lbl, LV_OBJ_FLAG_HIDDEN);

    divoom_astrootoo_source_create_playback_transport(phandle, card_x, card_w);
    divoom_astrootoo_source_create_mode_hint_bar(phandle, "SourceBtModeHint", 1,
        DIVOOM_ASTRO_BT_MODE_BAR_BG);

    phandle->bt_line1[0] = '\0';
    phandle->bt_line2[0] = '\0';
    phandle->bt_line1_dirty = 0;
    phandle->bt_line2_dirty = 0;
    divoom_bt_music_lrc_set_callback(divoom_astrootoo_source_bt_lrc_cb, phandle);
    divoom_astrootoo_source_refresh_playback_ui(phandle);
}

static void divoom_astrootoo_source_build_card_playback(Pdivoom_astrootoo_source_ctrl phandle, uint8 mode)
{
    /* 几何真源：AstroTooUI sourceBluetoothPlaybackLayout.ts + SourcePlaybackInfoPanel.module.css */
    lv_obj_t *card;
    lv_obj_t *plate;
    lv_obj_t *btn;
    lv_obj_t *img;
    lv_coord_t card_w;
    lv_coord_t card_x;
    lv_coord_t tx;
    lv_coord_t btn_w;
    lv_coord_t inner_w;
    int i;
    const char *hint_key;

    phandle->playback_source = mode;
    phandle->transport_focus = DIVOOM_ASTRO_SOURCE_TRANSPORT_PLAY;
    if (mode == DIVOOM_MUSIC_CTRL_MODE_UAC)
    {
        divoom_astrootoo_source_build_uac_playback(phandle);
        return;
    }
    if (mode == DIVOOM_MUSIC_CTRL_MODE_BT)
    {
        divoom_astrootoo_source_build_bt_playback(phandle);
        return;
    }

    card_w = DIVOOM_ASTRO_BT_CARD_W;
    card_x = (DIVOOM_SCREEN_WIDTH - card_w) / 2;
    inner_w = card_w - DIVOOM_ASTRO_BT_CARD_PAD_X * 2;

    card = lv_obj_create(phandle->content);
    lv_obj_set_size(card, card_w, DIVOOM_ASTRO_BT_CARD_H);
    lv_obj_set_pos(card, card_x, DIVOOM_ASTRO_BT_CARD_CONTENT_Y);
    lv_obj_set_style_bg_color(card, lv_color_hex(DIVOOM_ASTRO_BG), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, DIVOOM_ASTRO_RADIUS_MD, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    phandle->card_cont = card;

    /* 歌名 · 卡内 y=87（屏 y163），32 号；超长单行滚动，不折行以免压歌词 */
    phandle->card_line1_lbl = lv_label_create(card);
    lv_obj_set_style_text_align(phandle->card_line1_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(phandle->card_line1_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(phandle->card_line1_lbl, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
    divoom_astrootoo_ui_list_label_prepare(phandle->card_line1_lbl, inner_w);
    lv_obj_set_pos(phandle->card_line1_lbl, DIVOOM_ASTRO_BT_CARD_PAD_X, DIVOOM_ASTRO_BT_TITLE_CARD_Y);

    /* 歌词灰盒 · 卡内 y=106 高 72（屏 y182），24 号居中 */
    plate = lv_obj_create(card);
    lv_obj_set_size(plate, inner_w, DIVOOM_ASTRO_BT_LYRIC_PLATE_H);
    lv_obj_set_pos(plate, DIVOOM_ASTRO_BT_CARD_PAD_X, DIVOOM_ASTRO_BT_LYRIC_PLATE_CARD_Y);
    lv_obj_set_style_bg_color(plate, lv_color_hex(DIVOOM_ASTRO_BG_MUTED), 0);
    lv_obj_set_style_bg_opa(plate, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(plate, 0, 0);
    lv_obj_set_style_radius(plate, DIVOOM_ASTRO_RADIUS_SM, 0);
    lv_obj_set_style_pad_all(plate, 0, 0);
    lv_obj_set_style_clip_corner(plate, true, 0);
    lv_obj_clear_flag(plate, LV_OBJ_FLAG_SCROLLABLE);
    phandle->card_lyric_plate = plate;

    phandle->card_line2_lbl = lv_label_create(plate);
    lv_obj_set_style_text_align(phandle->card_line2_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(phandle->card_line2_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    divoom_astrootoo_source_label_font(phandle->card_line2_lbl, 24, phandle->menu_id);
    divoom_astrootoo_ui_list_label_prepare(phandle->card_line2_lbl, inner_w - 24);
    lv_obj_center(phandle->card_line2_lbl);

    /* 三键 · 卡内 y=210（屏 y286），72 / 86 / 72，间距 12 */
    tx = (card_w - (DIVOOM_ASTRO_BT_TRANSPORT_SIDE_W + DIVOOM_ASTRO_BT_TRANSPORT_GAP
        + DIVOOM_ASTRO_BT_TRANSPORT_CENTER_W + DIVOOM_ASTRO_BT_TRANSPORT_GAP
        + DIVOOM_ASTRO_BT_TRANSPORT_SIDE_W)) / 2;
    for (i = 0; i < 3; i++)
    {
        btn_w = (i == 1) ? DIVOOM_ASTRO_BT_TRANSPORT_CENTER_W : DIVOOM_ASTRO_BT_TRANSPORT_SIDE_W;
        btn = lv_btn_create(card);
        divoom_astrootoo_source_apply_card_transport_btn_style(btn);
        lv_obj_set_size(btn, btn_w, DIVOOM_ASTRO_BT_TRANSPORT_H);
        lv_obj_set_pos(btn, tx, DIVOOM_ASTRO_BT_TRANSPORT_CARD_Y);
        tx += btn_w + DIVOOM_ASTRO_BT_TRANSPORT_GAP;
        phandle->transport_btns[i] = btn;
        img = lv_img_create(btn);
        phandle->transport_imgs[i] = img;
        if (i == 0)
        {
            divoom_menu_control_lv_img_set_src(img, DIVOOM_ASTROTOO_SOURCE_UI_DIR "source_transport_prev.png");
        }
        else if (i == 2)
        {
            divoom_menu_control_lv_img_set_src(img, DIVOOM_ASTROTOO_SOURCE_UI_DIR "source_transport_next.png");
        }
        lv_obj_center(img);
        divoom_astrootoo_source_attach_key(phandle, btn);
    }

    hint_key = "SourcePlaybackHint";
    if (mode == DIVOOM_MUSIC_CTRL_MODE_LINEIN)
    {
#if DIVOOM_MUSIC_CTRL_LINEIN_ENABLE
        hint_key = "SourceAuxModeHint";
#endif
    }

    divoom_astrootoo_source_create_mode_hint_bar(phandle, hint_key, 1,
        DIVOOM_ASTRO_BT_MODE_BAR_BG);
    divoom_astrootoo_source_refresh_playback_ui(phandle);
}

/* 32 号字的 line_height 贴着字面，标签按这个高度裁切时 y/q 的下沿会被切掉。 */
static void divoom_astrootoo_source_bt_title_fit(lv_obj_t *lbl)
{
    const lv_font_t *font;
    lv_coord_t line_h;
    lv_coord_t box_h;

    if (!lbl)
    {
        return;
    }
    font = lv_obj_get_style_text_font(lbl, 0);
    line_h = font ? (lv_coord_t)lv_font_get_line_height(font) : 32;
    box_h = line_h + line_h / 5;
    if (box_h < line_h + 4)
    {
        box_h = line_h + 4;
    }
    lv_obj_set_height(lbl, box_h);
    lv_obj_set_style_max_height(lbl, box_h, 0);
}

static void divoom_astrootoo_source_card_set_scroll_line(lv_obj_t *lbl, const char *text)
{
    const char *cur;

    if (!lbl)
    {
        return;
    }
    if (!text)
    {
        text = "";
    }
    cur = lv_label_get_text(lbl);
    if (cur && strcmp(cur, text) == 0)
    {
        return;
    }
    lv_label_set_text(lbl, text);
    divoom_astrootoo_ui_list_label_marquee_always(lbl);
}

static void divoom_astrootoo_source_refresh_playback_ui(Pdivoom_astrootoo_source_ctrl phandle)
{
    int playing;
    const char *line1;
    const char *line2;
    uint8 plugged;

    if (!phandle->card_line1_lbl)
    {
        return;
    }

    playing = divoom_music_ctrl_is_playing();
    divoom_astrootoo_source_set_transport_img(phandle, phandle->transport_imgs[1], playing, 1);

    if (phandle->playback_source == DIVOOM_MUSIC_CTRL_MODE_UAC)
    {
        lv_label_set_text(phandle->card_line1_lbl, divoom_menu_control_get_disp_str("SourceUacIdleCaption"));
        if (phandle->card_line2_lbl)
        {
            lv_label_set_text(phandle->card_line2_lbl, divoom_menu_control_get_disp_str("ConnectedDevices"));
        }
        if (phandle->card_lyric_plate)
        {
            lv_obj_clear_flag(phandle->card_lyric_plate, LV_OBJ_FLAG_HIDDEN);
        }
        if (phandle->uac_wave[0])
        {
            lv_opa_t wave_opa = playing ? LV_OPA_COVER : (lv_opa_t)140;

            lv_obj_set_style_opa(phandle->uac_wave[0], wave_opa, 0);
            lv_obj_set_style_opa(phandle->uac_wave[1], wave_opa, 0);
        }
        return;
    }

    if (phandle->playback_source == DIVOOM_MUSIC_CTRL_MODE_BT)
    {
        int has_lyric;

        divoom_astrootoo_source_bt_cover_apply(phandle);
        if (!divoom_main_get_audio_type_statu(3))
        {
            if (divoom_tws_host_is_right_linked())
            {
                line1 = divoom_menu_control_get_disp_str("RightChannel");
            }
            else
            {
                line1 = divoom_menu_control_get_disp_str("BluetoothIdleTitle");
            }
            line2 = "";
        }
        else
        {
            if (phandle->bt_line1_dirty)
            {
                phandle->bt_line1_dirty = 0;
            }
            line1 = phandle->bt_line1[0] ? phandle->bt_line1 : divoom_menu_control_get_disp_str("UnknownMusicName");
            line2 = phandle->bt_line2;
        }
        has_lyric = (line2 && line2[0]) ? 1 : 0;
        lv_obj_set_style_text_font(phandle->card_line1_lbl,
            divoom_menu_control_get_font_info(has_lyric ? 24 : 32, phandle->menu_id), 0);
        lv_obj_set_y(phandle->card_line1_lbl,
            has_lyric ? DIVOOM_ASTRO_BT_SONG_Y : DIVOOM_ASTRO_BT_SONG_ONLY_Y);
        divoom_astrootoo_source_card_set_scroll_line(phandle->card_line1_lbl, line1);
        divoom_astrootoo_source_bt_title_fit(phandle->card_line1_lbl);
        if (phandle->card_line2_lbl)
        {
            lv_obj_t *lyric_box;

            lyric_box = lv_obj_get_parent(phandle->card_line2_lbl);
            if (has_lyric)
            {
                lv_obj_set_width(phandle->card_line2_lbl, DIVOOM_ASTRO_BT_CARD_W - 40);
                divoom_astrootoo_source_card_set_scroll_line(phandle->card_line2_lbl, line2);
                divoom_astrootoo_source_bt_title_fit(phandle->card_line2_lbl);
                lv_obj_clear_flag(phandle->card_line2_lbl, LV_OBJ_FLAG_HIDDEN);
                if (lyric_box)
                {
                    lv_obj_clear_flag(lyric_box, LV_OBJ_FLAG_HIDDEN);
                }
            }
            else if (lyric_box)
            {
                lv_obj_add_flag(lyric_box, LV_OBJ_FLAG_HIDDEN);
            }
        }
        return;
    }
    else if (phandle->playback_source == DIVOOM_MUSIC_CTRL_MODE_LINEIN)
    {
#if DIVOOM_MUSIC_CTRL_LINEIN_ENABLE
        plugged = divoom_main_get_audio_type_statu(1);
        line1 = plugged ? divoom_menu_control_get_disp_str("AUXFullName") : divoom_menu_control_get_disp_str("SourceAuxIdleCaption");
        line2 = "";
#else
        line1 = "";
        line2 = "";
#endif
    }
    else
    {
        line1 = "";
        line2 = "";
    }

    divoom_astrootoo_source_card_set_scroll_line(phandle->card_line1_lbl, line1);
    if (phandle->card_line2_lbl)
    {
        divoom_astrootoo_source_card_set_scroll_line(phandle->card_line2_lbl, line2);
    }
    /* 无歌词时隐藏灰盒（未连接 / AUX / UAC 状态） */
    if (phandle->card_lyric_plate)
    {
        if (line2 && line2[0])
        {
            lv_obj_clear_flag(phandle->card_lyric_plate, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(phandle->card_lyric_plate, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void divoom_astrootoo_source_on_play_status_changed(void)
{
    Pdivoom_astrootoo_source_ctrl phandle;

    /* JL 0x32 在 UART 线程投递，禁止直接操作 LVGL；由 refresh_timer 在 LVGL 上下文刷新 */
    phandle = g_astrootoo_source_handle;
    if (!phandle || !divoom_astrootoo_source_is_card_playback_page(phandle->page))
    {
        return;
    }
    phandle->playback_ui_dirty = 1;
}

static void divoom_astrootoo_source_refresh_tf_ui(Pdivoom_astrootoo_source_ctrl phandle)
{
    Pdivoom_tf_music_item item;
    Pdivoom_music_ctrl_tf_play_info tf_info;
    char tbuf[16];
    int pos;
    int dur;
    int pct;
    int playing;

    if (phandle->page != DIVOOM_ASTRO_SOURCE_PAGE_TF || !phandle->tf_title_lbl)
    {
        return;
    }

    item = divoom_tf_music_get_playing_music_info();
    if (item && item->music_name)
    {
        char name_buf[256];

        strncpy(name_buf, item->music_name, sizeof(name_buf) - 1);
        name_buf[sizeof(name_buf) - 1] = '\0';
        lv_label_set_text(phandle->tf_title_lbl, divoom_astrootoo_source_path_basename(name_buf));
    }
    else
    {
        lv_label_set_text(phandle->tf_title_lbl, divoom_menu_control_get_disp_str("UnknownMusicName"));
    }

    tf_info = divoom_music_ctrl_get_tf_info();
    if (tf_info != NULL)
    {
        pos = (int)tf_info->cur_time;
        dur = (int)tf_info->totle_time;
        playing = (tf_info->status != 0) ? 1 : 0;
    }
    else
    {
        pos = 0;
        dur = 0;
        playing = divoom_music_ctrl_is_playing();
    }
    divoom_astrootoo_source_set_transport_img(phandle, phandle->tf_transport_imgs[1], playing, 1);
    divoom_astrootoo_source_tf_vinyl_set_spin(phandle, playing);
    if (dur > 0 && pos > dur)
    {
        pos = dur;
    }
    if (dur > 0)
    {
        pct = pos * 100 / dur;
        if (pct > 100)
        {
            pct = 100;
        }
    }
    else
    {
        pct = 0;
    }
    if (phandle->tf_progress_bar)
    {
        lv_bar_set_value(phandle->tf_progress_bar, pct, LV_ANIM_OFF);
        if (pct >= 100)
        {
            lv_obj_set_style_radius(phandle->tf_progress_bar, 4, LV_PART_INDICATOR);
        }
        else
        {
            lv_obj_set_style_radius(phandle->tf_progress_bar, 4, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        }
    }
    if (phandle->tf_progress_thumb)
    {
        lv_coord_t thumb_x;

        if (pct > 0 && pct < 100)
        {
            thumb_x = DIVOOM_ASTRO_TF_INFO_X
                + (DIVOOM_ASTRO_TF_INFO_W * pct / 100) - 6;
            lv_obj_clear_flag(phandle->tf_progress_thumb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(phandle->tf_progress_thumb, thumb_x, DIVOOM_ASTRO_TF_PROGRESS_Y);
        }
        else
        {
            lv_obj_add_flag(phandle->tf_progress_thumb, LV_OBJ_FLAG_HIDDEN);
        }
    }
    divoom_astrootoo_source_fmt_mmss(tbuf, sizeof(tbuf), pos);
    lv_label_set_text(phandle->tf_time_pos_lbl, tbuf);
    divoom_astrootoo_source_fmt_mmss(tbuf, sizeof(tbuf), dur);
    lv_label_set_text(phandle->tf_time_dur_lbl, tbuf);

    if (phandle->tf_secondary_imgs[DIVOOM_ASTRO_SOURCE_TF_SEC_REPEAT])
    {
        int play_mode = divoom_main_set_tf_music_mode(-1);
        divoom_menu_control_lv_img_set_src(phandle->tf_secondary_imgs[DIVOOM_ASTRO_SOURCE_TF_SEC_REPEAT],
            divoom_astrootoo_source_tf_play_mode_icon(play_mode));
    }
    divoom_astrootoo_source_tf_refresh_bar_focus(phandle);
}

#define DIVOOM_ASTRO_TF_VINYL_SPIN_MS   120
#define DIVOOM_ASTRO_TF_VINYL_STEP      200 /* 20°，约 2.2s 一圈 */
#define DIVOOM_ASTRO_TF_VINYL_DISC      108

static void divoom_astrootoo_source_tf_vinyl_apply_angle(Pdivoom_astrootoo_source_ctrl phandle)
{
    if (!phandle || !phandle->tf_vinyl_disc)
    {
        return;
    }
    if (phandle->tf_vinyl_angle >= 3600)
    {
        phandle->tf_vinyl_angle -= 3600;
    }
    lv_obj_set_style_transform_angle(phandle->tf_vinyl_disc, phandle->tf_vinyl_angle, 0);
}

static void divoom_astrootoo_source_tf_vinyl_timer_cb(lv_timer_t *timer)
{
    Pdivoom_astrootoo_source_ctrl phandle;

    phandle = (Pdivoom_astrootoo_source_ctrl)lv_timer_get_user_data(timer);
    if (!phandle || !phandle->tf_vinyl_disc)
    {
        return;
    }
    phandle->tf_vinyl_angle += DIVOOM_ASTRO_TF_VINYL_STEP;
    divoom_astrootoo_source_tf_vinyl_apply_angle(phandle);
}

static void divoom_astrootoo_source_tf_vinyl_set_spin(Pdivoom_astrootoo_source_ctrl phandle, int playing)
{
    if (!phandle || !phandle->tf_vinyl_timer)
    {
        return;
    }
    if (playing)
    {
        lv_timer_resume(phandle->tf_vinyl_timer);
    }
    else
    {
        lv_timer_pause(phandle->tf_vinyl_timer);
    }
}

static lv_obj_t *divoom_astrootoo_source_tf_vinyl_circle(lv_obj_t *parent, lv_coord_t size, uint32_t color)
{
    lv_obj_t *obj;

    obj = lv_obj_create(parent);
    lv_obj_set_size(obj, size, size);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    return obj;
}

static void divoom_astrootoo_source_tf_vinyl_groove(lv_obj_t *parent, lv_coord_t size)
{
    lv_obj_t *obj;

    obj = lv_obj_create(parent);
    lv_obj_set_size(obj, size, size);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x4A4744), 0);
    lv_obj_set_style_border_width(obj, 2, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(obj);
}

static void divoom_astrootoo_source_tf_vinyl_hl(lv_obj_t *parent, lv_coord_t size,
    uint16_t start_deg, uint16_t end_deg)
{
    lv_obj_t *arc;

    arc = lv_arc_create(parent);
    lv_obj_set_size(arc, size, size);
    lv_obj_center(arc);
    lv_arc_set_bg_angles(arc, 0, 0);
    lv_arc_set_angles(arc, start_deg, end_deg);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_opa(arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 3, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x9A9691), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);
}

static void divoom_astrootoo_source_tf_vinyl_build(Pdivoom_astrootoo_source_ctrl phandle, lv_obj_t *meta,
    int draw_rim)
{
    lv_obj_t *slot;
    lv_obj_t *disc;
    lv_obj_t *rim;
    lv_obj_t *arm;
    lv_obj_t *arm_edge;
    lv_obj_t *pivot;
    lv_obj_t *head;
    /* 两端收进转轴和唱头，避免圆角端点露在外面 */
    static const lv_point_t arm_pts[] =
    {
        {114, 18},
        {111, 74},
        {100, 86},
    };

    slot = lv_obj_create(meta);
    lv_obj_set_size(slot, DIVOOM_ASTRO_TF_VINYL_PX, DIVOOM_ASTRO_TF_VINYL_PX);
    lv_obj_set_pos(slot, DIVOOM_ASTRO_TF_VINYL_X, DIVOOM_ASTRO_TF_VINYL_Y);
    lv_obj_set_style_bg_opa(slot, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(slot, 0, 0);
    lv_obj_set_style_pad_all(slot, 0, 0);
    lv_obj_clear_flag(slot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(slot, LV_OBJ_FLAG_CLICKABLE);

    /* 外圈主体是浅米色，右上留缺口给唱臂；圈内到黑胶之间不填色。蓝牙封面不画这圈 */
    if (draw_rim)
    {
        rim = lv_arc_create(slot);
        lv_obj_set_size(rim, 130, 130);
        lv_obj_center(rim);
        lv_arc_set_bg_angles(rim, 0, 0);
        lv_arc_set_angles(rim, 0, 273);
        lv_obj_remove_style(rim, NULL, LV_PART_KNOB);
        lv_obj_clear_flag(rim, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_arc_opa(rim, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_arc_width(rim, 5, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(rim, lv_color_hex(0xE4DFD9), LV_PART_INDICATOR);
        lv_obj_set_style_arc_rounded(rim, true, LV_PART_INDICATOR);
    }

    disc = lv_obj_create(slot);
    lv_obj_set_size(disc, DIVOOM_ASTRO_TF_VINYL_DISC, DIVOOM_ASTRO_TF_VINYL_DISC);
    lv_obj_set_style_bg_opa(disc, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(disc, 0, 0);
    lv_obj_set_style_pad_all(disc, 0, 0);
    lv_obj_clear_flag(disc, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(disc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(disc, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_transform_pivot_x(disc, DIVOOM_ASTRO_TF_VINYL_DISC / 2, 0);
    lv_obj_set_style_transform_pivot_y(disc, DIVOOM_ASTRO_TF_VINYL_DISC / 2, 0);
    phandle->tf_vinyl_disc = disc;

    lv_obj_center(divoom_astrootoo_source_tf_vinyl_circle(disc, DIVOOM_ASTRO_TF_VINYL_DISC, 0x2C2A27));
    /* 三圈空心圆；靠里两圈各有一对对称高光弧 */
    divoom_astrootoo_source_tf_vinyl_groove(disc, 56);
    divoom_astrootoo_source_tf_vinyl_hl(disc, 56, 201, 259);
    divoom_astrootoo_source_tf_vinyl_hl(disc, 56, 21, 79);
    divoom_astrootoo_source_tf_vinyl_groove(disc, 76);
    divoom_astrootoo_source_tf_vinyl_hl(disc, 76, 188, 259);
    divoom_astrootoo_source_tf_vinyl_hl(disc, 76, 0, 79);
    divoom_astrootoo_source_tf_vinyl_groove(disc, 92);

    lv_obj_center(divoom_astrootoo_source_tf_vinyl_circle(disc, 28, 0xF8B634));
    lv_obj_center(divoom_astrootoo_source_tf_vinyl_circle(disc, 6, 0x2C2A27));
    divoom_astrootoo_source_tf_vinyl_apply_angle(phandle);

    /* 唱臂不放进 disc，避免跟着转。深色粗线垫底，形成约 1px 轮廓 */
    arm_edge = lv_line_create(slot);
    lv_line_set_points(arm_edge, arm_pts, 3);
    lv_obj_set_style_bg_opa(arm_edge, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(arm_edge, 0, 0);
    lv_obj_set_style_line_width(arm_edge, 8, 0);
    lv_obj_set_style_line_color(arm_edge, lv_color_hex(0x787070), 0);
    lv_obj_set_style_line_rounded(arm_edge, true, 0);
    lv_obj_clear_flag(arm_edge, LV_OBJ_FLAG_CLICKABLE);

    arm = lv_line_create(slot);
    lv_line_set_points(arm, arm_pts, 3);
    lv_obj_set_style_bg_opa(arm, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(arm, 0, 0);
    lv_obj_set_style_line_width(arm, 5, 0);
    lv_obj_set_style_line_color(arm, lv_color_hex(0xE8E4DE), 0);
    lv_obj_set_style_line_rounded(arm, true, 0);
    lv_obj_clear_flag(arm, LV_OBJ_FLAG_CLICKABLE);

    pivot = divoom_astrootoo_source_tf_vinyl_circle(slot, 14, 0xE8E4DE);
    lv_obj_set_style_border_color(pivot, lv_color_hex(0x787070), 0);
    lv_obj_set_style_border_width(pivot, 1, 0);
    lv_obj_set_pos(pivot, 107, 9);
    head = lv_obj_create(slot);
    lv_obj_set_size(head, 13, 9);
    lv_obj_set_pos(head, 92, 84);
    lv_obj_set_style_radius(head, 3, 0);
    lv_obj_set_style_bg_color(head, lv_color_hex(0xE8E4DE), 0);
    lv_obj_set_style_bg_opa(head, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(head, lv_color_hex(0x787070), 0);
    lv_obj_set_style_border_width(head, 1, 0);
    lv_obj_set_style_pad_all(head, 0, 0);
    lv_obj_clear_flag(head, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(head, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_transform_pivot_x(head, 6, 0);
    lv_obj_set_style_transform_pivot_y(head, 4, 0);
    /* 长边与杆下部 (-11, 12) 垂直 */
    lv_obj_set_style_transform_angle(head, 425, 0);
}

static void divoom_astrootoo_source_build_tf_playback(Pdivoom_astrootoo_source_ctrl phandle)
{
    lv_obj_t *card;
    lv_obj_t *meta;
    lv_obj_t *bar;
    lv_obj_t *btn;
    lv_obj_t *img;
    lv_coord_t card_w;
    lv_coord_t card_x;
    int i;
    static const char *sec_icons[3];
    int play_mode = divoom_main_set_tf_music_mode(-1);

    sec_icons[0] = divoom_astrootoo_source_tf_play_mode_icon(play_mode);
    sec_icons[1] = DIVOOM_ASTROTOO_SOURCE_UI_DIR "source_tf_list.png";
    sec_icons[2] = DIVOOM_ASTROTOO_SOURCE_UI_DIR "source_tf_refresh.png";

    phandle->tf_transport_focus = DIVOOM_ASTRO_SOURCE_TRANSPORT_PLAY;
    phandle->tf_secondary_focus = DIVOOM_ASTRO_SOURCE_TF_SEC_LIST;
    phandle->tf_focus_row = 0;

    lv_obj_clear_flag(phandle->content, LV_OBJ_FLAG_SCROLLABLE);

    /* 布局容器：白面板和两组按钮各自描边，外层不再套大边框 */
    card_w = DIVOOM_ASTRO_BT_CARD_W;
    card_x = (DIVOOM_SCREEN_WIDTH - card_w) / 2;
    card = lv_obj_create(phandle->content);
    lv_obj_set_size(card, card_w, DIVOOM_ASTRO_BT_CARD_H);
    lv_obj_set_pos(card, card_x, DIVOOM_ASTRO_BT_CARD_CONTENT_Y);
    lv_obj_set_style_bg_opa(card, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_radius(card, 0, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    phandle->card_cont = card;

    /* 白面板：左唱片空位，右歌名 / 进度 / 时间 */
    meta = lv_obj_create(card);
    lv_obj_set_style_bg_color(meta, lv_color_hex(DIVOOM_ASTRO_TF_PANEL_BG), 0);
    lv_obj_set_style_bg_opa(meta, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(meta, lv_color_hex(DIVOOM_ASTRO_TF_PANEL_BORDER), 0);
    lv_obj_set_style_border_width(meta, 2, 0);
    lv_obj_set_style_radius(meta, DIVOOM_ASTRO_RADIUS_MD, 0);
    lv_obj_set_style_pad_all(meta, 0, 0);
    lv_obj_clear_flag(meta, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(meta, DIVOOM_ASTRO_TF_PANEL_W, DIVOOM_ASTRO_TF_PANEL_H);
    lv_obj_set_pos(meta, DIVOOM_ASTRO_TF_PANEL_X, DIVOOM_ASTRO_TF_PANEL_Y);
    phandle->tf_meta_cont = meta;

    divoom_astrootoo_source_tf_vinyl_build(phandle, meta, 1);
    if (phandle->tf_vinyl_timer)
    {
        lv_timer_del(phandle->tf_vinyl_timer);
    }
    phandle->tf_vinyl_timer = lv_timer_create(divoom_astrootoo_source_tf_vinyl_timer_cb,
        DIVOOM_ASTRO_TF_VINYL_SPIN_MS, phandle);
    if (phandle->tf_vinyl_timer)
    {
        lv_timer_pause(phandle->tf_vinyl_timer);
    }

    phandle->tf_title_lbl = lv_label_create(meta);
    lv_obj_set_width(phandle->tf_title_lbl, DIVOOM_ASTRO_TF_INFO_W);
    lv_label_set_long_mode(phandle->tf_title_lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_color(phandle->tf_title_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    divoom_astrootoo_source_label_font(phandle->tf_title_lbl, 24, phandle->menu_id);
    lv_obj_set_pos(phandle->tf_title_lbl, DIVOOM_ASTRO_TF_INFO_X, DIVOOM_ASTRO_TF_TITLE_Y);

    bar = lv_bar_create(meta);
    lv_obj_set_size(bar, DIVOOM_ASTRO_TF_INFO_W, DIVOOM_ASTRO_TF_PROGRESS_H);
    lv_obj_set_pos(bar, DIVOOM_ASTRO_TF_INFO_X, DIVOOM_ASTRO_TF_PROGRESS_Y);
    lv_bar_set_range(bar, 0, 100);
    lv_obj_set_style_pad_all(bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(bar, 0, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bar, lv_color_hex(DIVOOM_ASTRO_SURFACE_IDLE), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(bar, lv_color_hex(DIVOOM_ASTRO_STROKE), LV_PART_MAIN);
    lv_obj_set_style_border_width(bar, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 6, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 4, LV_PART_INDICATOR);
    phandle->tf_progress_bar = bar;

    phandle->tf_progress_thumb = lv_obj_create(meta);
    lv_obj_set_size(phandle->tf_progress_thumb, 12, 12);
    lv_obj_set_style_bg_color(phandle->tf_progress_thumb, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
    lv_obj_set_style_bg_opa(phandle->tf_progress_thumb, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(phandle->tf_progress_thumb, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
    lv_obj_set_style_border_width(phandle->tf_progress_thumb, 2, 0);
    lv_obj_set_style_radius(phandle->tf_progress_thumb, 12, 0);
    lv_obj_set_style_pad_all(phandle->tf_progress_thumb, 0, 0);
    lv_obj_clear_flag(phandle->tf_progress_thumb, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(phandle->tf_progress_thumb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(phandle->tf_progress_thumb);

    phandle->tf_time_pos_lbl = lv_label_create(meta);
    lv_obj_set_style_text_color(phandle->tf_time_pos_lbl, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    divoom_astrootoo_source_label_font(phandle->tf_time_pos_lbl, 16, phandle->menu_id);
    lv_label_set_text(phandle->tf_time_pos_lbl, "00:00");
    lv_obj_set_pos(phandle->tf_time_pos_lbl, DIVOOM_ASTRO_TF_INFO_X, DIVOOM_ASTRO_TF_TIME_Y);

    phandle->tf_time_dur_lbl = lv_label_create(meta);
    lv_obj_set_style_text_color(phandle->tf_time_dur_lbl, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    divoom_astrootoo_source_label_font(phandle->tf_time_dur_lbl, 16, phandle->menu_id);
    lv_obj_set_style_text_align(phandle->tf_time_dur_lbl, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_width(phandle->tf_time_dur_lbl, 72);
    lv_label_set_text(phandle->tf_time_dur_lbl, "00:00");
    lv_obj_set_pos(phandle->tf_time_dur_lbl,
        DIVOOM_ASTRO_TF_INFO_X + DIVOOM_ASTRO_TF_INFO_W - 72, DIVOOM_ASTRO_TF_TIME_Y);

    phandle->tf_transport_bar = divoom_astrootoo_source_tf_create_bar(card,
        DIVOOM_ASTRO_TF_TRANSPORT_CARD_Y);
    for (i = 0; i < 3; i++)
    {
        btn = lv_btn_create(phandle->tf_transport_bar);
        divoom_astrootoo_source_tf_style_bar_btn(btn);
        lv_obj_set_height(btn, LV_PCT(100));
        lv_obj_set_flex_grow(btn, 1);
        phandle->tf_transport_btns[i] = btn;
        img = lv_img_create(btn);
        phandle->tf_transport_imgs[i] = img;
        if (i == 0)
        {
            divoom_menu_control_lv_img_set_src(img, DIVOOM_ASTROTOO_SOURCE_UI_DIR "source_transport_prev.png");
        }
        else if (i == 2)
        {
            divoom_menu_control_lv_img_set_src(img, DIVOOM_ASTROTOO_SOURCE_UI_DIR "source_transport_next.png");
        }
        lv_obj_center(img);
        divoom_astrootoo_source_attach_key(phandle, btn);
    }

    phandle->tf_secondary_bar = divoom_astrootoo_source_tf_create_bar(card,
        DIVOOM_ASTRO_TF_SECONDARY_CARD_Y);
    for (i = 0; i < 3; i++)
    {
        btn = lv_btn_create(phandle->tf_secondary_bar);
        divoom_astrootoo_source_tf_style_bar_btn(btn);
        lv_obj_set_height(btn, LV_PCT(100));
        lv_obj_set_flex_grow(btn, 1);
        phandle->tf_secondary_btns[i] = btn;
        img = lv_img_create(btn);
        phandle->tf_secondary_imgs[i] = img;
        divoom_menu_control_lv_img_set_src(img, sec_icons[i]);
        lv_obj_center(img);
        divoom_astrootoo_source_attach_key(phandle, btn);
    }

    divoom_astrootoo_source_create_mode_hint_bar(phandle, "SourceTfModeHint", 1,
        DIVOOM_ASTRO_BT_MODE_BAR_BG);
    divoom_astrootoo_source_tf_refresh_bar_focus(phandle);

    divoom_music_ctrl_poll_tf_play_info();
    divoom_astrootoo_source_refresh_tf_ui(phandle);
}

static void divoom_astrootoo_source_tf_list_enable_scroll(Pdivoom_astrootoo_source_ctrl phandle)
{
    if (!phandle || !phandle->content)
    {
        return;
    }
    lv_obj_add_flag(phandle->content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(phandle->content, LV_SCROLLBAR_MODE_OFF);
    lv_obj_scroll_to_y(phandle->content, 0, LV_ANIM_OFF);
}

static void divoom_astrootoo_source_tf_window_clamp(Pdivoom_astrootoo_source_ctrl phandle)
{
    int total;
    int start;
    int focus;

    total = phandle->tf_track_count;
    start = phandle->tf_window_start;
    focus = phandle->tf_logical_focus;
    if (total <= 0)
    {
        phandle->tf_window_start = 0;
        phandle->tf_logical_focus = 0;
        return;
    }
    if (focus < 0)
    {
        focus = 0;
    }
    if (focus >= total)
    {
        focus = total - 1;
    }
    if (total <= DIVOOM_ASTRO_SOURCE_TF_SLOT_CNT)
    {
        start = 0;
    }
    else
    {
        if (focus < start)
        {
            start = focus;
        }
        if (focus >= start + DIVOOM_ASTRO_SOURCE_TF_SLOT_CNT)
        {
            start = focus - DIVOOM_ASTRO_SOURCE_TF_SLOT_CNT + 1;
        }
        if (start < 0)
        {
            start = 0;
        }
        if (start > total - DIVOOM_ASTRO_SOURCE_TF_SLOT_CNT)
        {
            start = total - DIVOOM_ASTRO_SOURCE_TF_SLOT_CNT;
        }
    }
    phandle->tf_window_start = start;
    phandle->tf_logical_focus = focus;
    phandle->tf_track_focus = focus;
}

static void divoom_astrootoo_source_tf_list_paint(Pdivoom_astrootoo_source_ctrl phandle)
{
    int i;
    int is_dir;
    const char *raw_name;
    char row_label[DIVOOM_ASTRO_SOURCE_TF_BROWSE_NAME_LEN];

    if (!phandle)
    {
        return;
    }
    for (i = 0; i < DIVOOM_ASTRO_SOURCE_TF_SLOT_CNT; i++)
    {
        if (!phandle->tf_slot_row[i])
        {
            continue;
        }
        if (i >= phandle->tf_slot_count)
        {
            lv_obj_add_flag(phandle->tf_slot_row[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        lv_obj_clear_flag(phandle->tf_slot_row[i], LV_OBJ_FLAG_HIDDEN);
        is_dir = phandle->tf_slots[i].is_dir ? 1 : 0;
        raw_name = phandle->tf_slots[i].name;
        if (!raw_name)
        {
            raw_name = "";
        }
        if (phandle->tf_slot_icon[i])
        {
            divoom_menu_control_lv_img_set_src(phandle->tf_slot_icon[i],
                is_dir ? "23/at_tf_folder.png" : "23/at_tf_music_note.png");
        }
        if (phandle->tf_slot_chevron[i])
        {
            if (is_dir)
            {
                lv_obj_clear_flag(phandle->tf_slot_chevron[i], LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_obj_add_flag(phandle->tf_slot_chevron[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        if (phandle->tf_slot_check[i])
        {
            lv_obj_add_flag(phandle->tf_slot_check[i], LV_OBJ_FLAG_HIDDEN);
        }
        divoom_astrootoo_source_tf_format_row_label(row_label, (int)sizeof(row_label), raw_name, is_dir);
        divoom_astrootoo_source_tf_row_label_set(phandle, phandle->tf_slot_label[i], row_label);
    }
}

static lv_obj_t *divoom_astrootoo_source_tf_list_focus_obj(Pdivoom_astrootoo_source_ctrl phandle)
{
    int slot;

    if (!phandle)
    {
        return NULL;
    }
    slot = phandle->tf_logical_focus - phandle->tf_window_start;
    if (slot < 0 || slot >= DIVOOM_ASTRO_SOURCE_TF_SLOT_CNT)
    {
        slot = 0;
    }
    if (phandle->tf_slot_row[slot]
        && !lv_obj_has_flag(phandle->tf_slot_row[slot], LV_OBJ_FLAG_HIDDEN))
    {
        return phandle->tf_slot_row[slot];
    }
    return phandle->tf_slot_row[0];
}

static void divoom_astrootoo_source_tf_list_apply_focus(Pdivoom_astrootoo_source_ctrl phandle,
    lv_anim_enable_t anim)
{
    lv_obj_t *obj;

    obj = divoom_astrootoo_source_tf_list_focus_obj(phandle);
    if (!obj)
    {
        return;
    }
    lv_group_focus_obj(obj);
    lv_obj_scroll_to_view(obj, anim);
}

static int divoom_astrootoo_source_tf_list_nav(Pdivoom_astrootoo_source_ctrl phandle, int go_up,
    lv_anim_enable_t anim)
{
    int total;
    int old_start;
    int old_focus;
    int wrap;
    int delta;
    lv_coord_t sy;

    if (!phandle)
    {
        return 0;
    }
    total = phandle->tf_track_count;
    if (total <= 0)
    {
        return 0;
    }
    old_start = phandle->tf_window_start;
    old_focus = phandle->tf_logical_focus;
    if (go_up)
    {
        phandle->tf_logical_focus = (old_focus + total - 1) % total;
    }
    else
    {
        phandle->tf_logical_focus = (old_focus + 1) % total;
    }
    wrap = (total > 1)
        && ((old_focus == 0 && phandle->tf_logical_focus == total - 1)
            || (old_focus == total - 1 && phandle->tf_logical_focus == 0));
    divoom_astrootoo_source_tf_window_clamp(phandle);
    delta = phandle->tf_window_start - old_start;
    if (delta != 0 || wrap)
    {
        divoom_astrootoo_source_collect_tf_browse_entries(phandle);
        divoom_astrootoo_source_tf_list_paint(phandle);
        if (phandle->content)
        {
            lv_obj_update_layout(phandle->content);
        }
    }
    if (wrap)
    {
        if (phandle->tf_logical_focus == 0 && phandle->content)
        {
            lv_obj_scroll_to_y(phandle->content, 0, LV_ANIM_OFF);
        }
        divoom_astrootoo_source_tf_list_apply_focus(phandle, LV_ANIM_OFF);
        return 1;
    }
    if (delta != 0 && phandle->content)
    {
        sy = lv_obj_get_scroll_y(phandle->content);
        sy = (lv_coord_t)(sy - delta * DIVOOM_ASTRO_SOURCE_TF_ROW_STRIDE);
        if (sy < 0)
        {
            sy = 0;
        }
        lv_obj_scroll_to_y(phandle->content, sy, LV_ANIM_OFF);
    }
    divoom_astrootoo_source_tf_list_apply_focus(phandle, anim);
    return 1;
}

static void divoom_astrootoo_source_tf_play_browse_slot(Pdivoom_astrootoo_source_ctrl phandle, int slot)
{
    int order;
    int music_id;
    uint8 vol;

    if (!phandle || slot < 0 || slot >= phandle->tf_slot_count
        || phandle->tf_slots[slot].is_dir || phandle->tf_slots[slot].id <= 0)
    {
        return;
    }
    music_id = phandle->tf_slots[slot].id;
    order = (phandle->tf_root_play_index >= 0) ? (phandle->tf_root_play_index + 2) : 0;
    vol = divoom_tf_music_get_volume();
    divoom_tf_music_set_playing_music_id(order, music_id);
    divoom_music_ctrl_play_tf_track_id(music_id, vol);
    /* 点歌进播放页：栈收回「列表→播放」，避免返回时反复经过目录 */
    divoom_astrootoo_source_drop_tf_catalog(phandle);
    phandle->page_stack[0] = DIVOOM_ASTRO_SOURCE_PAGE_LIST;
    phandle->page_stack_depth = 1;
    divoom_astrootoo_source_show_page(phandle, DIVOOM_ASTRO_SOURCE_PAGE_TF);
    divoom_music_ctrl_poll_tf_play_info();
    divoom_astrootoo_source_refresh_tf_ui(phandle);
}

static void divoom_astrootoo_source_tf_enter_list_row(Pdivoom_astrootoo_source_ctrl phandle)
{
    int slot;
    int at_hub;

    if (!phandle)
    {
        return;
    }
    slot = phandle->tf_logical_focus - phandle->tf_window_start;
    if (slot < 0 || slot >= phandle->tf_slot_count)
    {
        return;
    }
    if (phandle->tf_slots[slot].is_dir)
    {
        if (phandle->tf_slots[slot].id <= 0)
        {
            return;
        }
        at_hub = (phandle->page == DIVOOM_ASTRO_SOURCE_PAGE_TF_HUB
            || phandle->tf_folder_id == 0);
        if (at_hub)
        {
            phandle->tf_root_play_index =
                divoom_astrootoo_source_tf_root_play_index_of(phandle->tf_slots[slot].id);
        }
        if (phandle->tf_folder_depth < DIVOOM_ASTRO_SOURCE_TF_FOLDER_STACK)
        {
            phandle->tf_folder_stack[phandle->tf_folder_depth++] = phandle->tf_folder_id;
        }
        phandle->tf_folder_id = phandle->tf_slots[slot].id;
        if (phandle->page == DIVOOM_ASTRO_SOURCE_PAGE_TF_HUB)
        {
            divoom_astrootoo_source_push_page(phandle, DIVOOM_ASTRO_SOURCE_PAGE_TF_TRACKS);
            return;
        }
        divoom_astrootoo_source_show_page(phandle, DIVOOM_ASTRO_SOURCE_PAGE_TF_TRACKS);
        return;
    }
    divoom_astrootoo_source_tf_play_browse_slot(phandle, slot);
}

static void divoom_astrootoo_source_build_tf_slot_list(Pdivoom_astrootoo_source_ctrl phandle)
{
    int i;

    divoom_astrootoo_source_tf_list_enable_scroll(phandle);
    phandle->tf_window_start = 0;
    if (phandle->tf_logical_focus < 0)
    {
        phandle->tf_logical_focus = 0;
    }
    divoom_astrootoo_source_tf_window_clamp(phandle);
    divoom_astrootoo_source_collect_tf_browse_entries(phandle);
    memset(phandle->tf_slot_row, 0, sizeof(phandle->tf_slot_row));
    memset(phandle->tf_slot_icon, 0, sizeof(phandle->tf_slot_icon));
    memset(phandle->tf_slot_label, 0, sizeof(phandle->tf_slot_label));
    memset(phandle->tf_slot_chevron, 0, sizeof(phandle->tf_slot_chevron));
    memset(phandle->tf_slot_check, 0, sizeof(phandle->tf_slot_check));
    for (i = 0; i < DIVOOM_ASTRO_SOURCE_TF_SLOT_CNT; i++)
    {
        phandle->tf_slot_row[i] = divoom_astrootoo_ui_create_tf_list_row(phandle->content, &phandle->ui,
            DIVOOM_ASTROTOO_TF_ROW_FOLDER, i, DIVOOM_ASTRO_SOURCE_LIST_ROW_H,
            DIVOOM_ASTRO_SOURCE_LIST_ROW_GAP, phandle->menu_id,
            &phandle->tf_slot_icon[i], &phandle->tf_slot_label[i],
            &phandle->tf_slot_chevron[i], &phandle->tf_slot_check[i]);
        divoom_astrootoo_source_attach_key(phandle, phandle->tf_slot_row[i]);
    }
    divoom_astrootoo_source_tf_list_paint(phandle);
}

static void divoom_astrootoo_source_build_tf_hub(Pdivoom_astrootoo_source_ctrl phandle)
{
    /* Hub：folder 0 的 dir_page，过滤挂载名，同时列出一级文件夹和根下歌曲 */
    if (phandle->tf_resume_focus)
    {
        phandle->tf_resume_focus = 0;
    }
    else
    {
        divoom_astrootoo_source_tf_browse_reset_nav(phandle);
    }
    divoom_astrootoo_source_collect_tf_browse_entries(phandle);
    divoom_astrootoo_source_build_tf_slot_list(phandle);
}

static void divoom_astrootoo_source_build_tf_tracks(Pdivoom_astrootoo_source_ctrl phandle)
{
    if (phandle->tf_resume_focus)
    {
        phandle->tf_resume_focus = 0;
    }
    else
    {
        phandle->tf_window_start = 0;
        phandle->tf_logical_focus = 0;
    }
    divoom_astrootoo_source_collect_tf_browse_entries(phandle);
    divoom_astrootoo_source_build_tf_slot_list(phandle);
}

static void divoom_astrootoo_source_build_picker(Pdivoom_astrootoo_source_ctrl phandle,
    const char **keys, int count, int committed)
{
    int i;

    phandle->picker_keys = keys;
    phandle->picker_count = count;
    phandle->picker_committed = committed;
    phandle->picker_focus = committed;
    phandle->direct_handle = divoom_menu_direct_ctrl_create(1, count);
    for (i = 0; i < count; i++)
    {
        divoom_astrootoo_source_create_picker_row(phandle, phandle->content, keys[i], i, committed, phandle->direct_handle);
    }
}

/* 顶部/收藏图标均复用既有资源，在create/timer载入，按键回调不解码图片。 */
static void divoom_astrootoo_source_refresh_net_radio_ui(Pdivoom_astrootoo_source_ctrl phandle)
{
    if (phandle->page == DIVOOM_ASTRO_SOURCE_PAGE_NET_RADIO)
    {
        divoom_radio_menu_refresh(phandle->net_menu);
    }
}

static void divoom_astrootoo_source_build_net_radio(Pdivoom_astrootoo_source_ctrl phandle)
{
    phandle->net_menu = divoom_radio_menu_create(phandle->content, phandle->header, phandle->menu_id, &phandle->net_memory);
}

/* 把对象加入分组（跳过空指针） */
static void divoom_astrootoo_source_grp_add(lv_group_t *grp, lv_obj_t *obj)
{
    if (grp && obj)
    {
        lv_group_add_obj(grp, obj);
    }
}

static void divoom_astrootoo_source_group_bind(Pdivoom_astrootoo_source_ctrl phandle)
{
    lv_group_t *grp;
    lv_obj_t *focus_obj;
    int i;
    int cnt;

    grp = lv_group_get_default();
    if (!grp)
    {
        return;
    }
    lv_group_remove_all_objs(grp);
    focus_obj = NULL;

    switch (phandle->page)
    {
    case DIVOOM_ASTRO_SOURCE_PAGE_LIST:
    {
        int row;

        for (i = 0; i < DIVOOM_ASTRO_SOURCE_MAIN_CNT; i++)
        {
            divoom_astrootoo_source_grp_add(grp, phandle->list_rows[i]);
        }
        row = divoom_astrootoo_source_list_row_for_mode(divoom_music_ctrl_get_mode());
        focus_obj = phandle->list_rows[row];
        if (!focus_obj)
        {
            focus_obj = phandle->list_rows[0];
        }
        break;
    }
    case DIVOOM_ASTRO_SOURCE_PAGE_BT:
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
    case DIVOOM_ASTRO_SOURCE_PAGE_UAC:
#endif
#if DIVOOM_MUSIC_CTRL_LINEIN_ENABLE
    case DIVOOM_ASTRO_SOURCE_PAGE_AUX:
#endif
    {
        for (i = 0; i < 3; i++)
        {
            divoom_astrootoo_source_grp_add(grp, phandle->transport_btns[i]);
        }
        focus_obj = phandle->transport_btns[phandle->transport_focus];
        break;
    }
    case DIVOOM_ASTRO_SOURCE_PAGE_TF:
    {
        for (i = 0; i < 3; i++)
        {
            divoom_astrootoo_source_grp_add(grp, phandle->tf_transport_btns[i]);
        }
        for (i = 0; i < 3; i++)
        {
            divoom_astrootoo_source_grp_add(grp, phandle->tf_secondary_btns[i]);
        }
        if (phandle->tf_focus_row == 0)
        {
            focus_obj = phandle->tf_transport_btns[phandle->tf_transport_focus];
        }
        else
        {
            focus_obj = phandle->tf_secondary_btns[phandle->tf_secondary_focus];
        }
        divoom_astrootoo_source_tf_refresh_bar_focus(phandle);
        break;
    }
    case DIVOOM_ASTRO_SOURCE_PAGE_TF_HUB:
    case DIVOOM_ASTRO_SOURCE_PAGE_TF_TRACKS:
    {
        for (i = 0; i < DIVOOM_ASTRO_SOURCE_TF_SLOT_CNT; i++)
        {
            if (phandle->tf_slot_row[i]
                && !lv_obj_has_flag(phandle->tf_slot_row[i], LV_OBJ_FLAG_HIDDEN))
            {
                divoom_astrootoo_source_grp_add(grp, phandle->tf_slot_row[i]);
            }
        }
        focus_obj = divoom_astrootoo_source_tf_list_focus_obj(phandle);
        break;
    }
    case DIVOOM_ASTRO_SOURCE_PAGE_TF_SCOPE:
    case DIVOOM_ASTRO_SOURCE_PAGE_TF_ORDER:
    case DIVOOM_ASTRO_SOURCE_PAGE_TF_EQ:
    case DIVOOM_ASTRO_SOURCE_PAGE_TF_FAVORITES:
    {
        cnt = phandle->content ? (int)lv_obj_get_child_cnt(phandle->content) : 0;
        for (i = 0; i < cnt; i++)
        {
            divoom_astrootoo_source_grp_add(grp, lv_obj_get_child(phandle->content, i));
        }
        if (phandle->direct_handle)
        {
            focus_obj = divoom_menu_direct_ctrl_get(phandle->direct_handle, NULL, DIVOOM_MENU_DIRECT_DOWN);
        }
        if (!focus_obj && phandle->content && cnt > 0)
        {
            focus_obj = lv_obj_get_child(phandle->content, 0);
        }
        break;
    }
    case DIVOOM_ASTRO_SOURCE_PAGE_NET_RADIO:
        /* 漫游/分区导航由 key_btn + 状态机处理，勿把 Tab/列表加入 group 以免抢走按键焦点 */
        focus_obj = phandle->key_btn;
        break;
    default:
    {
        break;
    }
    }

    /* key_btn 作为兜底，确保任何页面都有对象接收 LV_EVENT_KEY */
    if (phandle->key_btn)
    {
        lv_group_add_obj(grp, phandle->key_btn);
    }
    if (!focus_obj)
    {
        focus_obj = phandle->key_btn;
    }
    if (focus_obj)
    {
        lv_group_focus_obj(focus_obj);
        divoom_astrootoo_ui_focus_scroll(focus_obj);
    }
}

static void divoom_astrootoo_source_enter_main_row(Pdivoom_astrootoo_source_ctrl phandle, int row)
{
    uint8 mode;
    divoom_astrootoo_source_page_t page;

    if (row < 0 || row >= DIVOOM_ASTRO_SOURCE_MAIN_CNT)
    {
        return;
    }
    page = g_source_main_rows[row].page;
    mode = g_source_main_rows[row].music_mode;
    if (page == DIVOOM_ASTRO_SOURCE_PAGE_NET_RADIO)
    {
        /* 仅浏览目录不抢占其他音源；选台后由播放器worker进入统一NET_FM音源。 */
        divoom_astrootoo_source_push_page(phandle, page);
        return;
    }
    if (mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
    {
#if !DIVOOM_SHOW_TF_NAND
        if (!divoom_main_get_audio_type_statu(2))
        {
            divoom_lvgl_top_source_switch_menu_init(DIVOOM_MUSIC_CTRL_MODE_LOCAL,
                DIVOOM_SOURCE_SWITCH_OSD_PLS_INSERT);
            return;
        }
#endif
        /* 切进 TF 只 set_mode，等 JL 0x5e 再播；已在 TF 才 start 续播 */
        if (divoom_music_ctrl_get_mode() != DIVOOM_MUSIC_CTRL_MODE_LOCAL)
            divoom_music_ctrl_set_mode(DIVOOM_MUSIC_CTRL_MODE_LOCAL);
        else
            divoom_tf_music_start();
    }
    else if (mode == DIVOOM_MUSIC_CTRL_MODE_LINEIN)
    {
#if DIVOOM_MUSIC_CTRL_LINEIN_ENABLE
        if (!divoom_main_get_audio_type_statu(1))
        {
            /* 未插入 AUX */
            return;
        }
        divoom_music_ctrl_set_mode(DIVOOM_MUSIC_CTRL_MODE_LINEIN);
#else
        return;
#endif
    }
    else if (mode == DIVOOM_MUSIC_CTRL_MODE_UAC)
    {
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
        if (!divoom_main_get_audio_type_statu(0))
        {
            divoom_lvgl_top_source_switch_menu_init(DIVOOM_MUSIC_CTRL_MODE_UAC,
                DIVOOM_SOURCE_SWITCH_OSD_PLS_INSERT);
            return;
        }
        divoom_music_ctrl_set_mode(DIVOOM_MUSIC_CTRL_MODE_UAC);
#else
        return;
#endif
    }
    else
    {
        divoom_music_ctrl_set_mode(DIVOOM_MUSIC_CTRL_MODE_BT);
    }
    divoom_astrootoo_source_push_page(phandle, page);
}

static void divoom_astrootoo_source_rebuild_tf_catalog(Pdivoom_astrootoo_source_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }
    if (phandle->page != DIVOOM_ASTRO_SOURCE_PAGE_TF_HUB
        && phandle->page != DIVOOM_ASTRO_SOURCE_PAGE_TF_TRACKS)
    {
        return;
    }
    divoom_astrootoo_source_tf_browse_reset_nav(phandle);
    if (phandle->page == DIVOOM_ASTRO_SOURCE_PAGE_TF_TRACKS)
    {
        divoom_astrootoo_source_pop_page(phandle);
        return;
    }
    if (phandle->content)
    {
        lv_obj_clean(phandle->content);
    }
    memset(phandle->tf_slot_row, 0, sizeof(phandle->tf_slot_row));
    memset(phandle->tf_slot_icon, 0, sizeof(phandle->tf_slot_icon));
    memset(phandle->tf_slot_label, 0, sizeof(phandle->tf_slot_label));
    memset(phandle->tf_slot_chevron, 0, sizeof(phandle->tf_slot_chevron));
    memset(phandle->tf_slot_check, 0, sizeof(phandle->tf_slot_check));
    divoom_astrootoo_source_build_tf_hub(phandle);
    divoom_astrootoo_source_group_bind(phandle);
}

static void divoom_astrootoo_source_tf_catalog_poll(Pdivoom_astrootoo_source_ctrl phandle)
{
#if defined(DIVOOM_TIMES_VIBE)
    if (!phandle)
    {
        return;
    }
    divoom_tf_music_poll_songlist_refresh();
    if (divoom_tf_music_take_ui_hotplug_reload())
    {
        divoom_astrootoo_source_rebuild_tf_catalog(phandle);
    }
    else
    {
        uint32 gen;

        gen = 0;
        if (divoom_database_tf_meta_get_u32("generation", &gen)
            && phandle->tf_generation >= 0
            && (int)gen != phandle->tf_generation)
        {
            divoom_astrootoo_source_rebuild_tf_catalog(phandle);
        }
    }
#else
    (void)phandle;
#endif
}

static void divoom_astrootoo_source_timer_cb(lv_timer_t *t)
{
    Pdivoom_astrootoo_source_ctrl phandle;

    phandle = (Pdivoom_astrootoo_source_ctrl)lv_timer_get_user_data(t);
    if (!phandle)
    {
        return;
    }
    if (phandle->page == DIVOOM_ASTRO_SOURCE_PAGE_NET_RADIO)
    {
        divoom_astrootoo_source_refresh_net_radio_ui(phandle);
        return;
    }
    if (phandle->bt_line1_dirty && phandle->card_line1_lbl)
    {
        phandle->bt_line1_dirty = 0;
        divoom_astrootoo_source_card_set_scroll_line(phandle->card_line1_lbl, phandle->bt_line1);
    }
    if (phandle->bt_line2_dirty && phandle->card_line2_lbl)
    {
        phandle->bt_line2_dirty = 0;
        divoom_astrootoo_source_card_set_scroll_line(phandle->card_line2_lbl, phandle->bt_line2);
    }
    if (divoom_astrootoo_source_is_card_playback_page(phandle->page))
    {
        if (phandle->playback_ui_dirty)
        {
            phandle->playback_ui_dirty = 0;
        }
        divoom_astrootoo_source_refresh_playback_ui(phandle);
    }
    else if (phandle->page == DIVOOM_ASTRO_SOURCE_PAGE_TF)
    {
        divoom_music_ctrl_poll_tf_play_info();
        divoom_astrootoo_source_refresh_tf_ui(phandle);
        divoom_astrootoo_source_tf_catalog_poll(phandle);
    }
    else if (phandle->page == DIVOOM_ASTRO_SOURCE_PAGE_TF_HUB
        || phandle->page == DIVOOM_ASTRO_SOURCE_PAGE_TF_TRACKS)
    {
        divoom_astrootoo_source_tf_catalog_poll(phandle);
    }
}

/* 表盘式二维导航：左右只移动列；网络、磁盘和图标刷新仍留在worker/timer。 */
static void divoom_astrootoo_source_net_handle_key(Pdivoom_astrootoo_source_ctrl phandle, uint8 key, uint8 press)
{
    if (divoom_radio_menu_key_event(phandle->net_menu, key, press))
    {
        divoom_astrootoo_source_pop_page(phandle);
    }
}

static void divoom_astrootoo_source_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_source_ctrl phandle;
    uint8_t key;
    lv_obj_t *focused;
    lv_obj_t *next;
    int direct;
    int row;

    phandle = (Pdivoom_astrootoo_source_ctrl)lv_event_get_user_data(e);
    if (!phandle || lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (phandle->page == DIVOOM_ASTRO_SOURCE_PAGE_NET_RADIO)
    {
        /* 电台页必须收到松手事件，才能合并一次物理按压中的连按和短按。 */
        divoom_astrootoo_source_net_handle_key(phandle, divoom_astrootoo_ui_key_code_from_event(e),
            divoom_astrootoo_ui_key_press_from_event(e));
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    key = divoom_astrootoo_ui_key_code_from_event(e);

    if (divoom_astrootoo_ui_key_is_esc(key))
    {
        if (phandle->page == DIVOOM_ASTRO_SOURCE_PAGE_TF_TRACKS)
        {
            divoom_astrootoo_source_tf_browse_go_up(phandle);
            return;
        }
        divoom_astrootoo_source_pop_page(phandle);
        return;
    }

    if (phandle->page == DIVOOM_ASTRO_SOURCE_PAGE_LIST)
    {
        focused = lv_group_get_focused(lv_group_get_default());
        if (divoom_astrootoo_ui_key_is_up(key) || divoom_astrootoo_ui_key_is_down(key))
        {
            direct = divoom_astrootoo_ui_key_is_up(key) ? DIVOOM_MENU_DIRECT_UP : DIVOOM_MENU_DIRECT_DOWN;
            next = divoom_menu_direct_ctrl_get(phandle->direct_handle, focused, direct);
            if (next && next != focused)
            {
                lv_group_focus_obj(next);
                divoom_astrootoo_ui_focus_scroll(next);
            }
            return;
        }
        if (divoom_astrootoo_ui_key_is_enter(key))
        {
            row = divoom_menu_direct_ctrl_get_pos(phandle->direct_handle, focused);
            divoom_astrootoo_source_enter_main_row(phandle, row);
        }
        return;
    }

    if (divoom_astrootoo_source_is_card_playback_page(phandle->page))
    {
        if (divoom_astrootoo_ui_key_is_left(key))
        {
            if (phandle->transport_focus > DIVOOM_ASTRO_SOURCE_TRANSPORT_PREV)
            {
                phandle->transport_focus--;
                lv_group_focus_obj(phandle->transport_btns[phandle->transport_focus]);
            }
            return;
        }
        if (divoom_astrootoo_ui_key_is_right(key))
        {
            if (phandle->transport_focus < DIVOOM_ASTRO_SOURCE_TRANSPORT_NEXT)
            {
                phandle->transport_focus++;
                lv_group_focus_obj(phandle->transport_btns[phandle->transport_focus]);
            }
            return;
        }
        if (divoom_astrootoo_ui_key_is_enter(key))
        {
            if (phandle->transport_focus == DIVOOM_ASTRO_SOURCE_TRANSPORT_PREV)
            {
                divoom_music_ctrl_play_pre();
            }
            else if (phandle->transport_focus == DIVOOM_ASTRO_SOURCE_TRANSPORT_NEXT)
            {
                divoom_music_ctrl_play_next();
            }
            else
            {
                divoom_music_ctrl_toggle_play();
            }
            divoom_astrootoo_source_refresh_playback_ui(phandle);
        }
        return;
    }

    if (phandle->page == DIVOOM_ASTRO_SOURCE_PAGE_TF)
    {
        if (divoom_astrootoo_ui_key_is_up(key) || divoom_astrootoo_ui_key_is_down(key))
        {
            int col;
            int to_secondary;

            /* 两行三列：上=播放键行，下=循环/列表/刷新；换行保持列
             * prev↔repeat、play↔list、next↔refresh。已在目标行则不动。 */
            to_secondary = divoom_astrootoo_ui_key_is_down(key) ? 1 : 0;
            if (to_secondary == (int)phandle->tf_focus_row)
            {
                return;
            }
            if (to_secondary)
            {
                col = (int)phandle->tf_transport_focus;
                if (col < 0)
                {
                    col = 0;
                }
                if (col > 2)
                {
                    col = 2;
                }
                phandle->tf_focus_row = 1;
                phandle->tf_secondary_focus = (divoom_astrootoo_source_tf_secondary_t)col;
                divoom_astrootoo_source_tf_focus_btn(phandle);
            }
            else
            {
                col = (int)phandle->tf_secondary_focus;
                if (col < 0)
                {
                    col = 0;
                }
                if (col > 2)
                {
                    col = 2;
                }
                phandle->tf_focus_row = 0;
                phandle->tf_transport_focus = (divoom_astrootoo_source_transport_t)col;
                divoom_astrootoo_source_tf_focus_btn(phandle);
            }
            return;
        }
        if (divoom_astrootoo_ui_key_is_left(key) || divoom_astrootoo_ui_key_is_right(key))
        {
            if (phandle->tf_focus_row == 0)
            {
                if (divoom_astrootoo_ui_key_is_left(key) && phandle->tf_transport_focus > 0)
                {
                    phandle->tf_transport_focus--;
                }
                if (divoom_astrootoo_ui_key_is_right(key) && phandle->tf_transport_focus < 2)
                {
                    phandle->tf_transport_focus++;
                }
            }
            else
            {
                if (divoom_astrootoo_ui_key_is_left(key) && phandle->tf_secondary_focus > 0)
                {
                    phandle->tf_secondary_focus--;
                }
                if (divoom_astrootoo_ui_key_is_right(key) && phandle->tf_secondary_focus < 2)
                {
                    phandle->tf_secondary_focus++;
                }
            }
            divoom_astrootoo_source_tf_focus_btn(phandle);
            return;
        }
        if (divoom_astrootoo_ui_key_is_enter(key))
        {
            if (phandle->tf_focus_row == 0)
            {
                if (phandle->tf_transport_focus == DIVOOM_ASTRO_SOURCE_TRANSPORT_PREV)
                {
                    divoom_music_ctrl_play_pre();
                }
                else if (phandle->tf_transport_focus == DIVOOM_ASTRO_SOURCE_TRANSPORT_NEXT)
                {
                    divoom_music_ctrl_play_next();
                }
                else
                {
                    divoom_music_ctrl_toggle_play();
                }
            }
            else if (phandle->tf_secondary_focus == DIVOOM_ASTRO_SOURCE_TF_SEC_LIST)
            {
                divoom_astrootoo_source_push_page(phandle, DIVOOM_ASTRO_SOURCE_PAGE_TF_HUB);
            }
            else if (phandle->tf_secondary_focus == DIVOOM_ASTRO_SOURCE_TF_SEC_REPEAT)
            {
                int mode;

                mode = divoom_astrootoo_source_cycle_tf_play_mode();
                divoom_astrootoo_source_tf_toast_show(
                    divoom_astrootoo_source_tf_play_mode_label(mode),
                    DIVOOM_ASTRO_TF_TOAST_MS);
            }
            else if (phandle->tf_secondary_focus == DIVOOM_ASTRO_SOURCE_TF_SEC_REFRESH)
            {
                divoom_tf_music_send_scan_music_list();
            }
            /* push_page 会 clear_content，勿在已进入 hub/tracks 等子页后刷新 TF 播放控件 */
            if (phandle->page == DIVOOM_ASTRO_SOURCE_PAGE_TF)
            {
                divoom_music_ctrl_poll_tf_play_info();
                divoom_astrootoo_source_refresh_tf_ui(phandle);
            }
        }
        return;
    }

    if (phandle->page == DIVOOM_ASTRO_SOURCE_PAGE_TF_HUB
        || phandle->page == DIVOOM_ASTRO_SOURCE_PAGE_TF_TRACKS)
    {
        if (divoom_astrootoo_ui_key_is_up(key) || divoom_astrootoo_ui_key_is_down(key))
        {
            uint8_t press;
            lv_anim_enable_t anim;

            press = divoom_astrootoo_ui_key_press_from_event(e);
            anim = (press == DIVOOM_PRESSING || press == DIVOOM_PRESSING_LONG
                || press == DIVOOM_PRESS_LONG) ? LV_ANIM_OFF : LV_ANIM_ON;
            divoom_astrootoo_source_tf_list_nav(phandle, divoom_astrootoo_ui_key_is_up(key) ? 1 : 0, anim);
            return;
        }
        if (divoom_astrootoo_ui_key_is_enter(key))
        {
            divoom_astrootoo_source_tf_enter_list_row(phandle);
        }
        return;
    }

    if (phandle->page == DIVOOM_ASTRO_SOURCE_PAGE_TF_SCOPE
        || phandle->page == DIVOOM_ASTRO_SOURCE_PAGE_TF_ORDER
        || phandle->page == DIVOOM_ASTRO_SOURCE_PAGE_TF_EQ
        || phandle->page == DIVOOM_ASTRO_SOURCE_PAGE_TF_FAVORITES)
    {
        focused = lv_group_get_focused(lv_group_get_default());
        if (divoom_astrootoo_ui_key_is_up(key) || divoom_astrootoo_ui_key_is_down(key))
        {
            direct = divoom_astrootoo_ui_key_is_up(key) ? DIVOOM_MENU_DIRECT_UP : DIVOOM_MENU_DIRECT_DOWN;
            next = divoom_menu_direct_ctrl_get(phandle->direct_handle, focused, direct);
            if (next && next != focused)
            {
                lv_group_focus_obj(next);
                divoom_astrootoo_ui_focus_scroll(next);
            }
            return;
        }
        if (divoom_astrootoo_ui_key_is_enter(key))
        {
            row = divoom_menu_direct_ctrl_get_pos(phandle->direct_handle, focused);
            phandle->picker_committed = row;
            if (phandle->page == DIVOOM_ASTRO_SOURCE_PAGE_TF_ORDER)
            {
                divoom_main_set_tf_music_mode(row);
            }
            divoom_astrootoo_source_pop_page(phandle);
        }
        return;
    }


}

static void divoom_astrootoo_source_show_page(Pdivoom_astrootoo_source_ctrl phandle, divoom_astrootoo_source_page_t page)
{
    const char *title;

    divoom_astrootoo_source_clear_content(phandle, page);
    phandle->page = page;

    title = divoom_astrootoo_source_page_title_key(page);
    if (phandle->header)
    {
        lv_obj_t *lab;

        lab = lv_obj_get_child(phandle->header, -1);
        if (lab)
        {
            lv_label_set_text(lab, divoom_menu_control_get_disp_str(title));
        }
    }

    switch (page)
    {
    case DIVOOM_ASTRO_SOURCE_PAGE_LIST:
    {
        divoom_astrootoo_source_build_list(phandle);
        break;
    }
    case DIVOOM_ASTRO_SOURCE_PAGE_BT:
    {
        divoom_bt_bip_set_store(1);
        divoom_astrootoo_source_build_card_playback(phandle, DIVOOM_MUSIC_CTRL_MODE_BT);
        break;
    }
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
    case DIVOOM_ASTRO_SOURCE_PAGE_UAC:
    {
        divoom_astrootoo_source_build_card_playback(phandle, DIVOOM_MUSIC_CTRL_MODE_UAC);
        break;
    }
#endif
#if DIVOOM_MUSIC_CTRL_LINEIN_ENABLE
    case DIVOOM_ASTRO_SOURCE_PAGE_AUX:
    {
        divoom_astrootoo_source_build_card_playback(phandle, DIVOOM_MUSIC_CTRL_MODE_LINEIN);
        break;
    }
#endif
    case DIVOOM_ASTRO_SOURCE_PAGE_TF:
    {
        divoom_astrootoo_source_build_tf_playback(phandle);
        break;
    }
    case DIVOOM_ASTRO_SOURCE_PAGE_NET_RADIO:
    {
        divoom_astrootoo_source_build_net_radio(phandle);
        break;
    }
    case DIVOOM_ASTRO_SOURCE_PAGE_TF_HUB:
    {
        divoom_astrootoo_source_build_tf_hub(phandle);
        break;
    }
    case DIVOOM_ASTRO_SOURCE_PAGE_TF_TRACKS:
    {
        lv_obj_t *lab;

        divoom_astrootoo_source_build_tf_tracks(phandle);
        if (phandle->header)
        {
            lab = lv_obj_get_child(phandle->header, -1);
            if (lab)
            {
                lv_label_set_text(lab, divoom_astrootoo_source_tf_browse_title(phandle));
            }
        }
        break;
    }
    case DIVOOM_ASTRO_SOURCE_PAGE_TF_SCOPE:
    {
        divoom_astrootoo_source_build_picker(phandle, g_scope_keys, 3, 0);
        break;
    }
    case DIVOOM_ASTRO_SOURCE_PAGE_TF_ORDER:
    {
        divoom_astrootoo_source_build_picker(phandle, g_order_keys, 3,
            divoom_main_set_tf_music_mode(-1));
        break;
    }
    case DIVOOM_ASTRO_SOURCE_PAGE_TF_EQ:
    {
        divoom_astrootoo_source_build_picker(phandle, g_eq_keys, 5, 0);
        break;
    }
    case DIVOOM_ASTRO_SOURCE_PAGE_TF_FAVORITES:
    {
        divoom_astrootoo_source_build_picker(phandle, g_net_station_names, 4, 0);
        break;
    }
    default:
    {
        break;
    }
    }

    divoom_astrootoo_source_group_bind(phandle);
    divoom_astrootoo_source_start_refresh_timer(phandle, page);
}

#if defined(DIVOOM_SIMULATOR) || defined(_WIN32)
static void divoom_astrootoo_source_sim_boot_subpage(Pdivoom_astrootoo_source_ctrl phandle)
{
    const char *sub;

    sub = getenv("DIVOOM_SIM_ASTROTOO_SOURCE_SUBPAGE");
    if (!sub || !sub[0])
    {
        return;
    }
    if (strcmp(sub, "bluetooth") == 0)
    {
        divoom_astrootoo_source_push_page(phandle, DIVOOM_ASTRO_SOURCE_PAGE_BT);
    }
    else if (strcmp(sub, "tf") == 0)
    {
        const char *play_env;

        if (divoom_music_ctrl_get_mode() != DIVOOM_MUSIC_CTRL_MODE_LOCAL)
            divoom_music_ctrl_set_mode(DIVOOM_MUSIC_CTRL_MODE_LOCAL);
        else
            divoom_tf_music_start();
        divoom_astrootoo_source_push_page(phandle, DIVOOM_ASTRO_SOURCE_PAGE_TF);
        play_env = getenv("DIVOOM_SIM_TF_PLAY_ID");
        if (play_env != NULL && play_env[0] != '\0')
        {
            int play_id = atoi(play_env);
            if (play_id > 0)
            {
                divoom_music_ctrl_play_tf_track_id(play_id, divoom_tf_music_get_volume());
                divoom_music_ctrl_poll_tf_play_info();
                divoom_astrootoo_source_refresh_tf_ui(phandle);
            }
        }
    }
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
    else if (strcmp(sub, "uac") == 0)
    {
        divoom_astrootoo_source_push_page(phandle, DIVOOM_ASTRO_SOURCE_PAGE_UAC);
    }
#endif
#if DIVOOM_MUSIC_CTRL_LINEIN_ENABLE
    else if (strcmp(sub, "aux") == 0)
    {
        divoom_astrootoo_source_push_page(phandle, DIVOOM_ASTRO_SOURCE_PAGE_AUX);
    }
#endif
    else if (strcmp(sub, "netRadio") == 0)
    {
        divoom_astrootoo_source_push_page(phandle, DIVOOM_ASTRO_SOURCE_PAGE_NET_RADIO);
    }
    else if (strcmp(sub, "tf_hub") == 0)
    {
        divoom_astrootoo_source_push_page(phandle, DIVOOM_ASTRO_SOURCE_PAGE_TF_HUB);
    }
    else if (strcmp(sub, "tf_eq") == 0)
    {
        divoom_astrootoo_source_push_page(phandle, DIVOOM_ASTRO_SOURCE_PAGE_TF_EQ);
    }
}
#endif

void *divoom_astrootoo_source_init(void *user_data, int menu_id)
{
    Pdivoom_astrootoo_source_ctrl phandle;

    (void)user_data;
    phandle = divoom_system_malloc(sizeof(divoom_astrootoo_source_ctrl));
    if (!phandle)
    {
        return NULL;
    }
    memset(phandle, 0, sizeof(*phandle));
    g_astrootoo_source_handle = phandle;
#if defined(DIVOOM_TIMES_VIBE)
    divoom_music_ctrl_set_play_status_notify(divoom_astrootoo_source_on_play_status_changed);
#endif
    phandle->menu_id = menu_id;
    phandle->page = DIVOOM_ASTRO_SOURCE_PAGE_LIST;
    phandle->page_stack_depth = 0;
    phandle->refresh_timer = NULL;
    phandle->tf_generation = -1;
    phandle->tf_root_play_index = -1;
    phandle->tf_resume_focus = 0;

    phandle->root_obj = divoom_astrootoo_ui_create_root(&phandle->ui);
    phandle->header = divoom_astrootoo_ui_create_page_header_tab(phandle->root_obj, &phandle->ui,
        "AudioSource", DIVOOM_ASTRO_SOURCE_HEADER, phandle->menu_id);
    phandle->content = divoom_astrootoo_ui_create_scroll_content(phandle->root_obj, &phandle->ui);
    phandle->key_btn = divoom_astrootoo_ui_create_key_capture_btn(phandle->root_obj, &phandle->ui,
        divoom_astrootoo_source_key_cb, phandle);
    if (phandle->key_btn)
    {
        lv_obj_move_foreground(phandle->key_btn);
    }

    divoom_astrootoo_source_show_page(phandle, DIVOOM_ASTRO_SOURCE_PAGE_LIST);
#if defined(DIVOOM_SIMULATOR) || defined(_WIN32)
    divoom_astrootoo_source_sim_boot_subpage(phandle);
#endif

    return phandle;
}

void divoom_astrootoo_source_disp(void *handle, void *active_obj)
{
    Pdivoom_astrootoo_source_ctrl phandle;

    (void)active_obj;
    phandle = (Pdivoom_astrootoo_source_ctrl)handle;
    if (!phandle)
    {
        return;
    }
    if (phandle->page == DIVOOM_ASTRO_SOURCE_PAGE_BT)
    {
        divoom_bt_bip_set_store(1);
    }
    /* 从其他菜单返回时恢复目录更新；后台播放始终独立于页面。 */
    divoom_radio_set_menu_active(phandle->page == DIVOOM_ASTRO_SOURCE_PAGE_NET_RADIO);
    lv_scr_load(phandle->root_obj);
    if (phandle->key_btn)
    {
        lv_obj_move_foreground(phandle->key_btn);
    }
    divoom_astrootoo_source_group_bind(phandle);
    divoom_astrootoo_source_start_refresh_timer(phandle, phandle->page);
}

void divoom_astrootoo_source_clean(void *handle)
{
    Pdivoom_astrootoo_source_ctrl phandle;

    phandle = (Pdivoom_astrootoo_source_ctrl)handle;
    if (!phandle)
    {
        return;
    }
    divoom_astrootoo_source_clear_content(phandle, DIVOOM_ASTRO_SOURCE_PAGE_LIST);
    divoom_astrootoo_source_tf_browse_reset_nav(phandle);
    if (phandle->key_btn)
    {
        lv_obj_del(phandle->key_btn);
        phandle->key_btn = NULL;
    }
    if (phandle->content)
    {
        lv_obj_del(phandle->content);
        phandle->content = NULL;
    }
    if (phandle->header)
    {
        lv_obj_del(phandle->header);
        phandle->header = NULL;
    }
    if (phandle->root_obj)
    {
        lv_obj_del(phandle->root_obj);
        phandle->root_obj = NULL;
    }
    divoom_astrootoo_ui_base_reset_styles(&phandle->ui);
#if defined(DIVOOM_TIMES_VIBE)
    if (g_astrootoo_source_handle == phandle)
    {
        divoom_music_ctrl_set_play_status_notify(NULL);
    }
#endif
    divoom_system_free(phandle);
    if (g_astrootoo_source_handle == phandle)
    {
        g_astrootoo_source_handle = NULL;
    }
}

void divoom_astrootoo_source_pause(void *handle)
{
    Pdivoom_astrootoo_source_ctrl phandle;

    phandle = (Pdivoom_astrootoo_source_ctrl)handle;
    if (!phandle)
    {
        return;
    }
    divoom_radio_set_menu_active(0);
    divoom_bt_music_lrc_set_callback(NULL, NULL);
    divoom_bt_bip_set_store(0);
}

int divoom_astrootoo_source_net_radio_is_open(void)
{
    int32 open;
    divoom_get_lvgl_lock();
    open = divoom_menu_control_get_active_menu_id() == DIVOOM_MENU_CONTROL_ID_AUDIO_SOURCE_MENU &&
        g_astrootoo_source_handle && g_astrootoo_source_handle->page == DIVOOM_ASTRO_SOURCE_PAGE_NET_RADIO;
    divoom_free_lvgl_lock();
    return open;
}

static void divoom_astrootoo_source_goto_music_mode_async(void *user_data)
{
    Pdivoom_astrootoo_source_ctrl phandle;
    divoom_astrootoo_source_page_t page;

    (void)user_data;
    if (!divoom_astrootoo_source_page_for_mode(divoom_music_ctrl_get_mode(), &page))
    {
        return;
    }
    if (divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_ID_AUDIO_SOURCE_MENU)
    {
        return;
    }
    phandle = g_astrootoo_source_handle;
    if (!phandle || !phandle->root_obj || !phandle->content)
    {
        return;
    }
    if (phandle->page == page)
    {
        return;
    }

    /* 离开 TF 目录时丢掉浏览栈，避免残留 folder_id */
    divoom_astrootoo_source_drop_tf_catalog(phandle);
    phandle->page_stack[0] = DIVOOM_ASTRO_SOURCE_PAGE_LIST;
    phandle->page_stack_depth = 1;
    divoom_astrootoo_source_show_page(phandle, page);
}

void divoom_astrootoo_source_goto_music_mode(uint8 mode)
{
    divoom_astrootoo_source_page_t page;

    if (!divoom_astrootoo_source_page_for_mode(mode, &page))
    {
        return;
    }
    if (divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_ID_AUDIO_SOURCE_MENU)
    {
        return;
    }

    /* MAIN 线程只投递；换页在 LVGL 任务里做。持递归锁再挂 async，避免抢链表。 */
    divoom_get_lvgl_lock();
    /* LV_RES_OK 在兼容头中按仿真宏映射，真机对应 LV_RESULT_OK。 */
    if (lv_async_call(divoom_astrootoo_source_goto_music_mode_async, NULL) != LV_RES_OK)
    {
        divoom_print("source: lv_async_call goto mode failed\n");
    }
    divoom_free_lvgl_lock();
}

int divoom_astrootoo_source_whitebox_get_page(void)
{
    Pdivoom_astrootoo_source_ctrl phandle;

    if (divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_ID_AUDIO_SOURCE_MENU)
    {
        return -1;
    }
    phandle = g_astrootoo_source_handle;
    if (!phandle)
    {
        return -1;
    }
    return (int)phandle->page;
}

int divoom_astrootoo_source_whitebox_radio_probe(const char *name, long *out)
{
    Pdivoom_astrootoo_source_ctrl phandle = g_astrootoo_source_handle;
    if (!phandle || phandle->page != DIVOOM_ASTRO_SOURCE_PAGE_NET_RADIO)
    {
        return 0;
    }
    return divoom_radio_menu_probe(phandle->net_menu, name, out);
}

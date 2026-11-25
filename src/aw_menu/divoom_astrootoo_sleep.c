#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_menu_direct_ctrl.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_astrootoo_theme.h"
#include "divoom_astrootoo_sleep.h"
#include "divoom_sleep_aid_config.h"
#include "divoom_sleep_aid_ctrl.h"
#include "divoom_manage_image.h"
#include "divoom_mic_record.h"
#include "divoom_local_music_ctrl.h"

#include <time.h>

/*
 * AstroToo 助眠页（480x480，对照 AstroTooUI SleepSplitMenu / split-v1）。
 * 顶部 7 Tab：收藏 / 噪音 / 自然 / 乐曲 / DIY / 录音 / 设置。
 * 音源全部内置本地，列表行尾为收藏/取消收藏（face_market_favorite 图标）。
 * 收藏 Tab：divoom_sleep_aid_fav_get_list；分类 Tab：全量列表 + 收藏切换。
 */

#define SLEEP_UI_DIR              "106/"
#define SLEEP_FAV_ICON            "102/face_market_favorite.png"
#define SLEEP_FAV_ICON_ON         "102/face_market_favorited.png"
#define SLEEP_TAB_COUNT           7
#define SLEEP_PAGE_BATCH          12

/* Tab 枚举 */
enum
{
    SLEEP_TAB_LOCAL = 0,
    SLEEP_TAB_NOISE,
    SLEEP_TAB_NATURE,
    SLEEP_TAB_MUSIC,
    SLEEP_TAB_DIY,
    SLEEP_TAB_REC,
    SLEEP_TAB_SETUP,
};

/* 焦点区域 */
enum
{
    SLEEP_PANE_STRIP = 0,
    SLEEP_PANE_LIST,
    SLEEP_PANE_SUB,      /* DIY / 录音 / 设置 子页 */
    SLEEP_PANE_PICK,     /* 设置 · 定时/音源 二级选择页 */
};

/* 子页可聚焦行类型 */
enum
{
    SUB_KIND_NONE = 0,
    SUB_KIND_SET_BRIGHT,   /* data: 0=睡前 1=睡后 */
    SUB_KIND_SET_VOL,      /* data: 0=睡前 1=睡后 */
    SUB_KIND_SET_TIMER,    /* data: 0=睡前 1=睡后 */
    SUB_KIND_SET_MASK,     /* 睡后开关 after_on_off */
    SUB_KIND_SET_MUSIC,    /* 睡后音源 三选一 */
    SUB_KIND_DIY_PREVIEW,  /* DIY 试听整包 */
    SUB_KIND_DIY_CH,       /* data: 显示通道 0..7 */
    SUB_KIND_REC_BTN,      /* 录音 开始/停止 */
    SUB_KIND_REC_CLIP,     /* data: 排序后的录音行 */
};

#define SLEEP_SUB_MAX_ROW   20
#define SLEEP_PICK_MAX      8
#define SLEEP_PICK_KIND_TIMER  1
#define SLEEP_PICK_KIND_MUSIC  2
#define SLEEP_RECORD_MAX_SEC   (5u * 60u)
#define SLEEP_REC_TIMER_MS             200
/* 录音文件时长估算（与 divoom_lvgl_sleep_aid.c 一致） */
#define SLEEP_REC_WAV_HDR              44
#define SLEEP_REC_BYTES_PER_SEC        32000
#define SLEEP_CHECK_ICON       "23/at_repeat_check.png"
#define SLEEP_DIY_CH_COUNT  8
#define SLEEP_SET_TIMER_PRESETS 6
#define SLEEP_AFTER_MUSIC_COUNT 3
#define SLEEP_DIY_MIX_VOICE_ID  1

/* 行内列焦点 */
enum
{
    SLEEP_COL_TEXT = 0,
    SLEEP_COL_ACTION,
};

/* 延后动作 */
enum
{
    SLEEP_ACT_NONE = 0,
    SLEEP_ACT_FAV_TOGGLE,
};

/* 布局（px，对照真源 SleepSplitMenu.module.css） */
#define SLEEP_STRIP_TOP           6
#define SLEEP_STRIP_H             64
#define SLEEP_TAB_W               64
#define SLEEP_TAB_H               64
#define SLEEP_TAB_GAP             4
#define SLEEP_STRIP_BACKDROP_H    74
#define SLEEP_CONTENT_TOP         (SLEEP_STRIP_BACKDROP_H + 6)
#define SLEEP_CONTENT_H           (DIVOOM_SCREEN_HIGH - SLEEP_CONTENT_TOP)
#define SLEEP_ROW_W               472
#define SLEEP_ROW_H               72
#define SLEEP_ROW_GAP             4
/* 子页底部额外滚动余量，保证末行（含焦点边框）能完整进入视口 */
#define SLEEP_SUB_SCROLL_PAD      (DIVOOM_ASTRO_ITEM_BORDER_W_ACTIVE + SLEEP_ROW_GAP + 8)
#define SLEEP_ACTION_W            56
#define SLEEP_TEXT_W              (SLEEP_ROW_W - SLEEP_ACTION_W - 4)
#define SLEEP_LIST_PAD_X          ((DIVOOM_SCREEN_WIDTH - SLEEP_ROW_W) / 2)

/* 子页控件尺寸（对照真源 SleepSplitMenu.module.css） */
#define SLEEP_DIY_ICON_SZ         48
#define SLEEP_SET_CTRL_W          128   /* 设置/DIY 滑动条列宽 */
#define SLEEP_SET_VAL_W           128   /* 设置静态行右侧预览值宽度 */
#define SLEEP_SLIDER_H            10
#define SLEEP_SWITCH_W            44
#define SLEEP_SWITCH_H            24
#define SLEEP_ENTER_SZ            40
#define SLEEP_REC_BTN_SZ          36
#define SLEEP_REC_ACTION_H        180
#define SLEEP_CHEVRON_W           DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W
#define SLEEP_CHEVRON_H           DIVOOM_ASTRO_SUBLEVEL_CHEVRON_H
#define SLEEP_SET_VAL_RIGHT       (16 + SLEEP_CHEVRON_W + DIVOOM_ASTRO_LIST_LABEL_GAP)
#define SLEEP_CHEVRON_ICON        SLEEP_UI_DIR "sleep_chevron.png"
#define SLEEP_PICK_ROW_W          DIVOOM_ASTRO_CARD_ROW_W
#define SLEEP_PICK_ROW_H          DIVOOM_ASTRO_CARD_ROW_H
#define SLEEP_PICK_ROW_GAP        DIVOOM_ASTRO_CARD_ROW_GAP
#define SLEEP_PICK_ROW_X          ((DIVOOM_SCREEN_WIDTH - SLEEP_PICK_ROW_W) / 2)
#define SLEEP_PICK_TOP_GAP        8
#define SLEEP_PICK_CHECK_SZ       24
#define SLEEP_PICK_LABEL_L_PAD    8
#define SLEEP_ACT_ICON_SZ         28

/* pixelRail 滑动条配色（真源 FlowtooSlider.module.css .wrapPixelRail） */
#define SLEEP_SLIDER_TRACK_HI     0xA8A8A8
#define SLEEP_SLIDER_TRACK_LO     0x585858
#define SLEEP_SLIDER_FILL_HI      0x90FF78
#define SLEEP_SLIDER_FILL_LO      0x28B028
#define SLEEP_SLIDER_KNOB         0x8A8A8A

/* DIY「进入睡眠」头部文本色（真源 tone=cyan，AstroToo 取墨色保持可读） */
#define SLEEP_DIY_ENTER_INK       DIVOOM_ASTRO_INK

typedef struct _sleep_row
{
    lv_obj_t *btn;
    lv_obj_t *text_cont;
    lv_obj_t *name_label;
    lv_obj_t *play_icon;
    lv_obj_t *action_cont;
    lv_obj_t *action_icon;
    uint32 voice_id;
    int audio_type;
    int video_type;
    uint8 favored;
} sleep_row, *Psleep_row;

/* 子页一行（DIY/录音/设置）：标签 + 滑动条 / 图标 / 数值 */
typedef struct _sleep_sub_row
{
    lv_obj_t *btn;
    lv_obj_t *name_label;   /* 左：标签 */
    lv_obj_t *val_label;    /* 右：数值文本（定时/音源/录音时长） */
    lv_obj_t *slider;       /* 右：滑动条（亮度/音量/DIY 通道） */
    lv_obj_t *left_icon;    /* 左：DIY 通道图标 48×48 */
    lv_obj_t *right_icon;   /* 右：开关/录音/箭头/收藏图标 */
    lv_obj_t *text_cont;    /* 双列行：左文本区 */
    lv_obj_t *action_cont;  /* 双列行：右操作区（收藏） */
    uint32 voice_id;
    int audio_type;
    int video_type;
    uint8 favored;
    int kind;
    int data;
} sleep_sub_row;

typedef struct _divoom_astrootoo_sleep_ctrl
{
    divoom_astrootoo_ui_base ui;
    int menu_id;
    lv_obj_t *root_obj;

    /* 顶栏 7 Tab */
    lv_obj_t *strip_cont;
    lv_obj_t *tab_btn[SLEEP_TAB_COUNT];
    lv_obj_t *tab_icon[SLEEP_TAB_COUNT];

    int active_tab;
    int focus_pane;

    /* 内容区（列表行 / 简单页） */
    lv_obj_t *content;
    sleep_row *rows;
    int row_count;
    int total_count;     /* 分类过滤后总条数（分段判断） */
    int page_loaded;     /* 已加载条数 */
    int list_focus;
    int col_focus;
    int sub_col_focus;
    void *list_direct;

    /* DIY / 录音 / 设置 子页 */
    sleep_sub_row sub[SLEEP_SUB_MAX_ROW];
    int sub_count;
    int sub_focus;
    int pending_sub_focus;   /* 子页重建后恢复焦点（<0 不恢复） */
    void *sub_direct;
    lv_timer_t *rec_timer;
    int rec_elapsed;
    uint8 rec_ui_busy;
    uint32 rec_start_tick_ms;       /* 0=已冻住/未在录 */
    uint32 rec_clip_sec_cache;      /* 本局冻住秒，绑最新 clip 的 file_id */
    uint32 rec_clip_file_id_cache;

    /* 设置 · 定时/音源 二级 picker（对照 SleepSettingTimerScreen / SourcePostScreen） */
    lv_obj_t *pick_root;
    lv_obj_t *pick_list;
    lv_obj_t *pick_row[SLEEP_PICK_MAX];
    lv_obj_t *pick_check[SLEEP_PICK_MAX];
    lv_obj_t *pick_chevron[SLEEP_PICK_MAX];
    int pick_kind;
    int pick_data;
    int pick_count;
    int pick_focus;
    int pick_return_sub;
    void *pick_direct;

    /* 延后构建/追加/动作 */
    lv_timer_t *build_timer;
    lv_timer_t *append_timer;
    lv_timer_t *action_timer;
    lv_timer_t *refocus_timer;
    int action_kind;
    uint32 action_voice_id;
    int action_aid_type;
    int action_video_type;
} divoom_astrootoo_sleep_ctrl, *Pdivoom_astrootoo_sleep_ctrl;

static const int s_sleep_timer_presets[SLEEP_SET_TIMER_PRESETS] = { 15, 30, 60, 90, 120, 0 };

/* DIY 行号 = settings.volume[] = mix_pcm PCM 下标（风扇..颂钵，无 remap） */

/* DIY 8 通道显示名（与参考工程 s_diy_ch_lang_keys 一致顺序） */
static const char *const s_sleep_diy_ch_keys[SLEEP_DIY_CH_COUNT] =
{
    "SleepAidDiyFan", "SleepAidDiyFrog", "SleepAidDiyFire", "SleepAidDiyWave",
    "SleepAidDiyRain", "SleepAidDiyCreek", "SleepAidDiyBird", "SleepAidDiyBowl",
};

static const char *const s_sleep_after_music_keys[SLEEP_AFTER_MUSIC_COUNT] =
{
    "SleepAidAfterMusic0", "SleepAidAfterMusic1", "SleepAidAfterMusic2",
};

/* 前向声明 */
static void sleep_request_build(Pdivoom_astrootoo_sleep_ctrl ph);
static void sleep_build_content(Pdivoom_astrootoo_sleep_ctrl ph);
static void sleep_strip_key_cb(lv_event_t *e);
static void sleep_list_key_cb(lv_event_t *e);
static void sleep_paint_strip(Pdivoom_astrootoo_sleep_ctrl ph);
static void sleep_set_list_focus(Pdivoom_astrootoo_sleep_ctrl ph, int row, int col);
static void sleep_paint_sub(Pdivoom_astrootoo_sleep_ctrl ph, int idx);
static void sleep_set_sub_focus(Pdivoom_astrootoo_sleep_ctrl ph, int idx);
static void sleep_sub_refresh_row(Pdivoom_astrootoo_sleep_ctrl ph, int idx);
static void sleep_sub_key_cb(lv_event_t *e);
static void sleep_list_click_cb(lv_event_t *e);
static void sleep_sub_click_cb(lv_event_t *e);
static void sleep_enter_content(Pdivoom_astrootoo_sleep_ctrl ph);
static void sleep_rec_stop_timer(Pdivoom_astrootoo_sleep_ctrl ph);
static void sleep_rec_refresh_ui(Pdivoom_astrootoo_sleep_ctrl ph);
static void sleep_rec_start_refresh(Pdivoom_astrootoo_sleep_ctrl ph);
static int sleep_voice_is_favored(uint32 audio_id, int audio_type);
static int sleep_sub_row_has_split(const sleep_sub_row *sr);
static int sleep_row_fav_action_visible(Psleep_row r);
static int sleep_row_fav_action_enabled(Psleep_row r);
static int sleep_sub_fav_action_visible(const sleep_sub_row *sr);
static int sleep_sub_fav_action_enabled(const sleep_sub_row *sr);
static void sleep_row_refresh_fav_action(Psleep_row r);
static void sleep_sub_refresh_fav_action(sleep_sub_row *sr);
static void sleep_sub_set_fav_icon(sleep_sub_row *sr);
static uint32 sleep_rec_fav_voice_id(void);
static void sleep_stop_voice_preview(void);
static void sleep_apply_list_voice(Pdivoom_astrootoo_sleep_ctrl ph);
static void sleep_apply_sub_voice(Pdivoom_astrootoo_sleep_ctrl ph);
static void sleep_setup_on_sub_focus_change(Pdivoom_astrootoo_sleep_ctrl ph, int old_idx, int new_idx);
static void sleep_setup_start_after_music_preview(void);
static void sleep_enter_play_from_list_row(Psleep_row r);
static void sleep_enter_play_from_sub_row(const sleep_sub_row *sr);

static const char *sleep_rec_action_hint_key(int busy, int recorded)
{
    if (busy)
    {
        return "SleepAidRecordStop";
    }
    if (recorded)
    {
        return "SleepAidRecHintRerecord";
    }
    return "SleepAidRecHintRecord";
}

/* 请求结束录音（异步 commit；由 refresh timer 在 record_busy 清零后重建 UI） */
static void sleep_rec_request_stop(Pdivoom_astrootoo_sleep_ctrl ph)
{
    if (!ph)
    {
        return;
    }
    if (!divoom_sleep_aid_ctrl_record_busy())
    {
        return;
    }
    divoom_sleep_aid_ctrl_record_ctrl(0);
    sleep_rec_refresh_ui(ph);
    if (!ph->rec_timer)
    {
        sleep_rec_start_refresh(ph);
    }
}

static uint32 sleep_rec_clip_duration_sec(Pdivoom_astrootoo_sleep_ctrl ph, uint32 file_id)
{
    char path[256];
    uint8 hdr[4];
    FILE *fp;
    int sz;

    if (file_id == 0)
    {
        return 0;
    }
    if (ph && ph->rec_clip_file_id_cache == file_id && ph->rec_clip_sec_cache > 0u)
    {
        return ph->rec_clip_sec_cache;
    }
    divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_SLEEP_AID_RECORD, file_id, path);
    if (!divoom_port_file_exists(path))
    {
        return 0;
    }
    sz = divoom_app_com_get_file_size(path);
    if (sz <= 0)
    {
        return 0;
    }

    fp = divoom_port_file_open(path, "rb");
    if (fp && divoom_port_file_read(hdr, 1, sizeof(hdr), fp) == sizeof(hdr))
    {
        divoom_port_file_close(fp);
        if (hdr[0] == 'R' && hdr[1] == 'I' && hdr[2] == 'F' && hdr[3] == 'F')
        {
            if (sz <= SLEEP_REC_WAV_HDR)
            {
                return 0;
            }
            return (uint32)((sz - SLEEP_REC_WAV_HDR) / SLEEP_REC_BYTES_PER_SEC);
        }
    }
    else if (fp)
    {
        divoom_port_file_close(fp);
    }

    return divoom_mic_record_file_duration_sec(path);
}

static void sleep_rec_format_date_label(char *buf, int buflen, uint32 utc_sec)
{
    struct tm tm_info;
    time_t t;

    if (!buf || buflen < 12)
    {
        return;
    }
    if (utc_sec == 0)
    {
        lv_snprintf(buf, buflen, "--");
        return;
    }
    t = (time_t)utc_sec;
#if defined(_WIN32)
    if (localtime_s(&tm_info, &t) == 0)
#else
    if (localtime_r(&t, &tm_info) != NULL)
#endif
    {
        lv_snprintf(buf, buflen, "%04d-%02d-%02d",
            tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday);
    }
    else
    {
        lv_snprintf(buf, buflen, "--");
    }
}

static void sleep_rec_format_duration_label(char *buf, int buflen, uint32 sec)
{
    if (!buf || buflen < 8)
    {
        return;
    }
    lv_snprintf(buf, buflen, "%u:%02u", (unsigned)(sec / 60u), (unsigned)(sec % 60u));
}

/* 回放行：未录=请先录音+0:00；录音中=录音中+计时；已录=左日期+右时长 */
static void sleep_rec_refresh_playback_row(Pdivoom_astrootoo_sleep_ctrl ph, sleep_sub_row *sr, int busy)
{
    char date_buf[32];
    char dur_buf[16];

    if (!sr || !sr->val_label)
    {
        return;
    }

    if (busy)
    {
        if (sr->name_label)
        {
            lv_label_set_text(sr->name_label,
                divoom_menu_control_get_disp_str("SleepAidRecording"));
        }
        lv_snprintf(dur_buf, sizeof(dur_buf), "%d:%02d",
            ph->rec_elapsed / 60, ph->rec_elapsed % 60);
        lv_label_set_text(sr->val_label, dur_buf);
        return;
    }

    if (divoom_sleep_aid_ctrl_record_clip_count() > 0)
    {
        int voice_id;
        int slot;
        uint32 utc_sec;
        uint32 file_id;
        uint32 sec;

        voice_id = divoom_sleep_aid_ctrl_record_clip_id(0);
        slot = divoom_sleep_aid_ctrl_record_sorted_slot(0);
        utc_sec = divoom_sleep_aid_ctrl_record_slot_utc(slot);
        file_id = (voice_id >= 0)
            ? (uint32)(voice_id - DIVOOM_VOICE_ID_SLEEP_AID_RECORD_BASE) : 0;
        sec = sleep_rec_clip_duration_sec(ph, file_id);
        sleep_rec_format_date_label(date_buf, sizeof(date_buf), utc_sec);
        sleep_rec_format_duration_label(dur_buf, sizeof(dur_buf), sec);
        if (voice_id >= 0)
        {
            sr->voice_id = (uint32)voice_id;
            sr->favored = sleep_voice_is_favored(sr->voice_id, DIVOOM_SLEEP_AID_TYPE_RECORD) ? 1 : 0;
            if (sleep_sub_row_has_split(sr))
            {
                sleep_sub_refresh_fav_action(sr);
            }
        }
        if (sr->name_label)
        {
            lv_label_set_text(sr->name_label, date_buf);
        }
        lv_label_set_text(sr->val_label, dur_buf);
        return;
    }

    sr->voice_id = sleep_rec_fav_voice_id();
    sr->favored = sleep_voice_is_favored(sr->voice_id, DIVOOM_SLEEP_AID_TYPE_RECORD) ? 1 : 0;
    if (sleep_sub_row_has_split(sr))
    {
        sleep_sub_refresh_fav_action(sr);
    }
    if (sr->name_label)
    {
        lv_label_set_text(sr->name_label,
            divoom_menu_control_get_disp_str("SleepAidRecPleaseFirst"));
    }
    lv_label_set_text(sr->val_label, "0:00");
}
static void sleep_pick_destroy(Pdivoom_astrootoo_sleep_ctrl ph);
static void sleep_pick_enter(Pdivoom_astrootoo_sleep_ctrl ph, int kind, int data);
static void sleep_pick_exit(Pdivoom_astrootoo_sleep_ctrl ph);
static void sleep_sub_restore_group(Pdivoom_astrootoo_sleep_ctrl ph);
static void sleep_show_empty(Pdivoom_astrootoo_sleep_ctrl ph);

/* ---------- 工具 ---------- */

static int sleep_tab_aid_type(int tab)
{
    switch (tab)
    {
    case SLEEP_TAB_NOISE:
    {
        return DIVOOM_SLEEP_AID_TYPE_NOISE;
    }
    case SLEEP_TAB_NATURE:
    {
        return DIVOOM_SLEEP_AID_TYPE_WHITE;
    }
    case SLEEP_TAB_MUSIC:
    {
        return DIVOOM_SLEEP_AID_TYPE_MUSIC;
    }
    default:
    {
        return -1;
    }
    }
}

static int sleep_tab_is_category(int tab)
{
    return (tab == SLEEP_TAB_NOISE || tab == SLEEP_TAB_NATURE || tab == SLEEP_TAB_MUSIC);
}

static int sleep_tab_is_list(int tab)
{
    return (tab == SLEEP_TAB_LOCAL || sleep_tab_is_category(tab));
}

/* Tab 图标文件名片段（与 ui/106/sleep_tab_<key>_<idle|focus>.png 对应） */
static const char *sleep_tab_icon_key(int tab)
{
    switch (tab)
    {
    case SLEEP_TAB_LOCAL:
    {
        return "local";
    }
    case SLEEP_TAB_NOISE:
    {
        return "noise";
    }
    case SLEEP_TAB_NATURE:
    {
        return "nature";
    }
    case SLEEP_TAB_MUSIC:
    {
        return "music";
    }
    case SLEEP_TAB_DIY:
    {
        return "diy";
    }
    case SLEEP_TAB_REC:
    {
        return "rec";
    }
    default:
    {
        return "setup";
    }
    }
}

static int sleep_voice_is_favored(uint32 audio_id, int audio_type)
{
    divoom_sleep_aid_fav_item *favs;
    int n = 0;
    int i;

    if (audio_id == 0)
    {
        return 0;
    }
    favs = divoom_sleep_aid_fav_get_list(&n);
    if (!favs || n <= 0)
    {
        if (favs)
        {
            divoom_system_free(favs);
        }
        return 0;
    }
    for (i = 0; i < n; i++)
    {
        if (favs[i].audio_id == audio_id && favs[i].audio_type == audio_type)
        {
            divoom_system_free(favs);
            return 1;
        }
    }
    divoom_system_free(favs);
    return 0;
}

static int sleep_fav_total_count(void)
{
    divoom_sleep_aid_fav_item *favs;
    int n = 0;

    favs = divoom_sleep_aid_fav_get_list(&n);
    if (favs)
    {
        divoom_system_free(favs);
    }
    return n;
}

static int sleep_fav_unfav_blocked(int favored)
{
    if (!favored)
    {
        return 0;
    }
    return sleep_fav_total_count() <= 1 ? 1 : 0;
}

static int sleep_row_find_by_voice(Pdivoom_astrootoo_sleep_ctrl ph, uint32 voice_id, int aid_type)
{
    int i;

    for (i = 0; i < ph->row_count; i++)
    {
        if (ph->rows[i].voice_id == voice_id && ph->rows[i].audio_type == aid_type)
        {
            return i;
        }
    }
    return -1;
}

static int sleep_row_index_from_target(Pdivoom_astrootoo_sleep_ctrl ph, lv_obj_t *target, int *on_action)
{
    int i;

    if (on_action)
    {
        *on_action = 0;
    }
    if (!ph || !target)
    {
        return -1;
    }
    for (i = 0; i < ph->row_count; i++)
    {
        Psleep_row r = &ph->rows[i];

        if (r->action_cont == target || r->action_icon == target)
        {
            if (on_action)
            {
                *on_action = 1;
            }
            return i;
        }
        if (r->btn == target || r->text_cont == target || r->name_label == target
            || r->play_icon == target)
        {
            return i;
        }
    }
    return -1;
}

static int sleep_sub_index_from_target(Pdivoom_astrootoo_sleep_ctrl ph, lv_obj_t *target, int *on_action)
{
    int i;

    if (on_action)
    {
        *on_action = 0;
    }
    if (!ph || !target)
    {
        return -1;
    }
    for (i = 0; i < ph->sub_count; i++)
    {
        sleep_sub_row *sr = &ph->sub[i];

        if (sr->action_cont == target || sr->right_icon == target)
        {
            if (on_action)
            {
                *on_action = 1;
            }
            return i;
        }
        if (sr->btn == target || sr->text_cont == target || sr->name_label == target
            || sr->val_label == target)
        {
            return i;
        }
    }
    return -1;
}

static void sleep_row_bind_pointer(Pdivoom_astrootoo_sleep_ctrl ph, Psleep_row r)
{
    if (!ph || !r || !r->btn)
    {
        return;
    }
    lv_obj_add_event_cb(r->btn, sleep_list_click_cb, LV_EVENT_CLICKED, ph);
    if (r->text_cont)
    {
        lv_obj_add_flag(r->text_cont, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(r->text_cont, sleep_list_click_cb, LV_EVENT_CLICKED, ph);
    }
    if (r->action_cont)
    {
        lv_obj_add_flag(r->action_cont, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(r->action_cont, sleep_list_click_cb, LV_EVENT_CLICKED, ph);
    }
}

static void sleep_sub_bind_pointer(Pdivoom_astrootoo_sleep_ctrl ph, sleep_sub_row *sr)
{
    if (!ph || !sr || !sr->btn)
    {
        return;
    }
    lv_obj_add_event_cb(sr->btn, sleep_sub_click_cb, LV_EVENT_CLICKED, ph);
    if (sr->text_cont)
    {
        lv_obj_add_flag(sr->text_cont, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(sr->text_cont, sleep_sub_click_cb, LV_EVENT_CLICKED, ph);
    }
    if (sr->action_cont)
    {
        lv_obj_add_flag(sr->action_cont, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(sr->action_cont, sleep_sub_click_cb, LV_EVENT_CLICKED, ph);
    }
}

static void sleep_apply_slot(lv_obj_t *o, int focus)
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

/* 最后一项收藏不可取消：仍显示收藏键，但置灰且不可聚焦 */
static void sleep_apply_slot_disabled(lv_obj_t *o)
{
    lv_obj_set_style_bg_color(o, lv_color_hex(DIVOOM_ASTRO_BG_MUTED), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(o, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_border_width(o, DIVOOM_ASTRO_ITEM_BORDER_W_INACTIVE, 0);
    lv_obj_set_style_radius(o, DIVOOM_ASTRO_RADIUS_SM, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
}

static void sleep_row_set_fav_icon(Psleep_row r)
{
    if (!r || !r->action_icon)
    {
        return;
    }
    divoom_menu_control_lv_img_set_src(r->action_icon,
        r->favored ? SLEEP_FAV_ICON_ON : SLEEP_FAV_ICON);
    lv_obj_center(r->action_icon);
}

static void sleep_sub_set_fav_icon(sleep_sub_row *sr)
{
    if (!sr || !sr->right_icon)
    {
        return;
    }
    divoom_menu_control_lv_img_set_src(sr->right_icon,
        sr->favored ? SLEEP_FAV_ICON_ON : SLEEP_FAV_ICON);
    lv_obj_center(sr->right_icon);
}

static int sleep_sub_row_has_split(const sleep_sub_row *sr)
{
    return (sr && sr->text_cont && sr->action_cont) ? 1 : 0;
}

static int sleep_row_fav_action_visible(Psleep_row r)
{
    return (r && r->action_cont) ? 1 : 0;
}

static int sleep_row_fav_action_enabled(Psleep_row r)
{
    if (!sleep_row_fav_action_visible(r))
    {
        return 0;
    }
    if (sleep_fav_unfav_blocked(r->favored))
    {
        return 0;
    }
    return 1;
}

static int sleep_sub_fav_action_visible(const sleep_sub_row *sr)
{
    return sleep_sub_row_has_split(sr) ? 1 : 0;
}

static int sleep_sub_fav_action_enabled(const sleep_sub_row *sr)
{
    if (!sleep_sub_fav_action_visible(sr))
    {
        return 0;
    }
    if (sleep_fav_unfav_blocked(sr->favored))
    {
        return 0;
    }
    return 1;
}

static void sleep_row_refresh_fav_action(Psleep_row r)
{
    if (!r || !r->action_cont)
    {
        return;
    }
    if (sleep_row_fav_action_visible(r))
    {
        lv_obj_clear_flag(r->action_cont, LV_OBJ_FLAG_HIDDEN);
        sleep_row_set_fav_icon(r);
        lv_obj_set_style_img_opa(r->action_icon,
            sleep_row_fav_action_enabled(r) ? LV_OPA_COVER : LV_OPA_40, 0);
    }
    else
    {
        lv_obj_add_flag(r->action_cont, LV_OBJ_FLAG_HIDDEN);
    }
}

static void sleep_sub_refresh_fav_action(sleep_sub_row *sr)
{
    if (!sr || !sr->action_cont)
    {
        return;
    }
    if (sleep_sub_fav_action_visible(sr))
    {
        lv_obj_clear_flag(sr->action_cont, LV_OBJ_FLAG_HIDDEN);
        sleep_sub_set_fav_icon(sr);
        lv_obj_set_style_img_opa(sr->right_icon,
            sleep_sub_fav_action_enabled(sr) ? LV_OPA_COVER : LV_OPA_40, 0);
    }
    else
    {
        lv_obj_add_flag(sr->action_cont, LV_OBJ_FLAG_HIDDEN);
    }
}

/* 录音收藏 ID：无片段时用 record_base+1 占位，避免 voice_id==0 无法收藏 */
static uint32 sleep_rec_fav_voice_id(void)
{
    int vid = divoom_sleep_aid_ctrl_record_clip_id(0);

    if (vid >= 0)
    {
        return (uint32)vid;
    }
    return (uint32)(DIVOOM_VOICE_ID_SLEEP_AID_RECORD_BASE + 1);
}

/* 子页双列行：左 text_cont（412px）+ 右 action_cont（56px），对照 AstroTooUI localRow */
static void sleep_sub_begin_split_row(Pdivoom_astrootoo_sleep_ctrl ph, sleep_sub_row *sr,
    lv_coord_t row_h, uint32 audio_id, int audio_type, int video_type)
{
    if (!ph || !sr || !sr->btn)
    {
        return;
    }

    if (row_h <= 0)
    {
        row_h = SLEEP_ROW_H;
    }
    if (video_type < 0 && audio_id != 0)
    {
        video_type = divoom_sleep_aid_voice_get_video_type(audio_type, audio_id);
    }
    sr->voice_id = audio_id;
    sr->audio_type = audio_type;
    sr->video_type = video_type;
    sr->favored = sleep_voice_is_favored(audio_id, audio_type) ? 1 : 0;

    lv_obj_set_style_bg_opa(sr->btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sr->btn, 0, 0);

    sr->text_cont = lv_obj_create(sr->btn);
    sleep_apply_slot(sr->text_cont, 0);
    lv_obj_set_size(sr->text_cont, SLEEP_TEXT_W, row_h);
    lv_obj_set_pos(sr->text_cont, 0, 0);
    lv_obj_clear_flag(sr->text_cont, LV_OBJ_FLAG_SCROLLABLE);

    sr->action_cont = lv_obj_create(sr->btn);
    sleep_apply_slot(sr->action_cont, 0);
    lv_obj_set_size(sr->action_cont, SLEEP_ACTION_W, row_h);
    lv_obj_set_pos(sr->action_cont, SLEEP_TEXT_W + 4, 0);
    lv_obj_clear_flag(sr->action_cont, LV_OBJ_FLAG_SCROLLABLE);

    sr->right_icon = lv_img_create(sr->action_cont);
    sleep_sub_refresh_fav_action(sr);
}

/* ---------- 列表构建 ---------- */

static void sleep_free_list(Pdivoom_astrootoo_sleep_ctrl ph)
{
    int i;

    sleep_pick_destroy(ph);
    sleep_rec_stop_timer(ph);
    if (ph->action_timer)
    {
        lv_timer_del(ph->action_timer);
        ph->action_timer = NULL;
    }
    ph->action_kind = SLEEP_ACT_NONE;
    if (ph->list_direct)
    {
        divoom_menu_direct_ctrl_destory(ph->list_direct);
        ph->list_direct = NULL;
    }
    if (ph->sub_direct)
    {
        divoom_menu_direct_ctrl_destory(ph->sub_direct);
        ph->sub_direct = NULL;
    }
    if (ph->content)
    {
        lv_obj_clean(ph->content);
    }
    if (ph->rows)
    {
        divoom_system_free(ph->rows);
        ph->rows = NULL;
    }
    ph->row_count = 0;
    ph->page_loaded = 0;
    ph->total_count = 0;
    ph->list_focus = 0;
    ph->col_focus = SLEEP_COL_TEXT;
    ph->sub_col_focus = SLEEP_COL_TEXT;

    for (i = 0; i < SLEEP_SUB_MAX_ROW; i++)
    {
        ph->sub[i].btn = NULL;
        ph->sub[i].val_label = NULL;
        ph->sub[i].text_cont = NULL;
        ph->sub[i].action_cont = NULL;
        ph->sub[i].kind = SUB_KIND_NONE;
        ph->sub[i].data = 0;
    }
    ph->sub_count = 0;
    ph->sub_focus = 0;
}

static void sleep_create_row(Pdivoom_astrootoo_sleep_ctrl ph, int idx,
    const divoom_sleep_aid_voice_list_item *item, int favored)
{
    Psleep_row r;
    lv_obj_t *btn;
    lv_coord_t y;
    char name_buf[DIVOOM_SLEEP_AID_VOICE_NAME_LEN];

    r = &ph->rows[idx];
    y = idx * (SLEEP_ROW_H + SLEEP_ROW_GAP);

    btn = lv_btn_create(ph->content);
    lv_obj_add_style(btn, &ph->ui.content_style, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_size(btn, SLEEP_ROW_W, SLEEP_ROW_H);
    lv_obj_set_pos(btn, SLEEP_LIST_PAD_X, y);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    /* 左：文本区（播放图标 + 名称） */
    r->text_cont = lv_obj_create(btn);
    sleep_apply_slot(r->text_cont, 0);
    lv_obj_set_size(r->text_cont, SLEEP_TEXT_W, SLEEP_ROW_H);
    lv_obj_set_pos(r->text_cont, 0, 0);
    lv_obj_clear_flag(r->text_cont, LV_OBJ_FLAG_SCROLLABLE);

    r->play_icon = lv_label_create(r->text_cont);
    lv_label_set_text(r->play_icon, "\xe2\x99\xaa"); /* ♪ U+266A */
    lv_obj_set_style_text_font(r->play_icon,
        divoom_menu_control_get_font_info(24, ph->menu_id), 0);
    lv_obj_set_style_text_color(r->play_icon, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_align(r->play_icon, LV_ALIGN_LEFT_MID, 14, 0);

    /* 显示名优先用列表项 name；缺失时按 cfg 语言文件查名；再缺退化为本地化前缀+ID */
    r->name_label = lv_label_create(r->text_cont);
    name_buf[0] = '\0';
    if (item && item->name[0])
    {
        strncpy(name_buf, item->name, sizeof(name_buf) - 1);
        name_buf[sizeof(name_buf) - 1] = '\0';
    }
    else if (item && item->audio_id)
    {
        divoom_sleep_aid_send_get_voice_name(item->audio_type, item->audio_id, name_buf);
    }
    if (name_buf[0] == '\0')
    {
        snprintf(name_buf, sizeof(name_buf), "%s%u",
            divoom_menu_control_get_disp_str("SleepAidVoicePrefix"),
            (unsigned)(item ? item->audio_id : 0));
    }
    lv_label_set_text(r->name_label, name_buf);
    lv_obj_set_style_text_color(r->name_label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(r->name_label,
        divoom_menu_control_get_font_info(24, ph->menu_id), 0);
    divoom_astrootoo_ui_list_label_prepare(r->name_label,
        SLEEP_TEXT_W - 44 - DIVOOM_ASTRO_LIST_LABEL_GAP);
    lv_obj_align(r->name_label, LV_ALIGN_LEFT_MID, 44, 0);
    divoom_astrootoo_ui_list_row_bind_marquee(btn, r->name_label);

    /* 右：操作区（收藏 / 取消收藏） */
    r->action_cont = lv_obj_create(btn);
    sleep_apply_slot(r->action_cont, 0);
    lv_obj_set_size(r->action_cont, SLEEP_ACTION_W, SLEEP_ROW_H);
    lv_obj_set_pos(r->action_cont, SLEEP_TEXT_W + 4, 0);
    lv_obj_clear_flag(r->action_cont, LV_OBJ_FLAG_SCROLLABLE);

    r->action_icon = lv_img_create(r->action_cont);
    lv_obj_center(r->action_icon);

    r->voice_id = item ? item->audio_id : 0;
    r->audio_type = item ? item->audio_type : -1;
    r->video_type = item ? item->video_type : -1;
    r->favored = (uint8)favored;
    sleep_row_refresh_fav_action(r);

    lv_obj_add_event_cb(btn, sleep_list_key_cb, LV_EVENT_KEY, ph);
    lv_group_add_obj(lv_group_get_default(), btn);
    r->btn = btn;
    sleep_row_bind_pointer(ph, r);
}

/* 分类列表：分段加载，行尾显示收藏态 */
static void sleep_build_category(Pdivoom_astrootoo_sleep_ctrl ph)
{
    int aid_type;
    divoom_sleep_aid_voice_list_item *page;
    int n = 0;
    int total = 0;
    int i;

    aid_type = sleep_tab_aid_type(ph->active_tab);
    page = divoom_sleep_aid_voice_get_list_page(aid_type, 0, SLEEP_PAGE_BATCH, -1, &n, &total);
    ph->total_count = total;
    if (!page || n <= 0)
    {
        if (page)
        {
            divoom_system_free(page);
        }
        return;
    }

    ph->rows = (sleep_row *)divoom_system_malloc((unsigned)total * sizeof(sleep_row));
    if (!ph->rows)
    {
        divoom_system_free(page);
        ph->total_count = 0;
        return;
    }

    for (i = 0; i < n; i++)
    {
        sleep_create_row(ph, i, &page[i],
            sleep_voice_is_favored(page[i].audio_id, page[i].audio_type));
    }
    divoom_system_free(page);

    ph->row_count = n;
    ph->page_loaded = n;

    ph->list_direct = divoom_menu_direct_ctrl_create(1, ph->row_count);
    for (i = 0; i < ph->row_count; i++)
    {
        divoom_menu_direct_ctrl_update(ph->list_direct, ph->rows[i].btn, 0, i);
    }
}

/* 收藏列表（对照 AstroTooUI local/favorites Tab） */
static void sleep_build_favorites(Pdivoom_astrootoo_sleep_ctrl ph)
{
    divoom_sleep_aid_voice_list_item *items;
    int n = 0;
    int i;

    items = divoom_sleep_aid_voice_get_list(DIVOOM_SLEEP_AID_TYPE_FAV, &n, -1);
    if (!items || n <= 0)
    {
        if (items)
        {
            divoom_system_free(items);
        }
        return;
    }

    ph->rows = (sleep_row *)divoom_system_malloc((unsigned)n * sizeof(sleep_row));
    if (!ph->rows)
    {
        divoom_system_free(items);
        return;
    }

    for (i = 0; i < n; i++)
    {
        sleep_create_row(ph, i, &items[i], 1);
    }
    divoom_system_free(items);

    ph->row_count = n;
    ph->page_loaded = n;
    ph->total_count = n;

    ph->list_direct = divoom_menu_direct_ctrl_create(1, ph->row_count);
    for (i = 0; i < ph->row_count; i++)
    {
        divoom_menu_direct_ctrl_update(ph->list_direct, ph->rows[i].btn, 0, i);
    }
}

/* ---------- 子页（DIY / 录音 / 设置）---------- */

/* 子页分区标题（非聚焦） */
static void sleep_sub_add_section(Pdivoom_astrootoo_sleep_ctrl ph, lv_coord_t *y, const char *key)
{
    lv_obj_t *lab;

    lab = lv_label_create(ph->content);
    lv_label_set_text(lab, divoom_menu_control_get_disp_str(key));
    lv_obj_set_style_text_color(lab, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(lab, divoom_menu_control_get_font_info(16, ph->menu_id), 0);
    lv_obj_set_pos(lab, SLEEP_LIST_PAD_X + 4, *y + 6);
    *y += 30;
}

/* pixelRail 绿轨滑动条（只读展示，由左右键驱动数值） */
static lv_obj_t *sleep_make_slider(lv_obj_t *parent, int value)
{
    lv_obj_t *s;

    s = lv_slider_create(parent);
    lv_obj_clear_flag(s, LV_OBJ_FLAG_CLICKABLE);
    lv_slider_set_range(s, 0, 100);
    lv_slider_set_value(s, value, LV_ANIM_OFF);
    lv_obj_set_height(s, SLEEP_SLIDER_H);

    /* 轨道：上浅下深灰 */
    lv_obj_set_style_bg_color(s, lv_color_hex(SLEEP_SLIDER_TRACK_HI), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(s, lv_color_hex(SLEEP_SLIDER_TRACK_LO), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(s, LV_GRAD_DIR_VER, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(s, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(s, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(s, 0, LV_PART_MAIN);

    /* 填充：上亮下深绿 */
    lv_obj_set_style_bg_color(s, lv_color_hex(SLEEP_SLIDER_FILL_HI), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(s, lv_color_hex(SLEEP_SLIDER_FILL_LO), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_dir(s, LV_GRAD_DIR_VER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(s, 0, LV_PART_INDICATOR);

    /* 滑块：灰底白边竖块 */
    lv_obj_set_style_bg_color(s, lv_color_hex(SLEEP_SLIDER_KNOB), LV_PART_KNOB);
    lv_obj_set_style_radius(s, 0, LV_PART_KNOB);
    lv_obj_set_style_border_color(s, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
    lv_obj_set_style_border_width(s, 1, LV_PART_KNOB);
    lv_obj_set_style_pad_hor(s, 3, LV_PART_KNOB);
    lv_obj_set_style_pad_ver(s, 6, LV_PART_KNOB);
    lv_obj_set_style_outline_width(s, 0, LV_PART_KNOB);
    return s;
}

/* 子页基础行：创建可聚焦容器（统一槽样式），返回行下标 */
static int sleep_sub_new(Pdivoom_astrootoo_sleep_ctrl ph, lv_coord_t *y, lv_coord_t h,
    int kind, int data, int split_row)
{
    sleep_sub_row *sr;
    lv_obj_t *btn;

    if (ph->sub_count >= SLEEP_SUB_MAX_ROW)
    {
        return -1;
    }
    sr = &ph->sub[ph->sub_count];
    sr->name_label = NULL;
    sr->val_label = NULL;
    sr->slider = NULL;
    sr->left_icon = NULL;
    sr->right_icon = NULL;
    sr->text_cont = NULL;
    sr->action_cont = NULL;
    sr->voice_id = 0;
    sr->audio_type = -1;
    sr->video_type = -1;
    sr->favored = 0;

    btn = lv_btn_create(ph->content);
    lv_obj_add_style(btn, &ph->ui.content_style, 0);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_size(btn, SLEEP_ROW_W, h);
    lv_obj_set_pos(btn, SLEEP_LIST_PAD_X, *y);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    if (split_row)
    {
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
    }
    else
    {
        sleep_apply_slot(btn, 0);
    }

    sr->btn = btn;
    sr->kind = kind;
    sr->data = data;

    lv_obj_add_event_cb(btn, sleep_sub_key_cb, LV_EVENT_KEY, ph);
    lv_group_add_obj(lv_group_get_default(), btn);

    ph->sub_count++;
    *y += h + SLEEP_ROW_GAP;
    return ph->sub_count - 1;
}

/* 行内左侧标签 */
static lv_obj_t *sleep_sub_make_label(Pdivoom_astrootoo_sleep_ctrl ph, lv_obj_t *btn,
    const char *label_key, lv_coord_t w, uint32 color)
{
    lv_obj_t *lab;

    lab = lv_label_create(btn);
    lv_label_set_text(lab, divoom_menu_control_get_disp_str(label_key));
    lv_obj_set_style_text_color(lab, lv_color_hex(color), 0);
    lv_obj_set_style_text_font(lab, divoom_menu_control_get_font_info(20, ph->menu_id), 0);
    lv_label_set_long_mode(lab, LV_LABEL_LONG_DOT);
    lv_obj_set_width(lab, w);
    lv_obj_align(lab, LV_ALIGN_LEFT_MID, 16, 0);
    return lab;
}

/* 设置/DIY 滑动条行：左标签(可空) + 右滑动条 128px */
static int sleep_sub_add_slider_row(Pdivoom_astrootoo_sleep_ctrl ph, lv_coord_t *y,
    int kind, int data, const char *label_key, int value)
{
    int idx;
    sleep_sub_row *sr;

    idx = sleep_sub_new(ph, y, SLEEP_ROW_H, kind, data, 0);
    if (idx < 0)
    {
        return -1;
    }
    sr = &ph->sub[idx];
    if (label_key)
    {
        sr->name_label = sleep_sub_make_label(ph, sr->btn, label_key,
            SLEEP_ROW_W - SLEEP_SET_CTRL_W - 40, DIVOOM_ASTRO_INK);
    }
    sr->slider = sleep_make_slider(sr->btn, value);
    lv_obj_set_width(sr->slider, SLEEP_SET_CTRL_W);
    lv_obj_align(sr->slider, LV_ALIGN_RIGHT_MID, -16, 0);
    return idx;
}

/* 设置静态行：左标签 + 右数值 + 箭头 */
static int sleep_sub_add_static_row(Pdivoom_astrootoo_sleep_ctrl ph, lv_coord_t *y,
    int kind, int data, const char *label_key)
{
    int idx;
    sleep_sub_row *sr;

    idx = sleep_sub_new(ph, y, SLEEP_ROW_H, kind, data, 0);
    if (idx < 0)
    {
        return -1;
    }
    sr = &ph->sub[idx];
    sr->name_label = sleep_sub_make_label(ph, sr->btn, label_key,
        SLEEP_ROW_W - 16 - SLEEP_SET_VAL_W - SLEEP_SET_VAL_RIGHT - DIVOOM_ASTRO_LIST_LABEL_GAP,
        DIVOOM_ASTRO_INK);

    sr->right_icon = lv_img_create(sr->btn);
    divoom_menu_control_lv_img_set_src(sr->right_icon, SLEEP_CHEVRON_ICON);
    divoom_astrootoo_ui_apply_sublevel_chevron_size(sr->right_icon);
    lv_obj_align(sr->right_icon, LV_ALIGN_RIGHT_MID, -16, 0);

    sr->val_label = lv_label_create(sr->btn);
    lv_obj_set_style_text_color(sr->val_label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(sr->val_label, divoom_menu_control_get_font_info(20, ph->menu_id), 0);
    divoom_astrootoo_ui_list_label_prepare(sr->val_label, SLEEP_SET_VAL_W);
    lv_obj_set_style_text_align(sr->val_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(sr->val_label, LV_ALIGN_RIGHT_MID, -SLEEP_SET_VAL_RIGHT, 0);
    divoom_astrootoo_ui_list_row_bind_marquee(sr->btn, sr->val_label);
    return idx;
}

/* 设置开关行：左标签 + 右开关图标（on/off） */
static int sleep_sub_add_toggle_row(Pdivoom_astrootoo_sleep_ctrl ph, lv_coord_t *y,
    int kind, int data, const char *label_key)
{
    int idx;
    sleep_sub_row *sr;

    idx = sleep_sub_new(ph, y, SLEEP_ROW_H, kind, data, 0);
    if (idx < 0)
    {
        return -1;
    }
    sr = &ph->sub[idx];
    sr->name_label = sleep_sub_make_label(ph, sr->btn, label_key,
        SLEEP_ROW_W - SLEEP_SET_CTRL_W - 40, DIVOOM_ASTRO_INK);

    sr->right_icon = lv_img_create(sr->btn);
    lv_obj_align(sr->right_icon, LV_ALIGN_RIGHT_MID, -16, 0);
    return idx;
}

/* 设置定时分钟值 -> 显示文本 */
static void sleep_fmt_timer(char *buf, int sz, int minutes)
{
    if (minutes <= 0)
    {
        snprintf(buf, (unsigned)sz, "%s", divoom_menu_control_get_disp_str("SleepAidTimerOff"));
    }
    else
    {
        snprintf(buf, (unsigned)sz, "%d %s", minutes,
            divoom_menu_control_get_disp_str("SleepAidTimerUnit"));
    }
}

/* 定时分钟 -> 预设下标（{15,30,60,90,120,0}） */
static int sleep_timer_preset_index(int minutes)
{
    int i;

    for (i = 0; i < SLEEP_SET_TIMER_PRESETS; i++)
    {
        if (s_sleep_timer_presets[i] == minutes)
        {
            return i;
        }
    }
    return 1;
}

/* ---------- 设置 · 定时/音源 二级 picker ---------- */

static void sleep_pick_apply_row(Pdivoom_astrootoo_sleep_ctrl ph, int idx, int focused)
{
    lv_obj_t *btn;

    if (idx < 0 || idx >= ph->pick_count)
    {
        return;
    }
    btn = ph->pick_row[idx];
    if (!btn)
    {
        return;
    }
    if (focused)
    {
        lv_obj_add_state(btn, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    }
    else
    {
        lv_obj_clear_state(btn, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    }
}

static void sleep_pick_refresh_checks(Pdivoom_astrootoo_sleep_ctrl ph)
{
    divoom_sleep_aid_info *inf;
    int committed = 0;
    int i;

    inf = divoom_sleep_aid_ctrl_get_settings();
    if (!inf)
    {
        return;
    }
    if (ph->pick_kind == SLEEP_PICK_KIND_TIMER)
    {
        committed = sleep_timer_preset_index(ph->pick_data ? inf->after_time : inf->before_time);
    }
    else if (ph->pick_kind == SLEEP_PICK_KIND_MUSIC)
    {
        committed = inf->after_music;
        if (committed < 0 || committed >= SLEEP_AFTER_MUSIC_COUNT)
        {
            committed = 0;
        }
    }

    for (i = 0; i < ph->pick_count; i++)
    {
        if (ph->pick_check[i])
        {
            if (i == committed)
            {
                lv_obj_clear_flag(ph->pick_check[i], LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_obj_add_flag(ph->pick_check[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        sleep_pick_apply_row(ph, i, (i == ph->pick_focus) ? 1 : 0);
    }
}

static void sleep_pick_set_focus(Pdivoom_astrootoo_sleep_ctrl ph, int idx)
{
    int old;

    if (ph->pick_count <= 0)
    {
        return;
    }
    if (idx < 0)
    {
        idx = 0;
    }
    if (idx >= ph->pick_count)
    {
        idx = ph->pick_count - 1;
    }
    old = ph->pick_focus;
    ph->pick_focus = idx;
    sleep_pick_apply_row(ph, old, 0);
    sleep_pick_apply_row(ph, idx, 1);
    if (ph->pick_row[idx])
    {
        lv_group_focus_obj(ph->pick_row[idx]);
        divoom_astrootoo_ui_focus_scroll(ph->pick_row[idx]);
    }
    if (ph->pick_kind == SLEEP_PICK_KIND_MUSIC)
    {
        divoom_sleep_aid_info *inf = divoom_sleep_aid_ctrl_get_settings();

        if (inf)
        {
            divoom_sleep_aid_ctrl_settings_after_music_preview(idx, inf->after_vol);
        }
    }
}

static void sleep_pick_commit(Pdivoom_astrootoo_sleep_ctrl ph, int idx)
{
    divoom_sleep_aid_info *inf;

    inf = divoom_sleep_aid_ctrl_get_settings();
    if (!inf || idx < 0 || idx >= ph->pick_count)
    {
        sleep_pick_exit(ph);
        return;
    }

    if (ph->pick_kind == SLEEP_PICK_KIND_TIMER)
    {
        int *pv = ph->pick_data ? &inf->after_time : &inf->before_time;

        *pv = s_sleep_timer_presets[idx];
    }
    else if (ph->pick_kind == SLEEP_PICK_KIND_MUSIC)
    {
        inf->after_music = idx;
    }
    divoom_sleep_aid_ctrl_settings_save();
    sleep_pick_exit(ph);
}

static void sleep_pick_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_sleep_ctrl ph;
    uint8_t key_code;
    lv_obj_t *target;
    int i;
    int found;

    ph = (Pdivoom_astrootoo_sleep_ctrl)lv_event_get_user_data(e);
    if (!ph || lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    if (ph->focus_pane != SLEEP_PANE_PICK || ph->pick_count <= 0)
    {
        return;
    }

    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    target = lv_event_get_target(e);
    found = ph->pick_focus;
    for (i = 0; i < ph->pick_count; i++)
    {
        if (ph->pick_row[i] == target)
        {
            found = i;
            break;
        }
    }
    ph->pick_focus = found;

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        sleep_pick_exit(ph);
        return;
    }

    if (divoom_astrootoo_ui_key_is_up(key_code))
    {
        sleep_pick_set_focus(ph, (found + ph->pick_count - 1) % ph->pick_count);
        return;
    }

    if (divoom_astrootoo_ui_key_is_down(key_code))
    {
        sleep_pick_set_focus(ph, (found + 1) % ph->pick_count);
        return;
    }

    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        sleep_pick_commit(ph, found);
    }
}

static void sleep_pick_destroy(Pdivoom_astrootoo_sleep_ctrl ph)
{
    int i;

    if (!ph)
    {
        return;
    }
    if (ph->pick_direct)
    {
        divoom_menu_direct_ctrl_destory(ph->pick_direct);
        ph->pick_direct = NULL;
    }
    if (ph->pick_root)
    {
        lv_obj_del(ph->pick_root);
        ph->pick_root = NULL;
    }
    ph->pick_list = NULL;
    ph->pick_count = 0;
    ph->pick_focus = 0;
    ph->pick_kind = 0;
    ph->pick_data = 0;
    for (i = 0; i < SLEEP_PICK_MAX; i++)
    {
        ph->pick_row[i] = NULL;
        ph->pick_check[i] = NULL;
        ph->pick_chevron[i] = NULL;
    }
    if (ph->focus_pane == SLEEP_PANE_PICK)
    {
        ph->focus_pane = SLEEP_PANE_SUB;
    }
}

static void sleep_grp_add(lv_group_t *g, lv_obj_t *obj)
{
    if (g && obj && lv_obj_is_valid(obj))
    {
        lv_group_add_obj(g, obj);
    }
}

static void sleep_sub_restore_group(Pdivoom_astrootoo_sleep_ctrl ph)
{
    lv_group_t *g;
    int i;

    g = lv_group_get_default();
    if (!g)
    {
        return;
    }
    lv_group_remove_all_objs(g);
    for (i = 0; i < ph->sub_count; i++)
    {
        sleep_grp_add(g, ph->sub[i].btn);
    }
    for (i = 0; i < SLEEP_TAB_COUNT; i++)
    {
        sleep_grp_add(g, ph->tab_btn[i]);
    }
}

static void sleep_pick_restore_group(Pdivoom_astrootoo_sleep_ctrl ph)
{
    lv_group_t *g;
    int i;

    g = lv_group_get_default();
    if (!g)
    {
        return;
    }
    lv_group_remove_all_objs(g);
    for (i = 0; i < ph->pick_count; i++)
    {
        sleep_grp_add(g, ph->pick_row[i]);
    }
}

static void sleep_strip_restore_group(Pdivoom_astrootoo_sleep_ctrl ph)
{
    lv_group_t *g;
    int i;

    g = lv_group_get_default();
    if (!g)
    {
        return;
    }
    lv_group_remove_all_objs(g);
    for (i = 0; i < SLEEP_TAB_COUNT; i++)
    {
        sleep_grp_add(g, ph->tab_btn[i]);
    }
    for (i = 0; i < ph->row_count; i++)
    {
        if (ph->rows)
        {
            sleep_grp_add(g, ph->rows[i].btn);
        }
    }
    for (i = 0; i < ph->sub_count; i++)
    {
        sleep_grp_add(g, ph->sub[i].btn);
    }
}

static void sleep_pick_build(Pdivoom_astrootoo_sleep_ctrl ph, const char *title_key)
{
    divoom_sleep_aid_info *inf;
    lv_coord_t y;
    int i;
    int n = 0;
    int initial = 0;
    char buf[64];

    inf = divoom_sleep_aid_ctrl_get_settings();
    if (!inf)
    {
        return;
    }

    ph->pick_root = lv_obj_create(ph->root_obj);
    lv_obj_add_style(ph->pick_root, &ph->ui.root_style, 0);
    lv_obj_set_size(ph->pick_root, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_pos(ph->pick_root, 0, 0);
    lv_obj_clear_flag(ph->pick_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_foreground(ph->pick_root);

    /* 与系统设置子页一致：tab 顶栏图（非四圆角纯色条） */
    divoom_astrootoo_ui_create_subpage_header(ph->pick_root, &ph->ui, title_key, ph->menu_id);
    ph->pick_list = divoom_astrootoo_ui_create_scroll_content(ph->pick_root, &ph->ui);

    if (ph->pick_kind == SLEEP_PICK_KIND_TIMER)
    {
        n = SLEEP_SET_TIMER_PRESETS;
        initial = sleep_timer_preset_index(ph->pick_data ? inf->after_time : inf->before_time);
    }
    else if (ph->pick_kind == SLEEP_PICK_KIND_MUSIC)
    {
        n = SLEEP_AFTER_MUSIC_COUNT;
        initial = inf->after_music;
        if (initial < 0 || initial >= SLEEP_AFTER_MUSIC_COUNT)
        {
            initial = 0;
        }
    }

    ph->pick_count = n;
    if (n <= 0)
    {
        return;
    }

    ph->pick_direct = divoom_menu_direct_ctrl_create(1, n);
    y = SLEEP_PICK_TOP_GAP;

    for (i = 0; i < n; i++)
    {
        lv_obj_t *btn;
        lv_obj_t *lab;
        lv_obj_t *chk;

        btn = lv_btn_create(ph->pick_list);
        lv_obj_add_style(btn, &ph->ui.card_item_style, 0);
        lv_obj_add_style(btn, &ph->ui.card_item_focus_style, LV_STATE_FOCUSED);
        lv_obj_add_style(btn, &ph->ui.card_item_focus_style, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
        lv_obj_set_size(btn, SLEEP_PICK_ROW_W, SLEEP_PICK_ROW_H);
        lv_obj_set_pos(btn, SLEEP_PICK_ROW_X, y + i * (SLEEP_PICK_ROW_H + SLEEP_PICK_ROW_GAP));
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

        chk = lv_img_create(btn);
        divoom_menu_control_lv_img_set_src(chk, SLEEP_CHECK_ICON);
        lv_obj_set_size(chk, SLEEP_PICK_CHECK_SZ, SLEEP_PICK_CHECK_SZ);
        lv_obj_align(chk, LV_ALIGN_RIGHT_MID, -16, 0);
        lv_obj_add_flag(chk, LV_OBJ_FLAG_HIDDEN);

        if (ph->pick_kind == SLEEP_PICK_KIND_TIMER)
        {
            sleep_fmt_timer(buf, (int)sizeof(buf), s_sleep_timer_presets[i]);
            lab = lv_label_create(btn);
            lv_label_set_text(lab, buf);
        }
        else
        {
            lab = lv_label_create(btn);
            lv_label_set_text(lab, divoom_menu_control_get_disp_str(s_sleep_after_music_keys[i]));
        }
        lv_obj_set_style_text_color(lab, lv_color_hex(DIVOOM_ASTRO_INK), 0);
        lv_obj_set_style_text_font(lab, divoom_menu_control_get_font_info(28, ph->menu_id), 0);
        lv_obj_align(lab, LV_ALIGN_LEFT_MID, SLEEP_PICK_LABEL_L_PAD, 0);
        {
            lv_coord_t trail_w;

            trail_w = DIVOOM_ASTRO_LIST_CHK_TRAIL_ALIGN(16);
            divoom_astrootoo_ui_list_label_prepare(lab,
                divoom_astrootoo_ui_list_label_width(SLEEP_PICK_ROW_W, SLEEP_PICK_LABEL_L_PAD, 16, trail_w));
            divoom_astrootoo_ui_list_row_bind_marquee(btn, lab);
        }

        lv_obj_add_event_cb(btn, sleep_pick_key_cb, LV_EVENT_KEY, ph);
        lv_group_add_obj(lv_group_get_default(), btn);
        divoom_menu_direct_ctrl_update(ph->pick_direct, btn, 0, i);

        ph->pick_row[i] = btn;
        ph->pick_check[i] = chk;
        ph->pick_chevron[i] = NULL;
    }

    ph->pick_focus = initial;
    sleep_pick_refresh_checks(ph);
    sleep_pick_set_focus(ph, initial);
}

static void sleep_pick_enter(Pdivoom_astrootoo_sleep_ctrl ph, int kind, int data)
{
    const char *title = NULL;

    if (!ph || ph->focus_pane == SLEEP_PANE_PICK)
    {
        return;
    }

    sleep_pick_destroy(ph);
    ph->pick_kind = kind;
    ph->pick_data = data;
    ph->pick_return_sub = ph->sub_focus;
    divoom_sleep_aid_ctrl_settings_volume_preview_stop();

    if (kind == SLEEP_PICK_KIND_TIMER)
    {
        title = data ? "SleepAidSetAfterTimer" : "SleepAidSetBeforeTimer";
    }
    else if (kind == SLEEP_PICK_KIND_MUSIC)
    {
        title = "SleepAidSetAfterMusic";
    }

    if (!title)
    {
        return;
    }

    lv_group_remove_all_objs(lv_group_get_default());
    sleep_pick_build(ph, title);
    if (ph->pick_count <= 0)
    {
        sleep_pick_destroy(ph);
        sleep_sub_restore_group(ph);
        sleep_set_sub_focus(ph, ph->pick_return_sub);
        return;
    }

    ph->focus_pane = SLEEP_PANE_PICK;
    sleep_paint_strip(ph);
}

static void sleep_pick_exit(Pdivoom_astrootoo_sleep_ctrl ph)
{
    int i;
    int ret;

    if (!ph || !ph->pick_root)
    {
        return;
    }

    divoom_sleep_aid_ctrl_settings_volume_preview_stop();
    ret = ph->pick_return_sub;
    sleep_pick_destroy(ph);
    sleep_sub_restore_group(ph);
    if (ret < 0 || ret >= ph->sub_count)
    {
        ret = ph->sub_focus;
    }
    sleep_set_sub_focus(ph, ret);
    if (ph->active_tab == SLEEP_TAB_SETUP && ret >= 0 && ret < ph->sub_count
        && ph->sub[ret].kind == SUB_KIND_SET_MUSIC)
    {
        sleep_setup_start_after_music_preview();
    }
    for (i = 0; i < ph->sub_count; i++)
    {
        if (ph->sub[i].kind == SUB_KIND_SET_TIMER || ph->sub[i].kind == SUB_KIND_SET_MUSIC)
        {
            sleep_sub_refresh_row(ph, i);
        }
    }
}

#define SLEEP_REC_BTN_ZOOM_BASE  ((int)((int32_t)LV_IMG_ZOOM_NONE * 36 / 24)) /* 设计 36px */
#define SLEEP_REC_BTN_ZOOM_PULSE ((int)(SLEEP_REC_BTN_ZOOM_BASE * 108 / 100))
#define SLEEP_REC_BTN_PULSE_MS   525

static void sleep_rec_btn_icon_pulse_cb(void *var, int32_t v)
{
    lv_img_set_zoom((lv_obj_t *)var, (uint16_t)v);
}

static void sleep_rec_btn_icon_pulse_stop(lv_obj_t *icon)
{
    if (!icon)
    {
        return;
    }
    lv_anim_del(icon, sleep_rec_btn_icon_pulse_cb);
    lv_img_set_zoom(icon, (uint16_t)SLEEP_REC_BTN_ZOOM_BASE);
}

static void sleep_rec_btn_icon_pulse_sync(lv_obj_t *icon, int live)
{
    lv_anim_t a;

    if (!icon)
    {
        return;
    }
    if (!live)
    {
        sleep_rec_btn_icon_pulse_stop(icon);
        return;
    }
    if (lv_anim_get(icon, sleep_rec_btn_icon_pulse_cb) != NULL)
    {
        return;
    }
    lv_img_set_zoom(icon, (uint16_t)SLEEP_REC_BTN_ZOOM_BASE);
    lv_anim_init(&a);
    lv_anim_set_var(&a, icon);
    lv_anim_set_exec_cb(&a, sleep_rec_btn_icon_pulse_cb);
    lv_anim_set_values(&a, SLEEP_REC_BTN_ZOOM_BASE, SLEEP_REC_BTN_ZOOM_PULSE);
    lv_anim_set_time(&a, SLEEP_REC_BTN_PULSE_MS);
    lv_anim_set_playback_time(&a, SLEEP_REC_BTN_PULSE_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_step);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);
}

static void sleep_rec_btn_icon_set(lv_obj_t *icon, const char *path)
{
    if (!icon)
    {
        return;
    }
    divoom_menu_control_lv_img_set_src(icon, path);
    lv_img_set_zoom(icon, (uint16_t)SLEEP_REC_BTN_ZOOM_BASE);
    lv_obj_center(icon);
}

static void sleep_rec_apply_btn_icon(lv_obj_t *icon, const char *path, int live)
{
    sleep_rec_btn_icon_set(icon, path);
    sleep_rec_btn_icon_pulse_sync(icon, live);
}

static const char *sleep_rec_btn_icon_path(int focused, int recorded, int busy)
{
    if (busy)
    {
        return SLEEP_UI_DIR "sleep_rec_btn_live.png";
    }
    if (focused)
    {
        return SLEEP_UI_DIR "sleep_rec_btn_ready.png";
    }
    if (recorded)
    {
        return SLEEP_UI_DIR "sleep_rec_btn_stop.png";
    }
    return SLEEP_UI_DIR "sleep_rec_btn_idle.png";
}

/* ---------- 录音 UI 刷新（异步 record_ctrl，轮询 record_busy） ---------- */

static void sleep_rec_refresh_ui(Pdivoom_astrootoo_sleep_ctrl ph)
{
    int i;
    int busy;
    int recorded;

    if (!ph || ph->active_tab != SLEEP_TAB_REC)
    {
        return;
    }

    busy = divoom_sleep_aid_ctrl_record_busy() ? 1 : 0;
    recorded = (divoom_sleep_aid_ctrl_record_clip_count() > 0) ? 1 : 0;

    for (i = 0; i < ph->sub_count; i++)
    {
        sleep_sub_row *sr = &ph->sub[i];

        if (sr->kind == SUB_KIND_REC_CLIP)
        {
            sleep_rec_refresh_playback_row(ph, sr, busy);
        }
        else if (sr->kind == SUB_KIND_REC_BTN)
        {
            if (sr->name_label)
            {
                lv_label_set_text(sr->name_label, divoom_menu_control_get_disp_str(
                    sleep_rec_action_hint_key(busy, recorded)));
            }
            if (sr->right_icon)
            {
                if (busy)
                {
                    sleep_rec_apply_btn_icon(sr->right_icon, SLEEP_UI_DIR "sleep_rec_btn_live.png", 1);
                }
                else
                {
                    int row_focus = (ph->focus_pane == SLEEP_PANE_SUB && i == ph->sub_focus) ? 1 : 0;

                    sleep_rec_apply_btn_icon(sr->right_icon,
                        sleep_rec_btn_icon_path(row_focus, recorded, 0), 0);
                }
                lv_obj_align(sr->right_icon, LV_ALIGN_RIGHT_MID, -20, 0);
            }
        }
    }
}

static void sleep_rec_refresh_timer_cb(lv_timer_t *t)
{
    Pdivoom_astrootoo_sleep_ctrl ph;
    uint8 was_busy;
    uint8 busy;
    uint32 enc_sec;
    uint32 now;

    ph = (Pdivoom_astrootoo_sleep_ctrl)lv_timer_get_user_data(t);
    if (!ph || ph->active_tab != SLEEP_TAB_REC)
    {
        return;
    }

    was_busy = ph->rec_ui_busy;
    busy = divoom_sleep_aid_ctrl_record_busy();
    enc_sec = divoom_sleep_aid_ctrl_record_enc_duration_sec();
    now = divoom_system_get_tickms();

    if (busy)
    {
        if (enc_sec > 0u)
        {
            ph->rec_elapsed = (int)enc_sec;
            ph->rec_start_tick_ms = 0;
        }
        else if (ph->rec_start_tick_ms != 0u)
        {
            ph->rec_elapsed = (int)((now - ph->rec_start_tick_ms) / 1000u);
        }
        if (ph->rec_elapsed >= (int)SLEEP_RECORD_MAX_SEC)
        {
            divoom_sleep_aid_ctrl_record_ctrl(0);
            ph->rec_elapsed = (int)SLEEP_RECORD_MAX_SEC;
            ph->rec_start_tick_ms = 0;
            busy = 0;
        }
    }
    else if (was_busy)
    {
        int voice_id;
        uint32 file_id = 0;

        if (enc_sec > 0u)
        {
            ph->rec_elapsed = (int)enc_sec;
        }
        voice_id = divoom_sleep_aid_ctrl_record_clip_id(0);
        if (voice_id >= 0)
        {
            file_id = (uint32)(voice_id - DIVOOM_VOICE_ID_SLEEP_AID_RECORD_BASE);
        }
        if (ph->rec_elapsed > 0 && file_id > 0u)
        {
            ph->rec_clip_sec_cache = (uint32)ph->rec_elapsed;
            ph->rec_clip_file_id_cache = file_id;
        }
        ph->rec_elapsed = 0;
        ph->rec_ui_busy = 0;
        ph->pending_sub_focus = 0;
        /* record_ctrl 为同步 commit，内存 manifest 已更新，勿 invalidate+重建（会误 prune 刚落盘 clip） */
        sleep_rec_refresh_ui(ph);
        sleep_rec_stop_timer(ph);
        return;
    }

    ph->rec_ui_busy = busy;
    sleep_rec_refresh_ui(ph);
}

static void sleep_rec_start_refresh(Pdivoom_astrootoo_sleep_ctrl ph)
{
    if (!ph)
    {
        return;
    }
    sleep_rec_stop_timer(ph);
    ph->rec_ui_busy = divoom_sleep_aid_ctrl_record_busy();
    ph->rec_timer = lv_timer_create(sleep_rec_refresh_timer_cb, SLEEP_REC_TIMER_MS, ph);
    if (ph->rec_timer)
    {
        lv_timer_set_repeat_count(ph->rec_timer, -1);
    }
}

/* 刷新某子页行：滑动条数值 / 开关图标 / 文本数值 */
static void sleep_sub_refresh_row(Pdivoom_astrootoo_sleep_ctrl ph, int idx)
{
    sleep_sub_row *sr;
    divoom_sleep_aid_info *inf;
    char buf[64];

    if (idx < 0 || idx >= ph->sub_count)
    {
        return;
    }
    sr = &ph->sub[idx];
    inf = divoom_sleep_aid_ctrl_get_settings();
    if (!inf)
    {
        return;
    }

    switch (sr->kind)
    {
    case SUB_KIND_SET_BRIGHT:
    {
        if (sr->slider)
        {
            lv_slider_set_value(sr->slider, sr->data ? inf->after_bright : inf->before_bright, LV_ANIM_OFF);
        }
        break;
    }
    case SUB_KIND_SET_VOL:
    {
        if (sr->slider)
        {
            lv_slider_set_value(sr->slider, sr->data ? inf->after_vol : inf->before_vol, LV_ANIM_OFF);
        }
        break;
    }
    case SUB_KIND_DIY_CH:
    {
        int v = (sr->data >= 0 && sr->data < SLEEP_DIY_CH_COUNT)
            ? inf->volume[sr->data] : 0;

        if (sr->slider)
        {
            lv_slider_set_value(sr->slider, v, LV_ANIM_OFF);
        }
        break;
    }
    case SUB_KIND_SET_TIMER:
    {
        if (sr->val_label)
        {
            sleep_fmt_timer(buf, (int)sizeof(buf), sr->data ? inf->after_time : inf->before_time);
            lv_label_set_text(sr->val_label, buf);
        }
        break;
    }
    case SUB_KIND_SET_MUSIC:
    {
        int mi = inf->after_music;

        if (mi < 0 || mi >= SLEEP_AFTER_MUSIC_COUNT)
        {
            mi = 0;
        }
        if (sr->val_label)
        {
            lv_label_set_text(sr->val_label, divoom_menu_control_get_disp_str(s_sleep_after_music_keys[mi]));
        }
        break;
    }
    case SUB_KIND_SET_MASK:
    {
        if (sr->right_icon)
        {
            divoom_menu_control_lv_img_set_src(sr->right_icon,
                inf->after_on_off ? SLEEP_UI_DIR "sleep_switch_on.png"
                : SLEEP_UI_DIR "sleep_switch_off.png");
            lv_obj_align(sr->right_icon, LV_ALIGN_RIGHT_MID, -16, 0);
        }
        break;
    }
    default:
    {
        break;
    }
    }
    if (sr->val_label && sr->btn)
    {
        divoom_astrootoo_ui_list_label_refresh_marquee(sr->btn, sr->val_label);
    }
}

static void sleep_sub_build_direct(Pdivoom_astrootoo_sleep_ctrl ph)
{
    int i;

    if (ph->sub_direct)
    {
        divoom_menu_direct_ctrl_destory(ph->sub_direct);
        ph->sub_direct = NULL;
    }
    if (ph->sub_count <= 0)
    {
        return;
    }
    ph->sub_direct = divoom_menu_direct_ctrl_create(1, ph->sub_count);
    for (i = 0; i < ph->sub_count; i++)
    {
        divoom_menu_direct_ctrl_update(ph->sub_direct, ph->sub[i].btn, 0, i);
    }
}

/* DIY「进入睡眠」头行：左文本 + 右收藏（对照 AstroTooUI localRow） */
static void sleep_build_diy_enter(Pdivoom_astrootoo_sleep_ctrl ph, lv_coord_t *y)
{
    int idx;
    sleep_sub_row *sr;

    idx = sleep_sub_new(ph, y, SLEEP_ROW_H, SUB_KIND_DIY_PREVIEW, 0, 1);
    if (idx < 0)
    {
        return;
    }
    sr = &ph->sub[idx];
    sleep_sub_begin_split_row(ph, sr, SLEEP_ROW_H, SLEEP_DIY_MIX_VOICE_ID,
        DIVOOM_SLEEP_AID_TYPE_DIY, -1);
    sr->name_label = sleep_sub_make_label(ph, sr->text_cont, "SleepAidEnterSleep",
        SLEEP_TEXT_W - 32, SLEEP_DIY_ENTER_INK);
    sleep_sub_bind_pointer(ph, sr);
}

/* DIY 通道行：左 48×48 图标 + 右滑动条（无文字） */
static void sleep_build_diy_channel(Pdivoom_astrootoo_sleep_ctrl ph, lv_coord_t *y, int ch)
{
    int idx;
    sleep_sub_row *sr;
    divoom_sleep_aid_info *inf;
    char src[32];
    int v;

    idx = sleep_sub_new(ph, y, SLEEP_ROW_H, SUB_KIND_DIY_CH, ch, 0);
    if (idx < 0)
    {
        return;
    }
    sr = &ph->sub[idx];

    sr->left_icon = lv_img_create(sr->btn);
    snprintf(src, sizeof(src), SLEEP_UI_DIR "sleep_diy_%d.png", ch + 1);
    divoom_menu_control_lv_img_set_src(sr->left_icon, src);
    lv_obj_align(sr->left_icon, LV_ALIGN_LEFT_MID, 20, 0);

    inf = divoom_sleep_aid_ctrl_get_settings();
    v = (inf && ch >= 0 && ch < SLEEP_DIY_CH_COUNT) ? inf->volume[ch] : 0;
    sr->slider = sleep_make_slider(sr->btn, v);
    lv_obj_set_width(sr->slider, SLEEP_ROW_W - SLEEP_DIY_ICON_SZ - 20 - 12 - 24);
    lv_obj_align(sr->slider, LV_ALIGN_RIGHT_MID, -16, 0);
}

/* DIY 混音子页：进入睡眠头 + 8 通道（图标 + 滑动条） */
static void sleep_build_diy(Pdivoom_astrootoo_sleep_ctrl ph)
{
    lv_coord_t y = 0;
    int i;

    sleep_build_diy_enter(ph, &y);
    sleep_sub_add_section(ph, &y, "SleepAidDiyEffectAdjust");
    for (i = 0; i < SLEEP_DIY_CH_COUNT; i++)
    {
        sleep_build_diy_channel(ph, &y, i);
    }
    sleep_sub_build_direct(ph);
}

/* 录音回放行：左文本+时长 + 右收藏 */
static void sleep_build_rec_playback(Pdivoom_astrootoo_sleep_ctrl ph, lv_coord_t *y, int recorded)
{
    int idx;
    sleep_sub_row *sr;
    int vid;
    uint32 audio_id;

    (void)recorded;
    idx = sleep_sub_new(ph, y, SLEEP_ROW_H, SUB_KIND_REC_CLIP, 0, 1);
    if (idx < 0)
    {
        return;
    }
    sr = &ph->sub[idx];
    audio_id = sleep_rec_fav_voice_id();
    sleep_sub_begin_split_row(ph, sr, SLEEP_ROW_H, audio_id, DIVOOM_SLEEP_AID_TYPE_RECORD, -1);

    sr->name_label = lv_label_create(sr->text_cont);
    lv_obj_set_style_text_color(sr->name_label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(sr->name_label, divoom_menu_control_get_font_info(20, ph->menu_id), 0);
    lv_obj_align(sr->name_label, LV_ALIGN_LEFT_MID, 16, 0);

    sr->val_label = lv_label_create(sr->text_cont);
    lv_obj_set_style_text_color(sr->val_label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(sr->val_label, divoom_menu_control_get_font_info(20, ph->menu_id), 0);
    lv_obj_align(sr->val_label, LV_ALIGN_RIGHT_MID, -8, 0);

    sleep_rec_refresh_playback_row(ph, sr, divoom_sleep_aid_ctrl_record_busy() ? 1 : 0);
    sleep_sub_bind_pointer(ph, sr);
}

/* 录音操作行：大卡片(180px) 提示 + 录音按钮（idle/stop/live） */
static void sleep_build_rec_action(Pdivoom_astrootoo_sleep_ctrl ph, lv_coord_t *y,
    int recorded, int busy)
{
    int idx;
    sleep_sub_row *sr;

    idx = sleep_sub_new(ph, y, SLEEP_REC_ACTION_H, SUB_KIND_REC_BTN, 0, 0);
    if (idx < 0)
    {
        return;
    }
    sr = &ph->sub[idx];

    sr->name_label = lv_label_create(sr->btn);
    lv_label_set_text(sr->name_label, divoom_menu_control_get_disp_str(
        sleep_rec_action_hint_key(busy, recorded)));
    lv_obj_set_style_text_color(sr->name_label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(sr->name_label, divoom_menu_control_get_font_info(20, ph->menu_id), 0);
    lv_obj_align(sr->name_label, LV_ALIGN_LEFT_MID, 20, 0);

    sr->right_icon = lv_img_create(sr->btn);
    sleep_rec_apply_btn_icon(sr->right_icon,
        sleep_rec_btn_icon_path(0, recorded, busy), busy);
    lv_obj_align(sr->right_icon, LV_ALIGN_RIGHT_MID, -20, 0);
}

/* 录音子页：回放行 + 录音卡片 + 底部覆盖提示 */
static void sleep_build_record(Pdivoom_astrootoo_sleep_ctrl ph)
{
    lv_coord_t y = 0;
    int busy;
    int recorded;
    lv_obj_t *foot;

    busy = divoom_sleep_aid_ctrl_record_busy() ? 1 : 0;
    recorded = (divoom_sleep_aid_ctrl_record_clip_count() > 0) ? 1 : 0;
    ph->rec_ui_busy = (uint8)busy;

    sleep_build_rec_playback(ph, &y, recorded);
    sleep_build_rec_action(ph, &y, recorded, busy);

    /* 底部提示：真机勿拼 emoji，FreeType 缺字形会卡死 LVGL（已复现） */
    foot = lv_label_create(ph->content);
    lv_label_set_text(foot,
        divoom_menu_control_get_disp_str("SleepAidRecHintFooter"));
    lv_obj_set_style_text_color(foot, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(foot, divoom_menu_control_get_font_info(16, ph->menu_id), 0);
    lv_label_set_long_mode(foot, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(foot, SLEEP_ROW_W - 40);
    lv_obj_set_style_text_align(foot, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(foot, SLEEP_LIST_PAD_X + 20, y + 16);

    sleep_sub_build_direct(ph);
}

/* 设置子页：睡前 亮度/音量/定时，睡后开关 +（开启时）亮度/音量/音源/定时 */
static void sleep_build_setup(Pdivoom_astrootoo_sleep_ctrl ph)
{
    lv_coord_t y = 0;
    divoom_sleep_aid_info *inf;

    inf = divoom_sleep_aid_ctrl_get_settings();
    if (!inf)
    {
        sleep_show_empty(ph);
        return;
    }

    sleep_sub_add_section(ph, &y, "SleepAidSetBeforeSection");
    sleep_sub_add_slider_row(ph, &y, SUB_KIND_SET_BRIGHT, 0, "SleepAidSetBeforeBrightness", inf->before_bright);
    sleep_sub_add_slider_row(ph, &y, SUB_KIND_SET_VOL, 0, "SleepAidSetBeforeSleepVol", inf->before_vol);
    sleep_sub_add_static_row(ph, &y, SUB_KIND_SET_TIMER, 0, "SleepAidSetBeforeTimer");

    sleep_sub_add_section(ph, &y, "SleepAidSetAfterSection");
    sleep_sub_add_toggle_row(ph, &y, SUB_KIND_SET_MASK, 0, "SleepAidSetAfterMask");
    if (inf->after_on_off)
    {
        sleep_sub_add_slider_row(ph, &y, SUB_KIND_SET_BRIGHT, 1, "SleepAidSetAfterBrightness", inf->after_bright);
        sleep_sub_add_static_row(ph, &y, SUB_KIND_SET_MUSIC, 0, "SleepAidSetAfterMusic");
        sleep_sub_add_slider_row(ph, &y, SUB_KIND_SET_VOL, 1, "SleepAidSetAfterSleepVol", inf->after_vol);
        sleep_sub_add_static_row(ph, &y, SUB_KIND_SET_TIMER, 1, "SleepAidSetAfterTimer");
    }

    /* 填充各行数值/开关 */
    {
        int i;

        for (i = 0; i < ph->sub_count; i++)
        {
            sleep_sub_refresh_row(ph, i);
        }
    }
    sleep_sub_build_direct(ph);
}

/* 子页子项为绝对定位，需底部占位撑开 scroll 区域，scroll_to_view 才能生效 */
static void sleep_content_update_sub_scroll_extent(Pdivoom_astrootoo_sleep_ctrl ph)
{
    lv_obj_t *spacer;
    uint32_t i;
    uint32_t n;
    lv_coord_t max_bottom = 0;
    lv_coord_t stop;
    lv_coord_t view_h;

    if (!ph || !ph->content)
    {
        return;
    }

    lv_obj_scroll_to_y(ph->content, 0, LV_ANIM_OFF);
    lv_obj_update_layout(ph->content);

    stop = lv_obj_get_style_space_top(ph->content, LV_PART_MAIN);
    view_h = lv_obj_get_height(ph->content) - stop
        - lv_obj_get_style_space_bottom(ph->content, LV_PART_MAIN);

    n = lv_obj_get_child_cnt(ph->content);
    for (i = 0; i < n; i++)
    {
        lv_obj_t *ch = lv_obj_get_child(ph->content, i);
        lv_coord_t bottom;

        if (!ch || lv_obj_has_flag(ch, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_FLOATING))
        {
            continue;
        }
        bottom = lv_obj_get_y(ch) + lv_obj_get_height(ch);
        if (bottom > max_bottom)
        {
            max_bottom = bottom;
        }
    }

    if (max_bottom <= view_h)
    {
        return;
    }

    spacer = lv_obj_create(ph->content);
    lv_obj_set_size(spacer, 1, SLEEP_SUB_SCROLL_PAD);
    lv_obj_set_pos(spacer, 0, max_bottom);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_clear_flag(spacer, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_update_layout(ph->content);
}

/* 将子页焦点行滚入可视区（绝对定位子项需先撑开 scroll 区域） */
static void sleep_content_scroll_sub_into_view(Pdivoom_astrootoo_sleep_ctrl ph, int idx)
{
    sleep_sub_row *sr;
    lv_coord_t stop;
    lv_coord_t sbottom;
    lv_coord_t view_h;
    lv_coord_t scroll_y;
    lv_coord_t row_top;
    lv_coord_t row_bottom;
    lv_coord_t vis_top;
    lv_coord_t vis_bottom;
    lv_coord_t target;
    lv_coord_t max_scroll;

    if (!ph || !ph->content || idx < 0 || idx >= ph->sub_count)
    {
        return;
    }
    sr = &ph->sub[idx];
    if (!sr->btn)
    {
        return;
    }

    lv_obj_update_layout(ph->content);
    stop = lv_obj_get_style_space_top(ph->content, LV_PART_MAIN);
    sbottom = lv_obj_get_style_space_bottom(ph->content, LV_PART_MAIN);
    view_h = lv_obj_get_height(ph->content) - stop - sbottom;
    scroll_y = lv_obj_get_scroll_y(ph->content);

    row_top = lv_obj_get_y(sr->btn);
    row_bottom = row_top + lv_obj_get_height(sr->btn);
    vis_top = scroll_y;
    vis_bottom = scroll_y + view_h;

    {
        lv_obj_t *title = NULL;
        uint32_t ci;
        uint32_t cn;
        lv_coord_t lead_top;

        cn = lv_obj_get_child_cnt(ph->content);
        for (ci = 0; ci < cn; ci++)
        {
            lv_obj_t *ch;

            ch = lv_obj_get_child(ph->content, (int32_t)ci);
            if (!ch)
            {
                continue;
            }
            if (ch == sr->btn)
            {
                break;
            }
            if (lv_obj_check_type(ch, &lv_btn_class))
            {
                title = NULL;
            }
            else if (!lv_obj_has_flag(ch, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_FLOATING))
            {
                title = ch;
            }
        }
        lead_top = row_top;
        if (title)
        {
            lead_top = (idx == 0) ? 0 : lv_obj_get_y(title);
        }
        if (lead_top >= vis_top && row_bottom <= vis_bottom)
        {
            return;
        }
        if (lead_top < vis_top)
        {
            target = lead_top;
        }
        else
        {
            target = row_bottom - view_h;
        }
    }

    max_scroll = scroll_y + lv_obj_get_scroll_bottom(ph->content);
    if (target > max_scroll)
    {
        target = max_scroll;
    }
    if (target < 0)
    {
        target = 0;
    }
    lv_obj_scroll_to_y(ph->content, target, LV_ANIM_ON);

    /* 末行仍被底部裁切时，滚到可滚动尽头 */
    if (idx == ph->sub_count - 1)
    {
        lv_obj_update_layout(ph->content);
        scroll_y = lv_obj_get_scroll_y(ph->content);
        row_bottom = lv_obj_get_y(sr->btn) + lv_obj_get_height(sr->btn);
        if (row_bottom > scroll_y + view_h)
        {
            max_scroll = scroll_y + lv_obj_get_scroll_bottom(ph->content);
            lv_obj_scroll_to_y(ph->content, max_scroll, LV_ANIM_ON);
        }
    }
}

static void sleep_show_empty(Pdivoom_astrootoo_sleep_ctrl ph)
{
    lv_obj_t *lab;

    lab = lv_label_create(ph->content);
    lv_label_set_text(lab, divoom_menu_control_get_disp_str("No items"));
    lv_obj_set_style_text_color(lab, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(lab, divoom_menu_control_get_font_info(20, ph->menu_id), 0);
    lv_obj_align(lab, LV_ALIGN_TOP_MID, 0, 24);
}

static void sleep_build_content(Pdivoom_astrootoo_sleep_ctrl ph)
{
    sleep_free_list(ph);

    if (ph->active_tab == SLEEP_TAB_LOCAL)
    {
        sleep_build_favorites(ph);
    }
    else if (sleep_tab_is_category(ph->active_tab))
    {
        sleep_build_category(ph);
    }
    else if (ph->active_tab == SLEEP_TAB_DIY)
    {
        sleep_build_diy(ph);
    }
    else if (ph->active_tab == SLEEP_TAB_REC)
    {
        sleep_build_record(ph);
    }
    else
    {
        sleep_build_setup(ph);
    }

    /* 子页：重建后恢复焦点（如开关切换导致行数变化） */
    if (!sleep_tab_is_list(ph->active_tab))
    {
        int i;

        /* 子页内容常高于视口（DIY/设置）：允许滚动但隐藏滚动条，焦点移动时滚入视口 */
        lv_obj_set_scrollbar_mode(ph->content, LV_SCROLLBAR_MODE_OFF);
        lv_obj_add_flag(ph->content, LV_OBJ_FLAG_SCROLLABLE);
        sleep_content_update_sub_scroll_extent(ph);
        for (i = 0; i < ph->sub_count; i++)
        {
            sleep_paint_sub(ph, i);
        }
        if (ph->sub_count <= 0)
        {
            sleep_show_empty(ph);
            return;
        }
        if (ph->focus_pane == SLEEP_PANE_SUB)
        {
            int f = (ph->pending_sub_focus >= 0) ? ph->pending_sub_focus : ph->sub_focus;

            if (f >= ph->sub_count)
            {
                f = ph->sub_count - 1;
            }
            sleep_set_sub_focus(ph, f);
        }
        ph->pending_sub_focus = -1;
        return;
    }

    if (ph->row_count <= 0)
    {
        sleep_show_empty(ph);
        return;
    }

    /* 进入列表前默认焦点在第一行（仅当焦点已在 list 区时立即上焦点） */
    if (ph->focus_pane == SLEEP_PANE_LIST)
    {
        sleep_set_list_focus(ph, 0, SLEEP_COL_TEXT);
    }

    lv_obj_set_scrollbar_mode(ph->content, LV_SCROLLBAR_MODE_OFF);
    if (sleep_tab_is_list(ph->active_tab))
    {
        lv_obj_add_flag(ph->content, LV_OBJ_FLAG_SCROLLABLE);
    }
    else
    {
        lv_obj_clear_flag(ph->content, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_scroll_to_y(ph->content, 0, LV_ANIM_OFF);
    }
}

/* ---------- 分段追加 ---------- */

static void sleep_append_cb(lv_timer_t *t)
{
    Pdivoom_astrootoo_sleep_ctrl ph;
    int aid_type;
    divoom_sleep_aid_voice_list_item *page;
    int n = 0;
    int total = 0;
    int i;
    int base;

    ph = (Pdivoom_astrootoo_sleep_ctrl)lv_timer_get_user_data(t);
    if (ph)
    {
        ph->append_timer = NULL;
    }
    lv_timer_del(t);
    if (!ph || !sleep_tab_is_category(ph->active_tab))
    {
        return;
    }
    if (ph->page_loaded >= ph->total_count || !ph->rows)
    {
        return;
    }

    aid_type = sleep_tab_aid_type(ph->active_tab);
    page = divoom_sleep_aid_voice_get_list_page(aid_type, ph->page_loaded, SLEEP_PAGE_BATCH, -1, &n, &total);
    if (!page || n <= 0)
    {
        if (page)
        {
            divoom_system_free(page);
        }
        return;
    }

    base = ph->page_loaded;
    for (i = 0; i < n && (base + i) < ph->total_count; i++)
    {
        sleep_create_row(ph, base + i, &page[i],
            sleep_voice_is_favored(page[i].audio_id, page[i].audio_type));
    }
    divoom_system_free(page);

    ph->row_count = base + n;
    ph->page_loaded = ph->row_count;

    /* 重建方向控制以纳入新行 */
    if (ph->list_direct)
    {
        divoom_menu_direct_ctrl_destory(ph->list_direct);
    }
    ph->list_direct = divoom_menu_direct_ctrl_create(1, ph->row_count);
    for (i = 0; i < ph->row_count; i++)
    {
        divoom_menu_direct_ctrl_update(ph->list_direct, ph->rows[i].btn, 0, i);
    }
}

static void sleep_request_append(Pdivoom_astrootoo_sleep_ctrl ph)
{
    if (!ph || ph->append_timer)
    {
        return;
    }
    if (!sleep_tab_is_category(ph->active_tab) || ph->page_loaded >= ph->total_count)
    {
        return;
    }
    ph->append_timer = lv_timer_create(sleep_append_cb, 0, ph);
    if (ph->append_timer)
    {
        lv_timer_set_repeat_count(ph->append_timer, 1);
    }
}

/* ---------- 焦点绘制 ---------- */

static void sleep_paint_strip(Pdivoom_astrootoo_sleep_ctrl ph)
{
    int i;
    int active;
    char src[40];

    for (i = 0; i < SLEEP_TAB_COUNT; i++)
    {
        /* 仅顶栏获焦时当前 TAB 用 focus 图；进入列表/子页后全部 idle */
        active = (i == ph->active_tab && ph->focus_pane == SLEEP_PANE_STRIP);
        /* 图标自带选中底：激活 TAB 用 focus，其余 idle（与真源像素一致） */
        snprintf(src, sizeof(src), SLEEP_UI_DIR "sleep_tab_%s_%s.png",
            sleep_tab_icon_key(i), active ? "focus" : "idle");
        divoom_menu_control_lv_img_set_src(ph->tab_icon[i], src);
        lv_obj_center(ph->tab_icon[i]);
    }
}

static void sleep_paint_row(Pdivoom_astrootoo_sleep_ctrl ph, int row)
{
    Psleep_row r;
    int text_focus;
    int action_focus;

    if (row < 0 || row >= ph->row_count)
    {
        return;
    }
    r = &ph->rows[row];
    text_focus = (ph->focus_pane == SLEEP_PANE_LIST && row == ph->list_focus
        && ph->col_focus == SLEEP_COL_TEXT);
    action_focus = (ph->focus_pane == SLEEP_PANE_LIST && row == ph->list_focus
        && ph->col_focus == SLEEP_COL_ACTION && sleep_row_fav_action_enabled(r));
    sleep_apply_slot(r->text_cont, text_focus);
    if (r->action_cont)
    {
        if (sleep_row_fav_action_visible(r) && !sleep_row_fav_action_enabled(r))
        {
            sleep_apply_slot_disabled(r->action_cont);
        }
        else
        {
            sleep_apply_slot(r->action_cont, action_focus);
        }
        sleep_row_refresh_fav_action(r);
    }
}

/* 焦点切换时试听；回顶栏时停止；ENTER 时进入正式播放（对照 divoom_lvgl_sleep_aid.c） */
static void sleep_stop_voice_preview(void)
{
    divoom_sleep_aid_ctrl_stop_play_voice();
}

static int sleep_sub_is_volume_row(const sleep_sub_row *sr)
{
    if (!sr || sr->kind != SUB_KIND_SET_VOL)
    {
        return 0;
    }
    return 1;
}

static void sleep_setup_stop_volume_preview(void)
{
    divoom_sleep_aid_ctrl_settings_volume_preview_stop();
}

static void sleep_setup_start_volume_preview(const sleep_sub_row *sr)
{
    divoom_sleep_aid_info *inf;

    if (!sleep_sub_is_volume_row(sr))
    {
        return;
    }
    inf = divoom_sleep_aid_ctrl_get_settings();
    if (!inf)
    {
        return;
    }
    if (sr->data == 0)
    {
        divoom_sleep_aid_ctrl_settings_volume_preview_start(inf->before_vol, 0);
    }
    else
    {
        divoom_sleep_aid_ctrl_settings_volume_preview_start(inf->after_vol, 1);
    }
}

static int sleep_setup_row_needs_before_vol_preview(const sleep_sub_row *sr)
{
    if (!sleep_sub_is_volume_row(sr))
    {
        return 0;
    }
    if (sr->data == 0)
    {
        return 1;
    }
    return 0;
}

static int sleep_setup_row_needs_after_music_preview(const sleep_sub_row *sr)
{
    if (!sr)
    {
        return 0;
    }
    if (sr->kind == SUB_KIND_SET_MUSIC)
    {
        return 1;
    }
    if (sleep_sub_is_volume_row(sr) && sr->data == 1)
    {
        return 1;
    }
    return 0;
}

static int sleep_setup_row_needs_preview(const sleep_sub_row *sr)
{
    if (sleep_setup_row_needs_before_vol_preview(sr))
    {
        return 1;
    }
    if (sleep_setup_row_needs_after_music_preview(sr))
    {
        return 1;
    }
    return 0;
}

static int sleep_setup_same_preview(const sleep_sub_row *old_sr, const sleep_sub_row *new_sr)
{
    if (!old_sr || !new_sr)
    {
        return 0;
    }
    if (sleep_setup_row_needs_before_vol_preview(old_sr)
        && sleep_setup_row_needs_before_vol_preview(new_sr))
    {
        return 1;
    }
    if (sleep_setup_row_needs_after_music_preview(old_sr)
        && sleep_setup_row_needs_after_music_preview(new_sr))
    {
        return 1;
    }
    return 0;
}

static void sleep_setup_start_after_music_preview(void)
{
    divoom_sleep_aid_info *inf;

    inf = divoom_sleep_aid_ctrl_get_settings();
    if (!inf)
    {
        return;
    }
    divoom_sleep_aid_ctrl_settings_after_music_preview(inf->after_music, inf->after_vol);
}

static void sleep_setup_start_preview_for_row(const sleep_sub_row *sr)
{
    if (sleep_setup_row_needs_before_vol_preview(sr))
    {
        sleep_setup_start_volume_preview(sr);
    }
    else if (sleep_setup_row_needs_after_music_preview(sr))
    {
        sleep_setup_start_after_music_preview();
    }
}

static void sleep_setup_on_sub_focus_change(Pdivoom_astrootoo_sleep_ctrl ph, int old_idx, int new_idx)
{
    sleep_sub_row *old_sr = NULL;
    sleep_sub_row *new_sr = NULL;

    if (!ph || ph->active_tab != SLEEP_TAB_SETUP)
    {
        return;
    }
    if (old_idx >= 0 && old_idx < ph->sub_count)
    {
        old_sr = &ph->sub[old_idx];
    }
    if (new_idx >= 0 && new_idx < ph->sub_count)
    {
        new_sr = &ph->sub[new_idx];
    }

    if (old_sr && sleep_setup_row_needs_preview(old_sr)
        && (!new_sr || !sleep_setup_same_preview(old_sr, new_sr)))
    {
        sleep_setup_stop_volume_preview();
    }
    if (new_sr && sleep_setup_row_needs_preview(new_sr)
        && (!old_sr || !sleep_setup_same_preview(old_sr, new_sr)))
    {
        sleep_setup_start_preview_for_row(new_sr);
    }
}

static void sleep_apply_list_voice(Pdivoom_astrootoo_sleep_ctrl ph)
{
    Psleep_row r;

    if (!ph || ph->focus_pane != SLEEP_PANE_LIST || !sleep_tab_is_list(ph->active_tab))
    {
        return;
    }
    if (ph->list_focus < 0 || ph->list_focus >= ph->row_count)
    {
        return;
    }
    r = &ph->rows[ph->list_focus];
    divoom_sleep_aid_ctrl_select_voice(r->audio_type, (int)r->voice_id);
}

static void sleep_apply_sub_voice(Pdivoom_astrootoo_sleep_ctrl ph)
{
    sleep_sub_row *sr;
    int vid;

    if (!ph || ph->focus_pane != SLEEP_PANE_SUB || ph->sub_count <= 0)
    {
        return;
    }
    if (ph->active_tab == SLEEP_TAB_REC && divoom_sleep_aid_ctrl_record_busy())
    {
        return;
    }
    sr = &ph->sub[ph->sub_focus];
    if (sr->kind == SUB_KIND_REC_BTN)
    {
        sleep_stop_voice_preview();
        return;
    }
    if (sr->kind == SUB_KIND_DIY_PREVIEW || sr->kind == SUB_KIND_DIY_CH)
    {
        divoom_sleep_aid_ctrl_select_voice(DIVOOM_SLEEP_AID_TYPE_DIY, SLEEP_DIY_MIX_VOICE_ID);
        return;
    }
    if (sr->kind == SUB_KIND_REC_CLIP)
    {
        vid = divoom_sleep_aid_ctrl_record_clip_id(sr->data);
        if (vid >= 0)
        {
            divoom_sleep_aid_ctrl_select_voice(DIVOOM_SLEEP_AID_TYPE_RECORD, vid);
        }
        return;
    }
    sleep_stop_voice_preview();
}

static void sleep_enter_play_from_list_row(Psleep_row r)
{
    if (!r)
    {
        return;
    }
    divoom_sleep_aid_ctrl_enter_play_voice(r->audio_type, (int)r->voice_id);
}

static void sleep_enter_play_from_sub_row(const sleep_sub_row *sr)
{
    int vid;

    if (!sr)
    {
        return;
    }
    if (sr->kind == SUB_KIND_DIY_PREVIEW || sr->kind == SUB_KIND_DIY_CH)
    {
        divoom_sleep_aid_ctrl_enter_play_voice(DIVOOM_SLEEP_AID_TYPE_DIY, SLEEP_DIY_MIX_VOICE_ID);
        return;
    }
    if (sr->kind == SUB_KIND_REC_CLIP)
    {
        vid = divoom_sleep_aid_ctrl_record_clip_id(sr->data);
        if (vid >= 0)
        {
            divoom_sleep_aid_ctrl_enter_play_voice(DIVOOM_SLEEP_AID_TYPE_RECORD, vid);
        }
    }
}

static void sleep_set_list_focus(Pdivoom_astrootoo_sleep_ctrl ph, int row, int col)
{
    int old_row;

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

    old_row = ph->list_focus;
    ph->list_focus = row;
    ph->col_focus = col;
    if (!sleep_row_fav_action_enabled(&ph->rows[row]))
    {
        ph->col_focus = SLEEP_COL_TEXT;
    }
    ph->focus_pane = SLEEP_PANE_LIST;

    if (old_row != row)
    {
        sleep_paint_row(ph, old_row);
    }
    sleep_paint_row(ph, row);
    sleep_paint_strip(ph);

    if (ph->rows[row].btn)
    {
        lv_group_focus_obj(ph->rows[row].btn);
        lv_obj_scroll_to_view(ph->rows[row].btn, LV_ANIM_ON);
    }

    /* 接近末尾时分段加载下一批 */
    if (sleep_tab_is_category(ph->active_tab)
        && row >= ph->page_loaded - 2 && ph->page_loaded < ph->total_count)
    {
        sleep_request_append(ph);
    }

    sleep_apply_list_voice(ph);
}

static void sleep_paint_sub(Pdivoom_astrootoo_sleep_ctrl ph, int idx)
{
    sleep_sub_row *sr;
    int focused;
    int text_focus;
    int action_focus;

    if (idx < 0 || idx >= ph->sub_count || !ph->sub[idx].btn)
    {
        return;
    }
    sr = &ph->sub[idx];
    focused = (ph->focus_pane == SLEEP_PANE_SUB && idx == ph->sub_focus) ? 1 : 0;

    if (sleep_sub_row_has_split(sr))
    {
        text_focus = focused && ph->sub_col_focus == SLEEP_COL_TEXT;
        action_focus = focused && ph->sub_col_focus == SLEEP_COL_ACTION
            && sleep_sub_fav_action_enabled(sr);
        sleep_apply_slot(sr->text_cont, text_focus);
        if (sleep_sub_fav_action_visible(sr) && !sleep_sub_fav_action_enabled(sr))
        {
            sleep_apply_slot_disabled(sr->action_cont);
        }
        else
        {
            sleep_apply_slot(sr->action_cont, action_focus);
        }
        sleep_sub_refresh_fav_action(sr);
        return;
    }

    sleep_apply_slot(sr->btn, focused);

    if (sr->kind == SUB_KIND_REC_BTN && sr->right_icon
        && !divoom_sleep_aid_ctrl_record_busy())
    {
        int recorded = (divoom_sleep_aid_ctrl_record_clip_count() > 0) ? 1 : 0;
        int row_focus = (ph->focus_pane == SLEEP_PANE_SUB && idx == ph->sub_focus) ? 1 : 0;

        sleep_rec_apply_btn_icon(sr->right_icon,
            sleep_rec_btn_icon_path(row_focus, recorded, 0), 0);
        lv_obj_align(sr->right_icon, LV_ALIGN_RIGHT_MID, -20, 0);
    }
}

static void sleep_set_sub_focus(Pdivoom_astrootoo_sleep_ctrl ph, int idx)
{
    int old;

    if (ph->sub_count <= 0)
    {
        return;
    }
    if (idx < 0)
    {
        idx = 0;
    }
    if (idx >= ph->sub_count)
    {
        idx = ph->sub_count - 1;
    }

    old = ph->sub_focus;
    ph->sub_focus = idx;
    ph->focus_pane = SLEEP_PANE_SUB;
    if (!sleep_sub_row_has_split(&ph->sub[idx]))
    {
        ph->sub_col_focus = SLEEP_COL_TEXT;
    }
    else if (!sleep_sub_fav_action_enabled(&ph->sub[idx]))
    {
        ph->sub_col_focus = SLEEP_COL_TEXT;
    }

    if (old != idx)
    {
        sleep_paint_sub(ph, old);
    }
    sleep_paint_sub(ph, idx);
    sleep_paint_strip(ph);

    if (ph->sub[idx].btn)
    {
        lv_group_focus_obj(ph->sub[idx].btn);
        sleep_content_scroll_sub_into_view(ph, idx);
    }

    if (ph->active_tab == SLEEP_TAB_SETUP)
    {
        if (old != idx)
        {
            sleep_setup_on_sub_focus_change(ph, old, idx);
        }
    }
    else
    {
        sleep_apply_sub_voice(ph);
    }
}

static void sleep_focus_strip(Pdivoom_astrootoo_sleep_ctrl ph)
{
    int old_pane = ph->focus_pane;

    if (old_pane == SLEEP_PANE_LIST || old_pane == SLEEP_PANE_SUB)
    {
        sleep_stop_voice_preview();
    }

    ph->focus_pane = SLEEP_PANE_STRIP;
    if (ph->row_count > 0)
    {
        sleep_paint_row(ph, ph->list_focus);
    }
    if (old_pane == SLEEP_PANE_SUB && ph->sub_count > 0)
    {
        sleep_paint_sub(ph, ph->sub_focus);
    }
    sleep_paint_strip(ph);
    sleep_strip_restore_group(ph);
    if (ph->tab_btn[ph->active_tab])
    {
        lv_group_focus_obj(ph->tab_btn[ph->active_tab]);
    }
}

static void sleep_refocus_strip_now(lv_timer_t *t)
{
    Pdivoom_astrootoo_sleep_ctrl ph;

    ph = (Pdivoom_astrootoo_sleep_ctrl)lv_timer_get_user_data(t);
    if (ph)
    {
        ph->refocus_timer = NULL;
    }
    lv_timer_del(t);
    if (!ph)
    {
        return;
    }
    if (divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_ID_SLEEP_AID)
    {
        return;
    }
    sleep_focus_strip(ph);
}

static void sleep_request_refocus_strip(Pdivoom_astrootoo_sleep_ctrl ph)
{
    if (!ph)
    {
        return;
    }
    if (ph->refocus_timer)
    {
        lv_timer_reset(ph->refocus_timer);
        return;
    }
    ph->refocus_timer = lv_timer_create(sleep_refocus_strip_now, 0, ph);
    if (ph->refocus_timer)
    {
        lv_timer_set_repeat_count(ph->refocus_timer, 1);
    }
}

/* 顶栏下/确认进入内容：列表 -> 首行；子页 -> 首个可聚焦行 */
static void sleep_enter_content(Pdivoom_astrootoo_sleep_ctrl ph)
{
    if (sleep_tab_is_list(ph->active_tab))
    {
        if (ph->row_count > 0)
        {
            sleep_set_list_focus(ph, 0, SLEEP_COL_TEXT);
        }
    }
    else
    {
        if (ph->sub_count > 0)
        {
            ph->sub_col_focus = SLEEP_COL_TEXT;
            sleep_set_sub_focus(ph, 0);
        }
    }
}

static void sleep_rec_stop_timer(Pdivoom_astrootoo_sleep_ctrl ph)
{
    int i;

    if (ph && ph->rec_timer)
    {
        lv_timer_del(ph->rec_timer);
        ph->rec_timer = NULL;
    }
    if (!ph)
    {
        return;
    }
    for (i = 0; i < ph->sub_count; i++)
    {
        if (ph->sub[i].kind == SUB_KIND_REC_BTN && ph->sub[i].right_icon)
        {
            sleep_rec_btn_icon_pulse_stop(ph->sub[i].right_icon);
        }
    }
}

/* ---------- 延后构建调度 ---------- */

static void sleep_build_cb(lv_timer_t *t)
{
    Pdivoom_astrootoo_sleep_ctrl ph;

    ph = (Pdivoom_astrootoo_sleep_ctrl)lv_timer_get_user_data(t);
    if (ph)
    {
        ph->build_timer = NULL;
    }
    lv_timer_del(t);
    if (!ph)
    {
        return;
    }
    sleep_build_content(ph);
}

static void sleep_request_build(Pdivoom_astrootoo_sleep_ctrl ph)
{
    if (!ph)
    {
        return;
    }
    if (ph->build_timer)
    {
        lv_timer_reset(ph->build_timer);
        return;
    }
    ph->build_timer = lv_timer_create(sleep_build_cb, 0, ph);
    if (ph->build_timer)
    {
        lv_timer_set_repeat_count(ph->build_timer, 1);
    }
}

/* ---------- 收藏切换（延后执行，避免 key 回调内重操作） ---------- */

static void sleep_toggle_favorite(Pdivoom_astrootoo_sleep_ctrl ph, int aid_type, uint32 voice_id,
                                  int video_type)
{
    int row;
    int i;
    int favored;
    int rc;
    sleep_sub_row *sr;

    if (!ph || voice_id == 0)
    {
        return;
    }
    if (video_type < 0)
    {
        video_type = divoom_sleep_aid_voice_get_video_type(aid_type, voice_id);
    }

    favored = sleep_voice_is_favored(voice_id, aid_type);
    if (favored && sleep_fav_unfav_blocked(1))
    {
        return;
    }
    if (favored)
    {
        rc = divoom_sleep_aid_fav_remove(voice_id, aid_type);
    }
    else
    {
        rc = divoom_sleep_aid_fav_add(voice_id, aid_type, video_type);
    }
    if (rc != 0)
    {
        return;
    }

    favored = favored ? 0 : 1;

    if (ph->active_tab == SLEEP_TAB_LOCAL)
    {
        int focus = ph->list_focus;

        sleep_request_build(ph);
        if (!favored && focus > 0)
        {
            ph->list_focus = focus - 1;
        }
        return;
    }

    row = sleep_row_find_by_voice(ph, voice_id, aid_type);
    if (row >= 0)
    {
        ph->rows[row].favored = favored ? 1 : 0;
        sleep_row_refresh_fav_action(&ph->rows[row]);
    }

    for (i = 0; i < ph->row_count; i++)
    {
        if (ph->rows[i].favored)
        {
            sleep_row_refresh_fav_action(&ph->rows[i]);
        }
    }

    for (i = 0; i < ph->sub_count; i++)
    {
        sr = &ph->sub[i];
        if (sr->voice_id == voice_id && sr->audio_type == aid_type)
        {
            sr->favored = favored ? 1 : 0;
        }
        if (sleep_sub_row_has_split(sr))
        {
            sleep_sub_refresh_fav_action(sr);
        }
    }

    for (i = 0; i < ph->row_count; i++)
    {
        if (ph->rows[i].favored || sleep_row_fav_action_visible(&ph->rows[i]))
        {
            sleep_paint_row(ph, i);
        }
    }
    for (i = 0; i < ph->sub_count; i++)
    {
        if (sleep_sub_row_has_split(&ph->sub[i]))
        {
            sleep_paint_sub(ph, i);
        }
    }
    if (ph->focus_pane == SLEEP_PANE_LIST
        && ph->col_focus == SLEEP_COL_ACTION
        && !sleep_row_fav_action_enabled(&ph->rows[ph->list_focus]))
    {
        ph->col_focus = SLEEP_COL_TEXT;
        sleep_paint_row(ph, ph->list_focus);
    }
}

static void sleep_request_fav_toggle(Pdivoom_astrootoo_sleep_ctrl ph, int aid_type, uint32 voice_id,
                                   int video_type)
{
    if (!ph || voice_id == 0)
    {
        return;
    }
    sleep_toggle_favorite(ph, aid_type, voice_id, video_type);
}

static void sleep_list_click_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_sleep_ctrl ph;
    lv_obj_t *target;
    int row;
    int on_action;
    Psleep_row r;
    int vt;

    if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    {
        return;
    }
    ph = (Pdivoom_astrootoo_sleep_ctrl)lv_event_get_user_data(e);
    if (!ph || !sleep_tab_is_list(ph->active_tab) || ph->row_count <= 0)
    {
        return;
    }

    target = lv_event_get_target(e);
    row = sleep_row_index_from_target(ph, target, &on_action);
    if (row < 0)
    {
        return;
    }

    sleep_set_list_focus(ph, row,
        (on_action && sleep_row_fav_action_enabled(&ph->rows[row]))
        ? SLEEP_COL_ACTION : SLEEP_COL_TEXT);
    r = &ph->rows[row];
    if (ph->col_focus == SLEEP_COL_ACTION && sleep_row_fav_action_enabled(r))
    {
        vt = r->video_type;
        if (vt < 0)
        {
            vt = divoom_sleep_aid_voice_get_video_type(r->audio_type, r->voice_id);
        }
        sleep_request_fav_toggle(ph, r->audio_type, r->voice_id, vt);
    }
}

static void sleep_sub_click_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_sleep_ctrl ph;
    lv_obj_t *target;
    int idx;
    int on_action;
    sleep_sub_row *sr;
    int vt;

    if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    {
        return;
    }
    ph = (Pdivoom_astrootoo_sleep_ctrl)lv_event_get_user_data(e);
    if (!ph || sleep_tab_is_list(ph->active_tab) || ph->sub_count <= 0)
    {
        return;
    }

    target = lv_event_get_target(e);
    idx = sleep_sub_index_from_target(ph, target, &on_action);
    if (idx < 0)
    {
        return;
    }

    sr = &ph->sub[idx];
    if (sleep_sub_row_has_split(sr))
    {
        if (on_action && sleep_sub_fav_action_enabled(sr))
        {
            ph->sub_col_focus = SLEEP_COL_ACTION;
        }
        else
        {
            ph->sub_col_focus = SLEEP_COL_TEXT;
        }
        sleep_set_sub_focus(ph, idx);
        if (ph->sub_col_focus == SLEEP_COL_ACTION && sr->voice_id != 0
            && sleep_sub_fav_action_enabled(sr))
        {
            vt = sr->video_type;
            if (vt < 0)
            {
                vt = divoom_sleep_aid_voice_get_video_type(sr->audio_type, sr->voice_id);
            }
            sleep_request_fav_toggle(ph, sr->audio_type, sr->voice_id, vt);
        }
        return;
    }

    sleep_set_sub_focus(ph, idx);
}

/* ---------- 按键 ---------- */

static void sleep_strip_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_sleep_ctrl ph;
    uint8_t key_code;

    ph = (Pdivoom_astrootoo_sleep_ctrl)lv_event_get_user_data(e);
    if (!ph || lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    if (ph->focus_pane != SLEEP_PANE_STRIP)
    {
        return;
    }

    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        /* 播放页返回后本页是清栈后的根页面，ESC 明确返回主菜单。 */
        divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_MAIN, NULL, 0, 0);
        return;
    }

    /* 顶栏：左/右在 TAB 间切换（不使用 is_prev/is_next，避免上下被当作左右） */
    if (divoom_astrootoo_ui_key_is_left(key_code))
    {
        sleep_stop_voice_preview();
        ph->active_tab = (ph->active_tab + SLEEP_TAB_COUNT - 1) % SLEEP_TAB_COUNT;
        sleep_paint_strip(ph);
        lv_group_focus_obj(ph->tab_btn[ph->active_tab]);
        sleep_request_build(ph);
        return;
    }

    if (divoom_astrootoo_ui_key_is_right(key_code))
    {
        sleep_stop_voice_preview();
        ph->active_tab = (ph->active_tab + 1) % SLEEP_TAB_COUNT;
        sleep_paint_strip(ph);
        lv_group_focus_obj(ph->tab_btn[ph->active_tab]);
        sleep_request_build(ph);
        return;
    }

    /* 顶栏：下 / 确认 进入当前 TAB 子页面 */
    if (divoom_astrootoo_ui_key_is_down(key_code) || divoom_astrootoo_ui_key_is_enter(key_code))
    {
        sleep_enter_content(ph);
        return;
    }

}

static void sleep_list_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_sleep_ctrl ph;
    uint8_t key_code;
    Psleep_row r;

    ph = (Pdivoom_astrootoo_sleep_ctrl)lv_event_get_user_data(e);
    if (!ph || lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    if (ph->row_count <= 0)
    {
        return;
    }
    if (ph->focus_pane != SLEEP_PANE_LIST)
    {
        /* 107 拆页后 group 可能落在列表末行，画面仍画着顶栏。
         * 按键按顶栏处理，避免 ESC 被当成「回顶栏」、左右落到末行。 */
        sleep_focus_strip(ph);
        sleep_strip_key_cb(e);
        return;
    }

    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        sleep_focus_strip(ph);
        return;
    }

    /* 左右：切换 文本/操作 列 */
    if (divoom_astrootoo_ui_key_is_left(key_code))
    {
        if (ph->col_focus != SLEEP_COL_TEXT)
        {
            sleep_set_list_focus(ph, ph->list_focus, SLEEP_COL_TEXT);
        }
        return;
    }
    if (divoom_astrootoo_ui_key_is_right(key_code))
    {
        if (ph->col_focus != SLEEP_COL_ACTION
            && sleep_row_fav_action_enabled(&ph->rows[ph->list_focus]))
        {
            sleep_set_list_focus(ph, ph->list_focus, SLEEP_COL_ACTION);
        }
        return;
    }

    /* 上：边界先判（首行 -> 回顶栏），非边界用网格控制取上一行 */
    if (divoom_astrootoo_ui_key_is_up(key_code))
    {
        lv_obj_t *next;
        int idx;

        if (ph->list_focus <= 0)
        {
            sleep_focus_strip(ph);
            return;
        }
        next = (lv_obj_t *)divoom_menu_direct_ctrl_get(ph->list_direct,
            ph->rows[ph->list_focus].btn, DIVOOM_MENU_DIRECT_UP);
        idx = next ? divoom_menu_direct_ctrl_get_pos(ph->list_direct, next) : -1;
        if (idx < 0 || idx >= ph->row_count)
        {
            idx = ph->list_focus - 1;
        }
        sleep_set_list_focus(ph, idx, ph->col_focus);
        return;
    }

    /* 下：边界先判（末行 -> 分段加载），非边界用网格控制取下一行 */
    if (divoom_astrootoo_ui_key_is_down(key_code))
    {
        lv_obj_t *next;
        int idx;

        if (ph->list_focus >= ph->row_count - 1)
        {
            if (sleep_tab_is_category(ph->active_tab) && ph->page_loaded < ph->total_count)
            {
                sleep_request_append(ph);
            }
            return;
        }
        next = (lv_obj_t *)divoom_menu_direct_ctrl_get(ph->list_direct,
            ph->rows[ph->list_focus].btn, DIVOOM_MENU_DIRECT_DOWN);
        idx = next ? divoom_menu_direct_ctrl_get_pos(ph->list_direct, next) : -1;
        if (idx < 0 || idx >= ph->row_count)
        {
            idx = ph->list_focus + 1;
        }
        sleep_set_list_focus(ph, idx, ph->col_focus);
        return;
    }

    /* 确认 */
    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        r = &ph->rows[ph->list_focus];
        if (ph->col_focus == SLEEP_COL_ACTION
            && sleep_row_fav_action_enabled(r))
        {
            int vt = r->video_type;

            if (vt < 0)
            {
                vt = divoom_sleep_aid_voice_get_video_type(r->audio_type, r->voice_id);
            }
            sleep_request_fav_toggle(ph, r->audio_type, r->voice_id, vt);
        }
        else
        {
            sleep_enter_play_from_list_row(r);
        }
        return;
    }
}

/* ---------- 子页按键 ---------- */

/* 调节亮度/音量（step=±5，clamp 0..100），写回设置并保存 */
static void sleep_sub_adjust_level(Pdivoom_astrootoo_sleep_ctrl ph, sleep_sub_row *sr, int delta)
{
    divoom_sleep_aid_info *inf;
    int *pv = NULL;
    int v;

    inf = divoom_sleep_aid_ctrl_get_settings();
    if (!inf)
    {
        return;
    }
    if (sr->kind == SUB_KIND_SET_BRIGHT)
    {
        pv = sr->data ? &inf->after_bright : &inf->before_bright;
    }
    else if (sr->kind == SUB_KIND_SET_VOL)
    {
        pv = sr->data ? &inf->after_vol : &inf->before_vol;
    }
    else if (sr->kind == SUB_KIND_DIY_CH)
    {
        if (sr->data >= 0 && sr->data < SLEEP_DIY_CH_COUNT)
        {
            pv = &inf->volume[sr->data];
        }
    }
    if (!pv)
    {
        return;
    }
    v = *pv + delta;
    if (v < 0)
    {
        v = 0;
    }
    if (v > 100)
    {
        v = 100;
    }
    *pv = v;
    divoom_sleep_aid_ctrl_settings_save();
    if (sr->kind == SUB_KIND_SET_VOL && ph->active_tab == SLEEP_TAB_SETUP)
    {
        divoom_sleep_aid_ctrl_settings_volume_preview_set_level(v);
    }
}

/* 定时分钟在预设间循环（dir=+1/-1） */
static void sleep_sub_adjust_timer(Pdivoom_astrootoo_sleep_ctrl ph, sleep_sub_row *sr, int dir)
{
    divoom_sleep_aid_info *inf;
    int *pv;
    int cur;
    int i;
    int found = -1;

    inf = divoom_sleep_aid_ctrl_get_settings();
    if (!inf)
    {
        return;
    }
    pv = sr->data ? &inf->after_time : &inf->before_time;
    cur = *pv;
    for (i = 0; i < SLEEP_SET_TIMER_PRESETS; i++)
    {
        if (s_sleep_timer_presets[i] == cur)
        {
            found = i;
            break;
        }
    }
    if (found < 0)
    {
        found = 0;
    }
    found = (found + dir + SLEEP_SET_TIMER_PRESETS) % SLEEP_SET_TIMER_PRESETS;
    *pv = s_sleep_timer_presets[found];
    divoom_sleep_aid_ctrl_settings_save();
}

/* 睡后音源三选一循环 */
static void sleep_sub_adjust_music(Pdivoom_astrootoo_sleep_ctrl ph, int dir)
{
    divoom_sleep_aid_info *inf;
    int mi;

    inf = divoom_sleep_aid_ctrl_get_settings();
    if (!inf)
    {
        return;
    }
    mi = inf->after_music;
    if (mi < 0 || mi >= SLEEP_AFTER_MUSIC_COUNT)
    {
        mi = 0;
    }
    mi = (mi + dir + SLEEP_AFTER_MUSIC_COUNT) % SLEEP_AFTER_MUSIC_COUNT;
    inf->after_music = mi;
    divoom_sleep_aid_ctrl_settings_save();
}

static void sleep_sub_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_sleep_ctrl ph;
    uint8_t key_code;
    sleep_sub_row *sr;
    int dir;

    ph = (Pdivoom_astrootoo_sleep_ctrl)lv_event_get_user_data(e);
    if (!ph || lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    if (ph->sub_count <= 0)
    {
        return;
    }
    if (ph->focus_pane != SLEEP_PANE_SUB)
    {
        sleep_focus_strip(ph);
        sleep_strip_key_cb(e);
        return;
    }

    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    /* 录音中：真源 SleepSplitMenu — Enter 任意行均结束录音；方向键忽略 */
    if (ph->active_tab == SLEEP_TAB_REC && divoom_sleep_aid_ctrl_record_busy())
    {
        if (divoom_astrootoo_ui_key_is_enter(key_code))
        {
            sleep_rec_request_stop(ph);
            return;
        }
        if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code)
            || divoom_astrootoo_ui_key_is_left(key_code) || divoom_astrootoo_ui_key_is_right(key_code))
        {
            return;
        }
    }

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        sleep_focus_strip(ph);
        return;
    }

    /* 上：首行回顶栏，否则上移 */
    if (divoom_astrootoo_ui_key_is_up(key_code))
    {
        if (ph->sub_focus <= 0)
        {
            sleep_focus_strip(ph);
            return;
        }
        sleep_set_sub_focus(ph, ph->sub_focus - 1);
        return;
    }
    /* 下：末行不动，否则下移 */
    if (divoom_astrootoo_ui_key_is_down(key_code))
    {
        if (ph->sub_focus < ph->sub_count - 1)
        {
            sleep_set_sub_focus(ph, ph->sub_focus + 1);
        }
        return;
    }

    sr = &ph->sub[ph->sub_focus];

    /* 左右：双列行切换文本/收藏，其余行调节数值 */
    if (divoom_astrootoo_ui_key_is_left(key_code) || divoom_astrootoo_ui_key_is_right(key_code))
    {
        if (sleep_sub_fav_action_visible(sr))
        {
            int col = ph->sub_col_focus;

            if (divoom_astrootoo_ui_key_is_left(key_code))
            {
                col = SLEEP_COL_TEXT;
            }
            else if (sleep_sub_fav_action_enabled(sr))
            {
                col = SLEEP_COL_ACTION;
            }
            else
            {
                col = SLEEP_COL_TEXT;
            }
            if (col != ph->sub_col_focus)
            {
                ph->sub_col_focus = col;
                sleep_paint_sub(ph, ph->sub_focus);
            }
            return;
        }

        dir = divoom_astrootoo_ui_key_is_right(key_code) ? 1 : -1;
        if (sr->kind == SUB_KIND_SET_BRIGHT || sr->kind == SUB_KIND_SET_VOL
            || sr->kind == SUB_KIND_DIY_CH)
        {
            sleep_sub_adjust_level(ph, sr, dir * 5);
            sleep_sub_refresh_row(ph, ph->sub_focus);
        }
        return;
    }

    /* 确认 */
    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        if (sleep_sub_fav_action_enabled(sr) && ph->sub_col_focus == SLEEP_COL_ACTION)
        {
            int vt = sr->video_type;

            if (vt < 0)
            {
                vt = divoom_sleep_aid_voice_get_video_type(sr->audio_type, sr->voice_id);
            }
            sleep_request_fav_toggle(ph, sr->audio_type, sr->voice_id, vt);
            return;
        }

        if (sr->kind == SUB_KIND_SET_TIMER)
        {
            sleep_pick_enter(ph, SLEEP_PICK_KIND_TIMER, sr->data);
            return;
        }
        if (sr->kind == SUB_KIND_SET_MUSIC)
        {
            sleep_pick_enter(ph, SLEEP_PICK_KIND_MUSIC, 0);
            return;
        }
        if (sr->kind == SUB_KIND_SET_MASK)
        {
            divoom_sleep_aid_info *inf = divoom_sleep_aid_ctrl_get_settings();

            if (inf)
            {
                inf->after_on_off = inf->after_on_off ? 0 : 1;
                divoom_sleep_aid_ctrl_settings_save();
                /* 行数随开关变化：重建并保留当前焦点 */
                ph->pending_sub_focus = ph->sub_focus;
                sleep_request_build(ph);
            }
        }
        else if (sr->kind == SUB_KIND_DIY_PREVIEW || sr->kind == SUB_KIND_DIY_CH
            || sr->kind == SUB_KIND_REC_CLIP)
        {
            sleep_enter_play_from_sub_row(sr);
        }
        else if (sr->kind == SUB_KIND_REC_BTN)
        {
            if (divoom_sleep_aid_ctrl_record_busy())
            {
                sleep_rec_request_stop(ph);
            }
            else
            {
                ph->rec_elapsed = 0;
                ph->rec_start_tick_ms = divoom_system_get_tickms();
                divoom_sleep_aid_ctrl_record_ctrl(1);
                sleep_rec_start_refresh(ph);
                sleep_rec_refresh_ui(ph);
            }
        }
        return;
    }
}

/* ---------- 顶栏构建 ---------- */

static void sleep_create_strip(Pdivoom_astrootoo_sleep_ctrl ph)
{
    int i;
    lv_coord_t start_x;

    /* 顶栏底板（真源 stripBackdrop：#243746） */
    ph->strip_cont = lv_obj_create(ph->root_obj);
    lv_obj_add_style(ph->strip_cont, &ph->ui.content_style, 0);
    lv_obj_set_size(ph->strip_cont, DIVOOM_SCREEN_WIDTH, SLEEP_STRIP_BACKDROP_H);
    lv_obj_set_pos(ph->strip_cont, 0, 0);
    lv_obj_set_style_bg_color(ph->strip_cont, lv_color_hex(DIVOOM_ASTRO_HEADER_V1), 0);
    lv_obj_set_style_bg_opa(ph->strip_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ph->strip_cont, 0, 0);
    lv_obj_clear_flag(ph->strip_cont, LV_OBJ_FLAG_SCROLLABLE);

    start_x = (DIVOOM_SCREEN_WIDTH - (SLEEP_TAB_COUNT * SLEEP_TAB_W
        + (SLEEP_TAB_COUNT - 1) * SLEEP_TAB_GAP)) / 2;

    for (i = 0; i < SLEEP_TAB_COUNT; i++)
    {
        lv_obj_t *btn;
        lv_obj_t *icon;

        /* 透明按钮承载 64×64 图标（图标自带选中底，不另叠边框/背景） */
        btn = lv_btn_create(ph->strip_cont);
        lv_obj_add_style(btn, &ph->ui.content_style, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_radius(btn, 0, 0);
        lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_pad_all(btn, 0, 0);
        lv_obj_set_size(btn, SLEEP_TAB_W, SLEEP_TAB_H);
        lv_obj_set_pos(btn, start_x + i * (SLEEP_TAB_W + SLEEP_TAB_GAP), SLEEP_STRIP_TOP);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

        icon = lv_img_create(btn);
        lv_obj_center(icon);

        lv_obj_add_event_cb(btn, sleep_strip_key_cb, LV_EVENT_KEY, ph);
        lv_group_add_obj(lv_group_get_default(), btn);

        ph->tab_btn[i] = btn;
        ph->tab_icon[i] = icon;
    }
}

/* ---------- 生命周期 ---------- */

void *divoom_astrootoo_sleep_init(void *user_data, int menu_id)
{
    Pdivoom_astrootoo_sleep_ctrl ph;

    (void)user_data;

    ph = divoom_system_malloc_out(sizeof(divoom_astrootoo_sleep_ctrl));
    if (!ph)
    {
        return NULL;
    }

    ph->menu_id = menu_id;
    ph->active_tab = SLEEP_TAB_LOCAL;
    ph->focus_pane = SLEEP_PANE_STRIP;
    ph->col_focus = SLEEP_COL_TEXT;
    ph->sub_col_focus = SLEEP_COL_TEXT;
    ph->action_kind = SLEEP_ACT_NONE;
    ph->pending_sub_focus = -1;
    ph->rec_elapsed = 0;
    ph->rec_ui_busy = 0;
    ph->rec_start_tick_ms = 0;
    ph->rec_clip_sec_cache = 0;
    ph->rec_clip_file_id_cache = 0;

    ph->root_obj = divoom_astrootoo_ui_create_root(&ph->ui);

    sleep_create_strip(ph);

    ph->content = lv_obj_create(ph->root_obj);
    lv_obj_add_style(ph->content, &ph->ui.grid_cont_style, 0);
    lv_obj_set_size(ph->content, DIVOOM_SCREEN_WIDTH, SLEEP_CONTENT_H);
    lv_obj_set_pos(ph->content, 0, SLEEP_CONTENT_TOP);
    lv_obj_set_style_pad_top(ph->content, 4, 0);
    lv_obj_set_layout(ph->content, LV_LAYOUT_NONE);
    lv_obj_set_scrollbar_mode(ph->content, LV_SCROLLBAR_MODE_OFF);

    /* init 内可直接构建首屏内容（非 key 回调） */
    sleep_build_content(ph);
    sleep_paint_strip(ph);

    return ph;
}

void divoom_astrootoo_sleep_disp(void *handle, void *active_obj)
{
    Pdivoom_astrootoo_sleep_ctrl ph;

    (void)active_obj;

    ph = (Pdivoom_astrootoo_sleep_ctrl)handle;
    if (!ph)
    {
        return;
    }

    lv_scr_load(ph->root_obj);
    if (ph->focus_pane == SLEEP_PANE_PICK && ph->pick_root && lv_obj_is_valid(ph->pick_root))
    {
        sleep_pick_restore_group(ph);
        sleep_pick_set_focus(ph, ph->pick_focus);
        return;
    }
    sleep_focus_strip(ph);
    /* 本 disp 之后 clearall 才会拆 107，再补一次顶栏焦点。 */
    sleep_request_refocus_strip(ph);
}

void divoom_astrootoo_sleep_clean(void *handle)
{
    Pdivoom_astrootoo_sleep_ctrl ph;

    ph = (Pdivoom_astrootoo_sleep_ctrl)handle;
    if (!ph)
    {
        return;
    }

    if (ph->build_timer)
    {
        lv_timer_del(ph->build_timer);
        ph->build_timer = NULL;
    }
    if (ph->refocus_timer)
    {
        lv_timer_del(ph->refocus_timer);
        ph->refocus_timer = NULL;
    }
    if (ph->append_timer)
    {
        lv_timer_del(ph->append_timer);
        ph->append_timer = NULL;
    }
    if (ph->action_timer)
    {
        lv_timer_del(ph->action_timer);
        ph->action_timer = NULL;
    }
    if (ph->rec_timer)
    {
        lv_timer_del(ph->rec_timer);
        ph->rec_timer = NULL;
    }
    /* 106→107 时新播放任务已经入队，旧页面销毁不能再追加 STOP。 */
    if (!divoom_sleep_aid_ctrl_is_play_session_active())
    {
        sleep_stop_voice_preview();
        divoom_sleep_aid_ctrl_settings_volume_preview_stop();
    }
    sleep_pick_destroy(ph);
    if (ph->sub_direct)
    {
        divoom_menu_direct_ctrl_destory(ph->sub_direct);
        ph->sub_direct = NULL;
    }
    if (ph->list_direct)
    {
        divoom_menu_direct_ctrl_destory(ph->list_direct);
        ph->list_direct = NULL;
    }
    if (ph->rows)
    {
        divoom_system_free(ph->rows);
        ph->rows = NULL;
    }
    if (ph->root_obj)
    {
        lv_obj_del(ph->root_obj);
        ph->root_obj = NULL;
    }

    divoom_astrootoo_ui_base_reset_styles(&ph->ui);
    divoom_system_free(ph);
}

void divoom_astrootoo_sleep_pause(void *handle)
{
    (void)handle;
}

static Pdivoom_astrootoo_sleep_ctrl divoom_astrootoo_sleep_whitebox_get(void)
{
    int mid;
    void *h;

    h = divoom_menu_control_get_clock_mode_menu_handle();
    if (!h)
    {
        return NULL;
    }
    mid = divoom_menu_control_get_active_menu_id();
    if (mid != DIVOOM_MENU_CONTROL_ID_SLEEP_AID
        && mid != DIVOOM_MENU_CONTROL_ID_SLEEP_AID_ACTIVE)
    {
        return NULL;
    }
    return (Pdivoom_astrootoo_sleep_ctrl)h;
}

int divoom_astrootoo_sleep_whitebox_get_active_tab(void)
{
    Pdivoom_astrootoo_sleep_ctrl ph;

    ph = divoom_astrootoo_sleep_whitebox_get();
    if (!ph)
    {
        return -1;
    }
    return ph->active_tab;
}

int divoom_astrootoo_sleep_whitebox_get_focus_pane(void)
{
    Pdivoom_astrootoo_sleep_ctrl ph;

    ph = divoom_astrootoo_sleep_whitebox_get();
    if (!ph)
    {
        return -1;
    }
    return ph->focus_pane;
}

int divoom_astrootoo_sleep_whitebox_get_sub_focus(void)
{
    Pdivoom_astrootoo_sleep_ctrl ph;

    ph = divoom_astrootoo_sleep_whitebox_get();
    if (!ph)
    {
        return -1;
    }
    return ph->sub_focus;
}

int divoom_astrootoo_sleep_whitebox_get_sub_count(void)
{
    Pdivoom_astrootoo_sleep_ctrl ph;

    ph = divoom_astrootoo_sleep_whitebox_get();
    if (!ph)
    {
        return -1;
    }
    return ph->sub_count;
}

int divoom_astrootoo_sleep_whitebox_get_sub_kind(void)
{
    Pdivoom_astrootoo_sleep_ctrl ph;

    ph = divoom_astrootoo_sleep_whitebox_get();
    if (!ph || ph->sub_focus < 0 || ph->sub_focus >= ph->sub_count)
    {
        return -1;
    }
    return ph->sub[ph->sub_focus].kind;
}

void divoom_astrootoo_sleep_sync_hw_volume_sliders(void)
{
    Pdivoom_astrootoo_sleep_ctrl ph;
    int i;

    if (divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_ID_SLEEP_AID)
    {
        return;
    }
    ph = divoom_astrootoo_sleep_whitebox_get();
    if (!ph || ph->active_tab != SLEEP_TAB_SETUP)
    {
        return;
    }
    divoom_sleep_aid_ctrl_settings_save();
    for (i = 0; i < ph->sub_count; i++)
    {
        if (ph->sub[i].kind == SUB_KIND_SET_VOL)
        {
            sleep_sub_refresh_row(ph, i);
        }
    }
}

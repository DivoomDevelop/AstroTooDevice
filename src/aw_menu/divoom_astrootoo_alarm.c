#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_menu_direct_ctrl.h"
#include "divoom_alarm.h"
#include "divoom_main.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_astrootoo_alarm.h"
#include "divoom_astrootoo_theme.h"
#include "divoom_tf_music.h"
#include "divoom_local_music_ctrl.h"
#include "divoom_mic_record.h"
#include "divoom_astrootoo_settings_internal.h"
#include "divoom_astrootoo_time_pick.h"
#include "divoom_manage_image.h"

#define DIVOOM_ASTROTOO_ALARM_UI_DIR          "23/"

/* 子页布局（480x480） */
#define DIVOOM_ASTRO_ALARM_BELL_COUNT   12
#define DIVOOM_ASTRO_ALARM_SUB_TOP      (DIVOOM_ASTRO_HEADER_H + 8)
#define DIVOOM_ASTRO_ALARM_EDIT_ROW_W   448
#define DIVOOM_ASTRO_ALARM_EDIT_ROW_H   64
#define DIVOOM_ASTRO_ALARM_EDIT_ROW_GAP 8
#define DIVOOM_ASTRO_ALARM_EDIT_PAD_X   16
#define DIVOOM_ASTRO_ALARM_BELL_CAT_KEY_X         44
#define DIVOOM_ASTRO_ALARM_BELL_CAT_VAL_INSET_R   14
#define DIVOOM_ASTRO_ALARM_BELL_CAT_VAL_MIN_W     72
#define DIVOOM_ASTRO_ALARM_SUBROW_H     56
#define DIVOOM_ASTRO_ALARM_SUBROW_GAP   8

/* 音量条（对照 AstroTooUI FlowtooSlider pixelRail：绿色上下渐变 + 像素滑块） */
#define DIVOOM_ASTRO_ALARM_VOL_FILL_TOP   0x90FF78
#define DIVOOM_ASTRO_ALARM_VOL_FILL_BOT   0x28B028
#define DIVOOM_ASTRO_ALARM_VOL_TRACK_TOP  0xA8A8A8
#define DIVOOM_ASTRO_ALARM_VOL_TRACK_BOT  0x585858
#define DIVOOM_ASTRO_ALARM_VOL_W          150

/* 铃声大类（对照模拟器 FLOWTOO_RINGTONE_PICKER_CATEGORIES：经典/录音/TF/蓝牙） */
#define DIVOOM_ASTRO_ALARM_RING_CAT_COUNT 4
#define DIVOOM_ASTRO_ALARM_RING_ITEM_MAX  12
#define DIVOOM_ASTRO_ALARM_TF_MODE        4
#define DIVOOM_ASTRO_ALARM_BT_MODE        3

/* 经典铃声图标网格（3 列 × 4 行，对照工程 49_bell_*.png 图案列表） */
#define DIVOOM_ASTRO_ALARM_GRID_COLS      3
#define DIVOOM_ASTRO_ALARM_GRID_ROWS      4
#define DIVOOM_ASTRO_ALARM_GRID_PAD       16
#define DIVOOM_ASTRO_ALARM_GRID_GAP       8
#define DIVOOM_ASTRO_ALARM_GRID_ICON      72  /* 切图已缩到此显示尺寸，1:1 显示 */

/* 录音页（真源 480-main AlarmRingtoneRecorderPick：448×214 面板 + 56 行 + gap16） */
#define DIVOOM_ASTRO_ALARM_REC_PANEL_W    DIVOOM_ASTRO_REPEAT_ROW_W
#define DIVOOM_ASTRO_ALARM_REC_PANEL_H    214
#define DIVOOM_ASTRO_ALARM_REC_ROW_H      DIVOOM_ASTRO_REPEAT_ROW_H
#define DIVOOM_ASTRO_ALARM_REC_GAP        16
#define DIVOOM_ASTRO_ALARM_REC_PANEL_GAP  10  /* 面板内 hint/图标/计时间距（设计 gap:10） */
#define DIVOOM_ASTRO_ALARM_REC_ICON_SLOT  80  /* 设计 iconSlot 80×80 */
#define DIVOOM_ASTRO_ALARM_REC_ICON_PX    48  /* 设计原图 48×48，1:1 显示 */
#define DIVOOM_ASTRO_ALARM_REC_HINT_OPA   ((lv_opa_t)183) /* ink @ 0.72 */
#define DIVOOM_ASTRO_ALARM_REC_TAB_INK    0xB8B0A8
#define DIVOOM_ASTRO_ALARM_REC_MAX_SEC    (5u * 60u)
#define DIVOOM_ASTRO_ALARM_REC_TMP_MP3    DIVOOM_TMP_DIR "/divoom_alarm_record_cap.mp3"
#define DIVOOM_ASTRO_ALARM_REC_MIN_BYTES  128
#define DIVOOM_ASTRO_ALARM_REC_MP3_BPS    DIVOOM_MIC_RECORD_MP3_CBR32_BPS
#define DIVOOM_ASTRO_ALARM_REC_TIMER_MS   200

/* TF 铃声选歌（对照 AstroTooUI SourceTfCardScreen / SourceTfCardTracksScreen） */
#define DIVOOM_ASTRO_ALARM_TF_VISIBLE_ROWS  6
#define DIVOOM_ASTRO_ALARM_TF_FOLDER_STACK  16
#define DIVOOM_ASTRO_ALARM_TF_LIST_TOP      8
#define DIVOOM_ASTRO_ALARM_TF_PREVIEW_SEC   60
enum
{
    DIVOOM_ASTRO_RING_CAT_CLASSIC = 0,  /* 经典铃声 mode0 tip_index 0..11 */
    DIVOOM_ASTRO_RING_CAT_RECORD,       /* 录音     mode1 */
    DIVOOM_ASTRO_RING_CAT_TF,           /* TF卡     SoundType 4 */
    DIVOOM_ASTRO_RING_CAT_BT,           /* 蓝牙     SoundType 3 */
};

/* 闹钟页面模式（对照 AstroTooUI AlarmScreen 的 mode 模型） */
enum
{
    DIVOOM_ASTROTOO_ALARM_MODE_LIST = 0,
    DIVOOM_ASTROTOO_ALARM_MODE_EDIT,
    DIVOOM_ASTROTOO_ALARM_MODE_TIME,
    DIVOOM_ASTROTOO_ALARM_MODE_REPEAT,
    DIVOOM_ASTROTOO_ALARM_MODE_BELL,
};

enum
{
    DIVOOM_ASTROTOO_ALARM_COL_BODY = 0,
    DIVOOM_ASTROTOO_ALARM_COL_TOGGLE,
};

typedef struct _divoom_astrootoo_alarm_row
{
    int alarm_index;
    lv_obj_t *btn;
    lv_obj_t *body_cont;
    lv_obj_t *time_label;
    lv_obj_t *meridiem_label;
    lv_obj_t *repeat_label;
    lv_obj_t *chevron_img;
    lv_obj_t *toggle_cont;
    lv_obj_t *switch_img;
} divoom_astrootoo_alarm_row, *Pdivoom_astrootoo_alarm_row;

typedef struct _divoom_astrootoo_alarm_ctrl
{
    divoom_astrootoo_ui_base ui;
    lv_style_t slot_idle_style;
    lv_style_t slot_focus_style;
    uint8 slot_styles_initialized;
    int menu_id;
    lv_obj_t *root_obj;
    lv_obj_t *key_btn;              /* 预留全屏捕获（列表态隐藏，与 games 一致） */
    lv_obj_t *list_header;
    lv_obj_t *content;
    void *direct_handle;
    T_DIVOOM_ALARM_ACTIVE_DO_INFO *alarm_list;
    divoom_astrootoo_alarm_row *rows;
    int alarm_count;
    int row_count;
    int column_focus;
    uint8 assets_loaded;
    lv_timer_t *load_timer;

    /* 子页 / 编辑 */
    int mode;
    int edit_index;                 /* 正在编辑的 alarm_list 下标；-1 表示新增 */
    int pending_rebuild_focus;      /* 新增保存后重建列表时聚焦的行 */
    T_DIVOOM_ALARM_ACTIVE_DO_INFO edit_info;
    T_DIVOOM_ALARM_ACTIVE_DO_INFO edit_info_baseline; /* 进入编辑页时的快照，用于检测是否修改 */
    lv_timer_t *rebuild_timer;

    /* edit 中枢页 */
    lv_obj_t *edit_root;
    lv_obj_t *edit_rows[4];
    lv_obj_t *edit_val[3];
    lv_obj_t *edit_chevron[3];
    lv_obj_t *edit_vol_bar;
    int edit_focus;
    void *edit_direct;

    /* timePick 子页 */
    lv_obj_t *time_root;
    lv_obj_t *tp_tray;
    lv_obj_t *tp_ampm_box;
    lv_obj_t *tp_ampm_lbl;
    lv_obj_t *tp_h_box;
    lv_obj_t *tp_m_box;
    lv_obj_t *tp_cancel;
    lv_obj_t *tp_confirm;
    lv_obj_t *tp_h_lbl;
    lv_obj_t *tp_m_lbl;
    lv_obj_t *tp_h_cap;
    lv_obj_t *tp_m_cap;
    int tp_pick_field;
    int tp_hour;                    /* 0-23 工作值 */
    int tp_minute;

    /* repeatPick 子页 */
    lv_obj_t *repeat_root;
    lv_obj_t *repeat_list;
    lv_obj_t *rp_row[8];
    lv_obj_t *rp_mark[8];
    int rp_focus;
    void *rp_direct;

    /* bellPick 子页（大类 -> 条目 两级） */
    lv_obj_t *bell_root;
    lv_obj_t *bell_cat_list;
    lv_obj_t *bell_item_list;
    lv_obj_t *bell_cat_row[DIVOOM_ASTRO_ALARM_RING_CAT_COUNT];
    lv_obj_t *bell_cat_mark[DIVOOM_ASTRO_ALARM_RING_CAT_COUNT];
    lv_obj_t *bell_cat_lbl[DIVOOM_ASTRO_ALARM_RING_CAT_COUNT];
    lv_obj_t *bell_cat_val[DIVOOM_ASTRO_ALARM_RING_CAT_COUNT];
    lv_obj_t *bell_cat_chevron[DIVOOM_ASTRO_ALARM_RING_CAT_COUNT];
    lv_obj_t *bell_item_row[DIVOOM_ASTRO_ALARM_RING_ITEM_MAX];
    lv_obj_t *bell_item_mark[DIVOOM_ASTRO_ALARM_RING_ITEM_MAX];
    lv_obj_t *bell_item_label[DIVOOM_ASTRO_ALARM_RING_ITEM_MAX];
    int bell_phase;                 /* 0=cats 2=classic网格 3=录音 4=TF选歌 */
    int bell_cat_focus;
    int bell_item_focus;
    int bell_cur_cat;               /* 进入条目页时的大类 */
    void *bell_cat_direct;
    void *bell_item_direct;

    /* 经典铃声图标网格（3×4） */
    lv_obj_t *bell_grid_list;
    lv_obj_t *bell_grid_btn[DIVOOM_ASTRO_ALARM_BELL_COUNT];
    lv_obj_t *bell_grid_check[DIVOOM_ASTRO_ALARM_BELL_COUNT];
    void *bell_grid_direct;
    int bell_grid_focus;

    /* 录音页（真源 AlarmRingtoneRecorderPick） */
    lv_obj_t *bell_record_list;
    lv_obj_t *rec_panel;
    lv_obj_t *rec_title;
    lv_obj_t *rec_icon_idle;
    lv_obj_t *rec_icon_stop;
    lv_obj_t *rec_play_img;
    lv_obj_t *rec_time_lbl;
    lv_obj_t *rec_file_btn;
    lv_obj_t *rec_file_title;
    lv_obj_t *rec_file_dur;
    lv_obj_t *rec_as_bell_btn;
    lv_obj_t *rec_as_bell_check;
    int rec_focus;
    uint8 rec_capture_active;
    uint8 rec_playing;
    uint8 rec_has_clip;
    uint8 rec_pending_relocate;     /* 新建闹钟 alarm_id=0 时录音，待分配 id 后搬迁 */
    uint8 rec_clip_month;           /* 1–12，无片为 0 */
    uint8 rec_clip_day;             /* 1–31，无片为 0 */
    uint32 rec_elapsed_sec;
    uint32 rec_clip_sec;
    uint32 rec_start_tick_ms;       /* 0=已冻住/未在录 */
    uint32 rec_play_start_tick_ms;
    uint32 rec_enc_sec;             /* 0x64 给出的秒；0=尚未结束 */
    lv_timer_t *rec_timer;

    /* TF 选歌（与播放页相同多层 dir_page，每屏仍 6 行） */
    lv_obj_t *bell_tf_root;
    lv_obj_t *bell_tf_list;
    lv_obj_t *bell_tf_title;
    lv_obj_t *bell_tf_row[DIVOOM_ASTRO_ALARM_TF_VISIBLE_ROWS];
    lv_obj_t *bell_tf_icon[DIVOOM_ASTRO_ALARM_TF_VISIBLE_ROWS];
    lv_obj_t *bell_tf_label[DIVOOM_ASTRO_ALARM_TF_VISIBLE_ROWS];
    lv_obj_t *bell_tf_chevron[DIVOOM_ASTRO_ALARM_TF_VISIBLE_ROWS];
    lv_obj_t *bell_tf_mark[DIVOOM_ASTRO_ALARM_TF_VISIBLE_ROWS];
    lv_obj_t *bell_tf_empty_btn;    /* 空列表时透明按键捕获，保证 ESC 可返回 */
    int tf_folder_id;
    int tf_folder_stack[DIVOOM_ASTRO_ALARM_TF_FOLDER_STACK];
    int tf_folder_depth;
    int tf_total;
    int tf_window_start;
    int tf_logical_focus;
    uint8 bell_tf_built;
    divoom_tf_entry tf_slots[DIVOOM_ASTRO_ALARM_TF_VISIBLE_ROWS];
    int tf_slot_count;
    lv_timer_t *tf_catalog_timer;
} divoom_astrootoo_alarm_ctrl, *Pdivoom_astrootoo_alarm_ctrl;

/* 前向声明 */
static void divoom_astrootoo_alarm_rec_relocate_placeholder(int new_alarm_id);
static void divoom_astrootoo_alarm_enter_list(Pdivoom_astrootoo_alarm_ctrl phandle, int focus_row);
static void divoom_astrootoo_alarm_enter_edit(Pdivoom_astrootoo_alarm_ctrl phandle, int alarm_index);
static void divoom_astrootoo_alarm_enter_time(Pdivoom_astrootoo_alarm_ctrl phandle);
static void divoom_astrootoo_alarm_enter_repeat(Pdivoom_astrootoo_alarm_ctrl phandle);
static void divoom_astrootoo_alarm_enter_bell(Pdivoom_astrootoo_alarm_ctrl phandle);
static void divoom_astrootoo_alarm_edit_key_cb(lv_event_t *e);
static void divoom_astrootoo_alarm_time_key_cb(lv_event_t *e);
static void divoom_astrootoo_alarm_repeat_key_cb(lv_event_t *e);
static void divoom_astrootoo_alarm_bell_key_cb(lv_event_t *e);
static void divoom_astrootoo_alarm_bell_grid_key_cb(lv_event_t *e);
static void divoom_astrootoo_alarm_bell_tf_key_cb(lv_event_t *e);
static void divoom_astrootoo_alarm_bell_stop_preview(void);
static void divoom_astrootoo_alarm_edit_preview_selected_sound(Pdivoom_astrootoo_alarm_ctrl phandle);
static void divoom_astrootoo_alarm_bell_preview_item(Pdivoom_astrootoo_alarm_ctrl phandle, int cat, int item);
static void divoom_astrootoo_alarm_bell_show_cats(Pdivoom_astrootoo_alarm_ctrl phandle);
static void divoom_astrootoo_alarm_rec_key_cb(lv_event_t *e);
static void divoom_astrootoo_alarm_tf_refresh_window(Pdivoom_astrootoo_alarm_ctrl phandle);
static void divoom_astrootoo_alarm_tf_bind_group(Pdivoom_astrootoo_alarm_ctrl phandle);
static void divoom_astrootoo_alarm_tf_apply_row_icons(Pdivoom_astrootoo_alarm_ctrl phandle);
static void divoom_astrootoo_alarm_tf_set_header_title(Pdivoom_astrootoo_alarm_ctrl phandle, const char *text);
static void divoom_astrootoo_alarm_tf_enter_folder(Pdivoom_astrootoo_alarm_ctrl phandle, int folder_id);
static int divoom_astrootoo_alarm_tf_go_up(Pdivoom_astrootoo_alarm_ctrl phandle);
static void divoom_astrootoo_alarm_edit_refresh_values(Pdivoom_astrootoo_alarm_ctrl phandle);
static void divoom_astrootoo_alarm_edit_on_modified(Pdivoom_astrootoo_alarm_ctrl phandle);
static void divoom_astrootoo_alarm_rec_relocate_placeholder(int new_alarm_id);
static void divoom_astrootoo_alarm_rebuild_cb(lv_timer_t *t);
static void divoom_astrootoo_alarm_rec_format_clock(uint32 sec, char *buf, int buf_len);
static void divoom_astrootoo_alarm_rec_format_date(uint8 month, uint8 day, char *buf, int buf_len);
static void divoom_astrootoo_alarm_rec_cat_summary(Pdivoom_astrootoo_alarm_ctrl phandle, char *buf, int len);
static void divoom_astrootoo_alarm_rec_load_clip_date(Pdivoom_astrootoo_alarm_ctrl phandle);
static void divoom_astrootoo_alarm_rec_stamp_clip_date_now(Pdivoom_astrootoo_alarm_ctrl phandle);
static void divoom_astrootoo_alarm_rec_set_hot(lv_obj_t *obj, int hot, int is_panel);

static void divoom_astrootoo_alarm_return_pre_timer_cb(lv_timer_t *t)
{
    divoom_menu_control_return_pre();
    lv_timer_del(t);
}

/* 经典铃声 12 图标（与工程 49_bell_*.png 同源，拷贝到 ui/23 加 at_ 前缀） */
static const char *divoom_astrootoo_alarm_bell_icons[DIVOOM_ASTRO_ALARM_BELL_COUNT] =
{
    "at_bell_rain.png", "at_bell_wave.png", "at_bell_city.png", "at_bell_forest.png",
    "at_bell_dolphin.png", "at_bell_sun.png", "at_bell_bird.png", "at_bell_ring.png",
    "at_bell_clock.png", "at_bell_piano.png", "at_bell_clock2.png", "at_bell_music.png",
};

/* 与番茄铃声同 key，走 strings.xml 本地化 */
static const char *const divoom_astrootoo_alarm_classic_ring_keys[DIVOOM_ASTRO_ALARM_BELL_COUNT] =
{
    "STORM", "WAVE", "CITY", "FOREST", "DOLPHIN", "SUNSET",
    "BIRD", "BELL", "DIGICLOCK", "PIANO", "ALARMCLOCK", "MELODY",
};

static void divoom_astrootoo_alarm_init_slot_styles(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    if (!phandle || phandle->slot_styles_initialized)
    {
        return;
    }

    lv_style_init(&phandle->slot_idle_style);
    lv_style_set_bg_color(&phandle->slot_idle_style, lv_color_hex(DIVOOM_ASTRO_SURFACE_IDLE));
    lv_style_set_bg_opa(&phandle->slot_idle_style, LV_OPA_COVER);
    lv_style_set_border_color(&phandle->slot_idle_style, lv_color_hex(DIVOOM_ASTRO_STROKE));
    lv_style_set_border_width(&phandle->slot_idle_style, 1);
    lv_style_set_radius(&phandle->slot_idle_style, DIVOOM_ASTRO_RADIUS_SM);
    lv_style_set_pad_all(&phandle->slot_idle_style, 0);

    lv_style_init(&phandle->slot_focus_style);
    lv_style_set_bg_color(&phandle->slot_focus_style, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS));
    lv_style_set_bg_opa(&phandle->slot_focus_style, LV_OPA_COVER);
    lv_style_set_border_color(&phandle->slot_focus_style, lv_color_hex(DIVOOM_ASTRO_STROKE));
    lv_style_set_border_width(&phandle->slot_focus_style, 1);
    lv_style_set_radius(&phandle->slot_focus_style, DIVOOM_ASTRO_RADIUS_SM);
    lv_style_set_pad_all(&phandle->slot_focus_style, 0);

    phandle->slot_styles_initialized = 1;
}

static void divoom_astrootoo_alarm_reset_slot_styles(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    if (!phandle || !phandle->slot_styles_initialized)
    {
        return;
    }

    lv_style_reset(&phandle->slot_idle_style);
    lv_style_reset(&phandle->slot_focus_style);
    phandle->slot_styles_initialized = 0;
}

static void divoom_astrootoo_alarm_apply_slot_style(lv_obj_t *slot, Pdivoom_astrootoo_alarm_ctrl phandle,
    int focused_sub)
{
    if (!slot || !phandle)
    {
        return;
    }

    lv_obj_remove_style(slot, &phandle->slot_idle_style, 0);
    lv_obj_remove_style(slot, &phandle->slot_focus_style, 0);
    if (focused_sub)
    {
        lv_obj_add_style(slot, &phandle->slot_focus_style, 0);
    }
    else
    {
        lv_obj_add_style(slot, &phandle->slot_idle_style, 0);
    }
}

/* 闹钟列表/编辑摘要：Mo…Su 英文两字母缩写（真源 flowtooAlarms.ts，与语言无关） */
static const char * const divoom_astrootoo_alarm_week_abbr[7] =
{
    "Mo", "Tu", "We", "Th", "Fr", "Sa", "Su",
};

static void divoom_astrootoo_alarm_format_repeat_summary(uint8 cyc_flag, char *buf, int len)
{
    int i;
    int idx;

    if (!buf || len <= 0)
    {
        return;
    }
    buf[0] = '\0';

    if (cyc_flag == 0)
    {
        const char *never;

        never = divoom_menu_control_get_disp_str("Never");
        snprintf(buf, (size_t)len, "%s", never ? never : "");
        return;
    }
    if (cyc_flag == 0x7f)
    {
        const char *daily;

        daily = divoom_menu_control_get_disp_str("every day");
        snprintf(buf, (size_t)len, "%s", daily ? daily : "");
        return;
    }

    idx = 0;
    for (i = 0; i < 7; i++)
    {
        if (cyc_flag & (uint8)(1 << ((i + 1) % 7)))
        {
            if (idx > 0)
            {
                idx += snprintf(buf + idx, (size_t)(len - idx), " ");
            }
            idx += snprintf(buf + idx, (size_t)(len - idx), "%s",
                divoom_astrootoo_alarm_week_abbr[i]);
        }
    }
}

static void divoom_astrootoo_alarm_set_repeat_label(Pdivoom_astrootoo_alarm_ctrl phandle, int row_index)
{
    T_DIVOOM_ALARM_ACTIVE_DO_INFO *item;
    char disp_str[64];

    if (!phandle || !phandle->rows || row_index < 0 || row_index >= phandle->row_count
        || !phandle->rows[row_index].repeat_label)
    {
        return;
    }

    if (!phandle->alarm_list)
    {
        return;
    }

    item = &phandle->alarm_list[phandle->rows[row_index].alarm_index];
    divoom_astrootoo_alarm_format_repeat_summary(item->cyc_flag, disp_str, (int)sizeof(disp_str));
    lv_label_set_text(phandle->rows[row_index].repeat_label, disp_str);
}

static void divoom_astrootoo_alarm_format_time_parts(Pdivoom_astrootoo_alarm_ctrl phandle, int alarm_index,
    char *tbuf, int tlen, char *mbuf, int mlen)
{
    T_DIVOOM_ALARM_ACTIVE_DO_INFO *item;
    uint8 hour;
    uint8 minute;

    if (tbuf && tlen > 0)
    {
        tbuf[0] = '\0';
    }
    if (mbuf && mlen > 0)
    {
        mbuf[0] = '\0';
    }
    if (!phandle || !phandle->alarm_list || alarm_index < 0 || alarm_index >= phandle->alarm_count
        || !tbuf || tlen <= 0)
    {
        if (tbuf && tlen > 0)
        {
            snprintf(tbuf, (size_t)tlen, "--:--");
        }
        return;
    }

    item = &phandle->alarm_list[alarm_index];
    hour = item->hour;
    minute = item->minute;
    if (divoom_astrootoo_time_pick_use_hour24())
    {
        snprintf(tbuf, (size_t)tlen, "%02u:%02u", (unsigned)hour, (unsigned)minute);
        return;
    }
    {
        int h12;
        const char *ap;

        h12 = hour % 12;
        if (h12 == 0)
        {
            h12 = 12;
        }
        snprintf(tbuf, (size_t)tlen, "%02d:%02u", h12, (unsigned)minute);
        if (mbuf && mlen > 0)
        {
            ap = divoom_menu_control_get_disp_str(hour < 12 ? "AM" : "PM");
            if (ap)
            {
                snprintf(mbuf, (size_t)mlen, "%s", ap);
            }
        }
    }
}

static void divoom_astrootoo_alarm_layout_list_row(Pdivoom_astrootoo_alarm_row row)
{
    lv_coord_t time_w;
    lv_coord_t mer_w;
    lv_coord_t repeat_x;
    lv_coord_t repeat_w;
    const lv_coord_t repeat_right = DIVOOM_ASTRO_ALARM_BODY_W - (14 + 12 + 8);

    if (!row || !row->time_label || !row->repeat_label)
    {
        return;
    }

    lv_obj_update_layout(row->time_label);
    time_w = lv_obj_get_width(row->time_label);
    mer_w = 0;
    if (row->meridiem_label && !lv_obj_has_flag(row->meridiem_label, LV_OBJ_FLAG_HIDDEN))
    {
        lv_obj_align_to(row->meridiem_label, row->time_label, LV_ALIGN_OUT_RIGHT_MID,
            DIVOOM_ASTRO_TIME_MERIDIEM_GAP, 0);
        lv_obj_update_layout(row->meridiem_label);
        mer_w = DIVOOM_ASTRO_TIME_MERIDIEM_GAP + lv_obj_get_width(row->meridiem_label);
    }
    repeat_x = 12 + time_w + mer_w + 8;
    repeat_w = repeat_right - repeat_x;
    if (repeat_w < 48)
    {
        repeat_w = 48;
    }
    lv_obj_set_width(row->repeat_label, repeat_w);
    lv_obj_align(row->repeat_label, LV_ALIGN_LEFT_MID, repeat_x, 0);
}

static void divoom_astrootoo_alarm_format_time(Pdivoom_astrootoo_alarm_ctrl phandle, int alarm_index,
    char *buf, int buf_len)
{
    divoom_astrootoo_alarm_format_time_parts(phandle, alarm_index, buf, buf_len, NULL, 0);
}

static void divoom_astrootoo_alarm_refresh_row_visual(Pdivoom_astrootoo_alarm_ctrl phandle, int row_index,
    int row_focused)
{
    Pdivoom_astrootoo_alarm_row row;
    int body_focus;
    int toggle_focus;
    int enabled;
    lv_opa_t row_opa;
    uint32_t text_color;

    if (!phandle || !phandle->rows || row_index < 0 || row_index >= phandle->row_count)
    {
        return;
    }

    row = &phandle->rows[row_index];
    body_focus = row_focused && (phandle->column_focus == DIVOOM_ASTROTOO_ALARM_COL_BODY);
    toggle_focus = row_focused && (phandle->column_focus == DIVOOM_ASTROTOO_ALARM_COL_TOGGLE);

    enabled = 1;
    if (phandle->alarm_list)
    {
        enabled = phandle->alarm_list[row->alarm_index].on_off ? 1 : 0;
    }

    /* 与定时开关机列表一致：禁用时整行（含 chevron / switch）50% 透明度 + -muted 字色 */
    row_opa = enabled ? LV_OPA_COVER : LV_OPA_50;
    text_color = enabled ? DIVOOM_ASTRO_INK : DIVOOM_ASTRO_INK_MUTED;

    divoom_astrootoo_alarm_apply_slot_style(row->body_cont, phandle, body_focus);
    divoom_astrootoo_alarm_apply_slot_style(row->toggle_cont, phandle, toggle_focus);

    if (row->body_cont)
    {
        lv_obj_set_style_opa(row->body_cont, row_opa, 0);
    }
    if (row->toggle_cont)
    {
        lv_obj_set_style_opa(row->toggle_cont, row_opa, 0);
    }
    if (row->time_label)
    {
        lv_obj_set_style_text_opa(row->time_label, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(row->time_label, lv_color_hex(text_color), 0);
    }
    if (row->meridiem_label)
    {
        lv_obj_set_style_text_opa(row->meridiem_label, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(row->meridiem_label, lv_color_hex(text_color), 0);
    }
    if (row->repeat_label)
    {
        lv_obj_set_style_text_opa(row->repeat_label, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(row->repeat_label, lv_color_hex(text_color), 0);
    }
}

static void divoom_astrootoo_alarm_refresh_all_rows(Pdivoom_astrootoo_alarm_ctrl phandle, lv_obj_t *focused_btn)
{
    int i;

    if (!phandle || !phandle->rows)
    {
        return;
    }

    for (i = 0; i < phandle->row_count; i++)
    {
        divoom_astrootoo_alarm_refresh_row_visual(phandle, i,
            (focused_btn && phandle->rows[i].btn == focused_btn) ? 1 : 0);
    }
}

static void divoom_astrootoo_alarm_refresh_row_data(Pdivoom_astrootoo_alarm_ctrl phandle, int row_index)
{
    char time_buf[16];
    char mer_buf[16];
    T_DIVOOM_ALARM_ACTIVE_DO_INFO *item;
    Pdivoom_astrootoo_alarm_row row;

    if (!phandle || !phandle->rows || row_index < 0 || row_index >= phandle->row_count)
    {
        return;
    }

    row = &phandle->rows[row_index];
    if (row->chevron_img)
    {
        lv_obj_clear_flag(row->chevron_img, LV_OBJ_FLAG_HIDDEN);
    }

    divoom_astrootoo_alarm_format_time_parts(phandle, row->alarm_index,
        time_buf, (int)sizeof(time_buf), mer_buf, (int)sizeof(mer_buf));
    if (row->time_label)
    {
        lv_label_set_text(row->time_label, time_buf);
    }
    if (row->meridiem_label)
    {
        if (mer_buf[0] != '\0')
        {
            lv_label_set_text(row->meridiem_label, mer_buf);
            lv_obj_clear_flag(row->meridiem_label, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_label_set_text(row->meridiem_label, "");
            lv_obj_add_flag(row->meridiem_label, LV_OBJ_FLAG_HIDDEN);
        }
    }
    divoom_astrootoo_alarm_set_repeat_label(phandle, row_index);
    divoom_astrootoo_alarm_layout_list_row(row);

    if (phandle->rows[row_index].switch_img)
    {
        lv_obj_clear_flag(phandle->rows[row_index].switch_img, LV_OBJ_FLAG_HIDDEN);
        if (phandle->assets_loaded && phandle->alarm_list)
        {
            item = &phandle->alarm_list[phandle->rows[row_index].alarm_index];
            if (item->on_off)
            {
                divoom_menu_control_lv_img_set_src(phandle->rows[row_index].switch_img,
                    DIVOOM_ASTROTOO_ALARM_UI_DIR "at_alarm_switch_on.png");
            }
            else
            {
                divoom_menu_control_lv_img_set_src(phandle->rows[row_index].switch_img,
                    DIVOOM_ASTROTOO_ALARM_UI_DIR "at_alarm_switch_off.png");
            }
        }
    }
}

static void divoom_astrootoo_alarm_toggle_row(Pdivoom_astrootoo_alarm_ctrl phandle, int row_index)
{
    T_DIVOOM_ALARM_ACTIVE_DO_INFO *item;

    if (!phandle || !phandle->alarm_list || row_index < 0 || row_index >= phandle->row_count)
    {
        return;
    }

    item = &phandle->alarm_list[phandle->rows[row_index].alarm_index];
    item->on_off = item->on_off ? 0 : 1;
    divoom_alarm_menu_update_alarm_item(item);
    divoom_astrootoo_alarm_refresh_row_data(phandle, row_index);
    divoom_astrootoo_alarm_refresh_row_visual(phandle, row_index, 1);
}

static void divoom_astrootoo_alarm_load_assets_cb(lv_timer_t *timer)
{
    Pdivoom_astrootoo_alarm_ctrl phandle;
    int i;

    phandle = (Pdivoom_astrootoo_alarm_ctrl)lv_timer_get_user_data(timer);
    if (phandle)
    {
        /* 一次性 timer（repeat_count=1）本次回调后由 LVGL 自动删除，
           必须清空句柄，否则 clean 再次 lv_timer_del 悬空指针会破坏 timer 链表导致死循环 */
        phandle->load_timer = NULL;
    }
    if (!phandle || phandle->assets_loaded)
    {
        return;
    }

    phandle->assets_loaded = 1;
    for (i = 0; i < phandle->row_count; i++)
    {
        if (phandle->rows[i].chevron_img)
        {
            divoom_menu_control_lv_img_set_src(phandle->rows[i].chevron_img,
                DIVOOM_ASTROTOO_ALARM_UI_DIR "at_alarm_chevron.png");
        }
        divoom_astrootoo_alarm_refresh_row_data(phandle, i);
    }
    for (i = 0; i < 3; i++)
    {
        if (phandle->edit_chevron[i])
        {
            divoom_menu_control_lv_img_set_src(phandle->edit_chevron[i],
                DIVOOM_ASTROTOO_ALARM_UI_DIR "at_alarm_chevron.png");
        }
    }
    for (i = 0; i < DIVOOM_ASTRO_ALARM_RING_CAT_COUNT; i++)
    {
        if (phandle->bell_cat_chevron[i])
        {
            divoom_menu_control_lv_img_set_src(phandle->bell_cat_chevron[i],
                DIVOOM_ASTROTOO_ALARM_UI_DIR "at_alarm_chevron.png");
        }
    }
}

static void divoom_astrootoo_alarm_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_alarm_ctrl phandle;
    Pdivoom_astrootoo_alarm_row row;
    uint8_t key_code;
    lv_obj_t *target;
    lv_obj_t *next;
    int i;

    phandle = (Pdivoom_astrootoo_alarm_ctrl)lv_event_get_user_data(e);
    if (!phandle || lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }

    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }

    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    if (phandle->mode != DIVOOM_ASTROTOO_ALARM_MODE_LIST)
    {
        return;
    }

    target = lv_event_get_target(e);
    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        lv_timer_t *tm;

        tm = lv_timer_create(divoom_astrootoo_alarm_return_pre_timer_cb, 0, NULL);
        if (tm)
        {
            lv_timer_set_repeat_count(tm, 1);
        }
        return;
    }

    if (!target)
    {
        return;
    }

    row = NULL;
    for (i = 0; i < phandle->row_count; i++)
    {
        if (phandle->rows[i].btn == target)
        {
            row = &phandle->rows[i];
            break;
        }
    }

    if (!row)
    {
        return;
    }

    /* ENTER：body 进编辑、toggle 切开关 */
    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        if (phandle->column_focus == DIVOOM_ASTROTOO_ALARM_COL_TOGGLE)
        {
            divoom_astrootoo_alarm_toggle_row(phandle, i);
        }
        else
        {
            divoom_astrootoo_alarm_enter_edit(phandle, row->alarm_index);
        }
        return;
    }

    /* 左键：toggle -> body */
    if (divoom_astrootoo_ui_key_is_left(key_code))
    {
        if (phandle->column_focus == DIVOOM_ASTROTOO_ALARM_COL_TOGGLE)
        {
            phandle->column_focus = DIVOOM_ASTROTOO_ALARM_COL_BODY;
            divoom_astrootoo_alarm_refresh_all_rows(phandle, target);
        }
        return;
    }

    /* 右键：body -> toggle */
    if (divoom_astrootoo_ui_key_is_right(key_code))
    {
        if (phandle->column_focus == DIVOOM_ASTROTOO_ALARM_COL_BODY)
        {
            phandle->column_focus = DIVOOM_ASTROTOO_ALARM_COL_TOGGLE;
            divoom_astrootoo_alarm_refresh_all_rows(phandle, target);
        }
        return;
    }

    /* 上/下：换行（保留当前列焦点）；direct_ctrl 主导，边界环绕（首<->末）与 UI 一致 */
    if (phandle->direct_handle
        && (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code)))
    {
        int go_up = divoom_astrootoo_ui_key_is_up(key_code);
        int direct = go_up ? DIVOOM_MENU_DIRECT_UP : DIVOOM_MENU_DIRECT_DOWN;

        next = divoom_menu_direct_ctrl_get(phandle->direct_handle, target, direct);
        if (!next || next == target)
        {
            int wrap_row = go_up ? (phandle->row_count - 1) : 0;

            if (wrap_row >= 0 && wrap_row < phandle->row_count)
            {
                next = phandle->rows[wrap_row].btn;
            }
        }

        if (next && next != target)
        {
            lv_group_focus_obj(next);
            divoom_astrootoo_ui_focus_scroll(next);
            divoom_astrootoo_alarm_refresh_all_rows(phandle, next);
        }
    }
}

static lv_obj_t *divoom_astrootoo_alarm_create_row(Pdivoom_astrootoo_alarm_ctrl phandle,
    int row_index, int alarm_index)
{
    lv_obj_t *btn;
    lv_obj_t *body_cont;
    lv_obj_t *time_label;
    lv_obj_t *meridiem_label;
    lv_obj_t *repeat_label;
    lv_obj_t *chevron_img;
    lv_obj_t *toggle_cont;
    lv_obj_t *switch_img;
    lv_coord_t y;

    y = row_index * (DIVOOM_ASTRO_ALARM_ROW_H + DIVOOM_ASTRO_ALARM_ROW_GAP);

    btn = lv_btn_create(phandle->content);
    lv_obj_add_style(btn, &phandle->ui.content_style, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_size(btn, DIVOOM_ASTRO_ALARM_LIST_W, DIVOOM_ASTRO_ALARM_ROW_H);
    lv_obj_set_pos(btn, DIVOOM_ASTRO_LIST_PAD_X, y);

    body_cont = lv_obj_create(btn);
    divoom_astrootoo_alarm_apply_slot_style(body_cont, phandle, 0);
    lv_obj_set_size(body_cont, DIVOOM_ASTRO_ALARM_BODY_W, DIVOOM_ASTRO_ALARM_ROW_H);
    lv_obj_set_pos(body_cont, 0, 0);
    lv_obj_clear_flag(body_cont, LV_OBJ_FLAG_SCROLLABLE);

    time_label = lv_label_create(body_cont);
    lv_obj_set_style_text_color(time_label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(time_label,
        divoom_menu_control_get_font_info(DIVOOM_ASTRO_ALARM_TIME_FONT_PX, phandle->menu_id), 0);
    lv_obj_align(time_label, LV_ALIGN_LEFT_MID, 12, 0);

    meridiem_label = lv_label_create(body_cont);
    lv_obj_set_style_text_color(meridiem_label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(meridiem_label,
        divoom_menu_control_get_font_info(DIVOOM_ASTRO_ALARM_MERIDIEM_FONT_PX, phandle->menu_id), 0);
    lv_obj_add_flag(meridiem_label, LV_OBJ_FLAG_HIDDEN);

    chevron_img = lv_img_create(body_cont);
    divoom_astrootoo_ui_apply_sublevel_chevron_size(chevron_img);
    lv_obj_align(chevron_img, LV_ALIGN_RIGHT_MID, -14, 0);

    /* 重复信息：固定宽度 + 文本右对齐 + 右边缘锚定在 chevron 左侧 */
    repeat_label = lv_label_create(body_cont);
    lv_obj_set_style_text_color(repeat_label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(repeat_label,
        divoom_menu_control_get_font_info(DIVOOM_ASTRO_ALARM_REPEAT_FONT_PX, phandle->menu_id), 0);
    lv_obj_set_width(repeat_label, DIVOOM_ASTRO_ALARM_REPEAT_W);
    lv_label_set_long_mode(repeat_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(repeat_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(repeat_label, LV_ALIGN_RIGHT_MID, -(14 + 12 + 8), 0);

    toggle_cont = lv_obj_create(btn);
    divoom_astrootoo_alarm_apply_slot_style(toggle_cont, phandle, 0);
    lv_obj_set_size(toggle_cont, DIVOOM_ASTRO_ALARM_TOGGLE_W, DIVOOM_ASTRO_ALARM_ROW_H);
    lv_obj_set_pos(toggle_cont, DIVOOM_ASTRO_ALARM_BODY_W + DIVOOM_ASTRO_ALARM_COL_GAP, 0);
    lv_obj_clear_flag(toggle_cont, LV_OBJ_FLAG_SCROLLABLE);

    switch_img = lv_img_create(toggle_cont);
    lv_obj_set_size(switch_img, 44, 24);
    lv_obj_center(switch_img);

    phandle->rows[row_index].alarm_index = alarm_index;
    phandle->rows[row_index].btn = btn;
    phandle->rows[row_index].body_cont = body_cont;
    phandle->rows[row_index].time_label = time_label;
    phandle->rows[row_index].meridiem_label = meridiem_label;
    phandle->rows[row_index].repeat_label = repeat_label;
    phandle->rows[row_index].chevron_img = chevron_img;
    phandle->rows[row_index].toggle_cont = toggle_cont;
    phandle->rows[row_index].switch_img = switch_img;

    lv_obj_add_event_cb(btn, divoom_astrootoo_alarm_key_cb, LV_EVENT_KEY, phandle);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_group_add_obj(lv_group_get_default(), btn);
    divoom_menu_direct_ctrl_update(phandle->direct_handle, btn, 0, row_index);
    divoom_astrootoo_alarm_refresh_row_data(phandle, row_index);
    return btn;
}

/* ===================== 子页 / 编辑 ===================== */

static void divoom_astrootoo_alarm_format_repeat_cyc(uint8 cyc, char *buf, int len)
{
    divoom_astrootoo_alarm_format_repeat_summary(cyc, buf, len);
}

/* ---------- 铃声大类 / 条目 映射（对照工程 divoom_lvgl_alarm_edit_menu 逻辑） ---------- */

static const char *divoom_astrootoo_alarm_ring_cat_key[DIVOOM_ASTRO_ALARM_RING_CAT_COUNT] =
{
    "classic", "Rec", "TFCard", "Bluetooth",
};

static int divoom_astrootoo_alarm_ring_cat_has_picker(int cat)
{
    return cat != DIVOOM_ASTRO_RING_CAT_BT;
}

#if 0 /* 唤醒场景条目列表（大类内 6 项）已下架 */
static int divoom_astrootoo_alarm_ring_item_count(int cat)
{
    switch (cat)
    {
    case DIVOOM_ASTRO_RING_CAT_CLASSIC:
        return 12;
    default:
        return 1;
    }
}
#endif

static void divoom_astrootoo_alarm_ring_item_label(int cat, int i, char *buf, int len)
{
    if (!buf || len <= 0)
    {
        return;
    }
    switch (cat)
    {
    case DIVOOM_ASTRO_RING_CAT_CLASSIC:
        if (i >= 0 && i < DIVOOM_ASTRO_ALARM_BELL_COUNT)
        {
            const char *name;

            name = divoom_menu_control_get_disp_str(
                divoom_astrootoo_alarm_classic_ring_keys[i]);
            snprintf(buf, (size_t)len, "%s",
                name ? name : divoom_astrootoo_alarm_classic_ring_keys[i]);
        }
        else
        {
            snprintf(buf, (size_t)len, "%s",
                divoom_menu_control_get_disp_str("STORM"));
        }
        break;
    case DIVOOM_ASTRO_RING_CAT_RECORD:
        snprintf(buf, (size_t)len, "%s", divoom_menu_control_get_disp_str("SleepAidTabRecord"));
        break;
    default:
        snprintf(buf, (size_t)len, "%s", divoom_menu_control_get_disp_str("TFCard"));
        break;
    }
}

static const char *divoom_astrootoo_alarm_tf_disp_basename(const char *path)
{
    const char *base;

    if (path == NULL || path[0] == '\0')
    {
        return "";
    }
    base = path;
    for (; *path; path++)
    {
        if (*path == '/' || *path == '\\')
        {
            base = path + 1;
        }
    }
    return base;
}

static void divoom_astrootoo_alarm_ring_tf_summary(const T_DIVOOM_ALARM_ACTIVE_DO_INFO *info,
    char *buf, int len)
{
    int music_id;
    Pdivoom_tf_music_item tf_item;
    const char *name;

    if (!buf || len <= 0)
    {
        return;
    }
    if (!info || info->mode != DIVOOM_ASTRO_ALARM_TF_MODE || info->leave_url[0] == '\0')
    {
        snprintf(buf, (size_t)len, "%s", divoom_menu_control_get_disp_str("TFCard"));
        return;
    }
    music_id = divoom_tf_music_alarm_resolve_music_id(info->leave_url);
    tf_item = divoom_tf_music_get_item_by_id(music_id);
    if (tf_item != NULL && tf_item->music_name != NULL && tf_item->music_name[0] != '\0')
    {
        name = divoom_astrootoo_alarm_tf_disp_basename(tf_item->music_name);
        snprintf(buf, (size_t)len, "%s", name);
        return;
    }
    name = divoom_astrootoo_alarm_tf_disp_basename(info->leave_url);
    if (name[0] != '\0')
    {
        snprintf(buf, (size_t)len, "%s", name);
    }
    else
    {
        snprintf(buf, (size_t)len, "%s", divoom_menu_control_get_disp_str("TFCard"));
    }
}

static int divoom_astrootoo_alarm_ring_cat_of(const T_DIVOOM_ALARM_ACTIVE_DO_INFO *info)
{
    if (!info)
    {
        return DIVOOM_ASTRO_RING_CAT_CLASSIC;
    }
    if (info->mode == DIVOOM_ASTRO_ALARM_BT_MODE)
    {
        return DIVOOM_ASTRO_RING_CAT_BT;
    }
    if (info->mode == 1)
    {
        return DIVOOM_ASTRO_RING_CAT_RECORD;
    }
    if (info->mode == DIVOOM_ASTRO_ALARM_TF_MODE)
    {
        return DIVOOM_ASTRO_RING_CAT_TF;
    }
    return DIVOOM_ASTRO_RING_CAT_CLASSIC;
}

static int divoom_astrootoo_alarm_ring_item_of(const T_DIVOOM_ALARM_ACTIVE_DO_INFO *info, int cat)
{
    if (!info)
    {
        return 0;
    }
    if (cat == DIVOOM_ASTRO_RING_CAT_CLASSIC)
    {
        return (info->tip_index < 12) ? info->tip_index : 0;
    }
    return 0;
}

/* 编辑行「铃声」摘要：大类 · 条目 */
static void divoom_astrootoo_alarm_ring_line(const T_DIVOOM_ALARM_ACTIVE_DO_INFO *info,
    char *buf, int len)
{
    int cat;
    char item[40];
    const char *cat_name;

    if (!buf || len <= 0)
    {
        return;
    }
    cat = divoom_astrootoo_alarm_ring_cat_of(info);
    cat_name = divoom_menu_control_get_disp_str(divoom_astrootoo_alarm_ring_cat_key[cat]);
    if (cat == DIVOOM_ASTRO_RING_CAT_BT)
    {
        snprintf(buf, (size_t)len, "%s", cat_name ? cat_name : "");
        return;
    }
    if (cat == DIVOOM_ASTRO_RING_CAT_TF)
    {
        divoom_astrootoo_alarm_ring_tf_summary(info, item, (int)sizeof(item));
    }
    else
    {
        divoom_astrootoo_alarm_ring_item_label(cat, divoom_astrootoo_alarm_ring_item_of(info, cat),
            item, (int)sizeof(item));
    }
    snprintf(buf, (size_t)len, "%s . %s", cat_name ? cat_name : "", item);
}

static void divoom_astrootoo_alarm_group_set(lv_obj_t **objs, int n)
{
    lv_group_t *g = lv_group_get_default();
    int i;

    lv_group_remove_all_objs(g);
    for (i = 0; i < n; i++)
    {
        if (objs[i])
        {
            lv_group_add_obj(g, objs[i]);
        }
    }
}

static void divoom_astrootoo_alarm_show_only(Pdivoom_astrootoo_alarm_ctrl phandle, int which)
{
    lv_obj_t *active = NULL;

    if (phandle->list_header)
    {
        if (which == DIVOOM_ASTROTOO_ALARM_MODE_LIST)
        {
            lv_obj_clear_flag(phandle->list_header, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(phandle->list_header, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (phandle->content)
    {
        if (which == DIVOOM_ASTROTOO_ALARM_MODE_LIST)
        {
            lv_obj_clear_flag(phandle->content, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(phandle->content, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (phandle->edit_root)
    {
        lv_obj_add_flag(phandle->edit_root, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->time_root)
    {
        lv_obj_add_flag(phandle->time_root, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->repeat_root)
    {
        lv_obj_add_flag(phandle->repeat_root, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->bell_root)
    {
        lv_obj_add_flag(phandle->bell_root, LV_OBJ_FLAG_HIDDEN);
    }

    switch (which)
    {
    case DIVOOM_ASTROTOO_ALARM_MODE_EDIT:
        active = phandle->edit_root;
        break;
    case DIVOOM_ASTROTOO_ALARM_MODE_TIME:
        active = phandle->time_root;
        break;
    case DIVOOM_ASTROTOO_ALARM_MODE_REPEAT:
        active = phandle->repeat_root;
        break;
    case DIVOOM_ASTROTOO_ALARM_MODE_BELL:
        active = phandle->bell_root;
        break;
    default:
        break;
    }
    if (active)
    {
        lv_obj_clear_flag(active, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(active);
    }
}

static void divoom_astrootoo_alarm_edit_refresh_focus(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    int i;

    for (i = 0; i < 4; i++)
    {
        if (phandle->edit_rows[i])
        {
            divoom_astrootoo_alarm_apply_slot_style(phandle->edit_rows[i], phandle,
                (i == phandle->edit_focus) ? 1 : 0);
        }
        if (i < 3 && phandle->edit_rows[i] && phandle->edit_val[i])
        {
            divoom_astrootoo_ui_list_label_refresh_marquee(phandle->edit_rows[i],
                phandle->edit_val[i]);
        }
    }
}

static void divoom_astrootoo_alarm_edit_refresh_values(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    char buf[64];
    int h;
    int m;
    int i;

    h = phandle->edit_info.hour;
    m = phandle->edit_info.minute;
    if (divoom_main_get_time_flag())
    {
        snprintf(buf, sizeof(buf), "%02d:%02d", h, m);
    }
    else
    {
        int hh = h % 12;
        const char *ap;

        if (hh == 0)
        {
            hh = 12;
        }
        ap = divoom_menu_control_get_disp_str(h < 12 ? "AM" : "PM");
        snprintf(buf, sizeof(buf), "%02d:%02d %s", hh, m, ap ? ap : "");
    }
    if (phandle->edit_val[0])
    {
        lv_label_set_text(phandle->edit_val[0], buf);
    }

    divoom_astrootoo_alarm_format_repeat_cyc(phandle->edit_info.cyc_flag, buf, (int)sizeof(buf));
    if (phandle->edit_val[1])
    {
        lv_label_set_text(phandle->edit_val[1], buf);
    }

    divoom_astrootoo_alarm_ring_line(&phandle->edit_info, buf, (int)sizeof(buf));
    if (phandle->edit_val[2])
    {
        lv_label_set_text(phandle->edit_val[2], buf);
    }

    if (phandle->edit_vol_bar)
    {
        lv_slider_set_value(phandle->edit_vol_bar, phandle->edit_info.volume, LV_ANIM_OFF);
    }
    for (i = 0; i < 3; i++)
    {
        if (phandle->edit_rows[i] && phandle->edit_val[i])
        {
            divoom_astrootoo_ui_list_label_refresh_marquee(phandle->edit_rows[i],
                phandle->edit_val[i]);
        }
    }
}

/* 相对进入编辑页时有改动则自动打开闹钟开关（对照旧版 alarm_edit 保存时默认开启） */
static void divoom_astrootoo_alarm_edit_on_modified(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }
    if (memcmp(&phandle->edit_info, &phandle->edit_info_baseline,
            sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO)) != 0)
    {
        phandle->edit_info.on_off = 1;
    }
}

static void divoom_astrootoo_alarm_tf_catalog_sync(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }
#ifdef DIVOOM_SIMULATOR
    if (divoom_database_manage_get_local_music_count() <= 0 && !divoom_tf_music_sim_is_scanning())
    {
        divoom_tf_music_sim_request_catalog();
    }
#endif
    divoom_tf_music_sync_prepare();
    divoom_tf_music_send_get_music_count();
}

static int divoom_astrootoo_alarm_tf_entry_is_mount(const divoom_tf_entry *entry)
{
    return (entry && entry->is_dir && divoom_tf_music_root_is_mount_label(entry->name)) ? 1 : 0;
}

static int divoom_astrootoo_alarm_tf_visible_total(int folder_id)
{
    divoom_tf_entry entry;
    int raw;
    int off;
    int count;

    if (divoom_database_tf_is_rebuilding())
    {
        return 0;
    }
    if (folder_id != 0)
    {
        return divoom_database_tf_dir_total(folder_id);
    }
    raw = divoom_database_tf_dir_total(0);
    count = 0;
    off = 0;
    while (off < raw)
    {
        if (divoom_database_tf_dir_page(0, off, 1, &entry, 1) <= 0)
        {
            break;
        }
        if (!divoom_astrootoo_alarm_tf_entry_is_mount(&entry))
        {
            count++;
        }
        off++;
    }
    return count;
}

static int divoom_astrootoo_alarm_tf_find_visible_index(int folder_id, int id, int is_dir)
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
        if (folder_id == 0 && divoom_astrootoo_alarm_tf_entry_is_mount(&entry))
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

static void divoom_astrootoo_alarm_tf_fill_dir_page(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    divoom_tf_entry entry;
    int skip;
    int filled;
    int off;
    int raw;

    if (!phandle)
    {
        return;
    }
    memset(phandle->tf_slots, 0, sizeof(phandle->tf_slots));
    if (divoom_database_tf_is_rebuilding())
    {
        phandle->tf_slot_count = 0;
        return;
    }
    if (phandle->tf_folder_id != 0)
    {
        phandle->tf_slot_count = divoom_database_tf_dir_page(phandle->tf_folder_id,
            phandle->tf_window_start, DIVOOM_ASTRO_ALARM_TF_VISIBLE_ROWS,
            phandle->tf_slots, DIVOOM_ASTRO_ALARM_TF_VISIBLE_ROWS);
        return;
    }
    raw = divoom_database_tf_dir_total(0);
    skip = phandle->tf_window_start;
    filled = 0;
    off = 0;
    while (off < raw && filled < DIVOOM_ASTRO_ALARM_TF_VISIBLE_ROWS)
    {
        if (divoom_database_tf_dir_page(0, off, 1, &entry, 1) <= 0)
        {
            break;
        }
        if (divoom_astrootoo_alarm_tf_entry_is_mount(&entry))
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

/* 与 T113 tf_sound_pick 一致：leave_url 只存 TF music_id（如 "3"），不存完整路径 */
static int divoom_astrootoo_alarm_tf_save_track_id(Pdivoom_astrootoo_alarm_ctrl phandle, int index)
{
    int slot;

    if (phandle == NULL || index < 0 || index >= phandle->tf_total)
    {
        return 0;
    }
    slot = index - phandle->tf_window_start;
    if (slot < 0 || slot >= phandle->tf_slot_count
        || phandle->tf_slots[slot].is_dir || phandle->tf_slots[slot].id <= 0)
    {
        return 0;
    }
    snprintf(phandle->edit_info.leave_url, sizeof(phandle->edit_info.leave_url),
             "%d", phandle->tf_slots[slot].id);
    return 1;
}

static void divoom_astrootoo_alarm_tf_catalog_timer_stop(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    if (phandle == NULL || phandle->tf_catalog_timer == NULL)
    {
        return;
    }
    lv_timer_del(phandle->tf_catalog_timer);
    phandle->tf_catalog_timer = NULL;
}

static void divoom_astrootoo_alarm_tf_show_folders(Pdivoom_astrootoo_alarm_ctrl phandle);

static void divoom_astrootoo_alarm_tf_catalog_timer_cb(lv_timer_t *t)
{
    Pdivoom_astrootoo_alarm_ctrl phandle;
    int old_count;

    phandle = (Pdivoom_astrootoo_alarm_ctrl)lv_timer_get_user_data(t);
    if (phandle == NULL || phandle->bell_phase != 4 || phandle->mode != DIVOOM_ASTROTOO_ALARM_MODE_BELL)
    {
        if (phandle)
        {
            divoom_astrootoo_alarm_tf_catalog_timer_stop(phandle);
        }
        return;
    }

#if defined(DIVOOM_TIMES_VIBE)
    divoom_tf_music_poll_songlist_refresh();
    if (divoom_tf_music_take_ui_hotplug_reload())
    {
        divoom_astrootoo_alarm_tf_show_folders(phandle);
        return;
    }
#endif

    old_count = phandle->tf_total;
    phandle->tf_total = divoom_astrootoo_alarm_tf_visible_total(phandle->tf_folder_id);
    if (phandle->tf_total == old_count)
    {
        return;
    }
    if (phandle->tf_logical_focus >= phandle->tf_total && phandle->tf_total > 0)
    {
        phandle->tf_logical_focus = phandle->tf_total - 1;
    }
    divoom_astrootoo_alarm_tf_refresh_window(phandle);
    divoom_astrootoo_alarm_tf_bind_group(phandle);
}

static void divoom_astrootoo_alarm_tf_catalog_timer_start(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    if (phandle == NULL)
    {
        return;
    }
    divoom_astrootoo_alarm_tf_catalog_timer_stop(phandle);
    phandle->tf_catalog_timer = lv_timer_create(divoom_astrootoo_alarm_tf_catalog_timer_cb, 500, phandle);
}

static void divoom_astrootoo_alarm_return_edit(Pdivoom_astrootoo_alarm_ctrl phandle, int focus_row)
{
    if (focus_row < 0 || focus_row > 3)
    {
        focus_row = 0;
    }
    divoom_astrootoo_alarm_tf_catalog_timer_stop(phandle);
    phandle->mode = DIVOOM_ASTROTOO_ALARM_MODE_EDIT;
    phandle->edit_focus = focus_row;
    divoom_astrootoo_alarm_show_only(phandle, DIVOOM_ASTROTOO_ALARM_MODE_EDIT);
    divoom_astrootoo_alarm_edit_refresh_values(phandle);
    divoom_astrootoo_alarm_edit_refresh_focus(phandle);
    divoom_astrootoo_alarm_group_set(phandle->edit_rows, 4);
    if (phandle->edit_rows[focus_row])
    {
        lv_group_focus_obj(phandle->edit_rows[focus_row]);
    }
}

static void divoom_astrootoo_alarm_enter_edit(Pdivoom_astrootoo_alarm_ctrl phandle, int alarm_index)
{
    if (!phandle || alarm_index < 0 || alarm_index >= phandle->alarm_count || !phandle->alarm_list)
    {
        return;
    }

    phandle->edit_index = alarm_index;
    memcpy(&phandle->edit_info, &phandle->alarm_list[alarm_index],
        sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
    memcpy(&phandle->edit_info_baseline, &phandle->edit_info, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
    phandle->rec_pending_relocate = 0;
    divoom_astrootoo_alarm_return_edit(phandle, 0);
}

static void divoom_astrootoo_alarm_edit_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_alarm_ctrl phandle;
    uint8_t key_code;
    lv_obj_t *target;
    int i;
    int found;

    phandle = (Pdivoom_astrootoo_alarm_ctrl)lv_event_get_user_data(e);
    if (!phandle || lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }

    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    target = lv_event_get_target(e);
    found = phandle->edit_focus;
    for (i = 0; i < 4; i++)
    {
        if (phandle->edit_rows[i] == target)
        {
            found = i;
            break;
        }
    }
    phandle->edit_focus = found;

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_astrootoo_alarm_bell_stop_preview();
        divoom_astrootoo_alarm_edit_on_modified(phandle);
        memcpy(&phandle->alarm_list[phandle->edit_index], &phandle->edit_info,
            sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
        divoom_alarm_menu_update_alarm_item(&phandle->alarm_list[phandle->edit_index]);
        divoom_astrootoo_alarm_refresh_row_data(phandle, phandle->edit_index);
        divoom_astrootoo_alarm_enter_list(phandle, phandle->edit_index);
        return;
    }

    if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code))
    {
        int go_up = divoom_astrootoo_ui_key_is_up(key_code);
        int new_focus = go_up ? ((found + 3) % 4) : ((found + 1) % 4);

        if (found == 3 && new_focus != 3)
        {
            divoom_astrootoo_alarm_bell_stop_preview();
        }
        phandle->edit_focus = new_focus;
        if (phandle->edit_rows[phandle->edit_focus])
        {
            lv_group_focus_obj(phandle->edit_rows[phandle->edit_focus]);
        }
        divoom_astrootoo_alarm_edit_refresh_focus(phandle);
        return;
    }

    if (divoom_astrootoo_ui_key_is_left(key_code) || divoom_astrootoo_ui_key_is_right(key_code))
    {
        if (found != 3)
        {
            divoom_astrootoo_alarm_bell_stop_preview();
            return;
        }
        {
            int v = phandle->edit_info.volume;

            v += divoom_astrootoo_ui_key_is_right(key_code) ? 5 : -5;
            if (v < 0)
            {
                v = 0;
            }
            if (v > 100)
            {
                v = 100;
            }
            phandle->edit_info.volume = (T_U8)v;
            divoom_astrootoo_alarm_edit_on_modified(phandle);
            divoom_astrootoo_alarm_edit_refresh_values(phandle);
            divoom_astrootoo_alarm_edit_preview_selected_sound(phandle);
        }
        return;
    }

    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        if (found == 0)
        {
            divoom_astrootoo_alarm_enter_time(phandle);
        }
        else if (found == 1)
        {
            divoom_astrootoo_alarm_enter_repeat(phandle);
        }
        else if (found == 2)
        {
            divoom_astrootoo_alarm_enter_bell(phandle);
        }
    }
}

/* ---------- timePick（对照 flowtooTimePickInput：↑↓调值，←→切焦点） ---------- */

static divoom_astrootoo_time_pick_widgets_t divoom_astrootoo_alarm_time_widgets(
    Pdivoom_astrootoo_alarm_ctrl phandle)
{
    divoom_astrootoo_time_pick_widgets_t widgets;

    widgets.ampm_box = phandle->tp_ampm_box;
    widgets.ampm_lbl = phandle->tp_ampm_lbl;
    widgets.h_box = phandle->tp_h_box;
    widgets.m_box = phandle->tp_m_box;
    widgets.h_lbl = phandle->tp_h_lbl;
    widgets.m_lbl = phandle->tp_m_lbl;
    widgets.h_cap = phandle->tp_h_cap;
    widgets.m_cap = phandle->tp_m_cap;
    widgets.cancel = phandle->tp_cancel;
    widgets.confirm = phandle->tp_confirm;
    widgets.use_hour24 = divoom_astrootoo_time_pick_use_hour24();
    return widgets;
}

static void divoom_astrootoo_alarm_time_refresh_pick(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    divoom_astrootoo_time_pick_widgets_t widgets;

    if (!phandle)
    {
        return;
    }
    widgets = divoom_astrootoo_alarm_time_widgets(phandle);
    divoom_astrootoo_time_pick_refresh_widgets(&widgets, phandle->tp_pick_field,
        phandle->tp_hour, phandle->tp_minute, widgets.use_hour24);
}

static void divoom_astrootoo_alarm_enter_time(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    lv_obj_t *focus_obj;
    divoom_astrootoo_time_pick_widgets_t widgets;

    divoom_astrootoo_alarm_bell_stop_preview();
    phandle->tp_hour = phandle->edit_info.hour;
    phandle->tp_minute = phandle->edit_info.minute;
    phandle->tp_pick_field = divoom_astrootoo_time_pick_initial_field(
        divoom_astrootoo_time_pick_use_hour24());
    phandle->mode = DIVOOM_ASTROTOO_ALARM_MODE_TIME;
    divoom_astrootoo_alarm_show_only(phandle, DIVOOM_ASTROTOO_ALARM_MODE_TIME);
    divoom_astrootoo_alarm_time_refresh_pick(phandle);
    widgets = divoom_astrootoo_alarm_time_widgets(phandle);
    focus_obj = divoom_astrootoo_time_pick_focus_obj(&widgets, phandle->tp_pick_field);
    if (focus_obj)
    {
        lv_group_t *g;
        lv_obj_t *objs[5];
        int cnt;
        int i;

        g = lv_group_get_default();
        lv_group_remove_all_objs(g);
        cnt = 0;
        objs[cnt++] = widgets.ampm_box;
        objs[cnt++] = widgets.h_box;
        objs[cnt++] = widgets.m_box;
        objs[cnt++] = widgets.cancel;
        objs[cnt++] = widgets.confirm;
        for (i = 0; i < cnt; i++)
        {
            if (objs[i] && (objs[i] != widgets.ampm_box || !widgets.use_hour24))
            {
                lv_group_add_obj(g, objs[i]);
            }
        }
        lv_group_focus_obj(focus_obj);
    }
}

static void divoom_astrootoo_alarm_time_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_alarm_ctrl phandle;
    uint8_t key_code;
    lv_obj_t *focus_obj;
    divoom_astrootoo_time_pick_state_t st;
    divoom_astrootoo_time_pick_key_action_t action;
    divoom_astrootoo_time_pick_widgets_t widgets;

    phandle = (Pdivoom_astrootoo_alarm_ctrl)lv_event_get_user_data(e);
    if (!phandle || lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }

    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    st.use_hour24 = divoom_astrootoo_time_pick_use_hour24();
    st.field = phandle->tp_pick_field;
    st.hour24 = phandle->tp_hour;
    st.minute = phandle->tp_minute;
    action = divoom_astrootoo_time_pick_handle_key(&st, key_code, 0);
    phandle->tp_pick_field = st.field;
    phandle->tp_hour = st.hour24;
    phandle->tp_minute = st.minute;

    if (action == DIVOOM_ASTRO_TP_KEY_CANCEL)
    {
        divoom_astrootoo_alarm_return_edit(phandle, 0);
        return;
    }
    if (action == DIVOOM_ASTRO_TP_KEY_CONFIRM)
    {
        phandle->edit_info.hour = phandle->tp_hour;
        phandle->edit_info.minute = phandle->tp_minute;
        divoom_astrootoo_alarm_edit_on_modified(phandle);
        divoom_astrootoo_alarm_return_edit(phandle, 0);
        return;
    }

    divoom_astrootoo_alarm_time_refresh_pick(phandle);
    widgets = divoom_astrootoo_alarm_time_widgets(phandle);
    focus_obj = divoom_astrootoo_time_pick_focus_obj(&widgets, phandle->tp_pick_field);
    if (focus_obj)
    {
        lv_group_focus_obj(focus_obj);
    }
}

/* ---------- repeatPick ---------- */

static void divoom_astrootoo_alarm_repeat_refresh(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    int i;

    for (i = 0; i < 8; i++)
    {
        int active;

        if (i < 7)
        {
            active = (phandle->edit_info.cyc_flag & (1 << ((i + 1) % 7))) ? 1 : 0;
        }
        else
        {
            active = (phandle->edit_info.cyc_flag == 0) ? 1 : 0;
        }
        if (phandle->rp_mark[i])
        {
            if (active)
            {
                lv_obj_clear_flag(phandle->rp_mark[i], LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_obj_add_flag(phandle->rp_mark[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        if (phandle->rp_row[i])
        {
            divoom_astrootoo_alarm_apply_slot_style(phandle->rp_row[i], phandle,
                (i == phandle->rp_focus) ? 1 : 0);
        }
    }
}

static void divoom_astrootoo_alarm_enter_repeat(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    divoom_astrootoo_alarm_bell_stop_preview();
    phandle->rp_focus = 0;
    phandle->mode = DIVOOM_ASTROTOO_ALARM_MODE_REPEAT;
    divoom_astrootoo_alarm_show_only(phandle, DIVOOM_ASTROTOO_ALARM_MODE_REPEAT);
    divoom_astrootoo_alarm_repeat_refresh(phandle);
    divoom_astrootoo_alarm_group_set(phandle->rp_row, 8);
    if (phandle->rp_row[0])
    {
        lv_group_focus_obj(phandle->rp_row[0]);
        divoom_astrootoo_ui_focus_scroll(phandle->rp_row[0]);
    }
}

static void divoom_astrootoo_alarm_repeat_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_alarm_ctrl phandle;
    uint8_t key_code;
    lv_obj_t *target;
    int i;
    int found;

    phandle = (Pdivoom_astrootoo_alarm_ctrl)lv_event_get_user_data(e);
    if (!phandle || lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }

    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    target = lv_event_get_target(e);
    found = phandle->rp_focus;
    for (i = 0; i < 8; i++)
    {
        if (phandle->rp_row[i] == target)
        {
            found = i;
            break;
        }
    }
    phandle->rp_focus = found;

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_astrootoo_alarm_return_edit(phandle, 1);
        return;
    }

    if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code))
    {
        int go_up = divoom_astrootoo_ui_key_is_up(key_code);

        phandle->rp_focus = go_up ? ((found + 7) % 8) : ((found + 1) % 8);
        if (phandle->rp_row[phandle->rp_focus])
        {
            lv_group_focus_obj(phandle->rp_row[phandle->rp_focus]);
            divoom_astrootoo_ui_focus_scroll(phandle->rp_row[phandle->rp_focus]);
        }
        divoom_astrootoo_alarm_repeat_refresh(phandle);
        return;
    }

    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        if (found == 7)
        {
            phandle->edit_info.cyc_flag = 0;
        }
        else
        {
            phandle->edit_info.cyc_flag ^= (uint8)(1 << ((found + 1) % 7));
        }
        divoom_astrootoo_alarm_edit_on_modified(phandle);
        divoom_astrootoo_alarm_repeat_refresh(phandle);
    }
}

/* ---------- bellPick：TF 卡选歌（多层 dir_page，每屏 6 行） ---------- */

static int divoom_astrootoo_alarm_tf_list_total(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    if (phandle == NULL)
    {
        return 0;
    }
    return phandle->tf_total;
}

static void divoom_astrootoo_alarm_tf_set_header_title(Pdivoom_astrootoo_alarm_ctrl phandle, const char *text)
{
    if (phandle && phandle->bell_tf_title && text)
    {
        lv_label_set_text(phandle->bell_tf_title, text);
    }
}

/* 编辑页调音量：按当前已选铃声类型试听（对照 divoom_alarm_edit_preview_selected_sound；TF 铃声不试听） */
static void divoom_astrootoo_alarm_edit_preview_selected_sound(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    char file_name[256];
    T_U8 vol;

    if (phandle == NULL)
    {
        return;
    }
    vol = phandle->edit_info.volume;
#if 0 /* 唤醒场景（mode4）已从铃声页下架 */
    if (phandle->edit_info.mode == 4)
    {
#ifdef DIVOOM_TIME_FRAME_SPEAKER
        divoom_alarm_ctrl_try_listen_scene(phandle->edit_info.scene_index, vol, 1, 1);
#endif
    }
    else
#endif
    if (phandle->edit_info.mode == 1)
    {
        if (phandle->edit_info.leave_url[0] == '\0')
        {
            divoom_astrootoo_alarm_bell_stop_preview();
            return;
        }
        snprintf(file_name, sizeof(file_name), "file://%s", phandle->edit_info.leave_url);
        divoom_local_music_ctrl_tone_play(file_name, vol, NULL, DIVOOM_ASTRO_ALARM_TF_PREVIEW_SEC);
    }
    else if (phandle->edit_info.mode == DIVOOM_ASTRO_ALARM_BT_MODE)
    {
        divoom_astrootoo_alarm_bell_stop_preview();
        return;
    }
    else if (phandle->edit_info.mode == DIVOOM_ASTRO_ALARM_TF_MODE)
    {
        /* TimesVibe/T113：TF 卡铃声不在闹钟 UI 内试听 */
        divoom_astrootoo_alarm_bell_stop_preview();
        return;
    }
    else
    {
        if (phandle->edit_info.tip_index >= DIVOOM_ASTRO_ALARM_BELL_COUNT)
        {
            return;
        }
        divoom_alarm_ctrl_try_listen((T_U8)phandle->edit_info.tip_index, vol, 1, 1);
    }
}

static void divoom_astrootoo_alarm_tf_refresh_window(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    int i;
    int total;
    int sel_music_id;

    if (phandle == NULL)
    {
        return;
    }

    phandle->tf_total = divoom_astrootoo_alarm_tf_visible_total(phandle->tf_folder_id);

    total = divoom_astrootoo_alarm_tf_list_total(phandle);
    sel_music_id = 0;
    if (phandle->edit_info.mode == DIVOOM_ASTRO_ALARM_TF_MODE
        && phandle->edit_info.leave_url[0] != '\0')
    {
        sel_music_id = divoom_tf_music_alarm_resolve_music_id(phandle->edit_info.leave_url);
    }

    if (total <= 0)
    {
        phandle->tf_window_start = 0;
        phandle->tf_logical_focus = 0;
    }
    else
    {
        if (phandle->tf_logical_focus < 0)
        {
            phandle->tf_logical_focus = 0;
        }
        if (phandle->tf_logical_focus >= total)
        {
            phandle->tf_logical_focus = total - 1;
        }
        if (phandle->tf_logical_focus < phandle->tf_window_start)
        {
            phandle->tf_window_start = phandle->tf_logical_focus;
        }
        if (phandle->tf_logical_focus >= phandle->tf_window_start + DIVOOM_ASTRO_ALARM_TF_VISIBLE_ROWS)
        {
            phandle->tf_window_start = phandle->tf_logical_focus - DIVOOM_ASTRO_ALARM_TF_VISIBLE_ROWS + 1;
        }
    }

    divoom_astrootoo_alarm_tf_fill_dir_page(phandle);
    divoom_astrootoo_alarm_tf_apply_row_icons(phandle);

    for (i = 0; i < DIVOOM_ASTRO_ALARM_TF_VISIBLE_ROWS; i++)
    {
        int show_chevron;
        int show_check;
        const char *label;

        if (!phandle->bell_tf_row[i])
        {
            continue;
        }
        if (i >= phandle->tf_slot_count)
        {
            lv_obj_add_flag(phandle->bell_tf_row[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        lv_obj_clear_flag(phandle->bell_tf_row[i], LV_OBJ_FLAG_HIDDEN);
        show_chevron = phandle->tf_slots[i].is_dir ? 1 : 0;
        show_check = 0;
        label = phandle->tf_slots[i].name[0] ? phandle->tf_slots[i].name : "?";
        if (phandle->bell_tf_label[i])
        {
            lv_label_set_text(phandle->bell_tf_label[i], label);
        }
        if (!phandle->tf_slots[i].is_dir && sel_music_id > 0
            && phandle->tf_slots[i].id == sel_music_id)
        {
            show_check = 1;
        }

        if (phandle->bell_tf_chevron[i])
        {
            if (show_chevron)
            {
                lv_obj_clear_flag(phandle->bell_tf_chevron[i], LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_obj_add_flag(phandle->bell_tf_chevron[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        if (phandle->bell_tf_mark[i])
        {
            if (show_check)
            {
                lv_obj_clear_flag(phandle->bell_tf_mark[i], LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_obj_add_flag(phandle->bell_tf_mark[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        if (phandle->bell_tf_label[i])
        {
            divoom_astrootoo_ui_tf_label_refresh_long_mode(phandle->bell_tf_label[i]);
        }
    }

    i = phandle->tf_logical_focus - phandle->tf_window_start;
    if (i >= 0 && i < DIVOOM_ASTRO_ALARM_TF_VISIBLE_ROWS && phandle->bell_tf_row[i])
    {
        lv_group_focus_obj(phandle->bell_tf_row[i]);
        divoom_astrootoo_ui_focus_scroll(phandle->bell_tf_row[i]);
    }
}

static void divoom_astrootoo_alarm_tf_bind_group(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    lv_group_t *g;
    int i;
    int vis;

    g = lv_group_get_default();
    lv_group_remove_all_objs(g);
    vis = 0;
    for (i = 0; i < DIVOOM_ASTRO_ALARM_TF_VISIBLE_ROWS; i++)
    {
        if (phandle->bell_tf_row[i] && !lv_obj_has_flag(phandle->bell_tf_row[i], LV_OBJ_FLAG_HIDDEN))
        {
            lv_group_add_obj(g, phandle->bell_tf_row[i]);
            vis++;
        }
    }
    if (vis == 0 && phandle->bell_tf_empty_btn)
    {
        lv_obj_clear_flag(phandle->bell_tf_empty_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(phandle->bell_tf_empty_btn);
        lv_group_add_obj(g, phandle->bell_tf_empty_btn);
        lv_group_focus_obj(phandle->bell_tf_empty_btn);
    }
    else if (phandle->bell_tf_empty_btn)
    {
        lv_obj_add_flag(phandle->bell_tf_empty_btn, LV_OBJ_FLAG_HIDDEN);
    }
    if (vis > 0)
    {
        i = phandle->tf_logical_focus - phandle->tf_window_start;
        if (i >= 0 && i < DIVOOM_ASTRO_ALARM_TF_VISIBLE_ROWS
            && phandle->bell_tf_row[i]
            && !lv_obj_has_flag(phandle->bell_tf_row[i], LV_OBJ_FLAG_HIDDEN))
        {
            lv_group_focus_obj(phandle->bell_tf_row[i]);
        }
    }
}

static void divoom_astrootoo_alarm_tf_apply_row_icons(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    int i;
    const char *icon_path;

    for (i = 0; i < DIVOOM_ASTRO_ALARM_TF_VISIBLE_ROWS; i++)
    {
        if (!phandle->bell_tf_icon[i])
        {
            continue;
        }
        if (i < phandle->tf_slot_count && phandle->tf_slots[i].is_dir)
        {
            icon_path = "23/at_tf_folder.png";
        }
        else
        {
            icon_path = "23/at_tf_music_note.png";
        }
        divoom_menu_control_lv_img_set_src(phandle->bell_tf_icon[i], icon_path);
    }
}

static void divoom_astrootoo_alarm_tf_refresh_title(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    divoom_tf_folder_info info;

    if (phandle->tf_folder_id <= 0)
    {
        divoom_astrootoo_alarm_tf_set_header_title(phandle,
            divoom_menu_control_get_disp_str("LocalMusic"));
        return;
    }
    memset(&info, 0, sizeof(info));
    if (divoom_database_tf_folder_info(phandle->tf_folder_id, &info) && info.name[0])
    {
        divoom_astrootoo_alarm_tf_set_header_title(phandle, info.name);
        return;
    }
    divoom_astrootoo_alarm_tf_set_header_title(phandle, "?");
}

static void divoom_astrootoo_alarm_tf_show_folders(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    phandle->tf_folder_id = 0;
    phandle->tf_folder_depth = 0;
    memset(phandle->tf_folder_stack, 0, sizeof(phandle->tf_folder_stack));
    phandle->tf_window_start = 0;
    phandle->tf_logical_focus = 0;
    divoom_astrootoo_alarm_tf_refresh_title(phandle);
    divoom_astrootoo_alarm_tf_refresh_window(phandle);
    divoom_astrootoo_alarm_tf_bind_group(phandle);
}

static void divoom_astrootoo_alarm_tf_enter_folder(Pdivoom_astrootoo_alarm_ctrl phandle, int folder_id)
{
    if (!phandle || folder_id <= 0)
    {
        return;
    }
    if (phandle->tf_folder_depth < DIVOOM_ASTRO_ALARM_TF_FOLDER_STACK)
    {
        phandle->tf_folder_stack[phandle->tf_folder_depth++] = phandle->tf_folder_id;
    }
    phandle->tf_folder_id = folder_id;
    phandle->tf_window_start = 0;
    phandle->tf_logical_focus = 0;
    divoom_astrootoo_alarm_tf_refresh_title(phandle);
    divoom_astrootoo_alarm_tf_refresh_window(phandle);
    divoom_astrootoo_alarm_tf_bind_group(phandle);
}

static int divoom_astrootoo_alarm_tf_go_up(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    int left_id;
    int idx;

    if (!phandle || (phandle->tf_folder_depth <= 0 && phandle->tf_folder_id <= 0))
    {
        return 0;
    }
    left_id = phandle->tf_folder_id;
    if (phandle->tf_folder_depth > 0)
    {
        phandle->tf_folder_id = phandle->tf_folder_stack[--phandle->tf_folder_depth];
    }
    else
    {
        phandle->tf_folder_id = 0;
    }
    phandle->tf_window_start = 0;
    phandle->tf_logical_focus = 0;
    idx = divoom_astrootoo_alarm_tf_find_visible_index(phandle->tf_folder_id, left_id, 1);
    if (idx >= 0)
    {
        phandle->tf_logical_focus = idx;
    }
    divoom_astrootoo_alarm_tf_refresh_title(phandle);
    divoom_astrootoo_alarm_tf_refresh_window(phandle);
    divoom_astrootoo_alarm_tf_bind_group(phandle);
    return 1;
}

static void divoom_astrootoo_alarm_bell_hide_tf_siblings(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    if (phandle->bell_cat_list)
    {
        lv_obj_add_flag(phandle->bell_cat_list, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->bell_item_list)
    {
        lv_obj_add_flag(phandle->bell_item_list, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->bell_grid_list)
    {
        lv_obj_add_flag(phandle->bell_grid_list, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->bell_record_list)
    {
        lv_obj_add_flag(phandle->bell_record_list, LV_OBJ_FLAG_HIDDEN);
    }
}

static void divoom_astrootoo_alarm_bell_show_tf(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    divoom_astrootoo_alarm_tf_catalog_sync(phandle);
    phandle->bell_phase = 4;
    phandle->bell_cur_cat = DIVOOM_ASTRO_RING_CAT_TF;
    divoom_astrootoo_alarm_bell_hide_tf_siblings(phandle);
    if (phandle->bell_tf_root)
    {
        lv_obj_clear_flag(phandle->bell_tf_root, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(phandle->bell_tf_root);
    }
    divoom_astrootoo_alarm_tf_show_folders(phandle);
    divoom_astrootoo_alarm_tf_catalog_timer_start(phandle);
}

static void divoom_astrootoo_alarm_bell_tf_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_alarm_ctrl phandle;
    uint8_t key_code;
    int total;
    int focus;

    phandle = (Pdivoom_astrootoo_alarm_ctrl)lv_event_get_user_data(e);
    if (!phandle || lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }

    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    total = divoom_astrootoo_alarm_tf_list_total(phandle);
    focus = phandle->tf_logical_focus;

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_astrootoo_alarm_bell_stop_preview();
        if (divoom_astrootoo_alarm_tf_go_up(phandle))
        {
            return;
        }
        phandle->bell_cat_focus = DIVOOM_ASTRO_RING_CAT_TF;
        divoom_astrootoo_alarm_bell_show_cats(phandle);
        return;
    }

    if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code))
    {
        int go_up;
        int n;

        if (total <= 0)
        {
            return;
        }
        go_up = divoom_astrootoo_ui_key_is_up(key_code);
        n = total;
        phandle->tf_logical_focus = go_up ? ((focus + n - 1) % n) : ((focus + 1) % n);
        divoom_astrootoo_alarm_tf_refresh_window(phandle);
        return;
    }

    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        int slot;

        if (total <= 0 || focus < 0 || focus >= total)
        {
            return;
        }
        slot = focus - phandle->tf_window_start;
        if (slot < 0 || slot >= phandle->tf_slot_count)
        {
            return;
        }
        if (phandle->tf_slots[slot].is_dir)
        {
            divoom_astrootoo_alarm_tf_enter_folder(phandle, phandle->tf_slots[slot].id);
            return;
        }
        if (!divoom_astrootoo_alarm_tf_save_track_id(phandle, focus))
        {
            return;
        }
        phandle->edit_info.mode = DIVOOM_ASTRO_ALARM_TF_MODE;
        divoom_astrootoo_alarm_edit_on_modified(phandle);
        phandle->bell_cat_focus = DIVOOM_ASTRO_RING_CAT_TF;
        divoom_astrootoo_alarm_bell_show_cats(phandle);
    }
}

/* ---------- bellPick：大类 -> 条目 两级 ---------- */

static void divoom_astrootoo_alarm_bell_stop_preview(void)
{
    divoom_alarm_ctrl_preview_stop();
    /* 场景试听可能未置 TRY_LISTEN，须无条件停 tone（对照 AlarmScreen stopAlarmRingtonePreview） */
    divoom_local_music_ctrl_stop_tone();
    /* 不对 TF 背景播放 pause_jl（TimesVibe TF 铃声无 picker 试听；见 T113 alarm_edit TF pick） */
}

/* 焦点项试听（不写入草稿；与 divoom_lvgl_alarm_edit_menu 方向键切换行为一致） */
static void divoom_astrootoo_alarm_bell_preview_item(Pdivoom_astrootoo_alarm_ctrl phandle, int cat, int item)
{
    if (phandle == NULL)
    {
        return;
    }
    if (cat == DIVOOM_ASTRO_RING_CAT_CLASSIC && item >= 0 && item < DIVOOM_ASTRO_ALARM_BELL_COUNT)
    {
        divoom_alarm_ctrl_try_listen((T_U8)item, phandle->edit_info.volume, 1, 1);
    }
}

static void divoom_astrootoo_alarm_bell_cat_bind_row(Pdivoom_astrootoo_alarm_ctrl phandle, int i);

static void divoom_astrootoo_alarm_bell_refresh_cats(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    int cur = divoom_astrootoo_alarm_ring_cat_of(&phandle->edit_info);
    int i;

    for (i = 0; i < DIVOOM_ASTRO_ALARM_RING_CAT_COUNT; i++)
    {
        if (phandle->bell_cat_mark[i])
        {
            if (i == cur)
            {
                lv_obj_clear_flag(phandle->bell_cat_mark[i], LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_obj_add_flag(phandle->bell_cat_mark[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        if (phandle->bell_cat_val[i])
        {
            if (i == cur)
            {
                char item[40];

                if (i == DIVOOM_ASTRO_RING_CAT_BT)
                {
                    lv_label_set_text(phandle->bell_cat_val[i], "");
                }
                else if (i == DIVOOM_ASTRO_RING_CAT_TF)
                {
                    divoom_astrootoo_alarm_ring_tf_summary(&phandle->edit_info, item, (int)sizeof(item));
                    lv_label_set_text(phandle->bell_cat_val[i], item);
                }
                else if (i == DIVOOM_ASTRO_RING_CAT_RECORD)
                {
                    divoom_astrootoo_alarm_rec_cat_summary(phandle, item, (int)sizeof(item));
                    lv_label_set_text(phandle->bell_cat_val[i], item);
                }
                else
                {
                    divoom_astrootoo_alarm_ring_item_label(i,
                        divoom_astrootoo_alarm_ring_item_of(&phandle->edit_info, i),
                        item, (int)sizeof(item));
                    lv_label_set_text(phandle->bell_cat_val[i], item);
                }
                divoom_astrootoo_alarm_bell_cat_bind_row(phandle, i);
                divoom_astrootoo_ui_list_label_refresh_marquee(phandle->bell_cat_row[i],
                    phandle->bell_cat_val[i]);
            }
            else
            {
                lv_label_set_text(phandle->bell_cat_val[i], "");
                divoom_astrootoo_alarm_bell_cat_bind_row(phandle, i);
                divoom_astrootoo_ui_list_label_refresh_marquee(phandle->bell_cat_row[i],
                    phandle->bell_cat_val[i]);
            }
        }
        if (phandle->bell_cat_row[i])
        {
            divoom_astrootoo_alarm_apply_slot_style(phandle->bell_cat_row[i], phandle,
                (i == phandle->bell_cat_focus) ? 1 : 0);
        }
        if (phandle->bell_cat_chevron[i])
        {
            if (divoom_astrootoo_alarm_ring_cat_has_picker(i))
            {
                lv_obj_clear_flag(phandle->bell_cat_chevron[i], LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_obj_add_flag(phandle->bell_cat_chevron[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

#if 0 /* 唤醒场景二级条目列表 */
static void divoom_astrootoo_alarm_bell_refresh_items(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    int cat = phandle->bell_cur_cat;
    int cnt = divoom_astrootoo_alarm_ring_item_count(cat);
    int sel = (cat == divoom_astrootoo_alarm_ring_cat_of(&phandle->edit_info))
        ? divoom_astrootoo_alarm_ring_item_of(&phandle->edit_info, cat) : -1;
    int i;

    for (i = 0; i < DIVOOM_ASTRO_ALARM_RING_ITEM_MAX; i++)
    {
        if (!phandle->bell_item_row[i])
        {
            continue;
        }
        if (i >= cnt)
        {
            lv_obj_add_flag(phandle->bell_item_row[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        lv_obj_clear_flag(phandle->bell_item_row[i], LV_OBJ_FLAG_HIDDEN);
        if (phandle->bell_item_label[i])
        {
            char item[40];

            divoom_astrootoo_alarm_ring_item_label(cat, i, item, (int)sizeof(item));
            lv_label_set_text(phandle->bell_item_label[i], item);
        }
        if (phandle->bell_item_mark[i])
        {
            if (i == sel)
            {
                lv_obj_clear_flag(phandle->bell_item_mark[i], LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_obj_add_flag(phandle->bell_item_mark[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        divoom_astrootoo_alarm_apply_slot_style(phandle->bell_item_row[i], phandle,
            (i == phandle->bell_item_focus) ? 1 : 0);
    }
}

static void divoom_astrootoo_alarm_bell_show_items(Pdivoom_astrootoo_alarm_ctrl phandle, int cat)
{
    int cnt;

    phandle->bell_phase = 1;
    phandle->bell_cur_cat = cat;
    cnt = divoom_astrootoo_alarm_ring_item_count(cat);
    phandle->bell_item_focus = (cat == divoom_astrootoo_alarm_ring_cat_of(&phandle->edit_info))
        ? divoom_astrootoo_alarm_ring_item_of(&phandle->edit_info, cat) : 0;
    if (phandle->bell_item_focus < 0 || phandle->bell_item_focus >= cnt)
    {
        phandle->bell_item_focus = 0;
    }
    if (phandle->bell_cat_list)
    {
        lv_obj_add_flag(phandle->bell_cat_list, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->bell_grid_list)
    {
        lv_obj_add_flag(phandle->bell_grid_list, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->bell_record_list)
    {
        lv_obj_add_flag(phandle->bell_record_list, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->bell_tf_root)
    {
        lv_obj_add_flag(phandle->bell_tf_root, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->bell_item_list)
    {
        lv_obj_clear_flag(phandle->bell_item_list, LV_OBJ_FLAG_HIDDEN);
    }
    divoom_astrootoo_alarm_bell_refresh_items(phandle);
    divoom_astrootoo_alarm_group_set(phandle->bell_item_row, cnt);
    if (phandle->bell_item_row[phandle->bell_item_focus])
    {
        lv_group_focus_obj(phandle->bell_item_row[phandle->bell_item_focus]);
        divoom_astrootoo_ui_focus_scroll(phandle->bell_item_row[phandle->bell_item_focus]);
    }
}
#endif

static void divoom_astrootoo_alarm_bell_show_cats(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    divoom_astrootoo_alarm_bell_stop_preview();
    divoom_astrootoo_alarm_tf_catalog_timer_stop(phandle);
    phandle->bell_phase = 0;
    if (phandle->bell_item_list)
    {
        lv_obj_add_flag(phandle->bell_item_list, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->bell_grid_list)
    {
        lv_obj_add_flag(phandle->bell_grid_list, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->bell_record_list)
    {
        lv_obj_add_flag(phandle->bell_record_list, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->bell_tf_root)
    {
        lv_obj_add_flag(phandle->bell_tf_root, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->bell_cat_list)
    {
        lv_obj_clear_flag(phandle->bell_cat_list, LV_OBJ_FLAG_HIDDEN);
    }
    divoom_astrootoo_alarm_bell_refresh_cats(phandle);
    divoom_astrootoo_alarm_group_set(phandle->bell_cat_row, DIVOOM_ASTRO_ALARM_RING_CAT_COUNT);
    if (phandle->bell_cat_row[phandle->bell_cat_focus])
    {
        lv_group_focus_obj(phandle->bell_cat_row[phandle->bell_cat_focus]);
        divoom_astrootoo_ui_focus_scroll(phandle->bell_cat_row[phandle->bell_cat_focus]);
    }
}

/* ---------- 经典铃声图标网格（3×4） ---------- */

static void divoom_astrootoo_alarm_bell_grid_refresh(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    int sel = (divoom_astrootoo_alarm_ring_cat_of(&phandle->edit_info) == DIVOOM_ASTRO_RING_CAT_CLASSIC)
        ? divoom_astrootoo_alarm_ring_item_of(&phandle->edit_info, DIVOOM_ASTRO_RING_CAT_CLASSIC) : -1;
    int i;

    for (i = 0; i < DIVOOM_ASTRO_ALARM_BELL_COUNT; i++)
    {
        if (phandle->bell_grid_check[i])
        {
            if (i == sel)
            {
                lv_obj_clear_flag(phandle->bell_grid_check[i], LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_obj_add_flag(phandle->bell_grid_check[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        if (phandle->bell_grid_btn[i])
        {
            divoom_astrootoo_alarm_apply_slot_style(phandle->bell_grid_btn[i], phandle,
                (i == phandle->bell_grid_focus) ? 1 : 0);
        }
    }
}

static void divoom_astrootoo_alarm_bell_show_grid(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    phandle->bell_phase = 2;
    phandle->bell_cur_cat = DIVOOM_ASTRO_RING_CAT_CLASSIC;
    phandle->bell_grid_focus = (divoom_astrootoo_alarm_ring_cat_of(&phandle->edit_info) == DIVOOM_ASTRO_RING_CAT_CLASSIC)
        ? divoom_astrootoo_alarm_ring_item_of(&phandle->edit_info, DIVOOM_ASTRO_RING_CAT_CLASSIC) : 0;
    if (phandle->bell_grid_focus < 0 || phandle->bell_grid_focus >= DIVOOM_ASTRO_ALARM_BELL_COUNT)
    {
        phandle->bell_grid_focus = 0;
    }
    if (phandle->bell_cat_list)
    {
        lv_obj_add_flag(phandle->bell_cat_list, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->bell_item_list)
    {
        lv_obj_add_flag(phandle->bell_item_list, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->bell_record_list)
    {
        lv_obj_add_flag(phandle->bell_record_list, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->bell_tf_root)
    {
        lv_obj_add_flag(phandle->bell_tf_root, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->bell_grid_list)
    {
        lv_obj_clear_flag(phandle->bell_grid_list, LV_OBJ_FLAG_HIDDEN);
    }
    divoom_astrootoo_alarm_bell_grid_refresh(phandle);
    divoom_astrootoo_alarm_group_set(phandle->bell_grid_btn, DIVOOM_ASTRO_ALARM_BELL_COUNT);
    if (phandle->bell_grid_btn[phandle->bell_grid_focus])
    {
        lv_group_focus_obj(phandle->bell_grid_btn[phandle->bell_grid_focus]);
        divoom_astrootoo_ui_focus_scroll(phandle->bell_grid_btn[phandle->bell_grid_focus]);
    }
    divoom_astrootoo_alarm_bell_preview_item(phandle, DIVOOM_ASTRO_RING_CAT_CLASSIC, phandle->bell_grid_focus);
}

/* ---------- 录音页（真实 MIC 录音 + tone 试听） ---------- */

static void divoom_astrootoo_alarm_rec_get_path(Pdivoom_astrootoo_alarm_ctrl phandle, char *path, int path_len)
{
	if (!phandle || !path || path_len <= 0)
	{
		return;
	}
	divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_ALARM_RECORD,
		(uint32)phandle->edit_info.alarm_id, path);
}

static int divoom_astrootoo_alarm_rec_file_valid(Pdivoom_astrootoo_alarm_ctrl phandle)
{
	char path[256];
	int sz;

	if (!phandle)
	{
		return 0;
	}
	divoom_astrootoo_alarm_rec_get_path(phandle, path, (int)sizeof(path));
	if (!divoom_port_file_exists(path))
	{
		return 0;
	}
	sz = divoom_app_com_get_file_size(path);
	return sz >= DIVOOM_ASTRO_ALARM_REC_MIN_BYTES;
}

static Pdivoom_astrootoo_alarm_ctrl s_alarm_rec_owner;

static uint32 divoom_astrootoo_alarm_rec_wall_sec(Pdivoom_astrootoo_alarm_ctrl phandle)
{
	uint32 now;

	if (!phandle || phandle->rec_start_tick_ms == 0u)
	{
		return 0;
	}
	now = divoom_system_get_tickms();
	return (now - phandle->rec_start_tick_ms) / 1000u;
}

static uint32 divoom_astrootoo_alarm_rec_duration_sec(Pdivoom_astrootoo_alarm_ctrl phandle)
{
	char path[256];

	if (!divoom_astrootoo_alarm_rec_file_valid(phandle))
	{
		return 0;
	}
	divoom_astrootoo_alarm_rec_get_path(phandle, path, (int)sizeof(path));
	return divoom_mic_record_file_duration_sec(path);
}

static void divoom_astrootoo_alarm_rec_done_cb(const char *path, uint32 duration_sec, int result)
{
	Pdivoom_astrootoo_alarm_ctrl phandle = s_alarm_rec_owner;

	(void)path;
	(void)result;
	if (!phandle)
	{
		return;
	}
	phandle->rec_enc_sec = duration_sec > 0u ? duration_sec : 1u;
	if (phandle->rec_enc_sec > DIVOOM_ASTRO_ALARM_REC_MAX_SEC)
	{
		phandle->rec_enc_sec = DIVOOM_ASTRO_ALARM_REC_MAX_SEC;
	}
}

static uint32 divoom_astrootoo_alarm_rec_live_sec(Pdivoom_astrootoo_alarm_ctrl phandle)
{
	uint32 sec;

	if (!phandle)
	{
		return 0;
	}
	if (phandle->rec_enc_sec > 0u)
	{
		sec = phandle->rec_enc_sec;
	}
	else if (phandle->rec_start_tick_ms != 0u)
	{
		sec = divoom_astrootoo_alarm_rec_wall_sec(phandle);
	}
	else
	{
		sec = phandle->rec_elapsed_sec;
	}
	if (sec == 0u)
	{
		sec = 1u;
	}
	if (sec > DIVOOM_ASTRO_ALARM_REC_MAX_SEC)
	{
		sec = DIVOOM_ASTRO_ALARM_REC_MAX_SEC;
	}
	return sec;
}

static int divoom_astrootoo_alarm_rec_commit(Pdivoom_astrootoo_alarm_ctrl phandle);

static void divoom_astrootoo_alarm_rec_freeze_and_commit(Pdivoom_astrootoo_alarm_ctrl phandle)
{
	if (!phandle)
	{
		return;
	}
	phandle->rec_elapsed_sec = divoom_astrootoo_alarm_rec_live_sec(phandle);
	if (phandle->rec_start_tick_ms != 0u)
	{
		phandle->rec_start_tick_ms = 0;
		divoom_astrootoo_alarm_rec_commit(phandle);
	}
	else if (phandle->rec_enc_sec > 0u && phandle->rec_has_clip)
	{
		phandle->rec_clip_sec = phandle->rec_elapsed_sec;
	}
}

static void divoom_astrootoo_alarm_rec_stop_play(Pdivoom_astrootoo_alarm_ctrl phandle)
{
	if (!phandle)
	{
		return;
	}
	if (phandle->rec_playing)
	{
		phandle->rec_playing = 0;
		divoom_local_music_ctrl_stop_tone();
	}
}

static void divoom_astrootoo_alarm_rec_stop_capture(Pdivoom_astrootoo_alarm_ctrl phandle)
{
	if (!phandle)
	{
		return;
	}
	if (divoom_local_music_ctrl_voice_clip_record_is_active())
	{
		divoom_local_music_ctrl_voice_clip_record_stop_sync();
	}
	phandle->rec_capture_active = 0;
}

static void divoom_astrootoo_alarm_rec_relocate_placeholder(int new_alarm_id)
{
	char placeholder[256];
	char dest[256];
	char dirpath[256];
	int sz;

	if (new_alarm_id <= 0)
	{
		return;
	}
	divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_ALARM_RECORD, 0, placeholder);
	if (!divoom_port_file_exists(placeholder))
	{
		return;
	}
	sz = divoom_app_com_get_file_size(placeholder);
	if (sz < DIVOOM_ASTRO_ALARM_REC_MIN_BYTES)
	{
		return;
	}
	divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_ALARM_RECORD,
		(uint32)new_alarm_id, dest);
	if (strcmp(placeholder, dest) == 0)
	{
		return;
	}
	divoom_manage_image_download_get_path_name(DIVOOM_MANAGE_IMAGE_ALARM_RECORD, 0, dirpath);
	divoom_app_com_check_and_mk_dir(dirpath);
	divoom_app_com_copy_file(placeholder, dest);
	if (!divoom_port_file_exists(dest)
		|| divoom_app_com_get_file_size(dest) < DIVOOM_ASTRO_ALARM_REC_MIN_BYTES)
	{
		return;
	}
	divoom_app_com_del_file(placeholder);
}

static int divoom_astrootoo_alarm_rec_commit(Pdivoom_astrootoo_alarm_ctrl phandle)
{
	char dest[256];
	char dirpath[256];
	int tmp_sz;
	int dest_sz;

	if (!phandle)
	{
		return -1;
	}
	tmp_sz = divoom_app_com_get_file_size((char *)DIVOOM_ASTRO_ALARM_REC_TMP_MP3);
	if (tmp_sz < DIVOOM_ASTRO_ALARM_REC_MIN_BYTES)
	{
		divoom_app_com_del_file((char *)DIVOOM_ASTRO_ALARM_REC_TMP_MP3);
		return -1;
	}
	divoom_manage_image_download_get_path_name(DIVOOM_MANAGE_IMAGE_ALARM_RECORD, 0, dirpath);
	divoom_app_com_check_and_mk_dir(dirpath);
	divoom_astrootoo_alarm_rec_get_path(phandle, dest, (int)sizeof(dest));
	divoom_app_com_copy_file((char *)DIVOOM_ASTRO_ALARM_REC_TMP_MP3, dest);
	divoom_app_com_del_file((char *)DIVOOM_ASTRO_ALARM_REC_TMP_MP3);
	if (!divoom_port_file_exists(dest))
	{
		return -1;
	}
	dest_sz = divoom_app_com_get_file_size(dest);
	if (dest_sz < DIVOOM_ASTRO_ALARM_REC_MIN_BYTES)
	{
		divoom_app_com_del_file(dest);
		return -1;
	}
	phandle->rec_has_clip = 1;
	phandle->rec_pending_relocate = (phandle->edit_info.alarm_id == 0) ? 1 : 0;
	if (phandle->rec_elapsed_sec > 0u)
	{
		phandle->rec_clip_sec = phandle->rec_elapsed_sec;
	}
	else
	{
		phandle->rec_clip_sec = divoom_astrootoo_alarm_rec_duration_sec(phandle);
	}
	if (phandle->rec_clip_sec == 0)
	{
		phandle->rec_clip_sec = 1;
	}
	divoom_astrootoo_alarm_rec_stamp_clip_date_now(phandle);
	return 0;
}

static int divoom_astrootoo_alarm_rec_start_capture(Pdivoom_astrootoo_alarm_ctrl phandle)
{
	if (!phandle)
	{
		return -1;
	}
	divoom_astrootoo_alarm_rec_stop_play(phandle);
	divoom_astrootoo_alarm_rec_stop_capture(phandle);
	divoom_app_com_del_file((char *)DIVOOM_ASTRO_ALARM_REC_TMP_MP3);
	s_alarm_rec_owner = phandle;
	phandle->rec_start_tick_ms = divoom_system_get_tickms();
	phandle->rec_enc_sec = 0;
	if (divoom_local_music_ctrl_voice_clip_record_start(DIVOOM_ASTRO_ALARM_REC_TMP_MP3, 0,
							    divoom_astrootoo_alarm_rec_done_cb) != 0)
	{
		s_alarm_rec_owner = NULL;
		return -1;
	}
	phandle->rec_capture_active = 1;
	phandle->rec_elapsed_sec = 0;
	return 0;
}

static void divoom_astrootoo_alarm_rec_start_play(Pdivoom_astrootoo_alarm_ctrl phandle)
{
	char path[256];
	uint32 sec;

	if (!phandle || !divoom_astrootoo_alarm_rec_file_valid(phandle))
	{
		return;
	}
	divoom_astrootoo_alarm_rec_stop_capture(phandle);
	divoom_astrootoo_alarm_bell_stop_preview();
	divoom_astrootoo_alarm_rec_get_path(phandle, path, (int)sizeof(path));
	sec = divoom_astrootoo_alarm_rec_duration_sec(phandle);
	if (sec <= 0)
	{
		sec = 1;
	}
	phandle->rec_elapsed_sec = 0;
	phandle->rec_play_start_tick_ms = divoom_system_get_tickms();
	divoom_local_music_ctrl_tone_play(path, phandle->edit_info.volume, NULL, (int)sec);
	phandle->rec_playing = 1;
}

static void divoom_astrootoo_alarm_rec_timer_stop(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    if (phandle->rec_timer)
    {
        lv_timer_del(phandle->rec_timer);
        phandle->rec_timer = NULL;
    }
}

#define DIVOOM_ASTRO_ALARM_REC_PULSE_MS       525
#define DIVOOM_ASTRO_ALARM_REC_ICON_ZOOM_MAX  ((int32_t)(LV_IMG_ZOOM_NONE * 106 / 100))

static void divoom_astrootoo_alarm_rec_icon_pulse_cb(void *var, int32_t v)
{
    lv_img_set_zoom((lv_obj_t *)var, (uint16_t)v);
}

static void divoom_astrootoo_alarm_rec_icon_pulse_stop(lv_obj_t *icon)
{
    if (!icon)
    {
        return;
    }
    lv_anim_del(icon, divoom_astrootoo_alarm_rec_icon_pulse_cb);
    lv_img_set_zoom(icon, LV_IMG_ZOOM_NONE);
}

static void divoom_astrootoo_alarm_rec_icon_pulse_sync(lv_obj_t *icon, int live)
{
    lv_anim_t a;

    if (!icon)
    {
        return;
    }
    if (!live)
    {
        divoom_astrootoo_alarm_rec_icon_pulse_stop(icon);
        return;
    }
    if (lv_anim_get(icon, divoom_astrootoo_alarm_rec_icon_pulse_cb) != NULL)
    {
        return;
    }
    lv_img_set_zoom(icon, LV_IMG_ZOOM_NONE);
    lv_anim_init(&a);
    lv_anim_set_var(&a, icon);
    lv_anim_set_exec_cb(&a, divoom_astrootoo_alarm_rec_icon_pulse_cb);
    lv_anim_set_values(&a, LV_IMG_ZOOM_NONE, DIVOOM_ASTRO_ALARM_REC_ICON_ZOOM_MAX);
    lv_anim_set_time(&a, DIVOOM_ASTRO_ALARM_REC_PULSE_MS);
    lv_anim_set_playback_time(&a, DIVOOM_ASTRO_ALARM_REC_PULSE_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_step);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);
}

static void divoom_astrootoo_alarm_rec_refresh(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    const char *title_key;
    char clock_buf[16];
    char date_buf[8];
    uint32 sec;
    int panel_hot;
    int file_hot;
    int as_hot;
    int use_selected;

    if (!phandle->rec_panel)
    {
        return;
    }

    if (phandle->rec_playing)
    {
        title_key = "VoiceRecordPlaying";
    }
    else if (phandle->rec_capture_active)
    {
        title_key = "VoiceRecordPressToEnd";
    }
    else
    {
        title_key = "VoiceRecordPressToStart";
    }
    if (phandle->rec_title)
    {
        lv_label_set_text(phandle->rec_title, divoom_menu_control_get_disp_str(title_key));
        lv_obj_set_style_text_color(phandle->rec_title, lv_color_hex(DIVOOM_ASTRO_INK), 0);
        lv_obj_set_style_text_opa(phandle->rec_title, DIVOOM_ASTRO_ALARM_REC_HINT_OPA, 0);
    }

    /* 录音按钮：空闲圆点 / 停止方块 / 播放（预载三张图，仅切显隐） */
    if (phandle->rec_icon_idle)
    {
        if (!phandle->rec_playing && !phandle->rec_capture_active)
        {
            lv_obj_clear_flag(phandle->rec_icon_idle, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(phandle->rec_icon_idle, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (phandle->rec_icon_stop)
    {
        int rec_live = (phandle->rec_capture_active && !phandle->rec_playing) ? 1 : 0;
        if (rec_live)
        {
            lv_obj_clear_flag(phandle->rec_icon_stop, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(phandle->rec_icon_stop, LV_OBJ_FLAG_HIDDEN);
        }
        divoom_astrootoo_alarm_rec_icon_pulse_sync(phandle->rec_icon_stop, rec_live);
    }
    if (phandle->rec_play_img)
    {
        if (phandle->rec_playing)
        {
            lv_obj_clear_flag(phandle->rec_play_img, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(phandle->rec_play_img, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (phandle->rec_playing || phandle->rec_capture_active)
    {
        sec = phandle->rec_elapsed_sec;
    }
    else if (phandle->rec_has_clip)
    {
        sec = phandle->rec_clip_sec;
    }
    else
    {
        sec = 0;
    }
    divoom_astrootoo_alarm_rec_format_clock(sec, clock_buf, (int)sizeof(clock_buf));
    if (phandle->rec_time_lbl)
    {
        lv_label_set_text(phandle->rec_time_lbl, clock_buf);
    }

    /* 文件行：日期 MM-DD / -- + 时长 HH:MM:SS */
    divoom_astrootoo_alarm_rec_format_date(phandle->rec_clip_month, phandle->rec_clip_day,
        date_buf, (int)sizeof(date_buf));
    if (phandle->rec_file_title)
    {
        lv_label_set_text(phandle->rec_file_title,
            phandle->rec_has_clip ? date_buf : "--");
    }
    if (phandle->rec_file_dur)
    {
        divoom_astrootoo_alarm_rec_format_clock(
            phandle->rec_has_clip ? phandle->rec_clip_sec : 0,
            clock_buf, (int)sizeof(clock_buf));
        lv_label_set_text(phandle->rec_file_dur, clock_buf);
        lv_obj_set_style_text_color(phandle->rec_file_dur, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    }

    use_selected = (phandle->edit_info.mode == 1 && phandle->rec_has_clip) ? 1 : 0;
    if (phandle->rec_as_bell_check)
    {
        if (use_selected)
        {
            lv_obj_clear_flag(phandle->rec_as_bell_check, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(phandle->rec_as_bell_check, LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* 聚焦琥珀；录音中面板强制琥珀（与设计 panelRecording 一致） */
    panel_hot = (phandle->rec_focus == 0) || phandle->rec_capture_active;
    file_hot = (phandle->rec_focus == 1);
    as_hot = (phandle->rec_focus == 2);
    divoom_astrootoo_alarm_rec_set_hot(phandle->rec_panel, panel_hot, 1);
    divoom_astrootoo_alarm_rec_set_hot(phandle->rec_file_btn, file_hot, 0);
    divoom_astrootoo_alarm_rec_set_hot(phandle->rec_as_bell_btn, as_hot, 0);
}

static void divoom_astrootoo_alarm_rec_timer_cb(lv_timer_t *t)
{
	Pdivoom_astrootoo_alarm_ctrl phandle = (Pdivoom_astrootoo_alarm_ctrl)lv_timer_get_user_data(t);
	uint8 was_busy;
	uint32 now;

	if (!phandle)
	{
		return;
	}

	was_busy = phandle->rec_capture_active;
	if (was_busy)
	{
		phandle->rec_elapsed_sec = divoom_astrootoo_alarm_rec_live_sec(phandle);
		if (phandle->rec_elapsed_sec >= DIVOOM_ASTRO_ALARM_REC_MAX_SEC)
		{
			divoom_astrootoo_alarm_rec_stop_capture(phandle);
			divoom_astrootoo_alarm_rec_freeze_and_commit(phandle);
		}
		else if (phandle->rec_enc_sec > 0u
			 || !divoom_local_music_ctrl_voice_clip_record_is_active())
		{
			divoom_astrootoo_alarm_rec_freeze_and_commit(phandle);
		}
	}
	else if (phandle->rec_playing)
	{
		now = divoom_system_get_tickms();
		if (phandle->rec_play_start_tick_ms != 0u)
		{
			phandle->rec_elapsed_sec = (now - phandle->rec_play_start_tick_ms) / 1000u;
		}
		if (phandle->rec_clip_sec > 0 && phandle->rec_elapsed_sec > phandle->rec_clip_sec)
		{
			phandle->rec_elapsed_sec = phandle->rec_clip_sec;
		}
	}

	phandle->rec_capture_active = divoom_local_music_ctrl_voice_clip_record_is_active()
		? 1 : phandle->rec_capture_active;

	if (phandle->rec_playing && !divoom_local_music_ctrl_get_tone_status())
	{
		phandle->rec_playing = 0;
	}

	divoom_astrootoo_alarm_rec_refresh(phandle);

	if (!phandle->rec_capture_active && !phandle->rec_playing)
	{
		divoom_astrootoo_alarm_rec_timer_stop(phandle);
	}
}

static void divoom_astrootoo_alarm_rec_stop_all(Pdivoom_astrootoo_alarm_ctrl phandle)
{
	divoom_astrootoo_alarm_rec_timer_stop(phandle);
	divoom_astrootoo_alarm_rec_stop_play(phandle);
	if (phandle->rec_capture_active)
	{
		divoom_astrootoo_alarm_rec_stop_capture(phandle);
		divoom_astrootoo_alarm_rec_freeze_and_commit(phandle);
	}
	phandle->rec_capture_active = 0;
	phandle->rec_playing = 0;
	divoom_alarm_ctrl_preview_stop();
}

static void divoom_astrootoo_alarm_bell_show_record(Pdivoom_astrootoo_alarm_ctrl phandle)
{
	phandle->bell_phase = 3;
	phandle->bell_cur_cat = DIVOOM_ASTRO_RING_CAT_RECORD;
	phandle->rec_focus = 0;
	phandle->rec_capture_active = 0;
	phandle->rec_playing = 0;
	phandle->rec_enc_sec = 0;
	phandle->rec_start_tick_ms = 0;
	phandle->rec_has_clip = divoom_astrootoo_alarm_rec_file_valid(phandle) ? 1 : 0;
	phandle->rec_clip_sec = phandle->rec_has_clip
		? divoom_astrootoo_alarm_rec_duration_sec(phandle) : 0;
	phandle->rec_elapsed_sec = 0;
	divoom_astrootoo_alarm_rec_load_clip_date(phandle);
    if (phandle->bell_cat_list)
    {
        lv_obj_add_flag(phandle->bell_cat_list, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->bell_item_list)
    {
        lv_obj_add_flag(phandle->bell_item_list, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->bell_grid_list)
    {
        lv_obj_add_flag(phandle->bell_grid_list, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->bell_tf_root)
    {
        lv_obj_add_flag(phandle->bell_tf_root, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->bell_record_list)
    {
        lv_obj_clear_flag(phandle->bell_record_list, LV_OBJ_FLAG_HIDDEN);
    }
    divoom_astrootoo_alarm_rec_refresh(phandle);
    {
        lv_obj_t *focus_objs[3];

        focus_objs[0] = phandle->rec_panel;
        focus_objs[1] = phandle->rec_file_btn;
        focus_objs[2] = phandle->rec_as_bell_btn;
        divoom_astrootoo_alarm_group_set(focus_objs, 3);
    }
    if (phandle->rec_panel)
    {
        lv_group_focus_obj(phandle->rec_panel);
    }
}

static void divoom_astrootoo_alarm_bell_enter_cat(Pdivoom_astrootoo_alarm_ctrl phandle, int cat)
{
    if (cat == DIVOOM_ASTRO_RING_CAT_CLASSIC)
    {
        divoom_astrootoo_alarm_bell_show_grid(phandle);
    }
    else if (cat == DIVOOM_ASTRO_RING_CAT_RECORD)
    {
        divoom_astrootoo_alarm_bell_show_record(phandle);
    }
    else if (cat == DIVOOM_ASTRO_RING_CAT_TF)
    {
        divoom_astrootoo_alarm_bell_show_tf(phandle);
    }
#if 0 /* 唤醒场景二级条目列表 */
    else
    {
        divoom_astrootoo_alarm_bell_show_items(phandle, cat);
    }
#endif
}

static void divoom_astrootoo_alarm_enter_bell(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    divoom_astrootoo_alarm_bell_stop_preview();
    phandle->mode = DIVOOM_ASTROTOO_ALARM_MODE_BELL;
    phandle->bell_cat_focus = divoom_astrootoo_alarm_ring_cat_of(&phandle->edit_info);
    divoom_astrootoo_alarm_show_only(phandle, DIVOOM_ASTROTOO_ALARM_MODE_BELL);
    divoom_astrootoo_alarm_bell_show_cats(phandle);
}

/* 条目选中 -> 写入草稿并试听 */
static void divoom_astrootoo_alarm_bell_apply_item(Pdivoom_astrootoo_alarm_ctrl phandle, int item)
{
    switch (phandle->bell_cur_cat)
    {
    case DIVOOM_ASTRO_RING_CAT_CLASSIC:
        phandle->edit_info.mode = 0;
        phandle->edit_info.tip_index = (T_U8)item;
        break;
    case DIVOOM_ASTRO_RING_CAT_RECORD:
        phandle->edit_info.mode = 1;
        break;
    case DIVOOM_ASTRO_RING_CAT_BT:
        phandle->edit_info.mode = DIVOOM_ASTRO_ALARM_BT_MODE;
        break;
    case DIVOOM_ASTRO_RING_CAT_TF:
        break;
    default:
        break;
    }
    divoom_astrootoo_alarm_edit_on_modified(phandle);
}

static void divoom_astrootoo_alarm_bell_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_alarm_ctrl phandle;
    uint8_t key_code;
    lv_obj_t *target;
    int i;
    int found;

    phandle = (Pdivoom_astrootoo_alarm_ctrl)lv_event_get_user_data(e);
    if (!phandle || lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }

    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    target = lv_event_get_target(e);

    if (phandle->bell_phase == 0)
    {
        /* 大类列表 */
        found = phandle->bell_cat_focus;
        for (i = 0; i < DIVOOM_ASTRO_ALARM_RING_CAT_COUNT; i++)
        {
            if (phandle->bell_cat_row[i] == target)
            {
                found = i;
                break;
            }
        }
        phandle->bell_cat_focus = found;

        if (divoom_astrootoo_ui_key_is_esc(key_code))
        {
            divoom_astrootoo_alarm_bell_stop_preview();
            divoom_astrootoo_alarm_return_edit(phandle, 2);
            return;
        }
        if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code))
        {
            int go_up = divoom_astrootoo_ui_key_is_up(key_code);
            int n = DIVOOM_ASTRO_ALARM_RING_CAT_COUNT;

            phandle->bell_cat_focus = go_up ? ((found + n - 1) % n) : ((found + 1) % n);
            if (phandle->bell_cat_row[phandle->bell_cat_focus])
            {
                lv_group_focus_obj(phandle->bell_cat_row[phandle->bell_cat_focus]);
                divoom_astrootoo_ui_focus_scroll(phandle->bell_cat_row[phandle->bell_cat_focus]);
            }
            divoom_astrootoo_alarm_bell_refresh_cats(phandle);
            return;
        }
        if (divoom_astrootoo_ui_key_is_enter(key_code) || divoom_astrootoo_ui_key_is_right(key_code))
        {
            if (!divoom_astrootoo_alarm_ring_cat_has_picker(found))
            {
                phandle->edit_info.mode = DIVOOM_ASTRO_ALARM_BT_MODE;
                divoom_astrootoo_alarm_edit_on_modified(phandle);
                divoom_astrootoo_alarm_bell_refresh_cats(phandle);
                return;
            }
            divoom_astrootoo_alarm_bell_enter_cat(phandle, found);
        }
        return;
    }

#if 0 /* 唤醒场景二级条目列表按键 */
    /* 条目列表 */
    found = phandle->bell_item_focus;
    for (i = 0; i < DIVOOM_ASTRO_ALARM_RING_ITEM_MAX; i++)
    {
        if (phandle->bell_item_row[i] == target)
        {
            found = i;
            break;
        }
    }
    phandle->bell_item_focus = found;

    if (divoom_astrootoo_ui_key_is_esc(key_code) || divoom_astrootoo_ui_key_is_left(key_code))
    {
        divoom_astrootoo_alarm_bell_stop_preview();
        phandle->bell_cat_focus = phandle->bell_cur_cat;
        divoom_astrootoo_alarm_bell_show_cats(phandle);
        return;
    }
    if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code))
    {
        int go_up = divoom_astrootoo_ui_key_is_up(key_code);
        int n = divoom_astrootoo_alarm_ring_item_count(phandle->bell_cur_cat);

        phandle->bell_item_focus = go_up ? ((found + n - 1) % n) : ((found + 1) % n);
        if (phandle->bell_item_row[phandle->bell_item_focus])
        {
            lv_group_focus_obj(phandle->bell_item_row[phandle->bell_item_focus]);
            divoom_astrootoo_ui_focus_scroll(phandle->bell_item_row[phandle->bell_item_focus]);
        }
        divoom_astrootoo_alarm_bell_refresh_items(phandle);
        divoom_astrootoo_alarm_bell_preview_item(phandle, phandle->bell_cur_cat, phandle->bell_item_focus);
        return;
    }
    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        divoom_astrootoo_alarm_bell_apply_item(phandle, found);
        /* 选中后回到大类页（与 AstroTooUI 一致），大类页显示新摘要 */
        phandle->bell_cat_focus = phandle->bell_cur_cat;
        divoom_astrootoo_alarm_bell_show_cats(phandle);
    }
#endif
}

/* 经典铃声图标网格按键：上下左右网格导航，ENTER 选中并返回大类页 */
static void divoom_astrootoo_alarm_bell_grid_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_alarm_ctrl phandle;
    uint8_t key_code;
    lv_obj_t *target;
    int i;
    int found;

    phandle = (Pdivoom_astrootoo_alarm_ctrl)lv_event_get_user_data(e);
    if (!phandle || lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }

    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    target = lv_event_get_target(e);
    found = phandle->bell_grid_focus;
    for (i = 0; i < DIVOOM_ASTRO_ALARM_BELL_COUNT; i++)
    {
        if (phandle->bell_grid_btn[i] == target)
        {
            found = i;
            break;
        }
    }
    phandle->bell_grid_focus = found;

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_astrootoo_alarm_bell_stop_preview();
        phandle->bell_cat_focus = DIVOOM_ASTRO_RING_CAT_CLASSIC;
        divoom_astrootoo_alarm_bell_show_cats(phandle);
        return;
    }
    if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code)
        || divoom_astrootoo_ui_key_is_left(key_code) || divoom_astrootoo_ui_key_is_right(key_code))
    {
        int direct;
        lv_obj_t *next;

        if (divoom_astrootoo_ui_key_is_up(key_code))
        {
            direct = DIVOOM_MENU_DIRECT_UP;
        }
        else if (divoom_astrootoo_ui_key_is_down(key_code))
        {
            direct = DIVOOM_MENU_DIRECT_DOWN;
        }
        else if (divoom_astrootoo_ui_key_is_left(key_code))
        {
            direct = DIVOOM_MENU_DIRECT_LEFT;
        }
        else
        {
            direct = DIVOOM_MENU_DIRECT_RIGHT;
        }
        next = divoom_menu_direct_ctrl_get(phandle->bell_grid_direct, target, direct);
        if (next && next != target)
        {
            for (i = 0; i < DIVOOM_ASTRO_ALARM_BELL_COUNT; i++)
            {
                if (phandle->bell_grid_btn[i] == next)
                {
                    phandle->bell_grid_focus = i;
                    break;
                }
            }
            lv_group_focus_obj(next);
            divoom_astrootoo_ui_focus_scroll(next);
            divoom_astrootoo_alarm_bell_grid_refresh(phandle);
            divoom_astrootoo_alarm_bell_preview_item(phandle, DIVOOM_ASTRO_RING_CAT_CLASSIC, phandle->bell_grid_focus);
        }
        return;
    }
    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        divoom_astrootoo_alarm_bell_apply_item(phandle, found);
        phandle->bell_cat_focus = DIVOOM_ASTRO_RING_CAT_CLASSIC;
        divoom_astrootoo_alarm_bell_show_cats(phandle);
    }
}

/* 录音页按键：上下在 面板/文件行/设为铃声 之间切换；ENTER 录音/播放/设为铃声；ESC 返回大类 */
static void divoom_astrootoo_alarm_rec_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_alarm_ctrl phandle;
    uint8_t key_code;
    lv_obj_t *target;
    lv_obj_t *focus_objs[3];

    phandle = (Pdivoom_astrootoo_alarm_ctrl)lv_event_get_user_data(e);
    if (!phandle || lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }

    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    target = lv_event_get_target(e);
    focus_objs[0] = phandle->rec_panel;
    focus_objs[1] = phandle->rec_file_btn;
    focus_objs[2] = phandle->rec_as_bell_btn;
    if (target == phandle->rec_file_btn)
    {
        phandle->rec_focus = 1;
    }
    else if (target == phandle->rec_as_bell_btn)
    {
        phandle->rec_focus = 2;
    }
    else
    {
        phandle->rec_focus = 0;
    }

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_astrootoo_alarm_rec_stop_all(phandle);
        phandle->bell_cat_focus = DIVOOM_ASTRO_RING_CAT_RECORD;
        divoom_astrootoo_alarm_bell_show_cats(phandle);
        return;
    }
    if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code))
    {
        int go_up = divoom_astrootoo_ui_key_is_up(key_code);

        phandle->rec_focus = go_up ? ((phandle->rec_focus + 2) % 3) : ((phandle->rec_focus + 1) % 3);
        if (focus_objs[phandle->rec_focus])
        {
            lv_group_focus_obj(focus_objs[phandle->rec_focus]);
            divoom_astrootoo_ui_focus_scroll(focus_objs[phandle->rec_focus]);
        }
        divoom_astrootoo_alarm_rec_refresh(phandle);
        return;
    }
    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        if (phandle->rec_focus == 0)
        {
            /* 录音面板：开始 / 停止录音 */
            if (phandle->rec_capture_active)
            {
                divoom_astrootoo_alarm_rec_stop_capture(phandle);
                divoom_astrootoo_alarm_rec_freeze_and_commit(phandle);
                divoom_astrootoo_alarm_rec_timer_stop(phandle);
            }
            else
            {
                if (divoom_astrootoo_alarm_rec_start_capture(phandle) == 0)
                {
                    divoom_astrootoo_alarm_rec_timer_stop(phandle);
                    phandle->rec_timer = lv_timer_create(divoom_astrootoo_alarm_rec_timer_cb,
                        DIVOOM_ASTRO_ALARM_REC_TIMER_MS, phandle);
                }
            }
            divoom_astrootoo_alarm_rec_refresh(phandle);
        }
        else if (phandle->rec_focus == 1)
        {
            /* 文件行：试听 / 停止 */
            if (!phandle->rec_has_clip && !divoom_astrootoo_alarm_rec_file_valid(phandle))
            {
                return;
            }
            if (phandle->rec_playing)
            {
                divoom_astrootoo_alarm_rec_stop_play(phandle);
                divoom_astrootoo_alarm_rec_timer_stop(phandle);
            }
            else
            {
                divoom_astrootoo_alarm_rec_start_play(phandle);
                if (!phandle->rec_timer)
                {
                    phandle->rec_timer = lv_timer_create(divoom_astrootoo_alarm_rec_timer_cb,
                        DIVOOM_ASTRO_ALARM_REC_TIMER_MS, phandle);
                }
            }
            divoom_astrootoo_alarm_rec_refresh(phandle);
        }
        else
        {
            /* 录音作为铃声 */
            if (phandle->rec_has_clip)
            {
                divoom_astrootoo_alarm_rec_stop_all(phandle);
                phandle->edit_info.mode = 1;
                divoom_astrootoo_alarm_edit_on_modified(phandle);
                phandle->bell_cat_focus = DIVOOM_ASTRO_RING_CAT_RECORD;
                divoom_astrootoo_alarm_bell_show_cats(phandle);
            }
        }
    }
}

/* ---------- 进入列表 / 列表重建 ---------- */

static void divoom_astrootoo_alarm_unbind_list_group(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    int i;

    if (!phandle || !phandle->rows)
    {
        return;
    }

    for (i = 0; i < phandle->row_count; i++)
    {
        if (phandle->rows[i].btn)
        {
            lv_group_remove_obj(phandle->rows[i].btn);
        }
    }
}

static void divoom_astrootoo_alarm_set_list_input(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    lv_group_t *g;
    int i;

    if (!phandle)
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

    g = lv_group_get_default();
    for (i = 0; i < phandle->row_count; i++)
    {
        if (phandle->rows && phandle->rows[i].btn
            && lv_obj_get_group(phandle->rows[i].btn) != g)
        {
            lv_group_add_obj(g, phandle->rows[i].btn);
        }
    }
}

static void divoom_astrootoo_alarm_enter_list(Pdivoom_astrootoo_alarm_ctrl phandle, int focus_row)
{
    lv_obj_t *focus;

    divoom_astrootoo_alarm_bell_stop_preview();
    phandle->mode = DIVOOM_ASTROTOO_ALARM_MODE_LIST;
    divoom_astrootoo_alarm_show_only(phandle, DIVOOM_ASTROTOO_ALARM_MODE_LIST);

    if (focus_row < 0 || focus_row >= phandle->row_count)
    {
        focus_row = 0;
    }
    phandle->column_focus = DIVOOM_ASTROTOO_ALARM_COL_BODY;

    divoom_astrootoo_alarm_set_list_input(phandle);

    focus = (phandle->rows && phandle->row_count > 0) ? phandle->rows[focus_row].btn : NULL;
    if (focus)
    {
        lv_group_focus_obj(focus);
        divoom_astrootoo_ui_focus_scroll(focus);
        divoom_astrootoo_alarm_refresh_all_rows(phandle, focus);
    }
}

static void divoom_astrootoo_alarm_rebuild_cb(lv_timer_t *t)
{
    Pdivoom_astrootoo_alarm_ctrl phandle;
    int i;

    phandle = (Pdivoom_astrootoo_alarm_ctrl)lv_timer_get_user_data(t);
    if (!phandle)
    {
        return;
    }
    phandle->rebuild_timer = NULL;

    if (phandle->rows)
    {
        for (i = 0; i < phandle->row_count; i++)
        {
            if (phandle->rows[i].btn)
            {
                lv_obj_del(phandle->rows[i].btn);
            }
        }
        divoom_system_free(phandle->rows);
        phandle->rows = NULL;
    }
    if (phandle->direct_handle)
    {
        divoom_menu_direct_ctrl_destory(phandle->direct_handle);
        phandle->direct_handle = NULL;
    }
    if (phandle->alarm_list)
    {
        divoom_system_free(phandle->alarm_list);
        phandle->alarm_list = NULL;
    }

    phandle->alarm_list = divoom_alarm_menu_get_alarm_list(&phandle->alarm_count);
    if (phandle->alarm_count < 0)
    {
        phandle->alarm_count = 0;
    }
    phandle->row_count = phandle->alarm_count;
    phandle->rows = NULL;
    if (phandle->row_count > 0)
    {
        phandle->rows = divoom_system_malloc(sizeof(divoom_astrootoo_alarm_row) * (size_t)phandle->row_count);
        if (!phandle->rows)
        {
            return;
        }
    }
    phandle->direct_handle = divoom_menu_direct_ctrl_create(1, phandle->row_count);

    for (i = 0; i < phandle->alarm_count; i++)
    {
        divoom_astrootoo_alarm_create_row(phandle, i, i);
    }

    /* 在 timer 内加载图标（非 key 回调，避免看门狗） */
    for (i = 0; i < phandle->row_count; i++)
    {
        if (phandle->rows[i].chevron_img)
        {
            divoom_menu_control_lv_img_set_src(phandle->rows[i].chevron_img,
                DIVOOM_ASTROTOO_ALARM_UI_DIR "at_alarm_chevron.png");
        }
        divoom_astrootoo_alarm_refresh_row_data(phandle, i);
    }

    divoom_astrootoo_alarm_enter_list(phandle, phandle->pending_rebuild_focus);
}

/* ---------- 子页创建 ---------- */

static void divoom_astrootoo_alarm_edit_bind_val(Pdivoom_astrootoo_alarm_ctrl phandle, int i)
{
    lv_obj_t *row;
    lv_obj_t *name;

    if (!phandle || i < 0 || i >= 3 || !phandle->edit_rows[i] || !phandle->edit_val[i])
    {
        return;
    }
    row = phandle->edit_rows[i];
    name = lv_obj_get_child(row, 0);
    if (!name)
    {
        return;
    }
    divoom_astrootoo_ui_list_kv_bind_row(name, phandle->edit_val[i],
        DIVOOM_ASTRO_ALARM_EDIT_ROW_W, 14, 14, DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W,
        -(14 + DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W + DIVOOM_ASTRO_LIST_LABEL_GAP), 0);
    divoom_astrootoo_ui_list_row_bind_marquee(row, phandle->edit_val[i]);
}

static void divoom_astrootoo_alarm_create_edit(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    static const char *names[4] = { "Time", "Repeat", "Bells", "Volume" };
    lv_obj_t *r;
    int i;

    r = lv_obj_create(phandle->root_obj);
    lv_obj_add_style(r, &phandle->ui.root_style, 0);
    lv_obj_set_size(r, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_pos(r, 0, 0);
    lv_obj_clear_flag(r, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(r, LV_OBJ_FLAG_HIDDEN);
    phandle->edit_root = r;

    divoom_astrootoo_ui_create_page_header_tab(r, &phandle->ui, "EditAlarm",
        DIVOOM_ASTROTOO_ALARM_UI_DIR "at_alarm_header.png", phandle->menu_id);
    phandle->edit_direct = divoom_menu_direct_ctrl_create(1, 4);

    for (i = 0; i < 4; i++)
    {
        lv_obj_t *btn;
        lv_obj_t *name;

        btn = lv_btn_create(r);
        lv_obj_remove_style_all(btn);
        divoom_astrootoo_alarm_apply_slot_style(btn, phandle, 0);
        lv_obj_set_size(btn, DIVOOM_ASTRO_ALARM_EDIT_ROW_W, DIVOOM_ASTRO_ALARM_EDIT_ROW_H);
        lv_obj_set_pos(btn, DIVOOM_ASTRO_ALARM_EDIT_PAD_X,
            DIVOOM_ASTRO_ALARM_SUB_TOP + i * (DIVOOM_ASTRO_ALARM_EDIT_ROW_H + DIVOOM_ASTRO_ALARM_EDIT_ROW_GAP));
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

        name = lv_label_create(btn);
        lv_obj_set_style_text_color(name, lv_color_hex(DIVOOM_ASTRO_INK), 0);
        lv_obj_set_style_text_font(name, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
        lv_obj_align(name, LV_ALIGN_LEFT_MID, 14, 0);
        lv_label_set_text(name, divoom_menu_control_get_disp_str(names[i]));

        if (i < 3)
        {
            lv_obj_t *val = lv_label_create(btn);
            lv_obj_t *chevron = lv_img_create(btn);

            lv_obj_set_style_text_color(val, lv_color_hex(DIVOOM_ASTRO_INK), 0);
            lv_obj_set_style_text_font(val, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
            /* 摘要 + chevron（对照 AlarmScreen FlowtooSublevelChevronIcon） */
            lv_obj_align(val, LV_ALIGN_RIGHT_MID, -(14 + 12 + 8), 0);
            phandle->edit_val[i] = val;

            divoom_astrootoo_ui_apply_sublevel_chevron_size(chevron);
            lv_obj_align(chevron, LV_ALIGN_RIGHT_MID, -14, 0);
            phandle->edit_chevron[i] = chevron;
        }
        else
        {
            lv_obj_t *bar;

            /* 用 lv_slider 还原 AstroTooUI SettingsSliderRow：仅滑条，不展示数值 */
            bar = lv_slider_create(btn);
            lv_obj_set_size(bar, DIVOOM_ASTRO_ALARM_VOL_W, 8);
            lv_obj_align(bar, LV_ALIGN_RIGHT_MID, -14, 0);
            lv_slider_set_range(bar, 0, 100);
            lv_obj_clear_flag(bar, LV_OBJ_FLAG_CLICKABLE);
            /* 轨道 */
            lv_obj_set_style_bg_color(bar, lv_color_hex(DIVOOM_ASTRO_ALARM_VOL_TRACK_TOP), LV_PART_MAIN);
            lv_obj_set_style_bg_grad_color(bar, lv_color_hex(DIVOOM_ASTRO_ALARM_VOL_TRACK_BOT), LV_PART_MAIN);
            lv_obj_set_style_bg_grad_dir(bar, LV_GRAD_DIR_VER, LV_PART_MAIN);
            lv_obj_set_style_radius(bar, 0, LV_PART_MAIN);
            lv_obj_set_style_border_width(bar, 0, LV_PART_MAIN);
            /* 填充 */
            lv_obj_set_style_bg_color(bar, lv_color_hex(DIVOOM_ASTRO_ALARM_VOL_FILL_TOP), LV_PART_INDICATOR);
            lv_obj_set_style_bg_grad_color(bar, lv_color_hex(DIVOOM_ASTRO_ALARM_VOL_FILL_BOT), LV_PART_INDICATOR);
            lv_obj_set_style_bg_grad_dir(bar, LV_GRAD_DIR_VER, LV_PART_INDICATOR);
            lv_obj_set_style_radius(bar, 0, LV_PART_INDICATOR);
            /* 滑块：浅色像素块 + 上左白/下右黑描边 */
            lv_obj_set_style_bg_color(bar, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
            lv_obj_set_style_radius(bar, 0, LV_PART_KNOB);
            lv_obj_set_style_pad_all(bar, 6, LV_PART_KNOB);
            lv_obj_set_style_border_width(bar, 1, LV_PART_KNOB);
            lv_obj_set_style_border_color(bar, lv_color_hex(0x000000), LV_PART_KNOB);
            lv_obj_set_style_outline_width(bar, 0, LV_PART_KNOB | LV_STATE_FOCUSED);
            lv_obj_set_style_shadow_width(bar, 0, LV_PART_KNOB);
            phandle->edit_vol_bar = bar;
        }

        lv_obj_add_event_cb(btn, divoom_astrootoo_alarm_edit_key_cb, LV_EVENT_KEY, phandle);
        divoom_menu_direct_ctrl_update(phandle->edit_direct, btn, 0, i);
        phandle->edit_rows[i] = btn;
        if (i < 3)
        {
            divoom_astrootoo_alarm_edit_bind_val(phandle, i);
        }
    }
}

static void divoom_astrootoo_alarm_create_time(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    lv_obj_t *r;
    divoom_astrootoo_time_pick_widgets_t widgets;
    lv_coord_t stack_y;

    r = lv_obj_create(phandle->root_obj);
    lv_obj_add_style(r, &phandle->ui.root_style, 0);
    lv_obj_set_size(r, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_pos(r, 0, 0);
    lv_obj_clear_flag(r, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(r, LV_OBJ_FLAG_HIDDEN);
    phandle->time_root = r;
    divoom_astrootoo_ui_create_page_header_tab(r, &phandle->ui, "Time",
        DIVOOM_ASTROTOO_ALARM_UI_DIR "at_alarm_header.png", phandle->menu_id);

    stack_y = DIVOOM_ASTRO_HEADER_H
        + divoom_astrootoo_settings_time_pick_stack_y(DIVOOM_ASTRO_CONTENT_H);
    divoom_astrootoo_settings_create_time_pick_stack(r, phandle->menu_id,
        &phandle->ui.content_style, stack_y, &widgets);
    phandle->tp_tray = widgets.tray;
    phandle->tp_ampm_box = widgets.ampm_box;
    phandle->tp_ampm_lbl = widgets.ampm_lbl;
    phandle->tp_h_box = widgets.h_box;
    phandle->tp_m_box = widgets.m_box;
    phandle->tp_h_lbl = widgets.h_lbl;
    phandle->tp_m_lbl = widgets.m_lbl;
    phandle->tp_h_cap = widgets.h_cap;
    phandle->tp_m_cap = widgets.m_cap;
    phandle->tp_cancel = widgets.cancel;
    phandle->tp_confirm = widgets.confirm;

    if (phandle->tp_ampm_box)
    {
        lv_obj_add_event_cb(phandle->tp_ampm_box, divoom_astrootoo_alarm_time_key_cb, LV_EVENT_KEY, phandle);
    }
    if (phandle->tp_h_box)
    {
        lv_obj_add_event_cb(phandle->tp_h_box, divoom_astrootoo_alarm_time_key_cb, LV_EVENT_KEY, phandle);
    }
    if (phandle->tp_m_box)
    {
        lv_obj_add_event_cb(phandle->tp_m_box, divoom_astrootoo_alarm_time_key_cb, LV_EVENT_KEY, phandle);
    }
    if (phandle->tp_cancel)
    {
        lv_obj_add_event_cb(phandle->tp_cancel, divoom_astrootoo_alarm_time_key_cb, LV_EVENT_KEY, phandle);
    }
    if (phandle->tp_confirm)
    {
        lv_obj_add_event_cb(phandle->tp_confirm, divoom_astrootoo_alarm_time_key_cb, LV_EVENT_KEY, phandle);
    }
}

static void divoom_astrootoo_alarm_create_sub_list_row(Pdivoom_astrootoo_alarm_ctrl phandle,
    lv_obj_t *parent, int index, const char *text, lv_event_cb_t cb,
    lv_obj_t **out_row, lv_obj_t **out_mark, lv_obj_t **out_label)
{
    lv_obj_t *btn;
    lv_obj_t *lab;
    lv_obj_t *mark;
    lv_coord_t y;

    y = index * (DIVOOM_ASTRO_ALARM_SUBROW_H + DIVOOM_ASTRO_ALARM_SUBROW_GAP);

    btn = lv_btn_create(parent);
    lv_obj_remove_style_all(btn);
    divoom_astrootoo_alarm_apply_slot_style(btn, phandle, 0);
    lv_obj_set_size(btn, DIVOOM_ASTRO_ALARM_EDIT_ROW_W, DIVOOM_ASTRO_ALARM_SUBROW_H);
    lv_obj_set_pos(btn, DIVOOM_ASTRO_ALARM_EDIT_PAD_X, y);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    lab = lv_label_create(btn);
    lv_obj_set_style_text_color(lab, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(lab, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
    lv_obj_align(lab, LV_ALIGN_LEFT_MID, 14, 0);
    lv_label_set_text(lab, text);
    divoom_astrootoo_ui_list_label_prepare(lab,
        divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_ALARM_EDIT_ROW_W, 14, 16,
            DIVOOM_ASTRO_LIST_CHK_TRAIL_ALIGN(16)));
    divoom_astrootoo_ui_list_row_bind_marquee(btn, lab);

    mark = lv_img_create(btn);
    lv_obj_set_size(mark, 24, 24);
    divoom_menu_control_lv_img_set_src(mark, DIVOOM_ASTROTOO_ALARM_UI_DIR "at_repeat_check.png");
    lv_obj_align(mark, LV_ALIGN_RIGHT_MID, -16, 0);
    lv_obj_add_flag(mark, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_event_cb(btn, cb, LV_EVENT_KEY, phandle);
    if (out_row)
    {
        *out_row = btn;
    }
    if (out_mark)
    {
        *out_mark = mark;
    }
    if (out_label)
    {
        *out_label = lab;
    }
}

static lv_obj_t *divoom_astrootoo_alarm_create_sub_scroll(Pdivoom_astrootoo_alarm_ctrl phandle,
    const char *title_key, lv_obj_t **out_root)
{
    lv_obj_t *r;
    lv_obj_t *list;

    r = lv_obj_create(phandle->root_obj);
    lv_obj_add_style(r, &phandle->ui.root_style, 0);
    lv_obj_set_size(r, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_pos(r, 0, 0);
    lv_obj_clear_flag(r, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(r, LV_OBJ_FLAG_HIDDEN);
    divoom_astrootoo_ui_create_page_header_tab(r, &phandle->ui, title_key,
        DIVOOM_ASTROTOO_ALARM_UI_DIR "at_alarm_header.png", phandle->menu_id);

    list = lv_obj_create(r);
    lv_obj_add_style(list, &phandle->ui.grid_cont_style, 0);
    lv_obj_set_size(list, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH - DIVOOM_ASTRO_ALARM_SUB_TOP);
    lv_obj_set_pos(list, 0, DIVOOM_ASTRO_ALARM_SUB_TOP);
    lv_obj_set_style_pad_top(list, 0, 0);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);

    if (out_root)
    {
        *out_root = r;
    }
    return list;
}

static void divoom_astrootoo_alarm_create_repeat(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    static const char *names[8] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun", "Never" };
    lv_obj_t *list;
    int i;

    list = divoom_astrootoo_alarm_create_sub_scroll(phandle, "Repeat", &phandle->repeat_root);
    phandle->repeat_list = list;
    phandle->rp_direct = divoom_menu_direct_ctrl_create(1, 8);

    for (i = 0; i < 8; i++)
    {
        divoom_astrootoo_alarm_create_sub_list_row(phandle, list, i,
            divoom_menu_control_get_disp_str(names[i]),
            divoom_astrootoo_alarm_repeat_key_cb, &phandle->rp_row[i], &phandle->rp_mark[i], NULL);
        divoom_menu_direct_ctrl_update(phandle->rp_direct, phandle->rp_row[i], 0, i);
    }
}

static lv_obj_t *divoom_astrootoo_alarm_create_sub_list(Pdivoom_astrootoo_alarm_ctrl phandle,
    lv_obj_t *parent)
{
    lv_obj_t *list;

    list = lv_obj_create(parent);
    lv_obj_add_style(list, &phandle->ui.grid_cont_style, 0);
    lv_obj_set_size(list, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH - DIVOOM_ASTRO_ALARM_SUB_TOP);
    lv_obj_set_pos(list, 0, DIVOOM_ASTRO_ALARM_SUB_TOP);
    lv_obj_set_style_pad_top(list, 0, 0);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
    return list;
}

static void divoom_astrootoo_alarm_bell_cat_bind_row(Pdivoom_astrootoo_alarm_ctrl phandle, int i)
{
    lv_obj_t *row;
    lv_obj_t *lab;
    lv_obj_t *val;

    if (!phandle || i < 0 || i >= DIVOOM_ASTRO_ALARM_RING_CAT_COUNT)
    {
        return;
    }
    row = phandle->bell_cat_row[i];
    lab = phandle->bell_cat_lbl[i];
    val = phandle->bell_cat_val[i];
    if (!row || !lab || !val)
    {
        return;
    }
    divoom_astrootoo_ui_list_kv_bind_row(lab, val,
        DIVOOM_ASTRO_ALARM_EDIT_ROW_W, DIVOOM_ASTRO_ALARM_BELL_CAT_KEY_X,
        DIVOOM_ASTRO_ALARM_BELL_CAT_VAL_INSET_R, DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W,
        -(DIVOOM_ASTRO_ALARM_BELL_CAT_VAL_INSET_R + DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W
            + DIVOOM_ASTRO_LIST_LABEL_GAP),
        DIVOOM_ASTRO_ALARM_BELL_CAT_VAL_MIN_W);
}

static void divoom_astrootoo_alarm_create_bell_cat_row(Pdivoom_astrootoo_alarm_ctrl phandle,
    lv_obj_t *parent, int i)
{
    lv_obj_t *btn;
    lv_obj_t *mark;
    lv_obj_t *lab;
    lv_obj_t *val;
    lv_obj_t *chevron;
    lv_coord_t y;

    y = i * (DIVOOM_ASTRO_ALARM_SUBROW_H + DIVOOM_ASTRO_ALARM_SUBROW_GAP);

    btn = lv_btn_create(parent);
    lv_obj_remove_style_all(btn);
    divoom_astrootoo_alarm_apply_slot_style(btn, phandle, 0);
    lv_obj_set_size(btn, DIVOOM_ASTRO_ALARM_EDIT_ROW_W, DIVOOM_ASTRO_ALARM_SUBROW_H);
    lv_obj_set_pos(btn, DIVOOM_ASTRO_ALARM_EDIT_PAD_X, y);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    mark = lv_img_create(btn);
    lv_obj_set_size(mark, 24, 24);
    divoom_menu_control_lv_img_set_src(mark, DIVOOM_ASTROTOO_ALARM_UI_DIR "at_repeat_check.png");
    lv_obj_align(mark, LV_ALIGN_LEFT_MID, 12, 0);
    lv_obj_add_flag(mark, LV_OBJ_FLAG_HIDDEN);

    lab = lv_label_create(btn);
    lv_obj_set_style_text_color(lab, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(lab, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
    lv_obj_align(lab, LV_ALIGN_LEFT_MID, DIVOOM_ASTRO_ALARM_BELL_CAT_KEY_X, 0);
    lv_label_set_text(lab, divoom_menu_control_get_disp_str(divoom_astrootoo_alarm_ring_cat_key[i]));

    val = lv_label_create(btn);
    lv_obj_set_style_text_color(val, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_opa(val, (lv_opa_t)(LV_OPA_COVER * 60 / 100), 0);
    lv_obj_set_style_text_font(val, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
    lv_label_set_text(val, "");

    chevron = lv_img_create(btn);
    divoom_astrootoo_ui_apply_sublevel_chevron_size(chevron);
    lv_obj_align(chevron, LV_ALIGN_RIGHT_MID, -DIVOOM_ASTRO_ALARM_BELL_CAT_VAL_INSET_R, 0);
    phandle->bell_cat_chevron[i] = chevron;

    lv_obj_add_event_cb(btn, divoom_astrootoo_alarm_bell_key_cb, LV_EVENT_KEY, phandle);
    phandle->bell_cat_row[i] = btn;
    phandle->bell_cat_mark[i] = mark;
    phandle->bell_cat_lbl[i] = lab;
    phandle->bell_cat_val[i] = val;
    divoom_astrootoo_alarm_bell_cat_bind_row(phandle, i);
    divoom_astrootoo_ui_list_row_bind_marquee(btn, val);
}

/* 经典铃声图标网格（3 列 × 4 行，对照工程 49_bell_*.png 图案列表） */
static void divoom_astrootoo_alarm_create_bell_grid(Pdivoom_astrootoo_alarm_ctrl phandle, lv_obj_t *parent)
{
    lv_coord_t avail_h = DIVOOM_SCREEN_HIGH - DIVOOM_ASTRO_ALARM_SUB_TOP;
    lv_coord_t cell_w = (DIVOOM_SCREEN_WIDTH - DIVOOM_ASTRO_ALARM_GRID_PAD * 2
        - DIVOOM_ASTRO_ALARM_GRID_GAP * (DIVOOM_ASTRO_ALARM_GRID_COLS - 1)) / DIVOOM_ASTRO_ALARM_GRID_COLS;
    lv_coord_t cell_h = (avail_h - DIVOOM_ASTRO_ALARM_GRID_PAD * 2
        - DIVOOM_ASTRO_ALARM_GRID_GAP * (DIVOOM_ASTRO_ALARM_GRID_ROWS - 1)) / DIVOOM_ASTRO_ALARM_GRID_ROWS;
    int i;

    phandle->bell_grid_list = divoom_astrootoo_alarm_create_sub_list(phandle, parent);
    lv_obj_add_flag(phandle->bell_grid_list, LV_OBJ_FLAG_HIDDEN);
    phandle->bell_grid_direct = divoom_menu_direct_ctrl_create(DIVOOM_ASTRO_ALARM_GRID_COLS,
        DIVOOM_ASTRO_ALARM_GRID_ROWS);

    for (i = 0; i < DIVOOM_ASTRO_ALARM_BELL_COUNT; i++)
    {
        int col = i % DIVOOM_ASTRO_ALARM_GRID_COLS;
        int row = i / DIVOOM_ASTRO_ALARM_GRID_COLS;
        lv_obj_t *btn;
        lv_obj_t *icon;
        lv_obj_t *chk;
        char path[48];

        btn = lv_btn_create(phandle->bell_grid_list);
        lv_obj_remove_style_all(btn);
        divoom_astrootoo_alarm_apply_slot_style(btn, phandle, 0);
        lv_obj_set_size(btn, cell_w, cell_h);
        lv_obj_set_pos(btn, DIVOOM_ASTRO_ALARM_GRID_PAD + col * (cell_w + DIVOOM_ASTRO_ALARM_GRID_GAP),
            DIVOOM_ASTRO_ALARM_GRID_PAD + row * (cell_h + DIVOOM_ASTRO_ALARM_GRID_GAP));
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

        icon = lv_img_create(btn);
        snprintf(path, sizeof(path), DIVOOM_ASTROTOO_ALARM_UI_DIR "%s",
            divoom_astrootoo_alarm_bell_icons[i]);
        divoom_menu_control_lv_img_set_src(icon, path);
        /* 切图已缩到显示尺寸（72×72），直接 1:1 显示，不再运行时缩放 */
        lv_obj_center(icon);

        chk = lv_img_create(btn);
        lv_obj_set_size(chk, 24, 24);
        divoom_menu_control_lv_img_set_src(chk, DIVOOM_ASTROTOO_ALARM_UI_DIR "at_repeat_check.png");
        lv_obj_align(chk, LV_ALIGN_BOTTOM_RIGHT, -4, -4);
        lv_obj_add_flag(chk, LV_OBJ_FLAG_HIDDEN);

        lv_obj_add_event_cb(btn, divoom_astrootoo_alarm_bell_grid_key_cb, LV_EVENT_KEY, phandle);
        divoom_menu_direct_ctrl_update(phandle->bell_grid_direct, btn, col, row);
        phandle->bell_grid_btn[i] = btn;
        phandle->bell_grid_check[i] = chk;
    }
}

/* 录音页面板/行：idle 暖灰；聚焦或录音中琥珀整底（真源 AlarmRingtoneRecorderPick） */
static void divoom_astrootoo_alarm_rec_apply_panel_style(lv_obj_t *btn, lv_coord_t radius,
    int is_panel)
{
    uint32 idle_bg = is_panel ? DIVOOM_ASTRO_BG_MUTED : DIVOOM_ASTRO_SURFACE_IDLE;

    lv_obj_set_style_bg_color(btn, lv_color_hex(idle_bg), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn, radius, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS),
        LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(btn, 2, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(btn, lv_color_hex(DIVOOM_ASTRO_ALARM_REC_TAB_INK),
        LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(btn, 0, 0);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
}

static void divoom_astrootoo_alarm_rec_format_clock(uint32 sec, char *buf, int buf_len)
{
    unsigned hh;
    unsigned mm;
    unsigned ss;

    if (!buf || buf_len < 9)
    {
        return;
    }
    if (sec > DIVOOM_ASTRO_ALARM_REC_MAX_SEC)
    {
        sec = DIVOOM_ASTRO_ALARM_REC_MAX_SEC;
    }
    hh = (unsigned)(sec / 3600u);
    mm = (unsigned)((sec % 3600u) / 60u);
    ss = (unsigned)(sec % 60u);
    snprintf(buf, (size_t)buf_len, "%02u:%02u:%02u", hh, mm, ss);
}

static void divoom_astrootoo_alarm_rec_format_date(uint8 month, uint8 day, char *buf, int buf_len)
{
    if (!buf || buf_len < 6)
    {
        return;
    }
    if (month < 1 || day < 1)
    {
        snprintf(buf, (size_t)buf_len, "--");
        return;
    }
    snprintf(buf, (size_t)buf_len, "%02u-%02u", (unsigned)month, (unsigned)day);
}

/* 铃声大类行右侧摘要：有录音文件时显示 MM-DD（对照 formatAlarmRingtoneRecordedDate） */
static void divoom_astrootoo_alarm_rec_cat_summary(Pdivoom_astrootoo_alarm_ctrl phandle, char *buf, int len)
{
    char path[256];
    divoom_port_stat_t st;
    time_t mt;
    struct tm tm_local;

    if (!buf || len <= 0)
    {
        return;
    }
    if (!phandle || !divoom_astrootoo_alarm_rec_file_valid(phandle))
    {
        divoom_astrootoo_alarm_ring_item_label(DIVOOM_ASTRO_RING_CAT_RECORD, 0, buf, len);
        return;
    }
    divoom_astrootoo_alarm_rec_get_path(phandle, path, (int)sizeof(path));
    if (divoom_port_stat(path, &st) != 0 || st.mtime <= 0)
    {
        divoom_astrootoo_alarm_rec_format_date(0, 0, buf, len);
        return;
    }
    mt = (time_t)st.mtime;
    if (localtime_r(&mt, &tm_local) == NULL)
    {
        divoom_astrootoo_alarm_rec_format_date(0, 0, buf, len);
        return;
    }
    divoom_astrootoo_alarm_rec_format_date((uint8)(tm_local.tm_mon + 1),
        (uint8)tm_local.tm_mday, buf, len);
}

static void divoom_astrootoo_alarm_rec_load_clip_date(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    char path[256];
    divoom_port_stat_t st;
    time_t mt;
    struct tm tm_local;

    phandle->rec_clip_month = 0;
    phandle->rec_clip_day = 0;
    if (!phandle->rec_has_clip)
    {
        return;
    }
    divoom_astrootoo_alarm_rec_get_path(phandle, path, (int)sizeof(path));
    if (divoom_port_stat(path, &st) != 0 || st.mtime <= 0)
    {
        return;
    }
    mt = (time_t)st.mtime;
    if (localtime_r(&mt, &tm_local) == NULL)
    {
        return;
    }
    phandle->rec_clip_month = (uint8)(tm_local.tm_mon + 1);
    phandle->rec_clip_day = (uint8)tm_local.tm_mday;
}

static void divoom_astrootoo_alarm_rec_stamp_clip_date_now(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    time_t now;
    struct tm tm_local;

    now = time(NULL);
    if (localtime_r(&now, &tm_local) == NULL)
    {
        phandle->rec_clip_month = 0;
        phandle->rec_clip_day = 0;
        return;
    }
    phandle->rec_clip_month = (uint8)(tm_local.tm_mon + 1);
    phandle->rec_clip_day = (uint8)tm_local.tm_mday;
}

static void divoom_astrootoo_alarm_rec_set_hot(lv_obj_t *obj, int hot, int is_panel)
{
    uint32 idle_bg = is_panel ? DIVOOM_ASTRO_BG_MUTED : DIVOOM_ASTRO_SURFACE_IDLE;

    if (!obj)
    {
        return;
    }
    if (hot)
    {
        lv_obj_set_style_bg_color(obj, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
        lv_obj_set_style_border_color(obj, lv_color_hex(DIVOOM_ASTRO_ALARM_REC_TAB_INK), 0);
    }
    else
    {
        lv_obj_set_style_bg_color(obj, lv_color_hex(idle_bg), 0);
        lv_obj_set_style_border_color(obj, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
    }
}

/* 录音页（真源 AlarmRingtoneRecorderPick：面板 + 计时 + 日期行 + 设为铃声） */
static void divoom_astrootoo_alarm_create_bell_record(Pdivoom_astrootoo_alarm_ctrl phandle, lv_obj_t *parent)
{
    lv_obj_t *list;
    lv_obj_t *panel;
    lv_obj_t *icon_slot;
    lv_obj_t *lab;
    lv_coord_t list_h;
    lv_coord_t stack_h;
    lv_coord_t px = (DIVOOM_SCREEN_WIDTH - DIVOOM_ASTRO_ALARM_REC_PANEL_W) / 2;
    lv_coord_t py;
    lv_coord_t row_y;
    /* 设计 panel：padding 12 + gap10；hint/clock 按 24 字号估高 */
    const lv_coord_t hint_h = 28;
    const lv_coord_t clock_h = 28;
    lv_coord_t content_h;
    lv_coord_t y0;

    list = divoom_astrootoo_alarm_create_sub_list(phandle, parent);
    lv_obj_add_flag(list, LV_OBJ_FLAG_HIDDEN);
    phandle->bell_record_list = list;

    list_h = DIVOOM_SCREEN_HIGH - DIVOOM_ASTRO_ALARM_SUB_TOP;
    stack_h = DIVOOM_ASTRO_ALARM_REC_PANEL_H + DIVOOM_ASTRO_ALARM_REC_GAP
        + DIVOOM_ASTRO_ALARM_REC_ROW_H + DIVOOM_ASTRO_ALARM_REC_GAP
        + DIVOOM_ASTRO_ALARM_REC_ROW_H;
    py = (list_h > stack_h) ? ((list_h - stack_h) / 2) : 8;

    /* 录音面板 448×214 */
    panel = lv_btn_create(list);
    lv_obj_remove_style_all(panel);
    divoom_astrootoo_alarm_rec_apply_panel_style(panel, DIVOOM_ASTRO_RADIUS_SM, 1);
    lv_obj_set_size(panel, DIVOOM_ASTRO_ALARM_REC_PANEL_W, DIVOOM_ASTRO_ALARM_REC_PANEL_H);
    lv_obj_set_pos(panel, px, py);
    lv_obj_add_event_cb(panel, divoom_astrootoo_alarm_rec_key_cb, LV_EVENT_KEY, phandle);
    phandle->rec_panel = panel;

    content_h = hint_h + DIVOOM_ASTRO_ALARM_REC_PANEL_GAP
        + DIVOOM_ASTRO_ALARM_REC_ICON_SLOT + DIVOOM_ASTRO_ALARM_REC_PANEL_GAP
        + clock_h;
    y0 = (DIVOOM_ASTRO_ALARM_REC_PANEL_H > content_h)
        ? ((DIVOOM_ASTRO_ALARM_REC_PANEL_H - content_h) / 2) : 12;

    /* hint：ink + opacity 0.72，字号 24 */
    phandle->rec_title = lv_label_create(panel);
    lv_obj_set_style_text_color(phandle->rec_title, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_opa(phandle->rec_title, DIVOOM_ASTRO_ALARM_REC_HINT_OPA, 0);
    lv_obj_set_style_text_font(phandle->rec_title, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
    lv_obj_set_style_text_align(phandle->rec_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(phandle->rec_title, divoom_menu_control_get_disp_str("VoiceRecordPressToStart"));
    lv_obj_set_width(phandle->rec_title, DIVOOM_ASTRO_ALARM_REC_PANEL_W - 32);
    lv_obj_align(phandle->rec_title, LV_ALIGN_TOP_MID, 0, y0);

    /* 图标槽 80×80，切图 48×48 居中（设计原图 1:1） */
    icon_slot = lv_obj_create(panel);
    lv_obj_remove_style_all(icon_slot);
    lv_obj_set_size(icon_slot, DIVOOM_ASTRO_ALARM_REC_ICON_SLOT, DIVOOM_ASTRO_ALARM_REC_ICON_SLOT);
    lv_obj_set_style_bg_opa(icon_slot, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(icon_slot, 0, 0);
    lv_obj_set_style_pad_all(icon_slot, 0, 0);
    lv_obj_clear_flag(icon_slot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(icon_slot, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(icon_slot, LV_ALIGN_TOP_MID, 0,
        y0 + hint_h + DIVOOM_ASTRO_ALARM_REC_PANEL_GAP);

    phandle->rec_icon_idle = lv_img_create(icon_slot);
    divoom_menu_control_lv_img_set_src(phandle->rec_icon_idle, DIVOOM_ASTROTOO_ALARM_UI_DIR "at_record_idle.png");
    lv_obj_center(phandle->rec_icon_idle);

    phandle->rec_icon_stop = lv_img_create(icon_slot);
    divoom_menu_control_lv_img_set_src(phandle->rec_icon_stop, DIVOOM_ASTROTOO_ALARM_UI_DIR "at_record_stop.png");
    lv_obj_center(phandle->rec_icon_stop);
    lv_obj_add_flag(phandle->rec_icon_stop, LV_OBJ_FLAG_HIDDEN);

    phandle->rec_play_img = lv_img_create(icon_slot);
    divoom_menu_control_lv_img_set_src(phandle->rec_play_img, DIVOOM_ASTROTOO_ALARM_UI_DIR "at_record_playing.png");
    lv_obj_center(phandle->rec_play_img);
    lv_obj_add_flag(phandle->rec_play_img, LV_OBJ_FLAG_HIDDEN);

    /* 计时 HH:MM:SS，字号 24 */
    phandle->rec_time_lbl = lv_label_create(panel);
    lv_obj_set_style_text_color(phandle->rec_time_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(phandle->rec_time_lbl, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
    lv_label_set_text(phandle->rec_time_lbl, "00:00:00");
    lv_obj_align(phandle->rec_time_lbl, LV_ALIGN_TOP_MID, 0,
        y0 + hint_h + DIVOOM_ASTRO_ALARM_REC_PANEL_GAP
        + DIVOOM_ASTRO_ALARM_REC_ICON_SLOT + DIVOOM_ASTRO_ALARM_REC_PANEL_GAP);

    /* 文件行：日期 + 时长（设计 menu 24 / FlowtooRepeatListRow 448×56） */
    row_y = py + DIVOOM_ASTRO_ALARM_REC_PANEL_H + DIVOOM_ASTRO_ALARM_REC_GAP;
    phandle->rec_file_btn = lv_btn_create(list);
    lv_obj_remove_style_all(phandle->rec_file_btn);
    divoom_astrootoo_alarm_rec_apply_panel_style(phandle->rec_file_btn, DIVOOM_ASTRO_RADIUS_SM, 0);
    lv_obj_set_size(phandle->rec_file_btn, DIVOOM_ASTRO_ALARM_REC_PANEL_W, DIVOOM_ASTRO_ALARM_REC_ROW_H);
    lv_obj_set_pos(phandle->rec_file_btn, px, row_y);
    lv_obj_add_event_cb(phandle->rec_file_btn, divoom_astrootoo_alarm_rec_key_cb, LV_EVENT_KEY, phandle);

    phandle->rec_file_title = lv_label_create(phandle->rec_file_btn);
    lv_obj_set_style_text_color(phandle->rec_file_title, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(phandle->rec_file_title, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
    lv_obj_align(phandle->rec_file_title, LV_ALIGN_LEFT_MID, 16, 0);
    lv_label_set_text(phandle->rec_file_title, "--");

    phandle->rec_file_dur = lv_label_create(phandle->rec_file_btn);
    lv_obj_set_style_text_color(phandle->rec_file_dur, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(phandle->rec_file_dur, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
    lv_obj_align(phandle->rec_file_dur, LV_ALIGN_RIGHT_MID, -16, 0);
    lv_label_set_text(phandle->rec_file_dur, "00:00:00");

    /* 录音作为铃声 + 勾选 */
    row_y += DIVOOM_ASTRO_ALARM_REC_ROW_H + DIVOOM_ASTRO_ALARM_REC_GAP;
    phandle->rec_as_bell_btn = lv_btn_create(list);
    lv_obj_remove_style_all(phandle->rec_as_bell_btn);
    divoom_astrootoo_alarm_rec_apply_panel_style(phandle->rec_as_bell_btn, DIVOOM_ASTRO_RADIUS_SM, 0);
    lv_obj_set_size(phandle->rec_as_bell_btn, DIVOOM_ASTRO_ALARM_REC_PANEL_W, DIVOOM_ASTRO_ALARM_REC_ROW_H);
    lv_obj_set_pos(phandle->rec_as_bell_btn, px, row_y);
    lv_obj_add_event_cb(phandle->rec_as_bell_btn, divoom_astrootoo_alarm_rec_key_cb, LV_EVENT_KEY, phandle);

    lab = lv_label_create(phandle->rec_as_bell_btn);
    lv_obj_set_style_text_color(lab, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(lab, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
    lv_obj_align(lab, LV_ALIGN_LEFT_MID, 16, 0);
    lv_label_set_text(lab, divoom_menu_control_get_disp_str("AlarmRecordAsBell"));
    divoom_astrootoo_ui_list_label_prepare(lab,
        divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_ALARM_REC_PANEL_W, 16, 16,
            DIVOOM_ASTRO_LIST_CHK_TRAIL_ALIGN(16)));
    divoom_astrootoo_ui_list_row_bind_marquee(phandle->rec_as_bell_btn, lab);

    phandle->rec_as_bell_check = lv_img_create(phandle->rec_as_bell_btn);
    lv_obj_set_size(phandle->rec_as_bell_check, 24, 24);
    divoom_menu_control_lv_img_set_src(phandle->rec_as_bell_check,
        DIVOOM_ASTROTOO_ALARM_UI_DIR "at_repeat_check.png");
    lv_obj_align(phandle->rec_as_bell_check, LV_ALIGN_RIGHT_MID, -16, 0);
    lv_obj_add_flag(phandle->rec_as_bell_check, LV_OBJ_FLAG_HIDDEN);
}

static void divoom_astrootoo_alarm_create_bell_tf(Pdivoom_astrootoo_alarm_ctrl phandle, lv_obj_t *parent)
{
    lv_obj_t *r;
    lv_obj_t *header;
    lv_obj_t *bg_img;
    lv_obj_t *list;
    int i;

    r = lv_obj_create(parent);
    lv_obj_add_style(r, &phandle->ui.root_style, 0);
    lv_obj_set_size(r, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_pos(r, 0, 0);
    lv_obj_clear_flag(r, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(r, LV_OBJ_FLAG_HIDDEN);
    phandle->bell_tf_root = r;

    header = lv_obj_create(r);
    lv_obj_add_style(header, &phandle->ui.content_style, 0);
    lv_obj_set_size(header, DIVOOM_SCREEN_WIDTH, DIVOOM_ASTRO_HEADER_H);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    bg_img = lv_img_create(header);
    lv_obj_set_size(bg_img, DIVOOM_SCREEN_WIDTH, DIVOOM_ASTRO_HEADER_H);
    divoom_menu_control_lv_img_set_src(bg_img, DIVOOM_ASTROTOO_ALARM_UI_DIR "at_alarm_header.png");

    phandle->bell_tf_title = lv_label_create(header);
    lv_label_set_text(phandle->bell_tf_title, divoom_menu_control_get_disp_str("LocalMusic"));
    lv_obj_set_style_text_color(phandle->bell_tf_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(phandle->bell_tf_title, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
    lv_obj_center(phandle->bell_tf_title);

    list = lv_obj_create(r);
    lv_obj_add_style(list, &phandle->ui.grid_cont_style, 0);
    lv_obj_set_size(list, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH - DIVOOM_ASTRO_ALARM_SUB_TOP);
    lv_obj_set_pos(list, 0, DIVOOM_ASTRO_ALARM_SUB_TOP);
    lv_obj_set_style_pad_top(list, DIVOOM_ASTRO_ALARM_TF_LIST_TOP, 0);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
    phandle->bell_tf_list = list;

    for (i = 0; i < DIVOOM_ASTRO_ALARM_TF_VISIBLE_ROWS; i++)
    {
        phandle->bell_tf_row[i] = divoom_astrootoo_ui_create_tf_list_row(list, &phandle->ui,
            DIVOOM_ASTROTOO_TF_ROW_FOLDER, i, DIVOOM_ASTRO_TF_LIST_ROW_H,
            DIVOOM_ASTRO_ALARM_TF_LIST_TOP, phandle->menu_id,
            &phandle->bell_tf_icon[i], &phandle->bell_tf_label[i],
            &phandle->bell_tf_chevron[i], &phandle->bell_tf_mark[i]);
        lv_obj_add_event_cb(phandle->bell_tf_row[i], divoom_astrootoo_alarm_bell_tf_key_cb,
            LV_EVENT_KEY, phandle);
    }

    phandle->bell_tf_empty_btn = divoom_astrootoo_ui_create_key_capture_btn(r, &phandle->ui,
        divoom_astrootoo_alarm_bell_tf_key_cb, phandle);
    lv_obj_add_flag(phandle->bell_tf_empty_btn, LV_OBJ_FLAG_HIDDEN);

    phandle->bell_tf_built = 1;
}

static void divoom_astrootoo_alarm_create_bell(Pdivoom_astrootoo_alarm_ctrl phandle)
{
    lv_obj_t *r;
    int i;

    r = lv_obj_create(phandle->root_obj);
    lv_obj_add_style(r, &phandle->ui.root_style, 0);
    lv_obj_set_size(r, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_pos(r, 0, 0);
    lv_obj_clear_flag(r, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(r, LV_OBJ_FLAG_HIDDEN);
    phandle->bell_root = r;
    divoom_astrootoo_ui_create_page_header_tab(r, &phandle->ui, "Bells",
        DIVOOM_ASTROTOO_ALARM_UI_DIR "at_alarm_header.png", phandle->menu_id);

    /* 大类列表 */
    phandle->bell_cat_list = divoom_astrootoo_alarm_create_sub_list(phandle, r);
    phandle->bell_cat_direct = divoom_menu_direct_ctrl_create(1, DIVOOM_ASTRO_ALARM_RING_CAT_COUNT);
    for (i = 0; i < DIVOOM_ASTRO_ALARM_RING_CAT_COUNT; i++)
    {
        divoom_astrootoo_alarm_create_bell_cat_row(phandle, phandle->bell_cat_list, i);
        divoom_menu_direct_ctrl_update(phandle->bell_cat_direct, phandle->bell_cat_row[i], 0, i);
    }

#if 0 /* 唤醒场景二级条目列表 UI */
    /* 条目列表（最多 12 行，按大类显示数量） */
    phandle->bell_item_list = divoom_astrootoo_alarm_create_sub_list(phandle, r);
    lv_obj_add_flag(phandle->bell_item_list, LV_OBJ_FLAG_HIDDEN);
    phandle->bell_item_direct = divoom_menu_direct_ctrl_create(1, DIVOOM_ASTRO_ALARM_RING_ITEM_MAX);
    for (i = 0; i < DIVOOM_ASTRO_ALARM_RING_ITEM_MAX; i++)
    {
        divoom_astrootoo_alarm_create_sub_list_row(phandle, phandle->bell_item_list, i, "",
            divoom_astrootoo_alarm_bell_key_cb, &phandle->bell_item_row[i],
            &phandle->bell_item_mark[i], &phandle->bell_item_label[i]);
        divoom_menu_direct_ctrl_update(phandle->bell_item_direct, phandle->bell_item_row[i], 0, i);
    }
#endif

    /* 经典铃声图标网格 + 录音页 + TF 选歌（隐藏的视图，按大类切换） */
    divoom_astrootoo_alarm_create_bell_grid(phandle, r);
    divoom_astrootoo_alarm_create_bell_record(phandle, r);
    divoom_astrootoo_alarm_create_bell_tf(phandle, r);
}

void *divoom_astrootoo_alarm_init(void *user_data, int menu_id)
{
    Pdivoom_astrootoo_alarm_ctrl phandle;
    int i;

    (void)user_data;

    phandle = divoom_system_malloc_out(sizeof(divoom_astrootoo_alarm_ctrl));
    if (!phandle)
    {
        return NULL;
    }

    phandle->menu_id = menu_id;
    phandle->column_focus = DIVOOM_ASTROTOO_ALARM_COL_BODY;
    divoom_astrootoo_alarm_init_slot_styles(phandle);

    phandle->alarm_list = divoom_alarm_menu_get_alarm_list(&phandle->alarm_count);
    if (phandle->alarm_count < 0)
    {
        phandle->alarm_count = 0;
    }

    phandle->row_count = phandle->alarm_count;
    phandle->rows = NULL;
    if (phandle->row_count > 0)
    {
        phandle->rows = divoom_system_malloc(sizeof(divoom_astrootoo_alarm_row) * (size_t)phandle->row_count);
        if (!phandle->rows)
        {
            divoom_astrootoo_alarm_clean(phandle);
            return NULL;
        }
    }

    phandle->root_obj = divoom_astrootoo_ui_create_root(&phandle->ui);
    phandle->list_header = divoom_astrootoo_ui_create_page_header_tab(phandle->root_obj, &phandle->ui,
        "Alarm", DIVOOM_ASTROTOO_ALARM_UI_DIR "at_alarm_header.png", phandle->menu_id);

    phandle->content = lv_obj_create(phandle->root_obj);
    lv_obj_add_style(phandle->content, &phandle->ui.grid_cont_style, 0);
    lv_obj_set_size(phandle->content, DIVOOM_SCREEN_WIDTH,
        DIVOOM_SCREEN_HIGH - DIVOOM_ASTRO_ALARM_LIST_TOP);
    lv_obj_set_pos(phandle->content, 0, DIVOOM_ASTRO_ALARM_LIST_TOP);
    lv_obj_set_style_pad_top(phandle->content, 4, 0);
    lv_obj_set_scrollbar_mode(phandle->content, LV_SCROLLBAR_MODE_OFF);

    phandle->direct_handle = divoom_menu_direct_ctrl_create(1, phandle->row_count);

    for (i = 0; i < phandle->alarm_count; i++)
    {
        divoom_astrootoo_alarm_create_row(phandle, i, i);
    }

    divoom_astrootoo_alarm_create_edit(phandle);
    divoom_astrootoo_alarm_create_time(phandle);
    divoom_astrootoo_alarm_create_repeat(phandle);
    divoom_astrootoo_alarm_create_bell(phandle);
    phandle->mode = DIVOOM_ASTROTOO_ALARM_MODE_LIST;

    phandle->key_btn = divoom_astrootoo_ui_create_key_capture_btn(phandle->root_obj, &phandle->ui,
        divoom_astrootoo_alarm_key_cb, phandle);
    divoom_astrootoo_alarm_set_list_input(phandle);

    phandle->load_timer = lv_timer_create(divoom_astrootoo_alarm_load_assets_cb, 0, phandle);
    if (phandle->load_timer)
    {
        lv_timer_set_repeat_count(phandle->load_timer, 1);
    }

    return phandle;
}

void divoom_astrootoo_alarm_disp(void *handle, void *active_obj)
{
    Pdivoom_astrootoo_alarm_ctrl phandle;

    (void)active_obj;

    phandle = (Pdivoom_astrootoo_alarm_ctrl)handle;
    if (!phandle)
    {
        return;
    }

    lv_scr_load(phandle->root_obj);
    divoom_astrootoo_alarm_enter_list(phandle, 0);
}

void divoom_astrootoo_alarm_clean(void *handle)
{
    Pdivoom_astrootoo_alarm_ctrl phandle;

    phandle = (Pdivoom_astrootoo_alarm_ctrl)handle;
    if (!phandle)
    {
        return;
    }
    if (s_alarm_rec_owner == phandle)
    {
        s_alarm_rec_owner = NULL;
    }

    if (phandle->load_timer)
    {
        lv_timer_del(phandle->load_timer);
        phandle->load_timer = NULL;
    }

    if (phandle->rebuild_timer)
    {
        lv_timer_del(phandle->rebuild_timer);
        phandle->rebuild_timer = NULL;
    }

    divoom_astrootoo_alarm_tf_catalog_timer_stop(phandle);

    if (phandle->direct_handle)
    {
        divoom_menu_direct_ctrl_destory(phandle->direct_handle);
        phandle->direct_handle = NULL;
    }
    if (phandle->edit_direct)
    {
        divoom_menu_direct_ctrl_destory(phandle->edit_direct);
        phandle->edit_direct = NULL;
    }
    if (phandle->rp_direct)
    {
        divoom_menu_direct_ctrl_destory(phandle->rp_direct);
        phandle->rp_direct = NULL;
    }
    if (phandle->bell_cat_direct)
    {
        divoom_menu_direct_ctrl_destory(phandle->bell_cat_direct);
        phandle->bell_cat_direct = NULL;
    }
#if 0 /* 唤醒场景二级条目列表 UI */
    if (phandle->bell_item_direct)
    {
        divoom_menu_direct_ctrl_destory(phandle->bell_item_direct);
        phandle->bell_item_direct = NULL;
    }
#endif
    if (phandle->bell_grid_direct)
    {
        divoom_menu_direct_ctrl_destory(phandle->bell_grid_direct);
        phandle->bell_grid_direct = NULL;
    }
    if (phandle->rec_timer)
    {
        lv_timer_del(phandle->rec_timer);
        phandle->rec_timer = NULL;
    }
    divoom_astrootoo_alarm_rec_icon_pulse_stop(phandle->rec_icon_stop);

    divoom_astrootoo_alarm_bell_stop_preview();

    divoom_astrootoo_alarm_unbind_list_group(phandle);

    if (phandle->key_btn)
    {
        lv_group_remove_obj(phandle->key_btn);
        phandle->key_btn = NULL;
    }

    if (phandle->root_obj)
    {
        lv_obj_del(phandle->root_obj);
        phandle->root_obj = NULL;
    }

    if (phandle->alarm_list)
    {
        divoom_system_free(phandle->alarm_list);
        phandle->alarm_list = NULL;
    }

    if (phandle->rows)
    {
        divoom_system_free(phandle->rows);
        phandle->rows = NULL;
    }

    divoom_astrootoo_alarm_reset_slot_styles(phandle);
    divoom_astrootoo_ui_base_reset_styles(&phandle->ui);
    divoom_system_free(phandle);
}

void divoom_astrootoo_alarm_pause(void *handle)
{
    (void)handle;
}

static Pdivoom_astrootoo_alarm_ctrl divoom_astrootoo_alarm_whitebox_get(void)
{
    int mid;
    void *h;

    h = divoom_menu_control_get_clock_mode_menu_handle();
    if (!h)
    {
        return NULL;
    }
    mid = divoom_menu_control_get_active_menu_id();
    if (mid != DIVOOM_MENU_CONTROL_ID_ALARM_LIST_MENU)
    {
        return NULL;
    }
    return (Pdivoom_astrootoo_alarm_ctrl)h;
}

int divoom_astrootoo_alarm_whitebox_get_mode(void)
{
    Pdivoom_astrootoo_alarm_ctrl phandle;

    phandle = divoom_astrootoo_alarm_whitebox_get();
    if (!phandle)
    {
        return -1;
    }
    return phandle->mode;
}

int divoom_astrootoo_alarm_whitebox_get_bell_phase(void)
{
    Pdivoom_astrootoo_alarm_ctrl phandle;

    phandle = divoom_astrootoo_alarm_whitebox_get();
    if (!phandle)
    {
        return -1;
    }
    return phandle->bell_phase;
}

int divoom_astrootoo_alarm_whitebox_get_bell_cat_focus(void)
{
    Pdivoom_astrootoo_alarm_ctrl phandle;

    phandle = divoom_astrootoo_alarm_whitebox_get();
    if (!phandle)
    {
        return -1;
    }
    return phandle->bell_cat_focus;
}

int divoom_astrootoo_alarm_whitebox_get_edit_focus(void)
{
    Pdivoom_astrootoo_alarm_ctrl phandle;

    phandle = divoom_astrootoo_alarm_whitebox_get();
    if (!phandle)
    {
        return -1;
    }
    return phandle->edit_focus;
}

int divoom_astrootoo_alarm_whitebox_get_rec_focus(void)
{
    Pdivoom_astrootoo_alarm_ctrl phandle;

    phandle = divoom_astrootoo_alarm_whitebox_get();
    if (!phandle)
    {
        return -1;
    }
    return phandle->rec_focus;
}

int divoom_astrootoo_alarm_whitebox_get_rec_capture_active(void)
{
    Pdivoom_astrootoo_alarm_ctrl phandle;

    phandle = divoom_astrootoo_alarm_whitebox_get();
    if (!phandle)
    {
        return -1;
    }
    return phandle->rec_capture_active ? 1 : 0;
}

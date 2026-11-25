#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_menu_direct_ctrl.h"
#include "divoom_main.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_astrootoo_tools.h"
#include "divoom_leave_msg_ctrl.h"
#include "divoom_leave_msg_record.h"
#include "divoom_local_music_ctrl.h"
#include "divoom_voice_read.h"
#include "divoom_lvgl_top_leave_msg_notice_menu.h"
#include "divoom_tomato_time.h"
#include "divoom_lvgl_dial_menu.h"

/* ===== 布局常量（480×480，真源 FlowToo 工具子屏） ===== */
#define DIVOOM_ASTRO_TOOL_BTN_H         72
#define DIVOOM_ASTRO_TOOL_BAR_W         448
#define DIVOOM_ASTRO_TOOL_BAR_GAP       8
#define DIVOOM_ASTRO_TOOL_BAR_Y         388
#define DIVOOM_ASTRO_TOOL_NOISE_BARS        12
#define DIVOOM_ASTRO_TOOL_NOISE_BAR_W       18
#define DIVOOM_ASTRO_TOOL_NOISE_SLOT_W      24
#define DIVOOM_ASTRO_TOOL_NOISE_BAR_GAP     6
#define DIVOOM_ASTRO_TOOL_NOISE_STAGE_H     120
#define DIVOOM_ASTRO_TOOL_NOISE_BAR_MAX_H   46
#define DIVOOM_ASTRO_TOOL_NOISE_BAR_MIN_H   8
#define DIVOOM_ASTRO_TOOL_NOISE_MIRROR_GAP  2
#define DIVOOM_ASTRO_TOOL_NOISE_BAR_SHADE   0xBA8827
#define DIVOOM_ASTRO_TOOL_NOISE_SCALE_H     90
#define DIVOOM_ASTRO_TOOL_NOISE_SCALE_Y     (DIVOOM_SCREEN_HIGH - DIVOOM_ASTRO_TOOL_NOISE_SCALE_H)
#define DIVOOM_ASTRO_TOOL_NOISE_ACTIVE_OPA  56
/* 工具底栏：真源 FlowtooToolActionBarV1 grid 1fr : 1.35fr */
#define DIVOOM_ASTRO_TOOL_ACTION_RESET_W \
    (((DIVOOM_ASTRO_TOOL_BAR_W - DIVOOM_ASTRO_TOOL_BAR_GAP) * 100) / 235)
#define DIVOOM_ASTRO_TOOL_ACTION_PLAY_W \
    ((DIVOOM_ASTRO_TOOL_BAR_W - DIVOOM_ASTRO_TOOL_BAR_GAP) - DIVOOM_ASTRO_TOOL_ACTION_RESET_W)
/* 倒计时：真源 CountdownScreen（分/秒框 200×140 · digit 72 · 双点冒号 · chevron） */
#define DIVOOM_ASTRO_CD_STATUS_Y           (DIVOOM_ASTRO_HEADER_H + 8)
#define DIVOOM_ASTRO_CD_ROW_PAD_TOP        12
#define DIVOOM_ASTRO_CD_SEG_W              200
#define DIVOOM_ASTRO_CD_SEG_H              140
#define DIVOOM_ASTRO_CD_SEG_PAD_X          28
#define DIVOOM_ASTRO_CD_SEG_GAP            10
#define DIVOOM_ASTRO_CD_COLON_W            20
#define DIVOOM_ASTRO_CD_COLON_DOT_SZ       8
#define DIVOOM_ASTRO_CD_COLON_DOT_GAP      20
#define DIVOOM_ASTRO_CD_CHEVRON_W          28
#define DIVOOM_ASTRO_CD_CHEVRON_H          14
#define DIVOOM_ASTRO_CD_CHEVRON_GAP        10
#define DIVOOM_ASTRO_CD_DIGIT_FONT         72
#define DIVOOM_ASTRO_CD_SEG_PAD_TOP        16
#define DIVOOM_ASTRO_CD_DIGIT_CELL_H       72
#define DIVOOM_ASTRO_CD_SEG_INNER_GAP      10
#define DIVOOM_ASTRO_CD_SEG_BODY_W \
    (DIVOOM_ASTRO_CD_SEG_W - DIVOOM_ASTRO_CD_SEG_PAD_X * 2)
#define DIVOOM_ASTRO_CD_UNIT_Y \
    (DIVOOM_ASTRO_CD_SEG_PAD_TOP + DIVOOM_ASTRO_CD_DIGIT_CELL_H + DIVOOM_ASTRO_CD_SEG_INNER_GAP)
#define DIVOOM_ASTRO_CD_ROW_TOTAL_W \
    (DIVOOM_ASTRO_CD_SEG_W * 2 + DIVOOM_ASTRO_CD_SEG_GAP * 2 + DIVOOM_ASTRO_CD_COLON_W)
#define DIVOOM_ASTRO_CD_ROW_X              ((DIVOOM_SCREEN_WIDTH - DIVOOM_ASTRO_CD_ROW_TOTAL_W) / 2)
#define DIVOOM_ASTRO_CD_MIN_X              DIVOOM_ASTRO_CD_ROW_X
#define DIVOOM_ASTRO_CD_COL_X \
    (DIVOOM_ASTRO_CD_MIN_X + DIVOOM_ASTRO_CD_SEG_W + DIVOOM_ASTRO_CD_SEG_GAP)
#define DIVOOM_ASTRO_CD_SEC_X \
    (DIVOOM_ASTRO_CD_COL_X + DIVOOM_ASTRO_CD_COLON_W + DIVOOM_ASTRO_CD_SEG_GAP)
#define DIVOOM_ASTRO_CD_COL_H \
    (DIVOOM_ASTRO_CD_CHEVRON_H + DIVOOM_ASTRO_CD_CHEVRON_GAP \
     + DIVOOM_ASTRO_CD_SEG_H + DIVOOM_ASTRO_CD_CHEVRON_GAP + DIVOOM_ASTRO_CD_CHEVRON_H)
#define DIVOOM_ASTRO_CD_ROW_Y0 \
    (DIVOOM_ASTRO_HEADER_H + DIVOOM_ASTRO_CD_ROW_PAD_TOP)
#define DIVOOM_ASTRO_CD_ROW_Y_CENTER \
    (DIVOOM_ASTRO_CD_ROW_Y0 \
     + (DIVOOM_ASTRO_TOOL_BAR_Y - DIVOOM_ASTRO_CD_ROW_Y0 - DIVOOM_ASTRO_CD_COL_H) / 2)
#define DIVOOM_ASTRO_CD_SEG_Y \
    (DIVOOM_ASTRO_CD_ROW_Y_CENTER + DIVOOM_ASTRO_CD_CHEVRON_H + DIVOOM_ASTRO_CD_CHEVRON_GAP)
/* 正计时 HH:MM:SS：真源 BitmapText 24×4（digit step15→60 / colon step7→28，合计 416）
 * 勿再用单 label + 超大字号（旧 108 时 "00:00:00"≈468 > 卡片 448，会溢出外框）。 */
#define DIVOOM_ASTRO_STOPWATCH_DIGIT_COUNT 6
#define DIVOOM_ASTRO_STOPWATCH_DIGIT_W     60
#define DIVOOM_ASTRO_STOPWATCH_COLON_W     28
#define DIVOOM_ASTRO_STOPWATCH_FONT_SIZE   90
#define DIVOOM_ASTRO_STOPWATCH_TIME_Y      48
#define DIVOOM_ASTRO_STOPWATCH_UNIT_Y      188
#define DIVOOM_ASTRO_STOPWATCH_TIME_W \
    (DIVOOM_ASTRO_STOPWATCH_DIGIT_W * DIVOOM_ASTRO_STOPWATCH_DIGIT_COUNT \
     + DIVOOM_ASTRO_STOPWATCH_COLON_W * 2)

#define DIVOOM_ASTROTOO_TOOLS_UI_DIR    "72/"
#define DIVOOM_ASTROTOO_TOOLS_HEADER    DIVOOM_ASTROTOO_TOOLS_UI_DIR "at_tools_header.png"
#define DIVOOM_ASTROTOO_TOOLS_CHEVRON   DIVOOM_ASTROTOO_TOOLS_UI_DIR "at_tools_chevron.png"
/* create_card_row 沿用 card_item_style pad=8；宽度公式须计入左右 padding */
#define DIVOOM_ASTRO_TOOLS_CARD_PAD           8
#define DIVOOM_ASTRO_TOOLS_CARD_LABEL_INSET_L (12 + DIVOOM_ASTRO_TOOLS_CARD_PAD)
#define DIVOOM_ASTRO_TOOLS_CARD_LABEL_INSET_R (8 + DIVOOM_ASTRO_TOOLS_CARD_PAD)
/* 计时器/倒计时底栏操作钮图标（真源 FlowtooToolActionBarV1：reset/play/pause，84×28） */
#define DIVOOM_ASTROTOO_TOOLS_ICON_RESET DIVOOM_ASTROTOO_TOOLS_UI_DIR "tools_reset.png"
#define DIVOOM_ASTROTOO_TOOLS_ICON_PLAY  DIVOOM_ASTROTOO_TOOLS_UI_DIR "tools_play.png"
#define DIVOOM_ASTROTOO_TOOLS_ICON_PAUSE DIVOOM_ASTROTOO_TOOLS_UI_DIR "tools_pause.png"
#define DIVOOM_ASTROTOO_TOOLS_CD_STEP_UP   DIVOOM_ASTROTOO_TOOLS_UI_DIR "countdown_step_up.png"
#define DIVOOM_ASTROTOO_TOOLS_CD_STEP_DOWN DIVOOM_ASTROTOO_TOOLS_UI_DIR "countdown_step_down.png"
#define DIVOOM_ASTROTOO_TOOLS_ICON_POMO_GEAR DIVOOM_ASTROTOO_TOOLS_UI_DIR "pomo_gear.png"
/* 温馨留言主钮图标：复用助眠录音 + 工具 play/pause */
#define DIVOOM_ASTRO_MEMO_ICON_DIR       "106/"
#define DIVOOM_ASTRO_MEMO_ICON_READY     DIVOOM_ASTRO_MEMO_ICON_DIR "sleep_rec_btn_ready.png"
#define DIVOOM_ASTRO_MEMO_ICON_LIVE      DIVOOM_ASTRO_MEMO_ICON_DIR "sleep_rec_btn_live.png"
#define DIVOOM_ASTRO_MEMO_REC_ZOOM_BASE  ((int)((int32_t)LV_IMG_ZOOM_NONE * 36 / 24))
#define DIVOOM_ASTRO_MEMO_REC_ZOOM_PULSE ((int)(DIVOOM_ASTRO_MEMO_REC_ZOOM_BASE * 108 / 100))
#define DIVOOM_ASTRO_MEMO_REC_PULSE_MS 525
#define DIVOOM_ASTRO_MEMO_METER_BARS     24
#define DIVOOM_ASTRO_MEMO_METER_H        28
#define DIVOOM_ASTRO_MEMO_TAPE_Y         88
#define DIVOOM_ASTRO_MEMO_TAPE_H         268
#define DIVOOM_ASTRO_MEMO_BTN_W          208
#define DIVOOM_ASTRO_MEMO_TIMER_MS       200
#define DIVOOM_ASTRO_MEMO_TICKS_PER_SEC  5

/* 底部操作提示文案（真源 FlowToo 工具子屏底栏 hint） */
#define DIVOOM_ASTRO_TOOL_HINT_Y        450

typedef struct _divoom_astrootoo_tools_ctrl
{
    divoom_astrootoo_ui_base ui;
    int menu_id;
    divoom_astrootoo_tools_page_t page;
    lv_obj_t *root_obj;
    lv_obj_t *content;
    void *direct_handle;
    lv_obj_t *key_btn;
    lv_timer_t *timer;

    /* 时间/状态显示 */
    lv_obj_t *time_lbl;
    lv_obj_t *stopwatch_digit_lbl[DIVOOM_ASTRO_STOPWATCH_DIGIT_COUNT];
    lv_obj_t *status_lbl;

    /* 计时类共用 */
    int running;
    int elapsed_sec;
    int remain_sec;
    int edit_min;
    int edit_sec;
    int focus_idx;
    lv_obj_t *btn_left;     /* 复位 / 减 */
    lv_obj_t *btn_right;    /* 开始|暂停 / 主操作 */
    lv_obj_t *btn_min;      /* 倒计时分 */
    lv_obj_t *btn_sec;      /* 倒计时秒 */
    lv_obj_t *btn_left_lbl;
    lv_obj_t *btn_right_lbl;
    lv_obj_t *btn_left_icon;    /* 复位图标 */
    lv_obj_t *btn_right_icon;   /* 开始|暂停 图标 */
    lv_obj_t *btn_min_lbl;
    lv_obj_t *btn_sec_lbl;

    int cd_status;          /* 0 idle 1 run 2 paused 3 finished */

    /* 番茄 */
    int pomo_phase;         /* 0 work 1 short 2 long */
    int pomo_round;         /* 本轮已完成工作段，喂进度序列（后端 tomato_cur_roll_times） */
    int pomo_today_cnt;     /* 右上角 xN，今日累计（后端 tomato_total_times） */
    int pomo_mode;          /* 0 idle 1 running 2 paused 3 arming(本地预备) */
    int pomo_prep_sec;      /* arming 主钮倒计时，真源 PREP_COUNTDOWN_SEC */
    uint32_t pomo_prep_next_ms; /* arming 下一档倒计时时刻（lv_tick_get），保证每格满 1s */
    int pomo_work_min;
    int pomo_short_min;
    int pomo_long_min;
    int pomo_cycles;        /* 长休前工作段数 1..4（= pomo_repeat_id 映射） */
    int pomo_repeat_id;     /* 重复预设 id 0..4（0 仅一段工作） */
    int pomo_ring;          /* 铃声索引 0..11 */
    int pomo_ring_on;       /* 铃声开关，对应后端 sound_on_off */
    int pomo_tomato_index;  /* tomato_queue 下标，与后端 cur id 对齐 */
    int pomo_view;          /* 0 run 1 settings 2 edit 3 repeat 4 ringtone */
    int pomo_edit_field;    /* 0 work 1 short 2 long */
    int pomo_set_focus;
    int pomo_set_ring_col;  /* 铃声行：0 body 1 toggle（真源 settingsRingColumnFocus） */
    int pomo_prog_n;        /* 进度序列格数 */
    int pomo_last_through;  /* 上次点亮到的下标，避免重复加载图标 */
    int pomo_layout_dirty;  /* 序列长度变化需重新排布 */
    int pomo_motion_frontier; /* 动效 frontier 格下标，-1 无 */
    int pomo_motion_next;     /* 动效 next hint 格下标，-1 无 */
    int pomo_motion_on;       /* 1=running 且应挂动效 */
    lv_coord_t pomo_prog_base_y[12]; /* 进度格静止 Y（呼吸 anim 基准） */
    int pomo_prog_kind[12]; /* 0 work 1 short 2 long */
    lv_obj_t *pomo_count_lbl;   /* 右上角已完成番茄计数 x0 */
    lv_obj_t *pomo_tomato_icon; /* 右上番茄图标 */
    lv_obj_t *pomo_time_card;   /* 大字时间卡片 */
    lv_obj_t *pomo_prog[12];    /* 进度序列图标 */
    lv_obj_t *pomo_set_cont;    /* 设置(齿轮)子页容器 */
    lv_obj_t *pomo_set_row[5];  /* 设置行按钮（第 5 行是铃声复合行外壳） */
    lv_obj_t *pomo_set_val[5];  /* 设置行右侧值标签 */
    lv_obj_t *pomo_set_ring_body;    /* 铃声行左侧 body */
    lv_obj_t *pomo_set_ring_toggle;  /* 铃声行右侧开关槽 */
    lv_obj_t *pomo_set_ring_sw;      /* 开关图标 */
    lv_obj_t *pomo_set_ring_chevron; /* 铃声 body 右侧 chevron，关时隐藏 */
    void *pomo_set_direct;      /* 设置列表网格导航 */
    lv_obj_t *pomo_edit_cont;   /* 单值编辑子页容器 */
    lv_obj_t *pomo_edit_title_lbl;
    lv_obj_t *pomo_edit_val_lbl;
    lv_obj_t *pomo_edit_unit_lbl;
    lv_obj_t *pomo_edit_key_btn;
    /* 重复预设选择子页（图标序列行 + 勾选） */
    lv_obj_t *pomo_repeat_cont;
    lv_obj_t *pomo_repeat_row[5];
    lv_obj_t *pomo_repeat_check[5];
    void *pomo_repeat_direct;
    int pomo_repeat_focus;
    /* 铃声选择子页（3×4 图标栅格，复用闹钟经典铃声图标） */
    lv_obj_t *pomo_ring_cont;
    lv_obj_t *pomo_ring_btn[12];
    lv_obj_t *pomo_ring_check[12];
    void *pomo_ring_direct;
    int pomo_ring_focus;
    /* 运行/暂停时 ESC 退出确认（真源 FlowtooConfirmModal） */
    lv_obj_t *pomo_exit_overlay;
    lv_obj_t *pomo_exit_cancel_btn;
    lv_obj_t *pomo_exit_ok_btn;
    lv_obj_t *pomo_exit_focus_restore;
    int pomo_exit_focus;    /* 0 继续专注 1 结束退出 */
    /* 阶段到时 256×256 完成动画（无 480 底） */
    lv_obj_t *pomo_anim_win;
    void *pomo_anim_disp_handle;
    lv_timer_t *pomo_anim_close_timer;
    lv_obj_t *pomo_anim_focus_restore;
    int pomo_anim_shell_flag;   /* 表盘已退出时自建的 dial menu 外壳，用完须自己拆 */

    /* 记分板 */
    int score_a;
    int score_b;
    int score_sel;
    lv_obj_t *score_a_card;
    lv_obj_t *score_b_card;
    lv_obj_t *score_a_lbl;
    lv_obj_t *score_b_lbl;

    /* 噪音 */
    int noise_db;
    uint32_t noise_phase;
    lv_obj_t *noise_bars[DIVOOM_ASTRO_TOOL_NOISE_BARS];
    lv_obj_t *noise_bar_reflect[DIVOOM_ASTRO_TOOL_NOISE_BARS];
    lv_obj_t *noise_scale[5];
    lv_obj_t *noise_scale_val[5];
    lv_obj_t *noise_scale_name[5];

    /* 留言/录音（真源 WarmMemoTapeDeck：磁带卡 + 读数 + 假电平 + 主钮） */
    int memo_state;         /* 0 idle 1 recording 2 playing */
    int memo_elapsed;
    int memo_clip;
    int memo_tick;          /* 200ms 节拍，开录等待超时用 */
    uint32 memo_start_tick_ms;      /* 0=未在录/已冻住 */
    uint32 memo_play_start_tick_ms;
    uint8 memo_play_armed;  /* tone 已真正起播 */
    uint8 memo_pending;     /* 0 idle; 1 异步开录等待 ctrl/mic */
    uint8 memo_start_wait;  /* 开录等待 mic 超时计数（秒） */
    uint32_t memo_meter_seed;
    lv_obj_t *memo_tape;
    lv_obj_t *memo_readout;
    lv_obj_t *memo_meter_row;
    lv_obj_t *memo_meter[DIVOOM_ASTRO_MEMO_METER_BARS];
    lv_obj_t *memo_btn;
    lv_obj_t *memo_btn_lbl;
    lv_obj_t *memo_btn_icon;
} divoom_astrootoo_tools_ctrl, *Pdivoom_astrootoo_tools_ctrl;

/* ===== 列表项定义 ===== */
typedef struct
{
    const char *label_key;
    int menu_id;
} divoom_astrootoo_tools_row_t;

static const divoom_astrootoo_tools_row_t g_tools_rows[] =
{
    { "Pomodoro",        DIVOOM_MENU_CONTROL_ID_POMODORO_MENU },
    { "StopWatch",       DIVOOM_MENU_CONTROL_ID_STOP_WATCH_MENU },
    { "Countdown",       DIVOOM_MENU_CONTROL_ID_COUNT_DOWN_MENU },
    { "Scoreboard",      DIVOOM_MENU_CONTROL_ID_SCORE_BOARD_MENU },
    { "VoiceRecordTool", DIVOOM_MENU_CONTROL_ID_VOICE_RECORD_TOOL_MENU },
    { "Noisemeter",      DIVOOM_MENU_CONTROL_ID_NOISE_MEASURE_MENU },
};

#define DIVOOM_ASTRO_TOOLS_ROW_CNT (int)(sizeof(g_tools_rows) / sizeof(g_tools_rows[0]))

/* ===== 工具函数 ===== */
static divoom_astrootoo_tools_page_t divoom_astrootoo_tools_page_from_menu_id(int menu_id)
{
    switch (menu_id)
    {
    case DIVOOM_MENU_CONTROL_ID_POMODORO_MENU:
    {
        return DIVOOM_ASTROTOO_TOOLS_PAGE_POMODORO;
    }
    case DIVOOM_MENU_CONTROL_ID_STOP_WATCH_MENU:
    {
        return DIVOOM_ASTROTOO_TOOLS_PAGE_STOPWATCH;
    }
    case DIVOOM_MENU_CONTROL_ID_COUNT_DOWN_MENU:
    {
        return DIVOOM_ASTROTOO_TOOLS_PAGE_COUNTDOWN;
    }
    case DIVOOM_MENU_CONTROL_ID_SCORE_BOARD_MENU:
    {
        return DIVOOM_ASTROTOO_TOOLS_PAGE_SCOREBOARD;
    }
    case DIVOOM_MENU_CONTROL_ID_VOICE_RECORD_TOOL_MENU:
    {
        return DIVOOM_ASTROTOO_TOOLS_PAGE_MEMO;
    }
    case DIVOOM_MENU_CONTROL_ID_NOISE_MEASURE_MENU:
    {
        return DIVOOM_ASTROTOO_TOOLS_PAGE_NOISE;
    }
    default:
    {
        return DIVOOM_ASTROTOO_TOOLS_PAGE_LIST;
    }
    }
}

static void divoom_astrootoo_tools_fmt_hms(char *buf, int n, int total_sec)
{
    int h;
    int m;
    int s;

    if (total_sec < 0)
    {
        total_sec = 0;
    }
    h = total_sec / 3600;
    m = (total_sec / 60) % 60;
    s = total_sec % 60;
    lv_snprintf(buf, n, "%02d:%02d:%02d", h, m, s);
}

static void divoom_astrootoo_tools_fmt_ms(char *buf, int n, int total_sec)
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

/* 大号居中数字 label */
static lv_obj_t *divoom_astrootoo_tools_create_big_time(Pdivoom_astrootoo_tools_ctrl phandle,
    lv_obj_t *parent, int font_size, int y, lv_color_t color)
{
    lv_obj_t *lbl;

    lbl = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(font_size, phandle->menu_id), 0);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_label_set_text(lbl, "00:00");
    lv_obj_set_width(lbl, DIVOOM_SCREEN_WIDTH);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(lbl, 0, y);
    return lbl;
}

/* 底栏操作按钮（卡片 + 焦点琥珀），返回 btn，label 通过 out 带出 */
static lv_obj_t *divoom_astrootoo_tools_create_action_btn(Pdivoom_astrootoo_tools_ctrl phandle,
    lv_obj_t *parent, const char *label_key, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h,
    lv_obj_t **out_lbl)
{
    lv_obj_t *btn;
    lv_obj_t *lbl;

    btn = lv_btn_create(parent);
    lv_obj_add_style(btn, &phandle->ui.card_item_style, 0);
    lv_obj_add_style(btn, &phandle->ui.card_item_focus_style, LV_STATE_FOCUSED);
    lv_obj_add_style(btn, &phandle->ui.card_item_focus_style, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_pos(btn, x, y);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    lbl = lv_label_create(btn);
    lv_label_set_text(lbl, divoom_menu_control_get_disp_str(label_key));
    lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
    lv_obj_center(lbl);

    if (out_lbl)
    {
        *out_lbl = lbl;
    }
    return btn;
}

/* 图标操作钮（真源 FlowtooToolActionBarV1：复位/播放/暂停 用图标，不用文字） */
static lv_obj_t *divoom_astrootoo_tools_create_icon_action_btn(Pdivoom_astrootoo_tools_ctrl phandle,
    lv_obj_t *parent, const char *icon_file, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h,
    lv_obj_t **out_icon)
{
    lv_obj_t *btn;
    lv_obj_t *icon;

    btn = lv_btn_create(parent);
    lv_obj_add_style(btn, &phandle->ui.card_item_style, 0);
    lv_obj_add_style(btn, &phandle->ui.card_item_focus_style, LV_STATE_FOCUSED);
    lv_obj_add_style(btn, &phandle->ui.card_item_focus_style, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_pos(btn, x, y);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    icon = lv_img_create(btn);
    divoom_menu_control_lv_img_set_src(icon, icon_file);
    lv_obj_center(icon);

    if (out_icon)
    {
        *out_icon = icon;
    }
    return btn;
}

/* 深色 H2 顶栏（真源 FlowtooPageHeaderTab：#243746 + 白字 32px） */
static void divoom_astrootoo_tools_create_header(Pdivoom_astrootoo_tools_ctrl phandle, const char *title_key)
{
    divoom_astrootoo_ui_create_page_header_tab(phandle->root_obj, &phandle->ui,
        title_key, DIVOOM_ASTROTOO_TOOLS_HEADER, phandle->menu_id);
}

/* 列表行右侧箭头（真源 SettingsModuleRow chevron） */
static void divoom_astrootoo_tools_add_chevron(lv_obj_t *btn)
{
    lv_obj_t *chev;

    chev = lv_img_create(btn);
    divoom_astrootoo_ui_apply_sublevel_chevron_size(chev);
    divoom_menu_control_lv_img_set_src(chev, DIVOOM_ASTROTOO_TOOLS_CHEVRON);
    lv_obj_align(chev, LV_ALIGN_RIGHT_MID, -8, 0);
}

/* 底部操作提示（真源工具子屏底栏 hint：左右 切换 · 确定 · 返回 退出 等） */
static void divoom_astrootoo_tools_create_hint(Pdivoom_astrootoo_tools_ctrl phandle, const char *hint_key)
{
    lv_obj_t *lbl;

    lbl = lv_label_create(phandle->root_obj);
    lv_label_set_text(lbl, divoom_menu_control_get_disp_str(hint_key));
    lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(20, phandle->menu_id), 0);
    lv_obj_set_width(lbl, DIVOOM_SCREEN_WIDTH);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(lbl, 0, DIVOOM_ASTRO_TOOL_HINT_Y);
}

/* 灰底圆角卡片（真源 StopwatchScreen.timeCard：#DDD8D0 + 2px stroke + radius 12）
 * pad 必须为 0：子控件按卡片外宽绝对定位，沿用 card_item_style 的 pad=8 会挤爆内容区。 */
static lv_obj_t *divoom_astrootoo_tools_create_card(Pdivoom_astrootoo_tools_ctrl phandle,
    lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *card;

    card = lv_obj_create(phandle->root_obj);
    lv_obj_add_style(card, &phandle->ui.card_item_style, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_size(card, w, h);
    lv_obj_set_pos(card, x, y);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

/* 时/分/秒 单位行：与上方 HH/MM/SS 两位数槽对齐（真源 StopwatchScreen.segment） */
static void divoom_astrootoo_tools_create_unit_row(Pdivoom_astrootoo_tools_ctrl phandle,
    lv_obj_t *parent, const char *k0, const char *k1, const char *k2, lv_coord_t y)
{
    const char *keys[3];
    lv_coord_t x0;
    lv_coord_t pair_w;
    lv_coord_t pitch;
    int i;

    keys[0] = k0;
    keys[1] = k1;
    keys[2] = k2;
    x0 = (DIVOOM_ASTRO_TOOL_BAR_W - DIVOOM_ASTRO_STOPWATCH_TIME_W) / 2;
    pair_w = DIVOOM_ASTRO_STOPWATCH_DIGIT_W * 2;
    pitch = pair_w + DIVOOM_ASTRO_STOPWATCH_COLON_W;
    for (i = 0; i < 3; i++)
    {
        lv_obj_t *u;

        if (!keys[i])
        {
            continue;
        }
        u = lv_label_create(parent);
        lv_label_set_text(u, divoom_menu_control_get_disp_str(keys[i]));
        lv_obj_set_style_text_color(u, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
        lv_obj_set_style_text_font(u, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
        lv_obj_set_width(u, pair_w);
        lv_obj_set_style_text_align(u, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_pos(u, x0 + i * pitch, y);
    }
}

static void divoom_astrootoo_tools_countdown_add_chevron(lv_obj_t *parent, lv_coord_t seg_x,
    lv_coord_t y, const char *icon_file)
{
    lv_obj_t *img;
    lv_coord_t cx;

    cx = seg_x + DIVOOM_ASTRO_CD_SEG_W / 2;
    img = lv_img_create(parent);
    divoom_menu_control_lv_img_set_src(img, icon_file);
    lv_obj_set_size(img, DIVOOM_ASTRO_CD_CHEVRON_W, DIVOOM_ASTRO_CD_CHEVRON_H);
    lv_obj_set_pos(img, cx - DIVOOM_ASTRO_CD_CHEVRON_W / 2, y);
    lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
}

static void divoom_astrootoo_tools_countdown_add_colon(lv_obj_t *parent, lv_coord_t col_x,
    lv_coord_t center_y)
{
    lv_coord_t dot_x;
    lv_coord_t dot_y0;
    int i;

    dot_x = col_x + (DIVOOM_ASTRO_CD_COLON_W - DIVOOM_ASTRO_CD_COLON_DOT_SZ) / 2;
    dot_y0 = center_y - DIVOOM_ASTRO_CD_COLON_DOT_GAP / 2 - DIVOOM_ASTRO_CD_COLON_DOT_SZ;
    for (i = 0; i < 2; i++)
    {
        lv_obj_t *dot;

        dot = lv_obj_create(parent);
        lv_obj_set_size(dot, DIVOOM_ASTRO_CD_COLON_DOT_SZ, DIVOOM_ASTRO_CD_COLON_DOT_SZ);
        lv_obj_set_pos(dot, dot_x,
            dot_y0 + i * (DIVOOM_ASTRO_CD_COLON_DOT_SZ + DIVOOM_ASTRO_CD_COLON_DOT_GAP));
        lv_obj_set_style_bg_color(dot, lv_color_hex(DIVOOM_ASTRO_INK), 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_set_style_pad_all(dot, 0, 0);
        lv_obj_set_style_radius(dot, 0, 0);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    }
}

/* 分/秒框：digit_cell 透明容器 + lv_obj_center，避免大字 label 直接定位视觉偏上 */
static lv_obj_t *divoom_astrootoo_tools_create_countdown_segment(
    Pdivoom_astrootoo_tools_ctrl phandle, lv_coord_t x, lv_coord_t y,
    const char *unit_key, lv_obj_t **out_lbl)
{
    lv_obj_t *btn;
    lv_obj_t *lbl;
    lv_obj_t *unit;
    lv_obj_t *digit_cell;

    btn = lv_btn_create(phandle->root_obj);
    lv_obj_add_style(btn, &phandle->ui.card_item_style, 0);
    lv_obj_add_style(btn, &phandle->ui.card_item_focus_style, LV_STATE_FOCUSED);
    lv_obj_add_style(btn, &phandle->ui.card_item_focus_style, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_radius(btn, 16, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_pad_all(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_size(btn, DIVOOM_ASTRO_CD_SEG_W, DIVOOM_ASTRO_CD_SEG_H);
    lv_obj_set_pos(btn, x, y);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    digit_cell = lv_obj_create(btn);
    lv_obj_set_style_bg_opa(digit_cell, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(digit_cell, 0, 0);
    lv_obj_set_style_pad_all(digit_cell, 0, 0);
    lv_obj_set_size(digit_cell, DIVOOM_ASTRO_CD_SEG_BODY_W, DIVOOM_ASTRO_CD_DIGIT_CELL_H);
    lv_obj_set_pos(digit_cell, DIVOOM_ASTRO_CD_SEG_PAD_X, DIVOOM_ASTRO_CD_SEG_PAD_TOP);
    lv_obj_clear_flag(digit_cell, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lbl = lv_label_create(digit_cell);
    lv_label_set_text(lbl, "00");
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(lbl,
        divoom_menu_control_get_font_info(DIVOOM_ASTRO_CD_DIGIT_FONT, phandle->menu_id), 0);
    lv_obj_center(lbl);

    unit = lv_label_create(btn);
    lv_label_set_text(unit, divoom_menu_control_get_disp_str(unit_key));
    lv_label_set_long_mode(unit, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(unit, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(unit, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
    lv_obj_set_width(unit, DIVOOM_ASTRO_CD_SEG_BODY_W);
    lv_obj_set_style_text_align(unit, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(unit, DIVOOM_ASTRO_CD_SEG_PAD_X, DIVOOM_ASTRO_CD_UNIT_Y);

    if (out_lbl)
    {
        *out_lbl = lbl;
    }
    return btn;
}

/* ====================================================================== */
/* 列表页                                                                  */
/* ====================================================================== */
static void divoom_astrootoo_tools_list_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_tools_ctrl phandle;
    lv_obj_t *target;
    lv_obj_t *next;
    uint8_t key_code;
    int pos;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }

    phandle = (Pdivoom_astrootoo_tools_ctrl)lv_event_get_user_data(e);
    target = lv_event_get_target(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);

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
            divoom_astrootoo_ui_focus_scroll(next);
        }
        return;
    }

    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        pos = divoom_menu_direct_ctrl_get_pos(phandle->direct_handle, target);
        if (pos >= 0 && pos < DIVOOM_ASTRO_TOOLS_ROW_CNT)
        {
            divoom_menu_control_load_next(g_tools_rows[pos].menu_id, NULL, 0, 0);
        }
    }
}

static void divoom_astrootoo_tools_group_bind_list(Pdivoom_astrootoo_tools_ctrl phandle,
    lv_obj_t *focus_target)
{
    lv_group_t *g;
    uint32_t i;
    uint32_t n;
    lv_obj_t *first_btn;
    lv_obj_t *ch;
    lv_obj_t *focus;

    if (!phandle || !phandle->content)
    {
        return;
    }
    g = lv_group_get_default();
    lv_group_remove_all_objs(g);
    first_btn = NULL;
    n = lv_obj_get_child_cnt(phandle->content);
    for (i = 0; i < n; i++)
    {
        ch = lv_obj_get_child(phandle->content, i);
        if (!ch)
        {
            continue;
        }
        lv_group_add_obj(g, ch);
        if (!first_btn)
        {
            first_btn = ch;
        }
    }
    focus = NULL;
    if (focus_target && lv_obj_is_valid(focus_target))
    {
        focus = focus_target;
    }
    else if (first_btn)
    {
        focus = first_btn;
    }
    if (focus)
    {
        lv_group_focus_obj(focus);
        divoom_astrootoo_ui_focus_scroll(focus);
    }
}

/* 闹钟响铃等 overlay 会 lv_group_remove_all_objs；return_pre 后必须重新入组再聚焦 */
static void divoom_astrootoo_tools_group_bind_objs(lv_obj_t **objs, int n, lv_obj_t *focus)
{
    lv_group_t *g;
    int i;

    g = lv_group_get_default();
    if (!g || !objs || n <= 0)
    {
        return;
    }

    lv_group_remove_all_objs(g);
    for (i = 0; i < n; i++)
    {
        if (objs[i] && lv_obj_is_valid(objs[i]))
        {
            lv_group_add_obj(g, objs[i]);
        }
    }
    if (focus && lv_obj_is_valid(focus))
    {
        lv_group_focus_obj(focus);
        divoom_astrootoo_ui_focus_scroll(focus);
    }
}

static void divoom_astrootoo_tools_build_list(Pdivoom_astrootoo_tools_ctrl phandle)
{
    int i;
    lv_obj_t *btn;

    phandle->direct_handle = divoom_menu_direct_ctrl_create(1, DIVOOM_ASTRO_TOOLS_ROW_CNT);
    divoom_astrootoo_tools_create_header(phandle, "Tool Box");
    phandle->content = divoom_astrootoo_ui_create_scroll_content(phandle->root_obj, &phandle->ui);

    for (i = 0; i < DIVOOM_ASTRO_TOOLS_ROW_CNT; i++)
    {
        lv_obj_t *lbl;

        btn = divoom_astrootoo_ui_create_card_row(phandle->content, &phandle->ui,
            g_tools_rows[i].label_key, i, DIVOOM_ASTRO_TOOLS_ROW_CNT, phandle->direct_handle,
            phandle->menu_id, 1);
        divoom_astrootoo_tools_add_chevron(btn);
        lbl = lv_obj_get_child(btn, 0);
        if (lbl)
        {
            divoom_astrootoo_ui_list_label_prepare(lbl,
                divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_CARD_ROW_W,
                    DIVOOM_ASTRO_TOOLS_CARD_LABEL_INSET_L,
                    DIVOOM_ASTRO_TOOLS_CARD_LABEL_INSET_R,
                    DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W));
            divoom_astrootoo_ui_list_row_bind_marquee(btn, lbl);
        }
        lv_obj_add_event_cb(btn, divoom_astrootoo_tools_list_key_cb, LV_EVENT_KEY, phandle);
    }

    divoom_astrootoo_tools_group_bind_list(phandle, NULL);
}

/* ====================================================================== */
/* 计时器 / 倒计时 / 番茄 共用底栏焦点切换                                  */
/* ====================================================================== */
static void divoom_astrootoo_tools_focus_lr(Pdivoom_astrootoo_tools_ctrl phandle, int idx)
{
    phandle->focus_idx = idx;
    if (idx == 0 && phandle->btn_left)
    {
        lv_group_focus_obj(phandle->btn_left);
    }
    else if (idx == 1 && phandle->btn_right)
    {
        lv_group_focus_obj(phandle->btn_right);
    }
}

/* ====================================================================== */
/* 正计时（Stopwatch）                                                     */
/* 与 App Tools/SetStopWatch / divoom_app_tool 同步                        */
/* APP/模型线程只改后台；LVGL 对象仅在本页 timer / key_cb 刷新             */
/* ====================================================================== */
#define DIVOOM_ASTRO_STOPWATCH_POLL_MS  200
#define DIVOOM_ASTRO_STOPWATCH_MAX_SEC  359999

static void divoom_astrootoo_tools_stopwatch_push_to_tool(uint8 ctrl_flag)
{
    divoom_app_tool_time_watch_info info = {0};

    /* 0 暂停 1 开始 2 重置；msg_type=0 时 set_time_watch 同步进 TIME_WATCH_SET */
    info.ctrl_flag = ctrl_flag;
    divoom_app_tool_set_time_watch(&info);
}

static void divoom_astrootoo_tools_stopwatch_load_from_tool(Pdivoom_astrootoo_tools_ctrl phandle)
{
    divoom_disp_ctrl_watch_ctrl watch_info = {0};
    int sec;

    divoom_app_tool_get_watch_cnt(&watch_info);
    phandle->running = (watch_info.ctrl_flag == 1) ? 1 : 0;
    sec = (int)(watch_info.watch_disp_value / 100);
    if (sec < 0)
    {
        sec = 0;
    }
    if (sec > DIVOOM_ASTRO_STOPWATCH_MAX_SEC)
    {
        sec = DIVOOM_ASTRO_STOPWATCH_MAX_SEC;
    }
    phandle->elapsed_sec = sec;
}

static void divoom_astrootoo_tools_stopwatch_refresh(Pdivoom_astrootoo_tools_ctrl phandle)
{
    char buf[16];
    char digit[2];
    static const uint8 digit_pos[DIVOOM_ASTRO_STOPWATCH_DIGIT_COUNT] = { 0, 1, 3, 4, 6, 7 };
    int i;

    divoom_astrootoo_tools_fmt_hms(buf, sizeof(buf), phandle->elapsed_sec);
    digit[1] = '\0';
    for (i = 0; i < DIVOOM_ASTRO_STOPWATCH_DIGIT_COUNT; i++)
    {
        if (phandle->stopwatch_digit_lbl[i])
        {
            digit[0] = buf[digit_pos[i]];
            lv_label_set_text(phandle->stopwatch_digit_lbl[i], digit);
        }
    }
    if (phandle->btn_right_icon)
    {
        divoom_menu_control_lv_img_set_src(phandle->btn_right_icon,
            phandle->running ? DIVOOM_ASTROTOO_TOOLS_ICON_PAUSE : DIVOOM_ASTROTOO_TOOLS_ICON_PLAY);
        lv_obj_center(phandle->btn_right_icon);
    }
}

static void divoom_astrootoo_tools_create_stopwatch_time(Pdivoom_astrootoo_tools_ctrl phandle,
    lv_obj_t *card)
{
    static const char initial_time[] = "00:00:00";
    lv_coord_t x;
    lv_coord_t slot_w;
    lv_obj_t *lbl;
    int digit_idx;
    int i;

    /* 六位数字 + 两冒号固定槽（合计 STOPWATCH_TIME_W=416），居中于 448 卡片内。 */
    x = (DIVOOM_ASTRO_TOOL_BAR_W - DIVOOM_ASTRO_STOPWATCH_TIME_W) / 2;
    digit_idx = 0;
    for (i = 0; i < 8; i++)
    {
        slot_w = (initial_time[i] == ':')
            ? DIVOOM_ASTRO_STOPWATCH_COLON_W : DIVOOM_ASTRO_STOPWATCH_DIGIT_W;
        lbl = lv_label_create(card);
        lv_label_set_text_fmt(lbl, "%c", initial_time[i]);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
        lv_obj_set_style_text_font(lbl,
            divoom_menu_control_get_font_info(DIVOOM_ASTRO_STOPWATCH_FONT_SIZE, phandle->menu_id), 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(lbl, slot_w);
        lv_obj_set_pos(lbl, x, DIVOOM_ASTRO_STOPWATCH_TIME_Y);
        if (initial_time[i] != ':' && digit_idx < DIVOOM_ASTRO_STOPWATCH_DIGIT_COUNT)
        {
            phandle->stopwatch_digit_lbl[digit_idx] = lbl;
            digit_idx++;
        }
        x += slot_w;
    }
}

static void divoom_astrootoo_tools_stopwatch_timer_cb(lv_timer_t *t)
{
    Pdivoom_astrootoo_tools_ctrl phandle;
    int old_running;
    int old_elapsed;

    phandle = (Pdivoom_astrootoo_tools_ctrl)lv_timer_get_user_data(t);
    if (!phandle || phandle->page != DIVOOM_ASTROTOO_TOOLS_PAGE_STOPWATCH)
    {
        return;
    }

    old_running = phandle->running;
    old_elapsed = phandle->elapsed_sec;
    divoom_astrootoo_tools_stopwatch_load_from_tool(phandle);
    if (phandle->running == old_running && phandle->elapsed_sec == old_elapsed)
    {
        return;
    }
    divoom_astrootoo_tools_stopwatch_refresh(phandle);
}

static void divoom_astrootoo_tools_stopwatch_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_tools_ctrl phandle;
    lv_obj_t *target;
    uint8_t key_code;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }

    phandle = (Pdivoom_astrootoo_tools_ctrl)lv_event_get_user_data(e);
    target = lv_event_get_target(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_menu_control_return_pre();
        return;
    }
    if (divoom_astrootoo_ui_key_is_left(key_code) || divoom_astrootoo_ui_key_is_prev(key_code))
    {
        divoom_astrootoo_tools_focus_lr(phandle, 0);
        return;
    }
    if (divoom_astrootoo_ui_key_is_right(key_code) || divoom_astrootoo_ui_key_is_next(key_code))
    {
        divoom_astrootoo_tools_focus_lr(phandle, 1);
        return;
    }
    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        if (target == phandle->btn_left)
        {
            divoom_astrootoo_tools_stopwatch_push_to_tool(2);
        }
        else
        {
            divoom_astrootoo_tools_stopwatch_push_to_tool(phandle->running ? 0 : 1);
        }
        divoom_astrootoo_tools_stopwatch_load_from_tool(phandle);
        divoom_astrootoo_tools_stopwatch_refresh(phandle);
    }
}

static void divoom_astrootoo_tools_build_stopwatch(Pdivoom_astrootoo_tools_ctrl phandle)
{
    lv_obj_t *card;

    divoom_astrootoo_tools_create_header(phandle, "StopWatch");

    card = divoom_astrootoo_tools_create_card(phandle, DIVOOM_ASTRO_LIST_PAD_X, 76,
        DIVOOM_ASTRO_TOOL_BAR_W, 256);
    divoom_astrootoo_tools_create_stopwatch_time(phandle, card);
    divoom_astrootoo_tools_create_unit_row(phandle, card, "Hr", "Min", "Sec",
        DIVOOM_ASTRO_STOPWATCH_UNIT_Y);

    phandle->btn_left_lbl = NULL;
    phandle->btn_right_lbl = NULL;
    phandle->btn_left = divoom_astrootoo_tools_create_icon_action_btn(phandle, phandle->root_obj,
        DIVOOM_ASTROTOO_TOOLS_ICON_RESET, DIVOOM_ASTRO_LIST_PAD_X, DIVOOM_ASTRO_TOOL_BAR_Y,
        DIVOOM_ASTRO_TOOL_ACTION_RESET_W, DIVOOM_ASTRO_TOOL_BTN_H, &phandle->btn_left_icon);
    phandle->btn_right = divoom_astrootoo_tools_create_icon_action_btn(phandle, phandle->root_obj,
        DIVOOM_ASTROTOO_TOOLS_ICON_PLAY,
        DIVOOM_ASTRO_LIST_PAD_X + DIVOOM_ASTRO_TOOL_ACTION_RESET_W + DIVOOM_ASTRO_TOOL_BAR_GAP,
        DIVOOM_ASTRO_TOOL_BAR_Y, DIVOOM_ASTRO_TOOL_ACTION_PLAY_W, DIVOOM_ASTRO_TOOL_BTN_H,
        &phandle->btn_right_icon);

    lv_obj_add_event_cb(phandle->btn_left, divoom_astrootoo_tools_stopwatch_key_cb, LV_EVENT_KEY, phandle);
    lv_obj_add_event_cb(phandle->btn_right, divoom_astrootoo_tools_stopwatch_key_cb, LV_EVENT_KEY, phandle);
    lv_group_add_obj(lv_group_get_default(), phandle->btn_left);
    lv_group_add_obj(lv_group_get_default(), phandle->btn_right);

    //divoom_astrootoo_tools_create_hint(phandle, "ToolHintLR");
    divoom_astrootoo_tools_stopwatch_load_from_tool(phandle);
    divoom_astrootoo_tools_stopwatch_refresh(phandle);
    divoom_astrootoo_tools_focus_lr(phandle, 1);
    phandle->timer = lv_timer_create(divoom_astrootoo_tools_stopwatch_timer_cb,
        DIVOOM_ASTRO_STOPWATCH_POLL_MS, phandle);
}

/* ====================================================================== */
/* 倒计时（Countdown）                                                     */
/* 与 App Tools/SetTimer / divoom_app_tool 同步                            */
/* ====================================================================== */
#define DIVOOM_ASTRO_COUNTDOWN_POLL_MS  200

static void divoom_astrootoo_tools_countdown_push_to_tool(Pdivoom_astrootoo_tools_ctrl phandle, uint8 status)
{
    divoom_app_tool_count_time_down_info info = {0};
    int total;

    if (status == 1)
    {
        if (phandle->cd_status == 0)
        {
            total = phandle->edit_min * 60 + phandle->edit_sec;
        }
        else
        {
            total = phandle->remain_sec;
        }
        if (total < 0)
        {
            total = 0;
        }
        info.minute = (uint8)(total / 60);
        info.second = (uint8)(total % 60);
        info.status = 1;
    }
    else if (status == 0)
    {
        total = phandle->remain_sec;
        if (total < 0)
        {
            total = 0;
        }
        info.minute = (uint8)(total / 60);
        info.second = (uint8)(total % 60);
        info.status = 0;
    }
    else
    {
        info.minute = 1;
        info.second = 0;
        info.status = 2;
    }
    info.lvgl_flag = 1;
    divoom_app_tool_set_count_time_down(&info);
}

static void divoom_astrootoo_tools_countdown_load_from_tool(Pdivoom_astrootoo_tools_ctrl phandle)
{
    uint8 st;
    int remain;

    st = divoom_app_tool_get_count_down_status();
    remain = (int)divoom_app_tool_get_count_down_cnt();
    if (remain < 0)
    {
        remain = 0;
    }

    if (st == DIVOOM_DISP_COUNT_TIME_DOWN_RUN)
    {
        phandle->cd_status = 1;
        phandle->remain_sec = remain;
        if (remain > 0)
        {
            phandle->edit_min = remain / 60;
            phandle->edit_sec = remain % 60;
        }
    }
    else if (st == DIVOOM_DISP_COUNT_TIME_DOWN_TIME_OVER)
    {
        phandle->cd_status = 3;
        phandle->remain_sec = 0;
    }
    else if (st == DIVOOM_DISP_COUNT_TIME_DOWN_PHONE_CANCEL
        || divoom_app_tool_get_count_down_paused())
    {
        phandle->cd_status = 2;
        phandle->remain_sec = remain;
        if (remain > 0)
        {
            phandle->edit_min = remain / 60;
            phandle->edit_sec = remain % 60;
        }
    }
}

static void divoom_astrootoo_tools_countdown_refresh(Pdivoom_astrootoo_tools_ctrl phandle)
{
    char buf[8];
    int disp_min;
    int disp_sec;

    /* 真源 CountdownScreen：始终用同一套「分/秒」两块布局；运行/暂停/结束时
       原位显示剩余时间（remain_sec），编辑态显示设定值（edit_min/edit_sec），
       不切换到独立的合并大字页面。 */
    if (phandle->cd_status == 0)
    {
        disp_min = phandle->edit_min;
        disp_sec = phandle->edit_sec;
    }
    else
    {
        disp_min = phandle->remain_sec / 60;
        disp_sec = phandle->remain_sec % 60;
    }

    /* 合并大字始终隐藏，分/秒两块始终显示 */
    if (phandle->time_lbl)
    {
        lv_obj_add_flag(phandle->time_lbl, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_clear_flag(phandle->btn_min, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(phandle->btn_sec, LV_OBJ_FLAG_HIDDEN);
    lv_snprintf(buf, sizeof(buf), "%02d", disp_min);
    lv_label_set_text(phandle->btn_min_lbl, buf);
    lv_snprintf(buf, sizeof(buf), "%02d", disp_sec);
    lv_label_set_text(phandle->btn_sec_lbl, buf);

    if (phandle->status_lbl)
    {
        /* 真源 CountdownScreen：运行/暂停不显示状态行，仅结束显示 countdown.done */
        if (phandle->cd_status == 3)
        {
            lv_label_set_text(phandle->status_lbl,
                divoom_menu_control_get_disp_str("CountdownDone"));
            lv_obj_set_style_text_color(phandle->status_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
            lv_obj_clear_flag(phandle->status_lbl, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_set_style_text_color(phandle->status_lbl, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
            lv_obj_add_flag(phandle->status_lbl, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (phandle->btn_right_icon)
    {
        /* 真源 FlowtooToolActionBarV1：运行中显示暂停图标，其余（编辑/暂停/结束）显示播放图标 */
        divoom_menu_control_lv_img_set_src(phandle->btn_right_icon,
            phandle->cd_status == 1 ? DIVOOM_ASTROTOO_TOOLS_ICON_PAUSE : DIVOOM_ASTROTOO_TOOLS_ICON_PLAY);
        lv_obj_center(phandle->btn_right_icon);
    }
}

static void divoom_astrootoo_tools_countdown_focus(Pdivoom_astrootoo_tools_ctrl phandle, int idx)
{
    int running;

    running = (phandle->cd_status == 1 || phandle->cd_status == 2 || phandle->cd_status == 3);
    if (running)
    {
        /* 运行态只在 复位/开始 之间切换 */
        if (idx < 2)
        {
            idx = 2;
        }
    }
    if (idx < 0)
    {
        idx = 0;
    }
    if (idx > 3)
    {
        idx = 3;
    }
    phandle->focus_idx = idx;
    if (idx == 0)
    {
        lv_group_focus_obj(phandle->btn_min);
    }
    else if (idx == 1)
    {
        lv_group_focus_obj(phandle->btn_sec);
    }
    else if (idx == 2)
    {
        lv_group_focus_obj(phandle->btn_left);
    }
    else
    {
        lv_group_focus_obj(phandle->btn_right);
    }
}

static void divoom_astrootoo_tools_countdown_timer_cb(lv_timer_t *t)
{
    Pdivoom_astrootoo_tools_ctrl phandle;
    uint8 st;
    int remain;
    int old_status;
    int old_remain;

    phandle = (Pdivoom_astrootoo_tools_ctrl)lv_timer_get_user_data(t);
    if (!phandle || phandle->page != DIVOOM_ASTROTOO_TOOLS_PAGE_COUNTDOWN)
    {
        return;
    }

    old_status = phandle->cd_status;
    old_remain = phandle->remain_sec;
    st = divoom_app_tool_get_count_down_status();
    remain = (int)divoom_app_tool_get_count_down_cnt();
    if (remain < 0)
    {
        remain = 0;
    }

    if (st == DIVOOM_DISP_COUNT_TIME_DOWN_RUN)
    {
        phandle->cd_status = 1;
        phandle->remain_sec = remain;
    }
    else if (st == DIVOOM_DISP_COUNT_TIME_DOWN_TIME_OVER)
    {
        phandle->cd_status = 3;
        phandle->remain_sec = 0;
    }
    else if (st == DIVOOM_DISP_COUNT_TIME_DOWN_PHONE_CANCEL
        || divoom_app_tool_get_count_down_paused())
    {
        phandle->cd_status = 2;
        phandle->remain_sec = remain;
    }
    else if (phandle->cd_status == 1)
    {
        phandle->cd_status = 2;
        phandle->remain_sec = remain;
    }

    if (phandle->cd_status == old_status && phandle->remain_sec == old_remain)
    {
        return;
    }
    divoom_astrootoo_tools_countdown_refresh(phandle);
    if ((old_status == 0 && phandle->cd_status == 1)
        || (old_status == 1 && phandle->cd_status == 2))
    {
        divoom_astrootoo_tools_countdown_focus(phandle, 3);
    }
}

static void divoom_astrootoo_tools_countdown_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_tools_ctrl phandle;
    lv_obj_t *target;
    uint8_t key_code;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }

    phandle = (Pdivoom_astrootoo_tools_ctrl)lv_event_get_user_data(e);
    target = lv_event_get_target(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        if (phandle->cd_status != 0)
        {
            phandle->cd_status = 0;
            phandle->remain_sec = 0;
            divoom_astrootoo_tools_countdown_push_to_tool(phandle, 2);
            divoom_astrootoo_tools_countdown_refresh(phandle);
            divoom_astrootoo_tools_countdown_focus(phandle, 3);
        }
        else
        {
            divoom_menu_control_return_pre();
        }
        return;
    }

    /* 编辑态：分/秒 上下调整 */
    if (phandle->cd_status == 0 && (target == phandle->btn_min || target == phandle->btn_sec)
        && (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code)))
    {
        int delta;

        delta = divoom_astrootoo_ui_key_is_up(key_code) ? 1 : -1;
        if (target == phandle->btn_min)
        {
            phandle->edit_min = (phandle->edit_min + delta + 60) % 60;
        }
        else
        {
            phandle->edit_sec = (phandle->edit_sec + delta + 60) % 60;
        }
        divoom_astrootoo_tools_countdown_refresh(phandle);
        return;
    }

    if (divoom_astrootoo_ui_key_is_left(key_code) || divoom_astrootoo_ui_key_is_prev(key_code))
    {
        divoom_astrootoo_tools_countdown_focus(phandle, phandle->focus_idx - 1);
        return;
    }
    if (divoom_astrootoo_ui_key_is_right(key_code) || divoom_astrootoo_ui_key_is_next(key_code))
    {
        divoom_astrootoo_tools_countdown_focus(phandle, phandle->focus_idx + 1);
        return;
    }

    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        if (target == phandle->btn_min || target == phandle->btn_sec)
        {
            divoom_astrootoo_tools_countdown_focus(phandle, 3);
            return;
        }
        if (target == phandle->btn_left)
        {
            /* 复位：回到编辑态 */
            phandle->cd_status = 0;
            phandle->remain_sec = 0;
            divoom_astrootoo_tools_countdown_push_to_tool(phandle, 2);
            divoom_astrootoo_tools_countdown_refresh(phandle);
            divoom_astrootoo_tools_countdown_focus(phandle, 3);
            return;
        }
        /* 开始 / 暂停 / 继续 */
        if (phandle->cd_status == 0)
        {
            if (phandle->edit_min == 0 && phandle->edit_sec == 0)
            {
                return;
            }
            phandle->remain_sec = phandle->edit_min * 60 + phandle->edit_sec;
            phandle->cd_status = 1;
            divoom_astrootoo_tools_countdown_push_to_tool(phandle, 1);
        }
        else if (phandle->cd_status == 1)
        {
            phandle->cd_status = 2;
            divoom_astrootoo_tools_countdown_push_to_tool(phandle, 0);
        }
        else if (phandle->cd_status == 2)
        {
            phandle->cd_status = 1;
            divoom_astrootoo_tools_countdown_push_to_tool(phandle, 1);
        }
        else if (phandle->cd_status == 3)
        {
            phandle->cd_status = 0;
            phandle->remain_sec = 0;
            divoom_astrootoo_tools_countdown_push_to_tool(phandle, 2);
        }
        divoom_astrootoo_tools_countdown_refresh(phandle);
        divoom_astrootoo_tools_countdown_focus(phandle, 3);
    }
}

static void divoom_astrootoo_tools_build_countdown(Pdivoom_astrootoo_tools_ctrl phandle)
{
    lv_coord_t colon_cy;

    phandle->edit_min = 5;
    phandle->edit_sec = 0;
    phandle->cd_status = 0;
    phandle->remain_sec = 0;
    divoom_astrootoo_tools_countdown_load_from_tool(phandle);

    divoom_astrootoo_tools_create_header(phandle, "Countdown");

    phandle->status_lbl = lv_label_create(phandle->root_obj);
    lv_obj_set_style_text_color(phandle->status_lbl, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(phandle->status_lbl, divoom_menu_control_get_font_info(20, phandle->menu_id), 0);
    lv_obj_set_width(phandle->status_lbl, DIVOOM_SCREEN_WIDTH);
    lv_obj_set_style_text_align(phandle->status_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(phandle->status_lbl, 0, DIVOOM_ASTRO_CD_STATUS_Y);
    lv_obj_add_flag(phandle->status_lbl, LV_OBJ_FLAG_HIDDEN);

    divoom_astrootoo_tools_countdown_add_chevron(phandle->root_obj,
        DIVOOM_ASTRO_CD_MIN_X, DIVOOM_ASTRO_CD_ROW_Y_CENTER, DIVOOM_ASTROTOO_TOOLS_CD_STEP_UP);
    divoom_astrootoo_tools_countdown_add_chevron(phandle->root_obj,
        DIVOOM_ASTRO_CD_MIN_X,
        DIVOOM_ASTRO_CD_ROW_Y_CENTER + DIVOOM_ASTRO_CD_CHEVRON_H + DIVOOM_ASTRO_CD_CHEVRON_GAP
        + DIVOOM_ASTRO_CD_SEG_H + DIVOOM_ASTRO_CD_CHEVRON_GAP,
        DIVOOM_ASTROTOO_TOOLS_CD_STEP_DOWN);
    divoom_astrootoo_tools_countdown_add_chevron(phandle->root_obj,
        DIVOOM_ASTRO_CD_SEC_X, DIVOOM_ASTRO_CD_ROW_Y_CENTER, DIVOOM_ASTROTOO_TOOLS_CD_STEP_UP);
    divoom_astrootoo_tools_countdown_add_chevron(phandle->root_obj,
        DIVOOM_ASTRO_CD_SEC_X,
        DIVOOM_ASTRO_CD_ROW_Y_CENTER + DIVOOM_ASTRO_CD_CHEVRON_H + DIVOOM_ASTRO_CD_CHEVRON_GAP
        + DIVOOM_ASTRO_CD_SEG_H + DIVOOM_ASTRO_CD_CHEVRON_GAP,
        DIVOOM_ASTROTOO_TOOLS_CD_STEP_DOWN);

    phandle->btn_min = divoom_astrootoo_tools_create_countdown_segment(phandle,
        DIVOOM_ASTRO_CD_MIN_X, DIVOOM_ASTRO_CD_SEG_Y, "Min", &phandle->btn_min_lbl);
    phandle->btn_sec = divoom_astrootoo_tools_create_countdown_segment(phandle,
        DIVOOM_ASTRO_CD_SEC_X, DIVOOM_ASTRO_CD_SEG_Y, "Sec", &phandle->btn_sec_lbl);

    colon_cy = DIVOOM_ASTRO_CD_SEG_Y + DIVOOM_ASTRO_CD_SEG_PAD_TOP + DIVOOM_ASTRO_CD_DIGIT_CELL_H / 2;
    divoom_astrootoo_tools_countdown_add_colon(phandle->root_obj,
        DIVOOM_ASTRO_CD_COL_X, colon_cy);

    phandle->time_lbl = divoom_astrootoo_tools_create_big_time(phandle, phandle->root_obj,
        72, 170, lv_color_hex(DIVOOM_ASTRO_INK));
    lv_obj_add_flag(phandle->time_lbl, LV_OBJ_FLAG_HIDDEN);

    phandle->btn_left_lbl = NULL;
    phandle->btn_right_lbl = NULL;
    phandle->btn_left = divoom_astrootoo_tools_create_icon_action_btn(phandle, phandle->root_obj,
        DIVOOM_ASTROTOO_TOOLS_ICON_RESET, DIVOOM_ASTRO_LIST_PAD_X, DIVOOM_ASTRO_TOOL_BAR_Y,
        DIVOOM_ASTRO_TOOL_ACTION_RESET_W, DIVOOM_ASTRO_TOOL_BTN_H, &phandle->btn_left_icon);
    phandle->btn_right = divoom_astrootoo_tools_create_icon_action_btn(phandle, phandle->root_obj,
        DIVOOM_ASTROTOO_TOOLS_ICON_PLAY,
        DIVOOM_ASTRO_LIST_PAD_X + DIVOOM_ASTRO_TOOL_ACTION_RESET_W + DIVOOM_ASTRO_TOOL_BAR_GAP,
        DIVOOM_ASTRO_TOOL_BAR_Y, DIVOOM_ASTRO_TOOL_ACTION_PLAY_W, DIVOOM_ASTRO_TOOL_BTN_H,
        &phandle->btn_right_icon);

    lv_obj_add_event_cb(phandle->btn_min, divoom_astrootoo_tools_countdown_key_cb, LV_EVENT_KEY, phandle);
    lv_obj_add_event_cb(phandle->btn_sec, divoom_astrootoo_tools_countdown_key_cb, LV_EVENT_KEY, phandle);
    lv_obj_add_event_cb(phandle->btn_left, divoom_astrootoo_tools_countdown_key_cb, LV_EVENT_KEY, phandle);
    lv_obj_add_event_cb(phandle->btn_right, divoom_astrootoo_tools_countdown_key_cb, LV_EVENT_KEY, phandle);
    lv_group_add_obj(lv_group_get_default(), phandle->btn_min);
    lv_group_add_obj(lv_group_get_default(), phandle->btn_sec);
    lv_group_add_obj(lv_group_get_default(), phandle->btn_left);
    lv_group_add_obj(lv_group_get_default(), phandle->btn_right);

    divoom_astrootoo_tools_countdown_refresh(phandle);
    divoom_astrootoo_tools_countdown_focus(phandle, phandle->cd_status == 0 ? 0 : 3);
    phandle->timer = lv_timer_create(divoom_astrootoo_tools_countdown_timer_cb,
        DIVOOM_ASTRO_COUNTDOWN_POLL_MS, phandle);
}

/* ====================================================================== */
/* 番茄钟（Pomodoro）                                                      */
/* ====================================================================== */
/* ===== 番茄设置默认值 / 范围（真源 PomodoroScreen RANGE_*） ===== */
#define DIVOOM_ASTRO_POMO_WORK_DEF    25
#define DIVOOM_ASTRO_POMO_WORK_LO     1
#define DIVOOM_ASTRO_POMO_WORK_HI     55
#define DIVOOM_ASTRO_POMO_SHORT_DEF   5
#define DIVOOM_ASTRO_POMO_SHORT_LO    1
#define DIVOOM_ASTRO_POMO_SHORT_HI    30
#define DIVOOM_ASTRO_POMO_LONG_DEF    15
#define DIVOOM_ASTRO_POMO_LONG_LO     1
#define DIVOOM_ASTRO_POMO_LONG_HI     45
#define DIVOOM_ASTRO_POMO_CYCLES_DEF  4
#define DIVOOM_ASTRO_POMO_CYCLES_LO   1
#define DIVOOM_ASTRO_POMO_CYCLES_HI   4
#define DIVOOM_ASTRO_POMO_RING_CNT    12
#define DIVOOM_ASTRO_POMO_SET_ROWS    5
#define DIVOOM_ASTRO_POMO_SET_RING_ROW 4
#define DIVOOM_ASTRO_POMO_RING_BODY_W \
    (DIVOOM_ASTRO_CARD_ROW_W - DIVOOM_ASTRO_ALARM_TOGGLE_W - DIVOOM_ASTRO_ALARM_COL_GAP)
#define DIVOOM_ASTRO_POMO_RING_COL_BODY   0
#define DIVOOM_ASTRO_POMO_RING_COL_TOGGLE 1
#define DIVOOM_ASTROTOO_POMO_SWITCH_ON    "23/at_alarm_switch_on.png"
#define DIVOOM_ASTROTOO_POMO_SWITCH_OFF   "23/at_alarm_switch_off.png"
#define DIVOOM_ASTRO_POMO_PROG_MAX    12
#define DIVOOM_ASTRO_POMO_PREP_SEC    3
#define DIVOOM_ASTRO_POMO_NOTICE_X       112
#define DIVOOM_ASTRO_POMO_NOTICE_Y       70
#define DIVOOM_ASTRO_POMO_NOTICE_SIZE    256
#define DIVOOM_ASTRO_POMO_NOTICE_MIN_MS  4000
/* Run 底栏（真源 PomodoroScreen.runFooter / gearBtn+mainBtn；样式仍用 card_item_* 同计时器） */
#define DIVOOM_ASTRO_POMO_FOOTER_X      32
#define DIVOOM_ASTRO_POMO_FOOTER_Y      382
#define DIVOOM_ASTRO_POMO_FOOTER_H      64
#define DIVOOM_ASTRO_POMO_FOOTER_GAP    16
#define DIVOOM_ASTRO_POMO_FOOTER_W      (DIVOOM_SCREEN_WIDTH - DIVOOM_ASTRO_POMO_FOOTER_X * 2)
#define DIVOOM_ASTRO_POMO_GEAR_BTN_W    72
#define DIVOOM_ASTRO_POMO_MAIN_BTN_W \
    (DIVOOM_ASTRO_POMO_FOOTER_W - DIVOOM_ASTRO_POMO_FOOTER_GAP - DIVOOM_ASTRO_POMO_GEAR_BTN_W)
/* 进度行几何/动效（真源 PomodoroScreen progressRow + pomoSlotBreathe/pomoSlotHint） */
#define DIVOOM_ASTRO_POMO_PROG_Y             314
#define DIVOOM_ASTRO_POMO_PROG_BREATHE_HALF_MS  750
#define DIVOOM_ASTRO_POMO_PROG_HINT_HALF_MS     1000
#define DIVOOM_ASTRO_POMO_PROG_HINT_DELAY_MS     750
#define DIVOOM_ASTRO_POMO_PROG_OPA_BREATHE_DIM  178  /* ~70% */
#define DIVOOM_ASTRO_POMO_PROG_OPA_HINT_DIM     191  /* ~75% */
/* 大字号：真源像素字 80px 满 em；本工程平滑字体 cap-height≈0.7em，需放大约 1.4× 才能视觉等高 */
#define DIVOOM_ASTRO_POMO_BIG_FONT    112

/* 铃声名（真源 alarm.ringtoneNames，英文 key；中文经 strings.xml 本地化） */
static const char *const g_pomo_ring_keys[DIVOOM_ASTRO_POMO_RING_CNT] =
{
    "STORM", "WAVE", "CITY", "FOREST", "DOLPHIN", "SUNSET",
    "BIRD", "BELL", "DIGICLOCK", "PIANO", "ALARMCLOCK", "MELODY",
};

/* 铃声图标（与闹钟经典铃声同源，直接引用 ui/23 下切图，避免重复拷贝） */
#define DIVOOM_ASTRO_POMO_BELL_DIR    "23/"
static const char *const g_pomo_bell_icons[DIVOOM_ASTRO_POMO_RING_CNT] =
{
    "at_bell_rain.png", "at_bell_wave.png", "at_bell_city.png", "at_bell_forest.png",
    "at_bell_dolphin.png", "at_bell_sun.png", "at_bell_bird.png", "at_bell_ring.png",
    "at_bell_clock.png", "at_bell_piano.png", "at_bell_clock2.png", "at_bell_music.png",
};

/* 重复预设展示顺序（真源 POMODORO_REPEAT_DISPLAY_ORDER：长序列在上，单番茄在下） */
#define DIVOOM_ASTRO_POMO_REPEAT_CNT  5
static const int g_pomo_repeat_order[DIVOOM_ASTRO_POMO_REPEAT_CNT] = { 4, 3, 2, 1, 0 };

/* 重复预设列表几何（真源 flowtooPomodoroRepeatPickFrameStyle + PomodoroRepeatPick） */
#define DIVOOM_ASTRO_POMO_REPEAT_ROW_W     466
#define DIVOOM_ASTRO_POMO_REPEAT_ROW_H     DIVOOM_ASTRO_CARD_ROW_H  /* 72 */
#define DIVOOM_ASTRO_POMO_REPEAT_ROW_GAP   6
#define DIVOOM_ASTRO_POMO_REPEAT_LIST_TOP  (DIVOOM_ASTRO_HEADER_H + 4)
#define DIVOOM_ASTRO_POMO_REPEAT_ICON_GAP  4
#define DIVOOM_ASTRO_POMO_REPEAT_PAD_L     4
#define DIVOOM_ASTRO_POMO_REPEAT_PAD_R     8

/* 铃声栅格（3 列 × 4 行，复用闹钟经典铃声布局参数） */
#define DIVOOM_ASTRO_POMO_RING_COLS   3
#define DIVOOM_ASTRO_POMO_RING_ROWS   4
#define DIVOOM_ASTRO_POMO_RING_PAD    16
#define DIVOOM_ASTRO_POMO_RING_GAP    8

/* 重复预设 id → 工作段数（真源 pomodoroRepeatIdToWorkCycles：0→1，其余=id） */
static int divoom_astrootoo_tools_pomo_repeat_cycles(int id)
{
    if (id <= 0)
    {
        return 1;
    }
    if (id > 4)
    {
        return 4;
    }
    return id;
}

static int divoom_astrootoo_tools_pomo_phase_secs(Pdivoom_astrootoo_tools_ctrl phandle, int phase)
{
    if (phase == 1)
    {
        return phandle->pomo_short_min * 60;   /* short break */
    }
    if (phase == 2)
    {
        return phandle->pomo_long_min * 60;    /* long break */
    }
    return phandle->pomo_work_min * 60;        /* work */
}

/* run 顶栏阶段文案（真源 chromeWork/chromeShort/chromeLong） */
static const char *divoom_astrootoo_tools_pomo_phase_key(int phase)
{
    if (phase == 1)
    {
        return "Short";
    }
    if (phase == 2)
    {
        return "Long";
    }
    return "Work";
}

/* 进度格图标：work=番茄 short=咖啡 long=哑铃；active 彩色 / idle 线框 */
static const char *divoom_astrootoo_tools_pomo_icon_path(int kind, int active)
{
    if (kind == 1)
    {
        return active ? DIVOOM_ASTROTOO_TOOLS_UI_DIR "pomo_short.png"
                      : DIVOOM_ASTROTOO_TOOLS_UI_DIR "pomo_short_idle.png";
    }
    if (kind == 2)
    {
        return active ? DIVOOM_ASTROTOO_TOOLS_UI_DIR "pomo_long.png"
                      : DIVOOM_ASTROTOO_TOOLS_UI_DIR "pomo_long_idle.png";
    }
    return active ? DIVOOM_ASTROTOO_TOOLS_UI_DIR "pomo_tomato.png"
                  : DIVOOM_ASTROTOO_TOOLS_UI_DIR "pomo_work_idle.png";
}

/* 由重复预设 id 生成序列（真源 sequenceForPomodoroRepeatPresetId / buildPomodoroRepeatSequence）：
 * id 0 → 仅番茄；id≥1 → T (C T)* … D（n 段专注 = n 番茄 + (n-1) 咖啡 + 哑铃） */
static void divoom_astrootoo_tools_pomo_build_seq_id(Pdivoom_astrootoo_tools_ctrl phandle,
    int id, int *kind_out, int *n_out)
{
    int idx = 0;
    int n;
    int i;

    if (id < 0)
    {
        id = 0;
    }
    if (id > 4)
    {
        id = 4;
    }
    if (id == 0)
    {
        kind_out[idx++] = 0; /* tomato */
        *n_out = idx;
        (void)phandle;
        return;
    }

    n = id;
    if (n < 1)
    {
        n = 1;
    }
    if (n > 4)
    {
        n = 4;
    }
    if (n <= 1)
    {
        kind_out[idx++] = 0; /* tomato */
        kind_out[idx++] = 2; /* dumbbell */
    }
    else
    {
        for (i = 0; i < n; i++)
        {
            kind_out[idx++] = 0; /* tomato */
            if (i < n - 1)
            {
                kind_out[idx++] = 1; /* coffee(short) */
            }
        }
        kind_out[idx++] = 2; /* dumbbell */
    }
    *n_out = idx;
    (void)phandle;
}

/* run 视图当前生效序列（由当前重复预设构建并标记需重排） */
static void divoom_astrootoo_tools_pomo_build_seq(Pdivoom_astrootoo_tools_ctrl phandle)
{
    divoom_astrootoo_tools_pomo_build_seq_id(phandle, phandle->pomo_repeat_id,
        phandle->pomo_prog_kind, &phandle->pomo_prog_n);
    phandle->pomo_layout_dirty = 1;
}

/* 当前应点亮到的最后一格下标（含）；idle 为 -1（真源 pomodoroProgressActiveThrough） */
static int divoom_astrootoo_tools_pomo_active_through(Pdivoom_astrootoo_tools_ctrl phandle)
{
    int cycles;
    int cycle_work;
    int through;

    if (phandle->pomo_mode == 0 || phandle->pomo_mode == 3)
    {
        return -1;
    }
    if (phandle->pomo_phase == 2)
    {
        return phandle->pomo_prog_n - 1;
    }

    cycles = phandle->pomo_cycles;
    if (cycles < 1)
    {
        cycles = 1;
    }
    /* 长休后进入新一轮：round 累计不清零，须对 cycles 取模（真源 completedWorkCount % cycles） */
    cycle_work = phandle->pomo_round % cycles;
    if (phandle->pomo_phase == 0)
    {
        through = cycle_work * 2;
    }
    else
    {
        through = cycle_work * 2 - 1;
    }
    if (through < -1)
    {
        through = -1;
    }
    if (through >= phandle->pomo_prog_n)
    {
        through = phandle->pomo_prog_n - 1;
    }
    return through;
}

static void divoom_astrootoo_tools_pomo_prog_frontier_anim_cb(void *var, int32_t v);
static void divoom_astrootoo_tools_pomo_prog_next_anim_cb(void *var, int32_t v);

static lv_coord_t s_pomo_prog_frontier_base_y;

static void divoom_astrootoo_tools_pomo_prog_slot_motion_reset(Pdivoom_astrootoo_tools_ctrl phandle,
    int slot)
{
    lv_obj_t *img;

    if (!phandle || slot < 0 || slot >= DIVOOM_ASTRO_POMO_PROG_MAX)
    {
        return;
    }
    img = phandle->pomo_prog[slot];
    if (!img)
    {
        return;
    }
    lv_anim_del(img, divoom_astrootoo_tools_pomo_prog_frontier_anim_cb);
    lv_anim_del(img, divoom_astrootoo_tools_pomo_prog_next_anim_cb);
    lv_obj_set_y(img, phandle->pomo_prog_base_y[slot]);
    lv_obj_set_style_opa(img, LV_OPA_COVER, 0);
}

static void divoom_astrootoo_tools_pomo_progress_motion_stop(Pdivoom_astrootoo_tools_ctrl phandle)
{
    int i;

    if (!phandle)
    {
        return;
    }
    for (i = 0; i < DIVOOM_ASTRO_POMO_PROG_MAX; i++)
    {
        divoom_astrootoo_tools_pomo_prog_slot_motion_reset(phandle, i);
    }
    phandle->pomo_motion_frontier = -1;
    phandle->pomo_motion_next = -1;
    phandle->pomo_motion_on = 0;
}

static void divoom_astrootoo_tools_pomo_prog_frontier_anim_cb(void *var, int32_t v)
{
    lv_obj_t *img = var;

    lv_obj_set_y(img, (lv_coord_t)(s_pomo_prog_frontier_base_y - (v ? 1 : 0)));
    lv_obj_set_style_opa(img, v ? LV_OPA_COVER : (lv_opa_t)DIVOOM_ASTRO_POMO_PROG_OPA_BREATHE_DIM, 0);
}

static void divoom_astrootoo_tools_pomo_prog_next_anim_cb(void *var, int32_t v)
{
    lv_obj_t *img = var;

    lv_obj_set_style_opa(img, v ? (lv_opa_t)DIVOOM_ASTRO_POMO_PROG_OPA_HINT_DIM : LV_OPA_COVER, 0);
}

static void divoom_astrootoo_tools_pomo_prog_start_frontier_anim(Pdivoom_astrootoo_tools_ctrl phandle,
    int slot)
{
    lv_obj_t *img;
    lv_anim_t a;

    if (!phandle || slot < 0 || slot >= DIVOOM_ASTRO_POMO_PROG_MAX)
    {
        return;
    }
    img = phandle->pomo_prog[slot];
    if (!img || lv_anim_get(img, divoom_astrootoo_tools_pomo_prog_frontier_anim_cb) != NULL)
    {
        return;
    }
    s_pomo_prog_frontier_base_y = phandle->pomo_prog_base_y[slot];
    lv_obj_set_y(img, s_pomo_prog_frontier_base_y);
    lv_obj_set_style_opa(img, (lv_opa_t)DIVOOM_ASTRO_POMO_PROG_OPA_BREATHE_DIM, 0);
    lv_anim_init(&a);
    lv_anim_set_var(&a, img);
    lv_anim_set_exec_cb(&a, divoom_astrootoo_tools_pomo_prog_frontier_anim_cb);
    lv_anim_set_values(&a, 0, 1);
    lv_anim_set_time(&a, DIVOOM_ASTRO_POMO_PROG_BREATHE_HALF_MS);
    lv_anim_set_playback_time(&a, DIVOOM_ASTRO_POMO_PROG_BREATHE_HALF_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_step);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);
}

static void divoom_astrootoo_tools_pomo_prog_start_next_anim(Pdivoom_astrootoo_tools_ctrl phandle,
    int slot)
{
    lv_obj_t *img;
    lv_anim_t a;

    if (!phandle || slot < 0 || slot >= DIVOOM_ASTRO_POMO_PROG_MAX)
    {
        return;
    }
    img = phandle->pomo_prog[slot];
    if (!img || lv_anim_get(img, divoom_astrootoo_tools_pomo_prog_next_anim_cb) != NULL)
    {
        return;
    }
    lv_obj_set_style_opa(img, LV_OPA_COVER, 0);
    lv_anim_init(&a);
    lv_anim_set_var(&a, img);
    lv_anim_set_exec_cb(&a, divoom_astrootoo_tools_pomo_prog_next_anim_cb);
    lv_anim_set_values(&a, 0, 1);
    lv_anim_set_time(&a, DIVOOM_ASTRO_POMO_PROG_HINT_HALF_MS);
    lv_anim_set_playback_time(&a, DIVOOM_ASTRO_POMO_PROG_HINT_HALF_MS);
    lv_anim_set_delay(&a, DIVOOM_ASTRO_POMO_PROG_HINT_DELAY_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_step);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);
}

/* running 时 frontier 呼吸 + 下一格 hint；暂停/idle/arming/弹窗/离页静止 */
static void divoom_astrootoo_tools_pomo_progress_motion_sync(Pdivoom_astrootoo_tools_ctrl phandle)
{
    int frontier;
    int next;
    int run_motion;

    if (!phandle || phandle->pomo_view != 0 || phandle->pomo_exit_overlay)
    {
        divoom_astrootoo_tools_pomo_progress_motion_stop(phandle);
        return;
    }

    run_motion = (phandle->pomo_mode == 1) ? 1 : 0;
    frontier = run_motion ? divoom_astrootoo_tools_pomo_active_through(phandle) : -1;
    next = (frontier >= 0 && frontier + 1 < phandle->pomo_prog_n) ? frontier + 1 : -1;
    if (run_motion == phandle->pomo_motion_on
        && frontier == phandle->pomo_motion_frontier
        && next == phandle->pomo_motion_next)
    {
        return;
    }

    divoom_astrootoo_tools_pomo_progress_motion_stop(phandle);
    phandle->pomo_motion_on = run_motion;
    phandle->pomo_motion_frontier = frontier;
    phandle->pomo_motion_next = next;
    if (!run_motion || frontier < 0)
    {
        return;
    }

    divoom_astrootoo_tools_pomo_prog_start_frontier_anim(phandle, frontier);
    if (next >= 0)
    {
        divoom_astrootoo_tools_pomo_prog_start_next_anim(phandle, next);
    }
}

/* 进度行排布（序列长度变化时重新居中并显隐多余格） */
static void divoom_astrootoo_tools_pomo_progress_layout(Pdivoom_astrootoo_tools_ctrl phandle)
{
    lv_coord_t x0;
    int n;
    int i;

    n = phandle->pomo_prog_n;
    x0 = (DIVOOM_SCREEN_WIDTH - (n * 48 + (n - 1) * 6)) / 2;
    for (i = 0; i < DIVOOM_ASTRO_POMO_PROG_MAX; i++)
    {
        if (!phandle->pomo_prog[i])
        {
            continue;
        }
        if (i < n)
        {
            lv_obj_set_pos(phandle->pomo_prog[i], x0 + i * (48 + 6), DIVOOM_ASTRO_POMO_PROG_Y);
            phandle->pomo_prog_base_y[i] = DIVOOM_ASTRO_POMO_PROG_Y;
            lv_obj_clear_flag(phandle->pomo_prog[i], LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(phandle->pomo_prog[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/* 进度行点亮刷新（仅在序列或点亮位变化时换图，避免 key 回调频繁文件 I/O） */
static void divoom_astrootoo_tools_pomo_progress_refresh(Pdivoom_astrootoo_tools_ctrl phandle)
{
    int through;
    int i;

    through = divoom_astrootoo_tools_pomo_active_through(phandle);
    if (!phandle->pomo_layout_dirty && through == phandle->pomo_last_through)
    {
        return;
    }
    if (phandle->pomo_layout_dirty)
    {
        divoom_astrootoo_tools_pomo_progress_motion_stop(phandle);
        divoom_astrootoo_tools_pomo_progress_layout(phandle);
        phandle->pomo_layout_dirty = 0;
    }
    for (i = 0; i < phandle->pomo_prog_n; i++)
    {
        if (!phandle->pomo_prog[i])
        {
            continue;
        }
        divoom_menu_control_lv_img_set_src(phandle->pomo_prog[i],
            divoom_astrootoo_tools_pomo_icon_path(phandle->pomo_prog_kind[i], i <= through));
    }
    phandle->pomo_last_through = through;
}

static void divoom_astrootoo_tools_pomo_set_refresh(Pdivoom_astrootoo_tools_ctrl phandle);
static void divoom_astrootoo_tools_pomo_bind_group(Pdivoom_astrootoo_tools_ctrl phandle,
    lv_obj_t *focus_override);
static void divoom_astrootoo_tools_pomo_show_run(Pdivoom_astrootoo_tools_ctrl phandle);
static void divoom_astrootoo_tools_pomo_show_settings(Pdivoom_astrootoo_tools_ctrl phandle);
static void divoom_astrootoo_tools_pomo_show_edit(Pdivoom_astrootoo_tools_ctrl phandle, int field);
static void divoom_astrootoo_tools_pomo_show_repeat(Pdivoom_astrootoo_tools_ctrl phandle);
static void divoom_astrootoo_tools_pomo_show_ringtone(Pdivoom_astrootoo_tools_ctrl phandle);
static void divoom_astrootoo_tools_pomo_refresh(Pdivoom_astrootoo_tools_ctrl phandle);
static void divoom_astrootoo_tools_pomo_sync_from_backend(Pdivoom_astrootoo_tools_ctrl phandle, int full);
static void divoom_astrootoo_tools_pomo_start_focus_session(Pdivoom_astrootoo_tools_ctrl phandle);
static void divoom_astrootoo_tools_pomo_begin_arming(Pdivoom_astrootoo_tools_ctrl phandle);
static void divoom_astrootoo_tools_pomo_notice_anim_close(Pdivoom_astrootoo_tools_ctrl phandle);
static void divoom_astrootoo_tools_pomo_notice_anim_play(Pdivoom_astrootoo_tools_ctrl phandle);

static Pdivoom_tomato_time_info divoom_astrootoo_tools_pomo_get_backend_info(void)
{
    int idx;
    Pdivoom_tomato_time_info info;

    idx = divoom_tomato_time_get_cur_tomato_index();
    if (idx != 0xff)
    {
        info = divoom_tomato_time_get_one_tomato(idx);
        if (info)
        {
            return info;
        }
    }
    return divoom_tomato_time_get_default_tomato();
}

static void divoom_astrootoo_tools_pomo_sync_tomato_index(Pdivoom_astrootoo_tools_ctrl phandle)
{
    int idx;

    if (!phandle)
    {
        return;
    }

    idx = divoom_tomato_time_get_cur_tomato_index();
    phandle->pomo_tomato_index = (idx == 0xff) ? 0 : idx;
}

static int divoom_astrootoo_tools_pomo_task_count(void)
{
    return divoom_tomato_time_get_tomato_num();
}

static int divoom_astrootoo_tools_pomo_can_switch_task(Pdivoom_astrootoo_tools_ctrl phandle)
{
    if (!phandle || phandle->pomo_view != 0)
    {
        return 0;
    }
    if (phandle->pomo_anim_win || phandle->pomo_exit_overlay)
    {
        return 0;
    }
    if (phandle->pomo_mode != 0)
    {
        return 0;
    }
    if (divoom_tomato_time_get_sub_status() == TOMATO_START)
    {
        return 0;
    }
    if (divoom_astrootoo_tools_pomo_task_count() <= 1)
    {
        return 0;
    }
    return 1;
}

static void divoom_astrootoo_tools_pomo_switch_task(Pdivoom_astrootoo_tools_ctrl phandle, int dir)
{
    int num;
    Pdivoom_tomato_time_info tomato_data;

    if (!divoom_astrootoo_tools_pomo_can_switch_task(phandle))
    {
        return;
    }

    num = divoom_astrootoo_tools_pomo_task_count();
    if (num <= 1)
    {
        return;
    }

    if (dir < 0)
    {
        if (phandle->pomo_tomato_index > 0)
        {
            phandle->pomo_tomato_index--;
        }
        else
        {
            phandle->pomo_tomato_index = num - 1;
        }
    }
    else
    {
        phandle->pomo_tomato_index++;
        if (phandle->pomo_tomato_index >= num)
        {
            phandle->pomo_tomato_index = 0;
        }
    }

    tomato_data = divoom_tomato_time_get_one_tomato(phandle->pomo_tomato_index);
    if (!tomato_data)
    {
        return;
    }

    divoom_tomato_time_change_tomato_info(tomato_data->id);
    divoom_astrootoo_tools_pomo_sync_from_backend(phandle, 1);
}

/* 幂等：只在值真的变了才重建序列，避免每秒 sync 触发进度格重排 */
static int divoom_astrootoo_tools_pomo_apply_backend_info(Pdivoom_astrootoo_tools_ctrl phandle,
    Pdivoom_tomato_time_info info)
{
    int changed = 0;

    if (!phandle || !info)
    {
        return 0;
    }

    if (info->work_time > 0 && phandle->pomo_work_min != info->work_time)
    {
        phandle->pomo_work_min = info->work_time;
        changed = 1;
    }
    if (info->short_rest_time > 0 && phandle->pomo_short_min != info->short_rest_time)
    {
        phandle->pomo_short_min = info->short_rest_time;
        changed = 1;
    }
    if (info->long_rest_time > 0 && phandle->pomo_long_min != info->long_rest_time)
    {
        phandle->pomo_long_min = info->long_rest_time;
        changed = 1;
    }
    if (phandle->pomo_repeat_id != info->roll_times)
    {
        phandle->pomo_repeat_id = info->roll_times;
        phandle->pomo_cycles = divoom_astrootoo_tools_pomo_repeat_cycles(phandle->pomo_repeat_id);
        changed = 1;
    }
    if (info->sound_type == 0 && info->sound_system_index < DIVOOM_ASTRO_POMO_RING_CNT)
    {
        phandle->pomo_ring = info->sound_system_index;
    }
    phandle->pomo_ring_on = info->sound_on_off ? 1 : 0;
    if (changed)
    {
        divoom_astrootoo_tools_pomo_build_seq(phandle);
    }
    return changed;
}

/* 仅在 LVGL 线程调用：读后端状态刷新 UI，不触发 lv_scr_load。
 * full=0 为每秒轻量 tick，只读状态不碰 tomato_queue；
 * full=1 用于进页 / set_tomato 之后 / App 触发同步，额外回灌配置。 */
static void divoom_astrootoo_tools_pomo_sync_from_backend(Pdivoom_astrootoo_tools_ctrl phandle, int full)
{
    Pdivoom_tomato_time_info info;
    uint8 sub;
    uint8 main_st;
    uint32 remain;
    int offset;

    if (!phandle || phandle->page != DIVOOM_ASTROTOO_TOOLS_PAGE_POMODORO)
    {
        return;
    }

    /* 用户在设置/编辑/重复/铃声子页时 phandle 是权威，别用后端值把输入冲掉 */
    if (full && phandle->pomo_view == 0)
    {
        info = divoom_astrootoo_tools_pomo_get_backend_info();
        divoom_astrootoo_tools_pomo_apply_backend_info(phandle, info);
        divoom_astrootoo_tools_pomo_sync_tomato_index(phandle);
    }

    sub = divoom_tomato_time_get_sub_status();
    main_st = divoom_tomato_time_get_main_status();
    phandle->pomo_today_cnt = divoom_tomato_time_get_cur_do_tomato_num();

    if (sub == TOMATO_STOP)
    {
        phandle->pomo_mode = 0;
        phandle->pomo_prep_sec = 0;
        phandle->pomo_prep_next_ms = 0;
        phandle->pomo_phase = 0;
        phandle->pomo_round = 0;
        phandle->remain_sec = 0;
        divoom_astrootoo_tools_pomo_refresh(phandle);
        return;
    }

    if (sub == TOMATO_PUASE)
    {
        phandle->pomo_mode = 2;
    }
    else
    {
        phandle->pomo_mode = 1;
    }
    phandle->pomo_prep_sec = 0;
    phandle->pomo_prep_next_ms = 0;

    /* 进度序列看本轮轮次；今日累计只用于右上角计数 */
    offset = divoom_tomato_time_get_tomato_offset();
    phandle->pomo_round = offset;

    if (main_st == TOMATO_MAIN_RESET)
    {
        /* 与后端同款判定：roll_times==0 时后端不进 RESET，此处不会被执行到 */
        if (phandle->pomo_repeat_id > 0 && offset >= phandle->pomo_repeat_id)
        {
            phandle->pomo_phase = 2;
        }
        else
        {
            phandle->pomo_phase = 1;
        }
    }
    else
    {
        phandle->pomo_phase = 0;
    }

    remain = divoom_tomato_time_get_time();
    if (remain > 0)
    {
        phandle->remain_sec = (int)remain;
    }
    else
    {
        phandle->remain_sec = divoom_astrootoo_tools_pomo_phase_secs(phandle, phandle->pomo_phase);
    }

    divoom_astrootoo_tools_pomo_refresh(phandle);
}

static void divoom_astrootoo_tools_pomo_refresh(Pdivoom_astrootoo_tools_ctrl phandle)
{
    char buf[8];
    Pdivoom_tomato_time_info info;
    const char *task_name;

    if (phandle->pomo_exit_overlay)
    {
        return;
    }

    if (phandle->pomo_mode == 0 || phandle->pomo_mode == 3)
    {
        /* idle / arming 显示 工作时长:00（真源 displayTime） */
        lv_snprintf(buf, sizeof(buf), "%02d:00", phandle->pomo_work_min);
    }
    else
    {
        divoom_astrootoo_tools_fmt_ms(buf, sizeof(buf), phandle->remain_sec);
    }
    lv_label_set_text(phandle->time_lbl, buf);

    if (phandle->status_lbl)
    {
        const char *cur_name;

        info = divoom_astrootoo_tools_pomo_get_backend_info();
        if (info && info->name[0] != '\0')
        {
            task_name = (const char *)info->name;
        }
        else
        {
            task_name = divoom_menu_control_get_disp_str("Work");
        }
        /* 倒计时会高频 refresh：同名不重设，避免打断跑马灯 */
        cur_name = lv_label_get_text(phandle->status_lbl);
        if (!cur_name || strcmp(cur_name, task_name) != 0)
        {
            lv_label_set_text(phandle->status_lbl, task_name);
            lv_label_set_long_mode(phandle->status_lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
            lv_obj_set_style_anim_speed(phandle->status_lbl, 30, 0);
        }
    }
    if (phandle->pomo_count_lbl)
    {
        char cbuf[8];

        lv_snprintf(cbuf, sizeof(cbuf), "x%d", phandle->pomo_today_cnt);
        lv_label_set_text(phandle->pomo_count_lbl, cbuf);
    }
    /* 主钮：arming 数字倒计时；running 暂停图标；idle/paused 播放图标 */
    if (phandle->pomo_mode == 3 && phandle->btn_right_lbl)
    {
        lv_snprintf(buf, sizeof(buf), "%d", phandle->pomo_prep_sec);
        lv_label_set_text(phandle->btn_right_lbl, buf);
        lv_obj_clear_flag(phandle->btn_right_lbl, LV_OBJ_FLAG_HIDDEN);
        if (phandle->btn_right_icon)
        {
            lv_obj_add_flag(phandle->btn_right_icon, LV_OBJ_FLAG_HIDDEN);
        }
    }
    else if (phandle->btn_right_icon)
    {
        if (phandle->btn_right_lbl)
        {
            lv_obj_add_flag(phandle->btn_right_lbl, LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_clear_flag(phandle->btn_right_icon, LV_OBJ_FLAG_HIDDEN);
        divoom_menu_control_lv_img_set_src(phandle->btn_right_icon,
            phandle->pomo_mode == 1 ? DIVOOM_ASTROTOO_TOOLS_ICON_PAUSE : DIVOOM_ASTROTOO_TOOLS_ICON_PLAY);
        lv_obj_center(phandle->btn_right_icon);
    }
    divoom_astrootoo_tools_pomo_progress_refresh(phandle);
    divoom_astrootoo_tools_pomo_progress_motion_sync(phandle);
}

static void divoom_astrootoo_tools_pomo_timer_cb(lv_timer_t *t)
{
    Pdivoom_astrootoo_tools_ctrl phandle;

    phandle = (Pdivoom_astrootoo_tools_ctrl)lv_timer_get_user_data(t);
    if (!phandle)
    {
        return;
    }

    if (phandle->pomo_mode == 3)
    {
        if (lv_tick_get() < phandle->pomo_prep_next_ms)
        {
            return;
        }
        phandle->pomo_prep_next_ms = lv_tick_get() + 1000;
        if (phandle->pomo_prep_sec > 0)
        {
            phandle->pomo_prep_sec--;
        }
        if (phandle->pomo_prep_sec <= 0)
        {
            divoom_astrootoo_tools_pomo_start_focus_session(phandle);
        }
        else if (phandle->pomo_view == 0 && !phandle->pomo_exit_overlay)
        {
            divoom_astrootoo_tools_pomo_refresh(phandle);
        }
        return;
    }

    /* 计时、阶段切换、到点铃声与动画全在 divoom_tomato_time_update，这里只跟随显示 */
    if (phandle->pomo_view == 0 && !phandle->pomo_exit_overlay)
    {
        divoom_astrootoo_tools_pomo_sync_from_backend(phandle, 0);
    }
}

/* 本地预备结束：下发 flag 5 交给后端起跑（真源 startFocusSession） */
static void divoom_astrootoo_tools_pomo_start_focus_session(Pdivoom_astrootoo_tools_ctrl phandle)
{
    phandle->pomo_prep_sec = 0;
    phandle->pomo_prep_next_ms = 0;
    divoom_tomato_time_set_tomato(NULL, 5);
    divoom_astrootoo_tools_pomo_sync_from_backend(phandle, 1);
}

/* idle 按主钮：主钮 3-2-1 预备（真源 beginArming；APP 启动仍走全屏 GIF，不经此路径） */
static void divoom_astrootoo_tools_pomo_begin_arming(Pdivoom_astrootoo_tools_ctrl phandle)
{
    phandle->pomo_prep_sec = DIVOOM_ASTRO_POMO_PREP_SEC;
    phandle->pomo_mode = 3;
    phandle->pomo_prep_next_ms = lv_tick_get() + 1000;
    if (phandle->timer)
    {
        lv_timer_reset(phandle->timer);
    }
    divoom_astrootoo_tools_pomo_refresh(phandle);
}

/* 主按钮：idle->arming；arming 忽略；running->暂停(flag 8)；paused->继续(flag 9) */
static void divoom_astrootoo_tools_pomo_toggle_main(Pdivoom_astrootoo_tools_ctrl phandle)
{
    if (phandle->pomo_mode == 3)
    {
        return;
    }
    if (phandle->pomo_mode == 0)
    {
        divoom_astrootoo_tools_pomo_begin_arming(phandle);
        return;
    }
    else if (phandle->pomo_mode == 1)
    {
        divoom_tomato_time_set_tomato(NULL, 8);
    }
    else
    {
        divoom_tomato_time_set_tomato(NULL, 9);
    }
    divoom_astrootoo_tools_pomo_sync_from_backend(phandle, 1);
}

static void divoom_astrootoo_tools_pomo_stop_backend_running(void)
{
    if (divoom_tomato_time_get_sub_status() != TOMATO_STOP)
        divoom_tomato_time_set_tomato(NULL, 0);
}

static void divoom_astrootoo_tools_pomo_reset_idle(Pdivoom_astrootoo_tools_ctrl phandle)
{
    divoom_astrootoo_tools_pomo_notice_anim_close(phandle);
    divoom_astrootoo_tools_pomo_stop_backend_running();
    phandle->pomo_mode = 0;
    phandle->pomo_prep_sec = 0;
    phandle->pomo_prep_next_ms = 0;
    phandle->pomo_phase = 0;
    phandle->pomo_round = 0;
    phandle->remain_sec = 0;
    divoom_astrootoo_tools_pomo_refresh(phandle);
}

static void divoom_astrootoo_tools_pomo_group_remove_obj_safe(lv_obj_t *obj)
{
    lv_group_t *g;

    g = lv_group_get_default();
    if (obj && g)
    {
        lv_group_remove_obj(obj);
    }
}

static void divoom_astrootoo_tools_pomo_notice_level_cb(void *user_data, void *lvgl_obj, int level)
{
    (void)user_data;
    (void)lvgl_obj;
    (void)level;
}

static void divoom_astrootoo_tools_pomo_notice_key_cb(lv_event_t *e)
{
    /* 播放中吞键，避免点到底下暂停 */
    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
}

/*
 * dial menu 的图像流水线要求 pdivoom_lvgl_dial_menu_handle 非空，而番茄页进来时表盘已退出。
 * 沿用歌词页做法（divoom_lvgl_lyric_effect.c）：挂一个只提供队列的外壳，用完拆掉。
 * 表盘本来就活着时不建也不拆，图直接登记进表盘队列。
 */
static void divoom_astrootoo_tools_pomo_notice_shell_open(Pdivoom_astrootoo_tools_ctrl phandle, lv_obj_t *win)
{
    if (!phandle || phandle->pomo_anim_shell_flag || divoom_lvgl_dial_menu_check_handle_exist())
    {
        return;
    }
    divoom_lvgl_dial_menu_init_lyric_shell(win, phandle->menu_id, 0);
    if (divoom_lvgl_dial_menu_check_handle_exist())
    {
        phandle->pomo_anim_shell_flag = 1;
    }
}

static void divoom_astrootoo_tools_pomo_notice_shell_close(Pdivoom_astrootoo_tools_ctrl phandle)
{
    if (!phandle || !phandle->pomo_anim_shell_flag)
    {
        return;
    }
    phandle->pomo_anim_shell_flag = 0;
    divoom_lvgl_dial_menu_exit_lyric_shell();
}

static void divoom_astrootoo_tools_pomo_notice_anim_close(Pdivoom_astrootoo_tools_ctrl phandle)
{
    lv_obj_t *restore;
    lv_group_t *g;

    if (!phandle || !phandle->pomo_anim_win)
    {
        return;
    }

    if (phandle->pomo_anim_close_timer)
    {
        lv_timer_del(phandle->pomo_anim_close_timer);
        phandle->pomo_anim_close_timer = NULL;
    }

    g = lv_group_get_default();
    if (g)
    {
        lv_group_focus_freeze(g, false);
    }

    divoom_astrootoo_tools_pomo_group_remove_obj_safe(phandle->pomo_anim_win);

    restore = phandle->pomo_anim_focus_restore;
    phandle->pomo_anim_focus_restore = NULL;
    if (restore != phandle->btn_left && restore != phandle->btn_right)
    {
        restore = phandle->btn_right;
    }

    if (phandle->pomo_anim_disp_handle)
    {
        divoom_lvgl_dial_menu_del_image_disp_handle(phandle->pomo_anim_disp_handle, 0);
        phandle->pomo_anim_disp_handle = NULL;
    }
    divoom_astrootoo_tools_pomo_notice_shell_close(phandle);

    lv_obj_del(phandle->pomo_anim_win);
    phandle->pomo_anim_win = NULL;

    if (restore == phandle->btn_left)
    {
        divoom_astrootoo_tools_focus_lr(phandle, 0);
    }
    else if (phandle->btn_right)
    {
        divoom_astrootoo_tools_focus_lr(phandle, 1);
    }
}

static void divoom_astrootoo_tools_pomo_notice_close_timer_cb(lv_timer_t *t)
{
    Pdivoom_astrootoo_tools_ctrl phandle;

    phandle = (Pdivoom_astrootoo_tools_ctrl)lv_timer_get_user_data(t);
    if (phandle)
    {
        phandle->pomo_anim_close_timer = NULL;
        divoom_astrootoo_tools_pomo_notice_anim_close(phandle);
    }
}

static void divoom_astrootoo_tools_pomo_notice_anim_play(Pdivoom_astrootoo_tools_ctrl phandle)
{
    Pdivoom_tomato_time_info info;
    Pdivoom_lvgl_dial_menu_image_ctrl pimg;
    divoom_lvgl_dial_menu_image_item image_item = {0};
    lv_obj_t *win;
    lv_obj_t *focused;
    lv_group_t *g;
    char file_name[256];
    uint32 image_id;
    int close_ms;
    int loop_ms;

    if (!phandle || phandle->pomo_anim_win)
    {
        return;
    }
    if (phandle->pomo_exit_overlay || phandle->pomo_view != 0)
    {
        return;
    }
    if (phandle->page != DIVOOM_ASTROTOO_TOOLS_PAGE_POMODORO)
    {
        return;
    }

    info = divoom_astrootoo_tools_pomo_get_backend_info();
    if (!info || info->id == 0 || info->image_file_id[0] == '\0')
    {
        return;
    }

    image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_TOMATO_GIF + info->id;
    if (!divoom_manage_image_check_exist(DIVOOM_MANAGE_IMAGE_TOMATO_TIME_FILE, image_id))
    {
        return;
    }

    file_name[0] = '\0';
    divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_TOMATO_TIME_FILE, image_id, file_name);
    if (file_name[0] == '\0')
    {
        return;
    }

    win = lv_obj_create(phandle->root_obj);
    if (!win)
    {
        return;
    }
    lv_obj_remove_style_all(win);
    lv_obj_set_size(win, DIVOOM_ASTRO_POMO_NOTICE_SIZE, DIVOOM_ASTRO_POMO_NOTICE_SIZE);
    lv_obj_set_pos(win, DIVOOM_ASTRO_POMO_NOTICE_X, DIVOOM_ASTRO_POMO_NOTICE_Y);
    lv_obj_add_flag(win, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(win, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(win, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(win, 0, 0);
    lv_obj_set_style_border_width(win, 0, 0);
    lv_obj_move_foreground(win);

    image_item.x = 0;
    image_item.y = 0;
    image_item.w = DIVOOM_ASTRO_POMO_NOTICE_SIZE;
    image_item.h = DIVOOM_ASTRO_POMO_NOTICE_SIZE;
    image_item.image_name = file_name;

    divoom_astrootoo_tools_pomo_notice_shell_open(phandle, win);
    phandle->pomo_anim_disp_handle = divoom_lvgl_dial_menu_add_divoom_file_handle(
        NULL, &image_item, win, 0, phandle, divoom_astrootoo_tools_pomo_notice_level_cb, 0);
    if (!phandle->pomo_anim_disp_handle)
    {
        divoom_astrootoo_tools_pomo_notice_shell_close(phandle);
        lv_obj_del(win);
        return;
    }

    phandle->pomo_anim_win = win;
    lv_obj_add_event_cb(win, divoom_astrootoo_tools_pomo_notice_key_cb, LV_EVENT_KEY, phandle);

    focused = lv_group_get_focused(lv_group_get_default());
    if (focused == phandle->btn_left || focused == phandle->btn_right)
    {
        phandle->pomo_anim_focus_restore = focused;
    }
    else
    {
        phandle->pomo_anim_focus_restore = phandle->btn_right;
    }

    g = lv_group_get_default();
    if (g)
    {
        lv_group_add_obj(g, win);
        lv_group_focus_obj(win);
        lv_group_focus_freeze(g, true);
    }

    pimg = (Pdivoom_lvgl_dial_menu_image_ctrl)phandle->pomo_anim_disp_handle;
    loop_ms = 0;
    if (pimg->frame_num > 0 && pimg->frame_time > 0)
    {
        loop_ms = pimg->frame_num * pimg->frame_time;
    }
    close_ms = (loop_ms > DIVOOM_ASTRO_POMO_NOTICE_MIN_MS) ? loop_ms : DIVOOM_ASTRO_POMO_NOTICE_MIN_MS;

    phandle->pomo_anim_close_timer = lv_timer_create(
        divoom_astrootoo_tools_pomo_notice_close_timer_cb, close_ms, phandle);
    if (phandle->pomo_anim_close_timer)
    {
        lv_timer_set_repeat_count(phandle->pomo_anim_close_timer, 1);
    }

    divoom_print("[pomo_notice] play %s close_ms=%d\n", file_name, close_ms);
}

static void divoom_astrootoo_tools_pomo_exit_confirm_raise(Pdivoom_astrootoo_tools_ctrl phandle)
{
    if (phandle && phandle->pomo_exit_overlay)
    {
        lv_obj_move_foreground(phandle->pomo_exit_overlay);
    }
}

static void divoom_astrootoo_tools_pomo_exit_confirm_close(Pdivoom_astrootoo_tools_ctrl phandle)
{
    lv_obj_t *restore;

    if (!phandle || !phandle->pomo_exit_overlay)
    {
        return;
    }
    restore = phandle->pomo_exit_focus_restore;
    if (restore != phandle->btn_left && restore != phandle->btn_right)
    {
        restore = phandle->btn_right;
    }
    phandle->pomo_exit_focus_restore = NULL;

    divoom_astrootoo_tools_pomo_group_remove_obj_safe(phandle->pomo_exit_cancel_btn);
    divoom_astrootoo_tools_pomo_group_remove_obj_safe(phandle->pomo_exit_ok_btn);
    lv_obj_del(phandle->pomo_exit_overlay);
    phandle->pomo_exit_overlay = NULL;
    phandle->pomo_exit_cancel_btn = NULL;
    phandle->pomo_exit_ok_btn = NULL;

    if (restore == phandle->btn_left)
    {
        divoom_astrootoo_tools_focus_lr(phandle, 0);
    }
    else
    {
        divoom_astrootoo_tools_focus_lr(phandle, 1);
    }
    divoom_astrootoo_tools_pomo_refresh(phandle);
}

static void divoom_astrootoo_tools_pomo_exit_confirm_refresh(Pdivoom_astrootoo_tools_ctrl phandle)
{
    int leave_focus;
    lv_obj_t *leave_lbl;

    if (!phandle->pomo_exit_cancel_btn || !phandle->pomo_exit_ok_btn)
    {
        return;
    }
    leave_focus = (phandle->pomo_exit_focus == 1);

    lv_obj_set_style_bg_opa(phandle->pomo_exit_cancel_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(phandle->pomo_exit_cancel_btn,
        lv_color_hex(leave_focus ? DIVOOM_ASTRO_SURFACE_IDLE : DIVOOM_ASTRO_SURFACE_FOCUS), 0);

    if (leave_focus)
    {
        lv_obj_set_style_bg_opa(phandle->pomo_exit_ok_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(phandle->pomo_exit_ok_btn, lv_color_hex(DIVOOM_ASTRO_DANGER), 0);
        lv_obj_set_style_border_width(phandle->pomo_exit_ok_btn, 0, 0);
    }
    else
    {
        lv_obj_set_style_bg_opa(phandle->pomo_exit_ok_btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(phandle->pomo_exit_ok_btn, 2, 0);
        lv_obj_set_style_border_color(phandle->pomo_exit_ok_btn, lv_color_hex(DIVOOM_ASTRO_DANGER), 0);
    }
    leave_lbl = lv_obj_get_child(phandle->pomo_exit_ok_btn, 0);
    if (leave_lbl)
    {
        lv_obj_set_style_text_color(leave_lbl,
            lv_color_hex(leave_focus ? DIVOOM_ASTRO_BG : DIVOOM_ASTRO_DANGER), 0);
    }

    lv_group_focus_obj(leave_focus ? phandle->pomo_exit_ok_btn : phandle->pomo_exit_cancel_btn);
    divoom_astrootoo_ui_confirm_modal_sync_btn_labels(phandle->pomo_exit_cancel_btn,
        phandle->pomo_exit_ok_btn, leave_focus);
    divoom_astrootoo_tools_pomo_exit_confirm_raise(phandle);
}

static void divoom_astrootoo_tools_pomo_exit_confirm_leave(Pdivoom_astrootoo_tools_ctrl phandle)
{
    divoom_astrootoo_tools_pomo_exit_confirm_close(phandle);
    divoom_astrootoo_tools_pomo_reset_idle(phandle);
    /* 先回栈再 cancel，避免同步 play 抢页后 PRE 变 stale；reset_idle 仍须在本页销毁前 */
    divoom_menu_control_return_pre();
    divoom_tomato_time_cancel();
}

static void divoom_astrootoo_tools_pomo_exit_confirm_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_tools_ctrl phandle;
    uint8_t key_code;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    phandle = (Pdivoom_astrootoo_tools_ctrl)lv_event_get_user_data(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_astrootoo_tools_pomo_exit_confirm_close(phandle);
        return;
    }
    if (divoom_astrootoo_ui_key_is_left(key_code) || divoom_astrootoo_ui_key_is_right(key_code)
        || divoom_astrootoo_ui_key_is_prev(key_code) || divoom_astrootoo_ui_key_is_next(key_code))
    {
        phandle->pomo_exit_focus = (phandle->pomo_exit_focus == 0) ? 1 : 0;
        divoom_astrootoo_tools_pomo_exit_confirm_refresh(phandle);
        return;
    }
    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        if (phandle->pomo_exit_focus == 1)
        {
            divoom_astrootoo_tools_pomo_exit_confirm_leave(phandle);
        }
        else
        {
            divoom_astrootoo_tools_pomo_exit_confirm_close(phandle);
        }
    }
}

static lv_obj_t *divoom_astrootoo_tools_pomo_exit_confirm_make_btn(Pdivoom_astrootoo_tools_ctrl phandle,
    lv_obj_t *parent, const char *label_key)
{
    return divoom_astrootoo_ui_confirm_modal_make_btn(parent,
        divoom_menu_control_get_disp_str(label_key), phandle->menu_id, 0,
        divoom_astrootoo_tools_pomo_exit_confirm_key_cb, phandle, 1);
}

static void divoom_astrootoo_tools_pomo_exit_confirm_open(Pdivoom_astrootoo_tools_ctrl phandle)
{
    lv_obj_t *frame;
    lv_obj_t *title;
    lv_obj_t *body;
    lv_obj_t *actions;
    lv_obj_t *stay_lbl;

    if (!phandle || phandle->pomo_exit_overlay || phandle->pomo_view != 0)
    {
        return;
    }
    if (phandle->pomo_anim_win)
    {
        return;
    }
    if (phandle->pomo_mode != 1 && phandle->pomo_mode != 2)
    {
        return;
    }

    phandle->pomo_exit_focus = 0;
    {
        lv_obj_t *focused;

        focused = lv_group_get_focused(lv_group_get_default());
        if (focused == phandle->btn_left || focused == phandle->btn_right)
        {
            phandle->pomo_exit_focus_restore = focused;
        }
        else
        {
            phandle->pomo_exit_focus_restore = phandle->btn_right;
        }
    }

    phandle->pomo_exit_overlay = divoom_astrootoo_ui_confirm_modal_create(
        phandle->root_obj, &frame);
    if (!phandle->pomo_exit_overlay || !frame)
    {
        return;
    }

    title = lv_label_create(frame);
    lv_label_set_text(title, divoom_menu_control_get_disp_str("PomodoroExitTitle"));
    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(30, phandle->menu_id), 0);
    lv_obj_set_style_pad_bottom(title, 10, 0);
    lv_obj_set_style_border_side(title, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(title, 2, 0);
    lv_obj_set_style_border_color(title, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);

    body = lv_label_create(frame);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_label_set_text(body, divoom_menu_control_get_disp_str("PomodoroExitBody"));
    lv_obj_set_width(body, LV_PCT(100));
    lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(body, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(body, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);

    actions = lv_obj_create(frame);
    lv_obj_remove_style_all(actions);
    lv_obj_set_width(actions, LV_PCT(100));
    lv_obj_set_height(actions, LV_SIZE_CONTENT);
    lv_obj_clear_flag(actions, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(actions, 8, 0);

    phandle->pomo_exit_cancel_btn = divoom_astrootoo_tools_pomo_exit_confirm_make_btn(phandle, actions,
        "PomodoroExitStay");
    phandle->pomo_exit_ok_btn = divoom_astrootoo_tools_pomo_exit_confirm_make_btn(phandle, actions,
        "PomodoroExitLeave");

    stay_lbl = lv_obj_get_child(phandle->pomo_exit_cancel_btn, 0);
    if (stay_lbl)
    {
        lv_obj_set_style_text_color(stay_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    }

    /* 透明遮罩：保留 run 层可见；弹窗置顶由 pomo_exit_confirm_raise 保证 */
    divoom_astrootoo_ui_confirm_modal_layout(frame);
    divoom_astrootoo_tools_pomo_exit_confirm_refresh(phandle);
    divoom_astrootoo_tools_pomo_exit_confirm_raise(phandle);
}

/* ===== run 视图按键 ===== */
static void divoom_astrootoo_tools_pomo_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_tools_ctrl phandle;
    lv_obj_t *target;
    uint8_t key_code;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }

    phandle = (Pdivoom_astrootoo_tools_ctrl)lv_event_get_user_data(e);
    target = lv_event_get_target(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    if (phandle->pomo_anim_win || phandle->pomo_exit_overlay)
    {
        return;
    }

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        if (phandle->pomo_mode == 1 || phandle->pomo_mode == 2)
        {
            divoom_astrootoo_tools_pomo_exit_confirm_open(phandle);
        }
        else if (phandle->pomo_mode == 3)
        {
            divoom_astrootoo_tools_pomo_reset_idle(phandle);
        }
        else
        {
            divoom_astrootoo_tools_pomo_stop_backend_running();
            divoom_menu_control_return_pre();
            divoom_tomato_time_cancel();
        }
        return;
    }
    if (divoom_astrootoo_ui_key_is_up(key_code))
    {
        if (divoom_astrootoo_tools_pomo_can_switch_task(phandle))
        {
            divoom_astrootoo_tools_pomo_switch_task(phandle, -1);
            return;
        }
    }
    else if (divoom_astrootoo_ui_key_is_down(key_code))
    {
        if (divoom_astrootoo_tools_pomo_can_switch_task(phandle))
        {
            divoom_astrootoo_tools_pomo_switch_task(phandle, 1);
            return;
        }
    }
    if (divoom_astrootoo_ui_key_is_left(key_code) || divoom_astrootoo_ui_key_is_prev(key_code))
    {
        divoom_astrootoo_tools_focus_lr(phandle, 0);
        return;
    }
    if (divoom_astrootoo_ui_key_is_right(key_code) || divoom_astrootoo_ui_key_is_next(key_code))
    {
        divoom_astrootoo_tools_focus_lr(phandle, 1);
        return;
    }
    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        if (target == phandle->btn_left)
        {
            if (phandle->pomo_mode == 3)
            {
                return;
            }
            divoom_astrootoo_tools_pomo_show_settings(phandle);
        }
        else
        {
            divoom_astrootoo_tools_pomo_toggle_main(phandle);
        }
    }
}

/* ===== 编辑(单值)视图按键：左右 ± 调档，确定/返回 回到设置 ===== */
static void divoom_astrootoo_tools_pomo_edit_apply_label(Pdivoom_astrootoo_tools_ctrl phandle)
{
    char buf[12];
    int v;

    if (phandle->pomo_edit_field == 1)
    {
        v = phandle->pomo_short_min;
    }
    else if (phandle->pomo_edit_field == 2)
    {
        v = phandle->pomo_long_min;
    }
    else
    {
        v = phandle->pomo_work_min;
    }

    lv_snprintf(buf, sizeof(buf), "%02d", v);
    lv_label_set_text(phandle->pomo_edit_val_lbl, buf);
    lv_obj_align(phandle->pomo_edit_val_lbl, LV_ALIGN_CENTER, 0, -16);
}

static void divoom_astrootoo_tools_pomo_edit_step(Pdivoom_astrootoo_tools_ctrl phandle, int dir)
{
    if (phandle->pomo_edit_field == 0)
    {
        phandle->pomo_work_min += dir;
        if (phandle->pomo_work_min < DIVOOM_ASTRO_POMO_WORK_LO) phandle->pomo_work_min = DIVOOM_ASTRO_POMO_WORK_LO;
        if (phandle->pomo_work_min > DIVOOM_ASTRO_POMO_WORK_HI) phandle->pomo_work_min = DIVOOM_ASTRO_POMO_WORK_HI;
    }
    else if (phandle->pomo_edit_field == 1)
    {
        phandle->pomo_short_min += dir;
        if (phandle->pomo_short_min < DIVOOM_ASTRO_POMO_SHORT_LO) phandle->pomo_short_min = DIVOOM_ASTRO_POMO_SHORT_LO;
        if (phandle->pomo_short_min > DIVOOM_ASTRO_POMO_SHORT_HI) phandle->pomo_short_min = DIVOOM_ASTRO_POMO_SHORT_HI;
    }
    else
    {
        phandle->pomo_long_min += dir;
        if (phandle->pomo_long_min < DIVOOM_ASTRO_POMO_LONG_LO) phandle->pomo_long_min = DIVOOM_ASTRO_POMO_LONG_LO;
        if (phandle->pomo_long_min > DIVOOM_ASTRO_POMO_LONG_HI) phandle->pomo_long_min = DIVOOM_ASTRO_POMO_LONG_HI;
    }
    divoom_astrootoo_tools_pomo_edit_apply_label(phandle);
}

/* 设备端改完时长/重复次数写回后端配置，联网时由 menu_update 上报 App */
static void divoom_astrootoo_tools_pomo_save_config(Pdivoom_astrootoo_tools_ctrl phandle)
{
    divoom_tomato_time_set_local_config((uint8)phandle->pomo_work_min,
        (uint8)phandle->pomo_short_min, (uint8)phandle->pomo_long_min,
        (uint8)phandle->pomo_repeat_id);
}

static void divoom_astrootoo_tools_pomo_edit_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_tools_ctrl phandle;
    uint8_t key_code;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    phandle = (Pdivoom_astrootoo_tools_ctrl)lv_event_get_user_data(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    if (divoom_astrootoo_ui_key_is_esc(key_code) || divoom_astrootoo_ui_key_is_enter(key_code))
    {
        divoom_astrootoo_tools_pomo_save_config(phandle);
        divoom_astrootoo_tools_pomo_show_settings(phandle);
        return;
    }
    if (divoom_astrootoo_ui_key_is_left(key_code) || divoom_astrootoo_ui_key_is_down(key_code))
    {
        divoom_astrootoo_tools_pomo_edit_step(phandle, -1);
        return;
    }
    if (divoom_astrootoo_ui_key_is_right(key_code) || divoom_astrootoo_ui_key_is_up(key_code))
    {
        divoom_astrootoo_tools_pomo_edit_step(phandle, 1);
        return;
    }
}

/* ===== 设置(齿轮)列表按键 ===== */
static void divoom_astrootoo_tools_pomo_set_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_tools_ctrl phandle;
    lv_obj_t *target;
    lv_obj_t *next;
    uint8_t key_code;
    int pos;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    phandle = (Pdivoom_astrootoo_tools_ctrl)lv_event_get_user_data(e);
    target = lv_event_get_target(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_astrootoo_tools_pomo_show_run(phandle);
        return;
    }
    if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code))
    {
        int direct;

        direct = divoom_astrootoo_ui_key_is_up(key_code) ? DIVOOM_MENU_DIRECT_UP : DIVOOM_MENU_DIRECT_DOWN;
        next = divoom_menu_direct_ctrl_get(phandle->pomo_set_direct, target, direct);
        if (next && next != target)
        {
            phandle->pomo_set_focus = divoom_menu_direct_ctrl_get_pos(phandle->pomo_set_direct, next);
            phandle->pomo_set_ring_col = DIVOOM_ASTRO_POMO_RING_COL_BODY;
            lv_group_focus_obj(next);
            divoom_astrootoo_ui_focus_scroll(next);
            divoom_astrootoo_tools_pomo_set_refresh(phandle);
        }
        return;
    }
    if (divoom_astrootoo_ui_key_is_left(key_code))
    {
        if (phandle->pomo_set_focus == DIVOOM_ASTRO_POMO_SET_RING_ROW)
        {
            phandle->pomo_set_ring_col = DIVOOM_ASTRO_POMO_RING_COL_BODY;
            divoom_astrootoo_tools_pomo_set_refresh(phandle);
        }
        return;
    }
    if (divoom_astrootoo_ui_key_is_right(key_code))
    {
        if (phandle->pomo_set_focus == DIVOOM_ASTRO_POMO_SET_RING_ROW)
        {
            phandle->pomo_set_ring_col = DIVOOM_ASTRO_POMO_RING_COL_TOGGLE;
            divoom_astrootoo_tools_pomo_set_refresh(phandle);
        }
        return;
    }
    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        pos = divoom_menu_direct_ctrl_get_pos(phandle->pomo_set_direct, target);
        if (pos == 0 || pos == 1 || pos == 2)
        {
            divoom_astrootoo_tools_pomo_show_edit(phandle, pos);
        }
        else if (pos == 3)
        {
            divoom_astrootoo_tools_pomo_show_repeat(phandle);
        }
        else if (pos == DIVOOM_ASTRO_POMO_SET_RING_ROW)
        {
            if (phandle->pomo_set_ring_col == DIVOOM_ASTRO_POMO_RING_COL_TOGGLE)
            {
                phandle->pomo_ring_on = phandle->pomo_ring_on ? 0 : 1;
                divoom_tomato_time_set_sound_on_off((uint8)phandle->pomo_ring_on);
                divoom_astrootoo_tools_pomo_set_refresh(phandle);
            }
            else if (phandle->pomo_ring_on)
            {
                divoom_astrootoo_tools_pomo_show_ringtone(phandle);
            }
        }
    }
}

/* 番茄设置行：左标题限宽 + 右值 marquee（与设置 link 行一致） */
static void divoom_astrootoo_tools_pomo_set_row_tune_marquee(lv_obj_t *btn, lv_obj_t *val)
{
    lv_obj_t *lbl;

    if (!btn || !val)
    {
        return;
    }
    lbl = lv_obj_get_child(btn, 0);
    if (!lbl)
    {
        return;
    }
    divoom_astrootoo_ui_list_kv_bind_row(lbl, val,
        DIVOOM_ASTRO_CARD_ROW_W, DIVOOM_ASTRO_TOOLS_CARD_LABEL_INSET_L,
        DIVOOM_ASTRO_TOOLS_CARD_LABEL_INSET_R, DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W,
        -(DIVOOM_ASTRO_TOOLS_CARD_LABEL_INSET_R + DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W
            + DIVOOM_ASTRO_LIST_LABEL_GAP),
        72);
    divoom_astrootoo_ui_list_row_bind_marquee(btn, val);
}

/* 设置行右侧值刷新 */
static void divoom_astrootoo_tools_pomo_set_refresh(Pdivoom_astrootoo_tools_ctrl phandle)
{
    char buf[24];
    const char *min_str;
    int i;

    min_str = divoom_menu_control_get_disp_str("Min");
    if (phandle->pomo_set_val[0])
    {
        lv_snprintf(buf, sizeof(buf), "%d %s", phandle->pomo_work_min, min_str);
        lv_label_set_text(phandle->pomo_set_val[0], buf);
    }
    if (phandle->pomo_set_val[1])
    {
        lv_snprintf(buf, sizeof(buf), "%d %s", phandle->pomo_short_min, min_str);
        lv_label_set_text(phandle->pomo_set_val[1], buf);
    }
    if (phandle->pomo_set_val[2])
    {
        lv_snprintf(buf, sizeof(buf), "%d %s", phandle->pomo_long_min, min_str);
        lv_label_set_text(phandle->pomo_set_val[2], buf);
    }
    if (phandle->pomo_set_val[3])
    {
        lv_snprintf(buf, sizeof(buf), "x%d",
            divoom_astrootoo_tools_pomo_repeat_cycles(phandle->pomo_repeat_id));
        lv_label_set_text(phandle->pomo_set_val[3], buf);
    }
    if (phandle->pomo_set_val[DIVOOM_ASTRO_POMO_SET_RING_ROW])
    {
        if (phandle->pomo_ring_on)
        {
            lv_label_set_text(phandle->pomo_set_val[DIVOOM_ASTRO_POMO_SET_RING_ROW],
                divoom_menu_control_get_disp_str(g_pomo_ring_keys[phandle->pomo_ring]));
            lv_obj_set_style_text_color(phandle->pomo_set_val[DIVOOM_ASTRO_POMO_SET_RING_ROW],
                lv_color_hex(DIVOOM_ASTRO_INK), 0);
        }
        else
        {
            lv_label_set_text(phandle->pomo_set_val[DIVOOM_ASTRO_POMO_SET_RING_ROW],
                divoom_menu_control_get_disp_str("Off"));
            lv_obj_set_style_text_color(phandle->pomo_set_val[DIVOOM_ASTRO_POMO_SET_RING_ROW],
                lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
        }
    }
    if (phandle->pomo_set_ring_chevron)
    {
        if (phandle->pomo_ring_on)
        {
            lv_obj_clear_flag(phandle->pomo_set_ring_chevron, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(phandle->pomo_set_ring_chevron, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (phandle->pomo_set_ring_sw)
    {
        divoom_menu_control_lv_img_set_src(phandle->pomo_set_ring_sw,
            phandle->pomo_ring_on ? DIVOOM_ASTROTOO_POMO_SWITCH_ON : DIVOOM_ASTROTOO_POMO_SWITCH_OFF);
        lv_obj_center(phandle->pomo_set_ring_sw);
    }
    if (phandle->pomo_set_ring_body)
    {
        int body_focus;
        int toggle_focus;
        lv_opa_t body_opa;

        body_focus = (phandle->pomo_set_focus == DIVOOM_ASTRO_POMO_SET_RING_ROW)
            && (phandle->pomo_set_ring_col == DIVOOM_ASTRO_POMO_RING_COL_BODY);
        toggle_focus = (phandle->pomo_set_focus == DIVOOM_ASTRO_POMO_SET_RING_ROW)
            && (phandle->pomo_set_ring_col == DIVOOM_ASTRO_POMO_RING_COL_TOGGLE);
        body_opa = phandle->pomo_ring_on ? LV_OPA_COVER : (lv_opa_t)107;

        lv_obj_remove_style(phandle->pomo_set_ring_body, &phandle->ui.card_item_style, 0);
        lv_obj_remove_style(phandle->pomo_set_ring_body, &phandle->ui.card_item_focus_style, 0);
        lv_obj_add_style(phandle->pomo_set_ring_body,
            body_focus ? &phandle->ui.card_item_focus_style : &phandle->ui.card_item_style, 0);
        lv_obj_set_style_opa(phandle->pomo_set_ring_body, body_opa, 0);

        if (phandle->pomo_set_ring_toggle)
        {
            lv_obj_remove_style(phandle->pomo_set_ring_toggle, &phandle->ui.card_item_style, 0);
            lv_obj_remove_style(phandle->pomo_set_ring_toggle, &phandle->ui.card_item_focus_style, 0);
            lv_obj_add_style(phandle->pomo_set_ring_toggle,
                toggle_focus ? &phandle->ui.card_item_focus_style : &phandle->ui.card_item_style, 0);
        }
    }
    for (i = 0; i < DIVOOM_ASTRO_POMO_SET_ROWS; i++)
    {
        if (phandle->pomo_set_row[i] && phandle->pomo_set_val[i])
        {
            divoom_astrootoo_ui_list_label_refresh_marquee(phandle->pomo_set_row[i],
                phandle->pomo_set_val[i]);
        }
    }
}

/*
 * r43757 的 disp 只把底栏两钮入组；设置/编辑/重复/铃声行会被踢出 group。
 * LVGL 的 lv_group_focus_obj 对不在组内的对象直接 return，导致无法聚焦设置。
 * 按当前 pomo_view 重绑可聚焦对象。
 */
static void divoom_astrootoo_tools_pomo_bind_group(Pdivoom_astrootoo_tools_ctrl phandle,
    lv_obj_t *focus_override)
{
    lv_obj_t *objs[DIVOOM_ASTRO_POMO_RING_CNT];
    lv_obj_t *focus;
    int n;
    int i;

    if (!phandle || phandle->pomo_exit_overlay || phandle->pomo_anim_win)
    {
        return;
    }

    n = 0;
    focus = NULL;
    if (phandle->pomo_view == 1)
    {
        for (i = 0; i < DIVOOM_ASTRO_POMO_SET_ROWS; i++)
        {
            objs[n++] = phandle->pomo_set_row[i];
        }
        if (phandle->pomo_set_focus < 0 || phandle->pomo_set_focus >= DIVOOM_ASTRO_POMO_SET_ROWS)
        {
            phandle->pomo_set_focus = 0;
        }
        focus = phandle->pomo_set_row[phandle->pomo_set_focus];
    }
    else if (phandle->pomo_view == 2)
    {
        objs[n++] = phandle->pomo_edit_key_btn;
        focus = phandle->pomo_edit_key_btn;
    }
    else if (phandle->pomo_view == 3)
    {
        for (i = 0; i < DIVOOM_ASTRO_POMO_REPEAT_CNT; i++)
        {
            objs[n++] = phandle->pomo_repeat_row[i];
        }
        if (phandle->pomo_repeat_focus < 0 || phandle->pomo_repeat_focus >= DIVOOM_ASTRO_POMO_REPEAT_CNT)
        {
            phandle->pomo_repeat_focus = 0;
        }
        focus = phandle->pomo_repeat_row[phandle->pomo_repeat_focus];
    }
    else if (phandle->pomo_view == 4)
    {
        for (i = 0; i < DIVOOM_ASTRO_POMO_RING_CNT; i++)
        {
            objs[n++] = phandle->pomo_ring_btn[i];
        }
        if (phandle->pomo_ring_focus < 0 || phandle->pomo_ring_focus >= DIVOOM_ASTRO_POMO_RING_CNT)
        {
            phandle->pomo_ring_focus = 0;
        }
        focus = phandle->pomo_ring_btn[phandle->pomo_ring_focus];
    }
    else
    {
        objs[n++] = phandle->btn_left;
        objs[n++] = phandle->btn_right;
        if (focus_override == phandle->btn_left || focus_override == phandle->btn_right)
        {
            focus = focus_override;
        }
        else
        {
            focus = (phandle->focus_idx == 0) ? phandle->btn_left : phandle->btn_right;
        }
        phandle->focus_idx = (focus == phandle->btn_left) ? 0 : 1;
    }

    if (focus_override && phandle->pomo_view != 0)
    {
        for (i = 0; i < n; i++)
        {
            if (objs[i] == focus_override)
            {
                focus = focus_override;
                break;
            }
        }
    }

    divoom_astrootoo_tools_group_bind_objs(objs, n, focus);
}

/* ===== 视图切换 ===== */
static void divoom_astrootoo_tools_pomo_show_run(Pdivoom_astrootoo_tools_ctrl phandle)
{
    phandle->pomo_view = 0;
    if (phandle->pomo_set_cont)
    {
        lv_obj_add_flag(phandle->pomo_set_cont, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->pomo_edit_cont)
    {
        lv_obj_add_flag(phandle->pomo_edit_cont, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->pomo_repeat_cont)
    {
        lv_obj_add_flag(phandle->pomo_repeat_cont, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->pomo_ring_cont)
    {
        lv_obj_add_flag(phandle->pomo_ring_cont, LV_OBJ_FLAG_HIDDEN);
    }
    /* 在子页期间轻量 tick 不刷新，回到 run 视图先跟后端对一次 */
    divoom_astrootoo_tools_pomo_sync_from_backend(phandle, 1);
    phandle->focus_idx = 1;
    divoom_astrootoo_tools_pomo_bind_group(phandle, NULL);
}

static void divoom_astrootoo_tools_pomo_show_settings(Pdivoom_astrootoo_tools_ctrl phandle)
{
    divoom_astrootoo_tools_pomo_notice_anim_close(phandle);
    divoom_astrootoo_tools_pomo_progress_motion_stop(phandle);
    phandle->pomo_view = 1;
    if (phandle->pomo_edit_cont)
    {
        lv_obj_add_flag(phandle->pomo_edit_cont, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->pomo_repeat_cont)
    {
        lv_obj_add_flag(phandle->pomo_repeat_cont, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->pomo_ring_cont)
    {
        lv_obj_add_flag(phandle->pomo_ring_cont, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->pomo_set_cont)
    {
        lv_obj_clear_flag(phandle->pomo_set_cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(phandle->pomo_set_cont);
    }
    divoom_astrootoo_tools_pomo_set_refresh(phandle);
    if (phandle->pomo_set_focus < 0 || phandle->pomo_set_focus >= DIVOOM_ASTRO_POMO_SET_ROWS)
    {
        phandle->pomo_set_focus = 0;
        phandle->pomo_set_ring_col = DIVOOM_ASTRO_POMO_RING_COL_BODY;
    }
    divoom_astrootoo_tools_pomo_bind_group(phandle, NULL);
}

static void divoom_astrootoo_tools_pomo_show_edit(Pdivoom_astrootoo_tools_ctrl phandle, int field)
{
    const char *title_key;

    divoom_astrootoo_tools_pomo_progress_motion_stop(phandle);
    phandle->pomo_view = 2;
    phandle->pomo_edit_field = field;
    if (field == 1)
    {
        title_key = "Short break";
    }
    else if (field == 2)
    {
        title_key = "Long break";
    }
    else
    {
        title_key = "Work duration";
    }
    lv_label_set_text(phandle->pomo_edit_title_lbl, divoom_menu_control_get_disp_str(title_key));
    lv_obj_center(phandle->pomo_edit_title_lbl);
    divoom_astrootoo_tools_pomo_edit_apply_label(phandle);

    if (phandle->pomo_edit_cont)
    {
        lv_obj_clear_flag(phandle->pomo_edit_cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(phandle->pomo_edit_cont);
    }
    divoom_astrootoo_tools_pomo_bind_group(phandle, NULL);
}

/* ===== 设置(齿轮)子页构建（深色顶栏 + 4 行卡片 + 铃声复合行：body|toggle） ===== */
static void divoom_astrootoo_tools_pomo_build_settings(Pdivoom_astrootoo_tools_ctrl phandle)
{
    static const char *const set_keys[DIVOOM_ASTRO_POMO_SET_ROWS] =
    {
        "Work", "Short break", "Long break", "Repeat", "Ringtone",
    };
    lv_obj_t *content;
    lv_obj_t *btn;
    lv_obj_t *val;
    lv_obj_t *body;
    lv_obj_t *toggle;
    lv_obj_t *lbl;
    lv_obj_t *chev;
    lv_coord_t row_x;
    lv_coord_t row_y;
    int i;

    phandle->pomo_set_cont = lv_obj_create(phandle->root_obj);
    lv_obj_add_style(phandle->pomo_set_cont, &phandle->ui.root_style, 0);
    lv_obj_set_size(phandle->pomo_set_cont, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_pos(phandle->pomo_set_cont, 0, 0);
    lv_obj_clear_flag(phandle->pomo_set_cont, LV_OBJ_FLAG_SCROLLABLE);

    divoom_astrootoo_ui_create_page_header_tab(phandle->pomo_set_cont, &phandle->ui,
        "Pomodoro Settings", DIVOOM_ASTROTOO_TOOLS_HEADER, phandle->menu_id);
    content = divoom_astrootoo_ui_create_scroll_content(phandle->pomo_set_cont, &phandle->ui);

    phandle->pomo_set_direct = divoom_menu_direct_ctrl_create(1, DIVOOM_ASTRO_POMO_SET_ROWS);
    for (i = 0; i < DIVOOM_ASTRO_POMO_SET_RING_ROW; i++)
    {
        btn = divoom_astrootoo_ui_create_card_row(content, &phandle->ui,
            set_keys[i], i, DIVOOM_ASTRO_POMO_SET_ROWS, phandle->pomo_set_direct, phandle->menu_id,
            0);
        phandle->pomo_set_row[i] = btn;
        divoom_astrootoo_tools_add_chevron(btn);

        val = lv_label_create(btn);
        lv_obj_set_style_text_color(val, lv_color_hex(DIVOOM_ASTRO_INK), 0);
        lv_obj_set_style_text_font(val, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
        lv_label_set_text(val, "");
        lv_obj_align(val, LV_ALIGN_RIGHT_MID,
            -(8 + DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W + DIVOOM_ASTRO_LIST_LABEL_GAP), 0);
        phandle->pomo_set_val[i] = val;
        divoom_astrootoo_tools_pomo_set_row_tune_marquee(btn, val);

        lv_obj_add_event_cb(btn, divoom_astrootoo_tools_pomo_set_key_cb, LV_EVENT_KEY, phandle);
        lv_group_add_obj(lv_group_get_default(), btn);
    }

    /* 铃声行：左侧 body（标题+值+chevron）+ 右侧开关，对齐闹钟列表 / 480 ringtoneSettingsRow */
    row_x = (DIVOOM_SCREEN_WIDTH - DIVOOM_ASTRO_CARD_ROW_W) / 2;
    row_y = DIVOOM_ASTRO_CARD_ROW_GAP
        + DIVOOM_ASTRO_POMO_SET_RING_ROW * (DIVOOM_ASTRO_CARD_ROW_H + DIVOOM_ASTRO_CARD_ROW_GAP);
    btn = lv_btn_create(content);
    lv_obj_add_style(btn, &phandle->ui.content_style, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_size(btn, DIVOOM_ASTRO_CARD_ROW_W, DIVOOM_ASTRO_CARD_ROW_H);
    lv_obj_set_pos(btn, row_x, row_y);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    body = lv_obj_create(btn);
    lv_obj_add_style(body, &phandle->ui.card_item_style, 0);
    lv_obj_set_size(body, DIVOOM_ASTRO_POMO_RING_BODY_W, DIVOOM_ASTRO_CARD_ROW_H);
    lv_obj_set_pos(body, 0, 0);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(body, 0, 0);

    lbl = lv_label_create(body);
    lv_label_set_text(lbl, divoom_menu_control_get_disp_str(set_keys[DIVOOM_ASTRO_POMO_SET_RING_ROW]));
    lv_obj_set_style_text_color(lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 12, 0);

    chev = lv_img_create(body);
    divoom_astrootoo_ui_apply_sublevel_chevron_size(chev);
    divoom_menu_control_lv_img_set_src(chev, DIVOOM_ASTROTOO_TOOLS_CHEVRON);
    lv_obj_align(chev, LV_ALIGN_RIGHT_MID, -8, 0);

    val = lv_label_create(body);
    lv_obj_set_style_text_color(val, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(val, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
    lv_label_set_text(val, "");
    lv_obj_align(val, LV_ALIGN_RIGHT_MID,
        -(8 + DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W + DIVOOM_ASTRO_LIST_LABEL_GAP), 0);

    toggle = lv_obj_create(btn);
    lv_obj_add_style(toggle, &phandle->ui.card_item_style, 0);
    lv_obj_set_size(toggle, DIVOOM_ASTRO_ALARM_TOGGLE_W, DIVOOM_ASTRO_CARD_ROW_H);
    lv_obj_set_pos(toggle, DIVOOM_ASTRO_POMO_RING_BODY_W + DIVOOM_ASTRO_ALARM_COL_GAP, 0);
    lv_obj_clear_flag(toggle, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(toggle, 0, 0);

    phandle->pomo_set_ring_sw = lv_img_create(toggle);
    lv_obj_set_size(phandle->pomo_set_ring_sw, 44, 24);
    divoom_menu_control_lv_img_set_src(phandle->pomo_set_ring_sw, DIVOOM_ASTROTOO_POMO_SWITCH_ON);
    lv_obj_center(phandle->pomo_set_ring_sw);

    phandle->pomo_set_row[DIVOOM_ASTRO_POMO_SET_RING_ROW] = btn;
    phandle->pomo_set_val[DIVOOM_ASTRO_POMO_SET_RING_ROW] = val;
    phandle->pomo_set_ring_body = body;
    phandle->pomo_set_ring_toggle = toggle;
    phandle->pomo_set_ring_chevron = chev;
    divoom_astrootoo_ui_list_kv_bind_row(lbl, val,
        DIVOOM_ASTRO_POMO_RING_BODY_W, DIVOOM_ASTRO_TOOLS_CARD_LABEL_INSET_L,
        DIVOOM_ASTRO_TOOLS_CARD_LABEL_INSET_R, DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W,
        -(DIVOOM_ASTRO_TOOLS_CARD_LABEL_INSET_R + DIVOOM_ASTRO_SUBLEVEL_CHEVRON_W
            + DIVOOM_ASTRO_LIST_LABEL_GAP),
        72);
    divoom_astrootoo_ui_list_row_bind_marquee(btn, val);

    lv_obj_add_event_cb(btn, divoom_astrootoo_tools_pomo_set_key_cb, LV_EVENT_KEY, phandle);
    lv_group_add_obj(lv_group_get_default(), btn);
    divoom_menu_direct_ctrl_update(phandle->pomo_set_direct, btn, 0, DIVOOM_ASTRO_POMO_SET_RING_ROW);

    lv_obj_add_flag(phandle->pomo_set_cont, LV_OBJ_FLAG_HIDDEN);
}

/* ===== 编辑(单值)子页构建（深色顶栏 + “-” 大字值 “+” + 单位） ===== */
static void divoom_astrootoo_tools_pomo_build_edit(Pdivoom_astrootoo_tools_ctrl phandle)
{
    lv_obj_t *frame;
    lv_obj_t *minus;
    lv_obj_t *plus;

    phandle->pomo_edit_cont = lv_obj_create(phandle->root_obj);
    lv_obj_add_style(phandle->pomo_edit_cont, &phandle->ui.root_style, 0);
    lv_obj_set_size(phandle->pomo_edit_cont, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_pos(phandle->pomo_edit_cont, 0, 0);
    lv_obj_clear_flag(phandle->pomo_edit_cont, LV_OBJ_FLAG_SCROLLABLE);

    /* 顶栏标题留空，叠加可动态改文案的独立 label（避免与内置标题重叠） */
    phandle->pomo_edit_title_lbl = lv_label_create(
        divoom_astrootoo_ui_create_page_header_tab(phandle->pomo_edit_cont, &phandle->ui,
            "", DIVOOM_ASTROTOO_TOOLS_HEADER, phandle->menu_id));
    lv_obj_set_style_text_color(phandle->pomo_edit_title_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(phandle->pomo_edit_title_lbl, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
    lv_obj_center(phandle->pomo_edit_title_lbl);

    /* 数值边框（真源 editValueFrame：228×132，2px #888888，圆角 16，透明底） */
    frame = lv_obj_create(phandle->pomo_edit_cont);
    lv_obj_clear_flag(frame, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(frame, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(frame, lv_color_hex(0x888888), 0);
    lv_obj_set_style_border_width(frame, 2, 0);
    lv_obj_set_style_radius(frame, 16, 0);
    lv_obj_set_style_pad_all(frame, 0, 0);
    lv_obj_set_style_shadow_width(frame, 0, 0);
    lv_obj_set_size(frame, 228, 132);
    lv_obj_align(frame, LV_ALIGN_CENTER, 0, -16);

    minus = lv_label_create(phandle->pomo_edit_cont);
    lv_label_set_text(minus, "-");
    lv_obj_set_style_text_color(minus, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(minus, divoom_menu_control_get_font_info(96, phandle->menu_id), 0);
    lv_obj_align(minus, LV_ALIGN_LEFT_MID, 40, -16);

    plus = lv_label_create(phandle->pomo_edit_cont);
    lv_label_set_text(plus, "+");
    lv_obj_set_style_text_color(plus, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(plus, divoom_menu_control_get_font_info(96, phandle->menu_id), 0);
    lv_obj_align(plus, LV_ALIGN_RIGHT_MID, -40, -16);

    phandle->pomo_edit_val_lbl = lv_label_create(phandle->pomo_edit_cont);
    lv_obj_set_style_text_color(phandle->pomo_edit_val_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(phandle->pomo_edit_val_lbl, divoom_menu_control_get_font_info(DIVOOM_ASTRO_POMO_BIG_FONT, phandle->menu_id), 0);
    lv_label_set_text(phandle->pomo_edit_val_lbl, "25");
    lv_obj_align(phandle->pomo_edit_val_lbl, LV_ALIGN_CENTER, 0, -16);

    phandle->pomo_edit_unit_lbl = lv_label_create(phandle->pomo_edit_cont);
    lv_label_set_text(phandle->pomo_edit_unit_lbl, divoom_menu_control_get_disp_str("Min"));
    lv_obj_set_style_text_color(phandle->pomo_edit_unit_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(phandle->pomo_edit_unit_lbl, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
    lv_obj_align(phandle->pomo_edit_unit_lbl, LV_ALIGN_CENTER, 0, 84);

    phandle->pomo_edit_key_btn = divoom_astrootoo_ui_create_key_capture_btn(phandle->pomo_edit_cont,
        &phandle->ui, divoom_astrootoo_tools_pomo_edit_key_cb, phandle);
    lv_group_add_obj(lv_group_get_default(), phandle->pomo_edit_key_btn);

    lv_obj_add_flag(phandle->pomo_edit_cont, LV_OBJ_FLAG_HIDDEN);
}

/* 列表/栅格项通用底+聚焦样式（灰底 STROKE 边 / 聚焦琥珀底 2px 边，去外圈） */
static void divoom_astrootoo_tools_pomo_slot_style(lv_obj_t *btn)
{
    lv_obj_remove_style_all(btn);
    lv_obj_set_style_bg_color(btn, lv_color_hex(DIVOOM_ASTRO_SURFACE_IDLE), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn, DIVOOM_ASTRO_RADIUS_SM, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_outline_width(btn, 0, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(btn, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(btn, 2, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_bg_color(btn, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(btn, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
}

/* 重复预设行勾选刷新（仅当前生效预设显示勾） */
static void divoom_astrootoo_tools_pomo_repeat_refresh(Pdivoom_astrootoo_tools_ctrl phandle)
{
    int i;

    for (i = 0; i < DIVOOM_ASTRO_POMO_REPEAT_CNT; i++)
    {
        if (!phandle->pomo_repeat_check[i])
        {
            continue;
        }
        if (g_pomo_repeat_order[i] == phandle->pomo_repeat_id)
        {
            lv_obj_clear_flag(phandle->pomo_repeat_check[i], LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(phandle->pomo_repeat_check[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/* ===== 重复预设选择子页按键 ===== */
static void divoom_astrootoo_tools_pomo_repeat_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_tools_ctrl phandle;
    lv_obj_t *target;
    lv_obj_t *next;
    uint8_t key_code;
    int i;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    phandle = (Pdivoom_astrootoo_tools_ctrl)lv_event_get_user_data(e);
    target = lv_event_get_target(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_astrootoo_tools_pomo_show_settings(phandle);
        return;
    }
    if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code))
    {
        int direct = divoom_astrootoo_ui_key_is_up(key_code) ? DIVOOM_MENU_DIRECT_UP : DIVOOM_MENU_DIRECT_DOWN;
        next = divoom_menu_direct_ctrl_get(phandle->pomo_repeat_direct, target, direct);
        if (next && next != target)
        {
            for (i = 0; i < DIVOOM_ASTRO_POMO_REPEAT_CNT; i++)
            {
                if (phandle->pomo_repeat_row[i] == next)
                {
                    phandle->pomo_repeat_focus = i;
                    break;
                }
            }
            lv_group_focus_obj(next);
            divoom_astrootoo_ui_focus_scroll(next);
        }
        return;
    }
    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        phandle->pomo_repeat_id = g_pomo_repeat_order[phandle->pomo_repeat_focus];
        phandle->pomo_cycles = divoom_astrootoo_tools_pomo_repeat_cycles(phandle->pomo_repeat_id);
        divoom_astrootoo_tools_pomo_build_seq(phandle);
        divoom_astrootoo_tools_pomo_save_config(phandle);
        divoom_astrootoo_tools_pomo_repeat_refresh(phandle);
        divoom_astrootoo_tools_pomo_show_settings(phandle);
    }
}

/* ===== 重复预设选择子页构建（深色顶栏 + 5 行 72px 序列 + 勾选） ===== */
static void divoom_astrootoo_tools_pomo_build_repeat(Pdivoom_astrootoo_tools_ctrl phandle)
{
    lv_coord_t row_w = DIVOOM_ASTRO_POMO_REPEAT_ROW_W;
    lv_coord_t row_h = DIVOOM_ASTRO_POMO_REPEAT_ROW_H;
    lv_coord_t row_x = (DIVOOM_SCREEN_WIDTH - row_w) / 2;
    lv_coord_t top = DIVOOM_ASTRO_POMO_REPEAT_LIST_TOP;
    int i;

    phandle->pomo_repeat_cont = lv_obj_create(phandle->root_obj);
    lv_obj_add_style(phandle->pomo_repeat_cont, &phandle->ui.root_style, 0);
    lv_obj_set_size(phandle->pomo_repeat_cont, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_pos(phandle->pomo_repeat_cont, 0, 0);
    lv_obj_clear_flag(phandle->pomo_repeat_cont, LV_OBJ_FLAG_SCROLLABLE);

    divoom_astrootoo_ui_create_page_header_tab(phandle->pomo_repeat_cont, &phandle->ui,
        "Repeat", DIVOOM_ASTROTOO_TOOLS_HEADER, phandle->menu_id);

    phandle->pomo_repeat_direct = divoom_menu_direct_ctrl_create(1, DIVOOM_ASTRO_POMO_REPEAT_CNT);
    for (i = 0; i < DIVOOM_ASTRO_POMO_REPEAT_CNT; i++)
    {
        lv_obj_t *btn;
        lv_obj_t *chk;
        int kinds[12];
        int n = 0;
        int k;
        lv_coord_t ix = DIVOOM_ASTRO_POMO_REPEAT_PAD_L;

        btn = lv_btn_create(phandle->pomo_repeat_cont);
        divoom_astrootoo_tools_pomo_slot_style(btn);
        lv_obj_set_size(btn, row_w, row_h);
        lv_obj_set_pos(btn, row_x, top + i * (row_h + DIVOOM_ASTRO_POMO_REPEAT_ROW_GAP));
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

        divoom_astrootoo_tools_pomo_build_seq_id(phandle, g_pomo_repeat_order[i], kinds, &n);
        for (k = 0; k < n; k++)
        {
            lv_obj_t *icon = lv_img_create(btn);
            divoom_menu_control_lv_img_set_src(icon,
                divoom_astrootoo_tools_pomo_icon_path(kinds[k], 1));
            lv_obj_align(icon, LV_ALIGN_LEFT_MID, ix, 0);
            ix += 48 + DIVOOM_ASTRO_POMO_REPEAT_ICON_GAP;
        }

        chk = lv_img_create(btn);
        lv_obj_set_size(chk, 24, 24);
        divoom_menu_control_lv_img_set_src(chk, DIVOOM_ASTRO_POMO_BELL_DIR "at_repeat_check.png");
        lv_obj_align(chk, LV_ALIGN_RIGHT_MID, -DIVOOM_ASTRO_POMO_REPEAT_PAD_R, 0);
        lv_obj_add_flag(chk, LV_OBJ_FLAG_HIDDEN);

        lv_obj_add_event_cb(btn, divoom_astrootoo_tools_pomo_repeat_key_cb, LV_EVENT_KEY, phandle);
        divoom_menu_direct_ctrl_update(phandle->pomo_repeat_direct, btn, 0, i);
        lv_group_add_obj(lv_group_get_default(), btn);
        phandle->pomo_repeat_row[i] = btn;
        phandle->pomo_repeat_check[i] = chk;
    }
    lv_obj_add_flag(phandle->pomo_repeat_cont, LV_OBJ_FLAG_HIDDEN);
}

/* 铃声格勾选刷新（仅当前选中显示勾） */
static void divoom_astrootoo_tools_pomo_ring_refresh(Pdivoom_astrootoo_tools_ctrl phandle)
{
    int i;

    for (i = 0; i < DIVOOM_ASTRO_POMO_RING_CNT; i++)
    {
        if (!phandle->pomo_ring_check[i])
        {
            continue;
        }
        if (i == phandle->pomo_ring)
        {
            lv_obj_clear_flag(phandle->pomo_ring_check[i], LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(phandle->pomo_ring_check[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static uint8_t divoom_astrootoo_tools_pomo_ring_preview_vol(void)
{
    Pdivoom_tomato_time_info info;

    info = divoom_astrootoo_tools_pomo_get_backend_info();
    if (info && info->sound_volume > 0)
    {
        return info->sound_volume;
    }
    return 50;
}

static void divoom_astrootoo_tools_pomo_ring_preview(int index)
{
    if (index < 0 || index >= DIVOOM_ASTRO_POMO_RING_CNT)
    {
        return;
    }
    divoom_tomato_time_try_listen((uint8)index, divoom_astrootoo_tools_pomo_ring_preview_vol(), 1, 1);
}

/* ===== 铃声选择子页按键（3×4 栅格，方向键导航） ===== */
static void divoom_astrootoo_tools_pomo_ring_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_tools_ctrl phandle;
    lv_obj_t *target;
    lv_obj_t *next;
    uint8_t key_code;
    int direct;
    int i;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    phandle = (Pdivoom_astrootoo_tools_ctrl)lv_event_get_user_data(e);
    target = lv_event_get_target(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_tomato_time_try_listen(0, 0, 0, 1);
        divoom_astrootoo_tools_pomo_show_settings(phandle);
        return;
    }
    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        for (i = 0; i < DIVOOM_ASTRO_POMO_RING_CNT; i++)
        {
            if (phandle->pomo_ring_btn[i] == target)
            {
                phandle->pomo_ring = i;
                break;
            }
        }
        divoom_tomato_time_try_listen(0, 0, 0, 1);
        divoom_tomato_time_set_system_bell((uint8)phandle->pomo_ring);
        divoom_astrootoo_tools_pomo_ring_refresh(phandle);
        divoom_astrootoo_tools_pomo_set_refresh(phandle);
        divoom_astrootoo_tools_pomo_show_settings(phandle);
        return;
    }
    direct = -1;
    if (divoom_astrootoo_ui_key_is_up(key_code)) direct = DIVOOM_MENU_DIRECT_UP;
    else if (divoom_astrootoo_ui_key_is_down(key_code)) direct = DIVOOM_MENU_DIRECT_DOWN;
    else if (divoom_astrootoo_ui_key_is_left(key_code)) direct = DIVOOM_MENU_DIRECT_LEFT;
    else if (divoom_astrootoo_ui_key_is_right(key_code)) direct = DIVOOM_MENU_DIRECT_RIGHT;
    if (direct < 0)
    {
        return;
    }
    next = divoom_menu_direct_ctrl_get(phandle->pomo_ring_direct, target, direct);
    if (next && next != target)
    {
        for (i = 0; i < DIVOOM_ASTRO_POMO_RING_CNT; i++)
        {
            if (phandle->pomo_ring_btn[i] == next)
            {
                phandle->pomo_ring_focus = i;
                break;
            }
        }
        lv_group_focus_obj(next);
        divoom_astrootoo_ui_focus_scroll(next);
        divoom_astrootoo_tools_pomo_ring_preview(phandle->pomo_ring_focus);
    }
}

/* ===== 铃声选择子页构建（深色顶栏 + 3×4 经典铃声图标栅格 + 勾选） ===== */
static void divoom_astrootoo_tools_pomo_build_ringtone(Pdivoom_astrootoo_tools_ctrl phandle)
{
    lv_coord_t top = DIVOOM_ASTRO_HEADER_H + 8;
    lv_coord_t avail_h = DIVOOM_SCREEN_HIGH - top;
    lv_coord_t cell_w = (DIVOOM_SCREEN_WIDTH - DIVOOM_ASTRO_POMO_RING_PAD * 2
        - DIVOOM_ASTRO_POMO_RING_GAP * (DIVOOM_ASTRO_POMO_RING_COLS - 1)) / DIVOOM_ASTRO_POMO_RING_COLS;
    lv_coord_t cell_h = (avail_h - DIVOOM_ASTRO_POMO_RING_PAD * 2
        - DIVOOM_ASTRO_POMO_RING_GAP * (DIVOOM_ASTRO_POMO_RING_ROWS - 1)) / DIVOOM_ASTRO_POMO_RING_ROWS;
    int i;

    phandle->pomo_ring_cont = lv_obj_create(phandle->root_obj);
    lv_obj_add_style(phandle->pomo_ring_cont, &phandle->ui.root_style, 0);
    lv_obj_set_size(phandle->pomo_ring_cont, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_pos(phandle->pomo_ring_cont, 0, 0);
    lv_obj_clear_flag(phandle->pomo_ring_cont, LV_OBJ_FLAG_SCROLLABLE);

    divoom_astrootoo_ui_create_page_header_tab(phandle->pomo_ring_cont, &phandle->ui,
        "Ringtone", DIVOOM_ASTROTOO_TOOLS_HEADER, phandle->menu_id);

    phandle->pomo_ring_direct = divoom_menu_direct_ctrl_create(DIVOOM_ASTRO_POMO_RING_COLS,
        DIVOOM_ASTRO_POMO_RING_ROWS);
    for (i = 0; i < DIVOOM_ASTRO_POMO_RING_CNT; i++)
    {
        int col = i % DIVOOM_ASTRO_POMO_RING_COLS;
        int row = i / DIVOOM_ASTRO_POMO_RING_COLS;
        lv_obj_t *btn;
        lv_obj_t *icon;
        lv_obj_t *chk;

        btn = lv_btn_create(phandle->pomo_ring_cont);
        divoom_astrootoo_tools_pomo_slot_style(btn);
        lv_obj_set_size(btn, cell_w, cell_h);
        lv_obj_set_pos(btn, DIVOOM_ASTRO_POMO_RING_PAD + col * (cell_w + DIVOOM_ASTRO_POMO_RING_GAP),
            top + DIVOOM_ASTRO_POMO_RING_PAD + row * (cell_h + DIVOOM_ASTRO_POMO_RING_GAP));
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

        icon = lv_img_create(btn);
        {
            char path[48];
            snprintf(path, sizeof(path), DIVOOM_ASTRO_POMO_BELL_DIR "%s", g_pomo_bell_icons[i]);
            divoom_menu_control_lv_img_set_src(icon, path);
        }
        lv_obj_center(icon);

        chk = lv_img_create(btn);
        lv_obj_set_size(chk, 24, 24);
        divoom_menu_control_lv_img_set_src(chk, DIVOOM_ASTRO_POMO_BELL_DIR "at_repeat_check.png");
        lv_obj_align(chk, LV_ALIGN_BOTTOM_RIGHT, -4, -4);
        lv_obj_add_flag(chk, LV_OBJ_FLAG_HIDDEN);

        lv_obj_add_event_cb(btn, divoom_astrootoo_tools_pomo_ring_key_cb, LV_EVENT_KEY, phandle);
        divoom_menu_direct_ctrl_update(phandle->pomo_ring_direct, btn, col, row);
        lv_group_add_obj(lv_group_get_default(), btn);
        phandle->pomo_ring_btn[i] = btn;
        phandle->pomo_ring_check[i] = chk;
    }
    lv_obj_add_flag(phandle->pomo_ring_cont, LV_OBJ_FLAG_HIDDEN);
}

static void divoom_astrootoo_tools_pomo_show_repeat(Pdivoom_astrootoo_tools_ctrl phandle)
{
    int i;

    divoom_astrootoo_tools_pomo_progress_motion_stop(phandle);
    phandle->pomo_view = 3;
    if (phandle->pomo_set_cont)
    {
        lv_obj_add_flag(phandle->pomo_set_cont, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->pomo_repeat_cont)
    {
        lv_obj_clear_flag(phandle->pomo_repeat_cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(phandle->pomo_repeat_cont);
    }
    /* 焦点落在当前生效预设行 */
    phandle->pomo_repeat_focus = 0;
    for (i = 0; i < DIVOOM_ASTRO_POMO_REPEAT_CNT; i++)
    {
        if (g_pomo_repeat_order[i] == phandle->pomo_repeat_id)
        {
            phandle->pomo_repeat_focus = i;
            break;
        }
    }
    divoom_astrootoo_tools_pomo_repeat_refresh(phandle);
    divoom_astrootoo_tools_pomo_bind_group(phandle, NULL);
}

static void divoom_astrootoo_tools_pomo_show_ringtone(Pdivoom_astrootoo_tools_ctrl phandle)
{
    divoom_astrootoo_tools_pomo_progress_motion_stop(phandle);
    phandle->pomo_view = 4;
    if (phandle->pomo_set_cont)
    {
        lv_obj_add_flag(phandle->pomo_set_cont, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->pomo_ring_cont)
    {
        lv_obj_clear_flag(phandle->pomo_ring_cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(phandle->pomo_ring_cont);
    }
    phandle->pomo_ring_focus = phandle->pomo_ring;
    if (phandle->pomo_ring_focus < 0 || phandle->pomo_ring_focus >= DIVOOM_ASTRO_POMO_RING_CNT)
    {
        phandle->pomo_ring_focus = 0;
    }
    divoom_astrootoo_tools_pomo_ring_refresh(phandle);
    divoom_astrootoo_tools_pomo_bind_group(phandle, NULL);
    divoom_astrootoo_tools_pomo_ring_preview(phandle->pomo_ring_focus);
}

/* 真源 PomodoroScreen(run 视图)：无顶栏奶油底 + 阶段/番茄计数 chrome + 白描边大字卡
 * + 进度图标行 + 底栏(齿轮/主操作，样式同计时器)；齿轮 -> 设置子页，主钮 -> 开始/暂停/继续 */
static void divoom_astrootoo_tools_build_pomodoro(Pdivoom_astrootoo_tools_ctrl phandle)
{
    lv_obj_t *card;
    lv_obj_t *tomato;
    int i;

    phandle->pomo_mode = 0;
    phandle->pomo_prep_sec = 0;
    phandle->pomo_prep_next_ms = 0;
    phandle->pomo_phase = 0;
    phandle->pomo_round = 0;
    phandle->pomo_today_cnt = 0;
    phandle->pomo_work_min = DIVOOM_ASTRO_POMO_WORK_DEF;
    phandle->pomo_short_min = DIVOOM_ASTRO_POMO_SHORT_DEF;
    phandle->pomo_long_min = DIVOOM_ASTRO_POMO_LONG_DEF;
    phandle->pomo_repeat_id = DIVOOM_ASTRO_POMO_CYCLES_DEF;
    phandle->pomo_cycles = divoom_astrootoo_tools_pomo_repeat_cycles(phandle->pomo_repeat_id);
    phandle->pomo_ring = 0;
    phandle->pomo_ring_on = 1;
    phandle->pomo_tomato_index = 0;
    phandle->pomo_view = 0;
    phandle->pomo_set_focus = 0;
    phandle->pomo_set_ring_col = DIVOOM_ASTRO_POMO_RING_COL_BODY;
    phandle->pomo_set_ring_body = NULL;
    phandle->pomo_set_ring_toggle = NULL;
    phandle->pomo_set_ring_sw = NULL;
    phandle->pomo_set_ring_chevron = NULL;
    phandle->pomo_last_through = -2;
    phandle->pomo_motion_frontier = -1;
    phandle->pomo_motion_next = -1;
    phandle->pomo_motion_on = 0;
    phandle->remain_sec = 0;
    divoom_astrootoo_tools_pomo_build_seq(phandle);

    /* 左上任务名：定宽单行循环滚（对齐 T113）；idle 时 UP/DOWN 切换 */
    phandle->status_lbl = lv_label_create(phandle->root_obj);
    lv_obj_set_style_text_color(phandle->status_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(phandle->status_lbl, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
    {
        const lv_font_t *name_font = lv_obj_get_style_text_font(phandle->status_lbl, 0);
        lv_coord_t name_h = name_font ? (lv_coord_t)lv_font_get_line_height(name_font) : 32;

        lv_obj_set_width(phandle->status_lbl, 300);
        lv_obj_set_height(phandle->status_lbl, name_h);
        lv_obj_set_style_max_height(phandle->status_lbl, name_h, 0);
    }
    lv_label_set_long_mode(phandle->status_lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_anim_speed(phandle->status_lbl, 30, 0);
    lv_obj_set_pos(phandle->status_lbl, 32, 32);
    divoom_astrootoo_tools_pomo_sync_tomato_index(phandle);

    /* 右上番茄计数：彩色番茄图标 48×48 + x0（真源 chromeRight） */
    tomato = lv_img_create(phandle->root_obj);
    divoom_menu_control_lv_img_set_src(tomato, DIVOOM_ASTROTOO_TOOLS_UI_DIR "pomo_tomato.png");
    lv_obj_set_pos(tomato, 356, 22);
    phandle->pomo_tomato_icon = tomato;

    phandle->pomo_count_lbl = lv_label_create(phandle->root_obj);
    lv_obj_set_style_text_color(phandle->pomo_count_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(phandle->pomo_count_lbl, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
    lv_obj_set_pos(phandle->pomo_count_lbl, 410, 32);

    /* 大字时间卡片：白描边圆角 460×180 @ (10,115)，内置 80px 大字居中 */
    card = lv_obj_create(phandle->root_obj);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(card, lv_color_hex(DIVOOM_ASTRO_BG), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_set_size(card, 460, 180);
    lv_obj_set_pos(card, 10, 115);
    phandle->pomo_time_card = card;

    phandle->time_lbl = lv_label_create(card);
    lv_obj_set_style_text_font(phandle->time_lbl, divoom_menu_control_get_font_info(DIVOOM_ASTRO_POMO_BIG_FONT, phandle->menu_id), 0);
    lv_obj_set_style_text_color(phandle->time_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_label_set_text(phandle->time_lbl, "25:00");
    lv_obj_center(phandle->time_lbl);

    /* 进度序列图标：预创建最大格数，按当前序列居中显隐（真源 progressRow top=314，48×48，gap 6） */
    for (i = 0; i < DIVOOM_ASTRO_POMO_PROG_MAX; i++)
    {
        phandle->pomo_prog[i] = lv_img_create(phandle->root_obj);
        divoom_menu_control_lv_img_set_src(phandle->pomo_prog[i],
            divoom_astrootoo_tools_pomo_icon_path(0, 0));
        lv_obj_add_flag(phandle->pomo_prog[i], LV_OBJ_FLAG_HIDDEN);
    }
    phandle->pomo_layout_dirty = 1;

    /* 底栏：样式同计时器 card_item_*；尺寸真源 runFooter（72 齿轮 + flex 主钮，gap 16，h 64 @ y382） */
    phandle->btn_left_lbl = NULL;
    phandle->btn_left = divoom_astrootoo_tools_create_icon_action_btn(phandle, phandle->root_obj,
        DIVOOM_ASTROTOO_TOOLS_ICON_POMO_GEAR, DIVOOM_ASTRO_POMO_FOOTER_X, DIVOOM_ASTRO_POMO_FOOTER_Y,
        DIVOOM_ASTRO_POMO_GEAR_BTN_W, DIVOOM_ASTRO_POMO_FOOTER_H, &phandle->btn_left_icon);
    phandle->btn_right = divoom_astrootoo_tools_create_icon_action_btn(phandle, phandle->root_obj,
        DIVOOM_ASTROTOO_TOOLS_ICON_PLAY,
        DIVOOM_ASTRO_POMO_FOOTER_X + DIVOOM_ASTRO_POMO_GEAR_BTN_W + DIVOOM_ASTRO_POMO_FOOTER_GAP,
        DIVOOM_ASTRO_POMO_FOOTER_Y, DIVOOM_ASTRO_POMO_MAIN_BTN_W, DIVOOM_ASTRO_POMO_FOOTER_H,
        &phandle->btn_right_icon);

    phandle->btn_right_lbl = lv_label_create(phandle->btn_right);
    lv_label_set_text(phandle->btn_right_lbl, "3");
    lv_obj_set_style_text_color(phandle->btn_right_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(phandle->btn_right_lbl, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
    lv_obj_center(phandle->btn_right_lbl);
    lv_obj_add_flag(phandle->btn_right_lbl, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_event_cb(phandle->btn_left, divoom_astrootoo_tools_pomo_key_cb, LV_EVENT_KEY, phandle);
    lv_obj_add_event_cb(phandle->btn_right, divoom_astrootoo_tools_pomo_key_cb, LV_EVENT_KEY, phandle);
    lv_group_add_obj(lv_group_get_default(), phandle->btn_left);
    lv_group_add_obj(lv_group_get_default(), phandle->btn_right);

    /* 子页（设置 / 编辑 / 重复 / 铃声）预构建并隐藏，避免在 key 回调内创建对象/加载图片 */
    divoom_astrootoo_tools_pomo_build_settings(phandle);
    divoom_astrootoo_tools_pomo_build_edit(phandle);
    divoom_astrootoo_tools_pomo_build_repeat(phandle);
    divoom_astrootoo_tools_pomo_build_ringtone(phandle);

    divoom_astrootoo_tools_pomo_refresh(phandle);
    divoom_astrootoo_tools_focus_lr(phandle, 1);
    phandle->timer = lv_timer_create(divoom_astrootoo_tools_pomo_timer_cb, 1000, phandle);
}

/* ====================================================================== */
/* 记分板（Scoreboard）                                                    */
/* score_a=BlueScore（上），score_b=RedScore（下），与 Tools/SetScoreBoard 对齐 */
/* ====================================================================== */
#define DIVOOM_ASTRO_SCORE_POLL_MS      200
#define DIVOOM_ASTRO_SCORE_CARD_H       168   /* 真源 scoreSide max-height */
#define DIVOOM_ASTRO_SCORE_FLEX_GAP     14    /* scoreBoard column gap */
#define DIVOOM_ASTRO_SCORE_VS_RAIL_H    20    /* vsRail 行高 */
#define DIVOOM_ASTRO_SCORE_DOT_SZ       7
#define DIVOOM_ASTRO_SCORE_DOT_GAP      14
#define DIVOOM_ASTRO_SCORE_PAD_TOP      8
#define DIVOOM_ASTRO_SCORE_PAD_BOT      12
/* 顶栏 64 + 列表间距 8，同模拟器 FLOWTOO_V1_LIST_TOP_Y */
#define DIVOOM_ASTRO_SCORE_BODY_TOP     (DIVOOM_ASTRO_HEADER_H + 8)
#define DIVOOM_ASTRO_SCORE_BLOCK_H      (DIVOOM_ASTRO_SCORE_CARD_H * 2 \
    + DIVOOM_ASTRO_SCORE_FLEX_GAP * 2 + DIVOOM_ASTRO_SCORE_VS_RAIL_H)
#define DIVOOM_ASTRO_SCORE_INNER_H      (DIVOOM_SCREEN_HIGH - DIVOOM_ASTRO_SCORE_BODY_TOP \
    - DIVOOM_ASTRO_SCORE_PAD_TOP - DIVOOM_ASTRO_SCORE_PAD_BOT)
#define DIVOOM_ASTRO_SCORE_TOP_Y        (DIVOOM_ASTRO_SCORE_BODY_TOP + DIVOOM_ASTRO_SCORE_PAD_TOP \
    + (DIVOOM_ASTRO_SCORE_INNER_H - DIVOOM_ASTRO_SCORE_BLOCK_H) / 2)

static void divoom_astrootoo_tools_score_load_from_tool(Pdivoom_astrootoo_tools_ctrl phandle)
{
    divoom_app_tool_score_info score_info = {0};

    divoom_app_tool_get_score(&score_info);
    phandle->score_a = (int)score_info.blue_score;
    phandle->score_b = (int)score_info.red_score;
    if (phandle->score_a < 0)
    {
        phandle->score_a = 0;
    }
    else if (phandle->score_a > 999)
    {
        phandle->score_a = 999;
    }
    if (phandle->score_b < 0)
    {
        phandle->score_b = 0;
    }
    else if (phandle->score_b > 999)
    {
        phandle->score_b = 999;
    }
}

static void divoom_astrootoo_tools_score_push_to_tool(Pdivoom_astrootoo_tools_ctrl phandle)
{
    divoom_app_tool_score_info score_info = {0};

    score_info.blue_score = (uint16)phandle->score_a;
    score_info.red_score = (uint16)phandle->score_b;
    /* 同步写入 app_tool；已在记分牌页时不会 clearall 重载 */
    divoom_app_tool_set_score(&score_info);
}

static void divoom_astrootoo_tools_score_refresh(Pdivoom_astrootoo_tools_ctrl phandle)
{
    char buf[8];

    if (!phandle->score_a_lbl || !phandle->score_b_lbl)
    {
        return;
    }

    lv_snprintf(buf, sizeof(buf), "%03d", phandle->score_a);
    lv_label_set_text(phandle->score_a_lbl, buf);
    lv_snprintf(buf, sizeof(buf), "%03d", phandle->score_b);
    lv_label_set_text(phandle->score_b_lbl, buf);

    if (phandle->score_sel == 0)
    {
        if (phandle->score_a_card)
        {
            lv_group_focus_obj(phandle->score_a_card);
        }
    }
    else if (phandle->score_b_card)
    {
        lv_group_focus_obj(phandle->score_b_card);
    }
}

/* App Tools/SetScoreBoard 写入 app_tool 后，轮询同步到当前页（避免仅本地 score_a/b） */
static void divoom_astrootoo_tools_score_timer_cb(lv_timer_t *t)
{
    Pdivoom_astrootoo_tools_ctrl phandle;
    divoom_app_tool_score_info score_info = {0};
    int blue;
    int red;

    phandle = (Pdivoom_astrootoo_tools_ctrl)lv_timer_get_user_data(t);
    if (!phandle || phandle->page != DIVOOM_ASTROTOO_TOOLS_PAGE_SCOREBOARD)
    {
        return;
    }

    divoom_app_tool_get_score(&score_info);
    blue = (int)score_info.blue_score;
    red = (int)score_info.red_score;
    if (blue < 0)
    {
        blue = 0;
    }
    else if (blue > 999)
    {
        blue = 999;
    }
    if (red < 0)
    {
        red = 0;
    }
    else if (red > 999)
    {
        red = 999;
    }

    if (phandle->score_a == blue && phandle->score_b == red)
    {
        return;
    }
    phandle->score_a = blue;
    phandle->score_b = red;
    divoom_astrootoo_tools_score_refresh(phandle);
}

static void divoom_astrootoo_tools_score_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_tools_ctrl phandle;
    uint8_t key_code;
    int *score;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }

    phandle = (Pdivoom_astrootoo_tools_ctrl)lv_event_get_user_data(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_menu_control_return_pre();
        return;
    }
    if (divoom_astrootoo_ui_key_is_up(key_code))
    {
        phandle->score_sel = 0;
        divoom_astrootoo_tools_score_refresh(phandle);
        return;
    }
    if (divoom_astrootoo_ui_key_is_down(key_code))
    {
        phandle->score_sel = 1;
        divoom_astrootoo_tools_score_refresh(phandle);
        return;
    }

    score = (phandle->score_sel == 0) ? &phandle->score_a : &phandle->score_b;
    if (divoom_astrootoo_ui_key_is_right(key_code) || divoom_astrootoo_ui_key_is_enter(key_code))
    {
        if (*score < 999)
        {
            (*score)++;
        }
        divoom_astrootoo_tools_score_refresh(phandle);
        divoom_astrootoo_tools_score_push_to_tool(phandle);
    }
    else if (divoom_astrootoo_ui_key_is_left(key_code))
    {
        if (*score > 0)
        {
            (*score)--;
        }
        divoom_astrootoo_tools_score_refresh(phandle);
        divoom_astrootoo_tools_score_push_to_tool(phandle);
    }
}

static lv_obj_t *divoom_astrootoo_tools_create_score_card(Pdivoom_astrootoo_tools_ctrl phandle,
    lv_coord_t y, lv_color_t num_color, lv_obj_t **out_lbl)
{
    lv_obj_t *card;
    lv_obj_t *lbl;

    card = lv_btn_create(phandle->root_obj);
    /* 真源记分板：聚焦卡用浅奶油底(#F5E6C8)+琥珀边，数字保持队伍色，区别于普通琥珀填充 */
    lv_obj_add_style(card, &phandle->ui.card_item_style, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xF6E7C6), LV_STATE_FOCUSED);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xF6E7C6), LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(card, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(card, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(card, 2, LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(card, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_size(card, DIVOOM_ASTRO_TOOL_BAR_W, DIVOOM_ASTRO_SCORE_CARD_H);
    lv_obj_set_pos(card, DIVOOM_ASTRO_LIST_PAD_X, y);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lbl = lv_label_create(card);
    lv_label_set_text(lbl, "0");
    lv_obj_set_style_text_color(lbl, num_color, 0);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(72, phandle->menu_id), 0);
    lv_obj_center(lbl);
    if (out_lbl)
    {
        *out_lbl = lbl;
    }

    lv_obj_add_event_cb(card, divoom_astrootoo_tools_score_key_cb, LV_EVENT_KEY, phandle);
    lv_group_add_obj(lv_group_get_default(), card);
    return card;
}

/* 真源 vsRail：20px 行内两颗 7px 圆点，水平居中 */
static void divoom_astrootoo_tools_create_score_dots(Pdivoom_astrootoo_tools_ctrl phandle,
    lv_coord_t rail_y)
{
    lv_obj_t *dot;
    lv_coord_t dot_y;
    lv_coord_t x0;
    lv_coord_t w;
    int i;

    w = DIVOOM_ASTRO_SCORE_DOT_SZ * 2 + DIVOOM_ASTRO_SCORE_DOT_GAP;
    x0 = (DIVOOM_SCREEN_WIDTH - w) / 2;
    dot_y = rail_y + (DIVOOM_ASTRO_SCORE_VS_RAIL_H - DIVOOM_ASTRO_SCORE_DOT_SZ) / 2;

    for (i = 0; i < 2; i++)
    {
        dot = lv_obj_create(phandle->root_obj);
        lv_obj_set_size(dot, DIVOOM_ASTRO_SCORE_DOT_SZ, DIVOOM_ASTRO_SCORE_DOT_SZ);
        lv_obj_set_pos(dot, x0 + i * (DIVOOM_ASTRO_SCORE_DOT_SZ + DIVOOM_ASTRO_SCORE_DOT_GAP), dot_y);
        lv_obj_set_style_bg_color(dot, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_set_style_pad_all(dot, 0, 0);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    }
}

static void divoom_astrootoo_tools_build_scoreboard(Pdivoom_astrootoo_tools_ctrl phandle)
{
    lv_coord_t card_a_y;
    lv_coord_t rail_y;
    lv_coord_t card_b_y;

    divoom_astrootoo_tools_create_header(phandle, "Scoreboard");

    divoom_astrootoo_tools_score_load_from_tool(phandle);
    phandle->score_sel = 0;

    card_a_y = DIVOOM_ASTRO_SCORE_TOP_Y;
    rail_y = card_a_y + DIVOOM_ASTRO_SCORE_CARD_H + DIVOOM_ASTRO_SCORE_FLEX_GAP;
    card_b_y = rail_y + DIVOOM_ASTRO_SCORE_VS_RAIL_H + DIVOOM_ASTRO_SCORE_FLEX_GAP;

    phandle->score_a_card = divoom_astrootoo_tools_create_score_card(phandle, card_a_y,
        lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), &phandle->score_a_lbl);
    phandle->score_b_card = divoom_astrootoo_tools_create_score_card(phandle, card_b_y,
        lv_color_hex(0x8A93A0), &phandle->score_b_lbl);
    divoom_astrootoo_tools_create_score_dots(phandle, rail_y);

    divoom_astrootoo_tools_score_refresh(phandle);
    phandle->timer = lv_timer_create(divoom_astrootoo_tools_score_timer_cb,
        DIVOOM_ASTRO_SCORE_POLL_MS, phandle);
}

/* ====================================================================== */
/* 噪音检测（Noise Meter）— JL 0x65 dB 推流                              */
/* ====================================================================== */
/* dB 刻度 5 档（真源 NoiseMeterScreen scale：20/40/60/80/100 dB + 环境名） */
static const int g_noise_scale_db[5] = { 20, 40, 60, 80, 100 };
static const char *g_noise_scale_key[5] =
{
    "NoiseEnvNight", "NoiseEnvLibrary", "NoiseEnvTalk", "NoiseEnvSubway", "NoiseEnvDrill"
};

/* 真源 NoiseMeterReferenceScale：高亮离当前读数最近的 20/40/60/80/100 */
static int divoom_astrootoo_tools_noise_nearest(int db)
{
    int i;
    int best;
    int best_dist;
    int dist;

    best = 0;
    best_dist = db - g_noise_scale_db[0];
    if (best_dist < 0)
    {
        best_dist = -best_dist;
    }
    for (i = 1; i < 5; i++)
    {
        dist = db - g_noise_scale_db[i];
        if (dist < 0)
        {
            dist = -dist;
        }
        if (dist < best_dist)
        {
            best_dist = dist;
            best = i;
        }
    }
    return best;
}

/* 16 点正弦，幅值 ±127 */
static const int8_t g_noise_sin16[16] =
{
    0, 49, 90, 117, 127, 117, 90, 49, 0, -49, -90, -117, -127, -117, -90, -49
};

/* 真源 demoBarLevels：按 dB 作底、各列独立律动。无频谱时用相位合成。 */
static lv_coord_t divoom_astrootoo_tools_noise_bar_h(int db, uint32_t phase, int i)
{
    int level;
    int wobble;
    int v;
    int h;

    if (db <= 0)
    {
        return DIVOOM_ASTRO_TOOL_NOISE_BAR_MIN_H;
    }
    if (db < 30)
    {
        db = 30;
    }
    if (db > 100)
    {
        db = 100;
    }
    /* 0.08..1.0 → 8..100 */
    level = 8 + (db - 30) * 92 / 70;
    wobble = 70
        + (int)g_noise_sin16[(phase + (uint32_t)i * 3u) & 15u] * 25 / 127
        + (int)g_noise_sin16[(phase * 2u + (uint32_t)i * 2u) & 15u] * 12 / 127;
    if (wobble < 40)
    {
        wobble = 40;
    }
    if (wobble > 100)
    {
        wobble = 100;
    }
    v = level * wobble / 100;
    if (v < 10)
    {
        v = 10;
    }
    if (v > 100)
    {
        v = 100;
    }
    h = DIVOOM_ASTRO_TOOL_NOISE_BAR_MAX_H * v / 100;
    if (h < DIVOOM_ASTRO_TOOL_NOISE_BAR_MIN_H)
    {
        h = DIVOOM_ASTRO_TOOL_NOISE_BAR_MIN_H;
    }
    return (lv_coord_t)h;
}

static void divoom_astrootoo_tools_noise_timer_cb(lv_timer_t *t)
{
    Pdivoom_astrootoo_tools_ctrl phandle;
    char buf[16];
    int i;
    int base;
    int bucket;
    lv_coord_t h;

    phandle = (Pdivoom_astrootoo_tools_ctrl)lv_timer_get_user_data(t);
    if (!phandle)
    {
        return;
    }

#ifdef DIVOOM_TIMES_VIBE
    base = divoom_btcore_uart_cmd_get_mic_noise_db();
#else
    base = 0;
#endif
    phandle->noise_db = base;
    phandle->noise_phase++;
    lv_snprintf(buf, sizeof(buf), "%d", phandle->noise_db);
    if (phandle->time_lbl)
    {
        lv_label_set_text(phandle->time_lbl, buf);
    }

    for (i = 0; i < DIVOOM_ASTRO_TOOL_NOISE_BARS; i++)
    {
        h = divoom_astrootoo_tools_noise_bar_h(base, phandle->noise_phase, i);
        if (phandle->noise_bars[i])
        {
            lv_obj_set_height(phandle->noise_bars[i], h);
        }
        if (phandle->noise_bar_reflect[i])
        {
            lv_obj_set_height(phandle->noise_bar_reflect[i], h);
        }
    }

    bucket = divoom_astrootoo_tools_noise_nearest(base);
    for (i = 0; i < 5; i++)
    {
        int active;
        lv_color_t ink;

        if (!phandle->noise_scale[i])
        {
            continue;
        }
        active = (i == bucket);
        ink = lv_color_hex(active ? DIVOOM_ASTRO_INK : DIVOOM_ASTRO_INK_MUTED);
        lv_obj_set_style_bg_opa(phandle->noise_scale[i],
            active ? DIVOOM_ASTRO_TOOL_NOISE_ACTIVE_OPA : LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(phandle->noise_scale[i],
            lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
        if (phandle->noise_scale_val[i])
        {
            lv_obj_set_style_text_color(phandle->noise_scale_val[i], ink, 0);
        }
        if (phandle->noise_scale_name[i])
        {
            lv_obj_set_style_text_color(phandle->noise_scale_name[i], ink, 0);
        }
    }
}

static void divoom_astrootoo_tools_noise_key_cb(lv_event_t *e)
{
    uint8_t key_code;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }
    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_menu_control_return_pre();
    }
}

static lv_obj_t *divoom_astrootoo_tools_noise_make_bar(lv_obj_t *parent, int reflect)
{
    lv_obj_t *bar;
    lv_obj_t *shade;

    bar = lv_obj_create(parent);
    lv_obj_set_style_bg_color(bar, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
    lv_obj_set_style_bg_opa(bar, reflect ? 97 : LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_set_style_shadow_width(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(bar, DIVOOM_ASTRO_TOOL_NOISE_BAR_W, DIVOOM_ASTRO_TOOL_NOISE_BAR_MIN_H);
    if (!reflect)
    {
        shade = lv_obj_create(bar);
        lv_obj_set_style_bg_color(shade, lv_color_hex(DIVOOM_ASTRO_TOOL_NOISE_BAR_SHADE), 0);
        lv_obj_set_style_border_width(shade, 0, 0);
        lv_obj_set_style_radius(shade, 0, 0);
        lv_obj_set_style_pad_all(shade, 0, 0);
        lv_obj_set_style_shadow_width(shade, 0, 0);
        lv_obj_clear_flag(shade, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(shade, DIVOOM_ASTRO_TOOL_NOISE_BAR_W, 4);
        lv_obj_align(shade, LV_ALIGN_BOTTOM_MID, 0, 0);
    }
    return bar;
}

static void divoom_astrootoo_tools_build_noise(Pdivoom_astrootoo_tools_ctrl phandle)
{
    lv_obj_t *meter;
    lv_obj_t *db_row;
    lv_obj_t *unit;
    lv_obj_t *stage;
    lv_obj_t *scale_bar;
    int i;
    lv_coord_t cell_w;
    char buf[16];

    divoom_astrootoo_tools_create_header(phandle, "Noisemeter");

    /* 中间区：数字+单位同行，下方镜像律动条（真源 meterZone） */
    meter = lv_obj_create(phandle->root_obj);
    lv_obj_set_style_bg_opa(meter, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(meter, 0, 0);
    lv_obj_set_style_pad_all(meter, 0, 0);
    lv_obj_set_style_pad_row(meter, 24, 0);
    lv_obj_set_style_shadow_width(meter, 0, 0);
    lv_obj_clear_flag(meter, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(meter, DIVOOM_SCREEN_WIDTH,
        DIVOOM_ASTRO_TOOL_NOISE_SCALE_Y - DIVOOM_ASTRO_HEADER_H);
    lv_obj_set_pos(meter, 0, DIVOOM_ASTRO_HEADER_H);
    lv_obj_set_flex_flow(meter, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(meter, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    db_row = lv_obj_create(meter);
    lv_obj_set_style_bg_opa(db_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(db_row, 0, 0);
    lv_obj_set_style_pad_all(db_row, 0, 0);
    lv_obj_set_style_pad_column(db_row, 10, 0);
    lv_obj_set_style_shadow_width(db_row, 0, 0);
    lv_obj_clear_flag(db_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(db_row, DIVOOM_SCREEN_WIDTH, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(db_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(db_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);

    phandle->time_lbl = lv_label_create(db_row);
    lv_obj_set_style_text_font(phandle->time_lbl,
        divoom_menu_control_get_font_info(72, phandle->menu_id), 0);
    lv_obj_set_style_text_color(phandle->time_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_label_set_text(phandle->time_lbl, "0");

    unit = lv_label_create(db_row);
    lv_label_set_text(unit, "dB");
    lv_obj_set_style_text_color(unit, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(unit, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
    lv_obj_set_style_pad_bottom(unit, 12, 0);

    stage = lv_obj_create(meter);
    lv_obj_set_style_bg_opa(stage, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(stage, 0, 0);
    lv_obj_set_style_pad_all(stage, 0, 0);
    lv_obj_set_style_pad_column(stage, DIVOOM_ASTRO_TOOL_NOISE_BAR_GAP, 0);
    lv_obj_set_style_shadow_width(stage, 0, 0);
    lv_obj_clear_flag(stage, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(stage, 400, DIVOOM_ASTRO_TOOL_NOISE_STAGE_H);
    lv_obj_set_flex_flow(stage, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(stage, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for (i = 0; i < DIVOOM_ASTRO_TOOL_NOISE_BARS; i++)
    {
        lv_obj_t *slot;

        slot = lv_obj_create(stage);
        lv_obj_set_style_bg_opa(slot, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(slot, 0, 0);
        lv_obj_set_style_pad_all(slot, 0, 0);
        lv_obj_set_style_pad_row(slot, DIVOOM_ASTRO_TOOL_NOISE_MIRROR_GAP, 0);
        lv_obj_set_style_shadow_width(slot, 0, 0);
        lv_obj_clear_flag(slot, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(slot, DIVOOM_ASTRO_TOOL_NOISE_SLOT_W, DIVOOM_ASTRO_TOOL_NOISE_STAGE_H);
        lv_obj_set_flex_flow(slot, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(slot, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        phandle->noise_bars[i] = divoom_astrootoo_tools_noise_make_bar(slot, 0);
        phandle->noise_bar_reflect[i] = divoom_astrootoo_tools_noise_make_bar(slot, 1);
    }

    /* 底栏：顶 2px 线 + 5 档 + 列间竖线（真源 NoiseMeterReferenceScale） */
    scale_bar = lv_obj_create(phandle->root_obj);
    lv_obj_set_style_bg_color(scale_bar, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
    lv_obj_set_style_border_width(scale_bar, 0, 0);
    lv_obj_set_style_radius(scale_bar, 0, 0);
    lv_obj_set_style_pad_all(scale_bar, 0, 0);
    lv_obj_set_style_shadow_width(scale_bar, 0, 0);
    lv_obj_clear_flag(scale_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(scale_bar, DIVOOM_SCREEN_WIDTH, 2);
    lv_obj_set_pos(scale_bar, 0, DIVOOM_ASTRO_TOOL_NOISE_SCALE_Y);

    cell_w = DIVOOM_SCREEN_WIDTH / 5;
    for (i = 0; i < 5; i++)
    {
        lv_obj_t *cell;
        lv_obj_t *val;
        lv_obj_t *name;

        cell = lv_obj_create(phandle->root_obj);
        lv_obj_set_style_bg_opa(cell, LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(cell, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
        lv_obj_set_style_border_width(cell, 0, 0);
        lv_obj_set_style_radius(cell, 6, 0);
        lv_obj_set_style_pad_all(cell, 0, 0);
        lv_obj_set_style_pad_top(cell, 8, 0);
        lv_obj_set_style_shadow_width(cell, 0, 0);
        lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(cell, cell_w, DIVOOM_ASTRO_TOOL_NOISE_SCALE_H - 2);
        lv_obj_set_pos(cell, i * cell_w, DIVOOM_ASTRO_TOOL_NOISE_SCALE_Y + 2);
        if (i > 0)
        {
            lv_obj_t *div;

            div = lv_obj_create(cell);
            lv_obj_set_style_bg_color(div, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
            lv_obj_set_style_border_width(div, 0, 0);
            lv_obj_set_style_radius(div, 0, 0);
            lv_obj_set_style_pad_all(div, 0, 0);
            lv_obj_set_style_shadow_width(div, 0, 0);
            lv_obj_clear_flag(div, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_size(div, 2, DIVOOM_ASTRO_TOOL_NOISE_SCALE_H - 2);
            lv_obj_set_pos(div, 0, 0);
        }

        val = lv_label_create(cell);
        lv_snprintf(buf, sizeof(buf), "%ddB", g_noise_scale_db[i]);
        lv_label_set_text(val, buf);
        lv_obj_set_style_text_color(val, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
        lv_obj_set_style_text_font(val, divoom_menu_control_get_font_info(16, phandle->menu_id), 0);
        lv_obj_set_width(val, cell_w);
        lv_obj_set_style_text_align(val, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(val, LV_ALIGN_TOP_MID, 0, 2);

        name = lv_label_create(cell);
        lv_label_set_text(name, divoom_menu_control_get_disp_str(g_noise_scale_key[i]));
        lv_obj_set_style_text_color(name, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
        lv_obj_set_style_text_font(name, divoom_menu_control_get_font_info(16, phandle->menu_id), 0);
        lv_label_set_long_mode(name, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(name, cell_w - 8);
        lv_obj_set_style_text_align(name, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(name, LV_ALIGN_TOP_MID, 0, 28);

        phandle->noise_scale[i] = cell;
        phandle->noise_scale_val[i] = val;
        phandle->noise_scale_name[i] = name;
    }

    phandle->key_btn = divoom_astrootoo_ui_create_key_capture_btn(phandle->root_obj, &phandle->ui,
        divoom_astrootoo_tools_noise_key_cb, phandle);
    lv_obj_move_foreground(phandle->key_btn);
    lv_group_add_obj(lv_group_get_default(), phandle->key_btn);

    phandle->timer = lv_timer_create(divoom_astrootoo_tools_noise_timer_cb, 100, phandle);
    divoom_astrootoo_tools_noise_timer_cb(phandle->timer);
}

/* ====================================================================== */
/* 温馨留言 / 录音（Warm Memo）— 原工具页 UI，底层走 leave_msg_ctrl        */
/* ====================================================================== */
#define DIVOOM_ASTRO_MEMO_CMD_IDLE           0u
#define DIVOOM_ASTRO_MEMO_CMD_PENDING_START  1u
#define DIVOOM_ASTRO_MEMO_RECORD_MAX_SEC     60
#define MEMO_UI_DBG_TAG                      "MEMO_UI_DBG"

static void divoom_astrootoo_tools_memo_dbg(const char *tag, Pdivoom_astrootoo_tools_ctrl ph)
{
	divoom_print("%s: %s st=%d elapsed=%d clip=%d pending=%u mic=%d ctrl=%d\n",
		     MEMO_UI_DBG_TAG, tag, ph->memo_state, ph->memo_elapsed, ph->memo_clip,
		     (unsigned)ph->memo_pending,
		     divoom_local_music_ctrl_voice_clip_record_is_active(),
		     divoom_leave_msg_ctrl_get_status());
}

static int divoom_astrootoo_tools_memo_capture_live(void)
{
	return divoom_local_music_ctrl_voice_clip_record_is_active();
}

static int divoom_astrootoo_tools_memo_push_ctrl_msg(uint8 msg_id, uint8 lvgl_flag,
						     uint8 record_gen)
{
	divoom_sys_msg_info msg = {0};
	uint8 *flag_heap = NULL;

	msg.src_id = DIVOOM_MODEL_LEAVE_MSG;
	msg.dest_id = DIVOOM_MODEL_LEAVE_MSG;
	msg.msg_type = 1;
	msg.msg_id = msg_id;
	if (msg_id == DIVOOM_LEAVE_MSG_CTRL_EVENT_LOCAL_RECORD)
		msg.attr = record_gen;
	if (lvgl_flag) {
		flag_heap = (uint8 *)divoom_system_malloc(1);
		if (!flag_heap)
			return 0;
		*flag_heap = 1;
		msg.data = flag_heap;
		msg.data_len = 1;
	}
	/*
	 * msg_type=1 走异步入队，divoom_sys_model_crl_msg_push 在入队后会把
	 * pmodel_link 置 NULL 并返回 0xFF，与是否入队成功无关。
	 * voice_record_tool 同样不检查返回值；此处仅 malloc 失败视为 push 失败。
	 */
	divoom_sys_model_crl_msg_push(&msg);
	return 1;
}

static void divoom_astrootoo_tools_memo_sync_clip(Pdivoom_astrootoo_tools_ctrl phandle)
{
	if (divoom_leave_msg_record_temp_clip_bytes() >= (int)DIVOOM_LEAVE_MSG_MIN_CLIP_BYTES
	    && divoom_leave_msg_record_clip_path_is_playable(DIVOOM_LEAVE_MSG_RECORD_TMP_MP3)) {
		phandle->memo_clip = divoom_leave_msg_ctrl_get_duration_sec();
		if (phandle->memo_clip <= 0)
			phandle->memo_clip = phandle->memo_elapsed;
	} else if (divoom_voice_read_file_is_exit((uint32)DIVOOM_VOICE_ID_LEAVE_MSG)) {
		phandle->memo_clip = divoom_leave_msg_ctrl_get_duration_sec();
		if (phandle->memo_clip <= 0) {
			uint32 ds;
			int mo, dd;

			if (divoom_leave_msg_record_read_last_saved_meta(&ds, &mo, &dd))
				phandle->memo_clip = (int)ds;
		}
	} else {
		phandle->memo_clip = 0;
	}
}

static void divoom_astrootoo_tools_memo_rec_icon_pulse_cb(void *var, int32_t v)
{
	lv_img_set_zoom((lv_obj_t *)var, (uint16_t)v);
}

static void divoom_astrootoo_tools_memo_rec_icon_pulse_stop(lv_obj_t *icon)
{
	if (!icon)
		return;
	lv_anim_del(icon, divoom_astrootoo_tools_memo_rec_icon_pulse_cb);
	lv_img_set_zoom(icon, (uint16_t)DIVOOM_ASTRO_MEMO_REC_ZOOM_BASE);
}

static void divoom_astrootoo_tools_memo_rec_icon_pulse_sync(lv_obj_t *icon, int live)
{
	lv_anim_t a;

	if (!icon)
		return;
	if (!live) {
		divoom_astrootoo_tools_memo_rec_icon_pulse_stop(icon);
		return;
	}
	if (lv_anim_get(icon, divoom_astrootoo_tools_memo_rec_icon_pulse_cb) != NULL)
		return;
	lv_img_set_zoom(icon, (uint16_t)DIVOOM_ASTRO_MEMO_REC_ZOOM_BASE);
	lv_anim_init(&a);
	lv_anim_set_var(&a, icon);
	lv_anim_set_exec_cb(&a, divoom_astrootoo_tools_memo_rec_icon_pulse_cb);
	lv_anim_set_values(&a, DIVOOM_ASTRO_MEMO_REC_ZOOM_BASE, DIVOOM_ASTRO_MEMO_REC_ZOOM_PULSE);
	lv_anim_set_time(&a, DIVOOM_ASTRO_MEMO_REC_PULSE_MS);
	lv_anim_set_playback_time(&a, DIVOOM_ASTRO_MEMO_REC_PULSE_MS);
	lv_anim_set_path_cb(&a, lv_anim_path_step);
	lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
	lv_anim_start(&a);
}

static void divoom_astrootoo_tools_memo_set_icon(Pdivoom_astrootoo_tools_ctrl phandle, const char *icon_file)
{
	if (!phandle || !phandle->memo_btn_icon || !icon_file)
		return;
	divoom_menu_control_lv_img_set_src(phandle->memo_btn_icon, icon_file);
	lv_obj_center(phandle->memo_btn_icon);
	if (strstr(icon_file, "sleep_rec_btn") != NULL)
		lv_img_set_zoom(phandle->memo_btn_icon, (uint16_t)DIVOOM_ASTRO_MEMO_REC_ZOOM_BASE);
	else
		lv_img_set_zoom(phandle->memo_btn_icon, LV_IMG_ZOOM_NONE);
}

static void divoom_astrootoo_tools_memo_update_meter(Pdivoom_astrootoo_tools_ctrl phandle, int active)
{
	int i;
	uint32_t s;

	if (!phandle || !phandle->memo_meter_row)
		return;

	if (!active) {
		lv_obj_add_flag(phandle->memo_meter_row, LV_OBJ_FLAG_HIDDEN);
		return;
	}
	lv_obj_clear_flag(phandle->memo_meter_row, LV_OBJ_FLAG_HIDDEN);

	/* 真源 formatMeterBars：伪随机高度，非真实麦克风电平 */
	phandle->memo_meter_seed = phandle->memo_meter_seed * 1664525u + 1013904223u;
	s = phandle->memo_meter_seed | 1u;
	for (i = 0; i < DIVOOM_ASTRO_MEMO_METER_BARS; i++) {
		lv_obj_t *bar;
		int h;

		s = s * 1664525u + 1013904223u;
		h = (int)((s % 12u) + 3u) * 2;
		if (h > DIVOOM_ASTRO_MEMO_METER_H)
			h = DIVOOM_ASTRO_MEMO_METER_H;
		bar = phandle->memo_meter[i];
		if (!bar)
			continue;
		lv_obj_set_height(bar, h);
		lv_obj_set_style_bg_color(bar,
			h > 8 ? lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS) : lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
		lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
	}
}

static void divoom_astrootoo_tools_memo_refresh(Pdivoom_astrootoo_tools_ctrl phandle)
{
	char buf[8];
	int shown;
	int live;

	if (!phandle || !phandle->time_lbl || !phandle->status_lbl)
		return;

	shown = (phandle->memo_state == 1) ? phandle->memo_elapsed :
		((phandle->memo_state == 2) ? phandle->memo_elapsed : phandle->memo_clip);
	divoom_astrootoo_tools_fmt_ms(buf, sizeof(buf), shown);
	lv_label_set_text(phandle->time_lbl, buf);

	live = (phandle->memo_state == 1 || phandle->memo_state == 2);
	if (phandle->memo_readout) {
		lv_obj_set_style_border_color(phandle->memo_readout,
			live ? lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS) : lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
	}

	if (phandle->memo_state == 1) {
		lv_obj_set_style_text_color(phandle->time_lbl, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
		lv_label_set_text(phandle->status_lbl,
			divoom_menu_control_get_disp_str("VoiceRecordPressToEnd"));
		/* 勿每 200ms set_icon：会重置 zoom 并打断脉冲动画 */
		divoom_astrootoo_tools_memo_rec_icon_pulse_sync(phandle->memo_btn_icon, 1);
		if (phandle->memo_btn)
			lv_obj_set_style_opa(phandle->memo_btn, LV_OPA_COVER, 0);
	} else if (phandle->memo_state == 2) {
		divoom_astrootoo_tools_memo_rec_icon_pulse_stop(phandle->memo_btn_icon);
		lv_obj_set_style_text_color(phandle->time_lbl, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
		lv_label_set_text(phandle->status_lbl,
			divoom_menu_control_get_disp_str("VoiceRecordPlaying"));
		divoom_astrootoo_tools_memo_set_icon(phandle, DIVOOM_ASTROTOO_TOOLS_ICON_PAUSE);
		/* 真源 primaryActionLocked：播放中整体约 38% 透明（交互仍按真机可停） */
		if (phandle->memo_btn)
			lv_obj_set_style_opa(phandle->memo_btn, LV_OPA_40, 0);
	} else {
		divoom_astrootoo_tools_memo_rec_icon_pulse_stop(phandle->memo_btn_icon);
		lv_obj_set_style_text_color(phandle->time_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
		if (phandle->memo_btn)
			lv_obj_set_style_opa(phandle->memo_btn, LV_OPA_COVER, 0);
		if (phandle->memo_clip > 0) {
			lv_label_set_text(phandle->status_lbl,
				divoom_menu_control_get_disp_str("VoiceRecordNewMsgNotice"));
			divoom_astrootoo_tools_memo_set_icon(phandle, DIVOOM_ASTROTOO_TOOLS_ICON_PLAY);
		} else {
			lv_label_set_text(phandle->status_lbl,
				divoom_menu_control_get_disp_str("VoiceRecordPressToStart"));
			divoom_astrootoo_tools_memo_set_icon(phandle, DIVOOM_ASTRO_MEMO_ICON_READY);
		}
	}
	divoom_astrootoo_tools_memo_update_meter(phandle, live);
	if (phandle->root_obj)
		lv_obj_invalidate(phandle->root_obj);
}

static void divoom_astrootoo_tools_memo_finish_record(Pdivoom_astrootoo_tools_ctrl phandle, int commit)
{
	uint32 dur;
	uint32 now;

	/* 先停编码器拿到 0x64 秒，再落盘；与闹钟同一口径 */
	divoom_leave_msg_ctrl_stop_local_record_sync();
	dur = (uint32)divoom_leave_msg_ctrl_get_duration_sec();
	if (dur == 0u && phandle->memo_start_tick_ms != 0u) {
		now = divoom_system_get_tickms();
		dur = (now - phandle->memo_start_tick_ms) / 1000u;
	}
	if (dur == 0u && divoom_leave_msg_record_temp_clip_bytes() > 0)
		dur = 1u;
	if (dur > (uint32)DIVOOM_ASTRO_MEMO_RECORD_MAX_SEC)
		dur = (uint32)DIVOOM_ASTRO_MEMO_RECORD_MAX_SEC;

	phandle->memo_pending = DIVOOM_ASTRO_MEMO_CMD_IDLE;
	phandle->memo_start_wait = 0;
	phandle->memo_start_tick_ms = 0;
	divoom_astrootoo_tools_memo_push_ctrl_msg(DIVOOM_LEAVE_MSG_CTRL_EVENT_STOP_RECORD, 0, 0);
	/* 工具页内不弹顶栏通知：落盘后留在待播放态，退出留言页再 notice */
	if (commit)
		(void)divoom_leave_msg_record_commit_to_leave_msg_file(dur);

	phandle->memo_state = 0;
	phandle->memo_elapsed = 0;
	phandle->memo_clip = dur > 0u ? (int)dur : 0;
	divoom_astrootoo_tools_memo_sync_clip(phandle);
	if (phandle->memo_clip <= 0 && dur > 0u)
		phandle->memo_clip = (int)dur;
	divoom_astrootoo_tools_memo_refresh(phandle);
}

static void divoom_astrootoo_tools_memo_start_play(Pdivoom_astrootoo_tools_ctrl phandle)
{
	char path[256];

	if (!divoom_leave_msg_record_get_playback_path(path, sizeof(path)))
		return;
	phandle->memo_elapsed = 0;
	phandle->memo_play_start_tick_ms = 0;
	phandle->memo_play_armed = 0;
	phandle->memo_state = 2;
	/* play_time=0：留言不 loop，只播一次 */
	divoom_local_music_ctrl_tone_play(path, divoom_leave_msg_record_get_play_volume(), NULL, 0);
	divoom_astrootoo_tools_memo_refresh(phandle);
}

static void divoom_astrootoo_tools_memo_stop_play(Pdivoom_astrootoo_tools_ctrl phandle)
{
	divoom_leave_msg_record_stop_preview_playback();
	phandle->memo_state = 0;
	phandle->memo_play_armed = 0;
	divoom_astrootoo_tools_memo_refresh(phandle);
}

static void divoom_astrootoo_tools_memo_finish_play(Pdivoom_astrootoo_tools_ctrl phandle)
{
	divoom_leave_msg_record_stop_preview_playback();
	divoom_leave_msg_record_delete_temp_file();
	divoom_leave_msg_record_delete_saved_on_disk();
	phandle->memo_state = 0;
	phandle->memo_elapsed = 0;
	phandle->memo_clip = 0;
	phandle->memo_play_armed = 0;
	divoom_astrootoo_tools_memo_refresh(phandle);
}

static void divoom_astrootoo_tools_memo_discard_record(Pdivoom_astrootoo_tools_ctrl phandle)
{
	divoom_leave_msg_ctrl_abort_local_record();
	divoom_leave_msg_record_delete_temp_file();
	phandle->memo_pending = DIVOOM_ASTRO_MEMO_CMD_IDLE;
	phandle->memo_start_wait = 0;
	phandle->memo_state = 0;
	phandle->memo_elapsed = 0;
	divoom_astrootoo_tools_memo_sync_clip(phandle);
	divoom_astrootoo_tools_memo_refresh(phandle);
}

/* 磁盘上仍有可播留言（顶栏 toast 与 ESC 回表盘共用） */
static int divoom_astrootoo_tools_memo_has_unread(Pdivoom_astrootoo_tools_ctrl phandle)
{
	char path[256];

	if (!phandle)
		return 0;
	divoom_astrootoo_tools_memo_sync_clip(phandle);
	if (phandle->memo_clip <= 0)
		return 0;
	if (!divoom_leave_msg_record_get_playback_path(path, sizeof(path)))
		return 0;
	return 1;
}

/* 菜单销毁时（tools_clean）再弹：页内录完不弹；勿在 ESC 路径弹，否则与 clean 双弹 */
static void divoom_astrootoo_tools_memo_notice_if_unread(Pdivoom_astrootoo_tools_ctrl phandle)
{
	uint32 dur;

	if (!divoom_astrootoo_tools_memo_has_unread(phandle))
		return;
	dur = (uint32)phandle->memo_clip;
	if (dur == 0u)
		dur = 1u;
	divoom_lvgl_top_leave_msg_notice_init(dur);
}

/* 停录/停播等资源收尾；顶栏通知只在 tools_clean 里调 notice */
static void divoom_astrootoo_tools_memo_exit_cleanup(Pdivoom_astrootoo_tools_ctrl phandle)
{
	divoom_leave_msg_record_stop_preview_playback();
	if (phandle->memo_state == 1
	    || phandle->memo_pending == DIVOOM_ASTRO_MEMO_CMD_PENDING_START
	    || divoom_leave_msg_ctrl_get_status() == DIVOOM_LEAVE_MSG_START_RECORD) {
		divoom_leave_msg_ctrl_abort_local_record();
		divoom_leave_msg_record_delete_temp_file();
	}
	divoom_astrootoo_tools_memo_push_ctrl_msg(DIVOOM_LEAVE_MSG_CTRL_EVENT_LVGL_EXIT, 0, 0);
	phandle->memo_pending = DIVOOM_ASTRO_MEMO_CMD_IDLE;
	phandle->memo_start_wait = 0;
	phandle->memo_state = 0;
}

static void divoom_astrootoo_tools_memo_timer_cb(lv_timer_t *t)
{
	Pdivoom_astrootoo_tools_ctrl phandle;
	int live;
	int sec_tick;
	uint32 now;

	phandle = (Pdivoom_astrootoo_tools_ctrl)lv_timer_get_user_data(t);
	if (!phandle)
		return;

	phandle->memo_tick++;
	sec_tick = ((phandle->memo_tick % DIVOOM_ASTRO_MEMO_TICKS_PER_SEC) == 0);
	now = divoom_system_get_tickms();

	/* 异步开录超时：约 5s（200ms × 25） */
	if (phandle->memo_pending == DIVOOM_ASTRO_MEMO_CMD_PENDING_START) {
		live = divoom_astrootoo_tools_memo_capture_live();
		if (live) {
			phandle->memo_pending = DIVOOM_ASTRO_MEMO_CMD_IDLE;
			phandle->memo_start_wait = 0;
			if (phandle->memo_start_tick_ms == 0u)
				phandle->memo_start_tick_ms = now;
		} else if (sec_tick) {
			phandle->memo_start_wait++;
			if (phandle->memo_start_wait >= 5) {
				phandle->memo_pending = DIVOOM_ASTRO_MEMO_CMD_IDLE;
				phandle->memo_start_wait = 0;
				phandle->memo_state = 0;
				phandle->memo_elapsed = 0;
				phandle->memo_start_tick_ms = 0;
				divoom_leave_msg_ctrl_abort_local_record();
				divoom_astrootoo_tools_memo_sync_clip(phandle);
				divoom_astrootoo_tools_memo_refresh(phandle);
				return;
			}
		}
	}

	if (phandle->memo_state == 1) {
		live = divoom_astrootoo_tools_memo_capture_live();
		if (live) {
			if (phandle->memo_start_tick_ms == 0u)
				phandle->memo_start_tick_ms = now;
			phandle->memo_elapsed = (int)((now - phandle->memo_start_tick_ms) / 1000u);
			if (phandle->memo_elapsed >= DIVOOM_ASTRO_MEMO_RECORD_MAX_SEC) {
				divoom_astrootoo_tools_memo_dbg("max_sec", phandle);
				divoom_astrootoo_tools_memo_finish_record(phandle, 1);
				return;
			}
		} else if (phandle->memo_pending != DIVOOM_ASTRO_MEMO_CMD_PENDING_START
			   && phandle->memo_start_tick_ms != 0u) {
			/*
			 * JL 侧 60s 封顶会自行停编，live 先变 0；整数秒在 59.xxx
			 * 时仍显示 59，且再也进不了「elapsed>=60 且 live」分支。
			 * 闹钟/助眠录音页用 was_busy→commit，这里同样跟编码器结束。
			 */
			divoom_astrootoo_tools_memo_dbg("enc_end", phandle);
			divoom_astrootoo_tools_memo_finish_record(phandle, 1);
			return;
		}
		divoom_astrootoo_tools_memo_refresh(phandle);
		return;
	}

	if (phandle->memo_state == 2) {
		if (!phandle->memo_play_armed) {
			if (divoom_local_music_ctrl_get_tone_status()) {
				phandle->memo_play_armed = 1;
				phandle->memo_play_start_tick_ms = now;
			}
			divoom_astrootoo_tools_memo_update_meter(phandle, 1);
			return;
		}
		if (!divoom_local_music_ctrl_get_tone_status()) {
			divoom_astrootoo_tools_memo_finish_play(phandle);
			return;
		}
		if (phandle->memo_play_start_tick_ms != 0u)
			phandle->memo_elapsed = (int)((now - phandle->memo_play_start_tick_ms) / 1000u);
		if (phandle->memo_clip > 0 && phandle->memo_elapsed > phandle->memo_clip)
			phandle->memo_elapsed = phandle->memo_clip;
		divoom_astrootoo_tools_memo_refresh(phandle);
	}
}

static void divoom_astrootoo_tools_memo_do_enter(Pdivoom_astrootoo_tools_ctrl phandle)
{
    uint8 record_gen;

    if (phandle->memo_state == 1) {
        divoom_astrootoo_tools_memo_finish_record(phandle, 1);
        return;
    }
    if (phandle->memo_state == 2) {
        divoom_astrootoo_tools_memo_stop_play(phandle);
        return;
    }
    if (phandle->memo_clip > 0) {
        divoom_astrootoo_tools_memo_start_play(phandle);
        return;
    }
    if (phandle->memo_pending != DIVOOM_ASTRO_MEMO_CMD_IDLE)
        return;

    divoom_leave_msg_ctrl_abort_local_record();
    divoom_leave_msg_record_delete_temp_file();
    phandle->memo_clip = 0;
    phandle->memo_state = 1;
    phandle->memo_elapsed = 0;
    phandle->memo_start_tick_ms = 0;
    phandle->memo_start_wait = 0;
    phandle->memo_pending = DIVOOM_ASTRO_MEMO_CMD_PENDING_START;
    record_gen = divoom_leave_msg_ctrl_alloc_record_gen();
    if (!divoom_astrootoo_tools_memo_push_ctrl_msg(
            DIVOOM_LEAVE_MSG_CTRL_EVENT_LOCAL_RECORD, 1, record_gen)) {
        phandle->memo_pending = DIVOOM_ASTRO_MEMO_CMD_IDLE;
        phandle->memo_state = 0;
        phandle->memo_elapsed = 0;
        divoom_astrootoo_tools_memo_dbg("start_fail", phandle);
    } else {
        if (phandle->timer)
            lv_timer_ready(phandle->timer);
        divoom_astrootoo_tools_memo_set_icon(phandle, DIVOOM_ASTRO_MEMO_ICON_LIVE);
        divoom_astrootoo_tools_memo_rec_icon_pulse_sync(phandle->memo_btn_icon, 1);
        divoom_astrootoo_tools_memo_dbg("start_ok", phandle);
    }
    divoom_astrootoo_tools_memo_refresh(phandle);
}

static void divoom_astrootoo_tools_memo_do_esc(Pdivoom_astrootoo_tools_ctrl phandle)
{
    if (phandle->memo_state == 1
        || phandle->memo_pending == DIVOOM_ASTRO_MEMO_CMD_PENDING_START) {
        divoom_astrootoo_tools_memo_discard_record(phandle);
    } else if (phandle->memo_state == 2) {
        divoom_astrootoo_tools_memo_stop_play(phandle);
    } else {
        int unread = divoom_astrootoo_tools_memo_has_unread(phandle);

        /* 有未读先回表盘（clearall 时尚无条）；条仍在 105 clean 里弹，此时 pinit 已结束。 */
        divoom_astrootoo_tools_memo_exit_cleanup(phandle);
        if (unread)
            divoom_main_return_startup_dial();
        else
            divoom_menu_control_return_pre();
    }
}

static void divoom_astrootoo_tools_memo_clicked_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED)
        return;
    divoom_astrootoo_tools_memo_do_enter(
        (Pdivoom_astrootoo_tools_ctrl)lv_event_get_user_data(e));
}

static void divoom_astrootoo_tools_memo_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_tools_ctrl phandle;
    uint8_t key_code;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }

    phandle = (Pdivoom_astrootoo_tools_ctrl)lv_event_get_user_data(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_astrootoo_tools_memo_do_esc(phandle);
        return;
    }

    if (divoom_astrootoo_ui_key_is_enter(key_code))
        divoom_astrootoo_tools_memo_do_enter(phandle);
}

static void divoom_astrootoo_tools_build_memo(Pdivoom_astrootoo_tools_ctrl phandle)
{
	lv_obj_t *tape;
	lv_obj_t *readout;
	lv_obj_t *meter_row;
	lv_obj_t *btn;
	lv_obj_t *icon;
	lv_coord_t btn_x;
	lv_coord_t bar_w;
	lv_coord_t gap;
	int i;

	phandle->memo_state = 0;
	phandle->memo_elapsed = 0;
	phandle->memo_clip = 0;
	phandle->memo_tick = 0;
	phandle->memo_start_tick_ms = 0;
	phandle->memo_play_start_tick_ms = 0;
	phandle->memo_play_armed = 0;
	phandle->memo_pending = DIVOOM_ASTRO_MEMO_CMD_IDLE;
	phandle->memo_start_wait = 0;
	phandle->memo_meter_seed = 1u;
	phandle->memo_tape = NULL;
	phandle->memo_readout = NULL;
	phandle->memo_meter_row = NULL;
	phandle->memo_btn_lbl = NULL;
	phandle->memo_btn_icon = NULL;
	for (i = 0; i < DIVOOM_ASTRO_MEMO_METER_BARS; i++)
		phandle->memo_meter[i] = NULL;
	divoom_astrootoo_tools_memo_sync_clip(phandle);

	divoom_astrootoo_tools_create_header(phandle, "VoiceRecordTool");

	/* 外层磁带卡（结构对齐真源 tapeModule，材质简化：纯色+描边） */
	tape = lv_obj_create(phandle->root_obj);
	phandle->memo_tape = tape;
	lv_obj_clear_flag(tape, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(tape, lv_color_hex(DIVOOM_ASTRO_SURFACE_IDLE), 0);
	lv_obj_set_style_bg_opa(tape, LV_OPA_COVER, 0);
	lv_obj_set_style_border_color(tape, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
	lv_obj_set_style_border_width(tape, 2, 0);
	lv_obj_set_style_radius(tape, DIVOOM_ASTRO_RADIUS_MD, 0);
	lv_obj_set_style_pad_all(tape, 12, 0);
	lv_obj_set_style_shadow_width(tape, 0, 0);
	lv_obj_set_size(tape, DIVOOM_ASTRO_TOOL_BAR_W, DIVOOM_ASTRO_MEMO_TAPE_H);
	lv_obj_set_pos(tape, DIVOOM_ASTRO_LIST_PAD_X, DIVOOM_ASTRO_MEMO_TAPE_Y);

	/* 内层读数区 */
	readout = lv_obj_create(tape);
	phandle->memo_readout = readout;
	lv_obj_clear_flag(readout, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(readout, lv_color_hex(DIVOOM_ASTRO_BG), 0);
	lv_obj_set_style_bg_opa(readout, LV_OPA_COVER, 0);
	lv_obj_set_style_border_color(readout, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
	lv_obj_set_style_border_width(readout, 2, 0);
	lv_obj_set_style_radius(readout, DIVOOM_ASTRO_RADIUS_SM, 0);
	lv_obj_set_style_pad_all(readout, 12, 0);
	lv_obj_set_style_shadow_width(readout, 0, 0);
	lv_obj_set_size(readout, DIVOOM_ASTRO_TOOL_BAR_W - 24, DIVOOM_ASTRO_MEMO_TAPE_H - 24);
	lv_obj_center(readout);

	phandle->time_lbl = lv_label_create(readout);
	lv_obj_set_style_text_font(phandle->time_lbl,
		divoom_menu_control_get_font_info(72, phandle->menu_id), 0);
	lv_obj_set_style_text_color(phandle->time_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
	lv_obj_set_style_text_align(phandle->time_lbl, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_text(phandle->time_lbl, "00:00");
	lv_obj_align(phandle->time_lbl, LV_ALIGN_TOP_MID, 0, 36);

	phandle->status_lbl = lv_label_create(readout);
	lv_obj_set_style_text_color(phandle->status_lbl, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
	lv_obj_set_style_text_font(phandle->status_lbl,
		divoom_menu_control_get_font_info(22, phandle->menu_id), 0);
	lv_obj_set_width(phandle->status_lbl, DIVOOM_ASTRO_TOOL_BAR_W - 64);
	lv_obj_set_style_text_align(phandle->status_lbl, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_long_mode(phandle->status_lbl, LV_LABEL_LONG_DOT);
	lv_obj_align(phandle->status_lbl, LV_ALIGN_TOP_MID, 0, 130);

	/* 假电平条 */
	meter_row = lv_obj_create(readout);
	phandle->memo_meter_row = meter_row;
	lv_obj_clear_flag(meter_row, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_opa(meter_row, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(meter_row, 0, 0);
	lv_obj_set_style_pad_all(meter_row, 0, 0);
	lv_obj_set_style_pad_column(meter_row, 2, 0);
	lv_obj_set_style_shadow_width(meter_row, 0, 0);
	lv_obj_set_size(meter_row, DIVOOM_ASTRO_TOOL_BAR_W - 80, DIVOOM_ASTRO_MEMO_METER_H);
	lv_obj_set_flex_flow(meter_row, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(meter_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
	lv_obj_align(meter_row, LV_ALIGN_BOTTOM_MID, 0, -8);
	lv_obj_add_flag(meter_row, LV_OBJ_FLAG_HIDDEN);

	gap = 2;
	bar_w = ((DIVOOM_ASTRO_TOOL_BAR_W - 80) - gap * (DIVOOM_ASTRO_MEMO_METER_BARS - 1))
		/ DIVOOM_ASTRO_MEMO_METER_BARS;
	if (bar_w < 2)
		bar_w = 2;
	for (i = 0; i < DIVOOM_ASTRO_MEMO_METER_BARS; i++) {
		lv_obj_t *bar;

		bar = lv_obj_create(meter_row);
		phandle->memo_meter[i] = bar;
		lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_set_style_bg_color(bar, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
		lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
		lv_obj_set_style_border_width(bar, 0, 0);
		lv_obj_set_style_radius(bar, 1, 0);
		lv_obj_set_style_pad_all(bar, 0, 0);
		lv_obj_set_style_shadow_width(bar, 0, 0);
		lv_obj_set_size(bar, bar_w, 4);
	}

	/* 琥珀主操作钮 + 图标（真源 primaryAction 208×72） */
	btn_x = (DIVOOM_SCREEN_WIDTH - DIVOOM_ASTRO_MEMO_BTN_W) / 2;
	btn = lv_btn_create(phandle->root_obj);
	phandle->memo_btn = btn;
	lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(btn, DIVOOM_ASTRO_MEMO_BTN_W, DIVOOM_ASTRO_TOOL_BTN_H);
	lv_obj_set_pos(btn, btn_x, DIVOOM_ASTRO_TOOL_BAR_Y);
	lv_obj_set_style_bg_color(btn, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
	lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
	lv_obj_set_style_border_color(btn, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
	lv_obj_set_style_border_width(btn, 2, 0);
	lv_obj_set_style_radius(btn, DIVOOM_ASTRO_RADIUS_SM, 0);
	lv_obj_set_style_shadow_width(btn, 0, 0);
	lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
	lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_border_color(btn, lv_color_hex(DIVOOM_ASTRO_FOCUS_HI), LV_STATE_FOCUSED);
	lv_obj_set_style_border_width(btn, 3, LV_STATE_FOCUSED);
	lv_obj_set_style_border_color(btn, lv_color_hex(DIVOOM_ASTRO_FOCUS_HI), LV_STATE_FOCUS_KEY);
	lv_obj_set_style_border_width(btn, 3, LV_STATE_FOCUS_KEY);

	icon = lv_img_create(btn);
	phandle->memo_btn_icon = icon;
	divoom_menu_control_lv_img_set_src(icon, DIVOOM_ASTRO_MEMO_ICON_READY);
	lv_img_set_zoom(icon, (uint16_t)DIVOOM_ASTRO_MEMO_REC_ZOOM_BASE);
	lv_obj_center(icon);

	lv_obj_add_event_cb(btn, divoom_astrootoo_tools_memo_key_cb, LV_EVENT_KEY, phandle);
	lv_obj_add_event_cb(btn, divoom_astrootoo_tools_memo_clicked_cb, LV_EVENT_CLICKED, phandle);
	lv_group_add_obj(lv_group_get_default(), btn);

	phandle->key_btn = divoom_astrootoo_ui_create_key_capture_btn(phandle->root_obj, &phandle->ui,
		divoom_astrootoo_tools_memo_key_cb, phandle);
	lv_obj_clear_flag(phandle->key_btn, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_move_background(phandle->key_btn);
	lv_group_add_obj(lv_group_get_default(), phandle->key_btn);

	divoom_astrootoo_tools_memo_refresh(phandle);
	lv_group_focus_obj(phandle->memo_btn);
	phandle->timer = lv_timer_create(divoom_astrootoo_tools_memo_timer_cb,
		DIVOOM_ASTRO_MEMO_TIMER_MS, phandle);
}

/* ====================================================================== */
/* 生命周期                                                                */
/* ====================================================================== */
void *divoom_astrootoo_tools_init(void *user_data, int menu_id)
{
    Pdivoom_astrootoo_tools_ctrl phandle;

    (void)user_data;

    phandle = divoom_system_malloc_out(sizeof(divoom_astrootoo_tools_ctrl));
    if (!phandle)
    {
        return NULL;
    }

    phandle->menu_id = menu_id;
    phandle->page = divoom_astrootoo_tools_page_from_menu_id(menu_id);
    phandle->root_obj = divoom_astrootoo_ui_create_root(&phandle->ui);

    switch (phandle->page)
    {
    case DIVOOM_ASTROTOO_TOOLS_PAGE_POMODORO:
    {
        divoom_astrootoo_tools_build_pomodoro(phandle);
        break;
    }
    case DIVOOM_ASTROTOO_TOOLS_PAGE_STOPWATCH:
    {
        divoom_astrootoo_tools_build_stopwatch(phandle);
        break;
    }
    case DIVOOM_ASTROTOO_TOOLS_PAGE_COUNTDOWN:
    {
        divoom_astrootoo_tools_build_countdown(phandle);
        break;
    }
    case DIVOOM_ASTROTOO_TOOLS_PAGE_SCOREBOARD:
    {
        divoom_astrootoo_tools_build_scoreboard(phandle);
        break;
    }
    case DIVOOM_ASTROTOO_TOOLS_PAGE_MEMO:
    {
        divoom_astrootoo_tools_build_memo(phandle);
        break;
    }
    case DIVOOM_ASTROTOO_TOOLS_PAGE_NOISE:
    {
        divoom_astrootoo_tools_build_noise(phandle);
        break;
    }
    default:
    {
        divoom_astrootoo_tools_build_list(phandle);
        break;
    }
    }

    return phandle;
}

void divoom_astrootoo_tools_disp(void *handle, void *active_obj)
{
    Pdivoom_astrootoo_tools_ctrl phandle;
    lv_obj_t *focus_target;

    phandle = (Pdivoom_astrootoo_tools_ctrl)handle;
    if (!phandle)
    {
        return;
    }

    lv_scr_load(phandle->root_obj);
    focus_target = (lv_obj_t *)active_obj;

    if (phandle->page == DIVOOM_ASTROTOO_TOOLS_PAGE_LIST)
    {
        /* menu_control：首次 load 传 active_obj=NULL；return_pre 恢复栈页传 cur_active_obj */
        divoom_astrootoo_tools_group_bind_list(phandle, focus_target);
        return;
    }

    if (phandle->page == DIVOOM_ASTROTOO_TOOLS_PAGE_POMODORO)
    {
        Pdivoom_tomato_time_info pomo_info;

        /* 本地改过的设置先上报，再把 queue 里的最新配置回灌进运行副本 */
        pomo_info = divoom_astrootoo_tools_pomo_get_backend_info();
        if (pomo_info && divoom_tomato_time_menu_update(pomo_info->id))
        {
            divoom_tomato_time_change_tomato_info(pomo_info->id);
        }
        divoom_astrootoo_tools_pomo_sync_from_backend(phandle, 1);
        divoom_astrootoo_tools_pomo_bind_group(phandle, focus_target);
        return;
    }
    if (phandle->page == DIVOOM_ASTROTOO_TOOLS_PAGE_NOISE && phandle->key_btn)
    {
        lv_obj_t *objs[1];

        objs[0] = phandle->key_btn;
        divoom_astrootoo_tools_group_bind_objs(objs, 1, phandle->key_btn);
        return;
    }
    if (phandle->page == DIVOOM_ASTROTOO_TOOLS_PAGE_SCOREBOARD)
    {
        lv_obj_t *objs[2];
        lv_obj_t *focus;

        objs[0] = phandle->score_a_card;
        objs[1] = phandle->score_b_card;
        if (focus_target && lv_obj_is_valid(focus_target))
        {
            focus = focus_target;
        }
        else if (phandle->score_sel == 1)
        {
            focus = phandle->score_b_card;
        }
        else
        {
            focus = phandle->score_a_card;
        }
        divoom_astrootoo_tools_group_bind_objs(objs, 2, focus);
        phandle->score_sel = (focus == phandle->score_b_card) ? 1 : 0;
        divoom_astrootoo_tools_score_refresh(phandle);
        return;
    }
    if (phandle->page == DIVOOM_ASTROTOO_TOOLS_PAGE_MEMO && phandle->memo_btn)
    {
        lv_obj_t *objs[1];

        objs[0] = phandle->memo_btn;
        divoom_astrootoo_tools_group_bind_objs(objs, 1, phandle->memo_btn);
        return;
    }
    if (phandle->page == DIVOOM_ASTROTOO_TOOLS_PAGE_COUNTDOWN && phandle->btn_min)
    {
        lv_obj_t *objs[4];
        int idx;

        objs[0] = phandle->btn_min;
        objs[1] = phandle->btn_sec;
        objs[2] = phandle->btn_left;
        objs[3] = phandle->btn_right;
        divoom_astrootoo_tools_group_bind_objs(objs, 4, NULL);
        idx = phandle->focus_idx;
        if (focus_target == phandle->btn_min)
        {
            idx = 0;
        }
        else if (focus_target == phandle->btn_sec)
        {
            idx = 1;
        }
        else if (focus_target == phandle->btn_left)
        {
            idx = 2;
        }
        else if (focus_target == phandle->btn_right)
        {
            idx = 3;
        }
        divoom_astrootoo_tools_countdown_focus(phandle, idx);
        return;
    }
    if (phandle->page == DIVOOM_ASTROTOO_TOOLS_PAGE_STOPWATCH && phandle->btn_left && phandle->btn_right)
    {
        lv_obj_t *objs[2];
        lv_obj_t *focus;

        objs[0] = phandle->btn_left;
        objs[1] = phandle->btn_right;
        if (focus_target && lv_obj_is_valid(focus_target))
        {
            focus = focus_target;
        }
        else
        {
            focus = (phandle->focus_idx == 0) ? phandle->btn_left : phandle->btn_right;
        }
        divoom_astrootoo_tools_group_bind_objs(objs, 2, focus);
        phandle->focus_idx = (focus == phandle->btn_left) ? 0 : 1;
        return;
    }
    if (phandle->btn_right)
    {
        lv_group_focus_obj(phandle->btn_right);
    }
    else if (phandle->content && lv_obj_get_child_cnt(phandle->content) > 0)
    {
        lv_group_focus_obj(lv_obj_get_child(phandle->content, 0));
    }
}

void divoom_astrootoo_tools_clean(void *handle)
{
    Pdivoom_astrootoo_tools_ctrl phandle;

    phandle = (Pdivoom_astrootoo_tools_ctrl)handle;
    if (!phandle)
    {
        return;
    }

    if (phandle->menu_id == DIVOOM_MENU_CONTROL_ID_VOICE_RECORD_TOOL_MENU)
    {
        divoom_astrootoo_tools_memo_exit_cleanup(phandle);
        /* 真正离页只在此处 toast。clearall 时 105 是当前页会先拆，父页 72/1 后拆。 */
        divoom_astrootoo_tools_memo_notice_if_unread(phandle);
    }

    if (phandle->timer)
    {
        lv_timer_del(phandle->timer);
        phandle->timer = NULL;
    }
    if (phandle->direct_handle)
    {
        divoom_menu_direct_ctrl_destory(phandle->direct_handle);
        phandle->direct_handle = NULL;
    }
    if (phandle->pomo_set_direct)
    {
        divoom_menu_direct_ctrl_destory(phandle->pomo_set_direct);
        phandle->pomo_set_direct = NULL;
    }
    if (phandle->pomo_repeat_direct)
    {
        divoom_menu_direct_ctrl_destory(phandle->pomo_repeat_direct);
        phandle->pomo_repeat_direct = NULL;
    }
    if (phandle->pomo_ring_direct)
    {
        divoom_menu_direct_ctrl_destory(phandle->pomo_ring_direct);
        phandle->pomo_ring_direct = NULL;
    }
    divoom_astrootoo_tools_memo_rec_icon_pulse_stop(phandle->memo_btn_icon);

    /* 番茄退出确认弹窗为 root_obj 子对象，离页前先从 group 摘除并清指针（对齐 settings confirm） */
    if (phandle->page == DIVOOM_ASTROTOO_TOOLS_PAGE_POMODORO)
    {
        divoom_astrootoo_tools_pomo_notice_anim_close(phandle);
        /* 外壳若漏拆，表盘 init 会误用这个残留 handle，这里兜一次 */
        divoom_astrootoo_tools_pomo_notice_shell_close(phandle);
        divoom_astrootoo_tools_pomo_progress_motion_stop(phandle);
        if (phandle->pomo_exit_overlay)
        {
            divoom_astrootoo_tools_pomo_group_remove_obj_safe(phandle->pomo_exit_cancel_btn);
            divoom_astrootoo_tools_pomo_group_remove_obj_safe(phandle->pomo_exit_ok_btn);
        }
        divoom_astrootoo_tools_pomo_group_remove_obj_safe(phandle->btn_left);
        divoom_astrootoo_tools_pomo_group_remove_obj_safe(phandle->btn_right);
        divoom_astrootoo_tools_pomo_group_remove_obj_safe(phandle->pomo_set_row[DIVOOM_ASTRO_POMO_SET_RING_ROW]);
        phandle->pomo_exit_overlay = NULL;
        phandle->pomo_exit_cancel_btn = NULL;
        phandle->pomo_exit_ok_btn = NULL;
        phandle->pomo_exit_focus_restore = NULL;
    }

    if (phandle->root_obj)
    {
        lv_obj_del(phandle->root_obj);
        phandle->root_obj = NULL;
    }

    divoom_astrootoo_ui_base_reset_styles(&phandle->ui);
    divoom_system_free(phandle);
}

void divoom_astrootoo_tools_pause(void *handle)
{
    (void)handle;
}

static void divoom_astrootoo_tools_pomo_async_sync_cb(void *user_data)
{
    Pdivoom_astrootoo_tools_ctrl phandle;

    (void)user_data;
    if (divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_ID_POMODORO_MENU)
    {
        return;
    }

    phandle = (Pdivoom_astrootoo_tools_ctrl)divoom_menu_control_get_clock_mode_menu_handle();
    if (!phandle || phandle->page != DIVOOM_ASTROTOO_TOOLS_PAGE_POMODORO)
    {
        return;
    }

    divoom_astrootoo_tools_pomo_sync_from_backend(phandle, 1);
}

void divoom_astrootoo_tools_pomo_request_sync_active_page(void)
{
    if (divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_ID_POMODORO_MENU)
        return;

    lv_async_call(divoom_astrootoo_tools_pomo_async_sync_cb, NULL);
}

static void divoom_astrootoo_tools_pomo_async_notice_cb(void *user_data)
{
    Pdivoom_astrootoo_tools_ctrl phandle;

    (void)user_data;
    if (divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_ID_POMODORO_MENU)
    {
        return;
    }

    phandle = (Pdivoom_astrootoo_tools_ctrl)divoom_menu_control_get_clock_mode_menu_handle();
    if (!phandle || phandle->page != DIVOOM_ASTROTOO_TOOLS_PAGE_POMODORO)
    {
        return;
    }

    divoom_astrootoo_tools_pomo_notice_anim_play(phandle);
}

void divoom_astrootoo_tools_pomo_request_notice_anim(void)
{
    if (divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_ID_POMODORO_MENU)
        return;

    lv_async_call(divoom_astrootoo_tools_pomo_async_notice_cb, NULL);
}

/* ====================================================================== */
/* 白盒探针只读访问                                                        */
/* ====================================================================== */
static Pdivoom_astrootoo_tools_ctrl divoom_astrootoo_tools_whitebox_get(void)
{
    int mid;
    void *h;

    h = divoom_menu_control_get_clock_mode_menu_handle();
    if (!h)
    {
        return NULL;
    }
    mid = divoom_menu_control_get_active_menu_id();
    switch (mid)
    {
    case DIVOOM_MENU_CONTROL_ID_TOOL:
    case DIVOOM_MENU_CONTROL_ID_POMODORO_MENU:
    case DIVOOM_MENU_CONTROL_ID_STOP_WATCH_MENU:
    case DIVOOM_MENU_CONTROL_ID_COUNT_DOWN_MENU:
    case DIVOOM_MENU_CONTROL_ID_SCORE_BOARD_MENU:
    case DIVOOM_MENU_CONTROL_ID_NOISE_MEASURE_MENU:
    case DIVOOM_MENU_CONTROL_ID_VOICE_RECORD_TOOL_MENU:
        return (Pdivoom_astrootoo_tools_ctrl)h;
    default:
        break;
    }
    return NULL;
}

int divoom_astrootoo_tools_whitebox_get_pomo_mode(void)
{
    Pdivoom_astrootoo_tools_ctrl phandle;

    phandle = divoom_astrootoo_tools_whitebox_get();
    if (!phandle || phandle->menu_id != DIVOOM_MENU_CONTROL_ID_POMODORO_MENU)
    {
        return -1;
    }
    return phandle->pomo_mode;
}

int divoom_astrootoo_tools_whitebox_get_pomo_view(void)
{
    Pdivoom_astrootoo_tools_ctrl phandle;

    phandle = divoom_astrootoo_tools_whitebox_get();
    if (!phandle || phandle->menu_id != DIVOOM_MENU_CONTROL_ID_POMODORO_MENU)
    {
        return -1;
    }
    return phandle->pomo_view;
}

int divoom_astrootoo_tools_whitebox_get_pomo_work_min(void)
{
    Pdivoom_astrootoo_tools_ctrl phandle;

    phandle = divoom_astrootoo_tools_whitebox_get();
    if (!phandle || phandle->menu_id != DIVOOM_MENU_CONTROL_ID_POMODORO_MENU)
    {
        return -1;
    }
    return phandle->pomo_work_min;
}

int divoom_astrootoo_tools_whitebox_get_stopwatch_running(void)
{
    Pdivoom_astrootoo_tools_ctrl phandle;

    phandle = divoom_astrootoo_tools_whitebox_get();
    if (!phandle || phandle->menu_id != DIVOOM_MENU_CONTROL_ID_STOP_WATCH_MENU)
    {
        return -1;
    }
    return phandle->running;
}

int divoom_astrootoo_tools_whitebox_get_stopwatch_elapsed_sec(void)
{
    Pdivoom_astrootoo_tools_ctrl phandle;

    phandle = divoom_astrootoo_tools_whitebox_get();
    if (!phandle || phandle->menu_id != DIVOOM_MENU_CONTROL_ID_STOP_WATCH_MENU)
    {
        return -1;
    }
    return phandle->elapsed_sec;
}

int divoom_astrootoo_tools_whitebox_get_countdown_cd_status(void)
{
    Pdivoom_astrootoo_tools_ctrl phandle;

    phandle = divoom_astrootoo_tools_whitebox_get();
    if (!phandle || phandle->menu_id != DIVOOM_MENU_CONTROL_ID_COUNT_DOWN_MENU)
    {
        return -1;
    }
    return phandle->cd_status;
}

int divoom_astrootoo_tools_whitebox_get_countdown_remain_s(void)
{
    Pdivoom_astrootoo_tools_ctrl phandle;

    phandle = divoom_astrootoo_tools_whitebox_get();
    if (!phandle || phandle->menu_id != DIVOOM_MENU_CONTROL_ID_COUNT_DOWN_MENU)
    {
        return -1;
    }
    if (phandle->cd_status == 0)
    {
        return phandle->edit_min * 60 + phandle->edit_sec;
    }
    return phandle->remain_sec;
}

int divoom_astrootoo_tools_whitebox_get_score_left(void)
{
    Pdivoom_astrootoo_tools_ctrl phandle;

    phandle = divoom_astrootoo_tools_whitebox_get();
    if (!phandle || phandle->menu_id != DIVOOM_MENU_CONTROL_ID_SCORE_BOARD_MENU)
    {
        return -1;
    }
    return phandle->score_a;
}

int divoom_astrootoo_tools_whitebox_get_score_right(void)
{
    Pdivoom_astrootoo_tools_ctrl phandle;

    phandle = divoom_astrootoo_tools_whitebox_get();
    if (!phandle || phandle->menu_id != DIVOOM_MENU_CONTROL_ID_SCORE_BOARD_MENU)
    {
        return -1;
    }
    return phandle->score_b;
}

int divoom_astrootoo_tools_whitebox_get_noise_db(void)
{
    Pdivoom_astrootoo_tools_ctrl phandle;

    phandle = divoom_astrootoo_tools_whitebox_get();
    if (!phandle || phandle->menu_id != DIVOOM_MENU_CONTROL_ID_NOISE_MEASURE_MENU)
    {
        return -1;
    }
    return phandle->noise_db;
}

int divoom_astrootoo_tools_whitebox_get_page(void)
{
    Pdivoom_astrootoo_tools_ctrl phandle;

    phandle = divoom_astrootoo_tools_whitebox_get();
    if (!phandle)
    {
        return -1;
    }
    return (int)phandle->page;
}

int divoom_astrootoo_tools_whitebox_get_list_focus(void)
{
    Pdivoom_astrootoo_tools_ctrl phandle;
    lv_obj_t *focused;
    int pos;

    phandle = divoom_astrootoo_tools_whitebox_get();
    if (!phandle || phandle->menu_id != DIVOOM_MENU_CONTROL_ID_TOOL
        || phandle->page != DIVOOM_ASTROTOO_TOOLS_PAGE_LIST || !phandle->direct_handle)
    {
        return -1;
    }
    focused = lv_group_get_focused(lv_group_get_default());
    if (!focused)
    {
        return -1;
    }
    pos = divoom_menu_direct_ctrl_get_pos(phandle->direct_handle, focused);
    if (pos < 0 || pos >= DIVOOM_ASTRO_TOOLS_ROW_CNT)
    {
        return -1;
    }
    return pos;
}

int divoom_astrootoo_tools_whitebox_get_record_state(void)
{
    Pdivoom_astrootoo_tools_ctrl phandle;

    phandle = divoom_astrootoo_tools_whitebox_get();
    if (!phandle || phandle->menu_id != DIVOOM_MENU_CONTROL_ID_VOICE_RECORD_TOOL_MENU)
    {
        return -1;
    }
    return phandle->memo_state;
}

int divoom_astrootoo_tools_memo_is_playing(void)
{
    /* memo_state：0 空闲 1 录音 2 回放 */
    return divoom_astrootoo_tools_whitebox_get_record_state() == 2;
}

void divoom_astrootoo_tools_memo_sync_ui_idle(void)
{
	Pdivoom_astrootoo_tools_ctrl phandle;

	phandle = divoom_astrootoo_tools_whitebox_get();
	if (!phandle || phandle->menu_id != DIVOOM_MENU_CONTROL_ID_VOICE_RECORD_TOOL_MENU)
		return;
	phandle->memo_pending = 0;
	phandle->memo_start_wait = 0;
	phandle->memo_state = 0;
	phandle->memo_elapsed = 0;
	divoom_astrootoo_tools_memo_sync_clip(phandle);
	divoom_astrootoo_tools_memo_refresh(phandle);
}

int divoom_astrootoo_tools_whitebox_get_record_clip_sec(void)
{
    Pdivoom_astrootoo_tools_ctrl phandle;

    phandle = divoom_astrootoo_tools_whitebox_get();
    if (!phandle || phandle->menu_id != DIVOOM_MENU_CONTROL_ID_VOICE_RECORD_TOOL_MENU)
    {
        return -1;
    }
    return phandle->memo_clip;
}

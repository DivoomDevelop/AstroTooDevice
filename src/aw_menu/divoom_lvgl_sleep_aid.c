/**
 * 睡眠辅助 UI（MasterGo 800x1280）：左侧 TAB + 右侧音源列表/子页。
 */
#include "divoom.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "divoom_menu_control.h"
#include "divoom_lvgl_sleep_aid.h"
#include "divoom_lvgl_message_box.h"
#include "divoom_sleep_aid_config.h"
#include "divoom_sleep_aid_ctrl.h"
#include "divoom_light_app.h"
#include "divoom_manage_image.h"
#include "divoom_voice_read.h"
#include "divoom_mic_record.h"
#include "divoom_http_client.h"

/* 布局：左栏 293px，右栏 507px（layer 609:73493） */
#define SA_HDR_Y            10
#define SA_SIDEBAR_X        10
#define SA_SIDEBAR_W        273
#define SA_SIDEBAR_H        1260
#define SA_LIST_X           293
#define SA_LIST_W           507
#define SA_LIST_H           1280
#define SA_TAB_COUNT        7
/* 左栏 TAB 项：MasterGo 609:73638（未激活）/ 609:73665（激活） */
#define SA_TAB_ITEM_Y       20
#define SA_TAB_ITEM_W       253
#define SA_TAB_ITEM_H       60
#define SA_TAB_ITEM_GAP     16
#define SA_TAB_ITEM_R       24
#define SA_TAB_INSET_X      ((SA_SIDEBAR_W - SA_TAB_ITEM_W) / 2)
#define SA_TAB_TEXT_PAD_X   20
#define SA_TAB_LABEL_W      208
#define SA_TAB_ARROW_X      228
#define SA_TAB_ARROW_Y      20
#define SA_ROW_H            72
#define SA_ROW_GAP          4
#define SA_LIST_ROW_W       SA_LIST_W
#define SA_LIST_MAX_ROWS    48
/* 音源行子项（MasterGo 598:86418） */
#define SA_ROW_ICON_SIZE    40
#define SA_ROW_ICON_LEFT    16
#define SA_ROW_LABEL_X      70
#define SA_ROW_FAV_RIGHT    16
#define SA_ROW_RADIUS       24
#define SA_LIST_ROW_START_Y 4
#define SA_ROW_FOCUS_BORDER_COLOR 0x6D6D6D  /* MasterGo 598:86419 选中框 */
#define SA_ROW_IMG_NORMAL   "105/105_normal.png"
#define SA_ROW_IMG_PLAY     "105/105_play.png"
#define SA_ROW_IMG_FAV      "105/105_fav.png"
#define SA_ROW_IMG_UNFAV    "105/105_unfav.png"
#define SA_ADD_DL_MAX           3
#define SA_DL_SPINNER_SIZE      40
#define SA_DL_SPINNER_LEFT      16
#define SA_DL_LOADING_LABEL_X   70
#define SA_DL_PCT_RIGHT_PAD     16
#define SA_DL_SPINNER_COLOR     0x3B3B3B
#define SA_SETT_IMG_BR_LEFT   "105/105_br_left.png"
#define SA_SETT_IMG_BR_RIGHT  "105/105_br_right.png"
#define SA_SETT_IMG_VOL_LEFT  "105/105_vol_left.png"
#define SA_SETT_IMG_VOL_RIGHT "105/105_vol_right.png"
#define SA_SETT_IMG_ON        "105/105_on.png"
#define SA_SETT_IMG_OFF       "105/105_off.png"

#define SA_COL_BG           0xE4E4E4
#define SA_COL_SIDEBAR      0xD1D1D1
#define SA_COL_TAB_ACTIVE_BG 0xE4E4E4  /* paint_530:63223，激活 TAB 底 */
#define SA_COL_TAB_TEXT     0x000000    /* paint_58:16559 */
#define SA_COL_TAB_ARROW    0x434242    /* paint_50:61459 */
#define SA_COL_CARD_BG      0xFFFFFF    /* 右侧子页卡片底 */
#define SA_COL_TEXT         0x434242
#define SA_COL_HEART        0xDB2020
#define SA_COL_ARROW        0x727272

/* 录音右栏（MasterGo 598:86093 列表 / 598:89724 待录 / 598:86179 录音中） */
#define SA_RECORD_MAX_CLIPS     5   /* 与 DIVOOM_SLEEP_AID_RECORD_MAX_SLOTS 一致，文件 ID 固定 1~5 循环复用 */
#define SA_RECORD_ROW_H         100
#define SA_RECORD_ROW_GAP       10
#define SA_RECORD_LIST_Y        10
#define SA_RECORD_ROW_W         497
#define SA_RECORD_ICON_SIZE     40
#define SA_RECORD_ICON_X        16
#define SA_RECORD_ICON_Y        30
#define SA_RECORD_TITLE_X       72
#define SA_RECORD_TITLE_Y       20
#define SA_RECORD_DUR_Y         56
#define SA_RECORD_PLAY_X        440
#define SA_RECORD_PLAY_Y        30
#define SA_RECORD_PANEL_GAP     20
#define SA_RECORD_PANEL_Y       (SA_RECORD_LIST_Y + SA_RECORD_MAX_CLIPS * (SA_RECORD_ROW_H + SA_RECORD_ROW_GAP) + SA_RECORD_PANEL_GAP)
#define SA_RECORD_PANEL_H       300
#define SA_RECORD_PANEL_W       497
#define SA_RECORD_PANEL_RADIUS  24
#define SA_RECORD_PANEL_BG      0xF2F2F2
#define SA_RECORD_HINT_COLOR    0x797979
#define SA_RECORD_IMG_WAVE      "105/105_record2.png"
#define SA_RECORD_IMG_IDLE      "105/105_record_round1.png"
#define SA_RECORD_IMG_STOP      "105/105_record_round2.png"
#define SA_RECORD_WAV_HDR       44
#define SA_RECORD_BYTES_PER_SEC 32000
#define SA_RECORD_MAX_DURATION_SEC  (5u * 60u)  /* 单次录音最长 5 分钟 */

/* DIY 右栏（MasterGo 598:86814 常态 / 598:87640 行内调节） */
#define SA_DIY_CARD_W           497
#define SA_DIY_TOP_BTN_H        90
#define SA_DIY_TOP_Y            10
#define SA_DIY_TOP_GAP          10
#define SA_DIY_TOP_LABEL_X      22
#define SA_DIY_TOP_LABEL_Y      25
#define SA_DIY_TOP_ICON_X       441
#define SA_DIY_TOP_ICON_Y       25
#define SA_DIY_SECTION_Y        240
#define SA_DIY_SECTION_TITLE_X  16
#define SA_DIY_SECTION_TITLE_H  26
#define SA_DIY_SECTION_TITLE_CLR 0x636363
#define SA_DIY_CH_FIRST_Y       276
#define SA_DIY_ROW_H            100
#define SA_DIY_ROW_GAP          10
#define SA_DIY_ICON_SIZE        40
#define SA_DIY_ICON_X           16
#define SA_DIY_LABEL_X          72
#define SA_DIY_LABEL_Y          6
#define SA_DIY_LABEL_H          40
#define SA_DIY_SLIDER_X         20
#define SA_DIY_SLIDER_Y         66
#define SA_DIY_SLIDER_W         456
#define SA_DIY_SLIDER_TRACK_H   8
#define SA_DIY_SLIDER_KNOB_SZ   24
/* 滑条对象高度=旋钮区；轨道 8px 用独立底条绘制（MAIN 背景会铺满控件高，不能靠 padding 当轨道） */
#define SA_DIY_SLIDER_CTRL_H    SA_DIY_SLIDER_KNOB_SZ
#define SA_DIY_SLIDER_TRACK_Y   (SA_DIY_SLIDER_Y + (SA_DIY_SLIDER_CTRL_H - SA_DIY_SLIDER_TRACK_H) / 2)
#define SA_DIY_VOL_MAX          100
#define SA_DIY_ROW_BG           0xF2F2F2
#define SA_DIY_BORDER_IDLE      0x6D6D6D
#define SA_DIY_BORDER_EDIT      0xE89B00
#define SA_DIY_SLIDER_TRACK     0xB1B1B1
#define SA_DIY_SLIDER_IND       0x707070
#define SA_DIY_SLIDER_KNOB      0x0CA9F1
#define SA_DIY_IMG_PREVIEW      SA_ROW_IMG_PLAY
#define SA_DIY_IMG_FAV          SA_ROW_IMG_FAV
/* DIY 混音收藏：整包音源 voice_id 固定为 1（与 divoom_sleep_aid_voice_get_list TYPE_DIY 一致） */
#define SA_DIY_MIX_VOICE_ID     1u

#define SA_ADD_DL_LOG(fmt, ...) divoom_print("sleep_aid_add: " fmt, ##__VA_ARGS__)
#define SA_DIY_ICON_Y           6

#define SA_DIY_FOCUS_PREVIEW    0
#define SA_DIY_FOCUS_FAV        1
#define SA_DIY_FOCUS_CH0        2
#define SA_DIY_FOCUS_CH_LAST    9
#define SA_DIY_FOCUS_COUNT      10

/* 设置右栏（MasterGo 598:89197 掩盖关 / 598:88153 掩盖开，容器 598:88153） */
#define SA_SETT_CARD_W          497
#define SA_SETT_ROW_H           90
#define SA_SETT_SECTION_TITLE_H 26
#define SA_SETT_SECTION_TITLE_X 16
#define SA_SETT_SECTION_TITLE_GAP 10
#define SA_SETT_ICON_SIZE       36
#define SA_SETT_ICON_L_X        22
#define SA_SETT_ICON_L_Y        27
#define SA_SETT_ICON_R_X        439
#define SA_SETT_SLIDER_X        74
#define SA_SETT_SLIDER_Y        41
#define SA_SETT_SLIDER_W        349
#define SA_SETT_SLIDER_CTRL_H   SA_DIY_SLIDER_KNOB_SZ
#define SA_SETT_SLIDER_TRACK_Y  (SA_SETT_SLIDER_Y + (SA_SETT_SLIDER_CTRL_H - SA_DIY_SLIDER_TRACK_H) / 2)
#define SA_SETT_ROW_LABEL_X     22
#define SA_SETT_ROW_LABEL_Y     25
#define SA_SETT_ROW_VALUE_RIGHT_PAD 22
#define SA_SETT_ROW_VALUE_X     SA_SETT_ROW_LABEL_X
#define SA_SETT_ROW_VALUE_W     (SA_SETT_CARD_W - SA_SETT_ROW_LABEL_X - SA_SETT_ROW_VALUE_RIGHT_PAD)
#define SA_SETT_TOGGLE_X        402
#define SA_SETT_TOGGLE_Y        23
#define SA_SETT_TOGGLE_W        72
#define SA_SETT_TOGGLE_H        44
#define SA_SETT_BRIGHT_LEVEL_MAX  100
#define SA_SETT_VOL_LEVEL_MAX     100
#define SA_SETT_PREVIEW_NONE    0
#define SA_SETT_PREVIEW_BRIGHT  1
#define SA_SETT_PREVIEW_VOL     2
/* 598:88153 右栏绝对 Y（list_scroll 内） */
#define SA_SETT_Y_BEFORE_BRIGHT   28
#define SA_SETT_Y_BEFORE_VOL      164
#define SA_SETT_Y_BEFORE_TIMER    300
#define SA_SETT_Y_AFTER_HDR       470
#define SA_SETT_Y_AFTER_MASK      506
#define SA_SETT_Y_AFTER_BRIGHT    606
#define SA_SETT_Y_AFTER_VOL       742
#define SA_SETT_Y_AFTER_MUSIC     878
#define SA_SETT_Y_AFTER_TIMER     978

#define SA_MBOX_KIND_VOICE      0
#define SA_MBOX_KIND_SETT_TIMER 1
#define SA_MBOX_KIND_SETT_MUSIC 2

#define SA_SETT_FIELD_BEFORE_TIME 0
#define SA_SETT_FIELD_AFTER_TIME  1

enum
{
	SA_TAB_FAV = 0,
	SA_TAB_NOISE,
	SA_TAB_WHITE,
	SA_TAB_MUSIC,
	SA_TAB_DIY,
	SA_TAB_RECORD,
	SA_TAB_SETTINGS,
};

struct _divoom_lvgl_sleep_aid_ctrl;
typedef struct _divoom_lvgl_sleep_aid_ctrl *Pdivoom_lvgl_sleep_aid_ctrl;

typedef struct _sleep_aid_add_dl_slot
{
	Pdivoom_lvgl_sleep_aid_ctrl ph;
	uint32 voice_id;
	char file_id[56];
	char local_path[256];
	uint32 read_len;
	uint32 tol_len;
	uint8 done;
	uint8 success;
	lv_obj_t *pct_lbl;
	lv_obj_t *spinner_arc;
} sleep_aid_add_dl_slot_t;

#define SA_VOICE_TAB_STORE_COUNT    4

typedef struct _sleep_aid_voice_tab_store
{
	uint32 *list_ids;
	int *list_types;
	int *list_video_types;
	char *list_names;
	int list_count;
	uint8 list_has_plus;
	int list_focus_idx;
	int list_row_paint_idx;
	lv_obj_t **list_row_objs;
	int list_row_cap;
} sleep_aid_voice_tab_store_t;

typedef struct _divoom_lvgl_sleep_aid_ctrl
{
	int menu_id;
	uint8 styles_inited;
	lv_style_t root_style;
	lv_style_t sidebar_style;
	lv_style_t tab_cell_style;
	lv_style_t tab_sel_style;
	lv_style_t list_scroll_style;
	lv_style_t row_style;
	lv_style_t diy_row_style;
	lv_obj_t *main_obj;
	lv_obj_t *sidebar_cont;
	lv_obj_t *content_cont;
	lv_obj_t *tab_panel[SA_TAB_COUNT];
	uint8 tab_built[SA_TAB_COUNT];
	int tab_saved_focus[SA_TAB_COUNT];
	sleep_aid_voice_tab_store_t voice_store[SA_VOICE_TAB_STORE_COUNT];
	int last_shown_tab;
	lv_obj_t *tab_objs[SA_TAB_COUNT];
	lv_obj_t *tab_text_lbl[SA_TAB_COUNT];
	lv_obj_t *tab_arrow_lbl[SA_TAB_COUNT];
	int tab_idx;
	uint8 tabs_focus;
	uint32 *list_ids;
	int *list_types;
	int *list_video_types;
	char *list_names;
	int list_count;
	uint8 list_has_plus;
	int list_focus_idx;
	lv_obj_t **list_row_objs;
	int list_row_cap;
	int list_row_paint_idx;
	lv_obj_t *diy_preview_btn;
	lv_obj_t *diy_fav_btn;
	lv_obj_t *diy_fav_icon;
	lv_obj_t *diy_ch_rows[8];
	lv_obj_t *diy_ch_sliders[8];
	lv_obj_t *diy_edit_cont;
	lv_obj_t *diy_edit_slider;
	lv_obj_t *diy_edit_lbl;
	lv_obj_t *diy_edit_icon;
	int diy_focus;
	int diy_sel_ch;
	uint8 diy_edit_active;
	lv_obj_t *sett_row_objs[8];
	int sett_row_count;
	lv_obj_t *sett_before_bright_sl;
	lv_obj_t *sett_before_vol_sl;
	lv_obj_t *sett_after_bright_sl;
	lv_obj_t *sett_after_vol_sl;
	lv_obj_t *sett_before_time_lbl;
	lv_obj_t *sett_after_time_lbl;
	lv_obj_t *sett_after_music_lbl;
	lv_obj_t *sett_mask_img;
	lv_obj_t *record_rows[SA_RECORD_MAX_CLIPS];
	lv_obj_t *record_row_titles[SA_RECORD_MAX_CLIPS];
	lv_obj_t *record_row_durs[SA_RECORD_MAX_CLIPS];
	lv_obj_t *record_empty_lbl;
	lv_obj_t *record_panel;
	lv_obj_t *record_panel_title;
	lv_obj_t *record_panel_hint;
	lv_obj_t *record_panel_icon;
	lv_obj_t *record_time_lbl;
	int record_row_count;
	int record_focus;
	uint8 record_ui_busy;
	uint32 record_elapsed_sec;
	int sett_row;
	uint8 sett_edit_active;
	uint8 sett_preview_kind;
	void *mbox;
	uint8 mbox_kind;
	int mbox_sett_field;
	int mbox_music_preview_sel;
	lv_timer_t *mbox_music_preview_timer;
	int mbox_voice_type;
	uint32 mbox_voice_id;
	uint8 mbox_is_fav;
	divoom_sleep_aid_fav_item *fav_cache;
	int fav_cache_count;
	uint8 fav_tab_stale;
	uint8 voice_list_cfg_revision_seen;
	uint8 add_dl_count;
	uint8 add_dl_cancel;
	uint8 add_dl_task_done;
	uint8 add_dl_spin_phase;
	sleep_aid_add_dl_slot_t add_dl_slots[SA_ADD_DL_MAX];
	lv_timer_t *add_dl_timer;
	lv_timer_t *add_dl_defer_timer;
	lv_timer_t *record_refresh_timer;
} divoom_lvgl_sleep_aid_ctrl, *Pdivoom_lvgl_sleep_aid_ctrl;

static const char *const s_tab_lang_keys[SA_TAB_COUNT] =
{
	"SleepAidTabFav",
	"SleepAidTabNoise",
	"SleepAidTabNature",
	"SleepAidTabMusic",
	"SleepAidTabDiy",
	"SleepAidTabRecord",
	"SleepAidTabSet",
};

/* 稿面 598:86814 通道顺序：风扇→蛙声→篝火→海浪→雨声→溪流→鸟叫→颂钵 */
static const char *const s_diy_ch_lang_keys[8] =
{
	"SleepAidDiyFan",
	"SleepAidDiyFrog",
	"SleepAidDiyFire",
	"SleepAidDiyWave",
	"SleepAidDiyRain",
	"SleepAidDiyCreek",
	"SleepAidDiyBird",
	"SleepAidDiyBowl",
};

static const char *const s_diy_ch_img[8] =
{
	"50/50_feng.png",
	"50/50_horse.png",
	"50/50_fire.png",
	"50/50_sea.png",
	"50/50_night.png",
	"50/50_creek.png",
	"50/50_bird.png",
	"50/50_quiet.png",
};

/* 显示行下标 = volume[] = mix_pcm PCM 下标（风扇..颂钵，无 remap） */
static int sleep_aid_diy_vol_idx(int disp_ch)
{
	if (disp_ch < 0 || disp_ch >= 8)
	{
		return 0;
	}
	return disp_ch;
}

static const int s_sett_timer_minutes[6] = { 15, 30, 60, 90, 120, 0 };

static const char *const s_sett_after_music_keys[3] =
{
	"SleepAidAfterMusic0",
	"SleepAidAfterMusic1",
	"SleepAidAfterMusic2",
};

static int sleep_aid_tab_aid_type(int tab)
{
	switch (tab)
	{
	case SA_TAB_FAV:
	{
		return DIVOOM_SLEEP_AID_TYPE_FAV;
	}
	case SA_TAB_NOISE:
	{
		return DIVOOM_SLEEP_AID_TYPE_NOISE;
	}
	case SA_TAB_WHITE:
	{
		return DIVOOM_SLEEP_AID_TYPE_WHITE;
	}
	case SA_TAB_MUSIC:
	{
		return DIVOOM_SLEEP_AID_TYPE_MUSIC;
	}
	default:
	{
		return -1;
	}
	}
}

static int sleep_aid_tab_is_voice_list(int tab)
{
	if (tab == SA_TAB_FAV || tab == SA_TAB_NOISE || tab == SA_TAB_WHITE || tab == SA_TAB_MUSIC)
	{
		return 1;
	}
	return 0;
}

static int sleep_aid_voice_store_index(int tab)
{
	switch (tab)
	{
	case SA_TAB_FAV:
	{
		return 0;
	}
	case SA_TAB_NOISE:
	{
		return 1;
	}
	case SA_TAB_WHITE:
	{
		return 2;
	}
	case SA_TAB_MUSIC:
	{
		return 3;
	}
	default:
	{
		return -1;
	}
	}
}

static void sleep_aid_slider_clear_border_outline_shadow(lv_obj_t *slider);
static void sleep_aid_diy_clear_ptrs(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_sett_clear_ptrs(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_sett_paint_focus(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_record_clamp_focus(Pdivoom_lvgl_sleep_aid_ctrl ph);
static int sleep_aid_sett_row_is_slider(int row, Pdivoom_sleep_aid_info inf);
static int sleep_aid_sett_row_is_brightness(int row, Pdivoom_sleep_aid_info inf);
static int sleep_aid_sett_row_is_volume(int row, Pdivoom_sleep_aid_info inf);
static int sleep_aid_sett_row_level(Pdivoom_sleep_aid_info inf, int row);
static uint8 sleep_aid_sett_level_to_screen_bright(int level);
static int sleep_aid_sett_level_to_voice_vol(int level);
static void sleep_aid_sett_end_edit_preview(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_sett_begin_edit_preview(Pdivoom_lvgl_sleep_aid_ctrl ph, int row);
static void sleep_aid_sett_preview_on_level_changed(Pdivoom_lvgl_sleep_aid_ctrl ph, int row, int level);
static void sleep_aid_sett_clear_edit(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_sett_after_music_preview_item(Pdivoom_lvgl_sleep_aid_ctrl ph, int music_idx);
static void sleep_aid_sett_music_preview_cancel(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_sett_music_mbox_focus_cb(void *user_data, int ctrl_id);
static void sleep_aid_mbox_close(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_mbox_cb(void *user_data, int ctrl_id);
static int sleep_aid_row_is_fav(Pdivoom_lvgl_sleep_aid_ctrl ph, uint32 vid, int vt);
static void sleep_aid_fav_cache_free(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_record_clear_ptrs(Pdivoom_lvgl_sleep_aid_ctrl ph);
static lv_obj_t *sleep_aid_record_focus_obj(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_record_paint_focus(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_add_dl_cancel(Pdivoom_lvgl_sleep_aid_ctrl ph);
static int sleep_aid_add_dl_voice_row(Pdivoom_lvgl_sleep_aid_ctrl ph, uint32 voice_id);
static void sleep_aid_add_dl_begin(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_add_dl_deferred_cb(lv_timer_t *t);
static void sleep_aid_list_focus_new_downloads(Pdivoom_lvgl_sleep_aid_ctrl ph, const uint32 *voice_ids, int n);
static void sleep_aid_send_get_all_three_types(void);
static lv_obj_t *sleep_aid_list_parent(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_detach_list_model_from_ph(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_voice_store_clear(Pdivoom_lvgl_sleep_aid_ctrl ph, int tab);
static void sleep_aid_voice_store_apply(Pdivoom_lvgl_sleep_aid_ctrl ph, int tab);
static void sleep_aid_voice_store_commit(Pdivoom_lvgl_sleep_aid_ctrl ph, int tab);
static void sleep_aid_voice_store_clear_all(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_tab_save_focus(Pdivoom_lvgl_sleep_aid_ctrl ph, int tab);
static void sleep_aid_tab_restore_focus(Pdivoom_lvgl_sleep_aid_ctrl ph, int tab);
static void sleep_aid_show_tab_panel(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_on_tab_leave(Pdivoom_lvgl_sleep_aid_ctrl ph, int tab);
static void sleep_aid_invalidate_tab(Pdivoom_lvgl_sleep_aid_ctrl ph, int tab);
static void sleep_aid_refresh_tab_labels(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_refresh_content_force(Pdivoom_lvgl_sleep_aid_ctrl ph, uint8 force_rebuild);
static void sleep_aid_refresh_content(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_paint_focus(Pdivoom_lvgl_sleep_aid_ctrl ph);
static int sleep_aid_list_total_rows(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_record_stop_refresh_timer(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_shutdown_async_work(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_reset_tab_content_focus(Pdivoom_lvgl_sleep_aid_ctrl ph, int tab);
static void sleep_aid_leave_subpage(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_teardown_session(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_apply_tab_visual(Pdivoom_lvgl_sleep_aid_ctrl ph, int idx);
static void sleep_aid_set_row_focus_border(lv_obj_t *row, uint8 focused);
static void sleep_aid_row_set_state_icon(lv_obj_t *row, uint8 is_play);
static void sleep_aid_row_set_fav_icon(lv_obj_t *row, uint8 is_fav);
static void sleep_aid_build_voice_list_ui(Pdivoom_lvgl_sleep_aid_ctrl ph);
static int sleep_aid_list_voice_focus_active(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_stop_list_preview(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_apply_list_voice(Pdivoom_lvgl_sleep_aid_ctrl ph);
static int sleep_aid_record_voice_focus_active(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_apply_record_voice(Pdivoom_lvgl_sleep_aid_ctrl ph);
static int sleep_aid_diy_ch_focus_active(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_apply_diy_voice(Pdivoom_lvgl_sleep_aid_ctrl ph);

/* 设置 / DIY 共用：8px 轨道底条 + 24px 旋钮区 + 透明 MAIN（与 MasterGo 一致） */
static lv_obj_t *sleep_aid_slider_create_track_bg(lv_obj_t *row, lv_coord_t x, lv_coord_t track_y, lv_coord_t w)
{
	lv_obj_t *track;

	track = lv_obj_create(row);
	lv_obj_set_size(track, w, SA_DIY_SLIDER_TRACK_H);
	lv_obj_set_pos(track, x, track_y);
	lv_obj_clear_flag(track, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_clear_flag(track, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_pad_all(track, 0, 0);
	lv_obj_set_style_border_width(track, 0, 0);
	lv_obj_set_style_radius(track, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_bg_color(track, lv_color_hex(SA_DIY_SLIDER_TRACK), 0);
	lv_obj_set_style_bg_opa(track, LV_OPA_COVER, 0);
	lv_obj_move_background(track);
	return track;
}

static void sleep_aid_slider_set_geometry(lv_obj_t *sl, lv_coord_t w)
{
	lv_coord_t pad_v;

	if (!sl)
	{
		return;
	}
	pad_v = (SA_DIY_SLIDER_CTRL_H - SA_DIY_SLIDER_TRACK_H) / 2;
	lv_obj_set_size(sl, w, SA_DIY_SLIDER_CTRL_H);
	lv_obj_set_style_pad_top(sl, pad_v, LV_PART_MAIN);
	lv_obj_set_style_pad_bottom(sl, pad_v, LV_PART_MAIN);
	lv_obj_set_style_pad_left(sl, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_right(sl, 0, LV_PART_MAIN);
	lv_obj_set_ext_click_area(sl, pad_v);
}

static void sleep_aid_slider_apply_style(lv_obj_t *sl, int knob_visible, int level_max)
{
	if (!sl)
	{
		return;
	}

	if (level_max < 0)
	{
		level_max = 0;
	}
	lv_slider_set_range(sl, 0, level_max);
	sleep_aid_slider_clear_border_outline_shadow(sl);
	lv_obj_set_style_bg_opa(sl, LV_OPA_TRANSP, LV_PART_MAIN);
	lv_obj_set_style_bg_color(sl, lv_color_hex(SA_DIY_SLIDER_IND), LV_PART_INDICATOR);
	lv_obj_set_style_bg_opa(sl, LV_OPA_COVER, LV_PART_INDICATOR);
	lv_obj_set_style_radius(sl, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
	lv_obj_set_style_bg_color(sl, lv_color_hex(SA_DIY_SLIDER_KNOB), LV_PART_KNOB);
	lv_obj_set_style_bg_opa(sl, knob_visible ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_KNOB);
	lv_obj_set_style_radius(sl, LV_RADIUS_CIRCLE, LV_PART_KNOB);
	lv_obj_set_style_width(sl, SA_DIY_SLIDER_KNOB_SZ, LV_PART_KNOB);
	lv_obj_set_style_height(sl, SA_DIY_SLIDER_KNOB_SZ, LV_PART_KNOB);
	lv_obj_set_style_pad_all(sl, 0, LV_PART_KNOB);
	lv_obj_set_style_transform_width(sl, 0, LV_PART_KNOB);
	lv_obj_set_style_transform_height(sl, 0, LV_PART_KNOB);
	lv_obj_clear_flag(sl, LV_OBJ_FLAG_CLICKABLE);
}

static void sleep_aid_diy_set_row_border(lv_obj_t *row, uint8 on, uint32_t color)
{
	if (!row)
	{
		return;
	}
	lv_obj_set_style_border_width(row, on ? 2 : 0, 0);
	lv_obj_set_style_border_color(row, lv_color_hex(color), 0);
	lv_obj_set_style_border_opa(row, LV_OPA_COVER, 0);
}

static void sleep_aid_diy_refresh_ch_slider(Pdivoom_lvgl_sleep_aid_ctrl ph, int ch)
{
	Pdivoom_sleep_aid_info inf;
	int v;

	if (ch < 0 || ch >= 8 || !ph->diy_ch_sliders[ch])
	{
		return;
	}
	inf = divoom_sleep_aid_ctrl_get_settings();
	v = inf->volume[sleep_aid_diy_vol_idx(ch)];
	if (v < 0)
	{
		v = 0;
	}
	if (v > SA_DIY_VOL_MAX)
	{
		v = SA_DIY_VOL_MAX;
	}
	lv_slider_set_value(ph->diy_ch_sliders[ch], v, LV_ANIM_OFF);
}

static void sleep_aid_diy_sync_sel_ch(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	sleep_aid_diy_refresh_ch_slider(ph, ph->diy_sel_ch);
}

static void sleep_aid_diy_update_fav_btn(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	uint8 is_fav;

	if (!ph || !ph->diy_fav_icon)
	{
		return;
	}
	is_fav = sleep_aid_row_is_fav(ph, SA_DIY_MIX_VOICE_ID, DIVOOM_SLEEP_AID_TYPE_DIY) ? (uint8)1 : (uint8)0;
	divoom_menu_control_lv_img_set_src(ph->diy_fav_icon, is_fav ? SA_ROW_IMG_FAV : SA_ROW_IMG_UNFAV);
}

static void sleep_aid_diy_apply_visual(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int i;
	uint8 focused;
	uint32_t border_color;

	if (ph->tab_idx != SA_TAB_DIY)
	{
		return;
	}

	for (i = 0; i < SA_DIY_FOCUS_COUNT; i++)
	{
		lv_obj_t *obj = NULL;
		int disp_ch;

		focused = (!ph->tabs_focus && ph->diy_focus == i) ? (uint8)1 : (uint8)0;
		border_color = SA_DIY_BORDER_IDLE;

		if (i == SA_DIY_FOCUS_PREVIEW)
		{
			obj = ph->diy_preview_btn;
		}
		else if (i == SA_DIY_FOCUS_FAV)
		{
			obj = ph->diy_fav_btn;
		}
		else if (i >= SA_DIY_FOCUS_CH0 && i <= SA_DIY_FOCUS_CH_LAST)
		{
			disp_ch = i - SA_DIY_FOCUS_CH0;
			obj = ph->diy_ch_rows[disp_ch];
			if (ph->diy_edit_active && ph->diy_sel_ch == disp_ch)
			{
				border_color = SA_DIY_BORDER_EDIT;
				focused = 1;
			}
			else if (ph->diy_edit_active)
			{
				focused = 0;
			}
		}
		sleep_aid_diy_set_row_border(obj, focused, border_color);
	}
}

static lv_obj_t *sleep_aid_diy_focus_scroll_target(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	if (ph->diy_focus == SA_DIY_FOCUS_PREVIEW)
	{
		return ph->diy_preview_btn;
	}
	if (ph->diy_focus == SA_DIY_FOCUS_FAV)
	{
		return ph->diy_fav_btn;
	}
	if (ph->diy_focus >= SA_DIY_FOCUS_CH0 && ph->diy_focus <= SA_DIY_FOCUS_CH_LAST)
	{
		return ph->diy_ch_rows[ph->diy_focus - SA_DIY_FOCUS_CH0];
	}
	return NULL;
}

static void sleep_aid_slider_clear_border_outline_shadow(lv_obj_t *slider)
{
	static const lv_part_t parts[] = { LV_PART_MAIN, LV_PART_INDICATOR, LV_PART_KNOB };
	static const lv_state_t states[] = {
		LV_STATE_DEFAULT,
		LV_STATE_FOCUSED,
		LV_STATE_FOCUS_KEY,
		LV_STATE_PRESSED
	};
	int pi;
	int si;

	if (!slider)
	{
		return;
	}

	for (pi = 0; pi < (int)(sizeof(parts) / sizeof(parts[0])); pi++)
	{
		for (si = 0; si < (int)(sizeof(states) / sizeof(states[0])); si++)
		{
			lv_obj_set_style_border_width(slider, 0, parts[pi] | states[si]);
			lv_obj_set_style_outline_width(slider, 0, parts[pi] | states[si]);
			lv_obj_set_style_outline_pad(slider, 0, parts[pi] | states[si]);
			lv_obj_set_style_shadow_width(slider, 0, parts[pi] | states[si]);
		}
	}
}

static void sleep_aid_init_styles(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	if (ph->styles_inited)
	{
		return;
	}

	lv_style_init(&ph->root_style);
	lv_style_set_bg_color(&ph->root_style, lv_color_hex(SA_COL_BG));
	lv_style_set_border_width(&ph->root_style, 0);
	lv_style_set_pad_all(&ph->root_style, 0);

	lv_style_init(&ph->sidebar_style);
	lv_style_set_bg_color(&ph->sidebar_style, lv_color_hex(SA_COL_SIDEBAR));
	lv_style_set_border_width(&ph->sidebar_style, 0);
	lv_style_set_pad_all(&ph->sidebar_style, 0);
	lv_style_set_radius(&ph->sidebar_style, 0);

	/* 未激活：609:73638，矩形透明 + 圆角 24 */
	lv_style_init(&ph->tab_cell_style);
	lv_style_set_bg_opa(&ph->tab_cell_style, LV_OPA_TRANSP);
	lv_style_set_border_width(&ph->tab_cell_style, 0);
	lv_style_set_pad_all(&ph->tab_cell_style, 0);
	lv_style_set_radius(&ph->tab_cell_style, SA_TAB_ITEM_R);

	/* 激活：609:73665，底 #E4E4E4 + 圆角 24 */
	lv_style_init(&ph->tab_sel_style);
	lv_style_set_bg_color(&ph->tab_sel_style, lv_color_hex(SA_COL_TAB_ACTIVE_BG));
	lv_style_set_bg_opa(&ph->tab_sel_style, LV_OPA_COVER);
	lv_style_set_border_width(&ph->tab_sel_style, 0);
	lv_style_set_pad_all(&ph->tab_sel_style, 0);
	lv_style_set_radius(&ph->tab_sel_style, SA_TAB_ITEM_R);

	lv_style_init(&ph->list_scroll_style);
	lv_style_set_bg_opa(&ph->list_scroll_style, LV_OPA_TRANSP);
	lv_style_set_border_width(&ph->list_scroll_style, 0);
	lv_style_set_pad_all(&ph->list_scroll_style, 0);

	lv_style_init(&ph->row_style);
	lv_style_set_bg_opa(&ph->row_style, LV_OPA_TRANSP);
	lv_style_set_border_width(&ph->row_style, 0);
	lv_style_set_pad_all(&ph->row_style, 0);

	lv_style_init(&ph->diy_row_style);
	lv_style_set_bg_color(&ph->diy_row_style, lv_color_hex(SA_DIY_ROW_BG));
	lv_style_set_bg_opa(&ph->diy_row_style, LV_OPA_COVER);
	lv_style_set_border_width(&ph->diy_row_style, 0);
	lv_style_set_pad_all(&ph->diy_row_style, 0);
	lv_style_set_radius(&ph->diy_row_style, 24);

	ph->styles_inited = 1;
}

static void sleep_aid_reset_styles(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	if (!ph->styles_inited)
	{
		return;
	}
	lv_style_reset(&ph->root_style);
	lv_style_reset(&ph->sidebar_style);
	lv_style_reset(&ph->tab_cell_style);
	lv_style_reset(&ph->tab_sel_style);
	lv_style_reset(&ph->list_scroll_style);
	lv_style_reset(&ph->row_style);
	lv_style_reset(&ph->diy_row_style);
	ph->styles_inited = 0;
}

static uint8 sleep_aid_ptr_in_voice_store(Pdivoom_lvgl_sleep_aid_ctrl ph, void *p)
{
	int i;

	if (!ph || !p)
	{
		return 0;
	}
	for (i = 0; i < SA_VOICE_TAB_STORE_COUNT; i++)
	{
		sleep_aid_voice_tab_store_t *st = &ph->voice_store[i];

		if (st->list_ids == p || st->list_types == p || st->list_video_types == p
		    || st->list_names == p || st->list_row_objs == p)
		{
			return 1;
		}
	}
	return 0;
}

/** 列表缓冲已 commit 到 voice_store 时仅断开 ph 引用，不释放（避免切 TAB 后 UAF）。 */
static void sleep_aid_detach_list_model_from_ph(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	if (!ph)
	{
		return;
	}
	ph->list_ids = NULL;
	ph->list_types = NULL;
	ph->list_video_types = NULL;
	ph->list_names = NULL;
	ph->list_count = 0;
	ph->list_has_plus = 0;
	ph->list_row_objs = NULL;
	ph->list_row_cap = 0;
	ph->list_row_paint_idx = -1;
}

static void sleep_aid_clear_list_model(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	if (!ph)
	{
		return;
	}
	if (ph->list_ids && !sleep_aid_ptr_in_voice_store(ph, ph->list_ids))
	{
		divoom_system_free(ph->list_ids);
	}
	if (ph->list_types && !sleep_aid_ptr_in_voice_store(ph, ph->list_types))
	{
		divoom_system_free(ph->list_types);
	}
	if (ph->list_video_types && !sleep_aid_ptr_in_voice_store(ph, ph->list_video_types))
	{
		divoom_system_free(ph->list_video_types);
	}
	if (ph->list_names && !sleep_aid_ptr_in_voice_store(ph, ph->list_names))
	{
		divoom_system_free(ph->list_names);
	}
	if (ph->list_row_objs && !sleep_aid_ptr_in_voice_store(ph, ph->list_row_objs))
	{
		divoom_system_free(ph->list_row_objs);
	}
	sleep_aid_detach_list_model_from_ph(ph);
}

static lv_obj_t *sleep_aid_list_parent(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	if (!ph || ph->tab_idx < 0 || ph->tab_idx >= SA_TAB_COUNT)
	{
		return NULL;
	}
	return ph->tab_panel[ph->tab_idx];
}

static void sleep_aid_voice_store_clear(Pdivoom_lvgl_sleep_aid_ctrl ph, int tab)
{
	int si;
	sleep_aid_voice_tab_store_t *st;

	si = sleep_aid_voice_store_index(tab);
	if (!ph || si < 0)
	{
		return;
	}
	st = &ph->voice_store[si];
	if (ph->list_ids && ph->list_ids == st->list_ids)
	{
		sleep_aid_detach_list_model_from_ph(ph);
	}
	if (st->list_ids)
	{
		divoom_system_free(st->list_ids);
	}
	if (st->list_types)
	{
		divoom_system_free(st->list_types);
	}
	if (st->list_video_types)
	{
		divoom_system_free(st->list_video_types);
	}
	if (st->list_names)
	{
		divoom_system_free(st->list_names);
	}
	if (st->list_row_objs)
	{
		divoom_system_free(st->list_row_objs);
	}
	memset(st, 0, sizeof(*st));
}

static void sleep_aid_voice_store_clear_all(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int i;

	if (!ph)
	{
		return;
	}
	sleep_aid_detach_list_model_from_ph(ph);
	for (i = 0; i < SA_VOICE_TAB_STORE_COUNT; i++)
	{
		if (ph->voice_store[i].list_ids)
		{
			divoom_system_free(ph->voice_store[i].list_ids);
		}
		if (ph->voice_store[i].list_types)
		{
			divoom_system_free(ph->voice_store[i].list_types);
		}
		if (ph->voice_store[i].list_video_types)
		{
			divoom_system_free(ph->voice_store[i].list_video_types);
		}
		if (ph->voice_store[i].list_names)
		{
			divoom_system_free(ph->voice_store[i].list_names);
		}
		if (ph->voice_store[i].list_row_objs)
		{
			divoom_system_free(ph->voice_store[i].list_row_objs);
		}
		memset(&ph->voice_store[i], 0, sizeof(ph->voice_store[i]));
	}
	ph->list_row_cap = 0;
	ph->list_row_paint_idx = -1;
}

static void sleep_aid_voice_store_apply(Pdivoom_lvgl_sleep_aid_ctrl ph, int tab)
{
	int si;
	sleep_aid_voice_tab_store_t *st;

	si = sleep_aid_voice_store_index(tab);
	if (!ph || si < 0)
	{
		return;
	}
	st = &ph->voice_store[si];
	ph->list_ids = st->list_ids;
	ph->list_types = st->list_types;
	ph->list_video_types = st->list_video_types;
	ph->list_names = st->list_names;
	ph->list_count = st->list_count;
	ph->list_has_plus = st->list_has_plus;
	ph->list_focus_idx = st->list_focus_idx;
	ph->list_row_paint_idx = st->list_row_paint_idx;
	ph->list_row_objs = st->list_row_objs;
	ph->list_row_cap = st->list_row_cap;
}

static void sleep_aid_voice_store_commit(Pdivoom_lvgl_sleep_aid_ctrl ph, int tab)
{
	int si;
	sleep_aid_voice_tab_store_t *st;

	si = sleep_aid_voice_store_index(tab);
	if (!ph || si < 0)
	{
		return;
	}
	st = &ph->voice_store[si];
	st->list_ids = ph->list_ids;
	st->list_types = ph->list_types;
	st->list_video_types = ph->list_video_types;
	st->list_names = ph->list_names;
	st->list_count = ph->list_count;
	st->list_has_plus = ph->list_has_plus;
	st->list_focus_idx = ph->list_focus_idx;
	st->list_row_paint_idx = ph->list_row_paint_idx;
	st->list_row_objs = ph->list_row_objs;
	st->list_row_cap = ph->list_row_cap;
}

static void sleep_aid_tab_save_focus(Pdivoom_lvgl_sleep_aid_ctrl ph, int tab)
{
	if (!ph || tab < 0 || tab >= SA_TAB_COUNT)
	{
		return;
	}
	if (sleep_aid_tab_is_voice_list(tab))
	{
		int si;

		si = sleep_aid_voice_store_index(tab);
		ph->tab_saved_focus[tab] = ph->list_focus_idx;
		if (si >= 0)
		{
			ph->voice_store[si].list_focus_idx = ph->list_focus_idx;
		}
	}
	else if (tab == SA_TAB_DIY)
	{
		ph->tab_saved_focus[tab] = ph->diy_focus;
	}
	else if (tab == SA_TAB_RECORD)
	{
		ph->tab_saved_focus[tab] = ph->record_focus;
	}
	else if (tab == SA_TAB_SETTINGS)
	{
		ph->tab_saved_focus[tab] = ph->sett_row;
	}
}

static void sleep_aid_tab_restore_focus(Pdivoom_lvgl_sleep_aid_ctrl ph, int tab)
{
	int focus;

	if (!ph || tab < 0 || tab >= SA_TAB_COUNT)
	{
		return;
	}
	focus = ph->tab_saved_focus[tab];
	if (sleep_aid_tab_is_voice_list(tab))
	{
		int rows;

		ph->list_focus_idx = focus;
		rows = sleep_aid_list_total_rows(ph);
		if (rows > 0 && ph->list_focus_idx >= rows)
		{
			ph->list_focus_idx = rows - 1;
		}
		if (ph->list_focus_idx < 0)
		{
			ph->list_focus_idx = 0;
		}
	}
	else if (tab == SA_TAB_DIY)
	{
		ph->diy_focus = focus;
		if (ph->diy_focus < 0 || ph->diy_focus >= SA_DIY_FOCUS_COUNT)
		{
			ph->diy_focus = SA_DIY_FOCUS_CH0;
		}
	}
	else if (tab == SA_TAB_RECORD)
	{
		ph->record_focus = focus;
		sleep_aid_record_clamp_focus(ph);
	}
	else if (tab == SA_TAB_SETTINGS)
	{
		ph->sett_row = focus;
		if (ph->sett_row_count > 0 && ph->sett_row >= ph->sett_row_count)
		{
			ph->sett_row = ph->sett_row_count - 1;
		}
		if (ph->sett_row < 0)
		{
			ph->sett_row = 0;
		}
	}
}

static void sleep_aid_show_tab_panel(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int i;

	if (!ph)
	{
		return;
	}
	for (i = 0; i < SA_TAB_COUNT; i++)
	{
		if (!ph->tab_panel[i])
		{
			continue;
		}
		if (i == ph->tab_idx)
		{
			lv_obj_clear_flag(ph->tab_panel[i], LV_OBJ_FLAG_HIDDEN);
		}
		else
		{
			lv_obj_add_flag(ph->tab_panel[i], LV_OBJ_FLAG_HIDDEN);
		}
	}
}

static void sleep_aid_on_tab_leave(Pdivoom_lvgl_sleep_aid_ctrl ph, int leaving_tab)
{
	if (!ph || leaving_tab < 0 || leaving_tab >= SA_TAB_COUNT)
	{
		return;
	}
	sleep_aid_tab_save_focus(ph, leaving_tab);
	if (sleep_aid_tab_is_voice_list(leaving_tab))
	{
		sleep_aid_stop_list_preview(ph);
		sleep_aid_voice_store_commit(ph, leaving_tab);
		sleep_aid_detach_list_model_from_ph(ph);
	}
	if (leaving_tab == SA_TAB_RECORD)
	{
		sleep_aid_record_stop_refresh_timer(ph);
		sleep_aid_stop_list_preview(ph);
	}
	else if (leaving_tab == SA_TAB_DIY)
	{
		sleep_aid_stop_list_preview(ph);
	}
	else if (leaving_tab == SA_TAB_SETTINGS)
	{
		sleep_aid_sett_end_edit_preview(ph);
		ph->sett_edit_active = 0;
	}
}

static void sleep_aid_invalidate_tab(Pdivoom_lvgl_sleep_aid_ctrl ph, int tab)
{
	if (!ph || tab < 0 || tab >= SA_TAB_COUNT)
	{
		return;
	}
	if (ph->tab_panel[tab])
	{
		lv_obj_clean(ph->tab_panel[tab]);
	}
	ph->tab_built[tab] = 0;
	if (sleep_aid_tab_is_voice_list(tab))
	{
		sleep_aid_voice_store_clear(ph, tab);
		if (ph->tab_idx == tab)
		{
			ph->list_ids = NULL;
			ph->list_types = NULL;
			ph->list_video_types = NULL;
			ph->list_names = NULL;
			ph->list_count = 0;
			ph->list_has_plus = 0;
			ph->list_row_objs = NULL;
			ph->list_row_cap = 0;
			ph->list_row_paint_idx = -1;
		}
	}
	else if (tab == SA_TAB_DIY)
	{
		sleep_aid_diy_clear_ptrs(ph);
	}
	else if (tab == SA_TAB_RECORD)
	{
		sleep_aid_record_clear_ptrs(ph);
		sleep_aid_record_stop_refresh_timer(ph);
	}
	else if (tab == SA_TAB_SETTINGS)
	{
		sleep_aid_sett_clear_ptrs(ph);
	}
}

static void sleep_aid_record_stop_refresh_timer(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	if (!ph || !ph->record_refresh_timer)
	{
		return;
	}
	lv_timer_del(ph->record_refresh_timer);
	ph->record_refresh_timer = NULL;
}

/** 子页焦点恢复默认（ESC 回侧栏或退出前调用）。 */
static void sleep_aid_reset_tab_content_focus(Pdivoom_lvgl_sleep_aid_ctrl ph, int tab)
{
	int saved;

	if (!ph || tab < 0 || tab >= SA_TAB_COUNT)
	{
		return;
	}

	saved = 0;
	if (sleep_aid_tab_is_voice_list(tab))
	{
		ph->list_focus_idx = 0;
		saved = ph->list_focus_idx;
	}
	else if (tab == SA_TAB_DIY)
	{
		ph->diy_focus = SA_DIY_FOCUS_CH0;
		ph->diy_sel_ch = 0;
		ph->diy_edit_active = 0;
		saved = ph->diy_focus;
	}
	else if (tab == SA_TAB_RECORD)
	{
		ph->record_focus = 0;
		ph->record_elapsed_sec = 0;
		ph->record_ui_busy = 0;
		saved = ph->record_focus;
	}
	else if (tab == SA_TAB_SETTINGS)
	{
		ph->sett_row = 0;
		sleep_aid_sett_clear_edit(ph);
		saved = ph->sett_row;
	}
	ph->tab_saved_focus[tab] = saved;
}

static void sleep_aid_leave_subpage(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	if (!ph || ph->tabs_focus)
	{
		return;
	}

	sleep_aid_record_stop_refresh_timer(ph);
	ph->record_elapsed_sec = 0;
	ph->record_ui_busy = 0;
	ph->diy_edit_active = 0;
	sleep_aid_sett_clear_edit(ph);
	sleep_aid_reset_tab_content_focus(ph, ph->tab_idx);
}

/** 退出菜单：释放 TAB 内容、缓存与异步任务，避免再次进入沿用旧状态。 */
static void sleep_aid_teardown_session(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int i;

	if (!ph)
	{
		return;
	}

	sleep_aid_shutdown_async_work(ph);
	sleep_aid_add_dl_cancel(ph);
	if (ph->mbox)
	{
		divoom_lvgl_message_box_destroy(ph->mbox, 0);
		ph->mbox = NULL;
	}
	for (i = 0; i < SA_TAB_COUNT; i++)
	{
		sleep_aid_invalidate_tab(ph, i);
		ph->tab_saved_focus[i] = 0;
	}
	ph->last_shown_tab = -1;
	ph->tab_idx = SA_TAB_FAV;
	ph->tabs_focus = 1;
	ph->list_focus_idx = 0;
	ph->diy_focus = SA_DIY_FOCUS_CH0;
	ph->diy_sel_ch = 0;
	ph->diy_edit_active = 0;
	ph->record_focus = 0;
	ph->record_elapsed_sec = 0;
	ph->record_ui_busy = 0;
	ph->sett_row = 0;
	sleep_aid_sett_end_edit_preview(ph);
	ph->sett_edit_active = 0;
	ph->sett_preview_kind = SA_SETT_PREVIEW_NONE;
	ph->fav_tab_stale = 0;
	sleep_aid_detach_list_model_from_ph(ph);
	sleep_aid_clear_list_model(ph);
	sleep_aid_diy_clear_ptrs(ph);
	sleep_aid_record_clear_ptrs(ph);
	sleep_aid_sett_clear_ptrs(ph);
	sleep_aid_voice_store_clear_all(ph);
	sleep_aid_fav_cache_free(ph);
}

/** 退出菜单时停止下载与录音定时器，避免 lv_timer/后台任务访问已释放的 ph */
static void sleep_aid_shutdown_async_work(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int i;

	if (!ph)
	{
		return;
	}

	ph->add_dl_cancel = 1;
	for (i = 0; i < SA_ADD_DL_MAX; i++)
	{
		ph->add_dl_slots[i].ph = NULL;
	}
	if (ph->add_dl_timer)
	{
		lv_timer_del(ph->add_dl_timer);
		ph->add_dl_timer = NULL;
	}
	if (ph->add_dl_defer_timer)
	{
		lv_timer_del(ph->add_dl_defer_timer);
		ph->add_dl_defer_timer = NULL;
	}
	sleep_aid_record_stop_refresh_timer(ph);
}

static void sleep_aid_add_dl_cancel(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int i;

	if (!ph)
	{
		return;
	}

	ph->add_dl_cancel = 1;
	for (i = 0; i < SA_ADD_DL_MAX; i++)
	{
		ph->add_dl_slots[i].ph = NULL;
	}
	if (ph->add_dl_timer)
	{
		lv_timer_del(ph->add_dl_timer);
		ph->add_dl_timer = NULL;
	}
	if (ph->add_dl_defer_timer)
	{
		lv_timer_del(ph->add_dl_defer_timer);
		ph->add_dl_defer_timer = NULL;
	}
	for (i = 0; i < SA_ADD_DL_MAX; i++)
	{
		ph->add_dl_slots[i].voice_id = 0;
		ph->add_dl_slots[i].file_id[0] = '\0';
		ph->add_dl_slots[i].local_path[0] = '\0';
		ph->add_dl_slots[i].read_len = 0;
		ph->add_dl_slots[i].tol_len = 0;
		ph->add_dl_slots[i].done = 0;
		ph->add_dl_slots[i].success = 0;
		ph->add_dl_slots[i].pct_lbl = NULL;
		ph->add_dl_slots[i].spinner_arc = NULL;
	}
	ph->add_dl_count = 0;
	ph->add_dl_task_done = 0;
	ph->add_dl_spin_phase = 0;
	ph->add_dl_cancel = 0;
}

static void sleep_aid_list_focus_new_downloads(Pdivoom_lvgl_sleep_aid_ctrl ph, const uint32 *voice_ids, int n)
{
	int i;
	int j;
	int best;

	if (!ph || !voice_ids || n <= 0 || !ph->list_ids || ph->list_count <= 0)
	{
		return;
	}

	best = -1;
	for (j = 0; j < n; j++)
	{
		if (voice_ids[j] == 0)
		{
			continue;
		}
		for (i = 0; i < ph->list_count; i++)
		{
			if (ph->list_ids[i] == voice_ids[j])
			{
				if (best < 0 || i < best)
				{
					best = i;
				}
				break;
			}
		}
	}
	if (best >= 0)
	{
		ph->list_focus_idx = best;
		ph->tabs_focus = 0;
	}
}

static int sleep_aid_add_dl_voice_row(Pdivoom_lvgl_sleep_aid_ctrl ph, uint32 voice_id)
{
	int i;

	if (!ph || voice_id == 0 || ph->add_dl_count == 0)
	{
		return 0;
	}
	for (i = 0; i < (int)ph->add_dl_count; i++)
	{
		if (ph->add_dl_slots[i].voice_id == voice_id && !ph->add_dl_slots[i].done)
		{
			return 1;
		}
	}
	return 0;
}

static void sleep_aid_add_dl_progress_cb(void *user_data, uint32 read_len, uint32 tol_len)
{
	sleep_aid_add_dl_slot_t *sl = (sleep_aid_add_dl_slot_t *)user_data;

	if (!sl || !sl->ph || sl->ph->add_dl_cancel)
	{
		return;
	}
	sl->read_len = read_len;
	sl->tol_len = tol_len;
}

static void *sleep_aid_add_dl_slot_task(void *param)
{
	sleep_aid_add_dl_slot_t *sl = (sleep_aid_add_dl_slot_t *)param;
	Pdivoom_lvgl_sleep_aid_ctrl ph;
	int ok;

	if (!sl || !sl->ph)
	{
		return NULL;
	}

	ph = sl->ph;
	if (!ph || ph->add_dl_cancel || !sl->file_id[0])
	{
		SA_ADD_DL_LOG("dl_task skip vid=%u cancel=%u file=%s\n",
			      (unsigned)sl->voice_id, ph ? (unsigned)ph->add_dl_cancel : 99u,
			      sl->file_id[0] ? sl->file_id : "(empty)");
		sl->done = 1;
		sl->success = 0;
		return NULL;
	}

	divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_SLEEP_AID_PIC, sl->voice_id, sl->local_path);
	SA_ADD_DL_LOG("dl_task start vid=%u path=%s file_id=%s\n",
		      (unsigned)sl->voice_id, sl->local_path, sl->file_id);
	ok = divoom_http_client_down_file_no_delay(sl->local_path, sl->file_id, 1, NULL,
						   sleep_aid_add_dl_progress_cb, sl);
	if (!sl->ph)
	{
		SA_ADD_DL_LOG("dl_task aborted vid=%u (ph gone)\n", (unsigned)sl->voice_id);
		return NULL;
	}
	sl->success = ok ? (uint8)1 : (uint8)0;
	sl->done = 1;
	SA_ADD_DL_LOG("dl_task done vid=%u ok=%u\n", (unsigned)sl->voice_id, (unsigned)sl->success);
	return NULL;
}

static void sleep_aid_add_dl_check_all_done(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int i;
	int done_n;

	if (!ph || ph->add_dl_count == 0)
	{
		return;
	}

	done_n = 0;
	for (i = 0; i < (int)ph->add_dl_count; i++)
	{
		if (ph->add_dl_slots[i].done)
		{
			done_n++;
		}
	}
	if (done_n < (int)ph->add_dl_count)
	{
		return;
	}

	ph->add_dl_task_done = 1;
	SA_ADD_DL_LOG("all_done count=%u\n", (unsigned)ph->add_dl_count);
}

static void sleep_aid_add_dl_timer_cb(lv_timer_t *t)
{
	Pdivoom_lvgl_sleep_aid_ctrl ph;
	int i;
	int pct;

	ph = (Pdivoom_lvgl_sleep_aid_ctrl)lv_timer_get_user_data(t);
	if (!ph || ph->add_dl_cancel)
	{
		return;
	}

	ph->add_dl_spin_phase = (uint8)((ph->add_dl_spin_phase + 30) % 360);
	for (i = 0; i < (int)ph->add_dl_count; i++)
	{
		sleep_aid_add_dl_slot_t *sl = &ph->add_dl_slots[i];
		char buf[16];

		if (sl->spinner_arc)
		{
			uint16_t start = ph->add_dl_spin_phase;
			uint16_t end = (uint16_t)((start + 270) % 360);

			lv_arc_set_angles(sl->spinner_arc, start, end);
		}
		if (sl->pct_lbl)
		{
			if (sl->tol_len > 0)
			{
				pct = (int)((unsigned long long)sl->read_len * 100 / sl->tol_len);
				if (pct > 100)
				{
					pct = 100;
				}
			}
			else
			{
				pct = 0;
			}
			snprintf(buf, sizeof(buf), "%d%%", pct);
			lv_label_set_text(sl->pct_lbl, buf);
		}
	}

	sleep_aid_add_dl_check_all_done(ph);
	if (ph->add_dl_task_done)
	{
		uint32 saved_voice_ids[SA_ADD_DL_MAX];
		int saved_n;
		int si;

		for (si = 0; si < (int)ph->add_dl_count; si++)
		{
			SA_ADD_DL_LOG("result vid=%u success=%u\n",
				      (unsigned)ph->add_dl_slots[si].voice_id,
				      (unsigned)ph->add_dl_slots[si].success);
		}

		saved_n = (int)ph->add_dl_count;
		if (saved_n > SA_ADD_DL_MAX)
		{
			saved_n = SA_ADD_DL_MAX;
		}
		for (si = 0; si < saved_n; si++)
		{
			saved_voice_ids[si] = ph->add_dl_slots[si].voice_id;
		}

		if (ph->add_dl_timer)
		{
			lv_timer_del(ph->add_dl_timer);
			ph->add_dl_timer = NULL;
		}
		sleep_aid_add_dl_cancel(ph);
		sleep_aid_invalidate_tab(ph, ph->tab_idx);
		sleep_aid_refresh_content_force(ph, 0);
		sleep_aid_list_focus_new_downloads(ph, saved_voice_ids, saved_n);
		sleep_aid_paint_focus(ph);
	}
}

static lv_obj_t *sleep_aid_add_dl_create_row(Pdivoom_lvgl_sleep_aid_ctrl ph, lv_coord_t y, int slot_idx)
{
	lv_obj_t *row;
	lv_obj_t *lb;
	lv_obj_t *pct;
	lv_obj_t *arc;
	sleep_aid_add_dl_slot_t *sl;

	if (!ph || slot_idx < 0 || slot_idx >= SA_ADD_DL_MAX)
	{
		return NULL;
	}

	sl = &ph->add_dl_slots[slot_idx];
	row = lv_obj_create(sleep_aid_list_parent(ph));
	lv_obj_add_style(row, &ph->row_style, 0);
	lv_obj_set_pos(row, 0, y);
	lv_obj_set_size(row, SA_LIST_ROW_W, SA_ROW_H);
	lv_obj_set_style_radius(row, SA_ROW_RADIUS, 0);
	lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

	arc = lv_arc_create(row);
	lv_obj_set_size(arc, SA_DL_SPINNER_SIZE, SA_DL_SPINNER_SIZE);
	lv_obj_align(arc, LV_ALIGN_LEFT_MID, SA_DL_SPINNER_LEFT, 0);
	lv_arc_set_bg_angles(arc, 0, 360);
	lv_arc_set_angles(arc, 0, 270);
	lv_arc_set_rotation(arc, 0);
	lv_arc_set_value(arc, 0);
	lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
	lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_arc_color(arc, lv_color_hex(SA_DL_SPINNER_COLOR), LV_PART_INDICATOR);
	lv_obj_set_style_arc_width(arc, 3, LV_PART_INDICATOR);
	lv_obj_set_style_arc_color(arc, lv_color_hex(0xD1D1D1), LV_PART_MAIN);
	lv_obj_set_style_arc_width(arc, 3, LV_PART_MAIN);
	sl->spinner_arc = arc;

	lb = lv_label_create(row);
	lv_obj_set_style_text_font(lb, divoom_menu_control_get_font_info(20, ph->menu_id), 0);
	lv_obj_set_style_text_color(lb, lv_color_hex(0x000000), 0);
	lv_label_set_text(lb, divoom_menu_control_get_disp_str("SleepAidLoading"));
	lv_obj_align(lb, LV_ALIGN_LEFT_MID, SA_DL_LOADING_LABEL_X, 0);
	lv_label_set_long_mode(lb, LV_LABEL_LONG_DOT);
	lv_obj_set_width(lb, SA_LIST_ROW_W - SA_DL_LOADING_LABEL_X - 80);

	pct = lv_label_create(row);
	lv_obj_set_style_text_font(pct, divoom_menu_control_get_font_info(20, ph->menu_id), 0);
	lv_obj_set_style_text_color(pct, lv_color_hex(0x000000), 0);
	lv_label_set_text(pct, "0%");
	lv_obj_set_style_text_align(pct, LV_TEXT_ALIGN_RIGHT, 0);
	lv_obj_set_width(pct, 80);
	lv_obj_align(pct, LV_ALIGN_RIGHT_MID, -SA_DL_PCT_RIGHT_PAD, 0);
	sl->pct_lbl = pct;

	return row;
}

static void sleep_aid_add_dl_begin(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int aid_type;
	uint32 voice_ids[SA_ADD_DL_MAX];
	char file_ids[SA_ADD_DL_MAX][56];
	int n;
	int i;

	if (!ph)
	{
		return;
	}

	aid_type = sleep_aid_tab_aid_type(ph->tab_idx);
	if (aid_type < 0)
	{
		return;
	}
	if (ph->tab_idx != SA_TAB_NOISE && ph->tab_idx != SA_TAB_WHITE && ph->tab_idx != SA_TAB_MUSIC)
	{
		return;
	}
	if (ph->add_dl_count > 0)
	{
		return;
	}

	n = divoom_sleep_aid_voice_pick_pending_add(aid_type, voice_ids, file_ids, SA_ADD_DL_MAX);
	if (n <= 0)
	{
		sleep_aid_send_get_all_three_types();
		sleep_aid_invalidate_tab(ph, ph->tab_idx);
		sleep_aid_refresh_content_force(ph, 0);
		return;
	}

	sleep_aid_add_dl_cancel(ph);
	ph->add_dl_count = (uint8)n;
	ph->add_dl_task_done = 0;
	ph->add_dl_spin_phase = 0;
	for (i = 0; i < n; i++)
	{
		ph->add_dl_slots[i].ph = ph;
		ph->add_dl_slots[i].voice_id = voice_ids[i];
		strncpy(ph->add_dl_slots[i].file_id, file_ids[i], sizeof(ph->add_dl_slots[i].file_id) - 1);
		ph->add_dl_slots[i].file_id[sizeof(ph->add_dl_slots[i].file_id) - 1] = '\0';
		ph->add_dl_slots[i].read_len = 0;
		ph->add_dl_slots[i].tol_len = 0;
		ph->add_dl_slots[i].done = 0;
		ph->add_dl_slots[i].success = 0;
		ph->add_dl_slots[i].pct_lbl = NULL;
		ph->add_dl_slots[i].spinner_arc = NULL;
		divoom_system_create_task(sleep_aid_add_dl_slot_task, "sleep_aid", &ph->add_dl_slots[i], DIVOOM_TASK_STACK_SLEEP_AID_DL);
	}

	sleep_aid_invalidate_tab(ph, ph->tab_idx);
	sleep_aid_refresh_content_force(ph, 0);
	sleep_aid_list_focus_new_downloads(ph, voice_ids, n);
	if (!ph->add_dl_timer)
	{
		ph->add_dl_timer = lv_timer_create(sleep_aid_add_dl_timer_cb, 100, ph);
	}
	SA_ADD_DL_LOG("begin ok: started %d downloads, timer=%p\n", n, (void *)ph->add_dl_timer);
	sleep_aid_paint_focus(ph);
}

static void sleep_aid_add_dl_deferred_cb(lv_timer_t *t)
{
	Pdivoom_lvgl_sleep_aid_ctrl ph;

	ph = (Pdivoom_lvgl_sleep_aid_ctrl)lv_timer_get_user_data(t);
	if (ph && ph->add_dl_defer_timer == t)
	{
		ph->add_dl_defer_timer = NULL;
	}
	lv_timer_del(t);
	if (!ph || ph->add_dl_cancel)
	{
		SA_ADD_DL_LOG("defer_cb skip ph=%p cancel=%u\n", (void *)ph,
			      ph ? (unsigned)ph->add_dl_cancel : 99u);
		return;
	}
	SA_ADD_DL_LOG("defer_cb -> begin\n");
	sleep_aid_add_dl_begin(ph);
}

static void sleep_aid_load_list_model(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int aid_type;

	sleep_aid_clear_list_model(ph);
	if (!sleep_aid_tab_is_voice_list(ph->tab_idx))
	{
		return;
	}

	aid_type = sleep_aid_tab_aid_type(ph->tab_idx);
	if (aid_type < 0)
	{
		return;
	}

	{
		int n1 = 0;
		int n0 = 0;
		int i;
		divoom_sleep_aid_voice_list_item *entries;
		divoom_sleep_aid_voice_list_item *pending;

		entries = divoom_sleep_aid_voice_get_list(aid_type, &n1, 1);
		ph->list_count = n1;
		divoom_print("load: %d, %d\n", aid_type, n1);

		if (entries && n1 > 0)
		{
			ph->list_ids = (uint32 *)divoom_system_malloc((unsigned)n1 * sizeof(uint32));
			ph->list_types = (int *)divoom_system_malloc((unsigned)n1 * sizeof(int));
			ph->list_video_types = (int *)divoom_system_malloc((unsigned)n1 * sizeof(int));
			ph->list_names = (char *)divoom_system_malloc((unsigned)n1 * DIVOOM_SLEEP_AID_VOICE_NAME_LEN);
			if (ph->list_ids && ph->list_types && ph->list_video_types && ph->list_names)
			{
				for (i = 0; i < n1; i++)
				{
					ph->list_ids[i] = entries[i].audio_id;
					ph->list_types[i] = entries[i].audio_type;
					ph->list_video_types[i] = entries[i].video_type;
					strncpy(ph->list_names + i * DIVOOM_SLEEP_AID_VOICE_NAME_LEN, entries[i].name,
						DIVOOM_SLEEP_AID_VOICE_NAME_LEN - 1);
					ph->list_names[i * DIVOOM_SLEEP_AID_VOICE_NAME_LEN + DIVOOM_SLEEP_AID_VOICE_NAME_LEN - 1] = '\0';
				}
			}
			else
			{
				sleep_aid_clear_list_model(ph);
			}
		}
		divoom_system_free(entries);

		if (ph->tab_idx == SA_TAB_NOISE || ph->tab_idx == SA_TAB_WHITE || ph->tab_idx == SA_TAB_MUSIC)
		{
			pending = divoom_sleep_aid_voice_get_list(aid_type, &n0, 0);
			ph->list_has_plus = (ph->add_dl_count > 0) ? (uint8)0
				: ((pending && n0 > 0) ? (uint8)1 : (uint8)0);
			SA_ADD_DL_LOG("load_model tab=%d aid=%d shown=%d pending=%d has_plus=%u add_dl=%u\n",
				      ph->tab_idx, aid_type, n1, n0, (unsigned)ph->list_has_plus,
				      (unsigned)ph->add_dl_count);
			divoom_system_free(pending);
		}
	}
}

static void sleep_aid_send_get_all_three_types(void)
{
	divoom_sleep_aid_send_get_all_list();
}

static int sleep_aid_list_total_rows(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	return ph->list_count + (ph->list_has_plus ? 1 : 0);
}

static void sleep_aid_list_rows_free(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	if (!ph)
	{
		return;
	}
	if (ph->list_row_objs)
	{
		divoom_system_free(ph->list_row_objs);
	}
	ph->list_row_objs = NULL;
	ph->list_row_cap = 0;
	ph->list_row_paint_idx = -1;
}

static int sleep_aid_list_rows_alloc(Pdivoom_lvgl_sleep_aid_ctrl ph, int rows)
{
	lv_obj_t **buf;

	if (!ph || rows <= 0)
	{
		sleep_aid_list_rows_free(ph);
		return 0;
	}
	sleep_aid_list_rows_free(ph);
	buf = (lv_obj_t **)divoom_system_malloc((unsigned)rows * sizeof(lv_obj_t *));
	if (!buf)
	{
		return -1;
	}
	ph->list_row_objs = buf;
	ph->list_row_cap = rows;
	ph->list_row_paint_idx = -1;
	return 0;
}

static void sleep_aid_list_row_set(Pdivoom_lvgl_sleep_aid_ctrl ph, int row_idx, lv_obj_t *row)
{
	if (!ph || !ph->list_row_objs || row_idx < 0 || row_idx >= ph->list_row_cap)
	{
		return;
	}
	ph->list_row_objs[row_idx] = row;
}

static lv_obj_t *sleep_aid_list_row_get(Pdivoom_lvgl_sleep_aid_ctrl ph, int row_idx)
{
	if (!ph || row_idx < 0)
	{
		return NULL;
	}
	if (ph->list_row_objs && row_idx < ph->list_row_cap)
	{
		return ph->list_row_objs[row_idx];
	}
	return NULL;
}

static void sleep_aid_apply_tab_visuals_delta(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int cur;
	int prev;

	if (!ph)
	{
		return;
	}
	cur = ph->tab_idx;
	prev = ph->last_shown_tab;
	sleep_aid_apply_tab_visual(ph, cur);
	if (prev >= 0 && prev < SA_TAB_COUNT && prev != cur)
	{
		sleep_aid_apply_tab_visual(ph, prev);
	}
}

static void sleep_aid_paint_voice_list_row(Pdivoom_lvgl_sleep_aid_ctrl ph, int row_idx, uint8 row_focused)
{
	lv_obj_t *row;
	int vt;
	uint8 is_plus;
	uint8 is_fav;
	uint8 is_play;

	row = sleep_aid_list_row_get(ph, row_idx);
	if (!row)
	{
		return;
	}
	is_plus = (ph->list_has_plus && row_idx == ph->list_count) ? (uint8)1 : (uint8)0;
	if (is_plus)
	{
		sleep_aid_set_row_focus_border(row, row_focused);
		return;
	}
	if (row_idx < ph->list_count && sleep_aid_add_dl_voice_row(ph, ph->list_ids[row_idx]))
	{
		sleep_aid_set_row_focus_border(row, row_focused);
		return;
	}
	sleep_aid_set_row_focus_border(row, row_focused);
	is_play = row_focused;
	sleep_aid_row_set_state_icon(row, is_play);
	if (row_idx < ph->list_count)
	{
		vt = ph->list_types ? ph->list_types[row_idx] : sleep_aid_tab_aid_type(ph->tab_idx);
		is_fav = (ph->tab_idx == SA_TAB_FAV) ? (uint8)1 : (uint8)sleep_aid_row_is_fav(ph, ph->list_ids[row_idx], vt);
		sleep_aid_row_set_fav_icon(row, is_fav);
	}
}

static void sleep_aid_paint_voice_list_focus(Pdivoom_lvgl_sleep_aid_ctrl ph, uint8 full_refresh)
{
	int i;
	int rows;
	int old_idx;
	int new_idx;
	lv_obj_t *focus_row;

	if (!ph || !sleep_aid_tab_is_voice_list(ph->tab_idx))
	{
		return;
	}

	rows = sleep_aid_list_total_rows(ph);
	old_idx = ph->list_row_paint_idx;
	new_idx = ph->list_focus_idx;

	if (full_refresh || !ph->list_row_objs)
	{
		for (i = 0; i < rows; i++)
		{
			uint8 focused;

			focused = (!ph->tabs_focus && i == new_idx) ? (uint8)1 : (uint8)0;
			sleep_aid_paint_voice_list_row(ph, i, focused);
		}
	}
	else if (ph->tabs_focus)
	{
		if (old_idx >= 0 && old_idx < rows)
		{
			sleep_aid_paint_voice_list_row(ph, old_idx, 0);
		}
	}
	else
	{
		if (old_idx >= 0 && old_idx < rows && old_idx != new_idx)
		{
			sleep_aid_paint_voice_list_row(ph, old_idx, 0);
		}
		if (new_idx >= 0 && new_idx < rows)
		{
			sleep_aid_paint_voice_list_row(ph, new_idx, 1);
		}
	}

	if (ph->tabs_focus)
	{
		ph->list_row_paint_idx = -1;
	}
	else
	{
		ph->list_row_paint_idx = new_idx;
	}

	focus_row = sleep_aid_list_row_get(ph, new_idx);
	if (focus_row && !ph->tabs_focus)
	{
		lv_obj_scroll_to_view(focus_row, LV_ANIM_OFF);
	}
}

static void sleep_aid_set_row_focus_border(lv_obj_t *row, uint8 focused)
{
	if (!row)
	{
		return;
	}

	if (focused)
	{
		lv_obj_set_style_border_width(row, 2, 0);
		lv_obj_set_style_border_color(row, lv_color_hex(SA_ROW_FOCUS_BORDER_COLOR), 0);
		lv_obj_set_style_border_side(row, LV_BORDER_SIDE_FULL, 0);
		lv_obj_set_style_border_opa(row, LV_OPA_COVER, 0);
		lv_obj_set_style_outline_width(row, 0, 0);
		lv_obj_set_style_shadow_width(row, 0, 0);
		lv_obj_set_style_radius(row, SA_ROW_RADIUS, 0);
	}
	else
	{
		lv_obj_set_style_border_width(row, 0, 0);
		lv_obj_set_style_radius(row, SA_ROW_RADIUS, 0);
	}
}

static void sleep_aid_fav_cache_free(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	if (!ph)
	{
		return;
	}
	if (ph->fav_cache)
	{
		divoom_system_free(ph->fav_cache);
	}
	ph->fav_cache = NULL;
	ph->fav_cache_count = 0;
}

static void sleep_aid_fav_cache_reload(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int n;

	if (!ph)
	{
		return;
	}

	sleep_aid_fav_cache_free(ph);
	ph->fav_cache = divoom_sleep_aid_fav_get_list(&n);
	ph->fav_cache_count = (ph->fav_cache && n > 0) ? n : 0;
}

/** 收藏变更：刷新内存缓存并标记收藏 TAB 需重建（其它 TAB 收藏后切到收藏可立即看到）。 */
static void sleep_aid_fav_list_changed(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	if (!ph)
	{
		return;
	}
	sleep_aid_fav_cache_reload(ph);
	ph->fav_tab_stale = 1;
}

/** 当前列表中查找收藏项下标（取消收藏前调用）。 */
static int sleep_aid_fav_find_list_index(Pdivoom_lvgl_sleep_aid_ctrl ph, uint32 vid, int vt)
{
	int i;

	if (!ph || !ph->list_ids || !ph->list_types || vid == 0)
	{
		return -1;
	}
	for (i = 0; i < ph->list_count; i++)
	{
		if (ph->list_ids[i] == vid && ph->list_types[i] == vt)
		{
			return i;
		}
	}
	return -1;
}

/**
 * 收藏 TAB 增删后：清空列表容器并整表重建（禁止走 tab 缓存刷新，避免旧行残留）。
 * @param removed_idx 取消收藏项在旧列表中的下标；新增时传 -1。
 */
static void sleep_aid_fav_tab_reload_ui(Pdivoom_lvgl_sleep_aid_ctrl ph, int removed_idx)
{
	int tab;
	int focus;
	int rows;
	lv_obj_t *panel;

	if (!ph)
	{
		return;
	}
	tab = SA_TAB_FAV;
	sleep_aid_fav_list_changed(ph);
	ph->fav_tab_stale = 0;

	if (ph->tab_idx != tab)
	{
		return;
	}

	focus = ph->list_focus_idx;
	if (removed_idx >= 0)
	{
		if (removed_idx < focus)
		{
			focus--;
		}
		if (focus < 0)
		{
			focus = 0;
		}
	}

	sleep_aid_invalidate_tab(ph, tab);
	panel = ph->tab_panel[tab];
	if (panel)
	{
		lv_obj_clean(panel);
	}
	sleep_aid_list_rows_free(ph);
	ph->tab_built[tab] = 0;

	sleep_aid_load_list_model(ph);
	rows = sleep_aid_list_total_rows(ph);
	if (rows <= 0)
	{
		focus = 0;
	}
	else if (focus >= rows)
	{
		focus = rows - 1;
	}
	ph->list_focus_idx = focus;
	ph->tab_saved_focus[tab] = focus;

	sleep_aid_build_voice_list_ui(ph);
	sleep_aid_voice_store_commit(ph, tab);
	ph->tab_built[tab] = 1;

	sleep_aid_apply_tab_visuals_delta(ph);
	sleep_aid_apply_list_voice(ph);
	sleep_aid_paint_voice_list_focus(ph, 1);
}

static int sleep_aid_row_is_fav(Pdivoom_lvgl_sleep_aid_ctrl ph, uint32 vid, int vt)
{
	int i;

	if (!ph || vid == 0)
	{
		return 0;
	}
	if (!ph->fav_cache || ph->fav_cache_count <= 0)
	{
		return 0;
	}
	for (i = 0; i < ph->fav_cache_count; i++)
	{
		if ((uint32)ph->fav_cache[i].audio_id == vid && ph->fav_cache[i].audio_type == vt)
		{
			return 1;
		}
	}
	return 0;
}

static lv_obj_t *sleep_aid_row_state_img(lv_obj_t *row)
{
	if (!row)
	{
		return NULL;
	}
	return lv_obj_get_child(row, 0);
}

static lv_obj_t *sleep_aid_row_fav_img(lv_obj_t *row)
{
	uint32_t n;

	if (!row)
	{
		return NULL;
	}
	n = lv_obj_get_child_cnt(row);
	if (n == 0)
	{
		return NULL;
	}
	return lv_obj_get_child(row, n - 1);
}

static void sleep_aid_row_set_state_icon(lv_obj_t *row, uint8 is_play)
{
	lv_obj_t *img;

	img = sleep_aid_row_state_img(row);
	if (!img)
	{
		return;
	}
	divoom_menu_control_lv_img_set_src(img, is_play ? SA_ROW_IMG_PLAY : SA_ROW_IMG_NORMAL);
}

static void sleep_aid_row_set_fav_icon(lv_obj_t *row, uint8 is_fav)
{
	lv_obj_t *img;

	img = sleep_aid_row_fav_img(row);
	if (!img)
	{
		return;
	}
	divoom_menu_control_lv_img_set_src(img, is_fav ? SA_ROW_IMG_FAV : SA_ROW_IMG_UNFAV);
}

static void sleep_aid_update_voice_list_row_icons(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int i;
	int rows;

	if (!ph || !sleep_aid_tab_is_voice_list(ph->tab_idx))
	{
		return;
	}

	rows = sleep_aid_list_total_rows(ph);
	for (i = 0; i < rows; i++)
	{
		lv_obj_t *row;
		int vt;
		uint8 is_plus;
		uint8 is_fav;
		uint8 is_play;

		row = sleep_aid_list_row_get(ph, i);
		if (!row)
		{
			continue;
		}

		is_plus = (ph->list_has_plus && i == ph->list_count) ? (uint8)1 : (uint8)0;
		if (is_plus)
		{
			continue;
		}
		if (i < ph->list_count && sleep_aid_add_dl_voice_row(ph, ph->list_ids[i]))
		{
			continue;
		}

		is_play = (!ph->tabs_focus && i == ph->list_focus_idx) ? (uint8)1 : (uint8)0;
		sleep_aid_row_set_state_icon(row, is_play);

		if (i < ph->list_count)
		{
			vt = ph->list_types ? ph->list_types[i] : sleep_aid_tab_aid_type(ph->tab_idx);
			is_fav = (ph->tab_idx == SA_TAB_FAV) ? (uint8)1 : (uint8)sleep_aid_row_is_fav(ph, ph->list_ids[i], vt);
			sleep_aid_row_set_fav_icon(row, is_fav);
		}
	}
}

/** 焦点在右侧音源列表某一行（非侧栏 TAB、非「+」、非下载中占位行）。 */
static int sleep_aid_list_voice_focus_active(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int rows;

	if (!ph || ph->tabs_focus)
	{
		return 0;
	}
	if (!sleep_aid_tab_is_voice_list(ph->tab_idx))
	{
		return 0;
	}
	rows = sleep_aid_list_total_rows(ph);
	if (rows <= 0 || !ph->list_ids || !ph->list_types)
	{
		return 0;
	}
	if (ph->list_focus_idx < 0 || ph->list_focus_idx >= ph->list_count)
	{
		return 0;
	}
	if (ph->list_has_plus && ph->list_focus_idx == ph->list_count)
	{
		return 0;
	}
	if (sleep_aid_add_dl_voice_row(ph, ph->list_ids[ph->list_focus_idx]))
	{
		return 0;
	}
	return 1;
}

static void sleep_aid_stop_list_preview(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	(void)ph;
	divoom_sleep_aid_ctrl_stop_play_voice();
}

static void sleep_aid_apply_list_voice(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	if (!sleep_aid_list_voice_focus_active(ph))
	{
		return;
	}

	divoom_sleep_aid_ctrl_select_voice(ph->list_types[ph->list_focus_idx], (int)ph->list_ids[ph->list_focus_idx]);
}

/** 焦点在录音列表有效行（非侧栏、非底部录音按钮、非录音中）。 */
static int sleep_aid_record_voice_focus_active(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int clips;

	if (!ph || ph->tabs_focus || ph->tab_idx != SA_TAB_RECORD)
	{
		return 0;
	}
	if (divoom_sleep_aid_ctrl_record_busy())
	{
		return 0;
	}
	clips = divoom_sleep_aid_ctrl_record_clip_count();
	if (ph->record_focus >= 0 && ph->record_focus < clips)
	{
		return 1;
	}
	return 0;
}

static void sleep_aid_apply_record_voice(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int voice_id;

	if (!sleep_aid_record_voice_focus_active(ph))
	{
		return;
	}
	voice_id = divoom_sleep_aid_ctrl_record_clip_id(ph->record_focus);
	if (voice_id < 0)
	{
		return;
	}
	divoom_sleep_aid_ctrl_select_voice(DIVOOM_SLEEP_AID_TYPE_RECORD, voice_id);
}

/** 焦点在 DIY 音效调节通道行（非侧栏、非预览/收藏按钮）。 */
static int sleep_aid_diy_ch_focus_active(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	if (!ph || ph->tabs_focus || ph->tab_idx != SA_TAB_DIY)
	{
		return 0;
	}
	if (ph->diy_focus >= SA_DIY_FOCUS_CH0 && ph->diy_focus <= SA_DIY_FOCUS_CH_LAST)
	{
		return 1;
	}
	return 0;
}

static void sleep_aid_apply_diy_voice(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	if (!sleep_aid_diy_ch_focus_active(ph))
	{
		return;
	}
	divoom_sleep_aid_ctrl_select_voice(DIVOOM_SLEEP_AID_TYPE_DIY, (int)SA_DIY_MIX_VOICE_ID);
}

static void sleep_aid_apply_tab_visual(Pdivoom_lvgl_sleep_aid_ctrl ph, int idx)
{
	lv_obj_t *tab;
	uint8 is_active;

	if (idx < 0 || idx >= SA_TAB_COUNT)
	{
		return;
	}

	tab = ph->tab_objs[idx];
	if (!tab)
	{
		return;
	}

	is_active = (idx == ph->tab_idx) ? (uint8)1 : (uint8)0;
	lv_obj_remove_style(tab, &ph->tab_sel_style, 0);
	lv_obj_remove_style(tab, &ph->tab_cell_style, 0);
	if (is_active)
	{
		lv_obj_add_style(tab, &ph->tab_sel_style, 0);
	}
	else
	{
		lv_obj_add_style(tab, &ph->tab_cell_style, 0);
	}

	if (ph->tab_text_lbl[idx])
	{
		lv_obj_set_style_text_color(ph->tab_text_lbl[idx], lv_color_hex(SA_COL_TAB_TEXT), 0);
	}
	if (ph->tab_arrow_lbl[idx])
	{
		lv_obj_set_style_text_color(ph->tab_arrow_lbl[idx], lv_color_hex(SA_COL_TAB_ARROW), 0);
	}

	if (ph->tabs_focus && is_active)
	{
		lv_obj_set_style_outline_width(tab, 2, LV_STATE_DEFAULT);
		lv_obj_set_style_outline_color(tab, lv_color_hex(0x000000), LV_STATE_DEFAULT);
		lv_obj_set_style_outline_pad(tab, 0, LV_STATE_DEFAULT);
	}
	else
	{
		lv_obj_set_style_outline_width(tab, 0, LV_STATE_DEFAULT);
	}
}

static void sleep_aid_paint_focus(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	lv_obj_t *focus_row;

	if (!ph)
	{
		return;
	}

	sleep_aid_apply_tab_visuals_delta(ph);

	if (!sleep_aid_tab_is_voice_list(ph->tab_idx))
	{
		if (ph->tab_idx == SA_TAB_DIY)
		{
			sleep_aid_diy_apply_visual(ph);
			focus_row = sleep_aid_diy_focus_scroll_target(ph);
			if (focus_row && !ph->tabs_focus)
			{
				lv_obj_scroll_to_view(focus_row, LV_ANIM_OFF);
			}
			return;
		}
		else if (ph->tab_idx == SA_TAB_SETTINGS)
		{
			sleep_aid_sett_paint_focus(ph);
			if (!ph->tabs_focus && ph->sett_row >= 0 && ph->sett_row < ph->sett_row_count)
			{
				focus_row = ph->sett_row_objs[ph->sett_row];
				if (focus_row)
				{
					lv_obj_scroll_to_view(focus_row, LV_ANIM_OFF);
				}
			}
			return;
		}
		else if (ph->tab_idx == SA_TAB_RECORD)
		{
			sleep_aid_record_paint_focus(ph);
			focus_row = sleep_aid_record_focus_obj(ph);
			if (focus_row)
			{
				lv_obj_scroll_to_view(focus_row, LV_ANIM_OFF);
			}
			return;
		}
		return;
	}

	sleep_aid_paint_voice_list_focus(ph, 0);
}

static void sleep_aid_build_voice_list_ui(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	lv_coord_t y = SA_LIST_ROW_START_Y;
	char buf[DIVOOM_SLEEP_AID_VOICE_NAME_LEN];
	int i;
	int rows;
	int show_heart_col;
	lv_obj_t *panel;

	panel = sleep_aid_list_parent(ph);
	if (panel)
	{
		lv_obj_clean(panel);
	}
	sleep_aid_list_rows_free(ph);

	rows = sleep_aid_list_total_rows(ph);
	sleep_aid_diy_clear_ptrs(ph);
	sleep_aid_record_clear_ptrs(ph);
	show_heart_col = 1;
	if (sleep_aid_list_rows_alloc(ph, rows) != 0)
	{
		return;
	}

	for (i = 0; i < rows; i++)
	{
		lv_obj_t *row;
		lv_obj_t *state_img;
		lv_obj_t *lb;
		lv_obj_t *fav_img;
		int vt;
		uint8 is_plus_row;
		uint8 is_fav;
		uint8 is_play;

		is_plus_row = (ph->list_has_plus && i == ph->list_count) ? (uint8)1 : (uint8)0;

		if (!is_plus_row && sleep_aid_add_dl_voice_row(ph, ph->list_ids[i]))
		{
			int si;

			for (si = 0; si < (int)ph->add_dl_count; si++)
			{
				if (ph->add_dl_slots[si].voice_id == ph->list_ids[i])
				{
					row = sleep_aid_add_dl_create_row(ph, y, si);
					sleep_aid_list_row_set(ph, i, row);
					break;
				}
			}
			y += SA_ROW_H + SA_ROW_GAP;
			continue;
		}

		row = lv_obj_create(sleep_aid_list_parent(ph));
		lv_obj_add_style(row, &ph->row_style, 0);
		lv_obj_set_pos(row, 0, y);
		lv_obj_set_size(row, SA_LIST_ROW_W, SA_ROW_H);
		lv_obj_set_style_radius(row, SA_ROW_RADIUS, 0);
		lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

		if (!is_plus_row)
		{
			state_img = lv_img_create(row);
			lv_obj_set_size(state_img, SA_ROW_ICON_SIZE, SA_ROW_ICON_SIZE);
			lv_obj_align(state_img, LV_ALIGN_LEFT_MID, SA_ROW_ICON_LEFT, 0);
			is_play = (!ph->tabs_focus && i == ph->list_focus_idx) ? (uint8)1 : (uint8)0;
			divoom_menu_control_lv_img_set_src(state_img, is_play ? SA_ROW_IMG_PLAY : SA_ROW_IMG_NORMAL);
		}

		lb = lv_label_create(row);
		lv_obj_set_style_text_font(lb, divoom_menu_control_get_font_info(20, ph->menu_id), 0);
		lv_obj_set_style_text_color(lb, lv_color_hex(SA_COL_TEXT), 0);
		if (is_plus_row)
		{
			lv_label_set_text(lb, divoom_menu_control_get_disp_str("SleepAidAddPlus"));
			lv_obj_align(lb, LV_ALIGN_LEFT_MID, SA_ROW_LABEL_X, 0);
		}
		else
		{
			int vt;

			buf[0] = '\0';
			if (ph->list_names)
			{
				strncpy(buf, ph->list_names + i * DIVOOM_SLEEP_AID_VOICE_NAME_LEN, sizeof(buf) - 1);
				buf[sizeof(buf) - 1] = '\0';
			}
			if (buf[0] == '\0')
			{
				vt = ph->list_types ? ph->list_types[i] : sleep_aid_tab_aid_type(ph->tab_idx);
				if (vt >= 0)
				{
					divoom_sleep_aid_send_get_voice_name(vt, ph->list_ids[i], buf);
				}
			}
			if (buf[0] == '\0')
			{
				snprintf(buf, sizeof(buf), "%s%u",
					 divoom_menu_control_get_disp_str("SleepAidVoicePrefix"),
					 (unsigned)ph->list_ids[i]);
			}
			lv_label_set_text(lb, buf);
			lv_obj_align(lb, LV_ALIGN_LEFT_MID, SA_ROW_LABEL_X, 0);
		}
		lv_label_set_long_mode(lb, LV_LABEL_LONG_DOT);
		lv_obj_set_width(lb, SA_LIST_ROW_W - SA_ROW_LABEL_X - SA_ROW_ICON_SIZE - SA_ROW_FAV_RIGHT - 8);

		if (show_heart_col && !is_plus_row)
		{
			vt = ph->list_types ? ph->list_types[i] : sleep_aid_tab_aid_type(ph->tab_idx);
			is_fav = (ph->tab_idx == SA_TAB_FAV) ? (uint8)1 : (uint8)sleep_aid_row_is_fav(ph, ph->list_ids[i], vt);
			fav_img = lv_img_create(row);
			lv_obj_set_size(fav_img, SA_ROW_ICON_SIZE, SA_ROW_ICON_SIZE);
			lv_obj_align(fav_img, LV_ALIGN_RIGHT_MID, -SA_ROW_FAV_RIGHT, 0);
			divoom_menu_control_lv_img_set_src(fav_img, is_fav ? SA_ROW_IMG_FAV : SA_ROW_IMG_UNFAV);
		}

		sleep_aid_list_row_set(ph, i, row);
		y += SA_ROW_H + SA_ROW_GAP;
	}
	{
		lv_coord_t scroll_h;

		scroll_h = SA_LIST_ROW_START_Y + (lv_coord_t)rows * (SA_ROW_H + SA_ROW_GAP);
		if (scroll_h < SA_LIST_H)
		{
			scroll_h = SA_LIST_H;
		}
		lv_obj_set_height(sleep_aid_list_parent(ph), scroll_h);
	}
	ph->list_row_paint_idx = -1;
}

static void sleep_aid_diy_clear_ptrs(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int i;

	ph->diy_preview_btn = NULL;
	ph->diy_fav_btn = NULL;
	ph->diy_fav_icon = NULL;
	ph->diy_edit_cont = NULL;
	ph->diy_edit_slider = NULL;
	ph->diy_edit_lbl = NULL;
	ph->diy_edit_icon = NULL;
	for (i = 0; i < 8; i++)
	{
		ph->diy_ch_rows[i] = NULL;
		ph->diy_ch_sliders[i] = NULL;
	}
}

static lv_obj_t *sleep_aid_diy_create_top_btn(Pdivoom_lvgl_sleep_aid_ctrl ph, lv_coord_t y,
	const char *lang_key, const char *icon_src, lv_obj_t **icon_out)
{
	lv_obj_t *btn;
	lv_obj_t *lb;
	lv_obj_t *icon;

	btn = lv_obj_create(sleep_aid_list_parent(ph));
	lv_obj_add_style(btn, &ph->diy_row_style, 0);
	lv_obj_set_pos(btn, 0, y);
	lv_obj_set_size(btn, SA_DIY_CARD_W, SA_DIY_TOP_BTN_H);
	lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

	lb = lv_label_create(btn);
	lv_label_set_text(lb, divoom_menu_control_get_disp_str(lang_key));
	lv_obj_set_style_text_font(lb, divoom_menu_control_get_font_info(24, ph->menu_id), 0);
	lv_obj_set_style_text_color(lb, lv_color_hex(0x000000), 0);
	lv_obj_set_pos(lb, SA_DIY_TOP_LABEL_X, SA_DIY_TOP_LABEL_Y);

	if (icon_src)
	{
		icon = lv_img_create(btn);
		lv_obj_set_size(icon, SA_DIY_ICON_SIZE, SA_DIY_ICON_SIZE);
		lv_obj_set_pos(icon, SA_DIY_TOP_ICON_X, SA_DIY_TOP_ICON_Y);
		divoom_menu_control_lv_img_set_src(icon, icon_src);
		if (icon_out)
		{
			*icon_out = icon;
		}
	}
	return btn;
}

static void sleep_aid_build_diy(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	lv_coord_t y;
	int i;

	sleep_aid_diy_clear_ptrs(ph);
	sleep_aid_record_clear_ptrs(ph);
	ph->diy_focus = SA_DIY_FOCUS_CH0;
	ph->diy_sel_ch = 0;
	ph->diy_edit_active = 0;

	y = SA_DIY_TOP_Y;
	ph->diy_preview_btn = sleep_aid_diy_create_top_btn(ph, y, "SleepAidPreview", SA_DIY_IMG_PREVIEW, NULL);
	y += SA_DIY_TOP_BTN_H + SA_DIY_TOP_GAP;
	ph->diy_fav_btn = sleep_aid_diy_create_top_btn(ph, y, "SleepAidFavEffects", SA_ROW_IMG_UNFAV, &ph->diy_fav_icon);
	sleep_aid_diy_update_fav_btn(ph);

	{
		lv_obj_t *title;

		title = lv_label_create(sleep_aid_list_parent(ph));
		lv_label_set_text(title, divoom_menu_control_get_disp_str("SleepAidDiyEffectAdjust"));
		lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(20, ph->menu_id), 0);
		lv_obj_set_style_text_color(title, lv_color_hex(SA_DIY_SECTION_TITLE_CLR), 0);
		lv_obj_set_pos(title, SA_DIY_SECTION_TITLE_X, SA_DIY_SECTION_Y);
		lv_obj_set_width(title, SA_DIY_CARD_W - SA_DIY_SECTION_TITLE_X * 2);
	}

	y = SA_DIY_CH_FIRST_Y;
	for (i = 0; i < 8; i++)
	{
		lv_obj_t *row;
		lv_obj_t *icon;
		lv_obj_t *lb;
		lv_obj_t *sl;

		row = lv_obj_create(sleep_aid_list_parent(ph));
		lv_obj_add_style(row, &ph->diy_row_style, 0);
		lv_obj_set_pos(row, 0, y);
		lv_obj_set_size(row, SA_DIY_CARD_W, SA_DIY_ROW_H);
		lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
		ph->diy_ch_rows[i] = row;

		icon = lv_img_create(row);
		lv_obj_set_size(icon, SA_DIY_ICON_SIZE, SA_DIY_ICON_SIZE);
		lv_obj_set_pos(icon, SA_DIY_ICON_X, SA_DIY_ICON_Y);
		divoom_menu_control_lv_img_set_src(icon, "105/105_normal.png");
		lv_obj_move_foreground(icon);

		lb = lv_label_create(row);
		lv_label_set_text(lb, divoom_menu_control_get_disp_str(s_diy_ch_lang_keys[i]));
		lv_obj_set_style_text_font(lb, divoom_menu_control_get_font_info(24, ph->menu_id), 0);
		lv_obj_set_style_text_color(lb, lv_color_hex(0x000000), 0);
		lv_obj_set_pos(lb, SA_DIY_LABEL_X, SA_DIY_LABEL_Y);
		lv_obj_set_size(lb, SA_DIY_CARD_W - SA_DIY_LABEL_X - 16, SA_DIY_LABEL_H);

		(void)sleep_aid_slider_create_track_bg(row, SA_DIY_SLIDER_X, SA_DIY_SLIDER_TRACK_Y, SA_DIY_SLIDER_W);

		sl = lv_slider_create(row);
		lv_obj_set_pos(sl, SA_DIY_SLIDER_X, SA_DIY_SLIDER_Y);
		sleep_aid_slider_set_geometry(sl, SA_DIY_SLIDER_W);
		sleep_aid_slider_apply_style(sl, 1, SA_DIY_VOL_MAX);
		ph->diy_ch_sliders[i] = sl;
		sleep_aid_diy_refresh_ch_slider(ph, i);

		y += SA_DIY_ROW_H + SA_DIY_ROW_GAP;
	}
}

static void sleep_aid_refresh_record_ui(Pdivoom_lvgl_sleep_aid_ctrl ph);
static void sleep_aid_record_update_panel(Pdivoom_lvgl_sleep_aid_ctrl ph);

static void sleep_aid_record_clear_ptrs(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int i;

	for (i = 0; i < SA_RECORD_MAX_CLIPS; i++)
	{
		ph->record_rows[i] = NULL;
		ph->record_row_titles[i] = NULL;
		ph->record_row_durs[i] = NULL;
	}
	ph->record_empty_lbl = NULL;
	ph->record_panel = NULL;
	ph->record_panel_title = NULL;
	ph->record_panel_hint = NULL;
	ph->record_panel_icon = NULL;
	ph->record_time_lbl = NULL;
	ph->record_row_count = 0;
	ph->record_focus = 0;
	ph->record_elapsed_sec = 0;
}

static int sleep_aid_record_panel_focus_idx(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	if (!ph)
	{
		return SA_RECORD_MAX_CLIPS;
	}
	return (ph->record_row_count > 0) ? ph->record_row_count : SA_RECORD_MAX_CLIPS;
}

/** 可聚焦项：已有录音行 0..clips-1 + 底部录音按钮；跳过「暂无录音」空行。 */
static int sleep_aid_record_valid_focus_list(Pdivoom_lvgl_sleep_aid_ctrl ph, int *out, int out_max)
{
	int clips;
	int panel_idx;
	int n;
	int i;

	if (!ph || !out || out_max <= 0)
	{
		return 0;
	}

	clips = divoom_sleep_aid_ctrl_record_clip_count();
	panel_idx = sleep_aid_record_panel_focus_idx(ph);
	n = 0;
	if (clips <= 0)
	{
		out[0] = panel_idx;
		return 1;
	}
	for (i = 0; i < clips && n < out_max; i++)
	{
		out[n] = i;
		n++;
	}
	if (n < out_max)
	{
		out[n] = panel_idx;
		n++;
	}
	return n;
}

static int sleep_aid_record_focus_item_count(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int list[SA_RECORD_MAX_CLIPS + 1];

	return sleep_aid_record_valid_focus_list(ph, list, SA_RECORD_MAX_CLIPS + 1);
}

static void sleep_aid_record_clamp_focus(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int list[SA_RECORD_MAX_CLIPS + 1];
	int n;
	int i;

	if (!ph)
	{
		return;
	}
	n = sleep_aid_record_valid_focus_list(ph, list, SA_RECORD_MAX_CLIPS + 1);
	if (n <= 0)
	{
		return;
	}
	for (i = 0; i < n; i++)
	{
		if (list[i] == ph->record_focus)
		{
			return;
		}
	}
	ph->record_focus = list[0];
}

static void sleep_aid_record_focus_move(Pdivoom_lvgl_sleep_aid_ctrl ph, int step)
{
	int list[SA_RECORD_MAX_CLIPS + 1];
	int n;
	int i;
	int cur_pos;

	if (!ph || step == 0)
	{
		return;
	}
	n = sleep_aid_record_valid_focus_list(ph, list, SA_RECORD_MAX_CLIPS + 1);
	if (n <= 0)
	{
		return;
	}
	cur_pos = -1;
	for (i = 0; i < n; i++)
	{
		if (list[i] == ph->record_focus)
		{
			cur_pos = i;
			break;
		}
	}
	if (cur_pos < 0)
	{
		cur_pos = 0;
	}
	cur_pos += step;
	while (cur_pos < 0)
	{
		cur_pos += n;
	}
	cur_pos %= n;
	ph->record_focus = list[cur_pos];
}

/** 侧栏在录音 TAB 按 ENTER：有录音聚焦第 1 条（最新），无录音聚焦底部录音按钮。 */
static void sleep_aid_record_focus_on_tab_enter(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int clips;
	int panel_focus;

	if (!ph)
	{
		return;
	}

	divoom_sleep_aid_ctrl_record_invalidate_manifest();
	clips = divoom_sleep_aid_ctrl_record_clip_count();
	panel_focus = sleep_aid_record_panel_focus_idx(ph);
	if (clips > 0)
	{
		ph->record_focus = 0;
	}
	else
	{
		ph->record_focus = panel_focus;
	}
	sleep_aid_record_clamp_focus(ph);
	sleep_aid_apply_record_voice(ph);
}

static void sleep_aid_record_relayout(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int i;
	lv_coord_t y;
	lv_coord_t scroll_h;

	if (!ph)
	{
		return;
	}

	ph->record_row_count = SA_RECORD_MAX_CLIPS;
	for (i = 0; i < SA_RECORD_MAX_CLIPS; i++)
	{
		if (!ph->record_rows[i])
		{
			continue;
		}
		y = SA_RECORD_LIST_Y + (lv_coord_t)i * (SA_RECORD_ROW_H + SA_RECORD_ROW_GAP);
		lv_obj_set_pos(ph->record_rows[i], 0, y);
		lv_obj_clear_flag(ph->record_rows[i], LV_OBJ_FLAG_HIDDEN);
	}
	if (ph->record_panel)
	{
		lv_obj_set_pos(ph->record_panel, 0, SA_RECORD_PANEL_Y);
	}
	scroll_h = SA_RECORD_PANEL_Y + SA_RECORD_PANEL_H + 24;
	if (scroll_h > SA_LIST_H)
	{
		lv_obj_set_height(sleep_aid_list_parent(ph), scroll_h);
	}
	else
	{
		lv_obj_set_height(sleep_aid_list_parent(ph), SA_LIST_H);
	}
	sleep_aid_record_clamp_focus(ph);
}

static lv_obj_t *sleep_aid_record_focus_obj(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int clips;
	int panel_idx;

	if (!ph || ph->tabs_focus)
	{
		return NULL;
	}
	clips = divoom_sleep_aid_ctrl_record_clip_count();
	panel_idx = sleep_aid_record_panel_focus_idx(ph);
	if (ph->record_focus >= 0 && ph->record_focus < clips)
	{
		return ph->record_rows[ph->record_focus];
	}
	if (ph->record_focus == panel_idx)
	{
		return ph->record_panel;
	}
	return NULL;
}

static void sleep_aid_record_paint_focus(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int i;
	int clips;
	int panel_idx;
	uint8 on_panel;

	if (!ph || ph->tab_idx != SA_TAB_RECORD)
	{
		return;
	}
	clips = divoom_sleep_aid_ctrl_record_clip_count();
	panel_idx = sleep_aid_record_panel_focus_idx(ph);
	on_panel = 0;
	if (!ph->tabs_focus && ph->record_focus == panel_idx)
	{
		on_panel = 1;
	}
	for (i = 0; i < SA_RECORD_MAX_CLIPS; i++)
	{
		uint8 focused;

		if (!ph->record_rows[i])
		{
			continue;
		}
		focused = (!ph->tabs_focus && i == ph->record_focus && i < clips) ? (uint8)1 : (uint8)0;
		sleep_aid_diy_set_row_border(ph->record_rows[i], focused, SA_ROW_FOCUS_BORDER_COLOR);
	}
	if (ph->record_panel)
	{
		sleep_aid_diy_set_row_border(ph->record_panel, on_panel, SA_ROW_FOCUS_BORDER_COLOR);
	}
}

static uint32 sleep_aid_record_wav_duration_sec(uint32 file_id)
{
	char path[256];
	uint8 hdr[4];
	FILE *fp;
	int sz;

	if (file_id == 0)
	{
		return 0;
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
			if (sz <= SA_RECORD_WAV_HDR)
			{
				return 0;
			}
			return (uint32)((sz - SA_RECORD_WAV_HDR) / SA_RECORD_BYTES_PER_SEC);
		}
	}
	else if (fp)
	{
		divoom_port_file_close(fp);
	}

	if (sz < 128)
	{
		return 0;
	}
	return divoom_mic_record_file_duration_sec(path);
}

static void sleep_aid_record_build_clip_title(char *buf, int buflen, uint32 utc_sec)
{
	struct tm tm_info;
	const char *label;
	time_t t;
	int mon;
	int mday;
	int hour;
	int min;

	if (!buf || buflen < 16 || utc_sec == 0)
	{
		return;
	}
	label = divoom_menu_control_get_disp_str("SleepAidRecordClipLabel");
	mon = 1;
	mday = 1;
	hour = 0;
	min = 0;
	t = (time_t)utc_sec;
	if (localtime_r(&t, &tm_info) != NULL)
	{
		mon = tm_info.tm_mon + 1;
		mday = tm_info.tm_mday;
		hour = tm_info.tm_hour;
		min = tm_info.tm_min;
	}
	snprintf(buf, buflen, "%d-%d %02d:%02d %s", mon, mday, hour, min, label);
}

static void sleep_aid_record_update_panel(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	uint8 busy;

	if (!ph->record_panel || !ph->record_panel_title || !ph->record_panel_hint)
	{
		return;
	}
	busy = ph->record_ui_busy;
	if (busy)
	{
		lv_label_set_text(ph->record_panel_title, divoom_menu_control_get_disp_str("SleepAidRecording"));
		lv_label_set_text(ph->record_panel_hint, divoom_menu_control_get_disp_str("VoiceRecordPressToEnd"));
	}
	else
	{
		lv_label_set_text(ph->record_panel_title, divoom_menu_control_get_disp_str("SleepAidTabRecord"));
		lv_label_set_text(ph->record_panel_hint, divoom_menu_control_get_disp_str("VoiceRecordPressToStart"));
	}
	if (ph->record_panel_icon)
	{
		divoom_menu_control_lv_img_set_src(ph->record_panel_icon,
						 busy ? SA_RECORD_IMG_STOP : SA_RECORD_IMG_IDLE);
	}
	if (ph->record_time_lbl)
	{
		if (busy)
		{
			unsigned mm;
			unsigned ss;
			uint32 sec;

			sec = ph->record_elapsed_sec;
			if (sec > SA_RECORD_MAX_DURATION_SEC)
			{
				sec = SA_RECORD_MAX_DURATION_SEC;
			}
			mm = (unsigned)(sec / 60u);
			ss = (unsigned)(sec % 60u);
			lv_label_set_text_fmt(ph->record_time_lbl, "%02u:%02u", mm, ss);
			lv_obj_clear_flag(ph->record_time_lbl, LV_OBJ_FLAG_HIDDEN);
		}
		else
		{
			lv_obj_add_flag(ph->record_time_lbl, LV_OBJ_FLAG_HIDDEN);
		}
	}
}

static void sleep_aid_record_refresh_timer_cb(lv_timer_t *t)
{
	Pdivoom_lvgl_sleep_aid_ctrl ph;
	uint8 busy;
	uint8 was_busy;

	ph = (Pdivoom_lvgl_sleep_aid_ctrl)lv_timer_get_user_data(t);
	if (!ph || ph->record_refresh_timer != t)
	{
		return;
	}
	was_busy = ph->record_ui_busy;
	busy = divoom_sleep_aid_ctrl_record_busy();
	if (busy)
	{
		ph->record_elapsed_sec++;
		if (ph->record_elapsed_sec >= SA_RECORD_MAX_DURATION_SEC)
		{
			divoom_sleep_aid_ctrl_record_ctrl(0);
			ph->record_elapsed_sec = SA_RECORD_MAX_DURATION_SEC;
			busy = 0;
			ph->record_focus = 0;
			lv_timer_set_period(t, 120);
			lv_timer_set_repeat_count(t, 8);
		}
	}
	else if (was_busy)
	{
		ph->record_elapsed_sec = 0;
		ph->record_focus = 0;
		divoom_sleep_aid_ctrl_record_ctrl(0);
		divoom_sleep_aid_ctrl_record_invalidate_manifest();
	}
	if (ph->tab_idx == SA_TAB_RECORD)
	{
		ph->record_ui_busy = busy;
		sleep_aid_refresh_record_ui(ph);
		sleep_aid_paint_focus(ph);
		if (!busy && was_busy)
		{
			sleep_aid_apply_record_voice(ph);
			lv_obj_t *focus_row;

			focus_row = sleep_aid_record_focus_obj(ph);
			if (focus_row)
			{
				lv_obj_scroll_to_view(focus_row, LV_ANIM_OFF);
			}
		}
	}
}

static void sleep_aid_refresh_record_ui(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int i;
	int clips;
	uint32 file_id;
	char title[64];
	char dur[16];
	unsigned mm;
	unsigned ss;
	uint32 sec;
	const char *no_clip_str;

	clips = divoom_sleep_aid_ctrl_record_clip_count();
	ph->record_ui_busy = divoom_sleep_aid_ctrl_record_busy();
	no_clip_str = divoom_menu_control_get_disp_str("VoiceRecordNoClip");

	for (i = 0; i < SA_RECORD_MAX_CLIPS; i++)
	{
		if (!ph->record_rows[i])
		{
			continue;
		}
		if (i < clips)
		{
			int voice_id;
			int slot;
			uint32 utc_sec;

			voice_id = divoom_sleep_aid_ctrl_record_clip_id(i);
			if (voice_id < 0)
			{
				if (ph->record_row_titles[i])
				{
					lv_label_set_text(ph->record_row_titles[i], no_clip_str);
				}
				if (ph->record_row_durs[i])
				{
					lv_label_set_text(ph->record_row_durs[i], "00:00");
				}
				continue;
			}
			slot = divoom_sleep_aid_ctrl_record_sorted_slot(i);
			utc_sec = divoom_sleep_aid_ctrl_record_slot_utc(slot);
			file_id = (uint32)(voice_id - DIVOOM_VOICE_ID_SLEEP_AID_RECORD_BASE);
			sleep_aid_record_build_clip_title(title, (int)sizeof(title), utc_sec);
			sec = sleep_aid_record_wav_duration_sec(file_id);
			mm = (unsigned)(sec / 60u);
			ss = (unsigned)(sec % 60u);
			snprintf(dur, sizeof(dur), "%02u:%02u", mm, ss);
			if (ph->record_row_titles[i])
			{
				lv_label_set_text(ph->record_row_titles[i], title);
			}
			if (ph->record_row_durs[i])
			{
				lv_label_set_text(ph->record_row_durs[i], dur);
			}
		}
		else
		{
			if (ph->record_row_titles[i])
			{
				lv_label_set_text(ph->record_row_titles[i], no_clip_str);
			}
			if (ph->record_row_durs[i])
			{
				lv_label_set_text(ph->record_row_durs[i], "00:00");
			}
		}
	}

	sleep_aid_record_relayout(ph);
	sleep_aid_record_update_panel(ph);
	sleep_aid_record_clamp_focus(ph);
	sleep_aid_record_paint_focus(ph);
}

static void sleep_aid_build_record(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	lv_coord_t y;
	int i;

	sleep_aid_diy_clear_ptrs(ph);
	sleep_aid_record_clear_ptrs(ph);
	ph->record_ui_busy = 0;

	y = SA_RECORD_LIST_Y;
	for (i = 0; i < SA_RECORD_MAX_CLIPS; i++)
	{
		lv_obj_t *row;
		lv_obj_t *icon;
		lv_obj_t *play;
		lv_obj_t *title;
		lv_obj_t *dur;

		row = lv_obj_create(sleep_aid_list_parent(ph));
		lv_obj_add_style(row, &ph->diy_row_style, 0);
		lv_obj_set_pos(row, 0, y);
		lv_obj_set_size(row, SA_RECORD_ROW_W, SA_RECORD_ROW_H);
		lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
		ph->record_rows[i] = row;

		icon = lv_img_create(row);
		lv_obj_set_size(icon, SA_RECORD_ICON_SIZE, SA_RECORD_ICON_SIZE);
		lv_obj_set_pos(icon, SA_RECORD_ICON_X, SA_RECORD_ICON_Y);
		divoom_menu_control_lv_img_set_src(icon, SA_RECORD_IMG_WAVE);

		title = lv_label_create(row);
		lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(24, ph->menu_id), 0);
		lv_obj_set_style_text_color(title, lv_color_hex(0x000000), 0);
		lv_obj_set_pos(title, SA_RECORD_TITLE_X, SA_RECORD_TITLE_Y);
		lv_obj_set_width(title, 320);
		lv_label_set_text(title, divoom_menu_control_get_disp_str("VoiceRecordNoClip"));
		ph->record_row_titles[i] = title;

		dur = lv_label_create(row);
		lv_obj_set_style_text_font(dur, divoom_menu_control_get_font_info(20, ph->menu_id), 0);
		lv_obj_set_style_text_color(dur, lv_color_hex(SA_RECORD_HINT_COLOR), 0);
		lv_obj_set_pos(dur, SA_RECORD_TITLE_X, SA_RECORD_DUR_Y);
		lv_label_set_text(dur, "00:00");
		ph->record_row_durs[i] = dur;

		play = lv_img_create(row);
		lv_obj_set_size(play, SA_RECORD_ICON_SIZE, SA_RECORD_ICON_SIZE);
		lv_obj_set_pos(play, SA_RECORD_PLAY_X, SA_RECORD_PLAY_Y);
		divoom_menu_control_lv_img_set_src(play, SA_ROW_IMG_PLAY);

		y += SA_RECORD_ROW_H + SA_RECORD_ROW_GAP;
	}

	ph->record_empty_lbl = NULL;

	ph->record_panel = lv_obj_create(sleep_aid_list_parent(ph));
	lv_obj_add_style(ph->record_panel, &ph->diy_row_style, 0);
	lv_obj_set_pos(ph->record_panel, 0, SA_RECORD_PANEL_Y);
	lv_obj_set_size(ph->record_panel, SA_RECORD_PANEL_W, SA_RECORD_PANEL_H);
	lv_obj_clear_flag(ph->record_panel, LV_OBJ_FLAG_SCROLLABLE);

	ph->record_panel_title = lv_label_create(ph->record_panel);
	lv_obj_set_style_text_font(ph->record_panel_title, divoom_menu_control_get_font_info(24, ph->menu_id), 0);
	lv_obj_set_style_text_color(ph->record_panel_title, lv_color_hex(0x000000), 0);
	lv_obj_set_width(ph->record_panel_title, 400);
	lv_obj_set_style_text_align(ph->record_panel_title, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_align(ph->record_panel_title, LV_ALIGN_TOP_MID, 0, 29);

	ph->record_panel_hint = lv_label_create(ph->record_panel);
	lv_obj_set_style_text_font(ph->record_panel_hint, divoom_menu_control_get_font_info(20, ph->menu_id), 0);
	lv_obj_set_style_text_color(ph->record_panel_hint, lv_color_hex(SA_RECORD_HINT_COLOR), 0);
	lv_obj_set_width(ph->record_panel_hint, 400);
	lv_obj_set_style_text_align(ph->record_panel_hint, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_align(ph->record_panel_hint, LV_ALIGN_TOP_MID, 0, 69);

	ph->record_panel_icon = lv_img_create(ph->record_panel);
	lv_obj_set_size(ph->record_panel_icon, 72, 72);
	lv_obj_align(ph->record_panel_icon, LV_ALIGN_CENTER, 0, 18);

	ph->record_time_lbl = lv_label_create(ph->record_panel);
	lv_obj_set_style_text_font(ph->record_time_lbl, divoom_menu_control_get_font_info(40, ph->menu_id), 0);
	lv_obj_set_style_text_color(ph->record_time_lbl, lv_color_hex(0x000000), 0);
	lv_obj_set_width(ph->record_time_lbl, 400);
	lv_obj_set_style_text_align(ph->record_time_lbl, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_align(ph->record_time_lbl, LV_ALIGN_BOTTOM_MID, 0, -30);
	lv_obj_add_flag(ph->record_time_lbl, LV_OBJ_FLAG_HIDDEN);

	sleep_aid_refresh_record_ui(ph);
}


static void sleep_aid_sett_clear_ptrs(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int i;

	for (i = 0; i < 8; i++)
	{
		ph->sett_row_objs[i] = NULL;
	}
	ph->sett_row_count = 0;
	ph->sett_before_bright_sl = NULL;
	ph->sett_before_vol_sl = NULL;
	ph->sett_after_bright_sl = NULL;
	ph->sett_after_vol_sl = NULL;
	ph->sett_before_time_lbl = NULL;
	ph->sett_after_time_lbl = NULL;
	ph->sett_after_music_lbl = NULL;
	ph->sett_mask_img = NULL;
}

static void sleep_aid_sett_format_timer_label(char *buf, int buflen, int minutes)
{
	if (minutes <= 0)
	{
		snprintf(buf, buflen, "%s", divoom_menu_control_get_disp_str("SleepAidTimerOff"));
	}
	else
	{
		snprintf(buf, buflen, "%dmin", minutes);
	}
}

static int sleep_aid_sett_timer_pick_index(int minutes)
{
	int i;

	for (i = 0; i < 6; i++)
	{
		if (s_sett_timer_minutes[i] == minutes)
		{
			return i;
		}
	}
	return 0;
}

static void sleep_aid_sett_update_mask_visual(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	Pdivoom_sleep_aid_info inf;
	uint8 on;

	if (!ph->sett_mask_img)
	{
		return;
	}
	inf = divoom_sleep_aid_ctrl_get_settings();
	on = inf->after_on_off ? (uint8)1 : (uint8)0;
	divoom_menu_control_lv_img_set_src(ph->sett_mask_img, on ? SA_SETT_IMG_ON : SA_SETT_IMG_OFF);
}

static void sleep_aid_sett_refresh_value_labels(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	Pdivoom_sleep_aid_info inf;
	char buf[32];
	int idx;

	inf = divoom_sleep_aid_ctrl_get_settings();
	if (ph->sett_before_time_lbl)
	{
		sleep_aid_sett_format_timer_label(buf, (int)sizeof(buf), inf->before_time);
		lv_label_set_text(ph->sett_before_time_lbl, buf);
	}
	if (ph->sett_after_time_lbl)
	{
		sleep_aid_sett_format_timer_label(buf, (int)sizeof(buf), inf->after_time);
		lv_label_set_text(ph->sett_after_time_lbl, buf);
	}
	if (ph->sett_after_music_lbl)
	{
		idx = inf->after_music;
		if (idx < 0)
		{
			idx = 0;
		}
		if (idx > 2)
		{
			idx = 2;
		}
		lv_label_set_text(ph->sett_after_music_lbl, divoom_menu_control_get_disp_str(s_sett_after_music_keys[idx]));
	}
}

static void sleep_aid_sett_place_section_title(Pdivoom_lvgl_sleep_aid_ctrl ph, lv_coord_t y, const char *lang_key)
{
	lv_obj_t *title;

	title = lv_label_create(sleep_aid_list_parent(ph));
	lv_label_set_text(title, divoom_menu_control_get_disp_str(lang_key));
	lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(20, ph->menu_id), 0);
	lv_obj_set_style_text_color(title, lv_color_hex(0x636363), 0);
	lv_obj_set_pos(title, SA_SETT_SECTION_TITLE_X, y);
	lv_obj_set_width(title, SA_LIST_ROW_W - 32);
}

static lv_obj_t *sleep_aid_sett_create_card(Pdivoom_lvgl_sleep_aid_ctrl ph, lv_coord_t y)
{
	lv_obj_t *row;

	row = lv_obj_create(sleep_aid_list_parent(ph));
	lv_obj_add_style(row, &ph->diy_row_style, 0);
	lv_obj_set_pos(row, 0, y);
	lv_obj_set_size(row, SA_SETT_CARD_W, SA_SETT_ROW_H);
	lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
	return row;
}

static lv_obj_t *sleep_aid_sett_add_slider_block(Pdivoom_lvgl_sleep_aid_ctrl ph, lv_coord_t section_y,
	const char *title_key, lv_obj_t **slider_out, int value, int level_max,
	const char *icon_left, const char *icon_right)
{
	lv_obj_t *row;
	lv_obj_t *sl;
	lv_obj_t *img_l;
	lv_obj_t *img_r;

	sleep_aid_sett_place_section_title(ph, section_y, title_key);
	row = sleep_aid_sett_create_card(ph, section_y + SA_SETT_SECTION_TITLE_H + SA_SETT_SECTION_TITLE_GAP);

	img_l = lv_img_create(row);
	lv_obj_set_size(img_l, SA_SETT_ICON_SIZE, SA_SETT_ICON_SIZE);
	lv_obj_set_pos(img_l, SA_SETT_ICON_L_X, SA_SETT_ICON_L_Y);
	divoom_menu_control_lv_img_set_src(img_l, icon_left);

	img_r = lv_img_create(row);
	lv_obj_set_size(img_r, SA_SETT_ICON_SIZE, SA_SETT_ICON_SIZE);
	lv_obj_set_pos(img_r, SA_SETT_ICON_R_X, SA_SETT_ICON_L_Y);
	divoom_menu_control_lv_img_set_src(img_r, icon_right);

	(void)sleep_aid_slider_create_track_bg(row, SA_SETT_SLIDER_X, SA_SETT_SLIDER_TRACK_Y, SA_SETT_SLIDER_W);

	sl = lv_slider_create(row);
	lv_obj_set_pos(sl, SA_SETT_SLIDER_X, SA_SETT_SLIDER_Y);
	sleep_aid_slider_set_geometry(sl, SA_SETT_SLIDER_W);
	sleep_aid_slider_apply_style(sl, 1, level_max);
	if (value < 0)
	{
		value = 0;
	}
	if (value > level_max)
	{
		value = level_max;
	}
	lv_slider_set_value(sl, value, LV_ANIM_OFF);
	if (slider_out)
	{
		*slider_out = sl;
	}
	return row;
}

static lv_obj_t *sleep_aid_sett_add_picker_card(Pdivoom_lvgl_sleep_aid_ctrl ph, lv_coord_t card_y,
	const char *title_key, lv_obj_t **value_lbl_out, const char *value_text)
{
	lv_obj_t *row;
	lv_obj_t *lb;
	lv_obj_t *val;

	row = sleep_aid_sett_create_card(ph, card_y);
	lb = lv_label_create(row);
	lv_label_set_text(lb, divoom_menu_control_get_disp_str(title_key));
	lv_obj_set_style_text_font(lb, divoom_menu_control_get_font_info(20, ph->menu_id), 0);
	lv_obj_set_style_text_color(lb, lv_color_hex(0x000000), 0);
	lv_obj_set_pos(lb, SA_SETT_ROW_LABEL_X, SA_SETT_ROW_LABEL_Y);

	val = lv_label_create(row);
	lv_obj_set_style_text_font(val, divoom_menu_control_get_font_info(20, ph->menu_id), 0);
	lv_obj_set_style_text_color(val, lv_color_hex(0x000000), 0);
	lv_obj_set_style_text_align(val, LV_TEXT_ALIGN_RIGHT, 0);
	lv_obj_set_width(val, SA_SETT_ROW_VALUE_W);
	lv_obj_set_pos(val, SA_SETT_ROW_VALUE_X, SA_SETT_ROW_LABEL_Y);
	lv_label_set_text(val, value_text);
	if (value_lbl_out)
	{
		*value_lbl_out = val;
	}
	return row;
}

static lv_obj_t *sleep_aid_sett_add_timer_card(Pdivoom_lvgl_sleep_aid_ctrl ph, lv_coord_t card_y,
	const char *title_key, lv_obj_t **value_lbl_out, int minutes)
{
	char buf[32];

	sleep_aid_sett_format_timer_label(buf, (int)sizeof(buf), minutes);
	return sleep_aid_sett_add_picker_card(ph, card_y, title_key, value_lbl_out, buf);
}

static lv_obj_t *sleep_aid_sett_add_mask_block(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	lv_obj_t *row;
	lv_obj_t *lb;

	sleep_aid_sett_place_section_title(ph, SA_SETT_Y_AFTER_HDR, "SleepAidSetAfterSection");
	row = sleep_aid_sett_create_card(ph, SA_SETT_Y_AFTER_MASK);
	lb = lv_label_create(row);
	lv_label_set_text(lb, divoom_menu_control_get_disp_str("SleepAidSetAfterMask"));
	lv_obj_set_style_text_font(lb, divoom_menu_control_get_font_info(20, ph->menu_id), 0);
	lv_obj_set_style_text_color(lb, lv_color_hex(0x000000), 0);
	lv_obj_set_pos(lb, SA_SETT_ROW_LABEL_X, SA_SETT_ROW_LABEL_Y);

	ph->sett_mask_img = lv_img_create(row);
	lv_obj_set_size(ph->sett_mask_img, SA_SETT_TOGGLE_W, SA_SETT_TOGGLE_H);
	lv_obj_set_pos(ph->sett_mask_img, SA_SETT_TOGGLE_X, SA_SETT_TOGGLE_Y);
	sleep_aid_sett_update_mask_visual(ph);
	return row;
}

static lv_obj_t *sleep_aid_sett_add_music_card(Pdivoom_lvgl_sleep_aid_ctrl ph, lv_coord_t card_y, int music_idx)
{
	lv_obj_t *row;
	int idx;

	idx = music_idx;
	if (idx < 0)
	{
		idx = 0;
	}
	if (idx > 2)
	{
		idx = 2;
	}
	row = sleep_aid_sett_add_picker_card(ph, card_y, "SleepAidSetAfterMusic", &ph->sett_after_music_lbl,
		divoom_menu_control_get_disp_str(s_sett_after_music_keys[idx]));
	return row;
}

static void sleep_aid_sett_paint_focus(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	Pdivoom_sleep_aid_info inf;
	int i;

	inf = divoom_sleep_aid_ctrl_get_settings();
	for (i = 0; i < ph->sett_row_count; i++)
	{
		uint8 focused;
		uint32 border_color;

		focused = (!ph->tabs_focus && ph->sett_row == i) ? (uint8)1 : (uint8)0;
		border_color = SA_DIY_BORDER_IDLE;
		if (focused && ph->sett_edit_active && sleep_aid_sett_row_is_slider(i, inf))
		{
			border_color = SA_DIY_BORDER_EDIT;
		}
		sleep_aid_diy_set_row_border(ph->sett_row_objs[i], focused, border_color);
	}
}

static void sleep_aid_sett_adjust_level(Pdivoom_lvgl_sleep_aid_ctrl ph, int row, int delta)
{
	Pdivoom_sleep_aid_info inf;
	int v;

	inf = divoom_sleep_aid_ctrl_get_settings();
	if (row == 0 && ph->sett_before_bright_sl)
	{
		v = inf->before_bright + delta;
		if (v < 0)
		{
			v = 0;
		}
		if (v > SA_SETT_BRIGHT_LEVEL_MAX)
		{
			v = SA_SETT_BRIGHT_LEVEL_MAX;
		}
		inf->before_bright = v;
		lv_slider_set_value(ph->sett_before_bright_sl, v, LV_ANIM_OFF);
	}
	else if (row == 1 && ph->sett_before_vol_sl)
	{
		v = inf->before_vol + delta;
		if (v < 0)
		{
			v = 0;
		}
		if (v > SA_SETT_VOL_LEVEL_MAX)
		{
			v = SA_SETT_VOL_LEVEL_MAX;
		}
		inf->before_vol = v;
		lv_slider_set_value(ph->sett_before_vol_sl, v, LV_ANIM_OFF);
	}
	else if (inf->after_on_off && row == 4 && ph->sett_after_bright_sl)
	{
		v = inf->after_bright + delta;
		if (v < 0)
		{
			v = 0;
		}
		if (v > SA_SETT_BRIGHT_LEVEL_MAX)
		{
			v = SA_SETT_BRIGHT_LEVEL_MAX;
		}
		inf->after_bright = v;
		lv_slider_set_value(ph->sett_after_bright_sl, v, LV_ANIM_OFF);
	}
	else if (inf->after_on_off && row == 5 && ph->sett_after_vol_sl)
	{
		v = inf->after_vol + delta;
		if (v < 0)
		{
			v = 0;
		}
		if (v > SA_SETT_VOL_LEVEL_MAX)
		{
			v = SA_SETT_VOL_LEVEL_MAX;
		}
		inf->after_vol = v;
		lv_slider_set_value(ph->sett_after_vol_sl, v, LV_ANIM_OFF);
	}
	divoom_sleep_aid_ctrl_settings_save();
	if (ph->sett_edit_active)
	{
		sleep_aid_sett_preview_on_level_changed(ph, row, sleep_aid_sett_row_level(inf, row));
	}
}

static int sleep_aid_sett_row_is_brightness(int row, Pdivoom_sleep_aid_info inf)
{
	if (row == 0)
	{
		return 1;
	}
	if (inf->after_on_off && row == 4)
	{
		return 1;
	}
	return 0;
}

static int sleep_aid_sett_row_is_volume(int row, Pdivoom_sleep_aid_info inf)
{
	if (row == 1)
	{
		return 1;
	}
	if (inf->after_on_off && row == 5)
	{
		return 1;
	}
	return 0;
}

static int sleep_aid_sett_row_level(Pdivoom_sleep_aid_info inf, int row)
{
	if (row == 0)
	{
		return inf->before_bright;
	}
	if (row == 1)
	{
		return inf->before_vol;
	}
	if (inf->after_on_off && row == 4)
	{
		return inf->after_bright;
	}
	if (inf->after_on_off && row == 5)
	{
		return inf->after_vol;
	}
	return 0;
}

static uint8 sleep_aid_sett_level_to_screen_bright(int level)
{
	if (level < 0)
	{
		level = 0;
	}
	if (level > SA_SETT_BRIGHT_LEVEL_MAX)
	{
		level = SA_SETT_BRIGHT_LEVEL_MAX;
	}
	return (uint8)level;
}

static int sleep_aid_sett_level_to_voice_vol(int level)
{
	if (level < 0)
	{
		level = 0;
	}
	if (level > SA_SETT_VOL_LEVEL_MAX)
	{
		level = SA_SETT_VOL_LEVEL_MAX;
	}
	return level;
}

static void sleep_aid_sett_end_edit_preview(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	if (!ph || ph->sett_preview_kind == SA_SETT_PREVIEW_NONE)
	{
		return;
	}
	if (ph->sett_preview_kind == SA_SETT_PREVIEW_BRIGHT)
	{
		divoom_main_apply_sys_light();
	}
	else if (ph->sett_preview_kind == SA_SETT_PREVIEW_VOL)
	{
		divoom_sleep_aid_ctrl_settings_volume_preview_stop();
	}
	ph->sett_preview_kind = SA_SETT_PREVIEW_NONE;
}

static void sleep_aid_sett_begin_edit_preview(Pdivoom_lvgl_sleep_aid_ctrl ph, int row)
{
	Pdivoom_sleep_aid_info inf;
	int level;

	if (!ph)
	{
		return;
	}
	inf = divoom_sleep_aid_ctrl_get_settings();
	sleep_aid_sett_end_edit_preview(ph);
	level = sleep_aid_sett_row_level(inf, row);
	if (sleep_aid_sett_row_is_brightness(row, inf))
	{
		ph->sett_preview_kind = SA_SETT_PREVIEW_BRIGHT;
		if (divoom_light_app_get_screen_status(0xFF))
			divoom_screen_bright(sleep_aid_sett_level_to_screen_bright(level));
	}
	else if (sleep_aid_sett_row_is_volume(row, inf))
	{
		uint8 is_after_vol;

		ph->sett_preview_kind = SA_SETT_PREVIEW_VOL;
		is_after_vol = (inf->after_on_off && row == 5) ? (uint8)1 : (uint8)0;
		divoom_sleep_aid_ctrl_settings_volume_preview_start(level, is_after_vol);
	}
}

static void sleep_aid_sett_preview_on_level_changed(Pdivoom_lvgl_sleep_aid_ctrl ph, int row, int level)
{
	Pdivoom_sleep_aid_info inf;

	if (!ph || !ph->sett_edit_active)
	{
		return;
	}
	inf = divoom_sleep_aid_ctrl_get_settings();
	if (ph->sett_preview_kind == SA_SETT_PREVIEW_BRIGHT && sleep_aid_sett_row_is_brightness(row, inf))
	{
		if (divoom_light_app_get_screen_status(0xFF))
			divoom_screen_bright(sleep_aid_sett_level_to_screen_bright(level));
	}
	else if (ph->sett_preview_kind == SA_SETT_PREVIEW_VOL && sleep_aid_sett_row_is_volume(row, inf))
	{
		divoom_sleep_aid_ctrl_settings_volume_preview_set_level(sleep_aid_sett_level_to_voice_vol(level));
	}
}

static void sleep_aid_sett_clear_edit(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	if (!ph)
	{
		return;
	}
	if (ph->sett_edit_active)
	{
		sleep_aid_sett_end_edit_preview(ph);
		ph->sett_edit_active = 0;
	}
}

static void sleep_aid_sett_after_music_preview_item(Pdivoom_lvgl_sleep_aid_ctrl ph, int music_idx)
{
	Pdivoom_sleep_aid_info inf;

	if (!ph || music_idx < 0 || music_idx > 2)
	{
		return;
	}
	inf = divoom_sleep_aid_ctrl_get_settings();
	divoom_sleep_aid_ctrl_settings_after_music_preview(music_idx, inf->after_vol);
}

static void sleep_aid_sett_music_preview_cancel(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	if (!ph)
	{
		return;
	}
	if (ph->mbox_music_preview_timer)
	{
		lv_timer_del(ph->mbox_music_preview_timer);
		ph->mbox_music_preview_timer = NULL;
	}
	ph->mbox_music_preview_sel = -1;
}

static void sleep_aid_sett_music_preview_deferred_cb(lv_timer_t *t)
{
	Pdivoom_lvgl_sleep_aid_ctrl ph;
	int sel;

	ph = (Pdivoom_lvgl_sleep_aid_ctrl)lv_timer_get_user_data(t);
	ph->mbox_music_preview_timer = NULL;
	lv_timer_del(t);
	if (!ph || !ph->mbox || ph->mbox_kind != SA_MBOX_KIND_SETT_MUSIC)
	{
		return;
	}
	sel = divoom_lvgl_message_box_get_active_item(ph->mbox);
	if (sel < 0 || sel > 2 || sel == ph->mbox_music_preview_sel)
	{
		return;
	}
	ph->mbox_music_preview_sel = sel;
	sleep_aid_sett_after_music_preview_item(ph, sel);
}

static void sleep_aid_sett_music_mbox_focus_cb(void *user_data, int ctrl_id)
{
	Pdivoom_lvgl_sleep_aid_ctrl ph;

	(void)ctrl_id;
	ph = (Pdivoom_lvgl_sleep_aid_ctrl)user_data;
	if (!ph || !ph->mbox || ph->mbox_kind != SA_MBOX_KIND_SETT_MUSIC)
	{
		return;
	}
	if (ph->mbox_music_preview_timer)
	{
		lv_timer_del(ph->mbox_music_preview_timer);
	}
	ph->mbox_music_preview_timer = lv_timer_create(sleep_aid_sett_music_preview_deferred_cb, 0, ph);
	if (ph->mbox_music_preview_timer)
	{
		lv_timer_set_repeat_count(ph->mbox_music_preview_timer, 1);
	}
}

static int sleep_aid_sett_row_is_slider(int row, Pdivoom_sleep_aid_info inf)
{
	if (row == 0 || row == 1)
	{
		return 1;
	}
	if (inf->after_on_off && (row == 4 || row == 5))
	{
		return 1;
	}
	return 0;
}

static void sleep_aid_sett_open_timer_picker(Pdivoom_lvgl_sleep_aid_ctrl ph, int field)
{
	void *mh;
	char buf[32];
	int i;
	int cur;
	int sel;
	Pdivoom_sleep_aid_info inf;

	if (ph->mbox)
	{
		return;
	}
	inf = divoom_sleep_aid_ctrl_get_settings();
	cur = (field == SA_SETT_FIELD_AFTER_TIME) ? inf->after_time : inf->before_time;
	sel = sleep_aid_sett_timer_pick_index(cur);

	ph->mbox_kind = SA_MBOX_KIND_SETT_TIMER;
	ph->mbox_sett_field = field;
	mh = divoom_lvgl_message_box_init(ph->menu_id, ph->main_obj, ph, sleep_aid_mbox_cb,
					  80, 40, 640, 544, NULL, lv_color_hex(0xE4E4E4), 6);
	ph->mbox = mh;
	if (!mh)
	{
		return;
	}

	for (i = 0; i < 6; i++)
	{
		sleep_aid_sett_format_timer_label(buf, (int)sizeof(buf), s_sett_timer_minutes[i]);
		divoom_lvgl_message_box_add_btn_item(mh, buf, 26,
						     lv_color_hex(0xE4E4E4), lv_color_hex(0x000000), lv_color_hex(0x000000),
						     i, 32, 32 + i * 80, 576, 80);
	}
	divoom_lvgl_message_box_set_active_item(mh, sel);
}

static void sleep_aid_sett_open_music_picker(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	void *mh;
	int i;
	int idx;
	Pdivoom_sleep_aid_info inf;

	if (ph->mbox)
	{
		return;
	}
	inf = divoom_sleep_aid_ctrl_get_settings();
	idx = inf->after_music;
	if (idx < 0)
	{
		idx = 0;
	}
	if (idx > 2)
	{
		idx = 2;
	}

	ph->mbox_kind = SA_MBOX_KIND_SETT_MUSIC;
	ph->mbox_sett_field = 0;
	mh = divoom_lvgl_message_box_init(ph->menu_id, ph->main_obj, ph, sleep_aid_mbox_cb,
					  80, 40, 640, 470, NULL, lv_color_hex(0xE4E4E4), 3);
	ph->mbox = mh;
	if (!mh)
	{
		return;
	}

	for (i = 0; i < 3; i++)
	{
		divoom_lvgl_message_box_add_btn_item(mh,
			divoom_menu_control_get_disp_str(s_sett_after_music_keys[i]),
			26, lv_color_hex(0xE4E4E4), lv_color_hex(0x000000), lv_color_hex(0x000000),
			i, 32, 32 + i * 114, 576, 98);
	}
	ph->mbox_music_preview_sel = -1;
	divoom_lvgl_message_box_set_focus_change_cb(mh, sleep_aid_sett_music_mbox_focus_cb, ph);
	divoom_lvgl_message_box_set_active_item(mh, idx);
}

static void sleep_aid_build_settings(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	Pdivoom_sleep_aid_info inf;
	int idx;
	lv_coord_t scroll_h;

	sleep_aid_sett_clear_ptrs(ph);
	sleep_aid_diy_clear_ptrs(ph);
	sleep_aid_record_clear_ptrs(ph);
	sleep_aid_sett_end_edit_preview(ph);
	ph->sett_edit_active = 0;
	inf = divoom_sleep_aid_ctrl_get_settings();
	idx = 0;

	ph->sett_row_objs[idx++] = sleep_aid_sett_add_slider_block(ph, SA_SETT_Y_BEFORE_BRIGHT,
		"SleepAidSetBeforeBrightness", &ph->sett_before_bright_sl, inf->before_bright,
		SA_SETT_BRIGHT_LEVEL_MAX, SA_SETT_IMG_BR_LEFT, SA_SETT_IMG_BR_RIGHT);
	ph->sett_row_objs[idx++] = sleep_aid_sett_add_slider_block(ph, SA_SETT_Y_BEFORE_VOL,
		"SleepAidSetBeforeSleepVol", &ph->sett_before_vol_sl, inf->before_vol,
		SA_SETT_VOL_LEVEL_MAX, SA_SETT_IMG_VOL_LEFT, SA_SETT_IMG_VOL_RIGHT);
	ph->sett_row_objs[idx++] = sleep_aid_sett_add_timer_card(ph, SA_SETT_Y_BEFORE_TIMER,
		"SleepAidSetBeforeTimer", &ph->sett_before_time_lbl, inf->before_time);

	ph->sett_row_objs[idx++] = sleep_aid_sett_add_mask_block(ph);

	if (inf->after_on_off)
	{
		ph->sett_row_objs[idx++] = sleep_aid_sett_add_slider_block(ph, SA_SETT_Y_AFTER_BRIGHT,
			"SleepAidSetAfterBrightness", &ph->sett_after_bright_sl, inf->after_bright,
			SA_SETT_BRIGHT_LEVEL_MAX, SA_SETT_IMG_BR_LEFT, SA_SETT_IMG_BR_RIGHT);
		ph->sett_row_objs[idx++] = sleep_aid_sett_add_slider_block(ph, SA_SETT_Y_AFTER_VOL,
			"SleepAidSetAfterSleepVol", &ph->sett_after_vol_sl, inf->after_vol,
			SA_SETT_VOL_LEVEL_MAX, SA_SETT_IMG_VOL_LEFT, SA_SETT_IMG_VOL_RIGHT);
		ph->sett_row_objs[idx++] = sleep_aid_sett_add_music_card(ph, SA_SETT_Y_AFTER_MUSIC, inf->after_music);
		ph->sett_row_objs[idx++] = sleep_aid_sett_add_timer_card(ph, SA_SETT_Y_AFTER_TIMER,
			"SleepAidSetAfterTimer", &ph->sett_after_time_lbl, inf->after_time);
		scroll_h = SA_SETT_Y_AFTER_TIMER + SA_SETT_ROW_H + 20;
	}
	else
	{
		scroll_h = SA_SETT_Y_AFTER_MASK + SA_SETT_ROW_H + 20;
	}

	ph->sett_row_count = idx;
	lv_obj_set_height(sleep_aid_list_parent(ph), scroll_h > SA_LIST_H ? scroll_h : SA_LIST_H);
	sleep_aid_sett_refresh_value_labels(ph);
}

static void sleep_aid_mbox_close(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	void *mh;
	uint8 mbox_kind;

	if (!ph || !ph->mbox)
	{
		return;
	}
	mbox_kind = ph->mbox_kind;
	mh = ph->mbox;
	ph->mbox = NULL;
	ph->mbox_kind = SA_MBOX_KIND_VOICE;
	if (mbox_kind == SA_MBOX_KIND_SETT_MUSIC)
	{
		sleep_aid_sett_music_preview_cancel(ph);
		divoom_sleep_aid_ctrl_settings_volume_preview_stop();
	}
	divoom_lvgl_message_box_destroy(mh, 0);
	if (lv_group_get_default())
	{
		lv_group_focus_obj(ph->main_obj);
	}
	sleep_aid_paint_focus(ph);
}

static void sleep_aid_mbox_cb(void *user_data, int ctrl_id)
{
	Pdivoom_lvgl_sleep_aid_ctrl ph;

	ph = (Pdivoom_lvgl_sleep_aid_ctrl)user_data;
	if (!ph || !ph->mbox)
	{
		return;
	}

	if (ctrl_id < 0)
	{
		sleep_aid_mbox_close(ph);
		return;
	}

	if (ph->mbox_kind == SA_MBOX_KIND_SETT_TIMER)
	{
		Pdivoom_sleep_aid_info inf;

		inf = divoom_sleep_aid_ctrl_get_settings();
		if (ctrl_id >= 0 && ctrl_id < 6)
		{
			int min;

			min = s_sett_timer_minutes[ctrl_id];
			if (ph->mbox_sett_field == SA_SETT_FIELD_AFTER_TIME)
			{
				inf->after_time = min;
			}
			else
			{
				inf->before_time = min;
			}
			sleep_aid_sett_refresh_value_labels(ph);
			divoom_sleep_aid_ctrl_settings_save();
		}
		sleep_aid_mbox_close(ph);
		return;
	}

	if (ph->mbox_kind == SA_MBOX_KIND_SETT_MUSIC)
	{
		Pdivoom_sleep_aid_info inf;

		inf = divoom_sleep_aid_ctrl_get_settings();
		if (ctrl_id >= 0 && ctrl_id < 3)
		{
			inf->after_music = ctrl_id;
			sleep_aid_sett_refresh_value_labels(ph);
			divoom_sleep_aid_ctrl_settings_save();
		}
		sleep_aid_mbox_close(ph);
		return;
	}

	if (ph->mbox_kind == SA_MBOX_KIND_VOICE)
	{
		if (ctrl_id == 0)
		{
			divoom_sleep_aid_ctrl_enter_play_voice(ph->mbox_voice_type, (int)ph->mbox_voice_id);
			sleep_aid_mbox_close(ph);
		}
		else if (ctrl_id == 1)
		{
			int removed_idx = -1;

			if (ph->mbox_is_fav)
			{
				if (ph->tab_idx == SA_TAB_FAV)
				{
					removed_idx = sleep_aid_fav_find_list_index(ph, ph->mbox_voice_id, ph->mbox_voice_type);
				}
				divoom_sleep_aid_fav_remove(ph->mbox_voice_id, ph->mbox_voice_type);
			}
			else
			{
				int vt;

				vt = divoom_sleep_aid_voice_get_video_type(ph->mbox_voice_type, ph->mbox_voice_id);
				divoom_sleep_aid_fav_add(ph->mbox_voice_id, ph->mbox_voice_type, vt);
			}

			sleep_aid_mbox_close(ph);
			if (ph->tab_idx == SA_TAB_FAV)
			{
				sleep_aid_fav_tab_reload_ui(ph, removed_idx);
			}
			else
			{
				sleep_aid_fav_list_changed(ph);
				if (sleep_aid_tab_is_voice_list(ph->tab_idx))
				{
					sleep_aid_update_voice_list_row_icons(ph);
				}
			}
		}
		else
		{
			sleep_aid_mbox_close(ph);
		}
	}
}

static void sleep_aid_open_voice_menu(Pdivoom_lvgl_sleep_aid_ctrl ph, int vt, uint32 vid)
{
	void *mh;
	char title[DIVOOM_SLEEP_AID_VOICE_NAME_LEN];

	if (ph->mbox)
	{
		return;
	}

	ph->mbox_voice_type = vt;
	ph->mbox_voice_id = vid;
	ph->mbox_is_fav = sleep_aid_row_is_fav(ph, vid, vt) ? (uint8)1 : (uint8)0;
	ph->mbox_kind = SA_MBOX_KIND_VOICE;

	mh = divoom_lvgl_message_box_init(ph->menu_id, ph->main_obj, ph, sleep_aid_mbox_cb,
					    80, 502, 640, 276, NULL, lv_color_hex(0xEFEFEF), 2);
	ph->mbox = mh;

	divoom_lvgl_message_box_add_btn_item(mh,
					     divoom_menu_control_get_disp_str("SleepAidPreview"),
					     26, lv_color_hex(0xCCCCCC), lv_color_hex(0x333333), lv_color_hex(0x111111),
					     0, 32, 32, 576, 98);

	divoom_lvgl_message_box_add_btn_item(mh,
					     divoom_menu_control_get_disp_str(ph->mbox_is_fav ? "SleepAidUnfav" : "SleepAidFav"),
					     26, lv_color_hex(0xCCCCCC), lv_color_hex(0x333333), lv_color_hex(0x111111),
					     1, 32, 146, 576, 98);

	divoom_lvgl_message_box_set_active_item(mh, 0);
}

static void sleep_aid_refresh_tab_labels(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	int i;

	if (!ph)
	{
		return;
	}
	for (i = 0; i < SA_TAB_COUNT; i++)
	{
		lv_obj_t *lb = lv_obj_get_child(ph->tab_objs[i], 0);

		if (lb)
		{
			lv_label_set_text(lb, divoom_menu_control_get_disp_str(s_tab_lang_keys[i]));
		}
		if (ph->tab_arrow_lbl[i])
		{
			lv_label_set_text(ph->tab_arrow_lbl[i], ">");
		}
	}
}

static void sleep_aid_refresh_content_force(Pdivoom_lvgl_sleep_aid_ctrl ph, uint8 force_rebuild)
{
	int i;
	int tab;
	int prev_tab;
	int saved_sett_row;

	if (!ph)
	{
		return;
	}

	tab = ph->tab_idx;
	prev_tab = ph->last_shown_tab;

	if (prev_tab >= 0 && prev_tab != tab)
	{
		sleep_aid_on_tab_leave(ph, prev_tab);
	}

	if (tab == SA_TAB_FAV && ph->fav_tab_stale)
	{
		sleep_aid_invalidate_tab(ph, SA_TAB_FAV);
		ph->fav_tab_stale = 0;
		force_rebuild = 1;
	}

	if (ph->tab_built[tab] && !force_rebuild)
	{
		if (sleep_aid_tab_is_voice_list(tab))
		{
			sleep_aid_voice_store_apply(ph, tab);
		}
		else if (tab == SA_TAB_RECORD)
		{
			divoom_sleep_aid_ctrl_record_invalidate_manifest();
			sleep_aid_refresh_record_ui(ph);
		}
		sleep_aid_tab_restore_focus(ph, tab);
		sleep_aid_show_tab_panel(ph);
		sleep_aid_paint_focus(ph);
		if (tab == SA_TAB_RECORD && !ph->tabs_focus)
		{
			sleep_aid_apply_record_voice(ph);
		}
		else if (tab == SA_TAB_DIY && !ph->tabs_focus)
		{
			sleep_aid_apply_diy_voice(ph);
		}
		ph->last_shown_tab = tab;
		return;
	}

	saved_sett_row = -1;
	if (tab == SA_TAB_SETTINGS)
	{
		saved_sett_row = ph->sett_row;
	}

	if (force_rebuild && ph->tab_built[tab])
	{
		sleep_aid_invalidate_tab(ph, tab);
	}

	ph->list_focus_idx = 0;
	ph->diy_focus = SA_DIY_FOCUS_CH0;
	ph->diy_sel_ch = 0;
	ph->diy_edit_active = 0;
	if (tab != SA_TAB_SETTINGS)
	{
		ph->sett_row = 0;
	}
	sleep_aid_sett_clear_edit(ph);

	switch (tab)
	{
	case SA_TAB_FAV:
	case SA_TAB_NOISE:
	case SA_TAB_WHITE:
	case SA_TAB_MUSIC:
	{
		sleep_aid_load_list_model(ph);
		sleep_aid_build_voice_list_ui(ph);
		sleep_aid_voice_store_commit(ph, tab);
		sleep_aid_apply_list_voice(ph);
		sleep_aid_apply_tab_visuals_delta(ph);
		sleep_aid_paint_voice_list_focus(ph, 1);
		break;
	}
	case SA_TAB_DIY:
	{
		sleep_aid_build_diy(ph);
		break;
	}
	case SA_TAB_RECORD:
	{
		sleep_aid_build_record(ph);
		break;
	}
	case SA_TAB_SETTINGS:
	{
		sleep_aid_build_settings(ph);
		if (saved_sett_row >= 0)
		{
			if (saved_sett_row >= ph->sett_row_count)
			{
				saved_sett_row = ph->sett_row_count - 1;
			}
			if (saved_sett_row < 0)
			{
				saved_sett_row = 0;
			}
			ph->sett_row = saved_sett_row;
		}
		break;
	}
	default:
	{
		break;
	}
	}

	ph->tab_built[tab] = 1;
	if (tab == SA_TAB_SETTINGS && saved_sett_row >= 0)
	{
		ph->tab_saved_focus[SA_TAB_SETTINGS] = ph->sett_row;
	}
	else
	{
		sleep_aid_tab_restore_focus(ph, tab);
	}
	sleep_aid_show_tab_panel(ph);
	if (!sleep_aid_tab_is_voice_list(tab))
	{
		sleep_aid_paint_focus(ph);
	}
	if (tab == SA_TAB_RECORD && !ph->tabs_focus)
	{
		sleep_aid_apply_record_voice(ph);
	}
	else if (tab == SA_TAB_DIY && !ph->tabs_focus)
	{
		sleep_aid_apply_diy_voice(ph);
	}
	sleep_aid_refresh_tab_labels(ph);
	ph->last_shown_tab = tab;
}

static void sleep_aid_refresh_content(Pdivoom_lvgl_sleep_aid_ctrl ph)
{
	sleep_aid_refresh_content_force(ph, 0);
}

static void sleep_aid_key_main(Pdivoom_lvgl_sleep_aid_ctrl ph, char key)
{
	if (ph->mbox)
	{
		if (key == DIVOOM_LVGL_KEY_NEXT || key == DIVOOM_LVGL_KEY_PRE ||
		    key == DIVOOM_LVGL_KEY_ENTER || key == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_lvgl_message_box_dispatch_key(ph->mbox, (uint8)key);
		}
		return;
	}

	if (key == DIVOOM_LVGL_KEY_ESC)
	{
		if (!ph->tabs_focus && ph->tab_idx == SA_TAB_DIY && ph->diy_edit_active)
		{
			ph->diy_edit_active = 0;
			sleep_aid_diy_apply_visual(ph);
			sleep_aid_paint_focus(ph);
			return;
		}
		if (!ph->tabs_focus && ph->tab_idx == SA_TAB_SETTINGS && ph->sett_edit_active)
		{
			if (ph->sett_preview_kind == SA_SETT_PREVIEW_VOL)
			{
				sleep_aid_stop_list_preview(ph);
				ph->sett_preview_kind = SA_SETT_PREVIEW_NONE;
				ph->sett_edit_active = 0;
			}
			else
			{
				sleep_aid_sett_clear_edit(ph);
			}
			sleep_aid_paint_focus(ph);
			return;
		}
		if (ph->tabs_focus)
		{
			divoom_menu_control_return_pre();
		}
		else
		{
			if (sleep_aid_tab_is_voice_list(ph->tab_idx)
			    || ph->tab_idx == SA_TAB_RECORD
			    || ph->tab_idx == SA_TAB_DIY)
			{
				sleep_aid_stop_list_preview(ph);
			}
			sleep_aid_leave_subpage(ph);
			ph->tabs_focus = 1;
		}
		sleep_aid_paint_focus(ph);
		return;
	}

	if (ph->tabs_focus)
	{
		if (key == DIVOOM_LVGL_KEY_NEXT || key == DIVOOM_LVGL_KEY_PRE)
		{
			if (key == DIVOOM_LVGL_KEY_NEXT)
			{
				ph->tab_idx++;
				if (ph->tab_idx >= SA_TAB_COUNT)
				{
					ph->tab_idx = 0;
				}
			}
			else
			{
				ph->tab_idx--;
				if (ph->tab_idx < 0)
				{
					ph->tab_idx = SA_TAB_COUNT - 1;
				}
			}
			sleep_aid_refresh_content(ph);
		}
		else if (key == DIVOOM_LVGL_KEY_ENTER)
		{
			/* 侧栏 NEXT/PRE 仅切换 TAB；ENTER 才进入右侧（列表 / DIY / 录音 / 设置） */
			ph->tabs_focus = 0;
			if (sleep_aid_tab_is_voice_list(ph->tab_idx))
			{
				int rows = sleep_aid_list_total_rows(ph);

				if (rows > 0 && ph->list_focus_idx >= rows)
				{
					ph->list_focus_idx = 0;
				}
				sleep_aid_apply_list_voice(ph);
			}
			else if (ph->tab_idx == SA_TAB_RECORD)
			{
				sleep_aid_record_focus_on_tab_enter(ph);
			}
			else if (ph->tab_idx == SA_TAB_DIY)
			{
				sleep_aid_apply_diy_voice(ph);
			}
			sleep_aid_paint_focus(ph);
			if (ph->tab_idx == SA_TAB_RECORD)
			{
				lv_obj_t *focus_row;

				focus_row = sleep_aid_record_focus_obj(ph);
				if (focus_row)
				{
					lv_obj_scroll_to_view(focus_row, LV_ANIM_OFF);
				}
			}
		}
		return;
	}

	switch (ph->tab_idx)
	{
	case SA_TAB_FAV:
	case SA_TAB_NOISE:
	case SA_TAB_WHITE:
	case SA_TAB_MUSIC:
	{
		int rows = sleep_aid_list_total_rows(ph);

		if (key == DIVOOM_LVGL_KEY_NEXT && rows > 0)
		{
			ph->list_focus_idx++;
			if (ph->list_focus_idx >= rows)
			{
				ph->list_focus_idx = 0;
			}
			sleep_aid_apply_list_voice(ph);
			sleep_aid_paint_focus(ph);
		}
		else if (key == DIVOOM_LVGL_KEY_PRE && rows > 0)
		{
			ph->list_focus_idx--;
			if (ph->list_focus_idx < 0)
			{
				ph->list_focus_idx = rows - 1;
			}
			sleep_aid_apply_list_voice(ph);
			sleep_aid_paint_focus(ph);
		}
		else if (key == DIVOOM_LVGL_KEY_ENTER && rows > 0)
		{
			if (ph->list_has_plus && ph->list_focus_idx == ph->list_count)
			{
				SA_ADD_DL_LOG("key ENTER on plus row tab=%d focus=%d count=%d\n",
					      ph->tab_idx, ph->list_focus_idx, ph->list_count);
				if (ph->add_dl_defer_timer)
				{
					lv_timer_del(ph->add_dl_defer_timer);
					ph->add_dl_defer_timer = NULL;
				}
				ph->add_dl_defer_timer = lv_timer_create(sleep_aid_add_dl_deferred_cb, 1, ph);
				if (ph->add_dl_defer_timer)
				{
					lv_timer_set_repeat_count(ph->add_dl_defer_timer, 1);
				}
				else
				{
					SA_ADD_DL_LOG("key ENTER: defer timer create failed\n");
				}
			}
			else if (ph->list_ids && ph->list_types)
			{
				sleep_aid_open_voice_menu(ph, ph->list_types[ph->list_focus_idx], ph->list_ids[ph->list_focus_idx]);
			}
		}
		break;
	}
	case SA_TAB_DIY:
	{
		Pdivoom_sleep_aid_info inf;
		int ch;

		inf = divoom_sleep_aid_ctrl_get_settings();
		if (ph->diy_edit_active)
		{
			if (key == DIVOOM_LVGL_KEY_NEXT || key == DIVOOM_LVGL_KEY_PRE)
			{
				int v;
				int vol_idx;

				ch = ph->diy_sel_ch;
				vol_idx = sleep_aid_diy_vol_idx(ch);
				v = inf->volume[vol_idx];
				if (key == DIVOOM_LVGL_KEY_NEXT)
				{
					v++;
					if (v > SA_DIY_VOL_MAX)
					{
						v = SA_DIY_VOL_MAX;
					}
				}
				else
				{
					v--;
					if (v < 0)
					{
						v = 0;
					}
				}
				inf->volume[vol_idx] = v;
				sleep_aid_diy_refresh_ch_slider(ph, ch);
				divoom_sleep_aid_ctrl_settings_save();
			}
			else if (key == DIVOOM_LVGL_KEY_ESC)
			{
				ph->diy_edit_active = 0;
			}
			sleep_aid_paint_focus(ph);
			break;
		}

		if (key == DIVOOM_LVGL_KEY_NEXT)
		{
			ph->diy_focus++;
			if (ph->diy_focus >= SA_DIY_FOCUS_COUNT)
			{
				ph->diy_focus = SA_DIY_FOCUS_PREVIEW;
			}
			if (ph->diy_focus >= SA_DIY_FOCUS_CH0 && ph->diy_focus <= SA_DIY_FOCUS_CH_LAST)
			{
				ph->diy_sel_ch = ph->diy_focus - SA_DIY_FOCUS_CH0;
				sleep_aid_diy_sync_sel_ch(ph);
				sleep_aid_apply_diy_voice(ph);
			}
			else
			{
				sleep_aid_stop_list_preview(ph);
			}
			sleep_aid_paint_focus(ph);
		}
		else if (key == DIVOOM_LVGL_KEY_PRE)
		{
			ph->diy_focus--;
			if (ph->diy_focus < SA_DIY_FOCUS_PREVIEW)
			{
				ph->diy_focus = SA_DIY_FOCUS_CH_LAST;
			}
			if (ph->diy_focus >= SA_DIY_FOCUS_CH0 && ph->diy_focus <= SA_DIY_FOCUS_CH_LAST)
			{
				ph->diy_sel_ch = ph->diy_focus - SA_DIY_FOCUS_CH0;
				sleep_aid_diy_sync_sel_ch(ph);
				sleep_aid_apply_diy_voice(ph);
			}
			else
			{
				sleep_aid_stop_list_preview(ph);
			}
			sleep_aid_paint_focus(ph);
		}
		else if (key == DIVOOM_LVGL_KEY_ENTER)
		{
			if (ph->diy_focus == SA_DIY_FOCUS_PREVIEW)
			{
				/* 效果预览：占位，后续接混音预览 API */
			}
			else if (ph->diy_focus == SA_DIY_FOCUS_FAV)
			{
				if (sleep_aid_row_is_fav(ph, SA_DIY_MIX_VOICE_ID, DIVOOM_SLEEP_AID_TYPE_DIY))
				{
					divoom_sleep_aid_fav_remove(SA_DIY_MIX_VOICE_ID, DIVOOM_SLEEP_AID_TYPE_DIY);
				}
				else
				{
					{
						int vt;

						vt = divoom_sleep_aid_voice_get_video_type(DIVOOM_SLEEP_AID_TYPE_DIY, SA_DIY_MIX_VOICE_ID);
						divoom_sleep_aid_fav_add(SA_DIY_MIX_VOICE_ID, DIVOOM_SLEEP_AID_TYPE_DIY, vt);
					}
				}
				sleep_aid_diy_update_fav_btn(ph);
				if (ph->tab_idx == SA_TAB_FAV)
				{
					sleep_aid_fav_tab_reload_ui(ph, -1);
				}
				else
				{
					sleep_aid_fav_list_changed(ph);
					sleep_aid_paint_focus(ph);
				}
			}
			else if (ph->diy_focus >= SA_DIY_FOCUS_CH0 && ph->diy_focus <= SA_DIY_FOCUS_CH_LAST)
			{
				ph->diy_sel_ch = ph->diy_focus - SA_DIY_FOCUS_CH0;
				ph->diy_edit_active = ph->diy_edit_active ? 0 : 1;
				sleep_aid_diy_sync_sel_ch(ph);
				sleep_aid_diy_apply_visual(ph);
				sleep_aid_paint_focus(ph);
			}
		}
		break;
	}
	case SA_TAB_RECORD:
	{
		if (key == DIVOOM_LVGL_KEY_NEXT)
		{
			sleep_aid_record_focus_move(ph, 1);
			if (ph->record_focus == sleep_aid_record_panel_focus_idx(ph))
			{
				sleep_aid_stop_list_preview(ph);
			}
			else
			{
				sleep_aid_apply_record_voice(ph);
			}
			sleep_aid_record_paint_focus(ph);
			{
				lv_obj_t *focus_row;

				focus_row = sleep_aid_record_focus_obj(ph);
				if (focus_row)
				{
					lv_obj_scroll_to_view(focus_row, LV_ANIM_OFF);
				}
			}
		}
		else if (key == DIVOOM_LVGL_KEY_PRE)
		{
			sleep_aid_record_focus_move(ph, -1);
			if (ph->record_focus == sleep_aid_record_panel_focus_idx(ph))
			{
				sleep_aid_stop_list_preview(ph);
			}
			else
			{
				sleep_aid_apply_record_voice(ph);
			}
			sleep_aid_record_paint_focus(ph);
			{
				lv_obj_t *focus_row;

				focus_row = sleep_aid_record_focus_obj(ph);
				if (focus_row)
				{
					lv_obj_scroll_to_view(focus_row, LV_ANIM_OFF);
				}
			}
		}
		else if (key == DIVOOM_LVGL_KEY_ENTER)
		{
			int panel_idx;

			panel_idx = sleep_aid_record_panel_focus_idx(ph);

			if (ph->record_focus == panel_idx)
			{
				uint8 was_busy;

				was_busy = divoom_sleep_aid_ctrl_record_busy();
				if (was_busy)
				{
					divoom_sleep_aid_ctrl_record_ctrl(0);
					ph->record_elapsed_sec = 0;
					ph->record_focus = 0;
					divoom_sleep_aid_ctrl_record_invalidate_manifest();
				}
				else
				{
					ph->record_elapsed_sec = 0;
					divoom_sleep_aid_ctrl_record_ctrl(1);
				}
				sleep_aid_refresh_record_ui(ph);
				if (!ph->record_ui_busy)
				{
					sleep_aid_apply_record_voice(ph);
				}
				sleep_aid_record_stop_refresh_timer(ph);
				ph->record_refresh_timer = lv_timer_create(sleep_aid_record_refresh_timer_cb,
							      ph->record_ui_busy ? 1000 : 120, ph);
				if (ph->record_refresh_timer)
				{
					if (ph->record_ui_busy)
					{
						lv_timer_set_repeat_count(ph->record_refresh_timer, -1);
					}
					else
					{
						lv_timer_set_repeat_count(ph->record_refresh_timer, 8);
					}
				}
				{
					lv_obj_t *focus_row;

					focus_row = sleep_aid_record_focus_obj(ph);
					if (focus_row)
					{
						lv_obj_scroll_to_view(focus_row, LV_ANIM_OFF);
					}
				}
			}
		}
		break;
	}
	case SA_TAB_SETTINGS:
	{
		Pdivoom_sleep_aid_info inf;

		inf = divoom_sleep_aid_ctrl_get_settings();
		if (ph->sett_edit_active && sleep_aid_sett_row_is_slider(ph->sett_row, inf))
		{
			if (key == DIVOOM_LVGL_KEY_NEXT)
			{
				sleep_aid_sett_adjust_level(ph, ph->sett_row, 1);
				sleep_aid_paint_focus(ph);
			}
			else if (key == DIVOOM_LVGL_KEY_PRE)
			{
				sleep_aid_sett_adjust_level(ph, ph->sett_row, -1);
				sleep_aid_paint_focus(ph);
			}
			else if (key == DIVOOM_LVGL_KEY_ENTER)
			{
				sleep_aid_sett_clear_edit(ph);
				sleep_aid_paint_focus(ph);
			}
			break;
		}

		if (key == DIVOOM_LVGL_KEY_NEXT)
		{
			sleep_aid_sett_clear_edit(ph);
			ph->sett_row++;
			if (ph->sett_row >= ph->sett_row_count)
			{
				ph->sett_row = 0;
			}
			sleep_aid_paint_focus(ph);
		}
		else if (key == DIVOOM_LVGL_KEY_PRE)
		{
			sleep_aid_sett_clear_edit(ph);
			ph->sett_row--;
			if (ph->sett_row < 0)
			{
				if (ph->sett_row_count > 0)
				{
					ph->sett_row = ph->sett_row_count - 1;
				}
				else
				{
					ph->sett_row = 0;
				}
			}
			sleep_aid_paint_focus(ph);
		}
		else if (key == DIVOOM_LVGL_KEY_ENTER)
		{
			if (ph->sett_row == 2)
			{
				sleep_aid_sett_open_timer_picker(ph, SA_SETT_FIELD_BEFORE_TIME);
			}
			else if (ph->sett_row == 3)
			{
				inf->after_on_off = !inf->after_on_off;
				divoom_sleep_aid_ctrl_settings_save();
				ph->tab_saved_focus[SA_TAB_SETTINGS] = ph->sett_row;
				sleep_aid_invalidate_tab(ph, SA_TAB_SETTINGS);
				sleep_aid_refresh_content_force(ph, 0);
			}
			else if (inf->after_on_off && ph->sett_row == 6)
			{
				sleep_aid_sett_open_music_picker(ph);
			}
			else if (inf->after_on_off && ph->sett_row == 7)
			{
				sleep_aid_sett_open_timer_picker(ph, SA_SETT_FIELD_AFTER_TIME);
			}
			else if (sleep_aid_sett_row_is_slider(ph->sett_row, inf))
			{
				ph->sett_edit_active = 1;
				sleep_aid_sett_begin_edit_preview(ph, ph->sett_row);
				sleep_aid_paint_focus(ph);
			}
		}
		break;
	}
	default:
	{
		break;
	}
	}
}

static void sleep_aid_event_cb(lv_event_t *e)
{
	lv_event_code_t code;
	Pdivoom_lvgl_sleep_aid_ctrl ph;

	code = lv_event_get_code(e);
	ph = (Pdivoom_lvgl_sleep_aid_ctrl)lv_event_get_user_data(e);
	if (!ph || code != LV_EVENT_KEY)
	{
		return;
	}
	sleep_aid_key_main(ph, ((char *)lv_event_get_param(e))[0]);
}

static void sleep_aid_back_cb(lv_event_t *e)
{
	lv_event_code_t code;
	char *key_info;

	code = lv_event_get_code(e);
	if (code != LV_EVENT_KEY)
	{
		return;
	}
	key_info = (char *)lv_event_get_param(e);
	if (key_info && (key_info[0] == DIVOOM_LVGL_KEY_ENTER
			 || key_info[0] == DIVOOM_LVGL_KEY_ESC))
	{
		divoom_menu_control_return_pre();
	}
}

void *divoom_lvgl_sleep_aid_init(void *user_data, int menu_id)
{
	int i;
	Pdivoom_lvgl_sleep_aid_ctrl ph;

	(void)user_data;
	ph = (Pdivoom_lvgl_sleep_aid_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_sleep_aid_ctrl));
	if (!ph)
	{
		return NULL;
	}
	memset(ph, 0, sizeof(*ph));
	ph->menu_id = menu_id;
	ph->tab_idx = SA_TAB_FAV;
	ph->tabs_focus = 1;
	ph->last_shown_tab = -1;
	ph->record_focus = 0;
	ph->diy_focus = SA_DIY_FOCUS_CH0;
	ph->diy_sel_ch = 0;
	ph->sett_row = 0;
	ph->list_row_paint_idx = -1;

	sleep_aid_init_styles(ph);
	divoom_sleep_aid_ctrl_get_settings();
	sleep_aid_fav_cache_reload(ph);

	ph->main_obj = lv_obj_create(NULL);
	lv_obj_add_style(ph->main_obj, &ph->root_style, 0);
	lv_obj_set_size(ph->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(ph->main_obj, 0, 0);
	lv_obj_clear_flag(ph->main_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(ph->main_obj, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(ph->main_obj, sleep_aid_event_cb, LV_EVENT_KEY, ph);

	{
		lv_obj_t *back;

		back = lv_img_create(ph->main_obj);
		lv_obj_set_pos(back, 36, 51);
		lv_obj_set_size(back, 78, 78);
		divoom_menu_control_lv_img_set_src(back, "26/26_back.png");
		lv_obj_add_flag(back, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_event_cb(back, sleep_aid_back_cb, LV_EVENT_KEY, ph);
	}
	ph->sidebar_cont = lv_obj_create(ph->main_obj);
	lv_obj_add_style(ph->sidebar_cont, &ph->sidebar_style, 0);
	lv_obj_set_pos(ph->sidebar_cont, SA_SIDEBAR_X, SA_HDR_Y);
	lv_obj_set_size(ph->sidebar_cont, SA_SIDEBAR_W, SA_SIDEBAR_H);
	lv_obj_clear_flag(ph->sidebar_cont, LV_OBJ_FLAG_SCROLLABLE);

	ph->content_cont = lv_obj_create(ph->main_obj);
	lv_obj_add_style(ph->content_cont, &ph->list_scroll_style, 0);
	lv_obj_set_pos(ph->content_cont, SA_LIST_X, 0);
	lv_obj_set_size(ph->content_cont, SA_LIST_W, SA_LIST_H);
	lv_obj_clear_flag(ph->content_cont, LV_OBJ_FLAG_SCROLLABLE);

	for (i = 0; i < SA_TAB_COUNT; i++)
	{
		ph->tab_panel[i] = lv_obj_create(ph->content_cont);
		lv_obj_add_style(ph->tab_panel[i], &ph->list_scroll_style, 0);
		lv_obj_set_pos(ph->tab_panel[i], 0, 0);
		lv_obj_set_size(ph->tab_panel[i], SA_LIST_W, SA_LIST_H);
		lv_obj_set_scrollbar_mode(ph->tab_panel[i], LV_SCROLLBAR_MODE_AUTO);
		lv_obj_set_scroll_dir(ph->tab_panel[i], LV_DIR_VER);
		lv_obj_add_flag(ph->tab_panel[i], LV_OBJ_FLAG_HIDDEN);
	}
	ph->last_shown_tab = -1;

	for (i = 0; i < SA_TAB_COUNT; i++)
	{
		lv_obj_t *t;
		lv_obj_t *lb;
		lv_obj_t *arrow;

		t = lv_obj_create(ph->sidebar_cont);
		lv_obj_add_style(t, &ph->tab_cell_style, 0);
		lv_obj_set_pos(t, SA_TAB_INSET_X, SA_TAB_ITEM_Y + i * (SA_TAB_ITEM_H + SA_TAB_ITEM_GAP));
		lv_obj_set_size(t, SA_TAB_ITEM_W, SA_TAB_ITEM_H);
		lv_obj_clear_flag(t, LV_OBJ_FLAG_SCROLLABLE);

		lb = lv_label_create(t);
		lv_obj_set_style_text_font(lb, divoom_menu_control_get_font_info(20, menu_id), 0);
		lv_obj_set_style_text_color(lb, lv_color_hex(SA_COL_TAB_TEXT), 0);
		lv_label_set_long_mode(lb, LV_LABEL_LONG_DOT);
		lv_obj_set_width(lb, SA_TAB_LABEL_W);
		lv_label_set_text(lb, divoom_menu_control_get_disp_str(s_tab_lang_keys[i]));
		lv_obj_set_pos(lb, SA_TAB_TEXT_PAD_X, SA_TAB_ARROW_Y);

		arrow = lv_label_create(t);
		lv_obj_set_style_text_font(arrow, divoom_menu_control_get_font_info(20, menu_id), 0);
		lv_obj_set_style_text_color(arrow, lv_color_hex(SA_COL_TAB_ARROW), 0);
		lv_label_set_text(arrow, ">");
		lv_obj_set_pos(arrow, SA_TAB_ARROW_X, SA_TAB_ARROW_Y);

		ph->tab_objs[i] = t;
		ph->tab_text_lbl[i] = lb;
		ph->tab_arrow_lbl[i] = arrow;
	}

	sleep_aid_apply_tab_visual(ph, ph->tab_idx);

	sleep_aid_refresh_content(ph);

	if (lv_group_get_default())
	{
		lv_group_add_obj(lv_group_get_default(), ph->main_obj);
	}

	return ph;
}

void divoom_lvgl_sleep_aid_disp(void *handle, void *active)
{
	Pdivoom_lvgl_sleep_aid_ctrl ph;

	ph = (Pdivoom_lvgl_sleep_aid_ctrl)handle;
	(void)active;
	if (!ph)
	{
		return;
	}
	lv_scr_load(ph->main_obj);
	lv_group_focus_obj(ph->main_obj);
	if (ph->tab_idx == SA_TAB_RECORD && ph->tab_built[SA_TAB_RECORD])
	{
		divoom_sleep_aid_ctrl_record_invalidate_manifest();
		sleep_aid_refresh_record_ui(ph);
	}
}

void divoom_lvgl_sleep_aid_pause(void *handle)
{
	Pdivoom_lvgl_sleep_aid_ctrl ph;

	ph = (Pdivoom_lvgl_sleep_aid_ctrl)handle;
	if (!ph)
	{
		return;
	}
	sleep_aid_sett_end_edit_preview(ph);
	ph->sett_edit_active = 0;
	divoom_sleep_aid_ctrl_stop_play_voice();
	sleep_aid_shutdown_async_work(ph);
}

void divoom_lvgl_sleep_aid_exit(void *handle)
{
	Pdivoom_lvgl_sleep_aid_ctrl ph;

	ph = (Pdivoom_lvgl_sleep_aid_ctrl)handle;
	if (!ph)
	{
		return;
	}
	divoom_sleep_aid_ctrl_stop_play_voice();
	divoom_sleep_aid_ctrl_settings_save();
	sleep_aid_teardown_session(ph);
	sleep_aid_reset_styles(ph);
	if (lv_group_get_default() && ph->main_obj)
	{
		lv_group_remove_obj(ph->main_obj);
	}
	lv_obj_del(ph->main_obj);
	ph->main_obj = NULL;
	divoom_system_free(ph);
}

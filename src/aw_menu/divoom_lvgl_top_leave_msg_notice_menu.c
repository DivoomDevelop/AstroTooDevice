#include "divoom.h"
#include "lvgl.h"
#include <string.h>
#include "divoom_menu_control.h"
#include "divoom_system_msg_notice.h"
#include "divoom_light_app.h"
#include "divoom_lvgl_top_leave_msg_notice_menu.h"
#include "divoom_leave_msg_record.h"
#include "divoom_leave_msg_ctrl.h"
#include "divoom_local_music_ctrl.h"
#include "divoom_main.h"
#include "divoom_astrootoo_theme.h"
#include "divoom_lvgl_font_manage.h"

/*
 * 表盘顶栏留言条 — 480-main WarmMemoSavedModal（416×76）
 * 左列图标+时长，右列文案居中；逻辑仍走 system_msg_notice。
 */
#define LEAVE_MSG_NOTICE_W_DESIGN	DIVOOM_ASTRO_TOP_TOAST_W
#define LEAVE_MSG_NOTICE_H_DESIGN	DIVOOM_ASTRO_LEAVE_NOTICE_H
#define LEAVE_MSG_ICON_PATH		"105/leave_msg_notice_icon.png"
/* 资源是 24×24，1:1 显示；设计槽仍是 32，图标在槽内居中，不做 zoom。 */
#define LEAVE_MSG_ICON_NATIVE_PX	24
#define LEAVE_NOTICE_TITLE_FONT_PX	24
/* 两行时目标上下边距（24×2=48 落在 64 槽里各约 8px，与设计 line-height:1 一致） */
#define LEAVE_NOTICE_TITLE_PAD_Y	8

#define LEAVE_NOTICE_PLAY_POLL_MS		200
#define LEAVE_NOTICE_PLAY_ARM_MAX_TICK		25 /* ~5s 内未检测到解码则认为失败并关闭 */

typedef struct _divoom_lvgl_top_leave_msg_notice_ctrl
{
	lv_obj_t *main_obj;
	lv_obj_t *title_label;
	lv_obj_t *dur_label;
	lv_timer_t *play_watch_timer;
	uint8 play_started;
	uint8 play_armed;
	uint8 play_finished;
	uint8 clip_discarded;
	int play_wait_ticks;
	uint32 dur_total_sec;
	uint32 countdown_remain_ms;
} divoom_lvgl_top_leave_msg_notice_ctrl, *Pdivoom_lvgl_top_leave_msg_notice_ctrl;

/* 留言顶栏单例：磁盘单槽，屏幕也只保留一张卡 */
static Pdivoom_lvgl_top_leave_msg_notice_ctrl s_leave_notice;

static void divoom_lvgl_top_leave_msg_notice_apply_card_style(lv_obj_t *root)
{
	static const lv_style_selector_t sel[] = {
		LV_PART_MAIN,
		LV_PART_MAIN | LV_STATE_FOCUSED,
		LV_PART_MAIN | LV_STATE_FOCUS_KEY,
		LV_PART_MAIN | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY,
	};
	unsigned i;

	for (i = 0; i < sizeof(sel) / sizeof(sel[0]); i++) {
		lv_obj_set_style_bg_color(root, lv_color_hex(DIVOOM_ASTRO_MODAL_BG), sel[i]);
		lv_obj_set_style_bg_opa(root, LV_OPA_COVER, sel[i]);
		lv_obj_set_style_border_width(root, 2, sel[i]);
		lv_obj_set_style_border_color(root, lv_color_hex(DIVOOM_ASTRO_STROKE), sel[i]);
		lv_obj_set_style_radius(root, DIVOOM_ASTRO_RADIUS_SM, sel[i]);
		lv_obj_set_style_shadow_width(root, 0, sel[i]);
		lv_obj_set_style_outline_width(root, 0, sel[i]);
		lv_obj_set_style_pad_all(root, 0, sel[i]);
		lv_obj_set_style_pad_row(root, 0, sel[i]);
		lv_obj_set_style_pad_column(root, 0, sel[i]);
		lv_obj_set_style_layout(root, LV_LAYOUT_NONE, sel[i]);
		lv_obj_set_style_clip_corner(root, true, sel[i]);
	}
	lv_obj_set_style_bg_opa(root, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(root, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_clear_flag(root, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
}

/* 透明槽：剥掉 theme card 的 pad/边框，再按 480-main flex 居中排 */
static lv_obj_t *divoom_lvgl_top_leave_msg_notice_make_slot(lv_obj_t *parent,
							     T_S32 x, T_S32 y, T_S32 w, T_S32 h)
{
	lv_obj_t *slot;

	slot = lv_obj_create(parent);
	lv_obj_remove_style_all(slot);
	lv_obj_set_pos(slot, x, y);
	lv_obj_set_size(slot, w, h);
	lv_obj_set_style_bg_opa(slot, LV_OPA_TRANSP, 0);
	lv_obj_set_style_pad_all(slot, 0, 0);
	lv_obj_set_style_pad_row(slot, 0, 0);
	lv_obj_set_style_pad_column(slot, 0, 0);
	lv_obj_clear_flag(slot, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_clear_flag(slot, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
	lv_obj_set_style_clip_corner(slot, true, 0);
	lv_obj_set_scrollbar_mode(slot, LV_SCROLLBAR_MODE_OFF);
	return slot;
}

static void divoom_lvgl_top_leave_msg_notice_slot_flex_center(lv_obj_t *slot)
{
	lv_obj_set_flex_flow(slot, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(slot, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
			      LV_FLEX_ALIGN_CENTER);
}

static void divoom_lvgl_top_leave_msg_notice_setup_label(lv_obj_t *label, T_S32 font_px,
							  T_U32 color, T_BOOL wrap, T_S32 box_w)
{
	lv_obj_set_width(label, box_w);
	lv_obj_set_style_min_width(label, box_w, 0);
	lv_obj_set_style_max_width(label, box_w, 0);
	lv_obj_set_style_pad_all(label, 0, 0);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(label,
		divoom_menu_control_get_font_info(font_px,
			DIVOOM_MENU_CONTROL_ID_TOP_LEAVE_MSG_NOTICE), 0);
	lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
	lv_obj_set_style_text_line_space(label, 0, 0);
	lv_label_set_long_mode(label, wrap ? LV_LABEL_LONG_WRAP : LV_LABEL_LONG_CLIP);
}

/*
 * 英文折两行时 FreeType line_height 常 >24，两行会高于 64px 槽而画出框外。
 * 按实际行数压 line_space，使块高落在槽内并留出上下边距；不让笔画叠在一起。
 */
static void divoom_lvgl_top_leave_msg_notice_fit_title(lv_obj_t *label)
{
	const lv_font_t *font;
	const char *txt;
	lv_point_t sz;
	T_S32 line_h;
	T_S32 letter_space;
	T_S32 box_w;
	T_S32 max_h;
	T_S32 n;
	T_S32 ls;
	T_S32 min_ls;

	if (!label)
		return;
	font = lv_obj_get_style_text_font(label, LV_PART_MAIN);
	txt = lv_label_get_text(label);
	if (!font || !txt || txt[0] == '\0')
		return;
	line_h = (T_S32)lv_font_get_line_height(font);
	if (line_h <= 0)
		line_h = LEAVE_NOTICE_TITLE_FONT_PX;
	letter_space = (T_S32)lv_obj_get_style_text_letter_space(label, LV_PART_MAIN);
	box_w = DIVOOM_ASTRO_LEAVE_NOTICE_MSG_W;

	lv_txt_get_size(&sz, txt, font, letter_space, 0, box_w, LV_TEXT_FLAG_NONE);
	max_h = DIVOOM_ASTRO_LEAVE_NOTICE_MSG_H - 2 * LEAVE_NOTICE_TITLE_PAD_Y;
	if (max_h < line_h)
		max_h = line_h;
	if (sz.y <= max_h) {
		lv_obj_set_style_text_line_space(label, 0, 0);
		return;
	}

	n = (sz.y + line_h / 2) / line_h;
	if (n < 2)
		n = 2;
	/* 行距下限：基线间距不少于字号，避免两行笔画重叠 */
	min_ls = LEAVE_NOTICE_TITLE_FONT_PX - line_h;
	if (min_ls > 0)
		min_ls = 0;
	ls = (max_h - n * line_h) / (n - 1);
	if (ls < min_ls)
		ls = min_ls;
	/* 仍高于槽则贴齐槽高（边距 0），宁可略挤也不画出框 */
	if (n * line_h + (n - 1) * ls > DIVOOM_ASTRO_LEAVE_NOTICE_MSG_H)
		ls = (DIVOOM_ASTRO_LEAVE_NOTICE_MSG_H - n * line_h) / (n - 1);
	if (ls > 0)
		ls = 0;
	lv_obj_set_style_text_line_space(label, ls, 0);
}

static void divoom_lvgl_top_leave_msg_notice_set_title(lv_obj_t *label)
{
	if (!label)
		return;
	lv_label_set_text(label, divoom_menu_control_get_disp_str("VoiceRecordNewMsgNotice"));
	divoom_lvgl_top_leave_msg_notice_fit_title(label);
}

static void divoom_lvgl_top_leave_msg_notice_set_dur_color(
	Pdivoom_lvgl_top_leave_msg_notice_ctrl c, int playing)
{
	if (!c || !c->dur_label)
		return;
	lv_obj_set_style_text_color(c->dur_label,
		lv_color_hex(playing ? DIVOOM_ASTRO_SURFACE_FOCUS : 0x000000), 0);
}

static void divoom_lvgl_top_leave_msg_notice_stop_watch(Pdivoom_lvgl_top_leave_msg_notice_ctrl c)
{
	if (!c || !c->play_watch_timer)
		return;
	lv_timer_del(c->play_watch_timer);
	c->play_watch_timer = NULL;
}

static void divoom_lvgl_top_leave_msg_notice_set_dur_total(Pdivoom_lvgl_top_leave_msg_notice_ctrl c)
{
	unsigned mm, ss;

	if (!c || !c->dur_label)
		return;
	mm = (unsigned)(c->dur_total_sec / 60u);
	ss = (unsigned)(c->dur_total_sec % 60u);
	lv_label_set_text_fmt(c->dur_label, "%02u:%02u", mm, ss);
}

/* Caller holds the LVGL lock through singleton lookup and refresh. */
static int divoom_lvgl_top_leave_msg_notice_refresh_locked(Pdivoom_lvgl_top_leave_msg_notice_ctrl c,
						     uint32_t duration_sec)
{
	uint8 was_playing;

	divoom_lvgl_top_leave_msg_notice_stop_watch(c);
	was_playing = c->play_started;
	c->play_started = 0;
	c->play_armed = 0;
	c->play_finished = 0;
	c->clip_discarded = 0;
	c->play_wait_ticks = 0;
	c->countdown_remain_ms = 0;
	c->dur_total_sec = duration_sec ? (uint32)duration_sec : 1u;
	if (c->title_label)
		divoom_lvgl_top_leave_msg_notice_set_title(c->title_label);
	divoom_lvgl_top_leave_msg_notice_set_dur_total(c);
	divoom_lvgl_top_leave_msg_notice_set_dur_color(c, 0);
	return was_playing;
}

static void divoom_lvgl_top_leave_msg_notice_discard_clip(Pdivoom_lvgl_top_leave_msg_notice_ctrl c)
{
	if (!c || c->clip_discarded || !c->play_finished)
		return;
	c->clip_discarded = 1;
	divoom_leave_msg_record_safe_discard_after_notice_play();
	divoom_leave_msg_ctrl_clear_saved_clip();
}

static void divoom_lvgl_top_leave_msg_notice_set_dur_remain(Pdivoom_lvgl_top_leave_msg_notice_ctrl c)
{
	unsigned mm, ss;
	uint32 rs;

	if (!c || !c->dur_label)
		return;
	rs = c->countdown_remain_ms / 1000u;
	mm = (unsigned)(rs / 60u);
	ss = (unsigned)(rs % 60u);
	lv_label_set_text_fmt(c->dur_label, "%02u:%02u", mm, ss);
}

static void divoom_lvgl_top_leave_msg_notice_play_tick(lv_timer_t *t)
{
	Pdivoom_lvgl_top_leave_msg_notice_ctrl c = (Pdivoom_lvgl_top_leave_msg_notice_ctrl)lv_timer_get_user_data(t);

	if (!c)
		return;
	if (!c->main_obj) {
		if (c->play_watch_timer) {
			lv_timer_del(c->play_watch_timer);
			c->play_watch_timer = NULL;
		}
		return;
	}

	if (!c->play_armed) {
		if (divoom_local_music_ctrl_get_tone_status()) {
			c->play_armed = 1;
			c->countdown_remain_ms = c->dur_total_sec * 1000u;
			divoom_lvgl_top_leave_msg_notice_set_dur_remain(c);
		} else if (++c->play_wait_ticks >= LEAVE_NOTICE_PLAY_ARM_MAX_TICK) {
			lv_timer_del(t);
			c->play_watch_timer = NULL;
			divoom_system_msg_notice_deliver_item_result((void *)c->main_obj, 1);
		}
		return;
	}

	if (divoom_local_music_ctrl_get_tone_status()) {
		uint32 step = t->period;

		if (c->countdown_remain_ms > step)
			c->countdown_remain_ms -= step;
		else
			c->countdown_remain_ms = 0;
		divoom_lvgl_top_leave_msg_notice_set_dur_remain(c);
		return;
	}

	c->play_finished = 1;
	divoom_lvgl_top_leave_msg_notice_discard_clip(c);
	lv_timer_del(t);
	c->play_watch_timer = NULL;
	divoom_system_msg_notice_deliver_item_result((void *)c->main_obj, 1);
}

static void divoom_lvgl_top_leave_msg_notice_begin_play(Pdivoom_lvgl_top_leave_msg_notice_ctrl c)
{
	char path[256];

	if (!c || c->play_started)
		return;

	c->play_started = 1;
	divoom_lvgl_top_leave_msg_notice_set_dur_color(c, 1);
	if (!divoom_leave_msg_record_get_playback_path(path, sizeof(path))) {
		divoom_system_msg_notice_deliver_item_result((void *)c->main_obj, 1);
		return;
	}

	/* play_time=0：不 loop，整段只播一次；倒计时仍用 dur_total_sec */
	divoom_local_music_ctrl_tone_play(path, divoom_leave_msg_record_get_play_volume(), NULL, 0);

	c->play_wait_ticks = 0;
	c->play_armed = 0;
	if (c->play_watch_timer) {
		lv_timer_del(c->play_watch_timer);
		c->play_watch_timer = NULL;
	}
	c->play_watch_timer = lv_timer_create(divoom_lvgl_top_leave_msg_notice_play_tick,
			LEAVE_NOTICE_PLAY_POLL_MS, c);
}

static int divoom_lvgl_top_leave_msg_notice_pre_key(void *para, char key0)
{
	Pdivoom_lvgl_top_leave_msg_notice_ctrl c = (Pdivoom_lvgl_top_leave_msg_notice_ctrl)para;

	(void)key0;
	if (!c)
		return 0;

	if (c->play_started)
		return 1;

	divoom_lvgl_top_leave_msg_notice_begin_play(c);
	return 1;
}

static void divoom_lvgl_top_leave_msg_notice_release_fonts(Pdivoom_lvgl_top_leave_msg_notice_ctrl c)
{
	/* close 走 lv_obj_del_async，回调时控件可能还在；先摘 FT 再清账本 */
	if (c && c->dur_label)
		lv_obj_set_style_text_font(c->dur_label, LV_FONT_DEFAULT, 0);
	if (c && c->title_label)
		lv_obj_set_style_text_font(c->title_label, LV_FONT_DEFAULT, 0);
	divoom_lvgl_font_manage_clear_clock_font(DIVOOM_MENU_CONTROL_ID_TOP_LEAVE_MSG_NOTICE, NULL);
}

static void divoom_lvgl_top_leave_msg_notice_del(Pdivoom_lvgl_top_leave_msg_notice_ctrl c)
{
	if (!c)
		return;
	if (s_leave_notice == c)
		s_leave_notice = NULL;
	divoom_lvgl_top_leave_msg_notice_release_fonts(c);
	divoom_system_free(c);
}

/* result: 1 确认（ENTER），0 取消或批量清除（与周年纪念条语义一致） */
static int divoom_lvgl_top_leave_msg_notice_menu_notice(void *para, int result)
{
	Pdivoom_lvgl_top_leave_msg_notice_ctrl c = (Pdivoom_lvgl_top_leave_msg_notice_ctrl)para;

	if (!c)
		return 0;
	if (s_leave_notice == c)
		s_leave_notice = NULL;
	divoom_lvgl_top_leave_msg_notice_stop_watch(c);
	if (c->play_started)
		divoom_leave_msg_record_stop_preview_playback();
	divoom_lvgl_top_leave_msg_notice_discard_clip(c);
	if (result == 1)
		divoom_light_app_set_clock_from_lvgl_dial_preview(divoom_light_app_get_cur_clock_id(), 0);
	divoom_lvgl_top_leave_msg_notice_del(c);
	return result == 1 ? 1 : 0;
}

int divoom_lvgl_top_leave_msg_notice_is_playing(void)
{
	int playing;

	divoom_get_lvgl_lock();
	playing = s_leave_notice && s_leave_notice->play_started && !s_leave_notice->play_finished;
	divoom_free_lvgl_lock();
	return playing;
}

void divoom_lvgl_top_leave_msg_notice_init(uint32_t duration_sec)
{
	Pdivoom_lvgl_top_leave_msg_notice_ctrl c;
	lv_obj_t *root;
	lv_obj_t *left_col;
	lv_obj_t *icon_slot;
	lv_obj_t *icon;
	lv_obj_t *dur_slot;
	lv_obj_t *msg_slot;
	int w = LEAVE_MSG_NOTICE_W_DESIGN;
	int h = LEAVE_MSG_NOTICE_H_DESIGN;

	if (duration_sec == 0u)
		duration_sec = 1u;
	/* Keep a published notice alive until its controls and key callback are ready. */
	divoom_get_lvgl_lock();
	if (s_leave_notice && s_leave_notice->main_obj) {
		int was_playing = divoom_lvgl_top_leave_msg_notice_refresh_locked(s_leave_notice, duration_sec);

		divoom_free_lvgl_lock();
		if (was_playing)
			divoom_leave_msg_record_stop_preview_playback();
		return;
	}

	c = (Pdivoom_lvgl_top_leave_msg_notice_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_top_leave_msg_notice_ctrl));
	if (!c) {
		divoom_free_lvgl_lock();
		return;
	}
	memset(c, 0, sizeof(*c));

	root = (lv_obj_t *)divoom_system_msg_notice_add(w, h, divoom_lvgl_top_leave_msg_notice_menu_notice, c);
	if (!root) {
		divoom_lvgl_top_leave_msg_notice_del(c);
		divoom_free_lvgl_lock();
		return;
	}
	c->main_obj = root;
	s_leave_notice = c;
	c->dur_total_sec = (uint32)duration_sec;

	divoom_lvgl_top_leave_msg_notice_apply_card_style(root);

	/* 左列：图标 32 槽 + 时长 32，与 480-main .leftCol 一致 */
	left_col = divoom_lvgl_top_leave_msg_notice_make_slot(root,
			DIVOOM_ASTRO_LEAVE_NOTICE_PAD_X, DIVOOM_ASTRO_LEAVE_NOTICE_PAD_Y,
			DIVOOM_ASTRO_LEAVE_NOTICE_COL_W, DIVOOM_ASTRO_LEAVE_NOTICE_MSG_H);
	lv_obj_set_flex_flow(left_col, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(left_col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
			      LV_FLEX_ALIGN_CENTER);

	icon_slot = divoom_lvgl_top_leave_msg_notice_make_slot(left_col, 0, 0,
			DIVOOM_ASTRO_LEAVE_NOTICE_COL_W, DIVOOM_ASTRO_LEAVE_NOTICE_ICON_SZ);
	divoom_lvgl_top_leave_msg_notice_slot_flex_center(icon_slot);
	icon = lv_img_create(icon_slot);
	divoom_menu_control_lv_img_set_src(icon, LEAVE_MSG_ICON_PATH);
	lv_obj_set_size(icon, LEAVE_MSG_ICON_NATIVE_PX, LEAVE_MSG_ICON_NATIVE_PX);

	dur_slot = divoom_lvgl_top_leave_msg_notice_make_slot(left_col, 0, 0,
			DIVOOM_ASTRO_LEAVE_NOTICE_COL_W, DIVOOM_ASTRO_LEAVE_NOTICE_DUR_H);
	divoom_lvgl_top_leave_msg_notice_slot_flex_center(dur_slot);
	c->dur_label = lv_label_create(dur_slot);
	divoom_lvgl_top_leave_msg_notice_setup_label(c->dur_label, LEAVE_NOTICE_TITLE_FONT_PX,
			0x000000, 0, DIVOOM_ASTRO_LEAVE_NOTICE_COL_W);
	divoom_lvgl_top_leave_msg_notice_set_dur_color(c, 0);

	/* 右列：文案在 304×64 内水平+垂直居中；折行后压行距，上下留边 */
	msg_slot = divoom_lvgl_top_leave_msg_notice_make_slot(root,
			DIVOOM_ASTRO_LEAVE_NOTICE_MSG_X, DIVOOM_ASTRO_LEAVE_NOTICE_PAD_Y,
			DIVOOM_ASTRO_LEAVE_NOTICE_MSG_W, DIVOOM_ASTRO_LEAVE_NOTICE_MSG_H);
	divoom_lvgl_top_leave_msg_notice_slot_flex_center(msg_slot);
	c->title_label = lv_label_create(msg_slot);
	divoom_lvgl_top_leave_msg_notice_setup_label(c->title_label, LEAVE_NOTICE_TITLE_FONT_PX,
			0x000000, 1, DIVOOM_ASTRO_LEAVE_NOTICE_MSG_W);
	divoom_lvgl_top_leave_msg_notice_set_title(c->title_label);
	divoom_lvgl_top_leave_msg_notice_set_dur_total(c);

	divoom_system_msg_notice_set_pre_key_cb((void *)root, divoom_lvgl_top_leave_msg_notice_pre_key);
	divoom_free_lvgl_lock();
}

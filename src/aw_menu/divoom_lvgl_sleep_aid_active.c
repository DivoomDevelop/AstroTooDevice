/**
 * 助眠音源播放页：响应 DIVOOM_SLEEP_AID_MSG_ENTER_PLAY_VOICE。
 * 首次播放进入参数/当前选中音源；NEXT/PRE 在收藏列表中切换。
 */
#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_menu_direct_ctrl.h"
#include "divoom_lvgl_sleep_aid_active.h"
#include "divoom_lvgl_top_bright_volume_video.h"
#include "divoom_sleep_aid_config.h"
#include "divoom_sleep_aid_ctrl.h"
#include "divoom_astrootoo_theme.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_pixel_format.h"

#include <stdio.h>
#include <string.h>

#if !defined(DIVOOM_SIMULATOR) || !(DIVOOM_SIMULATOR)
extern void *sunxifb_alloc(size_t size, char *label);
extern void sunxifb_free(void **data, char *label);
extern void sunxifb_flush_cache(void *data, size_t size);
extern void *sunxifb_mem_get_phyaddr(void *data);
#endif

/* AstroTooUI toastTopSleepEnter + 同槽定时 HUD（MasterGo 1731:018554） */
#define SA_ACTIVE_TOP_HINT_AUTO_MS  2600
#define SA_ACTIVE_TOP_HUD_AUTO_MS   3000
#define SA_ACTIVE_TOP_PILL_X        31
#define SA_ACTIVE_TOP_PILL_Y        28
#define SA_ACTIVE_TOP_PILL_W        (DIVOOM_SCREEN_WIDTH - SA_ACTIVE_TOP_PILL_X * 2)
#define SA_ACTIVE_TOP_PILL_H        58
#define SA_ACTIVE_TOP_PILL_PAD_X    14
#define SA_ACTIVE_TOP_PILL_PAD_Y    13
#define SA_ACTIVE_TOP_ICON_SZ       28
#define SA_ACTIVE_TOP_ICON_GAP      10
#define SA_ACTIVE_TOP_COUNT_W       82
#define SA_ACTIVE_TOP_COUNT_H       32
#define SA_ACTIVE_TOP_HUD_X         353
#define SA_ACTIVE_TOP_HUD_Y         41
#define SA_ACTIVE_TOP_PILL_BG       0x8D949C
#define SA_ACTIVE_TOP_COUNT_COLOR   0x3D3D3D
#define SA_ACTIVE_TOP_HUD_COLOR     0x757575
#define SA_ACTIVE_TOP_UI_DIR        "107/"
#define SA_ACTIVE_TOP_ICON_PATH     SA_ACTIVE_TOP_UI_DIR "sleep_active_enter_mode.png"
#define SA_ACTIVE_SHORTCUT_SZ       42
#define SA_ACTIVE_SHORTCUT_TOP      20
#define SA_ACTIVE_SHORTCUT_RIGHT    20
#define SA_ACTIVE_SHORTCUT_AUTO_MS  5000
#define SA_ACTIVE_SHORTCUT_ICON     SA_ACTIVE_TOP_UI_DIR "sleep_shortcut_timer_focus.png"
#define SA_ACTIVE_PICKER_MAX        6
#define SA_ACTIVE_PICKER_VISIBLE    4
#define SA_ACTIVE_PICKER_ROW_H      62
#define SA_ACTIVE_PICKER_ROW_GAP    6
#define SA_ACTIVE_PICKER_CHECK_SZ   24
#define SA_ACTIVE_PICKER_ROW_PAD    20
#define SA_ACTIVE_PICKER_CHECK_GAP  8
#define SA_ACTIVE_PICKER_LIST_H     (SA_ACTIVE_PICKER_VISIBLE * SA_ACTIVE_PICKER_ROW_H \
				     + (SA_ACTIVE_PICKER_VISIBLE - 1) * SA_ACTIVE_PICKER_ROW_GAP)
#define SA_ACTIVE_CHECK_ICON       "23/at_repeat_check.png"

typedef struct _divoom_lvgl_sleep_aid_active_ctrl
{
	int menu_id;
	int voice_type;
	int voice_id;
	int video_type;
	int fav_idx;
	divoom_sleep_aid_fav_item *fav_list;
	int fav_count;
	uint8 first_play_done;
	lv_style_t root_style;
	lv_style_t top_hint_pill_style;
	lv_obj_t *main_obj;
	lv_obj_t *focus_btn;
	lv_obj_t *title_lbl;
	lv_obj_t *top_hint_cont;
	lv_obj_t *top_hint_pill;
	lv_obj_t *top_hint_title_lbl;
	lv_obj_t *top_hint_count_lbl;
	lv_obj_t *top_hint_hud;
	lv_obj_t *top_hint_hud_lbl;
	lv_obj_t *shortcut_cont;
	lv_obj_t *shortcut_img;
	lv_obj_t *picker_overlay;
	lv_obj_t *picker_shell;
	lv_obj_t *picker_list;
	lv_obj_t *picker_row[SA_ACTIVE_PICKER_MAX];
	lv_obj_t *picker_check[SA_ACTIVE_PICKER_MAX];
	void *picker_direct;
	int picker_count;
	int picker_focus;
	int picker_minutes[SA_ACTIVE_PICKER_MAX];
	lv_img_dsc_t picker_check_dsc;
	lv_style_t picker_row_style;
	lv_style_t picker_row_focus_style;
	uint8 picker_styles_inited;
	lv_timer_t *top_hint_hide_timer;
	lv_timer_t *top_hint_hud_hide_timer;
	lv_timer_t *top_hint_count_timer;
	lv_timer_t *shortcut_hide_timer;
	void *top_hint_osd_buf[2];
	uint32 top_hint_osd_buf_size;
	uint8 top_hint_osd_front_index;
	uint8 top_hint_osd_enabled;
	void *enter_param;
} divoom_lvgl_sleep_aid_active_ctrl, *Pdivoom_lvgl_sleep_aid_active_ctrl;

static void sleep_aid_active_fav_free(Pdivoom_lvgl_sleep_aid_active_ctrl ph);
static void sleep_aid_active_top_hint_stop_timers(Pdivoom_lvgl_sleep_aid_active_ctrl ph);
static void sleep_aid_active_top_hint_refresh_count(Pdivoom_lvgl_sleep_aid_active_ctrl ph);
static lv_obj_t *sleep_aid_active_top_hint_osd_src(Pdivoom_lvgl_sleep_aid_active_ctrl ph);
static void sleep_aid_active_top_hint_update_osd(Pdivoom_lvgl_sleep_aid_active_ctrl ph);
static void sleep_aid_active_top_hint_release_osd(Pdivoom_lvgl_sleep_aid_active_ctrl ph);
static void sleep_aid_active_top_hint_hide(Pdivoom_lvgl_sleep_aid_active_ctrl ph);
static void sleep_aid_active_top_hint_switch_to_hud(Pdivoom_lvgl_sleep_aid_active_ctrl ph);
static void sleep_aid_active_top_hint_pulse_hud(Pdivoom_lvgl_sleep_aid_active_ctrl ph);
static void sleep_aid_active_top_hint_show(Pdivoom_lvgl_sleep_aid_active_ctrl ph);
static void sleep_aid_active_top_hint_create(Pdivoom_lvgl_sleep_aid_active_ctrl ph);
static void sleep_aid_active_shortcut_hide(Pdivoom_lvgl_sleep_aid_active_ctrl ph);
static void sleep_aid_active_shortcut_show(Pdivoom_lvgl_sleep_aid_active_ctrl ph);
static void sleep_aid_active_picker_hide(Pdivoom_lvgl_sleep_aid_active_ctrl ph, int pulse_hud);
static void sleep_aid_active_picker_show(Pdivoom_lvgl_sleep_aid_active_ctrl ph);
static void sleep_aid_active_hide_all_overlays(Pdivoom_lvgl_sleep_aid_active_ctrl ph);

static const int s_sa_timer_pre[SA_ACTIVE_PICKER_MAX] = { 15, 30, 60, 90, 120, 0 };
static const int s_sa_timer_post[SA_ACTIVE_PICKER_MAX] = { 30, 60, 120, 240, 360, 480 };

static void sleep_aid_active_fav_free(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	if (!ph)
	{
		return;
	}
	if (ph->fav_list)
	{
		divoom_system_free(ph->fav_list);
	}
	ph->fav_list = NULL;
	ph->fav_count = 0;
	ph->fav_idx = -1;
}

static void sleep_aid_active_fav_reload(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	int n;

	if (!ph)
	{
		return;
	}
	sleep_aid_active_fav_free(ph);
	ph->fav_list = divoom_sleep_aid_fav_get_list(&n);
	ph->fav_count = (ph->fav_list && n > 0) ? n : 0;
}

static int sleep_aid_active_fav_find_idx(Pdivoom_lvgl_sleep_aid_active_ctrl ph, int voice_type, int voice_id)
{
	int i;

	if (!ph || !ph->fav_list || ph->fav_count <= 0 || voice_id <= 0)
	{
		return -1;
	}
	for (i = 0; i < ph->fav_count; i++)
	{
		if ((int)ph->fav_list[i].audio_id == voice_id && ph->fav_list[i].audio_type == voice_type)
		{
			return i;
		}
	}
	return -1;
}

static void sleep_aid_active_refresh_title(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	(void)ph;
}

static void sleep_aid_active_top_hint_stop_timers(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	if (!ph)
	{
		return;
	}
	if (ph->top_hint_hide_timer)
	{
		lv_timer_del(ph->top_hint_hide_timer);
		ph->top_hint_hide_timer = NULL;
	}
	if (ph->top_hint_hud_hide_timer)
	{
		lv_timer_del(ph->top_hint_hud_hide_timer);
		ph->top_hint_hud_hide_timer = NULL;
	}
	if (ph->top_hint_count_timer)
	{
		lv_timer_del(ph->top_hint_count_timer);
		ph->top_hint_count_timer = NULL;
	}
	if (ph->shortcut_hide_timer)
	{
		lv_timer_del(ph->shortcut_hide_timer);
		ph->shortcut_hide_timer = NULL;
	}
}

static void sleep_aid_active_top_hint_disable_osd(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	if (!ph || !ph->top_hint_osd_enabled)
	{
		return;
	}
	divoom_menu_control_disable_video_ui_overlay();
	ph->top_hint_osd_enabled = 0;
}

static void sleep_aid_active_top_hint_release_osd(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	int i;

	if (!ph)
	{
		return;
	}
	sleep_aid_active_top_hint_disable_osd(ph);
#if !defined(DIVOOM_SIMULATOR) || !(DIVOOM_SIMULATOR)
	for (i = 0; i < 2; i++)
	{
		if (ph->top_hint_osd_buf[i])
		{
			sunxifb_free(&ph->top_hint_osd_buf[i],
				i == 0 ? "sleep_top_osd0" : "sleep_top_osd1");
		}
	}
#else
	(void)i;
#endif
	ph->top_hint_osd_buf_size = 0;
	ph->top_hint_osd_front_index = 0;
}

static int sleep_aid_active_shortcut_visible(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	return (ph && ph->shortcut_cont &&
		!lv_obj_has_flag(ph->shortcut_cont, LV_OBJ_FLAG_HIDDEN)) ? 1 : 0;
}

static int sleep_aid_active_picker_visible(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	return (ph && ph->picker_overlay &&
		!lv_obj_has_flag(ph->picker_overlay, LV_OBJ_FLAG_HIDDEN)) ? 1 : 0;
}

static lv_obj_t *sleep_aid_active_top_hint_osd_src(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	if (!ph)
	{
		return NULL;
	}
	if (sleep_aid_active_picker_visible(ph) && ph->picker_shell)
	{
		return ph->picker_shell;
	}
	if (sleep_aid_active_shortcut_visible(ph))
	{
		return ph->shortcut_cont;
	}
	if (!ph->top_hint_cont ||
		lv_obj_has_flag(ph->top_hint_cont, LV_OBJ_FLAG_HIDDEN))
	{
		return NULL;
	}
	if (ph->top_hint_pill &&
		!lv_obj_has_flag(ph->top_hint_pill, LV_OBJ_FLAG_HIDDEN))
	{
		return ph->top_hint_pill;
	}
	if (ph->top_hint_hud &&
		!lv_obj_has_flag(ph->top_hint_hud, LV_OBJ_FLAG_HIDDEN))
	{
		return ph->top_hint_hud;
	}
	return NULL;
}

static void sleep_aid_active_top_hint_update_osd(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
#if !defined(DIVOOM_SIMULATOR) || !(DIVOOM_SIMULATOR)
	lv_draw_buf_t draw_buf;
	lv_area_t coords;
	lv_obj_t *osd_src;
	void *back_buf;
	void *phy_addr;
	uint32 need_size;
	uint32 image_w;
	uint32 image_h;
	uint32 y;
	uint8 back_index;
	int i;
	int ret;

	osd_src = sleep_aid_active_top_hint_osd_src(ph);
	if (!osd_src)
	{
		return;
	}

	lv_obj_update_layout(osd_src);
	/* The overlay src has no shadow or outline, so its extended draw size is 0. */
	image_w = (uint32)lv_obj_get_width(osd_src);
	image_h = (uint32)lv_obj_get_height(osd_src);
	if (image_w == 0 || image_h == 0 ||
		image_w > DIVOOM_SCREEN_WIDTH || image_h > DIVOOM_SCREEN_HIGH)
	{
		divoom_print("sleep TOP OSD invalid size: %ux%u\n", image_w, image_h);
		return;
	}

	need_size = LV_DRAW_BUF_SIZE(image_w, image_h, LV_COLOR_FORMAT_ARGB8888);
	if (!ph->top_hint_osd_buf[0] || !ph->top_hint_osd_buf[1] ||
		ph->top_hint_osd_buf_size < need_size)
	{
		sleep_aid_active_top_hint_release_osd(ph);
		for (i = 0; i < 2; i++)
		{
			ph->top_hint_osd_buf[i] = sunxifb_alloc(need_size,
				i == 0 ? "sleep_top_osd0" : "sleep_top_osd1");
			if (!ph->top_hint_osd_buf[i])
			{
				divoom_print("sleep TOP OSD buffer:%d alloc fail: %u\n", i, need_size);
				sleep_aid_active_top_hint_release_osd(ph);
				return;
			}
		}
		ph->top_hint_osd_buf_size = need_size;
	}

	back_index = ph->top_hint_osd_enabled ?
		(uint8)(ph->top_hint_osd_front_index ^ 1U) : 0;
	back_buf = ph->top_hint_osd_buf[back_index];
	memset(back_buf, 0, ph->top_hint_osd_buf_size);
	ret = lv_draw_buf_init(&draw_buf, image_w, image_h,
		LV_COLOR_FORMAT_ARGB8888, LV_STRIDE_AUTO,
		back_buf, ph->top_hint_osd_buf_size);
	if (ret != LV_RES_OK)
	{
		divoom_print("sleep TOP OSD draw buffer init fail: %d\n", ret);
		return;
	}
	ret = lv_snapshot_take_to_draw_buf(osd_src,
		LV_COLOR_FORMAT_ARGB8888, &draw_buf);
	if (ret != LV_RES_OK)
	{
		divoom_print("sleep TOP OSD snapshot fail: %d\n", ret);
		return;
	}

	image_w = draw_buf.header.w;
	image_h = draw_buf.header.h;
	for (y = 0; y < image_h; y++)
	{
		lv_color32_t *src = (lv_color32_t *)((uint8_t *)draw_buf.data +
			y * draw_buf.header.stride);
		uint16_t *dst = (uint16_t *)draw_buf.data + y * image_w;
		uint32 x;

		for (x = 0; x < image_w; x++)
		{
			lv_color32_t pixel = src[x];
			dst[x] = (pixel.alpha >= 0x80U ? 0x8000U : 0U) |
				((uint16_t)(pixel.red >> 3) << 10) |
				((uint16_t)(pixel.green >> 3) << 5) |
				(uint16_t)(pixel.blue >> 3);
		}
	}

	lv_obj_get_coords(osd_src, &coords);
	phy_addr = sunxifb_mem_get_phyaddr(draw_buf.data);
	if (!phy_addr)
	{
		divoom_print("sleep TOP OSD physical address fail\n");
		return;
	}
	sunxifb_flush_cache(draw_buf.data,
		image_w * image_h * (uint32)sizeof(uint16_t));
	ret = divoom_menu_control_set_video_ui_overlay_argb1555(phy_addr,
		coords.x1, coords.y1,
		image_w, image_h);
	if (ret != 0)
	{
		divoom_print("sleep TOP OSD layer update fail: %d\n", ret);
		return;
	}
	ph->top_hint_osd_front_index = back_index;
	ph->top_hint_osd_enabled = 1;
	divoom_print("sleep TOP OSD updated: %ux%u index:%u\n",
		image_w, image_h, back_index);
#else
	(void)ph;
#endif
}

static void sleep_aid_active_top_hint_format_count(char *buf, int buflen, int remain_sec)
{
	int min;
	int sec;

	if (!buf || buflen <= 0)
	{
		return;
	}
	if (remain_sec < 0)
	{
		snprintf(buf, (size_t)buflen, "--:--");
		return;
	}
	if (remain_sec > 5999 * 60 + 59)
	{
		remain_sec = 5999 * 60 + 59;
	}
	min = remain_sec / 60;
	sec = remain_sec % 60;
	snprintf(buf, (size_t)buflen, "%d:%02d", min, sec);
}

static void sleep_aid_active_top_hint_refresh_count(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	char buf[16];
	int remain;

	if (!ph)
	{
		return;
	}
	remain = divoom_sleep_aid_ctrl_get_phase_remain_sec();
	sleep_aid_active_top_hint_format_count(buf, (int)sizeof(buf), remain);
	if (ph->top_hint_count_lbl)
	{
		lv_label_set_text(ph->top_hint_count_lbl, buf);
	}
	if (ph->top_hint_hud_lbl)
	{
		lv_label_set_text(ph->top_hint_hud_lbl, buf);
	}
}

static void sleep_aid_active_top_hint_count_timer_cb(lv_timer_t *t)
{
	Pdivoom_lvgl_sleep_aid_active_ctrl ph;

	ph = (Pdivoom_lvgl_sleep_aid_active_ctrl)lv_timer_get_user_data(t);
	if (!ph || !ph->top_hint_cont)
	{
		return;
	}
	sleep_aid_active_top_hint_refresh_count(ph);
	if (sleep_aid_active_picker_visible(ph) ||
		sleep_aid_active_shortcut_visible(ph))
	{
		return;
	}
	sleep_aid_active_top_hint_update_osd(ph);
}

static void sleep_aid_active_top_hint_hide_timer_cb(lv_timer_t *t)
{
	Pdivoom_lvgl_sleep_aid_active_ctrl ph;

	ph = (Pdivoom_lvgl_sleep_aid_active_ctrl)lv_timer_get_user_data(t);
	if (!ph)
	{
		return;
	}
	ph->top_hint_hide_timer = NULL;
	sleep_aid_active_top_hint_switch_to_hud(ph);
}

static void sleep_aid_active_top_hint_hud_hide_timer_cb(lv_timer_t *t)
{
	Pdivoom_lvgl_sleep_aid_active_ctrl ph;

	ph = (Pdivoom_lvgl_sleep_aid_active_ctrl)lv_timer_get_user_data(t);
	if (!ph)
	{
		return;
	}
	ph->top_hint_hud_hide_timer = NULL;
	sleep_aid_active_top_hint_hide(ph);
}

static void sleep_aid_active_top_hint_ensure_count_timer(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	if (!ph || ph->top_hint_count_timer)
	{
		return;
	}
	ph->top_hint_count_timer = lv_timer_create(sleep_aid_active_top_hint_count_timer_cb, 1000, ph);
	if (ph->top_hint_count_timer)
	{
		lv_timer_set_repeat_count(ph->top_hint_count_timer, -1);
	}
}

static void sleep_aid_active_top_hint_restart_hud_hide(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	if (!ph)
	{
		return;
	}
	if (ph->top_hint_hud_hide_timer)
	{
		lv_timer_del(ph->top_hint_hud_hide_timer);
		ph->top_hint_hud_hide_timer = NULL;
	}
	ph->top_hint_hud_hide_timer = lv_timer_create(sleep_aid_active_top_hint_hud_hide_timer_cb,
						     SA_ACTIVE_TOP_HUD_AUTO_MS, ph);
	if (ph->top_hint_hud_hide_timer)
	{
		lv_timer_set_repeat_count(ph->top_hint_hud_hide_timer, 1);
	}
}

static void sleep_aid_active_top_hint_hide(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	if (!ph || !ph->top_hint_cont)
	{
		return;
	}
	if (ph->top_hint_hide_timer)
	{
		lv_timer_del(ph->top_hint_hide_timer);
		ph->top_hint_hide_timer = NULL;
	}
	if (ph->top_hint_hud_hide_timer)
	{
		lv_timer_del(ph->top_hint_hud_hide_timer);
		ph->top_hint_hud_hide_timer = NULL;
	}
	if (ph->top_hint_count_timer)
	{
		lv_timer_del(ph->top_hint_count_timer);
		ph->top_hint_count_timer = NULL;
	}
	if (ph->top_hint_pill)
	{
		lv_obj_add_flag(ph->top_hint_pill, LV_OBJ_FLAG_HIDDEN);
	}
	if (ph->top_hint_hud)
	{
		lv_obj_add_flag(ph->top_hint_hud, LV_OBJ_FLAG_HIDDEN);
	}
	lv_obj_add_flag(ph->top_hint_cont, LV_OBJ_FLAG_HIDDEN);
	if (!sleep_aid_active_shortcut_visible(ph) &&
		!sleep_aid_active_picker_visible(ph))
	{
		sleep_aid_active_top_hint_disable_osd(ph);
	}
}

static void sleep_aid_active_shortcut_hide(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	if (!ph)
	{
		return;
	}
	if (ph->shortcut_hide_timer)
	{
		lv_timer_del(ph->shortcut_hide_timer);
		ph->shortcut_hide_timer = NULL;
	}
	if (ph->shortcut_cont)
	{
		lv_obj_add_flag(ph->shortcut_cont, LV_OBJ_FLAG_HIDDEN);
	}
}

static void sleep_aid_active_bind_focus(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	lv_group_t *g;

	if (!ph || !ph->focus_btn || !lv_obj_is_valid(ph->focus_btn))
	{
		return;
	}
	/* clearall 先 disp 选择页再拆本页。若此时再 focus 本页按钮，
	 * 随后 lv_obj_del 会把 default group 甩到选择页列表最后一行。 */
	if (ph->main_obj && lv_scr_act() != ph->main_obj)
	{
		return;
	}
	g = lv_group_get_default();
	if (!g)
	{
		return;
	}
	if (lv_obj_get_group(ph->focus_btn) != g)
	{
		lv_group_add_obj(g, ph->focus_btn);
	}
	lv_group_focus_obj(ph->focus_btn);
}

static void sleep_aid_active_picker_restore_focus(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	sleep_aid_active_bind_focus(ph);
}

static void sleep_aid_active_picker_hide(Pdivoom_lvgl_sleep_aid_active_ctrl ph, int pulse_hud)
{
	if (!ph)
	{
		return;
	}
	if (ph->picker_direct)
	{
		divoom_menu_direct_ctrl_destory(ph->picker_direct);
		ph->picker_direct = NULL;
	}
	if (ph->picker_overlay)
	{
		lv_obj_del(ph->picker_overlay);
		ph->picker_overlay = NULL;
		ph->picker_shell = NULL;
		ph->picker_list = NULL;
		memset(ph->picker_row, 0, sizeof(ph->picker_row));
		memset(ph->picker_check, 0, sizeof(ph->picker_check));
	}
	ph->picker_count = 0;
	ph->picker_focus = 0;
	sleep_aid_active_picker_restore_focus(ph);
	if (pulse_hud)
	{
		sleep_aid_active_top_hint_pulse_hud(ph);
		return;
	}
	sleep_aid_active_top_hint_disable_osd(ph);
}

static void sleep_aid_active_hide_all_overlays(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	if (!ph)
	{
		return;
	}
	sleep_aid_active_shortcut_hide(ph);
	sleep_aid_active_picker_hide(ph, 0);
	sleep_aid_active_top_hint_hide(ph);
	sleep_aid_active_top_hint_disable_osd(ph);
}

static void sleep_aid_active_top_hint_switch_to_hud(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	if (!ph || !ph->top_hint_cont || !ph->top_hint_hud)
	{
		sleep_aid_active_top_hint_hide(ph);
		return;
	}
	if (ph->top_hint_pill)
	{
		lv_obj_add_flag(ph->top_hint_pill, LV_OBJ_FLAG_HIDDEN);
	}
	lv_obj_clear_flag(ph->top_hint_hud, LV_OBJ_FLAG_HIDDEN);
	lv_obj_clear_flag(ph->top_hint_cont, LV_OBJ_FLAG_HIDDEN);
	sleep_aid_active_top_hint_refresh_count(ph);
	sleep_aid_active_top_hint_update_osd(ph);
}

static void sleep_aid_active_top_hint_pulse_hud(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	if (!ph || !ph->top_hint_cont)
	{
		return;
	}
	if (sleep_aid_active_shortcut_visible(ph) ||
		sleep_aid_active_picker_visible(ph))
	{
		return;
	}
	if (ph->top_hint_pill &&
		!lv_obj_has_flag(ph->top_hint_pill, LV_OBJ_FLAG_HIDDEN))
	{
		return;
	}
	divoom_lvgl_top_bright_volume_video_exit();
	if (ph->top_hint_hide_timer)
	{
		lv_timer_del(ph->top_hint_hide_timer);
		ph->top_hint_hide_timer = NULL;
	}
	if (ph->top_hint_pill)
	{
		lv_obj_add_flag(ph->top_hint_pill, LV_OBJ_FLAG_HIDDEN);
	}
	if (ph->top_hint_hud)
	{
		lv_obj_clear_flag(ph->top_hint_hud, LV_OBJ_FLAG_HIDDEN);
	}
	lv_obj_clear_flag(ph->top_hint_cont, LV_OBJ_FLAG_HIDDEN);
	lv_obj_move_foreground(ph->top_hint_cont);
	sleep_aid_active_top_hint_refresh_count(ph);
	sleep_aid_active_top_hint_update_osd(ph);
	sleep_aid_active_top_hint_ensure_count_timer(ph);
	sleep_aid_active_top_hint_restart_hud_hide(ph);
}

static void sleep_aid_active_top_hint_show(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	if (!ph || !ph->top_hint_cont)
	{
		return;
	}
	/* channel 1/layer 1 is shared by the two video TOP overlays. */
	divoom_lvgl_top_bright_volume_video_exit();
	sleep_aid_active_shortcut_hide(ph);
	sleep_aid_active_picker_hide(ph, 0);
	sleep_aid_active_top_hint_stop_timers(ph);
	if (ph->top_hint_hud)
	{
		lv_obj_add_flag(ph->top_hint_hud, LV_OBJ_FLAG_HIDDEN);
	}
	if (ph->top_hint_pill)
	{
		lv_obj_clear_flag(ph->top_hint_pill, LV_OBJ_FLAG_HIDDEN);
	}
	sleep_aid_active_top_hint_refresh_count(ph);
	lv_obj_clear_flag(ph->top_hint_cont, LV_OBJ_FLAG_HIDDEN);
	lv_obj_move_foreground(ph->top_hint_cont);
	sleep_aid_active_top_hint_update_osd(ph);
	ph->top_hint_hide_timer = lv_timer_create(sleep_aid_active_top_hint_hide_timer_cb,
						 SA_ACTIVE_TOP_HINT_AUTO_MS, ph);
	if (ph->top_hint_hide_timer)
	{
		lv_timer_set_repeat_count(ph->top_hint_hide_timer, 1);
	}
	sleep_aid_active_top_hint_restart_hud_hide(ph);
	sleep_aid_active_top_hint_ensure_count_timer(ph);
}

static void sleep_aid_active_top_hint_create(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	lv_obj_t *pill;
	lv_obj_t *hud;
	lv_obj_t *icon_img;
	lv_obj_t *title;
	lv_coord_t icon_y;
	lv_coord_t title_x;
	lv_coord_t title_w;
	lv_coord_t count_x;
	lv_coord_t count_y;
	lv_coord_t cont_h;

	if (!ph || !ph->main_obj || ph->top_hint_cont)
	{
		return;
	}

	lv_style_init(&ph->top_hint_pill_style);
	lv_style_set_bg_color(&ph->top_hint_pill_style, lv_color_hex(SA_ACTIVE_TOP_PILL_BG));
	lv_style_set_bg_opa(&ph->top_hint_pill_style, LV_OPA_COVER);
	lv_style_set_border_width(&ph->top_hint_pill_style, 0);
	lv_style_set_radius(&ph->top_hint_pill_style, DIVOOM_ASTRO_RADIUS_SM);
	lv_style_set_pad_all(&ph->top_hint_pill_style, 0);
	lv_style_set_shadow_width(&ph->top_hint_pill_style, 0);

	cont_h = SA_ACTIVE_TOP_PILL_Y + SA_ACTIVE_TOP_PILL_H;
	if (cont_h < SA_ACTIVE_TOP_HUD_Y + SA_ACTIVE_TOP_COUNT_H)
	{
		cont_h = SA_ACTIVE_TOP_HUD_Y + SA_ACTIVE_TOP_COUNT_H;
	}

	ph->top_hint_cont = lv_obj_create(lv_layer_top());
	lv_obj_add_style(ph->top_hint_cont, &ph->root_style, 0);
	lv_obj_set_size(ph->top_hint_cont, DIVOOM_SCREEN_WIDTH, cont_h);
	lv_obj_set_pos(ph->top_hint_cont, 0, 0);
	lv_obj_set_style_bg_opa(ph->top_hint_cont, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(ph->top_hint_cont, 0, 0);
	lv_obj_set_style_pad_all(ph->top_hint_cont, 0, 0);
	lv_obj_clear_flag(ph->top_hint_cont, LV_OBJ_FLAG_SCROLLABLE);

	pill = lv_obj_create(ph->top_hint_cont);
	ph->top_hint_pill = pill;
	lv_obj_add_style(pill, &ph->top_hint_pill_style, 0);
	lv_obj_set_size(pill, SA_ACTIVE_TOP_PILL_W, SA_ACTIVE_TOP_PILL_H);
	lv_obj_set_pos(pill, SA_ACTIVE_TOP_PILL_X, SA_ACTIVE_TOP_PILL_Y);
	lv_obj_clear_flag(pill, LV_OBJ_FLAG_SCROLLABLE);

	icon_y = SA_ACTIVE_TOP_PILL_PAD_Y +
		 (SA_ACTIVE_TOP_PILL_H - SA_ACTIVE_TOP_PILL_PAD_Y * 2 - SA_ACTIVE_TOP_ICON_SZ) / 2;
	count_x = SA_ACTIVE_TOP_PILL_W - SA_ACTIVE_TOP_PILL_PAD_X - SA_ACTIVE_TOP_COUNT_W;
	count_y = SA_ACTIVE_TOP_PILL_PAD_Y;
	title_x = SA_ACTIVE_TOP_PILL_PAD_X + SA_ACTIVE_TOP_ICON_SZ + SA_ACTIVE_TOP_ICON_GAP;
	title_w = count_x - SA_ACTIVE_TOP_ICON_GAP - title_x;

	icon_img = lv_img_create(pill);
	lv_obj_set_size(icon_img, SA_ACTIVE_TOP_ICON_SZ, SA_ACTIVE_TOP_ICON_SZ);
	divoom_menu_control_lv_img_set_src(icon_img, SA_ACTIVE_TOP_ICON_PATH);
	lv_obj_set_pos(icon_img, SA_ACTIVE_TOP_PILL_PAD_X, icon_y);

	title = lv_label_create(pill);
	lv_label_set_text(title, divoom_menu_control_get_disp_str("sleepEnterStub"));
	lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(24, ph->menu_id), 0);
	lv_obj_set_style_text_color(title, lv_color_hex(0x000000), 0);
	lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
	lv_obj_set_size(title, title_w, SA_ACTIVE_TOP_COUNT_H);
	lv_obj_set_pos(title, title_x, count_y);
	ph->top_hint_title_lbl = title;

	ph->top_hint_count_lbl = lv_label_create(pill);
	lv_obj_set_style_text_font(ph->top_hint_count_lbl, divoom_menu_control_get_font_info(24, ph->menu_id), 0);
	lv_obj_set_style_text_color(ph->top_hint_count_lbl, lv_color_hex(SA_ACTIVE_TOP_COUNT_COLOR), 0);
	lv_obj_set_style_text_align(ph->top_hint_count_lbl, LV_TEXT_ALIGN_RIGHT, 0);
	lv_obj_set_size(ph->top_hint_count_lbl, SA_ACTIVE_TOP_COUNT_W, SA_ACTIVE_TOP_COUNT_H);
	lv_obj_set_pos(ph->top_hint_count_lbl, count_x, count_y);

	hud = lv_obj_create(ph->top_hint_cont);
	ph->top_hint_hud = hud;
	lv_obj_set_size(hud, SA_ACTIVE_TOP_COUNT_W, SA_ACTIVE_TOP_COUNT_H);
	lv_obj_set_pos(hud, SA_ACTIVE_TOP_HUD_X, SA_ACTIVE_TOP_HUD_Y);
	lv_obj_set_style_bg_opa(hud, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(hud, 0, 0);
	lv_obj_set_style_outline_width(hud, 0, 0);
	lv_obj_set_style_pad_all(hud, 0, 0);
	lv_obj_set_style_radius(hud, 0, 0);
	lv_obj_set_style_shadow_width(hud, 0, 0);
	lv_obj_clear_flag(hud, LV_OBJ_FLAG_SCROLLABLE);

	ph->top_hint_hud_lbl = lv_label_create(hud);
	lv_obj_set_style_text_font(ph->top_hint_hud_lbl, divoom_menu_control_get_font_info(24, ph->menu_id), 0);
	lv_obj_set_style_text_color(ph->top_hint_hud_lbl, lv_color_hex(SA_ACTIVE_TOP_HUD_COLOR), 0);
	lv_obj_set_style_text_align(ph->top_hint_hud_lbl, LV_TEXT_ALIGN_RIGHT, 0);
	lv_obj_set_size(ph->top_hint_hud_lbl, SA_ACTIVE_TOP_COUNT_W, SA_ACTIVE_TOP_COUNT_H);
	lv_obj_set_pos(ph->top_hint_hud_lbl, 0, 0);
	lv_obj_add_flag(hud, LV_OBJ_FLAG_HIDDEN);

	lv_obj_add_flag(ph->top_hint_cont, LV_OBJ_FLAG_HIDDEN);
}

static void sleep_aid_active_shortcut_hide_timer_cb(lv_timer_t *t)
{
	Pdivoom_lvgl_sleep_aid_active_ctrl ph;

	ph = (Pdivoom_lvgl_sleep_aid_active_ctrl)lv_timer_get_user_data(t);
	if (!ph)
	{
		return;
	}
	ph->shortcut_hide_timer = NULL;
	sleep_aid_active_shortcut_hide(ph);
	if (!sleep_aid_active_picker_visible(ph) &&
		(!ph->top_hint_cont || lv_obj_has_flag(ph->top_hint_cont, LV_OBJ_FLAG_HIDDEN)))
	{
		sleep_aid_active_top_hint_disable_osd(ph);
	}
}

static void sleep_aid_active_shortcut_create(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	if (!ph || ph->shortcut_cont)
	{
		return;
	}

	ph->shortcut_cont = lv_obj_create(lv_layer_top());
	lv_obj_remove_style_all(ph->shortcut_cont);
	lv_obj_set_size(ph->shortcut_cont, SA_ACTIVE_SHORTCUT_SZ, SA_ACTIVE_SHORTCUT_SZ);
	lv_obj_set_pos(ph->shortcut_cont,
		DIVOOM_SCREEN_WIDTH - SA_ACTIVE_SHORTCUT_RIGHT - SA_ACTIVE_SHORTCUT_SZ,
		SA_ACTIVE_SHORTCUT_TOP);
	lv_obj_set_style_bg_opa(ph->shortcut_cont, LV_OPA_TRANSP, 0);
	lv_obj_set_style_pad_all(ph->shortcut_cont, 0, 0);
	lv_obj_set_style_border_width(ph->shortcut_cont, 0, 0);
	lv_obj_clear_flag(ph->shortcut_cont, LV_OBJ_FLAG_SCROLLABLE);

	ph->shortcut_img = lv_img_create(ph->shortcut_cont);
	lv_obj_set_size(ph->shortcut_img, SA_ACTIVE_SHORTCUT_SZ, SA_ACTIVE_SHORTCUT_SZ);
	divoom_menu_control_lv_img_set_src(ph->shortcut_img, SA_ACTIVE_SHORTCUT_ICON);
	lv_obj_set_pos(ph->shortcut_img, 0, 0);
	lv_obj_add_flag(ph->shortcut_cont, LV_OBJ_FLAG_HIDDEN);
}

static void sleep_aid_active_shortcut_show(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	if (!ph)
	{
		return;
	}
	divoom_lvgl_top_bright_volume_video_exit();
	sleep_aid_active_picker_hide(ph, 0);
	sleep_aid_active_top_hint_hide(ph);
	sleep_aid_active_shortcut_create(ph);
	if (!ph->shortcut_cont)
	{
		return;
	}
	if (ph->shortcut_hide_timer)
	{
		lv_timer_del(ph->shortcut_hide_timer);
		ph->shortcut_hide_timer = NULL;
	}
	lv_obj_clear_flag(ph->shortcut_cont, LV_OBJ_FLAG_HIDDEN);
	lv_obj_move_foreground(ph->shortcut_cont);
	sleep_aid_active_top_hint_update_osd(ph);
	ph->shortcut_hide_timer = lv_timer_create(sleep_aid_active_shortcut_hide_timer_cb,
						 SA_ACTIVE_SHORTCUT_AUTO_MS, ph);
	if (ph->shortcut_hide_timer)
	{
		lv_timer_set_repeat_count(ph->shortcut_hide_timer, 1);
	}
}

static void sleep_aid_active_fmt_timer(char *buf, int sz, int minutes)
{
	if (!buf || sz <= 0)
	{
		return;
	}
	if (minutes <= 0)
	{
		snprintf(buf, (unsigned)sz, "%s",
			 divoom_menu_control_get_disp_str("SleepAidTimerOff"));
		return;
	}
	snprintf(buf, (unsigned)sz, "%d %s", minutes,
		 divoom_menu_control_get_disp_str("SleepAidTimerUnit"));
}

static void sleep_aid_active_free_check_dsc(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	if (!ph || !ph->picker_check_dsc.data)
	{
		return;
	}
	divoom_system_free((void *)ph->picker_check_dsc.data);
	ph->picker_check_dsc.data = NULL;
	ph->picker_check_dsc.data_size = 0;
}

#if !DIVOOM_LVGL9_IS_SIM
static void sleep_aid_active_sunxi_abgr_to_rgb565a8(const uint8 *src_abgr,
	uint8 *dst_rgb565a8, uint32_t px_count)
{
	uint8 *alpha;
	uint32_t i;

	if (!src_abgr || !dst_rgb565a8)
	{
		return;
	}
	alpha = dst_rgb565a8 + px_count * DIVOOM_APP_RGB565_BYTES_PER_PX;
	for (i = 0; i < px_count; i++)
	{
		const uint8 *src = src_abgr + i * 4U;
		uint16 rgb565;

		rgb565 = divoom_rgb888_to_rgb565(src[3], src[2], src[1]);
		divoom_store_rgb565_le(dst_rgb565a8 + i * DIVOOM_APP_RGB565_BYTES_PER_PX,
			rgb565);
		alpha[i] = src[0];
	}
}
#endif

/* 与主菜单顶栏相同：进页时把 PNG 解到 RAM，overlay 截图不再走 SUNXI PNG 硬解。 */
static int sleep_aid_active_cache_check_png(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	char path_buf[160];
	lv_img_decoder_dsc_t decoder_dsc;
	lv_img_header_t header;
	lv_img_dsc_t src_dsc;
	uint32_t size;

	if (!ph)
	{
		return 0;
	}
	sleep_aid_active_free_check_dsc(ph);
	divoom_menu_control_get_img_path(SA_ACTIVE_CHECK_ICON, path_buf);
	if (lv_img_decoder_open(&decoder_dsc, path_buf, lv_color_hex(0), 0) != LV_RES_OK)
	{
		divoom_print("sleep chk cache: decoder open fail\n");
		return 0;
	}
	if (lv_img_decoder_get_info(path_buf, &header) != LV_RES_OK ||
	    decoder_dsc.img_data == NULL)
	{
		lv_img_decoder_close(&decoder_dsc);
		divoom_print("sleep chk cache: decoder info fail\n");
		return 0;
	}
	size = divoom_pixel_format_size((uint32_t)header.w, (uint32_t)header.h,
		LV_COLOR_FORMAT_RGB565A8);
	if (size == 0)
	{
		lv_img_decoder_close(&decoder_dsc);
		return 0;
	}
	ph->picker_check_dsc.data = divoom_system_malloc(size);
	if (!ph->picker_check_dsc.data)
	{
		lv_img_decoder_close(&decoder_dsc);
		divoom_print("sleep chk cache: malloc fail %u\n", (unsigned)size);
		return 0;
	}
#if DIVOOM_LVGL9_IS_SIM
	divoom_lv_img_dsc_init_fmt(&src_dsc, header.w, header.h,
		(lv_color_format_t)decoder_dsc.header.cf, decoder_dsc.img_data);
	if (!divoom_lv_img_dsc_convert_buf(&src_dsc, (uint8 *)ph->picker_check_dsc.data,
		LV_COLOR_FORMAT_RGB565A8))
	{
		divoom_system_free((void *)ph->picker_check_dsc.data);
		ph->picker_check_dsc.data = NULL;
		lv_img_decoder_close(&decoder_dsc);
		return 0;
	}
#else
	{
		const char *decoder_name = "unknown";

		if (decoder_dsc.dsc.decoder && decoder_dsc.dsc.decoder->name)
		{
			decoder_name = decoder_dsc.dsc.decoder->name;
		}
		if (strcmp(decoder_name, "SUNXI_PNG") == 0)
		{
			sleep_aid_active_sunxi_abgr_to_rgb565a8(decoder_dsc.img_data,
				(uint8 *)ph->picker_check_dsc.data,
				(uint32_t)header.w * (uint32_t)header.h);
		}
		else
		{
			divoom_lv_img_dsc_init_fmt(&src_dsc, header.w, header.h,
				(lv_color_format_t)decoder_dsc.header.cf, decoder_dsc.img_data);
			if (!divoom_lv_img_dsc_convert_buf(&src_dsc,
				(uint8 *)ph->picker_check_dsc.data, LV_COLOR_FORMAT_RGB565A8))
			{
				divoom_system_free((void *)ph->picker_check_dsc.data);
				ph->picker_check_dsc.data = NULL;
				lv_img_decoder_close(&decoder_dsc);
				divoom_print("sleep chk cache: convert fail decoder=%s\n",
					     decoder_name);
				return 0;
			}
		}
	}
#endif
	divoom_lv_img_dsc_init_fmt(&ph->picker_check_dsc, header.w, header.h,
		LV_COLOR_FORMAT_RGB565A8, ph->picker_check_dsc.data);
#if !DIVOOM_LVGL9_IS_SIM
	/* 仿真图像头没有 stride 字段，保留真机的显式行跨度设置。 */
	ph->picker_check_dsc.header.stride = divoom_pixel_format_stride((uint32_t)header.w,
		LV_COLOR_FORMAT_RGB565A8);
#endif
	lv_img_decoder_close(&decoder_dsc);
	return 1;
}

static void sleep_aid_active_picker_refresh_checks(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	divoom_sleep_aid_info *inf;
	int committed;
	int i;

	if (!ph)
	{
		return;
	}
	inf = divoom_sleep_aid_ctrl_get_settings();
	committed = -1;
	if (inf)
	{
		int cur;

		cur = (divoom_sleep_aid_ctrl_get_play_time_phase() ==
		       DIVOOM_SLEEP_AID_PLAY_TIME_PHASE_AFTER) ?
			inf->after_time : inf->before_time;
		for (i = 0; i < ph->picker_count; i++)
		{
			if (ph->picker_minutes[i] == cur)
			{
				committed = i;
				break;
			}
		}
	}
	for (i = 0; i < ph->picker_count; i++)
	{
		if (!ph->picker_check[i])
		{
			continue;
		}
		if (i == committed)
		{
			lv_obj_clear_flag(ph->picker_check[i], LV_OBJ_FLAG_HIDDEN);
		}
		else
		{
			lv_obj_add_flag(ph->picker_check[i], LV_OBJ_FLAG_HIDDEN);
		}
	}
}

static void sleep_aid_active_picker_scroll_to_focus(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	int n;
	int idx;
	int first;
	int last;
	int max_first;
	lv_coord_t step;
	lv_coord_t y;

	if (!ph || !ph->picker_list || ph->picker_count <= 0)
	{
		return;
	}
	n = ph->picker_count;
	idx = ph->picker_focus;
	if (idx < 0)
	{
		idx = 0;
	}
	if (idx >= n)
	{
		idx = n - 1;
	}
	max_first = n - SA_ACTIVE_PICKER_VISIBLE;
	if (max_first < 0)
	{
		max_first = 0;
	}
	step = SA_ACTIVE_PICKER_ROW_H + SA_ACTIVE_PICKER_ROW_GAP;
	y = lv_obj_get_scroll_y(ph->picker_list);
	first = (step > 0) ? ((y + step / 2) / step) : 0;
	if (first < 0)
	{
		first = 0;
	}
	if (first > max_first)
	{
		first = max_first;
	}
	last = first + SA_ACTIVE_PICKER_VISIBLE - 1;
	if (idx < first)
	{
		first = idx;
	}
	else if (idx > last)
	{
		first = idx - (SA_ACTIVE_PICKER_VISIBLE - 1);
	}
	if (first < 0)
	{
		first = 0;
	}
	if (first > max_first)
	{
		first = max_first;
	}
	lv_obj_scroll_to_y(ph->picker_list, first * step, LV_ANIM_OFF);
	lv_obj_update_layout(ph->picker_list);
}

static void sleep_aid_active_picker_set_focus(Pdivoom_lvgl_sleep_aid_active_ctrl ph, int idx)
{
	int i;
	int n;

	if (!ph || ph->picker_count <= 0)
	{
		return;
	}
	n = ph->picker_count;
	idx %= n;
	if (idx < 0)
	{
		idx += n;
	}
	ph->picker_focus = idx;
	for (i = 0; i < n; i++)
	{
		if (!ph->picker_row[i])
		{
			continue;
		}
		if (i == idx)
		{
			lv_obj_add_state(ph->picker_row[i], LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
		}
		else
		{
			lv_obj_clear_state(ph->picker_row[i], LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
		}
	}
	sleep_aid_active_picker_scroll_to_focus(ph);
	sleep_aid_active_bind_focus(ph);
	if (ph->picker_overlay)
	{
		lv_obj_update_layout(ph->picker_overlay);
	}
	sleep_aid_active_top_hint_update_osd(ph);
}

static void sleep_aid_active_picker_commit(Pdivoom_lvgl_sleep_aid_active_ctrl ph, int idx)
{
	if (!ph || idx < 0 || idx >= ph->picker_count)
	{
		sleep_aid_active_picker_hide(ph, 0);
		return;
	}
	divoom_sleep_aid_ctrl_set_phase_timer_minutes(ph->picker_minutes[idx]);
	sleep_aid_active_top_hint_refresh_count(ph);
	sleep_aid_active_picker_hide(ph, 1);
}

static void sleep_aid_active_picker_ensure_styles(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	if (!ph || ph->picker_styles_inited)
	{
		return;
	}
	lv_style_init(&ph->picker_row_style);
	lv_style_set_bg_color(&ph->picker_row_style, lv_color_hex(DIVOOM_ASTRO_SURFACE_IDLE));
	lv_style_set_bg_opa(&ph->picker_row_style, LV_OPA_COVER);
	lv_style_set_border_color(&ph->picker_row_style, lv_color_hex(DIVOOM_ASTRO_STROKE));
	lv_style_set_border_width(&ph->picker_row_style, 1);
	lv_style_set_radius(&ph->picker_row_style, DIVOOM_ASTRO_RADIUS_SM);
	lv_style_set_pad_all(&ph->picker_row_style, 0);

	lv_style_init(&ph->picker_row_focus_style);
	lv_style_set_bg_color(&ph->picker_row_focus_style, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS));
	lv_style_set_bg_opa(&ph->picker_row_focus_style, LV_OPA_COVER);
	lv_style_set_border_color(&ph->picker_row_focus_style, lv_color_hex(DIVOOM_ASTRO_ITEM_BORDER_ACTIVE));
	lv_style_set_border_width(&ph->picker_row_focus_style, DIVOOM_ASTRO_ITEM_BORDER_W_ACTIVE);
	lv_style_set_radius(&ph->picker_row_focus_style, DIVOOM_ASTRO_RADIUS_SM);
	lv_style_set_pad_all(&ph->picker_row_focus_style, 0);
	ph->picker_styles_inited = 1;
}

static void sleep_aid_active_picker_show(Pdivoom_lvgl_sleep_aid_active_ctrl ph)
{
	lv_obj_t *frame;
	lv_obj_t *list;
	const int *mins;
	const char *title_key;
	int is_after;
	int cur;
	int initial;
	int i;
	int n;
	lv_coord_t row_w;
	lv_coord_t trail_w;
	char buf[64];
	divoom_sleep_aid_info *inf;

	if (!ph)
	{
		return;
	}
	divoom_lvgl_top_bright_volume_video_exit();
	sleep_aid_active_shortcut_hide(ph);
	sleep_aid_active_top_hint_hide(ph);
	sleep_aid_active_picker_hide(ph, 0);
	sleep_aid_active_picker_ensure_styles(ph);

	is_after = (divoom_sleep_aid_ctrl_get_play_time_phase() ==
		    DIVOOM_SLEEP_AID_PLAY_TIME_PHASE_AFTER) ? 1 : 0;
	mins = is_after ? s_sa_timer_post : s_sa_timer_pre;
	n = SA_ACTIVE_PICKER_MAX;
	title_key = is_after ? "SleepAidSetAfterTimer" : "SleepAidSetBeforeTimer";
	inf = divoom_sleep_aid_ctrl_get_settings();
	cur = inf ? (is_after ? inf->after_time : inf->before_time) : 30;
	initial = 1;
	for (i = 0; i < n; i++)
	{
		if (mins[i] == cur)
		{
			initial = i;
			break;
		}
	}

	ph->picker_overlay = divoom_astrootoo_ui_alert_modal_create(lv_layer_top(), &frame);
	if (!ph->picker_overlay || !frame)
	{
		return;
	}
	ph->picker_shell = lv_obj_get_child(ph->picker_overlay, 0);
	divoom_astrootoo_ui_alert_modal_add_title(frame,
		divoom_menu_control_get_disp_str(title_key), ph->menu_id);

	list = lv_obj_create(frame);
	ph->picker_list = list;
	lv_obj_remove_style_all(list);
	lv_obj_set_width(list, LV_PCT(100));
	lv_obj_set_height(list, SA_ACTIVE_PICKER_LIST_H);
	lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
	lv_obj_set_style_pad_all(list, 0, 0);
	lv_obj_set_style_pad_row(list, SA_ACTIVE_PICKER_ROW_GAP, 0);
	lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_scroll_dir(list, LV_DIR_VER);
	lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);

	ph->picker_direct = NULL;
	ph->picker_count = n;
	row_w = DIVOOM_ASTRO_CONFIRM_MODAL_SHELL_W - 4 - 32;
	trail_w = 0;
	for (i = 0; i < n; i++)
	{
		lv_obj_t *btn;
		lv_obj_t *lab;
		lv_obj_t *chk;
		lv_coord_t lab_x;

		ph->picker_minutes[i] = mins[i];
		btn = lv_obj_create(list);
		lv_obj_remove_style_all(btn);
		lv_obj_add_style(btn, &ph->picker_row_style, 0);
		lv_obj_add_style(btn, &ph->picker_row_focus_style, LV_STATE_FOCUSED);
		lv_obj_add_style(btn, &ph->picker_row_focus_style, LV_STATE_FOCUS_KEY);
		lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
		lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
		lv_obj_set_style_shadow_width(btn, 0, 0);
		lv_obj_set_width(btn, LV_PCT(100));
		lv_obj_set_height(btn, SA_ACTIVE_PICKER_ROW_H);
		lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);

		chk = lv_img_create(btn);
		if (ph->picker_check_dsc.data)
		{
			lv_img_set_src(chk, &ph->picker_check_dsc);
		}
		else
		{
			divoom_menu_control_lv_img_set_src(chk, SA_ACTIVE_CHECK_ICON);
		}
		lv_obj_set_size(chk, SA_ACTIVE_PICKER_CHECK_SZ, SA_ACTIVE_PICKER_CHECK_SZ);
		lv_obj_align(chk, LV_ALIGN_LEFT_MID, SA_ACTIVE_PICKER_ROW_PAD, 0);
		lv_obj_add_flag(chk, LV_OBJ_FLAG_HIDDEN);

		lab_x = SA_ACTIVE_PICKER_ROW_PAD + SA_ACTIVE_PICKER_CHECK_SZ + SA_ACTIVE_PICKER_CHECK_GAP;
		sleep_aid_active_fmt_timer(buf, (int)sizeof(buf), mins[i]);
		lab = lv_label_create(btn);
		lv_label_set_text(lab, buf);
		lv_obj_set_style_text_color(lab, lv_color_hex(DIVOOM_ASTRO_INK), 0);
		lv_obj_set_style_text_font(lab, divoom_menu_control_get_font_info(28, ph->menu_id), 0);
		lv_obj_align(lab, LV_ALIGN_LEFT_MID, lab_x, 0);
		divoom_astrootoo_ui_list_label_prepare(lab,
			divoom_astrootoo_ui_list_label_width(row_w, lab_x,
							     SA_ACTIVE_PICKER_ROW_PAD, trail_w));
		divoom_astrootoo_ui_list_row_bind_marquee(btn, lab);
		lv_obj_move_foreground(chk);

		ph->picker_row[i] = btn;
		ph->picker_check[i] = chk;
	}

	sleep_aid_active_picker_refresh_checks(ph);
	lv_obj_update_layout(ph->picker_overlay);
	sleep_aid_active_bind_focus(ph);
	sleep_aid_active_picker_set_focus(ph, initial);
	if (ph->picker_shell)
	{
		lv_obj_move_foreground(ph->picker_overlay);
		sleep_aid_active_top_hint_update_osd(ph);
	}
}

static void sleep_aid_active_apply_voice(Pdivoom_lvgl_sleep_aid_active_ctrl ph, int voice_type, int voice_id, int video_type)
{
	Pdivoom_sleep_aid_ctrl c;

	if (!ph || voice_id <= 0)
	{
		return;
	}
	ph->voice_type = voice_type;
	ph->voice_id = voice_id;
	ph->video_type = video_type;
	ph->fav_idx = sleep_aid_active_fav_find_idx(ph, voice_type, voice_id);

	c = divoom_sleep_aid_ctrl_instance();
	if (c)
	{
		c->sel_voice_type = voice_type;
		c->sel_voice_id = voice_id;
	}

	sleep_aid_active_refresh_title(ph);
	divoom_sleep_aid_ctrl_play_voice_with_video(voice_type, voice_id, video_type);
}

static void sleep_aid_active_step_fav(Pdivoom_lvgl_sleep_aid_active_ctrl ph, int step)
{
	int idx;

	if (!ph || ph->fav_count <= 0 || (step != 1 && step != -1))
	{
		return;
	}

	if (ph->fav_idx < 0)
	{
		idx = (step > 0) ? 0 : (ph->fav_count - 1);
	}
	else
	{
		idx = ph->fav_idx + step;
		if (idx < 0)
		{
			idx = ph->fav_count - 1;
		}
		if (idx >= ph->fav_count)
		{
			idx = 0;
		}
	}

	sleep_aid_active_apply_voice(ph, ph->fav_list[idx].audio_type, (int)ph->fav_list[idx].audio_id,
				     ph->fav_list[idx].video_type);
}

static void divoom_lvgl_sleep_aid_active_event_cb(lv_event_t *e)
{
	Pdivoom_lvgl_sleep_aid_active_ctrl ph;
	lv_event_code_t code;
	char *key_info;
	char c;

	ph = (Pdivoom_lvgl_sleep_aid_active_ctrl)lv_event_get_user_data(e);
	code = lv_event_get_code(e);
	if (!ph || code != LV_EVENT_KEY)
	{
		return;
	}

	key_info = (char *)lv_event_get_param(e);
	c = key_info ? (char)key_info[0] : 0;
	if ((c == DIVOOM_LVGL_KEY_ESC || c == DIVOOM_LVGL_KEY_ENTER)
	    && !divoom_astrootoo_ui_key_is_menu_press(e, (uint8_t)c))
	{
		return;
	}

	if (c == DIVOOM_KEY_LIGHT_LEFT || c == DIVOOM_KEY_LIGHT_RIGHT)
	{
		/* The brightness/volume module takes ownership of the video TOP layer. */
		sleep_aid_active_hide_all_overlays(ph);
		if (key_info)
		{
			divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
		}
		return;
	}
	if (sleep_aid_active_picker_visible(ph))
	{
		if (c == DIVOOM_LVGL_KEY_ESC)
		{
			sleep_aid_active_picker_hide(ph, 0);
			return;
		}
		if (c == DIVOOM_LVGL_KEY_ENTER)
		{
			sleep_aid_active_picker_commit(ph, ph->picker_focus);
			return;
		}
		if (divoom_astrootoo_ui_key_is_up((uint8_t)c))
		{
			if (divoom_astrootoo_ui_key_is_nav_press(e))
			{
				sleep_aid_active_picker_set_focus(ph, ph->picker_focus - 1);
			}
			return;
		}
		if (divoom_astrootoo_ui_key_is_down((uint8_t)c))
		{
			if (divoom_astrootoo_ui_key_is_nav_press(e))
			{
				sleep_aid_active_picker_set_focus(ph, ph->picker_focus + 1);
			}
			return;
		}
		if (key_info)
		{
			divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
		}
		return;
	}
	if (c == DIVOOM_LVGL_KEY_ESC)
	{
		if (sleep_aid_active_shortcut_visible(ph))
		{
			sleep_aid_active_shortcut_hide(ph);
			sleep_aid_active_top_hint_disable_osd(ph);
			return;
		}
		divoom_sleep_aid_ctrl_active_esc_back();
		return;
	}
	if (c == DIVOOM_LVGL_KEY_ENTER)
	{
		if (sleep_aid_active_shortcut_visible(ph))
		{
			sleep_aid_active_picker_show(ph);
			return;
		}
		sleep_aid_active_shortcut_show(ph);
		return;
	}
	if (sleep_aid_active_shortcut_visible(ph))
	{
		if (c == DIVOOM_LVGL_KEY_NEXT || c == DIVOOM_LVGL_KEY_PRE ||
			c == DIVOOM_LVGL_KEY_LEFT || c == DIVOOM_LVGL_KEY_RIGHT)
		{
			return;
		}
		if (key_info)
		{
			divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
		}
		return;
	}
	sleep_aid_active_top_hint_pulse_hud(ph);
	if (c == DIVOOM_LVGL_KEY_NEXT)
	{
		sleep_aid_active_step_fav(ph, 1);
		return;
	}
	if (c == DIVOOM_LVGL_KEY_PRE)
	{
		sleep_aid_active_step_fav(ph, -1);
		return;
	}
	if (key_info)
	{
		divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
	}
}

void *divoom_lvgl_sleep_aid_active_init(void *user_data, int menu_id)
{
	Pdivoom_lvgl_sleep_aid_active_ctrl ph;
	Pdivoom_sleep_aid_active_enter ent;

	ph = divoom_system_malloc(sizeof(divoom_lvgl_sleep_aid_active_ctrl));
	if (!ph)
	{
		return NULL;
	}
	memset(ph, 0, sizeof(*ph));

	ph->menu_id = menu_id;
	ph->voice_type = DIVOOM_SLEEP_AID_TYPE_FAV;
	ph->voice_id = 0;
	ph->video_type = -1;
	ph->fav_idx = -1;
	ph->fav_list = NULL;
	ph->fav_count = 0;
	ph->first_play_done = 0;
	ph->enter_param = user_data;
	ph->main_obj = NULL;
	ph->focus_btn = NULL;
	ph->title_lbl = NULL;
	ph->top_hint_cont = NULL;
	ph->top_hint_pill = NULL;
	ph->top_hint_title_lbl = NULL;
	ph->top_hint_count_lbl = NULL;
	ph->top_hint_hud = NULL;
	ph->top_hint_hud_lbl = NULL;
	ph->top_hint_hide_timer = NULL;
	ph->top_hint_hud_hide_timer = NULL;
	ph->top_hint_count_timer = NULL;
	ph->top_hint_osd_buf[0] = NULL;
	ph->top_hint_osd_buf[1] = NULL;
	ph->top_hint_osd_buf_size = 0;
	ph->top_hint_osd_front_index = 0;
	ph->top_hint_osd_enabled = 0;

	ent = (Pdivoom_sleep_aid_active_enter)user_data;
	if (ent && ent->voice_id > 0)
	{
		ph->voice_type = ent->voice_type;
		ph->voice_id = ent->voice_id;
		ph->video_type = ent->video_type;
	}
	else
	{
		divoom_sleep_aid_ctrl_get_selected_voice(&ph->voice_type, &ph->voice_id);
	}

	sleep_aid_active_fav_reload(ph);
	sleep_aid_active_cache_check_png(ph);
	ph->fav_idx = sleep_aid_active_fav_find_idx(ph, ph->voice_type, ph->voice_id);
	if (ph->fav_idx >= 0 && ph->video_type < 0)
	{
		ph->video_type = ph->fav_list[ph->fav_idx].video_type;
	}

	lv_style_init(&ph->root_style);
	lv_style_set_bg_color(&ph->root_style, lv_color_hex(0x000000));
	lv_style_set_bg_opa(&ph->root_style, LV_OPA_COVER);
	lv_style_set_border_width(&ph->root_style, 0);
	lv_style_set_pad_all(&ph->root_style, 0);
	lv_style_set_radius(&ph->root_style, 0);

	ph->main_obj = lv_obj_create(NULL);
	lv_obj_add_style(ph->main_obj, &ph->root_style, 0);
	lv_obj_set_size(ph->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_clear_flag(ph->main_obj, LV_OBJ_FLAG_SCROLLABLE);

	ph->focus_btn = lv_btn_create(ph->main_obj);
	lv_obj_add_style(ph->focus_btn, &ph->root_style, 0);
	lv_obj_set_size(ph->focus_btn, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(ph->focus_btn, 0, 0);
	lv_obj_set_style_bg_opa(ph->focus_btn, LV_OPA_TRANSP, 0);
	lv_obj_clear_flag(ph->focus_btn, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_event_cb(ph->focus_btn, divoom_lvgl_sleep_aid_active_event_cb, LV_EVENT_KEY, ph);
	if (lv_group_get_default())
	{
		lv_group_add_obj(lv_group_get_default(), ph->focus_btn);
	}

	sleep_aid_active_top_hint_create(ph);
	return ph;
}

void divoom_lvgl_sleep_aid_active_disp(void *handle, void *active_obj)
{
	Pdivoom_lvgl_sleep_aid_active_ctrl ph;

	(void)active_obj;
	ph = (Pdivoom_lvgl_sleep_aid_active_ctrl)handle;
	if (!ph)
	{
		return;
	}

	lv_scr_load(ph->main_obj);
	divoom_menu_control_change_disp_mode(ph->main_obj, 1);
	sleep_aid_active_bind_focus(ph);

	if (!ph->first_play_done && ph->voice_id > 0)
	{
		ph->first_play_done = 1;
		divoom_sleep_aid_ctrl_play_voice_with_video(ph->voice_type, (uint32)ph->voice_id, ph->video_type);
	}
	sleep_aid_active_top_hint_show(ph);
}

void divoom_lvgl_sleep_aid_active_pause(void *handle)
{
	Pdivoom_lvgl_sleep_aid_active_ctrl ph;

	ph = (Pdivoom_lvgl_sleep_aid_active_ctrl)handle;
	if (!ph)
	{
		return;
	}
	divoom_lvgl_top_bright_volume_video_exit();
	sleep_aid_active_hide_all_overlays(ph);
	divoom_sleep_aid_ctrl_cancel_play_voice_wait(3000);
	divoom_menu_control_change_disp_mode(ph->main_obj, 0);
	sleep_aid_active_top_hint_release_osd(ph);
}

void divoom_lvgl_sleep_aid_active_exit(void *handle)
{
	Pdivoom_lvgl_sleep_aid_active_ctrl ph;

	ph = (Pdivoom_lvgl_sleep_aid_active_ctrl)handle;
	if (!ph)
	{
		return;
	}

	divoom_lvgl_top_bright_volume_video_exit();
	sleep_aid_active_hide_all_overlays(ph);
	divoom_sleep_aid_ctrl_cancel_play_voice();
	divoom_menu_control_change_disp_mode(ph->main_obj, 0);
	sleep_aid_active_top_hint_release_osd(ph);
	sleep_aid_active_free_check_dsc(ph);

	if (ph->shortcut_cont)
	{
		lv_obj_del(ph->shortcut_cont);
		ph->shortcut_cont = NULL;
		ph->shortcut_img = NULL;
	}
	if (ph->top_hint_cont)
	{
		lv_obj_del(ph->top_hint_cont);
		ph->top_hint_cont = NULL;
		ph->top_hint_pill = NULL;
		ph->top_hint_title_lbl = NULL;
		ph->top_hint_count_lbl = NULL;
		ph->top_hint_hud = NULL;
		ph->top_hint_hud_lbl = NULL;
	}
	if (ph->main_obj)
	{
		lv_obj_del(ph->main_obj);
	}
	lv_style_reset(&ph->root_style);
	lv_style_reset(&ph->top_hint_pill_style);
	if (ph->picker_styles_inited)
	{
		lv_style_reset(&ph->picker_row_style);
		lv_style_reset(&ph->picker_row_focus_style);
		ph->picker_styles_inited = 0;
	}
	sleep_aid_active_fav_free(ph);
	if (ph->enter_param)
	{
		divoom_system_free(ph->enter_param);
	}
	divoom_system_free(ph);
}

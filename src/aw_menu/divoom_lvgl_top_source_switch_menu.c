#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_music_ctrl.h"
#include "divoom_lvgl_top_source_switch_menu.h"
#include "divoom_lvgl_top_control.h"

#ifdef DIVOOM_PRODUCT_ASTRO_TOO
#include "divoom_astrootoo_theme.h"
#include "divoom_lvgl_font_manage.h"

/* 顶栏 toast 图标与 reached_home / OSD 同源（82/）；40×40 设计师 SVG 导出 */
#define DIVOOM_ASTRO_TOP_TOAST_UI_DIR "82/"

/* Astrotoo_UI flowtooSourceSwitchOsd.ts FLOWTOO_SOURCE_SWITCH_OSD_MS */
#define DIVOOM_SOURCE_SWITCH_MENU_TICK_MS       200
#define DIVOOM_SOURCE_SWITCH_MENU_DISMISS_TICKS 11

typedef struct _divoom_lvgl_top_source_switch_menu_ctrl
{
	lv_obj_t *panel_obj;
	lv_obj_t *icon_obj;
	lv_obj_t *text_cont_obj;
	lv_obj_t *label_obj;
	lv_timer_t *dismiss_timer;
	lv_style_t panel_style;
	lv_style_t cont_style;
	uint8 styles_initialized;
	uint8 music_mode;
	uint8 notice_type;
	int exit_ticks;
} divoom_lvgl_top_source_switch_menu_ctrl, *Pdivoom_lvgl_top_source_switch_menu_ctrl;

static divoom_lvgl_top_source_switch_menu_ctrl g_source_switch_menu_ctrl;
static Pdivoom_lvgl_top_source_switch_menu_ctrl pdivoom_lvgl_top_source_switch_menu_handle = NULL;

static void source_switch_menu_reset_styles(Pdivoom_lvgl_top_source_switch_menu_ctrl ph)
{
	if (ph && ph->styles_initialized)
	{
		lv_style_reset(&ph->panel_style);
		lv_style_reset(&ph->cont_style);
		ph->styles_initialized = 0;
	}
}

static lv_coord_t source_switch_menu_text_width(void)
{
	lv_coord_t text_w;

	text_w = DIVOOM_ASTRO_TOP_TOAST_W
		- DIVOOM_ASTRO_TOP_TOAST_PAD_X * 2
		- DIVOOM_ASTRO_TOP_TOAST_ICON * 2
		- DIVOOM_ASTRO_TOP_TOAST_GAP * 2;
	if (text_w < 120)
	{
		text_w = 120;
	}
	return text_w;
}

static void source_switch_menu_get_assets(uint8 music_mode, const char **img_path, const char **source_key)
{
	switch (music_mode)
	{
	case DIVOOM_MUSIC_CTRL_MODE_NET_FM:
	{
		/* 复用音源目录中已登记的AstroTooUI电台图，不增加近似绘制资源。 */
		*img_path = "91/source_net_radio.png";
		*source_key = "SourceNetRadio";
		break;
	}
	case DIVOOM_MUSIC_CTRL_MODE_BT:
		*img_path = DIVOOM_ASTRO_TOP_TOAST_UI_DIR "at_toast_source_bluetooth.png";
		*source_key = "Bluetooth";
		break;
	case DIVOOM_MUSIC_CTRL_MODE_LOCAL:
		*img_path = DIVOOM_ASTRO_TOP_TOAST_UI_DIR "at_toast_source_tf.png";
		*source_key = "TFCard";
		break;
#if DIVOOM_MUSIC_CTRL_LINEIN_ENABLE
	case DIVOOM_MUSIC_CTRL_MODE_LINEIN:
		*img_path = DIVOOM_ASTRO_TOP_TOAST_UI_DIR "at_toast_source_aux.png";
		*source_key = "SourceAUX";
		break;
#endif
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
	case DIVOOM_MUSIC_CTRL_MODE_UAC:
		*img_path = DIVOOM_ASTRO_TOP_TOAST_UI_DIR "at_toast_source_uac.png";
		*source_key = "SourceUAC";
		break;
#endif
	default:
		*img_path = DIVOOM_ASTRO_TOP_TOAST_UI_DIR "at_toast_source_bluetooth.png";
		*source_key = "Bluetooth";
		break;
	}
}

static void source_switch_menu_format_label(char *buf, int buf_sz, const char *source_key,
					   uint8 notice_type)
{
	const char *tpl;
	const char *source_name;

	if (!buf || buf_sz <= 0)
	{
		return;
	}
	buf[0] = '\0';
	source_name = divoom_menu_control_get_disp_str(source_key);
	if (!source_name)
	{
		source_name = "";
	}
	/* 无其它音源：只显示源名（蓝牙），不要「切换为」 */
	if (notice_type == DIVOOM_SOURCE_SWITCH_OSD_CURRENT)
	{
		snprintf(buf, (size_t)buf_sz, "%s", source_name);
		return;
	}
	tpl = divoom_menu_control_get_disp_str("SourceSwitchOsdTpl");
	if (tpl && strchr(tpl, '%'))
	{
		snprintf(buf, (size_t)buf_sz, tpl, source_name);
	}
	else
	{
		snprintf(buf, (size_t)buf_sz, "%s%s", "Source: ", source_name);
	}
}

static const char *source_switch_menu_pls_insert_str_key(uint8 music_mode)
{
	switch (music_mode)
	{
	case DIVOOM_MUSIC_CTRL_MODE_LOCAL:
		return "TFPlsInsert";
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
	case DIVOOM_MUSIC_CTRL_MODE_UAC:
		return "UACPlsInsert";
#endif
#if DIVOOM_MUSIC_CTRL_LINEIN_ENABLE
	case DIVOOM_MUSIC_CTRL_MODE_LINEIN:
		return "AUXPlsInsert";
#endif
	default:
		return NULL;
	}
}

static void source_switch_menu_update_content(Pdivoom_lvgl_top_source_switch_menu_ctrl ph)
{
	const char *img_path;
	const char *source_key;
	char label_buf[128];
	lv_coord_t text_w;
	const char *label_text;

	source_switch_menu_get_assets(ph->music_mode, &img_path, &source_key);
	if (ph->notice_type == DIVOOM_SOURCE_SWITCH_OSD_PLS_INSERT)
	{
		const char *str_key;

		str_key = source_switch_menu_pls_insert_str_key(ph->music_mode);
		label_text = str_key ? divoom_menu_control_get_disp_str(str_key) : "";
		if (!label_text)
		{
			label_text = "";
		}
	}
	else
	{
		source_switch_menu_format_label(label_buf, (int)sizeof(label_buf), source_key,
						ph->notice_type);
		label_text = label_buf;
	}

	if (ph->icon_obj)
	{
		divoom_menu_control_lv_img_set_src(ph->icon_obj, img_path);
	}
	if (ph->label_obj)
	{
		text_w = source_switch_menu_text_width();
		lv_obj_set_width(ph->text_cont_obj, text_w);
		lv_obj_set_width(ph->label_obj, text_w);
		lv_label_set_text(ph->label_obj, label_text);
	}
}

static void source_switch_menu_release_fonts(Pdivoom_lvgl_top_source_switch_menu_ctrl ph)
{
	/* close 走 lv_obj_del_async，回调时控件可能还在；先摘 FT 再清账本 */
	if (ph && ph->label_obj)
		lv_obj_set_style_text_font(ph->label_obj, LV_FONT_DEFAULT, 0);
	divoom_lvgl_font_manage_clear_clock_font(DIVOOM_MENU_CONTROL_ID_TOP_SOURCE_SWITCH, NULL);
}

static void source_switch_menu_del(Pdivoom_lvgl_top_source_switch_menu_ctrl ph)
{
	if (!ph)
	{
		return;
	}
	if (ph->dismiss_timer)
	{
		lv_timer_del(ph->dismiss_timer);
		ph->dismiss_timer = NULL;
	}
	source_switch_menu_release_fonts(ph);
	if (ph->panel_obj)
	{
		lv_obj_del_async(ph->panel_obj);
		ph->panel_obj = NULL;
	}
	ph->icon_obj = NULL;
	ph->text_cont_obj = NULL;
	ph->label_obj = NULL;
	ph->notice_type = DIVOOM_SOURCE_SWITCH_OSD_SWITCH;
	source_switch_menu_reset_styles(ph);
	if (pdivoom_lvgl_top_source_switch_menu_handle == ph)
	{
		pdivoom_lvgl_top_source_switch_menu_handle = NULL;
	}
}

static void source_switch_menu_timer_cb(lv_timer_t *timer)
{
	(void)timer;

	if (!pdivoom_lvgl_top_source_switch_menu_handle)
	{
		return;
	}
	pdivoom_lvgl_top_source_switch_menu_handle->exit_ticks++;
	if (pdivoom_lvgl_top_source_switch_menu_handle->exit_ticks >= DIVOOM_SOURCE_SWITCH_MENU_DISMISS_TICKS)
	{
		source_switch_menu_del(pdivoom_lvgl_top_source_switch_menu_handle);
	}
}

static void source_switch_menu_create_ui(Pdivoom_lvgl_top_source_switch_menu_ctrl ph)
{
	lv_obj_t *spacer;
	lv_coord_t text_w;

	ph->exit_ticks = 0;
	ph->styles_initialized = 0;

	lv_style_init(&ph->panel_style);
	lv_style_set_bg_color(&ph->panel_style, lv_color_hex(DIVOOM_ASTRO_BG));
	lv_style_set_bg_opa(&ph->panel_style, LV_OPA_COVER);
	lv_style_set_border_width(&ph->panel_style, 2);
	lv_style_set_border_color(&ph->panel_style, lv_color_hex(DIVOOM_ASTRO_STROKE));
	lv_style_set_radius(&ph->panel_style, DIVOOM_ASTRO_RADIUS_SM);
	lv_style_set_pad_top(&ph->panel_style, DIVOOM_ASTRO_TOP_TOAST_PAD_Y);
	lv_style_set_pad_bottom(&ph->panel_style, DIVOOM_ASTRO_TOP_TOAST_PAD_Y);
	lv_style_set_pad_left(&ph->panel_style, DIVOOM_ASTRO_TOP_TOAST_PAD_X);
	lv_style_set_pad_right(&ph->panel_style, DIVOOM_ASTRO_TOP_TOAST_PAD_X);
	lv_style_set_pad_column(&ph->panel_style, DIVOOM_ASTRO_TOP_TOAST_GAP);
	lv_style_set_shadow_width(&ph->panel_style, 0);
	lv_style_set_outline_width(&ph->panel_style, 0);

	lv_style_init(&ph->cont_style);
	lv_style_set_bg_opa(&ph->cont_style, LV_OPA_TRANSP);
	lv_style_set_border_width(&ph->cont_style, 0);
	lv_style_set_pad_all(&ph->cont_style, 0);
	lv_style_set_pad_gap(&ph->cont_style, 0);
	lv_style_set_radius(&ph->cont_style, 0);
	ph->styles_initialized = 1;

	ph->panel_obj = lv_obj_create(lv_layer_top());
	lv_obj_add_style(ph->panel_obj, &ph->panel_style, 0);
	lv_obj_set_pos(ph->panel_obj, DIVOOM_ASTRO_TOP_TOAST_X, DIVOOM_ASTRO_TOP_TOAST_Y);
	lv_obj_set_size(ph->panel_obj, DIVOOM_ASTRO_TOP_TOAST_W, DIVOOM_ASTRO_TOP_TOAST_H);
	lv_obj_clear_flag(ph->panel_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_clear_flag(ph->panel_obj, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_flex_flow(ph->panel_obj, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(ph->panel_obj, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
			      LV_FLEX_ALIGN_CENTER);

	ph->icon_obj = lv_img_create(ph->panel_obj);
	lv_obj_set_size(ph->icon_obj, DIVOOM_ASTRO_TOP_TOAST_ICON, DIVOOM_ASTRO_TOP_TOAST_ICON);

	text_w = source_switch_menu_text_width();

	ph->text_cont_obj = lv_obj_create(ph->panel_obj);
	lv_obj_add_style(ph->text_cont_obj, &ph->cont_style, 0);
	lv_obj_set_size(ph->text_cont_obj, text_w, LV_SIZE_CONTENT);
	lv_obj_clear_flag(ph->text_cont_obj, LV_OBJ_FLAG_SCROLLABLE);

	ph->label_obj = lv_label_create(ph->text_cont_obj);
	lv_obj_set_width(ph->label_obj, text_w);
	lv_label_set_long_mode(ph->label_obj, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ph->label_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(ph->label_obj,
				   divoom_menu_control_get_font_info(24, DIVOOM_MENU_CONTROL_ID_TOP_SOURCE_SWITCH),
				   0);
	lv_obj_set_style_text_color(ph->label_obj, lv_color_hex(DIVOOM_ASTRO_INK), 0);
	lv_obj_center(ph->label_obj);

	spacer = lv_obj_create(ph->panel_obj);
	lv_obj_add_style(spacer, &ph->cont_style, 0);
	lv_obj_set_size(spacer, DIVOOM_ASTRO_TOP_TOAST_ICON, DIVOOM_ASTRO_TOP_TOAST_ICON);
	lv_obj_clear_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);

	source_switch_menu_update_content(ph);

	ph->dismiss_timer = lv_timer_create(source_switch_menu_timer_cb, DIVOOM_SOURCE_SWITCH_MENU_TICK_MS,
					    NULL);
	divoom_lvgl_layer_top_raise(ph->panel_obj);
}

static void source_switch_menu_show(uint8 music_mode, uint8 notice_type)
{
	divoom_get_lvgl_lock();

	if (pdivoom_lvgl_top_source_switch_menu_handle != NULL)
	{
		pdivoom_lvgl_top_source_switch_menu_handle->exit_ticks = 0;
		pdivoom_lvgl_top_source_switch_menu_handle->music_mode = music_mode;
		pdivoom_lvgl_top_source_switch_menu_handle->notice_type = notice_type;
		source_switch_menu_update_content(pdivoom_lvgl_top_source_switch_menu_handle);
		divoom_lvgl_layer_top_raise(pdivoom_lvgl_top_source_switch_menu_handle->panel_obj);
		divoom_free_lvgl_lock();
		return;
	}

	memset(&g_source_switch_menu_ctrl, 0, sizeof(g_source_switch_menu_ctrl));
	pdivoom_lvgl_top_source_switch_menu_handle = &g_source_switch_menu_ctrl;
	pdivoom_lvgl_top_source_switch_menu_handle->music_mode = music_mode;
	pdivoom_lvgl_top_source_switch_menu_handle->notice_type = notice_type;
	source_switch_menu_create_ui(pdivoom_lvgl_top_source_switch_menu_handle);

	divoom_free_lvgl_lock();
}

void divoom_lvgl_top_source_switch_menu_init(uint8 music_mode, uint8 notice_type)
{
	source_switch_menu_show(music_mode, notice_type);
}

void divoom_lvgl_top_source_switch_menu_exit(void)
{
	divoom_get_lvgl_lock();
	source_switch_menu_del(pdivoom_lvgl_top_source_switch_menu_handle);
	divoom_free_lvgl_lock();
}

#else /* !DIVOOM_PRODUCT_ASTRO_TOO */

void divoom_lvgl_top_source_switch_menu_init(uint8 music_mode, uint8 notice_type)
{
	(void)music_mode;
	(void)notice_type;
}

void divoom_lvgl_top_source_switch_menu_exit(void)
{
}

#endif /* DIVOOM_PRODUCT_ASTRO_TOO */

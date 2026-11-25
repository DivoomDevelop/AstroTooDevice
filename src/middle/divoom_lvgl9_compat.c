/*
 * LVGL9 compatibility shims declared in divoom_lvgl9_compat.h.
 * Intentionally dual-build: sim uses DIVOOM_LVGL9_IS_SIM branches; RTOS uses !SIM block.
 */
#include "divoom_lvgl9_compat.h"
#include "lvgl.h"
#include <string.h>
#if !DIVOOM_LVGL9_IS_SIM
#include "src/misc/cache/instance/lv_image_cache.h"
#if LV_USE_SNAPSHOT
#include "src/others/snapshot/lv_snapshot.h"
#endif
#endif

static int s_screen_transp;

void divoom_lvgl9_set_screen_transp(int on)
{
	s_screen_transp = on ? 1 : 0;
}

int divoom_lvgl9_get_screen_transp(void)
{
	return s_screen_transp;
}

void lv_disp_set_color_format(lv_disp_t *disp, lv_color_format_t color_format)
{
	if (disp != NULL)
		lv_display_set_color_format(disp, color_format);
}

void lv_disp_set_bg_opa(lv_disp_t *disp, lv_opa_t opa)
{
	lv_obj_t *scr;

	(void)disp;
	scr = lv_screen_active();
	if (scr != NULL)
		lv_obj_set_style_bg_opa(scr, opa, 0);
}

void lv_disp_set_bg_color(lv_disp_t *disp, lv_color_t color)
{
	lv_obj_t *scr;

	(void)disp;
	scr = lv_screen_active();
	if (scr != NULL)
		lv_obj_set_style_bg_color(scr, color, 0);
}

lv_coord_t divoom_lv_txt_get_width(const char *txt, uint32_t length,
				   const lv_font_t *font, lv_coord_t letter_space,
				   lv_text_flag_t flags)
{
	(void)flags;
	return (lv_coord_t)lv_text_get_width(txt, length, font, letter_space);
}

void lv_canvas_draw_text(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y,
			 lv_coord_t max_w, lv_draw_label_dsc_t *draw_dsc,
			 const char *txt)
{
	lv_layer_t layer;
	lv_area_t coords;
	lv_draw_label_dsc_t dsc;
	lv_coord_t canvas_h;

	if (canvas == NULL || draw_dsc == NULL || txt == NULL || max_w <= 0)
		return;

	canvas_h = lv_obj_get_height(canvas);
	if (canvas_h <= y)
		return;

	dsc = *draw_dsc;
	dsc.text = txt;
	coords.x1 = x;
	coords.y1 = y;
	coords.x2 = x + max_w - 1;
	coords.y2 = canvas_h - 1;

	lv_canvas_init_layer(canvas, &layer);
	lv_draw_label(&layer, &dsc, &coords);
	lv_canvas_finish_layer(canvas, &layer);
	lv_obj_invalidate(canvas);
}

lv_opa_t lv_img_buf_get_px_alpha(const lv_img_dsc_t *dsc, lv_coord_t x,
				 lv_coord_t y)
{
	(void)dsc;
	(void)x;
	(void)y;
	return LV_OPA_COVER;
}

void lv_img_buf_set_px_alpha(const lv_img_dsc_t *dsc, lv_coord_t x, lv_coord_t y,
			     lv_opa_t opa)
{
	(void)dsc;
	(void)x;
	(void)y;
	(void)opa;
}

#if !DIVOOM_LVGL9_IS_SIM
static const uint8_t *divoom_lvgl9_decoded_pixels(const lv_image_decoder_dsc_t *dsc)
{
	if (dsc == NULL || dsc->decoded == NULL)
		return NULL;
	return dsc->decoded->data;
}

lv_result_t lv_img_decoder_open(lv_img_decoder_dsc_t *dsc, const char *src,
				lv_color_t color, int32_t frame_id)
{
	lv_image_decoder_args_t args;

	if (dsc == NULL || src == NULL)
		return LV_RESULT_INVALID;
	(void)color;
	(void)frame_id;
	memset(&args, 0, sizeof(args));
	if (lv_image_decoder_open(&dsc->dsc, src, &args) != LV_RESULT_OK)
		return LV_RESULT_INVALID;
	dsc->header = dsc->dsc.header;
	dsc->img_data = divoom_lvgl9_decoded_pixels(&dsc->dsc);
	return LV_RESULT_OK;
}

lv_result_t lv_img_decoder_get_info(const char *src, lv_image_header_t *header)
{
	return lv_image_decoder_get_info(src, header);
}

void lv_img_decoder_close(lv_img_decoder_dsc_t *dsc)
{
	if (dsc != NULL)
		lv_image_decoder_close(&dsc->dsc);
}

void lv_img_cache_set_size(uint32_t size)
{
	lv_image_cache_resize(size, true);
}

#undef lv_obj_add_event
void lv_obj_add_event(lv_obj_t *obj, lv_event_cb_t cb, lv_event_code_t code, void *user_data)
{
	lv_obj_add_event_cb(obj, cb, code, user_data);
}

#if LV_USE_SNAPSHOT
lv_img_dsc_t *divoom_lvgl9_snapshot_take(lv_obj_t *obj, lv_color_format_t cf)
{
	lv_draw_buf_t *buf;

#undef lv_snapshot_take
	buf = lv_snapshot_take(obj, cf);
#define lv_snapshot_take(obj, cf)  divoom_lvgl9_snapshot_take((obj), (lv_color_format_t)(cf))
	/*
	 * LVGL9 的快照就是可直接作为图像源的 lv_draw_buf_t。不能只复制其
	 * lv_img_dsc_t 前缀，否则会丢失 unaligned_data/handlers 并触发解码断言。
	 */
	if (buf != NULL && (buf->unaligned_data == NULL || buf->handlers == NULL))
	{
		lv_draw_buf_destroy(buf);
		buf = NULL;
	}
	return (lv_img_dsc_t *)buf;
}

void divoom_lvgl9_snapshot_free(lv_img_dsc_t *dsc)
{
	if (dsc != NULL)
	{
		lv_draw_buf_destroy((lv_draw_buf_t *)dsc);
	}
}
#else
lv_img_dsc_t *divoom_lvgl9_snapshot_take(lv_obj_t *obj, lv_color_format_t cf)
{
	(void)obj;
	(void)cf;
	return NULL;
}
void divoom_lvgl9_snapshot_free(lv_img_dsc_t *dsc)
{
	if (dsc != NULL)
		lv_free((void *)dsc);
}
#endif
#endif

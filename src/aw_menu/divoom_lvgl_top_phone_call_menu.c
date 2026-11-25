#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_font_manage.h"
#include "divoom_lvgl_top_phone_call_menu.h"
#include "divoom_lvgl_top_control.h"
#include "divoom_btcore_uart_cmd.h"
#include <string.h>

#ifdef DIVOOM_PRODUCT_ASTRO_TOO
#include "divoom_astrootoo_theme.h"

#define DIVOOM_ASTRO_PHONE_TOAST_ICON_PATH  "82/at_toast_phone.png"

#define PHONE_CALL_OP_NONE    0u
#define PHONE_CALL_OP_SHOW    1u
#define PHONE_CALL_OP_DISMISS 2u

typedef struct _divoom_lvgl_top_phone_call_menu_ctrl {
	void *main_obj;
	void *top_disp_obj;
	void *img_obj;
	void *label_obj;
	uint8_t call_type;
	char number[DIVOOM_BTCORE_PHONE_ALERT_NUMBER_MAX + 1];
	uint8_t number_len;
	char name[DIVOOM_BTCORE_PHONE_ALERT_NAME_MAX + 1];
	uint8_t name_len;
} divoom_lvgl_top_phone_call_menu_ctrl, *Pdivoom_lvgl_top_phone_call_menu_ctrl;

static Pdivoom_lvgl_top_phone_call_menu_ctrl pdivoom_lvgl_top_phone_call_menu_handle = NULL;
static volatile uint8_t s_phone_call_pending_op = PHONE_CALL_OP_NONE;
static volatile uint8_t s_phone_call_pending_call_type;
static char s_phone_call_pending_number[DIVOOM_BTCORE_PHONE_ALERT_NUMBER_MAX + 1];
static volatile uint8_t s_phone_call_pending_number_len;
static char s_phone_call_pending_name[DIVOOM_BTCORE_PHONE_ALERT_NAME_MAX + 1];
static volatile uint8_t s_phone_call_pending_name_len;

static void divoom_lvgl_top_phone_call_menu_apply_panel_style(lv_obj_t *obj)
{
	lv_obj_set_style_bg_color(obj, lv_color_hex(DIVOOM_ASTRO_BG), 0);
	lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(obj, 2, 0);
	lv_obj_set_style_border_color(obj, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
	lv_obj_set_style_radius(obj, DIVOOM_ASTRO_RADIUS_MD, 0);
	lv_obj_set_style_pad_top(obj, DIVOOM_ASTRO_TOP_TOAST_PAD_Y, 0);
	lv_obj_set_style_pad_bottom(obj, DIVOOM_ASTRO_TOP_TOAST_PAD_Y, 0);
	lv_obj_set_style_pad_left(obj, DIVOOM_ASTRO_TOP_TOAST_PAD_X, 0);
	lv_obj_set_style_pad_right(obj, DIVOOM_ASTRO_TOP_TOAST_PAD_X, 0);
	lv_obj_set_style_pad_column(obj, DIVOOM_ASTRO_TOP_TOAST_GAP, 0);
	lv_obj_set_style_shadow_width(obj, 0, 0);
	lv_obj_set_style_outline_width(obj, 0, 0);
}

static void divoom_lvgl_top_phone_call_menu_copy_field(char *dst, size_t dst_sz,
						       const char *src, uint8_t src_len)
{
	size_t n;

	if (dst == NULL || dst_sz == 0u)
		return;
	if (src == NULL || src_len == 0u) {
		dst[0] = '\0';
		return;
	}
	n = src_len;
	if (n >= dst_sz)
		n = dst_sz - 1u;
	memcpy(dst, src, n);
	dst[n] = '\0';
}

static const char *divoom_lvgl_top_phone_call_menu_tip_key(uint8_t call_type)
{
	if (call_type == DIVOOM_BTCORE_PHONE_ALERT_CALL_ACTIVE)
		return "PhoneCallActiveTip";
	if (call_type == DIVOOM_BTCORE_PHONE_ALERT_CALL_OUTGOING)
		return "OutgoingCallTip";
	return "IncomingCallTip";
}

static void divoom_lvgl_top_phone_call_menu_format_label(Pdivoom_lvgl_top_phone_call_menu_ctrl handle,
							 char *dst, size_t dst_sz)
{
	const char *tip;
	size_t n;

	if (handle == NULL || dst == NULL || dst_sz == 0u)
		return;

	tip = divoom_menu_control_get_disp_str(divoom_lvgl_top_phone_call_menu_tip_key(handle->call_type));
	if (handle->name_len > 0u) {
		n = handle->name_len;
		if (n >= dst_sz)
			n = dst_sz - 1u;
		memcpy(dst, handle->name, n);
		dst[n] = '\0';
		return;
	}
	if (handle->number_len > 0u) {
		n = handle->number_len;
		if (n >= dst_sz)
			n = dst_sz - 1u;
		memcpy(dst, handle->number, n);
		dst[n] = '\0';
		return;
	}
	strncpy(dst, tip, dst_sz - 1u);
	dst[dst_sz - 1u] = '\0';
}

static void divoom_lvgl_top_phone_call_menu_update_disp(Pdivoom_lvgl_top_phone_call_menu_ctrl handle)
{
	char label_buf[DIVOOM_BTCORE_PHONE_ALERT_NAME_MAX + 1];

	if (handle == NULL || handle->label_obj == NULL)
		return;

	divoom_lvgl_top_phone_call_menu_format_label(handle, label_buf, sizeof(label_buf));
	lv_label_set_text(handle->label_obj, label_buf);
}

static void divoom_lvgl_top_phone_call_menu_del_menu(void)
{
	if (pdivoom_lvgl_top_phone_call_menu_handle == NULL)
		return;

	divoom_lvgl_top_control_remove_top(pdivoom_lvgl_top_phone_call_menu_handle);
	lv_obj_del_async(pdivoom_lvgl_top_phone_call_menu_handle->main_obj);
	divoom_system_free(pdivoom_lvgl_top_phone_call_menu_handle);
	divoom_local_movie_play_ctrl(1);
	pdivoom_lvgl_top_phone_call_menu_handle = NULL;
}

static void divoom_lvgl_top_phone_call_menu_show(uint8_t call_type, const char *number,
						 uint8_t number_len, const char *name,
						 uint8_t name_len)
{
	lv_coord_t text_w;
	char label_buf[DIVOOM_BTCORE_PHONE_ALERT_NAME_MAX + 1];
	lv_obj_t *img_obj;
	lv_obj_t *label;
	lv_obj_t *spacer;

	divoom_get_lvgl_lock();
	if (divoom_menu_control_get_active_menu_id() == DIVOOM_MENU_CONTROL_ID_SYSTEM_POWER_ON) {
		divoom_free_lvgl_lock();
		return;
	}

	if (pdivoom_lvgl_top_phone_call_menu_handle != NULL) {
		pdivoom_lvgl_top_phone_call_menu_handle->call_type = call_type;
		divoom_lvgl_top_phone_call_menu_copy_field(
			pdivoom_lvgl_top_phone_call_menu_handle->number,
			sizeof(pdivoom_lvgl_top_phone_call_menu_handle->number),
			number, number_len);
		pdivoom_lvgl_top_phone_call_menu_handle->number_len = number_len;
		divoom_lvgl_top_phone_call_menu_copy_field(
			pdivoom_lvgl_top_phone_call_menu_handle->name,
			sizeof(pdivoom_lvgl_top_phone_call_menu_handle->name),
			name, name_len);
		pdivoom_lvgl_top_phone_call_menu_handle->name_len = name_len;
		divoom_lvgl_top_phone_call_menu_update_disp(pdivoom_lvgl_top_phone_call_menu_handle);
		divoom_lvgl_layer_top_raise(pdivoom_lvgl_top_phone_call_menu_handle->main_obj);
		divoom_free_lvgl_lock();
		return;
	}

	pdivoom_lvgl_top_phone_call_menu_handle =
		(Pdivoom_lvgl_top_phone_call_menu_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_top_phone_call_menu_ctrl));
	if (pdivoom_lvgl_top_phone_call_menu_handle == NULL) {
		divoom_free_lvgl_lock();
		return;
	}

	divoom_local_movie_play_ctrl(0);
	memset(pdivoom_lvgl_top_phone_call_menu_handle, 0, sizeof(divoom_lvgl_top_phone_call_menu_ctrl));
	pdivoom_lvgl_top_phone_call_menu_handle->call_type = call_type;
	divoom_lvgl_top_phone_call_menu_copy_field(
		pdivoom_lvgl_top_phone_call_menu_handle->number,
		sizeof(pdivoom_lvgl_top_phone_call_menu_handle->number),
		number, number_len);
	pdivoom_lvgl_top_phone_call_menu_handle->number_len = number_len;
	divoom_lvgl_top_phone_call_menu_copy_field(
		pdivoom_lvgl_top_phone_call_menu_handle->name,
		sizeof(pdivoom_lvgl_top_phone_call_menu_handle->name),
		name, name_len);
	pdivoom_lvgl_top_phone_call_menu_handle->name_len = name_len;

	pdivoom_lvgl_top_phone_call_menu_handle->main_obj = lv_obj_create(lv_layer_top());
	lv_obj_set_style_bg_opa(pdivoom_lvgl_top_phone_call_menu_handle->main_obj, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(pdivoom_lvgl_top_phone_call_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_border_width(pdivoom_lvgl_top_phone_call_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_shadow_width(pdivoom_lvgl_top_phone_call_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_outline_width(pdivoom_lvgl_top_phone_call_menu_handle->main_obj, 0, 0);
	lv_obj_clear_flag(pdivoom_lvgl_top_phone_call_menu_handle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(pdivoom_lvgl_top_phone_call_menu_handle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(pdivoom_lvgl_top_phone_call_menu_handle->main_obj, 0, 0);

	pdivoom_lvgl_top_phone_call_menu_handle->top_disp_obj =
		lv_obj_create(pdivoom_lvgl_top_phone_call_menu_handle->main_obj);
	divoom_lvgl_top_phone_call_menu_apply_panel_style(pdivoom_lvgl_top_phone_call_menu_handle->top_disp_obj);
	lv_obj_clear_flag(pdivoom_lvgl_top_phone_call_menu_handle->top_disp_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_clear_flag(pdivoom_lvgl_top_phone_call_menu_handle->top_disp_obj, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_pos(pdivoom_lvgl_top_phone_call_menu_handle->top_disp_obj,
		       DIVOOM_ASTRO_TOP_TOAST_X, DIVOOM_ASTRO_TOP_TOAST_Y);
	lv_obj_set_size(pdivoom_lvgl_top_phone_call_menu_handle->top_disp_obj,
			DIVOOM_ASTRO_TOP_TOAST_W, DIVOOM_ASTRO_TOP_TOAST_H);
	lv_obj_set_flex_flow(pdivoom_lvgl_top_phone_call_menu_handle->top_disp_obj, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(pdivoom_lvgl_top_phone_call_menu_handle->top_disp_obj,
			      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	img_obj = lv_img_create(pdivoom_lvgl_top_phone_call_menu_handle->top_disp_obj);
	divoom_menu_control_lv_img_set_src(img_obj, DIVOOM_ASTRO_PHONE_TOAST_ICON_PATH);
	lv_obj_set_size(img_obj, DIVOOM_ASTRO_TOP_TOAST_ICON, DIVOOM_ASTRO_TOP_TOAST_ICON);
	pdivoom_lvgl_top_phone_call_menu_handle->img_obj = img_obj;

	text_w = DIVOOM_ASTRO_TOP_TOAST_W - DIVOOM_ASTRO_TOP_TOAST_PAD_X * 2
		 - DIVOOM_ASTRO_TOP_TOAST_ICON * 2 - DIVOOM_ASTRO_TOP_TOAST_GAP * 2;
	if (text_w < 120)
		text_w = 120;

	label = lv_label_create(pdivoom_lvgl_top_phone_call_menu_handle->top_disp_obj);
	lv_obj_set_width(label, text_w);
	lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(label,
		divoom_menu_control_get_font_info(22, DIVOOM_MENU_CONTROL_ID_TOP_AUDIO_TIP_MENU), 0);
	lv_obj_set_style_text_color(label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
	divoom_lvgl_top_phone_call_menu_format_label(pdivoom_lvgl_top_phone_call_menu_handle,
						     label_buf, sizeof(label_buf));
	lv_label_set_text(label, label_buf);
	pdivoom_lvgl_top_phone_call_menu_handle->label_obj = label;

	spacer = lv_obj_create(pdivoom_lvgl_top_phone_call_menu_handle->top_disp_obj);
	lv_obj_set_size(spacer, DIVOOM_ASTRO_TOP_TOAST_ICON, DIVOOM_ASTRO_TOP_TOAST_ICON);
	lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(spacer, 0, 0);
	lv_obj_set_style_pad_all(spacer, 0, 0);
	lv_obj_clear_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);

	divoom_lvgl_top_control_add_top(pdivoom_lvgl_top_phone_call_menu_handle,
					pdivoom_lvgl_top_phone_call_menu_handle->top_disp_obj);
	divoom_lvgl_layer_top_raise(pdivoom_lvgl_top_phone_call_menu_handle->main_obj);
	divoom_free_lvgl_lock();
}

void divoom_lvgl_top_phone_call_menu_notify(uint8_t call_type, const char *number,
					    uint8_t number_len, const char *name,
					    uint8_t name_len)
{
	if (call_type != DIVOOM_BTCORE_PHONE_ALERT_CALL_INCOMING
	    && call_type != DIVOOM_BTCORE_PHONE_ALERT_CALL_OUTGOING
	    && call_type != DIVOOM_BTCORE_PHONE_ALERT_CALL_ACTIVE)
		return;

	divoom_lvgl_top_phone_call_menu_copy_field(s_phone_call_pending_number,
						   sizeof(s_phone_call_pending_number),
						   number, number_len);
	s_phone_call_pending_number_len = number_len;
	divoom_lvgl_top_phone_call_menu_copy_field(s_phone_call_pending_name,
						   sizeof(s_phone_call_pending_name),
						   name, name_len);
	s_phone_call_pending_name_len = name_len;
	s_phone_call_pending_call_type = call_type;
	s_phone_call_pending_op = PHONE_CALL_OP_SHOW;
}

void divoom_lvgl_top_phone_call_menu_dismiss(void)
{
	s_phone_call_pending_op = PHONE_CALL_OP_DISMISS;
}

void divoom_lvgl_top_phone_call_menu_poll(void)
{
	uint8_t op;
	uint8_t call_type;
	uint8_t number_len;
	uint8_t name_len;
	char number[DIVOOM_BTCORE_PHONE_ALERT_NUMBER_MAX + 1];
	char name[DIVOOM_BTCORE_PHONE_ALERT_NAME_MAX + 1];

	op = s_phone_call_pending_op;
	if (op == PHONE_CALL_OP_NONE)
		return;
	if (op == PHONE_CALL_OP_DISMISS) {
		s_phone_call_pending_op = PHONE_CALL_OP_NONE;
		divoom_lvgl_top_phone_call_menu_del_menu();
		return;
	}
	if (!divoom_lvgl_font_manage_is_inited())
		return;

	s_phone_call_pending_op = PHONE_CALL_OP_NONE;
	call_type = s_phone_call_pending_call_type;
	number_len = s_phone_call_pending_number_len;
	name_len = s_phone_call_pending_name_len;
	memcpy(number, s_phone_call_pending_number, sizeof(number));
	memcpy(name, s_phone_call_pending_name, sizeof(name));
	divoom_lvgl_top_phone_call_menu_show(call_type, number, number_len, name, name_len);
}

void divoom_lvgl_top_phone_call_menu_exit(void)
{
	s_phone_call_pending_op = PHONE_CALL_OP_NONE;
	divoom_lvgl_top_phone_call_menu_del_menu();
}

int divoom_lvgl_top_phone_call_menu_active(void)
{
	if (pdivoom_lvgl_top_phone_call_menu_handle != NULL)
		return 1;
	if (s_phone_call_pending_op == PHONE_CALL_OP_SHOW)
		return 1;
	return 0;
}

#else /* !DIVOOM_PRODUCT_ASTRO_TOO */

void divoom_lvgl_top_phone_call_menu_notify(uint8_t call_type, const char *number,
					    uint8_t number_len, const char *name,
					    uint8_t name_len)
{
	(void)call_type;
	(void)number;
	(void)number_len;
	(void)name;
	(void)name_len;
}

void divoom_lvgl_top_phone_call_menu_dismiss(void)
{
}

void divoom_lvgl_top_phone_call_menu_poll(void)
{
}

void divoom_lvgl_top_phone_call_menu_exit(void)
{
}

int divoom_lvgl_top_phone_call_menu_active(void)
{
	return 0;
}

#endif /* DIVOOM_PRODUCT_ASTRO_TOO */

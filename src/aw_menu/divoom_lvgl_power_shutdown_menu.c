#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_power_shutdown_menu.h"

typedef struct _divoom_lvgl_power_shutdown_menu_ctrl
{
	lv_obj_t *main_obj;
} divoom_lvgl_power_shutdown_menu_ctrl;

static void divoom_lvgl_power_shutdown_menu_key_cb(lv_event_t *e)
{
	(void)e;
	/* 真关机黑屏：吞掉按键，勿开屏/切页 */
}

void *divoom_lvgl_power_shutdown_menu_init(void *user_data, int menu_id)
{
	divoom_lvgl_power_shutdown_menu_ctrl *phandle;

	(void)user_data;
	(void)menu_id;

	phandle = (divoom_lvgl_power_shutdown_menu_ctrl *)divoom_system_malloc(
		sizeof(divoom_lvgl_power_shutdown_menu_ctrl));
	if (!phandle)
		return NULL;

	phandle->main_obj = lv_obj_create(NULL);
	lv_obj_remove_style_all(phandle->main_obj);
	lv_obj_set_style_bg_opa(phandle->main_obj, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(0x000000), 0);
	lv_obj_set_style_pad_all(phandle->main_obj, 0, 0);
	lv_obj_set_style_border_width(phandle->main_obj, 0, 0);
	lv_obj_set_style_shadow_width(phandle->main_obj, 0, 0);
	lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(phandle->main_obj, 0, 0);
	lv_obj_add_event_cb(phandle->main_obj, divoom_lvgl_power_shutdown_menu_key_cb,
			    LV_EVENT_KEY, phandle);

	return phandle;
}

void divoom_lvgl_power_shutdown_menu_disp(void *handle, void *active_obj)
{
	divoom_lvgl_power_shutdown_menu_ctrl *phandle =
		(divoom_lvgl_power_shutdown_menu_ctrl *)handle;

	(void)active_obj;
	if (!phandle || !phandle->main_obj)
		return;

	lv_scr_load(phandle->main_obj);
	lv_group_focus_obj(phandle->main_obj);
	divoom_screen_bright(0);
}

void divoom_lvgl_power_shutdown_menu_exit(void *handle)
{
	divoom_lvgl_power_shutdown_menu_ctrl *phandle =
		(divoom_lvgl_power_shutdown_menu_ctrl *)handle;

	if (!phandle)
		return;
	if (phandle->main_obj)
		lv_obj_del(phandle->main_obj);
	divoom_system_free(phandle);
}

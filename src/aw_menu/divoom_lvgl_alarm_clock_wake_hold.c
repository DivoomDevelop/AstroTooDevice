#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"

#if defined(DIVOOM_TIME_FRAME_SPEAKER)
#include "divoom_lvgl_alarm_clock_wake_hold.h"

typedef struct _divoom_lvgl_alarm_clock_wake_hold_ctrl
{
	int menu_id;
	lv_obj_t *main_obj;
	lv_obj_t *key_btn;
} divoom_lvgl_alarm_clock_wake_hold_ctrl;

static void divoom_lvgl_alarm_clock_wake_hold_event_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	divoom_print("wft test\n");
	if (code == LV_EVENT_KEY) {
		char *key_info = (char *)lv_event_get_param(e);

		if (key_info)
			divoom_main_keypad_send_key_info((uint8)key_info[0], (uint16)key_info[1]);
	} else if (code == LV_EVENT_CLICKED) {
		/* 触摸无 LV 键值：等同灯键短按，走 main 里 try_dismiss 分支 */
		divoom_main_keypad_send_key_info(DIVOOM_KEY_LIGHT_ENTER, DIVOOM_PRESS_SHORT);
	}
}

void *divoom_lvgl_alarm_clock_wake_hold_init(void *user_data, int menu_id)
{
	divoom_lvgl_alarm_clock_wake_hold_ctrl *h;

	divoom_print("wft test\n");
	(void)user_data;
	h = (divoom_lvgl_alarm_clock_wake_hold_ctrl *)divoom_system_malloc(sizeof(*h));
	if (!h)
		return NULL;
	h->menu_id = menu_id;
	h->main_obj = lv_obj_create(NULL);
	lv_obj_set_size(h->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_clear_flag(h->main_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(h->main_obj, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(h->main_obj, LV_OPA_COVER, 0);

	h->key_btn = lv_btn_create(h->main_obj);
	lv_obj_remove_style_all(h->key_btn);
	lv_obj_set_size(h->key_btn, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(h->key_btn, 0, 0);
	lv_obj_set_style_bg_opa(h->key_btn, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(h->key_btn, 0, 0);
	lv_obj_add_flag(h->key_btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);
	lv_obj_add_event_cb(h->key_btn, divoom_lvgl_alarm_clock_wake_hold_event_cb, LV_EVENT_KEY, NULL);

	return h;
}

void divoom_lvgl_alarm_clock_wake_hold_disp(void *handle, void *active_obj)
{
	divoom_lvgl_alarm_clock_wake_hold_ctrl *h = (divoom_lvgl_alarm_clock_wake_hold_ctrl *)handle;
	lv_group_t *grp;

	(void)active_obj;
	if (!h)
		return;

	lv_scr_load(h->main_obj);
	divoom_menu_control_change_disp_mode(h->main_obj, 0);
	grp = lv_group_get_default();
	if (grp) {
		lv_group_remove_all_objs(grp);
		lv_group_add_obj(grp, h->key_btn);
		lv_group_focus_obj(h->key_btn);
	}
	{
		lv_disp_t *disp = lv_disp_get_default();

		if (disp)
			lv_refr_now(disp);
	}

	divoom_print("wft test 1111111111111111111111111111111\n");
//	divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_START_UP_DISP_DIAL);
}

void divoom_lvgl_alarm_clock_wake_hold_pause(void *handle)
{
	(void)handle;
}

void divoom_lvgl_alarm_clock_wake_hold_exit(void *handle)
{
	divoom_lvgl_alarm_clock_wake_hold_ctrl *h = (divoom_lvgl_alarm_clock_wake_hold_ctrl *)handle;

	if (!h)
		return;
	if (h->main_obj)
		lv_obj_del(h->main_obj);
	divoom_system_free(h);
}
#endif /* DIVOOM_TIME_FRAME_SPEAKER */

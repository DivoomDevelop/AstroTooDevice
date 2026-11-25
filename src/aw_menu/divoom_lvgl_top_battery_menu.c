#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_font_manage.h"
#include "divoom_lvgl_poweron.h"
#include "divoom_lvgl_top_battery_menu.h"
#include "divoom_lvgl_top_control.h"

#ifdef DIVOOM_PRODUCT_ASTRO_TOO
#include "divoom_astrootoo_theme.h"

/* 顶栏 toast 图标与 source_switch / reached_home 同源（82/）；40×40 SVG 导出 */
#define DIVOOM_ASTRO_TOP_TOAST_UI_DIR "82/"

static const char *divoom_lvgl_top_battery_menu_icon_path(int battery_flag)
{
	switch (battery_flag) {
	case 0:
		return DIVOOM_ASTRO_TOP_TOAST_UI_DIR "at_toast_battery_low.png";
	case 1:
		return DIVOOM_ASTRO_TOP_TOAST_UI_DIR "at_toast_battery_charging.png";
	default:
		return DIVOOM_ASTRO_TOP_TOAST_UI_DIR "at_toast_battery_full.png";
	}
}
#endif

typedef struct _divoom_lvgl_top_battery_menu_ctrl
{
	void *main_obj;		

	int bettery_flag;
	void *top_disp_obj;

	void *img_obj;
	void *label_obj;

	int exit_time;
	lv_timer_t *menu_exit_timer;
}divoom_lvgl_top_battery_menu_ctrl, *Pdivoom_lvgl_top_battery_menu_ctrl;

Pdivoom_lvgl_top_battery_menu_ctrl pdivoom_lvgl_top_battery_menu_handle = NULL;	
static volatile int s_battery_tip_pending_valid = 0;
static volatile int s_battery_tip_pending_flag = 0;

static void divoom_lvgl_top_battery_menu_show(int battery_flag);

void divoom_lvgl_top_battery_menu_del_menu(void)
{
	if(pdivoom_lvgl_top_battery_menu_handle->menu_exit_timer)
	{
		lv_timer_del(pdivoom_lvgl_top_battery_menu_handle->menu_exit_timer);
	}
    divoom_lvgl_top_control_remove_top(pdivoom_lvgl_top_battery_menu_handle);
	divoom_menu_control_update_video_toast(DIVOOM_VIDEO_TOAST_BATTERY, NULL);
	lv_obj_del_async(pdivoom_lvgl_top_battery_menu_handle->main_obj);
	divoom_system_free(pdivoom_lvgl_top_battery_menu_handle);
	pdivoom_lvgl_top_battery_menu_handle = NULL;
}


void divoom_lvgl_top_battery_menu_time_handle(lv_timer_t *exit_timer)
{
	if(pdivoom_lvgl_top_battery_menu_handle != NULL)
	{
		pdivoom_lvgl_top_battery_menu_handle->exit_time++;

		if(pdivoom_lvgl_top_battery_menu_handle->exit_time == 1)
		{
			lv_obj_clear_flag(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, LV_OBJ_FLAG_HIDDEN);
			divoom_lvgl_layer_top_raise(pdivoom_lvgl_top_battery_menu_handle->main_obj);
		}
		else if(pdivoom_lvgl_top_battery_menu_handle->exit_time >= 8)//主菜单不变动自动退出
		{
			divoom_lvgl_top_battery_menu_del_menu();
		}
	}
}

void divoom_lvgl_top_battery_menu_update_disp(void)//0:低电量  1:充电中            2:电池已充满
{
#ifdef DIVOOM_PRODUCT_ASTRO_TOO
	divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_battery_menu_handle->img_obj,
		divoom_lvgl_top_battery_menu_icon_path(pdivoom_lvgl_top_battery_menu_handle->bettery_flag));
#else
	if(pdivoom_lvgl_top_battery_menu_handle->bettery_flag == 0)
    {
		divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_battery_menu_handle->img_obj, "98/98_low_charge.png");
	}
	else if(pdivoom_lvgl_top_battery_menu_handle->bettery_flag == 1)
    {
		divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_battery_menu_handle->img_obj, "98/98_charging.png");
	}
	else if(pdivoom_lvgl_top_battery_menu_handle->bettery_flag == 2)
	{
		divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_battery_menu_handle->img_obj, "98/98_charge_max.png");
	}
#endif
	if(pdivoom_lvgl_top_battery_menu_handle->bettery_flag == 0)
   {
		lv_label_set_text(pdivoom_lvgl_top_battery_menu_handle->label_obj, divoom_menu_control_get_disp_str("LowBatteryTip"));
	}
	else if(pdivoom_lvgl_top_battery_menu_handle->bettery_flag == 1)
   {
		lv_label_set_text(pdivoom_lvgl_top_battery_menu_handle->label_obj, divoom_menu_control_get_disp_str("ChargingBatteryTip"));
	}
	else if(pdivoom_lvgl_top_battery_menu_handle->bettery_flag == 2)
	{
		lv_label_set_text(pdivoom_lvgl_top_battery_menu_handle->label_obj, divoom_menu_control_get_disp_str("MaxBatteryTip"));
	}
}

void divoom_lvgl_top_battery_menu_init(int battery_flag)//0:低电量  1:充电中            2:电池已充满
{
	/* UART/充电检测线程禁止直接操作 LVGL；由 LVGL handler poll 展示 */
	s_battery_tip_pending_flag = battery_flag;
	s_battery_tip_pending_valid = 1;
}

/* 低电 toast（flag==0）是否正在显示，供顶格音量+防抖 */
int divoom_lvgl_top_battery_menu_low_tip_active(void)
{
	if (pdivoom_lvgl_top_battery_menu_handle != NULL
	    && pdivoom_lvgl_top_battery_menu_handle->bettery_flag == 0)
		return 1;
	if (s_battery_tip_pending_valid && s_battery_tip_pending_flag == 0)
		return 1;
	return 0;
}

void divoom_lvgl_top_battery_menu_poll(void)
{
	int flag;

	divoom_menu_control_update_video_toast(DIVOOM_VIDEO_TOAST_BATTERY,
		pdivoom_lvgl_top_battery_menu_handle ? pdivoom_lvgl_top_battery_menu_handle->top_disp_obj : NULL);
	if (!s_battery_tip_pending_valid)
		return;
	/* 字体与语言表都在 menu_control_init 里建；未就绪时先留着，等 UI 起来再弹 */
	if (!divoom_lvgl_font_manage_is_inited())
		return;
	flag = s_battery_tip_pending_flag;
	/* 开机动画/开机页未结束：低电 toast 文字会乱码；先留着 pending，不要进 show */
	if (flag == 0 &&
	    (divoom_lvgl_poweron_is_boot_pending() ||
	     divoom_menu_control_get_active_menu_id() == DIVOOM_MENU_CONTROL_ID_SYSTEM_POWER_ON))
		return;
	s_battery_tip_pending_valid = 0;
	divoom_lvgl_top_battery_menu_show(flag);
}

static void divoom_lvgl_top_battery_menu_show(int battery_flag)
{
	divoom_get_lvgl_lock();
	if(divoom_menu_control_get_active_menu_id() == DIVOOM_MENU_CONTROL_ID_SYSTEM_POWER_ON)
	{
		divoom_free_lvgl_lock();
		return;
	}

	if(pdivoom_lvgl_top_battery_menu_handle != NULL)
	{
		pdivoom_lvgl_top_battery_menu_handle->exit_time = 0;
		pdivoom_lvgl_top_battery_menu_handle->bettery_flag = battery_flag;
		divoom_lvgl_top_battery_menu_update_disp();
		lv_obj_add_flag(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, LV_OBJ_FLAG_HIDDEN);
		divoom_lvgl_layer_top_raise(pdivoom_lvgl_top_battery_menu_handle->main_obj);
		divoom_free_lvgl_lock();
		return;
	}
	pdivoom_lvgl_top_battery_menu_handle = (Pdivoom_lvgl_top_battery_menu_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_top_battery_menu_ctrl));
	if(pdivoom_lvgl_top_battery_menu_handle == NULL)
	{
		divoom_free_lvgl_lock();
		return;
	}
	memset(pdivoom_lvgl_top_battery_menu_handle, 0, sizeof(divoom_lvgl_top_battery_menu_ctrl));
	pdivoom_lvgl_top_battery_menu_handle->exit_time = 0;
	pdivoom_lvgl_top_battery_menu_handle->bettery_flag = battery_flag;
	pdivoom_lvgl_top_battery_menu_handle->main_obj = lv_obj_create(lv_layer_top()); //在顶层创建主页面
	lv_obj_set_style_bg_opa(pdivoom_lvgl_top_battery_menu_handle->main_obj, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(pdivoom_lvgl_top_battery_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_border_width(pdivoom_lvgl_top_battery_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_shadow_width(pdivoom_lvgl_top_battery_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_outline_width(pdivoom_lvgl_top_battery_menu_handle->main_obj, 0, 0);
	lv_obj_clear_flag(pdivoom_lvgl_top_battery_menu_handle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(pdivoom_lvgl_top_battery_menu_handle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(pdivoom_lvgl_top_battery_menu_handle->main_obj, 0, 0);
	pdivoom_lvgl_top_battery_menu_handle->top_disp_obj = lv_btn_create(pdivoom_lvgl_top_battery_menu_handle->main_obj);
#ifdef DIVOOM_PRODUCT_ASTRO_TOO
	{
		lv_obj_t *img_obj;
		lv_obj_t *label;
		lv_obj_t *spacer;
		lv_coord_t text_w;
		const char *img_path;
		const char *str_key;

		if (pdivoom_lvgl_top_battery_menu_handle->bettery_flag == 0) {
			str_key = "LowBatteryTip";
		} else if (pdivoom_lvgl_top_battery_menu_handle->bettery_flag == 1) {
			str_key = "ChargingBatteryTip";
		} else {
			str_key = "MaxBatteryTip";
		}
		img_path = divoom_lvgl_top_battery_menu_icon_path(pdivoom_lvgl_top_battery_menu_handle->bettery_flag);

		lv_obj_set_style_bg_color(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, lv_color_hex(DIVOOM_ASTRO_BG), 0);
		lv_obj_set_style_bg_opa(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, LV_OPA_COVER, 0);
		lv_obj_set_style_border_width(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, 2, 0);
		lv_obj_set_style_border_color(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
		lv_obj_set_style_radius(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, DIVOOM_ASTRO_RADIUS_MD, 0);
		lv_obj_set_style_pad_top(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, DIVOOM_ASTRO_TOP_TOAST_PAD_Y, 0);
		lv_obj_set_style_pad_bottom(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, DIVOOM_ASTRO_TOP_TOAST_PAD_Y, 0);
		lv_obj_set_style_pad_left(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, DIVOOM_ASTRO_TOP_TOAST_PAD_X, 0);
		lv_obj_set_style_pad_right(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, DIVOOM_ASTRO_TOP_TOAST_PAD_X, 0);
		lv_obj_set_style_pad_column(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, DIVOOM_ASTRO_TOP_TOAST_GAP, 0);
		lv_obj_set_style_shadow_width(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, 0, 0);
		lv_obj_set_style_outline_width(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, 0, 0);
		lv_obj_clear_flag(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_set_pos(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj,
			       DIVOOM_ASTRO_TOP_TOAST_X, DIVOOM_ASTRO_TOP_TOAST_Y);
		lv_obj_set_size(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj,
				DIVOOM_ASTRO_TOP_TOAST_W, DIVOOM_ASTRO_TOP_TOAST_H);
		lv_obj_set_flex_flow(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, LV_FLEX_FLOW_ROW);
		lv_obj_set_flex_align(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj,
				      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		img_obj = lv_img_create(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj);
		divoom_menu_control_lv_img_set_src(img_obj, img_path);
		lv_obj_set_size(img_obj, DIVOOM_ASTRO_TOP_TOAST_ICON, DIVOOM_ASTRO_TOP_TOAST_ICON);
		pdivoom_lvgl_top_battery_menu_handle->img_obj = img_obj;

		text_w = DIVOOM_ASTRO_TOP_TOAST_W - DIVOOM_ASTRO_TOP_TOAST_PAD_X * 2
			 - DIVOOM_ASTRO_TOP_TOAST_ICON * 2 - DIVOOM_ASTRO_TOP_TOAST_GAP * 2;
		if (text_w < 120)
			text_w = 120;

		label = lv_label_create(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj);
		lv_obj_set_width(label, text_w);
		lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
		lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
		lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(22, DIVOOM_MENU_CONTROL_ID_TOP_BATTERY_MENU), 0);
		lv_obj_set_style_text_color(label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
		lv_label_set_text(label, divoom_menu_control_get_disp_str(str_key));
		pdivoom_lvgl_top_battery_menu_handle->label_obj = label;

		spacer = lv_obj_create(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj);
		lv_obj_set_size(spacer, DIVOOM_ASTRO_TOP_TOAST_ICON, DIVOOM_ASTRO_TOP_TOAST_ICON);
		lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
		lv_obj_set_style_border_width(spacer, 0, 0);
		lv_obj_set_style_pad_all(spacer, 0, 0);
		lv_obj_clear_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);
	}
#else
	lv_obj_set_style_outline_width(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, 0, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, 4, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_shadow_width(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, 0, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_pad_all(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, 0, 0);
	lv_obj_set_style_bg_color(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_bg_opa(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, 85*LV_OPA_100/100, 0);
	lv_obj_set_style_border_color(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, lv_color_hex(0x0), 0);
	lv_obj_set_style_border_opa(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, 12*LV_OPA_100/100, 0);
	lv_obj_set_style_radius(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, 48, 0);	
    lv_obj_clear_flag(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, 40, 40);
    lv_obj_set_size(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj, 720, 96);
    void *img_obj = lv_img_create(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj);
	if(pdivoom_lvgl_top_battery_menu_handle->bettery_flag == 0)
    {
		divoom_menu_control_lv_img_set_src(img_obj, "98/98_low_charge.png");
	}
	else if(pdivoom_lvgl_top_battery_menu_handle->bettery_flag == 1)
    {
		divoom_menu_control_lv_img_set_src(img_obj, "98/98_charging.png");
	}
	else if(pdivoom_lvgl_top_battery_menu_handle->bettery_flag == 2)
	{
		divoom_menu_control_lv_img_set_src(img_obj, "98/98_charge_max.png");
	}
	lv_obj_set_pos(img_obj, 21-4, 21-4);
    pdivoom_lvgl_top_battery_menu_handle->img_obj = img_obj;
	void *label = lv_label_create(pdivoom_lvgl_top_battery_menu_handle->top_disp_obj);
    lv_obj_set_pos(label, 96, 12+18);
    lv_obj_set_size(label, 466, 72);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(23, DIVOOM_MENU_CONTROL_ID_TOP_BATTERY_MENU), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x0), 0);
	if(pdivoom_lvgl_top_battery_menu_handle->bettery_flag == 0)
    {
		lv_label_set_text(label, divoom_menu_control_get_disp_str("LowBatteryTip"));
	}
	else if(pdivoom_lvgl_top_battery_menu_handle->bettery_flag == 1)
    {
		lv_label_set_text(label, divoom_menu_control_get_disp_str("ChargingBatteryTip"));
	}
	else if(pdivoom_lvgl_top_battery_menu_handle->bettery_flag == 2)
	{
		lv_label_set_text(label, divoom_menu_control_get_disp_str("MaxBatteryTip"));
	}
	pdivoom_lvgl_top_battery_menu_handle->label_obj = label;
#endif
	pdivoom_lvgl_top_battery_menu_handle->menu_exit_timer = lv_timer_create(divoom_lvgl_top_battery_menu_time_handle, 500, NULL);
	divoom_lvgl_top_control_add_top(pdivoom_lvgl_top_battery_menu_handle, pdivoom_lvgl_top_battery_menu_handle->top_disp_obj);
	divoom_lvgl_layer_top_raise(pdivoom_lvgl_top_battery_menu_handle->main_obj);
	divoom_free_lvgl_lock();
}

void divoom_lvgl_top_battery_menu_exit(void)
{
	s_battery_tip_pending_valid = 0;
	if(pdivoom_lvgl_top_battery_menu_handle != NULL)
	{
		if(pdivoom_lvgl_top_battery_menu_handle->menu_exit_timer)
		{
			lv_timer_del(pdivoom_lvgl_top_battery_menu_handle->menu_exit_timer);
		}
        divoom_lvgl_top_control_remove_top(pdivoom_lvgl_top_battery_menu_handle);
		divoom_menu_control_update_video_toast(DIVOOM_VIDEO_TOAST_BATTERY, NULL);
		lv_obj_del_async(pdivoom_lvgl_top_battery_menu_handle->main_obj);
		divoom_system_free(pdivoom_lvgl_top_battery_menu_handle);
		pdivoom_lvgl_top_battery_menu_handle = NULL;
	}
}


#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"

typedef struct _divoom_lvgl_setting_theme_ctrl
{
    int menu_id;
	lv_style_t backgroud_type;
	void *main_obj;

	void *cur_active_obj;
	lv_style_t btn_normal_style;
	lv_style_t btn_select_style;
	
	int theme_flag;

	void *theme_btn[2];
	void *theme_label[2];
	void *theme_select_img[2];
}divoom_lvgl_setting_theme_ctrl, *Pdivoom_lvgl_setting_theme_ctrl;

static void divoom_lvgl_setting_theme_menu_event_cb(lv_event_t * e)
{
	lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_setting_theme_ctrl phandle = (Pdivoom_lvgl_setting_theme_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
	Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();

	if(code == LV_EVENT_KEY)
	{
		char c = *((char *)lv_event_get_param(e));
		
		if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			if(phandle->theme_flag)
			{
				phandle->theme_flag = 0;
				lv_group_focus_obj(phandle->theme_btn[0]);
				lv_obj_set_style_text_color(phandle->theme_label[0], lv_color_hex(0xF2F2F2), 0);
				lv_obj_set_style_text_color(phandle->theme_label[1], lv_color_hex(0x909090), 0);
			}
			else
			{
				phandle->theme_flag = 1;
				lv_group_focus_obj(phandle->theme_btn[1]);
				lv_obj_set_style_text_color(phandle->theme_label[0], lv_color_hex(0x909090), 0);
				lv_obj_set_style_text_color(phandle->theme_label[1], lv_color_hex(0xF2F2F2), 0);
			}
		}
		else if(c == DIVOOM_LVGL_KEY_PRE)
		{
			if(phandle->theme_flag)
			{
				phandle->theme_flag = 0;
				lv_group_focus_obj(phandle->theme_btn[0]);
				lv_obj_set_style_text_color(phandle->theme_label[0], lv_color_hex(0xF2F2F2), 0);
				lv_obj_set_style_text_color(phandle->theme_label[1], lv_color_hex(0x909090), 0);
			}
			else
			{
				phandle->theme_flag = 1;
				lv_group_focus_obj(phandle->theme_btn[1]);
				lv_obj_set_style_text_color(phandle->theme_label[0], lv_color_hex(0x909090), 0);
				lv_obj_set_style_text_color(phandle->theme_label[1], lv_color_hex(0xF2F2F2), 0);
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			if(phandle->theme_flag)
			{
				lv_obj_add_flag(phandle->theme_select_img[0], LV_OBJ_FLAG_HIDDEN);
				lv_obj_clear_flag(phandle->theme_select_img[1], LV_OBJ_FLAG_HIDDEN);
			}
			else
			{
				lv_obj_add_flag(phandle->theme_select_img[1], LV_OBJ_FLAG_HIDDEN);
				lv_obj_clear_flag(phandle->theme_select_img[0], LV_OBJ_FLAG_HIDDEN);
			}
			if(psys_config->theme_flag != phandle->theme_flag)
			{
				psys_config->theme_flag = phandle->theme_flag;
				divoom_main_save_config();
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_menu_control_return_pre();
		}
		else
		{
			return;
		}
	}
}

void *divoom_lvgl_setting_theme_init(void *user_data, int menu_id)
{
	Pdivoom_lvgl_setting_theme_ctrl phandle = divoom_system_malloc(sizeof(divoom_lvgl_setting_theme_ctrl));

	Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();
	phandle->theme_flag = psys_config->theme_flag;
	phandle->cur_active_obj = NULL;
	phandle->main_obj = lv_obj_create(NULL);
    phandle->menu_id = menu_id;
	lv_style_init(&phandle->backgroud_type);
    lv_style_set_bg_color(&phandle->backgroud_type, lv_color_hex(0x000000));
    lv_style_set_text_color(&phandle->backgroud_type, lv_color_hex(0xFFFFFF));
    lv_obj_add_style(phandle->main_obj, &phandle->backgroud_type, 0);
    lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);

	
	lv_style_init(&phandle->btn_normal_style);
	lv_style_set_bg_color(&phandle->btn_normal_style, lv_color_black());
	lv_style_set_pad_all(&phandle->btn_normal_style, 0);
	lv_style_set_border_width(&phandle->btn_normal_style, 6);
	lv_style_set_border_color(&phandle->btn_normal_style, lv_color_hex(0x404040));
	lv_style_set_radius(&phandle->btn_normal_style, 32);
	lv_style_set_shadow_width(&phandle->btn_normal_style, 0);
	lv_style_set_outline_width(&phandle->btn_normal_style, 0);
	
	lv_style_init(&phandle->btn_select_style);
	lv_style_set_bg_color(&phandle->btn_select_style, lv_color_black());
	lv_style_set_pad_all(&phandle->btn_select_style, 0);
	lv_style_set_border_width(&phandle->btn_select_style, 6);
	lv_style_set_border_color(&phandle->btn_select_style, lv_color_hex(0xBBBBBB));
	lv_style_set_radius(&phandle->btn_select_style, 32);
	lv_style_set_shadow_width(&phandle->btn_select_style, 0);
	lv_style_set_outline_width(&phandle->btn_select_style, 0);

	lv_obj_t *title = lv_label_create(phandle->main_obj);
	lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_size(title, 400-100, 56);
	lv_obj_set_pos(title, 334-100, 61);
	lv_label_set_text(title, divoom_menu_control_get_disp_str("ThemeAppearance"));

	//classic
	lv_obj_t *classic_btn = lv_btn_create(phandle->main_obj);
	lv_obj_set_size(classic_btn, 332+12, 524+12);
	lv_obj_set_pos(classic_btn, 46, 198);
	lv_obj_add_style(classic_btn, &phandle->btn_normal_style, 0);
	lv_obj_add_style(classic_btn, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(classic_btn, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	//lv_obj_set_style_pad_all(classic_btn, 0, 0);
	//lv_obj_set_style_shadow_width(classic_btn, 0, 0);
	//lv_obj_set_style_outline_width(classic_btn, 0, 0);
	//lv_obj_set_style_outline_width(classic_btn, 0, LV_STATE_FOCUSED);
	//lv_obj_set_style_outline_width(classic_btn, 0, LV_STATE_FOCUS_KEY);
	//lv_obj_set_style_radius(classic_btn, 32, 0);
	//lv_obj_set_style_bg_color(classic_btn, lv_color_black(), 0);
	//lv_obj_set_style_border_width(classic_btn, 6, 0);
	//lv_obj_set_style_border_color(classic_btn, lv_color_hex(0x404040), 0);
	//lv_obj_set_style_border_color(classic_btn, lv_color_hex(0xBBBBBB), LV_STATE_FOCUSED);
	//lv_obj_set_style_border_color(classic_btn, lv_color_hex(0xBBBBBB), LV_STATE_FOCUS_KEY);
	lv_obj_add_event_cb(classic_btn, divoom_lvgl_setting_theme_menu_event_cb, LV_EVENT_KEY, phandle);
	phandle->theme_btn[0] = classic_btn;

	lv_obj_t *classic_img = lv_img_create(classic_btn);
	lv_obj_set_size(classic_img, 320, 512);
	lv_obj_set_pos(classic_img, 6, 6);
	divoom_menu_control_lv_img_set_src(classic_img, "71/71_classic.png");

	lv_obj_t *system_label = lv_label_create(classic_img);
	lv_obj_set_style_text_font(system_label, divoom_menu_control_get_font_info(12, phandle->menu_id), 0);
	lv_obj_set_style_text_color(system_label, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_align(system_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_size(system_label, 85, 16);
	lv_obj_set_pos(system_label, 90, 119);
	lv_label_set_text(system_label, divoom_menu_control_get_disp_str("SystemSetting"));

	lv_obj_t *dial_label = lv_label_create(classic_img);
	lv_obj_set_style_text_font(dial_label, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
	lv_obj_set_style_text_color(dial_label, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_align(dial_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_size(dial_label, 123, 37);
	lv_obj_set_pos(dial_label, 52, 238);
	lv_label_set_text(dial_label, divoom_menu_control_get_disp_str("Dial"));

	lv_obj_t *slide_label = lv_label_create(classic_img);
	lv_obj_set_style_text_font(slide_label, divoom_menu_control_get_font_info(12, phandle->menu_id), 0);
	lv_obj_set_style_text_color(slide_label, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_align(slide_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_size(slide_label, 85, 16);
	lv_obj_set_pos(slide_label, 90, 376);
	lv_label_set_text(slide_label, divoom_menu_control_get_disp_str("SlideEffect"));
	
	lv_obj_t *classic_theme_label = lv_label_create(phandle->main_obj);
	lv_obj_set_style_text_font(classic_theme_label, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_obj_set_style_text_color(classic_theme_label, lv_color_hex(0x909090), 0);
	lv_obj_set_style_text_align(classic_theme_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_size(classic_theme_label, 344, 64);
	lv_obj_set_pos(classic_theme_label, 40, 760+16);
	lv_label_set_text(classic_theme_label, divoom_menu_control_get_disp_str("ClassicAndSimple"));
	phandle->theme_label[0] = classic_theme_label;

	lv_obj_t *circle_img1 = lv_img_create(phandle->main_obj);
	lv_obj_set_size(circle_img1, 58, 58);
	lv_obj_set_pos(circle_img1, 183, 856);
	divoom_menu_control_lv_img_set_src(circle_img1, "71/71_circle.png");

	lv_obj_t *select_img1 = lv_img_create(phandle->main_obj);
	lv_obj_set_size(select_img1, 44, 44);
	lv_obj_set_pos(select_img1, 193, 852);
	divoom_menu_control_lv_img_set_src(select_img1, "71/71_select.png");
	lv_obj_add_flag(select_img1, LV_OBJ_FLAG_HIDDEN);
	phandle->theme_select_img[0] = select_img1;

	//Sci-Fi UI
	lv_obj_t *sci_fi_btn = lv_btn_create(phandle->main_obj);
	lv_obj_set_size(sci_fi_btn, 332+12, 524+12);
	lv_obj_set_pos(sci_fi_btn, 422, 198);
	lv_obj_add_style(sci_fi_btn, &phandle->btn_normal_style, 0);
	lv_obj_add_style(sci_fi_btn, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(sci_fi_btn, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	//lv_obj_set_style_pad_all(sci_fi_btn, 0, 0);
	//lv_obj_set_style_shadow_width(sci_fi_btn, 0, 0);
	//lv_obj_set_style_outline_width(sci_fi_btn, 0, 0);
	//lv_obj_set_style_outline_width(sci_fi_btn, 0, LV_STATE_FOCUSED);
	//lv_obj_set_style_outline_width(sci_fi_btn, 0, LV_STATE_FOCUS_KEY);
	//lv_obj_set_style_radius(sci_fi_btn, 32, 0);
	//lv_obj_set_style_bg_color(sci_fi_btn, lv_color_black(), 0);
	//lv_obj_set_style_border_width(sci_fi_btn, 6, 0);
	//lv_obj_set_style_border_color(sci_fi_btn, lv_color_hex(0x404040), 0);
	//lv_obj_set_style_border_color(sci_fi_btn, lv_color_hex(0xBBBBBB), LV_STATE_FOCUSED);
	//lv_obj_set_style_border_color(sci_fi_btn, lv_color_hex(0xBBBBBB), LV_STATE_FOCUS_KEY);
	lv_obj_add_event_cb(sci_fi_btn, divoom_lvgl_setting_theme_menu_event_cb, LV_EVENT_KEY, phandle);
	phandle->theme_btn[1] = sci_fi_btn;

	lv_obj_t *sci_fi_img = lv_img_create(sci_fi_btn);
	lv_obj_set_size(sci_fi_img, 320, 512);
	lv_obj_set_pos(sci_fi_img, 6, 6);
	divoom_menu_control_lv_img_set_src(sci_fi_img, "71/71_sci_fi.png");

	lv_obj_t *system_label2 = lv_label_create(sci_fi_img);
	lv_obj_set_style_text_font(system_label2, divoom_menu_control_get_font_info(12, phandle->menu_id), 0);
	lv_obj_set_style_text_color(system_label2, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_align(system_label2, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_size(system_label2, 85, 16);
	lv_obj_set_pos(system_label2, 87, 115);
	lv_label_set_text(system_label2, divoom_menu_control_get_disp_str("SystemSetting"));

	lv_obj_t *dial_label2 = lv_label_create(sci_fi_img);
	lv_obj_set_style_text_font(dial_label2, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
	lv_obj_set_style_text_color(dial_label2, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_align(dial_label2, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_size(dial_label2, 123, 37);
	lv_obj_set_pos(dial_label2, 52, 238);
	lv_label_set_text(dial_label2, divoom_menu_control_get_disp_str("Dial"));

	lv_obj_t *slide_label2 = lv_label_create(sci_fi_img);
	lv_obj_set_style_text_font(slide_label2, divoom_menu_control_get_font_info(12, phandle->menu_id), 0);
	lv_obj_set_style_text_color(slide_label2, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_align(slide_label2, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_size(slide_label2, 85, 16);
	lv_obj_set_pos(slide_label2, 90, 380);
	lv_label_set_text(slide_label2, divoom_menu_control_get_disp_str("SlideEffect"));
	
	lv_obj_t *sci_fi_theme_label = lv_label_create(phandle->main_obj);
	lv_obj_set_style_text_font(sci_fi_theme_label, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_obj_set_style_text_color(sci_fi_theme_label, lv_color_hex(0x909090), 0);
	lv_obj_set_style_text_align(sci_fi_theme_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_size(sci_fi_theme_label, 344, 64);
	lv_obj_set_pos(sci_fi_theme_label, 416, 760+16);
	lv_label_set_text(sci_fi_theme_label, divoom_menu_control_get_disp_str("Sci-Fi UI"));
	phandle->theme_label[1] = sci_fi_theme_label;

	lv_obj_t *circle_img2 = lv_img_create(phandle->main_obj);
	lv_obj_set_size(circle_img2, 58, 58);
	lv_obj_set_pos(circle_img2, 559, 856);
	divoom_menu_control_lv_img_set_src(circle_img2, "71/71_circle.png");

	lv_obj_t *select_img2 = lv_img_create(phandle->main_obj);
	lv_obj_set_size(select_img2, 44, 44);
	lv_obj_set_pos(select_img2, 193+376, 852);
	divoom_menu_control_lv_img_set_src(select_img2, "71/71_select.png");
	lv_obj_add_flag(select_img2, LV_OBJ_FLAG_HIDDEN);
	phandle->theme_select_img[1] = select_img2;

	if(phandle->theme_flag)
	{
		phandle->cur_active_obj = phandle->theme_btn[1];
		lv_obj_set_style_text_color(phandle->theme_label[1], lv_color_hex(0xF2F2F2), 0);
		lv_obj_clear_flag(phandle->theme_select_img[1], LV_OBJ_FLAG_HIDDEN);
	}
	else
	{
		phandle->cur_active_obj = phandle->theme_btn[0];
		lv_obj_set_style_text_color(phandle->theme_label[0], lv_color_hex(0xF2F2F2), 0);
		lv_obj_clear_flag(phandle->theme_select_img[0], LV_OBJ_FLAG_HIDDEN);
	}
	
	return phandle;
}


void divoom_lvgl_setting_theme_disp(void *handle, void *active)
{
	Pdivoom_lvgl_setting_theme_ctrl phandle = (Pdivoom_lvgl_setting_theme_ctrl)handle;

	if(phandle->cur_active_obj)
	{
		lv_group_focus_obj(phandle->cur_active_obj);
		phandle->cur_active_obj = NULL;
	}
	lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_setting_theme_exit(void *handle)
{
	Pdivoom_lvgl_setting_theme_ctrl phandle = (Pdivoom_lvgl_setting_theme_ctrl)handle;

	lv_style_reset(&phandle->backgroud_type);
	lv_style_reset(&phandle->btn_normal_style);
	lv_style_reset(&phandle->btn_select_style);
	lv_obj_del(phandle->main_obj);
	divoom_system_free(phandle);
}


#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"

typedef struct _divoom_lvgl_setting_color_temp_ctrl
{
    int menu_id;
    
	lv_style_t btn_normal_style;
	lv_style_t btn_select_style;
	lv_style_t btn_edit_style;

	int btn_status;
	char file_name[64];
	void *main_obj;	
	void *color_temp_btn;
	void *color_temp_slider;

	Pdivoom_sys_setting_info psys_config;
}divoom_lvgl_setting_color_temp_ctrl, *Pdivoom_lvgl_setting_color_temp_ctrl;

void divoom_lvgl_setting_color_temp_disp(void *handle, void *active_obj)
{
	Pdivoom_lvgl_setting_color_temp_ctrl phandle = (Pdivoom_lvgl_setting_color_temp_ctrl)handle;

	if(phandle->color_temp_btn)
	{
		lv_group_focus_obj(phandle->color_temp_btn);
	}
	else 
	{
		lv_group_focus_obj(active_obj);
	}

	lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_setting_color_temp_exit(void *handle)
{
	Pdivoom_lvgl_setting_color_temp_ctrl phandle = (Pdivoom_lvgl_setting_color_temp_ctrl)handle;
	
	lv_style_reset(&phandle->btn_normal_style);
	lv_style_reset(&phandle->btn_select_style);
	lv_style_reset(&phandle->btn_edit_style);
	lv_obj_del(phandle->main_obj);
	divoom_system_free(phandle);
}

void divoom_lvgl_setting_color_temp_event_cb(lv_event_t *e)
{
	lv_obj_t *cur_btn_obj = lv_event_get_target(e);
	lv_event_code_t code = lv_event_get_code(e);
	Pdivoom_lvgl_setting_color_temp_ctrl phandle = (Pdivoom_lvgl_setting_color_temp_ctrl)lv_event_get_user_data(e);
	int cur_pos, value;
	
	if(code == LV_EVENT_KEY)
	{
		char c = *((char *)lv_event_get_param(e));

		if(c == DIVOOM_LVGL_KEY_PRE)
		{
			if(phandle->btn_status == 1)
			{
				value = lv_slider_get_value(phandle->color_temp_slider);
				
				value = (value < 231) ? (value + 25) : (256);
				lv_slider_set_value(phandle->color_temp_slider, value, LV_ANIM_ON);
				divoom_main_set_color_temp(value);
			}
		}
		else if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			if(phandle->btn_status == 1)
			{
				value = lv_slider_get_value(phandle->color_temp_slider);
				
				value = (value > -231) ? (value - 25) : (-256);
				lv_slider_set_value(phandle->color_temp_slider, value, LV_ANIM_ON);
				divoom_main_set_color_temp(value);
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			if(phandle->btn_status == 0)
			{
				phandle->btn_status = 1;
				lv_obj_add_style(cur_btn_obj, &phandle->btn_edit_style, LV_STATE_FOCUSED);
				lv_obj_add_style(cur_btn_obj, &phandle->btn_edit_style, LV_STATE_FOCUS_KEY);
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			if(phandle->btn_status == 1)
			{
				phandle->btn_status = 0;
				lv_obj_add_style(cur_btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
				lv_obj_add_style(cur_btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
			}
			else
			{
				divoom_main_send_sys_settings_config();
				divoom_menu_control_return_pre();
			}
		}
		
	}

}


void *divoom_lvgl_setting_color_temp_init(void *user_data, int menu_id)
{
	Pdivoom_lvgl_setting_color_temp_ctrl phandle = divoom_system_malloc(sizeof(divoom_lvgl_setting_color_temp_ctrl));
	
	phandle->menu_id = menu_id;
	phandle->btn_status = 0;
	phandle->psys_config = divoom_main_get_system_config();

	phandle->main_obj = lv_obj_create(NULL);

	lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(0xE4E4E4), LV_PART_MAIN);
	lv_obj_set_pos(phandle->main_obj, 0, 0);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);  
	lv_obj_set_scrollbar_mode(phandle->main_obj, LV_SCROLLBAR_MODE_OFF);
	lv_obj_set_scroll_dir(phandle->main_obj, LV_DIR_VER);
	lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_SCROLLABLE);

	lv_style_init(&phandle->btn_normal_style);
	lv_style_set_bg_color(&phandle->btn_normal_style, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->btn_normal_style, 0);
	lv_style_set_border_width(&phandle->btn_normal_style, 4);
	lv_style_set_border_color(&phandle->btn_normal_style, lv_color_hex(0xE4E4E4));
	lv_style_set_radius(&phandle->btn_normal_style, 17);
	lv_style_set_shadow_width(&phandle->btn_normal_style, 0);
	lv_style_set_outline_width(&phandle->btn_normal_style, 0);

	lv_style_init(&phandle->btn_select_style);
	lv_style_set_bg_color(&phandle->btn_select_style, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->btn_select_style, 0);
	lv_style_set_border_width(&phandle->btn_select_style, 4);
	lv_style_set_border_color(&phandle->btn_select_style, lv_color_hex(0x6D6D6D));
	lv_style_set_radius(&phandle->btn_select_style, 17);
	lv_style_set_shadow_width(&phandle->btn_select_style, 0);
	lv_style_set_outline_width(&phandle->btn_select_style, 0);

	lv_style_init(&phandle->btn_edit_style);
	lv_style_set_bg_color(&phandle->btn_edit_style, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->btn_edit_style, 0);
	lv_style_set_border_width(&phandle->btn_edit_style, 4);
	lv_style_set_border_color(&phandle->btn_edit_style, lv_color_hex(0xE89B00));
	lv_style_set_radius(&phandle->btn_edit_style, 17);
	lv_style_set_shadow_width(&phandle->btn_edit_style, 0);
	lv_style_set_outline_width(&phandle->btn_edit_style, 0);

	lv_obj_t *title_label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(title_label_obj, 100, 50);
	lv_obj_set_size(title_label_obj, 600, 78);
    lv_label_set_text(title_label_obj, divoom_menu_control_get_disp_str("ColorTemp"));
    lv_obj_set_style_text_color(title_label_obj, lv_color_hex(0x0), 0);
    lv_obj_set_style_text_align(title_label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(title_label_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0); 

	lv_obj_t *cool_label_obj = lv_label_create(phandle->main_obj);
	lv_obj_set_pos(cool_label_obj, 76, 160);
	lv_obj_set_size(cool_label_obj, 310, 32);
	lv_label_set_text(cool_label_obj, divoom_menu_control_get_disp_str("Cooler"));
	lv_obj_set_style_text_color(cool_label_obj, lv_color_hex(0x636363), 0);
	lv_obj_set_style_text_align(cool_label_obj, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_font(cool_label_obj, divoom_menu_control_get_font_info(24, phandle->menu_id), 0); 

	lv_obj_t *warm_label_obj = lv_label_create(phandle->main_obj);
	lv_obj_set_pos(warm_label_obj, 414, 160);
	lv_obj_set_size(warm_label_obj, 310, 32);
	lv_label_set_text(warm_label_obj, divoom_menu_control_get_disp_str("Warmer"));
	lv_obj_set_style_text_color(warm_label_obj, lv_color_hex(0x636363), 0);
	lv_obj_set_style_text_align(warm_label_obj, LV_TEXT_ALIGN_RIGHT, 0);
	lv_obj_set_style_text_font(warm_label_obj, divoom_menu_control_get_font_info(24, phandle->menu_id), 0); 

	phandle->color_temp_btn = lv_btn_create(phandle->main_obj);

	lv_obj_add_style(phandle->color_temp_btn, &phandle->btn_normal_style, 0);
    lv_obj_add_style(phandle->color_temp_btn, &phandle->btn_select_style, LV_STATE_FOCUSED);
    lv_obj_add_style(phandle->color_temp_btn, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(phandle->color_temp_btn, 0, 0);
    lv_obj_set_pos(phandle->color_temp_btn, 44, 213);
   	lv_obj_set_size(phandle->color_temp_btn, 712, 90);
	lv_obj_add_event_cb(phandle->color_temp_btn, divoom_lvgl_setting_color_temp_event_cb, LV_EVENT_KEY, phandle);

	lv_obj_t *slide_obj = lv_slider_create(phandle->main_obj);
    lv_slider_set_range(slide_obj, -256, 256);
    lv_obj_set_pos(slide_obj, 135, 258-4);
    lv_obj_set_size(slide_obj, 542, 12);
	divoom_menu_control_get_img_path("26/26_color_temp.png", phandle->file_name);
	lv_obj_set_style_bg_img_src(slide_obj, phandle->file_name, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(slide_obj, 0, LV_PART_INDICATOR);
	lv_obj_set_style_pad_all(slide_obj, 8, LV_PART_KNOB);
	lv_obj_set_style_bg_color(slide_obj, lv_color_hex(0xA9A9A9), LV_PART_KNOB);
    lv_slider_set_value(slide_obj, phandle->psys_config->color_temp, LV_ANIM_ON);
    lv_obj_set_style_bg_color(slide_obj, lv_color_hex(0x707070), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slide_obj, lv_color_hex(0xB1B1B1), LV_PART_MAIN);
    phandle->color_temp_slider = slide_obj;

	return phandle;
}


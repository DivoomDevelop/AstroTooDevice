#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"

typedef struct _divoom_lvgl_audio_source_menu_ctrl
{
    int menu_id;
	lv_style_t backgroud_type;
	void *main_obj;
	void *pdirect_handle;

	lv_style_t normal_type;
	lv_style_t press_type;

	void *cur_active_obj;

	void *help_btn;
	void *bg_cover_obj;
	void *help_img_obj;
	void *help_correct_btn;

	int remind_flag;
	void *remind_obj;
	void *remind_img;
	void *remind_label;
	int remind_time;
	lv_timer_t*remind_timer;
}divoom_lvgl_audio_source_menu_ctrl, *Pdivoom_lvgl_audio_source_menu_ctrl;

char *divoom_lvgl_audio_source_help_img_str[] = {
	"91/91_help_English.png",
	"91/91_help_SC.png",
	"91/91_help_TC.png",
	"91/91_help_JP.png",
	"91/91_help_Germany.png",
	"91/91_help_Thai.png",
	"91/91_help_Russian.png",
	"91/91_help_Korean.png",
	"91/91_help_Vietnam.png",
	"91/91_help_French.png",
	"91/91_help_TR.png",
	"91/91_help_Spanish.png",
	"91/91_help_Portuguese.png",
	"91/91_help_IN.png",
	"91/91_help_Italy.png",
	"91/91_help_Ukraine.png",
	"91/91_help_SV.png",
	"91/91_help_DA.png",
	"91/91_help_Poland.png",
};

void divoom_lvgl_audio_source_menu_create_remind_box(Pdivoom_lvgl_audio_source_menu_ctrl phandle, int remind_flag)
{
	phandle->remind_obj = lv_obj_create(phandle->main_obj);
	
	lv_obj_set_pos(phandle->remind_obj, 268-4-25, 44-4);
	lv_obj_set_size(phandle->remind_obj, 264+8+50, 88+8);
	lv_obj_set_style_border_width(phandle->remind_obj, 4, 0);
	lv_obj_set_style_pad_all(phandle->remind_obj, 0, 0);
	lv_obj_set_style_border_color(phandle->remind_obj, lv_color_hex(0x0), 0);
	lv_obj_set_style_border_opa(phandle->remind_obj, 12*LV_OPA_100/100, 0);
	lv_obj_set_style_bg_color(phandle->remind_obj, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_bg_opa(phandle->remind_obj, 85*LV_OPA_100/100, 0);
	lv_obj_set_style_radius(phandle->remind_obj, 48, 0);
	lv_obj_clear_flag(phandle->remind_obj, LV_OBJ_FLAG_SCROLLABLE);

	/*lv_obj_t *circle_obj = lv_obj_create(phandle->remind_obj);
	
	lv_obj_set_pos(circle_obj, 21-4, 21-4);
	lv_obj_set_size(circle_obj, 54, 54);
	lv_obj_set_style_border_width(circle_obj, 0, 0);
	lv_obj_set_style_pad_all(circle_obj, 0, 0);

	lv_obj_set_style_bg_color(circle_obj, lv_color_hex(0x555555), 0);
	lv_obj_set_style_bg_opa(circle_obj, LV_OPA_100, 0);
	lv_obj_set_style_radius(circle_obj, LV_RADIUS_CIRCLE, 0);
	*/
	lv_obj_t *img_obj = lv_img_create(phandle->remind_obj);
	lv_obj_set_pos(img_obj, 21-4, 21-4);
	
	if(remind_flag == 0)
	{
		divoom_menu_control_lv_img_set_src(img_obj, "91/91_tf_insert.png");
	}
	else if(remind_flag == 1)
	{
		divoom_menu_control_lv_img_set_src(img_obj, "91/91_tf_read_fail.png");
	}
	else if(remind_flag == 2)
	{
		divoom_menu_control_lv_img_set_src(img_obj, "91/91_aux_insert.png");
	}
	else if(remind_flag == 3)
	{
		divoom_menu_control_lv_img_set_src(img_obj, "91/91_uac_insert.png");
	}
	phandle->remind_img = img_obj;

	lv_obj_t *tool_label = lv_label_create(phandle->remind_obj);
	lv_obj_set_pos(tool_label, 71, 8+25);
    lv_obj_set_size(tool_label, 176+50, 72);
	lv_obj_set_style_text_font(tool_label, divoom_menu_control_get_font_info(20, phandle->menu_id), 0);
	lv_obj_set_style_text_color(tool_label, lv_color_hex(0x0), 0);
	lv_obj_set_style_text_align(tool_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_long_mode(tool_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
	
	if(remind_flag == 0)
	{
		lv_label_set_text(tool_label, divoom_menu_control_get_disp_str("TFPlsInsert"));
	}
	else if(remind_flag == 1)
	{
		lv_label_set_text(tool_label, divoom_menu_control_get_disp_str("TFReadFail"));
	}
	else if(remind_flag == 2)
	{
		lv_label_set_text(tool_label, divoom_menu_control_get_disp_str("AUXPlsInsert"));
	}
	else if(remind_flag == 3)
	{
		lv_label_set_text(tool_label, divoom_menu_control_get_disp_str("UACPlsInsert"));
	}
	phandle->remind_label = tool_label;
}

void divoom_lvgl_audio_source_menu_update_remind_box(Pdivoom_lvgl_audio_source_menu_ctrl phandle, int remind_flag)
{
	if(remind_flag == 0)
	{
		divoom_menu_control_lv_img_set_src(phandle->remind_img, "91/91_tf_insert.png");
	}
	else if(remind_flag == 1)
	{
		divoom_menu_control_lv_img_set_src(phandle->remind_img, "91/91_tf_read_fail.png");
	}
	else if(remind_flag == 2)
	{
		divoom_menu_control_lv_img_set_src(phandle->remind_img, "91/91_aux_insert.png");
	}
	else if(remind_flag == 3)
	{
		divoom_menu_control_lv_img_set_src(phandle->remind_img, "91/91_uac_insert.png");
	}

	if(remind_flag == 0)
	{
		lv_label_set_text(phandle->remind_label, divoom_menu_control_get_disp_str("TFPlsInsert"));
	}
	else if(remind_flag == 1)
	{
		lv_label_set_text(phandle->remind_label, divoom_menu_control_get_disp_str("TFReadFail"));
	}
	else if(remind_flag == 2)
	{
		lv_label_set_text(phandle->remind_label, divoom_menu_control_get_disp_str("AUXPlsInsert"));
	}
	else if(remind_flag == 3)
	{
		lv_label_set_text(phandle->remind_label, divoom_menu_control_get_disp_str("UACPlsInsert"));
	}
}

void divoom_lvgl_audio_source_menu_remind_box_time_handle(lv_timer_t *exit_timer)
{
	Pdivoom_lvgl_audio_source_menu_ctrl phandle = (Pdivoom_lvgl_audio_source_menu_ctrl)lv_timer_get_user_data(exit_timer);
	
	if(phandle == NULL)
	{
		return;
	}

	phandle->remind_time++;

	if(phandle->remind_time == 2)
	{
		divoom_lvgl_audio_source_menu_update_remind_box(phandle, phandle->remind_flag);
		lv_obj_clear_flag(phandle->remind_obj, LV_OBJ_FLAG_HIDDEN);
	}
	else if(phandle->remind_time >= 6)
	{
		if(phandle->remind_timer)
		{
			lv_timer_pause(phandle->remind_timer);
		}
		if(phandle->remind_obj)
		{
			lv_obj_add_flag(phandle->remind_obj, LV_OBJ_FLAG_HIDDEN);
		}
	}
}


void divoom_lvgl_audio_source_menu_process_remind_box(Pdivoom_lvgl_audio_source_menu_ctrl phandle, int remind_flag)
{
	phandle->remind_flag = remind_flag;

	if(phandle->remind_obj == NULL)
	{
		divoom_lvgl_audio_source_menu_create_remind_box(phandle, phandle->remind_flag);
	}

	lv_obj_add_flag(phandle->remind_obj, LV_OBJ_FLAG_HIDDEN);

	phandle->remind_time = 0;

	if(phandle->remind_timer == NULL)
	{
		phandle->remind_timer = lv_timer_create(divoom_lvgl_audio_source_menu_remind_box_time_handle, 1000, phandle);
	}
	else
	{
		lv_timer_resume(phandle->remind_timer);
	}
}

void divoom_lvgl_audio_source_menu_set_event_cb(lv_event_t *e)
{
	int i, active_id, lang_flag;
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *cur_btn_obj = lv_event_get_target(e);
	Pdivoom_lvgl_audio_source_menu_ctrl phandle = (Pdivoom_lvgl_audio_source_menu_ctrl)lv_event_get_user_data(e);
		
	if(code == LV_EVENT_KEY)
	{
		char c = *((char *)lv_event_get_param(e));
		
		if(c == DIVOOM_LVGL_KEY_PRE)
		{
			cur_btn_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_btn_obj, DIVOOM_MENU_DIRECT_DOWN);
			lv_group_focus_obj(cur_btn_obj);
		}
		else if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			cur_btn_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_btn_obj, DIVOOM_MENU_DIRECT_UP);
			lv_group_focus_obj(cur_btn_obj);
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			active_id = divoom_menu_direct_ctrl_get_pos(phandle->pdirect_handle, cur_btn_obj);
			if(active_id == 0)
			{
				divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_BLUETOOTH, NULL, 0, 0);
			}
			else if(active_id == 1)
			{
#if DIVOOM_SHOW_TF_NAND
				divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_LOCAL_MUSIC_MENU, NULL, 0, 0);
#else
				if(divoom_main_get_audio_type_statu(2))
				{
					divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_LOCAL_MUSIC_MENU, NULL, 0, 0);
				}
				else
				{
					divoom_lvgl_audio_source_menu_process_remind_box(phandle, 0);
				}
#endif
			}
			else if(active_id == 2)
			{
				if(divoom_main_get_audio_type_statu(1))
				{
					divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_AUX_MUSIC_MENU, NULL, 0, 0);
				}
				else
				{
					divoom_lvgl_audio_source_menu_process_remind_box(phandle, 2);
				}
			}
			else if(active_id == 3)
			{
				if(divoom_main_get_audio_type_statu(0))
				{
					divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_UAC_MUSIC_MENU, NULL, 0, 0);
				}
				else
				{
					divoom_lvgl_audio_source_menu_process_remind_box(phandle, 3);
				}		
			}
			else if(active_id == 4) //使用说明
			{
				lang_flag = divoom_main_set_lang_flag(-1);

				if(lang_flag < sizeof(divoom_lvgl_audio_source_help_img_str)/sizeof(divoom_lvgl_audio_source_help_img_str[0]))
				{
					divoom_menu_control_lv_img_set_src(phandle->help_img_obj, divoom_lvgl_audio_source_help_img_str[lang_flag]);
				}
				else
				{
					divoom_menu_control_lv_img_set_src(phandle->help_img_obj, divoom_lvgl_audio_source_help_img_str[0]);
				}
				lv_obj_clear_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);
				lv_group_focus_obj(phandle->help_correct_btn);
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_menu_control_return_pre();
		}
		else
		{
			char *key_info = (char *)lv_event_get_param(e);
			divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
		}
	}
}

void divoom_lvgl_audio_source_menu_help_event_cb(lv_event_t *e)
{
	int i, active_id;
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *cur_btn_obj = lv_event_get_target(e);
	Pdivoom_lvgl_audio_source_menu_ctrl phandle = (Pdivoom_lvgl_audio_source_menu_ctrl)lv_event_get_user_data(e);
		
	if(code == LV_EVENT_KEY)
	{
		char c = *((char *)lv_event_get_param(e));
		
		if(c == DIVOOM_LVGL_KEY_PRE)
		{

		}
		else if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			lv_obj_add_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);
			lv_group_focus_obj(phandle->help_btn);
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			lv_obj_add_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);
			lv_group_focus_obj(phandle->help_btn);
		}
		else
		{

		}
	}
}


void *divoom_lvgl_audio_source_menu_init(void *user_data, int menu_id)
{
	int i;
	int lang_flag = divoom_main_set_lang_flag(-1);
	Pdivoom_lvgl_audio_source_menu_ctrl phandle = (Pdivoom_lvgl_audio_source_menu_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_audio_source_menu_ctrl));

	phandle->pdirect_handle = divoom_menu_direct_ctrl_create(1, 5);

	phandle->cur_active_obj = NULL;
	phandle->main_obj = lv_obj_create(NULL);
	phandle->menu_id = menu_id;
	lv_style_init(&phandle->backgroud_type);
	lv_style_set_bg_color(&phandle->backgroud_type, lv_color_hex(0xE4E4E4));
	lv_style_set_text_color(&phandle->backgroud_type, lv_color_hex(0x0));
	lv_obj_add_style(phandle->main_obj, &phandle->backgroud_type, 0);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(phandle->main_obj, 0, 0);

	lv_obj_t *tool_label = lv_label_create(phandle->main_obj);
	lv_obj_set_pos(tool_label, 367-100, 61);
    lv_obj_set_size(tool_label, 300, 56);
	lv_obj_set_style_text_font(tool_label, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_obj_set_style_text_color(tool_label, lv_color_hex(0x0), 0);
	lv_obj_set_style_text_align(tool_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_text(tool_label, divoom_menu_control_get_disp_str("AudioSource"));

	//divoom_menu_control_init_btn_stype(&phandle->press_type, NULL, 
    //                                        &phandle->normal_type, NULL, 
    //                                        NULL, NULL, 17);
    
	lv_style_init(&phandle->normal_type);
	lv_style_set_bg_color(&phandle->normal_type, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->normal_type, 0);
	lv_style_set_border_width(&phandle->normal_type, 4);
	lv_style_set_border_color(&phandle->normal_type, lv_color_hex(0xE4E4E4));
	lv_style_set_radius(&phandle->normal_type, 17);
	lv_style_set_shadow_width(&phandle->normal_type, 0);
	lv_style_set_outline_width(&phandle->normal_type, 0);

	lv_style_init(&phandle->press_type);
	lv_style_set_bg_color(&phandle->press_type, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->press_type, 0);
	lv_style_set_border_width(&phandle->press_type, 4);
	lv_style_set_border_color(&phandle->press_type, lv_color_hex(0x6D6D6D));
	lv_style_set_radius(&phandle->press_type, 17);
	lv_style_set_shadow_width(&phandle->press_type, 0);
	lv_style_set_outline_width(&phandle->press_type, 0);

	for(i = 0;i < 4; i++)
	{
		/*lv_obj_t *obj = lv_obj_create(phandle->main_obj);
		lv_obj_add_style(obj, &phandle->obj_style, 0);
		lv_obj_set_size(obj, 720, 138);
		lv_obj_set_pos(obj, 40, 160+138*i);
		lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);*/
		
		lv_obj_t *btn = lv_btn_create(phandle->main_obj);
		lv_obj_set_size(btn, 712, 90);
		lv_obj_set_pos(btn, 44, 164+98*i);
		lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
		
		lv_obj_add_style(btn, &phandle->normal_type, 0);
		
		lv_obj_add_style(btn, &phandle->press_type, LV_STATE_FOCUSED);
		lv_obj_add_style(btn, &phandle->press_type, LV_STATE_FOCUS_KEY);
		lv_obj_set_style_pad_all(btn, 0, 0);
		//lv_obj_set_style_bg_color(btn, lv_color_hex(0x181818), 0);
		lv_obj_add_event_cb(btn, divoom_lvgl_audio_source_menu_set_event_cb, LV_EVENT_KEY, phandle);
			
		divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn, 0, i);

		lv_obj_t *img_obj = lv_img_create(btn);
		lv_obj_set_pos(img_obj, 16, 29);

		lv_obj_t *time_label = lv_label_create(btn);
		lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
		lv_obj_set_style_text_color(time_label, lv_color_black(), 0);
		lv_obj_set_size(time_label, 400, 34);
		lv_obj_set_pos(time_label, 60, 28);

		if(i == 0)
		{
			phandle->cur_active_obj = btn;
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Bluetooth"));
			divoom_menu_control_lv_img_set_src(img_obj, "91/91_bluetooth.png");
		}
		else if(i == 1)
		{
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("TFCard"));
			divoom_menu_control_lv_img_set_src(img_obj, "91/91_tfcard.png");
		}
		else if(i == 2)
		{
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("AUX"));
			divoom_menu_control_lv_img_set_src(img_obj, "91/91_aux.png");
		}
		else if(i == 3)
		{
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("UAC"));
			divoom_menu_control_lv_img_set_src(img_obj, "91/91_uac.png");
		}

		lv_obj_t *right_img = lv_img_create(btn);
		divoom_menu_control_lv_img_set_src(right_img, "49/49_right.png");
		lv_obj_set_pos(right_img, 644, 27);

	}
	
	lv_obj_t *btn = lv_btn_create(phandle->main_obj);
	lv_obj_set_size(btn, 712, 90);
	lv_obj_set_pos(btn, 46, 596);
	lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_style(btn, &phandle->normal_type, 0);
	lv_obj_add_style(btn, &phandle->press_type, LV_STATE_FOCUSED);
	lv_obj_add_style(btn, &phandle->press_type, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn, 0, 0);
	phandle->help_btn = btn;

	lv_obj_add_event_cb(btn, divoom_lvgl_audio_source_menu_set_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn, 0, 4);

	lv_obj_t *img_obj2 = lv_img_create(btn);
	lv_obj_set_pos(img_obj2, 16, 29);
	divoom_menu_control_lv_img_set_src(img_obj2, "91/91_help.png");

	lv_obj_t *time_label2 = lv_label_create(btn);
	lv_obj_set_style_text_font(time_label2, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
	lv_obj_set_style_text_color(time_label2, lv_color_black(), 0);
	lv_obj_set_size(time_label2, 400, 34);
	lv_obj_set_pos(time_label2, 60, 28-4);
	lv_label_set_text(time_label2, divoom_menu_control_get_disp_str("Help"));

	lv_obj_t *right_img = lv_img_create(btn);
	divoom_menu_control_lv_img_set_src(right_img, "49/49_right.png");
	lv_obj_set_pos(right_img, 644, 27);

	lv_obj_t *help_label5 = lv_label_create(phandle->main_obj);
	lv_obj_set_style_text_font(help_label5, divoom_menu_control_get_font_info(20, phandle->menu_id), 0);
	lv_obj_set_style_text_color(help_label5, lv_color_hex(0x666666), 0);
	lv_obj_set_style_text_align(help_label5, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_size(help_label5, 720, 60);
	lv_obj_set_pos(help_label5, 40, 1140);
	lv_label_set_text(help_label5, divoom_menu_control_get_disp_str("AudioDesp"));

	//使用帮助
	phandle->bg_cover_obj = lv_obj_create(phandle->main_obj);
	lv_obj_set_style_bg_color(phandle->bg_cover_obj, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_bg_opa(phandle->bg_cover_obj, 85*LV_OPA_100/100, 0);
	lv_obj_set_style_pad_all(phandle->bg_cover_obj, 0, 0);
	lv_obj_set_style_border_width(phandle->bg_cover_obj, 0, 0);
	lv_obj_set_style_outline_width(phandle->bg_cover_obj, 0, 0);
	lv_obj_set_size(phandle->bg_cover_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(phandle->bg_cover_obj, 0, 0);
	lv_obj_add_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);
	
	//使用帮助弹窗
	lv_obj_t *help_obj = lv_obj_create(phandle->bg_cover_obj);
	lv_obj_set_style_bg_color(help_obj, lv_color_hex(0xF2F2F2), 0);
	lv_obj_set_style_pad_all(help_obj, 0, 0);
	lv_obj_set_style_border_width(help_obj, 0, 0);
	lv_obj_set_style_outline_width(help_obj, 0, 0);
	lv_obj_set_style_radius(help_obj, 36, 0);
	lv_obj_set_size(help_obj, 640, 1024);
	lv_obj_set_pos(help_obj, 80, 128);

	lv_obj_t *help_label2 = lv_label_create(help_obj);
	lv_obj_set_style_text_font(help_label2, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
	lv_obj_set_style_text_color(help_label2, lv_color_black(), 0);
	lv_obj_set_style_text_align(help_label2, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_size(help_label2, 600, 40);
	lv_obj_set_pos(help_label2, 20, 20);
	lv_label_set_text(help_label2, divoom_menu_control_get_disp_str("Help"));

	/*lv_obj_t *help_label3 = lv_label_create(help_obj);
	lv_obj_set_style_text_font(help_label3, divoom_menu_control_get_font_info(18, phandle->menu_id), 0);
	lv_obj_set_style_text_color(help_label3, lv_color_black(), 0);
	lv_obj_set_size(help_label3, 600, 806);
	lv_obj_set_pos(help_label3, 20, 80);
	lv_label_set_text(help_label3, divoom_menu_control_get_disp_str("HelpDesp"));
	*/
	lv_obj_t *img_lang_obj = lv_img_create(help_obj);
	lv_obj_set_pos(img_lang_obj, 20, 80);
	
	if(lang_flag < sizeof(divoom_lvgl_audio_source_help_img_str)/sizeof(divoom_lvgl_audio_source_help_img_str[0]))
	{
		divoom_menu_control_lv_img_set_src(img_lang_obj, divoom_lvgl_audio_source_help_img_str[lang_flag]);
	}
	else
	{
		divoom_menu_control_lv_img_set_src(img_lang_obj, divoom_lvgl_audio_source_help_img_str[0]);
	}
	phandle->help_img_obj = img_lang_obj;

	lv_obj_t *btn2 = lv_btn_create(help_obj);
	lv_obj_set_size(btn2, 282, 90);
	lv_obj_set_pos(btn2, 179, 910);
	lv_obj_clear_flag(btn2, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(btn2, lv_color_hex(0xF2F2F2), 0);
	lv_obj_set_style_pad_all(btn2, 0, 0);
	lv_obj_set_style_radius(btn2, 99, 0);
	lv_obj_set_style_border_width(btn2, 4, 0);
	lv_obj_set_style_border_color(btn2, lv_color_hex(0x0), 0);
	lv_obj_set_style_outline_width(btn2, 0, 0);
	lv_obj_set_style_outline_width(btn2, 0, LV_STATE_FOCUSED);
	lv_obj_set_style_outline_width(btn2, 0, LV_STATE_FOCUS_KEY);
	
	lv_obj_add_event_cb(btn2, divoom_lvgl_audio_source_menu_help_event_cb, LV_EVENT_KEY, phandle);
	phandle->help_correct_btn = btn2;

	lv_obj_t *help_label4 = lv_label_create(btn2);
	lv_obj_set_style_text_font(help_label4, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_obj_set_style_text_color(help_label4, lv_color_black(), 0);
	lv_obj_set_style_text_align(help_label4, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_size(help_label4, 282, 44);
	lv_obj_set_pos(help_label4, 0, 23);
	lv_label_set_text(help_label4, divoom_menu_control_get_disp_str("Ok"));


	return phandle;
}

void divoom_lvgl_audio_source_menu_disp(void *handle, void *active)
{
	Pdivoom_lvgl_audio_source_menu_ctrl phandle = (Pdivoom_lvgl_audio_source_menu_ctrl)handle;

	if(phandle->cur_active_obj)
	{
		lv_group_focus_obj(phandle->cur_active_obj);
		phandle->cur_active_obj = NULL;
	}
	if(active)
	{
		lv_group_focus_obj(active);
	}
	lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_audio_source_menu_exit(void *handle)
{
	Pdivoom_lvgl_audio_source_menu_ctrl phandle = (Pdivoom_lvgl_audio_source_menu_ctrl)handle;
	
	lv_style_reset(&phandle->backgroud_type);
	lv_style_reset(&phandle->normal_type);
	lv_style_reset(&phandle->press_type);

	if(phandle->remind_timer)
	{
		lv_timer_pause(phandle->remind_timer);
		lv_timer_del(phandle->remind_timer);
		phandle->remind_timer = NULL;
	}

	divoom_menu_direct_ctrl_destory(phandle->pdirect_handle);
	lv_obj_del(phandle->main_obj);
	divoom_system_free(phandle);
	phandle = NULL;
}


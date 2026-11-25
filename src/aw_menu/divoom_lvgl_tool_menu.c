#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"

#ifdef DIVOOM_TIME_FRAME_SPEAKER

typedef struct _divoom_lvgl_tool_menu_ctrl
{
    int menu_id;
	lv_style_t backgroud_type;
	void *main_obj;
	void *pdirect_handle;

	lv_style_t normal_type;
	lv_style_t press_type;

	void *cur_active_obj;
}divoom_lvgl_tool_menu_ctrl, *Pdivoom_lvgl_tool_menu_ctrl;

void divoom_lvgl_tool_menu_cancel_disp(void)
{
	divoom_sys_msg_info model_msg = {0};
	model_msg.src_id = DIVOOM_MODEL_APP_TOOL;
	model_msg.dest_id = DIVOOM_MODEL_APP_TOOL;
	model_msg.msg_id = DIVOOM_APP_LVGL_DISP_EXIT;
	model_msg.msg_type = 1;
	divoom_sys_model_crl_msg_push(&model_msg);
}

void divoom_lvgl_tool_menu_set_event_cb(lv_event_t *e)
{
	int i, active_id;
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *cur_btn_obj = lv_event_get_target(e);
	Pdivoom_lvgl_tool_menu_ctrl phandle = (Pdivoom_lvgl_tool_menu_ctrl)lv_event_get_user_data(e);
		
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
				divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_POMODORO_MENU, NULL, 0, 0);
			}
			if(active_id == 1)
			{
				divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_STOP_WATCH_MENU, NULL, 0, 0);
			}
			else if(active_id == 2)
			{
				divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_COUNT_DOWN_MENU, NULL, 0, 0);
			}
			else if(active_id == 3)
			{
				divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SCORE_BOARD_MENU, NULL, 0, 0);
			}
			else if(active_id == 4)
			{
				divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_NOISE_MEASURE_MENU, NULL, 0, 0);
			}
			else if(active_id == 5)
			{
				divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_VOICE_RECORD_TOOL_MENU, NULL, 0, 0);
			}

		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			uint8 event_id = 0, mode_id = 0;
			mode_id = divoom_app_schedule_get_active_model(&event_id, DIVOOM_APP_SCHEDULE_RESOURCCE_RGB);
			if (mode_id == DIVOOM_MODEL_APP_TOOL && event_id == DIVOOM_APP_TOOL_EVENT_PLAY)
				divoom_lvgl_tool_menu_cancel_disp();
			else
				divoom_menu_control_return_pre();
		}
		else
		{
			char *key_info = (char *)lv_event_get_param(e);
			divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
		}
	}
}

void *divoom_lvgl_tool_menu_init(void *user_data, int menu_id)
{
	int i;
	Pdivoom_lvgl_tool_menu_ctrl phandle = (Pdivoom_lvgl_tool_menu_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_tool_menu_ctrl));
	
	phandle->pdirect_handle = divoom_menu_direct_ctrl_create(1, 6);

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
	lv_obj_set_pos(tool_label, 367, 61);
    lv_obj_set_size(tool_label, 300, 56);
	lv_obj_set_style_text_font(tool_label, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_obj_set_style_text_color(tool_label, lv_color_hex(0x0), 0);
	lv_label_set_text(tool_label, divoom_menu_control_get_disp_str("Tool"));

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

	for(i = 0;i < 6; i++)
	{
		/*lv_obj_t *obj = lv_obj_create(phandle->main_obj);
		lv_obj_add_style(obj, &phandle->obj_style, 0);
		lv_obj_set_size(obj, 720, 138);
		lv_obj_set_pos(obj, 40, 160+138*i);
		lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);*/
		
		lv_obj_t *btn  = lv_btn_create(phandle->main_obj);
		lv_obj_set_size(btn, 712, 90);
		lv_obj_set_pos(btn, 44, 164+98*i);
		lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
		
		lv_obj_add_style(btn, &phandle->normal_type, 0);
		
		lv_obj_add_style(btn, &phandle->press_type, LV_STATE_FOCUSED);
		lv_obj_add_style(btn, &phandle->press_type, LV_STATE_FOCUS_KEY);
		lv_obj_set_style_pad_all(btn, 0, 0);
		//lv_obj_set_style_bg_color(btn, lv_color_hex(0x181818), 0);
		lv_obj_add_event_cb(btn, divoom_lvgl_tool_menu_set_event_cb, LV_EVENT_KEY, phandle);
			
		divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn, 0, i);

		lv_obj_t *time_label = lv_label_create(btn);
		lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
		lv_obj_set_style_text_color(time_label, lv_color_black(), 0);
		lv_obj_set_size(time_label, 400, 34);
		lv_obj_set_pos(time_label, 32, 28);

		if(i == 0)
		{
			phandle->cur_active_obj = btn;
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Pomodoro"));
		}
		if(i == 1)
		{
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("StopWatch"));
		}
		else if(i == 2)
		{
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Countdown"));
		}
		else if(i == 3)
		{
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Scoreboard"));
		}
		else if(i == 4)
		{
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Noisemeter"));
		}
		else if(i == 5)
		{
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("VoiceRecordTool"));
		}

		lv_obj_t *right_img = lv_img_create(btn);
		divoom_menu_control_lv_img_set_src(right_img, "49/49_right.png");
		lv_obj_set_pos(right_img, 644, 27);

	}
	
	return phandle;
}

void divoom_lvgl_tool_menu_disp(void *handle, void *active)
{
	Pdivoom_lvgl_tool_menu_ctrl phandle = (Pdivoom_lvgl_tool_menu_ctrl)handle;

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

void divoom_lvgl_tool_menu_exit(void *handle)
{
	Pdivoom_lvgl_tool_menu_ctrl phandle = (Pdivoom_lvgl_tool_menu_ctrl)handle;
	
	lv_style_reset(&phandle->backgroud_type);
	lv_style_reset(&phandle->normal_type);
	lv_style_reset(&phandle->press_type);
	divoom_menu_direct_ctrl_destory(phandle->pdirect_handle);
	lv_obj_del(phandle->main_obj);
	divoom_system_free(phandle);
}


#else
typedef struct _divoom_lvgl_tool_menu_ctrl
{
    int menu_id;
	lv_style_t backgroud_type;
	void *main_obj;
	void *pdirect_handle;

	lv_style_t normal_type;
	lv_style_t press_type;

	void *cur_active_obj;
}divoom_lvgl_tool_menu_ctrl, *Pdivoom_lvgl_tool_menu_ctrl;

void divoom_lvgl_tool_menu_cancel_disp(void)
{
	divoom_sys_msg_info model_msg = {0};
	model_msg.src_id = DIVOOM_MODEL_APP_TOOL;
	model_msg.dest_id = DIVOOM_MODEL_APP_TOOL;
	model_msg.msg_id = DIVOOM_APP_LVGL_DISP_EXIT;
	model_msg.msg_type = 1;
	divoom_sys_model_crl_msg_push(&model_msg);
}

void divoom_lvgl_tool_menu_set_event_cb(lv_event_t *e)
{
	int i, active_id;
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *cur_btn_obj = lv_event_get_target(e);
	Pdivoom_lvgl_tool_menu_ctrl phandle = (Pdivoom_lvgl_tool_menu_ctrl)lv_event_get_user_data(e);
		
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
				divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_STOP_WATCH_MENU, NULL, 0, 0);
			}
			else if(active_id == 1)
			{
				divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_COUNT_DOWN_MENU, NULL, 0, 0);
			}
			else if(active_id == 2)
			{
				divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SCORE_BOARD_MENU, NULL, 0, 0);
			}
			else if(active_id == 3)
			{
				divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_NOISE_MEASURE_MENU, NULL, 0, 0);
			}
			else if(active_id == 4)
			{
				divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_GOOGLE_CALENDAR_MENU, cur_btn_obj, 0, 0);
			}
			else if(active_id == 5)
			{
				divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_OUTLOOK_CALENDAR_MENU, NULL, 0, 0);
			}
			else if(active_id == 6)
			{
				divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_VOICE_RECORD_TOOL_MENU, NULL, 0, 0);
			}

		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			uint8 event_id = 0, mode_id = 0;
			mode_id = divoom_app_schedule_get_active_model(&event_id, DIVOOM_APP_SCHEDULE_RESOURCCE_RGB);
			if (mode_id == DIVOOM_MODEL_APP_TOOL && event_id == DIVOOM_APP_TOOL_EVENT_PLAY)
				divoom_lvgl_tool_menu_cancel_disp();
			else
				divoom_menu_control_return_pre();
		}
		else
		{
			char *key_info = (char *)lv_event_get_param(e);
			divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
		}
	}
}

void *divoom_lvgl_tool_menu_init(void *user_data, int menu_id)
{
	int i;
	Pdivoom_lvgl_tool_menu_ctrl phandle = (Pdivoom_lvgl_tool_menu_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_tool_menu_ctrl));
	
	phandle->pdirect_handle = divoom_menu_direct_ctrl_create(1, 7);

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
	lv_obj_set_pos(tool_label, 367, 61);
    lv_obj_set_size(tool_label, 300, 56);
	lv_obj_set_style_text_font(tool_label, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_obj_set_style_text_color(tool_label, lv_color_hex(0x0), 0);
	lv_label_set_text(tool_label, divoom_menu_control_get_disp_str("Tool"));

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

	for(i = 0;i < 7; i++)
	{
		/*lv_obj_t *obj = lv_obj_create(phandle->main_obj);
		lv_obj_add_style(obj, &phandle->obj_style, 0);
		lv_obj_set_size(obj, 720, 138);
		lv_obj_set_pos(obj, 40, 160+138*i);
		lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);*/
		
		lv_obj_t *btn  = lv_btn_create(phandle->main_obj);
		lv_obj_set_size(btn, 712, 90);
		lv_obj_set_pos(btn, 44, 164+98*i);
		lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
		
		lv_obj_add_style(btn, &phandle->normal_type, 0);
		
		lv_obj_add_style(btn, &phandle->press_type, LV_STATE_FOCUSED);
		lv_obj_add_style(btn, &phandle->press_type, LV_STATE_FOCUS_KEY);
		lv_obj_set_style_pad_all(btn, 0, 0);
		//lv_obj_set_style_bg_color(btn, lv_color_hex(0x181818), 0);
		lv_obj_add_event_cb(btn, divoom_lvgl_tool_menu_set_event_cb, LV_EVENT_KEY, phandle);
			
		divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn, 0, i);

		lv_obj_t *time_label = lv_label_create(btn);
		lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
		lv_obj_set_style_text_color(time_label, lv_color_black(), 0);
		lv_obj_set_size(time_label, 400, 34);
		lv_obj_set_pos(time_label, 32, 28);

		if(i == 0)
		{
			phandle->cur_active_obj = btn;
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("StopWatch"));
		}
		else if(i == 1)
		{
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Countdown"));
		}
		else if(i == 2)
		{
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Scoreboard"));
		}
		else if(i == 3)
		{
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Noisemeter"));
		}
		else if(i == 4)
		{
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("GoogleCalendar"));
		}
		else if(i == 5)
		{
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("OutLookCalendar"));
		}
		else if(i == 6)
		{
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("VoiceRecordTool"));
		}

		lv_obj_t *right_img = lv_img_create(btn);
		divoom_menu_control_lv_img_set_src(right_img, "49/49_right.png");
		lv_obj_set_pos(right_img, 644, 27);

	}
	
	return phandle;
}

void divoom_lvgl_tool_menu_disp(void *handle, void *active)
{
	Pdivoom_lvgl_tool_menu_ctrl phandle = (Pdivoom_lvgl_tool_menu_ctrl)handle;

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

void divoom_lvgl_tool_menu_exit(void *handle)
{
	Pdivoom_lvgl_tool_menu_ctrl phandle = (Pdivoom_lvgl_tool_menu_ctrl)handle;
	
	lv_style_reset(&phandle->backgroud_type);
	lv_style_reset(&phandle->normal_type);
	lv_style_reset(&phandle->press_type);
	divoom_menu_direct_ctrl_destory(phandle->pdirect_handle);
	lv_obj_del(phandle->main_obj);
	divoom_system_free(phandle);
}
#endif
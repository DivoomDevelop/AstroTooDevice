#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_stream_cont.h"

typedef struct _divoom_lvgl_count_down_menu_ctrl
{
    int menu_id;
	lv_style_t backgroud_type;
	void *main_obj;

	lv_style_t btn_normal_type;
	lv_style_t btn_press_type;

	void *time_mark_obj[3];

	void *stream_cont_hour;
	void *stream_cont_minute;
	void *stream_cont_second;
	int select_id;
	void *btn_list[5];
	void *pdirect_handle;
	void *first_active_obj;

	int btn_status;
	
	int start_flag;
	void *start_btn;
	void *back_btn;
	void *music_btn;

	int   music_index;
	void *music_label_obj;
	void *bells_obj;	//选择铃声页面
	void *bell_cont_obj[12];
	void *cur_active_obj;
	void *pdirect_music_handle;
	
	void *time_obj;
	int hour;
	int minute;
	lv_timer_t *menu_bar_timer;
}divoom_lvgl_count_down_menu_ctrl, *Pdivoom_lvgl_count_down_menu_ctrl;

static const char *divoom_lvgl_count_down_menu_img_list[] = {
    "29/29_bell_rain.png",
    "29/29_bell_wave.png",
    "29/29_bell_city.png",
    "29/29_bell_forest.png",
    "29/29_bell_dolphin.png",
    "29/29_bell_sun.png",
    "29/29_bell_bird.png",
    "29/29_bell_ring.png",
    "29/29_bell_clock.png",
    "29/29_bell_piano.png",
    "29/29_bell_clock2.png",
    "29/29_bell_music.png",
};

void divoom_lvgl_count_down_menu_start_time(void *handle)
{
	int hour, minute, second, total_sec = 0;
	Pdivoom_app_tool_count_time_down_info count_info;
	divoom_sys_msg_info msg_info = {0};
	Pdivoom_lvgl_count_down_menu_ctrl phandle = (Pdivoom_lvgl_count_down_menu_ctrl)handle;
	
	//发送开始
	count_info = divoom_system_malloc(sizeof(divoom_app_tool_count_time_down_info));
	hour = divoom_stream_cont_get_current_selected(phandle->stream_cont_hour);
	minute = divoom_stream_cont_get_current_selected(phandle->stream_cont_minute);
	second = divoom_stream_cont_get_current_selected(phandle->stream_cont_second);
	
	msg_info.dest_id = DIVOOM_MODEL_APP_TOOL;
	msg_info.msg_id = DIVOOM_APP_COUNT_TIME_DOWN_SET;	
	msg_info.msg_type = 1;

	count_info->hour = hour;
	count_info->minute = minute;
	count_info->second = second;
	count_info->status = 1;
	count_info->lvgl_flag = 1;
	
	msg_info.data = (uint8 *)count_info;
	divoom_sys_model_crl_msg_push(&msg_info);
}



void divoom_lvgl_count_down_menu_event_cb(lv_event_t *e)
{
	int i;
	Pdivoom_lvgl_count_down_menu_ctrl phandle = (Pdivoom_lvgl_count_down_menu_ctrl)lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *cur_btn_obj = lv_event_get_target(e);

	if(code == LV_EVENT_KEY)
	{
		char c = *((char*)lv_event_get_param(e));
		
		if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			if(phandle->btn_status == 0)
			{
				cur_btn_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_btn_obj, DIVOOM_MENU_DIRECT_LEFT);                            
                lv_group_focus_obj(cur_btn_obj);
				for(i = 0; i < 5; i++)
                {
                   if(cur_btn_obj == phandle->btn_list[i])
                   {
                      phandle->select_id = i;
                      break;
                   }
                }
				
				for(i = 0;i < 3;i++)
				{
					if(phandle->select_id < 3)
					{
						if(phandle->select_id == i)
						{
							lv_obj_clear_flag(phandle->time_mark_obj[i], LV_OBJ_FLAG_HIDDEN);
						}
						else
						{
							lv_obj_add_flag(phandle->time_mark_obj[i], LV_OBJ_FLAG_HIDDEN);
						}
					}
					else
					{
						lv_obj_add_flag(phandle->time_mark_obj[i], LV_OBJ_FLAG_HIDDEN);
					}
				}
				
			}
			else//处理时间选择
			{
				if(phandle->select_id == 0)
				{
					divoom_stream_cont_key_roll_down(phandle->stream_cont_hour);
				}
				else if(phandle->select_id == 1)
				{
					divoom_stream_cont_key_roll_down(phandle->stream_cont_minute);
				}
				else if(phandle->select_id == 2)
				{
					divoom_stream_cont_key_roll_down(phandle->stream_cont_second);
				}
			}
		}
		else if(c == DIVOOM_LVGL_KEY_PRE)
		{
			if(phandle->btn_status == 0)
			{
				cur_btn_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_btn_obj, DIVOOM_MENU_DIRECT_RIGHT);                            
                lv_group_focus_obj(cur_btn_obj);
				for(i = 0; i < 5; i++)
                {
                   if(cur_btn_obj == phandle->btn_list[i])
                   {
                      phandle->select_id = i;
                      break;
                   }
                }

				for(i = 0;i < 3;i++)
				{
					if(phandle->select_id < 3)
					{
						if(phandle->select_id == i)
						{
							lv_obj_clear_flag(phandle->time_mark_obj[i], LV_OBJ_FLAG_HIDDEN);
						}
						else
						{
							lv_obj_add_flag(phandle->time_mark_obj[i], LV_OBJ_FLAG_HIDDEN);
						}
					}
					else
					{
						lv_obj_add_flag(phandle->time_mark_obj[i], LV_OBJ_FLAG_HIDDEN);
					}
				}
			}
			else
			{
				if(phandle->select_id == 0)
				{
					divoom_stream_cont_key_roll_up(phandle->stream_cont_hour);
				}
				else if(phandle->select_id == 1)
				{
					divoom_stream_cont_key_roll_up(phandle->stream_cont_minute);
				}
				else if(phandle->select_id == 2)
				{
					divoom_stream_cont_key_roll_up(phandle->stream_cont_second);
				}
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			if(phandle->btn_status == 0)
			{
				divoom_menu_control_return_pre();
			}
			else
			{
				if(phandle->select_id < 3)
				{
					phandle->btn_status = 0;
					lv_obj_set_style_border_color(phandle->btn_list[phandle->select_id], lv_color_hex(0x0), LV_STATE_FOCUSED);
					lv_obj_set_style_border_color(phandle->btn_list[phandle->select_id], lv_color_hex(0x0), LV_STATE_FOCUS_KEY);
				}
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			if(phandle->btn_status == 0)
			{
				if(phandle->select_id < 3)
				{
					phandle->btn_status = 1;
					lv_obj_set_style_border_color(phandle->btn_list[phandle->select_id], lv_color_hex(0x00C700), LV_STATE_FOCUSED);
					lv_obj_set_style_border_color(phandle->btn_list[phandle->select_id], lv_color_hex(0x00C700), LV_STATE_FOCUS_KEY);
				}
				else if(phandle->select_id == 3)//开始计时
				{
					phandle->start_flag = 1;
					divoom_lvgl_count_down_menu_start_time(phandle);
					divoom_app_tool_set_count_down_music_index(phandle->music_index);
					divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_COUNT_DOWN_START_MENU, NULL, 0, 0);
				}
				else//选择铃声
				{
					lv_obj_clear_flag(phandle->bells_obj, LV_OBJ_FLAG_HIDDEN);
					lv_group_focus_obj(phandle->cur_active_obj);
				}
			}
			else
			{
				/*if(phandle->select_id == 0)//处理时间选择
				{
					divoom_stream_cont_key_roll_down(phandle->stream_cont_hour);
				}
				else if(phandle->select_id == 1)
				{
					divoom_stream_cont_key_roll_down(phandle->stream_cont_minute);
				}
				else if(phandle->select_id == 2)
				{
					divoom_stream_cont_key_roll_down(phandle->stream_cont_second);
				}*/
			}
		}
		else
		{
			char *key_info = (char *)lv_event_get_param(e);
			divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
			return;
		}
		
	}
}

void divoom_lvgl_count_down_menu_refresh_menu_bar(lv_timer_t *menu_bar_timer)
{
	int hour;
    Pdivoom_lvgl_count_down_menu_ctrl phandle = (Pdivoom_lvgl_count_down_menu_ctrl)lv_timer_get_user_data(menu_bar_timer);
	time_t time_now;
	struct tm p_now;
	
    time_now = time(NULL);
	localtime_r(&time_now, &p_now);
	hour = p_now.tm_hour;
    if(divoom_main_get_time_flag() == 0)
    {
        if(hour != 12)
        {
            hour = hour % 12;
            if(hour == 0)
            {
                hour = 12;
            }
        }
    }

    if(hour != phandle->hour || p_now.tm_min != phandle->minute)
    {
        phandle->hour = hour;
        phandle->minute = p_now.tm_min;
        lv_label_set_text_fmt(phandle->time_obj, "%02d: %02d", hour, phandle->minute);
    }
}

void divoom_lvgl_count_down_menu_create_title_area(Pdivoom_lvgl_count_down_menu_ctrl phandle)
{
	int hour;
	char disp_str[32];
	time_t time_now;
	struct tm p_now;
	
	lv_obj_t *title_obj = lv_obj_create(phandle->main_obj);
	lv_obj_set_style_border_width(title_obj, 0, 0);
	lv_obj_set_style_bg_color(title_obj, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_text_color(title_obj, lv_color_hex(0x0), 0);
	lv_obj_set_style_pad_all(title_obj, 0, 0);
	lv_obj_clear_flag(title_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(title_obj, 800, 78); 
	lv_obj_set_pos(title_obj, 0, 40);
	
	lv_obj_t *timer_label = lv_label_create(title_obj);
	lv_obj_set_style_text_font(timer_label, divoom_menu_control_get_font_info(48, phandle->menu_id), 0);
	lv_obj_set_size(timer_label, 350, 48);
	lv_obj_set_style_text_align(timer_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_text(timer_label, divoom_menu_control_get_disp_str("Countdown"));
	lv_obj_center(timer_label);

	lv_obj_t *time_label = lv_label_create(title_obj);
	lv_obj_set_size(time_label, 98, 28); 
	lv_obj_set_pos(time_label, 662, 25);
	lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
		
	time_now = time(NULL);
	localtime_r(&time_now, &p_now);
	hour = p_now.tm_hour;
	if(divoom_main_get_time_flag() == 0)
	{
		if(hour != 12)
		{
			hour = hour % 12;
			if(hour == 0)
			{
				hour = 12;
			}
		}
	}
	
	phandle->hour = hour;
	phandle->minute = p_now.tm_min;
	sprintf(disp_str, "%02d: %02d", hour, p_now.tm_min);
	lv_label_set_text(time_label, disp_str);
	phandle->time_obj = time_label;
	phandle->menu_bar_timer = lv_timer_create(divoom_lvgl_count_down_menu_refresh_menu_bar, 1000, phandle);
}

void divoom_lvgl_count_down_menu_bell_list_event_cb(lv_event_t * e)
{
	int i, tip_index;
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *cur_btn_obj = lv_event_get_target(e);
	Pdivoom_lvgl_count_down_menu_ctrl phandle = (Pdivoom_lvgl_count_down_menu_ctrl)lv_event_get_user_data(e);
	
	if(code == LV_EVENT_KEY)
	{
		char c = *((char *)lv_event_get_param(e));

		if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			lv_group_focus_obj(divoom_menu_direct_ctrl_get(phandle->pdirect_music_handle, cur_btn_obj, DIVOOM_MENU_DIRECT_LEFT));
		}
		else if(c == DIVOOM_LVGL_KEY_PRE)
		{
			lv_group_focus_obj(divoom_menu_direct_ctrl_get(phandle->pdirect_music_handle, cur_btn_obj, DIVOOM_MENU_DIRECT_RIGHT));
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_app_tool_count_down_menu_try_listen(0, 0, 0);
			lv_obj_add_flag(phandle->bells_obj, LV_OBJ_FLAG_HIDDEN);
			lv_label_set_text_fmt(phandle->music_label_obj, "Music %d", phandle->music_index + 1);
			lv_group_focus_obj(phandle->music_btn);
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			for(int i = 0; i < 12; i++)
			{
				if(cur_btn_obj == phandle->bell_cont_obj[i])
				{
					phandle->cur_active_obj = phandle->bell_cont_obj[i];
					phandle->music_index = i;
					tip_index = i;
					divoom_app_tool_count_down_menu_try_listen(tip_index, divoom_main_get_tone_volume(), 1);
					break;
				}
			}
		}
		else
		{
			return;
		}
	}
}

void divoom_lvgl_count_down_menu_create_bell_list(Pdivoom_lvgl_count_down_menu_ctrl phandle, void *main_obj)
{
	int i, j;

	lv_obj_t *bells_obj = lv_obj_create(main_obj);
	lv_obj_set_style_bg_color(bells_obj, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_border_width(bells_obj, 0, 0);
	lv_obj_set_style_pad_all(bells_obj, 0, 0);
	lv_obj_set_size(bells_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(bells_obj, 0, 0);
	lv_obj_add_flag(bells_obj, LV_OBJ_FLAG_HIDDEN);
	phandle->bells_obj = bells_obj;

	lv_obj_t *bell_label = lv_label_create(bells_obj);
	lv_obj_set_size(bell_label, 360, 56);
	lv_obj_set_pos(bell_label, 220, 52);
	lv_obj_set_style_text_color(bell_label, lv_color_hex(0x0), 0);
	lv_obj_set_style_text_font(bell_label, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_obj_set_style_text_align(bell_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_text(bell_label, divoom_menu_control_get_disp_str("Bells"));

	//lv_group_t *group = lv_group_create();
	for(i = 0; i < 3; i++)
	{
		for(j = 0; j < 4; j++)
		{
			lv_obj_t *bell_btn = lv_btn_create(phandle->bells_obj);
			lv_obj_remove_style_all(bell_btn);
			lv_obj_set_style_pad_all(bell_btn, 0, 0);
			lv_obj_set_style_radius(bell_btn, 23, 0);
			lv_obj_set_style_border_width(bell_btn, 4, 0);
			lv_obj_set_style_bg_color(bell_btn, lv_color_hex(0xE2E2E2), 0);
			lv_obj_set_style_bg_opa(bell_btn, 7*LV_OPA_100/100, 0);
			lv_obj_set_style_shadow_width(bell_btn, 0, 0);
			//lv_obj_set_style_outline_width(bell_btn, 0, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
			lv_obj_set_style_border_color(bell_btn, lv_color_hex(0xE2E2E2), 0);
			lv_obj_set_style_border_opa(bell_btn, 7*LV_OPA_100/100, 0);
			lv_obj_set_style_border_color(bell_btn, lv_color_hex(0xE89B00), LV_STATE_FOCUSED);
			lv_obj_set_style_border_opa(bell_btn, LV_OPA_100, LV_STATE_FOCUSED);
			lv_obj_set_style_border_color(bell_btn, lv_color_hex(0xE89B00), LV_STATE_FOCUS_KEY);
			lv_obj_set_style_border_opa(bell_btn, LV_OPA_100, LV_STATE_FOCUS_KEY);
			
			lv_obj_set_size(bell_btn, 150, 150);
			lv_obj_set_pos(bell_btn, 40 + 190*j, 143 + 190*i);

			lv_obj_t *bell_img = lv_img_create(bell_btn);
			divoom_menu_control_lv_img_set_src(bell_img, divoom_lvgl_count_down_menu_img_list[i*4+j]);
			lv_obj_center(bell_img);

			if(i == 0 && j == 0)
			{
				phandle->cur_active_obj = bell_btn;
			}
			phandle->bell_cont_obj[i*4+j] = bell_btn;
			//lv_group_add_obj(group, bell_btn);
			//lv_obj_add_event_cb(bell_btn, divoom_lvgl_count_down_menu_bell_list_event_cb, LV_EVENT_CLICKED, phandle);
			divoom_menu_direct_ctrl_update(phandle->pdirect_music_handle, bell_btn, j, i);
			lv_obj_add_event_cb(bell_btn, divoom_lvgl_count_down_menu_bell_list_event_cb, LV_EVENT_KEY, phandle);
		}
	}
	
}


void divoom_lvgl_count_down_menu_create_time_cont(Pdivoom_lvgl_count_down_menu_ctrl phandle)
{
	lv_obj_t *hour_obj = lv_obj_create(phandle->main_obj);
	lv_obj_set_style_bg_color(hour_obj, lv_color_hex(0x00C700), 0);
	lv_obj_set_style_radius(hour_obj, 15, 0);
	lv_obj_set_style_border_width(hour_obj, 0, 0);
	lv_obj_set_style_pad_all(hour_obj, 0, 0);
	lv_obj_clear_flag(hour_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_t *hour_label = lv_label_create(hour_obj);
	lv_obj_set_style_text_font(hour_label, divoom_menu_control_get_font_info(40, phandle->menu_id), 0);
	lv_obj_set_style_text_color(hour_label, lv_color_hex(0x004700), 0);
	lv_label_set_text(hour_label, divoom_menu_control_get_disp_str("h"));
	lv_obj_center(hour_label);
	lv_obj_set_size(hour_obj, 77, 71);
	lv_obj_set_pos(hour_obj, 159, 338);
	phandle->time_mark_obj[0] = hour_obj;

	lv_obj_t *minute_obj = lv_obj_create(phandle->main_obj);
	lv_obj_set_style_bg_color(minute_obj, lv_color_hex(0x00C700), 0);
	lv_obj_set_style_radius(minute_obj, 15, 0);
	lv_obj_set_style_border_width(minute_obj, 0, 0);
	lv_obj_set_style_pad_all(minute_obj, 0, 0);
	lv_obj_clear_flag(minute_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_t *minute_label = lv_label_create(minute_obj);
	lv_obj_set_style_text_font(minute_label, divoom_menu_control_get_font_info(40, phandle->menu_id), 0);
	lv_obj_set_style_text_color(minute_label, lv_color_hex(0x004700), 0);
	lv_label_set_text(minute_label, divoom_menu_control_get_disp_str("min"));
	lv_obj_center(minute_label);
	lv_obj_set_size(minute_obj, 77, 71);
	lv_obj_set_pos(minute_obj, 427, 338);
	lv_obj_add_flag(minute_obj, LV_OBJ_FLAG_HIDDEN);
	phandle->time_mark_obj[1] = minute_obj;

	lv_obj_t *second_obj = lv_obj_create(phandle->main_obj);
	lv_obj_set_style_bg_color(second_obj, lv_color_hex(0x00C700), 0);
	lv_obj_set_style_radius(second_obj, 15, 0);
	lv_obj_set_style_border_width(second_obj, 0, 0);
	lv_obj_set_style_pad_all(second_obj, 0, 0);
	lv_obj_clear_flag(second_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_t *second_label = lv_label_create(second_obj);
	lv_obj_set_style_text_font(second_label, divoom_menu_control_get_font_info(40, phandle->menu_id), 0);
	lv_obj_set_style_text_color(second_label, lv_color_hex(0x004700), 0);
	lv_label_set_text(second_label, divoom_menu_control_get_disp_str("s"));
	lv_obj_center(second_label);
	lv_obj_set_size(second_obj, 77, 71);
	lv_obj_set_pos(second_obj, 694, 338);
	lv_obj_add_flag(second_obj, LV_OBJ_FLAG_HIDDEN);
	phandle->time_mark_obj[2] = second_obj;
	
	lv_obj_t *colon_label1 = lv_label_create(phandle->main_obj);
	lv_obj_set_style_text_font(colon_label1, divoom_menu_control_get_font_info(67, phandle->menu_id), 0);
	lv_obj_set_style_text_color(colon_label1, lv_color_hex(0x333333), 0);	
	lv_label_set_text(colon_label1, ":");
	lv_obj_set_pos(colon_label1, 249, 551);
	lv_obj_set_size(colon_label1, 50, 75);

	lv_obj_t *colon_label2 = lv_label_create(phandle->main_obj);
	lv_obj_set_style_text_font(colon_label2, divoom_menu_control_get_font_info(67, phandle->menu_id), 0);
	lv_obj_set_style_text_color(colon_label2, lv_color_hex(0x333333), 0);	
	lv_label_set_text(colon_label2, ":");
	lv_obj_set_pos(colon_label2, 517, 551);
	lv_obj_set_size(colon_label2, 50, 75);

	phandle->stream_cont_hour = divoom_stream_cont_init(phandle->main_obj, 25, 422, 216-8, 334-8, 208, 122-2, 
			94, 94, 0x333333, 24, 0, 2);
	phandle->stream_cont_minute = divoom_stream_cont_init(phandle->main_obj, 292, 422, 216-8, 334-8, 208, 122-2, 
			94, 94, 0x333333, 60, 0, 2);
	phandle->stream_cont_second = divoom_stream_cont_init(phandle->main_obj, 560, 422, 216-8, 334-8, 208, 122-2, 
			94, 94, 0x333333, 60, 0, 2);
	divoom_stream_cont_set_current_selected(phandle->stream_cont_hour, 5);
	divoom_stream_cont_set_current_selected(phandle->stream_cont_minute, 5);
	divoom_stream_cont_set_current_selected(phandle->stream_cont_second, 5);


	void *stream_hour_btn = divoom_stream_cont_get_cont_obj(phandle->stream_cont_hour);
	lv_obj_set_style_border_color(stream_hour_btn, lv_color_hex(0xBEBEBE), 0);
	lv_obj_set_style_border_color(stream_hour_btn, lv_color_hex(0x0), LV_STATE_FOCUSED);
	lv_obj_set_style_border_color(stream_hour_btn, lv_color_hex(0x0), LV_STATE_FOCUS_KEY);
	lv_obj_set_style_border_width(stream_hour_btn, 4, 0);
	lv_obj_set_style_radius(stream_hour_btn, 51, 0);
	lv_obj_set_style_clip_corner(stream_hour_btn, true, 0);
	lv_obj_add_event_cb(stream_hour_btn, divoom_lvgl_count_down_menu_event_cb, LV_EVENT_KEY, phandle);
	phandle->btn_list[0] = stream_hour_btn;
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, stream_hour_btn, 0, 0);
	if(phandle->first_active_obj == NULL)
	{
		phandle->first_active_obj = stream_hour_btn;
	}
	lv_obj_t *stream_hour_cover = lv_img_create(stream_hour_btn);
	divoom_menu_control_lv_img_set_src(stream_hour_cover, "29/29_cont_cover.png");
	lv_obj_set_pos(stream_hour_cover, -4, -4);

	void *stream_minute_btn = divoom_stream_cont_get_cont_obj(phandle->stream_cont_minute);
	lv_obj_set_style_border_color(stream_minute_btn, lv_color_hex(0xBEBEBE), 0);
	lv_obj_set_style_border_color(stream_minute_btn, lv_color_hex(0x0), LV_STATE_FOCUSED);
	lv_obj_set_style_border_color(stream_minute_btn, lv_color_hex(0x0), LV_STATE_FOCUS_KEY);
	lv_obj_set_style_border_width(stream_minute_btn, 4, 0);
	lv_obj_set_style_radius(stream_minute_btn, 51, 0);
	lv_obj_set_style_clip_corner(stream_minute_btn, true, 0);
	lv_obj_add_event_cb(stream_minute_btn, divoom_lvgl_count_down_menu_event_cb, LV_EVENT_KEY, phandle);
	phandle->btn_list[1] = stream_minute_btn;
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, stream_minute_btn, 0, 1);
	lv_obj_t *stream_minute_cover = lv_img_create(stream_minute_btn);
	divoom_menu_control_lv_img_set_src(stream_minute_cover, "29/29_cont_cover.png");
	lv_obj_set_pos(stream_minute_cover, -4, -4);
	
	void *stream_second_btn = divoom_stream_cont_get_cont_obj(phandle->stream_cont_second);
	lv_obj_set_style_border_color(stream_second_btn, lv_color_hex(0xBEBEBE), 0);
	lv_obj_set_style_border_color(stream_second_btn, lv_color_hex(0x0), LV_STATE_FOCUSED);
	lv_obj_set_style_border_color(stream_second_btn, lv_color_hex(0x0), LV_STATE_FOCUS_KEY);
	lv_obj_set_style_border_width(stream_second_btn, 4, 0);
	lv_obj_set_style_radius(stream_second_btn, 51, 0);
	lv_obj_set_style_clip_corner(stream_second_btn, true, 0);
	lv_obj_add_event_cb(stream_second_btn, divoom_lvgl_count_down_menu_event_cb, LV_EVENT_KEY, phandle);
	phandle->btn_list[2] = stream_second_btn;
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, stream_second_btn, 0, 2);
	lv_obj_t *stream_second_cover = lv_img_create(stream_second_btn);
	divoom_menu_control_lv_img_set_src(stream_second_cover, "29/29_cont_cover.png");
	lv_obj_set_pos(stream_second_cover, -4, -4);
}


void *divoom_lvgl_count_down_menu_init(void *user_data, int menu_id)
{
	Pdivoom_lvgl_count_down_menu_ctrl phandle = (Pdivoom_lvgl_count_down_menu_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_count_down_menu_ctrl));
    phandle->menu_id = menu_id;
	phandle->start_flag = 0;
	phandle->btn_status = 0;
	phandle->select_id = 0;
	phandle->first_active_obj = NULL;
	phandle->pdirect_handle = divoom_menu_direct_ctrl_create(1, 5);
	phandle->pdirect_music_handle = divoom_menu_direct_ctrl_create(4, 3);
	phandle->main_obj = lv_obj_create(NULL);
	
	lv_style_init(&phandle->backgroud_type);
	lv_style_set_bg_color(&phandle->backgroud_type, lv_color_hex(0xE4E4E4));
	lv_style_set_text_color(&phandle->backgroud_type, lv_color_hex(0x0));
	lv_obj_add_style(phandle->main_obj, &phandle->backgroud_type, 0);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);

	lv_style_init(&phandle->btn_normal_type);
	lv_style_set_bg_color(&phandle->btn_normal_type, lv_color_white());
	lv_style_set_pad_all(&phandle->btn_normal_type, 0);
	lv_style_set_border_width(&phandle->btn_normal_type, 0);
	lv_style_set_shadow_width(&phandle->btn_normal_type, 0);
	lv_style_set_bg_opa(&phandle->btn_normal_type, LV_OPA_10);

	lv_style_init(&phandle->btn_press_type);
	lv_style_set_bg_color(&phandle->btn_press_type, lv_color_white());
	lv_style_set_pad_all(&phandle->btn_press_type, 0);
	lv_style_set_border_width(&phandle->btn_press_type, 0);
	lv_style_set_shadow_width(&phandle->btn_press_type, 0);
	lv_style_set_bg_opa(&phandle->btn_press_type, LV_OPA_30);
	
	divoom_lvgl_count_down_menu_create_title_area(phandle);
	divoom_lvgl_count_down_menu_create_time_cont(phandle);

	//start
	//lv_obj_t *start_obj = lv_obj_create(phandle->main_obj);
	//lv_obj_set_x(start_obj, 575);
	//lv_obj_set_y(start_obj, 860);
	//lv_obj_set_width(start_obj, 191);
	//lv_obj_set_height(start_obj, 191);
	//lv_obj_set_style_pad_all(start_obj, 0, 0);
	//lv_obj_set_style_border_width(start_obj, 5, 0);
	//lv_obj_set_style_border_color(start_obj, lv_color_hex(0x122914), 0);
	//lv_obj_set_style_radius(start_obj, LV_RADIUS_CIRCLE, 0);
	//lv_obj_set_style_bg_color(start_obj, lv_color_hex(0x000000), 0);
	//lv_obj_set_style_bg_opa(start_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	//lv_obj_set_style_bg_opa(start_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	//lv_obj_set_style_shadow_width(start_obj, 0, 0); //去掉边框底部的阴影

	//phandle->start_btn = lv_btn_create(start_obj);
	phandle->start_btn = lv_btn_create(phandle->main_obj);
	lv_obj_remove_style_all(phandle->start_btn);
	lv_obj_set_width(phandle->start_btn, 171);
	lv_obj_set_height(phandle->start_btn, 171);
	lv_obj_set_pos(phandle->start_btn, 575, 860);
	lv_obj_set_style_pad_all(phandle->start_btn, 0, 0);
	lv_obj_set_style_border_width(phandle->start_btn, 5, 0);
	lv_obj_set_style_border_color(phandle->start_btn, lv_color_hex(0xE4E4E4), 0);
	//lv_obj_set_style_border_opa(phandle->start_btn, LV_OPA_0, 0);
	lv_obj_set_style_radius(phandle->start_btn, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_bg_color(phandle->start_btn, lv_color_hex(0x99D999), 0);
	lv_obj_set_style_bg_color(phandle->start_btn, lv_color_hex(0x6BD96B), LV_STATE_FOCUSED);
	lv_obj_set_style_bg_color(phandle->start_btn, lv_color_hex(0x6BD96B), LV_STATE_FOCUS_KEY);
	//lv_obj_set_style_bg_color(phandle->start_btn, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_bg_opa(phandle->start_btn, LV_OPA_100, 0);
	lv_obj_set_style_bg_opa(phandle->start_btn, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	lv_obj_set_style_bg_opa(phandle->start_btn, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_shadow_width(phandle->start_btn, 0, 0); //去掉边框底部的阴影
	lv_obj_set_style_outline_width(phandle->start_btn, 5, 0);
	lv_obj_set_style_outline_color(phandle->start_btn, lv_color_hex(0x99D999), 0);
	lv_obj_set_style_outline_color(phandle->start_btn, lv_color_hex(0x00C700), LV_STATE_FOCUSED);
	lv_obj_set_style_outline_color(phandle->start_btn, lv_color_hex(0x00C700), LV_STATE_FOCUS_KEY);
	lv_obj_set_style_text_color(phandle->start_btn, lv_color_hex(0x4E8E4E), 0);
	lv_obj_set_style_text_color(phandle->start_btn, lv_color_hex(0x032C03), LV_STATE_FOCUSED);
	lv_obj_set_style_text_color(phandle->start_btn, lv_color_hex(0x032C03), LV_STATE_FOCUS_KEY);
	//lv_obj_set_align(phandle->start_btn, LV_ALIGN_CENTER);
	//lv_obj_add_event_cb(phandle->start_btn, divoom_lvgl_count_down_menu_event_cb, LV_EVENT_CLICKED, phandle);
	lv_obj_add_event_cb(phandle->start_btn, divoom_lvgl_count_down_menu_event_cb, LV_EVENT_KEY, phandle);
	phandle->btn_list[3] = phandle->start_btn;
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, phandle->start_btn, 0, 3);

	lv_obj_t *start_label = lv_label_create(phandle->start_btn);
	//lv_obj_set_style_text_color(start_label, lv_color_hex(0x67CE67), 0);
	lv_obj_set_style_text_font(start_label, divoom_menu_control_get_font_info(37, phandle->menu_id), 0);
	lv_label_set_text(start_label, divoom_menu_control_get_disp_str("Start"));
	lv_obj_center(start_label);

	//When Timer Ends
	lv_obj_t *music_btn = lv_btn_create(phandle->main_obj);
	lv_obj_remove_style_all(music_btn);
	lv_obj_set_style_radius(music_btn, 18, 0);
	lv_obj_set_pos(music_btn, 33, 1102);
	lv_obj_set_size(music_btn, 726, 89);
	lv_obj_set_style_pad_all(music_btn, 0, 0);
	lv_obj_set_style_shadow_width(music_btn, 0, 0);
	lv_obj_set_style_border_width(music_btn, 4, 0);
	lv_obj_set_style_border_color(music_btn, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_border_color(music_btn, lv_color_hex(0x0), LV_STATE_FOCUSED);
	lv_obj_set_style_border_color(music_btn, lv_color_hex(0x0), LV_STATE_FOCUS_KEY);
	lv_obj_set_style_bg_color(music_btn, lv_color_hex(0xF2F2F2), 0);
	lv_obj_set_style_bg_opa(music_btn, LV_OPA_100, 0);
	//lv_obj_add_event(music_btn, divoom_lvgl_count_down_menu_event_cb, LV_EVENT_CLICKED, phandle);
	lv_obj_add_event(music_btn, divoom_lvgl_count_down_menu_event_cb, LV_EVENT_KEY, phandle);
	phandle->music_btn = music_btn;
	phandle->btn_list[4] = music_btn;
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, music_btn, 0, 4);

	lv_obj_t *tip_label = lv_label_create(music_btn);
	lv_obj_set_style_text_font(tip_label, divoom_menu_control_get_font_info(33, phandle->menu_id), 0);
	lv_label_set_text(tip_label, divoom_menu_control_get_disp_str("When Timer Ends"));
	lv_obj_set_size(tip_label, 500, 48);
	lv_obj_set_pos(tip_label, 36, 21);

	phandle->music_index = 0;
	divoom_app_tool_set_count_down_music_index(phandle->music_index);
	lv_obj_t *music_label = lv_label_create(music_btn);
	lv_obj_set_style_text_color(music_label, lv_color_hex(0x616161), 0);
	lv_obj_set_style_text_font(music_label, divoom_menu_control_get_font_info(33, phandle->menu_id), 0);
	lv_obj_set_style_text_align(music_label, LV_TEXT_ALIGN_RIGHT, 0);
	lv_label_set_text_fmt(music_label, "Music %d", phandle->music_index+1);
	lv_obj_set_size(music_label, 160, 48);
	lv_obj_set_pos(music_label, 530-40, 21);
	phandle->music_label_obj = music_label;

	lv_obj_t *right_img = lv_img_create(music_btn);
	divoom_menu_control_lv_img_set_src(right_img, "29/29_right.png");
	lv_obj_set_pos(right_img, 666, 34-5);

	divoom_lvgl_count_down_menu_create_bell_list(phandle, phandle->main_obj);
	
	return phandle;
}

void divoom_lvgl_count_down_menu_disp(void *handle, void *active)
{
	Pdivoom_lvgl_count_down_menu_ctrl phandle = (Pdivoom_lvgl_count_down_menu_ctrl)handle;
	
	if(phandle->first_active_obj)
	{
		lv_group_focus_obj(phandle->first_active_obj);
		phandle->first_active_obj = NULL;
	}
	if(active)
	{
		if(phandle->start_flag)
		{
			phandle->start_flag = 0;
			divoom_stream_cont_set_current_selected(phandle->stream_cont_hour, 5);
			divoom_stream_cont_set_current_selected(phandle->stream_cont_minute, 5);
			divoom_stream_cont_set_current_selected(phandle->stream_cont_second, 5);

			phandle->music_index = 0;
			lv_label_set_text_fmt(phandle->music_label_obj, "Music %d", phandle->music_index+1);
			divoom_app_tool_set_count_down_music_index(phandle->music_index);
			
			phandle->cur_active_obj = phandle->bell_cont_obj[0];
		}
		lv_group_focus_obj(active);
	}

	lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_count_down_menu_exit(void *handle)
{
	Pdivoom_lvgl_count_down_menu_ctrl phandle = (Pdivoom_lvgl_count_down_menu_ctrl)handle;
	
	lv_timer_del(phandle->menu_bar_timer);
	lv_style_reset(&phandle->backgroud_type);
	lv_style_reset(&phandle->btn_normal_type);
	lv_style_reset(&phandle->btn_press_type);
	divoom_stream_cont_del(phandle->stream_cont_hour);
	divoom_stream_cont_del(phandle->stream_cont_minute);
	divoom_stream_cont_del(phandle->stream_cont_second);
	divoom_menu_direct_ctrl_destory(phandle->pdirect_handle);
	divoom_menu_direct_ctrl_destory(phandle->pdirect_music_handle);
	lv_obj_del(phandle->main_obj);
	divoom_system_free(phandle);

}



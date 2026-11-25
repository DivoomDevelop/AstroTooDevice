#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_stream_cont.h"

typedef struct _divoom_lvgl_count_down_start_menu_ctrl
{
    int menu_id;
	lv_style_t backgroud_type;
	void *main_obj;
	void *cout_obj;

	lv_style_t btn_normal_type;
	lv_style_t btn_press_type;

	void *pdirect_handle;
	void *pause_obj;
	void *pause_btn;
	void *pause_label;
	void *cancel_btn;
	//void *back_btn;
	void *first_active_obj;

	void *music_btn;
	int   music_index;
	void *music_label_obj;
	void *bells_obj;	//选择铃声页面
	void *bell_cont_obj[12];
	void *cur_active_obj;
	
	int status; // 0:停止	1:启动	 2:取消
	int count_down_hour;
	int count_down_minute;
	int count_down_second;
	void *arc_obj;
	lv_timer_t *arc_run_timer;

	void *time_label;
	void *remind_img;
	int  next_reach_time;
	void *end_time_label;
	
	void *time_obj;
	int hour;
	int minute;
	lv_timer_t *menu_bar_timer;
}divoom_lvgl_count_down_start_menu_ctrl, *Pdivoom_lvgl_count_down_start_menu_ctrl;

static const char *divoom_lvgl_count_down_start_menu_img_list[] = {
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

int divoom_lvgl_count_down_menu_get_disp_time(void *handle, int flag)
{
	Pdivoom_lvgl_count_down_start_menu_ctrl phandle = (Pdivoom_lvgl_count_down_start_menu_ctrl)handle;
	int sec = 0;	

	sec = divoom_app_tool_get_count_down_cnt();
	if (divoom_app_tool_get_count_down_status() == DIVOOM_DISP_COUNT_TIME_DOWN_WAIT || flag)
	{
		return phandle->count_down_hour*3600 + phandle->count_down_minute*60 + phandle->count_down_second;//等待状态下存在设备设置时间
	}

	return sec;
}

void divoom_lvgl_count_down_start_menu_start_time(void *handle)
{
	Pdivoom_lvgl_count_down_start_menu_ctrl phandle = (Pdivoom_lvgl_count_down_start_menu_ctrl)handle;
	uint32 sec = 0;
	//发送开始
	Pdivoom_app_tool_count_time_down_info count_info = divoom_system_malloc(sizeof(divoom_app_tool_count_time_down_info));
	divoom_sys_msg_info msg_info = {0};
	msg_info.dest_id = DIVOOM_MODEL_APP_TOOL;
	msg_info.msg_id = DIVOOM_APP_COUNT_TIME_DOWN_SET;	
	msg_info.msg_type = 1;
	sec = divoom_app_tool_get_count_down_cnt();	

	count_info->hour = phandle->count_down_hour;
	count_info->minute = phandle->count_down_minute;
	count_info->second = phandle->count_down_second;
	count_info->status = 1;
	count_info->lvgl_flag = 1;
	msg_info.data = (uint8 *)count_info;
	divoom_sys_model_crl_msg_push(&msg_info);
}

void divoom_lvgl_count_down_start_menu_pause_time(void *handle)
{
	Pdivoom_lvgl_count_down_start_menu_ctrl phandle = (Pdivoom_lvgl_count_down_start_menu_ctrl)handle;
	uint32 sec = 0;

	//发送暂停
	Pdivoom_app_tool_count_time_down_info count_info = divoom_system_malloc(sizeof(divoom_app_tool_count_time_down_info));
	divoom_sys_msg_info msg_info = {0};
	msg_info.dest_id = DIVOOM_MODEL_APP_TOOL;
	msg_info.msg_id = DIVOOM_APP_COUNT_TIME_DOWN_SET;	
	msg_info.msg_type = 1;
	sec = divoom_app_tool_get_count_down_cnt();	

	phandle->count_down_hour = sec/60/60;
	phandle->count_down_minute = sec/60%60;
	phandle->count_down_second = sec%60;
	count_info->hour = sec/60/60;
	count_info->minute = sec/60%60;
	count_info->second = sec%60;
	count_info->status = 0;
	count_info->lvgl_flag = 1;
	msg_info.data = (uint8 *)count_info;
	divoom_sys_model_crl_msg_push(&msg_info);
}

void divoom_lvgl_count_down_start_menu_reset_time(void *handle)
{
	Pdivoom_lvgl_count_down_start_menu_ctrl phandle = (Pdivoom_lvgl_count_down_start_menu_ctrl)handle;
	int sec = 60;
	//发送复位
	Pdivoom_app_tool_count_time_down_info count_info = divoom_system_malloc(sizeof(divoom_app_tool_count_time_down_info));
	divoom_sys_msg_info msg_info = {0};
	msg_info.dest_id = DIVOOM_MODEL_APP_TOOL;
	msg_info.msg_id = DIVOOM_APP_COUNT_TIME_DOWN_SET;	
	msg_info.msg_type = 1;

	phandle->count_down_hour = 0;
	phandle->count_down_minute = 1;
	phandle->count_down_second = 0;

	count_info->hour = sec/60/60;
	count_info->minute = sec/60%60;
	count_info->second = sec%60;
	count_info->status = 2;
	count_info->lvgl_flag = 1;
	msg_info.data = (uint8 *)count_info;
	divoom_sys_model_crl_msg_push(&msg_info);
}


int divoom_lvgl_count_down_start_menu_return(void *handle)
{
	int status;
	Pdivoom_lvgl_count_down_start_menu_ctrl phandle = (Pdivoom_lvgl_count_down_start_menu_ctrl)handle;

	status = divoom_app_tool_get_count_down_status();

	if(status == DIVOOM_DISP_COUNT_TIME_DOWN_TIME_OVER)
	{
		divoom_lvgl_count_down_start_menu_reset_time(phandle);
        return 1;
	}
    return 0;
}

void divoom_lvgl_count_down_start_menu_refresh_end_time(void *handle)
{
	int hour, time_sec, total_time;
	time_t end_time;
	time_t time_now;
	struct tm p_now;
	Pdivoom_lvgl_count_down_start_menu_ctrl phandle = (Pdivoom_lvgl_count_down_start_menu_ctrl)handle;

	total_time = divoom_app_tool_get_count_down_next_reach_time();
	
	if(phandle->next_reach_time == total_time)
	{
		return;
	}
	
	phandle->next_reach_time = total_time;

	time_now = time(NULL);
	time_sec = divoom_app_tool_get_count_down_cnt();
	end_time = time_now + time_sec;//当前时间加上倒计时时间
	localtime_r(&end_time, &p_now);

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

		lv_label_set_text_fmt(phandle->end_time_label, "%02d:%02d", hour, p_now.tm_min);
	}
	else
	{
		lv_label_set_text_fmt(phandle->end_time_label, "%02d:%02d", p_now.tm_hour, p_now.tm_min);
	}
}

void divoom_lvgl_count_down_start_menu_event_cb(lv_event_t *e)
{
	Pdivoom_lvgl_count_down_start_menu_ctrl phandle = (Pdivoom_lvgl_count_down_start_menu_ctrl)lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *cur_btn_obj = lv_event_get_target(e);

	/*switch(code)
	{
		case LV_EVENT_CLICKED:
			if(cur_btn_obj == phandle->pause_btn)
			{
				if (divoom_app_tool_get_count_down_status() == DIVOOM_DISP_COUNT_TIME_DOWN_WAIT)
				{
					divoom_lvgl_count_down_start_menu_start_time(phandle);
				}
				else
				{
					divoom_lvgl_count_down_start_menu_pause_time(phandle);
				}
			}
			else if(cur_btn_obj == phandle->back_btn)
			{
				divoom_menu_control_return_pre();
			}
			else if(cur_btn_obj == phandle->cancel_btn)
			{
				//复位
				divoom_lvgl_count_down_start_menu_reset_time(phandle);
				divoom_menu_control_return_pre();
			}
			else if(cur_btn_obj == phandle->music_btn)
			{
				lv_obj_clear_flag(phandle->bells_obj, LV_OBJ_FLAG_HIDDEN);
				lv_group_focus_obj(phandle->cur_active_obj);
				phandle->cur_active_obj = NULL;
			}
			break;

        default:
            break;

	}*/
	
	if(code == LV_EVENT_KEY)
	{
		char c = *((char*)lv_event_get_param(e));

		if(c == DIVOOM_LVGL_KEY_PRE)
		{
			lv_group_focus_obj(divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_btn_obj, DIVOOM_MENU_DIRECT_UP));
		}
		else if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			lv_group_focus_obj(divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_btn_obj, DIVOOM_MENU_DIRECT_DOWN));
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			if(cur_btn_obj == phandle->pause_btn)
			{
				if (divoom_app_tool_get_count_down_status() == DIVOOM_DISP_COUNT_TIME_DOWN_WAIT)
				{
					divoom_lvgl_count_down_start_menu_start_time(phandle);
				}
				else
				{
					divoom_lvgl_count_down_start_menu_pause_time(phandle);
				}
			}
			else
			{
				//复位
				divoom_lvgl_count_down_start_menu_reset_time(phandle);
				//divoom_menu_control_return_pre();
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			if(divoom_lvgl_count_down_start_menu_return(phandle) == 0)
			{
			    divoom_menu_control_return_pre();
			}
		}
		else
		{
			char *key_info = (char *)lv_event_get_param(e);
			divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
		}
	}
}


void divoom_lvgl_count_down_start_menu_refresh_menu_bar(lv_timer_t *menu_bar_timer)
{
	int hour;
    Pdivoom_lvgl_count_down_start_menu_ctrl phandle = (Pdivoom_lvgl_count_down_start_menu_ctrl)lv_timer_get_user_data(menu_bar_timer);
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

void divoom_lvgl_count_down_start_menu_create_title_area(Pdivoom_lvgl_count_down_start_menu_ctrl phandle)
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
	phandle->menu_bar_timer = lv_timer_create(divoom_lvgl_count_down_start_menu_refresh_menu_bar, 1000, phandle);
}

/*void divoom_lvgl_count_down_start_menu_bell_list_back_event_cb(lv_event_t * e)
{
	int tip_index;
	lv_event_code_t code = lv_event_get_code(e);
	Pdivoom_lvgl_count_down_start_menu_ctrl phandle = (Pdivoom_lvgl_count_down_start_menu_ctrl)lv_event_get_user_data(e);
	
	switch(code)
	{
		case LV_EVENT_CLICKED:
			tip_index = phandle->music_index;
			divoom_app_tool_set_count_down_music_index(tip_index);
			divoom_app_tool_count_down_menu_try_listen(0, 0, 0);
			lv_obj_add_flag(phandle->bells_obj, LV_OBJ_FLAG_HIDDEN);
			lv_label_set_text_fmt(phandle->music_label_obj, "Music %d", phandle->music_index+1);
			break;
        default:
            break;
	}
}

void divoom_lvgl_count_down_start_menu_bell_list_event_cb(lv_event_t * e)
{
	int i, tip_index;
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *cur_btn_obj = lv_event_get_target(e);
	Pdivoom_lvgl_count_down_start_menu_ctrl phandle = (Pdivoom_lvgl_count_down_start_menu_ctrl)lv_event_get_user_data(e);
	
	switch(code)
	{
		case LV_EVENT_CLICKED:
			for(i = 0;i < 12; i++)
			{
				if(cur_btn_obj == phandle->bell_cont_obj[i])
				{
					phandle->cur_active_obj = phandle->bell_cont_obj[i];
					phandle->music_index = i;
					tip_index = i;
					divoom_app_tool_count_down_menu_try_listen(tip_index, 50, 1);
					break;
				}
			}
        	break;
        default:
            break;
	}
}*/

void divoom_lvgl_count_down_start_menu_create_bell_list(Pdivoom_lvgl_count_down_start_menu_ctrl phandle, void *main_obj)
{
	int i, j;

	lv_obj_t *bells_obj = lv_obj_create(main_obj);
	lv_obj_set_style_bg_color(bells_obj, lv_color_black(), 0);
	lv_obj_set_style_border_width(bells_obj, 0, 0);
	lv_obj_set_style_pad_all(bells_obj, 0, 0);
	lv_obj_set_size(bells_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(bells_obj, 0, 0);
	lv_obj_add_flag(bells_obj, LV_OBJ_FLAG_HIDDEN);
	phandle->bells_obj = bells_obj;
	
	/*lv_obj_t *back_btn = lv_btn_create(bells_obj);
	lv_obj_set_size(back_btn, 72, 72); 
	lv_obj_set_pos(back_btn, 15, 44);
	lv_obj_set_style_bg_opa(back_btn, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(back_btn, 0, 0);
	lv_obj_set_style_shadow_width(back_btn, 0, 0);
	lv_obj_clear_flag(back_btn, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_event(back_btn, divoom_lvgl_count_down_start_menu_bell_list_back_event_cb, LV_EVENT_CLICKED, phandle);
	
	lv_obj_t *back_img = lv_img_create(back_btn);
	divoom_menu_control_lv_img_set_src(back_img, "29/29_bell_back.png");*/

	lv_obj_t *bell_label = lv_label_create(bells_obj);
	lv_obj_set_size(bell_label, 360, 56);
	lv_obj_set_pos(bell_label, 220, 52);
	lv_obj_set_style_text_color(bell_label, lv_color_white(), 0);
	lv_obj_set_style_text_font(bell_label, divoom_menu_control_get_font_info(48, phandle->menu_id), 0);
	lv_obj_set_style_text_align(bell_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_text(bell_label, divoom_menu_control_get_disp_str("Bells"));

	//lv_group_t *group = lv_group_create();
	for(i = 0; i < 3; i++)
	{
		for(j = 0; j < 4; j++)
		{
			lv_obj_t *bell_btn = lv_btn_create(phandle->bells_obj);
			lv_obj_set_style_pad_all(bell_btn, 0, 0);
			lv_obj_set_style_border_width(bell_btn, 4, 0);
			lv_obj_set_style_bg_color(bell_btn, lv_color_hex(0xE2E2E2), 0);
			lv_obj_set_style_bg_opa(bell_btn, 7*LV_OPA_100/100, 0);
			lv_obj_set_style_shadow_width(bell_btn, 0, 0);

			lv_obj_set_style_border_color(bell_btn, lv_color_hex(0xE2E2E2), 0);
			lv_obj_set_style_border_opa(bell_btn, 7*LV_OPA_100/100, 0);
			lv_obj_set_style_border_color(bell_btn, lv_color_hex(0xF1A33B), LV_STATE_FOCUSED);
			lv_obj_set_style_border_opa(bell_btn, LV_OPA_100, LV_STATE_FOCUSED);
			lv_obj_set_style_border_color(bell_btn, lv_color_hex(0xF1A33B), LV_STATE_FOCUS_KEY);
			lv_obj_set_style_border_opa(bell_btn, LV_OPA_100, LV_STATE_FOCUS_KEY);
			
			lv_obj_set_size(bell_btn, 150, 150);
			lv_obj_set_pos(bell_btn, 40 + 190*j, 208 + 190*i);

			lv_obj_t *bell_img = lv_img_create(bell_btn);
			divoom_menu_control_lv_img_set_src(bell_img, divoom_lvgl_count_down_start_menu_img_list[i*4+j]);
			lv_obj_center(bell_img);

			if(i*4+j == phandle->music_index)
			{
				phandle->cur_active_obj = bell_btn;
			}
			phandle->bell_cont_obj[i*4+j] = bell_btn;
			//lv_group_add_obj(group, bell_btn);
			//lv_obj_add_event_cb(bell_btn, divoom_lvgl_count_down_start_menu_bell_list_event_cb, LV_EVENT_CLICKED, phandle);
		}
	}
	
}


void divoom_lvgl_count_down_start_menu_refresh(struct _lv_timer_t * tool_refresh_time)
{
	static int sec_time = 0;
	int total_time;
	Pdivoom_lvgl_count_down_start_menu_ctrl phandle = (Pdivoom_lvgl_count_down_start_menu_ctrl)lv_timer_get_user_data(tool_refresh_time);
	
	sec_time = divoom_lvgl_count_down_menu_get_disp_time(phandle, 0);
	
	if(sec_time >= 0)
	{
		if(sec_time/60/60 > 0)
		{
			lv_label_set_text_fmt(phandle->time_label, "%02d:%02d:%02d", sec_time/60/60, sec_time/60%60, sec_time%60);
		}
		else
		{
			lv_label_set_text_fmt(phandle->time_label, "%02d:%02d", sec_time/60%60, sec_time%60);
		}
	}
	else
	{
		lv_label_set_text(phandle->time_label, "00:00");
	}

	if (divoom_app_tool_get_count_down_status() != phandle->status)
	{
		phandle->status = divoom_app_tool_get_count_down_status();

		if(phandle->status == DIVOOM_DISP_COUNT_TIME_DOWN_PHONE_CANCEL)
		{
			//divoom_menu_control_return_pre();
		}
		//根据状态变更显示图标
		if (phandle->status == DIVOOM_DISP_COUNT_TIME_DOWN_WAIT)
		{
			lv_obj_set_style_bg_color(phandle->pause_btn, lv_color_hex(0x99D999), 0);
			lv_obj_set_style_bg_color(phandle->pause_btn, lv_color_hex(0x6BD96B), LV_STATE_FOCUSED);
			lv_obj_set_style_bg_color(phandle->pause_btn, lv_color_hex(0x6BD96B), LV_STATE_FOCUS_KEY);
			//lv_obj_set_style_border_color(phandle->pause_obj, lv_color_hex(0x122914), 0);
			//lv_obj_set_style_text_color(phandle->pause_label, lv_color_hex(0x67CE67), 0);
			//lv_label_set_text(phandle->pause_label, divoom_menu_control_get_disp_str("Resume"));
			lv_obj_set_style_text_color(phandle->pause_btn, lv_color_hex(0x4E8E4E), 0);
			lv_obj_set_style_text_color(phandle->pause_btn, lv_color_hex(0x032C03), LV_STATE_FOCUSED);
			lv_obj_set_style_text_color(phandle->pause_btn, lv_color_hex(0x032C03), LV_STATE_FOCUS_KEY);
			lv_obj_set_style_outline_color(phandle->pause_btn, lv_color_hex(0x99D999), 0);
			lv_obj_set_style_outline_color(phandle->pause_btn, lv_color_hex(0x00C700), LV_STATE_FOCUSED);
			lv_obj_set_style_outline_color(phandle->pause_btn, lv_color_hex(0x00C700), LV_STATE_FOCUS_KEY);
			lv_label_set_text(phandle->pause_label, divoom_menu_control_get_disp_str("Resume"));
			
			//提醒时间和图案变暗
			lv_obj_set_style_text_opa(phandle->end_time_label, LV_OPA_70, 0);
			lv_obj_set_style_img_opa(phandle->remind_img, LV_OPA_70, 0);
		}
		else
		{
			lv_obj_set_style_bg_color(phandle->pause_btn, lv_color_hex(0xF3BD76), 0);
			lv_obj_set_style_bg_color(phandle->pause_btn, lv_color_hex(0xF3BD76), LV_STATE_FOCUSED);
			lv_obj_set_style_bg_color(phandle->pause_btn, lv_color_hex(0xF3BD76), LV_STATE_FOCUS_KEY);
			//lv_obj_set_style_border_color(phandle->pause_obj, lv_color_hex(0x302007), 0);
			//lv_obj_set_style_text_color(phandle->pause_label, lv_color_hex(0xF1A33B), 0);
			//lv_label_set_text(phandle->pause_label, divoom_menu_control_get_disp_str("Pause"));
			lv_obj_set_style_text_color(phandle->pause_btn, lv_color_hex(0xA87D40), 0);
			lv_obj_set_style_text_color(phandle->pause_btn, lv_color_hex(0x241703), LV_STATE_FOCUSED);
			lv_obj_set_style_text_color(phandle->pause_btn, lv_color_hex(0x241703), LV_STATE_FOCUS_KEY);
			lv_obj_set_style_outline_color(phandle->pause_btn, lv_color_hex(0xF3BD76), 0);
			lv_obj_set_style_outline_color(phandle->pause_btn, lv_color_hex(0xD9830F), LV_STATE_FOCUSED);
			lv_obj_set_style_outline_color(phandle->pause_btn, lv_color_hex(0xD9830F), LV_STATE_FOCUS_KEY);
			lv_label_set_text(phandle->pause_label, divoom_menu_control_get_disp_str("Pause"));
			
			//提醒时间和图案变亮
			lv_obj_set_style_text_opa(phandle->end_time_label, LV_OPA_100, 0);
			lv_obj_set_style_img_opa(phandle->remind_img, LV_OPA_100, 0);
			//更新到点时间
			if(phandle->status == DIVOOM_DISP_COUNT_TIME_DOWN_RUN)
			{
				divoom_lvgl_count_down_start_menu_refresh_end_time(phandle);
			}
		}
	}

	divoom_lvgl_count_down_start_menu_refresh_end_time(phandle);
	//lv_arc_set_value(phandle->arc_obj, sec_time/3);
}

void divoom_lvgl_count_down_start_menu_create_time_area(Pdivoom_lvgl_count_down_start_menu_ctrl phandle)
{	
	uint32 time_sec = 0, hour = 0;
	time_t end_time;
	time_t time_now;
	struct tm p_now;
	
	time_now = time(NULL);
	time_sec = divoom_app_tool_get_count_down_cnt();
	end_time = time_now + time_sec;//当前时间加上倒计时时间
	localtime_r(&end_time, &p_now);
	phandle->status = divoom_app_tool_get_count_down_status();
	//phandle->music_index = divoom_app_tool_get_count_down_music_index();
	
	//圆弧
	lv_obj_t *arc = lv_arc_create(phandle->main_obj);
	lv_arc_set_rotation(arc, 270);
	lv_arc_set_bg_angles(arc, 0, 360);
	lv_obj_remove_style(arc, NULL, LV_PART_KNOB);   /*Be sure the knob is not displayed*/
	lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);  /*To not allow adjusting by click*/
	lv_obj_set_style_arc_color(arc, lv_color_hex(0xE89B00), 0);
	lv_obj_set_style_arc_color(arc, lv_color_hex(0xE89B00), LV_PART_INDICATOR);
	lv_obj_set_style_arc_width(arc, 16, 0);
	lv_obj_set_style_arc_width(arc, 16, LV_PART_INDICATOR);
	//lv_arc_set_mode(arc, LV_ARC_MODE_REVERSE);
	lv_arc_set_range(arc, 0, 100);
	lv_arc_set_value(arc, 100);
	lv_obj_set_pos(arc, 85, 213);
	lv_obj_set_size(arc, 631, 631);

	phandle->arc_run_timer = lv_timer_create(divoom_lvgl_count_down_start_menu_refresh, 250, phandle);
	phandle->arc_obj = arc;

	//时间
	lv_obj_t *time_label = lv_label_create(phandle->main_obj);
	lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(120, phandle->menu_id), 0);
	lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_color(time_label, lv_color_hex(0x333333), 0);
	lv_obj_set_pos(time_label, 149-45, 430);
	lv_obj_set_size(time_label, 600, 152);
	phandle->time_label = time_label;
	
	if(time_sec/60/60 > 0)
	{
		lv_label_set_text_fmt(time_label, "%02d:%02d:%02d", time_sec/60/60, time_sec/60%60, time_sec%60);
	}
	else
	{
		lv_label_set_text_fmt(time_label, "%02d:%02d", time_sec/60%60, time_sec%60);
	}

	lv_obj_t *remind_img = lv_img_create(phandle->main_obj);
	divoom_menu_control_lv_img_set_src(remind_img, "29/29_remind.png");
	lv_obj_set_pos(remind_img, 307, 598+8);
	phandle->remind_img = remind_img;

	lv_obj_t  *end_time_label = lv_label_create(phandle->main_obj);
	lv_obj_set_style_text_font(end_time_label, divoom_menu_control_get_font_info(50, phandle->menu_id), 0);
	lv_obj_set_style_text_color(end_time_label, lv_color_hex(0x707070), 0);
	lv_obj_set_pos(end_time_label, 355, 593+3);
	lv_obj_set_size(end_time_label, 200, 52);
	phandle->end_time_label = end_time_label;
	
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

		lv_label_set_text_fmt(end_time_label, "%02d:%02d", hour, p_now.tm_min);
	}
	else
	{
		lv_label_set_text_fmt(end_time_label, "%02d:%02d", p_now.tm_hour, p_now.tm_min);
	}

}

void *divoom_lvgl_count_down_start_menu_init(void *user_data, int menu_id)
{
	Pdivoom_lvgl_count_down_start_menu_ctrl phandle = (Pdivoom_lvgl_count_down_start_menu_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_count_down_start_menu_ctrl));
    phandle->menu_id = menu_id;
	phandle->first_active_obj = NULL;
	phandle->count_down_hour = 0;
	phandle->count_down_minute = 1;
	phandle->count_down_second = 0;
	phandle->pdirect_handle = divoom_menu_direct_ctrl_create(1, 2);
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
	
	divoom_lvgl_count_down_start_menu_create_title_area(phandle);
	divoom_lvgl_count_down_start_menu_create_time_area(phandle);

	//start
	//lv_obj_t *pause_obj = lv_obj_create(phandle->main_obj);
	//lv_obj_set_x(pause_obj, 575);
	//lv_obj_set_y(pause_obj, 849);
	//lv_obj_set_width(pause_obj, 191);
	//lv_obj_set_height(pause_obj, 191);
	//lv_obj_set_style_pad_all(pause_obj, 0, 0);
	//lv_obj_set_style_border_width(pause_obj, 5, 0);
	//lv_obj_set_style_border_color(pause_obj, lv_color_hex(0x302007), 0);
	//lv_obj_set_style_radius(pause_obj, LV_RADIUS_CIRCLE, 0);
	//lv_obj_set_style_bg_color(pause_obj, lv_color_hex(0x000000), 0);
	//lv_obj_set_style_bg_opa(pause_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	//lv_obj_set_style_bg_opa(pause_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	//lv_obj_set_style_shadow_width(pause_obj, 0, 0); //去掉边框底部的阴影
	//phandle->pause_obj = pause_obj;

	//phandle->pause_btn = lv_btn_create(pause_obj);
	phandle->pause_btn = lv_btn_create(phandle->main_obj);
	lv_obj_remove_style_all(phandle->pause_btn);
	lv_obj_set_width(phandle->pause_btn, 171);
	lv_obj_set_height(phandle->pause_btn, 171);
	lv_obj_set_pos(phandle->pause_btn, 579, 928);
	lv_obj_set_style_pad_all(phandle->pause_btn, 0, 0);
	lv_obj_set_style_border_width(phandle->pause_btn, 5, 0);
	lv_obj_set_style_border_color(phandle->pause_btn, lv_color_hex(0xE4E4E4), 0);
	//lv_obj_set_style_border_opa(phandle->pause_btn, LV_OPA_0, 0);
	lv_obj_set_style_radius(phandle->pause_btn, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_bg_color(phandle->pause_btn, lv_color_hex(0xF3BD76), 0);
	lv_obj_set_style_bg_opa(phandle->pause_btn, LV_OPA_100, 0);
	lv_obj_set_style_bg_opa(phandle->pause_btn, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	lv_obj_set_style_bg_opa(phandle->pause_btn, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_shadow_width(phandle->pause_btn, 0, 0); //去掉边框底部的阴影
	lv_obj_set_style_outline_width(phandle->pause_btn, 5, 0);
	lv_obj_set_style_text_color(phandle->pause_btn, lv_color_hex(0xA87D40), 0);
	lv_obj_set_style_text_color(phandle->pause_btn, lv_color_hex(0x241703), LV_STATE_FOCUSED);
	lv_obj_set_style_text_color(phandle->pause_btn, lv_color_hex(0x241703), LV_STATE_FOCUS_KEY);
	lv_obj_set_style_outline_color(phandle->pause_btn, lv_color_hex(0xF3BD76), 0);
	lv_obj_set_style_outline_color(phandle->pause_btn, lv_color_hex(0xD9830F), LV_STATE_FOCUSED);
	lv_obj_set_style_outline_color(phandle->pause_btn, lv_color_hex(0xD9830F), LV_STATE_FOCUS_KEY);
	//lv_obj_set_align(phandle->pause_btn, LV_ALIGN_CENTER);
	//lv_obj_add_event_cb(phandle->pause_btn, divoom_lvgl_count_down_start_menu_event_cb, LV_EVENT_CLICKED, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, phandle->pause_btn, 0, 1);
	lv_obj_add_event_cb(phandle->pause_btn, divoom_lvgl_count_down_start_menu_event_cb, LV_EVENT_KEY, phandle);
	if(phandle->first_active_obj == NULL)
	{
		phandle->first_active_obj = phandle->pause_btn;
	}
	
	lv_obj_t *pause_label = lv_label_create(phandle->pause_btn);
	//lv_obj_set_style_text_color(pause_label, lv_color_hex(0xF1A33B), 0);
	lv_obj_set_style_text_font(pause_label, divoom_menu_control_get_font_info(37, phandle->menu_id), 0);
	lv_label_set_text(pause_label, divoom_menu_control_get_disp_str("Pause"));
	lv_obj_center(pause_label);
	phandle->pause_label = pause_label;

	//cancel
	//lv_obj_t *cancel_obj = lv_obj_create(phandle->main_obj);
	//lv_obj_set_x(cancel_obj, 40);
	//lv_obj_set_y(cancel_obj, 849);
	//lv_obj_set_width(cancel_obj, 191);
	//lv_obj_set_height(cancel_obj, 191);
	//lv_obj_set_style_pad_all(cancel_obj, 0, 0);
	//lv_obj_set_style_border_width(cancel_obj, 5, 0);
	//lv_obj_set_style_border_color(cancel_obj, lv_color_hex(0x1C1C1D), 0);
	//lv_obj_set_style_radius(cancel_obj, LV_RADIUS_CIRCLE, 0);
	//lv_obj_set_style_bg_color(cancel_obj, lv_color_hex(0x000000), 0);
	//lv_obj_set_style_bg_opa(cancel_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	//lv_obj_set_style_bg_opa(cancel_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	//lv_obj_set_style_shadow_width(cancel_obj, 0, 0); //去掉边框底部的阴影

	//phandle->cancel_btn = lv_btn_create(cancel_obj);
	phandle->cancel_btn = lv_btn_create(phandle->main_obj);
	lv_obj_remove_style_all(phandle->cancel_btn);
	lv_obj_set_width(phandle->cancel_btn, 171);
	lv_obj_set_height(phandle->cancel_btn, 171);
	lv_obj_set_pos(phandle->cancel_btn, 44, 928);
	lv_obj_set_style_pad_all(phandle->cancel_btn, 0, 0);
	lv_obj_set_style_border_width(phandle->cancel_btn, 5, 0);
	lv_obj_set_style_border_color(phandle->cancel_btn, lv_color_hex(0xE4E4E4), 0);
	//lv_obj_set_style_border_opa(phandle->cancel_btn, LV_OPA_0, 0);
	lv_obj_set_style_radius(phandle->cancel_btn, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_bg_color(phandle->cancel_btn, lv_color_hex(0xCECECE), 0);
	lv_obj_set_style_bg_opa(phandle->cancel_btn, LV_OPA_100, 0);
	lv_obj_set_style_bg_opa(phandle->cancel_btn, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	lv_obj_set_style_bg_opa(phandle->cancel_btn, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_shadow_width(phandle->cancel_btn, 0, 0); //去掉边框底部的阴影
	lv_obj_set_style_outline_width(phandle->cancel_btn, 5, 0);
	lv_obj_set_style_text_color(phandle->cancel_btn, lv_color_hex(0x808080), 0);
	lv_obj_set_style_text_color(phandle->cancel_btn, lv_color_hex(0x0), LV_STATE_FOCUSED);
	lv_obj_set_style_text_color(phandle->cancel_btn, lv_color_hex(0x0), LV_STATE_FOCUS_KEY);
	lv_obj_set_style_outline_color(phandle->cancel_btn, lv_color_hex(0xCECECE), 0);
	lv_obj_set_style_outline_color(phandle->cancel_btn, lv_color_hex(0x0), LV_STATE_FOCUSED);
	lv_obj_set_style_outline_color(phandle->cancel_btn, lv_color_hex(0x0), LV_STATE_FOCUS_KEY);
	//lv_obj_set_align(phandle->cancel_btn, LV_ALIGN_CENTER);
	//lv_obj_add_event_cb(phandle->cancel_btn, divoom_lvgl_count_down_start_menu_event_cb, LV_EVENT_CLICKED, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, phandle->cancel_btn, 0, 0);
	lv_obj_add_event_cb(phandle->cancel_btn, divoom_lvgl_count_down_start_menu_event_cb, LV_EVENT_KEY, phandle);

	lv_obj_t *cancel_label = lv_label_create(phandle->cancel_btn);
	//lv_obj_set_style_text_color(cancel_label, lv_color_hex(0x5A5A5E), 0);
	lv_obj_set_style_text_font(cancel_label, divoom_menu_control_get_font_info(37, phandle->menu_id), 0);
	lv_label_set_text(cancel_label, divoom_menu_control_get_disp_str("Cancel"));
	lv_obj_center(cancel_label);

	//When Timer Ends
	/*lv_obj_t *music_btn = lv_btn_create(phandle->main_obj);
	lv_obj_add_style(music_btn, &phandle->btn_normal_type, 0);
	lv_obj_add_style(music_btn, &phandle->btn_press_type, LV_STATE_PRESSED);
	lv_obj_set_style_radius(music_btn, 18, 0);
	lv_obj_set_pos(music_btn, 33, 1091);
	lv_obj_set_size(music_btn, 726, 89);
	lv_obj_add_event(music_btn, divoom_lvgl_count_down_start_menu_event_cb, LV_EVENT_CLICKED, phandle);
	phandle->music_btn = music_btn;

	lv_obj_t *tip_label = lv_label_create(music_btn);
	lv_obj_set_style_text_font(tip_label, divoom_menu_control_get_font_info(33, phandle->menu_id), 0);
	lv_label_set_text(tip_label, divoom_menu_control_get_disp_str("When Timer Ends"));
	lv_obj_set_size(tip_label, 500, 48);
	lv_obj_set_pos(tip_label, 36, 21);

	lv_obj_t *music_label = lv_label_create(music_btn);
	lv_obj_set_style_text_color(music_label, lv_color_hex(0x9E9E9E), 0);
	lv_obj_set_style_text_font(music_label, divoom_menu_control_get_font_info(33, phandle->menu_id), 0);
	lv_obj_set_style_text_align(music_label, LV_TEXT_ALIGN_RIGHT, 0);
	lv_label_set_text_fmt(music_label, "Music %d", phandle->music_index+1);
	lv_obj_set_size(music_label, 160, 48);
	lv_obj_set_pos(music_label, 530-40, 21);
	phandle->music_label_obj = music_label;

	lv_obj_t *right_img = lv_img_create(music_btn);
	divoom_menu_control_lv_img_set_src(right_img, "29/29_right.png");
	lv_obj_set_pos(right_img, 666, 34-5);

	divoom_lvgl_count_down_start_menu_create_bell_list(phandle, phandle->main_obj);*/

	if (phandle->status == DIVOOM_DISP_COUNT_TIME_DOWN_WAIT)
	{
		lv_obj_set_style_bg_color(phandle->pause_btn, lv_color_hex(0x99D999), 0);
		lv_obj_set_style_bg_color(phandle->pause_btn, lv_color_hex(0x6BD96B), LV_STATE_FOCUSED);
		lv_obj_set_style_bg_color(phandle->pause_btn, lv_color_hex(0x6BD96B), LV_STATE_FOCUS_KEY);
		//lv_obj_set_style_border_color(phandle->pause_obj, lv_color_hex(0x122914), 0);
		//lv_obj_set_style_text_color(phandle->pause_label, lv_color_hex(0x67CE67), 0);
		//lv_label_set_text(phandle->pause_label, divoom_menu_control_get_disp_str("Resume"));
		lv_obj_set_style_text_color(phandle->pause_btn, lv_color_hex(0x4E8E4E), 0);
		lv_obj_set_style_text_color(phandle->pause_btn, lv_color_hex(0x032C03), LV_STATE_FOCUSED);
		lv_obj_set_style_text_color(phandle->pause_btn, lv_color_hex(0x032C03), LV_STATE_FOCUS_KEY);
		lv_obj_set_style_outline_color(phandle->pause_btn, lv_color_hex(0x99D999), 0);
		lv_obj_set_style_outline_color(phandle->pause_btn, lv_color_hex(0x00C700), LV_STATE_FOCUSED);
		lv_obj_set_style_outline_color(phandle->pause_btn, lv_color_hex(0x00C700), LV_STATE_FOCUS_KEY);
		lv_label_set_text(phandle->pause_label, divoom_menu_control_get_disp_str("Resume"));
			
		//提醒时间和图案变暗
		lv_obj_set_style_text_opa(phandle->end_time_label, LV_OPA_70, 0);
		lv_obj_set_style_img_opa(phandle->remind_img, LV_OPA_70, 0);
	}
	else
	{
		lv_obj_set_style_bg_color(phandle->pause_btn, lv_color_hex(0xF3BD76), 0);
		lv_obj_set_style_bg_color(phandle->pause_btn, lv_color_hex(0xF3BD76), LV_STATE_FOCUSED);
		lv_obj_set_style_bg_color(phandle->pause_btn, lv_color_hex(0xF3BD76), LV_STATE_FOCUS_KEY);
		//lv_obj_set_style_border_color(phandle->pause_obj, lv_color_hex(0x302007), 0);
		//lv_obj_set_style_text_color(phandle->pause_label, lv_color_hex(0xF1A33B), 0);
		//lv_label_set_text(phandle->pause_label, divoom_menu_control_get_disp_str("Pause"));
		lv_obj_set_style_text_color(phandle->pause_btn, lv_color_hex(0xA87D40), 0);
		lv_obj_set_style_text_color(phandle->pause_btn, lv_color_hex(0x241703), LV_STATE_FOCUSED);
		lv_obj_set_style_text_color(phandle->pause_btn, lv_color_hex(0x241703), LV_STATE_FOCUS_KEY);
		lv_obj_set_style_outline_color(phandle->pause_btn, lv_color_hex(0xF3BD76), 0);
		lv_obj_set_style_outline_color(phandle->pause_btn, lv_color_hex(0xD9830F), LV_STATE_FOCUSED);
		lv_obj_set_style_outline_color(phandle->pause_btn, lv_color_hex(0xD9830F), LV_STATE_FOCUS_KEY);
		lv_label_set_text(phandle->pause_label, divoom_menu_control_get_disp_str("Pause"));
			
		//提醒时间和图案变亮
		lv_obj_set_style_text_opa(phandle->end_time_label, LV_OPA_100, 0);
		lv_obj_set_style_img_opa(phandle->remind_img, LV_OPA_100, 0);
		if(phandle->status == DIVOOM_DISP_COUNT_TIME_DOWN_RUN)
		{
			//更新到点时间
			divoom_lvgl_count_down_start_menu_refresh_end_time(phandle);
		}
	}
	
	return phandle;
}

void divoom_lvgl_count_down_start_menu_disp(void *handle, void *active)
{
	Pdivoom_lvgl_count_down_start_menu_ctrl phandle = (Pdivoom_lvgl_count_down_start_menu_ctrl)handle;

	if(phandle->first_active_obj)
	{
		lv_group_focus_obj(phandle->first_active_obj);
		phandle->first_active_obj = NULL;
	}
	lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_count_down_start_menu_exit(void *handle)
{
	Pdivoom_lvgl_count_down_start_menu_ctrl phandle = (Pdivoom_lvgl_count_down_start_menu_ctrl)handle;

	if(phandle->menu_bar_timer)
	{
		lv_timer_del(phandle->menu_bar_timer);
	}
	if(phandle->arc_run_timer)
	{
		lv_timer_del(phandle->arc_run_timer);
	}
	lv_style_reset(&phandle->backgroud_type);
	lv_style_reset(&phandle->btn_normal_type);
	lv_style_reset(&phandle->btn_press_type);
	divoom_menu_direct_ctrl_destory(phandle->pdirect_handle);
	lv_obj_del(phandle->main_obj);
	divoom_system_free(phandle);

}



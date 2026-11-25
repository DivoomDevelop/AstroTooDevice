#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"

typedef struct _divoom_lvgl_stop_watch_menu_ctrl
{
    int menu_id;
	lv_style_t backgroud_type;
	void *main_obj;
	void *back_btn;

	int status;
	void *time_obj;
	void *time_obj2;
	void *time_obj3;
	lv_style_t time_obj_style;

	void *lap_btn;
	void *lap_label;

	lv_style_t btn_normal_style;
	lv_style_t btn_select_style;

	//void *start_obj;
	void *start_btn;
	void *start_label;

	void *lap_cont;
	int lap_num;
	uint32 lap_time[100];
	void *pdirect_handle;
	void *first_active_obj;

	lv_timer_t *tool_refresh_timer;
	
	void *time_label_obj;
	int hour;
	int minute;
	lv_timer_t *menu_bar_timer;

	void *box_img;
	void *bg_img;
	int disp_times;		
	int bg_img_flag;
}divoom_lvgl_stop_watch_menu_ctrl, *Pdivoom_lvgl_stop_watch_menu_ctrl;


void divoom_lvgl_stop_watch_menu_refresh_menu_bar(lv_timer_t *menu_bar_timer)
{
	int hour;
    Pdivoom_lvgl_stop_watch_menu_ctrl phandle = (Pdivoom_lvgl_stop_watch_menu_ctrl)lv_timer_get_user_data(menu_bar_timer);
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
        lv_label_set_text_fmt(phandle->time_label_obj, "%02d: %02d", hour, phandle->minute);
    }
}

void divoom_lvgl_stop_watch_menu_ctrl_time(int flag)
{
	divoom_sys_msg_info model_msg = {0};
	divoom_app_tool_time_watch_info watch_ctrl;

	if (flag == 2)
		watch_ctrl.ctrl_flag = 2;
	else if (flag == 0)
		watch_ctrl.ctrl_flag = 0;
	else
		watch_ctrl.ctrl_flag = 1;

	model_msg.src_id = DIVOOM_MODEL_APP_TOOL;
	model_msg.dest_id = DIVOOM_MODEL_APP_TOOL;
	model_msg.msg_id = DIVOOM_APP_TIME_WATCH_SET;
	model_msg.data = (uint8 *)&watch_ctrl;

	divoom_sys_model_crl_msg_push(&model_msg);
}

void divoom_lvgl_stop_watch_menu_event_cb(lv_event_t * e);

void divoom_lvgl_stop_watch_remind_box(Pdivoom_lvgl_stop_watch_menu_ctrl phandle);

void divoom_lvgl_stop_watch_menu_add_lap(Pdivoom_lvgl_stop_watch_menu_ctrl phandle)
{
	if(phandle->lap_num >= 99)
	{
		if(phandle->bg_img_flag == 0)
		{
			phandle->bg_img_flag = 1;
			divoom_lvgl_stop_watch_remind_box(phandle);
		}
		return;
	}
	int sec, millisecond;
	divoom_disp_ctrl_watch_ctrl watch_info;
	divoom_app_tool_get_watch_cnt(&watch_info);
	sec = watch_info.watch_disp_value/100;
	millisecond = watch_info.watch_disp_value%100;
	phandle->lap_time[phandle->lap_num] = watch_info.watch_disp_value;
	
	lv_obj_t *obj = lv_obj_create(phandle->lap_cont);
	lv_obj_set_size(obj, 720, 98);
	lv_obj_set_style_pad_all(obj, 0, 0);
	lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);
	lv_obj_set_style_border_width(obj, 0, 0);
	lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
		
	lv_obj_t *btn = lv_btn_create(obj);
	lv_obj_set_size(btn, 712, 90);
	lv_obj_add_style(btn, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	//lv_obj_set_style_pad_all(btn, 0, 0);
	//lv_obj_set_style_shadow_width(btn, 0, 0);
	//lv_obj_set_style_border_width(btn, 4, 0);
	//lv_obj_set_style_border_color(btn, lv_color_black(), 0);
	//lv_obj_set_style_border_color(btn, lv_color_hex(0xBBBBBB), LV_STATE_FOCUSED);
	//lv_obj_set_style_border_color(btn, lv_color_hex(0xBBBBBB), LV_STATE_FOCUS_KEY);
	//lv_obj_set_style_bg_color(btn, lv_color_hex(0x181818), 0);
	lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_event_cb(btn, divoom_lvgl_stop_watch_menu_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn, phandle->lap_num+2, 0);
	
	lv_obj_t *lap_time_label = lv_label_create(obj);
	lv_obj_set_style_text_font(lap_time_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
	lv_obj_set_style_text_color(lap_time_label, lv_color_black(), 0);
	phandle->lap_num++;
	lv_label_set_text_fmt(lap_time_label, "%s %d", divoom_menu_control_get_disp_str("Lap"), phandle->lap_num);
	lv_obj_set_pos(lap_time_label, 32, 28);
	
	lv_obj_t *time_label2 = lv_label_create(obj);
	lv_obj_set_style_text_font(time_label2, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
	lv_obj_set_style_text_color(time_label2, lv_color_hex(0x616161), 0);
	lv_obj_set_pos(time_label2, 561, 28);

	if(sec/60 < 100)
	{
		lv_label_set_text_fmt(time_label2, "%02d:%02d.%02d", sec/60, sec%60, millisecond%100);
	}
	else
	{
		//lv_label_set_text_fmt(time_label2, "99:%02d.%02d", sec%60, millisecond%100);
		lv_label_set_text_fmt(time_label2, "99:99.99");
	}
	divoom_app_tool_set_watch_lap_nums(&phandle->lap_time[0], phandle->lap_num);
}




void divoom_lvgl_stop_watch_menu_create_laps(Pdivoom_lvgl_stop_watch_menu_ctrl phandle)
{
	int sec, millisecond, i;

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
	
	for(i = 0;i < phandle->lap_num; i++)
	{
		lv_obj_t *obj = lv_obj_create(phandle->lap_cont);
		lv_obj_set_size(obj, 720, 98);
		lv_obj_set_style_pad_all(obj, 0, 0);
		lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);
		lv_obj_set_style_border_width(obj, 0, 0);
		lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
		
		lv_obj_t *btn = lv_btn_create(obj);
		lv_obj_set_size(btn, 712, 90);
		lv_obj_add_style(btn, &phandle->btn_normal_style, 0);
		lv_obj_add_style(btn, &phandle->btn_select_style, LV_STATE_FOCUSED);
		lv_obj_add_style(btn, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
		//lv_obj_set_style_pad_all(btn, 0, 0);
		//lv_obj_set_style_shadow_width(btn, 0, 0);
		//lv_obj_set_style_border_width(btn, 4, 0);
		//lv_obj_set_style_border_color(btn, lv_color_black(), 0);
		//lv_obj_set_style_border_color(btn, lv_color_hex(0xBBBBBB), LV_STATE_FOCUSED);
		//lv_obj_set_style_border_color(btn, lv_color_hex(0xBBBBBB), LV_STATE_FOCUS_KEY);
		//lv_obj_set_style_bg_color(btn, lv_color_hex(0x181818), 0);
		lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_add_event_cb(btn, divoom_lvgl_stop_watch_menu_event_cb, LV_EVENT_KEY, phandle);
		divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn, i+2, 0);
		
		lv_obj_t *lap_time_label = lv_label_create(obj);
		lv_obj_set_style_text_font(lap_time_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
		lv_obj_set_style_text_color(lap_time_label, lv_color_black(), 0);
		lv_label_set_text_fmt(lap_time_label, "%s %d", divoom_menu_control_get_disp_str("Lap"), i+1);
		lv_obj_set_pos(lap_time_label, 32, 28);
	
		lv_obj_t *time_label2 = lv_label_create(obj);
		lv_obj_set_style_text_font(time_label2, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
		lv_obj_set_style_text_color(time_label2, lv_color_hex(0x616161), 0);
		lv_obj_set_pos(time_label2, 561, 28);

		sec = phandle->lap_time[i]/100;
		millisecond = phandle->lap_time[i]%100;
		
		if(sec/60 < 100)
		{
			lv_label_set_text_fmt(time_label2, "%02d:%02d.%02d", sec/60, sec%60, millisecond%100);
		}
		else
		{
			//lv_label_set_text_fmt(time_label2, "99:%02d.%02d", sec%60, millisecond%100);
			lv_label_set_text_fmt(time_label2, "99:99.99");
		}
	}
}

void divoom_lvgl_stop_watch_menu_reset_lap(Pdivoom_lvgl_stop_watch_menu_ctrl phandle)
{
	memset(phandle->lap_time, 0, sizeof(phandle->lap_time));
	
	for(int i = 0; i < phandle->lap_num; i++)
	{
		divoom_menu_direct_ctrl_update(phandle->pdirect_handle, NULL, i + 2, 0);
	}
	
	phandle->lap_num = 0;
	divoom_app_tool_set_watch_lap_nums(&phandle->lap_time[0], phandle->lap_num);
	lv_obj_clean(phandle->lap_cont);
}
//显示开始
void divoom_lvgl_stop_watch_menu_start_status(Pdivoom_lvgl_stop_watch_menu_ctrl phandle)
{
	//启动按键
	lv_obj_set_style_bg_color(phandle->start_btn, lv_color_hex(0x99D999), 0);
	lv_obj_set_style_bg_color(phandle->start_btn, lv_color_hex(0x6BD96B), LV_STATE_FOCUSED);
	lv_obj_set_style_bg_color(phandle->start_btn, lv_color_hex(0x6BD96B), LV_STATE_FOCUS_KEY);
	
	//lv_obj_set_style_border_color(phandle->start_obj, lv_color_hex(0x122914), 0);
	//lv_obj_set_style_text_color(phandle->start_label, lv_color_hex(0x67CE67), 0);
	//lv_label_set_text(phandle->start_label, divoom_menu_control_get_disp_str("Start"));
	lv_obj_set_style_text_color(phandle->start_btn, lv_color_hex(0x4E8E4E), 0);
	lv_obj_set_style_text_color(phandle->start_btn, lv_color_hex(0x032C03), LV_STATE_FOCUSED);
	lv_obj_set_style_text_color(phandle->start_btn, lv_color_hex(0x032C03), LV_STATE_FOCUS_KEY);
	lv_obj_set_style_outline_color(phandle->start_btn, lv_color_hex(0x99D999), 0);
	lv_obj_set_style_outline_color(phandle->start_btn, lv_color_hex(0x00C700), LV_STATE_FOCUSED);
	lv_obj_set_style_outline_color(phandle->start_btn, lv_color_hex(0x00C700), LV_STATE_FOCUS_KEY);
	lv_label_set_text(phandle->start_label, divoom_menu_control_get_disp_str("Start"));

	//复位按键
	lv_label_set_text(phandle->lap_label, divoom_menu_control_get_disp_str("Reset"));
}

void divoom_lvgl_stop_watch_menu_stop_status(Pdivoom_lvgl_stop_watch_menu_ctrl phandle)
{
	lv_obj_set_style_bg_color(phandle->start_btn, lv_color_hex(0xF3BD76), 0);
	lv_obj_set_style_bg_color(phandle->start_btn, lv_color_hex(0xF3BD76), LV_STATE_FOCUSED);
	lv_obj_set_style_bg_color(phandle->start_btn, lv_color_hex(0xF3BD76), LV_STATE_FOCUS_KEY);
	
	//lv_obj_set_style_border_color(phandle->start_obj, lv_color_hex(0x302007), 0);
	//lv_obj_set_style_text_color(phandle->start_label, lv_color_hex(0x916223), 0);
	//lv_label_set_text(phandle->start_label, divoom_menu_control_get_disp_str("Stop"));
	lv_obj_set_style_text_color(phandle->start_btn, lv_color_hex(0xA87D40), 0);
	lv_obj_set_style_text_color(phandle->start_btn, lv_color_hex(0x241703), LV_STATE_FOCUSED);
	lv_obj_set_style_text_color(phandle->start_btn, lv_color_hex(0x241703), LV_STATE_FOCUS_KEY);
	lv_obj_set_style_outline_color(phandle->start_btn, lv_color_hex(0xF3BD76), 0);
	lv_obj_set_style_outline_color(phandle->start_btn, lv_color_hex(0xD9830F), LV_STATE_FOCUSED);
	lv_obj_set_style_outline_color(phandle->start_btn, lv_color_hex(0xD9830F), LV_STATE_FOCUS_KEY);
	lv_label_set_text(phandle->start_label, divoom_menu_control_get_disp_str("Stop"));

	lv_label_set_text(phandle->lap_label, divoom_menu_control_get_disp_str("Lap"));
}

void divoom_lvgl_stop_watch_menu_reset_status(Pdivoom_lvgl_stop_watch_menu_ctrl phandle)
{
	divoom_lvgl_stop_watch_menu_reset_lap(phandle);
	
	lv_obj_set_style_bg_color(phandle->start_btn, lv_color_hex(0x99D999), 0);
	lv_obj_set_style_bg_color(phandle->start_btn, lv_color_hex(0x6BD96B), LV_STATE_FOCUSED);
	lv_obj_set_style_bg_color(phandle->start_btn, lv_color_hex(0x6BD96B), LV_STATE_FOCUS_KEY);
	
	//lv_obj_set_style_border_color(phandle->start_obj, lv_color_hex(0x122914), 0);
	//lv_obj_set_style_text_color(phandle->start_label, lv_color_hex(0x67CE67), 0);
	//lv_label_set_text(phandle->start_label, divoom_menu_control_get_disp_str("Start"));
	lv_obj_set_style_text_color(phandle->start_btn, lv_color_hex(0x4E8E4E), 0);
	lv_obj_set_style_text_color(phandle->start_btn, lv_color_hex(0x032C03), LV_STATE_FOCUSED);
	lv_obj_set_style_text_color(phandle->start_btn, lv_color_hex(0x032C03), LV_STATE_FOCUS_KEY);
	lv_obj_set_style_outline_color(phandle->start_btn, lv_color_hex(0x99D999), 0);
	lv_obj_set_style_outline_color(phandle->start_btn, lv_color_hex(0x00C700), LV_STATE_FOCUSED);
	lv_obj_set_style_outline_color(phandle->start_btn, lv_color_hex(0x00C700), LV_STATE_FOCUS_KEY);
	lv_label_set_text(phandle->start_label, divoom_menu_control_get_disp_str("Start"));
	
	lv_label_set_text(phandle->lap_label, divoom_menu_control_get_disp_str("Lap"));

}

void divoom_lvgl_stop_watch_menu_event_cb(lv_event_t * e)
{
	int active_id;
	Pdivoom_lvgl_stop_watch_menu_ctrl phandle = (Pdivoom_lvgl_stop_watch_menu_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *cur_obj = lv_event_get_target(e);

	/*switch(code)
	{
		case LV_EVENT_CLICKED:
			if(cur_obj == phandle->back_btn)
			{
				divoom_app_tool_set_watch_lap_nums(&phandle->lap_time[0], phandle->lap_num);
				divoom_menu_control_return_pre();
			}
			else if(cur_obj == phandle->lap_btn)
			{
				if(phandle->status == 1)
				{
					divoom_lvgl_stop_watch_menu_add_lap(phandle);
				}
				else if(phandle->status == 0)
				{
					phandle->status = 2;
					divoom_lvgl_stop_watch_menu_ctrl_time(2);
					divoom_lvgl_stop_watch_menu_reset_status(phandle);
				}
			}
			else if(cur_obj == phandle->start_btn)
			{
				
				if (phandle->status == 2 || phandle->status == 0)
				{
					phandle->status = 1;
					divoom_lvgl_stop_watch_menu_ctrl_time(1);
					divoom_lvgl_stop_watch_menu_stop_status(phandle);
				}
				else
				{
					phandle->status = 0;
					divoom_lvgl_stop_watch_menu_ctrl_time(0);
					divoom_lvgl_stop_watch_menu_start_status(phandle);
				}
			}
			break;
        default:
            break;
	}*/
	if(code == LV_EVENT_KEY)
	{
		char c = *((char *)lv_event_get_param(e));

		if(c == DIVOOM_LVGL_KEY_PRE)
		{
			cur_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_obj, DIVOOM_MENU_DIRECT_LEFT);                            
            lv_group_focus_obj(cur_obj);
		}
		else if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			cur_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_obj, DIVOOM_MENU_DIRECT_RIGHT);                            
            lv_group_focus_obj(cur_obj);
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			active_id = divoom_menu_direct_ctrl_get_pos(phandle->pdirect_handle, cur_obj);
			
			if(active_id == 0)
			{
				if(phandle->status == 1)
				{
					divoom_lvgl_stop_watch_menu_add_lap(phandle);
				}
				else if(phandle->status == 0)
				{
					phandle->status = 2;
					divoom_lvgl_stop_watch_menu_ctrl_time(2);
					divoom_lvgl_stop_watch_menu_reset_status(phandle);
				}
			}
			else if(active_id == 1)
			{
				if (phandle->status == 2 || phandle->status == 0)
				{
					phandle->status = 1;
					divoom_lvgl_stop_watch_menu_ctrl_time(1);
					divoom_lvgl_stop_watch_menu_stop_status(phandle);
				}
				else
				{
					phandle->status = 0;
					divoom_lvgl_stop_watch_menu_ctrl_time(0);
					divoom_lvgl_stop_watch_menu_start_status(phandle);
				}
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_app_tool_set_watch_lap_nums(&phandle->lap_time[0], phandle->lap_num);
			divoom_menu_control_return_pre();
		}
		else
		{
			char *key_info = (char *)lv_event_get_param(e);
			divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
            return;
        }
	}
}

void divoom_lvgl_stop_watch_menu_box_event_cb(lv_event_t * e)
{
	Pdivoom_lvgl_stop_watch_menu_ctrl phandle = (Pdivoom_lvgl_stop_watch_menu_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *cur_obj = lv_event_get_target(e);

	if(code == LV_EVENT_KEY)
	{		
		char c = *((char *)lv_event_get_param(e));

		if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			if(phandle->bg_img_flag == 1)
			{
				phandle->bg_img_flag = 0;
			}
			
			lv_obj_add_flag(phandle->bg_img, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(phandle->box_img, LV_OBJ_FLAG_HIDDEN);
			lv_group_focus_obj(phandle->lap_btn);
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			if(phandle->bg_img_flag == 1)
			{
				phandle->bg_img_flag = 0;
			}
			lv_obj_add_flag(phandle->bg_img, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(phandle->box_img, LV_OBJ_FLAG_HIDDEN);
			lv_group_focus_obj(phandle->lap_btn);
		}
	}
}


void divoom_lvgl_stop_watch_remind_box(Pdivoom_lvgl_stop_watch_menu_ctrl phandle)
{
	lv_obj_t *bg_img = lv_img_create(phandle->main_obj);
	divoom_menu_control_lv_img_set_src(bg_img, "28/28_bg_cover.png");
	lv_obj_set_pos(bg_img, 0, 0);
	phandle->bg_img = bg_img;

	lv_obj_t *box_img = lv_img_create(phandle->main_obj);
	divoom_menu_control_lv_img_set_src(box_img, "28/28_box.png");
	lv_obj_set_pos(box_img, 80, 436);
	phandle->box_img = box_img;

	lv_obj_t *title_label = lv_label_create(box_img);
	lv_obj_set_pos(title_label, 0, 23+5);
	lv_obj_set_size(title_label, 640, 44);
	lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_color(title_label, lv_color_hex(0x000000), 0);
	lv_obj_set_style_text_font(title_label, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_label_set_text(title_label, divoom_menu_control_get_disp_str("StopWatch"));
	
	lv_obj_t *text_label = lv_label_create(box_img);
	lv_obj_set_pos(text_label, 20, 90+50);
	lv_obj_set_size(text_label, 600, 180-50);
	lv_obj_set_style_text_align(text_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_color(text_label, lv_color_hex(0xFF4337), 0);
	lv_obj_set_style_text_font(text_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
	lv_label_set_text(text_label, divoom_menu_control_get_disp_str("StopWatchLimit"));

	lv_obj_t *btn = lv_btn_create(box_img);
	lv_obj_remove_style_all(btn);
	lv_obj_set_style_border_width(btn, 4, 0);
	lv_obj_set_style_border_color(btn, lv_color_hex(0x6D6D6D), 0);
	lv_obj_set_style_bg_color(btn, lv_color_hex(0xF2F2F2), 0);
	lv_obj_set_style_radius(btn, 99, 0);
	lv_obj_set_style_pad_all(btn, 0, 0);
	lv_obj_set_pos(btn, 179, 294);
	lv_obj_set_size(btn, 282, 90);
	lv_group_focus_obj(btn);
	lv_obj_add_event_cb(btn, divoom_lvgl_stop_watch_menu_box_event_cb, LV_EVENT_KEY, phandle);
	
	lv_obj_t *btn_label = lv_label_create(btn);
	lv_obj_set_pos(btn_label, 0, 23);
	lv_obj_set_size(btn_label, 282, 44);
	lv_obj_set_style_text_align(btn_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_color(btn_label, lv_color_hex(0x000000), 0);
	lv_obj_set_style_text_font(btn_label, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_label_set_text(btn_label, divoom_menu_control_get_disp_str("Ok"));
}

void divoom_lvgl_stop_watch_menu_refresh(struct _lv_timer_t * tool_refresh_time)
{
	static int sec_time = 0, millisecond = 0;
	struct tm p_now;
	divoom_disp_ctrl_watch_ctrl watch_info;
	Pdivoom_lvgl_stop_watch_menu_ctrl phandle = (Pdivoom_lvgl_stop_watch_menu_ctrl)lv_timer_get_user_data(tool_refresh_time);
	
	divoom_app_tool_get_watch_cnt(&watch_info);
	sec_time = watch_info.watch_disp_value/100;
	millisecond = watch_info.watch_disp_value%100;

	if (sec_time/60/10 > 9)
	{
		lv_table_set_cell_value_fmt(phandle->time_obj, 0, 0, "%d", 9);
		lv_table_set_cell_value_fmt(phandle->time_obj, 0, 1, "%d", 9);
		lv_table_set_cell_value_fmt(phandle->time_obj2, 0, 0, "%d", 9);
		lv_table_set_cell_value_fmt(phandle->time_obj2, 0, 1, "%d", 9);
		lv_table_set_cell_value_fmt(phandle->time_obj3, 0, 0, "%d", 9);
		lv_table_set_cell_value_fmt(phandle->time_obj3, 0, 1, "%d", 9);

		if(phandle->bg_img_flag == 0 && phandle->disp_times == 0)
		{
			phandle->disp_times = 1;
			phandle->bg_img_flag = 1;//上限提醒页面
			divoom_lvgl_stop_watch_remind_box(phandle);
		}
	}
	else
	{
		lv_table_set_cell_value_fmt(phandle->time_obj, 0, 0, "%d", sec_time/60/10);
		lv_table_set_cell_value_fmt(phandle->time_obj, 0, 1, "%d", sec_time/60%10);
		lv_table_set_cell_value_fmt(phandle->time_obj2, 0, 0, "%d", sec_time%60/10);
		lv_table_set_cell_value_fmt(phandle->time_obj2, 0, 1, "%d", sec_time%60%10);
		lv_table_set_cell_value_fmt(phandle->time_obj3, 0, 0, "%d", millisecond/10%10);
		lv_table_set_cell_value_fmt(phandle->time_obj3, 0, 1, "%d", millisecond%10);
	}
	
	

	if (phandle->status != watch_info.ctrl_flag)
	{
		phandle->status = watch_info.ctrl_flag;
		if (phandle->status == 1)// 0:停止	1:启动	 2:取消
		{
			divoom_lvgl_stop_watch_menu_stop_status(phandle);
		}
		else if(phandle->status == 0)
		{
			divoom_lvgl_stop_watch_menu_start_status(phandle);
		}
		else
		{
			divoom_lvgl_stop_watch_menu_reset_status(phandle);
		}
	}
	
}

void divoom_lvgl_stop_watch_menu_create_title_area(Pdivoom_lvgl_stop_watch_menu_ctrl phandle)
{
	int hour;
	char disp_str[32];
	time_t time_now;
	struct tm p_now;
	
	lv_obj_t *title = lv_obj_create(phandle->main_obj);
	lv_obj_set_style_bg_color(title, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_border_width(title, 0, 0);
	lv_obj_set_style_pad_all(title, 0, 0);
	lv_obj_set_style_text_color(title, lv_color_black(), 0);
	lv_obj_clear_flag(title, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(title, 800, 78); 
	lv_obj_set_pos(title, 0, 40);

	/*lv_obj_t *back_btn = lv_btn_create(title);
	lv_obj_set_size(back_btn, 48, 48); 
	lv_obj_set_pos(back_btn, 25, 15);
	lv_obj_set_style_bg_opa(back_btn, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(back_btn, 0, 0);
	lv_obj_set_style_shadow_width(back_btn, 0, 0);
	lv_obj_clear_flag(back_btn, LV_OBJ_FLAG_SCROLLABLE);
	phandle->back_btn = back_btn;
	lv_obj_add_event(back_btn, divoom_lvgl_stop_watch_menu_event_cb, LV_EVENT_CLICKED, phandle);
	
	lv_obj_t *image_obj = lv_img_create(back_btn);
    lv_obj_set_size(image_obj, 48, 48);
    divoom_menu_control_lv_img_set_src(image_obj, "28/28_back.png");*/

	lv_obj_t *stop_watch_label = lv_label_create(title);
	lv_obj_set_size(stop_watch_label, 200, 48); 
	lv_obj_set_style_text_font(stop_watch_label, divoom_menu_control_get_font_info(48, phandle->menu_id), 0);
	lv_label_set_text(stop_watch_label, divoom_menu_control_get_disp_str("StopWatch"));
	lv_obj_set_pos(stop_watch_label, 350, 15);

	lv_obj_t *time_label = lv_label_create(title);
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
	phandle->time_label_obj = time_label;
	phandle->menu_bar_timer = lv_timer_create(divoom_lvgl_stop_watch_menu_refresh_menu_bar, 1000, phandle);
}

void divoom_lvgl_stop_watch_menu_create_time_area(Pdivoom_lvgl_stop_watch_menu_ctrl phandle)
{
	int sec_time, millisecond, i;
	divoom_disp_ctrl_watch_ctrl watch_info = {0};
	phandle->lap_num = 0;
	
	//minute
	phandle->time_obj = lv_table_create(phandle->main_obj);
	lv_obj_remove_style_all(phandle->time_obj);

	lv_obj_set_x(phandle->time_obj, 80);
	lv_obj_set_y(phandle->time_obj, 322);
	lv_obj_set_width(phandle->time_obj, 180);
	lv_obj_set_height(phandle->time_obj, 181);
    lv_table_set_row_cnt(phandle->time_obj, 1); //1行
    lv_table_set_col_cnt(phandle->time_obj, 2); //2列

	lv_style_init(&phandle->time_obj_style);
	lv_style_set_text_font(&phandle->time_obj_style, divoom_menu_control_get_font_info(140, phandle->menu_id));//divoom_lvgl_font_40_ptr
	lv_style_set_text_color(&phandle->time_obj_style, lv_color_hex(0x333333));
	lv_style_set_text_align(&phandle->time_obj_style, LV_ALIGN_CENTER);

	lv_table_set_cell_value(phandle->time_obj, 0, 0, "0");
	lv_table_set_cell_value(phandle->time_obj, 0, 1, "0");
	lv_table_set_col_width(phandle->time_obj, 0, 90); //设置单元格宽度
	lv_table_set_col_width(phandle->time_obj, 1, 90);
    lv_obj_add_style(phandle->time_obj, &phandle->time_obj_style, 0);
	lv_obj_clear_flag(phandle->time_obj, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *label = lv_label_create(phandle->main_obj);
    lv_obj_set_x(label, 260);
	lv_obj_set_y(label, 322);
	lv_obj_set_width(label, 50);
	lv_obj_set_height(label, 181);
	lv_obj_add_style(label, &phandle->time_obj_style, 0);
    lv_label_set_text(label, ":");

	//second
	phandle->time_obj2 = lv_table_create(phandle->main_obj);
	lv_obj_remove_style_all(phandle->time_obj2);

	lv_obj_set_x(phandle->time_obj2, 310);
	lv_obj_set_y(phandle->time_obj2, 322);
	lv_obj_set_width(phandle->time_obj2, 180);
	lv_obj_set_height(phandle->time_obj2, 181);
    lv_table_set_row_cnt(phandle->time_obj2, 1); //1行
    lv_table_set_col_cnt(phandle->time_obj2, 2); //2列

	lv_table_set_cell_value(phandle->time_obj2, 0, 0, "0");
	lv_table_set_cell_value(phandle->time_obj2, 0, 1, "0");
	lv_table_set_col_width(phandle->time_obj2, 0, 90); //设置单元格宽度
	lv_table_set_col_width(phandle->time_obj2, 1, 90);
    lv_obj_add_style(phandle->time_obj2, &phandle->time_obj_style, 0);
	lv_obj_clear_flag(phandle->time_obj2, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *label2 = lv_label_create(phandle->main_obj);
    lv_obj_set_x(label2, 490);
	lv_obj_set_y(label2, 322);
	lv_obj_set_width(label2, 50);
	lv_obj_set_height(label2, 181);
	lv_obj_add_style(label2, &phandle->time_obj_style, 0);
    lv_label_set_text(label2, ".");
	
	//ms
	phandle->time_obj3 = lv_table_create(phandle->main_obj);
	lv_obj_remove_style_all(phandle->time_obj3);

	lv_obj_set_x(phandle->time_obj3, 540);
	lv_obj_set_y(phandle->time_obj3, 322);
	lv_obj_set_width(phandle->time_obj3, 180);
	lv_obj_set_height(phandle->time_obj3, 181);
    lv_table_set_row_cnt(phandle->time_obj3, 1); //1行
    lv_table_set_col_cnt(phandle->time_obj3, 2); //2列

	lv_table_set_cell_value(phandle->time_obj3, 0, 0, "0");
	lv_table_set_cell_value(phandle->time_obj3, 0, 1, "0");
	lv_table_set_col_width(phandle->time_obj3, 0, 90); //设置单元格宽度
	lv_table_set_col_width(phandle->time_obj3, 1, 90);
    lv_obj_add_style(phandle->time_obj3, &phandle->time_obj_style, 0);
	lv_obj_clear_flag(phandle->time_obj3, LV_OBJ_FLAG_SCROLLABLE);

	divoom_app_tool_get_watch_cnt(&watch_info);
	sec_time = watch_info.watch_disp_value/100;
	millisecond = watch_info.watch_disp_value%100;
    phandle->status = watch_info.ctrl_flag;

	if (sec_time/60/10 > 9)
	{
		lv_table_set_cell_value_fmt(phandle->time_obj, 0, 0, "%d", 9);
		lv_table_set_cell_value_fmt(phandle->time_obj, 0, 1, "%d", 9);
		lv_table_set_cell_value_fmt(phandle->time_obj2, 0, 0, "%d", 9);
		lv_table_set_cell_value_fmt(phandle->time_obj2, 0, 1, "%d", 9);
		lv_table_set_cell_value_fmt(phandle->time_obj3, 0, 0, "%d", 9);
		lv_table_set_cell_value_fmt(phandle->time_obj3, 0, 1, "%d", 9);
	}
	else
	{
		lv_table_set_cell_value_fmt(phandle->time_obj, 0, 0, "%d", sec_time/60/10);
		lv_table_set_cell_value_fmt(phandle->time_obj, 0, 1, "%d", sec_time/60%10);
		lv_table_set_cell_value_fmt(phandle->time_obj2, 0, 0, "%d", sec_time%60/10);
		lv_table_set_cell_value_fmt(phandle->time_obj2, 0, 1, "%d", sec_time%60%10);
		lv_table_set_cell_value_fmt(phandle->time_obj3, 0, 0, "%d", millisecond/10%10);
		lv_table_set_cell_value_fmt(phandle->time_obj3, 0, 1, "%d", millisecond%10);
	}
	
	
	
	//Lap
	//lv_obj_t *lap_obj = lv_obj_create(phandle->main_obj);
	//lv_obj_set_x(lap_obj, 42);
	//lv_obj_set_y(lap_obj, 691);
	//lv_obj_set_width(lap_obj, 191);
	//lv_obj_set_height(lap_obj, 191);
	//lv_obj_set_style_pad_all(lap_obj, 0, 0);
	//lv_obj_set_style_border_width(lap_obj, 5, 0);
	//lv_obj_set_style_border_color(lap_obj, lv_color_hex(0x1C1C1D), 0);
	//lv_obj_set_style_radius(lap_obj, LV_RADIUS_CIRCLE, 0);
	//lv_obj_set_style_bg_color(lap_obj, lv_color_hex(0x000000), 0);
	//lv_obj_set_style_bg_opa(lap_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	//lv_obj_set_style_bg_opa(lap_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	//lv_obj_set_style_shadow_width(lap_obj, 0, 0); //去掉边框底部的阴影
	
	//lv_obj_t *lap_btn = lv_btn_create(lap_obj);
	lv_obj_t *lap_btn = lv_btn_create(phandle->main_obj);
	lv_obj_remove_style_all(lap_btn);
	lv_obj_set_width(lap_btn, 171);
	lv_obj_set_height(lap_btn, 171);
	lv_obj_set_pos(lap_btn, 50, 695);
	lv_obj_set_style_pad_all(lap_btn, 0, 0);
	lv_obj_set_style_border_width(lap_btn, 5, 0);
	//lv_obj_set_style_border_opa(lap_btn, LV_OPA_0, 0);
	lv_obj_set_style_border_color(lap_btn, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_radius(lap_btn, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_bg_color(lap_btn, lv_color_hex(0xCECECE), 0);
	lv_obj_set_style_bg_color(lap_btn, lv_color_hex(0xCECECE), LV_STATE_FOCUSED);
	lv_obj_set_style_bg_color(lap_btn, lv_color_hex(0xCECECE), LV_STATE_FOCUS_KEY);
	lv_obj_set_style_bg_opa(lap_btn, LV_OPA_100, 0);
	lv_obj_set_style_bg_opa(lap_btn, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	lv_obj_set_style_bg_opa(lap_btn, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_shadow_width(lap_btn, 0, 0); //去掉边框底部的阴影
	lv_obj_set_style_outline_width(lap_btn, 5, 0);
	lv_obj_set_style_outline_color(lap_btn, lv_color_hex(0xCECECE), 0);
	lv_obj_set_style_outline_color(lap_btn, lv_color_hex(0x0), LV_STATE_FOCUSED);
	lv_obj_set_style_outline_color(lap_btn, lv_color_hex(0x0), LV_STATE_FOCUS_KEY);
	lv_obj_set_style_text_color(lap_btn, lv_color_hex(0x808080), 0);
	lv_obj_set_style_text_color(lap_btn, lv_color_hex(0x0), LV_STATE_FOCUSED);
	lv_obj_set_style_text_color(lap_btn, lv_color_hex(0x0), LV_STATE_FOCUS_KEY);
	//lv_obj_add_event_cb(lap_btn, divoom_lvgl_stop_watch_menu_event_cb, LV_EVENT_CLICKED, phandle);
	lv_obj_add_event_cb(lap_btn, divoom_lvgl_stop_watch_menu_event_cb, LV_EVENT_KEY, phandle);
	phandle->lap_btn = lap_btn;
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, lap_btn, 0, 0);
	
	lv_obj_t *lap_label = lv_label_create(lap_btn);
	//lv_obj_set_style_text_color(lap_label, lv_color_hex(0x5A5A5E), 0);
	lv_obj_set_style_text_font(lap_label, divoom_menu_control_get_font_info(37, phandle->menu_id), 0);
	lv_label_set_text(lap_label, divoom_menu_control_get_disp_str("Lap"));
	lv_obj_center(lap_label);
	phandle->lap_label = lap_label;

	//start
	//lv_obj_t *start_obj = lv_obj_create(phandle->main_obj);
	//lv_obj_set_x(start_obj, 577);
	//lv_obj_set_y(start_obj, 691);
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
	//phandle->start_obj = start_obj;

	//phandle->start_btn = lv_btn_create(start_obj);
	phandle->start_btn = lv_btn_create(phandle->main_obj);
	lv_obj_remove_style_all(phandle->start_btn);
	lv_obj_set_width(phandle->start_btn, 171);
	lv_obj_set_height(phandle->start_btn, 171);
	lv_obj_set_pos(phandle->start_btn, 578, 695);
	lv_obj_set_style_pad_all(phandle->start_btn, 0, 0);
	lv_obj_set_style_border_width(phandle->start_btn, 5, 0);
	lv_obj_set_style_border_color(phandle->start_btn, lv_color_hex(0xE4E4E4), 0);
	//lv_obj_set_style_border_opa(phandle->start_btn, LV_OPA_0, 0);
	lv_obj_set_style_radius(phandle->start_btn, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_bg_color(phandle->start_btn, lv_color_hex(0x99D999), 0);
	lv_obj_set_style_bg_opa(phandle->start_btn, LV_OPA_100, 0);
	lv_obj_set_style_bg_opa(phandle->start_btn, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	lv_obj_set_style_bg_opa(phandle->start_btn, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_shadow_width(phandle->start_btn, 0, 0); //去掉边框底部的阴影
	lv_obj_set_style_outline_width(phandle->start_btn, 5, 0);
	//lv_obj_add_event_cb(phandle->start_btn, divoom_lvgl_stop_watch_menu_event_cb, LV_EVENT_CLICKED, phandle);
	lv_obj_add_event_cb(phandle->start_btn, divoom_lvgl_stop_watch_menu_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, phandle->start_btn, 1, 0);
	if(phandle->first_active_obj == NULL)
	{
		phandle->first_active_obj = phandle->start_btn;
	}
	
	lv_obj_t *start_label = lv_label_create(phandle->start_btn);
	//lv_obj_set_style_text_color(start_label, lv_color_hex(0x67CE67), 0);
	lv_obj_set_style_text_font(start_label, divoom_menu_control_get_font_info(37, phandle->menu_id), 0);
	lv_label_set_text(start_label, divoom_menu_control_get_disp_str("Start"));
	lv_obj_center(start_label);
	phandle->start_label = start_label;

	//lap time
	lv_obj_t *lap_cont = lv_obj_create(phandle->main_obj);
	lv_obj_set_style_pad_all(lap_cont, 0, 0);
	lv_obj_set_style_border_width(lap_cont, 0, 0);
	lv_obj_set_style_bg_opa(lap_cont, LV_OPA_0, 0);
	lv_obj_set_style_bg_opa(lap_cont, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	lv_obj_set_style_bg_opa(lap_cont, LV_OPA_100, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_size(lap_cont, 726+37, 347-30);
	lv_obj_set_pos(lap_cont, 37, 933);
	//lv_obj_set_flex_flow(lap_cont, LV_FLEX_FLOW_COLUMN_REVERSE);
	lv_obj_set_flex_flow(lap_cont, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_row(lap_cont, 0, 0);
	phandle->lap_cont = lap_cont;

	if (phandle->status == 1)// 0:停止	1:启动	 2:取消
	{
		divoom_lvgl_stop_watch_menu_stop_status(phandle);
	}
	else if(phandle->status == 0)
	{
		divoom_lvgl_stop_watch_menu_start_status(phandle);
	}
	else
	{
		divoom_lvgl_stop_watch_menu_reset_status(phandle);
	}

	divoom_app_tool_get_watch_lap_nums(&phandle->lap_num, &phandle->lap_time[0]);
	divoom_lvgl_stop_watch_menu_create_laps(phandle);
	
	phandle->tool_refresh_timer = lv_timer_create(divoom_lvgl_stop_watch_menu_refresh, 80, phandle);
}

void *divoom_lvgl_stop_watch_menu_init(void *user_data, int menu_id)
{
	Pdivoom_lvgl_stop_watch_menu_ctrl phandle = divoom_system_malloc(sizeof(divoom_lvgl_stop_watch_menu_ctrl));
    
	phandle->pdirect_handle = divoom_menu_direct_ctrl_create(102, 1);
	phandle->main_obj = lv_obj_create(NULL);
	phandle->bg_img_flag = 0;
	phandle->disp_times = 0;
	phandle->first_active_obj = NULL;
    phandle->menu_id = menu_id;
	lv_style_init(&phandle->backgroud_type);
    lv_style_set_bg_color(&phandle->backgroud_type, lv_color_hex(0xE4E4E4));
    lv_style_set_text_color(&phandle->backgroud_type, lv_color_hex(0x0));
    lv_obj_add_style(phandle->main_obj, &phandle->backgroud_type, 0);
	lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);

	divoom_lvgl_stop_watch_menu_create_title_area(phandle);
	divoom_lvgl_stop_watch_menu_create_time_area(phandle);

	return phandle;
}

void divoom_lvgl_stop_watch_menu_disp(void *handle, void *active)
{
	Pdivoom_lvgl_stop_watch_menu_ctrl phandle = (Pdivoom_lvgl_stop_watch_menu_ctrl)handle;

	if(phandle->first_active_obj)
	{
		lv_group_focus_obj(phandle->first_active_obj);
		phandle->first_active_obj = NULL;
	}
	if(active)
	{
		lv_group_focus_obj(active);
	}
	
	lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_stop_watch_menu_exit(void *handle)
{
	Pdivoom_lvgl_stop_watch_menu_ctrl phandle = (Pdivoom_lvgl_stop_watch_menu_ctrl)handle;
	
	lv_style_reset(&phandle->backgroud_type);
	lv_style_reset(&phandle->time_obj_style);
	lv_style_reset(&phandle->btn_normal_style);
	lv_style_reset(&phandle->btn_select_style);
	divoom_menu_direct_ctrl_destory(phandle->pdirect_handle);
	lv_timer_del(phandle->tool_refresh_timer);
	lv_timer_del(phandle->menu_bar_timer);
	lv_obj_del(phandle->main_obj);
	divoom_system_free(phandle);
}


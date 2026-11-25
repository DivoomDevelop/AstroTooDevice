#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"

typedef struct _divoom_lvgl_setting_power_off_ctrl
{
    int menu_id;
	lv_style_t backgroud_type;
	void *main_obj;
	//void *time_cont;
	int am_pm_index;

	lv_style_t btn_normal_style;
	lv_style_t btn_edit_style;
	lv_style_t btn_select_style;

	//void *back_btn;
	void *first_active_obj;
	void *power_on_off_btn;		//定时开关
	void *switch_img;
	
	void *start_am_pm_btn;
	void *start_am_pm_label;
	void *start_hour_btn;
	void *start_hour_label;
	void *start_minute_btn;
	void *start_minute_label;

	void *end_am_pm_btn;
	void *end_am_pm_label;
	void *end_hour_btn;
	void *end_hour_label;
	void *end_minute_btn;
	void *end_minute_label;
	//void *check_btn;
	//void *cancel_btn;

	int btn_status;
	void *pdirect_handle;
	Pdivoom_device_on_off_config_para pon_off_config;
	int power_on_off;
	int power_on_minute;			//结束时间
	int power_on_hour;			
	int power_on_am_pm_index;
	int power_off_minute;			//开始时间
	int power_off_hour;			
	int power_off_am_pm_index;
}divoom_lvgl_setting_power_off_ctrl, *Pdivoom_lvgl_setting_power_off_ctrl;

/*void divoom_lvgl_setting_power_off_set_stream_cont_time(Pdivoom_lvgl_setting_power_off_ctrl phandle, int flag)
{
	if(flag == 0)//开始时间
	{
		if(divoom_main_get_time_flag() == 0)
		{
			int hour = phandle->power_off_hour;
        	if(hour != 12)
        	{
           		hour = hour % 12;
           		if(hour == 0)
           		{
            	   	hour = 12;
           		}
       		}
			if(phandle->power_off_hour > 12)
			{
				phandle->power_off_am_pm_index = 1;
				divoom_lvgl_setting_power_off_scroll(phandle, -120);
				lv_obj_scroll_to_y(phandle->roller, 120, LV_ANIM_OFF);
				divoom_stream_cont_set_current_selected(phandle->stream_cont_hour, hour);
			}
			else
			{
				phandle->power_off_am_pm_index = 0;
				divoom_lvgl_setting_power_off_scroll(phandle, 0);
				lv_obj_scroll_to_y(phandle->roller, 0, LV_ANIM_OFF);
				divoom_stream_cont_set_current_selected(phandle->stream_cont_hour, hour);
			}
			divoom_stream_cont_set_current_selected(phandle->stream_cont_minute, phandle->power_off_minute);
		}
		else
		{
			divoom_stream_cont_set_current_selected(phandle->stream_cont_hour, phandle->power_off_hour);
			divoom_stream_cont_set_current_selected(phandle->stream_cont_minute, phandle->power_off_minute);
		}
	}
	else if(flag == 1)//结束时间
	{
		if(divoom_main_get_time_flag() == 0)
		{
			int hour = phandle->power_on_hour;
        	if(hour != 12)
        	{
           		hour = hour % 12;
           		if(hour == 0)
           		{
            	   	hour = 12;
           		}
       		}

			if(phandle->power_on_hour > 12)
			{
				phandle->power_on_am_pm_index = 1;
				divoom_lvgl_setting_power_off_scroll(phandle, -120);
				lv_obj_scroll_to_y(phandle->roller, 120, LV_ANIM_OFF);
				divoom_stream_cont_set_current_selected(phandle->stream_cont_hour, hour);
			}
			else
			{
				phandle->power_on_am_pm_index = 0;
				divoom_lvgl_setting_power_off_scroll(phandle, 0);
				lv_obj_scroll_to_y(phandle->roller, 0, LV_ANIM_OFF);
				divoom_stream_cont_set_current_selected(phandle->stream_cont_hour, hour);
			}
			divoom_stream_cont_set_current_selected(phandle->stream_cont_minute, phandle->power_on_minute);
		}
		else
		{
			divoom_stream_cont_set_current_selected(phandle->stream_cont_hour, phandle->power_on_hour);
			divoom_stream_cont_set_current_selected(phandle->stream_cont_minute, phandle->power_on_minute);
		}
	}
}*/

void divoom_lvgl_setting_power_off_update_time(Pdivoom_lvgl_setting_power_off_ctrl phandle)
{
	divoom_sys_msg_info model_msg = {0};
	Pdivoom_device_on_off_config_para pon_off_config;
	divoom_device_on_off_config_para power_on_off_config;

	if(divoom_main_get_time_flag() == 0)	
	{
		if(phandle->power_off_am_pm_index)
		{
			phandle->power_off_hour = phandle->power_off_hour + 12;
		}
		if(phandle->power_on_am_pm_index)
		{
			phandle->power_on_hour = phandle->power_on_hour + 12;
		}
		
	}

	pon_off_config = (Pdivoom_device_on_off_config_para)divoom_system_malloc(sizeof(divoom_device_on_off_config_para));
	pon_off_config->start = phandle->power_off_hour*60 + phandle->power_off_minute;
	pon_off_config->end = phandle->power_on_hour*60 + phandle->power_on_minute;
	pon_off_config->on_off = phandle->power_on_off;
	//divoom_print("power_off_hour = %d, power_off_minute = %d, power_on_hour = %d, power_on_minute = %d\n", phandle->power_off_hour,phandle->power_off_minute,phandle->power_on_hour,phandle->power_on_minute);	
	
	//divoom_print("start = %d, end = %d, on_off = %d\n", pon_off_config->start,pon_off_config->end,pon_off_config->on_off);	
	model_msg.src_id = DIVOOM_MODEL_APP_TOOL;
	model_msg.dest_id = DIVOOM_MODEL_MAIN;
	model_msg.msg_type = 1;
	model_msg.msg_id = DIVOOM_MAIN_NOTICE_UPDATE_ON_OFF_TIME;
	model_msg.data = (uint8 *)pon_off_config;
	divoom_sys_model_crl_msg_push(&model_msg);

	power_on_off_config.on_off = pon_off_config->on_off;
	power_on_off_config.start = pon_off_config->start;
	power_on_off_config.end = pon_off_config->end;
	divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_SET_ON_OFF_MODE, DIVOOM_SERVER_TOPIC, &power_on_off_config);
}

void divoom_lvgl_setting_power_off_event_cb(lv_event_t * e)
{
	Pdivoom_lvgl_setting_power_off_ctrl phandle = (Pdivoom_lvgl_setting_power_off_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *cur_obj = lv_event_get_target(e);

	/*switch(code)
	{
		case LV_EVENT_CLICKED:
			if(phandle->back_btn == cur_obj)
			{
				divoom_lvgl_setting_power_off_update_time(phandle);
				divoom_menu_control_return_pre();
			}
			else if(phandle->switch_btn == cur_obj)
			{
				if(phandle->power_on_off == 1)
				{
					phandle->power_on_off = 0;
					divoom_menu_control_lv_img_set_src(phandle->switch_img, "62/62_button_off.png");
				}
				else
				{
					phandle->power_on_off = 1;
					divoom_menu_control_lv_img_set_src(phandle->switch_img, "62/62_button_on.png");
				}
			}
			else if(phandle->start_time_btn == cur_obj)
			{
				phandle->seleted_id = 1;
				divoom_lvgl_setting_power_off_set_stream_cont_time(phandle, 0);
				lv_obj_clear_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);
				lv_obj_clear_flag(phandle->time_cont, LV_OBJ_FLAG_HIDDEN);
			}
			else if(phandle->end_time_btn == cur_obj)
			{
				phandle->seleted_id = 2;
				divoom_lvgl_setting_power_off_set_stream_cont_time(phandle, 1);
				lv_obj_clear_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);
				lv_obj_clear_flag(phandle->time_cont, LV_OBJ_FLAG_HIDDEN);
			}
			else if(phandle->check_btn == cur_obj)
			{
				divoom_stream_cont_pause_cont(phandle->stream_cont_hour);
				divoom_stream_cont_pause_cont(phandle->stream_cont_minute);
				
				divoom_lvgl_setting_power_off_set_label_time(phandle);
				
				lv_obj_add_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);
				lv_obj_add_flag(phandle->time_cont, LV_OBJ_FLAG_HIDDEN);
			}
			else if(phandle->cancel_btn == cur_obj)
			{
				divoom_stream_cont_pause_cont(phandle->stream_cont_hour);
				divoom_stream_cont_pause_cont(phandle->stream_cont_minute);
				lv_obj_add_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);
				lv_obj_add_flag(phandle->time_cont, LV_OBJ_FLAG_HIDDEN);
			}
			break;

        default:
            break;

	}*/
	if(code == LV_EVENT_KEY)
    {
		Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();
		char c = *((char *)lv_event_get_param(e));
			
		switch(c)
        {
        	case DIVOOM_LVGL_KEY_PRE:
				if(phandle->btn_status == 0)
				{
					lv_group_focus_obj(divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_obj, DIVOOM_MENU_DIRECT_RIGHT));
	 			}
				else
				{
					if(cur_obj == phandle->start_am_pm_btn)
					{
						if(phandle->power_off_am_pm_index)
						{
							phandle->power_off_am_pm_index = 0;
							lv_label_set_text(phandle->start_am_pm_label, divoom_menu_control_get_disp_str("AM"));
						}
						else 
						{
							phandle->power_off_am_pm_index = 1;
							lv_label_set_text(phandle->start_am_pm_label, divoom_menu_control_get_disp_str("PM"));
						}
					}
					else if(cur_obj == phandle->start_hour_btn)
					{
						if(divoom_main_get_time_flag() == 0)
                		{
                    		if(phandle->power_off_hour >= 12)
                    		{
                    	    	phandle->power_off_hour = 1;
                    		}
                    		else
                    		{
                    	    	phandle->power_off_hour ++;
                    		}
                		}
               			else
                		{
                    		if(phandle->power_off_hour >= 23)
                    		{
                    		    phandle->power_off_hour = 0;
                    		}
                    		else
                    		{
                        		phandle->power_off_hour ++;
                    		}
                		}
                		lv_label_set_text_fmt(phandle->start_hour_label, "%02d", phandle->power_off_hour);
					}
					else if(cur_obj == phandle->start_minute_btn)
					{
						if(phandle->power_off_minute >= 59)
                		{
                		    phandle->power_off_minute = 0;
                		}
                		else
                		{
                		    phandle->power_off_minute ++;
                		}
                		lv_label_set_text_fmt(phandle->start_minute_label, "%02d", phandle->power_off_minute);
					}
					else if(cur_obj == phandle->end_am_pm_btn)
					{
						if(phandle->power_on_am_pm_index)
						{
							phandle->power_on_am_pm_index = 0;
							lv_label_set_text(phandle->end_am_pm_label, divoom_menu_control_get_disp_str("AM"));
						}
						else 
						{
							phandle->power_on_am_pm_index = 1;
							lv_label_set_text(phandle->end_am_pm_label, divoom_menu_control_get_disp_str("PM"));
						}
					}
					else if(cur_obj == phandle->end_hour_btn)
					{
						if(divoom_main_get_time_flag() == 0)
                		{
                    		if(phandle->power_on_hour >= 12)
                    		{
                    		    phandle->power_on_hour = 1;
                    		}
                    		else
                    		{
                    		    phandle->power_on_hour ++;
                    		}
                		}
               			else
                		{
                    		if(phandle->power_on_hour >= 23)
                    		{
                    		    phandle->power_on_hour = 0;
                    		}
                    		else
                    		{
                        		phandle->power_on_hour ++;
                    		}
                		}
                		lv_label_set_text_fmt(phandle->end_hour_label, "%02d", phandle->power_on_hour);
					}
					else if(cur_obj == phandle->end_minute_btn)
					{
						if(phandle->power_on_minute >= 59)
                		{
                		    phandle->power_on_minute = 0;
                		}
                		else
                		{
                	  	 	phandle->power_on_minute ++;
                		}
               			lv_label_set_text_fmt(phandle->end_minute_label, "%02d", phandle->power_on_minute);
					}
				}
				break;
			case DIVOOM_LVGL_KEY_NEXT:
				if(phandle->btn_status == 0)
				{
					lv_group_focus_obj(divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_obj, DIVOOM_MENU_DIRECT_LEFT));
	 			}
				else
				{
					if(cur_obj == phandle->start_am_pm_btn)
					{
						if(phandle->power_off_am_pm_index)
						{
							phandle->power_off_am_pm_index = 0;
							lv_label_set_text(phandle->start_am_pm_label, divoom_menu_control_get_disp_str("AM"));
						}
						else 
						{
							phandle->power_off_am_pm_index = 1;
							lv_label_set_text(phandle->start_am_pm_label, divoom_menu_control_get_disp_str("PM"));
						}
					}
					else if(cur_obj == phandle->start_hour_btn)
					{
						if(divoom_main_get_time_flag() == 0)
                		{
                    		if(phandle->power_off_hour <= 1)
                    		{
                    	    	phandle->power_off_hour = 12;
                    		}
                    		else
                    		{
                    	    	phandle->power_off_hour --;
                    		}
                		}
               			else
                		{
                    		if(phandle->power_off_hour <= 0)
                    		{
                    		    phandle->power_off_hour = 23;
                    		}
                    		else
                    		{
                        		phandle->power_off_hour --;
                    		}
                		}
                		lv_label_set_text_fmt(phandle->start_hour_label, "%02d", phandle->power_off_hour);
					}
					else if(cur_obj == phandle->start_minute_btn)
					{
						if(phandle->power_off_minute <= 0)
                		{
                		    phandle->power_off_minute = 59;
                		}
                		else
                		{
                		    phandle->power_off_minute --;
                		}
                		lv_label_set_text_fmt(phandle->start_minute_label, "%02d", phandle->power_off_minute);
					}
					else if(cur_obj == phandle->end_am_pm_btn)
					{
						if(phandle->power_on_am_pm_index)
						{
							phandle->power_on_am_pm_index = 0;
							lv_label_set_text(phandle->end_am_pm_label, divoom_menu_control_get_disp_str("AM"));
						}
						else 
						{
							phandle->power_on_am_pm_index = 1;
							lv_label_set_text(phandle->end_am_pm_label, divoom_menu_control_get_disp_str("PM"));
						}
					}
					else if(cur_obj == phandle->end_hour_btn)
					{
						if(divoom_main_get_time_flag() == 0)
                		{
                    		if(phandle->power_on_hour <= 1)
                    		{
                    		    phandle->power_on_hour = 12;
                    		}
                    		else
                    		{
                    		    phandle->power_on_hour --;
                    		}
                		}
               			else
                		{
                    		if(phandle->power_on_hour <= 0)
                    		{
                    		    phandle->power_on_hour = 23;
                    		}
                    		else
                    		{
                        		phandle->power_on_hour --;
                    		}
                		}
                		lv_label_set_text_fmt(phandle->end_hour_label, "%02d", phandle->power_on_hour);
					}
					else if(cur_obj == phandle->end_minute_btn)
					{
						if(phandle->power_on_minute <= 0)
                		{
                		    phandle->power_on_minute = 59;
                		}
                		else
                		{
                	  	 	phandle->power_on_minute --;
                		}
               			lv_label_set_text_fmt(phandle->end_minute_label, "%02d", phandle->power_on_minute);
					}
				}
	 		break;
		case DIVOOM_LVGL_KEY_ENTER:
			if(phandle->btn_status == 0)
			{
				if(cur_obj == phandle->power_on_off_btn)
				{
					if(phandle->power_on_off == 1)
					{
						phandle->power_on_off = 0;
						divoom_menu_control_lv_img_set_src(phandle->switch_img, "26/26_off.png");
					}
					else
					{
						phandle->power_on_off = 1;
						divoom_menu_control_lv_img_set_src(phandle->switch_img, "26/26_on.png");
					}
				}
				else
				{
					phandle->btn_status = 1;
					lv_obj_add_style(cur_obj, &phandle->btn_edit_style, LV_STATE_FOCUSED);
					lv_obj_add_style(cur_obj, &phandle->btn_edit_style, LV_STATE_FOCUS_KEY);
				}
			}
			else
			{
				if(cur_obj == phandle->start_am_pm_btn)
				{
					if(phandle->power_off_am_pm_index)
					{
						phandle->power_off_am_pm_index = 0;
						lv_label_set_text(phandle->start_am_pm_label, divoom_menu_control_get_disp_str("AM"));
					}
					else 
					{
						phandle->power_off_am_pm_index = 1;
						lv_label_set_text(phandle->start_am_pm_label, divoom_menu_control_get_disp_str("PM"));
					}
				}
				else if(cur_obj == phandle->start_hour_btn)
				{
					if(divoom_main_get_time_flag() == 0)
                	{
                    	if(phandle->power_off_hour >= 12)
                    	{
                        	phandle->power_off_hour = 1;
                    	}
                    	else
                    	{
                        	phandle->power_off_hour ++;
                    	}
                	}
               		else
                	{
                    	if(phandle->power_off_hour >= 23)
                    	{
                    	    phandle->power_off_hour = 0;
                    	}
                    	else
                    	{
                       		phandle->power_off_hour ++;
                   		}
                	}
                	lv_label_set_text_fmt(phandle->start_hour_label, "%02d", phandle->power_off_hour);
				}
				else if(cur_obj == phandle->start_minute_btn)
				{
					if(phandle->power_off_minute >= 59)
                	{
                	    phandle->power_off_minute = 0;
                	}
                	else
                	{
                	    phandle->power_off_minute ++;
                	}
                	lv_label_set_text_fmt(phandle->start_minute_label, "%02d", phandle->power_off_minute);
				}
				else if(cur_obj == phandle->end_am_pm_btn)
				{
					if(phandle->power_on_am_pm_index)
					{
						phandle->power_on_am_pm_index = 0;
						lv_label_set_text(phandle->end_am_pm_label, divoom_menu_control_get_disp_str("AM"));
					}
					else 
					{
						phandle->power_on_am_pm_index = 1;
						lv_label_set_text(phandle->end_am_pm_label, divoom_menu_control_get_disp_str("PM"));
					}
				}
				else if(cur_obj == phandle->end_hour_btn)
				{
					if(divoom_main_get_time_flag() == 0)
                	{
                    	if(phandle->power_on_hour >= 12)
                    	{
                    	    phandle->power_on_hour = 1;
                    	}
                    	else
                    	{
                    	    phandle->power_on_hour ++;
                    	}
                	}
               		else
                	{
                    	if(phandle->power_on_hour >= 23)
                    	{
                    	    phandle->power_on_hour = 0;
                    	}
                    	else
                   		{
                       		phandle->power_on_hour ++;
                   		}
                	}
                	lv_label_set_text_fmt(phandle->end_hour_label, "%02d", phandle->power_on_hour);
				}
				else if(cur_obj == phandle->end_minute_btn)
				{
					if(phandle->power_on_minute >= 59)
                	{
                	    phandle->power_on_minute = 0;
                	}
                	else
                	{
                  	 	phandle->power_on_minute ++;
                	}
               		lv_label_set_text_fmt(phandle->end_minute_label, "%02d", phandle->power_on_minute);
				}
			}
			break;
		case DIVOOM_LVGL_KEY_ESC:
			if(phandle->btn_status == 0)
			{
				divoom_lvgl_setting_power_off_update_time(phandle);
				divoom_menu_control_return_pre();
			}
			else
			{
				phandle->btn_status = 0;
				lv_obj_add_style(cur_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
				lv_obj_add_style(cur_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
			}
			break;
		default:
			break;
		}
 	}
}


void divoom_lvgl_setting_power_off_create_title_area(Pdivoom_lvgl_setting_power_off_ctrl phandle)
{
	lv_obj_t *title = lv_obj_create(phandle->main_obj);
	lv_obj_set_style_bg_color(title, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_border_width(title, 0, 0);
	lv_obj_set_style_pad_all(title, 0, 0);
	lv_obj_set_style_text_color(title, lv_color_black(), 0);
	lv_obj_clear_flag(title, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(title, 800, 78); 
	lv_obj_set_pos(title, 0, 50);

	lv_obj_t *power_off_label = lv_label_create(title);
	lv_obj_set_size(power_off_label, 700, 56); 
	lv_obj_set_style_text_font(power_off_label, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_obj_set_style_text_align(power_off_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_text(power_off_label, divoom_menu_control_get_disp_str("PowerOnOffTime"));
	lv_obj_set_pos(power_off_label, 70, 9);

}

void divoom_lvgl_setting_power_off_create_time_area(Pdivoom_lvgl_setting_power_off_ctrl phandle)
{
	/*int hour;
	char disp_str[32];
	
	lv_obj_t *switch_obj = lv_obj_create(phandle->main_obj);
	lv_obj_add_style(switch_obj, &phandle->cont_type, 0);
	lv_obj_clear_flag(switch_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(switch_obj, 726, 90);
	lv_obj_set_pos(switch_obj, 37, 209);
	lv_obj_t *switch_label = lv_label_create(switch_obj);
	lv_obj_set_size(switch_label, 500, 42);
	lv_obj_set_pos(switch_label, 50, 24);
	lv_obj_set_style_text_font(switch_label, divoom_menu_control_get_font_info(32), 0);
	lv_label_set_text(switch_label, divoom_menu_control_get_disp_str("PowerOnOffTime"));
	
	lv_obj_t *switch_btn = lv_btn_create(switch_obj);
	lv_obj_set_style_pad_all(switch_btn, 0, 0);
	lv_obj_set_style_bg_opa(switch_btn, LV_OPA_0, 0);
	lv_obj_set_style_shadow_width(switch_btn, 0, 0);
	lv_obj_set_pos(switch_btn, 594, 14);
	phandle->switch_btn = switch_btn;
	lv_obj_add_event(switch_btn, divoom_lvgl_setting_power_off_event_cb, LV_EVENT_CLICKED, phandle);
	
	lv_obj_t *switch_img = lv_img_create(switch_btn);
	if(phandle->power_on_off)
	{
		divoom_menu_control_lv_img_set_src(switch_img, "62/62_button_on.png");
	}
	else
	{
		divoom_menu_control_lv_img_set_src(switch_img, "62/62_button_off.png");
	}
	phandle->switch_img = switch_img;

	//开机/结束时间
	lv_obj_t *power_on_off_time_obj = lv_obj_create(phandle->main_obj);
	lv_obj_add_style(power_on_off_time_obj, &phandle->cont_type, 0);
	lv_obj_clear_flag(power_on_off_time_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(power_on_off_time_obj, 726, 270);
	lv_obj_set_pos(power_on_off_time_obj, 37, 325);
	lv_obj_t *power_on_off_time_label = lv_label_create(power_on_off_time_obj);
	lv_obj_set_style_text_font(power_on_off_time_label, divoom_menu_control_get_font_info(32), 0);
	lv_obj_set_size(power_on_off_time_label, 350, 42);
	lv_obj_set_pos(power_on_off_time_label, 36, 24);
	lv_label_set_text(power_on_off_time_label, divoom_menu_control_get_disp_str("Start/End Time"));

	//时间
	lv_obj_t *time_obj = lv_obj_create(power_on_off_time_obj);
	lv_obj_set_style_border_width(time_obj, 0, 0);
	lv_obj_set_style_pad_all(time_obj, 0, 0);
	lv_obj_set_style_bg_opa(time_obj, LV_OPA_0, 0);
	lv_obj_clear_flag(time_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(time_obj, 726, 79);
	lv_obj_set_pos(time_obj, 0, 135);

	lv_obj_t *start_time_btn = lv_btn_create(time_obj);//开始时间
	lv_obj_add_style(start_time_btn, &phandle->btn_normal_type, 0);
	lv_obj_set_style_bg_opa(start_time_btn, LV_OPA_0, LV_STATE_PRESSED);
	lv_obj_set_size(start_time_btn, 300, 79);
	lv_obj_set_pos(start_time_btn, 40, 0);
	phandle->start_time_btn = start_time_btn;
	lv_obj_add_event(start_time_btn, divoom_lvgl_setting_power_off_event_cb, LV_EVENT_CLICKED, phandle);
	lv_obj_t *end_time_btn = lv_btn_create(time_obj);//结束时间
	lv_obj_add_style(end_time_btn, &phandle->btn_normal_type, 0);
	lv_obj_set_style_bg_opa(start_time_btn, LV_OPA_0, LV_STATE_PRESSED);
	lv_obj_set_size(end_time_btn, 300, 79);
	lv_obj_set_pos(end_time_btn, 390, 0);
	phandle->end_time_btn = end_time_btn;
	lv_obj_add_event(end_time_btn, divoom_lvgl_setting_power_off_event_cb, LV_EVENT_CLICKED, phandle);

	lv_obj_t *line_label = lv_label_create(time_obj);
	lv_obj_set_style_text_font(line_label, divoom_menu_control_get_font_info(60), 0);
	lv_obj_set_style_text_color(line_label, lv_color_white(), 0);
	lv_label_set_text(line_label, "-");
	lv_obj_center(line_label);

	lv_obj_t *start_time_label = lv_label_create(start_time_btn);
	lv_obj_set_style_text_font(start_time_label, divoom_menu_control_get_font_info(60), 0);
	lv_obj_set_style_text_align(start_time_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_height(start_time_label, 79);
	lv_obj_center(start_time_label);
	phandle->start_time_label = start_time_label;
	lv_obj_t *end_time_label = lv_label_create(end_time_btn);
	lv_obj_set_style_text_font(end_time_label, divoom_menu_control_get_font_info(60), 0);
	lv_obj_set_style_text_align(end_time_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_height(end_time_label, 79);
	lv_obj_center(end_time_label);
	phandle->end_time_label = end_time_label;
	*/

	int active_id = 0, i, hour;
	
	//divoom_menu_control_init_btn_stype(&phandle->btn_select_style, NULL, &phandle->btn_normal_style, NULL, &phandle->btn_edit_style, NULL, 19);

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

	lv_obj_t *cont = lv_obj_create(phandle->main_obj);
	lv_obj_remove_style(cont, NULL, LV_PART_SCROLLBAR);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_outline_width(cont, 0, 0);
    lv_obj_set_style_border_side(cont, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_pad_bottom(cont, 48, 0);
    lv_obj_set_size(cont, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH - 128);
    lv_obj_set_pos(cont, 0, 128);
	
	//定时开关机
	lv_obj_t *btn_obj = lv_btn_create(cont);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 204-128);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_setting_power_off_event_cb, LV_EVENT_KEY, phandle); 
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, active_id, 0);
	active_id++;
	phandle->power_on_off_btn = btn_obj;
	if(phandle->first_active_obj == NULL)
	{
		phandle->first_active_obj = btn_obj;
	}
	
    lv_obj_t *label_obj = lv_label_create(cont);
    lv_obj_set_pos(label_obj, 76, 232-128);
	lv_obj_set_size(label_obj, 500, 34);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("PowerOnOffTime"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);  
    
    lv_obj_t *image_obj = lv_img_create(cont);
	lv_obj_set_pos(image_obj, 652, 227-128);
    lv_obj_set_size(image_obj, 72, 44);
    if(phandle->power_on_off)
    {
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_on.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_off.png");
    }
    phandle->switch_img = image_obj;

	//开始时间
	label_obj = lv_label_create(cont);
    lv_obj_set_pos(label_obj, 76, 342-128);
	lv_obj_set_size(label_obj, 310, 32);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("StartTime"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x636363), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(24, phandle->menu_id), 0); 

	if(divoom_main_get_time_flag())//24小时
    {
    	
    	for(i = 0;i < 2; i++)
		{
			/*lv_obj_t *obj = lv_obj_create(phandle->main_obj);
			lv_obj_add_style(obj, &phandle->obj_style, 0);
			lv_obj_set_size(obj, 720, 138);
			lv_obj_set_pos(obj, 40, 160+138*i);
			lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);*/
		
			lv_obj_t *btn  = lv_btn_create(cont);
			lv_obj_set_size(btn, 712, 138);
			lv_obj_set_pos(btn, 44, 386+138*i-128);
			lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

			lv_obj_add_style(btn, &phandle->btn_normal_style, 0);
		
			lv_obj_add_style(btn, &phandle->btn_select_style, LV_STATE_FOCUSED);
			lv_obj_add_style(btn, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
			lv_obj_set_style_pad_all(btn, 0, 0);
			lv_obj_add_event_cb(btn, divoom_lvgl_setting_power_off_event_cb, LV_EVENT_KEY, phandle);
			
			divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn, active_id, 0);
			active_id ++;

			lv_obj_t *time_label = lv_label_create(btn);
			lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
			lv_obj_set_style_text_color(time_label, lv_color_hex(0x0), 0);
			lv_obj_set_size(time_label, 400, 34);
			lv_obj_set_pos(time_label, 32, 48);

			lv_obj_t *disp_obj = lv_obj_create(btn);
			lv_obj_set_style_pad_all(disp_obj, 0, 0);
			lv_obj_set_style_radius(disp_obj, 14, 0);
			lv_obj_set_style_bg_color(disp_obj, lv_color_hex(0xFFFFFF), 0);
			lv_obj_set_style_border_color(disp_obj, lv_color_hex(0xDCDCDC), 0);
			lv_obj_set_style_border_width(disp_obj, 4, 0);
			lv_obj_set_size(disp_obj, 180, 80);
			lv_obj_set_pos(disp_obj, 496, 25);
			lv_obj_clear_flag(disp_obj, LV_OBJ_FLAG_SCROLLABLE);

			lv_obj_t *disp_label = lv_label_create(disp_obj);
			lv_obj_set_style_text_font(disp_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
			lv_obj_set_style_text_color(disp_label, lv_color_hex(0x636363), 0);
			lv_obj_set_style_text_align(disp_label, LV_TEXT_ALIGN_CENTER, 0);
			lv_obj_set_size(disp_label, 180, 34);
			lv_obj_set_pos(disp_label, 0, 23);
			if(i == 0)//开始时间小时
			{
				phandle->start_hour_btn = btn;
				phandle->start_hour_label = disp_label;
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Hour"));
        		lv_label_set_text_fmt(disp_label, "%02d", phandle->power_off_hour);
			}
			else if(i == 1)//开始时间分钟
			{
				phandle->start_minute_btn = btn;
				phandle->start_minute_label = disp_label;
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Minute"));
				lv_label_set_text_fmt(disp_label, "%02d", phandle->power_off_minute);
			}
	
			lv_obj_t *up_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(up_img, "26/26_up.png");
			lv_obj_set_pos(up_img, 578, 4);
			lv_obj_t *down_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(down_img, "26/26_down.png");	
			lv_obj_set_pos(down_img, 578, 110);
		}
		phandle->start_am_pm_btn = NULL;
	}
	else
	{
		for(i = 0;i < 3; i++)
		{
			/*lv_obj_t *obj = lv_obj_create(phandle->main_obj);
			lv_obj_add_style(obj, &phandle->obj_style, 0);
			lv_obj_set_size(obj, 720, 138);
			lv_obj_set_pos(obj, 40, 160+138*i);
			lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);*/
		
			lv_obj_t *btn  = lv_btn_create(cont);
			lv_obj_set_size(btn, 712, 138);
			lv_obj_set_pos(btn, 44, 386+138*i-128);
			lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
			lv_obj_add_style(btn, &phandle->btn_normal_style, 0);
		
			lv_obj_add_style(btn, &phandle->btn_select_style, LV_STATE_FOCUSED);
			lv_obj_add_style(btn, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
			lv_obj_set_style_pad_all(btn, 0, 0);
			lv_obj_add_event_cb(btn, divoom_lvgl_setting_power_off_event_cb, LV_EVENT_KEY, phandle);
			
			divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn, active_id, 0);
			active_id ++;

			lv_obj_t *time_label = lv_label_create(btn);
			lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
			lv_obj_set_style_text_color(time_label, lv_color_hex(0x0), 0);
			lv_obj_set_size(time_label, 400, 34);
			lv_obj_set_pos(time_label, 32, 48);

			lv_obj_t *disp_obj = lv_obj_create(btn);
			lv_obj_set_style_pad_all(disp_obj, 0, 0);
			lv_obj_set_style_radius(disp_obj, 14, 0);
			lv_obj_set_style_bg_color(disp_obj, lv_color_hex(0xFFFFFF), 0);
			lv_obj_set_style_border_color(disp_obj, lv_color_hex(0xDCDCDC), 0);
			lv_obj_set_style_border_width(disp_obj, 4, 0);
			lv_obj_set_size(disp_obj, 180, 80);
			lv_obj_set_pos(disp_obj, 496, 25);
			lv_obj_clear_flag(disp_obj, LV_OBJ_FLAG_SCROLLABLE);

			lv_obj_t *disp_label = lv_label_create(disp_obj);
			lv_obj_set_style_text_font(disp_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
			lv_obj_set_style_text_color(disp_label, lv_color_hex(0x636363), 0);
			lv_obj_set_style_text_align(disp_label, LV_TEXT_ALIGN_CENTER, 0);
			lv_obj_set_size(disp_label, 180, 34);
			lv_obj_set_pos(disp_label, 0, 23);
			if(i == 0)//开始时间时间段
			{
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("TimePeriod"));
				if(phandle->power_off_hour < 12)
				{
					phandle->power_off_am_pm_index = 0;
					lv_label_set_text(disp_label, divoom_menu_control_get_disp_str("AM"));
				}
				else
				{
					phandle->power_off_am_pm_index = 1;
					lv_label_set_text(disp_label, divoom_menu_control_get_disp_str("PM"));
				}
				phandle->start_am_pm_btn = btn;
				phandle->start_am_pm_label = disp_label;
			}
			else if(i == 1)//开始时间小时
			{
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Hour"));
				hour = phandle->power_off_hour;
            	if(hour != 12)
            	{
               		hour = hour % 12;
                	if(hour == 0)
                	{
                    	hour = 12;
                	}
            	}
				phandle->power_off_hour = hour;
				phandle->start_hour_btn = btn;		//
				phandle->start_hour_label = disp_label;
				lv_label_set_text_fmt(disp_label, "%02d", hour);
			}
			else if(i == 2)//开始时间分钟
			{
				phandle->start_minute_btn = btn;
				phandle->start_minute_label = disp_label;
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Minute"));
				lv_label_set_text_fmt(disp_label, "%02d", phandle->power_off_minute);
			}
	
			lv_obj_t *up_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(up_img, "26/26_up.png");
			lv_obj_set_pos(up_img, 578, 4);
			lv_obj_t *down_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(down_img, "26/26_down.png");	
			lv_obj_set_pos(down_img, 578, 110);
		}
	}
	
	//结束时间
	label_obj = lv_label_create(cont);
	if(divoom_main_get_time_flag())//12小时和24小时坐标不同
	{
		lv_obj_set_pos(label_obj, 76, 702-128);
	}
	else
	{
		lv_obj_set_pos(label_obj, 76, 840-128);
	}
    
	lv_obj_set_size(label_obj, 310, 32);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("EndTime"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x636363), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);

	if(divoom_main_get_time_flag())//24小时
    {
    	for(i = 0;i < 2; i++)
		{
			/*lv_obj_t *obj = lv_obj_create(phandle->main_obj);
			lv_obj_add_style(obj, &phandle->obj_style, 0);
			lv_obj_set_size(obj, 720, 138);
			lv_obj_set_pos(obj, 40, 160+138*i);
			lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);*/
		
			lv_obj_t *btn  = lv_btn_create(cont);
			lv_obj_set_size(btn, 712, 138);
			lv_obj_set_pos(btn, 44, 746+138*i-128);
			lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

			lv_obj_add_style(btn, &phandle->btn_normal_style, 0);
		
			lv_obj_add_style(btn, &phandle->btn_select_style, LV_STATE_FOCUSED);
			lv_obj_add_style(btn, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
			lv_obj_set_style_pad_all(btn, 0, 0);
			lv_obj_add_event_cb(btn, divoom_lvgl_setting_power_off_event_cb, LV_EVENT_KEY, phandle);
			
			divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn, active_id, 0);
			active_id ++;

			lv_obj_t *time_label = lv_label_create(btn);
			lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
			lv_obj_set_style_text_color(time_label, lv_color_hex(0x0), 0);
			lv_obj_set_size(time_label, 400, 34);
			lv_obj_set_pos(time_label, 32, 48);

			lv_obj_t *disp_obj = lv_obj_create(btn);
			lv_obj_set_style_pad_all(disp_obj, 0, 0);
			lv_obj_set_style_radius(disp_obj, 14, 0);
			lv_obj_set_style_bg_color(disp_obj, lv_color_hex(0xFFFFFF), 0);
			lv_obj_set_style_border_color(disp_obj, lv_color_hex(0xDCDCDC), 0);
			lv_obj_set_style_border_width(disp_obj, 4, 0);
			lv_obj_set_size(disp_obj, 180, 80);
			lv_obj_set_pos(disp_obj, 496, 25);
			lv_obj_clear_flag(disp_obj, LV_OBJ_FLAG_SCROLLABLE);

			lv_obj_t *disp_label = lv_label_create(disp_obj);
			lv_obj_set_style_text_font(disp_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
			lv_obj_set_style_text_color(disp_label, lv_color_hex(0x636363), 0);
			lv_obj_set_style_text_align(disp_label, LV_TEXT_ALIGN_CENTER, 0);
			lv_obj_set_size(disp_label, 180, 34);
			lv_obj_set_pos(disp_label, 0, 23);
			if(i == 0)//结束时间小时
			{
				phandle->end_hour_btn = btn;
				phandle->end_hour_label = disp_label;
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Hour"));
        		lv_label_set_text_fmt(disp_label, "%02d", phandle->power_on_hour);
			}
			else if(i == 1)//结束时间分钟
			{
				phandle->end_minute_btn = btn;
				phandle->end_minute_label = disp_label;
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Minute"));
				lv_label_set_text_fmt(disp_label, "%02d", phandle->power_on_minute);
			}
	
			lv_obj_t *up_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(up_img, "26/26_up.png");
			lv_obj_set_pos(up_img, 578, 4);
			lv_obj_t *down_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(down_img, "26/26_down.png");	
			lv_obj_set_pos(down_img, 578, 110);
		}
		phandle->end_am_pm_btn = NULL;
	}
	else
	{
		for(i = 0;i < 3; i++)
		{
			/*lv_obj_t *obj = lv_obj_create(phandle->main_obj);
			lv_obj_add_style(obj, &phandle->obj_style, 0);
			lv_obj_set_size(obj, 720, 138);
			lv_obj_set_pos(obj, 40, 160+138*i);
			lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);*/
		
			lv_obj_t *btn  = lv_btn_create(cont);
			lv_obj_set_size(btn, 712, 138);
			lv_obj_set_pos(btn, 44, 884+138*i-128);
			lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
			lv_obj_add_style(btn, &phandle->btn_normal_style, 0);
		
			lv_obj_add_style(btn, &phandle->btn_select_style, LV_STATE_FOCUSED);
			lv_obj_add_style(btn, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
			lv_obj_set_style_pad_all(btn, 0, 0);
			lv_obj_add_event_cb(btn, divoom_lvgl_setting_power_off_event_cb, LV_EVENT_KEY, phandle);
			
			divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn, active_id, 0);
			active_id ++;

			lv_obj_t *time_label = lv_label_create(btn);
			lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
			lv_obj_set_style_text_color(time_label, lv_color_hex(0x0), 0);
			lv_obj_set_size(time_label, 400, 34);
			lv_obj_set_pos(time_label, 32, 48);

			lv_obj_t *disp_obj = lv_obj_create(btn);
			lv_obj_set_style_pad_all(disp_obj, 0, 0);
			lv_obj_set_style_radius(disp_obj, 14, 0);
			lv_obj_set_style_bg_color(disp_obj, lv_color_hex(0xFFFFFF), 0);
			lv_obj_set_style_border_color(disp_obj, lv_color_hex(0xDCDCDC), 0);
			lv_obj_set_style_border_width(disp_obj, 4, 0);
			lv_obj_set_size(disp_obj, 180, 80);
			lv_obj_set_pos(disp_obj, 496, 25);
			lv_obj_clear_flag(disp_obj, LV_OBJ_FLAG_SCROLLABLE);

			lv_obj_t *disp_label = lv_label_create(disp_obj);
			lv_obj_set_style_text_font(disp_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
			lv_obj_set_style_text_color(disp_label, lv_color_hex(0x636363), 0);
			lv_obj_set_style_text_align(disp_label, LV_TEXT_ALIGN_CENTER, 0);
			lv_obj_set_size(disp_label, 180, 34);
			lv_obj_set_pos(disp_label, 0, 23);
			if(i == 0)//结束时间时间段
			{
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("TimePeriod"));
				if(phandle->power_on_hour < 12)
				{
					phandle->power_on_am_pm_index = 0;
					lv_label_set_text(disp_label, divoom_menu_control_get_disp_str("AM"));
				}
				else
				{
					phandle->power_on_am_pm_index = 1;
					lv_label_set_text(disp_label, divoom_menu_control_get_disp_str("PM"));
				}
				phandle->end_am_pm_btn = btn;
				phandle->end_am_pm_label = disp_label;
			}
			else if(i == 1)//结束时间小时
			{
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Hour"));
				hour = phandle->power_on_hour;
            	if(hour != 12)
            	{
               		hour = hour % 12;
                	if(hour == 0)
                	{
                    	hour = 12;
                	}
            	}
				phandle->power_on_hour = hour;
				phandle->end_hour_btn = btn;
				phandle->end_hour_label = disp_label;
				lv_label_set_text_fmt(disp_label, "%02d", hour);
			}
			else if(i == 2)//结束时间分钟
			{
				phandle->end_minute_btn = btn;	
				phandle->end_minute_label = disp_label;
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Minute"));
				lv_label_set_text_fmt(disp_label, "%02d", phandle->power_on_minute);
			}
	
			lv_obj_t *up_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(up_img, "26/26_up.png");
			lv_obj_set_pos(up_img, 578, 4);
			lv_obj_t *down_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(down_img, "26/26_down.png");	
			lv_obj_set_pos(down_img, 578, 110);
		}
	}
}


void *divoom_lvgl_setting_power_off_init(void *user_data, int menu_id)
{
	Pdivoom_lvgl_setting_power_off_ctrl phandle = (Pdivoom_lvgl_setting_power_off_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_setting_power_off_ctrl));
    phandle->menu_id = menu_id;
	phandle->pdirect_handle = divoom_menu_direct_ctrl_create(7, 1);
	phandle->pon_off_config = divoom_main_get_power_on_off_config();
	phandle->btn_status = 0;
	phandle->first_active_obj = NULL;

	phandle->power_on_off = phandle->pon_off_config->on_off;
	phandle->power_off_hour = phandle->pon_off_config->start/60; 
	phandle->power_off_minute = phandle->pon_off_config->start%60;
	phandle->power_on_hour = phandle->pon_off_config->end/60;
	phandle->power_on_minute = phandle->pon_off_config->end%60;
	
	phandle->main_obj = lv_obj_create(NULL);
	
	lv_style_init(&phandle->backgroud_type);
    lv_style_set_bg_color(&phandle->backgroud_type, lv_color_hex(0xE4E4E4));
    lv_style_set_text_color(&phandle->backgroud_type, lv_color_hex(0x0));
    lv_obj_add_style(phandle->main_obj, &phandle->backgroud_type, 0);
    lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_scrollbar_mode(phandle->main_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(phandle->main_obj, LV_DIR_VER);
	lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_SCROLLABLE);

	divoom_lvgl_setting_power_off_create_title_area(phandle);
	divoom_lvgl_setting_power_off_create_time_area(phandle);

	return phandle;
}


void divoom_lvgl_setting_power_off_disp(void *handle, void *active)
{
	Pdivoom_lvgl_setting_power_off_ctrl phandle = (Pdivoom_lvgl_setting_power_off_ctrl)handle;

	if(phandle->first_active_obj)
	{
		lv_group_focus_obj(phandle->first_active_obj);
		phandle->first_active_obj = NULL;
	}
	else if(active)
	{
		lv_group_focus_obj(active);
	}
	lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_setting_power_off_exit(void *handle)
{
	Pdivoom_lvgl_setting_power_off_ctrl phandle = (Pdivoom_lvgl_setting_power_off_ctrl)handle;

	lv_style_reset(&phandle->backgroud_type);
	lv_style_reset(&phandle->btn_normal_style);
	lv_style_reset(&phandle->btn_edit_style);
	lv_style_reset(&phandle->btn_select_style);
	divoom_menu_direct_ctrl_destory(phandle->pdirect_handle);
	lv_obj_del(phandle->main_obj);
	divoom_system_free(phandle);
}




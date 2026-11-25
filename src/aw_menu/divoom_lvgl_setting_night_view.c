#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"

typedef struct _divoom_lvgl_setting_night_view_ctrl
{
    int menu_id;
    lv_style_t normal_style;
    void *main_obj;
    void *back_obj;
	void *night_view_btn;
    void *switch_obj;
	void *bright_btn;
    void *bright_obj;
    void *time_obj;

	int btn_status;
	void *pdirect_handle;
	lv_style_t btn_normal_style;
	lv_style_t btn_edit_style;
	lv_style_t btn_select_style;

	int bright;
	int night_view_on_off;
	int start_am_pm_flag;
	int start_hour;
	int start_minute;
	int end_am_pm_flag;
	int end_hour;
	int end_minute;

	void *first_active_obj;
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
}divoom_lvgl_setting_night_view_ctrl, *Pdivoom_lvgl_setting_night_view_ctrl;

void divoom_lvgl_setting_night_view_update_time(Pdivoom_lvgl_setting_night_view_ctrl phandle)
{
	int start_hour, end_hour;
	divoom_sys_msg_info model_msg = {0};
	Pdivoom_system_setting_json_night_view pnight_view;
	divoom_system_setting_json_night_view night_view;

	pnight_view = (Pdivoom_system_setting_json_night_view)divoom_system_malloc(sizeof(divoom_system_setting_json_night_view));
	if(divoom_main_get_time_flag() == 0)	
	{
		if(phandle->start_am_pm_flag)
		{
			start_hour = phandle->start_hour + 12;
		}
		else
		{
			start_hour = phandle->start_hour;
		}
		
		if(phandle->end_am_pm_flag)
		{
			end_hour = phandle->end_hour + 12;
			
		}
		else
		{
			end_hour = phandle->end_hour;
		}
	}
	else
	{
		start_hour = phandle->start_hour;
		end_hour = phandle->end_hour;
	}

	pnight_view->start = start_hour*60 + phandle->start_minute;
	pnight_view->end = end_hour*60 + phandle->end_minute;
	pnight_view->on_off = phandle->night_view_on_off;
	pnight_view->bright = phandle->bright;
	
	model_msg.src_id = DIVOOM_MODEL_APP_TOOL;
	model_msg.dest_id = DIVOOM_MODEL_MAIN;
	model_msg.msg_type = 1;
	model_msg.msg_id = DIVOOM_MAIN_EVENT_SET_DEVICE_NIGHT_VIEW;
	model_msg.data = (uint8 *)pnight_view;
	divoom_sys_model_crl_msg_push(&model_msg);
	
	night_view.bright = phandle->bright;
	night_view.on_off = phandle->night_view_on_off;
	night_view.start = start_hour*60 + phandle->start_minute;
	night_view.end = end_hour*60 + phandle->end_minute;
	
	divoom_net_app_send_packet(DIVOOM_NET_COMM_SET_SYS_NIGHT_VIEW, DIVOOM_SERVER_TOPIC, &night_view);
}


static void divoom_lvgl_setting_night_view_menu_event_cb(lv_event_t * e)
{
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_setting_night_view_ctrl phandle = (Pdivoom_lvgl_setting_night_view_ctrl)lv_event_get_user_data(e);
    Pdivoom_system_setting_json_night_view pnight_view = divoom_main_get_night_view_info();
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    /*if(code == LV_EVENT_KEY)
    {
        Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();
        
        if(phandle->back_obj == cur_obj)
        {
            divoom_menu_control_return_pre();
            return;
        }
        else if(phandle->bright_obj == cur_obj)
        {
            pnight_view->bright = lv_slider_get_value(cur_obj);
            divoom_main_save_config();
        }
        else if(phandle->switch_obj == cur_obj)
        {
            if(pnight_view->on_off)
            {
                pnight_view->on_off = 0;
                divoom_menu_control_lv_img_set_src(phandle->switch_obj, "26/26_off.png");
            }
            else
            {
                pnight_view->on_off = 1;
                divoom_menu_control_lv_img_set_src(phandle->switch_obj, "26/26_on.png");
            }
            divoom_main_save_config();
        }
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
					if(cur_obj == phandle->bright_btn)
					{
						int value = lv_slider_get_value(phandle->bright_obj);
						value = (value < 95) ? (value + 5) : (100);
						lv_slider_set_value(phandle->bright_obj, value, LV_ANIM_ON);
						//pnight_view->bright = value;
						phandle->bright = value;
						if(pnight_view->on_off)
						{
							divoom_lvgl_setting_night_view_update_time(phandle);
						}
					}
					else if(cur_obj == phandle->start_am_pm_btn)
					{
						if(phandle->start_am_pm_flag)
						{
							phandle->start_am_pm_flag = 0;
							lv_label_set_text(phandle->start_am_pm_label, divoom_menu_control_get_disp_str("AM"));
						}
						else 
						{
							phandle->start_am_pm_flag = 1;
							lv_label_set_text(phandle->start_am_pm_label, divoom_menu_control_get_disp_str("PM"));
						}
					}
					else if(cur_obj == phandle->start_hour_btn)
					{
						if(divoom_main_get_time_flag() == 0)
                		{
                    		if(phandle->start_hour >= 12)
                    		{
                    	    	phandle->start_hour = 1;
                    		}
                    		else
                    		{
                    	    	phandle->start_hour ++;
                    		}
                		}
               			else
                		{
                    		if(phandle->start_hour >= 23)
                    		{
                    		    phandle->start_hour = 0;
                    		}
                    		else
                    		{
                        		phandle->start_hour ++;
                    		}
                		}
                		lv_label_set_text_fmt(phandle->start_hour_label, "%02d", phandle->start_hour);
					}
					else if(cur_obj == phandle->start_minute_btn)
					{
						if(phandle->start_minute >= 59)
                		{
                		    phandle->start_minute = 0;
                		}
                		else
                		{
                		    phandle->start_minute ++;
                		}
                		lv_label_set_text_fmt(phandle->start_minute_label, "%02d", phandle->start_minute);
					}
					else if(cur_obj == phandle->end_am_pm_btn)
					{
						if(phandle->end_am_pm_flag)
						{
							phandle->end_am_pm_flag = 0;
							lv_label_set_text(phandle->end_am_pm_label, divoom_menu_control_get_disp_str("AM"));
						}
						else 
						{
							phandle->end_am_pm_flag = 1;
							lv_label_set_text(phandle->end_am_pm_label, divoom_menu_control_get_disp_str("PM"));
						}
					}
					else if(cur_obj == phandle->end_hour_btn)
					{
						if(divoom_main_get_time_flag() == 0)
                		{
                    		if(phandle->end_hour >= 12)
                    		{
                    		    phandle->end_hour = 1;
                    		}
                    		else
                    		{
                    		    phandle->end_hour ++;
                    		}
                		}
               			else
                		{
                    		if(phandle->end_hour >= 23)
                    		{
                    		    phandle->end_hour = 0;
                    		}
                    		else
                    		{
                        		phandle->end_hour ++;
                    		}
                		}
                		lv_label_set_text_fmt(phandle->end_hour_label, "%02d", phandle->end_hour);
					}
					else if(cur_obj == phandle->end_minute_btn)
					{
						if(phandle->end_minute >= 59)
                		{
                		    phandle->end_minute = 0;
                		}
                		else
                		{
                	  	 	phandle->end_minute ++;
                		}
               			lv_label_set_text_fmt(phandle->end_minute_label, "%02d", phandle->end_minute);
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
					if(cur_obj == phandle->bright_btn)
					{
						int value = lv_slider_get_value(phandle->bright_obj);
						value = (value > 5) ? (value - 5) : (0);
						lv_slider_set_value(phandle->bright_obj, value, LV_ANIM_ON);
						//pnight_view->bright = value;
						phandle->bright = value;
						if(pnight_view->on_off)
						{
							divoom_lvgl_setting_night_view_update_time(phandle);
						}
					}
					else if(cur_obj == phandle->start_am_pm_btn)
					{
						if(phandle->start_am_pm_flag)
						{
							phandle->start_am_pm_flag = 0;
							lv_label_set_text(phandle->start_am_pm_label, divoom_menu_control_get_disp_str("AM"));
						}
						else 
						{
							phandle->start_am_pm_flag = 1;
							lv_label_set_text(phandle->start_am_pm_label, divoom_menu_control_get_disp_str("PM"));
						}
					}
					else if(cur_obj == phandle->start_hour_btn)
					{
						if(divoom_main_get_time_flag() == 0)
                		{
                    		if(phandle->start_hour <= 1)
                    		{
                    	    	phandle->start_hour = 12;
                    		}
                    		else
                    		{
                    	    	phandle->start_hour --;
                    		}
                		}
               			else
                		{
                    		if(phandle->start_hour <= 0)
                    		{
                    		    phandle->start_hour = 23;
                    		}
                    		else
                    		{
                        		phandle->start_hour --;
                    		}
                		}
                		lv_label_set_text_fmt(phandle->start_hour_label, "%02d", phandle->start_hour);
					}
					else if(cur_obj == phandle->start_minute_btn)
					{
						if(phandle->start_minute <= 0)
                		{
                		    phandle->start_minute = 59;
                		}
                		else
                		{
                		    phandle->start_minute --;
                		}
                		lv_label_set_text_fmt(phandle->start_minute_label, "%02d", phandle->start_minute);
					}
					else if(cur_obj == phandle->end_am_pm_btn)
					{
						if(phandle->end_am_pm_flag)
						{
							phandle->end_am_pm_flag = 0;
							lv_label_set_text(phandle->end_am_pm_label, divoom_menu_control_get_disp_str("AM"));
						}
						else 
						{
							phandle->end_am_pm_flag = 1;
							lv_label_set_text(phandle->end_am_pm_label, divoom_menu_control_get_disp_str("PM"));
						}
					}
					else if(cur_obj == phandle->end_hour_btn)
					{
						if(divoom_main_get_time_flag() == 0)
                		{
                    		if(phandle->end_hour <= 1)
                    		{
                    		    phandle->end_hour = 12;
                    		}
                    		else
                    		{
                    		    phandle->end_hour --;
                    		}
                		}
               			else
                		{
                    		if(phandle->end_hour <= 0)
                    		{
                    		    phandle->end_hour = 23;
                    		}
                    		else
                    		{
                        		phandle->end_hour --;
                    		}
                		}
                		lv_label_set_text_fmt(phandle->end_hour_label, "%02d", phandle->end_hour);
					}
					else if(cur_obj == phandle->end_minute_btn)
					{
						if(phandle->end_minute <= 0)
                		{
                		    phandle->end_minute = 59;
                		}
                		else
                		{
                	  	 	phandle->end_minute --;
                		}
               			lv_label_set_text_fmt(phandle->end_minute_label, "%02d", phandle->end_minute);
					}
				}
	 		break;
		case DIVOOM_LVGL_KEY_ENTER:
			if(phandle->btn_status == 0)
			{
				if(cur_obj == phandle->night_view_btn)
				{
					if(phandle->night_view_on_off == 1)
					{
						phandle->night_view_on_off = 0;
						divoom_menu_control_lv_img_set_src(phandle->switch_obj, "26/26_off.png");
					}
					else
					{
						phandle->night_view_on_off = 1;
						divoom_menu_control_lv_img_set_src(phandle->switch_obj, "26/26_on.png");
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
				if(cur_obj == phandle->bright_btn)
				{
					int value = lv_slider_get_value(phandle->bright_obj);
					value = (value < 95) ? (value + 5) : (100);
					lv_slider_set_value(phandle->bright_obj, value, LV_ANIM_ON);
					phandle->bright = value;
					if(pnight_view->on_off)
					{
						divoom_lvgl_setting_night_view_update_time(phandle);
					}
					divoom_main_save_config();
				}
				else if(cur_obj == phandle->start_am_pm_btn)
				{
					if(phandle->start_am_pm_flag)
					{
						phandle->start_am_pm_flag = 0;
						lv_label_set_text(phandle->start_am_pm_label, divoom_menu_control_get_disp_str("AM"));
					}
					else 
					{
						phandle->start_am_pm_flag = 1;
						lv_label_set_text(phandle->start_am_pm_label, divoom_menu_control_get_disp_str("PM"));
					}
				}
				else if(cur_obj == phandle->start_hour_btn)
				{
					if(divoom_main_get_time_flag() == 0)
                	{
                    	if(phandle->start_hour >= 12)
                    	{
                        	phandle->start_hour = 1;
                    	}
                    	else
                    	{
                        	phandle->start_hour ++;
                    	}
                	}
               		else
                	{
                    	if(phandle->start_hour >= 23)
                    	{
                    	    phandle->start_hour = 0;
                    	}
                    	else
                    	{
                       		phandle->start_hour ++;
                   		}
                	}
                	lv_label_set_text_fmt(phandle->start_hour_label, "%02d", phandle->start_hour);
				}
				else if(cur_obj == phandle->start_minute_btn)
				{
					if(phandle->start_minute >= 59)
                	{
                	    phandle->start_minute = 0;
                	}
                	else
                	{
                	    phandle->start_minute ++;
                	}
                	lv_label_set_text_fmt(phandle->start_minute_label, "%02d", phandle->start_minute);
				}
				else if(cur_obj == phandle->end_am_pm_btn)
				{
					if(phandle->end_am_pm_flag)
					{
						phandle->end_am_pm_flag = 0;
						lv_label_set_text(phandle->end_am_pm_label, divoom_menu_control_get_disp_str("AM"));
					}
					else 
					{
						phandle->end_am_pm_flag = 1;
						lv_label_set_text(phandle->end_am_pm_label, divoom_menu_control_get_disp_str("PM"));
					}
				}
				else if(cur_obj == phandle->end_hour_btn)
				{
					if(divoom_main_get_time_flag() == 0)
                	{
                    	if(phandle->end_hour >= 12)
                    	{
                    	    phandle->end_hour = 1;
                    	}
                    	else
                    	{
                    	    phandle->end_hour ++;
                    	}
                	}
               		else
                	{
                    	if(phandle->end_hour >= 23)
                    	{
                    	    phandle->end_hour = 0;
                    	}
                    	else
                   		{
                       		phandle->end_hour ++;
                   		}
                	}
                	lv_label_set_text_fmt(phandle->end_hour_label, "%02d", phandle->end_hour);
				}
				else if(cur_obj == phandle->end_minute_btn)
				{
					if(phandle->end_minute >= 59)
                	{
                	    phandle->end_minute = 0;
                	}
                	else
                	{
                  	 	phandle->end_minute ++;
                	}
               		lv_label_set_text_fmt(phandle->end_minute_label, "%02d", phandle->end_minute);
				}
			}
			break;
		case DIVOOM_LVGL_KEY_ESC:
			if(phandle->btn_status == 0)
			{
				divoom_lvgl_setting_night_view_update_time(phandle);
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

void *divoom_lvgl_setting_night_view_init(void *user_data, int menu_id)
{
    Pdivoom_lvgl_setting_night_view_ctrl pmenu_handle = divoom_system_malloc(sizeof(divoom_lvgl_setting_night_view_ctrl));

	int active_id = 0, i, hour;
    void *image_obj, *label_obj, *slide_obj, *btn_obj;
    Pdivoom_system_setting_json_night_view pnight_view = divoom_main_get_night_view_info();
	pmenu_handle->pdirect_handle = divoom_menu_direct_ctrl_create(8, 1);
	pmenu_handle->btn_status = 0;
	pmenu_handle->first_active_obj = NULL;
	pmenu_handle->menu_id = menu_id;
    divoom_menu_control_init_lable_style(&pmenu_handle->normal_style);
    pmenu_handle->main_obj = lv_obj_create(NULL);  
    lv_obj_set_style_bg_color(pmenu_handle->main_obj, lv_color_hex(0xE4E4E4), LV_PART_MAIN);
    lv_obj_set_pos(pmenu_handle->main_obj, 0, 0);
	lv_obj_set_size(pmenu_handle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);  

    lv_obj_add_style(pmenu_handle->main_obj, &pmenu_handle->normal_style, 0);
    lv_obj_set_scrollbar_mode(pmenu_handle->main_obj, LV_SCROLLBAR_MODE_OFF);
    //lv_obj_set_scroll_dir(pmenu_handle->main_obj, LV_DIR_VER);
	///lv_obj_set_style_pad_bottom(pmenu_handle->main_obj, 71, 0);
	lv_obj_clear_flag(pmenu_handle->main_obj, LV_OBJ_FLAG_SCROLLABLE);

	/*
    image_obj = lv_img_create(pmenu_handle->main_obj);
	lv_obj_set_pos(image_obj, 19, 50);
    lv_obj_set_size(image_obj, 78, 78);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_back.png");
    pmenu_handle->back_obj = image_obj;
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_setting_night_view_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
    
    image_obj = lv_img_create(pmenu_handle->main_obj);
	lv_obj_set_pos(image_obj, 37, 209);
    lv_obj_set_size(image_obj, 726, 476);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_nightview_bg.png");
    */
	
    label_obj = lv_label_create(pmenu_handle->main_obj);
    lv_obj_set_pos(label_obj, 100, 50);
	lv_obj_set_size(label_obj, 600, 78);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("NightViewMode"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, pmenu_handle->menu_id), 0);  

	//divoom_menu_control_init_btn_stype3(&pmenu_handle->btn_select_style, NULL, &pmenu_handle->btn_normal_style, NULL, &pmenu_handle->btn_edit_style, NULL, 17);

	lv_style_init(&pmenu_handle->btn_normal_style);
	lv_style_set_bg_color(&pmenu_handle->btn_normal_style, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&pmenu_handle->btn_normal_style, 0);
	lv_style_set_border_width(&pmenu_handle->btn_normal_style, 4);
	lv_style_set_border_color(&pmenu_handle->btn_normal_style, lv_color_hex(0xE4E4E4));
	lv_style_set_radius(&pmenu_handle->btn_normal_style, 17);
	lv_style_set_shadow_width(&pmenu_handle->btn_normal_style, 0);
	lv_style_set_outline_width(&pmenu_handle->btn_normal_style, 0);

	lv_style_init(&pmenu_handle->btn_select_style);
	lv_style_set_bg_color(&pmenu_handle->btn_select_style, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&pmenu_handle->btn_select_style, 0);
	lv_style_set_border_width(&pmenu_handle->btn_select_style, 4);
	lv_style_set_border_color(&pmenu_handle->btn_select_style, lv_color_hex(0x6D6D6D));
	lv_style_set_radius(&pmenu_handle->btn_select_style, 17);
	lv_style_set_shadow_width(&pmenu_handle->btn_select_style, 0);
	lv_style_set_outline_width(&pmenu_handle->btn_select_style, 0);

	lv_style_init(&pmenu_handle->btn_edit_style);
	lv_style_set_bg_color(&pmenu_handle->btn_edit_style, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&pmenu_handle->btn_edit_style, 0);
	lv_style_set_border_width(&pmenu_handle->btn_edit_style, 4);
	lv_style_set_border_color(&pmenu_handle->btn_edit_style, lv_color_hex(0xE89B00));
	lv_style_set_radius(&pmenu_handle->btn_edit_style, 17);
	lv_style_set_shadow_width(&pmenu_handle->btn_edit_style, 0);
	lv_style_set_outline_width(&pmenu_handle->btn_edit_style, 0);

	lv_obj_t *cont = lv_obj_create(pmenu_handle->main_obj);
	lv_obj_remove_style(cont, NULL, LV_PART_SCROLLBAR);
    lv_obj_set_style_pad_all(cont, 0, 0);
    //lv_obj_set_style_pad_gap(cont, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_outline_width(cont, 0, 0);
    lv_obj_set_style_border_side(cont, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_pad_bottom(cont, 71, 0);
    lv_obj_set_size(cont, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH - 128);
    lv_obj_set_pos(cont, 0, 128);
	
	//夜览模式
	btn_obj = lv_btn_create(cont);
	lv_obj_add_style(btn_obj, &pmenu_handle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &pmenu_handle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &pmenu_handle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 164-128);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_setting_night_view_menu_event_cb, LV_EVENT_KEY, pmenu_handle); 
	divoom_menu_direct_ctrl_update(pmenu_handle->pdirect_handle, btn_obj, active_id, 0);
	active_id++;
	pmenu_handle->night_view_btn = btn_obj;
	if(pmenu_handle->first_active_obj == NULL)
	{
		pmenu_handle->first_active_obj = btn_obj;
	}
	
    label_obj = lv_label_create(cont);
    lv_obj_set_pos(label_obj, 76, 192-128);
	lv_obj_set_size(label_obj, 500, 34);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("NightViewMode"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, pmenu_handle->menu_id), 0);  
    
    image_obj = lv_img_create(cont);
	lv_obj_set_pos(image_obj, 652, 187-128);
    lv_obj_set_size(image_obj, 72, 44);
    if(pnight_view->on_off)
    {
    	pmenu_handle->night_view_on_off = 1;
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_on.png");
    }
    else
    {
    	pmenu_handle->night_view_on_off = 0;
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_off.png");
    }
    //lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_add_event_cb(image_obj, divoom_lvgl_setting_night_view_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
    pmenu_handle->switch_obj = image_obj;

    //亮度
	btn_obj = lv_btn_create(cont);
	lv_obj_add_style(btn_obj, &pmenu_handle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &pmenu_handle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &pmenu_handle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 346-128);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_setting_night_view_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
	divoom_menu_direct_ctrl_update(pmenu_handle->pdirect_handle, btn_obj, active_id, 0);
	active_id++;
	pmenu_handle->bright_btn = btn_obj;
	
    label_obj = lv_label_create(cont);
    lv_obj_set_pos(label_obj, 76, 302-128);
	lv_obj_set_size(label_obj, 310, 32);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("ScreenBright"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x636363), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(24, pmenu_handle->menu_id), 0);  
      
    image_obj = lv_img_create(cont);
	lv_obj_set_pos(image_obj, 76, 373-128);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_birght_min.png");
    
    image_obj = lv_img_create(cont);
	lv_obj_set_pos(image_obj, 688, 373-128);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_birght_max.png");
    
    slide_obj = lv_slider_create(cont);
    lv_slider_set_range(slide_obj, 0, 100);
    lv_obj_set_pos(slide_obj, 133, 391-128);
    lv_obj_set_size(slide_obj, 530, 4);
    lv_slider_set_value(slide_obj, pnight_view->bright, LV_ANIM_ON);
    lv_obj_set_style_bg_color(slide_obj, lv_color_hex(0x707070), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slide_obj, lv_color_hex(0xB1B1B1), LV_PART_MAIN);
    //lv_obj_add_flag(slide_obj, LV_OBJ_FLAG_CLICKABLE);
    ///lv_obj_add_event_cb(slide_obj, divoom_lvgl_setting_night_view_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
    pmenu_handle->bright_obj = slide_obj;
	pmenu_handle->bright = pnight_view->bright;

    /*label_obj = lv_label_create(pmenu_handle->main_obj);
    lv_obj_set_pos(label_obj, 80, 528);
	lv_obj_set_size(label_obj, 600, 42);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("StartEndTime"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32), 0);  
	*/
	
    //下面创建一个开始与结束的时间列表
    //开始时间
	label_obj = lv_label_create(cont);
    lv_obj_set_pos(label_obj, 76, 483-128);
	lv_obj_set_size(label_obj, 310, 32);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("StartTime"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x636363), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(24, pmenu_handle->menu_id), 0); 

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
			lv_obj_set_pos(btn, 44, 527+138*i-128);		
			lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

			lv_obj_add_style(btn, &pmenu_handle->btn_normal_style, 0);
		
			lv_obj_add_style(btn, &pmenu_handle->btn_select_style, LV_STATE_FOCUSED);
			lv_obj_add_style(btn, &pmenu_handle->btn_select_style, LV_STATE_FOCUS_KEY);
			lv_obj_set_style_pad_all(btn, 0, 0);
			lv_obj_add_event_cb(btn, divoom_lvgl_setting_night_view_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
			
			divoom_menu_direct_ctrl_update(pmenu_handle->pdirect_handle, btn, active_id, 0);
			active_id ++;

			lv_obj_t *time_label = lv_label_create(btn);
			lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(26, pmenu_handle->menu_id), 0);
			lv_obj_set_style_text_color(time_label, lv_color_hex(0x0), 0);
			lv_obj_set_size(time_label, 400, 34);
			lv_obj_set_pos(time_label, 32, 48);

			lv_obj_t *disp_obj = lv_obj_create(btn);
			lv_obj_set_style_pad_all(disp_obj, 0, 0);
			lv_obj_set_style_bg_color(disp_obj, lv_color_hex(0xFFFFFF), 0);
			lv_obj_set_style_border_color(disp_obj, lv_color_hex(0xDCDCDC), 0);
			lv_obj_set_style_border_width(disp_obj, 4, 0);
			lv_obj_set_style_radius(disp_obj, 14, 0);
			lv_obj_set_size(disp_obj, 180, 80);
			lv_obj_set_pos(disp_obj, 496, 25);
			lv_obj_clear_flag(disp_obj, LV_OBJ_FLAG_SCROLLABLE);

			lv_obj_t *disp_label = lv_label_create(disp_obj);
			lv_obj_set_style_text_font(disp_label, divoom_menu_control_get_font_info(26, pmenu_handle->menu_id), 0);
			lv_obj_set_style_text_color(disp_label, lv_color_hex(0x636363), 0);
			lv_obj_set_style_text_align(disp_label, LV_TEXT_ALIGN_CENTER, 0);
			lv_obj_set_size(disp_label, 180, 34);
			lv_obj_set_pos(disp_label, 0, 23);
			if(i == 0)//开始时间小时
			{
				pmenu_handle->start_hour = pnight_view->start/60;
				pmenu_handle->start_hour_btn = btn;
				pmenu_handle->start_hour_label = disp_label;
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Hour"));
        		lv_label_set_text_fmt(disp_label, "%02d", pmenu_handle->start_hour);
			}
			else if(i == 1)//开始时间分钟
			{
				pmenu_handle->start_minute = pnight_view->start%60;
				pmenu_handle->start_minute_btn = btn;
				pmenu_handle->start_minute_label = disp_label;
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Minute"));
				lv_label_set_text_fmt(disp_label, "%02d", pmenu_handle->start_minute);
			}
	
			lv_obj_t *up_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(up_img, "26/26_up.png");
			lv_obj_set_pos(up_img, 578, 4);
			lv_obj_t *down_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(down_img, "26/26_down.png");	
			lv_obj_set_pos(down_img, 578, 110);
		}
		pmenu_handle->start_am_pm_btn = NULL;
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
			lv_obj_set_pos(btn, 44, 527+138*i-128);		
			lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
			lv_obj_add_style(btn, &pmenu_handle->btn_normal_style, 0);
		
			lv_obj_add_style(btn, &pmenu_handle->btn_select_style, LV_STATE_FOCUSED);
			lv_obj_add_style(btn, &pmenu_handle->btn_select_style, LV_STATE_FOCUS_KEY);
			lv_obj_set_style_pad_all(btn, 0, 0);
			lv_obj_add_event_cb(btn, divoom_lvgl_setting_night_view_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
			
			divoom_menu_direct_ctrl_update(pmenu_handle->pdirect_handle, btn, active_id, 0);
			active_id ++;

			lv_obj_t *time_label = lv_label_create(btn);
			lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(26, pmenu_handle->menu_id), 0);
			lv_obj_set_style_text_color(time_label, lv_color_hex(0x0), 0);
			lv_obj_set_size(time_label, 400, 34);
			lv_obj_set_pos(time_label, 32, 48);

			lv_obj_t *disp_obj = lv_obj_create(btn);
			lv_obj_set_style_pad_all(disp_obj, 0, 0);
			lv_obj_set_style_bg_color(disp_obj, lv_color_hex(0xFFFFFF), 0);
			lv_obj_set_style_border_color(disp_obj, lv_color_hex(0xDCDCDC), 0);
			lv_obj_set_style_border_width(disp_obj, 4, 0);
			lv_obj_set_style_radius(disp_obj, 14, 0);
			lv_obj_set_size(disp_obj, 180, 80);
			lv_obj_set_pos(disp_obj, 496, 25);
			lv_obj_clear_flag(disp_obj, LV_OBJ_FLAG_SCROLLABLE);

			lv_obj_t *disp_label = lv_label_create(disp_obj);
			lv_obj_set_style_text_font(disp_label, divoom_menu_control_get_font_info(26, pmenu_handle->menu_id), 0);
			lv_obj_set_style_text_color(disp_label, lv_color_hex(0x636363), 0);
			lv_obj_set_style_text_align(disp_label, LV_TEXT_ALIGN_CENTER, 0);
			lv_obj_set_size(disp_label, 180, 34);
			lv_obj_set_pos(disp_label, 0, 23);
			if(i == 0)//开始时间时间段
			{
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("TimePeriod"));
				if(pnight_view->start/60 < 12)
				{
					pmenu_handle->start_am_pm_flag = 0;
					lv_label_set_text(disp_label, divoom_menu_control_get_disp_str("AM"));
				}
				else
				{
					pmenu_handle->start_am_pm_flag = 1;
					lv_label_set_text(disp_label, divoom_menu_control_get_disp_str("PM"));
				}
				pmenu_handle->start_am_pm_btn = btn;
				pmenu_handle->start_am_pm_label = disp_label;
			}
			else if(i == 1)//开始时间小时
			{
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Hour"));
				hour = pnight_view->start/60;
            	if(hour != 12)
            	{
               		hour = hour % 12;
                	if(hour == 0)
                	{
                    	hour = 12;
                	}
            	}
				pmenu_handle->start_hour = hour;
				pmenu_handle->start_hour_btn = btn;		//
				pmenu_handle->start_hour_label = disp_label;
				lv_label_set_text_fmt(disp_label, "%02d", hour);
			}
			else if(i == 2)//开始时间分钟
			{
				pmenu_handle->start_minute = pnight_view->start%60;
				pmenu_handle->start_minute_btn = btn;
				pmenu_handle->start_minute_label = disp_label;
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Minute"));
				lv_label_set_text_fmt(disp_label, "%02d", pmenu_handle->start_minute);
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
		lv_obj_set_pos(label_obj, 76, 843-128);
	}
	else
	{
		lv_obj_set_pos(label_obj, 76, 982-128);
	}
    
	lv_obj_set_size(label_obj, 310, 32);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("EndTime"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x636363), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(24, pmenu_handle->menu_id), 0);

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
			lv_obj_set_pos(btn, 44, 887+138*i-128);
			lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

			lv_obj_add_style(btn, &pmenu_handle->btn_normal_style, 0);
		
			lv_obj_add_style(btn, &pmenu_handle->btn_select_style, LV_STATE_FOCUSED);
			lv_obj_add_style(btn, &pmenu_handle->btn_select_style, LV_STATE_FOCUS_KEY);
			lv_obj_set_style_pad_all(btn, 0, 0);
			lv_obj_add_event_cb(btn, divoom_lvgl_setting_night_view_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
			
			divoom_menu_direct_ctrl_update(pmenu_handle->pdirect_handle, btn, active_id, 0);
			active_id ++;

			lv_obj_t *time_label = lv_label_create(btn);
			lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(26, pmenu_handle->menu_id), 0);
			lv_obj_set_style_text_color(time_label, lv_color_hex(0x0), 0);
			lv_obj_set_size(time_label, 400, 34);
			lv_obj_set_pos(time_label, 32, 48);

			lv_obj_t *disp_obj = lv_obj_create(btn);
			lv_obj_set_style_pad_all(disp_obj, 0, 0);
			lv_obj_set_style_bg_color(disp_obj, lv_color_hex(0xFFFFFF), 0);
			lv_obj_set_style_border_color(disp_obj, lv_color_hex(0xDCDCDC), 0);
			lv_obj_set_style_border_width(disp_obj, 4, 0);
			lv_obj_set_style_radius(disp_obj, 14, 0);
			lv_obj_set_size(disp_obj, 180, 80);
			lv_obj_set_pos(disp_obj, 496, 25);
			lv_obj_clear_flag(disp_obj, LV_OBJ_FLAG_SCROLLABLE);

			lv_obj_t *disp_label = lv_label_create(disp_obj);
			lv_obj_set_style_text_font(disp_label, divoom_menu_control_get_font_info(26, pmenu_handle->menu_id), 0);
			lv_obj_set_style_text_color(disp_label, lv_color_hex(0x636363), 0);
			lv_obj_set_style_text_align(disp_label, LV_TEXT_ALIGN_CENTER, 0);
			lv_obj_set_size(disp_label, 180, 34);
			lv_obj_set_pos(disp_label, 0, 23);
			if(i == 0)//结束时间小时
			{
				pmenu_handle->end_hour = pnight_view->end/60;
				pmenu_handle->end_hour_btn = btn;
				pmenu_handle->end_hour_label = disp_label;
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Hour"));
        		lv_label_set_text_fmt(disp_label, "%02d", pmenu_handle->end_hour);
			}
			else if(i == 1)//结束时间分钟
			{
				pmenu_handle->end_minute = pnight_view->end%60;
				pmenu_handle->end_minute_btn = btn;
				pmenu_handle->end_minute_label = disp_label;
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Minute"));
				lv_label_set_text_fmt(disp_label, "%02d", pmenu_handle->end_minute);
			}
	
			lv_obj_t *up_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(up_img, "26/26_up.png");
			lv_obj_set_pos(up_img, 578, 4);
			lv_obj_t *down_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(down_img, "26/26_down.png");	
			lv_obj_set_pos(down_img, 578, 110);
		}
		pmenu_handle->end_am_pm_btn = NULL;
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
			lv_obj_set_pos(btn, 44, 1026+138*i-128);
			lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
			lv_obj_add_style(btn, &pmenu_handle->btn_normal_style, 0);
		
			lv_obj_add_style(btn, &pmenu_handle->btn_select_style, LV_STATE_FOCUSED);
			lv_obj_add_style(btn, &pmenu_handle->btn_select_style, LV_STATE_FOCUS_KEY);
			lv_obj_set_style_pad_all(btn, 0, 0);
			lv_obj_add_event_cb(btn, divoom_lvgl_setting_night_view_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
			
			divoom_menu_direct_ctrl_update(pmenu_handle->pdirect_handle, btn, active_id, 0);
			active_id ++;

			lv_obj_t *time_label = lv_label_create(btn);
			lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(26, pmenu_handle->menu_id), 0);
			lv_obj_set_style_text_color(time_label, lv_color_hex(0x0), 0);
			lv_obj_set_size(time_label, 400, 34);
			lv_obj_set_pos(time_label, 32, 48);

			lv_obj_t *disp_obj = lv_obj_create(btn);
			lv_obj_set_style_pad_all(disp_obj, 0, 0);
			lv_obj_set_style_bg_color(disp_obj, lv_color_hex(0xFFFFFF), 0);
			lv_obj_set_style_border_color(disp_obj, lv_color_hex(0xDCDCDC), 0);
			lv_obj_set_style_border_width(disp_obj, 4, 0);
			lv_obj_set_style_radius(disp_obj, 14, 0);
			lv_obj_set_size(disp_obj, 180, 80);
			lv_obj_set_pos(disp_obj, 496, 25);
			lv_obj_clear_flag(disp_obj, LV_OBJ_FLAG_SCROLLABLE);

			lv_obj_t *disp_label = lv_label_create(disp_obj);
			lv_obj_set_style_text_font(disp_label, divoom_menu_control_get_font_info(26, pmenu_handle->menu_id), 0);
			lv_obj_set_style_text_color(disp_label, lv_color_hex(0x636363), 0);
			lv_obj_set_style_text_align(disp_label, LV_TEXT_ALIGN_CENTER, 0);
			lv_obj_set_size(disp_label, 180, 34);
			lv_obj_set_pos(disp_label, 0, 23);
			if(i == 0)//结束时间时间段
			{
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("TimePeriod"));
				if(pnight_view->end/60 < 12)
				{
					pmenu_handle->end_am_pm_flag = 0;
					lv_label_set_text(disp_label, divoom_menu_control_get_disp_str("AM"));
				}
				else
				{
					pmenu_handle->end_am_pm_flag = 1;
					lv_label_set_text(disp_label, divoom_menu_control_get_disp_str("PM"));
				}
				pmenu_handle->end_am_pm_btn = btn;
				pmenu_handle->end_am_pm_label = disp_label;
			}
			else if(i == 1)//结束时间小时
			{
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Hour"));
				hour = pnight_view->end/60;
            	if(hour != 12)
            	{
               		hour = hour % 12;
                	if(hour == 0)
                	{
                    	hour = 12;
                	}
            	}
				pmenu_handle->end_hour = hour;
				pmenu_handle->end_hour_btn = btn;
				pmenu_handle->end_hour_label = disp_label;
				lv_label_set_text_fmt(disp_label, "%02d", hour);
			}
			else if(i == 2)//结束时间分钟
			{
				pmenu_handle->end_minute = pnight_view->end%60;
				pmenu_handle->end_minute_btn = btn;	
				pmenu_handle->end_minute_label = disp_label;
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Minute"));
				lv_label_set_text_fmt(disp_label, "%02d", pmenu_handle->end_minute);
			}
	
			lv_obj_t *up_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(up_img, "26/26_up.png");
			lv_obj_set_pos(up_img, 578, 4);
			lv_obj_t *down_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(down_img, "26/26_down.png");	
			lv_obj_set_pos(down_img, 578, 110);
		}
	}
	
    return pmenu_handle;
}

void divoom_lvgl_setting_night_view_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_setting_night_view_ctrl phandle = (Pdivoom_lvgl_setting_night_view_ctrl)handle;

	if(phandle->first_active_obj)
	{
		lv_group_focus_obj(phandle->first_active_obj);
		phandle->first_active_obj = NULL;
	}
    lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_setting_night_view_exit(void *handle)
{
    Pdivoom_lvgl_setting_night_view_ctrl phandle = (Pdivoom_lvgl_setting_night_view_ctrl)handle;

    lv_obj_del(phandle->main_obj);
    lv_style_reset(&phandle->normal_style);
	lv_style_reset(&phandle->btn_normal_style);
	lv_style_reset(&phandle->btn_edit_style);
	lv_style_reset(&phandle->btn_select_style);
	divoom_menu_direct_ctrl_destory(phandle->pdirect_handle);
    divoom_system_free(phandle);
    return ;
}




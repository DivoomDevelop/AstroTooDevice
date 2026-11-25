#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_top_main_menu.h"

typedef struct _divoom_alarm_menu_item
{
	void *alarm_cout_obj;
	void *alarm_btn_obj;
    void *ampm_lable_obj;
	void *time_label_obj;
    void *week_lable_obj;
	void *switch_btn_obj;
    void *swtich_image_obj;
}divoom_alarm_menu_item, *Pdivoom_alarm_menu_item;


typedef struct _divoom_lvgl_alarm_menu_ctrl
{
    int menu_id;
	int revise_alarm;
	int alarm_num;
	lv_style_t backgroud_type;
	void *main_obj;
	void *cout_obj;

	lv_style_t cont_type;
	lv_coord_t col_dsc[2];
    lv_coord_t row_dsc[50];

	int btn_status; // 0:未选中 1:选中
	int select_id;
	lv_style_t press_type;
	lv_style_t normal_type;
	lv_style_t select_press_type;
	void *first_active_obj;

	void *time_obj;
	
	T_DIVOOM_ALARM_ACTIVE_DO_INFO *palarm_info;
	Pdivoom_alarm_menu_item pmenu_obj_list;
	void *pdirect_handle;
	void *timer_handle;
	int hour;
	int minute;
	lv_timer_t *menu_bar_timer;
}divoom_lvgl_alarm_menu_ctrl, *Pdivoom_lvgl_alarm_menu_ctrl;

/*void divoom_alarm_menu_back_event_cb(lv_event_t *e)
{
	Pdivoom_lvgl_alarm_menu_ctrl phandle = (Pdivoom_lvgl_alarm_menu_ctrl)lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);

	switch(code)
	{
		case LV_EVENT_CLICKED:
			divoom_menu_control_return_pre();
			break;

        default:
            break;

	}
}

void divoom_alarm_menu_add_event_cb(lv_event_t *e)
{
	Pdivoom_lvgl_alarm_menu_ctrl phandle = (Pdivoom_lvgl_alarm_menu_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
	switch(code)
	{
		case LV_EVENT_CLICKED:
			phandle->revise_alarm = 1;
			divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_ALARM_ADD_MENU, NULL, 0, 0);
			break;

        default:
            break;

	}
}

void divoom_alarm_menu_switch_event_cb(lv_event_t *e)
{
	int i;
	Pdivoom_lvgl_alarm_menu_ctrl phandle = (Pdivoom_lvgl_alarm_menu_ctrl)lv_event_get_user_data(e);
	lv_obj_t *cur_btn_obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
	switch(code)
	{
		case LV_EVENT_PRESSED:
			for(i = 0;i < phandle->alarm_num; i++)
			{
				if(phandle->pmenu_obj_list[i].switch_btn_obj== cur_btn_obj)
				{
					break;
				}
			}
			if(i >= phandle->alarm_num)
			{
				return;
			}
			if(phandle->palarm_info[i].on_off == 1)
			{
				phandle->palarm_info[i].on_off = 0;
				divoom_menu_control_lv_img_set_src(phandle->pmenu_obj_list[i].swtich_image_obj, "23/23_off.png");
			}
			else if(phandle->palarm_info[i].on_off == 0)
			{
				phandle->palarm_info[i].on_off = 1;
				divoom_menu_control_lv_img_set_src(phandle->pmenu_obj_list[i].swtich_image_obj, "23/23_on.png");
			}
			divoom_alarm_menu_update_alarm_item(&phandle->palarm_info[i]);
			break;

        default:
            break;

	}

}*/

void divoom_alarm_menu_edit_event_cb(lv_event_t *e)
{
	int i;
	Pdivoom_lvgl_alarm_menu_ctrl phandle = (Pdivoom_lvgl_alarm_menu_ctrl)lv_event_get_user_data(e);
	lv_obj_t *cur_btn_obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
	int cur_pos = divoom_menu_direct_ctrl_get_pos(phandle->pdirect_handle, cur_btn_obj);
	
	if(code == LV_EVENT_KEY)
	{
		char c = *((char *)lv_event_get_param(e));

		if(c == DIVOOM_LVGL_KEY_PRE)
		{
			if(phandle->btn_status == 0)
			{
				cur_btn_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_btn_obj, DIVOOM_MENU_DIRECT_DOWN);
				lv_group_focus_obj(cur_btn_obj);
				for(i = 0;i < phandle->alarm_num; i++)
				{
					if(cur_btn_obj == phandle->pmenu_obj_list[i].alarm_btn_obj)
					{
						phandle->select_id = i;
						break;
					}
				}
			}
			else//选中
			{
				
			}
		}         
		else if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			if(phandle->btn_status == 0)
			{
				cur_btn_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_btn_obj, DIVOOM_MENU_DIRECT_UP);
				lv_group_focus_obj(cur_btn_obj);
				for(i = 0;i < phandle->alarm_num; i++)
				{
					if(cur_btn_obj == phandle->pmenu_obj_list[i].alarm_btn_obj)
					{
						phandle->select_id = i;
						break;
					}
				}
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			if(phandle->btn_status == 0)
			{
				divoom_menu_control_return_pre();
			}
			//else if(phandle->btn_status == 1)
			//{
			//	phandle->btn_status = 0;
			//	lv_obj_add_style(phandle->pmenu_obj_list[phandle->select_id].alarm_btn_obj, &phandle->press_type, LV_STATE_FOCUSED);
            //  lv_obj_add_style(phandle->pmenu_obj_list[phandle->select_id].alarm_btn_obj, &phandle->press_type, LV_STATE_FOCUS_KEY);
			//}
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			//if(phandle->btn_status == 0)
			//{
			//	phandle->btn_status = 1;
			//	lv_obj_add_style(phandle->pmenu_obj_list[phandle->select_id].alarm_btn_obj, &phandle->select_press_type, LV_STATE_FOCUSED);
            //  lv_obj_add_style(phandle->pmenu_obj_list[phandle->select_id].alarm_btn_obj, &phandle->select_press_type, LV_STATE_FOCUS_KEY);
			//}
			//else
			//{
				if(phandle->alarm_num == 0)
				{
					return;
				}
				phandle->revise_alarm = 1;
				divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_ALARM_EDIT_MENU, &phandle->palarm_info[cur_pos], 0, 0);
			//}
		}		
		else
		{
			char *key_info = (char *)lv_event_get_param(e);
			divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
		}
	}
}

void divoom_alarm_menu_create_week_label_table(Pdivoom_lvgl_alarm_menu_ctrl phandle, Pdivoom_alarm_menu_item  pmenu_obj_list, int week)
{
    int i, off_x = 6, index = 0, len = 0;
	char disp_str[60];
	char week_cyc[7][10] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    void *week_label_obj;

    if(pmenu_obj_list->week_lable_obj)
    {
        lv_obj_del(pmenu_obj_list->week_lable_obj);
        pmenu_obj_list->week_lable_obj = NULL;
    }

    week_label_obj = lv_label_create(pmenu_obj_list->alarm_btn_obj);
    pmenu_obj_list->week_lable_obj = week_label_obj;
    lv_obj_set_style_text_font(week_label_obj, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
    lv_obj_set_pos(week_label_obj, 36, 95);
    lv_obj_set_size(week_label_obj, 641, 32);
    lv_obj_set_style_text_color(week_label_obj, lv_color_hex(0x7A7A7A), 0);
	if(week == 0x00)
	{
		lv_label_set_text(week_label_obj, divoom_menu_control_get_disp_str("Never"));
        return;
	}
    else if(week == 0x7f)
    {
        lv_label_set_text(week_label_obj, divoom_menu_control_get_disp_str("every day"));
        return;
    }
    else if(week == 0x3e)
    {
        lv_label_set_text(week_label_obj, divoom_menu_control_get_disp_str("every weekday"));
        return;
	}
	else if(week == 0x41)
	{
		lv_label_set_text(week_label_obj, divoom_menu_control_get_disp_str("every weekend"));
        return;
	}
	
    for(i = 0; i < 7; i++)
    {
        if(week & ( 1 << ((i + 1)%7)))
        {
			len = sprintf(disp_str + index, "%s ", divoom_menu_control_get_disp_str(week_cyc[i]));
            index += len;
        }
    }
	if(index > 0)
	{
		disp_str[index - 1] = '\0';
		lv_label_set_text(pmenu_obj_list->week_lable_obj, divoom_menu_control_get_disp_str(disp_str));
	}
}

void divoom_menu_alarm_refresh_menu_bar(lv_timer_t *menu_bar_timer)
{
    int hour;
    Pdivoom_lvgl_alarm_menu_ctrl phandle = (Pdivoom_lvgl_alarm_menu_ctrl)lv_timer_get_user_data(menu_bar_timer);
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
        lv_label_set_text_fmt(phandle->time_obj, "%02d:%02d", hour, phandle->minute);
    }

}




void divoom_alarm_menu_create_title_area(Pdivoom_lvgl_alarm_menu_ctrl phandle)
{
	int hour;
	char disp_str[32];
	time_t time_now;
	struct tm p_now;
	
	lv_style_init(&phandle->cont_type);
	lv_style_set_pad_all(&phandle->cont_type, 0);
	lv_style_set_pad_gap(&phandle->cont_type, 0);
	lv_style_set_border_width(&phandle->cont_type, 0);
	lv_style_set_outline_width(&phandle->cont_type, 0);
	lv_style_set_border_side(&phandle->cont_type, LV_BORDER_SIDE_NONE);
	lv_style_set_bg_color(&phandle->cont_type, lv_color_hex(0xE4E4E4));
	
	/*lv_obj_t *title = lv_obj_create(phandle->main_obj);
	lv_obj_set_style_bg_opa(title, LV_OPA_0, 0);
	lv_obj_set_style_border_width(title, 0, 0);
	lv_obj_set_style_pad_all(title, 0, 0);
	lv_obj_set_style_text_color(title, lv_color_white(), 0);
	lv_obj_clear_flag(title, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(title, 800, 78); 
	lv_obj_set_pos(title, 0, 40);
	
	lv_obj_t *back_btn = lv_btn_create(title);
	lv_obj_set_size(back_btn, 48, 48); 
	lv_obj_set_pos(back_btn, 25, 15);
	lv_obj_set_style_bg_opa(back_btn, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(back_btn, 0, 0);
	lv_obj_set_style_shadow_width(back_btn, 0, 0);
	lv_obj_clear_flag(back_btn, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_event(back_btn, divoom_alarm_menu_back_event_cb, LV_EVENT_CLICKED, phandle);
		
	lv_obj_t *back_img = lv_img_create(back_btn);
	lv_obj_set_size(back_img, 48, 48); 
	divoom_menu_control_lv_img_set_src(back_img, "23/23_back.png");
	*/
	
	lv_obj_t *alarm_label = lv_label_create(phandle->main_obj);
	lv_obj_set_size(alarm_label, 200, 40);
	lv_obj_set_pos(alarm_label, 367, 51);
	lv_obj_set_style_text_font(alarm_label, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	//lv_obj_set_style_text_align(alarm_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_text(alarm_label, divoom_menu_control_get_disp_str("Alarm"));
	
		
	lv_obj_t *time_label = lv_label_create(phandle->main_obj);
	lv_obj_set_size(time_label, 160, 40); 
	lv_obj_set_pos(time_label, 40, 51);
	lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
		
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
	
	sprintf(disp_str, "%02d:%02d", hour, p_now.tm_min);
	lv_label_set_text(time_label, disp_str);
	phandle->time_obj = time_label;
	phandle->menu_bar_timer = lv_timer_create(divoom_menu_alarm_refresh_menu_bar, 1000, phandle);
}


void divoom_alarm_menu_create_image_grad(Pdivoom_lvgl_alarm_menu_ctrl phandle)
{
	int i, j, menu_items, y = 0;
	char disp_str[32];

	//divoom_menu_control_init_btn_stype(&phandle->press_type, NULL, 
    //                                        &phandle->normal_type, NULL, 
    //                                        &phandle->select_press_type, NULL, 32);
    
	lv_style_init(&phandle->normal_type);
	lv_style_set_bg_color(&phandle->normal_type, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->normal_type, 0);
	lv_style_set_border_width(&phandle->normal_type, 4);
	lv_style_set_border_color(&phandle->normal_type, lv_color_hex(0xE4E4E4));
	lv_style_set_radius(&phandle->normal_type, 32);
	lv_style_set_shadow_width(&phandle->normal_type, 0);
	lv_style_set_outline_width(&phandle->normal_type, 0);

	lv_style_init(&phandle->press_type);
	lv_style_set_bg_color(&phandle->press_type, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->press_type, 0);
	lv_style_set_border_width(&phandle->press_type, 4);
	lv_style_set_border_color(&phandle->press_type, lv_color_hex(0x6D6D6D));
	lv_style_set_radius(&phandle->press_type, 32);
	lv_style_set_shadow_width(&phandle->press_type, 0);
	lv_style_set_outline_width(&phandle->press_type, 0);

	lv_style_init(&phandle->select_press_type);
	lv_style_set_bg_color(&phandle->select_press_type, lv_color_hex(0x181818));
	lv_style_set_pad_all(&phandle->select_press_type, 0);
	lv_style_set_border_width(&phandle->select_press_type, 4);
	lv_style_set_border_color(&phandle->select_press_type, lv_color_hex(0xFFC300));
	lv_style_set_radius(&phandle->select_press_type, 32);
	lv_style_set_shadow_width(&phandle->select_press_type, 0);
	lv_style_set_outline_width(&phandle->select_press_type, 0);
											
	//闹钟列表
	lv_obj_t * cont = lv_obj_create(phandle->main_obj);
	lv_obj_remove_style(cont, NULL, LV_PART_SCROLLBAR);
	lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_0, 0);
    lv_obj_set_size(cont, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH - 148);
    lv_obj_set_pos(cont, 0, 148);
	lv_obj_set_style_pad_all(cont, 0, 0);
	lv_obj_set_style_shadow_width(cont, 0, 0);
	lv_obj_set_style_outline_width(cont, 0, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	//lv_obj_set_style_pad_row(cont, 16, 0);
	lv_obj_set_style_pad_bottom(cont, 98, 0);
	lv_obj_set_style_base_dir(cont, LV_BASE_DIR_AUTO, 0);
	lv_obj_add_event_cb(cont, divoom_alarm_menu_edit_event_cb, LV_EVENT_KEY, phandle);
	
	phandle->cout_obj = cont;
#if 1	
	menu_items = phandle->alarm_num;
	phandle->col_dsc[0] = DIVOOM_SCREEN_WIDTH;
    phandle->col_dsc[1] = LV_GRID_TEMPLATE_LAST;
    for(i = 0; i < menu_items; i++)
    {
        phandle->row_dsc[i] = 176;
    }
	phandle->row_dsc[i] = LV_GRID_TEMPLATE_LAST;
	if(phandle->alarm_num > 0)
	{
		lv_obj_set_grid_dsc_array(cont, phandle->col_dsc, phandle->row_dsc);
	}
	for(j = 0; j < menu_items; j ++)
    {
        lv_obj_t * obj = lv_obj_create(cont);
        lv_obj_add_style(obj, &phandle->cont_type, 0);
		
        lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_CENTER, 0, 1,
                             LV_GRID_ALIGN_STRETCH, j, 1);
							 
		lv_obj_set_size(obj, 720, 160);
		lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
		phandle->pmenu_obj_list[j].alarm_cout_obj = obj;
		
		lv_obj_t *btn_obj = lv_btn_create(obj);
		lv_obj_clear_flag(btn_obj, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(btn_obj, 712, 150);
        //lv_obj_add_style(btn_obj, &phandle->btn_normal_type, 0);
        //lv_obj_add_style(btn_obj, &phandle->btn_press_type, LV_STATE_PRESSED);
		//lv_obj_set_style_radius(btn_obj, 45, 0);
		//lv_obj_add_event(btn_obj, divoom_alarm_menu_edit_event_cb, LV_EVENT_CLICKED, phandle);
		lv_obj_add_style(btn_obj, &phandle->normal_type, 0);
    	lv_obj_add_style(btn_obj, &phandle->press_type, LV_STATE_FOCUSED);
    	lv_obj_add_style(btn_obj, &phandle->press_type, LV_STATE_FOCUS_KEY);
		lv_obj_set_style_pad_all(btn_obj, 0, 0);
		lv_obj_add_event_cb(btn_obj, divoom_alarm_menu_edit_event_cb, LV_EVENT_KEY, phandle);
		if(phandle->first_active_obj == NULL)
        {
            phandle->first_active_obj = btn_obj;
        }
		divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, j);
		
		phandle->pmenu_obj_list[j].alarm_btn_obj = btn_obj;

		lv_obj_t *time_label = lv_label_create(btn_obj);
		lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(56, phandle->menu_id), 0);
		lv_obj_set_style_text_color(time_label, lv_color_hex(0x454545), 0);

		lv_obj_t *time_info = lv_label_create(btn_obj);
		//lv_obj_set_style_text_color(time_info, lv_color_hex(0xFFFFFF), 0);
		lv_obj_set_style_text_font(time_info, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
		//lv_obj_set_style_text_opa(time_info, LV_OPA_100 * 56/100, 0);
		lv_obj_set_style_text_color(time_info, lv_color_hex(0x7A7A7A), 0);
		
		if(divoom_main_get_time_flag())
        {
        	lv_obj_add_flag(time_info, LV_OBJ_FLAG_HIDDEN);
			lv_obj_set_size(time_label, 450, 60);
			lv_obj_set_pos(time_label, 36, 23-5);
            sprintf(disp_str, "%02d:%02d", phandle->palarm_info[j].hour, phandle->palarm_info[j].minute);
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str(disp_str));
        }
        else
        {
        	lv_obj_set_pos(time_info, 36, 23+28-5);
        	lv_obj_set_size(time_info, 110-40, 60);

			lv_obj_set_pos(time_label, 150-50, 23-5);
			lv_obj_set_size(time_label, 400, 60);
			
            int hour = phandle->palarm_info[j].hour;
            if(hour != 12)
            {
                hour = hour % 12;
                if(hour == 0)
                {
                    hour = 12;
                }
            }
			sprintf(disp_str, "%02d:%02d", hour, phandle->palarm_info[j].minute);
            if(phandle->palarm_info[j].hour >= 12)
            {
            	lv_label_set_text(time_info, divoom_menu_control_get_disp_str("PM"));
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str(disp_str));
            }
            else
            {
            	lv_label_set_text(time_info, divoom_menu_control_get_disp_str("AM"));
				lv_label_set_text(time_label, divoom_menu_control_get_disp_str(disp_str));
            }
			
        }
		
		phandle->pmenu_obj_list[j].ampm_lable_obj = time_info;
		phandle->pmenu_obj_list[j].time_label_obj = time_label;
		divoom_alarm_menu_create_week_label_table(phandle,&phandle->pmenu_obj_list[j], phandle->palarm_info[j].cyc_flag);
		
		/*lv_obj_t *switch_btn = lv_btn_create(btn_obj);
		lv_obj_set_style_pad_all(switch_btn, 0, 0);
		lv_obj_set_style_bg_opa(switch_btn, LV_OPA_0, 0);
		lv_obj_set_style_shadow_width(switch_btn, 0, 0);
		lv_obj_set_style_outline_width(switch_btn, 0, 0);
		lv_obj_set_style_border_width(switch_btn, 0, 0);
		lv_obj_set_pos(switch_btn, 604, 54);
		lv_obj_set_size(switch_btn, 72, 44);*/

		
		lv_obj_t *switch_img_obj = lv_img_create(btn_obj);
		lv_obj_set_pos(switch_img_obj, 604, 54);
		lv_obj_set_size(switch_img_obj, 72, 44);
		if(phandle->palarm_info[j].on_off)
		{
			divoom_menu_control_lv_img_set_src(switch_img_obj, "23/23_on.png");
		}
		else
		{
			divoom_menu_control_lv_img_set_src(switch_img_obj, "23/23_off.png");
		}
		//phandle->pmenu_obj_list[j].switch_btn_obj = switch_btn;
		phandle->pmenu_obj_list[j].swtich_image_obj = switch_img_obj;
		//lv_obj_add_event_cb(switch_btn, divoom_alarm_menu_switch_event_cb, LV_EVENT_PRESSED, phandle);
	}
#endif	
	//添加闹钟
	//lv_obj_t *add_btn = lv_btn_create(phandle->main_obj);
	/*lv_obj_t *add_btn = lv_btn_create(cont);
	lv_obj_set_size(add_btn, 180, 180);
	lv_obj_clear_flag(add_btn, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_event(add_btn, divoom_alarm_menu_add_event_cb, LV_EVENT_CLICKED, phandle);
	lv_obj_set_style_bg_opa(add_btn, LV_OPA_0, 0);
	lv_obj_set_style_radius(add_btn, 99, 0);
	lv_obj_set_style_pad_all(add_btn, 0, 0);
	lv_obj_set_style_shadow_width(add_btn, 0, 0);
	lv_obj_set_style_bg_color(add_btn, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(add_btn, LV_OPA_10, LV_STATE_PRESSED);
	lv_obj_add_flag(add_btn, LV_OBJ_FLAG_IGNORE_LAYOUT);

	lv_obj_t *add_img = lv_img_create(add_btn);
	lv_obj_set_style_radius(add_img, 99, 0);
	divoom_menu_control_lv_img_set_src(add_img, "23/23_add.png");

	if(phandle->alarm_num == 0)
	{
		lv_obj_set_pos(add_btn, 310, 378);
	}
	else
	{
		lv_obj_align_to(add_btn, phandle->pmenu_obj_list[menu_items - 1].alarm_btn_obj, LV_ALIGN_OUT_BOTTOM_LEFT, 270, 62);	
	}*/
}

void divoom_alarm_menu_refresh_timer_handle(lv_timer_t *timer_handle)
{
    char disp_str[32];
    int j, alarm_num;
    T_DIVOOM_ALARM_ACTIVE_DO_INFO *palarm_info;
	Pdivoom_lvgl_alarm_menu_ctrl phandle = (Pdivoom_lvgl_alarm_menu_ctrl)lv_timer_get_user_data(timer_handle);

	if(lv_scr_act() != phandle->main_obj)
    {
        return;
    }

	palarm_info = divoom_alarm_menu_get_alarm_list(&alarm_num);

	if(alarm_num != phandle->alarm_num)
    {
    	divoom_system_free(palarm_info);
		return;
    }
	if(memcmp(palarm_info, phandle->palarm_info, alarm_num * sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO)) == 0)
	{
		divoom_system_free(palarm_info);
		return;
	}
	divoom_system_free(phandle->palarm_info);
	phandle->palarm_info = palarm_info;
	phandle->alarm_num = alarm_num;

    for(j = 0; j < phandle->alarm_num; j++)
    {
        if(phandle->palarm_info[j].on_off)
        {
            divoom_menu_control_lv_img_set_src(phandle->pmenu_obj_list[j].swtich_image_obj, "23/23_on.png");
        }
        else
        {
            divoom_menu_control_lv_img_set_src(phandle->pmenu_obj_list[j].swtich_image_obj, "23/23_off.png");
        }
        if(divoom_main_get_time_flag())
        {
            lv_label_set_text_fmt(phandle->pmenu_obj_list[j].time_label_obj, "%02d:%02d", phandle->palarm_info[j].hour, phandle->palarm_info[j].minute);
        }
        else
        {
            int hour = phandle->palarm_info[j].hour;
            if(hour != 12)
            {
                hour = hour % 12;
                if(hour == 0)
                {
                    hour = 12;
                }
            }
            if(phandle->palarm_info[j].hour >= 12)
            {
            	lv_label_set_text(phandle->pmenu_obj_list[j].ampm_lable_obj, divoom_menu_control_get_disp_str("PM"));
                lv_label_set_text_fmt(phandle->pmenu_obj_list[j].time_label_obj, "%02d:%02d", hour, phandle->palarm_info[j].minute);
            }
            else
            {
            	lv_label_set_text(phandle->pmenu_obj_list[j].ampm_lable_obj, divoom_menu_control_get_disp_str("AM"));
                lv_label_set_text_fmt(phandle->pmenu_obj_list[j].time_label_obj, "%02d:%02d", hour, phandle->palarm_info[j].minute);
            }
        }
        divoom_alarm_menu_create_week_label_table(phandle, &phandle->pmenu_obj_list[j], phandle->palarm_info[j].cyc_flag);
    }
	
	/*if(phandle->cout_obj)
	{
		lv_obj_del(phandle->cout_obj);
	}
	divoom_system_free(phandle->palarm_info);
	divoom_system_free(phandle->pmenu_obj_list);
	phandle->alarm_num = alarm_num;
	phandle->pmenu_obj_list = (Pdivoom_alarm_menu_item)divoom_system_malloc(sizeof(divoom_alarm_menu_item)*phandle->alarm_num);
	phandle->palarm_info = palarm_info;
	divoom_alarm_menu_create_image_grad(phandle);
	lv_group_focus_obj(phandle->pmenu_obj_list[phandle->select_id].alarm_btn_obj);*/
	
}

void *divoom_lvgl_alarm_menu_init(void *user_data, int menu_id)
{
	Pdivoom_lvgl_alarm_menu_ctrl phandle = (Pdivoom_lvgl_alarm_menu_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_alarm_menu_ctrl));

	phandle->palarm_info = divoom_alarm_menu_get_alarm_list(&phandle->alarm_num);
	phandle->pmenu_obj_list = (Pdivoom_alarm_menu_item)divoom_system_malloc(sizeof(divoom_alarm_menu_item)*phandle->alarm_num);
	phandle->pdirect_handle = divoom_menu_direct_ctrl_create(1, phandle->alarm_num);
	phandle->revise_alarm = 0;
	phandle->btn_status = 0;
	phandle->select_id = 0;
	phandle->first_active_obj = NULL;
	phandle->main_obj = lv_obj_create(NULL);
    phandle->menu_id = menu_id;
	lv_style_init(&phandle->backgroud_type);
    lv_style_set_bg_color(&phandle->backgroud_type, lv_color_hex(0xE4E4E4));
    lv_style_set_text_color(&phandle->backgroud_type, lv_color_hex(0x0));
    lv_obj_add_style(phandle->main_obj, &phandle->backgroud_type, 0);
    lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);

	divoom_alarm_menu_create_title_area(phandle);
	divoom_alarm_menu_create_image_grad(phandle);
	phandle->timer_handle = lv_timer_create(divoom_alarm_menu_refresh_timer_handle, 500, phandle);
	
	return phandle;
}

void divoom_lvgl_alarm_menu_disp(void *handle, void *active)
{
	int alarm_num, scroll_pos;
	T_DIVOOM_ALARM_ACTIVE_DO_INFO *palarm_info;
	Pdivoom_lvgl_alarm_menu_ctrl phandle = (Pdivoom_lvgl_alarm_menu_ctrl)handle;

	
	if(phandle->first_active_obj)
	{
		lv_group_focus_obj(phandle->first_active_obj);
		phandle->first_active_obj = NULL;
	}
	
	if(active)
	{	
		if(phandle->revise_alarm)
		{
			phandle->revise_alarm = 0;
			/*palarm_info = divoom_alarm_menu_get_alarm_list(&alarm_num);

			if(phandle->alarm_num != alarm_num)
			{
				phandle->select_id = 0;
			}
			if(phandle->cout_obj)
			{
				lv_obj_del(phandle->cout_obj);
			}
			divoom_system_free(phandle->palarm_info);
			divoom_system_free(phandle->pmenu_obj_list);
			divoom_menu_direct_ctrl_destory(phandle->pdirect_handle);
			phandle->alarm_num = alarm_num;
			phandle->pmenu_obj_list = (Pdivoom_alarm_menu_item)divoom_system_malloc(sizeof(divoom_alarm_menu_item)*phandle->alarm_num);
			phandle->palarm_info = palarm_info;
			phandle->pdirect_handle = divoom_menu_direct_ctrl_create(1, phandle->alarm_num);
			divoom_alarm_menu_create_image_grad(phandle);
			if(phandle->alarm_num != 0)
			{
				lv_group_focus_obj(phandle->pmenu_obj_list[phandle->select_id].alarm_btn_obj);
			}*/
			
			if(phandle->palarm_info[phandle->select_id].on_off)
			{
				divoom_menu_control_lv_img_set_src(phandle->pmenu_obj_list[phandle->select_id].swtich_image_obj, "23/23_on.png");
			}
			else
			{
				divoom_menu_control_lv_img_set_src(phandle->pmenu_obj_list[phandle->select_id].swtich_image_obj, "23/23_off.png");
			}
			if(divoom_main_get_time_flag())
			{
				lv_label_set_text_fmt(phandle->pmenu_obj_list[phandle->select_id].time_label_obj, "%02d:%02d", phandle->palarm_info[phandle->select_id].hour, phandle->palarm_info[phandle->select_id].minute);
			}
			else
			{
				int hour = phandle->palarm_info[phandle->select_id].hour;
				if(hour != 12)
				{
					hour = hour % 12;
					if(hour == 0)
					{
						hour = 12;
					}
				}
				if(phandle->palarm_info[phandle->select_id].hour >= 12)
				{
					lv_label_set_text(phandle->pmenu_obj_list[phandle->select_id].ampm_lable_obj, divoom_menu_control_get_disp_str("PM"));
					lv_label_set_text_fmt(phandle->pmenu_obj_list[phandle->select_id].time_label_obj, "%02d:%02d", hour, phandle->palarm_info[phandle->select_id].minute);
				}
				else
				{
					lv_label_set_text(phandle->pmenu_obj_list[phandle->select_id].ampm_lable_obj, divoom_menu_control_get_disp_str("AM"));
					lv_label_set_text_fmt(phandle->pmenu_obj_list[phandle->select_id].time_label_obj, "%02d:%02d", hour, phandle->palarm_info[phandle->select_id].minute);
				}
			}
			divoom_alarm_menu_create_week_label_table(phandle, &phandle->pmenu_obj_list[phandle->select_id], phandle->palarm_info[phandle->select_id].cyc_flag);
		}
		lv_group_focus_obj(active);
	}
	lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_alarm_menu_eixt(void *handle)
{
	Pdivoom_lvgl_alarm_menu_ctrl phandle = (Pdivoom_lvgl_alarm_menu_ctrl)handle;
	
	lv_timer_del(phandle->menu_bar_timer);
	lv_timer_del(phandle->timer_handle);
	lv_obj_del(phandle->cout_obj);
	lv_obj_del(phandle->main_obj);
	lv_style_reset(&phandle->backgroud_type);
	lv_style_reset(&phandle->cont_type);
	lv_style_reset(&phandle->normal_type);
	lv_style_reset(&phandle->press_type);
	lv_style_reset(&phandle->select_press_type);
	divoom_menu_direct_ctrl_destory(phandle->pdirect_handle);
	divoom_system_free(phandle->palarm_info);
	divoom_system_free(phandle->pmenu_obj_list);
	divoom_system_free(phandle);

}


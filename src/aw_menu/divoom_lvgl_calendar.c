#include "divoom.h"
#include "divoom_menu_control.h"

#if 1

typedef struct _divoom_lvgl_calendar_content_ctrl
{
    int active_id;
    int event_num;
    lv_style_t label_normal_type;
    lv_style_t label_active_type;   
    void *active_obj;
    void *event_label_tbl_obj[50];
    void *main_obj;
}divoom_lvgl_calendar_content_ctrl, *Pdivoom_lvgl_calendar_content_ctrl;

typedef struct _divoom_lvgl_calendar_ctrl
{
    uint8 get_calendar_info;// 是否得到信息
    uint8 calendar_flag; //0:google, 1:outlook
    lv_coord_t col_dsc[8];
    lv_coord_t row_dsc[7];
    lv_style_t grid_type;
    lv_style_t btn_type;
    lv_style_t btn_press_type;   
    lv_style_t label_normal_type;
    lv_style_t label_active_type;     
    int first_pos; 
    int first_day_time;
    int month;
    int menu_id;
    char tmp_buf[1024];
    char disp_buf[2048];
    void *main_obj;
    void *date_label_obj;
    void *time_label_obj;
    void *date_btn_obj[6][7];   
    void *day_label_obj[6][7];  
    void *event_label_obj[6][7][6];
    void *month_event_queue[42];    //当月的事件
    void *time_handle;
    void *confirm_message_ctrl;
    Pdivoom_lvgl_calendar_content_ctrl pcontent_handle;
}divoom_lvgl_calendar_ctrl, *Pdivoom_lvgl_calendar_ctrl;

void divoom_lvgl_calendar_load_calenday_event(Pdivoom_lvgl_calendar_ctrl phandle);

void divoom_lvgl_calendar_destory_content_disp(Pdivoom_lvgl_calendar_ctrl phandle, int flag);

void divoom_lvgl_calendar_create_confirm_dialog_callback(void *handle, int ctrl_id)
{
    Pdivoom_lvgl_calendar_ctrl phandle = (Pdivoom_lvgl_calendar_ctrl)handle;
    
    
    divoom_lvgl_message_box_destroy(phandle->confirm_message_ctrl, 0);
    
    phandle->confirm_message_ctrl = NULL;
    lv_group_focus_obj(phandle->main_obj);
    
}


/*********************************************************************
  Function:        divoom_lvgl_photo_manage_create_confirm_dialog
  Description:     没有日历提醒
  Input:           phandle
  
  Return:          NONE
**********************************************************************/

static void divoom_lvgl_calendar_create_confirm_dialog(Pdivoom_lvgl_calendar_ctrl phandle)
{
    // 如果消息框已经存在，不重复创建，防止对象泄漏
    if(phandle->confirm_message_ctrl != NULL)
    {
        return;
    }
    
    phandle->confirm_message_ctrl = divoom_lvgl_message_box_init(phandle->menu_id, phandle->main_obj, phandle, divoom_lvgl_calendar_create_confirm_dialog_callback, 
        80, 320, 640, 408, "2/2_confirm_bg.png", lv_color_hex(0xE4E4E4), 1);
    
    
    divoom_lvgl_message_box_add_btn_item(phandle->confirm_message_ctrl, divoom_menu_control_get_disp_str("Confirm"), 32, lv_color_hex(0), lv_color_hex(0),lv_color_hex(0), 0, 
        179, 290, 282, 90);
    
    if(phandle->calendar_flag == 0)
    {
        divoom_lvgl_message_box_add_label_item(phandle->confirm_message_ctrl, divoom_menu_control_get_disp_str("GoogleCalendar"), -1, 40, lv_color_hex(0x333333), LV_TEXT_ALIGN_CENTER,
            0, 59, 640, 75);
    }
    else
    {
        divoom_lvgl_message_box_add_label_item(phandle->confirm_message_ctrl, divoom_menu_control_get_disp_str("OutLookCalendar"), -1, 40, lv_color_hex(0x333333), LV_TEXT_ALIGN_CENTER,
            0, 59, 640, 75);
    }
    
    if(phandle->calendar_flag == 0)
    {
        if(divoom_google_calendar_check_login())
        {
            divoom_lvgl_message_box_add_label_item(phandle->confirm_message_ctrl, divoom_menu_control_get_disp_str("NoEventItemContent"), -1, 26, lv_color_hex(0x333333), LV_TEXT_ALIGN_CENTER,
                20, 141, 600, 103);
        }
        else
        {
            divoom_lvgl_message_box_add_label_item(phandle->confirm_message_ctrl, divoom_menu_control_get_disp_str("NoLoginCalendar"), -1, 26, lv_color_hex(0x333333), LV_TEXT_ALIGN_CENTER,
                20, 141, 600, 103);
        }
    }
    else
    {

        if(divoom_outlook_calendar_check_login())
        {
            divoom_lvgl_message_box_add_label_item(phandle->confirm_message_ctrl, divoom_menu_control_get_disp_str("NoEventItemContent"), -1, 26, lv_color_hex(0x333333), LV_TEXT_ALIGN_CENTER,
                20, 141, 600, 103);
        }
        else
        {
            divoom_lvgl_message_box_add_label_item(phandle->confirm_message_ctrl, divoom_menu_control_get_disp_str("NoLoginCalendar"), -1, 26, lv_color_hex(0x333333), LV_TEXT_ALIGN_CENTER,
                20, 141, 600, 103);
        }
    }
    

    divoom_lvgl_message_box_set_active_item(phandle->confirm_message_ctrl, 0);
    
}


#if 1
void divoom_lvgl_calendar_content_event_cb(lv_event_t * e)
{
	lv_obj_t *cur_obj;
    Pdivoom_lvgl_calendar_ctrl phandle = (Pdivoom_lvgl_calendar_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target(e);
	
    if(code == LV_EVENT_KEY)
	{
		char *c = ((char *)lv_event_get_param(e));
        if(c[0] == DIVOOM_LVGL_KEY_ESC || c[0] == DIVOOM_LVGL_KEY_ENTER)
        {
            divoom_lvgl_calendar_destory_content_disp(phandle, 1);
        }
		else if(c[0] == DIVOOM_LVGL_KEY_PRE)
		{
            if(phandle->pcontent_handle->active_id == 0)
            {
                phandle->pcontent_handle->active_id = phandle->pcontent_handle->event_num - 1;
            }
            else
            {
                phandle->pcontent_handle->active_id --;
            }
            lv_group_focus_obj(phandle->pcontent_handle->event_label_tbl_obj[phandle->pcontent_handle->active_id]);
		}
		else if(c[0] == DIVOOM_LVGL_KEY_NEXT)
		{
            if(phandle->pcontent_handle->active_id == phandle->pcontent_handle->event_num - 1)
            {
                phandle->pcontent_handle->active_id = 0;
            }
            else
            {
                phandle->pcontent_handle->active_id ++;
            }
            lv_group_focus_obj(phandle->pcontent_handle->event_label_tbl_obj[phandle->pcontent_handle->active_id]);
		}
		else
		{
            divoom_main_keypad_send_key_info(c[0], c[1]);
			return;
		}
    }
}

void divoom_lvgl_calendar_create_content_disp(Pdivoom_lvgl_calendar_ctrl phandle, void *cur_obj, void *event_queue, int index)
{
    /*lv_obj_t * label = lv_label_create(parent);

    */
    Pdivoom_lvgl_calendar_content_ctrl pcontent_handle;
    Pdivoom_lvgl_calendar_event_info pevent_item;
	struct tm cur_tm;    
    int start_y = 0, label_high, bg_color, font_color;
    int cur_time;
    
    if(event_queue == NULL || divoom_queue_items(event_queue) == 0)
    {
        return;
    }
    
    cur_time = phandle->first_day_time + index *24*3600;
    localtime_r((time_t *)&cur_time, &cur_tm);
    
    pcontent_handle = divoom_system_malloc(sizeof(divoom_lvgl_calendar_content_ctrl));
    phandle->pcontent_handle = pcontent_handle;
    
    
    lv_style_init(&pcontent_handle->label_active_type);
    lv_style_set_border_color(&pcontent_handle->label_active_type, lv_color_hex(0xFFC31D));
    lv_style_set_outline_color(&pcontent_handle->label_active_type, lv_color_hex(0xFFC31D));
    lv_style_set_border_color(&pcontent_handle->label_active_type, lv_color_hex(0xFFC31D));
    lv_style_set_border_width(&pcontent_handle->label_active_type, 3);
    lv_style_set_radius(&pcontent_handle->label_active_type, 15);
    lv_style_set_outline_width(&pcontent_handle->label_active_type, 0);
    lv_style_set_shadow_width(&pcontent_handle->label_active_type, 0);
    lv_style_set_border_side(&pcontent_handle->label_active_type, LV_BORDER_SIDE_FULL);
    
    lv_style_init(&pcontent_handle->label_normal_type);
    lv_style_set_border_color(&pcontent_handle->label_normal_type, lv_color_hex(0xFFFFFF));
    lv_style_set_outline_color(&pcontent_handle->label_normal_type, lv_color_hex(0xFFFFFF));
    lv_style_set_border_width(&pcontent_handle->label_normal_type, 0);
    lv_style_set_outline_width(&pcontent_handle->label_normal_type, 0);
    lv_style_set_shadow_width(&pcontent_handle->label_normal_type, 0);
    lv_style_set_radius(&pcontent_handle->label_normal_type, 15);
    lv_style_set_border_side(&pcontent_handle->label_normal_type, LV_BORDER_SIDE_NONE);
    
    pcontent_handle->main_obj = lv_btn_create(phandle->main_obj);
    lv_obj_set_pos(pcontent_handle->main_obj, 0, 0);
    lv_obj_set_size(pcontent_handle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_style_bg_color(pcontent_handle->main_obj, lv_color_hex(0xEBEFF3), 0);
    
    //
    void *label_obj = lv_label_create(pcontent_handle->main_obj);
	lv_obj_set_size(label_obj, 378, 73);
	lv_obj_set_pos(label_obj, 64, 57);
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0x6371BD), 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(55, phandle->menu_id), 0);
	lv_obj_set_style_text_align(label_obj,LV_TEXT_ALIGN_LEFT, 0);    
	sprintf(phandle->disp_buf, "%s, %s %d", divoom_disp_clock_get_week2_disp_str(cur_tm.tm_wday, 1), divoom_disp_clock_get_mon_disp_str(cur_tm.tm_mon),cur_tm.tm_mday);
	lv_label_set_text(label_obj, phandle->disp_buf);
    
    void *cont = lv_obj_create(pcontent_handle->main_obj);
    lv_obj_add_style(cont, &phandle->grid_type, 0);
	lv_obj_set_size(cont, 704, 1061);
	lv_obj_set_pos(cont, 48, 159);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_radius(cont, 41, 0);
    
    pcontent_handle->event_num = divoom_queue_items(event_queue);
    start_y = 30;
    for(int i = 0; i < pcontent_handle->event_num; i++)
    {
        pevent_item = divoom_queue_get_index_item(event_queue, i);
        if(pevent_item == NULL)
        {
            break;
        }
        bg_color = (pevent_item->bg_color[0] << 16) + (pevent_item->bg_color[1] << 8) + pevent_item->bg_color[2];     
        font_color = (pevent_item->fr_color[0] << 16) + (pevent_item->fr_color[1] << 8) + pevent_item->fr_color[2]; 
        
        
        lv_obj_t * obj = lv_btn_create(cont);
        lv_obj_add_style(obj, &pcontent_handle->label_normal_type, 0);
        lv_obj_add_style(obj, &pcontent_handle->label_active_type, LV_STATE_FOCUSED);
        lv_obj_add_style(obj, &pcontent_handle->label_active_type, LV_STATE_FOCUS_KEY);
        divoom_print("disp: %d, %x, %x\n", start_y, bg_color, font_color);
        lv_obj_set_pos(obj, 50, start_y);
        lv_obj_set_size(obj, 625, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(obj, lv_color_hex(bg_color), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(obj, lv_color_hex(font_color), LV_PART_MAIN | LV_STATE_DEFAULT);
        label_high = 0;
        
        label_obj = lv_label_create(obj);        
        // 同时设置宽度和高度
        lv_obj_set_pos(label_obj, 0, 4);
        lv_obj_set_size(label_obj, 605, 53);  // 宽度100，高度自适应
        label_high = 53 + 4;

        // 设置换行模式和文本
        lv_label_set_long_mode(label_obj, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_color(label_obj, lv_color_hex(font_color), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(40, phandle->menu_id), 0);
        lv_obj_set_style_text_align(label_obj,LV_TEXT_ALIGN_LEFT, 0);
        sprintf(phandle->disp_buf, "%02d:%02d ~ %02d:%02d", pevent_item->start_time/60, pevent_item->start_time%60, pevent_item->end_time/60, pevent_item->end_time%60);
        lv_label_set_text(label_obj, phandle->disp_buf);
        label_high += 13;
        
        // 设置换行模式和文本
        label_obj = lv_label_create(obj);        
        // 同时设置宽度和高度
        lv_obj_set_pos(label_obj, 15, label_high);
        lv_obj_set_size(label_obj, 605, LV_SIZE_CONTENT);  // 宽度100，高度自适应

        
        lv_label_set_long_mode(label_obj, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_color(label_obj, lv_color_hex(font_color), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(36, phandle->menu_id), 0);
        lv_obj_set_style_text_align(label_obj,LV_TEXT_ALIGN_LEFT, 0);
        if(pevent_item->title)
        {
            lv_label_set_text(label_obj, pevent_item->title);
        }
        else
        {
            lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("NoTitle"));
        }
        lv_obj_update_layout(label_obj);
        label_high += lv_obj_get_height(label_obj);
        label_high += 13;
        
        if(pevent_item->msg_info)
        {
            label_obj = lv_label_create(obj);        
            // 同时设置宽度和高度
            lv_obj_set_pos(label_obj, 15, label_high);
            lv_obj_set_size(label_obj, 605, LV_SIZE_CONTENT);  // 宽度100，高度自适应

            
            lv_label_set_long_mode(label_obj, LV_LABEL_LONG_WRAP);
            lv_obj_set_style_text_color(label_obj, lv_color_hex(font_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
            lv_obj_set_style_text_align(label_obj,LV_TEXT_ALIGN_LEFT, 0);
            lv_label_set_text(label_obj, pevent_item->msg_info);
            lv_obj_update_layout(label_obj);
            label_high += lv_obj_get_height(label_obj);
            label_high += 13;
        }
        label_high += 15;
        
        lv_obj_set_height(obj, label_high);
        label_high += 15;
        pcontent_handle->event_label_tbl_obj[i] = obj;
        start_y +=  label_high;
        
        lv_obj_add_event_cb(obj, divoom_lvgl_calendar_content_event_cb, LV_EVENT_KEY, phandle);
    }
    lv_group_focus_obj(pcontent_handle->event_label_tbl_obj[0]);
    phandle->pcontent_handle->active_id = 0;

    pcontent_handle->active_obj = cur_obj;
}
void divoom_lvgl_calendar_destory_content_disp(Pdivoom_lvgl_calendar_ctrl phandle, int flag)
{
    Pdivoom_lvgl_calendar_content_ctrl pcontent_handle = phandle->pcontent_handle;

    if(pcontent_handle == NULL)
    {
        return;
    }
    lv_group_focus_obj(pcontent_handle->active_obj);
    if(flag)
    {
        lv_obj_del_async(pcontent_handle->main_obj);
    }
    else
    {
        lv_obj_del(pcontent_handle->main_obj);
    }
    lv_style_reset(&pcontent_handle->label_normal_type);
    lv_style_reset(&pcontent_handle->label_active_type);
    divoom_system_free(pcontent_handle);
    phandle->pcontent_handle = NULL;

}
#endif

void *divoom_lvgl_calendar_get_active_event_obj(Pdivoom_lvgl_calendar_ctrl phandle, void *cur_obj, int *id)
{
    int i, index;
    
    for(i = 0; i <42; i ++)
    {
        index = i;
        if(phandle->month_event_queue[i])
        {
            if(phandle->date_btn_obj[index/7][index%7] == cur_obj)
            {
                *id = i;
                return phandle->month_event_queue[i];
            }
        }
    }
    return NULL;
}

void *divoom_lvgl_calendar_find_next_obj(Pdivoom_lvgl_calendar_ctrl phandle, int dir, void *cur_obj)
{
    void *first_obj = NULL, *pre_obj = NULL;
    int i, find_pos = -1, cur_id = 0, index, first_pos;
    
    for(i = 0; i <42; i ++)
    {
        index = i;
        if(phandle->month_event_queue[i])
        {
            if(first_obj == NULL)
            {
                first_obj = phandle->date_btn_obj[index/7][index%7];
                first_pos = index;
            }
            if(phandle->date_btn_obj[index/7][index%7] == cur_obj)
            {
                break;
            }
            pre_obj = phandle->date_btn_obj[index/7][index%7];
            find_pos = index;
        }
    }
    if(i < 0)
    {
        return phandle->main_obj;
    }
    
    if(dir == -1)
    {
        if(pre_obj == NULL)
        {
            for(int i = 41; i >= 0; i --)
            {
                if(phandle->month_event_queue[i])
                {
                    index = i;
                    return phandle->date_btn_obj[index/7][index%7];
                }
                
            }
        }
        return pre_obj;
    }
    else 
    {
        for(i++; i <42; i ++)
        {
            index = i + phandle->first_pos;
            if(phandle->month_event_queue[i])
            {
                    index = i;
                    return phandle->date_btn_obj[index/7][index%7];
            }
        }
        return first_obj;        
    }    
}

void divoom_lvgl_calendar_event_cb(lv_event_t * e)
{
	int tip_index, active_id;
	lv_obj_t *cur_obj;
    Pdivoom_lvgl_calendar_ctrl phandle = (Pdivoom_lvgl_calendar_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target(e);
	
    if(code == LV_EVENT_KEY)
	{
		char *c = ((char *)lv_event_get_param(e));
        if(c[0] == DIVOOM_LVGL_KEY_ESC)
        {
            if(divoom_menu_control_menu_items() > 1)
            {
                divoom_menu_control_return_pre();
            }
            else
            {
                divoom_main_keypad_send_key_info(c[0], c[1]);
            }
        }
        else
        {
            
            if(c[0] == DIVOOM_LVGL_KEY_PRE || c[0] == DIVOOM_LVGL_KEY_NEXT || c[0] == DIVOOM_LVGL_KEY_ENTER)                
            {
                
                if(cur_obj == phandle->main_obj)
                {
                    //提醒用户，没有当月无日历事件
                    
                    divoom_lvgl_calendar_create_confirm_dialog(phandle);
                    return;
                }
                
        		if(c[0] == DIVOOM_LVGL_KEY_PRE)
        		{
                    cur_obj = divoom_lvgl_calendar_find_next_obj(phandle, -1, cur_obj);
                    lv_group_focus_obj(cur_obj);
        		}
        		else if(c[0] == DIVOOM_LVGL_KEY_NEXT)
        		{
                    cur_obj = divoom_lvgl_calendar_find_next_obj(phandle, 1, cur_obj);
                    lv_group_focus_obj(cur_obj);
        		}
        		else if(c[0] == DIVOOM_LVGL_KEY_ENTER)
        		{
                    //这个将进入下一级菜单查看事件
                    int index = 0;
                    void *new_obj = divoom_lvgl_calendar_get_active_event_obj(phandle, cur_obj, &index);
                    divoom_lvgl_calendar_create_content_disp(phandle, cur_obj, new_obj, index);
        		}
            }
    		else
    		{
                divoom_main_keypad_send_key_info(c[0], c[1]);
    			return;
    		}
        }
	}

}

//创建日历UI
void divoom_lvgl_calendar_draw_ui(Pdivoom_lvgl_calendar_ctrl phandle)
{
    int i, j;
    char disp_info[32];
    void *label_obj;
	struct tm cur_tm = {0}, first_day_tm = {0};    
    int cur_time = time(0), first_day_time, one_day_time, today_time, end_time;

    today_time = cur_time;
    lv_style_init(&phandle->grid_type);
    lv_style_set_pad_all(&phandle->grid_type, 0);
    lv_style_set_pad_gap(&phandle->grid_type, 0);
    lv_style_set_border_width(&phandle->grid_type, 0);
    lv_style_set_border_side(&phandle->grid_type, LV_BORDER_SIDE_NONE);
    lv_style_set_bg_color(&phandle->grid_type, lv_color_hex(0xFFFFFF));
    
    
    lv_obj_t * cont = lv_obj_create(phandle->main_obj);
    lv_obj_add_style(cont, &phandle->grid_type, 0);
    lv_obj_set_size(cont, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH - 130);
    lv_obj_set_pos(cont, 0, 130);
	lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_opa(cont, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cont, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
    
    localtime_r((time_t *)&cur_time, &cur_tm);
    phandle->month = cur_tm.tm_mon;

    
    //
    phandle->date_label_obj = lv_label_create(phandle->main_obj);
	lv_obj_set_size(phandle->date_label_obj, 200, 66);
	lv_obj_set_pos(phandle->date_label_obj, 36, 22);
	lv_obj_set_style_text_color(phandle->date_label_obj, lv_color_hex(0x9E9E9E), 0);
	lv_obj_set_style_text_font(phandle->date_label_obj, divoom_menu_control_get_font_info(50, phandle->menu_id), 0);
	lv_obj_set_style_text_align(phandle->date_label_obj,LV_TEXT_ALIGN_LEFT, 0);
    
	sprintf(disp_info, "%s %d", divoom_disp_clock_get_week2_disp_str(cur_tm.tm_wday, 1), cur_tm.tm_mday);
	lv_label_set_text(phandle->date_label_obj, disp_info);
    
    
    
    phandle->time_label_obj = lv_label_create(phandle->main_obj);
	lv_obj_set_size(phandle->time_label_obj, 194, 66);
	lv_obj_set_pos(phandle->time_label_obj, 574, 22);
	lv_obj_set_style_text_color(phandle->time_label_obj, lv_color_hex(0x9E9E9E), 0);
	lv_obj_set_style_text_font(phandle->time_label_obj, divoom_menu_control_get_font_info(50, phandle->menu_id), 0);
	lv_obj_set_style_text_align(phandle->time_label_obj,LV_TEXT_ALIGN_RIGHT, 0);
	sprintf(disp_info, "%02d:%02d", cur_tm.tm_hour, cur_tm.tm_min);
	lv_label_set_text(phandle->time_label_obj, disp_info);
    
       
    memcpy(&first_day_tm, &cur_tm, sizeof(cur_tm));
    //先得到这个月一号是星期几
    first_day_tm.tm_mday = 1;
    first_day_tm.tm_hour = 0;
    first_day_tm.tm_min = 0;
    first_day_tm.tm_sec = 0;
    one_day_time = mktime(&first_day_tm);
    
    
    //先得到这个月一号是星期几
    first_day_tm.tm_mon ++;
    if(first_day_tm.tm_mon > 11)
    {
        first_day_tm.tm_mon = 0;
        first_day_tm.tm_year ++;
    }
    first_day_tm.tm_mday = 1;
    first_day_tm.tm_hour = 0;
    first_day_tm.tm_min = 0;
    first_day_tm.tm_sec = 0;
    end_time = mktime(&first_day_tm);
    
    localtime_r((time_t *)&one_day_time, &first_day_tm);
    phandle->first_pos = first_day_tm.tm_wday;
    //得到这个日历的第一天
    first_day_time = one_day_time - first_day_tm.tm_wday *(24*3600);
    phandle->first_day_time = first_day_time;
    divoom_menu_control_init_btn_stype(&phandle->btn_press_type, NULL, &phandle->btn_type, NULL, NULL, NULL, 0);
    
    lv_style_set_bg_color(&phandle->btn_press_type, lv_color_hex(0xFFFFFF));
    lv_style_set_border_color(&phandle->btn_press_type, lv_color_hex(0xFFC31D));
    lv_style_set_border_width(&phandle->btn_press_type, 2);
    lv_style_set_border_side(&phandle->btn_press_type, LV_BORDER_SIDE_FULL);
    lv_style_set_pad_all(&phandle->btn_press_type, 0);
    lv_style_set_pad_gap(&phandle->btn_press_type, 0);
    lv_style_set_bg_color(&phandle->btn_type, lv_color_hex(0xFFFFFF));
    lv_style_set_border_color(&phandle->btn_type, lv_color_hex(0xE8EAED));
    lv_style_set_border_width(&phandle->btn_type, 2);
    lv_style_set_border_side(&phandle->btn_type, LV_BORDER_SIDE_TOP|LV_BORDER_SIDE_RIGHT);
    lv_style_set_pad_all(&phandle->btn_type, 0);
    lv_style_set_pad_gap(&phandle->btn_type, 0);
    
    for(i = 0; i < 7; i ++)
    {
        label_obj = lv_label_create(phandle->main_obj);
        lv_obj_set_size(label_obj, 112, 21);
        lv_obj_set_pos(label_obj, 2+114*i, 108);
        lv_obj_set_style_text_color(label_obj, lv_color_hex(0xBBBBBB), 0);
        lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(20, phandle->menu_id), 0);
        lv_obj_set_style_text_align(label_obj,LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_text(label_obj, divoom_disp_clock_get_week2_disp_str(i, 0));
    }
    
    divoom_menu_control_init_btn_stype(&phandle->label_active_type, NULL, &phandle->label_normal_type, NULL, NULL, NULL, 0);
    lv_style_set_border_color(&phandle->label_active_type, lv_color_hex(0xFFC31D));
    lv_style_set_border_color(&phandle->label_normal_type, lv_color_hex(0xFFFFFF));
    

    lv_obj_set_style_base_dir(cont, LV_BASE_DIR_AUTO, 0);
    for(i = 0; i < 7; i ++)
    {
        phandle->col_dsc[i] = 114;
    }
    phandle->col_dsc[7] = LV_GRID_TEMPLATE_LAST;
    for(i = 0; i < 6; i ++)
    {
        phandle->row_dsc[i] = 192;
    }
    phandle->row_dsc[6] = LV_GRID_TEMPLATE_LAST;
    lv_obj_set_grid_dsc_array(cont, phandle->col_dsc, phandle->row_dsc);
    
    
    for(i = 0; i < 6; i++)
    {
        for(j = 0; j < 7; j ++)
        {
            localtime_r((time_t *)&first_day_time, &first_day_tm);
            
            
            lv_obj_t * obj = lv_btn_create(cont);
            lv_obj_add_style(obj, &phandle->btn_type, 0);
            lv_obj_add_style(obj, &phandle->btn_press_type, LV_STATE_FOCUSED);
            lv_obj_add_style(obj, &phandle->btn_press_type, LV_STATE_FOCUS_KEY);
            lv_obj_set_size(obj, 114, 192);
            lv_obj_center(obj);
            lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_STRETCH, j, 1,
                                 LV_GRID_ALIGN_STRETCH, i, 1);
            phandle->date_btn_obj[i][j] = obj;
            
            
            label_obj = lv_label_create(obj);
            lv_obj_set_size(label_obj, 34, 31);
            lv_obj_set_pos(label_obj, 40, 9);
            if(first_day_time < one_day_time || first_day_time >= end_time)
            {
                lv_obj_set_style_text_color(label_obj, lv_color_hex(0xBBBBBB), 0);
            }
            else
            {
                lv_obj_set_style_text_color(label_obj, lv_color_hex(0x2B2B2B), 0);
            }
            lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(25, phandle->menu_id), 0);
            lv_obj_set_style_text_align(label_obj,LV_TEXT_ALIGN_CENTER, 0);
            sprintf(disp_info, "%d", first_day_tm.tm_mday);
            lv_label_set_text(label_obj, disp_info);
            phandle->day_label_obj[i][j] = label_obj;
            

            for(int k = 0; k < 5; k++)
            {
                label_obj = lv_label_create(obj);
                lv_obj_add_style(label_obj, &phandle->label_normal_type, 0);
                if(k == 4)
                {
                    lv_obj_set_pos(label_obj, 5, 36 + 33 *k);
                }
                else
                {
                    lv_obj_set_pos(label_obj, 5, 46 + 33 *k);
                }
                lv_obj_set_size(label_obj, 202, 21);
                lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);
                lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(16, phandle->menu_id), 0);
                lv_obj_set_style_text_align(label_obj,LV_TEXT_ALIGN_LEFT, 0);            
                lv_label_set_long_mode(label_obj, LV_LABEL_LONG_CLIP); //自动换行
                lv_obj_add_flag(label_obj, LV_OBJ_FLAG_HIDDEN);                
                lv_obj_set_style_bg_opa(label_obj, LV_OPA_TRANSP, 0);
                phandle->event_label_obj[i][j][k] = label_obj;
            }
            if(phandle->month_event_queue[i*7+j])
            {
                int rgb_value, font_color;
                Pdivoom_lvgl_calendar_event_info pfirst_item, psecond_item;

                
                for(int k = 0; k < 4 && divoom_queue_items(phandle->month_event_queue[i*7+j]); k++)
                {
                    pfirst_item = divoom_queue_get_index_item(phandle->month_event_queue[i*7+j], k);
                    if(pfirst_item == NULL)
                    {
                        break;
                    }
                    label_obj = phandle->event_label_obj[i][j][k];
                    lv_obj_clear_flag(label_obj, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_set_style_bg_opa(label_obj, LV_OPA_COVER, 0);
                    rgb_value = (pfirst_item->bg_color[0] << 16) + (pfirst_item->bg_color[1] << 8) + pfirst_item->bg_color[2]; 
                    lv_obj_set_style_bg_color(label_obj, lv_color_hex(rgb_value), 0);
                    font_color = (pfirst_item->fr_color[0] << 16) + (pfirst_item->fr_color[1] << 8) + pfirst_item->fr_color[2]; 
                    lv_obj_set_style_text_color(label_obj, lv_color_hex(font_color), 0);
                    //sprintf(phandle->tmp_buf, "%02d:%02d %s",pfirst_item->start_time/60, pfirst_item->start_time%60, pfirst_item->title);
                    if(pfirst_item->title && strlen(pfirst_item->title))
                    {
                        lv_label_set_text(label_obj, pfirst_item->title);
                    }
                    else
                    {
                        lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("NoTitle"));
                    }
                }
                label_obj = phandle->event_label_obj[i][j][4];
                lv_obj_set_style_bg_color(label_obj, lv_color_hex(0xFFFFFF), 0);
                lv_obj_set_style_text_color(label_obj, lv_color_hex(0), 0);
                lv_obj_clear_flag(label_obj, LV_OBJ_FLAG_HIDDEN);                        
                lv_label_set_text(label_obj, "...");
                if(divoom_queue_items(phandle->month_event_queue[i*7+j]) <= 4)
                {
                    lv_obj_add_flag(label_obj, LV_OBJ_FLAG_HIDDEN);                        
                }
                else
                {
                    lv_obj_clear_flag(label_obj, LV_OBJ_FLAG_HIDDEN);                        
                }
            }
            else
            {
            }
            
            
            lv_obj_add_event_cb(obj, divoom_lvgl_calendar_event_cb, LV_EVENT_KEY, phandle);

            first_day_time += 24*3600;
        }
    }
    
}

int divoom_lvgl_calendar_get_update_flag(Pdivoom_lvgl_calendar_ctrl phandle)
{
    if(phandle->calendar_flag == 0)
    {
        return divoom_google_calendar_get_update_event_flag();
    }
    else
    {
        return divoom_outlook_calendar_get_update_flag();
    }
}

//刷新UI
void divoom_lvgl_calendar_update_ui(struct _lv_timer_t *timer_data)
{
    int have_items = 0;
    void *first_obj = NULL;
    char disp_info[32];
    Pdivoom_lvgl_calendar_ctrl phandle = (Pdivoom_lvgl_calendar_ctrl)lv_timer_get_user_data(timer_data);
	struct tm cur_tm, first_day_tm;    
    int cur_time = time(0), first_day_time, one_day_time, today_time, end_time;
    
    localtime_r((time_t *)&cur_time, &cur_tm);

    //先更新时间信息
	sprintf(disp_info, "%s %d", divoom_disp_clock_get_week2_disp_str(cur_tm.tm_wday, 1), cur_tm.tm_mday);
	lv_label_set_text(phandle->date_label_obj, disp_info);
	sprintf(disp_info, "%02d:%02d", cur_tm.tm_hour, cur_tm.tm_min);
	lv_label_set_text(phandle->time_label_obj, disp_info);

    if(phandle->get_calendar_info == 0 || phandle->month != cur_tm.tm_mon || divoom_lvgl_calendar_get_update_flag(phandle))
    {        
        divoom_print("aj test!\n");
        divoom_lvgl_calendar_load_calenday_event(phandle);
        memcpy(&first_day_tm, &cur_tm, sizeof(cur_tm));
        //先得到这个月一号是星期几
        first_day_tm.tm_mday = 1;
        first_day_tm.tm_hour = 0;
        first_day_tm.tm_min = 0;
        first_day_tm.tm_sec = 0;
        one_day_time = mktime(&first_day_tm);
        //先得到这个月一号是星期几
        first_day_tm.tm_mon ++;
        if(first_day_tm.tm_mon > 11)
        {
            first_day_tm.tm_mon = 0;
            first_day_tm.tm_year ++;
        }
        first_day_tm.tm_mday = 1;
        first_day_tm.tm_hour = 0;
        first_day_tm.tm_min = 0;
        first_day_tm.tm_sec = 0;
        end_time = mktime(&first_day_tm);
        
        localtime_r((time_t *)&one_day_time, &first_day_tm);
        phandle->first_pos = first_day_tm.tm_wday;
        //得到这个日历的第一天
        first_day_time = one_day_time - first_day_tm.tm_wday *(24*3600);
        phandle->first_day_time = first_day_time;
        for(int i = 0; i < 6; i ++)
        {
            for(int j = 0; j < 7; j++)
            {
                localtime_r((time_t *)&first_day_time, &first_day_tm);

                //显示日期
                if(first_day_time < one_day_time || first_day_time >= end_time)
                {
                    lv_obj_set_style_text_color(phandle->day_label_obj[i][j], lv_color_hex(0xBBBBBB), 0);
                }
                else
                {
                    lv_obj_set_style_text_color(phandle->day_label_obj[i][j], lv_color_hex(0x2B2B2B), 0);
                }
                lv_label_set_text_fmt(phandle->day_label_obj[i][j], "%d", first_day_tm.tm_mday);

                //j显示事件
                for(int k = 0; k < 5; k++)
                {
                    lv_obj_add_style(phandle->event_label_obj[i][j][k], &phandle->label_normal_type, 0);
                    lv_obj_set_style_bg_opa(phandle->event_label_obj[i][j][k], LV_OPA_TRANSP, 0);
                    lv_obj_add_flag(phandle->event_label_obj[i][j][k], LV_OBJ_FLAG_HIDDEN);
                }
                if(phandle->month_event_queue[i*7+j])
                {
                    int rgb_value, k;
                    Pdivoom_lvgl_calendar_event_info pfirst_item;

                    have_items ++;
                    for(k= 0; k < divoom_queue_items(phandle->month_event_queue[i*7+j]) && k < 4; k++)
                    {
                        lv_obj_clear_flag(phandle->event_label_obj[i][j][k], LV_OBJ_FLAG_HIDDEN);                        
                        lv_obj_set_style_bg_opa(phandle->event_label_obj[i][j][k], LV_OPA_COVER, 0);
                        pfirst_item = divoom_queue_get_index_item(phandle->month_event_queue[i*7+j], k);
                        rgb_value = (pfirst_item->bg_color[0] << 16) + (pfirst_item->bg_color[1] << 8) + pfirst_item->bg_color[2]; 
                        lv_obj_set_style_bg_color(phandle->event_label_obj[i][j][k], lv_color_hex(rgb_value), 0);
                        rgb_value = (pfirst_item->fr_color[0] << 16) + (pfirst_item->fr_color[1] << 8) + pfirst_item->fr_color[2]; 
                        lv_obj_set_style_text_color(phandle->event_label_obj[i][j][k], lv_color_hex(rgb_value), 0);
                        //sprintf(phandle->tmp_buf, "%02d:%02d %s",pfirst_item->start_time/60, pfirst_item->start_time%60, pfirst_item->title);
                        if(pfirst_item->title && strlen(pfirst_item->title))
                        {
                            lv_label_set_text(phandle->event_label_obj[i][j][k], pfirst_item->title);
                        }
                        else
                        {
                            lv_label_set_text(phandle->event_label_obj[i][j][k], divoom_menu_control_get_disp_str("NoTitle"));
                        }
                        
                    }
                    if(divoom_queue_items(phandle->month_event_queue[i*7+j]) > 4)
                    {
                        lv_obj_clear_flag(phandle->event_label_obj[i][j][4], LV_OBJ_FLAG_HIDDEN);
                    }
                    else
                    {
                        lv_obj_add_flag(phandle->event_label_obj[i][j][4], LV_OBJ_FLAG_HIDDEN);
                    }
                }
                else
                {
                }
                
                first_day_time += 24*3600;
            }
        }
        
        int index = -1, start_index;
        
        cur_time = cur_time -(cur_tm.tm_mday - 1) * 24*3600;
        localtime_r((time_t *)&cur_time, &first_day_tm);
        start_index = cur_tm.tm_mday + first_day_tm.tm_wday -1;
        for(int i = start_index; i < 42; i++)
        {
            if(phandle->month_event_queue[i])
            {
                index = i;
                break;
            }
        }
        if(index == -1)
        {
            for(int i = 0; i < start_index; i++)
            {
                if(phandle->month_event_queue[i])
                {
                    index = i;
                    break;
                }
            }
        }
        if(index == -1)
        {
            lv_group_focus_obj(phandle->main_obj);
        }
        else
        {
            lv_group_focus_obj(phandle->date_btn_obj[index/7][index%7]);    
        }
        if(phandle->confirm_message_ctrl && have_items)
        {
            divoom_print("close message box!\n");
            // 先将焦点移回日历主界面，防止后续按键事件发送到已销毁的消息框
            if(index == -1)
            {
                lv_group_focus_obj(phandle->main_obj);
            }
            else
            {
                lv_group_focus_obj(phandle->date_btn_obj[index/7][index%7]);
            }
            // 使用同步删除(sync_flag=1)，立即删除消息框对象
            divoom_lvgl_message_box_destroy(phandle->confirm_message_ctrl, 1);                
            phandle->confirm_message_ctrl = NULL;    
        }
        
        phandle->month = cur_tm.tm_mon;
    }
}

void divoom_lvgl_calendar_load_calenday_event_free_item(void *item)
{
    Pdivoom_lvgl_calendar_event_info pcur_item = (Pdivoom_lvgl_calendar_event_info)item;

    if(pcur_item->title)
    {
        divoom_system_free(pcur_item->title);
    }
    
    if(pcur_item->msg_info)
    {
        divoom_system_free(pcur_item->msg_info);
    }
    divoom_system_free(pcur_item);
}


Pdivoom_lvgl_calendar_event_info divoom_lvgl_calendar_create_event(int start_time, int end_time, int r, int g, int b, char *title, char *msg)
{
    Pdivoom_lvgl_calendar_event_info pinfo = divoom_system_malloc(sizeof(divoom_lvgl_calendar_event_info));

    pinfo->start_time = start_time;
    pinfo->end_time = end_time;
    pinfo->bg_color[0] = r;
    pinfo->bg_color[1] = g;
    pinfo->bg_color[2] = b;
    if(msg)
    {
        pinfo->msg_info = divoom_app_com_copy_string(msg, -1);
    }
    if(title)
    {
        pinfo->title = divoom_app_com_copy_string(title, -1);
    }
    return pinfo;
}
/**
 * NAME              divoom_lvgl_calendar_load_calenday_one_month
 * *  @brief        加载某一个月份的所有事件
 * *  @para         phandle          
                    year        
                    month
 * *  @return       
 * */

void divoom_lvgl_calendar_load_calenday_one_month(Pdivoom_lvgl_calendar_ctrl phandle, int year, int month)
{
    void *queue_item;
    Pdivoom_lvgl_calendar_event_info pevent_item;

    if(phandle->calendar_flag == 0)
    {
        phandle->get_calendar_info = divoom_google_calendar_load_calenday_current_month(phandle->month_event_queue);
    }
    else
    {
        phandle->get_calendar_info = divoom_outlook_calendar_load_calenday_current_month(phandle->month_event_queue);
    }
    return;
    if(month == 6)
    {
        //6月默认创建4个事件，1号3个，5号1个，15号2个，27号4个
        queue_item= divoom_queue_init(100, __func__, __LINE__);
        pevent_item = divoom_lvgl_calendar_create_event(7*60 + 0, 7*60 + 30, 0xFF, 0x00, 0x00, "this is 1start exec", "go test");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(8*60 +30, 9*60 + 30, 0xFF, 0xFF, 0x00, "go school", "go test 123312321");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(10*60 + 0, 11*60 + 30, 0xFF, 0x00, 0xFF, "今天天气真好", "go test顶戴");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(11*60 + 0, 11*60 + 30, 0xFF, 0x00, 0xFF, "今天天气真好", "go test顶戴");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(12*60 + 0, 12*60 + 30, 0xFF, 0x00, 0xFF, "今天天气真好", "go test顶戴");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(13*60 + 0, 13*60 + 30, 0xFF, 0x00, 0xFF, "今天天气真好", "go test顶戴");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(14*60 + 0, 14*60 + 30, 0xFF, 0x00, 0xFF, "今天天气真好", "go test顶戴");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(15*60 + 0, 15*60 + 30, 0xFF, 0x00, 0xFF, "今天天气真好", "go test顶戴");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(16*60 + 0, 16*60 + 30, 0xFF, 0x00, 0xFF, "今天天气真好", "go test顶戴");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(17*60 + 0, 17*60 + 30, 0xFF, 0x00, 0xFF, "今天天气真好", "go test顶戴");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(18*60 + 0, 18*60 + 30, 0xFF, 0x00, 0xFF, "今天天气真好", "go test顶戴");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(19*60 + 0, 19*60 + 30, 0xFF, 0x00, 0xFF, "今天天气真好", "go test顶戴");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(20*60 + 0, 20*60 + 30, 0xFF, 0x00, 0xFF, "今天天气真好", "go test顶戴");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(21*60 + 0, 21*60 + 30, 0xFF, 0x00, 0xFF, "今天天气真好", "go test顶戴");
        divoom_queue_push(queue_item, pevent_item);
        
        phandle->month_event_queue[0] = queue_item;
        
        queue_item = divoom_queue_init(100, __func__, __LINE__);
        pevent_item = divoom_lvgl_calendar_create_event(11*60 + 30, 12*60 + 30, 0xFF, 0xFF, 0x00, "thi is 5 test,it is ok?", "go test");
        divoom_queue_push(queue_item, pevent_item);
        
        phandle->month_event_queue[4] = queue_item;
        
        queue_item = divoom_queue_init(100, __func__, __LINE__);
        pevent_item = divoom_lvgl_calendar_create_event(10*60 + 30, 12*60 + 30, 0xFF, 0x00, 0xFF, "thi is 14 test,it is ok?", "go test");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(15*60 + 30, 12*60 + 30, 0xFF, 0xFF, 0x00, "thi is 14 test,it is ok?", "go test");
        divoom_queue_push(queue_item, pevent_item);
        phandle->month_event_queue[14] = queue_item;
        
        queue_item = divoom_queue_init(100, __func__, __LINE__);
        pevent_item = divoom_lvgl_calendar_create_event(14*60 + 30, 12*60 + 30, 0x00, 0x00, 0xFF, "thi is 25 test,it is ok?", "go test");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(16*60 + 30, 12*60 + 30, 0xFF, 0xFF, 0x00, "thi is 25 test,it is ok?", "go test");
        divoom_queue_push(queue_item, pevent_item);
        phandle->month_event_queue[26] = queue_item;
    }
    else if(month == 7)
    {
    }
    else
    {
        //其它默认创建6个事件，3号1个，6号1个，10号2个，15号4个，20号3个，28号5个
        queue_item= divoom_queue_init(100, __func__, __LINE__);
        pevent_item = divoom_lvgl_calendar_create_event(14*60 + 30, 12*60 + 30, 0x00, 0x00, 0xFF, "thi is 3 test,it is ok?", "go test");
        divoom_queue_push(queue_item, pevent_item);
        phandle->month_event_queue[2] = queue_item;
        
        queue_item = divoom_queue_init(100, __func__, __LINE__);
        pevent_item = divoom_lvgl_calendar_create_event(10*60 + 30, 12*60 + 30, 0x00, 0xFF, 0x00, "thi is 6 test,it is ok?", "go test");
        divoom_queue_push(queue_item, pevent_item);
        phandle->month_event_queue[5] = queue_item;
        
        queue_item = divoom_queue_init(100, __func__, __LINE__);
        pevent_item = divoom_lvgl_calendar_create_event(16*60 + 30, 12*60 + 30, 0xFF, 0xFF, 0x00, "thi is 10 test,it is ok?", "go test");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(17*60 + 30, 12*60 + 30, 0x00, 0xFF, 0x00, "thi is 10 test,it is ok?", "go test");
        divoom_queue_push(queue_item, pevent_item);
        phandle->month_event_queue[9] = queue_item;
        
        queue_item = divoom_queue_init(100, __func__, __LINE__);
        pevent_item = divoom_lvgl_calendar_create_event(12*60 + 30, 12*60 + 30, 0xFF, 0x00, 0xFF, "thi is 15 test,it is ok?", "go test");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(13*60 + 30, 12*60 + 30, 0x00, 0xFF, 0x00, "thi is 15 test,it is ok?", "go test");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(14*60 + 30, 12*60 + 30, 0xFF, 0x00, 0xFF, "thi is 15 test,it is ok?", "go test");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(15*60 + 30, 12*60 + 30, 0x00, 0xFF, 0x00, "thi is 15 test,it is ok?", "go test");
        divoom_queue_push(queue_item, pevent_item);
        phandle->month_event_queue[14] = queue_item;
        
        queue_item = divoom_queue_init(100, __func__, __LINE__);
        pevent_item = divoom_lvgl_calendar_create_event(20*60 + 30, 12*60 + 30, 0xFF, 0x00, 0xFF, "thi is 20 test,it is ok?", "go test");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(13*60 + 30, 12*60 + 30, 0x00, 0xFF, 0x00, "thi is 20 test,it is ok?", "go test");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(14*60 + 30, 12*60 + 30, 0xFF, 0x00, 0xFF, "thi is 20 test,it is ok?", "go test");
        divoom_queue_push(queue_item, pevent_item);
        phandle->month_event_queue[19] = queue_item;
        
        queue_item = divoom_queue_init(100, __func__, __LINE__);
        pevent_item = divoom_lvgl_calendar_create_event(18*60 + 30, 12*60 + 30, 0xFF, 0x00, 0xFF, "thi is 28 test,it is ok?", "go test");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(19*60 + 30, 12*60 + 30, 0x00, 0xFF, 0x00, "thi is 28 test,it is ok?", "go test");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(14*60 + 30, 12*60 + 30, 0xFF, 0x00, 0xFF, "thi is 28 test,it is ok?", "go test");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(13*60 + 30, 12*60 + 30, 0x00, 0xFF, 0x00, "thi is 28 test,it is ok?", "go test");
        divoom_queue_push(queue_item, pevent_item);
        pevent_item = divoom_lvgl_calendar_create_event(14*60 + 30, 12*60 + 30, 0xFF, 0x00, 0xFF, "thi is 28 test,it is ok?", "go test");
        divoom_queue_push(queue_item, pevent_item);
        phandle->month_event_queue[27] = queue_item;
    }
}

/**
 * NAME              divoom_lvgl_calendar_load_calenday_event
 * *  @brief        月份发生变化，将重新加载所有事件
 * *  @para         phandle 
 * *  @return       
 * */

void divoom_lvgl_calendar_load_calenday_event(Pdivoom_lvgl_calendar_ctrl phandle)
{
	struct tm cur_tm;    
    int cur_time = time(0);
    
    localtime_r((time_t *)&cur_time, &cur_tm);
    

    divoom_lvgl_calendar_load_calenday_one_month(phandle, cur_tm.tm_year + 1900, cur_tm.tm_mon + 1);
}

/**
 * NAME              divoom_lvgl_calendar_init
 * *  @brief        初始化日历模块
 * *  @para         phandle 
 * *  @return       
 * */

void *divoom_lvgl_calendar_init(void* user_data, int menu_id)
{
    Pdivoom_lvgl_calendar_ctrl phandle;

    phandle = divoom_system_malloc(sizeof(divoom_lvgl_calendar_ctrl));
    if(user_data)
    {
        phandle->calendar_flag = 0;
    }
    else
    {
        phandle->calendar_flag = 1;
    }
    phandle->main_obj = lv_btn_create(NULL);
    divoom_lvgl_calendar_load_calenday_event(phandle);
    divoom_lvgl_calendar_draw_ui(phandle);
    
    phandle->time_handle = lv_timer_create(divoom_lvgl_calendar_update_ui, 1000, phandle);
    
    
    lv_obj_add_event_cb(phandle->main_obj, divoom_lvgl_calendar_event_cb, LV_EVENT_KEY, phandle);
    
    
    return phandle;
}

/**
 * NAME              divoom_lvgl_calendar_disp
 * *  @brief        显示日历模块
 * *  @para         phandle 
 * *  @return       
 * */

void divoom_lvgl_calendar_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_calendar_ctrl phandle = (Pdivoom_lvgl_calendar_ctrl)handle;    
    struct tm cur_tm, first_tm;    
    time_t cur_time = time(0), first_day_time;
    int index = -1, start_index;
    
    localtime_r((time_t *)&cur_time, &cur_tm);
    first_day_time = cur_time -(cur_tm.tm_mday - 1) * 24*3600;
    localtime_r((time_t *)&first_day_time, &first_tm);
    start_index = cur_tm.tm_mday + first_tm.tm_wday -1;
    for(int i = start_index; i < 42; i++)
    {
        if(phandle->month_event_queue[i])
        {
            index = i;
            break;
        }
    }
    if(index == -1)
    {
        for(int i = 0; i < start_index; i++)
        {
            if(phandle->month_event_queue[i])
            {
                index = i;
                break;
            }
        }
    }
    if(index == -1)
    {
        lv_group_focus_obj(phandle->main_obj);
    }
    else
    {
        lv_group_focus_obj(phandle->date_btn_obj[index/7][index%7]);        
    }
    
    lv_scr_load(phandle->main_obj);
    
}

/**
 * NAME              divoom_lvgl_calendar_exit
 * *  @brief        退出日历模块
 * *  @para         phandle 
 * *  @return       
 * */

void divoom_lvgl_calendar_exit(void *handle)
{    
    Pdivoom_lvgl_calendar_ctrl phandle = (Pdivoom_lvgl_calendar_ctrl)handle;    
    
    if(phandle->calendar_flag == 0)
    {
        divoom_outlook_calendar_update_menu_flag(0);
    }
    else
    {
        divoom_outlook_calendar_update_menu_flag(0);
    }

    lv_timer_del(phandle->time_handle);
    divoom_lvgl_calendar_destory_content_disp(phandle, 0);
    if(phandle->confirm_message_ctrl)
    {
        divoom_lvgl_message_box_destroy(phandle->confirm_message_ctrl, 1);
        phandle->confirm_message_ctrl = NULL;
    }
    lv_obj_del(phandle->main_obj);
    lv_style_reset(&phandle->grid_type);
    lv_style_reset(&phandle->btn_type);
    lv_style_reset(&phandle->btn_press_type);
    lv_style_reset(&phandle->label_normal_type);
    lv_style_reset(&phandle->label_active_type);
    for(int i = 0; i < 42; i++)
    {
        if(phandle->month_event_queue[i])
        {
            divoom_queue_destroy(phandle->month_event_queue[i], divoom_lvgl_calendar_load_calenday_event_free_item);
            phandle->month_event_queue[i] = NULL;
        }
    }
    divoom_system_free(phandle);
}
#endif




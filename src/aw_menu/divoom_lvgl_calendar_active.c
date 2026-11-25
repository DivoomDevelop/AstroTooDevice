#include "divoom.h"
#include "divoom_menu_control.h"
#include <stdio.h>
#include <string.h>

#define CAL_ACTIVE_TITLE_START_Y    128
#define CAL_ACTIVE_TITLE_PAD_X      16
#define CAL_ACTIVE_TITLE_WIDTH      (DIVOOM_SCREEN_WIDTH - 48)

typedef struct _divoom_lvgl_calendar_active_ctrl
{
    uint8 get_data_flag;
    uint8 calendar_flag;
    lv_style_t grid_type;
    int menu_id;
    void *main_obj;
    void *date_label_obj;
    void *time_label_obj;
    void *title_label_obj;
    void *desc_label_obj;
    void *btn_cout_obj;
    void *time_handle;
    Pdivoom_lvgl_calendar_event_info pevent_info;
}divoom_lvgl_calendar_active_ctrl, *Pdivoom_lvgl_calendar_active_ctrl;

static void divoom_lvgl_calendar_active_set_date_label(void *label, const struct tm *tm)
{
    char buf[32];
    int wday = tm->tm_wday;

    if (wday < 0 || wday > 6)
    {
        wday = 0;
    }
    snprintf(buf, sizeof(buf), "%s %02d",
        divoom_disp_clock_get_week2_disp_str(wday, 1), tm->tm_mday);
    lv_label_set_text((lv_obj_t *)label, buf);
}

static void divoom_lvgl_calendar_active_set_time_label(void *label, int start_min, int end_min)
{
    char buf[32];

    snprintf(buf, sizeof(buf), "%02d:%02d~%02d:%02d",
        start_min / 60, start_min % 60, end_min / 60, end_min % 60);
    lv_label_set_text((lv_obj_t *)label, buf);
}

void divoom_lvgl_calendar_active_event_cb(lv_event_t * e)
{
	int tip_index, active_id;
	lv_obj_t *cur_obj;
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target(e);
	
    if(code == LV_EVENT_KEY)
	{
		char *c = ((char *)lv_event_get_param(e));
        
        divoom_main_keypad_send_key_info(c[0], c[1]);
	
    }
}


void *divoom_lvgl_calendar_active_init(void* user_data, int menu_id)
{
    Pdivoom_lvgl_calendar_event_info pevent_info;
    Pdivoom_lvgl_calendar_active_ctrl phandle;
	struct tm cur_tm;    
    int start_y, bg_color, font_color;
    time_t cur_time = time(NULL);
    
    memset(&cur_tm, 0, sizeof(cur_tm));
    localtime_r(&cur_time, &cur_tm);

    phandle = divoom_system_malloc(sizeof(divoom_lvgl_calendar_active_ctrl));
    phandle->menu_id = menu_id;

    pevent_info = divoom_system_malloc(sizeof(divoom_lvgl_calendar_event_info));
    phandle->pevent_info = pevent_info;

    if(user_data)
    {
        phandle->calendar_flag = 0;
        phandle->get_data_flag = divoom_google_calendar_get_active_lvgl_event_info(pevent_info);
    }
    else
    {
        phandle->calendar_flag = 1;
        phandle->get_data_flag = divoom_outlook_calendar_get_active_lvgl_event_info(pevent_info);
    }
    divoom_print("phandle->get_data_flag: %d\n", phandle->get_data_flag);
    bg_color = (pevent_info->bg_color[0] << 16) + (pevent_info->bg_color[1] << 8) + pevent_info->bg_color[2];     
    font_color = (pevent_info->fr_color[0] << 16) + (pevent_info->fr_color[1] << 8) + pevent_info->fr_color[2]; 
    lv_style_init(&phandle->grid_type);
    lv_style_set_pad_all(&phandle->grid_type, 0);
    lv_style_set_pad_gap(&phandle->grid_type, 0);
    lv_style_set_border_width(&phandle->grid_type, 0);
    lv_style_set_border_side(&phandle->grid_type, LV_BORDER_SIDE_NONE);
    
    
    phandle->main_obj = lv_btn_create(NULL);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(0xEBEFF3), 0);

    void *btn_obj = lv_btn_create(phandle->main_obj);
	lv_obj_set_pos(btn_obj, 12, 12);
	lv_obj_set_size(btn_obj, DIVOOM_SCREEN_WIDTH - 24, DIVOOM_SCREEN_HIGH - 84);
    lv_obj_set_style_radius(btn_obj, 24, 0);
    lv_obj_add_style(btn_obj, &phandle->grid_type, 0);
	lv_obj_clear_flag(btn_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(btn_obj, lv_color_hex(bg_color), 0);
    phandle->btn_cout_obj = btn_obj;

    void *label_obj = lv_label_create(btn_obj);
	lv_obj_set_pos(label_obj, 0, 20);
	lv_obj_set_width(label_obj, DIVOOM_SCREEN_WIDTH - 24);
	lv_obj_set_style_text_color(label_obj, lv_color_hex(font_color ? font_color : 0x000000), 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(36, phandle->menu_id), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    divoom_lvgl_calendar_active_set_date_label(label_obj, &cur_tm);
    phandle->date_label_obj = label_obj;

    label_obj = lv_label_create(btn_obj);
	lv_obj_set_pos(label_obj, 0, 68);
	lv_obj_set_width(label_obj, DIVOOM_SCREEN_WIDTH - 24);
	lv_obj_set_style_text_color(label_obj, lv_color_hex(font_color ? font_color : 0x000000), 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(48, phandle->menu_id), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    divoom_lvgl_calendar_active_set_time_label(label_obj, pevent_info->start_time, pevent_info->end_time);
    phandle->time_label_obj = label_obj;

    start_y = CAL_ACTIVE_TITLE_START_Y;
    label_obj = lv_label_create(btn_obj);
	lv_obj_set_pos(label_obj, CAL_ACTIVE_TITLE_PAD_X, start_y);
	lv_obj_set_width(label_obj, CAL_ACTIVE_TITLE_WIDTH);
    lv_label_set_long_mode(label_obj, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_color(label_obj, lv_color_hex(font_color ? font_color : 0x000000), 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    if(pevent_info->title && strlen(pevent_info->title))
    {
    	lv_label_set_text(label_obj, pevent_info->title);
    }
    else
    {
    	lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("NoTitle"));
    }
    lv_obj_update_layout(label_obj);
    start_y += lv_obj_get_height(label_obj) + 12;
    phandle->title_label_obj = label_obj;

    label_obj = lv_label_create(btn_obj);
	lv_obj_set_pos(label_obj, CAL_ACTIVE_TITLE_PAD_X, start_y);
	lv_obj_set_width(label_obj, CAL_ACTIVE_TITLE_WIDTH);
    lv_label_set_long_mode(label_obj, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_color(label_obj, lv_color_hex(font_color ? font_color : 0x000000), 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    if(pevent_info->msg_info && strlen(pevent_info->msg_info))
    {
	    lv_label_set_text(label_obj, pevent_info->msg_info);
    }
    else
    {
	    lv_label_set_text(label_obj, "  ");
    }
    lv_obj_update_layout(label_obj);
    phandle->desc_label_obj = label_obj;

    btn_obj = lv_btn_create(phandle->main_obj);
	lv_obj_set_pos(btn_obj, (DIVOOM_SCREEN_WIDTH - 240) / 2, DIVOOM_SCREEN_HIGH - 64);
	lv_obj_set_size(btn_obj, 240, 48);
    lv_obj_set_style_radius(btn_obj, 24, 0);
    lv_obj_set_style_bg_color(btn_obj, lv_color_hex(0XFDB548), 0);
	lv_obj_clear_flag(btn_obj, LV_OBJ_FLAG_SCROLLABLE);

    label_obj = lv_label_create(btn_obj);
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0xffffff), 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("Stop"));
    lv_obj_center(label_obj);

    lv_obj_add_event_cb(phandle->main_obj, divoom_lvgl_calendar_active_event_cb, LV_EVENT_KEY, phandle);

    return phandle;
}

void divoom_lvgl_calendar_active_update_time_handle(struct _lv_timer_t *time_data)
{
    divoom_lvgl_calendar_event_info pevent_info = {0};
    Pdivoom_lvgl_calendar_active_ctrl phandle = (Pdivoom_lvgl_calendar_active_ctrl)lv_timer_get_user_data(time_data);  
    struct tm cur_tm;    
    int start_y, bg_color, font_color;
    time_t cur_time = time(NULL);
    void *label_obj;  
    
    memset(&cur_tm, 0, sizeof(cur_tm));
    localtime_r(&cur_time, &cur_tm);

    if(phandle->calendar_flag == 0)
    {
        if(divoom_google_calendar_get_active_lvgl_event_info(&pevent_info) == 0)
        {
            return;
        }
    }
    else
    {
        if(divoom_outlook_calendar_get_active_lvgl_event_info(&pevent_info) == 0)
        {
            return;
        }
    }
    
    if(phandle->get_data_flag == 0 || pevent_info.start_time != phandle->pevent_info->start_time)
    {        
        //更新
        phandle->get_data_flag = 1;
        bg_color = (pevent_info.bg_color[0] << 16) + (pevent_info.bg_color[1] << 8) + pevent_info.bg_color[2];     
        font_color = (pevent_info.fr_color[0] << 16) + (pevent_info.fr_color[1] << 8) + pevent_info.fr_color[2]; 
    	lv_obj_set_style_bg_color(phandle->btn_cout_obj, lv_color_hex(bg_color), 0);
        if(phandle->pevent_info->msg_info)
        {
            divoom_system_free(phandle->pevent_info->msg_info);
        }
        if(phandle->pevent_info->title)
        {
            divoom_system_free(phandle->pevent_info->title);
        }
        memcpy(phandle->pevent_info, &pevent_info, sizeof(pevent_info));
        divoom_lvgl_calendar_active_set_date_label(phandle->date_label_obj, &cur_tm);
	    lv_obj_set_style_text_color(phandle->date_label_obj, lv_color_hex(font_color), 0);
        divoom_lvgl_calendar_active_set_time_label(phandle->time_label_obj, pevent_info.start_time, pevent_info.end_time);
        
	    lv_obj_set_style_text_color(phandle->time_label_obj, lv_color_hex(font_color), 0);
        start_y = CAL_ACTIVE_TITLE_START_Y;
        lv_obj_clear_flag(phandle->title_label_obj , LV_OBJ_FLAG_HIDDEN);
        
        if(phandle->pevent_info->title && strlen(phandle->pevent_info->title))
        {
    	    lv_label_set_text(phandle->title_label_obj, phandle->pevent_info->title);    
        }
        else
        {
    	    lv_label_set_text(phandle->title_label_obj, divoom_menu_control_get_disp_str("NoTitle"));    
        }
	    lv_obj_set_style_text_color(phandle->title_label_obj, lv_color_hex(font_color), 0);
        lv_obj_update_layout(phandle->title_label_obj);
        start_y += lv_obj_get_height(phandle->title_label_obj) + 12;
        
    	lv_obj_set_pos(phandle->desc_label_obj, CAL_ACTIVE_TITLE_PAD_X, start_y);
        if(phandle->pevent_info->msg_info && strlen(phandle->pevent_info->msg_info))
        {
    	    lv_label_set_text(phandle->desc_label_obj, phandle->pevent_info->msg_info);    
        }
        else
        {
    	    lv_label_set_text(phandle->desc_label_obj, "  ");    
        }
	    lv_obj_set_style_text_color(phandle->desc_label_obj, lv_color_hex(font_color), 0);
        
    }
    else 
    {
        //检查是否结束
        if(pevent_info.msg_info)
        {
            divoom_system_free(pevent_info.msg_info);
        }
        if(pevent_info.title)
        {
            divoom_system_free(pevent_info.title);
        }
        if(cur_tm.tm_hour * 60 + cur_tm.tm_min >= phandle->pevent_info->end_time)
        {
            divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_ACTIVE_CALENDAR_TIMEOUT);
        }
    }

    
}


void divoom_lvgl_calendar_active_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_calendar_active_ctrl phandle = (Pdivoom_lvgl_calendar_active_ctrl)handle;    

    lv_group_focus_obj(phandle->main_obj);
    
    lv_scr_load(phandle->main_obj);
    phandle->time_handle = lv_timer_create(divoom_lvgl_calendar_active_update_time_handle, 500, phandle);
}


void divoom_lvgl_calendar_active_exit(void *handle)
{
    Pdivoom_lvgl_calendar_active_ctrl phandle = (Pdivoom_lvgl_calendar_active_ctrl)handle;    

    if(phandle->time_handle)
    {
        lv_timer_del(phandle->time_handle);
    }
    lv_style_reset(&phandle->grid_type);
    lv_obj_del(phandle->main_obj);
    if(phandle->pevent_info->msg_info)
    {
        divoom_system_free(phandle->pevent_info->msg_info);
    }
    if(phandle->pevent_info->title)
    {
        divoom_system_free(phandle->pevent_info->title);
    }
    divoom_system_free(phandle->pevent_info);
    divoom_system_free(phandle);
}




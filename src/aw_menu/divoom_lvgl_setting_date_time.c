#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"

typedef struct _divoom_lvgl_setting_data_format
{
    int  format_flag;
    void *main_obj;
    void *back_obj;
	void *btn_tbl_obj[DIVOOM_DISP_TIME_FORMAT_END_TYPE];
    void *label_tbl_obj[DIVOOM_DISP_TIME_FORMAT_END_TYPE];
    void *select_tbl_obj[DIVOOM_DISP_TIME_FORMAT_END_TYPE];
}divoom_lvgl_setting_data_format, *Pdivoom_lvgl_setting_data_format;

typedef struct _divoom_lvgl_setting_date_time_ctrl
{
    int menu_id;
    lv_style_t normal_style;
	lv_style_t btn_normal_style;
	lv_style_t btn_press_style;
    void *main_obj;
    void *back_obj;
	void *hour24_btn_obj;
    void *hour24_image_obj;
    void *date_format_obj;
	void *pdirect_handle;
	void *first_active_obj;
    divoom_lvgl_setting_data_format data_format_menu;
}divoom_lvgl_setting_date_time_ctrl, *Pdivoom_lvgl_setting_date_time_ctrl;

const char *divoom_lvgl_system_setting_date_time_format_flag_str[] =
{
    "YYYY-MM-DD",
    "DD-MM-YYYY",
    "MM-DD-YYYY",
    "YYYY.MM.DD",
    "DD.MM.YYYY",
    "MM.DD.YYYY",
    "YYYY/MM/DD",
    "DD/MM/YYYY",
    "MM/DD/YYYY",    
};

static void divoom_lvgl_setting_date_time_menu_event_cb(lv_event_t * e)
{
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_setting_date_time_ctrl pmenu_handle = (Pdivoom_lvgl_setting_date_time_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    /*if(code == LV_EVENT_KEY)
    {
        Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();
        if(pmenu_handle->back_obj == cur_obj)
        {
            divoom_menu_control_return_pre();
            return;        
        }
        else if(pmenu_handle->date_format_obj == cur_obj)
        {
            pmenu_handle->data_format_menu.format_flag = divoom_main_get_time_format_type();
            
            for(int i = 0; i < DIVOOM_DISP_TIME_FORMAT_END_TYPE; i++)
            {
                if(i == pmenu_handle->data_format_menu.format_flag)
                {
                    lv_obj_clear_flag(pmenu_handle->data_format_menu.select_tbl_obj[i], LV_OBJ_FLAG_HIDDEN);
                }
                else
                {
                    lv_obj_add_flag(pmenu_handle->data_format_menu.select_tbl_obj[i], LV_OBJ_FLAG_HIDDEN);
                }
            }
            lv_obj_clear_flag(pmenu_handle->data_format_menu.main_obj, LV_OBJ_FLAG_HIDDEN);
        }
        else if(pmenu_handle->hour24_image_obj == cur_obj)
        {
            if(psys_config->hour12_24)
            {
                psys_config->hour12_24 = 0;
                divoom_menu_control_lv_img_set_src(pmenu_handle->hour24_image_obj, "26/26_off.png");
            }
            else
            {
                psys_config->hour12_24 = 1;
                divoom_menu_control_lv_img_set_src(pmenu_handle->hour24_image_obj, "26/26_on.png");
            }
        }
    }*/
    if(code == LV_EVENT_KEY)
    {
    	int active_id;
		char c = *((char *)lv_event_get_param(e));
		Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();
		divoom_lvgl_setting_data_format *pdata_format_menu = &pmenu_handle->data_format_menu;
		
		if(c == DIVOOM_LVGL_KEY_PRE)
		{
			lv_group_focus_obj(divoom_menu_direct_ctrl_get(pmenu_handle->pdirect_handle, cur_obj, DIVOOM_MENU_DIRECT_DOWN));
		}
		else if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			lv_group_focus_obj(divoom_menu_direct_ctrl_get(pmenu_handle->pdirect_handle, cur_obj, DIVOOM_MENU_DIRECT_UP));
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			active_id = divoom_menu_direct_ctrl_get_pos(pmenu_handle->pdirect_handle, cur_obj);
			if(active_id == 0)
			{
				if(psys_config->hour12_24)
            	{
                	psys_config->hour12_24 = 0;
                	divoom_menu_control_lv_img_set_src(pmenu_handle->hour24_image_obj, "26/26_off.png");
            	}
            	else
            	{
                	psys_config->hour12_24 = 1;
                	divoom_menu_control_lv_img_set_src(pmenu_handle->hour24_image_obj, "26/26_on.png");
           	 	}
			}
			else
			{
				for(int i = 0; i < sizeof(divoom_lvgl_system_setting_date_time_format_flag_str)/sizeof(divoom_lvgl_system_setting_date_time_format_flag_str[0]); i++)
        		{
            		if(pdata_format_menu->btn_tbl_obj[i] == cur_obj)
            		{
                		if(i != pdata_format_menu->format_flag)
                		{
                    		divoom_main_set_time_format_type(i);
                    		lv_obj_clear_flag(pdata_format_menu->select_tbl_obj[i], LV_OBJ_FLAG_HIDDEN);
                    		lv_obj_add_flag(pdata_format_menu->select_tbl_obj[pdata_format_menu->format_flag], LV_OBJ_FLAG_HIDDEN);
                    		pdata_format_menu->format_flag = i;
                		}
                		break;
            		}
        		}
			}
			divoom_main_save_config();
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_main_send_sys_settings_config();
			divoom_menu_control_return_pre();
		}
		else
		{
			return;
		}
	}
}

/*static void divoom_lvgl_setting_date_format_menu_event_cb(lv_event_t * e)
{
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_setting_date_time_ctrl phandle = (Pdivoom_lvgl_setting_date_time_ctrl)lv_event_get_user_data(e);
    divoom_lvgl_setting_data_format *pmenu_handle = &phandle->data_format_menu;
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_KEY)
    {
        Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();
        if(pmenu_handle->back_obj == cur_obj)
        {
            lv_obj_add_flag(pmenu_handle->main_obj, LV_OBJ_FLAG_HIDDEN);
            //更新显示
            lv_label_set_text(phandle->date_format_obj, divoom_menu_control_get_disp_str(divoom_lvgl_system_setting_date_time_format_flag_str[pmenu_handle->format_flag]));
        }
        for(int i = 0; i < sizeof(divoom_lvgl_system_setting_date_time_format_flag_str)/sizeof(divoom_lvgl_system_setting_date_time_format_flag_str[0]); i++)
        {
            if(pmenu_handle->label_tbl_obj[i] == cur_obj)
            {
                if(i != pmenu_handle->format_flag)
                {
                    divoom_main_set_time_format_type(i);
                    lv_obj_clear_flag(pmenu_handle->select_tbl_obj[i], LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(pmenu_handle->select_tbl_obj[pmenu_handle->format_flag], LV_OBJ_FLAG_HIDDEN);
                    pmenu_handle->format_flag = i;
                }
                return;
            }
        }
    }
}*/

void divoom_lvgl_setting_create_date_format_menu(Pdivoom_lvgl_setting_date_time_ctrl phandle, Pdivoom_sys_setting_info psys_config)
{
    int i;
    void *image_obj, *label_obj, *cout_obj, *btn_obj;
    divoom_lvgl_setting_data_format *pmenu_handle = &phandle->data_format_menu;
    pmenu_handle->format_flag = divoom_main_get_time_format_type();

    //pmenu_handle->main_obj = lv_img_create(phandle->main_obj);  
    //lv_obj_set_pos(pmenu_handle->main_obj, 0, 0);
	//lv_obj_set_size(pmenu_handle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);  
    //divoom_menu_control_lv_img_set_src(pmenu_handle->main_obj, "2/2_bg.png");
    //lv_obj_add_style(pmenu_handle->main_obj, &phandle->normal_style, 0);
    //lv_obj_clear_flag(pmenu_handle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
    
    //cout_obj = lv_img_create(pmenu_handle->main_obj);  
    //lv_obj_set_pos(cout_obj, 149, 388);
	//lv_obj_set_size(cout_obj, 501, 504);  
    //divoom_menu_control_lv_img_set_src(cout_obj, "26/26_date_bg.png");
    
    //image_obj = lv_img_create(cout_obj);  
    //lv_obj_set_pos(image_obj, 21, 24);
	//lv_obj_set_size(image_obj, 48, 48);  
    //divoom_menu_control_lv_img_set_src(image_obj, "26/26_small_back.png");
    //lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_add_event_cb(image_obj, divoom_lvgl_setting_date_format_menu_event_cb, LV_EVENT_KEY, phandle);
    //pmenu_handle->back_obj = image_obj;
    
    //label_obj = lv_label_create(cout_obj);
    //lv_obj_set_pos(label_obj, 86, 27);
	//lv_obj_set_size(label_obj, 330, 42);
    //lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("Date_format"));
    //lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);
    //lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    //lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(36), 0);  
	cout_obj = phandle->main_obj;
	
    for(i = 0; i < sizeof(divoom_lvgl_system_setting_date_time_format_flag_str)/sizeof(divoom_lvgl_system_setting_date_time_format_flag_str[0]); i++)
    {
		btn_obj = lv_btn_create(cout_obj);
		lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
		lv_obj_add_style(btn_obj, &phandle->btn_press_style, LV_STATE_FOCUSED);
		lv_obj_add_style(btn_obj, &phandle->btn_press_style, LV_STATE_FOCUS_KEY);
		lv_obj_set_style_pad_all(btn_obj, 0, 0);
		lv_obj_set_size(btn_obj, 712, 90);
		lv_obj_set_pos(btn_obj, 44, 348+98*i);
		pmenu_handle->btn_tbl_obj[i] = btn_obj;
		divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, i+1);
		lv_obj_add_event_cb(btn_obj, divoom_lvgl_setting_date_time_menu_event_cb, LV_EVENT_KEY, phandle);
		
        label_obj = lv_label_create(cout_obj);
        lv_obj_set_pos(label_obj, 76, 375 + i * 98);
        lv_obj_set_size(label_obj, 393, 36);
        lv_label_set_text(label_obj, divoom_lvgl_system_setting_date_time_format_flag_str[i]);
        lv_obj_set_style_text_color(label_obj, lv_color_hex(0x0), 0);
        //lv_obj_set_style_text_opa(label_obj, LV_OPA_100*85/100, 0);
        lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, phandle->menu_id), 0); 
        //lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
        //lv_obj_add_event_cb(label_obj, divoom_lvgl_setting_date_format_menu_event_cb, LV_EVENT_KEY, phandle);
        pmenu_handle->label_tbl_obj[i] = label_obj;
        
        image_obj = lv_img_create(btn_obj);  
        //lv_obj_set_pos(image_obj, 688, 375 + i * 95);
        lv_obj_set_pos(image_obj, 644, 27);
    	lv_obj_set_size(image_obj, 36, 36);  
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_lang_select.png");
        pmenu_handle->select_tbl_obj[i] = image_obj;
		if(i == pmenu_handle->format_flag)
		{
			lv_obj_clear_flag(image_obj, LV_OBJ_FLAG_HIDDEN);
		}
		else
		{
			lv_obj_add_flag(image_obj, LV_OBJ_FLAG_HIDDEN);
		}
    }
    
    //lv_obj_add_flag(pmenu_handle->main_obj, LV_OBJ_FLAG_HIDDEN);
}

void *divoom_lvgl_setting_date_time_init(void *user_data, int menu_id)
{
    Pdivoom_lvgl_setting_date_time_ctrl pmenu_handle = divoom_system_malloc(sizeof(divoom_lvgl_setting_date_time_ctrl));

    void *image_obj, *label_obj, *slide_obj, *btn_obj;
    int time_format = divoom_main_get_time_format_type();
    Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();
	pmenu_handle->first_active_obj = NULL;
	pmenu_handle->menu_id = menu_id;
	pmenu_handle->pdirect_handle = divoom_menu_direct_ctrl_create(1, 10);
	//divoom_menu_control_init_btn_stype(&pmenu_handle->btn_press_style, NULL, &pmenu_handle->btn_normal_style, NULL, NULL, NULL, 19);

	lv_style_init(&pmenu_handle->btn_normal_style);
	lv_style_set_bg_color(&pmenu_handle->btn_normal_style, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&pmenu_handle->btn_normal_style, 0);
	lv_style_set_border_width(&pmenu_handle->btn_normal_style, 4);
	lv_style_set_border_color(&pmenu_handle->btn_normal_style, lv_color_hex(0xE4E4E4));
	lv_style_set_radius(&pmenu_handle->btn_normal_style, 17);
	lv_style_set_shadow_width(&pmenu_handle->btn_normal_style, 0);
	lv_style_set_outline_width(&pmenu_handle->btn_normal_style, 0);

	lv_style_init(&pmenu_handle->btn_press_style);
	lv_style_set_bg_color(&pmenu_handle->btn_press_style, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&pmenu_handle->btn_press_style, 0);
	lv_style_set_border_width(&pmenu_handle->btn_press_style, 4);
	lv_style_set_border_color(&pmenu_handle->btn_press_style, lv_color_hex(0x6D6D6D));
	lv_style_set_radius(&pmenu_handle->btn_press_style, 17);
	lv_style_set_shadow_width(&pmenu_handle->btn_press_style, 0);
	lv_style_set_outline_width(&pmenu_handle->btn_press_style, 0);
	
    pmenu_handle->main_obj = lv_obj_create(NULL);  
    lv_obj_set_style_bg_color(pmenu_handle->main_obj, lv_color_hex(0xE4E4E4), LV_PART_MAIN);
    lv_obj_set_pos(pmenu_handle->main_obj, 0, 0);
	lv_obj_set_size(pmenu_handle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);  
    lv_obj_add_style(pmenu_handle->main_obj, &pmenu_handle->normal_style, 0);
    lv_obj_set_scrollbar_mode(pmenu_handle->main_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(pmenu_handle->main_obj, LV_DIR_VER);
    lv_obj_clear_flag(pmenu_handle->main_obj, LV_OBJ_FLAG_SCROLLABLE);


    //image_obj = lv_img_create(pmenu_handle->main_obj);
	//lv_obj_set_pos(image_obj, 19, 50);
    //lv_obj_set_size(image_obj, 78, 78);
    //divoom_menu_control_lv_img_set_src(image_obj, "26/26_back.png");
    //pmenu_handle->back_obj = image_obj;
    //lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_add_event_cb(image_obj, divoom_lvgl_setting_date_time_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
    
    label_obj = lv_label_create(pmenu_handle->main_obj);
    lv_obj_set_pos(label_obj, 316, 69);
	lv_obj_set_size(label_obj, 600, 40);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("DateTime"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, pmenu_handle->menu_id), 0);  
    

    //image_obj = lv_img_create(pmenu_handle->main_obj);
	//lv_obj_set_pos(image_obj, 37, 190);
    //lv_obj_set_size(image_obj, 726, 180);
    //divoom_menu_control_lv_img_set_src(image_obj, "26/26_2line_bg.png");

	//24小时制开关
	btn_obj = lv_btn_create(pmenu_handle->main_obj);
	lv_obj_set_pos(btn_obj, 44, 164);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_add_style(btn_obj, &pmenu_handle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &pmenu_handle->btn_press_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &pmenu_handle->btn_press_style, LV_STATE_FOCUS_KEY);
	divoom_menu_direct_ctrl_update(pmenu_handle->pdirect_handle, btn_obj, 0, 0);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_setting_date_time_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
	pmenu_handle->hour24_btn_obj = btn_obj;
	if(pmenu_handle->first_active_obj == NULL)
	{
		pmenu_handle->first_active_obj = btn_obj;
	}
	
    label_obj = lv_label_create(pmenu_handle->main_obj);
    lv_obj_set_pos(label_obj, 76, 192);
	lv_obj_set_size(label_obj, 400, 34);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("24_hours"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, pmenu_handle->menu_id), 0);  
    
    image_obj = lv_img_create(pmenu_handle->main_obj);
	lv_obj_set_pos(image_obj, 652, 187);
    lv_obj_set_size(image_obj, 72, 44);
    if(psys_config->hour12_24)
    {
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_on.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_off.png");
    }
    pmenu_handle->hour24_image_obj = image_obj;
    //lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_add_event_cb(image_obj, divoom_lvgl_setting_date_time_menu_event_cb, LV_EVENT_KEY, pmenu_handle);

	//日期格式
    label_obj = lv_label_create(pmenu_handle->main_obj);
    lv_obj_set_pos(label_obj, 74, 305);
	lv_obj_set_size(label_obj, 300, 32);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("Date_format"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x636363), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(24, pmenu_handle->menu_id), 0);  
    
    //label_obj = lv_label_create(pmenu_handle->main_obj);
    //lv_obj_set_pos(label_obj, 419, 304);
	//lv_obj_set_size(label_obj, 312, 42);
    //lv_label_set_text(label_obj, divoom_menu_control_get_disp_str(divoom_lvgl_system_setting_date_time_format_flag_str[time_format]));
    //lv_obj_set_style_text_color(label_obj, lv_color_hex(0X8F8F8F), 0);
    //lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_RIGHT, 0);
    //lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32), 0);  
    //lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_add_event_cb(label_obj, divoom_lvgl_setting_date_time_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
    //pmenu_handle->date_format_obj = label_obj;

    divoom_lvgl_setting_create_date_format_menu(pmenu_handle, psys_config);
    

    return pmenu_handle;
}

void divoom_lvgl_setting_date_time_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_setting_date_time_ctrl phandle = (Pdivoom_lvgl_setting_date_time_ctrl)handle;

	if(phandle->first_active_obj)
	{
		lv_group_focus_obj(phandle->first_active_obj);
		phandle->first_active_obj = NULL;
	}
    lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_setting_date_time_exit(void *handle)
{
    Pdivoom_lvgl_setting_date_time_ctrl phandle = (Pdivoom_lvgl_setting_date_time_ctrl)handle;

    lv_obj_del(phandle->main_obj);
    lv_style_reset(&phandle->normal_style);
	lv_style_reset(&phandle->btn_normal_style);
	lv_style_reset(&phandle->btn_press_style);
	divoom_menu_direct_ctrl_destory(phandle->pdirect_handle);
    divoom_system_free(phandle);
    return ;
}





#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"

typedef struct _divoom_lvgl_normal_menu_list_ctrl
{
    int menu_items;
    int muti_flag;  //是否多选
    int last_select_id;     //最后一个选择项
    int menu_id;
    void *para;
    void *main_obj;
    void *back_obj;
    void *confirm_obj;
	void *cur_active_obj;
    lv_style_t btn_normal;
    lv_style_t btn_press;
	void *tip_label_obj; 
	char *title_tip[DIVOOM_LVGL_NORMAL_MENU_MAX_SELECT_NUM];
    uint8  select_info[DIVOOM_LVGL_NORMAL_MENU_MAX_SELECT_NUM];   //是否选择
    void *select_obj[DIVOOM_LVGL_NORMAL_MENU_MAX_SELECT_NUM];
    void *image_obj[DIVOOM_LVGL_NORMAL_MENU_MAX_SELECT_NUM];
    void *btn_obj[DIVOOM_LVGL_NORMAL_MENU_MAX_SELECT_NUM];
    void *pdirect_menu_handle;
    divoom_lvgl_normal_menu_list_callback *pcallback;
    divoom_lvgl_normal_menu_list_change_callback *pchange_callback;
}divoom_lvgl_normal_menu_list_ctrl, *Pdivoom_lvgl_normal_menu_list_ctrl;

static void divoom_lvgl_normal_menu_list_menu_event_cb(lv_event_t * e)
{
	int cur_pos;
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_normal_menu_list_ctrl phandle = (Pdivoom_lvgl_normal_menu_list_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_KEY)
    {        
        char *key_code = ((char *)lv_event_get_param(e));

        switch(key_code[0])
        {            
            case DIVOOM_LVGL_KEY_NEXT:
            case DIVOOM_LVGL_KEY_PRE:
                cur_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_menu_handle, cur_obj, key_code[0]==DIVOOM_LVGL_KEY_NEXT?DIVOOM_MENU_DIRECT_LEFT:DIVOOM_MENU_DIRECT_RIGHT);                            
                lv_group_focus_obj(cur_obj);

				if(phandle->tip_label_obj)
				{
					cur_pos = divoom_menu_direct_ctrl_get_pos(phandle->pdirect_menu_handle, cur_obj);				
					lv_label_set_text(phandle->tip_label_obj, phandle->title_tip[cur_pos]);
				}
                break;
               
            case DIVOOM_LVGL_KEY_ESC:
                if(phandle->pcallback(phandle->para, phandle->muti_flag, phandle->select_info, phandle->menu_items))
                {
                    divoom_menu_control_return_pre();
                }
                break;
               
            case DIVOOM_LVGL_KEY_ENTER:
                for(int i = 0; i < phandle->menu_items; i++)
                {
                    if(phandle->btn_obj[i] == cur_obj)
                    {
                        if(phandle->muti_flag)
                        {
                            if(phandle->select_info[i])
                            {
                                lv_obj_add_flag(phandle->image_obj[i], LV_OBJ_FLAG_HIDDEN);
                                phandle->select_info[i] = 0;
                            }
                            else
                            {
                                lv_obj_clear_flag(phandle->image_obj[i], LV_OBJ_FLAG_HIDDEN);
                                phandle->select_info[i] = 1;
                            }
                        }
                        else if(i != phandle->last_select_id)
                        {
                            phandle->select_info[i] = 1;
                            if(phandle->last_select_id != -1)
                            {
                                phandle->select_info[phandle->last_select_id] = 0;
                                lv_obj_add_flag(phandle->image_obj[phandle->last_select_id], LV_OBJ_FLAG_HIDDEN);
                            }
                            lv_obj_clear_flag(phandle->image_obj[i], LV_OBJ_FLAG_HIDDEN);
                            phandle->last_select_id = i; 
                            if(phandle->pchange_callback)
                            {
                                phandle->pchange_callback(phandle->para, i);
                            }
                        }
                        break;
                    }
                }
                break;
        }
    }
}


void *divoom_lvgl_normal_menu_list_init(void *user_data, int menu_id)
{
    int i;
    void *image_obj, *label_obj, *slide_obj, *btn_obj;
    Pdivoom_lvgl_normal_menu_list_ctrl pmenu_handle;
    Pdivoom_lvgl_normal_menu_list_info para_info = (Pdivoom_lvgl_normal_menu_list_info)user_data;

    pmenu_handle = divoom_system_malloc(sizeof(divoom_lvgl_normal_menu_list_ctrl));
    pmenu_handle->pdirect_menu_handle = divoom_menu_direct_ctrl_create(1, para_info->menu_items);
	//divoom_menu_control_init_btn_stype(&pmenu_handle->btn_press, NULL, &pmenu_handle->btn_normal, NULL, NULL, NULL, 19);

	lv_style_init(&pmenu_handle->btn_normal);
	lv_style_set_bg_color(&pmenu_handle->btn_normal, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&pmenu_handle->btn_normal, 0);
	lv_style_set_border_width(&pmenu_handle->btn_normal, 4);
	lv_style_set_border_color(&pmenu_handle->btn_normal, lv_color_hex(0xF2F2F2));
	lv_style_set_radius(&pmenu_handle->btn_normal, 17);
	lv_style_set_shadow_width(&pmenu_handle->btn_normal, 0);
	lv_style_set_outline_width(&pmenu_handle->btn_normal, 0);

	lv_style_init(&pmenu_handle->btn_press);
	lv_style_set_bg_color(&pmenu_handle->btn_press, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&pmenu_handle->btn_press, 0);
	lv_style_set_border_width(&pmenu_handle->btn_press, 4);
	lv_style_set_border_color(&pmenu_handle->btn_press, lv_color_hex(0x6D6D6D));
	lv_style_set_radius(&pmenu_handle->btn_press, 17);
	lv_style_set_shadow_width(&pmenu_handle->btn_press, 0);
	lv_style_set_outline_width(&pmenu_handle->btn_press, 0);

	pmenu_handle->cur_active_obj = NULL;
	pmenu_handle->menu_id = menu_id;
    pmenu_handle->muti_flag = para_info->muti_flag;
    pmenu_handle->menu_items = para_info->menu_items;
    memcpy(pmenu_handle->select_info, para_info->select_info, para_info->menu_items);
    pmenu_handle->pcallback = para_info->pcallback;
    pmenu_handle->pchange_callback = para_info->pchange_callback;
    pmenu_handle->para = para_info->phandle;    
    pmenu_handle->main_obj = lv_obj_create(NULL);  
    lv_obj_set_style_bg_color(pmenu_handle->main_obj, lv_color_hex(0xE4E4E4), LV_PART_MAIN);
    lv_obj_set_pos(pmenu_handle->main_obj, 0, 0);
	lv_obj_set_size(pmenu_handle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);  

    lv_obj_add_style(pmenu_handle->main_obj, &pmenu_handle->btn_normal, 0);
    lv_obj_set_scrollbar_mode(pmenu_handle->main_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(pmenu_handle->main_obj, LV_DIR_VER);
	lv_obj_clear_flag(pmenu_handle->main_obj, LV_OBJ_FLAG_SCROLLABLE);

    #if 0
    image_obj = lv_img_create(pmenu_handle->main_obj);
	lv_obj_set_pos(image_obj, 19, 50);
    lv_obj_set_size(image_obj, 78, 78);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_back.png");
    pmenu_handle->back_obj = image_obj;
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_normal_menu_list_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
    #endif
    if(para_info->confirm_flag)
    {
        image_obj = lv_img_create(pmenu_handle->main_obj);
        lv_obj_set_pos(image_obj, 19, 44);
    	lv_obj_set_size(image_obj, 90, 90);
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_select.png");
        pmenu_handle->confirm_obj = image_obj;
        lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(image_obj, divoom_lvgl_normal_menu_list_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
    }
    

    if((para_info->title_image_flag == 1) && para_info->title_image_path)
    {
    	image_obj = lv_img_create(pmenu_handle->main_obj);
		divoom_menu_control_lv_img_set_src(image_obj, para_info->title_image_path);
		lv_obj_set_pos(image_obj, 372, 54);
    }
	else
	{
    	label_obj = lv_label_create(pmenu_handle->main_obj);
    	lv_obj_set_pos(label_obj, 100, 50);
		lv_obj_set_size(label_obj, 600, 78);
    	lv_label_set_text(label_obj, para_info->disp_title);
    	lv_obj_set_style_text_color(label_obj, lv_color_black(), 0);
    	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, menu_id), 0);  
	}

	if(para_info->title_tip_flag)
	{
		lv_obj_t* label_cont = lv_obj_create(pmenu_handle->main_obj);
		lv_obj_set_size(label_cont, 600, 72);
		lv_obj_set_pos(label_cont, 100, 110+5);
		lv_obj_set_style_bg_opa(label_cont, LV_OPA_TRANSP, 0);
    	lv_obj_set_style_border_width(label_cont, 0, 0);
		lv_obj_clear_flag(label_cont, LV_OBJ_FLAG_SCROLLABLE);

		pmenu_handle->tip_label_obj = lv_label_create(label_cont);
		//lv_obj_set_pos(pmenu_handle->tip_label_obj, 40, 114);
		//lv_obj_set_size(pmenu_handle->tip_label_obj, 720, 56);
		lv_obj_set_width(pmenu_handle->tip_label_obj, 600);
		lv_label_set_long_mode(pmenu_handle->tip_label_obj, LV_LABEL_LONG_WRAP);
		lv_obj_set_align(pmenu_handle->tip_label_obj, LV_ALIGN_CENTER);	//上下居中对齐
    	lv_obj_set_style_text_color(pmenu_handle->tip_label_obj, lv_color_hex(0x0E7AD9), 0);
    	lv_obj_set_style_text_align(pmenu_handle->tip_label_obj, LV_TEXT_ALIGN_CENTER, 0);
    	lv_obj_set_style_text_font(pmenu_handle->tip_label_obj, divoom_menu_control_get_font_info(24, menu_id), 0);  	
	}
	else
	{
		pmenu_handle->tip_label_obj = NULL;
	}
	
    if(para_info->bg_image_path)
    {
        image_obj = lv_img_create(pmenu_handle->main_obj);
        lv_obj_set_pos(image_obj, 37, 209);
    	lv_obj_set_size(image_obj, para_info->bg_width, para_info->bg_high);
        divoom_menu_control_lv_img_set_src(image_obj, para_info->bg_image_path);
    }

	lv_obj_t *cont = lv_obj_create(pmenu_handle->main_obj);
	lv_obj_remove_style(cont, NULL, LV_PART_SCROLLBAR);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_outline_width(cont, 0, 0);
    lv_obj_set_style_border_side(cont, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_pad_bottom(cont, 71, 0);
    lv_obj_set_size(cont, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH - 128);
	
	if(para_info->title_tip_flag)
    {
		lv_obj_set_pos(cont, 0, 128 + 73);
	}
	else
	{
		lv_obj_set_pos(cont, 0, 128);		
	}

    pmenu_handle->last_select_id  = -1;
    for(i = 0; i < para_info->menu_items; i++)
    {
        btn_obj  = lv_btn_create(cont);
        lv_obj_add_style(btn_obj, &pmenu_handle->btn_normal, 0);
        lv_obj_add_style(btn_obj, &pmenu_handle->btn_press, LV_STATE_FOCUSED);
        lv_obj_add_style(btn_obj, &pmenu_handle->btn_press, LV_STATE_FOCUS_KEY);
		lv_obj_set_style_pad_all(btn_obj, 0, 0);
		if(para_info->title_tip_flag)
        {
			lv_obj_set_pos(btn_obj, 44, 164 + 98 * i - 128 - 30);
    	}
		else
		{
			lv_obj_set_pos(btn_obj, 44, 164 + 98 * i - 128);
		}
		lv_obj_set_size(btn_obj, 712, 90);

		if((para_info->btn_image_flag == 1) && para_info->btn_image_path[i])
		{
			image_obj = lv_img_create(btn_obj);
			divoom_menu_control_lv_img_set_src(image_obj, para_info->btn_image_path[i]);
        	lv_obj_set_pos(image_obj, 32, 28);
		}
		else
		{
        	label_obj = lv_label_create(btn_obj);
        	lv_obj_set_pos(label_obj, 32, 28);
    		lv_obj_set_size(label_obj, 500, 34);
        	lv_label_set_text(label_obj, para_info->select_str[i]);
    	
        	lv_obj_set_style_text_color(label_obj, lv_color_black(), 0);
        	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
        	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, menu_id), 0);  
		}
        image_obj = lv_img_create(btn_obj);
    	lv_obj_set_pos(image_obj, 644, 27);// + 90 * i);
        lv_obj_set_size(image_obj, 36, 36);
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_lang_select.png");
        pmenu_handle->image_obj[i] = image_obj;
        if(para_info->select_info[i] == 0 || para_info->select_flag == 0)
        {
            lv_obj_add_flag(image_obj, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            pmenu_handle->last_select_id = i;
			pmenu_handle->cur_active_obj = btn_obj;
        }

		if(para_info->title_tip_flag)
		{
			pmenu_handle->title_tip[i] = para_info->title_tip[i];
		}

        //lv_obj_add_event_cb(btn_obj, divoom_lvgl_normal_menu_list_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
        lv_obj_add_event_cb(btn_obj, divoom_lvgl_normal_menu_list_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
        pmenu_handle->btn_obj[i] = btn_obj;
        divoom_menu_direct_ctrl_update(pmenu_handle->pdirect_menu_handle, btn_obj, 0, i);
    }   

	if(para_info->title_tip_flag)
	{
		if(pmenu_handle->last_select_id == -1)
		{
			lv_label_set_text(pmenu_handle->tip_label_obj, para_info->title_tip[0]);
		}
		else
		{
			lv_label_set_text(pmenu_handle->tip_label_obj, para_info->title_tip[pmenu_handle->last_select_id]);
		}
	}

    return pmenu_handle;    
}

void divoom_lvgl_normal_menu_list_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_normal_menu_list_ctrl phandle = (Pdivoom_lvgl_normal_menu_list_ctrl)handle;

	if(phandle->cur_active_obj)
	{
		lv_group_focus_obj(phandle->cur_active_obj);
		phandle->cur_active_obj = NULL;
	}
	else if(phandle->btn_obj[0])
	{
		lv_group_focus_obj(phandle->btn_obj[0]);
    }
	lv_scr_load(phandle->main_obj);
}


void divoom_lvgl_normal_menu_list_exit(void *handle)
{
    Pdivoom_lvgl_normal_menu_list_ctrl phandle = (Pdivoom_lvgl_normal_menu_list_ctrl)handle;

    lv_obj_del(phandle->main_obj);
    divoom_menu_direct_ctrl_destory(phandle->pdirect_menu_handle);
	lv_style_reset(&phandle->btn_normal);
	lv_style_reset(&phandle->btn_press);
    divoom_system_free(phandle);
}



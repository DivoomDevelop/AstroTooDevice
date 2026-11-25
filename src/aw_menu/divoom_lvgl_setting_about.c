#include "divoom_build_config.h"
#if DIVOOM_BUILD_SIMULATOR
#include <sys/statvfs.h>
#else
#include "divoom_rtos_statvfs_stub.h"
#endif
#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"

typedef struct _divoom_lvgl_setting_about_ctrl
{
    int menu_id;
    lv_style_t  normal_style;
	lv_style_t  btn_normal_style;
	lv_style_t  btn_select_style;
	void *first_active_obj;
	void *pdirect_handle;

	void *main_obj;
    //void *back_obj;
    void *auto_update_obj;
    void *new_version_obj;
    void *update_obj;
    void *update_more_obj;

	void *auto_update_btn;

	char update_info[512];
	void *box_btn[2];
	void *bg_cover;
	void *update_box;
	void *new_version_btn;
}divoom_lvgl_setting_about_ctrl, *Pdivoom_lvgl_setting_about_ctrl;

static void divoom_lvgl_setting_about_menu_event_cb(lv_event_t * e)
{
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_setting_about_ctrl phandle = (Pdivoom_lvgl_setting_about_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    /*if(code == LV_EVENT_CLICKED)
    {
        Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();
        
        if(phandle->back_obj == cur_obj)
        {
            divoom_menu_control_return_pre();
            return;
        }
        if(phandle->auto_update_obj == cur_obj)
        {
            Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();

            if(psys_config->auto_update)
            {
                psys_config->auto_update = 0;
                divoom_menu_control_lv_img_set_src(cur_obj, "26/26_off.png");
            }
            else
            {
                psys_config->auto_update = 1;
                divoom_menu_control_lv_img_set_src(cur_obj, "26/26_on.png");
            }
            return;
        }
        if(phandle->new_version_obj == cur_obj || phandle->update_obj == cur_obj || phandle->update_more_obj == cur_obj)
        {
            //这个位置启动升级
            divoom_main_user_start_update_device();
            return;
        }
    }*/
    if(code == LV_EVENT_KEY)
    {
		char c = *((char *)lv_event_get_param(e));
			
		if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			lv_group_focus_obj(divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_obj, DIVOOM_MENU_DIRECT_LEFT));
		}
		else if(c == DIVOOM_LVGL_KEY_PRE)
		{
			lv_group_focus_obj(divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_obj, DIVOOM_MENU_DIRECT_RIGHT));
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			if(phandle->auto_update_btn == cur_obj)
			{
				Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();

            	if(psys_config->auto_update)
            	{
                	psys_config->auto_update = 0;
                	divoom_menu_control_lv_img_set_src(phandle->auto_update_obj, "26/26_off.png");
            	}
            	else
            	{
            	    psys_config->auto_update = 1;
            	    divoom_menu_control_lv_img_set_src(phandle->auto_update_obj, "26/26_on.png");
            	}
				divoom_main_save_config();
				return;
			}
			else if(phandle->new_version_btn == cur_obj)
			{
				lv_obj_clear_flag(phandle->bg_cover, LV_OBJ_FLAG_HIDDEN);
				lv_obj_clear_flag(phandle->update_box, LV_OBJ_FLAG_HIDDEN);
				lv_group_focus_obj(phandle->box_btn[1]);
            	return;
			}
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

static void divoom_lvgl_system_setting_about_select_box_event_cb(lv_event_t * e)
{
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_setting_about_ctrl phandle = (Pdivoom_lvgl_setting_about_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);

	 if(code == LV_EVENT_KEY)
    {
		char c = *((char *)lv_event_get_param(e));
			
		if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			if(cur_obj == phandle->box_btn[0])
			{
				lv_group_focus_obj(phandle->box_btn[1]);
			}
			else
			{
				lv_group_focus_obj(phandle->box_btn[0]);
			}
		}
		else if(c == DIVOOM_LVGL_KEY_PRE)
		{
			if(cur_obj == phandle->box_btn[0])
			{
				lv_group_focus_obj(phandle->box_btn[1]);
			}
			else
			{
				lv_group_focus_obj(phandle->box_btn[0]);
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			if(cur_obj == phandle->box_btn[0])//取消
			{
				lv_obj_add_flag(phandle->bg_cover, LV_OBJ_FLAG_HIDDEN);
				lv_obj_add_flag(phandle->update_box, LV_OBJ_FLAG_HIDDEN);
			}
			else//确定
			{
				divoom_main_user_start_update_device();
				lv_obj_add_flag(phandle->bg_cover, LV_OBJ_FLAG_HIDDEN);
				lv_obj_add_flag(phandle->update_box, LV_OBJ_FLAG_HIDDEN);
			}
			lv_group_focus_obj(phandle->new_version_btn);
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			lv_obj_add_flag(phandle->bg_cover, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(phandle->update_box, LV_OBJ_FLAG_HIDDEN);
			lv_group_focus_obj(phandle->new_version_btn);
		}
		else
		{
			return;
		}
	}
}


void divoom_lvgl_setting_about_create_box(Pdivoom_lvgl_setting_about_ctrl pmenu_handle)
{
	lv_obj_t *bg_img = lv_img_create(pmenu_handle->main_obj);
	divoom_menu_control_lv_img_set_src(bg_img, "26/26_bg_cover.png");
	lv_obj_set_pos(bg_img, 0, 0);
	pmenu_handle->bg_cover = bg_img;
	lv_obj_add_flag(pmenu_handle->bg_cover, LV_OBJ_FLAG_HIDDEN);

	lv_obj_t *image_obj = lv_img_create(pmenu_handle->main_obj);
	divoom_menu_control_lv_img_set_src(image_obj, "26/26_about_box.png");
	lv_obj_set_pos(image_obj, 80, 280);
	pmenu_handle->update_box = image_obj;
	lv_obj_add_flag(pmenu_handle->update_box, LV_OBJ_FLAG_HIDDEN);

	lv_obj_t *label_obj = lv_label_create(image_obj);
	lv_obj_set_pos(label_obj, 0, 23);
	lv_obj_set_size(label_obj, 640, 44);
	lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("DeviceUpdate"));
	//lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, pmenu_handle->menu_id), 0);

	lv_obj_t *label_obj1 = lv_label_create(image_obj);
	lv_obj_set_pos(label_obj1, 32, 111);
	lv_obj_set_size(label_obj1, 576, 450);
	lv_label_set_text(label_obj1, pmenu_handle->update_info);
	//lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
	lv_obj_set_style_text_align(label_obj1, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(label_obj1, divoom_menu_control_get_font_info(24, pmenu_handle->menu_id), 0);

	
	//取消
	lv_obj_t *btn_obj = lv_btn_create(image_obj);
	lv_obj_set_size(btn_obj, 282, 90);
	lv_obj_set_pos(btn_obj, 24, 426);
	lv_obj_set_style_radius(btn_obj, 99, 0);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_style_bg_color(btn_obj, lv_color_hex(0xF2F2F2), 0);
	lv_obj_set_style_shadow_width(btn_obj, 0, 0);
	lv_obj_set_style_border_width(btn_obj, 4, 0);
	lv_obj_set_style_outline_width(btn_obj, 0, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_border_color(btn_obj, lv_color_hex(0xA9A9A9), 0);
	lv_obj_set_style_border_color(btn_obj, lv_color_hex(0x000000), LV_STATE_FOCUSED);
	lv_obj_set_style_border_color(btn_obj, lv_color_hex(0x000000), LV_STATE_FOCUS_KEY);
	lv_obj_set_style_text_color(btn_obj, lv_color_hex(0xA9A9A9), 0);
	lv_obj_set_style_text_color(btn_obj, lv_color_hex(0x000000), LV_STATE_FOCUSED);
	lv_obj_set_style_text_color(btn_obj, lv_color_hex(0x000000), LV_STATE_FOCUS_KEY);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_about_select_box_event_cb, LV_EVENT_KEY, pmenu_handle);
	pmenu_handle->box_btn[0] = btn_obj;

	lv_obj_t *label_obj2 = lv_label_create(btn_obj);
	lv_obj_set_pos(label_obj2, 0, 23);
	lv_obj_set_size(label_obj2, 282, 44);
	lv_label_set_text(label_obj2, divoom_menu_control_get_disp_str("Cancel"));
	//lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
	lv_obj_set_style_text_align(label_obj2, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(label_obj2, divoom_menu_control_get_font_info(32, pmenu_handle->menu_id), 0);
	
	//确定
	lv_obj_t *btn_obj2 = lv_btn_create(image_obj);
	lv_obj_set_size(btn_obj2, 282, 90);
	lv_obj_set_pos(btn_obj2, 334, 426);
	lv_obj_set_style_radius(btn_obj2, 99, 0);
	lv_obj_set_style_pad_all(btn_obj2, 0, 0);
	lv_obj_set_style_bg_color(btn_obj2, lv_color_hex(0xF2F2F2), 0);
	lv_obj_set_style_shadow_width(btn_obj2, 0, 0);
	lv_obj_set_style_border_width(btn_obj2, 4, 0);
	lv_obj_set_style_outline_width(btn_obj2, 0, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_border_color(btn_obj2, lv_color_hex(0xA9A9A9), 0);
	lv_obj_set_style_border_color(btn_obj2, lv_color_hex(0x0), LV_STATE_FOCUSED);
	lv_obj_set_style_border_color(btn_obj2, lv_color_hex(0x0), LV_STATE_FOCUS_KEY);
	lv_obj_set_style_text_color(btn_obj2, lv_color_hex(0xA9A9A9), 0);
	lv_obj_set_style_text_color(btn_obj2, lv_color_hex(0x0), LV_STATE_FOCUSED);
	lv_obj_set_style_text_color(btn_obj2, lv_color_hex(0x0), LV_STATE_FOCUS_KEY);
	lv_obj_add_event_cb(btn_obj2, divoom_lvgl_system_setting_about_select_box_event_cb, LV_EVENT_KEY,pmenu_handle);
	pmenu_handle->box_btn[1] = btn_obj2;

	lv_obj_t *label_obj3 = lv_label_create(btn_obj2);
	lv_obj_set_pos(label_obj3, 0, 24);
	lv_obj_set_size(label_obj3, 282, 42);
	lv_label_set_text(label_obj3, divoom_menu_control_get_disp_str("Ok"));
	//lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
	lv_obj_set_style_text_align(label_obj3, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(label_obj3, divoom_menu_control_get_font_info(32, pmenu_handle->menu_id), 0);
}

void *divoom_lvgl_setting_about_init(void *user_data, int menu_id)
{
    Pdivoom_lvgl_setting_about_ctrl pmenu_handle = divoom_system_malloc(sizeof(divoom_lvgl_setting_about_ctrl));

    int i;
    void *image_obj, *label_obj, *slide_obj, *btn_obj; 
    struct statvfs stat;
    Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();
	//divoom_menu_control_init_btn_stype(&pmenu_handle->btn_select_style, NULL, &pmenu_handle->btn_normal_style, NULL, &pmenu_handle->btn_edit_style, NULL, 17);
	pmenu_handle->pdirect_handle = divoom_menu_direct_ctrl_create(3, 1);
	pmenu_handle->first_active_obj = NULL;
    pmenu_handle->menu_id = menu_id;
	
    pmenu_handle->main_obj = lv_obj_create(NULL);  
    divoom_menu_control_init_lable_style(&pmenu_handle->normal_style);
    lv_obj_set_style_bg_color(pmenu_handle->main_obj, lv_color_hex(0xE4E4E4), LV_PART_MAIN);
    lv_obj_set_pos(pmenu_handle->main_obj, 0, 0);
	lv_obj_set_size(pmenu_handle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);  

    lv_obj_add_style(pmenu_handle->main_obj, &pmenu_handle->normal_style, 0);
    lv_obj_set_scrollbar_mode(pmenu_handle->main_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(pmenu_handle->main_obj, LV_DIR_VER);

    /*image_obj = lv_img_create(pmenu_handle->main_obj);
	lv_obj_set_pos(image_obj, 19, 50);
    lv_obj_set_size(image_obj, 78, 78);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_back.png");
    pmenu_handle->back_obj = image_obj;
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_setting_about_menu_event_cb, LV_EVENT_CLICKED, pmenu_handle);
    


    image_obj = lv_img_create(pmenu_handle->main_obj);
    lv_obj_set_pos(image_obj, 37, 209);
	lv_obj_set_size(image_obj, 726, 180);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_2line_bg.png");
    */

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
	
    label_obj = lv_label_create(pmenu_handle->main_obj);
    lv_obj_set_pos(label_obj, 367, 69);
	lv_obj_set_size(label_obj, 600, 40);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("About"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, pmenu_handle->menu_id), 0);  

    //可用空间
	btn_obj = lv_btn_create(pmenu_handle->main_obj);
	lv_obj_add_style(btn_obj, &pmenu_handle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &pmenu_handle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &pmenu_handle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 164);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_setting_about_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
	divoom_menu_direct_ctrl_update(pmenu_handle->pdirect_handle, btn_obj, 0, 0);
	if(pmenu_handle->first_active_obj == NULL)
	{
		pmenu_handle->first_active_obj = btn_obj;
	}
	
    label_obj = lv_label_create(pmenu_handle->main_obj);
    lv_obj_set_pos(label_obj, 76, 192);
    lv_obj_set_size(label_obj, 300, 34);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("FreeRoom"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, pmenu_handle->menu_id), 0);  
    

    label_obj = lv_label_create(pmenu_handle->main_obj);
    lv_obj_set_pos(label_obj, 635, 192);
    lv_obj_set_size(label_obj, 271, 34);
    
    if (statvfs("/overlay", &stat) == 0) {
        
        unsigned long free = stat.f_bfree * (stat.f_bsize/1024)/1024;
        if(free > 1024*10)
        {
            lv_label_set_text_fmt(label_obj, "%ldG",free/1024);
        }
        else
        {
            lv_label_set_text_fmt(label_obj, "%ldM",free);
        }
    
    } else {
        lv_label_set_text(label_obj, "----");
    }
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x616161), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, pmenu_handle->menu_id), 0);  

	//自动更新
	btn_obj = lv_btn_create(pmenu_handle->main_obj);
	lv_obj_add_style(btn_obj, &pmenu_handle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &pmenu_handle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &pmenu_handle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 262);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_setting_about_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
	divoom_menu_direct_ctrl_update(pmenu_handle->pdirect_handle, btn_obj, 1, 0);
	pmenu_handle->auto_update_btn = btn_obj;
	
    label_obj = lv_label_create(pmenu_handle->main_obj);
    lv_obj_set_pos(label_obj, 76, 290);
    lv_obj_set_size(label_obj, 400, 34);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("AutoUpdate"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, pmenu_handle->menu_id), 0);  

    image_obj = lv_img_create(pmenu_handle->main_obj);
    lv_obj_set_pos(image_obj, 652, 285);
    lv_obj_set_size(image_obj, 72, 44);
    if(psys_config->auto_update)
    {
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_on.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_off.png");
    }
    //lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_add_event_cb(image_obj, divoom_lvgl_setting_about_menu_event_cb, LV_EVENT_CLICKED, pmenu_handle);
    pmenu_handle->auto_update_obj = image_obj;



    //image_obj = lv_img_create(pmenu_handle->main_obj);
    //lv_obj_set_pos(image_obj, 37, 415);
	//lv_obj_set_size(image_obj, 726, 90);
    //divoom_menu_control_lv_img_set_src(image_obj, "26/26_1line_bg.png");

	//新版本
	btn_obj = lv_btn_create(pmenu_handle->main_obj);
	lv_obj_add_style(btn_obj, &pmenu_handle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &pmenu_handle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &pmenu_handle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 404);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_setting_about_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
	pmenu_handle->new_version_btn = btn_obj;
	
	if(divoom_main_get_device_last_version() <= DIVOOM_SOFTWARE_VERSION)
    {        
        lv_obj_add_flag(btn_obj, LV_OBJ_FLAG_HIDDEN);
    }
	else
	{
		divoom_menu_direct_ctrl_update(pmenu_handle->pdirect_handle, btn_obj, 2, 0);
	}
	
    label_obj = lv_label_create(pmenu_handle->main_obj);
    lv_obj_set_pos(label_obj, 76, 432);
    lv_obj_set_size(label_obj, 400, 34);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("NewVersion"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, pmenu_handle->menu_id), 0);  
    //lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_add_event_cb(label_obj, divoom_lvgl_setting_about_menu_event_cb, LV_EVENT_CLICKED, pmenu_handle);
    pmenu_handle->new_version_obj = label_obj;
    if(divoom_main_get_device_last_version() <= DIVOOM_SOFTWARE_VERSION)
    {        
        lv_obj_add_flag(label_obj, LV_OBJ_FLAG_HIDDEN);
    }

    label_obj = lv_label_create(pmenu_handle->main_obj);
    lv_obj_set_pos(label_obj, 546-250, 432);
    lv_obj_set_size(label_obj, 400, 34);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("DownloadUpdate"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x0CA9F1), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, pmenu_handle->menu_id), 0);  
    //lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_add_event_cb(label_obj, divoom_lvgl_setting_about_menu_event_cb, LV_EVENT_CLICKED, pmenu_handle);
    pmenu_handle->update_obj = label_obj;
    if(divoom_main_get_device_last_version() <= DIVOOM_SOFTWARE_VERSION)
    {        
        lv_obj_add_flag(label_obj, LV_OBJ_FLAG_HIDDEN);
    }

    image_obj = lv_img_create(pmenu_handle->main_obj);
    lv_obj_set_pos(image_obj, 688, 431);
	lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    //lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_add_event_cb(image_obj, divoom_lvgl_setting_about_menu_event_cb, LV_EVENT_CLICKED, pmenu_handle);
    pmenu_handle->update_more_obj = image_obj;
    if(divoom_main_get_device_last_version() <= DIVOOM_SOFTWARE_VERSION)
    {        
        lv_obj_add_flag(image_obj, LV_OBJ_FLAG_HIDDEN);
    }


    label_obj = lv_label_create(pmenu_handle->main_obj);
    lv_obj_set_pos(label_obj, 73, 1032);
    lv_obj_set_size(label_obj, 654, 48);
    lv_label_set_text_fmt(label_obj, "%s:%d",divoom_menu_control_get_disp_str("DeviceVersion"),DIVOOM_SOFTWARE_VERSION);
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x636363), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(40, pmenu_handle->menu_id), 0);  

	if(divoom_main_get_device_last_version() <= DIVOOM_SOFTWARE_VERSION)
    {        
        
    }
	else
	{
		divoom_main_get_device_last_version_update_info(pmenu_handle->update_info);
		divoom_lvgl_setting_about_create_box(pmenu_handle);
	}
	
    return pmenu_handle;
}

void divoom_lvgl_setting_about_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_setting_about_ctrl phandle = (Pdivoom_lvgl_setting_about_ctrl)handle;

	if(phandle->first_active_obj)
	{
		lv_group_focus_obj(phandle->first_active_obj);
		phandle->first_active_obj = NULL;
	}
    lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_setting_about_exit(void *handle)
{
    Pdivoom_lvgl_setting_about_ctrl phandle = (Pdivoom_lvgl_setting_about_ctrl)handle;

    lv_obj_del(phandle->main_obj);
    lv_style_reset(&phandle->normal_style);
	lv_style_reset(&phandle->btn_select_style);
	lv_style_reset(&phandle->btn_normal_style);
	divoom_menu_direct_ctrl_destory(phandle->pdirect_handle);
    divoom_system_free(phandle);
    return ;
}


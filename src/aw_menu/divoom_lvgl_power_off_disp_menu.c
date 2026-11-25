#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_power_off_disp_menu.h"

typedef struct _divoom_lvgl_power_off_disp_menu_ctrl
{
    int menu_id;
	void *main_obj;

}divoom_lvgl_power_off_disp_menu_ctrl, *Pdivoom_lvgl_power_off_disp_menu_ctrl;

//Pdivoom_lvgl_power_off_disp_menu_ctrl pdivoom_lvgl_power_off_disp_menu_handle = NULL;

#if 0
void divoom_lvgl_top_power_off_disp_menu_del(void)
{
	if(pdivoom_lvgl_power_off_disp_menu_handle != NULL)
	{
		divoom_lvgl_power_off_disp_menu_open_screen();

		divoom_lvgl_top_control_remove_top(pdivoom_lvgl_power_off_disp_menu_handle);
		lv_obj_del_async(pdivoom_lvgl_power_off_disp_menu_handle->main_obj);
		divoom_system_free(pdivoom_lvgl_power_off_disp_menu_handle);

		pdivoom_lvgl_power_off_disp_menu_handle = NULL;
	}
}


void divoom_lvgl_top_power_off_disp_menu_event_cb(lv_event_t * e)
{
	Pdivoom_lvgl_power_off_disp_menu_ctrl phandle = (Pdivoom_lvgl_power_off_disp_menu_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *cur_obj = lv_event_get_target(e);

	if(code == LV_EVENT_KEY)
    {
		divoom_sys_msg_info msg_info = {0};
		
		msg_info.dest_id = DIVOOM_MODEL_MAIN;
		msg_info.msg_id = DIVOOM_MAIN_EVENT_POWER_OFF_DISP_MENU_CANCLE;
		divoom_sys_model_crl_msg_push(&msg_info);
	
		//divoom_lvgl_power_off_disp_menu_open_screen();
		divoom_lvgl_top_power_off_disp_menu_del();
	}
}


void divoom_lvgl_top_power_off_disp_menu_init(void)
{
	divoom_get_lvgl_lock();

	if(pdivoom_lvgl_power_off_disp_menu_handle != NULL)
	{
		divoom_free_lvgl_lock();
		return;
	}

	pdivoom_lvgl_power_off_disp_menu_handle = (Pdivoom_lvgl_power_off_disp_menu_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_power_off_disp_menu_ctrl));

	if(pdivoom_lvgl_power_off_disp_menu_handle == NULL)
	{
		divoom_free_lvgl_lock();
		return;
	}

	
	pdivoom_lvgl_power_off_disp_menu_handle->main_obj = lv_btn_create(lv_layer_top());

	lv_obj_remove_style_all(pdivoom_lvgl_power_off_disp_menu_handle->main_obj);
	lv_obj_set_style_bg_opa(pdivoom_lvgl_power_off_disp_menu_handle->main_obj, LV_OPA_100, 0);
	lv_obj_set_style_bg_color(pdivoom_lvgl_power_off_disp_menu_handle->main_obj, lv_color_hex(0x000000), 0);
	lv_obj_set_style_pad_all(pdivoom_lvgl_power_off_disp_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_border_width(pdivoom_lvgl_power_off_disp_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_shadow_width(pdivoom_lvgl_power_off_disp_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_outline_width(pdivoom_lvgl_power_off_disp_menu_handle->main_obj, 0, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_clear_flag(pdivoom_lvgl_power_off_disp_menu_handle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(pdivoom_lvgl_power_off_disp_menu_handle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(pdivoom_lvgl_power_off_disp_menu_handle->main_obj, 0, 0);
	lv_obj_add_event_cb(pdivoom_lvgl_power_off_disp_menu_handle->main_obj, divoom_lvgl_top_power_off_disp_menu_event_cb, LV_EVENT_KEY, pdivoom_lvgl_power_off_disp_menu_handle); 

	divoom_lvgl_power_off_disp_menu_close_screen();
	
	divoom_lvgl_top_control_add_top(pdivoom_lvgl_power_off_disp_menu_handle, pdivoom_lvgl_power_off_disp_menu_handle->main_obj);
	
	divoom_free_lvgl_lock();
	
	//return phandle;
}

void divoom_lvgl_top_power_off_disp_menu_exit(void)
{
	if(pdivoom_lvgl_power_off_disp_menu_handle != NULL)
	{
		divoom_get_lvgl_lock();
		//divoom_lvgl_power_off_disp_menu_open_screen();

		divoom_lvgl_top_control_remove_top(pdivoom_lvgl_power_off_disp_menu_handle);
		lv_obj_del(pdivoom_lvgl_power_off_disp_menu_handle->main_obj);
		divoom_system_free(pdivoom_lvgl_power_off_disp_menu_handle);

		pdivoom_lvgl_power_off_disp_menu_handle = NULL;
		divoom_free_lvgl_lock();
	}
}

#endif

void divoom_lvgl_power_off_disp_menu_close_screen(void)
{
	int on_off_switch = 0;
	divoom_sys_msg_info model_msg = {0};

	//model_msg.src_id = DIVOOM_MODEL_MAIN; 
	model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
    model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SCREEN_CTRL;
    model_msg.data = (uint8 *)&on_off_switch;
    divoom_sys_model_crl_msg_push(&model_msg);

	//divoom_screen_bright(0);
	//divoom_disp_ambient_ctrl_set_rgb_on_off(0);
}

void divoom_lvgl_power_off_disp_menu_open_screen(void)
{
	int on_off_switch = 1;
	divoom_sys_msg_info model_msg = {0};

	//model_msg.src_id = DIVOOM_MODEL_MAIN; 
	model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
    model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_SCREEN_CTRL;
    model_msg.data = (uint8 *)&on_off_switch;
    divoom_sys_model_crl_msg_push(&model_msg);

	//divoom_screen_bright(divoom_main_get_sys_light());
	//divoom_disp_ambient_ctrl_set_rgb_on_off(1);
}


void divoom_lvgl_power_off_disp_menu_event_cb(lv_event_t * e)
{
	Pdivoom_lvgl_power_off_disp_menu_ctrl phandle = (Pdivoom_lvgl_power_off_disp_menu_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *cur_obj = lv_event_get_target(e);
	
	if(code == LV_EVENT_KEY)
    {
		//divoom_app_schedule_item_info push_info = {0};
		
        //push_info.event_id = DIVOOM_MAIN_PLAY_ENTER_POWER_OFF_DISP_MENU_EVENT;
        //push_info.model_id = DIVOOM_MODEL_MAIN;
        //push_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
       	//divoom_app_schedule_cancel(&push_info);

#if 0
		divoom_lvgl_power_off_disp_menu_open_screen();
#endif
		divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_POWER_OFF_DISP_MENU_CANCLE);
		
		//divoom_main_set_on_off_work_status(2);
	}
	
}

void *divoom_lvgl_power_off_disp_menu_init(void *user_data, int menu_id)
{
	Pdivoom_lvgl_power_off_disp_menu_ctrl phandle = (Pdivoom_lvgl_power_off_disp_menu_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_power_off_disp_menu_ctrl));

	phandle->main_obj = lv_btn_create(NULL);

	lv_obj_remove_style_all(phandle->main_obj);
	lv_obj_set_style_bg_opa(phandle->main_obj, LV_OPA_100, 0);
	lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(0x000000), 0);
	lv_obj_set_style_pad_all(phandle->main_obj, 0, 0);
	lv_obj_set_style_border_width(phandle->main_obj, 0, 0);
	lv_obj_set_style_shadow_width(phandle->main_obj, 0, 0);
	lv_obj_set_style_outline_width(phandle->main_obj, 0, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(phandle->main_obj, 0, 0);
	lv_obj_add_event_cb(phandle->main_obj, divoom_lvgl_power_off_disp_menu_event_cb, LV_EVENT_KEY, phandle); 

	divoom_lvgl_power_off_disp_menu_close_screen();
	
	return phandle;
}

void divoom_lvgl_power_off_disp_menu_disp(void *handle, void *active)
{
	Pdivoom_lvgl_power_off_disp_menu_ctrl phandle = (Pdivoom_lvgl_power_off_disp_menu_ctrl)handle;

	if(phandle->main_obj)
	{
		lv_group_focus_obj(phandle->main_obj);
	}
	else if(active)
	{
		lv_group_focus_obj(active);
	}
	
	lv_scr_load(phandle->main_obj);
}


void divoom_lvgl_power_off_disp_menu_exit(void *handle)
{
	Pdivoom_lvgl_power_off_disp_menu_ctrl phandle = (Pdivoom_lvgl_power_off_disp_menu_ctrl)handle;

	
	lv_obj_del(phandle->main_obj);
	divoom_system_free(phandle);
}


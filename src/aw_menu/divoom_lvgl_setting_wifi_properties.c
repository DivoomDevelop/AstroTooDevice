#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#ifdef DIVOOM_USE_RK_IC
#include <Rk_wifi.h>
#else
#include  "wifimg.h"
#endif

typedef struct _divoom_lvgl_setting_wifi_ctrl
{
    int menu_id;
	void *main_obj;

	lv_style_t btn_normal_style;
	lv_style_t btn_select_style;

	void *pdirect_handle;
	void *cur_active_obj;
  
}divoom_lvgl_setting_wifi_properties_ctrl, *Pdivoom_lvgl_setting_wifi_properties_ctrl;

void divoom_lvgl_setting_wifi_properties_menu_event_cb(lv_event_t * e)
{
	lv_obj_t *cur_obj;
    Pdivoom_lvgl_setting_wifi_properties_ctrl phandle = (Pdivoom_lvgl_setting_wifi_properties_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);

	if(code == LV_EVENT_KEY)
	{
		char c = *((char *)lv_event_get_param(e));

		if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			lv_group_focus_obj(divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_obj, DIVOOM_MENU_DIRECT_DOWN));
		}
		else if(c == DIVOOM_LVGL_KEY_PRE)
		{
			lv_group_focus_obj(divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_obj, DIVOOM_MENU_DIRECT_UP));
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_menu_control_return_pre();
		}
		else
		{
			return;
		}
	}
}

void *divoom_lvgl_setting_wifi_properties_init(void *user_data, int menu_id)
{
	int i;
	Pdivoom_lvgl_setting_wifi_properties_ctrl phandle = divoom_system_malloc(sizeof(divoom_lvgl_setting_wifi_properties_ctrl));

#ifdef DIVOOM_USE_RK_IC
		RK_WIFI_INFO_Connection_s conn_info;
		RK_wifi_running_getConnectionInfo(&conn_info);
#else
		wifi_sta_info_t wifi_info;
		wifi_sta_get_info(&wifi_info);
#endif
    phandle->menu_id = menu_id;
	phandle->cur_active_obj = NULL;
	phandle->pdirect_handle = divoom_menu_direct_ctrl_create(1, 4);
	//divoom_menu_control_init_btn_stype(&phandle->btn_select_style, NULL, &phandle->btn_normal_style, NULL, NULL, NULL, 17);

	lv_style_init(&phandle->btn_normal_style);
	lv_style_set_bg_color(&phandle->btn_normal_style, lv_color_hex(0x181818));
	lv_style_set_pad_all(&phandle->btn_normal_style, 0);
	lv_style_set_border_width(&phandle->btn_normal_style, 4);
	lv_style_set_border_color(&phandle->btn_normal_style, lv_color_black());
	lv_style_set_radius(&phandle->btn_normal_style, 17);
	lv_style_set_shadow_width(&phandle->btn_normal_style, 0);
	lv_style_set_outline_width(&phandle->btn_normal_style, 0);

	lv_style_init(&phandle->btn_select_style);
	lv_style_set_bg_color(&phandle->btn_select_style, lv_color_hex(0x181818));
	lv_style_set_pad_all(&phandle->btn_select_style, 0);
	lv_style_set_border_width(&phandle->btn_select_style, 4);
	lv_style_set_border_color(&phandle->btn_select_style, lv_color_hex(0xBBBBBB));
	lv_style_set_radius(&phandle->btn_select_style, 17);
	lv_style_set_shadow_width(&phandle->btn_select_style, 0);
	lv_style_set_outline_width(&phandle->btn_select_style, 0);
	
	phandle->main_obj = lv_obj_create(NULL);
	lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(0), LV_PART_MAIN);
    lv_obj_set_pos(phandle->main_obj, 0, 0);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH); 
	
	lv_obj_t *title = lv_label_create(phandle->main_obj);
	lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_obj_set_size(title, 300, 40);
	lv_obj_set_pos(title, 333, 69);
	lv_label_set_text(title, divoom_menu_control_get_disp_str("NetworkProperties"));

	for(i = 0;i < 4; i++)
	{
		lv_obj_t *btn_obj  = lv_btn_create(phandle->main_obj);
        lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
        lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
        lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
		lv_obj_set_style_pad_all(btn_obj, 0, 0);
        lv_obj_set_pos(btn_obj, 44, 164 + 98 * i);
    	lv_obj_set_size(btn_obj, 712, 90);
		
        lv_obj_t *label_obj = lv_label_create(btn_obj);
        lv_obj_set_pos(label_obj, 32, 28);
    	lv_obj_set_size(label_obj, 500, 34);
        lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, phandle->menu_id), 0); 

		lv_obj_t *properties_label_obj = lv_label_create(btn_obj);
        if(i == 0)
		{
			lv_obj_align(properties_label_obj, LV_ALIGN_TOP_RIGHT, -32, 28);
        }
		else
		{
			lv_obj_set_pos(properties_label_obj, 356, 28);
    		lv_obj_set_size(properties_label_obj, 324, 34);
        }
        lv_obj_set_style_text_color(properties_label_obj, lv_color_hex(0x9E9E9E), 0);
        lv_obj_set_style_text_align(properties_label_obj, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_style_text_font(properties_label_obj, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
		
        if(i == 0)
        {
        	phandle->cur_active_obj = btn_obj;
        	lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("CurrentWifi"));
#ifdef DIVOOM_USE_RK_IC
			lv_label_set_text_fmt(properties_label_obj, "%s", conn_info.ssid);
#else
			lv_label_set_text_fmt(properties_label_obj, "%s", wifi_info.ssid);
#endif
        	lv_obj_t *image_obj = lv_img_create(btn_obj);
    		lv_obj_align_to(image_obj, properties_label_obj, LV_ALIGN_TOP_LEFT, -45, -5);
        	lv_obj_set_size(image_obj, 36, 36);
            if(wifi_info.rssi > -60)
            {
        	    divoom_menu_control_lv_img_set_src(image_obj, "73/73_wifi_level_3.png");
            }
            else if(wifi_info.rssi > -70)
            {
        	    divoom_menu_control_lv_img_set_src(image_obj, "73/73_wifi_level_2.png");
            }
            else
            {
        	    divoom_menu_control_lv_img_set_src(image_obj, "73/73_wifi_level_1.png");
            }
        }
		else if(i == 1)
		{
			lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("ConnectionStatus"));
			lv_label_set_text(properties_label_obj, divoom_menu_control_get_disp_str("Connected"));
		}
		else if(i == 2)
		{
			lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("IP Address"));
#ifdef DIVOOM_USE_RK_IC
			lv_label_set_text_fmt(properties_label_obj, "%s", conn_info.ip_address);
#else
			lv_label_set_text_fmt(properties_label_obj, "%d.%d.%d.%d", wifi_info.ip_addr[0], wifi_info.ip_addr[1], wifi_info.ip_addr[2], wifi_info.ip_addr[3]);
#endif
		}
		else
		{
			lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("Mac Address"));
#ifdef DIVOOM_USE_RK_IC
			lv_label_set_text_fmt(properties_label_obj, "%s", conn_info.mac_address);
#else
			lv_label_set_text_fmt(properties_label_obj, "%02x:%02x:%02x:%02x:%02x:%02x", wifi_info.mac_addr[0], wifi_info.mac_addr[1], wifi_info.mac_addr[2], wifi_info.mac_addr[3], wifi_info.mac_addr[4], wifi_info.mac_addr[5]);
#endif
		}
        
        lv_obj_add_event_cb(btn_obj, divoom_lvgl_setting_wifi_properties_menu_event_cb, LV_EVENT_KEY, phandle);
        divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, i);
	}
	
	return phandle;
}

void divoom_lvgl_setting_wifi_properties_disp(void *handle, void *active_obj)
{
	Pdivoom_lvgl_setting_wifi_properties_ctrl phandle = (Pdivoom_lvgl_setting_wifi_properties_ctrl)handle;

	if(phandle->cur_active_obj)
	{
		lv_group_focus_obj(phandle->cur_active_obj);
		phandle->cur_active_obj = NULL;
	}
	if(active_obj)
	{
		lv_group_focus_obj(active_obj);
	}
	lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_setting_wifi_properties_exit(void *handle)
{
	Pdivoom_lvgl_setting_wifi_properties_ctrl phandle = (Pdivoom_lvgl_setting_wifi_properties_ctrl)handle;

	lv_style_reset(&phandle->btn_select_style);
	lv_style_reset(&phandle->btn_normal_style);
	divoom_menu_direct_ctrl_destory(phandle->pdirect_handle);
	lv_obj_del(phandle->main_obj);
	divoom_system_free(phandle);
}



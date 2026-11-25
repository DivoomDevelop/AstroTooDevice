#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"

extern uint8 divoom_ble_is_run(void);

enum{
    DIVOOM_LVGL_CONFIG_WIFI_STATUS_WAIT_BLE_CONNECT,    //等待BLE命令
    DIVOOM_LVGL_CONFIG_WIFI_STATUS_WAIT_WIFI_CONNECT,   //接受到BLE命令，等待wifi连接
    DIVOOM_LVGL_CONFIG_WIFI_STATUS_WIFI_CONNECTING,     //连接上WIFI,等待分配IP
    DIVOOM_LVGL_CONFIG_WIFI_STATUS_WIFI_CONNECT_OK,     //连接上服务器
};
    
#define DIVOOM_LVGL_CONFIG_WIFI_FILE_NAME "/usr/share/divoom_app/ui/53/wifi_video.mp4"

enum{
	DIVOOM_LVGL_SELECT_BOX_BTN_CANCEL,	//取消
	DIVOOM_LVGL_SELECT_BOX_BTN_OK,		//确定
};

typedef struct _divoom_lvgl_config_wifi_ctrl
{
    int ble_run_flag;
    int menu_id;
    int   idle_time;
    int   count_down_num;
    uint8 wifi_status;
    uint8 wifi_disp_id;
	int run_flag;
	uint8 mac[6];
    lv_style_t  normal_style;
    lv_style_t  btn_normal_style;
    lv_style_t  btn_active_style;
    void *time_handle;
    void *main_obj;
    void *wifi_image_obj;
    void *app_image_obj;
    void *count_down_label_obj;
    void *count_down_image_obj;
    void *pass_obj;
    void *title_obj;
    void *desc1_obj;
    void *desc2_obj;
	void *desc3_obj;

	void *bg_cover_obj;
	void *reset_restore_box_obj;
	void *box_btn[2];
	void *idle_time_handle;

#ifdef DIVOOM_TIME_FRAME_SPEAKER
#define WIFI_AUDIO_HINT_AUTO_MS   3000
#define WIFI_AUDIO_HINT_PILL_X    42
#define WIFI_AUDIO_HINT_PILL_Y    18
#define WIFI_AUDIO_HINT_PILL_W    720
#define WIFI_AUDIO_HINT_PILL_H    100
#define WIFI_AUDIO_HINT_ICON_PAD_L  14
#define WIFI_AUDIO_HINT_ICON_SZ     54
#define WIFI_AUDIO_HINT_TEXT_PAD_L  88
#define WIFI_AUDIO_HINT_TEXT_W      560

	void *audio_hint_cont;
	lv_style_t audio_hint_pill_style;
	void *audio_hide_timer;
	int audio_btn_flag;
	uint8 audio_hint_timer_started;
#endif
}divoom_lvgl_config_wifi_ctrl, *Pdivoom_lvgl_config_wifi_ctrl;

int divoom_ble_get_wifi_status(void);

#define WIFI_CONFIG_MAC_LABEL_H           28
#define WIFI_CONFIG_MAC_LABEL_BOTTOM_PAD  24

static void divoom_lvgl_config_wifi_update_mac_label(Pdivoom_lvgl_config_wifi_ctrl phandle)
{
	if (!phandle || !phandle->desc3_obj)
		return;

	divoom_main_get_device_mac_addr_ifconfig(phandle->mac, NULL);
	lv_label_set_text_fmt(phandle->desc3_obj, "%s:%02x:%02x:%02x:%02x:%02x:%02x",
		divoom_menu_control_get_disp_str("Mac Address"),
		phandle->mac[0], phandle->mac[1], phandle->mac[2],
		phandle->mac[3], phandle->mac[4], phandle->mac[5]);
	lv_obj_set_width(phandle->desc3_obj, 723);
	lv_obj_set_height(phandle->desc3_obj, WIFI_CONFIG_MAC_LABEL_H);
	lv_obj_align(phandle->desc3_obj, LV_ALIGN_BOTTOM_MID, 0, -WIFI_CONFIG_MAC_LABEL_BOTTOM_PAD);
	lv_obj_set_style_text_color(phandle->desc3_obj, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_align(phandle->desc3_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(phandle->desc3_obj, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
	lv_obj_clear_flag(phandle->desc3_obj, LV_OBJ_FLAG_HIDDEN);
}

static void divoom_lvgl_config_wifi_box_menu_event_cb(lv_event_t * e)
{
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_config_wifi_ctrl phandle = (Pdivoom_lvgl_config_wifi_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
	char *key_info = (char *)lv_event_get_param(e);

	if(code == LV_EVENT_KEY)
	{
		if(key_info[0] == DIVOOM_LVGL_KEY_PRE || key_info[0] == DIVOOM_LVGL_KEY_NEXT)
		{
			if(cur_obj == phandle->box_btn[DIVOOM_LVGL_SELECT_BOX_BTN_CANCEL])
			{
				lv_group_focus_obj(phandle->box_btn[DIVOOM_LVGL_SELECT_BOX_BTN_OK]);
			}
			else
			{
				lv_group_focus_obj(phandle->box_btn[DIVOOM_LVGL_SELECT_BOX_BTN_CANCEL]);
			}
		}
		else if(key_info[0] == DIVOOM_LVGL_KEY_ESC)
		{
			lv_obj_add_flag(phandle->reset_restore_box_obj, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);
			lv_group_focus_obj(phandle->pass_obj);
		}
		else if(key_info[0] == DIVOOM_LVGL_KEY_ENTER)
		{
			if(cur_obj == phandle->box_btn[DIVOOM_LVGL_SELECT_BOX_BTN_CANCEL])//取消
			{
				
			}
			else//确定
			{
				divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_RESET_FACTORY_SETTING);
			}
			lv_obj_add_flag(phandle->reset_restore_box_obj, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);
			lv_group_focus_obj(phandle->pass_obj);
		}
	}
}

#ifdef DIVOOM_TIME_FRAME_SPEAKER
static void divoom_lvgl_config_wifi_audio_hint_hide(Pdivoom_lvgl_config_wifi_ctrl phandle)
{
	if (!phandle)
		return;

	if (phandle->audio_hide_timer)
	{
		lv_timer_del(phandle->audio_hide_timer);
		phandle->audio_hide_timer = NULL;
	}
	phandle->audio_hint_timer_started = 0;
	phandle->audio_btn_flag = 1;
	if (phandle->audio_hint_cont)
		lv_obj_add_flag(phandle->audio_hint_cont, LV_OBJ_FLAG_HIDDEN);
}

static void divoom_lvgl_config_wifi_audio_hint_timer_cb(struct _lv_timer_t *timer)
{
	Pdivoom_lvgl_config_wifi_ctrl phandle = (Pdivoom_lvgl_config_wifi_ctrl)lv_timer_get_user_data(timer);

	divoom_lvgl_config_wifi_audio_hint_hide(phandle);
}

static void divoom_lvgl_config_wifi_audio_hint_show(Pdivoom_lvgl_config_wifi_ctrl phandle)
{
	if (!phandle || phandle->audio_btn_flag || !phandle->audio_hint_cont)
		return;

	lv_obj_clear_flag(phandle->audio_hint_cont, LV_OBJ_FLAG_HIDDEN);
	lv_obj_move_foreground(phandle->audio_hint_cont);
	if (!phandle->audio_hint_timer_started)
	{
		phandle->audio_hide_timer = lv_timer_create(divoom_lvgl_config_wifi_audio_hint_timer_cb,
			WIFI_AUDIO_HINT_AUTO_MS, phandle);
		if (phandle->audio_hide_timer)
			lv_timer_set_repeat_count(phandle->audio_hide_timer, 1);
		phandle->audio_hint_timer_started = 1;
	}
}
#endif

void divoom_lvgl_config_wifi_menu_check_time_handle(struct _lv_timer_t *user_data)
{
    Pdivoom_lvgl_config_wifi_ctrl phandle = (Pdivoom_lvgl_config_wifi_ctrl)lv_timer_get_user_data(user_data);

#if 0
    divoom_print("idle time: %d, %d\n", phandle->idle_time, divoom_system_get_tickms());
    phandle->idle_time ++;

    if(phandle->idle_time == 10)
    {
        divoom_main_enter_stanby_mode();
        divoom_port_delay_ms((1) * 1000U);
        exit(0);
    }
	#endif
    
}

static void divoom_lvgl_config_wifi_menu_event_cb(lv_event_t * e)
{
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_config_wifi_ctrl phandle = (Pdivoom_lvgl_config_wifi_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
	char *key_info = (char *)lv_event_get_param(e);

	if(code == LV_EVENT_KEY && key_info[0] == DIVOOM_LVGL_KEY_HOME && key_info[1] == DIVOOM_PRESS_LONG_LONG)
	{
		lv_obj_clear_flag(phandle->reset_restore_box_obj, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);
		lv_group_focus_obj(phandle->box_btn[DIVOOM_LVGL_SELECT_BOX_BTN_CANCEL]);
	}
    else if(code == LV_EVENT_KEY)
    {
        char *key_code = ((char *)lv_event_get_param(e));

        if(key_code[0] == DIVOOM_LVGL_KEY_ESC)
        {
            if(phandle->wifi_status == DIVOOM_LVGL_CONFIG_WIFI_STATUS_WIFI_CONNECT_OK)
            {
                Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();
                
                if(phandle->pass_obj == cur_obj)
                {
                    divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_CONFIG_NET_WORK_OVER);
                    divoom_main_set_wifi_config_status(1);
                    return;
                }
            }
            else
            {
                divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_CONFIG_NET_WORK_CANCLE);
                divoom_menu_control_return_pre();
            }
        }
    }
}


void divoom_lvgl_config_wifi_time_loop(struct _lv_timer_t *ptime_info)
{
    int cur_wifi_status;
    Pdivoom_lvgl_config_wifi_ctrl phandle = (Pdivoom_lvgl_config_wifi_ctrl)lv_timer_get_user_data(ptime_info);

    /*
    
	DIVOOM_WIFI_CONNECTING = 0,     //连接中
	DIVOOM_WIFI_NO_SSID    = 1,        //ssid错误
	DIVOOM_WIFI_OTHER_ERR  = 2,      //密码错误或者其他错误
	DIVOOM_WIFI_CONNECTED  = 3,      //连接成功
	DIVOOM_WIFI_NOT_CONFIG = 4,     //没有配置过WiFi
	DIVOOM_WIFI_CONNECTED_SER = 5,     //已经连上服务器
	DIVOOM_WIFI_NO_FIRST_USER = 6,		//不是第一次配网绑定的用户
	DIVOOM_WIFI_BLE_WILL_EXIT = 7,		//BLE的内存已经释放了
	DIVOOM_WIFI_DISCONNECTED,			//wifi断开,有可能是没连上报的断开,也有可能是连接上之后断开了
	DIVOOM_WIFI_RECONFIG,			//wifi重新配置
	DIVOOM_WIFI_START,     //wifi打开
	*/
	if(phandle->ble_run_flag == 0)
	{
        phandle->ble_run_flag = divoom_ble_is_run();	
	}
    if(phandle->ble_run_flag)
    {
        cur_wifi_status = divoom_ble_get_wifi_status();
    }
    else
    {
        cur_wifi_status = DIVOOM_WIFI_NOT_CONFIG;
    }
	
    divoom_print("wifi status: %d\n", cur_wifi_status);
    if(cur_wifi_status == DIVOOM_WIFI_NOT_CONFIG || cur_wifi_status == DIVOOM_WIFI_NO_SSID || cur_wifi_status == DIVOOM_WIFI_OTHER_ERR)	//40%
    {
        cur_wifi_status = DIVOOM_LVGL_CONFIG_WIFI_STATUS_WAIT_BLE_CONNECT;
    }
    else if(cur_wifi_status == DIVOOM_WIFI_CONNECTING)	//60%
    {
        cur_wifi_status = DIVOOM_LVGL_CONFIG_WIFI_STATUS_WAIT_WIFI_CONNECT;
    }
    else if(cur_wifi_status == DIVOOM_WIFI_CONNECTED)	//80%
    {
        cur_wifi_status = DIVOOM_LVGL_CONFIG_WIFI_STATUS_WIFI_CONNECTING;
    }
    else if(cur_wifi_status == DIVOOM_WIFI_CONNECTED_SER)
    {
        cur_wifi_status = DIVOOM_LVGL_CONFIG_WIFI_STATUS_WIFI_CONNECT_OK;
    }
    else
    {
        cur_wifi_status = phandle->wifi_status;
    }

    if(cur_wifi_status != phandle->wifi_status)
    {
        phandle->wifi_status = cur_wifi_status;
        switch(cur_wifi_status)
        {
            case DIVOOM_LVGL_CONFIG_WIFI_STATUS_WAIT_BLE_CONNECT:    //等待BLE命令
                lv_label_set_text(phandle->title_obj, divoom_menu_control_get_disp_str("WaitConfigWifi1"));
                lv_label_set_text(phandle->desc1_obj, divoom_menu_control_get_disp_str("WaitWifiConnecting1"));
                lv_label_set_text(phandle->desc2_obj, divoom_menu_control_get_disp_str("DownloadApp"));
                lv_obj_set_pos(phandle->desc2_obj, 302, 1109);
                lv_obj_set_size(phandle->desc2_obj, 476, 90);
                lv_obj_set_style_text_font(phandle->desc2_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
				
				divoom_lvgl_config_wifi_update_mac_label(phandle);
                break;
                
            case DIVOOM_LVGL_CONFIG_WIFI_STATUS_WAIT_WIFI_CONNECT:   //接受到BLE命令，等待wifi连接
                lv_obj_add_flag(phandle->app_image_obj, LV_OBJ_FLAG_HIDDEN);
                lv_label_set_text(phandle->title_obj, divoom_menu_control_get_disp_str("WaitConfigWifi2"));
                lv_label_set_text(phandle->desc1_obj, divoom_menu_control_get_disp_str("WaitWifiConnecting2"));

				lv_obj_add_flag(phandle->desc2_obj, LV_OBJ_FLAG_HIDDEN);
				divoom_lvgl_config_wifi_update_mac_label(phandle);
                break;
                
            case DIVOOM_LVGL_CONFIG_WIFI_STATUS_WIFI_CONNECTING:     //连接上WIFI,等待分配IP
                lv_label_set_text(phandle->title_obj, divoom_menu_control_get_disp_str("WaitConfigWifi3"));
                lv_label_set_text(phandle->desc1_obj, divoom_menu_control_get_disp_str("WaitWifiConnecting3"));
			
                //lv_label_set_text(phandle->desc2_obj, divoom_menu_control_get_disp_str("WifiCOnfigNotice"));
                lv_obj_add_flag(phandle->desc2_obj, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(phandle->app_image_obj, LV_OBJ_FLAG_HIDDEN);
				divoom_lvgl_config_wifi_update_mac_label(phandle);
                break;
                
            case DIVOOM_LVGL_CONFIG_WIFI_STATUS_WIFI_CONNECT_OK:     //连接上服务器
                lv_label_set_text(phandle->title_obj, divoom_menu_control_get_disp_str("WaitConfigWifi4"));
                lv_label_set_text(phandle->desc1_obj, divoom_menu_control_get_disp_str("WaitWifiConnecting4"));
                lv_obj_add_flag(phandle->app_image_obj, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(phandle->desc2_obj, LV_OBJ_FLAG_HIDDEN);
				divoom_lvgl_config_wifi_update_mac_label(phandle);
                //lv_obj_clear_flag(phandle->pass_obj, LV_OBJ_FLAG_HIDDEN);
                //lv_obj_clear_flag(phandle->count_down_image_obj, LV_OBJ_FLAG_HIDDEN);
                //lv_obj_clear_flag(phandle->count_down_label_obj, LV_OBJ_FLAG_HIDDEN);
                phandle->count_down_num = 2;
                
                break;
        }
    }

	divoom_lvgl_config_wifi_update_mac_label(phandle);
    
    if(phandle->wifi_status != DIVOOM_LVGL_CONFIG_WIFI_STATUS_WIFI_CONNECT_OK)
    {
        if(phandle->wifi_disp_id == 3)
        {
            divoom_menu_control_lv_img_set_src(phandle->wifi_image_obj, "53/53_wifi3.png");
        }
        else if(phandle->wifi_disp_id == 2)
        {
            divoom_menu_control_lv_img_set_src(phandle->wifi_image_obj, "53/53_wifi2.png");
        }
        else if(phandle->wifi_disp_id == 1)
        {
            divoom_menu_control_lv_img_set_src(phandle->wifi_image_obj, "53/53_wifi1.png");
        }
        else
        {
            divoom_menu_control_lv_img_set_src(phandle->wifi_image_obj, "53/53_wifi0.png");
        }
        phandle->wifi_disp_id = (phandle->wifi_disp_id + 1) % 4;
    }
    else
    {
        //lv_label_set_text_fmt(phandle->count_down_label_obj, "%d",phandle->count_down_num/2);
#ifdef DIVOOM_TIME_FRAME_SPEAKER
        if(phandle->audio_btn_flag == 0)
		{
			divoom_lvgl_config_wifi_audio_hint_show(phandle);
		}
		else
		{
			if(phandle->count_down_num == 0)
        	{
            	divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_CONFIG_NET_WORK_OVER);
            	divoom_main_set_wifi_config_status(1);
        	}
        	else
       		{
            	phandle->count_down_num --;
        	}
		}
#else
		if(phandle->count_down_num == 0)
        {
            divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_CONFIG_NET_WORK_OVER);
            divoom_main_set_wifi_config_status(1);
        }
        else
        {
            phandle->count_down_num --;
        }
#endif
    }
}

void *divoom_lvgl_config_wifi_task_main(void *handle)
{
	int ret;
	Pdivoom_lvgl_config_wifi_ctrl phandle = (Pdivoom_lvgl_config_wifi_ctrl)handle;

	while(phandle && phandle->run_flag)
	{
		ret = divoom_main_get_device_mac_addr(phandle->mac);

		if(ret > 0)
		{
			phandle->run_flag = 0;
		}

		divoom_system_delay_ms(1000);
	}

	return NULL;
}

void *divoom_lvgl_config_wifi_init(void *user_data, int menu_id)
{
	int config_flag = 1;
    void *image_obj, *label_obj;
    Pdivoom_lvgl_config_wifi_ctrl phandle;

	if(user_data != NULL)
	{
		config_flag = *(int *)user_data;
		divoom_system_free(user_data);
	}
	divoom_wifi_set_config(config_flag);

    phandle = divoom_system_malloc(sizeof(divoom_lvgl_config_wifi_ctrl));
    phandle->menu_id = menu_id;
	phandle->run_flag = 1;
#ifdef DIVOOM_TIME_FRAME_SPEAKER
    phandle->audio_btn_flag = 0;
    phandle->audio_hint_timer_started = 0;
    phandle->audio_hide_timer = NULL;
    phandle->audio_hint_cont = NULL;
#endif
    phandle->main_obj = lv_obj_create(NULL);  
    divoom_menu_control_init_lable_style(&phandle->normal_style);
    lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(0x0), LV_PART_MAIN);
    lv_obj_set_pos(phandle->main_obj, 0, 0);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);  

    lv_obj_add_style(phandle->main_obj, &phandle->normal_style, 0);
    lv_obj_set_scrollbar_mode(phandle->main_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(phandle->main_obj, LV_DIR_VER);

	//image_obj = lv_img_create(phandle->main_obj);
	//image_obj = lv_obj_create(phandle->main_obj);
	//lv_obj_set_style_bg_img_src(image_obj, "A:/usr/share/divoom_app/ui/53/53_bg_cover.png", 0);
	//lv_obj_set_pos(image_obj, 0, 0);
    //lv_obj_set_size(image_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    //divoom_menu_control_lv_img_set_src(image_obj, "53/53_bg_cover.png");
	//lv_obj_add_flag(image_obj, LV_OBJ_FLAG_HIDDEN);
	//phandle->wifi_image_obj = image_obj;
	
    //image_obj = lv_img_create(phandle->main_obj);
	//lv_obj_set_pos(image_obj, 40, 277);
    //lv_obj_set_size(image_obj, 720, 720);
    //divoom_menu_control_lv_img_set_src(image_obj, "53/53_wifi_bg.png");

    image_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(image_obj, 0, 0);
    //lv_obj_set_size(image_obj, 316, 316);
    divoom_menu_control_lv_img_set_src(image_obj, "53/53_wifi0.png");
    phandle->wifi_image_obj = image_obj;
    phandle->wifi_disp_id = 1;
    
    image_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(image_obj, 120, 1068);
    lv_obj_set_size(image_obj, 141, 141);
    divoom_menu_control_lv_img_set_src(image_obj, "53/53_app.png");
    phandle->app_image_obj = image_obj;
    
    image_obj = lv_btn_create(phandle->main_obj);

    divoom_menu_control_init_btn_stype(&phandle->btn_normal_style, NULL, NULL, NULL, &phandle->btn_active_style, NULL, 59);
    lv_obj_add_style(image_obj, &phandle->btn_normal_style, 0);
    lv_obj_add_style(image_obj, &phandle->btn_active_style, LV_STATE_FOCUSED);
    lv_obj_add_style(image_obj, &phandle->btn_active_style, LV_STATE_FOCUS_KEY);
    
	lv_obj_set_pos(image_obj, 40, 1075);
    lv_obj_set_size(image_obj, 726, 128);
    phandle->pass_obj = image_obj;
    lv_obj_add_event_cb(phandle->pass_obj, divoom_lvgl_config_wifi_menu_event_cb, LV_EVENT_KEY, phandle);
    lv_obj_add_flag(phandle->pass_obj, LV_OBJ_FLAG_HIDDEN);
    
    
    label_obj = lv_label_create(image_obj);
    lv_obj_set_pos(label_obj, 60, 36);
	lv_obj_set_size(label_obj, 600, 48);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("Pass"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(50, phandle->menu_id), 0); 
    
    
    image_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(image_obj, 686, 24);
    lv_obj_set_size(image_obj, 90, 90);
    divoom_menu_control_lv_img_set_src(image_obj, "53/53_count_dwon.png");
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_HIDDEN);
    phandle->count_down_image_obj = image_obj;

    label_obj = lv_label_create(image_obj);
    lv_label_set_text(label_obj, "3");
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(46, phandle->menu_id), 0); 
    lv_obj_center(label_obj);
    phandle->count_down_label_obj = label_obj;
    lv_obj_add_flag(label_obj, LV_OBJ_FLAG_HIDDEN);
    
    label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 39, 100);
	lv_obj_set_size(label_obj, 723, 160);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("WaitConfigWifi1"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(54, phandle->menu_id), 0);  
    phandle->title_obj = label_obj;

    label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 100, 850);
	lv_obj_set_size(label_obj, 600, 165);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("WaitWifiConnecting1"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x333333), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(40, phandle->menu_id), 0);  
    phandle->desc1_obj = label_obj;

    label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 287, 1068);
	lv_obj_set_size(label_obj, 476, 160);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("DownloadApp"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);  
    phandle->desc2_obj = label_obj;

	label_obj = lv_label_create(phandle->main_obj);
    phandle->desc3_obj = label_obj;

    phandle->time_handle = lv_timer_create(divoom_lvgl_config_wifi_time_loop, 500, phandle);
    phandle->wifi_status = DIVOOM_LVGL_CONFIG_WIFI_STATUS_WAIT_BLE_CONNECT;
    divoom_lvgl_config_wifi_update_mac_label(phandle);

	image_obj = lv_img_create(phandle->main_obj);
	divoom_menu_control_lv_img_set_src(image_obj, "26/26_bg_cover.png");
	lv_obj_set_size(image_obj, 800, 1280);//2060);
	lv_obj_add_flag(image_obj, LV_OBJ_FLAG_HIDDEN);
	phandle->bg_cover_obj = image_obj;

	image_obj = lv_img_create(phandle->main_obj);
	divoom_menu_control_lv_img_set_src(image_obj, "26/26_select_box.png");
	lv_obj_set_pos(image_obj, 80, 320);
	lv_obj_add_flag(image_obj, LV_OBJ_FLAG_HIDDEN);
	phandle->reset_restore_box_obj = image_obj;

	label_obj = lv_label_create(image_obj);
    lv_obj_set_pos(label_obj, 0, 23);
	lv_obj_set_size(label_obj, 640, 44);
	lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("Factory Restore"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0); 
	
	label_obj = lv_label_create(image_obj);
	lv_obj_set_pos(label_obj, 20, 90+10);
	lv_obj_set_size(label_obj, 600-30, 180);
	lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("FactoryRestoreComfirm"));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
	
	//取消
	lv_obj_t *btn_obj = lv_btn_create(image_obj);
	lv_obj_set_size(btn_obj, 282, 90);
	lv_obj_set_pos(btn_obj, 24, 294);
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
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_config_wifi_box_menu_event_cb, LV_EVENT_KEY, phandle);
	phandle->box_btn[DIVOOM_LVGL_SELECT_BOX_BTN_CANCEL] = btn_obj;

	label_obj = lv_label_create(btn_obj);
	lv_obj_set_pos(label_obj, 0, 24);
	lv_obj_set_size(label_obj, 282, 42);
	lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("Cancel"));
	//lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	
	//确定
	btn_obj = lv_btn_create(image_obj);
	lv_obj_set_size(btn_obj, 282, 90);
	lv_obj_set_pos(btn_obj, 334, 294);
	lv_obj_set_style_radius(btn_obj, 99, 0);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_style_bg_color(btn_obj, lv_color_hex(0xF2F2F2), 0);
	lv_obj_set_style_shadow_width(btn_obj, 0, 0);
	lv_obj_set_style_border_width(btn_obj, 4, 0);
	lv_obj_set_style_outline_width(btn_obj, 0, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_border_color(btn_obj, lv_color_hex(0xF6BDBA), 0);
	lv_obj_set_style_border_color(btn_obj, lv_color_hex(0xFF4337), LV_STATE_FOCUSED);
	lv_obj_set_style_border_color(btn_obj, lv_color_hex(0xFF4337), LV_STATE_FOCUS_KEY);
	lv_obj_set_style_text_color(btn_obj, lv_color_hex(0xF6BDBA), 0);
	lv_obj_set_style_text_color(btn_obj, lv_color_hex(0xCE0C00), LV_STATE_FOCUSED);
	lv_obj_set_style_text_color(btn_obj, lv_color_hex(0xCE0C00), LV_STATE_FOCUS_KEY);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_config_wifi_box_menu_event_cb, LV_EVENT_KEY, phandle);
	phandle->box_btn[DIVOOM_LVGL_SELECT_BOX_BTN_OK] = btn_obj;

	label_obj = lv_label_create(btn_obj);
	lv_obj_set_pos(label_obj, 0, 24);
	lv_obj_set_size(label_obj, 282, 42);
	lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("Ok"));
	//lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);

#ifdef DIVOOM_TIME_FRAME_SPEAKER
	{
		lv_obj_t *pill_obj;
		lv_obj_t *icon_img;

		lv_style_init(&phandle->audio_hint_pill_style);
		lv_style_set_bg_color(&phandle->audio_hint_pill_style, lv_color_hex(0xE4E4E4));
		lv_style_set_bg_opa(&phandle->audio_hint_pill_style, LV_OPA_90);
		lv_style_set_border_color(&phandle->audio_hint_pill_style, lv_color_hex(0xFFFFFF));
		lv_style_set_border_opa(&phandle->audio_hint_pill_style, LV_OPA_60);
		lv_style_set_border_width(&phandle->audio_hint_pill_style, 1);
		lv_style_set_radius(&phandle->audio_hint_pill_style, WIFI_AUDIO_HINT_PILL_H / 2);
		lv_style_set_pad_all(&phandle->audio_hint_pill_style, 0);
		lv_style_set_shadow_width(&phandle->audio_hint_pill_style, 0);

		image_obj = lv_obj_create(phandle->main_obj);
		phandle->audio_hint_cont = image_obj;
		lv_obj_set_size(image_obj, DIVOOM_SCREEN_WIDTH, WIFI_AUDIO_HINT_PILL_Y + WIFI_AUDIO_HINT_PILL_H);
		lv_obj_set_pos(image_obj, 0, 0);
		lv_obj_set_style_bg_opa(image_obj, LV_OPA_TRANSP, 0);
		lv_obj_set_style_pad_all(image_obj, 0, 0);
		lv_obj_set_style_border_width(image_obj, 0, 0);
		lv_obj_clear_flag(image_obj, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_add_flag(image_obj, LV_OBJ_FLAG_HIDDEN);

		pill_obj = lv_obj_create(image_obj);
		lv_obj_remove_style_all(pill_obj);
		lv_obj_add_style(pill_obj, &phandle->audio_hint_pill_style, 0);
		lv_obj_set_size(pill_obj, WIFI_AUDIO_HINT_PILL_W, WIFI_AUDIO_HINT_PILL_H);
		lv_obj_set_pos(pill_obj, WIFI_AUDIO_HINT_PILL_X, WIFI_AUDIO_HINT_PILL_Y);
		lv_obj_set_style_shadow_width(pill_obj, 0, 0);
		lv_obj_set_style_clip_corner(pill_obj, true, 0);
		lv_obj_clear_flag(pill_obj, LV_OBJ_FLAG_SCROLLABLE);

		icon_img = lv_img_create(pill_obj);
		lv_img_set_size_mode(icon_img, LV_IMG_SIZE_MODE_REAL);
		lv_obj_set_size(icon_img, WIFI_AUDIO_HINT_ICON_SZ, WIFI_AUDIO_HINT_ICON_SZ);
		divoom_menu_control_lv_img_set_src(icon_img, "1/1_audio_speaker.png");
		lv_obj_align(icon_img, LV_ALIGN_LEFT_MID, WIFI_AUDIO_HINT_ICON_PAD_L, 0);

		label_obj = lv_label_create(pill_obj);
		lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("AudioConnectDesp"));
		lv_label_set_long_mode(label_obj, LV_LABEL_LONG_DOT);
		lv_obj_set_style_text_color(label_obj, lv_color_hex(0x000000), 0);
		lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
		lv_obj_set_width(label_obj, WIFI_AUDIO_HINT_TEXT_W);
		lv_obj_align(label_obj, LV_ALIGN_LEFT_MID, WIFI_AUDIO_HINT_TEXT_PAD_L, 0);
	}
#endif
    phandle->idle_time_handle = lv_timer_create(divoom_lvgl_config_wifi_menu_check_time_handle, 60*1000, phandle);

    return phandle;  
}


void divoom_lvgl_config_wifi_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_config_wifi_ctrl phandle = (Pdivoom_lvgl_config_wifi_ctrl)handle;

    lv_scr_load(phandle->main_obj);
    lv_group_focus_obj(phandle->pass_obj);
}

void divoom_lvgl_config_wifi_pause(void *handle)
{

}

void divoom_lvgl_config_wifi_exit(void *handle)
{
    Pdivoom_lvgl_config_wifi_ctrl phandle = (Pdivoom_lvgl_config_wifi_ctrl)handle;

	phandle->run_flag = 0;
    lv_timer_del(phandle->idle_time_handle);
    lv_timer_del(phandle->time_handle);
#ifdef DIVOOM_TIME_FRAME_SPEAKER
	if (phandle->audio_hide_timer)
	{
		lv_timer_del(phandle->audio_hide_timer);
		phandle->audio_hide_timer = NULL;
	}
#endif
    lv_obj_del(phandle->main_obj);
    lv_style_reset(&phandle->normal_style);
    lv_style_reset(&phandle->btn_normal_style);
    lv_style_reset(&phandle->btn_active_style);
#ifdef DIVOOM_TIME_FRAME_SPEAKER
	lv_style_reset(&phandle->audio_hint_pill_style);
#endif
    divoom_system_free(phandle);
	divoom_wifi_set_config(0);
}



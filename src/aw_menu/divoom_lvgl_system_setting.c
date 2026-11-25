#include "divoom_build_config.h"
#if DIVOOM_BUILD_SIMULATOR
#include <sys/statvfs.h>
#else
#include "divoom_rtos_statvfs_stub.h"
#endif
#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_bt_music.h"
#include <string.h>

extern int divoom_config_set_china_config(int flag);

#ifdef DIVOOM_TIME_FRAME_SPEAKER

#define SYS_SET_CONN_STATUS_LABEL_X   500
#define SYS_SET_CONN_STATUS_LABEL_W   180

enum{
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN,         //亮度
    //DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_MEMORY_BTN,  //音量记忆
    //DIVOOM_LVGL_SYSTEM_SETTING_MENU_THEME_APPEARANCE_BTN, //主题外观
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_SLIDE_SETTING_BTN,	//幻灯片设置
    //DIVOOM_LVGL_SYSTEM_SETTING_MENU_POWER_ON_OFF_TIME_BTN,      //定时开关机
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_NIGHTVIEW_BTN,      //夜览
	DIVOOM_LVGL_SYSTEM_SETTING_MENU_COLOR_TEMP_BTN,      //色温调节
	//DIVOOM_LVGL_SYSTEM_SETTING_MENU_MEDIA_VOLUME_BTN, 		//媒体音量
	//DIVOOM_LVGL_SYSTEM_SETTING_MENU_MEDIA_VOLUME_EFFECTS_BTN,		//音效
    //DIVOOM_LVGL_SYSTEM_SETTING_MENU_SCREEN_SAVE_BTN,      //屏保
    //DIVOOM_LVGL_SYSTEM_SETTING_MENU_IDLE_POWER_OFF_BTN,      //空闲关机时间
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN,         //系统提示音音量
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIFI_BTN,      //WIFI
	DIVOOM_LVGL_SYSTEM_SETTING_MENU_BLUETOOTH_BTN,      //蓝牙
	DIVOOM_LVGL_SYSTEM_SETTING_MENU_BATTERY_BTN,      //电池
    //DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIFI_PROPERTIES_BTN,  //网络属性
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_DATA_TIME_BTN,      //日期与时间
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIND_SPEED_BTN,      //风速单位
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_TEMPERATURE_BTN,      //温度
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_LANGUAGE_BTN,      //语言
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_MCP_TOOL_BTN,    //PC表盘创作&MCP
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_SHOW_GRID_BTN,     //显示像素作品网格
    //DIVOOM_LVGL_SYSTEM_SETTING_MENU_MIRROR_BTN,      //镜像
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_ABOUT_BTN,      //关于
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_RESET_ALL_BTN, //重置设置和网络
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_RESTORE_BTN,      //清除内置储存数据
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_END_BTN,      //
};

enum{
	DIVOOM_LVGL_SELECT_BOX_BTN_CANCEL,	//取消
	DIVOOM_LVGL_SELECT_BOX_BTN_OK,		//确定
};

typedef struct _divoom_lvgl_system_setting_ctrl
{
    int menu_id;
    int lang;
    void *main_obj;
    void *back_obj;
    void *func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_END_BTN];
    void *label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_END_BTN];
    void *image_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_END_BTN];
    void *lang_label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_END_BTN*2];
    lv_style_t  normal_style;
	lv_style_t  btn_normal_style;
	lv_style_t	btn_select_style;
	lv_style_t	btn_edit_style;
	void *first_active_obj;
	void *pdirect_handle;
	int btn_status;

	int select_id;
	void *select_focus_btn;
	void *bg_cover_obj;
	void *reset_restore_box_obj;
	void *restore_box_obj;
	void *box_btn[2];

	int wait_img_angle;
	int btn_wait_flag;
	void *waiting_obj;
    void *time_handle;

	int refresh_time;

	void *message_box_handle;
	Pdivoom_sys_setting_info psys_config;
}divoom_lvgl_system_setting_ctrl, *Pdivoom_lvgl_system_setting_ctrl;

typedef struct _divoom_lvgl_system_setting_menu_item
{
    int font_size;
    char *setting_str;
}divoom_lvgl_system_setting_menu_item, *Pdivoom_lvgl_system_setting_menu_item;

static const divoom_lvgl_system_setting_menu_item pdivoom_lvgl_setting_list[] =
{
	{32, "SystemSetting"},  
	{24, "Display"},
	{26, "SlideSetting"},
	//{26, "PowerOnOffTime"},
	{26, "NightViewMode"},
	{26, "ColorTemp"},
	//{24, "MediaAndEffects"},
	//{26, "AudioEffects"},
	//{26, "AudioEffect1"},
	{24, "VoiceNotice"},
	{26, "SystemVoiceNotice"},
	//{24, "Network"},
	//{26, "NetworkConnect"},
	{26, "Wi-Fi"},
	{26, "Bluetooth"},
	{26, "Battery"},
	{24, "Options"},
	{26, "DateTime"},
	{26, "WindSpeedUnit"},
	{26, "m/s"},
	{26, "Temperature"},
	{26, "Centigrade"},
	{26, "Language"},
	{26, "PcClockMcpTool"},
	{26, "ShowGrid"},
	{24, "SystemManage"},
	{26, "About"},
	{26, "Reset all network and settings"},
	{26, "Factory Restore"},
	{32, "Reset all network and settings"},
	{26, "ResetNetworkConfirm"},
	{32, "Cancel"},
	{32, "Ok"},
};

void divoom_lvgl_system_setting_set_battery_level_img(void *image_obj)
{
	if(divoom_is_in_charge())
	{
		if(divoom_power_get_level() == PWR_LEVEL_0)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_c1.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_1)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_c2.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_2)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_c3.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_3)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_c4.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_4)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_c5.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_5)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_c6.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_6)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_c7.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_7)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_c8.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_8)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_c9.png");
		}
		else
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_c10.png");
		}
	}
	else
	{
		if(divoom_power_get_level() == PWR_LEVEL_0)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_d1.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_1)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_d2.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_2)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_d3.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_3)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_d4.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_4)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_d5.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_5)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_d6.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_6)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_d7.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_7)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_d8.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_8)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_d9.png");
		}
		else
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_d10.png");
		}
	}
}

void divoom_lvgl_system_setting_create_wind_speed_setting_callback(void *handle, int ctrl_id)
{
    Pdivoom_lvgl_system_setting_ctrl phandle = (Pdivoom_lvgl_system_setting_ctrl)handle;
    
    if(ctrl_id < 0)
    {
    }
    else 
    {
        phandle->psys_config->wind_speed_flag = ctrl_id;
		if(phandle->psys_config->wind_speed_flag == 0)
		{
			lv_label_set_text(phandle->lang_label_obj_tbl[13], divoom_menu_control_get_disp_str("m/s"));
		}
		else if(phandle->psys_config->wind_speed_flag == 1)
		{
			lv_label_set_text(phandle->lang_label_obj_tbl[13], divoom_menu_control_get_disp_str("km/h"));
		}
		else if(phandle->psys_config->wind_speed_flag == 2)
		{
			lv_label_set_text(phandle->lang_label_obj_tbl[13], divoom_menu_control_get_disp_str("mph"));
		}
		else
		{
			lv_label_set_text(phandle->lang_label_obj_tbl[13], divoom_menu_control_get_disp_str("kt"));
		}
		divoom_main_send_sys_settings_config();
        divoom_main_save_config();
    }
    lv_group_focus_obj(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIND_SPEED_BTN]);
    divoom_lvgl_message_box_destroy(phandle->message_box_handle, 0);
    
    phandle->message_box_handle = NULL;
}


void divoom_lvgl_system_setting_create_wind_speed_setting(Pdivoom_lvgl_system_setting_ctrl phandle)
{
	if(phandle->message_box_handle != NULL)
	{
		return;
	}

    phandle->message_box_handle = divoom_lvgl_message_box_init(phandle->menu_id, phandle->main_obj, phandle, 
        divoom_lvgl_system_setting_create_wind_speed_setting_callback, 80, 415, 640, 504, NULL, lv_color_hex(0xF2F2F2), 4);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("m/s"), 26,lv_color_hex(0xF2F2F2), lv_color_hex(0),lv_color_hex(0), 0, 
        32, 32, 576, 98);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("km/h"), 26, lv_color_hex(0xF2F2F2),lv_color_hex(0),lv_color_hex(0), 1, 
        32, 146, 576, 98);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("mph"), 26,lv_color_hex(0xF2F2F2), lv_color_hex(0),lv_color_hex(0), 2, 
        32, 260, 576, 98);
	divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("kt"), 26,lv_color_hex(0xF2F2F2), lv_color_hex(0),lv_color_hex(0), 3, 
        32, 374, 576, 98);
    divoom_lvgl_message_box_set_active_item(phandle->message_box_handle, phandle->psys_config->wind_speed_flag);
}

void divoom_lvgl_system_setting_create_audio_effect_setting_callback(void *handle, int ctrl_id)
{
    Pdivoom_lvgl_system_setting_ctrl phandle = (Pdivoom_lvgl_system_setting_ctrl)handle;
    
    if(ctrl_id < 0)
    {
    }
    else 
    {
        phandle->psys_config->audio_effect = ctrl_id;
		if(phandle->psys_config->audio_effect == 0)
		{
			lv_label_set_text(phandle->lang_label_obj_tbl[7], divoom_menu_control_get_disp_str("AudioEffect1"));
		}
		else if(phandle->psys_config->audio_effect == 1)
		{
			lv_label_set_text(phandle->lang_label_obj_tbl[7], divoom_menu_control_get_disp_str("AudioEffect2"));
		}
		else if(phandle->psys_config->audio_effect == 2)
		{
			lv_label_set_text(phandle->lang_label_obj_tbl[7], divoom_menu_control_get_disp_str("AudioEffect3"));
		}
		else if(phandle->psys_config->audio_effect == 3)
		{
			lv_label_set_text(phandle->lang_label_obj_tbl[7], divoom_menu_control_get_disp_str("AudioEffect4"));
		}
		else if(phandle->psys_config->audio_effect == 4)
		{
			lv_label_set_text(phandle->lang_label_obj_tbl[7], divoom_menu_control_get_disp_str("AudioEffect5"));
		}
		else if(phandle->psys_config->audio_effect == 5)
		{
			lv_label_set_text(phandle->lang_label_obj_tbl[7], divoom_menu_control_get_disp_str("AudioEffect6"));
		}
		else if(phandle->psys_config->audio_effect == 6)
		{
			lv_label_set_text(phandle->lang_label_obj_tbl[7], divoom_menu_control_get_disp_str("AudioEffect7"));
		}
		else if(phandle->psys_config->audio_effect == 7)
		{
			lv_label_set_text(phandle->lang_label_obj_tbl[7], divoom_menu_control_get_disp_str("AudioEffect8"));
		}

		//divoom_main_send_sys_settings_config();
        divoom_main_save_config();
    }
    //lv_group_focus_obj(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_MEDIA_VOLUME_EFFECTS_BTN]);
    divoom_lvgl_message_box_destroy(phandle->message_box_handle, 0);
    
    phandle->message_box_handle = NULL;
}


void divoom_lvgl_system_setting_create_audio_effects_setting(Pdivoom_lvgl_system_setting_ctrl phandle)
{
	if(phandle->message_box_handle != NULL)
	{
		return;
	}

    phandle->message_box_handle = divoom_lvgl_message_box_init(phandle->menu_id, phandle->main_obj, phandle, 
        divoom_lvgl_system_setting_create_audio_effect_setting_callback, 80, 288, 640, 704, NULL, lv_color_hex(0xF2F2F2), 8);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("AudioEffect1"), 26,lv_color_hex(0xF2F2F2), lv_color_hex(0),lv_color_hex(0), 0, 
        32, 32, 576, 80);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("AudioEffect2"), 26, lv_color_hex(0xF2F2F2),lv_color_hex(0),lv_color_hex(0), 1, 
        32, 112, 576, 80);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("AudioEffect3"), 26,lv_color_hex(0xF2F2F2), lv_color_hex(0),lv_color_hex(0), 2, 
        32, 192, 576, 80);
	divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("AudioEffect4"), 26,lv_color_hex(0xF2F2F2), lv_color_hex(0),lv_color_hex(0), 3, 
        32, 272, 576, 80);

	divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("AudioEffect5"), 26,lv_color_hex(0xF2F2F2), lv_color_hex(0),lv_color_hex(0), 4, 
        32, 352, 576, 80);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("AudioEffect6"), 26, lv_color_hex(0xF2F2F2),lv_color_hex(0),lv_color_hex(0), 5, 
        32, 432, 576, 80);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("AudioEffect7"), 26,lv_color_hex(0xF2F2F2), lv_color_hex(0),lv_color_hex(0), 6, 
        32, 512, 576, 80);
	divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("AudioEffect8"), 26,lv_color_hex(0xF2F2F2), lv_color_hex(0),lv_color_hex(0), 7, 
        32, 592, 576, 80);

    divoom_lvgl_message_box_set_active_item(phandle->message_box_handle, phandle->psys_config->audio_effect);
}


static void divoom_lvgl_system_setting_main_event_cb(lv_event_t * e)
{
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_system_setting_ctrl phandle = (Pdivoom_lvgl_system_setting_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    //if(code == LV_EVENT_CLICKED)
    if(code == LV_EVENT_KEY)
    {
    	int i, value, active_id;
		char c = *((char *)lv_event_get_param(e));
		char *key_info = (char *)lv_event_get_param(e);
		
        Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();
        divoom_system_setting_birghtness_conf brightness;
		
        if(phandle == NULL || phandle->btn_wait_flag == 1)
        { 	
            return;
        }
        switch(c)
        {
        	case DIVOOM_LVGL_KEY_PRE:
			case DIVOOM_LVGL_KEY_NEXT:
				if(phandle->btn_status == 0)
				{
					cur_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_obj, c == DIVOOM_LVGL_KEY_NEXT?DIVOOM_MENU_DIRECT_UP:DIVOOM_MENU_DIRECT_DOWN);                            
                	lv_group_focus_obj(cur_obj);
				}
				else
				{
					active_id = divoom_menu_direct_ctrl_get_pos(phandle->pdirect_handle, cur_obj);
					if(active_id == 0)
					{
						value = lv_slider_get_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN]);
						if(c == DIVOOM_LVGL_KEY_NEXT)
						{
                            #ifdef DIVOOM_TIME_FRAME_SUPPORT_LOW_LIGHT
                            if(value <= DIVOOM_SYS_BRIGHT_ON_FLOOR)
                            {
                                value = 0;
                            }
                            else  if(value)
                            {
							    value = (value > 5) ? (value - 5) : (DIVOOM_SYS_BRIGHT_ON_FLOOR);
                                if(value <= DIVOOM_SYS_BRIGHT_ON_FLOOR)
                                {
                                    value = DIVOOM_SYS_BRIGHT_ON_FLOOR;
                                }
                            }
                            #else
							if(value > 15)
							{
								value = value - 5;
							}
							else if(value > 10)
							{
								value = 10;
							}
							else //if(value <= 10)
							{
								value = 0;
							}
                            #endif
						}
						else
						{
                            #ifdef DIVOOM_TIME_FRAME_SUPPORT_LOW_LIGHT
                            if(value == 0)
                            {
                                value = DIVOOM_SYS_BRIGHT_ON_FLOOR;
                            }
                            else if(value == DIVOOM_SYS_BRIGHT_ON_FLOOR)
                            {
                                value = DIVOOM_SYS_BRIGHT_ON_FLOOR + 5;
                            }
                            else  if(value)
                            {
							    value = (value < 95) ? (value + 5) : (100);
                            }
                            #else
							if(value < 10)
							{
								value = 10;
                            }
							else
							{
								value = (value < 95) ? (value + 5) : (100);
							}
							#endif
						}
						lv_slider_set_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN], value, LV_ANIM_ON);
                        divoom_main_set_sys_light(value);
						brightness.light = value;
						divoom_net_app_send_packet(DIVOOM_NET_COMM_SET_SYS_BRIGHTNESS, DIVOOM_SERVER_TOPIC, &brightness);
					}
					/*else if(active_id == 4)
					{
						value = lv_slider_get_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_MEDIA_VOLUME_BTN]);
						if(c == DIVOOM_LVGL_KEY_NEXT)
						{
							value = (value > 5) ? (value - 5) : (0);
						}
						else
						{
							value = (value < 95) ? (value + 5) : (100);
						}
						lv_slider_set_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_MEDIA_VOLUME_BTN], value, LV_ANIM_ON);
						psys_config->audio_volume = value;
						divoom_main_send_sys_settings_config();
						divoom_main_save_config();
						
					}*/
					else if(active_id == DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN)
					{
						value = lv_slider_get_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN]);
						if(c == DIVOOM_LVGL_KEY_NEXT)
						{
							value = (value > 5) ? (value - 5) : (0);
						}
						else
						{
							value = (value < 95) ? (value + 5) : (100);
						}
						lv_slider_set_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN], value, LV_ANIM_ON);
						psys_config->volume = value;
						divoom_main_send_sys_settings_config();
						divoom_main_save_config();
					}
				}
				break;
			case DIVOOM_LVGL_KEY_ENTER:
				active_id = divoom_menu_direct_ctrl_get_pos(phandle->pdirect_handle, cur_obj);
				if(divoom_config_set_china_config(-1) == 1 && active_id >= DIVOOM_LVGL_SYSTEM_SETTING_MENU_LANGUAGE_BTN)
				{
					active_id++;
				}
				
				if(phandle->btn_status == 0)
				{
					if(active_id == 0)
					{
						phandle->btn_status = 1;
						lv_obj_add_style(cur_obj, &phandle->btn_edit_style, LV_STATE_FOCUSED);
						lv_obj_add_style(cur_obj, &phandle->btn_edit_style, LV_STATE_FOCUS_KEY);
					}
					else if(active_id == DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN)
					{
						phandle->btn_status = 1;
						lv_obj_add_style(cur_obj, &phandle->btn_edit_style, LV_STATE_FOCUSED);
						lv_obj_add_style(cur_obj, &phandle->btn_edit_style, LV_STATE_FOCUS_KEY);
					}
					else
					{
						switch(active_id)
                		{
                    	 	case DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN:
                        		value = lv_slider_get_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN]);
								value = (value < 95) ? (value + 5) : (100);
								lv_slider_set_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN], value, LV_ANIM_ON);
                        		divoom_main_set_sys_light(value);
								brightness.light = value;
								divoom_net_app_send_packet(DIVOOM_NET_COMM_SET_SYS_BRIGHTNESS, DIVOOM_SERVER_TOPIC, &brightness);
								break;
                        	
                   	 	    case DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN:
                     	   		value = lv_slider_get_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN]);
								value = (value < 95) ? (value + 5) : (100);
								lv_slider_set_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN], value, LV_ANIM_ON);
								psys_config->volume = value;
								divoom_main_send_sys_settings_config();
								divoom_main_save_config();
                        		break;
                        
						/*	case DIVOOM_LVGL_SYSTEM_SETTING_MENU_MEDIA_VOLUME_BTN:
                     	   		value = lv_slider_get_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_MEDIA_VOLUME_BTN]);
								value = (value < 95) ? (value + 5) : (100);
								lv_slider_set_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_MEDIA_VOLUME_BTN], value, LV_ANIM_ON);
								psys_config->audio_volume = value;
								divoom_main_send_sys_settings_config();
								divoom_main_save_config();
                        		break;
                    		case DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_MEMORY_BTN:
                        		if(psys_config->volume_memory)
                        		{
                        	 	   psys_config->volume_memory = 0;
                        		    divoom_menu_control_lv_img_set_src(phandle->func_obj_tbl[i], "26/26_off.png");
                        		}
                        		else
                        		{
                        		    psys_config->volume_memory = 1;
                        		    divoom_menu_control_lv_img_set_src(phandle->func_obj_tbl[i], "26/26_on.png");
                        		}
                        		divoom_main_save_config();
                        		break;
                   		
                   			case DIVOOM_LVGL_SYSTEM_SETTING_MENU_THEME_APPEARANCE_BTN:
								divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_THEME, NULL, 0, 0);
								break;
						*/	
                   			case DIVOOM_LVGL_SYSTEM_SETTING_MENU_SLIDE_SETTING_BTN:
								divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SLIDE_SETING_MENU, NULL, 0, 0);
								break;
							
							case DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIND_SPEED_BTN:
								divoom_lvgl_system_setting_create_wind_speed_setting(phandle);
								break;
							
                    		case DIVOOM_LVGL_SYSTEM_SETTING_MENU_DATA_TIME_BTN:
                        		divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_DATE_TIME, NULL, 0, 0);
                        		break;
                        
                    		case DIVOOM_LVGL_SYSTEM_SETTING_MENU_TEMPERATURE_BTN:
                    	    	divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_TEMPERATURE, NULL, 0, 0);
                        		break;
                        
                    		case DIVOOM_LVGL_SYSTEM_SETTING_MENU_LANGUAGE_BTN:
                        		divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_LANGUGE, NULL, 0, 0);
                        		break;

                    		case DIVOOM_LVGL_SYSTEM_SETTING_MENU_MCP_TOOL_BTN:
                        		divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_MCP_TOOL, NULL, 0, 0);
                        		break;
                        
                    	/*	case DIVOOM_LVGL_SYSTEM_SETTING_MENU_BLUETOOTH_BTN:
                        		divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_BLUETOOTH, NULL, 0, 0);
                        		break;
                        */
                    		case DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIFI_BTN:
                        		divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_WIFI, NULL, 0, 0);
                        		break;

						/*	case DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIFI_PROPERTIES_BTN:
								divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_WIFI_PROPERTIES, NULL, 0, 0);
								break;
						*/	
                    		case DIVOOM_LVGL_SYSTEM_SETTING_MENU_NIGHTVIEW_BTN:
                        		divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_NIGHTVIEW, NULL, 0, 0);
                        		break;

							case DIVOOM_LVGL_SYSTEM_SETTING_MENU_COLOR_TEMP_BTN:
                        		divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_COLOR_TEMP, NULL, 0, 0);
                        		break;
                        
                    	/*	case DIVOOM_LVGL_SYSTEM_SETTING_MENU_SCREEN_SAVE_BTN:
                        		divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_SCREEN_SAVE, NULL, 0, 0);
                        		break;
                        
                    		case DIVOOM_LVGL_SYSTEM_SETTING_MENU_IDLE_POWER_OFF_BTN:
                        		divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_IDLE_POWER_OFF_TIME, NULL, 0, 0);                        
                        		break;
                        
                    		case DIVOOM_LVGL_SYSTEM_SETTING_MENU_POWER_ON_OFF_TIME_BTN:
								divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_POWER_OFF_TIME, NULL, 0, 0);
                        		break;
                        
                    		case DIVOOM_LVGL_SYSTEM_SETTING_MENU_MIRROR_BTN:
                        		if(psys_config->mirror_flag)
                        		{
                         	   		psys_config->mirror_flag = 0;
                         	 	  	divoom_menu_control_lv_img_set_src(phandle->func_obj_tbl[active_id], "26/26_off.png");
                        		}
                        		else
                        		{
                        	    	psys_config->mirror_flag = 1;
                        	    	divoom_menu_control_lv_img_set_src(phandle->func_obj_tbl[active_id], "26/26_on.png");
                        		}
								divoom_main_send_sys_settings_config();
                        		divoom_main_save_config();
                        		break;
                        */ 	
							case DIVOOM_LVGL_SYSTEM_SETTING_MENU_BLUETOOTH_BTN:
								divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_BLUETOOTH, NULL, 0, 0);
								break;

						/*	case DIVOOM_LVGL_SYSTEM_SETTING_MENU_MEDIA_VOLUME_EFFECTS_BTN:
								divoom_lvgl_system_setting_create_audio_effects_setting(phandle);
								break;
						*/
                        	case DIVOOM_LVGL_SYSTEM_SETTING_MENU_SHOW_GRID_BTN:
								if(psys_config->show_grid_flag)
                        		{
                         	   		psys_config->show_grid_flag = 0;
                         	 	  	divoom_menu_control_lv_img_set_src(phandle->func_obj_tbl[active_id], "26/26_off.png");
                        		}
                        		else
                        		{
                        	    	psys_config->show_grid_flag = 1;
                        	    	divoom_menu_control_lv_img_set_src(phandle->func_obj_tbl[active_id], "26/26_on.png");
                        		}
								divoom_main_send_sys_settings_config();
                        		divoom_main_save_config();
								break;
                    		case DIVOOM_LVGL_SYSTEM_SETTING_MENU_ABOUT_BTN:
                    			{    
                        			divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_ABOUT, NULL, 0, 0);
                    			}
                        		break;
                        
                    		case DIVOOM_LVGL_SYSTEM_SETTING_MENU_RESTORE_BTN:
								//divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_RESTORE, NULL, 0, 0);
								lv_label_set_text(phandle->lang_label_obj_tbl[22], divoom_menu_control_get_disp_str("Factory Restore"));
								lv_label_set_text(phandle->lang_label_obj_tbl[23], divoom_menu_control_get_disp_str("FactoryRestoreComfirm"));
								lv_label_set_text(phandle->lang_label_obj_tbl[24], divoom_menu_control_get_disp_str("Cancel"));
								lv_label_set_text(phandle->lang_label_obj_tbl[25], divoom_menu_control_get_disp_str("Ok"));
								lv_group_focus_obj(phandle->box_btn[DIVOOM_LVGL_SELECT_BOX_BTN_OK]);
								phandle->select_focus_btn = cur_obj;
								phandle->select_id = 2;
								lv_obj_clear_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);
								lv_obj_clear_flag(phandle->reset_restore_box_obj, LV_OBJ_FLAG_HIDDEN);
								break;
							case DIVOOM_LVGL_SYSTEM_SETTING_MENU_RESET_ALL_BTN:
								lv_label_set_text(phandle->lang_label_obj_tbl[22], divoom_menu_control_get_disp_str("Reset all network and settings"));
								lv_label_set_text(phandle->lang_label_obj_tbl[23], divoom_menu_control_get_disp_str("ResetNetworkConfirm"));
								lv_label_set_text(phandle->lang_label_obj_tbl[24], divoom_menu_control_get_disp_str("Cancel"));
								lv_label_set_text(phandle->lang_label_obj_tbl[25], divoom_menu_control_get_disp_str("Ok"));
								lv_group_focus_obj(phandle->box_btn[DIVOOM_LVGL_SELECT_BOX_BTN_OK]);
								phandle->select_focus_btn = cur_obj;
								phandle->select_id = 1;
								lv_obj_clear_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);
								lv_obj_clear_flag(phandle->reset_restore_box_obj, LV_OBJ_FLAG_HIDDEN);
                        		break;
                        	default:
								break;
                		}
					}
				}
				else
				{
					/*if(active_id == 0)
					{
						value = lv_slider_get_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN]);
						value = (value < 95) ? (value + 5) : (100);
						lv_slider_set_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN], value, LV_ANIM_ON);
                        divoom_main_set_sys_light(value);
						brightness.light = value;
						divoom_net_app_send_packet(DIVOOM_NET_COMM_SET_SYS_BRIGHTNESS, DIVOOM_SERVER_TOPIC, &brightness);
					}
					else if(active_id == 3)
					{
						value = lv_slider_get_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN]);
						value = (value < 95) ? (value + 5) : (100);
						lv_slider_set_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN], value, LV_ANIM_ON);
						psys_config->volume = value;
						divoom_main_send_sys_settings_config();
						divoom_main_save_config();
					}*/
				}
        		break;
				
        	case DIVOOM_LVGL_KEY_ESC:
				if(phandle->btn_status == 0)
				{
                    divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_EXIT_MENU_SETTING);
				}
				else if(phandle->btn_status == 1)
				{
					phandle->btn_status = 0;
					lv_obj_add_style(cur_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
					lv_obj_add_style(cur_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
				}
				break;
				
			default:
				divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
				break;
       }
    }
}

void divoom_lvgl_system_setting_waiting_anim_exec_cb(void * var, int32_t v) 
{
    Pdivoom_lvgl_system_setting_ctrl phandle = (Pdivoom_lvgl_system_setting_ctrl)var; 

	if(phandle == NULL)
	{
		return;
	}
	
	phandle->wait_img_angle += v;

	if(phandle->wait_img_angle >= 3600)
	{
		phandle->wait_img_angle = 0;
	}

    lv_img_set_angle(phandle->waiting_obj, phandle->wait_img_angle);        // 设置图像角度
}


void divoom_lvgl_system_setting_start_waiting_anim(Pdivoom_lvgl_system_setting_ctrl phandle)
{
	lv_anim_t anim;

	lv_anim_init(&anim);
	lv_anim_set_var(&anim, phandle);
 	lv_anim_set_exec_cb(&anim, divoom_lvgl_system_setting_waiting_anim_exec_cb);
 	lv_anim_set_time(&anim, 175);
 	lv_anim_set_values(&anim, 0, 450);			//45度
	lv_anim_set_path_cb(&anim, lv_anim_path_step);
	lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);

	lv_obj_clear_flag(phandle->waiting_obj, LV_OBJ_FLAG_HIDDEN);

	lv_anim_start(&anim);
}

static void divoom_lvgl_system_setting_select_box_event_cb(lv_event_t * e)
{
	lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_system_setting_ctrl phandle = (Pdivoom_lvgl_system_setting_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);

    if(code == LV_EVENT_KEY)
    {
		char c = *((char *)lv_event_get_param(e));

		switch(c)
		{
			case DIVOOM_LVGL_KEY_PRE:
			case DIVOOM_LVGL_KEY_NEXT:
				if(cur_obj == phandle->box_btn[DIVOOM_LVGL_SELECT_BOX_BTN_CANCEL])
				{
					lv_group_focus_obj(phandle->box_btn[DIVOOM_LVGL_SELECT_BOX_BTN_OK]);
				}
				else
				{
					lv_group_focus_obj(phandle->box_btn[DIVOOM_LVGL_SELECT_BOX_BTN_CANCEL]);
				}
				break;
			case DIVOOM_LVGL_KEY_ENTER:
				if(cur_obj == phandle->box_btn[DIVOOM_LVGL_SELECT_BOX_BTN_CANCEL])//取消
				{
					lv_obj_add_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);
					lv_obj_add_flag(phandle->reset_restore_box_obj, LV_OBJ_FLAG_HIDDEN);
				}
				else//确定
				{
					int event_info[] = {DIVOOM_MAIN_EVENT_CONFIG_NET_WORK, DIVOOM_MAIN_EVENT_FORMAT_USERDATA, DIVOOM_MAIN_EVENT_RESET_FACTORY_SETTING};
					if(phandle->select_id == 1)
					{
						divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_CONFIG_NET_WORK);
						lv_obj_add_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);
					}
					else if(phandle->select_id == 2)
					{
						divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_RESET_FACTORY_SETTING);
						phandle->btn_wait_flag = 1;
						divoom_lvgl_system_setting_start_waiting_anim(phandle);
					}
					
					lv_obj_add_flag(phandle->reset_restore_box_obj, LV_OBJ_FLAG_HIDDEN);
				}
				lv_group_focus_obj(phandle->select_focus_btn);
				phandle->select_focus_btn = NULL;
				break;
			case DIVOOM_LVGL_KEY_ESC:
				lv_obj_add_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);
				lv_obj_add_flag(phandle->reset_restore_box_obj, LV_OBJ_FLAG_HIDDEN);
				lv_group_focus_obj(phandle->select_focus_btn);
				phandle->select_focus_btn = NULL;
				break;
			default:
				break;
		}
    }
}


static void divoom_lvgl_system_setting_update_wifi_bt_status(Pdivoom_lvgl_system_setting_ctrl phandle)
{
	lv_obj_t *status_lbl;
	char ssid[64];
	const DeviceInfo *bt_dev;

	if (!phandle)
		return;

	status_lbl = (lv_obj_t *)phandle->image_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIFI_BTN];
	if (status_lbl)
	{
		if (divoom_wifi_model_get_wifi_status())
		{
			memset(ssid, 0, sizeof(ssid));
			divoom_wifi_get_connect_ssid_info(ssid);
			if (ssid[0] != '\0')
				lv_label_set_text(status_lbl, ssid);
			else
				lv_label_set_text(status_lbl, "");
		}
		else
		{
			lv_label_set_text(status_lbl, "");
		}
	}

	status_lbl = (lv_obj_t *)phandle->image_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BLUETOOTH_BTN];
	if (status_lbl)
	{
		if (divoom_bt_music_is_connected())
		{
			bt_dev = divoom_bt_music_get_current_device_info();
			if (bt_dev && bt_dev->device_name[0] != '\0')
				lv_label_set_text(status_lbl, bt_dev->device_name);
			else
				lv_label_set_text(status_lbl, "");
		}
		else
		{
			lv_label_set_text(status_lbl, "");
		}
	}
}

static void divoom_lvgl_system_setting_show_cached_conn_status(Pdivoom_lvgl_system_setting_ctrl phandle)
{
	if (!phandle)
		return;
	divoom_lvgl_system_setting_set_battery_level_img(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BATTERY_BTN]);
	divoom_lvgl_system_setting_update_wifi_bt_status(phandle);
}

static void divoom_lvgl_system_setting_refresh_conn_status(Pdivoom_lvgl_system_setting_ctrl phandle)
{
	if (!phandle)
		return;
	divoom_lvgl_system_setting_show_cached_conn_status(phandle);
}

/* 进入设置页时向 JL 查询已连接手机信息（异步应答后由定时刷新更新蓝牙行） */
static void divoom_lvgl_system_setting_request_bt_phone_info(void)
{
#ifdef DIVOOM_TIMES_VIBE
	(void)divoom_bt_music_fetch_connected_phone_info_from_jl();
#endif
}

void divoom_lvgl_system_setting_refresh_brightness(struct _lv_timer_t *userdata)
{
    Pdivoom_lvgl_system_setting_ctrl phandle = (Pdivoom_lvgl_system_setting_ctrl)lv_timer_get_user_data(userdata);

    
	if(lv_slider_get_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN]) != divoom_main_get_sys_light())
	{
        lv_slider_set_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN], divoom_main_get_sys_light(), LV_ANIM_ON);
	}

	phandle->refresh_time++;

	if (phandle->refresh_time == 1 || phandle->refresh_time % 100 == 0)
	{
		divoom_lvgl_system_setting_refresh_conn_status(phandle);
	}
}

void *divoom_lvgl_system_setting_init(void *user_data, int menu_id)
{
	int index = 0, y_offset = -280;
    void *image_obj, *label_obj, *slide_obj, *btn_obj;
    Pdivoom_lvgl_system_setting_ctrl phandle;
    Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();
	int hide_language_btn = (divoom_config_set_china_config(-1) == 1);
	int lang_menu_y_offset = hide_language_btn ? -1 : 0;
	int lang_pos_y_offset = hide_language_btn ? -98 : 0;
	
    phandle = divoom_system_malloc(sizeof(divoom_lvgl_system_setting_ctrl));
    phandle->menu_id = menu_id;
    phandle->lang = psys_config->lang;
	phandle->refresh_time = 0;
	phandle->pdirect_handle = divoom_menu_direct_ctrl_create(1, hide_language_btn ? 16 : 17);
	phandle->first_active_obj = NULL;
	phandle->psys_config = divoom_main_get_system_config();

    divoom_menu_control_init_lable_style(&phandle->normal_style);
    phandle->main_obj = lv_obj_create(NULL);    
    lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(0xE4E4E4), LV_PART_MAIN);
    lv_obj_set_pos(phandle->main_obj, 0, 0);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_add_style(phandle->main_obj, &phandle->normal_style, 0);
    lv_obj_set_scrollbar_mode(phandle->main_obj, LV_SCROLLBAR_MODE_OFF);
    //lv_obj_set_scroll_dir(phandle->main_obj, LV_DIR_VER);
	lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
    //lv_obj_set_style_bg_opa(phandle->main_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
    
	lv_style_init(&phandle->btn_normal_style);
	lv_style_set_bg_color(&phandle->btn_normal_style, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->btn_normal_style, 0);
	lv_style_set_border_width(&phandle->btn_normal_style, 4);
	lv_style_set_border_color(&phandle->btn_normal_style, lv_color_hex(0xE4E4E4));
	lv_style_set_radius(&phandle->btn_normal_style, 17);
	lv_style_set_shadow_width(&phandle->btn_normal_style, 0);
	lv_style_set_outline_width(&phandle->btn_normal_style, 0);

	lv_style_init(&phandle->btn_select_style);
	lv_style_set_bg_color(&phandle->btn_select_style, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->btn_select_style, 0);
	lv_style_set_border_width(&phandle->btn_select_style, 4);
	lv_style_set_border_color(&phandle->btn_select_style, lv_color_hex(0x6D6D6D));
	lv_style_set_radius(&phandle->btn_select_style, 17);
	lv_style_set_shadow_width(&phandle->btn_select_style, 0);
	lv_style_set_outline_width(&phandle->btn_select_style, 0);

	lv_style_init(&phandle->btn_edit_style);
	lv_style_set_bg_color(&phandle->btn_edit_style, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->btn_edit_style, 0);
	lv_style_set_border_width(&phandle->btn_edit_style, 4);
	lv_style_set_border_color(&phandle->btn_edit_style, lv_color_hex(0xE89B00));
	lv_style_set_radius(&phandle->btn_edit_style, 17);
	lv_style_set_shadow_width(&phandle->btn_edit_style, 0);
	lv_style_set_outline_width(&phandle->btn_edit_style, 0);
	
    label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 100, 50);
	lv_obj_set_size(label_obj, 600, 78);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0);  
    index++;
	
    /*label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 74, 168);
	lv_obj_set_size(label_obj, 600, 32);
    phandle->lang_label_obj_tbl[1] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[1], divoom_menu_control_get_disp_str("ScreenBright"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X9E9E9E), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);  
      
    image_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(image_obj, 37, 209);
    lv_obj_set_size(image_obj, 726, 90);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_1line_bg.png");*/

	lv_obj_t *cont_obj = lv_obj_create(phandle->main_obj);
	lv_obj_remove_style(cont_obj, NULL, LV_PART_SCROLLBAR);
    lv_obj_set_style_pad_all(cont_obj, 0, 0);
    lv_obj_set_style_border_width(cont_obj, 0, 0);
    lv_obj_set_style_outline_width(cont_obj, 0, 0);
    lv_obj_set_style_border_side(cont_obj, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_style_bg_color(cont_obj, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_pad_bottom(cont_obj, 44, 0);
    lv_obj_set_size(cont_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH - 128);
    lv_obj_set_pos(cont_obj, 0, 128);

	//显示
	label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 32);
	lv_obj_set_size(label_obj, 310, 32);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x636363), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN] = label_obj;
	index++;
	
	//亮度
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 76);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 0);
	if(phandle->first_active_obj == NULL)
	{
		phandle->first_active_obj = btn_obj;
	}
	
	image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 76, 103);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_birght_min.png");
    
    
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 103);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_birght_max.png");
    
    
    
    slide_obj = lv_slider_create(cont_obj);
    lv_slider_set_range(slide_obj, 0, 100);
    lv_obj_set_pos(slide_obj, 133, 121);
    lv_obj_set_size(slide_obj, 530, 4);
    lv_slider_set_value(slide_obj, psys_config->brightness, LV_ANIM_ON);
    lv_obj_set_style_bg_color(slide_obj, lv_color_hex(0x707070), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slide_obj, lv_color_hex(0xB1B1B1), LV_PART_MAIN);
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN] = slide_obj;
    

    
    /*label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 74, 339);
	lv_obj_set_size(label_obj, 600, 32);
    phandle->lang_label_obj_tbl[2] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[2], divoom_menu_control_get_disp_str("VoiceNotice"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X9E9E9E), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(24, phandle->menu_id), 0); 
    
    
    image_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(image_obj, 37, 377);
    lv_obj_set_size(image_obj, 726, 180);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_2line_bg.png");
    
    
    image_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(image_obj, 37, 377);
    lv_obj_set_size(image_obj, 90, 90);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_volume_min.png");
    
    image_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(image_obj, 37, 673);
    lv_obj_set_size(image_obj, 90, 90);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_volume_max.png");
    
    
    slide_obj = lv_slider_create(phandle->main_obj);
    lv_slider_set_range(slide_obj, 0, 100);
    lv_obj_set_pos(slide_obj, 135, 422);
    lv_obj_set_size(slide_obj, 530, 4);
    lv_slider_set_value(slide_obj, psys_config->volume, LV_ANIM_ON);
    lv_obj_set_style_bg_color(slide_obj, lv_color_hex(DIVOOM_MENNU_SLIDER_ACTIVE_COLOR), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slide_obj, lv_color_hex(DIVOOM_MENNU_SLIDER_UNACTIVE_COLOR), LV_PART_MAIN);
    lv_obj_add_flag(slide_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(slide_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN] = slide_obj;
	
    

    label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 74, 491);
	lv_obj_set_size(label_obj, 500, 42);
    phandle->lang_label_obj_tbl[3] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[3], divoom_menu_control_get_disp_str("Volume_memory"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_MEMORY_BTN] = label_obj;
    lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(label_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
    
    

    image_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(image_obj, 598, 467);
    lv_obj_set_size(image_obj, 165, 90);
    if(psys_config->volume_memory)
    {
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_on.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_off.png");
    }
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_MEMORY_BTN] = image_obj;
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
	
    
    image_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(image_obj, 37, 585);
    lv_obj_set_size(image_obj, 726, 450);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_5line_bg.png");
    

	//主题外观
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 302-128);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 1);

	label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 330-128);
	lv_obj_set_size(label_obj, 400, 34);
    phandle->lang_label_obj_tbl[2] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[2], divoom_menu_control_get_disp_str("ThemeAppearance"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_THEME_APPEARANCE_BTN] = label_obj;
    
	image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 329-128);
	lv_obj_set_size(image_obj, 36, 36);
	divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
	phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_THEME_APPEARANCE_BTN] = image_obj;
	*/
    //幻灯片设置
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 174);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 1);

	label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 202);
	lv_obj_set_size(label_obj, 400, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_SLIDE_SETTING_BTN] = label_obj;
    
	image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 201);
	lv_obj_set_size(image_obj, 36, 36);
	divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
	phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_SLIDE_SETTING_BTN] = image_obj;
	index++;
	
	//定时关机时间
    /*btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 272);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 2);
    

    label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 300);
	lv_obj_set_size(label_obj, 400+100, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_POWER_ON_OFF_TIME_BTN] = label_obj;
    
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 299);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_POWER_ON_OFF_TIME_BTN] = image_obj;
    index++;
	*/
	//夜览模式
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 272);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 2);
    
    label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 300);
	lv_obj_set_size(label_obj, 400, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_NIGHTVIEW_BTN] = label_obj;
   
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 299);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_NIGHTVIEW_BTN] = image_obj;
    index++;

	//色温调节
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 370);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 3);
    
    label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 398);
	lv_obj_set_size(label_obj, 400, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_NIGHTVIEW_BTN] = label_obj;
   
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 397);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_COLOR_TEMP_BTN] = image_obj;
    index++;

	/*
	//屏保设置
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 694-128);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 5);
		
	label_obj = lv_label_create(cont_obj);
	lv_obj_set_pos(label_obj, 76, 722-128);
	lv_obj_set_size(label_obj, 400, 34);
	phandle->lang_label_obj_tbl[6] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[6], divoom_menu_control_get_disp_str("ScreenSave"));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, phandle->menu_id), 0); 
	phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_SCREEN_SAVE_BTN] = label_obj;
	//lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
	//lv_obj_add_event_cb(label_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
	
		
	image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 721-128);
	lv_obj_set_size(image_obj, 36, 36);
	divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
	phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_SCREEN_SAVE_BTN] = image_obj;
	//lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
	//lv_obj_add_event_cb(image_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
	
	//媒体与音效
	label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 508);
	lv_obj_set_size(label_obj, 310, 32);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x636363), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    //phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN] = label_obj;
	index++;

	//媒体音量
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 552);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 4);
	
	image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 72, 577);
    lv_obj_set_size(image_obj, 40, 40);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_vol_min.png");
    
    
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 684, 577);
    lv_obj_set_size(image_obj, 40, 40);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_vol_max.png");
    
    
    
    slide_obj = lv_slider_create(cont_obj);
    lv_slider_set_range(slide_obj, 0, 100);
    lv_obj_set_pos(slide_obj, 148, 593);
    lv_obj_set_size(slide_obj, 509, 4);
    lv_slider_set_value(slide_obj, psys_config->audio_volume, LV_ANIM_ON);
    lv_obj_set_style_bg_color(slide_obj, lv_color_hex(0x707070), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slide_obj, lv_color_hex(0xB1B1B1), LV_PART_MAIN);
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_MEDIA_VOLUME_BTN] = slide_obj;

	//音效
    btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 650);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 5);
	
	label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 678);
	lv_obj_set_size(label_obj, 600, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_MEDIA_VOLUME_EFFECTS_BTN] = label_obj;
    //lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_add_event_cb(label_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
    index++;

	label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 600-300, 678);
	lv_obj_set_size(label_obj, 198+200, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    if(psys_config->audio_effect == 0)
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str("audioeffect1"));
    }
    else if(psys_config->audio_effect == 1)
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str("audioeffect2"));
    }
	else if(psys_config->audio_effect == 2)
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str("audioeffect3"));
    }
	else if(psys_config->audio_effect == 3)
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str("audioeffect4"));
    }
	else if(psys_config->audio_effect == 4)
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str("audioeffect5"));
    }
	else if(psys_config->audio_effect == 5)
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str("audioeffect6"));
    }
	else if(psys_config->audio_effect == 6)
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str("audioeffect7"));
    }
	else if(psys_config->audio_effect == 7)
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str("audioeffect8"));
    }

    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X616161), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->image_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_MEDIA_VOLUME_EFFECTS_BTN] = label_obj;

    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 677);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_MEDIA_VOLUME_EFFECTS_BTN] = btn_obj;
    index++;
	*/


	//系统提示音
	label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 788 + y_offset);
	lv_obj_set_size(label_obj, 310, 32);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x636363), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    //phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN] = label_obj;
	index++;
	/* 占位：原「按键音效」项已从界面移除，保持与 pdivoom_lvgl_setting_list 索引一致 */
	index++;

	//音量
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 832 + y_offset);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 4);
	
	image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 72, 857 + y_offset);
    lv_obj_set_size(image_obj, 40, 40);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_vol_min.png");
    
    
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 684, 857 + y_offset);
    lv_obj_set_size(image_obj, 40, 40);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_vol_max.png");
    
    
    
    slide_obj = lv_slider_create(cont_obj);
    lv_slider_set_range(slide_obj, 0, 100);
    lv_obj_set_pos(slide_obj, 148, 873 + y_offset);
    lv_obj_set_size(slide_obj, 509, 4);
    lv_slider_set_value(slide_obj, psys_config->audio_volume, LV_ANIM_ON);
    lv_obj_set_style_bg_color(slide_obj, lv_color_hex(0x707070), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slide_obj, lv_color_hex(0xB1B1B1), LV_PART_MAIN);
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN] = slide_obj;
	
	//空闲关机时间
	/*btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 792-128);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 6);
		
	label_obj = lv_label_create(cont_obj);
	lv_obj_set_pos(label_obj, 76, 820-128);
	lv_obj_set_size(label_obj, 400, 34);
	phandle->lang_label_obj_tbl[7] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[7], divoom_menu_control_get_disp_str("IdlePowerOffTime"));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, phandle->menu_id), 0); 
	phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_IDLE_POWER_OFF_BTN] = label_obj;
	//lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
	//lv_obj_add_event_cb(label_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
		
	image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 819-128);
	lv_obj_set_size(image_obj, 36, 36);
	divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
	phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_IDLE_POWER_OFF_BTN] = image_obj;
	//lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
	//lv_obj_add_event_cb(image_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
	*/

	//网络
	/*label_obj = lv_label_create(cont_obj);
	lv_obj_set_pos(label_obj, 76, 690);
	lv_obj_set_size(label_obj, 310, 32);
	phandle->lang_label_obj_tbl[index] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0x636363), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
	index++;
	*/
	
	//网络连接 Wi-Fi
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 974 + y_offset);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 5);
		
	label_obj = lv_label_create(cont_obj);
	lv_obj_set_pos(label_obj, 76, 1002 + y_offset);
	lv_obj_set_size(label_obj, 400, 34);
	phandle->lang_label_obj_tbl[index] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
	phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIFI_BTN] = label_obj;

	label_obj = lv_label_create(cont_obj);
	lv_obj_set_pos(label_obj, SYS_SET_CONN_STATUS_LABEL_X, 1002 + y_offset);
	lv_obj_set_width(label_obj, SYS_SET_CONN_STATUS_LABEL_W);
	lv_label_set_long_mode(label_obj, LV_LABEL_LONG_DOT);
	lv_label_set_text(label_obj, "");
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0x616161), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_RIGHT, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
	phandle->image_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIFI_BTN] = label_obj;
		
	image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 1001 + y_offset);
	lv_obj_set_size(image_obj, 36, 36);
	divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
	phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIFI_BTN] = image_obj;
	index++;

	//蓝牙
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 1072 + y_offset);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 6);
		
	label_obj = lv_label_create(cont_obj);
	lv_obj_set_pos(label_obj, 76, 1100 + y_offset);
	lv_obj_set_size(label_obj, 400, 34);
	phandle->lang_label_obj_tbl[index] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
	phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BLUETOOTH_BTN] = label_obj;

	label_obj = lv_label_create(cont_obj);
	lv_obj_set_pos(label_obj, SYS_SET_CONN_STATUS_LABEL_X, 1100 + y_offset);
	lv_obj_set_width(label_obj, SYS_SET_CONN_STATUS_LABEL_W);
	lv_label_set_long_mode(label_obj, LV_LABEL_LONG_DOT);
	lv_label_set_text(label_obj, "");
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0x616161), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_RIGHT, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
	phandle->image_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BLUETOOTH_BTN] = label_obj;
		
	image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 1099 + y_offset);
	lv_obj_set_size(image_obj, 36, 36);
	divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
	phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BLUETOOTH_BTN] = image_obj;
	index++;

	//电池
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 1170 + y_offset);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 7);
		
	label_obj = lv_label_create(cont_obj);
	lv_obj_set_pos(label_obj, 76, 1198 + y_offset);
	lv_obj_set_size(label_obj, 400, 34);
	phandle->lang_label_obj_tbl[index] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
	phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BATTERY_BTN] = label_obj;
		
	image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 660, 1195 + y_offset);
	lv_obj_set_size(image_obj, 64, 40);
	divoom_lvgl_system_setting_set_battery_level_img(image_obj);
	phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BATTERY_BTN] = image_obj;
	index++;
	
	//偏好
	label_obj = lv_label_create(cont_obj);
	lv_obj_set_pos(label_obj, 76, 1308 + y_offset);
	lv_obj_set_size(label_obj, 310, 32);
	phandle->lang_label_obj_tbl[index] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0x636363), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
	index++;
	
    /*image_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(image_obj, 37, 1058);
    lv_obj_set_size(image_obj, 726, 450);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_5line_bg.png");
	
	label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 74, 880);
	lv_obj_set_size(label_obj, 400, 42);
    phandle->lang_label_obj_tbl[8] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[8], divoom_menu_control_get_disp_str("Bluetooth"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BLUETOOTH_BTN] = label_obj;
    lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(label_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
    
    image_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(image_obj, 673, 855);
    lv_obj_set_size(image_obj, 90, 90);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BLUETOOTH_BTN] = image_obj;
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
	
	
	//网络属性
    btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 1156-128);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 8);
	
	label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 1184-128);
	lv_obj_set_size(label_obj, 400, 34);
    phandle->lang_label_obj_tbl[10] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[10], divoom_menu_control_get_disp_str("NetworkProperties"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIFI_PROPERTIES_BTN] = label_obj;
    //lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_add_event_cb(label_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
    
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 1184-128);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIFI_PROPERTIES_BTN] = image_obj;
    */

	//时间与日期
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 1352 + y_offset);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 8);
	
    label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 1380 + y_offset);
	lv_obj_set_size(label_obj, 400, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_DATA_TIME_BTN] = label_obj;
    
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 1379 + y_offset);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_DATA_TIME_BTN] = image_obj;
    index++;
	
	//风速单位
    btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 1450 + y_offset);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 9);
	
	label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 1478 + y_offset);
	lv_obj_set_size(label_obj, 600, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIND_SPEED_BTN] = label_obj;
    //lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_add_event_cb(label_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
    index++;

	label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 600-100, 1478 + y_offset);
	lv_obj_set_size(label_obj, 198, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    if(psys_config->wind_speed_flag == 0)
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str("m/s"));
    }
    else if(psys_config->wind_speed_flag == 1)
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str("km/h"));
    }
	else if(psys_config->wind_speed_flag == 2)
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str("mph"));
    }
	else
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str("kt"));
    }

    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X616161), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->image_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIND_SPEED_BTN] = label_obj;

    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 1477 + y_offset);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIND_SPEED_BTN] = btn_obj;
    index++;

	//温度
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 1548 + y_offset);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 10);
	
    label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 1576 + y_offset);
	lv_obj_set_size(label_obj, 400, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_TEMPERATURE_BTN] = label_obj;
    index++;
	
    label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 600-100, 1576 + y_offset);
	lv_obj_set_size(label_obj, 198, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    if(psys_config->temp_c_f_flag)
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str("Fahrenheit"));
    }
    else
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str("Centigrade"));
    }
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X616161), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->image_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_TEMPERATURE_BTN] = label_obj;

    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 1575 + y_offset);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_TEMPERATURE_BTN] = image_obj;
    index++;
    
	//设备语言
	if(!hide_language_btn)
	{
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 1646 + y_offset);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 11);
	
    label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 1674 + y_offset);
	lv_obj_set_size(label_obj, 400, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_LANGUAGE_BTN] = label_obj;
    
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 1673 + y_offset);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_LANGUAGE_BTN] = image_obj;
	}
    index++;

	//PC表盘创作&MCP
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 1744 + y_offset + lang_pos_y_offset);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 12 + lang_menu_y_offset);

	label_obj = lv_label_create(cont_obj);
	lv_obj_set_pos(label_obj, 76, 1772 + y_offset + lang_pos_y_offset);
	lv_obj_set_size(label_obj, 600, 34);
	phandle->lang_label_obj_tbl[index] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0);
	phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_MCP_TOOL_BTN] = label_obj;

	image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 1771 + y_offset + lang_pos_y_offset);
	lv_obj_set_size(image_obj, 36, 36);
	divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
	phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_MCP_TOOL_BTN] = btn_obj;
	index++;

	//显示像素作品网格
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 1842 + y_offset + lang_pos_y_offset);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 13 + lang_menu_y_offset);
	
    label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 1870 + y_offset + lang_pos_y_offset);
	lv_obj_set_size(label_obj, 400, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_SHOW_GRID_BTN] = label_obj;
    
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 652, 1865 + y_offset + lang_pos_y_offset);
    lv_obj_set_size(image_obj, 72, 44);
	if(psys_config->show_grid_flag)
    {
    	divoom_menu_control_lv_img_set_src(image_obj, "26/26_on.png");
    }
	else
	{
		divoom_menu_control_lv_img_set_src(image_obj, "26/26_off.png");
	}
	phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_SHOW_GRID_BTN] = image_obj;
    index++;

	//系统管理
	label_obj = lv_label_create(cont_obj);
	lv_obj_set_pos(label_obj, 76, 1980 + y_offset + lang_pos_y_offset);
	lv_obj_set_size(label_obj, 310, 32);
	phandle->lang_label_obj_tbl[index] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0X636363), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
	index++;
	
	/*//控制
	label_obj = lv_label_create(cont_obj);
	lv_obj_set_pos(label_obj, 76, 1588-128);
	lv_obj_set_size(label_obj, 310, 32);
	phandle->lang_label_obj_tbl[15] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[15], divoom_menu_control_get_disp_str("Control"));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0X999999), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(24, phandle->menu_id), 0); 
	//phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_PREFERENCE_BTN] = label_obj;
	

    //镜像翻转
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 1548-128);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 12);
	
    label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 1564-128);
	lv_obj_set_size(label_obj, 400, 34);
    phandle->lang_label_obj_tbl[16] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[16], divoom_menu_control_get_disp_str("Mirror"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, phandle->menu_id), 0); 

	label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 1600-128);
	lv_obj_set_size(label_obj, 700, 22);
    phandle->lang_label_obj_tbl[17] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[17], divoom_menu_control_get_disp_str("MirrorPurpose"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0XAAAAAA), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(16, phandle->menu_id), 0); 
    
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 652, 1571-128);
    lv_obj_set_size(image_obj, 72, 44);
    if(psys_config->mirror_flag)
    {
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_on.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_off.png");
    }
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_MIRROR_BTN] = image_obj;
    //lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_add_event_cb(image_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
	*/
    
    //image_obj = lv_img_create(phandle->main_obj);
	//lv_obj_set_pos(image_obj, 37, 1534);
    //lv_obj_set_size(image_obj, 726, 90);
    //divoom_menu_control_lv_img_set_src(image_obj, "26/26_1line_bg.png");
    
	//关于
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 2024 + y_offset + lang_pos_y_offset);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 14 + lang_menu_y_offset);
	
    label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 2052 + y_offset + lang_pos_y_offset);
	lv_obj_set_size(label_obj, 400, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_ABOUT_BTN] = label_obj;
    //lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_add_event_cb(label_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
    
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 2051 + y_offset + lang_pos_y_offset);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_ABOUT_BTN] = image_obj;
    index++;
    
    /*image_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(image_obj, 37, 1650);
    lv_obj_set_size(image_obj, 726, 90);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_1line_bg.png");

	//重置
	label_obj = lv_label_create(cont_obj);
	lv_obj_set_pos(label_obj, 76, 1770-128);
	lv_obj_set_size(label_obj, 310, 32);
	phandle->lang_label_obj_tbl[19] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[19], divoom_menu_control_get_disp_str("ResetSystem"));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0X999999), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(24, phandle->menu_id), 0); 
	//phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_PREFERENCE_BTN] = label_obj;
	*/
	
	//重置设置和网络
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 2122 + y_offset + lang_pos_y_offset);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 15 + lang_menu_y_offset);
	
    label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 2150 + y_offset + lang_pos_y_offset);
	lv_obj_set_size(label_obj, 600, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_RESET_ALL_BTN] = label_obj;
    
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 2149 + y_offset + lang_pos_y_offset);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_RESET_ALL_BTN] = image_obj;
    index++;
	
    //清除内置储存数据
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 2220 + y_offset + lang_pos_y_offset);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 16 + lang_menu_y_offset);
	
    label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 2248 + y_offset + lang_pos_y_offset);
	lv_obj_set_size(label_obj, 400+100, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_RESTORE_BTN] = label_obj;
    
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 2247 + y_offset + lang_pos_y_offset);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_RESTORE_BTN] = image_obj;
    index++;
	
	//重置设置和网络弹窗
	image_obj = lv_img_create(phandle->main_obj);
	divoom_menu_control_lv_img_set_src(image_obj, "26/26_bg_cover.png");
	lv_obj_set_size(image_obj, 800, 1280);//2060);
	lv_obj_add_flag(image_obj, LV_OBJ_FLAG_HIDDEN);
	phandle->bg_cover_obj = image_obj;
	
	image_obj = lv_img_create(phandle->main_obj);
	divoom_menu_control_lv_img_set_src(image_obj, "26/26_select_box.png");
	lv_obj_set_pos(image_obj, 80, 320);//+800);
	lv_obj_add_flag(image_obj, LV_OBJ_FLAG_HIDDEN);
	phandle->reset_restore_box_obj = image_obj;

	label_obj = lv_label_create(image_obj);
    lv_obj_set_pos(label_obj, 0, 23);
	lv_obj_set_size(label_obj, 640, 44);
	phandle->lang_label_obj_tbl[index] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
	index++;
	
	label_obj = lv_label_create(image_obj);
	lv_obj_set_pos(label_obj, 20, 90+10);
	lv_obj_set_size(label_obj, 600-30, 180);
	phandle->lang_label_obj_tbl[index] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0);
	index++;
	
	//取消
	btn_obj = lv_btn_create(image_obj);
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
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_select_box_event_cb, LV_EVENT_KEY, phandle);
	phandle->box_btn[DIVOOM_LVGL_SELECT_BOX_BTN_CANCEL] = btn_obj;

	label_obj = lv_label_create(btn_obj);
	lv_obj_set_pos(label_obj, 0, 24);
	lv_obj_set_size(label_obj, 282, 42);
	phandle->lang_label_obj_tbl[index] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
	//lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0);
	index++;
	
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
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_select_box_event_cb, LV_EVENT_KEY, phandle);
	phandle->box_btn[DIVOOM_LVGL_SELECT_BOX_BTN_OK] = btn_obj;

	label_obj = lv_label_create(btn_obj);
	lv_obj_set_pos(label_obj, 0, 24);
	lv_obj_set_size(label_obj, 282, 42);
	phandle->lang_label_obj_tbl[index] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
	//lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0);

	image_obj = lv_img_create(phandle->main_obj);
	divoom_menu_control_lv_img_set_src(image_obj, "26/26_waiting.png");
	lv_obj_set_pos(image_obj, 360, 600);
	lv_obj_add_flag(image_obj, LV_OBJ_FLAG_HIDDEN);
	phandle->waiting_obj = image_obj;
	phandle->wait_img_angle = 0;
	phandle->btn_wait_flag = 0;
#if 0
    //下面创建各个子窗口
    divoom_lvgl_system_setting_create_date_time_menu(phandle, psys_config);
    
    divoom_lvgl_system_setting_create_temperature_menu(phandle, psys_config);
    
    divoom_lvgl_system_setting_create_screen_save_menu(phandle, psys_config);
    
    divoom_lvgl_system_setting_create_language_menu(phandle, psys_config);
    
    divoom_lvgl_system_setting_create_nightview_menu(phandle, psys_config);
    
    divoom_lvgl_system_setting_create_idle_poweroff_menu(phandle, psys_config);
    
    divoom_lvgl_system_setting_create_about_menu(phandle, psys_config);
    
    #endif
    phandle->time_handle = lv_timer_create(divoom_lvgl_system_setting_refresh_brightness, 16, phandle);
    return phandle;
}

void divoom_lvgl_system_setting_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_system_setting_ctrl phandle = (Pdivoom_lvgl_system_setting_ctrl)handle;
    Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();

    if(psys_config->lang != phandle->lang)
    {
        phandle->lang = psys_config->lang;

		for(int i = 0;i < sizeof(pdivoom_lvgl_setting_list)/sizeof(pdivoom_lvgl_setting_list[0]); i++)
		{
			lv_obj_set_style_text_font(phandle->lang_label_obj_tbl[i], divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[i].font_size, phandle->menu_id), 0);  
        	lv_label_set_text(phandle->lang_label_obj_tbl[i], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[i].setting_str));
		}
    }

	/*if(phandle->psys_config->audio_effect == 0)
	{
		lv_label_set_text(phandle->lang_label_obj_tbl[7], divoom_menu_control_get_disp_str("AudioEffect1"));
	}
	else if(phandle->psys_config->audio_effect == 1)
	{
		lv_label_set_text(phandle->lang_label_obj_tbl[7], divoom_menu_control_get_disp_str("AudioEffect2"));
	}
	else if(phandle->psys_config->audio_effect == 2)
	{
		lv_label_set_text(phandle->lang_label_obj_tbl[7], divoom_menu_control_get_disp_str("AudioEffect3"));
	}
	else if(phandle->psys_config->audio_effect == 3)
	{
		lv_label_set_text(phandle->lang_label_obj_tbl[7], divoom_menu_control_get_disp_str("AudioEffect4"));
	}
	else if(phandle->psys_config->audio_effect == 4)
	{
		lv_label_set_text(phandle->lang_label_obj_tbl[7], divoom_menu_control_get_disp_str("AudioEffect5"));
	}
	else if(phandle->psys_config->audio_effect == 5)
	{
		lv_label_set_text(phandle->lang_label_obj_tbl[7], divoom_menu_control_get_disp_str("AudioEffect6"));
	}
	else if(phandle->psys_config->audio_effect == 6)
	{
		lv_label_set_text(phandle->lang_label_obj_tbl[7], divoom_menu_control_get_disp_str("AudioEffect7"));
	}
	else if(phandle->psys_config->audio_effect == 7)
	{
		lv_label_set_text(phandle->lang_label_obj_tbl[7], divoom_menu_control_get_disp_str("AudioEffect8"));
	}
	*/
	if(psys_config->wind_speed_flag == 0)
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[13], divoom_menu_control_get_disp_str("m/s"));
    }
    else if(psys_config->wind_speed_flag == 1)
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[13], divoom_menu_control_get_disp_str("km/h"));
    }
	else if(psys_config->wind_speed_flag == 2)
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[13], divoom_menu_control_get_disp_str("mph"));
    }
	else
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[13], divoom_menu_control_get_disp_str("kt"));
    }

    if(psys_config->temp_c_f_flag)
    {
		lv_label_set_text(phandle->lang_label_obj_tbl[15], divoom_menu_control_get_disp_str("Fahrenheit"));
    }
    else
    {
		lv_label_set_text(phandle->lang_label_obj_tbl[15], divoom_menu_control_get_disp_str("Centigrade"));
    }
	if(psys_config->show_grid_flag)
    {
        divoom_menu_control_lv_img_set_src(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_SHOW_GRID_BTN], "26/26_on.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_SHOW_GRID_BTN], "26/26_off.png");
    }
    lv_slider_set_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN], psys_config->brightness, LV_ANIM_OFF);
	//lv_slider_set_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_MEDIA_VOLUME_BTN], psys_config->audio_volume, LV_ANIM_OFF);
    lv_slider_set_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN], psys_config->volume, LV_ANIM_OFF);
	if(phandle->first_active_obj)
	{
		lv_group_focus_obj(phandle->first_active_obj);
		phandle->first_active_obj = NULL;
	}
	if(active_obj)
    {
		lv_group_focus_obj(active_obj);
	}

    lv_scr_load(phandle->main_obj);
	phandle->refresh_time = 0;
	divoom_lvgl_system_setting_request_bt_phone_info();
	divoom_lvgl_system_setting_show_cached_conn_status(phandle);
    
}

void divoom_lvgl_system_setting_pause(void *handle)
{
}

void divoom_lvgl_system_setting_exit(void *handle)
{
    Pdivoom_lvgl_system_setting_ctrl phandle = (Pdivoom_lvgl_system_setting_ctrl)handle;

	if(phandle->btn_wait_flag)
	{
		lv_anim_del(phandle, NULL);
    }
    lv_timer_del(phandle->time_handle);
	lv_obj_del(phandle->main_obj);
    lv_style_reset(&phandle->normal_style);
	lv_style_reset(&phandle->btn_normal_style);
	lv_style_reset(&phandle->btn_edit_style);
	lv_style_reset(&phandle->btn_select_style);
	divoom_menu_direct_ctrl_destory(phandle->pdirect_handle);
    divoom_system_free(phandle);
    return ;
}



#else
enum{
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN,         //亮度
    //DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_MEMORY_BTN,  //音量记忆
    //DIVOOM_LVGL_SYSTEM_SETTING_MENU_THEME_APPEARANCE_BTN, //主题外观
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_SLIDE_SETTING_BTN,	//幻灯片设置
    //DIVOOM_LVGL_SYSTEM_SETTING_MENU_POWER_ON_OFF_TIME_BTN,      //定时开关机
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_NIGHTVIEW_BTN,      //夜览
	DIVOOM_LVGL_SYSTEM_SETTING_MENU_COLOR_TEMP_BTN,      //色温调节
    //DIVOOM_LVGL_SYSTEM_SETTING_MENU_SCREEN_SAVE_BTN,      //屏保
    //DIVOOM_LVGL_SYSTEM_SETTING_MENU_IDLE_POWER_OFF_BTN,      //空闲关机时间
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN,         //系统提示音音量
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIFI_BTN,      //WIFI
    //DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIFI_PROPERTIES_BTN,  //网络属性
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_DATA_TIME_BTN,      //日期与时间
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIND_SPEED_BTN,      //风速单位
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_TEMPERATURE_BTN,      //温度
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_LANGUAGE_BTN,      //语言
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_MCP_TOOL_BTN,    //PC表盘创作&MCP
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_SHOW_GRID_BTN,     //显示像素作品网格
    //DIVOOM_LVGL_SYSTEM_SETTING_MENU_BLUETOOTH_BTN,      //蓝牙
    //DIVOOM_LVGL_SYSTEM_SETTING_MENU_MIRROR_BTN,      //镜像
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_ABOUT_BTN,      //关于
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_RESET_ALL_BTN, //重置设置和网络
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_RESTORE_BTN,      //清除内置储存数据
    DIVOOM_LVGL_SYSTEM_SETTING_MENU_END_BTN,      //
};

enum{
	DIVOOM_LVGL_SELECT_BOX_BTN_CANCEL,	//取消
	DIVOOM_LVGL_SELECT_BOX_BTN_OK,		//确定
};

typedef struct _divoom_lvgl_system_setting_ctrl
{
    int menu_id;
    int lang;
    void *main_obj;
    void *back_obj;
    void *func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_END_BTN];
    void *label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_END_BTN];
    void *image_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_END_BTN];
    void *lang_label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_END_BTN*2];
    lv_style_t  normal_style;
	lv_style_t  btn_normal_style;
	lv_style_t	btn_select_style;
	lv_style_t	btn_edit_style;
	void *first_active_obj;
	void *pdirect_handle;
	int btn_status;

	int select_id;
	void *select_focus_btn;
	void *bg_cover_obj;
	void *reset_restore_box_obj;
	void *restore_box_obj;
	void *box_btn[2];

	int wait_img_angle;
	int btn_wait_flag;
	void *waiting_obj;
    void *time_handle;

	void *message_box_handle;
	Pdivoom_sys_setting_info psys_config;
}divoom_lvgl_system_setting_ctrl, *Pdivoom_lvgl_system_setting_ctrl;

typedef struct _divoom_lvgl_system_setting_menu_item
{
    int font_size;
    char *setting_str;
}divoom_lvgl_system_setting_menu_item, *Pdivoom_lvgl_system_setting_menu_item;

static const divoom_lvgl_system_setting_menu_item pdivoom_lvgl_setting_list[] =
{
	{32, "SystemSetting"},  
	{24, "Display"},
	{26, "SlideSetting"},
	//{26, "PowerOnOffTime"},
	{26, "NightViewMode"},
	{26, "ColorTemp"},
	{24, "VoiceNotice"},
	{24, "Network"},
	{26, "NetworkConnect"},
	{24, "Options"},
	{26, "DateTime"},
	{26, "WindSpeedUnit"},
	{26, "m/s"},
	{26, "Temperature"},
	{26, "Centigrade"},
	{26, "Language"},
	{26, "PcClockMcpTool"},
	{26, "ShowGrid"},
	{24, "SystemManage"},
	{26, "About"},
	{26, "Reset all network and settings"},
	{26, "Factory Restore"},
	{32, "Reset all network and settings"},
	{26, "ResetNetworkConfirm"},
	{32, "Cancel"},
	{32, "Ok"},
};


void divoom_lvgl_system_setting_create_wind_speed_setting_callback(void *handle, int ctrl_id)
{
    Pdivoom_lvgl_system_setting_ctrl phandle = (Pdivoom_lvgl_system_setting_ctrl)handle;
    
    if(ctrl_id < 0)
    {
    }
    else 
    {
        phandle->psys_config->wind_speed_flag = ctrl_id;
		if(phandle->psys_config->wind_speed_flag == 0)
		{
			lv_label_set_text(phandle->lang_label_obj_tbl[11], divoom_menu_control_get_disp_str("m/s"));
		}
		else if(phandle->psys_config->wind_speed_flag == 1)
		{
			lv_label_set_text(phandle->lang_label_obj_tbl[11], divoom_menu_control_get_disp_str("km/h"));
		}
		else if(phandle->psys_config->wind_speed_flag == 2)
		{
			lv_label_set_text(phandle->lang_label_obj_tbl[11], divoom_menu_control_get_disp_str("mph"));
		}
		else
		{
			lv_label_set_text(phandle->lang_label_obj_tbl[11], divoom_menu_control_get_disp_str("kt"));
		}
		divoom_main_send_sys_settings_config();
        divoom_main_save_config();
    }
    lv_group_focus_obj(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIND_SPEED_BTN]);
    divoom_lvgl_message_box_destroy(phandle->message_box_handle, 0);
    
    phandle->message_box_handle = NULL;
}


void divoom_lvgl_system_setting_create_wind_speed_setting(Pdivoom_lvgl_system_setting_ctrl phandle)
{
    phandle->message_box_handle = divoom_lvgl_message_box_init(phandle->menu_id, phandle->main_obj, phandle, 
        divoom_lvgl_system_setting_create_wind_speed_setting_callback, 80, 415, 640, 504, NULL, lv_color_hex(0xF2F2F2), 4);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("m/s"), 26,lv_color_hex(0xF2F2F2), lv_color_hex(0),lv_color_hex(0), 0, 
        32, 32, 576, 98);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("km/h"), 26, lv_color_hex(0xF2F2F2),lv_color_hex(0),lv_color_hex(0), 1, 
        32, 146, 576, 98);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("mph"), 26,lv_color_hex(0xF2F2F2), lv_color_hex(0),lv_color_hex(0), 2, 
        32, 260, 576, 98);
	divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("kt"), 26,lv_color_hex(0xF2F2F2), lv_color_hex(0),lv_color_hex(0), 3, 
        32, 374, 576, 98);
    divoom_lvgl_message_box_set_active_item(phandle->message_box_handle, phandle->psys_config->wind_speed_flag);
}


static void divoom_lvgl_system_setting_main_event_cb(lv_event_t * e)
{
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_system_setting_ctrl phandle = (Pdivoom_lvgl_system_setting_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    //if(code == LV_EVENT_CLICKED)
    if(code == LV_EVENT_KEY)
    {
    	int i, value, active_id;
		char c = *((char *)lv_event_get_param(e));
		char *key_info = (char *)lv_event_get_param(e);
		
        Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();
        divoom_system_setting_birghtness_conf brightness;
		
        if(phandle == NULL || phandle->btn_wait_flag == 1)
        { 	
            return;
        }
        switch(c)
        {
        	case DIVOOM_LVGL_KEY_PRE:
			case DIVOOM_LVGL_KEY_NEXT:
				if(phandle->btn_status == 0)
				{
					cur_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_obj, c == DIVOOM_LVGL_KEY_NEXT?DIVOOM_MENU_DIRECT_UP:DIVOOM_MENU_DIRECT_DOWN);                            
                	lv_group_focus_obj(cur_obj);
				}
				else
				{
					active_id = divoom_menu_direct_ctrl_get_pos(phandle->pdirect_handle, cur_obj);
					if(active_id == 0)
					{
						value = lv_slider_get_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN]);
						if(c == DIVOOM_LVGL_KEY_NEXT)
						{
                            #ifdef DIVOOM_TIME_FRAME_SUPPORT_LOW_LIGHT
                            if(value <= DIVOOM_SYS_BRIGHT_ON_FLOOR)
                            {
                                value = 0;
                            }
                            else  if(value)
                            {
							    value = (value > 5) ? (value - 5) : (DIVOOM_SYS_BRIGHT_ON_FLOOR);
                                if(value <= DIVOOM_SYS_BRIGHT_ON_FLOOR)
                                {
                                    value = DIVOOM_SYS_BRIGHT_ON_FLOOR;
                                }
                            }
                            #else
							if(value > 15)
							{
								value = value - 5;
							}
							else if(value > 10)
							{
								value = 10;
							}
							else //if(value <= 10)
							{
								value = 0;
							}
                            #endif
						}
						else
						{
                            #ifdef DIVOOM_TIME_FRAME_SUPPORT_LOW_LIGHT
                            if(value == 0)
                            {
                                value = DIVOOM_SYS_BRIGHT_ON_FLOOR;
                            }
                            else if(value == DIVOOM_SYS_BRIGHT_ON_FLOOR)
                            {
                                value = DIVOOM_SYS_BRIGHT_ON_FLOOR + 5;
                            }
                            else  if(value)
                            {
							    value = (value < 95) ? (value + 5) : (100);
                            }
                            #else
							if(value < 10)
							{
								value = 10;
                            }
							else
							{
								value = (value < 95) ? (value + 5) : (100);
							}
							#endif
						}
						lv_slider_set_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN], value, LV_ANIM_ON);
                        divoom_main_set_sys_light(value);
						brightness.light = value;
						divoom_net_app_send_packet(DIVOOM_NET_COMM_SET_SYS_BRIGHTNESS, DIVOOM_SERVER_TOPIC, &brightness);
					}
					else if(active_id == 4)
					{
						value = lv_slider_get_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN]);
						if(c == DIVOOM_LVGL_KEY_NEXT)
						{
							value = (value > 5) ? (value - 5) : (0);
						}
						else
						{
							value = (value < 95) ? (value + 5) : (100);
						}
						lv_slider_set_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN], value, LV_ANIM_ON);
						psys_config->volume = value;
						divoom_main_send_sys_settings_config();
						divoom_main_save_config();
					}
				}
				break;
			case DIVOOM_LVGL_KEY_ENTER:
				active_id = divoom_menu_direct_ctrl_get_pos(phandle->pdirect_handle, cur_obj);
				
				if(phandle->btn_status == 0)
				{
					if(active_id == 0)
					{
						phandle->btn_status = 1;
						lv_obj_add_style(cur_obj, &phandle->btn_edit_style, LV_STATE_FOCUSED);
						lv_obj_add_style(cur_obj, &phandle->btn_edit_style, LV_STATE_FOCUS_KEY);
					}
					else if(active_id == 4)
					{
						phandle->btn_status = 1;
						lv_obj_add_style(cur_obj, &phandle->btn_edit_style, LV_STATE_FOCUSED);
						lv_obj_add_style(cur_obj, &phandle->btn_edit_style, LV_STATE_FOCUS_KEY);
					}
					else
					{
						switch(active_id)
                		{
                    	 	case DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN:
                        		value = lv_slider_get_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN]);
								value = (value < 95) ? (value + 5) : (100);
								lv_slider_set_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN], value, LV_ANIM_ON);
                        		divoom_main_set_sys_light(value);
								brightness.light = value;
								divoom_net_app_send_packet(DIVOOM_NET_COMM_SET_SYS_BRIGHTNESS, DIVOOM_SERVER_TOPIC, &brightness);
								break;
                        	
                   	 	    case DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN:
                     	   		value = lv_slider_get_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN]);
								value = (value < 95) ? (value + 5) : (100);
								lv_slider_set_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN], value, LV_ANIM_ON);
								psys_config->volume = value;
								divoom_main_send_sys_settings_config();
								divoom_main_save_config();
                        		break;
                        
                    	/*	case DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_MEMORY_BTN:
                        		if(psys_config->volume_memory)
                        		{
                        	 	   psys_config->volume_memory = 0;
                        		    divoom_menu_control_lv_img_set_src(phandle->func_obj_tbl[i], "26/26_off.png");
                        		}
                        		else
                        		{
                        		    psys_config->volume_memory = 1;
                        		    divoom_menu_control_lv_img_set_src(phandle->func_obj_tbl[i], "26/26_on.png");
                        		}
                        		divoom_main_save_config();
                        		break;
                   		
                   			case DIVOOM_LVGL_SYSTEM_SETTING_MENU_THEME_APPEARANCE_BTN:
								divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_THEME, NULL, 0, 0);
								break;
						*/	
                   			case DIVOOM_LVGL_SYSTEM_SETTING_MENU_SLIDE_SETTING_BTN:
								divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SLIDE_SETING_MENU, NULL, 0, 0);
								break;
							
							case DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIND_SPEED_BTN:
								divoom_lvgl_system_setting_create_wind_speed_setting(phandle);
								break;
							
                    		case DIVOOM_LVGL_SYSTEM_SETTING_MENU_DATA_TIME_BTN:
                        		divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_DATE_TIME, NULL, 0, 0);
                        		break;
                        
                    		case DIVOOM_LVGL_SYSTEM_SETTING_MENU_TEMPERATURE_BTN:
                    	    	divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_TEMPERATURE, NULL, 0, 0);
                        		break;
                        
                    		case DIVOOM_LVGL_SYSTEM_SETTING_MENU_LANGUAGE_BTN:
                        		divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_LANGUGE, NULL, 0, 0);
                        		break;
                        
                    	/*	case DIVOOM_LVGL_SYSTEM_SETTING_MENU_BLUETOOTH_BTN:
                        		divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_BLUETOOTH, NULL, 0, 0);
                        		break;
                        */
                    		case DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIFI_BTN:
                        		divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_WIFI, NULL, 0, 0);
                        		break;

						/*	case DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIFI_PROPERTIES_BTN:
								divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_WIFI_PROPERTIES, NULL, 0, 0);
								break;
						*/	
                    		case DIVOOM_LVGL_SYSTEM_SETTING_MENU_NIGHTVIEW_BTN:
                        		divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_NIGHTVIEW, NULL, 0, 0);
                        		break;

							case DIVOOM_LVGL_SYSTEM_SETTING_MENU_COLOR_TEMP_BTN:
                        		divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_COLOR_TEMP, NULL, 0, 0);
                        		break;
                        
                    	/*	case DIVOOM_LVGL_SYSTEM_SETTING_MENU_SCREEN_SAVE_BTN:
                        		divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_SCREEN_SAVE, NULL, 0, 0);
                        		break;
                        
                    		case DIVOOM_LVGL_SYSTEM_SETTING_MENU_IDLE_POWER_OFF_BTN:
                        		divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_IDLE_POWER_OFF_TIME, NULL, 0, 0);                        
                        		break;
                        
                    		case DIVOOM_LVGL_SYSTEM_SETTING_MENU_POWER_ON_OFF_TIME_BTN:
								divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_POWER_OFF_TIME, NULL, 0, 0);
                        		break;
                        
                    		case DIVOOM_LVGL_SYSTEM_SETTING_MENU_MIRROR_BTN:
                        		if(psys_config->mirror_flag)
                        		{
                         	   		psys_config->mirror_flag = 0;
                         	 	  	divoom_menu_control_lv_img_set_src(phandle->func_obj_tbl[active_id], "26/26_off.png");
                        		}
                        		else
                        		{
                        	    	psys_config->mirror_flag = 1;
                        	    	divoom_menu_control_lv_img_set_src(phandle->func_obj_tbl[active_id], "26/26_on.png");
                        		}
								divoom_main_send_sys_settings_config();
                        		divoom_main_save_config();
                        		break;
                        */ 	
                    		case DIVOOM_LVGL_SYSTEM_SETTING_MENU_MCP_TOOL_BTN:
                        		divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_MCP_TOOL, NULL, 0, 0);
                        		break;

                        	case DIVOOM_LVGL_SYSTEM_SETTING_MENU_SHOW_GRID_BTN:
								if(psys_config->show_grid_flag)
                        		{
                         	   		psys_config->show_grid_flag = 0;
                         	 	  	divoom_menu_control_lv_img_set_src(phandle->func_obj_tbl[active_id], "26/26_off.png");
                        		}
                        		else
                        		{
                        	    	psys_config->show_grid_flag = 1;
                        	    	divoom_menu_control_lv_img_set_src(phandle->func_obj_tbl[active_id], "26/26_on.png");
                        		}
								divoom_main_send_sys_settings_config();
                        		divoom_main_save_config();
								break;
                    		case DIVOOM_LVGL_SYSTEM_SETTING_MENU_ABOUT_BTN:
                    			{    
                        			divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_ABOUT, NULL, 0, 0);
                    			}
                        		break;
                        
                    		case DIVOOM_LVGL_SYSTEM_SETTING_MENU_RESTORE_BTN:
								//divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_RESTORE, NULL, 0, 0);
								lv_label_set_text(phandle->lang_label_obj_tbl[20], divoom_menu_control_get_disp_str("Factory Restore"));
								lv_label_set_text(phandle->lang_label_obj_tbl[21], divoom_menu_control_get_disp_str("FactoryRestoreComfirm"));
								lv_label_set_text(phandle->lang_label_obj_tbl[22], divoom_menu_control_get_disp_str("Cancel"));
								lv_label_set_text(phandle->lang_label_obj_tbl[23], divoom_menu_control_get_disp_str("Ok"));
								lv_group_focus_obj(phandle->box_btn[DIVOOM_LVGL_SELECT_BOX_BTN_CANCEL]);
								phandle->select_focus_btn = cur_obj;
								phandle->select_id = 2;
								lv_obj_clear_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);
								lv_obj_clear_flag(phandle->reset_restore_box_obj, LV_OBJ_FLAG_HIDDEN);
								break;
							case DIVOOM_LVGL_SYSTEM_SETTING_MENU_RESET_ALL_BTN:
								lv_label_set_text(phandle->lang_label_obj_tbl[20], divoom_menu_control_get_disp_str("Reset all network and settings"));
								lv_label_set_text(phandle->lang_label_obj_tbl[21], divoom_menu_control_get_disp_str("ResetNetworkConfirm"));
								lv_label_set_text(phandle->lang_label_obj_tbl[22], divoom_menu_control_get_disp_str("Cancel"));
								lv_label_set_text(phandle->lang_label_obj_tbl[23], divoom_menu_control_get_disp_str("Ok"));
								lv_group_focus_obj(phandle->box_btn[DIVOOM_LVGL_SELECT_BOX_BTN_CANCEL]);
								phandle->select_focus_btn = cur_obj;
								phandle->select_id = 1;
								lv_obj_clear_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);
								lv_obj_clear_flag(phandle->reset_restore_box_obj, LV_OBJ_FLAG_HIDDEN);
                        		break;
                        	default:
								break;
                		}
					}
				}
				else
				{
					/*if(active_id == 0)
					{
						value = lv_slider_get_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN]);
						value = (value < 95) ? (value + 5) : (100);
						lv_slider_set_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN], value, LV_ANIM_ON);
                        divoom_main_set_sys_light(value);
						brightness.light = value;
						divoom_net_app_send_packet(DIVOOM_NET_COMM_SET_SYS_BRIGHTNESS, DIVOOM_SERVER_TOPIC, &brightness);
					}
					else if(active_id == 3)
					{
						value = lv_slider_get_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN]);
						value = (value < 95) ? (value + 5) : (100);
						lv_slider_set_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN], value, LV_ANIM_ON);
						psys_config->volume = value;
						divoom_main_send_sys_settings_config();
						divoom_main_save_config();
					}*/
				}
        		break;
				
        	case DIVOOM_LVGL_KEY_ESC:
				if(phandle->btn_status == 0)
				{
                    divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_EXIT_MENU_SETTING);
				}
				else if(phandle->btn_status == 1)
				{
					phandle->btn_status = 0;
					lv_obj_add_style(cur_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
					lv_obj_add_style(cur_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
				}
				break;
				
			default:
				divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
				break;
       }
    }
}

void divoom_lvgl_system_setting_waiting_anim_exec_cb(void * var, int32_t v) 
{
    Pdivoom_lvgl_system_setting_ctrl phandle = (Pdivoom_lvgl_system_setting_ctrl)var; 

	if(phandle == NULL)
	{
		return;
	}
	
	phandle->wait_img_angle += v;

	if(phandle->wait_img_angle >= 3600)
	{
		phandle->wait_img_angle = 0;
	}

    lv_img_set_angle(phandle->waiting_obj, phandle->wait_img_angle);        // 设置图像角度
}


void divoom_lvgl_system_setting_start_waiting_anim(Pdivoom_lvgl_system_setting_ctrl phandle)
{
	lv_anim_t anim;

	lv_anim_init(&anim);
	lv_anim_set_var(&anim, phandle);
 	lv_anim_set_exec_cb(&anim, divoom_lvgl_system_setting_waiting_anim_exec_cb);
 	lv_anim_set_time(&anim, 175);
 	lv_anim_set_values(&anim, 0, 450);			//45度
	lv_anim_set_path_cb(&anim, lv_anim_path_step);
	lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);

	lv_obj_clear_flag(phandle->waiting_obj, LV_OBJ_FLAG_HIDDEN);

	lv_anim_start(&anim);
}

static void divoom_lvgl_system_setting_select_box_event_cb(lv_event_t * e)
{
	lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_system_setting_ctrl phandle = (Pdivoom_lvgl_system_setting_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);

    if(code == LV_EVENT_KEY)
    {
		char c = *((char *)lv_event_get_param(e));

		switch(c)
		{
			case DIVOOM_LVGL_KEY_PRE:
			case DIVOOM_LVGL_KEY_NEXT:
				if(cur_obj == phandle->box_btn[DIVOOM_LVGL_SELECT_BOX_BTN_CANCEL])
				{
					lv_group_focus_obj(phandle->box_btn[DIVOOM_LVGL_SELECT_BOX_BTN_OK]);
				}
				else
				{
					lv_group_focus_obj(phandle->box_btn[DIVOOM_LVGL_SELECT_BOX_BTN_CANCEL]);
				}
				break;
			case DIVOOM_LVGL_KEY_ENTER:
				if(cur_obj == phandle->box_btn[DIVOOM_LVGL_SELECT_BOX_BTN_CANCEL])//取消
				{
					lv_obj_add_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);
					lv_obj_add_flag(phandle->reset_restore_box_obj, LV_OBJ_FLAG_HIDDEN);
				}
				else//确定
				{
					int event_info[] = {DIVOOM_MAIN_EVENT_CONFIG_NET_WORK, DIVOOM_MAIN_EVENT_FORMAT_USERDATA, DIVOOM_MAIN_EVENT_RESET_FACTORY_SETTING};
					if(phandle->select_id == 1)
					{
						divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_CONFIG_NET_WORK);
						lv_obj_add_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);
					}
					else if(phandle->select_id == 2)
					{
						divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_RESET_FACTORY_SETTING);
						phandle->btn_wait_flag = 1;
						divoom_lvgl_system_setting_start_waiting_anim(phandle);
					}
					
					lv_obj_add_flag(phandle->reset_restore_box_obj, LV_OBJ_FLAG_HIDDEN);
				}
				lv_group_focus_obj(phandle->select_focus_btn);
				phandle->select_focus_btn = NULL;
				break;
			case DIVOOM_LVGL_KEY_ESC:
				lv_obj_add_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);
				lv_obj_add_flag(phandle->reset_restore_box_obj, LV_OBJ_FLAG_HIDDEN);
				lv_group_focus_obj(phandle->select_focus_btn);
				phandle->select_focus_btn = NULL;
				break;
			default:
				break;
		}
    }
}


void divoom_lvgl_system_setting_refresh_brightness(struct _lv_timer_t *userdata)
{
    Pdivoom_lvgl_system_setting_ctrl phandle = (Pdivoom_lvgl_system_setting_ctrl)lv_timer_get_user_data(userdata);

    
	if(lv_slider_get_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN]) != divoom_main_get_sys_light())
	{
        lv_slider_set_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN], divoom_main_get_sys_light(), LV_ANIM_ON);
	}
}

void *divoom_lvgl_system_setting_init(void *user_data, int menu_id)
{
	int index = 0;
    void *image_obj, *label_obj, *slide_obj, *btn_obj;
    Pdivoom_lvgl_system_setting_ctrl phandle;
    Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();
	
    phandle = divoom_system_malloc(sizeof(divoom_lvgl_system_setting_ctrl));
    phandle->menu_id = menu_id;
    phandle->lang = psys_config->lang;
	phandle->pdirect_handle = divoom_menu_direct_ctrl_create(1, 14);
	phandle->first_active_obj = NULL;
	phandle->psys_config = divoom_main_get_system_config();

    divoom_menu_control_init_lable_style(&phandle->normal_style);
    phandle->main_obj = lv_obj_create(NULL);    
    lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(0xE4E4E4), LV_PART_MAIN);
    lv_obj_set_pos(phandle->main_obj, 0, 0);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_add_style(phandle->main_obj, &phandle->normal_style, 0);
    lv_obj_set_scrollbar_mode(phandle->main_obj, LV_SCROLLBAR_MODE_OFF);
    //lv_obj_set_scroll_dir(phandle->main_obj, LV_DIR_VER);
	lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
    //lv_obj_set_style_bg_opa(phandle->main_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
    
	lv_style_init(&phandle->btn_normal_style);
	lv_style_set_bg_color(&phandle->btn_normal_style, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->btn_normal_style, 0);
	lv_style_set_border_width(&phandle->btn_normal_style, 4);
	lv_style_set_border_color(&phandle->btn_normal_style, lv_color_hex(0xE4E4E4));
	lv_style_set_radius(&phandle->btn_normal_style, 17);
	lv_style_set_shadow_width(&phandle->btn_normal_style, 0);
	lv_style_set_outline_width(&phandle->btn_normal_style, 0);

	lv_style_init(&phandle->btn_select_style);
	lv_style_set_bg_color(&phandle->btn_select_style, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->btn_select_style, 0);
	lv_style_set_border_width(&phandle->btn_select_style, 4);
	lv_style_set_border_color(&phandle->btn_select_style, lv_color_hex(0x6D6D6D));
	lv_style_set_radius(&phandle->btn_select_style, 17);
	lv_style_set_shadow_width(&phandle->btn_select_style, 0);
	lv_style_set_outline_width(&phandle->btn_select_style, 0);

	lv_style_init(&phandle->btn_edit_style);
	lv_style_set_bg_color(&phandle->btn_edit_style, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->btn_edit_style, 0);
	lv_style_set_border_width(&phandle->btn_edit_style, 4);
	lv_style_set_border_color(&phandle->btn_edit_style, lv_color_hex(0xE89B00));
	lv_style_set_radius(&phandle->btn_edit_style, 17);
	lv_style_set_shadow_width(&phandle->btn_edit_style, 0);
	lv_style_set_outline_width(&phandle->btn_edit_style, 0);
	
    label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 100, 50);
	lv_obj_set_size(label_obj, 600, 78);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0);  
    index++;
	
    /*label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 74, 168);
	lv_obj_set_size(label_obj, 600, 32);
    phandle->lang_label_obj_tbl[1] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[1], divoom_menu_control_get_disp_str("ScreenBright"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X9E9E9E), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);  
      
    image_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(image_obj, 37, 209);
    lv_obj_set_size(image_obj, 726, 90);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_1line_bg.png");*/

	lv_obj_t *cont_obj = lv_obj_create(phandle->main_obj);
	lv_obj_remove_style(cont_obj, NULL, LV_PART_SCROLLBAR);
    lv_obj_set_style_pad_all(cont_obj, 0, 0);
    lv_obj_set_style_border_width(cont_obj, 0, 0);
    lv_obj_set_style_outline_width(cont_obj, 0, 0);
    lv_obj_set_style_border_side(cont_obj, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_style_bg_color(cont_obj, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_pad_bottom(cont_obj, 44, 0);
    lv_obj_set_size(cont_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH - 128);
    lv_obj_set_pos(cont_obj, 0, 128);

	//显示
	label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 32);
	lv_obj_set_size(label_obj, 310, 32);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x636363), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN] = label_obj;
	index++;
	
	//亮度
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 76);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 0);
	if(phandle->first_active_obj == NULL)
	{
		phandle->first_active_obj = btn_obj;
	}
	
	image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 76, 103);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_birght_min.png");
    
    
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 103);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_birght_max.png");
    
    
    
    slide_obj = lv_slider_create(cont_obj);
    lv_slider_set_range(slide_obj, 0, 100);
    lv_obj_set_pos(slide_obj, 133, 121);
    lv_obj_set_size(slide_obj, 530, 4);
    lv_slider_set_value(slide_obj, psys_config->brightness, LV_ANIM_ON);
    lv_obj_set_style_bg_color(slide_obj, lv_color_hex(0x707070), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slide_obj, lv_color_hex(0xB1B1B1), LV_PART_MAIN);
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN] = slide_obj;
    

    
    /*label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 74, 339);
	lv_obj_set_size(label_obj, 600, 32);
    phandle->lang_label_obj_tbl[2] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[2], divoom_menu_control_get_disp_str("VoiceNotice"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X9E9E9E), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(24, phandle->menu_id), 0); 
    
    
    image_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(image_obj, 37, 377);
    lv_obj_set_size(image_obj, 726, 180);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_2line_bg.png");
    
    
    image_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(image_obj, 37, 377);
    lv_obj_set_size(image_obj, 90, 90);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_volume_min.png");
    
    image_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(image_obj, 37, 673);
    lv_obj_set_size(image_obj, 90, 90);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_volume_max.png");
    
    
    slide_obj = lv_slider_create(phandle->main_obj);
    lv_slider_set_range(slide_obj, 0, 100);
    lv_obj_set_pos(slide_obj, 135, 422);
    lv_obj_set_size(slide_obj, 530, 4);
    lv_slider_set_value(slide_obj, psys_config->volume, LV_ANIM_ON);
    lv_obj_set_style_bg_color(slide_obj, lv_color_hex(DIVOOM_MENNU_SLIDER_ACTIVE_COLOR), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slide_obj, lv_color_hex(DIVOOM_MENNU_SLIDER_UNACTIVE_COLOR), LV_PART_MAIN);
    lv_obj_add_flag(slide_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(slide_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN] = slide_obj;
	
    

    label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 74, 491);
	lv_obj_set_size(label_obj, 500, 42);
    phandle->lang_label_obj_tbl[3] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[3], divoom_menu_control_get_disp_str("Volume_memory"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_MEMORY_BTN] = label_obj;
    lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(label_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
    
    

    image_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(image_obj, 598, 467);
    lv_obj_set_size(image_obj, 165, 90);
    if(psys_config->volume_memory)
    {
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_on.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_off.png");
    }
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_MEMORY_BTN] = image_obj;
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
	
    
    image_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(image_obj, 37, 585);
    lv_obj_set_size(image_obj, 726, 450);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_5line_bg.png");
    

	//主题外观
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 302-128);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 1);

	label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 330-128);
	lv_obj_set_size(label_obj, 400, 34);
    phandle->lang_label_obj_tbl[2] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[2], divoom_menu_control_get_disp_str("ThemeAppearance"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_THEME_APPEARANCE_BTN] = label_obj;
    
	image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 329-128);
	lv_obj_set_size(image_obj, 36, 36);
	divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
	phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_THEME_APPEARANCE_BTN] = image_obj;
	*/
    //幻灯片设置
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 174);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 1);

	label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 202);
	lv_obj_set_size(label_obj, 400, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_SLIDE_SETTING_BTN] = label_obj;
    
	image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 201);
	lv_obj_set_size(image_obj, 36, 36);
	divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
	phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_SLIDE_SETTING_BTN] = image_obj;
	index++;
	
	//定时关机时间
    /*btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 272);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 2);
    

    label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 300);
	lv_obj_set_size(label_obj, 400+100, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_POWER_ON_OFF_TIME_BTN] = label_obj;
    
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 299);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_POWER_ON_OFF_TIME_BTN] = image_obj;
    index++;
	*/
	//夜览模式
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 272);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 2);
    
    label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 300);
	lv_obj_set_size(label_obj, 400, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_NIGHTVIEW_BTN] = label_obj;
   
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 299);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_NIGHTVIEW_BTN] = image_obj;
    index++;

	//色温调节
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 370);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 3);
    
    label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 398);
	lv_obj_set_size(label_obj, 400, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_NIGHTVIEW_BTN] = label_obj;
   
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 397);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_COLOR_TEMP_BTN] = image_obj;
    index++;

	/*
	//屏保设置
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 694-128);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 5);
		
	label_obj = lv_label_create(cont_obj);
	lv_obj_set_pos(label_obj, 76, 722-128);
	lv_obj_set_size(label_obj, 400, 34);
	phandle->lang_label_obj_tbl[6] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[6], divoom_menu_control_get_disp_str("ScreenSave"));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, phandle->menu_id), 0); 
	phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_SCREEN_SAVE_BTN] = label_obj;
	//lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
	//lv_obj_add_event_cb(label_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
	
		
	image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 721-128);
	lv_obj_set_size(image_obj, 36, 36);
	divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
	phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_SCREEN_SAVE_BTN] = image_obj;
	//lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
	//lv_obj_add_event_cb(image_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
	*/
	//系统提示音
	label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 508);
	lv_obj_set_size(label_obj, 310, 32);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x636363), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    //phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN] = label_obj;
	index++;
	//音量
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 552);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 4);
	
	image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 72, 577);
    lv_obj_set_size(image_obj, 40, 40);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_vol_min.png");
    
    
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 684, 577);
    lv_obj_set_size(image_obj, 40, 40);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_vol_max.png");
    
    
    
    slide_obj = lv_slider_create(cont_obj);
    lv_slider_set_range(slide_obj, 0, 100);
    lv_obj_set_pos(slide_obj, 148, 593);
    lv_obj_set_size(slide_obj, 509, 4);
    lv_slider_set_value(slide_obj, psys_config->volume, LV_ANIM_ON);
    lv_obj_set_style_bg_color(slide_obj, lv_color_hex(0x707070), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slide_obj, lv_color_hex(0xB1B1B1), LV_PART_MAIN);
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN] = slide_obj;
	
	//空闲关机时间
	/*btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 792-128);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 6);
		
	label_obj = lv_label_create(cont_obj);
	lv_obj_set_pos(label_obj, 76, 820-128);
	lv_obj_set_size(label_obj, 400, 34);
	phandle->lang_label_obj_tbl[7] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[7], divoom_menu_control_get_disp_str("IdlePowerOffTime"));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, phandle->menu_id), 0); 
	phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_IDLE_POWER_OFF_BTN] = label_obj;
	//lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
	//lv_obj_add_event_cb(label_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
		
	image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 819-128);
	lv_obj_set_size(image_obj, 36, 36);
	divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
	phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_IDLE_POWER_OFF_BTN] = image_obj;
	//lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
	//lv_obj_add_event_cb(image_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
	*/

	//网络
	label_obj = lv_label_create(cont_obj);
	lv_obj_set_pos(label_obj, 76, 690);
	lv_obj_set_size(label_obj, 310, 32);
	phandle->lang_label_obj_tbl[index] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0x636363), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
	index++;

	
	//网络连接
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 734);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 5);
		
	label_obj = lv_label_create(cont_obj);
	lv_obj_set_pos(label_obj, 76, 762);
	lv_obj_set_size(label_obj, 400, 34);
	phandle->lang_label_obj_tbl[index] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
	phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIFI_BTN] = label_obj;
		
	image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 761);
	lv_obj_set_size(image_obj, 36, 36);
	divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
	phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIFI_BTN] = image_obj;
	index++;
	
	//偏好
	label_obj = lv_label_create(cont_obj);
	lv_obj_set_pos(label_obj, 76, 872);
	lv_obj_set_size(label_obj, 310, 32);
	phandle->lang_label_obj_tbl[index] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0x636363), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
	index++;
	
    /*image_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(image_obj, 37, 1058);
    lv_obj_set_size(image_obj, 726, 450);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_5line_bg.png");
	
	label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 74, 880);
	lv_obj_set_size(label_obj, 400, 42);
    phandle->lang_label_obj_tbl[8] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[8], divoom_menu_control_get_disp_str("Bluetooth"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BLUETOOTH_BTN] = label_obj;
    lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(label_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
    
    image_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(image_obj, 673, 855);
    lv_obj_set_size(image_obj, 90, 90);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BLUETOOTH_BTN] = image_obj;
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
	
	
	//网络属性
    btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 1156-128);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 8);
	
	label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 1184-128);
	lv_obj_set_size(label_obj, 400, 34);
    phandle->lang_label_obj_tbl[10] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[10], divoom_menu_control_get_disp_str("NetworkProperties"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIFI_PROPERTIES_BTN] = label_obj;
    //lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_add_event_cb(label_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
    
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 1184-128);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIFI_PROPERTIES_BTN] = image_obj;
    */

	//时间与日期
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 916);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 6);
	
    label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 944);
	lv_obj_set_size(label_obj, 400, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_DATA_TIME_BTN] = label_obj;
    
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 943);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_DATA_TIME_BTN] = image_obj;
    index++;
	
	//风速单位
    btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 1014);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 7);
	
	label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 1042);
	lv_obj_set_size(label_obj, 600, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIND_SPEED_BTN] = label_obj;
    //lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_add_event_cb(label_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
    index++;

	label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 600-100, 1042);
	lv_obj_set_size(label_obj, 198, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    if(psys_config->wind_speed_flag == 0)
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str("m/s"));
    }
    else if(psys_config->wind_speed_flag == 1)
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str("km/h"));
    }
	else if(psys_config->wind_speed_flag == 2)
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str("mph"));
    }
	else
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str("kt"));
    }

    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X616161), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->image_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIND_SPEED_BTN] = label_obj;

    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 1042);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_WIND_SPEED_BTN] = btn_obj;
    index++;

	//温度
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 1112);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 8);
	
    label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 1140);
	lv_obj_set_size(label_obj, 400, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_TEMPERATURE_BTN] = label_obj;
    index++;
	
    label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 600-100, 1140);
	lv_obj_set_size(label_obj, 198, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    if(psys_config->temp_c_f_flag)
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str("Fahrenheit"));
    }
    else
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str("Centigrade"));
    }
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X616161), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->image_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_TEMPERATURE_BTN] = label_obj;

    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 1139);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_TEMPERATURE_BTN] = image_obj;
    index++;
    
	//设备语言
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 1210);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 9);
	
    label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 1238);
	lv_obj_set_size(label_obj, 400, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_LANGUAGE_BTN] = label_obj;
    
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 1237);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_LANGUAGE_BTN] = image_obj;
    index++;

	//PC表盘创作&MCP
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 1308);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 10);

	label_obj = lv_label_create(cont_obj);
	lv_obj_set_pos(label_obj, 76, 1336);
	lv_obj_set_size(label_obj, 600, 34);
	phandle->lang_label_obj_tbl[index] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0);
	phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_MCP_TOOL_BTN] = label_obj;

	image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 1335);
	lv_obj_set_size(image_obj, 36, 36);
	divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
	phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_MCP_TOOL_BTN] = btn_obj;
	index++;

	//显示像素作品网格
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 1406);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 11);
	
    label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 1434);
	lv_obj_set_size(label_obj, 400, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_SHOW_GRID_BTN] = label_obj;
    
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 652, 1429);
    lv_obj_set_size(image_obj, 72, 44);
	if(psys_config->show_grid_flag)
    {
    	divoom_menu_control_lv_img_set_src(image_obj, "26/26_on.png");
    }
	else
	{
		divoom_menu_control_lv_img_set_src(image_obj, "26/26_off.png");
	}
	phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_SHOW_GRID_BTN] = image_obj;
    index++;

	//系统管理
	label_obj = lv_label_create(cont_obj);
	lv_obj_set_pos(label_obj, 76, 1574-128);
	lv_obj_set_size(label_obj, 310, 32);
	phandle->lang_label_obj_tbl[index] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0X636363), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
	index++;
	
	/*//控制
	label_obj = lv_label_create(cont_obj);
	lv_obj_set_pos(label_obj, 76, 1588-128);
	lv_obj_set_size(label_obj, 310, 32);
	phandle->lang_label_obj_tbl[15] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[15], divoom_menu_control_get_disp_str("Control"));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0X999999), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(24, phandle->menu_id), 0); 
	//phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_PREFERENCE_BTN] = label_obj;
	

    //镜像翻转
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 1548-128);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 12);
	
    label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 1564-128);
	lv_obj_set_size(label_obj, 400, 34);
    phandle->lang_label_obj_tbl[16] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[16], divoom_menu_control_get_disp_str("Mirror"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, phandle->menu_id), 0); 

	label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 1600-128);
	lv_obj_set_size(label_obj, 700, 22);
    phandle->lang_label_obj_tbl[17] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[17], divoom_menu_control_get_disp_str("MirrorPurpose"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0XAAAAAA), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(16, phandle->menu_id), 0); 
    
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 652, 1571-128);
    lv_obj_set_size(image_obj, 72, 44);
    if(psys_config->mirror_flag)
    {
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_on.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_off.png");
    }
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_MIRROR_BTN] = image_obj;
    //lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_add_event_cb(image_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
	*/
    
    //image_obj = lv_img_create(phandle->main_obj);
	//lv_obj_set_pos(image_obj, 37, 1534);
    //lv_obj_set_size(image_obj, 726, 90);
    //divoom_menu_control_lv_img_set_src(image_obj, "26/26_1line_bg.png");
    
	//关于
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 1618-128);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 11);
	
    label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 1646-128);
	lv_obj_set_size(label_obj, 400, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_ABOUT_BTN] = label_obj;
    //lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_add_event_cb(label_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_CLICKED, phandle);
    
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 1645-128);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_ABOUT_BTN] = image_obj;
    index++;
    
    /*image_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(image_obj, 37, 1650);
    lv_obj_set_size(image_obj, 726, 90);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_1line_bg.png");

	//重置
	label_obj = lv_label_create(cont_obj);
	lv_obj_set_pos(label_obj, 76, 1770-128);
	lv_obj_set_size(label_obj, 310, 32);
	phandle->lang_label_obj_tbl[19] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[19], divoom_menu_control_get_disp_str("ResetSystem"));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0X999999), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(24, phandle->menu_id), 0); 
	//phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_PREFERENCE_BTN] = label_obj;
	*/
	
	//重置设置和网络
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 1716-128);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 12);
	
    label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 1744-128);
	lv_obj_set_size(label_obj, 600, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_RESET_ALL_BTN] = label_obj;
    
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 1743-128);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_RESET_ALL_BTN] = image_obj;
    index++;
	
    //清除内置储存数据
	btn_obj = lv_btn_create(cont_obj);
	lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_size(btn_obj, 712, 90);
	lv_obj_set_pos(btn_obj, 44, 1814-128);
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_main_event_cb, LV_EVENT_KEY, phandle);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, 13);
	
    label_obj = lv_label_create(cont_obj);
    lv_obj_set_pos(label_obj, 76, 1842-128);
	lv_obj_set_size(label_obj, 400+100, 34);
    phandle->lang_label_obj_tbl[index] = label_obj;
    lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
    phandle->label_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_RESTORE_BTN] = label_obj;
    
    image_obj = lv_img_create(cont_obj);
	lv_obj_set_pos(image_obj, 688, 1841-128);
    lv_obj_set_size(image_obj, 36, 36);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_RESTORE_BTN] = image_obj;
    index++;
	
	//重置设置和网络弹窗
	image_obj = lv_img_create(phandle->main_obj);
	divoom_menu_control_lv_img_set_src(image_obj, "26/26_bg_cover.png");
	lv_obj_set_size(image_obj, 800, 1280);//2060);
	lv_obj_add_flag(image_obj, LV_OBJ_FLAG_HIDDEN);
	phandle->bg_cover_obj = image_obj;
	
	image_obj = lv_img_create(phandle->main_obj);
	divoom_menu_control_lv_img_set_src(image_obj, "26/26_select_box.png");
	lv_obj_set_pos(image_obj, 80, 320);//+800);
	lv_obj_add_flag(image_obj, LV_OBJ_FLAG_HIDDEN);
	phandle->reset_restore_box_obj = image_obj;

	label_obj = lv_label_create(image_obj);
    lv_obj_set_pos(label_obj, 0, 23);
	lv_obj_set_size(label_obj, 640, 44);
	phandle->lang_label_obj_tbl[index] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0); 
	index++;
	
	label_obj = lv_label_create(image_obj);
	lv_obj_set_pos(label_obj, 20, 90+10);
	lv_obj_set_size(label_obj, 600-30, 180);
	phandle->lang_label_obj_tbl[index] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0X0), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0);
	index++;
	
	//取消
	btn_obj = lv_btn_create(image_obj);
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
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_select_box_event_cb, LV_EVENT_KEY, phandle);
	phandle->box_btn[DIVOOM_LVGL_SELECT_BOX_BTN_CANCEL] = btn_obj;

	label_obj = lv_label_create(btn_obj);
	lv_obj_set_pos(label_obj, 0, 24);
	lv_obj_set_size(label_obj, 282, 42);
	phandle->lang_label_obj_tbl[index] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
	//lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0);
	index++;
	
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
	lv_obj_add_event_cb(btn_obj, divoom_lvgl_system_setting_select_box_event_cb, LV_EVENT_KEY, phandle);
	phandle->box_btn[DIVOOM_LVGL_SELECT_BOX_BTN_OK] = btn_obj;

	label_obj = lv_label_create(btn_obj);
	lv_obj_set_pos(label_obj, 0, 24);
	lv_obj_set_size(label_obj, 282, 42);
	phandle->lang_label_obj_tbl[index] = label_obj;
	lv_label_set_text(phandle->lang_label_obj_tbl[index], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[index].setting_str));
	//lv_obj_set_style_text_color(label_obj, lv_color_hex(0XFFFFFF), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[index].font_size, phandle->menu_id), 0);

	image_obj = lv_img_create(phandle->main_obj);
	divoom_menu_control_lv_img_set_src(image_obj, "26/26_waiting.png");
	lv_obj_set_pos(image_obj, 360, 600);
	lv_obj_add_flag(image_obj, LV_OBJ_FLAG_HIDDEN);
	phandle->waiting_obj = image_obj;
	phandle->wait_img_angle = 0;
	phandle->btn_wait_flag = 0;
#if 0
    //下面创建各个子窗口
    divoom_lvgl_system_setting_create_date_time_menu(phandle, psys_config);
    
    divoom_lvgl_system_setting_create_temperature_menu(phandle, psys_config);
    
    divoom_lvgl_system_setting_create_screen_save_menu(phandle, psys_config);
    
    divoom_lvgl_system_setting_create_language_menu(phandle, psys_config);
    
    divoom_lvgl_system_setting_create_nightview_menu(phandle, psys_config);
    
    divoom_lvgl_system_setting_create_idle_poweroff_menu(phandle, psys_config);
    
    divoom_lvgl_system_setting_create_about_menu(phandle, psys_config);
    
    #endif
    phandle->time_handle = lv_timer_create(divoom_lvgl_system_setting_refresh_brightness, 16, phandle);
    return phandle;
}

void divoom_lvgl_system_setting_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_system_setting_ctrl phandle = (Pdivoom_lvgl_system_setting_ctrl)handle;
    Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();

    if(psys_config->lang != phandle->lang)
    {
        phandle->lang = psys_config->lang;

		for(int i = 0;i < sizeof(pdivoom_lvgl_setting_list)/sizeof(pdivoom_lvgl_setting_list[0]); i++)
		{
			lv_obj_set_style_text_font(phandle->lang_label_obj_tbl[i], divoom_menu_control_get_font_info(pdivoom_lvgl_setting_list[i].font_size, phandle->menu_id), 0);  
        	lv_label_set_text(phandle->lang_label_obj_tbl[i], divoom_menu_control_get_disp_str(pdivoom_lvgl_setting_list[i].setting_str));
		}
    }
	if(psys_config->wind_speed_flag == 0)
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[11], divoom_menu_control_get_disp_str("m/s"));
    }
    else if(psys_config->wind_speed_flag == 1)
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[11], divoom_menu_control_get_disp_str("km/h"));
    }
	else if(psys_config->wind_speed_flag == 2)
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[11], divoom_menu_control_get_disp_str("mph"));
    }
	else
    {
        lv_label_set_text(phandle->lang_label_obj_tbl[11], divoom_menu_control_get_disp_str("kt"));
    }
    if(psys_config->temp_c_f_flag)
    {
		lv_label_set_text(phandle->lang_label_obj_tbl[13], divoom_menu_control_get_disp_str("Fahrenheit"));
    }
    else
    {
		lv_label_set_text(phandle->lang_label_obj_tbl[13], divoom_menu_control_get_disp_str("Centigrade"));
    }
	if(psys_config->show_grid_flag)
    {
        divoom_menu_control_lv_img_set_src(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_SHOW_GRID_BTN], "26/26_on.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_SHOW_GRID_BTN], "26/26_off.png");
    }
    lv_slider_set_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_BRIGHT_BTN], psys_config->brightness, LV_ANIM_ON);
    lv_slider_set_value(phandle->func_obj_tbl[DIVOOM_LVGL_SYSTEM_SETTING_MENU_VOLUME_BTN], psys_config->volume, LV_ANIM_ON);
	if(phandle->first_active_obj)
	{
		lv_group_focus_obj(phandle->first_active_obj);
		phandle->first_active_obj = NULL;
	}
	if(active_obj)
    {
		lv_group_focus_obj(active_obj);
	}

    lv_scr_load(phandle->main_obj);
    
}

void divoom_lvgl_system_setting_pause(void *handle)
{
}

void divoom_lvgl_system_setting_exit(void *handle)
{
    Pdivoom_lvgl_system_setting_ctrl phandle = (Pdivoom_lvgl_system_setting_ctrl)handle;

	if(phandle->btn_wait_flag)
	{
		lv_anim_del(phandle, NULL);
    }
    lv_timer_del(phandle->time_handle);
	lv_obj_del(phandle->main_obj);
    lv_style_reset(&phandle->normal_style);
	lv_style_reset(&phandle->btn_normal_style);
	lv_style_reset(&phandle->btn_edit_style);
	lv_style_reset(&phandle->btn_select_style);
	divoom_menu_direct_ctrl_destory(phandle->pdirect_handle);
    divoom_system_free(phandle);
    return ;
}
#endif


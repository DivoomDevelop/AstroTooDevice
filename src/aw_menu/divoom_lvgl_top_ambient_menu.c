#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_top_ambient_menu.h"

#define MAX_AMBIENT_EFFECT 8

typedef struct _divoom_lvgl_top_ambient_menu_ctrl
{
	void *main_obj;

	void *main_cout_obj;	//选择菜单
	void *main_label_obj;
	void *main_select_image[6];	//主菜单选择图片
	void *main_effect_image;	//主菜单灯效图片
	void *main_colormode_image;		//主菜单色彩模式图片
	int   main_id;
	int   send_flag;

	char file_name[64];

	void *screen_bright_obj;	//屏幕亮度菜单
	void *screen_arc_obj;
	void *screen_bright_label;
	void *screen_disp_label;
	int bright_select_id;

	Pdivoom_pic_phone_manage_slide_config pslide_config;

	//void *ambient_bright_obj;	//氛围灯亮度菜单
	//void *ambient_label_obj;
	//int ambient_bright;

	void *ambient_effect_obj;	//氛围灯效果菜单
	void *ambient_effect_image[8];	//灯效选中icon
	void *ambient_effect_icon[8];	//灯效icon
	void *ambient_effect_label;

	//void *colormode_obj;	//色彩模式菜单
	//void *colormode_sel_label;
	//void *colormode_desp_label;
	//void *colormode_image[2];	//和color_cyc_flag对应,0对应不循环,1对应循环
	
	void *coloradjust_obj;		//色彩调节菜单
	//void *coloradjust_wheel_obj;
	void *coloradjust_knob_obj;
	//void *coloradjust_select_label;
	void *coloradjust_color_label;
	int color_index;
	int hue_angle;
	//int key_turn_time;
	//int speed_level;
	
	divoom_channel_set_rgb_info rgb_info;

	int exit_time;
	lv_timer_t *menu_exit_timer;
}divoom_lvgl_top_ambient_menu_ctrl, *Pdivoom_lvgl_top_ambient_menu_ctrl;

Pdivoom_lvgl_top_ambient_menu_ctrl pdivoom_lvgl_top_ambient_menu_handle = NULL;

const char *divoom_top_ambient_mode_str[] = {
	"SleepMode",
	"ScreenBrightness",
	"AmbientBrightness",
	//"ColorMode",
	"VedioVolume",
	"ColorAdjustment",
	"AmbientEffect",
};

const int divoom_top_ambient_coloradjust_color_angle[] = {
	90,  72,  54,  36,  18,
	0, 	 342, 324, 306, 288,
	270, 252, 234, 216, 198,
	180, 162, 144, 126, 108,
};

const int divoom_top_ambient_coloradjust_color_hex[] = {
	0,   		0xFF0000,  0xFF4800,  0xFF9000,  0xFFD000,
	0xA8FF00, 	0x54FF00,  0x00FF00,  0x00FF54,  0x00FFA8,
	0x00FFD0,   0x00A8FF,  0x0054FF,  0x0000FF,  0x5400FF,
	0xA800FF,   0xD000FF,  0xFF00A8,  0xFF0054,  0xFFFFFF,
};

void divoom_lvgl_top_ambient_menu_del_menu(void)
{
	if(pdivoom_lvgl_top_ambient_menu_handle->menu_exit_timer)
	{
		lv_timer_del(pdivoom_lvgl_top_ambient_menu_handle->menu_exit_timer);
	}
    divoom_print("aj test!\n");
    divoom_lvgl_top_control_remove_top(pdivoom_lvgl_top_ambient_menu_handle);
	lv_obj_del_async(pdivoom_lvgl_top_ambient_menu_handle->main_obj);
	divoom_system_free(pdivoom_lvgl_top_ambient_menu_handle);
	pdivoom_lvgl_top_ambient_menu_handle = NULL;
}


void divoom_lvgl_top_ambient_menu_time_handle(lv_timer_t *exit_timer)
{
	if(pdivoom_lvgl_top_ambient_menu_handle != NULL)
	{
		pdivoom_lvgl_top_ambient_menu_handle->exit_time++;

		if(pdivoom_lvgl_top_ambient_menu_handle->exit_time >= DIVOOM_EXIT_MENU_TIME_OUT)//主菜单不变动自动退出
		{
			int screen_bright = divoom_main_get_sys_light();
			divoom_system_setting_birghtness_conf brightness;
			brightness.light = screen_bright;
		
			divoom_disp_ambient_ctrl_set_disp_info(&pdivoom_lvgl_top_ambient_menu_handle->rgb_info, 1);
			divoom_pic_phone_manage_save_slide_config(1);
			divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_SET_RGB_INFO, DIVOOM_SERVER_TOPIC, (void *)&pdivoom_lvgl_top_ambient_menu_handle->rgb_info);
			divoom_net_app_send_packet(DIVOOM_NET_COMM_SET_SYS_BRIGHTNESS, DIVOOM_SERVER_TOPIC, (void *)&brightness);
		
            divoom_print("aj test!\n");
			divoom_lvgl_top_ambient_menu_del_menu();
		}
	}
}

void divoom_lvgl_top_ambient_menu_update_ambient_effect_menu_icon(void)
{
	char image_str[48];
	
	for(int i = 0; i < MAX_AMBIENT_EFFECT; i++)
	{
		//灯效图片
		if(pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color_cyc_flag)
		{
			sprintf(image_str, "76/76_effect%d_mix.png", i);
		}
		else
		{
			sprintf(image_str, "76/76_effect%d_signal.png", i);
		}
		
		divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_icon[i], image_str);
	}
}

//主菜单回调
static void divoom_lvgl_top_ambient_menu_main_cout_event_cb(lv_event_t * e)
{
	char image_str[48];
	lv_event_code_t code = lv_event_get_code(e);
	
	if(code == LV_EVENT_KEY)
    {
		uint32 para = *((uint32 *)lv_event_get_param(e));
        char c = (para&0xff);

		if(pdivoom_lvgl_top_ambient_menu_handle == NULL)
		{
			divoom_print("test\n");
			return;
		}

		pdivoom_lvgl_top_ambient_menu_handle->exit_time = 0;
		
		if(c == DIVOOM_KEY_LIGHT_RIGHT)
		{
			int main_id = pdivoom_lvgl_top_ambient_menu_handle->main_id;
			sprintf(image_str, "76/76_mode%d_unselect.png", main_id);	//取消选中
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->main_select_image[main_id], image_str);
		
			if(pdivoom_lvgl_top_ambient_menu_handle->main_id == 0)
			{
				pdivoom_lvgl_top_ambient_menu_handle->main_id = sizeof(divoom_top_ambient_mode_str)/sizeof(divoom_top_ambient_mode_str[0]) - 1;
			}
			else
			{
				pdivoom_lvgl_top_ambient_menu_handle->main_id--;
			}
			
			main_id = pdivoom_lvgl_top_ambient_menu_handle->main_id;
			sprintf(image_str, "76/76_mode%d_select.png", main_id);		//选中
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->main_select_image[main_id], image_str);

			lv_label_set_text(pdivoom_lvgl_top_ambient_menu_handle->main_label_obj, divoom_menu_control_get_disp_str(divoom_top_ambient_mode_str[main_id]));
		}
		else if(c == DIVOOM_KEY_LIGHT_LEFT)
		{
			int main_id = pdivoom_lvgl_top_ambient_menu_handle->main_id;
			sprintf(image_str, "76/76_mode%d_unselect.png", main_id);	//取消选中
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->main_select_image[main_id], image_str);
		
			pdivoom_lvgl_top_ambient_menu_handle->main_id++;
			if(pdivoom_lvgl_top_ambient_menu_handle->main_id >= sizeof(divoom_top_ambient_mode_str)/sizeof(divoom_top_ambient_mode_str[0]))
			{
				pdivoom_lvgl_top_ambient_menu_handle->main_id = 0;
			}

			main_id = pdivoom_lvgl_top_ambient_menu_handle->main_id;
			sprintf(image_str, "76/76_mode%d_select.png", main_id);		//选中
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->main_select_image[main_id], image_str);

			lv_label_set_text(pdivoom_lvgl_top_ambient_menu_handle->main_label_obj, divoom_menu_control_get_disp_str(divoom_top_ambient_mode_str[main_id]));
		}
		else if(c == DIVOOM_KEY_LIGHT_ENTER)
		{
			lv_obj_add_flag(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj, LV_OBJ_FLAG_HIDDEN);

			if(pdivoom_lvgl_top_ambient_menu_handle->main_id != 0)
			{
				pdivoom_lvgl_top_ambient_menu_handle->send_flag = 1;
			}

			if(pdivoom_lvgl_top_ambient_menu_handle->main_id == 0)	//休眠
			{
				//divoom_sys_msg_info model_msg = {0};
			
				if(pdivoom_lvgl_top_ambient_menu_handle->send_flag)
				{
					int screen_bright = divoom_main_get_sys_light();
					divoom_system_setting_birghtness_conf brightness;
					brightness.light = screen_bright;
		
					divoom_disp_ambient_ctrl_set_disp_info(&pdivoom_lvgl_top_ambient_menu_handle->rgb_info, 1);
					divoom_pic_phone_manage_save_slide_config(1);
					divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_SET_RGB_INFO, DIVOOM_SERVER_TOPIC, (void *)&pdivoom_lvgl_top_ambient_menu_handle->rgb_info);
					divoom_net_app_send_packet(DIVOOM_NET_COMM_SET_SYS_BRIGHTNESS, DIVOOM_SERVER_TOPIC, (void *)&brightness);
				}

				//model_msg.dest_id = DIVOOM_MODEL_MAIN; 
        		//model_msg.msg_id = DIVOOM_MAIN_EVENT_POWER_OFF_DISP_MENU_ENTER;
        		//divoom_sys_model_crl_msg_push(&model_msg);

				divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_POWER_OFF_DISP_MENU_ENTER);
				
                divoom_print("aj test!\n");
				divoom_lvgl_top_ambient_menu_del_menu();
			}
			else if(pdivoom_lvgl_top_ambient_menu_handle->main_id == 1)	//屏幕亮度
			{
				int screen_bright = divoom_main_get_sys_light();
				
				lv_label_set_text(pdivoom_lvgl_top_ambient_menu_handle->screen_disp_label, divoom_menu_control_get_disp_str("ScreenBrightness"));
				lv_label_set_text_fmt(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_label, "%d%%", screen_bright);
				lv_arc_set_value(pdivoom_lvgl_top_ambient_menu_handle->screen_arc_obj, screen_bright);

				divoom_menu_control_get_img_path("76/76_bright.png", pdivoom_lvgl_top_ambient_menu_handle->file_name);
				lv_obj_set_style_bg_img_src(pdivoom_lvgl_top_ambient_menu_handle->screen_arc_obj, pdivoom_lvgl_top_ambient_menu_handle->file_name, LV_PART_KNOB);

				lv_obj_clear_flag(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, LV_OBJ_FLAG_HIDDEN);
				lv_group_focus_obj(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj);
				pdivoom_lvgl_top_ambient_menu_handle->bright_select_id = 0;
			}
			else if(pdivoom_lvgl_top_ambient_menu_handle->main_id == 2)	//氛围灯亮度
			{
				int ambient_bright = divoom_disp_ambient_ctrl_get_brightness();
				
				lv_label_set_text(pdivoom_lvgl_top_ambient_menu_handle->screen_disp_label, divoom_menu_control_get_disp_str("AmbientBrightness"));
				lv_label_set_text_fmt(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_label, "%d%%", ambient_bright);
				lv_arc_set_value(pdivoom_lvgl_top_ambient_menu_handle->screen_arc_obj, ambient_bright);

				divoom_menu_control_get_img_path("76/76_bright.png", pdivoom_lvgl_top_ambient_menu_handle->file_name);
				lv_obj_set_style_bg_img_src(pdivoom_lvgl_top_ambient_menu_handle->screen_arc_obj, pdivoom_lvgl_top_ambient_menu_handle->file_name, LV_PART_KNOB);

				lv_obj_clear_flag(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, LV_OBJ_FLAG_HIDDEN);
				lv_group_focus_obj(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj);
				pdivoom_lvgl_top_ambient_menu_handle->bright_select_id = 1;
			}
			else if(pdivoom_lvgl_top_ambient_menu_handle->main_id == 3)	//视频音量
			{
				int video_volume = pdivoom_lvgl_top_ambient_menu_handle->pslide_config->video_volume;
				
				lv_label_set_text(pdivoom_lvgl_top_ambient_menu_handle->screen_disp_label, divoom_menu_control_get_disp_str("VedioVolume"));
				lv_label_set_text_fmt(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_label, "%d", video_volume);
				lv_arc_set_value(pdivoom_lvgl_top_ambient_menu_handle->screen_arc_obj, video_volume);

				divoom_menu_control_get_img_path("76/76_video_volume.png", pdivoom_lvgl_top_ambient_menu_handle->file_name);
				lv_obj_set_style_bg_img_src(pdivoom_lvgl_top_ambient_menu_handle->screen_arc_obj, pdivoom_lvgl_top_ambient_menu_handle->file_name, LV_PART_KNOB);

				lv_obj_clear_flag(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, LV_OBJ_FLAG_HIDDEN);
				lv_group_focus_obj(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj);
				pdivoom_lvgl_top_ambient_menu_handle->bright_select_id = 2;
			}
			else if(pdivoom_lvgl_top_ambient_menu_handle->main_id == 4)	//色彩调节
			{
				lv_obj_clear_flag(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_obj, LV_OBJ_FLAG_HIDDEN);
				lv_group_focus_obj(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_obj);
			}
			else if(pdivoom_lvgl_top_ambient_menu_handle->main_id == 5)	//灯光效果
			{
				divoom_lvgl_top_ambient_menu_update_ambient_effect_menu_icon();
				
				lv_group_focus_obj(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_obj);
				lv_obj_clear_flag(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_obj, LV_OBJ_FLAG_HIDDEN);
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			int screen_bright = divoom_main_get_sys_light();
			divoom_system_setting_birghtness_conf brightness;
			brightness.light = screen_bright;
		
			divoom_disp_ambient_ctrl_set_disp_info(&pdivoom_lvgl_top_ambient_menu_handle->rgb_info, 1);
			divoom_pic_phone_manage_save_slide_config(1);
			divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_SET_RGB_INFO, DIVOOM_SERVER_TOPIC, (void *)&pdivoom_lvgl_top_ambient_menu_handle->rgb_info);
			divoom_net_app_send_packet(DIVOOM_NET_COMM_SET_SYS_BRIGHTNESS, DIVOOM_SERVER_TOPIC, (void *)&brightness);
            divoom_print("aj test!\n");

			divoom_lvgl_top_ambient_menu_del_menu();
		}
		
	}
}

//屏幕亮度氛围灯亮度回调
static void divoom_lvgl_top_ambient_menu_screen_bright_cout_event_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	
	if(code == LV_EVENT_KEY)
    {
		uint32 para = *((uint32 *)lv_event_get_param(e));
        char c = (para&0xff);

		
		if(pdivoom_lvgl_top_ambient_menu_handle == NULL)
		{
			return;
		}

		pdivoom_lvgl_top_ambient_menu_handle->exit_time = 0;	//操作退出时间清零

		if(c == DIVOOM_KEY_LIGHT_RIGHT)
		{
			int value = lv_arc_get_value(pdivoom_lvgl_top_ambient_menu_handle->screen_arc_obj);
			if(pdivoom_lvgl_top_ambient_menu_handle->bright_select_id == 0)
			{
#ifdef DIVOOM_TIME_FRAME_SUPPORT_LOW_LIGHT
                if(value <= DIVOOM_SYS_BRIGHT_ON_FLOOR)
                {
                    value = 0;
                }
                else if(value < DIVOOM_SYS_BRIGHT_ON_FLOOR + 5)
                {
                    value = DIVOOM_SYS_BRIGHT_ON_FLOOR;
                }
                else 
                {
                    value = (value > DIVOOM_SYS_BRIGHT_ON_FLOOR) ? (value - 5) : (DIVOOM_SYS_BRIGHT_ON_FLOOR);
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
                value = (value > 5) ? (value - 5) : (0);
            }
			
			lv_arc_set_value(pdivoom_lvgl_top_ambient_menu_handle->screen_arc_obj, value);
			lv_obj_invalidate(pdivoom_lvgl_top_ambient_menu_handle->screen_arc_obj);
			if(pdivoom_lvgl_top_ambient_menu_handle->bright_select_id == 0)
			{
				lv_label_set_text_fmt(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_label, "%d%%", value);
				divoom_main_set_sys_light(value);
			}
			else if(pdivoom_lvgl_top_ambient_menu_handle->bright_select_id == 1)
			{
				lv_label_set_text_fmt(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_label, "%d%%", value);
				pdivoom_lvgl_top_ambient_menu_handle->rgb_info.brightness = value;
				divoom_disp_ambient_ctrl_set_brightness((uint8 *)&value, 1);
			}
			else if(pdivoom_lvgl_top_ambient_menu_handle->bright_select_id == 2)
			{
				lv_label_set_text_fmt(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_label, "%d", value);
				pdivoom_lvgl_top_ambient_menu_handle->pslide_config->video_volume = value;
			}
		}
		else if(c == DIVOOM_KEY_LIGHT_LEFT)
		{
			int value = lv_arc_get_value(pdivoom_lvgl_top_ambient_menu_handle->screen_arc_obj);
			if(pdivoom_lvgl_top_ambient_menu_handle->bright_select_id == 0)
			{
#ifdef DIVOOM_TIME_FRAME_SUPPORT_LOW_LIGHT
                if(value < DIVOOM_SYS_BRIGHT_ON_FLOOR)
                {
                    value = DIVOOM_SYS_BRIGHT_ON_FLOOR;
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
            else
            {
			    value = (value < 95) ? (value + 5) : (100);
            }
			
			lv_arc_set_value(pdivoom_lvgl_top_ambient_menu_handle->screen_arc_obj, value);
			lv_obj_invalidate(pdivoom_lvgl_top_ambient_menu_handle->screen_arc_obj);
			if(pdivoom_lvgl_top_ambient_menu_handle->bright_select_id == 0)
			{
				lv_label_set_text_fmt(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_label, "%d%%", value);
				divoom_main_set_sys_light(value);
			}
			else if(pdivoom_lvgl_top_ambient_menu_handle->bright_select_id == 1)
			{
				lv_label_set_text_fmt(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_label, "%d%%", value);
				pdivoom_lvgl_top_ambient_menu_handle->rgb_info.brightness = value;
				divoom_disp_ambient_ctrl_set_brightness((uint8 *)&value, 1);
			}
			else if(pdivoom_lvgl_top_ambient_menu_handle->bright_select_id == 2)
			{
				lv_label_set_text_fmt(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_label, "%d", value);
				pdivoom_lvgl_top_ambient_menu_handle->pslide_config->video_volume = value;
			}
		}
		else if(c == DIVOOM_KEY_LIGHT_ENTER)
		{
			lv_group_focus_obj(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj);
			lv_obj_clear_flag(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, LV_OBJ_FLAG_HIDDEN);
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			lv_group_focus_obj(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj);
			lv_obj_clear_flag(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, LV_OBJ_FLAG_HIDDEN);
		}
		
	}
}

//灯光效果回调
static void divoom_lvgl_top_ambient_menu_ambient_effect_cout_event_cb(lv_event_t * e)
{
	char image_str[32];
	lv_event_code_t code = lv_event_get_code(e);
	//Pdivoom_channel_set_rgb_info rgb_info = divoom_disp_ambient_ctrl_get_disp_info();
	
	if(code == LV_EVENT_KEY)
    {
		uint32 para = *((uint32 *)lv_event_get_param(e));
        char c = (para&0xff);

		if(pdivoom_lvgl_top_ambient_menu_handle == NULL)
		{
			return;
		}

		pdivoom_lvgl_top_ambient_menu_handle->exit_time = 0;

		if(c == DIVOOM_KEY_LIGHT_RIGHT)
		{
			int select_effect = pdivoom_lvgl_top_ambient_menu_handle->rgb_info.rgb_light_list[0].select_effect;
			sprintf(image_str, "76/76_effect%d_unselect.png", select_effect);
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_image[select_effect], image_str);
		
			if(pdivoom_lvgl_top_ambient_menu_handle->rgb_info.rgb_light_list[0].select_effect == 0)
			{
				pdivoom_lvgl_top_ambient_menu_handle->rgb_info.rgb_light_list[0].select_effect = MAX_AMBIENT_EFFECT - 1;
			}
			else
			{
				pdivoom_lvgl_top_ambient_menu_handle->rgb_info.rgb_light_list[0].select_effect--;
			}
			
			select_effect = pdivoom_lvgl_top_ambient_menu_handle->rgb_info.rgb_light_list[0].select_effect;
			sprintf(image_str, "76/76_effect%d_select.png", select_effect);
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_image[select_effect], image_str);

			lv_label_set_text_fmt(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_label, "%s%d", divoom_menu_control_get_disp_str("AmbientEffect"), select_effect + 1);

			divoom_disp_ambient_ctrl_set_disp_info(&pdivoom_lvgl_top_ambient_menu_handle->rgb_info, 1);
			//divoom_disp_ambient_ctrl_set_mode(select_effect);
		}
		else if(c == DIVOOM_KEY_LIGHT_LEFT)
		{
			int select_effect = pdivoom_lvgl_top_ambient_menu_handle->rgb_info.rgb_light_list[0].select_effect;
			sprintf(image_str, "76/76_effect%d_unselect.png", select_effect);
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_image[select_effect], image_str);

			pdivoom_lvgl_top_ambient_menu_handle->rgb_info.rgb_light_list[0].select_effect++;
			if(pdivoom_lvgl_top_ambient_menu_handle->rgb_info.rgb_light_list[0].select_effect >= MAX_AMBIENT_EFFECT)
			{
				pdivoom_lvgl_top_ambient_menu_handle->rgb_info.rgb_light_list[0].select_effect = 0;
			}

			select_effect = pdivoom_lvgl_top_ambient_menu_handle->rgb_info.rgb_light_list[0].select_effect;
			sprintf(image_str, "76/76_effect%d_select.png", select_effect);
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_image[select_effect], image_str);

			lv_label_set_text_fmt(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_label, "%s%d", divoom_menu_control_get_disp_str("AmbientEffect"), select_effect + 1);

			divoom_disp_ambient_ctrl_set_disp_info(&pdivoom_lvgl_top_ambient_menu_handle->rgb_info, 1);
			//divoom_disp_ambient_ctrl_set_mode(select_effect);
		}
		else if(c == DIVOOM_KEY_LIGHT_ENTER)
		{
			if(pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color_cyc_flag)
			{
				sprintf(image_str, "76/76_effect%d_mix.png", pdivoom_lvgl_top_ambient_menu_handle->rgb_info.rgb_light_list[0].select_effect);  //灯效图片
			}
			else
			{
				sprintf(image_str, "76/76_effect%d_signal.png", pdivoom_lvgl_top_ambient_menu_handle->rgb_info.rgb_light_list[0].select_effect);  //灯效图片
			}
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->main_effect_image, image_str);
	
			lv_group_focus_obj(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj);
			lv_obj_clear_flag(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_obj, LV_OBJ_FLAG_HIDDEN);
		
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			if(pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color_cyc_flag)
			{
				sprintf(image_str, "76/76_effect%d_mix.png", pdivoom_lvgl_top_ambient_menu_handle->rgb_info.rgb_light_list[0].select_effect);  //灯效图片
			}
			else
			{
				sprintf(image_str, "76/76_effect%d_signal.png", pdivoom_lvgl_top_ambient_menu_handle->rgb_info.rgb_light_list[0].select_effect);  //灯效图片
			}
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->main_effect_image, image_str);

			lv_group_focus_obj(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj);
			lv_obj_clear_flag(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_obj, LV_OBJ_FLAG_HIDDEN);
		}
	}
}

//色彩模式回调
/*static void divoom_lvgl_top_ambient_menu_color_mode_cout_event_cb(lv_event_t * e)
{
	char image_str[32];
	lv_event_code_t code = lv_event_get_code(e);
	//Pdivoom_channel_set_rgb_info rgb_info = divoom_disp_ambient_ctrl_get_disp_info();
	
	if(code == LV_EVENT_KEY)
    {
		uint32 para = *((uint32 *)lv_event_get_param(e));
        char c = (para&0xff);

		if(pdivoom_lvgl_top_ambient_menu_handle == NULL)
		{
			return;
		}

		pdivoom_lvgl_top_ambient_menu_handle->exit_time = 0;

		if(c == DIVOOM_KEY_LIGHT_LEFT)
		{
			int color_cyc_flag = pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color_cyc_flag;

			sprintf(image_str, "76/76_colormode%d_unselect.png", color_cyc_flag);
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->colormode_image[color_cyc_flag], image_str);
			if(pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color_cyc_flag)
			{
				pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color_cyc_flag = 0;
			}
			else
			{
				pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color_cyc_flag = 1;
			}

			color_cyc_flag = pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color_cyc_flag;
			sprintf(image_str, "76/76_colormode%d_select.png", color_cyc_flag);
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->colormode_image[color_cyc_flag], image_str);

			if(color_cyc_flag)
			{
				lv_label_set_text(pdivoom_lvgl_top_ambient_menu_handle->colormode_sel_label, divoom_menu_control_get_disp_str("ColorCycle"));
				lv_label_set_text(pdivoom_lvgl_top_ambient_menu_handle->colormode_desp_label, divoom_menu_control_get_disp_str("ColorCycleDesp"));
			}
			else
			{
				lv_label_set_text(pdivoom_lvgl_top_ambient_menu_handle->colormode_sel_label, divoom_menu_control_get_disp_str("ColorSingle"));
				lv_label_set_text(pdivoom_lvgl_top_ambient_menu_handle->colormode_desp_label, divoom_menu_control_get_disp_str("ColorSingleDesp"));
			}
			divoom_disp_ambient_ctrl_set_disp_info(&pdivoom_lvgl_top_ambient_menu_handle->rgb_info, 1);
		}
		else if(c == DIVOOM_KEY_LIGHT_RIGHT)
		{
			int color_cyc_flag = pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color_cyc_flag;

			//当前选中变未选中
			sprintf(image_str, "76/76_colormode%d_unselect.png", color_cyc_flag);
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->colormode_image[color_cyc_flag], image_str);
			if(pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color_cyc_flag)
			{
				pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color_cyc_flag = 0;
			}
			else
			{
				pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color_cyc_flag = 1;
			}

			//当前未选中变选中
			color_cyc_flag = pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color_cyc_flag;
			sprintf(image_str, "76/76_colormode%d_select.png", color_cyc_flag);
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->colormode_image[color_cyc_flag], image_str);

			if(color_cyc_flag)
			{
				lv_label_set_text(pdivoom_lvgl_top_ambient_menu_handle->colormode_sel_label, divoom_menu_control_get_disp_str("ColorCycle"));
				lv_label_set_text(pdivoom_lvgl_top_ambient_menu_handle->colormode_desp_label, divoom_menu_control_get_disp_str("ColorCycleDesp"));
			}
			else
			{
				lv_label_set_text(pdivoom_lvgl_top_ambient_menu_handle->colormode_sel_label, divoom_menu_control_get_disp_str("ColorSingle"));
				lv_label_set_text(pdivoom_lvgl_top_ambient_menu_handle->colormode_desp_label, divoom_menu_control_get_disp_str("ColorSingleDesp"));
			}
			divoom_disp_ambient_ctrl_set_disp_info(&pdivoom_lvgl_top_ambient_menu_handle->rgb_info, 1);
		}
		else if(c == DIVOOM_KEY_LIGHT_ENTER)
		{
			if(pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color_cyc_flag)
			{
				sprintf(image_str, "76/76_mode2_cycle.png");	 //色彩循环模式
				divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->main_colormode_image, image_str);
			}
			else
			{
				sprintf(image_str, "76/76_mode2_single.png");	 //单色模式
				divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->main_colormode_image, image_str);
			}
			lv_group_focus_obj(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj);
			lv_obj_clear_flag(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(pdivoom_lvgl_top_ambient_menu_handle->colormode_obj, LV_OBJ_FLAG_HIDDEN);

		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			if(pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color_cyc_flag)
			{
				sprintf(image_str, "76/76_mode2_cycle.png");	 //色彩循环模式
				divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->main_colormode_image, image_str);
			}
			else
			{
				sprintf(image_str, "76/76_mode2_single.png");	 //单色模式
				divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->main_colormode_image, image_str);
			}
			lv_group_focus_obj(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj);
			lv_obj_clear_flag(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(pdivoom_lvgl_top_ambient_menu_handle->colormode_obj, LV_OBJ_FLAG_HIDDEN);

		}
	}

}
*/
void divoom_lvgl_top_ambient_menu_calculate_position(int hue, int *x, int *y)
{
    // 将色相角度转换为弧度
    double angle_rad = hue * 3.1415 / 180.0;

    // 计算 x 和 y 坐标
    (*x) = (int)(400 + 152 * cos(angle_rad) - 7 + 0.5);
    (*y) = (int)(640 - 152 * sin(angle_rad) - 7 + 0.5);

	//divoom_print("hue = %d, x = %d, y =%d\n", pdivoom_lvgl_top_ambient_menu_handle->hue_angle, *x, *y);
}

// 函数：将RGB转换为Hue，返回int类型，范围0~360
int divoom_lvgl_top_ambient_menu_rgb_to_hue(int r, int g, int b) 
{
    float hue = 0.0;
    float min, max, delta;

    // 将RGB值归一化到[0, 1]范围
    float rf = r / 255.0;
    float gf = g / 255.0;
    float bf = b / 255.0;

    // 找到RGB中的最大值和最小值
    min = rf;
    if (gf < min) min = gf;
    if (bf < min) min = bf;

    max = rf;
    if (gf > max) max = gf;
    if (bf > max) max = bf;

    delta = max - min;

    // 计算Hue
    if (delta == 0) {
        hue = 0; // 无色差，Hue为0
    } else if (max == rf) {
        hue = (gf - bf) / delta; // 红色是最大值
    } else if (max == gf) {
        hue = 2.0 + (bf - rf) / delta; // 绿色是最大值
    } else if (max == bf) {
        hue = 4.0 + (rf - gf) / delta; // 蓝色是最大值
    }

    hue *= 60; // 将Hue转换为度数
    if (hue < 0) {
        hue += 360; // 确保Hue在[0, 360]范围内
    }

    // 将Hue转换为int并确保范围在0~360
    int hueInt = (int)(hue + 0.5); // 四舍五入
    if (hueInt >= 360) hueInt -= 360; // 如果大于等于360，减去360

    return hueInt;
}

// 根据色相（Hue）计算 RGB 值，并通过指针参数返回
void divoom_lvgl_top_ambient_menu_hue_to_rgb(int hue, int *r, int *g, int *b)
{
	int sector = hue / 60;	// 色相所在的扇区（0-5）
	double rem = (hue - sector * 60) / 60.0;  // 扇区内的剩余部分（0-1）
	
	uint8_t p = 0;	// 最小值
	uint8_t q = (uint8_t)(255 * (1 - rem) + 0.5);  // 中间值
	uint8_t t = (uint8_t)(255 * rem + 0.5);		 // 最大值
	
	uint8_t r_val, g_val, b_val;
	
	switch (sector)
	{
		case 0: r_val = 255; g_val = t; b_val = p; break;  // 红色到黄色
		case 1: r_val = q; g_val = 255; b_val = p; break;  // 黄色到绿色
		case 2: r_val = p; g_val = 255; b_val = t; break;  // 绿色到青色
		case 3: r_val = p; g_val = q; b_val = 255; break;  // 青色到蓝色
		case 4: r_val = t; g_val = p; b_val = 255; break;  // 蓝色到品红
		default: r_val = 255; g_val = p; b_val = q; break;	// 品红到红色
	}
	
	// 将结果存储到传入的指针参数中
	(*r) = r_val;
	(*g) = g_val;
	(*b) = b_val;

}

//色彩调节回调
static void divoom_lvgl_top_ambient_menu_color_adjust_cout_event_cb(lv_event_t * e)
{
	int x, y, key_turn_time, r, g, b;
	char image_str[32];
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_KEY)
    {
		uint32 para = *((uint32 *)lv_event_get_param(e));
        char c = (para&0xff);

		if(pdivoom_lvgl_top_ambient_menu_handle == NULL)
		{
			return;
		}

		pdivoom_lvgl_top_ambient_menu_handle->exit_time = 0;
		
		if(c == DIVOOM_KEY_LIGHT_RIGHT)
		{
			if(pdivoom_lvgl_top_ambient_menu_handle->color_index == 0)
			{
				pdivoom_lvgl_top_ambient_menu_handle->color_index = sizeof(divoom_top_ambient_coloradjust_color_hex)/sizeof(divoom_top_ambient_coloradjust_color_hex[0]) - 1;
			}
			else
			{
				pdivoom_lvgl_top_ambient_menu_handle->color_index--;
			}

			//lv_label_set_text_fmt(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_select_label, "Color %d", pdivoom_lvgl_top_ambient_menu_handle->color_index+1);

			if(pdivoom_lvgl_top_ambient_menu_handle->color_index == 0)
			{
				lv_label_set_text(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_color_label, "RGB");

				pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color_cyc_flag = 1;
			}
			else
			{
				int color_hex = divoom_top_ambient_coloradjust_color_hex[pdivoom_lvgl_top_ambient_menu_handle->color_index];
			
				lv_label_set_text_fmt(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_color_label, "#%06X", color_hex);
				//更新氛围灯颜色
				pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color[0] = (color_hex >> 16) & 0xFF;
				pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color[1] = (color_hex >> 8) & 0xFF;
				pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color[2] = color_hex & 0xFF;
				pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color_cyc_flag = 0;
			}
			
			pdivoom_lvgl_top_ambient_menu_handle->hue_angle = divoom_top_ambient_coloradjust_color_angle[pdivoom_lvgl_top_ambient_menu_handle->color_index];
			divoom_lvgl_top_ambient_menu_calculate_position(pdivoom_lvgl_top_ambient_menu_handle->hue_angle, &x, &y);
			lv_obj_set_pos(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_knob_obj, x, y);
			
			//divoom_lvgl_top_ambient_menu_hue_to_rgb(pdivoom_lvgl_top_ambient_menu_handle->hue_angle, &r, &g, &b);
			
			divoom_disp_ambient_ctrl_set_disp_info(&pdivoom_lvgl_top_ambient_menu_handle->rgb_info, 1);
		}
		else if(c == DIVOOM_KEY_LIGHT_LEFT)
		{
			pdivoom_lvgl_top_ambient_menu_handle->color_index++;
			if(pdivoom_lvgl_top_ambient_menu_handle->color_index >= sizeof(divoom_top_ambient_coloradjust_color_hex)/sizeof(divoom_top_ambient_coloradjust_color_hex[0]))
			{
				pdivoom_lvgl_top_ambient_menu_handle->color_index = 0;
			}

			//lv_label_set_text_fmt(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_select_label, "Color %d", pdivoom_lvgl_top_ambient_menu_handle->color_index+1);

			if(pdivoom_lvgl_top_ambient_menu_handle->color_index == 0)
			{
				lv_label_set_text(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_color_label, "RGB");

				pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color_cyc_flag = 1;
			}
			else
			{
				int color_hex = divoom_top_ambient_coloradjust_color_hex[pdivoom_lvgl_top_ambient_menu_handle->color_index];
			
				lv_label_set_text_fmt(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_color_label, "#%06X", color_hex);
				//更新氛围灯颜色
				pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color[0] = (color_hex >> 16) & 0xFF;
				pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color[1] = (color_hex >> 8) & 0xFF;
				pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color[2] = color_hex & 0xFF;
				pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color_cyc_flag = 0;
			}

			pdivoom_lvgl_top_ambient_menu_handle->hue_angle = divoom_top_ambient_coloradjust_color_angle[pdivoom_lvgl_top_ambient_menu_handle->color_index];
			divoom_lvgl_top_ambient_menu_calculate_position(pdivoom_lvgl_top_ambient_menu_handle->hue_angle, &x, &y);
			lv_obj_set_pos(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_knob_obj, x, y);
			
			divoom_disp_ambient_ctrl_set_disp_info(&pdivoom_lvgl_top_ambient_menu_handle->rgb_info, 1);
			
		}
		else if(c == DIVOOM_KEY_LIGHT_ENTER)
		{
			if(pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color_cyc_flag)
			{
				sprintf(image_str, "76/76_effect%d_mix.png", pdivoom_lvgl_top_ambient_menu_handle->rgb_info.rgb_light_list[0].select_effect);  //灯效图片
			}
			else
			{
				sprintf(image_str, "76/76_effect%d_signal.png", pdivoom_lvgl_top_ambient_menu_handle->rgb_info.rgb_light_list[0].select_effect);  //灯效图片
			}
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->main_effect_image, image_str);
			
			lv_group_focus_obj(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj);
			lv_obj_clear_flag(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_obj, LV_OBJ_FLAG_HIDDEN);
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			if(pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color_cyc_flag)
			{
				sprintf(image_str, "76/76_effect%d_mix.png", pdivoom_lvgl_top_ambient_menu_handle->rgb_info.rgb_light_list[0].select_effect);  //灯效图片
			}
			else
			{
				sprintf(image_str, "76/76_effect%d_signal.png", pdivoom_lvgl_top_ambient_menu_handle->rgb_info.rgb_light_list[0].select_effect);  //灯效图片
			}
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->main_effect_image, image_str);
			
			lv_group_focus_obj(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj);
			lv_obj_clear_flag(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_obj, LV_OBJ_FLAG_HIDDEN);
		}
	}

}




void divoom_lvgl_top_ambient_menu_create_icon(void)
{
	int i, select_effect, x, y, color_hex, cur_color_hex;
	//int mode_xy[5][2] = {{288-6,440-3}, {467-5,483-6}, {407-6,680-5}, {212-5,680-6}, {199-5,483-6}};	//主菜单模式背景图片坐标
	//int disp_xy[5][2] = {{375,451}, {510,556}, {459,736}, {282,735}, {224,562}};		//主菜单模式icon图片坐标
	int mode_xy[6][2] = {{307-5,439-3}, {459-3,470-3}, {459-5,648-6}, {307-5,734-6}, {200-5,648-6}, {200-5,470-6}};	//主菜单模式背景图片坐标
	int disp_xy[6][2] = {{375,460}, {506,526}, {497,679}, {369,765}, {239,688}, {236,532}};		//主菜单模式icon图片坐标
	//灯光效果背景图片坐标
	int image_xy[MAX_AMBIENT_EFFECT][2] = {{327,440}, {444,456}, {499, 567}, {444,684}, {327,738}, {217,683}, {200,567}, {217,456}};
	//灯光效果icon图片坐标
	int effect_xy[MAX_AMBIENT_EFFECT][2] = {{370,460}, {482,506}, {527, 613}, {482,720}, {373,765}, {263,720}, {221,613}, {263,506}};
	int bright = divoom_main_get_sys_light();
	char image_str[32];
	Pdivoom_channel_set_rgb_info rgb_info = divoom_disp_ambient_ctrl_get_disp_info();

	if(rgb_info->rgb_light_list[0].select_effect >= MAX_AMBIENT_EFFECT)
	{
		select_effect = 0;
	}
	else
	{
		select_effect = rgb_info->rgb_light_list[0].select_effect;
	}
	
	memcpy(&pdivoom_lvgl_top_ambient_menu_handle->rgb_info, rgb_info, sizeof(divoom_channel_set_rgb_info));

	//选择菜单
	pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj = lv_btn_create(pdivoom_lvgl_top_ambient_menu_handle->main_obj);

	lv_obj_remove_style_all(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj);
	lv_obj_set_style_pad_all(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj, 0, 0);
	lv_obj_set_style_border_width(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj, 0, 0);
	lv_obj_set_style_outline_width(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj, 0, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_shadow_width(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj, 0, 0);
	lv_obj_set_style_bg_opa(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj, LV_OPA_100, 0);
	lv_obj_set_style_bg_color(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj, lv_color_hex(0xF2F2F2), 0);
	lv_obj_clear_flag(pdivoom_lvgl_top_ambient_menu_handle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_pos(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj, 0, 0);
	lv_obj_set_size(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_add_event_cb(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj, divoom_lvgl_top_ambient_menu_main_cout_event_cb, LV_EVENT_KEY, NULL);

	lv_obj_t *main_bg_img = lv_img_create(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj);
	divoom_menu_control_lv_img_set_src(main_bg_img, "76/76_mode_circle.png");
	lv_obj_set_pos(main_bg_img, 305, 545);

	lv_obj_t* cont = lv_obj_create(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj);
	lv_obj_set_size(cont, 190, 190);
	lv_obj_set_pos(cont, 305, 545);
	lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
	//lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *main_label_obj = lv_label_create(cont);
	lv_obj_set_style_text_font(main_label_obj, divoom_menu_control_get_font_info(32, DIVOOM_MENU_CONTROL_ID_TOP_AMBIENT_MENU), 0);
	lv_obj_set_style_text_color(main_label_obj, lv_color_black(), 0);
	lv_obj_set_style_text_align(main_label_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_align(main_label_obj, LV_ALIGN_CENTER);	//上下居中对齐
	//lv_obj_set_size(main_label_obj, 190, 80);
	//lv_obj_set_pos(main_label_obj, 305, 600+20);
	lv_obj_set_width(main_label_obj, 190);
	lv_label_set_text(main_label_obj, divoom_menu_control_get_disp_str(divoom_top_ambient_mode_str[pdivoom_lvgl_top_ambient_menu_handle->main_id]));
	pdivoom_lvgl_top_ambient_menu_handle->main_label_obj = main_label_obj;

	for(i = 0;i < 6; i++)	//模式图片创建
	{
		pdivoom_lvgl_top_ambient_menu_handle->main_select_image[i] = lv_img_create(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj);

		if(i == pdivoom_lvgl_top_ambient_menu_handle->main_id)
		{
			sprintf(image_str, "76/76_mode%d_select.png", i);	//选中图形
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->main_select_image[i], image_str);
		}
		else
		{
			sprintf(image_str, "76/76_mode%d_unselect.png", i);	 //未选中图形
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->main_select_image[i], image_str);
		}
		lv_obj_set_pos(pdivoom_lvgl_top_ambient_menu_handle->main_select_image[i], mode_xy[i][0], mode_xy[i][1]);

		//菜单icon图片
		lv_obj_t *disp_img = lv_img_create(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj);
		
		/*if(i == 2)	//色彩循环
		{
			if(rgb_info->color_cyc_flag)
			{
				sprintf(image_str, "76/76_mode2_cycle.png");	 //色彩循环模式
				divoom_menu_control_lv_img_set_src(disp_img, image_str);
			}
			else
			{
				sprintf(image_str, "76/76_mode2_single.png");	 //单色模式
				divoom_menu_control_lv_img_set_src(disp_img, image_str);
			}
			pdivoom_lvgl_top_ambient_menu_handle->main_colormode_image = disp_img;
		}
		else */
		if(i == 5)	//灯效
		{
			if(pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color_cyc_flag)
			{
				sprintf(image_str, "76/76_effect%d_mix.png", select_effect);  //灯效图片
			}
			else
			{
				sprintf(image_str, "76/76_effect%d_signal.png", select_effect);  //灯效图片
			}
			divoom_menu_control_lv_img_set_src(disp_img, image_str);
			pdivoom_lvgl_top_ambient_menu_handle->main_effect_image = disp_img;
		}
		else	//其他模式
		{
			sprintf(image_str, "76/76_mode%d.png", i);
			divoom_menu_control_lv_img_set_src(disp_img, image_str);
		}
		
		lv_obj_set_pos(disp_img, disp_xy[i][0], disp_xy[i][1]);
		
	}

	//屏幕亮度菜单
	pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj = lv_btn_create(pdivoom_lvgl_top_ambient_menu_handle->main_obj);

	lv_obj_remove_style_all(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj);
	lv_obj_set_style_pad_all(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, 0, 0);
	lv_obj_set_style_border_width(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, 0, 0);
	lv_obj_set_style_outline_width(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, 0, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_shadow_width(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, 0, 0);
	lv_obj_set_style_bg_opa(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, LV_OPA_100, 0);
	lv_obj_set_style_bg_color(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, lv_color_hex(0xF2F2F2), 0);
	lv_obj_set_pos(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, 0, 0);
	lv_obj_set_size(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_add_event_cb(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, divoom_lvgl_top_ambient_menu_screen_bright_cout_event_cb, LV_EVENT_KEY, NULL);

	/*lv_obj_set_style_bg_opa(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, 75*LV_OPA_100/100, 0);
	lv_obj_set_style_radius(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_bg_color(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_border_width(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, 4, 0);
	lv_obj_set_style_border_color(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_border_opa(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, LV_OPA_30, 0);
	lv_obj_set_style_shadow_color(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, lv_color_hex(0x000000), 0);
	lv_obj_set_style_shadow_opa(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, LV_OPA_30, 0);
	lv_obj_set_style_shadow_ofs_y(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, 4, 0);
	lv_obj_set_style_shadow_spread(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, 10, 0);
	
	lv_obj_set_size(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, 400, 400);
	lv_obj_set_pos(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, 200, 440);
	lv_obj_add_event_cb(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, divoom_lvgl_top_ambient_menu_screen_bright_cout_event_cb, LV_EVENT_KEY, NULL);
	*/
	
	lv_obj_t *bg_img = lv_img_create(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj);
	divoom_menu_control_lv_img_set_src(bg_img, "76/76_bright_circle.png");
	lv_obj_set_pos(bg_img, 200-15, 440-15);

	lv_obj_t *arc = lv_arc_create(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj);
	lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270);
	lv_obj_set_style_arc_color(arc, lv_color_hex(0x666666), 0);
	lv_obj_set_style_arc_color(arc, lv_color_hex(0xEAEAEA), LV_PART_INDICATOR);
	lv_obj_set_style_arc_width(arc, 24, 0);
	lv_obj_set_style_arc_width(arc, 24, LV_PART_INDICATOR);
	
	lv_obj_set_style_pad_all(arc, 15, LV_PART_KNOB); // 设置旋钮大小
	lv_obj_set_style_border_width(arc, 4, LV_PART_KNOB);
	lv_obj_set_style_border_color(arc, lv_color_hex(0x000000), LV_PART_KNOB);
	lv_obj_set_style_border_opa(arc, 16 * LV_OPA_100/100, LV_PART_KNOB);
	lv_obj_set_style_bg_color(arc, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
	lv_obj_set_style_shadow_width(arc, 10, LV_PART_KNOB);  // 设置阴影宽度
	lv_obj_set_style_shadow_ofs_y(arc, 4, LV_PART_KNOB);   // 设置阴影 Y 偏移量
	lv_obj_set_style_shadow_color(arc, lv_color_hex(0x000000), LV_PART_KNOB);
	lv_obj_set_style_shadow_opa(arc, LV_OPA_70, LV_PART_KNOB); // 设置阴影透明度
	divoom_menu_control_get_img_path("76/76_bright.png", pdivoom_lvgl_top_ambient_menu_handle->file_name);
	lv_obj_set_style_bg_img_src(arc, pdivoom_lvgl_top_ambient_menu_handle->file_name, LV_PART_KNOB);
	//lv_obj_set_style_bg_img_opa(arc, LV_OPA_100, LV_PART_KNOB); // 设置图片透明度
	lv_arc_set_range(arc, 0, 100);
	lv_arc_set_value(arc, bright);
	lv_obj_set_size(arc, 324, 324);
	lv_obj_set_pos(arc, 238+3, 473+3);
	pdivoom_lvgl_top_ambient_menu_handle->screen_arc_obj = arc;
	
	//显示亮度
	lv_obj_t *bright_label = lv_label_create(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj);
	lv_obj_set_style_text_font(bright_label, divoom_menu_control_get_font_info(72, DIVOOM_MENU_CONTROL_ID_TOP_AMBIENT_MENU), 0);
	lv_obj_set_style_text_color(bright_label, lv_color_black(), 0);
	lv_obj_set_style_text_align(bright_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_size(bright_label, 210, 95);
	lv_obj_set_pos(bright_label, 295, 593);
	lv_label_set_text_fmt(bright_label, "%d%%", bright);
	pdivoom_lvgl_top_ambient_menu_handle->screen_bright_label = bright_label;

	lv_obj_t *disp_label = lv_label_create(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj);
	lv_obj_set_style_text_font(disp_label, divoom_menu_control_get_font_info(24, DIVOOM_MENU_CONTROL_ID_TOP_AMBIENT_MENU), 0);
	lv_obj_set_style_text_color(disp_label, lv_color_black(), 0);
	lv_obj_set_style_text_align(disp_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_size(disp_label, 500, 32);
	lv_obj_set_pos(disp_label, 150, 775-10);
	lv_label_set_text(disp_label, divoom_menu_control_get_disp_str("ScreenBright"));
	pdivoom_lvgl_top_ambient_menu_handle->screen_disp_label = disp_label;

	lv_obj_add_flag(pdivoom_lvgl_top_ambient_menu_handle->screen_bright_obj, LV_OBJ_FLAG_HIDDEN);

	//色彩模式菜单
	/*pdivoom_lvgl_top_ambient_menu_handle->colormode_obj = lv_btn_create(pdivoom_lvgl_top_ambient_menu_handle->main_obj);
	
	lv_obj_remove_style_all(pdivoom_lvgl_top_ambient_menu_handle->colormode_obj);
	lv_obj_set_style_pad_all(pdivoom_lvgl_top_ambient_menu_handle->colormode_obj, 0, 0);
	lv_obj_set_style_border_width(pdivoom_lvgl_top_ambient_menu_handle->colormode_obj, 0, 0);
	lv_obj_set_style_outline_width(pdivoom_lvgl_top_ambient_menu_handle->colormode_obj, 0, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_shadow_width(pdivoom_lvgl_top_ambient_menu_handle->colormode_obj, 0, 0);
	lv_obj_set_style_bg_opa(pdivoom_lvgl_top_ambient_menu_handle->colormode_obj, LV_OPA_0, 0);
	lv_obj_set_pos(pdivoom_lvgl_top_ambient_menu_handle->colormode_obj, 54, 390);
	lv_obj_set_size(pdivoom_lvgl_top_ambient_menu_handle->colormode_obj, 692, 772);
	lv_obj_add_event_cb(pdivoom_lvgl_top_ambient_menu_handle->colormode_obj, divoom_lvgl_top_ambient_menu_color_mode_cout_event_cb, LV_EVENT_KEY, NULL);

	lv_obj_t *circle_img = lv_img_create(pdivoom_lvgl_top_ambient_menu_handle->colormode_obj);
	divoom_menu_control_lv_img_set_src(circle_img, "76/76_mode_circle.png");
	lv_obj_set_pos(circle_img, 155+96, 155);	//中心圆

	lv_obj_t *sel_label = lv_label_create(pdivoom_lvgl_top_ambient_menu_handle->colormode_obj);
	lv_obj_set_style_text_font(sel_label, divoom_menu_control_get_font_info(32, DIVOOM_MENU_CONTROL_ID_TOP_AMBIENT_MENU), 0);
	lv_obj_set_style_text_color(sel_label, lv_color_black(), 0);
	lv_obj_set_style_text_align(sel_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_size(sel_label, 190, 80);
	lv_obj_set_pos(sel_label, 155+96, 210+20);
	if(rgb_info->color_cyc_flag)	//显示当前色彩模式
	{
		lv_label_set_text(sel_label, divoom_menu_control_get_disp_str("ColorCycle"));
	}
	else
	{
		lv_label_set_text(sel_label, divoom_menu_control_get_disp_str("ColorSingle"));
	}
	pdivoom_lvgl_top_ambient_menu_handle->colormode_sel_label = sel_label;


	for(i = 0; i < 2; i++)	//创建色彩模式图片
	{
		pdivoom_lvgl_top_ambient_menu_handle->colormode_image[i] = lv_img_create(pdivoom_lvgl_top_ambient_menu_handle->colormode_obj);

		if(i == rgb_info->color_cyc_flag)	
		{
			sprintf(image_str, "76/76_colormode%d_select.png", i); 	//选中图片
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->colormode_image[i], image_str);
		}
		else		
		{
			sprintf(image_str, "76/76_colormode%d_unselect.png", i);	//未选中图片
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->colormode_image[i], image_str);
		}

		if(i == 0)	//单色模式
		{
			lv_obj_set_pos(pdivoom_lvgl_top_ambient_menu_handle->colormode_image[i], 252+96, 0);
		}
		else		//循环模式
		{
			lv_obj_set_pos(pdivoom_lvgl_top_ambient_menu_handle->colormode_image[i], 0+96, 0);
		}
	}

	lv_obj_t *color_mode_label = lv_label_create(pdivoom_lvgl_top_ambient_menu_handle->colormode_obj);
	lv_obj_set_style_text_font(color_mode_label, divoom_menu_control_get_font_info(32, DIVOOM_MENU_CONTROL_ID_TOP_AMBIENT_MENU), 0);
	lv_obj_set_style_text_color(color_mode_label, lv_color_white(), 0);
	lv_obj_set_style_text_align(color_mode_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_size(color_mode_label, 692, 240);
	//lv_obj_align_to(color_mode_label, pdivoom_lvgl_top_ambient_menu_handle->colormode_obj, LV_ALIGN_OUT_BOTTOM_LEFT, -96, 32);
	lv_obj_set_pos(color_mode_label, 0, 532);
	if(rgb_info->color_cyc_flag)//色彩模式描述
	{
		lv_label_set_text(color_mode_label, divoom_menu_control_get_disp_str("ColorCycleDesp"));
	}
	else
	{
		lv_label_set_text(color_mode_label, divoom_menu_control_get_disp_str("ColorSingleDesp"));
	}
	pdivoom_lvgl_top_ambient_menu_handle->colormode_desp_label = color_mode_label;
	lv_obj_add_flag(pdivoom_lvgl_top_ambient_menu_handle->colormode_obj, LV_OBJ_FLAG_HIDDEN);
	*/
	//色彩调节菜单
	pdivoom_lvgl_top_ambient_menu_handle->coloradjust_obj = lv_btn_create(pdivoom_lvgl_top_ambient_menu_handle->main_obj);
	
	lv_obj_remove_style_all(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_obj);
	lv_obj_set_style_pad_all(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_obj, 0, 0);
	lv_obj_set_style_border_width(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_obj, 0, 0);
	lv_obj_set_style_outline_width(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_obj, 0, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_shadow_width(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_obj, 0, 0);
	lv_obj_set_style_bg_opa(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_obj, LV_OPA_100, 0);
	lv_obj_set_style_bg_color(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_obj, lv_color_hex(0xF2F2F2), 0);
	lv_obj_set_pos(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_obj, 0, 0);
	lv_obj_set_size(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_add_event_cb(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_obj, divoom_lvgl_top_ambient_menu_color_adjust_cout_event_cb, LV_EVENT_KEY, NULL);

	if(rgb_info->color_cyc_flag == 1)
	{
		pdivoom_lvgl_top_ambient_menu_handle->color_index = 0;
	}
	else
	{	
		cur_color_hex = (rgb_info->color[0] << 16) + (rgb_info->color[1] << 8) + rgb_info->color[2];
		for(i = 0; i < sizeof(divoom_top_ambient_coloradjust_color_hex)/sizeof(divoom_top_ambient_coloradjust_color_hex[0]); i++)
		{
			color_hex = divoom_top_ambient_coloradjust_color_hex[i];

			if((cur_color_hex == color_hex) && (rgb_info->color_cyc_flag == 0))
			{
				pdivoom_lvgl_top_ambient_menu_handle->color_index = i;
				break;
			}
		}
		if(i >= sizeof(divoom_top_ambient_coloradjust_color_hex)/sizeof(divoom_top_ambient_coloradjust_color_hex[0]))
		{
			pdivoom_lvgl_top_ambient_menu_handle->color_index = 0;
		}
	}
	
	lv_obj_t *adjust_circle_img = lv_img_create(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_obj);
	divoom_menu_control_lv_img_set_src(adjust_circle_img, "76/76_mode_circle.png");
	lv_obj_set_pos(adjust_circle_img, 305, 545);	//中心圆

	//lv_obj_t *adjust_sel_label = lv_label_create(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_obj);
	//lv_obj_set_style_text_font(adjust_sel_label, divoom_menu_control_get_font_info(24, DIVOOM_MENU_CONTROL_ID_TOP_AMBIENT_MENU), 0);
	//lv_obj_set_style_text_color(adjust_sel_label, lv_color_black(), 0);
	//lv_obj_set_style_text_align(adjust_sel_label, LV_TEXT_ALIGN_CENTER, 0);
	//lv_obj_set_size(adjust_sel_label, 134, 32);
	//lv_obj_set_pos(adjust_sel_label, 333, 610);
	//lv_label_set_text_fmt(adjust_sel_label, "Color %d", pdivoom_lvgl_top_ambient_menu_handle->color_index+1);
	//pdivoom_lvgl_top_ambient_menu_handle->coloradjust_select_label = adjust_sel_label;

	lv_obj_t *adjust_color_label = lv_label_create(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_obj);
	lv_obj_set_style_text_font(adjust_color_label, divoom_menu_control_get_font_info(32, DIVOOM_MENU_CONTROL_ID_TOP_AMBIENT_MENU), 0);
	lv_obj_set_style_text_color(adjust_color_label, lv_color_hex(0x0), 0);
	lv_obj_set_style_text_align(adjust_color_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_size(adjust_color_label, 190, 80);
	lv_obj_set_pos(adjust_color_label, 304, 600+20);
	if(pdivoom_lvgl_top_ambient_menu_handle->color_index == 0)
	{
		lv_label_set_text(adjust_color_label, "RGB");
	}
	else
	{
		lv_label_set_text_fmt(adjust_color_label, "#%06X", divoom_top_ambient_coloradjust_color_hex[pdivoom_lvgl_top_ambient_menu_handle->color_index]);
	}
	pdivoom_lvgl_top_ambient_menu_handle->coloradjust_color_label = adjust_color_label;	

	lv_obj_t *wheel_img = lv_img_create(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_obj);
	divoom_menu_control_lv_img_set_src(wheel_img, "76/76_coloradjust_wheel.png");
	//lv_obj_set_style_img_opa(wheel_img, LV_OPA_100, LV_PART_MAIN); 
	lv_obj_set_pos(wheel_img, 171-5, 411-5);

	lv_obj_t *knob_img = lv_img_create(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_obj);
	divoom_menu_control_lv_img_set_src(knob_img, "76/76_coloradjust_knob.png");
	pdivoom_lvgl_top_ambient_menu_handle->coloradjust_knob_obj = knob_img;
	
	//计算当前颜色对应的角度
	//pdivoom_lvgl_top_ambient_menu_handle->hue_angle = divoom_lvgl_top_ambient_menu_rgb_to_hue(rgb_info->color[0], rgb_info->color[1], rgb_info->color[2]);
	pdivoom_lvgl_top_ambient_menu_handle->hue_angle = divoom_top_ambient_coloradjust_color_angle[pdivoom_lvgl_top_ambient_menu_handle->color_index];
	divoom_lvgl_top_ambient_menu_calculate_position(pdivoom_lvgl_top_ambient_menu_handle->hue_angle, &x, &y);
	lv_obj_set_pos(knob_img, x, y);
	
	lv_obj_add_flag(pdivoom_lvgl_top_ambient_menu_handle->coloradjust_obj, LV_OBJ_FLAG_HIDDEN);

	//氛围灯灯光效果菜单
	pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_obj = lv_btn_create(pdivoom_lvgl_top_ambient_menu_handle->main_obj);
	
	lv_obj_remove_style_all(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_obj);
	lv_obj_set_style_pad_all(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_obj, 0, 0);
	lv_obj_set_style_border_width(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_obj, 0, 0);
	lv_obj_set_style_outline_width(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_obj, 0, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_shadow_width(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_obj, 0, 0);
	lv_obj_set_style_bg_opa(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_obj, LV_OPA_100, 0);
	lv_obj_set_style_bg_color(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_obj, lv_color_hex(0xF2F2F2), 0);
	lv_obj_set_pos(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_obj, 0, 0);
	lv_obj_set_size(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_add_event_cb(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_obj, divoom_lvgl_top_ambient_menu_ambient_effect_cout_event_cb, LV_EVENT_KEY, NULL);

	lv_obj_t *center_circle = lv_img_create(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_obj);
	divoom_menu_control_lv_img_set_src(center_circle, "76/76_mode_circle.png");
	lv_obj_set_pos(center_circle, 305, 545);

	lv_obj_t* label_cont = lv_obj_create(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_obj);
	lv_obj_set_size(label_cont, 190, 190);
	lv_obj_set_pos(label_cont, 305, 545);
	lv_obj_set_style_bg_opa(label_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(label_cont, 0, 0);
	//lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_clear_flag(label_cont, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *ambient_effect_label = lv_label_create(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_obj);
	lv_obj_set_style_text_font(ambient_effect_label, divoom_menu_control_get_font_info(32, DIVOOM_MENU_CONTROL_ID_TOP_AMBIENT_MENU), 0);
	lv_obj_set_style_text_color(ambient_effect_label, lv_color_black(), 0);
	lv_obj_set_style_text_align(ambient_effect_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_align(ambient_effect_label, LV_ALIGN_CENTER);	//上下居中对齐
	//lv_obj_set_size(ambient_effect_label, 190, 80);
	//lv_obj_set_pos(ambient_effect_label, 305, 600+20);
	lv_obj_set_width(ambient_effect_label, 190);
	lv_label_set_text_fmt(ambient_effect_label, "%s%d", divoom_menu_control_get_disp_str("AmbientEffect"), select_effect + 1);
	pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_label = ambient_effect_label;
	
	for(i = 0; i < MAX_AMBIENT_EFFECT; i++)
	{
		pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_image[i] = lv_img_create(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_obj);

		if(select_effect == i)	//灯效背景图片
		{
			sprintf(image_str, "76/76_effect%d_select.png", i);		//选中图片
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_image[i], image_str);
		}
		else
		{
			sprintf(image_str, "76/76_effect%d_unselect.png", i);	//未选中图片
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_image[i], image_str);
		}
		lv_obj_set_pos(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_image[i], image_xy[i][0], image_xy[i][1]);

		//灯效图片
		lv_obj_t *effect_img = lv_img_create(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_obj);
		if(pdivoom_lvgl_top_ambient_menu_handle->rgb_info.color_cyc_flag)
		{
			sprintf(image_str, "76/76_effect%d_mix.png", i);
		}
		else
		{
			sprintf(image_str, "76/76_effect%d_signal.png", i);
		}
		divoom_menu_control_lv_img_set_src(effect_img, image_str);
		lv_obj_set_pos(effect_img, effect_xy[i][0], effect_xy[i][1]);
		pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_icon[i] = effect_img;
	}

	lv_obj_add_flag(pdivoom_lvgl_top_ambient_menu_handle->ambient_effect_obj, LV_OBJ_FLAG_HIDDEN);
}

void divoom_lvgl_top_ambient_menu_init(void)
{
	divoom_get_lvgl_lock();

	if(pdivoom_lvgl_top_ambient_menu_handle != NULL)
	{
		divoom_free_lvgl_lock();
		return;
	}
	
	pdivoom_lvgl_top_ambient_menu_handle = (Pdivoom_lvgl_top_ambient_menu_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_top_ambient_menu_ctrl));
	
	if(pdivoom_lvgl_top_ambient_menu_handle == NULL)
	{
		divoom_free_lvgl_lock();
		return;
	}
	memset(pdivoom_lvgl_top_ambient_menu_handle, 0, sizeof(divoom_lvgl_top_ambient_menu_ctrl));

	//pdivoom_lvgl_top_ambient_menu_handle->rgb_info = divoom_disp_ambient_ctrl_get_disp_info();
	pdivoom_lvgl_top_ambient_menu_handle->main_id = 0;	//默认选择屏幕亮度
	pdivoom_lvgl_top_ambient_menu_handle->exit_time = 0;
	pdivoom_lvgl_top_ambient_menu_handle->pslide_config = divoom_pic_phone_manage_get_slide_config();
	pdivoom_lvgl_top_ambient_menu_handle->send_flag = 0;
	
	pdivoom_lvgl_top_ambient_menu_handle->main_obj = lv_obj_create(lv_layer_top()); //在顶层创建主页面

	lv_obj_set_style_bg_opa(pdivoom_lvgl_top_ambient_menu_handle->main_obj, 0, 0);
	//lv_obj_set_style_bg_color(pdivoom_lvgl_top_ambient_menu_handle->main_obj, lv_color_hex(0x000000), 0);
	lv_obj_set_style_pad_all(pdivoom_lvgl_top_ambient_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_border_width(pdivoom_lvgl_top_ambient_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_shadow_width(pdivoom_lvgl_top_ambient_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_outline_width(pdivoom_lvgl_top_ambient_menu_handle->main_obj, 0, 0);
	lv_obj_clear_flag(pdivoom_lvgl_top_ambient_menu_handle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(pdivoom_lvgl_top_ambient_menu_handle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(pdivoom_lvgl_top_ambient_menu_handle->main_obj, 0, 0);
	
	
	divoom_lvgl_top_ambient_menu_create_icon();

	pdivoom_lvgl_top_ambient_menu_handle->menu_exit_timer = lv_timer_create(divoom_lvgl_top_ambient_menu_time_handle, 1000, NULL);

	//lv_group_focus_obj(pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj)
    divoom_lvgl_top_control_add_top(pdivoom_lvgl_top_ambient_menu_handle, pdivoom_lvgl_top_ambient_menu_handle->main_cout_obj);

	divoom_free_lvgl_lock();
}

void divoom_lvgl_top_ambient_menu_exit(void)
{
	if(pdivoom_lvgl_top_ambient_menu_handle != NULL)
	{
		if(pdivoom_lvgl_top_ambient_menu_handle->menu_exit_timer)
		{
			lv_timer_del(pdivoom_lvgl_top_ambient_menu_handle->menu_exit_timer);
		}
        divoom_print("aj test!\n");
        divoom_lvgl_top_control_remove_top(pdivoom_lvgl_top_ambient_menu_handle);
		
		lv_obj_del_async(pdivoom_lvgl_top_ambient_menu_handle->main_obj);
		divoom_system_free(pdivoom_lvgl_top_ambient_menu_handle);
		pdivoom_lvgl_top_ambient_menu_handle = NULL;
	}
}


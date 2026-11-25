#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_music_ctrl.h"
#include "divoom_lvgl_top_bright_volume_menu.h"
#include "divoom_lvgl_top_bright_volume_video.h"
#include "divoom_sleep_aid_ctrl.h"
#include "divoom_local_music_ctrl.h"
#include "divoom_lvgl_poweron.h"
#ifdef DIVOOM_PRODUCT_ASTRO_TOO
#include "divoom_astrootoo_tools.h"
#include "divoom_lvgl_top_leave_msg_notice_menu.h"
#include "divoom_audio_session.h"
#include "divoom_lvgl_lyric_dial.h"
#endif

#if defined(DIVOOM_TIME_FRAME_SPEAKER) && !defined(DIVOOM_PRODUCT_ASTRO_TOO)

/* MasterGo 549:59004 亮度 / 549:59473 音量：单列圆角面板，ENTER 切换模式 */
#define TBV_PANEL_X           636
#define TBV_PANEL_Y           312
#define TBV_PANEL_W           164
#define TBV_PANEL_H           354
#define TBV_PILL_X            32
#define TBV_PILL_Y            32
#define TBV_PILL_W            100
#define TBV_PILL_H            290
#define TBV_PILL_RADIUS       50
#define TBV_SLIDER_X          55
#define TBV_SLIDER_Y          56
#define TBV_SLIDER_W          54
#define TBV_SLIDER_H          242
#define TBV_ICON_X            66
#define TBV_ICON_Y            252

typedef struct _divoom_lvgl_top_bright_volume_menu_ctrl
{
	void *main_obj;
	void *panel_cont;
	void *panel_btn;
	void *slider;
	void *icon_img;

	int select_id;	/* 0:音量  1:亮度 */
	int volume;
	int bright;
    int sleep_flag;

	int exit_time;
	lv_timer_t *menu_exit_timer;
	lv_timer_t *bright_vol_timer;
}divoom_lvgl_top_bright_volume_menu_ctrl, *Pdivoom_lvgl_top_bright_volume_menu_ctrl;


Pdivoom_lvgl_top_bright_volume_menu_ctrl pdivoom_lvgl_top_bright_volume_menu_handle = NULL;

void divoom_lvgl_top_bright_volume_menu_del_menu(void)
{
	if(pdivoom_lvgl_top_bright_volume_menu_handle->menu_exit_timer)
	{
		lv_timer_del(pdivoom_lvgl_top_bright_volume_menu_handle->menu_exit_timer);
	}
	if(pdivoom_lvgl_top_bright_volume_menu_handle->bright_vol_timer)
	{
		lv_timer_del(pdivoom_lvgl_top_bright_volume_menu_handle->bright_vol_timer);
	}
    divoom_lvgl_top_control_remove_top(pdivoom_lvgl_top_bright_volume_menu_handle);
	lv_obj_del_async(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj);
	divoom_system_free(pdivoom_lvgl_top_bright_volume_menu_handle);
	pdivoom_lvgl_top_bright_volume_menu_handle = NULL;
}


void divoom_lvgl_top_bright_volume_menu_time_handle(lv_timer_t *exit_timer)
{
	if(pdivoom_lvgl_top_bright_volume_menu_handle != NULL)
	{
		pdivoom_lvgl_top_bright_volume_menu_handle->exit_time++;

		if(pdivoom_lvgl_top_bright_volume_menu_handle->exit_time >= DIVOOM_LVGL_TOP_BRIGHT_VOLUME_MENU_EXIT_TIME)//主菜单不变动自动退出
		{
			int screen_bright = divoom_main_get_sys_light();
			divoom_system_setting_birghtness_conf brightness;
			brightness.light = screen_bright;
		
			if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 1)
			{
				divoom_net_app_send_packet(DIVOOM_NET_COMM_SET_SYS_BRIGHTNESS, DIVOOM_SERVER_TOPIC, (void *)&brightness);
			}
            else if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 0)
            {
				divoom_main_save_config();
            }

			divoom_lvgl_top_bright_volume_menu_del_menu();
		}
	}
}

void divoom_lvgl_top_bright_volume_menu_bright_vol_time_handle(lv_timer_t *exit_timer)
{
	if(pdivoom_lvgl_top_bright_volume_menu_handle != NULL && pdivoom_lvgl_top_bright_volume_menu_handle->sleep_flag == 0)
	{
		int value;

		if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 1)
		{
		 	value = divoom_main_get_sys_light();
        }
		else if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 0)
		{
			value = divoom_music_ctrl_get_volume();
		}

		if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 1 && pdivoom_lvgl_top_bright_volume_menu_handle->bright != value)
		{
			pdivoom_lvgl_top_bright_volume_menu_handle->exit_time = 0;
			pdivoom_lvgl_top_bright_volume_menu_handle->bright = value;
			lv_slider_set_value(pdivoom_lvgl_top_bright_volume_menu_handle->slider, value, LV_ANIM_ON);
		}
        else if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 0 && pdivoom_lvgl_top_bright_volume_menu_handle->volume != value)
        {
			pdivoom_lvgl_top_bright_volume_menu_handle->exit_time = 0;
			pdivoom_lvgl_top_bright_volume_menu_handle->volume = value;
			lv_slider_set_value(pdivoom_lvgl_top_bright_volume_menu_handle->slider, value, LV_ANIM_ON);
        }
	}
}


static void divoom_lvgl_top_bright_volume_menu_slider_style(lv_obj_t *slider)
{
	if (!slider)
	{
		return;
	}
	lv_slider_set_range(slider, 0, 100);
	lv_obj_set_style_pad_all(slider, 0, 0);
	lv_obj_set_style_bg_color(slider, lv_color_hex(0x888888), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(slider, LV_OPA_80, LV_PART_MAIN);
	lv_obj_set_style_radius(slider, 27, LV_PART_MAIN);
	lv_obj_set_style_bg_color(slider, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR);
	lv_obj_set_style_radius(slider, 0, LV_PART_INDICATOR);
	lv_obj_set_style_bg_opa(slider, LV_OPA_0, LV_PART_KNOB);
}

static void divoom_lvgl_top_bright_volume_menu_apply_mode(void)
{
	Pdivoom_lvgl_top_bright_volume_menu_ctrl ph;
	lv_obj_t *slider;
	lv_obj_t *icon;

	ph = pdivoom_lvgl_top_bright_volume_menu_handle;
	if (!ph || !ph->slider || !ph->icon_img)
	{
		return;
	}
	slider = (lv_obj_t *)ph->slider;
	icon = (lv_obj_t *)ph->icon_img;

	if (ph->select_id == 1)
	{
		ph->bright = divoom_main_get_sys_light();
		lv_slider_set_value(slider, ph->bright, LV_ANIM_OFF);
		divoom_menu_control_lv_img_set_src(icon, "26/26_top_bright2.png");
	}
	else
	{
		ph->volume = divoom_music_ctrl_get_volume();
		lv_slider_set_value(slider, ph->volume, LV_ANIM_OFF);
		divoom_menu_control_lv_img_set_src(icon, "26/26_top_volume2.png");
	}
	lv_obj_set_style_img_recolor(icon, lv_color_hex(0x000000), 0);
}

//主菜单回调
static void divoom_lvgl_top_bright_volume_menu_main_cout_event_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_KEY)
    {
		uint32 para = *((uint32 *)lv_event_get_param(e));
        char c = (para&0xff);

		if(pdivoom_lvgl_top_bright_volume_menu_handle == NULL)
		{
			divoom_print("test\n");
			return;
		}

		pdivoom_lvgl_top_bright_volume_menu_handle->exit_time = 0;
		
		if(c == DIVOOM_KEY_LIGHT_LEFT)
		{
			int value;
            

            if(pdivoom_lvgl_top_bright_volume_menu_handle->sleep_flag)
            {
                value = lv_slider_get_value(pdivoom_lvgl_top_bright_volume_menu_handle->slider);            
            }
            else
            {
    			if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 1)
    			{
    			 	value = lv_slider_get_value(pdivoom_lvgl_top_bright_volume_menu_handle->slider);
                }
    			else
    			{
    				value = divoom_music_ctrl_get_volume();
    			}
            }

			if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 1)
			{
				if(value == 0)
				{
					value = 1;
				}
				else if(value == 1)
				{
					value = 5;
				}
				else  if(value)
				{
					value = (value < 95) ? (value + 5) : (100);
				}
			}
			else
			{
                if(pdivoom_lvgl_top_bright_volume_menu_handle->sleep_flag)
                {
				    value = (value < 98) ? (value + 2) : (100);
                }
                else
                {
				    value = (value < 95) ? (value + 5) : (100);
                }
			}
			

            if(pdivoom_lvgl_top_bright_volume_menu_handle->sleep_flag)
            {
                Pdivoom_sleep_aid_info psleep_config = (divoom_sleep_aid_info *)divoom_sleep_aid_ctrl_get_settings();
    			if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 1)
    			{
    				pdivoom_lvgl_top_bright_volume_menu_handle->bright = value;
    				lv_slider_set_value(pdivoom_lvgl_top_bright_volume_menu_handle->slider, value, LV_ANIM_ON);
    				divoom_main_set_sleep_brightness((uint8)value);
                    if(divoom_sleep_aid_ctrl_get_play_time_phase() == DIVOOM_SLEEP_AID_PLAY_TIME_PHASE_BEFORE)
                    {
                        psleep_config->before_bright = value;
                    }
                    else
                    {
                        psleep_config->after_bright = value;
                    }
    			}
                else if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 0)
                {
    				pdivoom_lvgl_top_bright_volume_menu_handle->volume = value;
    				lv_slider_set_value(pdivoom_lvgl_top_bright_volume_menu_handle->slider, value, LV_ANIM_ON);
    				divoom_sleep_aid_ctrl_set_voice_vol(value);
                    if(divoom_sleep_aid_ctrl_get_play_time_phase() == DIVOOM_SLEEP_AID_PLAY_TIME_PHASE_BEFORE)
                    {
                        psleep_config->before_vol = value;
                    }
                    else
                    {
                        psleep_config->after_vol = value;
                    }
                }
            }
            else
            {

    			//lv_obj_invalidate(pdivoom_lvgl_top_bright_volume_menu_handle->bright_volume_slider);
    			if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 1)
    			{
    				pdivoom_lvgl_top_bright_volume_menu_handle->bright = value;
    				lv_slider_set_value(pdivoom_lvgl_top_bright_volume_menu_handle->slider, value, LV_ANIM_ON);
    				divoom_main_set_sys_light(value);
    			}
                else if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 0)
                {
    				pdivoom_lvgl_top_bright_volume_menu_handle->volume = value;
    				lv_slider_set_value(pdivoom_lvgl_top_bright_volume_menu_handle->slider, value, LV_ANIM_ON);
    				divoom_music_ctrl_set_volume(value);
                }
            }
		}
		else if(c == DIVOOM_KEY_LIGHT_RIGHT)
		{
			int value;
            
            if(pdivoom_lvgl_top_bright_volume_menu_handle->sleep_flag)
            {
                value = lv_slider_get_value(pdivoom_lvgl_top_bright_volume_menu_handle->slider);                
            }
            else
            {

    			if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 1)
    			{
    			 	value = lv_slider_get_value(pdivoom_lvgl_top_bright_volume_menu_handle->slider);
                }
    			else
    			{
    				value = divoom_music_ctrl_get_volume();
    			}
            }
            
            if(pdivoom_lvgl_top_bright_volume_menu_handle->sleep_flag)
            {
    			if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 1)
    			{
    				if(value == 1)
    				{
    					value = 0;
    				}
    				else if(value)
    				{
    					value = (value > 5) ? (value - 5) : (1);
    				}
    			}
    			else
    			{
                    value = (value > 2) ? (value - 2) : (0);
    			}
            }
            else
            {
    			if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 1)
    			{
    				if(value == 1)
    				{
    					value = 0;
    				}
    				else if(value)
    				{
    					value = (value > 5) ? (value - 5) : (1);
    				}
    			}
    			else
    			{
                    value = (value > 5) ? (value - 5) : (0);
    			}
            }

			
			//lv_obj_invalidate(pdivoom_lvgl_top_bright_volume_menu_handle->bright_volume_slider);
            if(pdivoom_lvgl_top_bright_volume_menu_handle->sleep_flag)
            {
                Pdivoom_sleep_aid_info psleep_config = (divoom_sleep_aid_info *)divoom_sleep_aid_ctrl_get_settings();
    			if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 1)
    			{
    				pdivoom_lvgl_top_bright_volume_menu_handle->bright = value;
    				lv_slider_set_value(pdivoom_lvgl_top_bright_volume_menu_handle->slider, value, LV_ANIM_ON);
    				divoom_main_set_sleep_brightness((uint8)value);
                    if(divoom_sleep_aid_ctrl_get_play_time_phase() == DIVOOM_SLEEP_AID_PLAY_TIME_PHASE_BEFORE)
                    {
                        psleep_config->before_bright = value;
                    }
                    else
                    {
                        psleep_config->after_bright = value;
                    }
    			}
                else if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 0)
                {
    				pdivoom_lvgl_top_bright_volume_menu_handle->volume = value;
    				lv_slider_set_value(pdivoom_lvgl_top_bright_volume_menu_handle->slider, value, LV_ANIM_ON);
    				divoom_sleep_aid_ctrl_set_voice_vol(value);
                    if(divoom_sleep_aid_ctrl_get_play_time_phase() == DIVOOM_SLEEP_AID_PLAY_TIME_PHASE_BEFORE)
                    {
                        psleep_config->before_vol = value;
                    }
                    else
                    {
                        psleep_config->after_vol = value;
                    }
                }
            }
            else
            {
    			if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 1)
    			{
    				pdivoom_lvgl_top_bright_volume_menu_handle->bright = value;
    				lv_slider_set_value(pdivoom_lvgl_top_bright_volume_menu_handle->slider, value, LV_ANIM_ON);
    				divoom_main_set_sys_light(value);
    			}
                else if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 0)
                {
    				pdivoom_lvgl_top_bright_volume_menu_handle->volume = value;
    				lv_slider_set_value(pdivoom_lvgl_top_bright_volume_menu_handle->slider, value, LV_ANIM_ON);
    				divoom_music_ctrl_set_volume(value);
                }
            }
		}
		else if(c == DIVOOM_KEY_LIGHT_ENTER)
		{
			if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 0)
			{
				pdivoom_lvgl_top_bright_volume_menu_handle->select_id = 1;
			}
			else
			{
				pdivoom_lvgl_top_bright_volume_menu_handle->select_id = 0;
			}
			divoom_lvgl_top_bright_volume_menu_apply_mode();
		}
		else
		{
			char *key_info = (char *)lv_event_get_param(e);
		
			divoom_lvgl_top_bright_volume_menu_del_menu();
			divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
		}
	}
}


void divoom_lvgl_top_bright_volume_menu_create_icon(void)
{
	lv_obj_t *cont;
	lv_obj_t *pill;
	lv_obj_t *panel_btn;
	lv_obj_t *slider;
	lv_obj_t *icon;

	cont = lv_obj_create(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj);
	lv_obj_set_size(cont, TBV_PANEL_W, TBV_PANEL_H);
	lv_obj_set_pos(cont, TBV_PANEL_X, TBV_PANEL_Y);
	lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_clip_corner(cont, true, 0);
	lv_obj_set_style_pad_all(cont, 0, 0);
	lv_obj_set_style_border_width(cont, 0, 0);
	lv_obj_set_style_radius(cont, 0, 0);
	lv_obj_set_style_shadow_width(cont, 0, 0);
	lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
	pdivoom_lvgl_top_bright_volume_menu_handle->panel_cont = cont;

	pill = lv_obj_create(cont);
	lv_obj_set_size(pill, TBV_PILL_W, TBV_PILL_H);
	lv_obj_set_pos(pill, TBV_PILL_X, TBV_PILL_Y);
	lv_obj_clear_flag(pill, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_pad_all(pill, 0, 0);
	lv_obj_set_style_border_width(pill, 1, 0);
	lv_obj_set_style_border_color(pill, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_border_opa(pill, LV_OPA_60, 0);
	lv_obj_set_style_radius(pill, TBV_PILL_RADIUS, 0);
	lv_obj_set_style_shadow_width(pill, 0, 0);
    if(pdivoom_lvgl_top_bright_volume_menu_handle->sleep_flag)
    {
	    lv_obj_set_style_bg_color(pill, lv_color_hex(0x303033), 0);
    }
    else
    {
	    lv_obj_set_style_bg_color(pill, lv_color_hex(0xE4E4E4), 0);
    }
	lv_obj_set_style_bg_opa(pill, LV_OPA_COVER, 0);

	panel_btn = lv_btn_create(cont);
	lv_obj_set_size(panel_btn, TBV_SLIDER_W + 12, TBV_SLIDER_H + 12);
	lv_obj_set_pos(panel_btn, TBV_SLIDER_X - 6, TBV_SLIDER_Y - 6);
	lv_obj_clear_flag(panel_btn, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_pad_all(panel_btn, 0, 0);
	lv_obj_set_style_border_width(panel_btn, 0, 0);
	lv_obj_set_style_radius(panel_btn, 27, 0);
	lv_obj_set_style_shadow_width(panel_btn, 0, 0);
	lv_obj_set_style_outline_width(panel_btn, 0, 0);
	lv_obj_set_style_outline_width(panel_btn, 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_bg_opa(panel_btn, LV_OPA_TRANSP, 0);
	pdivoom_lvgl_top_bright_volume_menu_handle->panel_btn = panel_btn;
	lv_obj_add_event_cb(panel_btn, divoom_lvgl_top_bright_volume_menu_main_cout_event_cb, LV_EVENT_KEY, NULL);

	slider = lv_slider_create(panel_btn);
	lv_obj_set_size(slider, TBV_SLIDER_W, TBV_SLIDER_H);
	lv_obj_center(slider);
	divoom_lvgl_top_bright_volume_menu_slider_style(slider);
	pdivoom_lvgl_top_bright_volume_menu_handle->slider = slider;

	icon = lv_img_create(cont);
	lv_obj_set_pos(icon, TBV_ICON_X, TBV_ICON_Y);
	lv_obj_set_style_img_recolor_opa(icon, LV_OPA_COVER, 0);
	pdivoom_lvgl_top_bright_volume_menu_handle->icon_img = icon;

    if(pdivoom_lvgl_top_bright_volume_menu_handle->sleep_flag)
    {
        Pdivoom_sleep_aid_info psleep_config = (divoom_sleep_aid_info *)divoom_sleep_aid_ctrl_get_settings();
        if(divoom_sleep_aid_ctrl_get_play_time_phase() == DIVOOM_SLEEP_AID_PLAY_TIME_PHASE_BEFORE)
        {
            pdivoom_lvgl_top_bright_volume_menu_handle->volume = psleep_config->before_vol;
            pdivoom_lvgl_top_bright_volume_menu_handle->bright = psleep_config->before_bright;
        }
        else
        {
            pdivoom_lvgl_top_bright_volume_menu_handle->volume = psleep_config->after_vol;
            pdivoom_lvgl_top_bright_volume_menu_handle->bright = psleep_config->after_bright;
        }
        if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 1)
        {
            lv_slider_set_value(pdivoom_lvgl_top_bright_volume_menu_handle->slider, pdivoom_lvgl_top_bright_volume_menu_handle->bright, LV_ANIM_ON);
        }
        else
        {
            lv_slider_set_value(pdivoom_lvgl_top_bright_volume_menu_handle->slider, pdivoom_lvgl_top_bright_volume_menu_handle->volume, LV_ANIM_ON);
        }
        
    }
    else
    {
        pdivoom_lvgl_top_bright_volume_menu_handle->volume = divoom_music_ctrl_get_volume();
        pdivoom_lvgl_top_bright_volume_menu_handle->bright = divoom_main_get_sys_light();
    }
	divoom_lvgl_top_bright_volume_menu_apply_mode();
}


void divoom_lvgl_top_bright_volume_menu_init(int bright_volume_flag)//1:亮度 2:视频音量3：提示音音量
{
	divoom_get_lvgl_lock();

	if(pdivoom_lvgl_top_bright_volume_menu_handle != NULL)
	{
		divoom_free_lvgl_lock();
		return;
	}
	
	pdivoom_lvgl_top_bright_volume_menu_handle = (Pdivoom_lvgl_top_bright_volume_menu_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_top_bright_volume_menu_ctrl));
	
	if(pdivoom_lvgl_top_bright_volume_menu_handle == NULL)
	{
		divoom_free_lvgl_lock();
		return;
	}
	memset(pdivoom_lvgl_top_bright_volume_menu_handle, 0, sizeof(divoom_lvgl_top_bright_volume_menu_ctrl));

	if(bright_volume_flag == 1)
	{
		pdivoom_lvgl_top_bright_volume_menu_handle->select_id = 1;
	}
	else
	{
		pdivoom_lvgl_top_bright_volume_menu_handle->select_id = 0;
	}
    if(divoom_menu_control_get_active_menu_id() == DIVOOM_MENU_CONTROL_ID_SLEEP_AID_ACTIVE)
    {
        pdivoom_lvgl_top_bright_volume_menu_handle->sleep_flag = 1;
    }
	pdivoom_lvgl_top_bright_volume_menu_handle->exit_time = 0;
	
	pdivoom_lvgl_top_bright_volume_menu_handle->main_obj = lv_btn_create(lv_layer_top()); //在顶层创建主页面

	lv_obj_remove_style_all(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj);
	lv_obj_set_style_bg_opa(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj, 0, 0);
	//lv_obj_set_style_bg_color(pdivoom_lvgl_top_ambient_menu_handle->main_obj, lv_color_hex(0x000000), 0);
	lv_obj_set_style_pad_all(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_border_width(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_shadow_width(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_outline_width(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj, 0, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_clear_flag(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj, 0, 0);
	
	//lv_obj_add_event_cb(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj, divoom_lvgl_top_bright_volume_menu_main_cout_event_cb, LV_EVENT_KEY, NULL);
	
	divoom_lvgl_top_bright_volume_menu_create_icon();

	pdivoom_lvgl_top_bright_volume_menu_handle->menu_exit_timer = lv_timer_create(divoom_lvgl_top_bright_volume_menu_time_handle, 1000, NULL);
	pdivoom_lvgl_top_bright_volume_menu_handle->bright_vol_timer = lv_timer_create(divoom_lvgl_top_bright_volume_menu_bright_vol_time_handle, 100, NULL);

    divoom_lvgl_top_control_add_top(pdivoom_lvgl_top_bright_volume_menu_handle, pdivoom_lvgl_top_bright_volume_menu_handle->panel_btn);
	lv_group_focus_obj(pdivoom_lvgl_top_bright_volume_menu_handle->panel_btn);

	divoom_free_lvgl_lock();
}

void divoom_lvgl_top_bright_volume_menu_exit(void)
{
	if(pdivoom_lvgl_top_bright_volume_menu_handle != NULL)
	{
		if(pdivoom_lvgl_top_bright_volume_menu_handle->menu_exit_timer)
		{
			lv_timer_del(pdivoom_lvgl_top_bright_volume_menu_handle->menu_exit_timer);
		}
		if(pdivoom_lvgl_top_bright_volume_menu_handle->bright_vol_timer)
		{
			lv_timer_del(pdivoom_lvgl_top_bright_volume_menu_handle->bright_vol_timer);
		}
        divoom_lvgl_top_control_remove_top(pdivoom_lvgl_top_bright_volume_menu_handle);
		lv_obj_del_async(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj);
		divoom_system_free(pdivoom_lvgl_top_bright_volume_menu_handle);
		pdivoom_lvgl_top_bright_volume_menu_handle = NULL;
	}
}

#elif defined(DIVOOM_PRODUCT_ASTRO_TOO)

#include "divoom_astrootoo_sleep.h"
#include "divoom_astrootoo_settings.h"
#include "divoom_lvgl_top_control.h"

/* AstroTooUI FlowtooVolumeOsd / FlowtooBrightnessOsd（480×480 顶栏水平条） */
#define DIVOOM_ASTRO_OSD_W              316
#define DIVOOM_ASTRO_OSD_H              40
#define DIVOOM_ASTRO_OSD_Y              32
#define DIVOOM_ASTRO_OSD_X              ((DIVOOM_SCREEN_WIDTH - DIVOOM_ASTRO_OSD_W) / 2)
#define DIVOOM_ASTRO_OSD_RADIUS         12
#define DIVOOM_ASTRO_OSD_BORDER         2
#define DIVOOM_ASTRO_OSD_INNER_W        (DIVOOM_ASTRO_OSD_W - DIVOOM_ASTRO_OSD_BORDER * 2)
#define DIVOOM_ASTRO_OSD_INNER_H        (DIVOOM_ASTRO_OSD_H - DIVOOM_ASTRO_OSD_BORDER * 2)
#define DIVOOM_ASTRO_OSD_ICON           24
#define DIVOOM_ASTRO_OSD_ICON_X         8
#define DIVOOM_ASTRO_OSD_UI_DIR         "82/"
#define DIVOOM_ASTRO_OSD_VOL_ICON       DIVOOM_ASTRO_OSD_UI_DIR "at_osd_volume_speaker.png"
#define DIVOOM_ASTRO_OSD_BRIGHT_ICON    DIVOOM_ASTRO_OSD_UI_DIR "at_osd_brightness_sun.png"
#define DIVOOM_ASTRO_OSD_DISMISS_TICKS  11   /* 11 × 200ms = 2200ms */
#if 0
#define DIVOOM_ASTRO_VOL_LONG_MS        2000
#endif
#define DIVOOM_ASTRO_BRIGHT_REPEAT_MS   350
#define DIVOOM_ASTRO_BRIGHTNESS_STEP_NORMAL 5
#define DIVOOM_ASTRO_SLEEP_VOLUME_STEP  2

typedef enum
{
    DIVOOM_ASTRO_OSD_MODE_VOLUME = 0,
    DIVOOM_ASTRO_OSD_MODE_BRIGHTNESS = 1,
} divoom_astro_osd_mode_t;

typedef struct _divoom_lvgl_top_bright_volume_menu_ctrl
{
    lv_obj_t *main_obj;
    lv_obj_t *fill_obj;
    lv_obj_t *icon_obj;
    lv_timer_t *menu_exit_timer;
    lv_style_t bar_style;
    lv_style_t fill_style;
    lv_style_t cont_style;
    divoom_astro_osd_mode_t mode;
    int fill_percent;
    int exit_time;
    uint8 styles_initialized;
    uint8 video_composited; /* 视频歌词使用原音量条样式，由歌词 OSD 合成。 */
    /* 录音回放会话音量：OSD 重建会 memset UI，这两项须单独保留 */
    uint8 clip_vol;
    uint8 clip_vol_valid;
} divoom_lvgl_top_bright_volume_menu_ctrl, *Pdivoom_lvgl_top_bright_volume_menu_ctrl;

#if 0
typedef struct
{
    uint8 active;
    uint8 key;
    int8 delta;
    uint8 brightness_mode;
    lv_timer_t *long_timer;
    lv_timer_t *repeat_timer;
} divoom_astro_osd_hold_t;
#endif

static divoom_lvgl_top_bright_volume_menu_ctrl g_astro_osd_ctrl = {0};
static Pdivoom_lvgl_top_bright_volume_menu_ctrl pdivoom_lvgl_top_bright_volume_menu_handle = NULL;
static uint32_t s_astro_bright_last_step_ms = 0;
#if 0
static divoom_astro_osd_hold_t s_astro_osd_hold;
#endif

static const uint8 s_astro_brightness_steps[] = {0, 20, 40, 60, 80, 100};
#define DIVOOM_ASTRO_BRIGHTNESS_SLOT_MAX 5

/* 视频全屏播放页：LVGL 被盖住，OSD 走 TOP 层；亮度用助眠档位。 */
static uint8 divoom_astro_osd_is_sleep_video_page(void)
{
    return divoom_menu_control_get_active_menu_id() == DIVOOM_MENU_CONTROL_ID_SLEEP_AID_ACTIVE;
}

/* 助眠页内：仅助眠音在播时调 before/after_vol；否则走媒体音量。APP 试听始终调助眠音量。 */
static uint8 divoom_astro_osd_is_sleep_aid_ui(void)
{
    int mid = divoom_menu_control_get_active_menu_id();

    return (mid == DIVOOM_MENU_CONTROL_ID_SLEEP_AID
        || mid == DIVOOM_MENU_CONTROL_ID_SLEEP_AID_ACTIVE);
}

static uint8 divoom_astro_osd_is_sleep_volume(void)
{
    if (divoom_sleep_aid_ctrl_app_preview_is_active())
    {
        return 1;
    }
    if (!divoom_astro_osd_is_sleep_aid_ui())
    {
        return 0;
    }
    if (divoom_sleep_aid_ctrl_is_sleep_voice_audible())
    {
        return 1;
    }
    /* 列表短试听：tone 占用 session，仍对应助眠 before_vol */
    if (divoom_local_music_ctrl_get_tone_status()
        && divoom_audio_session_get_state() == DIVOOM_AUDIO_SESSION_STATE_TONE)
    {
        return 1;
    }
    return 0;
}

/* 仅工具录音回放、留言顶栏试听：VOL 同步 tone_lite 数字音量 */
static int divoom_astro_osd_clip_live_vol_allowed(void)
{
    if (divoom_astrootoo_tools_memo_is_playing())
        return 1;
    if (divoom_lvgl_top_leave_msg_notice_is_playing())
        return 1;
    return 0;
}

static void divoom_astro_osd_alloc_ui_ctrl(void)
{
    uint8 clip_vol;
    uint8 clip_vol_valid;

    if (pdivoom_lvgl_top_bright_volume_menu_handle != NULL)
        return;
    clip_vol = g_astro_osd_ctrl.clip_vol;
    clip_vol_valid = g_astro_osd_ctrl.clip_vol_valid;
    memset(&g_astro_osd_ctrl, 0, sizeof(g_astro_osd_ctrl));
    g_astro_osd_ctrl.clip_vol = clip_vol;
    g_astro_osd_ctrl.clip_vol_valid = clip_vol_valid;
    pdivoom_lvgl_top_bright_volume_menu_handle = &g_astro_osd_ctrl;
}

static void divoom_lvgl_top_bright_volume_menu_create_ui(Pdivoom_lvgl_top_bright_volume_menu_ctrl phandle,
    const char *icon_file);
static void divoom_lvgl_top_bright_volume_menu_show_osd(Pdivoom_lvgl_top_bright_volume_menu_ctrl phandle,
    divoom_astro_osd_mode_t mode, int fill_percent, uint8 video_composited);

static void divoom_lvgl_top_bright_volume_menu_reset_styles(Pdivoom_lvgl_top_bright_volume_menu_ctrl phandle)
{
    if (phandle && phandle->styles_initialized)
    {
        lv_style_reset(&phandle->bar_style);
        lv_style_reset(&phandle->fill_style);
        lv_style_reset(&phandle->cont_style);
        phandle->styles_initialized = 0;
    }
}

#if 0
static void divoom_astro_osd_hold_clear(void)
{
    if (s_astro_osd_hold.long_timer)
    {
        lv_timer_del(s_astro_osd_hold.long_timer);
        s_astro_osd_hold.long_timer = NULL;
    }
    if (s_astro_osd_hold.repeat_timer)
    {
        lv_timer_del(s_astro_osd_hold.repeat_timer);
        s_astro_osd_hold.repeat_timer = NULL;
    }
    memset(&s_astro_osd_hold, 0, sizeof(s_astro_osd_hold));
}
#endif

static void divoom_lvgl_top_bright_volume_menu_del_menu(void)
{
    if (pdivoom_lvgl_top_bright_volume_menu_handle == NULL)
    {
        return;
    }

    if (pdivoom_lvgl_top_bright_volume_menu_handle->menu_exit_timer)
    {
        lv_timer_del(pdivoom_lvgl_top_bright_volume_menu_handle->menu_exit_timer);
        pdivoom_lvgl_top_bright_volume_menu_handle->menu_exit_timer = NULL;
    }

    if (pdivoom_lvgl_top_bright_volume_menu_handle->main_obj)
    {
        lv_obj_del_async(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj);
        pdivoom_lvgl_top_bright_volume_menu_handle->main_obj = NULL;
    }

    divoom_lvgl_top_bright_volume_menu_reset_styles(pdivoom_lvgl_top_bright_volume_menu_handle);
    pdivoom_lvgl_top_bright_volume_menu_handle = NULL;
}

static void divoom_lvgl_top_bright_volume_menu_time_handle(lv_timer_t *exit_timer)
{
    (void)exit_timer;

    if (pdivoom_lvgl_top_bright_volume_menu_handle == NULL)
    {
        return;
    }

    pdivoom_lvgl_top_bright_volume_menu_handle->exit_time++;
    if (pdivoom_lvgl_top_bright_volume_menu_handle->exit_time >= DIVOOM_ASTRO_OSD_DISMISS_TICKS)
    {
        if (pdivoom_lvgl_top_bright_volume_menu_handle->mode == DIVOOM_ASTRO_OSD_MODE_BRIGHTNESS)
        {
            divoom_main_upload_sys_brightness((uint8)pdivoom_lvgl_top_bright_volume_menu_handle->fill_percent);
        }

        divoom_lvgl_top_bright_volume_menu_del_menu();
    }
}

static int divoom_astro_osd_brightness_to_slot(uint8 brightness)
{
    int i;
    int best_slot = 0;
    int best_diff = 1000;

    for (i = 0; i <= DIVOOM_ASTRO_BRIGHTNESS_SLOT_MAX; i++)
    {
        int diff;

        diff = (int)s_astro_brightness_steps[i] - (int)brightness;
        if (diff < 0)
        {
            diff = -diff;
        }
        if (diff < best_diff)
        {
            best_diff = diff;
            best_slot = i;
        }
    }
    return best_slot;
}

static void divoom_astro_osd_apply_brightness_percent(int percent)
{
    if (percent < 0)
    {
        percent = 0;
    }
    else if (percent > 100)
    {
        percent = 100;
    }

    if (divoom_astro_osd_is_sleep_video_page())
    {
        divoom_main_set_sleep_brightness((uint8)percent);
    }
    else
    {
        divoom_main_set_sys_light((uint8)percent);
        divoom_astrootoo_settings_notify_sys_brightness();
    }
}

static void divoom_lvgl_top_bright_volume_menu_update_fill(Pdivoom_lvgl_top_bright_volume_menu_ctrl phandle)
{
    lv_coord_t fill_w;

    if (phandle == NULL || phandle->fill_obj == NULL)
    {
        return;
    }

    if (phandle->fill_percent <= 0)
    {
        fill_w = 0;
        lv_obj_add_flag(phandle->fill_obj, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        fill_w = (lv_coord_t)((phandle->fill_percent * DIVOOM_ASTRO_OSD_INNER_W + 50) / 100);
        if (fill_w > DIVOOM_ASTRO_OSD_INNER_W)
        {
            fill_w = DIVOOM_ASTRO_OSD_INNER_W;
        }
        lv_obj_clear_flag(phandle->fill_obj, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_set_width(phandle->fill_obj, fill_w);
}

static void divoom_lvgl_top_bright_volume_menu_show_osd(Pdivoom_lvgl_top_bright_volume_menu_ctrl phandle,
    divoom_astro_osd_mode_t mode, int fill_percent, uint8 video_composited)
{
    const char *icon_file;

    if (phandle == NULL)
    {
        return;
    }

    phandle->mode = mode;
    phandle->video_composited = video_composited;
    phandle->fill_percent = fill_percent;
    phandle->exit_time = 0;

    icon_file = (mode == DIVOOM_ASTRO_OSD_MODE_BRIGHTNESS)
        ? DIVOOM_ASTRO_OSD_BRIGHT_ICON
        : DIVOOM_ASTRO_OSD_VOL_ICON;

    if (phandle->main_obj == NULL)
    {
        divoom_lvgl_top_bright_volume_menu_create_ui(phandle, icon_file);
    }
    else if (phandle->icon_obj)
    {
        divoom_menu_control_lv_img_set_src(phandle->icon_obj, icon_file);
    }

    divoom_lvgl_top_bright_volume_menu_update_fill(phandle);
    if (phandle->main_obj)
    {
#if !defined(DIVOOM_SIMULATOR) || !(DIVOOM_SIMULATOR)
        if (phandle->video_composited)
        {
            /* 原控件只在歌词快照中绘制；普通 UI 刷新不额外创建音量条图层。 */
            lv_obj_update_layout(phandle->main_obj);
            lv_obj_add_flag(phandle->main_obj, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_HIDDEN);
        }
#endif
        divoom_lvgl_layer_top_raise(phandle->main_obj);
    }
}

void divoom_lvgl_top_bright_volume_menu_show_composited(int brightness_mode, int fill_percent)
{
    divoom_astro_osd_alloc_ui_ctrl();
    divoom_lvgl_top_bright_volume_menu_show_osd(pdivoom_lvgl_top_bright_volume_menu_handle,
        brightness_mode ? DIVOOM_ASTRO_OSD_MODE_BRIGHTNESS : DIVOOM_ASTRO_OSD_MODE_VOLUME,
        fill_percent, 1);
}

void *divoom_lvgl_top_bright_volume_menu_get_composited_obj(void)
{
    Pdivoom_lvgl_top_bright_volume_menu_ctrl phandle = pdivoom_lvgl_top_bright_volume_menu_handle;
    return phandle && phandle->video_composited ? phandle->main_obj : NULL;
}

void divoom_lvgl_top_bright_volume_menu_exit_composited(void)
{
    if (divoom_lvgl_top_bright_volume_menu_get_composited_obj() != NULL)
    {
        divoom_lvgl_top_bright_volume_menu_del_menu();
    }
}

static void divoom_lvgl_top_bright_volume_menu_pulse_volume_locked(int fill_percent)
{
    if (divoom_lvgl_poweron_is_boot_pending())
    {
        divoom_print("osd volume: skip during poweron\n");
        return;
    }

    if (fill_percent < 0)
    {
        fill_percent = 0;
    }
    else if (fill_percent > 100)
    {
        fill_percent = 100;
    }

    if (divoom_lvgl_lyric_dial_is_video_active())
    {
        /* 复用正常音量条的样式，借助歌词已有 TOP OSD 显示到视频上。 */
        divoom_lvgl_top_bright_volume_video_exit();
        divoom_lvgl_top_bright_volume_menu_show_composited(0, fill_percent);
        return;
    }
    if (divoom_astro_osd_is_sleep_video_page())
    {
        divoom_lvgl_top_bright_volume_menu_del_menu();
        divoom_lvgl_top_bright_volume_video_show(0, fill_percent);
        return;
    }
    divoom_lvgl_top_bright_volume_video_exit();
    divoom_astro_osd_alloc_ui_ctrl();
    divoom_lvgl_top_bright_volume_menu_show_osd(pdivoom_lvgl_top_bright_volume_menu_handle,
        DIVOOM_ASTRO_OSD_MODE_VOLUME, fill_percent, 0);
    divoom_astrootoo_sleep_sync_hw_volume_sliders();
}

static void divoom_lvgl_top_bright_volume_menu_pulse_volume(int fill_percent)
{
    divoom_get_lvgl_lock();
    divoom_lvgl_top_bright_volume_menu_pulse_volume_locked(fill_percent);
    divoom_free_lvgl_lock();
}

void divoom_lvgl_top_bright_volume_menu_on_hfp_call_volume(uint8 ui_percent)
{
    if (divoom_astro_osd_is_sleep_volume())
        return;
    divoom_lvgl_top_bright_volume_menu_pulse_volume((int)ui_percent);
}

static void divoom_lvgl_top_bright_volume_menu_pulse_brightness_locked(int percent)
{
    if (divoom_lvgl_lyric_dial_is_video_active())
    {
        divoom_lvgl_top_bright_volume_video_exit();
        divoom_lvgl_top_bright_volume_menu_show_composited(1, percent);
        return;
    }
    if (divoom_astro_osd_is_sleep_video_page())
    {
        divoom_lvgl_top_bright_volume_menu_del_menu();
        divoom_lvgl_top_bright_volume_video_show(1, percent);
        return;
    }
    divoom_lvgl_top_bright_volume_video_exit();
    divoom_astro_osd_alloc_ui_ctrl();
    divoom_lvgl_top_bright_volume_menu_show_osd(pdivoom_lvgl_top_bright_volume_menu_handle,
        DIVOOM_ASTRO_OSD_MODE_BRIGHTNESS, percent, 0);
}

static void divoom_lvgl_top_bright_volume_menu_pulse_brightness(int percent)
{
    divoom_get_lvgl_lock();
    divoom_lvgl_top_bright_volume_menu_pulse_brightness_locked(percent);
    divoom_free_lvgl_lock();
}

static int divoom_lvgl_top_bright_volume_menu_hint_low_if_needed(uint8 cur_vol, uint8 req_vol)
{
	uint8 cap;

	if (req_vol <= cur_vol)
		return 0;
	if (divoom_is_charge_sta())
		return 0;
	if (!PowerVolIsLowPower())
		return 0;
	if (divoom_lvgl_poweron_is_boot_pending())
		return 0;

	cap = divoom_com_convert_vol(divoom_is_charge_sta() ? 8 : 3);
	if (cur_vol < cap)
		return 0;

	if (divoom_lvgl_top_battery_menu_low_tip_active())
		return 1;

	divoom_sys_msg_info model_msg = {0};
	uint8 *bat_stat = (uint8 *)divoom_system_malloc(1);
	if (!bat_stat)
		return 1;
	*bat_stat = BATTERY_STAT_LOW_WARN;
	model_msg.msg_type = 1;
	model_msg.src_id = DIVOOM_MODEL_MAIN;
	model_msg.dest_id = DIVOOM_MODEL_MAIN;
	model_msg.msg_id = DIVOOM_MAIN_EVENT_BATTERY;
	model_msg.data = bat_stat;
	model_msg.data_len = 1;
	divoom_sys_model_crl_msg_push(&model_msg);

	return 1;
}

extern int divoom_lvgl_top_battery_menu_low_tip_active(void);
static void divoom_astro_osd_step_volume(int8 delta)
{
    divoom_sleep_aid_info *sleep_info;
    int phase;
    int sleep_vol;
    uint8 vol;
    uint8 new_vol;

    if (divoom_astro_osd_is_sleep_volume())
    {
        sleep_info = divoom_sleep_aid_ctrl_get_settings();
        if (!sleep_info)
        {
            return;
        }
        phase = divoom_sleep_aid_ctrl_get_play_time_phase();
        sleep_vol = (phase == DIVOOM_SLEEP_AID_PLAY_TIME_PHASE_AFTER)
            ? sleep_info->after_vol : sleep_info->before_vol;
        if (delta > 0)
        {
            sleep_vol += DIVOOM_ASTRO_SLEEP_VOLUME_STEP;
            if (sleep_vol > 100)
            {
                sleep_vol = 100;
            }
        }
        else
        {
            sleep_vol -= DIVOOM_ASTRO_SLEEP_VOLUME_STEP;
            if (sleep_vol < 0)
            {
                sleep_vol = 0;
            }
        }
        if (phase == DIVOOM_SLEEP_AID_PLAY_TIME_PHASE_AFTER)
        {
            sleep_info->after_vol = sleep_vol;
        }
        else
        {
            sleep_info->before_vol = sleep_vol;
        }
        divoom_sleep_aid_ctrl_set_voice_vol(sleep_vol);
        divoom_print("sleep volume step: phase:%d delta:%d value:%d\n",
            phase, delta, sleep_vol);
        divoom_lvgl_top_bright_volume_menu_pulse_volume(sleep_vol);
        return;
    }

    if (divoom_astro_osd_clip_live_vol_allowed())
    {
        if (!g_astro_osd_ctrl.clip_vol_valid)
        {
            g_astro_osd_ctrl.clip_vol = divoom_music_ctrl_get_volume();
            g_astro_osd_ctrl.clip_vol_valid = 1;
        }
        new_vol = divoom_com_vol_step_ui(g_astro_osd_ctrl.clip_vol, delta);
        g_astro_osd_ctrl.clip_vol = new_vol;
        if (divoom_local_music_ctrl_get_tone_status())
            divoom_local_music_ctrl_set_tone_volume((int)new_vol);
        divoom_lvgl_top_bright_volume_menu_pulse_volume((int)new_vol);
        return;
    }
    g_astro_osd_ctrl.clip_vol_valid = 0;

    if (divoom_music_ctrl_is_phone_call())
    {
        uint8 cur_level;
        uint8 next_level;

        cur_level = divoom_music_ctrl_get_hfp_call_level();
        next_level = cur_level;
        if (delta > 0)
        {
            if (next_level < DIVOOM_BTCORE_HFP_CALL_VOL_MAX)
                next_level++;
        }
        else if (delta < 0)
        {
            if (next_level > 0)
                next_level--;
        }
        if (next_level != cur_level)
            divoom_music_ctrl_set_hfp_call_volume(next_level);
        divoom_lvgl_top_bright_volume_menu_pulse_volume(
            (int)divoom_music_ctrl_get_hfp_call_volume_ui());
        return;
    }

    vol = divoom_music_ctrl_get_volume();
    new_vol = divoom_com_vol_step_ui(vol, delta);
    if (delta > 0
        && divoom_lvgl_top_bright_volume_menu_hint_low_if_needed(vol, new_vol))
    {
        divoom_lvgl_top_bright_volume_menu_pulse_volume((int)vol);
        return;
    }
    if (new_vol != vol)
    {
        divoom_music_ctrl_set_volume(new_vol);
    }
    divoom_lvgl_top_bright_volume_menu_pulse_volume((int)divoom_music_ctrl_get_volume());
}

static void divoom_astro_osd_step_brightness_normal(int8 delta)
{
    int old_percent;
    int percent;

    old_percent = divoom_main_get_sys_light();
    percent = old_percent;
    if (delta > 0)
    {
        percent += DIVOOM_ASTRO_BRIGHTNESS_STEP_NORMAL;
        if (percent > 100)
        {
            percent = 100;
        }
    }
    else
    {
        percent -= DIVOOM_ASTRO_BRIGHTNESS_STEP_NORMAL;
        if (percent < 0)
        {
            percent = 0;
        }
    }

    if (percent != old_percent)
    {
        divoom_astro_osd_apply_brightness_percent(percent);
    }
    divoom_lvgl_top_bright_volume_menu_pulse_brightness_locked(percent);
}

static void divoom_astro_osd_step_brightness_sleep(int8 delta)
{
    int slot;
    int next_slot;
    int percent;

    slot = divoom_astro_osd_brightness_to_slot(divoom_main_get_sleep_brightness());
    next_slot = slot + delta;
    if (next_slot < 0)
    {
        next_slot = 0;
    }
    else if (next_slot > DIVOOM_ASTRO_BRIGHTNESS_SLOT_MAX)
    {
        next_slot = DIVOOM_ASTRO_BRIGHTNESS_SLOT_MAX;
    }

    percent = (int)s_astro_brightness_steps[next_slot];
    if (next_slot != slot)
    {
        divoom_astro_osd_apply_brightness_percent(percent);
    }
    divoom_lvgl_top_bright_volume_menu_pulse_brightness_locked(percent);
}

static void divoom_astro_osd_step_brightness(int8 delta)
{
    if (divoom_astro_osd_is_sleep_video_page())
    {
        divoom_astro_osd_step_brightness_sleep(delta);
    }
    else
    {
        divoom_astro_osd_step_brightness_normal(delta);
    }
}

static void divoom_astro_osd_do_brightness_step(int8 delta)
{
    divoom_get_lvgl_lock();
    divoom_astro_osd_step_brightness(delta);
    divoom_free_lvgl_lock();
}

#if 0
static void divoom_astro_osd_hold_repeat_cb(lv_timer_t *timer)
{
    (void)timer;

    if (!s_astro_osd_hold.active || !s_astro_osd_hold.brightness_mode)
    {
        return;
    }

    divoom_astro_osd_step_brightness(s_astro_osd_hold.delta);
}

static void divoom_astro_osd_hold_long_cb(lv_timer_t *timer)
{
    (void)timer;

    s_astro_osd_hold.long_timer = NULL;
    if (!s_astro_osd_hold.active)
    {
        return;
    }

    s_astro_osd_hold.brightness_mode = 1;
    divoom_astro_osd_step_brightness(s_astro_osd_hold.delta);

    if (s_astro_osd_hold.repeat_timer == NULL)
    {
        s_astro_osd_hold.repeat_timer = lv_timer_create(divoom_astro_osd_hold_repeat_cb,
            DIVOOM_ASTRO_BRIGHT_REPEAT_MS, NULL);
    }
}
#endif

static void divoom_lvgl_top_bright_volume_menu_create_ui(Pdivoom_lvgl_top_bright_volume_menu_ctrl phandle,
    const char *icon_file)
{
    uint8 sleep_mode;

    phandle->styles_initialized = 0;
    sleep_mode = divoom_astro_osd_is_sleep_video_page();

    lv_style_init(&phandle->bar_style);
    lv_style_set_bg_color(&phandle->bar_style,
        lv_color_hex(sleep_mode ? 0x161616 : 0xB5B5B5));
    lv_style_set_bg_opa(&phandle->bar_style,
        sleep_mode ? (lv_opa_t)(LV_OPA_COVER * 70 / 100) : LV_OPA_COVER);
    lv_style_set_border_width(&phandle->bar_style, DIVOOM_ASTRO_OSD_BORDER);
    lv_style_set_border_color(&phandle->bar_style, lv_color_hex(0x333333));
    lv_style_set_radius(&phandle->bar_style, DIVOOM_ASTRO_OSD_RADIUS);
    lv_style_set_pad_all(&phandle->bar_style, 0);
    lv_style_set_clip_corner(&phandle->bar_style, true);
    lv_style_set_shadow_width(&phandle->bar_style, 0);
    lv_style_set_outline_width(&phandle->bar_style, 0);

    lv_style_init(&phandle->fill_style);
    lv_style_set_bg_color(&phandle->fill_style,
        lv_color_hex(sleep_mode ? 0x666666 : 0xF3F3F3));
    lv_style_set_bg_opa(&phandle->fill_style, LV_OPA_COVER);
    lv_style_set_border_width(&phandle->fill_style, 0);
    /* 右侧实时进度边保持方角；左圆角由 bar 的裁剪区域形成。 */
    lv_style_set_radius(&phandle->fill_style, 0);
    lv_style_set_pad_all(&phandle->fill_style, 0);

    lv_style_init(&phandle->cont_style);
    lv_style_set_bg_opa(&phandle->cont_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&phandle->cont_style, 0);
    lv_style_set_pad_all(&phandle->cont_style, 0);
    phandle->styles_initialized = 1;

    /* lv_layer_top 对应 React portal，main_obj 直接对应 316x40 的 OSD 根节点。 */
    phandle->main_obj = lv_obj_create(lv_layer_top());
    lv_obj_add_style(phandle->main_obj, &phandle->bar_style, 0);
    lv_obj_set_pos(phandle->main_obj, DIVOOM_ASTRO_OSD_X, DIVOOM_ASTRO_OSD_Y);
    lv_obj_set_size(phandle->main_obj, DIVOOM_ASTRO_OSD_W, DIVOOM_ASTRO_OSD_H);
    lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_CLICKABLE);

    phandle->fill_obj = lv_obj_create(phandle->main_obj);
    lv_obj_add_style(phandle->fill_obj, &phandle->fill_style, 0);
    lv_obj_set_pos(phandle->fill_obj, 0, 0);
    lv_obj_set_height(phandle->fill_obj, DIVOOM_ASTRO_OSD_INNER_H);
    lv_obj_clear_flag(phandle->fill_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(phandle->fill_obj, LV_OBJ_FLAG_CLICKABLE);

    phandle->icon_obj = lv_img_create(phandle->main_obj);
    divoom_menu_control_lv_img_set_src(phandle->icon_obj, icon_file);
    lv_obj_set_size(phandle->icon_obj, DIVOOM_ASTRO_OSD_ICON, DIVOOM_ASTRO_OSD_ICON);
    lv_obj_set_pos(phandle->icon_obj, DIVOOM_ASTRO_OSD_ICON_X,
        (DIVOOM_ASTRO_OSD_INNER_H - DIVOOM_ASTRO_OSD_ICON) / 2);

    if (phandle->menu_exit_timer == NULL)
    {
        phandle->menu_exit_timer = lv_timer_create(divoom_lvgl_top_bright_volume_menu_time_handle, 200, NULL);
    }
}

void divoom_lvgl_top_bright_volume_menu_init(int bright_volume_flag)
{
    int fill_percent;

    divoom_get_lvgl_lock();

    if (bright_volume_flag == 1)
    {
        fill_percent = divoom_astro_osd_is_sleep_video_page()
            ? (int)divoom_main_get_sleep_brightness()
            : divoom_main_get_sys_light();
        divoom_lvgl_top_bright_volume_menu_pulse_brightness_locked(fill_percent);
    }
    else
    {
        if (divoom_astro_osd_is_sleep_volume())
        {
            divoom_sleep_aid_info *sleep_info = divoom_sleep_aid_ctrl_get_settings();
            int phase = divoom_sleep_aid_ctrl_get_play_time_phase();

            fill_percent = (sleep_info && phase == DIVOOM_SLEEP_AID_PLAY_TIME_PHASE_AFTER)
                ? sleep_info->after_vol : (sleep_info ? sleep_info->before_vol : 0);
        }
        else if (divoom_astro_osd_clip_live_vol_allowed() && g_astro_osd_ctrl.clip_vol_valid)
        {
            fill_percent = (int)g_astro_osd_ctrl.clip_vol;
        }
        else if (divoom_music_ctrl_is_phone_call())
        {
            fill_percent = (int)divoom_music_ctrl_get_hfp_call_volume_ui();
        }
        else
        {
            fill_percent = (int)divoom_music_ctrl_get_volume();
        }
        divoom_lvgl_top_bright_volume_menu_pulse_volume_locked(fill_percent);
    }

    divoom_free_lvgl_lock();
}

void divoom_lvgl_top_bright_volume_menu_exit(void)
{
    divoom_get_lvgl_lock();
#if 0
    divoom_astro_osd_hold_clear();
#endif
    divoom_lvgl_top_bright_volume_video_exit();
    divoom_lvgl_top_bright_volume_menu_del_menu();
    divoom_free_lvgl_lock();
}

int divoom_lvgl_top_bright_volume_menu_handle_key(uint8 key_value, uint8 key_type)
{
#if 0
    int8 delta;

    if (key_value != DIVOOM_KEY_LIGHT_LEFT && key_value != DIVOOM_KEY_LIGHT_RIGHT)
    {
        return 0;
    }

    delta = (key_value == DIVOOM_KEY_LIGHT_RIGHT) ? 1 : -1;

    if (key_type == DIVOOM_PRESSING)
    {
        if (!s_astro_osd_hold.active)
        {
            s_astro_osd_hold.active = 1;
            s_astro_osd_hold.key = key_value;
            s_astro_osd_hold.delta = delta;
            s_astro_osd_hold.brightness_mode = 0;
            s_astro_osd_hold.long_timer = lv_timer_create(divoom_astro_osd_hold_long_cb,
                DIVOOM_ASTRO_VOL_LONG_MS, NULL);
        }
        return 1;
    }

    if (key_type == DIVOOM_PRESS_SHORT)
    {
        if (s_astro_osd_hold.active && s_astro_osd_hold.key == key_value && !s_astro_osd_hold.brightness_mode)
        {
            divoom_astro_osd_step_volume(delta);
        }
        if (s_astro_osd_hold.brightness_mode)
        {
            divoom_main_apply_sys_light();
        }
        divoom_astro_osd_hold_clear();
        return 1;
    }

    if (key_type == DIVOOM_PRESS_UP)
    {
        if (s_astro_osd_hold.brightness_mode)
        {
            divoom_main_apply_sys_light();
        }
        divoom_astro_osd_hold_clear();
        return 1;
    }

    if (key_type == DIVOOM_PRESS_LONG || key_type == DIVOOM_PRESSING_LONG
        || key_type == DIVOOM_PRESS_LONG_LONG)
    {
        return 1;
    }

    return 0;
#else
    int8 delta;

    if (key_value != DIVOOM_KEY_LIGHT_LEFT && key_value != DIVOOM_KEY_LIGHT_RIGHT)
    {
        return 0;
    }

    delta = (key_value == DIVOOM_KEY_LIGHT_RIGHT) ? 1 : -1;

    if (key_type == DIVOOM_PRESS_SHORT)
    {
        divoom_astro_osd_step_volume(delta);
        return 1;
    }

    if (key_type == DIVOOM_PRESS_LONG)
    {
        s_astro_bright_last_step_ms = divoom_system_get_tickms();
        divoom_astro_osd_do_brightness_step(delta);
        return 1;
    }

    if (key_type == DIVOOM_PRESSING)
    {
        uint32_t now = divoom_system_get_tickms();

        if (now - s_astro_bright_last_step_ms < DIVOOM_ASTRO_BRIGHT_REPEAT_MS)
        {
            return 1;
        }
        s_astro_bright_last_step_ms = now;
        divoom_astro_osd_do_brightness_step(delta);
        return 1;
    }

    return 0;
#endif
}

#else

typedef struct _divoom_lvgl_top_bright_volume_menu_ctrl
{
	void *main_obj;

	int  select_id;
	void *bright_volume_slider;	//亮度音量对象	

	int bright;
	Pdivoom_pic_phone_manage_slide_config pslide_config;

	int exit_time;
	lv_timer_t *menu_exit_timer;
}divoom_lvgl_top_bright_volume_menu_ctrl, *Pdivoom_lvgl_top_bright_volume_menu_ctrl;


Pdivoom_lvgl_top_bright_volume_menu_ctrl pdivoom_lvgl_top_bright_volume_menu_handle = NULL;

void divoom_lvgl_top_bright_volume_menu_del_menu(void)
{
	if(pdivoom_lvgl_top_bright_volume_menu_handle->menu_exit_timer)
	{
		lv_timer_del(pdivoom_lvgl_top_bright_volume_menu_handle->menu_exit_timer);
	}
    divoom_lvgl_top_control_remove_top(pdivoom_lvgl_top_bright_volume_menu_handle);
	lv_obj_del_async(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj);
	divoom_system_free(pdivoom_lvgl_top_bright_volume_menu_handle);
	pdivoom_lvgl_top_bright_volume_menu_handle = NULL;
}


void divoom_lvgl_top_bright_volume_menu_time_handle(lv_timer_t *exit_timer)
{
	if(pdivoom_lvgl_top_bright_volume_menu_handle != NULL)
	{
		pdivoom_lvgl_top_bright_volume_menu_handle->exit_time++;

		if(pdivoom_lvgl_top_bright_volume_menu_handle->exit_time >= DIVOOM_LVGL_TOP_BRIGHT_VOLUME_MENU_EXIT_TIME)//主菜单不变动自动退出
		{
			int screen_bright = divoom_main_get_sys_light();
			divoom_system_setting_birghtness_conf brightness;
			brightness.light = screen_bright;
		
			if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 1)
			{
				divoom_net_app_send_packet(DIVOOM_NET_COMM_SET_SYS_BRIGHTNESS, DIVOOM_SERVER_TOPIC, (void *)&brightness);
			}
			else if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 2)
			{
				divoom_pic_phone_manage_save_slide_config(1);
			}
            else
            {
                divoom_main_send_sys_settings_config();
                divoom_main_save_config();
            }

			divoom_lvgl_top_bright_volume_menu_del_menu();
		}
	}
}


//主菜单回调
static void divoom_lvgl_top_bright_volume_menu_main_cout_event_cb(lv_event_t * e)
{
	char image_str[48];
	lv_event_code_t code = lv_event_get_code(e);
	
	if(code == LV_EVENT_KEY)
    {
		uint32 para = *((uint32 *)lv_event_get_param(e));
        char c = (para&0xff);

		if(pdivoom_lvgl_top_bright_volume_menu_handle == NULL)
		{
			divoom_print("test\n");
			return;
		}

		pdivoom_lvgl_top_bright_volume_menu_handle->exit_time = 0;
		
		if(c == DIVOOM_KEY_LIGHT_LEFT)
		{
			int value = lv_slider_get_value(pdivoom_lvgl_top_bright_volume_menu_handle->bright_volume_slider);
            
			if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 1)
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
			
			lv_slider_set_value(pdivoom_lvgl_top_bright_volume_menu_handle->bright_volume_slider, value, LV_ANIM_ON);
			//lv_obj_invalidate(pdivoom_lvgl_top_bright_volume_menu_handle->bright_volume_slider);
			if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 1)
			{
				divoom_main_set_sys_light(value);
			}
			else if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 2)
			{
                if(pdivoom_lvgl_top_bright_volume_menu_handle->pslide_config->video_volume != value)
                {
				    pdivoom_lvgl_top_bright_volume_menu_handle->pslide_config->video_volume = value;
                    divoom_local_movie_play_set_movie_volume(divoom_pic_phone_manage_convert_video_volume(value));
                }
			}
            else
            {
                if(value != divoom_main_get_sys_volume())
                {
                    divoom_main_set_tone_volume(value);
                    divoom_local_music_ctrl_set_tone_volume(value/2);
                }
            }
		}
		else if(c == DIVOOM_KEY_LIGHT_RIGHT)
		{
			int value = lv_slider_get_value(pdivoom_lvgl_top_bright_volume_menu_handle->bright_volume_slider);
			if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 1)
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
			
			lv_slider_set_value(pdivoom_lvgl_top_bright_volume_menu_handle->bright_volume_slider, value, LV_ANIM_ON);
			//lv_obj_invalidate(pdivoom_lvgl_top_bright_volume_menu_handle->bright_volume_slider);
			if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 1)
			{
				divoom_main_set_sys_light(value);
			}
			else if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 2)
			{
                if(pdivoom_lvgl_top_bright_volume_menu_handle->pslide_config->video_volume != value)
                {
				    pdivoom_lvgl_top_bright_volume_menu_handle->pslide_config->video_volume = value;
                    divoom_local_movie_play_set_movie_volume(divoom_pic_phone_manage_convert_video_volume(value));
                }
			}
            else
            {
                if(value != divoom_main_get_sys_volume())
                {
                    divoom_local_music_ctrl_set_tone_volume(value/2);
                    divoom_main_set_tone_volume(value);
                }
            }
		}
		else
		{
			int screen_bright = divoom_main_get_sys_light();
			divoom_system_setting_birghtness_conf brightness;
			brightness.light = screen_bright;

			if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 1)
			{
				divoom_net_app_send_packet(DIVOOM_NET_COMM_SET_SYS_BRIGHTNESS, DIVOOM_SERVER_TOPIC, (void *)&brightness);
			}
			else if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 2)
			{
				divoom_pic_phone_manage_save_slide_config(1);
			}
            else
            {
                divoom_main_send_sys_settings_config();
                divoom_main_save_config();
            }

			divoom_lvgl_top_bright_volume_menu_del_menu();
		}
		
	}
}


void divoom_lvgl_top_bright_volume_menu_create_icon(void)
{
	lv_obj_t* bg_cover = lv_img_create(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj);
	divoom_menu_control_lv_img_set_src(bg_cover, "26/26_top_bg.png");
	lv_obj_set_style_img_opa(bg_cover, LV_OPA_80, 0);
	lv_obj_set_pos(bg_cover, 22-15, 19-10);

	lv_obj_t* cont = lv_obj_create(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj);
	lv_obj_set_size(cont, 76, 242);
	lv_obj_set_pos(cont, 22, 19);
	lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_clip_corner(cont, true, 0);
	lv_obj_set_style_pad_all(cont, 0, 0);
	lv_obj_set_style_border_width(cont, 0, 0);
	lv_obj_set_style_radius(cont, 24, 0);
	lv_obj_set_style_shadow_width(cont, 0, 0);
	//lv_obj_set_style_shadow_ofs_x(cont, 0, 0);
	//lv_obj_set_style_shadow_ofs_y(cont, 4, 0);
	//lv_obj_set_style_shadow_color(cont, lv_color_hex(0x0), 0);
	//lv_obj_set_style_shadow_opa(cont, 35*LV_OPA_100/100, 0);
	lv_obj_set_style_bg_opa(cont, 0, 0);

 	lv_obj_t* slider = lv_slider_create(cont);
 	lv_obj_set_size(slider, 76, 242);
 	lv_obj_center(slider);
 	lv_obj_set_style_pad_all(slider, 0, 0);
 	lv_slider_set_range(slider, 0, 100);

	if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 1)
	{
		pdivoom_lvgl_top_bright_volume_menu_handle->bright = divoom_main_get_sys_light();
 		lv_slider_set_value(slider, pdivoom_lvgl_top_bright_volume_menu_handle->bright, LV_ANIM_OFF);
	}
	else if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 2)
	{
		pdivoom_lvgl_top_bright_volume_menu_handle->pslide_config = divoom_pic_phone_manage_get_slide_config();
		lv_slider_set_value(slider, pdivoom_lvgl_top_bright_volume_menu_handle->pslide_config->video_volume, LV_ANIM_OFF);
	}
    else 
    {
        divoom_print("init vol: %d\n", divoom_main_get_sys_volume());
		lv_slider_set_value(slider, divoom_main_get_sys_volume(), LV_ANIM_OFF);
    }

	lv_obj_set_style_bg_color(slider, lv_color_hex(0x888888), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(slider, LV_OPA_80, LV_PART_MAIN);
 	lv_obj_set_style_radius(slider, 24, LV_PART_MAIN);
	
	lv_obj_set_style_bg_color(slider, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR);
	lv_obj_set_style_radius(slider, 0, LV_PART_INDICATOR);

	lv_obj_set_style_bg_opa(slider, LV_OPA_0, LV_PART_KNOB);
	pdivoom_lvgl_top_bright_volume_menu_handle->bright_volume_slider = slider;

	lv_obj_t* icon = lv_img_create(cont);
	if(pdivoom_lvgl_top_bright_volume_menu_handle->select_id == 1)
	{
		divoom_menu_control_lv_img_set_src(icon, "26/26_top_bright.png");
	}
	else
	{
		divoom_menu_control_lv_img_set_src(icon, "26/26_top_volume.png");
	}
	lv_obj_set_pos(icon, 18, 184);
	
	
}


void divoom_lvgl_top_bright_volume_menu_init(int bright_volume_flag)//1:亮度 2:视频音量3：提示音音量
{
	divoom_get_lvgl_lock();

	if(pdivoom_lvgl_top_bright_volume_menu_handle != NULL)
	{
		divoom_free_lvgl_lock();
		return;
	}
	
	pdivoom_lvgl_top_bright_volume_menu_handle = (Pdivoom_lvgl_top_bright_volume_menu_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_top_bright_volume_menu_ctrl));
	
	if(pdivoom_lvgl_top_bright_volume_menu_handle == NULL)
	{
		divoom_free_lvgl_lock();
		return;
	}
	memset(pdivoom_lvgl_top_bright_volume_menu_handle, 0, sizeof(divoom_lvgl_top_bright_volume_menu_ctrl));

	pdivoom_lvgl_top_bright_volume_menu_handle->select_id = bright_volume_flag;	
	pdivoom_lvgl_top_bright_volume_menu_handle->exit_time = 0;
	
	pdivoom_lvgl_top_bright_volume_menu_handle->main_obj = lv_btn_create(lv_layer_top()); //在顶层创建主页面

	lv_obj_remove_style_all(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj);
	lv_obj_set_style_bg_opa(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj, 0, 0);
	//lv_obj_set_style_bg_color(pdivoom_lvgl_top_ambient_menu_handle->main_obj, lv_color_hex(0x000000), 0);
	lv_obj_set_style_pad_all(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_border_width(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_shadow_width(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_outline_width(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj, 0, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_clear_flag(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj, 120, 280);
	lv_obj_set_pos(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj, 662, 349);
	
	lv_obj_add_event_cb(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj, divoom_lvgl_top_bright_volume_menu_main_cout_event_cb, LV_EVENT_KEY, NULL);
	
	divoom_lvgl_top_bright_volume_menu_create_icon();

	pdivoom_lvgl_top_bright_volume_menu_handle->menu_exit_timer = lv_timer_create(divoom_lvgl_top_bright_volume_menu_time_handle, 1000, NULL);

    divoom_lvgl_top_control_add_top(pdivoom_lvgl_top_bright_volume_menu_handle, pdivoom_lvgl_top_bright_volume_menu_handle->main_obj);

	divoom_free_lvgl_lock();
}

void divoom_lvgl_top_bright_volume_menu_exit(void)
{
	if(pdivoom_lvgl_top_bright_volume_menu_handle != NULL)
	{
		if(pdivoom_lvgl_top_bright_volume_menu_handle->menu_exit_timer)
		{
			lv_timer_del(pdivoom_lvgl_top_bright_volume_menu_handle->menu_exit_timer);
		}
        divoom_lvgl_top_control_remove_top(pdivoom_lvgl_top_bright_volume_menu_handle);
		lv_obj_del_async(pdivoom_lvgl_top_bright_volume_menu_handle->main_obj);
		divoom_system_free(pdivoom_lvgl_top_bright_volume_menu_handle);
		pdivoom_lvgl_top_bright_volume_menu_handle = NULL;
	}
}
#endif

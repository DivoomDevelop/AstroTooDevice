#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_top_main_menu.h"
#include "divoom_music_ctrl.h"

#if 1//def DIVOOM_TIME_FRAME_SPEAKER

typedef struct _divoom_lvgl_top_audio_menu_ctrl
{
	void *main_obj;		
	void *cout_obj;

	void *select_img[DIVOOM_TOP_AUDIO_MENU_SLOT_MAX];
	void *disp_img[DIVOOM_TOP_AUDIO_MENU_SLOT_MAX];

	int select_id;
	int cur_support_mode[DIVOOM_TOP_AUDIO_MENU_SLOT_MAX];

	int exit_time;
	lv_timer_t *menu_exit_timer;
}divoom_lvgl_top_audio_menu_ctrl, *Pdivoom_lvgl_top_audio_menu_ctrl;

Pdivoom_lvgl_top_audio_menu_ctrl pdivoom_lvgl_top_audio_menu_handle = NULL;	
	
#if DIVOOM_MUSIC_CTRL_LINEIN_ENABLE && DIVOOM_MUSIC_CTRL_UAC_ENABLE
const char *divoom_top_audio_img_list[] = {
	"97/97_top_audio_bluetooth.png",
	"97/97_top_audio_tf.png",
	"97/97_top_audio_aux.png",
	"97/97_top_audio_uac.png",
};
#elif DIVOOM_MUSIC_CTRL_LINEIN_ENABLE
const char *divoom_top_audio_img_list[] = {
	"97/97_top_audio_bluetooth.png",
	"97/97_top_audio_tf.png",
	"97/97_top_audio_aux.png",
};
#elif DIVOOM_MUSIC_CTRL_UAC_ENABLE
const char *divoom_top_audio_img_list[] = {
	"97/97_top_audio_bluetooth.png",
	"97/97_top_audio_tf.png",
	"97/97_top_audio_uac.png",
};
#else
const char *divoom_top_audio_img_list[] = {
	"97/97_top_audio_bluetooth.png",
	"97/97_top_audio_tf.png",
};
#endif

static uint8 divoom_lvgl_top_audio_menu_slot_mode(int slot)
{
	uint8 audio_table[] =
	{
		DIVOOM_MUSIC_CTRL_MODE_BT,
		DIVOOM_MUSIC_CTRL_MODE_LOCAL,
#if DIVOOM_MUSIC_CTRL_LINEIN_ENABLE
		DIVOOM_MUSIC_CTRL_MODE_LINEIN,
#endif
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
		DIVOOM_MUSIC_CTRL_MODE_UAC,
#endif
	};

	if (slot < 0 || slot >= (int)(sizeof(audio_table) / sizeof(audio_table[0])))
		return DIVOOM_MUSIC_CTRL_MODE_BT;
	return audio_table[slot];
}

uint8 divoom_lvgl_top_audio_menu_get_music_mode(void)
{
	int i, cur_mode = 0;

	uint8 audio_table[] =
	{
		DIVOOM_MUSIC_CTRL_MODE_BT,
		DIVOOM_MUSIC_CTRL_MODE_LOCAL,
#if DIVOOM_MUSIC_CTRL_LINEIN_ENABLE
		DIVOOM_MUSIC_CTRL_MODE_LINEIN,
#endif
#if DIVOOM_MUSIC_CTRL_UAC_ENABLE
		DIVOOM_MUSIC_CTRL_MODE_UAC,
#endif
	};
	const int audio_table_len = sizeof(audio_table) / sizeof(audio_table[0]);
	cur_mode = divoom_music_ctrl_get_mode();
	for (i = 0; i < audio_table_len; i++)
	{
		if (audio_table[i] == cur_mode)
		{
			cur_mode = i;
			break;
		}	
	}
	return cur_mode;
}

void divoom_lvgl_top_audio_menu_create_support_mode_menu(void)
{
	int i;

	for (i = 0; i < DIVOOM_TOP_AUDIO_MENU_SLOT_MAX; i++)
	{
		uint8 mode = divoom_lvgl_top_audio_menu_slot_mode(i);
		int support = divoom_music_ctrl_is_mode_supported(mode);

		pdivoom_lvgl_top_audio_menu_handle->cur_support_mode[i] = support;
		if (support)
			lv_obj_set_style_img_recolor(pdivoom_lvgl_top_audio_menu_handle->disp_img[i], lv_color_hex(0x0), 0);
		else
			lv_obj_set_style_img_recolor(pdivoom_lvgl_top_audio_menu_handle->disp_img[i], lv_color_hex(0xCECECE), 0);
	}
}


void divoom_lvgl_top_audio_menu_update_support_mode_menu(int index)
{
	int support = pdivoom_lvgl_top_audio_menu_handle->cur_support_mode[index];

	if(support)
	{
		lv_obj_set_style_img_recolor(pdivoom_lvgl_top_audio_menu_handle->disp_img[index], lv_color_hex(0x0), 0);	
	}
	else
	{
		lv_obj_set_style_img_recolor(pdivoom_lvgl_top_audio_menu_handle->disp_img[index], lv_color_hex(0xCECECE), 0);		
	}
}


void divoom_lvgl_top_audio_menu_if_update_mode_menu(void)
{
	int i;

	for (i = 0; i < DIVOOM_TOP_AUDIO_MENU_SLOT_MAX; i++)
	{
		uint8 mode = divoom_lvgl_top_audio_menu_slot_mode(i);
		int support = divoom_music_ctrl_is_mode_supported(mode);

		if (support != pdivoom_lvgl_top_audio_menu_handle->cur_support_mode[i])
		{
			pdivoom_lvgl_top_audio_menu_handle->cur_support_mode[i] = support;
			divoom_lvgl_top_audio_menu_update_support_mode_menu(i);
		}
	}
}

uint8 divoom_lvgl_top_audio_menu_is_open(void)
{
	return (pdivoom_lvgl_top_audio_menu_handle != NULL) ? 1 : 0;
}

void divoom_lvgl_top_audio_menu_refresh_select(void)
{
	int i;

	if (pdivoom_lvgl_top_audio_menu_handle == NULL)
		return;

	divoom_get_lvgl_lock();
	pdivoom_lvgl_top_audio_menu_handle->select_id =
		divoom_lvgl_top_audio_menu_get_music_mode();
	for (i = 0; i < DIVOOM_TOP_AUDIO_MENU_SLOT_MAX; i++)
	{
		if (pdivoom_lvgl_top_audio_menu_handle->select_id == i)
			lv_obj_clear_flag(pdivoom_lvgl_top_audio_menu_handle->select_img[i],
					  LV_OBJ_FLAG_HIDDEN);
		else
			lv_obj_add_flag(pdivoom_lvgl_top_audio_menu_handle->select_img[i],
					 LV_OBJ_FLAG_HIDDEN);
	}
	divoom_lvgl_top_audio_menu_if_update_mode_menu();
	divoom_free_lvgl_lock();
}

void divoom_lvgl_top_audio_menu_del_menu(void)
{
	if(pdivoom_lvgl_top_audio_menu_handle != NULL)
	{
		if(pdivoom_lvgl_top_audio_menu_handle->menu_exit_timer)
		{
			lv_timer_del(pdivoom_lvgl_top_audio_menu_handle->menu_exit_timer);
		}
    	divoom_lvgl_top_control_remove_top(pdivoom_lvgl_top_audio_menu_handle);
		lv_obj_del_async(pdivoom_lvgl_top_audio_menu_handle->main_obj);
		divoom_system_free(pdivoom_lvgl_top_audio_menu_handle);
        divoom_local_movie_play_ctrl(1);
		pdivoom_lvgl_top_audio_menu_handle = NULL;
	}
}

void divoom_lvgl_top_audio_menu_time_handle(lv_timer_t *exit_timer)
{
	if(pdivoom_lvgl_top_audio_menu_handle != NULL)
	{
		pdivoom_lvgl_top_audio_menu_handle->exit_time++;

		if(pdivoom_lvgl_top_audio_menu_handle->exit_time >= DIVOOM_EXIT_MENU_TIME_OUT)//主菜单不变动自动退出
		{
			divoom_lvgl_top_audio_menu_del_menu();
		}
		else if(pdivoom_lvgl_top_audio_menu_handle->exit_time > 1)
		{
			divoom_lvgl_top_audio_menu_if_update_mode_menu();
		}
	}
}

static void divoom_lvgl_top_audio_menu_event_cb(lv_event_t * e)
{
	int i;
    lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_KEY)
    {
        char *key_info = (char *)lv_event_get_param(e);
        char c = key_info[0];

        if(pdivoom_lvgl_top_audio_menu_handle == NULL)
        {
            divoom_print("test!\n");
			return;
        }

		pdivoom_lvgl_top_audio_menu_handle->exit_time = 0;

		if(c == DIVOOM_KEY_MUSIC_MODE)
		{
			divoom_music_ctrl_set_next_mode();
			pdivoom_lvgl_top_audio_menu_handle->select_id = divoom_lvgl_top_audio_menu_get_music_mode();
			for(i = 0; i < DIVOOM_TOP_AUDIO_MENU_SLOT_MAX; i++)
			{
				if(pdivoom_lvgl_top_audio_menu_handle->select_id == i)
				{
					//lv_obj_set_style_img_recolor(pdivoom_lvgl_top_audio_menu_handle->disp_img[i], lv_color_hex(0x0), 0);
					lv_obj_clear_flag(pdivoom_lvgl_top_audio_menu_handle->select_img[i], LV_OBJ_FLAG_HIDDEN);
				}
				else
				{
					//lv_obj_set_style_img_recolor(pdivoom_lvgl_top_audio_menu_handle->disp_img[i], lv_color_hex(0x828282), 0);
					lv_obj_add_flag(pdivoom_lvgl_top_audio_menu_handle->select_img[i], LV_OBJ_FLAG_HIDDEN);
				}
			}
		}
		else
		{
			divoom_lvgl_top_audio_menu_del_menu();
			divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
		}
		/*else if(c == DIVOOM_LVGL_KEY_PRE) 
		{
			pdivoom_lvgl_top_audio_menu_handle->select_id ++;
   			if(pdivoom_lvgl_top_audio_menu_handle->select_id >= sizeof(divoom_top_audio_img_list)/sizeof(divoom_top_audio_img_list[0]))
    		{
        		pdivoom_lvgl_top_audio_menu_handle->select_id = 0;
    		}
						
			for(i = 0; i < DIVOOM_TOP_AUDIO_MENU_SLOT_MAX; i++)
			{
				if(pdivoom_lvgl_top_audio_menu_handle->select_id == i)
				{
					lv_obj_set_style_img_recolor(pdivoom_lvgl_top_audio_menu_handle->disp_img[i], lv_color_hex(0x0), 0);
					lv_obj_clear_flag(pdivoom_lvgl_top_audio_menu_handle->select_img[i], LV_OBJ_FLAG_HIDDEN);
				}
				else
				{
					lv_obj_set_style_img_recolor(pdivoom_lvgl_top_audio_menu_handle->disp_img[i], lv_color_hex(0x828282), 0);
					lv_obj_add_flag(pdivoom_lvgl_top_audio_menu_handle->select_img[i], LV_OBJ_FLAG_HIDDEN);
				}
			}
        }
		else if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			if(pdivoom_lvgl_top_audio_menu_handle->select_id == 0)
    		{
        		pdivoom_lvgl_top_audio_menu_handle->select_id = sizeof(divoom_top_audio_img_list)/sizeof(divoom_top_audio_img_list[0]) - 1;
    		}
    		else
    		{
        		pdivoom_lvgl_top_audio_menu_handle->select_id --;
    		}

			for(i = 0; i < DIVOOM_TOP_AUDIO_MENU_SLOT_MAX; i++)
			{
				if(pdivoom_lvgl_top_audio_menu_handle->select_id == i)
				{
					lv_obj_set_style_img_recolor(pdivoom_lvgl_top_audio_menu_handle->disp_img[i], lv_color_hex(0x0), 0);
					lv_obj_clear_flag(pdivoom_lvgl_top_audio_menu_handle->select_img[i], LV_OBJ_FLAG_HIDDEN);
				}
				else
				{
					lv_obj_set_style_img_recolor(pdivoom_lvgl_top_audio_menu_handle->disp_img[i], lv_color_hex(0x828282), 0);
					lv_obj_add_flag(pdivoom_lvgl_top_audio_menu_handle->select_img[i], LV_OBJ_FLAG_HIDDEN);
				}
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{	
			int select_id = pdivoom_lvgl_top_audio_menu_handle->select_id;
			divoom_lvgl_top_audio_menu_del_menu();
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_lvgl_top_audio_menu_del_menu();
		}*/
    }
	
}

void divoom_lvgl_top_audio_menu_create_icon(void)
{
	int i, x, y, zoom, angle, opa;
	
	pdivoom_lvgl_top_audio_menu_handle->cout_obj = lv_btn_create(pdivoom_lvgl_top_audio_menu_handle->main_obj);

	lv_obj_remove_style_all(pdivoom_lvgl_top_audio_menu_handle->cout_obj);
	lv_obj_set_style_pad_all(pdivoom_lvgl_top_audio_menu_handle->cout_obj, 0, 0);
	lv_obj_set_style_border_width(pdivoom_lvgl_top_audio_menu_handle->cout_obj, 0, 0);
	lv_obj_set_style_outline_width(pdivoom_lvgl_top_audio_menu_handle->cout_obj, 0, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_shadow_width(pdivoom_lvgl_top_audio_menu_handle->cout_obj, 0, 0);
	lv_obj_set_style_bg_opa(pdivoom_lvgl_top_audio_menu_handle->cout_obj, LV_OPA_0, 0);
	lv_obj_set_pos(pdivoom_lvgl_top_audio_menu_handle->cout_obj, 0, 0);
    lv_obj_set_size(pdivoom_lvgl_top_audio_menu_handle->cout_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_add_event_cb(pdivoom_lvgl_top_audio_menu_handle->cout_obj, divoom_lvgl_top_audio_menu_event_cb, LV_EVENT_KEY, NULL);

	lv_obj_t *bg_img = lv_img_create(pdivoom_lvgl_top_audio_menu_handle->cout_obj);
	divoom_menu_control_lv_img_set_src(bg_img, "97/97_top_audio.png");
	//lv_obj_set_style_img_opa(bg_img, 85*LV_OPA_100/100, 0);
	lv_obj_set_pos(bg_img, 120-15, 18-10);

	for(i = 0; i < DIVOOM_TOP_AUDIO_MENU_SLOT_MAX; i++)
	{
		pdivoom_lvgl_top_audio_menu_handle->select_img[i] = lv_img_create(pdivoom_lvgl_top_audio_menu_handle->cout_obj);
		divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_audio_menu_handle->select_img[i], "97/97_top_select.png");
		lv_obj_add_flag(pdivoom_lvgl_top_audio_menu_handle->select_img[i], LV_OBJ_FLAG_HIDDEN);

		lv_obj_set_size(pdivoom_lvgl_top_audio_menu_handle->select_img[i], 150, 110);
		lv_obj_set_pos(pdivoom_lvgl_top_audio_menu_handle->select_img[i], (130-15)+140*i, 28-13);
		
		//
		pdivoom_lvgl_top_audio_menu_handle->disp_img[i] = lv_img_create(pdivoom_lvgl_top_audio_menu_handle->cout_obj);
		divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_audio_menu_handle->disp_img[i], divoom_top_audio_img_list[i]);
		
		//lv_obj_set_style_img_recolor(pdivoom_lvgl_top_audio_menu_handle->disp_img[i], lv_color_hex(0x828282), 0);
		lv_obj_set_style_img_recolor_opa(pdivoom_lvgl_top_audio_menu_handle->disp_img[i], LV_OPA_100, 0);
		lv_obj_set_size(pdivoom_lvgl_top_audio_menu_handle->disp_img[i], 140, 100);
		lv_obj_set_pos(pdivoom_lvgl_top_audio_menu_handle->disp_img[i], 120+140*i, 18);

		if(pdivoom_lvgl_top_audio_menu_handle->select_id == i)
		{
			//lv_obj_set_style_img_recolor(pdivoom_lvgl_top_audio_menu_handle->disp_img[i], lv_color_hex(0x0), 0);
			lv_obj_clear_flag(pdivoom_lvgl_top_audio_menu_handle->select_img[i], LV_OBJ_FLAG_HIDDEN);
			
		}
	}
	
	divoom_lvgl_top_audio_menu_create_support_mode_menu();
}

void divoom_lvgl_top_audio_menu_init(void)
{
	divoom_get_lvgl_lock();

	if(pdivoom_lvgl_top_audio_menu_handle != NULL)
	{
		divoom_free_lvgl_lock();
		return;
	}

	pdivoom_lvgl_top_audio_menu_handle = (Pdivoom_lvgl_top_audio_menu_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_top_audio_menu_ctrl));

	if(pdivoom_lvgl_top_audio_menu_handle == NULL)
	{
		divoom_free_lvgl_lock();
		return;
	}
    divoom_local_movie_play_ctrl(0);
	memset(pdivoom_lvgl_top_audio_menu_handle, 0, sizeof(divoom_lvgl_top_audio_menu_ctrl));

	pdivoom_lvgl_top_audio_menu_handle->select_id = divoom_lvgl_top_audio_menu_get_music_mode();
	pdivoom_lvgl_top_audio_menu_handle->exit_time = 0;
	
	pdivoom_lvgl_top_audio_menu_handle->main_obj = lv_obj_create(lv_layer_top()); //在顶层创建主页面

	lv_obj_set_style_bg_opa(pdivoom_lvgl_top_audio_menu_handle->main_obj, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(pdivoom_lvgl_top_audio_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_border_width(pdivoom_lvgl_top_audio_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_shadow_width(pdivoom_lvgl_top_audio_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_outline_width(pdivoom_lvgl_top_audio_menu_handle->main_obj, 0, 0);
	lv_obj_clear_flag(pdivoom_lvgl_top_audio_menu_handle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(pdivoom_lvgl_top_audio_menu_handle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(pdivoom_lvgl_top_audio_menu_handle->main_obj, 0, 0);
	
	divoom_lvgl_top_audio_menu_create_icon();

	pdivoom_lvgl_top_audio_menu_handle->menu_exit_timer = lv_timer_create(divoom_lvgl_top_audio_menu_time_handle, 1000, NULL);
	
	divoom_lvgl_top_control_add_top(pdivoom_lvgl_top_audio_menu_handle, pdivoom_lvgl_top_audio_menu_handle->cout_obj);
	divoom_free_lvgl_lock();
}


void divoom_lvgl_top_audio_menu_exit(void)
{
	if(pdivoom_lvgl_top_audio_menu_handle != NULL)
	{
		if(pdivoom_lvgl_top_audio_menu_handle->menu_exit_timer)
		{
			lv_timer_del(pdivoom_lvgl_top_audio_menu_handle->menu_exit_timer);
		}
        divoom_lvgl_top_control_remove_top(pdivoom_lvgl_top_audio_menu_handle);
		lv_obj_del_async(pdivoom_lvgl_top_audio_menu_handle->main_obj);
		divoom_system_free(pdivoom_lvgl_top_audio_menu_handle);
        divoom_local_movie_play_ctrl(1);
		pdivoom_lvgl_top_audio_menu_handle = NULL;
	}
}
#endif

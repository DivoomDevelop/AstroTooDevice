#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_music_ctrl.h"
#include "divoom_uac.h"

typedef struct _divoom_lvgl_uac_music_menu_ctrl
{
    int menu_id;

	void *main_obj;
	void *cout_obj;

	void *image_obj;

	int play_status;

	lv_timer_t *time_handle;
}divoom_lvgl_uac_music_menu_ctrl, *Pdivoom_lvgl_uac_music_menu_ctrl;

void divoom_lvgl_uac_music_menu_time_loop(struct _lv_timer_t * ptime_info)
{
    Pdivoom_lvgl_uac_music_menu_ctrl phandle = (Pdivoom_lvgl_uac_music_menu_ctrl)lv_timer_get_user_data(ptime_info);
    int cur_play_status = divoom_main_get_audio_type_statu(0);
    
    if(cur_play_status != phandle->play_status)
    {
        phandle->play_status = cur_play_status;
        if(phandle->play_status)
        {
            divoom_menu_control_lv_img_set_src(phandle->image_obj, "95/95_UAC_connected.png");
        }
        else
        {
            divoom_menu_control_lv_img_set_src(phandle->image_obj, "95/95_UAC_disconnected.png");
        }
    }
}

void divoom_lvgl_uac_music_menu_event_cb(lv_event_t *e)
{
	int i;
	Pdivoom_lvgl_uac_music_menu_ctrl phandle = (Pdivoom_lvgl_uac_music_menu_ctrl)lv_event_get_user_data(e);
	lv_obj_t *cur_btn_obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
	
	if(code == LV_EVENT_KEY)
	{
		char c = *((char *)lv_event_get_param(e));

		if(c == DIVOOM_LVGL_KEY_PRE)
		{
			divoom_music_ctrl_play_pre();
		}
		else if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			divoom_music_ctrl_play_next();
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_menu_control_return_pre();
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			
		}		
		else
		{
			char *key_info = (char *)lv_event_get_param(e);
			divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
		}
	}
}

void *divoom_lvgl_uac_music_menu_init(void *user_data, int menu_id)
{
	int i;
    Pdivoom_lvgl_uac_music_menu_ctrl phandle;    
    void *slide_obj, *label_obj, *image_obj;
    
    divoom_music_ctrl_set_mode(DIVOOM_MUSIC_CTRL_MODE_UAC);

    phandle = divoom_system_malloc(sizeof(divoom_lvgl_uac_music_menu_ctrl));

    phandle->menu_id = menu_id;
	phandle->play_status = divoom_main_get_audio_type_statu(0);
    phandle->main_obj = lv_obj_create(NULL);

    lv_obj_set_pos(phandle->main_obj, 0, 0);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_pad_all(phandle->main_obj, 0, 0);
    lv_obj_set_scrollbar_mode(phandle->main_obj, LV_SCROLLBAR_MODE_OFF);
	
	phandle->cout_obj = lv_btn_create(phandle->main_obj);

    lv_obj_set_pos(phandle->cout_obj, 80, 128);
	lv_obj_set_size(phandle->cout_obj, 640, 1024);
    lv_obj_set_style_bg_color(phandle->cout_obj, lv_color_hex(0xF2F2F2), 0);
	lv_obj_set_style_radius(phandle->cout_obj, 36, 0);
	lv_obj_set_style_outline_width(phandle->cout_obj, 0, 0);
	lv_obj_set_style_outline_width(phandle->cout_obj, 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_set_scrollbar_mode(phandle->cout_obj, LV_SCROLLBAR_MODE_OFF);
	lv_obj_set_style_pad_all(phandle->cout_obj, 0, 0);
	lv_obj_add_event_cb(phandle->cout_obj, divoom_lvgl_uac_music_menu_event_cb, LV_EVENT_KEY, phandle);

	phandle->image_obj = lv_img_create(phandle->cout_obj);
	if(phandle->play_status)
    {
        divoom_menu_control_lv_img_set_src(phandle->image_obj, "95/95_UAC_connected.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(phandle->image_obj, "95/95_UAC_disconnected.png");
    }
	lv_obj_set_pos(phandle->image_obj, 130, 302);
 
    label_obj = lv_label_create(phandle->cout_obj);
    lv_obj_set_pos(label_obj, 115, 653);
	lv_obj_set_size(label_obj, 414, 56);
    lv_label_set_text(label_obj, "UAC(USB Audio)");
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0); 
	   
    phandle->time_handle = lv_timer_create(divoom_lvgl_uac_music_menu_time_loop, 1000, phandle);
    lv_timer_pause(phandle->time_handle);
    
    return phandle;
    
}

void divoom_lvgl_uac_music_menu_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_uac_music_menu_ctrl phandle = (Pdivoom_lvgl_uac_music_menu_ctrl)handle;

	if(active_obj)
	{
		lv_group_focus_obj(active_obj);
	}    
	else if(phandle->cout_obj)
	{
		lv_group_focus_obj(phandle->cout_obj);
	}

    lv_timer_resume(phandle->time_handle);
    lv_scr_load(phandle->main_obj);
    
    
}

void divoom_lvgl_uac_music_menu_pause(void *handle)
{
    Pdivoom_lvgl_uac_music_menu_ctrl phandle = (Pdivoom_lvgl_uac_music_menu_ctrl)handle;
    
    lv_timer_pause(phandle->time_handle);
}

void divoom_lvgl_uac_music_menu_exit(void *handle)
{
    Pdivoom_lvgl_uac_music_menu_ctrl phandle = (Pdivoom_lvgl_uac_music_menu_ctrl)handle;
    
    lv_timer_del(phandle->time_handle);
    lv_obj_del(phandle->main_obj);
    
    divoom_system_free(phandle);
}


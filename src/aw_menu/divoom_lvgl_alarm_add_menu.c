#include "divoom_lvgl_stream_cont.h"
#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#ifdef DIVOOM_TIME_FRAME_SPEAKER
#include "divoom_alarm_tf_sound_pick.h"
#endif
#include <string.h>

typedef struct _divoom_alarm_add_menu_time_obj_item
{
	void *cont;
	void *time_label;
	int font_size;
	int obj_height;
	int y;
}divoom_alarm_add_menu_time_obj_item, *Pdivoom_alarm_add_menu_time_obj_item;

typedef struct _divoom_lvgl_alarm_add_menu_ctrl
{
    int menu_id;
	lv_style_t backgroud_type;
	void *main_obj;
	void *bg_obj;
	void *roller;
	int am_pm_index;  //0:am 1:pm
	divoom_alarm_add_menu_time_obj_item am_obj;
	divoom_alarm_add_menu_time_obj_item pm_obj;

	void *stream_cont_hour;
	void *stream_cont_minute;

	lv_style_t obj_style;
	lv_style_t btn_normal_type;
	lv_style_t btn_press_type;

	void *time_obj;
	int active_id;

	void *repeat_btn;
	void *cys_label_obj;		//重复标签对象
	void *week_list_obj;			
	void *weekday_btn_obj[7];	
	void *weekday_image_obj[7];	//多选重复

	void *sound_btn;
	void *music_label_obj;		//铃声标签对象
	void *bells_obj;
	void *bell_cont_obj[12];
	void *cur_active_obj;
	
#ifdef DIVOOM_TIME_FRAME_SPEAKER
	divoom_alarm_tf_sound_pick_ctrl *tf_sound_pick;
#endif
	T_DIVOOM_ALARM_ACTIVE_DO_INFO *add_alarm_info;

	int hour;
	int minute;
	lv_timer_t *menu_bar_timer;
}divoom_lvgl_alarm_add_menu_ctrl, *Pdivoom_lvgl_alarm_add_menu_ctrl;

static const char *divoom_alarm_add_bell_menu_img_list[] = {
    "48/48_bell_rain.png",
    "48/48_bell_wave.png",
    "48/48_bell_city.png",
    "48/48_bell_forest.png",
    "48/48_bell_dolphin.png",
    "48/48_bell_sun.png",
    "48/48_bell_bird.png",
    "48/48_bell_ring.png",
    "48/48_bell_clock.png",
    "48/48_bell_piano.png",
    "48/48_bell_clock2.png",
    "48/48_bell_music.png",
};


#ifdef DIVOOM_TIME_FRAME_SPEAKER
static void divoom_alarm_add_menu_open_classic_cb(void *user_data)
{
	Pdivoom_lvgl_alarm_add_menu_ctrl phandle = (Pdivoom_lvgl_alarm_add_menu_ctrl)user_data;

	if(phandle == NULL)
	{
		return;
	}
	phandle->active_id = 3;
	lv_obj_clear_flag(phandle->bells_obj, LV_OBJ_FLAG_HIDDEN);
	if(phandle->cur_active_obj)
	{
		lv_group_focus_obj(phandle->cur_active_obj);
	}
}
#endif


void divoom_menu_alarm_refresh_add_menu_bar(lv_timer_t *menu_bar_timer)
{
    int hour;
    Pdivoom_lvgl_alarm_add_menu_ctrl phandle = (Pdivoom_lvgl_alarm_add_menu_ctrl)lv_timer_get_user_data(menu_bar_timer);
	time_t time_now;
	struct tm p_now;
	
    time_now = time(NULL);
	localtime_r(&time_now, &p_now);
	hour = p_now.tm_hour;
    if(divoom_main_get_time_flag() == 0)
    {
        if(hour != 12)
        {
            hour = hour % 12;
            if(hour == 0)
            {
                hour = 12;
            }
        }
    }

    if(hour != phandle->hour || p_now.tm_min != phandle->minute)
    {
        phandle->hour = hour;
        phandle->minute = p_now.tm_min;
        lv_label_set_text_fmt(phandle->time_obj, "%02d:%02d", hour, phandle->minute);
    }

}

void divoom_alarm_add_menu_event_cb(lv_event_t * e)
{
	int i;
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *cur_btn_obj = lv_event_get_target(e);
	Pdivoom_lvgl_alarm_add_menu_ctrl phandle = (Pdivoom_lvgl_alarm_add_menu_ctrl)lv_event_get_user_data(e);

	switch(code)
	{
		case LV_EVENT_KEY:
			if(cur_btn_obj == phandle->repeat_btn)
			{
				phandle->active_id = 2;
				lv_obj_clear_flag(phandle->bg_obj, LV_OBJ_FLAG_HIDDEN);
				lv_obj_clear_flag(phandle->week_list_obj, LV_OBJ_FLAG_HIDDEN);
			}
			else if(cur_btn_obj == phandle->sound_btn)
			{
#ifdef DIVOOM_TIME_FRAME_SPEAKER
				divoom_alarm_tf_sound_pick_open_select(phandle->tf_sound_pick);
#else
				phandle->active_id = 3;
				lv_obj_clear_flag(phandle->bells_obj, LV_OBJ_FLAG_HIDDEN);
				lv_group_focus_obj(phandle->cur_active_obj);
				phandle->cur_active_obj = NULL;
#endif
			}
            break;
        default:
            break;
	}

}


void divoom_alarm_add_menu_cancel_event_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	Pdivoom_lvgl_alarm_add_menu_ctrl phandle = (Pdivoom_lvgl_alarm_add_menu_ctrl)lv_event_get_user_data(e);
	
	switch(code)
	{
		case LV_EVENT_KEY:
            divoom_menu_control_return_pre();
            break;
        default:
            break;
	}

}

void divoom_alarm_add_menu_check_event_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	Pdivoom_lvgl_alarm_add_menu_ctrl phandle = (Pdivoom_lvgl_alarm_add_menu_ctrl)lv_event_get_user_data(e);
	
	switch(code)
	{
		case LV_EVENT_KEY:
			//添加闹钟
			phandle->add_alarm_info->hour = divoom_stream_cont_get_current_selected(phandle->stream_cont_hour);
			if(divoom_main_get_time_flag() == 0)
			{
				phandle->add_alarm_info->hour = phandle->am_pm_index ? (phandle->add_alarm_info->hour + 12) : (phandle->add_alarm_info->hour);
			}
			phandle->add_alarm_info->minute = divoom_stream_cont_get_current_selected(phandle->stream_cont_minute);
			//divoom_print("hour = %d,minute = %d,cyc = 0x%x,tip_index = %d, snooze = %d\n", phandle->add_alarm_info->hour, phandle->add_alarm_info->minute, phandle->add_alarm_info->cyc_flag, phandle->add_alarm_info->tip_index, phandle->add_alarm_info->snooze_flag);
			phandle->add_alarm_info->on_off = 1;
			phandle->add_alarm_info->type = DIVOOM_ALARM_TYPE_ALARM;
			divoom_alarm_menu_add_alarm_item(phandle->add_alarm_info);
            divoom_menu_control_return_pre();
        	break;
        default:
            break;
	}

}

void divoom_alarm_add_menu_set_repeat_label_text(Pdivoom_lvgl_alarm_add_menu_ctrl phandle)
{
	int i, index = 0;
	char disp_str[32];
	char weekday[7][5] = {"Mo","Tu","We","Th","Fr","Sa","Su"};

	for(i = 0;i < 7; i++)
	{
		if(phandle->add_alarm_info->cyc_flag & (1 << ((i + 1)%7)))
        {
          	sprintf(disp_str + index, "%s ", weekday[i]);
			index += 3;
       	}
	}
	if(index > 0)
	{
		disp_str[index - 1] = '\0';
		lv_label_set_text(phandle->cys_label_obj, divoom_menu_control_get_disp_str(disp_str));
	}
	else
	{
		lv_label_set_text(phandle->cys_label_obj, divoom_menu_control_get_disp_str("Never"));
	}
	
}


void divoom_alarm_add_menu_back_event_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	Pdivoom_lvgl_alarm_add_menu_ctrl phandle = (Pdivoom_lvgl_alarm_add_menu_ctrl)lv_event_get_user_data(e);
	
	switch(code)
	{
		case LV_EVENT_PRESSED:
			if(phandle->active_id == 2)
			{
				lv_obj_add_flag(phandle->bg_obj, LV_OBJ_FLAG_HIDDEN);
				lv_obj_add_flag(phandle->week_list_obj, LV_OBJ_FLAG_HIDDEN);
				//更新重复标签
				divoom_alarm_add_menu_set_repeat_label_text(phandle);
			}
			else if(phandle->active_id == 3)
			{
				divoom_alarm_ctrl_try_listen(0, 0, 0, 0);
				lv_obj_add_flag(phandle->bells_obj, LV_OBJ_FLAG_HIDDEN);
#ifdef DIVOOM_TIME_FRAME_SPEAKER
				divoom_alarm_tf_sound_pick_refresh_label(phandle->tf_sound_pick);
#else
				lv_label_set_text_fmt(phandle->music_label_obj, "Music %d", phandle->add_alarm_info->tip_index + 1);
#endif
			}
			phandle->active_id = 0;
			break;
        default:
            break;
	}

}

void divoom_alarm_add_menu_week_list_event_cb(lv_event_t * e)
{
	int i;
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *cur_btn_obj = lv_event_get_target(e);
	Pdivoom_lvgl_alarm_add_menu_ctrl phandle = (Pdivoom_lvgl_alarm_add_menu_ctrl)lv_event_get_user_data(e);
	
	switch(code)
	{
		case LV_EVENT_KEY:
			for(i = 0;i < 7; i++)
			{
				if(phandle->weekday_btn_obj[i] == cur_btn_obj)
				{
					if(phandle->add_alarm_info->cyc_flag & (1 << ((i+1)%7)))
           			{
               			 phandle->add_alarm_info->cyc_flag &= ~ (1 << ((i+1)%7));
               			 lv_obj_add_flag(phandle->weekday_image_obj[i], LV_OBJ_FLAG_HIDDEN);

            		}
            		else
            		{
                		phandle->add_alarm_info->cyc_flag |= (1 << ((i+1)%7));
                		lv_obj_clear_flag(phandle->weekday_image_obj[i], LV_OBJ_FLAG_HIDDEN);
            		}
					break;
				}
			}
        	break;
        default:
            break;
	}
}


void divoom_alarm_add_menu_create_week_list(Pdivoom_lvgl_alarm_add_menu_ctrl phandle, void *main_obj)
{
	int i;
	const char week_day[7][16] = {"Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday"};

	phandle->week_list_obj = lv_obj_create(main_obj);
	lv_obj_set_size(phandle->week_list_obj, 501, 572);
	lv_obj_add_style(phandle->week_list_obj, &phandle->obj_style, 0);
	lv_obj_set_style_radius(phandle->week_list_obj, 23, 0);
	lv_obj_set_style_bg_opa(phandle->week_list_obj, LV_OPA_0, 0);
	lv_obj_clear_flag(phandle->week_list_obj, LV_OBJ_FLAG_SCROLLABLE);
	
	if(divoom_main_get_time_flag())
	{
		lv_obj_set_pos(phandle->week_list_obj, 130, 321);
	}
	else
	{
		lv_obj_set_pos(phandle->week_list_obj, 150, 321);
	}
	
	lv_obj_t *box_img = lv_img_create(phandle->week_list_obj);
	divoom_menu_control_lv_img_set_src(box_img, "48/48_list_cont.png");

	lv_obj_t *week_list = lv_list_create(phandle->week_list_obj);
	lv_obj_clear_flag(week_list, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_opa(week_list, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(week_list, 0, 0);
	lv_obj_set_style_border_width(week_list, 0, 0);
	lv_obj_set_style_radius(week_list, 23, 0);
	lv_obj_set_style_bg_opa(week_list, LV_OPA_0, 0);
	lv_obj_set_size(week_list, 501, 572);
	lv_obj_set_style_base_dir(week_list, LV_BASE_DIR_LTR, 0);
	lv_obj_set_flex_flow(week_list, LV_FLEX_FLOW_COLUMN);

	lv_obj_t *week_obj = lv_obj_create(week_list);
	lv_obj_set_size(week_obj, 501, 96);
	lv_obj_add_style(week_obj, &phandle->obj_style, 0);
	lv_obj_set_style_bg_opa(week_obj, LV_OPA_0, 0);
	lv_obj_clear_flag(week_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_t *label = lv_label_create(week_obj);
	lv_obj_set_size(label, 330, 36);
	lv_obj_set_pos(label, 86, 30);
	lv_obj_set_style_text_color(label, lv_color_white(), 0);
	lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(36, phandle->menu_id), 0);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_text(label, divoom_menu_control_get_disp_str("Repeat"));
	lv_obj_t *back_btn = lv_btn_create(week_obj);
	lv_obj_set_style_bg_opa(back_btn, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(back_btn, 0, 0);
	lv_obj_set_style_shadow_width(back_btn, 0, 0);
	lv_obj_add_event(back_btn, divoom_alarm_add_menu_back_event_cb, LV_EVENT_PRESSED, phandle);
	lv_obj_t *back_img = lv_img_create(back_btn);
	divoom_menu_control_lv_img_set_src(back_img, "48/48_back.png");
	lv_obj_set_size(back_img, 48, 48);
	lv_obj_set_size(back_btn, 48, 48);
	lv_obj_set_pos(back_btn, 21, 24);
	
	for(i = 0;i < 7;i++)
	{
		lv_obj_t *weekday_btn = lv_btn_create(week_list);
		lv_obj_set_size(weekday_btn, 501, 68);
		lv_obj_clear_flag(weekday_btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);
		lv_obj_add_event_cb(weekday_btn, divoom_alarm_add_menu_week_list_event_cb, LV_EVENT_KEY, phandle);
		phandle->weekday_btn_obj[i] = weekday_btn;

		lv_obj_t *disp_weekday = lv_label_create(weekday_btn);
		lv_obj_set_size(disp_weekday, 393, 36);
		lv_obj_set_pos(disp_weekday, 36, 16);
		lv_obj_set_style_text_font(disp_weekday, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
		lv_obj_set_style_text_opa(disp_weekday, LV_OPA_100 * 85/100, 0);
		lv_label_set_text(disp_weekday, divoom_menu_control_get_disp_str(week_day[i]));

		lv_obj_t *weekday_img = lv_img_create(weekday_btn);
		divoom_menu_control_lv_img_set_src(weekday_img, "48/48_selete.png");
		lv_obj_set_size(weekday_img, 36, 36);
		lv_obj_set_pos(weekday_img, 429, 16);
		phandle->add_alarm_info->cyc_flag |= (1 << ((i+1)%7));
		phandle->weekday_image_obj[i] = weekday_img;

		lv_obj_add_style(weekday_btn, &phandle->btn_normal_type, 0);
    	lv_obj_add_style(weekday_btn, &phandle->btn_press_type, LV_STATE_PRESSED);
		
		
	}
	lv_obj_add_flag(phandle->week_list_obj, LV_OBJ_FLAG_HIDDEN);
}

void divoom_alarm_add_bell_menu_event_cb(lv_event_t * e)
{
	int tip_index;
	lv_obj_t *cur_obj;
    Pdivoom_lvgl_alarm_add_menu_ctrl phandle = (Pdivoom_lvgl_alarm_add_menu_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target(e);
    switch(code)
	{
		case LV_EVENT_KEY:
			for(int i = 0; i < 12; i++)
			{
				if(cur_obj == phandle->bell_cont_obj[i])
				{
					phandle->cur_active_obj = phandle->bell_cont_obj[i];
					phandle->add_alarm_info->tip_index = i;
					phandle->add_alarm_info->mode = 0;
					tip_index = i;
					divoom_alarm_ctrl_try_listen(tip_index, 50, 1, 1);
#ifdef DIVOOM_TIME_FRAME_SPEAKER
					divoom_alarm_tf_sound_pick_refresh_label(phandle->tf_sound_pick);
#endif
					break;
				}
			}
			break;

        default:
            break;

	}

}


void divoom_alarm_add_menu_create_bell_list(Pdivoom_lvgl_alarm_add_menu_ctrl phandle, void *main_obj)
{
	int i, j;

	lv_obj_t *bells_obj = lv_obj_create(main_obj);
	lv_obj_set_style_bg_color(bells_obj, lv_color_black(), 0);
	lv_obj_set_style_border_width(bells_obj, 0, 0);
	lv_obj_set_style_pad_all(bells_obj, 0, 0);
	lv_obj_set_size(bells_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(bells_obj, 0, 0);
	lv_obj_add_flag(bells_obj, LV_OBJ_FLAG_HIDDEN);
	phandle->bells_obj = bells_obj;
	
	lv_obj_t *back_btn = lv_btn_create(bells_obj);
	lv_obj_set_size(back_btn, 72, 72); 
	lv_obj_set_pos(back_btn, 15, 44);
	lv_obj_set_style_bg_opa(back_btn, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(back_btn, 0, 0);
	lv_obj_set_style_shadow_width(back_btn, 0, 0);
	lv_obj_clear_flag(back_btn, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_event(back_btn, divoom_alarm_add_menu_back_event_cb, LV_EVENT_PRESSED, phandle);
	
	lv_obj_t *back_img = lv_img_create(back_btn);
	divoom_menu_control_lv_img_set_src(back_img, "48/48_bell_back.png");

	lv_obj_t *bell_label = lv_label_create(bells_obj);
	lv_obj_set_size(bell_label, 360, 56);
	lv_obj_set_pos(bell_label, 220, 52);
	lv_obj_set_style_text_color(bell_label, lv_color_white(), 0);
	lv_obj_set_style_text_font(bell_label, divoom_menu_control_get_font_info(48, phandle->menu_id), 0);
	lv_obj_set_style_text_align(bell_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_text(bell_label, divoom_menu_control_get_disp_str("Bells"));

	lv_group_t *group = lv_group_create();
	for(i = 0; i < 3; i++)
	{
		for(j = 0; j < 4; j++)
		{
			lv_obj_t *bell_btn = lv_btn_create(phandle->bells_obj);
			lv_obj_set_style_pad_all(bell_btn, 0, 0);
			lv_obj_set_style_border_width(bell_btn, 4, 0);
			lv_obj_set_style_bg_color(bell_btn, lv_color_hex(0xE2E2E2), 0);
			lv_obj_set_style_bg_opa(bell_btn, 7*LV_OPA_100/100, 0);
			lv_obj_set_style_shadow_width(bell_btn, 0, 0);

			lv_obj_set_style_border_color(bell_btn, lv_color_hex(0xE2E2E2), 0);
			lv_obj_set_style_border_opa(bell_btn, 7*LV_OPA_100/100, 0);
			lv_obj_set_style_border_color(bell_btn, lv_color_hex(0xF1A33B), LV_STATE_FOCUSED);
			lv_obj_set_style_border_opa(bell_btn, LV_OPA_100, LV_STATE_FOCUSED);
			lv_obj_set_style_border_color(bell_btn, lv_color_hex(0xF1A33B), LV_STATE_FOCUS_KEY);
			lv_obj_set_style_border_opa(bell_btn, LV_OPA_100, LV_STATE_FOCUS_KEY);
			
			lv_obj_set_size(bell_btn, 150, 150);
			lv_obj_set_pos(bell_btn, 40 + 190*j, 208 + 190*i);

			lv_obj_t *bell_img = lv_img_create(bell_btn);
			divoom_menu_control_lv_img_set_src(bell_img, divoom_alarm_add_bell_menu_img_list[i*4+j]);
			lv_obj_center(bell_img);

			if(i == 0 && j == 0)
			{
				phandle->cur_active_obj = bell_btn;
			}
			phandle->bell_cont_obj[i*4+j] = bell_btn;
			lv_group_add_obj(group, bell_btn);
			lv_obj_add_event_cb(bell_btn, divoom_alarm_add_bell_menu_event_cb, LV_EVENT_KEY, phandle);
		}
	}
	
}

void divoom_alarm_add_menu_scroll(Pdivoom_lvgl_alarm_add_menu_ctrl phandle, int y)
{
	if(y <= 0 && y >= -80)//y对于0位置向上走过多少距离
	{
		if(y >= -40)
		{
			phandle->am_pm_index = 0;	
		}
		else
		{
			phandle->am_pm_index = 1;	
		}
		int diff_y = 160 *y/80;//最大改变坐标是160
		int diff_height = 160 * y/80;
		int diff_font_size = 20 * y/80;
		int diff_opa = LV_OPA_70 * y/80;
				
		lv_obj_set_y(phandle->pm_obj.cont, phandle->pm_obj.y + diff_y);//最小y坐标是160
				
		lv_obj_set_height(phandle->am_obj.cont, phandle->am_obj.obj_height + diff_height);
		lv_obj_set_height(phandle->pm_obj.cont, phandle->pm_obj.obj_height - diff_height);
				
		lv_obj_set_style_text_font(phandle->am_obj.time_label, divoom_menu_control_get_font_info(phandle->am_obj.font_size + diff_font_size, phandle->menu_id), 0);
		lv_obj_set_style_text_font(phandle->pm_obj.time_label, divoom_menu_control_get_font_info(phandle->pm_obj.font_size - diff_font_size, phandle->menu_id), 0);
				
		lv_obj_set_style_text_opa(phandle->am_obj.time_label, LV_OPA_100 + diff_opa, 0);
		lv_obj_set_style_text_opa(phandle->pm_obj.time_label, LV_OPA_30 - diff_opa, 0);
				
		lv_obj_center(phandle->am_obj.time_label);
		lv_obj_center(phandle->pm_obj.time_label);
	}
}

static void divoom_alarm_add_menu_scroll_event_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
	Pdivoom_lvgl_alarm_add_menu_ctrl phandle = (Pdivoom_lvgl_alarm_add_menu_ctrl)lv_event_get_user_data(e);

	int start_y = lv_obj_get_scroll_y(phandle->roller);//0<start_y<80 
	switch (code)
	{
		case LV_EVENT_SCROLL_BEGIN:
			break;
		case LV_EVENT_SCROLL_END:
			//对齐
			if(phandle->am_pm_index == 0)
			{
				divoom_alarm_add_menu_scroll(phandle, 0);
				lv_obj_scroll_to_y(phandle->roller, 0, LV_ANIM_OFF);
			}
			else
			{
				divoom_alarm_add_menu_scroll(phandle, -80);
				lv_obj_scroll_to_y(phandle->roller, 80, LV_ANIM_OFF);
			}
			break;
		case LV_EVENT_SCROLL:
		{
			//改变高度,位置,透明度和字体大小,
			int y = 0 - start_y;
			divoom_alarm_add_menu_scroll(phandle, y);
		}
			break;


        default:
            break;

		
	}
    
}


void *divoom_lvgl_alarm_add_menu_init(void *user_data, int menu_id)
{
	int alarm_num, hour, i;
	char roller_str[30];
	char disp_str[32];
	time_t time_now;
	struct tm p_now;
	Pdivoom_lvgl_alarm_add_menu_ctrl phandle;

	phandle = (Pdivoom_lvgl_alarm_add_menu_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_alarm_add_menu_ctrl));
	memset(phandle, 0, sizeof(divoom_lvgl_alarm_add_menu_ctrl));
	phandle->add_alarm_info = (T_DIVOOM_ALARM_ACTIVE_DO_INFO*)divoom_system_malloc(sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
	memset(phandle->add_alarm_info, 0, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
	phandle->active_id = 0;
    phandle->menu_id = menu_id;
	phandle->main_obj = lv_obj_create(NULL);
	lv_style_init(&phandle->backgroud_type);
    lv_style_set_bg_color(&phandle->backgroud_type, lv_color_hex(DIVOOM_MENNU_BACK_GROUD_COLOR));
    lv_style_set_text_color(&phandle->backgroud_type, lv_color_hex(DIVOOM_MENNU_TEXT_COLOR));
    lv_obj_add_style(phandle->main_obj, &phandle->backgroud_type, 0);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	
	lv_obj_t *top = lv_obj_create(phandle->main_obj);
	lv_obj_set_size(top, 800, 172);
	lv_obj_set_style_border_width(top, 0, 0);
	lv_obj_set_style_pad_all(top, 0, 0);
	lv_obj_set_style_bg_color(top, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(top, LV_OPA_100 * 46 / 100, 0);
	
	lv_obj_t *add_alarm = lv_label_create(top);
	lv_obj_set_pos(add_alarm, 40, 58);
    lv_obj_set_size(add_alarm, 400, 72);
	lv_obj_set_style_text_font(add_alarm, divoom_menu_control_get_font_info(56, phandle->menu_id), 0);
	lv_obj_set_style_text_color(add_alarm, lv_color_hex(DIVOOM_MENNU_TEXT_COLOR), 0);
	lv_label_set_text(add_alarm, divoom_menu_control_get_disp_str("AddAlarm"));

	lv_obj_t *time_label = lv_label_create(top);
	lv_obj_set_pos(time_label, 460, 50);
    lv_obj_set_size(time_label, 300, 72);
	lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(48, phandle->menu_id), 0);
	lv_obj_set_style_text_color(time_label, lv_color_hex(DIVOOM_MENNU_TEXT_COLOR), 0);
	lv_obj_set_style_text_opa(time_label, 217, 0);

	time_now = time(NULL);
	localtime_r(&time_now, &p_now);
	hour = p_now.tm_hour;
	if(divoom_main_get_time_flag() == 0)
    {
        if(hour != 12)
        {
            hour = hour % 12;
            if(hour == 0)
            {
                hour = 12;
            }
        }
    }
	sprintf(disp_str, "%02d:%02d", hour, p_now.tm_min);
	lv_label_set_text(time_label, disp_str);
	lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_RIGHT, 0);
	phandle->menu_bar_timer = lv_timer_create(divoom_menu_alarm_refresh_add_menu_bar, 1000, phandle);
	phandle->time_obj = time_label;

	if(divoom_main_get_time_flag() == 0)
	{
		//AM PM
		lv_obj_t *roller = lv_obj_create(phandle->main_obj);
		lv_obj_set_size(roller, 190, 400);
		lv_obj_set_pos(roller, 75, 204);
		lv_obj_set_style_pad_all(roller, 0, 0);
		lv_obj_set_style_border_width(roller, 0, 0);
		lv_obj_set_style_bg_opa(roller, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
		lv_obj_set_style_bg_opa(roller, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
		
		lv_obj_set_style_shadow_width(roller, 0, 0);
		lv_obj_set_style_bg_color(roller, lv_color_hex(0x000000), 0);
		lv_obj_set_style_text_color(roller, lv_color_hex(0xFFFFFF), 0);
		lv_obj_clear_flag(roller, LV_OBJ_FLAG_SCROLL_ELASTIC);
		phandle->roller = roller;
		phandle->am_pm_index = 0;
		
		for(i = 0; i < 4; i++)
		{
			lv_obj_t *obj = lv_obj_create(roller);
			lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);
			lv_obj_set_style_border_width(obj, 0, 0);
			lv_obj_set_style_pad_all(obj, 0, 0);
			lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
			lv_obj_set_x(obj, 0);
			lv_obj_set_width(obj, 190);
			
			//设置label
			lv_obj_t *label_obj = lv_label_create(obj);
			lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);
			lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
			lv_obj_center(label_obj);

			if (i == 0 || i == 3)
			{
				int y = (i == 0) ? 0 : 400;
				lv_obj_set_y(obj, y);
				lv_obj_set_height(obj, 80);
				lv_obj_set_style_opa(label_obj, LV_OPA_0, 0);
			}
			else if(i == 1)
			{
				phandle->am_obj.y = 80;
				phandle->am_obj.obj_height = 240;
				phandle->am_obj.font_size = 60;
				phandle->am_obj.cont = obj;
				phandle->am_obj.time_label = label_obj;
				lv_obj_set_y(obj, 80);
				lv_obj_set_height(obj, 240);
				lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("AM"));
				lv_obj_set_style_text_opa(label_obj, LV_OPA_100, 0);
				lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(60, phandle->menu_id), 0);
			}
			else if(i == 2)
			{
				phandle->pm_obj.y = 320;
				phandle->pm_obj.obj_height = 80;
				phandle->pm_obj.font_size = 40;
				phandle->pm_obj.cont = obj;
				phandle->pm_obj.time_label = label_obj;
				lv_obj_set_y(obj, 320);
				lv_obj_set_height(obj, 80);
				lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("PM"));
				lv_obj_set_style_text_opa(label_obj, LV_OPA_30, 0);
				lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(40, phandle->menu_id), 0);
			}		
		}
		lv_obj_set_scroll_dir(roller, LV_DIR_VER);
		lv_obj_add_event(phandle->roller, divoom_alarm_add_menu_scroll_event_handler, LV_EVENT_ALL, phandle); // 添加滚动事件处理程序

		phandle->stream_cont_hour = divoom_stream_cont_init(phandle->main_obj, 305, 185, 190, 480, 190, 160,
			80, 100, 0xFFFFFF, 12, 1, 2);//12小时制
		phandle->stream_cont_minute = divoom_stream_cont_init(phandle->main_obj, 535, 185, 190, 480, 190, 160,
			80, 100, 0xFFFFFF, 60, 0, 2);
	}
	else
	{
		phandle->stream_cont_hour = divoom_stream_cont_init(phandle->main_obj, 190, 185, 190, 480, 190, 160, 
			80, 100, 0xFFFFFF, 24, 0, 2);
		phandle->stream_cont_minute = divoom_stream_cont_init(phandle->main_obj, 420, 185, 190, 480, 190, 160, 
			80, 100, 0xFFFFFF, 60, 0, 2);
		
	}
	lv_obj_t *rect_img = lv_img_create(phandle->main_obj);
	divoom_menu_control_lv_img_set_src(rect_img, "48/48_rect.png");
	lv_obj_set_pos(rect_img, 35, 321);
	
	lv_style_init(&phandle->obj_style);
	lv_style_set_pad_all(&phandle->obj_style, 0);
	lv_style_set_border_width(&phandle->obj_style, 0);
	
	lv_style_init(&phandle->btn_normal_type);
	lv_style_set_bg_opa(&phandle->btn_normal_type, LV_OPA_10);
    lv_style_set_bg_color(&phandle->btn_normal_type, lv_color_white());
    lv_style_set_pad_all(&phandle->btn_normal_type, 0);
    lv_style_set_border_width(&phandle->btn_normal_type, 0);
    lv_style_set_outline_width(&phandle->btn_normal_type, 0);
    lv_style_set_shadow_width(&phandle->btn_normal_type, 0);
    lv_style_set_border_side(&phandle->btn_normal_type, LV_BORDER_SIDE_NONE);
	lv_style_set_radius(&phandle->btn_normal_type, 0);

    lv_style_init(&phandle->btn_press_type);
	lv_style_set_bg_opa(&phandle->btn_press_type, LV_OPA_50);
    lv_style_set_bg_color(&phandle->btn_press_type, lv_color_white());
    lv_style_set_border_width(&phandle->btn_press_type, 0);
    lv_style_set_shadow_width(&phandle->btn_press_type, 0);
    lv_style_set_outline_width(&phandle->btn_press_type, 0);
	lv_style_set_pad_all(&phandle->btn_press_type, 0);
    lv_style_set_border_side(&phandle->btn_press_type, LV_BORDER_SIDE_NONE);
    lv_style_set_radius(&phandle->btn_press_type, 0);
	
	//lv_obj_t *cont = lv_obj_create(phandle->main_obj);
	lv_obj_t *list = lv_list_create(phandle->main_obj);
	lv_obj_clear_flag(list, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_set_style_bg_color(list, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(list, LV_OPA_10, 0);
	lv_obj_set_style_pad_all(list, 0, 0);
	lv_obj_set_style_border_width(list, 0, 0);
	lv_obj_set_style_radius(list, 32, 0);
	lv_obj_set_size(list, 728, 172);
	lv_obj_set_pos(list, 36, 744);
	lv_obj_set_style_base_dir(list, LV_BASE_DIR_LTR, 0);
	lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
	
	
	for(i = 1;i < 3; i++)
	{
		if(i == 1)
		{
			//重复
			lv_obj_t *repeat_btn = lv_btn_create(list);
			lv_obj_set_size(repeat_btn, 728, 86);
			lv_obj_clear_flag(repeat_btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);
			lv_obj_add_event_cb(repeat_btn, divoom_alarm_add_menu_event_cb, LV_EVENT_KEY, phandle);
			phandle->repeat_btn = repeat_btn;
			
			lv_obj_t *disp_repeat = lv_label_create(repeat_btn);
			lv_obj_set_size(disp_repeat, 210, 42);
			lv_obj_set_pos(disp_repeat, 40, 22);
			lv_obj_set_style_text_font(disp_repeat, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
			lv_label_set_text(disp_repeat, divoom_menu_control_get_disp_str("Repeat"));
			lv_obj_t *cys_label = lv_label_create(repeat_btn);
			lv_obj_set_style_text_opa(cys_label, LV_OPA_60, 0);
			lv_obj_set_size(cys_label, 461, 37);
			lv_obj_set_pos(cys_label, 201, 25);
			lv_obj_set_style_text_font(cys_label, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
			lv_label_set_text(cys_label, divoom_menu_control_get_disp_str("Mo Tu We Th Fr Sa Su"));
			lv_obj_set_style_text_align(cys_label, LV_TEXT_ALIGN_RIGHT, 0);
			phandle->cys_label_obj = cys_label;
			lv_obj_t *repeat_img = lv_img_create(repeat_btn);
			divoom_menu_control_lv_img_set_src(repeat_img, "48/48_right.png");
			lv_obj_set_size(repeat_img, 36, 36);
			lv_obj_set_pos(repeat_img, 666, 25);
			lv_obj_set_style_img_opa(repeat_img, LV_OPA_100, 0);

			lv_obj_add_style(repeat_btn, &phandle->btn_normal_type, 0);
   			lv_obj_add_style(repeat_btn, &phandle->btn_press_type, LV_STATE_PRESSED);

			lv_obj_t *line_img2 = lv_img_create(phandle->main_obj);
			divoom_menu_control_lv_img_set_src(line_img2, "48/48_line.png");
			lv_obj_set_size(line_img2, 688, 2);
			lv_obj_set_pos(line_img2, 76, 830);
			lv_obj_set_style_img_opa(line_img2, LV_OPA_100, 0);

		}
		if(i == 2)
		{
			//铃声
			lv_obj_t *sound_btn = lv_btn_create(list);
			lv_obj_set_size(sound_btn, 728, 86);
			lv_obj_clear_flag(sound_btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);
			phandle->sound_btn = sound_btn;
			lv_obj_add_event_cb(sound_btn, divoom_alarm_add_menu_event_cb, LV_EVENT_KEY, phandle);
				
			lv_obj_t *disp_sound = lv_label_create(sound_btn);
			lv_obj_set_size(disp_sound, 210, 42);
			lv_obj_set_pos(disp_sound, 40, 22);
			lv_obj_set_style_text_font(disp_sound, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
			lv_label_set_text(disp_sound, divoom_menu_control_get_disp_str("Bells"));
			lv_obj_t *music_label = lv_label_create(sound_btn);
			lv_obj_set_style_text_opa(music_label, LV_OPA_60, 0);
			lv_obj_set_size(music_label, 210, 37);
			lv_obj_set_pos(music_label, 452, 25);
			lv_obj_set_style_text_font(music_label, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
			lv_label_set_text(music_label, divoom_menu_control_get_disp_str("Music 1"));
			lv_obj_set_style_text_align(music_label, LV_TEXT_ALIGN_RIGHT, 0);
			phandle->music_label_obj = music_label;
			lv_obj_t *sound_img = lv_img_create(sound_btn);
			divoom_menu_control_lv_img_set_src(sound_img, "48/48_right.png");
			lv_obj_set_size(sound_img, 36, 36);
			lv_obj_set_pos(sound_img, 666, 25);
			lv_obj_set_style_img_opa(sound_img, LV_OPA_100, 0);
			lv_obj_add_style(sound_btn, &phandle->btn_normal_type, 0);
    		lv_obj_add_style(sound_btn, &phandle->btn_press_type, LV_STATE_PRESSED);

		}
		
	}
	
	lv_obj_t *add_btn = lv_btn_create(phandle->main_obj);
	lv_obj_set_size(add_btn, 180, 180);
	lv_obj_set_pos(add_btn, 445, 1040);
	lv_obj_set_style_radius(add_btn, 99, 0);
	lv_obj_set_style_pad_all(add_btn, 0, 0);
	lv_obj_set_style_bg_opa(add_btn, LV_OPA_0, 0);
	lv_obj_set_style_shadow_width(add_btn, 0, 0);
	lv_obj_set_style_bg_color(add_btn, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(add_btn, LV_OPA_10, LV_STATE_PRESSED);
	
	lv_obj_t *add_img = lv_img_create(add_btn);
	divoom_menu_control_lv_img_set_src(add_img, "48/48_check.png");
	lv_obj_add_event_cb(add_btn, divoom_alarm_add_menu_check_event_cb, LV_EVENT_KEY, phandle);
	
	lv_obj_t *cancel_btn = lv_btn_create(phandle->main_obj);
	lv_obj_set_size(cancel_btn, 180, 180);
	lv_obj_set_pos(cancel_btn, 175, 1040);
	lv_obj_set_style_radius(cancel_btn, 99, 0);
	lv_obj_set_style_pad_all(cancel_btn, 0, 0);
	lv_obj_set_style_bg_opa(cancel_btn, LV_OPA_0, 0);
	lv_obj_set_style_shadow_width(cancel_btn, 0, 0);
	lv_obj_set_style_bg_color(cancel_btn, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(cancel_btn, LV_OPA_10, LV_STATE_PRESSED);

	lv_obj_t *cancel_img = lv_img_create(cancel_btn);
	divoom_menu_control_lv_img_set_src(cancel_img, "48/48_cancel.png");
	lv_obj_add_event_cb(cancel_btn, divoom_alarm_add_menu_cancel_event_cb, LV_EVENT_KEY, phandle);

	lv_obj_t *bg_obj = lv_obj_create(phandle->main_obj);
	lv_obj_set_size(bg_obj, 800, 1280);
	lv_obj_set_pos(bg_obj, 0, 0);
	lv_obj_set_style_bg_opa(bg_obj, LV_OPA_0, 0);
	lv_obj_set_style_border_width(bg_obj, 0, 0);
	lv_obj_set_style_pad_all(bg_obj, 0, 0);
	lv_obj_clear_flag(bg_obj, LV_OBJ_FLAG_SCROLLABLE);
	phandle->bg_obj = bg_obj;
	
	lv_obj_t *bg_img = lv_img_create(bg_obj);
	divoom_menu_control_lv_img_set_src(bg_img, "48/48_bg_cover.png");
	lv_obj_add_flag(phandle->bg_obj, LV_OBJ_FLAG_HIDDEN);

	divoom_alarm_add_menu_create_week_list(phandle, phandle->main_obj);
	divoom_alarm_add_menu_create_bell_list(phandle, phandle->main_obj);
#ifdef DIVOOM_TIME_FRAME_SPEAKER
	{
		divoom_alarm_tf_sound_pick_cfg pick_cfg;

		memset(&pick_cfg, 0, sizeof(pick_cfg));
		pick_cfg.menu_id = menu_id;
		pick_cfg.parent = phandle->main_obj;
		pick_cfg.alarm_info = phandle->add_alarm_info;
		pick_cfg.sound_btn = phandle->sound_btn;
		pick_cfg.music_label_obj = phandle->music_label_obj;
		pick_cfg.open_classic = divoom_alarm_add_menu_open_classic_cb;
		pick_cfg.user_data = phandle;
		phandle->tf_sound_pick = divoom_alarm_tf_sound_pick_create(&pick_cfg);
	}
#endif
	return phandle;
}

void divoom_lvgl_alarm_add_menu_disp(void *handle, void *active)
{
    Pdivoom_lvgl_alarm_add_menu_ctrl phandle = (Pdivoom_lvgl_alarm_add_menu_ctrl)handle;
    
    lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_alarm_add_menu_eixt(void *handle)
{
	Pdivoom_lvgl_alarm_add_menu_ctrl phandle = (Pdivoom_lvgl_alarm_add_menu_ctrl)handle;

#ifdef DIVOOM_TIME_FRAME_SPEAKER
	if(phandle->tf_sound_pick)
	{
		divoom_alarm_tf_sound_pick_destroy(phandle->tf_sound_pick);
		phandle->tf_sound_pick = NULL;
	}
#endif
	lv_style_reset(&phandle->backgroud_type);
	lv_style_reset(&phandle->obj_style);
	lv_style_reset(&phandle->btn_normal_type);
	lv_style_reset(&phandle->btn_press_type);
	divoom_stream_cont_del(phandle->stream_cont_hour);
	divoom_stream_cont_del(phandle->stream_cont_minute);
	lv_obj_del(phandle->main_obj);
	if(phandle->menu_bar_timer)
	{
		lv_timer_del(phandle->menu_bar_timer);
	}
	divoom_system_free(phandle->add_alarm_info);
	divoom_system_free(phandle);

}





















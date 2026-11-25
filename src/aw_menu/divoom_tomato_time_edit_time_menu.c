#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_menu_direct_ctrl.h"
#include "divoom_tomato_time_menu.h"

#define DIVOOM_MAX_TIME_LABEL	 	99
#define DIVOOM_TABLE_HEIGHT			160
typedef struct _divoom_tomato_time_edit_time_menu_ctrl
{
    int menu_id;
	lv_style_t backgroud_type;
	void *main_obj;
	Pdivoom_tomato_time_info tomato_data;
    struct _lv_timer_t * tomato_refresh_time;
	int time_edit_index;
	lv_style_t style;
	lv_obj_t *arrow_btn_obj;
	lv_obj_t *label_time_obj;
	lv_obj_t *start_btn_obj;
	lv_obj_t *roller;
	lv_obj_t *cont_obj;
	lv_obj_t *time_select_btn_obj;
	lv_point_t start_pressed_point;
	lv_point_t end_pressed_point;
	int screen_status;	//	   0:正常状态 1：屏被按下 
	int time_index;
	int last_pos_y; 
	int slider_type; //滚动方向
	int roller_flag;
	lv_obj_t *label_obj[3];
}divoom_tomato_time_edit_time_menu_ctrl, *Pdivoom_tomato_time_edit_time_menu_ctrl;

static char menu_text_table[][20] = {
	"Work",
	"ShortRest",
	"LongRest",
	"Ok",
};

void divoom_tomato_time_edit_menu_time_display_time_label(void *obj);
void divoom_tomato_time_edit_menu_change_label(void *handle, int new_page) ;
void divoom_tomato_time_edit_menu_time_change_item(void *handle);

void divoom_tomato_time_edit_menu_time(Pdivoom_tomato_time_edit_time_menu_ctrl phandle)
{
	int i = phandle->tomato_data->edit_item;

	phandle->tomato_data->update_flag = 1;
	if (i == 0)
	{
		phandle->tomato_data->work_time = phandle->time_index;
	}
	else if (i == 1)
	{
		phandle->tomato_data->short_rest_time = phandle->time_index;
	}
	else
	{
		phandle->tomato_data->long_rest_time = phandle->time_index;
	}
}

static void divoom_tomato_time_edit_menu_event_handler(lv_event_t * e)
{
	Pdivoom_tomato_time_edit_time_menu_ctrl phandle = (Pdivoom_tomato_time_edit_time_menu_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
	
	if (code == LV_EVENT_KEY)
	{
		char c = *((char *)lv_event_get_param(e));

		if (c == DIVOOM_LVGL_KEY_NEXT)
		{
			phandle->time_index++;
			if (phandle->time_index > DIVOOM_MAX_TIME_LABEL)
				phandle->time_index = 1;

//			divoom_tomato_time_edit_menu_change_label(phandle, phandle->time_index - 1);
			divoom_tomato_time_edit_menu_time_change_item(phandle);
		}
		else if (c == DIVOOM_LVGL_KEY_PRE)
		{
			phandle->time_index--;
			if (phandle->time_index < 1)
				phandle->time_index = DIVOOM_MAX_TIME_LABEL;

//			divoom_tomato_time_edit_menu_change_label(phandle, phandle->time_index - 1);
			divoom_tomato_time_edit_menu_time_change_item(phandle);
		}
		else if (c == DIVOOM_LVGL_KEY_ENTER)
		{
			divoom_tomato_time_edit_menu_time(phandle);
			divoom_menu_control_return_pre();
		}
		else if (c == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_menu_control_return_pre();
		}
		else
		{
			char *key_info = (char *)lv_event_get_param(e);
			divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
		}
	}
}

void divoom_tomato_time_edit_menu_change_label(void *handle, int new_page) 
{
	Pdivoom_tomato_time_edit_time_menu_ctrl phandle = (Pdivoom_tomato_time_edit_time_menu_ctrl)handle;
    int label_width = DIVOOM_TABLE_HEIGHT; 
    int target_y = new_page * label_width;

    lv_obj_scroll_to_y(phandle->roller, target_y, LV_ANIM_OFF);
}

void divoom_tomato_time_edit_menu_in_scrollable_container(void *handle) 
{
	Pdivoom_tomato_time_edit_time_menu_ctrl phandle = (Pdivoom_tomato_time_edit_time_menu_ctrl)handle;
	
	int i = 0, y = 0;
	//创建一个滚动容器
	phandle->roller = lv_btn_create(phandle->time_select_btn_obj);
	lv_obj_remove_style_all(phandle->roller);
	lv_obj_set_width(phandle->roller, 700);
	lv_obj_set_height(phandle->roller, 480); 

	lv_obj_set_style_pad_all(phandle->roller, 0, 0);
	lv_obj_set_style_border_opa(phandle->roller, LV_OPA_0, 0);	 
	lv_obj_set_style_bg_color(phandle->roller, lv_color_hex(0x000000), 0);
	lv_obj_set_style_bg_opa(phandle->roller, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	lv_obj_set_style_bg_opa(phandle->roller, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_shadow_width(phandle->roller, 0, 0); //去掉边框底部的阴影
	lv_obj_set_style_text_color(phandle->roller, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_font(phandle->roller, divoom_menu_control_get_font_info2(24, 150, phandle->menu_id), 0);
//	lv_obj_clear_flag(phandle->roller, LV_OBJ_FLAG_SCROLLABLE);
	for (i = 0; i < DIVOOM_MAX_TIME_LABEL; i++)
	{
		//设置label
		lv_obj_t *label_obj = lv_label_create(phandle->roller);
		lv_obj_set_x(label_obj, 0);
		lv_obj_set_y(label_obj, i * DIVOOM_TABLE_HEIGHT);
		lv_obj_set_width(label_obj, 700);
		lv_obj_set_height(label_obj, DIVOOM_TABLE_HEIGHT);
		lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);

		if (i == 0 || i == DIVOOM_MAX_TIME_LABEL - 1)
		{
			lv_label_set_text_fmt(label_obj, "%d:00", i);
			lv_obj_set_style_opa(label_obj, LV_OPA_0, 0);
		}
		else
		{
			lv_label_set_text_fmt(label_obj, "%d:00", i);
		}
	}

	lv_obj_set_scroll_dir(phandle->roller, LV_DIR_VER);
	lv_obj_center(phandle->roller);
	phandle->cont_obj = phandle->roller;
    lv_obj_add_event(phandle->roller, divoom_tomato_time_edit_menu_event_handler, LV_EVENT_KEY, phandle); // 添加滚动事件处理程序
}

void divoom_tomato_time_edit_menu_time_change_item(void *handle)
{
	Pdivoom_tomato_time_edit_time_menu_ctrl phandle = (Pdivoom_tomato_time_edit_time_menu_ctrl)handle;

	int i, time_index = phandle->time_index;

	for (i = 0; i < 3; i++)
	{
		if (i == 0)
		{
			lv_label_set_text_fmt(phandle->label_obj[i], "%d:00", time_index -1);
			if (time_index -1 < 1)
			{
				lv_label_set_text_fmt(phandle->label_obj[i], "%d:00", 99);
			}
		}
		else if (i == 1)
		{
			lv_label_set_text_fmt(phandle->label_obj[i], "%d:00", time_index);
		}
		else
		{
			lv_label_set_text_fmt(phandle->label_obj[i], "%d:00", time_index + 1);
			if (time_index + 1 > 99)
			{
				lv_label_set_text_fmt(phandle->label_obj[i], "%d:00", 1);
			}
		}
	}
}

void divoom_tomato_time_edit_menu_time_create_table(void *handle)
{
	Pdivoom_tomato_time_edit_time_menu_ctrl phandle = (Pdivoom_tomato_time_edit_time_menu_ctrl)handle;
	
	int i = 0, y = 0, time_index = 0;
	phandle->roller = lv_btn_create(phandle->time_select_btn_obj);
	lv_obj_remove_style_all(phandle->roller);
	lv_obj_set_width(phandle->roller, 700);
	lv_obj_set_height(phandle->roller, 480); 

	lv_obj_set_style_pad_all(phandle->roller, 0, 0);
	lv_obj_set_style_border_opa(phandle->roller, LV_OPA_0, 0);	 
	lv_obj_set_style_bg_color(phandle->roller, lv_color_hex(0xD8D8D8), 0);
	lv_obj_set_style_bg_opa(phandle->roller, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	lv_obj_set_style_bg_opa(phandle->roller, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_shadow_width(phandle->roller, 0, 0); //去掉边框底部的阴影
	lv_obj_set_style_text_color(phandle->roller, lv_color_hex(DIVOOM_TOMATO_MENU_TEXT_COLOR), 0);
	lv_obj_set_style_text_font(phandle->roller, divoom_menu_control_get_font_info2(24, 150, phandle->menu_id), 0);
//	lv_obj_clear_flag(phandle->roller, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_center(phandle->roller);
	time_index = phandle->time_index;

	for (i = 0; i < 3; i++)
	{
		phandle->label_obj[i] = lv_label_create(phandle->roller);
		lv_obj_set_x(phandle->label_obj[i], 0);
		lv_obj_set_y(phandle->label_obj[i], i * DIVOOM_TABLE_HEIGHT);
		lv_obj_set_width(phandle->label_obj[i], 700);
		lv_obj_set_height(phandle->label_obj[i], DIVOOM_TABLE_HEIGHT);
		lv_obj_set_style_text_align(phandle->label_obj[i], LV_TEXT_ALIGN_CENTER, 0);
		lv_obj_set_style_text_color(phandle->label_obj[i], lv_color_hex(0x999999), 0);
		if (i == 1)
			lv_obj_set_style_text_color(phandle->label_obj[i], lv_color_hex(0x333333), 0);

		if (i == 0)
		{
			lv_label_set_text_fmt(phandle->label_obj[i], "%d:00", time_index -1);
			if (time_index -1 < 1)
			{
				lv_label_set_text_fmt(phandle->label_obj[i], "%d:00", 99);
			}
		}
		else if (i == 1)
		{
			lv_label_set_text_fmt(phandle->label_obj[i], "%d:00", time_index);
		}
		else
		{
			lv_label_set_text_fmt(phandle->label_obj[i], "%d:00", time_index + 1);
			if (time_index + 1 > 99)
			{
				lv_label_set_text_fmt(phandle->label_obj[i], "%d:00", 1);
			}
		}
	}
	phandle->cont_obj = phandle->roller;
    lv_obj_add_event(phandle->roller, divoom_tomato_time_edit_menu_event_handler, LV_EVENT_KEY, phandle); // 添加滚动事件处理程序
}

void divoom_tomato_time_edit_menu_time_create_image_grad(Pdivoom_tomato_time_edit_time_menu_ctrl phandle)
{
	//时间对象
	phandle->label_time_obj = divoom_tomato_time_menu_set_label_time_obj(phandle->main_obj, 340, 60, phandle->menu_id);
	divoom_tomato_time_menu_display_time_label(phandle->label_time_obj);

	//文本框
	lv_obj_t *label_obj = lv_label_create(phandle->main_obj);
	lv_obj_remove_style_all(label_obj);
	lv_obj_set_x(label_obj, 40);
	lv_obj_set_y(label_obj, 160);
	lv_obj_set_width(label_obj, 720);
	lv_obj_set_height(label_obj, 78);
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0xD43939), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(80, phandle->menu_id), 0);
	int i = phandle->tomato_data->edit_item;
	if (i == 0)
		lv_obj_set_style_text_color(label_obj, lv_color_hex(0xD43939), 0);
	else if (i == 1)
		lv_obj_set_style_text_color(label_obj, lv_color_hex(0xE1842D), 0);
	else 
		lv_obj_set_style_text_color(label_obj, lv_color_hex(0x1CB1DF), 0);
	lv_label_set_text_fmt(label_obj, "%s", divoom_menu_control_get_disp_str(menu_text_table[i]));

	//矩形
	phandle->time_select_btn_obj = lv_obj_create(phandle->main_obj);
	lv_obj_set_x(phandle->time_select_btn_obj, 40);
	lv_obj_set_y(phandle->time_select_btn_obj, 298);
	lv_obj_set_size(phandle->time_select_btn_obj, 720, 500);
	lv_obj_set_style_pad_all(phandle->time_select_btn_obj, 0, 0);
	lv_obj_set_style_radius(phandle->time_select_btn_obj, 51, 0);
	lv_obj_set_style_border_width(phandle->time_select_btn_obj, 8, 0);
	lv_obj_set_style_border_color(phandle->time_select_btn_obj, lv_color_hex(0xBBBBBB), 0);
	lv_obj_set_style_bg_color(phandle->time_select_btn_obj, lv_color_hex(0xD8D8D8), 0);
	lv_obj_set_style_bg_opa(phandle->time_select_btn_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	lv_obj_set_style_bg_opa(phandle->time_select_btn_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_shadow_width(phandle->time_select_btn_obj, 0, 0); //去掉边框底部的阴影
	lv_obj_clear_flag(phandle->time_select_btn_obj, LV_OBJ_FLAG_SCROLLABLE);	

	if (phandle->tomato_data->edit_item == 0)
		phandle->time_index = phandle->tomato_data->work_time;
	else if (phandle->tomato_data->edit_item == 1)
		phandle->time_index = phandle->tomato_data->short_rest_time;
	else
		phandle->time_index = phandle->tomato_data->long_rest_time;

	divoom_tomato_time_edit_menu_time_create_table(phandle);
	lv_obj_t *img = lv_img_create(phandle->time_select_btn_obj);
	lv_obj_set_align(img, LV_ALIGN_CENTER);
	divoom_menu_control_lv_img_set_src(img, "27/27_time_seting_bg.png");

	//min
	lv_obj_t *label_obj1 = lv_label_create(phandle->main_obj);
	lv_obj_remove_style_all(label_obj1);
	lv_obj_set_x(label_obj1, 40);
	lv_obj_set_y(label_obj1, 858);
	lv_obj_set_size(label_obj1, 720, 78);
    lv_obj_set_style_text_align(label_obj1, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_color(label_obj1, lv_color_hex(0x333333), 0);
	lv_obj_set_style_text_font(label_obj1, divoom_menu_control_get_font_info(60, phandle->menu_id), 0);
	lv_label_set_text(label_obj1, "Min");
}

void *divoom_tomato_time_edit_time_menu_init(void *user_data, int menu_id)
{
	Pdivoom_tomato_time_edit_time_menu_ctrl phandle = divoom_system_malloc(sizeof(divoom_tomato_time_edit_time_menu_ctrl));
    phandle->menu_id = menu_id;
    phandle->tomato_data = (Pdivoom_tomato_time_info)user_data;
    phandle->main_obj = lv_obj_create(NULL);
    lv_style_init(&phandle->backgroud_type);
    lv_style_set_bg_color(&phandle->backgroud_type, lv_color_hex(DIVOOM_TOMATO_MENU_BACK_GROUD_COLOR));
    lv_style_set_text_color(&phandle->backgroud_type, lv_color_hex(DIVOOM_TOMATO_MENU_TEXT_COLOR));
    lv_obj_add_style(phandle->main_obj, &phandle->backgroud_type, 0);
    lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    divoom_tomato_time_edit_menu_time_create_image_grad(phandle);
    return phandle;
}

void divoom_tomato_time_edit_time_menu_disp(void *handle, void *active)
{
	Pdivoom_tomato_time_edit_time_menu_ctrl phandle = (Pdivoom_tomato_time_edit_time_menu_ctrl)handle;
    
	lv_group_focus_obj(phandle->cont_obj);
    lv_scr_load(phandle->main_obj);
}

void divoom_tomato_time_edit_time_menu_clean(void *handle)
{
	Pdivoom_tomato_time_edit_time_menu_ctrl phandle = (Pdivoom_tomato_time_edit_time_menu_ctrl)handle;
	
	if (phandle->tomato_refresh_time)
	{
		lv_timer_del(phandle->tomato_refresh_time);
	}

    lv_obj_del(phandle->main_obj);
	lv_style_reset(&phandle->backgroud_type);
	lv_style_reset(&phandle->style);
    divoom_system_free(phandle);
}


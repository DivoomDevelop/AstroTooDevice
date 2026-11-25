#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_menu_direct_ctrl.h"
#include "divoom_tomato_time_menu.h"

typedef struct _divoom_tomato_time_edit_menu_ctrl
{
    int menu_id;
	lv_obj_t *tomato_repeat_obj;
	lv_obj_t *tomato_work_time_obj;
	lv_obj_t *tomato_short_time_obj;
	lv_obj_t *tomato_long_time_obj;
	lv_obj_t *tomato_bell_obj;
	lv_style_t cont_type;
	lv_style_t backgroud_type;
	lv_style_t btn_type;
	lv_style_t btn_press_type;
	void *main_obj;
	Pdivoom_tomato_time_info tomato_data;
	lv_obj_t *arrow_btn_obj;
	lv_obj_t *btn_table_obj[5];
	lv_style_t style_line;
	void *first_active_obj;
	void *pdirect_handle;
}divoom_tomato_time_edit_menu_ctrl, *Pdivoom_tomato_time_edit_menu_ctrl;

static char menu_text_table[][20] = {
	"TargetEditing",
	"Work",
	"ShortRest",
	"LongRest",
	"Repeat",
	"Bells",
};

static void divoom_tomato_time_edit_menu_event_cb(lv_event_t * e)
{
	Pdivoom_tomato_time_edit_menu_ctrl phandle = (Pdivoom_tomato_time_edit_menu_ctrl)lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *cur_obj = lv_event_get_target(e);
	int select_id = 0;

	if (code == LV_EVENT_KEY)
	{
		char c = *((char *)lv_event_get_param(e));
        int cur_pos = divoom_menu_direct_ctrl_get_pos(phandle->pdirect_handle, cur_obj);

		if (c == DIVOOM_LVGL_KEY_PRE)
		{
			cur_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_obj, DIVOOM_MENU_DIRECT_UP);
			lv_group_focus_obj(cur_obj);
		}
		else if (c == DIVOOM_LVGL_KEY_NEXT)
		{
			cur_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_obj, DIVOOM_MENU_DIRECT_DOWN);
			lv_group_focus_obj(cur_obj);
		}
		else if (c == DIVOOM_LVGL_KEY_ENTER)
		{
			if (cur_pos == 0 || cur_pos == 1 || cur_pos == 2)
			{
				phandle->tomato_data->edit_item = cur_pos;
				divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_TOMA_EDIT_TIME, phandle->tomato_data, 0, 0);
			}
			else if (cur_pos == 3)
			{
				divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_TOMA_EDIT_REPEAT, phandle->tomato_data, 0, 0);
			}	
			else
			{
				divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_TOMA_EDIT_BELL, phandle->tomato_data, 0, 0);
			}
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

void divoom_tomato_time_edit_menu_create_image_grad(Pdivoom_tomato_time_edit_menu_ctrl phandle)
{
	int i = 0;

//    divoom_menu_control_init_btn_stype(&phandle->btn_type, NULL, NULL, NULL, &phandle->btn_press_type, NULL, 19);
	lv_style_init(&phandle->btn_type);
	lv_style_set_bg_color(&phandle->btn_type, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->btn_type, 0);
	lv_style_set_border_width(&phandle->btn_type, 1);
	lv_style_set_border_color(&phandle->btn_type, lv_color_hex(0x6D6D6D));
	lv_style_set_radius(&phandle->btn_type, 17);
	lv_style_set_shadow_width(&phandle->btn_type, 0);
	lv_style_set_outline_width(&phandle->btn_type, 0);

	lv_style_init(&phandle->btn_press_type);
	lv_style_set_bg_color(&phandle->btn_press_type, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->btn_press_type, 0);
	lv_style_set_border_width(&phandle->btn_press_type, 4);
	lv_style_set_border_color(&phandle->btn_press_type, lv_color_hex(0xBBBBBB));
	lv_style_set_radius(&phandle->btn_press_type, 17);
	lv_style_set_shadow_width(&phandle->btn_press_type, 0);
	lv_style_set_outline_width(&phandle->btn_press_type, 0);

	//文本框
	lv_obj_t *label_obj = lv_label_create(phandle->main_obj);
//	lv_obj_remove_style_all(label_obj);
	lv_obj_set_x(label_obj, 0);
	lv_obj_set_y(label_obj, 60);
	lv_obj_set_size(label_obj, 720, 40);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_color(label_obj, lv_color_hex(DIVOOM_TOMATO_MENU_TEXT_COLOR), 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(40, phandle->menu_id), 0);
	lv_label_set_text_fmt(label_obj, "%s", divoom_menu_control_get_disp_str(menu_text_table[0]));

	//列表
	lv_obj_t *table_obj = lv_obj_create(phandle->main_obj);
	lv_obj_set_x(table_obj, 40);
	lv_obj_set_y(table_obj, 160);
	lv_obj_set_width(table_obj, 720);
	lv_obj_set_height(table_obj, 450 +4*8+ 44);
	lv_obj_set_style_radius(table_obj, 18, 0);
	lv_obj_set_style_pad_all(table_obj, 0, 0);
	lv_obj_set_style_border_width(table_obj, 0, 0);
	lv_obj_set_style_border_opa(table_obj, LV_OPA_0, 0);	 
	lv_obj_set_style_bg_color(table_obj, lv_color_hex(DIVOOM_TOMATO_MENU_BACK_GROUD_COLOR), 0); //DIVOOM_MENNU_BACK_GROUD_COLOR  0x1C1C1D
	lv_obj_set_style_bg_opa(table_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	lv_obj_set_style_bg_opa(table_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_shadow_width(table_obj, 0, 0); //去掉边框底部的阴影	

	for (i = 0; i < 5; i++)
	{
        lv_obj_t *btn_obj = lv_btn_create(table_obj);
		if (phandle->first_active_obj == NULL)
		{
			phandle->first_active_obj = btn_obj;
		}
		divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, i);
		lv_obj_add_event_cb(btn_obj, divoom_tomato_time_edit_menu_event_cb, LV_EVENT_KEY, phandle);
//        lv_obj_add_style(btn_obj, &phandle->cont_type, 0);
        lv_obj_set_x(btn_obj, 0);

		if (i == 3 || i == 4)
			lv_obj_set_y(btn_obj, i*98 + 44);
		else
			lv_obj_set_y(btn_obj, i*98);
        lv_obj_set_size(btn_obj, 720, 90);

		lv_obj_set_style_pad_all(btn_obj, 0, 0);
//		lv_obj_set_style_border_width(btn_obj, 0, 0);
		lv_obj_set_style_border_opa(btn_obj, LV_OPA_100, 0);	 
		lv_obj_set_style_bg_color(btn_obj, lv_color_hex(0xF2F2F2), 0);
		lv_obj_set_style_bg_opa(btn_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
		lv_obj_set_style_bg_opa(btn_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
		lv_obj_set_style_shadow_width(btn_obj, 0, 0); //去掉边框底部的阴影	

        lv_obj_add_style(btn_obj, &phandle->btn_type, 0);
        lv_obj_add_style(btn_obj, &phandle->btn_press_type, LV_STATE_FOCUSED);
        lv_obj_add_style(btn_obj, &phandle->btn_press_type, LV_STATE_FOCUS_KEY);

    	lv_obj_t *label = lv_label_create(btn_obj);
		lv_obj_remove_style_all(label);
		lv_obj_set_x(label, 40);
		lv_obj_set_y(label, 24);
		lv_obj_set_width(label, 212);
		lv_obj_set_height(label, 42);
		lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
		lv_obj_set_style_text_color(label, lv_color_hex(DIVOOM_TOMATO_MENU_TEXT_COLOR), 0);
		lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
		lv_label_set_text_fmt(label, "%s", divoom_menu_control_get_disp_str(menu_text_table[i + 1]));
        phandle->btn_table_obj[i] = btn_obj;

		lv_obj_t *arrow_icon_obj = lv_obj_create(btn_obj);
		lv_obj_remove_style_all(arrow_icon_obj);
		lv_obj_set_x(arrow_icon_obj, 650);
		lv_obj_set_y(arrow_icon_obj, 21);
		lv_obj_set_width(arrow_icon_obj, 48);
		lv_obj_set_height(arrow_icon_obj, 48);

		lv_obj_t *img_obj = lv_img_create(arrow_icon_obj); //在容器里面创建图像
		lv_obj_set_align(img_obj, LV_ALIGN_CENTER);
		divoom_menu_control_lv_img_set_src(img_obj, "27/27_arrow_right.png");

		if (i == 0)
		{
			phandle->tomato_work_time_obj = lv_label_create(btn_obj);
			lv_obj_remove_style_all(phandle->tomato_work_time_obj);
			lv_obj_set_x(phandle->tomato_work_time_obj, 434);
			lv_obj_set_y(phandle->tomato_work_time_obj, 24);
			lv_obj_set_width(phandle->tomato_work_time_obj, 212);
			lv_obj_set_height(phandle->tomato_work_time_obj, 42);
			lv_obj_set_style_text_align(phandle->tomato_work_time_obj, LV_TEXT_ALIGN_RIGHT, 0);
			lv_obj_set_style_text_color(phandle->tomato_work_time_obj, lv_color_hex(DIVOOM_TOMATO_MENU_TEXT_COLOR), 0);
			lv_obj_set_style_text_font(phandle->tomato_work_time_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
			lv_label_set_text_fmt(phandle->tomato_work_time_obj, "%d min", phandle->tomato_data->work_time);
		}
        else if(i == 1)
		{
			phandle->tomato_short_time_obj = lv_label_create(btn_obj);
			lv_obj_remove_style_all(phandle->tomato_short_time_obj);
			lv_obj_set_x(phandle->tomato_short_time_obj, 434);
			lv_obj_set_y(phandle->tomato_short_time_obj, 24);
			lv_obj_set_width(phandle->tomato_short_time_obj, 212);
			lv_obj_set_height(phandle->tomato_short_time_obj, 42);
			lv_obj_set_style_text_align(phandle->tomato_short_time_obj, LV_TEXT_ALIGN_RIGHT, 0);
			lv_obj_set_style_text_color(phandle->tomato_short_time_obj, lv_color_hex(DIVOOM_TOMATO_MENU_TEXT_COLOR), 0);
			lv_obj_set_style_text_font(phandle->tomato_short_time_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
			lv_label_set_text_fmt(phandle->tomato_short_time_obj, "%d min", phandle->tomato_data->short_rest_time);
		}
		else if(i == 2)
		{
			phandle->tomato_long_time_obj = lv_label_create(btn_obj);
			lv_obj_remove_style_all(phandle->tomato_long_time_obj);
			lv_obj_set_x(phandle->tomato_long_time_obj, 434);
			lv_obj_set_y(phandle->tomato_long_time_obj, 24);
			lv_obj_set_width(phandle->tomato_long_time_obj, 212);
			lv_obj_set_height(phandle->tomato_long_time_obj, 42);
			lv_obj_set_style_text_align(phandle->tomato_long_time_obj, LV_TEXT_ALIGN_RIGHT, 0);
			lv_obj_set_style_text_color(phandle->tomato_long_time_obj, lv_color_hex(DIVOOM_TOMATO_MENU_TEXT_COLOR), 0);
			lv_obj_set_style_text_font(phandle->tomato_long_time_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
			lv_label_set_text_fmt(phandle->tomato_long_time_obj, "%d min", phandle->tomato_data->long_rest_time);
		}
        else if(i == 3)
		{
			phandle->tomato_repeat_obj = lv_label_create(btn_obj);
			lv_obj_remove_style_all(phandle->tomato_repeat_obj);
			lv_obj_set_x(phandle->tomato_repeat_obj, 585);
			lv_obj_set_y(phandle->tomato_repeat_obj, 24);
			lv_obj_set_width(phandle->tomato_repeat_obj, 61);
			lv_obj_set_height(phandle->tomato_repeat_obj, 42);
			lv_obj_set_style_text_font(phandle->tomato_repeat_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
			lv_obj_set_style_text_color(phandle->tomato_repeat_obj, lv_color_hex(DIVOOM_TOMATO_MENU_TEXT_COLOR), 0);
			lv_obj_set_style_text_align(phandle->tomato_repeat_obj, LV_TEXT_ALIGN_RIGHT, 0);
			lv_label_set_text_fmt(phandle->tomato_repeat_obj, "x%d", phandle->tomato_data->roll_times);

			lv_obj_t *tomato_icon_obj = lv_obj_create(btn_obj);
			lv_obj_remove_style_all(tomato_icon_obj);
			lv_obj_set_x(tomato_icon_obj, 537);
			lv_obj_set_y(tomato_icon_obj, 21);
			lv_obj_set_width(tomato_icon_obj, 48);
			lv_obj_set_height(tomato_icon_obj, 48);

			lv_obj_t *img_obj = lv_img_create(tomato_icon_obj); //在容器里面创建图像
			lv_obj_set_align(img_obj, LV_ALIGN_CENTER);
			divoom_menu_control_lv_img_set_src(img_obj, "27/27_working_small.png");
		}
        else if(i == 4)
		{
			phandle->tomato_bell_obj = lv_label_create(btn_obj);
			lv_obj_remove_style_all(phandle->tomato_bell_obj);
			lv_obj_set_x(phandle->tomato_bell_obj, 434);
			lv_obj_set_y(phandle->tomato_bell_obj, 24);
			lv_obj_set_width(phandle->tomato_bell_obj, 212);
			lv_obj_set_height(phandle->tomato_bell_obj, 42);
			lv_obj_set_style_text_align(phandle->tomato_bell_obj,LV_TEXT_ALIGN_RIGHT, 0);
			lv_obj_set_style_text_font(phandle->tomato_bell_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
			lv_obj_set_style_text_color(phandle->tomato_bell_obj, lv_color_hex(DIVOOM_TOMATO_MENU_TEXT_COLOR), 0);
			lv_label_set_text_fmt(phandle->tomato_bell_obj, "%s %d", divoom_menu_control_get_disp_str("Bells"), phandle->tomato_data->sound_system_index + 1);
		}
	}
}

void *divoom_tomato_time_edit_menu_init(void *user_data, int menu_id)
{
	Pdivoom_tomato_time_edit_menu_ctrl phandle = divoom_system_malloc(sizeof(divoom_tomato_time_edit_menu_ctrl));
    phandle->menu_id = menu_id;
   	phandle->first_active_obj = NULL;
    phandle->pdirect_handle = divoom_menu_direct_ctrl_create(1, 5);  
	phandle->tomato_data = (Pdivoom_tomato_time_info)user_data;
    phandle->main_obj = lv_obj_create(NULL);
    lv_style_init(&phandle->backgroud_type);
    lv_style_set_bg_color(&phandle->backgroud_type, lv_color_hex(DIVOOM_TOMATO_MENU_BACK_GROUD_COLOR));
    lv_style_set_text_color(&phandle->backgroud_type, lv_color_hex(DIVOOM_TOMATO_MENU_TEXT_COLOR));
    lv_obj_add_style(phandle->main_obj, &phandle->backgroud_type, 0);
    lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    divoom_tomato_time_edit_menu_create_image_grad(phandle);
    return phandle;
}

void divoom_tomato_time_edit_menu_disp(void *handle, void *active)
{
	Pdivoom_tomato_time_edit_menu_ctrl phandle = (Pdivoom_tomato_time_edit_menu_ctrl)handle;

    lv_scr_load(phandle->main_obj);
	
	if(phandle->first_active_obj)
    {
        int select_id;
        lv_group_focus_obj(phandle->first_active_obj);
        phandle->first_active_obj = NULL;
    }
    else
    {
        //配置发生变更，将刷新相关元素
        lv_group_focus_obj(active);
		lv_label_set_text_fmt(phandle->tomato_work_time_obj, "%d min", phandle->tomato_data->work_time);
		lv_label_set_text_fmt(phandle->tomato_short_time_obj, "%d min", phandle->tomato_data->short_rest_time);
		lv_label_set_text_fmt(phandle->tomato_long_time_obj, "%d min", phandle->tomato_data->long_rest_time);
		lv_label_set_text_fmt(phandle->tomato_repeat_obj, "x%d", phandle->tomato_data->roll_times);
		lv_label_set_text_fmt(phandle->tomato_bell_obj, "%s %d", divoom_menu_control_get_disp_str("Bells"), phandle->tomato_data->sound_system_index + 1);
    }
}

void divoom_tomato_time_edit_menu_clean(void *handle)
{
	Pdivoom_tomato_time_edit_menu_ctrl phandle = (Pdivoom_tomato_time_edit_menu_ctrl)handle;
    lv_obj_del(phandle->main_obj);

	lv_style_reset(&phandle->backgroud_type);
	lv_style_reset(&phandle->cont_type);
	lv_style_reset(&phandle->btn_type);
	lv_style_reset(&phandle->btn_press_type);
	lv_style_reset(&phandle->style_line);
    divoom_menu_direct_ctrl_destory(phandle->pdirect_handle);
    divoom_system_free(phandle);
}


#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_menu_direct_ctrl.h"
#include "divoom_tomato_time_menu.h"

typedef struct _divoom_tomato_time_edit_repeat_menu_ctrl
{
    int menu_id;
	lv_style_t cont_type;
	lv_style_t backgroud_type;
	lv_style_t btn_type;
	lv_style_t btn_press_type;
	lv_obj_t *repeat_img[5];
	void *main_obj;
	Pdivoom_tomato_time_info tomato_data;

	lv_obj_t *arrow_btn_obj;
	lv_obj_t *label_time_obj;
	lv_timer_t *tomato_refresh_time;
	lv_style_t style_line;
	lv_obj_t *btn_table_obj[5];
	void *first_active_obj;
	void *pdirect_handle;
}divoom_tomato_time_edit_repeat_menu_ctrl, *Pdivoom_tomato_time_edit_repeat_menu_ctrl;

static char menu_text_table[][20] = {
	"RepeateType",
};

static void divoom_tomato_time_edit_repeat_menu_event_cb(lv_event_t * e)
{
	int active_id, value, i;
	lv_obj_t *cur_obj;
	Pdivoom_tomato_time_edit_repeat_menu_ctrl phandle = (Pdivoom_tomato_time_edit_repeat_menu_ctrl)lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);
	cur_obj = lv_event_get_target_obj(e);

	if (code == LV_EVENT_KEY)
	{
		char c = *((char *)lv_event_get_param(e));
        int cur_pos = divoom_menu_direct_ctrl_get_pos(phandle->pdirect_handle, cur_obj);

		if (c == DIVOOM_LVGL_KEY_PRE)
		{
			lv_group_focus_obj(divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_obj, DIVOOM_MENU_DIRECT_UP));
		}
		else if (c == DIVOOM_LVGL_KEY_NEXT)
		{
			lv_group_focus_obj(divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_obj, DIVOOM_MENU_DIRECT_DOWN));
		}
		else if (c == DIVOOM_LVGL_KEY_ENTER)
		{
			phandle->tomato_data->roll_times = 4 - cur_pos;
			phandle->tomato_data->update_flag = 1;

			for (i = 0; i < 5; i++)
			{
				if (i == cur_pos)
				{
					lv_obj_set_style_opa(phandle->repeat_img[i], LV_OPA_100, 0);
				}
				else
				{
					lv_obj_set_style_opa(phandle->repeat_img[i], LV_OPA_0, 0);
				}
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

void divoom_tomato_time_edit_repeat_menu_task(struct _lv_timer_t * tomato_refresh_time)
{
    Pdivoom_tomato_time_edit_repeat_menu_ctrl phandle = (Pdivoom_tomato_time_edit_repeat_menu_ctrl)lv_timer_get_user_data(tomato_refresh_time);
	divoom_tomato_time_menu_display_time_label(phandle->label_time_obj);
}

void divoom_tomato_time_edit_repeat_menu_create_image_grad(Pdivoom_tomato_time_edit_repeat_menu_ctrl phandle)
{
	//文本框
	lv_obj_t *label_obj = lv_label_create(phandle->main_obj);
	lv_obj_remove_style_all(label_obj);
	lv_obj_set_x(label_obj, 0);
	lv_obj_set_y(label_obj, 61);
	lv_obj_set_size(label_obj, DIVOOM_SCREEN_WIDTH, 56);

	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_color(label_obj, lv_color_hex(DIVOOM_TOMATO_MENU_TEXT_COLOR), 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_label_set_text_fmt(label_obj, "%s", divoom_menu_control_get_disp_str(menu_text_table[0]));

	//列表
	lv_obj_t *table_obj = lv_obj_create(phandle->main_obj);
	lv_obj_set_x(table_obj, 44);
	lv_obj_set_y(table_obj, 164);
	lv_obj_set_width(table_obj, 720);
	lv_obj_set_height(table_obj, 660);
	lv_obj_set_style_radius(table_obj, 18, 0);
	lv_obj_set_style_pad_all(table_obj, 0, 0);
	lv_obj_set_style_border_width(table_obj, 1, 0);
	lv_obj_set_style_border_opa(table_obj, LV_OPA_0, 0);	 
	lv_obj_set_style_bg_color(table_obj, lv_color_hex(DIVOOM_TOMATO_MENU_BACK_GROUD_COLOR), 0);// 0x1C1C1D
	lv_obj_set_style_bg_opa(table_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	lv_obj_set_style_bg_opa(table_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_shadow_width(table_obj, 0, 0); //去掉边框底部的阴影	

	lv_style_init(&phandle->cont_type);
	lv_style_set_pad_all(&phandle->cont_type, 0);
	lv_style_set_pad_gap(&phandle->cont_type, 1);
	lv_style_set_border_width(&phandle->cont_type, 0);
	lv_style_set_outline_width(&phandle->cont_type, 0);
	lv_style_set_border_side(&phandle->cont_type, LV_BORDER_SIDE_NONE);
	lv_style_set_bg_color(&phandle->cont_type, lv_color_hex(DIVOOM_TOMATO_MENU_BACK_GROUD_COLOR));
//    divoom_menu_control_init_btn_stype(&phandle->btn_type, NULL, NULL, NULL, &phandle->btn_press_type,NULL, 19);

	lv_style_init(&phandle->btn_type);
	lv_style_set_bg_color(&phandle->btn_type, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->btn_type, 0);
	lv_style_set_border_width(&phandle->btn_type, 4);
	lv_style_set_border_color(&phandle->btn_type, lv_color_hex(0xF2F2F2));
	lv_style_set_radius(&phandle->btn_type, 17);
	lv_style_set_shadow_width(&phandle->btn_type, 0);
	lv_style_set_outline_width(&phandle->btn_type, 0);

	lv_style_init(&phandle->btn_press_type);
	lv_style_set_bg_color(&phandle->btn_press_type, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->btn_press_type, 0);
	lv_style_set_border_width(&phandle->btn_press_type, 4);
	lv_style_set_border_color(&phandle->btn_press_type, lv_color_hex(0x6D6D6D));
	lv_style_set_radius(&phandle->btn_press_type, 17);
	lv_style_set_shadow_width(&phandle->btn_press_type, 0);
	lv_style_set_outline_width(&phandle->btn_press_type, 0);

	for (int i = 0; i < 5; i++)
	{
        lv_obj_t *btn_obj = lv_btn_create(table_obj);
		if(phandle->first_active_obj == NULL && phandle->tomato_data->roll_times == 4 - i)
		{
		   phandle->first_active_obj = btn_obj;
		}
		divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, i);

		lv_obj_add_event_cb(btn_obj, divoom_tomato_time_edit_repeat_menu_event_cb, LV_EVENT_KEY, phandle);
//        lv_obj_add_style(btn_obj, &phandle->cont_type, 0);
        lv_obj_set_x(btn_obj, 0);
		lv_obj_set_y(btn_obj, i*98);
        lv_obj_set_size(btn_obj, 712, 90);
        lv_obj_add_style(btn_obj, &phandle->btn_type, 0);
		lv_obj_add_style(btn_obj, &phandle->btn_press_type, LV_STATE_FOCUSED);
		lv_obj_add_style(btn_obj, &phandle->btn_press_type, LV_STATE_FOCUS_KEY);
        phandle->btn_table_obj[i] = btn_obj;

		phandle->repeat_img[i] = lv_img_create(btn_obj);
		lv_obj_set_pos(phandle->repeat_img[i], 628, 19);
		lv_obj_set_size(phandle->repeat_img[i], 52, 52);
		divoom_menu_control_lv_img_set_src(phandle->repeat_img[i], "27/27_ok.png");
		lv_obj_set_style_opa(phandle->repeat_img[i], LV_OPA_0, 0);
		if (phandle->tomato_data->roll_times == 4 - i)
		{
			lv_obj_set_style_opa(phandle->repeat_img[i], LV_OPA_100, 0);
		}
		
		for (int j = 0; j < 8 - (i*2); j++)
		{
			lv_obj_t *img_cont = lv_obj_create(btn_obj); //创建图像的容器
			lv_obj_remove_style_all(img_cont);
			lv_obj_set_x(img_cont, 40 + j*68);
			lv_obj_set_y(img_cont, 19);
			lv_obj_set_width(img_cont, 52);
			lv_obj_set_height(img_cont, 52);

			lv_obj_t *img_obj = lv_img_create(img_cont); //在容器里面创建图像
			lv_obj_set_align(img_obj, LV_ALIGN_CENTER);

			if (j%2 == 0)
			{
				divoom_menu_control_lv_img_set_src(img_obj, "27/27_working2.png");
			}
			else
			{
				if (j == 8 - (i*2) - 1)
					divoom_menu_control_lv_img_set_src(img_obj, "27/27_long_reseting2.png");
				else
					divoom_menu_control_lv_img_set_src(img_obj, "27/27_reseting2.png");
			}
		}

		if (i == 4)
		{
			lv_obj_t *img_cont = lv_obj_create(btn_obj); //创建图像的容器
			lv_obj_remove_style_all(img_cont);
			lv_obj_set_x(img_cont, 40);
			lv_obj_set_y(img_cont, 19);
			lv_obj_set_width(img_cont, 52);
			lv_obj_set_height(img_cont, 52);

			lv_obj_t *img_obj = lv_img_create(img_cont); //在容器里面创建图像
			lv_obj_set_align(img_obj, LV_ALIGN_CENTER);
			divoom_menu_control_lv_img_set_src(img_obj, "27/27_working2.png");
		}
	}
}

void *divoom_tomato_time_edit_repeat_menu_init(void *user_data, int menu_id)
{
	Pdivoom_tomato_time_edit_repeat_menu_ctrl phandle = divoom_system_malloc(sizeof(divoom_tomato_time_edit_repeat_menu_ctrl));
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
    divoom_tomato_time_edit_repeat_menu_create_image_grad(phandle);
    return phandle;
}

void divoom_tomato_time_edit_repeat_menu_disp(void *handle, void *active)
{
	Pdivoom_tomato_time_edit_repeat_menu_ctrl phandle = (Pdivoom_tomato_time_edit_repeat_menu_ctrl)handle;

    lv_scr_load(phandle->main_obj);
	if(phandle->first_active_obj)
	{
	  lv_group_focus_obj(phandle->first_active_obj);
	  phandle->first_active_obj = NULL;
	}
	else
	{
	  //配置发生变更，将刷新相关元素
	  lv_group_focus_obj(active);
	}
}

void divoom_tomato_time_edit_repeat_menu_clean(void *handle)
{
	Pdivoom_tomato_time_edit_repeat_menu_ctrl phandle = (Pdivoom_tomato_time_edit_repeat_menu_ctrl)handle;

	if (phandle->tomato_refresh_time)
	{
		lv_timer_del(phandle->tomato_refresh_time);
	}

    lv_obj_del(phandle->main_obj);
	lv_style_reset(&phandle->cont_type);
	lv_style_reset(&phandle->btn_type);
	lv_style_reset(&phandle->btn_press_type);
	lv_style_reset(&phandle->backgroud_type);
	lv_style_reset(&phandle->style_line);
    divoom_menu_direct_ctrl_destory(phandle->pdirect_handle);
    divoom_system_free(phandle);
}

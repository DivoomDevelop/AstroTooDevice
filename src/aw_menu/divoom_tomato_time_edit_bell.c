#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_queue.h"
#include "divoom_menu_direct_ctrl.h"
#include "divoom_tomato_time_edit_bell_menu.h"
#include "divoom_tomato_time_menu.h"

typedef struct _divoom_tomato_time_edit_bell_menu_ctrl
{
    int menu_id;
    lv_coord_t col_dsc[5];
    lv_coord_t row_dsc[7];
    lv_obj_t *main_obj;
    lv_obj_t *cur_active_obj;
    lv_style_t backgroud_type;
    lv_style_t grid_type;
    lv_style_t cont_type;
    lv_style_t btn_type;
    lv_style_t btn_press_type;
    Pdivoom_tomato_time_info tomato_time_info;

	lv_obj_t *arrow_btn_obj;
	lv_obj_t *label_time_obj;
	lv_timer_t *tomato_refresh_time;
	lv_obj_t *table_btn_obj[24];
	void *pdirect_handle;
}divoom_tomato_time_edit_bell_menu_ctrl, *Pdivoom_tomato_time_edit_bell_menu_ctrl;

static const char *divoom_tomato_time_edit_bell_menu_img_list[] = {
	"39/39_1.png",
	"39/39_2.png",
	"39/39_3.png",
	"39/39_4.png",
	"39/39_5.png",
	"39/39_6.png",
	"39/39_7.png",
	"39/39_8.png",
	"39/39_9.png",
	"39/39_10.png",
	"39/39_11.png",
	"39/39_12.png",
	"39/39_13.png",
	"39/39_14.png",
	"39/39_15.png",
	"39/39_16.png",
	"39/39_17.png",
	"39/39_18.png",
	"39/39_19.png",
	"39/39_20.png",
	"39/39_21.png",
	"39/39_22.png",
};

static char menu_text_table[][20] = {
	"Bells",
};

static void divoom_tomato_time_edit_bell_menu_event_cb(lv_event_t * e)
{
    int trigger_mode, i, j;
    lv_obj_t *cur_obj;
    Pdivoom_tomato_time_edit_bell_menu_ctrl phandle = (Pdivoom_tomato_time_edit_bell_menu_ctrl)lv_event_get_user_data(e);
 	Pdivoom_tomato_time_info tomato_time_info = phandle->tomato_time_info;
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target(e);

	if(code == LV_EVENT_KEY)
    {
        char c = *((char *)lv_event_get_param(e));

        if(c == DIVOOM_LVGL_KEY_NEXT) 
		{
			cur_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_obj, DIVOOM_MENU_DIRECT_RIGHT);
            lv_group_focus_obj(cur_obj);
            trigger_mode = divoom_menu_direct_ctrl_get_pos(phandle->pdirect_handle, cur_obj);
            divoom_tomato_time_try_listen(trigger_mode, tomato_time_info->sound_volume, 1, 1);
        }
        else if(c == DIVOOM_LVGL_KEY_PRE) 
		{
            cur_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_obj, DIVOOM_MENU_DIRECT_LEFT);
            lv_group_focus_obj(cur_obj);
            trigger_mode = divoom_menu_direct_ctrl_get_pos(phandle->pdirect_handle, cur_obj);
            divoom_tomato_time_try_listen(trigger_mode, tomato_time_info->sound_volume, 1, 1);
        }
        else if(c == DIVOOM_LVGL_KEY_ENTER)
        {
			tomato_time_info->sound_system_index = divoom_menu_direct_ctrl_get_pos(phandle->pdirect_handle, cur_obj);
       		tomato_time_info->sound_type = 0;
			tomato_time_info->update_flag = 1;
			divoom_tomato_time_try_listen(tomato_time_info->sound_system_index, tomato_time_info->sound_volume, 0, 1);
            divoom_menu_control_return_pre();
        }
        else if(c == DIVOOM_LVGL_KEY_ESC)
        {
            //将退出所有菜单
			trigger_mode = divoom_menu_direct_ctrl_get_pos(phandle->pdirect_handle, cur_obj);
			divoom_tomato_time_try_listen(trigger_mode, tomato_time_info->sound_volume, 0, 1);
            divoom_menu_control_return_pre();
        }
        else 
		{
			char *key_info = (char *)lv_event_get_param(e);
			divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
        }
    }
}

void divoom_tomato_time_edit_bell_menu_task(struct _lv_timer_t * tomato_refresh_time)
{
    Pdivoom_tomato_time_edit_bell_menu_ctrl phandle = (Pdivoom_tomato_time_edit_bell_menu_ctrl)lv_timer_get_user_data(tomato_refresh_time);
	divoom_tomato_time_menu_display_time_label(phandle->label_time_obj);
}

void divoom_tomato_time_edit_bell_menu_create_image_grad(Pdivoom_tomato_time_edit_bell_menu_ctrl phandle)
{
    int i, j;
    Pdivoom_tomato_time_info tomato_time_info = (Pdivoom_tomato_time_info)phandle->tomato_time_info;

    lv_style_init(&phandle->grid_type);
    lv_style_set_pad_all(&phandle->grid_type, 0);
    lv_style_set_pad_gap(&phandle->grid_type, 0);
    lv_style_set_border_width(&phandle->grid_type, 0);
    lv_style_set_border_side(&phandle->grid_type, LV_BORDER_SIDE_NONE);
    lv_style_set_bg_color(&phandle->grid_type, lv_color_hex(DIVOOM_TOMATO_MENU_BACK_GROUD_COLOR));

	//文本框
	lv_obj_t *label_obj = lv_label_create(phandle->main_obj);
	lv_obj_remove_style_all(label_obj);
	lv_obj_set_x(label_obj, 367);
	lv_obj_set_y(label_obj, 61);
	lv_obj_set_style_text_color(label_obj, lv_color_hex(DIVOOM_TOMATO_MENU_TEXT_COLOR), 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(48, phandle->menu_id), 0);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
	lv_label_set_text(label_obj, divoom_menu_control_get_disp_str(menu_text_table[0]));

    lv_style_init(&phandle->cont_type);

    lv_style_set_pad_all(&phandle->cont_type, 0);
    lv_style_set_pad_gap(&phandle->cont_type, 0);
    lv_style_set_border_width(&phandle->cont_type, 0);
    lv_style_set_outline_width(&phandle->cont_type, 0);
    lv_style_set_border_side(&phandle->cont_type, LV_BORDER_SIDE_NONE);

    lv_style_set_bg_color(&phandle->cont_type, lv_color_hex(DIVOOM_TOMATO_MENU_BACK_GROUD_COLOR));

    lv_obj_t * cont = lv_obj_create(phandle->main_obj);
    lv_obj_add_style(cont, &phandle->grid_type, 0);
    lv_obj_set_size(cont, DIVOOM_SCREEN_WIDTH - 70, DIVOOM_SCREEN_HIGH - 247 + 110);
    lv_obj_set_pos(cont, 40, 158);
	lv_obj_set_style_pad_all(cont, 0, 0);
	lv_obj_set_style_border_width(cont, 0, 0);
	lv_obj_set_style_border_opa(cont, LV_OPA_0, 0);	
	lv_obj_set_style_bg_opa(cont, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cont, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_shadow_width(cont, 0, 0); //去掉边框底部的阴影

    divoom_menu_control_init_btn_stype(&phandle->btn_type, NULL, NULL, NULL, &phandle->btn_press_type, NULL, 19);
    lv_obj_set_style_base_dir(cont, LV_BASE_DIR_AUTO, 0);
    for(i = 0; i < 4; i ++)
    {
        phandle->col_dsc[i] = 134 + 48;
    }
    phandle->col_dsc[4] = LV_GRID_TEMPLATE_LAST;
    for(i = 0; i < 6; i ++)
    {
        phandle->row_dsc[i] = 134 + 56;
    }
    phandle->row_dsc[6] = LV_GRID_TEMPLATE_LAST;
    lv_obj_set_grid_dsc_array(cont, phandle->col_dsc, phandle->row_dsc);

    for(i = 0; i < 6; i++)
    {
        for(j = 0; j < 4; j ++)
        {
			if (i == 5 && j > 1)
				break;

            lv_obj_t * obj = lv_btn_create(cont);
            lv_obj_add_style(obj, &phandle->btn_type, 0);
            lv_obj_add_style(obj, &phandle->btn_press_type, LV_STATE_FOCUSED);
            lv_obj_add_style(obj, &phandle->btn_press_type, LV_STATE_FOCUS_KEY);
            lv_obj_add_style(obj, &phandle->cont_type, 0);
            lv_obj_set_size(obj, 134, 134);
            lv_obj_center(obj);
            lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_STRETCH, j, 1,
                                 LV_GRID_ALIGN_STRETCH, i, 1);
            divoom_menu_direct_ctrl_update(phandle->pdirect_handle, obj, j, i);
			phandle->table_btn_obj[i*4 + j] = obj;
			if (phandle->cur_active_obj == NULL)
				phandle->cur_active_obj = obj;

            lv_obj_t *img_obj = lv_img_create(obj);
            divoom_menu_control_lv_img_set_src(img_obj, divoom_tomato_time_edit_bell_menu_img_list[i*4+j]);
            lv_obj_center(img_obj);
            lv_obj_add_event_cb(obj, divoom_tomato_time_edit_bell_menu_event_cb, LV_EVENT_KEY, phandle);
        }
    }
}

void *divoom_tomato_time_edit_bell_menu_init(void *user_data, int menu_id)
{
    Pdivoom_tomato_time_edit_bell_menu_ctrl phandle = divoom_system_malloc(sizeof(divoom_tomato_time_edit_bell_menu_ctrl));
    phandle->menu_id = menu_id;
	phandle->cur_active_obj = NULL;
	phandle->pdirect_handle = divoom_menu_direct_ctrl_create(4, 6);
    phandle->main_obj = lv_obj_create(NULL);
	phandle->tomato_time_info = (Pdivoom_tomato_time_info)user_data;

    lv_style_init(&phandle->backgroud_type);
    lv_style_set_bg_color(&phandle->backgroud_type, lv_color_hex(DIVOOM_TOMATO_MENU_BACK_GROUD_COLOR));
    lv_style_set_text_color(&phandle->backgroud_type, lv_color_hex(DIVOOM_TOMATO_MENU_TEXT_COLOR));
    lv_obj_add_style(phandle->main_obj, &phandle->backgroud_type, 0);
    lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    divoom_tomato_time_edit_bell_menu_create_image_grad(phandle);
    return phandle;
}

void divoom_tomato_time_edit_bell_menu_disp(void *handle, void *active)
{
	Pdivoom_tomato_time_edit_bell_menu_ctrl phandle = (Pdivoom_tomato_time_edit_bell_menu_ctrl)handle;

	lv_scr_load(phandle->main_obj);
	if (phandle->cur_active_obj)
	{
		lv_group_focus_obj(phandle->cur_active_obj);
		phandle->cur_active_obj = NULL;
	}
}

void divoom_tomato_time_edit_bell_menu_exit(void *handle)
{
    Pdivoom_tomato_time_edit_bell_menu_ctrl phandle = (Pdivoom_tomato_time_edit_bell_menu_ctrl)handle;

	if (phandle->tomato_refresh_time)
		lv_timer_del(phandle->tomato_refresh_time);

    lv_obj_del(phandle->main_obj);
	lv_style_reset(&phandle->backgroud_type);
	lv_style_reset(&phandle->cont_type);
	lv_style_reset(&phandle->grid_type);
	lv_style_reset(&phandle->btn_type);
	lv_style_reset(&phandle->btn_press_type);
    divoom_menu_direct_ctrl_destory(phandle->pdirect_handle);
    divoom_system_free(phandle);
}


#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_queue.h"
#include "divoom_white_noise_menu.h"
#include "divoom_white_noise_time_menu.h"

typedef struct _divoom_white_noise_time_menu_ctrl
{
    int menu_id;
    int active_id;
    lv_coord_t col_dsc[2];
    lv_coord_t row_dsc[20];
    lv_style_t cont_type;
    lv_style_t grid_type;
    lv_style_t backgroud_type;
    lv_style_t btn_type;
    lv_style_t btn_press_type;
    void *first_active_obj;
    void *main_obj;
    void *image_select_tbl[20];
    divoom_white_noise_info *base_info;

	lv_obj_t *arrow_btn_obj;
	lv_obj_t *btn[10];
	lv_obj_t *img[10];
	lv_style_t style_line;
}divoom_white_noise_time_menu_ctrl, *Pdivoom_white_noise_time_menu_ctrl;

static const int time_second_table_value[] = {
    600,
    900,
    1800,
    2700,
    3600,
    5400,};

static char menu_text_table[][20] = {
	"HoldingTime",
	"AutoPowerOff",
};

static divoom_white_noise_info base_info = {1, 1, 600, 1, 0,{0}};

static void divoom_white_noise_time_menu_switch_event_cb(lv_event_t * e)
{
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target(e));
    Pdivoom_white_noise_time_menu_ctrl phandle = (Pdivoom_white_noise_time_menu_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target(e);

	switch (code)
	{
		case LV_EVENT_CLICKED:
			if (cur_obj == phandle->arrow_btn_obj)
			{
				//退出
				divoom_menu_control_return_pre();
			}
			else
			{
				if (cur_obj == phandle->btn[0])
				{
					if(phandle->base_info->power_off == 0)
	                {
	                    phandle->base_info->power_off = 1;
						phandle->base_info->idle_time = 0;
	                    divoom_menu_control_lv_img_set_src(phandle->img[0], "50/50_switch_off.png");
	                }
                	else
	                {
	                    phandle->base_info->power_off = 0;
						phandle->base_info->idle_time = 0;
	                    divoom_menu_control_lv_img_set_src(phandle->img[0], "50/50_switch_on.png");
	                }
					break;
				}

				for (int i = 1; i < 7; i++)
				{
					if (cur_obj == phandle->btn[i])
					{
						lv_obj_clear_flag(phandle->img[i], LV_OBJ_FLAG_HIDDEN);
					}
					else
					{
						lv_obj_add_flag(phandle->img[i], LV_OBJ_FLAG_HIDDEN);
					}
				}
			}
			
			break;
        default:
            break;
	}

}

void divoom_white_noise_time_menu_create_image_grad(Pdivoom_white_noise_time_menu_ctrl phandle)
{
    int i, j, menu_items;

//    divoom_menu_control_init_btn_stype(&phandle->btn_type, &phandle->btn_press_type);

	//返回箭头
	phandle->arrow_btn_obj = divoom_white_noise_menu_set_return_arrow_obj(phandle->main_obj);
	lv_obj_add_event_cb(phandle->arrow_btn_obj, divoom_white_noise_time_menu_switch_event_cb, LV_EVENT_CLICKED, phandle);	

	//文本框
	lv_obj_t *label_obj = lv_label_create(phandle->main_obj);
//	lv_obj_remove_style_all(label_obj);
	lv_obj_set_x(label_obj, 0);
	lv_obj_set_y(label_obj, 66);
	lv_obj_set_size(label_obj, 800, 78);
	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(48, phandle->menu_id), 0);
	lv_label_set_text_fmt(label_obj, "%s", divoom_menu_control_get_disp_str(menu_text_table[0]));

	lv_obj_t *btn_obj = lv_obj_create(phandle->main_obj);
	lv_obj_set_x(btn_obj, 70);
	lv_obj_set_y(btn_obj, 209);
	lv_obj_set_width(btn_obj, 661);
	lv_obj_set_height(btn_obj, 125);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_style_border_width(btn_obj, 1, 0);
	lv_obj_set_style_border_opa(btn_obj, LV_OPA_0, 0);
	lv_obj_set_style_radius(btn_obj, 28, 0);
	lv_obj_set_style_bg_color(btn_obj, lv_color_hex(0x222223), 0);
	lv_obj_set_style_bg_opa(btn_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	lv_obj_set_style_bg_opa(btn_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_shadow_width(btn_obj, 0, 0); //去掉边框底部的阴影

	//文本框
	lv_obj_t *label_obj2 = lv_label_create(btn_obj);
//	lv_obj_remove_style_all(label_obj);
	lv_obj_set_x(label_obj2, 35);
	lv_obj_set_y(label_obj2, 25);
	lv_obj_set_width(label_obj2, 500);
	lv_obj_set_height(label_obj2, 64);
	lv_obj_set_style_text_color(label_obj2, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_align(label_obj2, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_font(label_obj2, divoom_menu_control_get_font_info(50, phandle->menu_id), 0);
	lv_label_set_text(label_obj2, divoom_menu_control_get_disp_str(menu_text_table[1]));

	i = 0;
	phandle->btn[i] = lv_btn_create(btn_obj);
	lv_obj_set_x(phandle->btn[i], 524);
	lv_obj_set_y(phandle->btn[i], 32 - 10);
	lv_obj_set_width(phandle->btn[i], 126);
	lv_obj_set_height(phandle->btn[i], 87);
	lv_obj_set_style_border_width(phandle->btn[i], 1, 0);
	lv_obj_set_style_border_opa(phandle->btn[i], LV_OPA_0, 0);
	lv_obj_set_style_bg_opa(phandle->btn[i], LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	lv_obj_set_style_bg_opa(phandle->btn[i], LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_shadow_width(phandle->btn[i], 0, 0); //去掉边框底部的阴影
	lv_obj_set_style_bg_color(phandle->btn[i], lv_color_hex(0x222223), 0);

	phandle->img[i] = lv_img_create(phandle->btn[i]);
	lv_obj_set_align(phandle->img[i], LV_ALIGN_CENTER);
	int on_off = phandle->base_info->on_off;
	if(phandle->base_info->power_off == 0)
	{
		divoom_menu_control_lv_img_set_src(phandle->img[i], "50/50_switch_on.png");
	}
	else
	{
		divoom_menu_control_lv_img_set_src(phandle->img[i], "50/50_switch_off.png");
	}
	lv_obj_add_event_cb(phandle->btn[i], divoom_white_noise_time_menu_switch_event_cb, LV_EVENT_CLICKED, phandle);

	//列表
	lv_obj_t *table_obj = lv_obj_create(phandle->main_obj);
	lv_obj_set_x(table_obj, 70);
	lv_obj_set_y(table_obj, 355);
	lv_obj_set_width(table_obj, 661);
	lv_obj_set_height(table_obj, 1030);
	lv_obj_set_style_radius(table_obj, 28, 0);
	lv_obj_set_style_pad_all(table_obj, 0, 0);
	lv_obj_set_style_border_width(table_obj, 0, 0);
	lv_obj_set_style_border_opa(table_obj, LV_OPA_0, 0);	 
	lv_obj_set_style_bg_color(table_obj, lv_color_hex(0x222223), 0);
	lv_obj_set_style_bg_opa(table_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	lv_obj_set_style_bg_opa(table_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_shadow_width(table_obj, 0, 0); //去掉边框底部的阴影	
	for (i = 0; i < 6; i++)
	{
        lv_obj_t *btn_obj = lv_btn_create(table_obj);
		lv_obj_add_event_cb(btn_obj, divoom_white_noise_time_menu_switch_event_cb, LV_EVENT_CLICKED, phandle);
//      lv_obj_add_style(btn_obj, &phandle->cont_type, 0);
        lv_obj_set_x(btn_obj, 0);
		lv_obj_set_y(btn_obj, i*171);
        lv_obj_set_size(btn_obj, 661, 171);

		lv_obj_set_style_pad_all(btn_obj, 0, 0);
		lv_obj_set_style_border_width(btn_obj, 0, 0);
		lv_obj_set_style_border_opa(btn_obj, LV_OPA_0, 0);	 
		lv_obj_set_style_bg_color(btn_obj, lv_color_hex(0x222223), 0);
		lv_obj_set_style_bg_opa(btn_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
		lv_obj_set_style_bg_opa(btn_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
		lv_obj_set_style_shadow_width(btn_obj, 0, 0); //去掉边框底部的阴影	

        lv_obj_add_style(btn_obj, &phandle->btn_type, 0);
        lv_obj_add_style(btn_obj, &phandle->btn_press_type, LV_STATE_CHECKED);

    	lv_obj_t *label = lv_label_create(btn_obj);
		lv_obj_remove_style_all(label);
		lv_obj_set_x(label, 34);
		lv_obj_set_y(label, 48);
		lv_obj_set_width(label, 212);
		lv_obj_set_height(label, 76);
		lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
		lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
		lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(52, phandle->menu_id), 0);
		lv_label_set_text_fmt(label, "%d min", time_second_table_value[i]/60);
        phandle->btn[i + 1] = btn_obj;

		lv_obj_t *arrow_icon_obj = lv_obj_create(btn_obj);
		lv_obj_remove_style_all(arrow_icon_obj);
		lv_obj_set_x(arrow_icon_obj, 573);
		lv_obj_set_y(arrow_icon_obj, 62);
		lv_obj_set_width(arrow_icon_obj, 48);
		lv_obj_set_height(arrow_icon_obj, 48);

		phandle->img[i + 1] = lv_img_create(arrow_icon_obj); //在容器里面创建图像
		lv_obj_set_align(phandle->img[i + 1], LV_ALIGN_CENTER);
		divoom_menu_control_lv_img_set_src(phandle->img[i + 1], "50/50_tick.png");

		if(time_second_table_value[i] != phandle->base_info->time)
        {
            lv_obj_add_flag(phandle->img[i + 1], LV_OBJ_FLAG_HIDDEN);
        }
	}

	for (i = 1; i < 6; i++)
	{
		//画直线
		static lv_point_t line_points[5][2];

		line_points[i-1][0].x = 0;
		line_points[i-1][0].y = i*171;
		line_points[i-1][1].x = 661;
		line_points[i-1][1].y = i*171;

	    lv_style_init(&phandle->style_line);
	    lv_style_set_line_width(&phandle->style_line, 2);
	    lv_style_set_line_color(&phandle->style_line, lv_color_hex(0x000000));
	    lv_style_set_line_rounded(&phandle->style_line, true);

	    lv_obj_t * line = lv_line_create(table_obj);
	    lv_line_set_points(line, line_points[i-1], 2);
	    lv_obj_add_style(line, &phandle->style_line, 0);
	}
}


void *divoom_white_noise_time_menu_init(void *user_data, int menu_id)
{
    Pdivoom_white_noise_time_menu_ctrl phandle = divoom_system_malloc(sizeof(divoom_white_noise_time_menu_ctrl));
    phandle->menu_id = menu_id;
    phandle->first_active_obj = NULL;
    phandle->base_info = &base_info;//(divoom_white_noise_info *)divoom_config_get_white_noise_value();
    phandle->main_obj = lv_obj_create(NULL);
    lv_style_init(&phandle->backgroud_type);
    lv_style_set_bg_color(&phandle->backgroud_type, lv_color_hex(DIVOOM_MENNU_BACK_GROUD_COLOR));
    lv_style_set_text_color(&phandle->backgroud_type, lv_color_hex(DIVOOM_MENNU_TEXT_COLOR));
    lv_obj_add_style(phandle->main_obj, &phandle->backgroud_type, 0);
    lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    divoom_white_noise_time_menu_create_image_grad(phandle);
    return phandle;
}

void *divoom_white_noise_time_menu_get_active(void *handle)
{
    Pdivoom_white_noise_time_menu_ctrl phandle = (Pdivoom_white_noise_time_menu_ctrl)handle;

    lv_group_t * cur_group_obj = lv_obj_get_group(phandle->main_obj);

    return lv_group_get_focused(cur_group_obj);
}

void divoom_white_noise_time_menu_disp(void *handle, void *active)
{
    Pdivoom_white_noise_time_menu_ctrl phandle = (Pdivoom_white_noise_time_menu_ctrl)handle;

    lv_scr_load(phandle->main_obj);
    if(phandle->first_active_obj)
    {
        lv_group_focus_obj(phandle->first_active_obj);
        phandle->first_active_obj = NULL;
    }
    else if(active)
    {
        lv_group_focus_obj(active);
    }
}
void divoom_white_noise_time_menu_exit(void *handle)
{
    Pdivoom_white_noise_time_menu_ctrl phandle = (Pdivoom_white_noise_time_menu_ctrl)handle;

    lv_obj_del(phandle->main_obj);
    
	lv_style_reset(&phandle->cont_type);
	lv_style_reset(&phandle->grid_type);
	lv_style_reset(&phandle->backgroud_type);
	lv_style_reset(&phandle->btn_type);
	lv_style_reset(&phandle->btn_press_type);
    divoom_system_free(phandle);
}


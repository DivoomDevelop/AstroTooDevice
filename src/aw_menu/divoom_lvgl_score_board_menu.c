#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"

typedef struct _divoom_lvgl_score_board_ctrl
{
    int menu_id;
	lv_style_t backgroud_type;
	void *main_obj;

	int btn_status;
	int red_score;
	int blue_score;

	void *cur_active_obj;
	void *pdirect_handle;

	void *blue_btn;
	void *blue_score_img[3];
	void *blue_select_img[2];
	void *red_btn;
	void *red_score_img[3];
	void *red_select_img[2];

	lv_timer_t *ptimer_refresh;
}divoom_lvgl_score_board_menu_ctrl, *Pdivoom_lvgl_score_board_menu_ctrl;

static const char *divoom_lvgl_score_board_menu_digit_img[] = 
{
	"30/30_zero.png",
	"30/30_one.png",
	"30/30_two.png",	
	"30/30_three.png",	
	"30/30_four.png",	
	"30/30_five.png",
	"30/30_six.png",
	"30/30_seven.png",
	"30/30_eight.png",
	"30/30_nine.png",
};


void divoom_lvgl_score_board_menu_set_score(Pdivoom_lvgl_score_board_menu_ctrl phandle)
{
	divoom_sys_msg_info msg_info = {0};
	Pdivoom_app_tool_score_info score_info = divoom_system_malloc(sizeof(divoom_app_tool_score_info));

	score_info->blue_score = phandle->blue_score;
	score_info->red_score = phandle->red_score;
	
	msg_info.data = (uint8 *)score_info;
	msg_info.msg_type = 1;
	msg_info.dest_id = DIVOOM_MODEL_APP_TOOL;
	msg_info.msg_id = DIVOOM_APP_SCORE_SET;
	divoom_sys_model_crl_msg_push(&msg_info);
}

void divoom_tool_score_board_menu_get_score(Pdivoom_lvgl_score_board_menu_ctrl phandle)
{
	divoom_app_tool_score_info score_info = {0};

	divoom_app_tool_get_score(&score_info);
	phandle->blue_score = score_info.blue_score;
	phandle->red_score = score_info.red_score;
}

void divoom_tool_score_board_menu_add_blue(Pdivoom_lvgl_score_board_menu_ctrl phandle)
{	
	divoom_app_tool_score_info score_info = {0};

	divoom_app_tool_get_score(&score_info);
	phandle->blue_score = score_info.blue_score;
	
	if (phandle->blue_score < 999)
	{
		phandle->blue_score++;
	}
	else
	{
		//phandle->blue_score = 0;
	}

	divoom_lvgl_score_board_menu_set_score(phandle);
}

void divoom_tool_score_board_menu_sub_blue(Pdivoom_lvgl_score_board_menu_ctrl phandle)
{	
	divoom_app_tool_score_info score_info = {0};

	divoom_app_tool_get_score(&score_info);
	phandle->blue_score = score_info.blue_score;
	
	if (phandle->blue_score > 0)
	{
		phandle->blue_score--;
	}
	else
	{
		//phandle->blue_score = 999;
	}

	divoom_lvgl_score_board_menu_set_score(phandle);
}

void divoom_tool_score_board_menu_add_red(Pdivoom_lvgl_score_board_menu_ctrl phandle)
{	
	divoom_app_tool_score_info score_info = {0};

	divoom_app_tool_get_score(&score_info);
	phandle->red_score = score_info.red_score;
	
	if (phandle->red_score < 999)
	{
		phandle->red_score++;
	}
	else
	{
		//phandle->red_score = 0;
	}

	divoom_lvgl_score_board_menu_set_score(phandle);
}

void divoom_tool_score_board_menu_sub_red(Pdivoom_lvgl_score_board_menu_ctrl phandle)
{	
	divoom_app_tool_score_info score_info = {0};

	divoom_app_tool_get_score(&score_info);
	phandle->red_score = score_info.red_score;
	
	if (phandle->red_score > 0)
	{
		phandle->red_score--;
	}
	else
	{
		//phandle->red_score = 999;
	}

	divoom_lvgl_score_board_menu_set_score(phandle);
}

void divoom_lvgl_score_board_menu_refresh_display(Pdivoom_lvgl_score_board_menu_ctrl phandle)
{
	divoom_app_tool_score_info score_info = {0};

	divoom_app_tool_get_score(&score_info);

	if((phandle->blue_score != score_info.blue_score) || (phandle->red_score != score_info.red_score))
	{
		phandle->blue_score = score_info.blue_score;
		phandle->red_score = score_info.red_score;
		divoom_menu_control_lv_img_set_src(phandle->blue_score_img[2], divoom_lvgl_score_board_menu_digit_img[phandle->blue_score%100%10]);
		divoom_menu_control_lv_img_set_src(phandle->blue_score_img[1], divoom_lvgl_score_board_menu_digit_img[phandle->blue_score%100/10]);
		divoom_menu_control_lv_img_set_src(phandle->blue_score_img[0], divoom_lvgl_score_board_menu_digit_img[phandle->blue_score/100]);//百
	
		divoom_menu_control_lv_img_set_src(phandle->red_score_img[2], divoom_lvgl_score_board_menu_digit_img[phandle->red_score%100%10]);
		divoom_menu_control_lv_img_set_src(phandle->red_score_img[1], divoom_lvgl_score_board_menu_digit_img[phandle->red_score%100/10]);
		divoom_menu_control_lv_img_set_src(phandle->red_score_img[0], divoom_lvgl_score_board_menu_digit_img[phandle->red_score/100]);//百		
	}
}

void divoom_lvgl_score_board_menu_refresh_task(struct _lv_timer_t *tool_refresh_time)
{
	Pdivoom_lvgl_score_board_menu_ctrl phandle = (Pdivoom_lvgl_score_board_menu_ctrl)lv_timer_get_user_data(tool_refresh_time);

	divoom_lvgl_score_board_menu_refresh_display(phandle);
}

void divoom_lvgl_score_board_menu_focus_score_btn(Pdivoom_lvgl_score_board_menu_ctrl phandle, int select_blue_flag)
{
	int i;

	if(select_blue_flag)
	{
		for(i = 0; i < 2; i++)
		{
			lv_obj_add_flag(phandle->red_select_img[i], LV_OBJ_FLAG_HIDDEN);
			lv_obj_clear_flag(phandle->blue_select_img[i], LV_OBJ_FLAG_HIDDEN);
		}
	}
	else
	{
		for(i = 0; i < 2; i++)
		{
			lv_obj_add_flag(phandle->blue_select_img[i], LV_OBJ_FLAG_HIDDEN);
			lv_obj_clear_flag(phandle->red_select_img[i], LV_OBJ_FLAG_HIDDEN);
		}
	}
}

static void divoom_lvgl_score_board_menu_event_cb(lv_event_t * e)
{
	int active_id;
	lv_obj_t *cur_obj;
    Pdivoom_lvgl_score_board_menu_ctrl phandle = (Pdivoom_lvgl_score_board_menu_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
	
	if(code == LV_EVENT_KEY)
	{
		char c = *((char *)lv_event_get_param(e));
		int active_id = divoom_menu_direct_ctrl_get_pos(phandle->pdirect_handle, cur_obj);
		
		if(c == DIVOOM_LVGL_KEY_PRE)
		{
			if(phandle->btn_status == 0)
			{
				if(cur_obj == phandle->blue_btn)
				{
					divoom_lvgl_score_board_menu_focus_score_btn(phandle, 0);
				}
				else
				{
					divoom_lvgl_score_board_menu_focus_score_btn(phandle, 1);
				}
				lv_group_focus_obj(divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_obj, DIVOOM_MENU_DIRECT_DOWN));
			}
			else
			{
				if(active_id == 0)
				{
					divoom_tool_score_board_menu_add_blue(phandle);
					divoom_lvgl_score_board_menu_refresh_display(phandle);
				}
				else
				{
					divoom_tool_score_board_menu_add_red(phandle);
					divoom_lvgl_score_board_menu_refresh_display(phandle);
				}
			}
		}
		else if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			if(phandle->btn_status == 0)
			{
				if(cur_obj == phandle->blue_btn)
				{
					divoom_lvgl_score_board_menu_focus_score_btn(phandle, 0);
				}
				else
				{
					divoom_lvgl_score_board_menu_focus_score_btn(phandle, 1);
				}
				lv_group_focus_obj(divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_obj, DIVOOM_MENU_DIRECT_UP));
			}
			else
			{
				if(active_id == 0)
				{
					divoom_tool_score_board_menu_sub_blue(phandle);
					divoom_lvgl_score_board_menu_refresh_display(phandle);
				}
				else
				{
					divoom_tool_score_board_menu_sub_red(phandle);
					divoom_lvgl_score_board_menu_refresh_display(phandle);
				}
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			if(phandle->btn_status == 0)
			{
				phandle->btn_status = 1;

				if(phandle->blue_btn == cur_obj)
				{
					divoom_menu_control_lv_img_set_src(phandle->blue_select_img[0], "30/30_left_select.png");
					divoom_menu_control_lv_img_set_src(phandle->blue_select_img[1], "30/30_right_select.png");
				}
				else
				{
					divoom_menu_control_lv_img_set_src(phandle->red_select_img[0], "30/30_left_select.png");
					divoom_menu_control_lv_img_set_src(phandle->red_select_img[1], "30/30_right_select.png");
				}
			}
			else
			{
				/*if(active_id == 0)
				{
					divoom_tool_score_board_menu_add_blue(phandle);
					divoom_lvgl_score_board_menu_refresh_display(phandle);
				}
				else
				{
					divoom_tool_score_board_menu_add_red(phandle);
					divoom_lvgl_score_board_menu_refresh_display(phandle);
				}*/
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			if(phandle->btn_status == 0)
			{
				divoom_menu_control_return_pre();
			}
			else
			{
				phandle->btn_status = 0;

				if(phandle->blue_btn == cur_obj)
				{
					divoom_menu_control_lv_img_set_src(phandle->blue_select_img[0], "30/30_left_unselect.png");
					divoom_menu_control_lv_img_set_src(phandle->blue_select_img[1], "30/30_right_unselect.png");
				}
				else
				{
					divoom_menu_control_lv_img_set_src(phandle->red_select_img[0], "30/30_left_unselect.png");
					divoom_menu_control_lv_img_set_src(phandle->red_select_img[1], "30/30_right_unselect.png");
				}
			}
		}
		else
		{
			char *key_info = (char *)lv_event_get_param(e);
			divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
			return;
		}
	}
}

void *divoom_lvgl_score_board_menu_init(void *user_data, int menu_id)
{
	int i, score;
	
	Pdivoom_lvgl_score_board_menu_ctrl phandle = (Pdivoom_lvgl_score_board_menu_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_score_board_menu_ctrl));
    phandle->menu_id = menu_id;
	phandle->pdirect_handle = divoom_menu_direct_ctrl_create(1, 2);
	phandle->cur_active_obj = NULL;
	phandle->btn_status = 0;
	divoom_tool_score_board_menu_get_score(phandle);
	phandle->main_obj = lv_obj_create(NULL);

	lv_style_init(&phandle->backgroud_type);
    lv_style_set_bg_opa(&phandle->backgroud_type, 0);
    lv_style_set_text_color(&phandle->backgroud_type, lv_color_hex(0x0));
    lv_obj_add_style(phandle->main_obj, &phandle->backgroud_type, 0);
    lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);

	/*lv_obj_t *top_img = lv_img_create(phandle->main_obj);
	divoom_menu_control_lv_img_set_src(top_img, "30/30_top.png");
	lv_obj_set_size(top_img, 501, 107);
	lv_obj_set_pos(top_img, 150, 0);

	lv_obj_t *rect_head_img = lv_img_create(phandle->main_obj);
	divoom_menu_control_lv_img_set_src(rect_head_img, "30/30_head.png");
	lv_obj_set_size(rect_head_img, 726, 28);
	lv_obj_set_pos(rect_head_img, 37, 107);

	lv_obj_t *bg_obj = lv_obj_create(phandle->main_obj);
	lv_obj_set_style_pad_all(bg_obj, 0, 0);
	lv_obj_set_style_border_width(bg_obj, 0, 0);
	lv_obj_set_style_bg_color(bg_obj, lv_color_hex(0x262827), 0);
	lv_obj_set_style_radius(bg_obj, 13, 0);
	lv_obj_set_pos(bg_obj, 37, 125);
	lv_obj_set_size(bg_obj, 726, 994);
	lv_obj_clear_flag(bg_obj, LV_OBJ_FLAG_SCROLLABLE);*/

	lv_obj_t *bg_obj_img = lv_img_create(phandle->main_obj);
	divoom_menu_control_lv_img_set_src(bg_obj_img, "30/30_bg.png");
	lv_obj_set_pos(bg_obj_img, 0, 0);

	lv_obj_t *blue_btn = lv_btn_create(phandle->main_obj);
	lv_obj_remove_style_all(blue_btn);
	lv_obj_set_style_pad_all(blue_btn, 0, 0);
	//lv_obj_set_style_radius(blue_btn, 32, 0);
	//lv_obj_set_style_bg_color(blue_btn, lv_color_hex(0xCACACA), 0);
	//lv_obj_set_style_shadow_width(blue_btn, 0, 0);
	//lv_obj_set_style_border_width(blue_btn, 6, 0);
	//lv_obj_set_style_outline_width(blue_btn, 0, 0);
	//lv_obj_set_style_border_color(blue_btn, lv_color_hex(0xCACACA), 0);
	//lv_obj_set_style_border_color(blue_btn, lv_color_hex(0x6D6D6D), LV_STATE_FOCUSED);
	//lv_obj_set_style_border_color(blue_btn, lv_color_hex(0x6D6D6D), LV_STATE_FOCUS_KEY);
	lv_obj_set_style_bg_opa(blue_btn, 0, 0);
	lv_obj_set_size(blue_btn, 572, 333);
	lv_obj_set_pos(blue_btn, 112, 271);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, blue_btn, 0, 0);
	lv_obj_add_event_cb(blue_btn, divoom_lvgl_score_board_menu_event_cb, LV_EVENT_KEY, phandle);
	phandle->blue_btn = blue_btn;

	if(phandle->cur_active_obj == NULL)
	{
		phandle->cur_active_obj = blue_btn;
	}
	
	lv_obj_t *blue_left_img = lv_img_create(phandle->main_obj);
	divoom_menu_control_lv_img_set_src(blue_left_img, "30/30_left_unselect.png");
	lv_obj_set_pos(blue_left_img, 0, 362);
	phandle->blue_select_img[0] = blue_left_img;

	lv_obj_t *blue_right_img = lv_img_create(phandle->main_obj);
	divoom_menu_control_lv_img_set_src(blue_right_img, "30/30_right_unselect.png");
	lv_obj_set_pos(blue_right_img, 746, 362);
	phandle->blue_select_img[1] = blue_right_img;
	
	//lv_obj_t *blue_score_bg_obj = lv_img_create(blue_btn);
	//lv_obj_set_pos(blue_score_bg_obj, 40, 36);
	//divoom_menu_control_lv_img_set_src(blue_score_bg_obj, "30/30_blue_bg.png");

	for(i = 0;i < 3; i++)
	{
		phandle->blue_score_img[i] = lv_img_create(phandle->main_obj);
		lv_obj_set_pos(phandle->blue_score_img[i], 54+231*i, 139);
	}

	divoom_menu_control_lv_img_set_src(phandle->blue_score_img[2], divoom_lvgl_score_board_menu_digit_img[phandle->blue_score%100%10]);
	divoom_menu_control_lv_img_set_src(phandle->blue_score_img[1], divoom_lvgl_score_board_menu_digit_img[phandle->blue_score%100/10]);
	divoom_menu_control_lv_img_set_src(phandle->blue_score_img[0], divoom_lvgl_score_board_menu_digit_img[phandle->blue_score/100]);
	
	lv_obj_t *red_btn = lv_btn_create(phandle->main_obj);
	lv_obj_remove_style_all(red_btn);
	lv_obj_set_style_pad_all(red_btn, 0, 0);
	//lv_obj_set_style_radius(red_btn, 32, 0);
	//lv_obj_set_style_bg_color(red_btn, lv_color_hex(0xCACACA), 0);
	//lv_obj_set_style_shadow_width(red_btn, 0, 0);
	//lv_obj_set_style_outline_width(red_btn, 0, 0);
	//lv_obj_set_style_border_width(red_btn, 6, 0);
	//lv_obj_set_style_border_color(red_btn, lv_color_hex(0xCACACA), 0);
	//lv_obj_set_style_border_color(red_btn, lv_color_hex(0x6D6D6D), LV_STATE_FOCUSED);
	//lv_obj_set_style_border_color(red_btn, lv_color_hex(0x6D6D6D), LV_STATE_FOCUS_KEY);
	lv_obj_set_style_bg_opa(red_btn, 0, 0);
	lv_obj_set_size(red_btn, 572, 333);
	lv_obj_set_pos(red_btn, 112, 677);
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, red_btn, 0, 1);
	lv_obj_add_event_cb(red_btn, divoom_lvgl_score_board_menu_event_cb, LV_EVENT_KEY, phandle);
	phandle->red_btn = red_btn;

	lv_obj_t *red_left_img = lv_img_create(phandle->main_obj);
	divoom_menu_control_lv_img_set_src(red_left_img, "30/30_left_unselect.png");
	lv_obj_set_pos(red_left_img, 0, 378+640);
	phandle->red_select_img[0] = red_left_img;

	lv_obj_t *red_right_img = lv_img_create(phandle->main_obj);
	divoom_menu_control_lv_img_set_src(red_right_img, "30/30_right_unselect.png");
	lv_obj_set_pos(red_right_img, 746, 378+640);
	phandle->red_select_img[1] = red_right_img;
	
	//lv_obj_t *red_score_bg_obj = lv_img_create(red_btn);
	//lv_obj_set_pos(red_score_bg_obj, 40, 36);
	//divoom_menu_control_lv_img_set_src(red_score_bg_obj, "30/30_blue_bg.png");
	
	for(i = 0;i < 3; i++)
	{
		phandle->red_score_img[i] = lv_img_create(phandle->main_obj);
		lv_obj_set_pos(phandle->red_score_img[i], 53+231*i, 171+640);
	}

	divoom_menu_control_lv_img_set_src(phandle->red_score_img[2], divoom_lvgl_score_board_menu_digit_img[phandle->red_score%100%10]);
	divoom_menu_control_lv_img_set_src(phandle->red_score_img[1], divoom_lvgl_score_board_menu_digit_img[phandle->red_score%100/10]);
	divoom_menu_control_lv_img_set_src(phandle->red_score_img[0], divoom_lvgl_score_board_menu_digit_img[phandle->red_score/100]);
		
	divoom_lvgl_score_board_menu_focus_score_btn(phandle, 1);
	
	phandle->ptimer_refresh = lv_timer_create(divoom_lvgl_score_board_menu_refresh_task, 200, phandle);

	return phandle;
}

void divoom_lvgl_score_board_menu_disp(void *handle, void *active)
{
	Pdivoom_lvgl_score_board_menu_ctrl phandle = (Pdivoom_lvgl_score_board_menu_ctrl)handle;

	if(phandle->cur_active_obj)
	{
		lv_group_focus_obj(phandle->cur_active_obj);
		phandle->cur_active_obj = NULL;
	}
	else if(active)
	{
		lv_group_focus_obj(active);
	}
	lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_score_board_menu_exit(void *handle)
{
	Pdivoom_lvgl_score_board_menu_ctrl phandle = (Pdivoom_lvgl_score_board_menu_ctrl)handle;
	
	lv_style_reset(&phandle->backgroud_type);
	lv_timer_del(phandle->ptimer_refresh);
	divoom_menu_direct_ctrl_destory(phandle->pdirect_handle);
	lv_obj_del(phandle->main_obj);
	divoom_system_free(phandle);
}


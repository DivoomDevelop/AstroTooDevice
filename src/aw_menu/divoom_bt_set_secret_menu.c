#include "divoom_lvgl_stream_cont.h"
#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"

#ifdef DIVOOM_TIME_FRAME_SPEAKER
typedef struct _divoom_bt_set_secret_menu_ctrl
{
    int menu_id;
	lv_style_t backgroud_type;
	lv_style_t btn_normal_style;
	lv_style_t btn_select_style;
	void *main_obj;

	lv_obj_t *bt_secret_switch;	//蓝牙密码开关
	lv_obj_t *cur_active_obj;
	lv_obj_t *bt_secret_image;  //蓝牙密码图片
	
	lv_obj_t *bt_set_secret_obj;  //设置密码
	lv_obj_t *bt_secret_btn[4];   //密码按钮
	int bt_set_flag;		//设置密码标志位
	int bt_select_id;		//密码当前选中位

	lv_obj_t *bt_secret_label[4];  //蓝牙密码
	lv_obj_t *bt_save_btn;			//蓝牙保存按钮
	//char bt_secret[4];		//编辑密码
	int active_id;

	Pdivoom_sys_setting_info psys_config;
}divoom_bt_set_secret_menu_ctrl, *Pdivoom_bt_set_secret_menu_ctrl;

void divoom_bt_set_secret_menu_event_cb(lv_event_t *e)
{
	int i;
	Pdivoom_bt_set_secret_menu_ctrl phandle = (Pdivoom_bt_set_secret_menu_ctrl)lv_event_get_user_data(e);
	lv_obj_t *cur_btn_obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
	
	if(code == LV_EVENT_KEY)
    {
		char c = *((char *)lv_event_get_param(e));
        Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();
        
		if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			if(phandle->bt_set_flag == 0)
			{
				if(cur_btn_obj == phandle->bt_secret_switch)
				{
					lv_group_focus_obj(phandle->bt_set_secret_obj);
				}
				else if(cur_btn_obj == phandle->bt_set_secret_obj)
				{
					lv_group_focus_obj(phandle->bt_save_btn);
				}
				else if(cur_btn_obj == phandle->bt_save_btn)
				{
					lv_group_focus_obj(phandle->bt_secret_switch);
				}
			}
			else
			{
				for(i = 0; i < 4; i++)
				{
					if(cur_btn_obj == phandle->bt_secret_btn[i])
					{
						if(phandle->psys_config->bt_password[i] > '8')
						{
							phandle->psys_config->bt_password[i] = '0';
						}
						else
						{
							phandle->psys_config->bt_password[i]++;
						}
						lv_obj_set_style_text_color(phandle->bt_secret_label[i], lv_color_hex(0xE89B00), 0);
						lv_label_set_text_fmt(phandle->bt_secret_label[i] , "%c", phandle->psys_config->bt_password[i]);
						break;
					}
				}
				
			}
		}
		else if(c == DIVOOM_LVGL_KEY_PRE)
		{
			if(phandle->bt_set_flag == 0)
			{
				if(cur_btn_obj == phandle->bt_secret_switch)
				{
					lv_group_focus_obj(phandle->bt_save_btn);
				}
				else if(cur_btn_obj == phandle->bt_set_secret_obj)
				{
					lv_group_focus_obj(phandle->bt_secret_switch);
				}
				else if(cur_btn_obj == phandle->bt_save_btn)
				{
					lv_group_focus_obj(phandle->bt_set_secret_obj);
				}
			}
			else
			{
				for(i = 0; i < 4; i++)
				{
					if(cur_btn_obj == phandle->bt_secret_btn[i])
					{
						if(phandle->psys_config->bt_password[i] < '1')
						{
							phandle->psys_config->bt_password[i] = '9';
						}
						else
						{
							phandle->psys_config->bt_password[i]--;
						}
						lv_obj_set_style_text_color(phandle->bt_secret_label[i], lv_color_hex(0xE89B00), 0);
						lv_label_set_text_fmt(phandle->bt_secret_label[i] , "%c", phandle->psys_config->bt_password[i]);
						break;
					}
				}
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			if(phandle->bt_set_flag == 0)
			{
				if(cur_btn_obj == phandle->bt_secret_switch)
				{
					//蓝牙开关
					if(phandle->psys_config->bt_secret_switch == 0)
					{
						phandle->psys_config->bt_secret_switch = 1;
						divoom_menu_control_lv_img_set_src(phandle->bt_secret_image, "26/26_on.png");
					}
					else
					{
						phandle->psys_config->bt_secret_switch = 0;
						divoom_menu_control_lv_img_set_src(phandle->bt_secret_image, "26/26_off.png");
					}
				}
				else if(cur_btn_obj == phandle->bt_set_secret_obj)
				{
					//设置密码
					phandle->bt_set_flag = 1;
					phandle->bt_select_id = 0;
					lv_obj_set_style_text_color(phandle->bt_secret_label[phandle->bt_select_id], lv_color_hex(0xE89B00), 0);
					lv_group_focus_obj(phandle->bt_secret_btn[0]);
				}
				else if(cur_btn_obj == phandle->bt_save_btn)
				{
					//保存密码
					divoom_main_save_config();
					divoom_bt_music_set_pair_pin(phandle->psys_config->bt_secret_switch, phandle->psys_config->bt_password);
					divoom_menu_control_return_pre();
				}
			}
			else
			{
				for(i = 0; i < 4; i++)
				{
					if(cur_btn_obj == phandle->bt_secret_btn[i])
					{
						break;
					}
				}

				if(i < 4)
				{
					lv_obj_set_style_text_color(phandle->bt_secret_label[i], lv_color_hex(0x0), 0);

					i = i + 1;
					if(i >= 4)
					{
						i = 0;
					}
					
					phandle->bt_select_id = i;
					lv_group_focus_obj(phandle->bt_secret_btn[i]);
					lv_obj_set_style_text_color(phandle->bt_secret_label[phandle->bt_select_id], lv_color_hex(0xE89B00), 0);

					//保存密码
					
				}
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			if(phandle->bt_set_flag == 0)
			{
				divoom_menu_control_return_pre();
			}
			else
			{
				phandle->bt_set_flag = 0;
				lv_obj_set_style_text_color(phandle->bt_secret_label[phandle->bt_select_id], lv_color_hex(0x0), 0);
				lv_group_focus_obj(phandle->bt_set_secret_obj);
			}
		}
    }
	

}

void divoom_bt_set_secret_menu_create_image_grad(Pdivoom_bt_set_secret_menu_ctrl phandle)
{
	int i;

	lv_style_init(&phandle->btn_normal_style);
	lv_style_set_bg_color(&phandle->btn_normal_style, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->btn_normal_style, 0);
	lv_style_set_border_width(&phandle->btn_normal_style, 4);
	lv_style_set_border_color(&phandle->btn_normal_style, lv_color_hex(0xE4E4E4));
	lv_style_set_radius(&phandle->btn_normal_style, 17);
	lv_style_set_shadow_width(&phandle->btn_normal_style, 0);
	lv_style_set_outline_width(&phandle->btn_normal_style, 0);

	lv_style_init(&phandle->btn_select_style);
	lv_style_set_bg_color(&phandle->btn_select_style, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->btn_select_style, 0);
	lv_style_set_border_width(&phandle->btn_select_style, 4);
	lv_style_set_border_color(&phandle->btn_select_style, lv_color_hex(0x6D6D6D));
	lv_style_set_radius(&phandle->btn_select_style, 17);
	lv_style_set_shadow_width(&phandle->btn_select_style, 0);
	lv_style_set_outline_width(&phandle->btn_select_style, 0);

	lv_obj_t *title = lv_obj_create(phandle->main_obj);
	lv_obj_set_style_bg_opa(title, LV_OPA_0, 0);
	lv_obj_set_style_border_width(title, 0, 0);
	lv_obj_set_style_pad_all(title, 0, 0);
	lv_obj_set_style_text_color(title, lv_color_black(), 0);
	lv_obj_clear_flag(title, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(title, 800, 78); 
	lv_obj_set_pos(title, 0, 40);
	
	lv_obj_t *setsecret_label = lv_label_create(title);
	lv_obj_set_size(setsecret_label, 200+400, 56); 
	lv_obj_set_style_text_font(setsecret_label, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_obj_set_style_text_align(setsecret_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_text(setsecret_label, divoom_menu_control_get_disp_str("BTSecret"));
	lv_obj_center(setsecret_label);
	
	//蓝牙密码开关
	phandle->bt_secret_switch = lv_btn_create(phandle->main_obj);
	lv_obj_add_style(phandle->bt_secret_switch, &phandle->btn_normal_style, 0);
	lv_obj_add_style(phandle->bt_secret_switch, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(phandle->bt_secret_switch, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(phandle->bt_secret_switch, 0, 0);
	lv_obj_set_size(phandle->bt_secret_switch, 712, 90);
	lv_obj_set_pos(phandle->bt_secret_switch, 44, 164);
	lv_obj_add_event_cb(phandle->bt_secret_switch, divoom_bt_set_secret_menu_event_cb, LV_EVENT_KEY, phandle);
	phandle->cur_active_obj = phandle->bt_secret_switch;

	lv_obj_t *label2 = lv_label_create(phandle->bt_secret_switch);
	lv_obj_set_size(label2, 473, 34); 
	lv_obj_set_style_text_color(label2, lv_color_hex(0x0), 0);
	lv_obj_set_style_text_font(label2, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
	lv_obj_set_style_text_align(label2, LV_TEXT_ALIGN_LEFT, 0);
	lv_label_set_text(label2, divoom_menu_control_get_disp_str("BTSecret"));
	lv_obj_set_pos(label2, 32, 28);

	phandle->bt_secret_image = lv_img_create(phandle->bt_secret_switch);
	lv_obj_set_pos(phandle->bt_secret_image, 608, 22);
	lv_obj_set_size(phandle->bt_secret_image, 72, 44); 
	if(phandle->psys_config->bt_secret_switch)
	{
		divoom_menu_control_lv_img_set_src(phandle->bt_secret_image, "26/26_on.png");
	}
	else
	{
		divoom_menu_control_lv_img_set_src(phandle->bt_secret_image, "26/26_off.png");
	}

	//设置密码
	phandle->bt_set_secret_obj = lv_btn_create(phandle->main_obj);
	lv_obj_add_style(phandle->bt_set_secret_obj, &phandle->btn_normal_style, 0);
	lv_obj_add_style(phandle->bt_set_secret_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
	lv_obj_add_style(phandle->bt_set_secret_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(phandle->bt_set_secret_obj, 0, 0);
	lv_obj_set_size(phandle->bt_set_secret_obj, 712, 420);
	lv_obj_set_pos(phandle->bt_set_secret_obj, 44, 302);
	lv_obj_add_event_cb(phandle->bt_set_secret_obj, divoom_bt_set_secret_menu_event_cb, LV_EVENT_KEY, phandle);
	
	lv_obj_t *setsecret_label2 = lv_label_create(phandle->bt_set_secret_obj);
	lv_obj_set_size(setsecret_label2, 400, 34); 
	lv_obj_set_style_text_color(setsecret_label2, lv_color_hex(0x0), 0);
	lv_obj_set_style_text_font(setsecret_label2, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
	lv_obj_set_style_text_align(setsecret_label2, LV_TEXT_ALIGN_LEFT, 0);
	lv_label_set_text(setsecret_label2, divoom_menu_control_get_disp_str("SetSecret"));
	lv_obj_set_pos(setsecret_label2, 32, 28);

	for(i = 0;i < 4; i++)
	{
		lv_obj_t *obj = lv_btn_create(phandle->bt_set_secret_obj);
		lv_obj_set_style_radius(obj, 16, 0);
		lv_obj_set_style_bg_color(obj, lv_color_hex(0xE4E4E4), 0);
		lv_obj_set_style_pad_all(obj, 0, 0);
		lv_obj_set_style_border_width(obj, 4, 0);
		lv_obj_set_style_border_color(obj, lv_color_hex(0xE4E4E4), 0);
		lv_obj_set_style_border_color(obj, lv_color_hex(0x6D6D6D), LV_STATE_FOCUSED);
		lv_obj_set_style_border_color(obj, lv_color_hex(0x6D6D6D), LV_STATE_FOCUS_KEY);
		lv_obj_set_style_outline_width(obj, 0, 0);
		lv_obj_set_style_outline_width(obj, 0, LV_STATE_FOCUSED);
		lv_obj_set_style_outline_width(obj, 0, LV_STATE_FOCUS_KEY);
		lv_obj_set_style_shadow_width(obj, 0, 0);
		lv_obj_set_size(obj, 156, 290);
		lv_obj_set_pos(obj, 32+i*164, 98);
		lv_obj_add_event_cb(obj, divoom_bt_set_secret_menu_event_cb, LV_EVENT_KEY, phandle);
		phandle->bt_secret_btn[i] = obj;

		lv_obj_t *setsecret_label3 = lv_label_create(obj);
		lv_obj_set_size(setsecret_label3, 156, 290); 
		lv_obj_set_style_text_font(setsecret_label3, divoom_menu_control_get_font_info(180, phandle->menu_id), 0);
		lv_obj_set_style_text_align(setsecret_label3, LV_TEXT_ALIGN_CENTER, 0);
		lv_obj_set_style_text_color(setsecret_label3, lv_color_hex(0x0), 0);

		if(phandle->psys_config->bt_password[i] >= '0' && phandle->psys_config->bt_password[i] <= '9')
		{
			lv_label_set_text_fmt(setsecret_label3, "%c", phandle->psys_config->bt_password[i]);
		}
		else
		{
			phandle->psys_config->bt_password[i] = '0';
			lv_label_set_text_fmt(setsecret_label3, "%c", phandle->psys_config->bt_password[i]);
		}

		lv_obj_set_pos(setsecret_label3, 0, 0);
		phandle->bt_secret_label[i] = setsecret_label3;
	}
	
	lv_obj_t *setsecret_label4 = lv_label_create(phandle->main_obj);
	lv_obj_set_size(setsecret_label4, 720, 60); 
	lv_obj_set_style_text_font(setsecret_label4, divoom_menu_control_get_font_info(20, phandle->menu_id), 0);
	lv_obj_set_style_text_align(setsecret_label4, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_color(setsecret_label4, lv_color_hex(0x666666), 0);
	lv_label_set_text(setsecret_label4, divoom_menu_control_get_disp_str("SetSecretTip"));
	lv_obj_set_pos(setsecret_label4, 40, 766);

	lv_obj_t *save_btn = lv_btn_create(phandle->main_obj);
	lv_obj_remove_style_all(save_btn);
	lv_obj_set_pos(save_btn, 54, 1054);
	lv_obj_set_width(save_btn, 692);
	lv_obj_set_height(save_btn, 108);
	lv_obj_set_style_pad_all(save_btn, 0, 0);
	lv_obj_set_style_border_width(save_btn, 6, 0);
	lv_obj_set_style_border_color(save_btn, lv_color_hex(0xE4E4E4), 0);
	//lv_obj_set_style_border_opa(save_btn, LV_OPA_0, 0);
	lv_obj_set_style_radius(save_btn, 75, 0);
	lv_obj_set_style_bg_color(save_btn, lv_color_hex(0x6BD96B), 0);
	lv_obj_set_style_bg_color(save_btn, lv_color_hex(0x17CD17), LV_STATE_FOCUSED);
	lv_obj_set_style_bg_color(save_btn, lv_color_hex(0x17CD17), LV_STATE_FOCUS_KEY);
	lv_obj_set_style_bg_opa(save_btn, LV_OPA_100, 0);
	lv_obj_set_style_bg_opa(save_btn, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	lv_obj_set_style_bg_opa(save_btn, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_shadow_width(save_btn, 0, 0); //去掉边框底部的阴影
	//lv_obj_set_align(save_btn, LV_ALIGN_CENTER);
	lv_obj_set_style_outline_width(save_btn, 6, 0);
	lv_obj_set_style_outline_color(save_btn, lv_color_hex(0x6BD96B), 0);
	lv_obj_set_style_outline_color(save_btn, lv_color_hex(0x198D19), LV_STATE_FOCUSED);
	lv_obj_set_style_outline_color(save_btn, lv_color_hex(0x198D19), LV_STATE_FOCUS_KEY);
	lv_obj_set_style_text_color(save_btn, lv_color_hex(0x2D712D), 0);
	lv_obj_set_style_text_color(save_btn, lv_color_hex(0x074F07), LV_STATE_FOCUSED);
	lv_obj_set_style_text_color(save_btn, lv_color_hex(0x074F07), LV_STATE_FOCUS_KEY);
	lv_obj_add_event_cb(save_btn, divoom_bt_set_secret_menu_event_cb, LV_EVENT_KEY, phandle);
	phandle->bt_save_btn = save_btn;
	
	lv_obj_t *save_label = lv_label_create(save_btn);
	//lv_obj_set_style_text_color(save_label, lv_color_hex(0x228B26), 0);
	lv_obj_set_style_text_font(save_label, divoom_menu_control_get_font_info(48, phandle->menu_id), 0);
	lv_label_set_text(save_label, divoom_menu_control_get_disp_str("Save"));
	lv_obj_center(save_label);
}


void *divoom_bt_set_secret_menu_init(void *user_data, int menu_id)
{
	int ret, i;

	Pdivoom_bt_set_secret_menu_ctrl phandle = divoom_system_malloc(sizeof(divoom_bt_set_secret_menu_ctrl));

	phandle->psys_config = divoom_main_get_system_config();

	phandle->main_obj = lv_obj_create(NULL);
    phandle->menu_id = menu_id;
	phandle->bt_set_flag = 0;

	lv_style_init(&phandle->backgroud_type);
    lv_style_set_bg_color(&phandle->backgroud_type, lv_color_hex(0xE4E4E4));
    lv_style_set_text_color(&phandle->backgroud_type, lv_color_hex(0x0));
    lv_obj_add_style(phandle->main_obj, &phandle->backgroud_type, 0);
    lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(phandle->main_obj, 0, 0);
	
	ret = divoom_bt_music_get_pair_pin(&phandle->psys_config->bt_secret_switch, phandle->psys_config->bt_password, 5);
	if(ret == -1)
	{
		for(i = 0; i < 4; i++)
		{
			phandle->psys_config->bt_password[i] = '0';
		}
	}

	divoom_bt_set_secret_menu_create_image_grad(phandle);
	
	return phandle;
}

void divoom_bt_set_secret_menu_disp(void *handle, void *active)
{
	Pdivoom_bt_set_secret_menu_ctrl phandle = (Pdivoom_bt_set_secret_menu_ctrl)handle;

	if(active)
	{
		lv_group_focus_obj(active);
	}	
	else if(phandle->cur_active_obj)
	{
		lv_group_focus_obj(phandle->cur_active_obj);
		phandle->cur_active_obj = NULL;
	}
	
	lv_scr_load(phandle->main_obj);
}

void divoom_bt_set_secret_menu_eixt(void *handle)
{
	Pdivoom_bt_set_secret_menu_ctrl phandle = (Pdivoom_bt_set_secret_menu_ctrl)handle;

	lv_style_reset(&phandle->backgroud_type);
	lv_style_reset(&phandle->btn_normal_style);
	lv_style_reset(&phandle->btn_select_style);
	lv_obj_del(phandle->main_obj);
	divoom_system_free(phandle);
}



#else
typedef struct _divoom_bt_set_secret_menu_ctrl
{
    int menu_id;
	lv_style_t backgroud_type;
	void *main_obj;

	void *stream_cont_bt_secret[4];
	int bt_secret[4];
	int active_id;

	void *back_btn;
	void *check_btn;
	void *cancel_btn;
}divoom_bt_set_secret_menu_ctrl, *Pdivoom_bt_set_secret_menu_ctrl;

void divoom_bt_set_secret_menu_event_cb(lv_event_t *e)
{
	int i;
	Pdivoom_bt_set_secret_menu_ctrl phandle = (Pdivoom_bt_set_secret_menu_ctrl)lv_event_get_user_data(e);
	lv_obj_t *cur_btn_obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
	
	switch(code)
	{
		case LV_EVENT_KEY:
			if(cur_btn_obj == phandle->back_btn)
			{
				divoom_menu_control_return_pre();
			}
			else if(cur_btn_obj == phandle->check_btn)
			{
				for(i = 0; i < 4; i++)
				{
					phandle->bt_secret[i] = divoom_stream_cont_get_current_selected(phandle->stream_cont_bt_secret[i]);
					//divoom_print("bt_secret[%d] = %d\n", i, phandle->bt_secret[i]);
				}
				divoom_menu_control_return_pre();
			}
			else if(cur_btn_obj == phandle->cancel_btn)
			{
				divoom_menu_control_return_pre();
			}
			break;
        default:
            break;
	}

}

void divoom_bt_set_secret_menu_create_image_grad(Pdivoom_bt_set_secret_menu_ctrl phandle)
{
	int i;

	lv_obj_t *title = lv_obj_create(phandle->main_obj);
	lv_obj_set_style_bg_opa(title, LV_OPA_0, 0);
	lv_obj_set_style_border_width(title, 0, 0);
	lv_obj_set_style_pad_all(title, 0, 0);
	lv_obj_set_style_text_color(title, lv_color_white(), 0);
	lv_obj_clear_flag(title, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(title, 800, 78); 
	lv_obj_set_pos(title, 0, 40);
	
	lv_obj_t *back_btn = lv_btn_create(title);
	lv_obj_set_size(back_btn, 78, 78); 
	lv_obj_set_pos(back_btn, 19, 0);
	lv_obj_set_style_bg_opa(back_btn, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(back_btn, 0, 0);
	lv_obj_set_style_shadow_width(back_btn, 0, 0);
	lv_obj_clear_flag(back_btn, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_event(back_btn, divoom_bt_set_secret_menu_event_cb, LV_EVENT_KEY, phandle);
	phandle->back_btn = back_btn;
		 
	lv_obj_t *back_img = lv_img_create(back_btn);
	lv_obj_set_size(back_img, 78, 78); 
	divoom_menu_control_lv_img_set_src(back_img, "64/64_back.png");
	
	lv_obj_t *setsecret_label = lv_label_create(title);
	lv_obj_set_size(setsecret_label, 200, 56); 
	lv_obj_set_style_text_font(setsecret_label, divoom_menu_control_get_font_info(48, phandle->menu_id), 0);
	lv_obj_set_style_text_align(setsecret_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_text(setsecret_label, divoom_menu_control_get_disp_str("SetSecret"));
	lv_obj_center(setsecret_label);
	
	for(i = 0;i < 4; i++)
	{
		lv_obj_t *obj = lv_obj_create(phandle->main_obj);
		lv_obj_set_style_radius(obj, 32, 0);
		lv_obj_set_style_bg_color(obj, lv_color_hex(0x2E2E2E), 0);
		lv_obj_set_style_pad_all(obj, 0, 0);
		lv_obj_set_style_border_width(obj, 0, 0);
		lv_obj_set_size(obj, 160, 200);
		lv_obj_set_pos(obj, 41+i*186, 453);
		
		phandle->stream_cont_bt_secret[i] = divoom_stream_cont_init(phandle->main_obj, 41+i*186, 340, 160, 411, 
			160, 157, 80, 100, 0xFFFFFF, 10, 0, 1);
		divoom_stream_cont_set_current_selected(phandle->stream_cont_bt_secret[i], 0);
	}

	lv_obj_t *add_btn = lv_btn_create(phandle->main_obj);
	lv_obj_set_size(add_btn, 180, 180);
	lv_obj_set_pos(add_btn, 551, 1040);
	lv_obj_set_style_radius(add_btn, 99, 0);
	lv_obj_set_style_pad_all(add_btn, 0, 0);
	lv_obj_set_style_bg_opa(add_btn, LV_OPA_0, 0);
	lv_obj_set_style_shadow_width(add_btn, 0, 0);
	lv_obj_set_style_bg_color(add_btn, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(add_btn, LV_OPA_10, LV_STATE_PRESSED);
	phandle->check_btn = add_btn;
	
	lv_obj_t *add_img = lv_img_create(add_btn);
	divoom_menu_control_lv_img_set_src(add_img, "64/64_check.png");
	lv_obj_add_event_cb(add_btn, divoom_bt_set_secret_menu_event_cb, LV_EVENT_KEY, phandle);
	
	lv_obj_t *cancel_btn = lv_btn_create(phandle->main_obj);
	lv_obj_set_size(cancel_btn, 180, 180);
	lv_obj_set_pos(cancel_btn, 74, 1040);
	lv_obj_set_style_radius(cancel_btn, 99, 0);
	lv_obj_set_style_pad_all(cancel_btn, 0, 0);
	lv_obj_set_style_bg_opa(cancel_btn, LV_OPA_0, 0);
	lv_obj_set_style_shadow_width(cancel_btn, 0, 0);
	lv_obj_set_style_bg_color(cancel_btn, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(cancel_btn, LV_OPA_10, LV_STATE_PRESSED);
	phandle->cancel_btn = cancel_btn;

	lv_obj_t *cancel_img = lv_img_create(cancel_btn);
	divoom_menu_control_lv_img_set_src(cancel_img, "64/64_cancel.png");
	lv_obj_add_event_cb(cancel_btn, divoom_bt_set_secret_menu_event_cb, LV_EVENT_KEY, phandle);
}


void *divoom_bt_set_secret_menu_init(void *user_data, int menu_id)
{
	Pdivoom_bt_set_secret_menu_ctrl phandle = divoom_system_malloc(sizeof(divoom_bt_set_secret_menu_ctrl));

	phandle->main_obj = lv_obj_create(NULL);
    phandle->menu_id = menu_id;
	lv_style_init(&phandle->backgroud_type);
    lv_style_set_bg_color(&phandle->backgroud_type, lv_color_hex(0x000000));
    lv_style_set_text_color(&phandle->backgroud_type, lv_color_hex(0xFFFFFF));
    lv_obj_add_style(phandle->main_obj, &phandle->backgroud_type, 0);
    lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);

	divoom_bt_set_secret_menu_create_image_grad(phandle);
	
	return phandle;
}

void divoom_bt_set_secret_menu_disp(void *handle, void *active)
{
	Pdivoom_bt_set_secret_menu_ctrl phandle = (Pdivoom_bt_set_secret_menu_ctrl)handle;
	
	lv_scr_load(phandle->main_obj);
}

void divoom_bt_set_secret_menu_eixt(void *handle)
{
	Pdivoom_bt_set_secret_menu_ctrl phandle = (Pdivoom_bt_set_secret_menu_ctrl)handle;

	for(int i = 0;i < 4; i++)
	{
		divoom_stream_cont_del(phandle->stream_cont_bt_secret[i]);
	}
	lv_style_reset(&phandle->backgroud_type);
	lv_obj_del(phandle->main_obj);
	divoom_system_free(phandle);
}
#endif

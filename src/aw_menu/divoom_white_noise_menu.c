#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_queue.h"
#include "divoom_white_noise_menu.h"

typedef struct _divoom_white_noise_menu_ctrl
{
    int menu_id;
    lv_coord_t col_dsc[3];
    lv_coord_t row_dsc[20];
    lv_obj_t *main_obj;
    lv_obj_t *cur_active_obj;
    lv_style_t backgroud_type;
    lv_obj_t *volume_slide_obj[20];
    lv_obj_t *switch_obj[20];
	lv_obj_t *btn[2];
	lv_obj_t *img[2];
	lv_obj_t *label_time;
	lv_obj_t *arrow_btn_obj;
    divoom_white_noise_info *base_info;
    lv_style_t grid_type;
	lv_style_t style_main;
	lv_style_t style_indicator;
	lv_style_t style_knob;
}divoom_white_noise_menu_ctrl, *Pdivoom_white_noise_menu_ctrl;

static Pdivoom_white_noise_menu_ctrl Phandle = NULL;

static const char *divoom_white_noise_img_list[] = {
    "50/50_feng.png",
    "50/50_night.png",
    "50/50_fire.png",
    "50/50_sea.png",
    "50/50_horse.png",
    "50/50_creek.png",
    "50/50_bird.png",
    "50/50_quiet.png",
};

static char menu_text_table[][20] = {
	"WhiteNoise",
};

static uint8 divoom_white_entern_flag = 0;

static divoom_white_noise_info base_info = {0};

uint8 divoom_white_noise_menu_set_enter_flag(uint8 flag)
{
	uint8 ret = 0;

	if (flag == 0xff)
	{
		ret = divoom_white_entern_flag;
		divoom_white_entern_flag = 0;
		return ret;
	}	
	divoom_white_entern_flag = flag;
}

void *divoom_white_noise_menu_set_return_arrow_obj(void *obj)
{
	//返回箭头
	lv_obj_t *arrow_btn_obj = lv_btn_create(obj);
	lv_obj_set_x(arrow_btn_obj, 19);
	lv_obj_set_y(arrow_btn_obj, 66);
	lv_obj_set_width(arrow_btn_obj, 78);
	lv_obj_set_height(arrow_btn_obj, 78);
	lv_obj_set_style_pad_all(arrow_btn_obj, 0, 0);
	lv_obj_set_style_border_width(arrow_btn_obj, 1, 0);
	lv_obj_set_style_border_opa(arrow_btn_obj, LV_OPA_0, 0);	 
	lv_obj_set_style_bg_color(arrow_btn_obj, lv_color_hex(0x000000), 0);
	lv_obj_set_style_bg_opa(arrow_btn_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	lv_obj_set_style_bg_opa(arrow_btn_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_shadow_width(arrow_btn_obj, 0, 0); //去掉边框底部的阴影

	lv_obj_t *arrow_img = lv_img_create(arrow_btn_obj); //在容器里面创建图像
	lv_obj_set_align(arrow_img, LV_ALIGN_CENTER);
	divoom_menu_control_lv_img_set_src(arrow_img, "50/50_left_arrow.png");
	return arrow_btn_obj;
}

static void divoom_white_noise_menu_event_cb(lv_event_t * e)
{
    int voice_num, i = 0;
    lv_obj_t *cur_obj;
    Pdivoom_white_noise_menu_ctrl phandle = (Pdivoom_white_noise_menu_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target(e);
    voice_num = sizeof(divoom_white_noise_img_list)/sizeof(divoom_white_noise_img_list[0]);

	if (code == LV_EVENT_CLICKED)
	{
		if (cur_obj == phandle->arrow_btn_obj)
		{
			//退出
			divoom_menu_control_return_pre();
		}
		else if (cur_obj == phandle->btn[0])
        {
            if (phandle->base_info->on_off == 0)
            {
                phandle->base_info->on_off = 1;
				divoom_white_noise_start();
                divoom_menu_control_lv_img_set_src(phandle->img[0], "50/50_switch_on2.png");
				divoom_white_noise_menu_set_enter_flag(1);
            }
			else if (phandle->base_info->on_off == 1)
			{
				phandle->base_info->on_off = 0;
				divoom_white_noise_stop();
				divoom_menu_control_lv_img_set_src(phandle->img[0], "50/50_switch_off2.png");
				divoom_white_noise_menu_set_enter_flag(1);
			}
        }
        else if(cur_obj == phandle->btn[1])
        {
            //时间页面
            divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_WHITE_NOISE_EDIT_TIME, phandle->base_info, 0, 0);
        }
	}
	else if (code == LV_EVENT_VALUE_CHANGED)
	{
		for (i = 0; i < voice_num; i++)
		{
			if (cur_obj == phandle->volume_slide_obj[i])
			{
				int vaule = lv_slider_get_value(cur_obj);
				phandle->base_info->volume[i] = vaule;
				phandle->base_info->fresh_flag = 1;
				break;
			}
		}
	}
	else if (code == LV_EVENT_KEY)
	{
		char *key_info = (char *)lv_event_get_param(e);
		divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
	}
}

void divoom_white_noise_create_image_grad(Pdivoom_white_noise_menu_ctrl phandle)
{
    int i, voice_num;
    voice_num = sizeof(divoom_white_noise_img_list)/sizeof(divoom_white_noise_img_list[0]);

	//返回箭头
	phandle->arrow_btn_obj = divoom_white_noise_menu_set_return_arrow_obj(phandle->main_obj);
	lv_obj_add_event_cb(phandle->arrow_btn_obj, divoom_white_noise_menu_event_cb, LV_EVENT_CLICKED, phandle);	

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

    lv_style_init(&phandle->grid_type);
    lv_style_set_pad_all(&phandle->grid_type, 0);
    lv_style_set_pad_gap(&phandle->grid_type, 0);
    lv_style_set_border_width(&phandle->grid_type, 1);
    lv_style_set_border_side(&phandle->grid_type, LV_BORDER_SIDE_NONE);
    lv_style_set_bg_color(&phandle->grid_type, lv_color_hex(DIVOOM_MENNU_NORMAL_BG_COLOR));

	lv_obj_t *btn_obj = lv_obj_create(phandle->main_obj);
	lv_obj_set_x(btn_obj, 29);
	lv_obj_set_y(btn_obj, 209);
	lv_obj_set_width(btn_obj, 742);
	lv_obj_set_height(btn_obj, 142);
	lv_obj_set_style_pad_all(btn_obj, 0, 0);
	lv_obj_set_style_border_width(btn_obj, 1, 0);
	lv_obj_set_style_border_opa(btn_obj, LV_OPA_0, 0);
    lv_obj_set_style_radius(btn_obj, 28, 0);
	lv_obj_set_style_bg_color(btn_obj, lv_color_hex(0x1A1A1A), 0);
	lv_obj_set_style_bg_opa(btn_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	lv_obj_set_style_bg_opa(btn_obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_shadow_width(btn_obj, 0, 0); //去掉边框底部的阴影

	//创建两个btn
	for (i = 0; i < 2; i++)
	{
		phandle->btn[i] = lv_btn_create(btn_obj);
//		lv_obj_remove_style_all(phandle->btn[i]);

		if (i == 0)
		{
			lv_obj_set_x(phandle->btn[i], 24);
			lv_obj_set_y(phandle->btn[i], 34);
			lv_obj_set_width(phandle->btn[i], 133);
			lv_obj_set_height(phandle->btn[i], 76);
			lv_obj_set_style_border_width(phandle->btn[i], 1, 0);
			lv_obj_set_style_border_opa(phandle->btn[i], LV_OPA_0, 0);
			lv_obj_set_style_bg_opa(phandle->btn[i], LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
	    	lv_obj_set_style_bg_opa(phandle->btn[i], LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
			lv_obj_set_style_shadow_width(phandle->btn[i], 0, 0); //去掉边框底部的阴影
			lv_obj_set_style_bg_color(phandle->btn[i], lv_color_hex(0x1A1A1A), 0);

			phandle->img[i] = lv_img_create(phandle->btn[i]);
			lv_obj_set_align(phandle->img[i], LV_ALIGN_CENTER);
			int on_off = phandle->base_info->on_off;
			if (on_off)
			{
				divoom_menu_control_lv_img_set_src(phandle->img[i], "50/50_switch_on2.png");
			}
			else
			{
				divoom_menu_control_lv_img_set_src(phandle->img[i], "50/50_switch_off2.png");
			}
		}
		else
		{
			lv_obj_set_x(phandle->btn[i], 195);
			lv_obj_set_y(phandle->btn[i], 29);
			lv_obj_set_width(phandle->btn[i], 503);
			lv_obj_set_height(phandle->btn[i], 76);
			lv_obj_set_style_border_width(phandle->btn[i], 8, 0);
			lv_obj_set_style_border_opa(phandle->btn[i], LV_OPA_100, 0);
			lv_obj_set_style_radius(phandle->btn[i], 470, 0);
			lv_obj_set_style_bg_opa(phandle->btn[i], LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
			lv_obj_set_style_bg_opa(phandle->btn[i], LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
			lv_obj_set_style_shadow_width(phandle->btn[i], 0, 0); //去掉边框底部的阴影
			lv_obj_set_style_bg_color(phandle->btn[i], lv_color_hex(0x31333A), 0);
			lv_obj_set_style_border_color(phandle->btn[i], lv_color_hex(0xFFF4F7), 0);

			phandle->label_time = lv_label_create(phandle->btn[i]);
			lv_obj_set_style_text_font(phandle->label_time, divoom_menu_control_get_font_info(52, phandle->menu_id), 0);
			lv_obj_set_style_text_color(phandle->label_time, lv_color_hex(0XFFFFFF), 0);
			lv_obj_set_align(phandle->label_time, LV_ALIGN_CENTER);

			if (phandle->base_info->time > 0)
				lv_label_set_text_fmt(phandle->label_time, "%d min", phandle->base_info->time/60);
			else
				lv_label_set_text(phandle->label_time, divoom_menu_control_get_disp_str("Never"));

		}
		lv_obj_add_event_cb(phandle->btn[i], divoom_white_noise_menu_event_cb, LV_EVENT_CLICKED, phandle);
	}

	lv_style_init(&phandle->style_main);
	lv_style_set_bg_opa(&phandle->style_main, LV_OPA_COVER);
	lv_style_set_bg_color(&phandle->style_main, lv_color_hex(0x7D7D7D));
	lv_style_set_radius(&phandle->style_main, 502);
	lv_style_set_pad_ver(&phandle->style_main, -2); //设置指示器大小

	lv_style_init(&phandle->style_indicator);
	lv_style_set_bg_opa(&phandle->style_indicator, LV_OPA_COVER);
	lv_style_set_bg_color(&phandle->style_indicator, lv_color_hex(0xFFFFFF));
	lv_style_set_radius(&phandle->style_indicator, 502);

	lv_style_init(&phandle->style_knob);
	lv_style_set_bg_opa(&phandle->style_knob, LV_OPA_COVER);
	lv_style_set_bg_color(&phandle->style_knob, lv_color_hex(0xFFFFFF));
	lv_style_set_border_color(&phandle->style_knob, lv_color_hex(0xB3B3B3));
	lv_style_set_border_width(&phandle->style_knob, 7);
	lv_style_set_radius(&phandle->style_knob, 99);
	lv_style_set_pad_all(&phandle->style_knob, 16); //设置把手大小

    for(i = 0; i < voice_num; i++)
	{
        lv_obj_t * obj = lv_obj_create(phandle->main_obj);
        lv_obj_add_style(obj, &phandle->grid_type, 0);
		lv_obj_set_pos(obj, 29, 363 + i*(172 + 15));
        lv_obj_set_size(obj, 742, 172);
		lv_obj_set_style_border_opa(obj, LV_OPA_0, 0);
		lv_obj_set_style_radius(obj, 30, 0);
		lv_obj_set_style_bg_opa(obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT); //滚动条透明度设为0
		lv_obj_set_style_bg_opa(obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
		lv_obj_set_style_shadow_width(obj, 0, 0); //去掉边框底部的阴影
		lv_obj_set_style_bg_color(obj, lv_color_hex(0x1A1A1A), 0);

        lv_obj_t *img_obj = lv_img_create(obj);
        lv_obj_set_pos(img_obj, 25, 25);
        lv_obj_set_size(img_obj, 122, 122);
        divoom_menu_control_lv_img_set_src(img_obj, divoom_white_noise_img_list[i]);

		phandle->volume_slide_obj[i] = lv_slider_create(obj);
		lv_obj_remove_style_all(phandle->volume_slide_obj[i]);		
        lv_slider_set_range(phandle->volume_slide_obj[i], 0, 100);
        lv_obj_set_pos(phandle->volume_slide_obj[i], 188, 75);
        lv_obj_set_size(phandle->volume_slide_obj[i], 518, 20);
        lv_slider_set_value(phandle->volume_slide_obj[i], phandle->base_info->volume[i], LV_ANIM_ON);
		lv_obj_add_style(phandle->volume_slide_obj[i], &phandle->style_main, LV_PART_MAIN);
		lv_obj_add_style(phandle->volume_slide_obj[i], &phandle->style_indicator, LV_PART_INDICATOR);
		lv_obj_add_style(phandle->volume_slide_obj[i], &phandle->style_knob, LV_PART_KNOB);
		lv_obj_add_event_cb(phandle->volume_slide_obj[i], divoom_white_noise_menu_event_cb, LV_EVENT_VALUE_CHANGED, phandle);
	}
}

void *divoom_white_noise_menu_init(void *user_data, int menu_id)
{
    Pdivoom_white_noise_menu_ctrl phandle = divoom_system_malloc(sizeof(divoom_white_noise_menu_ctrl));
    phandle->menu_id = menu_id;
    phandle->cur_active_obj = NULL;
    phandle->base_info = (divoom_white_noise_info *)divoom_white_noise_get_base_info();
    phandle->base_info->on_off = 1;
    phandle->main_obj = lv_obj_create(NULL);
    lv_style_init(&phandle->backgroud_type);
    lv_style_set_bg_color(&phandle->backgroud_type, lv_color_hex(DIVOOM_MENNU_BACK_GROUD_COLOR));
    lv_style_set_text_color(&phandle->backgroud_type, lv_color_hex(DIVOOM_MENNU_TEXT_COLOR));
    lv_obj_add_style(phandle->main_obj, &phandle->backgroud_type, 0);
    lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    divoom_white_noise_create_image_grad(phandle);
	divoom_white_noise_start();
	Phandle = phandle;
	lv_obj_add_event_cb(phandle->main_obj, divoom_white_noise_menu_event_cb, LV_EVENT_KEY, phandle);
    return phandle;
}

void divoom_white_noise_menu_app_ctrl(uint8 ctrl, void *para)
{
	Pdivoom_white_noise_info noise_info = (divoom_white_noise_info *)divoom_white_noise_get_base_info();
	Pdivoom_white_noise_info noise_para = NULL;

	if (ctrl)
	{
		if (para)
		{
			noise_para = (Pdivoom_white_noise_info)para;
			if (noise_info->on_off != noise_para->on_off)
				noise_info->idle_time = 0;
			if (noise_para->on_off)
			{
//                divoom_light_app_ctrl_confirm_screen_status();
				if (Phandle)
					divoom_menu_control_lv_img_set_src(Phandle->img[0], "50/50_switch_on2.png");
				if (divoom_mix_pcm_get_play_stat() == 0)
				{
					divoom_white_noise_start();
					divoom_white_noise_menu_set_enter_flag(1);
				}
			}
			else
			{
				if (Phandle)
					divoom_menu_control_lv_img_set_src(Phandle->img[0], "50/50_switch_off2.png");
				if (divoom_mix_pcm_get_play_stat())
				{
					divoom_white_noise_stop();
					divoom_white_noise_menu_set_enter_flag(1);
				}
			}

			if (noise_info->power_off != noise_para->power_off)
				noise_info->idle_time = 0;
			if (noise_info->time != noise_para->time)
				noise_info->idle_time = 0;

			noise_info->on_off = noise_para->on_off;
			noise_info->power_off = noise_para->power_off;
			noise_info->time = noise_para->time;
			memcpy(noise_info->volume, noise_para->volume, sizeof(noise_para->volume));

			if (Phandle)
			{
				if (noise_info->time > 0)
					lv_label_set_text_fmt(Phandle->label_time, "%d min", noise_info->time/60);
				else
					lv_label_set_text(Phandle->label_time, divoom_menu_control_get_disp_str("Never"));

				int i = 0, volume = 0;
				for (i = 0; i < 8; i++)
				{
	    			volume = noise_info->volume[i];
	                if(volume >= 100)
	                {
	                    volume = 100;
	                }
	                lv_slider_set_value(Phandle->volume_slide_obj[i], volume, LV_ANIM_ON);
				}
			}
		}
	}
	else
	{
		Pdivoom_white_noise_info send_noise = (Pdivoom_white_noise_info)divoom_system_malloc(sizeof(divoom_white_noise_info));

		if (divoom_mix_pcm_get_play_stat())
			noise_info->on_off = 1;
		else
			noise_info->on_off = 0;

		memcpy(send_noise, noise_info, sizeof(divoom_white_noise_info));
    	divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_WHITE_NOISE, MQTT_SER, send_noise);
	}
}

void divoom_white_noise_menu_loop(void)
{
	Pdivoom_white_noise_info noise_info = (divoom_white_noise_info *)divoom_white_noise_get_base_info();
	uint8 event_id = 0, mode_id = 0;
	mode_id = divoom_app_schedule_get_active_model(&event_id, DIVOOM_APP_SCHEDULE_RESOURCCE_RGB);

	if (noise_info->on_off == 0 || divoom_mix_pcm_get_play_stat() == 0)
		return;

	if (noise_info->power_off == 0) //结束后关机
	{
		if (noise_info->time)
		{
			noise_info->idle_time++;
            divoom_print("%s: idle time: %d, poweroff time: %d\n",__func__, (int)noise_info->idle_time, (int)noise_info->time);
			if (noise_info->idle_time >= noise_info->time)
			{
				divoom_print("%s: the system is auto power off beacuse off idle time: %d\n", __func__, noise_info->idle_time);
            	noise_info->on_off = 0; //关闭
				noise_info->idle_time = 0;
//				if (mode_id == DIVOOM_MODEL_APP_ALARM && event_id == DIVOOM_ALARM_SCHEDULE_ALARM_ACTIVE) //闹钟状态下不关机
//					return;
//				divoom_os_com_system_power_off();
			}
		}
	}
	else //结束后不关机
	{
		if (noise_info->time) //时间到退出
		{
			noise_info->idle_time++;
            divoom_print("%s: idle time: %d, exit time: %d\n",__func__, (int)noise_info->idle_time, (int)noise_info->time);
			if (noise_info->idle_time >= noise_info->time)
			{
				noise_info->on_off = 0;
				noise_info->idle_time = 0;
				divoom_print("%s: white noise exit beacuse off idle time: %d\n", __func__, noise_info->idle_time);
				divoom_white_noise_stop();
			}
		}
	}
}

void divoom_white_noise_menu_disp(void *handle, void *active)
{
    Pdivoom_white_noise_menu_ctrl phandle = (Pdivoom_white_noise_menu_ctrl)handle;

    lv_scr_load(phandle->main_obj);

	if (phandle->base_info->on_off )
    {
        divoom_menu_control_lv_img_set_src(phandle->img[0], "50/50_switch_on2.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(phandle->img[0], "50/50_switch_off2.png");
    }
	
	if (phandle->base_info->time > 0)
		lv_label_set_text_fmt(phandle->label_time, "%d min", phandle->base_info->time/60);
	else
		lv_label_set_text(phandle->label_time, divoom_menu_control_get_disp_str("Never"));
}

void divoom_white_noise_menu_clean(void *handle)
{
    Pdivoom_white_noise_menu_ctrl phandle = (Pdivoom_white_noise_menu_ctrl)handle;

	Phandle = NULL;
    lv_obj_del(phandle->main_obj);
	lv_style_reset(&phandle->backgroud_type);
	lv_style_reset(&phandle->grid_type);
    divoom_system_free(phandle);
}


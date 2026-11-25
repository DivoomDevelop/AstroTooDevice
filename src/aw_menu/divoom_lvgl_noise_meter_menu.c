#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"


typedef struct _divoom_lvgl_noise_meter_menu_ctrl
{
    int menu_id;
	lv_style_t backgroud_type;
	void *main_obj;
	void *btn_obj;

	void *knob_obj;
	void *db_label_obj;

	lv_timer_t * noise_refresh_time;
	
	void *cur_active_obj;
}divoom_lvgl_noise_meter_menu_ctrl, *Pdivoom_lvgl_noise_meter_menu_ctrl;


int divoom_lvgl_noise_meter_menu_get_value(void)
{
	return divoom_eq_noise_mic_fft_get_noise_value();
}


int divoom_lvgl_noise_meter_menu_calculate_angle(int db_value)
{
	//根据分贝数计算旋转角度
	int angle;
	
	if(db_value < 50 && db_value >= 0)
	{
		//125度有50分贝
		angle = -1250 + db_value * 1250 / 50;
	}
	else if(db_value >= 50 && db_value < 100)
	{
		angle = -1250 + db_value * 1250 / 50;
	}
	else if(db_value < 0)
	{
		angle = -1250;
	}
	else if(db_value >= 100)
	{
		angle = 1250;
	}
	else
	{
		angle = -1250;
	}

	return angle;
}

static void divoom_lvgl_noise_meter_menu_event_cb(lv_event_t * e)
{
    lv_obj_t *cur_obj;
    Pdivoom_lvgl_noise_meter_menu_ctrl phandle = (Pdivoom_lvgl_noise_meter_menu_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target(e);

    if(code == LV_EVENT_KEY)
    {
        char c = *((char *)lv_event_get_param(e));

        if(c == DIVOOM_LVGL_KEY_NEXT)
		{
        }
        else if(c == DIVOOM_LVGL_KEY_PRE)
		{
        }
        else if(c == DIVOOM_LVGL_KEY_ENTER)
        {
        }
        else if(c == DIVOOM_LVGL_KEY_ESC)
        {
            //将退出所有菜单
			//int event_id = 0, mode_id = 0;
			//mode_id = divoom_app_schedule_get_active_model(&event_id);
			//if (mode_id == DIVOOM_MODEL_APP_TOOL && event_id == DIVOOM_APP_TOOL_EVENT_PLAY)
			//	divoom_tool_noise_menu_cancel_disp();
			//else
				divoom_menu_control_return_pre();
        }
        else
		{
			char *key_info = (char *)lv_event_get_param(e);
			divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
            return;
        }
    }
}

void divoom_lvgl_noise_meter_menu_refresh(lv_timer_t * tool_refresh_time)
{
	//定时刷新噪音值
	int value, angle;
	static int db = 0;
	Pdivoom_lvgl_noise_meter_menu_ctrl phandle = lv_timer_get_user_data(tool_refresh_time);

	value = divoom_lvgl_noise_meter_menu_get_value();
	lv_label_set_text_fmt(phandle->db_label_obj, "%02d", value);
	
	angle = divoom_lvgl_noise_meter_menu_calculate_angle(value);
	lv_img_set_angle(phandle->knob_obj, angle);
}

void divoom_lvgl_noise_meter_menu_set_zoom_cb(void *obj, int32_t zoom)
{
	lv_obj_t *img_obj = (lv_obj_t *)obj;

	lv_img_set_zoom(img_obj, zoom);
}


void divoom_lvgl_noise_meter_menu_create_time_area(Pdivoom_lvgl_noise_meter_menu_ctrl phandle)
{
	int angle, value;

	//lv_obj_t *bg_img = lv_img_create(phandle->main_obj);
	//divoom_menu_control_lv_img_set_src(bg_img, "32/32_background.png");
	//lv_obj_set_pos(bg_img, 82, 159);

	phandle->btn_obj = lv_btn_create(phandle->main_obj);
	lv_obj_set_style_pad_all(phandle->btn_obj, 0, 0);
	lv_obj_set_style_radius(phandle->btn_obj, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_bg_color(phandle->btn_obj, lv_color_hex(0xF2F2F2), 0);
	lv_obj_set_style_border_width(phandle->btn_obj, 0, 0);
	lv_obj_set_style_shadow_width(phandle->btn_obj, 0, 0);
	lv_obj_set_style_outline_width(phandle->btn_obj, 0, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_size(phandle->btn_obj, 636, 636);
	lv_obj_set_pos(phandle->btn_obj, 82, 159);
	lv_obj_add_event_cb(phandle->btn_obj, divoom_lvgl_noise_meter_menu_event_cb, LV_EVENT_KEY, phandle);

	if(phandle->cur_active_obj == NULL)
	{
		phandle->cur_active_obj = phandle->btn_obj;		
	}

	lv_obj_t *color_arc_img = lv_img_create(phandle->btn_obj);
	divoom_menu_control_lv_img_set_src(color_arc_img, "32/32_color_arc.png");
	lv_obj_set_pos(color_arc_img, 18, 18);

	lv_obj_t *black_arc_img = lv_img_create(phandle->btn_obj);
	divoom_menu_control_lv_img_set_src(black_arc_img, "32/32_arc_black.png");
	lv_obj_set_pos(black_arc_img, 18, 18);

	lv_obj_t *white_arc_img = lv_img_create(phandle->btn_obj);
	divoom_menu_control_lv_img_set_src(white_arc_img, "32/32_arc_white.png");
	lv_obj_set_pos(white_arc_img, 18, 18);

	lv_obj_t *arc_img = lv_img_create(phandle->btn_obj);
	divoom_menu_control_lv_img_set_src(arc_img, "32/32_db.png");
	lv_obj_set_pos(arc_img, 114, 125);

	//分贝指针图
	lv_obj_t *rect_knob_img = lv_img_create(phandle->btn_obj);
	divoom_menu_control_lv_img_set_src(rect_knob_img, "32/32_knob.png");
	lv_img_set_pivot(rect_knob_img, 5, 274);
	lv_obj_set_pos(rect_knob_img, 312, 52);
	phandle->knob_obj = rect_knob_img;

	value = divoom_lvgl_noise_meter_menu_get_value();
	angle = divoom_lvgl_noise_meter_menu_calculate_angle(value);
	lv_img_set_angle(phandle->knob_obj, angle);

	lv_obj_t *arc_knob_img = lv_img_create(phandle->btn_obj);
	divoom_menu_control_lv_img_set_src(arc_knob_img, "32/32_arc_knob.png");
	lv_obj_set_pos(arc_knob_img, 201, 202);

	//分贝标签
	lv_obj_t *db_disp_label = lv_label_create(phandle->btn_obj);
	lv_obj_set_style_text_font(db_disp_label, divoom_menu_control_get_font_info(100, phandle->menu_id), 0);
	lv_obj_set_style_text_color(db_disp_label, lv_color_black(), 0);
	lv_obj_set_style_text_align(db_disp_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_pos(db_disp_label, 230, 221+15);
	lv_obj_set_size(db_disp_label, 176, 145);
	lv_label_set_text_fmt(db_disp_label, "%02d", divoom_lvgl_noise_meter_menu_get_value());
	phandle->db_label_obj = db_disp_label;
	

	//分贝单位底图
	lv_obj_t *db_bg_img = lv_img_create(phandle->btn_obj);
	divoom_menu_control_lv_img_set_src(db_bg_img, "32/32_db_bg.png");
	lv_obj_set_pos(db_bg_img, 221, 365);

	//单位
	lv_obj_t *db_label = lv_label_create(phandle->btn_obj);
	lv_obj_set_style_text_font(db_label, divoom_menu_control_get_font_info(39, phandle->menu_id), 0);
	lv_obj_set_style_text_color(db_label, lv_color_hex(0x333333), 0);
	lv_obj_set_style_text_align(db_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_pos(db_label, 287, 365+5);
	lv_obj_set_size(db_label, 57, 57);
	lv_label_set_text(db_label, divoom_menu_control_get_disp_str("dB"));

	lv_obj_t *img_obj1 = lv_img_create(phandle->btn_obj);
	divoom_menu_control_lv_img_set_src(img_obj1, "32/32_rect.png");
	lv_obj_set_pos(img_obj1, 246, 552);
	
	lv_obj_t *img_obj2 = lv_img_create(phandle->main_obj);
	divoom_menu_control_lv_img_set_src(img_obj2, "32/32_db_disp.png");
	lv_obj_set_pos(img_obj2, 21-3, 927-3);

	lv_anim_t a;
	lv_anim_init(&a);
	lv_anim_set_var(&a, img_obj2);
	lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
	lv_anim_set_exec_cb(&a, divoom_lvgl_noise_meter_menu_set_zoom_cb);
	lv_anim_set_values(&a, 230, 256);
	lv_anim_set_time(&a, 450);
	lv_anim_set_playback_time(&a, 450);
	lv_anim_start(&a);

	phandle->noise_refresh_time = lv_timer_create(divoom_lvgl_noise_meter_menu_refresh, 100, phandle);
	
}


void *divoom_lvgl_noise_meter_menu_init(void *user_data, int menu_id)
{
	Pdivoom_lvgl_noise_meter_menu_ctrl phandle = (Pdivoom_lvgl_noise_meter_menu_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_noise_meter_menu_ctrl));

	if(phandle == NULL)
	{
		return NULL;
	}

	memset(phandle, 0, sizeof(divoom_lvgl_noise_meter_menu_ctrl));
	
	phandle->main_obj = lv_obj_create(NULL);
	phandle->menu_id = menu_id;
	phandle->cur_active_obj = NULL;
	
	lv_style_init(&phandle->backgroud_type);
	lv_style_set_bg_color(&phandle->backgroud_type, lv_color_hex(0xE4E4E4));
	lv_style_set_text_color(&phandle->backgroud_type, lv_color_hex(0x0));
	lv_obj_add_style(phandle->main_obj, &phandle->backgroud_type, 0);
	lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(phandle->main_obj, 0, 0);

	

	divoom_lvgl_noise_meter_menu_create_time_area(phandle);


	return phandle;
}


void divoom_lvgl_noise_meter_menu_disp(void *handle, void *active)
{
	Pdivoom_lvgl_noise_meter_menu_ctrl phandle = (Pdivoom_lvgl_noise_meter_menu_ctrl)handle;

	if(phandle->cur_active_obj)
    {
        lv_group_focus_obj(phandle->cur_active_obj);
        phandle->cur_active_obj = NULL;
    }
    if(active)
    {
        lv_group_focus_obj(active);
    }
	lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_noise_meter_menu_exit(void *handle)
{
	Pdivoom_lvgl_noise_meter_menu_ctrl phandle = (Pdivoom_lvgl_noise_meter_menu_ctrl)handle;

	lv_timer_del(phandle->noise_refresh_time);
	lv_style_reset(&phandle->backgroud_type);
	lv_obj_del(phandle->main_obj);
	divoom_system_free(phandle);
}


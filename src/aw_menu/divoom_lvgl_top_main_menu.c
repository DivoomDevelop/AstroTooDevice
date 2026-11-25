#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_top_main_menu.h"
#include "divoom_pixel_format.h"

#ifdef DIVOOM_TIME_FRAME_SPEAKER

/* 歌词入口开关：0=屏蔽主菜单歌词选项（歌词以后放到表盘里显示），1=显示 */
#define DIVOOM_TOP_MAIN_SHOW_LYRIC  0

#if 0  //音箱矩形菜单版本
typedef struct _divoom_lvgl_top_main_menu_ctrl
{
	void *main_obj;		
	void *cout_obj;
	void *label_obj;

	int select_id;
	int radius;
	int center_x;
	int center_y;

	int anim_end_flag;			//动画结束标志

	void *select_img[10];
	void *disp_img[10];
	void *disp_label[10];
	//Pdivoom_lvgl_top_main_menu_image_ctrl pdisp_handle[10];

	int exit_time;
	lv_timer_t *menu_exit_timer;
}divoom_lvgl_top_main_menu_ctrl, *Pdivoom_lvgl_top_main_menu_ctrl;

Pdivoom_lvgl_top_main_menu_ctrl pdivoom_lvgl_top_main_menu_handle = NULL;	
	
const char *divoom_top_main_mode_str[] = {
	"My Clocks2",
	"Lyric2",
	"WhiteNoise2",
	"AlbumManage2",
	"Alarm2",
	"Calendar2",
	"Tool Box2",
	"AudioSource2",
	"Settings2",
};


const char *divoom_top_main_img_list[] = {
	"75/75_top_dial2.png",
	"75/75_top_lyric2.png",
	"75/75_top_whitenoise2.png",
	"75/75_top_album2.png",
	"75/75_top_alarm2.png",
	"75/75_top_calendar2.png",
	"75/75_top_tool2.png",
	"75/75_top_audio2.png",
	"75/75_top_setting2.png",
};

const int divoom_top_main_next_menu_id[] =
{
	DIVOOM_MENU_CONTROL_ID_DIAL_PREVIEW,
	DIVOOM_MENU_CONTROL_ID_LYRIC_MENU,
	DIVOOM_MENU_CONTROL_ID_WHITE_NOISE_PLAY_MAIN,
	DIVOOM_MENU_CONTROL_ID_SELECT_ALBUM_PLAY,	
	DIVOOM_MENU_CONTROL_ID_ALARM_LIST_MENU,
	DIVOOM_MENU_CONTROL_ID_CALENDAR_LIST_MENU,
	DIVOOM_MENU_CONTROL_ID_TOOL,
	DIVOOM_MENU_CONTROL_ID_AUDIO_SOURCE_MENU,
	DIVOOM_MENU_CONTROL_ID_SYSTEM_SETING_MENU,
};

void divoom_lvgl_top_main_menu_del_menu(void)
{
	if(pdivoom_lvgl_top_main_menu_handle->menu_exit_timer)
	{
		lv_timer_del(pdivoom_lvgl_top_main_menu_handle->menu_exit_timer);
	}
    divoom_lvgl_top_control_remove_top(pdivoom_lvgl_top_main_menu_handle);
	lv_obj_del_async(pdivoom_lvgl_top_main_menu_handle->main_obj);
	divoom_system_free(pdivoom_lvgl_top_main_menu_handle);
        divoom_local_movie_play_ctrl(1);
	pdivoom_lvgl_top_main_menu_handle = NULL;
}

void divoom_lvgl_top_main_menu_time_handle(lv_timer_t *exit_timer)
{
	if(pdivoom_lvgl_top_main_menu_handle != NULL)
	{
		pdivoom_lvgl_top_main_menu_handle->exit_time++;

		if(pdivoom_lvgl_top_main_menu_handle->exit_time >= DIVOOM_EXIT_MENU_TIME_OUT)//主菜单不变动自动退出
		{
			if(pdivoom_lvgl_top_main_menu_handle->anim_end_flag == 1)
			{
				divoom_lvgl_top_main_menu_del_menu();
			}
		}
	}
}

static void divoom_lvgl_top_main_menu_event_cb(lv_event_t * e)
{
	int i;
    lv_event_code_t code = lv_event_get_code(e);
	
	if(code == LV_EVENT_KEY)
    {
        char *key_info = (char *)lv_event_get_param(e);
        char c = key_info[0];

        if(pdivoom_lvgl_top_main_menu_handle == NULL)
        {
            divoom_print("aj test!\n");
			return;
        }

		pdivoom_lvgl_top_main_menu_handle->exit_time = 0;

		if(c == DIVOOM_LVGL_KEY_PRE) 
		{
			pdivoom_lvgl_top_main_menu_handle->select_id ++;
   			if(pdivoom_lvgl_top_main_menu_handle->select_id >= sizeof(divoom_top_main_img_list)/sizeof(divoom_top_main_img_list[0]))
    		{
        		pdivoom_lvgl_top_main_menu_handle->select_id = 0;
    		}
						
			for(i = 0; i < 9; i++)
			{
				if(pdivoom_lvgl_top_main_menu_handle->select_id == i)
				{
					lv_obj_set_style_img_recolor(pdivoom_lvgl_top_main_menu_handle->disp_img[i], lv_color_hex(0x0), 0);
					lv_obj_set_style_text_color(pdivoom_lvgl_top_main_menu_handle->disp_label[i], lv_color_hex(0x0), 0);
					lv_obj_clear_flag(pdivoom_lvgl_top_main_menu_handle->select_img[i], LV_OBJ_FLAG_HIDDEN);
				}
				else
				{
					lv_obj_set_style_img_recolor(pdivoom_lvgl_top_main_menu_handle->disp_img[i], lv_color_hex(0x838383), 0);
					lv_obj_set_style_text_color(pdivoom_lvgl_top_main_menu_handle->disp_label[i], lv_color_hex(0x838383), 0);
					lv_obj_add_flag(pdivoom_lvgl_top_main_menu_handle->select_img[i], LV_OBJ_FLAG_HIDDEN);
				}
			}

            //divoom_lvgl_top_main_menu_angle_pre_anim();
        }
		else if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			if(pdivoom_lvgl_top_main_menu_handle->select_id == 0)
    		{
        		pdivoom_lvgl_top_main_menu_handle->select_id = sizeof(divoom_top_main_img_list)/sizeof(divoom_top_main_img_list[0]) - 1;
    		}
    		else
    		{
        		pdivoom_lvgl_top_main_menu_handle->select_id --;
    		}

			for(i = 0; i < 9; i++)
			{
				if(pdivoom_lvgl_top_main_menu_handle->select_id == i)
				{
					lv_obj_set_style_img_recolor(pdivoom_lvgl_top_main_menu_handle->disp_img[i], lv_color_hex(0x0), 0);
					lv_obj_set_style_text_color(pdivoom_lvgl_top_main_menu_handle->disp_label[i], lv_color_hex(0x0), 0);
					lv_obj_clear_flag(pdivoom_lvgl_top_main_menu_handle->select_img[i], LV_OBJ_FLAG_HIDDEN);
				}
				else
				{
					lv_obj_set_style_img_recolor(pdivoom_lvgl_top_main_menu_handle->disp_img[i], lv_color_hex(0x838383), 0);
					lv_obj_set_style_text_color(pdivoom_lvgl_top_main_menu_handle->disp_label[i], lv_color_hex(0x838383), 0);
					lv_obj_add_flag(pdivoom_lvgl_top_main_menu_handle->select_img[i], LV_OBJ_FLAG_HIDDEN);
				}
			}

			//divoom_lvgl_top_main_menu_angle_next_anim();
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{	
			int select_id = pdivoom_lvgl_top_main_menu_handle->select_id;
			divoom_lvgl_top_main_menu_del_menu();
            if(divoom_top_main_next_menu_id[select_id] == DIVOOM_MENU_CONTROL_ID_SYSTEM_SETING_MENU)
            {
                divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_ENTER_MENU_SETTING);
            }
            else
            {
			    divoom_menu_control_clearall_load(divoom_top_main_next_menu_id[select_id], NULL, 0, 0);
            }
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_lvgl_top_main_menu_del_menu();
		}
        else if(c == DIVOOM_LVGL_KEY_HOME)
        {
            divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
        }
    }
	
}

void divoom_lvgl_top_main_menu_create_icon(void)
{
	int i, x, y, zoom, angle, opa;
	
	pdivoom_lvgl_top_main_menu_handle->cout_obj = lv_btn_create(pdivoom_lvgl_top_main_menu_handle->main_obj);

	lv_obj_remove_style_all(pdivoom_lvgl_top_main_menu_handle->cout_obj);
	lv_obj_set_style_pad_all(pdivoom_lvgl_top_main_menu_handle->cout_obj, 0, 0);
	lv_obj_set_style_border_width(pdivoom_lvgl_top_main_menu_handle->cout_obj, 0, 0);
	lv_obj_set_style_outline_width(pdivoom_lvgl_top_main_menu_handle->cout_obj, 0, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_shadow_width(pdivoom_lvgl_top_main_menu_handle->cout_obj, 0, 0);
	lv_obj_set_style_bg_opa(pdivoom_lvgl_top_main_menu_handle->cout_obj, LV_OPA_0, 0);
	lv_obj_set_pos(pdivoom_lvgl_top_main_menu_handle->cout_obj, 0, 0);
    lv_obj_set_size(pdivoom_lvgl_top_main_menu_handle->cout_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_add_event_cb(pdivoom_lvgl_top_main_menu_handle->cout_obj, divoom_lvgl_top_main_menu_event_cb, LV_EVENT_KEY, NULL);

	lv_obj_t *bg_img = lv_img_create(pdivoom_lvgl_top_main_menu_handle->cout_obj);
	divoom_menu_control_lv_img_set_src(bg_img, "75/75_top_rect_bg.png");
	//lv_obj_set_style_img_opa(bg_img, 85*LV_OPA_100/100, 0);
	lv_obj_set_pos(bg_img, 21-15, 175-15);

	for(i = 0;i < 9; i++)
	{
		pdivoom_lvgl_top_main_menu_handle->select_img[i] = lv_img_create(pdivoom_lvgl_top_main_menu_handle->cout_obj);
		divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_main_menu_handle->select_img[i], "75/75_top_rect_select.png");
		lv_obj_add_flag(pdivoom_lvgl_top_main_menu_handle->select_img[i], LV_OBJ_FLAG_HIDDEN);

		lv_obj_set_size(pdivoom_lvgl_top_main_menu_handle->select_img[i], 302, 122);
		lv_obj_set_pos(pdivoom_lvgl_top_main_menu_handle->select_img[i], 31-6, (185-6) + i * 110);
		
		//
		pdivoom_lvgl_top_main_menu_handle->disp_img[i] = lv_img_create(pdivoom_lvgl_top_main_menu_handle->cout_obj);
		divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_main_menu_handle->disp_img[i], divoom_top_main_img_list[i]);
		
		lv_obj_set_style_img_recolor(pdivoom_lvgl_top_main_menu_handle->disp_img[i], lv_color_hex(0x838383), 0);
		lv_obj_set_style_img_recolor_opa(pdivoom_lvgl_top_main_menu_handle->disp_img[i], LV_OPA_100, 0);
		lv_obj_set_size(pdivoom_lvgl_top_main_menu_handle->disp_img[i], 56, 56);
		lv_obj_set_pos(pdivoom_lvgl_top_main_menu_handle->disp_img[i], 66, 212+110*i);

		//
		pdivoom_lvgl_top_main_menu_handle->disp_label[i] = lv_label_create(pdivoom_lvgl_top_main_menu_handle->cout_obj);
		
		lv_obj_set_style_text_font(pdivoom_lvgl_top_main_menu_handle->disp_label[i], divoom_menu_control_get_font_info(25, DIVOOM_MENU_CONTROL_ID_MAIN), 0);
		lv_obj_set_style_text_color(pdivoom_lvgl_top_main_menu_handle->disp_label[i], lv_color_hex(0x838383), 0);
		lv_obj_set_style_text_align(pdivoom_lvgl_top_main_menu_handle->disp_label[i], LV_TEXT_ALIGN_LEFT, 0);
		lv_label_set_long_mode(pdivoom_lvgl_top_main_menu_handle->disp_label[i], LV_LABEL_LONG_SCROLL_CIRCULAR);
		lv_obj_set_size(pdivoom_lvgl_top_main_menu_handle->disp_label[i], 149+20, 56);
		lv_obj_set_pos(pdivoom_lvgl_top_main_menu_handle->disp_label[i], 142, 212+10 + 110*i);
		lv_label_set_text(pdivoom_lvgl_top_main_menu_handle->disp_label[i], divoom_menu_control_get_disp_str(divoom_top_main_mode_str[i]));

		if(pdivoom_lvgl_top_main_menu_handle->select_id == i)
		{
			lv_obj_set_style_img_recolor(pdivoom_lvgl_top_main_menu_handle->disp_img[i], lv_color_hex(0x0), 0);
			lv_obj_clear_flag(pdivoom_lvgl_top_main_menu_handle->select_img[i], LV_OBJ_FLAG_HIDDEN);
			lv_obj_set_style_text_color(pdivoom_lvgl_top_main_menu_handle->disp_label[i], lv_color_black(), 0);
			
		}
	}
	
}

void divoom_lvgl_top_main_menu_init(void)
{
	divoom_get_lvgl_lock();

	if(pdivoom_lvgl_top_main_menu_handle != NULL)
	{
		divoom_free_lvgl_lock();
		return;
	}

	pdivoom_lvgl_top_main_menu_handle = (Pdivoom_lvgl_top_main_menu_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_top_main_menu_ctrl));

	if(pdivoom_lvgl_top_main_menu_handle == NULL)
	{
		divoom_free_lvgl_lock();
		return;
	}
    divoom_local_movie_play_ctrl(0);
	memset(pdivoom_lvgl_top_main_menu_handle, 0, sizeof(divoom_lvgl_top_main_menu_ctrl));

	pdivoom_lvgl_top_main_menu_handle->center_x = 400;
	pdivoom_lvgl_top_main_menu_handle->center_y = 1367;
	pdivoom_lvgl_top_main_menu_handle->radius = 370;
	pdivoom_lvgl_top_main_menu_handle->select_id = 0;
	pdivoom_lvgl_top_main_menu_handle->anim_end_flag = 1;
	pdivoom_lvgl_top_main_menu_handle->exit_time = 0;
	
	pdivoom_lvgl_top_main_menu_handle->main_obj = lv_obj_create(lv_layer_top()); //在顶层创建主页面

	lv_obj_set_style_bg_opa(pdivoom_lvgl_top_main_menu_handle->main_obj, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(pdivoom_lvgl_top_main_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_border_width(pdivoom_lvgl_top_main_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_shadow_width(pdivoom_lvgl_top_main_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_outline_width(pdivoom_lvgl_top_main_menu_handle->main_obj, 0, 0);
	lv_obj_clear_flag(pdivoom_lvgl_top_main_menu_handle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(pdivoom_lvgl_top_main_menu_handle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(pdivoom_lvgl_top_main_menu_handle->main_obj, 0, 0);
	
	divoom_lvgl_top_main_menu_create_icon();

	pdivoom_lvgl_top_main_menu_handle->menu_exit_timer = lv_timer_create(divoom_lvgl_top_main_menu_time_handle, 1000, NULL);
	
	divoom_lvgl_top_control_add_top(pdivoom_lvgl_top_main_menu_handle, pdivoom_lvgl_top_main_menu_handle->cout_obj);
	divoom_free_lvgl_lock();
}


void divoom_lvgl_top_main_menu_exit(void)
{
	if(pdivoom_lvgl_top_main_menu_handle != NULL)
	{
		if(pdivoom_lvgl_top_main_menu_handle->menu_exit_timer)
		{
			lv_timer_del(pdivoom_lvgl_top_main_menu_handle->menu_exit_timer);
		}
        divoom_lvgl_top_control_remove_top(pdivoom_lvgl_top_main_menu_handle);
		lv_obj_del_async(pdivoom_lvgl_top_main_menu_handle->main_obj);
		divoom_system_free(pdivoom_lvgl_top_main_menu_handle);
        divoom_local_movie_play_ctrl(1);
		pdivoom_lvgl_top_main_menu_handle = NULL;
	}
}	

#else //音箱圆圈菜单版本

#define DIVOOM_TOP_MAIN_MENU_ZOOM_VALUE_60  154
#define DIVOOM_TOP_MAIN_MENU_ZOOM_VALUE_70  179
#define DIVOOM_TOP_MAIN_MENU_ZOOM_VALUE_80	205

typedef struct _divoom_lvgl_top_main_menu_ctrl
{
	void *main_obj;		
	void *cout_obj;
	void *label_obj;

	int select_id;
	int radius;
	int center_x;
	int center_y;

	int anim_end_flag;			//动画结束标志
	int pre_circle_angle[7];	//转换前角度
	int circle_angle[7];		//相对于右边水平半径的夹角
	void *select_img[7];
	//Pdivoom_lvgl_top_main_menu_image_ctrl pdisp_handle[10];

	int exit_time;
	lv_timer_t *menu_exit_timer;
}divoom_lvgl_top_main_menu_ctrl, *Pdivoom_lvgl_top_main_menu_ctrl;

Pdivoom_lvgl_top_main_menu_ctrl pdivoom_lvgl_top_main_menu_handle = NULL;

const char *divoom_top_main_mode_str[] = {
	"Tool Box2",
	"Settings2",
	"AudioSource2",
	"Alarm2",
	"WhiteNoise2",
	"My Clocks2",
#if DIVOOM_TOP_MAIN_SHOW_LYRIC
	"Lyric2",
#endif
	"AlbumManage2",
	"Calendar2",
};


const char *divoom_top_main_img_list[] = {
	"75/75_top_glass_tool.png",
	"75/75_top_glass_setting.png",
	"75/75_top_glass_audio.png",
	"75/75_top_glass_alarm.png",
	"75/75_top_glass_whitenoise.png",
	"75/75_top_glass_dial.png",
#if DIVOOM_TOP_MAIN_SHOW_LYRIC
	"75/75_top_glass_lyric.png",
#endif
	"75/75_top_glass_album.png",
	"75/75_top_glass_calendar.png",
};

const int divoom_top_main_next_menu_id[] =
{
	DIVOOM_MENU_CONTROL_ID_TOOL,
	DIVOOM_MENU_CONTROL_ID_SYSTEM_SETING_MENU,
	DIVOOM_MENU_CONTROL_ID_AUDIO_SOURCE_MENU,
	DIVOOM_MENU_CONTROL_ID_ALARM_LIST_MENU,
	DIVOOM_MENU_CONTROL_ID_SLEEP_AID,
	DIVOOM_MENU_CONTROL_ID_DIAL_PREVIEW2,
#if DIVOOM_TOP_MAIN_SHOW_LYRIC
	DIVOOM_MENU_CONTROL_ID_LYRIC_MENU,
#endif
	DIVOOM_MENU_CONTROL_ID_SELECT_ALBUM_PLAY,	
	DIVOOM_MENU_CONTROL_ID_CALENDAR_LIST_MENU,
};

#if DIVOOM_TOP_MAIN_SHOW_LYRIC
#define DIVOOM_TOP_MAIN_MENU_COUNT  9
#else
#define DIVOOM_TOP_MAIN_MENU_COUNT  8
#endif
#define DIVOOM_TOP_MAIN_ICON_FIRST  1
#define DIVOOM_TOP_MAIN_ICON_LAST   6

/* slot 3 对应当前选中项；select_id==2 时与原先 list[i-1]/i==6->list[5] 一致 */
static int divoom_lvgl_top_main_menu_slot_to_img_idx(int slot)
{
	const int delta[7] = { 0, -2, -1, 0, 1, 2, 3 };
	int sel, idx;

	if (pdivoom_lvgl_top_main_menu_handle == NULL || slot < DIVOOM_TOP_MAIN_ICON_FIRST || slot > DIVOOM_TOP_MAIN_ICON_LAST)
	{
		return 0;
	}

	sel = pdivoom_lvgl_top_main_menu_handle->select_id;
	idx = sel + delta[slot];

	while (idx < 0)
	{
		idx += DIVOOM_TOP_MAIN_MENU_COUNT;
	}
	idx %= DIVOOM_TOP_MAIN_MENU_COUNT;

	return idx;
}

static void divoom_lvgl_top_main_menu_refresh_icon_src(void)
{
	int slot;

	if (pdivoom_lvgl_top_main_menu_handle == NULL)
	{
		return;
	}

	for (slot = DIVOOM_TOP_MAIN_ICON_FIRST; slot <= DIVOOM_TOP_MAIN_ICON_LAST; slot++) 
	{	
		int li = divoom_lvgl_top_main_menu_slot_to_img_idx(slot);
		
		divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_main_menu_handle->select_img[slot],
			divoom_top_main_img_list[li]);
	}
}

static int divoom_lvgl_top_main_menu_mod_menu_idx(int idx)
{
	while (idx < 0)
	{
		idx += DIVOOM_TOP_MAIN_MENU_COUNT;
	}
	
	return idx % DIVOOM_TOP_MAIN_MENU_COUNT;
}

static int divoom_lvgl_top_main_menu_slot_at_angle0(void)
{
	int i;

	if (pdivoom_lvgl_top_main_menu_handle == NULL)
	{
		return DIVOOM_TOP_MAIN_ICON_FIRST;
	}

	for (i = DIVOOM_TOP_MAIN_ICON_FIRST; i <= DIVOOM_TOP_MAIN_ICON_LAST; i++) 
	{
		if (pdivoom_lvgl_top_main_menu_handle->circle_angle[i] == 0)
		{
			return i;
		}
	}
	return 6;
}

static void divoom_lvgl_top_main_menu_refresh_only_angle0_clockwise(void)
{
	int slot, li;

	if (pdivoom_lvgl_top_main_menu_handle == NULL)
	{
		return;
	}

	slot = divoom_lvgl_top_main_menu_slot_at_angle0();
	li = divoom_lvgl_top_main_menu_mod_menu_idx(pdivoom_lvgl_top_main_menu_handle->select_id - 3);
	
	divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_main_menu_handle->select_img[slot],
		divoom_top_main_img_list[li]);
}

static void divoom_lvgl_top_main_menu_refresh_only_angle0_ccw(void)
{
	int slot, li;

	if (pdivoom_lvgl_top_main_menu_handle == NULL)
	{
		return;
	}

	slot = divoom_lvgl_top_main_menu_slot_at_angle0();
	li = divoom_lvgl_top_main_menu_mod_menu_idx(pdivoom_lvgl_top_main_menu_handle->select_id + 3);
	
	divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_main_menu_handle->select_img[slot],
		divoom_top_main_img_list[li]);
}

//计算图片坐标
void divoom_lvgl_top_main_menu_calculate_coordinate(int circle_angle, int* x, int* y)
{
	double arc_angle = circle_angle *3.14/180;

	if(pdivoom_lvgl_top_main_menu_handle == NULL)
	{
		return;
	}
	
	(*x) = pdivoom_lvgl_top_main_menu_handle->center_x + pdivoom_lvgl_top_main_menu_handle->radius * cos(arc_angle) - 100;	//减去图形长度的一半	
	(*y) = pdivoom_lvgl_top_main_menu_handle->center_y - pdivoom_lvgl_top_main_menu_handle->radius * sin(arc_angle) - 100;

}

void divoom_lvgl_top_main_menu_calculate_opa(int circle_angle, int* img_opa)
{
	int opa;
	int opa_25 = 25 * LV_OPA_100/100;
	int opa_75 = 75 * LV_OPA_100/100;

	if(circle_angle >= 0 && circle_angle < 30)
	{
		opa = opa_25 + circle_angle * (LV_OPA_50 - opa_25)/30;
	}
	else if(circle_angle >= 30 && circle_angle < 56)
	{
		opa = LV_OPA_50 + (circle_angle - 30) * (opa_75 - LV_OPA_50)/26;
	}
	else if(circle_angle >= 56 && circle_angle < 90)
	{
		opa = opa_75 + (circle_angle - 56) * (LV_OPA_100 - opa_75)/34;
	}
	else if(circle_angle >= 90 && circle_angle < 124)
	{
		opa = LV_OPA_100 - (circle_angle - 90) * (LV_OPA_100 - opa_75)/34;
	}
	else if(circle_angle >= 124 && circle_angle < 150)
	{
		opa = opa_75 - (circle_angle - 124) * (opa_75 - LV_OPA_50)/26;
	}
	else if(circle_angle >= 150 && circle_angle < 180)
	{
		opa = LV_OPA_50 - (circle_angle - 150) * (LV_OPA_50 - opa_25)/30;
	}
	else if(circle_angle == 180)
	{
		opa = opa_25;
	}
	else
	{
		opa = LV_OPA_100;
	}

	(*img_opa) = opa;
}

//计算图片缩小倍数
void divoom_lvgl_top_main_menu_calculate_zoom(int circle_angle, int* img_zoom)
{
	int zoom;
	int zoom_25 = 25 * 256/100;
	int zoom_50 = 50 * 256/100;
	int zoom_75 = 75 * 256/100;

	if(circle_angle >= 0 && circle_angle < 30)	//25%~50%
	{
		zoom = zoom_25 + circle_angle * (zoom_50 - zoom_25)/30;
	}
	else if(circle_angle >= 30 && circle_angle < 56) //50%~75%
	{
		zoom = zoom_50 + (circle_angle - 30) * (zoom_75 - zoom_50)/26;
	}
	else if(circle_angle >= 56 && circle_angle < 90) //75%~100%
	{
		zoom = zoom_75 + (circle_angle - 56) * (256 - zoom_75)/34;
	}
	else if(circle_angle >= 90 && circle_angle < 124)
	{
		//zoom = DIVOOM_TOP_MAIN_MENU_ZOOM_VALUE_80 + (120 - circle_angle) * (256 - DIVOOM_TOP_MAIN_MENU_ZOOM_VALUE_80)/30;
		zoom = 256 - (circle_angle - 90) * (256 - zoom_75)/34;
	}
	else if(circle_angle >= 124 && circle_angle < 150)
	{
		//zoom = DIVOOM_TOP_MAIN_MENU_ZOOM_VALUE_70 + (120 + 30 - circle_angle) * (DIVOOM_TOP_MAIN_MENU_ZOOM_VALUE_80 - DIVOOM_TOP_MAIN_MENU_ZOOM_VALUE_70)/30;
		zoom = zoom_75 - (circle_angle - 124) * (zoom_75 - zoom_50)/26;
	}
	else if(circle_angle >= 150 && circle_angle < 180)
	{
		zoom = zoom_50 - (circle_angle - 150) * (zoom_50 - zoom_25)/30;
	}
	else if(circle_angle == 180)
	{
		zoom = zoom_25;
	}
	else
	{
		zoom = 256;
	}
			
	(*img_zoom) = zoom;
}

void divoom_lvgl_top_main_menu_calculate_img_angle(int circle_angle, int* img_angle)
{
	int angle;	//图片旋转角度

	if(circle_angle >= 0 && circle_angle < 90)		//顺时针旋转图片
	{
		angle = 90 - circle_angle;	
	}
	else if(circle_angle >= 90 && circle_angle <= 180) //逆时针旋转图片
	{
		angle = 90 - circle_angle;
	}
	else
	{
		angle = 0;
	}

	(*img_angle) = angle * 10;
}

//逆时针旋转30度动画回调
void divoom_lvgl_top_main_angle_pre_anim_cb(void* var, int32_t v)
{
	int i, x, y, angle, zoom, opa;
	int diff_angle, circle_angle;

	if(pdivoom_lvgl_top_main_menu_handle == NULL)
	{
		return;
	}
	
	for(i = 1;i < 7; i++)
	{
		circle_angle = pdivoom_lvgl_top_main_menu_handle->pre_circle_angle[i];

		// v 0 30
		if(circle_angle == 30 || circle_angle == 124)	//减慢
		{
			circle_angle = circle_angle + v * 26 / 30;
		}
		else if(circle_angle == 56 || circle_angle == 90)	//加快
		{
			circle_angle = circle_angle + v * 34 / 30;
		}
		else	//速度不变
		{
			circle_angle += v;	
		}
		
		//if(circle_angle >= 255)//逆时针转超出放到右边起点
		if(circle_angle >= 165)
		{
			//circle_angle -= 270;
			circle_angle -= 180;
		}

		//divoom_print("v = %d\n", circle_angle - pdivoom_lvgl_top_main_menu_handle->pre_circle_angle[i]);
		
		//divoom_lvgl_top_main_menu_calculate_opa(circle_angle, &opa);
		//lv_obj_set_style_img_opa(pdivoom_lvgl_top_main_menu_handle->select_img[i], opa, 0);
		
		divoom_lvgl_top_main_menu_calculate_img_angle(circle_angle, &angle);
		lv_img_set_angle(pdivoom_lvgl_top_main_menu_handle->select_img[i], angle);

		divoom_lvgl_top_main_menu_calculate_zoom(circle_angle, &zoom);
		lv_img_set_zoom(pdivoom_lvgl_top_main_menu_handle->select_img[i], zoom);
		
		divoom_lvgl_top_main_menu_calculate_coordinate(circle_angle, &x, &y);
		lv_obj_set_pos(pdivoom_lvgl_top_main_menu_handle->select_img[i], x, y);

	}

}

void divoom_lvgl_top_main_menu_pre_anim_ready_cb(struct _lv_anim_t *a)
{
	if(pdivoom_lvgl_top_main_menu_handle == NULL)
	{
		return;
	}
	
	pdivoom_lvgl_top_main_menu_handle->anim_end_flag = 1;
}


//逆时针旋转30度动画
void divoom_lvgl_top_main_menu_angle_pre_anim(void)
{
	lv_anim_t a;

	if(pdivoom_lvgl_top_main_menu_handle->anim_end_flag == 0)
	{
		return;
	}

	pdivoom_lvgl_top_main_menu_handle->exit_time = 0;
	pdivoom_lvgl_top_main_menu_handle->anim_end_flag = 0;

	divoom_lvgl_top_main_menu_refresh_only_angle0_ccw();

	pdivoom_lvgl_top_main_menu_handle->select_id ++;
    if(pdivoom_lvgl_top_main_menu_handle->select_id >= DIVOOM_TOP_MAIN_MENU_COUNT)
    {
        pdivoom_lvgl_top_main_menu_handle->select_id = 0;
    }

	lv_label_set_text(pdivoom_lvgl_top_main_menu_handle->label_obj, divoom_menu_control_get_disp_str(divoom_top_main_mode_str[pdivoom_lvgl_top_main_menu_handle->select_id]));

	lv_anim_init(&a);
 	lv_anim_set_exec_cb(&a, divoom_lvgl_top_main_angle_pre_anim_cb);
 	lv_anim_set_time(&a, 300);
 	lv_anim_set_values(&a, 0, 30);			//30度
	lv_anim_set_path_cb(&a, lv_anim_path_linear);
	lv_anim_set_ready_cb(&a, divoom_lvgl_top_main_menu_pre_anim_ready_cb);

	for(int i = 1; i < 7; i++)
	{
		//divoom_print("pre_circle_angle = %d, circle_angle = %d\n", pdivoom_lvgl_top_main_menu_handle->pre_circle_angle[i], pdivoom_lvgl_top_main_menu_handle->circle_angle[i]);
		pdivoom_lvgl_top_main_menu_handle->pre_circle_angle[i] = pdivoom_lvgl_top_main_menu_handle->circle_angle[i];

		if(pdivoom_lvgl_top_main_menu_handle->circle_angle[i] == 30 || pdivoom_lvgl_top_main_menu_handle->circle_angle[i] == 124)
		{
			pdivoom_lvgl_top_main_menu_handle->circle_angle[i] += 26;	//转动26度
		}
		else if(pdivoom_lvgl_top_main_menu_handle->circle_angle[i] == 90 || pdivoom_lvgl_top_main_menu_handle->circle_angle[i] == 56)
		{
			pdivoom_lvgl_top_main_menu_handle->circle_angle[i] += 34;	//转动34度
		}
		else
		{
			pdivoom_lvgl_top_main_menu_handle->circle_angle[i] += 30;
		}
		
		if(pdivoom_lvgl_top_main_menu_handle->circle_angle[i] > 150)
		{
			pdivoom_lvgl_top_main_menu_handle->circle_angle[i] = 0;
		}
	}
	
	lv_anim_start(&a);
}

//顺时针旋转30度动画回调
void divoom_lvgl_top_main_angle_next_anim_cb(void* var, int32_t v)
{
	int i, x, y, angle, zoom, opa;
	int diff_angle, circle_angle;

	if(pdivoom_lvgl_top_main_menu_handle == NULL)
	{
		return;
	}
	
	for(i = 1;i < 7; i++)
	{
		circle_angle = pdivoom_lvgl_top_main_menu_handle->pre_circle_angle[i];
				
		// v 0 -30
		if(circle_angle == 56 || circle_angle == 150)	//减慢
		{
			circle_angle = circle_angle + v * 26 / 30;
		}
		else if(circle_angle == 124 || circle_angle == 90)	//加快
		{
			circle_angle = circle_angle + v * 34 / 30;
		}
		else	
		{
			circle_angle += v;
		}
				
		if(circle_angle <= 0) //顺时针转超出放到左边起点
		{
			//circle_angle += 270;
			circle_angle += 180;
		}

		//divoom_print("v = %d\n", circle_angle - pdivoom_lvgl_top_main_menu_handle->pre_circle_angle[i]);
		
		//divoom_lvgl_top_main_menu_calculate_opa(circle_angle, &opa);
		//lv_obj_set_style_img_opa(pdivoom_lvgl_top_main_menu_handle->select_img[i], opa, 0);
		
		divoom_lvgl_top_main_menu_calculate_img_angle(circle_angle, &angle);
		lv_img_set_angle(pdivoom_lvgl_top_main_menu_handle->select_img[i], angle);

		divoom_lvgl_top_main_menu_calculate_zoom(circle_angle, &zoom);
		lv_img_set_zoom(pdivoom_lvgl_top_main_menu_handle->select_img[i], zoom);
		
		divoom_lvgl_top_main_menu_calculate_coordinate(circle_angle, &x, &y);
		lv_obj_set_pos(pdivoom_lvgl_top_main_menu_handle->select_img[i], x, y);

	}

}

void divoom_lvgl_top_main_menu_next_anim_ready_cb(struct _lv_anim_t *a)
{
	if(pdivoom_lvgl_top_main_menu_handle == NULL)
	{
		return;
	}

	pdivoom_lvgl_top_main_menu_handle->anim_end_flag = 1;
}

//顺时针旋转30度动画
void divoom_lvgl_top_main_menu_angle_next_anim(void)
{
	lv_anim_t a;

	if(pdivoom_lvgl_top_main_menu_handle->anim_end_flag == 0)
	{
		return;
	}
	
	pdivoom_lvgl_top_main_menu_handle->exit_time = 0;
	pdivoom_lvgl_top_main_menu_handle->anim_end_flag = 0;

	divoom_lvgl_top_main_menu_refresh_only_angle0_clockwise();

	if(pdivoom_lvgl_top_main_menu_handle->select_id == 0)
    {
        pdivoom_lvgl_top_main_menu_handle->select_id = DIVOOM_TOP_MAIN_MENU_COUNT - 1;
    }
    else
    {
        pdivoom_lvgl_top_main_menu_handle->select_id --;
    }

	lv_label_set_text(pdivoom_lvgl_top_main_menu_handle->label_obj, divoom_menu_control_get_disp_str(divoom_top_main_mode_str[pdivoom_lvgl_top_main_menu_handle->select_id]));

	lv_anim_init(&a);
 	lv_anim_set_exec_cb(&a, divoom_lvgl_top_main_angle_next_anim_cb);
 	lv_anim_set_time(&a, 300);
 	lv_anim_set_values(&a, 0, -30);			//-30度
	lv_anim_set_path_cb(&a, lv_anim_path_linear);
	lv_anim_set_ready_cb(&a, divoom_lvgl_top_main_menu_next_anim_ready_cb);

	for(int i = 1; i < 7; i++)
	{
		pdivoom_lvgl_top_main_menu_handle->pre_circle_angle[i] = pdivoom_lvgl_top_main_menu_handle->circle_angle[i];

		if(pdivoom_lvgl_top_main_menu_handle->circle_angle[i] == 56 || pdivoom_lvgl_top_main_menu_handle->circle_angle[i] == 150)
		{
			pdivoom_lvgl_top_main_menu_handle->circle_angle[i] -= 26;
		}
		else if(pdivoom_lvgl_top_main_menu_handle->circle_angle[i] == 124 || pdivoom_lvgl_top_main_menu_handle->circle_angle[i] == 90)
		{
			pdivoom_lvgl_top_main_menu_handle->circle_angle[i] -= 34;
		}
		else
		{
			pdivoom_lvgl_top_main_menu_handle->circle_angle[i] -= 30;
		}
		
		if(pdivoom_lvgl_top_main_menu_handle->circle_angle[i] < 0)
		{
			pdivoom_lvgl_top_main_menu_handle->circle_angle[i] = 150;
		}
	}

	lv_anim_start(&a);
}

void divoom_lvgl_top_main_menu_del_menu(void)
{
	if(pdivoom_lvgl_top_main_menu_handle->menu_exit_timer)
	{
		lv_timer_del(pdivoom_lvgl_top_main_menu_handle->menu_exit_timer);
	}
    divoom_lvgl_top_control_remove_top(pdivoom_lvgl_top_main_menu_handle);
	lv_obj_del_async(pdivoom_lvgl_top_main_menu_handle->main_obj);
	divoom_system_free(pdivoom_lvgl_top_main_menu_handle);
        divoom_local_movie_play_ctrl(1);
	pdivoom_lvgl_top_main_menu_handle = NULL;
}

static void divoom_lvgl_top_main_menu_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
	
	if(code == LV_EVENT_KEY)
    {
        char *key_info = (char *)lv_event_get_param(e);
        char c = key_info[0];

        if(pdivoom_lvgl_top_main_menu_handle == NULL || pdivoom_lvgl_top_main_menu_handle->anim_end_flag == 0)
        {
            divoom_print("test!\n");
			return;
        }
		if(c == DIVOOM_LVGL_KEY_NEXT) 
		{
            divoom_lvgl_top_main_menu_angle_pre_anim();
        }
		else if(c == DIVOOM_LVGL_KEY_PRE)
		{
			divoom_lvgl_top_main_menu_angle_next_anim();
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{	
			int select_id = pdivoom_lvgl_top_main_menu_handle->select_id;
			divoom_lvgl_top_main_menu_del_menu();
            if(divoom_top_main_next_menu_id[select_id] == DIVOOM_MENU_CONTROL_ID_SYSTEM_SETING_MENU)
            {
                divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_ENTER_MENU_SETTING);
            }
            else
            {
			    divoom_menu_control_clearall_load(divoom_top_main_next_menu_id[select_id], NULL, 0, 0);
            }
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_lvgl_top_main_menu_del_menu();
		}
        else if(c == DIVOOM_LVGL_KEY_HOME)
        {
            divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
        }
    }
	
}

void divoom_lvgl_top_main_menu_time_handle(lv_timer_t *exit_timer)
{
	if(pdivoom_lvgl_top_main_menu_handle != NULL)
	{
		pdivoom_lvgl_top_main_menu_handle->exit_time++;

		if(pdivoom_lvgl_top_main_menu_handle->exit_time >= DIVOOM_EXIT_MENU_TIME_OUT)//主菜单不变动自动退出
		{
			if(pdivoom_lvgl_top_main_menu_handle->anim_end_flag == 1)
			{
				divoom_lvgl_top_main_menu_del_menu();
			}
		}
	}
}

void divoom_lvgl_top_main_menu_create_icon(void)
{
	int i, x, y, zoom, angle, opa;
	
	pdivoom_lvgl_top_main_menu_handle->cout_obj = lv_btn_create(pdivoom_lvgl_top_main_menu_handle->main_obj);

	lv_obj_remove_style_all(pdivoom_lvgl_top_main_menu_handle->cout_obj);
	lv_obj_set_style_pad_all(pdivoom_lvgl_top_main_menu_handle->cout_obj, 0, 0);
	lv_obj_set_style_border_width(pdivoom_lvgl_top_main_menu_handle->cout_obj, 0, 0);
	lv_obj_set_style_outline_width(pdivoom_lvgl_top_main_menu_handle->cout_obj, 0, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_shadow_width(pdivoom_lvgl_top_main_menu_handle->cout_obj, 0, 0);
	lv_obj_set_style_bg_opa(pdivoom_lvgl_top_main_menu_handle->cout_obj, LV_OPA_0, 0);
	lv_obj_set_pos(pdivoom_lvgl_top_main_menu_handle->cout_obj, 0, 0);
    lv_obj_set_size(pdivoom_lvgl_top_main_menu_handle->cout_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_add_event_cb(pdivoom_lvgl_top_main_menu_handle->cout_obj, divoom_lvgl_top_main_menu_event_cb, LV_EVENT_KEY, NULL);

	//lv_obj_t *shadow_img = lv_img_create(pdivoom_lvgl_top_main_menu_handle->cout_obj);
	//divoom_menu_control_lv_img_set_src(shadow_img, "75/75_top_glass_shadow_bg.png");
	//lv_obj_set_pos(shadow_img, 0, 0);

	lv_obj_t *bg_img = lv_img_create(pdivoom_lvgl_top_main_menu_handle->cout_obj);
	divoom_menu_control_lv_img_set_src(bg_img, "75/75_top_glass_bg.png");
	lv_obj_set_pos(bg_img, 0, 1280 - 467);

	lv_obj_t *label = lv_label_create(bg_img);
	lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(26, DIVOOM_MENU_CONTROL_ID_MAIN), 0);
	lv_obj_set_style_text_color(label, lv_color_hex(0x333333), 0);
	//lv_obj_set_style_text_color(label, lv_color_black(), 0);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_size(label, 320, 72);
	lv_obj_set_pos(label, 240, 345+20);
	lv_label_set_text(label, divoom_menu_control_get_disp_str(divoom_top_main_mode_str[pdivoom_lvgl_top_main_menu_handle->select_id]));
	pdivoom_lvgl_top_main_menu_handle->label_obj = label;

	for(i = 1;i < 7; i++)
	{
		pdivoom_lvgl_top_main_menu_handle->select_img[i] = lv_img_create(pdivoom_lvgl_top_main_menu_handle->cout_obj);

		if(i == 2)	//角度不一样
		{
			pdivoom_lvgl_top_main_menu_handle->pre_circle_angle[i] = 180 - 30*i + 4;
			pdivoom_lvgl_top_main_menu_handle->circle_angle[i] = 180 - 30*i + 4;
		}
		else if(i == 4)
		{
			pdivoom_lvgl_top_main_menu_handle->pre_circle_angle[i] = 180 - 30*i - 4;
			pdivoom_lvgl_top_main_menu_handle->circle_angle[i] = 180 - 30*i - 4;
		}
		else
		{
			pdivoom_lvgl_top_main_menu_handle->pre_circle_angle[i] = 180 - 30*i;
			pdivoom_lvgl_top_main_menu_handle->circle_angle[i] = 180 - 30*i;
			//pdivoom_lvgl_top_main_menu_handle->pre_circle_angle[i] = 270 - 30*i;
			//pdivoom_lvgl_top_main_menu_handle->circle_angle[i] = 270 - 30*i;
		}

		//divoom_lvgl_top_main_menu_calculate_opa(pdivoom_lvgl_top_main_menu_handle->circle_angle[i], &opa);
		//lv_obj_set_style_img_opa(pdivoom_lvgl_top_main_menu_handle->select_img[i], opa, 0);
		lv_obj_set_style_img_opa(pdivoom_lvgl_top_main_menu_handle->select_img[i], LV_OPA_100, 0);

		divoom_lvgl_top_main_menu_calculate_img_angle(pdivoom_lvgl_top_main_menu_handle->circle_angle[i], &angle);
		lv_img_set_angle(pdivoom_lvgl_top_main_menu_handle->select_img[i], angle);

		divoom_lvgl_top_main_menu_calculate_zoom(pdivoom_lvgl_top_main_menu_handle->circle_angle[i], &zoom);
		lv_img_set_zoom(pdivoom_lvgl_top_main_menu_handle->select_img[i], zoom);
		
		//lv_obj_set_size(phandle->select_img[i], 128, 128);
		divoom_lvgl_top_main_menu_calculate_coordinate(pdivoom_lvgl_top_main_menu_handle->circle_angle[i], &x, &y);
		lv_obj_set_pos(pdivoom_lvgl_top_main_menu_handle->select_img[i], x, y);

	}

	divoom_lvgl_top_main_menu_refresh_icon_src();
}

void divoom_lvgl_top_main_menu_init(void)
{
	divoom_get_lvgl_lock();

	if(pdivoom_lvgl_top_main_menu_handle != NULL)
	{
		divoom_free_lvgl_lock();
		return;
	}

	pdivoom_lvgl_top_main_menu_handle = (Pdivoom_lvgl_top_main_menu_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_top_main_menu_ctrl));

	if(pdivoom_lvgl_top_main_menu_handle == NULL)
	{
		divoom_free_lvgl_lock();
		return;
	}
    divoom_local_movie_play_ctrl(0);
	memset(pdivoom_lvgl_top_main_menu_handle, 0, sizeof(divoom_lvgl_top_main_menu_ctrl));

	pdivoom_lvgl_top_main_menu_handle->center_x = 24 + 376;//400;
	pdivoom_lvgl_top_main_menu_handle->center_y = 996 + 376;//1367;
	pdivoom_lvgl_top_main_menu_handle->radius = 376;
	pdivoom_lvgl_top_main_menu_handle->select_id = 5;
	pdivoom_lvgl_top_main_menu_handle->anim_end_flag = 1;
	pdivoom_lvgl_top_main_menu_handle->exit_time = 0;
	
	pdivoom_lvgl_top_main_menu_handle->main_obj = lv_obj_create(lv_layer_top()); //在顶层创建主页面

	lv_obj_set_style_bg_opa(pdivoom_lvgl_top_main_menu_handle->main_obj, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(pdivoom_lvgl_top_main_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_border_width(pdivoom_lvgl_top_main_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_shadow_width(pdivoom_lvgl_top_main_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_outline_width(pdivoom_lvgl_top_main_menu_handle->main_obj, 0, 0);
	lv_obj_clear_flag(pdivoom_lvgl_top_main_menu_handle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(pdivoom_lvgl_top_main_menu_handle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(pdivoom_lvgl_top_main_menu_handle->main_obj, 0, 0);
	
	
	divoom_lvgl_top_main_menu_create_icon();

	pdivoom_lvgl_top_main_menu_handle->menu_exit_timer = lv_timer_create(divoom_lvgl_top_main_menu_time_handle, 1000, NULL);
	
	//lv_group_focus_obj(pdivoom_lvgl_top_main_menu_handle->cout_obj);
	divoom_lvgl_top_control_add_top(pdivoom_lvgl_top_main_menu_handle, pdivoom_lvgl_top_main_menu_handle->cout_obj);
	divoom_free_lvgl_lock();
}


void divoom_lvgl_top_main_menu_exit(void)
{
	if(pdivoom_lvgl_top_main_menu_handle != NULL)
	{
		if(pdivoom_lvgl_top_main_menu_handle->menu_exit_timer)
		{
			lv_timer_del(pdivoom_lvgl_top_main_menu_handle->menu_exit_timer);
		}
        divoom_lvgl_top_control_remove_top(pdivoom_lvgl_top_main_menu_handle);
		lv_obj_del_async(pdivoom_lvgl_top_main_menu_handle->main_obj);
		divoom_system_free(pdivoom_lvgl_top_main_menu_handle);
        divoom_local_movie_play_ctrl(1);
		pdivoom_lvgl_top_main_menu_handle = NULL;
	}
}





#endif
	
#else	//灯效版本
#define DIVOOM_TOP_MAIN_MENU_ZOOM_VALUE_60  154
#define DIVOOM_TOP_MAIN_MENU_ZOOM_VALUE_70  179
#define DIVOOM_TOP_MAIN_MENU_ZOOM_VALUE_80	205

typedef struct _divoom_lvgl_top_main_menu_ctrl
{
	void *main_obj;		
	void *cout_obj;
	void *label_obj;

	int select_id;
	int radius;
	int center_x;
	int center_y;

	int anim_end_flag;			//动画结束标志
	int pre_circle_angle[7];	//转换前角度
	int circle_angle[7];		//相对于右边水平半径的夹角
	void *select_img[7];
	//Pdivoom_lvgl_top_main_menu_image_ctrl pdisp_handle[7];

	int exit_time;
	lv_timer_t *menu_exit_timer;
}divoom_lvgl_top_main_menu_ctrl, *Pdivoom_lvgl_top_main_menu_ctrl;

Pdivoom_lvgl_top_main_menu_ctrl pdivoom_lvgl_top_main_menu_handle = NULL;

const char *divoom_top_main_mode_str[] = {
	"Tool Box",
	"Alarm",
	"My Clocks",
	"Pomodoro",
	"Settings",
	"AlbumManage",
};


const char *divoom_top_main_img_list[] = {
	"75/75_top_tool.png",
	"75/75_top_alarm.png",
	"75/75_top_dial.png",
	"75/75_top_mato.png",
	"75/75_top_setting.png",
	"75/75_top_album.png",
};

const int divoom_top_main_next_menu_id[] =
{
	DIVOOM_MENU_CONTROL_ID_TOOL,
	DIVOOM_MENU_CONTROL_ID_ALARM_LIST_MENU,
	DIVOOM_MENU_CONTROL_ID_DIAL_PREVIEW,
	DIVOOM_MENU_CONTROL_ID_POMODORO_MENU,
	DIVOOM_MENU_CONTROL_ID_SYSTEM_SETING_MENU,
	DIVOOM_MENU_CONTROL_ID_SELECT_ALBUM_PLAY,
};

void *divoom_lvgl_top_main_menu_open_resource_handle(char *file_name, int *resource_type, int *out_w, int *out_h, int *out_num)
{
    void *pwebp_handle;
    int image_w = 0, image_h = 0, frame_num = 1;
    
    *resource_type = divoom_app_com_get_resouce_type(file_name);
    if(*resource_type == DIVOOM_RESOURCE_TYPE_ERR)
    {
        return NULL;
    }
    //divoom_print("resource_type: %d\n",*resource_type);
    if(*resource_type == DIVOOM_RESOURCE_TYPE_WEBP)
    {
        
        pwebp_handle = divoom_libwebp_decode_init(file_name);
        if(pwebp_handle == NULL)
        {
            return NULL;
        }
        frame_num = divoom_libwebp_decode_get_frame_info(pwebp_handle, &image_w, &image_h);
    }
    else if(*resource_type == DIVOOM_RESOURCE_TYPE_JPEG)
    {
        pwebp_handle = divoom_decode_jpg_bg_init_scaled_lvfmt(file_name, 0, 0,
            &image_w, &image_h, LV_COLOR_FORMAT_RGB565A8);
    }
    else if(*resource_type == DIVOOM_RESOURCE_TYPE_JPEG_GIF)
    {
        pwebp_handle = divoom_decode_jpeg_gif_init_rgb565a8(file_name,
            &image_w, &image_h, &frame_num); 
    }
    else if(*resource_type == DIVOOM_RESOURCE_TYPE_GIF)
    {
        
        pwebp_handle = divoom_gif_lib_init(file_name, NULL, 0);
        if(pwebp_handle == NULL)
        {
            return NULL;
        }
        
        divoom_gif_lib_get_pic_info(pwebp_handle, &image_w, &image_h, NULL);
        
        if(out_num)
        {
            frame_num = 100;
        }
    }
    else 
    {
       
    }
    
    if(out_w)
    {
        *out_w = image_w;
    }
    if(out_h)
    {
        *out_h = image_h;
    }
    if(out_num)
    {
        *out_num = frame_num;
    }
    return pwebp_handle;
}

void divoom_lvgl_top_main_menu_release_resource_handle(void *pwebp_handle, int resource_type)
{
    if(pwebp_handle == NULL)
    {
        return;
    }
    
    //divoom_print("resource_type: %d\n",resource_type);
    if(resource_type == DIVOOM_RESOURCE_TYPE_WEBP)
    {
        divoom_libwebp_decode_exit(pwebp_handle);
    }
    else if(resource_type == DIVOOM_RESOURCE_TYPE_JPEG)
    {
        divoom_decode_jpg_bg_exit(pwebp_handle);
    }
    else if(resource_type == DIVOOM_RESOURCE_TYPE_JPEG_GIF)
    {
        divoom_decode_jpeg_gif_exit(pwebp_handle);
    }
    else if(resource_type == DIVOOM_RESOURCE_TYPE_GIF)
    {
        divoom_gif_lib_exit(pwebp_handle);
    }
    else
    {
        
    }
    
}

void divoom_lvgl_top_main_menu_clear_image_handle(Pdivoom_lvgl_top_main_menu_image_ctrl pimage_handle)
{
    if(pimage_handle)
    {
        if(pimage_handle->disp_time_handle)
        {
            lv_timer_del(pimage_handle->disp_time_handle);
        }
		
		lv_img_cache_invalidate_src(pimage_handle->lvgl_image_buf); 
        lv_obj_del(pimage_handle->lvgl_image_obj);
        //if(pimage_handle->image_type == DIVOOM_LVGL_DIAL_MENU_TYPE_LOCAL_IMAGE)
        //{
            divoom_lvgl_top_main_menu_release_resource_handle(pimage_handle->pwebp_handle, pimage_handle->resource_type);
            pimage_handle->pwebp_handle = NULL;
        //}
		if (pimage_handle->lvgl_image_buf)
		{
			if (pimage_handle->lvgl_image_buf->data)
			{
				divoom_system_free((void *)pimage_handle->lvgl_image_buf->data);
				pimage_handle->lvgl_image_buf->data = NULL;
			}
			divoom_system_free(pimage_handle->lvgl_image_buf);
		}
		//if (pimage_handle->lvgl_image_info)
		//{
		//	pimage_handle->lvgl_image_info->flag = 0; 
		//}
        if(pimage_handle->divoom_rgb_buf)
        {
            divoom_system_free(pimage_handle->divoom_rgb_buf);
        }
        divoom_system_free(pimage_handle->image_name);
        divoom_system_free(pimage_handle);
    }
}



void divoom_lvgl_top_main_menu_set_img_dsc_t(Pdivoom_lvgl_top_main_menu_image_ctrl pimage_handle, int w, int h)
{
	int flag = 0;

	if (pimage_handle->lvgl_image_buf == NULL)
		pimage_handle->lvgl_image_buf = divoom_system_malloc(sizeof(lv_img_dsc_t));

	if (pimage_handle->lvgl_image_buf->header.w != w ||
        pimage_handle->lvgl_image_buf->header.h != h ||
        pimage_handle->lvgl_image_buf->header.cf != LV_COLOR_FORMAT_RGB565A8)
	{
		flag = 1;
	}

	divoom_lv_img_dsc_init_fmt(pimage_handle->lvgl_image_buf, w, h,
        LV_COLOR_FORMAT_RGB565A8, pimage_handle->lvgl_image_buf->data);

	if (flag)
	{
		if (pimage_handle->lvgl_image_buf->data)
			divoom_system_free((uint8 *)pimage_handle->lvgl_image_buf->data);

		pimage_handle->lvgl_image_buf->data =
            divoom_system_malloc(pimage_handle->lvgl_image_buf->data_size);
	}
}

void divoom_lvgl_top_main_menu_read_image_buf(Pdivoom_lvgl_top_main_menu_image_ctrl pimage_handle, struct _lv_timer_t *user_time_handle)
{
    uint8 *frame_buf = NULL;
    uint8 *decode_tmp_buf = NULL;
    uint8 *dst_buf;
	
	//if (pimage_handle->image_type != DIVOOM_LVGL_DIAL_MENU_TYPE_DIVOOM_QUEUE && pimage_handle->image_type != DIVOOM_LVGL_DIAL_MENU_TYPE_DIVOOM_EQ_DIAL)
	//{
	    if(pimage_handle->frame_num < 1)
	    {
	        return;
	    }
	    if(pimage_handle->frame_num == 1 && user_time_handle)
	    {
	        return;
	    }
	//}
    //if(pimage_handle->image_type == DIVOOM_LVGL_DIAL_MENU_TYPE_LOCAL_IMAGE)
    //{
        int end_flag = 0;

        if(pimage_handle->lvgl_image_buf->data == NULL)
        {
            pimage_handle->lvgl_image_buf->data =
                divoom_system_malloc(pimage_handle->lvgl_image_buf->data_size);
        }
        if(pimage_handle->lvgl_image_buf->data == NULL)
        {
            return;
        }
        dst_buf = (uint8 *)pimage_handle->lvgl_image_buf->data;
        
        if(pimage_handle->resource_type == DIVOOM_RESOURCE_TYPE_WEBP)
        {
            if(!pimage_handle->image_strech &&
                pimage_handle->lvgl_image_buf->header.cf == LV_COLOR_FORMAT_RGB565A8)
            {
                frame_buf = divoom_libwebp_decode_get_frame_rgb565a8_buffer(pimage_handle->pwebp_handle,
                    &pimage_handle->frame_time, &end_flag, dst_buf);
            }
            else
            {
                decode_tmp_buf = divoom_system_malloc(divoom_pixel_format_size(
                    (uint32)pimage_handle->image_width,
                    (uint32)pimage_handle->image_high,
                    LV_COLOR_FORMAT_RGB565A8));
                if(decode_tmp_buf == NULL)
                {
                    return;
                }
                frame_buf = divoom_libwebp_decode_get_frame_rgb565a8_buffer(pimage_handle->pwebp_handle,
                    &pimage_handle->frame_time, &end_flag, decode_tmp_buf);
            }
        }
        else if(pimage_handle->resource_type == DIVOOM_RESOURCE_TYPE_GIF)
        {
            
            frame_buf = divoom_gif_lib_get_frame_rgb565a8_data(pimage_handle->pwebp_handle, &pimage_handle->frame_time, &end_flag);
            
        }
        else if(pimage_handle->resource_type == DIVOOM_RESOURCE_TYPE_JPEG_GIF)
        {            
            frame_buf = divoom_decode_jpeg_gif_get_frame_rgb565a8_buffer(pimage_handle->pwebp_handle, &pimage_handle->frame_time, &end_flag, NULL);
            
        }
        else if(pimage_handle->resource_type == DIVOOM_RESOURCE_TYPE_JPEG)
        {            
            end_flag = 1;
            pimage_handle->frame_time = 1000;            
            frame_buf = divoom_decode_jpg_bg_get_lvfmt_buf(pimage_handle->pwebp_handle);
            
        }
        else
        {
            if(decode_tmp_buf)
            {
                divoom_system_free(decode_tmp_buf);
            }
            return ;
        }
        if(frame_buf == NULL)
        {
            if(decode_tmp_buf)
            {
                divoom_system_free(decode_tmp_buf);
            }
            return;
        }
        /*if(end_flag)
        {
            if(pimage_handle->pfinish_callback)
            {
                if(pimage_handle->pfinish_callback(pimage_handle->user_data))
                {
                    if(pimage_handle->resource_type == DIVOOM_RESOURCE_TYPE_WEBP)
                    {
                        divoom_system_free(frame_buf);
                    }
                    return;
                }
            }
        }*/
        if(frame_buf != dst_buf)
        {
            divoom_rgb565a8_to_lvfmt_nearest(frame_buf,
                (uint32)pimage_handle->image_width,
                (uint32)pimage_handle->image_high, dst_buf,
                (uint32)pimage_handle->area_w,
                (uint32)pimage_handle->area_h,
                pimage_handle->lvgl_image_buf->header.cf);
        }
        
        if(decode_tmp_buf)
        {
            divoom_system_free(decode_tmp_buf);
        }
        lv_obj_invalidate(pimage_handle->lvgl_image_obj);

    //}
}


void divoom_lvgl_top_main_menu_refresh_local_image_time(struct _lv_timer_t *user_time_handle)
{
    Pdivoom_lvgl_top_main_menu_image_ctrl pimage_handle = (Pdivoom_lvgl_top_main_menu_image_ctrl)lv_timer_get_user_data(user_time_handle);
    
    //divoom_lvgl_top_main_menu_read_image_buf(pimage_handle, user_time_handle);
    //if(pimage_handle->angle_value)
    //{
    //    lv_img_set_angle(pimage_handle->lvgl_image_obj, pimage_handle->angle_value);
    //}
}

void *divoom_lvgl_top_main_menu_add_local_image_handle(void *handle, Pdivoom_lvgl_top_main_menu_image_item pimage_info, void *obj)
{
    Pdivoom_lvgl_top_main_menu_image_ctrl pimage_handle = (Pdivoom_lvgl_top_main_menu_image_ctrl)handle;
    
    if(pimage_info->image_name == NULL)
    {
        return handle;
    }

    if(pimage_handle == NULL)
    {
        pimage_handle = divoom_system_malloc(sizeof(divoom_lvgl_top_main_menu_image_ctrl));
        if(pimage_handle == NULL)
        {
            return NULL;            
        }
        memset(pimage_handle, 0, sizeof(divoom_lvgl_top_main_menu_image_ctrl));
        
        pimage_handle->pwebp_handle = divoom_lvgl_top_main_menu_open_resource_handle(pimage_info->image_name, &pimage_handle->resource_type, &pimage_handle->image_width, &pimage_handle->image_high, &pimage_handle->frame_num);
        if(pimage_handle->pwebp_handle == NULL)
        {
            divoom_system_free(pimage_handle);
            return NULL;
        }
        
        //pimage_handle->image_type = DIVOOM_LVGL_DIAL_MENU_TYPE_LOCAL_IMAGE;
        pimage_handle->area_w = pimage_info->w;
        pimage_handle->area_h = pimage_info->h;
        pimage_handle->image_name = divoom_app_com_copy_string(pimage_info->image_name, 0);
        pimage_handle->lvgl_image_obj = lv_img_create(obj);
		pimage_handle->obj = obj;
		//pimage_handle->lvgl_image_info = NULL;

        lv_obj_set_pos(pimage_handle->lvgl_image_obj, pimage_info->x, pimage_info->y);
        lv_obj_set_size(pimage_handle->lvgl_image_obj, pimage_info->w, pimage_info->h);
        
        if(pimage_info->w == pimage_handle->image_width && pimage_info->h == pimage_handle->image_high)
        {
            pimage_handle->image_strech = 0;
        }
        else
        {
            pimage_handle->image_strech = 1;
        }

		divoom_lvgl_top_main_menu_set_img_dsc_t(pimage_handle, pimage_info->w, pimage_info->h);
        lv_img_set_antialias(pimage_handle->lvgl_image_obj, pimage_info->antialias_flag);

        divoom_lvgl_top_main_menu_read_image_buf(pimage_handle, NULL);
        lv_img_set_src(pimage_handle->lvgl_image_obj, pimage_handle->lvgl_image_buf);
        if(pimage_info->angle)
        {
            lv_img_set_angle(pimage_handle->lvgl_image_obj, pimage_info->angle);
        }
        pimage_handle->angle_value = pimage_info->angle;
        pimage_handle->disp_time_handle = lv_timer_create(divoom_lvgl_top_main_menu_refresh_local_image_time, pimage_handle->frame_time, pimage_handle);
        //divoom_queue_push(pdivoom_lvgl_dial_menu_handle->disp_image_queue, pimage_handle);
    }
    else if(strcmp(pimage_handle->image_name, pimage_info->image_name) == 0)
    {
        //同一个文件
        if(pimage_handle->angle_value != pimage_info->angle)
        {
            pimage_handle->angle_value = pimage_info->angle;
            lv_img_set_angle(pimage_handle->lvgl_image_obj, pimage_info->angle);
            
        }
        return pimage_handle;
    }
    else
    {
    
		divoom_lvgl_top_main_menu_set_img_dsc_t(pimage_handle, pimage_info->w, pimage_info->h);
		lv_img_cache_invalidate_src(pimage_handle->lvgl_image_buf); 
        lv_img_set_src(pimage_handle->lvgl_image_obj, pimage_handle->lvgl_image_buf);

        //变更图像
        if(pimage_handle->disp_time_handle)
        {
            lv_timer_del(pimage_handle->disp_time_handle);
            pimage_handle->disp_time_handle = NULL;
        }
        divoom_lvgl_top_main_menu_release_resource_handle(pimage_handle->pwebp_handle, pimage_handle->resource_type);
        pimage_handle->pwebp_handle = NULL;
        if(pimage_handle->image_name)
        {
            divoom_system_free(pimage_handle->image_name);
        }
        pimage_handle->image_name = divoom_app_com_copy_string(pimage_info->image_name, 0);
        pimage_handle->pwebp_handle = divoom_lvgl_top_main_menu_open_resource_handle(pimage_info->image_name, &pimage_handle->resource_type,
            &pimage_handle->image_width, &pimage_handle->image_high, &pimage_handle->frame_num);
        if(pimage_handle->pwebp_handle == NULL)
        {
            return pimage_handle;        
        }
        if(pimage_info->w == pimage_handle->image_width && pimage_info->h == pimage_handle->image_high)
        {
            pimage_handle->image_strech = 0;
        }
        else
        {
            pimage_handle->image_strech = 1;
        }
        divoom_lvgl_top_main_menu_read_image_buf(pimage_handle, NULL);
        if(pimage_info->angle || pimage_handle->angle_value != pimage_info->angle)
        {
            lv_img_set_angle(pimage_handle->lvgl_image_obj, pimage_info->angle);
            pimage_handle->angle_value = pimage_info->angle;
        }
        lv_obj_invalidate(pimage_handle->lvgl_image_obj);
        //如果是动画，将要开启定时器
        pimage_handle->disp_time_handle = lv_timer_create(divoom_lvgl_top_main_menu_refresh_local_image_time, pimage_handle->frame_time, pimage_handle);
   
   }

    return pimage_handle;
}


//计算图片坐标
void divoom_lvgl_top_main_menu_calculate_coordinate(int circle_angle, int* x, int* y)
{
	double arc_angle = circle_angle *3.14/180;

	if(pdivoom_lvgl_top_main_menu_handle == NULL)
	{
		return;
	}
	
	(*x) = pdivoom_lvgl_top_main_menu_handle->center_x + pdivoom_lvgl_top_main_menu_handle->radius * cos(arc_angle) - 80;	//减去图形长度的一半	
	(*y) = pdivoom_lvgl_top_main_menu_handle->center_y - pdivoom_lvgl_top_main_menu_handle->radius * sin(arc_angle) - 80;

}

void divoom_lvgl_top_main_menu_calculate_opa(int circle_angle, int* img_opa)
{
	int opa;
	int opa_25 = 25 * LV_OPA_100/100;
	int opa_75 = 75 * LV_OPA_100/100;

	if(circle_angle >= 0 && circle_angle < 30)
	{
		opa = opa_25 + circle_angle * (LV_OPA_50 - opa_25)/30;
	}
	else if(circle_angle >= 30 && circle_angle < 56)
	{
		opa = LV_OPA_50 + (circle_angle - 30) * (opa_75 - LV_OPA_50)/26;
	}
	else if(circle_angle >= 56 && circle_angle < 90)
	{
		opa = opa_75 + (circle_angle - 56) * (LV_OPA_100 - opa_75)/34;
	}
	else if(circle_angle >= 90 && circle_angle < 124)
	{
		opa = LV_OPA_100 - (circle_angle - 90) * (LV_OPA_100 - opa_75)/34;
	}
	else if(circle_angle >= 124 && circle_angle < 150)
	{
		opa = opa_75 - (circle_angle - 124) * (opa_75 - LV_OPA_50)/26;
	}
	else if(circle_angle >= 150 && circle_angle < 180)
	{
		opa = LV_OPA_50 - (circle_angle - 150) * (LV_OPA_50 - opa_25)/30;
	}
	else if(circle_angle == 180)
	{
		opa = opa_25;
	}
	else
	{
		opa = LV_OPA_100;
	}

	(*img_opa) = opa;
}

//计算图片缩小倍数
void divoom_lvgl_top_main_menu_calculate_zoom(int circle_angle, int* img_zoom)
{
	int zoom;
	int zoom_25 = 25 * 256/100;
	int zoom_50 = 50 * 256/100;
	int zoom_75 = 75 * 256/100;

	if(circle_angle >= 0 && circle_angle < 30)	//25%~50%
	{
		zoom = zoom_25 + circle_angle * (zoom_50 - zoom_25)/30;
	}
	else if(circle_angle >= 30 && circle_angle < 56) //50%~75%
	{
		zoom = zoom_50 + (circle_angle - 30) * (zoom_75 - zoom_50)/26;
	}
	else if(circle_angle >= 56 && circle_angle < 90) //75%~100%
	{
		zoom = zoom_75 + (circle_angle - 56) * (256 - zoom_75)/34;
	}
	else if(circle_angle >= 90 && circle_angle < 124)
	{
		//zoom = DIVOOM_TOP_MAIN_MENU_ZOOM_VALUE_80 + (120 - circle_angle) * (256 - DIVOOM_TOP_MAIN_MENU_ZOOM_VALUE_80)/30;
		zoom = 256 - (circle_angle - 90) * (256 - zoom_75)/34;
	}
	else if(circle_angle >= 124 && circle_angle < 150)
	{
		//zoom = DIVOOM_TOP_MAIN_MENU_ZOOM_VALUE_70 + (120 + 30 - circle_angle) * (DIVOOM_TOP_MAIN_MENU_ZOOM_VALUE_80 - DIVOOM_TOP_MAIN_MENU_ZOOM_VALUE_70)/30;
		zoom = zoom_75 - (circle_angle - 124) * (zoom_75 - zoom_50)/26;
	}
	else if(circle_angle >= 150 && circle_angle < 180)
	{
		zoom = zoom_50 - (circle_angle - 150) * (zoom_50 - zoom_25)/30;
	}
	else if(circle_angle == 180)
	{
		zoom = zoom_25;
	}
	else
	{
		zoom = 256;
	}
			
	(*img_zoom) = zoom;
}

void divoom_lvgl_top_main_menu_calculate_img_angle(int circle_angle, int* img_angle)
{
	int angle;	//图片旋转角度

	if(circle_angle >= 0 && circle_angle < 90)		//顺时针旋转图片
	{
		angle = 90 - circle_angle;	
	}
	else if(circle_angle >= 90 && circle_angle <= 180) //逆时针旋转图片
	{
		angle = 90 - circle_angle;
	}
	else
	{
		angle = 0;
	}

	(*img_angle) = angle * 10;
}

//逆时针旋转30度动画回调
void divoom_lvgl_top_main_angle_pre_anim_cb(void* var, int32_t v)
{
	int i, x, y, angle, zoom, opa;
	int diff_angle, circle_angle;

	if(pdivoom_lvgl_top_main_menu_handle == NULL)
	{
		return;
	}
	
	for(i = 1;i < 7; i++)
	{
		circle_angle = pdivoom_lvgl_top_main_menu_handle->pre_circle_angle[i];

		// v 0 30
		if(circle_angle == 30 || circle_angle == 124)	//减慢
		{
			circle_angle = circle_angle + v * 26 / 30;
		}
		else if(circle_angle == 56 || circle_angle == 90)	//加快
		{
			circle_angle = circle_angle + v * 34 / 30;
		}
		else	//速度不变
		{
			circle_angle += v;	
		}
		
		if(circle_angle >= 165)//逆时针转超出放到右边起点
		{
			circle_angle -= 180;
		}

		//divoom_print("v = %d\n", circle_angle - pdivoom_lvgl_top_main_menu_handle->pre_circle_angle[i]);
		
		divoom_lvgl_top_main_menu_calculate_opa(circle_angle, &opa);
		lv_obj_set_style_img_opa(pdivoom_lvgl_top_main_menu_handle->select_img[i], opa, 0);
		
		divoom_lvgl_top_main_menu_calculate_img_angle(circle_angle, &angle);
		lv_img_set_angle(pdivoom_lvgl_top_main_menu_handle->select_img[i], angle);

		divoom_lvgl_top_main_menu_calculate_zoom(circle_angle, &zoom);
		lv_img_set_zoom(pdivoom_lvgl_top_main_menu_handle->select_img[i], zoom);
		
		divoom_lvgl_top_main_menu_calculate_coordinate(circle_angle, &x, &y);
		lv_obj_set_pos(pdivoom_lvgl_top_main_menu_handle->select_img[i], x, y);

	}

}

void divoom_lvgl_top_main_menu_pre_anim_ready_cb(struct _lv_anim_t *a)
{
	if(pdivoom_lvgl_top_main_menu_handle == NULL)
	{
		return;
	}
	
	pdivoom_lvgl_top_main_menu_handle->anim_end_flag = 1;
}


//逆时针旋转30度动画
void divoom_lvgl_top_main_menu_angle_pre_anim(void)
{
	lv_anim_t a;

	if(pdivoom_lvgl_top_main_menu_handle->anim_end_flag == 0)
	{
		return;
	}

	pdivoom_lvgl_top_main_menu_handle->exit_time = 0;
	pdivoom_lvgl_top_main_menu_handle->anim_end_flag = 0;

	pdivoom_lvgl_top_main_menu_handle->select_id ++;
    if(pdivoom_lvgl_top_main_menu_handle->select_id >= sizeof(divoom_top_main_img_list)/sizeof(divoom_top_main_img_list[0]))
    {
        pdivoom_lvgl_top_main_menu_handle->select_id = 0;
    }

	lv_label_set_text(pdivoom_lvgl_top_main_menu_handle->label_obj, divoom_menu_control_get_disp_str(divoom_top_main_mode_str[pdivoom_lvgl_top_main_menu_handle->select_id]));
	
	lv_anim_init(&a);
 	lv_anim_set_exec_cb(&a, divoom_lvgl_top_main_angle_pre_anim_cb);
 	lv_anim_set_time(&a, 300);
 	lv_anim_set_values(&a, 0, 30);			//30度
	lv_anim_set_path_cb(&a, lv_anim_path_linear);
	lv_anim_set_ready_cb(&a, divoom_lvgl_top_main_menu_pre_anim_ready_cb);

	for(int i = 1; i < 7; i++)
	{
		pdivoom_lvgl_top_main_menu_handle->pre_circle_angle[i] = pdivoom_lvgl_top_main_menu_handle->circle_angle[i];

		if(pdivoom_lvgl_top_main_menu_handle->circle_angle[i] == 30 || pdivoom_lvgl_top_main_menu_handle->circle_angle[i] == 124)
		{
			pdivoom_lvgl_top_main_menu_handle->circle_angle[i] += 26;	//转动26度
		}
		else if(pdivoom_lvgl_top_main_menu_handle->circle_angle[i] == 90 || pdivoom_lvgl_top_main_menu_handle->circle_angle[i] == 56)
		{
			pdivoom_lvgl_top_main_menu_handle->circle_angle[i] += 34;	//转动34度
		}
		else
		{
			pdivoom_lvgl_top_main_menu_handle->circle_angle[i] += 30;
		}
		
		if(pdivoom_lvgl_top_main_menu_handle->circle_angle[i] > 150)
		{
			pdivoom_lvgl_top_main_menu_handle->circle_angle[i] = 0;
		}
	}
	
	lv_anim_start(&a);
}

//顺时针旋转30度动画回调
void divoom_lvgl_top_main_angle_next_anim_cb(void* var, int32_t v)
{
	int i, x, y, angle, zoom, opa;
	int diff_angle, circle_angle;

	if(pdivoom_lvgl_top_main_menu_handle == NULL)
	{
		return;
	}
	
	for(i = 1;i < 7; i++)
	{
		circle_angle = pdivoom_lvgl_top_main_menu_handle->pre_circle_angle[i];
				
		// v 0 -30
		if(circle_angle == 56 || circle_angle == 150)	//减慢
		{
			circle_angle = circle_angle + v * 26 / 30;
		}
		else if(circle_angle == 124 || circle_angle == 90)	//加快
		{
			circle_angle = circle_angle + v * 34 / 30;
		}
		else	
		{
			circle_angle += v;
		}
				
		if(circle_angle <= 0) //顺时针转超出放到左边起点
		{
			circle_angle += 180;
		}

		//divoom_print("v = %d\n", circle_angle - pdivoom_lvgl_top_main_menu_handle->pre_circle_angle[i]);
		
		divoom_lvgl_top_main_menu_calculate_opa(circle_angle, &opa);
		lv_obj_set_style_img_opa(pdivoom_lvgl_top_main_menu_handle->select_img[i], opa, 0);
		
		divoom_lvgl_top_main_menu_calculate_img_angle(circle_angle, &angle);
		lv_img_set_angle(pdivoom_lvgl_top_main_menu_handle->select_img[i], angle);

		divoom_lvgl_top_main_menu_calculate_zoom(circle_angle, &zoom);
		lv_img_set_zoom(pdivoom_lvgl_top_main_menu_handle->select_img[i], zoom);
		
		divoom_lvgl_top_main_menu_calculate_coordinate(circle_angle, &x, &y);
		lv_obj_set_pos(pdivoom_lvgl_top_main_menu_handle->select_img[i], x, y);

	}

}

void divoom_lvgl_top_main_menu_next_anim_ready_cb(struct _lv_anim_t *a)
{
	if(pdivoom_lvgl_top_main_menu_handle == NULL)
	{
		return;
	}

	pdivoom_lvgl_top_main_menu_handle->anim_end_flag = 1;
}

//顺时针旋转30度动画
void divoom_lvgl_top_main_menu_angle_next_anim(void)
{
	lv_anim_t a;

	if(pdivoom_lvgl_top_main_menu_handle->anim_end_flag == 0)
	{
		return;
	}
	
	pdivoom_lvgl_top_main_menu_handle->exit_time = 0;
	pdivoom_lvgl_top_main_menu_handle->anim_end_flag = 0;

	if(pdivoom_lvgl_top_main_menu_handle->select_id == 0)
    {
        pdivoom_lvgl_top_main_menu_handle->select_id = sizeof(divoom_top_main_img_list)/sizeof(divoom_top_main_img_list[0]) - 1;
    }
    else
    {
        pdivoom_lvgl_top_main_menu_handle->select_id --;
    }

	lv_label_set_text(pdivoom_lvgl_top_main_menu_handle->label_obj, divoom_menu_control_get_disp_str(divoom_top_main_mode_str[pdivoom_lvgl_top_main_menu_handle->select_id]));
	
	lv_anim_init(&a);
 	lv_anim_set_exec_cb(&a, divoom_lvgl_top_main_angle_next_anim_cb);
 	lv_anim_set_time(&a, 300);
 	lv_anim_set_values(&a, 0, -30);			//-30度
	lv_anim_set_path_cb(&a, lv_anim_path_linear);
	lv_anim_set_ready_cb(&a, divoom_lvgl_top_main_menu_next_anim_ready_cb);

	for(int i = 1; i < 7; i++)
	{
		pdivoom_lvgl_top_main_menu_handle->pre_circle_angle[i] = pdivoom_lvgl_top_main_menu_handle->circle_angle[i];

		if(pdivoom_lvgl_top_main_menu_handle->circle_angle[i] == 56 || pdivoom_lvgl_top_main_menu_handle->circle_angle[i] == 150)
		{
			pdivoom_lvgl_top_main_menu_handle->circle_angle[i] -= 26;
		}
		else if(pdivoom_lvgl_top_main_menu_handle->circle_angle[i] == 124 || pdivoom_lvgl_top_main_menu_handle->circle_angle[i] == 90)
		{
			pdivoom_lvgl_top_main_menu_handle->circle_angle[i] -= 34;
		}
		else
		{
			pdivoom_lvgl_top_main_menu_handle->circle_angle[i] -= 30;
		}
		
		if(pdivoom_lvgl_top_main_menu_handle->circle_angle[i] < 0)
		{
			pdivoom_lvgl_top_main_menu_handle->circle_angle[i] = 150;
		}
	}

	lv_anim_start(&a);
}

void divoom_lvgl_top_main_menu_del_menu(void)
{
	if(pdivoom_lvgl_top_main_menu_handle->menu_exit_timer)
	{
		lv_timer_del(pdivoom_lvgl_top_main_menu_handle->menu_exit_timer);
	}
    divoom_lvgl_top_control_remove_top(pdivoom_lvgl_top_main_menu_handle);
	lv_obj_del_async(pdivoom_lvgl_top_main_menu_handle->main_obj);
	divoom_system_free(pdivoom_lvgl_top_main_menu_handle);
        divoom_local_movie_play_ctrl(1);
	pdivoom_lvgl_top_main_menu_handle = NULL;
}

static void divoom_lvgl_top_main_menu_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
	
	if(code == LV_EVENT_KEY)
    {
        char *key_info = (char *)lv_event_get_param(e);
        char c = key_info[0];

        if(pdivoom_lvgl_top_main_menu_handle == NULL || pdivoom_lvgl_top_main_menu_handle->anim_end_flag == 0)
        {
            divoom_print("aj test!\n");
			return;
        }
		if(c == DIVOOM_LVGL_KEY_NEXT) 
		{
            divoom_lvgl_top_main_menu_angle_pre_anim();
        }
		else if(c == DIVOOM_LVGL_KEY_PRE)
		{
			divoom_lvgl_top_main_menu_angle_next_anim();
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{	
			int select_id = pdivoom_lvgl_top_main_menu_handle->select_id;
			divoom_lvgl_top_main_menu_del_menu();
            if(divoom_top_main_next_menu_id[select_id] == DIVOOM_MENU_CONTROL_ID_SYSTEM_SETING_MENU)
            {
                divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_ENTER_MENU_SETTING);
            }
            else
            {
			    divoom_menu_control_clearall_load(divoom_top_main_next_menu_id[select_id], NULL, 0, 0);
            }
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_lvgl_top_main_menu_del_menu();
		}
        else if(c == DIVOOM_LVGL_KEY_HOME)
        {
            divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
        }
    }
	
}

void divoom_lvgl_top_main_menu_image_handle_exit(void)
{
	//for(int i = 0;i < 7; i++)
	//{
	//	divoom_lvgl_top_main_menu_clear_image_handle(phandle->pdisp_handle[i]);
	//}
}

void divoom_lvgl_top_main_menu_time_handle(lv_timer_t *exit_timer)
{
	if(pdivoom_lvgl_top_main_menu_handle != NULL)
	{
		pdivoom_lvgl_top_main_menu_handle->exit_time++;

		if(pdivoom_lvgl_top_main_menu_handle->exit_time >= DIVOOM_EXIT_MENU_TIME_OUT)//主菜单不变动自动退出
		{
			if(pdivoom_lvgl_top_main_menu_handle->anim_end_flag == 1)
			{
				divoom_lvgl_top_main_menu_del_menu();
			}
		}
	}
}

void divoom_lvgl_top_main_menu_create_icon(void)
{
	int i, x, y, zoom, angle, opa;
	
	pdivoom_lvgl_top_main_menu_handle->cout_obj = lv_btn_create(pdivoom_lvgl_top_main_menu_handle->main_obj);

	lv_obj_remove_style_all(pdivoom_lvgl_top_main_menu_handle->cout_obj);
	lv_obj_set_style_pad_all(pdivoom_lvgl_top_main_menu_handle->cout_obj, 0, 0);
	lv_obj_set_style_border_width(pdivoom_lvgl_top_main_menu_handle->cout_obj, 0, 0);
	lv_obj_set_style_outline_width(pdivoom_lvgl_top_main_menu_handle->cout_obj, 0, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_shadow_width(pdivoom_lvgl_top_main_menu_handle->cout_obj, 0, 0);
	lv_obj_set_style_bg_opa(pdivoom_lvgl_top_main_menu_handle->cout_obj, LV_OPA_0, 0);
	lv_obj_set_pos(pdivoom_lvgl_top_main_menu_handle->cout_obj, 0, 0);
    lv_obj_set_size(pdivoom_lvgl_top_main_menu_handle->cout_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_add_event_cb(pdivoom_lvgl_top_main_menu_handle->cout_obj, divoom_lvgl_top_main_menu_event_cb, LV_EVENT_KEY, NULL);

	lv_obj_t *bg_img = lv_img_create(pdivoom_lvgl_top_main_menu_handle->cout_obj);
	divoom_menu_control_lv_img_set_src(bg_img, "75/75_top_bg.png");
	lv_obj_set_pos(bg_img, 0, 937);

	lv_obj_t *label = lv_label_create(bg_img);
	lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(30, DIVOOM_MENU_CONTROL_ID_MAIN), 0);
	//lv_obj_set_style_text_color(label, lv_color_hex(0x053A39), 0);
	lv_obj_set_style_text_color(label, lv_color_black(), 0);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_size(label, 320, 60);
	lv_obj_set_pos(label, 238, 238+10);
	lv_label_set_text(label, divoom_menu_control_get_disp_str(divoom_top_main_mode_str[pdivoom_lvgl_top_main_menu_handle->select_id]));
	pdivoom_lvgl_top_main_menu_handle->label_obj = label;

	for(i = 1;i < 7; i++)
	{
		pdivoom_lvgl_top_main_menu_handle->select_img[i] = lv_img_create(pdivoom_lvgl_top_main_menu_handle->cout_obj);
		
		if(i == 0)
		{
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_main_menu_handle->select_img[i], divoom_top_main_img_list[4]);
		}
		else if(i >= 1 && i < 6)
		{
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_main_menu_handle->select_img[i], divoom_top_main_img_list[i-1]);
		}
		else if(i == 6)
		{
			divoom_menu_control_lv_img_set_src(pdivoom_lvgl_top_main_menu_handle->select_img[i], divoom_top_main_img_list[5]);
		}

		if(i == 2)	//角度不一样
		{
			pdivoom_lvgl_top_main_menu_handle->pre_circle_angle[i] = 180 - 30*i + 4;
			pdivoom_lvgl_top_main_menu_handle->circle_angle[i] = 180 - 30*i + 4;
		}
		else if(i == 4)
		{
			pdivoom_lvgl_top_main_menu_handle->pre_circle_angle[i] = 180 - 30*i - 4;
			pdivoom_lvgl_top_main_menu_handle->circle_angle[i] = 180 - 30*i - 4;
		}
		else
		{
			pdivoom_lvgl_top_main_menu_handle->pre_circle_angle[i] = 180 - 30*i;
			pdivoom_lvgl_top_main_menu_handle->circle_angle[i] = 180 - 30*i;
		}

		divoom_lvgl_top_main_menu_calculate_opa(pdivoom_lvgl_top_main_menu_handle->circle_angle[i], &opa);
		lv_obj_set_style_img_opa(pdivoom_lvgl_top_main_menu_handle->select_img[i], opa, 0);
		
		divoom_lvgl_top_main_menu_calculate_img_angle(pdivoom_lvgl_top_main_menu_handle->circle_angle[i], &angle);
		lv_img_set_angle(pdivoom_lvgl_top_main_menu_handle->select_img[i], angle);

		divoom_lvgl_top_main_menu_calculate_zoom(pdivoom_lvgl_top_main_menu_handle->circle_angle[i], &zoom);
		lv_img_set_zoom(pdivoom_lvgl_top_main_menu_handle->select_img[i], zoom);
		
		//lv_obj_set_size(phandle->select_img[i], 128, 128);
		divoom_lvgl_top_main_menu_calculate_coordinate(pdivoom_lvgl_top_main_menu_handle->circle_angle[i], &x, &y);
		lv_obj_set_pos(pdivoom_lvgl_top_main_menu_handle->select_img[i], x, y);

	}
	
}

void divoom_lvgl_top_main_menu_init(void)
{
	divoom_get_lvgl_lock();

	if(pdivoom_lvgl_top_main_menu_handle != NULL)
	{
		divoom_free_lvgl_lock();
		return;
	}

	pdivoom_lvgl_top_main_menu_handle = (Pdivoom_lvgl_top_main_menu_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_top_main_menu_ctrl));

	if(pdivoom_lvgl_top_main_menu_handle == NULL)
	{
		divoom_free_lvgl_lock();
		return;
	}
    divoom_local_movie_play_ctrl(0);
	memset(pdivoom_lvgl_top_main_menu_handle, 0, sizeof(divoom_lvgl_top_main_menu_ctrl));

	pdivoom_lvgl_top_main_menu_handle->center_x = 400;
	pdivoom_lvgl_top_main_menu_handle->center_y = 1367;
	pdivoom_lvgl_top_main_menu_handle->radius = 370;
	pdivoom_lvgl_top_main_menu_handle->select_id = 2;
	pdivoom_lvgl_top_main_menu_handle->anim_end_flag = 1;
	pdivoom_lvgl_top_main_menu_handle->exit_time = 0;
	
	pdivoom_lvgl_top_main_menu_handle->main_obj = lv_obj_create(lv_layer_top()); //在顶层创建主页面

	lv_obj_set_style_bg_opa(pdivoom_lvgl_top_main_menu_handle->main_obj, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(pdivoom_lvgl_top_main_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_border_width(pdivoom_lvgl_top_main_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_shadow_width(pdivoom_lvgl_top_main_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_outline_width(pdivoom_lvgl_top_main_menu_handle->main_obj, 0, 0);
	lv_obj_clear_flag(pdivoom_lvgl_top_main_menu_handle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(pdivoom_lvgl_top_main_menu_handle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(pdivoom_lvgl_top_main_menu_handle->main_obj, 0, 0);
	
	
	divoom_lvgl_top_main_menu_create_icon();

	pdivoom_lvgl_top_main_menu_handle->menu_exit_timer = lv_timer_create(divoom_lvgl_top_main_menu_time_handle, 1000, NULL);
	
	//lv_group_focus_obj(pdivoom_lvgl_top_main_menu_handle->cout_obj);
	divoom_lvgl_top_control_add_top(pdivoom_lvgl_top_main_menu_handle, pdivoom_lvgl_top_main_menu_handle->cout_obj);
	divoom_free_lvgl_lock();
}


void divoom_lvgl_top_main_menu_exit(void)
{
	if(pdivoom_lvgl_top_main_menu_handle != NULL)
	{
		if(pdivoom_lvgl_top_main_menu_handle->menu_exit_timer)
		{
			lv_timer_del(pdivoom_lvgl_top_main_menu_handle->menu_exit_timer);
		}
        divoom_lvgl_top_control_remove_top(pdivoom_lvgl_top_main_menu_handle);
		lv_obj_del_async(pdivoom_lvgl_top_main_menu_handle->main_obj);
		divoom_system_free(pdivoom_lvgl_top_main_menu_handle);
        divoom_local_movie_play_ctrl(1);
		pdivoom_lvgl_top_main_menu_handle = NULL;
	}
}
#endif

/**
 * 白噪音播放主页面：壁炉背景 + “白噪音进入默认播放”
 * 按确认键或左右旋转进入列表页 divoom_lvgl_white_noise_list_menu
 */
#include "divoom.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_white_noise_play_main.h"
#include "divoom_white_noise_player.h"
#include "divoom_local_movie_play.h"

/* 背景图：与九宫格篝火一致，或使用专用播放背景图 */
#define WHITE_NOISE_PLAY_MAIN_BG_IMG   "93/93_fire.png"

//"篝火",
//"海浪",
//"涟漪",
//"水波纹",
//"光影",
//"冥想",
//"光影流动",
//"雨滴",
//"溪流",


const char *divoom_lvgl_white_noise_effect_bg_name[] =
{
    "/usr/share/divoom_app/white_noise_movie/篝火.mp4",
    "/usr/share/divoom_app/white_noise_movie/海浪.mp4",
    "/usr/share/divoom_app/white_noise_movie/涟漪.mp4",
    "/usr/share/divoom_app/white_noise_movie/水波纹.mp4",
    "/usr/share/divoom_app/white_noise_movie/光影.mp4",
    "/usr/share/divoom_app/white_noise_movie/冷色冥想.mp4",
    "/usr/share/divoom_app/white_noise_movie/光影流动.mp4",
    "/usr/share/divoom_app/white_noise_movie/雨滴.mp4",
    "/usr/share/divoom_app/white_noise_movie/溪流.mp4",
};

#define WHITE_NOISE_BG_CNT  ((int)(sizeof(divoom_lvgl_white_noise_effect_bg_name) / sizeof((divoom_lvgl_white_noise_effect_bg_name)[0])))

static int divoom_lvgl_white_noise_effect_to_bg_index(int effect_idx)
{
	if (effect_idx < 0)
		return 0;
	if (effect_idx >= WHITE_NOISE_BG_CNT)
		return WHITE_NOISE_BG_CNT -1;
	return effect_idx;
}

typedef struct _divoom_lvgl_white_noise_play_main_ctrl
{
	int menu_id;
	int first_flag;
	int last_video_idx;
	lv_obj_t *main_screen;
	lv_obj_t *bg_img;
	void *focus_obj;
} divoom_lvgl_white_noise_play_main_ctrl, *Pdivoom_lvgl_white_noise_play_main_ctrl;

static void divoom_lvgl_white_noise_play_main_switch_effect(Pdivoom_lvgl_white_noise_play_main_ctrl phandle, int step)
{
	int cur_effect;
	int next_effect;
	int sleep_minutes;
	int vidx;

	if (phandle == NULL || WHITE_NOISE_BG_CNT <= 0 || (step != 1 && step != -1))
		return;

	cur_effect = divoom_white_noise_player_get_current_effect();
	if (cur_effect < 0 || cur_effect >= WHITE_NOISE_BG_CNT)
		cur_effect = 0;

	next_effect = (cur_effect + step + WHITE_NOISE_BG_CNT) % WHITE_NOISE_BG_CNT;
	sleep_minutes = divoom_white_noise_player_get_sleep_timer_minutes();

	divoom_white_noise_player_start(next_effect, sleep_minutes);

	vidx = divoom_lvgl_white_noise_effect_to_bg_index(next_effect);
	divoom_local_movie_play_play((char *)divoom_lvgl_white_noise_effect_bg_name[vidx], 0xFF, -1, NULL, NULL);
	phandle->first_flag = 1;
	phandle->last_video_idx = vidx;
}

static void divoom_lvgl_white_noise_play_main_event_cb(lv_event_t *e)
{
	Pdivoom_lvgl_white_noise_play_main_ctrl phandle = (Pdivoom_lvgl_white_noise_play_main_ctrl)lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);
	char *key_info = (char *)lv_event_get_param(e);
	char c = key_info ? (char)key_info[0] : 0;
	int *list_type = NULL;

	if (code != LV_EVENT_KEY)
		return;

	/* ESC：返回上一级（更多功能等） */
	if (c == DIVOOM_LVGL_KEY_ESC) 
	{
		divoom_menu_control_return_pre();
		return;
	}
	/* 确认键、左旋、右旋：进入白噪音列表页 */
	if (c == DIVOOM_LVGL_KEY_ENTER) //进入全部列表
	{
		list_type = divoom_system_malloc(sizeof(int));
		*list_type = 0;
		divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_WHITE_NOISE_LIST, (void *)list_type, 0, 0);
		return;
	}

	if (c == DIVOOM_LVGL_KEY_NEXT)
	{
		divoom_lvgl_white_noise_play_main_switch_effect(phandle, 1);
		return;
	}

	if (c == DIVOOM_LVGL_KEY_PRE)
	{
		divoom_lvgl_white_noise_play_main_switch_effect(phandle, -1);
		return;
	}
	
	if (c == DIVOOM_LVGL_KEY_FAVORITE && key_info[1] == PRESS_SHORT) //进入收藏列表
	{
		list_type = divoom_system_malloc(sizeof(int));
		*list_type = 1;
		divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_WHITE_NOISE_LIST, (void *)list_type, 0, 0); //谁用谁释放
		return;
	}

	if (c == DIVOOM_LVGL_KEY_FAVORITE && key_info[1] == PRESS_LONG)
	{
		//收藏当前选中的提示音
		divoom_white_noise_player_toggle_favorite(divoom_white_noise_player_get_current_effect());
		return;
	}

	if (key_info)
		divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
}

void *divoom_lvgl_white_noise_play_main_init(void *user_data, int menu_id)
{
	Pdivoom_lvgl_white_noise_play_main_ctrl phandle = divoom_system_malloc(sizeof(divoom_lvgl_white_noise_play_main_ctrl));
	if (phandle == NULL)
		return NULL;
	memset(phandle, 0, sizeof(*phandle));
	phandle->menu_id = menu_id;
	phandle->last_video_idx = -1;
	phandle->first_flag = 0;

	phandle->main_screen = lv_obj_create(NULL);
	lv_obj_set_size(phandle->main_screen, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_style_bg_color(phandle->main_screen, lv_color_hex(0x1a1a1a), 0);
	lv_obj_set_style_border_width(phandle->main_screen, 0, 0);
	lv_obj_set_style_pad_all(phandle->main_screen, 0, 0);
	lv_obj_clear_flag(phandle->main_screen, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_scrollbar_mode(phandle->main_screen, LV_SCROLLBAR_MODE_OFF);

	/* 可聚焦容器以接收按键 */
	phandle->focus_obj = lv_btn_create(phandle->main_screen);
	lv_obj_set_size(phandle->focus_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(phandle->focus_obj, 0, 0);
	lv_obj_set_style_bg_opa(phandle->focus_obj, LV_OPA_0, 0);
	lv_obj_set_style_border_width(phandle->focus_obj, 0, 0);
	lv_obj_add_flag(phandle->focus_obj, LV_OBJ_FLAG_CLICKABLE);
	lv_group_focus_obj(phandle->focus_obj);
	lv_obj_add_event_cb(phandle->focus_obj, divoom_lvgl_white_noise_play_main_event_cb, LV_EVENT_KEY, phandle);

	/* 提示音在 init 里启动；对应 mp4 背景在 disp 里与 divoom_lvgl_lyric_effect 一致启动 */
	divoom_white_noise_player_play_current_effect();

	return phandle;
}

void divoom_lvgl_white_noise_play_main_disp(void *handle, void *active_obj)
{
	Pdivoom_lvgl_white_noise_play_main_ctrl phandle = (Pdivoom_lvgl_white_noise_play_main_ctrl)handle;
	int vidx;

	if (phandle == NULL)
		return;

	divoom_menu_control_change_disp_mode(phandle->main_screen, 1);

	vidx = divoom_lvgl_white_noise_effect_to_bg_index(divoom_white_noise_player_get_current_effect());
//	if (phandle->first_flag && phandle->last_video_idx == vidx)
//	{
//		divoom_local_movie_play_ctrl(1);
//	}
//	else
	{
		divoom_local_movie_play_play((char *)divoom_lvgl_white_noise_effect_bg_name[vidx], 0xFF, -1, NULL, NULL);
		divoom_white_noise_player_play_current_effect();
		phandle->first_flag = 1;
		phandle->last_video_idx = vidx;
	}

	lv_scr_load(phandle->main_screen);
	lv_group_focus_obj(phandle->focus_obj);
}

void divoom_lvgl_white_noise_play_main_pause(void *handle)
{
	Pdivoom_lvgl_white_noise_play_main_ctrl phandle = (Pdivoom_lvgl_white_noise_play_main_ctrl)handle;
	if (phandle == NULL)
		return;
//	divoom_local_movie_play_ctrl(0);
	divoom_menu_control_change_disp_mode(phandle->main_screen, 0);
	divoom_local_movie_play_stop_movie();
	divoom_white_noise_player_stop();
}

void divoom_lvgl_white_noise_play_main_exit(void *handle)
{
	Pdivoom_lvgl_white_noise_play_main_ctrl phandle = (Pdivoom_lvgl_white_noise_play_main_ctrl)handle;
	if (phandle == NULL)
		return;

	lv_obj_del(phandle->main_screen);
	divoom_system_free(phandle);
}

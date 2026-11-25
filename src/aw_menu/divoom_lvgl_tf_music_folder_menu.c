#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_tf_music.h"
#include <string.h>

/* 右侧列表槽位数；旋钮焦点、环回、翻页、数组大小均用此值 */
#define DIVOOM_LVGL_TF_MUSIC_RIGHT_VISIBLE_ROWS   30

typedef struct _divoom_lvgl_tf_music_folder_menu_ctrl
{
    int menu_id;
    void *main_obj;
    void *time_handle;

    lv_style_t cout_style;
	lv_style_t normal_type;
	lv_style_t press_type;

	void *top_battery_img;

	lv_obj_t *left_folder_btn[DIVOOM_TF_MUSIC_MAX_ROOT];	//左侧文件夹按钮
	void *cur_active_obj;

	void *play_status_img;	//当前播放状态图片
	void *like_img;			//收藏图片
	void *play_order_img;	//播放顺序图片

	void *pdirect_handle;

	void *right_panel;
	void *right_music_btn[DIVOOM_LVGL_TF_MUSIC_RIGHT_VISIBLE_ROWS];
	void *right_music_img[DIVOOM_LVGL_TF_MUSIC_RIGHT_VISIBLE_ROWS];
	void *right_music_label[DIVOOM_LVGL_TF_MUSIC_RIGHT_VISIBLE_ROWS];
	void *right_music_like[DIVOOM_LVGL_TF_MUSIC_RIGHT_VISIBLE_ROWS];

	int right_first_index;  /* 槽位 0 对应 subtree offset */
	int right_focus_slot;   /* 焦点槽位 0 .. VISIBLE_ROWS-1 */

	int music_count;
	int folder_id;
	divoom_tf_entry page[DIVOOM_LVGL_TF_MUSIC_RIGHT_VISIBLE_ROWS];
	int page_n;
	Pdivoom_tf_music_item pcur_play_music;

	Pdivoom_sys_setting_info psys_config;

	int left_selected_index;   /* 0=全部 1=收藏 2+=一级目录 */
	int left_item_count;
	divoom_tf_entry hub[DIVOOM_TF_MUSIC_MAX_ROOT];
	int hub_count;

	lv_obj_t *play_bar_label;  /* 底部栏歌曲名，用于定时刷新 */
	int last_play_music_id;
	lv_timer_t *play_refresh_timer;

	int hour;
	int minute;
	lv_obj_t *time_label;
	lv_timer_t *menu_bar_timer;
}divoom_lvgl_tf_music_folder_menu_ctrl, *Pdivoom_lvgl_tf_music_folder_menu_ctrl;

static void divoom_lvgl_tf_music_folder_menu_refresh_right_slots(Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle);
static void divoom_lvgl_tf_music_folder_menu_update_music_list(Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle);
static void divoom_lvgl_tf_music_folder_menu_update_play_obj(Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle);

char *divoom_lvgl_tf_music_folder_menu_get_music_name(char *file_name)
{
    return divoom_tf_music_get_disp_name(file_name);
}

static int divoom_lvgl_tf_music_folder_load_hub(divoom_tf_entry *out, int cap)
{
	int n;
	int i;
	int keep;

	if (out == NULL || cap <= 0)
		return 0;
	if (divoom_database_tf_is_rebuilding())
		return 0;
	n = divoom_database_tf_dir_page(0, 0, cap, out, cap);
	keep = 0;
	for (i = 0; i < n; i++) {
		if (!out[i].is_dir || out[i].id <= 0)
			continue;
		if (strcasecmp(out[i].name, "tf") == 0)
			continue;
		if (keep != i)
			out[keep] = out[i];
		keep++;
	}
	return keep;
}

static int divoom_lvgl_tf_music_folder_id_from_left(Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle)
{
	int idx;

	if (phandle == NULL)
		return 0;
	idx = phandle->left_selected_index;
	if (idx <= 0)
		return 0;
#if DIVOOM_TF_MUSIC_FAVORITES_UI
	if (idx == 1)
		return -1;
	idx -= DIVOOM_TF_MUSIC_FOLDER_LEFT_BASE;
#else
	idx -= DIVOOM_TF_MUSIC_FOLDER_LEFT_BASE;
#endif
	if (idx < 0 || idx >= phandle->hub_count)
		return 0;
	return phandle->hub[idx].id;
}

/* 当前窗口内有效行数（不超过 VISIBLE_ROWS） */
static int divoom_lvgl_tf_music_folder_menu_right_visible_in_window(Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle)
{
	int left;

	if (phandle == NULL || phandle->music_count <= 0)
		return 0;
	left = phandle->music_count - phandle->right_first_index;
	if (left > DIVOOM_LVGL_TF_MUSIC_RIGHT_VISIBLE_ROWS)
		return DIVOOM_LVGL_TF_MUSIC_RIGHT_VISIBLE_ROWS;
	return left;
}

static int divoom_lvgl_tf_music_folder_menu_get_global_index(Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle)
{
	int g;

	if (phandle == NULL)
		return -1;
	g = phandle->right_first_index + phandle->right_focus_slot;
	if (g < 0 || g >= phandle->music_count)
		return -1;
	return g;
}

/* 左侧目录选中高亮（与 ENTER 切换目录时一致） */
static void divoom_lvgl_tf_music_folder_menu_apply_left_selection(Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle, int index)
{
	int k;

	if (phandle == NULL || index < 0 || index >= phandle->left_item_count)
		return;

	phandle->left_selected_index = index;
	for (k = 0; k < phandle->left_item_count; k++)
	{
		if (phandle->left_folder_btn[k] == NULL)
			continue;
		lv_obj_set_style_bg_color(phandle->left_folder_btn[k], lv_color_hex(0xD5D5D5), 0);
		lv_obj_set_style_bg_opa(phandle->left_folder_btn[k], (k == index) ? LV_OPA_100 : 0, 0);
	}
	if (phandle->left_folder_btn[index])
	{
		lv_obj_set_style_bg_color(phandle->left_folder_btn[index], lv_color_hex(0xE4E4E4), 0);
		lv_obj_set_style_bg_opa(phandle->left_folder_btn[index], LV_OPA_100, 0);
	}
}

/* 在当前目录列表内滚动到正在播放的曲目并高亮 */
static void divoom_lvgl_tf_music_folder_menu_sync_list_to_playing(Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle)
{
	int cur_id;
	int gi;
	int max_slot;
	int focus_slot;

	if (phandle == NULL)
		return;

	cur_id = divoom_tf_music_get_playing_music_id();
	if (cur_id <= 0 || phandle->music_count <= 0)
		return;

	gi = divoom_database_tf_track_row(phandle->folder_id, cur_id);
	if (gi < 0 || gi >= phandle->music_count)
		return;

	max_slot = divoom_lvgl_tf_music_folder_menu_right_visible_in_window(phandle);
	if (max_slot <= 0)
		return;

	if (gi < phandle->right_first_index)
		phandle->right_first_index = gi;
	else if (gi >= phandle->right_first_index + max_slot)
		phandle->right_first_index = gi - max_slot + 1;

	focus_slot = gi - phandle->right_first_index;
	if (focus_slot >= max_slot)
		focus_slot = max_slot - 1;
	if (focus_slot < 0)
		focus_slot = 0;
	phandle->right_focus_slot = focus_slot;

	divoom_lvgl_tf_music_folder_menu_refresh_right_slots(phandle);
	if (phandle->right_music_btn[focus_slot])
		lv_group_focus_obj(phandle->right_music_btn[focus_slot]);
}

/* 左侧目录 + 右侧列表与当前 play_order / 正在播放曲目对齐 */
static void divoom_lvgl_tf_music_folder_menu_sync_to_playback(Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle)
{
	int play_order;
	int left_idx;

	if (phandle == NULL)
		return;

	play_order = divoom_tf_music_get_play_order();
	left_idx = divoom_tf_music_folder_play_order_to_left(play_order);
	if (left_idx < 0 || left_idx >= phandle->left_item_count)
		left_idx = 0;

	if (left_idx != phandle->left_selected_index)
	{
		divoom_lvgl_tf_music_folder_menu_apply_left_selection(phandle, left_idx);
		divoom_lvgl_tf_music_folder_menu_update_music_list(phandle);
	}
	else
	{
		divoom_lvgl_tf_music_folder_menu_apply_left_selection(phandle, left_idx);
	}

	divoom_lvgl_tf_music_folder_menu_sync_list_to_playing(phandle);
	divoom_lvgl_tf_music_folder_menu_update_play_obj(phandle);
}

/* 按 right_first_index 重绑各槽文案与图标，并恢复焦点 */
static void divoom_lvgl_tf_music_folder_menu_refresh_right_slots(Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle)
{
	int i;
	int cur_id;
	int max_slot;
	lv_obj_t *item_cont;

	if (phandle == NULL)
		return;

	cur_id = divoom_tf_music_get_playing_music_id();

	if (phandle->right_first_index < 0)
		phandle->right_first_index = 0;
	memset(phandle->page, 0, sizeof(phandle->page));
	phandle->page_n = 0;
	if (phandle->music_count > 0 && phandle->folder_id >= 0 && !divoom_database_tf_is_rebuilding())
	{
		phandle->page_n = divoom_database_tf_subtree_page(phandle->folder_id,
		                                                 phandle->right_first_index,
		                                                 DIVOOM_LVGL_TF_MUSIC_RIGHT_VISIBLE_ROWS,
		                                                 phandle->page,
		                                                 DIVOOM_LVGL_TF_MUSIC_RIGHT_VISIBLE_ROWS);
	}

	for (i = 0; i < DIVOOM_LVGL_TF_MUSIC_RIGHT_VISIBLE_ROWS; i++) {
		int gi = phandle->right_first_index + i;

		item_cont = phandle->right_music_btn[i];
		if (item_cont == NULL)
			continue;

		if (i >= phandle->page_n || phandle->page[i].id <= 0 || phandle->page[i].is_dir
			|| gi < 0 || gi >= phandle->music_count) {
			lv_obj_add_flag(item_cont, LV_OBJ_FLAG_HIDDEN);
			continue;
		}

		lv_obj_clear_flag(item_cont, LV_OBJ_FLAG_HIDDEN);

		if (phandle->right_music_label[i] && phandle->page[i].name[0] != '\0') {
			lv_label_set_text(phandle->right_music_label[i],
				divoom_lvgl_tf_music_folder_menu_get_music_name(phandle->page[i].name));
		}

		if (phandle->right_music_img[i]) {
			if (phandle->page[i].id == cur_id) {
				divoom_menu_control_lv_img_set_src(phandle->right_music_img[i], "92/92_playing.png");
				lv_obj_set_style_bg_color(item_cont, lv_color_hex(0xC1C1C1), 0);
			} else {
				divoom_menu_control_lv_img_set_src(phandle->right_music_img[i], "92/92_music.png");
				lv_obj_set_style_bg_color(item_cont, lv_color_hex(0xE4E4E4), 0);
			}
		}

#if DIVOOM_TF_MUSIC_FAVORITES_UI
		if (phandle->right_music_like[i])
			divoom_menu_control_lv_img_set_src(phandle->right_music_like[i], "92/92_unlike.png");
#endif
	}

	max_slot = divoom_lvgl_tf_music_folder_menu_right_visible_in_window(phandle) - 1;
	if (max_slot < 0)
		max_slot = 0;
	if (phandle->right_focus_slot < 0)
		phandle->right_focus_slot = 0;
	if (phandle->right_focus_slot >= DIVOOM_LVGL_TF_MUSIC_RIGHT_VISIBLE_ROWS)
		phandle->right_focus_slot = DIVOOM_LVGL_TF_MUSIC_RIGHT_VISIBLE_ROWS - 1;
	if (phandle->right_focus_slot > max_slot)
		phandle->right_focus_slot = max_slot;

	if (phandle->right_music_btn[phandle->right_focus_slot])
		lv_group_focus_obj(phandle->right_music_btn[phandle->right_focus_slot]);
}

void divoom_lvgl_tf_music_folder_set_battery_level_img(void *image_obj)
{
	if(divoom_is_in_charge())
	{
		if(divoom_power_get_level() == PWR_LEVEL_0)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_c1.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_1)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_c2.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_2)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_c3.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_3)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_c4.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_4)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_c5.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_5)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_c6.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_6)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_c7.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_7)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_c8.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_8)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_c9.png");
		}
		else
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_c10.png");
		}
	}
	else
	{
		if(divoom_power_get_level() == PWR_LEVEL_0)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_d1.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_1)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_d2.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_2)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_d3.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_3)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_d4.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_4)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_d5.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_5)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_d6.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_6)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_d7.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_7)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_d8.png");
		}
		else if(divoom_power_get_level() == PWR_LEVEL_8)
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_d9.png");
		}
		else
		{
			divoom_menu_control_lv_img_set_src(image_obj, "26/26_battery_d10.png");
		}
	}
}


void divoom_lvgl_tf_music_folder_menu_refresh_menu_bar(lv_timer_t *menu_bar_timer)
{
    int hour;
    Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle = (Pdivoom_lvgl_tf_music_folder_menu_ctrl)lv_timer_get_user_data(menu_bar_timer);
	time_t time_now;
	struct tm p_now;
	
    time_now = time(NULL);
	localtime_r(&time_now, &p_now);
	hour = p_now.tm_hour;
    if(divoom_main_get_time_flag() == 0)
    {
        if(hour != 12)
        {
            hour = hour % 12;
            if(hour == 0)
            {
                hour = 12;
            }
        }
    }

    if(hour != phandle->hour || p_now.tm_min != phandle->minute)
    {
        phandle->hour = hour;
        phandle->minute = p_now.tm_min;
        lv_label_set_text_fmt(phandle->time_label, "%02d:%02d", hour, phandle->minute);
    }

	divoom_lvgl_tf_music_folder_set_battery_level_img(phandle->top_battery_img);
}

/* 刷新底部栏和右侧列表的当前播放状态（定时器内或切歌后调用） */
static void divoom_lvgl_tf_music_folder_menu_update_play_obj(Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle)
{
	int i;
	int cur_id;
	lv_obj_t *item_cont;

	if (phandle == NULL)
		return;
	
	phandle->pcur_play_music = divoom_tf_music_get_playing_music_info();
	phandle->last_play_music_id = divoom_tf_music_get_playing_music_id();

	if (phandle->play_bar_label) 
	{
		if (phandle->pcur_play_music && phandle->pcur_play_music->music_name)
			lv_label_set_text(phandle->play_bar_label, divoom_lvgl_tf_music_folder_menu_get_music_name(phandle->pcur_play_music->music_name));
		else
			lv_label_set_text(phandle->play_bar_label, divoom_menu_control_get_disp_str("UnknownMusicName"));
	}

	if (phandle->play_status_img)
	{
		if (divoom_tf_music_get_music_status())
			divoom_menu_control_lv_img_set_src(phandle->play_status_img, "34/34_pause.png");
		else
			divoom_menu_control_lv_img_set_src(phandle->play_status_img, "34/34_play.png");
	}

	cur_id = divoom_tf_music_get_playing_music_id();

	for (i = 0; i < DIVOOM_LVGL_TF_MUSIC_RIGHT_VISIBLE_ROWS; i++)
	{
		if(phandle->pcur_play_music == NULL)
		{
			break;
		}

		if (phandle->right_music_btn[i] == NULL)
		{
			continue;
		}

		item_cont = phandle->right_music_btn[i];

		if (i < phandle->page_n && phandle->page[i].id == cur_id)
		{
			divoom_menu_control_lv_img_set_src(phandle->right_music_img[i], "92/92_playing.png");
			lv_obj_set_style_bg_color(item_cont, lv_color_hex(0xC1C1C1), 0);
		}
		else
		{
			divoom_menu_control_lv_img_set_src(phandle->right_music_img[i], "92/92_music.png");
			lv_obj_set_style_bg_color(item_cont, lv_color_hex(0xE4E4E4), 0);
		}
	}
}

static void divoom_lvgl_tf_music_folder_menu_play_refresh_cb(lv_timer_t *timer)
{
	Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle = (Pdivoom_lvgl_tf_music_folder_menu_ctrl)lv_timer_get_user_data(timer);
	int cur_id, i;
	void *item_cont;

	if (phandle == NULL)
		return;

	cur_id = divoom_tf_music_get_playing_music_id();

	if (cur_id != phandle->last_play_music_id)
	{
		divoom_lvgl_tf_music_folder_menu_sync_to_playback(phandle);
	}

#if DIVOOM_TF_MUSIC_FAVORITES_UI
	if (phandle->like_img)
	{
		if (phandle->pcur_play_music && phandle->pcur_play_music->like)
			divoom_menu_control_lv_img_set_src(phandle->like_img, "34/34_like.png");
		else
			divoom_menu_control_lv_img_set_src(phandle->like_img, "34/34_unlike.png");
	}
#endif

	if (phandle->play_status_img)
	{
		if (divoom_tf_music_get_music_status())
			divoom_menu_control_lv_img_set_src(phandle->play_status_img, "34/34_pause.png");
		else
			divoom_menu_control_lv_img_set_src(phandle->play_status_img, "34/34_play.png");
	}

	for (i = 0; i < DIVOOM_LVGL_TF_MUSIC_RIGHT_VISIBLE_ROWS; i++)
	{
		if(phandle->pcur_play_music == NULL)
		{
			break;	
		}

		if (phandle->right_music_btn[i] == NULL)
		{
			continue;
		}

		item_cont = phandle->right_music_btn[i];

		if (i < phandle->page_n && phandle->page[i].id == phandle->pcur_play_music->id)
		{
#if DIVOOM_TF_MUSIC_FAVORITES_UI
			if (phandle->pcur_play_music->like)
			{
				divoom_menu_control_lv_img_set_src(phandle->right_music_like[i], "92/92_like.png");
			}
			else
			{
				divoom_menu_control_lv_img_set_src(phandle->right_music_like[i], "92/92_unlike.png");
			}
#endif
			break;
		}
	}
}

void divoom_lvgl_tf_music_folder_menu_create_play_obj(Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle)
{
	lv_obj_t * line = lv_line_create(phandle->main_obj);  // 在当前屏幕上创建
	
	static lv_point_t line_points[] = {
		{20, 1137},	// 起点
		{780, 1137}, // 终点
	};
	
	lv_line_set_points(line, line_points, 2);
	lv_obj_set_style_line_color(line, lv_color_hex(0xC2C2C2), 0);  
	lv_obj_set_style_line_width(line, 2, 0);

	//当前播放歌曲
	lv_obj_t *play_obj = lv_obj_create(phandle->main_obj);

    lv_obj_set_pos(play_obj, 0, 1135);
	lv_obj_set_size(play_obj, DIVOOM_SCREEN_WIDTH, 145);
    lv_obj_set_style_bg_color(play_obj, lv_color_hex(0x0), 0);
	lv_obj_set_style_bg_opa(play_obj, 0, 0);
	lv_obj_set_style_pad_all(play_obj, 0, 0);
	lv_obj_set_style_border_width(play_obj, 0, 0);
	lv_obj_set_style_outline_width(play_obj, 0, 0);
	lv_obj_set_style_shadow_width(play_obj, 0, 0);	
    lv_obj_set_scrollbar_mode(play_obj, LV_SCROLLBAR_MODE_OFF);
	
	lv_obj_t *play_status_obj = lv_img_create(play_obj);

	if(divoom_tf_music_get_music_status())
	{
		divoom_menu_control_lv_img_set_src(play_status_obj, "34/34_pause.png");
	}
	else
	{
		divoom_menu_control_lv_img_set_src(play_status_obj, "34/34_play.png");
	}
	lv_obj_set_pos(play_status_obj, 43, 34);
	phandle->play_status_img = play_status_obj;

	lv_obj_t *label_obj = lv_label_create(play_obj);
    lv_obj_set_pos(label_obj, 120, 21+20);
	lv_obj_set_size(label_obj, 520, 80);
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
	lv_label_set_long_mode(label_obj, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, phandle->menu_id), 0); 

	if(phandle->pcur_play_music != NULL && phandle->pcur_play_music->music_name != NULL) 
	{
		lv_label_set_text_fmt(label_obj, "%s", divoom_lvgl_tf_music_folder_menu_get_music_name(phandle->pcur_play_music->music_name));
   	}
	else
	{
		lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("UnknownMusicName"));
	}
	phandle->play_bar_label = label_obj;

#if DIVOOM_TF_MUSIC_FAVORITES_UI
	lv_obj_t *like_obj = lv_img_create(play_obj);
	if(phandle->pcur_play_music != NULL && phandle->pcur_play_music->like)
	{
		divoom_menu_control_lv_img_set_src(like_obj, "34/34_like.png");
	}
	else
	{
		divoom_menu_control_lv_img_set_src(like_obj, "34/34_unlike.png");
	}
	lv_obj_set_pos(like_obj, 650, 31);
	phandle->like_img = like_obj;
#else
	phandle->like_img = NULL;
#endif

	lv_obj_t *play_order_obj = lv_img_create(play_obj);
	if(phandle->psys_config->tf_music_mode == 0)
	{
		divoom_menu_control_lv_img_set_src(play_order_obj, "34/34_sequential.png");
	}
	else if(phandle->psys_config->tf_music_mode == 1)
	{
		divoom_menu_control_lv_img_set_src(play_order_obj, "34/34_single_repeat.png");
	}
	else if(phandle->psys_config->tf_music_mode == 2)
	{
		divoom_menu_control_lv_img_set_src(play_order_obj, "34/34_random.png");
	}
	lv_obj_set_pos(play_order_obj, 720, 31);
	phandle->play_order_img = play_order_obj;
}

static void divoom_lvgl_tf_music_menu_right_panel_event_cb(lv_event_t * e)
{
	int i;
	int slot;
	int g;
	int last_g;
	lv_obj_t *cur_obj;
	Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle = (Pdivoom_lvgl_tf_music_folder_menu_ctrl)lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);
	cur_obj = lv_event_get_target_obj(e);

	if (code != LV_EVENT_KEY || phandle == NULL)
		return;

	char c = *((char *)lv_event_get_param(e));

	for (slot = -1, i = 0; i < DIVOOM_LVGL_TF_MUSIC_RIGHT_VISIBLE_ROWS; i++) {
		if (phandle->right_music_btn[i] == cur_obj) {
			slot = i;
			break;
		}
	}
	if (slot >= 0 && slot < DIVOOM_LVGL_TF_MUSIC_RIGHT_VISIBLE_ROWS)
		phandle->right_focus_slot = slot;

	if (c == DIVOOM_LVGL_KEY_ENTER && phandle->page_n > 0 && phandle->music_count > 0)
	{
		g = divoom_lvgl_tf_music_folder_menu_get_global_index(phandle);
		if (g >= 0) {
			int slot = g - phandle->right_first_index;
			int track_id;

			if (slot >= 0 && slot < phandle->page_n && !phandle->page[slot].is_dir)
			{
				track_id = phandle->page[slot].id;
				if (track_id > 0)
				{
					int order = divoom_tf_music_folder_left_to_play_order(phandle->left_selected_index);
					divoom_tf_music_set_playing_music_id(order, track_id);
					divoom_lvgl_tf_music_folder_menu_update_play_obj(phandle);
					divoom_lvgl_tf_music_folder_menu_sync_list_to_playing(phandle);
				}
			}
		}
		return;
	}

	if (c == DIVOOM_LVGL_KEY_ESC)
	{
		if (phandle->left_selected_index >= 0 && phandle->left_selected_index < phandle->left_item_count && phandle->left_folder_btn[phandle->left_selected_index])
		{
			lv_obj_set_style_bg_color(phandle->left_folder_btn[phandle->left_selected_index], lv_color_hex(0xD5D5D5), 0);
			lv_obj_set_style_bg_opa(phandle->left_folder_btn[phandle->left_selected_index], 0, 0);
			lv_group_focus_obj(phandle->left_folder_btn[phandle->left_selected_index]);
		}
		return;
	}

	if (c == DIVOOM_LVGL_KEY_PRE)
	{
		if (phandle->music_count <= 0)
			return;

		if (phandle->right_focus_slot > 0) {
			phandle->right_focus_slot--;
			if (phandle->right_music_btn[phandle->right_focus_slot])
				lv_group_focus_obj(phandle->right_music_btn[phandle->right_focus_slot]);
			return;
		}

		if (phandle->right_first_index > 0) {
			phandle->right_first_index--;
			divoom_lvgl_tf_music_folder_menu_refresh_right_slots(phandle);
			return;
		}

		/* 已到列表顶再按 PRE：环到末尾一屏 */
		if (phandle->music_count > DIVOOM_LVGL_TF_MUSIC_RIGHT_VISIBLE_ROWS) {
			phandle->right_first_index = phandle->music_count - DIVOOM_LVGL_TF_MUSIC_RIGHT_VISIBLE_ROWS;
			phandle->right_focus_slot = DIVOOM_LVGL_TF_MUSIC_RIGHT_VISIBLE_ROWS - 1;
		} else {
			phandle->right_first_index = 0;
			phandle->right_focus_slot = phandle->music_count - 1;
		}
		divoom_lvgl_tf_music_folder_menu_refresh_right_slots(phandle);
		return;
	}

	if (c == DIVOOM_LVGL_KEY_NEXT)
	{
		if (phandle->music_count <= 0)
			return;

		g = divoom_lvgl_tf_music_folder_menu_get_global_index(phandle);
		last_g = phandle->music_count - 1;

		if (g >= 0 && g < last_g) {
			if (phandle->right_focus_slot < DIVOOM_LVGL_TF_MUSIC_RIGHT_VISIBLE_ROWS - 1
				&& phandle->right_first_index + phandle->right_focus_slot + 1 < phandle->music_count) {
				phandle->right_focus_slot++;
				if (phandle->right_music_btn[phandle->right_focus_slot])
					lv_group_focus_obj(phandle->right_music_btn[phandle->right_focus_slot]);
			} else {
				phandle->right_first_index++;
				divoom_lvgl_tf_music_folder_menu_refresh_right_slots(phandle);
			}
			return;
		}

		/* 已在最后一首再按 NEXT：回到顶部 */
		phandle->right_first_index = 0;
		phandle->right_focus_slot = 0;
		divoom_lvgl_tf_music_folder_menu_refresh_right_slots(phandle);
		return;
	}

	{
		char *key_info = (char *)lv_event_get_param(e);
		divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
	}
}


// 创建音乐列表项
static void divoom_lvgl_tf_music_folder_menu_create_music_item(Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle, lv_obj_t *parent, const char *name, int index, int is_playing)
{
    lv_obj_t *item_cont = lv_btn_create(parent);
    lv_obj_set_size(item_cont, 489+6, 72+6);
    /*lv_obj_set_style_bg_opa(item_cont, LV_OPA_100, 0);
    lv_obj_set_style_border_width(item_cont, 3, 0);
    lv_obj_set_style_border_color(item_cont, lv_color_hex(0x6D6D6D), 0);
	lv_obj_set_style_border_color(item_cont, lv_color_hex(0xE4E4E4), LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_radius(item_cont, 24, 0);
    lv_obj_set_style_pad_all(item_cont, 0, 0);
	*/
	lv_obj_add_style(item_cont, &phandle->normal_type, 0);
	lv_obj_add_style(item_cont, &phandle->press_type, LV_STATE_FOCUSED);
	lv_obj_add_style(item_cont, &phandle->press_type, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(item_cont, 0, 0);
    lv_obj_clear_flag(item_cont, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_event_cb(item_cont, divoom_lvgl_tf_music_menu_right_panel_event_cb, LV_EVENT_KEY, phandle);

    // music名称
    lv_obj_t *name_label = lv_label_create(item_cont);
    if (name && name[0] != '\0')
    {
        lv_label_set_text(name_label, divoom_lvgl_tf_music_folder_menu_get_music_name((char *)name));
    }
    else
    {
        lv_label_set_text(name_label, "Unknown Music");
    }
    lv_obj_set_style_text_font(name_label, divoom_menu_control_get_font_info(20, phandle->menu_id), 0);
    lv_obj_set_style_text_color(name_label, lv_color_hex(0x0), 0);
    lv_obj_set_pos(name_label, 53, 10+14);
#if DIVOOM_TF_MUSIC_FAVORITES_UI
    lv_obj_set_width(name_label, 497-120);
#else
    lv_obj_set_width(name_label, 497-60);
#endif
	lv_obj_set_height(name_label, 52-32);
    lv_label_set_long_mode(name_label, LV_LABEL_LONG_DOT);
	
	lv_obj_t *playing_icon = lv_img_create(item_cont);
	lv_obj_set_pos(playing_icon, 15, 20);
 	
	if(is_playing)
	{
		divoom_menu_control_lv_img_set_src(playing_icon, "92/92_playing.png");
	
		lv_obj_set_style_bg_color(item_cont, lv_color_hex(0xC1C1C1), 0);
	}
	else if (is_playing == 0)
	{
		divoom_menu_control_lv_img_set_src(playing_icon, "92/92_music.png");
	
		lv_obj_set_style_bg_color(item_cont, lv_color_hex(0xE4E4E4), 0);
	}

#if DIVOOM_TF_MUSIC_FAVORITES_UI
    lv_obj_t *heart_icon = lv_img_create(item_cont);
    {
		divoom_menu_control_lv_img_set_src(heart_icon, "92/92_unlike.png");
    }
	lv_obj_set_pos(heart_icon, 442, 20);
	phandle->right_music_like[index] = heart_icon;
#else
	phandle->right_music_like[index] = NULL;
#endif

	phandle->right_music_btn[index] = item_cont; 
	phandle->right_music_img[index] =playing_icon;
	phandle->right_music_label[index] = name_label;


}

// 更新音乐列表（根据 left_selected_index：0=全部 1=收藏 2+=根目录）
static void divoom_lvgl_tf_music_folder_menu_update_music_list(Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle)
{
    int i;
    int is_playing;

    if (phandle == NULL)
        return;

    if (phandle->right_panel)
    {
	    lv_obj_clean(phandle->right_panel);
	}

	for (i = 0; i < DIVOOM_LVGL_TF_MUSIC_RIGHT_VISIBLE_ROWS; i++)
	{
		phandle->right_music_btn[i] = NULL;
		phandle->right_music_img[i] = NULL;
		phandle->right_music_label[i] = NULL;
		phandle->right_music_like[i] = NULL;
	}

    memset(phandle->page, 0, sizeof(phandle->page));
    phandle->page_n = 0;
    phandle->music_count = 0;
    phandle->folder_id = divoom_lvgl_tf_music_folder_id_from_left(phandle);
    if (phandle->folder_id >= 0 && !divoom_database_tf_is_rebuilding())
        phandle->music_count = divoom_database_tf_subtree_count(phandle->folder_id);

    if (phandle->music_count <= 0)
    {
	    return;
	}

	phandle->right_first_index = 0;
	phandle->right_focus_slot = 0;

    for (i = 0; i < DIVOOM_LVGL_TF_MUSIC_RIGHT_VISIBLE_ROWS; i++)
	{
        is_playing = 0;
        divoom_lvgl_tf_music_folder_menu_create_music_item(phandle, phandle->right_panel, NULL, i, is_playing);
    }

	divoom_lvgl_tf_music_folder_menu_refresh_right_slots(phandle);
	divoom_lvgl_tf_music_folder_menu_sync_list_to_playing(phandle);
}

// 创建右侧FM列表面板
static void divoom_lvgl_tf_music_folder_menu_create_right_panel(Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle)
{
    int left_idx;

    left_idx = divoom_tf_music_folder_play_order_to_left(divoom_tf_music_get_play_order());
    if (left_idx < 0 || left_idx >= phandle->left_item_count)
        left_idx = 0;
    phandle->left_selected_index = left_idx;
    divoom_lvgl_tf_music_folder_menu_apply_left_selection(phandle, left_idx);

    phandle->right_panel = lv_obj_create(phandle->main_obj);
    lv_obj_set_size(phandle->right_panel, 507, 1055);
    lv_obj_set_pos(phandle->right_panel, 293, 80);
    lv_obj_set_style_bg_opa(phandle->right_panel, LV_OPA_100, 0);
	lv_obj_set_style_bg_color(phandle->right_panel, lv_color_hex(0xE4E4E4), 0);
    lv_obj_set_style_border_width(phandle->right_panel, 0, 0);
	lv_obj_set_style_radius(phandle->right_panel, 0, 0);
    lv_obj_set_style_pad_all(phandle->right_panel, 0, 0);
	lv_obj_set_style_pad_bottom(phandle->right_panel, 20, 0);
	lv_obj_set_style_pad_top(phandle->right_panel, 20, 0);
	lv_obj_set_scroll_dir(phandle->right_panel, LV_DIR_VER);
	lv_obj_set_scrollbar_mode(phandle->right_panel, LV_SCROLLBAR_MODE_OFF);
	lv_obj_set_flex_flow(phandle->right_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(phandle->right_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // 更新音乐列表
    divoom_lvgl_tf_music_folder_menu_update_music_list(phandle);
}


static void divoom_lvgl_tf_music_menu_left_panel_event_cb(lv_event_t * e)
{
	int i, index = -1;
	lv_obj_t *cur_obj;
	Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle = (Pdivoom_lvgl_tf_music_folder_menu_ctrl)lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);
	cur_obj = lv_event_get_target_obj(e);

	if (phandle == NULL)
	{
		return;
	}

	if (code != LV_EVENT_KEY)
	{
		return;
	}

	char c = *((char *)lv_event_get_param(e));

	for (i = 0; i < phandle->left_item_count; i++) 
	{
		if (phandle->left_folder_btn[i] != NULL && cur_obj == phandle->left_folder_btn[i]) 
		{
			index = i;
			break;
		}
	}

	if (index < 0)
	{
		return;
	}

	if (c == DIVOOM_LVGL_KEY_NEXT) 
	{
		index = (index + 1) % phandle->left_item_count;
		lv_group_focus_obj(phandle->left_folder_btn[index]);
	} 
	else if (c == DIVOOM_LVGL_KEY_PRE) 
	{
		index = (index - 1 + phandle->left_item_count) % phandle->left_item_count;
		lv_group_focus_obj(phandle->left_folder_btn[index]);
	} 
	else if (c == DIVOOM_LVGL_KEY_ENTER)
	{
		divoom_lvgl_tf_music_folder_menu_apply_left_selection(phandle, index);
		divoom_lvgl_tf_music_folder_menu_update_music_list(phandle);

		if (phandle->music_count > 0 && phandle->right_music_btn[0])
		{
			lv_group_focus_obj(phandle->right_music_btn[0]);
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



// 创建左侧分类面板
static void divoom_lvgl_tf_music_folder_menu_create_left_panel(Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle)
{	
	int i, index = 0;

    // 创建左侧面板容器
    lv_obj_t *left_panel = lv_obj_create(phandle->main_obj);
    lv_obj_set_size(left_panel, 273, 1035);
    lv_obj_set_pos(left_panel, 10, 90);
    lv_obj_set_style_bg_opa(left_panel, LV_OPA_100, 0);
	lv_obj_set_style_bg_color(left_panel, lv_color_hex(0xD5D5D5), 0);
    lv_obj_set_style_border_width(left_panel, 0, 0);
    lv_obj_set_style_pad_all(left_panel, 0, 0);
	lv_obj_set_style_radius(left_panel, 32, 0);
	//lv_obj_clear_flag(left_panel, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *all_btn = lv_btn_create(left_panel);
    lv_obj_set_size(all_btn, 245, 72);
	lv_obj_set_pos(all_btn, 14, 14);
    lv_obj_set_style_pad_all(all_btn, 0, 0);
    lv_obj_set_style_border_width(all_btn, 3, 0);
	lv_obj_set_style_border_color(all_btn, lv_color_hex(0xD5D5D5), 0);
	lv_obj_set_style_border_opa(all_btn, LV_OPA_100, 0);
	lv_obj_set_style_border_color(all_btn, lv_color_hex(0x6D6D6D), LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_border_opa(all_btn, LV_OPA_100, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_radius(all_btn, 22, 0);
    lv_obj_set_style_shadow_width(all_btn, 0, 0);
	lv_obj_set_style_bg_opa(all_btn, 0, 0);
	lv_obj_set_style_outline_width(all_btn, 0, 0);
	lv_obj_set_style_outline_width(all_btn, 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_scrollbar_mode(all_btn, LV_SCROLLBAR_MODE_OFF);
	lv_obj_add_event_cb(all_btn, divoom_lvgl_tf_music_menu_left_panel_event_cb, LV_EVENT_KEY, phandle);
	phandle->left_folder_btn[index++] = all_btn;
	
	lv_obj_t *folder_img = lv_img_create(all_btn);
	divoom_menu_control_lv_img_set_src(folder_img, "92/92_folder.png");
	lv_obj_set_pos(folder_img, 10, 20);

	lv_obj_t *all_label = lv_label_create(all_btn);
			
	lv_obj_set_style_text_font(all_label, divoom_menu_control_get_font_info(20, phandle->menu_id), 0);
	lv_obj_set_style_text_color(all_label, lv_color_hex(0x0), 0);
	lv_obj_set_scrollbar_mode(all_label, LV_SCROLLBAR_MODE_OFF);
	lv_obj_set_size(all_label, 183, 52);	
	lv_obj_set_pos(all_label, 52, 10+12);
	lv_label_set_long_mode(all_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
	lv_label_set_text(all_label, divoom_menu_control_get_disp_str("Allpath"));

#if DIVOOM_TF_MUSIC_FAVORITES_UI
	lv_obj_t *favorites_btn = lv_btn_create(left_panel);
    lv_obj_set_size(favorites_btn, 245, 72);
	lv_obj_set_pos(favorites_btn, 14, 14+77*1);
    lv_obj_set_style_pad_all(favorites_btn, 0, 0);
    lv_obj_set_style_border_width(favorites_btn, 3, 0);
	lv_obj_set_style_border_color(favorites_btn, lv_color_hex(0xD5D5D5), 0);
	lv_obj_set_style_border_opa(favorites_btn, LV_OPA_100, 0);
	lv_obj_set_style_border_color(favorites_btn, lv_color_hex(0x6D6D6D), LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_border_opa(favorites_btn, LV_OPA_100, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_radius(favorites_btn, 22, 0);
    lv_obj_set_style_shadow_width(favorites_btn, 0, 0);
	lv_obj_set_style_bg_opa(favorites_btn, 0, 0);
	lv_obj_set_style_outline_width(favorites_btn, 0, 0);
	lv_obj_set_style_outline_width(favorites_btn, 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_scrollbar_mode(favorites_btn, LV_SCROLLBAR_MODE_OFF);
	lv_obj_add_event_cb(favorites_btn, divoom_lvgl_tf_music_menu_left_panel_event_cb, LV_EVENT_KEY, phandle);
	phandle->left_folder_btn[index++] = favorites_btn;
	
	lv_obj_t *folder_img2 = lv_img_create(favorites_btn);
	divoom_menu_control_lv_img_set_src(folder_img2, "92/92_folder.png");
	lv_obj_set_pos(folder_img2, 10, 20);

	lv_obj_t *favorites_label2 = lv_label_create(favorites_btn);
			
	lv_obj_set_style_text_font(favorites_label2, divoom_menu_control_get_font_info(20, phandle->menu_id), 0);
	lv_obj_set_style_text_color(favorites_label2, lv_color_hex(0x0), 0);
	lv_obj_set_scrollbar_mode(favorites_label2, LV_SCROLLBAR_MODE_OFF);
	lv_obj_set_size(favorites_label2, 183, 52);	
	lv_obj_set_pos(favorites_label2, 52, 10+12);
	lv_label_set_long_mode(favorites_label2, LV_LABEL_LONG_SCROLL_CIRCULAR);
	lv_label_set_text(favorites_label2, divoom_menu_control_get_disp_str("Favorites"));
#endif

    phandle->hub_count = divoom_lvgl_tf_music_folder_load_hub(phandle->hub, DIVOOM_TF_MUSIC_MAX_ROOT);
    phandle->left_item_count = DIVOOM_TF_MUSIC_FOLDER_LEFT_BASE + phandle->hub_count;

    for (i = 0; i < phandle->hub_count; i++) 
	{
        lv_obj_t *primary_btn = lv_btn_create(left_panel);
        lv_obj_set_size(primary_btn, 245, 72);
        lv_obj_set_pos(primary_btn, 14, 14 + 77 * (DIVOOM_TF_MUSIC_FOLDER_LEFT_BASE + i));
        lv_obj_set_style_pad_all(primary_btn, 0, 0);
        lv_obj_set_style_border_width(primary_btn, 3, 0);
        lv_obj_set_style_border_color(primary_btn, lv_color_hex(0xD5D5D5), 0);
        lv_obj_set_style_border_opa(primary_btn, LV_OPA_100, 0);
        lv_obj_set_style_border_color(primary_btn, lv_color_hex(0x6D6D6D), LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_border_opa(primary_btn, LV_OPA_100, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_radius(primary_btn, 22, 0);
        lv_obj_set_style_shadow_width(primary_btn, 0, 0);
        lv_obj_set_style_bg_opa(primary_btn, 0, 0);
        lv_obj_set_style_outline_width(primary_btn, 0, 0);
        lv_obj_set_style_outline_width(primary_btn, 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
        lv_obj_set_scrollbar_mode(primary_btn, LV_SCROLLBAR_MODE_OFF);
        lv_obj_add_event_cb(primary_btn, divoom_lvgl_tf_music_menu_left_panel_event_cb, LV_EVENT_KEY, phandle);
        phandle->left_folder_btn[index++] = primary_btn;

        lv_obj_t *folder_icon = lv_img_create(primary_btn);
        divoom_menu_control_lv_img_set_src(folder_icon, "92/92_folder.png");
        lv_obj_set_pos(folder_icon, 10, 20);

        lv_obj_t *primary_label = lv_label_create(primary_btn);
        lv_label_set_text(primary_label, phandle->hub[i].name[0] ? phandle->hub[i].name : "");
        lv_obj_set_style_text_font(primary_label, divoom_menu_control_get_font_info(20, phandle->menu_id), 0);
        lv_obj_set_style_text_color(primary_label, lv_color_hex(0x0), 0);
        lv_obj_set_scrollbar_mode(primary_label, LV_SCROLLBAR_MODE_OFF);
		lv_label_set_long_mode(primary_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_size(primary_label, 183, 52);
        lv_obj_set_pos(primary_label, 52, 10 + 12);
    }
}


static void divoom_lvgl_tf_music_folder_menu_create_top_buttons(Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle)
{
	int hour;
	time_t time_now;
	struct tm p_now;
	
    time_now = time(NULL);
	localtime_r(&time_now, &p_now);
	hour = p_now.tm_hour;
    if(divoom_main_get_time_flag() == 0)
    {
        if(hour != 12)
        {
            hour = hour % 12;
            if(hour == 0)
            {
                hour = 12;
            }
        }
    }
	
	// 添加标签
    lv_obj_t *time_label = lv_label_create(phandle->main_obj);
    lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
	lv_obj_set_size(time_label, 400, 60);
	lv_obj_set_pos(time_label, 40, 10+20);
	lv_label_set_text_fmt(time_label, "%02d:%02d", hour, p_now.tm_min);
	phandle->time_label = time_label;
	phandle->menu_bar_timer = lv_timer_create(divoom_lvgl_tf_music_folder_menu_refresh_menu_bar, 1000, phandle);
	
    // 添加标签
    lv_obj_t *label = lv_label_create(phandle->main_obj);
    lv_label_set_text(label, divoom_menu_control_get_disp_str("LocalMusic"));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_obj_set_size(label, 400, 60);
	lv_obj_set_pos(label, 200, 10+15);

	lv_obj_t *image_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(image_obj, 696, 20);
	lv_obj_set_size(image_obj, 64, 40);
	divoom_lvgl_tf_music_folder_set_battery_level_img(image_obj);
	phandle->top_battery_img = image_obj;
}


void *divoom_lvgl_tf_music_folder_menu_init(void *user_data, int menu_id)
{
	Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle = divoom_system_malloc(sizeof(divoom_lvgl_tf_music_folder_menu_ctrl));

	if(phandle == NULL)
	{
		return NULL;
	}
    phandle->menu_id = menu_id;
	memset(phandle, 0, sizeof(divoom_lvgl_tf_music_folder_menu_ctrl));
	phandle->psys_config = divoom_main_get_system_config();

	divoom_tf_music_sync_prepare();
	divoom_tf_music_restore_saved_playback();
	phandle->pcur_play_music = divoom_tf_music_get_playing_music_info();
	phandle->last_play_music_id = divoom_tf_music_get_playing_music_id();

    phandle->main_obj = lv_obj_create(NULL);
    divoom_menu_control_init_lable_style(&phandle->cout_style);
    lv_obj_add_style(phandle->main_obj, &phandle->cout_style, 0);
    lv_obj_set_pos(phandle->main_obj, 0, 0);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(0xE4E4E4), 0);
    lv_obj_set_scrollbar_mode(phandle->main_obj, LV_SCROLLBAR_MODE_OFF);
	
	lv_style_init(&phandle->normal_type);
	lv_style_set_bg_color(&phandle->normal_type, lv_color_hex(0xE4E4E4));
	lv_style_set_pad_all(&phandle->normal_type, 0);
	lv_style_set_border_width(&phandle->normal_type, 4);
	lv_style_set_border_color(&phandle->normal_type, lv_color_hex(0xE4E4E4));
	lv_style_set_radius(&phandle->normal_type, 17);
	lv_style_set_shadow_width(&phandle->normal_type, 0);
	lv_style_set_outline_width(&phandle->normal_type, 0);

	lv_style_init(&phandle->press_type);
	lv_style_set_bg_color(&phandle->press_type, lv_color_hex(0xE4E4E4));
	lv_style_set_pad_all(&phandle->press_type, 0);
	lv_style_set_border_width(&phandle->press_type, 4);
	lv_style_set_border_color(&phandle->press_type, lv_color_hex(0x6D6D6D));
	lv_style_set_radius(&phandle->press_type, 17);
	lv_style_set_shadow_width(&phandle->press_type, 0);
	lv_style_set_outline_width(&phandle->press_type, 0);

	divoom_lvgl_tf_music_folder_menu_create_top_buttons(phandle);
	divoom_lvgl_tf_music_folder_menu_create_left_panel(phandle);
	divoom_lvgl_tf_music_folder_menu_create_right_panel(phandle);
	divoom_lvgl_tf_music_folder_menu_create_play_obj(phandle);
	phandle->play_refresh_timer = lv_timer_create(divoom_lvgl_tf_music_folder_menu_play_refresh_cb, 500, phandle);

	return phandle;
}

void divoom_lvgl_tf_music_folder_menu_disp(void *handle, void *active_obj)
{
	Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle = (Pdivoom_lvgl_tf_music_folder_menu_ctrl)handle;

	if (phandle->play_refresh_timer)
		lv_timer_resume(phandle->play_refresh_timer);
	if (phandle->menu_bar_timer)
		lv_timer_resume(phandle->menu_bar_timer);
	divoom_tf_music_sync_play_status_ui();
	divoom_lvgl_tf_music_folder_menu_sync_to_playback(phandle);
	if (active_obj)
		lv_group_focus_obj(active_obj);
	lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_tf_music_folder_menu_pause(void *handle)
{
	Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle = (Pdivoom_lvgl_tf_music_folder_menu_ctrl)handle;

	if (phandle->menu_bar_timer)
		lv_timer_pause(phandle->menu_bar_timer);
	if (phandle->play_refresh_timer)
		lv_timer_pause(phandle->play_refresh_timer);
}

void divoom_lvgl_tf_music_folder_menu_exit(void *handle)
{
	Pdivoom_lvgl_tf_music_folder_menu_ctrl phandle = (Pdivoom_lvgl_tf_music_folder_menu_ctrl)handle;

	lv_style_reset(&phandle->cout_style);
	lv_style_reset(&phandle->normal_type);
	lv_style_reset(&phandle->press_type);

	memset(phandle->page, 0, sizeof(phandle->page));
	phandle->page_n = 0;
	phandle->music_count = 0;

	if (phandle->play_refresh_timer) {
		lv_timer_del(phandle->play_refresh_timer);
		phandle->play_refresh_timer = NULL;
	}
	lv_timer_del(phandle->menu_bar_timer);
	lv_obj_del(phandle->main_obj);
	divoom_system_free(phandle);
}



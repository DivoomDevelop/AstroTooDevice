#include "divoom_alarm_tf_sound_pick.h"

#ifdef DIVOOM_TIME_FRAME_SPEAKER

#include "divoom_menu_control.h"
#include "divoom_tf_music.h"
#include "lvgl.h"
#include <string.h>

#define DIVOOM_ALARM_TF_SOUND_ITEM_COUNT      4
#define DIVOOM_ALARM_TF_SOUND_VISIBLE_ROWS      30
#define DIVOOM_ALARM_TF_SOUND_PANEL_H           1180

struct divoom_alarm_tf_sound_pick_ctrl
{
	divoom_alarm_tf_sound_pick_cfg cfg;
	void *sound_select_obj;
	void *sound_select_btn[4];
	void *pdirect_sound_select_handle;
	void *tf_pick_obj;
	void *tf_left_panel;
	void *tf_right_panel;
	lv_obj_t *tf_left_folder_btn[DIVOOM_TF_MUSIC_MAX_ROOT + 2];
	void *tf_right_music_btn[30];
	void *tf_right_music_label[30];
	int tf_left_selected_index;
	int tf_left_item_count;
	divoom_tf_entry tf_hub[DIVOOM_TF_MUSIC_MAX_ROOT];
	int tf_hub_count;
	int tf_folder_id;
	divoom_tf_entry tf_page[DIVOOM_ALARM_TF_SOUND_VISIBLE_ROWS];
	int tf_page_n;
	int tf_music_count;
	int tf_right_first_index;
	int tf_right_focus_slot;
	lv_style_t tf_normal_type;
	lv_style_t tf_press_type;
	uint8 tf_styles_inited;
};

static void divoom_alarm_tf_sound_rebuild_left(divoom_alarm_tf_sound_pick_ctrl *ctrl);

int divoom_alarm_tf_sound_card_inserted(void)
{
	return divoom_main_get_audio_type_statu(2) ? 1 : 0;
}

static int divoom_alarm_tf_sound_card_available(void)
{
	return divoom_alarm_tf_sound_card_inserted();
}

static void divoom_alarm_tf_sound_paint_left_selection(divoom_alarm_tf_sound_pick_ctrl *ctrl, int index);
static void divoom_alarm_tf_sound_update_music_list(divoom_alarm_tf_sound_pick_ctrl *ctrl);

static int divoom_alarm_tf_sound_enter_tf_pick(divoom_alarm_tf_sound_pick_ctrl *ctrl)
{
	lv_obj_t *focus_left;

	if(ctrl == NULL || ctrl->tf_pick_obj == NULL || !divoom_alarm_tf_sound_card_available())
	{
		return -1;
	}

	divoom_tf_music_send_get_music_count();
	ctrl->tf_left_selected_index = 0;
	divoom_alarm_tf_sound_rebuild_left(ctrl);
	divoom_alarm_tf_sound_update_music_list(ctrl);
	lv_obj_add_flag(ctrl->sound_select_obj, LV_OBJ_FLAG_HIDDEN);
	lv_obj_clear_flag(ctrl->tf_pick_obj, LV_OBJ_FLAG_HIDDEN);
	lv_obj_move_foreground(ctrl->tf_pick_obj);
	focus_left = ctrl->tf_left_folder_btn[0];
	if(focus_left)
	{
		lv_group_focus_obj(focus_left);
		lv_obj_scroll_to_view(focus_left, LV_ANIM_OFF);
	}
	return 0;
}

static int divoom_alarm_tf_sound_load_hub(divoom_tf_entry *out, int cap)
{
	int n;
	int i;
	int keep;

	if(out == NULL || cap <= 0)
	{
		return 0;
	}
	if(divoom_database_tf_is_rebuilding())
	{
		return 0;
	}
	n = divoom_database_tf_dir_page(0, 0, cap, out, cap);
	keep = 0;
	for(i = 0; i < n; i++)
	{
		if(!out[i].is_dir || out[i].id <= 0)
		{
			continue;
		}
		if(strcasecmp(out[i].name, "tf") == 0)
		{
			continue;
		}
		if(keep != i)
		{
			out[keep] = out[i];
		}
		keep++;
	}
	return keep;
}

static int divoom_alarm_tf_sound_hub_folder_id(divoom_alarm_tf_sound_pick_ctrl *ctrl)
{
	if(ctrl == NULL)
	{
		return 0;
	}
	if(ctrl->tf_hub_count <= 0)
	{
		return 0;
	}
	if(ctrl->tf_left_selected_index < 0 || ctrl->tf_left_selected_index >= ctrl->tf_hub_count)
	{
		return 0;
	}
	return ctrl->tf_hub[ctrl->tf_left_selected_index].id;
}

static void divoom_alarm_tf_sound_free_music_list(divoom_alarm_tf_sound_pick_ctrl *ctrl)
{
	if(ctrl == NULL)
	{
		return;
	}
	memset(ctrl->tf_page, 0, sizeof(ctrl->tf_page));
	ctrl->tf_page_n = 0;
	ctrl->tf_music_count = 0;
	ctrl->tf_folder_id = 0;
}

static int divoom_alarm_tf_sound_right_visible_in_window(divoom_alarm_tf_sound_pick_ctrl *ctrl)
{
	int left;

	if(ctrl == NULL || ctrl->tf_music_count <= 0)
	{
		return 0;
	}
	left = ctrl->tf_music_count - ctrl->tf_right_first_index;
	if(left > DIVOOM_ALARM_TF_SOUND_VISIBLE_ROWS)
	{
		return DIVOOM_ALARM_TF_SOUND_VISIBLE_ROWS;
	}
	return left;
}

static int divoom_alarm_tf_sound_get_global_index(divoom_alarm_tf_sound_pick_ctrl *ctrl)
{
	int g;

	if(ctrl == NULL)
	{
		return -1;
	}
	g = ctrl->tf_right_first_index + ctrl->tf_right_focus_slot;
	if(g < 0 || g >= ctrl->tf_music_count)
	{
		return -1;
	}
	return g;
}

void divoom_alarm_tf_sound_pick_refresh_label(divoom_alarm_tf_sound_pick_ctrl *ctrl)
{
	char disp_str[64];
	char tf_label[64];
	char track_name[DIVOOM_TF_NAME_MAX];
	const char *name;
	int music_id;

	if(ctrl == NULL || ctrl->cfg.alarm_info == NULL || ctrl->cfg.music_label_obj == NULL)
	{
		return;
	}
	if(ctrl->cfg.alarm_info->mode == 1)
	{
		lv_label_set_text(ctrl->cfg.music_label_obj, divoom_menu_control_get_disp_str("SleepAidTabRecord"));
	}
	else if(ctrl->cfg.alarm_info->mode == DIVOOM_ALARM_TF_SOUND_MODE)
	{
		tf_label[0] = '\0';
		track_name[0] = '\0';
		music_id = divoom_tf_music_alarm_resolve_music_id(ctrl->cfg.alarm_info->leave_url);
		if(music_id > 0 && divoom_database_tf_track_name(music_id, track_name, (int)sizeof(track_name))
		   && track_name[0] != '\0')
		{
			name = divoom_tf_music_get_disp_name(track_name);
			strncpy(tf_label, name, sizeof(tf_label) - 1);
			tf_label[sizeof(tf_label) - 1] = '\0';
		}
		else if(ctrl->cfg.alarm_info->leave_url[0] != '\0')
		{
			name = divoom_tf_music_get_disp_name(ctrl->cfg.alarm_info->leave_url);
			strncpy(tf_label, name, sizeof(tf_label) - 1);
			tf_label[sizeof(tf_label) - 1] = '\0';
		}
		else
		{
			strncpy(tf_label, divoom_menu_control_get_disp_str("TFCard"), sizeof(tf_label) - 1);
			tf_label[sizeof(tf_label) - 1] = '\0';
		}
		lv_label_set_text(ctrl->cfg.music_label_obj, tf_label);
	}
	else
	{
		lv_label_set_text_fmt(ctrl->cfg.music_label_obj, "Music %d", ctrl->cfg.alarm_info->tip_index + 1);
	}

	if(ctrl->cfg.sound_music_label)
	{
		if(ctrl->cfg.alarm_info->mode == 1)
		{
			lv_label_set_text(ctrl->cfg.sound_music_label, divoom_menu_control_get_disp_str("SleepAidTabRecord"));
		}
		else if(ctrl->cfg.alarm_info->mode == DIVOOM_ALARM_TF_SOUND_MODE)
		{
			lv_label_set_text(ctrl->cfg.sound_music_label, lv_label_get_text(ctrl->cfg.music_label_obj));
		}
		else
		{
			lv_label_set_text_fmt(ctrl->cfg.sound_music_label, "Music %d", ctrl->cfg.alarm_info->tip_index + 1);
		}
	}
}

static void divoom_alarm_tf_sound_refresh_right_slots(divoom_alarm_tf_sound_pick_ctrl *ctrl);
static void divoom_alarm_tf_sound_update_music_list(divoom_alarm_tf_sound_pick_ctrl *ctrl);
static void divoom_alarm_tf_sound_paint_left_selection(divoom_alarm_tf_sound_pick_ctrl *ctrl, int index);
static void divoom_alarm_tf_sound_save_selection(divoom_alarm_tf_sound_pick_ctrl *ctrl, int global_index);
static void divoom_alarm_tf_sound_pick_left_event_cb(lv_event_t *e);
static void divoom_alarm_tf_sound_pick_right_event_cb(lv_event_t *e);
static void divoom_alarm_tf_sound_select_menu_event_cb(lv_event_t *e);
static lv_obj_t *divoom_alarm_tf_sound_create_folder_btn(divoom_alarm_tf_sound_pick_ctrl *ctrl,
                                                        lv_obj_t *parent, int y, const char *label_text);
static lv_obj_t *divoom_alarm_tf_sound_create_sound_list_btn(divoom_alarm_tf_sound_pick_ctrl *ctrl,
                                                            lv_obj_t *parent, int idx, int y,
                                                            const char *title_key, const char *desp_key,
                                                            uint8 disabled);
static void divoom_alarm_tf_sound_create_tf_pick_page(divoom_alarm_tf_sound_pick_ctrl *ctrl);
static void divoom_alarm_tf_sound_create_sound_select_page(divoom_alarm_tf_sound_pick_ctrl *ctrl);

static void divoom_alarm_tf_sound_refresh_right_slots(divoom_alarm_tf_sound_pick_ctrl *ctrl)
{
	int i;
	int max_slot;
	lv_obj_t *item_cont;

	if(ctrl == NULL)
	{
		return;
	}
	if(ctrl->tf_right_first_index < 0)
	{
		ctrl->tf_right_first_index = 0;
	}
	memset(ctrl->tf_page, 0, sizeof(ctrl->tf_page));
	ctrl->tf_page_n = 0;
	if(ctrl->tf_music_count > 0 && !divoom_database_tf_is_rebuilding())
	{
		ctrl->tf_page_n = divoom_database_tf_subtree_page(ctrl->tf_folder_id,
		                                                 ctrl->tf_right_first_index,
		                                                 DIVOOM_ALARM_TF_SOUND_VISIBLE_ROWS,
		                                                 ctrl->tf_page,
		                                                 DIVOOM_ALARM_TF_SOUND_VISIBLE_ROWS);
	}
	for(i = 0; i < DIVOOM_ALARM_TF_SOUND_VISIBLE_ROWS; i++)
	{
		int gi = ctrl->tf_right_first_index + i;

		item_cont = ctrl->tf_right_music_btn[i];
		if(item_cont == NULL)
		{
			continue;
		}
		if(i >= ctrl->tf_page_n || ctrl->tf_page[i].id <= 0 || ctrl->tf_page[i].is_dir
		   || gi < 0 || gi >= ctrl->tf_music_count)
		{
			lv_obj_add_flag(item_cont, LV_OBJ_FLAG_HIDDEN);
			continue;
		}
		lv_obj_clear_flag(item_cont, LV_OBJ_FLAG_HIDDEN);
		if(ctrl->tf_right_music_label[i] && ctrl->tf_page[i].name[0] != '\0')
		{
			lv_label_set_text(ctrl->tf_right_music_label[i],
			                  divoom_tf_music_get_disp_name(ctrl->tf_page[i].name));
		}
	}

	max_slot = divoom_alarm_tf_sound_right_visible_in_window(ctrl) - 1;
	if(max_slot < 0)
	{
		max_slot = 0;
	}
	if(ctrl->tf_right_focus_slot < 0)
	{
		ctrl->tf_right_focus_slot = 0;
	}
	if(ctrl->tf_right_focus_slot > max_slot)
	{
		ctrl->tf_right_focus_slot = max_slot;
	}
	if(ctrl->tf_pick_obj
	   && !lv_obj_has_flag(ctrl->tf_pick_obj, LV_OBJ_FLAG_HIDDEN)
	   && ctrl->tf_right_music_btn[ctrl->tf_right_focus_slot])
	{
		lv_group_focus_obj(ctrl->tf_right_music_btn[ctrl->tf_right_focus_slot]);
		lv_obj_scroll_to_view(ctrl->tf_right_music_btn[ctrl->tf_right_focus_slot], LV_ANIM_OFF);
	}
}

static void divoom_alarm_tf_sound_create_music_item(divoom_alarm_tf_sound_pick_ctrl *ctrl,
                                                   lv_obj_t *parent, const char *name, int index)
{
	lv_obj_t *item_cont;
	lv_obj_t *name_label;
	lv_obj_t *music_icon;

	item_cont = lv_btn_create(parent);
	lv_obj_set_size(item_cont, 489 + 6, 72 + 6);
	lv_obj_add_style(item_cont, &ctrl->tf_normal_type, 0);
	lv_obj_add_style(item_cont, &ctrl->tf_press_type, LV_STATE_FOCUSED);
	lv_obj_add_style(item_cont, &ctrl->tf_press_type, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(item_cont, 0, 0);
	lv_obj_clear_flag(item_cont, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_event_cb(item_cont, divoom_alarm_tf_sound_pick_right_event_cb, LV_EVENT_KEY, ctrl);

	music_icon = lv_img_create(item_cont);
	lv_obj_set_pos(music_icon, 15, 20);
	divoom_menu_control_lv_img_set_src(music_icon, "92/92_music.png");

	name_label = lv_label_create(item_cont);
	if(name && name[0] != '\0')
	{
		lv_label_set_text(name_label, divoom_tf_music_get_disp_name(name));
	}
	else
	{
		lv_label_set_text(name_label, divoom_menu_control_get_disp_str("UnknownMusicName"));
	}
	lv_obj_set_style_text_font(name_label, divoom_menu_control_get_font_info(20, ctrl->cfg.menu_id), 0);
	lv_obj_set_style_text_color(name_label, lv_color_hex(0x0), 0);
	lv_obj_set_pos(name_label, 53, 24);
	lv_obj_set_width(name_label, 497 - 120);
	lv_obj_set_height(name_label, 52 - 32);
	lv_label_set_long_mode(name_label, LV_LABEL_LONG_DOT);

	ctrl->tf_right_music_btn[index] = item_cont;
	ctrl->tf_right_music_label[index] = name_label;
}

static void divoom_alarm_tf_sound_update_music_list(divoom_alarm_tf_sound_pick_ctrl *ctrl)
{
	int i;

	if(ctrl == NULL || !divoom_alarm_tf_sound_card_available())
	{
		return;
	}
	if(ctrl->tf_right_panel)
	{
		lv_obj_clean(ctrl->tf_right_panel);
	}
	for(i = 0; i < DIVOOM_ALARM_TF_SOUND_VISIBLE_ROWS; i++)
	{
		ctrl->tf_right_music_btn[i] = NULL;
		ctrl->tf_right_music_label[i] = NULL;
	}
	divoom_alarm_tf_sound_free_music_list(ctrl);

	ctrl->tf_folder_id = divoom_alarm_tf_sound_hub_folder_id(ctrl);
	if(!divoom_database_tf_is_rebuilding())
	{
		ctrl->tf_music_count = divoom_database_tf_subtree_count(ctrl->tf_folder_id);
	}
	if(ctrl->tf_music_count <= 0)
	{
		return;
	}

	ctrl->tf_right_first_index = 0;
	ctrl->tf_right_focus_slot = 0;
	for(i = 0; i < DIVOOM_ALARM_TF_SOUND_VISIBLE_ROWS; i++)
	{
		divoom_alarm_tf_sound_create_music_item(ctrl, ctrl->tf_right_panel, NULL, i);
	}
	divoom_alarm_tf_sound_refresh_right_slots(ctrl);
}

static lv_obj_t *divoom_alarm_tf_sound_create_folder_btn(divoom_alarm_tf_sound_pick_ctrl *ctrl,
                                                        lv_obj_t *parent, int y, const char *label_text)
{
	lv_obj_t *btn;
	lv_obj_t *folder_img;
	lv_obj_t *label;

	btn = lv_btn_create(parent);
	lv_obj_set_size(btn, 245, 72);
	lv_obj_set_pos(btn, 14, y);
	lv_obj_set_style_pad_all(btn, 0, 0);
	lv_obj_set_style_border_width(btn, 3, 0);
	lv_obj_set_style_border_color(btn, lv_color_hex(0xD5D5D5), 0);
	lv_obj_set_style_border_opa(btn, LV_OPA_100, 0);
	lv_obj_set_style_border_color(btn, lv_color_hex(0x6D6D6D), LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_border_opa(btn, LV_OPA_100, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_radius(btn, 22, 0);
	lv_obj_set_style_shadow_width(btn, 0, 0);
	lv_obj_set_style_bg_opa(btn, 0, 0);
	lv_obj_set_style_outline_width(btn, 0, 0);
	lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_scrollbar_mode(btn, LV_SCROLLBAR_MODE_OFF);
	lv_obj_add_event_cb(btn, divoom_alarm_tf_sound_pick_left_event_cb, LV_EVENT_KEY, ctrl);

	folder_img = lv_img_create(btn);
	divoom_menu_control_lv_img_set_src(folder_img, "92/92_folder.png");
	lv_obj_set_pos(folder_img, 10, 20);

	label = lv_label_create(btn);
	lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(20, ctrl->cfg.menu_id), 0);
	lv_obj_set_style_text_color(label, lv_color_hex(0x0), 0);
	lv_obj_set_scrollbar_mode(label, LV_SCROLLBAR_MODE_OFF);
	lv_obj_set_size(label, 183, 52);
	lv_obj_set_pos(label, 52, 22);
	lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
	lv_label_set_text(label, label_text);
	return btn;
}

static void divoom_alarm_tf_sound_paint_left_selection(divoom_alarm_tf_sound_pick_ctrl *ctrl, int index)
{
	int k;

	if(ctrl == NULL)
	{
		return;
	}
	for(k = 0; k < ctrl->tf_left_item_count; k++)
	{
		if(ctrl->tf_left_folder_btn[k])
		{
			lv_obj_set_style_bg_color(ctrl->tf_left_folder_btn[k], lv_color_hex(0xD5D5D5), 0);
			lv_obj_set_style_bg_opa(ctrl->tf_left_folder_btn[k], (k == index) ? LV_OPA_100 : 0, 0);
		}
	}
	if(ctrl->tf_left_folder_btn[index])
	{
		lv_obj_set_style_bg_color(ctrl->tf_left_folder_btn[index], lv_color_hex(0xE4E4E4), 0);
		lv_obj_set_style_bg_opa(ctrl->tf_left_folder_btn[index], LV_OPA_100, 0);
	}
}

static void divoom_alarm_tf_sound_save_selection(divoom_alarm_tf_sound_pick_ctrl *ctrl, int global_index)
{
	int slot;
	int track_id;

	if(ctrl == NULL || ctrl->cfg.alarm_info == NULL
	   || global_index < 0 || global_index >= ctrl->tf_music_count)
	{
		return;
	}
	slot = global_index - ctrl->tf_right_first_index;
	if(slot < 0 || slot >= ctrl->tf_page_n || ctrl->tf_page[slot].is_dir)
	{
		return;
	}
	track_id = ctrl->tf_page[slot].id;
	if(track_id <= 0)
	{
		return;
	}
	ctrl->cfg.alarm_info->mode = DIVOOM_ALARM_TF_SOUND_MODE;
	snprintf(ctrl->cfg.alarm_info->leave_url, sizeof(ctrl->cfg.alarm_info->leave_url),
	         "%d", track_id);
	divoom_alarm_tf_sound_pick_refresh_label(ctrl);
	lv_obj_add_flag(ctrl->tf_pick_obj, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(ctrl->sound_select_obj, LV_OBJ_FLAG_HIDDEN);
	if(ctrl->cfg.sound_btn)
	{
		lv_group_focus_obj(ctrl->cfg.sound_btn);
	}
}

static void divoom_alarm_tf_sound_pick_right_event_cb(lv_event_t *e)
{
	int i;
	int slot;
	int g;
	int last_g;
	lv_obj_t *cur_obj;
	divoom_alarm_tf_sound_pick_ctrl *ctrl = (divoom_alarm_tf_sound_pick_ctrl *)lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);

	cur_obj = lv_event_get_target(e);
	if(code != LV_EVENT_KEY || ctrl == NULL)
	{
		return;
	}
	{
		char c = *((char *)lv_event_get_param(e));

		for(slot = -1, i = 0; i < DIVOOM_ALARM_TF_SOUND_VISIBLE_ROWS; i++)
		{
			if(ctrl->tf_right_music_btn[i] == cur_obj)
			{
				slot = i;
				break;
			}
		}
		if(slot >= 0 && slot < DIVOOM_ALARM_TF_SOUND_VISIBLE_ROWS)
		{
			ctrl->tf_right_focus_slot = slot;
		}

		if(c == DIVOOM_LVGL_KEY_ENTER && ctrl->tf_page_n > 0 && ctrl->tf_music_count > 0)
		{
			g = divoom_alarm_tf_sound_get_global_index(ctrl);
			if(g >= 0)
			{
				divoom_alarm_tf_sound_save_selection(ctrl, g);
			}
			return;
		}
		if(c == DIVOOM_LVGL_KEY_ESC)
		{
			if(ctrl->tf_left_selected_index >= 0
			   && ctrl->tf_left_selected_index < ctrl->tf_left_item_count
			   && ctrl->tf_left_folder_btn[ctrl->tf_left_selected_index])
			{
				divoom_alarm_tf_sound_paint_left_selection(ctrl, ctrl->tf_left_selected_index);
				lv_group_focus_obj(ctrl->tf_left_folder_btn[ctrl->tf_left_selected_index]);
				lv_obj_scroll_to_view(ctrl->tf_left_folder_btn[ctrl->tf_left_selected_index], LV_ANIM_OFF);
			}
			return;
		}
		if(c == DIVOOM_LVGL_KEY_PRE)
		{
			if(ctrl->tf_music_count <= 0)
			{
				return;
			}
			if(ctrl->tf_right_focus_slot > 0)
			{
				ctrl->tf_right_focus_slot--;
				if(ctrl->tf_right_music_btn[ctrl->tf_right_focus_slot])
				{
					lv_group_focus_obj(ctrl->tf_right_music_btn[ctrl->tf_right_focus_slot]);
					lv_obj_scroll_to_view(ctrl->tf_right_music_btn[ctrl->tf_right_focus_slot], LV_ANIM_ON);
				}
				return;
			}
			if(ctrl->tf_right_first_index > 0)
			{
				ctrl->tf_right_first_index--;
				divoom_alarm_tf_sound_refresh_right_slots(ctrl);
				return;
			}
			if(ctrl->tf_music_count > DIVOOM_ALARM_TF_SOUND_VISIBLE_ROWS)
			{
				ctrl->tf_right_first_index = ctrl->tf_music_count - DIVOOM_ALARM_TF_SOUND_VISIBLE_ROWS;
				ctrl->tf_right_focus_slot = DIVOOM_ALARM_TF_SOUND_VISIBLE_ROWS - 1;
			}
			else
			{
				ctrl->tf_right_first_index = 0;
				ctrl->tf_right_focus_slot = ctrl->tf_music_count - 1;
			}
			divoom_alarm_tf_sound_refresh_right_slots(ctrl);
			return;
		}
		if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			if(ctrl->tf_music_count <= 0)
			{
				return;
			}
			g = divoom_alarm_tf_sound_get_global_index(ctrl);
			last_g = ctrl->tf_music_count - 1;
			if(g >= 0 && g < last_g)
			{
				if(ctrl->tf_right_focus_slot < DIVOOM_ALARM_TF_SOUND_VISIBLE_ROWS - 1
				   && ctrl->tf_right_first_index + ctrl->tf_right_focus_slot + 1 < ctrl->tf_music_count)
				{
					ctrl->tf_right_focus_slot++;
					if(ctrl->tf_right_music_btn[ctrl->tf_right_focus_slot])
					{
						lv_group_focus_obj(ctrl->tf_right_music_btn[ctrl->tf_right_focus_slot]);
						lv_obj_scroll_to_view(ctrl->tf_right_music_btn[ctrl->tf_right_focus_slot], LV_ANIM_ON);
					}
				}
				else
				{
					ctrl->tf_right_first_index++;
					divoom_alarm_tf_sound_refresh_right_slots(ctrl);
				}
				return;
			}
			ctrl->tf_right_first_index = 0;
			ctrl->tf_right_focus_slot = 0;
			divoom_alarm_tf_sound_refresh_right_slots(ctrl);
		}
	}
}

static void divoom_alarm_tf_sound_pick_left_event_cb(lv_event_t *e)
{
	int i;
	int index = -1;
	lv_obj_t *cur_obj;
	divoom_alarm_tf_sound_pick_ctrl *ctrl = (divoom_alarm_tf_sound_pick_ctrl *)lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);

	cur_obj = lv_event_get_target(e);
	if(code != LV_EVENT_KEY || ctrl == NULL)
	{
		return;
	}
	{
		char c = *((char *)lv_event_get_param(e));

		for(i = 0; i < ctrl->tf_left_item_count; i++)
		{
			if(ctrl->tf_left_folder_btn[i] != NULL && cur_obj == ctrl->tf_left_folder_btn[i])
			{
				index = i;
				break;
			}
		}
		if(index < 0)
		{
			return;
		}
		if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			index = (index + 1) % ctrl->tf_left_item_count;
			lv_group_focus_obj(ctrl->tf_left_folder_btn[index]);
			lv_obj_scroll_to_view(ctrl->tf_left_folder_btn[index], LV_ANIM_ON);
		}
		else if(c == DIVOOM_LVGL_KEY_PRE)
		{
			index = (index - 1 + ctrl->tf_left_item_count) % ctrl->tf_left_item_count;
			lv_group_focus_obj(ctrl->tf_left_folder_btn[index]);
			lv_obj_scroll_to_view(ctrl->tf_left_folder_btn[index], LV_ANIM_ON);
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			ctrl->tf_left_selected_index = index;
			divoom_alarm_tf_sound_paint_left_selection(ctrl, index);
			divoom_alarm_tf_sound_update_music_list(ctrl);
			if(ctrl->tf_music_count > 0 && ctrl->tf_right_music_btn[0])
			{
				lv_group_focus_obj(ctrl->tf_right_music_btn[0]);
				lv_obj_scroll_to_view(ctrl->tf_right_music_btn[0], LV_ANIM_OFF);
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_alarm_tf_sound_pick_refresh_label(ctrl);
			lv_obj_add_flag(ctrl->tf_pick_obj, LV_OBJ_FLAG_HIDDEN);
			lv_obj_clear_flag(ctrl->sound_select_obj, LV_OBJ_FLAG_HIDDEN);
			lv_group_focus_obj(ctrl->sound_select_btn[3]);
			lv_obj_scroll_to_view(ctrl->sound_select_btn[3], LV_ANIM_OFF);
		}
	}
}

static lv_obj_t *divoom_alarm_tf_sound_create_sound_list_btn(divoom_alarm_tf_sound_pick_ctrl *ctrl,
                                                            lv_obj_t *parent, int idx, int y,
                                                            const char *title_key, const char *desp_key,
                                                            uint8 disabled)
{
	lv_obj_t *btn;
	lv_obj_t *title;
	lv_obj_t *desp;
	lv_obj_t *arrow;

	btn = lv_btn_create(parent);
	lv_obj_set_style_pad_all(btn, 0, 0);
	lv_obj_set_style_pad_gap(btn, 0, 0);
	lv_obj_set_style_bg_color(btn, lv_color_hex(0xF2F2F2), 0);
	lv_obj_set_style_radius(btn, 17, 0);
	lv_obj_set_style_border_width(btn, 4, 0);
	lv_obj_set_style_outline_width(btn, 0, 0);
	lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_shadow_width(btn, 0, 0);
	lv_obj_set_style_border_color(btn, lv_color_hex(0xF2F2F2), 0);
	lv_obj_set_style_border_color(btn, lv_color_hex(0xE89B00), LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_size(btn, 712, 120);
	lv_obj_set_pos(btn, 44, y);
	if(disabled)
	{
		lv_obj_add_state(btn, LV_STATE_DISABLED);
		lv_obj_set_style_opa(btn, LV_OPA_30, 0);
		lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);
	}
	else
	{
		lv_obj_add_event_cb(btn, divoom_alarm_tf_sound_select_menu_event_cb, LV_EVENT_KEY, ctrl);
	}
	ctrl->sound_select_btn[idx] = btn;

	title = lv_label_create(btn);
	lv_obj_set_size(title, 400, 34);
	lv_obj_set_pos(title, 32, 28);
	lv_obj_set_style_text_color(title, lv_color_black(), 0);
	lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(26, ctrl->cfg.menu_id), 0);
	lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_LEFT, 0);
	lv_label_set_text(title, divoom_menu_control_get_disp_str(title_key));

	desp = lv_label_create(btn);
	lv_obj_set_size(desp, 560, 26);
	lv_obj_set_pos(desp, 32, 66);
	lv_obj_set_style_text_color(desp, lv_color_hex(0x5B5B5B), 0);
	lv_obj_set_style_text_font(desp, divoom_menu_control_get_font_info(20, ctrl->cfg.menu_id), 0);
	lv_obj_set_style_text_align(desp, LV_TEXT_ALIGN_LEFT, 0);
	lv_label_set_text(desp, divoom_menu_control_get_disp_str(desp_key));

	if(!disabled)
	{
		arrow = lv_img_create(btn);
		lv_obj_set_pos(arrow, 644, 42);
		divoom_menu_control_lv_img_set_src(arrow, "49/49_right.png");
	}
	return btn;
}

static void divoom_alarm_tf_sound_rebuild_left(divoom_alarm_tf_sound_pick_ctrl *ctrl)
{
	int i;
	int index;
	lv_obj_t *left_panel;

	if(ctrl == NULL || ctrl->tf_left_panel == NULL)
	{
		return;
	}
	left_panel = (lv_obj_t *)ctrl->tf_left_panel;
	lv_obj_clean(left_panel);
	for(i = 0; i < DIVOOM_TF_MUSIC_MAX_ROOT + 2; i++)
	{
		ctrl->tf_left_folder_btn[i] = NULL;
	}

	ctrl->tf_hub_count = divoom_alarm_tf_sound_load_hub(ctrl->tf_hub, DIVOOM_TF_MUSIC_MAX_ROOT);
	index = 0;
	if(ctrl->tf_hub_count <= 0)
	{
		ctrl->tf_left_folder_btn[index++] = divoom_alarm_tf_sound_create_folder_btn(
		    ctrl, left_panel, 14, divoom_menu_control_get_disp_str("Allpath"));
		ctrl->tf_left_item_count = 1;
	}
	else
	{
		for(i = 0; i < ctrl->tf_hub_count && index < DIVOOM_TF_MUSIC_MAX_ROOT + 2; i++)
		{
			ctrl->tf_left_folder_btn[index++] = divoom_alarm_tf_sound_create_folder_btn(
			    ctrl, left_panel, 14 + 77 * i,
			    ctrl->tf_hub[i].name[0] ? ctrl->tf_hub[i].name : "");
		}
		ctrl->tf_left_item_count = index;
	}
	if(ctrl->tf_left_selected_index < 0 || ctrl->tf_left_selected_index >= ctrl->tf_left_item_count)
	{
		ctrl->tf_left_selected_index = 0;
	}
	divoom_alarm_tf_sound_paint_left_selection(ctrl, ctrl->tf_left_selected_index);
}

static void divoom_alarm_tf_sound_create_tf_pick_page(divoom_alarm_tf_sound_pick_ctrl *ctrl)
{
	lv_obj_t *left_panel;
	lv_obj_t *title;

	if(ctrl == NULL || ctrl->tf_pick_obj != NULL || ctrl->cfg.parent == NULL)
	{
		return;
	}
	if(!ctrl->tf_styles_inited)
	{
		lv_style_init(&ctrl->tf_normal_type);
		lv_style_set_bg_color(&ctrl->tf_normal_type, lv_color_hex(0xE4E4E4));
		lv_style_set_pad_all(&ctrl->tf_normal_type, 0);
		lv_style_set_border_width(&ctrl->tf_normal_type, 4);
		lv_style_set_border_color(&ctrl->tf_normal_type, lv_color_hex(0xE4E4E4));
		lv_style_set_radius(&ctrl->tf_normal_type, 17);
		lv_style_set_shadow_width(&ctrl->tf_normal_type, 0);
		lv_style_set_outline_width(&ctrl->tf_normal_type, 0);

		lv_style_init(&ctrl->tf_press_type);
		lv_style_set_bg_color(&ctrl->tf_press_type, lv_color_hex(0xE4E4E4));
		lv_style_set_pad_all(&ctrl->tf_press_type, 0);
		lv_style_set_border_width(&ctrl->tf_press_type, 4);
		lv_style_set_border_color(&ctrl->tf_press_type, lv_color_hex(0x6D6D6D));
		lv_style_set_radius(&ctrl->tf_press_type, 17);
		lv_style_set_shadow_width(&ctrl->tf_press_type, 0);
		lv_style_set_outline_width(&ctrl->tf_press_type, 0);
		ctrl->tf_styles_inited = 1;
	}

	ctrl->tf_pick_obj = lv_obj_create(ctrl->cfg.parent);
	lv_obj_set_style_bg_color(ctrl->tf_pick_obj, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_border_width(ctrl->tf_pick_obj, 0, 0);
	lv_obj_set_style_pad_all(ctrl->tf_pick_obj, 0, 0);
	lv_obj_set_size(ctrl->tf_pick_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(ctrl->tf_pick_obj, 0, 0);
	lv_obj_add_flag(ctrl->tf_pick_obj, LV_OBJ_FLAG_HIDDEN);

	title = lv_label_create(ctrl->tf_pick_obj);
	lv_label_set_text(title, divoom_menu_control_get_disp_str("LocalMusic"));
	lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(32, ctrl->cfg.menu_id), 0);
	lv_obj_set_style_text_color(title, lv_color_hex(0x0), 0);
	lv_obj_set_size(title, 400, 60);
	lv_obj_set_pos(title, 200, 25);

	left_panel = lv_obj_create(ctrl->tf_pick_obj);
	lv_obj_set_size(left_panel, 273, DIVOOM_ALARM_TF_SOUND_PANEL_H);
	lv_obj_set_pos(left_panel, 10, 90);
	lv_obj_set_style_bg_opa(left_panel, LV_OPA_100, 0);
	lv_obj_set_style_bg_color(left_panel, lv_color_hex(0xD5D5D5), 0);
	lv_obj_set_style_border_width(left_panel, 0, 0);
	lv_obj_set_style_pad_all(left_panel, 0, 0);
	lv_obj_set_style_radius(left_panel, 32, 0);

	ctrl->tf_left_panel = left_panel;
	divoom_alarm_tf_sound_rebuild_left(ctrl);

	ctrl->tf_left_selected_index = 0;
	ctrl->tf_right_first_index = 0;
	ctrl->tf_right_focus_slot = 0;

	ctrl->tf_right_panel = lv_obj_create(ctrl->tf_pick_obj);
	lv_obj_set_size(ctrl->tf_right_panel, 507, DIVOOM_ALARM_TF_SOUND_PANEL_H);
	lv_obj_set_pos(ctrl->tf_right_panel, 293, 80);
	lv_obj_set_style_bg_opa(ctrl->tf_right_panel, LV_OPA_100, 0);
	lv_obj_set_style_bg_color(ctrl->tf_right_panel, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_border_width(ctrl->tf_right_panel, 0, 0);
	lv_obj_set_style_radius(ctrl->tf_right_panel, 0, 0);
	lv_obj_set_style_pad_all(ctrl->tf_right_panel, 0, 0);
	lv_obj_set_style_pad_bottom(ctrl->tf_right_panel, 20, 0);
	lv_obj_set_style_pad_top(ctrl->tf_right_panel, 20, 0);
	lv_obj_set_scroll_dir(ctrl->tf_right_panel, LV_DIR_VER);
	lv_obj_set_scrollbar_mode(ctrl->tf_right_panel, LV_SCROLLBAR_MODE_OFF);
	lv_obj_set_flex_flow(ctrl->tf_right_panel, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(ctrl->tf_right_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

	divoom_alarm_tf_sound_paint_left_selection(ctrl, 0);
}

static void divoom_alarm_tf_sound_create_sound_select_page(divoom_alarm_tf_sound_pick_ctrl *ctrl)
{
	lv_obj_t *classic_btn;
	lv_obj_t *music_label;
	lv_obj_t *title;

	if(ctrl == NULL || ctrl->cfg.parent == NULL || ctrl->sound_select_obj != NULL)
	{
		return;
	}

	ctrl->pdirect_sound_select_handle = divoom_menu_direct_ctrl_create(1, DIVOOM_ALARM_TF_SOUND_ITEM_COUNT);

	ctrl->sound_select_obj = lv_obj_create(ctrl->cfg.parent);
	lv_obj_set_style_bg_color(ctrl->sound_select_obj, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_border_width(ctrl->sound_select_obj, 0, 0);
	lv_obj_set_style_pad_all(ctrl->sound_select_obj, 0, 0);
	lv_obj_set_size(ctrl->sound_select_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(ctrl->sound_select_obj, 0, 0);
	lv_obj_add_flag(ctrl->sound_select_obj, LV_OBJ_FLAG_HIDDEN);

	title = lv_label_create(ctrl->sound_select_obj);
	lv_obj_set_size(title, 360, 56);
	lv_obj_set_pos(title, 220, 52);
	lv_obj_set_style_text_color(title, lv_color_black(), 0);
	lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(32, ctrl->cfg.menu_id), 0);
	lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_text(title, divoom_menu_control_get_disp_str("Bells"));

	classic_btn = divoom_alarm_tf_sound_create_sound_list_btn(ctrl, ctrl->sound_select_obj, 0, 160,
	                                                          "classic", "classic_desp", 0);
	music_label = lv_label_create(classic_btn);
	lv_obj_set_size(music_label, 209, 34);
	lv_obj_set_pos(music_label, 427, 43);
	lv_obj_set_style_text_color(music_label, lv_color_hex(0x0), 0);
	lv_obj_set_style_text_font(music_label, divoom_menu_control_get_font_info(26, ctrl->cfg.menu_id), 0);
	lv_obj_set_style_text_align(music_label, LV_TEXT_ALIGN_RIGHT, 0);
	if(ctrl->cfg.alarm_info)
	{
		lv_label_set_text_fmt(music_label, "Music %d", ctrl->cfg.alarm_info->tip_index + 1);
	}
	if(ctrl->cfg.sound_music_label)
	{
		lv_label_set_text_fmt(ctrl->cfg.sound_music_label, "Music %d", ctrl->cfg.alarm_info->tip_index + 1);
	}

	divoom_alarm_tf_sound_create_sound_list_btn(ctrl, ctrl->sound_select_obj, 1, 298, "scene", "scene_desp",
	                                           ctrl->cfg.open_scene ? 0 : 1);
	divoom_alarm_tf_sound_create_sound_list_btn(ctrl, ctrl->sound_select_obj, 2, 436,
	                                           "SleepAidTabRecord", "record_desp",
	                                           ctrl->cfg.open_record ? 0 : 1);
	divoom_alarm_tf_sound_create_sound_list_btn(ctrl, ctrl->sound_select_obj, 3, 574, "TFCard", "TFCard_desp",
	                                           divoom_alarm_tf_sound_card_available() ? 0 : 1);

	divoom_alarm_tf_sound_create_tf_pick_page(ctrl);
	divoom_alarm_tf_sound_pick_apply_tf_state(ctrl);
}

static void divoom_alarm_tf_sound_select_menu_event_cb(lv_event_t *e)
{
	int i;
	lv_obj_t *cur_obj;
	lv_obj_t *next_obj;
	divoom_alarm_tf_sound_pick_ctrl *ctrl = (divoom_alarm_tf_sound_pick_ctrl *)lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);

	cur_obj = lv_event_get_target(e);
	if(code != LV_EVENT_KEY || ctrl == NULL)
	{
		return;
	}
	{
		char c = *((char *)lv_event_get_param(e));

		if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			next_obj = divoom_menu_direct_ctrl_get(ctrl->pdirect_sound_select_handle, cur_obj, DIVOOM_MENU_DIRECT_UP);
			if(next_obj && next_obj != cur_obj)
			{
				lv_group_focus_obj(next_obj);
				lv_obj_scroll_to_view(next_obj, LV_ANIM_OFF);
			}
		}
		else if(c == DIVOOM_LVGL_KEY_PRE)
		{
			next_obj = divoom_menu_direct_ctrl_get(ctrl->pdirect_sound_select_handle, cur_obj, DIVOOM_MENU_DIRECT_DOWN);
			if(next_obj && next_obj != cur_obj)
			{
				lv_group_focus_obj(next_obj);
				lv_obj_scroll_to_view(next_obj, LV_ANIM_OFF);
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_alarm_tf_sound_pick_refresh_label(ctrl);
			lv_obj_add_flag(ctrl->sound_select_obj, LV_OBJ_FLAG_HIDDEN);
			if(ctrl->cfg.sound_btn)
			{
				lv_group_focus_obj(ctrl->cfg.sound_btn);
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			for(i = 0; i < DIVOOM_ALARM_TF_SOUND_ITEM_COUNT; i++)
			{
				if(cur_obj == ctrl->sound_select_btn[i])
				{
					break;
				}
			}
			if(i == 3 && !divoom_alarm_tf_sound_card_available())
			{
				return;
			}
			if(i >= DIVOOM_ALARM_TF_SOUND_ITEM_COUNT)
			{
				return;
			}
			if(i == 0)
			{
				lv_obj_add_flag(ctrl->sound_select_obj, LV_OBJ_FLAG_HIDDEN);
				if(ctrl->cfg.open_classic)
				{
					ctrl->cfg.open_classic(ctrl->cfg.user_data);
				}
			}
			else if(i == 1)
			{
				lv_obj_add_flag(ctrl->sound_select_obj, LV_OBJ_FLAG_HIDDEN);
				if(ctrl->cfg.open_scene)
				{
					ctrl->cfg.open_scene(ctrl->cfg.user_data);
				}
			}
			else if(i == 2)
			{
				lv_obj_add_flag(ctrl->sound_select_obj, LV_OBJ_FLAG_HIDDEN);
				if(ctrl->cfg.open_record)
				{
					ctrl->cfg.open_record(ctrl->cfg.user_data);
				}
			}
			else if(i == 3)
			{
				divoom_alarm_tf_sound_enter_tf_pick(ctrl);
			}
		}
	}
}

void divoom_alarm_tf_sound_pick_apply_tf_state(divoom_alarm_tf_sound_pick_ctrl *ctrl)
{
	int tf_ok;
	int item_idx;
	lv_obj_t *tf_btn;

	if(ctrl == NULL)
	{
		return;
	}
	tf_ok = divoom_alarm_tf_sound_card_available();
	if(!tf_ok && ctrl->tf_pick_obj && !lv_obj_has_flag(ctrl->tf_pick_obj, LV_OBJ_FLAG_HIDDEN))
	{
		lv_obj_add_flag(ctrl->tf_pick_obj, LV_OBJ_FLAG_HIDDEN);
		if(ctrl->sound_select_obj)
		{
			lv_obj_clear_flag(ctrl->sound_select_obj, LV_OBJ_FLAG_HIDDEN);
		}
	}
	tf_btn = ctrl->sound_select_btn[3];
	if(tf_btn)
	{
		if(tf_ok)
		{
			lv_obj_clear_state(tf_btn, LV_STATE_DISABLED);
			lv_obj_set_style_opa(tf_btn, LV_OPA_COVER, 0);
			lv_obj_add_flag(tf_btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);
			lv_obj_remove_event_cb(tf_btn, divoom_alarm_tf_sound_select_menu_event_cb);
			lv_obj_add_event_cb(tf_btn, divoom_alarm_tf_sound_select_menu_event_cb, LV_EVENT_KEY, ctrl);
		}
		else
		{
			lv_obj_add_state(tf_btn, LV_STATE_DISABLED);
			lv_obj_set_style_opa(tf_btn, LV_OPA_30, 0);
			lv_obj_clear_flag(tf_btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);
			lv_obj_remove_event_cb(tf_btn, divoom_alarm_tf_sound_select_menu_event_cb);
		}
	}
	item_idx = 0;
	if(ctrl->sound_select_btn[0])
	{
		item_idx++;
	}
	if(ctrl->sound_select_btn[1] && ctrl->cfg.open_scene)
	{
		item_idx++;
	}
	if(ctrl->sound_select_btn[2] && ctrl->cfg.open_record)
	{
		item_idx++;
	}
	if(tf_ok && ctrl->sound_select_btn[3])
	{
		item_idx++;
	}
	if(item_idx <= 0)
	{
		item_idx = 1;
	}
	if(ctrl->pdirect_sound_select_handle)
	{
		divoom_menu_direct_ctrl_destory(ctrl->pdirect_sound_select_handle);
		ctrl->pdirect_sound_select_handle = NULL;
	}
	ctrl->pdirect_sound_select_handle = divoom_menu_direct_ctrl_create(1, item_idx);
	if(ctrl->pdirect_sound_select_handle == NULL)
	{
		return;
	}
	item_idx = 0;
	if(ctrl->sound_select_btn[0])
	{
		divoom_menu_direct_ctrl_update(ctrl->pdirect_sound_select_handle, ctrl->sound_select_btn[0], 0, item_idx++);
	}
	if(ctrl->sound_select_btn[1] && ctrl->cfg.open_scene)
	{
		divoom_menu_direct_ctrl_update(ctrl->pdirect_sound_select_handle, ctrl->sound_select_btn[1], 0, item_idx++);
	}
	if(ctrl->sound_select_btn[2] && ctrl->cfg.open_record)
	{
		divoom_menu_direct_ctrl_update(ctrl->pdirect_sound_select_handle, ctrl->sound_select_btn[2], 0, item_idx++);
	}
	if(tf_ok && ctrl->sound_select_btn[3])
	{
		divoom_menu_direct_ctrl_update(ctrl->pdirect_sound_select_handle, ctrl->sound_select_btn[3], 0, item_idx++);
	}
}

divoom_alarm_tf_sound_pick_ctrl *divoom_alarm_tf_sound_pick_create(const divoom_alarm_tf_sound_pick_cfg *cfg)
{
	divoom_alarm_tf_sound_pick_ctrl *ctrl;

	if(cfg == NULL || cfg->parent == NULL || cfg->alarm_info == NULL)
	{
		return NULL;
	}
	ctrl = (divoom_alarm_tf_sound_pick_ctrl *)divoom_system_malloc(sizeof(divoom_alarm_tf_sound_pick_ctrl));
	if(ctrl == NULL)
	{
		return NULL;
	}
	memset(ctrl, 0, sizeof(divoom_alarm_tf_sound_pick_ctrl));
	ctrl->cfg = *cfg;
	divoom_alarm_tf_sound_create_sound_select_page(ctrl);
	return ctrl;
}

void divoom_alarm_tf_sound_pick_destroy(divoom_alarm_tf_sound_pick_ctrl *ctrl)
{
	if(ctrl == NULL)
	{
		return;
	}
	divoom_alarm_tf_sound_free_music_list(ctrl);
	if(ctrl->tf_styles_inited)
	{
		lv_style_reset(&ctrl->tf_normal_type);
		lv_style_reset(&ctrl->tf_press_type);
	}
	if(ctrl->pdirect_sound_select_handle)
	{
		divoom_menu_direct_ctrl_destory(ctrl->pdirect_sound_select_handle);
	}
	divoom_system_free(ctrl);
}

void divoom_alarm_tf_sound_pick_open_select(divoom_alarm_tf_sound_pick_ctrl *ctrl)
{
	lv_obj_t *focus_btn;

	if(ctrl == NULL || ctrl->sound_select_obj == NULL)
	{
		return;
	}
	divoom_alarm_tf_sound_pick_apply_tf_state(ctrl);
	lv_obj_clear_flag(ctrl->sound_select_obj, LV_OBJ_FLAG_HIDDEN);
	lv_obj_move_foreground(ctrl->sound_select_obj);

	focus_btn = ctrl->sound_select_btn[0];
	if(ctrl->cfg.alarm_info)
	{
		if(ctrl->cfg.alarm_info->mode == 1)
		{
			focus_btn = ctrl->sound_select_btn[2];
		}
		else if(ctrl->cfg.alarm_info->mode == DIVOOM_ALARM_TF_SOUND_MODE
		        && divoom_alarm_tf_sound_card_available())
		{
			focus_btn = ctrl->sound_select_btn[3];
		}
	}
	if(focus_btn)
	{
		lv_group_focus_obj(focus_btn);
		lv_obj_scroll_to_view(focus_btn, LV_ANIM_OFF);
	}
}

int divoom_alarm_tf_sound_pick_is_subpage_visible(divoom_alarm_tf_sound_pick_ctrl *ctrl)
{
	if(ctrl == NULL)
	{
		return 0;
	}
	if(ctrl->sound_select_obj && !lv_obj_has_flag(ctrl->sound_select_obj, LV_OBJ_FLAG_HIDDEN))
	{
		return 1;
	}
	if(ctrl->tf_pick_obj && !lv_obj_has_flag(ctrl->tf_pick_obj, LV_OBJ_FLAG_HIDDEN))
	{
		return 1;
	}
	return 0;
}

#endif

#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_tf_music.h"
#include <string.h>

#ifdef DIVOOM_TIME_FRAME_SPEAKER

#if DIVOOM_TF_MUSIC_FAVORITES_UI
static int divoom_lvgl_tf_music_menu_setting_kind(int i)
{
	return i;
}
#elif defined(DIVOOM_TIMES_VIBE)
static int divoom_lvgl_tf_music_menu_setting_kind(int i)
{
	switch (i)
	{
	case 0: return 0;
	case 1: return 5;
	case 2: return 1;
	case 3: return 3;
	default: return -1;
	}
}
#else
static int divoom_lvgl_tf_music_menu_setting_kind(int i)
{
	return (i >= 2) ? (i + 1) : i;
}
#endif

#if defined(DIVOOM_TIMES_VIBE)
#define TF_MUSIC_MENU_IDX_REFRESH     1
#define TF_MUSIC_MENU_IDX_NOW_PLAY    2
#define TF_MUSIC_MENU_IDX_PLAY_ORDER  3
#else
#define TF_MUSIC_MENU_IDX_NOW_PLAY    1
#define TF_MUSIC_MENU_IDX_PLAY_ORDER  2
#endif

typedef struct _divoom_lvgl_tf_music_menu_ctrl
{
    int menu_id;
    int  play_music_id;
    int  play_status;
    int  bg_index;
    int  play_mode;
    int  play_order;
    char music_name[256];
    char music_title[256];
    void *main_obj;

    void *time_handle;

    lv_style_t cout_style;
	lv_style_t normal_type;
	lv_style_t press_type;

	void *select_btn[5];		//选择按钮
	void *select_label[5];		//选择标签
	void *pdirect_handle;
	void *cur_active_obj;

	int	last_play_music_id;
	Pdivoom_tf_music_item pcur_play_music;
	void *play_status_img;		//播放状态图片
	void *play_name_label;		//播放名称标签
	void *like_img;				//收藏图片
	void *play_order_img;		//播放顺序图片

	divoom_tf_entry root_folders[DIVOOM_TF_MUSIC_MAX_ROOT];
	int root_count;

	lv_timer_t *music_refresh_timer;

#if defined(DIVOOM_TIMES_VIBE)
	void *refresh_flush_img;
	void *refresh_done_label;
	lv_anim_t refresh_spin_anim;
	lv_timer_t *refresh_done_timer;
	int refresh_ui_busy;
#endif

	void *message_box_handle;	
	void *bg_cover_obj;

	Pdivoom_sys_setting_info psys_config;
}divoom_lvgl_tf_music_menu_ctrl, *Pdivoom_lvgl_tf_music_menu_ctrl;

void divoom_lvgl_tf_music_menu_create_playback_order_callback(void *handle, int ctrl_id)
{
    Pdivoom_lvgl_tf_music_menu_ctrl phandle = (Pdivoom_lvgl_tf_music_menu_ctrl)handle;
    
    if(ctrl_id < 0)
    {
    }
    else 
    {
        phandle->psys_config->tf_music_mode = ctrl_id;
		divoom_main_set_tf_music_mode(phandle->psys_config->tf_music_mode);

		if(phandle->psys_config->tf_music_mode == 0)
		{
			divoom_menu_control_lv_img_set_src(phandle->play_order_img, "34/34_sequential.png");
		}
		else if(phandle->psys_config->tf_music_mode == 1)
		{
			divoom_menu_control_lv_img_set_src(phandle->play_order_img, "34/34_single_repeat.png");
		}
		else if(phandle->psys_config->tf_music_mode == 2)
		{
			divoom_menu_control_lv_img_set_src(phandle->play_order_img, "34/34_random.png");
		}

		//更新播放顺序标签
		if(phandle->psys_config->tf_music_mode == 0)
		{
			lv_label_set_text(phandle->select_label[3], divoom_menu_control_get_disp_str("SequentialPlay"));
		}
		else if(phandle->psys_config->tf_music_mode == 1)
		{
			lv_label_set_text(phandle->select_label[3], divoom_menu_control_get_disp_str("SinglePlay"));
		}
		else if(phandle->psys_config->tf_music_mode == 2)
		{
			lv_label_set_text(phandle->select_label[3], divoom_menu_control_get_disp_str("RandomPlay"));
		}
		divoom_main_save_config();
    }
    lv_group_focus_obj(phandle->select_btn[3]);
    divoom_lvgl_message_box_destroy(phandle->message_box_handle, 0);
    
	lv_obj_add_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);
    phandle->message_box_handle = NULL;
}

void divoom_lvgl_tf_music_menu_create_playback_order_box(Pdivoom_lvgl_tf_music_menu_ctrl phandle)
{
	if(phandle->message_box_handle != NULL)
	{
		return;
	}

	lv_obj_clear_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);
    phandle->message_box_handle = divoom_lvgl_message_box_init(phandle->menu_id, phandle->bg_cover_obj, phandle, 
        divoom_lvgl_tf_music_menu_create_playback_order_callback, 80, 487, 640, 307, NULL, lv_color_hex(0xF2F2F2), 3);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("SequentialPlay"), 26,lv_color_hex(0xF2F2F2), lv_color_hex(0x6D6D6D),lv_color_hex(0), 0, 
        32, 32, 576, 80);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("SinglePlay"), 26, lv_color_hex(0xF2F2F2),lv_color_hex(0x6D6D6D),lv_color_hex(0), 1, 
        32, 112, 576, 80);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("RandomPlay"), 26,lv_color_hex(0xF2F2F2), lv_color_hex(0x6D6D6D),lv_color_hex(0), 2, 
        32, 192, 576, 80);

    divoom_lvgl_message_box_set_active_item(phandle->message_box_handle, phandle->psys_config->tf_music_mode);
}

void divoom_lvgl_tf_music_menu_create_audio_effect_callback(void *handle, int ctrl_id)
{
    Pdivoom_lvgl_tf_music_menu_ctrl phandle = (Pdivoom_lvgl_tf_music_menu_ctrl)handle;
    
    if(ctrl_id < 0)
    {
    }
    else 
    {
        phandle->psys_config->audio_effect = ctrl_id;
		if(phandle->psys_config->audio_effect == 0)
		{
			lv_label_set_text(phandle->select_label[4], divoom_menu_control_get_disp_str("AudioEffect1"));
		}
		else if(phandle->psys_config->audio_effect == 1)
		{
			lv_label_set_text(phandle->select_label[4], divoom_menu_control_get_disp_str("AudioEffect2"));
		}
		else if(phandle->psys_config->audio_effect == 2)
		{
			lv_label_set_text(phandle->select_label[4], divoom_menu_control_get_disp_str("AudioEffect3"));
		}
		else if(phandle->psys_config->audio_effect == 3)
		{
			lv_label_set_text(phandle->select_label[4], divoom_menu_control_get_disp_str("AudioEffect4"));
		}
		else if(phandle->psys_config->audio_effect == 4)
		{
			lv_label_set_text(phandle->select_label[4], divoom_menu_control_get_disp_str("AudioEffect5"));
		}
		else if(phandle->psys_config->audio_effect == 5)
		{
			lv_label_set_text(phandle->select_label[4], divoom_menu_control_get_disp_str("AudioEffect6"));
		}
		else if(phandle->psys_config->audio_effect == 6)
		{
			lv_label_set_text(phandle->select_label[4], divoom_menu_control_get_disp_str("AudioEffect7"));
		}
		else if(phandle->psys_config->audio_effect == 7)
		{
			lv_label_set_text(phandle->select_label[4], divoom_menu_control_get_disp_str("AudioEffect8"));
		}

		//divoom_main_send_sys_settings_config();
        divoom_main_save_config();
    }
    lv_group_focus_obj(phandle->select_btn[4]);
    divoom_lvgl_message_box_destroy(phandle->message_box_handle, 0);
    lv_obj_add_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);

    phandle->message_box_handle = NULL;
}


void divoom_lvgl_tf_music_menu_create_audio_effects_setting(Pdivoom_lvgl_tf_music_menu_ctrl phandle)
{
	if(phandle->message_box_handle != NULL)
	{
		return;
	}

	lv_obj_clear_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);
    phandle->message_box_handle = divoom_lvgl_message_box_init(phandle->menu_id, phandle->bg_cover_obj, phandle, 
        divoom_lvgl_tf_music_menu_create_audio_effect_callback, 80, 288, 640, 704, NULL, lv_color_hex(0xF2F2F2), 8);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("AudioEffect1"), 26,lv_color_hex(0xF2F2F2), lv_color_hex(0),lv_color_hex(0), 0, 
        32, 32, 576, 80);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("AudioEffect2"), 26, lv_color_hex(0xF2F2F2),lv_color_hex(0),lv_color_hex(0), 1, 
        32, 112, 576, 80);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("AudioEffect3"), 26,lv_color_hex(0xF2F2F2), lv_color_hex(0),lv_color_hex(0), 2, 
        32, 192, 576, 80);
	divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("AudioEffect4"), 26,lv_color_hex(0xF2F2F2), lv_color_hex(0),lv_color_hex(0), 3, 
        32, 272, 576, 80);

	divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("AudioEffect5"), 26,lv_color_hex(0xF2F2F2), lv_color_hex(0),lv_color_hex(0), 4, 
        32, 352, 576, 80);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("AudioEffect6"), 26, lv_color_hex(0xF2F2F2),lv_color_hex(0),lv_color_hex(0), 5, 
        32, 432, 576, 80);
    divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("AudioEffect7"), 26,lv_color_hex(0xF2F2F2), lv_color_hex(0),lv_color_hex(0), 6, 
        32, 512, 576, 80);
	divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("AudioEffect8"), 26,lv_color_hex(0xF2F2F2), lv_color_hex(0),lv_color_hex(0), 7, 
        32, 592, 576, 80);

    divoom_lvgl_message_box_set_active_item(phandle->message_box_handle, phandle->psys_config->audio_effect);
}

static int divoom_lvgl_tf_music_menu_load_hub(divoom_tf_entry *out, int cap)
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

void divoom_lvgl_tf_music_menu_create_current_playing_folder_callback(void *handle, int ctrl_id)
{
    Pdivoom_lvgl_tf_music_menu_ctrl phandle = (Pdivoom_lvgl_tf_music_menu_ctrl)handle;
    
    if(ctrl_id < 0)
    {
    }
    else 
    {
		if(ctrl_id == 0)
		{
			lv_label_set_text(phandle->select_label[1], divoom_menu_control_get_disp_str("Allpath"));
		}
#if DIVOOM_TF_MUSIC_FAVORITES_UI
		else if(ctrl_id == 1)
		{
			lv_label_set_text(phandle->select_label[1], divoom_menu_control_get_disp_str("Favorites"));
		}
		else
		{
			lv_label_set_text(phandle->select_label[1],
			    (ctrl_id - 2 >= 0 && ctrl_id - 2 < phandle->root_count)
			        ? phandle->root_folders[ctrl_id - 2].name : "");
		}
		divoom_main_set_tf_music_order(ctrl_id);
#else
		else
		{
			lv_label_set_text(phandle->select_label[1],
			    (ctrl_id - 1 >= 0 && ctrl_id - 1 < phandle->root_count)
			        ? phandle->root_folders[ctrl_id - 1].name : "");
		}
		divoom_main_set_tf_music_order(ctrl_id == 0 ? 0 : ctrl_id + 1);
#endif
    }
    lv_group_focus_obj(phandle->select_btn[1]);
    divoom_lvgl_message_box_destroy(phandle->message_box_handle, 0);
    lv_obj_add_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);

    phandle->message_box_handle = NULL;
}


void divoom_lvgl_tf_music_menu_create_current_playing_setting(Pdivoom_lvgl_tf_music_menu_ctrl phandle)
{
	int i, height, pos_y;

	if(phandle->message_box_handle != NULL)
	{
		return;
	}

	lv_obj_clear_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);

	phandle->root_count = divoom_lvgl_tf_music_menu_load_hub(phandle->root_folders, DIVOOM_TF_MUSIC_MAX_ROOT);

	height = 224 + phandle->root_count * 83;

	if(height >= 1104)
	{
		height = 1104;
	}

	pos_y = (DIVOOM_SCREEN_HIGH - height) / 2;

    phandle->message_box_handle = divoom_lvgl_message_box_init(phandle->menu_id, phandle->bg_cover_obj, phandle, 
        divoom_lvgl_tf_music_menu_create_current_playing_folder_callback, 80, pos_y, 640, height, NULL, lv_color_hex(0xF2F2F2),
#if DIVOOM_TF_MUSIC_FAVORITES_UI
		phandle->root_count + 2);
#else
		phandle->root_count + 1);
#endif
    
	divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("Allpath"), 26, lv_color_hex(0xF2F2F2), lv_color_hex(0),lv_color_hex(0), 0, 
        	32, 32, 576, 80);

#if DIVOOM_TF_MUSIC_FAVORITES_UI
	divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, divoom_menu_control_get_disp_str("Favorites"), 26, lv_color_hex(0xF2F2F2), lv_color_hex(0),lv_color_hex(0), 1, 
        	32, 32+80*1, 576, 80);
#endif

	for(i = 0;i < phandle->root_count; i++)
	{
#if DIVOOM_TF_MUSIC_FAVORITES_UI
		divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, phandle->root_folders[i].name, 26, lv_color_hex(0xF2F2F2), lv_color_hex(0),lv_color_hex(0), i+2, 
        	32, 32+80*(i+2), 576, 80);
#else
		divoom_lvgl_message_box_add_btn_item(phandle->message_box_handle, phandle->root_folders[i].name, 26, lv_color_hex(0xF2F2F2), lv_color_hex(0),lv_color_hex(0), i+1, 
        	32, 32+80*(i+1), 576, 80);
#endif
	}
	
    divoom_lvgl_message_box_set_active_item(phandle->message_box_handle,
        divoom_tf_music_folder_play_order_to_left(divoom_tf_music_get_play_order()));
}


#if defined(DIVOOM_TIMES_VIBE)
static void divoom_lvgl_tf_music_menu_refresh_spin_anim_cb(void *obj, int32_t v)
{
	lv_img_set_angle((lv_obj_t *)obj, (int16_t)v);
}

static void divoom_lvgl_tf_music_menu_refresh_spin_start(Pdivoom_lvgl_tf_music_menu_ctrl phandle)
{
	lv_obj_t *img;

	if (!phandle || !phandle->refresh_flush_img)
		return;

	img = (lv_obj_t *)phandle->refresh_flush_img;
	lv_anim_del(img, divoom_lvgl_tf_music_menu_refresh_spin_anim_cb);
	lv_anim_init(&phandle->refresh_spin_anim);
	lv_anim_set_var(&phandle->refresh_spin_anim, img);
	lv_anim_set_exec_cb(&phandle->refresh_spin_anim,
			    divoom_lvgl_tf_music_menu_refresh_spin_anim_cb);
	lv_anim_set_values(&phandle->refresh_spin_anim, 0, 3600);
	lv_anim_set_time(&phandle->refresh_spin_anim, 1000);
	lv_anim_set_repeat_count(&phandle->refresh_spin_anim, LV_ANIM_REPEAT_INFINITE);
	lv_anim_start(&phandle->refresh_spin_anim);
}

static void divoom_lvgl_tf_music_menu_refresh_spin_stop(Pdivoom_lvgl_tf_music_menu_ctrl phandle)
{
	if (!phandle || !phandle->refresh_flush_img)
		return;

	lv_anim_del(phandle->refresh_flush_img, divoom_lvgl_tf_music_menu_refresh_spin_anim_cb);
	lv_img_set_angle((lv_obj_t *)phandle->refresh_flush_img, 0);
}

static void divoom_lvgl_tf_music_menu_refresh_done_timer_cb(lv_timer_t *timer)
{
	Pdivoom_lvgl_tf_music_menu_ctrl phandle = (Pdivoom_lvgl_tf_music_menu_ctrl)lv_timer_get_user_data(timer);

	if (!phandle)
		return;

	if (phandle->refresh_done_label)
		lv_obj_add_flag((lv_obj_t *)phandle->refresh_done_label, LV_OBJ_FLAG_HIDDEN);
	if (phandle->refresh_flush_img)
		lv_obj_clear_flag((lv_obj_t *)phandle->refresh_flush_img, LV_OBJ_FLAG_HIDDEN);

	phandle->refresh_done_timer = NULL;
	lv_timer_del(timer);
}

static void divoom_lvgl_tf_music_menu_show_refresh_done(Pdivoom_lvgl_tf_music_menu_ctrl phandle)
{
	if (!phandle)
		return;

	divoom_lvgl_tf_music_menu_refresh_spin_stop(phandle);
	phandle->refresh_ui_busy = 0;

	if (phandle->refresh_flush_img)
		lv_obj_add_flag((lv_obj_t *)phandle->refresh_flush_img, LV_OBJ_FLAG_HIDDEN);
	if (phandle->refresh_done_label)
	{
		lv_obj_clear_flag((lv_obj_t *)phandle->refresh_done_label, LV_OBJ_FLAG_HIDDEN);
		lv_label_set_text((lv_obj_t *)phandle->refresh_done_label,
				  divoom_menu_control_get_disp_str("RefreshComplete"));
	}

	if (phandle->refresh_done_timer)
	{
		lv_timer_del(phandle->refresh_done_timer);
		phandle->refresh_done_timer = NULL;
	}
	phandle->refresh_done_timer = lv_timer_create(divoom_lvgl_tf_music_menu_refresh_done_timer_cb,
						    3000, phandle);
	lv_timer_set_repeat_count(phandle->refresh_done_timer, 1);
}

static void divoom_lvgl_tf_music_menu_refresh_event_cb(void *user, int event)
{
	Pdivoom_lvgl_tf_music_menu_ctrl phandle = (Pdivoom_lvgl_tf_music_menu_ctrl)user;

	if (!phandle)
		return;

	if (event == DIVOOM_TF_MUSIC_SONGLIST_REFRESH_DONE)
		divoom_lvgl_tf_music_menu_show_refresh_done(phandle);
	else if (phandle->refresh_ui_busy)
	{
		divoom_lvgl_tf_music_menu_refresh_spin_stop(phandle);
		phandle->refresh_ui_busy = 0;
	}
}

static void divoom_lvgl_tf_music_menu_start_refresh(Pdivoom_lvgl_tf_music_menu_ctrl phandle)
{
	if (!phandle || phandle->refresh_ui_busy)
		return;

	phandle->refresh_ui_busy = 1;
	if (phandle->refresh_done_timer)
	{
		lv_timer_del(phandle->refresh_done_timer);
		phandle->refresh_done_timer = NULL;
	}
	if (phandle->refresh_done_label)
		lv_obj_add_flag((lv_obj_t *)phandle->refresh_done_label, LV_OBJ_FLAG_HIDDEN);
	if (phandle->refresh_flush_img)
		lv_obj_clear_flag((lv_obj_t *)phandle->refresh_flush_img, LV_OBJ_FLAG_HIDDEN);

	divoom_lvgl_tf_music_menu_refresh_spin_start(phandle);
	divoom_tf_music_send_scan_music_list();
}
#endif


static void divoom_lvgl_tf_music_menu_set_event_cb(lv_event_t * e)
{
	int i;
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_tf_music_menu_ctrl phandle = (Pdivoom_lvgl_tf_music_menu_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);

    if(code == LV_EVENT_KEY)
    {
		char c = *((char *)lv_event_get_param(e));

		if(c == DIVOOM_LVGL_KEY_NEXT || c == DIVOOM_LVGL_KEY_PRE)
		{
			cur_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_obj, c == DIVOOM_LVGL_KEY_NEXT?DIVOOM_MENU_DIRECT_UP:DIVOOM_MENU_DIRECT_DOWN);                            
            lv_group_focus_obj(cur_obj);
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			if(cur_obj == phandle->select_btn[0])
			{
				divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_LOCAL_MUSIC_FOLDER_MENU, NULL, 0, 0);
			}
#if defined(DIVOOM_TIMES_VIBE)
			else if(cur_obj == phandle->select_btn[TF_MUSIC_MENU_IDX_REFRESH])
			{
				divoom_lvgl_tf_music_menu_start_refresh(phandle);
			}
#endif
			else if(cur_obj == phandle->select_btn[TF_MUSIC_MENU_IDX_NOW_PLAY])
			{
				divoom_lvgl_tf_music_menu_create_current_playing_setting(phandle);
			}
#if DIVOOM_TF_MUSIC_FAVORITES_UI
			else if(cur_obj == phandle->select_btn[2])
			{
			}
			else if(cur_obj == phandle->select_btn[3])
			{
				divoom_lvgl_tf_music_menu_create_playback_order_box(phandle);
			}
#else
			else if(cur_obj == phandle->select_btn[TF_MUSIC_MENU_IDX_PLAY_ORDER])
			{
				divoom_lvgl_tf_music_menu_create_playback_order_box(phandle);
			}
#endif
			//else if(cur_obj == phandle->select_btn[4])
			//{
			//	divoom_lvgl_tf_music_menu_create_audio_effects_setting(phandle);
			//}
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_menu_control_return_pre();
		}
		else
		{
			char *key_info = (char *)lv_event_get_param(e);
			divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
		}
    }
}

char *divoom_lvgl_tf_music_menu_get_music_name(char *file_name)
{
    return divoom_tf_music_get_disp_name(file_name);
}


/* 刷新底部栏和右侧列表的当前播放状态（定时器内或切歌后调用） */
static void divoom_lvgl_tf_music_menu_update_play_obj(Pdivoom_lvgl_tf_music_menu_ctrl phandle)
{
	int i;
	int cur_id;
	lv_obj_t *item_cont;

	if (phandle == NULL)
		return;
	
	phandle->pcur_play_music = divoom_tf_music_get_playing_music_info();
	phandle->last_play_music_id = divoom_tf_music_get_playing_music_id();

	if (phandle->play_name_label) 
	{
		if (phandle->pcur_play_music && phandle->pcur_play_music->music_name)
			lv_label_set_text(phandle->play_name_label, divoom_lvgl_tf_music_menu_get_music_name(phandle->pcur_play_music->music_name));
		else
			lv_label_set_text(phandle->play_name_label, divoom_menu_control_get_disp_str("UnknownMusicName"));
	}

	if (phandle->play_status_img)
	{
		if (divoom_tf_music_get_music_status())
			divoom_menu_control_lv_img_set_src(phandle->play_status_img, "34/34_pause.png");
		else
			divoom_menu_control_lv_img_set_src(phandle->play_status_img, "34/34_play.png");
	}

}

static void divoom_lvgl_tf_music_menu_update_select_disp_label(Pdivoom_lvgl_tf_music_menu_ctrl phandle)
{
#if defined(DIVOOM_TIMES_VIBE)
	if (phandle->select_label[TF_MUSIC_MENU_IDX_NOW_PLAY])
		lv_label_set_text_fmt(phandle->select_label[TF_MUSIC_MENU_IDX_NOW_PLAY], "%s",
				      divoom_tf_music_get_cur_play_root_name());
#else
	if (phandle->select_label[1])
		lv_label_set_text_fmt(phandle->select_label[1], "%s",
				      divoom_tf_music_get_cur_play_root_name());
#endif

#if DIVOOM_TF_MUSIC_FAVORITES_UI
	{
		int count = divoom_tf_music_get_like_music_count();
		lv_label_set_text_fmt(phandle->select_label[2], "%d", count);
	}
#endif

	{
#if defined(DIVOOM_TIMES_VIBE)
		void *playback_label = phandle->select_label[TF_MUSIC_MENU_IDX_PLAY_ORDER];
#elif DIVOOM_TF_MUSIC_FAVORITES_UI
		void *playback_label = phandle->select_label[3];
#else
		void *playback_label = phandle->select_label[2];
#endif

		if(playback_label)
		{
			if(phandle->psys_config->tf_music_mode == 0)
			{
				lv_label_set_text(playback_label, divoom_menu_control_get_disp_str("SequentialPlay"));
			}
			else if(phandle->psys_config->tf_music_mode == 1)
			{
				lv_label_set_text(playback_label, divoom_menu_control_get_disp_str("SinglePlay"));
			}
			else if(phandle->psys_config->tf_music_mode == 2)
			{
				lv_label_set_text(playback_label, divoom_menu_control_get_disp_str("RandomPlay"));
			}
		}
	}
}

static void divoom_lvgl_tf_music_menu_play_refresh_cb(lv_timer_t *timer)
{
	Pdivoom_lvgl_tf_music_menu_ctrl phandle = (Pdivoom_lvgl_tf_music_menu_ctrl)lv_timer_get_user_data(timer);
	int cur_id;

	if (phandle == NULL)
		return;

	cur_id = divoom_tf_music_get_playing_music_id();

	if (cur_id != phandle->last_play_music_id)
	{
		divoom_lvgl_tf_music_menu_update_play_obj(phandle);
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

	divoom_lvgl_tf_music_menu_update_select_disp_label(phandle);

#if defined(DIVOOM_TIMES_VIBE)
	divoom_tf_music_poll_songlist_refresh();
#endif
}


void divoom_lvgl_tf_music_menu_create_play_obj(Pdivoom_lvgl_tf_music_menu_ctrl phandle)
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
		lv_label_set_text_fmt(label_obj, "%s", divoom_lvgl_tf_music_menu_get_music_name(phandle->pcur_play_music->music_name));
   	}
	else
	{
		lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("UnknownMusicName"));
	}
	phandle->play_name_label = label_obj;

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



void *divoom_lvgl_tf_music_menu_init(void *user_data, int menu_id)
{
	int i, count;
    Pdivoom_lvgl_tf_music_menu_ctrl phandle;    
    void *slide_obj, *label_obj, *image_obj;
    Pdivoom_tf_music_item pitem;
    
    divoom_music_ctrl_set_mode(DIVOOM_MUSIC_CTRL_MODE_LOCAL);
    divoom_tf_music_sync_prepare();
    /* 冷启动：从配置恢复曲目到内存供 UI 显示，不向 JL 重发（已在播则跳过） */
    divoom_tf_music_restore_saved_playback();

    phandle = divoom_system_malloc(sizeof(divoom_lvgl_tf_music_menu_ctrl));
    memset(phandle, 0, sizeof(*phandle));

	phandle->psys_config = divoom_main_get_system_config();
	phandle->pcur_play_music = divoom_tf_music_get_playing_music_info();
	phandle->last_play_music_id = divoom_tf_music_get_playing_music_id();
	phandle->root_count = divoom_lvgl_tf_music_menu_load_hub(phandle->root_folders, DIVOOM_TF_MUSIC_MAX_ROOT);

    phandle->play_music_id = -1;
    phandle->menu_id = menu_id;
	phandle->pdirect_handle = divoom_menu_direct_ctrl_create(1, DIVOOM_TF_MUSIC_MENU_SETTING_COUNT);

	if(phandle->psys_config->tf_music_mode > 3)
	{
		phandle->psys_config->tf_music_mode = 0;
	}

    phandle->main_obj = lv_obj_create(NULL);
    divoom_menu_control_init_lable_style(&phandle->cout_style);
    lv_obj_add_style(phandle->main_obj, &phandle->cout_style, 0);
    lv_obj_set_pos(phandle->main_obj, 0, 0);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(0xE4E4E4), 0);
    lv_obj_set_scrollbar_mode(phandle->main_obj, LV_SCROLLBAR_MODE_OFF);
 
    label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 334-130, 69);
	lv_obj_set_size(label_obj, 400, 40);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("LocalMusic"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0x0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0); 
	
	lv_style_init(&phandle->normal_type);
	lv_style_set_bg_color(&phandle->normal_type, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->normal_type, 0);
	lv_style_set_border_width(&phandle->normal_type, 4);
	lv_style_set_border_color(&phandle->normal_type, lv_color_hex(0xE4E4E4));
	lv_style_set_radius(&phandle->normal_type, 17);
	lv_style_set_shadow_width(&phandle->normal_type, 0);
	lv_style_set_outline_width(&phandle->normal_type, 0);

	lv_style_init(&phandle->press_type);
	lv_style_set_bg_color(&phandle->press_type, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->press_type, 0);
	lv_style_set_border_width(&phandle->press_type, 4);
	lv_style_set_border_color(&phandle->press_type, lv_color_hex(0x6D6D6D));
	lv_style_set_radius(&phandle->press_type, 17);
	lv_style_set_shadow_width(&phandle->press_type, 0);
	lv_style_set_outline_width(&phandle->press_type, 0);

	for(i = 0;i < DIVOOM_TF_MUSIC_MENU_SETTING_COUNT; i++)
	{
		int item_kind = divoom_lvgl_tf_music_menu_setting_kind(i);
		/*lv_obj_t *obj = lv_obj_create(phandle->main_obj);
		lv_obj_add_style(obj, &phandle->obj_style, 0);
		lv_obj_set_size(obj, 720, 138);
		lv_obj_set_pos(obj, 40, 160+138*i);
		lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);*/
		
		lv_obj_t *btn = lv_btn_create(phandle->main_obj);
		lv_obj_set_size(btn, 752, 90);
		lv_obj_set_pos(btn, 24, 164+98*i);
		lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
		
		lv_obj_add_style(btn, &phandle->normal_type, 0);
		
		lv_obj_add_style(btn, &phandle->press_type, LV_STATE_FOCUSED);
		lv_obj_add_style(btn, &phandle->press_type, LV_STATE_FOCUS_KEY);
		lv_obj_set_style_pad_all(btn, 0, 0);
		//lv_obj_set_style_bg_color(btn, lv_color_hex(0x181818), 0);
		lv_obj_add_event_cb(btn, divoom_lvgl_tf_music_menu_set_event_cb, LV_EVENT_KEY, phandle);
		divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn, 0, i);
		phandle->select_btn[i] = btn;

		lv_obj_t *time_label = lv_label_create(btn);
		lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
		lv_obj_set_style_text_color(time_label, lv_color_black(), 0);
		lv_obj_set_size(time_label, 400, 34);
		lv_obj_set_pos(time_label, 32, 28);

		if(item_kind == 0)
		{
			phandle->cur_active_obj = btn;
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("FolderBrowsing"));
		}
#if defined(DIVOOM_TIMES_VIBE)
		else if(item_kind == 5)
		{
			lv_obj_t *flush_img;
			lv_obj_t *done_label;

			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("RefreshTfCardList"));

			flush_img = lv_img_create(btn);
			divoom_menu_control_lv_img_set_src(flush_img, "34/34_circle_flush.png");
			lv_obj_align(flush_img, LV_ALIGN_RIGHT_MID, -32, 0);
			phandle->refresh_flush_img = flush_img;

			done_label = lv_label_create(btn);
			lv_obj_set_style_text_font(done_label,
						   divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
			lv_obj_set_style_text_color(done_label, lv_color_hex(0x666666), 0);
			lv_obj_set_style_text_align(done_label, LV_TEXT_ALIGN_RIGHT, 0);
			lv_obj_align(done_label, LV_ALIGN_RIGHT_MID, -20, 0);
			lv_label_set_text(done_label, divoom_menu_control_get_disp_str("RefreshComplete"));
			lv_obj_add_flag(done_label, LV_OBJ_FLAG_HIDDEN);
			phandle->refresh_done_label = done_label;
			continue;
		}
#endif
		else if(item_kind == 1)
		{
			lv_obj_t *time_label2 = lv_label_create(btn);
			lv_obj_set_style_text_font(time_label2, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
			lv_obj_set_style_text_color(time_label2, lv_color_black(), 0);
			lv_obj_set_style_text_align(time_label2, LV_TEXT_ALIGN_RIGHT, 0);
			lv_obj_set_height(time_label2, 34);
			lv_obj_align(time_label2, LV_ALIGN_TOP_RIGHT, -76, 28);
			lv_label_set_long_mode(time_label2, LV_LABEL_LONG_DOT);
			lv_label_set_text_fmt(time_label2, "%s", divoom_tf_music_get_cur_play_root_name());
			phandle->select_label[i] = time_label2;

			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("NowPlaying"));
		}
#if DIVOOM_TF_MUSIC_FAVORITES_UI
		else if(item_kind == 2)
		{
			lv_obj_t *time_label2 = lv_label_create(btn);
			lv_obj_set_style_text_font(time_label2, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
			lv_obj_set_style_text_color(time_label2, lv_color_black(), 0);
			lv_obj_set_style_text_align(time_label2, LV_TEXT_ALIGN_RIGHT, 0);
			lv_obj_set_height(time_label2, 34);
			lv_obj_align(time_label2, LV_ALIGN_TOP_RIGHT, -76, 28);
			lv_label_set_long_mode(time_label2, LV_LABEL_LONG_SCROLL_CIRCULAR);
			count = divoom_tf_music_get_like_music_count();
			lv_label_set_text_fmt(time_label2, "%d", count);
			phandle->select_label[i] = time_label2;

			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("Favorites"));
		}
#endif
		else if(item_kind == 3)
		{
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("PlaybackOrder"));
			
			lv_obj_t *time_label2 = lv_label_create(btn);
			lv_obj_set_style_text_font(time_label2, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
			lv_obj_set_style_text_color(time_label2, lv_color_black(), 0);
			lv_obj_set_style_text_align(time_label2, LV_TEXT_ALIGN_RIGHT, 0);
			lv_obj_set_height(time_label2, 34);
			lv_obj_align(time_label2, LV_ALIGN_TOP_RIGHT, -76, 28);
			phandle->select_label[i] = time_label2;

			if(phandle->psys_config->tf_music_mode == 0)
			{
				lv_label_set_text(time_label2, divoom_menu_control_get_disp_str("SequentialPlay"));
			}
			else if(phandle->psys_config->tf_music_mode == 1)
			{
				lv_label_set_text(time_label2, divoom_menu_control_get_disp_str("SinglePlay"));
			}
			else if(phandle->psys_config->tf_music_mode == 2)
			{
				lv_label_set_text(time_label2, divoom_menu_control_get_disp_str("RandomPlay"));
			}
		}
		else if(item_kind == 4)
		{
			lv_label_set_text(time_label, divoom_menu_control_get_disp_str("AudioEffects"));

			lv_obj_t *time_label2 = lv_label_create(btn);
			lv_obj_set_style_text_font(time_label2, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
			lv_obj_set_style_text_color(time_label2, lv_color_black(), 0);
			lv_obj_set_style_text_align(time_label2, LV_TEXT_ALIGN_RIGHT, 0);
			lv_obj_set_height(time_label2, 34);
			lv_obj_align(time_label2, LV_ALIGN_TOP_RIGHT, -76, 28);

			if(phandle->psys_config->audio_effect == 0)
			{
				lv_label_set_text(time_label2, divoom_menu_control_get_disp_str("AudioEffect1"));
			}
			else if(phandle->psys_config->audio_effect == 1)
			{
				lv_label_set_text(time_label2, divoom_menu_control_get_disp_str("AudioEffect2"));
			}
			else if(phandle->psys_config->audio_effect == 2)
			{
				lv_label_set_text(time_label2, divoom_menu_control_get_disp_str("AudioEffect3"));
			}
			else if(phandle->psys_config->audio_effect == 3)
			{
				lv_label_set_text(time_label2, divoom_menu_control_get_disp_str("AudioEffect4"));
			}
			else if(phandle->psys_config->audio_effect == 4)
			{
				lv_label_set_text(time_label2, divoom_menu_control_get_disp_str("AudioEffect5"));
			}
			else if(phandle->psys_config->audio_effect == 5)
			{
				lv_label_set_text(time_label2, divoom_menu_control_get_disp_str("AudioEffect6"));
			}
			else if(phandle->psys_config->audio_effect == 6)
			{
				lv_label_set_text(time_label2, divoom_menu_control_get_disp_str("AudioEffect7"));
			}
			else if(phandle->psys_config->audio_effect == 7)
			{
				lv_label_set_text(time_label2, divoom_menu_control_get_disp_str("AudioEffect8"));
			}

			phandle->select_label[i] = time_label2;			

			//lv_obj_set_size(time_label2, 400, 34);
			//lv_obj_set_pos(time_label2, 32, 28);
		}

		lv_obj_t *right_img = lv_img_create(btn);
		divoom_menu_control_lv_img_set_src(right_img, "49/49_right.png");
		lv_obj_set_pos(right_img, 684, 27);
		
	}
	
#if DIVOOM_TF_MUSIC_FAVORITES_UI
	{
		lv_obj_t *time_label = lv_label_create(phandle->main_obj);
		lv_obj_set_style_text_font(time_label, divoom_menu_control_get_font_info(20, phandle->menu_id), 0);
		lv_obj_set_style_text_color(time_label, lv_color_hex(0x666666), 0);
		lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_CENTER, 0);
		lv_label_set_text(time_label, divoom_menu_control_get_disp_str("LocalMusicTip"));
		lv_obj_set_size(time_label, 720, 60);
		lv_obj_set_pos(time_label, 40, 690 - 90);
	}
#endif

	divoom_lvgl_tf_music_menu_create_play_obj(phandle);

	phandle->bg_cover_obj = lv_obj_create(phandle->main_obj);
	lv_obj_set_style_bg_color(phandle->bg_cover_obj, lv_color_hex(0xE4E4E4), 0);
	lv_obj_set_style_bg_opa(phandle->bg_cover_obj, 85*LV_OPA_100/100, 0);
	lv_obj_set_style_pad_all(phandle->bg_cover_obj, 0, 0);
	lv_obj_set_style_border_width(phandle->bg_cover_obj, 0, 0);
	lv_obj_set_style_outline_width(phandle->bg_cover_obj, 0, 0);
	lv_obj_set_size(phandle->bg_cover_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(phandle->bg_cover_obj, 0, 0);
	lv_obj_add_flag(phandle->bg_cover_obj, LV_OBJ_FLAG_HIDDEN);

    phandle->time_handle = lv_timer_create(divoom_lvgl_tf_music_menu_play_refresh_cb, 500, phandle);
    lv_timer_pause(phandle->time_handle);

#if defined(DIVOOM_TIMES_VIBE)
	phandle->refresh_ui_busy = 0;
	phandle->refresh_done_timer = NULL;
	divoom_tf_music_set_songlist_refresh_callback(divoom_lvgl_tf_music_menu_refresh_event_cb,
						      phandle);
#endif
    
    return phandle;
    
}

void divoom_lvgl_tf_music_menu_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_tf_music_menu_ctrl phandle = (Pdivoom_lvgl_tf_music_menu_ctrl)handle;
    
	if(phandle->select_btn[0])
	{
		lv_group_focus_obj(phandle->select_btn[0]);
	}

	if(active_obj)
	{
		lv_group_focus_obj(active_obj);
	}
	
	divoom_tf_music_sync_play_status_ui();
	divoom_lvgl_tf_music_menu_update_select_disp_label(phandle);
	divoom_lvgl_tf_music_menu_update_play_obj(phandle);
    lv_timer_resume(phandle->time_handle);
    
    lv_scr_load(phandle->main_obj);
    
    
}

void divoom_lvgl_tf_music_menu_pause(void *handle)
{
    Pdivoom_lvgl_tf_music_menu_ctrl phandle = (Pdivoom_lvgl_tf_music_menu_ctrl)handle;
    
    lv_timer_pause(phandle->time_handle);
}

void divoom_lvgl_tf_music_menu_exit(void *handle)
{
    Pdivoom_lvgl_tf_music_menu_ctrl phandle = (Pdivoom_lvgl_tf_music_menu_ctrl)handle;
    
#if defined(DIVOOM_TIMES_VIBE)
	divoom_tf_music_set_songlist_refresh_callback(NULL, NULL);
	divoom_lvgl_tf_music_menu_refresh_spin_stop(phandle);
	if (phandle->refresh_done_timer)
	{
		lv_timer_del(phandle->refresh_done_timer);
		phandle->refresh_done_timer = NULL;
	}
#endif

    lv_timer_del(phandle->time_handle);
    lv_obj_del(phandle->main_obj);
    lv_style_reset(&phandle->cout_style);
	lv_style_reset(&phandle->normal_type);
	lv_style_reset(&phandle->press_type);
    
    divoom_system_free(phandle);
}


#else
#define DIVOOM_LVGL_TF_MUSIC_LIST_VISIBLE_ROWS  12

typedef struct  _divoom_lvgl_tf_music_menu_list
{
    int   playing_index;
    int   active_index;
    int   total_items;
    int   first_index;
	lv_coord_t col_dsc[2];
    lv_coord_t row_dsc[DIVOOM_LVGL_TF_MUSIC_LIST_VISIBLE_ROWS + 1];
    void  *play_image_obj;
    void  *all_music_obj;
    void  *like_music_obj;
    void  *switch_image_obj;
    void  *list_cout;
    void  *back_obj;
    void  *grad_obj;
    void  *music_list_obj[DIVOOM_LVGL_TF_MUSIC_LIST_VISIBLE_ROWS];
    void  *title_list_obj[DIVOOM_LVGL_TF_MUSIC_LIST_VISIBLE_ROWS];
    void  *like_list_obj[DIVOOM_LVGL_TF_MUSIC_LIST_VISIBLE_ROWS];
    divoom_tf_entry page[DIVOOM_LVGL_TF_MUSIC_LIST_VISIBLE_ROWS];
    int   page_n;
}divoom_lvgl_tf_music_menu_list, *Pdivoom_lvgl_tf_music_menu_list;

typedef struct  _divoom_lvgl_tf_music_menu_ctrl
{
    int menu_id;
    int  play_music_id;
    int  play_status;
    int  bg_index;
    int  play_mode;
    int  play_order;
    char music_name[256];
    char music_title[256];
    void *main_obj;
    void *back_obj;
    void  *music_album_img_obj;
    void *volume_obj;
    void *mode_obj;
    void *more_obj;
    void *pre_obj;
    void *next_obj;
    void *play_obj;
    void *like_obj;
    void *lyric_obj;
    void *music_title_obj;
    void *time_handle;
    lv_anim_t image_anim;    //底图旋转动画
    divoom_lvgl_tf_music_menu_list plist_menu;
    lv_style_t cout_style;
    lv_style_t grid_type;
}divoom_lvgl_tf_music_menu_ctrl, *Pdivoom_lvgl_tf_music_menu_ctrl;


const char *divoom_lvgl_tf_music_mode_image_name[] = 
{
    "34/34_loop.png",
    "34/34_random.png",
    "34/34_single.png",
};

void divoom_lvgl_tf_music_remove_list_menu(Pdivoom_lvgl_tf_music_menu_ctrl phandle);
void divoom_lvgl_tf_music_list_menu_reload_grap(Pdivoom_lvgl_tf_music_menu_ctrl phandle);

static void divoom_lvgl_tf_music_list_menu_event_cb(lv_event_t * e)
{
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_tf_music_menu_ctrl phandle = (Pdivoom_lvgl_tf_music_menu_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_CLICKED)
    {
        
        if(cur_obj == phandle->plist_menu.back_obj)
        {
            divoom_lvgl_tf_music_remove_list_menu(phandle);
        }
        else if(cur_obj == phandle->plist_menu.all_music_obj)
        {
            if(phandle->plist_menu.active_index == 1)
            {
                phandle->plist_menu.active_index = 0;
                
                divoom_lvgl_tf_music_list_menu_reload_grap(phandle);
            }
        }
        else if(cur_obj == phandle->plist_menu.like_music_obj)
        {
            if(phandle->plist_menu.active_index == 0)
            {
                phandle->plist_menu.active_index = 1;
                
                divoom_lvgl_tf_music_list_menu_reload_grap(phandle);
            }
        }
        else
        {
            for(int i = 0; i < phandle->plist_menu.page_n; i++)
            {
                int track_id;

                track_id = phandle->plist_menu.page[i].id;
                if(track_id <= 0 || phandle->plist_menu.page[i].is_dir)
                {
                    continue;
                }
                if(cur_obj == phandle->plist_menu.like_list_obj[i])
                {
                    divoom_tf_music_set_like_flag(track_id, 1);
                    if(phandle->play_music_id == track_id)
                    {
                        divoom_menu_control_lv_img_set_src(phandle->like_obj, "34/34_like.png");
                    }
                    divoom_menu_control_lv_img_set_src(phandle->plist_menu.like_list_obj[i], "34/34_like.png");
                    break;
                }
                else if(cur_obj == phandle->plist_menu.music_list_obj[i])
                {
                    divoom_tf_music_set_playing_music_id(phandle->plist_menu.active_index, track_id);
                    phandle->play_order = phandle->plist_menu.active_index;
                    if(phandle->plist_menu.playing_index != -1
                       && phandle->plist_menu.playing_index < DIVOOM_LVGL_TF_MUSIC_LIST_VISIBLE_ROWS)
                    {
                        lv_obj_set_style_text_color(phandle->plist_menu.title_list_obj[phandle->plist_menu.playing_index], lv_color_hex(0xFFFFFF), 0);
                    }
                    phandle->plist_menu.playing_index = i;
                    lv_obj_set_style_text_color(phandle->plist_menu.title_list_obj[phandle->plist_menu.playing_index], lv_color_hex(0xED5050), 0);
                    if(phandle->plist_menu.play_image_obj)
                    {
                        lv_obj_del(phandle->plist_menu.play_image_obj);
                    }
                    void *image_obj = lv_img_create(phandle->plist_menu.music_list_obj[i]);
                    lv_obj_set_pos(image_obj, 10, 15);
                    lv_obj_set_size(image_obj, 60, 60);
                    divoom_menu_control_lv_img_set_src(image_obj, "34/34_playing.png");
                    phandle->plist_menu.play_image_obj = image_obj;
                    break;
                }
            }
        }
    }
}

char *divoom_lvgl_tf_music_list_menu_get_music_name(char *file_name)
{
    return divoom_tf_music_get_disp_name(file_name);
}

void divoom_lvgl_tf_music_list_menu_reload_grap(Pdivoom_lvgl_tf_music_menu_ctrl phandle)
{
    int i, j;
    int vis;
    void *label_obj, *image_obj;
    Pdivoom_lvgl_tf_music_menu_list plist_menu_handle = &phandle->plist_menu ;

    if(plist_menu_handle->play_image_obj)
    {
        lv_obj_del(plist_menu_handle->play_image_obj);
        plist_menu_handle->play_image_obj = NULL;
    }
    
    
    if(phandle->plist_menu.active_index)
    {
        lv_obj_set_style_text_color(plist_menu_handle->like_music_obj, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_color(plist_menu_handle->all_music_obj, lv_color_hex(0x797979), 0);
        divoom_menu_control_lv_img_set_src(plist_menu_handle->switch_image_obj, "34/34_like_music.png");
    }
    else
    {
        lv_obj_set_style_text_color(plist_menu_handle->like_music_obj, lv_color_hex(0x797979), 0);
        lv_obj_set_style_text_color(plist_menu_handle->all_music_obj, lv_color_hex(0xFFFFFF), 0);
        divoom_menu_control_lv_img_set_src(plist_menu_handle->switch_image_obj, "34/34_all_music.png");
    }

    memset(plist_menu_handle->page, 0, sizeof(plist_menu_handle->page));
    plist_menu_handle->page_n = 0;
    plist_menu_handle->first_index = 0;
    if(phandle->plist_menu.active_index == 0 && !divoom_database_tf_is_rebuilding())
    {
        plist_menu_handle->total_items = divoom_database_tf_subtree_count(0);
        plist_menu_handle->page_n = divoom_database_tf_subtree_page(0, 0,
            DIVOOM_LVGL_TF_MUSIC_LIST_VISIBLE_ROWS, plist_menu_handle->page,
            DIVOOM_LVGL_TF_MUSIC_LIST_VISIBLE_ROWS);
    }
    else
    {
        plist_menu_handle->total_items = 0;
    }
    divoom_print("music info: %d, %d\n", plist_menu_handle->total_items, phandle->plist_menu.active_index);
    vis = plist_menu_handle->page_n;

	plist_menu_handle->col_dsc[0] = DIVOOM_SCREEN_WIDTH;
    plist_menu_handle->col_dsc[1] = LV_GRID_TEMPLATE_LAST;
    if(plist_menu_handle->grad_obj)
    {
        lv_obj_del(plist_menu_handle->grad_obj);
    }
    
    

    lv_style_init(&phandle->grid_type);
    lv_style_set_pad_all(&phandle->grid_type, 0);
    lv_style_set_pad_gap(&phandle->grid_type, 0);
    lv_style_set_border_width(&phandle->grid_type, 0);
    lv_style_set_border_side(&phandle->grid_type, LV_BORDER_SIDE_NONE);
    lv_style_set_bg_color(&phandle->grid_type, lv_color_hex(DIVOOM_MENNU_BACK_GROUD_COLOR));
	lv_obj_t * cont = lv_obj_create(phandle->plist_menu.list_cout);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0), 0);
    lv_obj_add_style(cont, &phandle->grid_type, 0);
    lv_obj_set_pos(cont, 0, 172);
    lv_obj_set_size(cont, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH - 172);
	lv_obj_set_style_pad_all(cont, 0, 0);
	lv_obj_set_style_border_width(cont, 0, 0);
	lv_obj_set_style_border_opa(cont, LV_OPA_0, 0);	
	lv_obj_set_style_bg_opa(cont, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cont, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
	lv_obj_set_style_shadow_width(cont, 0, 0); //去掉边框底部的阴影
	lv_obj_set_style_base_dir(cont, LV_BASE_DIR_AUTO, 0);
    plist_menu_handle->grad_obj = cont; 

    plist_menu_handle->col_dsc[0] = 800;
    plist_menu_handle->col_dsc[1] = LV_GRID_TEMPLATE_LAST;
    for(i = 0; i < vis; i++)
    {
        plist_menu_handle->row_dsc[i] = 100;
    }
    
	plist_menu_handle->row_dsc[i] = LV_GRID_TEMPLATE_LAST;
	if(vis > 0)
	{
		lv_obj_set_grid_dsc_array(plist_menu_handle->grad_obj, plist_menu_handle->col_dsc, plist_menu_handle->row_dsc);
	}
    plist_menu_handle->playing_index = -1;
    
	for(j = 0; j < vis; j ++)
    {
        
        lv_obj_t * obj = lv_obj_create(cont);
        lv_obj_add_style(obj, &phandle->cout_style, 0);
		plist_menu_handle->music_list_obj[j] = obj;
        lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_CENTER, 0, 1,
                             LV_GRID_ALIGN_STRETCH, j, 1);
        lv_obj_set_style_bg_color(obj, lv_color_hex(0), 0);
							 
		lv_obj_set_size(obj, DIVOOM_SCREEN_WIDTH, 90);
        lv_obj_center(obj);
        
        
        label_obj = lv_label_create(obj);
        lv_obj_set_pos(label_obj, 10, 30);
        lv_obj_set_size(label_obj, 60, 30);
        lv_label_set_text_fmt(label_obj, "%d", j + 1);
        lv_obj_set_style_text_color(label_obj, lv_color_hex(0x6D6D6D), 0);
        lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(24, phandle->menu_id), 0); 
        

        label_obj = lv_label_create(obj);
        lv_obj_set_pos(label_obj, 80, 28);
        lv_obj_set_size(label_obj, 620, 34);
        lv_label_set_text(label_obj, divoom_lvgl_tf_music_list_menu_get_music_name(plist_menu_handle->page[j].name));
        if(plist_menu_handle->page[j].id == phandle->play_music_id)
        {
            plist_menu_handle->playing_index = j;
            lv_obj_set_style_text_color(label_obj, lv_color_hex(0xED5050), 0);
            image_obj = lv_img_create(obj);
            lv_obj_set_pos(image_obj, 10, 15);
            lv_obj_set_size(image_obj, 60, 60);
            divoom_menu_control_lv_img_set_src(image_obj, "34/34_playing.png");
            plist_menu_handle->play_image_obj = image_obj;
        }
        else
        {
            lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);
        }
        
        lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(24, phandle->menu_id), 0); 
		plist_menu_handle->title_list_obj[j] = label_obj;
        
        
        
        image_obj = lv_img_create(obj);
        lv_obj_set_pos(image_obj, 710, 15);
        lv_obj_set_size(image_obj, 60, 60);
        lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);   
        divoom_menu_control_lv_img_set_src(image_obj, "34/34_unlike.png");
        lv_obj_add_event(image_obj, divoom_lvgl_tf_music_list_menu_event_cb, LV_EVENT_CLICKED, phandle);
		plist_menu_handle->like_list_obj[j] = image_obj;
        

		lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
		lv_obj_add_event_cb(obj, divoom_lvgl_tf_music_list_menu_event_cb, LV_EVENT_CLICKED, phandle);
	}
    
    
}

void divoom_lvgl_tf_music_add_list_menu(Pdivoom_lvgl_tf_music_menu_ctrl phandle)
{
    void *label_obj;
    Pdivoom_lvgl_tf_music_menu_list plist_menu_handle = &phandle->plist_menu ;
    
    
    if(phandle->plist_menu.list_cout)
    {
        
        return;
    }
    plist_menu_handle->play_image_obj = NULL;
    phandle->plist_menu.active_index = phandle->play_order;
    phandle->plist_menu.list_cout = lv_obj_create(phandle->main_obj);
    lv_obj_add_style(phandle->plist_menu.list_cout, &phandle->cout_style, 0);
    lv_obj_set_pos(phandle->plist_menu.list_cout, 0, 0);
	lv_obj_set_size(phandle->plist_menu.list_cout, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_style_bg_color(phandle->plist_menu.list_cout, lv_color_hex(0), 0);

    plist_menu_handle->back_obj = lv_img_create(phandle->plist_menu.list_cout);
	lv_obj_set_pos(plist_menu_handle->back_obj, 10, 51);
    lv_obj_set_size(plist_menu_handle->back_obj, 72, 72);
    lv_obj_add_flag(plist_menu_handle->back_obj, LV_OBJ_FLAG_CLICKABLE);   
    divoom_menu_control_lv_img_set_src(plist_menu_handle->back_obj, "34/34_back.png");
    lv_obj_add_event(plist_menu_handle->back_obj, divoom_lvgl_tf_music_list_menu_event_cb, LV_EVENT_CLICKED, phandle);
    
    plist_menu_handle->switch_image_obj = lv_img_create(phandle->plist_menu.list_cout);
	lv_obj_set_pos(plist_menu_handle->switch_image_obj, 110, 50);
    lv_obj_set_size(plist_menu_handle->switch_image_obj, 580, 72);
    if(phandle->plist_menu.active_index)
    {
        divoom_menu_control_lv_img_set_src(plist_menu_handle->switch_image_obj, "34/34_like_music.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(plist_menu_handle->switch_image_obj, "34/34_all_music.png");
    }

    
    label_obj = lv_label_create(phandle->plist_menu.list_cout);
    lv_obj_set_pos(label_obj, 170, 59);
    lv_obj_set_size(label_obj, 170, 56);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("List"));
    if(phandle->plist_menu.active_index)
    {
        lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);
    }
    else
    {
        lv_obj_set_style_text_color(label_obj, lv_color_hex(0x797979), 0);
    }
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(28, phandle->menu_id), 0); 
    lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);   
    lv_obj_add_event(label_obj, divoom_lvgl_tf_music_list_menu_event_cb, LV_EVENT_CLICKED, phandle);
    plist_menu_handle->all_music_obj = label_obj;

    label_obj = lv_label_create(phandle->plist_menu.list_cout);
    lv_obj_set_pos(label_obj, 460, 59);
    lv_obj_set_size(label_obj, 170, 56);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("Favorites"));
    if(phandle->plist_menu.active_index)
    {
        lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);
    }
    else
    {
        lv_obj_set_style_text_color(label_obj, lv_color_hex(0x797979), 0);
    }
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(28, phandle->menu_id), 0); 
    lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);   
    lv_obj_add_event(label_obj, divoom_lvgl_tf_music_list_menu_event_cb, LV_EVENT_CLICKED, phandle);
    plist_menu_handle->like_music_obj = label_obj;

    
    
    divoom_lvgl_tf_music_list_menu_reload_grap(phandle);

    
}

void divoom_lvgl_tf_music_remove_list_menu(Pdivoom_lvgl_tf_music_menu_ctrl phandle)
{
    Pdivoom_lvgl_tf_music_menu_list plist_menu_handle = &phandle->plist_menu ;

    

    lv_obj_del(phandle->plist_menu.list_cout);
    phandle->plist_menu.list_cout = NULL;
    phandle->plist_menu.grad_obj = NULL;
    plist_menu_handle->play_image_obj = NULL;
    memset(plist_menu_handle->page, 0, sizeof(plist_menu_handle->page));
    plist_menu_handle->page_n = 0;
    plist_menu_handle->total_items = 0;
}

static void divoom_lvgl_tf_music_main_event_cb(lv_event_t * e)
{
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_tf_music_menu_ctrl phandle = (Pdivoom_lvgl_tf_music_menu_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_CLICKED)
    {
        
        if(cur_obj == phandle->back_obj)
        {
            divoom_menu_control_return_pre();
            return;
        }
        else if(cur_obj == phandle->music_album_img_obj)
        {
            char image_name[64];
            if(phandle->bg_index < 6)
            {
                phandle->bg_index ++;
            }
            else
            {
                phandle->bg_index = 0;
            }
            divoom_main_set_tf_music_bg(phandle->bg_index);
            sprintf(image_name, "34/34_cd%d.png", phandle->bg_index + 1);
            divoom_menu_control_lv_img_set_src(phandle->music_album_img_obj, image_name);
        }
        else if(cur_obj == phandle->mode_obj)
        {
            if(phandle->play_mode < 2)
            {
                phandle->play_mode ++;
            }
            else
            {
                phandle->play_mode = 0;
            }
            divoom_main_set_tf_music_mode(phandle->play_mode);
            divoom_menu_control_lv_img_set_src(phandle->mode_obj, divoom_lvgl_tf_music_mode_image_name[phandle->play_mode]);
        }
        else if(cur_obj == phandle->like_obj)
        {
            Pdivoom_tf_music_item pitem = divoom_tf_music_get_playing_music_info();

            if(pitem->like)
            {
                divoom_menu_control_lv_img_set_src(cur_obj, "34/34_unlike.png");
            }
            else
            {
                divoom_menu_control_lv_img_set_src(cur_obj, "34/34_like.png");
            }  
            divoom_tf_music_set_like_flag(pitem->id, pitem->like?0:1);
        }
        else if(cur_obj == phandle->more_obj)
        {
            
            divoom_lvgl_tf_music_add_list_menu(phandle);            
        }
        else if(cur_obj == phandle->pre_obj)
        {
            divoom_music_ctrl_play_pre();
        }
        else if(cur_obj == phandle->next_obj)
        {
            divoom_music_ctrl_play_next();
        }
        else if(cur_obj == phandle->play_obj)
        {
            phandle->play_status = phandle->play_status?0:1;
            divoom_music_ctrl_set_play(phandle->play_status);
            if(phandle->play_status)
            {
                divoom_menu_control_lv_img_set_src(phandle->play_obj, "34/34_pause.png");
            }
            else
            {
                divoom_menu_control_lv_img_set_src(phandle->play_obj, "34/34_play.png");
            }
        }
        else if(cur_obj == phandle->volume_obj)
        {
            /* TF：本机数字音量；Soft Volume Master 保持最大，由 divoom_main_init_sys_vol / 切模式时保证 */
            int volume = lv_slider_get_value(cur_obj);
            divoom_tf_music_set_volume((uint8)volume);
        }
    }
}

void divoom_lvgl_tf_music_menu_time_loop(struct _lv_timer_t * ptime_info)
{
    Pdivoom_lvgl_tf_music_menu_ctrl phandle = (Pdivoom_lvgl_tf_music_menu_ctrl)lv_timer_get_user_data(ptime_info);
    int cur_play_status = divoom_tf_music_get_music_status();
    Pdivoom_tf_music_item pitem = divoom_tf_music_get_playing_music_info();
    
    if(cur_play_status != phandle->play_status)
    {
        phandle->play_status = cur_play_status;
        if(phandle->play_status)
        {
            divoom_menu_control_lv_img_set_src(phandle->play_obj, "33/33_pause.png");
        }
        else
        {
            divoom_menu_control_lv_img_set_src(phandle->play_obj, "33/33_play.png");
        }
    }
    if(pitem->id != phandle->play_music_id)
    {
        phandle->play_music_id = pitem->id;
        if(pitem && pitem->music_name)
        {
            lv_label_set_text(phandle->music_title_obj, divoom_lvgl_tf_music_list_menu_get_music_name(pitem->music_name));
        }
        if(pitem->like)
        {
            divoom_menu_control_lv_img_set_src(phandle->like_obj, "34/34_like.png");
        }
        else
        {
            divoom_menu_control_lv_img_set_src(phandle->like_obj, "34/34_unlike.png");
        }
    }
    
}

void divoom_lvgl_tf_music_menu_bg_anim(void * a, int32_t v)
{
    lv_img_set_angle(a, 10*v);
}


void *divoom_lvgl_tf_music_menu_init(void *user_data, int menu_id)
{    
    Pdivoom_lvgl_tf_music_menu_ctrl phandle;    
    void *slide_obj, *label_obj, *image_obj;
    Pdivoom_tf_music_item pitem;
    
    divoom_music_ctrl_set_mode(DIVOOM_MUSIC_CTRL_MODE_LOCAL);
    /* 音量由 divoom_tf_music_set_volume / divoom_main_set_sys_vol（ALSA softvol）统一设置 */

    phandle = divoom_system_malloc(sizeof(divoom_lvgl_tf_music_menu_ctrl));
    phandle->play_music_id = -1;
    phandle->menu_id = menu_id;
    phandle->main_obj = lv_obj_create(NULL);
    divoom_menu_control_init_lable_style(&phandle->cout_style);
    lv_obj_add_style(phandle->main_obj, &phandle->cout_style, 0);
    lv_obj_set_pos(phandle->main_obj, 0, 0);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(0), 0);
    lv_obj_set_scrollbar_mode(phandle->main_obj, LV_SCROLLBAR_MODE_OFF);
 
    
    phandle->back_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(phandle->back_obj, 30, 30);
    lv_obj_set_size(phandle->back_obj, 72, 72);
    lv_obj_add_flag(phandle->back_obj, LV_OBJ_FLAG_CLICKABLE);   
    divoom_menu_control_lv_img_set_src(phandle->back_obj, "34/34_back.png");
    lv_obj_add_event(phandle->back_obj, divoom_lvgl_tf_music_main_event_cb, LV_EVENT_CLICKED, phandle);
    
    
    label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 160, 38);
	lv_obj_set_size(label_obj, 480, 56);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("LocalMusic"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(40, phandle->menu_id), 0); 
    

    phandle->bg_index = divoom_main_set_tf_music_bg(-1);
    if(phandle->bg_index >= 7)
    {
        phandle->play_mode = 0;
    }
    sprintf(phandle->music_name, "34/34_cd%d.png", phandle->bg_index + 1);
    phandle->music_album_img_obj = lv_img_create(phandle->main_obj);
    lv_obj_set_pos(phandle->music_album_img_obj, 90, 131);
	lv_obj_set_size(phandle->music_album_img_obj, 620, 620);
    divoom_menu_control_lv_img_set_src(phandle->music_album_img_obj, phandle->music_name);
    lv_obj_add_flag(phandle->music_album_img_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(phandle->music_album_img_obj, divoom_lvgl_tf_music_main_event_cb, LV_EVENT_CLICKED, phandle);
    

    lv_anim_init(&phandle->image_anim);
    lv_anim_set_var(&phandle->image_anim, phandle->music_album_img_obj);
    lv_anim_path_linear(&phandle->image_anim);
    lv_anim_set_path_cb(&phandle->image_anim, lv_anim_path_linear);
    lv_anim_set_exec_cb(&phandle->image_anim, (lv_anim_exec_xcb_t) divoom_lvgl_tf_music_menu_bg_anim);
    lv_anim_set_time(&phandle->image_anim, 3600);
    lv_anim_set_values(&phandle->image_anim, 0, 360);
    lv_anim_set_repeat_count(&phandle->image_anim, LV_ANIM_REPEAT_INFINITE); 
    lv_anim_start(&phandle->image_anim); 

    pitem = divoom_tf_music_get_playing_music_info();

    image_obj = lv_img_create(phandle->main_obj);
    lv_obj_set_pos(image_obj, 650, 837);
	lv_obj_set_size(image_obj, 60, 60);
    if(pitem && pitem->like)
    {
        divoom_menu_control_lv_img_set_src(image_obj, "34/34_like.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(image_obj, "34/34_unlike.png");
    }
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_tf_music_main_event_cb, LV_EVENT_CLICKED, phandle);
    phandle->like_obj = image_obj;
    
    

    image_obj = lv_img_create(phandle->main_obj);
    lv_obj_set_pos(image_obj, 81, 968);
	lv_obj_set_size(image_obj, 48, 48);
    divoom_menu_control_lv_img_set_src(image_obj, "34/34_vol1.png");
    
    image_obj = lv_img_create(phandle->main_obj);
    lv_obj_set_pos(image_obj, 671, 968);
	lv_obj_set_size(image_obj, 48, 48);
    divoom_menu_control_lv_img_set_src(image_obj, "34/34_vol2.png");

    slide_obj = lv_slider_create(phandle->main_obj);
    lv_obj_set_style_opa(slide_obj, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_slider_set_range(slide_obj, 0, 100);
    lv_obj_set_pos(slide_obj, 153, 984);
    lv_obj_set_size(slide_obj, 494, 16);
    lv_slider_set_value(slide_obj, divoom_tf_music_get_volume(), LV_ANIM_ON);
    lv_obj_set_style_bg_color(slide_obj, lv_color_hex(DIVOOM_MENNU_SLIDER_ACTIVE_COLOR), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slide_obj, lv_color_hex(DIVOOM_MENNU_SLIDER_UNACTIVE_COLOR), LV_PART_MAIN);
    lv_obj_add_flag(slide_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(slide_obj, divoom_lvgl_tf_music_main_event_cb, LV_EVENT_CLICKED, phandle);
    phandle->volume_obj = slide_obj;
    
    
    phandle->play_mode = divoom_main_set_tf_music_mode(-1);
    if(phandle->play_mode >= 3)
    {
        phandle->play_mode = 0;
    }
    phandle->mode_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(phandle->mode_obj, 67, 1114);
    lv_obj_set_size(phandle->mode_obj, 90, 90);
    lv_obj_add_flag(phandle->mode_obj, LV_OBJ_FLAG_CLICKABLE);   
    divoom_menu_control_lv_img_set_src(phandle->mode_obj, divoom_lvgl_tf_music_mode_image_name[phandle->play_mode]);
    lv_obj_add_event(phandle->mode_obj, divoom_lvgl_tf_music_main_event_cb, LV_EVENT_CLICKED, phandle);
    
    

    phandle->pre_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(phandle->pre_obj, 209, 1114);
    lv_obj_set_size(phandle->pre_obj, 90, 90);
    lv_obj_add_flag(phandle->pre_obj, LV_OBJ_FLAG_CLICKABLE);   
    divoom_menu_control_lv_img_set_src(phandle->pre_obj, "34/34_pre.png");
    lv_obj_add_event(phandle->pre_obj, divoom_lvgl_tf_music_main_event_cb, LV_EVENT_CLICKED, phandle);
    
    phandle->next_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(phandle->next_obj, 501, 1114);
    lv_obj_set_size(phandle->next_obj, 90, 90);
    lv_obj_add_flag(phandle->next_obj, LV_OBJ_FLAG_CLICKABLE);   
    divoom_menu_control_lv_img_set_src(phandle->next_obj, "34/34_next.png");
    lv_obj_add_event(phandle->next_obj, divoom_lvgl_tf_music_main_event_cb, LV_EVENT_CLICKED, phandle);
    

    phandle->play_status = divoom_tf_music_get_music_status();
    phandle->play_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(phandle->play_obj, 355, 1114);
    lv_obj_set_size(phandle->play_obj, 90, 90);
    lv_obj_add_flag(phandle->play_obj, LV_OBJ_FLAG_CLICKABLE);   
    if(phandle->play_status)
    {
        divoom_menu_control_lv_img_set_src(phandle->play_obj, "34/34_pause.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(phandle->play_obj, "34/34_play.png");
    }
    lv_obj_add_event(phandle->play_obj, divoom_lvgl_tf_music_main_event_cb, LV_EVENT_CLICKED, phandle);

    phandle->more_obj = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(phandle->more_obj, 643, 1114);
    lv_obj_set_size(phandle->more_obj, 90, 90);
    lv_obj_add_flag(phandle->more_obj, LV_OBJ_FLAG_CLICKABLE);   
    divoom_menu_control_lv_img_set_src(phandle->more_obj, "34/34_more.png");
    lv_obj_add_event(phandle->more_obj, divoom_lvgl_tf_music_main_event_cb, LV_EVENT_CLICKED, phandle);
    


    label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, 90, 817);
	lv_obj_set_size(label_obj, 560, 100);
    if(pitem && pitem->music_name)
    {
        lv_label_set_text(label_obj, divoom_lvgl_tf_music_list_menu_get_music_name(pitem->music_name));
        phandle->play_music_id = pitem->id;
    }
    else
    {
        lv_label_set_text(label_obj, "");
    }
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xABABAB), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(34, phandle->menu_id), 0); 
    phandle->music_title_obj = label_obj; 
    

    phandle->time_handle = lv_timer_create(divoom_lvgl_tf_music_menu_time_loop, 1000, phandle);
    lv_timer_pause(phandle->time_handle);
    
    
    return  phandle;
    
}

void divoom_lvgl_tf_music_menu_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_tf_music_menu_ctrl phandle = (Pdivoom_lvgl_tf_music_menu_ctrl)handle;
    

    lv_timer_resume(phandle->time_handle);
    
    lv_scr_load(phandle->main_obj);
    
    
}

void divoom_lvgl_tf_music_menu_pause(void *handle)
{
    Pdivoom_lvgl_tf_music_menu_ctrl phandle = (Pdivoom_lvgl_tf_music_menu_ctrl)handle;
    
    lv_timer_pause(phandle->time_handle);
}

void divoom_lvgl_tf_music_menu_exit(void *handle)
{
    Pdivoom_lvgl_tf_music_menu_ctrl phandle = (Pdivoom_lvgl_tf_music_menu_ctrl)handle;
    
    lv_anim_del(phandle->music_album_img_obj, NULL);
    lv_timer_del(phandle->time_handle);
    lv_obj_del(phandle->main_obj);
    lv_style_reset(&phandle->cout_style);
    lv_style_reset(&phandle->grid_type);
    
    divoom_system_free(phandle);
}
#endif

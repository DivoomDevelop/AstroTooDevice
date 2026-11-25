#include "divoom.h"
#include "divoom_menu_control.h"
#include "divoom_music_ctrl.h"
#include "divoom_lvgl_top_music_ctrl_hint.h"
#include "divoom_lvgl_top_control.h"

#ifdef DIVOOM_TIME_FRAME_SPEAKER

/* MasterGo 549:60027 顶部音乐控制提示 */
#define MC_HINT_AUTO_MS     3000
#define MC_HINT_PILL_X      160
#define MC_HINT_PILL_Y      18
#define MC_HINT_PILL_W      480
#define MC_HINT_PILL_H      100
#define MC_HINT_PRE_X       248
#define MC_HINT_CENTER_X    376
#define MC_HINT_NEXT_X      504
#define MC_HINT_ICON_Y      44
#define MC_HINT_ICON_SZ     48

typedef enum {
	MC_HINT_SLOT_PRE = 0,
	MC_HINT_SLOT_CENTER,
	MC_HINT_SLOT_NEXT,
	MC_HINT_SLOT_NUM,
} mc_hint_slot_t;

typedef struct _divoom_lvgl_top_music_ctrl_hint_ctrl {
	void *main_obj;
	void *cout_obj;
	lv_obj_t *pill_obj;
	lv_obj_t *icon_img[MC_HINT_SLOT_NUM];
	char icon_name[MC_HINT_SLOT_NUM][24];
	lv_style_t pill_style;
	lv_timer_t *hide_timer;
} divoom_lvgl_top_music_ctrl_hint_ctrl, *Pdivoom_lvgl_top_music_ctrl_hint_ctrl;

static Pdivoom_lvgl_top_music_ctrl_hint_ctrl pdivoom_lvgl_top_music_ctrl_hint_handle = NULL;

static void divoom_lvgl_top_music_ctrl_hint_del_menu(void);

static int divoom_lvgl_top_music_ctrl_hint_is_music_key(uint8 key_value)
{
	return key_value == DIVOOM_KEY_MUSIC_PREV ||
	       key_value == DIVOOM_KEY_MUSIC_NEXT ||
	       key_value == DIVOOM_KEY_MUSIC_PLAY;
}

static void divoom_lvgl_top_music_ctrl_hint_set_icon(lv_obj_t *img, const char *name, char *cached_name, int name_len)
{
	char path[48];

	if (cached_name && strncmp(cached_name, name, name_len) == 0)
		return;

	snprintf(path, sizeof(path), "1/1_%s.png", name);
	divoom_menu_control_lv_img_set_src(img, path);
	if (cached_name)
	{
		strncpy(cached_name, name, name_len - 1);
		cached_name[name_len - 1] = '\0';
	}
}

static void divoom_lvgl_top_music_ctrl_hint_apply_icons(uint8 key_value, uint8 playing)
{
	Pdivoom_lvgl_top_music_ctrl_hint_ctrl ph;
	const char *pre_name;
	const char *center_name;
	const char *next_name;

	ph = pdivoom_lvgl_top_music_ctrl_hint_handle;
	if (!ph)
		return;

	if (key_value == DIVOOM_KEY_MUSIC_PREV)
	{
		pre_name = "pre_active";
		center_name = playing ? "play_no_active" : "pause_no_active";
		next_name = "next_no_active";
	}
	else if (key_value == DIVOOM_KEY_MUSIC_NEXT)
	{
		pre_name = "pre_no_active";
		center_name = playing ? "play_no_active" : "pause_no_active";
		next_name = "next_active";
	}
	else if (key_value == DIVOOM_KEY_MUSIC_PLAY)
	{
		pre_name = "pre_no_active";
		center_name = playing ? "pause_active" : "play_active";
		next_name = "next_no_active";
	}
	else
	{
		pre_name = "pre_no_active";
		center_name = playing ? "pause_no_active" : "play_no_active";
		next_name = "next_no_active";
	}

	divoom_lvgl_top_music_ctrl_hint_set_icon(ph->icon_img[MC_HINT_SLOT_PRE], pre_name,
		ph->icon_name[MC_HINT_SLOT_PRE], sizeof(ph->icon_name[MC_HINT_SLOT_PRE]));
	divoom_lvgl_top_music_ctrl_hint_set_icon(ph->icon_img[MC_HINT_SLOT_CENTER], center_name,
		ph->icon_name[MC_HINT_SLOT_CENTER], sizeof(ph->icon_name[MC_HINT_SLOT_CENTER]));
	divoom_lvgl_top_music_ctrl_hint_set_icon(ph->icon_img[MC_HINT_SLOT_NEXT], next_name,
		ph->icon_name[MC_HINT_SLOT_NEXT], sizeof(ph->icon_name[MC_HINT_SLOT_NEXT]));
}

static void divoom_lvgl_top_music_ctrl_hint_event_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code != LV_EVENT_KEY)
		return;

	if (pdivoom_lvgl_top_music_ctrl_hint_handle == NULL)
		return;

	{
		char *key_info = (char *)lv_event_get_param(e);
		uint8 key_value = key_info[0];

		/* 3s 内连续按音乐键：只换图标，不销毁胶囊 */
		if (!divoom_lvgl_top_music_ctrl_hint_is_music_key(key_value))
			divoom_lvgl_top_music_ctrl_hint_del_menu();
		divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
	}
}

static void divoom_lvgl_top_music_ctrl_hint_create_icon(void)
{
	Pdivoom_lvgl_top_music_ctrl_hint_ctrl ph;
	lv_obj_t *pill;
	int i;
	static const int icon_x[MC_HINT_SLOT_NUM] = { MC_HINT_PRE_X, MC_HINT_CENTER_X, MC_HINT_NEXT_X };

	ph = pdivoom_lvgl_top_music_ctrl_hint_handle;
	if (!ph)
		return;

	ph->cout_obj = lv_btn_create(ph->main_obj);
	lv_obj_remove_style_all(ph->cout_obj);
	lv_obj_set_style_pad_all(ph->cout_obj, 0, 0);
	lv_obj_set_style_border_width(ph->cout_obj, 0, 0);
	lv_obj_set_style_outline_width(ph->cout_obj, 0, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
	lv_obj_set_style_shadow_width(ph->cout_obj, 0, 0);
	lv_obj_set_style_bg_opa(ph->cout_obj, LV_OPA_0, 0);
	lv_obj_set_pos(ph->cout_obj, 0, 0);
	lv_obj_set_size(ph->cout_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_add_event_cb(ph->cout_obj, divoom_lvgl_top_music_ctrl_hint_event_cb, LV_EVENT_KEY, NULL);

	lv_style_init(&ph->pill_style);
	lv_style_set_bg_color(&ph->pill_style, lv_color_hex(0xE4E4E4));
	lv_style_set_bg_opa(&ph->pill_style, LV_OPA_90);
	lv_style_set_border_color(&ph->pill_style, lv_color_hex(0xFFFFFF));
	lv_style_set_border_opa(&ph->pill_style, LV_OPA_60);
	lv_style_set_border_width(&ph->pill_style, 1);
	lv_style_set_radius(&ph->pill_style, MC_HINT_PILL_H / 2);
	lv_style_set_pad_all(&ph->pill_style, 0);
	lv_style_set_shadow_width(&ph->pill_style, 10);
	lv_style_set_shadow_ofs_y(&ph->pill_style, 4);
	lv_style_set_shadow_opa(&ph->pill_style, LV_OPA_30);
	lv_style_set_shadow_color(&ph->pill_style, lv_color_hex(0x000000));

	pill = lv_obj_create(ph->cout_obj);
	ph->pill_obj = pill;
	lv_obj_remove_style_all(pill);
	lv_obj_add_style(pill, &ph->pill_style, 0);
	lv_obj_set_size(pill, MC_HINT_PILL_W, MC_HINT_PILL_H);
	lv_obj_set_pos(pill, MC_HINT_PILL_X, MC_HINT_PILL_Y);
	lv_obj_clear_flag(pill, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_clear_flag(pill, LV_OBJ_FLAG_CLICKABLE);

	for (i = 0; i < MC_HINT_SLOT_NUM; i++)
	{
		ph->icon_img[i] = lv_img_create(ph->cout_obj);
		lv_img_set_size_mode(ph->icon_img[i], LV_IMG_SIZE_MODE_REAL);
		lv_obj_set_size(ph->icon_img[i], MC_HINT_ICON_SZ, MC_HINT_ICON_SZ);
		lv_obj_set_pos(ph->icon_img[i], icon_x[i], MC_HINT_ICON_Y);
		lv_obj_clear_flag(ph->icon_img[i], LV_OBJ_FLAG_CLICKABLE);
		ph->icon_name[i][0] = '\0';
	}
}

static void divoom_lvgl_top_music_ctrl_hint_del_menu(void)
{
	Pdivoom_lvgl_top_music_ctrl_hint_ctrl ph;

	ph = pdivoom_lvgl_top_music_ctrl_hint_handle;
	if (!ph)
		return;

	if (ph->hide_timer)
	{
		lv_timer_del(ph->hide_timer);
		ph->hide_timer = NULL;
	}
	divoom_lvgl_top_control_remove_top(ph);
	lv_obj_del_async(ph->main_obj);
	lv_style_reset(&ph->pill_style);
	divoom_system_free(ph);
	pdivoom_lvgl_top_music_ctrl_hint_handle = NULL;
}

static void divoom_lvgl_top_music_ctrl_hint_timer_cb(lv_timer_t *timer)
{
	(void)timer;
	divoom_lvgl_top_music_ctrl_hint_del_menu();
}

void divoom_lvgl_top_music_ctrl_hint_show(uint8 key_value)
{
	Pdivoom_lvgl_top_music_ctrl_hint_ctrl ph;
	uint8 playing;

	if (!divoom_lvgl_top_music_ctrl_hint_is_music_key(key_value))
		return;

	divoom_get_lvgl_lock();
	playing = divoom_music_ctrl_is_playing();
	ph = pdivoom_lvgl_top_music_ctrl_hint_handle;
	if (ph)
	{
		divoom_lvgl_top_music_ctrl_hint_apply_icons(key_value, playing);
		if (ph->hide_timer)
			lv_timer_reset(ph->hide_timer);
		divoom_lvgl_layer_top_raise(ph->main_obj);
		divoom_free_lvgl_lock();
		return;
	}

	ph = (Pdivoom_lvgl_top_music_ctrl_hint_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_top_music_ctrl_hint_ctrl));
	if (!ph)
	{
		divoom_free_lvgl_lock();
		return;
	}
	memset(ph, 0, sizeof(*ph));
	pdivoom_lvgl_top_music_ctrl_hint_handle = ph;

	ph->main_obj = lv_obj_create(lv_layer_top());
	lv_obj_set_style_bg_opa(ph->main_obj, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(ph->main_obj, 0, 0);
	lv_obj_set_style_border_width(ph->main_obj, 0, 0);
	lv_obj_set_style_shadow_width(ph->main_obj, 0, 0);
	lv_obj_set_style_outline_width(ph->main_obj, 0, 0);
	lv_obj_clear_flag(ph->main_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(ph->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(ph->main_obj, 0, 0);

	divoom_lvgl_top_music_ctrl_hint_create_icon();
	divoom_lvgl_top_music_ctrl_hint_apply_icons(key_value, playing);
	ph->hide_timer = lv_timer_create(divoom_lvgl_top_music_ctrl_hint_timer_cb, MC_HINT_AUTO_MS, NULL);
	divoom_lvgl_top_control_add_top(ph, ph->cout_obj);
	divoom_lvgl_layer_top_raise(ph->main_obj);
	divoom_free_lvgl_lock();
}

void divoom_lvgl_top_music_ctrl_hint_exit(void)
{
	divoom_lvgl_top_music_ctrl_hint_del_menu();
}

#endif

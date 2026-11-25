#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_font_manage.h"
#include "divoom_lvgl_top_audio_tip_menu.h"
#include "divoom_lvgl_top_control.h"

#ifdef DIVOOM_PRODUCT_ASTRO_TOO
#include "divoom_astrootoo_theme.h"

#define DIVOOM_ASTRO_TOP_TOAST_UI_DIR "82/"
#endif

typedef struct _divoom_lvgl_top_audio_tip_menu_ctrl
{
	void *main_obj;		
	void *cout_obj;
	void *label_obj;

	int audio_tip_flag;
	void *top_disp_obj;

	int exit_time;
	lv_timer_t *menu_exit_timer;
}divoom_lvgl_top_audio_tip_menu_ctrl, *Pdivoom_lvgl_top_audio_tip_menu_ctrl;

Pdivoom_lvgl_top_audio_tip_menu_ctrl pdivoom_lvgl_top_audio_tip_menu_handle = NULL;
static volatile int s_audio_tip_pending_valid = 0;
static volatile int s_audio_tip_pending_flag = 0;

static void divoom_lvgl_top_audio_tip_menu_show(int audio_tip_flag);

#ifdef DIVOOM_PRODUCT_ASTRO_TOO
static void divoom_lvgl_top_audio_tip_menu_get_assets(int flag, const char **img_path, const char **str_key)
{
	switch (flag) {
	case DIVOOM_AUDIO_TIP_TF_INSERTED:
	case DIVOOM_AUDIO_TIP_TF_REMOVED:
		*img_path = DIVOOM_ASTRO_TOP_TOAST_UI_DIR "at_toast_source_tf.png";
		*str_key = (flag == DIVOOM_AUDIO_TIP_TF_INSERTED) ? "TFInsertedTip" : "TFRemovedTip";
		break;
	case DIVOOM_AUDIO_TIP_AUX_INSERTED:
		*img_path = "99/99_aux_inserted_tip.png";
		*str_key = "AUXInsertedTip";
		break;
	case DIVOOM_AUDIO_TIP_AUX_REMOVED:
		*img_path = "99/99_aux_removed_tip.png";
		*str_key = "AUXRemovedTip";
		break;
	case DIVOOM_AUDIO_TIP_UAC_INSERTED:
	case DIVOOM_AUDIO_TIP_UAC_REMOVED:
		*img_path = DIVOOM_ASTRO_TOP_TOAST_UI_DIR "at_toast_source_uac.png";
		*str_key = (flag == DIVOOM_AUDIO_TIP_UAC_INSERTED) ? "UACInsertedTip" : "UACRemovedTip";
		break;
	case DIVOOM_AUDIO_TIP_TWS_CONNECTED_LEFT:
	case DIVOOM_AUDIO_TIP_TWS_CONNECTED_RIGHT:
		*img_path = DIVOOM_ASTRO_TOP_TOAST_UI_DIR "at_toast_source_bluetooth.png";
		*str_key = (flag == DIVOOM_AUDIO_TIP_TWS_CONNECTED_LEFT)
			? "LeftChannel" : "RightChannel";
		break;
	case DIVOOM_AUDIO_TIP_BT_CONNECTED:
	default:
		*img_path = DIVOOM_ASTRO_TOP_TOAST_UI_DIR "at_toast_source_bluetooth.png";
		*str_key = (flag == DIVOOM_AUDIO_TIP_BT_CONNECTED) ? "Connected" : "BTDisconnectedTip";
		break;
	}
}

static void divoom_lvgl_top_audio_tip_menu_apply_panel_style(lv_obj_t *obj)
{
	lv_obj_set_style_bg_color(obj, lv_color_hex(DIVOOM_ASTRO_BG), 0);
	lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(obj, 2, 0);
	lv_obj_set_style_border_color(obj, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
	lv_obj_set_style_radius(obj, DIVOOM_ASTRO_RADIUS_MD, 0);
	lv_obj_set_style_pad_top(obj, DIVOOM_ASTRO_TOP_TOAST_PAD_Y, 0);
	lv_obj_set_style_pad_bottom(obj, DIVOOM_ASTRO_TOP_TOAST_PAD_Y, 0);
	lv_obj_set_style_pad_left(obj, DIVOOM_ASTRO_TOP_TOAST_PAD_X, 0);
	lv_obj_set_style_pad_right(obj, DIVOOM_ASTRO_TOP_TOAST_PAD_X, 0);
	lv_obj_set_style_pad_column(obj, DIVOOM_ASTRO_TOP_TOAST_GAP, 0);
	lv_obj_set_style_shadow_width(obj, 0, 0);
	lv_obj_set_style_outline_width(obj, 0, 0);
}
#endif

void divoom_lvgl_top_audio_tip_menu_del_menu(void)
{
	if(pdivoom_lvgl_top_audio_tip_menu_handle->menu_exit_timer)
	{
		lv_timer_del(pdivoom_lvgl_top_audio_tip_menu_handle->menu_exit_timer);
	}
#ifndef DIVOOM_PRODUCT_ASTRO_TOO
    divoom_lvgl_top_control_remove_top(pdivoom_lvgl_top_audio_tip_menu_handle);
#endif
	divoom_menu_control_update_video_toast(DIVOOM_VIDEO_TOAST_AUDIO, NULL);
	lv_obj_del_async(pdivoom_lvgl_top_audio_tip_menu_handle->main_obj);
	divoom_system_free(pdivoom_lvgl_top_audio_tip_menu_handle);
	pdivoom_lvgl_top_audio_tip_menu_handle = NULL;
}

void divoom_lvgl_top_audio_tip_removed_return_menu(int audio_tip_flag, int active_id)
{
	if(audio_tip_flag == DIVOOM_AUDIO_TIP_TF_REMOVED && active_id == DIVOOM_MENU_CONTROL_ID_LOCAL_MUSIC_FOLDER_MENU)
	{
		divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_DISP_CLOCK_DIAL);
	}
	else if(audio_tip_flag == DIVOOM_AUDIO_TIP_AUX_REMOVED && active_id == DIVOOM_MENU_CONTROL_ID_AUX_MUSIC_MENU)
	{
		divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_DISP_CLOCK_DIAL);
	}
	else if(audio_tip_flag == DIVOOM_AUDIO_TIP_UAC_REMOVED && active_id == DIVOOM_MENU_CONTROL_ID_UAC_MUSIC_MENU)
	{
		divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_DISP_CLOCK_DIAL);
	}
	else if(audio_tip_flag == DIVOOM_AUDIO_TIP_BT_DISCONNECTED && active_id == DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_BLUETOOTH)
	{
		divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_DISP_CLOCK_DIAL);
	}
}

void divoom_lvgl_top_audio_tip_menu_time_handle(lv_timer_t *exit_timer)
{
	int audio_tip_flag;
	int exit_time;

	(void)exit_timer;
	if (pdivoom_lvgl_top_audio_tip_menu_handle == NULL)
		return;

	pdivoom_lvgl_top_audio_tip_menu_handle->exit_time++;
	audio_tip_flag = pdivoom_lvgl_top_audio_tip_menu_handle->audio_tip_flag;
	exit_time = pdivoom_lvgl_top_audio_tip_menu_handle->exit_time;

	switch (audio_tip_flag) {
	case DIVOOM_AUDIO_TIP_TF_INSERTED:
	case DIVOOM_AUDIO_TIP_AUX_INSERTED:
	case DIVOOM_AUDIO_TIP_UAC_INSERTED:
	case DIVOOM_AUDIO_TIP_BT_CONNECTED:
	case DIVOOM_AUDIO_TIP_TWS_CONNECTED_LEFT:
	case DIVOOM_AUDIO_TIP_TWS_CONNECTED_RIGHT:
		if (exit_time >= 4)
			divoom_lvgl_top_audio_tip_menu_del_menu();
		break;

	case DIVOOM_AUDIO_TIP_TF_REMOVED:
	case DIVOOM_AUDIO_TIP_AUX_REMOVED:
	case DIVOOM_AUDIO_TIP_UAC_REMOVED:
		if (exit_time >= 2) {
			int active_id = divoom_menu_control_get_active_menu_id();

			divoom_lvgl_top_audio_tip_menu_del_menu();
			divoom_lvgl_top_audio_tip_removed_return_menu(audio_tip_flag, active_id);
		}
		break;

	case DIVOOM_AUDIO_TIP_BT_DISCONNECTED:
		/* 设置页仍有效，只关 toast，不退回表盘 */
		if (exit_time >= 2)
			divoom_lvgl_top_audio_tip_menu_del_menu();
		break;

	default:
		break;
	}
}

#ifndef DIVOOM_PRODUCT_ASTRO_TOO
static void divoom_lvgl_top_audio_tip_menu_event_cb(lv_event_t * e)
{
	int i;
    lv_event_code_t code = lv_event_get_code(e);
	
	if(code == LV_EVENT_KEY)
    {
        char *key_info = (char *)lv_event_get_param(e);
        char c = key_info[0];
		int active_id, audio_tip_flag;
				
        if(pdivoom_lvgl_top_audio_tip_menu_handle == NULL)
        {
			return;
        }

		pdivoom_lvgl_top_audio_tip_menu_handle->exit_time = 0;

		active_id = divoom_menu_control_get_active_menu_id();
		audio_tip_flag = pdivoom_lvgl_top_audio_tip_menu_handle->audio_tip_flag;

		if(c == DIVOOM_LVGL_KEY_PRE) 
		{
			divoom_lvgl_top_audio_tip_menu_del_menu();
			divoom_lvgl_top_audio_tip_removed_return_menu(audio_tip_flag, active_id);
        }
		else if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			divoom_lvgl_top_audio_tip_menu_del_menu();
			divoom_lvgl_top_audio_tip_removed_return_menu(audio_tip_flag, active_id);
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			int audio_tip_flag, active_id;

			audio_tip_flag = pdivoom_lvgl_top_audio_tip_menu_handle->audio_tip_flag;
			active_id = divoom_menu_control_get_active_menu_id();

			divoom_lvgl_top_audio_tip_menu_del_menu();
            
			if(audio_tip_flag == DIVOOM_AUDIO_TIP_TF_INSERTED)
    		{
				divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_LOCAL_MUSIC_FOLDER_MENU, NULL, 0, 0);
			}
			else if(audio_tip_flag == DIVOOM_AUDIO_TIP_TF_REMOVED)
    		{
				if(active_id == DIVOOM_MENU_CONTROL_ID_LOCAL_MUSIC_FOLDER_MENU)
				{
					divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_DISP_CLOCK_DIAL);
				}
			}
			else if(audio_tip_flag == DIVOOM_AUDIO_TIP_AUX_INSERTED)
			{
				divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_AUX_MUSIC_MENU, NULL, 0, 0);
			}
			else if(audio_tip_flag == DIVOOM_AUDIO_TIP_AUX_REMOVED)
			{
				if(active_id == DIVOOM_MENU_CONTROL_ID_AUX_MUSIC_MENU)
				{
					divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_DISP_CLOCK_DIAL);
				}
			}
			else if(audio_tip_flag == DIVOOM_AUDIO_TIP_UAC_INSERTED)
			{
				divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_UAC_MUSIC_MENU, NULL, 0, 0);
			}
			else if(audio_tip_flag == DIVOOM_AUDIO_TIP_UAC_REMOVED)
			{
				if(active_id == DIVOOM_MENU_CONTROL_ID_UAC_MUSIC_MENU)
				{
					divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_DISP_CLOCK_DIAL);
				}
			}
			else if(audio_tip_flag == DIVOOM_AUDIO_TIP_BT_CONNECTED)
			{
				divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_BLUETOOTH, NULL, 0, 0);
			}
			else if(audio_tip_flag == DIVOOM_AUDIO_TIP_BT_DISCONNECTED)
			{
				if(active_id == DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_BLUETOOTH)
				{
					divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_DISP_CLOCK_DIAL);
				}
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_lvgl_top_audio_tip_menu_del_menu();
			divoom_lvgl_top_audio_tip_removed_return_menu(audio_tip_flag, active_id);
		}
		else
		{
			divoom_lvgl_top_audio_tip_menu_del_menu();
			divoom_lvgl_top_audio_tip_removed_return_menu(audio_tip_flag, active_id);
		}
    }
	
}
#endif /* !DIVOOM_PRODUCT_ASTRO_TOO */


void divoom_lvgl_top_audio_tip_menu_init(int audio_tip_flag)
{
	/* UART/热插拔线程禁止直接操作 LVGL；由 LVGL handler poll 展示 */
	s_audio_tip_pending_flag = audio_tip_flag;
	s_audio_tip_pending_valid = 1;
}

void divoom_lvgl_top_audio_tip_menu_poll(void)
{
	int flag;

	divoom_menu_control_update_video_toast(DIVOOM_VIDEO_TOAST_AUDIO,
		pdivoom_lvgl_top_audio_tip_menu_handle ? pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj : NULL);
	if (!s_audio_tip_pending_valid)
		return;
	s_audio_tip_pending_valid = 0;
	flag = s_audio_tip_pending_flag;
	divoom_lvgl_top_audio_tip_menu_show(flag);
}

static void divoom_lvgl_top_audio_tip_menu_show(int audio_tip_flag)
{
	if (!divoom_lvgl_font_manage_is_inited())
		return;

	if(pdivoom_lvgl_top_audio_tip_menu_handle != NULL)
	{
		divoom_lvgl_top_audio_tip_menu_del_menu();
	}

	pdivoom_lvgl_top_audio_tip_menu_handle = (Pdivoom_lvgl_top_audio_tip_menu_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_top_audio_tip_menu_ctrl));

	if(pdivoom_lvgl_top_audio_tip_menu_handle == NULL)
	{
		return;
	}
	memset(pdivoom_lvgl_top_audio_tip_menu_handle, 0, sizeof(divoom_lvgl_top_audio_tip_menu_ctrl));

	pdivoom_lvgl_top_audio_tip_menu_handle->exit_time = 0;
	pdivoom_lvgl_top_audio_tip_menu_handle->audio_tip_flag = audio_tip_flag;

	pdivoom_lvgl_top_audio_tip_menu_handle->main_obj = lv_obj_create(lv_layer_top()); //在顶层创建主页面

	lv_obj_set_style_bg_opa(pdivoom_lvgl_top_audio_tip_menu_handle->main_obj, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(pdivoom_lvgl_top_audio_tip_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_border_width(pdivoom_lvgl_top_audio_tip_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_shadow_width(pdivoom_lvgl_top_audio_tip_menu_handle->main_obj, 0, 0);
	lv_obj_set_style_outline_width(pdivoom_lvgl_top_audio_tip_menu_handle->main_obj, 0, 0);
	lv_obj_clear_flag(pdivoom_lvgl_top_audio_tip_menu_handle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(pdivoom_lvgl_top_audio_tip_menu_handle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(pdivoom_lvgl_top_audio_tip_menu_handle->main_obj, 0, 0);
	
#ifdef DIVOOM_PRODUCT_ASTRO_TOO
	pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj = lv_obj_create(pdivoom_lvgl_top_audio_tip_menu_handle->main_obj);
#else
	pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj = lv_btn_create(pdivoom_lvgl_top_audio_tip_menu_handle->main_obj);
#endif
    	
#ifdef DIVOOM_PRODUCT_ASTRO_TOO
	{
		const char *img_path;
		const char *str_key;
		lv_obj_t *img_obj;
		lv_obj_t *label;
		lv_obj_t *spacer;
		lv_coord_t text_w;

		divoom_lvgl_top_audio_tip_menu_get_assets(audio_tip_flag, &img_path, &str_key);
		divoom_lvgl_top_audio_tip_menu_apply_panel_style(
			pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj);
		lv_obj_clear_flag(pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_clear_flag(pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_set_pos(pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj,
			       DIVOOM_ASTRO_TOP_TOAST_X, DIVOOM_ASTRO_TOP_TOAST_Y);
		lv_obj_set_size(pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj,
				DIVOOM_ASTRO_TOP_TOAST_W, DIVOOM_ASTRO_TOP_TOAST_H);
		lv_obj_set_flex_flow(pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj, LV_FLEX_FLOW_ROW);
		lv_obj_set_flex_align(pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj,
				      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		img_obj = lv_img_create(pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj);
		divoom_menu_control_lv_img_set_src(img_obj, img_path);
		lv_obj_set_size(img_obj, DIVOOM_ASTRO_TOP_TOAST_ICON, DIVOOM_ASTRO_TOP_TOAST_ICON);

		text_w = DIVOOM_ASTRO_TOP_TOAST_W - DIVOOM_ASTRO_TOP_TOAST_PAD_X * 2
			 - DIVOOM_ASTRO_TOP_TOAST_ICON * 2 - DIVOOM_ASTRO_TOP_TOAST_GAP * 2;
		if (text_w < 120)
			text_w = 120;

		label = lv_label_create(pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj);
		lv_obj_set_width(label, text_w);
		lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
		lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
		lv_obj_set_style_text_font(label,
			divoom_menu_control_get_font_info(22, DIVOOM_MENU_CONTROL_ID_TOP_AUDIO_TIP_MENU), 0);
		lv_obj_set_style_text_color(label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
		if (audio_tip_flag == DIVOOM_AUDIO_TIP_TWS_CONNECTED_LEFT
			|| audio_tip_flag == DIVOOM_AUDIO_TIP_TWS_CONNECTED_RIGHT)
		{
			char buf[96];

			snprintf(buf, sizeof(buf), "TWS %s · %s",
				divoom_menu_control_get_disp_str("Connected"),
				divoom_menu_control_get_disp_str(str_key));
			lv_label_set_text(label, buf);
		}
		else
		{
			lv_label_set_text(label, divoom_menu_control_get_disp_str(str_key));
		}
		pdivoom_lvgl_top_audio_tip_menu_handle->label_obj = label;

		spacer = lv_obj_create(pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj);
		lv_obj_set_size(spacer, DIVOOM_ASTRO_TOP_TOAST_ICON, DIVOOM_ASTRO_TOP_TOAST_ICON);
		lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
		lv_obj_set_style_border_width(spacer, 0, 0);
		lv_obj_set_style_pad_all(spacer, 0, 0);
		lv_obj_clear_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);
	}
#else
	lv_obj_set_style_outline_width(pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj, 0, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj, 4, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_shadow_width(pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj, 0, LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_pad_all(pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj, 0, 0);
	lv_obj_set_style_bg_color(pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_bg_opa(pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj, 85*LV_OPA_100/100, 0);
	lv_obj_set_style_border_color(pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj, lv_color_hex(0x0), 0);
	lv_obj_set_style_border_opa(pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj, 12*LV_OPA_100/100, 0);
	lv_obj_set_style_radius(pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj, 48, 0);	
	
    lv_obj_clear_flag(pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj, divoom_lvgl_top_audio_tip_menu_event_cb, LV_EVENT_KEY, NULL);
    lv_obj_set_pos(pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj, 40, 40);
    lv_obj_set_size(pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj, 720, 96);

    void *img_obj = lv_img_create(pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj);
	if(pdivoom_lvgl_top_audio_tip_menu_handle->audio_tip_flag == DIVOOM_AUDIO_TIP_TF_INSERTED)
    {
		divoom_menu_control_lv_img_set_src(img_obj, "99/99_tf_inserted_tip.png");
	}
	else if(pdivoom_lvgl_top_audio_tip_menu_handle->audio_tip_flag == DIVOOM_AUDIO_TIP_TF_REMOVED)
    {
		divoom_menu_control_lv_img_set_src(img_obj, "99/99_tf_removed_tip.png");
	}
	else if(pdivoom_lvgl_top_audio_tip_menu_handle->audio_tip_flag == DIVOOM_AUDIO_TIP_AUX_INSERTED)
	{
		divoom_menu_control_lv_img_set_src(img_obj, "99/99_aux_inserted_tip.png");
	}
	else if(pdivoom_lvgl_top_audio_tip_menu_handle->audio_tip_flag == DIVOOM_AUDIO_TIP_AUX_REMOVED)
	{
		divoom_menu_control_lv_img_set_src(img_obj, "99/99_aux_removed_tip.png");
	}
	else if(pdivoom_lvgl_top_audio_tip_menu_handle->audio_tip_flag == DIVOOM_AUDIO_TIP_UAC_INSERTED)
	{
		divoom_menu_control_lv_img_set_src(img_obj, "99/99_uac_inserted_tip.png");
	}
	else if(pdivoom_lvgl_top_audio_tip_menu_handle->audio_tip_flag == DIVOOM_AUDIO_TIP_UAC_REMOVED)
	{
		divoom_menu_control_lv_img_set_src(img_obj, "99/99_uac_removed_tip.png");
	}
	else if(pdivoom_lvgl_top_audio_tip_menu_handle->audio_tip_flag == DIVOOM_AUDIO_TIP_BT_CONNECTED)
	{
		divoom_menu_control_lv_img_set_src(img_obj, "99/99_bt_connected_tip.png");
	}
	else if(pdivoom_lvgl_top_audio_tip_menu_handle->audio_tip_flag == DIVOOM_AUDIO_TIP_BT_DISCONNECTED)
	{
		divoom_menu_control_lv_img_set_src(img_obj, "99/99_bt_disconnected_tip.png");
	}
	lv_obj_set_pos(img_obj, 21-4, 21-4);
    
	lv_obj_t* cont = lv_obj_create(pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj);
	lv_obj_set_size(cont, 466+120, 72);
	lv_obj_set_pos(cont, 96-4, 12-2);
	lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
	lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

	void *label = lv_label_create(cont);
	lv_obj_set_width(label, 466+120);
	lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
	lv_obj_set_align(label, LV_ALIGN_CENTER);	//上下居中对齐
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(23, DIVOOM_MENU_CONTROL_ID_TOP_AUDIO_TIP_MENU), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x0), 0);
	if(pdivoom_lvgl_top_audio_tip_menu_handle->audio_tip_flag == DIVOOM_AUDIO_TIP_TF_INSERTED)
    {
		lv_label_set_text(label, divoom_menu_control_get_disp_str("TFInsertedTip"));
	}
	else if(pdivoom_lvgl_top_audio_tip_menu_handle->audio_tip_flag == DIVOOM_AUDIO_TIP_TF_REMOVED)
    {
		lv_label_set_text(label, divoom_menu_control_get_disp_str("TFRemovedTip"));
	}
	else if(pdivoom_lvgl_top_audio_tip_menu_handle->audio_tip_flag == DIVOOM_AUDIO_TIP_AUX_INSERTED)
	{
		lv_label_set_text(label, divoom_menu_control_get_disp_str("AUXInsertedTip"));
	}
	else if(pdivoom_lvgl_top_audio_tip_menu_handle->audio_tip_flag == DIVOOM_AUDIO_TIP_AUX_REMOVED)
	{
		lv_label_set_text(label, divoom_menu_control_get_disp_str("AUXRemovedTip"));
	}
	else if(pdivoom_lvgl_top_audio_tip_menu_handle->audio_tip_flag == DIVOOM_AUDIO_TIP_UAC_INSERTED)
	{
		lv_label_set_text(label, divoom_menu_control_get_disp_str("UACInsertedTip"));
	}
	else if(pdivoom_lvgl_top_audio_tip_menu_handle->audio_tip_flag == DIVOOM_AUDIO_TIP_UAC_REMOVED)
	{
		lv_label_set_text(label, divoom_menu_control_get_disp_str("UACRemovedTip"));
	}
	else if(pdivoom_lvgl_top_audio_tip_menu_handle->audio_tip_flag == DIVOOM_AUDIO_TIP_BT_CONNECTED)
	{
		lv_label_set_text(label, divoom_menu_control_get_disp_str("Connected"));
	}
	else if(pdivoom_lvgl_top_audio_tip_menu_handle->audio_tip_flag == DIVOOM_AUDIO_TIP_BT_DISCONNECTED)
	{
		lv_label_set_text(label, divoom_menu_control_get_disp_str("BTDisconnectedTip"));
	}
#endif
	pdivoom_lvgl_top_audio_tip_menu_handle->menu_exit_timer = lv_timer_create(divoom_lvgl_top_audio_tip_menu_time_handle, 1000, NULL);
	divoom_lvgl_layer_top_raise(pdivoom_lvgl_top_audio_tip_menu_handle->main_obj);
	
#ifndef DIVOOM_PRODUCT_ASTRO_TOO
	divoom_lvgl_top_control_add_top(pdivoom_lvgl_top_audio_tip_menu_handle, pdivoom_lvgl_top_audio_tip_menu_handle->top_disp_obj);
#endif
}

void divoom_lvgl_top_audio_tip_menu_exit(void)
{
	if(pdivoom_lvgl_top_audio_tip_menu_handle != NULL)
	{
		if(pdivoom_lvgl_top_audio_tip_menu_handle->menu_exit_timer)
		{
			lv_timer_del(pdivoom_lvgl_top_audio_tip_menu_handle->menu_exit_timer);
		}
#ifndef DIVOOM_PRODUCT_ASTRO_TOO
        divoom_lvgl_top_control_remove_top(pdivoom_lvgl_top_audio_tip_menu_handle);
#endif
		divoom_menu_control_update_video_toast(DIVOOM_VIDEO_TOAST_AUDIO, NULL);
		lv_obj_del_async(pdivoom_lvgl_top_audio_tip_menu_handle->main_obj);
		divoom_system_free(pdivoom_lvgl_top_audio_tip_menu_handle);
		pdivoom_lvgl_top_audio_tip_menu_handle = NULL;
	}
}

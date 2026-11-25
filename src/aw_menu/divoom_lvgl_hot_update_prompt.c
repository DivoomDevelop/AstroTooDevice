#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_lvgl_hot_update_prompt.h"
#include <time.h>

#define DIVOOM_HOT_UPDATE_PROMPT_POLL_MS       100
#define DIVOOM_HOT_UPDATE_PROMPT_AUTO_EXIT_MS 5000
#define DIVOOM_HOT_UPDATE_PROMPT_BODY_X         40
#define DIVOOM_HOT_UPDATE_PROMPT_BODY_W        400
#define DIVOOM_HOT_UPDATE_PROMPT_BODY_Y         64
#define DIVOOM_HOT_UPDATE_PROMPT_BODY_H        294
#define DIVOOM_HOT_UPDATE_PROMPT_BUTTON_Y      372
#define DIVOOM_HOT_UPDATE_PROMPT_BUTTON_W      196
#define DIVOOM_HOT_UPDATE_PROMPT_BUTTON_H       56
#define DIVOOM_HOT_UPDATE_PROMPT_BUTTON_GAP      8

enum
{
    DIVOOM_HOT_UPDATE_PROMPT_PHASE_ASK = 0,
    DIVOOM_HOT_UPDATE_PROMPT_PHASE_START,
    DIVOOM_HOT_UPDATE_PROMPT_PHASE_WAIT_RESULT,
};

typedef struct _divoom_lvgl_hot_update_prompt_ctrl
{
    divoom_astrootoo_ui_base ui;
    int menu_id;
    int focus_index;
    int phase;
    int exit_requested;
    uint32 show_tick;
    const char *body_text;
    const char *updating_text;
    lv_obj_t *root_obj;
    lv_obj_t *body_label;
    lv_obj_t *cancel_btn;
    lv_obj_t *update_btn;
    lv_obj_t *key_btn;
    lv_timer_t *poll_timer;
} divoom_lvgl_hot_update_prompt_ctrl, *Pdivoom_lvgl_hot_update_prompt_ctrl;

static int32 g_divoom_hot_update_prompt_last_date = -1;

static int divoom_lvgl_hot_update_prompt_get_date(
    int32 *date_token, int32 *mday)
{
    time_t now;
    struct tm time_info;

    now = time(NULL);
#if defined(_WIN32)
    if(localtime_s(&time_info, &now) != 0)
    {
        return 0;
    }
#else
    {
        struct tm *local_info = localtime(&now);

        if(local_info == NULL)
        {
            return 0;
        }
        time_info = *local_info;
    }
#endif
    if(date_token != NULL)
    {
        *date_token = (int32)((time_info.tm_year + 1900) * 10000 +
            (time_info.tm_mon + 1) * 100 + time_info.tm_mday);
    }
    if(mday != NULL)
    {
        *mday = (int32)time_info.tm_mday;
    }
    return 1;
}


static lv_obj_t *divoom_lvgl_hot_update_prompt_create_button(
    Pdivoom_lvgl_hot_update_prompt_ctrl phandle,
    int x, const char *text)
{
    lv_obj_t *button;
    lv_obj_t *label;

    button = lv_btn_create(phandle->root_obj);
    lv_obj_set_pos(button, x, DIVOOM_HOT_UPDATE_PROMPT_BUTTON_Y);
    lv_obj_set_size(button, DIVOOM_HOT_UPDATE_PROMPT_BUTTON_W,
        DIVOOM_HOT_UPDATE_PROMPT_BUTTON_H);
    lv_obj_set_style_pad_all(button, 0, 0);
    lv_obj_set_style_border_width(button, 2, 0);
    lv_obj_set_style_border_color(button,
        lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
    lv_obj_set_style_radius(button, DIVOOM_ASTRO_RADIUS_MD, 0);
    lv_obj_set_style_shadow_width(button, 0, 0);
    lv_obj_clear_flag(button, LV_OBJ_FLAG_SCROLLABLE);

    label = lv_label_create(button);
    lv_label_set_text(label, text ? text : "");
    lv_obj_set_style_text_color(label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(label,
        divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
    lv_obj_center(label);
    return button;
}

static void divoom_lvgl_hot_update_prompt_refresh_focus(
    Pdivoom_lvgl_hot_update_prompt_ctrl phandle)
{
    lv_obj_t *buttons[2];
    int i;

    if(phandle == NULL)
    {
        return;
    }
    buttons[0] = phandle->cancel_btn;
    buttons[1] = phandle->update_btn;
    for(i = 0; i < 2; i++)
    {
        lv_obj_set_style_bg_color(buttons[i], lv_color_hex(
            i == phandle->focus_index ? DIVOOM_ASTRO_SURFACE_FOCUS :
                DIVOOM_ASTRO_SURFACE_IDLE), 0);
        lv_obj_set_style_bg_opa(buttons[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(buttons[i], lv_color_hex(
            i == phandle->focus_index ? DIVOOM_ASTRO_TAB_INK :
                DIVOOM_ASTRO_STROKE), 0);
    }
}

static void divoom_lvgl_hot_update_prompt_leave(
    Pdivoom_lvgl_hot_update_prompt_ctrl phandle)
{
    if(phandle == NULL || phandle->exit_requested)
    {
        return;
    }
    phandle->exit_requested = 1;
    if(phandle->poll_timer != NULL)
    {
        lv_timer_pause(phandle->poll_timer);
    }
    divoom_menu_control_return_pre();
}

static void divoom_lvgl_hot_update_prompt_key_cb(lv_event_t *event)
{
    Pdivoom_lvgl_hot_update_prompt_ctrl phandle;
    uint8 key_code;

    if(lv_event_get_code(event) != LV_EVENT_KEY)
    {
        return;
    }
    phandle = (Pdivoom_lvgl_hot_update_prompt_ctrl)
        lv_event_get_user_data(event);
    if(phandle == NULL || phandle->phase !=
        DIVOOM_HOT_UPDATE_PROMPT_PHASE_ASK)
    {
        return;
    }
    key_code = divoom_astrootoo_ui_key_code_from_event(event);
    if(divoom_astrootoo_ui_key_is_menu_press(event, key_code) == 0)
    {
        return;
    }
    if(divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_lvgl_hot_update_prompt_leave(phandle);
    }
    else if(divoom_astrootoo_ui_key_is_left(key_code) ||
        divoom_astrootoo_ui_key_is_right(key_code))
    {
        phandle->focus_index = phandle->focus_index == 0 ? 1 : 0;
        divoom_lvgl_hot_update_prompt_refresh_focus(phandle);
    }
    else if(divoom_astrootoo_ui_key_is_enter(key_code))
    {
        if(phandle->focus_index == 0)
        {
            divoom_lvgl_hot_update_prompt_leave(phandle);
        }
        else
        {
            /* 网络请求延后到 timer，按键回调只切轻量 UI 状态。 */
            phandle->phase = DIVOOM_HOT_UPDATE_PROMPT_PHASE_START;
            lv_label_set_text(phandle->body_label, phandle->updating_text);
            lv_obj_add_flag(phandle->cancel_btn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(phandle->update_btn, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void divoom_lvgl_hot_update_prompt_poll_cb(lv_timer_t *timer)
{
    Pdivoom_lvgl_hot_update_prompt_ctrl phandle;
    int32 add_file_num;
    int32 mday;
    int result_state;
    uint32 now_tick;

    phandle = (Pdivoom_lvgl_hot_update_prompt_ctrl)
        lv_timer_get_user_data(timer);
    if(phandle == NULL || phandle->exit_requested)
    {
        return;
    }
    if(phandle->phase == DIVOOM_HOT_UPDATE_PROMPT_PHASE_ASK)
    {
        now_tick = (uint32)divoom_system_get_tickms();
        if((uint32)(now_tick - phandle->show_tick) >=
            DIVOOM_HOT_UPDATE_PROMPT_AUTO_EXIT_MS)
        {
            divoom_lvgl_hot_update_prompt_leave(phandle);
        }
        return;
    }
    if(phandle->phase == DIVOOM_HOT_UPDATE_PROMPT_PHASE_START)
    {
        if(divoom_lvgl_hot_update_prompt_get_date(NULL, &mday) == 0 ||
            divoom_subscribe_manage_reload_more_gif(mday) == 0)
        {
            divoom_lvgl_hot_update_prompt_leave(phandle);
            return;
        }
        phandle->phase = DIVOOM_HOT_UPDATE_PROMPT_PHASE_WAIT_RESULT;
        return;
    }

    result_state = divoom_subscribe_manage_reload_result_get(&add_file_num);
    if(result_state > 0)
    {
        /* 后台“更多 GIF”更新静默下载，进度页只由订阅配置更新触发。 */
        divoom_lvgl_hot_update_prompt_leave(phandle);
    }
}

void *divoom_lvgl_hot_update_prompt_init(void *user_data, int menu_id)
{
    Pdivoom_lvgl_hot_update_prompt_ctrl phandle;
    lv_obj_t *body_cont;

    (void)user_data;
    phandle = divoom_system_malloc_out(
        sizeof(divoom_lvgl_hot_update_prompt_ctrl));
    if(phandle == NULL)
    {
        return NULL;
    }
    phandle->menu_id = menu_id;
    phandle->root_obj = divoom_astrootoo_ui_create_root(&phandle->ui);
    lv_obj_set_style_bg_color(phandle->root_obj,
        lv_color_hex(DIVOOM_ASTRO_BG), 0);

    divoom_astrootoo_ui_create_page_header_tab(phandle->root_obj,
        &phandle->ui, "cloudFaceHotUpdateTitle",
        DIVOOM_ASTRO_SUBPAGE_HEADER_IMG, menu_id);

    body_cont = lv_obj_create(phandle->root_obj);
    lv_obj_add_style(body_cont, &phandle->ui.content_style, 0);
    lv_obj_set_pos(body_cont,
        DIVOOM_HOT_UPDATE_PROMPT_BODY_X, DIVOOM_HOT_UPDATE_PROMPT_BODY_Y);
    lv_obj_set_size(body_cont,
        DIVOOM_HOT_UPDATE_PROMPT_BODY_W, DIVOOM_HOT_UPDATE_PROMPT_BODY_H);
    lv_obj_set_style_bg_opa(body_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(body_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(body_cont, LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(body_cont, LV_OBJ_FLAG_SCROLLABLE);

    phandle->body_label = lv_label_create(body_cont);
    phandle->body_text = divoom_menu_control_get_disp_str(
        "cloudFaceHotUpdateBody");
    lv_label_set_text(phandle->body_label, phandle->body_text);
    lv_label_set_long_mode(phandle->body_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(phandle->body_label, DIVOOM_HOT_UPDATE_PROMPT_BODY_W);
    lv_obj_set_height(phandle->body_label, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(phandle->body_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(phandle->body_label,
        lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(phandle->body_label,
        divoom_menu_control_get_font_info(32, menu_id), 0);
    lv_obj_set_style_text_line_space(phandle->body_label, 16, 0);

    phandle->cancel_btn = divoom_lvgl_hot_update_prompt_create_button(
        phandle, DIVOOM_HOT_UPDATE_PROMPT_BODY_X,
        divoom_menu_control_get_disp_str("Cancel"));
    phandle->update_btn = divoom_lvgl_hot_update_prompt_create_button(
        phandle, DIVOOM_HOT_UPDATE_PROMPT_BODY_X +
            DIVOOM_HOT_UPDATE_PROMPT_BUTTON_W +
            DIVOOM_HOT_UPDATE_PROMPT_BUTTON_GAP,
        divoom_menu_control_get_disp_str("cloudFaceHotUpdateConfirm"));
    phandle->updating_text = divoom_menu_control_get_disp_str(
        "cloudFaceHotUpdateToast");
    divoom_lvgl_hot_update_prompt_refresh_focus(phandle);

    phandle->key_btn = divoom_astrootoo_ui_create_key_capture_btn(
        phandle->root_obj, &phandle->ui,
        divoom_lvgl_hot_update_prompt_key_cb, phandle);
    lv_obj_move_foreground(phandle->key_btn);
    lv_group_add_obj(lv_group_get_default(), phandle->key_btn);

    phandle->poll_timer = lv_timer_create(
        divoom_lvgl_hot_update_prompt_poll_cb,
        DIVOOM_HOT_UPDATE_PROMPT_POLL_MS, phandle);
    lv_timer_pause(phandle->poll_timer);
    return phandle;
}

void divoom_lvgl_hot_update_prompt_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_hot_update_prompt_ctrl phandle;

    (void)active_obj;
    phandle = (Pdivoom_lvgl_hot_update_prompt_ctrl)handle;
    if(phandle == NULL)
    {
        return;
    }
    phandle->focus_index = 0;
    phandle->phase = DIVOOM_HOT_UPDATE_PROMPT_PHASE_ASK;
    phandle->exit_requested = 0;
    phandle->show_tick = (uint32)divoom_system_get_tickms();
    lv_label_set_text(phandle->body_label, phandle->body_text);
    lv_obj_clear_flag(phandle->cancel_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(phandle->update_btn, LV_OBJ_FLAG_HIDDEN);
    divoom_lvgl_hot_update_prompt_refresh_focus(phandle);
    lv_scr_load(phandle->root_obj);
    lv_group_focus_obj(phandle->key_btn);
    lv_timer_resume(phandle->poll_timer);
}

void divoom_lvgl_hot_update_prompt_pause(void *handle)
{
    Pdivoom_lvgl_hot_update_prompt_ctrl phandle;

    phandle = (Pdivoom_lvgl_hot_update_prompt_ctrl)handle;
    if(phandle != NULL && phandle->poll_timer != NULL)
    {
        lv_timer_pause(phandle->poll_timer);
    }
}

void divoom_lvgl_hot_update_prompt_exit(void *handle)
{
    Pdivoom_lvgl_hot_update_prompt_ctrl phandle;

    phandle = (Pdivoom_lvgl_hot_update_prompt_ctrl)handle;
    if(phandle == NULL)
    {
        return;
    }
    if(phandle->poll_timer != NULL)
    {
        lv_timer_del(phandle->poll_timer);
    }
    if(phandle->root_obj != NULL)
    {
        lv_obj_del(phandle->root_obj);
    }
    divoom_astrootoo_ui_base_reset_styles(&phandle->ui);
    divoom_system_free(phandle);
}

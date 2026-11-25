#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_astrootoo_watch.h"
#include "divoom_orbital_system_bg.h"
#if defined(DIVOOM_SIMULATOR) && DIVOOM_SIMULATOR
#include "sim_pal.h"
#endif

typedef struct _divoom_astrootoo_watch_ctrl
{
    divoom_astrootoo_ui_base ui;
    lv_obj_t *root_obj;
    lv_obj_t *key_btn;
    lv_obj_t *bg_obj;
} divoom_astrootoo_watch_ctrl, *Pdivoom_astrootoo_watch_ctrl;

static void divoom_astrootoo_watch_open_main_menu_cb(lv_timer_t *timer)
{
    divoom_menu_control_load_next_direct(DIVOOM_MENU_CONTROL_ID_MAIN, NULL, 0, 0, 0);
    lv_timer_del(timer);
}

static void divoom_astrootoo_watch_open_main_menu(void)
{
    /* 延后一帧：避免在 LV_EVENT_KEY 回调内同步切屏 */
    lv_timer_create(divoom_astrootoo_watch_open_main_menu_cb, 1, NULL);
}

static void divoom_astrootoo_watch_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_watch_ctrl phandle;
    uint8_t key_code;

    phandle = (Pdivoom_astrootoo_watch_ctrl)lv_event_get_user_data(e);
    if (!phandle || lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }

    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }

    if (phandle->key_btn)
    {
        lv_group_focus_obj(phandle->key_btn);
    }

    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    if (key_code == 0)
    {
        key_code = (uint8_t)lv_event_get_key(e);
    }

    if (divoom_astrootoo_ui_key_is_enter(key_code) || divoom_astrootoo_ui_key_is_home(key_code))
    {
        divoom_astrootoo_watch_open_main_menu();
    }
}

void *divoom_astrootoo_watch_init(void *user_data, int menu_id)
{
    Pdivoom_astrootoo_watch_ctrl phandle;

    (void)user_data;
    (void)menu_id;

    phandle = divoom_system_malloc_out(sizeof(divoom_astrootoo_watch_ctrl));
    if (!phandle)
    {
        return NULL;
    }

    phandle->root_obj = divoom_astrootoo_ui_create_root(&phandle->ui);
    phandle->bg_obj = divoom_orbital_system_bg_create(phandle->root_obj);
    if (phandle->bg_obj)
    {
        /* 系统表盘页只承载轨道系统背景动效；日期、时间、天气等文字信息不在本控件内绘制。 */
        lv_obj_set_pos(phandle->bg_obj, 0, 0);
        lv_obj_set_size(phandle->bg_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    }

    phandle->key_btn = divoom_astrootoo_ui_create_key_capture_btn(phandle->root_obj, &phandle->ui,
        divoom_astrootoo_watch_key_cb, phandle);
    lv_obj_move_foreground(phandle->key_btn);
    lv_group_add_obj(lv_group_get_default(), phandle->key_btn);

    return phandle;
}

void divoom_astrootoo_watch_disp(void *handle, void *active_obj)
{
    Pdivoom_astrootoo_watch_ctrl phandle;

    (void)active_obj;

    phandle = (Pdivoom_astrootoo_watch_ctrl)handle;
    if (!phandle)
    {
        return;
    }

    lv_scr_load(phandle->root_obj);
    if (phandle->bg_obj)
    {
        divoom_orbital_system_bg_set_active(phandle->bg_obj, 1);
    }
#if defined(DIVOOM_SIMULATOR) && DIVOOM_SIMULATOR
    {
        lv_indev_t *indev = sim_pal_get_lvgl_keypad_indev();

        if (indev)
        {
            lv_indev_set_group(indev, lv_group_get_default());
        }
    }
#endif
    if (phandle->key_btn)
    {
        lv_group_t *g = lv_group_get_default();

        if (lv_obj_get_group(phandle->key_btn) != g)
        {
            lv_group_add_obj(g, phandle->key_btn);
        }
        lv_group_focus_obj(phandle->key_btn);
    }
}

void divoom_astrootoo_watch_clean(void *handle)
{
    Pdivoom_astrootoo_watch_ctrl phandle;

    phandle = (Pdivoom_astrootoo_watch_ctrl)handle;
    if (!phandle)
    {
        return;
    }

    if (phandle->root_obj)
    {
        lv_obj_del(phandle->root_obj);
        phandle->root_obj = NULL;
    }

    divoom_astrootoo_ui_base_reset_styles(&phandle->ui);
    divoom_system_free(phandle);
}

void divoom_astrootoo_watch_pause(void *handle)
{
    Pdivoom_astrootoo_watch_ctrl phandle;

    phandle = (Pdivoom_astrootoo_watch_ctrl)handle;
    if (!phandle || !phandle->bg_obj)
    {
        return;
    }

    divoom_orbital_system_bg_set_active(phandle->bg_obj, 0);
}

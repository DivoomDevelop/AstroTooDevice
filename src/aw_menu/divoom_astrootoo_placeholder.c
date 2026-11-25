#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_astrootoo_ui_base.h"

typedef struct _divoom_astrootoo_placeholder_ctrl
{
    divoom_astrootoo_ui_base ui;
    lv_obj_t *root_obj;
    lv_obj_t *key_btn;
} divoom_astrootoo_placeholder_ctrl, *Pdivoom_astrootoo_placeholder_ctrl;

static void divoom_astrootoo_placeholder_key_cb(lv_event_t *e)
{
    uint8_t key_code;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }

    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_menu_control_return_pre();
    }
}

static const char *divoom_astrootoo_placeholder_title_from_menu_id(int menu_id)
{
    switch (menu_id)
    {
    case DIVOOM_MENU_CONTROL_ID_ALARM_LIST_MENU:
    {
        return "ALARM";
    }
    case DIVOOM_MENU_CONTROL_ID_SLEEP_AID:
    {
        return "Sleep";
    }
    case DIVOOM_MENU_CONTROL_ID_AUDIO_SOURCE_MENU:
    {
        return "Music";
    }
    case DIVOOM_MENU_CONTROL_ID_TOOL:
    {
        return "Tool";
    }
    case DIVOOM_MENU_CONTROL_ID_LYRIC_MENU:
    {
        return "Lyric";
    }
    case DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_NIGHTVIEW:
    {
        return "Night mode";
    }
    case DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_COLOR_TEMP:
    {
        return "Color temperature";
    }
    case DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_WIFI:
    {
        return "Wi-Fi";
    }
    case DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_BLUETOOTH:
    {
        return "Bluetooth";
    }
    case DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_ABOUT:
    {
        return "About";
    }
    default:
    {
        return "Settings";
    }
    }
}

void *divoom_astrootoo_placeholder_init(void *user_data, int menu_id)
{
    Pdivoom_astrootoo_placeholder_ctrl phandle;
    lv_obj_t *label;
    const char *title;

    (void)user_data;

    phandle = divoom_system_malloc_out(sizeof(divoom_astrootoo_placeholder_ctrl));
    if (!phandle)
    {
        return NULL;
    }

    title = divoom_astrootoo_placeholder_title_from_menu_id(menu_id);

    phandle->root_obj = divoom_astrootoo_ui_create_root(&phandle->ui);
    divoom_astrootoo_ui_create_header(phandle->root_obj, &phandle->ui, title, menu_id);

    label = lv_label_create(phandle->root_obj);
    lv_label_set_text(label, divoom_menu_control_get_disp_str("About"));
    lv_obj_set_style_text_color(label, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    phandle->key_btn = divoom_astrootoo_ui_create_key_capture_btn(phandle->root_obj, &phandle->ui,
        divoom_astrootoo_placeholder_key_cb, phandle);
    lv_obj_move_foreground(phandle->key_btn);
    lv_group_add_obj(lv_group_get_default(), phandle->key_btn);
    return phandle;
}

void divoom_astrootoo_placeholder_disp(void *handle, void *active_obj)
{
    Pdivoom_astrootoo_placeholder_ctrl phandle;

    phandle = (Pdivoom_astrootoo_placeholder_ctrl)handle;
    if (!phandle)
    {
        return;
    }

    lv_scr_load(phandle->root_obj);
    if (phandle->key_btn)
    {
        lv_group_focus_obj(phandle->key_btn);
    }
}

void divoom_astrootoo_placeholder_clean(void *handle)
{
    Pdivoom_astrootoo_placeholder_ctrl phandle;

    phandle = (Pdivoom_astrootoo_placeholder_ctrl)handle;
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

void divoom_astrootoo_placeholder_pause(void *handle)
{
}

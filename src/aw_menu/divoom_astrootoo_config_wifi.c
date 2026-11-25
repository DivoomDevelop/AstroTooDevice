#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_wifi_con.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_astrootoo_config_wifi.h"

extern uint8 divoom_ble_is_run(void);
int divoom_ble_get_wifi_status(void);

enum
{
    DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WAIT_BLE,
    DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WAIT_WIFI,
    DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WIFI_CONNECTING,
    DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WIFI_OK,
};

#define ASTRO_WIFI_TEXT_W             (DIVOOM_SCREEN_WIDTH - DIVOOM_ASTRO_PAGE_PAD * 2)

typedef struct _divoom_astrootoo_config_wifi_ctrl
{
    divoom_astrootoo_ui_base ui;
    int menu_id;
    int ble_run_flag;
    int count_down_num;
    int wifi_config_started;
    uint8 wifi_status;
    uint8 wifi_disp_id;
    uint8 mac[6];
    lv_timer_t *time_handle;
    lv_obj_t *root_obj;
    lv_obj_t *key_btn;
    lv_obj_t *title_obj;
    lv_obj_t *desc_obj;
    lv_obj_t *hint_obj;
    lv_obj_t *mac_obj;
} divoom_astrootoo_config_wifi_ctrl, *Pdivoom_astrootoo_config_wifi_ctrl;

static void divoom_astrootoo_config_wifi_update_mac(Pdivoom_astrootoo_config_wifi_ctrl phandle)
{
    if (!phandle || !phandle->mac_obj)
    {
        return;
    }

    divoom_main_get_device_mac_addr_ifconfig(phandle->mac, NULL);
    lv_label_set_text_fmt(phandle->mac_obj, "%s:%02x:%02x:%02x:%02x:%02x:%02x",
        divoom_menu_control_get_disp_str("Mac Address"),
        phandle->mac[0], phandle->mac[1], phandle->mac[2],
        phandle->mac[3], phandle->mac[4], phandle->mac[5]);
}

static void divoom_astrootoo_config_wifi_apply_status_ui(Pdivoom_astrootoo_config_wifi_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }

    switch (phandle->wifi_status)
    {
    case DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WAIT_BLE:
    {
        lv_label_set_text(phandle->title_obj, divoom_menu_control_get_disp_str("WaitConfigWifi1"));
        lv_label_set_text(phandle->desc_obj, divoom_menu_control_get_disp_str("WaitWifiConnecting1"));
        lv_label_set_text(phandle->hint_obj, divoom_menu_control_get_disp_str("DownloadApp"));
        lv_obj_clear_flag(phandle->hint_obj, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    case DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WAIT_WIFI:
    {
        lv_label_set_text(phandle->title_obj, divoom_menu_control_get_disp_str("WaitConfigWifi2"));
        lv_label_set_text(phandle->desc_obj, divoom_menu_control_get_disp_str("WaitWifiConnecting2"));
        lv_obj_add_flag(phandle->hint_obj, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    case DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WIFI_CONNECTING:
    {
        lv_label_set_text(phandle->title_obj, divoom_menu_control_get_disp_str("WaitConfigWifi3"));
        lv_label_set_text(phandle->desc_obj, divoom_menu_control_get_disp_str("WaitWifiConnecting3"));
        lv_obj_add_flag(phandle->hint_obj, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    case DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WIFI_OK:
    {
        lv_label_set_text(phandle->title_obj, divoom_menu_control_get_disp_str("WaitConfigWifi4"));
        lv_label_set_text(phandle->desc_obj, divoom_menu_control_get_disp_str("WaitWifiConnecting4"));
        lv_obj_add_flag(phandle->hint_obj, LV_OBJ_FLAG_HIDDEN);
        phandle->count_down_num = 2;
        break;
    }
    default:
    {
        break;
    }
    }
    divoom_astrootoo_config_wifi_update_mac(phandle);
}

static void divoom_astrootoo_config_wifi_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_config_wifi_ctrl phandle;
    uint8_t key_code;

    phandle = (Pdivoom_astrootoo_config_wifi_ctrl)lv_event_get_user_data(e);
    if (!phandle || lv_event_get_code(e) != LV_EVENT_KEY)
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
        if (phandle->wifi_status == DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WIFI_OK)
        {
            divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_CONFIG_NET_WORK_OVER);
            divoom_main_set_wifi_config_status(1);
        }
        else
        {
            /* 仅投递事件，由 divoom_main 统一 return_pre，避免与同步 return 双重弹栈 */
            divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_CONFIG_NET_WORK_CANCLE);
        }
    }
}

static void divoom_astrootoo_config_wifi_time_cb(lv_timer_t *timer)
{
    int cur_wifi_status;
    Pdivoom_astrootoo_config_wifi_ctrl phandle;

    phandle = (Pdivoom_astrootoo_config_wifi_ctrl)lv_timer_get_user_data(timer);
    if (!phandle)
    {
        return;
    }

    if (!phandle->wifi_config_started)
    {
        if (!divoom_wifi_is_ready())
        {
            return;
        }
        phandle->wifi_config_started = 1;
        divoom_print("[WIFI_CFG] UI enter config page, set_config(1)\n");
        divoom_wifi_set_config(1);
    }

    if (phandle->ble_run_flag == 0)
    {
        phandle->ble_run_flag = divoom_ble_is_run();
    }
    if (phandle->ble_run_flag)
    {
        cur_wifi_status = divoom_ble_get_wifi_status();
    }
    else
    {
        cur_wifi_status = DIVOOM_WIFI_NOT_CONFIG;
    }

    if (cur_wifi_status == DIVOOM_WIFI_NOT_CONFIG ||
        cur_wifi_status == DIVOOM_WIFI_NO_SSID ||
        cur_wifi_status == DIVOOM_WIFI_OTHER_ERR)
    {
        cur_wifi_status = DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WAIT_BLE;
    }
    else if (cur_wifi_status == DIVOOM_WIFI_CONNECTING)
    {
        cur_wifi_status = DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WAIT_WIFI;
    }
    else if (cur_wifi_status == DIVOOM_WIFI_CONNECTED)
    {
        cur_wifi_status = DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WIFI_CONNECTING;
    }
    else if (cur_wifi_status == DIVOOM_WIFI_CONNECTED_SER)
    {
        cur_wifi_status = DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WIFI_OK;
    }
    else
    {
        cur_wifi_status = phandle->wifi_status;
    }

    if (cur_wifi_status != phandle->wifi_status)
    {
        phandle->wifi_status = (uint8)cur_wifi_status;
        divoom_astrootoo_config_wifi_apply_status_ui(phandle);
    }

    divoom_astrootoo_config_wifi_update_mac(phandle);

    if (phandle->wifi_status == DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WIFI_OK)
    {
        if (phandle->count_down_num == 0)
        {
            divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_CONFIG_NET_WORK_OVER);
            divoom_main_set_wifi_config_status(1);
        }
        else
        {
            phandle->count_down_num--;
        }
    }
}

void *divoom_astrootoo_config_wifi_init(void *user_data, int menu_id)
{
    int config_flag;
    Pdivoom_astrootoo_config_wifi_ctrl phandle;

    config_flag = 1;
    if (user_data != NULL)
    {
        config_flag = *(int *)user_data;
        divoom_system_free(user_data);
    }
    (void)config_flag;

    phandle = divoom_system_malloc_out(sizeof(divoom_astrootoo_config_wifi_ctrl));
    if (!phandle)
    {
        return NULL;
    }

    phandle->menu_id = menu_id;
    phandle->ble_run_flag = 0;
    phandle->wifi_disp_id = 0;
    phandle->wifi_config_started = 0;
    phandle->wifi_status = DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WAIT_BLE;
    phandle->count_down_num = 0;

    phandle->root_obj = divoom_astrootoo_ui_create_root(&phandle->ui);
    divoom_astrootoo_ui_create_header(phandle->root_obj, &phandle->ui, "Wi-Fi", phandle->menu_id);

    phandle->title_obj = lv_label_create(phandle->root_obj);
    lv_obj_set_width(phandle->title_obj, ASTRO_WIFI_TEXT_W);
    lv_label_set_long_mode(phandle->title_obj, LV_LABEL_LONG_WRAP);
    lv_label_set_text(phandle->title_obj, divoom_menu_control_get_disp_str("WaitConfigWifi1"));
    lv_obj_set_style_text_color(phandle->title_obj, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_align(phandle->title_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(phandle->title_obj, LV_ALIGN_CENTER, 0, -48);

    phandle->desc_obj = lv_label_create(phandle->root_obj);
    lv_obj_set_width(phandle->desc_obj, ASTRO_WIFI_TEXT_W);
    lv_label_set_long_mode(phandle->desc_obj, LV_LABEL_LONG_WRAP);
    lv_label_set_text(phandle->desc_obj, divoom_menu_control_get_disp_str("WaitWifiConnecting1"));
    lv_obj_set_style_text_color(phandle->desc_obj, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_align(phandle->desc_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(phandle->desc_obj, LV_ALIGN_CENTER, 0, 16);

    phandle->hint_obj = lv_label_create(phandle->root_obj);
    lv_obj_set_width(phandle->hint_obj, ASTRO_WIFI_TEXT_W);
    lv_label_set_long_mode(phandle->hint_obj, LV_LABEL_LONG_WRAP);
    lv_label_set_text(phandle->hint_obj, divoom_menu_control_get_disp_str("DownloadApp"));
    lv_obj_set_style_text_color(phandle->hint_obj, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_align(phandle->hint_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(phandle->hint_obj, LV_ALIGN_CENTER, 0, 72);

    phandle->mac_obj = lv_label_create(phandle->root_obj);
    lv_obj_set_width(phandle->mac_obj, ASTRO_WIFI_TEXT_W);
    lv_obj_set_style_text_color(phandle->mac_obj, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_align(phandle->mac_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(phandle->mac_obj, LV_ALIGN_BOTTOM_MID, 0, -12);

    phandle->key_btn = divoom_astrootoo_ui_create_key_capture_btn(phandle->root_obj, &phandle->ui,
        divoom_astrootoo_config_wifi_key_cb, phandle);
    lv_obj_move_foreground(phandle->key_btn);
    {
        lv_group_t *g = lv_group_get_default();

        if (g)
        {
            lv_group_remove_all_objs(g);
        }
        lv_group_add_obj(lv_group_get_default(), phandle->key_btn);
    }

    divoom_astrootoo_config_wifi_apply_status_ui(phandle);
    phandle->time_handle = lv_timer_create(divoom_astrootoo_config_wifi_time_cb, 500, phandle);

    return phandle;
}

void divoom_astrootoo_config_wifi_disp(void *handle, void *active_obj)
{
    Pdivoom_astrootoo_config_wifi_ctrl phandle;

    (void)active_obj;
    phandle = (Pdivoom_astrootoo_config_wifi_ctrl)handle;
    if (!phandle)
    {
        return;
    }

    lv_scr_load(phandle->root_obj);
    if (phandle->key_btn)
    {
        lv_group_t *g = lv_group_get_default();

        if (g)
        {
            lv_group_remove_all_objs(g);
            lv_group_add_obj(g, phandle->key_btn);
        }
        lv_group_focus_obj(phandle->key_btn);
    }
}

void divoom_astrootoo_config_wifi_pause(void *handle)
{
    Pdivoom_astrootoo_config_wifi_ctrl phandle;
    lv_group_t *g;

    phandle = (Pdivoom_astrootoo_config_wifi_ctrl)handle;
    if (!phandle || !phandle->key_btn)
    {
        return;
    }
    g = lv_group_get_default();
    if (g)
    {
        lv_group_remove_obj(phandle->key_btn);
    }
}

void divoom_astrootoo_config_wifi_exit(void *handle)
{
    Pdivoom_astrootoo_config_wifi_ctrl phandle;

    phandle = (Pdivoom_astrootoo_config_wifi_ctrl)handle;
    if (!phandle)
    {
        return;
    }

    if (phandle->time_handle)
    {
        lv_timer_del(phandle->time_handle);
    }
    if (phandle->root_obj)
    {
        lv_obj_del(phandle->root_obj);
    }
    divoom_astrootoo_ui_base_reset_styles(&phandle->ui);
    divoom_system_free(phandle);
    divoom_wifi_set_config(0);
}

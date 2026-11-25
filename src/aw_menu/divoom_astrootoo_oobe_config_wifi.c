#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_wifi_con.h"
#include "divoom_main.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_astrootoo_oobe_config_wifi.h"
#include "divoom_lvgl_play_gif_file.h"
#include "divoom_pixel_format.h"

extern uint8 divoom_ble_is_run(void);
int divoom_ble_get_wifi_status(void);
void divoom_ble_set_wifi_status(int status);

enum
{
    DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WAIT_BLE,
    DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WAIT_WIFI,
    DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WIFI_CONNECTING,
    DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WIFI_OK,
};

enum
{
    DIVOOM_ASTRO_OOBE_PHASE_MAIN = 0,
    DIVOOM_ASTRO_OOBE_PHASE_SKIPPED,
};

#define ASTRO_OOBE_UI_DIR               "53/"
#define ASTRO_OOBE_SKIP_GLOBE           ASTRO_OOBE_UI_DIR "oobe_skip_globe.png"
#define ASTRO_OOBE_SKIP_BT              ASTRO_OOBE_UI_DIR "oobe_skip_bt.png"
#define ASTRO_OOBE_SKIP_WIFI            ASTRO_OOBE_UI_DIR "oobe_skip_wifi.png"
#define ASTRO_OOBE_WIFI_SATELLITE_ICON  ASTRO_OOBE_UI_DIR "oobe_wifi_satellite_icon.png"
#define ASTRO_OOBE_ACTIVATING_GLOBE_GRAY ASTRO_OOBE_UI_DIR "oobe_activating_globe_gray.png"
#define ASTRO_OOBE_SUCCESS_GLOBE        ASTRO_OOBE_UI_DIR "oobe_success_globe.png"
#define ASTRO_OOBE_SUCCESS_CHECK        ASTRO_OOBE_UI_DIR "oobe_success_check.png"

#define ASTRO_OOBE_WAIT_PAIRING_GIF     "/usr/share/divoom_app/ui/53/oobe_wait_pairing_anim.gif"
#define ASTRO_OOBE_WIFI_SIGNAL_GIF      "/usr/share/divoom_app/ui/53/oobe_wifi_signal_anim.gif"
#define ASTRO_OOBE_ACTIVATING_PROGRESS_GIF "/usr/share/divoom_app/ui/53/oobe_activating_progress.gif"

#define OOBE_TOP_Y                      22
#define OOBE_BADGE_SIZE                 32
#define OOBE_BADGE_X                    22
#define OOBE_BADGE_BG                   0xFBF6F4
#define OOBE_BADGE_BORDER               0xF8B634
#define OOBE_BADGE_INK                  0xF8B634
#define OOBE_STEP_INK                   0x3D3D3D
#define OOBE_SKIP_INK                   0x9A938B
#define OOBE_SUB_INK                    0x6D6B6A
#define OOBE_HINT_INK                   0x9A938B
#define OOBE_MAC_INK                    0xC7C7C7
#define OOBE_SUBTITLE_Y                 400
#define OOBE_SUCCESS_HINT_Y             384
#define OOBE_PAIRING_HINT_BOTTOM        40
#define OOBE_MAC_BOTTOM_PAD             12
#define OOBE_TOP_BAR_W                  (DIVOOM_SCREEN_WIDTH - OOBE_BADGE_X - 16)
#define OOBE_TOP_BAR_SKIP_MAX_W         220
#define OOBE_TOP_BAR_COL_PAD            8
#define OOBE_FONT_MENU                  24
#define OOBE_FONT_BODY                  16
#define OOBE_FONT_MAC                   16
#define OOBE_TEXT_W                     (DIVOOM_SCREEN_WIDTH - 32)
#define OOBE_WIFI_CONFIG_FLAG_ADD_NETWORK  2

#define OOBE_WAIT_PAIRING_X             90
#define OOBE_WAIT_PAIRING_Y             172
#define OOBE_WAIT_PAIRING_W             298
#define OOBE_WAIT_PAIRING_H             97

#define OOBE_WIFI_SIGNAL_X              162
#define OOBE_WIFI_SIGNAL_Y              143
#define OOBE_WIFI_SIGNAL_W              156
#define OOBE_WIFI_SIGNAL_H              123

#define OOBE_WIFI_SATELLITE_X           190
#define OOBE_WIFI_SATELLITE_Y           277
#define OOBE_WIFI_SATELLITE_W           100
#define OOBE_WIFI_SATELLITE_H           60

#define OOBE_ACTIVATING_GLOBE_X         78
#define OOBE_ACTIVATING_GLOBE_Y         89
#define OOBE_ACTIVATING_GLOBE_W         324
#define OOBE_ACTIVATING_GLOBE_H         167

#define OOBE_PROGRESS_X                 108
#define OOBE_PROGRESS_Y                 308
#define OOBE_PROGRESS_W                 266
#define OOBE_PROGRESS_H                 26

#define OOBE_SUCCESS_GLOBE_X            78
#define OOBE_SUCCESS_GLOBE_Y            89
#define OOBE_SUCCESS_GLOBE_W          324
#define OOBE_SUCCESS_GLOBE_H          167
#define OOBE_SUCCESS_CHECK_X            182
#define OOBE_SUCCESS_CHECK_Y            292
#define OOBE_SUCCESS_CHECK_W            116
#define OOBE_SUCCESS_CHECK_H            57

/* O5 跳过 Wi-Fi：skip-globe 162×84；顶栏 y=16+16+4-8+4（FlowtooOobe.module.css） */
#define OOBE_SKIP_GLOBE_W               162
#define OOBE_SKIP_GLOBE_H               84
#define OOBE_SKIP_ROW_W                 448
#define OOBE_SKIP_TEXT_COL_W            (OOBE_SKIP_ROW_W - 56)
#define OOBE_SKIP_DIVIDER_H             2
#define OOBE_SKIP_CONFIRM_W             448
#define OOBE_SKIP_CONFIRM_H             48
#define OOBE_SKIP_CONFIRM_BOTTOM        24

#define OOBE_ANIM_BUF_BYTES             (OOBE_WAIT_PAIRING_W * OOBE_WAIT_PAIRING_H * 3)

typedef struct _divoom_astrootoo_oobe_config_wifi_ctrl
{
    divoom_astrootoo_ui_base ui;
    int menu_id;
    int config_flag;
    int ble_run_flag;
    int count_down_num;
    int wifi_config_started;
    uint8 add_network_mode;
    uint8 wifi_status;
    uint8 oobe_phase;
    uint8 gif_started;
    uint8 mac[6];
    lv_timer_t *time_handle;
    lv_obj_t *root_obj;
    lv_obj_t *key_btn;
    lv_obj_t *top_bar;
    lv_obj_t *badge_lbl;
    lv_obj_t *step_lbl;
    lv_obj_t *skip_lbl;
    lv_obj_t *panel_wait_ble;
    lv_obj_t *panel_wait_wifi;
    lv_obj_t *panel_connecting;
    lv_obj_t *panel_success;
    lv_obj_t *panel_skip;
    lv_obj_t *mac_obj;
    lv_obj_t *anim_img;
    lv_obj_t *success_ready_line2;
    lv_img_dsc_t anim_dsc;
    uint8 *anim_buf;
    void *active_gif_handle;
} divoom_astrootoo_oobe_config_wifi_ctrl, *Pdivoom_astrootoo_oobe_config_wifi_ctrl;

static void divoom_astrootoo_oobe_config_wifi_gif_stop(Pdivoom_astrootoo_oobe_config_wifi_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }

    if (phandle->active_gif_handle)
    {
        divoom_lvgl_play_gif_file_destory_handle_reuse_image_buf(phandle->active_gif_handle, 0);
        phandle->active_gif_handle = NULL;
    }
    if (phandle->anim_img)
    {
        lv_obj_add_flag(phandle->anim_img, LV_OBJ_FLAG_HIDDEN);
    }
}

static void divoom_astrootoo_oobe_config_wifi_gif_start(Pdivoom_astrootoo_oobe_config_wifi_ctrl phandle,
    lv_obj_t *panel, const char *path, lv_coord_t x, lv_coord_t y, int w, int h)
{
    if (!phandle || !panel || !phandle->anim_img || !phandle->anim_buf || !path)
    {
        return;
    }

    divoom_astrootoo_oobe_config_wifi_gif_stop(phandle);
    if (divoom_app_com_check_file_exist(path) == 0)
    {
        return;
    }

    lv_obj_set_parent(phandle->anim_img, panel);
    lv_obj_set_pos(phandle->anim_img, x, y);
    lv_obj_set_size(phandle->anim_img, w, h);
    lv_obj_clear_flag(phandle->anim_img, LV_OBJ_FLAG_HIDDEN);

    divoom_lv_img_dsc_init_fmt(&phandle->anim_dsc, w, h,
        LV_COLOR_FORMAT_RGB565A8, phandle->anim_buf);
    phandle->active_gif_handle = divoom_lvgl_play_gif_file_create_handle_reuse_image_buf((char *)path,
        phandle->anim_img, &phandle->anim_dsc, w, h, 0, 0);
    if (phandle->active_gif_handle)
    {
        divoom_lvgl_play_gif_file_start_play(phandle->active_gif_handle);
    }
    lv_img_set_src(phandle->anim_img, &phandle->anim_dsc);
    lv_obj_invalidate(phandle->anim_img);
}

static void divoom_astrootoo_oobe_config_wifi_sync_gif(Pdivoom_astrootoo_oobe_config_wifi_ctrl phandle)
{
    if (!phandle || !phandle->gif_started
        || phandle->oobe_phase == DIVOOM_ASTRO_OOBE_PHASE_SKIPPED)
    {
        divoom_astrootoo_oobe_config_wifi_gif_stop(phandle);
        return;
    }

    switch (phandle->wifi_status)
    {
    case DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WAIT_BLE:
        divoom_astrootoo_oobe_config_wifi_gif_start(phandle, phandle->panel_wait_ble,
            ASTRO_OOBE_WAIT_PAIRING_GIF, OOBE_WAIT_PAIRING_X, OOBE_WAIT_PAIRING_Y,
            OOBE_WAIT_PAIRING_W, OOBE_WAIT_PAIRING_H);
        break;
    case DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WAIT_WIFI:
        divoom_astrootoo_oobe_config_wifi_gif_start(phandle, phandle->panel_wait_wifi,
            ASTRO_OOBE_WIFI_SIGNAL_GIF, OOBE_WIFI_SIGNAL_X, OOBE_WIFI_SIGNAL_Y,
            OOBE_WIFI_SIGNAL_W, OOBE_WIFI_SIGNAL_H);
        break;
    case DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WIFI_CONNECTING:
        divoom_astrootoo_oobe_config_wifi_gif_start(phandle, phandle->panel_connecting,
            ASTRO_OOBE_ACTIVATING_PROGRESS_GIF, OOBE_PROGRESS_X, OOBE_PROGRESS_Y,
            OOBE_PROGRESS_W, OOBE_PROGRESS_H);
        break;
    default:
        divoom_astrootoo_oobe_config_wifi_gif_stop(phandle);
        break;
    }
}

static void divoom_astrootoo_oobe_config_wifi_raise_chrome(Pdivoom_astrootoo_oobe_config_wifi_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }

    if (phandle->oobe_phase != DIVOOM_ASTRO_OOBE_PHASE_SKIPPED && phandle->top_bar)
    {
        lv_obj_move_foreground(phandle->top_bar);
    }
    if (phandle->key_btn)
    {
        lv_obj_move_foreground(phandle->key_btn);
    }
    if (phandle->mac_obj)
    {
        lv_obj_move_foreground(phandle->mac_obj);
    }
}

static void divoom_astrootoo_oobe_config_wifi_set_panel_visible(Pdivoom_astrootoo_oobe_config_wifi_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }

    if (phandle->oobe_phase == DIVOOM_ASTRO_OOBE_PHASE_SKIPPED)
    {
        lv_obj_add_flag(phandle->top_bar, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->panel_wait_ble, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->panel_wait_wifi, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->panel_connecting, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->panel_success, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(phandle->panel_skip, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_clear_flag(phandle->top_bar, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(phandle->panel_skip, LV_OBJ_FLAG_HIDDEN);

    if (phandle->wifi_status == DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WAIT_BLE)
    {
        lv_obj_clear_flag(phandle->panel_wait_ble, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->panel_wait_wifi, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->panel_connecting, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->panel_success, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(phandle->skip_lbl, LV_OBJ_FLAG_HIDDEN);
    }
    else if (phandle->wifi_status == DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WAIT_WIFI)
    {
        lv_obj_add_flag(phandle->panel_wait_ble, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(phandle->panel_wait_wifi, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->panel_connecting, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->panel_success, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(phandle->skip_lbl, LV_OBJ_FLAG_HIDDEN);
    }
    else if (phandle->wifi_status == DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WIFI_CONNECTING)
    {
        lv_obj_add_flag(phandle->panel_wait_ble, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->panel_wait_wifi, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(phandle->panel_connecting, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->panel_success, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->skip_lbl, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(phandle->panel_wait_ble, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->panel_wait_wifi, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->panel_connecting, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(phandle->panel_success, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->skip_lbl, LV_OBJ_FLAG_HIDDEN);
    }
}

static void divoom_astrootoo_oobe_config_wifi_update_mac(Pdivoom_astrootoo_oobe_config_wifi_ctrl phandle)
{
    if (!phandle || !phandle->mac_obj)
    {
        return;
    }

    divoom_main_get_device_mac_addr_ifconfig(phandle->mac, NULL);
    lv_label_set_text_fmt(phandle->mac_obj, "%s: %02x:%02x:%02x:%02x:%02x:%02x",
        divoom_menu_control_get_disp_str("Mac Address"),
        phandle->mac[0], phandle->mac[1], phandle->mac[2],
        phandle->mac[3], phandle->mac[4], phandle->mac[5]);
}

static void divoom_astrootoo_oobe_config_wifi_update_mac_layout(Pdivoom_astrootoo_oobe_config_wifi_ctrl phandle)
{
    if (!phandle || !phandle->mac_obj)
    {
        return;
    }

    if (phandle->oobe_phase == DIVOOM_ASTRO_OOBE_PHASE_SKIPPED
        || phandle->wifi_status == DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WIFI_OK
        || phandle->wifi_status != DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WAIT_BLE)
    {
        lv_obj_add_flag(phandle->mac_obj, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_clear_flag(phandle->mac_obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(phandle->mac_obj, LV_ALIGN_BOTTOM_MID, 0, -OOBE_MAC_BOTTOM_PAD);
    divoom_astrootoo_oobe_config_wifi_update_mac(phandle);
}

static void divoom_astrootoo_oobe_config_wifi_update_top_bar_layout(
    Pdivoom_astrootoo_oobe_config_wifi_ctrl phandle);

static void divoom_astrootoo_oobe_config_wifi_apply_status_ui(Pdivoom_astrootoo_oobe_config_wifi_ctrl phandle)
{
    const char *line2;

    if (!phandle)
    {
        return;
    }

    switch (phandle->wifi_status)
    {
    case DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WAIT_BLE:
        lv_label_set_text(phandle->badge_lbl, "2");
        lv_label_set_text(phandle->step_lbl, divoom_menu_control_get_disp_str("WaitConfigWifi1"));
        phandle->count_down_num = 0;
        break;
    case DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WAIT_WIFI:
        lv_label_set_text(phandle->badge_lbl, "3");
        lv_label_set_text(phandle->step_lbl, divoom_menu_control_get_disp_str("WaitConfigWifi2"));
        phandle->count_down_num = 0;
        break;
    case DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WIFI_CONNECTING:
        lv_label_set_text(phandle->badge_lbl, "4");
        lv_label_set_text(phandle->step_lbl, divoom_menu_control_get_disp_str("WaitConfigWifi3"));
        phandle->count_down_num = 0;
        break;
    case DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WIFI_OK:
        lv_label_set_text(phandle->badge_lbl, "5");
        lv_label_set_text(phandle->step_lbl, divoom_menu_control_get_disp_str("OobeSuccessStepName"));
        phandle->count_down_num = 2;
        break;
    default:
        break;
    }

    if (phandle->success_ready_line2)
    {
        line2 = divoom_menu_control_get_disp_str("OobeSuccessReadyLine2");
        if (line2 != NULL && line2[0] != '\0')
        {
            lv_label_set_text(phandle->success_ready_line2, line2);
            lv_obj_clear_flag(phandle->success_ready_line2, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(phandle->success_ready_line2, LV_OBJ_FLAG_HIDDEN);
        }
    }

    divoom_astrootoo_oobe_config_wifi_set_panel_visible(phandle);
    divoom_astrootoo_oobe_config_wifi_update_top_bar_layout(phandle);
    divoom_astrootoo_oobe_config_wifi_update_mac_layout(phandle);
    if (phandle->gif_started)
    {
        divoom_astrootoo_oobe_config_wifi_sync_gif(phandle);
    }
    divoom_astrootoo_oobe_config_wifi_raise_chrome(phandle);
}

static void divoom_astrootoo_oobe_config_wifi_show_skip(Pdivoom_astrootoo_oobe_config_wifi_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }
    phandle->oobe_phase = DIVOOM_ASTRO_OOBE_PHASE_SKIPPED;
    if (phandle->mac_obj)
    {
        lv_obj_add_flag(phandle->mac_obj, LV_OBJ_FLAG_HIDDEN);
    }
    divoom_astrootoo_oobe_config_wifi_set_panel_visible(phandle);
    divoom_astrootoo_oobe_config_wifi_gif_stop(phandle);
}

static void divoom_astrootoo_oobe_config_wifi_finish(Pdivoom_astrootoo_oobe_config_wifi_ctrl phandle)
{
    (void)phandle;
    divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_CONFIG_NET_WORK_OVER);
    divoom_main_set_wifi_config_status(1);
}

#if defined(DIVOOM_SIMULATOR)
static void divoom_astrootoo_oobe_config_wifi_sim_advance(Pdivoom_astrootoo_oobe_config_wifi_ctrl phandle)
{
    static const int ble_map[4] =
    {
        DIVOOM_WIFI_NOT_CONFIG,
        DIVOOM_WIFI_CONNECTING,
        DIVOOM_WIFI_CONNECTED,
        DIVOOM_WIFI_CONNECTED_SER,
    };

    phandle->wifi_status = (uint8)((phandle->wifi_status + 1) % 4);
    phandle->ble_run_flag = (phandle->wifi_status > 0) ? 1 : 0;
    divoom_ble_set_wifi_status(ble_map[phandle->wifi_status]);
    divoom_astrootoo_oobe_config_wifi_apply_status_ui(phandle);
}
#endif

static void divoom_astrootoo_oobe_config_wifi_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_oobe_config_wifi_ctrl phandle;
    uint8_t key_code;

    phandle = (Pdivoom_astrootoo_oobe_config_wifi_ctrl)lv_event_get_user_data(e);
    if (!phandle || lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }

    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }

    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    if (phandle->oobe_phase == DIVOOM_ASTRO_OOBE_PHASE_SKIPPED)
    {
        if (divoom_astrootoo_ui_key_is_enter(key_code))
        {
            divoom_astrootoo_oobe_config_wifi_finish(phandle);
        }
        return;
    }

#if defined(DIVOOM_SIMULATOR)
    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        divoom_astrootoo_oobe_config_wifi_sim_advance(phandle);
        return;
    }
#endif

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        if (phandle->wifi_status == DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WIFI_OK)
        {
            divoom_astrootoo_oobe_config_wifi_finish(phandle);
        }
        else if (phandle->wifi_status == DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WAIT_BLE
            || phandle->wifi_status == DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WAIT_WIFI)
        {
            divoom_astrootoo_oobe_config_wifi_show_skip(phandle);
        }
    }
}

static void divoom_astrootoo_oobe_config_wifi_time_cb(lv_timer_t *timer)
{
    int cur_wifi_status;
    Pdivoom_astrootoo_oobe_config_wifi_ctrl phandle;

    phandle = (Pdivoom_astrootoo_oobe_config_wifi_ctrl)lv_timer_get_user_data(timer);
    if (!phandle || phandle->oobe_phase == DIVOOM_ASTRO_OOBE_PHASE_SKIPPED)
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
        divoom_wifi_set_config(phandle->config_flag);
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
        if (phandle->add_network_mode)
        {
            /* 系统 Wi-Fi 添加网络时 App 已经进入配网流程，二维码下载页只用于首次配网。 */
            cur_wifi_status = DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WAIT_WIFI;
        }
        else
        {
            cur_wifi_status = DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WAIT_BLE;
        }
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
        divoom_astrootoo_oobe_config_wifi_apply_status_ui(phandle);
    }

    divoom_astrootoo_oobe_config_wifi_update_mac(phandle);

    if (phandle->wifi_status == DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WIFI_OK)
    {
        if (phandle->count_down_num == 0)
        {
            divoom_astrootoo_oobe_config_wifi_finish(phandle);
        }
        else
        {
            phandle->count_down_num--;
        }
    }
}

static void divoom_astrootoo_oobe_config_wifi_style_top_bar_label(lv_obj_t *lbl,
    lv_coord_t max_w, lv_text_align_t align)
{
    if (!lbl)
    {
        return;
    }

    lv_obj_set_width(lbl, max_w);
    lv_obj_set_height(lbl, LV_SIZE_CONTENT);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(lbl, align, 0);
    lv_obj_add_flag(lbl, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
}

static lv_coord_t divoom_astrootoo_oobe_config_wifi_step_max_w(uint8 skip_visible)
{
    lv_coord_t step_w;

    step_w = OOBE_TOP_BAR_W - OOBE_BADGE_SIZE - OOBE_TOP_BAR_COL_PAD;
    if (skip_visible)
    {
        step_w -= OOBE_TOP_BAR_SKIP_MAX_W;
    }
    return step_w;
}

static void divoom_astrootoo_oobe_config_wifi_update_top_bar_layout(
    Pdivoom_astrootoo_oobe_config_wifi_ctrl phandle)
{
    lv_obj_t *left;
    lv_coord_t bar_h;
    lv_coord_t step_h;
    lv_coord_t skip_h;
    uint8 skip_visible;

    if (!phandle || !phandle->step_lbl || !phandle->skip_lbl || !phandle->top_bar)
    {
        return;
    }

    skip_visible = !lv_obj_has_flag(phandle->skip_lbl, LV_OBJ_FLAG_HIDDEN);
    divoom_astrootoo_oobe_config_wifi_style_top_bar_label(phandle->step_lbl,
        divoom_astrootoo_oobe_config_wifi_step_max_w(skip_visible), LV_TEXT_ALIGN_LEFT);
    if (skip_visible)
    {
        divoom_astrootoo_oobe_config_wifi_style_top_bar_label(phandle->skip_lbl,
            OOBE_TOP_BAR_SKIP_MAX_W, LV_TEXT_ALIGN_RIGHT);
    }

    lv_obj_update_layout(phandle->step_lbl);
    step_h = lv_obj_get_height(phandle->step_lbl);
    skip_h = 0;
    if (skip_visible)
    {
        lv_obj_update_layout(phandle->skip_lbl);
        skip_h = lv_obj_get_height(phandle->skip_lbl);
    }

    bar_h = step_h > skip_h ? step_h : skip_h;
    if (bar_h < OOBE_BADGE_SIZE)
    {
        bar_h = OOBE_BADGE_SIZE;
    }

    left = lv_obj_get_parent(phandle->step_lbl);
    lv_obj_set_height(phandle->top_bar, bar_h);
    if (left)
    {
        lv_obj_set_height(left, bar_h);
    }
}

static lv_obj_t *divoom_astrootoo_oobe_config_wifi_create_wrap_label(lv_obj_t *parent, int menu_id,
    const char *key, lv_color_t color, int font_px, lv_text_align_t align)
{
    lv_obj_t *lbl;
    const char *text;

    text = key ? divoom_menu_control_get_disp_str(key) : "";
    lbl = lv_label_create(parent);
    lv_obj_set_width(lbl, OOBE_TEXT_W);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_label_set_text(lbl, text ? text : "");
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_text_align(lbl, align, 0);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(font_px, menu_id), 0);
    return lbl;
}

static lv_obj_t *divoom_astrootoo_oobe_config_wifi_create_caption(lv_obj_t *parent, int menu_id,
    const char *key, lv_color_t color, int font_px, lv_coord_t y)
{
    lv_obj_t *lbl;

    lbl = lv_label_create(parent);
    lv_obj_set_width(lbl, OOBE_TEXT_W);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_label_set_text(lbl, divoom_menu_control_get_disp_str(key));
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(lbl, divoom_menu_control_get_font_info(font_px, menu_id), 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, y);
    return lbl;
}

static lv_obj_t *divoom_astrootoo_oobe_config_wifi_create_top_bar(lv_obj_t *parent, int menu_id,
    Pdivoom_astrootoo_oobe_config_wifi_ctrl phandle)
{
    lv_obj_t *bar;
    lv_obj_t *left;
    lv_obj_t *badge;
    lv_obj_t *skip_lbl;
    lv_coord_t bar_h;

    bar_h = OOBE_BADGE_SIZE;

    bar = lv_obj_create(parent);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, OOBE_TOP_BAR_W, bar_h);
    lv_obj_set_pos(bar, OOBE_BADGE_X, OOBE_TOP_Y);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(bar, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    left = lv_obj_create(bar);
    lv_obj_remove_style_all(left);
    lv_obj_set_flex_grow(left, 1);
    lv_obj_set_width(left, 1);
    lv_obj_set_height(left, bar_h);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(left, OOBE_TOP_BAR_COL_PAD, 0);
    lv_obj_add_flag(left, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

    badge = lv_obj_create(left);
    lv_obj_remove_style_all(badge);
    lv_obj_set_size(badge, OOBE_BADGE_SIZE, OOBE_BADGE_SIZE);
    lv_obj_set_style_bg_color(badge, lv_color_hex(OOBE_BADGE_BG), 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(badge, lv_color_hex(OOBE_BADGE_BORDER), 0);
    lv_obj_set_style_border_width(badge, 2, 0);
    lv_obj_set_style_radius(badge, 8, 0);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

    phandle->badge_lbl = lv_label_create(badge);
    lv_label_set_text(phandle->badge_lbl, "2");
    lv_obj_set_style_text_color(phandle->badge_lbl, lv_color_hex(OOBE_BADGE_INK), 0);
    lv_obj_set_style_text_font(phandle->badge_lbl, divoom_menu_control_get_font_info(20, menu_id), 0);
    lv_obj_center(phandle->badge_lbl);

    phandle->step_lbl = lv_label_create(left);
    lv_label_set_text(phandle->step_lbl, divoom_menu_control_get_disp_str("WaitConfigWifi1"));
    lv_obj_set_style_text_color(phandle->step_lbl, lv_color_hex(OOBE_STEP_INK), 0);
    lv_obj_set_style_text_font(phandle->step_lbl, divoom_menu_control_get_font_info(OOBE_FONT_MENU, menu_id), 0);

    skip_lbl = lv_label_create(bar);
    lv_label_set_text(skip_lbl, divoom_menu_control_get_disp_str("OobeSkipHint"));
    lv_obj_set_style_text_color(skip_lbl, lv_color_hex(OOBE_SKIP_INK), 0);
    lv_obj_set_style_text_font(skip_lbl, divoom_menu_control_get_font_info(OOBE_FONT_BODY, menu_id), 0);
    phandle->skip_lbl = skip_lbl;

    divoom_astrootoo_oobe_config_wifi_update_top_bar_layout(phandle);

    return bar;
}

static lv_obj_t *divoom_astrootoo_oobe_config_wifi_create_wait_ble_panel(lv_obj_t *parent, int menu_id)
{
    lv_obj_t *panel;
    lv_obj_t *hint_stack;

    panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    hint_stack = lv_obj_create(panel);
    lv_obj_remove_style_all(hint_stack);
    lv_obj_set_width(hint_stack, OOBE_TEXT_W);
    lv_obj_set_height(hint_stack, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(hint_stack, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(hint_stack, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(hint_stack, 8, 0);
    lv_obj_align(hint_stack, LV_ALIGN_BOTTOM_MID, 0, -OOBE_PAIRING_HINT_BOTTOM);
    lv_obj_clear_flag(hint_stack, LV_OBJ_FLAG_SCROLLABLE);

    divoom_astrootoo_oobe_config_wifi_create_wrap_label(hint_stack, menu_id, "WaitWifiConnecting1",
        lv_color_hex(OOBE_SUB_INK), OOBE_FONT_MENU, LV_TEXT_ALIGN_CENTER);
    divoom_astrootoo_oobe_config_wifi_create_wrap_label(hint_stack, menu_id, "OobeAppDownloadHint",
        lv_color_hex(OOBE_HINT_INK), OOBE_FONT_BODY, LV_TEXT_ALIGN_CENTER);

    return panel;
}

static lv_obj_t *divoom_astrootoo_oobe_config_wifi_create_wait_wifi_panel(lv_obj_t *parent, int menu_id)
{
    lv_obj_t *panel;
    lv_obj_t *satellite;

    panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    satellite = lv_img_create(panel);
    divoom_menu_control_lv_img_set_src(satellite, ASTRO_OOBE_WIFI_SATELLITE_ICON);
    lv_obj_set_pos(satellite, OOBE_WIFI_SATELLITE_X, OOBE_WIFI_SATELLITE_Y);
    lv_obj_set_size(satellite, OOBE_WIFI_SATELLITE_W, OOBE_WIFI_SATELLITE_H);

    divoom_astrootoo_oobe_config_wifi_create_caption(panel, menu_id, "WaitWifiConnecting2",
        lv_color_hex(OOBE_SUB_INK), OOBE_FONT_MENU, OOBE_SUBTITLE_Y);

    return panel;
}

static lv_obj_t *divoom_astrootoo_oobe_config_wifi_create_connecting_panel(lv_obj_t *parent, int menu_id)
{
    lv_obj_t *panel;
    lv_obj_t *globe;

    panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    globe = lv_img_create(panel);
    divoom_menu_control_lv_img_set_src(globe, ASTRO_OOBE_ACTIVATING_GLOBE_GRAY);
    lv_obj_set_pos(globe, OOBE_ACTIVATING_GLOBE_X, OOBE_ACTIVATING_GLOBE_Y);
    lv_obj_set_size(globe, OOBE_ACTIVATING_GLOBE_W, OOBE_ACTIVATING_GLOBE_H);

    divoom_astrootoo_oobe_config_wifi_create_caption(panel, menu_id, "WaitWifiConnecting3",
        lv_color_hex(OOBE_SUB_INK), OOBE_FONT_MENU, OOBE_SUBTITLE_Y);

    return panel;
}

static lv_obj_t *divoom_astrootoo_oobe_config_wifi_create_success_panel(lv_obj_t *parent, int menu_id,
    Pdivoom_astrootoo_oobe_config_wifi_ctrl phandle)
{
    lv_obj_t *panel;
    lv_obj_t *globe;
    lv_obj_t *check;

    panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    globe = lv_img_create(panel);
    divoom_menu_control_lv_img_set_src(globe, ASTRO_OOBE_SUCCESS_GLOBE);
    lv_obj_set_pos(globe, OOBE_SUCCESS_GLOBE_X, OOBE_SUCCESS_GLOBE_Y);
    lv_obj_set_size(globe, OOBE_SUCCESS_GLOBE_W, OOBE_SUCCESS_GLOBE_H);

    check = lv_img_create(panel);
    divoom_menu_control_lv_img_set_src(check, ASTRO_OOBE_SUCCESS_CHECK);
    lv_obj_set_pos(check, OOBE_SUCCESS_CHECK_X, OOBE_SUCCESS_CHECK_Y);
    lv_obj_set_size(check, OOBE_SUCCESS_CHECK_W, OOBE_SUCCESS_CHECK_H);

    {
        lv_obj_t *hint_stack;

        hint_stack = lv_obj_create(panel);
        lv_obj_remove_style_all(hint_stack);
        lv_obj_set_width(hint_stack, OOBE_TEXT_W);
        lv_obj_set_height(hint_stack, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(hint_stack, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(hint_stack, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(hint_stack, 2, 0);
        lv_obj_align(hint_stack, LV_ALIGN_TOP_MID, 0, OOBE_SUCCESS_HINT_Y);
        lv_obj_clear_flag(hint_stack, LV_OBJ_FLAG_SCROLLABLE);

        divoom_astrootoo_oobe_config_wifi_create_wrap_label(hint_stack, menu_id, "WaitWifiConnecting4",
            lv_color_hex(OOBE_HINT_INK), OOBE_FONT_MENU, LV_TEXT_ALIGN_CENTER);
        phandle->success_ready_line2 = divoom_astrootoo_oobe_config_wifi_create_wrap_label(hint_stack, menu_id,
            "OobeSuccessReadyLine2", lv_color_hex(OOBE_HINT_INK), OOBE_FONT_MENU, LV_TEXT_ALIGN_CENTER);
    }

    return panel;
}

static lv_obj_t *divoom_astrootoo_oobe_config_wifi_skip_add_desc_line(
    lv_obj_t *parent, int menu_id, const char *key)
{
    lv_obj_t *line;
    const char *text;

    text = divoom_menu_control_get_disp_str(key);
    line = lv_label_create(parent);
    lv_obj_set_width(line, OOBE_SKIP_TEXT_COL_W);
    lv_label_set_long_mode(line, LV_LABEL_LONG_WRAP);
    lv_label_set_text(line, text ? text : "");
    lv_obj_set_style_text_color(line, lv_color_hex(OOBE_HINT_INK), 0);
    lv_obj_set_style_text_font(line, divoom_menu_control_get_font_info(OOBE_FONT_BODY, menu_id), 0);
    return line;
}

static lv_obj_t *divoom_astrootoo_oobe_config_wifi_skip_add_row(lv_obj_t *parent, int menu_id,
    const char *icon_path, const char *title_key, const char *line1_key, const char *line2_key)
{
    lv_obj_t *row;
    lv_obj_t *icon;
    lv_obj_t *text_col;
    lv_obj_t *title;
    lv_obj_t *desc_box;

    row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_width(row, OOBE_SKIP_ROW_W);
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(row, 12, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    icon = lv_img_create(row);
    divoom_menu_control_lv_img_set_src(icon, icon_path);
    lv_obj_set_size(icon, 44, 44);

    text_col = lv_obj_create(row);
    lv_obj_remove_style_all(text_col);
    lv_obj_set_width(text_col, OOBE_SKIP_TEXT_COL_W);
    lv_obj_set_height(text_col, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(text_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(text_col, 2, 0);
    lv_obj_clear_flag(text_col, LV_OBJ_FLAG_SCROLLABLE);

    title = lv_label_create(text_col);
    lv_obj_set_width(title, OOBE_SKIP_TEXT_COL_W);
    lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP);
    lv_label_set_text(title, divoom_menu_control_get_disp_str(title_key));
    lv_obj_set_style_text_color(title, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(OOBE_FONT_MENU, menu_id), 0);

    desc_box = lv_obj_create(text_col);
    lv_obj_remove_style_all(desc_box);
    lv_obj_set_width(desc_box, OOBE_SKIP_TEXT_COL_W);
    lv_obj_set_height(desc_box, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(desc_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(desc_box, 2, 0);
    lv_obj_set_style_pad_top(desc_box, 4, 0);
    lv_obj_clear_flag(desc_box, LV_OBJ_FLAG_SCROLLABLE);

    divoom_astrootoo_oobe_config_wifi_skip_add_desc_line(desc_box, menu_id, line1_key);
    divoom_astrootoo_oobe_config_wifi_skip_add_desc_line(desc_box, menu_id, line2_key);

    return row;
}

static lv_obj_t *divoom_astrootoo_oobe_config_wifi_skip_add_divider(lv_obj_t *parent)
{
    lv_obj_t *divider;

    divider = lv_obj_create(parent);
    lv_obj_remove_style_all(divider);
    lv_obj_set_width(divider, OOBE_SKIP_ROW_W);
    lv_obj_set_height(divider, OOBE_SKIP_DIVIDER_H);
    lv_obj_set_style_bg_color(divider, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
    lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);
    lv_obj_clear_flag(divider, LV_OBJ_FLAG_SCROLLABLE);
    return divider;
}

static lv_obj_t *divoom_astrootoo_oobe_config_wifi_create_skip_panel(lv_obj_t *parent, int menu_id)
{
    lv_obj_t *panel;
    lv_obj_t *header;
    lv_obj_t *globe;
    lv_obj_t *title;
    lv_obj_t *info_list;
    lv_obj_t *confirm;
    lv_obj_t *confirm_lbl;

    panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_bottom(panel, OOBE_SKIP_CONFIRM_BOTTOM, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    header = lv_obj_create(panel);
    lv_obj_remove_style_all(header);
    lv_obj_set_width(header, OOBE_SKIP_ROW_W);
    lv_obj_set_height(header, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_top(header, 24, 0);
    lv_obj_set_style_pad_bottom(header, 8, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    globe = lv_img_create(header);
    divoom_menu_control_lv_img_set_src(globe, ASTRO_OOBE_SKIP_GLOBE);
    lv_obj_set_size(globe, OOBE_SKIP_GLOBE_W, OOBE_SKIP_GLOBE_H);

    title = lv_label_create(header);
    lv_obj_set_width(title, OOBE_TEXT_W);
    lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP);
    lv_label_set_text(title, divoom_menu_control_get_disp_str("OobeWifiSkippedTitle"));
    lv_obj_set_style_text_color(title, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(title, divoom_menu_control_get_font_info(OOBE_FONT_MENU, menu_id), 0);

    info_list = lv_obj_create(panel);
    lv_obj_remove_style_all(info_list);
    lv_obj_set_width(info_list, OOBE_SKIP_ROW_W);
    lv_obj_set_flex_grow(info_list, 1);
    lv_obj_set_height(info_list, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(info_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(info_list, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(info_list, 6, 0);
    lv_obj_set_style_pad_ver(info_list, 14, 0);
    lv_obj_clear_flag(info_list, LV_OBJ_FLAG_SCROLLABLE);

    divoom_astrootoo_oobe_config_wifi_skip_add_row(info_list, menu_id, ASTRO_OOBE_SKIP_BT,
        "OobeWifiSkippedBtTitle", "OobeWifiSkippedBtDescLine1", "OobeWifiSkippedBtDescLine2");
    divoom_astrootoo_oobe_config_wifi_skip_add_divider(info_list);
    divoom_astrootoo_oobe_config_wifi_skip_add_row(info_list, menu_id, ASTRO_OOBE_SKIP_WIFI,
        "OobeWifiSkippedWifiTitle", "OobeWifiSkippedWifiDescLine1", "OobeWifiSkippedWifiDescLine2");

    confirm = lv_obj_create(panel);
    lv_obj_remove_style_all(confirm);
    lv_obj_set_size(confirm, OOBE_SKIP_CONFIRM_W, OOBE_SKIP_CONFIRM_H);
    lv_obj_set_style_bg_color(confirm, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
    lv_obj_set_style_bg_opa(confirm, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(confirm, DIVOOM_ASTRO_RADIUS_MD, 0);
    lv_obj_set_style_pad_all(confirm, 0, 0);
    lv_obj_set_style_margin_top(confirm, 8, 0);
    lv_obj_clear_flag(confirm, LV_OBJ_FLAG_SCROLLABLE);

    confirm_lbl = lv_label_create(confirm);
    lv_label_set_text(confirm_lbl, divoom_menu_control_get_disp_str("OobeWifiSkippedConfirm"));
    lv_obj_set_style_text_color(confirm_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(confirm_lbl, divoom_menu_control_get_font_info(OOBE_FONT_MENU, menu_id), 0);
    lv_obj_center(confirm_lbl);

    return panel;
}

void *divoom_astrootoo_oobe_config_wifi_init(void *user_data, int menu_id)
{
    int config_flag;
    Pdivoom_astrootoo_oobe_config_wifi_ctrl phandle;

    config_flag = 1;
    if (user_data != NULL)
    {
        config_flag = *(int *)user_data;
        divoom_system_free(user_data);
    }
    phandle = divoom_system_malloc_out(sizeof(divoom_astrootoo_oobe_config_wifi_ctrl));
    if (!phandle)
    {
        return NULL;
    }

    memset(phandle, 0, sizeof(*phandle));
    /* 切图在 ui/53/，字体资源按 CONFIG_WIFI menu_id 解析 */
    (void)menu_id;
    phandle->menu_id = DIVOOM_MENU_CONTROL_ID_CONFIG_WIFI;
    phandle->config_flag = config_flag;
    phandle->wifi_config_started = 0;
    phandle->add_network_mode = (config_flag == OOBE_WIFI_CONFIG_FLAG_ADD_NETWORK) ? 1 : 0;
    phandle->wifi_status = phandle->add_network_mode ?
        DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WAIT_WIFI :
        DIVOOM_ASTRO_CONFIG_WIFI_STATUS_WAIT_BLE;
    phandle->oobe_phase = DIVOOM_ASTRO_OOBE_PHASE_MAIN;

    phandle->root_obj = divoom_astrootoo_ui_create_root(&phandle->ui);

    phandle->anim_buf = (uint8 *)divoom_system_malloc(OOBE_ANIM_BUF_BYTES);
    phandle->anim_img = lv_img_create(phandle->root_obj);
    lv_obj_add_flag(phandle->anim_img, LV_OBJ_FLAG_HIDDEN);

    phandle->mac_obj = lv_label_create(phandle->root_obj);
    lv_obj_set_width(phandle->mac_obj, OOBE_TEXT_W);
    lv_obj_set_style_text_color(phandle->mac_obj, lv_color_hex(OOBE_MAC_INK), 0);
    lv_obj_set_style_text_align(phandle->mac_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(phandle->mac_obj, divoom_menu_control_get_font_info(OOBE_FONT_MAC, menu_id), 0);
    lv_obj_add_flag(phandle->mac_obj, LV_OBJ_FLAG_HIDDEN);

    phandle->top_bar = divoom_astrootoo_oobe_config_wifi_create_top_bar(phandle->root_obj, menu_id, phandle);
    divoom_astrootoo_oobe_config_wifi_update_top_bar_layout(phandle);
    phandle->panel_wait_ble = divoom_astrootoo_oobe_config_wifi_create_wait_ble_panel(
        phandle->root_obj, menu_id);
    phandle->panel_wait_wifi = divoom_astrootoo_oobe_config_wifi_create_wait_wifi_panel(
        phandle->root_obj, menu_id);
    phandle->panel_connecting = divoom_astrootoo_oobe_config_wifi_create_connecting_panel(
        phandle->root_obj, menu_id);
    phandle->panel_success = divoom_astrootoo_oobe_config_wifi_create_success_panel(
        phandle->root_obj, menu_id, phandle);
    phandle->panel_skip = divoom_astrootoo_oobe_config_wifi_create_skip_panel(phandle->root_obj, menu_id);
    lv_obj_add_flag(phandle->panel_skip, LV_OBJ_FLAG_HIDDEN);

    phandle->key_btn = divoom_astrootoo_ui_create_key_capture_btn(phandle->root_obj, &phandle->ui,
        divoom_astrootoo_oobe_config_wifi_key_cb, phandle);
    lv_obj_move_foreground(phandle->key_btn);
    lv_group_add_obj(lv_group_get_default(), phandle->key_btn);

    divoom_astrootoo_oobe_config_wifi_apply_status_ui(phandle);
    phandle->time_handle = lv_timer_create(divoom_astrootoo_oobe_config_wifi_time_cb, 500, phandle);

    return phandle;
}

void divoom_astrootoo_oobe_config_wifi_disp(void *handle, void *active_obj)
{
    Pdivoom_astrootoo_oobe_config_wifi_ctrl phandle;

    (void)active_obj;
    phandle = (Pdivoom_astrootoo_oobe_config_wifi_ctrl)handle;
    if (!phandle)
    {
        return;
    }

    lv_scr_load(phandle->root_obj);
    phandle->gif_started = 1;
    divoom_astrootoo_oobe_config_wifi_sync_gif(phandle);
    divoom_astrootoo_oobe_config_wifi_raise_chrome(phandle);
    if (phandle->key_btn)
    {
        lv_group_focus_obj(phandle->key_btn);
    }
}

void divoom_astrootoo_oobe_config_wifi_pause(void *handle)
{
    (void)handle;
}

void divoom_astrootoo_oobe_config_wifi_exit(void *handle)
{
    Pdivoom_astrootoo_oobe_config_wifi_ctrl phandle;

    phandle = (Pdivoom_astrootoo_oobe_config_wifi_ctrl)handle;
    if (!phandle)
    {
        return;
    }

    divoom_astrootoo_oobe_config_wifi_gif_stop(phandle);
    if (phandle->time_handle)
    {
        lv_timer_del(phandle->time_handle);
    }
    if (phandle->root_obj)
    {
        lv_obj_del(phandle->root_obj);
    }
    if (phandle->anim_buf)
    {
        divoom_system_free(phandle->anim_buf);
    }
    divoom_astrootoo_ui_base_reset_styles(&phandle->ui);
    divoom_system_free(phandle);
    divoom_wifi_set_config(0);
}

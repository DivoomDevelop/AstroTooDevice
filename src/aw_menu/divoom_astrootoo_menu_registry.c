#include "divoom.h"
#include "divoom_menu_control.h"
#include "divoom_astrootoo_menu_registry.h"
#include "divoom_astrootoo_watch.h"
#include "divoom_astrootoo_main_menu.h"
#include "divoom_astrootoo_language_route.h"
#include "divoom_lvgl_base_dial_menu.h"
#include "divoom_astrootoo_settings.h"
#include "divoom_lvgl_poweron.h"
#include "divoom_astrootoo_config_wifi.h"
#include "divoom_astrootoo_oobe_config_wifi.h"
#include "divoom_astrootoo_oobe_welcome.h"
#include "divoom_astrootoo_alarm.h"
#include "divoom_astrootoo_tools.h"
#include "divoom_astrootoo_games.h"
#include "divoom_astrootoo_sleep.h"
#include "divoom_astrootoo_source.h"
#include "divoom_astrootoo_face_market.h"
#include "divoom_lvgl_set_language.h"
#include "divoom_lvgl_select_album_play.h"
#include "divoom_lvgl_photo_manage.h"
#include "divoom_lvgl_photo_view.h"
#include "divoom_lvgl_photo_add_picture_to_album.h"
#include "divoom_lvgl_file_transfer.h"
#include "divoom_lvgl_hot_update_prompt.h"
#include "divoom_lvgl_gallery_transfer.h"
#include "divoom_lvgl_sleep_aid_active.h"
#include "divoom_alarm_active_menu.h"
#include "divoom_lvgl_calendar_active.h"
#include "divoom_lvgl_device_update.h"
#include "divoom_lvgl_mixer.h"
#include "divoom_lvgl_mix_music2.h"
#include "divoom_lvgl_power_shutdown_menu.h"
#include "divoom_lvgl_power_off_disp_menu.h"
#include "divoom_lvgl_reseting_device.h"

extern void *divoom_astrootoo_placeholder_init(void *user_data, int menu_id);
extern void divoom_astrootoo_placeholder_disp(void *handle, void *active_obj);
extern void divoom_astrootoo_placeholder_clean(void *handle);
extern void divoom_astrootoo_placeholder_pause(void *handle);
extern void *divoom_astrootoo_settings_tws_init(void *user_data, int menu_id);
extern void divoom_astrootoo_settings_tws_disp(void *handle, void *active_obj);
extern void divoom_astrootoo_settings_tws_clean(void *handle);
extern void divoom_astrootoo_settings_tws_pause(void *handle);

#ifdef DIVOOM_PRODUCT_ASTRO_TOO

const divoom_menu_control_item_info pdivoom_menu_list[] =
{
    /* batch 1：表盘根 + 主菜单 */
    ASTRO_MENU_ROOT(DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID,
        divoom_lvgl_base_dial_menu_init,  divoom_lvgl_base_dial_menu_exit, 
        divoom_lvgl_base_dial_menu_disp, divoom_lvgl_base_dial_menu_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_MAIN,
        divoom_astrootoo_main_menu_init, divoom_astrootoo_main_menu_clean,
        divoom_astrootoo_language_route_main_disp, divoom_astrootoo_main_menu_pause),

    /* batch 2：设置树（push_flag=1，每屏独立 menu_id） */
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETING_MENU,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_DISPLAY,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_MEDIA,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_CONNECTION,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_PREFERENCES,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_SYSTEM,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_BRIGHTNESS,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_VOLUME,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_NIGHTVIEW,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_SLIDE_SETING_MENU,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_SLIDESHOW_ORDER,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_WIFI,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_WIFI_ADD,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_WIFI_PASSWORD,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_BLUETOOTH,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_TWS,
        divoom_astrootoo_settings_tws_init, divoom_astrootoo_settings_tws_clean,
        divoom_astrootoo_settings_tws_disp, divoom_astrootoo_settings_tws_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_LANGUGE,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_EQUALIZER,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_WIND_UNIT,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_KEY_LIGHT_EFFECTS,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_DATE_FORMAT,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_IDLE_POWER_OFF_TIME,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_POWER_OFF_TIME,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_DATE_TIME,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
#if DIVOOM_ASTRO_MANUAL_DATETIME_SET_ENABLE
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_DATE_TIME_TIME,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_DATE_TIME_DATE,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
#endif
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_TEMPERATURE,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_BT_SET_SECRET,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_MCP_TOOL,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_ABOUT,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_CONNECTION_CODE,
        divoom_astrootoo_settings_init, divoom_astrootoo_settings_clean,
        divoom_astrootoo_settings_disp, divoom_astrootoo_settings_pause),

    /* batch 3–6：主菜单入口已注册 menu_id，暂用 placeholder */
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_ALARM_LIST_MENU,
        divoom_astrootoo_alarm_init, divoom_astrootoo_alarm_clean,
        divoom_astrootoo_alarm_disp, divoom_astrootoo_alarm_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_SLEEP_AID,
        divoom_astrootoo_sleep_init, divoom_astrootoo_sleep_clean,
        divoom_astrootoo_sleep_disp, divoom_astrootoo_sleep_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_AUDIO_SOURCE_MENU,
        divoom_astrootoo_source_init, divoom_astrootoo_source_clean,
        divoom_astrootoo_source_disp, divoom_astrootoo_source_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_STORE_CLOCK_MARKET,
        divoom_astrootoo_face_market_init, divoom_astrootoo_face_market_clean,
        divoom_astrootoo_face_market_disp, divoom_astrootoo_face_market_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_TOOL,
        divoom_astrootoo_tools_init, divoom_astrootoo_tools_clean,
        divoom_astrootoo_tools_disp, divoom_astrootoo_tools_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_POMODORO_MENU,
        divoom_astrootoo_tools_init, divoom_astrootoo_tools_clean,
        divoom_astrootoo_tools_disp, divoom_astrootoo_tools_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_STOP_WATCH_MENU,
        divoom_astrootoo_tools_init, divoom_astrootoo_tools_clean,
        divoom_astrootoo_tools_disp, divoom_astrootoo_tools_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_COUNT_DOWN_MENU,
        divoom_astrootoo_tools_init, divoom_astrootoo_tools_clean,
        divoom_astrootoo_tools_disp, divoom_astrootoo_tools_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_SCORE_BOARD_MENU,
        divoom_astrootoo_tools_init, divoom_astrootoo_tools_clean,
        divoom_astrootoo_tools_disp, divoom_astrootoo_tools_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_NOISE_MEASURE_MENU,
        divoom_astrootoo_tools_init, divoom_astrootoo_tools_clean,
        divoom_astrootoo_tools_disp, divoom_astrootoo_tools_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_VOICE_RECORD_TOOL_MENU,
        divoom_astrootoo_tools_init, divoom_astrootoo_tools_clean,
        divoom_astrootoo_tools_disp, divoom_astrootoo_tools_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_LYRIC_MENU,
        divoom_astrootoo_placeholder_init, divoom_astrootoo_placeholder_clean,
        divoom_astrootoo_placeholder_disp, divoom_astrootoo_placeholder_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_ASTROTOO_GAMES,
        divoom_astrootoo_games_init, divoom_astrootoo_games_clean,
        divoom_astrootoo_games_disp, divoom_astrootoo_games_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_SELECT_ALBUM_PLAY,
        divoom_lvgl_select_album_play_init, divoom_lvgl_select_album_play_exit,
        divoom_lvgl_select_album_play_disp, divoom_lvgl_select_album_play_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_PHOTO_MANAGE,
        divoom_lvgl_photo_manage_init, divoom_lvgl_photo_manage_exit,
        divoom_lvgl_photo_manage_disp, divoom_lvgl_photo_manage_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_PHOTO_VIEW,
        divoom_lvgl_photo_view_init, divoom_lvgl_photo_view_exit,
        divoom_lvgl_photo_view_disp, divoom_lvgl_photo_view_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_ADD_PICTURE_TO_ALBUM,
        divoom_lvgl_select_album_play_init, divoom_lvgl_select_album_play_exit,
        divoom_lvgl_select_album_play_disp, divoom_lvgl_select_album_play_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_ADD_PHOTO,
        divoom_lvgl_photo_add_picture_to_album_init, divoom_lvgl_photo_add_picture_to_album_exit,
        divoom_lvgl_photo_add_picture_to_album_disp, divoom_lvgl_photo_add_picture_to_album_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_ASTROTOO_PLACEHOLDER,
        divoom_astrootoo_placeholder_init, divoom_astrootoo_placeholder_clean,
        divoom_astrootoo_placeholder_disp, divoom_astrootoo_placeholder_pause),
    ASTRO_MENU_STACK(1, DIVOOM_MENU_CONTROL_ID_SLEEP_AID_ACTIVE, 
        divoom_lvgl_sleep_aid_active_init, divoom_lvgl_sleep_aid_active_exit, 
        divoom_lvgl_sleep_aid_active_disp, divoom_lvgl_sleep_aid_active_pause),
	
    ASTRO_MENU_OVERLAY(1, DIVOOM_MENU_CONTROL_ID_ALARM_ACTIVE_MENU, 
    divoom_alarm_active_menu_init, divoom_alarm_active_menu_exit, 
    divoom_alarm_active_menu_disp, NULL),

    /* Outlook/Google 日历 Active 全屏（活动进行中） */
    ASTRO_MENU_OVERLAY(1, DIVOOM_MENU_CONTROL_ID_GOOGLE_CALENDAR_ACTIVE_MENU,
        divoom_lvgl_calendar_active_init, divoom_lvgl_calendar_active_exit,
        divoom_lvgl_calendar_active_disp, NULL),
    ASTRO_MENU_OVERLAY(1, DIVOOM_MENU_CONTROL_ID_OUTLOOK_CALENDAR_ACTIVE_MENU,
        divoom_lvgl_calendar_active_init, divoom_lvgl_calendar_active_exit,
        divoom_lvgl_calendar_active_disp, NULL),

    /* 开机/配网全屏：push_flag=0 */
    ASTRO_MENU_OVERLAY(1, DIVOOM_MENU_CONTROL_ID_ASTROTOO_OOBE_WELCOME,
        divoom_astrootoo_oobe_welcome_init, divoom_astrootoo_oobe_welcome_exit,
        divoom_astrootoo_oobe_welcome_disp, divoom_astrootoo_oobe_welcome_pause),
    ASTRO_MENU_OVERLAY(1, DIVOOM_MENU_CONTROL_ID_ASTROTOO_OOBE_CONFIG_WIFI,
        divoom_astrootoo_oobe_config_wifi_init, divoom_astrootoo_oobe_config_wifi_exit,
        divoom_astrootoo_oobe_config_wifi_disp, divoom_astrootoo_oobe_config_wifi_pause),
    ASTRO_MENU_OVERLAY(1, DIVOOM_MENU_CONTROL_ID_CONFIG_WIFI,
        divoom_astrootoo_oobe_config_wifi_init, divoom_astrootoo_oobe_config_wifi_exit,
        divoom_astrootoo_oobe_config_wifi_disp, divoom_astrootoo_oobe_config_wifi_pause),
    ASTRO_MENU_OVERLAY(1, DIVOOM_MENU_CONTROL_ID_SYSTEM_POWER_ON,
        divoom_lvgl_poweron_init, divoom_lvgl_poweron_exit,
        divoom_lvgl_poweron_disp, divoom_lvgl_poweron_pause),
    ASTRO_MENU_OVERLAY(1, DIVOOM_MENU_CONTROL_ID_ACCEPT_APP_PHOTO_VIEW,
        divoom_lvgl_gallery_transfer_init, divoom_lvgl_gallery_transfer_exit,
        divoom_lvgl_gallery_transfer_disp, divoom_lvgl_gallery_transfer_pause),
    ASTRO_MENU_OVERLAY(1, DIVOOM_MENU_CONTROL_ID_FILE_TRANSFER,
        divoom_lvgl_file_transfer_init, divoom_lvgl_file_transfer_exit,
        divoom_lvgl_file_transfer_disp, divoom_lvgl_file_transfer_pause),
//    ASTRO_MENU_OVERLAY(1, DIVOOM_MENU_CONTROL_ID_HOT_UPDATE_PROMPT,
//        divoom_lvgl_hot_update_prompt_init, divoom_lvgl_hot_update_prompt_exit,
//        divoom_lvgl_hot_update_prompt_disp, divoom_lvgl_hot_update_prompt_pause),
    ASTRO_MENU_OVERLAY(1, DIVOOM_MENU_CONTROL_ID_MIXER,
        divoom_lvgl_mixer_init, divoom_lvgl_mixer_exit,
        divoom_lvgl_mixer_disp, divoom_lvgl_mixer_pause),
    ASTRO_MENU_OVERLAY(1, DIVOOM_MENU_CONTROL_ID_MIX_MUSIC2,
        divoom_lvgl_mix_music2_init, divoom_lvgl_mix_music2_exit,
        divoom_lvgl_mix_music2_disp, divoom_lvgl_mix_music2_pause),

    /* App/云端固件升级、JL OTA：全屏覆盖，push_flag=0 */
    ASTRO_MENU_OVERLAY(0, DIVOOM_MENU_CONTROL_ID_UPDATE_DEVICE_FIRMWARE,
        divoom_lvgl_device_update_init, divoom_lvgl_device_update_exit,
        divoom_lvgl_device_update_disp, NULL),
    ASTRO_MENU_OVERLAY(0, DIVOOM_MENU_CONTROL_ID_JL_OTA_UPDATE,
        divoom_lvgl_jl_ota_update_init, divoom_lvgl_jl_ota_update_exit,
        divoom_lvgl_jl_ota_update_disp, NULL),
    ASTRO_MENU_OVERLAY(0, DIVOOM_MENU_CONTROL_ID_POWER_SHUTDOWN,
        divoom_lvgl_power_shutdown_menu_init, divoom_lvgl_power_shutdown_menu_exit,
        divoom_lvgl_power_shutdown_menu_disp, NULL),
    /* 关屏占位页：熄屏叠黑页。表盘 ROOT 不压栈，开屏走 return_startup_dial 重建 */
    ASTRO_MENU_OVERLAY(1, DIVOOM_MENU_CONTROL_ID_POWER_OFF_DISP_MENU,
        divoom_lvgl_power_off_disp_menu_init, divoom_lvgl_power_off_disp_menu_exit,
        divoom_lvgl_power_off_disp_menu_disp, NULL),
    /* 恢复出厂等待页：全屏覆盖，刷完后由 MAIN 做 mkfs */
    ASTRO_MENU_OVERLAY(0, DIVOOM_MENU_CONTROL_ID_RESETING_DEVICE_MENU,
        divoom_lvgl_reseting_device_init, divoom_lvgl_reseting_device_exit,
        divoom_lvgl_reseting_device_disp, NULL),
};

const unsigned int pdivoom_menu_list_count =
    (unsigned int)(sizeof(pdivoom_menu_list) / sizeof(pdivoom_menu_list[0]));

#endif

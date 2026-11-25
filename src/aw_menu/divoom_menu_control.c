#include "divoom.h"
#include "divoom_lvgl_gallery_transfer.h"
#include "lvgl.h"
#include "divoom_load_message.h"
#include "divoom_menu_control.h"
#ifdef DIVOOM_PRODUCT_ASTRO_TOO
#include "divoom_lvgl_top_radio_wait.h"
#endif
#include "divoom_screen_driver.h"
#include "divoom_popup_menu.h"
#include "divoom_queue.h"
#include "divoom_pixel_format.h"
#include "divoom_os_com.h"
#if (!defined(DIVOOM_SIMULATOR) || !(DIVOOM_SIMULATOR)) && defined(CONFIG_COMPONENTS_AW_EYESEE_MPP_VO)
#include <video/sunxi_display2.h>
#define DIVOOM_MENU_CONTROL_USE_MPP_UI_LAYER 1
extern int hwd_layer_open(unsigned int hlay);
extern int hwd_layer_close(unsigned int hlay);
extern int hwd_layer_top(unsigned int hlay);
extern int hwd_layer_bottom(unsigned int hlay);
extern int disp_ioctl(int cmd, void *arg);
extern void disp_enable_update(void);
extern void disp_disable_update(void);
extern void *sunxifb_alloc(size_t size, char *label);
extern void sunxifb_free(void **data, char *label);
extern void sunxifb_flush_cache(void *data, size_t size);
extern void *sunxifb_mem_get_phyaddr(void *data);
#else
#define DIVOOM_MENU_CONTROL_USE_MPP_UI_LAYER 0
#endif
#if defined(DIVOOM_SIMULATOR) && DIVOOM_SIMULATOR
#include "sim_pal.h"
#include <stdlib.h>
#endif
/* 字体管理在 ASTRO_TOO 与非 ASTRO_TOO 下均被本文件调用（load_font/clear_clock_font/init），
 * 必须无条件包含；否则 ASTRO_TOO 构建会因缺原型把返回的 64 位字体指针隐式截断成 int 而崩溃 */
#include "divoom_lvgl_font_manage.h"

#define DIVOOM_TIME_FLIP_DEMO_BOOT_ONLY              1
#define DIVOOM_VIDEO_TOP_OSD_CHANNEL                  1
#define DIVOOM_VIDEO_TOP_OSD_LAYER                    1

#ifndef DIVOOM_PRODUCT_ASTRO_TOO
#include "divoom_lvgl_dial_menu.h"
#include "divoom_lvgl_photo_stream.h"
#include "divoom_lvgl_photo_manage.h"
#include "divoom_lvgl_main_menu.h"
#include "divoom_lvgl_photo_view.h"
#include "divoom_lvgl_test_image.h"
#include "divoom_lvgl_photo_push.h"
#include "divoom_lvgl_font_manage.h"
#include "divoom_lvgl_photo_slide_effect.h"
#include "divoom_lvgl_select_album_play.h"
#include "divoom_lvgl_photo_add_picture_to_album.h"
#include "divoom_lvgl_component_menu.h"
#include "divoom_lvgl_photo_setting_menu.h"
#include "divoom_lvgl_dial_preview_menu.h"
#include "divoom_lvgl_dial_preview_menu2.h"
#include "divoom_lvgl_system_menu.h"
#include "divoom_lvgl_more_function.h"
#include "divoom_tomato_time_menu.h"
#include "divoom_tomato_time_edit_menu.h"
#include "divoom_tomato_time_edit_time_menu.h"
#include "divoom_tomato_time_edit_repeat_menu.h"
#include "divoom_tomato_time_edit_bell_menu.h"
#include "divoom_lvgl_system_setting.h"
#include "divoom_lvgl_alarm_menu.h"
#include "divoom_lvgl_alarm_edit_menu.h"
#include "divoom_lvgl_alarm_add_menu.h"
#include "divoom_white_noise_menu.h"
#include "divoom_white_noise_time_menu.h"
#include "divoom_lvgl_white_noise_list_menu.h"
#include "divoom_lvgl_white_noise_play_main.h"
#include "divoom_lvgl_setting_about.h"
#include "divoom_lvgl_mcp_tool.h"
#include "divoom_lvgl_setting_idle_poweroff.h"
#include "divoom_lvgl_setting_temperature.h"
#include "divoom_lvgl_set_language.h"
#include "divoom_lvgl_setting_screen_save.h"
#include "divoom_lvgl_setting_date_time.h"
#include "divoom_lvgl_setting_bluetooth.h"
#include "divoom_lvgl_setting_wifi.h"
#include "divoom_lvgl_setting_night_view.h"
#include "divoom_lvgl_setting_restore.h"
#include "divoom_lvgl_config_wifi.h"
#include "divoom_lvgl_device_update.h"
#include "divoom_lvgl_bt_music_menu.h"
#include "divoom_lvgl_tf_music_menu.h"
#include "divoom_lvgl_setting_power_off.h"
#include "divoom_lvgl_lyric_effect.h"
#include "divoom_alarm_active_menu.h"
#include "divoom_bt_set_secret_menu.h"
#include "divoom_lvgl_poweron.h"
#include "divoom_lvgl_alarm_clock_wake_hold.h"
#include "divoom_lvgl_count_down_menu.h"
#include "divoom_lvgl_count_down_start_menu.h"
#include "divoom_lvgl_stop_watch_menu.h"
#include "divoom_lvgl_slide_setting.h"
#include "divoom_lvgl_setting_theme.h"
#include "divoom_lvgl_tool_menu.h"
#include "divoom_lvgl_score_board_menu.h"
#include "divoom_lvgl_setting_wifi_properties.h"
#include "divoom_lvgl_accept_app_photo_view.h"
#include "divoom_lvgl_base_dial_menu.h"
#include "divoom_lvgl_top_main_menu.h"
#include "divoom_lvgl_noise_meter_menu.h"
#include "divoom_lvgl_top_ambient_menu.h"
#if 0 /* legacy: voice_record_tool — AstroToo menu 105 用 astrootoo_tools MEMO */
#include "divoom_lvgl_voice_record_tool_menu.h"
#endif
#if 0 /* legacy: product test menus — disabled for MVP pt_ui */
#include "divoom_lvgl_product_test_menu.h"
#endif
#include "divoom_lvgl_reseting_device.h"
#include "divoom_lvgl_power_off_disp_menu.h"
#include "divoom_lvgl_calendar.h"
#include "divoom_lvgl_calendar_active.h"
#include "divoom_lvgl_setting_color_temp.h"
#include "divoom_lvgl_demo_dial_menu.h"
#include "divoom_lvgl_calendar_list_menu.h"
#include "divoom_lvgl_audio_source_menu.h"
#include "divoom_lvgl_tf_music_folder_menu.h"
#include "divoom_lvgl_aux_music_menu.h"
#include "divoom_lvgl_uac_music_menu.h"
#include "divoom_lvgl_lyric_bg_list_menu.h"
#include "divoom_lvgl_sleep_aid.h"
#include "divoom_lvgl_sleep_aid_active.h"
#else
#include "divoom_astrootoo_watch.h"
#include "divoom_astrootoo_main_menu.h"
#include "divoom_astrootoo_settings.h"
#include "divoom_astrootoo_menu_compat.h"
#include "divoom_lvgl_poweron.h"
#include "divoom_lvgl_config_wifi.h"
#include "divoom_lvgl_set_language.h"
#endif

enum{
    DIVOOM_MENU_CONTROL_EVENT_NEXT,             //进入下一级菜单
    DIVOOM_MENU_CONTROL_EVENT_PRE,              //返回前一级菜单
    DIVOOM_MENU_CONTROL_EVENT_NEXT_POP_CUR,     //进入下一级菜单，并且将当前菜单弹出
    DIVOOM_MENU_CONTROL_EVENT_RETURN_MAIN,      //返回最上级菜单
    DIVOOM_MENU_CONTROL_EVENT_CLEAR_ALL_LOAD,   //清除所有当前菜单，再进入下一级
    DIVOOM_MENU_CONTROL_EVENT_POP_MODEL_ALL,    //清除指定模块和事件的所有菜单
    DIVOOM_MENU_CONTROL_EVENT_POP_MENU,         //清除指定模块和事件的所有菜单
    DIVOOM_MENU_CONTROL_EVENT_NEXT_AND_PASS,   //如果菜单里面有这个菜单就不管了
    DIVOOM_MENU_CONTROL_EVENT_PHOTO_CHANGE_SLIDE,   //这个是做特例，为了切换幻灯片的
    DIVOOM_MENU_CONTROL_EVENT_SWITCH_LANGUAGE,      //切换语言并强制重建导航链
    DIVOOM_MENU_CONTROL_EVENT_REDISPLAY_CURRENT,     //不换页，重新 disp 当前页
};

static int divoom_menu_control_poweroff_blocks_menu(int menu_id)
{
	if (!divoom_onoff_is_poweroff_active())
		return 0;
	return menu_id != DIVOOM_MENU_CONTROL_ID_POWER_SHUTDOWN;
}

typedef void DIVOOM_MENU_CONTROL_LVGL_CTRL_CALLBACK(void *user_data);

typedef  struct _divoom_menu_control_lvgl_ctrl_para
{
    int run_flag;
    void *sem_data;
    void *user_data;
    DIVOOM_MENU_CONTROL_LVGL_CTRL_CALLBACK *pcallback;

}divoom_menu_control_lvgl_ctrl_para, *Pdivoom_menu_control_lvgl_ctrl_para;

typedef struct _divoom_menu_control_event_info
{
    int type;
    int  model_id;      //属于哪个系统模式
    int  event_id;      //属于哪个系统事件
    void *user_data;
    int menu_id;
    int root_menu_id;
    int lang_id;
    uint32 menu_generation;
}divoom_menu_control_event_info, *Pdivoom_menu_control_event_info;

#define DIVOOM_IMAGE_BASE_PATH  "A:/usr/share/divoom_app/ui/"
#define DIVOOM_IMAGE_BASE_PATH2  "/usr/share/divoom_app/ui/"

#ifdef DIVOOM_PRODUCT_ASTRO_TOO
extern const divoom_menu_control_item_info pdivoom_menu_list[];
extern const unsigned int pdivoom_menu_list_count;
#else
#if 0
const divoom_menu_control_item_info pdivoom_menu_list[] =
{
//    {0, 1, DIVOOM_MENU_CONTROL_ID_MAIN, divoom_lvgl_system_menu_init,  divoom_lvgl_system_menu_exit, divoom_lvgl_system_menu_disp, divoom_lvgl_system_menu_pause},
    {0, 1, DIVOOM_MENU_CONTROL_ID_LYRIC_MENU, divoom_lvgl_lyric_effect_init,  divoom_lvgl_lyric_effect_exit, divoom_lvgl_lyric_effect_disp, divoom_lvgl_lyric_effect_pause},
    //{1, 1, DIVOOM_MENU_CONTROL_ID_MAIN, divoom_lvgl_main_menu_init,  divoom_lvgl_main_menu_exit, divoom_lvgl_main_menu_disp, NULL},
    {0, 1, DIVOOM_MENU_CONTROL_ID_MORE_FUNCTION, divoom_lvgl_more_function_init,  divoom_lvgl_more_function_exit, divoom_lvgl_more_function_disp, divoom_lvgl_more_function_pause},
    {0, 0, DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID, divoom_lvgl_base_dial_menu_init,  divoom_lvgl_base_dial_menu_exit, divoom_lvgl_base_dial_menu_disp, divoom_lvgl_base_dial_menu_pause},
    {1, 2, DIVOOM_MENU_CONTROL_ID_PHOTO_MANAGE, divoom_lvgl_photo_manage_init,  divoom_lvgl_photo_manage_exit, divoom_lvgl_photo_manage_disp, divoom_lvgl_photo_manage_pause},
    {1, 1, DIVOOM_MENU_CONTROL_ID_PHOTO_VIEW, divoom_lvgl_photo_view_init,  divoom_lvgl_photo_view_exit, divoom_lvgl_photo_view_disp, divoom_lvgl_photo_view_pause},
    {1, 0, DIVOOM_MENU_CONTROL_ID_ACCEPT_APP_PHOTO_VIEW, divoom_lvgl_accept_app_photo_view_init,  divoom_lvgl_accept_app_photo_view_exit, divoom_lvgl_accept_app_photo_view_disp, divoom_lvgl_accept_app_photo_view_pause},
    {1, 1, DIVOOM_MENU_CONTROL_ID_SELECT_ALBUM_PLAY, divoom_lvgl_select_album_play_init,  divoom_lvgl_select_album_play_exit, divoom_lvgl_select_album_play_disp, divoom_lvgl_select_album_play_pause},        
    {1, 1, DIVOOM_MENU_CONTROL_ID_ADD_PICTURE_TO_ALBUM, divoom_lvgl_select_album_play_init,  divoom_lvgl_select_album_play_exit, divoom_lvgl_select_album_play_disp, divoom_lvgl_select_album_play_pause},        
    {1, 1, DIVOOM_MENU_CONTROL_ID_PHOTO_SLIDE_EFFECT, divoom_lvgl_photo_slide_effect_init,  divoom_lvgl_photo_slide_effect_exit, divoom_lvgl_photo_slide_effect_disp, divoom_lvgl_photo_slide_effect_pause},        
    {1, 1, DIVOOM_MENU_CONTROL_ID_ADD_PHOTO, divoom_lvgl_photo_manage_init,  divoom_lvgl_photo_manage_exit, divoom_lvgl_photo_manage_disp, NULL},
    {1, 1, DIVOOM_MENU_CONTROL_ID_PHOTO_SETTING, divoom_lvgl_photo_setting_menu_init,  divoom_lvgl_photo_setting_menu_exit, divoom_lvgl_photo_setting_menu_disp, NULL},        
	{0, 1, DIVOOM_MENU_CONTROL_ID_COMPONENT, divoom_lvgl_component_menu_init, divoom_lvgl_component_menu_exit, divoom_lvgl_component_menu_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_DIAL_PREVIEW2, divoom_lvgl_dial_preview_menu_init, divoom_lvgl_dial_preview_menu_exit, divoom_lvgl_dial_preview_menu_disp, divoom_lvgl_dial_preview_menu_pause},
	{1, 0, DIVOOM_MENU_CONTROL_ID_DIAL_PREVIEW_FAVORITES, divoom_lvgl_dial_preview_menu_init, divoom_lvgl_dial_preview_menu_exit, divoom_lvgl_dial_preview_menu_disp, divoom_lvgl_dial_preview_menu_pause},
	{1, 0, DIVOOM_MENU_CONTROL_ID_DIAL_PREVIEW, divoom_lvgl_dial_preview_menu2_init, divoom_lvgl_dial_preview_menu2_exit, divoom_lvgl_dial_preview_menu2_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_POMODORO_MENU, divoom_tomato_time_menu_init, divoom_tomato_time_menu_clean, divoom_tomato_time_menu_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_TOMA_EDIT, divoom_tomato_time_edit_menu_init, divoom_tomato_time_edit_menu_clean, divoom_tomato_time_edit_menu_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_TOMA_EDIT_TIME, divoom_tomato_time_edit_time_menu_init, divoom_tomato_time_edit_time_menu_clean, divoom_tomato_time_edit_time_menu_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_TOMA_EDIT_REPEAT, divoom_tomato_time_edit_repeat_menu_init, divoom_tomato_time_edit_repeat_menu_clean, divoom_tomato_time_edit_repeat_menu_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_TOMA_EDIT_BELL, divoom_tomato_time_edit_bell_menu_init, divoom_tomato_time_edit_bell_menu_exit, divoom_tomato_time_edit_bell_menu_disp, NULL},        
	{1, 1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETING_MENU, divoom_lvgl_system_setting_init, divoom_lvgl_system_setting_exit, divoom_lvgl_system_setting_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_ABOUT, divoom_lvgl_setting_about_init, divoom_lvgl_setting_about_exit, divoom_lvgl_setting_about_disp, NULL},
	{1, 0, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_MCP_TOOL, divoom_lvgl_mcp_tool_init, divoom_lvgl_mcp_tool_exit, divoom_lvgl_mcp_tool_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_IDLE_POWER_OFF_TIME, divoom_lvgl_setting_idle_poweroff_init, divoom_lvgl_setting_idle_poweroff_exit, divoom_lvgl_setting_idle_poweroff_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_TEMPERATURE, divoom_lvgl_setting_temperature_init, divoom_lvgl_setting_temperature_exit, divoom_lvgl_setting_temperature_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_LANGUGE, divoom_lvgl_setting_set_language_init, divoom_lvgl_setting_set_language_exit, divoom_lvgl_setting_set_language_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_SCREEN_SAVE, divoom_lvgl_setting_screen_save_init, divoom_lvgl_setting_screen_save_exit, divoom_lvgl_setting_screen_save_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_WIFI, divoom_lvgl_setting_wifi_init, divoom_lvgl_setting_wifi_exit, divoom_lvgl_setting_wifi_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_BLUETOOTH, divoom_lvgl_setting_bluetooth_init, divoom_lvgl_setting_bluetooth_exit, divoom_lvgl_setting_bluetooth_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_DATE_TIME, divoom_lvgl_setting_date_time_init, divoom_lvgl_setting_date_time_exit, divoom_lvgl_setting_date_time_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_NIGHTVIEW, divoom_lvgl_setting_night_view_init, divoom_lvgl_setting_night_view_exit, divoom_lvgl_setting_night_view_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_RESTORE, divoom_lvgl_setting_restore_init, divoom_lvgl_setting_restore_exit, divoom_lvgl_setting_restore_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_ALARM_LIST_MENU, divoom_lvgl_alarm_menu_init, divoom_lvgl_alarm_menu_eixt, divoom_lvgl_alarm_menu_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_ALARM_ADD_MENU, divoom_lvgl_alarm_add_menu_init, divoom_lvgl_alarm_add_menu_eixt, divoom_lvgl_alarm_add_menu_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_ALARM_EDIT_MENU, divoom_lvgl_alarm_edit_menu_init, divoom_lvgl_alarm_edit_menu_exit, divoom_lvgl_alarm_edit_menu_disp, NULL},
    {1, 1, DIVOOM_MENU_CONTROL_ID_WHITE_NOISE_MENU, divoom_white_noise_menu_init, divoom_white_noise_menu_clean, divoom_white_noise_menu_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_WHITE_NOISE_LIST, divoom_lvgl_white_noise_list_menu_init, divoom_lvgl_white_noise_list_menu_exit, divoom_lvgl_white_noise_list_menu_disp, divoom_lvgl_white_noise_list_menu_pause},
	{1, 1, DIVOOM_MENU_CONTROL_ID_WHITE_NOISE_PLAY_MAIN, divoom_lvgl_white_noise_play_main_init, divoom_lvgl_white_noise_play_main_exit, divoom_lvgl_white_noise_play_main_disp, divoom_lvgl_white_noise_play_main_pause},
	{1, 1, DIVOOM_MENU_CONTROL_ID_WHITE_NOISE_EDIT_TIME, divoom_white_noise_time_menu_init, divoom_white_noise_time_menu_exit, divoom_white_noise_time_menu_disp, NULL},
	{1, 0, DIVOOM_MENU_CONTROL_ID_CONFIG_WIFI, divoom_lvgl_config_wifi_init, divoom_lvgl_config_wifi_exit, divoom_lvgl_config_wifi_disp, divoom_lvgl_config_wifi_pause},
	{1, 0, DIVOOM_MENU_CONTROL_ID_UPDATE_DEVICE_FIRMWARE, divoom_lvgl_device_update_init, divoom_lvgl_device_update_exit, divoom_lvgl_device_update_disp, NULL},
	{1, 0, DIVOOM_MENU_CONTROL_ID_JL_OTA_UPDATE, divoom_lvgl_jl_ota_update_init, divoom_lvgl_jl_ota_update_exit, divoom_lvgl_jl_ota_update_disp, NULL},
	//{1, 0, DIVOOM_MENU_CONTROL_ID_BLUE_MUSIC_MENU, divoom_lvgl_bt_music_menu_init, divoom_lvgl_bt_music_menu_exit, divoom_lvgl_bt_music_menu_disp, divoom_lvgl_bt_music_menu_pause},
	//{1, 0, DIVOOM_MENU_CONTROL_ID_LOCAL_MUSIC_MENU, divoom_lvgl_tf_music_menu_init, divoom_lvgl_tf_music_menu_exit, divoom_lvgl_tf_music_menu_disp, divoom_lvgl_tf_music_menu_pause},
	{1, 1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_POWER_OFF_TIME, divoom_lvgl_setting_power_off_init, divoom_lvgl_setting_power_off_exit, divoom_lvgl_setting_power_off_disp, NULL},
	{1, 0, DIVOOM_MENU_CONTROL_ID_ALARM_ACTIVE_MENU, divoom_alarm_active_menu_init, divoom_alarm_active_menu_exit, divoom_alarm_active_menu_disp, NULL},
	//{1, 1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_BT_SET_SECRET, divoom_bt_set_secret_menu_init, divoom_bt_set_secret_menu_eixt, divoom_bt_set_secret_menu_disp, NULL},
	/* 开机页是一次性页面，切走后不应压栈复用，避免与视频退出后的对象生命周期交叉 */
	{1, 0, DIVOOM_MENU_CONTROL_ID_SYSTEM_POWER_ON, divoom_lvgl_poweron_init, divoom_lvgl_poweron_exit, divoom_lvgl_poweron_disp, divoom_lvgl_poweron_pause},
#if 0 /* legacy: product test menus — disabled for MVP pt_ui */
	{1, 1, DIVOOM_MENU_CONTROL_ID_PRODUCT_STRESS_TESTING_MENU, divoom_lvgl_poweron_init, divoom_lvgl_poweron_exit, divoom_lvgl_poweron_disp, divoom_lvgl_poweron_pause},
#endif
	{1, 1, DIVOOM_MENU_CONTROL_ID_COUNT_DOWN_MENU, divoom_lvgl_count_down_menu_init, divoom_lvgl_count_down_menu_exit, divoom_lvgl_count_down_menu_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_COUNT_DOWN_START_MENU, divoom_lvgl_count_down_start_menu_init, divoom_lvgl_count_down_start_menu_exit, divoom_lvgl_count_down_start_menu_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_STOP_WATCH_MENU, divoom_lvgl_stop_watch_menu_init, divoom_lvgl_stop_watch_menu_exit, divoom_lvgl_stop_watch_menu_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_SLIDE_SETING_MENU, divoom_lvgl_slide_setting_init, divoom_lvgl_slide_setting_exit, divoom_lvgl_slide_setting_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_THEME, divoom_lvgl_setting_theme_init, divoom_lvgl_setting_theme_exit, divoom_lvgl_setting_theme_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_TOOL, divoom_lvgl_tool_menu_init, divoom_lvgl_tool_menu_exit, divoom_lvgl_tool_menu_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_SCORE_BOARD_MENU, divoom_lvgl_score_board_menu_init, divoom_lvgl_score_board_menu_exit, divoom_lvgl_score_board_menu_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_WIFI_PROPERTIES, divoom_lvgl_setting_wifi_properties_init, divoom_lvgl_setting_wifi_properties_exit, divoom_lvgl_setting_wifi_properties_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_NOISE_MEASURE_MENU, divoom_lvgl_noise_meter_menu_init, divoom_lvgl_noise_meter_menu_exit, divoom_lvgl_noise_meter_menu_disp, NULL},
#if 0 /* legacy: voice_record_tool menu 105 — AstroToo 见 astrootoo_menu_registry */
	{1, 1, DIVOOM_MENU_CONTROL_ID_VOICE_RECORD_TOOL_MENU, divoom_lvgl_voice_record_tool_menu_init, divoom_lvgl_voice_record_tool_menu_exit, divoom_lvgl_voice_record_tool_menu_disp, NULL},
#endif
#if 0 /* legacy: product test menus — disabled for MVP pt_ui */
	{1, 1, DIVOOM_MENU_CONTROL_ID_PRODUCT_TEST_MENU, divoom_lvgl_product_test_menu_init, divoom_lvgl_product_test_menu_exit, divoom_lvgl_product_test_menu_disp, NULL},
#endif
	{1, 1, DIVOOM_MENU_CONTROL_ID_RESETING_DEVICE_MENU, divoom_lvgl_reseting_device_init, divoom_lvgl_reseting_device_exit, divoom_lvgl_reseting_device_disp, NULL},
	{1, 0, DIVOOM_MENU_CONTROL_ID_POWER_OFF_DISP_MENU, divoom_lvgl_power_off_disp_menu_init, divoom_lvgl_power_off_disp_menu_exit, divoom_lvgl_power_off_disp_menu_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_GOOGLE_CALENDAR_MENU, divoom_lvgl_calendar_init, divoom_lvgl_calendar_exit, divoom_lvgl_calendar_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_OUTLOOK_CALENDAR_MENU, divoom_lvgl_calendar_init, divoom_lvgl_calendar_exit, divoom_lvgl_calendar_disp, NULL},
	{1, 0, DIVOOM_MENU_CONTROL_ID_GOOGLE_CALENDAR_ACTIVE_MENU, divoom_lvgl_calendar_active_init, divoom_lvgl_calendar_active_exit, divoom_lvgl_calendar_active_disp, NULL},
	{1, 0, DIVOOM_MENU_CONTROL_ID_OUTLOOK_CALENDAR_ACTIVE_MENU, divoom_lvgl_calendar_active_init, divoom_lvgl_calendar_active_exit, divoom_lvgl_calendar_active_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_COLOR_TEMP, divoom_lvgl_setting_color_temp_init, divoom_lvgl_setting_color_temp_exit, divoom_lvgl_setting_color_temp_disp, NULL},
	{1, 0, DIVOOM_MENU_CONTROL_ID_DEMO_DIAL_MENU, divoom_lvgl_demo_dial_menu_init, divoom_lvgl_demo_dial_menu_exit, divoom_lvgl_demo_dial_menu_disp, divoom_lvgl_demo_dial_menu_pause},
	{1, 1, DIVOOM_MENU_CONTROL_ID_CALENDAR_LIST_MENU, divoom_lvgl_calendar_list_menu_init, divoom_lvgl_calendar_list_menu_exit, divoom_lvgl_calendar_list_menu_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_BT_SET_SECRET, divoom_bt_set_secret_menu_init, divoom_bt_set_secret_menu_eixt, divoom_bt_set_secret_menu_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_AUDIO_SOURCE_MENU, divoom_lvgl_audio_source_menu_init, divoom_lvgl_audio_source_menu_exit, divoom_lvgl_audio_source_menu_disp, NULL},
	{1, 1, DIVOOM_MENU_CONTROL_ID_LOCAL_MUSIC_MENU, divoom_lvgl_tf_music_menu_init, divoom_lvgl_tf_music_menu_exit, divoom_lvgl_tf_music_menu_disp, divoom_lvgl_tf_music_menu_pause},
	{1, 1, DIVOOM_MENU_CONTROL_ID_LOCAL_MUSIC_FOLDER_MENU, divoom_lvgl_tf_music_folder_menu_init, divoom_lvgl_tf_music_folder_menu_exit, divoom_lvgl_tf_music_folder_menu_disp, divoom_lvgl_tf_music_folder_menu_pause},
	{1, 1, DIVOOM_MENU_CONTROL_ID_AUX_MUSIC_MENU, divoom_lvgl_aux_music_menu_init, divoom_lvgl_aux_music_menu_exit, divoom_lvgl_aux_music_menu_disp, divoom_lvgl_aux_music_menu_pause},
	{1, 1, DIVOOM_MENU_CONTROL_ID_UAC_MUSIC_MENU, divoom_lvgl_uac_music_menu_init, divoom_lvgl_uac_music_menu_exit, divoom_lvgl_uac_music_menu_disp, divoom_lvgl_uac_music_menu_pause},
#ifdef DIVOOM_TIME_FRAME_SPEAKER
	{1, 0, DIVOOM_MENU_CONTROL_ID_ALARM_CLOCK_WAKE_HOLD, divoom_lvgl_alarm_clock_wake_hold_init, divoom_lvgl_alarm_clock_wake_hold_exit, divoom_lvgl_alarm_clock_wake_hold_disp, divoom_lvgl_alarm_clock_wake_hold_pause},
#endif
	{1, 1, DIVOOM_MENU_CONTROL_ID_LYRIC_BG_LIST_MENU, divoom_lvgl_lyric_bg_list_menu_init, divoom_lvgl_lyric_bg_list_menu_exit, divoom_lvgl_lyric_bg_list_menu_disp, divoom_lvgl_lyric_bg_list_menu_pause},
	{1, 0, DIVOOM_MENU_CONTROL_ID_SLEEP_AID, divoom_lvgl_sleep_aid_init, divoom_lvgl_sleep_aid_exit, divoom_lvgl_sleep_aid_disp, divoom_lvgl_sleep_aid_pause},
	{1, 0, DIVOOM_MENU_CONTROL_ID_SLEEP_AID_ACTIVE, divoom_lvgl_sleep_aid_active_init, divoom_lvgl_sleep_aid_active_exit, divoom_lvgl_sleep_aid_active_disp, divoom_lvgl_sleep_aid_active_pause},
};
#endif
#endif


typedef struct _divoom_menu_control_menu_info
{
    int key_flag;
    int push_flag;      //如果有新菜单来，这级菜单是否压入队列, 1:压入，0：不压入
    int  menu_id;
    int  model_id;      //属于哪个系统模式
    int  event_id;      //属于哪个系统事件
    void *cur_active_obj;
    void *user_data;                        //相关的数据结构
    DIVOOM_MENU_CONTROL_MENU_LVGL_CLEAN *pclean;    //菜单的加载动画
    DIVOOM_MENU_CONTROL_MENU_LVGL_DISP *pdisp;    //菜单的加载动画
    DIVOOM_MENU_CONTROL_MENU_LVGL_PAUSE *pause;    //菜单的加载动画
    void *plvgl_obj;
}divoom_menu_control_menu_info, *Pdivoom_menu_control_menu_info;

typedef struct _divoom_menu_control_ctrl
{
    int first_menu_id;
    int  load_music_font_flag;
    uint32 key_press_time;
    Pdivoom_menu_control_menu_info pcur_menu;
    void *load_font_queue;
    void *load_font_sem;
    void *wait_sem;
    void *menu_queue;
    void *event_queue;
    void *remove_queue;
    void *del_queue;
    void * lvgl_key_group;
	lv_font_t *menu_font;
	lv_font_t *lyric_font; 
    struct _lv_timer_t * event_time_handle;
    struct _lv_timer_t * system_call_lvgl_time_handle;
    int video_mode;
    uint8 video_ui_handoff; /* 持 LVGL 锁：0 普通，1 暂缓旧 UI 恢复，2 等新根屏提交。 */
    void *switch_placeholder; /* 持 LVGL 锁时借用当前切页栈对象，切页结束前清空。 */
}divoom_menu_control_ctrl, *Pdivoom_menu_control_ctrl;

static Pdivoom_menu_control_ctrl pdivoom_menu_control_handle = NULL;
static uint32 s_menu_control_generation = 1;

static void divoom_menu_control_mark_current_changed(void)
{
    s_menu_control_generation++;
    if (s_menu_control_generation == 0)
    {
        s_menu_control_generation = 1;
    }
}

/*********************************************************************
  Function:        divoom_menu_control_clear_remove_menu
  Description:     移除正要删除的菜单
  Input:           menu_id
  Return:          NONE
**********************************************************************/

void divoom_menu_control_clear_remove_menu(int menu_id);
void divoom_menu_control_add_all_remove_menu(void);

/*********************************************************************
  Function:        divoom_menu_control_get_slide_stype
  Description:     判断用户的手势
  Input:           x1
                   y1
                   x2
                   y2
  Return:          NONE
**********************************************************************/

int divoom_menu_control_get_slide_stype(int x1, int y1, int x2, int y2)
{
    int abs_x, abs_y;

    abs_x = x1>x2?x1-x2:x2-x1;
    abs_y = y1>y2?y1-y2:y2-y1;
    if(abs_x < 10 &&  abs_y < 10)
    {
        return DIVOOM_MENU_CONTROL_SREEN_SLIDE_EMPTY;
    }
    
    if(abs_x > abs_y)
    {
        return x1>x2?DIVOOM_MENU_CONTROL_SREEN_SLIDE_LEFT:DIVOOM_MENU_CONTROL_SREEN_SLIDE_RIGHT;
    }
    else
    {
        return y1>y2?DIVOOM_MENU_CONTROL_SREEN_SLIDE_UP:DIVOOM_MENU_CONTROL_SREEN_SLIDE_DOWN;
    }
}



/*********************************************************************
  Function:        divoom_menu_control_init_btn_stype
  Description:     初始化菜单样式
  Input:           active  有二个宽度的边框
                   unactive   没有宽度的边框
                   press   有二个宽度的边框
  Return:          NONE
**********************************************************************/

void divoom_menu_control_init_btn_stype(lv_style_t *active, const char *active_img, 
                                            lv_style_t *unactive, const char *unactive_img, 
                                            lv_style_t *press, const char *press_img, int radio_value)
{
    
    char *file_name = divoom_system_malloc(256);


    if(active)
    {
        lv_style_init(active);
        lv_style_set_bg_opa(active, 0);
        lv_style_set_text_color(active, lv_color_hex(DIVOOM_MENNU_TEXT_UNACTIVE_COLOR));
        lv_style_set_border_color(active, lv_color_hex(DIVOOM_MENNU_ACTIVE_BG_COLOR));
        lv_style_set_outline_color(active, lv_color_hex(DIVOOM_MENNU_NORMAL_BG_COLOR));
        lv_style_set_border_width(active, 4);
        lv_style_set_radius(active, radio_value);
        lv_style_set_outline_width(active, 0);
        lv_style_set_shadow_width(active, 0);
        lv_style_set_border_side(active, LV_BORDER_SIDE_FULL);
        if(active_img)
        {
            strcpy(file_name, DIVOOM_IMAGE_BASE_PATH);
            strcat(file_name, active_img);
            lv_style_set_bg_img_src(active, file_name);
        }
    }

    if(unactive)
    {
        lv_style_init(unactive);
        lv_style_set_bg_opa(unactive, 0);        
        lv_style_set_text_color(unactive, lv_color_hex(DIVOOM_MENNU_TEXT_UNACTIVE_COLOR));
        lv_style_set_border_color(unactive, lv_color_hex(DIVOOM_MENNU_NORMAL_BG_COLOR));
        lv_style_set_outline_color(unactive, lv_color_hex(DIVOOM_MENNU_NORMAL_BG_COLOR));
        lv_style_set_border_width(unactive, 0);
        lv_style_set_outline_width(unactive, 0);
        lv_style_set_shadow_width(unactive, 0);
        lv_style_set_radius(unactive, 0);
        lv_style_set_border_side(unactive, LV_BORDER_SIDE_NONE);
        if(unactive_img)
        {
            strcpy(file_name, DIVOOM_IMAGE_BASE_PATH);
            strcat(file_name, unactive_img);
            lv_style_set_bg_img_src(unactive, file_name);
        }
    }

    if(press)
    {
        lv_style_init(press);
        lv_style_set_bg_opa(press, 0);  
        lv_style_set_text_color(press, lv_color_hex(DIVOOM_MENNU_TEXT_ACTIVE_COLOR));
        lv_style_set_bg_color(press, lv_color_hex(DIVOOM_MENNU_FOCUS_BG_COLOR));
        lv_style_set_border_width(press, 4);
        lv_style_set_shadow_width(press, 0);
        lv_style_set_outline_width(press, 0);
        lv_style_set_radius(press, radio_value);
        lv_style_set_border_side(press, LV_BORDER_SIDE_FULL);
        lv_style_set_border_color(press, lv_color_hex(DIVOOM_MENNU_FOCUS_BORDER_COLOR));
        if(press_img)
        {
            strcpy(file_name, DIVOOM_IMAGE_BASE_PATH);
            strcat(file_name, press_img);
            lv_style_set_bg_img_src(press, file_name);
        }
    }
    
    divoom_system_free(file_name);
}

/*********************************************************************
  Function:        divoom_menu_control_init_btn_stype
  Description:     初始化菜单样式
  Input:           normal  未选中状态
                   select_press   选中按了确定键状态
                   press   选中还未按确定键状态
  Return:          NONE
**********************************************************************/

void divoom_menu_control_init_btn_select_stype(lv_style_t *normal, lv_style_t *select_press, lv_style_t *press)
{
    
    if(normal)
    {
        lv_style_init(normal);
        lv_style_set_bg_color(normal, lv_color_hex(DIVOOM_MENNU_NORMAL_BG_COLOR));
        lv_style_set_text_color(normal, lv_color_hex(DIVOOM_MENNU_TEXT_UNACTIVE_COLOR));
        lv_style_set_border_color(normal, lv_color_hex(DIVOOM_MENNU_ACTIVE_BG_COLOR));
        lv_style_set_outline_color(normal, lv_color_hex(DIVOOM_MENNU_NORMAL_BG_COLOR));
        lv_style_set_border_width(normal, 0);
        lv_style_set_radius(normal, 5);
        lv_style_set_outline_width(normal, 0);
        lv_style_set_shadow_width(normal, 0);
        lv_style_set_border_side(normal, LV_BORDER_SIDE_NONE);
    }

    if(select_press)
    {
        lv_style_init(select_press);
        lv_style_set_bg_color(select_press, lv_color_hex(DIVOOM_MENNU_NORMAL_BG_COLOR));
        lv_style_set_text_color(select_press, lv_color_hex(DIVOOM_MENNU_TEXT_UNACTIVE_COLOR));
        lv_style_set_border_color(select_press, lv_color_hex(DIVOOM_MENNU_FOCUS_BORDER_COLOR));
        lv_style_set_outline_color(select_press, lv_color_hex(DIVOOM_MENNU_FOCUS_BORDER_COLOR));
        lv_style_set_border_width(select_press, 2);
        lv_style_set_outline_width(select_press, 0);
        lv_style_set_shadow_width(select_press, 0);
        lv_style_set_border_side(select_press, LV_BORDER_SIDE_NONE);
    }

    if(press)
    {
        lv_style_init(press);
        lv_style_set_text_color(press, lv_color_hex(DIVOOM_MENNU_TEXT_ACTIVE_COLOR));
        lv_style_set_bg_color(press, lv_color_hex(DIVOOM_MENNU_NORMAL_BG_COLOR));
        lv_style_set_border_color(select_press, lv_color_hex(DIVOOM_MENNU_FOCUS_BG_COLOR));
        lv_style_set_outline_color(select_press, lv_color_hex(DIVOOM_MENNU_FOCUS_BG_COLOR));
        lv_style_set_border_width(press, 2);
        lv_style_set_shadow_width(press, 0);
        lv_style_set_outline_width(press, 0);
        lv_style_set_radius(press, 5);
    }
    
}

/*********************************************************************
  Function:        divoom_menu_control_check_time_out_and_exit
  Description:     检查是超时退出当前菜单
  Input:           none
  Return:          NONE
**********************************************************************/

int divoom_menu_control_check_time_out_and_exit(void)
{
    int menu_id, lock_time;
    
    if(pdivoom_menu_control_handle->pcur_menu)
    {
        menu_id = pdivoom_menu_control_handle->pcur_menu->menu_id;
        if(menu_id >= DIVOOM_DISP_CLOCK_MODE )
        {
            return 0;
        }
        else
        {
                return 0;
        }
        
    }
    return  0;

}


/*********************************************************************
  Function:        divoom_menu_control_key_press_notice
  Description:     有按键来，清除空闲时间
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_menu_control_key_press_notice(void)
{
    if(pdivoom_menu_control_handle)
    {
        pdivoom_menu_control_handle->key_press_time = divoom_system_get_tickms()/1000;
    }

}

/*********************************************************************
  Function:        divoom_menu_control_destroy_system_font
  Description:     释放字体
  Input:           lang 0 菜单字体 1 歌词字体
  Return:          NONE
**********************************************************************/

void divoom_menu_control_destroy_system_font(int lang)
{
	if (lang == 0)
	{
		if (pdivoom_menu_control_handle->menu_font)
		{	
			lv_font_free(pdivoom_menu_control_handle->menu_font);
			pdivoom_menu_control_handle->menu_font = NULL;
		}
	}	
	else
	{
		if (pdivoom_menu_control_handle->lyric_font)	
		{	
			lv_font_free(pdivoom_menu_control_handle->lyric_font);
			pdivoom_menu_control_handle->lyric_font = NULL;
		}
	}
}

/*********************************************************************
  Function:        divoom_menu_control_load_system_font
  Description:     lang 0 加载菜单字体 1 加载歌词字体
  Input:           lang
  Return:          NONE
**********************************************************************/

void divoom_menu_control_load_system_font(int lang)
{
    #define DIVOOM_FONT_BASE_PATH  "/userdata/ui/"
	int languge = 0;
	char *file_name = divoom_system_malloc(strlen(DIVOOM_FONT_BASE_PATH)  + 30);
	
    strcpy(file_name, DIVOOM_FONT_BASE_PATH);
	if (lang == 0)
	{
		if (pdivoom_menu_control_handle->menu_font)
		{	
			lv_font_free(pdivoom_menu_control_handle->menu_font);
			pdivoom_menu_control_handle->menu_font = NULL;
		}
	}	
	else
	{
		if (pdivoom_menu_control_handle->lyric_font)	
		{	
            divoom_system_free(file_name);
            return;
		}
	}

	if (lang == 0)
	{
		strcat(file_name, "divoom_font16_menu.bin");
	}
	else
	{
		languge = 1;
		if (languge == DIVOOM_LVGL_MENU_LANG_SC || languge ==DIVOOM_LVGL_MENU_LANG_TC)
		{
			strcat(file_name, "divoom_font16_lyric_CN.bin");
		}
		else
		{
			strcat(file_name, "divoom_font16_lyric.bin");
		}
	}
	lv_font_t *font = lv_font_load(file_name);
    divoom_system_free(file_name);

    if(!font) 
	{
        divoom_print("font create failed.");

		if (lang == 0)
			pdivoom_menu_control_handle->menu_font = NULL;
		else
			pdivoom_menu_control_handle->lyric_font = NULL;
		return;
    }

	if (lang == 0)
		pdivoom_menu_control_handle->menu_font = font;
	else
		pdivoom_menu_control_handle->lyric_font = font;
}

/*********************************************************************
  Function:        divoom_menu_control_change_disp_mode
  Description:     修改显示模式
  Input:           video_flag  1：显示视频； 0：标准显示
  Return:          NONE
**********************************************************************/

#if DIVOOM_MENU_CONTROL_USE_MPP_UI_LAYER
static int divoom_menu_control_set_sunxifb_layer(int enable)
{
    struct disp_layer_config config;
    unsigned long arg[6];
    int ret;

    memset(&config, 0, sizeof(config));
    memset(arg, 0, sizeof(arg));
    config.enable = enable ? true : false;
    config.channel = 1;
    config.layer_id = 0;

    arg[0] = 0;
    arg[1] = (unsigned long)&config;
    arg[2] = 1;
    ret = disp_ioctl(DISP_LAYER_SET_CONFIG, (void *)arg);
    divoom_print("change disp mode sunxifb layer: chn:%d layer:%d enable:%d ret:%d\n",
                 config.channel, config.layer_id, enable, ret);
    return ret;
}

static int divoom_menu_control_is_video_disp(void)
{
    return (pdivoom_menu_control_handle != NULL) ? pdivoom_menu_control_handle->video_mode : 0;
}

static void divoom_menu_control_set_video_ui_layer(int video_flag)
{
    int ui_layer = 8; /* HLAY(UI_LYCHN, UI_LYLAY), see hwdisplay.h */

    divoom_get_lvgl_lock();
    if (divoom_os_com_lcd_is_disabled())
    {
        /* Owners still finish their logical handoff after LCD shutdown. */
        if (pdivoom_menu_control_handle)
            pdivoom_menu_control_handle->video_mode = video_flag ? 1 : 0;
        divoom_free_lvgl_lock();
        return;
    }
    if (video_flag)
    {
        int sunxifb_ret;
        int hwd_bottom_ret;
        int hwd_close_ret;

        disp_disable_update();
        /* Boot video reaches here before MPP hwd_init(), so hwd_layer_close()
         * cannot close the sunxifb layer. Disable its real DISP2 layer first. */
        sunxifb_ret = divoom_menu_control_set_sunxifb_layer(0);
        hwd_bottom_ret = hwd_layer_bottom(ui_layer);
        hwd_close_ret = hwd_layer_close(ui_layer);
        divoom_print("change disp mode video: ui layer:%d sunxifb:%d bottom:%d close:%d old:%d\n",
                     ui_layer, sunxifb_ret, hwd_bottom_ret, hwd_close_ret,
                     pdivoom_menu_control_handle ? pdivoom_menu_control_handle->video_mode : 0);
        if(pdivoom_menu_control_handle)
        {
            pdivoom_menu_control_handle->video_mode = 1;
        }
    }
    else
    {
        int hwd_open_ret;
        int hwd_top_ret;
        /* 先提交当前根屏，再打开硬件 UI 层，避免把视频前的旧帧闪出来。 */
        disp_enable_update();
        if (lv_scr_act())
        {
            lv_obj_invalidate(lv_scr_act());
        }
        lv_refr_now(lv_disp_get_default());
        hwd_open_ret = hwd_layer_open(ui_layer);
        hwd_top_ret = hwd_layer_top(ui_layer);
        divoom_print("change disp mode normal: layer:%d hwd_open:%d hwd_top:%d old:%d\n",
                     ui_layer,
                     hwd_open_ret,
                     hwd_top_ret,
                     pdivoom_menu_control_handle ? pdivoom_menu_control_handle->video_mode : 0);
        if(pdivoom_menu_control_handle)
        {
            pdivoom_menu_control_handle->video_mode = 0;
        }
    }
    divoom_free_lvgl_lock();
}
#else
static int divoom_menu_control_is_video_disp(void)
{
    return 0;
}

static void divoom_menu_control_set_video_ui_layer(int video_flag)
{
    (void)video_flag;
}
#endif

static int divoom_menu_control_set_video_ui_overlay_16bit(void *phy_addr,
    int x, int y, int width, int height, uint8 alpha_bits, int layer_id)
{
#if DIVOOM_MENU_CONTROL_USE_MPP_UI_LAYER
    struct disp_layer_config config;
    unsigned long arg[6];
    int ret;
    int vsync_ret = 0;

    if (phy_addr == NULL || width <= 0 || height <= 0)
    {
        return -1;
    }

    divoom_get_lvgl_lock();
    if (divoom_os_com_lcd_is_disabled())
    {
        divoom_free_lvgl_lock();
        return -1;
    }
    memset(&config, 0, sizeof(config));
    memset(arg, 0, sizeof(arg));
    /* Keep TOP OSD independent from both owners: sunxifb uses channel 1/layer 0,
     * while MPP reserves channel 2/layer 0.  This layer can therefore be
     * created or updated at any time during video playback. */
    config.channel = DIVOOM_VIDEO_TOP_OSD_CHANNEL;
    config.layer_id = layer_id;
    config.enable = true;
    config.info.mode = LAYER_MODE_BUFFER;
    config.info.zorder = 16 + layer_id - DIVOOM_VIDEO_TOP_OSD_LAYER;
    config.info.alpha_mode = 0;
    config.info.alpha_value = 0xff;
    config.info.screen_win.x = x;
    config.info.screen_win.y = y;
    config.info.screen_win.width = width;
    config.info.screen_win.height = height;
    config.info.fb.addr[0] = (unsigned long long)(uintptr_t)phy_addr;
    config.info.fb.size[0].width = width;
    config.info.fb.size[0].height = height;
    config.info.fb.size[1] = config.info.fb.size[0];
    config.info.fb.size[2] = config.info.fb.size[0];
    /* 歌词使用 4 位透明度保留抗锯齿，其他调用方继续使用原来的 1555。 */
    config.info.fb.format = alpha_bits == 4 ? DISP_FORMAT_ARGB_4444 : DISP_FORMAT_ARGB_1555;
    config.info.fb.pre_multiply = false;
    config.info.fb.flags = DISP_BF_NORMAL;
    config.info.fb.crop.x = 0;
    config.info.fb.crop.y = 0;
    config.info.fb.crop.width = ((uint64_t)width) << 32;
    config.info.fb.crop.height = ((uint64_t)height) << 32;

    arg[0] = 0;
    arg[1] = (unsigned long)&config;
    arg[2] = 1;
    ret = disp_ioctl(DISP_LAYER_SET_CONFIG, (void *)arg);
    if(ret == 0)
    {
        memset(arg, 0, sizeof(arg));
        arg[0] = 0;
        vsync_ret = disp_ioctl(DISP_WAIT_VSYNC, (void *)arg);
    }
    //divoom_print("video TOP ARGB1555 overlay: ch:%d layer:%d pos:%d,%d size:%dx%d ret:%d vsync:%d\n",
    //    config.channel, config.layer_id, x, y, width, height, ret, vsync_ret);
    divoom_free_lvgl_lock();
    return ret;
#else
    (void)layer_id;
    (void)alpha_bits;
    (void)phy_addr;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    return -1;
#endif
}

int divoom_menu_control_set_video_ui_overlay_argb1555(void *phy_addr,
    int x, int y, int width, int height)
{
    return divoom_menu_control_set_video_ui_overlay_16bit(phy_addr,
        x, y, width, height, 1, DIVOOM_VIDEO_TOP_OSD_LAYER);
}

int divoom_menu_control_set_video_ui_overlay_argb4444(void *phy_addr,
    int x, int y, int width, int height)
{
    return divoom_menu_control_set_video_ui_overlay_16bit(phy_addr,
        x, y, width, height, 4, DIVOOM_VIDEO_TOP_OSD_LAYER);
}

static int divoom_menu_control_disable_video_ui_overlay_layer(int layer_id)
{
#if DIVOOM_MENU_CONTROL_USE_MPP_UI_LAYER
    struct disp_layer_config config;
    unsigned long arg[6];
    int ret;
    int vsync_ret = 0;

    divoom_get_lvgl_lock();
    if (divoom_os_com_lcd_is_disabled())
    {
        divoom_free_lvgl_lock();
        return 0;
    }
    memset(&config, 0, sizeof(config));
    memset(arg, 0, sizeof(arg));
    config.channel = DIVOOM_VIDEO_TOP_OSD_CHANNEL;
    config.layer_id = layer_id;
    config.enable = false;
    arg[0] = 0;
    arg[1] = (unsigned long)&config;
    arg[2] = 1;
    ret = disp_ioctl(DISP_LAYER_SET_CONFIG, (void *)arg);
    if(ret == 0)
    {
        memset(arg, 0, sizeof(arg));
        arg[0] = 0;
        vsync_ret = disp_ioctl(DISP_WAIT_VSYNC, (void *)arg);
    }
    divoom_print("video ui overlay disabled: ch:%d layer:%d ret:%d vsync:%d\n",
        config.channel, config.layer_id, ret, vsync_ret);
    divoom_free_lvgl_lock();
    return ret;
#else
    (void)layer_id;
    return 0;
#endif
}

int divoom_menu_control_disable_video_ui_overlay(void)
{
    return divoom_menu_control_disable_video_ui_overlay_layer(DIVOOM_VIDEO_TOP_OSD_LAYER);
}

/* Static toast snapshots use the two remaining UI layers. They never replace
 * the clock/lyrics OSD on layer 1 or reopen the opaque RGB565 framebuffer. */
void divoom_menu_control_update_video_toast(int slot, void *obj)
{
#if DIVOOM_MENU_CONTROL_USE_MPP_UI_LAYER && LVGL_VERSION_MAJOR >= 9
    static struct {
        void *obj;
        void *buffer;
        int enabled;
    } toasts[DIVOOM_VIDEO_TOAST_COUNT];
    lv_draw_buf_t draw_buf;
    lv_area_t coords;
    uint32 width, height, size, x, y;
    void *phy_addr;
    int layer_id = 2 + slot;

    if (slot < 0 || slot >= DIVOOM_VIDEO_TOAST_COUNT)
        return;
    if (!divoom_menu_control_is_video_disp() || obj == NULL ||
        !lv_obj_is_valid(obj) || !lv_obj_is_visible(obj))
        obj = NULL;
    if (toasts[slot].obj == obj)
        return;

    /* Disable and wait for scanout before freeing or replacing its buffer. */
    if (toasts[slot].enabled &&
        divoom_menu_control_disable_video_ui_overlay_layer(layer_id) != 0)
        return;
    toasts[slot].enabled = 0;
    toasts[slot].obj = NULL;
    if (toasts[slot].buffer)
        sunxifb_free(&toasts[slot].buffer, "video_toast");
    if (obj == NULL)
        return;

    lv_obj_update_layout(obj);
    width = lv_obj_get_width(obj);
    height = lv_obj_get_height(obj);
    if (width == 0 || height == 0 || width > DIVOOM_SCREEN_WIDTH || height > 128U)
        return;
    size = LV_DRAW_BUF_SIZE(width, height, LV_COLOR_FORMAT_ARGB8888);
    toasts[slot].buffer = sunxifb_alloc(size, "video_toast");
    if (toasts[slot].buffer == NULL)
        return;
    memset(toasts[slot].buffer, 0, size);
    if (lv_draw_buf_init(&draw_buf, width, height, LV_COLOR_FORMAT_ARGB8888,
            LV_STRIDE_AUTO, toasts[slot].buffer, size) != LV_RES_OK ||
        lv_snapshot_take_to_draw_buf(obj, LV_COLOR_FORMAT_ARGB8888, &draw_buf) != LV_RES_OK)
        goto fail;

    width = draw_buf.header.w;
    height = draw_buf.header.h;
    /* Compact in place, preserving four alpha bits around rounded corners. */
    for (y = 0; y < height; y++) {
        lv_color32_t *src = (lv_color32_t *)((uint8 *)draw_buf.data + y * draw_buf.header.stride);
        uint16 *dst = (uint16 *)draw_buf.data + y * width;
        for (x = 0; x < width; x++) {
            lv_color32_t pixel = src[x];
            dst[x] = ((uint16)(pixel.alpha >> 4) << 12) |
                ((uint16)(pixel.red >> 4) << 8) |
                ((uint16)(pixel.green >> 4) << 4) | (pixel.blue >> 4);
        }
    }
    phy_addr = sunxifb_mem_get_phyaddr(draw_buf.data);
    if (phy_addr == NULL)
        goto fail;
    lv_obj_get_coords(obj, &coords);
    sunxifb_flush_cache(draw_buf.data, width * height * sizeof(uint16));
    if (divoom_menu_control_set_video_ui_overlay_16bit(phy_addr,
            coords.x1, coords.y1, width, height, 4, layer_id) != 0)
        goto fail;
    toasts[slot].obj = obj;
    toasts[slot].enabled = 1;
    return;
fail:
    sunxifb_free(&toasts[slot].buffer, "video_toast");
#else
    (void)slot;
    (void)obj;
#endif
}

void divoom_menu_control_change_disp_mode(void *main_obj, int video_flag)
{
    int active_menu_id = divoom_menu_control_get_active_menu_id();
    /* 停片与恢复 LVGL 参数照常进行；切页时只把硬件曝光留给新根屏。 */
    if(video_flag && pdivoom_menu_control_handle)
    {
        pdivoom_menu_control_handle->video_ui_handoff = 0;
    }
    if(!video_flag && pdivoom_menu_control_handle &&
        pdivoom_menu_control_handle->video_ui_handoff)
    {
        pdivoom_menu_control_handle->video_ui_handoff = 2;
        pdivoom_menu_control_handle->video_mode = 0;
    }
    else
    {
        divoom_menu_control_set_video_ui_layer(video_flag);
    }

    if(video_flag == 0 && (active_menu_id == DIVOOM_MENU_CONTROL_ID_LYRIC_MENU || active_menu_id == DIVOOM_MENU_CONTROL_ID_WHITE_NOISE_PLAY_MAIN) && main_obj != lv_scr_act())
    {
        return;
    }
    if(video_flag)
    {
#if LVGL_VERSION_MAJOR >= 9
        divoom_lvgl9_set_screen_transp(1);
        /* Main framebuffer stays RGB565; transparent TOP content uses a
         * compact hardware overlay when required. */
        lv_disp_set_color_format(lv_disp_get_default(), DIVOOM_LVGL_DISPLAY_COLOR_FORMAT);
#else
        lv_disp_get_default()->driver->screen_transp = 1;
        lv_memset_00(lv_disp_get_default()->driver->draw_buf->buf_act,
            lv_disp_get_default()->driver->draw_buf->size * sizeof(lv_color32_t));
#endif
         lv_disp_set_bg_opa(lv_disp_get_default(), LV_OPA_TRANSP);
         if (main_obj)
         {
             lv_obj_set_style_bg_opa(main_obj, LV_OPA_TRANSP, 0);
         }
        divoom_print("set lvgl background transparent for video\n");
    }
    else
    {
        divoom_print("restore ui layer by menu control: layer:\n");
#if LVGL_VERSION_MAJOR >= 9
        divoom_lvgl9_set_screen_transp(0);
        lv_disp_set_color_format(lv_disp_get_default(), DIVOOM_LVGL_DISPLAY_COLOR_FORMAT);
#else
         lv_disp_get_default()->driver->screen_transp = 0;
#endif
         lv_disp_set_bg_opa(lv_disp_get_default(), LV_OPA_COVER);
         if (main_obj)
         {
             lv_obj_set_style_bg_opa(main_obj, LV_OPA_COVER, 0);
         }
    }
}

/*********************************************************************
  Function:        divoom_menu_control_change_font_lang
  Description:     切换语言调用切换字体结构体
  Input:           lang
  Return:          NONE
**********************************************************************/

void divoom_menu_control_change_font_lang(int lang)
{
    int old_lang;
    int cur_type, old_type;

    old_lang = 0 ;//divoom_config_set_system_language(0xFF);
	//divoom_config_set_system_language(lang);
	if (lang == DIVOOM_LVGL_MENU_LANG_SC || lang ==DIVOOM_LVGL_MENU_LANG_TC)
	{
        cur_type = 0;
	}
	else
	{
        cur_type = 1;
	}
	if (old_lang == DIVOOM_LVGL_MENU_LANG_SC || old_lang ==DIVOOM_LVGL_MENU_LANG_TC)
	{
        old_type = 0;
	}
	else
	{
        old_type = 1;
	}
    if(cur_type != old_type)
    {
        uint8 *type = divoom_system_malloc(sizeof(1));
        *type = 2;
        divoom_queue_pop_all(pdivoom_menu_control_handle->load_font_queue, NULL);
        divoom_queue_push(pdivoom_menu_control_handle->load_font_queue, type);
        
        type = divoom_system_malloc(sizeof(1));
        *type = 1;
        divoom_queue_push(pdivoom_menu_control_handle->load_font_queue, type);
        divoom_system_put_sem(pdivoom_menu_control_handle->load_font_sem);            
    
    }
    //divoom_load_message_set_lang(lang);
}


int divoom_menu_control_get_system_font_id(void)
{
	/* 当前固件固定使用系统菜单字体，避免在独立控件预览阶段读取尚未初始化的主状态。 */
	return DIVOOM_SYSTEM_FONT_MENU_ID;
	
    /*switch(lang)
    {
        case DIVOOM_LVGL_MENU_LANG_SC:       //简体中文     
            font_id = DIVOOM_SYSTEM_SC_FONT_MENU_ID;
        	break;
            
        case DIVOOM_LVGL_MENU_LANG_JP:       //日语   
            font_id = DIVOOM_SYSTEM_JP_FONT_MENU_ID;        
        	break;
            
        case DIVOOM_LVGL_MENU_LANG_Thai:     //泰文    
            font_id = DIVOOM_SYSTEM_TH_FONT_MENU_ID;       
        	break;
            
        case DIVOOM_LVGL_MENU_LANG_Korean:   //韩国人    
            font_id = DIVOOM_SYSTEM_KR_FONT_MENU_ID;     
        	break;
            
        case DIVOOM_LVGL_MENU_LANG_TC:       //繁体           
            font_id = DIVOOM_SYSTEM_TC_FONT_MENU_ID;
        	break;

        default:
            font_id = DIVOOM_SYSTEM_OTHER_FONT_MENU_ID;
        	break;
    }
    divoom_print("fond: %d, %d\n", lang, font_id);
    return font_id;*/
}

/*********************************************************************
  Function:        divoom_menu_control_get_font_info
  Description:     返回对应字号的字体
  Input:           font_size 0xFF表示取歌词字体 否则取菜单字体
  Return:          NONE
**********************************************************************/

void *divoom_menu_control_get_font_info(int font_size, int menu_id)
{
    void *font_handle;
    font_handle = divoom_lvgl_font_manage_load_font(divoom_menu_control_get_system_font_id(), font_size, NULL, 0, 10, 64, NULL, menu_id, NULL);  
    if(font_handle == NULL)
    {
		return (void *)LV_FONT_DEFAULT;
    }
    return font_handle;
    #if 0
	if (font_size == 12)
	{
		return (void *)&lv_font_montserrat_12;
	}
    else if(font_size == 16)
    {
		if (pdivoom_menu_control_handle->menu_font)
	        return (void *)pdivoom_menu_control_handle->menu_font;
		return (void *)pdivoom_menu_control_handle->menu_font;
    }
    else if(font_size == 32)
    {
		if (pdivoom_menu_control_handle->menu_font)
			return (void *)pdivoom_menu_control_handle->menu_font;
		return (void *)pdivoom_menu_control_handle->menu_font;
    }
    else if(font_size == 40)
    {
		if (pdivoom_menu_control_handle->menu_font)
			return (void *)pdivoom_menu_control_handle->menu_font;
		return (void *)pdivoom_menu_control_handle->menu_font;
    }
	else if (font_size == 50)
	{
		return (void *)pdivoom_menu_control_handle->menu_font;
	}
	else if (font_size == 0xff)
	{
		if (pdivoom_menu_control_handle->lyric_font)
			return (void *)pdivoom_menu_control_handle->lyric_font;
		return (void *)pdivoom_menu_control_handle->menu_font;
	}
    return NULL;
    #endif
}

/*********************************************************************
  Function:        divoom_menu_control_get_font_info2
  Description:     返回对应字号的字体
  Input:           
  Return:          NONE
**********************************************************************/

void *divoom_menu_control_get_font_info2(int font_id, int font_size, int menu_id)
{
    void *font_handle;
    font_handle = divoom_lvgl_font_manage_load_font(font_id, font_size, NULL, 0, 0, 0, NULL, menu_id, NULL);  
    if(font_handle == NULL)
    {
		return (void *)LV_FONT_DEFAULT;
    }
    return font_handle;
}

/* 缺字保底仍走页面字体管理：样式参与缓存键，保持请求字号不变。 */
void *divoom_menu_control_get_font_info_style(int font_id, int font_size,
    uint16 font_style, int menu_id)
{
    void *font = divoom_lvgl_font_manage_load_font_style(font_id, font_size,
        font_style, menu_id, NULL);
    if (font == NULL)
    {
        font = divoom_menu_control_get_font_info(font_size, menu_id);
    }
    return font;
}

/*********************************************************************
  Function:        divoom_menu_control_get_disp_str
  Description:     通过关键字符串，要处理多语言得到显示字串
  Input:           src
                   SRC
  Return:          NONE
**********************************************************************/

char *divoom_menu_control_get_disp_str(const char * src)
{
    //return (char *)src;
    return divoom_load_message_get_value((char *)src);
}

/*********************************************************************
  Function:        divoom_menu_control_disp_label_text
  Description:     静态文本的单行与多行的显示适配
  Input:           label_obj
                   disp_str         显示的字符串
                   max_width        显示位置的最大宽度
                   max_high         显示位置的最大高度
                   use_font         显示使用的字体
                   font_high        显示字体的高度
                   left_pad_x       左边预留的位置
  Return:          NONE
**********************************************************************/

void divoom_menu_control_disp_label_text(void *label_obj, char *disp_str, int max_width, int max_high, int font_high, int left_pad_x, int align_mode, int menu_id)
{
    int text_len;
    void* use_font = divoom_menu_control_get_font_info(font_high, menu_id);

    if(use_font == NULL)
    {
        return;
    }
    lv_label_set_long_mode(label_obj, LV_LABEL_LONG_WRAP);

    //lv_obj_set_y(label_obj, max_high - font_high);
    lv_obj_set_size(label_obj, max_width, font_high);
    lv_obj_set_style_text_align(label_obj, align_mode, 0);
    lv_obj_set_style_text_font(label_obj, use_font, 0);
    lv_obj_set_style_text_color(label_obj, lv_color_hex(DIVOOM_MENNU_TEXT_COLOR), 0);
    lv_label_set_text(label_obj, disp_str);

}
uint8 divoom_menu_control_get_bat_file_name(char *file_name)
{
    uint8 level = 0;

    return level;

}

uint8 divoom_menu_control_get_work_mode_file_name(char *file_name)
{
    uint8 level = 0;

    return level;

}

/*********************************************************************
  Function:        divoom_menu_control_refresh_menu_bar
  Description:     刷新默认的菜单工具栏
  Input:           parent_obj   父窗口句柄
  Return:          NONE
**********************************************************************/
void divoom_menu_control_refresh_menu_bar(struct _lv_timer_t * menu_bar_time)
{
}

/*********************************************************************
  Function:        divoom_menu_control_destory_menu_bar
  Description:     退出菜单工具栏
  Input:           phandle
  Return:          NONE
**********************************************************************/

void divoom_menu_control_destory_menu_bar(Pdivoom_menu_control_menu_bar phandle)
{
    if(phandle->menu_bar_time)
    {
        lv_timer_del(phandle->menu_bar_time);
    }
}

/*********************************************************************
  Function:        divoom_menu_control_create_menu_bar
  Description:     创建默认的菜单工具栏
  Input:           phandle
                   main_obj 父窗口句柄
  Return:          NONE
**********************************************************************/

void divoom_menu_control_create_menu_bar(Pdivoom_menu_control_menu_bar phandle, void *main_obj)
{
}

// 生成高斯核（与LVGL相同算法）
static void generate_gaussian_kernel(float* kernel, int radius) {
    float sigma = radius/3.0f ;
    float sum = 0.0f;
    
    for (int i = -radius; i <= radius; i++) {
        float x = i;
        kernel[i + radius] = expf(-(x * x) / (2 * sigma * sigma));
        sum += kernel[i + radius];
    }
    
    // 归一化
    for (int i = 0; i < 2 * radius + 1; i++) {
        kernel[i] /= sum;
    }
}

// 水平模糊（操作RGB565数据）
static void blur_horizontal(uint8_t* buf, uint8_t *out, int width, int height, int radius, float* kernel, uint8 *exit_flag) {
    
    for (int y = 0; y < height && *exit_flag == 1; y++) {
        for (int x = 0; x < width; x++) {
            float b = 0, g = 0, r = 0;
            int base_pos = (y * width + x) * DIVOOM_APP_RGB565_BYTES_PER_PX;
            
            for (int i = -radius; i <= radius; i++) {
                int px = x + i;
                if (px < 0) px = 0;
                if (px >= width) px = width - 1;
                
                int pixel_pos = (y * width + px) * DIVOOM_APP_RGB565_BYTES_PER_PX;
                uint16 c = (uint16)buf[pixel_pos] | ((uint16)buf[pixel_pos + 1] << 8);
                float weight = kernel[i + radius];
                
                r += (uint8)(((c >> 11) & 0x1F) << 3) * weight;
                g += (uint8)(((c >> 5) & 0x3F) << 2) * weight;
                b += (uint8)((c & 0x1F) << 3) * weight;
            }
            
            divoom_store_rgb565_le(&out[base_pos], divoom_rgb888_to_rgb565(
                (uint8)(r + 0.5f), (uint8)(g + 0.5f), (uint8)(b + 0.5f)));
        }
    }
    
}

// 垂直模糊（操作RGB565数据）
static void blur_vertical(uint8_t* buf, uint8_t *out,int width, int height, int radius, float* kernel, uint8 *exit_flag) {
    
    
    for (int x = 0; x < width && *exit_flag == 1; x++) {
        for (int y = 0; y < height; y++) {
            float b = 0, g = 0, r = 0;
            int base_pos = (y * width + x) * DIVOOM_APP_RGB565_BYTES_PER_PX;
            
            for (int i = -radius; i <= radius; i++) {
                int py = y + i;
                if (py < 0) py = 0;
                if (py >= height) py = height - 1;
                
                int pixel_pos = (py * width + x) * DIVOOM_APP_RGB565_BYTES_PER_PX;
                uint16 c = (uint16)buf[pixel_pos] | ((uint16)buf[pixel_pos + 1] << 8);
                float weight = kernel[i + radius];
                
                r += (uint8)(((c >> 11) & 0x1F) << 3) * weight;
                g += (uint8)(((c >> 5) & 0x3F) << 2) * weight;
                b += (uint8)((c & 0x1F) << 3) * weight;
            }
            
            divoom_store_rgb565_le(&out[base_pos], divoom_rgb888_to_rgb565(
                (uint8)(r + 0.5f), (uint8)(g + 0.5f), (uint8)(b + 0.5f)));
        }
    }
    
}

// 添加50%黑色遮罩并增加透明度
static void add_dark_mask(uint8_t* buf, int width, int height) {
    for (int i = 0; i < width * height * 4; i += 4) {
        buf[i + 0] = buf[i + 0] / 2;  // B
        buf[i + 1] = buf[i + 1] / 2;  // G
        buf[i + 2] = buf[i + 2] / 2;  // R
        //buf[i + 3] = 255 ;  // A (将Alpha减半以增加透明度)
    }
}


/*********************************************************************
  Function:        divoom_menu_control_get_gaussian_buf
  Description:     得到高斯模糊的图像
  Input:           in_buf
                   in_w
                   in_h
                   out_buf
                   out_w
                   out_h
  Return:          NONE
**********************************************************************/

void divoom_menu_control_get_gaussian_buf(uint8 *in_buf, int in_w, int in_h, uint8 *out_buf, int out_w, int out_h, int blur_radius, uint8 *exit_flag)
{
    uint8 *temp, *small_buf;

    small_buf = malloc((out_w / 4) * (out_h / 4) * DIVOOM_APP_RGB565_BYTES_PER_PX);
    if(small_buf == NULL)
    {
        return;
    }
    divoom_rk_mpp_stretch_image(in_buf, in_w, in_h, RK_FORMAT_BGRA_8888, small_buf, out_w/4, out_h/4);
    
    // 生成高斯核
    int kernel_size = 2 * blur_radius + 1;
    float* kernel = (float*)malloc(kernel_size * sizeof(float));
    if(kernel == NULL)
    {
        free(small_buf);
        return;
    }
    generate_gaussian_kernel(kernel, blur_radius);

    temp = malloc((out_w / 4) * (out_h / 4) * DIVOOM_APP_RGB565_BYTES_PER_PX);
    if(temp == NULL)
    {
        free(small_buf);
        free(kernel);
        return;
    }
    // 水平+垂直分离模糊
    blur_horizontal(small_buf, temp, out_w/4, out_h/4, blur_radius, kernel, exit_flag);
    blur_vertical(temp, small_buf, out_w/4, out_h/4, blur_radius, kernel, exit_flag);
    
    // 3. 添加50%黑色遮罩并增加透明度
    //add_dark_mask(small_buf, out_w/4, out_h/4);
    // 释放资源
    divoom_rk_mpp_stretch_image(small_buf, out_w/4, out_h/4,
        RK_FORMAT_RGB_565, out_buf, out_w, out_h);
    free(temp);
    free(small_buf);
    free(kernel);
}

/*********************************************************************
  Function:        divoom_menu_control_get_black_buf
  Description:     得到某种颜色的底图
  Input:           in_buf
                   in_w
                   in_h
                   color_info
  Return:          NONE
**********************************************************************/

void divoom_menu_control_get_black_buf(uint8 *in_buf, int in_w, int in_h, lv_color_t color_info)
{
    lv_obj_t *canvas =lv_canvas_create(NULL);//创建画布
    lv_canvas_set_buffer(canvas,(void *)in_buf, in_w,in_h, LV_COLOR_FORMAT_RGB565A8);
    lv_canvas_fill_bg(canvas, color_info, LV_OPA_100);
    lv_obj_del(canvas);
}

/*********************************************************************
  Function:        divoom_menu_control_get_img_path2
  Description:     从相对而言路径得到LVGL使用的图像路径
  Input:           src
                   path
  Return:          NONE
**********************************************************************/

void divoom_menu_control_get_img_path2(const char * src, char *path)
{
    strcpy(path, DIVOOM_IMAGE_BASE_PATH2);
    strcat(path, src);
}



/*********************************************************************
  Function:        divoom_menu_control_get_img_path
  Description:     从相对而言路径得到LVGL使用的图像路径
  Input:           src
                   path
  Return:          NONE
**********************************************************************/

void divoom_menu_control_get_img_path(const char * src, char *path)
{
    strcpy(path, DIVOOM_IMAGE_BASE_PATH);
    strcat(path, src);
}

/*********************************************************************
  Function:        divoom_menu_control_lv_img_set_src
  Description:     只传相对而言路径，转到LVGL
  Input:           obj
                   SRC
  Return:          NONE
**********************************************************************/

void divoom_menu_control_lv_img_set_src(lv_obj_t * obj, const char * src)
{
    char *file_name = divoom_system_malloc(strlen(DIVOOM_IMAGE_BASE_PATH) + strlen(src) + 10);

    strcpy(file_name, DIVOOM_IMAGE_BASE_PATH);
    strcat(file_name, src);
    lv_img_cache_invalidate_src(file_name);
    lv_img_set_src(obj, file_name);
    divoom_system_free(file_name);
}

void divoom_menu_control_slider_clear_border_outline_shadow(lv_obj_t *obj)
{
    static const lv_part_t parts[3] = { LV_PART_MAIN, LV_PART_INDICATOR, LV_PART_KNOB };
    static const lv_state_t states[4] = {
        LV_STATE_DEFAULT,
        LV_STATE_FOCUSED,
        LV_STATE_FOCUS_KEY,
        LV_STATE_PRESSED
    };
    int ip;
    int is;

    if (!obj)
    {
        return;
    }

    for (ip = 0; ip < 3; ip++)
    {
        for (is = 0; is < 4; is++)
        {
            lv_style_selector_t sel = parts[ip] | states[is];
            lv_obj_set_style_border_width(obj, 0, sel);
            lv_obj_set_style_outline_width(obj, 0, sel);
            lv_obj_set_style_outline_pad(obj, 0, sel);
            lv_obj_set_style_shadow_width(obj, 0, sel);
        }
    }
}

/*********************************************************************
  Function:        divoom_menu_control_lv_component_img_set_src
  Description:     组件icon图片显示
  Input:           obj
                   SRC
  Return:          NONE
**********************************************************************/

void divoom_menu_control_lv_component_img_set_src(lv_obj_t * obj, int model_id, int image_id)
{
    char temp_name[128];
	divoom_manage_image_download_get_file_name(model_id, image_id, temp_name);

    char *file_name = divoom_system_malloc(strlen(temp_name) + 10);
	sprintf(file_name, "A:%s", temp_name);
	lv_img_cache_invalidate_src(obj); //把该资源清一把，否则会出现显示错乱问题
    lv_img_set_src(obj, file_name);
    divoom_system_free(file_name);
}

int divoom_menu_control_compre_menu(void *item1, void *item2)
{
    int *menu_id = (int *)item1;
    Pdivoom_menu_control_menu_info pload = (Pdivoom_menu_control_menu_info)item2;

    return *menu_id == pload->menu_id? 0:1;
}

static unsigned int divoom_menu_control_menu_list_size(void)
{
#ifdef DIVOOM_PRODUCT_ASTRO_TOO
    return pdivoom_menu_list_count;
#else
    return (unsigned int)(sizeof(pdivoom_menu_list) / sizeof(pdivoom_menu_list[0]));
#endif
}

/*********************************************************************
  Function:        divoom_menu_control_get_menu_id
  Description:     查找相关MENU的结构体信息替代ID
  Input:           menu_id
  Return:          menu_id
**********************************************************************/

int divoom_menu_control_get_menu_id(int menu_id)
{
    int i;

    if(menu_id < DIVOOM_DISP_CLOCK_MODE)
    {
        return menu_id;
    }

    for(i = 0; i < (int)divoom_menu_control_menu_list_size(); i++)
    {
        if(pdivoom_menu_list[i].id == menu_id)
        {
            return menu_id;
        }
    }
    return DIVOOM_DISP_CLOCK_MODE;
}

/*********************************************************************
  Function:        divoom_menu_control_get_menu_info
  Description:     查找相关MENU的结构体信息
  Input:           menu_id
  Return:          NONE
**********************************************************************/

divoom_menu_control_item_info *divoom_menu_control_get_menu_info(int menu_id)
{
    int i;

    for(i = 0; i < (int)divoom_menu_control_menu_list_size(); i++)
    {
        if(pdivoom_menu_list[i].id == menu_id)
        {
            return (divoom_menu_control_item_info*)&pdivoom_menu_list[i];
        }
    }
    if(menu_id < DIVOOM_DISP_CLOCK_MODE)
    {
        return NULL;
    }
    return divoom_menu_control_get_menu_info(DIVOOM_DISP_CLOCK_MODE);
}

/*********************************************************************
  Function:        divoom_menu_control_get_actice_animation_menu_handle
  Description:     获取激活菜单的句柄
  Input:           menu_id
  Return:          NONE
**********************************************************************/

void *divoom_menu_control_get_actice_animation_menu_handle()
{
    if(pdivoom_menu_control_handle->pcur_menu && pdivoom_menu_control_handle->pcur_menu->menu_id >= DIVOOM_DISP_CLOCK_MODE)
    {
        return pdivoom_menu_control_handle->pcur_menu->plvgl_obj;
    }
    return NULL;

}

/*********************************************************************
  Function:        divoom_menu_control_get_clock_mode_menu_handle
  Description:     获取表盘几个APP激活菜单的句柄
  Input:           menu_id
  Return:          NONE
**********************************************************************/

void *divoom_menu_control_get_clock_mode_menu_handle()
{
    if(pdivoom_menu_control_handle && pdivoom_menu_control_handle->pcur_menu)
    {
        return pdivoom_menu_control_handle->pcur_menu->plvgl_obj;
    }
    return NULL;

}

/*********************************************************************
  Function:        divoom_menu_control_is_enter_config_wifi
  Description:     是否进入配网状态
  Input:           
  Return:          1 or 0
**********************************************************************/

int divoom_menu_control_is_enter_config_wifi(void)
{
    if(pdivoom_menu_control_handle == NULL)
    {
        return 0;
    }
    if(pdivoom_menu_control_handle->pcur_menu)
    {
        if(pdivoom_menu_control_handle->pcur_menu->menu_id == DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_LANGUGE
            || pdivoom_menu_control_handle->pcur_menu->menu_id == DIVOOM_MENU_CONTROL_ID_ASTROTOO_OOBE_WELCOME
            || pdivoom_menu_control_handle->pcur_menu->menu_id == DIVOOM_MENU_CONTROL_ID_ASTROTOO_OOBE_CONFIG_WIFI
            || pdivoom_menu_control_handle->pcur_menu->menu_id == DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_WIFI
            || pdivoom_menu_control_handle->pcur_menu->menu_id == DIVOOM_MENU_CONTROL_ID_CONFIG_WIFI)
        {
            return 0;
        }
    }
    return 1;
}

/*********************************************************************
  Function:        divoom_menu_control_get_active_menu_id
  Description:     获取激活菜单的ID
  Input:           
  Return:          菜单的ID
**********************************************************************/

int divoom_menu_control_get_active_menu_id()
{
    if(pdivoom_menu_control_handle && pdivoom_menu_control_handle->pcur_menu)
    {
        return pdivoom_menu_control_handle->pcur_menu->menu_id;
    }
    return DIVOOM_DISP_CLOCK_MODE;

}

int divoom_menu_control_is_initialized(void)
{
    return pdivoom_menu_control_handle != NULL;
}

int divoom_menu_control_get_stack_depth(void)
{
    if (pdivoom_menu_control_handle && pdivoom_menu_control_handle->menu_queue)
    {
        return divoom_queue_items(pdivoom_menu_control_handle->menu_queue);
    }
    return 0;
}

int divoom_menu_control_get_del_queue_count(void)
{
    if (pdivoom_menu_control_handle && pdivoom_menu_control_handle->del_queue)
    {
        return divoom_queue_items(pdivoom_menu_control_handle->del_queue);
    }
    return 0;
}

static void divoom_menu_control_add_key_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_KEY)
    {
        char *key_info = (char *)lv_event_get_param(e);
        divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
    }
}

/*********************************************************************
  Function:        divoom_menu_control_add_key_event
  Description:     增加统一按键处理函数
  Input:           
  Return:          
**********************************************************************/

void divoom_menu_control_add_key_event(void)
{
    #if 0
    void *main_obj = lv_scr_act();

    if(pdivoom_menu_control_handle->pcur_menu->key_flag == 0)
    {
        lv_obj_add_flag(main_obj, LV_OBJ_FLAG_CLICKABLE);
        lv_group_focus_obj(main_obj);
        lv_group_remove_all_objs(pdivoom_menu_control_handle->lvgl_key_group);
        lv_group_add_obj(pdivoom_menu_control_handle->lvgl_key_group, main_obj);  
        lv_obj_add_event_cb(main_obj, divoom_menu_control_add_key_event_cb, LV_EVENT_KEY, NULL);
    }
    #endif
}

/*********************************************************************
  Function:        divoom_menu_control_set_menu_clear_flag
  Description:     设置当前菜单清除标记
  Input:           menu_id   菜单的ID
  Return:          
**********************************************************************/

void divoom_menu_control_set_menu_clear_flag(int menu_id)
{
    if(pdivoom_menu_control_handle->pcur_menu)
    {
        if(pdivoom_menu_control_handle->pcur_menu->menu_id == menu_id)
        {
            pdivoom_menu_control_handle->pcur_menu->push_flag = 0;
        }
    }

}

/*********************************************************************
  Function:        divoom_menu_control_return_to_lvgl_menu
  Description:     加载菜单，这个是从其它模式切换到菜单时调用，
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_menu_control_return_to_lvgl_menu(void)
{
    int try_time = 0, menu_id = 0;
    Pdivoom_menu_control_menu_info pmenu_info;
    while(pdivoom_menu_control_handle == NULL)
    {
        divoom_system_delay_ms(100);
        try_time ++;
        if(0 == (try_time % 20))
        {
            divoom_print("%s:%d wait %d\n", __func__, __LINE__, try_time);
        }
    }
    if(pdivoom_menu_control_handle->pcur_menu && pdivoom_menu_control_handle->pcur_menu->menu_id < DIVOOM_DISP_CLOCK_MODE)
    {
        if(pdivoom_menu_control_handle->pcur_menu->push_flag == 0)
        {
            divoom_menu_control_return_pre();
        }
        return;
    }
    pmenu_info = divoom_queue_get_cmp(pdivoom_menu_control_handle->menu_queue, &menu_id, divoom_menu_control_compre_menu);
    if(pmenu_info)
    {
        //如果存在，将最上层的一级菜单弹出，认为只有一级是提醒或屏保
        divoom_menu_control_return_pre();
    }
    else
    {
        //不存在，直接将菜单压入
        divoom_menu_control_load_next_direct(menu_id, NULL, 0, 0, 0);
    }
}


/*********************************************************************
  Function:        divoom_menu_control_find_menu_active
  Description:     查询某个菜单是否在显示队列中
  Input:           menu_id
  Return:          NONE
**********************************************************************/

int divoom_menu_control_find_menu_active(int menu_id)
{
    Pdivoom_menu_control_menu_info pmenu_info;


    if(pdivoom_menu_control_handle->pcur_menu && pdivoom_menu_control_handle->pcur_menu->menu_id == menu_id)
    {
        return 1;
    }
    pmenu_info = divoom_queue_get_cmp(pdivoom_menu_control_handle->menu_queue, &menu_id, divoom_menu_control_compre_menu);

    return pmenu_info != NULL;
}

/*********************************************************************
  Function:        divoom_menu_control_set_language
  Description:     修改语言
  Input:           lang
  Return:          NONE
**********************************************************************/

void divoom_menu_control_set_language(int lang)
{
    divoom_load_message_set_lang(lang);
}

#if defined(DIVOOM_SIMULATOR) && DIVOOM_SIMULATOR
static int s_menu_control_sim_stale_pre_ignored;

int divoom_menu_control_sim_stale_pre_ignored(void)
{
    return s_menu_control_sim_stale_pre_ignored;
}

static int divoom_menu_control_switch_language_sim_alloc_fail(void)
{
    static int fail_consumed = 0;
    const char *fail_env = getenv("DIVOOM_SIM_LANGUAGE_SWITCH_ALLOC_FAIL_ONCE");

    /* 模拟器专项：只拒绝第一次语言事件申请，验证配置不会在入队失败时提前提交。 */
    if (!fail_consumed && fail_env && strcmp(fail_env, "1") == 0)
    {
        fail_consumed = 1;
        return 1;
    }
    return 0;
}

static void divoom_menu_control_switch_language_sim_queue_stale_pre(void)
{
    static int pre_consumed = 0;
    const char *pre_env = getenv("DIVOOM_SIM_LANGUAGE_SWITCH_PENDING_PRE_ONCE");
    Pdivoom_menu_control_event_info pre_event;

    if (pre_consumed || !pre_env || strcmp(pre_env, "1") != 0)
    {
        return;
    }
    pre_event = divoom_system_malloc(sizeof(divoom_menu_control_event_info));
    if (pre_event == NULL)
    {
        return;
    }
    pre_consumed = 1;
    pre_event->type = DIVOOM_MENU_CONTROL_EVENT_PRE;
    pre_event->menu_id = pdivoom_menu_control_handle->pcur_menu
        ? pdivoom_menu_control_handle->pcur_menu->menu_id : -1;
    pre_event->menu_generation = s_menu_control_generation;
    divoom_queue_push(pdivoom_menu_control_handle->event_queue, pre_event);
    divoom_print("menu: simulate queued PRE before language switch source=%d generation=%u\n",
        pre_event->menu_id, (unsigned)pre_event->menu_generation);
}
#endif

/*********************************************************************
  Function:        divoom_menu_control_switch_language
  Description:     将语言加载和导航链重建延后到菜单事件循环，避免按键回调执行重 I/O
  Input:           lang, root_menu_id, target_menu_id
  Return:          1:事件已入队，0:参数无效或事件申请失败
**********************************************************************/
int divoom_menu_control_switch_language(int lang, int root_menu_id, int target_menu_id)
{
    Pdivoom_menu_control_event_info event_info;

    if (pdivoom_menu_control_handle == NULL
        || lang < 0 || lang >= DIVOOM_LVGL_MENU_LANG_END)
    {
        return 0;
    }
    root_menu_id = divoom_menu_control_get_menu_id(root_menu_id);
    target_menu_id = divoom_menu_control_get_menu_id(target_menu_id);
    if (divoom_menu_control_get_menu_info(root_menu_id) == NULL
        || divoom_menu_control_get_menu_info(target_menu_id) == NULL)
    {
        return 0;
    }

#if defined(DIVOOM_SIMULATOR) && DIVOOM_SIMULATOR
    if (divoom_menu_control_switch_language_sim_alloc_fail())
    {
        divoom_print("menu: simulate switch language event malloc fail lang=%d\n", lang);
        return 0;
    }
#endif

    event_info = divoom_system_malloc(sizeof(divoom_menu_control_event_info));
    if (event_info == NULL)
    {
        divoom_print("menu: switch language event malloc fail lang=%d\n", lang);
        return 0;
    }

    /* 新语言路由已成为唯一有效导航，立即让此前排队的 PRE 事件失效。 */
#if defined(DIVOOM_SIMULATOR) && DIVOOM_SIMULATOR
    divoom_menu_control_switch_language_sim_queue_stale_pre();
#endif
    divoom_menu_control_mark_current_changed();
    event_info->type = DIVOOM_MENU_CONTROL_EVENT_SWITCH_LANGUAGE;
    event_info->lang_id = lang;
    event_info->root_menu_id = root_menu_id;
    event_info->menu_id = target_menu_id;
    event_info->menu_generation = s_menu_control_generation;
    divoom_queue_push(pdivoom_menu_control_handle->event_queue, event_info);
    divoom_menu_control_add_all_remove_menu();
    return 1;
}

/* Initialize locally; an unsuccessful initializer must not become current. */
static Pdivoom_menu_control_menu_info divoom_menu_control_create_menu_node(
    divoom_menu_control_item_info *raw, int menu_id, void *user_data,
    int model_id, int event_id)
{
    Pdivoom_menu_control_menu_info menu = divoom_system_malloc(sizeof(*menu));
    if(menu == NULL)
    {
        divoom_print("menu: node allocation failed id=%d\n", menu_id);
        return NULL;
    }
    memset(menu, 0, sizeof(*menu));
    menu->user_data = user_data;
    menu->pclean = raw->pclean;
    menu->pdisp = raw->pdisp;
    menu->pause = raw->pause;
    menu->menu_id = menu_id;
    menu->model_id = model_id;
    menu->event_id = event_id;
    menu->push_flag = raw->push_flag;
    menu->key_flag = raw->key_flag;
    menu->plvgl_obj = raw->pinit(user_data, raw->id);
    if(menu->plvgl_obj == NULL)
    {
        divoom_print("menu: initialization failed id=%d\n", menu_id);
        divoom_system_free(menu);
        return NULL;
    }
    return menu;
}

static void divoom_menu_control_drop_failed_placeholder(void);

/*********************************************************************
  Function:        divoom_menu_control_load_next_direct
  Description:     马上加载菜单，会将当前菜单压入队列
  Input:           menu_id
  Return:          NONE
**********************************************************************/

void divoom_menu_control_load_next_direct(int menu_id, void* user_data, int pop_flag, int model_id, int event_id)
{
    int try_time = 0;
    int push_old = 0;
    divoom_menu_control_item_info *praw_menu_info;
    Pdivoom_menu_control_menu_info pmenu_info, pdel_handle = NULL;

    menu_id = divoom_menu_control_get_menu_id(menu_id);
    if (divoom_menu_control_poweroff_blocks_menu(menu_id))
        return;
    while(pdivoom_menu_control_handle == NULL)
    {
        divoom_system_delay_ms(100);
        try_time ++;
        if(0 == (try_time % 20))
        {
            divoom_print("%s:%d wait %d\n", __func__, __LINE__, try_time);
        }
    }
    if(pdivoom_menu_control_handle->pcur_menu && pdivoom_menu_control_handle->pcur_menu->menu_id == menu_id)
    {
        if(menu_id == DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID)
        {
            divoom_lvgl_base_dial_menu_update_clock_info(divoom_menu_control_get_clock_mode_menu_handle(), user_data, 0);
        }
        divoom_print("%s:%d pass the same menu id: %d\n", __func__, __LINE__, menu_id);
        return;
    }
    divoom_print("%s:%d start menu id: %d\n", __func__, __LINE__, menu_id);

    if(pdivoom_menu_control_handle->pcur_menu && pdivoom_menu_control_handle->pcur_menu->menu_id != menu_id)
    {
        //查看要增加的菜单是否压在队列中，如果压在队列中，将把队列中这个菜单前面的全部弹出
        pmenu_info = divoom_queue_get_cmp(pdivoom_menu_control_handle->menu_queue, &menu_id, divoom_menu_control_compre_menu);
        if(pmenu_info)
        {
            while(1)
            {
                pmenu_info = divoom_queue_pop_tail(pdivoom_menu_control_handle->menu_queue);
                if(pmenu_info->menu_id == menu_id)
                {
                    break;
                }
                divoom_menu_control_clear_remove_menu(pmenu_info->menu_id);
                divoom_queue_push(pdivoom_menu_control_handle->del_queue, pmenu_info);

            }
            pdel_handle = pdivoom_menu_control_handle->pcur_menu;
            if(pdel_handle && pdel_handle->pause)
            {
                pdel_handle->pause(pdel_handle->plvgl_obj);
            }
            pdivoom_menu_control_handle->pcur_menu = pmenu_info;
            divoom_menu_control_mark_current_changed();
            //如果已经存在的的
            praw_menu_info = divoom_menu_control_get_menu_info(menu_id);
            if(praw_menu_info && praw_menu_info->push_flag == 2)
            {
                pmenu_info->pclean(pmenu_info->plvgl_obj);
                divoom_lvgl_font_manage_clear_clock_font(pmenu_info->menu_id, NULL);
                pmenu_info->plvgl_obj = praw_menu_info->pinit(user_data, praw_menu_info->id);
                if(pmenu_info->plvgl_obj == NULL)
                {
                    pdivoom_menu_control_handle->pcur_menu = pdel_handle;
                    divoom_system_free(pmenu_info);
                    divoom_menu_control_mark_current_changed();
                    if(pdel_handle && pdel_handle->pdisp)
                        pdel_handle->pdisp(pdel_handle->plvgl_obj, pdel_handle->cur_active_obj);
                    divoom_menu_control_drop_failed_placeholder();
                    divoom_lvgl_top_control_update_active();
                    divoom_menu_control_add_key_event();
                    return;
                }
				divoom_lvgl_top_main_menu_exit();
				divoom_lvgl_top_ambient_menu_exit();
                pdivoom_menu_control_handle->pcur_menu->pdisp(pdivoom_menu_control_handle->pcur_menu->plvgl_obj, NULL);
            }
            else
            {
            	divoom_lvgl_top_main_menu_exit();
				divoom_lvgl_top_ambient_menu_exit();
                pdivoom_menu_control_handle->pcur_menu->pdisp(pdivoom_menu_control_handle->pcur_menu->plvgl_obj, pdivoom_menu_control_handle->pcur_menu->cur_active_obj);
            }
            divoom_menu_control_drop_failed_placeholder();
            divoom_lvgl_top_control_update_active();
            divoom_menu_control_add_key_event();
            if(pdel_handle)
            {
                divoom_menu_control_clear_remove_menu(pdel_handle->menu_id);
                divoom_queue_push(pdivoom_menu_control_handle->del_queue, pdel_handle);
            }
            return;
        }
    }
    praw_menu_info = divoom_menu_control_get_menu_info(menu_id);

    if(praw_menu_info == NULL)
    {
        divoom_print("%s:%d it fails to find menu id: %d\n", __func__, __LINE__,menu_id);
        return;
    }
	
	divoom_lvgl_top_main_menu_exit();
	divoom_lvgl_top_ambient_menu_exit();
    if(pdivoom_menu_control_handle->pcur_menu)
    {
        if(pdivoom_menu_control_handle->pcur_menu->push_flag && pop_flag == 0)
        {
            lv_group_t * cur_group_obj = lv_group_get_default();

            pdivoom_menu_control_handle->pcur_menu->cur_active_obj = lv_group_get_focused(cur_group_obj);
            if(pdivoom_menu_control_handle->pcur_menu->pause)
            {
                pdivoom_menu_control_handle->pcur_menu->pause(pdivoom_menu_control_handle->pcur_menu->plvgl_obj);
            }
            //pdivoom_menu_control_handle->pcur_menu->cur_active_obj = pdivoom_menu_control_handle->pcur_menu->pget_active(pdivoom_menu_control_handle->pcur_menu->plvgl_obj);
            push_old = 1;
        }
        else
        {
            if(pdivoom_menu_control_handle->pcur_menu->pause)
            {
                pdivoom_menu_control_handle->pcur_menu->pause(pdivoom_menu_control_handle->pcur_menu->plvgl_obj);
            }
            pdel_handle = pdivoom_menu_control_handle->pcur_menu;
        }
        if(model_id == 0)
        {
            model_id = pdivoom_menu_control_handle->pcur_menu->model_id;
            event_id = pdivoom_menu_control_handle->pcur_menu->event_id;
        }
    }
    
    pmenu_info = divoom_queue_pop_cmp(pdivoom_menu_control_handle->del_queue, &menu_id, divoom_menu_control_compre_menu);
    if(pmenu_info)
    {
        divoom_print("we will dele and reload: %d\n", menu_id);
        pmenu_info->pclean(pmenu_info->plvgl_obj);
        divoom_lvgl_font_manage_clear_clock_font(pmenu_info->menu_id, NULL);
        
        divoom_system_free(pmenu_info);
    }
    
    divoom_print("%s:%d it start to menu id: %d\n", __func__, __LINE__,menu_id);
    pmenu_info = divoom_menu_control_create_menu_node(praw_menu_info,
        menu_id, user_data, model_id, event_id);
    if(pmenu_info == NULL)
    {
        if(pdivoom_menu_control_handle->pcur_menu && pdivoom_menu_control_handle->pcur_menu->pdisp)
            pdivoom_menu_control_handle->pcur_menu->pdisp(
                pdivoom_menu_control_handle->pcur_menu->plvgl_obj,
                pdivoom_menu_control_handle->pcur_menu->cur_active_obj);
        divoom_menu_control_drop_failed_placeholder();
        divoom_lvgl_top_control_update_active();
        divoom_menu_control_add_key_event();
        return;
    }
    if(push_old && !divoom_queue_push(pdivoom_menu_control_handle->menu_queue,
            pdivoom_menu_control_handle->pcur_menu))
        pdel_handle = pdivoom_menu_control_handle->pcur_menu;
    pdivoom_menu_control_handle->pcur_menu = pmenu_info;
    divoom_menu_control_mark_current_changed();
    
    
    pdivoom_menu_control_handle->pcur_menu->pdisp(pdivoom_menu_control_handle->pcur_menu->plvgl_obj, NULL);   
    divoom_menu_control_drop_failed_placeholder();
    divoom_lvgl_top_control_update_active(); 
    divoom_menu_control_add_key_event();
    
    if(menu_id == DIVOOM_MENU_CONTROL_ID_MAIN)
    {
        uint8 *type = divoom_system_malloc(sizeof(1));
        if(type)
        {
            *type = 1;
            divoom_queue_pop_all(pdivoom_menu_control_handle->load_font_queue, NULL);
            if(divoom_queue_push(pdivoom_menu_control_handle->load_font_queue, type))
                divoom_system_put_sem(pdivoom_menu_control_handle->load_font_sem);
            else
                divoom_system_free(type);
        }
    }
    else if(menu_id  >= DIVOOM_DISP_CLOCK_MODE)
    {
        uint8 *type = divoom_system_malloc(sizeof(1));
        if(type)
        {
            *type = 2;
            divoom_queue_pop_all(pdivoom_menu_control_handle->load_font_queue, NULL);
            if(divoom_queue_push(pdivoom_menu_control_handle->load_font_queue, type))
                divoom_system_put_sem(pdivoom_menu_control_handle->load_font_sem);
            else
                divoom_system_free(type);
        }
    }
    if(pdel_handle)
    {
        divoom_menu_control_clear_remove_menu(pdel_handle->menu_id);
        divoom_queue_push(pdivoom_menu_control_handle->del_queue, pdel_handle);
        
    }
}

/*********************************************************************
  Function:        divoom_menu_control_load_next_direct
  Description:     如果队列中有这个菜单就不动作，否则压入队列
  Input:           menu_id
  Return:          NONE
**********************************************************************/

void divoom_menu_control_load_next_check_pass(int menu_id, void* user_data, int pop_flag, int model_id, int event_id)
{    
    Pdivoom_menu_control_menu_info pmenu_info = NULL, pfind = NULL;
    menu_id = divoom_menu_control_get_menu_id(menu_id);
    if (divoom_menu_control_poweroff_blocks_menu(menu_id))
        return;
    
    if(pdivoom_menu_control_handle->pcur_menu && pdivoom_menu_control_handle->pcur_menu->menu_id == menu_id)
    {
        if(menu_id == DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID)
        {
            divoom_lvgl_base_dial_menu_update_clock_info(divoom_menu_control_get_clock_mode_menu_handle(), user_data, 0);
        }
        return;
    }
    pmenu_info = divoom_queue_get_cmp(pdivoom_menu_control_handle->menu_queue, &menu_id, divoom_menu_control_compre_menu);
    if(pmenu_info == NULL)
    {
        divoom_menu_control_load_next_direct(menu_id, user_data, pop_flag, model_id, event_id);
        return;
    }
    //将不是menu_id的菜单全部弹出
    divoom_queue_lock(pdivoom_menu_control_handle->menu_queue);
    while(1)
    {
        pmenu_info = divoom_queue_pop(pdivoom_menu_control_handle->menu_queue);
        if(pmenu_info == NULL)
        {
            break;
        }
        if(pmenu_info->menu_id == menu_id && pmenu_info->model_id == model_id && pmenu_info->event_id == event_id)
        {
            break;
        }
        //model_id为0的是延升的菜单，不做统计
        if(pmenu_info->model_id && (pmenu_info->model_id != model_id || pmenu_info->event_id != event_id))
        {
            pfind = pmenu_info;
        }
    }
    divoom_queue_unlock(pdivoom_menu_control_handle->menu_queue);
    if(pfind)
    {
        while(1)
        {
            pmenu_info = divoom_queue_pop(pdivoom_menu_control_handle->menu_queue);
            divoom_print("pop menu id: %d, %d, %d\n", pmenu_info->model_id, pmenu_info->event_id, pmenu_info->menu_id);
            divoom_menu_control_clear_remove_menu(pmenu_info->menu_id);
            divoom_queue_push(pdivoom_menu_control_handle->del_queue, pmenu_info);
            if(pfind == pmenu_info)
            {
                break;
            }
        }
        
    }
}

int divoom_menu_control_pop_model_all_menu_compare(void *item1, void *item2)
{
    Pdivoom_menu_control_menu_info pitem1_info = (Pdivoom_menu_control_menu_info)item1;
    Pdivoom_menu_control_menu_info pitem2_info = (Pdivoom_menu_control_menu_info)item2;

    if(pitem1_info->model_id == pitem2_info->model_id && pitem1_info->event_id == pitem2_info->event_id)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/*********************************************************************
  Function:        divoom_menu_control_load_next_direct
  Description:     马上加载菜单，会将当前菜单压入队列
  Input:           menu_id
  Return:          NONE
**********************************************************************/

void divoom_menu_control_pop_model_all_menu(int model_id, int event_id)
{
    int try_time = 0;
    divoom_menu_control_item_info *praw_menu_info;
    divoom_menu_control_menu_info check_menu_info;
    Pdivoom_menu_control_menu_info pmenu_info, pdel_handle = NULL;

    check_menu_info.model_id = model_id;
    check_menu_info.event_id = event_id;
    do{
        pdel_handle = divoom_queue_pop_cmp(pdivoom_menu_control_handle->menu_queue, &check_menu_info, divoom_menu_control_pop_model_all_menu_compare);
        if(pdel_handle)
        {
            divoom_menu_control_clear_remove_menu(pdel_handle->menu_id);
            divoom_queue_push(pdivoom_menu_control_handle->del_queue, pdel_handle);
        }
    }while(pdel_handle);
    if(divoom_queue_items(pdivoom_menu_control_handle->menu_queue)==0)
    {
        //如果没有菜单将直接退出
        return;
    }

    if(pdivoom_menu_control_handle->pcur_menu && pdivoom_menu_control_handle->pcur_menu->model_id == model_id &&
        pdivoom_menu_control_handle->pcur_menu->event_id == event_id)
    {
        pdel_handle = pdivoom_menu_control_handle->pcur_menu;
        if(pdel_handle && pdel_handle->pause)
        {
            pdel_handle->pause(pdel_handle->plvgl_obj);
        }
        pdivoom_menu_control_handle->pcur_menu = divoom_queue_pop(pdivoom_menu_control_handle->menu_queue);
        divoom_menu_control_mark_current_changed();
		divoom_lvgl_top_main_menu_exit();
		divoom_lvgl_top_ambient_menu_exit();
		pdivoom_menu_control_handle->pcur_menu->pdisp(pdivoom_menu_control_handle->pcur_menu->plvgl_obj, NULL);
        divoom_lvgl_top_control_update_active(); 
        divoom_menu_control_add_key_event();
        divoom_menu_control_clear_remove_menu(pdel_handle->menu_id);
        divoom_queue_push(pdivoom_menu_control_handle->del_queue, pdel_handle);
    }
    
}

/*********************************************************************
  Function:        divoom_menu_control_load_next_direct
  Description:     马上加载菜单，会将当前菜单压入队列
  Input:           menu_id
  Return:          NONE
**********************************************************************/

void divoom_menu_control_pop_menu_id(int menu_id)
{
    Pdivoom_menu_control_menu_info pmenu_info, pdel_handle = NULL;

    do{
        pdel_handle = divoom_queue_pop_cmp(pdivoom_menu_control_handle->menu_queue, &menu_id, divoom_menu_control_compre_menu);
        if(pdel_handle)
        {
            divoom_menu_control_clear_remove_menu(pdel_handle->menu_id);
            divoom_queue_push(pdivoom_menu_control_handle->del_queue, pdel_handle);
        }
    }while(pdel_handle);
    if(divoom_queue_items(pdivoom_menu_control_handle->menu_queue)==0)
    {
        //如果没有菜单将直接退出
        return;
    }

    if(pdivoom_menu_control_handle->pcur_menu && pdivoom_menu_control_handle->pcur_menu->menu_id == menu_id)
    {
        pdel_handle = pdivoom_menu_control_handle->pcur_menu;
        if(pdel_handle && pdel_handle->pause)
        {
            pdel_handle->pause(pdel_handle->plvgl_obj);
        }
        pdivoom_menu_control_handle->pcur_menu = divoom_queue_pop(pdivoom_menu_control_handle->menu_queue);
        divoom_menu_control_mark_current_changed();
		divoom_lvgl_top_main_menu_exit();
		divoom_lvgl_top_ambient_menu_exit();
		pdivoom_menu_control_handle->pcur_menu->pdisp(pdivoom_menu_control_handle->pcur_menu->plvgl_obj, NULL);
        divoom_lvgl_top_control_update_active(); 
        divoom_menu_control_add_key_event();
        divoom_menu_control_clear_remove_menu(pdel_handle->menu_id);
        divoom_queue_push(pdivoom_menu_control_handle->del_queue, pdel_handle);
    }
    
}


/*********************************************************************
  Function:        divoom_menu_control_return_pre
  Description:     退出当前菜单，返回上一级
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_menu_control_return_pre_inner(void)
{
    Pdivoom_menu_control_menu_info pdel_info;
    if(pdivoom_menu_control_handle == NULL)
    {
        return;
    }
    if(divoom_queue_items(pdivoom_menu_control_handle->menu_queue) == 0)
    {
        //已经是主菜单了，无法再退出,将通知系统，进入表盘显示
        //divoom_light_app_return_disp_dial();
        return;
    }
    if(pdivoom_menu_control_handle->pcur_menu && pdivoom_menu_control_handle->pcur_menu->pause)
    {
        pdivoom_menu_control_handle->pcur_menu->pause(pdivoom_menu_control_handle->pcur_menu->plvgl_obj);
    }
    pdel_info = pdivoom_menu_control_handle->pcur_menu;
    pdivoom_menu_control_handle->pcur_menu = divoom_queue_pop_tail(pdivoom_menu_control_handle->menu_queue);
    divoom_menu_control_mark_current_changed();
	divoom_lvgl_top_main_menu_exit();
	divoom_lvgl_top_ambient_menu_exit();
	pdivoom_menu_control_handle->pcur_menu->pdisp(pdivoom_menu_control_handle->pcur_menu->plvgl_obj, pdivoom_menu_control_handle->pcur_menu->cur_active_obj);
    divoom_lvgl_top_control_update_active(); 
    divoom_menu_control_add_key_event();
    if(pdel_info)
    {
        divoom_menu_control_clear_remove_menu(pdel_info->menu_id);
        divoom_queue_push(pdivoom_menu_control_handle->del_queue, pdel_info);
    }
}


/*********************************************************************
  Function:        divoom_menu_control_return_main
  Description:     返回第一级主菜单
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_menu_control_return_main_inner(void)
{
    Pdivoom_menu_control_menu_info pcur_menu = NULL;
    if(pdivoom_menu_control_handle == NULL)
    {
        return;
    }
    if(divoom_queue_items(pdivoom_menu_control_handle->menu_queue) == 0)
    {
        return;
    }
    pcur_menu = pdivoom_menu_control_handle->pcur_menu;
    pdivoom_menu_control_handle->pcur_menu = NULL;
    while(divoom_queue_items(pdivoom_menu_control_handle->menu_queue))
    {
        if(pdivoom_menu_control_handle->pcur_menu)
        {
            divoom_menu_control_clear_remove_menu(pdivoom_menu_control_handle->pcur_menu->menu_id);
            divoom_queue_push(pdivoom_menu_control_handle->del_queue, pdivoom_menu_control_handle->pcur_menu);
        }
        pdivoom_menu_control_handle->pcur_menu = divoom_queue_pop_tail(pdivoom_menu_control_handle->menu_queue);
    }
    divoom_menu_control_mark_current_changed();
	divoom_lvgl_top_main_menu_exit();
	divoom_lvgl_top_ambient_menu_exit();
    if(pcur_menu && pcur_menu->pause)
    {
        pcur_menu->pause(pcur_menu->plvgl_obj);
    }
    pdivoom_menu_control_handle->pcur_menu->pdisp(pdivoom_menu_control_handle->pcur_menu->plvgl_obj, pdivoom_menu_control_handle->pcur_menu->cur_active_obj);
    divoom_lvgl_top_control_update_active(); 
    divoom_menu_control_add_key_event();
    if(pcur_menu)
    {
        divoom_menu_control_clear_remove_menu(pcur_menu->menu_id);
        divoom_queue_push(pdivoom_menu_control_handle->del_queue, pcur_menu);
    }

}


typedef struct _divoom_menu_control_switch_placeholder
{
    lv_obj_t *obj;
    lv_img_dsc_t *snap;
}divoom_menu_control_switch_placeholder, *Pdivoom_menu_control_switch_placeholder;

/* Explicit ownership for a failure screen which outlives the switching stack. */
static divoom_menu_control_switch_placeholder failed_switch_placeholder;

/* 拍到当前帧才做中间页；拍失败不铺黑屏，由调用方先建新页再拆旧页。 */
static void divoom_menu_control_load_switch_placeholder(Pdivoom_menu_control_switch_placeholder out)
{
    lv_obj_t *act;
    lv_img_dsc_t *snap;

    if(out == NULL)
    {
        return;
    }
    memset(out, 0, sizeof(*out));
    act = lv_scr_act();
    snap = act ? lv_snapshot_take(act, LV_COLOR_FORMAT_RGB565) : NULL;
    if(snap)
    {
        out->obj = lv_img_create(NULL);
        if(out->obj)
        {
            lv_obj_set_size(out->obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
            lv_img_set_src(out->obj, snap);
            out->snap = snap;
            lv_scr_load(out->obj);
            lv_refr_now(lv_disp_get_default());
            return;
        }
        lv_snapshot_free(snap);
    }
}

static void divoom_menu_control_drop_switch_placeholder(Pdivoom_menu_control_switch_placeholder placeholder)
{
    if(placeholder == NULL)
    {
        return;
    }
    if(placeholder->obj && lv_obj_is_valid(placeholder->obj))
    {
        if(lv_scr_act() == placeholder->obj)
        {
            return;
        }
        lv_obj_del(placeholder->obj);
    }
    placeholder->obj = NULL;
    if(placeholder->snap)
    {
        lv_snapshot_free(placeholder->snap);
        placeholder->snap = NULL;
    }
}

static void divoom_menu_control_drop_failed_placeholder(void)
{
    divoom_menu_control_drop_switch_placeholder(&failed_switch_placeholder);
}

/* 新根屏已加载后回收过渡快照，再允许视频申请解码内存；仍在显示占位屏时不释放。 */
void divoom_menu_control_release_switch_snapshot(void)
{
    Pdivoom_menu_control_switch_placeholder placeholder;
    uint32 snapshot_bytes;

    if(pdivoom_menu_control_handle == NULL ||
        pdivoom_menu_control_handle->switch_placeholder == NULL)
    {
        return;
    }
    placeholder = pdivoom_menu_control_handle->switch_placeholder;
    snapshot_bytes = placeholder->snap ? placeholder->snap->data_size : 0;
    divoom_menu_control_drop_switch_placeholder(placeholder);
    if(snapshot_bytes > 0 && placeholder->snap == NULL)
    {
        divoom_print("menu: released switch snapshot before video: %lu bytes\n",
            (unsigned long)snapshot_bytes);
    }
}

static void divoom_menu_control_destroy_menu_now(Pdivoom_menu_control_menu_info pmenu)
{
    if(pmenu == NULL)
    {
        return;
    }
    divoom_print("clear menu: %d\n", pmenu->menu_id);
    divoom_menu_control_clear_remove_menu(pmenu->menu_id);
    divoom_popup_menu_destroy_all();
    /* Returning to Wi-Fi uses the regular LVGL refresh timer. */
    if (pmenu->menu_id != DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_CONNECTION_CODE
        || !pdivoom_menu_control_handle->pcur_menu
        || pdivoom_menu_control_handle->pcur_menu->menu_id
            != DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_WIFI)
    {
        lv_refr_now(lv_disp_get_default());
    }
    lv_img_cache_invalidate_src(NULL);
    if(pmenu->pclean)
    {
        pmenu->pclean(pmenu->plvgl_obj);
    }
    divoom_lvgl_font_manage_clear_clock_font(pmenu->menu_id, NULL);
    divoom_system_free(pmenu);
}

static void divoom_menu_control_destroy_del_queue_now(void)
{
    while(divoom_queue_items(pdivoom_menu_control_handle->del_queue))
    {
        Pdivoom_menu_control_menu_info pmenu = divoom_queue_pop(pdivoom_menu_control_handle->del_queue);
        if(pmenu)
        {
            divoom_menu_control_destroy_menu_now(pmenu);
        }
    }
}

/*********************************************************************
  Function:        divoom_menu_control_clear_all_load_inner
  Description:     马上将所有的菜单全部弹出，加载新的菜单
  Input:           none
  Return:          NONE
**********************************************************************/

static void divoom_menu_control_clear_all_load_direct_inner(int menu_id, void* user_data,
    int model_id, int event_id, int force_recreate)
{
    int try_time = 0;
    int deferred_pause = 0;
    int skip_switch_snapshot = 0;
    int show_transfer_first;
    divoom_menu_control_item_info *praw_menu_info;
    Pdivoom_menu_control_menu_info pcur_menu = NULL, pfind_menu = NULL;
    divoom_menu_control_switch_placeholder switch_placeholder;

    memset(&switch_placeholder, 0, sizeof(switch_placeholder));
    menu_id = divoom_menu_control_get_menu_id(menu_id);
    /* A preempted/cancelled resource event may still have an asynchronous page request queued. */
    if (menu_id == DIVOOM_MENU_CONTROL_ID_ACCEPT_APP_PHOTO_VIEW &&
        model_id == DIVOOM_MODEL_OTA && event_id == DIVOOM_OTA_SERVER_SYSTEM_FILE_UPDATE &&
        !divoom_update_system_file_is_resource_updating())
        return;
    show_transfer_first = menu_id == DIVOOM_MENU_CONTROL_ID_ACCEPT_APP_PHOTO_VIEW &&
        (user_data != NULL || divoom_update_system_file_is_resource_updating());
    if(menu_id == DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID &&
        divoom_update_system_file_is_resource_updating())
    {
        /* Discard a dial request queued before the resource page took ownership. */
        divoom_print("menu: defer dial while system resources updating\n");
        return;
    }
    if(menu_id == DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID &&
        divoom_light_app_get_clock_download_session())
    {
        /* 旧播放请求可能排在新配置的下载等待页之前，不能消费半更新的素材。 */
        divoom_print("menu: defer dial while clock resources pending\n");
        return;
    }
    if(show_transfer_first && user_data != NULL)
    {
        if(!divoom_light_app_clock_download_session_is_current((uint32)(uintptr_t)user_data))
            return;
    }
    if (divoom_menu_control_poweroff_blocks_menu(menu_id))
        return;
    divoom_print("menu: clear_all_load_direct enter menu=%d model=%d event=%d finish_start=%d cur_menu=%d\n",
                 menu_id, model_id, event_id, divoom_main_get_finish_start_up(),
                 pdivoom_menu_control_handle && pdivoom_menu_control_handle->pcur_menu
                     ? pdivoom_menu_control_handle->pcur_menu->menu_id : -1);
    while(pdivoom_menu_control_handle == NULL)
    {
        divoom_system_delay_ms(100);
        try_time ++;
        if(0 == (try_time % 20))
        {
            divoom_print("%s:%d wait %d\n", __func__, __LINE__, try_time);
        }
    }


    if(!force_recreate && pdivoom_menu_control_handle->pcur_menu
        && pdivoom_menu_control_handle->pcur_menu->menu_id == menu_id)
    {
        divoom_print("menu: clear_all_load_direct already on menu=%d, skip reload\n", menu_id);
        if(menu_id == DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID)
        {
            divoom_lvgl_base_dial_menu_update_clock_info(divoom_menu_control_get_clock_mode_menu_handle(), user_data, 0);
        }
        else if(show_transfer_first)
        {
            /* 连续添加沿用当前页面，重绑会话并恢复计时，避免重复分配整页。 */
            divoom_lvgl_gallery_transfer_bind_session(
                pdivoom_menu_control_handle->pcur_menu->plvgl_obj, (uint32)(uintptr_t)user_data);
        }
        return;
    }


    menu_id = divoom_menu_control_get_menu_id(menu_id);
    while(divoom_queue_items(pdivoom_menu_control_handle->menu_queue))
    {
        pcur_menu = divoom_queue_pop_tail(pdivoom_menu_control_handle->menu_queue);
        if(pcur_menu)
        {
            if(!force_recreate && pcur_menu->menu_id == menu_id)
            {
                pfind_menu = pcur_menu;
                break;
            }
            divoom_menu_control_clear_remove_menu(pcur_menu->menu_id);
            divoom_queue_push(pdivoom_menu_control_handle->del_queue, pcur_menu);
        }
    }

    pcur_menu = pdivoom_menu_control_handle->pcur_menu;
    praw_menu_info = divoom_menu_control_get_menu_info(menu_id);
    if(praw_menu_info)
    {
        /* 注册本次切页的临时资源，歌词页可在加载根屏后、起播前归还快照。 */
        pdivoom_menu_control_handle->switch_placeholder = &switch_placeholder;
        /* 普通 UI 拍到快照才先拆；旧视频先暂停再起新页，旧页面对象在新根屏加载后销毁。 */
        Pdivoom_menu_control_menu_info pmenu_info = divoom_queue_pop_cmp(pdivoom_menu_control_handle->del_queue, &menu_id, divoom_menu_control_compre_menu);
        if(pmenu_info)
        {
            divoom_print("we will dele and reload: %d\n", menu_id);
            pmenu_info->pclean(pmenu_info->plvgl_obj);
            divoom_lvgl_font_manage_clear_clock_font(pmenu_info->menu_id, NULL);
            
            divoom_system_free(pmenu_info);
        }

        if(pcur_menu && !show_transfer_first && divoom_menu_control_is_video_disp())
        {
            /* 新页面接管前不展示旧视频页的普通 UI；解码资源仍先释放。 */
            pdivoom_menu_control_handle->video_ui_handoff = 1;
        }
        if(pcur_menu && pcur_menu->menu_id == DIVOOM_MENU_CONTROL_ID_SYSTEM_POWER_ON &&
            !divoom_lvgl_poweron_is_video_phase() && divoom_menu_control_is_video_disp())
        {
            /* 开机片已结束：在新页面 pdisp 前恢复 UI 层，避免透明模式遗留到普通表盘。 */
            divoom_menu_control_change_disp_mode(NULL, 0);
        }
        if(pcur_menu && pcur_menu->pause)
        {
            if(show_transfer_first || divoom_menu_control_is_video_disp())
            {
                deferred_pause = 1;
            }
            else
            {
                pcur_menu->pause(pcur_menu->plvgl_obj);
            }
        }
        if(pcur_menu && pcur_menu->menu_id == DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID
            && divoom_lvgl_base_dial_menu_is_empty_photo_album(pcur_menu->plvgl_obj))
        {
            skip_switch_snapshot = 1;
        }
        if(pcur_menu && !show_transfer_first &&
            !divoom_menu_control_is_video_disp() && !skip_switch_snapshot)
        {
            divoom_menu_control_load_switch_placeholder(&switch_placeholder);
            if(switch_placeholder.snap)
            {
                pdivoom_menu_control_handle->pcur_menu = NULL;
                divoom_menu_control_destroy_menu_now(pcur_menu);
                pcur_menu = NULL;
                malloc_trim(0);
            }
        }
        /* video_mode 是当前是不是视频显示，不是下一页是不是视频。还在普通 UI（从表盘/列表进睡眠也算）：先拆 del_queue，避免视频起播后再拆主菜单抢 cedarv。已经在播视频：这里不拆，等离开视频、video_mode 清 0 后由事件循环末尾再拆。 */
        if(!show_transfer_first && !divoom_menu_control_is_video_disp())
        {
            divoom_menu_control_destroy_del_queue_now();
        }

        /* 全局播放器和视频图层必须先由旧页归还，再由新页初始化、起播。
         * 否则旧开机页/视频页的 pause 会把刚启动的新视频一起停止。
         * 下载等待页不接管播放器，保留先提交等待画面再暂停旧页的顺序。 */
        if(deferred_pause && !show_transfer_first && pcur_menu && pcur_menu->pause)
        {
            divoom_print("menu: pause old video before new page: %d -> %d\n",
                pcur_menu->menu_id, menu_id);
            pcur_menu->pause(pcur_menu->plvgl_obj);
            deferred_pause = 0;
        }

        /* The font registry is keyed by menu ID, not page generation. Finish
         * the old same-ID page before a new one can register its fonts. */
        if(pfind_menu == NULL && pcur_menu && pcur_menu->menu_id == menu_id)
        {
            switch_placeholder.obj = lv_img_create(NULL);
            if(switch_placeholder.obj == NULL)
                goto init_failed;
            lv_obj_set_style_bg_color(switch_placeholder.obj, lv_color_black(), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(switch_placeholder.obj, LV_OPA_COVER, LV_PART_MAIN);
            lv_scr_load(switch_placeholder.obj);
            if(deferred_pause && pcur_menu->pause)
            {
                pcur_menu->pause(pcur_menu->plvgl_obj);
                deferred_pause = 0;
            }
            pdivoom_menu_control_handle->pcur_menu = NULL;
            divoom_menu_control_destroy_menu_now(pcur_menu);
            pcur_menu = NULL;
        }
        if(pfind_menu == NULL)
        {
            Pdivoom_menu_control_menu_info new_menu = divoom_menu_control_create_menu_node(
                praw_menu_info, menu_id, user_data, model_id, event_id);
            if(new_menu == NULL)
                goto init_failed;
            pdivoom_menu_control_handle->pcur_menu = new_menu;
			divoom_lvgl_top_main_menu_exit();
			divoom_lvgl_top_ambient_menu_exit();
	        pdivoom_menu_control_handle->pcur_menu->pdisp(pdivoom_menu_control_handle->pcur_menu->plvgl_obj, NULL);
		}
		else
		{   
			pdivoom_menu_control_handle->pcur_menu = pfind_menu;
            if(menu_id  >= DIVOOM_DISP_CLOCK_MODE)
            {
                divoom_lvgl_base_dial_menu_update_clock_info(pdivoom_menu_control_handle->pcur_menu->plvgl_obj,user_data, 0);
            }
			divoom_lvgl_top_main_menu_exit();
			divoom_lvgl_top_ambient_menu_exit();
            pdivoom_menu_control_handle->pcur_menu->pdisp(pdivoom_menu_control_handle->pcur_menu->plvgl_obj, pdivoom_menu_control_handle->pcur_menu->cur_active_obj);

		}
        if(pdivoom_menu_control_handle->video_ui_handoff)
        {
            uint8 restore_ui = pdivoom_menu_control_handle->video_ui_handoff == 2;
            pdivoom_menu_control_handle->video_ui_handoff = 0;
            if(restore_ui)
            {
                /* pdisp 已加载目标根屏，普通菜单只在这里恢复并提交一次。 */
                divoom_menu_control_change_disp_mode(lv_scr_act(), 0);
            }
        }
        if(show_transfer_first)
        {
            /* lv_refr_now 本身不会开启 framebuffer 提交或硬件 UI 层。
             * 等待页先接管显示，再停旧表盘，避免首帧只画进隐藏缓冲区。 */
            divoom_menu_control_change_disp_mode(lv_scr_act(), 0);
            lv_refr_now(lv_disp_get_default());
            divoom_print("menu: clock wait first frame submitted\n");
        }
        if(pcur_menu)
        {
            if(deferred_pause && pcur_menu->pause)
            {
                pcur_menu->pause(pcur_menu->plvgl_obj);
            }
            divoom_menu_control_destroy_menu_now(pcur_menu);
            pcur_menu = NULL;
        }
        if(show_transfer_first && !divoom_menu_control_is_video_disp())
        {
            divoom_menu_control_destroy_del_queue_now();
        }
        if(show_transfer_first)
        {
            /* 旧页的停止/清理可能改写图层或 framebuffer，结束后重新提交等待页。 */
            divoom_menu_control_change_disp_mode(lv_scr_act(), 0);
            lv_obj_invalidate(lv_scr_act());
            lv_refr_now(lv_disp_get_default());
            divoom_print("menu: clock wait frame restored after old cleanup\n");
        }
        divoom_menu_control_drop_failed_placeholder();
        divoom_menu_control_drop_switch_placeholder(&switch_placeholder);
        pdivoom_menu_control_handle->switch_placeholder = NULL;
        divoom_menu_control_mark_current_changed();
        divoom_lvgl_top_control_update_active(); 
        divoom_menu_control_add_key_event();
        if(menu_id == DIVOOM_MENU_CONTROL_ID_MAIN)
        {
            uint8 *type = divoom_system_malloc(sizeof(1));
            if(type)
            {
                *type = 1;
                divoom_queue_pop_all(pdivoom_menu_control_handle->load_font_queue, NULL);
                if(divoom_queue_push(pdivoom_menu_control_handle->load_font_queue, type))
                    divoom_system_put_sem(pdivoom_menu_control_handle->load_font_sem);
                else
                    divoom_system_free(type);
            }
        }
        else if(menu_id  >= DIVOOM_DISP_CLOCK_MODE)
        {
            uint8 *type = divoom_system_malloc(sizeof(1));
            if(type)
            {
                *type = 2;
                divoom_queue_pop_all(pdivoom_menu_control_handle->load_font_queue, NULL);
                if(divoom_queue_push(pdivoom_menu_control_handle->load_font_queue, type))
                    divoom_system_put_sem(pdivoom_menu_control_handle->load_font_sem);
                else
                    divoom_system_free(type);
            }
        }
        return;
init_failed:
        pdivoom_menu_control_handle->pcur_menu = pcur_menu;
        if(pcur_menu && pcur_menu->pdisp)
            pcur_menu->pdisp(pcur_menu->plvgl_obj, pcur_menu->cur_active_obj);
        else if(switch_placeholder.obj && lv_obj_is_valid(switch_placeholder.obj) &&
            lv_scr_act() == switch_placeholder.obj)
        {
            /* Detach before freeing the large active snapshot; keep only its screen. */
            if(switch_placeholder.snap)
            {
                lv_img_set_src(switch_placeholder.obj, NULL);
                lv_snapshot_free(switch_placeholder.snap);
            }
            switch_placeholder.snap = NULL;
            divoom_menu_control_drop_failed_placeholder();
            failed_switch_placeholder = switch_placeholder;
            memset(&switch_placeholder, 0, sizeof(switch_placeholder));
        }
        if(pdivoom_menu_control_handle->video_ui_handoff)
        {
            uint8 restore_ui = pdivoom_menu_control_handle->video_ui_handoff == 2;
            pdivoom_menu_control_handle->video_ui_handoff = 0;
            if(restore_ui)
                divoom_menu_control_change_disp_mode(lv_scr_act(), 0);
        }
        divoom_menu_control_drop_switch_placeholder(&switch_placeholder);
        pdivoom_menu_control_handle->switch_placeholder = NULL;
        divoom_menu_control_mark_current_changed();
        if(pcur_menu)
        {
            divoom_lvgl_top_control_update_active();
            divoom_menu_control_add_key_event();
        }
    }
    else
    {
        divoom_print("menu: clear_all_load_direct no menu info for id=%d\n", menu_id);
    }

}

void divoom_menu_control_clear_all_load_direct(int menu_id, void* user_data, int model_id, int event_id)
{
    divoom_menu_control_clear_all_load_direct_inner(menu_id, user_data, model_id, event_id, 0);
}

static void divoom_menu_control_process_event_queue_once(int lock_already_held)
{
    Pdivoom_menu_control_event_info event_info;

    if (pdivoom_menu_control_handle == NULL)
    {
        return;
    }

    if (divoom_queue_items(pdivoom_menu_control_handle->event_queue))
    {
        event_info = divoom_queue_pop(pdivoom_menu_control_handle->event_queue);
        if (event_info)
        {
            if(divoom_main_get_finish_start_up() == 0)
            {
                if((event_info->type == DIVOOM_MENU_CONTROL_EVENT_NEXT
                    || event_info->type == DIVOOM_MENU_CONTROL_EVENT_CLEAR_ALL_LOAD
                    || event_info->type == DIVOOM_MENU_CONTROL_EVENT_SWITCH_LANGUAGE)
                    && event_info->menu_id != DIVOOM_MENU_CONTROL_ID_SYSTEM_POWER_ON)
                {
                    divoom_print("menu: defer event type=%d menu=%d (finish_start=0)\n",
                                 event_info->type, event_info->menu_id);
                    divoom_queue_push(pdivoom_menu_control_handle->event_queue, event_info);
                    return ;
                }
            }
            if (divoom_onoff_is_poweroff_active()) {
                if (divoom_menu_control_get_active_menu_id()
                    == DIVOOM_MENU_CONTROL_ID_POWER_SHUTDOWN) {
                    divoom_print("menu: drop event type=%d menu=%d (on shutdown page)\n",
                                 event_info->type, event_info->menu_id);
                    divoom_system_free(event_info);
                    return;
                }
                if (!(event_info->type == DIVOOM_MENU_CONTROL_EVENT_CLEAR_ALL_LOAD
                      && event_info->menu_id == DIVOOM_MENU_CONTROL_ID_POWER_SHUTDOWN)) {
                    divoom_print("menu: drop event type=%d menu=%d (poweroff)\n",
                                 event_info->type, event_info->menu_id);
                    divoom_system_free(event_info);
                    return;
                }
            }
            switch (event_info->type)
            {
                case DIVOOM_MENU_CONTROL_EVENT_PHOTO_CHANGE_SLIDE:
                    if (pdivoom_menu_control_handle->pcur_menu
                        && pdivoom_menu_control_handle->pcur_menu->menu_id == DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID)
                    {
                        divoom_lvgl_base_dial_menu_change_photo_album(pdivoom_menu_control_handle->pcur_menu->plvgl_obj);
                    }
                    break;
                case DIVOOM_MENU_CONTROL_EVENT_REDISPLAY_CURRENT:
                    if (pdivoom_menu_control_handle->pcur_menu
                        && pdivoom_menu_control_handle->pcur_menu->pdisp)
                    {
                        pdivoom_menu_control_handle->pcur_menu->pdisp(
                            pdivoom_menu_control_handle->pcur_menu->plvgl_obj,
                            pdivoom_menu_control_handle->pcur_menu->cur_active_obj);
                        divoom_lvgl_top_control_update_active();
                        divoom_menu_control_add_key_event();
                        divoom_main_apply_sys_light();
                    }
                    break;
                case DIVOOM_MENU_CONTROL_EVENT_POP_MENU:
                    divoom_menu_control_pop_menu_id(event_info->menu_id);
                    break;

                case DIVOOM_MENU_CONTROL_EVENT_POP_MODEL_ALL:
                    divoom_menu_control_pop_model_all_menu(event_info->model_id, event_info->event_id);
                    break;

                case DIVOOM_MENU_CONTROL_EVENT_NEXT_POP_CUR:
                    divoom_menu_control_load_next_direct(event_info->menu_id, event_info->user_data, 1, event_info->model_id, event_info->event_id);
                    break;

                case DIVOOM_MENU_CONTROL_EVENT_NEXT:
                    divoom_menu_control_load_next_direct(event_info->menu_id, event_info->user_data, 0, event_info->model_id, event_info->event_id);
                    break;

                case DIVOOM_MENU_CONTROL_EVENT_NEXT_AND_PASS:
                    divoom_menu_control_load_next_check_pass(event_info->menu_id, event_info->user_data, 0, event_info->model_id, event_info->event_id);
                    break;

                case DIVOOM_MENU_CONTROL_EVENT_PRE:
                    if (pdivoom_menu_control_handle->pcur_menu
                        && pdivoom_menu_control_handle->pcur_menu->menu_id == event_info->menu_id
                        && s_menu_control_generation == event_info->menu_generation)
                    {
                        divoom_menu_control_return_pre_inner();
                    }
                    else
                    {
#if defined(DIVOOM_SIMULATOR) && DIVOOM_SIMULATOR
                        s_menu_control_sim_stale_pre_ignored = 1;
#endif
                        divoom_print("menu: ignore stale PRE source=%d/%u current=%d/%u\n",
                                     event_info->menu_id,
                                     (unsigned)event_info->menu_generation,
                                     pdivoom_menu_control_handle->pcur_menu
                                         ? pdivoom_menu_control_handle->pcur_menu->menu_id : -1,
                                     (unsigned)s_menu_control_generation);
                    }
                    break;

                case DIVOOM_MENU_CONTROL_EVENT_RETURN_MAIN:
                    divoom_menu_control_return_main_inner();
                    break;

                case DIVOOM_MENU_CONTROL_EVENT_CLEAR_ALL_LOAD:
                    if(event_info->menu_id == DIVOOM_MENU_CONTROL_ID_ACCEPT_APP_PHOTO_VIEW &&
                        event_info->user_data != NULL &&
                        !divoom_light_app_clock_download_session_is_current(
                            (uint32)(uintptr_t)event_info->user_data))
                    {
                        /* 表盘已就绪，丢弃迟到的等待页，避免先闪等待再切回。 */
                        divoom_print("menu: skip completed clock wait\n");
                        break;
                    }
                    divoom_print("menu: process CLEAR_ALL_LOAD menu=%d model=%d event=%d\n",
                                 event_info->menu_id, event_info->model_id, event_info->event_id);
                    divoom_menu_control_clear_all_load_direct(event_info->menu_id, event_info->user_data, event_info->model_id, event_info->event_id);
                    break;

                case DIVOOM_MENU_CONTROL_EVENT_SWITCH_LANGUAGE:
                {
                    if (s_menu_control_generation != event_info->menu_generation)
                    {
                        divoom_print("menu: ignore stale language switch lang=%d generation=%u/%u\n",
                            event_info->lang_id,
                            (unsigned)event_info->menu_generation,
                            (unsigned)s_menu_control_generation);
                        break;
                    }

                    /* 旧页面均持有创建时复制的文案；先换字典，再强制新建整条可返回路由。 */
                    divoom_popup_menu_destroy_all();
                    divoom_load_message_set_lang(event_info->lang_id);
                    divoom_menu_control_clear_all_load_direct_inner(
                        event_info->root_menu_id, NULL, 0, 0, 1);
                    if (event_info->menu_id != event_info->root_menu_id)
                    {
                        divoom_menu_control_load_next_direct(event_info->menu_id, NULL, 0, 0, 0);
                    }
                    divoom_print("menu: language switched lang=%d route=%d->%d\n",
                        event_info->lang_id, event_info->root_menu_id, event_info->menu_id);
                    break;
                }
            }
            divoom_system_free(event_info);
        }
    }
    if (!divoom_menu_control_is_video_disp())
    {
        divoom_menu_control_destroy_del_queue_now();
    }
}

void divoom_menu_control_event_queue(lv_timer_t *user_data)
{
        if (divoom_queue_items(pdivoom_menu_control_handle->event_queue)
            || divoom_queue_items(pdivoom_menu_control_handle->del_queue))
        {
            divoom_menu_control_process_event_queue_once(1);
        }
}

#if defined(DIVOOM_SIMULATOR) && DIVOOM_SIMULATOR
void divoom_menu_control_poll(void)
{
    divoom_menu_control_process_event_queue_once(1);
}
#endif


/*********************************************************************
  Function:        divoom_menu_control_wait_load_system_font
  Description:     等待歌词字体加载
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_menu_control_wait_load_system_font(void)
{
    int try_times = 0;
    if(pdivoom_menu_control_handle->load_music_font_flag == 0)
    {
        uint8 *type = divoom_system_malloc(1);
        if(type)
        {
            *type = 1;
            divoom_queue_push(pdivoom_menu_control_handle->load_font_queue, type);
            divoom_system_put_sem(pdivoom_menu_control_handle->load_font_sem);
            while(pdivoom_menu_control_handle->load_music_font_flag == 0 && try_times < 1000)
            {
                try_times ++;
                divoom_system_delay_ms(10);
                if(try_times % 100 == 0)
                {
                    divoom_print("%s wait: %d\n", __func__, try_times);
                }
            }
            divoom_print("%s wait time: %d\n", __func__, try_times);
        }
    }
}

/*********************************************************************
  Function:        divoom_menu_control_check_menu_font
  Description:     检查歌词字体是否加载
  Input:           none
  Return:          1 or 0
**********************************************************************/

int divoom_menu_control_check_menu_font(void)
{
    return pdivoom_menu_control_handle->load_music_font_flag;
}

void divoom_menu_control_load_system_font_task(void *data)
{
    uint8 *type;

    pdivoom_menu_control_handle->load_music_font_flag = 0;
	while(1)
	{
        if(divoom_queue_items(pdivoom_menu_control_handle->load_font_queue) == 0)
        {
            divoom_system_get_sem(pdivoom_menu_control_handle->load_font_sem);
        }
        type = divoom_queue_pop(pdivoom_menu_control_handle->load_font_queue);
        if(type)
        {
            /* lv_font_load/lv_font_free 操作 LVGL 堆，必须持 LVGL 锁，
               否则与 UI 线程并发分配/渲染会破坏 LVGL 堆并导致访问越界崩溃 */
            divoom_get_lvgl_lock();
            if(*type == 1)
            {
			    divoom_menu_control_load_system_font(1);
                pdivoom_menu_control_handle->load_music_font_flag = 1;
            }
            else
            {
			    divoom_menu_control_destroy_system_font(1);
                pdivoom_menu_control_handle->load_music_font_flag = 0;
            }
            divoom_free_lvgl_lock();
            divoom_system_free(type);
        }
	}
}


/*********************************************************************
  Function:        divoom_menu_control_init
  Description:     菜单模块初始化
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_menu_control_init(void)
{
    int lang = divoom_main_set_lang_flag(-1);
    pdivoom_menu_control_handle = (Pdivoom_menu_control_ctrl)divoom_system_malloc(sizeof(divoom_menu_control_ctrl));
    pdivoom_menu_control_handle->pcur_menu = NULL;
    pdivoom_menu_control_handle->video_mode = 0;
    pdivoom_menu_control_handle->menu_queue = divoom_queue_init(100, __func__, __LINE__);
    pdivoom_menu_control_handle->event_queue = divoom_queue_init(100, __func__, __LINE__);
    pdivoom_menu_control_handle->remove_queue = divoom_queue_init(100, __func__, __LINE__);
    pdivoom_menu_control_handle->del_queue = divoom_queue_init(100, __func__, __LINE__);
    pdivoom_menu_control_handle->load_font_queue = divoom_queue_init(50, __func__, __LINE__);
    pdivoom_menu_control_handle->wait_sem = divoom_system_create_sem(1, 0);
	pdivoom_menu_control_handle->menu_font = NULL;
	pdivoom_menu_control_handle->lyric_font = NULL;
    pdivoom_menu_control_handle->load_font_sem = divoom_system_create_sem(1, 0);
    //这个位置进行LVGL的初始化
    //divoom_system_create_task(divoom_menu_control_event_queue, "menu_event", pdivoom_menu_control_handle, DIVOOM_TASK_STACK_MENU_EVENT);
    if(lang == -1)
    {
        divoom_load_message_init_lang(DIVOOM_LVGL_MENU_LANG_SC);
    }
    else
    {
        //divoom_load_message_init_lang(DIVOOM_LVGL_MENU_LANG_SC);
        divoom_load_message_init_lang(lang);
    }
    divoom_popup_menu_init();
    divoom_lvgl_font_manage_init();
#if 1
    pdivoom_menu_control_handle->lvgl_key_group = lv_group_create();
    {
        lv_indev_t *indev = NULL;

#if defined(DIVOOM_SIMULATOR) && DIVOOM_SIMULATOR
        indev = sim_pal_get_lvgl_keypad_indev();
#endif
        if (indev == NULL)
        {
            indev = lv_indev_get_next(NULL);
        }

        lv_group_set_default(pdivoom_menu_control_handle->lvgl_key_group);
        if (indev)
        {
            lv_indev_set_group(indev, pdivoom_menu_control_handle->lvgl_key_group);
        }
    }
#endif
#if 0 /* legacy: product test early return — disabled for MVP pt_ui */
	if (divoom_main_get_poweron_mode() == DIVOOM_ONOFF_MODE_TEST) {
		lv_timer_create(divoom_menu_control_event_queue, 20, pdivoom_menu_control_handle);
		return;
	}
#endif
    #ifdef DIVOOM_WIFI_RF_TEST
    pdivoom_menu_control_handle->first_menu_id = DIVOOM_MENU_CONTROL_ID_MAIN;
    #else
    //加载第一个页面
    if(lang == -1)
    {
        //设置语言
        pdivoom_menu_control_handle->first_menu_id = DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_LANGUGE;
    }
    else if(divoom_main_set_wifi_config_status(-1) == 0)
    {
        //配网
        pdivoom_menu_control_handle->first_menu_id = DIVOOM_MENU_CONTROL_ID_ASTROTOO_OOBE_CONFIG_WIFI;
    }
    else
    {
#ifndef DIVOOM_SIMULATOR
        pdivoom_menu_control_handle->first_menu_id = DIVOOM_MENU_CONTROL_ID_SYSTEM_POWER_ON;
#else
        pdivoom_menu_control_handle->first_menu_id = DIVOOM_MENU_CONTROL_ID_MAIN;
#endif
    }
    #endif

	if(divoom_main_get_dial_demo_flag())
	{
		pdivoom_menu_control_handle->first_menu_id = DIVOOM_MENU_CONTROL_ID_MAIN;
	}


    //pdivoom_menu_control_handle->first_menu_id = DIVOOM_MENU_CONTROL_ID_PHOTO_SLIDE_EFFECT;
    /*
     * 闹钟 RTC 开机：与首页是否为 POWER_ON 无关，必须进 WAKE_HOLD。
     * 已配网首页是开机动画（r42369），不能再要求 first==MAIN，否则会播完视频后黑屏不响。
     */
#ifdef DIVOOM_TIME_FRAME_SPEAKER
    if (divoom_main_get_poweron_mode() == DIVOOM_ONOFF_MODE_CLOCK)
    {
        pdivoom_menu_control_handle->first_menu_id = DIVOOM_MENU_CONTROL_ID_ALARM_CLOCK_WAKE_HOLD;
        divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_ALARM_CLOCK_WAKE_HOLD, NULL, 0, 0);
        /* 跳过开机动画：发 0x5f、结束 pending，否则 0x19 会被 F101 一直 defer */
        divoom_lvgl_poweron_notify_jl_audio_ready();
    }
    else
#endif
    if(pdivoom_menu_control_handle->first_menu_id == DIVOOM_MENU_CONTROL_ID_MAIN &&
		divoom_main_is_app_exit_flag())
    {
        divoom_lvgl_poweron_notify_boot_ready();
    }
#ifdef DIVOOM_PRODUCT_ASTRO_TOO
    else
    {
        divoom_menu_control_load_next(pdivoom_menu_control_handle->first_menu_id, NULL, 0, 0);
        if (pdivoom_menu_control_handle->first_menu_id != DIVOOM_MENU_CONTROL_ID_SYSTEM_POWER_ON
            && pdivoom_menu_control_handle->first_menu_id != DIVOOM_MENU_CONTROL_ID_ALARM_CLOCK_WAKE_HOLD) {
            divoom_print("poweron: skip boot video, mark boot ready\n");
            divoom_lvgl_poweron_notify_boot_ready();
        }
    }
#else
    else
    {
        divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_POWER_ON, NULL, 0, 0);
    }
#endif
    lv_timer_create(divoom_menu_control_event_queue, 20, pdivoom_menu_control_handle);
#ifdef DIVOOM_PRODUCT_ASTRO_TOO
    divoom_lvgl_top_radio_wait_init();
#endif
	//divoom_system_create_task_pro(divoom_menu_control_load_system_font_task,  "dm_menu_font", NULL, 512, 1, 0, 0);
}

/*********************************************************************
  Function:        divoom_menu_control_menu_get_first_menu_id
  Description:     获取初始菜单页面ID
  Input:
  Return:          菜单级数
**********************************************************************/

int divoom_menu_control_menu_get_first_menu_id(void)
{
    return pdivoom_menu_control_handle->first_menu_id;
        
}

/*********************************************************************
  Function:        divoom_menu_control_menu_items
  Description:     当前菜单有多少级
  Input:
  Return:          菜单级数
**********************************************************************/

int divoom_menu_control_menu_items(void)
{
    int tol_items = divoom_queue_items(pdivoom_menu_control_handle->menu_queue);

    if(pdivoom_menu_control_handle->pcur_menu)
    {
        tol_items ++;
    }

    return tol_items;
}

/*********************************************************************
  Function:        divoom_menu_control_add_all_remove_menu
  Description:     将移除队列中所有要移除的菜单
  Input:           menu_id
  Return:          NONEs
**********************************************************************/

void divoom_menu_control_add_all_remove_menu(void)
{
    int *pop_id;
    Pdivoom_menu_control_event_info event_info;

    while(divoom_queue_items(pdivoom_menu_control_handle->remove_queue))
    {
        pop_id = divoom_queue_pop(pdivoom_menu_control_handle->remove_queue);
        if(pop_id == NULL)
        {
            break;
        }
        *pop_id = divoom_menu_control_get_menu_id(*pop_id);
        event_info = divoom_system_malloc(sizeof(divoom_menu_control_event_info));
        if(event_info)
        {
            event_info->type = DIVOOM_MENU_CONTROL_EVENT_POP_MENU;
            event_info->menu_id = *pop_id;
            divoom_queue_push(pdivoom_menu_control_handle->event_queue, event_info);
        }
        divoom_system_free(pop_id);
    }
    divoom_system_put_sem(pdivoom_menu_control_handle->wait_sem);
}


/*********************************************************************
  Function:        divoom_menu_control_load_next
  Description:     加载菜单，会将当前菜单压入队列
  Input:           menu_id
                   user_data  这个外面要申请，在菜单初始化时再由菜单级释放
  Return:          NONE
**********************************************************************/

void divoom_menu_control_load_next(int menu_id, void *user_data, int model_id, int event_id)
{
    int try_time = 0;
    Pdivoom_menu_control_event_info event_info;

    divoom_menu_control_item_info *praw_menu_info;

    while(pdivoom_menu_control_handle == NULL)
    {
        divoom_system_delay_ms(100);
        try_time ++;
        if(0 == (try_time % 20))
        {
            divoom_print("%s:%d wait %d\n", __func__, __LINE__, try_time);
        }
    }
    menu_id = divoom_menu_control_get_menu_id(menu_id);
    if (divoom_menu_control_poweroff_blocks_menu(menu_id))
        return;
    
    praw_menu_info = divoom_menu_control_get_menu_info(menu_id);
    if(praw_menu_info && praw_menu_info->id == DIVOOM_DISP_CLOCK_MODE)
    {
        //这个模式统一认为是APP触发的，将认为清除之前所有模式
        divoom_menu_control_clearall_load(menu_id, user_data, model_id, event_id);
        return;
    }
    event_info = divoom_system_malloc(sizeof(divoom_menu_control_event_info));
    if(event_info)
    {
        event_info->type = DIVOOM_MENU_CONTROL_EVENT_NEXT;
        event_info->menu_id = menu_id;
        event_info->model_id = model_id;
        event_info->event_id = event_id;
        event_info->user_data = user_data;
        divoom_queue_push(pdivoom_menu_control_handle->event_queue, event_info);
    }
    divoom_menu_control_add_all_remove_menu();
}

int divoom_menu_control_clear_remove_menu_compare(void *item1, void *item2)
{
    return *(int *)item1 == *(int *)item2 ?0:1;
}


/*********************************************************************
  Function:        divoom_menu_control_clear_remove_menu
  Description:     移除正要删除的菜单
  Input:           menu_id
  Return:          NONE
**********************************************************************/

void divoom_menu_control_clear_remove_menu(int menu_id)
{
    int *pop_id ;
    while(1)
    {
        pop_id =divoom_queue_pop_cmp(pdivoom_menu_control_handle->remove_queue, &menu_id, divoom_menu_control_clear_remove_menu_compare);

        if(pop_id)
        {
            divoom_system_free(pop_id);
        }
        else
        {
            break;
        }
    }
}


/*********************************************************************
  Function:        divoom_menu_control_remove_menu
  Description:     弹出某个菜单,这个弹出菜单的时机，是在有新菜单压入之后
  Input:           menu_id
  Return:          NONE
**********************************************************************/

void divoom_menu_control_remove_menu(int menu_id)
{
    int *pop_id;    
    Pdivoom_menu_control_menu_info pdel_handle = NULL;
    
    pdel_handle = divoom_queue_get_cmp(pdivoom_menu_control_handle->del_queue, &menu_id, divoom_menu_control_compre_menu);
    if(pdel_handle)
    {
        divoom_print("pass remove menu id: %d\n", menu_id);
        return;
    }
    
    pop_id = divoom_system_malloc(sizeof(int));

    *pop_id = menu_id;
    divoom_queue_push(pdivoom_menu_control_handle->remove_queue, pop_id);
}

/*********************************************************************
  Function:        divoom_menu_control_load_next
  Description:     加载菜单，会将当前菜单弹出队列
  Input:           menu_id
                   user_data  这个外面要申请，在菜单初始化时再由菜单级释放
  Return:          NONE
**********************************************************************/

void divoom_menu_control_load_next_pop_cur(int menu_id, void *user_data, int model_id, int event_id)
{
    int try_time = 0;
    Pdivoom_menu_control_event_info event_info;

    divoom_menu_control_item_info *praw_menu_info;

    while(pdivoom_menu_control_handle == NULL)
    {
        divoom_system_delay_ms(100);
        try_time ++;
        if(0 == (try_time % 20))
        {
            divoom_print("%s:%d wait %d\n", __func__, __LINE__, try_time);
        }
    }
    menu_id = divoom_menu_control_get_menu_id(menu_id);
    if (divoom_menu_control_poweroff_blocks_menu(menu_id))
        return;
    
    praw_menu_info = divoom_menu_control_get_menu_info(menu_id);
    if(praw_menu_info && praw_menu_info->id == DIVOOM_DISP_CLOCK_MODE)
    {
        //这个模式统一认为是APP触发的，将认为清除之前所有模式
        divoom_menu_control_clearall_load(menu_id, user_data, model_id, event_id);
        return;
    }
    event_info = divoom_system_malloc(sizeof(divoom_menu_control_event_info));
    if(event_info)
    {
        event_info->type = DIVOOM_MENU_CONTROL_EVENT_NEXT_POP_CUR;
        event_info->menu_id = menu_id;
        event_info->model_id = model_id;
        event_info->event_id = event_id;
        event_info->user_data = user_data;
        divoom_queue_push(pdivoom_menu_control_handle->event_queue, event_info);
    }
    divoom_menu_control_add_all_remove_menu();
}
int divoom_menu_control_clear_model_all_menu_compare(void *item1, void *item2)
{
    Pdivoom_menu_control_event_info pitem1_info = (Pdivoom_menu_control_event_info)item1;
    Pdivoom_menu_control_event_info pitem2_info = (Pdivoom_menu_control_event_info)item2;

    if(pitem1_info->model_id == pitem2_info->model_id && pitem1_info->event_id == pitem2_info->model_id)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/*********************************************************************
  Function:        divoom_menu_control_clear_model_all_menu
  Description:     将取消模块和事件关联的所有菜单队列
  Input:           model_id
                   event_id
  Return:          NONE
**********************************************************************/

void divoom_menu_control_clear_model_all_menu(int model_id, int event_id)
{
    Pdivoom_menu_control_event_info event_info;
    divoom_menu_control_event_info  del_info;

    del_info.model_id = model_id;
    del_info.event_id = event_id;
    do{
        event_info = divoom_queue_pop_cmp(pdivoom_menu_control_handle->event_queue, &del_info, divoom_menu_control_clear_model_all_menu_compare);
        divoom_system_free(event_info);
    }while(event_info);

    event_info = divoom_system_malloc(sizeof(divoom_menu_control_event_info));
    if(event_info)
    {
        
        event_info->type = DIVOOM_MENU_CONTROL_EVENT_POP_MODEL_ALL;
        event_info->model_id = model_id;
        event_info->event_id = event_id;
        divoom_queue_push(pdivoom_menu_control_handle->event_queue, event_info);
        divoom_system_put_sem(pdivoom_menu_control_handle->wait_sem);
    }
}

/*********************************************************************
  Function:        divoom_menu_control_clearall_load
  Description:     将取消所有菜单队列，只加这个菜单,这个接口在APP发图或选择表盘的时候，将使用，所有菜单状态规0
  Input:           menu_id
  Return:          NONE
**********************************************************************/

void divoom_menu_control_clearall_load(int menu_id, void *user_data, int model_id, int event_id)
{
    Pdivoom_menu_control_event_info event_info;
    
    menu_id = divoom_menu_control_get_menu_id(menu_id);
    if (divoom_menu_control_poweroff_blocks_menu(menu_id))
        return;
    divoom_print("menu: clearall_load queue menu=%d model=%d event=%d finish_start=%d\n",
                 menu_id, model_id, event_id, divoom_main_get_finish_start_up());

    event_info = divoom_system_malloc(sizeof(divoom_menu_control_event_info));
    if(event_info)
    {
        event_info->type = DIVOOM_MENU_CONTROL_EVENT_CLEAR_ALL_LOAD;
        event_info->menu_id = menu_id;
        event_info->model_id = model_id;
        event_info->event_id = event_id;
        event_info->user_data = user_data;
        divoom_queue_push(pdivoom_menu_control_handle->event_queue, event_info);
    }
    else
    {
        divoom_print("menu: clearall_load malloc fail menu=%d\n", menu_id);
    }
    divoom_menu_control_add_all_remove_menu();
}

/*********************************************************************
  Function:        divoom_menu_control_load_next_with_disp
  Description:     带显示ID的加载MENU方式
  Input:           menu_id
  Return:          NONE
**********************************************************************/

void divoom_menu_control_load_next_with_disp(int menu_id, void *user_data, int model_id, int event_id, int flag)
{
    Pdivoom_menu_control_event_info event_info;
    
    menu_id = divoom_menu_control_get_menu_id(menu_id);
    if (divoom_menu_control_poweroff_blocks_menu(menu_id))
        return;

    event_info = divoom_system_malloc(sizeof(divoom_menu_control_event_info));
    if(event_info)
    {
        if(flag)
        {
            event_info->type = DIVOOM_MENU_CONTROL_EVENT_NEXT;
        }
        else
        {
            event_info->type = DIVOOM_MENU_CONTROL_EVENT_NEXT_AND_PASS;
        }
        event_info->menu_id = menu_id;
        event_info->model_id = model_id;
        event_info->event_id = event_id;
        event_info->user_data = user_data;
        divoom_queue_push(pdivoom_menu_control_handle->event_queue, event_info);
    }
    divoom_menu_control_add_all_remove_menu();
}

int divoom_menu_control_change_slide_effect_compare(void *item1, void *item2)
{
    int type = *(int *)item1;
    Pdivoom_menu_control_event_info event_info = (Pdivoom_menu_control_event_info)item2;

    return type == event_info->type?0:1;
}

/*********************************************************************
  Function:        divoom_menu_control_change_slide_effect
  Description:     切换幻灯片效果
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_menu_control_change_slide_effect(void)
{
    int type = DIVOOM_MENU_CONTROL_EVENT_PHOTO_CHANGE_SLIDE;
    Pdivoom_menu_control_event_info event_info = NULL;
    
    if(pdivoom_menu_control_handle == NULL)
    {
        return;
    }
    event_info = divoom_queue_pop_cmp(pdivoom_menu_control_handle->event_queue, &type, divoom_menu_control_change_slide_effect_compare);
    if(event_info == NULL)
    {
        event_info = divoom_system_malloc(sizeof(divoom_menu_control_event_info));
    }
    if(event_info)
    {
        event_info->type = DIVOOM_MENU_CONTROL_EVENT_PHOTO_CHANGE_SLIDE;
        divoom_queue_push(pdivoom_menu_control_handle->event_queue, event_info);
        divoom_system_put_sem(pdivoom_menu_control_handle->wait_sem);
    }
}

/*********************************************************************
  Function:        divoom_menu_control_return_pre
  Description:     退出当前菜单，返回上一级
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_menu_control_redisp_current(void)
{
    Pdivoom_menu_control_event_info event_info;

    if (pdivoom_menu_control_handle == NULL)
    {
        return;
    }

    event_info = divoom_system_malloc(sizeof(divoom_menu_control_event_info));
    if (event_info)
    {
        event_info->type = DIVOOM_MENU_CONTROL_EVENT_REDISPLAY_CURRENT;
        divoom_queue_push(pdivoom_menu_control_handle->event_queue, event_info);
    }
}

void divoom_menu_control_return_pre(void)
{
    Pdivoom_menu_control_event_info event_info;
    
    if(pdivoom_menu_control_handle == NULL)
    {
        return;
    }
    if(divoom_queue_items(pdivoom_menu_control_handle->menu_queue) == 0)
    {
        //如果之有一级菜单返回的话，将直接返回主菜单
        divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP ,DIVOOM_LIGHT_APP_EVENT_START_UP_DISP_DIAL);
        return;
    }
    
    event_info = divoom_system_malloc(sizeof(divoom_menu_control_event_info));
    if(event_info)
    {
        event_info->type = DIVOOM_MENU_CONTROL_EVENT_PRE;
        event_info->menu_id = pdivoom_menu_control_handle->pcur_menu
                                  ? pdivoom_menu_control_handle->pcur_menu->menu_id : -1;
        event_info->menu_generation = s_menu_control_generation;
        divoom_queue_push(pdivoom_menu_control_handle->event_queue, event_info);
        divoom_menu_control_add_all_remove_menu();
    }
}


/*********************************************************************
  Function:        divoom_menu_control_return_main
  Description:     返回第一级主菜单
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_menu_control_return_main(void)
{
    Pdivoom_menu_control_event_info event_info;

    if(pdivoom_menu_control_handle == NULL)
    {
        return;
    }
    event_info = divoom_system_malloc(sizeof(divoom_menu_control_event_info));
    if(event_info)
    {
        event_info->type = DIVOOM_MENU_CONTROL_EVENT_RETURN_MAIN;
        divoom_queue_push(pdivoom_menu_control_handle->event_queue, event_info);
        divoom_menu_control_add_all_remove_menu();
    }
}



void divoom_menu_control_lvgl_ctrl_timer_handle(struct _lv_timer_t * para)
{
    divoom_menu_control_lvgl_ctrl_para *para_info = lv_timer_get_user_data(para);


    if(para_info && para_info->run_flag == 0)
    {
        para_info->pcallback(para_info->user_data);
        divoom_system_put_sem(para_info->sem_data);
        para_info->run_flag = 1;
    }
}

/*********************************************************************
  Function:        divoom_menu_control_lvgl_ctrl
  Description:     这个函数主要是用于调用LVGL的一些内部函数，避免多任务冲突,
                   只能在非LVGL的菜单中使用，否则将会出现死锁情况
                   非常重要


  Input:           pcallback
                   user_data
  Return:          NONE
**********************************************************************/

void divoom_menu_control_lvgl_ctrl(DIVOOM_MENU_CONTROL_LVGL_CTRL_CALLBACK pcallback, void *user_data)
{
    lv_timer_t *time_handle;
    Pdivoom_menu_control_lvgl_ctrl_para para_info = divoom_system_malloc(sizeof(divoom_menu_control_lvgl_ctrl_para));

	if (para_info == NULL)
		return;
    para_info->run_flag  = 0;
    para_info->sem_data = divoom_system_create_sem(1, 0);
    para_info->user_data = user_data;
    para_info->pcallback = pcallback;

    time_handle =  lv_timer_create(divoom_menu_control_lvgl_ctrl_timer_handle, 2000, para_info);
	if (time_handle == NULL)
	{
		divoom_system_destroy_sem(para_info->sem_data);
		divoom_system_free(para_info);
		return;
	}
    lv_timer_set_repeat_count(time_handle, 1);
	lv_timer_set_period(time_handle, 1);
    divoom_system_get_sem(para_info->sem_data);
    divoom_system_destroy_sem(para_info->sem_data);
	divoom_system_free(para_info);
}

/*********************************************************************
  Function:        divoom_menu_control_init_lable_style
  Description:     创建LABLE的样式
  Input:           label_style
  Return:          NONE
**********************************************************************/

void divoom_menu_control_init_lable_style(lv_style_t *label_style)
{
    lv_style_init(label_style);
    
    lv_style_set_pad_all(label_style, 0);
    lv_style_set_pad_gap(label_style, 0);
    lv_style_set_border_width(label_style, 0);
    lv_style_set_border_side(label_style, 0);
    lv_style_set_outline_width(label_style, 0);
    lv_style_set_outline_pad(label_style, 0);
    lv_style_set_radius(label_style, 0);  
    lv_style_set_border_side(label_style, LV_BORDER_SIDE_NONE);  
    
}


/*********************************************************************
  Function:        divoom_menu_control_exit
  Description:     退出菜单，清除所有资源
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_menu_control_exit(void)
{
    if(pdivoom_menu_control_handle == NULL)
    {
        return;
    }

    pdivoom_menu_control_handle->pcur_menu->pclean(pdivoom_menu_control_handle->pcur_menu->plvgl_obj);
    divoom_lvgl_font_manage_clear_clock_font(pdivoom_menu_control_handle->pcur_menu->menu_id, NULL);
    divoom_system_free(pdivoom_menu_control_handle->pcur_menu);
    while(divoom_queue_items(pdivoom_menu_control_handle->menu_queue))
    {
        pdivoom_menu_control_handle->pcur_menu = divoom_queue_pop(pdivoom_menu_control_handle->menu_queue);
        pdivoom_menu_control_handle->pcur_menu->pclean(pdivoom_menu_control_handle->pcur_menu->plvgl_obj);
        divoom_lvgl_font_manage_clear_clock_font(pdivoom_menu_control_handle->pcur_menu->menu_id, NULL);
        divoom_system_free(pdivoom_menu_control_handle->pcur_menu);
    }
    divoom_queue_destroy(pdivoom_menu_control_handle->menu_queue, NULL);
    divoom_system_free(pdivoom_menu_control_handle);
    pdivoom_menu_control_handle = NULL;

    //这下面对LVGL进行卸载，释放相关资源

}

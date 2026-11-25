#ifndef _divoom_astrootoo_settings_h_
#define _divoom_astrootoo_settings_h_

typedef enum
{
    DIVOOM_ASTROTOO_SETTINGS_PAGE_ROOT = 0,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_DISPLAY,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_BRIGHTNESS,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_MEDIA,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_VOLUME,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_CONNECTION,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_PREFERENCES,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_SYSTEM,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_NIGHTMODE,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_SLIDESHOW,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_SLIDESHOW_PHOTO_ORDER,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_LANGUAGE,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_BLUETOOTH,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_ADD,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_WIFI_PASSWORD,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_EQUALIZER,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_IDLE_SHUTDOWN,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_SCHEDULED_POWER,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_TIME,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_WIND_UNIT,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_TEMP_UNIT,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_FORMAT,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_TIME_TIME,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_DATE_TIME_DATE,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_BT_PASSWORD,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_MCP,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_KEY_LIGHT_EFFECTS,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_ABOUT,
    DIVOOM_ASTROTOO_SETTINGS_PAGE_CONNECTION_CODE,
} divoom_astrootoo_settings_page_t;

void *divoom_astrootoo_settings_init(void *user_data, int menu_id);
void divoom_astrootoo_settings_disp(void *handle, void *active_obj);
void divoom_astrootoo_settings_clean(void *handle);
void divoom_astrootoo_settings_pause(void *handle);

/* 系统亮度被机械键/OSD 等外部修改后，刷新设置根页/亮度子页滑条（不同步提示音音量） */
void divoom_astrootoo_settings_notify_sys_brightness(void);

/* 白盒压测：系统设置根页子项循环 tour（仅模拟器调用） */
void divoom_astrootoo_settings_whitebox_tour_reset(void);
void divoom_astrootoo_settings_whitebox_tour_configure(unsigned timeout_ms, unsigned item_timeout_ms);
int divoom_astrootoo_settings_whitebox_tour_tick(unsigned now_ms);
const char *divoom_astrootoo_settings_whitebox_tour_fail_detail(void);
int divoom_astrootoo_settings_whitebox_tour_get_index(void);
int divoom_astrootoo_settings_whitebox_tour_is_done(void);

#endif

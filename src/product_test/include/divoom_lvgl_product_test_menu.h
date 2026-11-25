#ifndef _divoom_lvgl_product_test_menu_h_
#define _divoom_lvgl_product_test_menu_h_

enum
{
	DIVOOM_PRODUCT_TEST_MENU_DISP_VERSION_OPTION,
    DIVOOM_PRODUCT_TEST_MENU_USR_CHECK_OPTION,
    DIVOOM_PRODUCT_TEST_MENU_ALL_WHITE_OPTION,
    DIVOOM_PRODUCT_TEST_MENU_RED_LIGHT_OPTON,
    DIVOOM_PRODUCT_TEST_MENU_GREEN_LIGHT_OPTION,
    DIVOOM_PRODUCT_TEST_MENU_BLUE_LIGHT_OPTION,
    DIVOOM_PRODUCT_TEST_MENU_MIC_OPTION,
    DIVOOM_PRODUCT_TEST_MENU_KEY_OPTION,
    DIVOOM_PRODUCT_TEST_MENU_TF_OPTION,
    DIVOOM_PRODUCT_TEST_MENU_WIFI_OPTION,
    DIVOOM_PRODUCT_TEST_MENU_END_OPTION,
	/* 现不测：放 END 后则 cur_mode++ 走不到；要用时挪回顺序位置 */
	DIVOOM_PRODUCT_TEST_MENU_BACK_LIGHT_OPTION, /* 原在蓝灯与喇叭之间 */
	DIVOOM_PRODUCT_TEST_MENU_LINEIN_OPTION,     /* 原在按键与 USB 之间 */
	DIVOOM_PRODUCT_TEST_MENU_BUZZER_OPTION,     /* 原在蓝灯与 MIC 之间 */
	DIVOOM_PRODUCT_TEST_MENU_UAC_OPTION,        /* 原在按键与 TF 之间 */
};

void *divoom_product_test_screen_create(void);
void divoom_product_test_screen_show(void *handle);
void divoom_product_test_screen_destroy(void *handle);
int divoom_product_test_screen_on_key(void *handle, uint8_t key, uint8_t press_type);

#endif

#ifndef _divoom_disp_test_h_
#define _divoom_disp_test_h_

enum
{
    DIVOOM_DISP_TEST_DISP_VERSION,
    DIVOOM_DISP_TEST_ALL_WHITE_ITEM,
    DIVOOM_DISP_TEST_RED_LIGHT_ITEM,
    DIVOOM_DISP_TEST_GREEN_LIGHT_ITEM,
    DIVOOM_DISP_TEST_BLUE_LIGHT_ITEM,
    DIVOOM_DISP_TEST_RUN_LIGHT_ITEM,
    DIVOOM_DISP_TEST_TEMP_BAT_ITEM,
    DIVOOM_DISP_TEST_KEY_ITEM,
    DIVOOM_DISP_TEST_MIC_ITEM,
    DIVOOM_DISP_TEST_BUZZER_ITEM,
    DIVOOM_DISP_TEST_FM_ITEM,
    DIVOOM_DISP_TEST_LINEIN_ITEM,
    DIVOOM_DISP_TEST_CHARGE_ITEM,
    DIVOOM_DISP_TEST_RTC_ITEM,
    DIVOOM_DISP_TEST_BLUE_ITEM,
    DIVOOM_DISP_TEST_WIFI_ITEM,
    DIVOOM_DISP_TEST_UPDATE_ITEM,
    DIVOOM_DISP_TEST_TEMP,
	DIVOOM_DISP_TEST_SD_ITEM,
	DIVOOM_DISP_TEST_SHOWNUM,
	DIVOOM_DISP_TEST_WIFI_RSSI_ITEM,
};
void divoom_disp_test_init(Pdivoom_disp_ctrl_product_test_info light_data);

void divoom_disp_test_close(void);
void divoom_disp_test_update(Pdivoom_disp_ctrl_product_test_info light_data);
int system_get_temp_info(void);

#endif



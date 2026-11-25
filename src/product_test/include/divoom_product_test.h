#ifndef _divoom_product_test_h_
#define _divoom_product_test_h_

#include <stdint.h>

enum
{
    DIVOOM_PRODUCT_TEST_START_EVENT,
};

enum
{
    DIVOOM_PRODUCT_TEST_MODE_START_UPDATE_CUSTOMIZE,
    DIVOOM_PRODUCT_TEST_MODE_START_UPDATE_FIRMWARE,

    DIVOOM_PRODUCT_TEST_MODE_UPDATE_RESULT,
    DIVOOM_PRODUCT_TEST_MODE_FIRMWARE_UPDATE_RESULT,
    DIVOOM_PRODUCT_TEST_MODE_FIRMWARE_UPDATE_PERCENT,

    DIVOOM_PRODUCT_TEST_MODE_SD_STATUS,
 	DIVOOM_PRODUCT_TEST_MODE_MIC_STATUS,
};

typedef struct __divoom_update_poweron_cfg_info
{
	uint8 user_lock;
    char  power_on_text[256];
}divoom_update_poweron_cfg_info,*Pdivoom_update_poweron_cfg_info;

typedef struct __divoom_update_json_listguide_info
{
	int CustomizeFlag;
	int prog_flag;
	int jl_prog_flag;
    char customize_json[256];
    char prog_json[256];
    char jl_prog_json[256];
}divoom_update_json_listguide_info,*Pdivoom_update_json_listguide_info;

#define DIVOOM_UPDATE_JSON_MAX_SPIFFS_FILE_NUM	200
typedef struct __divoom_update_json_spiffs_file_info
{
    char local_file_name[128];
    char url[256];
}divoom_update_json_spiffs_file_info,*Pdivoom_update_json_spiffs_file_info;

typedef struct __divoom_update_json_customize_info
{
	uint8 user_lock;
    char  power_on_text[256];
	int spiffs_file_num;
	divoom_update_json_spiffs_file_info spiffs_file_list[DIVOOM_UPDATE_JSON_MAX_SPIFFS_FILE_NUM];
}divoom_update_json_customize_info,*Pdivoom_update_json_customize_info;

typedef struct __divoom_update_json_firmware_info
{
	int version;
    char sha1[64];
    char fileid[256];
}divoom_update_json_firmware_info,*Pdivoom_update_firmware_info;

#define DIVOOM_PT_BRIGHTNESS_DEFAULT  100
#define DIVOOM_PT_VOLUME_DEFAULT      100

#define DIVOOM_PRODUCT_TEST_INSERT_UAC     0
#define DIVOOM_PRODUCT_TEST_INSERT_LINEIN  1
#define DIVOOM_PRODUCT_TEST_INSERT_TF      2
#define DIVOOM_PRODUCT_TEST_INSERT_USB     3

enum {
	DIVOOM_PRODUCT_TEST_BOOT_NORMAL = 0,
	DIVOOM_PRODUCT_TEST_BOOT_UPGRADE_AGING,
	DIVOOM_PRODUCT_TEST_BOOT_FORMAT_RECOVERY,
};

/* 产测工作线程消息：LVGL/UART 只 post，阻塞活在 divoom_product_test_task */
enum {
	DIVOOM_PT_MSG_CHECK_LINK = 1, /* 握手；杰理随后主动上报插拔，不查询 */
	DIVOOM_PT_MSG_BOOT_DONE,      /* 通知杰理 AIC host audio ready */
	DIVOOM_PT_MSG_ENTER_EFEX,     /* 0x67 进 FEL/efex */
	DIVOOM_PT_MSG_POWER_OFF,      /* 杰理 0x19 关机（不播提示音） */
	DIVOOM_PT_MSG_WIFI_UP,        /* STA 已连上工厂 AP */
	DIVOOM_PT_MSG_START_OTA,      /* GOT_IP：listguide → Customize → Prog → JL Prog */
};

int divoom_product_test_task_start(void);
void divoom_product_test_post(uint32 msg, uint32 param);

void divoom_product_test_set_active(int active);
int divoom_product_test_is_active(void);

uint8 divoom_product_test_get_brightness(void);
void divoom_product_test_set_brightness(uint8 value);
uint8 divoom_product_test_get_volume(void);
void divoom_product_test_set_volume(uint8 value);

uint8 divoom_product_test_get_insert_status(uint8 type);

void divoom_product_test_on_btcore_cmd(uint8_t cmd, const uint8_t *cmd_data, uint16_t cmd_len);

int divoom_product_test_boot(uint8 boot_mode);
int divoom_product_test_is_upgrade_aging(void);
int divoom_product_test_is_format_recovery(void);
int divoom_product_test_enter_upgrade_aging(void);
/* Reserve /usr against updates before starting the read-only test. */
int divoom_product_test_start_usr_check(void);
void divoom_product_test_stop_aging_audio(void);
void divoom_product_test_select_next(void);
void divoom_product_test_select_other(void);
void divoom_product_test_try_boot_done(void);
void divoom_product_test_on_leave_version(void);
void divoom_product_test_ensure_buzzer(void);
void divoom_product_test_ensure_mic(void);
int divoom_product_test_ensure_wifi(void);
void divoom_product_test_ensure_insert(void);
void divoom_product_test_ensure_rgb(void);

int divoom_product_test_init(void);
uint8 divoom_product_test_wifi_model_init(void);
int divoom_product_test_wifi_init(void);
/* The test UI enters first; format workers wait for deferred Wi-Fi startup. */
int divoom_product_test_wifi_boot_pending(void);
void divoom_product_test_wifi_cleanup(void);
void divoom_product_test_enter_efex_boot(void);

/* 版本页轮询：0 连接中 / 1 下载中 / 2 有结果 / 3 无更新 / 4 刷写中 / 5 listguide 失败 / 6 解压中 / 7 JL 升级 */
#define DIVOOM_PT_DL_CONNECTING       0
#define DIVOOM_PT_DL_DOWNLOADING      1
#define DIVOOM_PT_DL_DONE             2
#define DIVOOM_PT_DL_NONE             3
#define DIVOOM_PT_DL_FLASHING         4
#define DIVOOM_PT_DL_LISTGUIDE_FAIL   5
#define DIVOOM_PT_DL_UNZIPPING        6
#define DIVOOM_PT_DL_JL_UPDATING      7

int divoom_product_test_get_download_file_flag(void);
int divoom_product_test_get_wifi_up(void);
void divoom_product_test_get_download_progress(int *cur, int *total);
int divoom_product_test_is_customize_updating(void);
int divoom_product_test_get_firmware_percent(void);
int divoom_product_test_get_firmware_result(void);
int divoom_product_test_get_customize_fail(void);
int divoom_product_test_get_firmware_checked(void);

void divoom_product_test_on_ota_percent(int percent);
void divoom_product_test_on_ota_unzip_percent(int percent);
void divoom_product_test_on_ota_result(int result);
/* Next 流程进入最后 WIFI 测试页后，才对刷写到 staging 的 Prog 放行。 */
void divoom_product_test_on_wifi_page(void);

uint8 divoom_product_test_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type);
uint8 divoom_product_test_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type);
uint8 divoom_product_test_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type);
uint8 divoom_product_test_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type);

/* Reserve file/network operations for the format page; reject active OTA. */
int divoom_product_test_begin_format(void);
void divoom_product_test_end_format(void);
void divoom_product_test_leave_format_recovery(void);

#endif

#ifndef _divoom_ota_server_
#define _divoom_ota_server_

typedef struct _divoom_ota_info
{
	uint8 sha1_sum[64];
	int8 file_addr[128];
}divoom_ota_info, *Pdivoom_ota_info;
enum
{
    DIVOOM_OTA_SERVER_APP_UPGRADE,
    DIVOOM_OTA_SERVER_SD_UPGRADE,
    DIVOOM_OTA_SERVER_JL_UPGRADE,
    DIVOOM_OTA_SERVER_SYSTEM_FILE_UPDATE,

};
    
enum
{
    DIVOOM_OTA_SERVER_UPGRADE_FILE_DOWNLOADING, //升级文件下载中
    DIVOOM_OTA_SERVER_UPGRADE_FILE_OK,  //程序文件已经是最新的
    DIVOOM_OTA_SERVER_UPGRADE_FILE_ERR, //升级文件下载失败
    DIVOOM_OTA_SERVER_UPGRADE_FILE_READY,  //升级文件已经准备好
    DIVOOM_OTA_SERVER_UPGRADE_FILE_ERROR_URL,  //获取URL失败
};


//内部事件宏，一般是由外部触发的
enum
{
	DIVOOM_OTA_SERVER_SRV_UPDATE, 
	DIVOOM_OTA_SERVER_LOCAL_UPDATE, 
	DIVOOM_OTA_SERVER_UPDATE, 
	DIVOOM_OTA_SERVER_UPDATE_TEST,
	DIVOOM_OTA_SERVER_SRV_UPDATE_FAIL,
	DIVOOM_OTA_SERVER_JL_SRV_UPDATE,
	DIVOOM_OTA_SERVER_TF_STAGING,
	DIVOOM_OTA_SERVER_TF_SESSION_ENTER,
	DIVOOM_OTA_SERVER_TF_SESSION_LEAVE,
};

void divoom_ota_server_init(void);
/* Existing resource worker owns this schedule; call outside the updater lock. */
int divoom_ota_server_begin_system_file_update(void);
void divoom_ota_server_end_system_file_update(void);
/* 产测/局域网：直接起 ota_local，不走 sys_model */
void divoom_ota_server_start_local_update(const char *url);
void divoom_ota_server_active_update(void);
uint8 divoom_ota_server_get_update_status(void);
void divoom_ota_server_set_update_status(uint8 status);
uint8 divoom_ota_server_get_show_percent(void);
void divoom_ota_server_set_staged_firmware(const char *full_path);
const char *divoom_ota_server_get_staged_firmware(void);

uint8 divoom_ota_server_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type);
uint8 divoom_ota_server_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type);
uint8 divoom_ota_server_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type);
uint8 divoom_ota_server_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type);

#endif


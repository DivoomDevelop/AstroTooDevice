#include "divoom.h"
#include "divoom_menu_control.h"
#include "divoom_product_test.h"
#include <string.h>
#if DIVOOM_BUILD_RTOS_DEVICE
#include "divoom_ota_rtos.h"
#include "divoom_port_fs.h"
#endif
#ifdef DIVOOM_TIMES_VIBE
#include "divoom_ota_jl.h"
#endif
#include "divoom_btcore_file_rx.h"
#include "divoom_lvgl_device_update.h"

#define DIVOOM_OTA_UPDATE_HAVE_FLASH

#define DIVOOM_OTA_SHA1_SIZE 64

#define DIVOOM_SD_UPDATE_CFG_FILE 	DIVOOM_LEGACY_SD_ROOT "/divoom_update_info_firmware.json"
#define DIVOOM_UPDATE_FILE_CHECK	"/spiffs/sd_file_checksum"
#define DIVOOM_UPDATE_FILE_CHECKSUM	"checksum"
#define DIVOOM_READ_LEN 1024

typedef struct _divoom_ota_server_ctrl
{    
    uint8 update_type;// 1-自动升级，2-app升级
	uint8 update_status; //1表示正在升级 0表示未升级
	uint8 show_percent;
    uint8 sha1_sum[DIVOOM_OTA_SHA1_SIZE];
	char sd_prog_file_name[128];
    int sha_block_len;
    int offset;
    int percent;
    uint32 cur_version;
	uint32 checksum;
	char Intranet_prog_url[256];
}divoom_ota_server_ctrl, *Pdivoom_ota_server_ctrl;
Pdivoom_ota_server_ctrl  pdivoom_ota_server_handle = NULL;
static uint8 s_tf_session_ui;
/* Only the existing resource worker writes this flag; scheduler callbacks never reenter cancel. */
static uint8 s_system_file_schedule;

static void divoom_ota_server_show_system_file_update(void)
{
    divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_ACCEPT_APP_PHOTO_VIEW,
        NULL, DIVOOM_MODEL_OTA, DIVOOM_OTA_SERVER_SYSTEM_FILE_UPDATE);
}

int divoom_ota_server_begin_system_file_update(void)
{
    divoom_app_schedule_item_info schedule = {0};
    if (!divoom_update_system_file_is_resource_updating())
        return 0;
    schedule.model_id = DIVOOM_MODEL_OTA;
    schedule.event_id = DIVOOM_OTA_SERVER_SYSTEM_FILE_UPDATE;
    schedule.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
    /* Never queue behind firmware/shutdown, so preempted resources cannot resume later. */
    schedule.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
    if (!divoom_app_schedule_push(&schedule))
        return 0;
    s_system_file_schedule = 1;
    if (!divoom_update_system_file_is_resource_updating()) {
        divoom_ota_server_end_system_file_update();
        return 0;
    }
    divoom_ota_server_show_system_file_update();
    divoom_print("ota: system file schedule acquired priority=8000\n");
    return 1;
}

void divoom_ota_server_end_system_file_update(void)
{
    divoom_app_schedule_item_info schedule = {0};
    if (!s_system_file_schedule)
        return;
    s_system_file_schedule = 0;
    schedule.model_id = DIVOOM_MODEL_OTA;
    schedule.event_id = DIVOOM_OTA_SERVER_SYSTEM_FILE_UPDATE;
    schedule.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
    /* Resource state is already idle; cancel may synchronously resume the previous UI. */
    divoom_app_schedule_cancel(&schedule);
    divoom_print("ota: system file schedule released\n");
}


static int divoom_ota_server_ensure_handle(void)
{
	if (pdivoom_ota_server_handle != NULL)
	{
		return 1;
	}
	pdivoom_ota_server_handle = divoom_system_malloc(sizeof(divoom_ota_server_ctrl));
	if (pdivoom_ota_server_handle == NULL)
	{
		divoom_print("%s: malloc fail\n", __func__);
		return 0;
	}
	memset(pdivoom_ota_server_handle, 0, sizeof(divoom_ota_server_ctrl));
	return 1;
}

static int divoom_ota_server_tf_apply_busy(void)
{
#if DIVOOM_BUILD_RTOS_DEVICE
	if (divoom_ota_rtos_is_running() || divoom_ota_rtos_flash_has_started())
		return 1;
#endif
	return 0;
}

static void divoom_ota_server_tf_show_ui(void)
{
	divoom_sys_msg_info model_msg = {0};
	divoom_app_schedule_item_info channel_schedule = {0};
	Pdivoom_general_json_get_update_file_info_respone pmenu;
	const char *staging = DIVOOM_BTCORE_JL_FW_UPDATE_PATH;

	if (s_tf_session_ui)
		return;
	if (!divoom_ota_server_ensure_handle())
		return;

	divoom_manage_image_up_netstatus(0);
	model_msg.src_id = DIVOOM_MODEL_OTA;
	model_msg.dest_id = DIVOOM_MODEL_MAIN;
	model_msg.msg_id = DIVOOM_MAIN_EVENT_DELETE_APP_ID;
	divoom_sys_model_crl_msg_push(&model_msg);

	channel_schedule.model_id = DIVOOM_MODEL_OTA;
	channel_schedule.event_id = DIVOOM_OTA_SERVER_APP_UPGRADE;
	channel_schedule.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
	channel_schedule.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
	if (!divoom_app_schedule_push(&channel_schedule)) {
		divoom_print("ota: tf session schedule_push fail\n");
		return;
	}

	divoom_lvgl_device_update_tf_reset();
	pdivoom_ota_server_handle->show_percent = 1;
	pdivoom_ota_server_handle->update_status = 1;
	pdivoom_ota_server_handle->update_type = 2;
	pmenu = divoom_system_malloc(sizeof(*pmenu));
	if (!pmenu) {
		divoom_lvgl_device_update_tf_end();
		divoom_app_schedule_cancel(&channel_schedule);
		return;
	}
	memset(pmenu, 0, sizeof(*pmenu));
	pmenu->type = 2;
	pmenu->version = DIVOOM_SOFTWARE_VERSION;
	strncpy(pmenu->file_id, staging, sizeof(pmenu->file_id) - 1);
	divoom_print("ota: tf session enter menu=%d file=%s\n",
		     DIVOOM_MENU_CONTROL_ID_UPDATE_DEVICE_FIRMWARE, staging);
	divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_UPDATE_DEVICE_FIRMWARE,
					  pmenu, DIVOOM_MODEL_OTA,
					  DIVOOM_OTA_SERVER_APP_UPGRADE);
	s_tf_session_ui = 1;
}

static void divoom_ota_server_tf_hide_ui(void)
{
	divoom_app_schedule_item_info channel_schedule = {0};

	if (!s_tf_session_ui)
		return;
	if (divoom_ota_server_tf_apply_busy()) {
		divoom_print("ota: tf session leave skip, apply busy\n");
		return;
	}

	channel_schedule.model_id = DIVOOM_MODEL_OTA;
	channel_schedule.event_id = DIVOOM_OTA_SERVER_APP_UPGRADE;
	channel_schedule.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
	divoom_app_schedule_cancel(&channel_schedule);
	if (pdivoom_ota_server_handle)
		pdivoom_ota_server_handle->update_status = 0;
	divoom_lvgl_device_update_tf_end();
	s_tf_session_ui = 0;
	/* first_menu_id 开机后一直是 POWER_ON，pop 过去会再播一次开机视频；栈已被 clearall，return_pre 回表盘 */
	divoom_menu_control_return_pre();
	divoom_print("ota: tf session leave\n");
}

void divoom_ota_server_show_pecent(uint8 pecent)
{
	divoom_disp_ctrl_product_test_info light_data;
	light_data.mode = DIVOOM_DISP_TEST_SHOWNUM;
	light_data.status = pecent;
	divoom_disp_ctrl_set_mode(DIVOOM_DISP_TEST_MODE, &light_data);
}

uint8 divoom_ota_server_get_update_status(void)
{
	if (pdivoom_ota_server_handle == NULL)
		return 0;

	return pdivoom_ota_server_handle->update_status;
}

void divoom_ota_server_set_update_status(uint8 status)
{
	if (!divoom_ota_server_ensure_handle())
	{
		return;
	}
	pdivoom_ota_server_handle->update_status = status;
}

uint8 divoom_ota_server_get_show_percent(void)
{
	if (!pdivoom_ota_server_handle)
		return 0;
	return pdivoom_ota_server_handle->show_percent;
}

void divoom_ota_server_set_staged_firmware(const char *full_path)
{
	if (!divoom_ota_server_ensure_handle() || !full_path)
		return;
	strncpy(pdivoom_ota_server_handle->sd_prog_file_name, full_path,
		sizeof(pdivoom_ota_server_handle->sd_prog_file_name) - 1);
	pdivoom_ota_server_handle->sd_prog_file_name[
		sizeof(pdivoom_ota_server_handle->sd_prog_file_name) - 1] = '\0';
}

const char *divoom_ota_server_get_staged_firmware(void)
{
	if (!pdivoom_ota_server_handle || !pdivoom_ota_server_handle->sd_prog_file_name[0])
		return NULL;
	return pdivoom_ota_server_handle->sd_prog_file_name;
}


void divoom_ota_write_data(void *user_data, uint8 *buf, int len, int tol)
{
    int percent;
    percent = (pdivoom_ota_server_handle->offset + len)*100/tol;
    if(percent != pdivoom_ota_server_handle->percent && percent < 100)
    {
	    divoom_print("divoom_ota_write_data: off:%d: %d, percent:%d,%d\n",pdivoom_ota_server_handle->offset, len, percent, pdivoom_ota_server_handle->show_percent);
        pdivoom_ota_server_handle->percent = percent;
		if (divoom_product_test_is_active())
		{
			divoom_product_test_on_ota_percent(percent);
		}
		else if(pdivoom_ota_server_handle->show_percent)
		{
	        divoom_disp_ctrl_set_mode(DIVOOM_DISP_DEVICE_UPDATE_MODE, &percent);
		}
		else
		{
			divoom_sys_msg_info model_msg = {0};
			model_msg.src_id = DIVOOM_MODEL_OTA; 
			model_msg.dest_id = DIVOOM_MODEL_PRODUCT_TEST; 
			model_msg.msg_id = DIVOOM_PRODUCT_TEST_MODE_FIRMWARE_UPDATE_PERCENT;
			model_msg.data = (uint8 *)&percent;
			divoom_sys_model_crl_msg_push(&model_msg);		
		}
    }
    pdivoom_ota_server_handle->offset += len;
}

static uint8 divoom_ota_server_download_firmware(char *file_addr, char *sha128_code)
{
#if DIVOOM_BUILD_RTOS_DEVICE
	return divoom_ota_rtos_download_and_prepare(file_addr, sha128_code,
						    pdivoom_ota_server_handle->show_percent,
						    NULL, NULL);
#else
	(void)file_addr;
	(void)sha128_code;
	return 0;
#endif
}

static uint8 divoom_ota_server_update(char *file_addr, char *sha128_code)
{
#if DIVOOM_BUILD_RTOS_DEVICE
	return divoom_ota_server_download_firmware(file_addr, sha128_code);
#else
    if(divoom_http_client_down_file_no_delay("/userdata/divoom_app_new", file_addr, pdivoom_ota_server_handle->show_percent, NULL, NULL, NULL))
    {
        char  sha1sum_info[128];
        sha1sum_info[0] = 0;
        divoom_system_delay_ms(1000);
        exec_command_system("sync");
        divoom_app_com_get_file_sha1sum("/userdata/divoom_app_new",sha1sum_info);

        if(!sha128_code || !sha128_code[0] || strcmp(sha1sum_info, sha128_code)== 0)
        {
            divoom_print("divoom_finish update!\n");
            exec_command_system("chmod +x /userdata/divoom_app_new");
            exec_command_system("mv -f /userdata/divoom_app_new /usr/bin/divoom_app");
            divoom_app_com_write_file(DIVOOM_APP_SHA256_SUM_FILE_PATH, sha1sum_info, strlen(sha1sum_info));
            exec_command_system("sync");
        }
        else
        {
            divoom_print("it fails compare sha1: %s, %s\n", sha1sum_info, sha128_code);
            return 0;
        }
    }
    return 1;
#endif
}

void divoom_ota_server_active_update(void)
{
#if DIVOOM_BUILD_RTOS_DEVICE
	const char *path = divoom_ota_server_get_staged_firmware();
	int ret;

	if (!path) {
		divoom_print("ota_server: active_update no staged firmware\n");
		if (divoom_product_test_is_active()) {
			int result = DIVOOM_OTA_SERVER_UPGRADE_FILE_ERR;

			divoom_product_test_on_ota_result(result);
		}
		return;
	}
	ret = divoom_ota_rtos_start_file(path);
	if (ret != 0 && divoom_product_test_is_active()) {
		int result = DIVOOM_OTA_SERVER_UPGRADE_FILE_ERR;

		divoom_print("ota_server: active_update start_file fail\n");
		divoom_product_test_on_ota_result(result);
	}
#endif
}

void divoom_ota_server_format_sha1_sum(char *net_sha_sum)
{
    int i, value;

    for(i=0; i<20; i++)
    {
        sscanf(&net_sha_sum[i*2],"%02x",&value);
        pdivoom_ota_server_handle->sha1_sum[i] = value;
    }
    divoom_com_print_buf((uint8 *)pdivoom_ota_server_handle->sha1_sum, 64);
}

void *divoom_ota_server_server_update_task_main(void *param)
{
	if(param)
	{
		int i = 0;
		char prog_file_name[256] = {0};
		strcpy(prog_file_name,param);
	    for(i = 0; i < 2; i++)
	    {
	        if(divoom_ota_server_update(prog_file_name, NULL))
	        {
	            break;
	        }
	    }
        if(i == 2)
        {
            //如果失败，将退出升级
            divoom_sys_msg_info model_msg = {0};
            divoom_app_schedule_item_info channel_schedule = {0};
            
            channel_schedule.model_id= DIVOOM_MODEL_OTA;  
            channel_schedule.event_id = DIVOOM_OTA_SERVER_APP_UPGRADE;  
            channel_schedule.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
        
            channel_schedule.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT; 
            divoom_app_schedule_cancel(&channel_schedule);
            
    		model_msg.src_id = DIVOOM_MODEL_SER_CON;
    		model_msg.dest_id = DIVOOM_MODEL_MAIN;
            model_msg.msg_type = 1;
    		model_msg.msg_id = DIVOOM_MAIN_NOTICE_RESET_APP_MQTT_CON;
            //升级失败，清除升级标记
            pdivoom_ota_server_handle->update_status = 0;
    		divoom_sys_model_crl_msg_push(&model_msg);
        }
	}
    return NULL;
}

static const char *divoom_ota_server_result_name(int result)
{
	switch (result) {
	case DIVOOM_OTA_SERVER_UPGRADE_FILE_OK:
		return "OK";
	case DIVOOM_OTA_SERVER_UPGRADE_FILE_ERR:
		return "ERR";
	case DIVOOM_OTA_SERVER_UPGRADE_FILE_READY:
		return "READY";
	case DIVOOM_OTA_SERVER_UPGRADE_FILE_ERROR_URL:
		return "URL";
	default:
		return "?";
	}
}

static void divoom_ota_server_notify_local_result(int *result)
{
	if (!result)
		return;
	if (divoom_product_test_is_active()) {
		divoom_product_test_on_ota_result(*result);
		return;
	}

	{
		divoom_sys_msg_info model_msg = {0};

		model_msg.data = (uint8 *)result;
		model_msg.src_id = DIVOOM_MODEL_OTA;
		model_msg.dest_id = DIVOOM_MODEL_PRODUCT_TEST;
		model_msg.msg_id = DIVOOM_PRODUCT_TEST_MODE_FIRMWARE_UPDATE_RESULT;
		divoom_sys_model_crl_msg_push(&model_msg);
	}
}

static void divoom_ota_server_notify_worker_start_failed(void)
{
	int result = DIVOOM_OTA_SERVER_UPGRADE_FILE_ERR;

	if (pdivoom_ota_server_handle)
		pdivoom_ota_server_handle->update_status = 0;
	divoom_print("ota_local: create worker failed\n");
	divoom_ota_server_notify_local_result(&result);
}

static int divoom_ota_server_sha1_is_valid(const char *sha1)
{
	int i;

	if (!sha1 || strlen(sha1) != 40)
		return 0;
	for (i = 0; i < 40; i++) {
		char ch = sha1[i];

		if (!((ch >= '0' && ch <= '9')
			|| (ch >= 'a' && ch <= 'f')
			|| (ch >= 'A' && ch <= 'F')))
			return 0;
	}
	return 1;
}

void *divoom_ota_server_local_update_task_main(void *param)
{
    #define DIVOOM_OTA_LOCAL_UPDATE_TRY_TIMES 3
    //第一：先检查文件确认是否要升级；第二再下载文件：第三校验文件正常将退出任务
    int i, result, server_version = 0;
	divoom_cJSON *root = NULL;
    char prog_file_name[128] = {0}, sha1_sum[64] = {0};

    char *file_info = divoom_http_client_get_srv(pdivoom_ota_server_handle->Intranet_prog_url);

    (void)param;
    
    if(file_info == NULL)
    {
        result = DIVOOM_OTA_SERVER_UPGRADE_FILE_ERROR_URL;
        divoom_print("ota_local: local=%d server=%d result=%s(%d)\n",
		     DIVOOM_SOFTWARE_VERSION, server_version,
		     divoom_ota_server_result_name(result), result);
        divoom_ota_server_notify_local_result(&result);
    }
    else
    {
		divoom_cJSON *version_item;
		divoom_cJSON *file_item;
		divoom_cJSON *sha1_item;

        root = divoom_cJSON_Parse(file_info);
        version_item = root ? divoom_cJSON_GetObjectItem(root, "Version") : NULL;
		file_item = root ? divoom_cJSON_GetObjectItem(root, "FileId") : NULL;
		sha1_item = root ? divoom_cJSON_GetObjectItem(root, "Sha1") : NULL;
        if (root == NULL || (root->type & 0xff) != divoom_cJSON_Object
			|| version_item == NULL
			|| (version_item->type & 0xff) != divoom_cJSON_Number
			|| version_item->valueint <= 0
			|| version_item->valuedouble != (double)version_item->valueint) {
            result = DIVOOM_OTA_SERVER_UPGRADE_FILE_ERROR_URL;
			divoom_print("ota_local: invalid response local=%d result=%s(%d)\n",
				 DIVOOM_SOFTWARE_VERSION,
				 divoom_ota_server_result_name(result), result);
            divoom_ota_server_notify_local_result(&result);
			if (root)
				divoom_cJSON_Delete(root);
            divoom_system_free(file_info);
            return NULL;
        }
        server_version = version_item->valueint;
        if(server_version > DIVOOM_SOFTWARE_VERSION)
        {
			if (file_item == NULL
				|| (file_item->type & 0xff) != divoom_cJSON_String
				|| file_item->valuestring == NULL
				|| file_item->valuestring[0] == '\0'
				|| strlen(file_item->valuestring) >= sizeof(prog_file_name)
				|| sha1_item == NULL
				|| (sha1_item->type & 0xff) != divoom_cJSON_String
				|| !divoom_ota_server_sha1_is_valid(sha1_item->valuestring)) {
				result = DIVOOM_OTA_SERVER_UPGRADE_FILE_ERROR_URL;
				divoom_print("ota_local: invalid upgrade fields version=%d\n",
					server_version);
				divoom_ota_server_notify_local_result(&result);
				divoom_cJSON_Delete(root);
				divoom_system_free(file_info);
				return NULL;
			}
            divoom_cJSON_GetObjectStringN(root, "FileId", prog_file_name,
				(int)sizeof(prog_file_name));
            divoom_cJSON_GetObjectStringN(root, "Sha1", sha1_sum,
				(int)sizeof(sha1_sum));
            divoom_print("file: %s, sha1:%s\n", prog_file_name, sha1_sum);
            for(i = 0; i < DIVOOM_OTA_LOCAL_UPDATE_TRY_TIMES; i++)
            {
                if(divoom_ota_server_update(prog_file_name, sha1_sum))
                {
                    break;
                }
            }
            if(i >= DIVOOM_OTA_LOCAL_UPDATE_TRY_TIMES)
            {
                result = DIVOOM_OTA_SERVER_UPGRADE_FILE_ERR;
            }
            else
            {
                result = DIVOOM_OTA_SERVER_UPGRADE_FILE_READY;
            }
            divoom_print("ota_local: local=%d server=%d result=%s(%d)\n",
			 DIVOOM_SOFTWARE_VERSION, server_version,
			 divoom_ota_server_result_name(result), result);
            divoom_ota_server_notify_local_result(&result);
        }
        else
        {
            result = DIVOOM_OTA_SERVER_UPGRADE_FILE_OK;
            divoom_print("ota_local: local=%d server=%d result=%s(%d)\n",
			 DIVOOM_SOFTWARE_VERSION, server_version,
			 divoom_ota_server_result_name(result), result);
            divoom_ota_server_notify_local_result(&result);
        }
        divoom_cJSON_Delete(root);
        divoom_system_free(file_info);
    }
    return NULL;
}

void divoom_ota_server_start_local_update(const char *url)
{
	if (!url || !url[0]) {
		divoom_ota_server_notify_worker_start_failed();
		return;
	}
	if (!divoom_ota_server_ensure_handle()) {
		divoom_ota_server_notify_worker_start_failed();
		return;
	}

	strncpy(pdivoom_ota_server_handle->Intranet_prog_url, url,
		sizeof(pdivoom_ota_server_handle->Intranet_prog_url) - 1);
	pdivoom_ota_server_handle->Intranet_prog_url[
		sizeof(pdivoom_ota_server_handle->Intranet_prog_url) - 1] = '\0';
	pdivoom_ota_server_handle->show_percent = 0;
	if (!divoom_system_create_task(divoom_ota_server_local_update_task_main,
			"ota_local", NULL, DIVOOM_TASK_STACK_OTA_SERVER_LOCAL))
		divoom_ota_server_notify_worker_start_failed();
}


uint8 divoom_ota_server_event(Pdivoom_sys_msg_info msg)
{
	if (msg->msg_id <= DIVOOM_OTA_SERVER_TF_SESSION_LEAVE)
	{
		if (!divoom_ota_server_ensure_handle())
		{
			return 0;
		}
	}
	switch (msg->msg_id)
	{
        case DIVOOM_OTA_SERVER_SRV_UPDATE_FAIL:
        {
                divoom_app_schedule_item_info channel_schedule = {0};
                channel_schedule.model_id= DIVOOM_MODEL_OTA;  
                channel_schedule.event_id = DIVOOM_OTA_SERVER_APP_UPGRADE;  
                channel_schedule.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
            
                pdivoom_ota_server_handle->update_status = 0;
                channel_schedule.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT; 
                divoom_app_schedule_cancel(&channel_schedule);

                channel_schedule.event_id = DIVOOM_OTA_SERVER_JL_UPGRADE;
                divoom_app_schedule_cancel(&channel_schedule);
        }
            break;
            
        case DIVOOM_OTA_SERVER_SRV_UPDATE:
        {
            divoom_sys_msg_info model_msg = {0};
            Pdivoom_general_json_get_update_file_info_respone pupdate_file = (Pdivoom_general_json_get_update_file_info_respone)msg->data;            
            divoom_app_schedule_item_info channel_schedule = {0};
            

            if(divoom_main_get_boot_ab_recovery_needed() ||
               pupdate_file->version > DIVOOM_SOFTWARE_VERSION)
            {
                divoom_manage_image_up_netstatus(0);    //关闭下载
        		model_msg.src_id = DIVOOM_MODEL_SER_CON;
        		model_msg.dest_id = DIVOOM_MODEL_MAIN;
        		model_msg.msg_id = DIVOOM_MAIN_EVENT_DELETE_APP_ID;
        		divoom_sys_model_crl_msg_push(&model_msg);
                
                channel_schedule.model_id= DIVOOM_MODEL_OTA;  
                channel_schedule.event_id = DIVOOM_OTA_SERVER_APP_UPGRADE;  
                channel_schedule.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
            
                channel_schedule.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT; 
                if(divoom_app_schedule_push(&channel_schedule))
                {
    				pdivoom_ota_server_handle->show_percent = 1;
                    pdivoom_ota_server_handle->update_type = pupdate_file->type;
                    pdivoom_ota_server_handle->cur_version = pupdate_file->version;
                    if(pdivoom_ota_server_handle->update_type == 2)
                    {
                        //divoom_light_app_open_screen();
                    }
                    pdivoom_ota_server_handle->update_status = 1;
                    Pdivoom_general_json_get_update_file_info_respone pmenuifno = divoom_system_malloc(sizeof(divoom_general_json_get_update_file_info_respone));
                    if (pmenuifno == NULL)
                    {
                        divoom_print("ota: clearall_load skip, malloc fail ver=%u\n",
                                     (unsigned)pupdate_file->version);
                    }
                    else
                    {
                        memcpy(pmenuifno, pupdate_file, sizeof(divoom_general_json_get_update_file_info_respone));
                        divoom_print("ota: clearall_load menu=%d ver=%u file=%s finish_start=%d\n",
                                     DIVOOM_MENU_CONTROL_ID_UPDATE_DEVICE_FIRMWARE,
                                     (unsigned)pupdate_file->version,
                                     pupdate_file->file_id,
                                     divoom_main_get_finish_start_up());
                        divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_UPDATE_DEVICE_FIRMWARE, pmenuifno, DIVOOM_MODEL_OTA, DIVOOM_OTA_SERVER_APP_UPGRADE);
                    }
                }
                else
                {
                    divoom_print("ota: schedule_push fail ver=%u\n",
                                 (unsigned)pupdate_file->version);
                }
            }
            else if(pupdate_file->type == 1)
            {
                divoom_system_font_download_all_fond_info();
            }
            else
            {
                divoom_print("ota: skip SRV_UPDATE ver=%u local=%u type=%u\n",
                             (unsigned)pupdate_file->version,
                             (unsigned)DIVOOM_SOFTWARE_VERSION,
                             (unsigned)pupdate_file->type);
            }
        }
            break;

#ifdef DIVOOM_TIMES_VIBE
        case DIVOOM_OTA_SERVER_JL_SRV_UPDATE:
        {
            Pdivoom_general_json_get_update_file_info_respone pjl =
                (Pdivoom_general_json_get_update_file_info_respone)msg->data;

            if (pjl != NULL)
                divoom_ota_jl_start_srv_update(pjl);
            /* msg->data 由 divoom_net_app_do_comm_free() 统一 divoom_free，勿在此释放 */
        }
            break;
#endif

        case DIVOOM_OTA_SERVER_LOCAL_UPDATE:
        {
			//测试模式中的局域网升级，后台执行
			if(msg->data)
			{
				strncpy(pdivoom_ota_server_handle->Intranet_prog_url,
					(char *)msg->data,
					sizeof(pdivoom_ota_server_handle->Intranet_prog_url) - 1);
				pdivoom_ota_server_handle->Intranet_prog_url[
					sizeof(pdivoom_ota_server_handle->Intranet_prog_url) - 1] = '\0';
			}
			pdivoom_ota_server_handle->show_percent = 0;
            //降低优先级
			if (!divoom_system_create_task(divoom_ota_server_local_update_task_main,
					"ota_local", NULL, DIVOOM_TASK_STACK_OTA_SERVER_LOCAL))
				divoom_ota_server_notify_worker_start_failed();
        }
            break;
            
		case DIVOOM_OTA_SERVER_UPDATE:
		{
#if DIVOOM_BUILD_RTOS_DEVICE
			Pdivoom_ota_info pota = (Pdivoom_ota_info)msg->data;

			if (pota && pota->file_addr[0])
				divoom_ota_rtos_start_file((char *)pota->file_addr);
#endif
			break;
		}
		case DIVOOM_OTA_SERVER_TF_SESSION_ENTER:
			divoom_ota_server_tf_show_ui();
			break;
		case DIVOOM_OTA_SERVER_TF_SESSION_LEAVE:
			divoom_ota_server_tf_hide_ui();
			break;
		case DIVOOM_OTA_SERVER_TF_STAGING:
		{
#if DIVOOM_BUILD_RTOS_DEVICE
			const char *staging = msg->data ? (const char *)msg->data : "";
			char fex_path[256];

			if (!staging[0])
				staging = DIVOOM_BTCORE_JL_FW_UPDATE_PATH;
			if (!staging[0])
				break;

			if (!s_tf_session_ui)
				divoom_ota_server_tf_show_ui();

			if (divoom_ota_rtos_build_save_path(NULL, fex_path, sizeof(fex_path)) != 0) {
				divoom_print("ota: tf build fex path fail\n");
				break;
			}
			if (divoom_ota_rtos_start_from_staging(staging, fex_path) != 0)
				divoom_print("ota: tf start_from_staging fail %s\n", staging);
#endif
			break;
		}
		case DIVOOM_OTA_SERVER_UPDATE_TEST: //产线自动升级
			divoom_ota_server_active_update();
			break;
		
		case DIVOOM_APP_SD_STATUS_IN_ID: //sd卡插入自动升级
			break;
	}
	return true;
}

uint8 divoom_ota_server_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type)
{
	(void)user_data;
    if (event_id == DIVOOM_OTA_SERVER_SYSTEM_FILE_UPDATE) {
        if (!divoom_update_system_file_is_resource_updating())
            return 0;
        divoom_ota_server_show_system_file_update();
        return 1;
    }
	divoom_print("ota: schedule_play event=%u resource=%u\n",
		     (unsigned)event_id, (unsigned)resouce_type);
	return 1;
}

uint8 divoom_ota_server_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type)
{
	(void)user_data;
    if (event_id == DIVOOM_OTA_SERVER_SYSTEM_FILE_UPDATE) {
        /* Called under the scheduler lock: only latch cancellation; never push/cancel here. */
        divoom_update_system_file_cancel_current();
        return 0;
    }
	divoom_print("ota: schedule_pause event=%u by %u:%u resource=%u\n",
		     (unsigned)event_id, (unsigned)active_model,
		     (unsigned)active_event, (unsigned)resouce_type);
	return 1;
}

uint8 divoom_ota_server_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type)
{
	(void)user_data;
    if (event_id == DIVOOM_OTA_SERVER_SYSTEM_FILE_UPDATE) {
        /* Preemption already latched cancellation in pause. A normal release must
         * not cancel a new settings round accepted while the worker publishes idle. */
        return 1;
    }
	divoom_print("ota: schedule_destroy event=%u status=%u resource=%u\n",
		     (unsigned)event_id, (unsigned)status, (unsigned)resouce_type);
	return 1;
}

uint8 divoom_ota_server_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type)
{
	(void)key_id;
	(void)key_type;
	(void)resouce_type;
	return 1;
}

static void divoom_ota_server_on_tf_fw_progress(uint8_t percent)
{
	divoom_lvgl_device_update_tf_set_percent((uint8)percent);
}

static void divoom_ota_server_on_tf_fw_saved(const char *path, int ok)
{
	divoom_sys_msg_info model_msg = {0};
	char *copy;
	size_t n;

	if (!ok || !path || !path[0]) {
		divoom_print("ota: tf fw save fail path=%s\n", path ? path : "");
		return;
	}

	n = strlen(path);
	copy = (char *)divoom_system_malloc(n + 1u);
	if (!copy) {
		divoom_print("ota: tf fw saved malloc fail\n");
		return;
	}
	memcpy(copy, path, n + 1u);

	model_msg.msg_type = 1;
	model_msg.src_id = DIVOOM_MODEL_OTA;
	model_msg.dest_id = DIVOOM_MODEL_OTA;
	model_msg.msg_id = DIVOOM_OTA_SERVER_TF_STAGING;
	model_msg.data = (uint8 *)copy;
	model_msg.data_len = (uint16)(n + 1u);
	divoom_sys_model_crl_msg_push(&model_msg);
	divoom_print("ota: tf fw saved %s\n", path);
}

void divoom_ota_server_init(void)
{
#ifdef DIVOOM_TIMES_VIBE
	divoom_ota_jl_init();
#endif
#if DIVOOM_BUILD_RTOS_DEVICE
	divoom_btcore_file_rx_set_fw_saved_callback(divoom_ota_server_on_tf_fw_saved);
	divoom_btcore_file_rx_set_fw_progress_callback(divoom_ota_server_on_tf_fw_progress);
#endif
	divoom_ota_server_ensure_handle();
	divoom_sys_model_crl_register(DIVOOM_MODEL_OTA, divoom_ota_server_event);
}

#include "divoom.h"


typedef struct _divoom_power_on_manage_wifi_info
{
    char ssid[128];
    char passwd[128];
    uint32 appid;
}divoom_power_on_manage_wifi_info, *Pdivoom_power_on_manage_wifi_info;


typedef struct _divoom_power_on_manage_handle
{
	uint8 cur_power_mode; //记录当前开机模式
	uint8 set_power_mode; //设置成指定模式
	divoom_power_on_manage_wifi_info wifi_info;
}divoom_power_on_manage_handle, *Pdivoom_power_on_manage_handle;

static Pdivoom_power_on_manage_handle pdivoom_power_on_mange_hander = NULL;



void divoom_power_on_save_cfg_info(void *data)
{
    int i, j;
    char *json_info;
    divoom_cJSON *root;

	Pdivoom_power_on_manage_handle para = (Pdivoom_power_on_manage_handle)data;
	if(NULL == data)   
	{
		return;
	}

	root = divoom_cJSON_CreateObject();

    if(root == NULL)
    {
        return;
    }
    divoom_cJSON_AddNumberToObject(root , "valid", 1);
    divoom_cJSON_AddNumberToObject(root , "power_on_mode", para->cur_power_mode);
    divoom_cJSON_AddNumberToObject(root , "set_power_mode", para->set_power_mode);    


	divoom_cJSON_AddStringToObject(root , "ssid", para->wifi_info.ssid);  
	divoom_cJSON_AddStringToObject(root , "passwd", para->wifi_info.passwd);  
    divoom_cJSON_AddNumberToObject(root , "appid", para->wifi_info.appid);

    json_info = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);
    divoom_app_com_write_file(DIVOOM_POWER_ON_MANAGE_CFG, (uint8 *)json_info, strlen(json_info));
    divoom_system_free(json_info);    
}


void divoom_power_on_manage_load_default_config(void)
{
	divoom_power_on_manage_handle info = {0};

	info.cur_power_mode = DIVOOM_POWER_ON_WIFI_CONFIG_MODE;
	info.set_power_mode = DIVOOM_POWER_ON_WIFI_MODE;

	divoom_power_on_save_cfg_info(&info);
	if(pdivoom_power_on_mange_hander)
	{
		memcpy(pdivoom_power_on_mange_hander,&info,sizeof(divoom_power_on_manage_handle));
	}
}


int divoom_power_on_manage_load_config(void)
{    
    int valid = 0;
    char *json_info;
    divoom_cJSON *root;
	if(NULL == pdivoom_power_on_mange_hander)
	{
		divoom_debug("pdivoom_power_on_mange_hander null!");
		return 0;
	}

    json_info = divoom_app_com_read_file(DIVOOM_POWER_ON_MANAGE_CFG, NULL);
    if(json_info == NULL)
    {
		//如果是空重新创建一个默认的
		divoom_debug("no file create default!");
		divoom_power_on_manage_load_default_config();
        return 0;
    }
    root = divoom_cJSON_Parse(json_info);
    divoom_system_free(json_info);
    if(root == NULL)
    {
		divoom_debug("no string create default!");
		divoom_power_on_manage_load_default_config();
        return 0;
    }
    pdivoom_power_on_mange_hander->cur_power_mode = divoom_cJSON_GetObjectInt(root, "power_on_mode");
    pdivoom_power_on_mange_hander->set_power_mode = divoom_cJSON_GetObjectInt(root, "set_power_mode");

    pdivoom_power_on_mange_hander->wifi_info.appid = divoom_cJSON_GetObjectInt(root, "appid");
	divoom_cJSON_GetObjectString(root, "ssid",pdivoom_power_on_mange_hander->wifi_info.ssid);
	divoom_cJSON_GetObjectString(root, "passwd",pdivoom_power_on_mange_hander->wifi_info.passwd);


	divoom_debug("power_on_mode:%d,ssid:%s,passwd:%s,appid:%d",
		pdivoom_power_on_mange_hander->cur_power_mode,
		pdivoom_power_on_mange_hander->wifi_info.ssid,
		pdivoom_power_on_mange_hander->wifi_info.passwd,
		pdivoom_power_on_mange_hander->wifi_info.appid);

    divoom_cJSON_Delete(root);
	return 1;
}



void divoom_power_on_manage_save_config(void)
{
	if(NULL == pdivoom_power_on_mange_hander)
	{
		return;
	}
	divoom_power_on_save_cfg_info(pdivoom_power_on_mange_hander);
}


/**
 * *  NAME          divoom_power_on_manage_check_switch_mode
 * *  @brief       	用户因为配网切换蓝牙而重启
 * *  @param[in] 	none 
 * *  @author       wangft                                   
 * *  @return       1蓝牙配网 0非蓝牙配网
 * */
uint8 divoom_power_on_manage_check_switch_mode(void)
{
	return 0;
}

void *divoom_power_on_manage_erase_app_id_task(void *param)
{
    return NULL;
}

/**
 * *  NAME          divoom_power_on_manage_erase_app_id
 * *  @brief       	清空 app id
 * *  @param[in] 	none 
 * *  @author       wangft                                   
 * *  @return       1成功 0失败
 * */
uint8 divoom_power_on_manage_erase_app_id(void)
{
    divoom_system_create_task(divoom_power_on_manage_erase_app_id_task, "pwr_erase_id", NULL, DIVOOM_TASK_STACK_POWER_ERASE_APP_ID);
	return 1;
}

/**
 * *  NAME          divoom_nvs_get_wifi_info
 * *  @brief       	获取 ssid passed
 * *  @param[in] 	ssid  	passwd 
 * *  @author       wangft                                   
 * *  @return       1获取成功 0获取失败
 * */
uint8 divoom_power_on_manage_get_wifi_info(char *ssid, uint8 *ssid_len, char *passwd, uint8 *passwd_len)
{
	return 0;
}

void divoom_power_on_manage_save_wifi_info_task(void *param)
{

}

/**
 * *  NAME          divoom_power_on_manage_save_wifi_info
 * *  @brief       	保存ssid passwd appid
 * *  @param[in] 	ssid  	passwd    appid
 * *  @author       wangft                                   
 * *  @return       1保存成功 0保存失败
 * */

uint8 divoom_power_on_manage_save_wifi_info(char *ssid, char *passwd, uint32 appid)
{
//    Pdivoom_power_on_manage_wifi_info pwifi_info;

//    pwifi_info = divoom_system_malloc(sizeof(divoom_power_on_manage_wifi_info));
//    strcpy(pwifi_info->ssid , ssid);
//    strcpy(pwifi_info->passwd , passwd);
//    pwifi_info->appid = appid;


	return 1;
}

/**
 * *  NAME          divoom_power_on_manage_set_power_mode
 * *  @brief       	设置开机模式
 * *  @param[in] 	power_mode 开机模式	reboot_flag 是否重启
 * *  @author       wangft                                   
 * *  @return       none
 * */
void divoom_power_on_manage_set_power_mode(uint8 power_mode, uint8 reboot_flag)
{
	if (pdivoom_power_on_mange_hander == NULL)
		return ;

	pdivoom_power_on_mange_hander->cur_power_mode  = power_mode;

	divoom_power_on_manage_save_config();

	if (reboot_flag)
	{
		//发送重启命令，目前是配网或者模式切换需要重启
		divoom_sys_msg_info msg = {0};
		msg.msg_id = DIVOOM_MAIN_EVENT_SOFT_RESTART;
		msg.src_id = DIVOOM_MODEL_POWER_ON;
		msg.dest_id = DIVOOM_MODEL_MAIN;
		divoom_sys_model_crl_msg_push(&msg);
	}
}

/**
 * *  NAME          divoom_power_on_manage_get_power_mode
 * *  @brief       	获取开机模式
 * *  @param[in] 	none
 * *  @author       wangft                                   
 * *  @return       none
 * */
uint8 divoom_power_on_manage_get_power_mode(void)
{
	if(divoom_main_get_wifi_config_flag())
	{
		return DIVOOM_POWER_ON_WIFI_MODE;
	}
	else
	{
		return DIVOOM_POWER_ON_WIFI_CONFIG_MODE;
	}
/*
	if (pdivoom_power_on_mange_hander == NULL)
		return DIVOOM_POWER_ON_WIFI_CONFIG_MODE;

	return pdivoom_power_on_mange_hander->cur_power_mode;
*/
}

void *divoom_power_on_manage_main_task(void *param)
{
    if(pdivoom_power_on_mange_hander->set_power_mode == DIVOOM_POWER_ON_FORMAT_SD_MODE)
    {
        divoom_main_format_device(1);
    }
    else if(pdivoom_power_on_mange_hander->set_power_mode == DIVOOM_POWER_ON_DEL_CONFIG_MODE)
    {
        divoom_main_format_device(0);
    }
    else
    {
	    divoom_power_on_manage_set_power_mode(pdivoom_power_on_mange_hander->set_power_mode, 1);
    }
    return NULL;
}

/**
 * *  NAME          divoom_power_on_manage_event
 * *  @brief       	外部事件处理
 * *  @param[in] 	msg 事件
 * *  @author       wangft                                   
 * *  @return       0 
 * */
uint8 divoom_power_on_manage_event(Pdivoom_sys_msg_info msg)
{
	if (pdivoom_power_on_mange_hander == NULL)
		return 0;

	divoom_print("%d", msg->msg_id);
	pdivoom_power_on_mange_hander->set_power_mode = msg->msg_id;
	switch(msg->msg_id)
	{
		case DIVOOM_POWER_ON_WIFI_CONFIG_MODE:
		case DIVOOM_POWER_ON_BLUE_MODE:
		case DIVOOM_POWER_ON_WIFI_MODE:
        case DIVOOM_POWER_ON_FORMAT_SD_MODE:
		case DIVOOM_POWER_ON_DEL_CONFIG_MODE:
			divoom_system_create_task(divoom_power_on_manage_main_task, "power_on", NULL, DIVOOM_TASK_STACK_POWER_ON_MAIN);
			break;
        
	}
	return 1;
}

/**
 * *  NAME          divoom_power_on_manage_init
 * *  @brief       	开机先进行判断当前开机模式
 * *  @param[in] 	none
 * *  @author       wangft                                   
 * *  @return       none
 * */
void divoom_power_on_manage_init(void)
{
	if (pdivoom_power_on_mange_hander)
		return;
	
	pdivoom_power_on_mange_hander = (Pdivoom_power_on_manage_handle)divoom_malloc(sizeof(divoom_power_on_manage_handle), DIVOOM_MODEL_POWER_ON, __LINE__, __FILE__);
	if (pdivoom_power_on_mange_hander == NULL)
	{
		divoom_print("malloc error");
		return ;
	}
	divoom_power_on_manage_load_config();
	divoom_sys_model_crl_register(DIVOOM_MODEL_POWER_ON, divoom_power_on_manage_event);
}


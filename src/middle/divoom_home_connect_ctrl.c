#include "divoom.h"

    

#define  divoom_home_connect_msg_Connected  "BSH.Common.Appliance.Connected"  //CONNECTED
#define  divoom_home_connect_msg_Disconnected  "BSH.Common.Appliance.Disconnected"  //DISCONNECTED
#define  divoom_home_connect_msg_OperationState   "BSH.Common.Status.OperationState"  //STATUS


typedef struct _divoom_home_connect_device_item_
{
    int  connect_type;
    char status[64];
    char disp_data[64];
    char type[64];
    char name[128];
    char id[128];
}divoom_home_connect_device_item, *Pdivoom_home_connect_device_item;

#define DIVOOM_HOME_CONNECT_BUF_SIZE    (2*1024)
typedef struct _divoom_home_connect_ctrl_
{
    uint8 run_flag;
    uint8 notice_flag;      //是否打开通知
    uint8 clock_flag;       //系统时钟显示
    uint8 reolad_status;    //是否重新加载状态
    int   indepence_index;
    int   lcd_id;
    int   clock_id;
    int   active_buf_len;
    char  buf[DIVOOM_HOME_CONNECT_BUF_SIZE];
    char  find_str[512];
    char *word;
    void *lock;
    int request_all_flag;
    int active_id;  //最后通知的设备ID
    int device_num;
    Pdivoom_home_connect_device_item pdevice_list;
    DIVOOM_HOME_CONNECT_EVENT_NOTICE *pcallback;
}divoom_home_connect_ctrl, *Pdivoom_home_connect_ctrl;

Pdivoom_home_connect_ctrl pdivoom_home_connnect_handle = NULL;



#define divoom_home_connect_OperationState_Inactive "BSH.Common.EnumType.OperationState.Inactive"
#define divoom_home_connect_OperationState_Ready "BSH.Common.EnumType.OperationState.Ready"
#define divoom_home_connect_OperationState_DelayedStart "BSH.Common.EnumType.OperationState.DelayedStart"
#define divoom_home_connect_OperationState_Run "BSH.Common.EnumType.OperationState.Run"
#define divoom_home_connect_OperationState_Pause "BSH.Common.EnumType.OperationState.Pause"
#define divoom_home_connect_OperationState_ActionRequired "BSH.Common.EnumType.OperationState.ActionRequired"
#define divoom_home_connect_OperationState_Finished "BSH.Common.EnumType.OperationState.Finished"
#define divoom_home_connect_OperationState_Error "BSH.Common.EnumType.OperationState.Error"
#define divoom_home_connect_OperationState_Aborting "BSH.Common.EnumType.OperationState.Aborting"

void divoom_home_connect_get_device_info(char *device_id, Pdivoom_home_connect_device_item pdevice_info)
{
    //GET /api/homeappliances/{haId}/status/BSH.Common.Status.OperationState
    /*
    Inactive
    Key: BSH.Common.EnumType.OperationState.Inactive
    Description: Home appliance is inactive. It could be switched off or in standby.
    Ready
    Key: BSH.Common.EnumType.OperationState.Ready
    Description: Home appliance is switched on. No program is active.
    Delayed Start
    Key: BSH.Common.EnumType.OperationState.DelayedStart
    Description: A program has been activated but has not started yet.
    Run
    Key: BSH.Common.EnumType.OperationState.Run
    Description: A program is currently active.
    Pause
    Key: BSH.Common.EnumType.OperationState.Pause
    Description: The active program has been paused.
    Action required
    Key: BSH.Common.EnumType.OperationState.ActionRequired
    Description: The active program requires a user interaction.
    Finished
    Key: BSH.Common.EnumType.OperationState.Finished
    Description: The active program has finished or has been aborted successfully.
    Error
    Key: BSH.Common.EnumType.OperationState.Error
    Description: The home appliance is in an error state.
    Aborting
    Key: BSH.Common.EnumType.OperationState.Aborting
    Description: The active program is currently aborting.
    */
    int i;
    char *respone_data;
    Pdivoom_http_client_url_head_info phead_info;
    divoom_cJSON *root = NULL, *obj;

    phead_info = divoom_system_malloc(sizeof(divoom_http_client_url_head_info));
    strcpy(phead_info->key, "Authorization");
    strcpy(phead_info->word, "Bearer ");
    divoom_system_get_lock(pdivoom_home_connnect_handle->lock);
    strcat(phead_info->word, pdivoom_home_connnect_handle->word);
    divoom_system_free_lock(pdivoom_home_connnect_handle->lock);
    if(pdevice_info == NULL)
    {
        //先查找是否是列表中的设备，如果不是，将添加新设备
        for(i = 0; i < pdivoom_home_connnect_handle->device_num; i++)
        {
            if(strcmp(pdivoom_home_connnect_handle->pdevice_list[i].id, device_id) == 0)
            {
                pdevice_info = &pdivoom_home_connnect_handle->pdevice_list[i];
                pdivoom_home_connnect_handle->active_id = i;
                divoom_print("find: %d, %s, %s\n", i, pdevice_info->name, pdevice_info->status);
                break;
            }
        }
        if(pdevice_info == NULL)
        {
            divoom_print("we will request new device: %s\n", device_id);
            sprintf(pdivoom_home_connnect_handle->find_str, "https://api.home-connect.com/api/homeappliances/%s", device_id);
            respone_data = divoom_http_client_url_request(pdivoom_home_connnect_handle->find_str, phead_info, 1, NULL, NULL);
            if(respone_data == NULL)
            {
                divoom_system_free(phead_info);
                divoom_print("err!\n");
                return;
            }
            root = divoom_cJSON_Parse(respone_data);
            divoom_system_free(respone_data);
            if(root == NULL)
            {
                divoom_system_free(phead_info);
                divoom_print("err!\n");
                return;
            }
            obj = divoom_cJSON_GetObjectItem(root, "data");
            if(obj == NULL)
            {
                divoom_system_free(phead_info);
                divoom_cJSON_Delete(root);
                divoom_print("err!\n");
                return ;
            }
            
            pdevice_info = divoom_system_malloc(sizeof(divoom_home_connect_device_item)*(pdivoom_home_connnect_handle->device_num + 1));
            if(pdevice_info == NULL)
            {
                divoom_system_free(phead_info);
                divoom_cJSON_Delete(root);
                divoom_print("err!\n");
                return;
            }
            memcpy(pdevice_info, pdivoom_home_connnect_handle->pdevice_list, sizeof(divoom_home_connect_device_item)*pdivoom_home_connnect_handle->device_num);
            divoom_system_free(pdivoom_home_connnect_handle->pdevice_list);
            pdivoom_home_connnect_handle->pdevice_list = pdevice_info;
            pdevice_info = &pdivoom_home_connnect_handle->pdevice_list[pdivoom_home_connnect_handle->device_num];    
            //新设备要重新获取一下设备
            strcpy(pdevice_info->id, device_id);
            divoom_cJSON_GetObjectString(root, "name", pdevice_info->name);
            divoom_cJSON_GetObjectString(root, "type", pdevice_info->type);
            pdevice_info->connect_type = divoom_cJSON_GetObjectInt(obj, "connected");
            pdivoom_home_connnect_handle->active_id = pdivoom_home_connnect_handle->device_num;
            pdivoom_home_connnect_handle->device_num ++;
            divoom_cJSON_Delete(root);
            //获取设备名称
        }
    }
    sprintf(pdivoom_home_connnect_handle->find_str, "https://api.home-connect.com/api/homeappliances/%s/status/BSH.Common.Status.OperationState", device_id);
    respone_data = divoom_http_client_url_request(pdivoom_home_connnect_handle->find_str, phead_info, 1, NULL, NULL);
    if(respone_data == NULL)
    {
        divoom_system_free(phead_info);
        divoom_cJSON_Delete(root);
        divoom_print("err!\n");
        return;
    }
    root = divoom_cJSON_Parse(respone_data);
    divoom_system_free(respone_data);
    if(root == NULL)
    {
        divoom_system_free(phead_info);
        divoom_cJSON_Delete(root);
        divoom_print("err!\n");
        return;
    }
    obj = divoom_cJSON_GetObjectItem(root, "data");
    if(obj)
    {
        divoom_cJSON_GetObjectString(obj, "value", pdivoom_home_connnect_handle->find_str);
        
        if(strcmp(pdivoom_home_connnect_handle->find_str, divoom_home_connect_OperationState_Inactive) == 0)
        {
            strcpy(pdevice_info->status, "Inactive");
        }
        else if(strcmp(pdivoom_home_connnect_handle->find_str, divoom_home_connect_OperationState_Ready) == 0)
        {
            strcpy(pdevice_info->status, "Ready");
        }
        else if(strcmp(pdivoom_home_connnect_handle->find_str, divoom_home_connect_OperationState_DelayedStart) == 0)
        {
            strcpy(pdevice_info->status, "DelayedStart");
        }
        else if(strcmp(pdivoom_home_connnect_handle->find_str, divoom_home_connect_OperationState_Run) == 0)
        {
            strcpy(pdevice_info->status, "Run");
        }
        else if(strcmp(pdivoom_home_connnect_handle->find_str, divoom_home_connect_OperationState_Pause) == 0)
        {
            strcpy(pdevice_info->status, "Pause");
        }
        else if(strcmp(pdivoom_home_connnect_handle->find_str, divoom_home_connect_OperationState_ActionRequired) == 0)
        {
            strcpy(pdevice_info->status, "ActionRequired");
        }
        else if(strcmp(pdivoom_home_connnect_handle->find_str, divoom_home_connect_OperationState_Finished) == 0)
        {
            strcpy(pdevice_info->status, "Finished");
        }
        else if(strcmp(pdivoom_home_connnect_handle->find_str, divoom_home_connect_OperationState_Error) == 0)
        {
            strcpy(pdevice_info->status, "Error");
        }
        else if(strcmp(pdivoom_home_connnect_handle->find_str, divoom_home_connect_OperationState_Aborting) == 0)
        {
            strcpy(pdevice_info->status, "Aborting");
        }
        divoom_print("device name :%s, status: %s\n", pdevice_info->name, pdevice_info->status);
    }
    divoom_system_free(phead_info);
    divoom_cJSON_Delete(root);
}

void divoom_home_connect_get_data_from_json(char *json_str)
{
    char *key_str, device_id[128], *start, *end;
    int i;
    divoom_cJSON *root = NULL, *list, *item;

    key_str = divoom_system_malloc(512);
    root = divoom_cJSON_Parse(json_str);
    if(root)
    {
        /*
        {
          "items": [
            {
              "key": "Cooking.Oven.Option.SetpointTemperature",
              "name": "Target temperature for the oven",
              "uri": "/api/homeappliances/BOSCH-HNG6764B6-0000000011FF/programs/active/options/Cooking.Oven.Option.SetpointTemperature",
              "timestamp": 1556793979,
              "level": "hint",
              "handling": "none",
              "value": 200,
              "unit": "°C"
            }
          ]
        }
        */
        list = divoom_cJSON_GetObjectItem(root, "items");

        if(list)
        {
            for(i = 0; i < divoom_cJSON_GetArraySize(list); i++)
            {
                item = divoom_cJSON_GetArrayItem(list, i);
                if(item == NULL)
                {
                    break;
                }
                divoom_cJSON_GetObjectString(item, "uri", key_str);
                if(strlen(key_str) == 0)
                {
                    break;
                }
                start = &key_str[strlen("/api/homeappliances/")];
                end = strchr(start, '/');
                memcpy(device_id, start, (uint32)(end - start));
                device_id[(uint32)(end - start)] = 0;
                divoom_cJSON_GetObjectString(item, "key", key_str);
                if(strcmp(key_str, divoom_home_connect_msg_Connected) == 0)
                {
                    divoom_home_connect_get_device_info(device_id, NULL);
                }
                else if(strcmp(key_str, divoom_home_connect_msg_Disconnected) == 0)
                {
                    //将设备的连接状态设置为不连接
                    for(i = 0; i < pdivoom_home_connnect_handle->device_num; i++)
                    {
                        if(strcpy(device_id, pdivoom_home_connnect_handle->pdevice_list[i].id) == 0)
                        {
                            pdivoom_home_connnect_handle->pdevice_list[i].connect_type = 0;
                            break;
                        }
                    }
                }
                else if(strcmp(key_str, divoom_home_connect_msg_OperationState) == 0)
                {
                    divoom_home_connect_get_device_info(device_id, NULL);
                    if(pdivoom_home_connnect_handle->pcallback)
                    {
                        divoom_print("event clock info: %d, %d, %d\n",pdivoom_home_connnect_handle->indepence_index, 
                            pdivoom_home_connnect_handle->lcd_id, pdivoom_home_connnect_handle->clock_id);
                        pdivoom_home_connnect_handle->pcallback( pdivoom_home_connnect_handle->clock_id,
                            pdivoom_home_connnect_handle->lcd_id, pdivoom_home_connnect_handle->indepence_index);
                    }
                }
                
            }
        }
        divoom_cJSON_Delete(root);
    }
    divoom_system_free(key_str);
}

void divoom_home_connect_do_respone_data(char *response_data, int len)
{
	//检查过来的数据最后一个字符是否是JSON的结束符
	int real_len = len, off, find_flag_num = 0;
	char *json_str, *start_str, *cur_str, *find_str = pdivoom_home_connnect_handle->find_str;    

	if (pdivoom_home_connnect_handle->active_buf_len == 0)
	{
		//过滤前面不是{
		off = 0;
		while (response_data[off] != '{')
		{
			off++;
			if (off >= len)
			{
				return;
			}
		}
		response_data += off;
		len -= off;
	}
	while (response_data[real_len - 1] != '}')
	{
		if ((response_data[real_len - 1] >= 'a' && response_data[real_len - 1] <= 'z') ||
			(response_data[real_len - 1] >= 'A' && response_data[real_len - 1] <= 'Z'))
		{
			break;
		}
		real_len--;
	}

	if ((response_data[real_len - 1] != '}'))
	{
		//不完整的包
		memcpy(&pdivoom_home_connnect_handle->buf[pdivoom_home_connnect_handle->active_buf_len], response_data, len);
		pdivoom_home_connnect_handle->active_buf_len += len;
		return;
	}
	if (pdivoom_home_connnect_handle->active_buf_len)
	{
		memcpy(&pdivoom_home_connnect_handle->buf[pdivoom_home_connnect_handle->active_buf_len], response_data, real_len);
		pdivoom_home_connnect_handle->active_buf_len += real_len;
		pdivoom_home_connnect_handle->buf[pdivoom_home_connnect_handle->active_buf_len] = 0;
		json_str = pdivoom_home_connnect_handle->buf;
	}
	else
	{
		//response_data[real_len] = 0;
		json_str = response_data;
	}
	find_flag_num = 1;
	start_str = json_str;
	cur_str = json_str + 1;
	while (*cur_str)
	{
		if (*cur_str == '{')
		{
			find_flag_num++;
		}
		else if (*cur_str == '}')
		{
			find_flag_num--;
			if (find_flag_num == 0)
			{
				//这个位置应该解析一下JSON数据
				char *ptr = cur_str;
				int find_len = (uint32)(cur_str - start_str);
				cur_str++;
				memcpy(find_str, start_str, find_len + 1);
				find_str[find_len+ 1] = 0;
                //对find_str进行数据解析
                divoom_home_connect_get_data_from_json(find_str);                
                
				start_str = cur_str;
				while (*cur_str && *cur_str != '{')
				{
					cur_str++;
				}
				if (*cur_str == '{')
				{
					//下一个解析开始
					start_str = cur_str;
				}
				else if (*cur_str == 0)
				{
					return ;
				}
				else
				{
					divoom_print("err: %s\n", ptr);
					return;
				}
				find_flag_num = 1;
			}
		}
		cur_str++;
	}
}


int divoom_home_connect_request_all(void);

int divoom_home_connect_request_all(void)
{
    int i, device_num;
    char *respone_data;    
    Pdivoom_http_client_url_head_info phead_info;
    divoom_cJSON *root = NULL, *obj, *list, *item;
    
    phead_info = divoom_system_malloc(sizeof(divoom_http_client_url_head_info));
    strcpy(phead_info->key, "Authorization");
    strcpy(phead_info->word, "Bearer ");
    divoom_system_get_lock(pdivoom_home_connnect_handle->lock);
    strcat(phead_info->word, pdivoom_home_connnect_handle->word);
    divoom_system_free_lock(pdivoom_home_connnect_handle->lock);
    respone_data = divoom_http_client_url_request("https://api.home-connect.com/api/homeappliances", phead_info, 1, NULL, NULL);
    if(respone_data == NULL)
    {
        divoom_print("err!\n");
        divoom_system_free(phead_info);
        return 0;
    }
    divoom_print("net_data: %s\n", respone_data);
    root = divoom_cJSON_Parse(respone_data);
    divoom_system_free(respone_data);
    if(root == NULL)
    {
        divoom_print("err!\n");
        divoom_system_free(phead_info);
        return 0;
    }
    obj = divoom_cJSON_GetObjectItem(root, "error");
    if(obj)
    {
        //这里出错了
        
        divoom_cJSON_GetObjectString(obj, "key", pdivoom_home_connnect_handle->find_str);
        if(strcmp(pdivoom_home_connnect_handle->find_str, "invalid_token") == 0)
        {
            //token过期
            divoom_clock_id_info request = {0};
            request.clock_id = pdivoom_home_connnect_handle->clock_id;
            divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_TIME_DIAL_ALL_DATA, DIVOOM_SERVER_TOPIC,&request);
            pdivoom_home_connnect_handle->request_all_flag = 1;
        }
        
        divoom_print("err!\n");
        divoom_cJSON_Delete(root);
        divoom_system_free(phead_info);
        return 0;
    }
    obj = divoom_cJSON_GetObjectItem(root, "data");
    list = divoom_cJSON_GetObjectItem(obj, "homeappliances");
    device_num = divoom_cJSON_GetArraySize(list);
    if(device_num == 0)
    {
        divoom_cJSON_Delete(root);
        divoom_system_free(phead_info);
        divoom_print("err!\n");
        return 0;
    }
    pdivoom_home_connnect_handle->device_num = device_num;
    pdivoom_home_connnect_handle->pdevice_list = divoom_system_malloc(sizeof(divoom_home_connect_device_item) * device_num);
    memset(pdivoom_home_connnect_handle->pdevice_list, 0, sizeof(divoom_home_connect_device_item) * device_num);
    for(i = 0; i < device_num; i ++)
    {
        item = divoom_cJSON_GetArrayItem(list, i);
        if(item == NULL)
        {
            break;
        }
        divoom_cJSON_GetObjectString(item, "name", pdivoom_home_connnect_handle->pdevice_list[i].name);
        divoom_cJSON_GetObjectString(item, "type", pdivoom_home_connnect_handle->pdevice_list[i].type);
        divoom_cJSON_GetObjectString(item, "haId", pdivoom_home_connnect_handle->pdevice_list[i].id);
        strcpy(pdivoom_home_connnect_handle->pdevice_list[i].status , "Ready");
        pdivoom_home_connnect_handle->pdevice_list[i].connect_type = divoom_cJSON_GetObjectInt(item, "connected");
        divoom_print("device info:id: %d, con: %d, name: %s; haid: %s; status: %s\n", i, pdivoom_home_connnect_handle->pdevice_list[i].connect_type
            ,pdivoom_home_connnect_handle->pdevice_list[i].name, pdivoom_home_connnect_handle->pdevice_list[i].id, pdivoom_home_connnect_handle->pdevice_list[i].status);
    }
    divoom_cJSON_Delete(root);
    pdivoom_home_connnect_handle->request_all_flag = 1;

    for(i = 0; i < device_num; i++)
    {
        if(pdivoom_home_connnect_handle->run_flag != 1)
        {
            break;
        }
        
        divoom_home_connect_get_device_info(pdivoom_home_connnect_handle->pdevice_list[i].id, &pdivoom_home_connnect_handle->pdevice_list[i]);
    }
    divoom_system_free(phead_info);
    return 1;
}

/*********************************************************************
  Function:        divoom_home_connect_init_handle
  Description:     初始化相关句柄
                    word        auth token
                    pcallback
                    clock_id
                    indepence_index
                    lcd_id
  Return:           none
  Author:           albert
**********************************************************************/

int divoom_home_connect_init_handle(char *word, DIVOOM_HOME_CONNECT_EVENT_NOTICE *pcallback)
{
    //先得到所有设备的ID与名称
    //https://simulator.home-connect.com/api/homeappliances

    
    if(pdivoom_home_connnect_handle != NULL)
    {
        divoom_system_get_lock(pdivoom_home_connnect_handle->lock);
        if(pdivoom_home_connnect_handle->word)
        {
            divoom_system_free(pdivoom_home_connnect_handle->word);
        }
        pdivoom_home_connnect_handle->word = divoom_app_com_copy_string(word, 0);
        divoom_system_free_lock(pdivoom_home_connnect_handle->lock);
        //pdivoom_home_connnect_handle->active_id = -1;
        if(pdivoom_home_connnect_handle->request_all_flag)
        {
            divoom_home_connect_request_all();
        }
        return 0;
    }
    pdivoom_home_connnect_handle = divoom_system_malloc(sizeof(divoom_home_connect_ctrl));
    pdivoom_home_connnect_handle->word = divoom_app_com_copy_string(word, 0);
    pdivoom_home_connnect_handle->pcallback = pcallback;
    pdivoom_home_connnect_handle->run_flag = 1;
    pdivoom_home_connnect_handle->active_id = -1;
    pdivoom_home_connnect_handle->request_all_flag = 0;
    pdivoom_home_connnect_handle->lock = divoom_system_create_lock();
    divoom_print("we will init handle!\n");
    //strcpy(phead_info.word, "Bearer eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6IjE5IiwieC1yZWciOiJTSU0iLCJ4LWVudiI6IlBSRCJ9.eyJzdWIiOiJkZXZwb3J0YWxVc2VySWQ6NTQ2ODMiLCJleHAiOjE2ODg3MjY5NjYsInNjb3BlIjpbIkNsZWFuaW5nUm9ib3QiLCJDbGVhbmluZ1JvYm90LUNvbnRyb2wiLCJDbGVhbmluZ1JvYm90LU1vbml0b3IiLCJDbGVhbmluZ1JvYm90LVNldHRpbmdzIiwiQ29mZmVlTWFrZXIiLCJDb2ZmZWVNYWtlci1Db250cm9sIiwiQ29mZmVlTWFrZXItTW9uaXRvciIsIkNvZmZlZU1ha2VyLVNldHRpbmdzIiwiQ29udHJvbCIsIkNvb2tQcm9jZXNzb3IiLCJDb29rUHJvY2Vzc29yLUNvbnRyb2wiLCJDb29rUHJvY2Vzc29yLU1vbml0b3IiLCJDb29rUHJvY2Vzc29yLVNldHRpbmdzIiwiRGlzaHdhc2hlciIsIkRpc2h3YXNoZXItQ29udHJvbCIsIkRpc2h3YXNoZXItTW9uaXRvciIsIkRpc2h3YXNoZXItU2V0dGluZ3MiLCJEcnllciIsIkRyeWVyLUNvbnRyb2wiLCJEcnllci1Nb25pdG9yIiwiRHJ5ZXItU2V0dGluZ3MiLCJGcmVlemVyIiwiRnJlZXplci1Db250cm9sIiwiRnJlZXplci1Nb25pdG9yIiwiRnJlZXplci1TZXR0aW5ncyIsIkZyaWRnZUZyZWV6ZXItQ29udHJvbCIsIkZyaWRnZUZyZWV6ZXItTW9uaXRvciIsIkZyaWRnZUZyZWV6ZXItU2V0dGluZ3MiLCJIb2IiLCJIb2ItQ29udHJvbCIsIkhvYi1Nb25pdG9yIiwiSG9iLVNldHRpbmdzIiwiSG9vZCIsIkhvb2QtQ29udHJvbCIsIkhvb2QtTW9uaXRvciIsIkhvb2QtU2V0dGluZ3MiLCJJZGVudGlmeUFwcGxpYW5jZSIsIk1vbml0b3IiLCJPdmVuIiwiT3Zlbi1Db250cm9sIiwiT3Zlbi1Nb25pdG9yIiwiT3Zlbi1TZXR0aW5ncyIsIlJlZnJpZ2VyYXRvciIsIlJlZnJpZ2VyYXRvci1Db250cm9sIiwiUmVmcmlnZXJhdG9yLU1vbml0b3IiLCJSZWZyaWdlcmF0b3ItU2V0dGluZ3MiLCJTZXR0aW5ncyIsIldhc2hlciIsIldhc2hlci1Db250cm9sIiwiV2FzaGVyLU1vbml0b3IiLCJXYXNoZXItU2V0dGluZ3MiLCJXYXNoZXJEcnllciIsIldhc2hlckRyeWVyLUNvbnRyb2wiLCJXYXNoZXJEcnllci1Nb25pdG9yIiwiV2FzaGVyRHJ5ZXItU2V0dGluZ3MiLCJXaW5lQ29vbGVyIiwiV2luZUNvb2xlci1Db250cm9sIiwiV2luZUNvb2xlci1Nb25pdG9yIiwiV2luZUNvb2xlci1TZXR0aW5ncyJdLCJhenAiOiIwRUIwRTI1MkJDRTBCODdDQTNBRTREQTNGMjlBOUIwQzg0QUJFMjRDMDhDMjkwOTFDODg0OEQyOTFBOUQ0MURGIiwiYXVkIjoiMEVCMEUyNTJCQ0UwQjg3Q0EzQUU0REEzRjI5QTlCMEM4NEFCRTI0QzA4QzI5MDkxQzg4NDhEMjkxQTlENDFERiIsInBybSI6W10sImlzcyI6ImV1OnNpbTpvYXV0aDoxIiwianRpIjoiNDZjMzE4MDgtODY2OC00MzRlLTgxNGYtNDBjZThlNTc3MTNhIiwiaWF0IjoxNjg4NjQwNTY2fQ.UgaKgf68DDWF4XqKMNbPfp7nDzuOv05Bp_qFRm4y35a4uWOOnE1TOSEIwRwSYlNaebXlqkLHuWs8VjxoEoWjeg");
    //divoom_system_create_task_prio(divoom_home_connect_main, "SSE", NULL, 20*1024, 1);
    return 1;
}

/*********************************************************************
  Function:        divoom_home_connect_update_autho_token
  Description:     更新autho token
                    word        auth token
  Return:           none
  Author:           albert
**********************************************************************/
void divoom_home_connect_update_autho_token(char *word)
{
    if(pdivoom_home_connnect_handle)
    {
        divoom_print("update token");
        divoom_system_get_lock(pdivoom_home_connnect_handle->lock);
        if(pdivoom_home_connnect_handle->word)
        {
            divoom_system_free(pdivoom_home_connnect_handle->word);
        }
        pdivoom_home_connnect_handle->word = divoom_app_com_copy_string(word, 0);
        divoom_system_free_lock(pdivoom_home_connnect_handle->lock);
    }
}

/*********************************************************************
  Function:        divoom_home_connect_start_event
  Description:     用户启动通知时调用
                    word        auth token
                    pcallback
                    clock_id
                    indepence_index
                    lcd_id
  Return:           none
  Author:           albert
**********************************************************************/
void divoom_home_connect_start_event(char *word, DIVOOM_HOME_CONNECT_EVENT_NOTICE *pcallback, int clock_id, int indepence_index, int lcd_id)
{
    divoom_home_connect_init_handle(word, pcallback);
    pdivoom_home_connnect_handle->notice_flag ++;
    pdivoom_home_connnect_handle->clock_id = clock_id;
    pdivoom_home_connnect_handle->indepence_index = indepence_index;
    pdivoom_home_connnect_handle->lcd_id = lcd_id;
    divoom_print("event num: %d, %d, %d, %d, %d\n", pdivoom_home_connnect_handle->notice_flag, pdivoom_home_connnect_handle->clock_flag, clock_id, indepence_index, lcd_id);
}

/*********************************************************************
  Function:        divoom_home_connect_cancel_event
  Description:     取消通知事件
  Return:           none
  Author:           albert
**********************************************************************/
void divoom_home_connect_cancel_event(void)
{
    if(pdivoom_home_connnect_handle)
    {
        divoom_print("cancel event!\n");
        pdivoom_home_connnect_handle->active_id = -1;
    }
}

void divoom_home_connect_destory(void)
{
    int try_times = 0;
    
    divoom_print("start exit!\n");
    pdivoom_home_connnect_handle->run_flag = 0;
    while(pdivoom_home_connnect_handle->run_flag != 2 && try_times < 600)
    {
        divoom_system_delay_ms(100);
        try_times ++;
    }
    divoom_print("end exit: %d, %d\n", pdivoom_home_connnect_handle->run_flag, try_times);
    if(pdivoom_home_connnect_handle->word)
    {
        divoom_system_free(pdivoom_home_connnect_handle->word);
    }
    divoom_system_destroy_lock(pdivoom_home_connnect_handle->lock);
    divoom_system_free(pdivoom_home_connnect_handle);
    pdivoom_home_connnect_handle = NULL;
    divoom_print("over exit!\n");
}

/*********************************************************************
  Function:        divoom_home_connect_stop_event
  Description:     用户关闭通知时调用
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_home_connect_stop_event(void)
{
    if(pdivoom_home_connnect_handle == NULL)
    {
        return;
    }
    pdivoom_home_connnect_handle->notice_flag --;
    divoom_print("notice num: %d\n", pdivoom_home_connnect_handle->notice_flag);
}
/*
{
   "error": {
     "key": "invalid_token",
    "description": "The access token expired."
 }
 }
 */


/*********************************************************************
  Function:        divoom_home_connect_start_clock
  Description:     用户切换表盘时调用，后面将数据获取时，将获取所有在线的设备数据
                    word        auth token
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_home_connect_start_clock(char *word, int indepence_index, int lcd_id, int clock_id)
{   
    if(pdivoom_home_connnect_handle)
    {
        pdivoom_home_connnect_handle->reolad_status = 1;
    }
    divoom_home_connect_init_handle(word, NULL);
    pdivoom_home_connnect_handle->clock_flag ++;
    pdivoom_home_connnect_handle->clock_id = clock_id;
    pdivoom_home_connnect_handle->indepence_index = indepence_index;
    pdivoom_home_connnect_handle->lcd_id = lcd_id;
    divoom_print("clock num: %d, %d\n", pdivoom_home_connnect_handle->clock_flag, pdivoom_home_connnect_handle->notice_flag);
}

/*********************************************************************
  Function:        divoom_home_connect_exit_clock
  Description:     显示表盘将退出
                    word        auth token
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_home_connect_exit_clock(void)
{   
    if(pdivoom_home_connnect_handle == NULL)
    {
        return;
    }
    pdivoom_home_connnect_handle->clock_flag --;
    divoom_print("clock num: %d\n", pdivoom_home_connnect_handle->clock_flag);
}

/*********************************************************************
  Function:        divoom_home_connect_get_disp_data
  Description:     获取表盘显示数据，如果
                    word        auth token
  Return:           none
  Author:           albert
**********************************************************************/

uint8 divoom_home_connect_get_disp_data(Pdivoom_clock_control_disp_data pdisp_data, int flag)
{
    
    if(pdivoom_home_connnect_handle == NULL)
    {
        return 0;
    }
    if(pdivoom_home_connnect_handle->request_all_flag == 0)
    {
        divoom_print("no finish!\n");
        DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, pdisp_data->num).num = 1;
        strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, pdisp_data->num).pdisp_name[0], "loading");
        strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, pdisp_data->num).pdisp_data[0], "loading");
        pdisp_data->num = 1;
        return 0;
    }
    if(pdivoom_home_connnect_handle->pdevice_list == NULL)
    {
        divoom_print("no device");
        DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, pdisp_data->num).num = 1;
        strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, pdisp_data->num).pdisp_name[0], "---");
        strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, pdisp_data->num).pdisp_data[0], "---");
        pdisp_data->num = 1;
        return 1;
    }
    if(flag ==1 || divoom_light_app_check_notice_status())
    {
        if(pdivoom_home_connnect_handle->active_id >= pdivoom_home_connnect_handle->device_num)
        {
            pdivoom_home_connnect_handle->active_id =  0;
        }
        pdisp_data->num = 1;
        pdisp_data->update = 1;
        DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).num = 1;
        strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_name[0], pdivoom_home_connnect_handle->pdevice_list[pdivoom_home_connnect_handle->active_id].name);
        strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[0], pdivoom_home_connnect_handle->pdevice_list[pdivoom_home_connnect_handle->active_id].status);
    }
    else
    {
        int i;
        
        pdisp_data->num = 0;
        pdisp_data->update = 1;
        for(i = 0; i < pdivoom_home_connnect_handle->device_num; i++)
        {
            if(pdivoom_home_connnect_handle->pdevice_list[i].connect_type)
            {
                DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, pdisp_data->num).num = 1;
                strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, pdisp_data->num).pdisp_name[0], pdivoom_home_connnect_handle->pdevice_list[i].name);
                strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, pdisp_data->num).pdisp_data[0], pdivoom_home_connnect_handle->pdevice_list[i].status);
                pdisp_data->num ++;
            }
        }
        if(pdisp_data->num == 0)
        {
            DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, pdisp_data->num).num = 1;
            strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, pdisp_data->num).pdisp_name[0], "---");
            strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, pdisp_data->num).pdisp_data[0], "---");
            pdisp_data->num = 1;
        }
    }
    return 1;
}

#if 0
/*
    下面是做相关测试的
*/

#define DIVOOM_HOME_CONNECT_AUTHO_TOKEN_STR "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6IjE5IiwieC1yZWciOiJTSU0iLCJ4LWVudiI6IlBSRCJ9.eyJzdWIiOiJkZXZwb3J0YWxVc2VySWQ6NTQ2ODMiLCJleHAiOjE2ODkxMzIwNTcsInNjb3BlIjpbIkNsZWFuaW5nUm9ib3QiLCJDbGVhbmluZ1JvYm90LUNvbnRyb2wiLCJDbGVhbmluZ1JvYm90LU1vbml0b3IiLCJDbGVhbmluZ1JvYm90LVNldHRpbmdzIiwiQ29mZmVlTWFrZXIiLCJDb2ZmZWVNYWtlci1Db250cm9sIiwiQ29mZmVlTWFrZXItTW9uaXRvciIsIkNvZmZlZU1ha2VyLVNldHRpbmdzIiwiQ29udHJvbCIsIkNvb2tQcm9jZXNzb3IiLCJDb29rUHJvY2Vzc29yLUNvbnRyb2wiLCJDb29rUHJvY2Vzc29yLU1vbml0b3IiLCJDb29rUHJvY2Vzc29yLVNldHRpbmdzIiwiRGlzaHdhc2hlciIsIkRpc2h3YXNoZXItQ29udHJvbCIsIkRpc2h3YXNoZXItTW9uaXRvciIsIkRpc2h3YXNoZXItU2V0dGluZ3MiLCJEcnllciIsIkRyeWVyLUNvbnRyb2wiLCJEcnllci1Nb25pdG9yIiwiRHJ5ZXItU2V0dGluZ3MiLCJGcmVlemVyIiwiRnJlZXplci1Db250cm9sIiwiRnJlZXplci1Nb25pdG9yIiwiRnJlZXplci1TZXR0aW5ncyIsIkZyaWRnZUZyZWV6ZXItQ29udHJvbCIsIkZyaWRnZUZyZWV6ZXItTW9uaXRvciIsIkZyaWRnZUZyZWV6ZXItU2V0dGluZ3MiLCJIb2IiLCJIb2ItQ29udHJvbCIsIkhvYi1Nb25pdG9yIiwiSG9iLVNldHRpbmdzIiwiSG9vZCIsIkhvb2QtQ29udHJvbCIsIkhvb2QtTW9uaXRvciIsIkhvb2QtU2V0dGluZ3MiLCJJZGVudGlmeUFwcGxpYW5jZSIsIk1vbml0b3IiLCJPdmVuIiwiT3Zlbi1Db250cm9sIiwiT3Zlbi1Nb25pdG9yIiwiT3Zlbi1TZXR0aW5ncyIsIlJlZnJpZ2VyYXRvciIsIlJlZnJpZ2VyYXRvci1Db250cm9sIiwiUmVmcmlnZXJhdG9yLU1vbml0b3IiLCJSZWZyaWdlcmF0b3ItU2V0dGluZ3MiLCJTZXR0aW5ncyIsIldhc2hlciIsIldhc2hlci1Db250cm9sIiwiV2FzaGVyLU1vbml0b3IiLCJXYXNoZXItU2V0dGluZ3MiLCJXYXNoZXJEcnllciIsIldhc2hlckRyeWVyLUNvbnRyb2wiLCJXYXNoZXJEcnllci1Nb25pdG9yIiwiV2FzaGVyRHJ5ZXItU2V0dGluZ3MiLCJXaW5lQ29vbGVyIiwiV2luZUNvb2xlci1Db250cm9sIiwiV2luZUNvb2xlci1Nb25pdG9yIiwiV2luZUNvb2xlci1TZXR0aW5ncyJdLCJhenAiOiIwRUIwRTI1MkJDRTBCODdDQTNBRTREQTNGMjlBOUIwQzg0QUJFMjRDMDhDMjkwOTFDODg0OEQyOTFBOUQ0MURGIiwiYXVkIjoiMEVCMEUyNTJCQ0UwQjg3Q0EzQUU0REEzRjI5QTlCMEM4NEFCRTI0QzA4QzI5MDkxQzg4NDhEMjkxQTlENDFERiIsInBybSI6W10sImlzcyI6ImV1OnNpbTpvYXV0aDoxIiwianRpIjoiMjU2YjAxYmEtZjhmZi00MmQwLThhZDctMDc0MDAxNzAxZDM0IiwiaWF0IjoxNjg5MDQ1NjU3fQ.RS3bq4GjiIbuQvcS-MG28H1e2dcFfGggmH8VLpTqmue99XbrLQ_JzY4_Z-Ay20djnX7gytIngQwxjVf21IvhHw"

#define DIVOOM_HOME_CONNECT_EVENT_TEST_STR1 "{  \"items\": [    {      \"key\": \"BSH.Common.Status.OperationState\",      \"name\": \"Target temperature for the oven\",      \"uri\": \"/api/homeappliances/SIEMENS-HCS05FRF1-94C11874474B/programs/active/options/Cooking.Oven.Option.SetpointTemperature\",      \"timestamp\": 1556793979,      \"level\": \"hint\",      \"handling\": \"none\",      \"value\": 200,      \"unit\": \"°C\"    }  ]}"
#define DIVOOM_HOME_CONNECT_EVENT_TEST_STR2 "{  \"items\": [    {      \"key\": \"BSH.Common.Status.OperationState\",      \"name\": \"Target temperature for the oven\",      \"uri\": \"/api/homeappliances/BOSCH-HCS01OVN1-5B101DB354BE/programs/active/options/Cooking.Oven.Option.SetpointTemperature\",      \"timestamp\": 1556793979,      \"level\": \"hint\",      \"handling\": \"none\",      \"value\": 200,      \"unit\": \"°C\"    }  ]}"
#define DIVOOM_HOME_CONNECT_EVENT_TEST_STR3 "{  \"items\": [    {      \"key\": \"BSH.Common.Status.OperationState\",      \"name\": \"Target temperature for the oven\",      \"uri\": \"/api/homeappliances/BOSCH-HCS06COM1-E49F0ED99296/programs/active/options/Cooking.Oven.Option.SetpointTemperature\",      \"timestamp\": 1556793979,      \"level\": \"hint\",      \"handling\": \"none\",      \"value\": 200,      \"unit\": \"°C\"    }  ]}"
#define DIVOOM_HOME_CONNECT_EVENT_TEST_STR4 "{  \"items\": [    {      \"key\": \"BSH.Common.Status.OperationState\",      \"name\": \"Target temperature for the oven\",      \"uri\": \"/api/homeappliances/SIEMENS-HCS02DWH1-06036E00DD28/programs/active/options/Cooking.Oven.Option.SetpointTemperature\",      \"timestamp\": 1556793979,      \"level\": \"hint\",      \"handling\": \"none\",      \"value\": 200,      \"unit\": \"°C\"    }  ]}"
#define DIVOOM_HOME_CONNECT_EVENT_TEST_STR5 "{  \"items\": [    {      \"key\": \"BSH.Common.Status.OperationState\",      \"name\": \"Target temperature for the oven\",      \"uri\": \"/api/homeappliances/SIEMENS-HCS03WCH1-7EDBD9212966/programs/active/options/Cooking.Oven.Option.SetpointTemperature\",      \"timestamp\": 1556793979,      \"level\": \"hint\",      \"handling\": \"none\",      \"value\": 200,      \"unit\": \"°C\"    }  ]}"
#define DIVOOM_HOME_CONNECT_EVENT_TEST_STR6 "{  \"items\": [    {      \"key\": \"BSH.Common.Status.OperationState\",      \"name\": \"Target temperature for the oven\",      \"uri\": \"/api/homeappliances/BOSCH-HCS04DYR1-2FC7C09D9EA9/programs/active/options/Cooking.Oven.Option.SetpointTemperature\",      \"timestamp\": 1556793979,      \"level\": \"hint\",      \"handling\": \"none\",      \"value\": 200,      \"unit\": \"°C\"    }  ]}"

void divoom_home_connect_test_notice(int indepence_index, int lcd_id, int clock_id)
{
    Pdivoom_clock_control_disp_data pdisp_data;
    
    divoom_print("notice info: %d, %d, %d\n", indepence_index, lcd_id, clock_id);
    pdisp_data = divoom_system_malloc(sizeof(divoom_clock_control_disp_data));
    if(pdisp_data)
    {
        divoom_clock_control_disp_data_init(pdisp_data);
        divoom_home_connect_get_disp_data(pdisp_data, 1);
        divoom_clock_control_disp_data_deinit(pdisp_data);
        divoom_system_free(pdisp_data);
    }
}

void divoom_home_connect_test_main(void *para)
{
    char *event_str_table[] = 
    {
        DIVOOM_HOME_CONNECT_EVENT_TEST_STR1,
        DIVOOM_HOME_CONNECT_EVENT_TEST_STR2,
        DIVOOM_HOME_CONNECT_EVENT_TEST_STR3,
        DIVOOM_HOME_CONNECT_EVENT_TEST_STR4,
        DIVOOM_HOME_CONNECT_EVENT_TEST_STR5,
        DIVOOM_HOME_CONNECT_EVENT_TEST_STR6,
    };
    int test_notice = 0;
    divoom_home_connect_start_event(DIVOOM_HOME_CONNECT_AUTHO_TOKEN_STR, divoom_home_connect_test_notice, 4, 3, 2);

    while(1)
    {
        if(pdivoom_home_connnect_handle->device_num)
        {
            divoom_home_connect_get_data_from_json(event_str_table[test_notice]);
            test_notice ++;
            if(test_notice >= 6)
            {
                test_notice = 0;
            }
        }
        else
        {
            divoom_print("wait request!\n");
        }
        divoom_system_delay_ms(5000);
    }
}

void divoom_home_connect_test_init(void)
{
    
    divoom_system_create_task_prio(divoom_home_connect_test_main, "SSE", NULL, 20*1024, 1);
}

#endif



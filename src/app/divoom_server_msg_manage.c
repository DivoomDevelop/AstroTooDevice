#include "divoom.h"
/* MQTT topic 前缀返回指针，调用前必须使用完整函数声明。 */
#include "divoom_open_interface.h"

#define		DIVOOM_SERVER_MANAGE_TAG				"DIVOOM_SERVER_MANAGE"

#define		DIVOOM_SERVER_MANAGE_CONNECTING			(0x01)
#define		DIVOOM_SERVER_MANAGE_NOCONNECT			(0x11)
#define		DIVOOM_SERVER_MANAGE_CONNECTED			(0x10)

//#define		DIVOOM_SERVER_MSG_MANAGE_STACK			(5*1024)
#define		DIVOOM_SERVER_MSG_MANAGE_STACK			(6*1024)
#define		DIVOOM_SERVER_MSG_MANAGE_ADD_SERVER		(10*1024)
#define		DIVOOM_SERVER_MSG_MANAGE_PRIO			(5)

#define		DIVOOM_MQTT_ALIYUN_SEND_BUF	(10<<10)
#define		DIVOOM_MQTT_ALIYUN_RECV_BUF	(100<<10)
#define		DIVOOM_MQTT_CONNECT_SERVER_FAIL_MAX_TIME	(10)
#define		DIVOOM_SERVER_MSG_MANAGE_DATA_MAX	(128<<10)

#define		DIVOOM_SERVER_MSG_MANAGE_TAG				"DIVOOM_SER_MSG_MAN"

#define		DIVOOM_SERVER_MSG_MANAGE_PRINT divoom_print

typedef struct{
	MQTT_STATUS status;
	void *handle;
	uint8_t *topic;
	uint8_t *data;
	uint32_t data_len;
}divoom_server_msg_task_struct, *Pdivoom_server_msg_task_struct;

typedef struct _divoom_srv_image_ctrl
{
	uint32 old_image_flag; //记录一下旧包
	int cur_image_flag;//记录新包
	uint16 image_num;
	uint32 user_id;
	void *local_image_queue; //先用队列进行保存
}divoom_srv_image_ctrl, *Pdivoom_srv_image_ctrl;

typedef struct{
	uint8								task_flag;
	//不要再使用了,会暂停所有的task,然后delay的话就会出错
    //EventGroupHandle_t					status_bits;
	uint8								status_bits;
	
	void *					cmd_sem;
	void					*event_lock;
	void								*task_queue;

	DIVOOM_MQTT_RECV_DATA_CB			server_data_cb;
	DIVOOM_SERVER_MSG_STATUS_CALLBACK	server_status_cb;
	Pdivoom_server_msg_struct			server_data;
	divoom_srv_image_ctrl				local_image_ctrl;
	//为了检测是否连接上服务器
	//0是没有连接过
	uint8								add_server_flag;

	//char device_id[64];
	int device_id;
	char device_mac[13]; //username 用mac地址
	char lwt_topic[64];
	//保存阿里云的相关信息
	char aliyunid[64];
	char product_key[20];
	char product_secret[64];
    void *mqtt_dis_connect_handle;
    uint8 mqtt_type;
    uint8 server_ok_flag;                   //是否成功连接过服务器
    int   fails_connect_server_con_times;   //连续连接服务器失败次数
}divoom_server_msg_all_struct, *Pdivoom_server_msg_all_struct;

Pdivoom_server_msg_all_struct pdivoom_server_msg_all_handle = NULL;

static void divoom_server_msg_manage_net_callback(MQTT_STATUS status, void *handle, uint8_t *topic, uint8_t *data, uint32_t data_len);
static void *divoom_server_msg_manage_do_msg_task(void *arg);
uint8 divoom_server_msg_manage_event(Pdivoom_sys_msg_info msg);


/*********************************************************
  * CREATED TIME	: 2018年 07月 16日 星期一 18:11:09 CST
  * FUNCTION		: divoom_server_msg_manage_init
  * IUPUT			: DIVOOM_MQTT_RECV_DATA_CB server_data_cb, DIVOOM_SERVER_MSG_STATUS_CALLBACK server_status_cb, Pdivoom_server_msg_struct server_data
  * OUTPUT			: void 
  * DESCRIPTION		: 初始化数据，需要保存起来，在WiFi连接上之后再使用,divoom_mqtt_model_init先放在这里初始化，这个函数最好是在WiFi初始化之前处理
						现在用阿里云，不需要外面传信息了。加个宏控制一下
  *********************************************************/
void divoom_server_msg_manage_init(DIVOOM_MQTT_RECV_DATA_CB server_data_cb, DIVOOM_SERVER_MSG_STATUS_CALLBACK server_status_cb, Pdivoom_server_msg_struct server_data)
{
	if(pdivoom_server_msg_all_handle != NULL)
	{
		DIVOOM_SERVER_MSG_MANAGE_PRINT("Wrong in %s:%d\n", __func__, __LINE__);
		return;
	}
	pdivoom_server_msg_all_handle = (Pdivoom_server_msg_all_struct)divoom_malloc(sizeof(divoom_server_msg_all_struct), DIVOOM_MODEL_SER_CON, __LINE__, __FILE__);
	if(pdivoom_server_msg_all_handle == NULL)
	{
		DIVOOM_SERVER_MSG_MANAGE_PRINT("Wrong in %s:%d\n", __func__, __LINE__);
		return;
	}
	memset(pdivoom_server_msg_all_handle, 0, sizeof(divoom_server_msg_all_struct));
	pdivoom_server_msg_all_handle->server_data = (Pdivoom_server_msg_struct)divoom_malloc(sizeof(divoom_server_msg_struct), DIVOOM_MODEL_SER_CON, __LINE__, __FILE__);
	if(pdivoom_server_msg_all_handle->server_data == NULL)
	{
		DIVOOM_SERVER_MSG_MANAGE_PRINT("Wrong in %s:%d\n", __func__, __LINE__);
		return;
	}
	pdivoom_server_msg_all_handle->server_data->add_ser_handle = NULL;
	pdivoom_server_msg_all_handle->server_data_cb = server_data_cb;
	pdivoom_server_msg_all_handle->server_status_cb = server_status_cb;
	pdivoom_server_msg_all_handle->add_server_flag = DIVOOM_SERVER_MANAGE_NOCONNECT;
	pdivoom_server_msg_all_handle->cmd_sem = divoom_system_create_sem(100, 0);
	pdivoom_server_msg_all_handle->event_lock = divoom_system_create_lock();
	pdivoom_server_msg_all_handle->task_flag = 1;
	pdivoom_server_msg_all_handle->status_bits = 1;
	pdivoom_server_msg_all_handle->local_image_ctrl.local_image_queue = divoom_queue_init(100, __func__, __LINE__);
	pdivoom_server_msg_all_handle->local_image_ctrl.cur_image_flag = -1;

	if (server_data)
		memcpy(pdivoom_server_msg_all_handle->server_data, server_data, sizeof(divoom_server_msg_struct));
	pdivoom_server_msg_all_handle->server_data->add_ser_handle = NULL;
	divoom_mqtt_model_init();
	pdivoom_server_msg_all_handle->task_queue = divoom_queue_init(1000, __func__, __LINE__);
#ifdef DIVOOM_TASK_PRINTF_STACK_INFO
	pdivoom_server_msg_all_handle->task_info = divoom_system_create_task(divoom_server_msg_manage_do_msg_task, "srv_msg", (void *)pdivoom_server_msg_all_handle, DIVOOM_TASK_STACK_SERVER_MSG);
#else
	divoom_system_create_task(divoom_server_msg_manage_do_msg_task, "srv_msg", (void *)pdivoom_server_msg_all_handle, DIVOOM_TASK_STACK_SERVER_MSG);
#endif

    divoom_sys_model_crl_register(DIVOOM_MODEL_SER_CON, divoom_server_msg_manage_event);

	return;
}

void divoom_server_msg_set_lwt_topic(void)
{
	sprintf(pdivoom_server_msg_all_handle->lwt_topic, "%s%d/state",divoom_open_get_topic(), pdivoom_server_msg_all_handle->device_id);
}

char *divoom_server_msg_lwt_json_packet(uint8 state)
{
	char *cmd_string;
	divoom_cJSON *root;
	root = divoom_cJSON_CreateObject();
	if(!root)
	{
		return NULL;
	}
	divoom_cJSON_AddNumberToObject(root,"Online", state);    
	divoom_cJSON_AddStringToObject(root,"DeviceMac",pdivoom_server_msg_all_handle->device_mac);
	divoom_cJSON_AddNumberToObject(root, "DeviceId", pdivoom_server_msg_all_handle->device_id);
	cmd_string = divoom_cJSON_Print(root);
	divoom_cJSON_Delete(root);
	return cmd_string;
}

char *divoom_server_msg_app_lwt_json_packet(uint8 state)
{
	char *cmd_string;
	divoom_cJSON *root;

	root = divoom_cJSON_CreateObject();
	if(!root)
	{
		return NULL;
	}
	divoom_cJSON_AddNumberToObject(root,"Online", state);    
	divoom_cJSON_AddNumberToObject(root, "DeviceId", pdivoom_server_msg_all_handle->device_id);
	divoom_cJSON_AddStringToObject(root, "Command", "Device/DisconnectMqtt");
	cmd_string = divoom_cJSON_Print(root);
	divoom_cJSON_Delete(root);
	return cmd_string;
}

uint8 divoom_server_msg_manage_event(Pdivoom_sys_msg_info msg)
{
    divoom_print("%s:event id: %d\n", __func__, msg->msg_id);
	divoom_system_get_lock(pdivoom_server_msg_all_handle->event_lock);
	switch(msg->msg_id)
	{
        case DIVOOM_SERVER_MSG_DISCONNECT_MQTT:
        {
            
            divoom_mqtt_disconnect_cb(pdivoom_server_msg_all_handle->mqtt_dis_connect_handle, pdivoom_server_msg_all_handle->mqtt_type);
        }
            break;
            
		case DIVOOM_SERVER_MSG_CONNECT_DIVOOM:
			{
				Pdivoom_server_msg_struct msg_info = divoom_malloc(sizeof(divoom_server_msg_struct), DIVOOM_MODEL_SER_CON, __LINE__, __FILE__);
				memcpy(msg_info, msg->data, sizeof(divoom_server_msg_struct));
				divoom_mqtt_struct info;
				memset(&info, 0, sizeof(divoom_mqtt_struct));

				info.is_reconnect = 1;
				info.type = MQTT_SER;
				info.port = msg_info->port;
				info.username = msg_info->username;
				info.password = msg_info->password;
				info.client_id = msg_info->clinet_id;
				info.host = msg_info->host;
				info.device_id = msg_info->device_id;
				strcpy(pdivoom_server_msg_all_handle->device_mac, msg_info->username);
				pdivoom_server_msg_all_handle->device_id = msg_info->device_id;
				divoom_server_msg_set_lwt_topic();
				char *lwt_msg = divoom_server_msg_lwt_json_packet(0);
				info.lwt_topic = pdivoom_server_msg_all_handle->lwt_topic;
				info.lwt_msg = lwt_msg;
				if(divoom_mqtt_model_add_server(&info, divoom_server_msg_manage_net_callback, info.type) != 0)
				{
					DIVOOM_SERVER_MSG_MANAGE_PRINT("Some Wrong in %s:%d\n", __func__, __LINE__);
				}
				divoom_free(lwt_msg);
				divoom_free_line(msg_info, __FILE__, __LINE__);
			}
			break;

		case DIVOOM_SERVER_MSG_ADD_SERVER:
			{
				Pdivoom_server_msg_struct msg_info = divoom_malloc(sizeof(divoom_server_msg_struct), DIVOOM_MODEL_SER_CON, __LINE__, __FILE__);
				memcpy(msg_info, msg->data, sizeof(divoom_server_msg_struct));
				divoom_mqtt_struct info;
				memset(&info, 0, sizeof(divoom_mqtt_struct));

				info.is_reconnect = 1;
				info.type = MQTT_APP;

				info.port = msg_info->port;
				info.username = msg_info->username;
				info.password = msg_info->password;
				info.client_id = msg_info->clinet_id;
				info.host = msg_info->host;
				char lwt_topic[20] = DIVOOM_MQTT_APP_LWT_TOPIC;
				char *app_lwt_msg = divoom_server_msg_app_lwt_json_packet(0);
				info.lwt_topic = lwt_topic;
				info.lwt_msg = app_lwt_msg;
				divoom_print("port: %d user_name: %s password: %s client_id: %s host: %s\n", msg_info->port, msg_info->username, msg_info->password, msg_info->clinet_id, msg_info->host);
				if(divoom_mqtt_model_add_server(&info, divoom_server_msg_manage_net_callback, info.type) != 0)
				{
					DIVOOM_SERVER_MSG_MANAGE_PRINT("Some Wrong in %s:%d\n", __func__, __LINE__);
				}
				
				divoom_free(app_lwt_msg);
				divoom_free_line(msg_info, __FILE__, __LINE__);
			}
			break;
		case DIVOOM_SERVER_MSG_SUB_TOPIC:
			{
				Pdivoom_server_msg_topic_struct topic_info = (Pdivoom_server_msg_topic_struct)msg->data;
				if(topic_info->server_type == MQTT_SER)
				{
					//只有server才可以这样订阅，和手机端在连接上之后订阅一个就可以了
					divoom_mqtt_model_sub_topic(topic_info->server_type, topic_info->topic, NULL, topic_info->topic_type);
				}
			}
			break;
		case DIVOOM_SERVER_MSG_DEL_SERVER:
			{
				Pdivoom_server_msg_destory_struct connect_info = (Pdivoom_server_msg_destory_struct)msg->data;
				if(connect_info->server_type == MQTT_APP)
				{
					divoom_mqtt_model_discon_by_host(connect_info->host);
				}
			}
			break;
		case DIVOOM_SERVER_MSG_GET_SERVER_STATUS:
			break;
	}
	divoom_system_free_lock(pdivoom_server_msg_all_handle->event_lock);

	return 1;
}

void divoomserver_msg_manage_free(void *ptr)
{
	divoom_free_line(ptr, __FILE__, __LINE__);
}

void divoom_server_msg_free_local_image(void *data)
{
	Pdivoom_server_msg_task_struct task_info = (Pdivoom_server_msg_task_struct)data;

	if (task_info)
	{
		if (task_info->data)
		  divoom_free(task_info->data);
		if (task_info->topic)
			divoom_free(task_info->topic);
		divoom_free(task_info);
	}
}

void divoom_server_msg_clear_image_flag(int user_id)
{
	if(pdivoom_server_msg_all_handle)
	{
		if (pdivoom_server_msg_all_handle->local_image_ctrl.user_id == user_id)
			pdivoom_server_msg_all_handle->local_image_ctrl.cur_image_flag = -1;
	}
}

//先断开所有的连接,包括服务器的,手机的,然后再调用divoom_mqtt_model_exit释放所有资源
void divoom_server_msg_manage_deinit(void)
{
    divoom_sys_msg_info model_msg = {0};
	if(pdivoom_server_msg_all_handle)
	{
		divoom_system_get_lock(pdivoom_server_msg_all_handle->event_lock);
		divoom_system_free_lock(pdivoom_server_msg_all_handle->event_lock);
		//需要先看divoom_servermsg_manage_add_aliyun_task是否还在运行,Y->exit task
		if(pdivoom_server_msg_all_handle->add_server_flag == DIVOOM_SERVER_MANAGE_CONNECTING)
		{
			pdivoom_server_msg_all_handle->add_server_flag = 0;
			while(pdivoom_server_msg_all_handle->add_server_flag == 0)
			{
                divoom_system_delay_ms(100);
			}
		}
		//断开手机连接
		model_msg.src_id = DIVOOM_MODEL_SER_CON;
		model_msg.dest_id = DIVOOM_MODEL_MAIN;
		model_msg.msg_id = DIVOOM_MAIN_EVENT_DELETE_APP_ID;
		divoom_sys_model_crl_msg_push(&model_msg);

		//先断开手机连接,需要通知到服务器手机断开了
		//断开服务器连接
		divoom_server_msg_manage_destroy_connection();
		//释放所有资源
		if(pdivoom_server_msg_all_handle->server_data)
		{
			divoom_free_line(pdivoom_server_msg_all_handle->server_data, __FILE__, __LINE__);
		}
		pdivoom_server_msg_all_handle->task_flag = 0;
		divoom_system_put_sem(pdivoom_server_msg_all_handle->cmd_sem);
		while(pdivoom_server_msg_all_handle->status_bits == 1)
		{
		    divoom_system_delay_ms(1);
		}
		//先确保都退出了,然后清理这个函数
		divoom_sys_model_crl_unregister(DIVOOM_MODEL_SER_CON);
		divoom_system_destroy_sem(pdivoom_server_msg_all_handle->cmd_sem);
		divoom_system_destroy_lock(pdivoom_server_msg_all_handle->event_lock);
		divoom_queue_destroy(pdivoom_server_msg_all_handle->local_image_ctrl.local_image_queue, divoom_server_msg_free_local_image);
		divoom_queue_destroy(pdivoom_server_msg_all_handle->task_queue, divoomserver_msg_manage_free);
		divoom_free_line(pdivoom_server_msg_all_handle, __FILE__, __LINE__);
		pdivoom_server_msg_all_handle = NULL;
	}
}

void divoom_server_msg_manage_destroy_connection(void)
{
	if(pdivoom_server_msg_all_handle->server_data->add_ser_handle != NULL)
	{
        DIVOOM_SERVER_MSG_MANAGE_PRINT("%s:disconnect:%d\n", __func__, MQTT_DISCONNECTED);
		divoom_mqtt_disconnect_cb(pdivoom_server_msg_all_handle->server_data->add_ser_handle, MQTT_SER);
		pdivoom_server_msg_all_handle->server_data->add_ser_handle = NULL;
	}
}

//收到MQTT的消息，先存放到队列里，在task里面处理，避免阻塞divoom_mqtt_event_handle
static void divoom_server_msg_manage_net_callback(MQTT_STATUS status, void *handle, uint8_t *topic, uint8_t *data, uint32_t data_len)
{
	int image_flag = -1;
	uint32 user_id = 0, packet_id = 0;

	if(pdivoom_server_msg_all_handle == NULL)
		return;
	if (topic && status == MQTT_DATA)
	    DIVOOM_SERVER_MSG_MANAGE_PRINT("topic:%s\n", (char *)topic);
    DIVOOM_SERVER_MSG_MANAGE_PRINT("status:%d\n", status);
    if(status == MQTT_ERROR)
    {
        return;
    }
    if(status == MQTT_DATA && (topic == NULL || data == NULL || data_len == 0 || data_len > DIVOOM_SERVER_MSG_MANAGE_DATA_MAX))
    {
        DIVOOM_SERVER_MSG_MANAGE_PRINT("%s: drop invalid mqtt data topic=%p data=%p len=%u\n",
            __func__, topic, data, (unsigned int)data_len);
        return;
    }
	//MQTT_DATA的topic/data只在回调期间有效；入队前在这里复制一份
	Pdivoom_server_msg_task_struct task_info_handle = (Pdivoom_server_msg_task_struct)divoom_malloc(sizeof(divoom_server_msg_task_struct), DIVOOM_MODEL_SER_CON, __LINE__, __FILE__);
    if(task_info_handle == NULL)
    {
        return;
    }
    memset(task_info_handle, 0, sizeof(divoom_server_msg_task_struct));
	task_info_handle->status = status;
	task_info_handle->handle = handle;
    if(topic)
    {
	    task_info_handle->topic = (uint8_t *)divoom_app_com_copy_string((char *)topic, 0);
    }
    else
    {
	    task_info_handle->topic = NULL;
    }
    if(data_len)
    {
	  	task_info_handle->data = divoom_system_malloc_no_zero(data_len + 1);
        if(task_info_handle->data == NULL)
        {
            DIVOOM_SERVER_MSG_MANAGE_PRINT("%s: malloc data fail len=%u\n", __func__, (unsigned int)data_len);
            if(task_info_handle->topic)
            {
                divoom_system_free(task_info_handle->topic);
            }
            divoom_system_free(task_info_handle);
            return;
        }
        memcpy(task_info_handle->data, data, data_len);
        task_info_handle->data[data_len] = 0;
    }
    else
    {
        task_info_handle->data = NULL;
    }
	task_info_handle->data_len = data_len;

	if (topic != NULL && strcmp((char *)topic, DIVOOM_MQTT_APP_BIN_TOPIC) == 0)
	{
		image_flag = divoom_drawing_local_binary_get_image_flag(data, data_len, &user_id, &packet_id);
	}

	if (image_flag < 0)
	{
		if (0 == divoom_queue_push(pdivoom_server_msg_all_handle->task_queue, (void *)task_info_handle))
		{
	        DIVOOM_SERVER_MSG_MANAGE_PRINT("%s:over flow: %d\n", __func__, task_info_handle->status);
	        if(task_info_handle->topic)
	        {
	            divoom_system_free(task_info_handle->topic);
	        }
	        if(task_info_handle->data)
	        {
	            divoom_system_free(task_info_handle->data);
	        }
	        divoom_system_free(task_info_handle);
		}
		divoom_system_put_sem(pdivoom_server_msg_all_handle->cmd_sem);
	}
	else
	{
		if (user_id != pdivoom_server_msg_all_handle->local_image_ctrl.user_id)
		{
			pdivoom_server_msg_all_handle->local_image_ctrl.cur_image_flag = -1;
			pdivoom_server_msg_all_handle->local_image_ctrl.user_id = user_id;
		}
        #if 0
		if (image_flag != pdivoom_server_msg_all_handle->local_image_ctrl.cur_image_flag) 
		{
			if (image_flag > pdivoom_server_msg_all_handle->local_image_ctrl.cur_image_flag) //新的动画
			{
                divoom_print("image flag: %d, %d\n",image_flag, pdivoom_server_msg_all_handle->local_image_ctrl.cur_image_flag);
				pdivoom_server_msg_all_handle->local_image_ctrl.cur_image_flag = image_flag;
				divoom_queue_pop_all(pdivoom_server_msg_all_handle->local_image_ctrl.local_image_queue, divoom_server_msg_free_local_image);
				divoom_net_app_clear_cmd(DIVOOM_NET_COMM_LOCAL_DRAWING);
			}
			else
			{
				divoom_free(topic);
				divoom_free(data);
				divoom_free(task_info_handle);
				return ;
			}
		}
        #endif
		//把数据写进队列
		divoom_queue_push(pdivoom_server_msg_all_handle->local_image_ctrl.local_image_queue, task_info_handle);
		divoom_system_put_sem(pdivoom_server_msg_all_handle->cmd_sem);
	}
}
#ifdef DIVOOM_TASK_PRINTF_STACK_INFO
void divoom_server_msg_manage_print_task_info()
{
	if(pdivoom_server_msg_all_handle && pdivoom_server_msg_all_handle->task_info)
	{
		divoom_print("This task is divoom_server_msg_manage_do_msg_task free stack:%d\n", uxTaskGetStackHighWaterMark(pdivoom_server_msg_all_handle->task_info));
	}
}
#endif

//task处理，把收到的消息通过server_data_cb发出去,在net_manage处理
static void *divoom_server_msg_manage_do_msg_task(void *arg)
{
	Pdivoom_server_msg_all_struct task_handle = (Pdivoom_server_msg_all_struct)arg;
	Pdivoom_server_msg_task_struct task_info_handle = NULL;
	task_handle->status_bits = 1;
	while(task_handle->task_flag)
	{
		divoom_system_get_sem(task_handle->cmd_sem);
		while (divoom_queue_not_empty(task_handle->local_image_ctrl.local_image_queue) && task_handle->task_flag)
		{
			task_info_handle = (Pdivoom_server_msg_task_struct)divoom_queue_pop(task_handle->local_image_ctrl.local_image_queue);
			if (task_info_handle)
			{
				task_handle->server_status_cb(task_info_handle->status, NULL);
				if (task_info_handle->handle != NULL && task_info_handle->data != NULL && task_info_handle->topic != NULL)
				{
					task_handle->server_data_cb(MQTT_DATA, task_info_handle->handle, task_info_handle->topic, task_info_handle->data, task_info_handle->data_len);

					//data由server_data_cb解析后释放；topic由本队列释放
					divoom_system_free(task_info_handle->topic);
				}
                else
                {
                    if(task_info_handle->data)
                    {
                        divoom_system_free(task_info_handle->data);                                
                    }
                    if(task_info_handle->topic)
                    {
                        divoom_system_free(task_info_handle->topic);                                
                    }
                }
	    		divoom_system_free(task_info_handle);
			}
		}

        while(divoom_queue_not_empty(task_handle->task_queue) && task_handle->task_flag)
		{
    		task_info_handle = (Pdivoom_server_msg_task_struct)divoom_queue_pop(task_handle->task_queue);
			if (task_info_handle == NULL)
				continue;

    		DIVOOM_SERVER_MSG_MANAGE_PRINT("%s:%d\n", __func__, task_info_handle->status);
    		switch(task_info_handle->status)
    		{
    			case MQTT_DATA:
    				{
    					task_handle->server_status_cb(task_info_handle->status, NULL);
    					if(task_info_handle->handle != NULL && task_info_handle->data != NULL && task_info_handle->topic != NULL)
    					{
    						task_handle->server_data_cb(MQTT_DATA, task_info_handle->handle, task_info_handle->topic, task_info_handle->data, task_info_handle->data_len);

    						//data由server_data_cb解析后释放；topic由本队列释放
    						divoom_system_free(task_info_handle->topic);
    					}
                        else
                        {
                            if(task_info_handle->data)
                            {
                                divoom_system_free(task_info_handle->data);                                
                            }
                            if(task_info_handle->topic)
                            {
                                divoom_system_free(task_info_handle->topic);                                
                            }
                        }
    				}
    				break;
    			case MQTT_CONNECTED:
    				{
						Pdivoom_mqtt_model_discon_struct con_info = (Pdivoom_mqtt_model_discon_struct)task_info_handle->data;

    					if(con_info->type == MQTT_SER)
    					{
    						//只是为了WiFi连接的地方做判断
    						task_handle->server_data->add_ser_handle = task_info_handle->handle;
    					}
    					task_handle->server_status_cb(task_info_handle->status, (char *)con_info);

    					//发送订阅
    					if(con_info->type == MQTT_SER)
    					{
                            int product_flag = divoom_main_get_custom_product_flag();
							char topic_buf[128] = {0};

                            pdivoom_server_msg_all_handle->server_ok_flag = 1;
                            pdivoom_server_msg_all_handle->fails_connect_server_con_times = 0;
                            sprintf(topic_buf, "%s%d/get",divoom_open_get_topic(), pdivoom_server_msg_all_handle->device_id);
    						divoom_mqtt_model_sub_topic(MQTT_SER, topic_buf, task_info_handle->handle, 0);
							//sprintf(topic_buf, DIVOOM_MQTT_DEVICE_LWT_TOPIC, pdivoom_server_msg_all_handle->device_id);
    						//divoom_mqtt_model_sub_topic(MQTT_SER, topic_buf, task_info_handle->handle, 0);
    						//divoom_mqtt_model_sub_topic(MQTT_SER, DIVOOM_MQTT_DEVICE_HEART_TOPIC, task_info_handle->handle, 0);
                            if(product_flag)
                            {
                                sprintf(topic_buf, "%scustom%d/get",divoom_open_get_topic(), pdivoom_server_msg_all_handle->device_id);
    						    divoom_mqtt_model_sub_topic(MQTT_SER, topic_buf, task_info_handle->handle, 0);
                            }
    					}
    					
    					if(con_info->type == MQTT_SER)
    					{
                        	divoom_sys_msg_info  msg = {0};
							//uint32_t			 app_id = 0;
                        	msg.msg_id = DIVOOM_MAIN_EVENT_SRV_MQTT_CON;
                        	msg.src_id = DIVOOM_MODEL_NULL;
                        	msg.dest_id = DIVOOM_MODEL_MAIN;
							//msg.msg_type = 1;
                            divoom_sys_model_crl_msg_push(&msg);
    					}

						if(con_info->host)
						{
							divoom_free_line(con_info->host, __FILE__, __LINE__);
						}
						divoom_free_line(con_info, __FILE__, __LINE__);
    				}
    				break;
    			case MQTT_SUBSCRIBED:
    				{
						Pdivoom_mqtt_model_sub_info sub_info = (Pdivoom_mqtt_model_sub_info)task_info_handle->data;
						
						task_handle->server_status_cb(task_info_handle->status, sub_info);
						divoom_free_line(sub_info, __FILE__, __LINE__);
					}
					break;
    			case MQTT_ERROR:
    				{
    					task_handle->server_status_cb(task_info_handle->status, NULL);
    				}
    				break;
    			case MQTT_DISCONNECTED:
    				{
    					if(task_info_handle->data)
    					{
    						Pdivoom_mqtt_model_discon_struct discon_info = (Pdivoom_mqtt_model_discon_struct)task_info_handle->data;
                            if(discon_info->host)
                            {
							    DIVOOM_SERVER_MSG_MANAGE_PRINT("HOST:%s\n", discon_info->host);
                            }
                            if(discon_info->type == MQTT_SER && pdivoom_server_msg_all_handle->server_ok_flag)
                            {
                                pdivoom_server_msg_all_handle->fails_connect_server_con_times ++;
                                if(pdivoom_server_msg_all_handle->fails_connect_server_con_times > DIVOOM_MQTT_CONNECT_SERVER_FAIL_MAX_TIME)
                                {       
                                    divoom_print_err("the system will reset!\n");
                                    divoom_system_reboot();
                                }
                            }
    						if(discon_info->is_reconnect == 0)
    						{
                                divoom_sys_msg_info model_msg = {0};
                                
                                pdivoom_server_msg_all_handle->mqtt_dis_connect_handle = task_info_handle->handle;
                                pdivoom_server_msg_all_handle->mqtt_type = discon_info->type;
                                
                                model_msg.dest_id = DIVOOM_MODEL_SER_CON;
                                model_msg.msg_id = DIVOOM_SERVER_MSG_DISCONNECT_MQTT; 
                                model_msg.msg_type = 1;
                                divoom_sys_model_crl_msg_push(&model_msg);
                                //divoom_mqtt_disconnect_cb(task_info_handle->handle, discon_info->type);
    						}
    						task_handle->server_status_cb(task_info_handle->status, (char *)discon_info);

    						if(discon_info->host)
    						{
    							divoom_free_line(discon_info->host, __FILE__, __LINE__);
    						}
    						divoom_free_line(discon_info, __FILE__, __LINE__);
    					}
    				}
    				break;
    		}

    		divoom_free_line(task_info_handle, __FILE__, __LINE__);
			//每条数据休息5ms,不然会占用CPU太多时间,但是休眠的话会影响大图的传输,先不使用吧
			//vTaskDelay(10 / portTICK_PERIOD_MS);
        }
	}
	task_handle->status_bits = 0;
    return NULL;
}

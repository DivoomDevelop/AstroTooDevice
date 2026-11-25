#ifndef _DIVOOM_SERVER_MSG_MANAGE_H_
#define _DIVOOM_SERVER_MSG_MANAGE_H_

//#define		DIVOOM_SERVER_MSG_TEST

#ifdef DIVOOM_SERVER_MSG_TEST
/*************************************************test**************************************************/
void divoom_server_msg_manage_test_main(void);
void divoom_server_msg_manage_test_topic(void);
void divoom_server_msg_manage_test_data(MQTT_STATUS status, void *handle, uint8_t *topic, uint8_t *data, uint32_t data_len);
void divoom_server_msg_manage_status_test(MQTT_STATUS status, void *data);
void divoom_server_msg_manage_test_add(void);
//void divoom_server_msg_manage_test_data1(MQTT_STATUS status, void *handle, uint8_t *topic, uint8_t *data, uint32_t data_len);
//void divoom_server_msg_manage_status_test1(MQTT_STATUS status, void *data);
/*************************************************test**************************************************/
//外部调用，得到WiFi的状态,如果WiFi连接上，这个函数开始connect server
void divoom_server_msg_manage_wifi_status_cb(DIVOOM_WIFI_STATUS status, void *data);
#endif

/*typedef enum{
	DIVOOM_SERVER_MSG_ERROE = 0,
	DIVOOM_SERVER_MSG_CONNECTED,
	DIVOOM_SERVER_MSG_DISCONNECTED,
	DIVOOM_SERVER_MSG_DATA,
	DIVOOM_SERVER_MSG_SUBSCRIBED,
	DIVOOM_SERVER_MSG_UNSUBSCRIBED,
	DIVOOM_SERVER_MSG_PUBLISHED,
}DIVOOM_SERVER_MSG_STATUS;*/

enum{
	DIVOOM_SERVER_MQTT_TOPIC_NORMAL,
	DIVOOM_SERVER_MQTT_TOPIC_BROADCAST,
};

//MQTT_DISCONNECTED的数据是uint8_t*的指针
typedef void (*DIVOOM_SERVER_MSG_STATUS_CALLBACK)(MQTT_STATUS status, void *data);

typedef struct{
	uint32		port;
	char		host[64];
	int 		device_id;
	char		clinet_id[64];//name of this connection
	char		username[128];
	char		password[512];

	//表示是否连上了服务器，用于销毁.外部使用可以置空
	void		*add_ser_handle;
}divoom_server_msg_struct, *Pdivoom_server_msg_struct;

typedef struct{
	//only for MQTT_APP!
	uint8		server_type;
	char		host[64];
}divoom_server_msg_destory_struct, *Pdivoom_server_msg_destory_struct;

typedef struct{
	uint8		server_type;
	uint8		topic_type;
	char		topic[32];
}divoom_server_msg_topic_struct, *Pdivoom_server_msg_topic_struct;

typedef enum{
	DIVOOM_SERVER_MSG_ADD_SERVER,			//连接手机的server，数据结构Pdivoom_server_msg_struct
	DIVOOM_SERVER_MSG_SUB_TOPIC,			//订阅主题命令，数据结构Pdivoom_server_msg_topic_struct
	DIVOOM_SERVER_MSG_DEL_SERVER,			//主动断开
	DIVOOM_SERVER_MSG_GET_SERVER_STATUS,	//得到server连接的状态

	DIVOOM_SERVER_MSG_CONNECT_ALIYUN,		//连接阿里云,有数据传入
	DIVOOM_SERVER_MSG_CONNECT_DIVOOM, 	  	//连接divoom mqtt
	DIVOOM_SERVER_MSG_DISCONNECT_MQTT, 	  	//断开MQTT
	DIVOOM_SERVER_MSG_RESET_DIVOOM,     
}DIVOOM_SERVER_MSG_EVENT;

/*********************************************************
  * CREATED TIME	: 2018年 07月 16日 星期一 18:11:09 CST
  * FUNCTION		: divoom_server_msg_manage_init
  * IUPUT			: DIVOOM_MQTT_RECV_DATA_CB server_data_cb, DIVOOM_SERVER_MSG_STATUS_CALLBACK server_status_cb, Pdivoom_server_msg_struct server_data
  * OUTPUT			: void 
  * DESCRIPTION		: 初始化数据，需要保存起来，在WiFi连接上之后再使用
  *********************************************************/
void divoom_server_msg_manage_init(DIVOOM_MQTT_RECV_DATA_CB server_data_cb, DIVOOM_SERVER_MSG_STATUS_CALLBACK server_status_cb, Pdivoom_server_msg_struct server_data);
/*
	cb:
	1-->net_manage:divoom_net_mqtt_manage_recv_callback,收到数据调用
	2-->mqtt status: DIVOOM_SERVER_MSG_STATUS
*/

void divoom_server_msg_manage_deinit(void);

void divoom_server_msg_manage_destroy_connection(void);

void divoom_server_msg_clear_image_flag(int user_id);

#ifdef DIVOOM_TASK_PRINTF_STACK_INFO
void divoom_server_msg_manage_print_task_info();
#endif

#endif





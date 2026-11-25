#ifndef _DIVOOM_MQTT_MODEL_H_
#define _DIVOOM_MQTT_MODEL_H_

typedef enum{
	MQTT_CONNECTED = 1,		//连接上
	MQTT_SUBSCRIBED,		//订阅成功
	MQTT_DATA,				//数据收完
	MQTT_DISCONNECTED,		//连接断开,数据是uint8类型，表示SERVER_TYPE
	MQTT_ERROR,				//错误，可能是申请内存失败等等
}MQTT_STATUS;

//优先判断status, 如果是MQTT_DISCONNECTED，要注意这个句柄可能就不行了，我这边会自动注销当前句柄的所有信息，最好不要阻塞这个函数,会导致MQTT的task阻塞
//divoom_mqtt_model_add_server注册的MQTT_DATA回调中，topic/data只在回调期间有效；异步处理需自行拷贝，不要释放传入指针
typedef void (*DIVOOM_MQTT_RECV_DATA_CB)(MQTT_STATUS status, void *handle, uint8_t *topic, uint8_t *data, uint32_t data_len);

#define DIVOOM_MQTT_HOST_LEN		(64)

typedef struct{
	uint32_t data_len;
	uint8_t *data;
	//因为event上报的时候，返回的是栈地址，只使用event的topic，指向的地址会变化
	uint8_t *topic;
	bool is_on_recv_data;
	uint8_t type;	// 1服务器mqtt 0手机端mqtt
	//需要填充的数据
	char host[DIVOOM_MQTT_HOST_LEN];
	uint8_t is_reconnect;		//是否自动重连，disconnect需要使用
	DIVOOM_MQTT_RECV_DATA_CB cb;
}divoom_mqtt_user_data, *Pdivoom_mqtt_user_data;

typedef enum{
	MQTT_SER = 1,//server
	MQTT_APP,    //app
	MQTT_ALL,    //app & server
}SERVER_TYPE;

typedef struct _divoom_mqtt_model_struct{
	uint8_t is_reconnect;
    uint32_t port;
	SERVER_TYPE type;
	char *host;
    char *client_id;
    char *username;
    char *password;
	char *msg_readbuf;
	char *msg_writebuf;
	int device_id;
	char *lwt_topic;
	char *lwt_msg;
}divoom_mqtt_struct, *Pdivoom_mqtt_struct;

typedef struct _divoom_mqtt_model_discon_struct{
	uint8_t *host;
	uint8_t is_reconnect;
	SERVER_TYPE type;

	//连接不上的次数，每10000ms次数+1,在connected命令的时候可以不用关心，在disconnect可以关心这个参数
	uint8_t discon_times;
	//void *client_handle;
}divoom_mqtt_model_discon_struct, *Pdivoom_mqtt_model_discon_struct;

typedef struct _divoom_mqtt_model_sub_info
{
	SERVER_TYPE type;
}divoom_mqtt_model_sub_info, *Pdivoom_mqtt_model_sub_info;

/*********************************************************
  * CREATED TIME	: 2018年 05月 30日 星期三 17:54:56 CST
  * FUNCTION		: divoom_mqtt_model_init
  * IUPUT			: void
  * OUTPUT			: esp_err_t 
  * DESCRIPTION		: 初始化mqtt的句柄信息，这边需要存储每个add的server，需要在send的时候根据type选择
  *********************************************************/
void divoom_mqtt_model_init(void);

/*********************************************************
  * CREATED TIME	: 2018年 05月 30日 星期三 18:04:59 CST
  * FUNCTION		: divoom_mqtt_model_add_server
  * IUPUT			: Pdivoom_mqtt_struct handle, DIVOOM_MQTT_RECV_DATA_CB cb
  * OUTPUT			: esp_err_t 
  * DESCRIPTION		: 需要连接的server，生成的句柄要保存在当前文件里面
  *********************************************************/
int divoom_mqtt_model_add_server(Pdivoom_mqtt_struct handle, DIVOOM_MQTT_RECV_DATA_CB cb, SERVER_TYPE type);

void divoom_mqtt_model_remove_server_connect(void);

/*********************************************************
  * CREATED TIME	: 2018年 05月 30日 星期三 18:08:40 CST
  * FUNCTION		: divoom_mqtt_model_send_data
  * IUPUT			: SEND_TYPE type, uint8_t *topic, uint32_t data_len
  * OUTPUT			: esp_err_t 
  * DESCRIPTION		: 主动发送数据,topic是对应的设备唯一的topic
  *********************************************************/
int divoom_mqtt_model_send_data(SERVER_TYPE type, uint8_t *topic, uint8_t *data, uint32_t data_len);

/*********************************************************
  * CREATED TIME	: 2018年 05月 30日 星期三 18:12:57 CST
  * FUNCTION		: divoom_mqtt_model_respon_data
  * IUPUT			: void *handle, uint8_t *topic, uint8_t *data, uint32_t data_len
  * OUTPUT			: esp_err_t 
  * DESCRIPTION		: 回复add server的cb的信息,topic是对应的设备唯一的topic
  *********************************************************/
int divoom_mqtt_model_respon_data(void *handle, uint8_t *topic, uint8_t *data, uint32_t data_len);

/*********************************************************
  * CREATED TIME	: 2018年 05月 30日 星期三 18:23:23 CST
  * FUNCTION		: divoom_mqtt_model_sub_topic
  * IUPUT			: SERVER_TYPE type, uint8_t *topic
  * OUTPUT			: esp_err_t 
  * DESCRIPTION		: 根据类型订阅主题,发布主题需要带数据，所以不提供,mqtt_handle just for phone mqtt connection(espmqtt)
  *********************************************************/
int divoom_mqtt_model_sub_topic(SERVER_TYPE type, char *topic, void *mqtt_handle, uint8_t topic_type);


//uint8_t divoom_mqtt_disconnect_cb(void *handle);
uint8_t divoom_mqtt_disconnect_cb(void *handle, SERVER_TYPE type);

//释放所有资源,必须在断开连接之后调用
int divoom_mqtt_model_exit(void);

//仅仅适用于mqtt_app
void divoom_mqtt_model_discon_by_host(char *host);
#ifdef DIVOOM_TASK_PRINTF_STACK_INFO
void divoom_espmqtt_print_task_info(void);
void divoom_aliyun_mqtt_print_task_info(void);
#endif

//获取逻辑
//return 0-->succes
//uint8_t divoom_mqtt_model_get_devicesecret_for_kv(void *buffer, int *buffer_len);
//
//uint8_t divoom_mqtt_model_set_devicesecret_for_kv(void *buffer, int buffer_len);

#endif




#ifndef _DIVOOM_BLE_ACCEPT_H_
#define _DIVOOM_BLE_ACCEPT_H_

/* Attributes State Machine */
enum
{
    IDX_SVC,
    IDX_CHAR_A,
    IDX_CHAR_VAL_A,
    IDX_CHAR_CFG_A,

    IDX_CHAR_B,
    IDX_CHAR_VAL_B,
    //IDX_CHAR_CFG_B,

    //IDX_CHAR_C,
    //IDX_CHAR_VAL_C,

    HRS_IDX_NB,
};

enum{
	/*BLE_CHECK_IS_DIVOOM_CON		=	0x80,
	BLE_DIVOOM_SEND_SSID		=	0x81,
	BLE_DIVOOM_GET_WIFI_STATUS	=	0x82,
	BLE_DIVOOM_SET_TZ_COUNTRY	=	0x83,
	BLE_DIVOOM_EXIT_WIFI		=	0x84,*/
	SPP_SET_MIX_MUISE_MODE		=	0x6A,
    SPP_SET_MUSICAL_INSTRUMENTS_MODE = 0XBC,
	//先验明身份,发送本机的userid校验
	//如果不是第一次配网的用户,直接断开
	START_BOND_BLE_COMMAND		=	0xF0,
	EXIT_BOND_BLE_COMMAND		=	0xF1,
	WIFI_CONFIG_BLE_COMMAND		=	0xF2,
	WIFI_STATUS_BLE_COMMAND		=	0xF3,
	SERVER_TYPE_BLE_COMMAND		=	0xF4,
	APP_DISCONNECT_BLE_COMMAND  =	0xF5,

	//发送给手机用这条命令,需要有具体的command和数据
	//第一位是command,第二,三位是长度,低位在前,后面跟数据
	WIFI_SEND_DATA_BLE_COMMAND	=	0xFA,
	WIFI_SEND_DATA_BLE_DIS_CONNECT_COMMAND	=	0xFB,
};


/*
enum{
	DIVOOM_BLE_NO_DATA			= 0,	//没有收到过数据，一般不上报,连接上了，没有发送过数据
	DIVOOM_BLE_CHECK_OK			= 1,	//连接并检验成功
	DIVOOM_BLE_GET_CONFIG		= 2,	//收到ssid pswd
	DIVOOM_BLE_GET_TZ_COUNTRY	= 3,	//收到时区和地区信息
	DIVOOM_BLE_EXIT_BLE			= 4,	//收到退出BLE命令
	DIVOOM_BLE_CHECK_FAIL		= 5,	//检验错误:
	DIVOOM_BLE_CHECK_ERR,				//检验出错等等
};
*/

//typedef void (*DIVOOM_BLE_CALLBACK)(uint8_t *ssid, uint8_t *pswd, uint8_t ssid_len, uint8_t pswd_len, Pdivoom_wifi_get_tz tz, Pdivoom_wifi_get_country_info country_info, uint8_t flag);
//typedef void (*DIVOOM_BLE_CALLBACK)(uint8_t *ssid, uint8_t *pswd, uint8_t ssid_len, uint8_t pswd_len, uint32_t app_id, uint8_t cmd);

void divoom_ble_recv_data_callback(const char *uuid, char *data, int len);

/* JL→T113 DM_BTCORE_SPPBLE_CONNECT_INFO：type(0=BLE,1=SPP) + flag(1=连接,0=断开) */
void divoom_ble_on_jl_connect_info(const uint8_t *cmd_data, uint16_t cmd_len);

void *divoom_ble_start(DIVOOM_BLE_CALLBACK callback, uint32_t app_id);

//void divoom_ble_wifi_status_notify(DIVOOM_WIFI_STATUS status);
//void divoom_ble_wifi_status_notify(uint8_t command, uint32_t size, uint8_t *data);

void divoom_ble_stop(uint8_t need_release_mem);
uint8 divoom_ble_is_run(void);

//返回当前配网状态信息
int divoom_ble_get_wifi_status(void);
void divoom_ble_set_wifi_status(int wifi_status);


#endif

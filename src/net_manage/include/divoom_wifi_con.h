#ifndef _DIVOOM_WIFI_BLE_CON_H_
#define _DIVOOM_WIFI_BLE_CON_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "divoom_type.h"

typedef enum
{
	DIVOOM_WIFI_EVENT_INIT,
	DIVOOM_WIFI_EVENT_ENTER_CONFIG,
	DIVOOM_WIFI_EVENT_CONNECT_WIFI,
	DIVOOM_WIFI_EVENT_EXIT_CONFIG,
	DIVOOM_WIFI_EVENT_RECONNECT,
	DIVOOM_WIFI_EVENT_WIFI_PASSWORD_ERR,
	DIVOOM_WIFI_EVENT_RESET_BLE,
	DIVOOM_WIFI_EVENT_CONNECTED,
	DIVOOM_WIFI_EVENT_DHCP_SUCCESS,
	DIVOOM_WIFI_EVENT_STOP_CONNECT,
	DIVOOM_WIFI_EVENT_STOP_MQTT,
	DIVOOM_WIFI_EVENT_DHCP_TIMEOUT,
}DIVOOM_WIFI_EVENT_ID;

typedef enum{
	DIVOOM_WIFI_CONNECTING = 0,     //连接中
	DIVOOM_WIFI_NO_SSID    = 1,        //ssid错误
	DIVOOM_WIFI_OTHER_ERR  = 2,      //密码错误或者其他错误
	DIVOOM_WIFI_CONNECTED  = 3,      //连接成功
	DIVOOM_WIFI_NOT_CONFIG = 4,     //没有配置过WiFi
	DIVOOM_WIFI_CONNECTED_SER = 5,     //已经连上服务器
	DIVOOM_WIFI_NO_FIRST_USER = 6,		//不是第一次配网绑定的用户
	DIVOOM_WIFI_BLE_WILL_EXIT = 7,		//BLE的内存已经释放了
	DIVOOM_WIFI_DISCONNECTED,			//wifi断开,有可能是没连上报的断开,也有可能是连接上之后断开了
	DIVOOM_WIFI_RECONFIG,			//wifi重新配置
	DIVOOM_WIFI_START,     //wifi打开
	DIVOOM_WIFI_GOT_IP,    //DHCP 成功，可访问网络（内部回调，不下发 BLE）
	//...and so on
}DIVOOM_WIFI_STATUS;

enum{
	DIVOOM_BLE_NO_DATA			= 0,	//没有收到过数据，一般不上报,连接上了，没有发送过数据
	DIVOOM_BLE_CHECK_OK			= 1,	//连接并检验成功
	DIVOOM_BLE_GET_CONFIG		= 2,	//收到ssid pswd
	DIVOOM_BLE_GET_MIX_MUSIC	= 3,	//收到ssid pswd
	DIVOOM_BLE_GET_TZ_COUNTRY	= 4,	//收到时区和地区信息
	DIVOOM_BLE_EXIT_BLE			= 5,	//收到退出BLE命令
	DIVOOM_BLE_CHECK_FAIL		= 6,	//检验错误:
	DIVOOM_BLE_GET_MUSICAL_INSTRUMENTS,	
	DIVOOM_BLE_CHECK_ERR,				//检验出错等等
	DIVOOM_BLE_AP_STA_CONFIG,
};

typedef struct{
	uint8_t		ssid[128];
	//uint8_t		ip[20];
	uint32_t	ssid_len;
	uint32_t	app_id;

	uint32_t	login_ps_len;
	uint8_t		login_ps[33];
}divoom_wifi_con_struct, *Pdivoom_wifi_con_struct;

typedef struct{
	uint32_t	ssid_len;
	uint8_t		*ssid;
	uint32_t	pswd_len;
	uint8_t		*pswd;
	uint32_t	app_id;
	//登录到我们自己的服务器的密码,用户名是本机的mac地址
	uint32_t	login_ps_len;
	uint8_t		login_ps[33];
}divoom_wifi_info_struct, *Pdivoom_wifi_info_struct;

typedef struct{
	uint8_t	music_mix_mode;
	uint8_t	music_mix_button_select;
	uint8_t	music_mix_onoff;
    uint8_t app_flag;    //APP控制标志	
}divoom_wifi_ble_music_info, *Pdivoom_wifi_ble_music_info;


typedef struct _divoom_wifi_ble_musical_instruments_info
{
    uint8_t mode;
    uint8_t index;
}divoom_wifi_ble_musical_instruments_info, *Pdivoom_wifi_ble_musical_instruments_info;


typedef struct{
	int16_t time_zone;
	uint8_t summer_zone;
	char tzcode[64];
}divoom_wifi_get_tz, *Pdivoom_wifi_get_tz;

typedef struct{
	//可变长度--> name:一串字母;city id:；经纬度：
	union{
		char		*name;
		uint32_t	city_id;
		uint8_t		lon_lat[4];//0->lon经度整数部分，1->lon经度小数部分,2->lat维度整数部分,3->lat维度小数部分
	}query;
	uint8_t union_num;	//0->name,1->city_id,2->lon_lat;if union_num = 0;must free name
	bool	units;		//0->摄氏度，1->华氏度.
	bool	lang;		//0->EN,1->CN;
}divoom_wifi_get_country_info, *Pdivoom_wifi_get_country_info;

typedef struct{
	uint8_t ssid[32];
	uint8_t ssid_len;
	uint8_t pswd[64];
	uint8_t pswd_len;
}divoom_wifi_ssid_pswd, *Pdivoom_wifi_ssid_pswd;

typedef struct{
	uint8_t ssid[32];
	uint8_t pswd[64];
}divoom_wifi_menu_wifi_info, *Pdivoom_wifi_menu_wifi_info;


typedef void (*DIVOOM_WIFI_BLE_CB)(DIVOOM_WIFI_STATUS status, void *data);
typedef void (*DIVOOM_BLE_CALLBACK)(uint8_t cmd, void *data);
typedef void (*DIVOOM_BT_STATUS_CB)(int  status);   //1：连接；0：断开
typedef void (*DIVOOM_WIFI_STATUS_CB)(void *msg);

/*
*divoom_callback: 需要传递进来的callback,不管连接WiFi是否成功，都会在callback里面返回
*need_con: 是否需要从新配网，1需要，会打开蓝牙进行配网，0，不需要，只是发起连接WiFi
*/
//void divoom_wifi_model_init(DIVOOM_WIFI_BLE_CB divoom_callback);
uint8_t divoom_wifi_model_init(DIVOOM_WIFI_BLE_CB divoom_callback, uint8_t is_config_wifi);

//void divoom_wifi_model_config_wifi(void);//set --> config
/*********************************************************
  * CREATED TIME	: 2018年 05月 24日 星期四 16:49:38 CST
  * FUNCTION		: divoom_wifi_model_config
  * IUPUT			: DIVOOM_WIFI_BLE_CB divoom_callback
  * OUTPUT			: void 
  * DESCRIPTION		: 调用这个函数开始配网
  *********************************************************/
void divoom_wifi_model_config(void);

Pdivoom_wifi_menu_wifi_info divoom_wifi_menu_get_all_wifi_info(int *wifi_num);
void divoom_wifi_connect_other_ssid(void);

void divoom_wifi_model_ble_init(DIVOOM_BLE_CALLBACK callback);

//1 success,0fail
uint8_t divoom_wifi_erase_by_key(void);

//暂时性的暂停连接WiFi,仅仅在没连上WiFi的时候在一直连接WiFi的时候
void divoom_wifi_stop_connect_wifi(void);

/* 非零表示 WiFi 工作线程仍在取消旧的 join，扫描需继续等待。 */
uint8 divoom_wifi_is_stop_connect_pending(void);

//继续连接存储的老的WiFi信息
void divoom_wifi_connect_wifi_in_nvs(void);
/* Format recovery takes ownership from factory provisioning. Does not start a join. */
void divoom_wifi_allow_saved_reconnect(void);

void divoom_wifi_model_deinit(void);
#ifdef DIVOOM_SIMULATOR
void divoom_wifi_con_poll(void);
#endif
/**
 * NAME         divoom_wifi_model_get_wifi_status
 * @BRIEF	    获取WIFI连接状态
 * @PARAM	    
                
                    
 * @RETURN	1返回连接；0：未连接
 * @RETVAL	
 */

int divoom_wifi_model_get_wifi_status(void);

/* Network usable after GOT_IP; cleared immediately on link/IP loss. */
int divoom_wifi_model_is_network_ready(void);
/* Blocks service restarts until the WiFi switch has resumed services. */
int divoom_wifi_model_network_services_paused(void);

/**
 * STA 链路是否已连（读 main_loop 约 1s 刷新的 aw_wifi_is_connected 结果）。
 * @return 1 已连，0 未连或尚无有效数据
 */
int divoom_wifi_model_get_sta_link_connected(void);

/**
 * 已连 STA RSSI（读 main_loop 约 5s、且已连时才 aw_wifi_sta_get_rssi 的结果；勿在 LVGL 里直调驱动）。
 * @param rssi 输出 dBm
 * @return 0 成功；非 0 未连或尚无有效 RSSI
 */
int divoom_wifi_model_get_sta_rssi(int8 *rssi);

/** main_loop 约 1s：仅 aw_wifi_is_connected，不碰 AIC8800 RSSI 接口 */
void divoom_wifi_model_refresh_sta_link(void);

/** main_loop 约 5s：已连 STA 时才 aw_wifi_sta_get_rssi；未连直接返回 */
void divoom_wifi_model_refresh_sta_rssi(void);

void divoom_wifi_save_config(void);

/* Drain wifi.conf writes and suppress new ones before /userdata is erased.
 * End only if maintenance is cancelled before erasure; otherwise reboot. */
int divoom_wifi_userdata_format_begin(void);
void divoom_wifi_userdata_format_end(void);
/* Caller has paused userdata persistence; -1 count means snapshot failed. */
Pdivoom_wifi_menu_wifi_info divoom_wifi_userdata_format_snapshot(int *count);

//测试模式给定SSID,PSWD直接连接WIFI
int divoom_wifi_model_test_mode_config_init(char *ssid,char *pswd, DIVOOM_WIFI_STATUS_CB divoom_callback);
void divoom_wifi_reset_ble(void);

int divoom_wifi_is_ready(void);
void divoom_wifi_set_config(int onoff);
/**
 * NAME         divoom_wifi_connect_ssid
 * @BRIEF	    连接WIFI，如果是同一个多络将不进行重连，否则先查找SSID，获取加密方式，如果没有找到SSID再重试所有的加密方式进行连接
 * @PARAM	    model_id
                event_id
                    
 * @RETURN	none
 * @RETVAL	
 */

void divoom_wifi_connect_ssid(char *ssid, char *password);

/* 确保 STA 已 init+start；从未连过 WiFi 时扫描前须调用。0 成功，非 0 失败。 */
int divoom_wifi_ensure_sta_started(void);

/* 设置页只读本次手动连接结果，自动重连不会覆盖该状态。 */
int divoom_wifi_get_manual_connect_result(void);

/* 本次手动连接失败时的 AIC 错误码；0 表示尚无明确底层错误。 */
int divoom_wifi_get_manual_connect_error_reason(void);
/*********************************************************************
  Function:        divoom_wifi_clear_all_wifi
  Description:     删除所有的本地网络，并断开所有链接
  Input:           none
  Return:          none
**********************************************************************/

void divoom_wifi_clear_all_wifi(void);
//获取最后连接的SSID
int divoom_wifi_get_connect_ssid_info(char *ssid);
/*********************************************************************
  Function:        divoom_wifi_del_wifi_config
  Description:     在送礼模式时，删除WIFI配置
  Input:           none
  Return:          none
**********************************************************************/

void divoom_wifi_del_wifi_config(void);

/**
 * NAME         divoom_wifi_get_current_ssid
 * @BRIEF	    得到当前正在连接的ＷＩＦＩ
 * @PARAM	    
                    
 * @RETURN	none
 * @RETVAL	
 */

void divoom_wifi_get_current_ssid(char *ssid);

void divoom_wifi_set_wifi_time_out_flag(int status_flag);

#endif

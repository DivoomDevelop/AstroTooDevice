#ifndef _DIVOOM_APP_CONFIG_WIFI_H_
#define _DIVOOM_APP_CONFIG_WIFI_H_

enum{
	//获取APPID
	DIVOOM_APP_CONFIG_WIFI_GET_APP_ID,
	//获取链接的WiFi的ssid
	DIVOOM_APP_CONFIG_WIFI_GET_SSID,

	//主动退出BLE
	DIVOOM_APP_CONFIG_WIFI_EXIT_BLE,
	//返回mac地址,可以直接从esp_read_mac获取,如果不想从这里获取的话
	DIVOOM_APP_CONFIG_BACK_MAC_ADDR,

	//清除第一次配网时保存的userid
	DIVOOM_APP_CONFIG_WIFI_RESET_USERID,
	DIVOOM_APP_CONFIG_WIFI_WIFI_CONNECT,
	DIVOOM_APP_CONFIG_WIFI_WIFI_GET_IP,
	DIVOOM_APP_CONFIG_WIFI_WIFI_START,
	DIVOOM_APP_CONFIG_WIFI_BLE_EXIT,
};

//创建一个微任务，处理divoom_wifi_model_init的callback的返回值，避免阻塞底层处理
void divoom_app_config_wifi_init(void);

void divoom_app_config_wifi_poweron_init(void);

void divoom_app_config_wifi_deinit(void);

#ifdef DIVOOM_SIMULATOR
/* 模拟器：线程被跳过时，在 main 启动阶段单次完成 WiFi init（非每帧 poll） */
void divoom_app_config_wifi_sim_finish_init(void);
#endif

#endif






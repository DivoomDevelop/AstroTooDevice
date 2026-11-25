#ifndef _DIVOOM_POWER_ON_MANAGE_
#define _DIVOOM_POWER_ON_MANAGE_

enum{
	DIVOOM_POWER_ON_WIFI_MODE = 0,	// wifi模式
	DIVOOM_POWER_ON_WIFI_CONFIG_MODE = 1, // 蓝牙配网模式
	DIVOOM_POWER_ON_BLUE_MODE = 2,
	DIVOOM_POWER_ON_FORMAT_SD_MODE = 3,
	DIVOOM_POWER_ON_DEL_CONFIG_MODE = 4,
};

/**
 * *  NAME          divoom_power_on_manage_check_switch_mode
 * *  @brief       	用户因为配网切换蓝牙而重启
 * *  @param[in] 	none 
 * *  @author       wangft                                   
 * *  @return       1蓝牙配网 0非蓝牙配网
 * */
uint8 divoom_power_on_manage_check_switch_mode(void);

/**
 * *  NAME          divoom_power_on_manage_erase_app_id
 * *  @brief       	清空 app id
 * *  @param[in] 	none 
 * *  @author       wangft                                   
 * *  @return       1成功 0失败
 * */
uint8 divoom_power_on_manage_erase_app_id(void);

/**
 * *  NAME          divoom_nvs_get_wifi_info
 * *  @brief       	获取 ssid passed
 * *  @param[in] 	ssid  	passwd 
 * *  @author       wangft                                   
 * *  @return       1获取成功 0获取失败
 * */
uint8 divoom_power_on_manage_get_wifi_info(char *ssid, uint8 *ssid_len, char *passwd, uint8 *passwd_len);

/**
 * *  NAME          divoom_power_on_manage_save_wifi_info
 * *  @brief       	保存ssid passwd appid
 * *  @param[in] 	ssid  	passwd    appid
 * *  @author       wangft                                   
 * *  @return       1保存成功 0保存失败
 * */

uint8 divoom_power_on_manage_save_wifi_info(char *ssid, char *passwd, uint32 appid);

/**
 * *  NAME          divoom_power_on_manage_set_power_mode
 * *  @brief       	设置开机模式
 * *  @param[in] 	power_mode 开机模式	reboot_flag 是否重启
 * *  @author       wangft                                   
 * *  @return       none
 * */
void divoom_power_on_manage_set_power_mode(uint8 power_mode, uint8 reboot_flag);

/**
 * *  NAME          divoom_power_on_manage_get_power_mode
 * *  @brief       	获取开机模式
 * *  @param[in] 	none
 * *  @author       wangft                                   
 * *  @return       none
 * */
uint8 divoom_power_on_manage_get_power_mode(void);

/**
 * *  NAME          divoom_power_on_manage_init
 * *  @brief       	开机先进行判断当前开机模式
 * *  @param[in] 	none
 * *  @author       wangft                                   
 * *  @return       none
 * */
void divoom_power_on_manage_init(void);

#endif

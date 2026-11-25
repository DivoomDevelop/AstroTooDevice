#ifndef _divoom_main_h_
#define _divoom_main_h_
#include "divoom_lyric_region.h"
#define DIVOOM_MAIN_MAX_APP_CON 8


enum
{
    DIVOOM_MAIN_EVENT_RESTORE_SYS_LIGHT,
    DIVOOM_MAIN_EVENT_SET_SYS_LIGHT,
	DIVOOM_MAIN_EVENT_ADD_SYS_LIGHT,	//增加亮度
	DIVOOM_MAIN_EVENT_DEC_SYS_LIGHT, //降低亮度
    DIVOOM_MAIN_EVENT_SET_ALL_MODE,
    DIVOOM_MAIN_EVENT_POWER_ON_PIC,  //设置开机画面 
    DIVOOM_MAIN_EVENT_SET_DEVICE_LOCATION,
    DIVOOM_MAIN_NET_SAND_PAINT, 
    DIVOOM_MAIN_NET_DRAW_REMOTE,
    DIVOOM_MAIN_EVENT_UPDATE_APP_ID,
    DIVOOM_MAIN_EVENT_PUSH_APP_ID,
    DIVOOM_MAIN_EVENT_DELETE_APP_ID,		//divoom_server_msg_manage.c发出此命令,进行断开所有连接
    DIVOOM_MAIN_EVENT_START_MUSIC_MIXER,    //系统将重启，切换到蓝牙
    DIVOOM_MAIN_EVENT_END_MUSIC_MIXER,    //系统将重启，切换到WIFI  
    DIVOOM_MAIN_EVENT_MUSIC_PAUSE,   
    DIVOOM_MAIN_EVENT_MUSIC_PLAY,   
    DIVOOM_MAIN_EVENT_SYSTEM_SLEEP,		//睡眠关机,参数2
	//这条命令把appid和ssid都传过来,这样绑定和DIVOOM_MAIN_EVENT_SRV_MQTT_CON不需要在取一次了
    DIVOOM_MAIN_EVENT_WIFI_CON,
    DIVOOM_MAIN_EVENT_WIFI_DISCON,   //
    DIVOOM_MAIN_EVENT_SRV_MQTT_CON,   //
    DIVOOM_MAIN_EVENT_SRV_MQTT_DISCON,   //
	DIVOOM_MAIN_EVENT_APP_MQTT_CON,
    DIVOOM_MAIN_EVENT_BIND_USER,   //绑定手机的appid
    DIVOOM_MAIN_EVENT_KEY_POWEROFF,   //按键关机，关屏动作,参数1
    DIVOOM_MAIN_EVENT_SOFT_RESTART,   //按键重启，配网或者是模式切换
	DIVOOM_MAIN_EVENT_STOP_CON_APP,		//主动断开APP连接
	DIVOOM_MAIN_EVENT_APP_UNBIND,		//APP取消绑定，我们这边从新获取applist
	DIVOOM_MAIN_REQUEST_EXPERT_PIC,		//请求达人图片
	DIVOOM_MAIN_REQUEST_EXPERT_ADD_PIC,	//请求增量图片命令
	DIVOOM_MAIN_REQUEST_CATEGORY_PIC,		//请求分类图库图片
	DIVOOM_MAIN_EVENT_EXIT_BLE,			//收到这个命令，BLE的内存已经释放了
	DIVOOM_MAIN_EVENT_KEY_PRESS_EVENT,	//按键数据，异步发送，需要释放数据，数据类型
	DIVOOM_MAIN_EVENT_IO_DETECT_EVENT,	//detect检测，异步发送，需要释放数据，数据类型
	DIVOOM_MAIN_EVENT_PLAY_VOICE,	//播放提示音
	DIVOOM_MAIN_EVENT_BATTERY,		//电池电量信息
	DIVOOM_MAIN_EVENT_REFRESH_MICORSEMI_FLASH,		//重新刷入MICROSEMI_FLASH
	DIVOOM_MAIN_EVENT_END_PLAY_VOIC_LOWPOWER,		//播完低电提示音
	DIVOOM_MAIN_EVENT_START_PLAY_VOIC_BT_CONNECT,		//开始播放蓝牙连接提示音
	DIVOOM_MAIN_EVENT_END_PLAY_VOIC_BT_CONNECT,		//播完蓝牙连接提示音
	DIVOOM_MAIN_EVENT_END_PLAY_CHECK_POWER,		//播完低电提示音
	DIVOOM_MAIN_EVENT_SET_SLEEP, //设备休眠	
	DIVOOM_MAIN_EVENT_AI_SET_BRIGHT, //调亮度
	DIVOOM_MAIN_EVENT_AI_CHECK_POWER, //设备电量查询	
	DIVOOM_MAIN_STOP_ALL_SERVER_CON,		//断开所有的连接,包括服务器,手机的,并且释放相关资源
	DIVOOM_MAIN_SET_POWERON_MODE,
	DIVOOM_MAIN_GET_SPI_DATA,					//这个目前只有关机动作,收到之后就开始存数据,再发送OK命令给gd
	DIVOOM_MAIN_SET_SD_MOUNT_OK_DO,				//没有参数,sd挂载成功需要做的事情
	DIVOOM_MAIN_SET_SD_UNMOUNT,				    //sd拔出
	DIVOOM_MAIN_SET_SD_ERR,				    //sd出错
	DIVOOM_MAIN_EVENT_POWER_ON_OVER,            //系统完成开机显示
	DIVOOM_MAIN_EVENT_COUNT_DOWN_EVENT,         //倒计时时间提醒
	DIVOOM_MAIN_SET_LOGIN_INFO,				//userid + accesstoken
	DIVOOM_MAIN_SET_ALIYUNINFO,					//阿里云登录名名私钥公钥
	DIVOOM_MAIN_NOTICE_OVER_MESSAGE,					//提示事件完成
	DIVOOM_MAIN_GET_DEVICE_PASSWORD,					//获取用户密码
	DIVOOM_MAIN_KEY_FUNC_PLAY_OVER,					//按键复合功能结束
	DIVOOM_MAIN_EVENT_POWER_ON_PRODUCT_TEST_UPDATE,	//测试模式存开机配置
	DIVOOM_MAIN_EVENT_SAVE_CONFIG_NOTICE_MSG,	    //保存配置的提示信息
	DIVOOM_MAIN_EVENT_SET_TIMEZONE,	    //设置时区
	DIVOOM_MAIN_EVENT_SET_TIME,	//	设置时间
	DIVOOM_MAIN_EVENT_SET_CURRENT_WEATHER, //设置当前天气
 	DIVOOM_MAIN_EVENT_SET_FUTURE_WEATHER, //设置未来天气
	DIVOOM_MAIN_EVENT_LEDSEG_UPDATE_FLAG,	    //更新FLAG
	DIVOOM_MAIN_EVENT_SCHEDULE_CANCEL,
	DIVOOM_MAIN_EVENT_DOWNLOAD_ALL_DATA,        //文件下载通知
	DIVOOM_MAIN_EVENT_NEW_SD_NOTICE,            //系统是一张新的ＳＤ卡
	DIVOOM_MAIN_EVENT_NOTICE_UPDATE_DEVICE,            //系统升级的提醒
	DIVOOM_MAIN_EVENT_FORMAT_SD_OK,            //格式化ＳＤ卡完成
	DIVOOM_MAIN_NOTICE_WIFI_NOTICE_MESSAGE,	   //wifi连接消息
	DIVOOM_MAIN_NOTICE_RESET_APP_MQTT_CON,	   //重置所有的ＭＱＴＴ连接
	DIVOOM_MAIN_EVENT_SD_FREE_ROOM_NOT_ENOUGH, //ＳＤ卡空间不够
	DIVOOM_MAIN_EVENT_SET_BLUE_NAME, //设置蓝牙名称
	DIVOOM_MAIN_EVENT_GET_BLUE_NAME, //获取蓝牙名称
	DIVOOM_MAIN_EVENT_GET_DEVICE_ID, //获取设备id
	DIVOOM_MAIN_EVENT_SET_DEVICE_NIGHT_VIEW, //设置设备夜览
	DIVOOM_MAIN_EVENT_INIT_DEVICE_NIGHT_VIEW, //初始配置设备夜览
	DIVOOM_MAIN_NOTICE_UPDATE_ON_OFF_TIME,					//更新定时关机的配置
	DIVOOM_MAIN_NOTICE_INIT_ON_OFF_TIME,					//初始配置定时关机的配置
	DIVOOM_MAIN_EVENT_CONFIG_NET_WORK,            //配置网络
	DIVOOM_MAIN_EVENT_FORMAT_USERDATA,            //格式化内部储存
	DIVOOM_MAIN_EVENT_RESET_FACTORY_SETTING,            //恢复出厂设置
	DIVOOM_MAIN_EVENT_CONFIG_NET_WORK_OVER,            //配置网络成功
	DIVOOM_MAIN_EVENT_CONFIG_NET_WORK_CANCLE,            //配置网络取消
	DIVOOM_MAIN_EVENT_BT_ON_EVENT,            //蓝牙启动
	DIVOOM_MAIN_EVENT_WIFI_BT_START_EVENT,            //开启蓝牙与WIFI
	DIVOOM_MAIN_EVENT_WIFI_OTHER_ERR,
	DIVOOM_MAIN_EVENT_ENTER_MENU_SETTING,
	DIVOOM_MAIN_EVENT_EXIT_MENU_SETTING,
	DIVOOM_MAIN_EVENT_RAM_GARBAGE,
	DIVOOM_MAIN_EVENT_CLEAR_USER_PHOTO,
	DIVOOM_MAIN_EVENT_SET_RGB_INFO,
	DIVOOM_MAIN_EVENT_SET_RGB_INIT_INFO,
	DIVOOM_MAIN_EVENT_GET_RGB_INFO,
	DIVOOM_MAIN_EVENT_WIFI_NOTICE_MENU_ENTER,
	DIVOOM_MAIN_EVENT_WIFI_NOTICE_MENU_CANCLE,
	DIVOOM_MAIN_EVENT_BRIGHT_MENU_ENTER,	//顶层亮度快捷提示
	DIVOOM_MAIN_EVENT_VOLUME_MENU_ENTER,	//顶层音量快捷提示
	DIVOOM_MAIN_EVENT_POWER_OFF_DISP_MENU_ENTER,	//显示定时开关机开始页面
	DIVOOM_MAIN_EVENT_POWER_OFF_DISP_MENU_CANCLE,	//退出定时开关机开始页面
	DIVOOM_MAIN_EVENT_SET_NIGHT_VIEW_MODE,	//进入预览模式
	DIVOOM_MAIN_EVENT_EXIT_NIGHT_VIEW_MODE,	//退出预览模式
	DIVOOM_MAIN_EVENT_CREATE_MOIVE_REVIEW_PIC,	//创建预览图
	DIVOOM_MAIN_EVENT_SET_POWER_ON_MODE,	//设置开关机
	DIVOOM_MAIN_EVENT_GET_POWER_ON_LIST,	//获取开关机列表
	DIVOOM_MAIN_EVENT_SEND_DANMAKU_TEXT,	//发送弹幕
	DIVOOM_MAIN_EVENT_SEND_RANDOM_DANMAKU_FACE,	//随机表情
	DIVOOM_MAIN_EVENT_GET_DANMAKU_CONFIG,	//获取弹幕配置
	DIVOOM_MAIN_EVENT_SET_DANMAKU_CONFIG,	//设置弹幕配置
	DIVOOM_MAIN_EVENT_GET_DEBUG_FLAG,	//获取调试模式标志
	DIVOOM_MAIN_EVENT_SEND_DANMAKU_FACE,	//发送确定表情
	DIVOOM_MAIN_EVENT_RETRY_MQTT_CONNECT,	//RTOS: 在 sys_model 线程重试 InitV3/MQTT，避免 main_loop 栈溢出
    DIVOOM_MAIN_EVENT_CHECK_SYSTEM_FILES, /* Settings: restart into shared recovery. */
    DIVOOM_MAIN_EVENT_SET_BETA_FEATURES,
    DIVOOM_MAIN_EVENT_BETA_FEATURES_SAVED,
};
    
typedef enum{
    DIVOOM_MAIN_PLAY_POWER_ON_EVENT,
    DIVOOM_MAIN_PLAY_POWER_OFF_EVENT,
    DIVOOM_MAIN_PLAY_LOW_POWER_EVENT,
	DIVOOM_MAIN_PLAY_ALL_NOTICE_EVENT,
	DIVOOM_MAIN_PLAY_WIFI_NOTICE_EVENT,
	DIVOOM_MAIN_PLAY_BLUE_NOTICE_EVENT,
    DIVOOM_MAIN_PLAY_CHECK_POWER_EVENT,
	DIVOOM_MAIN_PLAY_WIFI_CONNECT_EVENT,
	DIVOOM_MAIN_PLAY_WIFI_CONFIG_EVENT,
	DIVOOM_MAIN_PLAY_KEY_FUNC_EVENT,
	DIVOOM_MAIN_PLAY_DEFAULT_TIME_EVENT,
	DIVOOM_MAIN_PLAY_IFFT_VOICE_EVENT,
	DIVOOM_MAIN_PLAY_IFFT_MESSAGE_EVENT,
	DIVOOM_MAIN_PLAY_WIFI_DISCONNECT_EVENT,
	DIVOOM_MAIN_PLAY_SD_IN_EVENT,
	DIVOOM_MAIN_PLAY_SD_OUT_EVENT,
	DIVOOM_MAIN_PLAY_SD_ERR_EVENT,
	DIVOOM_MAIN_PLAY_DOWNLOAD_OK_EVENT,
	DIVOOM_MAIN_PLAY_DEVICE_UPDATE_EVENT,
	DIVOOM_MAIN_PLAY_SD_FREE_WARNING,
	DIVOOM_MAIN_PLAY_ENTER_SYSTEM_MENU_SETTING,
	DIVOOM_MAIN_PLAY_ENTER_POWER_OFF_DISP_MENU_EVENT,
	
}T_DIVOOM_MAIN_SCHEDUEL_EVENT;

void divoom_main_tf_det_cb(int inserted);
void divoom_main_linein_det_cb(int inserted);
void divoom_main_usb_det_cb(int inserted);


uint8 divoom_main_get_audio_type_statu(uint8 types);
void divoom_main_exit(void);

/** 真关机：切专用黑屏页并等待 LVGL 切完（须在 poweroff_active 且非产测） */
void divoom_main_enter_power_shutdown_display(void);

void divoom_main(void);
void *divoom_main_loop(void *data);
uint8 divoom_main_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type);

uint8 divoom_main_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type);

uint8 divoom_main_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type);

uint8 divoom_main_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type);

//得到显示字符串
char *divoom_main_get_disp_title(uint8 type);

void divoom_main_keypad_send_key_info(uint8 key_value, uint16 press_type);

uint8 divoom_main_set_device_position(char *lot, char *lat);

void divoom_main_get_device_position(char *lot, char *lat);
void divoom_main_get_weather_info(Pdivoom_disp_weather_info weather_info);
Pdivoom_channel_item_info  divoom_main_get_channel_info(int app_id);
uint8 divoom_main_get_sys_light(void);
/* 用户系统亮度 → PWM：0 映射为 DIVOOM_SYS_BRIGHT_ON_FLOOR。熄屏用 divoom_screen_bright(0) */
uint8 divoom_main_get_sys_light_hw(void);
void divoom_main_apply_sys_light(void);
void divoom_main_set_sys_light(uint8 value);
/* 助眠当前屏亮，字面写 PWM（0=灭），不改系统配置 */
void divoom_main_set_sleep_brightness(uint8 value);
/* 覆盖中返回助眠 PWM；否则返回系统配置档 */
uint8 divoom_main_get_sleep_brightness(void);
/* 夜览覆盖屏亮，不改系统配置；用户 0 映射为 DIVOOM_SYS_BRIGHT_ON_FLOOR */
void divoom_main_set_night_brightness(uint8 value);
void divoom_main_clear_night_brightness(void);
void divoom_main_upload_sys_brightness(uint8 value);
void divoom_main_get_key_light(uint8 *mode, uint8 *on_off, uint8 *brightness);
int divoom_main_set_key_light(uint8 mode, uint8 on_off, uint8 brightness);
void divoom_main_key_light_cycle_next(void);
uint8 divoom_main_get_protest_flag(void);
uint8 divoom_main_get_time_flag(void);
uint8 divoom_main_get_sys_volume(void);
uint8 divoom_main_get_time_format_type(void);
void divoom_main_set_time_format_type(uint8 type);
uint8 divoom_main_get_wind_speed_flag(void);
void divoom_main_set_wind_speed_flag(uint8 flag);
void divoom_main_set_time_flag(int mode);
Pdivoom_screen_color_info divoom_main_get_time_ptr(void);
uint32 divoom_main_manage_get_run_channel_id(void);
void divoom_main_out_disp_str(char *data,uint8 cyc_flag,void *callback);
void divoom_main_set_text_dir(void);
void divoom_main_out_disp_str_remove_title(char *data,uint8 cyc_flag,void *callback);
void divoom_main_change_sys_volumn(uint8 dir);
void divoom_main_save_notice_dis(void);

uint32 divoom_main_onoff_get_next_active_alarm_time(void);
void divoom_main_set_timezone(Pdivoom_wifi_get_tz ptz);
int divoom_main_get_temp_flag(void);
int divoom_main_get_battery_level(void);
int divoom_main_get_show_grid_flag(void);
void divoom_main_set_show_grid_flag(int flag);
void divoom_main_set_temp_flag(int mode);
void divoom_main_set_screen_dir(int mode);
void divoom_main_set_mirror_flag(int mode);
void divoom_main_set_white_banlance(divoom_screen_color_info *para);
void divoom_main_set_high_light_mode(int mode);
void divoom_main_play_buzzer(Pdivoom_devcice_play_buzzer_respone para);
void divoom_main_play_voice_tip(int event, void *param);

//通知系统必要数据已经下载完成，可以启动显示了
int divoom_main_download_data_notice(int no_cache_num, int cache_num);
//SD卡空间不够提醒
void divoom_main_sd_free_room_not_enough(void);
void divoom_main_update_storage_status(uint32 free_space);

int divoom_main_update_log_level(int log_level);
void divoom_main_new_sd_notice(void);
uint8 divoom_main_get_new_sd_status(void);

//APP心跳包
void divoom_main_manage_accept_app_hearbeat(int user_id);
/*
  Function:       divoom_main_disp_wifi_config
  Description:	   WIFI 配网过程中
  Input:          time
  Return:         NONE
*/

T_VOID divoom_main_disp_wifi_config(int request_flag, int time);
T_VOID divoom_main_disp_wifi_connecting(int request_flag, int time);
void divoom_main_disp_wifi_dis_time_out(void);
//清除服务器心跳包的时间，只要收到命令就认为有心跳

void divoom_main_reset_check_mqtt_server_heart(void);

void divoom_main_format_udisk_partition(void);
void divoom_main_format_divoom_config_partition(void);
/* 卸载 /userdata → f_mkfs(/dev/UDISK) → 再挂载。不 seed、不 reboot。成功 0，失败 -1。 */
int divoom_main_mkfs_udisk_partition(void);
//删除配置，重新配网
void divoom_main_format_device(uint8 sd_format);

/*
  Function:       divoom_main_disp_device_reset
  Description:	   设备reset时的图案
  Input:          time
  Return:         NONE
*/

void divoom_main_disp_device_reset(void);
//APP心跳包
void divoom_main_manage_send_connect_app(int user_id);
T_VOID divoom_main_disp_sd_err_type(T_U8 flag);
char *divoom_main_get_blue_name(void);

void divoom_main_disp_blue_connect_ok(void);
void divoom_main_disp_blue_dis_connect(void);
Pdivoom_device_china_day_info divoom_main_get_china_data_info(void);
void divoom_main_get_all_config(char *buf);

uint8 divoom_main_get_screen_switch(void);

int8 divoom_main_get_wifi_rssi(void);

void divoom_main_set_screen_switch(int mode);
T_VOID divoom_main_start_format_sd(T_VOID);


/**
 * NAME         divoom_main_get_server_type
 * @BRIEF	返回服务器类型
 * @PARAM	
                    
 * @RETURN	1:国外服务器，0：国内服务器
 * @RETVAL	
 */

uint8 divoom_main_get_server_type(void);
uint8 divoom_main_net_radio_available(void);
void divoom_main_set_beta_features(uint8 enabled);

/**
 * NAME         divoom_main_update_server_type
 * @BRIEF	更新服务器的地址
 * @PARAM	cur_tm
                    
 * @RETURN	1:国外，0：国内
 * @RETVAL	
 */

void divoom_main_update_server_type(int type);

/**
 * NAME         divoom_main_get_file_server_addr
 * @BRIEF	返回服务器文件地址
 * @PARAM	
                    
 * @RETURN	1:国外服务器，0：国内服务器
 * @RETVAL	
 */

char *divoom_main_get_file_server_addr(void);
/**
 * NAME         divoom_main_get_file_server_addr
 * @BRIEF	返回服务器文件地址
 * @PARAM	
                    
 * @RETURN	1:国外服务器，0：国内服务器
 * @RETVAL	
 */

char *divoom_main_get_file_other_server_addr(void);

/**
 * NAME         divoom_main_get_http_server_addr
 * @BRIEF	返回服务器命令地址
 * @PARAM	
                    
 * @RETURN	1:国外服务器，0：国内服务器
 * @RETVAL	
 */

char *divoom_main_get_http_server_addr(void);
/**
 * NAME         divoom_main_get_ftp_server_ip
 * @BRIEF	返回服务器FTP地址
 * @PARAM	
                    
 * @RETURN	
 * @RETVAL	
 */

char *divoom_main_get_ftp_server_ip(void);
/**
 * NAME         divoom_main_get_https_server_ip
 * @BRIEF	返回服务器https地址
 * @PARAM	
                    
 * @RETURN	
 * @RETVAL	
 */

char *divoom_main_get_https_server_ip(void);

void divoom_main_send_lwt_msg(int flag);
Pdivoom_sys_tidal_moment_info divoom_main_get_tidal_moment_data(void);
//取消定时开关机
void divoom_main_cancel_on_off(void);
void* divoom_main_get_power_on_off_config(void);
int divoom_main_copy_power_on_off_config(void *config);
void* divoom_main_model_get_power_on_item(void);
void* divoom_main_model_get_danmaku_config(void);

uint8 divoom_main_set_on_off_work_status(uint8 state);
uint8 divoom_main_set_night_view_work_status(uint8 state);
int divoom_main_get_dial_demo_flag(void);

int divoom_main_get_sunrise_time(void);
int divoom_main_get_sunset_time(void);

float divoom_main_get_wind_speed(void);
int divoom_main_get_today_visibility(void);
uint8 divoom_main_get_mic_switch(void);

/**
 * NAME         divoom_main_get_device_mac_addr
 * @BRIEF	    获取MAC 地址
 * @PARAM	    mac_addr 返回六个字节
                
                    
 * @RETURN	1：成功获取到，0：没有获取到
 * @RETVAL	
 */

int divoom_main_get_device_mac_addr(uint8 *mac_addr);

//得到当前播放的歌名
char *divoom_main_get_music_name(void);

//得到当前播放的歌手
char *divoom_main_get_music_singer(void);

//得到当前播放的歌词
char *divoom_main_get_music_lyric(void);

//得到上一句歌词
char *divoom_main_get_music_prev_lyric(void);

//得到当前播放的状态
int divoom_main_get_play_music_status(void);
Pdivoom_sys_setting_info divoom_main_get_system_config(void);
void divoom_main_save_config(void);
Pdivoom_system_setting_json_night_view divoom_main_get_night_view_info(void);
void divoom_main_send_sys_settings_config(void);

/**
 * NAME         divoom_main_push_model_event_id
 * @BRIEF	    向某个模块抛异步事件
 * @PARAM	    model_id
                event_id
                    
 * @RETURN	none
 * @RETVAL	
 */

void divoom_main_push_model_event_id(int model_id, uint8 event_id);

#ifdef DIVOOM_TIME_FRAME_SPEAKER
/* CLOCK 黑屏脱困：切系统表盘等；由 main 按键消息路径或 PMIC 调用；LVGL 页内请 keypad_send 抛键 */
int divoom_main_try_dismiss_clock_black_screen(void);
#endif

//蓝牙连接状态，1：连接；0：等待连接
uint8 divoom_main_get_blue_con_status(void);
//有连接过蓝牙信息为1，没有被连接过为0
uint8 divoom_main_get_blue_device_num(void);

//蓝牙连接，1：连接；0：断开连接
void divoom_main_ctrl_blue_connect(uint8 con_flag);

//清除所有蓝牙配对信息
void divoom_main_unpair_blue_device(void);
//设置配置网络状态
int divoom_main_set_wifi_config_status(int mode);
int divoom_main_set_lang_flag(int lang);
//设备是否在自升级状态
int divoom_main_get_device_self_update_flag(void);

//获取设备最新版本
int divoom_main_get_device_last_version(void);
void divoom_main_get_device_last_version_update_info(char *save_update_info);

//更新升级相关信息
void *divoom_main_set_device_update_info(void *pupdate_info);
//取得设备最新版本
void divoom_main_check_deivce_update_version(void);
//用户启动设备升级
int divoom_main_user_start_update_device(void);

//设置或获取系统音量
int divoom_main_set_sys_vol(int vol);
/* 杰里 MCU 侧音量（DM_BTCORE_TONE_VOLUME）；真机实现，勿再套 SPEAKER/VIBE 产品宏 */
void divoom_main_set_jl_mcu_tone_volume_tag(int vol_0_100, const char *caller);
#define divoom_main_set_jl_mcu_tone_volume(vol) divoom_main_set_jl_mcu_tone_volume_tag((vol), __func__)
void divoom_main_ramp_jl_mcu_tone_volume(int vol_0_100);
void divoom_main_init_sys_vol(void);  /* 初始化/恢复声卡音量与通路，反复打开应用或切到 TF 时调用可避免无声音 */
void divoom_main_set_audio_volume(uint8 value);

//获取是否已配网标志
int divoom_main_get_wifi_config_flag(void);
/* 标记本次 MQTT 登录来自用户主动添加网络；成功路由后自动清除。 */
void divoom_main_set_wifi_add_network_route_pending(uint8 pending);
/* 先显示缓存表盘，再异步发送开机表盘事件刷新资源。 */
void divoom_main_return_startup_dial(void);
#ifdef DIVOOM_SIMULATOR
/* 页面验收复用真实联网成功分流，不复制一套测试判断。 */
void divoom_main_sim_route_after_add_network_success(uint32 host_id);
void divoom_main_sim_reset_startup_dial_request_count(void);
uint32 divoom_main_sim_get_startup_dial_request_count(void);
#endif

int exec_command_system(const char *cmd);
int divoom_main_set_tf_music_mode(int mode);
int divoom_main_set_tf_music_bg(int id);
int divoom_main_set_tf_music_id(int id);
int divoom_main_set_tf_music_order(int mode);

//设置全局的图像句柄，为了方便显示

void divoom_main_set_global_image_handle(int model_id, int image_id, int voice_flag);

//获取全局的图像句柄，为了方便显示

void divoom_main_get_global_image_handle(int *model_id, int *image_id, int *voice_flag);
void divoom_main_get_http_weather_info(void * weather_info);

//获取与更新系统文件更新时间
int divoom_main_set_system_file_update_time(int system_update_time);
//是否是系统异常退出的
int divoom_main_is_app_exit_flag(void);
int divoom_main_update_china_data_update_time(void);

//检查系统可用内存是否小于33M
int divoom_main_check_ram_garbage(void);
//系统音量
int divoom_main_get_tone_volume(void);
int divoom_main_get_poweron_mode(void);
void divoom_main_finish_clock_wake_poweron(void);
//系统音量
void divoom_main_set_tone_volume(int volume);
void divoom_main_set_color_temp(int value);

//设置是否忽略WIFI断开的信息
int divoom_main_set_wifi_disconnect_notice_flag(int flag);

//获取设备主人
uint32 divoom_main_get_host_id(void);
//设置设备主人
void divoom_main_set_host_id(int host_id);

//设备分享码由首次联网流程异步请求，设置页只读取缓存
void divoom_main_request_device_share_code(void);
void divoom_main_update_device_share_code(uint32 share_code, uint32 expire_time);
int divoom_main_get_device_share_code(uint32 *share_code, uint32 *expire_time);

//更新
void divoom_main_update_last_download_file(void);
//获取IP地址与MAC地址

int divoom_main_get_device_mac_addr_ifconfig(uint8 *mac_addr, uint8 *ip4_addr);
void divoom_set_mirror_mode(int mirror_flag);
/*********************************************************************
  Function:        divoom_main_update_test_file
  Description:     上传测试文件
                    
  Para:             para
  Return:           
**********************************************************************/

void divoom_main_update_test_file(void *para);

/*********************************************************************
  Function:        divoom_main_update_dead_lock_file
  Description:     上传死锁文件
                    
  Para:             para
  Return:           
**********************************************************************/

void divoom_main_update_dead_lock_file(char *lock_info);

//设备切换到GIF模式
int divoom_main_change_to_gif_mode(int flag);
//设备设置升级标记
int divoom_main_set_update_fireware_flag(int flag);

//得到临时文件的ID,这个从系统开始自动增加的
int divoom_main_get_tmp_file_index(void);

//客户产品代码
int divoom_main_get_custom_product_flag(void);

//得到一个可以用的用户级的表盘
int divoom_main_get_new_custon_clock_id(void);

//得到一个保存用户表盘列表
void divoom_main_save_custom_clock_list(void *para );
int divoom_config_set_china_config(int flag);
void divoom_main_remove_custom_clcok(int *clock_id);


//返回系统是否完成启动

int divoom_main_get_finish_start_up(void);

/** pDivoomHandle 已分配且开机动画结束；BtCore 0x19 须在此之后处理 */
int divoom_main_is_btcore_poweroff_ready(void);

void divoom_main_print_ram(char *tag, int line);

//进入测试模式删除配置文件
void divoom_main_del_config(void);

/* 网络歌词策略直接跟随服务器类型：国内服关闭，国外服开启。 */
int divoom_main_get_ip_region(void);
int divoom_main_net_lyric_region_off(void);
int divoom_main_get_boot_ab_recovery_needed(void);


void divoom_main_check_jl_update_version(void);
#endif

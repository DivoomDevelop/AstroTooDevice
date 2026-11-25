#ifndef _divoom_sys_data_h_
#define _divoom_sys_data_h_
#include "divoom_device_json.h"
#include "divoom_system_tool_json.h"
#include "divoom_power_control.h"

#define BT_NAME_MAX_LEN 32

/******************************************************************
钟控数据结构
******************************************************************/
typedef struct _divoom_manange_device_self_info
{
    int  time_zone;          //时区
    int  summer_zone;          //时区
    int  kernel_version;
    int  divoom_version;
    int  blue_version;
    int  hard_version;
    int  srv_reg_id;
	int device_id;
  //  char city_name[128];		//城市名称
}divoom_manange_device_self_info, *Pdivoom_manange_device_self_info;

typedef struct _divoom_sys_setting_info
{    
    uint8 reset_flag;
    uint8 wifi_config_flag;     //0:未绑定用户的配网；1：已经连接过用户
    uint8 server_flag;
    int lang;
    uint8 log_level;
    uint8 mic_swtich;
	uint8 user_lock;		//开机自定义更新的锁：1：定制产品不可更新，0：正常产品
	uint8 brightness;
	uint8 theme_flag;		//主题外观 0:经典简约 1:Sci-Fi UI
	uint8 audio_volume;		//音效音量
	uint8 audio_effect; 	//音效
	uint8 volume_switch;	//按键音效开关
	uint8 volume;           //提示音音量
	uint8 volume_memory;    //开关机记忆音量
	uint8 music_volume_bt;      //蓝牙模式音量 0-100
	uint8 music_volume_local;   //TF卡/本地模式音量 0-100
	uint8 music_volume_linein; //Line-In模式音量 0-100
	uint8 music_volume_uac;    //UAC模式音量 0-100
	uint8 music_volume_netfm;  //网络电台模式音量 0-100
	uint8 auto_update;      //自动更新
	uint8 bt_secret;
	uint8 bt_switch;		//蓝牙开关
	uint8 bt_auto_connect;      //蓝牙自动连接
	uint8 bt_connect_status;	//蓝牙连接状态
	int  bt_secret_switch;      //蓝牙密码开关
	char  bt_password[5];		//蓝牙密码（4位数字 + '\0'）
	uint8 mirror_flag;      //镜像标志
	uint8 screen_switch;    //屏开关
    uint8 temp_c_f_flag;    //0:摄氏;1:华氏
    uint8 wind_speed_flag;	//风速单位   0:m/s 1:km/h 2:mph 3:kt
    uint8 time_format;      //日期格式
    uint8 hour12_24;    //0:12;1:24
    uint8 screen_dir;    //屏的角度
    uint8 show_time_flag;   //动画是否显示时间
    uint8 screen_save_mode; //屏保模式
    uint8 show_grid_flag; 	//显示像素作品网格
	uint8 dial_demo_flag;	//表盘展示模式标志      1:打开表盘展示模式      0:关闭
#if DIVOOM_SHOW_MODE
    uint8 show_store_sched_flag; /* 展机已写入默认营业定时开关机，避免每次 GOT_IP 覆盖门店改过的时间 */
#endif
    uint8 tf_music_mode;    //TF卡播放模式
    uint8 tf_music_bg;      //TF卡播放歌曲背景ID
    uint8 tf_music_order;    //TF卡播放的播放顺序，1：播放收藏列表；0：全部
    int   tf_music_id;      //TF卡播放歌曲ID
    int   idle_poweroff_time;   //空闲关机时间
	int	  color_temp;		//色温
	uint8 key_light_mode;		/* 按键 RGB 灯效 0~7 */
	uint8 key_light_on_off;		/* 按键 RGB 开关 */
	uint8 key_light_brightness;	/* 按键 RGB 亮度 0~100 */
    char  power_on_id[64];
	char tzcode[64];		//时区代号
    char lat[11];
    char lot[11];
    uint8 beta_features;          // 海外 Beta 内测开关，默认关闭
    uint8 beta_test_flag_pending;  // 最新选择等待服务器确认，重连时重新发送
}divoom_sys_setting_info, *Pdivoom_sys_setting_info;

typedef struct _divoom_weather_setting_info
{    
    int hourly;
    int specific;
    int time;
}divoom_weather_setting_info, *Pdivoom_weather_setting_info;

typedef struct _divoom_danmaku_config_deal_settings
{
	int disp_area_flag;
	int speed;
} divoom_danmaku_config_deal_settings, *Pdivoom_danmaku_config_deal_settings;


typedef struct _divoom_tool_setting_info
{    
    int score_active;          //激活方
    int blue_score;           //蓝方分数
    int red_score;            //红方分数
    uint32 start_watch_time; //计时器启动时间
    uint32 pause_watch_time; //计时器暂停时间
    int tool_flag;
    int stop_watch_status;  //计时器状态
    int count_down_start;   //倒计时开始时间
    int count_down_pause;   //倒计时暂停时间
    int count_down_time;    //倒计是时间
    int count_down_status;  //倒计时是否启动 0:未启动;1:在运行;2:最后15提醒阶段;3:时间已到
    int count_down_flag;  //倒计时是否启动 0:启动;1:reset;2:stop
}divoom_tool_setting_info, *Pdivoom_tool_setting_info;

typedef struct _divoom_disp_day_weather_info
{
    int utc_time;
    int weather_pic_id;
    float min_temp;
    float max_temp;
}divoom_disp_day_weather_info, *Pdivoom_disp_day_weather_info;

typedef struct _divoom_disp_hour_weather_info
{
    //int utc_time;
    int weather_pic_id;
    float temp;
}divoom_disp_hour_weather_info, *Pdivoom_disp_hour_weather_info;

enum{
    DIVOOM_DISP_WEATHER_TODAY_INFO,
    DIVOOM_DISP_WEATHER_FURTHER_INFO,
};
typedef struct _divoom_disp_weather_info
{
    uint8       request_weather_flag;       //是否请求完成s
    T_U16       weather_pic_id;
    int         pressure;      //Atmospheric pressure (on the sea level, if there is no sea_level or grnd_level data), hPa
    int         humidity;      //Humidity, %
    int         visibility;    //Visibility, meter
    int         sunrise_time;
    int         sunset_time;
    double      wind_speed;  //Wind speed. Unit Default: meter/sec, Metric: meter/sec, Imperial: miles/hour.
    double      temp;        // Temperature. Unit Default: Kelvin, Metric: Celsius, Imperial: Fahrenheit.
    double      min_temp;        // Temperature. Unit Default: Kelvin, Metric: Celsius, Imperial: Fahrenheit.
    double      max_temp;        // Temperature. Unit Default: Kelvin, Metric: Celsius, Imperial: Fahrenheit.
    uint32      update_real_time;
    uint32      update_future_time;
    char        weather_str[32];
    uint32      request_time;
	divoom_disp_hour_weather_info hour_table[48];
    divoom_disp_day_weather_info futrue_table[40];
}divoom_disp_weather_info, *Pdivoom_disp_weather_info;

typedef struct _divoom_blue_info
{
    int auto_connect_flag;
    int spp_switch;
    char spp_secret[6];
	char blue_name[BT_NAME_MAX_LEN];
}divoom_blue_info, *Pdivoom_blue_info;

typedef struct _divoom_sys_tidal_moment_info
{	
    uint8   tidal_max_flag[64];
    int     tidal_comment_table[64];
}divoom_sys_tidal_moment_info, *Pdivoom_sys_tidal_moment_info;

typedef struct _divoom_audio_det_info
{
	uint8 uac_statu; //0未插入 1 已插入
	uint8 linein_statu; //0未插入 1 已插入
	uint8 tf_statu; //0未插入 1 已插入
	uint8 fast_chg_statu; //0快充 1未快(PF5/USB12V-DET)
}divoom_audio_det_info, *Pdivoom_audio_det_info;

typedef struct _divoom_sys_ctrl_handle
{	
    int     device_china_flag;      //是否支持中文
    int     local_clock_load_flag;
    int     sys_vol;
	int     custom_product_flag;
    int     tmp_file_index;
    void    *tmp_file_lock;
    uint8   new_server_mode;
    uint8   gif_mode;
    uint8   update_firmware_flag;   //是否升级
    uint8   use_https_flag;
    uint8   use_ip_flag;        //使用IP地址还是使用域名
    uint8   init_time_flag;     //时间是否初始化
    uint8   downloading_num;    //正在下载文件的个数
    uint8   total_num;          //总文件的个数
    uint8     china_region;
    int     ip_region;
    uint8   net_lyric_region_off;   //1:大陆网络歌词关闭，0:其他已识别地区；0xFF:未知
    int   back_lang;
    int     mqtt_con_time;
    int     last_download_file_time;    //最后下载文件的时间
    int     wifi_config_reset_all_flag; //重新配网后的清除配置标记
    uint8   bond_flag;
    uint8   pass_wifi_discon_notice;    //是否跳过ＷＩＦＩ超时断开的提醒
    uint8   reset_flag;
    uint8   sys_finish_init_ok; //系统是否完成初始化
    uint8   first_request_flag; //系统第一次请求
    uint8   device_share_code_valid; //设备分享码已从服务器获取
    uint32  device_share_code;
    uint32  device_share_code_expire_time; //服务器返回的 UTC 到期时间
    uint8   new_sd_flag;        //是否是一张新的ＳＤ卡
	uint8   storage_status_valid;    //是否已经统计过存储状态
	uint8   storage_status_reported; //当前存储状态是否已经上报
	uint8   storage_full_flag;       //当前 /userdata 空间不足标志
	uint8   blue_mac_addr[6];
	uint8   phone_mac_addr[6];
    uint8   back_light_off;
    uint8   key_func;
    uint32  key_op_time;
    uint32  check_server_heart_time;
    int     read_weather_time;
    int     wifi_err_status;
	char	ssid[128];
	//放在这里保存,避免绑定的时候再去取一次
	uint32	host_id;
	uint32	app_id;
    char    net_ip[32];
    uint8   charge_status;
    uint8   play_status;
    uint8   request_update_flag;
    uint32  request_update_time;
    uint32  connect_serve_time;
    int     sunrise_second;
    int     sunset_second;
	uint8 	master_flag; 	//用来标记主机
    time_t  system_sleeep_time;
    divoom_sys_tidal_moment_info tidal_mement_info;
    divoom_sys_setting_info sys_setting;
    uint32 beta_test_flag_packet; // MAIN 当前等待确认的 Beta 上报包号，0 表示无请求
    /* 助眠临时背光：不改 sys_setting.brightness；0 字面灭 */
    uint8 sleep_bright_override;
    uint8 sleep_bright;
    /* 夜览临时背光：不改系统亮度；用户 0 → DIVOOM_SYS_BRIGHT_ON_FLOOR */
    uint8 night_bright_override;
    uint8 night_bright;
    divoom_system_setting_json_night_view night_view;
    divoom_device_on_off_config_para      on_off_config;
	divoom_power_control_item power_on_info[DIVOOM_POWER_CTRL_NUM];
    Pdivoom_device_app_id_list app_list;
	void    *app_list_lock;
	divoom_manange_device_self_info 		device_info;				//钟控本身的属性
	int   china_day_info_update_time;
	divoom_device_china_day_info  china_day_info;
	divoom_disp_weather_info weather_info;
    void *pupdate_info;
    void *pjl_update_info;
    int pglobal_image_model_id;
    int pglobal_image_image_id;
    int pglobal_image_voice_flag;
	bool	device_wifi_status;											//连接WiFi的情况,1-->连上了,0-->没连上
	bool    divoom_connect_srv;
    bool    divoom_first_connect;
	bool    divoom_srv_respone;
	bool    divoom_wifi_dis_connect_flag;
    char    server_type[36];
	//http://-->7 addr-->, \0-->1, reserve-->1
	char	ip_host[24];

	//需要保存的标志
	bool	update_flag;
	//开关机标志,一般上点之后置1,为开机状态
	//长按电源键变为关机状态,要销毁很多资源,比如WiFi,ble,和其他模块等等
	uint8	power_onoff_flag[2];
    int     system_file_update_time;
	divoom_blue_info blue_info;
	uint8 ledseg_update_flag;
	uint8 http_request_ok;	//1表示ok 0表示非ok
	uint8 http_request_frist; 
	uint8 poweron_mode;
    int   send_server_heart_time;
    int   test_light;
    int   system_err_log;
	int8 rssi;
    void *key_queue;
    void *pnight_view_time_handle;
	uint8 product_test_flag;
	divoom_danmaku_config_deal_settings danmaku_config;
	divoom_audio_det_info audio_det_info;
}divoom_sys_ctrl_handle, *Pdivoom_sys_ctrl_handle;

typedef struct _divoom_sys_play_voice_info
{
    int id;
    int vol;
    int cyc;
}divoom_sys_play_voice_info, *Pdivoom_sys_play_voice_info;

typedef enum{
	DIVOOM_HTTP,
	DIVOOM_HTTPS,
	//like ftp,and so on..
}DIVOOM_PROTOCOL;

int divoom_main_get_time_zone(void);
uint8 *divoom_main_get_phone_blue_addr(void);
int divoom_main_get_device_id(void);
void divoom_main_set_mast_flag(int flag);


//void divoom_main_request_weather(void);
void divoom_main_disp_vol_info(uint8 mode, uint8 dir, uint8 vol);
//void divoom_main_set_tencent_device_id(char *id);

void  divoom_main_get_tool_timer_info(Pdivoom_system_tool_json_timer_info timer_info);

void  divoom_main_manage_app_con(Pdivoom_device_app_id_list app_list);
void divoom_main_mqtt_del_discon_info(char *discon_ip);
void  divoom_main_mqtt_notice(char *con_ip, uint8 flag);

int divoom_main_get_wifi_con_status(void);

int divoom_main_get_server_con_status(void);

#endif

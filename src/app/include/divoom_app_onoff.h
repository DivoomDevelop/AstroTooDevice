#if 0
#ifndef _DIVOOM_APP_ONOFF_H_
#define _DIVOOM_APP_ONOFF_H_

//开发的时候默认关闭这个宏，打开为添加测试模式
//#define		DIVOOM_ONOFF_TEST_MODE
//不再需要开机控制
//后续新板出来就把这一块相关的东西全删掉
//#define		DIVOOM_ONOFF_NO_POWER_CONTROL
//信息瀑使用adc读取按键,原来的gpio案件的逻辑还保留,谁知道那一天就又换回去了呢

#define		DIVOOM_APP_ONOFF_OFFSET			(50)

typedef enum{
	//按键开机
	DIVOOM_POWER_ON_BY_KEY,
	//不是按键开机
	DIVOOM_POWER_ON_OTHER,
}DIVOOM_POWER_ON_STATUS;

enum{
	//默认进入WiFi模式吧
	DIVOOM_APP_ONOFF_WIFI_MODE = 0,
	DIVOOM_APP_ONOFF_WIFI_CONFIG_MODE = 1,
	//DIVOOM_APP_ONOFF_BT_MODE = 2,
};

//使用user key的数据结构
/*typedef enum{
	//短按
	DIVOOM_KEY_SHORT_PRESS,
	//长按
	DIVOOM_KEY_LONG_PRESS,
	//一直按
	DIVOOM_KEY_PRESSING,
	DIVOOM_KEY_LONG_LONG_PRESS,
	//按键松开
	DIVOOM_KEY_UP,
}DIVOOM_KEY_PRESS_STATE;*/

typedef enum{
	DIVOOM_IO_UNACTIVE,
	DIVOOM_IO_ACTIVE,
}DIVOOM_IO_INOUT_STATE;

typedef enum{
	//按键类型，数据结构Pdivoom_app_onoff_key_gpio_info
	DIVOOM_KEY_IO_TYPE_KEY,
	//adc按键类型,数据结构 
	DIVOOM_KEY_ADC_TYPE_KEY,
	//detect类型，数据结构Pdivoom_app_onoff_detect_gpio_info
	DIVOOM_KEY_IO_TYPE_IO,
}DIVOOM_KEY_IO_TYPE;

enum{
	DIVOOM_APP_ONOFF_SET_KEY_INFO,
	DIVOOM_APP_ONOFF_DEL_KEY_INFO,
	DIVOOM_APP_ONOFF_SET_IO_INFO,
	DIVOOM_APP_ONOFF_DEL_IO_INFO,

	DIVOOM_APP_ONOFF_CHANGE_MODE,
	DIVOOM_APP_ONOFF_CONFIG_WIFI_MODE,

	DIVOOM_APP_ONOFF_SOFT_POWEROFF,
};

//data, key_press_state,触发之后的状态是什么触发
//typedef void (*DIVOOM_APP_KEY_ACTIVE_CALLBACK)(void *data, DIVOOM_KEY_PRESS_STATE key_press_state);
//typedef void (*DIVOOM_APP_DEVICE_INOUT_CALLBACK)(void *data, DIVOOM_IO_INIUT_STATE io_state);

/*typedef enum{
	//开关机
	DIVOOM_KEY_POWER_SWITCH,
	//mode键和模式切换
	DIVOOM_KEY_MODE_CONFIG_KEY,
	DIVOOM_KEY_TEST_MODE,
}DIVOOM_KEY_TYPE;*/

typedef enum{
	DIVOOM_USB_DETECT,
	DIVOOM_SD_DETECT,
	DIVOOM_POWER_DETECT,
	//监测是否需要清除当前配对信息
	DIVOOM_RESET_DETECT,
}DIVOOM_IO_TYPE;

//send info
/*typedef struct{
	DIVOOM_KEY_TYPE key_value;
	//当前的按键类型，只在发送的时候填充这个数据，注册key进来不需要填充
	DIVOOM_KEY_PRESS_STATE press_type;
}divoom_app_onoff_key_info, *Pdivoom_app_onoff_key_info;*/

//目前最多支持两个按键
//在使用的时候如果只有一个按键，一定要放到key_info[0]里面，一定要从前往后排
//其他的都要置0xFF，一定一定要置0xFF
typedef struct{
	uint8_t key_num;
	uint8_t key_info[2];
	//接通之后的电平状态
	uint8_t active_level[2];
	DIVOOM_KEY_VALUE key_value;
}divoom_app_onoff_key_gpio_info, *Pdivoom_app_onoff_key_gpio_info;

typedef struct _divoom_app_onoff_key_adc_info_
{
	//固定的一个gpio口,做标识用,使用adc1
	uint8_t		key_num;
	//按下之后的adc值,上下浮动50mv
	uint32_t	adc_value_max;
	//和 adc_value_max 相差100
	uint32_t	adc_value_min;
	DIVOOM_KEY_VALUE key_value;
}divoom_app_onoff_key_adc_info, *Pdivoom_app_onoff_key_adc_info;

//send io info
typedef struct{
	DIVOOM_IO_TYPE io_value;
	//当前是插入还是拔出状态，只在发送的时候填充这个数据，注册io进来不需要填充
	DIVOOM_IO_INOUT_STATE now_status;
}divoom_app_onoff_io_info, *Pdivoom_app_onoff_io_info;
//detect检测
typedef struct{
	uint8_t io_info;
	//接通之后的电平状态
	uint8_t active_level;
	DIVOOM_IO_TYPE io_value;
}divoom_app_onoff_detect_gpio_info, *Pdivoom_app_onoff_detect_gpio_info;

//开机调用:检查是否是timer开机，是否是按键开机，其他模式开机
void divoom_app_onoff_check_init(void);

#ifndef DIVOOM_ONOFF_NO_POWER_CONTROL
//是否是闹钟开机，1是，0不是
bool divoom_app_onoff_get_poweron_alarm_flag(void);

//设置一般都是设置为0
void divoom_app_onoff_set_poweron_alarm_flag(bool flag);
#endif

//获取当前开机模式
uint8_t divoom_app_onoff_get_cur_power_on_mode(void);

//比如DIVOOM_APP_ONOFF_WIFI_MODE, DIVOOM_APP_ONOFF_WIFI_CONFIG_MODE, DIVOOM_APP_ONOFF_BT_MODE
//void divoom_app_onoff_set_next_poweron_mode(uint8_t next_mode);
void divoom_app_onoff_set_next_poweron_mode(uint8_t next_mode, uint8_t need_reboot);

//进行软件关机，传入参数，下一个utc开机时间，内部会减去当前时间，算出下次开机设置的时间
//有保存配置命令发出
//void divoom_app_onoff_poweroff_soft(uint32_t next_wakeup_time);

#ifndef DIVOOM_ONOFF_NO_POWER_CONTROL
//返回是不是按键开机
DIVOOM_POWER_ON_STATUS divoom_app_onoff_get_is_key_poweron(void);
#endif

//等待检测够3秒
#ifdef DIVOOM_ONOFF_TEST_MODE
int divoom_app_onoff_wait_check_init(void);
#else
void divoom_app_onoff_wait_check_init(void);
#endif

void divoom_app_onoff_register_event_init(void);

#ifdef DIVOOM_TASK_PRINTF_STACK_INFO
void divoom_app_onoff_check_task_stack_info(void);
#endif

void divoom_app_onoff_add_key_and_detect(void);




void divoom_app_onoff_init(void);
int divoom_app_onoff_check_poweron_init(void);
void divoom_app_onoff_set_sleep(void);
void divooom_app_onoff_set_power(uint8 power_status);



#endif
#endif

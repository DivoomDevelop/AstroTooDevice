#ifndef _DIVOOM_SLEEP_H_
#define _DIVOOM_SLEEP_H_

//test sleep
//#define DIVOOM_SLEEP_TEST_USE
//test sleep

enum 
{   
	DIVOOM_SLEEP_APP_EVENT_WAITING,
	DIVOOM_SLEEP_APP_EVENT_TRY_LISTEN,
};

enum
{
	DIVOOM_SLEEP_STATUS_TEST_MODE,    //试听状态
	DIVOOM_SLEEP_STATUS_STANDBY,    //没有激活
	DIVOOM_SLEEP_STATUS_WAITING,    //正在等待
	DIVOOM_SLEEP_STATUS_SLEEP,      //已经激活
};

//内部事件宏，一般是由外部触发的
enum            
{           
	DIVOOM_SLEEP_INIT_STATUS,  //初始化睡眠信息
	DIVOOM_SLEEP_GET_STATUS,  //得到当前睡眠的状态
	DIVOOM_SLEEP_GET_INFO,    //得到当前睡眠的所有信息
	DIVOOM_SLEEP_TEST_MODE,    //试用模式
	DIVOOM_SLEEP_EXIT_TEST_MODE,    //退出试用模式
	DIVOOM_SLEEP_CANCEL,      //取消睡眠模式
	DIVOOM_SLEEP_SWITCH,      //开关睡眠模式，只有在等待状态才能开关
	DIVOOM_SLEEP_UPDATE,      //更新睡眠模式
	DIVOOM_SLEEP_SAVE,        //保存睡眠配置信息
	DIVOOM_SLEEP_GET_MUSIC_PLAY,    //得到睡眠是否是播放音乐频道
};

void divoom_sleep_init(void);
uint8 divoom_sleep_is_available(void);
void divoom_sleep_get(Pdivoom_system_tool_json_get_sleep pctrl_info);
void divoom_sleep_main(void);

uint8 divoom_sleep_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type);

uint8 divoom_sleep_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type);

uint8 divoom_sleep_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type);

uint8 divoom_sleep_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type);
uint8 divoom_sleep_get_run_status(void);

#endif





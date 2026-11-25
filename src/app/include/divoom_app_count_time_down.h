#ifndef _DIVOOM_APP_COUNT_TIME_DOWN_H_
#define _DIVOOM_APP_COUNT_TIME_DOWN_H_

/*
//测试代码宏
//#define DIVOOM_APP_COUNT_DOWN_TEST

//控制倒计时的播放与暂停以及显示

enum{
	DIVOOM_APP_COUNT_TIME_DOWN_EVENT_PLAY,
};

enum
{
	DIVOOM_APP_COUNT_TIME_DOWN_SET,		//数据结构暂时用Pdivoom_app_count_time_down_info，确定了之后再改
	DIVOOM_APP_COUNT_TIME_DOWN_GET,
};

typedef struct _divoom_app_count_time_down_info_
{
	uint8		status;
	uint8		minute;
	uint8		second;
}divoom_app_count_time_down_info, *Pdivoom_app_count_time_down_info;

void divoom_app_count_time_down_init(void);

void divoom_app_count_time_down_close(void);

//在主循环里面不断地轮训
void divoom_app_count_time_down_main(void);

uint8 divoom_app_count_time_down_schdule_play(uint8 event_id, void *user_data, uint32 resouce_type);

uint8 divoom_app_count_time_down_schdule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type);

uint8 divoom_app_count_time_down_schdule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type);

uint8 divoom_app_count_time_down_schdule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type);
void  divoom_app_count_time_down_get_timer_info(Pdivoom_system_tool_json_timer_info timer_info);
*/
#endif





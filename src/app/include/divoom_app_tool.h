#ifndef _DIVOOM_APP_TOOL_H_
#define _DIVOOM_APP_TOOL_H_

//测试代码宏
//#define DIVOOM_APP_COUNT_DOWN_TEST

//控制倒计时的播放与暂停以及显示

enum{
	DIVOOM_APP_TOOL_EVENT_PLAY,
	DIVOOM_APP_COOUNT_TIME_DOWN_OVER_EVENT_PLAY,
};

enum
{
	DIVOOM_APP_COUNT_TIME_DOWN_SET,		//数据结构暂时用Pdivoom_app_count_time_down_info，确定了之后再改
	DIVOOM_APP_COUNT_TIME_DOWN_GET,
	DIVOOM_APP_COUNT_TIME_DOWN_START,

	DIVOOM_APP_SCORE_SET,				//设置score分数并显示
	DIVOOM_APP_SCORE_GET,				//设置score分数并显示
	DIVOOM_APP_SCORE_START,				//设置score分数并显示

	DIVOOM_APP_TIME_WATCH_SET,			//计时器
	DIVOOM_APP_TIME_WATCH_GET,			//计时器
	DIVOOM_APP_TIME_WATCH_START,			//计时器

	DIVOOM_APP_NOISE_SET,				//噪音
	DIVOOM_APP_NOISE_GET,
	DIVOOM_APP_NOISE_START,

	DIVOOM_APP_POMODORO_SET, //设置番茄参数
	DIVOOM_APP_POMODORO_GET, //获取番茄参数
	DIVOOM_APP_POMODORO_START, //显示番茄时间

	DIVOOM_APP_LVGL_DISP_EXIT,	//lvgl退出显示
};

typedef struct __divoom_app_tool_pomodoro_time_info_
{
	uint8 pomodoro_time; //工作时间长度  
	uint8 pomodoro_long_rest_time; //长休息时长
	uint8 pomodoro_short_rest_time; //短休息时长
	uint8 pomodoro_roll_times; //番茄次数
	uint8 pomodoro_status; //番茄状态
}divoom_app_tool_pomodoro_time_info, *Pdivoom_app_tool_pomodoro_time_info;

typedef struct __divoom_app_tool_time_watch_info
{
	uint8  ctrl_flag;
}divoom_app_tool_time_watch_info, *Pdivoom_app_tool_time_watch_info;


typedef struct __divoom_app_tool_score_info
{
	uint8  on_off;
	uint16 red_score;
	uint16 blue_score;
}divoom_app_tool_score_info, *Pdivoom_app_tool_score_info;

typedef struct __divoom_app_tool_count_time_down_info_
{
	uint8		status;
	uint8		hour;
	uint8		minute;
	uint8		second;
	uint8 		lvgl_flag; // 1 lvgl菜单调用
}divoom_app_tool_count_time_down_info, *Pdivoom_app_tool_count_time_down_info;

typedef struct __divoom_app_tool_noise_info
{
	uint8  ctrl_flag;
}divoom_app_tool_noise_info, *Pdivoom_app_tool_noise_info;


void divoom_app_tool_init(void);

void divoom_app_tool_close(void);

//在主循环里面不断地轮训
void divoom_app_tool_main(void);

uint8 divoom_app_tool_schedule_play_callback(uint8 event_id, void *user_data, uint32 resouce_type);

uint8 divoom_app_tool_schedule_pause_callback(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type);

uint8 divoom_app_tool_schedule_destroy_callback(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type);

uint8 divoom_app_tool_scdedule_key_callback(uint8 key, uint8 key_status, uint32 resouce_type);

//uint8 divoom_app_tool_event(Pdivoom_sys_msg_info msg);

//void divoom_app_tool_set_event(uint8 model_id, uint8 *data);

//uint8 divoom_app_tool_get_event(uint8 model_id, uint8 *data);

//uint32 divoom_app_tool_get_watch_cnt(void);
void divoom_app_tool_get_watch_cnt(Pdivoom_disp_ctrl_watch_ctrl watch_info);

void divoom_app_tool_get_score(Pdivoom_app_tool_score_info score_info);
uint32 divoom_app_tool_get_count_down_cnt(void);
uint32 divoom_app_tool_get_count_down_next_reach_time(void);
uint8 divoom_app_tool_get_count_down_status(void);
uint8 divoom_app_tool_get_count_down_paused(void);
uint8 divoom_app_tool_get_count_down_music_index(void);
void divoom_app_tool_set_count_down_music_index(int music_index);
void divoom_app_tool_count_down_play_voice(int time);
void divoom_app_tool_count_down_stop_voice(void);
void divoom_app_tool_count_down_menu_try_listen(uint8 voice_id, uint8 vol, uint8 on_off);
void divoom_app_tool_get_watch_lap_nums(int *lap_num, uint32 *lap_time);
void divoom_app_tool_set_watch_lap_nums(uint32 *lap_time, uint32 lap_num);


void divoom_app_tool_set_time_watch(void *data);

void divoom_app_tool_set_score(void *data);

void divoom_app_tool_set_count_time_down(void *data);

void divoom_app_tool_set_noise(void *data);
uint32 divoom_app_tool_get_score_value(uint8 flag);

#endif





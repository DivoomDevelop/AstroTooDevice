#ifndef _divoom_tomato_time_
#define _divoom_tomato_time_

enum
{
	DIVOOM_APP_TOMATO_SET, //设置番茄参数
	DIVOOM_APP_TOMATO_GET, //获取番茄参数
	DIVOOM_APP_TOMATO_SEND_LIST_COUNT, //发送番茄个数
	DIVOOM_APP_TOMATO_SEND_ONE_BY_ONE, //单个发送番茄
	DIVOOM_APP_TOMATO_DELETE, //删除番茄时间
	DIVOOM_APP_TOMATO_START, //显示番茄时间
	DIVOOM_APP_TOMATO_FOCUS_DONE, //专注记录接收反馈
	DIVOOM_APP_TOMATO_LISTEN, //试听
	DIVOOM_APP_TOMATO_LISTEN_VOLUME, //试听音量
	DIVOOM_APP_TOMATO_DISP_NOTICE, //显示弹窗
	DIVOOM_APP_TOMATO_EXIT_NOTICE, //退出弹窗
	DIVOOM_APP_TOMATO_EXIT_DISP_EVENT, // 退出提醒
	DIVOOM_APP_TOMATO_EXIT_DISP, //退出番茄时钟显示
};

typedef enum{
	TOMATO_MAIN_WAIT,
	TOMATO_MAIN_FOCUS,
	TOMATO_MAIN_RESET,
}DIVOOM_TOMATO_MAIN_STATUS;

typedef enum{
	TOMATO_STOP,
	TOMATO_START,
	TOMATO_PUASE,
}DIVOOM_TOMATO_STATUS;

enum
{
    DIVOOM_TOMATO_SCHEDULE_TOMATO_ACTIVE,
    DIVOOM_TOMATO_SCHEDULE_TOMATO_TRY_VOICE,
};

uint8 divoom_tomato_time_schedule_pause_callback(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type);

uint8 divoom_tomato_time_schedule_play_callback(uint8 event_id, void *user_data, uint32 resouce_type);

uint8 divoom_tomato_time_schedule_destory_callback(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type);

uint8 divoom_tomato_time_schedule_key_callback(uint8 key_value, uint8 key_status, uint32 resouce_type);

void divoom_tomato_time_loop(void);
void divoom_tomato_time_config_save(void);

void divoom_tomato_time_init(void);

void divoom_tomato_time_focus_record_pack(void);

uint8 divoom_tomato_time_get_main_status(void);
uint8 divoom_tomato_time_get_sub_status(void);

uint32 divoom_tomato_time_get_time(void);

uint8 divoom_tomato_time_event(Pdivoom_sys_msg_info msg);
Pdivoom_tomato_time_info divoom_tomato_time_get_pre_tomato(void);
Pdivoom_tomato_time_info divoom_tomato_time_get_next_tomato(void);
Pdivoom_tomato_time_info divoom_tomato_time_get_one_tomato(int index);
int divoom_tomato_time_get_tomato_num(void);
int divoom_tomato_time_get_cur_do_tomato_num(void);
int divoom_tomato_time_get_tomato_offset(void);
void divoom_tomato_time_change_tomato_info(int id);
Pdivoom_tomato_time_info divoom_tomato_time_get_default_tomato(void);
void divoom_tomato_time_cancel(void);
int divoom_tomato_time_menu_update(uint32 tomato_id);
void divoom_tomato_time_set_tomato(Pdivoom_tomato_time_info info, int flag);
int divoom_tomato_time_get_cur_tomato_index(void);
void divoom_tomato_time_try_listen(uint8 voice_id, uint8 vol, uint8 on_off, uint8 lvgl);
void divoom_tomato_time_play_phase_notice_tone(void);
void divoom_tomato_time_set_system_bell(uint8 system_index);
void divoom_tomato_time_set_sound_on_off(uint8 on_off);
void divoom_tomato_time_set_local_config(uint8 work_min, uint8 short_min, uint8 long_min, uint8 roll_times);
void divoom_tomato_time_get_tomato_list(void);

#endif

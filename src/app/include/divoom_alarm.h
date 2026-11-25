#ifndef _DIVOOM_ALARM_H_
#define _DIVOOM_ALARM_H_

#include <time.h>

//#define DIVOOM_ALARM_VOICE_GIF_USE

/*****************************test*******************************/
//#define DIVOOM_ALARM_TEST_USE
/*****************************test*******************************/
#define DIVOOM_ALARM_ACTIVE_TIME		(300) 
#define DIVOOM_ALARM_RESTORE_TIME		(600)//(300)   //S

enum
{
	DIVOOM_ALARM_APP_EVENT_ALARM = 1,
	DIVOOM_ALARM_APP_EVENT_LISTEN,
};

typedef struct{
	uint8 vol;
	uint8 music_type;
}divoom_alarm_listen_struct, *Pdivoom_alarm_listen_struct;

typedef struct{
	uint8_t	mode;
	uint8_t	index;
}divoom_alarm_get_gif_addr, *Pdivoom_alarm_get_gif_addr;

enum
{
	//DIVOOM_APP_SYSTEM_UPDATE_TIME,
	DIVOOM_ALARM_UPDATE_ALARM,
	DIVOOM_ALARM_SET_ALARM,
	DIVOOM_ALARM_DEL_ALARM,
	DIVOOM_ALARM_UPDATE_MEMORIAL,
	DIVOOM_ALARM_SET_MEMORIAL,
	DIVOOM_ALARM_DEL_MEMORIAL,
	DIVOOM_ALARM_GET_STATUS,
	DIVOOM_ALARM_CANCEL,
	DIVOOM_ALARM_LISTEN_ALARM,
	DIVOOM_ALARM_GET_NEXT_ALARM_TIME,			//获取下一个闹钟的utc时间
	DIVOON_ALARM_END,
	DIVOOM_ALARM_LISTEN_ALARM_SET_VOLUMN,
	DIVOOM_ALARM_TF_FALLBACK_TONE,
	//DIVOOM_APP_KEY_EVENT_ID,
#ifdef DIVOOM_ALARM_TEST_USE
//**************************test****************************
	DIVOOM_ALARM_TEST_ADD_ALARM,
//**************************test****************************
#endif
	DIVOOM_ALARM_END_EVENT,
};

enum
{
    DIVOOM_ALARM_STATUS_WAIT,
    DIVOOM_ALARM_STATUS_INTTRUPT,
    DIVOOM_ALARM_STATUS_ACTIVE,
    DIVOOM_ALARM_STATUS_SNOOZE,
    DIVOOM_ALARM_STATUS_TRY_LISTEN,
    DIVOOM_ALARM_STATUS_OVER_SNOOZE,
    DIVOOM_ALARM_STATUS_SCHEDULE_OUT,
};

//#ifdef DIVOOM_ALARM_VOICE_GIF_USE
/*typedef enum
{
	DIVOOM_ALARM_LEAVE_READY_LOAD,    //需要加载留言文件
	DIVOOM_ALARM_LEAVE_OK_LOAD,       //已经加载留言文件
	DIVOOM_ALARM_LEAVE_NO_LOAD,       //没有要加载的留言文件
}DIVOOM_LEAVE_LOAD_STATUS;*/

//#endif

/*typedef struct _divoom_alarm_link_info
{
	int   alarm_id;             //第几个闹钟或者纪念日
	uint8 alarm_type;           //闹钟类型0为闹钟；1为纪念日
	uint8 alarm_status;           //闹钟是否有效
	uint8 cyc_flag;             //循环周期，以星期为单位
	struct tm alarm_time;            //当cyc_flag为0的话，是一次生闹钟，这个值为到点的时间值，如果CYC_FLAG不为0，将是当天的时间值。
	uint8 vol;                 //闹钟音量
#ifdef DIVOOM_ALARM_VOICE_GIF_USE
	uint8 music_type;				//音乐类型
	uint8 load_alarm_leave_flag;    //留言加载状态
	uint8 load_pic_flag;            //图片加载状态
	char *alarm_leave_id;       //当音乐类型为0的时候，这个才有效，录音的id
	char *image_info;             //闹钟到点的触发音乐信息的路径
#endif
	struct _divoom_alarm_link_info *next;
}divoom_alarm_link_info, *Pdivoom_alarm_link_info;*/

void divoom_alarm_init(uint8 poweron_mode);
void divoom_alarm_clear_rtc_clock_wake_pending(void);
int divoom_alarm_is_rtc_clock_wake_pending(void);
void divoom_alarm_disp_add_del_pic(uint8_t flag);

uint8 divoom_alarm_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type);

uint8 divoom_alarm_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type);

uint8 divoom_alarm_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type);

uint8 divoom_alarm_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type);

void divoom_alarm_stop_alarm_task(void);

void divoom_alarm_start_alarm_task(void);
T_U32 divoom_alarm_time_to_sec(Pdivoom_alarm_system_date date);
void divoom_alarm_get_sys_time(Pdivoom_alarm_system_date date);
T_U32 divoom_alarm_get_today_sec(void);

int divoom_alarm_poweron_get_all_alarm_info(void *dialy_alarm_queue);
uint32 divoom_alarm_poweron_lib_get_next_active_alarm_time(void *dialy_alarm_queue);
T_VOID divoom_alarm_lib_convert_alarm_time_to_local(T_DIVOOM_ALARM_ACTIVE_DO_INFO *palarm_info, Pdivoom_alarm_system_date local_time, Pdivoom_alarm_system_date cur_time);
void divoom_alarm_get_current_alarm_name_str(char *disp_str);
void divoom_alarm_get_current_alarm_time_str(char *disp_str);
void divoom_alarm_get_active_menu_date_str(char *disp_str, int disp_str_size);
int divoom_alarm_get_current_alarm_mode(void);
#if 0 /* 唤醒场景：AstroToo 无调用方 */
int divoom_alarm_get_current_alarm_scene_index(void);
#endif
int divoom_alarm_get_alarm_active_status(int *remain_time);
void divoom_alarm_process_alarm_menu_btn(int btn_flag);
T_VOID divoom_alarm_ctrl_try_listen(T_U8 voice_id, T_U8 vol, T_U8 on_off, T_U8 lvgl);
#ifdef DIVOOM_TIME_FRAME_SPEAKER
T_VOID divoom_alarm_ctrl_try_listen_scene(T_U8 scene_index, T_U8 vol, T_U8 on_off, T_U8 lvgl);
#endif
T_VOID divoom_alarm_ctrl_set_vol(T_U8 vol);
T_VOID divoom_alarm_ctrl_preview_stop(T_VOID);
T_S32 divoom_alarm_get_active_alarm(void);

/*********************************************************************
  Function:        divoom_alarm_menu_add_alarm_item
  Description:     增加一个闹钟
  Input:           alarm_id   
  Return:          NONE
**********************************************************************/


void divoom_alarm_menu_add_alarm_item(T_DIVOOM_ALARM_ACTIVE_DO_INFO *palarm_info);


/*********************************************************************
  Function:        divoom_alarm_menu_delete_alarm
  Description:     删除某个闹钟
  Input:           alarm_id   
  Return:          NONE
**********************************************************************/


void divoom_alarm_menu_delete_alarm(int alarm_id);

/*********************************************************************
  Function:        divoom_alarm_menu_update_alarm_item
  Description:     更新某个闹钟的信息
  Input:           palarm_info   
  Return:          NONE
**********************************************************************/


void divoom_alarm_menu_update_alarm_item(T_DIVOOM_ALARM_ACTIVE_DO_INFO *palarm_info);
/*********************************************************************
  Function:        divoom_alarm_menu_get_alarm_list
  Description:     获取闹钟列表，这个列表返回使用后要释放
  Input:           alarm_num  闹钟个数  
  Return:          返回闹钟列表指针
**********************************************************************/

T_DIVOOM_ALARM_ACTIVE_DO_INFO *divoom_alarm_menu_get_alarm_list(int *alarm_num);
/*********************************************************************
  Function:        divoom_alarm_get_last_alarm_info
  Description:     得到马上要激活的闹钟,为了主菜单显示最近的uyqk
  
  Input:            
  Return:          返回闹钟列表指针
**********************************************************************/
T_DIVOOM_ALARM_ACTIVE_DO_INFO *divoom_alarm_get_last_alarm_info(void);
//得到激活的闹钟的时间
int divoom_alarm_get_run_alarm_time(void);

/*********************************************************************
  Function:        divoom_alarm_get_active_dialy_name
  Description:     获取激活的纪念日的名称
  
  Input:            
  Return:          返回纪念日的名称
**********************************************************************/

char *divoom_alarm_get_active_dialy_name(void);

/** 计算闹钟模块下一次应对 RTC 写入的唤醒时刻（含提前开机量），无则返回 0 */
time_t divoom_alarm_get_next_rtc_wake_unix(void);

void divoom_alarm_main(void *arg);

#endif






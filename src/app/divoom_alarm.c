#include <stdint.h>
#include <stdarg.h>
#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_dial_menu.h"
#include "divoom_lvgl_base_dial_menu.h"

#define divoom_alarm_debug  divoom_debug

#ifdef DIVOOM_TIME_FRAME_SPEAKER
#include <time.h>
#include "divoom_onoff.h"
#include "divoom_tf_music.h"
#include "divoom_music_ctrl.h"
#include "divoom_power_control.h"
#include "divoom_jl_amp_adapter.h"
#define DIVOOM_ALARM_SCHEDULE_RESOURCE_TYPE (DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE | DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE)
#else
#define DIVOOM_ALARM_SCHEDULE_RESOURCE_TYPE DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE
#endif

#if 1
//#define DIVOOM_ALARM_PASS_TIME			(5*60)   //S
#define DIVOOM_ALARM_PASS_TIME			(300)   //S
#define DIVOOM_ALARM_TRY_VOICE_TIME		(60)   //S
#define DIVOOM_ALARM_MAX_VOICE_TIME		(60)   //S
//#define DIVOOM_ALARM_RESTORE_TIME		(5*60)   //S
#define DIVOOM_ALARM_LIGHT_TIME		    (600)   //S
#define DIVOOM_ALARM_MAX_LIGHT_TIME		(120)   //S
//#define DIVOOM_ALARM_RESTORE_TIME		(60)   //S
#define DIVOOM_ALARM_NOTIFY_TIME		(4000) //MS
#define DIVOOM_ALARM_TRY_TIMES			3

/*
 * 写 RTC 唤醒的时刻 = 响铃时刻 − early（秒）。
 * 设为 0：在「设定响铃时间」到点唤醒。
 * 若开机常慢于响铃点，可改为 30～60，让机器提前拉电再进应用等响铃。
 */
#ifndef DIVOOM_ALARM_HW_WAKE_EARLY_SEC
#define DIVOOM_ALARM_HW_WAKE_EARLY_SEC		60
#endif
#ifndef DIVOOM_ALARM_HW_WAKE_EARLY_HEAVY_SEC
#define DIVOOM_ALARM_HW_WAKE_EARLY_HEAVY_SEC	60
#endif

#define DIVOOM_ALARM_TAG				"DIVOOM_ALARM"

#define DIVOOM_ALARM_TASK_STACK			(1024*4)
#define DIVOOM_ALARM_TASK_PRIOR			(1)

#define DIVOOM_ALARM_PRINT(format,...)	divoom_print(format, ##__VA_ARGS__);

//存储方式，先存放两个int的时间，第一个是闹钟的，第二个是纪念日的
//后面就连续写数据，数据结构为T_DIVOOM_ALARM_ACTIVE_DO_INFO
//#define DIVOOM_ALARM_INFO_PATH			"divoom_alarm_info.cfg"
//#define DIVOOM_DIALY_INFO_PATH			"divoom_dialy_info.cfg"

//divoom_alarm文件分部：第一个字节闹钟数量a，之后是10*T_DIVOOM_ALARM_CFG数据，然后是一个字节纪念日数量b，之后是10*T_DIVOOM_DIALY_CFG数据
//#define DIVOOM_ALARM_FILE				"/spiffs/divoom_alarm"
//#define DIVOOM_APP_ALARM_CONFIG_NAME  "divoom_alarm_config"

/*typedef struct _divoom_alarm_link_info
{
	T_DIVOOM_UPDATE_ALARM_CFG *alarm_info;
	struct _divoom_alarm_link_info *next;
}divoom_alarm_link_info, *Pdivoom_alarm_link_info;*/


typedef struct{
	time_t							next_active_light_time;		//下一次亮度的秒数，utc获取的对比
	time_t							next_active_time;			//下一次的秒数，utc获取的对比
	time_t							end_time;		//已经超时的闹钟，再次激活时间

	//SemaphoreHandle_t				task_mux;
	void				*divoom_alarm_task_mux;

	DIVOOM_ALARM_ACTIVE_CALLBACK	alarm_active_cb;				//闹钟响起，调这个函数

    uint8                           first_load_flag;
	uint8							active_status;				//是否在运行，可能不需要
	uint8                           read_image_flag;
	uint8_t							alarm_status;				//alarm目前的状态
	uint8_t							run_times;				//当前贪睡级别
	uint8_t							play_vol;					//当前闹钟声音大小
	uint8_t                         early_ctrl_flag;        //当前改变颜色的状态
	//cur_alarm_info想存放图片内容,用image_data
	T_DIVOOM_ALARM_ACTIVE_DO_INFO	*cur_alarm_info;			//当前正在运行的闹钟


	int								pnet_alarm_info;			//获取闹钟的是否有更新的标志
	uint32                          update_net_info_time;       //获取的更新时间
    T_VOID                          *dialy_alarm_queue;

	uint8							run_flag;
	uint8							stop_flag;
    char                            next_image_file[52];
    uint8                           *cur_image_data;
    int                             cur_image_len;
    uint8                           *next_image_data;
    int                             next_image_len;
    int                             next_active_id;
    uint32                          resouce_type;
	void							*task_handle;
	T_DIVOOM_ALARM_ACTIVE_DO_INFO   *last_alarm_info;
	divoom_lvgl_dial_ctr_info dial_info;
	/** RTC 闹钟唤醒开机：跳过一次「一次性闹钟已过期则关闭」，避免晚启动误删刚唤醒的闹钟 */
	uint8_t							rtc_clock_skip_expire_once;
	/** RTC 闹钟唤醒：首帧缩短轮询间隔，尽快进入 INTTRUPT / 闹钟界面 */
	uint8_t							rtc_clock_short_first_delay;
	uint8_t							rtc_clock_wake_pending;
	uint8_t							rtc_clock_wake_ui_shown;
}divoom_alarm_struct, *Pdivoom_alarm_struct;

Pdivoom_alarm_struct pdivoom_alarm_handle = NULL;

void divoom_alarm_clear_rtc_clock_wake_pending(void)
{
	if (pdivoom_alarm_handle) {
		pdivoom_alarm_handle->rtc_clock_wake_pending = 0;
		pdivoom_alarm_handle->rtc_clock_wake_ui_shown = 0;
	}
}

int divoom_alarm_is_rtc_clock_wake_pending(void)
{
	return pdivoom_alarm_handle && pdivoom_alarm_handle->rtc_clock_wake_pending;
}


void divoom_alarm_stop_voice(Pdivoom_alarm_struct alarm_handle);
T_VOID divoom_alarm_lib_set_next_active_alarm(void);
void divoom_alarm_del_alarm_dialy_id(int type, int id);
void divoom_alarm_del_all_item(void);
void divoom_alarm_del_by_mode(uint8_t mode);
uint8 divoom_alarm_get_dialay_info(void);
uint8 divoom_alarm_get_alram_info(void);
void divoom_alarm_update_alram_item(int is_get_all, int alarm_id);
void divoom_alarm_update_dialy_item(int is_get_all, int alarm_id);
static void divoom_alarm_cancel_alarm(void);
void divoom_alarm_disp_gif(Pdivoom_alarm_struct alarm_handle);
void divoom_alarm_cancel_by_id(int index, uint8_t type);
void divoom_alarm_get_disp_gif(T_DIVOOM_ALARM_ACTIVE_DO_INFO *alarm_active_info);
void divoom_alarm_play_voice(Pdivoom_alarm_struct alarm_handle, int time);
void divoom_alarm_stop_voice(Pdivoom_alarm_struct alarm_handle);
static void divoom_alarm_play_builtin_fallback_tone(Pdivoom_alarm_struct alarm_handle, int volumn, const char *why);
void *divoom_alarm_com_msg_malloc(void *msg_data);
void *divoom_alarm_com_msg_free(void *msg_data);
void divoom_alarm_play_listen_voice(T_U8 sound_type, int volume);
//读取last_updata_time
void divoom_alarm_get_last_update_time(int *alarm_last_update_time, int *dialy_last_update_time);
//存储一个闹钟信息
//void divoom_alarm_save_one_alarm_info(T_DIVOOM_ALARM_ACTIVE_DO_INFO data);
//存储全部闹钟信息,这样做的话就不能保证数据完全是由下层处理的了
void divoom_alarm_save_all_alarm_info();
//获取全部闹钟信息，在开机的获取，如果获取到的信息没有变化就把本地的存到链表里面
void divoom_alarm_get_all_alarm_info(void);
void divoom_alarm_update_alarm_info(void *data);
void divoom_alarm_update_dialy_info(void *data);
void divoom_alarm_play_listen_voice_over_callback(uint8 status);
void divoom_alarm_update_one_alarm_info(Pdivoom_net_alarm_set_alarm_info alarm_info);
void divoom_alarm_update_dialy_one_info(Pdivoom_net_memorial_info memorial_item);

#ifdef DIVOOM_ALARM_TEST_USE
void divoom_alarm_test_alarm(void);
#endif

enum{
    DIVOOM_ALARM_EARLY_COLOR_WAIT,
    DIVOOM_ALARM_EARLY_COLOR_ACTIVE,
    DIVOOM_ALARM_EARLY_COLOR_PASS,
};
/*#define DIVOOM_ALARM_DIALY_MUSIC_FILE_NAME "alarm_guitar.mp3"

const char divoom_alarm_music_file_table[][25] = 
{    
//    "",
//    "",
	"Clock_Rainstorm.mp3",
	"Clock_Ocean_Waves.mp3",
	"Clock_Urban.mp3",	
	"Clock_Forest.mp3",	
	"Clock_Whale.mp3",	
	"Clock_Energize.mp3",
	"Clock_Eagle.mp3",
	"Clock_Campanula.mp3",
	"Clock_Normal1.mp3",
	"Clock_Piano.mp3",
	"Clock_Normal3.mp3",
	"Clock_Normal2.mp3",
};*/

#define DIVOOM_ALARM_DIALY_VOICE_ID DIVOOM_VOICE_ID_CLOCK_GUITAR
const T_U8 divoom_alarm_voice_table[] = {
	DIVOOM_VOICE_ID_CLOCK_RAINSTORM, DIVOOM_VOICE_ID_CLOCK_OCEAN_WAVES,
	DIVOOM_VOICE_ID_CLOCK_URBAN, DIVOOM_VOICE_ID_CLOCK_FOREST,
	DIVOOM_VOICE_ID_CLOCK_WHALE, DIVOOM_VOICE_ID_CLOCK_ENERGIZE,
	DIVOOM_VOICE_ID_CLOCK_EAGLE, DIVOOM_VOICE_ID_CLOCK_CAMPANULA,
	DIVOOM_VOICE_ID_CLOCK_NORMAL1, DIVOOM_VOICE_ID_CLOCK_PIANO,
	DIVOOM_VOICE_ID_CLOCK_NORMAL3, DIVOOM_VOICE_ID_CLOCK_NORMAL2,
};

#ifdef DIVOOM_TIME_FRAME_SPEAKER
static const T_U8 divoom_alarm_scene_voice_table[] = {
	DIVOOM_VOICE_ID_CLOCK_SOFT_FRADIENT,
	DIVOOM_VOICE_ID_CLOCK_SKYLINE,
	DIVOOM_VOICE_ID_CLOCK_MISTY_MEADOW,
	DIVOOM_VOICE_ID_CLOCK_CITE_SCAPE,
	DIVOOM_VOICE_ID_CLOCK_COURTYARD,
	DIVOOM_VOICE_ID_CLOCK_FOREST_DAWM,
};
#endif


T_U32 divoom_alarm_get_today_sec(void)
{
	time_t time_now, time_today_zero;
	struct tm p_now;
	struct tm p_today_zero;
	time_now = time(0);
	localtime_r(&time_now, &p_now);
	memcpy(&p_today_zero, &p_now, sizeof(struct tm));
	time_now = mktime(&p_now);
	p_today_zero.tm_hour = 0;
	p_today_zero.tm_min = 0;
	p_today_zero.tm_sec = 0;
	time_today_zero = mktime(&p_today_zero);
	if(time_now >= time_today_zero)
		return (time_now - time_today_zero);
	else
		return 0;
}

void divoom_alarm_get_sys_time(Pdivoom_alarm_system_date date)
{
	time_t time_now;
	struct tm p_now;
	time_now = time(0);
	localtime_r(&time_now, &p_now);
	if(date != NULL)
	{
		date->year = p_now.tm_year;
		date->month = p_now.tm_mon;
		date->day = p_now.tm_mday;
		date->hour = p_now.tm_hour;
		date->minute = p_now.tm_min;
		date->second = p_now.tm_sec;
		date->week = p_now.tm_wday;
	}
}

void divoom_alarm_sec_to_date(Pdivoom_alarm_system_date date, T_U32 second)
{
	struct tm p_date;
	time_t time_now = (time_t)second;
	localtime_r(&time_now, &p_date);
	if(date)
	{
		date->year = p_date.tm_year;
		date->month = p_date.tm_mon;
		date->day = p_date.tm_mday;
		date->hour = p_date.tm_hour;
		date->minute = p_date.tm_min;
		date->second = p_date.tm_sec;
	}
}

T_U32 divoom_alarm_time_to_sec(Pdivoom_alarm_system_date date)
{
	struct tm p_date;
	time_t sec_all;
	if(date != NULL)
	{
		memset(&p_date, 0, sizeof(struct tm));
		//让系统自动选择是否夏令时
		p_date.tm_isdst = -1;
		p_date.tm_year = date->year;
		p_date.tm_mon = date->month;
		p_date.tm_mday = date->day;
		p_date.tm_hour = date->hour;
		p_date.tm_min = date->minute;
		p_date.tm_sec = date->second;
	}
	if((sec_all = mktime(&p_date)) != -1)
	{
		return (T_U32)sec_all;
	}
	return 0;
}


void divoom_alarm_print_link_item(uint8 type)
{
    int i, number;
    T_DIVOOM_ALARM_ACTIVE_DO_INFO *data_item;

    number = divoom_queue_items(pdivoom_alarm_handle->dialy_alarm_queue);

    for(i = 0; i < number; i ++)
	{
        data_item = divoom_queue_get_index_item(pdivoom_alarm_handle->dialy_alarm_queue, i);
		if(data_item && type == data_item->type)
		{
			divoom_print("alarm_id = %d,on_off = %d,type = %d,index = %d,cyc_flag = %x,data = %d-%d-%d,time = %d-%d\n"
                , data_item->alarm_id, data_item->on_off, data_item->type, data_item->index, data_item->cyc_flag
                , data_item->year, data_item->month+1, data_item->day, data_item->hour, data_item->minute);
		}
	}
}

/**
 * NAME         divoom_alarm_unactive_all_alarm
 * @BRIEF	    将所有类型的闹钟全部设置为无效方式
                
 * @PARAM	    type : 类型
 
 * @RETURN	NONE
 * @RETVAL	
 */


void divoom_alarm_unactive_all_alarm(uint8 type)
{
    int i, number;
    T_DIVOOM_ALARM_ACTIVE_DO_INFO *data_item;

    number = divoom_queue_items(pdivoom_alarm_handle->dialy_alarm_queue);

    for(i = 0; i < number; i ++)
	{
        data_item = divoom_queue_get_index_item(pdivoom_alarm_handle->dialy_alarm_queue, i);
		if(data_item && type == data_item->type)
		{
            data_item->active_status = 0;
		}
	}
}
int divoom_alarm_remove_unactive_alarm_compare(void *item1, void *item2)
{
    T_DIVOOM_ALARM_ACTIVE_DO_INFO *data_item = (T_DIVOOM_ALARM_ACTIVE_DO_INFO *)item2;

    if(data_item->active_status == 0)
    {
        return  0;
    }
    else
    {
        return 1;
    }
}



/**
 * NAME         divoom_alarm_remove_unactive_alarm
 * @BRIEF	    从队列中移除所有没有效的闹钟
                
 * @PARAM	    type : 类型
 
 * @RETURN	NONE
 * @RETVAL	
 */


void divoom_alarm_remove_unactive_alarm(void)
{
    uint32 img_id;
    T_DIVOOM_ALARM_ACTIVE_DO_INFO *data_item;

    while(1)
    {
        data_item = divoom_queue_pop_cmp(pdivoom_alarm_handle->dialy_alarm_queue, NULL, divoom_alarm_remove_unactive_alarm_compare);
        if(data_item == NULL)
        {
            break;
        }
        pdivoom_alarm_handle->pnet_alarm_info = 1;
        if(data_item->type == DIVOOM_ALARM_TYPE_ALARM)
        {
            img_id = DIVOOM_MANAGE_IMAGE_ID_BASE_ALARM_GIF + data_item->alarm_id;
            divoom_alarm_cancel_by_id(data_item->alarm_id, DIVOOM_ALARM_TYPE_ALARM);
        }
        else
        {
            img_id = DIVOOM_MANAGE_IMAGE_ID_BASE_DIALY_GIF + data_item->alarm_id;
            divoom_alarm_cancel_by_id(data_item->alarm_id, DIVOOM_ALARM_TYPE_DIALY);
        }
        
        divoom_manage_image_del(DIVOOM_MANAGE_IMAGE_ALARM_GIF,&img_id, 1, 0);
        divoom_system_free(data_item);
    }
}


int divoom_alarm_find_alarm_compare(void *item1, void *item2)
{
    int *alarm_id = (int *)item1;
    T_DIVOOM_ALARM_ACTIVE_DO_INFO *data_item = (T_DIVOOM_ALARM_ACTIVE_DO_INFO *)item2;

    if(data_item->type == DIVOOM_ALARM_TYPE_ALARM && data_item->alarm_id == *alarm_id)
    {
        return  0;
    }
    else
    {
        return 1;
    }
}

int divoom_alarm_find_dialy_compare(void *item1, void *item2)
{
    int *alarm_id = (int *)item1;
    T_DIVOOM_ALARM_ACTIVE_DO_INFO *data_item = (T_DIVOOM_ALARM_ACTIVE_DO_INFO *)item2;

    if(data_item->type == DIVOOM_ALARM_TYPE_DIALY && data_item->alarm_id == *alarm_id)
    {
        return  0;
    }
    else
    {
        return 1;
    }
}



uint8 divoom_alarm_get_dialay_info(void)
{
    //divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_MEMORIAL, DIVOOM_SERVER_TOPIC, NULL);
	return 0;
}

#ifdef DIVOOM_TIME_FRAME_SPEAKER
/* TF 铃声 leave_url 统一为 music_id 字符串（对照 T113 tf_sound_pick） */
static void divoom_alarm_normalize_tf_leave_url(T_DIVOOM_ALARM_ACTIVE_DO_INFO *info)
{
	int music_id;

	if (info == NULL || info->mode != 4 || info->leave_url[0] == '\0')
	{
		return;
	}
	music_id = divoom_tf_music_alarm_resolve_music_id(info->leave_url);
	if (music_id > 0)
	{
		snprintf(info->leave_url, sizeof(info->leave_url), "%d", music_id);
	}
}

static int divoom_alarm_tf_use_first_track_if_needed(T_DIVOOM_ALARM_ACTIVE_DO_INFO *info)
{
	int music_id;
	int first_id;

	if (info == NULL || info->mode != 4)
	{
		return 0;
	}
	music_id = divoom_tf_music_alarm_resolve_music_id(info->leave_url);
	if (music_id > 0)
	{
		return music_id;
	}
	first_id = divoom_tf_music_get_first_music_id();
	if (first_id <= 0)
	{
		return 0;
	}
	snprintf(info->leave_url, sizeof(info->leave_url), "%d", first_id);
	return first_id;
}
#endif

#if DIVOOM_SHOW_MODE
/* 展示机闹钟不按周循环；纪念日 DIALY 不动 */
static void divoom_alarm_show_mode_strip_week_cyc(T_DIVOOM_ALARM_ACTIVE_DO_INFO *info)
{
    if (info && info->type != DIVOOM_ALARM_TYPE_DIALY)
        info->cyc_flag = 0;
}
#endif

void divoom_alarm_convert_net_alarm_to_local( T_DIVOOM_ALARM_ACTIVE_DO_INFO *local_info, Pdivoom_net_alarm_set_alarm_info net_alarm)
{
    int j = 0;
	struct tm time_info;    
	time_t time_now;

    local_info->index = net_alarm->alarm_id;
    local_info->alarm_id = net_alarm->alarm_id;
    local_info->type = DIVOOM_ALARM_TYPE_ALARM;
    local_info->on_off = net_alarm->on_off;
    time_now = net_alarm->utc_time;
    localtime_r(&time_now, &time_info);
    local_info->year = time_info.tm_year;
    local_info->month = time_info.tm_mon;
    local_info->day = time_info.tm_mday;
    local_info->hour = time_info.tm_hour;
    local_info->minute = time_info.tm_min;
	local_info->tip_index = net_alarm->sound_system;
	local_info->mode = net_alarm->sound_type;
	local_info->volume = net_alarm->volume;
    local_info->cyc_flag = net_alarm->cyc[6] > 0?1:0;
    for(j=0;j<6;j++)
    {
    	if(net_alarm->cyc[j])
    		local_info->cyc_flag |= (1<<(j+1));
    }
    local_info->update_time = net_alarm->update_time;
    local_info->early_flag = 0;
    strcpy((char *)local_info->image_data, net_alarm->image_id);
	memcpy(local_info->leave_url, net_alarm->voice_id, sizeof(net_alarm->voice_id));
	local_info->leave_url[sizeof(local_info->leave_url) - 1] = '\0';
#ifdef DIVOOM_TIME_FRAME_SPEAKER
	divoom_alarm_normalize_tf_leave_url(local_info);
	divoom_alarm_tf_use_first_track_if_needed(local_info);
#endif
    local_info->active_status = 1;
	if(strlen(net_alarm->alarm_name) > 0)
	{
		strcpy((char *)local_info->alarm_name, net_alarm->alarm_name);
	}
	
#if DIVOOM_SHOW_MODE
    divoom_alarm_show_mode_strip_week_cyc(local_info);
#endif
    if(local_info->cyc_flag == 0 && net_alarm->utc_time < (int)time(0))
    {
        local_info->on_off = 0;
        //divoom_print("%s: we will pass the alarm time: %d, %d\n", __func__, net_alarm->utc_time, (int)time(0));
    }
	
}

void divoom_alarm_update_one_alarm_info(Pdivoom_net_alarm_set_alarm_info alarm_info)
{
    uint32 image_id;
    T_DIVOOM_ALARM_ACTIVE_DO_INFO *data_item;
    divoom_manage_image_item_info image_info;

#if DIVOOM_SHOW_MODE
    if (alarm_info)
        memset(alarm_info->cyc, 0, sizeof(alarm_info->cyc));
#endif

    data_item = divoom_queue_get_cmp(pdivoom_alarm_handle->dialy_alarm_queue, &alarm_info->alarm_id, divoom_alarm_find_alarm_compare);
    if(data_item)
    {
        if(data_item->update_time != alarm_info->update_time)
        {
            //纪念日有更新，将更新
            /*if(strcmp(data_item->image_data, alarm_info->image_id))
            {
                image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_ALARM_GIF + alarm_info->alarm_id;
                divoom_manage_image_del(DIVOOM_MANAGE_IMAGE_ALARM_GIF, &image_id, 1, 0);
            }*/

			if(strcmp(data_item->leave_url, alarm_info->voice_id))
			{
				image_id = (uint32)alarm_info->alarm_id;
				divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_ALARM_RECORD, &image_id, 1);
			}
            divoom_alarm_convert_net_alarm_to_local(data_item, alarm_info);
            divoom_alarm_cancel_by_id(data_item->alarm_id, DIVOOM_ALARM_TYPE_ALARM);
            pdivoom_alarm_handle->pnet_alarm_info = 1;
        }
        data_item->active_status = 1;
    }
    else
    {       
        //新增加的闹钟先删除文件
        /*
        image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_ALARM_GIF + alarm_info->alarm_id;
        divoom_manage_image_del(DIVOOM_MANAGE_IMAGE_ALARM_GIF,&image_id, 1, 0);
        */
		image_id = (uint32)alarm_info->alarm_id;
		divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_ALARM_RECORD, &image_id, 1);
		
        data_item = divoom_system_malloc(sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
        if(data_item == NULL)
        {
            divoom_print("%s: it is the error and pls check it!\n",__func__);
            return;
        }
        
        divoom_alarm_convert_net_alarm_to_local(data_item, alarm_info);
        divoom_queue_push(pdivoom_alarm_handle->dialy_alarm_queue, data_item);
        pdivoom_alarm_handle->pnet_alarm_info = 1;
    }
    
	/*strcpy(image_info.file_addr, alarm_info->image_id);
	image_info.flag = 1;
	image_info.image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_ALARM_GIF + alarm_info->alarm_id;
	image_info.pcallback = divoom_app_com_finishcallback;
	divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_ALARM_GIF, &image_info, 1);
	*/

	if(strlen(alarm_info->voice_id) && alarm_info->sound_type != 4)
    {
		/* APP 录音落到 43/{alarm_id}.bin（与本地 UI 一致，内容嗅探 AAC/MP3） */
		image_info.image_id = (uint32)alarm_info->alarm_id;
		strcpy(image_info.file_addr, alarm_info->voice_id);
		image_info.flag = 1;
		image_info.pcallback = divoom_app_com_finishcallback;
		divoom_manage_image_add_font(DIVOOM_MANAGE_IMAGE_ALARM_RECORD, &image_info, 1);
    }
}


//返回0不需要读取，其他需要
uint8 divoom_alarm_get_alram_info(void)
{
    divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_ALARM, DIVOOM_SERVER_TOPIC, NULL);
	return 0;
}


T_U8 divoom_alarm_lib_compare_local_time(Pdivoom_alarm_system_date time1, Pdivoom_alarm_system_date time2)
{
    if(time1->year> time2->year)
    {
        return 1;
    }
    else if(time1->year < time2->year)
    {
        return 0;
    }
    if(time1->month> time2->month)
    {
        return 1;
    }
    else if(time1->month < time2->month)
    {
        return 0;
    }
    if(time1->day > time2->day)
    {
        return 1;
    }
    else if(time1->day < time2->day)
    {
        return 0;
    }
    if(time1->hour> time2->hour)
    {
        return 1;
    }
    else if(time1->hour < time2->hour)
    {
        return 0;
    }
    if(time1->minute> time2->minute)
    {
        return 1;
    }
    else if(time1->minute < time2->minute)
    {
        return 0;
    }
    if(time1->second> time2->second)
    {
        return 1;
    }
    else if(time1->second < time2->second)
    {
        return 0;
    }
    return 2;
}

T_VOID divoom_alarm_lib_ctrl_add_days(Pdivoom_alarm_system_date cur_time, T_U8 days)
{
    T_U32 all_second;
    
	//divoom_com_print("divoom_com_time_to_sec(cur_time) = %d\n", divoom_com_time_to_sec(cur_time));
	//divoom_com_print("cur_time:%d;%d:%d:%d:%d\n", cur_time->year, cur_time->month, cur_time->day, cur_time->hour, cur_time->minute);
    all_second = divoom_alarm_time_to_sec(cur_time) + days * (24*3600);
	//divoom_com_print("all_second = %d\n", all_second);
    divoom_alarm_sec_to_date(cur_time, all_second);
	//divoom_com_print("cur_time:%d;%d:%d:%d:%d\n", cur_time->year, cur_time->month, cur_time->day, cur_time->hour, cur_time->minute);
	//divoom_com_print("divoom_com_time_to_sec(cur_time) = %d\n", divoom_com_time_to_sec(cur_time));
}

T_U8 divoom_alarm_power_on_status(DIVOOM_ALARM_STATUS flag, T_U8 status)
{
	(void)status;
	if(flag == DIVOOM_ALARM_GET_POWERON_STATUS)
	{
		/* RTC 闹钟开机：过点仍算这一次，避免换算跳到下一周/明天 */
		return (divoom_main_get_poweron_mode() == DIVOOM_ONOFF_MODE_CLOCK) ? 1 : 0;
	}
	else if(flag == DIVOOM_ALARM_SET_POWERON_STATUS)
	{
		return 1;
	}

	return 1;
}


T_VOID divoom_alarm_lib_convert_alarm_time_to_local(T_DIVOOM_ALARM_ACTIVE_DO_INFO *palarm_info, Pdivoom_alarm_system_date local_time, Pdivoom_alarm_system_date cur_time)
{
    T_U32 today_second, tmp_time;

    memcpy(local_time, cur_time, sizeof(divoom_alarm_system_date));
	today_second = divoom_alarm_get_today_sec();
	today_second %= 60*60*24;
    tmp_time = (T_U32)palarm_info->hour * 3600 + (T_U32)palarm_info->minute * 60;
	//divoom_com_print("week = %d, today_second = %d, tmp_time = %d\n", palarm_info->week_day, today_second, tmp_time);
    local_time->second = 0;
    local_time->hour = palarm_info->hour;
    local_time->minute = palarm_info->minute;
	//divoom_com_print("index:%d, local_time:%d:%d:%d:%d:%d\n", palarm_info->index, local_time->year, local_time->month, local_time->day, local_time->hour, local_time->minute);
	//divoom_com_print("cyc_flag:%d, cur_time->week:%d", palarm_info->cyc_flag, cur_time->week);
    if(palarm_info->cyc_flag)
    {
        int i;

        //如果今天闹钟有效，并且没有过时
        if(palarm_info->cyc_flag & (1 << cur_time->week))
        {
            if(today_second < tmp_time)
            {
                return;
            }
			//判断是不是同时刻的闹钟
			//避免更新成下一个闹钟的信息
			if(divoom_alarm_power_on_status(DIVOOM_ALARM_GET_POWERON_STATUS, 0) && ((today_second - tmp_time) < 60))
			{
				return;
			}
        }
        for(i = cur_time->week + 1; i < cur_time->week + 7; i ++)
        {
            if(palarm_info->cyc_flag & ( 1 << (i % 7)))
            {
                break;
            }
        }
		divoom_alarm_lib_ctrl_add_days(local_time, i - cur_time->week);  
    }
    else
    {
        if(today_second > tmp_time)
        {
			//避免更新成下一个闹钟的信息
			if(divoom_alarm_power_on_status(DIVOOM_ALARM_GET_POWERON_STATUS, 0) && ((today_second - tmp_time) < 60))
				return;
            divoom_alarm_lib_ctrl_add_days(local_time, 1);
        }
    }
}

void divoom_alarm_lib_convert_dialy_time_to_local(T_DIVOOM_ALARM_ACTIVE_DO_INFO *palarm_info, Pdivoom_alarm_system_date local_time, Pdivoom_alarm_system_date cur_time)
{
	T_U32 local_sec, cur_sec;

    local_time->year = cur_time->year;
    local_time->month = palarm_info->month;
    local_time->day = palarm_info->day;
    local_time->hour = palarm_info->hour;
    local_time->minute = palarm_info->minute;
    local_time->second = 0;

	//divoom_com_print("cur_time:%d:%d:%d:%d:%d\n", cur_time->year, cur_time->month, cur_time->day, cur_time->hour, cur_time->minute);
	//divoom_com_print("local_time:%d:%d:%d:%d:%d\n", local_time->year, local_time->month, local_time->day, local_time->hour, local_time->minute);
	if(cur_time->second == 0)
	{
		//避免在上一次获取到的时间是0秒,导致下面的年不会++
		cur_time->second += 5;
	}

	local_sec = divoom_alarm_time_to_sec(local_time);
	cur_sec = divoom_alarm_time_to_sec(cur_time);
	//divoom_com_print("local_sec:%d, cur_sec:%d\n", local_sec, cur_sec);

	if(local_sec < cur_sec)
	{
		//判断是不是同时刻的纪念日唤醒开机,唤醒开机避免更新成下一个闹钟的信息
		if(divoom_alarm_power_on_status(DIVOOM_ALARM_GET_POWERON_STATUS, 1) && ((cur_sec - local_sec) < 60))
		{
			divoom_print("THIS IS POWERON_ALARM & dialy!!!\n");
			return;
		}
		local_time->year ++;
	}
}


T_U8 divoom_alarm_lib_compare_alarm_time(T_DIVOOM_ALARM_ACTIVE_DO_INFO *palarm_info1, T_DIVOOM_ALARM_ACTIVE_DO_INFO *palarm_info2, Pdivoom_alarm_system_date cur_time, Pdivoom_alarm_system_date active_time)
{
	divoom_alarm_system_date local_time1;
	divoom_alarm_system_date local_time2;
	T_U8 ret;

    divoom_alarm_lib_convert_alarm_time_to_local(palarm_info1, &local_time1, cur_time);
    divoom_alarm_lib_convert_alarm_time_to_local(palarm_info2, &local_time2, cur_time);
	//divoom_com_print("index:%d, local_time1:%d:%d:%d:%d:%d\n", palarm_info1->index, local_time1.year, local_time1.month, local_time1.day, local_time1.hour, local_time1.minute);
	//divoom_com_print("index;%d, local_time2:%d:%d:%d:%d:%d\n", palarm_info2->index, local_time2.year, local_time2.month, local_time2.day, local_time2.hour, local_time2.minute);
    ret = divoom_alarm_lib_compare_local_time(&local_time1, &local_time2);

	if(ret == 1)
	{
		memcpy(active_time, &local_time2, sizeof(divoom_alarm_system_date));
	}
	else
	{
		memcpy(active_time, &local_time1, sizeof(divoom_alarm_system_date));
	}

	return ret;
}

T_U8 divoom_alarm_lib_compare_dialy_time(T_DIVOOM_ALARM_ACTIVE_DO_INFO *palarm_info1, T_DIVOOM_ALARM_ACTIVE_DO_INFO *palarm_info2, Pdivoom_alarm_system_date cur_time, Pdivoom_alarm_system_date active_time)
{
	divoom_alarm_system_date local_time1;
	divoom_alarm_system_date local_time2;
	T_U8 ret;

    divoom_alarm_lib_convert_dialy_time_to_local(palarm_info1, &local_time1, cur_time);
    divoom_alarm_lib_convert_dialy_time_to_local(palarm_info2, &local_time2, cur_time);
    ret = divoom_alarm_lib_compare_local_time(&local_time1, &local_time2);
	//divoom_com_print("cur_time:%d:%d:%d:%d:%d:%d\n", cur_time->year, cur_time->month, cur_time->day, cur_time->hour, cur_time->minute, cur_time->second);
	//divoom_com_print("local_time:%d:%d:%d:%d:%d:%d\n", local_time1.year, local_time1.month, local_time1.day, local_time1.hour, local_time1.minute, local_time1.second);
	//divoom_com_print("local_time2.year = %d:%d:%d:%d:%d:%d\n", local_time2.year, local_time2.month, local_time2.day, local_time2.hour, local_time2.minute, local_time2.second);
	//divoom_com_print("ret = %d\n", ret);
	if(ret == 1)
    {
        memcpy(active_time, &local_time2, sizeof(divoom_alarm_system_date));
    }
    else
    {
        memcpy(active_time, &local_time1, sizeof(divoom_alarm_system_date));
    }
	return ret;
}

T_S32 divoom_alarm_get_active_alarm(void)
{
    return pdivoom_alarm_handle->next_active_id;
}

T_VOID divoom_alarm_lib_set_next_active_alarm(void)
{
    T_U8 i, find_alarm_index = 0xFF, find_dialy_index = 0XFF/*, alarm_cyc_flag = 0, dialy_cyc_flag= 0*/;
    divoom_alarm_system_date cur_time, alarm_time, dialy_time;
	T_DIVOOM_ALARM_ACTIVE_DO_INFO *alarm_info, find_info, *dialy_info, find_dialy_info;
    char new_image_id[52];
	T_U8 ret = 0;

	memset(&find_info, 0, sizeof(find_info));
	memset(&find_dialy_info, 0, sizeof(find_dialy_info));

    pdivoom_alarm_handle->next_active_id = -1;
    pdivoom_alarm_handle->next_active_light_time = 0;
    divoom_alarm_get_sys_time(&cur_time);
	divoom_print("cur_time:%d:%d:%d:%d:%d", cur_time.year + 1900, cur_time.month + 1, cur_time.day, cur_time.hour, cur_time.minute);
    //先得到最近的一个闹钟的信息
    pdivoom_alarm_handle->last_alarm_info = NULL;
    for(i = 0; i < divoom_queue_items(pdivoom_alarm_handle->dialy_alarm_queue); i ++)
    {
        alarm_info = divoom_queue_get_index_item(pdivoom_alarm_handle->dialy_alarm_queue, i);
        if(alarm_info == NULL)
        {
            break;
        }
        //divoom_print("alarm = %d ,%d:%d, %d, %d\n", alarm_info->alarm_id, alarm_info->hour, alarm_info->minute, alarm_info->cyc_flag, alarm_info->on_off);
        if(alarm_info->on_off != 1 || alarm_info->type != DIVOOM_ALARM_TYPE_ALARM)
        {
            continue;
        }
       
        if(alarm_info->on_off && alarm_info->cyc_flag == 0)
        {
            /* RTC 闹钟刚把机器唤醒时，开机往往已晚于闹钟点，不能按「过期」关掉，否则永远进不了闹钟页 */
            if (!pdivoom_alarm_handle->rtc_clock_skip_expire_once) {
            //如果是过期的闹钟，将取消
            alarm_time.year = alarm_info->year;
            alarm_time.month = alarm_info->month;
            alarm_time.day = alarm_info->day;
            alarm_time.hour = alarm_info->hour;
            alarm_time.minute = alarm_info->minute;
            alarm_time.second = 0;
            if(divoom_alarm_lib_compare_local_time(&cur_time, &alarm_time))
            {
                alarm_info->on_off = 0;
                divoom_print("we will close the alarm!\n");
                continue;
            }
            }
        } 
        if(find_alarm_index == 0xFF)
        {
            find_alarm_index = i;
            memcpy(&find_info, alarm_info, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
            divoom_alarm_lib_convert_alarm_time_to_local(&find_info, &alarm_time, &cur_time);
            continue;
        }
        
		ret = divoom_alarm_lib_compare_alarm_time(&find_info, alarm_info, &cur_time, &alarm_time);
        if(ret == 2)//时间相同的闹钟
        {
		}
		else if(ret == 1)
		{
			find_alarm_index = i;
			memcpy(&find_info, alarm_info, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
		}
    }

    if(find_alarm_index != 0xFF)
    {
        pdivoom_alarm_handle->last_alarm_info =
            divoom_queue_get_index_item(pdivoom_alarm_handle->dialy_alarm_queue, find_alarm_index);
    }
    else
    {
        pdivoom_alarm_handle->last_alarm_info = NULL;
    }

	divoom_print("find_alarm_index = %d\n", find_alarm_index);
	if (find_alarm_index != 0xFF)
		divoom_print("find_info.hour = %d, find_info.minute = %d, \n", find_info.hour, find_info.minute);
    //再得到最近的一个纪念日信息

    for(i = 0; i < divoom_queue_items(pdivoom_alarm_handle->dialy_alarm_queue); i ++)
    {

        dialy_info = divoom_queue_get_index_item(pdivoom_alarm_handle->dialy_alarm_queue, i);
        if(dialy_info == NULL)
        {
            break;
        }
        if(dialy_info->on_off != 1 || dialy_info->type != DIVOOM_ALARM_TYPE_DIALY)
        {
            continue;
        }

        if(find_dialy_index == 0xFF)
        {
            find_dialy_index = i;
            memcpy(&find_dialy_info, dialy_info, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
            divoom_alarm_lib_convert_dialy_time_to_local(&find_dialy_info, &dialy_time, &cur_time);
            continue;
        }
        ret = divoom_alarm_lib_compare_dialy_time(&find_dialy_info, dialy_info, &cur_time, &dialy_time);
		if(ret == 2)
        {
			find_dialy_index = i;
			memcpy(&find_dialy_info, dialy_info, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
        }
		else if( ret == 1)
		{
			find_dialy_index = i;
			memcpy(&find_dialy_info, dialy_info, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
		}
    }


    if(find_dialy_index == 0xFF && find_alarm_index == 0XFF)
    {
        //没有激活的闹钟信息
        divoom_print("%s: all alarm and dialy is unactive!\n", __func__);
        pdivoom_alarm_handle->last_alarm_info = NULL;
        pdivoom_alarm_handle->next_active_time = 0;
        pdivoom_alarm_handle->next_active_id = -1;
		return;
    }
    else if(find_dialy_index != 0xFF && find_alarm_index != 0XFF)
    {
		ret = divoom_alarm_lib_compare_local_time(&alarm_time, &dialy_time);
        //比较闹钟和纪念日
		if(ret == 2)
		{
            pdivoom_alarm_handle->next_active_id = find_alarm_index;
            pdivoom_alarm_handle->next_active_time = divoom_alarm_time_to_sec(&alarm_time);
            strcpy(new_image_id, find_info.image_data);
		}
        else if(ret == 1)
        {
            pdivoom_alarm_handle->next_active_id = find_dialy_index;
            pdivoom_alarm_handle->next_active_time = divoom_alarm_time_to_sec(&dialy_time);
            strcpy(new_image_id, find_dialy_info.image_data);

		}
        else
        {
            pdivoom_alarm_handle->next_active_id = find_alarm_index;
            pdivoom_alarm_handle->next_active_time = divoom_alarm_time_to_sec(&alarm_time);
            strcpy(new_image_id, find_info.image_data);
        }
    }
    else if(find_dialy_index == 0xFF)
    {
        pdivoom_alarm_handle->next_active_id = find_alarm_index;
        pdivoom_alarm_handle->next_active_time = divoom_alarm_time_to_sec(&alarm_time);
		strcpy(new_image_id, find_info.image_data);
	}
    else
    {
        pdivoom_alarm_handle->next_active_id = find_dialy_index;
        pdivoom_alarm_handle->next_active_time = divoom_alarm_time_to_sec(&dialy_time);
        strcpy(new_image_id, find_dialy_info.image_data);
	}
    alarm_info = divoom_queue_get_index_item(pdivoom_alarm_handle->dialy_alarm_queue, pdivoom_alarm_handle->next_active_id);
    
    pdivoom_alarm_handle->next_active_light_time = pdivoom_alarm_handle->next_active_time;
    if(strcmp(alarm_info->image_data, pdivoom_alarm_handle->next_image_file))
    {
        if(pdivoom_alarm_handle->next_image_data)
        {
            divoom_system_free(pdivoom_alarm_handle->next_image_data);
            pdivoom_alarm_handle->next_image_data = NULL;
        }
       strcpy(pdivoom_alarm_handle->next_image_file, alarm_info->image_data);
    }
    pdivoom_alarm_handle->early_ctrl_flag = DIVOOM_ALARM_EARLY_COLOR_WAIT;
    pdivoom_alarm_handle->read_image_flag = 0;
}


void divoom_alarm_update_alarm_list_info(void *data)
{
	int i = 0;
	Pdivoom_net_alarm_get_alarm_item alarm_info = (Pdivoom_net_alarm_get_alarm_item)data;    
    
    if(alarm_info == NULL)
    {
        return;
    }
    divoom_alarm_unactive_all_alarm(DIVOOM_ALARM_TYPE_ALARM);    

	for(i=0;i<alarm_info->alarm_num;i++)
	{
        divoom_alarm_update_one_alarm_info(&alarm_info->alarm_list[i]);
	}
	//移除服务器上已经删除的闹钟
    divoom_alarm_remove_unactive_alarm();
    divoom_alarm_lib_set_next_active_alarm();    
}

void divoom_alarm_update_alarm_item_info(void *data)
{
	int i = 0;
	Pdivoom_net_alarm_get_alarm_item alarm_info = (Pdivoom_net_alarm_get_alarm_item)data;    
    
    if(alarm_info == NULL)
    {
        return;
    }
    
	for(i=0;i<alarm_info->alarm_num;i++)
	{
        divoom_alarm_update_one_alarm_info(&alarm_info->alarm_list[i]);
	}
    
    divoom_alarm_lib_set_next_active_alarm();    
}


/*********************************************************************
  Function:        divoom_alarm_menu_get_alarm_list
  Description:     获取闹钟列表，这个列表返回使用后要释放
  Input:           alarm_num  闹钟个数  
  Return:          返回闹钟列表指针
**********************************************************************/

T_DIVOOM_ALARM_ACTIVE_DO_INFO *divoom_alarm_menu_get_alarm_list(int *alarm_num)
{
    int i;
    T_DIVOOM_ALARM_ACTIVE_DO_INFO *alarm_info, *palarm_list;

    (*alarm_num) = 0;
    for(i = 0; i < divoom_queue_items(pdivoom_alarm_handle->dialy_alarm_queue); i ++)
    {
        alarm_info = divoom_queue_get_index_item(pdivoom_alarm_handle->dialy_alarm_queue, i);
        if(alarm_info == NULL)
        {
            break;
        }
        //divoom_print("alarm = %d ,%d:%d, %d, %d\n", alarm_info->alarm_id, alarm_info->hour, alarm_info->minute, alarm_info->cyc_flag, alarm_info->on_off);
        //if(alarm_info->on_off != 1 || alarm_info->type != DIVOOM_ALARM_TYPE_ALARM)
        //{
        //    continue;
        //}
		if(alarm_info->type != DIVOOM_ALARM_TYPE_ALARM)
        {
            continue;
        }
        (*alarm_num) ++;

    }

    (*alarm_num) ++;
    palarm_list = divoom_system_malloc((*alarm_num) * sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
	(*alarm_num) = 0;
    for(i = 0; i < divoom_queue_items(pdivoom_alarm_handle->dialy_alarm_queue); i ++)
    {
        alarm_info = divoom_queue_get_index_item(pdivoom_alarm_handle->dialy_alarm_queue, i);
        if(alarm_info == NULL)
        {
            break;
        }
        //divoom_print("alarm = %d ,%d:%d, %d, %d\n", alarm_info->alarm_id, alarm_info->hour, alarm_info->minute, alarm_info->cyc_flag, alarm_info->on_off);
        //if(alarm_info->on_off != 1 || alarm_info->type != DIVOOM_ALARM_TYPE_ALARM)
        //{
        //    continue;
        //}
		if(alarm_info->type != DIVOOM_ALARM_TYPE_ALARM)
        {
            continue;
        }
        memcpy(&palarm_list[*alarm_num], alarm_info, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
        (*alarm_num) ++;       
    }

    return palarm_list;
}

/*********************************************************************
  Function:        divoom_alarm_menu_convert_local_to_net
  Description:     将本地的闹钟信息转换成发送给服务器的相关信息
  Input:           palarm_info  
                   pnet_alarm_info
  Return:          NONE
**********************************************************************/


void divoom_alarm_menu_convert_local_to_net(T_DIVOOM_ALARM_ACTIVE_DO_INFO *palarm_info, divoom_net_alarm_set_alarm_info *pnet_alarm_info)
{
    int i;
    divoom_alarm_system_date cur_time;
    
    divoom_alarm_get_sys_time(&cur_time);
    cur_time.hour = palarm_info->hour;
    cur_time.minute = palarm_info->minute;
    cur_time.second = 0;
	pnet_alarm_info->alarm_id = palarm_info->alarm_id;	
	pnet_alarm_info->utc_time = divoom_alarm_time_to_sec(&cur_time);
    if(pnet_alarm_info->utc_time < time(0))
    {
        pnet_alarm_info->utc_time += 24*3600;
    }
    pnet_alarm_info->on_off = palarm_info->on_off;
    pnet_alarm_info->volume = palarm_info->volume;
    pnet_alarm_info->sound_type  = palarm_info->mode;
    pnet_alarm_info->sound_system = palarm_info->tip_index;
	pnet_alarm_info->cyc[6] = (palarm_info->cyc_flag & 0x01);
    for(i = 0;i < 6; i++)
    {
    	if(palarm_info->cyc_flag & (1 << (i+1)))
        {
            pnet_alarm_info->cyc[i] = 1;
        }
        else
        {
            pnet_alarm_info->cyc[i] = 0;
        }
    }
    //for(i = 0; i < 7; i++)
    //{
    //    if(palarm_info->cyc_flag & (1 << i))
    //    {
    //        pnet_alarm_info->cyc[i] = 1;
    //    }
    //    else
    //    {
    //        pnet_alarm_info->cyc[i] = 0;
    //    }
    //}
    strcpy(pnet_alarm_info->alarm_name, palarm_info->alarm_name);
	strcpy(pnet_alarm_info->image_id, palarm_info->image_data);
    strcpy(pnet_alarm_info->voice_id, palarm_info->leave_url);
}

/*********************************************************************
  Function:        divoom_alarm_menu_update_alarm_item
  Description:     更新某个闹钟的信息
  Input:           palarm_info   
  Return:          NONE
**********************************************************************/


void divoom_alarm_menu_update_alarm_item(T_DIVOOM_ALARM_ACTIVE_DO_INFO *palarm_info)
{
    T_DIVOOM_ALARM_ACTIVE_DO_INFO *pcur_alarm;
    divoom_net_alarm_set_alarm_info app_alarm_info;
	divoom_alarm_system_date cur_time;

#if DIVOOM_SHOW_MODE
    divoom_alarm_show_mode_strip_week_cyc(palarm_info);
#endif

    divoom_alarm_get_sys_time(&cur_time);
	
    pcur_alarm = divoom_queue_get_cmp(pdivoom_alarm_handle->dialy_alarm_queue, &palarm_info->alarm_id, divoom_alarm_find_alarm_compare);
    if(pcur_alarm)
    {
        if(pcur_alarm != palarm_info)
        {
        	if(cur_time.hour > palarm_info->hour ||
				 (cur_time.hour == palarm_info->hour && cur_time.minute >= palarm_info->minute))
        	{	
        		//过期的时间
				cur_time.hour = palarm_info->hour;
        		cur_time.minute = palarm_info->minute;
        		cur_time.second = 0;
				divoom_alarm_lib_ctrl_add_days(&cur_time, 1);
				
			}

			palarm_info->year = cur_time.year;
			palarm_info->month = cur_time.month;
			palarm_info->day = cur_time.day;
			
			memcpy(pcur_alarm, palarm_info, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
        }
		 
        divoom_alarm_menu_convert_local_to_net(palarm_info, &app_alarm_info);
		divoom_net_app_send_packet(DIVOOM_NET_COMM_UPDATE_ALARM, DIVOOM_SERVER_TOPIC, &app_alarm_info);
		divoom_alarm_lib_set_next_active_alarm(); 
		divoom_alarm_save_all_alarm_info();
    }
}

/*********************************************************************
  Function:        divoom_alarm_menu_delete_alarm
  Description:     删除某个闹钟
  Input:           alarm_id   
  Return:          NONE
**********************************************************************/


void divoom_alarm_menu_delete_alarm(int alarm_id)
{
    T_DIVOOM_ALARM_ACTIVE_DO_INFO *pcur_alarm;
    
    pcur_alarm = divoom_queue_pop_cmp(pdivoom_alarm_handle->dialy_alarm_queue, &alarm_id, divoom_alarm_find_alarm_compare);
    if(pcur_alarm)
    {
        //先通知服务器删除再释放资源       
        divoom_net_app_send_packet(DIVOOM_NET_COMM_DEL_ALARM, DIVOOM_SERVER_TOPIC, &alarm_id);
        divoom_system_free(pcur_alarm);
        divoom_alarm_lib_set_next_active_alarm();    
    }
}

/*********************************************************************
  Function:        divoom_alarm_menu_add_alarm_item
  Description:     增加一个闹钟
  Input:           alarm_id   
  Return:          NONE
**********************************************************************/


void divoom_alarm_menu_add_alarm_item(T_DIVOOM_ALARM_ACTIVE_DO_INFO *palarm_info)
{
	Pdivoom_net_command ret_data = NULL;
    divoom_net_alarm_set_alarm_info app_alarm_info;

#if DIVOOM_SHOW_MODE
    divoom_alarm_show_mode_strip_week_cyc(palarm_info);
#endif

    palarm_info->alarm_id = 0;
    divoom_alarm_menu_convert_local_to_net(palarm_info, &app_alarm_info);

    ret_data = divoom_net_app_send_packet_wait(DIVOOM_NET_COMM_UPDATE_ALARM, DIVOOM_SERVER_TOPIC, &app_alarm_info);
    if(ret_data)
    {
        T_DIVOOM_ALARM_ACTIVE_DO_INFO *pnew_item = divoom_system_malloc(sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
        
        palarm_info->alarm_id = *(int *)ret_data->cmd_data;
        divoom_net_app_free_packet(ret_data);
        memcpy(pnew_item, palarm_info, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
        pnew_item->type = DIVOOM_ALARM_TYPE_ALARM;
        pnew_item->active_status = 1;
        divoom_queue_push(pdivoom_alarm_handle->dialy_alarm_queue, pnew_item);
    }
}

void divoom_alarm_update_dialy_one_info(Pdivoom_net_memorial_info memorial_item)
{
    uint32 img_id;
    divoom_manage_image_item_info image_item;
    T_DIVOOM_ALARM_ACTIVE_DO_INFO *data_item;

    
    data_item = divoom_queue_get_cmp(pdivoom_alarm_handle->dialy_alarm_queue, &memorial_item->memorial_id, divoom_alarm_find_dialy_compare);
    if(data_item)
    {
        if(data_item->update_time != memorial_item->update_time)
        {
            //纪念日有更新，将更新
            if(strcmp(data_item->image_data, memorial_item->image_data))
            {
                //先删除老文件
                img_id = DIVOOM_MANAGE_IMAGE_ID_BASE_DIALY_GIF + memorial_item->memorial_id;
                divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_ALARM_GIF, &img_id, 1);
            }
            data_item->month = memorial_item->mon - 1;
            data_item->day = memorial_item->day;
            data_item->hour =memorial_item->min / 60;
            data_item->minute = memorial_item->min % 60; 
            data_item->update_time = memorial_item->update_time;
            strcpy(data_item->image_data, memorial_item->image_data);
            strcpy(data_item->alarm_name, memorial_item->name);
            
        }
        data_item->active_status = 1;
    }
    else
    {
        //新增加的闹钟先删除文件
        img_id = DIVOOM_MANAGE_IMAGE_ID_BASE_DIALY_GIF + memorial_item->memorial_id;
        divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_ALARM_GIF, &img_id, 1);
        data_item = divoom_system_malloc(sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
        
        data_item->month = memorial_item->mon - 1;
        data_item->day = memorial_item->day;
        data_item->hour = memorial_item->min / 60;
        data_item->minute = memorial_item->min % 60; 
        data_item->update_time = memorial_item->update_time;
        strcpy(data_item->image_data, memorial_item->image_data);
        strcpy(data_item->alarm_name, memorial_item->name);
        data_item->active_status = 1;
        data_item->alarm_id = memorial_item->memorial_id;
        data_item->early_flag = 0;
        data_item->on_off = 1;
        data_item->type = DIVOOM_ALARM_TYPE_DIALY;
        divoom_queue_push(pdivoom_alarm_handle->dialy_alarm_queue, data_item);
    }

	if(strlen(data_item->image_data))
    {
    	image_item.flag = 1;
    	image_item.image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_DIALY_GIF + memorial_item->memorial_id;
    	strcpy(image_item.file_addr, data_item->image_data);
    	image_item.pcallback = divoom_app_com_finishcallback;
    	divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_ALARM_GIF,&image_item, 1);
	}
}

void divoom_alarm_update_dialy_item_info(void *data)
{
	int i = 0;
	Pdivoom_get_memorial_item_info memorial_info = (Pdivoom_get_memorial_item_info)data;
    

    
	for(i=0;i<memorial_info->memorial_num;i++)
	{
        divoom_alarm_cancel_by_id(memorial_info->memorial_list[i].memorial_id, DIVOOM_ALARM_TYPE_DIALY);
        divoom_alarm_update_dialy_one_info(&memorial_info->memorial_list[i]);
	}
    divoom_alarm_lib_set_next_active_alarm();
	return;
}

void divoom_alarm_update_dialy_list_info(void *data)
{
	int i = 0;
	Pdivoom_get_memorial_item_info memorial_info = (Pdivoom_get_memorial_item_info)data;

    divoom_alarm_unactive_all_alarm(DIVOOM_ALARM_TYPE_DIALY);
    
	for(i=0;i<memorial_info->memorial_num;i++)
	{
        divoom_alarm_update_dialy_one_info(&memorial_info->memorial_list[i]);
	}
	//移除服务器上已经删除的闹钟
    divoom_alarm_remove_unactive_alarm();
    divoom_alarm_lib_set_next_active_alarm();
	return;
}



uint8 divoom_alarm_get_alram_update_list_info(void)
{
    divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_ALARM, DIVOOM_SERVER_TOPIC, NULL);
	return 0;
}

uint8 divoom_alarm_get_dialay_update_list_info(void)
{
    //divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_MEMORIAL, DIVOOM_SERVER_TOPIC, NULL);
	return 0;
}



uint8 divoom_alarm_event(Pdivoom_sys_msg_info msg)
{
    if(pdivoom_alarm_handle == NULL)
    {
        return DIVOOM_MODEL_NO_INIT;
    }
    divoom_print("divoom_alarm_event msg_id = %d\n", msg->msg_id);
	divoom_system_get_lock(pdivoom_alarm_handle->divoom_alarm_task_mux);
    switch(msg->msg_id)
    {
        case DIVOOM_DEVICE_SYSTEM_RESET_ALL:
            //先删除开机动画，再向系统获取相关配置
            divoom_app_com_del_file(DIVOOM_ALARM_INFO_PATH);
            divoom_manage_image_remove(DIVOOM_MANAGE_IMAGE_ALARM_GIF, 0);
            break;
            
        case DIVOOM_DEVICE_EVENT_TIME_INIT_OK:
            divoom_alarm_lib_set_next_active_alarm();
            divoom_power_control_on_time_sync();
            break;

            
		case DIVOOM_ALARM_GET_NEXT_ALARM_TIME:
			{
                //我们将提前开机
                
                T_DIVOOM_ALARM_ACTIVE_DO_INFO   *next_alarm_info;           //下一个闹钟到点的数据信息
                
                if(pdivoom_alarm_handle->next_active_id != -1)
                {
                    int early_time = 0;
                    next_alarm_info = divoom_queue_get_index_item(pdivoom_alarm_handle->dialy_alarm_queue, pdivoom_alarm_handle->next_active_id);
                    if(next_alarm_info->early_flag)
                    {
                        early_time = DIVOOM_ALARM_HW_WAKE_EARLY_HEAVY_SEC;
                    }
                    else
                    {
                        early_time = DIVOOM_ALARM_HW_WAKE_EARLY_SEC;
                    }
                    if(pdivoom_alarm_handle->next_active_time > time(0) + early_time)
                    {
                        divoom_print("we will start up with early : %d\n", early_time);
                        msg->result = (uint32 )pdivoom_alarm_handle->next_active_time - early_time; 
                    }
                    else
                    {
                        divoom_print("we will start up with early after 10s\n");
                        msg->result =  time(0) + 10;
                    }
                }
                else
                {
                    msg->result = 0;
                }
			}
			break;
        
		case DIVOOM_DEVICE_CON_SRV_ID:
			divoom_alarm_get_alram_update_list_info();
			divoom_alarm_get_dialay_update_list_info();
            break;
            
        case DIVOOM_ALARM_UPDATE_ALARM:        //闹钟更新
			divoom_alarm_update_alarm_list_info(msg->data);
			divoom_alarm_print_link_item(DIVOOM_ALARM_TYPE_ALARM);
            if((pdivoom_alarm_handle->first_load_flag & (1 << DIVOOM_ALARM_TYPE_ALARM)) == 0)
            {
                pdivoom_alarm_handle->first_load_flag |= (1 << DIVOOM_ALARM_TYPE_ALARM);
            }
            else
            {
                //divoom_main_save_notice_dis();
            }
			divoom_alarm_save_all_alarm_info();
            break;

        case DIVOOM_ALARM_SET_ALARM:
            divoom_alarm_update_alarm_item_info(msg->data);
            divoom_main_save_notice_dis();
			divoom_alarm_save_all_alarm_info();
            break;

        
        case DIVOOM_ALARM_DEL_ALARM:
			{
				T_DIVOOM_ALARM_ACTIVE_DO_INFO *alarm_info;
                
                divoom_main_save_notice_dis();
                Pdivoom_net_alarm_set_alarm_respone_info palarm_info = (Pdivoom_net_alarm_set_alarm_respone_info)msg->data;
				divoom_print("DIVOOM_ALARM_DEL_ALARM: id: %d",*(int *)msg->data);
				divoom_alarm_cancel_by_id(palarm_info->alarm_id, DIVOOM_ALARM_TYPE_ALARM);
                alarm_info = divoom_queue_pop_cmp(pdivoom_alarm_handle->dialy_alarm_queue, &palarm_info->alarm_id, divoom_alarm_find_alarm_compare);
                if (alarm_info != NULL)
                {
                    divoom_system_free(alarm_info);
                }
                divoom_alarm_lib_set_next_active_alarm();
                divoom_alarm_save_all_alarm_info();
			}
            break;
            
        case DIVOOM_ALARM_DEL_MEMORIAL:
        {
		    T_DIVOOM_ALARM_ACTIVE_DO_INFO *dialy_info;
            Pdivoom_net_memorial_del_memorial_info pmem_info = (Pdivoom_net_memorial_del_memorial_info)msg->data;
            uint32 image_id;
            divoom_print("DIVOOM_ALARM_DEL_MEMORIAL: id: %d\n",pmem_info->memorial_id);
			divoom_alarm_cancel_by_id(pmem_info->memorial_id, DIVOOM_ALARM_TYPE_DIALY);
            divoom_main_save_notice_dis();
            
            dialy_info = divoom_queue_pop_cmp(pdivoom_alarm_handle->dialy_alarm_queue, &pmem_info->memorial_id, divoom_alarm_find_dialy_compare);
            if (dialy_info != NULL)
            {
                image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_ALARM_GIF + dialy_info->alarm_id;
                divoom_manage_image_del(DIVOOM_MANAGE_IMAGE_ALARM_GIF, &image_id, 1, 0);
                divoom_system_free(dialy_info);
            }
            divoom_alarm_lib_set_next_active_alarm();
			divoom_alarm_save_all_alarm_info();
        }
            break;
		
		case DIVOOM_ALARM_LISTEN_ALARM:
		{
			Pdivoom_net_alarm_listen_alarm_respone_info info = (Pdivoom_net_alarm_listen_alarm_respone_info)msg->data;
			divoom_alarm_ctrl_try_listen(info->sound_type, info->volume, info->start_flag, 0);
		}
			break;
            
        case DIVOOM_ALARM_UPDATE_MEMORIAL:        //纪念日更新
            if((pdivoom_alarm_handle->first_load_flag & (1 << DIVOOM_ALARM_TYPE_DIALY)) == 0)
            {
                pdivoom_alarm_handle->first_load_flag |= (1 << DIVOOM_ALARM_TYPE_DIALY);
            }
            else
            {
                //divoom_main_save_notice_dis();
            }
			divoom_alarm_update_dialy_list_info(msg->data);
			divoom_print("PRINT MEMORIAL BEGIN!!!!!!\n");
			divoom_alarm_print_link_item(DIVOOM_ALARM_TYPE_DIALY);
			divoom_print("PRINT MEMORIAL END!!!!!!\n");
			divoom_alarm_save_all_alarm_info();
            break;
            
            
        case DIVOOM_ALARM_SET_MEMORIAL:        //纪念日更新
            divoom_main_save_notice_dis();
			divoom_alarm_update_dialy_item_info(msg->data);
			divoom_alarm_save_all_alarm_info();
            break;
            
            
            
        case DIVOOM_ALARM_GET_STATUS:
            break;
            
            
        case DIVOOM_ALARM_CANCEL:
            break;


        case DIVOON_ALARM_END:
            break;

		case DIVOOM_ALARM_LISTEN_ALARM_SET_VOLUMN:
			divoom_alarm_ctrl_set_vol(*(int *)msg->data);
			break;

		case DIVOOM_ALARM_TF_FALLBACK_TONE:
			if (pdivoom_alarm_handle->alarm_status == DIVOOM_ALARM_STATUS_ACTIVE
			    && pdivoom_alarm_handle->cur_alarm_info != NULL)
			{
				divoom_alarm_play_builtin_fallback_tone(pdivoom_alarm_handle,
					pdivoom_alarm_handle->cur_alarm_info->volume,
					"tf 0x5e timeout");
			}
			break;

		case DIVOOM_APP_CONFIG_SAVE_EVENT_ID:
			//保存链表里面的全部信息
			divoom_alarm_save_all_alarm_info();
            break;
            
        case DIVOOM_APP_KEY_EVENT_ID:
			//在贪睡时或者闹钟激活时，闹钟无条件取消
			if((pdivoom_alarm_handle->alarm_status != DIVOOM_ALARM_STATUS_WAIT))
			{
				divoom_alarm_cancel_alarm();
			}
            break;
    }
    divoom_system_free_lock(pdivoom_alarm_handle->divoom_alarm_task_mux);

    return DIVOOM_SUCCESS;
}

void divoom_alarm_cancel_by_id(int index, uint8_t type)
{
	if((pdivoom_alarm_handle->alarm_status != DIVOOM_ALARM_STATUS_WAIT))
	{
		if((pdivoom_alarm_handle->cur_alarm_info && pdivoom_alarm_handle->cur_alarm_info->alarm_id == index && pdivoom_alarm_handle->cur_alarm_info->type == type))
		{
			divoom_alarm_cancel_alarm();
		}
	}
}

void divoom_alarm_cancel_now_alarm(void)
{
	divoom_alarm_stop_voice(pdivoom_alarm_handle);
    
	divoom_alarm_cancel_alarm();
    

	pdivoom_alarm_handle->active_status = 0;
	pdivoom_alarm_handle->cur_alarm_info = NULL;
	pdivoom_alarm_handle->alarm_status = DIVOOM_ALARM_STATUS_WAIT;
}

//闹钟一般不需要继续运行吧
uint8 divoom_alarm_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type)
{    
    switch(event_id)
    {
        case DIVOOM_ALARM_APP_EVENT_ALARM:
			if(pdivoom_alarm_handle->cur_alarm_info)
			{
				divoom_alarm_play_voice(pdivoom_alarm_handle, DIVOOM_ALARM_ACTIVE_TIME);
				divoom_alarm_disp_gif(pdivoom_alarm_handle);
				pdivoom_alarm_handle->play_vol = 1;
				pdivoom_alarm_handle->active_status = DIVOOM_ALARM_APP_EVENT_ALARM;
				pdivoom_alarm_handle->alarm_status = DIVOOM_ALARM_STATUS_ACTIVE;
			}
            break;
    }
    return 1;
}


uint8 divoom_alarm_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type)
{
	switch(event_id)
	{
		case DIVOOM_ALARM_APP_EVENT_ALARM:
			divoom_alarm_stop_voice(NULL);
			break;
        case DIVOOM_ALARM_APP_EVENT_LISTEN:
            /* 试听与系统 tip 一样只停 tone/duck，勿 pause 音乐 */
            divoom_local_music_ctrl_stop_tone();
            pdivoom_alarm_handle->active_status = 0;
            break;
	}
	return 1;
}

uint8 divoom_alarm_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type)
{
    divoom_sys_msg_info model_msg = {0};

    switch(event_id)
    {
        case DIVOOM_ALARM_APP_EVENT_LISTEN:
            divoom_local_music_ctrl_stop_tone();
            pdivoom_alarm_handle->active_status = 0;
            break;
            
        case DIVOOM_ALARM_APP_EVENT_ALARM:
			//stop voice
			pdivoom_alarm_handle->alarm_status = DIVOOM_ALARM_STATUS_WAIT;
			divoom_alarm_stop_voice(pdivoom_alarm_handle);
			pdivoom_alarm_handle->active_status = 0;
			pdivoom_alarm_handle->cur_alarm_info = NULL;

            //恢复默认亮度
            model_msg.src_id = DIVOOM_MODEL_ALARM;  
            model_msg.dest_id = DIVOOM_MODEL_MAIN;          
            model_msg.msg_id = DIVOOM_MAIN_EVENT_RESTORE_SYS_LIGHT; 
            divoom_sys_model_crl_msg_push(&model_msg);
			break;
	}
    return 1;
}

void divoom_alarm_disp_snooze(uint8_t index, T_U8 flag)
{
    divoom_disp_ctrl_alarm_info alarm_info = {0};
    
    pdivoom_alarm_handle->alarm_status = DIVOOM_ALARM_STATUS_SNOOZE;
    if(flag == 1)
    {
        alarm_info.status = 1;
    }
    else
    {
        alarm_info.status = 2;
    }

	alarm_info.image_id = 0;
    divoom_alarm_stop_voice(NULL);
	if(pdivoom_alarm_handle->cur_alarm_info->type != DIVOOM_ALARM_TYPE_ALARM)
    {
		//divoom_disp_ctrl_set_mode(DIVOOM_DISP_ALARM_MODE, &alarm_info);
	}
}

uint8 divoom_alarm_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type)
{
	(void)resouce_type;
	(void)key_type;

	/* APP 试听：无 cur_alarm_info，勿走 cancel_alarm（会空指针）；统一 preview_stop */
	if (pdivoom_alarm_handle->alarm_status == DIVOOM_ALARM_STATUS_TRY_LISTEN)
	{
		divoom_alarm_ctrl_preview_stop();
		return 1;
	}

	if(pdivoom_alarm_handle->alarm_status != DIVOOM_ALARM_STATUS_WAIT)
	{
        if(key_id == kbFUNC && pdivoom_alarm_handle->alarm_status == DIVOOM_ALARM_STATUS_ACTIVE)
        {
            divoom_system_get_lock(pdivoom_alarm_handle->divoom_alarm_task_mux);            
            pdivoom_alarm_handle->end_time = time(0) + DIVOOM_ALARM_RESTORE_TIME;            
            //显示贪睡图案--need
            divoom_alarm_disp_snooze(pdivoom_alarm_handle->cur_alarm_info->index, 0);
            divoom_system_free_lock(pdivoom_alarm_handle->divoom_alarm_task_mux);
            if(pdivoom_alarm_handle->cur_alarm_info->type == DIVOOM_ALARM_TYPE_ALARM)
            {
                divoom_net_app_send_packet(DIVOOM_NET_COMM_IFFT_TRIGGER_ALARM_SNOOZE, DIVOOM_SERVER_TOPIC, NULL);
            }
        }
        else
        {
            divoom_alarm_cancel_alarm();
        }
	}
    else
    {
        divoom_alarm_cancel_alarm();
    }
	return 1;
}

void divoom_alarm_get_current_alarm_time_str(char *disp_str)
{
    T_DIVOOM_ALARM_ACTIVE_DO_INFO *data_item = pdivoom_alarm_handle->cur_alarm_info;

    if(data_item)
    {
        if(divoom_main_get_time_flag())
        {
            sprintf(disp_str, "%02d:%02d", data_item->hour, data_item->minute);
        }
        else
        {
            int hour = data_item->hour;
            if(hour != 12)
            {
                hour = hour % 12;
                if(hour == 0)
                {
                    hour = 12;
                }
            }
            if(data_item->hour > 12)
            {
                sprintf(disp_str, "%02d:%02d", hour, data_item->minute);
            }
            else
            {
                sprintf(disp_str, "%02d:%02d", hour, data_item->minute);
            }
        }
    }
    else if (disp_str)
    {
        /* 无当前闹钟时不要填 12:00：响铃页会当成真实时刻。空串表示无效。 */
        disp_str[0] = '\0';
    }
}

int divoom_alarm_get_current_alarm_mode(void)
{
	if (pdivoom_alarm_handle == NULL || pdivoom_alarm_handle->cur_alarm_info == NULL)
		return -1;

	return pdivoom_alarm_handle->cur_alarm_info->mode;
}

#if 0 /* 唤醒场景：AstroToo 无调用方 */
int divoom_alarm_get_current_alarm_scene_index(void)
{
	if(pdivoom_alarm_handle == NULL)
	{
		return 0;
	}

	return pdivoom_alarm_handle->cur_alarm_info->scene_index;
}
#endif


void divoom_alarm_get_current_alarm_name_str(char *disp_str)
{
    T_DIVOOM_ALARM_ACTIVE_DO_INFO *data_item = pdivoom_alarm_handle->cur_alarm_info;

    if(data_item && data_item->alarm_name != NULL && strlen(data_item->alarm_name) != 0)
    {
		sprintf(disp_str, "%s", divoom_menu_control_get_disp_str(data_item->alarm_name));
    }
    else
    {
        sprintf(disp_str, "%s1", divoom_menu_control_get_disp_str("Alarm"));
    }
}

int divoom_alarm_get_alarm_active_status(int *remain_time)
{
	time_t now_time;
    
	now_time = time(0);
    *remain_time = pdivoom_alarm_handle->end_time - now_time;

	return pdivoom_alarm_handle->alarm_status;
}

void divoom_alarm_process_alarm_menu_btn(int btn_flag)//0:取消闹钟  1:稍后提醒
{
	if(pdivoom_alarm_handle->alarm_status != DIVOOM_ALARM_STATUS_WAIT)
	{
        if(btn_flag == 1 && pdivoom_alarm_handle->alarm_status == DIVOOM_ALARM_STATUS_ACTIVE)
        {
            divoom_system_get_lock(pdivoom_alarm_handle->divoom_alarm_task_mux);            
            pdivoom_alarm_handle->end_time = time(0) + DIVOOM_ALARM_RESTORE_TIME;
			pdivoom_alarm_handle->run_times ++;
            //显示贪睡图案--need
            divoom_alarm_disp_snooze(pdivoom_alarm_handle->cur_alarm_info->index, 0);
            pdivoom_alarm_handle->alarm_status = DIVOOM_ALARM_STATUS_SNOOZE;
            divoom_system_free_lock(pdivoom_alarm_handle->divoom_alarm_task_mux);
            if(pdivoom_alarm_handle->cur_alarm_info->type == DIVOOM_ALARM_TYPE_ALARM)
            {
                divoom_net_app_send_packet(DIVOOM_NET_COMM_IFFT_TRIGGER_ALARM_SNOOZE, DIVOOM_SERVER_TOPIC, NULL);
            }
        }
        else if(pdivoom_alarm_handle->cur_alarm_info != NULL)
        {
            divoom_alarm_cancel_alarm();
        }
	}
    else if(pdivoom_alarm_handle->cur_alarm_info != NULL)
    {
        divoom_alarm_cancel_alarm();
    }
}




/*********************************************************************
  Function:         divoom_alarm_ctrl_set_vol
  Description:     试听某个闹钟的音源
  Input:
  Output             NONE
  Author:           
  Data:            
**********************************************************************/

#define DIVOOM_ALARM_PREVIEW_NONE    0u
#define DIVOOM_ALARM_PREVIEW_CLASSIC 1u
#define DIVOOM_ALARM_PREVIEW_SCENE   2u

static uint8_t s_alarm_preview_kind = DIVOOM_ALARM_PREVIEW_NONE;
static uint8_t s_alarm_preview_voice_id = 0xFFu;
static uint8_t s_alarm_preview_scene = 0xFFu;

#define DIVOOM_ALARM_PREVIEW_DEBOUNCE_MS  80u

typedef struct {
	uint8_t active;
	uint8_t kind;
	uint8_t key;
	uint8_t vol;
	uint32_t fire_ms;
} divoom_alarm_preview_debounce_t;

static divoom_alarm_preview_debounce_t s_alarm_preview_debounce;

static void divoom_alarm_preview_debounce_cancel(void)
{
	s_alarm_preview_debounce.active = 0;
}

static void divoom_alarm_preview_start_classic(T_U8 voice_id, T_U8 vol, T_U8 lvgl);
#ifdef DIVOOM_TIME_FRAME_SPEAKER
static void divoom_alarm_preview_start_scene(T_U8 scene_index, T_U8 vol, T_U8 lvgl);
#endif

static void divoom_alarm_preview_debounce_fire(void)
{
	divoom_alarm_preview_debounce_t req;

	if (!s_alarm_preview_debounce.active)
	{
		return;
	}
	if (divoom_system_get_tickms() < s_alarm_preview_debounce.fire_ms)
	{
		return;
	}
	req = s_alarm_preview_debounce;
	s_alarm_preview_debounce.active = 0;
	if (req.kind == DIVOOM_ALARM_PREVIEW_CLASSIC)
	{
		divoom_alarm_preview_start_classic(req.key, req.vol, 0);
	}
#ifdef DIVOOM_TIME_FRAME_SPEAKER
	else if (req.kind == DIVOOM_ALARM_PREVIEW_SCENE)
	{
		divoom_alarm_preview_start_scene(req.key, req.vol, 0);
	}
#endif
}

static void divoom_alarm_preview_debounce_poll(void)
{
	divoom_alarm_preview_debounce_fire();
}

static void divoom_alarm_preview_debounce_arm(uint8_t kind, uint8_t key, uint8_t vol)
{
	s_alarm_preview_debounce.active = 1;
	s_alarm_preview_debounce.kind = kind;
	s_alarm_preview_debounce.key = key;
	s_alarm_preview_debounce.vol = vol;
	s_alarm_preview_debounce.fire_ms =
		divoom_system_get_tickms() + DIVOOM_ALARM_PREVIEW_DEBOUNCE_MS;
}

T_VOID divoom_alarm_ctrl_set_vol(T_U8 vol)
{
	if (s_alarm_preview_debounce.active)
	{
		s_alarm_preview_debounce.vol = vol;
	}
    divoom_local_music_ctrl_set_tone_volume(vol);
}

int divoom_alarm_ctrl_try_listen_over(char *music_name, int status);

static void divoom_alarm_preview_mark_active(uint8_t kind, uint8_t key, T_U8 vol)
{
	(void)vol;

	if (pdivoom_alarm_handle == NULL)
	{
		return;
	}
	pdivoom_alarm_handle->alarm_status = DIVOOM_ALARM_STATUS_TRY_LISTEN;
	pdivoom_alarm_handle->end_time = time(0) + DIVOOM_ALARM_TRY_VOICE_TIME;
	s_alarm_preview_kind = kind;
	if (kind == DIVOOM_ALARM_PREVIEW_CLASSIC)
	{
		s_alarm_preview_voice_id = key;
		s_alarm_preview_scene = 0xFFu;
	}
	else if (kind == DIVOOM_ALARM_PREVIEW_SCENE)
	{
		s_alarm_preview_scene = key;
		s_alarm_preview_voice_id = 0xFFu;
	}
}

static void divoom_alarm_preview_clear_mark(void)
{
	s_alarm_preview_kind = DIVOOM_ALARM_PREVIEW_NONE;
	s_alarm_preview_voice_id = 0xFFu;
	s_alarm_preview_scene = 0xFFu;
}

T_VOID divoom_alarm_ctrl_preview_stop(T_VOID)
{
	divoom_alarm_preview_debounce_cancel();
	if (pdivoom_alarm_handle == NULL
	    || pdivoom_alarm_handle->alarm_status != DIVOOM_ALARM_STATUS_TRY_LISTEN)
	{
		return;
	}
	divoom_alarm_ctrl_try_listen_over(NULL, 0);
	divoom_local_music_ctrl_stop_tone();
	pdivoom_alarm_handle->alarm_status = DIVOOM_ALARM_STATUS_WAIT;
	pdivoom_alarm_handle->end_time = 0;
	divoom_alarm_preview_clear_mark();
}

static void divoom_alarm_preview_play_file(const char *file_name, T_U8 vol,
					  uint8_t kind, uint8_t key)
{
	divoom_alarm_preview_mark_active(kind, key, vol);
	divoom_print("alarm preview play: kind=%u key=%u vol=%u\n",
		     (unsigned)kind, (unsigned)key, (unsigned)vol);
	divoom_local_music_ctrl_tone_play((char *)file_name, vol,
					  divoom_alarm_ctrl_try_listen_over,
					  DIVOOM_ALARM_TRY_VOICE_TIME);
}

// pnext_callback ,播放完了触发回调通知上面,如果返回0，将结束播放，否则继续启动播放
int divoom_alarm_ctrl_try_listen_over(char *music_name, int status)//status:1:正常播放结束 ；0：文件异常
{
	divoom_app_schedule_item_info alarm_request_info = {0};
	
	(void)music_name;
	(void)status;
	pdivoom_alarm_handle->active_status = 0;
	pdivoom_alarm_handle->alarm_status = DIVOOM_ALARM_STATUS_WAIT;
    alarm_request_info.model_id = DIVOOM_MODEL_ALARM;
    alarm_request_info.event_id = DIVOOM_ALARM_APP_EVENT_LISTEN;
    alarm_request_info.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
    alarm_request_info.resource_type = DIVOOM_ALARM_SCHEDULE_RESOURCE_TYPE;
    divoom_app_schedule_cancel(&alarm_request_info);
	divoom_alarm_preview_clear_mark();

	return 0;
}


/*********************************************************************
  Function:         divoom_alarm_preview_start_classic
  Description:     设备/UI 或防抖到期后真正启动经典铃声试听
**********************************************************************/
/* APP 试听：占 VOICE 以便设备 ESC 进 alarm key_handle；pause 由 music_ctrl 对 LISTEN 跳过 */
static int divoom_alarm_preview_app_schedule_begin(void)
{
	divoom_app_schedule_item_info info = {0};

	info.model_id = DIVOOM_MODEL_ALARM;
	info.event_id = DIVOOM_ALARM_APP_EVENT_LISTEN;
	info.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
	info.resource_type = DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE;
	return divoom_app_schedule_push(&info) != 0;
}

static void divoom_alarm_preview_start_classic(T_U8 voice_id, T_U8 vol, T_U8 lvgl)
{
	char file_name[128];

	divoom_voice_read_get_filename(divoom_alarm_voice_table[voice_id], file_name);

	if (pdivoom_alarm_handle->alarm_status == DIVOOM_ALARM_STATUS_TRY_LISTEN)
	{
		if (s_alarm_preview_kind == DIVOOM_ALARM_PREVIEW_CLASSIC
		    && s_alarm_preview_voice_id == voice_id)
		{
			pdivoom_alarm_handle->end_time = time(0) + DIVOOM_ALARM_TRY_VOICE_TIME;
			divoom_local_music_ctrl_set_tone_volume(vol);
			return;
		}
		divoom_alarm_preview_play_file(file_name, vol,
					       DIVOOM_ALARM_PREVIEW_CLASSIC, voice_id);
		return;
	}

	if (!lvgl && !divoom_alarm_preview_app_schedule_begin())
	{
		return;
	}

	divoom_alarm_preview_play_file(file_name, vol,
				       DIVOOM_ALARM_PREVIEW_CLASSIC, voice_id);
}

#ifdef DIVOOM_TIME_FRAME_SPEAKER
static void divoom_alarm_preview_start_scene(T_U8 scene_index, T_U8 vol, T_U8 lvgl)
{
	char file_name[128];

	if (scene_index >= (sizeof(divoom_alarm_scene_voice_table)
			     / sizeof(divoom_alarm_scene_voice_table[0])))
	{
		return;
	}

	divoom_voice_read_get_filename(divoom_alarm_scene_voice_table[scene_index], file_name);

	if (pdivoom_alarm_handle->alarm_status == DIVOOM_ALARM_STATUS_TRY_LISTEN)
	{
		if (s_alarm_preview_kind == DIVOOM_ALARM_PREVIEW_SCENE
		    && s_alarm_preview_scene == scene_index)
		{
			pdivoom_alarm_handle->end_time = time(0) + DIVOOM_ALARM_TRY_VOICE_TIME;
			divoom_local_music_ctrl_set_tone_volume(vol);
			return;
		}
		divoom_alarm_preview_play_file(file_name, vol,
					       DIVOOM_ALARM_PREVIEW_SCENE, scene_index);
		return;
	}

	if (!lvgl && !divoom_alarm_preview_app_schedule_begin())
	{
		return;
	}

	divoom_alarm_preview_play_file(file_name, vol,
				       DIVOOM_ALARM_PREVIEW_SCENE, scene_index);
}
#endif


/*********************************************************************
  Function:         divoom_alarm_ctrl_try_listen
  Description:     试听某个闹钟的音源
  Input:
  Output             NONE
  Author:           
  Data:            
**********************************************************************/

T_VOID divoom_alarm_ctrl_try_listen(T_U8 voice_id, T_U8 vol, T_U8 on_off, T_U8 lvgl)
{
    if(on_off == 0)
    {
		divoom_alarm_preview_debounce_cancel();
		divoom_alarm_ctrl_preview_stop();
		return;
    }

	/* 已在试听：同曲只改音量；换曲走 tone cut */
	if (pdivoom_alarm_handle->alarm_status == DIVOOM_ALARM_STATUS_TRY_LISTEN)
	{
		if (s_alarm_preview_kind == DIVOOM_ALARM_PREVIEW_CLASSIC
		    && s_alarm_preview_voice_id == voice_id)
		{
			pdivoom_alarm_handle->end_time = time(0) + DIVOOM_ALARM_TRY_VOICE_TIME;
			divoom_local_music_ctrl_set_tone_volume(vol);
			return;
		}
	}

	/* APP 远程试听：80ms 防抖，只播最后一次 */
	if (!lvgl)
	{
		divoom_alarm_preview_debounce_arm(DIVOOM_ALARM_PREVIEW_CLASSIC, voice_id, vol);
		return;
	}

	divoom_alarm_preview_start_classic(voice_id, vol, lvgl);
}

#ifdef DIVOOM_TIME_FRAME_SPEAKER
T_VOID divoom_alarm_ctrl_try_listen_scene(T_U8 scene_index, T_U8 vol, T_U8 on_off, T_U8 lvgl)
{
	if(on_off == 0)
	{
		divoom_alarm_preview_debounce_cancel();
		divoom_alarm_ctrl_preview_stop();
		return;
	}

	if(scene_index >= (sizeof(divoom_alarm_scene_voice_table) / sizeof(divoom_alarm_scene_voice_table[0])))
	{
		return;
	}

	if (pdivoom_alarm_handle->alarm_status == DIVOOM_ALARM_STATUS_TRY_LISTEN)
	{
		if (s_alarm_preview_kind == DIVOOM_ALARM_PREVIEW_SCENE
		    && s_alarm_preview_scene == scene_index)
		{
			pdivoom_alarm_handle->end_time = time(0) + DIVOOM_ALARM_TRY_VOICE_TIME;
			divoom_local_music_ctrl_set_tone_volume(vol);
			return;
		}
	}

	if (!lvgl)
	{
		divoom_alarm_preview_debounce_arm(DIVOOM_ALARM_PREVIEW_SCENE, scene_index, vol);
		return;
	}

	divoom_alarm_preview_start_scene(scene_index, vol, lvgl);
}
#endif

void divoom_alarm_play_listen_voice_over_callback(uint8 status)
{
#if 0
    divoom_app_schedule_item_info event_info = {0};

    pdivoom_alarm_handle->active_status = 0;
	event_info.model_id = DIVOOM_MODEL_ALARM;
	event_info.event_id = DIVOOM_ALARM_APP_EVENT_LISTEN;
    event_info.resource_type = DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE;
    divoom_app_schedule_cancel(&event_info);
#endif
}

void divoom_alarm_play_listen_voice(T_U8 sound_type, int volume)
{
#if 1


#else           
    char file_name[128];

    strcpy(file_name, DIVOOM_ALARM_MUSIC_BASE_PATH);
    strcat(file_name, divoom_alarm_music_file_table[sound_type]);
    divoom_local_music_play(file_name, volume, 0, divoom_alarm_play_listen_voice_over_callback, 0);
#endif
}

static void divoom_alarm_play_builtin_fallback_tone(Pdivoom_alarm_struct alarm_handle, int volumn, const char *why)
{
	char file_name[128];
	int tip_index;
	int table_n;
	uint8 voice_id;

	if (alarm_handle == NULL || alarm_handle->cur_alarm_info == NULL)
		return;

	table_n = (int)(sizeof(divoom_alarm_voice_table) / sizeof(divoom_alarm_voice_table[0]));
	tip_index = alarm_handle->cur_alarm_info->tip_index;
	if (tip_index >= 0 && tip_index < table_n
	    && divoom_voice_read_file_is_exit(divoom_alarm_voice_table[tip_index]))
		voice_id = divoom_alarm_voice_table[tip_index];
	else
		voice_id = DIVOOM_VOICE_ID_CLOCK_NORMAL1;

	divoom_print("alarm: TF fallback builtin tone id=%u why=%s\n",
		     (unsigned)voice_id, why ? why : "");
	divoom_voice_read_get_filename(voice_id, file_name);
	divoom_local_music_ctrl_tone_play(file_name, volumn, NULL, DIVOOM_ALARM_ACTIVE_TIME);
}

void divoom_alarm_play_voice(Pdivoom_alarm_struct alarm_handle, int time)
{
#if 1
	int alarm_id;
	char file_name[128];
	int tip_index = alarm_handle->cur_alarm_info->tip_index;
	int volumn = alarm_handle->cur_alarm_info->volume;

	(void)time;
#ifdef DIVOOM_TIME_FRAME_SPEAKER
	/* WAKE_HOLD 开机保持 mute（r43223）；真正响铃再解，避免黑屏等点时漏出开机/回连提示音 */
	divoom_jl_release_boot_audio_gates_once();
#endif
	if(alarm_handle->cur_alarm_info->type == DIVOOM_ALARM_TYPE_ALARM)
	{
		if(alarm_handle->cur_alarm_info->mode == 0)
		{
			divoom_voice_read_get_filename(divoom_alarm_voice_table[tip_index], file_name);
			divoom_local_music_ctrl_tone_play(file_name, volumn, NULL, DIVOOM_ALARM_ACTIVE_TIME);
		}
		else if(alarm_handle->cur_alarm_info->mode == 1)
		{
			alarm_id = alarm_handle->cur_alarm_info->alarm_id;
			divoom_voice_read_get_filename(DIVOOM_VOICE_ID_START_ALARM_VOICE + alarm_id, file_name);
			divoom_local_music_ctrl_tone_play(file_name, volumn, NULL, DIVOOM_ALARM_ACTIVE_TIME);
		}
#ifdef DIVOOM_TIME_FRAME_SPEAKER
#ifdef DIVOOM_TIMES_VIBE
		else if(alarm_handle->cur_alarm_info->mode == 3)
		{
			if (divoom_music_ctrl_alarm_play_bt((uint8)volumn) != 0)
				divoom_alarm_play_builtin_fallback_tone(alarm_handle, volumn, "bt play failed");
		}
#endif
		else if(alarm_handle->cur_alarm_info->mode == 4)
		{
#ifdef DIVOOM_TIMES_VIBE
			int music_id;
			int play_ret;

			music_id = divoom_alarm_tf_use_first_track_if_needed(alarm_handle->cur_alarm_info);
			if(music_id <= 0)
			{
				divoom_alarm_play_builtin_fallback_tone(alarm_handle, volumn, "tf resolve failed");
			}
			else
			{
				play_ret = divoom_tf_music_play_music_id_ex(music_id, (uint8)volumn, 1);
				divoom_print("alarm: TF play music_id=%d ret=%d\n", music_id, play_ret);
				if (play_ret < 0)
					divoom_alarm_play_builtin_fallback_tone(alarm_handle, volumn, "tf play failed");
			}
#else
			if(strlen(alarm_handle->cur_alarm_info->leave_url) > 0)
			{
				snprintf(file_name, sizeof(file_name), "file://%s", alarm_handle->cur_alarm_info->leave_url);
				divoom_local_music_ctrl_tone_play(file_name, volumn, NULL, DIVOOM_ALARM_ACTIVE_TIME);
			}
#endif
		}
#endif
	}
	else
	{
		divoom_voice_read_get_filename(DIVOOM_ALARM_DIALY_VOICE_ID, file_name);
		divoom_local_music_ctrl_tone_play(file_name, divoom_main_get_tone_volume(), NULL, DIVOOM_ALARM_ACTIVE_TIME);
	}
	
	

#else
	//play voice;
    char file_name[128];

    if(divoom_main_get_wifi_con_status())
    {
        if(strlen(alarm_handle->cur_alarm_info->record_id))
        {
            strcpy(file_name, DIVOOM_SERVER_URL);
            strcat(file_name, alarm_handle->cur_alarm_info->record_id);
            divoom_local_music_play(file_name, alarm_handle->play_vol, 0, NULL, 1);
        }
        else if(strlen(alarm_handle->cur_alarm_info->music_info))
        {
            //fm
            divoom_sys_msg_info model_msg = {0};

            model_msg.msg_type = 1;
            model_msg.src_id = DIVOOM_MODEL_ALARM;  
            model_msg.dest_id = DIVOOM_MODEL_NET_RADIO;          
            model_msg.msg_id = DIVOOM_NET_RADIO_GET_STATION; 
            model_msg.data = (uint8 *)divoom_app_com_copy_string(alarm_handle->cur_alarm_info->music_info, 0);
            divoom_sys_model_crl_msg_push(&model_msg);  
            model_msg.msg_id = DIVOOM_NET_RADIO_PLAY; 
            model_msg.data = NULL;
            divoom_sys_model_crl_msg_push(&model_msg);  
        }
        else
        {
            strcpy(file_name, DIVOOM_ALARM_MUSIC_BASE_PATH);
            if(alarm_handle->cur_alarm_info->sound_type < 2)
            {
                divoom_local_music_play(DIVOOM_AI_ALARM_MUSIC_PATH, alarm_handle->play_vol, 0, NULL, 1);
            }
            else
            {
                strcat(file_name, divoom_alarm_music_file_table[alarm_handle->cur_alarm_info->sound_type]);
                divoom_local_music_play(file_name, alarm_handle->play_vol, 0, NULL, 1);
            }
        }
    }
    else
    {
        //没有网络时，先播放本地音乐
        divoom_local_music_play(DIVOOM_AI_ALARM_MUSIC_PATH, alarm_handle->play_vol, 0, NULL, 1);
    }
#endif
}

void divoom_alarm_stop_voice(Pdivoom_alarm_struct alarm_handle)
{
	int classic_tone = 0;

	(void)alarm_handle;
	/* 经典铃声 mode 0 只停提示音，不改手机直接播放的播放状态。录音/蓝牙/TF 仍暂停。 */
	if (pdivoom_alarm_handle && pdivoom_alarm_handle->cur_alarm_info
	    && pdivoom_alarm_handle->cur_alarm_info->type == DIVOOM_ALARM_TYPE_ALARM
	    && pdivoom_alarm_handle->cur_alarm_info->mode == 0)
		classic_tone = 1;
#ifdef DIVOOM_TIMES_VIBE
	if (!classic_tone)
		divoom_music_ctrl_pause_output_only();
#endif
	divoom_local_music_ctrl_stop_tone();
}

void divoom_alarm_active_disp_play(int disp_mode, int clock_id)
{
	
    if(disp_mode == 1)//云图
    {
    	pdivoom_alarm_handle->dial_info.disp_mode = 0;
        pdivoom_alarm_handle->dial_info.clock_id = DIVOOM_DIALY_CLOCK_ID; //divoom_light_app_get_divoom_animation_edge_clock_id();
        divoom_main_set_global_image_handle(DIVOOM_MANAGE_IMAGE_ALARM_GIF, DIVOOM_MANAGE_IMAGE_ID_BASE_DIALY_GIF + pdivoom_alarm_handle->cur_alarm_info->alarm_id, 0);
    }
    else//表盘
    {
        pdivoom_alarm_handle->dial_info.clock_id = clock_id;
    }
	
    divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID, (void *)&pdivoom_alarm_handle->dial_info, DIVOOM_MODEL_ALARM, DIVOOM_ALARM_APP_EVENT_ALARM);
}


void divoom_alarm_disp_gif(Pdivoom_alarm_struct alarm_handle)
{
    if (!alarm_handle || !alarm_handle->cur_alarm_info)
        return;
    divoom_light_app_open_screen();
    if (!alarm_handle->cur_alarm_info)
    {
        return;
    }
	if(alarm_handle->cur_alarm_info->type == DIVOOM_ALARM_TYPE_ALARM)
	{
		divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_ALARM_ACTIVE_MENU, NULL, 0, 0);
	}
	else
	{
        //先使用闹钟，后面再做特殊菜单
		//divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_ALARM_ACTIVE_MENU, NULL, 0, 0);
        divoom_alarm_active_disp_play(1, 0);
	}
}

static void divoom_alarm_cancel_alarm(void)
{
#if 1
	//还是需要取消schdule的
	divoom_app_schedule_item_info event_info = {0};

	if (pdivoom_alarm_handle->cur_alarm_info == NULL)
	{
		pdivoom_alarm_handle->alarm_status = DIVOOM_ALARM_STATUS_WAIT;
		return;
	}

    if(pdivoom_alarm_handle->cur_alarm_info->type == DIVOOM_ALARM_TYPE_ALARM)
    {
        divoom_net_app_send_packet(DIVOOM_NET_COMM_IFFT_TRIGGER_ALARM_STOP, DIVOOM_SERVER_TOPIC, &pdivoom_alarm_handle->cur_alarm_info->alarm_id);
    }
    else
    {
        divoom_net_app_send_packet(DIVOOM_NET_COMM_IFFT_TRIGGER_MEMORIAL_STOP, DIVOOM_SERVER_TOPIC, &pdivoom_alarm_handle->cur_alarm_info->alarm_id);
    }

	event_info.model_id = DIVOOM_MODEL_ALARM;
	event_info.event_id = DIVOOM_ALARM_APP_EVENT_ALARM;
	event_info.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
    event_info.resource_type = DIVOOM_ALARM_SCHEDULE_RESOURCE_TYPE;
	divoom_app_schedule_cancel(&event_info);
	
	pdivoom_alarm_handle->alarm_status = DIVOOM_ALARM_STATUS_WAIT;
#endif
}

static void divoom_alarm_run_alarm(Pdivoom_alarm_struct pdivoom_alarm_active_handle)
{
    if (!pdivoom_alarm_active_handle)
        return;
    if (!pdivoom_alarm_active_handle->cur_alarm_info)
    {
        goto inactive;
    }
    pdivoom_alarm_active_handle->alarm_status = DIVOOM_ALARM_STATUS_ACTIVE;
    divoom_main_cancel_on_off();
    if (!pdivoom_alarm_active_handle->cur_alarm_info)
    {
        goto inactive;
    }
	divoom_alarm_disp_gif(pdivoom_alarm_active_handle);
    if (!pdivoom_alarm_active_handle->cur_alarm_info)
    {
        goto inactive;
    }
	pdivoom_alarm_active_handle->play_vol = 0;
	pdivoom_alarm_active_handle->active_status = 1;
	divoom_alarm_play_voice(pdivoom_alarm_active_handle, 0);
    if (!pdivoom_alarm_active_handle->cur_alarm_info)
    {
        goto inactive;
    }
    if(pdivoom_alarm_active_handle->cur_alarm_info->type == DIVOOM_ALARM_TYPE_ALARM)
    {
        divoom_net_app_send_packet(DIVOOM_NET_COMM_IFFT_TRIGGER_ALARM_START, DIVOOM_SERVER_TOPIC, &pdivoom_alarm_active_handle->cur_alarm_info->alarm_id);
    }
    else
    {
        divoom_net_app_send_packet(DIVOOM_NET_COMM_IFFT_TRIGGER_MEMORIAL_START, DIVOOM_SERVER_TOPIC, &pdivoom_alarm_active_handle->cur_alarm_info->alarm_id);
    }
    return;

inactive:
    pdivoom_alarm_active_handle->active_status = 0;
    pdivoom_alarm_active_handle->alarm_status = DIVOOM_ALARM_STATUS_WAIT;
}
/*********************************************************************
  Function:        divoom_alarm_get_active_dialy_name
  Description:     获取激活的纪念日的名称
  
  Input:            
  Return:          返回纪念日的名称
**********************************************************************/
char *divoom_alarm_get_active_dialy_name(void)
{
    if(pdivoom_alarm_handle->cur_alarm_info == NULL)
    {
        return NULL;
    }
    return pdivoom_alarm_handle->cur_alarm_info->alarm_name;
}

static void divoom_alarm_active_alarm(Pdivoom_alarm_struct pdivoom_alarm_active_handle)
{
	divoom_app_schedule_item_info event_info = {0};
    
	if(pdivoom_alarm_active_handle == NULL)
	{
		divoom_print( "have some wrong in %s:%d", __FILE__, __LINE__);
		return;
	}

    if (!pdivoom_alarm_active_handle->cur_alarm_info)
    {
        pdivoom_alarm_active_handle->active_status = 0;
        pdivoom_alarm_active_handle->alarm_status = DIVOOM_ALARM_STATUS_WAIT;
        divoom_alarm_lib_set_next_active_alarm();
        return;
    }

	event_info.model_id = DIVOOM_MODEL_ALARM;
	event_info.event_id = DIVOOM_ALARM_APP_EVENT_ALARM;
	event_info.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
	event_info.resource_type = DIVOOM_ALARM_SCHEDULE_RESOURCE_TYPE;
    pdivoom_alarm_handle->resouce_type = divoom_app_schedule_push(&event_info);
	if(0 == pdivoom_alarm_handle->resouce_type)
	{
        pdivoom_alarm_handle->alarm_status = DIVOOM_ALARM_STATUS_WAIT;
		return;
	}
    
    pdivoom_alarm_handle->active_status = DIVOOM_ALARM_APP_EVENT_ALARM;

    divoom_alarm_run_alarm(pdivoom_alarm_active_handle);
    
    divoom_alarm_lib_set_next_active_alarm();
}

//得到马上要激活的闹钟
T_DIVOOM_ALARM_ACTIVE_DO_INFO *divoom_alarm_get_last_alarm_info(void)
{
    return pdivoom_alarm_handle->last_alarm_info;
}

//得到激活的闹钟的时间
int divoom_alarm_get_run_alarm_time(void)
{
    return pdivoom_alarm_handle->next_active_time - time(0);
}


void divoom_alarm_init(uint8 poweron_mode)
{
	if(pdivoom_alarm_handle)
	{
		divoom_print( "Some wrong in alarm init.");
		return;
	}
	pdivoom_alarm_handle = (Pdivoom_alarm_struct)divoom_malloc(sizeof(divoom_alarm_struct), DIVOOM_MODEL_ALARM, __LINE__, __FILE__);
	if(pdivoom_alarm_handle == NULL)
	{
		divoom_print( "Can't malloc for alarm.");
	}
	memset(pdivoom_alarm_handle, 0, sizeof(divoom_alarm_struct));

#ifdef DIVOOM_ALARM_TEST_USE
	divoom_print("pdivoom_alarm_handle->pnet_alarm_info = %d, pdivoom_alarm_handle->pnet_dialy_info = %d\n", 
			pdivoom_alarm_handle->pnet_alarm_info, pdivoom_alarm_handle->pnet_dialy_info);
#endif
	
	pdivoom_alarm_handle->divoom_alarm_task_mux = divoom_system_create_lock();


    pdivoom_alarm_handle->dialy_alarm_queue = divoom_queue_init(100, __func__, __LINE__);
	pdivoom_alarm_handle->alarm_status = DIVOOM_ALARM_STATUS_WAIT;
    pdivoom_alarm_handle->next_active_id = -1;


	pdivoom_alarm_handle->run_flag = 1;
	pdivoom_alarm_handle->stop_flag = 0;

	divoom_alarm_get_all_alarm_info();
	if (poweron_mode == DIVOOM_ONOFF_MODE_CLOCK)
		pdivoom_alarm_handle->rtc_clock_skip_expire_once = 1;
    if(time(0) > 1597112532)
    {
        divoom_alarm_lib_set_next_active_alarm();
    }
	pdivoom_alarm_handle->rtc_clock_skip_expire_once = 0;
    if(poweron_mode == DIVOOM_ONOFF_MODE_CLOCK)
    {
		/* 已过到点：首帧就会进 INTTRUPT，缩短首次 sleep 尽快拉界面；提前唤醒则保持 next_active_time 等到真正到点 */
		if (pdivoom_alarm_handle->next_active_id >= 0 && pdivoom_alarm_handle->next_active_time != 0) {
			time_t tn = time(NULL);
			if (tn >= (time_t)pdivoom_alarm_handle->next_active_time)
				pdivoom_alarm_handle->rtc_clock_short_first_delay = 1;
		}
	    divoom_app_schedule_item_info event_info = {0};
        
    	event_info.model_id = DIVOOM_MODEL_ALARM;
    	event_info.event_id = DIVOOM_ALARM_APP_EVENT_ALARM;
    	event_info.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
    	event_info.resource_type = DIVOOM_ALARM_SCHEDULE_RESOURCE_TYPE;
        pdivoom_alarm_handle->resouce_type = divoom_app_schedule_push(&event_info);
        //divoom_disp_ctrl_set_mode(DIVOOM_DISP_NO_DISP_MODE, NULL);
//        divoom_mix_screen_oneline_led_bright(0);
//        divoom_mix_screen_oneline_led_init_screen(&color);
    }
	divoom_sys_model_crl_register(DIVOOM_MODEL_ALARM ,divoom_alarm_event);
	//pdivoom_alarm_handle->task_handle = divoom_system_create_task(divoom_alarm_main, "alarm", (void *)pdivoom_alarm_handle, DIVOOM_TASK_STACK_ALARM);    

	return;
}

//还是不要放在主循环了,有一些下载处理
void divoom_alarm_main(void *arg)
{
	Pdivoom_alarm_struct pdivoom_alarm_task_handle = (Pdivoom_alarm_struct)pdivoom_alarm_handle;
	time_t now_time;

	divoom_alarm_preview_debounce_poll();
    
	now_time = time(0);
    if(now_time < DIVOOM_BASE_UTC_TIME)
    {            
        return;
    }
	divoom_system_get_lock(pdivoom_alarm_task_handle->divoom_alarm_task_mux);
	
	//防止只有一个闹钟的时候next_alarm_info其实就是当前运行的闹钟的数据被重写
	if(pdivoom_alarm_task_handle->next_active_id != -1)
	{
        T_DIVOOM_ALARM_ACTIVE_DO_INFO   *next_alarm_info;           //下一个闹钟到点的数据信息

        next_alarm_info = divoom_queue_get_index_item(pdivoom_alarm_task_handle->dialy_alarm_queue, pdivoom_alarm_task_handle->next_active_id);
        if(next_alarm_info && (0 == (now_time % 60)))
        {
            divoom_print("%s: next alarm info; utc: %d,now_time : %d;%d-%d-%d %d:%d\n",__func__,(int)pdivoom_alarm_task_handle->next_active_light_time, (int)now_time, next_alarm_info->year+1900 , next_alarm_info->month + 1, next_alarm_info->day, next_alarm_info->hour, next_alarm_info->minute);
        }

	}

	if(pdivoom_alarm_task_handle->next_active_time && (now_time >= pdivoom_alarm_task_handle->next_active_time))
	{
		{
			//通知可以更新显示断码屏时间标志，用于开机显示
			divoom_sys_msg_info model_msg = {0};
			uint8 *ledseg_update_flag = divoom_system_malloc(sizeof(uint8));
			model_msg.src_id = DIVOOM_MODEL_ALARM; 
			model_msg.dest_id = DIVOOM_MODEL_MAIN; 
			model_msg.msg_id = DIVOOM_MAIN_EVENT_LEDSEG_UPDATE_FLAG;
			model_msg.msg_type = 1;
			model_msg.data = ledseg_update_flag;
			ledseg_update_flag[0] = 1;
			divoom_sys_model_crl_msg_push(&model_msg);
		}
        pdivoom_alarm_task_handle->alarm_status = DIVOOM_ALARM_STATUS_INTTRUPT;
		pdivoom_alarm_task_handle->cur_alarm_info = divoom_queue_get_index_item(pdivoom_alarm_task_handle->dialy_alarm_queue, pdivoom_alarm_task_handle->next_active_id);
        if(pdivoom_alarm_task_handle->cur_alarm_info && pdivoom_alarm_task_handle->cur_alarm_info->cyc_flag == 0  && pdivoom_alarm_task_handle->cur_alarm_info->type != DIVOOM_ALARM_TYPE_DIALY)
        {
            pdivoom_alarm_task_handle->cur_alarm_info->on_off = 0;
            pdivoom_alarm_task_handle->pnet_alarm_info = 1;
        }
		pdivoom_alarm_task_handle->run_times = 0;
        pdivoom_alarm_task_handle->play_vol = 0;
	}
	
    switch(pdivoom_alarm_task_handle->alarm_status)
    {        
        case DIVOOM_ALARM_STATUS_WAIT:
            break;
    
        case DIVOOM_ALARM_STATUS_OVER_SNOOZE:
            break;
            
        case DIVOOM_ALARM_STATUS_ACTIVE:
			if(pdivoom_alarm_handle->active_status)
            {
                if(now_time >= pdivoom_alarm_task_handle->end_time)
                {
                    pdivoom_alarm_task_handle->run_times ++;
                    
                    if(pdivoom_alarm_handle->cur_alarm_info->type == DIVOOM_ALARM_TYPE_ALARM)
                    {
                        divoom_net_app_send_packet(DIVOOM_NET_COMM_IFFT_TRIGGER_ALARM_SNOOZE, DIVOOM_SERVER_TOPIC, &pdivoom_alarm_handle->cur_alarm_info->alarm_id);
                    }
                    
                    if(pdivoom_alarm_task_handle->run_times < DIVOOM_ALARM_TRY_TIMES)
                    {
                        //pdivoom_alarm_task_handle->end_time = now_time + pdivoom_alarm_task_handle->run_times * DIVOOM_ALARM_RESTORE_TIME;
                        pdivoom_alarm_task_handle->end_time = now_time + DIVOOM_ALARM_RESTORE_TIME;
                    }
                    else
                    {
                        //最后一次将一直显示贪睡时间,设置为超时时间为一年
                        pdivoom_alarm_task_handle->end_time = now_time + 365*24*3600;
                    }
					
                    //显示贪睡图案--need
                    divoom_alarm_disp_snooze(pdivoom_alarm_task_handle->cur_alarm_info->index, pdivoom_alarm_task_handle->run_times>=DIVOOM_ALARM_TRY_TIMES);
					
                }
                else
                {
                    
                }
            }
            
            
            break;
            
        case DIVOOM_ALARM_STATUS_SNOOZE:
            if(pdivoom_alarm_handle->active_status)
            {
            	if(now_time >= pdivoom_alarm_task_handle->end_time)
                {
                    //pdivoom_alarm_task_handle->end_time = now_time + DIVOOM_ALARM_RESTORE_TIME;
                    pdivoom_alarm_task_handle->end_time = now_time + DIVOOM_ALARM_ACTIVE_TIME;
                    divoom_alarm_run_alarm(pdivoom_alarm_task_handle);
                    divoom_print("%s: we will active alarm after snooze!\n", __func__);
                }
            }
            break;
    
        case DIVOOM_ALARM_STATUS_INTTRUPT:
			divoom_alarm_active_alarm(pdivoom_alarm_task_handle);
            //pdivoom_alarm_task_handle->end_time = now_time + DIVOOM_ALARM_RESTORE_TIME;
			pdivoom_alarm_task_handle->end_time = now_time + DIVOOM_ALARM_ACTIVE_TIME;
            break;
    
        case DIVOOM_ALARM_STATUS_TRY_LISTEN:
			if(now_time >= pdivoom_alarm_task_handle->end_time)
            {
				/* 到期只停 tip；勿 stop_voice，否则会 pause 音乐 */
				divoom_alarm_ctrl_preview_stop();
            }
            break;
            
    }

	divoom_system_free_lock(pdivoom_alarm_task_handle->divoom_alarm_task_mux);
	if (pdivoom_alarm_task_handle->rtc_clock_short_first_delay) {
		pdivoom_alarm_task_handle->rtc_clock_short_first_delay = 0;
	} 
}

#if 0
uint8 divoom_alarm_check_save_alarm_info(void)
{
	FILE *fd = NULL;
	T_DIVOOM_ALARM_ACTIVE_DO_INFO data_read, *cur_item;
    int i, number, read_bytes;

	if((fd = divoom_port_file_open(DIVOOM_ALARM_INFO_PATH, "rb")) == NULL)
	{
		divoom_print("Some Wrong in %d: Maybe no this file:%s!\n", __LINE__, DIVOOM_ALARM_INFO_PATH);
		return true;
	}
    
    if( 0 == divoom_port_file_read(&number, 1, sizeof(number), fd))
    {
		goto divoom_alarm_check_save_alarm_info_err_table;
    }
    if(number !=  divoom_queue_items(pdivoom_alarm_handle->dialy_alarm_queue))
    {
		divoom_print("alarm number is different!\n");
		goto divoom_alarm_check_save_alarm_info_err_table;
    }

    
    for(i = 0; i < number; i++)
    {
        read_bytes = divoom_port_file_read(&data_read, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO), 1, fd);
        if(read_bytes == 0)
        {
    		divoom_print("read alarm is different!\n");
		    goto divoom_alarm_check_save_alarm_info_err_table;
        }
        cur_item = divoom_queue_get_index_item(pdivoom_alarm_handle->dialy_alarm_queue, i);
        if(cur_item == NULL ||
            memcmp(cur_item, &data_read, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO)))
        {
    		divoom_print("alarm  data is different!\n");
		    goto divoom_alarm_check_save_alarm_info_err_table;
        }
    }
    
	divoom_port_file_close(fd);
    return false;
    
    divoom_alarm_check_save_alarm_info_err_table:
    divoom_port_file_close(fd); 
    return true;
}

void divoom_alarm_save_all_alarm_info_local(void)
{
    int i, number;
	FILE *fd = NULL;
    uint16 check_sum = 0;
	T_DIVOOM_ALARM_ACTIVE_DO_INFO *data_read;

    if(pdivoom_alarm_handle->pnet_alarm_info == 0)
    {
        return ;
    }
    if(divoom_alarm_check_save_alarm_info() == false)
    {
        divoom_print("we will pass save alarm data!\n");
        return;
    }
    
    number = divoom_queue_items(pdivoom_alarm_handle->dialy_alarm_queue);
	if(number)
	{
		if((fd = divoom_port_file_open(DIVOOM_ALARM_INFO_PATH, "wb")) == NULL)
		{
			divoom_print("Some Wrong in %d, maybe no this file:%s\n", __LINE__, DIVOOM_ALARM_INFO_PATH);
			return;
		}
        
        check_sum = divoom_app_com_get_check_sum((uint8 *)&pdivoom_alarm_handle->update_net_info_time, sizeof(pdivoom_alarm_handle->update_net_info_time));
		divoom_port_file_write(&pdivoom_alarm_handle->update_net_info_time, sizeof(pdivoom_alarm_handle->update_net_info_time), 1, fd);
        
        check_sum += divoom_app_com_get_check_sum((uint8 *)&number, sizeof(number));
		divoom_port_file_write(&number, sizeof(number), 1, fd);

        for(i = 0; i < number; i ++)
        {
            data_read = divoom_queue_get_index_item(pdivoom_alarm_handle->dialy_alarm_queue, i);
            if(data_read)
            {
                divoom_port_file_write(data_read, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO), 1, fd);
                check_sum += divoom_app_com_get_check_sum((uint8 *)data_read, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));   
            }
            
        }
		divoom_port_file_write(&check_sum, sizeof(check_sum), 1, fd);
		divoom_port_file_close(fd);
	}
}

void divoom_alarm_save_all_alarm_info(void)
{
    divoom_save_all_config_add(divoom_alarm_save_all_alarm_info_local);
}

void divoom_alarm_get_all_alarm_info(void)
{
	FILE *fd = NULL;
	T_DIVOOM_ALARM_ACTIVE_DO_INFO *data_read;
    int i, number, read_bytes;
    uint16 check_sum = 0, raw_check_sum = 0;

	if((fd = divoom_port_file_open(DIVOOM_ALARM_INFO_PATH, "rb")) == NULL)
	{
		divoom_print("Some Wrong in %d: Maybe no this file:%s!\n", __LINE__, DIVOOM_ALARM_INFO_PATH);
		return ;
	}
    
    if(divoom_app_com_check_check_sum(fd) == 0)
    {
		divoom_port_file_close(fd);
        divoom_print("config is err!\n");
        goto divoom_alarm_get_all_alarm_info_err_table;
    }
    if(0 == divoom_port_file_read(&pdivoom_alarm_handle->update_net_info_time, sizeof(pdivoom_alarm_handle->update_net_info_time), 1, fd))
    {
        goto divoom_alarm_get_all_alarm_info_err_table;
    }
    check_sum = divoom_app_com_get_check_sum((uint8 *)&pdivoom_alarm_handle->update_net_info_time, sizeof(pdivoom_alarm_handle->update_net_info_time));
    
    if(0 == divoom_port_file_read(&number, sizeof(number), 1, fd))
    {
        goto divoom_alarm_get_all_alarm_info_err_table;
    }
    check_sum += divoom_app_com_get_check_sum((uint8 *)&number, sizeof(number));
    for(i = 0; i < number && number < 1000; i++)
    {
        data_read = divoom_system_malloc(sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
        read_bytes = divoom_port_file_read(data_read, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO), 1, fd);
        if(read_bytes == 0)
        {
            break;
        }
        divoom_queue_push(pdivoom_alarm_handle->dialy_alarm_queue, data_read);   
        check_sum += divoom_app_com_get_check_sum((uint8 *)data_read, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));     
    }
    if(0 == divoom_port_file_read(&raw_check_sum, sizeof(raw_check_sum), 1, fd))
    {
        goto divoom_alarm_get_all_alarm_info_err_table;
    }
	divoom_port_file_close(fd);
    if(raw_check_sum != check_sum)
    {
        //所有闹钟无效
        divoom_alarm_get_all_alarm_info_err_table:
        divoom_app_com_del_file(DIVOOM_ALARM_INFO_PATH);
        divoom_queue_pop_all(pdivoom_alarm_handle->dialy_alarm_queue, NULL);
        pdivoom_alarm_handle->next_active_id = -1;
        pdivoom_alarm_handle->update_net_info_time = 0;
    }

}



int divoom_alarm_poweron_get_all_alarm_info(void *dialy_alarm_queue)
{
	FILE *fd = NULL;
	T_DIVOOM_ALARM_ACTIVE_DO_INFO *data_read;
    int i, number, read_bytes;
    uint16 check_sum = 0, raw_check_sum = 0;

	if((fd = divoom_port_file_open(DIVOOM_ALARM_INFO_PATH, "rb")) == NULL)
	{
		divoom_print("Some Wrong in %d: Maybe no this file:%s!\n", __LINE__, DIVOOM_ALARM_INFO_PATH);
		return 0;
	}
    
    if(0 == divoom_port_file_read(&number, sizeof(number), 1, fd))
    {
        goto divoom_alarm_poweron_get_all_alarm_info_err_table;
    }
    check_sum = divoom_app_com_get_check_sum((uint8 *)&number, sizeof(number));
    for(i = 0; i < number; i++)
    {
        data_read = divoom_system_malloc(sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
        read_bytes = divoom_port_file_read(data_read, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO), 1, fd);
        if(read_bytes == 0)
        {
            break;
        }
        divoom_queue_push(dialy_alarm_queue, data_read);   
        check_sum += divoom_app_com_get_check_sum((uint8 *)data_read, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));     
    }
    if(0 == divoom_port_file_read(&raw_check_sum, sizeof(raw_check_sum), 1, fd))
    {
        goto divoom_alarm_poweron_get_all_alarm_info_err_table;
    }
	divoom_port_file_close(fd);
    if(raw_check_sum != check_sum)
    {
        //所有闹钟无效
        divoom_alarm_poweron_get_all_alarm_info_err_table:
        divoom_queue_pop_all(dialy_alarm_queue, NULL);
//        pdivoom_alarm_handle->next_active_id = -1;
		return 0;
    }
	return 1;
}
#endif
void divoom_alarm_save_all_alarm_info_local(void)
{
    int i, number;
	FILE *fd = NULL;
	T_DIVOOM_ALARM_ACTIVE_DO_INFO *data_read;

    divoom_print("save config!\n");
    number = divoom_queue_items(pdivoom_alarm_handle->dialy_alarm_queue);
	if((fd = divoom_port_file_open(DIVOOM_ALARM_INFO_PATH, "w+")) == NULL)
	{
		divoom_print("Some Wrong in %d, maybe no this file:%s\n", __LINE__, DIVOOM_ALARM_INFO_PATH);
		return;
	}
	if(number)
	{
        char *json_string = NULL;

        divoom_cJSON *root, *list, *item;

        root = divoom_cJSON_CreateObject();
        list = divoom_cJSON_CreateArray();
        divoom_cJSON_AddItemToObject(root, "AlarmList", list);
        for(i = 0; i < number; i ++)
        {
            data_read = divoom_queue_get_index_item(pdivoom_alarm_handle->dialy_alarm_queue, i);
            if(data_read)
            {
                item = divoom_cJSON_CreateObject();

                divoom_cJSON_AddStringToObject(item, "alarm_name", data_read->alarm_name);
                divoom_cJSON_AddNumberToObject(item, "active_status", data_read->active_status);
                divoom_cJSON_AddNumberToObject(item, "on_off", data_read->on_off);
                divoom_cJSON_AddNumberToObject(item, "type", data_read->type);
                divoom_cJSON_AddNumberToObject(item, "index", data_read->index);
                divoom_cJSON_AddNumberToObject(item, "cyc_flag", data_read->cyc_flag);
                divoom_cJSON_AddNumberToObject(item, "month", data_read->month);
                divoom_cJSON_AddNumberToObject(item, "day", data_read->day);
                divoom_cJSON_AddNumberToObject(item, "minute", data_read->minute);
                divoom_cJSON_AddNumberToObject(item, "hour", data_read->hour);
                divoom_cJSON_AddNumberToObject(item, "year", data_read->year);
                divoom_cJSON_AddNumberToObject(item, "early_flag", data_read->early_flag);
                divoom_cJSON_AddNumberToObject(item, "alarm_id", data_read->alarm_id);
                divoom_cJSON_AddStringToObject(item, "image_data", data_read->image_data);
                divoom_cJSON_AddNumberToObject(item, "update_time", data_read->update_time);
                divoom_cJSON_AddNumberToObject(item, "mode", data_read->mode);
                divoom_cJSON_AddNumberToObject(item, "volume", data_read->volume);
                divoom_cJSON_AddStringToObject(item, "leave_url", data_read->leave_url);
                divoom_cJSON_AddNumberToObject(item, "fm", data_read->fm*10);
                divoom_cJSON_AddNumberToObject(item, "tip_index", data_read->tip_index);
                divoom_cJSON_AddNumberToObject(item, "scene_index", data_read->scene_index);
                divoom_cJSON_AddNumberToObject(item, "alarm_set_time", data_read->alarm_set_time);
                divoom_cJSON_AddItemToArray(list, item);
            }
        }
        json_string = divoom_cJSON_Print(root);
        divoom_cJSON_Delete(root);
        //divoom_app_com_write_file(DIVOOM_ALARM_INFO_PATH,json_string, strlen(json_string));
        if (json_string)
		{
			divoom_port_file_write(json_string, strlen(json_string), 1, fd);
			divoom_system_free(json_string);
		}
		divoom_port_file_close(fd);
	}
    else
    {
    	divoom_port_file_close(fd);
        divoom_app_com_del_file(DIVOOM_ALARM_INFO_PATH);
    }
	
}

void divoom_alarm_save_all_alarm_info(void)
{
    divoom_alarm_save_all_alarm_info_local();
}

void divoom_alarm_get_all_alarm_info(void)
{
	T_DIVOOM_ALARM_ACTIVE_DO_INFO *data_read;
    int i, file_len = 0, ret = 0;
	FILE *fd = NULL;
	uint8 *read_buf = NULL;
	divoom_cJSON *root, *list,*item;
	
	if((fd = divoom_port_file_open(DIVOOM_ALARM_INFO_PATH, "r")) == NULL)
	{
		divoom_print("Some Wrong in %d: Maybe no this file:%s!\n", __LINE__, DIVOOM_ALARM_INFO_PATH);
		return ;
	}

	divoom_port_file_seek(fd, 0, SEEK_END);
	file_len = divoom_port_file_tell(fd);
	divoom_port_file_seek(fd, 0, SEEK_SET);
	read_buf = divoom_system_malloc(file_len + 1);
	ret = divoom_port_file_read(read_buf, file_len, 1, fd);
	
	root = divoom_cJSON_Parse((char *)read_buf);
	divoom_system_free(read_buf);

	if (root == NULL)
	{
		divoom_port_file_close(fd);
		return ;
	}
	
    list = divoom_cJSON_GetObjectItem(root, "AlarmList");
    if(list == NULL || divoom_cJSON_GetArraySize(list) == 0)
    {
        divoom_cJSON_Delete(root);
        return;
    }
    for(i = 0; i < divoom_cJSON_GetArraySize(list); i++)
    {
        item  = divoom_cJSON_GetArrayItem(list, i);
        if(item == NULL)
        {
            break;
        }
        data_read = divoom_system_malloc(sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
        if(data_read == NULL)
        {
            break;
        }
        divoom_cJSON_GetObjectString(item, "alarm_name", data_read->alarm_name);
        data_read->active_status = divoom_cJSON_GetObjectInt(item, "active_status");
        data_read->on_off = divoom_cJSON_GetObjectInt(item, "on_off");
        data_read->type = divoom_cJSON_GetObjectInt(item, "type");
        data_read->index = divoom_cJSON_GetObjectInt(item, "index");
        data_read->cyc_flag = divoom_cJSON_GetObjectInt(item, "cyc_flag");
#if DIVOOM_SHOW_MODE
        divoom_alarm_show_mode_strip_week_cyc(data_read);
#endif
        data_read->month = divoom_cJSON_GetObjectInt(item, "month");
        data_read->day = divoom_cJSON_GetObjectInt(item, "day");
        data_read->hour = divoom_cJSON_GetObjectInt(item, "hour");
        data_read->minute = divoom_cJSON_GetObjectInt(item, "minute");
        data_read->year = divoom_cJSON_GetObjectInt(item, "year");
        data_read->early_flag = divoom_cJSON_GetObjectInt(item, "early_flag");
        data_read->alarm_id = divoom_cJSON_GetObjectInt(item, "alarm_id");
        divoom_cJSON_GetObjectString(item, "image_data", data_read->image_data);
        data_read->update_time = divoom_cJSON_GetObjectInt(item, "update_time");
        data_read->mode = divoom_cJSON_GetObjectInt(item, "mode");
        data_read->volume = divoom_cJSON_GetObjectInt(item, "volume");
        divoom_cJSON_GetObjectString(item, "leave_url", data_read->leave_url);
        data_read->fm = divoom_cJSON_GetObjectInt(item, "fm");
        data_read->fm = data_read->fm/10;
        data_read->tip_index = divoom_cJSON_GetObjectInt(item, "tip_index");
        data_read->scene_index = divoom_cJSON_GetObjectInt(item, "scene_index");		
        data_read->alarm_set_time = divoom_cJSON_GetObjectInt(item, "alarm_set_time");
#ifdef DIVOOM_TIME_FRAME_SPEAKER
        divoom_alarm_normalize_tf_leave_url(data_read);
#endif
        divoom_queue_push(pdivoom_alarm_handle->dialy_alarm_queue, data_read);

    }
	divoom_port_file_close(fd);
	divoom_cJSON_Delete(root);
}

int divoom_alarm_poweron_get_all_alarm_info(void *dialy_alarm_queue)
{
	T_DIVOOM_ALARM_ACTIVE_DO_INFO *data_read;
    int i, file_len = 0, ret = 0;
	FILE *fd = NULL;
	uint8 *read_buf = NULL;
	divoom_cJSON *root, *list,*item;

	if((fd = divoom_port_file_open(DIVOOM_ALARM_INFO_PATH, "r")) == NULL)
	{
		divoom_print("Some Wrong in %d: Maybe no this file:%s!\n", __LINE__, DIVOOM_ALARM_INFO_PATH);
		return 0;
	}
	
	divoom_port_file_seek(fd, 0, SEEK_END);
	file_len = divoom_port_file_tell(fd);
	divoom_port_file_seek(fd, 0, SEEK_SET);
	read_buf = divoom_system_malloc(file_len + 1);
	ret = divoom_port_file_read(read_buf, file_len, 1, fd);
	
	root = divoom_cJSON_Parse((char *)read_buf);
	divoom_system_free(read_buf);

	if (root == NULL)
	{
		divoom_port_file_close(fd);
		return 0;
	}
	
    list = divoom_cJSON_GetObjectItem(root, "AlarmList");
    if(list == NULL || divoom_cJSON_GetArraySize(list) == 0)
    {
        divoom_cJSON_Delete(root);
        return 0;
    }
    for(i = 0; i < divoom_cJSON_GetArraySize(list); i++)
    {
        item  = divoom_cJSON_GetArrayItem(list, i);
        if(item == NULL)
        {
            break;
        }
        data_read = divoom_system_malloc(sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
        if(data_read == NULL)
        {
            break;
        }
        divoom_cJSON_GetObjectString(item, "alarm_name", data_read->alarm_name);
        data_read->active_status = divoom_cJSON_GetObjectInt(item, "active_status");
        data_read->on_off = divoom_cJSON_GetObjectInt(item, "on_off");
        data_read->type = divoom_cJSON_GetObjectInt(item, "type");
        data_read->index = divoom_cJSON_GetObjectInt(item, "index");
        data_read->cyc_flag = divoom_cJSON_GetObjectInt(item, "cyc_flag");
#if DIVOOM_SHOW_MODE
        divoom_alarm_show_mode_strip_week_cyc(data_read);
#endif
        data_read->month = divoom_cJSON_GetObjectInt(item, "month");
        data_read->day = divoom_cJSON_GetObjectInt(item, "day");
        data_read->hour = divoom_cJSON_GetObjectInt(item, "hour");
        data_read->minute = divoom_cJSON_GetObjectInt(item, "minute");
        data_read->year = divoom_cJSON_GetObjectInt(item, "year");
        data_read->early_flag = divoom_cJSON_GetObjectInt(item, "early_flag");
        data_read->alarm_id = divoom_cJSON_GetObjectInt(item, "alarm_id");
        divoom_cJSON_GetObjectString(item, "image_data", data_read->image_data);
        data_read->update_time = divoom_cJSON_GetObjectInt(item, "update_time");
        data_read->mode = divoom_cJSON_GetObjectInt(item, "mode");
        data_read->volume = divoom_cJSON_GetObjectInt(item, "volume");
        divoom_cJSON_GetObjectString(item, "leave_url", data_read->leave_url);
        data_read->fm = divoom_cJSON_GetObjectInt(item, "fm");
        data_read->fm = data_read->fm/10;
        data_read->tip_index = divoom_cJSON_GetObjectInt(item, "tip_index");
        data_read->scene_index = divoom_cJSON_GetObjectInt(item, "scene_index");
        data_read->alarm_set_time = divoom_cJSON_GetObjectInt(item, "alarm_set_time");
#ifdef DIVOOM_TIME_FRAME_SPEAKER
        divoom_alarm_normalize_tf_leave_url(data_read);
#endif
        divoom_queue_push(dialy_alarm_queue, data_read);

    }
	divoom_port_file_close(fd);
	divoom_cJSON_Delete(root);
	
	return 1;
}



uint32 divoom_alarm_poweron_lib_get_next_active_alarm_time(void *dialy_alarm_queue)
{
    T_U8 i, find_alarm_index = 0xFF, find_dialy_index = 0XFF/*, alarm_cyc_flag = 0, dialy_cyc_flag= 0*/;
    divoom_alarm_system_date cur_time, alarm_time, dialy_time;
	T_DIVOOM_ALARM_ACTIVE_DO_INFO *alarm_info, find_info, *dialy_info, find_dialy_info;
	T_U8 ret = 0;
	int next_active_id = -1;
	time_t next_active_time = 0;

	memset(&find_info, 0, sizeof(find_info));
	memset(&find_dialy_info, 0, sizeof(find_dialy_info));

	if(NULL == dialy_alarm_queue)
	{
		divoom_print("%s: null queue!\n", __func__);
		return 0;
	}

    divoom_alarm_get_sys_time(&cur_time);
	divoom_print("cur_time:%d:%d:%d:%d:%d", cur_time.year + 1900, cur_time.month + 1, cur_time.day, cur_time.hour, cur_time.minute);
    //先得到最近的一个闹钟的信息
    for(i = 0; i < divoom_queue_items(dialy_alarm_queue); i ++)
    {
        alarm_info = divoom_queue_get_index_item(dialy_alarm_queue, i);
        if(alarm_info == NULL)
        {
            break;
        }
        if(alarm_info->on_off != 1 || alarm_info->type != DIVOOM_ALARM_TYPE_ALARM)
        {
            continue;
        }
        
        if(find_alarm_index == 0xFF)
        {
            find_alarm_index = i;
            memcpy(&find_info, alarm_info, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
            divoom_alarm_lib_convert_alarm_time_to_local(&find_info, &alarm_time, &cur_time);
            continue;
        }
        
		ret = divoom_alarm_lib_compare_alarm_time(&find_info, alarm_info, &cur_time, &alarm_time);
        if(ret == 2)//时间相同的闹钟
        {
		}
		else if(ret == 1)
		{
			find_alarm_index = i;
			memcpy(&find_info, alarm_info, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
		}
    }

	divoom_print("find_alarm_index = %d\n", find_alarm_index);
	if (find_alarm_index != 0xFF)
		divoom_print("find_info.hour = %d, find_info.minute = %d, \n", find_info.hour, find_info.minute);
    //再得到最近的一个纪念日信息

    for(i = 0; i < divoom_queue_items(dialy_alarm_queue); i ++)
    {

        dialy_info = divoom_queue_get_index_item(dialy_alarm_queue, i);
        if(dialy_info == NULL)
        {
            break;
        }
        if(dialy_info->on_off != 1 || dialy_info->type != DIVOOM_ALARM_TYPE_DIALY)
        {
            continue;
        }

        if(find_dialy_index == 0xFF)
        {
            find_dialy_index = i;
            memcpy(&find_dialy_info, dialy_info, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
            divoom_alarm_lib_convert_dialy_time_to_local(&find_dialy_info, &dialy_time, &cur_time);
            continue;
        }
        ret = divoom_alarm_lib_compare_dialy_time(&find_dialy_info, dialy_info, &cur_time, &dialy_time);
		if(ret == 2)
        {
			find_dialy_index = i;
			memcpy(&find_dialy_info, dialy_info, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
        }
		else if( ret == 1)
		{
			find_dialy_index = i;
			memcpy(&find_dialy_info, dialy_info, sizeof(T_DIVOOM_ALARM_ACTIVE_DO_INFO));
		}
    }


    if(find_dialy_index == 0xFF && find_alarm_index == 0XFF)
    {
        //没有激活的闹钟信息
        divoom_print("%s: all alarm and dialy is unactive!\n", __func__);
        next_active_time = 0;
        next_active_id = -1;
		return 0;
    }
    else if(find_dialy_index != 0xFF && find_alarm_index != 0XFF)
    {
		ret = divoom_alarm_lib_compare_local_time(&alarm_time, &dialy_time);
        //比较闹钟和纪念日
		if(ret == 2)
		{
            next_active_id = find_alarm_index;
            next_active_time = divoom_alarm_time_to_sec(&alarm_time);
		}
        else if(ret == 1)
        {
            next_active_id = find_dialy_index;
            next_active_time = divoom_alarm_time_to_sec(&dialy_time);
		}
        else
        {
            next_active_id = find_alarm_index;
            next_active_time = divoom_alarm_time_to_sec(&alarm_time);
        }
    }
    else if(find_dialy_index == 0xFF)
    {
        next_active_id = find_alarm_index;
        next_active_time = divoom_alarm_time_to_sec(&alarm_time);
	}
    else
    {
        next_active_id = find_dialy_index;
        next_active_time = divoom_alarm_time_to_sec(&dialy_time);
	}
    alarm_info = divoom_queue_get_index_item(dialy_alarm_queue, next_active_id);
    
//    next_active_light_time = next_active_time;
//	*active_time = next_active_time;

	int early_time = 0;
	uint32 result = 0;

	if(alarm_info->early_flag)
	{
		early_time = DIVOOM_ALARM_HW_WAKE_EARLY_HEAVY_SEC;
	}
	else
	{
		early_time = DIVOOM_ALARM_HW_WAKE_EARLY_SEC;
	}
	if(next_active_time > time(0) + early_time)
	{
		divoom_print("we will start up with early : %d\n", early_time);
		result = (uint32 )next_active_time - early_time; 
	}
	else
	{
		divoom_print("we will start up with early after 10s\n");
		result = time(0) + 10;
	}
	return result;    
}

/*********************************************************************
  Function:        divoom_alarm_get_next_rtc_wake_unix
  Description:     闹钟模块下一次应对 RTC 写入的唤醒时刻（含提前开机量）
**********************************************************************/
time_t divoom_alarm_get_next_rtc_wake_unix(void)
{
#ifndef DIVOOM_TIME_FRAME_SPEAKER
	return (time_t)0;
#else
	time_t t_next, t_hw;
	int early_time;
	T_DIVOOM_ALARM_ACTIVE_DO_INFO *alarm_info;

	if (pdivoom_alarm_handle == NULL)
		return (time_t)0;

	divoom_alarm_lib_set_next_active_alarm();

	if (pdivoom_alarm_handle->next_active_id < 0 ||
	    pdivoom_alarm_handle->next_active_time == 0)
		return (time_t)0;

	t_next = (time_t)pdivoom_alarm_handle->next_active_time;
	if (t_next <= time(NULL))
		return (time_t)0;

	alarm_info = divoom_queue_get_index_item(pdivoom_alarm_handle->dialy_alarm_queue,
						 pdivoom_alarm_handle->next_active_id);
	if (alarm_info == NULL)
		return (time_t)0;

	if (alarm_info->early_flag)
		early_time = DIVOOM_ALARM_HW_WAKE_EARLY_HEAVY_SEC;
	else
		early_time = DIVOOM_ALARM_HW_WAKE_EARLY_SEC;

	if (t_next > time(NULL) + early_time)
		t_hw = t_next - (time_t)early_time;
	else
		t_hw = time(NULL) + 10;

	return t_hw;
#endif
}

#endif



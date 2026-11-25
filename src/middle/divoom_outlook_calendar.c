#include "divoom.h"
#include <time.h>
#include "divoom_menu_control.h"
#include "divoom_calendar_today_watch_layout.h"

#if 1
#define DIVOOM_OUTLOOK_CALENDAR_EVENT_URL "https://graph.microsoft.com/v1.0/me/events?$top=999&$orderby%3Dstart%2FdateTime%20asc&$filter%3Dstart%2FdateTime%20ge%20'"
#define DIVOOM_OUTLOOK_CALENDAR_EVENT_URL2 "T00%3A00%3A00Z'%20and%20end%2FdateTime%20le%20'"
#define DIVOOM_OUTLOOK_CALENDAR_EVENT_URL3 "T23%3A59%3A59Z'&select=subject,bodyPreview,start,end,recurrence,reminderMinutesBeforeStart,isReminderOn,isAllDay,lastModifiedDateTime,categories,calendar"
#define DIVOOM_OUTLOOK_CALENDAR_COLOR_URL "https://graph.microsoft.com/v1.0/me/outlook/masterCategories"
#define DIVOOM_OUTLOOK_CALENDAR_DEFAULT_COLOR_URL "https://graph.microsoft.com/v1.0/me/calendars"
#define DIVOOM_OUTLOOK_CALENDAR_REQUEST_EVENT_LIST_TIME  30
#define DIVOOM_OUTLOOK_CALENDAR_GRAPH_FAIL_BACKOFF 300
#define DIVOOM_OUTLOOK_CONFIG_POLL_MAX_FAIL       4

static const int divoom_outlook_config_poll_intervals[] = { 5, 30, 60 };

#define DIVOOM_OUTLOOK_CALENDAR_POPUP_W        (DIVOOM_SCREEN_WIDTH - 32)
#define DIVOOM_OUTLOOK_CALENDAR_POPUP_H        96
#define DIVOOM_OUTLOOK_CALENDAR_POPUP_RADIUS   20
#define DIVOOM_OUTLOOK_CALENDAR_POPUP_ICON_SZ  56
#define DIVOOM_OUTLOOK_CALENDAR_POPUP_PAD      12
/* 避开弹窗圆角裁剪区（radius=20 时 x>=40 才完整显示） */
#define DIVOOM_OUTLOOK_CALENDAR_POPUP_ICON_X   (DIVOOM_OUTLOOK_CALENDAR_POPUP_RADIUS * 2)
#define DIVOOM_OUTLOOK_CALENDAR_POPUP_TEXT_GAP 12
#define DIVOOM_OUTLOOK_CALENDAR_POPUP_TEXT_COLOR 0x000000
#define DIVOOM_OUTLOOK_CALENDAR_POPUP_TIME_H   22
#define DIVOOM_OUTLOOK_CALENDAR_POPUP_TITLE_H  36
#define DIVOOM_OUTLOOK_CALENDAR_POPUP_TEXT_ROW_GAP 4
#define DIVOOM_OUTLOOK_CALENDAR_LOG_SRC_W      125
#define DIVOOM_OUTLOOK_CALENDAR_LOG_SRC_H      116

enum{
    DIVOOM_OUTLOOK_CALENDAR_STATUS_IDLE,
    DIVOOM_OUTLOOK_CALENDAR_STATUS_NOTICE,
    DIVOOM_OUTLOOK_CALENDAR_STATUS_ACTIVE,
    DIVOOM_OUTLOOK_CALENDAR_STATUS_PASS,
    DIVOOM_OUTLOOK_CALENDAR_STATUS_TIME_OUT,
};

void divoom_outlook_calendar_send_event_notice_message(void *data, int status);


enum{
    DIVOOM_OUTLOOK_CALENDAR_REPEAT_NONE,
    DIVOOM_OUTLOOK_CALENDAR_REPEAT_DAY,     //按天重复
    DIVOOM_OUTLOOK_CALENDAR_REPEAT_WEEK,    //按周
    DIVOOM_OUTLOOK_CALENDAR_REPEAT_MON,     //按月
    DIVOOM_OUTLOOK_CALENDAR_REPEAT_YEAR,    //按年
};
    
// Outlook Category Colors RGB Values
const uint32_t divoom_outlook_calendar_category_color_list[] = {
    
    0x00FF0000,  // Preset0 - Red
    0x00FFA500,  // Preset1 - Orange
    0x008B4513,  // Preset2 - Brown
    0x00FFFF00,  // Preset3 - Yellow
    0x0000FF00,  // Preset4 - Green
    0x00008080,  // Preset5 - Teal
    0x00808000,  // Preset6 - Olive
    0x000000FF,  // Preset7 - Blue
    0x00800080,  // Preset8 - Purple
    0x00DC143C,  // Preset9 - Cranberry
    0x00C0C0C0,  // Preset10 - Steel
    0x00808080,  // Preset11 - DarkSteel
    0x00808080,  // Preset12 - Gray
    0x00404040,  // Preset13 - DarkGray
    0x00000000,  // Preset14 - Black
    0x008B0000,  // Preset15 - DarkRed
    0x00FF8C00,  // Preset16 - DarkOrange
    0x00654321,  // Preset17 - DarkBrown
    0x00B8860B,  // Preset18 - DarkYellow
    0x00006400,  // Preset19 - DarkGreen
    0x00004040,  // Preset20 - DarkTeal
    0x00556B2F,  // Preset21 - DarkOlive
    0x0000008B,  // Preset22 - DarkBlue
    0x004B0082,  // Preset23 - DarkPurple
    0x008B0000   // Preset24 - DarkCranberry
};

typedef struct _divoom_outlook_calendar_color_item
{
    int color_index;
    char *color_name;
    char *id;
}divoom_outlook_calendar_color_item, *Pdivoom_outlook_calendar_color_item;

typedef struct _divoom_outlook_calendar_default_color_item
{
    uint32 color_value;
    char *id;
}divoom_outlook_calendar_default_color_item, *Pdivoom_outlook_calendar_default_color_item;

typedef struct _divoom_outlook_calendar_event_item
{
    uint8 status;
    uint8 notice_flag;
    uint8 all_day_flag;
    uint32 bg_color;
    void *lvgl_obj;
    char *event_id;
    char *update_time;
    char *title;        //事件标题
    char *desc;         //事件描述
    uint8 repeat_type;  //重复类型
    uint8 week_repeat[7];   //周期重复
    time_t  start_utc;     //默认开始时间
    time_t  end_utc;       //默认结束时间
    time_t  next_start_utc;     //激活后开始时间
    time_t  next_end_utc;       //激活后结束时间
    time_t  range_start_utc;     //事件开始日期 
    time_t  range_end_utc;       //事件结束日期
    int  reminder_time; //提前提醒时间
    int  notice_reminder_time; //提前提醒UTC
}divoom_outlook_calendar_event_item, *Pdivoom_outlook_calendar_event_item;


typedef struct _divoom_outlook_calendar_cal_ctrl
{
    uint8   update_default_color_flag;
    uint8   menu_flag;    // 是否显示菜单
    uint8   pop_flag;    // 是否有弹窗提醒
    uint8   event_flag;    //是否事件通知功能
    uint8   update_flag;    //是否有变化，这个读清
    uint8   watch_refresh_flag; //表盘刷新请求
    int     event_num;
    int     event_request_time;   //更新时间
    int     graph_backoff_until;
    uint8   config_poll_fail_stage;
    uint8   config_poll_stopped;
    time_t  config_poll_next_time;
    void  *run_sem;
    char  *token;
    void    *color_queue;
    void    *default_color_queue;
    void   *lock_handle;
    Pdivoom_outlook_calendar_event_item pevent_list;      //日历列表
    Pdivoom_outlook_calendar_event_item prunning_event;      //正在激活的日历
    void    *notice_lvgl_obj;
}divoom_outlook_calendar_cal_ctrl, *Pdivoom_outlook_calendar_cal_ctrl;

void divoom_outlook_calendar_send_event_notice_message(void *data, int status);

Pdivoom_outlook_calendar_cal_ctrl pdivoom_outlook_calendar_cal_handle = NULL;

void divoom_outlook_calendar_load_config(void)
{
    divoom_cJSON *root,*item;
    char *file_buf;

    file_buf = divoom_app_com_read_file(DIVOOM_OUTLOOK_CALENDAR_CONFIG, NULL);
    if(file_buf == NULL)
    {
        return ;
    }
    root = divoom_cJSON_Parse(file_buf);
    divoom_system_free(file_buf);
    if(root == NULL)
    {
        return  ;
    }

    pdivoom_outlook_calendar_cal_handle->pop_flag = divoom_cJSON_GetObjectInt(root, "pop_flag");
    pdivoom_outlook_calendar_cal_handle->event_flag = divoom_cJSON_GetObjectInt(root, "event_flag");
	item = divoom_cJSON_GetObjectItem(root, "token");
    if(item && item->valuestring)
    {
        pdivoom_outlook_calendar_cal_handle->token = divoom_app_com_copy_string(item->valuestring, -1);
    }
    return ;
}



void divoom_outlook_calendar_save_config(void)
{
    char *json_info = NULL;
    divoom_cJSON *root;
	root = divoom_cJSON_CreateObject();

    if(root == NULL || pdivoom_outlook_calendar_cal_handle == NULL)
    {
        return;
    }
    
    
	divoom_cJSON_AddNumberToObject(root, "pop_flag", pdivoom_outlook_calendar_cal_handle->pop_flag);
	divoom_cJSON_AddNumberToObject(root, "event_flag", pdivoom_outlook_calendar_cal_handle->event_flag);
    if(pdivoom_outlook_calendar_cal_handle->token)
    {
        divoom_cJSON_AddStringToObject(root , "token", pdivoom_outlook_calendar_cal_handle->token);
    }
    json_info = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);
    if (json_info != NULL)
    {
        divoom_app_com_write_file(DIVOOM_OUTLOOK_CALENDAR_CONFIG, (uint8 *)json_info, strlen(json_info));
        divoom_system_free(json_info);
    }
}

static void divoom_outlook_calendar_save_config_values(int pop_flag, int event_flag, char *token)
{
	char *json_info = NULL;
	divoom_cJSON *root = divoom_cJSON_CreateObject();

	if (root == NULL || token == NULL || strlen(token) == 0)
		return;
	if (pop_flag != -1)
		divoom_cJSON_AddNumberToObject(root, "pop_flag", pop_flag);
	else
		divoom_cJSON_AddNumberToObject(root, "pop_flag", 1);
	if (event_flag != -1)
		divoom_cJSON_AddNumberToObject(root, "event_flag", event_flag);
	else
		divoom_cJSON_AddNumberToObject(root, "event_flag", 1);
	divoom_cJSON_AddStringToObject(root, "token", token);
	json_info = divoom_cJSON_Print(root);
	divoom_cJSON_Delete(root);
	if (json_info != NULL)
	{
		divoom_app_com_write_file(DIVOOM_OUTLOOK_CALENDAR_CONFIG, (uint8 *)json_info, strlen(json_info));
		divoom_system_free(json_info);
	}
}
/*
{
    "error": {
        "code": "InvalidAuthenticationToken",
        "message": "IDX14100: JWT is not well formed, there are no dots (.).\nThe token needs to be in JWS or JWE Compact Serialization Format. (JWS): 'EncodedHeader.EncodedPayload.EncodedSignature'. (JWE): 'EncodedProtectedHeader.EncodedEncryptedKey.EncodedInitializationVector.EncodedCiphertext.EncodedAuthenticationTag'.",
        "innerError": {
            "date": "2025-07-12T07:23:01",
            "request-id": "921730aa-2666-45d0-8982-caeb3f78426b",
            "client-request-id": "921730aa-2666-45d0-8982-caeb3f78426b"
        }
    }
}
*/

/*********************************************************************
Function:         divoom_outlook_calendar_free_all_cal
Description:      释放日历资源
Input:            data
Return:           
Author:           albert
**********************************************************************/

void divoom_outlook_calendar_free_all_event(Pdivoom_outlook_calendar_event_item pevent_list, int event_num)
{
    for(int i= 0; i < event_num; i++)
    {
        divoom_system_free(pevent_list[i].event_id);
        divoom_system_free(pevent_list[i].update_time);
        divoom_system_free(pevent_list[i].title);
        divoom_system_free(pevent_list[i].desc);
    }
    divoom_system_free(pevent_list);
    
}

int divoom_outlook_calendar_getDaysInMonth(int year, int month) {
    // 定义每个月的天数数组（非闰年）
    const int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    // 检查月份是否有效
    if (month < 1 || month > 12) {
        return -1;  // 返回-1表示无效月份
    }
    
    // 如果是2月，需要检查是否闰年
    if (month == 2) {
        // 闰年规则：能被4整除且不能被100整除，或者能被400整除
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            return 29;
        }
    }
    
    return daysInMonth[month - 1];
}

void divoom_outlook_calendar_get_next_mday(struct tm *cur_date)
{
    int days = 0;
    
    for(int i = 0; i < 12; i++)
    {        
        cur_date->tm_mon ++;
        if(cur_date->tm_mon >12)
        {
            cur_date->tm_year ++;
            cur_date->tm_mon = 1;
        }
        days = divoom_outlook_calendar_getDaysInMonth(cur_date->tm_year + 1900, cur_date->tm_mon);
        if(days >= cur_date->tm_mday)
        {
            return;
        }
    }
}


void divoom_outlook_calendar_formart_event_item(Pdivoom_outlook_calendar_event_item pevent_item)
{
    int today_sec;
    time_t cur_time;
    struct tm cur_date = {0}, start_date = {0};
    

    cur_time = time(0);
    if( pevent_item->range_end_utc && cur_time >= pevent_item->range_end_utc)
    {
        //如果超过结束的，将丢弃
        pevent_item->status = DIVOOM_OUTLOOK_CALENDAR_STATUS_TIME_OUT;
        return;
    }
    
    if(cur_time < pevent_item->range_start_utc)
    {
        //如果还没有到的话，直接使用开始时间
        pevent_item->next_start_utc = pevent_item->start_utc;
        pevent_item->next_end_utc = pevent_item->end_utc;
        pevent_item->status = DIVOOM_OUTLOOK_CALENDAR_STATUS_IDLE;
        goto divoom_outlook_calendar_formart_event_item_get_reminder_label;
    }
    
    switch(pevent_item->repeat_type)
    {
        case DIVOOM_OUTLOOK_CALENDAR_REPEAT_NONE:
            if(cur_time >= pevent_item->start_utc)
            {
                //已经启动了的事件，丢弃
                pevent_item->status = DIVOOM_OUTLOOK_CALENDAR_STATUS_TIME_OUT;
            }
            pevent_item->next_start_utc = pevent_item->start_utc;
            pevent_item->next_end_utc = pevent_item->end_utc;
                
            break;
            
        case DIVOOM_OUTLOOK_CALENDAR_REPEAT_DAY:
            //取今天或明天的时间
            localtime_r(&cur_time, &cur_date);
            localtime_r(&pevent_item->start_utc, &start_date);
            today_sec = cur_date.tm_hour * 3600 + cur_date.tm_min * 60 + cur_date.tm_sec;
            cur_date.tm_hour = start_date.tm_hour;
            cur_date.tm_min = start_date.tm_min;
            cur_date.tm_sec = start_date.tm_sec;
            pevent_item->next_start_utc = mktime(&cur_date);
            if(start_date.tm_hour * 3600 + start_date.tm_min * 60 + start_date.tm_sec > today_sec)
            {
                //使用今天的时间,就不用动了
            }
            else
            {
                //今天的时间已经过了，将使用明天的时间
                pevent_item->next_start_utc += 24*3600;
            }
            break;
            
        case DIVOOM_OUTLOOK_CALENDAR_REPEAT_WEEK:            
            localtime_r(&cur_time, &cur_date);
            localtime_r(&pevent_item->start_utc, &start_date);
            today_sec = cur_date.tm_hour * 3600 + cur_date.tm_min * 60 + cur_date.tm_sec;
            cur_date.tm_hour = start_date.tm_hour;
            cur_date.tm_min = start_date.tm_min;
            cur_date.tm_sec = start_date.tm_sec;
            pevent_item->next_start_utc = mktime(&cur_date);
            if(pevent_item->week_repeat[cur_date.tm_wday])
            {
                //今天要执行
                if(start_date.tm_hour * 3600 + start_date.tm_min * 60 + start_date.tm_sec > today_sec)
                {
                    //使用今天的时间,就不用动了
                }
                else
                {
                    //今天的时间已经过了，将使用下一个周期的时间
                    for(int i = 0; i < 7; i++)
                    {
                        pevent_item->next_start_utc += 24*3600;
                        if(pevent_item->week_repeat[(cur_date.tm_wday + i + 1)%7])
                        {
                            break;
                        }
                    }
                }
            }
            else
            {
                //今天不在循环中，将使用下一个周期的时间
                for(int i = 0; i < 7; i++)
                {
                    pevent_item->next_start_utc += 24*3600;
                    if(pevent_item->week_repeat[(cur_date.tm_wday + i + 1)%7])
                    {
                        break;
                    }
                }
            }
            break;
            
        case DIVOOM_OUTLOOK_CALENDAR_REPEAT_MON:         
            localtime_r(&cur_time, &cur_date);
            localtime_r(&pevent_item->start_utc, &start_date);
            today_sec = cur_date.tm_hour * 3600 + cur_date.tm_min * 60 + cur_date.tm_sec;
            cur_date.tm_hour = start_date.tm_hour;
            cur_date.tm_min = start_date.tm_min;
            cur_date.tm_sec = start_date.tm_sec;
            pevent_item->next_start_utc = mktime(&cur_date);
            if(start_date.tm_mday == cur_date.tm_mday)
            {
                if(start_date.tm_hour * 3600 + start_date.tm_min * 60 + start_date.tm_sec > today_sec)
                {
                    //如果是今天
                }
                else
                {
                    //下个月这一号
                    divoom_outlook_calendar_get_next_mday(&cur_date);
                    pevent_item->next_start_utc = mktime(&cur_date);
                }
            }
            else if(start_date.tm_mday > cur_date.tm_mday)
            {
                //这个月这一号
                cur_date.tm_mday = start_date.tm_mday;
                pevent_item->next_start_utc = mktime(&cur_date);
            }
            else
            {
                //下个月这一号
                cur_date.tm_mday = start_date.tm_mday;
                divoom_outlook_calendar_get_next_mday(&cur_date);
                pevent_item->next_start_utc = mktime(&cur_date);
            }
            break;
            
        case DIVOOM_OUTLOOK_CALENDAR_REPEAT_YEAR:
            localtime_r(&cur_time, &cur_date);
            localtime_r(&pevent_item->start_utc, &start_date);
            today_sec = cur_date.tm_hour * 3600 + cur_date.tm_min * 60 + cur_date.tm_sec;
            cur_date.tm_hour = start_date.tm_hour;
            cur_date.tm_min = start_date.tm_min;
            cur_date.tm_sec = start_date.tm_sec;
            pevent_item->next_start_utc = mktime(&cur_date);
            if(cur_date.tm_mon == start_date.tm_mon && cur_date.tm_mday == start_date.tm_mday)
            {
                if(start_date.tm_hour * 3600 + start_date.tm_min * 60 + start_date.tm_sec > today_sec)
                {
                    //就是今天
                }
                else
                {
                    //明年这个月这号
                    cur_date.tm_year ++;
                    pevent_item->next_start_utc = mktime(&cur_date);
                }
            }
            else if(cur_date.tm_mon > start_date.tm_mon ||(cur_date.tm_mon == start_date.tm_mon && cur_date.tm_mday < start_date.tm_mday))
            {
                //今年这个月这号
                cur_date.tm_mon = start_date.tm_mon ;
                cur_date.tm_mday = start_date.tm_mday;
                pevent_item->next_start_utc = mktime(&cur_date);
            }
            else
            {
                //明年这个月这号
                cur_date.tm_year ++;
                pevent_item->next_start_utc = mktime(&cur_date);
            }
            break;
            
    }
    pevent_item->next_end_utc = pevent_item->next_start_utc + (pevent_item->end_utc - pevent_item->start_utc);
    if(pevent_item->range_end_utc && (pevent_item->range_end_utc < pevent_item->next_start_utc || pevent_item->range_end_utc < cur_time))
    {
        pevent_item->status = DIVOOM_OUTLOOK_CALENDAR_STATUS_TIME_OUT;
    }
    else if(pevent_item->range_end_utc == 0 && pevent_item->next_end_utc < cur_time)
    {
        pevent_item->status = DIVOOM_OUTLOOK_CALENDAR_STATUS_TIME_OUT;
    }
    else if(pevent_item->next_start_utc < cur_time)
    {
        pevent_item->status = DIVOOM_OUTLOOK_CALENDAR_STATUS_PASS;
    }
    else
    {
        pevent_item->status = DIVOOM_OUTLOOK_CALENDAR_STATUS_IDLE;
    }
    divoom_outlook_calendar_formart_event_item_get_reminder_label:
    pevent_item->notice_reminder_time = 0;
    if(pevent_item->status == DIVOOM_OUTLOOK_CALENDAR_STATUS_IDLE)
    {
        if(pevent_item->reminder_time)
        {
            pevent_item->notice_reminder_time = pevent_item->next_start_utc - pevent_item->reminder_time;
            if(pevent_item->notice_reminder_time < cur_time)
            {
                pevent_item->notice_reminder_time = 0;
            }
        }
    }
    divoom_print("event info: %s, %d, %d, %d\n", pevent_item->title, pevent_item->next_start_utc, pevent_item->next_end_utc, pevent_item->status);
    
}
time_t divoom_outlook_calendar_get_utc_time(const char *datetime) {
    struct tm tm = {0};
    int year, mon, day, hour, min, sec;

    if (datetime == NULL || datetime[0] == 0)
        return 0;

    if (strptime(datetime, "%Y-%m-%dT%H:%M:%S", &tm) == NULL)
    {
        sec = 0;
        hour = 0;
        min = 0;
        if (sscanf(datetime, "%d-%d-%dT%d:%d:%d", &year, &mon, &day, &hour, &min, &sec) >= 3)
        {
            tm.tm_year = year - 1900;
            tm.tm_mon = mon - 1;
            tm.tm_mday = day;
            tm.tm_hour = hour;
            tm.tm_min = min;
            tm.tm_sec = sec;
        }
        else if (sscanf(datetime, "%d-%d-%d", &year, &mon, &day) == 3)
        {
            tm.tm_year = year - 1900;
            tm.tm_mon = mon - 1;
            tm.tm_mday = day;
        }
        else
        {
            divoom_print("outlook parse time failed: %s\n", datetime);
            return 0;
        }
    }
    return divoom_timelib_std_mktime(&tm);
}

int divoom_outlook_calendar_check_event_update_time(divoom_cJSON *list)
{
    int i, j;
	divoom_cJSON *item, *data_obj, *update_obj;
    Pdivoom_outlook_calendar_event_item pevent_item;

    if(pdivoom_outlook_calendar_cal_handle->event_num != divoom_cJSON_GetArraySize(list))
    {
        return 1;
    }
    
    for(i = 0; i < divoom_cJSON_GetArraySize(list); i ++)
    {
        item = divoom_cJSON_GetArrayItem(list, i);
        
        data_obj  = divoom_cJSON_GetObjectItem(item, "id");
        if(data_obj && data_obj->valuestring)
        {
            update_obj  = divoom_cJSON_GetObjectItem(item, "lastModifiedDateTime");
            if(update_obj && update_obj->valuestring)
            {
                for(j = 0; j < pdivoom_outlook_calendar_cal_handle->event_num; j++)
                {
                    if(strcmp(pdivoom_outlook_calendar_cal_handle->pevent_list[j].event_id, data_obj->valuestring) == 0)
                    {
                        if(strcmp(pdivoom_outlook_calendar_cal_handle->pevent_list[j].update_time, update_obj->valuestring) != 0)
                        {
                            //时间不一样
                            return 1;
                        }
                        break;
                    }
                }
                if(j >= pdivoom_outlook_calendar_cal_handle->event_num)
                {
                    //新的
                    return 1;
                }
            }
        }
        
    }
    return 0;
}


void divoom_outlook_calendar_free_color_item(void *data)
{
    Pdivoom_outlook_calendar_color_item pitem = (Pdivoom_outlook_calendar_color_item)data;

    divoom_system_free(pitem->color_name);
    divoom_system_free(pitem->id);
    divoom_system_free(pitem);
}

void divoom_outlook_calendar_free_default_color_item(void *data)
{
    Pdivoom_outlook_calendar_default_color_item pitem = (Pdivoom_outlook_calendar_default_color_item)data;

    divoom_system_free(pitem->id);
    divoom_system_free(pitem);
}

int divoom_outlook_calendar_find_default_color_value_compare(void *item1, void *item2)
{
    char *color_name = (char *)item1;
    Pdivoom_outlook_calendar_default_color_item pcolor_item = (Pdivoom_outlook_calendar_default_color_item)item2;

    return strcmp(color_name, pcolor_item->id);
}

static int divoom_outlook_calendar_refresh_token_locked(const char *used_token)
{
    Pdivoom_net_command ret_data;
    Pdivoom_device_google_calendar_config_respone_info pinfo;
    int same = 0;

    if(used_token && pdivoom_outlook_calendar_cal_handle->token && strcmp(used_token, pdivoom_outlook_calendar_cal_handle->token))
    {
        divoom_print("outlook token already updated, retry\n");
        return 1;
    }
    divoom_system_free_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);
    ret_data = divoom_net_app_send_packet_http(DIVOOM_NET_COMM_DEVICE_GET_OUTLOOK_CALENDAR_CONFIG, NULL, 1);
    divoom_system_get_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);
    if(ret_data == NULL)
    {
        divoom_print("outlook token refresh fail\n");
        return 0;
    }
    pinfo = (Pdivoom_device_google_calendar_config_respone_info)ret_data->cmd_data;
    if(pinfo == NULL || strlen(pinfo->token) == 0)
    {
        divoom_print("outlook token refresh empty\n");
        divoom_net_app_free_packet(ret_data);
        return 0;
    }
    if(used_token && pdivoom_outlook_calendar_cal_handle->token && strcmp(used_token, pdivoom_outlook_calendar_cal_handle->token))
    {
        divoom_print("outlook token already updated after refresh wait, retry\n");
        divoom_net_app_free_packet(ret_data);
        return 1;
    }
    same = (pdivoom_outlook_calendar_cal_handle->token && strcmp(pdivoom_outlook_calendar_cal_handle->token, pinfo->token) == 0);
    if(!same)
    {
        if(pdivoom_outlook_calendar_cal_handle->token)
        {
            divoom_system_free(pdivoom_outlook_calendar_cal_handle->token);
            pdivoom_outlook_calendar_cal_handle->token = NULL;
        }
        pdivoom_outlook_calendar_cal_handle->token = divoom_app_com_copy_string(pinfo->token, -1);
        divoom_outlook_calendar_save_config();
    }
    divoom_print("outlook token refresh len=%d same=%d\n", (int)strlen(pinfo->token), same);
    divoom_net_app_free_packet(ret_data);
    return same ? 0 : 1;
}

static int divoom_outlook_calendar_graph_in_backoff(void)
{
    if(pdivoom_outlook_calendar_cal_handle == NULL)
        return 0;
    if(pdivoom_outlook_calendar_cal_handle->graph_backoff_until == 0)
        return 0;
    return time(0) < pdivoom_outlook_calendar_cal_handle->graph_backoff_until;
}

static void divoom_outlook_calendar_note_graph_transport_fail(void)
{
    pdivoom_outlook_calendar_cal_handle->graph_backoff_until = (int)time(0) + DIVOOM_OUTLOOK_CALENDAR_GRAPH_FAIL_BACKOFF;
    divoom_print("outlook graph transport fail, backoff %ds status=%d errno=%d\n", DIVOOM_OUTLOOK_CALENDAR_GRAPH_FAIL_BACKOFF, divoom_http_client_last_http_status(), divoom_http_client_last_errno());
}

static void divoom_outlook_calendar_config_poll_reset(void)
{
    if(pdivoom_outlook_calendar_cal_handle == NULL)
    {
        return;
    }

    pdivoom_outlook_calendar_cal_handle->config_poll_fail_stage = 0;
    pdivoom_outlook_calendar_cal_handle->config_poll_stopped = 0;
    pdivoom_outlook_calendar_cal_handle->config_poll_next_time = 0;
}

static int divoom_outlook_calendar_config_poll_due(time_t now)
{
    if(pdivoom_outlook_calendar_cal_handle == NULL)
    {
        return 0;
    }
    if(pdivoom_outlook_calendar_cal_handle->config_poll_stopped)
    {
        return 0;
    }
    if(pdivoom_outlook_calendar_cal_handle->config_poll_next_time == 0)
    {
        return 1;
    }

    return now >= pdivoom_outlook_calendar_cal_handle->config_poll_next_time;
}

static void divoom_outlook_calendar_config_poll_on_fail(time_t now)
{
    int interval_idx;

    if(pdivoom_outlook_calendar_cal_handle == NULL)
    {
        return;
    }

    pdivoom_outlook_calendar_cal_handle->config_poll_fail_stage++;
    if(pdivoom_outlook_calendar_cal_handle->config_poll_fail_stage >= DIVOOM_OUTLOOK_CONFIG_POLL_MAX_FAIL)
    {
        pdivoom_outlook_calendar_cal_handle->config_poll_stopped = 1;
        divoom_print("outlook config poll stopped after %d empty responses\n",
            DIVOOM_OUTLOOK_CONFIG_POLL_MAX_FAIL);
        return;
    }

    interval_idx = (int)pdivoom_outlook_calendar_cal_handle->config_poll_fail_stage - 1;
    if(interval_idx < 0)
    {
        interval_idx = 0;
    }
    if(interval_idx >= (int)(sizeof(divoom_outlook_config_poll_intervals) / sizeof(divoom_outlook_config_poll_intervals[0])))
    {
        interval_idx = (int)(sizeof(divoom_outlook_config_poll_intervals) / sizeof(divoom_outlook_config_poll_intervals[0])) - 1;
    }
    pdivoom_outlook_calendar_cal_handle->config_poll_next_time =
        now + divoom_outlook_config_poll_intervals[interval_idx];
    divoom_print("outlook config poll retry in %ds (stage %u)\n",
        divoom_outlook_config_poll_intervals[interval_idx],
        (unsigned)pdivoom_outlook_calendar_cal_handle->config_poll_fail_stage);
}

static void divoom_outlook_calendar_config_poll_on_success(void)
{
    divoom_outlook_calendar_config_poll_reset();
}

static int divoom_outlook_calendar_try_refresh_or_backoff(const char *used_token)
{
    if(divoom_http_client_last_is_auth_error())
        return divoom_outlook_calendar_refresh_token_locked(used_token);
    divoom_outlook_calendar_note_graph_transport_fail();
    return 0;
}

void divoom_outlook_calendar_request_default_color_info(void)
{
    char *cal_data = NULL, *used_token = NULL;
	divoom_cJSON *root = NULL, *list, *item, *data_obj;
    int hex_color, try_times = 0;
    Pdivoom_http_client_url_head_info phead_info;
    Pdivoom_outlook_calendar_default_color_item pcolor_item;

divoom_outlook_calendar_request_default_color_info_retry:
    phead_info = divoom_system_malloc(sizeof(divoom_http_client_url_head_info));
    if(phead_info == NULL)
    {
        divoom_system_free(used_token);
        return;
    }
    if(used_token)
    {
        divoom_system_free(used_token);
        used_token = NULL;
    }
    if(pdivoom_outlook_calendar_cal_handle->token)
        used_token = divoom_app_com_copy_string(pdivoom_outlook_calendar_cal_handle->token, -1);
    divoom_http_client_head_set_bearer_auth(phead_info, pdivoom_outlook_calendar_cal_handle->token);
    divoom_system_free_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);
    cal_data = divoom_http_client_url_request(DIVOOM_OUTLOOK_CALENDAR_DEFAULT_COLOR_URL, phead_info, 1, NULL, NULL);
    divoom_system_get_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);
    divoom_system_free(phead_info);
    if(cal_data == NULL)
    {
        if(try_times == 0 && used_token && pdivoom_outlook_calendar_cal_handle->token && strcmp(used_token, pdivoom_outlook_calendar_cal_handle->token))
        {
            try_times++;
            goto divoom_outlook_calendar_request_default_color_info_retry;
        }
        if(divoom_http_client_last_is_auth_error() == 0)
            divoom_outlook_calendar_note_graph_transport_fail();
        divoom_system_free(used_token);
        return;
    }
    divoom_system_free(used_token);
	root = divoom_cJSON_Parse(cal_data);
    divoom_system_free(cal_data);
	if (!root)
	{       
		return ;
	}
    //这个不用考虑错误处理器
    list = divoom_cJSON_GetObjectItem(root, "value");

    if(list == NULL || divoom_cJSON_GetArraySize(list) == 0)
    {   
        divoom_cJSON_Delete(root);  
        return ;
    }

    
    pdivoom_outlook_calendar_cal_handle->update_default_color_flag = 0;
    for(int i = 0; i < divoom_cJSON_GetArraySize(list); i++)
    {
        item = divoom_cJSON_GetArrayItem(list, i);
        if(item == NULL)
        {
            break;
        }
        data_obj  = divoom_cJSON_GetObjectItem(item, "hexColor");
        if(strlen(data_obj->valuestring) < 2)
        {
            hex_color = 0x00FF00;
        }
        else
        {
            sscanf(data_obj->valuestring, "#%x", &hex_color);
        }
        data_obj  = divoom_cJSON_GetObjectItem(item, "id");
        if(data_obj == NULL || data_obj->valuestring == NULL || strlen(data_obj->valuestring) == 0)
        {
            continue;
        }

        pcolor_item = divoom_queue_pop_cmp(pdivoom_outlook_calendar_cal_handle->default_color_queue, data_obj->valuestring, divoom_outlook_calendar_find_default_color_value_compare);
        if(pcolor_item)
        {
            if(pcolor_item->color_value  != hex_color)
            {
                pdivoom_outlook_calendar_cal_handle->update_default_color_flag = 1;
                pcolor_item->color_value  = hex_color;
            }
        }
        else
        {
            pcolor_item = divoom_system_malloc(sizeof(divoom_outlook_calendar_default_color_item));
            pcolor_item->color_value = hex_color;
            if(data_obj && data_obj->valuestring)
            {
                pcolor_item->id = divoom_app_com_copy_string(data_obj->valuestring, -1);
            }
        }
        divoom_queue_push(pdivoom_outlook_calendar_cal_handle->default_color_queue, pcolor_item);
    }
    divoom_cJSON_Delete(root);  
}

void divoom_outlook_calendar_request_color_info(void)
{
    char *cal_data = NULL, *used_token = NULL;
	divoom_cJSON *root = NULL, *list, *item, *data_obj;
    Pdivoom_http_client_url_head_info phead_info;
    Pdivoom_outlook_calendar_color_item pcolor_item;
    int try_times = 0;

    divoom_print("we will request outlook!\n");
divoom_outlook_calendar_request_color_info_retry:
    phead_info = divoom_system_malloc(sizeof(divoom_http_client_url_head_info));
    if(phead_info == NULL)
    {
        divoom_system_free(used_token);
        return;
    }
    if(used_token)
    {
        divoom_system_free(used_token);
        used_token = NULL;
    }
    if(pdivoom_outlook_calendar_cal_handle->token)
        used_token = divoom_app_com_copy_string(pdivoom_outlook_calendar_cal_handle->token, -1);
    divoom_http_client_head_set_bearer_auth(phead_info, pdivoom_outlook_calendar_cal_handle->token);
    divoom_system_free_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);
    cal_data = divoom_http_client_url_request(DIVOOM_OUTLOOK_CALENDAR_COLOR_URL, phead_info, 1, NULL, NULL);
    divoom_system_get_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);
    divoom_system_free(phead_info);
    if(cal_data == NULL)
    {
        if(try_times == 0 && divoom_outlook_calendar_try_refresh_or_backoff(used_token))
        {
            try_times++;
            goto divoom_outlook_calendar_request_color_info_retry;
        }
        divoom_system_free(used_token);
        return;
    }
    divoom_system_free(used_token);
	root = divoom_cJSON_Parse(cal_data);
    divoom_system_free(cal_data);
	if (!root)
	{       
		return ;
	}
    //这个不用考虑错误处理器
    list = divoom_cJSON_GetObjectItem(root, "value");

    if(list == NULL || divoom_cJSON_GetArraySize(list) == 0)
    {   
        divoom_cJSON_Delete(root);  
        return ;
    }

    divoom_queue_pop_all(pdivoom_outlook_calendar_cal_handle->color_queue, divoom_outlook_calendar_free_color_item);
    for(int i = 0; i < divoom_cJSON_GetArraySize(list); i++)
    {
        item = divoom_cJSON_GetArrayItem(list, i);
        if(item == NULL)
        {
            break;
        }
        pcolor_item = divoom_system_malloc(sizeof(divoom_outlook_calendar_color_item));
        if(pcolor_item == NULL)
        {
            break;
        }
        data_obj  = divoom_cJSON_GetObjectItem(item, "id");
        if(data_obj && data_obj->valuestring)
        {
            pcolor_item->id = divoom_app_com_copy_string(data_obj->valuestring, -1);
        }

        data_obj  = divoom_cJSON_GetObjectItem(item, "displayName");
        if(data_obj && data_obj->valuestring)
        {
            pcolor_item->color_name = divoom_app_com_copy_string(data_obj->valuestring, -1);
        }

        data_obj  = divoom_cJSON_GetObjectItem(item, "color");
        if(data_obj && data_obj->valuestring)
        {
            sscanf(data_obj->valuestring, "preset%d", &pcolor_item->color_index);
        }

        divoom_queue_push(pdivoom_outlook_calendar_cal_handle->color_queue, pcolor_item);
    }
    divoom_cJSON_Delete(root);  
}


int divoom_outlook_calendar_get_color_value_compare(void *item1, void *item2)
{
    char *color_name = (char *)item1;
    Pdivoom_outlook_calendar_color_item pcolor_item = (Pdivoom_outlook_calendar_color_item)item2;

    return strcmp(color_name, pcolor_item->color_name);
}

uint32 divoom_outlook_calendar_get_color_value(char *clock_name)
{
    Pdivoom_outlook_calendar_color_item pcolor_item;


    pcolor_item = divoom_queue_get_cmp(pdivoom_outlook_calendar_cal_handle->color_queue, clock_name, divoom_outlook_calendar_get_color_value_compare);
    if(pcolor_item)
    {
        return divoom_outlook_calendar_category_color_list[pcolor_item->color_index];
    }
    divoom_outlook_calendar_request_color_info();
    pcolor_item = divoom_queue_get_cmp(pdivoom_outlook_calendar_cal_handle->color_queue, clock_name, divoom_outlook_calendar_get_color_value_compare);
    if(pcolor_item)
    {
        return divoom_outlook_calendar_category_color_list[pcolor_item->color_index];
    }
    else
    {
        return 0x00FF00;
    }
}

int divoom_outlook_calendar_get_default_color_value_compare(void *item1, void *item2)
{
    char *color_name = (char *)item1;
    Pdivoom_outlook_calendar_default_color_item pcolor_item = (Pdivoom_outlook_calendar_default_color_item)item2;

    return strstr(color_name, pcolor_item->id)?0:1;
}

uint32 divoom_outlook_calendar_get_default_color_from_id(char *cal_id)
{
    Pdivoom_outlook_calendar_default_color_item pcolor_item;
    
    pcolor_item = divoom_queue_get_cmp(pdivoom_outlook_calendar_cal_handle->default_color_queue, cal_id, divoom_outlook_calendar_get_default_color_value_compare);
    if(pcolor_item)
    {
        return pcolor_item->color_value;
    }
    return 0x00FF00;
    
}

void divoom_outlook_calendar_request_url_info(void)
{
    int try_times = 0, url_len = 0, i;
    char *cal_data = NULL, *url_str, *encode_url, *active_event_id = NULL, *active_event_update = NULL, *used_token = NULL;
    time_t cur_time, start_time, end_time;
    struct tm start_date = {0}, end_date = {0}, update_date= {0};
	divoom_cJSON *root = NULL, *list, *item, *data_obj, *tmp1_obj, *tmp2_obj, *week_obj;
    Pdivoom_http_client_url_head_info phead_info;
    Pdivoom_outlook_calendar_event_item pevent_item;

    divoom_print("we will request outlook!\n");
    divoom_outlook_calendar_request_url_info_retry_lable:

    phead_info = divoom_system_malloc(sizeof(divoom_http_client_url_head_info));
    if(phead_info == NULL)
    {
        divoom_system_free(used_token);
        return;
    }
    if(used_token)
    {
        divoom_system_free(used_token);
        used_token = NULL;
    }
    if(pdivoom_outlook_calendar_cal_handle->token)
        used_token = divoom_app_com_copy_string(pdivoom_outlook_calendar_cal_handle->token, -1);
    cur_time = time(0);
    localtime_r(&cur_time, &start_date);
    start_time = cur_time - (start_date.tm_mday - 1) * 24*3600 - start_date.tm_hour*3600 - start_date.tm_min*60 - start_date.tm_sec ;
    localtime_r(&start_time, &start_date);
    start_time -= (start_date.tm_wday  * 24*3600);
    localtime_r(&start_time, &start_date);
    end_time = start_time + 42 *24 * 3600;
    localtime_r(&end_time, &end_date);
    
    divoom_http_client_head_set_bearer_auth(phead_info, pdivoom_outlook_calendar_cal_handle->token);
    url_str = divoom_system_malloc(2048);
    sprintf(url_str, "%s%d-%02d-%02d%s%d-%02d-%02d%s", DIVOOM_OUTLOOK_CALENDAR_EVENT_URL, start_date.tm_year + 1900 , start_date.tm_mon+1, start_date.tm_mday,DIVOOM_OUTLOOK_CALENDAR_EVENT_URL2, end_date.tm_year + 1900 , end_date.tm_mon+1, end_date.tm_mday, DIVOOM_OUTLOOK_CALENDAR_EVENT_URL3);
    
    divoom_system_free_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);
    cal_data = divoom_http_client_url_request(url_str, phead_info, 1, NULL, NULL);    
    divoom_system_get_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);
    
    divoom_system_free(url_str);
    divoom_system_free(phead_info);
    if(cal_data == NULL)
    {
        if(try_times == 0 && divoom_outlook_calendar_try_refresh_or_backoff(used_token))
        {
            try_times++;
            goto divoom_outlook_calendar_request_url_info_retry_lable;
        }
        divoom_system_free(used_token);
        return;
    }
	root = divoom_cJSON_Parse(cal_data);
    divoom_system_free(cal_data);
	if (!root)
	{
        divoom_system_free(used_token);
		return ;
	}
    item =  divoom_cJSON_GetObjectItem(root, "error");
    if(item)
    {
        int err_code = divoom_cJSON_GetObjectInt(item, "code");
        divoom_cJSON_Delete(root);
        if((err_code == 401 || err_code == 403 || divoom_http_client_last_is_auth_error()) && try_times == 0 && divoom_outlook_calendar_refresh_token_locked(used_token))
        {
            try_times++;
            goto divoom_outlook_calendar_request_url_info_retry_lable;
        }
        divoom_system_free(used_token);
        return;
    }
    divoom_system_free(used_token);
    used_token = NULL;
    list = divoom_cJSON_GetObjectItem(root, "value");

    if(list == NULL)
    {
        
        divoom_cJSON_Delete(root);  
        return ;
    }

    if(pdivoom_outlook_calendar_cal_handle->update_default_color_flag == 0 && divoom_outlook_calendar_check_event_update_time(list) == 0)
    {
        int need_reparse = 0;

        for(i = 0; i < pdivoom_outlook_calendar_cal_handle->event_num; i++)
        {
            if(pdivoom_outlook_calendar_cal_handle->pevent_list[i].start_utc == 0
                || pdivoom_outlook_calendar_cal_handle->pevent_list[i].end_utc == 0)
            {
                need_reparse = 1;
                break;
            }
        }
        if(!need_reparse)
        {
            divoom_print("pass update!\n");
            divoom_cJSON_Delete(root);
            return ;
        }
    }
    pdivoom_outlook_calendar_cal_handle->update_default_color_flag = 0;
    if(pdivoom_outlook_calendar_cal_handle->prunning_event)
    {
        active_event_id = divoom_app_com_copy_string(pdivoom_outlook_calendar_cal_handle->prunning_event->event_id, -1);
        active_event_update = divoom_app_com_copy_string(pdivoom_outlook_calendar_cal_handle->prunning_event->update_time, -1);
    }
    if(pdivoom_outlook_calendar_cal_handle->pevent_list)
    {
        divoom_outlook_calendar_free_all_event(pdivoom_outlook_calendar_cal_handle->pevent_list, pdivoom_outlook_calendar_cal_handle->event_num);
        pdivoom_outlook_calendar_cal_handle->pevent_list = NULL;
        pdivoom_outlook_calendar_cal_handle->prunning_event = NULL;
    }
    pdivoom_outlook_calendar_cal_handle->event_num = divoom_cJSON_GetArraySize(list);
    pdivoom_outlook_calendar_cal_handle->pevent_list = divoom_system_malloc(sizeof(divoom_outlook_calendar_event_item)*pdivoom_outlook_calendar_cal_handle->event_num);
    for(int i = 0; i < pdivoom_outlook_calendar_cal_handle->event_num; i++)
    {
        item = divoom_cJSON_GetArrayItem(list, i);
        if(item == NULL)
        {
            break;
        }
        
        pevent_item = &pdivoom_outlook_calendar_cal_handle->pevent_list[i];
        memset(pevent_item, 0, sizeof(divoom_outlook_calendar_event_item));
        pevent_item->notice_flag = divoom_cJSON_GetObjectInt(item, "isReminderOn");
        pevent_item->all_day_flag = divoom_cJSON_GetObjectInt(item, "isAllDay");
        pevent_item->reminder_time = divoom_cJSON_GetObjectInt(item, "reminderMinutesBeforeStart") * 60;
        data_obj  = divoom_cJSON_GetObjectItem(item, "categories");
        pevent_item->bg_color = 0x00FF00;
        if(data_obj && divoom_cJSON_GetArraySize(data_obj))
        {
            data_obj = divoom_cJSON_GetArrayItem(data_obj, 0);
            if(data_obj && data_obj->valuestring)
            {
                pevent_item->bg_color = divoom_outlook_calendar_get_color_value(data_obj->valuestring);
            }
        }
        else
        {
            data_obj  = divoom_cJSON_GetObjectItem(item, "calendar@odata.associationLink");
            if(data_obj && data_obj->valuestring)
            {
                pevent_item->bg_color = divoom_outlook_calendar_get_default_color_from_id(data_obj->valuestring);            
            }
        }
        data_obj  = divoom_cJSON_GetObjectItem(item, "subject");
        if(data_obj && data_obj->valuestring)
        {
            pevent_item->title = divoom_app_com_copy_string(data_obj->valuestring, -1);
        }
        
        data_obj  = divoom_cJSON_GetObjectItem(item, "id");
        if(data_obj && data_obj->valuestring)
        {
            pevent_item->event_id = divoom_app_com_copy_string(data_obj->valuestring, -1);
        }
        
        data_obj  = divoom_cJSON_GetObjectItem(item, "lastModifiedDateTime");
        if(data_obj && data_obj->valuestring)
        {
            pevent_item->update_time = divoom_app_com_copy_string(data_obj->valuestring, -1);
        }

        if(active_event_id)
        {
            if(strcmp(active_event_id, pevent_item->event_id) == 0)
            {
                if(strcmp(active_event_update, pevent_item->update_time))
                {
                    //激活的发生变化
                    divoom_outlook_calendar_send_event_notice_message(NULL, DIVOOM_OUTLOOK_CALENDAR_STATUS_IDLE);
                }
                else
                {
                    pdivoom_outlook_calendar_cal_handle->prunning_event = pevent_item;
                }
            }
        }
        
        data_obj  = divoom_cJSON_GetObjectItem(item, "bodyPreview");
        if(data_obj && data_obj->valuestring)
        {
            pevent_item->desc = divoom_app_com_copy_string(data_obj->valuestring, -1);
        }
        
        data_obj = divoom_cJSON_GetObjectItem(item, "start");
        if(data_obj)
        {
            tmp1_obj = divoom_cJSON_GetObjectItem(data_obj, "date");
            if(tmp1_obj && tmp1_obj->valuestring)
            {
                pevent_item->start_utc = divoom_outlook_calendar_get_utc_time(tmp1_obj->valuestring);
            }
            else
            {
                tmp1_obj = divoom_cJSON_GetObjectItem(data_obj, "dateTime");
                if(tmp1_obj && tmp1_obj->valuestring)
                    pevent_item->start_utc = divoom_outlook_calendar_get_utc_time(tmp1_obj->valuestring);
            }
        }
        
        data_obj = divoom_cJSON_GetObjectItem(item, "end");
        if(data_obj)
        {
            tmp1_obj = divoom_cJSON_GetObjectItem(data_obj, "date");
            if(tmp1_obj && tmp1_obj->valuestring)
            {
                pevent_item->end_utc = divoom_outlook_calendar_get_utc_time(tmp1_obj->valuestring);
                if(pevent_item->end_utc > 0)
                    pevent_item->end_utc += 86400;
            }
            else
            {
                tmp1_obj = divoom_cJSON_GetObjectItem(data_obj, "dateTime");
                if(tmp1_obj && tmp1_obj->valuestring)
                    pevent_item->end_utc = divoom_outlook_calendar_get_utc_time(tmp1_obj->valuestring);
            }
        }
        
        data_obj = divoom_cJSON_GetObjectItem(item, "recurrence");
        if(data_obj)
        {
            
            tmp1_obj = divoom_cJSON_GetObjectItem(data_obj, "pattern");
            if(tmp1_obj)
            {
                
                tmp2_obj = divoom_cJSON_GetObjectItem(tmp1_obj, "type");
                if(tmp2_obj && tmp2_obj->valuestring)
                {
                    if(strcmp(tmp2_obj->valuestring, "absoluteYearly") == 0)
                    {
                        pevent_item->repeat_type = DIVOOM_OUTLOOK_CALENDAR_REPEAT_YEAR;
                    }
                    else if(strcmp(tmp2_obj->valuestring, "absoluteMonthly") == 0)
                    {
                        pevent_item->repeat_type = DIVOOM_OUTLOOK_CALENDAR_REPEAT_MON;
                    }
                    else if(strcmp(tmp2_obj->valuestring, "weekly") == 0)
                    {
                        pevent_item->repeat_type = DIVOOM_OUTLOOK_CALENDAR_REPEAT_WEEK;
                        tmp2_obj = divoom_cJSON_GetObjectItem(tmp1_obj, "daysOfWeek");
                        /*
                        "monday",
                        "",
                        "",
                        "",
                        "",
                        "",
                        ""
                        */
                        for(int j = 0; tmp2_obj && j < divoom_cJSON_GetArraySize(tmp2_obj); j++)
                        {
                            week_obj = divoom_cJSON_GetArrayItem(tmp2_obj, j);
                            if(week_obj && week_obj->valuestring)
                            {
                                if(strcmp(week_obj->valuestring, "sunday") == 0)
                                {
                                    pevent_item->week_repeat[0] = 1;
                                }
                                else if(strcmp(week_obj->valuestring, "monday") == 0)
                                {
                                    pevent_item->week_repeat[1] = 1;
                                }
                                else if(strcmp(week_obj->valuestring, "tuesday") == 0)
                                {
                                    pevent_item->week_repeat[2] = 1;
                                }
                                else if(strcmp(week_obj->valuestring, "wednesday") == 0)
                                {
                                    pevent_item->week_repeat[3] = 1;
                                }
                                else if(strcmp(week_obj->valuestring, "thursday") == 0)
                                {
                                    pevent_item->week_repeat[4] = 1;
                                }
                                else if(strcmp(week_obj->valuestring, "friday") == 0)
                                {
                                    pevent_item->week_repeat[5] = 1;
                                }
                                else if(strcmp(week_obj->valuestring, "saturday") == 0)
                                {
                                    pevent_item->week_repeat[6] = 1;
                                }
                            }
                        }
                    }
                    else if(strcmp(tmp2_obj->valuestring, "daily") == 0)
                    {
                        pevent_item->repeat_type = DIVOOM_OUTLOOK_CALENDAR_REPEAT_DAY;
                    }
                    else
                    {
                        pevent_item->repeat_type = DIVOOM_OUTLOOK_CALENDAR_REPEAT_NONE;
                    }
                }
            }

            
            
            tmp1_obj = divoom_cJSON_GetObjectItem(data_obj, "range");
            if(tmp1_obj)
            {
                pevent_item->range_end_utc = 0;
                tmp2_obj = divoom_cJSON_GetObjectItem(tmp1_obj, "startDate");
                
                if(tmp2_obj && tmp2_obj->valuestring)
                {
                    update_date.tm_hour = 0;
                    update_date.tm_min = 0;
                    update_date.tm_sec = 0;
                    sscanf(tmp2_obj->valuestring, "%d-%d-%d", &update_date.tm_year,&update_date.tm_mon ,
                        &update_date.tm_mday);
                    update_date.tm_year -= 1900;
                    update_date.tm_mon --;
                    pevent_item->range_start_utc = mktime(&update_date);
                }
                
                tmp2_obj = divoom_cJSON_GetObjectItem(tmp1_obj, "type");
                if(tmp2_obj && tmp2_obj->valuestring)
                {
                    
                    if(strcmp(tmp2_obj->valuestring, "endDate") == 0)
                    {
                        tmp2_obj = divoom_cJSON_GetObjectItem(tmp1_obj, "endDate");
                        update_date.tm_hour = 23;
                        update_date.tm_min = 59;
                        update_date.tm_sec = 59;
                        sscanf(tmp2_obj->valuestring, "%d-%d-%d", &update_date.tm_year,&update_date.tm_mon ,
                            &update_date.tm_mday);
                        update_date.tm_year -= 1900;
                        update_date.tm_mon --;
                        pevent_item->range_end_utc = mktime(&update_date);
                    }
                }
            }
            
        }
        
        divoom_outlook_calendar_formart_event_item(pevent_item);
        
    }
    divoom_cJSON_Delete(root);  
    if(active_event_id)
    {
        divoom_system_free(active_event_id);
    }
    if(active_event_update)
    {
        divoom_system_free(active_event_update);
    }
    pdivoom_outlook_calendar_cal_handle->update_flag = 1;
    divoom_print("outlook event sync: num=%d\n", pdivoom_outlook_calendar_cal_handle->event_num);
    divoom_outlook_calendar_request_watch_refresh_force();
    
}

/*********************************************************************
Function:         divoom_google_calendar_send_event_notice
Description:      向系统发送提醒事件
Input:            none
Return:           
Author:           albert
**********************************************************************/

void divoom_outlook_calendar_send_event_notice(void *data)
{
	divoom_sys_msg_info model_msg = {0};

    pdivoom_outlook_calendar_cal_handle->prunning_event = data;

    if(data)
    {
        data = divoom_system_malloc(sizeof(int));
    }
	model_msg.src_id = DIVOOM_MODEL_NET;
    model_msg.msg_type = 1;
	model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
	model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_OUTLOOK_CALENDAR_CTRL;
	model_msg.data = data;
	divoom_sys_model_crl_msg_push(&model_msg);
}


/*********************************************************************
Function:         divoom_outlook_calendar_user_cancle_event
Description:      按键取消
Input:            
Return:           
Author:           albert
**********************************************************************/

void divoom_outlook_calendar_user_cancle_event(void)
{
    if (pdivoom_outlook_calendar_cal_handle == NULL)
    {
        return;
    }
    divoom_print("aj test!\n");
    divoom_system_get_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);
    if(pdivoom_outlook_calendar_cal_handle->prunning_event)
    {
        divoom_print("aj test!\n");
        pdivoom_outlook_calendar_cal_handle->prunning_event->status = DIVOOM_OUTLOOK_CALENDAR_STATUS_PASS;
        pdivoom_outlook_calendar_cal_handle->prunning_event = NULL;
    	divoom_sys_msg_info model_msg = {0};

    	model_msg.src_id = DIVOOM_MODEL_NET;
        model_msg.msg_type = 1;
    	model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
    	model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_OUTLOOK_CALENDAR_CTRL;
    	model_msg.data = NULL;
    	divoom_sys_model_crl_msg_push(&model_msg);
    }
    divoom_system_free_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);
}

/*********************************************************************
Function:         divoom_outlook_calendar_get_active_lvgl_event_info
Description:      得到LVGL显示的信息
Input:            data
Return:           
Author:           albert
**********************************************************************/

int divoom_outlook_calendar_get_active_lvgl_event_info(void *data)
{
    int ret = 1;
    Pdivoom_lvgl_calendar_event_info pevent_info = (Pdivoom_lvgl_calendar_event_info)data;
    time_t cur_time; 
    struct tm cur_tm;
    
    
    if(divoom_system_try_lock(pdivoom_outlook_calendar_cal_handle->lock_handle))
    {
        return 0;
    }
    if(pdivoom_outlook_calendar_cal_handle->prunning_event)
    {
        pevent_info->bg_color[0] = pdivoom_outlook_calendar_cal_handle->prunning_event->bg_color >> 16;
        pevent_info->bg_color[1] = (pdivoom_outlook_calendar_cal_handle->prunning_event->bg_color >> 8) &0xFF;
        pevent_info->bg_color[2] = pdivoom_outlook_calendar_cal_handle->prunning_event->bg_color & 0xFF;
        memset(pevent_info->fr_color, 0, 3);
        cur_time = pdivoom_outlook_calendar_cal_handle->prunning_event->start_utc;
        localtime_r(&cur_time, &cur_tm);
        pevent_info->start_time =  cur_tm.tm_hour *60 + cur_tm.tm_min;
        cur_time = pdivoom_outlook_calendar_cal_handle->prunning_event->end_utc;
        localtime_r(&cur_time, &cur_tm);
        pevent_info->end_time =  cur_tm.tm_hour *60 + cur_tm.tm_min;
        if(pevent_info->end_time == 0)
        {
            pevent_info->end_time = 23*60 +59;
        }
        if(pdivoom_outlook_calendar_cal_handle->prunning_event->title == NULL)
        {
            pevent_info->title = divoom_app_com_copy_string("  ", -1);
        }
        else
        {
            pevent_info->title = divoom_app_com_copy_string( pdivoom_outlook_calendar_cal_handle->prunning_event->title, -1);
        }
        pevent_info->msg_info = divoom_app_com_copy_string(pdivoom_outlook_calendar_cal_handle->prunning_event->desc, -1);
    }
    else
    {
        ret = 0;
    }
    divoom_system_free_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);
    return ret;

}

int divoom_outlook_calendar_add_popup_notice_message_callback(void *para, int result)
{
    Pdivoom_outlook_calendar_event_item pevent_item = (Pdivoom_outlook_calendar_event_item)para;
    for(int i= 0; i < pdivoom_outlook_calendar_cal_handle->event_num; i++)
    {
        if(pevent_item == &pdivoom_outlook_calendar_cal_handle->pevent_list[i])
        {
            pevent_item->lvgl_obj = NULL;
            return 0;
        }
    }
    return 0;
}

/*********************************************************************
Function:         divoom_google_calendar_send_event_notice_message
Description:      通知系统事件触发
Input:            data
Return:           
Author:           albert
**********************************************************************/

static int divoom_outlook_calendar_popup_font_color(uint32 bg_color)
{
    int r = (bg_color >> 16) & 0xFF;
    int g = (bg_color >> 8) & 0xFF;
    int b = bg_color & 0xFF;
    int luminance = (r * 299 + g * 587 + b * 114) / 1000;

    return luminance > 160 ? 0x2C2A27 : 0xFFFFFF;
}

static uint32 divoom_outlook_calendar_popup_bg_color(uint32 bg_color)
{
    uint32 rgb = bg_color & 0xFFFFFF;

    if(rgb == 0)
    {
        return 0x0078D4;
    }
    return rgb;
}

static void divoom_outlook_calendar_popup_fit_icon(void *image_obj)
{
    int scale_w, scale_h, scale;
    lv_obj_t *img = (lv_obj_t *)image_obj;

    if(img == NULL)
    {
        return;
    }

    scale_w = DIVOOM_OUTLOOK_CALENDAR_POPUP_ICON_SZ * LV_IMG_ZOOM_NONE / DIVOOM_OUTLOOK_CALENDAR_LOG_SRC_W;
    scale_h = DIVOOM_OUTLOOK_CALENDAR_POPUP_ICON_SZ * LV_IMG_ZOOM_NONE / DIVOOM_OUTLOOK_CALENDAR_LOG_SRC_H;
    scale = scale_w < scale_h ? scale_w : scale_h;
    if(scale < 1)
    {
        scale = 1;
    }

    lv_img_set_size_mode(img, LV_IMG_SIZE_MODE_REAL);
    lv_img_set_zoom(img, (uint16_t)scale);
    lv_img_set_pivot(img, DIVOOM_OUTLOOK_CALENDAR_LOG_SRC_W / 2, DIVOOM_OUTLOOK_CALENDAR_LOG_SRC_H / 2);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
}

void divoom_outlook_calendar_add_popup_notice_message(Pdivoom_outlook_calendar_event_item pevent_item)
{
    int bg_value, font_value, text_x, text_w, text_y0, icon_y;
    struct tm cur_tm, end_tm;
    time_t cur_time = pevent_item->next_start_utc ? pevent_item->next_start_utc : pevent_item->start_utc;
    time_t end_time = pevent_item->next_end_utc ? pevent_item->next_end_utc : pevent_item->end_utc;
    void *lable_obj = NULL;
    void *lvgl_obj = NULL;
    void *icon_cont = NULL;
    void *image_obj = NULL;
    int text_block_h;

    if(pevent_item == NULL || pevent_item->lvgl_obj)
    {
        return;
    }

    localtime_r(&cur_time, &cur_tm);
    localtime_r(&end_time, &end_tm);

    bg_value = divoom_outlook_calendar_popup_bg_color(pevent_item->bg_color);
    font_value = DIVOOM_OUTLOOK_CALENDAR_POPUP_TEXT_COLOR;
    lvgl_obj = divoom_system_msg_notice_add(DIVOOM_OUTLOOK_CALENDAR_POPUP_W, DIVOOM_OUTLOOK_CALENDAR_POPUP_H,
            divoom_outlook_calendar_add_popup_notice_message_callback, pevent_item);
    if(lvgl_obj == NULL)
    {
        return;
    }
    pevent_item->lvgl_obj = lvgl_obj;

    text_x = DIVOOM_OUTLOOK_CALENDAR_POPUP_ICON_X + DIVOOM_OUTLOOK_CALENDAR_POPUP_ICON_SZ
           + DIVOOM_OUTLOOK_CALENDAR_POPUP_TEXT_GAP;
    text_w = DIVOOM_OUTLOOK_CALENDAR_POPUP_W - text_x - DIVOOM_OUTLOOK_CALENDAR_POPUP_PAD;
    text_block_h = DIVOOM_OUTLOOK_CALENDAR_POPUP_TIME_H + DIVOOM_OUTLOOK_CALENDAR_POPUP_TEXT_ROW_GAP
                   + DIVOOM_OUTLOOK_CALENDAR_POPUP_TITLE_H;
    text_y0 = (DIVOOM_OUTLOOK_CALENDAR_POPUP_H - text_block_h) / 2;

    divoom_get_lvgl_lock();
    lv_obj_set_style_bg_color(lvgl_obj, lv_color_hex(bg_value), 0);
    lv_obj_set_style_bg_opa(lvgl_obj, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(lvgl_obj, DIVOOM_OUTLOOK_CALENDAR_POPUP_RADIUS, 0);
    lv_obj_set_style_radius(lvgl_obj, DIVOOM_OUTLOOK_CALENDAR_POPUP_RADIUS, LV_STATE_FOCUSED);
    lv_obj_set_style_radius(lvgl_obj, DIVOOM_OUTLOOK_CALENDAR_POPUP_RADIUS, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_pad_all(lvgl_obj, 0, 0);
    lv_obj_set_style_border_width(lvgl_obj, 0, 0);
    lv_obj_set_style_border_width(lvgl_obj, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(lvgl_obj, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(lvgl_obj, 0, 0);
    lv_obj_clear_flag(lvgl_obj, LV_OBJ_FLAG_SCROLLABLE);

    icon_y = (DIVOOM_OUTLOOK_CALENDAR_POPUP_H - DIVOOM_OUTLOOK_CALENDAR_POPUP_ICON_SZ) / 2;
    icon_cont = lv_obj_create(lvgl_obj);
    lv_obj_set_pos(icon_cont, DIVOOM_OUTLOOK_CALENDAR_POPUP_ICON_X, icon_y);
    lv_obj_set_size(icon_cont, DIVOOM_OUTLOOK_CALENDAR_POPUP_ICON_SZ, DIVOOM_OUTLOOK_CALENDAR_POPUP_ICON_SZ);
    lv_obj_set_style_bg_opa(icon_cont, LV_OPA_0, 0);
    lv_obj_set_style_border_width(icon_cont, 0, 0);
    lv_obj_set_style_pad_all(icon_cont, 0, 0);
    lv_obj_clear_flag(icon_cont, LV_OBJ_FLAG_SCROLLABLE);

    image_obj = lv_img_create(icon_cont);
    divoom_menu_control_lv_img_set_src(image_obj, "82/outlook_calendar_log.png");
    divoom_outlook_calendar_popup_fit_icon(image_obj);

    lable_obj = lv_label_create(lvgl_obj);
    lv_obj_set_pos(lable_obj, text_x, text_y0);
    lv_obj_set_size(lable_obj, text_w, DIVOOM_OUTLOOK_CALENDAR_POPUP_TIME_H);
    lv_obj_set_style_text_color(lable_obj, lv_color_hex(font_value), 0);
    lv_obj_set_style_bg_color(lable_obj, lv_color_hex(bg_value), 0);
    lv_obj_set_style_bg_opa(lable_obj, LV_OPA_0, 0);
    lv_obj_set_style_text_font(lable_obj, divoom_menu_control_get_font_info(20, 0), 0);
    lv_obj_set_style_text_align(lable_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_pad_all(lable_obj, 0, 0);
    if(pevent_item->all_day_flag)
    {
        lv_label_set_text(lable_obj, "All day");
    }
    else
    {
        lv_label_set_text_fmt(lable_obj, "%d:%02d-%d:%02d",
            cur_tm.tm_hour, cur_tm.tm_min, end_tm.tm_hour, end_tm.tm_min);
    }

    lable_obj = lv_label_create(lvgl_obj);
    lv_obj_set_size(lable_obj, text_w, DIVOOM_OUTLOOK_CALENDAR_POPUP_TITLE_H);
    lv_obj_set_pos(lable_obj, text_x, text_y0 + DIVOOM_OUTLOOK_CALENDAR_POPUP_TIME_H
        + DIVOOM_OUTLOOK_CALENDAR_POPUP_TEXT_ROW_GAP);
    lv_obj_set_style_text_color(lable_obj, lv_color_hex(font_value), 0);
    lv_obj_set_style_bg_color(lable_obj, lv_color_hex(bg_value), 0);
    lv_obj_set_style_bg_opa(lable_obj, LV_OPA_0, 0);
    lv_obj_set_style_text_font(lable_obj, divoom_menu_control_get_font_info(24, 0), 0);
    lv_obj_set_style_text_align(lable_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_pad_all(lable_obj, 0, 0);
    lv_label_set_text(lable_obj, pevent_item->title ? pevent_item->title : " ");
    lv_label_set_long_mode(lable_obj, LV_LABEL_LONG_DOT);
    divoom_free_lvgl_lock();
}


/*********************************************************************
Function:         divoom_outlook_calendar_send_event_notice_message
Description:      通知系统事件触发
Input:            data
Return:           
Author:           albert
**********************************************************************/

void divoom_outlook_calendar_send_event_notice_message(void *data, int status)
{    
    Pdivoom_outlook_calendar_event_item pevent_item = (Pdivoom_outlook_calendar_event_item)data;
    

    if(pevent_item)
    {
        divoom_print("aj test: %s: %d\n", pevent_item->title, status);
    }
    if(status == DIVOOM_OUTLOOK_CALENDAR_STATUS_NOTICE)
    {
        //这里增加一个系统消息
        if(pdivoom_outlook_calendar_cal_handle->pop_flag)
        {
            divoom_outlook_calendar_add_popup_notice_message(data);
        }
    }
    else if(status == DIVOOM_OUTLOOK_CALENDAR_STATUS_ACTIVE)
    {
        if(pdivoom_outlook_calendar_cal_handle->event_flag)
        {
            divoom_outlook_calendar_send_event_notice(data);
            if(pevent_item->lvgl_obj)
            {
                divoom_system_msg_notice_clear_one(pevent_item->lvgl_obj, 1);
                pevent_item->lvgl_obj = NULL;
            }
        }
    }
    else if(status == DIVOOM_OUTLOOK_CALENDAR_STATUS_IDLE)
    {
        if(pdivoom_outlook_calendar_cal_handle->event_flag)
        {
            if(pevent_item == NULL
                || pdivoom_outlook_calendar_cal_handle->prunning_event == pevent_item)
            {
                divoom_outlook_calendar_send_event_notice(NULL);
            }
            else if(pevent_item)
            {
                divoom_print("outlook idle skip: %s (prunning=%s)\n",
                    pevent_item->title ? pevent_item->title : "?",
                    pdivoom_outlook_calendar_cal_handle->prunning_event
                        && pdivoom_outlook_calendar_cal_handle->prunning_event->title
                        ? pdivoom_outlook_calendar_cal_handle->prunning_event->title : "?");
            }
        }
    }
    
}


void divoom_outlook_calendar_add_lvgl_event(void **queue_item, int pos, Pdivoom_lvgl_calendar_event_info plvgl_event_item)
{
    if(queue_item[pos] == NULL)
    {
        queue_item[pos] = divoom_queue_init(100, __func__, __LINE__);
    }
    divoom_queue_push(queue_item[pos], plvgl_event_item);
    //divoom_print("add : %d, %d, %s\n", pos, divoom_queue_items(queue_item[pos]), plvgl_event_item->title);
}

Pdivoom_lvgl_calendar_event_info divoom_outlook_calendar_create_lvgl_event(time_t start_utc, time_t end_utc, char *title, char *desc, uint32 bg_color)
{
    struct tm cur_date = {0};
    Pdivoom_lvgl_calendar_event_info plvgl_event_item;
    
    plvgl_event_item = divoom_system_malloc(sizeof(divoom_lvgl_calendar_event_info));
    plvgl_event_item->bg_color[0] = bg_color >> 16;
    plvgl_event_item->bg_color[1] = (bg_color >> 8) &0xFF;
    plvgl_event_item->bg_color[2] = bg_color & 0xFF;
    memset(plvgl_event_item->fr_color, 00, 3);  
    localtime_r(&start_utc, &cur_date);              
    plvgl_event_item->start_time = cur_date.tm_hour * 60 + cur_date.tm_min;
    localtime_r(&end_utc, &cur_date);          
    plvgl_event_item->end_time = cur_date.tm_hour * 60 + cur_date.tm_min;
    if(title == NULL)
    {
        plvgl_event_item->title = divoom_app_com_copy_string("  ", -1);
    }
    else
    {
        plvgl_event_item->title = divoom_app_com_copy_string(title, -1);
    }
    plvgl_event_item->msg_info = divoom_app_com_copy_string(desc, -1);
    return plvgl_event_item;
}
/**
 * 判断是否为闰年
 * @param year 年份
 * @return 是闰年返回true，否则返回false
 */
bool divoom_outlook_calendar_is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

/**
 * 计算指定年月的天数
 * @param year 年份
 * @param month 月份 (1-12)
 * @return 该月的天数，如果月份无效返回-1
 */
int divoom_outlook_calendar_get_days_in_month(int year, int month) {
    // 检查月份是否有效
    if (month < 1 || month > 12) {
        return -1;
    }
    
    // 每月天数数组（非闰年）
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    // 如果是2月，需要判断是否为闰年
    if (month == 2) {
        return divoom_outlook_calendar_is_leap_year(year) ? 29 : 28;
    }
    
    return days_in_month[month - 1];
}

int divoom_outlook_calendar_check_date_ok(struct tm *cur_date)
{
    return cur_date->tm_mday <= divoom_outlook_calendar_get_days_in_month(cur_date->tm_year + 1900 , cur_date->tm_mon+1);
}

void divoom_outlook_calendar_load_calenday_all_from_event(void **queue_item, Pdivoom_outlook_calendar_event_item pevent_item, time_t start_utc, time_t end_utc)
{
    int active_time, day_index;
    time_t cur_time;
    struct tm cur_date = {0}, start_date ={0}, tmp_date = {0};
    Pdivoom_lvgl_calendar_event_info plvgl_event_item;
    
//    divoom_print("aj test!: %s, %d, %d, %d, %d, %d, %d, %d\n", pevent_item->title, pevent_item->repeat_type, start_utc, end_utc, pevent_item->start_utc, pevent_item->end_utc, pevent_item->range_start_utc, pevent_item->range_end_utc);
    if(pevent_item->start_utc > end_utc)
    {
        //这个事件不在这个月
        return;
    }
    
    if( pevent_item->range_end_utc && start_utc >= pevent_item->range_end_utc)
    {
        //这个事件在当前显示页面已经过期，不显示
        return;
    }
    
    if(pevent_item->range_start_utc > end_utc)
    {
        //这个事件起始时间超过在当前显示页面结束时间
        return;
    }
    
    switch(pevent_item->repeat_type)
    {
        case DIVOOM_OUTLOOK_CALENDAR_REPEAT_NONE:
 //           divoom_print("aj test!: all_day_flag:%d\n", pevent_item->all_day_flag);
            if(pevent_item->all_day_flag)
            {
                active_time = pevent_item->start_utc;
                while(active_time >= start_utc && active_time < end_utc)
                {
                    
                    plvgl_event_item = divoom_outlook_calendar_create_lvgl_event(active_time, active_time + 24*3600, pevent_item->title, pevent_item->desc, pevent_item->bg_color);
                    divoom_outlook_calendar_add_lvgl_event(queue_item, (active_time - start_utc)/(24*3600), plvgl_event_item);
                    active_time += 24*3600;
                }
            }
            else
            {
                if(pevent_item->start_utc >= start_utc && pevent_item->start_utc < end_utc)
                {
                    
                    plvgl_event_item = divoom_outlook_calendar_create_lvgl_event(pevent_item->start_utc, pevent_item->end_utc, pevent_item->title, pevent_item->desc, pevent_item->bg_color);
                    divoom_outlook_calendar_add_lvgl_event(queue_item, (pevent_item->start_utc - start_utc)/(24*3600), plvgl_event_item);
                }
            }
                
            break;
            
        case DIVOOM_OUTLOOK_CALENDAR_REPEAT_DAY:
            //取今天或明天的时间
            if(pevent_item->start_utc < start_utc)
            {
                day_index = 0;
                localtime_r(&pevent_item->start_utc, &start_date);
                active_time = start_utc + start_date.tm_hour * 3600 + start_date.tm_min * 60 + start_date.tm_sec;
            }
            else
            {
                day_index = (pevent_item->start_utc - start_utc)/(24*3600);
                active_time = pevent_item->start_utc;
            }
         //   divoom_print("start: %d\n", day_index);
            for(; day_index < 42 && active_time < end_utc; day_index++)
            {
                plvgl_event_item = divoom_outlook_calendar_create_lvgl_event(active_time, active_time + (pevent_item->end_utc - pevent_item->start_utc), pevent_item->title, pevent_item->desc, pevent_item->bg_color);            
                divoom_outlook_calendar_add_lvgl_event(queue_item, day_index, plvgl_event_item);                
                active_time += 24 *3600;
            }
            break;
            
        case DIVOOM_OUTLOOK_CALENDAR_REPEAT_WEEK:  
            if(pevent_item->start_utc < start_utc)
            {
                localtime_r(&pevent_item->start_utc, &start_date);
                active_time = start_utc + start_date.tm_hour * 3600 + start_date.tm_min * 60 + start_date.tm_sec;
                for(int i = 0; i < 6; i++)
                {
                    for(int j = 0; j < 7 && active_time < end_utc; j++)
                    {
                        if(pevent_item->week_repeat[j])
                        {
                            plvgl_event_item = divoom_outlook_calendar_create_lvgl_event(active_time, active_time + (pevent_item->end_utc - pevent_item->start_utc), pevent_item->title, pevent_item->desc, pevent_item->bg_color);                                    
                            divoom_outlook_calendar_add_lvgl_event(queue_item, i*7+j, plvgl_event_item);
                        }
                        active_time += 24*3600;
                    }
                }
            }
            else
            {
                day_index = (pevent_item->start_utc - start_utc)/(24*3600);
                active_time = pevent_item->start_utc;
                for(; day_index < 42 && active_time < end_utc; day_index++)
                {
                    if(pevent_item->week_repeat[day_index%7])
                    {
                        plvgl_event_item = divoom_outlook_calendar_create_lvgl_event(active_time, active_time + (pevent_item->end_utc - pevent_item->start_utc), pevent_item->title, pevent_item->desc, pevent_item->bg_color);                                    
                        divoom_outlook_calendar_add_lvgl_event(queue_item, day_index, plvgl_event_item);
                    }
                    active_time += 24*3600;
                }
            }        
            break;
            
        case DIVOOM_OUTLOOK_CALENDAR_REPEAT_MON: 
            localtime_r(&start_utc, &cur_date);
            localtime_r(&pevent_item->start_utc, &start_date);
            start_date.tm_year = cur_date.tm_year;
            start_date.tm_mon = cur_date.tm_mon;
            for(int i = 0; i < 3; i++)
            {
                if(divoom_outlook_calendar_check_date_ok(&start_date))
                {
                    active_time = mktime(&start_date);
                    if(active_time >= pevent_item->start_utc && active_time >= start_utc && active_time < end_utc)
                    {
                        plvgl_event_item = divoom_outlook_calendar_create_lvgl_event(active_time, active_time + (pevent_item->end_utc - pevent_item->start_utc), pevent_item->title, pevent_item->desc, pevent_item->bg_color);                                    
                        divoom_outlook_calendar_add_lvgl_event(queue_item, (active_time - start_utc)/(24*3600), plvgl_event_item);                    
                    }
                }
                if(start_date.tm_mon < 12)
                {
                    start_date.tm_mon ++;
                }
                else
                {
                    start_date.tm_year ++;
                    start_date.tm_mon = 1;
                }
                
            }
            
            break;
            
        case DIVOOM_OUTLOOK_CALENDAR_REPEAT_YEAR:
            localtime_r(&start_utc, &cur_date);
            localtime_r(&end_utc, &start_date);
            localtime_r(&pevent_item->start_utc, &tmp_date);
            tmp_date.tm_year = cur_date.tm_year - 1;
            for(int i = 0; i < 3; i++)
            {
                if(divoom_outlook_calendar_check_date_ok(&start_date))
                {
                    active_time = mktime(&tmp_date);
                    if(active_time >= start_utc && active_time < end_utc)
                    {
                        plvgl_event_item = divoom_outlook_calendar_create_lvgl_event(active_time, active_time + (pevent_item->end_utc - pevent_item->start_utc), pevent_item->title, pevent_item->desc, pevent_item->bg_color);                                    
                        divoom_outlook_calendar_add_lvgl_event(queue_item, (active_time - start_utc)/(24*3600), plvgl_event_item); 
                        break;
                    }
                }
                tmp_date.tm_year ++;
            }            
            break;
            
    }
}

void divoom_lvgl_calendar_load_calenday_event_free_item(void *item);

int divoom_google_calendar_load_calenday_current_month_compare(void *item1, void *item2);
/*********************************************************************
Function:         divoom_outlook_calendar_load_calenday_current_month
Description:      加载当前月的所有事件
Input:            data
Return:           
Author:           albert
**********************************************************************/

int divoom_outlook_calendar_load_calenday_current_month(void **queue_item)
{
    #if 1
    time_t cur_time, start_time, end_time;
    struct tm cur_date = {0};
    Pdivoom_outlook_calendar_event_item pevent_item;

    if(pdivoom_outlook_calendar_cal_handle == NULL)
    {
        return 0;
    }
    
    divoom_system_put_sem(pdivoom_outlook_calendar_cal_handle->run_sem);
    if(divoom_system_try_lock(pdivoom_outlook_calendar_cal_handle->lock_handle))
    {
        return 0;
    }
    
    for(int i = 0; i < 42; i++)
    {
        if(queue_item[i])
        {
            divoom_queue_destroy(queue_item[i], divoom_lvgl_calendar_load_calenday_event_free_item);
            queue_item[i] = NULL;
        }
    }
    
    cur_time = time(0);
    
    localtime_r(&cur_time, &cur_date);
    start_time = cur_time - (cur_date.tm_mday - 1) * 24*3600 - cur_date.tm_hour*3600 - cur_date.tm_min*60 - cur_date.tm_sec ;
    localtime_r(&start_time, &cur_date);
    start_time -= (cur_date.tm_wday  * 24*3600);
    end_time = start_time + 42 *24 * 3600;

    divoom_print("event num: %d\n", pdivoom_outlook_calendar_cal_handle->event_num);
    for(int j = 0; j < pdivoom_outlook_calendar_cal_handle->event_num; j ++)
    {
        divoom_outlook_calendar_load_calenday_all_from_event(queue_item, &pdivoom_outlook_calendar_cal_handle->pevent_list[j], start_time, end_time);
    }
    divoom_system_free_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);
    if(pdivoom_outlook_calendar_cal_handle->event_num > 1)
    {
        for(int i = 0; i < 42; i++)
        {
            if(queue_item[i])
            {
                divoom_queue_sort(queue_item[i], divoom_google_calendar_load_calenday_current_month_compare);
            }
        }
    }
    return 1;
    #endif
}

int divoom_outlook_calendar_check_run_flag(void)
{
    if(pdivoom_outlook_calendar_cal_handle->token && strlen(pdivoom_outlook_calendar_cal_handle->token))
    {
        if(pdivoom_outlook_calendar_cal_handle->pop_flag || pdivoom_outlook_calendar_cal_handle->event_flag || pdivoom_outlook_calendar_cal_handle->menu_flag || pdivoom_outlook_calendar_cal_handle->watch_refresh_flag)
        {
            return 1;
        }
    }
    return 0;
}

void *divoom_outlook_calendar_main_task(void *para)
{
    int j, cur_mon = -1;
    time_t cur_time;
    struct tm cur_date = {0};
    static int last_server_con = 0;
    Pdivoom_outlook_calendar_event_item pevent_item;
    
    while(1)
    {
        int server_con;

        cur_time = time(0);
        server_con = divoom_main_get_server_con_status();
        if(server_con && !last_server_con)
        {
            divoom_outlook_calendar_config_poll_reset();
        }
        last_server_con = server_con;

        if(pdivoom_outlook_calendar_cal_handle == NULL)
        {
            divoom_system_delay_ms(1000);
            continue;
        }
        if(pdivoom_outlook_calendar_cal_handle->token == NULL || strlen(pdivoom_outlook_calendar_cal_handle->token) == 0)
        {
            if(server_con && divoom_outlook_calendar_config_poll_due(cur_time))
            {
                Pdivoom_net_command ret_data;
                int got_token = 0;

                ret_data = divoom_net_app_send_packet_http(DIVOOM_NET_COMM_DEVICE_GET_OUTLOOK_CALENDAR_CONFIG, NULL, 0);
                if(ret_data)
                {
                    Pdivoom_device_google_calendar_config_respone_info pinfo =
                        (Pdivoom_device_google_calendar_config_respone_info)ret_data->cmd_data;

                    if(pinfo && strlen(pinfo->token))
                    {
                        divoom_outlook_calendar_update_token(pinfo->pop_flag, pinfo->event_flag, pinfo->token);
                        divoom_outlook_calendar_config_poll_on_success();
                        got_token = 1;
                    }
                    divoom_net_app_free_packet(ret_data);
                }
                if(!got_token)
                {
                    divoom_outlook_calendar_config_poll_on_fail(cur_time);
                }
            }
            divoom_system_delay_ms(500);
            continue;
        }
        if(server_con)
        {
            
            if(divoom_outlook_calendar_check_run_flag())
            {
                
                localtime_r(&cur_time, &cur_date);
                if(cur_date.tm_mon != cur_mon)
                {
                    cur_mon = cur_date.tm_mon;
                    pdivoom_outlook_calendar_cal_handle->event_request_time = 0;
                }
                if(divoom_outlook_calendar_graph_in_backoff())
                {
                    pdivoom_outlook_calendar_cal_handle->watch_refresh_flag = 0;
                }
                else if(pdivoom_outlook_calendar_cal_handle->event_request_time + DIVOOM_OUTLOOK_CALENDAR_REQUEST_EVENT_LIST_TIME < cur_time)
                {              
                    divoom_system_get_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);
                    divoom_outlook_calendar_request_default_color_info();
                    if(!divoom_outlook_calendar_graph_in_backoff())
                        divoom_outlook_calendar_request_url_info();
                    if(!divoom_outlook_calendar_graph_in_backoff())
                        divoom_outlook_calendar_request_color_info();
                    pdivoom_outlook_calendar_cal_handle->watch_refresh_flag = 0;
                    
                    divoom_system_free_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);
                    pdivoom_outlook_calendar_cal_handle->event_request_time = cur_time;
                }
            }
            else
            {
                divoom_system_get_sem(pdivoom_outlook_calendar_cal_handle->run_sem);
                continue;
            }
        }
        if(pdivoom_outlook_calendar_cal_handle->event_num)
        {
            divoom_system_get_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);
            for(j = 0; j < pdivoom_outlook_calendar_cal_handle->event_num; j ++)
            {
                pevent_item = &pdivoom_outlook_calendar_cal_handle->pevent_list[j];
                switch(pevent_item->status)
                {
                        
                    case DIVOOM_OUTLOOK_CALENDAR_STATUS_IDLE:
                    case DIVOOM_OUTLOOK_CALENDAR_STATUS_NOTICE:
                        if(pevent_item->notice_flag && pevent_item->notice_reminder_time && pevent_item->notice_reminder_time <= cur_time)
                        {
                            //抛出提醒事件
                            pevent_item->status = DIVOOM_OUTLOOK_CALENDAR_STATUS_NOTICE;
                            pevent_item->notice_reminder_time = 0;
                            divoom_outlook_calendar_send_event_notice_message(pevent_item, DIVOOM_OUTLOOK_CALENDAR_STATUS_NOTICE);
                        }
                        else if(pevent_item->next_start_utc < cur_time && pevent_item->next_end_utc > cur_time)
                        {
                            //事件激活
                            if(pevent_item->all_day_flag)
                            {
                                pevent_item->status = DIVOOM_OUTLOOK_CALENDAR_STATUS_PASS;
                            }
                            else
                            {
                                pevent_item->status = DIVOOM_OUTLOOK_CALENDAR_STATUS_ACTIVE;
                                divoom_outlook_calendar_send_event_notice_message(pevent_item, DIVOOM_OUTLOOK_CALENDAR_STATUS_ACTIVE);
                            }
                        }
                        else if(pevent_item->next_end_utc < cur_time)
                        {
                            pevent_item->status = DIVOOM_OUTLOOK_CALENDAR_STATUS_PASS;
                        }
                        break;
                        
                    case DIVOOM_OUTLOOK_CALENDAR_STATUS_PASS:
                    case DIVOOM_OUTLOOK_CALENDAR_STATUS_ACTIVE:
                        if(pevent_item->next_end_utc < cur_time)
                        {
                            //将这个事件弹出,通知事件结束
                            int cur_status = pevent_item->status;
                            divoom_print("event : %s, %d, %d\n", pevent_item->title,pevent_item->next_end_utc, cur_time);
                            pevent_item->status = DIVOOM_OUTLOOK_CALENDAR_STATUS_IDLE;
                            divoom_outlook_calendar_formart_event_item(pevent_item);
                            if(cur_status == DIVOOM_OUTLOOK_CALENDAR_STATUS_ACTIVE)
                            {
                                if(pdivoom_outlook_calendar_cal_handle->prunning_event == pevent_item
                                    || pdivoom_outlook_calendar_cal_handle->prunning_event == NULL)
                                {
                                    divoom_outlook_calendar_send_event_notice_message(pevent_item, DIVOOM_OUTLOOK_CALENDAR_STATUS_IDLE);
                                }
                                else
                                {
                                    divoom_print("outlook end skip: %s (prunning=%s)\n",
                                        pevent_item->title ? pevent_item->title : "?",
                                        pdivoom_outlook_calendar_cal_handle->prunning_event->title
                                            ? pdivoom_outlook_calendar_cal_handle->prunning_event->title : "?");
                                }
                            }
                        }
                        break;
                }
            }
            divoom_system_free_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);
        }
        divoom_system_delay_ms(1000);
        
    }
}

/*********************************************************************
Function:         divoom_outlook_calendar_update_menu_flag
Description:      进入与退出菜单
Input:            menu_flag
Return:           
Author:           albert
**********************************************************************/

void divoom_outlook_calendar_update_menu_flag(uint8 menu_flag)
{
    if(pdivoom_outlook_calendar_cal_handle == NULL)
    {
        return ;
    }
    pdivoom_outlook_calendar_cal_handle->menu_flag = menu_flag;
    divoom_system_put_sem(pdivoom_outlook_calendar_cal_handle->run_sem);
}

/*********************************************************************
Function:         divoom_outlook_calendar_update_switch
Description:      更新开关
Input:            purl_head_info
                　head_num
                  clock_id
Return:           
Author:           albert
**********************************************************************/

void divoom_outlook_calendar_update_switch(int pop_flag, int event_flag)
{
    if(pdivoom_outlook_calendar_cal_handle == NULL)
    {
        return ;
    }
    divoom_system_get_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);
    pdivoom_outlook_calendar_cal_handle->pop_flag = pop_flag;
    if(event_flag == 0 && pdivoom_outlook_calendar_cal_handle->event_flag == 1)
    {
        divoom_outlook_calendar_send_event_notice_message(NULL, DIVOOM_OUTLOOK_CALENDAR_STATUS_IDLE);
    }
    pdivoom_outlook_calendar_cal_handle->event_flag = event_flag;
    divoom_system_free_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);
    divoom_outlook_calendar_save_config();    
    divoom_system_put_sem(pdivoom_outlook_calendar_cal_handle->run_sem);
}



/*********************************************************************
Function:         divoom_outlook_calendar_get_update_flag
Description:      得到更新标志
Input:            
Return:           NONE
Author:           albert
**********************************************************************/

int divoom_outlook_calendar_get_update_flag(void)
{
    int ret;

    if(pdivoom_outlook_calendar_cal_handle == NULL)
    {
        return 0;
    }
    ret = pdivoom_outlook_calendar_cal_handle->update_flag;

    pdivoom_outlook_calendar_cal_handle->update_flag = 0;
    return ret;
}

/*********************************************************************
Function:         divoom_outlook_calendar_check_login
Description:      是否有ＴＯＫＥＮ
Input:            
Return:           
Author:           albert
**********************************************************************/

int divoom_outlook_calendar_check_login(void)
{
    if(pdivoom_outlook_calendar_cal_handle)
    {
        return pdivoom_outlook_calendar_cal_handle->token && strlen(pdivoom_outlook_calendar_cal_handle->token);
    }
    return 0;
    
}

/*********************************************************************
Function:         divoom_outlook_calendar_update
Description:      更新ＴＯＫＥＮ
Input:            purl_head_info
                　head_num
                  clock_id
Return:           
Author:           albert
**********************************************************************/

void divoom_outlook_calendar_update_token(int pop_flag, int event_flag, char *token)
{
    #if 1
    if(token == NULL || strlen(token) == 0)
    {
        divoom_print("token is NULL!\n");
        return;
    }
    if(pdivoom_outlook_calendar_cal_handle == NULL)
    {
        divoom_outlook_calendar_save_config_values(pop_flag, event_flag, token);
        divoom_print("outlook token saved early (init pending)\n");
        return;
    }
    divoom_system_get_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);
        if(strlen(token) &&(pdivoom_outlook_calendar_cal_handle->token == NULL  || strcmp(pdivoom_outlook_calendar_cal_handle->token, token)))
        {
            //如果TOKEN发生变化，将重新获取一次
            if(pdivoom_outlook_calendar_cal_handle->token)
            {
                divoom_system_free(pdivoom_outlook_calendar_cal_handle->token);
                pdivoom_outlook_calendar_cal_handle->token = NULL;
            }
            pdivoom_outlook_calendar_cal_handle->token = divoom_app_com_copy_string(token, -1);
            pdivoom_outlook_calendar_cal_handle->event_request_time = 0;
            pdivoom_outlook_calendar_cal_handle->graph_backoff_until = 0;
            divoom_outlook_calendar_config_poll_reset();
        }
        if(pop_flag != -1)
        {
            pdivoom_outlook_calendar_cal_handle->pop_flag = pop_flag;
        }
        if(event_flag != -1)
        {
            if(event_flag == 0 && pdivoom_outlook_calendar_cal_handle->event_flag == 1)
            {
                divoom_outlook_calendar_send_event_notice_message(NULL, DIVOOM_OUTLOOK_CALENDAR_STATUS_IDLE);
            }
            pdivoom_outlook_calendar_cal_handle->event_flag = event_flag;
        }
        divoom_system_free_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);
        divoom_outlook_calendar_save_config();
        divoom_system_put_sem(pdivoom_outlook_calendar_cal_handle->run_sem);
    #endif
}

#define DIVOOM_OUTLOOK_CALENDAR_TODAY_WATCH_MAX_ROWS DIVOOM_CALENDAR_TODAY_WATCH_MAX_ROWS

static void divoom_outlook_calendar_blend_watch_beige(uint8 *rgb)
{
	if (rgb == NULL)
		return;
	rgb[0] = (uint8)((rgb[0] * 55 + 0xE5 * 45) / 100);
	rgb[1] = (uint8)((rgb[1] * 55 + 0xD9 * 45) / 100);
	rgb[2] = (uint8)((rgb[2] * 55 + 0xC5 * 45) / 100);
}

static void divoom_outlook_calendar_soften_watch_color(Pdivoom_outlook_calendar_event_item pevent_item, uint8 *out_color)
{
	if (pevent_item == NULL || out_color == NULL)
		return;
	out_color[0] = (uint8)((pevent_item->bg_color >> 16) & 0xFF);
	out_color[1] = (uint8)((pevent_item->bg_color >> 8) & 0xFF);
	out_color[2] = (uint8)(pevent_item->bg_color & 0xFF);
	divoom_outlook_calendar_blend_watch_beige(out_color);
}

static int divoom_outlook_calendar_pack_watch_color(uint8 *bg_color)
{
	if (bg_color == NULL)
		return 0;
	return ((int)bg_color[0] << 16) | ((int)bg_color[1] << 8) | (int)bg_color[2];
}

static void divoom_outlook_calendar_format_today_event_time(Pdivoom_outlook_calendar_event_item pevent_item, char *buf, int buf_size)
{
	struct tm start_tm, end_tm;
	time_t start_local, end_local;

	if (pevent_item == NULL || buf == NULL || buf_size <= 0)
		return;
	start_local = pevent_item->start_utc;
	end_local = pevent_item->end_utc;
	localtime_r(&start_local, &start_tm);
	localtime_r(&end_local, &end_tm);
	sprintf(buf, "%02d:%02d-%02d:%02d",
		start_tm.tm_hour, start_tm.tm_min, end_tm.tm_hour, end_tm.tm_min);
}

static int divoom_outlook_calendar_watch_try_lock(void)
{
	int retry;

	if (pdivoom_outlook_calendar_cal_handle == NULL)
		return 1;
	if (!divoom_system_try_lock(pdivoom_outlook_calendar_cal_handle->lock_handle))
		return 0;
	for (retry = 0; retry < 50; retry++)
	{
		divoom_system_delay_ms(10);
		if (!divoom_system_try_lock(pdivoom_outlook_calendar_cal_handle->lock_handle))
			return 0;
	}
	return 1;
}

static void divoom_outlook_calendar_request_watch_refresh_internal(int force)
{
	if (pdivoom_outlook_calendar_cal_handle == NULL)
		return;
	if (divoom_system_try_lock(pdivoom_outlook_calendar_cal_handle->lock_handle))
		return;
	if (force)
		pdivoom_outlook_calendar_cal_handle->graph_backoff_until = 0;
	if (force || pdivoom_outlook_calendar_cal_handle->event_request_time + DIVOOM_OUTLOOK_CALENDAR_REQUEST_EVENT_LIST_TIME <= time(0))
		pdivoom_outlook_calendar_cal_handle->event_request_time = 0;
	pdivoom_outlook_calendar_cal_handle->watch_refresh_flag = 1;
	divoom_system_free_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);
	divoom_system_put_sem(pdivoom_outlook_calendar_cal_handle->run_sem);
}

void divoom_outlook_calendar_request_watch_refresh(void)
{
	divoom_outlook_calendar_request_watch_refresh_internal(0);
}

void divoom_outlook_calendar_request_watch_refresh_force(void)
{
	divoom_outlook_calendar_request_watch_refresh_internal(1);
}

int divoom_outlook_calendar_watch_cache_is_stale(const char *url_info, int cached_num, int clock_id)
{
	divoom_calendar_today_watch_layout layout;

	if (url_info == NULL || cached_num <= 0)
		return 0;
	if (strstr(url_info, "TodayWatch") != NULL)
	{
		layout = divoom_calendar_today_watch_get_layout(clock_id);
		return cached_num < layout.data_num;
	}
	if (strstr(url_info, "NextEventWatch") != NULL)
		return cached_num < DIVOOM_OUTLOOK_CALENDAR_NEXT_EVENT_WATCH_DATA_NUM;
	if (strstr(url_info, "ThisWeekWatch") != NULL)
		return cached_num < DIVOOM_OUTLOOK_CALENDAR_WEEK_WATCH_DATA_NUM;
	return 0;
}

int divoom_outlook_calendar_get_this_week_watch_data(void *pdata)
{
	Pdivoom_clock_control_disp_data pdisp_data = (Pdivoom_clock_control_disp_data)pdata;
	Pdivoom_outlook_calendar_event_item pevent_item;
	time_t now, day_start, week_start;
	struct tm cur_tm;
	int i, d, day_counts[7];
	static time_t last_watch_sync_req = 0;

	pdisp_data->num = 0;
	pdisp_data->update = 0;
	memset(day_counts, 0, sizeof(day_counts));

	if (pdivoom_outlook_calendar_cal_handle == NULL)
	{
		divoom_clock_control_disp_data_deinit(pdisp_data);
	divoom_clock_control_disp_data_init(pdisp_data);
		pdisp_data->num = 1;
		pdisp_data->update = 1;
		return 1;
	}

	now = time(0);
	if (now - last_watch_sync_req >= 30)
	{
		last_watch_sync_req = now;
		divoom_outlook_calendar_request_watch_refresh();
	}

	if (divoom_outlook_calendar_watch_try_lock())
		return 0;

	divoom_clock_control_disp_data_deinit(pdisp_data);
	divoom_clock_control_disp_data_init(pdisp_data);
	localtime_r(&now, &cur_tm);
	day_start = now - cur_tm.tm_hour * 3600 - cur_tm.tm_min * 60 - cur_tm.tm_sec;
	week_start = day_start - ((cur_tm.tm_wday + 6) % 7) * 86400;

	for (i = 0; i < pdivoom_outlook_calendar_cal_handle->event_num; i++)
	{
		pevent_item = &pdivoom_outlook_calendar_cal_handle->pevent_list[i];
		for (d = 0; d < 7; d++)
		{
			time_t d_start = week_start + (time_t)d * 86400;
			time_t d_end = d_start + 86400;

			if (pevent_item->end_utc <= d_start || pevent_item->start_utc >= d_end)
				continue;
			if (day_counts[d] < 99)
				day_counts[d]++;
		}
	}

	divoom_system_free_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);

	pdisp_data->num = 1;
	pdisp_data->update = 1;
	memset(divoom_clock_control_disp_data_get_item(pdisp_data, 0), 0, sizeof(divoom_clock_control_disp_array_item));
	DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).num = DIVOOM_OUTLOOK_CALENDAR_WEEK_WATCH_DATA_NUM;
	for (d = 0; d < 7; d++)
		DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).hour_value[d] = day_counts[d];
	DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).hour_value[7] = (int)week_start;
	divoom_print("outlook week watch: counts=%d,%d,%d,%d,%d,%d,%d week_start=%d events=%d\n",
		day_counts[0], day_counts[1], day_counts[2], day_counts[3],
		day_counts[4], day_counts[5], day_counts[6], (int)week_start,
		pdivoom_outlook_calendar_cal_handle->event_num);
	return 1;
}

#define DIVOOM_CALENDAR_NEXT_EVENT_TIME_EMPTY       "--:-- - --:--"
#define DIVOOM_CALENDAR_NEXT_EVENT_COUNTDOWN_EMPTY  "--:--:--"

static void divoom_outlook_calendar_fill_today_event_watch_empty(Pdivoom_clock_control_disp_data pdisp_data, divoom_calendar_today_watch_layout layout, int logged_in)
{
	int row;

	pdisp_data->num = 1;
	pdisp_data->update = 1;
	memset(divoom_clock_control_disp_data_get_item(pdisp_data, 0), 0, sizeof(divoom_clock_control_disp_array_item));
	DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).num = layout.data_num;
	DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).hour_value[5] = -1;

	strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[0], DIVOOM_CALENDAR_NEXT_EVENT_TIME_EMPTY);
	if (logged_in)
	{
		strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[5], "No Events Today");
		if (layout.has_footer)
			strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[10], "No events today");
	}
	else
	{
		strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[5], "Calendar Not Connected");
		if (layout.has_footer)
			strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[10], "Connect calendar");
	}
	DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data_roll_flag[5] = 1;
	divoom_calendar_format_today_activities(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[DIVOOM_CALENDAR_TODAY_ACTIVITIES_INDEX], 0);

	for (row = 1; row < layout.max_rows; row++)
	{
		DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[row][0] = 0;
		DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[row + 5][0] = 0;
		DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).hour_value[row] = 0;
	}
}

int divoom_outlook_calendar_get_today_event_watch_data(void *pdata, int clock_id)
{
	Pdivoom_clock_control_disp_data pdisp_data = (Pdivoom_clock_control_disp_data)pdata;
	divoom_calendar_today_watch_layout layout = divoom_calendar_today_watch_get_layout(clock_id);
	Pdivoom_outlook_calendar_event_item today_events[32];
	int today_event_num = 0;
	Pdivoom_outlook_calendar_event_item pevent_item, tmp_event;
	time_t now, day_start, day_end;
	struct tm cur_tm;
	int i, j, row;
	static time_t last_watch_sync_req = 0;

	pdisp_data->num = 0;
	pdisp_data->update = 0;

	if (pdivoom_outlook_calendar_cal_handle == NULL || !divoom_outlook_calendar_check_login())
	{
		divoom_clock_control_disp_data_deinit(pdisp_data);
		divoom_clock_control_disp_data_init(pdisp_data);
		divoom_outlook_calendar_fill_today_event_watch_empty(pdisp_data, layout, divoom_outlook_calendar_check_login());
		return 1;
	}

	now = time(0);
	if (now - last_watch_sync_req >= 30)
	{
		last_watch_sync_req = now;
		divoom_outlook_calendar_request_watch_refresh();
	}

	if (divoom_outlook_calendar_watch_try_lock())
		return 0;

	divoom_clock_control_disp_data_deinit(pdisp_data);
	divoom_clock_control_disp_data_init(pdisp_data);
	localtime_r(&now, &cur_tm);
	day_start = now - cur_tm.tm_hour * 3600 - cur_tm.tm_min * 60 - cur_tm.tm_sec;
	day_end = day_start + 24 * 3600;

	for (i = 0; i < pdivoom_outlook_calendar_cal_handle->event_num; i++)
	{
		pevent_item = &pdivoom_outlook_calendar_cal_handle->pevent_list[i];
		if (pevent_item->end_utc <= day_start || pevent_item->start_utc >= day_end)
			continue;
		if (today_event_num >= (int)(sizeof(today_events) / sizeof(today_events[0])))
			break;
		today_events[today_event_num++] = pevent_item;
	}

	for (i = 0; i < today_event_num; i++)
	{
		for (j = i + 1; j < today_event_num; j++)
		{
			if (today_events[j]->start_utc < today_events[i]->start_utc)
			{
				tmp_event = today_events[i];
				today_events[i] = today_events[j];
				today_events[j] = tmp_event;
			}
		}
	}

	{
		int start_idx, show_num, remaining, win_start, row_offset, evt_idx;
		int max_rows = layout.max_rows;

		start_idx = 0;
		for (i = 0; i < today_event_num; i++)
		{
			if (today_events[i]->end_utc > now)
			{
				start_idx = i;
				break;
			}
			start_idx = today_event_num;
		}

		if (start_idx == 0 || today_event_num - start_idx >= max_rows)
		{
			win_start = start_idx;
			show_num = today_event_num - start_idx;
			if (show_num > max_rows)
				show_num = max_rows;
			row_offset = 0;
		}
		else
		{
			show_num = today_event_num;
			if (show_num > max_rows)
				show_num = max_rows;
			win_start = today_event_num - show_num;
			if (win_start < 0)
				win_start = 0;
			row_offset = 0;
		}
		remaining = today_event_num - (win_start + show_num);
		if (remaining < 0)
			remaining = 0;

		divoom_system_free_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);

		pdisp_data->num = 1;
		pdisp_data->update = 1;
		memset(divoom_clock_control_disp_data_get_item(pdisp_data, 0), 0, sizeof(divoom_clock_control_disp_array_item));
		DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).num = layout.data_num;
		int active_watch_row = -1;
		time_t active_watch_start = 0;

		DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).hour_value[5] = -1;

		if (today_event_num == 0)
		{
			divoom_outlook_calendar_fill_today_event_watch_empty(pdisp_data, layout, 1);
			divoom_print("outlook today watch: clock:%d rows:%d total:0 (empty)\n", clock_id, max_rows);
			return 1;
		}

		for (row = 0; row < max_rows; row++)
		{
			if (row >= row_offset && row < row_offset + show_num)
			{
				uint8 watch_color[3];
				time_t evt_start, evt_end;

				evt_idx = win_start + (row - row_offset);
				pevent_item = today_events[evt_idx];
				evt_start = pevent_item->next_start_utc ? pevent_item->next_start_utc : pevent_item->start_utc;
				evt_end = pevent_item->next_end_utc ? pevent_item->next_end_utc : pevent_item->end_utc;
				divoom_outlook_calendar_format_today_event_time(pevent_item,
					DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[row], DIVOOM_CLOCK_CONTROL_DISPLAY_MAX_LEN);
				if (pevent_item->title && strlen(pevent_item->title) > 0)
					strncpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[row + 5], pevent_item->title, DIVOOM_CLOCK_CONTROL_DISPLAY_MAX_LEN - 1);
				else
					strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[row + 5], "No title");
				divoom_outlook_calendar_soften_watch_color(pevent_item, watch_color);
				DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).hour_value[row] = divoom_outlook_calendar_pack_watch_color(watch_color);
				DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data_roll_flag[row + 5] = 1;
				if (evt_start <= now && evt_end > now)
				{
					if (active_watch_row < 0 || evt_start >= active_watch_start)
					{
						active_watch_row = row;
						active_watch_start = evt_start;
					}
				}
			}
			else
			{
				DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[row][0] = 0;
				DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[row + 5][0] = 0;
				DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).hour_value[row] = 0;
			}
		}

		DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).hour_value[5] = active_watch_row;

		for (row = max_rows; row < DIVOOM_CALENDAR_TODAY_WATCH_MAX_ROWS; row++)
		{
			DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[row][0] = 0;
			DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[row + 5][0] = 0;
			DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).hour_value[row] = 0;
		}

		if (layout.has_footer)
		{
		if (remaining > 0)
		{
			if (remaining == 1)
				strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[10], "1 More Event Today");
			else
				sprintf(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[10], "%d More Events Today", remaining);
		}
		else if (start_idx >= today_event_num && today_event_num > 0)
			strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[10], "No More Events Today");
		else
			strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[10], " ");
		}
		else
		{
			DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[10][0] = 0;
		}
		divoom_calendar_format_today_activities(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[DIVOOM_CALENDAR_TODAY_ACTIVITIES_INDEX], today_event_num);

		divoom_print("outlook today watch: clock:%d rows:%d total:%d, start:%d, win_start:%d, show:%d, active_row:%d\n",
			clock_id,
			max_rows,
			today_event_num,
			start_idx < today_event_num ? start_idx : -1,
			win_start,
			show_num,
			DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).hour_value[5]);
		return 1;
	}

	divoom_system_free_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);
	return 0;
}

static void divoom_outlook_calendar_fill_next_event_watch_defaults(Pdivoom_clock_control_disp_data pdisp_data, int logged_in, int today_count)
{
	divoom_clock_control_disp_data_deinit(pdisp_data);
	divoom_clock_control_disp_data_init(pdisp_data);
	pdisp_data->num = 1;
	pdisp_data->update = 1;
	memset(divoom_clock_control_disp_data_get_item(pdisp_data, 0), 0, sizeof(divoom_clock_control_disp_array_item));
	DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).num = DIVOOM_OUTLOOK_CALENDAR_NEXT_EVENT_WATCH_DATA_NUM;
	DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data_roll_flag[1] = 1;
	DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data_roll_flag[3] = 2;
	DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).hour_value[0] = 0;

	if (logged_in)
	{
		strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[1], "No Upcoming Events");
		strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[2], DIVOOM_CALENDAR_NEXT_EVENT_TIME_EMPTY);
		strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[3], "No more events today");
	}
	else
	{
		strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[1], "Calendar Not Connected");
		strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[2], DIVOOM_CALENDAR_NEXT_EVENT_TIME_EMPTY);
		strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[3], "Connect calendar");
	}
	divoom_calendar_format_today_activities(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[DIVOOM_CALENDAR_TODAY_ACTIVITIES_INDEX], today_count);
}

int divoom_outlook_calendar_get_next_event_remain_sec(void)
{
	Pdivoom_outlook_calendar_event_item pevent_item;
	static time_t last_ai_sync_req = 0;
	time_t now;
	time_t remain_time;
	int remain_sec;
	int best_remain;
	int i;

	if (pdivoom_outlook_calendar_cal_handle == NULL ||
		divoom_outlook_calendar_check_login() == 0)
	{
		return DIVOOM_CALENDAR_EVENT_REMAIN_NONE;
	}
	now = time(0);
	if (now - last_ai_sync_req >= 30)
	{
		last_ai_sync_req = now;
		divoom_outlook_calendar_request_watch_refresh();
	}
	/* 网络线程持锁时立即返回，避免阻塞 AI 表盘的 LVGL 定时回调。 */
	if (divoom_system_try_lock(pdivoom_outlook_calendar_cal_handle->lock_handle))
	{
		return DIVOOM_CALENDAR_EVENT_REMAIN_BUSY;
	}
	best_remain = DIVOOM_CALENDAR_EVENT_REMAIN_NONE;
	if (pdivoom_outlook_calendar_cal_handle->pevent_list != NULL)
	{
		for (i = 0; i < pdivoom_outlook_calendar_cal_handle->event_num; i++)
		{
			pevent_item = &pdivoom_outlook_calendar_cal_handle->pevent_list[i];
			if (pevent_item->start_utc < now)
			{
				continue;
			}
			remain_time = pevent_item->start_utc - now;
			remain_sec = remain_time > 0x7FFFFFFF ? 0x7FFFFFFF : (int)remain_time;
			if (best_remain < 0 || remain_sec < best_remain)
			{
				best_remain = remain_sec;
			}
		}
	}
	divoom_system_free_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);
	return best_remain;
}

int divoom_outlook_calendar_get_next_event_watch_data(void *pdata)
{
	Pdivoom_clock_control_disp_data pdisp_data = (Pdivoom_clock_control_disp_data)pdata;
	Pdivoom_outlook_calendar_event_item best = NULL;
	Pdivoom_outlook_calendar_event_item pevent_item;
	time_t now, day_start, day_end, start_local, end_local;
	struct tm start_tm, end_tm, cur_tm;
	int i, future_today = 0, remaining, total_today = 0;
	static time_t last_watch_sync_req = 0;

	pdisp_data->num = 0;
	pdisp_data->update = 0;

	if (pdivoom_outlook_calendar_cal_handle == NULL || !divoom_outlook_calendar_check_login())
	{
		divoom_outlook_calendar_fill_next_event_watch_defaults(pdisp_data, 0, 0);
		return 1;
	}

	now = time(0);
	if (now - last_watch_sync_req >= 30)
	{
		last_watch_sync_req = now;
		divoom_outlook_calendar_request_watch_refresh();
	}

	if (divoom_outlook_calendar_watch_try_lock())
		return 0;

	divoom_clock_control_disp_data_deinit(pdisp_data);
	divoom_clock_control_disp_data_init(pdisp_data);
	localtime_r(&now, &cur_tm);
	day_start = now - cur_tm.tm_hour * 3600 - cur_tm.tm_min * 60 - cur_tm.tm_sec;
	day_end = day_start + 24 * 3600;

	for (i = 0; i < pdivoom_outlook_calendar_cal_handle->event_num; i++)
	{
		pevent_item = &pdivoom_outlook_calendar_cal_handle->pevent_list[i];
		if (pevent_item->start_utc >= day_end || pevent_item->end_utc <= day_start)
			continue;
		total_today++;
		if (pevent_item->end_utc <= now)
			continue;
		if (pevent_item->start_utc > now)
		{
			future_today++;
			if (best == NULL || pevent_item->start_utc < best->start_utc)
				best = pevent_item;
		}
	}

	divoom_system_free_lock(pdivoom_outlook_calendar_cal_handle->lock_handle);

	pdisp_data->num = 1;
	pdisp_data->update = 1;
	memset(divoom_clock_control_disp_data_get_item(pdisp_data, 0), 0, sizeof(divoom_clock_control_disp_array_item));
	DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).num = DIVOOM_OUTLOOK_CALENDAR_NEXT_EVENT_WATCH_DATA_NUM;

	if (best == NULL)
	{
		divoom_outlook_calendar_fill_next_event_watch_defaults(pdisp_data, 1, total_today);
		return 1;
	}

	if (best->title && strlen(best->title) > 0)
		strncpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[1], best->title, DIVOOM_CLOCK_CONTROL_DISPLAY_MAX_LEN - 1);
	else
		strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[1], "No title");

	start_local = best->start_utc;
	end_local = best->end_utc;
	localtime_r(&start_local, &start_tm);
	localtime_r(&end_local, &end_tm);
	sprintf(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[2], "%02d:%02d-%02d:%02d",
		start_tm.tm_hour, start_tm.tm_min, end_tm.tm_hour, end_tm.tm_min);

	remaining = future_today - 1;
	if (remaining <= 0)
		strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[3], "0 more events today");
	else if (remaining == 1)
		strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[3], "1 more event today");
	else
		sprintf(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[3], "%d more events today", remaining);

	DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).bg_color[0] = (uint8)((best->bg_color >> 16) & 0xFF);
	DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).bg_color[1] = (uint8)((best->bg_color >> 8) & 0xFF);
	DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).bg_color[2] = (uint8)(best->bg_color & 0xFF);
	DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).hour_value[0] = (int)best->start_utc;
	DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data_roll_flag[1] = 1;
	DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data_roll_flag[3] = 2;
	divoom_calendar_format_today_activities(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[DIVOOM_CALENDAR_TODAY_ACTIVITIES_INDEX], total_today);
	divoom_print("outlook next event watch: %s, today:%d, future:%d\n",
		DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[1], total_today, future_today);
	return 1;
}


void divoom_outlook_calendar_init(void)
{
    if (pdivoom_outlook_calendar_cal_handle != NULL)
    {
        return;
    }
    pdivoom_outlook_calendar_cal_handle = divoom_system_malloc(sizeof(divoom_outlook_calendar_cal_ctrl));
    memset(pdivoom_outlook_calendar_cal_handle, 0, sizeof(divoom_outlook_calendar_cal_ctrl));
    divoom_outlook_calendar_load_config();
    if(pdivoom_outlook_calendar_cal_handle->token == NULL || strlen(pdivoom_outlook_calendar_cal_handle->token) == 0)
        divoom_print("outlook calendar: no token after load_config\n");
    pdivoom_outlook_calendar_cal_handle->event_num = 0;
    pdivoom_outlook_calendar_cal_handle->run_sem = divoom_system_create_sem(1, 0);
    pdivoom_outlook_calendar_cal_handle->color_queue = divoom_queue_init(100, __func__, __LINE__);
    pdivoom_outlook_calendar_cal_handle->default_color_queue = divoom_queue_init(100, __func__, __LINE__);
    pdivoom_outlook_calendar_cal_handle->lock_handle = divoom_system_create_lock();
    divoom_system_create_task(divoom_outlook_calendar_main_task, "outlook_cal", pdivoom_outlook_calendar_cal_handle, DIVOOM_TASK_STACK_OUTLOOK_CAL);
    
}
#endif

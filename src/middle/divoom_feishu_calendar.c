#include "divoom.h"
#include <time.h>
#include "divoom_menu_control.h"

#if 1
#define DIVOOM_FEISHU_CALENDAR_REQUEST_EVENT_LIST_TIME  30

enum{
    DIVOOM_FEISHU_CALENDAR_STATUS_IDLE,
    DIVOOM_FEISHU_CALENDAR_STATUS_NOTICE,
    DIVOOM_FEISHU_CALENDAR_STATUS_ACTIVE,
    DIVOOM_FEISHU_CALENDAR_STATUS_PASS,
    DIVOOM_FEISHU_CALENDAR_STATUS_TIME_OUT,
};

void divoom_feishu_calendar_send_event_notice_message(void *data, int status);


enum{
    DIVOOM_FEISHU_CALENDAR_REPEAT_NONE,
    DIVOOM_FEISHU_CALENDAR_REPEAT_DAY,     //按天重复
    DIVOOM_FEISHU_CALENDAR_REPEAT_WEEK,    //按周
    DIVOOM_FEISHU_CALENDAR_REPEAT_MON,     //按月
    DIVOOM_FEISHU_CALENDAR_REPEAT_YEAR,    //按年
};
    
// Outlook Category Colors RGB Values
const uint32_t divoom_feishu_calendar_category_color_list[] = {
    
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

typedef struct _divoom_feishu_calendar_color_item
{
    int color_index;
    char *color_name;
    char *id;
}divoom_feishu_calendar_color_item, *Pdivoom_feishu_calendar_color_item;

typedef struct _divoom_feishu_calendar_default_color_item
{
    uint32 color_value;
    char *id;
}divoom_feishu_calendar_default_color_item, *Pdivoom_feishu_calendar_default_color_item;

typedef struct _divoom_feishu_calendar_event_item
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
}divoom_feishu_calendar_event_item, *Pdivoom_feishu_calendar_event_item;


typedef struct _divoom_feishu_calendar_cal_ctrl
{
    uint8   update_default_color_flag;
    uint8   menu_flag;    // 是否显示菜单
    uint8   pop_flag;    // 是否有弹窗提醒
    uint8   event_flag;    //是否事件通知功能
    uint8   update_flag;    //是否有变化，这个读清
    int     event_num;
    int     event_request_time;   //更新时间
    void  *run_sem;
    char  *token;
    char  *calendar_id;
    void    *color_queue;
    void    *default_color_queue;
    void   *lock_handle;
    Pdivoom_feishu_calendar_event_item pevent_list;      //日历列表
    Pdivoom_feishu_calendar_event_item prunning_event;      //正在激活的日历
    void    *notice_lvgl_obj;
}divoom_feishu_calendar_cal_ctrl, *Pdivoom_feishu_calendar_cal_ctrl;

void divoom_feishu_calendar_send_event_notice_message(void *data, int status);

Pdivoom_feishu_calendar_cal_ctrl pdivoom_feishu_calendar_cal_handle = NULL;

void divoom_feishu_calendar_load_config(void)
{
    divoom_cJSON *root,*item;
    char *file_buf;

    file_buf = divoom_app_com_read_file(DIVOOM_FEISHU_CALENDAR_CONFIG, NULL);
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

    pdivoom_feishu_calendar_cal_handle->pop_flag = divoom_cJSON_GetObjectInt(root, "pop_flag");
    pdivoom_feishu_calendar_cal_handle->event_flag = divoom_cJSON_GetObjectInt(root, "event_flag");
	item = divoom_cJSON_GetObjectItem(root, "token");
    if(item && item->valuestring)
    {
        pdivoom_feishu_calendar_cal_handle->token = divoom_app_com_copy_string(item->valuestring, -1);
    }
	item = divoom_cJSON_GetObjectItem(root, "calendar_id");
    if(item && item->valuestring)
    {
        if(pdivoom_feishu_calendar_cal_handle->calendar_id)
        {
            divoom_system_free(pdivoom_feishu_calendar_cal_handle->calendar_id);
        }
        pdivoom_feishu_calendar_cal_handle->calendar_id = divoom_app_com_copy_string(item->valuestring, -1);
    }
    return ;
}




void divoom_feishu_calendar_update_extra(char *calendar_id, char *union_id)
{
    if(pdivoom_feishu_calendar_cal_handle == NULL)
    {
        return;
    }
    divoom_system_get_lock(pdivoom_feishu_calendar_cal_handle->lock_handle);
    if(calendar_id && calendar_id[0])
    {
        if(pdivoom_feishu_calendar_cal_handle->calendar_id)
        {
            divoom_system_free(pdivoom_feishu_calendar_cal_handle->calendar_id);
        }
        pdivoom_feishu_calendar_cal_handle->calendar_id = divoom_app_com_copy_string(calendar_id, -1);
        pdivoom_feishu_calendar_cal_handle->event_request_time = 0;
    }
    divoom_system_free_lock(pdivoom_feishu_calendar_cal_handle->lock_handle);
    divoom_feishu_calendar_save_config();
}

void divoom_feishu_calendar_save_config(void)
{
    char *json_info = NULL;
    divoom_cJSON *root;
	root = divoom_cJSON_CreateObject();

    if(root == NULL)
    {
        return;
    }
    
    
	divoom_cJSON_AddNumberToObject(root, "pop_flag", pdivoom_feishu_calendar_cal_handle->pop_flag);
	divoom_cJSON_AddNumberToObject(root, "event_flag", pdivoom_feishu_calendar_cal_handle->event_flag);
    if(pdivoom_feishu_calendar_cal_handle->token)
    {
        divoom_cJSON_AddStringToObject(root , "token", pdivoom_feishu_calendar_cal_handle->token);
    }
    if(pdivoom_feishu_calendar_cal_handle->calendar_id)
    {
        divoom_cJSON_AddStringToObject(root , "calendar_id", pdivoom_feishu_calendar_cal_handle->calendar_id);
    }
    json_info = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);
    if (json_info != NULL)
    {
        divoom_app_com_write_file(DIVOOM_FEISHU_CALENDAR_CONFIG, (uint8 *)json_info, strlen(json_info));
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
Function:         divoom_feishu_calendar_free_all_cal
Description:      释放日历资源
Input:            data
Return:           
Author:           albert
**********************************************************************/

void divoom_feishu_calendar_free_all_event(Pdivoom_feishu_calendar_event_item pevent_list, int event_num)
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

int divoom_feishu_calendar_getDaysInMonth(int year, int month) {
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

void divoom_feishu_calendar_get_next_mday(struct tm *cur_date)
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
        days = divoom_feishu_calendar_getDaysInMonth(cur_date->tm_year + 1900, cur_date->tm_mon);
        if(days >= cur_date->tm_mday)
        {
            return;
        }
    }
}


void divoom_feishu_calendar_formart_event_item(Pdivoom_feishu_calendar_event_item pevent_item)
{
    int today_sec;
    time_t cur_time;
    struct tm cur_date = {0}, start_date = {0};
    

    cur_time = time(0);
    if( pevent_item->range_end_utc && cur_time >= pevent_item->range_end_utc)
    {
        //如果超过结束的，将丢弃
        pevent_item->status = DIVOOM_FEISHU_CALENDAR_STATUS_TIME_OUT;
        return;
    }
    
    if(cur_time < pevent_item->range_start_utc)
    {
        //如果还没有到的话，直接使用开始时间
        pevent_item->next_start_utc = pevent_item->start_utc;
        pevent_item->next_end_utc = pevent_item->end_utc;
        pevent_item->status = DIVOOM_FEISHU_CALENDAR_STATUS_IDLE;
        goto divoom_feishu_calendar_formart_event_item_get_reminder_label;
    }
    
    switch(pevent_item->repeat_type)
    {
        case DIVOOM_FEISHU_CALENDAR_REPEAT_NONE:
            if(cur_time >= pevent_item->start_utc)
            {
                //已经启动了的事件，丢弃
                pevent_item->status = DIVOOM_FEISHU_CALENDAR_STATUS_TIME_OUT;
            }
            pevent_item->next_start_utc = pevent_item->start_utc;
            pevent_item->next_end_utc = pevent_item->end_utc;
                
            break;
            
        case DIVOOM_FEISHU_CALENDAR_REPEAT_DAY:
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
            
        case DIVOOM_FEISHU_CALENDAR_REPEAT_WEEK:            
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
            
        case DIVOOM_FEISHU_CALENDAR_REPEAT_MON:         
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
                    divoom_feishu_calendar_get_next_mday(&cur_date);
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
                divoom_feishu_calendar_get_next_mday(&cur_date);
                pevent_item->next_start_utc = mktime(&cur_date);
            }
            break;
            
        case DIVOOM_FEISHU_CALENDAR_REPEAT_YEAR:
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
        pevent_item->status = DIVOOM_FEISHU_CALENDAR_STATUS_TIME_OUT;
    }
    else if(pevent_item->range_end_utc == 0 && pevent_item->next_end_utc < cur_time)
    {
        pevent_item->status = DIVOOM_FEISHU_CALENDAR_STATUS_TIME_OUT;
    }
    else if(pevent_item->next_start_utc < cur_time)
    {
        pevent_item->status = DIVOOM_FEISHU_CALENDAR_STATUS_PASS;
    }
    else
    {
        pevent_item->status = DIVOOM_FEISHU_CALENDAR_STATUS_IDLE;
    }
    divoom_feishu_calendar_formart_event_item_get_reminder_label:
    pevent_item->notice_reminder_time = 0;
    if(pevent_item->status == DIVOOM_FEISHU_CALENDAR_STATUS_IDLE)
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
time_t divoom_feishu_calendar_get_utc_time(const char *datetime) {
    struct tm tm = {0};
    strptime(datetime, "%Y-%m-%dT%H:%M:%S", &tm);
    return divoom_timelib_timegm(&tm);
}

int divoom_feishu_calendar_check_event_update_time(divoom_cJSON *list)
{
    int i, j;
	divoom_cJSON *item, *data_obj, *update_obj;
    Pdivoom_feishu_calendar_event_item pevent_item;

    if(pdivoom_feishu_calendar_cal_handle->event_num != divoom_cJSON_GetArraySize(list))
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
                for(j = 0; j < pdivoom_feishu_calendar_cal_handle->event_num; j++)
                {
                    if(strcmp(pdivoom_feishu_calendar_cal_handle->pevent_list[j].event_id, data_obj->valuestring) == 0)
                    {
                        if(strcmp(pdivoom_feishu_calendar_cal_handle->pevent_list[j].update_time, update_obj->valuestring) != 0)
                        {
                            //时间不一样
                            return 1;
                        }
                        break;
                    }
                }
                if(j >= pdivoom_feishu_calendar_cal_handle->event_num)
                {
                    //新的
                    return 1;
                }
            }
        }
        
    }
    return 0;
}


void divoom_feishu_calendar_free_color_item(void *data)
{
    Pdivoom_feishu_calendar_color_item pitem = (Pdivoom_feishu_calendar_color_item)data;

    divoom_system_free(pitem->color_name);
    divoom_system_free(pitem->id);
    divoom_system_free(pitem);
}

void divoom_feishu_calendar_free_default_color_item(void *data)
{
    Pdivoom_feishu_calendar_default_color_item pitem = (Pdivoom_feishu_calendar_default_color_item)data;

    divoom_system_free(pitem->id);
    divoom_system_free(pitem);
}

int divoom_feishu_calendar_find_default_color_value_compare(void *item1, void *item2)
{
    char *color_name = (char *)item1;
    Pdivoom_feishu_calendar_default_color_item pcolor_item = (Pdivoom_feishu_calendar_default_color_item)item2;

    return strcmp(color_name, pcolor_item->id);
}


static time_t divoom_feishu_calendar_get_event_timestamp(divoom_cJSON *time_obj)
{
    divoom_cJSON *ts;

    if(time_obj == NULL)
    {
        return 0;
    }
    ts = divoom_cJSON_GetObjectItem(time_obj, "timestamp");
    if(ts && ts->valuestring)
    {
        return (time_t)atoi(ts->valuestring);
    }
    return 0;
}

static int divoom_feishu_calendar_get_reminder_seconds(divoom_cJSON *item)
{
    divoom_cJSON *reminders, *reminder_item;
    int minutes = 0;

    reminders = divoom_cJSON_GetObjectItem(item, "reminders");
    if(reminders && (reminders->type & 0xFF) == divoom_cJSON_Array && divoom_cJSON_GetArraySize(reminders) > 0)
    {
        reminder_item = divoom_cJSON_GetArrayItem(reminders, 0);
        minutes = divoom_cJSON_GetObjectInt(reminder_item, "minutes");
    }
    if(minutes <= 0)
    {
        return 0;
    }
    return minutes * 60;
}

void divoom_feishu_calendar_request_default_color_info(void)
{
}

void divoom_feishu_calendar_request_color_info(void)
{
}

int divoom_feishu_calendar_check_event_update_time(divoom_cJSON *list)
{
    return 1;
}

void divoom_feishu_calendar_request_url_info(void)
{
    int try_times = 0;
    char *cal_data = NULL, *url_str = NULL, *active_event_id = NULL, *active_event_update = NULL;
    char *calendar_id = NULL;
    time_t cur_time, start_time, end_time;
    struct tm start_date = {0}, end_date = {0};
    divoom_cJSON *root = NULL, *data_obj = NULL, *list = NULL, *item = NULL, *tmp_obj = NULL;
    Pdivoom_http_client_url_head_info phead_info;
    Pdivoom_feishu_calendar_event_item pevent_item;

    divoom_feishu_calendar_request_url_info_retry_lable:
    if(pdivoom_feishu_calendar_cal_handle == NULL || pdivoom_feishu_calendar_cal_handle->token == NULL)
    {
        return;
    }

    phead_info = divoom_system_malloc(sizeof(divoom_http_client_url_head_info));
    if(phead_info == NULL)
    {
        return;
    }

    cur_time = time(0);
    localtime_r(&cur_time, &start_date);
    start_time = cur_time - (start_date.tm_mday - 1) * 24 * 3600 - start_date.tm_hour * 3600 - start_date.tm_min * 60 - start_date.tm_sec;
    localtime_r(&start_time, &start_date);
    start_time -= (start_date.tm_wday * 24 * 3600);
    end_time = start_time + 42 * 24 * 3600;

    calendar_id = pdivoom_feishu_calendar_cal_handle->calendar_id;
    if(calendar_id == NULL || calendar_id[0] == 0)
    {
        calendar_id = "primary";
    }

    url_str = divoom_system_malloc(2048);
    sprintf(url_str,
        "https://open.feishu.cn/open-apis/calendar/v4/calendars/%s/events?start_time=%ld&end_time=%ld&page_size=500",
        calendar_id, (long)start_time, (long)end_time);

    divoom_http_client_head_set_bearer_auth(phead_info, pdivoom_feishu_calendar_cal_handle->token);
    divoom_system_free_lock(pdivoom_feishu_calendar_cal_handle->lock_handle);
    cal_data = divoom_http_client_url_request(url_str, phead_info, 1, NULL, NULL);
    divoom_system_get_lock(pdivoom_feishu_calendar_cal_handle->lock_handle);
    divoom_system_free(url_str);
    divoom_system_free(phead_info);

    if(cal_data == NULL)
    {
        return;
    }

    root = divoom_cJSON_Parse(cal_data);
    divoom_system_free(cal_data);
    if(root == NULL)
    {
        return;
    }

    if(divoom_cJSON_GetObjectInt(root, "code") != 0)
    {
        Pdivoom_net_command ret_data = NULL;

        divoom_cJSON_Delete(root);
        try_times++;
        if(try_times > 1)
        {
            return;
        }
        if(pdivoom_feishu_calendar_cal_handle->token)
        {
            divoom_system_free(pdivoom_feishu_calendar_cal_handle->token);
            pdivoom_feishu_calendar_cal_handle->token = NULL;
        }
        ret_data = divoom_net_app_send_packet_http(DIVOOM_NET_COMM_DEVICE_GET_FEISHU_CALENDAR_CONFIG, NULL, 0);
        if(ret_data)
        {
            Pdivoom_device_google_calendar_config_respone_info pinfo = (Pdivoom_device_google_calendar_config_respone_info)ret_data->cmd_data;

            if(pinfo && strlen(pinfo->token))
            {
                pdivoom_feishu_calendar_cal_handle->token = divoom_app_com_copy_string(pinfo->token, -1);
                if(strlen(pinfo->calendar_id))
                {
                    divoom_feishu_calendar_update_extra(pinfo->calendar_id, NULL);
                }
            }
            divoom_net_app_free_packet(ret_data);
        }
        if(pdivoom_feishu_calendar_cal_handle->token == NULL)
        {
            return;
        }
        goto divoom_feishu_calendar_request_url_info_retry_lable;
    }

    data_obj = divoom_cJSON_GetObjectItem(root, "data");
    list = data_obj ? divoom_cJSON_GetObjectItem(data_obj, "items") : NULL;
    if(list == NULL)
    {
        divoom_cJSON_Delete(root);
        return;
    }

    if(pdivoom_feishu_calendar_cal_handle->prunning_event)
    {
        active_event_id = divoom_app_com_copy_string(pdivoom_feishu_calendar_cal_handle->prunning_event->event_id, -1);
        active_event_update = divoom_app_com_copy_string(pdivoom_feishu_calendar_cal_handle->prunning_event->update_time, -1);
    }
    if(pdivoom_feishu_calendar_cal_handle->pevent_list)
    {
        divoom_feishu_calendar_free_all_event(pdivoom_feishu_calendar_cal_handle->pevent_list, pdivoom_feishu_calendar_cal_handle->event_num);
        pdivoom_feishu_calendar_cal_handle->pevent_list = NULL;
        pdivoom_feishu_calendar_cal_handle->prunning_event = NULL;
    }

    pdivoom_feishu_calendar_cal_handle->event_num = divoom_cJSON_GetArraySize(list);
    if(pdivoom_feishu_calendar_cal_handle->event_num > 0)
    {
        pdivoom_feishu_calendar_cal_handle->pevent_list = divoom_system_malloc(sizeof(divoom_feishu_calendar_event_item) * pdivoom_feishu_calendar_cal_handle->event_num);
    }

    for(int i = 0; i < pdivoom_feishu_calendar_cal_handle->event_num; i++)
    {
        item = divoom_cJSON_GetArrayItem(list, i);
        if(item == NULL)
        {
            break;
        }

        pevent_item = &pdivoom_feishu_calendar_cal_handle->pevent_list[i];
        pevent_item->bg_color = 0x0000FF00;
        pevent_item->repeat_type = DIVOOM_FEISHU_CALENDAR_REPEAT_NONE;
        pevent_item->notice_flag = 1;
        pevent_item->reminder_time = divoom_feishu_calendar_get_reminder_seconds(item);
        if(pevent_item->reminder_time == 0)
        {
            pevent_item->notice_flag = 0;
        }

        tmp_obj = divoom_cJSON_GetObjectItem(item, "summary");
        if(tmp_obj && tmp_obj->valuestring)
        {
            pevent_item->title = divoom_app_com_copy_string(tmp_obj->valuestring, -1);
        }

        tmp_obj = divoom_cJSON_GetObjectItem(item, "event_id");
        if(tmp_obj && tmp_obj->valuestring)
        {
            pevent_item->event_id = divoom_app_com_copy_string(tmp_obj->valuestring, -1);
        }

        tmp_obj = divoom_cJSON_GetObjectItem(item, "create_time");
        if(tmp_obj && tmp_obj->valuestring)
        {
            pevent_item->update_time = divoom_app_com_copy_string(tmp_obj->valuestring, -1);
        }

        if(active_event_id && pevent_item->event_id && strcmp(active_event_id, pevent_item->event_id) == 0)
        {
            if(active_event_update && pevent_item->update_time && strcmp(active_event_update, pevent_item->update_time))
            {
                divoom_feishu_calendar_send_event_notice_message(NULL, DIVOOM_FEISHU_CALENDAR_STATUS_IDLE);
            }
            else
            {
                pdivoom_feishu_calendar_cal_handle->prunning_event = pevent_item;
            }
        }

        tmp_obj = divoom_cJSON_GetObjectItem(item, "description");
        if(tmp_obj && tmp_obj->valuestring)
        {
            pevent_item->desc = divoom_app_com_copy_string(tmp_obj->valuestring, -1);
        }

        tmp_obj = divoom_cJSON_GetObjectItem(item, "start_time");
        pevent_item->start_utc = divoom_feishu_calendar_get_event_timestamp(tmp_obj);
        tmp_obj = divoom_cJSON_GetObjectItem(item, "end_time");
        pevent_item->end_utc = divoom_feishu_calendar_get_event_timestamp(tmp_obj);
        pevent_item->all_day_flag = 0;
        divoom_feishu_calendar_formart_event_item(pevent_item);
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
    pdivoom_feishu_calendar_cal_handle->update_flag = 1;
}

void divoom_feishu_calendar_send_event_notice(void *data)
{
	divoom_sys_msg_info model_msg = {0};

    pdivoom_feishu_calendar_cal_handle->prunning_event = data;

    if(data)
    {
        data = divoom_system_malloc(sizeof(int));
    }
	model_msg.src_id = DIVOOM_MODEL_NET;
    model_msg.msg_type = 1;
	model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
	model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_FEISHU_CALENDAR_CTRL;
	model_msg.data = data;
	divoom_sys_model_crl_msg_push(&model_msg);
}


/*********************************************************************
Function:         divoom_feishu_calendar_user_cancle_event
Description:      按键取消
Input:            
Return:           
Author:           albert
**********************************************************************/

void divoom_feishu_calendar_user_cancle_event(void)
{
    divoom_print("aj test!\n");
    divoom_system_get_lock(pdivoom_feishu_calendar_cal_handle->lock_handle);
    if(pdivoom_feishu_calendar_cal_handle->prunning_event)
    {
        divoom_print("aj test!\n");
        pdivoom_feishu_calendar_cal_handle->prunning_event->status = DIVOOM_FEISHU_CALENDAR_STATUS_PASS;
        pdivoom_feishu_calendar_cal_handle->prunning_event = NULL;
    	divoom_sys_msg_info model_msg = {0};

    	model_msg.src_id = DIVOOM_MODEL_NET;
        model_msg.msg_type = 1;
    	model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
    	model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_FEISHU_CALENDAR_CTRL;
    	model_msg.data = NULL;
    	divoom_sys_model_crl_msg_push(&model_msg);
    }
    divoom_system_free_lock(pdivoom_feishu_calendar_cal_handle->lock_handle);
}

/*********************************************************************
Function:         divoom_feishu_calendar_get_active_lvgl_event_info
Description:      得到LVGL显示的信息
Input:            data
Return:           
Author:           albert
**********************************************************************/

int divoom_feishu_calendar_get_active_lvgl_event_info(void *data)
{
    int ret = 1;
    Pdivoom_lvgl_calendar_event_info pevent_info = (Pdivoom_lvgl_calendar_event_info)data;
    time_t cur_time; 
    struct tm cur_tm;
    
    
    if(divoom_system_try_lock(pdivoom_feishu_calendar_cal_handle->lock_handle))
    {
        return 0;
    }
    if(pdivoom_feishu_calendar_cal_handle->prunning_event)
    {
        pevent_info->bg_color[0] = pdivoom_feishu_calendar_cal_handle->prunning_event->bg_color >> 16;
        pevent_info->bg_color[1] = (pdivoom_feishu_calendar_cal_handle->prunning_event->bg_color >> 8) &0xFF;
        pevent_info->bg_color[2] = pdivoom_feishu_calendar_cal_handle->prunning_event->bg_color & 0xFF;
        memset(pevent_info->fr_color, 0, 3);
        cur_time = pdivoom_feishu_calendar_cal_handle->prunning_event->start_utc;
        localtime_r(&cur_time, &cur_tm);
        pevent_info->start_time =  cur_tm.tm_hour *60 + cur_tm.tm_min;
        cur_time = pdivoom_feishu_calendar_cal_handle->prunning_event->end_utc;
        localtime_r(&cur_time, &cur_tm);
        pevent_info->end_time =  cur_tm.tm_hour *60 + cur_tm.tm_min;
        if(pevent_info->end_time == 0)
        {
            pevent_info->end_time = 23*60 +59;
        }
        if(pdivoom_feishu_calendar_cal_handle->prunning_event->title == NULL)
        {
            pevent_info->title = divoom_app_com_copy_string("  ", -1);
        }
        else
        {
            pevent_info->title = divoom_app_com_copy_string( pdivoom_feishu_calendar_cal_handle->prunning_event->title, -1);
        }
        pevent_info->msg_info = divoom_app_com_copy_string(pdivoom_feishu_calendar_cal_handle->prunning_event->desc, -1);
    }
    else
    {
        ret = 0;
    }
    divoom_system_free_lock(pdivoom_feishu_calendar_cal_handle->lock_handle);
    return ret;

}

int divoom_feishu_calendar_add_popup_notice_message_callback(void *para, int result)
{
    Pdivoom_feishu_calendar_event_item pevent_item = (Pdivoom_feishu_calendar_event_item)para;
    for(int i= 0; i < pdivoom_feishu_calendar_cal_handle->event_num; i++)
    {
        if(pevent_item == &pdivoom_feishu_calendar_cal_handle->pevent_list[i])
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

void divoom_feishu_calendar_add_popup_notice_message(Pdivoom_feishu_calendar_event_item pevent_item)
{    
    int bg_value, font_value;
    struct tm cur_tm, end_tm;
    time_t cur_time =pevent_item->start_utc;
    
    localtime_r(&cur_time, &cur_tm);
    cur_time = pevent_item->end_utc;
    localtime_r(&cur_time, &end_tm);
    
    bg_value = pevent_item->bg_color; 
    void *lvgl_obj = divoom_system_msg_notice_add(720, 216, divoom_feishu_calendar_add_popup_notice_message_callback, pevent_item);
    pevent_item->lvgl_obj = lvgl_obj;
    
    
    divoom_get_lvgl_lock();
	lv_obj_set_style_bg_color(lvgl_obj, lv_color_hex(bg_value), 0);

    void *image_obj = lv_img_create(lvgl_obj);
	lv_obj_set_size(image_obj, 125, 125);
	lv_obj_set_pos(image_obj, 48, 46);
	divoom_menu_control_lv_img_set_src(image_obj, "82/feishu_calendar_log.png");
    #if 0
    void *lable_obj = lv_label_create(image_obj);
	lv_obj_set_size(lable_obj, 62, 62);
    lv_obj_center(lable_obj);
	lv_obj_set_style_text_color(lable_obj, lv_color_hex(0x1A73E8), 0);
	lv_obj_set_style_bg_color(lable_obj, lv_color_hex(bg_value), 0);
	lv_obj_set_style_text_font(lable_obj, divoom_menu_control_get_font_info(44, 0), 0);
	lv_obj_set_style_text_align(lable_obj,LV_TEXT_ALIGN_CENTER, 0);    
	lv_label_set_text_fmt(lable_obj, "%d", cur_tm.tm_mday);
    #endif
    
    font_value = 0; 
    void *lable_obj = lv_label_create(lvgl_obj);
	lv_obj_set_pos(lable_obj, 208, 52);
	lv_obj_set_size(lable_obj, 284, 35);
	lv_obj_set_style_text_color(lable_obj, lv_color_hex(font_value), 0);
	lv_obj_set_style_bg_color(lable_obj, lv_color_hex(bg_value), 0);
	lv_obj_set_style_text_font(lable_obj, divoom_menu_control_get_font_info(28, 0), 0);
	lv_obj_set_style_text_align(lable_obj,LV_TEXT_ALIGN_LEFT, 0);    
	lv_label_set_text_fmt(lable_obj, "%d:%02d-%d:%02d", cur_tm.tm_hour, cur_tm.tm_min,end_tm.tm_hour, end_tm.tm_min);

    
    
    lable_obj = lv_label_create(lvgl_obj);
	lv_obj_set_size(lable_obj, 500, 54);
	lv_obj_set_pos(lable_obj, 206, 103);
	lv_obj_set_style_text_color(lable_obj, lv_color_hex(font_value), 0);
	lv_obj_set_style_bg_color(lable_obj, lv_color_hex(bg_value), 0);
	lv_obj_set_style_text_font(lable_obj, divoom_menu_control_get_font_info(44, 0), 0);
	lv_obj_set_style_text_align(lable_obj,LV_TEXT_ALIGN_LEFT, 0);    
	lv_label_set_text_fmt(lable_obj, "%s", pevent_item->title?pevent_item->title:" ");
    divoom_free_lvgl_lock();
    
    
}


/*********************************************************************
Function:         divoom_feishu_calendar_send_event_notice_message
Description:      通知系统事件触发
Input:            data
Return:           
Author:           albert
**********************************************************************/

void divoom_feishu_calendar_send_event_notice_message(void *data, int status)
{    
    Pdivoom_feishu_calendar_event_item pevent_item = (Pdivoom_feishu_calendar_event_item)data;
    

    if(pevent_item)
    {
        divoom_print("aj test: %s: %d\n", pevent_item->title, status);
    }
    if(status == DIVOOM_FEISHU_CALENDAR_STATUS_NOTICE)
    {
        //这里增加一个系统消息
        if(pdivoom_feishu_calendar_cal_handle->pop_flag)
        {
            divoom_feishu_calendar_add_popup_notice_message(data);
        }
    }
    else if(status == DIVOOM_FEISHU_CALENDAR_STATUS_ACTIVE)
    {
        if(pdivoom_feishu_calendar_cal_handle->event_flag)
        {
            divoom_feishu_calendar_send_event_notice(data);
            if(pevent_item->lvgl_obj)
            {
                divoom_system_msg_notice_clear_one(pevent_item->lvgl_obj, 1);
                pevent_item->lvgl_obj = NULL;
            }
        }
    }
    else if(status == DIVOOM_FEISHU_CALENDAR_STATUS_IDLE)
    {
        if(pdivoom_feishu_calendar_cal_handle->event_flag)
        {
            divoom_feishu_calendar_send_event_notice(NULL);
        }
    }
    
}


void divoom_feishu_calendar_add_lvgl_event(void **queue_item, int pos, Pdivoom_lvgl_calendar_event_info plvgl_event_item)
{
    if(queue_item[pos] == NULL)
    {
        queue_item[pos] = divoom_queue_init(100, __func__, __LINE__);
    }
    divoom_queue_push(queue_item[pos], plvgl_event_item);
    //divoom_print("add : %d, %d, %s\n", pos, divoom_queue_items(queue_item[pos]), plvgl_event_item->title);
}

Pdivoom_lvgl_calendar_event_info divoom_feishu_calendar_create_lvgl_event(time_t start_utc, time_t end_utc, char *title, char *desc, uint32 bg_color)
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
bool divoom_feishu_calendar_is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

/**
 * 计算指定年月的天数
 * @param year 年份
 * @param month 月份 (1-12)
 * @return 该月的天数，如果月份无效返回-1
 */
int divoom_feishu_calendar_get_days_in_month(int year, int month) {
    // 检查月份是否有效
    if (month < 1 || month > 12) {
        return -1;
    }
    
    // 每月天数数组（非闰年）
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    // 如果是2月，需要判断是否为闰年
    if (month == 2) {
        return divoom_feishu_calendar_is_leap_year(year) ? 29 : 28;
    }
    
    return days_in_month[month - 1];
}

int divoom_feishu_calendar_check_date_ok(struct tm *cur_date)
{
    return cur_date->tm_mday <= divoom_feishu_calendar_get_days_in_month(cur_date->tm_year + 1900 , cur_date->tm_mon+1);
}

void divoom_feishu_calendar_load_calenday_all_from_event(void **queue_item, Pdivoom_feishu_calendar_event_item pevent_item, time_t start_utc, time_t end_utc)
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
        case DIVOOM_FEISHU_CALENDAR_REPEAT_NONE:
 //           divoom_print("aj test!: all_day_flag:%d\n", pevent_item->all_day_flag);
            if(pevent_item->all_day_flag)
            {
                active_time = pevent_item->start_utc;
                while(active_time >= start_utc && active_time < end_utc)
                {
                    
                    plvgl_event_item = divoom_feishu_calendar_create_lvgl_event(active_time, active_time + 24*3600, pevent_item->title, pevent_item->desc, pevent_item->bg_color);
                    divoom_feishu_calendar_add_lvgl_event(queue_item, (active_time - start_utc)/(24*3600), plvgl_event_item);
                    active_time += 24*3600;
                }
            }
            else
            {
                if(pevent_item->start_utc >= start_utc && pevent_item->start_utc < end_utc)
                {
                    
                    plvgl_event_item = divoom_feishu_calendar_create_lvgl_event(pevent_item->start_utc, pevent_item->end_utc, pevent_item->title, pevent_item->desc, pevent_item->bg_color);
                    divoom_feishu_calendar_add_lvgl_event(queue_item, (pevent_item->start_utc - start_utc)/(24*3600), plvgl_event_item);
                }
            }
                
            break;
            
        case DIVOOM_FEISHU_CALENDAR_REPEAT_DAY:
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
                plvgl_event_item = divoom_feishu_calendar_create_lvgl_event(active_time, active_time + (pevent_item->end_utc - pevent_item->start_utc), pevent_item->title, pevent_item->desc, pevent_item->bg_color);            
                divoom_feishu_calendar_add_lvgl_event(queue_item, day_index, plvgl_event_item);                
                active_time += 24 *3600;
            }
            break;
            
        case DIVOOM_FEISHU_CALENDAR_REPEAT_WEEK:  
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
                            plvgl_event_item = divoom_feishu_calendar_create_lvgl_event(active_time, active_time + (pevent_item->end_utc - pevent_item->start_utc), pevent_item->title, pevent_item->desc, pevent_item->bg_color);                                    
                            divoom_feishu_calendar_add_lvgl_event(queue_item, i*7+j, plvgl_event_item);
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
                        plvgl_event_item = divoom_feishu_calendar_create_lvgl_event(active_time, active_time + (pevent_item->end_utc - pevent_item->start_utc), pevent_item->title, pevent_item->desc, pevent_item->bg_color);                                    
                        divoom_feishu_calendar_add_lvgl_event(queue_item, day_index, plvgl_event_item);
                    }
                    active_time += 24*3600;
                }
            }        
            break;
            
        case DIVOOM_FEISHU_CALENDAR_REPEAT_MON: 
            localtime_r(&start_utc, &cur_date);
            localtime_r(&pevent_item->start_utc, &start_date);
            start_date.tm_year = cur_date.tm_year;
            start_date.tm_mon = cur_date.tm_mon;
            for(int i = 0; i < 3; i++)
            {
                if(divoom_feishu_calendar_check_date_ok(&start_date))
                {
                    active_time = mktime(&start_date);
                    if(active_time >= pevent_item->start_utc && active_time >= start_utc && active_time < end_utc)
                    {
                        plvgl_event_item = divoom_feishu_calendar_create_lvgl_event(active_time, active_time + (pevent_item->end_utc - pevent_item->start_utc), pevent_item->title, pevent_item->desc, pevent_item->bg_color);                                    
                        divoom_feishu_calendar_add_lvgl_event(queue_item, (active_time - start_utc)/(24*3600), plvgl_event_item);                    
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
            
        case DIVOOM_FEISHU_CALENDAR_REPEAT_YEAR:
            localtime_r(&start_utc, &cur_date);
            localtime_r(&end_utc, &start_date);
            localtime_r(&pevent_item->start_utc, &tmp_date);
            tmp_date.tm_year = cur_date.tm_year - 1;
            for(int i = 0; i < 3; i++)
            {
                if(divoom_feishu_calendar_check_date_ok(&start_date))
                {
                    active_time = mktime(&tmp_date);
                    if(active_time >= start_utc && active_time < end_utc)
                    {
                        plvgl_event_item = divoom_feishu_calendar_create_lvgl_event(active_time, active_time + (pevent_item->end_utc - pevent_item->start_utc), pevent_item->title, pevent_item->desc, pevent_item->bg_color);                                    
                        divoom_feishu_calendar_add_lvgl_event(queue_item, (active_time - start_utc)/(24*3600), plvgl_event_item); 
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
Function:         divoom_feishu_calendar_load_calenday_current_month
Description:      加载当前月的所有事件
Input:            data
Return:           
Author:           albert
**********************************************************************/

int divoom_feishu_calendar_load_calenday_current_month(void **queue_item)
{
    #if 1
    time_t cur_time, start_time, end_time;
    struct tm cur_date = {0};
    Pdivoom_feishu_calendar_event_item pevent_item;

    if(pdivoom_feishu_calendar_cal_handle == NULL)
    {
        return 0;
    }
    
    divoom_system_put_sem(pdivoom_feishu_calendar_cal_handle->run_sem);
    if(divoom_system_try_lock(pdivoom_feishu_calendar_cal_handle->lock_handle))
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

    divoom_print("event num: %d\n", pdivoom_feishu_calendar_cal_handle->event_num);
    for(int j = 0; j < pdivoom_feishu_calendar_cal_handle->event_num; j ++)
    {
        divoom_feishu_calendar_load_calenday_all_from_event(queue_item, &pdivoom_feishu_calendar_cal_handle->pevent_list[j], start_time, end_time);
    }
    divoom_system_free_lock(pdivoom_feishu_calendar_cal_handle->lock_handle);
    if(pdivoom_feishu_calendar_cal_handle->event_num > 1)
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

int divoom_feishu_calendar_check_run_flag(void)
{
    if(pdivoom_feishu_calendar_cal_handle->token && strlen(pdivoom_feishu_calendar_cal_handle->token))
    {
        if(pdivoom_feishu_calendar_cal_handle->pop_flag || pdivoom_feishu_calendar_cal_handle->event_flag || pdivoom_feishu_calendar_cal_handle->menu_flag)
        {
            return 1;
        }
    }
    return 0;
}

void *divoom_feishu_calendar_main_task(void *para)
{
    int j, cur_mon = -1;
    time_t cur_time;
    struct tm cur_date = {0};
    Pdivoom_feishu_calendar_event_item pevent_item;
    
    while(1)
    {
        cur_time = time(0);
        if(divoom_main_get_server_con_status())
        {
            
            if(divoom_feishu_calendar_check_run_flag())
            {
                
                localtime_r(&cur_time, &cur_date);
                if(cur_date.tm_mon != cur_mon)
                {
                    cur_mon = cur_date.tm_mon;
                    pdivoom_feishu_calendar_cal_handle->event_request_time = 0;
                }
                if(pdivoom_feishu_calendar_cal_handle->event_request_time + DIVOOM_FEISHU_CALENDAR_REQUEST_EVENT_LIST_TIME < cur_time)
                {              
                    divoom_system_get_lock(pdivoom_feishu_calendar_cal_handle->lock_handle);
                    divoom_feishu_calendar_request_default_color_info();
                    divoom_feishu_calendar_request_url_info();
                    divoom_feishu_calendar_request_color_info();
                    
                    divoom_system_free_lock(pdivoom_feishu_calendar_cal_handle->lock_handle);
                    pdivoom_feishu_calendar_cal_handle->event_request_time = cur_time;
                }
            }
            else
            {
                divoom_system_get_sem(pdivoom_feishu_calendar_cal_handle->run_sem);
                continue;
            }
        }
        if(pdivoom_feishu_calendar_cal_handle->event_num)
        {
            divoom_system_get_lock(pdivoom_feishu_calendar_cal_handle->lock_handle);
            for(j = 0; j < pdivoom_feishu_calendar_cal_handle->event_num; j ++)
            {
                pevent_item = &pdivoom_feishu_calendar_cal_handle->pevent_list[j];
                switch(pevent_item->status)
                {
                        
                    case DIVOOM_FEISHU_CALENDAR_STATUS_IDLE:
                    case DIVOOM_FEISHU_CALENDAR_STATUS_NOTICE:
                        if(pevent_item->notice_flag && pevent_item->notice_reminder_time && pevent_item->notice_reminder_time <= cur_time)
                        {
                            //抛出提醒事件
                            pevent_item->status = DIVOOM_FEISHU_CALENDAR_STATUS_NOTICE;
                            pevent_item->notice_reminder_time = 0;
                            divoom_feishu_calendar_send_event_notice_message(pevent_item, DIVOOM_FEISHU_CALENDAR_STATUS_NOTICE);
                        }
                        else if(pevent_item->next_start_utc < cur_time && pevent_item->next_end_utc > cur_time)
                        {
                            //事件激活
                            if(pevent_item->all_day_flag)
                            {
                                pevent_item->status = DIVOOM_FEISHU_CALENDAR_STATUS_PASS;
                            }
                            else
                            {
                                pevent_item->status = DIVOOM_FEISHU_CALENDAR_STATUS_ACTIVE;
                                divoom_feishu_calendar_send_event_notice_message(pevent_item, DIVOOM_FEISHU_CALENDAR_STATUS_ACTIVE);
                            }
                        }
                        else if(pevent_item->next_end_utc < cur_time)
                        {
                            pevent_item->status = DIVOOM_FEISHU_CALENDAR_STATUS_PASS;
                        }
                        break;
                        
                    case DIVOOM_FEISHU_CALENDAR_STATUS_PASS:
                    case DIVOOM_FEISHU_CALENDAR_STATUS_ACTIVE:
                        if(pevent_item->next_end_utc < cur_time)
                        {
                            //将这个事件弹出,通知事件结束
                            int cur_status = pevent_item->status;
                            divoom_print("event : %s, %d, %d\n", pevent_item->title,pevent_item->next_end_utc, cur_time);
                            pevent_item->status = DIVOOM_FEISHU_CALENDAR_STATUS_IDLE;
                            divoom_feishu_calendar_formart_event_item(pevent_item);
                            if(cur_status == DIVOOM_FEISHU_CALENDAR_STATUS_ACTIVE)
                            {
                                divoom_feishu_calendar_send_event_notice_message(pevent_item, DIVOOM_FEISHU_CALENDAR_STATUS_IDLE);
                            }
                        }
                        break;
                }
            }
            divoom_system_free_lock(pdivoom_feishu_calendar_cal_handle->lock_handle);
        }
        divoom_system_delay_ms(1000);
        
    }
}

/*********************************************************************
Function:         divoom_feishu_calendar_update_menu_flag
Description:      进入与退出菜单
Input:            menu_flag
Return:           
Author:           albert
**********************************************************************/

void divoom_feishu_calendar_update_menu_flag(uint8 menu_flag)
{
    if(pdivoom_feishu_calendar_cal_handle == NULL)
    {
        return ;
    }
    pdivoom_feishu_calendar_cal_handle->menu_flag = menu_flag;
    divoom_system_put_sem(pdivoom_feishu_calendar_cal_handle->run_sem);
}

/*********************************************************************
Function:         divoom_feishu_calendar_update_switch
Description:      更新开关
Input:            purl_head_info
                　head_num
                  clock_id
Return:           
Author:           albert
**********************************************************************/

void divoom_feishu_calendar_update_switch(int pop_flag, int event_flag)
{
    if(pdivoom_feishu_calendar_cal_handle == NULL)
    {
        return ;
    }
    divoom_system_get_lock(pdivoom_feishu_calendar_cal_handle->lock_handle);
    pdivoom_feishu_calendar_cal_handle->pop_flag = pop_flag;
    if(event_flag == 0 && pdivoom_feishu_calendar_cal_handle->event_flag == 1)
    {
        divoom_feishu_calendar_send_event_notice_message(NULL, DIVOOM_FEISHU_CALENDAR_STATUS_IDLE);
    }
    pdivoom_feishu_calendar_cal_handle->event_flag = event_flag;
    divoom_system_free_lock(pdivoom_feishu_calendar_cal_handle->lock_handle);
    divoom_feishu_calendar_save_config();    
    divoom_system_put_sem(pdivoom_feishu_calendar_cal_handle->run_sem);
}



/*********************************************************************
Function:         divoom_feishu_calendar_get_update_flag
Description:      得到更新标志
Input:            
Return:           NONE
Author:           albert
**********************************************************************/

int divoom_feishu_calendar_get_update_flag(void)
{
    int ret;

    if(pdivoom_feishu_calendar_cal_handle == NULL)
    {
        return 0;
    }
    ret = pdivoom_feishu_calendar_cal_handle->update_flag;

    pdivoom_feishu_calendar_cal_handle->update_flag = 0;
    return ret;
}

/*********************************************************************
Function:         divoom_feishu_calendar_check_login
Description:      是否有ＴＯＫＥＮ
Input:            
Return:           
Author:           albert
**********************************************************************/

int divoom_feishu_calendar_check_login(void)
{
    if(pdivoom_feishu_calendar_cal_handle)
    {
        return pdivoom_feishu_calendar_cal_handle->token && strlen(pdivoom_feishu_calendar_cal_handle->token);
    }
    return 0;
    
}

/*********************************************************************
Function:         divoom_feishu_calendar_update
Description:      更新ＴＯＫＥＮ
Input:            purl_head_info
                　head_num
                  clock_id
Return:           
Author:           albert
**********************************************************************/

void divoom_feishu_calendar_update_token(int pop_flag, int event_flag, char *token)
{
    #if 1
    if(strlen(token) == 0)
    {
        divoom_print("token is NULL!\n");
        return;
    }
    if(pdivoom_feishu_calendar_cal_handle)
    {
        divoom_system_get_lock(pdivoom_feishu_calendar_cal_handle->lock_handle);
        if(strlen(token) &&(pdivoom_feishu_calendar_cal_handle->token == NULL  || strcmp(pdivoom_feishu_calendar_cal_handle->token, token)))
        {
            //如果TOKEN发生变化，将重新获取一次
            if(pdivoom_feishu_calendar_cal_handle->token)
            {
                divoom_system_free(pdivoom_feishu_calendar_cal_handle->token);
                pdivoom_feishu_calendar_cal_handle->token = NULL;
            }
            pdivoom_feishu_calendar_cal_handle->token = divoom_app_com_copy_string(token, -1);
            pdivoom_feishu_calendar_cal_handle->event_request_time = 0;
        }
        if(pop_flag != -1)
        {
            pdivoom_feishu_calendar_cal_handle->pop_flag = pop_flag;
        }
        if(event_flag != -1)
        {
            if(event_flag == 0 && pdivoom_feishu_calendar_cal_handle->event_flag == 1)
            {
                divoom_feishu_calendar_send_event_notice_message(NULL, DIVOOM_FEISHU_CALENDAR_STATUS_IDLE);
            }
            pdivoom_feishu_calendar_cal_handle->event_flag = event_flag;
        }
        divoom_system_free_lock(pdivoom_feishu_calendar_cal_handle->lock_handle);
        divoom_feishu_calendar_save_config();
        divoom_system_put_sem(pdivoom_feishu_calendar_cal_handle->run_sem);
    }
    #endif
}


void divoom_feishu_calendar_init(void)
{
    pdivoom_feishu_calendar_cal_handle = divoom_system_malloc(sizeof(divoom_feishu_calendar_cal_ctrl));
    divoom_feishu_calendar_load_config();
    pdivoom_feishu_calendar_cal_handle->event_num = 0;
    pdivoom_feishu_calendar_cal_handle->run_sem = divoom_system_create_sem(1, 0);
    pdivoom_feishu_calendar_cal_handle->color_queue = divoom_queue_init(100, __func__, __LINE__);
    pdivoom_feishu_calendar_cal_handle->default_color_queue = divoom_queue_init(100, __func__, __LINE__);
    pdivoom_feishu_calendar_cal_handle->lock_handle = divoom_system_create_lock();
    divoom_system_create_task(divoom_feishu_calendar_main_task, "feishu_cal", pdivoom_feishu_calendar_cal_handle, DIVOOM_TASK_STACK_FEISHU_CAL);
    
}
#endif



#include "divoom.h"
#include "divoom_menu_control.h"
#include "divoom_calendar_today_watch_layout.h"

#if 1
#define DIVOOM_GOOGLE_CALENDAR_REQUEST_CAL_LIST "https://www.googleapis.com/calendar/v3/users/me/calendarList?maxResults=2500&minAccessRole=owner"
#define DIVOOM_GOOGLE_CALENDAR_REQUEST_OTHER_CAL_LIST "https://www.googleapis.com/calendar/v3/users/me/calendarList?maxResults=2500&syncToken="
#define DIVOOM_GOOGLE_CALENDAR_REQUEST_EVENT_UPDATE_TIME "https://www.googleapis.com/calendar/v3/calendars/calendarId/events?calendarId=%s&maxResults=2500&timeMin=%04d-%02d-%02dT00:00:00Z&timeMax=%04d-%02d-%02dT00:00:00Z&orderBy=startTime&singleEvents=true&fields=updated"
#define DIVOOM_GOOGLE_CALENDAR_REQUEST_EVENT_LIST "https://www.googleapis.com/calendar/v3/calendars/calendarId/events?calendarId=%s&maxResults=2500&timeMin=%04d-%02d-%02dT00:00:00Z&timeMax=%04d-%02d-%02dT00:00:00Z&orderBy=startTime&singleEvents=true&fields=updated,defaultReminders,items(id,summary,updated,description,colorId,start,end,reminders)"
#define DIVOOM_GOOGLE_CALENDAR_REQUEST_COLOR_LIST "https://www.googleapis.com/calendar/v3/colors"
#define DIVOOM_GOOGLE_CALENDAR_REQUEST_CAL_LIST_TIME 300 
#define DIVOOM_GOOGLE_CALENDAR_REQUEST_COLOR_LIST_TIME 600 
#define DIVOOM_GOOGLE_CALENDAR_REQUEST_EVENT_LIST_TIME 30 
#define DIVOOM_GOOGLE_CALENDAR_EVENT_NOTICE_TIME 120 

#define DIVOOM_GOOGLE_CALENDAR_POPUP_W        (DIVOOM_SCREEN_WIDTH - 32)
#define DIVOOM_GOOGLE_CALENDAR_POPUP_H        96
#define DIVOOM_GOOGLE_CALENDAR_POPUP_RADIUS   20
#define DIVOOM_GOOGLE_CALENDAR_POPUP_ICON_SZ  56
#define DIVOOM_GOOGLE_CALENDAR_POPUP_PAD      12
#define DIVOOM_GOOGLE_CALENDAR_POPUP_ICON_X   (DIVOOM_GOOGLE_CALENDAR_POPUP_RADIUS * 2)
#define DIVOOM_GOOGLE_CALENDAR_POPUP_TEXT_GAP 12
#define DIVOOM_GOOGLE_CALENDAR_POPUP_TIME_H   22
#define DIVOOM_GOOGLE_CALENDAR_POPUP_TITLE_H  36
#define DIVOOM_GOOGLE_CALENDAR_POPUP_TEXT_ROW_GAP 4

//#define DIVOOM_GOOGLE_CALENDAR_TEST

enum{
    DIVOOM_GOOGLE_CALENDAR_STATUS_IDLE,
    DIVOOM_GOOGLE_CALENDAR_STATUS_NOTICE,
    DIVOOM_GOOGLE_CALENDAR_STATUS_ACTIVE,
    DIVOOM_GOOGLE_CALENDAR_STATUS_PASS,
};

typedef struct _divoom_google_calendar_color_item
{
    int type; // 0:日历； 1：事件 
    char color_id[16];
    uint8 bg_color[3];
    uint8 fr_color[3];
}divoom_google_calendar_color_item, *Pdivoom_google_calendar_color_item;

//GET https://gmail.googleapis.com/gmail/v1/users/me/threads/1983803b637b7c8f?format=metadata&metadataHeaders=Subject&metadataHeaders=From&metadataHeaders=Date
//GET https://gmail.googleapis.com/gmail/v1/users/{userId}/threads?q=is:unread
typedef struct _divoom_google_calendar_cal_ctrl
{
    uint8   menu_flag;
    uint8   pop_flag;    // 是否有弹窗提醒
    uint8   event_flag;    //是否事件通知功能
    uint8   mail_pop_flag;    // 是否有邮件弹窗提醒
    uint8   watch_refresh_flag; //表盘刷新请求，通知全关时也要能后台拉一次
    uint8   update_flag;    //是否有变化，这个读清
    uint8   net_status;     //网络状态
    int     clock_id;       //表盘ＩＤ
    int     color_request_time;   //更新时间
    int     calendar_request_time;   //更新时间
    int     event_request_time;   //更新时间
    int     head_num;        //请求头数目
    void    *phandle;
    char    *nextSyncTokee;
    char    color_update_time[32];
    char  *google_token;
    uint32 token_generation;
    uint32 token_refresh_tick;
    uint8 token_refresh_pending;
    char  *lock_handle;
    void  *gmail_queue;
    void *run_sem;
    Pdivoom_google_calendar_event_item prunning_event;      //正在激活的日历
    int calendar_num;
    Pdivoom_google_calendar_cal_item    calendar_list;   //日历队列
    void    *color_list;   //日历队列
    void    *event_msg_queue;   //弹窗提醒句柄
    void    *notice_lvgl_obj;
}divoom_google_calendar_cal_ctrl, *Pdivoom_google_calendar_cal_ctrl;


void divoom_google_calendar_load_config(void);

void divoom_google_calendar_remove_cal_item(char *id_name);

void divoom_google_calendar_save_cal_item(Pdivoom_google_calendar_cal_item pcal_item);
void divoom_google_calendar_save_config(void);
int divoom_google_calendar_get_color_value(int type, char *color_id, uint8 *bg_color, uint8 *fr_color);
void divoom_google_calendar_send_event_notice_message(void *data, int status);
int divoom_google_calendar_remove_popup_notice_message_compare(void *item1, void *item2);


Pdivoom_google_calendar_cal_ctrl pdivoom_google_calendar_cal_handle = NULL;

static int divoom_google_calendar_is_event_dismissed(Pdivoom_google_calendar_event_item pevent_item)
{
    return pevent_item && pevent_item->user_dismissed;
}

static void divoom_google_calendar_resolve_event_color(Pdivoom_google_calendar_event_item pevent_item, Pdivoom_google_calendar_cal_item pcal_item)
{
	if (pevent_item == NULL || pcal_item == NULL)
	{
		return;
	}
	if (pevent_item->color_id[0] && divoom_google_calendar_get_color_value(1, pevent_item->color_id, pevent_item->bg_color, pevent_item->fr_color))
	{
		return;
	}
	memcpy(pevent_item->bg_color, pcal_item->bg_color, 3);
	memcpy(pevent_item->fr_color, pcal_item->fr_color, 3);
}

static void divoom_google_calendar_parse_event_color_id(divoom_cJSON *item, char *color_id, int color_id_size)
{
	divoom_cJSON *data_obj;

	if (item == NULL || color_id == NULL || color_id_size <= 0)
	{
		return;
	}
	color_id[0] = 0;
	data_obj = divoom_cJSON_GetObjectItem(item, "colorId");
	if (data_obj == NULL)
	{
		return;
	}
	if (data_obj->valuestring)
	{
		strncpy(color_id, data_obj->valuestring, color_id_size - 1);
	}
	else if (data_obj->type == divoom_cJSON_Number)
	{
		sprintf(color_id, "%d", data_obj->valueint);
	}
	color_id[color_id_size - 1] = 0;
}

static void divoom_google_calendar_resolve_cal_item_color(divoom_cJSON *item, Pdivoom_google_calendar_cal_item pcal_item, const char *cal_id)
{
	char cal_color_id[16];
	char url_info[64];
	int color_value;

	if (item == NULL || pcal_item == NULL)
	{
		return;
	}
	divoom_google_calendar_parse_event_color_id(item, cal_color_id, sizeof(cal_color_id));
	if (cal_color_id[0] && divoom_google_calendar_get_color_value(0, cal_color_id, pcal_item->bg_color, pcal_item->fr_color))
	{
		divoom_print("cal sync: %s, colorId:%s, bg:%02x%02x%02x\n", cal_id ? cal_id : "-", cal_color_id,
			pcal_item->bg_color[0], pcal_item->bg_color[1], pcal_item->bg_color[2]);
		return;
	}
	divoom_cJSON_GetObjectString(item, "backgroundColor", url_info);
	if (url_info[0])
	{
		sscanf(url_info, "#%x", &color_value);
		pcal_item->bg_color[0] = ((color_value >> 16) & 0xff);
		pcal_item->bg_color[1] = ((color_value >> 8) & 0xff);
		pcal_item->bg_color[2] = (color_value & 0xff);
	}
	divoom_cJSON_GetObjectString(item, "foregroundColor", url_info);
	if (url_info[0])
	{
		sscanf(url_info, "#%x", &color_value);
		pcal_item->fr_color[0] = ((color_value >> 16) & 0xff);
		pcal_item->fr_color[1] = ((color_value >> 8) & 0xff);
		pcal_item->fr_color[2] = (color_value & 0xff);
	}
	divoom_print("cal sync: %s, colorId:-, bg:%02x%02x%02x\n", cal_id ? cal_id : "-",
		pcal_item->bg_color[0], pcal_item->bg_color[1], pcal_item->bg_color[2]);
}

/*********************************************************************
Function:         divoom_google_calendar_free_all_event
Description:      释放日历事件资源
Input:            data
Return:           
Author:           albert
**********************************************************************/

void divoom_google_calendar_free_all_event(Pdivoom_google_calendar_event_item pevent_list, int event_num)
{
    for(int i= 0; i < event_num; i++)
    {
        divoom_system_free(pevent_list[i].event_id);
        divoom_system_free(pevent_list[i].title);
        divoom_system_free(pevent_list[i].desc);
        divoom_system_free(pevent_list[i].updated_time);
    }
    divoom_system_free(pevent_list);
    
}

static void divoom_google_calendar_detach_cal_event_popups(Pdivoom_google_calendar_cal_item pcal_item)
{
	Pdivoom_google_calendar_event_item pevent_item;
	int i;

	if (pcal_item == NULL || pcal_item->event_list == NULL || pcal_item->event_num <= 0
		|| pdivoom_google_calendar_cal_handle == NULL)
	{
		return;
	}
	for (i = 0; i < pcal_item->event_num; i++)
	{
		pevent_item = &pcal_item->event_list[i];
		if (divoom_queue_get_cmp(pdivoom_google_calendar_cal_handle->event_msg_queue, pevent_item,
			divoom_google_calendar_remove_popup_notice_message_compare) == NULL)
		{
			continue;
		}
		divoom_queue_pop_cmp(pdivoom_google_calendar_cal_handle->event_msg_queue, pevent_item,
			divoom_google_calendar_remove_popup_notice_message_compare);
	}
}

/*********************************************************************
Function:         divoom_google_calendar_find_active_event
Description:      得到激活的事件状态
Input:            data
Return:           
Author:           albert
**********************************************************************/

Pdivoom_google_calendar_event_item divoom_google_calendar_find_active_event(Pdivoom_google_calendar_event_item pevent_list, int event_num, int *result)
{
    int find_num = 0;
    Pdivoom_google_calendar_event_item pfind_list = NULL;
    for(int i= 0; i < event_num; i++)
    {
        if(pevent_list[i].status == DIVOOM_GOOGLE_CALENDAR_STATUS_ACTIVE
            || pevent_list[i].status == DIVOOM_GOOGLE_CALENDAR_STATUS_PASS
            || pevent_list[i].status == DIVOOM_GOOGLE_CALENDAR_STATUS_NOTICE)
        {
            find_num ++;
        }
    }
    *result = find_num;
    if(find_num == 0)
    {
        return NULL;
    }

    pfind_list = divoom_system_malloc(find_num * sizeof(divoom_google_calendar_event_item));
    for(int i= 0, find_num = 0; i < event_num; i++)
    {
        if(pevent_list[i].status == DIVOOM_GOOGLE_CALENDAR_STATUS_ACTIVE
            || pevent_list[i].status == DIVOOM_GOOGLE_CALENDAR_STATUS_PASS
            || pevent_list[i].status == DIVOOM_GOOGLE_CALENDAR_STATUS_NOTICE)
        {
            pfind_list[find_num].status = pevent_list[i].status;
            pfind_list[find_num].user_dismissed = pevent_list[i].user_dismissed;
            pfind_list[find_num].lvgl_obj = pevent_list[i].lvgl_obj;
            pfind_list[find_num].event_id = divoom_app_com_copy_string(pevent_list[i].event_id, -1);
            pfind_list[find_num].updated_time = divoom_app_com_copy_string(pevent_list[i].updated_time, -1);
            find_num ++;
        }
    }
    return pfind_list;
    
}

/*********************************************************************
Function:         divoom_google_calendar_free_all_cal
Description:      释放日历资源
Input:            data
Return:           
Author:           albert
**********************************************************************/

void divoom_google_calendar_free_all_cal(Pdivoom_google_calendar_cal_item pcal_item, int calendar_unm)
{
    if(pcal_item == NULL)
    {
        return;
    }
    for(int i = 0; i < calendar_unm; i++)
    {
        divoom_google_calendar_free_all_event(pcal_item[i].event_list, pcal_item[i].event_num);
        divoom_system_free(pcal_item[i].id);
    }
    divoom_system_free(pcal_item);
}

/*********************************************************************
Function:         divoom_google_calendar_request_cal_list
Description:      请求最新的日历信息
Input:            
Return:           
Author:           albert
**********************************************************************/

void divoom_google_calendar_add_cal_item(Pdivoom_google_calendar_cal_item pcal_item)
{
    Pdivoom_google_calendar_cal_item pnew_list = divoom_system_malloc((pdivoom_google_calendar_cal_handle->calendar_num + 1) * sizeof(divoom_google_calendar_cal_item));


    if(pdivoom_google_calendar_cal_handle->calendar_num)
    {
        memcpy(pnew_list, pdivoom_google_calendar_cal_handle->calendar_list, pdivoom_google_calendar_cal_handle->calendar_num * sizeof(divoom_google_calendar_cal_item));
    }
    divoom_system_free(pdivoom_google_calendar_cal_handle->calendar_list);
    pdivoom_google_calendar_cal_handle->calendar_list = pnew_list;
    memcpy(&pdivoom_google_calendar_cal_handle->calendar_list[pdivoom_google_calendar_cal_handle->calendar_num], pcal_item, sizeof(divoom_google_calendar_cal_item));
    pdivoom_google_calendar_cal_handle->calendar_list[pdivoom_google_calendar_cal_handle->calendar_num].id = divoom_app_com_copy_string(pcal_item->id, -1);
    pdivoom_google_calendar_cal_handle->calendar_list[pdivoom_google_calendar_cal_handle->calendar_num].event_num = 0;
    pdivoom_google_calendar_cal_handle->calendar_list[pdivoom_google_calendar_cal_handle->calendar_num].event_list = NULL;
    pdivoom_google_calendar_cal_handle->calendar_num ++;
}

/*********************************************************************
Function:         divoom_google_calendar_update_cal_item
Description:      更新的日历信息
Input:            
Return:           
Author:           albert
**********************************************************************/

void divoom_google_calendar_update_cal_item(Pdivoom_google_calendar_cal_item pcal_item, char *id_name)
{
    Pdivoom_google_calendar_cal_item pnew_list = divoom_system_malloc((pdivoom_google_calendar_cal_handle->calendar_num + 1) * sizeof(divoom_google_calendar_cal_item));

    if(pcal_item == NULL || id_name == NULL)
    {
        return;
    }
    for(int i = 0; i < pdivoom_google_calendar_cal_handle->calendar_num; i++)
    {
        if(strcmp(pdivoom_google_calendar_cal_handle->calendar_list[i].id, id_name) == 0)
        {
            //更新
            
            pdivoom_google_calendar_cal_handle->calendar_list[i].last_update_time[0] = 0;    
            memcpy(pdivoom_google_calendar_cal_handle->calendar_list[i].bg_color, pcal_item->bg_color, 3);     //背景颜色
            memcpy(pdivoom_google_calendar_cal_handle->calendar_list[i].fr_color, pcal_item->fr_color, 3);     //前景颜色
            memcpy(pdivoom_google_calendar_cal_handle->calendar_list[i].reminder_time , pcal_item->reminder_time, pcal_item->reminder_num * sizeof(int));   //默认的提醒时间
            pdivoom_google_calendar_cal_handle->event_request_time = 0;
            for (int j = 0; j < pdivoom_google_calendar_cal_handle->calendar_list[i].event_num; j++)
            {
                divoom_google_calendar_resolve_event_color(&pdivoom_google_calendar_cal_handle->calendar_list[i].event_list[j],
                    &pdivoom_google_calendar_cal_handle->calendar_list[i]);
            }
            
            return;
        }
    }
            
    pcal_item->id = id_name;
    divoom_google_calendar_add_cal_item(pcal_item);
}


/*********************************************************************
Function:         divoom_google_calendar_request_cal_list
Description:      请求最新的日历信息
Input:            
Return:           
Author:           albert
**********************************************************************/

void divoom_google_calendar_remove_cal_item(char *id_name)
{
    for(int i = 0; i < pdivoom_google_calendar_cal_handle->calendar_num; i++)
    {
        if(strcmp(pdivoom_google_calendar_cal_handle->calendar_list[i].id, id_name) == 0)
        {
            divoom_google_calendar_free_all_event(pdivoom_google_calendar_cal_handle->calendar_list[i].event_list, pdivoom_google_calendar_cal_handle->calendar_list[i].event_num);
            divoom_system_free(pdivoom_google_calendar_cal_handle->calendar_list[i].id);
            if( i != pdivoom_google_calendar_cal_handle->calendar_num - 1)
            {
                memmove(&pdivoom_google_calendar_cal_handle->calendar_list[i], &pdivoom_google_calendar_cal_handle->calendar_list[i + 1], sizeof(divoom_google_calendar_cal_item) * (pdivoom_google_calendar_cal_handle->calendar_num - i - 1));
            }
            pdivoom_google_calendar_cal_handle->calendar_num --;
            break;
        }
    }
}

/*********************************************************************
Function:         divoom_google_calendar_update_goole_token
Description:      更新日历的TOKEN
Input:            
Return:           
Author:           albert
**********************************************************************/

int divoom_google_calendar_update_goole_token(void)
{
    /* Called with the calendar lock held. The response is applied by net_app. */
    divoom_system_free(pdivoom_google_calendar_cal_handle->google_token);
    pdivoom_google_calendar_cal_handle->google_token = NULL;
    pdivoom_google_calendar_cal_handle->token_generation ++;
    pdivoom_google_calendar_cal_handle->token_refresh_pending = 1;
    pdivoom_google_calendar_cal_handle->token_refresh_tick = divoom_system_get_tickms();
    divoom_print("google token refresh queued; skip current sync\n");
    /* Enqueuing can wait for the HTTP worker's safe_lock: release our lock first. */
    divoom_system_free_lock(pdivoom_google_calendar_cal_handle->lock_handle);
    divoom_net_app_send_packet_http(DIVOOM_NET_COMM_DEVICE_GET_GOOGLE_CALENDAR_CONFIG, NULL, 0);
    divoom_system_get_lock(pdivoom_google_calendar_cal_handle->lock_handle);
    return 0;
}

/*********************************************************************
Function:         divoom_google_calendar_check_goole_err
Description:      检查报文是否有问题
Input:            
Return:           1：没有错误，0：要重新请求; -1:取消请求
Author:           albert
**********************************************************************/

int divoom_google_calendar_check_goole_err(divoom_cJSON *root)
{
    divoom_cJSON *item = divoom_cJSON_GetObjectItem(root, "error");
    if(item)
    {
        int err_id = divoom_cJSON_GetObjectInt(item, "code");
        if(err_id == 401)
        {
            //增加TOKEN过期
            divoom_google_calendar_update_goole_token();
            return -1;
        }
        else 
        {
            //其它错误，将重置nextSyncTokee
            divoom_system_free(pdivoom_google_calendar_cal_handle->nextSyncTokee);
            pdivoom_google_calendar_cal_handle->nextSyncTokee = NULL;
            divoom_google_calendar_free_all_cal(pdivoom_google_calendar_cal_handle->calendar_list, pdivoom_google_calendar_cal_handle->calendar_num);
            pdivoom_google_calendar_cal_handle->calendar_num = 0;
            pdivoom_google_calendar_cal_handle->calendar_list = NULL;
        }
        return 0;
        
    }
    return 1;
}

/*********************************************************************
Function:         divoom_google_calendar_request_goole_data
Description:      请求最新的日历信息
Input:            
Return:           
Author:           albert
**********************************************************************/

char *divoom_google_calendar_request_goole_data(char *url_addr)
{
    char *cal_data = NULL;
    uint32 token_generation = pdivoom_google_calendar_cal_handle->token_generation;
    Pdivoom_http_client_url_head_info phead_info = divoom_system_malloc(sizeof(divoom_http_client_url_head_info));

    if(phead_info)
    {
        divoom_http_client_head_set_bearer_auth(phead_info, pdivoom_google_calendar_cal_handle->google_token);
        if(divoom_main_get_server_type())
        {
            divoom_system_free_lock(pdivoom_google_calendar_cal_handle->lock_handle);
            cal_data = divoom_http_client_url_request(url_addr, phead_info, 1, NULL, NULL);
            divoom_system_get_lock(pdivoom_google_calendar_cal_handle->lock_handle);
        }
        else
        {
            //向服务器转请求
            divoom_device_get_url_info_respone_info info = {0};
            Pdivoom_net_command ret_data = NULL;

            info.body = "";
            info.url_addr = url_addr;
            info.head_num = 1;
            info.phead_info = phead_info;
            divoom_system_free_lock(pdivoom_google_calendar_cal_handle->lock_handle);
            ret_data = divoom_net_app_send_packet_http(DIVOOM_NET_COMM_DEVICE_GET_URL_NET, &info, 1);
            divoom_system_get_lock(pdivoom_google_calendar_cal_handle->lock_handle);
            if(ret_data)
            {                
                cal_data = (char *)ret_data->cmd_data;
                //divoom_print("get data: %s\n", cal_data);
                ret_data->cmd_data = NULL;
                divoom_net_app_free_packet(ret_data);
            }
        }
        divoom_system_free(phead_info);
    }
    if(token_generation != pdivoom_google_calendar_cal_handle->token_generation)
    {
        /* A token callback ran while the request was in flight. */
        divoom_system_free(cal_data);
        cal_data = NULL;
    }
    return cal_data;
}

/*********************************************************************
Function:         divoom_google_calendar_request_cal_list
Description:      请求最新的日历信息
Input:            
Return:           
Author:           albert
**********************************************************************/

void divoom_google_calendar_request_cal_list(void)
{
    int i ,tol, deleted_flag, color_value, retry_time = 0, request_status;
    char *cal_data, url_info[512];
	divoom_cJSON *root = NULL, *list, *item, *id, *color_obj, *remind_list, *remind_item;
    divoom_google_calendar_cal_item *pcal_item, cal_item = {0};

    pcal_item = &cal_item;
    divoom_google_calendar_request_cal_list_retry_table:
    #ifdef DIVOOM_GOOGLE_CALENDAR_TEST
    if(pdivoom_google_calendar_cal_handle->nextSyncTokee && strlen(pdivoom_google_calendar_cal_handle->nextSyncTokee))
    {
        cal_data = NULL;
    }
    else
    {
        cal_data = divoom_app_com_read_file("/userdata/cal/calendar.txt", NULL);
    }
    
    #else
    if(pdivoom_google_calendar_cal_handle->google_token == NULL || strlen(pdivoom_google_calendar_cal_handle->google_token) == 0)
    {
        return;
    }
    if(pdivoom_google_calendar_cal_handle->nextSyncTokee && strlen(pdivoom_google_calendar_cal_handle->nextSyncTokee))
    {
        sprintf(url_info, "%s%s",DIVOOM_GOOGLE_CALENDAR_REQUEST_OTHER_CAL_LIST,pdivoom_google_calendar_cal_handle->nextSyncTokee);
    }
    else
    {
        strcpy(url_info,DIVOOM_GOOGLE_CALENDAR_REQUEST_CAL_LIST);
    }
    cal_data = divoom_google_calendar_request_goole_data(url_info);
    #endif
    if(cal_data == NULL)
    {   
        return;
    }
	root = divoom_cJSON_Parse(cal_data);
    divoom_system_free(cal_data);
	if (!root)
	{
        
		return ;
	}

    if((request_status = divoom_google_calendar_check_goole_err(root)) <= 0)
    {
        divoom_cJSON_Delete(root);
        root = NULL;
        retry_time ++;
        if(request_status < 0 || retry_time > 1)
        {
            return;
        }
        goto divoom_google_calendar_request_cal_list_retry_table;
    }
    
    divoom_system_free(pdivoom_google_calendar_cal_handle->nextSyncTokee);
    pdivoom_google_calendar_cal_handle->nextSyncTokee = NULL;
    item = divoom_cJSON_GetObjectItem(root, "nextSyncToken");
    if(item && item->valuestring)
    {
        pdivoom_google_calendar_cal_handle->nextSyncTokee = divoom_app_com_copy_string(item->valuestring, -1);
    }
    else
    {
        pdivoom_google_calendar_cal_handle->nextSyncTokee = NULL;
    }
    
    list = divoom_cJSON_GetObjectItem(root, "items");
    if(list == NULL || divoom_cJSON_GetArraySize(list)==0)
    {
        divoom_cJSON_Delete(root);
        
		return ;
    }
    
    tol  = divoom_cJSON_GetArraySize(list);
    for(i = 0; i < tol; i ++)
    {
        item = divoom_cJSON_GetArrayItem(list, i);
        if(item == NULL)
        {
            continue;
        }
        deleted_flag = divoom_cJSON_GetObjectInt(item, "deleted");
        divoom_cJSON_GetObjectString(item, "updated", pcal_item->last_update_time);
        id = divoom_cJSON_GetObjectItem(item, "id");
        if(id && id->valuestring)
        {
            if(deleted_flag)
            {
                //删除相关的日历
                
                divoom_google_calendar_remove_cal_item(id->valuestring);
                pdivoom_google_calendar_cal_handle->event_request_time = 0;
                continue;
            }
            //更新日历相关信息
            divoom_google_calendar_resolve_cal_item_color(item, pcal_item, id->valuestring);
            remind_list = divoom_cJSON_GetObjectItem(item, "defaultReminders");

            pcal_item->reminder_num = 0;
            for(int k = 0; k < divoom_cJSON_GetArraySize(remind_list); k++)
            {
                remind_item = divoom_cJSON_GetArrayItem(remind_list, i);
                if(remind_item = NULL)
                {
                    break;
                }
                divoom_cJSON_GetObjectString(remind_item, "method", url_info);
                if(strcmp(url_info, "popup") == 0)
                {
                    pcal_item->reminder_time[pcal_item->reminder_num] = divoom_cJSON_GetObjectInt(remind_item, "minutes");
                    pcal_item->reminder_num ++;
                }
            }
            
            divoom_google_calendar_update_cal_item(pcal_item, id->valuestring);
            
        }
        
    }
    
    divoom_cJSON_Delete(root);
}



#if 0
#include <time.h>
#include <stdio.h>
#include <string.h>

int utc_to_local_rfc3339(time_t utc_time, char *output, size_t size) {
    struct tm local_tm;
    char datetime[32];
    char timezone[8];
    
    if (output == NULL || size < 32) {
        return -1;
    }
    
    // 转换为本地时间
    if (localtime_r(&utc_time, &local_tm) == NULL) {
        return -1;
    }
    
    // 格式化日期时间
    if (strftime(datetime, sizeof(datetime), "%Y-%m-%dT%H:%M:%S", &local_tm) == 0) {
        return -1;
    }
    
    
    // 格式化完整的RFC3339字符串
    if (timezone[0] != '+' && timezone[0] != '-') {
        return -1;
    }
    
    sprintf(output, "%s%c%c%c:%c%c", 
             datetime,
             timezone[0], timezone[1], timezone[2],
             timezone[3], timezone[4]);
             
    return 0;
}

#endif

/*********************************************************************
Function:         divoom_google_calendar_get_all_color_info
Description:      重新请求颜色列表，只有一个事件更新，先调用这个
Input:            cal_id
Return:           
Author:           albert
**********************************************************************/

int divoom_google_calendar_get_all_color_info(void)
{
    int color_value, retry_time = 0, request_status;
    char *color_data, color_update_time[32];
    Pdivoom_google_calendar_color_item pcolor_item;
	divoom_cJSON *root = NULL, *list, *item, *data_obj, *tmp_obj;

    divoom_google_calendar_get_all_color_info_retry_lable:
    #ifdef DIVOOM_GOOGLE_CALENDAR_TEST
    color_data = divoom_app_com_read_file("/userdata/cal/color.txt", NULL);
    #else 
    if(pdivoom_google_calendar_cal_handle->google_token == NULL || strlen(pdivoom_google_calendar_cal_handle->google_token) == 0)
    {
        return 0;
    }
    
    color_data = divoom_google_calendar_request_goole_data(DIVOOM_GOOGLE_CALENDAR_REQUEST_COLOR_LIST);
    #endif
    if(color_data == NULL)
    {
        
        return 0;
    }
    root = divoom_cJSON_Parse(color_data);
    divoom_system_free(color_data);
    if (!root)
    {
        
        
        return 0;
    }
    
    if((request_status = divoom_google_calendar_check_goole_err(root)) <= 0)
    {
        divoom_cJSON_Delete(root);
        root = NULL;
        retry_time ++;
        if(request_status < 0 || retry_time > 1)
        {
            return 0;
        }
        goto divoom_google_calendar_get_all_color_info_retry_lable;
    }
    
    divoom_cJSON_GetObjectString(root, "updated", color_update_time);
    if(strcmp(color_update_time, pdivoom_google_calendar_cal_handle->color_update_time) == 0)
    {
        
        
        divoom_cJSON_Delete(root);
        return 0;
    }
    strcpy(pdivoom_google_calendar_cal_handle->color_update_time, color_update_time);
    divoom_queue_pop_all(pdivoom_google_calendar_cal_handle->color_list, NULL);
    data_obj = divoom_cJSON_GetObjectItem(root, "calendar");
    if(data_obj && data_obj->child)
    {
        list = data_obj->child;
        
        while (list && list->string) {
            pcolor_item = divoom_system_malloc(sizeof(divoom_google_calendar_color_item));

            pcolor_item->type = 0;
            strcpy(pcolor_item->color_id,list->string);
        
            divoom_cJSON_GetObjectString(list, "background", color_update_time); 
            sscanf(color_update_time, "#%x", &color_value);
            pcolor_item->bg_color[0] = ((color_value >> 16) &0xff);
            pcolor_item->bg_color[1] = ((color_value >> 8) &0xff);
            pcolor_item->bg_color[2] = ((color_value >> 0) &0xff);
            divoom_cJSON_GetObjectString(list, "foreground", color_update_time); 
            sscanf(color_update_time, "#%x", &color_value);
            pcolor_item->fr_color[0] = ((color_value >> 16) &0xff);
            pcolor_item->fr_color[1] = ((color_value >> 8) &0xff);
            pcolor_item->fr_color[2] = ((color_value >> 0) &0xff);
            divoom_queue_push(pdivoom_google_calendar_cal_handle->color_list, pcolor_item);
            list = list->next;
        }
    }
    data_obj = divoom_cJSON_GetObjectItem(root, "event");
    if(data_obj && data_obj->child)
    {
        list = data_obj->child;
        
        while (list && list->string) {
            pcolor_item = divoom_system_malloc(sizeof(divoom_google_calendar_color_item));

            pcolor_item->type = 1;
            strcpy(pcolor_item->color_id ,list->string);
        
            divoom_cJSON_GetObjectString(list, "background", color_update_time); 
            sscanf(color_update_time, "#%x", &color_value);
            pcolor_item->bg_color[0] = ((color_value >> 16) &0xff);
            pcolor_item->bg_color[1] = ((color_value >> 8) &0xff);
            pcolor_item->bg_color[2] = ((color_value >> 0) &0xff);
            divoom_cJSON_GetObjectString(list, "foreground", color_update_time); 
            sscanf(color_update_time, "#%x", &color_value);
            pcolor_item->fr_color[0] = ((color_value >> 16) &0xff);
            pcolor_item->fr_color[1] = ((color_value >> 8) &0xff);
            pcolor_item->fr_color[2] = ((color_value >> 0) &0xff);
            divoom_queue_push(pdivoom_google_calendar_cal_handle->color_list, pcolor_item);
            list = list->next;
        }
    }
    divoom_google_calendar_save_config();
    divoom_cJSON_Delete(root);
    return 1;
}

/*********************************************************************
Function:         divoom_google_calendar_format_event_notice_time
Description:      计算事件的提醒时间
Input:            data
Return:           
Author:           albert
**********************************************************************/

int divoom_google_calendar_format_event_notice_time(void *data)
{
    time_t cur_time, reminder_time = 0;
    Pdivoom_google_calendar_event_item pevent_item = (Pdivoom_google_calendar_event_item)data;
    struct tm cur_date = {0};
    cur_time = time(0);

    if(pevent_item->start_utc < cur_time)
    {
        if(pevent_item->end_utc > cur_time)
        {
            pevent_item->status = DIVOOM_GOOGLE_CALENDAR_STATUS_PASS;
        }
        pevent_item->next_notice_time = 0;
        return 0;
    }
    pevent_item->next_notice_time = pevent_item->start_utc;
    for(int i = 0; i < pevent_item->reminder_num; i++)
    {
        reminder_time = pevent_item->start_utc - pevent_item->reminder_time[i];
        if(reminder_time <= cur_time)
        {
            continue;
        }
        if(pevent_item->next_notice_time > (int)reminder_time)
        {
            pevent_item->next_notice_time = (int)reminder_time;
        }
    }
    if(pevent_item->next_notice_time == pevent_item->start_utc)
    {
        pevent_item->next_notice_time = 0;
    }
        return 1;
}

/*********************************************************************
Function:         divoom_google_calendar_request_event_list
Description:      重新请求某一日历下的所有事件
Input:            cal_id
Return:           
Author:           albert
**********************************************************************/

int divoom_google_calendar_request_event_list(Pdivoom_google_calendar_cal_item pcal_item)
{
    char url_info[512];
    char timezone_path[128];
    int cal_num, update_time, j, k, event_num, active_flag, next_mon, next_day, next_year, delete_flag, last_update_time = 0, all_day_flag, cur_active_event_num;
    time_t cur_time, start_time, end_time;
    struct tm cur_date = {0}, update_date = {0};
    char *cal_data, *active_event_id = NULL, *active_event_updated = NULL;
	divoom_cJSON *root = NULL, *list, *item, *data_obj, *tmp_obj;
    Pdivoom_google_calendar_event_item pevent_item;
    Pdivoom_google_calendar_event_item pcur_active_event_list;

    cur_time = time(0);
    #ifndef DIVOOM_GOOGLE_CALENDAR_TEST
    if(pdivoom_google_calendar_cal_handle->google_token == NULL || strlen(pdivoom_google_calendar_cal_handle->google_token) == 0)
    {
        return 0;
    }
    #endif
    localtime_r(&cur_time, &cur_date);
    start_time = cur_time - (cur_date.tm_mday - 1) * 24*3600 - cur_date.tm_hour*3600 - cur_date.tm_min*60 - cur_date.tm_sec ;
    localtime_r(&start_time, &cur_date);
    start_time -= (cur_date.tm_wday  * 24*3600);
    end_time = start_time + 42 *24 * 3600;
    localtime_r(&end_time, &cur_date);
    next_mon = cur_date.tm_mon + 1;
    next_year = cur_date.tm_year + 1900;
    next_day = cur_date.tm_mday;
    localtime_r(&start_time, &cur_date);
    
    #ifdef DIVOOM_GOOGLE_CALENDAR_TEST
    if(strcmp(pcal_item->id, "socialdivoom@gmail.com") == 0)
    {
        cal_data = divoom_app_com_read_file("/userdata/cal/divoom.txt", NULL);
    }
    else
    {
        cal_data = divoom_app_com_read_file("/userdata/cal/DivoomTest.txt", NULL);
    }
    #else
    if(strlen(pcal_item->last_update_time))
    {
        //如果有时间，将先检查时间
        sprintf(url_info, DIVOOM_GOOGLE_CALENDAR_REQUEST_EVENT_UPDATE_TIME, pcal_item->id,cur_date.tm_year + 1900,cur_date.tm_mon + 1,cur_date.tm_mday,
            next_year, next_mon, next_day);     
        cal_data = divoom_google_calendar_request_goole_data(url_info);
        if(cal_data == NULL)
        {
            return 0;
        }
        root = divoom_cJSON_Parse(cal_data);
        divoom_system_free(cal_data);
        if (!root)
        {        
            return 0;
        }
        
        if(divoom_google_calendar_check_goole_err(root) <= 0)
        {
            /* Error handling may invalidate pcal_item; retry on the next sync. */
            divoom_cJSON_Delete(root);
            return 0;
        }
    
        tmp_obj = divoom_cJSON_GetObjectItem(root, "updated");
        if(tmp_obj && tmp_obj->valuestring)
        {
            divoom_print("update: %s, %s\n", tmp_obj->valuestring, pcal_item->last_update_time);
            if(strcmp(tmp_obj->valuestring, pcal_item->last_update_time) == 0)
            {
                divoom_cJSON_Delete(root);
                return 0;
            }
        }
        else
        {
            divoom_print("no update time!\n");
        }
        divoom_cJSON_Delete(root);
        root = NULL;
    }
    sprintf(url_info, DIVOOM_GOOGLE_CALENDAR_REQUEST_EVENT_LIST, pcal_item->id,cur_date.tm_year + 1900,cur_date.tm_mon + 1,cur_date.tm_mday,
        next_year, next_mon, next_day);     
//"https://www.googleapis.com/calendar/v3/calendars/calendarId/events?calendarId=%s&maxResults=10000&timeMin=%04d-%02d-01T00:00:00Z&timeMax=%04d-%02d-01dT00:00:00Z&syncToken=%s&showDeleted=1&orderBy=startTime&singleEvents=1"
    cal_data = divoom_google_calendar_request_goole_data(url_info);
    #endif
    
    if(cal_data == NULL)
    {
        
        return 0;
    }
    root = divoom_cJSON_Parse(cal_data);
    divoom_system_free(cal_data);
    if (!root)
    {        
        return 0;
    }
    
    if(divoom_google_calendar_check_goole_err(root) <= 0)
    {
        divoom_cJSON_Delete(root);
        return 0;
    }
    tmp_obj = divoom_cJSON_GetObjectItem(root, "updated");
    if(tmp_obj && tmp_obj->valuestring)
    {
        strcpy(pcal_item->last_update_time, tmp_obj->valuestring);
    }
    
    
    //先统计默认的提醒时间
    list = divoom_cJSON_GetObjectItem(root, "defaultReminders");
    pcal_item->reminder_num = 0;
    for(j = 0; list && j <  divoom_cJSON_GetArraySize(list); j ++)
    {
        item = divoom_cJSON_GetArrayItem(list, j);
        divoom_cJSON_GetObjectString(item, "method", url_info);
        if(strcmp(url_info, "popup") == 0)
        {
            pcal_item->reminder_time[pcal_item->reminder_num] = divoom_cJSON_GetObjectInt(item, "minutes") * 60;
            pcal_item->reminder_num ++;
        }
    }
    
        //统计所有的事件信息
    list = divoom_cJSON_GetObjectItem(root, "items");
    if(list == NULL)
    {
        divoom_cJSON_Delete(root);
        
        return 0;
    }
    //每次都是取全部事件，所以这个地方先删除
    //先统计，状态为激活与取消的数据
    event_num = divoom_cJSON_GetArraySize(list);
    if(pdivoom_google_calendar_cal_handle->prunning_event)
    {
        for(j = 0; j < pcal_item->event_num; j++)
        {
            if(&pcal_item->event_list[j] == pdivoom_google_calendar_cal_handle->prunning_event)
            {
                active_event_id = divoom_app_com_copy_string(pdivoom_google_calendar_cal_handle->prunning_event->event_id, -1);
                active_event_updated = divoom_app_com_copy_string(pdivoom_google_calendar_cal_handle->prunning_event->updated_time, -1);
                pdivoom_google_calendar_cal_handle->prunning_event = NULL;
            }
        }
    }
    pcur_active_event_list = divoom_google_calendar_find_active_event(pcal_item->event_list, pcal_item->event_num, &cur_active_event_num);
    divoom_google_calendar_detach_cal_event_popups(pcal_item);
    divoom_google_calendar_free_all_event(pcal_item->event_list, pcal_item->event_num);
    pcal_item->event_num = event_num;
    pcal_item->event_list = divoom_system_malloc(sizeof(divoom_google_calendar_event_item)*event_num);
    for(j = 0; j < event_num; j ++)
    {
        
        item = divoom_cJSON_GetArrayItem(list, j);
        if(item == NULL)
        {
        
            
            break;
        }
        pevent_item = &pcal_item->event_list[j];
        memset(pevent_item, 0, sizeof(divoom_google_calendar_event_item));
        
        divoom_google_calendar_parse_event_color_id(item, pevent_item->color_id, sizeof(pevent_item->color_id));
        divoom_google_calendar_resolve_event_color(pevent_item, pcal_item);
        data_obj = divoom_cJSON_GetObjectItem(item, "summary");
        if (data_obj && data_obj->valuestring && pevent_item->color_id[0])
        {
            divoom_print("event sync: %s, colorId:%s, color:%02x%02x%02x\n", data_obj->valuestring, pevent_item->color_id,
                pevent_item->bg_color[0], pevent_item->bg_color[1], pevent_item->bg_color[2]);
        }
        
        
    
        
        data_obj = divoom_cJSON_GetObjectItem(item, "id");
        if(data_obj && data_obj->valuestring)
        {
            pevent_item->event_id  = divoom_app_com_copy_string(data_obj->valuestring, -1);
        }
        
        data_obj = divoom_cJSON_GetObjectItem(item, "updated");
        if(data_obj && data_obj->valuestring)
        {
            pevent_item->updated_time  = divoom_app_com_copy_string(data_obj->valuestring, -1);
        }

        if((active_event_id && strcmp(pevent_item->event_id, active_event_id) == 0))
        {
            //激活的发生变化，将取消
            if(active_event_updated && strcmp(pevent_item->updated_time, active_event_updated))
            {
                divoom_google_calendar_send_event_notice_message(NULL, DIVOOM_GOOGLE_CALENDAR_STATUS_IDLE);
            }
            else if(!divoom_google_calendar_is_event_dismissed(pevent_item))
            {
                pdivoom_google_calendar_cal_handle->prunning_event = pevent_item;
            }
        }
        for(int m = 0; m < cur_active_event_num; m ++)
        {
            if(strcmp(pevent_item->event_id, pcur_active_event_list[m].event_id) == 0)
            {
                if(strcmp(pevent_item->updated_time, pcur_active_event_list[m].updated_time) == 0)
                {
                    pevent_item->status = pcur_active_event_list[m].status;
                    pevent_item->user_dismissed = pcur_active_event_list[m].user_dismissed;
                    if (divoom_google_calendar_is_event_dismissed(pevent_item))
                    {
                        pevent_item->status = DIVOOM_GOOGLE_CALENDAR_STATUS_PASS;
                    }
                    if (pcur_active_event_list[m].lvgl_obj)
                    {
                        pevent_item->lvgl_obj = pcur_active_event_list[m].lvgl_obj;
                        divoom_queue_push(pdivoom_google_calendar_cal_handle->event_msg_queue, pevent_item);
                        divoom_system_msg_notice_update_para(pevent_item->lvgl_obj, pevent_item);
                    }
                }
                break;
            }
        }
        data_obj = divoom_cJSON_GetObjectItem(item, "summary");
        if(data_obj && data_obj->valuestring)
        {
            pevent_item->title  = divoom_app_com_copy_string(data_obj->valuestring, -1);
        }
        data_obj = divoom_cJSON_GetObjectItem(item, "description");
        if(data_obj && data_obj->valuestring)
        {
            pevent_item->desc  = divoom_app_com_copy_string(data_obj->valuestring, -1);
        }
    
        
        data_obj = divoom_cJSON_GetObjectItem(item, "start");
        if(data_obj)
        {
            tmp_obj = divoom_cJSON_GetObjectItem(data_obj, "dateTime");
            if(tmp_obj && tmp_obj->valuestring)
            {
                sscanf(tmp_obj->valuestring, "%d-%d-%dT%d:%d:%d", &update_date.tm_year,&update_date.tm_mon ,
                    &update_date.tm_mday, &update_date.tm_hour, &update_date.tm_min, &update_date.tm_sec);
                if(strchr(tmp_obj->valuestring, 'Z'))
                {
                    tmp_obj = divoom_cJSON_GetObjectItem(data_obj, "timeZone");
                    if(tmp_obj && tmp_obj->valuestring)
                    {
                        char *time_zone_ptr;
                        int hour, min;
                        
                        for(int i = 0; i < strlen(tmp_obj->valuestring); i++)
                        {
                            if(tmp_obj->valuestring[i] == ' ')
                            {
                                tmp_obj->valuestring[i] = '_';
                            }
                        }

                        sprintf(timezone_path, "TZ=/usr/share/zoneinfo/%s date +%%z", tmp_obj->valuestring);
                        divoom_exec_cmd(timezone_path, url_info, sizeof(url_info) - 1);
                        time_zone_ptr = strchr(url_info, '+');
                        if(time_zone_ptr)
                        {
                            time_zone_ptr ++;
                            sscanf(time_zone_ptr, "%02d%02d", &hour, &min);
                            update_date.tm_hour += hour;
                            update_date.tm_min += min;
                        }
                        else
                        {
                            time_zone_ptr = strchr(url_info, '-');
                            if(time_zone_ptr)
                            {
                                time_zone_ptr ++;
                                sscanf(time_zone_ptr, "%02d%02d", &hour, &min);
                                update_date.tm_hour -= hour;
                                update_date.tm_min -= min;
                            }
                        }
                    }
                }
            }
            else
            {
                tmp_obj = divoom_cJSON_GetObjectItem(data_obj, "date");
                if(tmp_obj && tmp_obj->valuestring)
                {
                    sscanf(tmp_obj->valuestring, "%d-%d-%d", &update_date.tm_year,&update_date.tm_mon ,
                        &update_date.tm_mday);
                }
                update_date.tm_hour = 0;
                update_date.tm_min  = 0;
                update_date.tm_sec = 0;
            }
            update_date.tm_year -= 1900;
            update_date.tm_mon --;
            pevent_item->start_utc = mktime(&update_date);
        }
    
        
        data_obj = divoom_cJSON_GetObjectItem(item, "end");
        all_day_flag = 0;
        if(data_obj)
        {
            tmp_obj = divoom_cJSON_GetObjectItem(data_obj, "dateTime");
            if(tmp_obj && tmp_obj->valuestring)
            {
                sscanf(tmp_obj->valuestring, "%d-%d-%dT%d:%d:%d\n", &update_date.tm_year,&update_date.tm_mon ,
                    &update_date.tm_mday, &update_date.tm_hour, &update_date.tm_min, &update_date.tm_sec);
                if(strchr(tmp_obj->valuestring, 'Z'))
                {
                    tmp_obj = divoom_cJSON_GetObjectItem(data_obj, "timeZone");
                    if(tmp_obj && tmp_obj->valuestring)
                    {
                        char *time_zone_ptr;
                        int hour, min;
                        
                        for(int i = 0; i < strlen(tmp_obj->valuestring); i++)
                        {
                            if(tmp_obj->valuestring[i] == ' ')
                            {
                                tmp_obj->valuestring[i] = '_';
                            }
                        }

                        sprintf(timezone_path, "TZ=/usr/share/zoneinfo/%s date +%%z", tmp_obj->valuestring);
                        divoom_exec_cmd(timezone_path, url_info, sizeof(url_info) - 1);
                        time_zone_ptr = strchr(url_info, '+');
                        if(time_zone_ptr)
                        {
                            time_zone_ptr ++;
                            sscanf(time_zone_ptr, "%02d%02d", &hour, &min);
                            update_date.tm_hour += hour;
                            update_date.tm_min += min;
                        }
                        else
                        {
                            time_zone_ptr = strchr(url_info, '-');
                            if(time_zone_ptr)
                            {
                                time_zone_ptr ++;
                                sscanf(time_zone_ptr, "%02d%02d", &hour, &min);
                                update_date.tm_hour -= hour;
                                update_date.tm_min -= min;
                            }
                        }
                    }
                }
                
            }
            else
            {
                tmp_obj = divoom_cJSON_GetObjectItem(data_obj, "date");
                if(tmp_obj && tmp_obj->valuestring)
                {
                    sscanf(tmp_obj->valuestring, "%d-%d-%d", &update_date.tm_year,&update_date.tm_mon ,
                        &update_date.tm_mday);
                }
                update_date.tm_hour = 0;
                update_date.tm_min  = 0;
                update_date.tm_sec = 0;
                all_day_flag = 1;
            }
            update_date.tm_year -= 1900;
            update_date.tm_mon --;
            pevent_item->end_utc = mktime(&update_date);
        }
        
        if(all_day_flag == 0)
        {
            data_obj = divoom_cJSON_GetObjectItem(item, "reminders");
            if(data_obj)
            {
                if(divoom_cJSON_GetObjectInt(data_obj, "useDefault") == 0)
                {
                    pevent_item->reminder_num = 0;
                    
                    data_obj = divoom_cJSON_GetObjectItem(data_obj, "overrides");
                    if(data_obj)
                    {
                        for(k = 0; k < divoom_cJSON_GetArraySize(data_obj); k++)
                        {
                            
                            tmp_obj = divoom_cJSON_GetArrayItem(data_obj, k);
                            divoom_cJSON_GetObjectString(tmp_obj, "method", url_info);
                            if(strcmp(url_info, "popup") == 0)
                            {
                                pevent_item->reminder_time[pevent_item->reminder_num] =  divoom_cJSON_GetObjectInt(tmp_obj, "minutes")*60;    
                                pevent_item->reminder_num ++;
                            }
                        }
                        
                    }
                }
                else
                {
                    //使用默认的
                    pevent_item->reminder_num = pcal_item->reminder_num;
                    memcpy(pevent_item->reminder_time, pcal_item->reminder_time, pcal_item->reminder_num * sizeof(int));
                    
                }
            }
        }        
        divoom_google_calendar_format_event_notice_time(pevent_item);
        if (divoom_google_calendar_is_event_dismissed(pevent_item))
        {
            pevent_item->status = DIVOOM_GOOGLE_CALENDAR_STATUS_PASS;
        }
        
    }
    if(pcur_active_event_list)
    {
        divoom_google_calendar_free_all_event(pcur_active_event_list, cur_active_event_num);
    }
    if(active_event_id)
    {
        divoom_system_free(active_event_id);
    }
    if(active_event_updated)
    {
        divoom_system_free(active_event_updated);
    }
    divoom_cJSON_Delete(root);
    return 1;
}


/*********************************************************************
Function:         divoom_google_calendar_user_cancle_event
Description:      按键取消
Input:            
Return:           
Author:           albert
**********************************************************************/

void divoom_google_calendar_user_cancle_event(void)
{
    Pdivoom_google_calendar_cal_item pcal_item;
    Pdivoom_google_calendar_event_item pevent_item;
    divoom_sys_msg_info model_msg = {0};
    time_t cur_time;
    int i, j, found = 0;

    if (pdivoom_google_calendar_cal_handle == NULL)
    {
        return;
    }
    cur_time = time(0);
    divoom_system_get_lock(pdivoom_google_calendar_cal_handle->lock_handle);
    for (i = 0; i < pdivoom_google_calendar_cal_handle->calendar_num; i++)
    {
        pcal_item = &pdivoom_google_calendar_cal_handle->calendar_list[i];
        for (j = 0; j < pcal_item->event_num; j++)
        {
            pevent_item = &pcal_item->event_list[j];
            if (pevent_item->start_utc < cur_time
                && pevent_item->end_utc > cur_time
                && !divoom_google_calendar_is_event_dismissed(pevent_item))
            {
                pevent_item->user_dismissed = 1;
                pevent_item->status = DIVOOM_GOOGLE_CALENDAR_STATUS_PASS;
                found = 1;
            }
        }
    }
    if(found)
    {
        pdivoom_google_calendar_cal_handle->prunning_event = NULL;

    	model_msg.src_id = DIVOOM_MODEL_NET;
        model_msg.msg_type = 1;
    	model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
    	model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_GOOGLE_CALENDAR_CTRL;
    	model_msg.data = NULL;
    	divoom_sys_model_crl_msg_push(&model_msg);
    }
    divoom_system_free_lock(pdivoom_google_calendar_cal_handle->lock_handle);
}


/*********************************************************************
Function:         divoom_google_calendar_request_all_cal_event
Description:      请求所有的日历所有事件信息，更新
Input:            
Return:           
Author:           albert
**********************************************************************/

int divoom_google_calendar_request_all_cal_event(void)
{
    int i;
    uint32 token_generation = pdivoom_google_calendar_cal_handle->token_generation;
    Pdivoom_google_calendar_cal_item pcal_item;
    

    for(int i = 0; i < pdivoom_google_calendar_cal_handle->calendar_num; i++)
    {
        pcal_item = &pdivoom_google_calendar_cal_handle->calendar_list[i];
        if(divoom_google_calendar_request_event_list(pcal_item) == 1)
        {
            pdivoom_google_calendar_cal_handle->update_flag = 1;
        }
        if(token_generation != pdivoom_google_calendar_cal_handle->token_generation)
        {
            break;
        }
    }

    return pdivoom_google_calendar_cal_handle->update_flag;
}

/*********************************************************************
Function:         divoom_google_calendar_send_event_notice
Description:      向系统发送提醒事件
Input:            none
Return:           
Author:           albert
**********************************************************************/

void divoom_google_calendar_send_event_notice(void *data)
{
	divoom_sys_msg_info model_msg = {0};

    pdivoom_google_calendar_cal_handle->prunning_event = data;

    if(data)
    {
        data = divoom_system_malloc(sizeof(int));
    }

    model_msg.msg_type = 1;
	model_msg.src_id = DIVOOM_MODEL_NET;
    
	model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
	model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_GOOGLE_CALENDAR_CTRL;
	model_msg.data = data;
	divoom_sys_model_crl_msg_push(&model_msg);
}

static void divoom_google_calendar_trim_text(char *text);

/**
 * @brief 将 Google 日历返回的 HTML 内容转换为纯文本
 * @param html_str 输入的 HTML 字符串
 * @param out_text 输出的纯文本缓冲区
 * @param out_size 输出缓冲区大小
 * @return 成功返回 0，失败返回 -1
 */
int divoom_google_calendar_html_to_text(const char *html_str, char *out_text, int out_size)
{
    if (!html_str || !out_text || out_size <= 0) {
        return -1;
    }

    const char *src = html_str;
    char *dst = out_text;
    char *dst_end = out_text + out_size - 1;
    int in_tag = 0;           // 是否在 HTML 标签内
    int in_entity = 0;        // 是否在 HTML 实体内
    char entity_buf[16] = {0};
    int entity_idx = 0;

    while (*src && dst < dst_end) {
        if (*src == '<') {
            // 检查是否是 <br> 或 <br/> 标签，转换为换行
            if (strncasecmp(src, "<br>", 4) == 0 || 
                strncasecmp(src, "<br/>", 5) == 0 ||
                strncasecmp(src, "<br />", 6) == 0) {
                *dst++ = '\n';
            }
            // 检查是否是 </p> 或 </div> 标签，添加换行
            else if (strncasecmp(src, "</p>", 4) == 0 || 
                     strncasecmp(src, "</div>", 6) == 0) {
                if (dst > out_text && *(dst-1) != '\n') {
                    *dst++ = '\n';
                }
            }
            in_tag = 1;
            src++;
        }
        else if (*src == '>') {
            in_tag = 0;
            src++;
        }
        else if (in_tag) {
            // 跳过标签内的内容
            src++;
        }
        else if (*src == '&') {
            // HTML 实体开始
            in_entity = 1;
            entity_idx = 0;
            memset(entity_buf, 0, sizeof(entity_buf));
            entity_buf[entity_idx++] = *src++;
        }
        else if (in_entity) {
            if (*src == ';') {
                entity_buf[entity_idx++] = *src++;
                in_entity = 0;
                
                // 解析常见的 HTML 实体
                if (strcmp(entity_buf, "&amp;") == 0) {
                    *dst++ = '&';
                }
                else if (strcmp(entity_buf, "&lt;") == 0) {
                    *dst++ = '<';
                }
                else if (strcmp(entity_buf, "&gt;") == 0) {
                    *dst++ = '>';
                }
                else if (strcmp(entity_buf, "&quot;") == 0) {
                    *dst++ = '"';
                }
                else if (strcmp(entity_buf, "&apos;") == 0) {
                    *dst++ = '\'';
                }
                else if (strcmp(entity_buf, "&nbsp;") == 0) {
                    *dst++ = ' ';
                }
                else if (strncmp(entity_buf, "&#", 2) == 0) {
                    // 数字实体 &#xxx; 或 &#xXXX;
                    int code = 0;
                    if (entity_buf[2] == 'x' || entity_buf[2] == 'X') {
                        sscanf(entity_buf + 3, "%x", &code);
                    } else {
                        sscanf(entity_buf + 2, "%d", &code);
                    }
                    if (code > 0 && code < 128) {
                        *dst++ = (char)code;
                    }
                }
                // 其他未知实体直接忽略
            }
            else if (entity_idx < (int)sizeof(entity_buf) - 1) {
                entity_buf[entity_idx++] = *src++;
            }
            else {
                // 实体太长，可能不是有效实体，输出原内容
                in_entity = 0;
                for (int i = 0; i < entity_idx && dst < dst_end; i++) {
                    *dst++ = entity_buf[i];
                }
            }
        }
        else {
            // 普通字符直接复制
            *dst++ = *src++;
        }
    }

    *dst = '\0';

    // 去除多余的空白字符
    divoom_google_calendar_trim_text(out_text);

    return 0;
}

/**
 * @brief 去除文本首尾空白，合并连续空白字符
 */
static void divoom_google_calendar_trim_text(char *text)
{
    if (!text || !*text) return;

    char *src = text;
    char *dst = text;
    int last_was_space = 1;  // 跳过开头的空白

    // 跳过开头空白
    while (*src && (*src == ' ' || *src == '\t' || *src == '\n' || *src == '\r')) {
        src++;
    }

    // 处理中间内容，合并连续空白
    while (*src) {
        if (*src == ' ' || *src == '\t') {
            if (!last_was_space) {
                *dst++ = ' ';
                last_was_space = 1;
            }
        }
        else if (*src == '\n' || *src == '\r') {
            if (!last_was_space) {
                *dst++ = '\n';
                last_was_space = 1;
            }
        }
        else {
            *dst++ = *src;
            last_was_space = 0;
        }
        src++;
    }

    // 去除尾部空白
    while (dst > text && (*(dst-1) == ' ' || *(dst-1) == '\n')) {
        dst--;
    }

    *dst = '\0';
}

/**
 * @brief 从 <a> 标签中提取链接文本
 * 例如: <a href="...">BOA</a> 提取出 "BOA"
 */
int divoom_google_calendar_extract_link_text(const char *html_str, char *out_text, int out_size)
{
    // 直接使用 html_to_text 函数即可
    return divoom_google_calendar_html_to_text(html_str, out_text, out_size);
}

/*********************************************************************
Function:         divoom_google_calendar_get_active_lvgl_event_info
Description:      得到LVGL显示的信息
Input:            data
Return:           
Author:           albert
**********************************************************************/

int divoom_google_calendar_get_active_lvgl_event_info(void *data)
{
    int ret = 1;
    Pdivoom_lvgl_calendar_event_info pevent_info = (Pdivoom_lvgl_calendar_event_info)data;
    time_t cur_time; 
    struct tm cur_tm;
    
    if(divoom_system_try_lock(pdivoom_google_calendar_cal_handle->lock_handle))
    {
        return 0;
    }
    if(pdivoom_google_calendar_cal_handle->prunning_event)
    {
        memcpy(pevent_info->bg_color, pdivoom_google_calendar_cal_handle->prunning_event->bg_color, 3);
        memcpy(pevent_info->fr_color, pdivoom_google_calendar_cal_handle->prunning_event->fr_color, 3);
        cur_time = pdivoom_google_calendar_cal_handle->prunning_event->start_utc;
        localtime_r(&cur_time, &cur_tm);
        pevent_info->start_time =  cur_tm.tm_hour *60 + cur_tm.tm_min;
        cur_time = pdivoom_google_calendar_cal_handle->prunning_event->end_utc;
        localtime_r(&cur_time, &cur_tm);
        pevent_info->end_time =  cur_tm.tm_hour *60 + cur_tm.tm_min;
        if(pevent_info->end_time == 0)
        {
            pevent_info->end_time = 23*60 +59;
        }
        pevent_info->title = divoom_app_com_copy_string( pdivoom_google_calendar_cal_handle->prunning_event->title, -1);
        pevent_info->msg_info = divoom_system_malloc(2048);
        divoom_google_calendar_extract_link_text(pdivoom_google_calendar_cal_handle->prunning_event->desc, pevent_info->msg_info, 2048);
    }
    else
    {
        ret = 0;
    }
    divoom_system_free_lock(pdivoom_google_calendar_cal_handle->lock_handle);
    return ret;

}

int divoom_google_calendar_compare_handle(void *item1, void *item2)
{
    return item1 == item2?0:1;
}
int divoom_google_calendar_check_run_flag(void)
{
    if(pdivoom_google_calendar_cal_handle->google_token && strlen(pdivoom_google_calendar_cal_handle->google_token))
    {
        #if 0
        /* 原先只看通知/菜单，通知全关时开机和表盘刷新都不会打 Google，本地一直 empty。 */
        if(pdivoom_google_calendar_cal_handle->pop_flag || pdivoom_google_calendar_cal_handle->event_flag || pdivoom_google_calendar_cal_handle->menu_flag || pdivoom_google_calendar_cal_handle->mail_pop_flag)
        {
            return 1;
        }
        #endif
        /* 增加 watch_refresh_flag，表盘刷新和开机 token 到位后也能后台拉事件，对齐 Outlook。 */
        if(pdivoom_google_calendar_cal_handle->pop_flag || pdivoom_google_calendar_cal_handle->event_flag || pdivoom_google_calendar_cal_handle->menu_flag || pdivoom_google_calendar_cal_handle->mail_pop_flag || pdivoom_google_calendar_cal_handle->watch_refresh_flag)
        {
            return 1;
        }
    }
    return 0;
}

void *divoom_google_calendar_main_task(void *para)
{
    int i, j, cal_num, event_num, first_flag = 1, cur_mon = -1;
    uint32 token_generation;
    time_t cur_time;
    Pdivoom_google_calendar_cal_item pcal_item;
    Pdivoom_google_calendar_event_item pevent_item;
    struct tm cur_tm;

    while(1)
    {
        //一分钟查询一次
        cur_time = time(0);
        if(divoom_main_get_server_con_status())
        {
            divoom_system_get_lock(pdivoom_google_calendar_cal_handle->lock_handle);
            if(pdivoom_google_calendar_cal_handle->token_refresh_pending)
            {
                if((uint32)(divoom_system_get_tickms() - pdivoom_google_calendar_cal_handle->token_refresh_tick) >= 30000U)
                {
                    divoom_google_calendar_update_goole_token();
                }
                goto calendar_sync_done;
            }
            token_generation = pdivoom_google_calendar_cal_handle->token_generation;
            if(divoom_google_calendar_check_run_flag())
            {
                localtime_r(&cur_time, &cur_tm);
                if(cur_mon != cur_tm.tm_mon)
                {
                    //如果月发生变化，将清理更新时间
                    cur_mon = cur_tm.tm_mon;
                    pdivoom_google_calendar_cal_handle->event_request_time = 0;

                    for(int i = 0; i < pdivoom_google_calendar_cal_handle->calendar_num; i++)
                    {
                        pdivoom_google_calendar_cal_handle->calendar_list[i].last_update_time[0] = 0;
                    }
                }
                if(pdivoom_google_calendar_cal_handle->color_request_time + DIVOOM_GOOGLE_CALENDAR_REQUEST_COLOR_LIST_TIME < cur_time)
                {                
                    divoom_google_calendar_get_all_color_info();                
                    if(token_generation != pdivoom_google_calendar_cal_handle->token_generation)
                    {
                        goto calendar_sync_done;
                    }
                    pdivoom_google_calendar_cal_handle->color_request_time = cur_time;
                }
                if(pdivoom_google_calendar_cal_handle->calendar_request_time + DIVOOM_GOOGLE_CALENDAR_REQUEST_CAL_LIST_TIME < cur_time)
                {
                    divoom_google_calendar_request_cal_list();  
                    if(token_generation != pdivoom_google_calendar_cal_handle->token_generation)
                    {
                        goto calendar_sync_done;
                    }
                    
                    pdivoom_google_calendar_cal_handle->calendar_request_time = cur_time;
                }

                //国内二分钟更新一次，国外30S更新一次
                if(divoom_main_get_server_type())
                {
                    if(pdivoom_google_calendar_cal_handle->event_request_time + DIVOOM_GOOGLE_CALENDAR_REQUEST_EVENT_LIST_TIME < cur_time)
                    {              
                        divoom_google_calendar_request_all_cal_event();
                        if(token_generation != pdivoom_google_calendar_cal_handle->token_generation)
                        {
                            goto calendar_sync_done;
                        }
                        
                        pdivoom_google_calendar_cal_handle->event_request_time = cur_time;
                        /* 拉完本次事件后清标志，避免通知全关时一直 30s 打 Google。 */
                        pdivoom_google_calendar_cal_handle->watch_refresh_flag = 0;
                    }
                }
                else
                {
                    if(pdivoom_google_calendar_cal_handle->event_request_time + DIVOOM_GOOGLE_CALENDAR_REQUEST_EVENT_LIST_TIME * 4 < cur_time)
                    {              
                        divoom_google_calendar_request_all_cal_event();
                        if(token_generation != pdivoom_google_calendar_cal_handle->token_generation)
                        {
                            goto calendar_sync_done;
                        }
                        
                        pdivoom_google_calendar_cal_handle->event_request_time = cur_time;
                        /* 拉完本次事件后清标志，避免通知全关时一直 120s 打 Google。 */
                        pdivoom_google_calendar_cal_handle->watch_refresh_flag = 0;
                    }
                }
                
            }
            else
            {
                divoom_system_free_lock(pdivoom_google_calendar_cal_handle->lock_handle);
                divoom_system_get_sem(pdivoom_google_calendar_cal_handle->run_sem);
                continue;
            }
calendar_sync_done:
            divoom_system_free_lock(pdivoom_google_calendar_cal_handle->lock_handle);
        }
        if(pdivoom_google_calendar_cal_handle->calendar_num)
        {
            divoom_system_get_lock(pdivoom_google_calendar_cal_handle->lock_handle);
            for(i = 0; i < pdivoom_google_calendar_cal_handle->calendar_num; i ++)
            {
                pcal_item = &pdivoom_google_calendar_cal_handle->calendar_list[i];
                for(j = 0; j < pcal_item->event_num; j ++)
                {
                    pevent_item = &pcal_item->event_list[j];
                    switch(pevent_item->status)
                    {
                        case DIVOOM_GOOGLE_CALENDAR_STATUS_IDLE:
                        case DIVOOM_GOOGLE_CALENDAR_STATUS_NOTICE:
                            if(pevent_item->next_notice_time && pevent_item->next_notice_time < cur_time)
                            {
                                //抛出提醒事件
                                pevent_item->status = DIVOOM_GOOGLE_CALENDAR_STATUS_NOTICE;
                                divoom_google_calendar_format_event_notice_time(pevent_item);
                                divoom_google_calendar_send_event_notice_message(pevent_item, DIVOOM_GOOGLE_CALENDAR_STATUS_NOTICE);
                            }
                            else if(pevent_item->start_utc < cur_time && pevent_item->end_utc > cur_time)
                            {
                                if(divoom_google_calendar_is_event_dismissed(pevent_item))
                                {
                                    pevent_item->status = DIVOOM_GOOGLE_CALENDAR_STATUS_PASS;
                                }
                                else
                                {
                                //事件激活
                                pevent_item->status = DIVOOM_GOOGLE_CALENDAR_STATUS_ACTIVE;
                                divoom_google_calendar_send_event_notice_message(pevent_item, DIVOOM_GOOGLE_CALENDAR_STATUS_ACTIVE);
                                }
                            }
                            else if(pevent_item->end_utc < cur_time)
                            {
                                pevent_item->status = DIVOOM_GOOGLE_CALENDAR_STATUS_PASS;
                            }
                            break;
                            
                        case DIVOOM_GOOGLE_CALENDAR_STATUS_PASS:
                        case DIVOOM_GOOGLE_CALENDAR_STATUS_ACTIVE:
                            if(pevent_item->end_utc < cur_time)
                            {
                                pevent_item->user_dismissed = 0;
                                //将这个事件弹出,通知事件结束
                                if(pevent_item->status == DIVOOM_GOOGLE_CALENDAR_STATUS_ACTIVE)
                                {
                                    divoom_google_calendar_send_event_notice_message(pevent_item, DIVOOM_GOOGLE_CALENDAR_STATUS_IDLE);
                                }
                                pevent_item->status = DIVOOM_GOOGLE_CALENDAR_STATUS_IDLE;
                            }
                            else if(pevent_item->status == DIVOOM_GOOGLE_CALENDAR_STATUS_PASS
                                && pevent_item->start_utc < cur_time
                                && pevent_item->end_utc > cur_time
                                && pdivoom_google_calendar_cal_handle->event_flag
                                && !divoom_google_calendar_is_event_dismissed(pevent_item))
                            {
                                pevent_item->status = DIVOOM_GOOGLE_CALENDAR_STATUS_ACTIVE;
                                divoom_google_calendar_send_event_notice_message(pevent_item, DIVOOM_GOOGLE_CALENDAR_STATUS_ACTIVE);
                            }
                            break;
                    }
                }
            }
            divoom_system_free_lock(pdivoom_google_calendar_cal_handle->lock_handle);
        }
        divoom_system_delay_ms(1000);
    }

    return NULL;
}

void divoom_google_calendar_load_config(void)
{
    divoom_cJSON *root,*item;
    char *file_buf;

    if (pdivoom_google_calendar_cal_handle == NULL)
    {
        return;
    }
    file_buf = divoom_app_com_read_file(DIVOOM_GOOGLE_CALENDAR_CONFIG, NULL);
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

    pdivoom_google_calendar_cal_handle->pop_flag = divoom_cJSON_GetObjectInt(root, "pop_flag");
    pdivoom_google_calendar_cal_handle->event_flag = divoom_cJSON_GetObjectInt(root, "event_flag");
	item = divoom_cJSON_GetObjectItem(root, "google_token");
    if(item && item->valuestring)
    {
        pdivoom_google_calendar_cal_handle->google_token = divoom_app_com_copy_string(item->valuestring, -1);
    }
    return ;
}



void divoom_google_calendar_save_config(void)
{
    char *json_info = NULL;
    divoom_cJSON *root;
	root = divoom_cJSON_CreateObject();

    if(root == NULL)
    {
        return;
    }
    
    
	divoom_cJSON_AddNumberToObject(root, "pop_flag", pdivoom_google_calendar_cal_handle->pop_flag);
	divoom_cJSON_AddNumberToObject(root, "event_flag", pdivoom_google_calendar_cal_handle->event_flag);
    if(pdivoom_google_calendar_cal_handle->google_token)
    {
        divoom_cJSON_AddStringToObject(root , "google_token", pdivoom_google_calendar_cal_handle->google_token);
    }
    json_info = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);
    divoom_app_com_write_file(DIVOOM_GOOGLE_CALENDAR_CONFIG, (uint8 *)json_info, strlen(json_info));
    divoom_system_free(json_info);
}


int divoom_google_calendar_get_color_value_compare(void *data1, void *data2)
{
    Pdivoom_google_calendar_color_item pcolor_item1 = (Pdivoom_google_calendar_color_item)data1;
    Pdivoom_google_calendar_color_item pcolor_item2 = (Pdivoom_google_calendar_color_item)data2;


    return (pcolor_item1->type == pcolor_item2->type && strcmp(pcolor_item1->color_id, pcolor_item2->color_id)== 0)?0:1;
}

/*********************************************************************
Function:         divoom_google_calendar_get_color_value
Description:      通过颜色ID得到ID
Input:            none
Return:           
Author:           albert
**********************************************************************/

int divoom_google_calendar_get_color_value(int type, char *color_id, uint8 *bg_color, uint8 *fr_color)
{
    divoom_google_calendar_color_item find_info;
    Pdivoom_google_calendar_color_item pcolor_item;

    find_info.type = type;
    strcpy(find_info.color_id ,color_id);

    
    pcolor_item = divoom_queue_get_cmp(pdivoom_google_calendar_cal_handle->color_list, &find_info, divoom_google_calendar_get_color_value_compare);
    if(pcolor_item == NULL)
    {
        divoom_print("it fails to get color: %d, %s, %d\n", type, color_id, divoom_queue_items(pdivoom_google_calendar_cal_handle->color_list));
        return 0;
    }
    memcpy(bg_color, pcolor_item->bg_color, 3);
    memcpy(fr_color, pcolor_item->fr_color, 3);
    return 1;
    
}

int divoom_google_calendar_remove_popup_notice_message_compare(void *item1, void *item2)
{
    return item1 == item2 ?0:1;
}

int divoom_google_calendar_add_popup_notice_message_callback(void *para, int result)
{
    Pdivoom_google_calendar_event_item pevent_item = (Pdivoom_google_calendar_event_item)para;

    if(pevent_item)
    {
        //可能更新了，这个事件就不存在了
        for(int i = 0; i < pdivoom_google_calendar_cal_handle->calendar_num; i++)
        {
            for(int j = 0; j < pdivoom_google_calendar_cal_handle->calendar_list[i].event_num; j++)  // 修复: i++ -> j++
            {
                if(&pdivoom_google_calendar_cal_handle->calendar_list[i].event_list[j] == pevent_item)
                {
                    pevent_item->lvgl_obj = NULL;
                    break;  // 找到后应该跳出
                }
            }
        }
        divoom_queue_pop_cmp(pdivoom_google_calendar_cal_handle->event_msg_queue, para, divoom_google_calendar_remove_popup_notice_message_compare);
        divoom_print("item info: %d\n", divoom_queue_items(pdivoom_google_calendar_cal_handle->event_msg_queue));
    }
    return 0;
}

static void divoom_google_calendar_popup_fit_icon(void *image_obj)
{
    lv_obj_t *img = (lv_obj_t *)image_obj;
    lv_img_header_t header = {0};
    const void *src;
    int scale_w, scale_h, scale;

    if(img == NULL)
    {
        return;
    }

    src = lv_img_get_src(img);
    if(src == NULL || lv_img_decoder_get_info(src, &header) != LV_RES_OK)
    {
        return;
    }
    if(header.w <= 0 || header.h <= 0)
    {
        return;
    }

    scale_w = DIVOOM_GOOGLE_CALENDAR_POPUP_ICON_SZ * LV_IMG_ZOOM_NONE / header.w;
    scale_h = DIVOOM_GOOGLE_CALENDAR_POPUP_ICON_SZ * LV_IMG_ZOOM_NONE / header.h;
    scale = scale_w < scale_h ? scale_w : scale_h;
    if(scale < 1)
    {
        scale = 1;
    }

    lv_img_set_size_mode(img, LV_IMG_SIZE_MODE_REAL);
    lv_img_set_zoom(img, (uint16_t)scale);
    lv_img_set_pivot(img, header.w / 2, header.h / 2);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
}

/*********************************************************************
Function:         divoom_google_calendar_send_event_notice_message
Description:      通知系统事件触发
Input:            data
Return:           
Author:           albert
**********************************************************************/

void divoom_google_calendar_add_popup_notice_message(Pdivoom_google_calendar_event_item pevent_item)
{    
    int bg_value, font_value;
    struct tm cur_tm, end_tm;
    time_t cur_time =pevent_item->start_utc;
    void *icon_cont = NULL;
    void *image_obj = NULL;
    
    if(divoom_queue_get_cmp(pdivoom_google_calendar_cal_handle->event_msg_queue, pevent_item, divoom_google_calendar_remove_popup_notice_message_compare))
    {
        divoom_print("pass the event!\n");
        return;
    }

    
    localtime_r(&cur_time, &cur_tm);
    cur_time = pevent_item->end_utc;
    localtime_r(&cur_time, &end_tm);
    
    int text_x, text_w, text_y0, icon_y, text_block_h;
    bg_value = (pevent_item->bg_color[0] << 16) + (pevent_item->bg_color[1] << 8) + pevent_item->bg_color[2]; 
    void *lvgl_obj = divoom_system_msg_notice_add(DIVOOM_GOOGLE_CALENDAR_POPUP_W, DIVOOM_GOOGLE_CALENDAR_POPUP_H,
            divoom_google_calendar_add_popup_notice_message_callback, pevent_item);
    if (lvgl_obj == NULL)
    {
        return;
    }
    pevent_item->lvgl_obj = lvgl_obj;
    divoom_queue_push(pdivoom_google_calendar_cal_handle->event_msg_queue, pevent_item);

    text_x = DIVOOM_GOOGLE_CALENDAR_POPUP_ICON_X + DIVOOM_GOOGLE_CALENDAR_POPUP_ICON_SZ
           + DIVOOM_GOOGLE_CALENDAR_POPUP_TEXT_GAP;
    text_w = DIVOOM_GOOGLE_CALENDAR_POPUP_W - text_x - DIVOOM_GOOGLE_CALENDAR_POPUP_PAD;
    text_block_h = DIVOOM_GOOGLE_CALENDAR_POPUP_TIME_H + DIVOOM_GOOGLE_CALENDAR_POPUP_TEXT_ROW_GAP + DIVOOM_GOOGLE_CALENDAR_POPUP_TITLE_H;
    text_y0 = (DIVOOM_GOOGLE_CALENDAR_POPUP_H - text_block_h) / 2;
    icon_y = (DIVOOM_GOOGLE_CALENDAR_POPUP_H - DIVOOM_GOOGLE_CALENDAR_POPUP_ICON_SZ) / 2;
    
    divoom_get_lvgl_lock();
	lv_obj_set_style_bg_color(lvgl_obj, lv_color_hex(bg_value), 0);
	lv_obj_set_style_bg_opa(lvgl_obj, LV_OPA_COVER, 0);
	lv_obj_set_style_radius(lvgl_obj, DIVOOM_GOOGLE_CALENDAR_POPUP_RADIUS, 0);
	lv_obj_set_style_radius(lvgl_obj, DIVOOM_GOOGLE_CALENDAR_POPUP_RADIUS, LV_STATE_FOCUSED);
	lv_obj_set_style_radius(lvgl_obj, DIVOOM_GOOGLE_CALENDAR_POPUP_RADIUS, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(lvgl_obj, 0, 0);
	lv_obj_set_style_border_width(lvgl_obj, 0, 0);
	lv_obj_set_style_border_width(lvgl_obj, 0, LV_STATE_FOCUSED);
	lv_obj_set_style_border_width(lvgl_obj, 0, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_outline_width(lvgl_obj, 0, 0);
	lv_obj_clear_flag(lvgl_obj, LV_OBJ_FLAG_SCROLLABLE);

    icon_cont = lv_obj_create(lvgl_obj);
    lv_obj_set_pos(icon_cont, DIVOOM_GOOGLE_CALENDAR_POPUP_ICON_X, icon_y);
    lv_obj_set_size(icon_cont, DIVOOM_GOOGLE_CALENDAR_POPUP_ICON_SZ, DIVOOM_GOOGLE_CALENDAR_POPUP_ICON_SZ);
    lv_obj_set_style_bg_opa(icon_cont, LV_OPA_0, 0);
    lv_obj_set_style_border_width(icon_cont, 0, 0);
    lv_obj_set_style_pad_all(icon_cont, 0, 0);
    lv_obj_clear_flag(icon_cont, LV_OBJ_FLAG_SCROLLABLE);

    image_obj = lv_img_create(icon_cont);
	divoom_menu_control_lv_img_set_src(image_obj, "82/goole_calendar_log.png");
    divoom_google_calendar_popup_fit_icon(image_obj);
    void *lable_obj = lv_label_create(icon_cont);
	lv_label_set_text(lable_obj, "");
	lv_obj_set_size(lable_obj, 40, 40);
    lv_obj_center(lable_obj);
	lv_obj_set_style_text_color(lable_obj, lv_color_hex(0x1A73E8), 0);
	lv_obj_set_style_bg_color(lable_obj, lv_color_hex(bg_value), 0);
	lv_obj_set_style_text_font(lable_obj, divoom_menu_control_get_font_info(24, 0), 0);
	lv_obj_set_style_text_align(lable_obj,LV_TEXT_ALIGN_CENTER, 0);    
	lv_label_set_text_fmt(lable_obj, "%d", cur_tm.tm_mday);
    
    
    font_value = (pevent_item->fr_color[0] << 16) + (pevent_item->fr_color[1] << 8) + pevent_item->fr_color[2]; 
    lable_obj = lv_label_create(lvgl_obj);
	lv_label_set_text(lable_obj, "");
	lv_obj_set_pos(lable_obj, text_x, text_y0);
	lv_obj_set_size(lable_obj, text_w, DIVOOM_GOOGLE_CALENDAR_POPUP_TIME_H);
	lv_obj_set_style_text_color(lable_obj, lv_color_hex(font_value), 0);
	lv_obj_set_style_bg_color(lable_obj, lv_color_hex(bg_value), 0);
	lv_obj_set_style_bg_opa(lable_obj, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(lable_obj, 0, 0);
	lv_obj_set_style_text_font(lable_obj, divoom_menu_control_get_font_info(20, 0), 0);
	lv_obj_set_style_text_align(lable_obj,LV_TEXT_ALIGN_LEFT, 0);    
	lv_label_set_text_fmt(lable_obj, "%d:%02d-%d:%02d", cur_tm.tm_hour, cur_tm.tm_min,end_tm.tm_hour, end_tm.tm_min);

    
    
    lable_obj = lv_label_create(lvgl_obj);
	lv_label_set_text(lable_obj, "");
	lv_obj_set_size(lable_obj, text_w, DIVOOM_GOOGLE_CALENDAR_POPUP_TITLE_H);
	lv_obj_set_pos(lable_obj, text_x, text_y0 + DIVOOM_GOOGLE_CALENDAR_POPUP_TIME_H + DIVOOM_GOOGLE_CALENDAR_POPUP_TEXT_ROW_GAP);
	lv_obj_set_style_text_color(lable_obj, lv_color_hex(font_value), 0);
	lv_obj_set_style_bg_color(lable_obj, lv_color_hex(bg_value), 0);
	lv_obj_set_style_bg_opa(lable_obj, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(lable_obj, 0, 0);
	lv_obj_set_style_text_font(lable_obj, divoom_menu_control_get_font_info(24, 0), 0);
	lv_obj_set_style_text_align(lable_obj,LV_TEXT_ALIGN_LEFT, 0);
	lv_label_set_text(lable_obj, pevent_item->title ? pevent_item->title : " ");
	lv_label_set_long_mode(lable_obj, LV_LABEL_LONG_DOT);
    divoom_free_lvgl_lock();
    
    
}

/*********************************************************************
Function:         divoom_google_calendar_send_event_notice_message
Description:      通知系统事件触发
Input:            data
Return:           
Author:           albert
**********************************************************************/

void divoom_google_calendar_send_event_notice_message(void *data, int status)
{    
    Pdivoom_google_calendar_event_item pevent_item = (Pdivoom_google_calendar_event_item)data;
    

    if(status == DIVOOM_GOOGLE_CALENDAR_STATUS_NOTICE)
    {
        //这里增加一个系统消息
        if(pdivoom_google_calendar_cal_handle->pop_flag)
        {
            divoom_print("msg : %s\n", pevent_item->title);
            divoom_google_calendar_add_popup_notice_message(pevent_item);
        }
    }
    else if(status == DIVOOM_GOOGLE_CALENDAR_STATUS_ACTIVE)
    {
        if(pdivoom_google_calendar_cal_handle->event_flag)
        {
            divoom_print("start : %s\n", pevent_item->title);
            divoom_google_calendar_send_event_notice(pevent_item);
            if(pevent_item->lvgl_obj)
            {
                divoom_system_msg_notice_clear_one(pevent_item->lvgl_obj, 1);
                pevent_item->lvgl_obj = NULL;
            }
        }
    }
    else if(status == DIVOOM_GOOGLE_CALENDAR_STATUS_IDLE)
    {
        if(pdivoom_google_calendar_cal_handle->event_flag)
        {
            if(pevent_item)
            {
                divoom_print("end : %s\n", pevent_item->title);
            }
            divoom_google_calendar_send_event_notice(NULL);
        }
    }
    
}

int divoom_google_calendar_load_calenday_current_month_compare(void *item1, void *item2)
{
    Pdivoom_lvgl_calendar_event_info plvgl_event_item1 = (Pdivoom_lvgl_calendar_event_info)item1;
    Pdivoom_lvgl_calendar_event_info plvgl_event_item2 = (Pdivoom_lvgl_calendar_event_info)item2;

    if(plvgl_event_item1->start_time > plvgl_event_item2->start_time)
    {
        return 1;
    }
    else if(plvgl_event_item1->start_time == plvgl_event_item2->start_time)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}
void divoom_lvgl_calendar_load_calenday_event_free_item(void *item);

/*********************************************************************
Function:         divoom_google_calendar_get_current_day_to_do_list
Description:      表盘获取当天的待办事项
Input:            data
Return:           
Author:           wangft
**********************************************************************/

int divoom_google_calendar_get_current_day_to_do_list(void *pdata)
{
    static void *month_event_queue[42] = {0};    // 当月的事件队列
    Pdivoom_clock_control_disp_data pdisp_data = (Pdivoom_clock_control_disp_data)pdata;
    Pdivoom_lvgl_calendar_event_info plvgl_event_item;
    int today_index = 0;
    int event_count = 0;
    struct tm cur_tm;
    time_t cur_time;
    int ret, index = 0, i;

    // 初始化显示数据结构
    divoom_clock_control_disp_data_deinit(pdisp_data);
	divoom_clock_control_disp_data_init(pdisp_data);
    pdisp_data->num = 0;
    pdisp_data->update = 0;

    // 加载当前月的所有事件
    ret = divoom_google_calendar_load_calenday_current_month(month_event_queue);
    if (ret != 1) 
	{
        // 加载失败，设置更新标志为0表示无数据
        pdisp_data->update = 0;
        return 0;
    }

    // 计算当天在月历中的索引位置
    cur_time = time(0);
    localtime_r(&cur_time, &cur_tm);
    
    // 计算当天在42天数组中的位置
    // 公式：当天索引 = (当前日期 - 1) + 当月第一天是周几
	
	//先计算当月第一天是星期几
    time_t first_day_time;
    struct tm first_day_tm;
    
    // 计算当月第一天的时间
    first_day_time = cur_time - (cur_tm.tm_mday - 1) * 24*3600 - cur_tm.tm_hour*3600 - cur_tm.tm_min*60 - cur_tm.tm_sec;
    localtime_r(&first_day_time, &first_day_tm);
    
    // 计算当月第一天是周几，然后计算当天在42天数组中的位置
    today_index = cur_tm.tm_mday - 1 + first_day_tm.tm_wday;
    
    // 防止数组越界
    if (today_index >= 42) 
	{
        today_index = 0;
    }

    // 检查当天是否有事件队列
    if (month_event_queue[today_index] == NULL) 
	{
        // 当天没有事件
		for (i = 0; i < 5; i++)
		{
			DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).bg_color[0] = 0XA4;
			DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).bg_color[1] = 0XBD;
			DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).bg_color[2] = 0XFC;
			strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).pdisp_data[0], "Get to-do list...");
			strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).pdisp_data[1], "Get to-do list...");
			DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).num = 2;
		}
        pdisp_data->num = i;
        pdisp_data->update = 1;
        return 1;
    }

    // 获取当天事件数量
    event_count = divoom_queue_items(month_event_queue[today_index]);
    if (event_count <= 0) 
	{
        // 当天没有事件
		for (i = 0; i < 5; i++)
		{
			DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).bg_color[0] = 0XA4;
			DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).bg_color[1] = 0XBD;
			DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).bg_color[2] = 0XFC;
			strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).pdisp_data[0], "Get to-do list...");
			strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).pdisp_data[1], "Get to-do list...");
			DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).num = 2;
		}
        pdisp_data->num = i;
        pdisp_data->update = 1;
        return 1;
    }

    // 限制最大显示数量，避免超出数组边界
    if (event_count > DIVOOM_CLOCK_CONTROL_MAX_DISP_NUM) 
	{
        event_count = DIVOOM_CLOCK_CONTROL_MAX_DISP_NUM;
    }

    // 设置显示数据
    pdisp_data->num = event_count;
    pdisp_data->update = 1;

    // 遍历当天的事件，填充显示数据
    for (i = 0; i < event_count; i++) 
	{
        plvgl_event_item = (Pdivoom_lvgl_calendar_event_info)divoom_queue_get_index_item(month_event_queue[today_index], i);
        if (!plvgl_event_item) 
		{
            continue;
        }

        // 初始化显示项
        memset(divoom_clock_control_disp_data_get_item(pdisp_data, i), 0, sizeof(divoom_clock_control_disp_array_item));
        // 设置基本属性
        DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).pdisp_data_roll_flag[0] = 1;
        DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).pdisp_data_roll_flag[3] = 2;

		// 格式化时间信息：HH:MM-HH:MM
        int start_hour = plvgl_event_item->start_time / 60;
        int start_min = plvgl_event_item->start_time % 60;
        int end_hour = plvgl_event_item->end_time / 60;
        int end_min = plvgl_event_item->end_time % 60;

		//设置颜色
		memcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).bg_color, plvgl_event_item->bg_color, sizeof(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).bg_color));

        // 设置事件标题
        if (plvgl_event_item->title && strlen(plvgl_event_item->title) > 0) 
		{
			sprintf(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).pdisp_data[0], "%02d:%02d %s", start_hour, start_min, plvgl_event_item->title);
			sprintf(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).pdisp_data[1], "%s", plvgl_event_item->title);
        } 
		else 
		{
            strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).pdisp_data[0], "no title");
			strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).pdisp_data[1], "no title");
        }

        sprintf(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).pdisp_data[2], "%02d:%02d-%02d:%02d", start_hour, start_min, end_hour, end_min);
        // 设置事件描述或时间信息
        if (plvgl_event_item->msg_info && strlen(plvgl_event_item->msg_info) > 0) 
		{
            strncpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).pdisp_data[3], plvgl_event_item->msg_info, DIVOOM_CLOCK_CONTROL_DISPLAY_MAX_LEN - 1);
        } 
		DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).num = 4;
    }
	return 1;
}

#define DIVOOM_GOOGLE_CALENDAR_TODAY_WATCH_MAX_ROWS DIVOOM_CALENDAR_TODAY_WATCH_MAX_ROWS

int divoom_google_calendar_watch_cache_is_stale(const char *url_info, int cached_num, int clock_id)
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
		return cached_num < DIVOOM_GOOGLE_CALENDAR_NEXT_EVENT_WATCH_DATA_NUM;
	if (strstr(url_info, "ThisWeekWatch") != NULL)
		return cached_num < DIVOOM_GOOGLE_CALENDAR_WEEK_WATCH_DATA_NUM;
	return 0;
}

static void divoom_google_calendar_blend_watch_beige(uint8 *rgb)
{
	if (rgb == NULL)
	{
		return;
	}
	rgb[0] = (uint8)((rgb[0] * 55 + 0xE5 * 45) / 100);
	rgb[1] = (uint8)((rgb[1] * 55 + 0xD9 * 45) / 100);
	rgb[2] = (uint8)((rgb[2] * 55 + 0xC5 * 45) / 100);
}

static void divoom_google_calendar_soften_watch_color(Pdivoom_google_calendar_event_item pevent_item, uint8 *out_color)
{
	uint8 src_color[3];
	uint8 fr_color[3];

	if (pevent_item == NULL || out_color == NULL)
	{
		return;
	}

	if (pevent_item->color_id[0]
		&& divoom_google_calendar_get_color_value(1, pevent_item->color_id, src_color, fr_color))
	{
		memcpy(out_color, src_color, 3);
	}
	else
	{
		memcpy(out_color, pevent_item->bg_color, 3);
	}
	divoom_google_calendar_blend_watch_beige(out_color);
}

static int divoom_google_calendar_pack_watch_color(uint8 *bg_color)
{
	if (bg_color == NULL)
	{
		return 0;
	}
	return ((int)bg_color[0] << 16) | ((int)bg_color[1] << 8) | (int)bg_color[2];
}

static void divoom_google_calendar_format_today_event_time(Pdivoom_google_calendar_event_item pevent_item, char *buf, int buf_size)
{
	struct tm start_tm, end_tm;
	time_t start_local, end_local;

	if (pevent_item == NULL || buf == NULL || buf_size <= 0)
	{
		return;
	}
	start_local = pevent_item->start_utc;
	end_local = pevent_item->end_utc;
	localtime_r(&start_local, &start_tm);
	localtime_r(&end_local, &end_tm);
	sprintf(buf, "%02d:%02d-%02d:%02d",
		start_tm.tm_hour, start_tm.tm_min, end_tm.tm_hour, end_tm.tm_min);
}

#define DIVOOM_CALENDAR_NEXT_EVENT_TIME_EMPTY       "--:-- - --:--"
#define DIVOOM_CALENDAR_NEXT_EVENT_COUNTDOWN_EMPTY  "--:--:--"

static void divoom_google_calendar_fill_today_event_watch_empty(Pdivoom_clock_control_disp_data pdisp_data, divoom_calendar_today_watch_layout layout, int logged_in)
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

/*********************************************************************
Function:         divoom_google_calendar_get_today_event_watch_data
Description:      表盘获取今日日程列表（最多5条+剩余活动数）
Input:            pdata
Return:           0:失败 1:成功
Author:           cursor
**********************************************************************/

static int divoom_google_calendar_watch_try_lock(void)
{
	int retry;

	if (pdivoom_google_calendar_cal_handle == NULL)
		return 1;
	if (!divoom_system_try_lock(pdivoom_google_calendar_cal_handle->lock_handle))
		return 0;
	for (retry = 0; retry < 50; retry++)
	{
		divoom_system_delay_ms(10);
		if (!divoom_system_try_lock(pdivoom_google_calendar_cal_handle->lock_handle))
			return 0;
	}
	return 1;
}

int divoom_google_calendar_get_this_week_watch_data(void *pdata)
{
	Pdivoom_clock_control_disp_data pdisp_data = (Pdivoom_clock_control_disp_data)pdata;
	Pdivoom_google_calendar_cal_item pcal_item;
	Pdivoom_google_calendar_event_item pevent_item;
	time_t now, day_start, week_start;
	struct tm cur_tm;
	int i, j, d, day_counts[7];
	static time_t last_watch_sync_req = 0;

	pdisp_data->num = 0;
	pdisp_data->update = 0;
	memset(day_counts, 0, sizeof(day_counts));

	if (pdivoom_google_calendar_cal_handle == NULL)
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
		divoom_google_calendar_request_watch_refresh();
	}

	if (divoom_google_calendar_watch_try_lock())
	{
		return 0;
	}

	divoom_clock_control_disp_data_deinit(pdisp_data);
	divoom_clock_control_disp_data_init(pdisp_data);
	localtime_r(&now, &cur_tm);
	day_start = now - cur_tm.tm_hour * 3600 - cur_tm.tm_min * 60 - cur_tm.tm_sec;
	week_start = day_start - ((cur_tm.tm_wday + 6) % 7) * 86400;

	for (i = 0; i < pdivoom_google_calendar_cal_handle->calendar_num; i++)
	{
		pcal_item = &pdivoom_google_calendar_cal_handle->calendar_list[i];
		for (j = 0; j < pcal_item->event_num; j++)
		{
			pevent_item = &pcal_item->event_list[j];
			for (d = 0; d < 7; d++)
			{
				time_t d_start = week_start + (time_t)d * 86400;
				time_t d_end = d_start + 86400;

				if (pevent_item->end_utc <= (int)d_start || pevent_item->start_utc >= (int)d_end)
					continue;
				if (day_counts[d] < 99)
					day_counts[d]++;
			}
		}
	}

	divoom_system_free_lock(pdivoom_google_calendar_cal_handle->lock_handle);

	pdisp_data->num = 1;
	pdisp_data->update = 1;
	memset(divoom_clock_control_disp_data_get_item(pdisp_data, 0), 0, sizeof(divoom_clock_control_disp_array_item));
	DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).num = DIVOOM_GOOGLE_CALENDAR_WEEK_WATCH_DATA_NUM;
	for (d = 0; d < 7; d++)
		DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).hour_value[d] = day_counts[d];
	DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).hour_value[7] = (int)week_start;
	return 1;
}

int divoom_google_calendar_get_today_event_watch_data(void *pdata, int clock_id)
{
	Pdivoom_clock_control_disp_data pdisp_data = (Pdivoom_clock_control_disp_data)pdata;
	divoom_calendar_today_watch_layout layout = divoom_calendar_today_watch_get_layout(clock_id);
	Pdivoom_google_calendar_event_item today_events[32];
	Pdivoom_google_calendar_cal_item today_event_cals[32];
	int today_event_num = 0;
	Pdivoom_google_calendar_cal_item pcal_item;
	Pdivoom_google_calendar_event_item pevent_item, tmp_event;
	time_t now, day_start, day_end;
	struct tm cur_tm;
	int i, j, row;
	static time_t last_watch_sync_req = 0;

	pdisp_data->num = 0;
	pdisp_data->update = 0;

	if (pdivoom_google_calendar_cal_handle == NULL || !divoom_google_calendar_check_login())
	{
		divoom_clock_control_disp_data_deinit(pdisp_data);
		divoom_clock_control_disp_data_init(pdisp_data);
		divoom_google_calendar_fill_today_event_watch_empty(pdisp_data, layout, divoom_google_calendar_check_login());
		return 1;
	}

	now = time(0);
	if (now - last_watch_sync_req >= 30)
	{
		last_watch_sync_req = now;
		divoom_google_calendar_request_watch_refresh();
	}

	if (divoom_google_calendar_watch_try_lock())
	{
		return 0;
	}

	divoom_clock_control_disp_data_deinit(pdisp_data);
	divoom_clock_control_disp_data_init(pdisp_data);
	pdisp_data->num = 0;
	pdisp_data->update = 0;

	localtime_r(&now, &cur_tm);
	day_start = now - cur_tm.tm_hour * 3600 - cur_tm.tm_min * 60 - cur_tm.tm_sec;
	day_end = day_start + 24 * 3600;

	for (i = 0; i < pdivoom_google_calendar_cal_handle->calendar_num; i++)
	{
		pcal_item = &pdivoom_google_calendar_cal_handle->calendar_list[i];
		for (j = 0; j < pcal_item->event_num; j++)
		{
			pevent_item = &pcal_item->event_list[j];
			divoom_google_calendar_resolve_event_color(pevent_item, pcal_item);
			if (pevent_item->end_utc <= day_start || pevent_item->start_utc >= day_end)
				continue;
			if (today_event_num >= (int)(sizeof(today_events) / sizeof(today_events[0])))
				break;
			today_event_cals[today_event_num] = pcal_item;
			today_events[today_event_num++] = pevent_item;
		}
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
				pcal_item = today_event_cals[i];
				today_event_cals[i] = today_event_cals[j];
				today_event_cals[j] = pcal_item;
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
			/* 没有已过去的活动，或当前及后续活动足够填满：顶部对齐，空行留在底部 */
			win_start = start_idx;
			show_num = today_event_num - start_idx;
			if (show_num > max_rows)
				show_num = max_rows;
			row_offset = 0;
		}
		else
		{
			/* 已有活动过去且当前/最后活动剩余不足一屏：把前面的活动也列出来，仍顶部对齐 */
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

		divoom_system_free_lock(pdivoom_google_calendar_cal_handle->lock_handle);

		pdisp_data->num = 1;
		pdisp_data->update = 1;
		memset(divoom_clock_control_disp_data_get_item(pdisp_data, 0), 0, sizeof(divoom_clock_control_disp_array_item));
		DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).num = layout.data_num;
		DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).hour_value[5] = -1;

		if (today_event_num == 0)
		{
			divoom_google_calendar_fill_today_event_watch_empty(pdisp_data, layout, 1);
			divoom_print("today event watch: clock:%d rows:%d total:0 (empty)\n", clock_id, max_rows);
			return 1;
		}

		for (row = 0; row < max_rows; row++)
		{
			if (row >= row_offset && row < row_offset + show_num)
			{
				uint8 watch_color[3];

				evt_idx = win_start + (row - row_offset);
				pevent_item = today_events[evt_idx];
				divoom_google_calendar_format_today_event_time(pevent_item,
					DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[row], DIVOOM_CLOCK_CONTROL_DISPLAY_MAX_LEN);
				if (pevent_item->title && strlen(pevent_item->title) > 0)
					strncpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[row + 5], pevent_item->title, DIVOOM_CLOCK_CONTROL_DISPLAY_MAX_LEN - 1);
				else
					strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[row + 5], "No title");
				divoom_google_calendar_soften_watch_color(pevent_item, watch_color);
				DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).hour_value[row] = divoom_google_calendar_pack_watch_color(watch_color);
				DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data_roll_flag[row + 5] = 1;
				divoom_print("today dot row%d: %s, evtColorId:%s, src:%02x%02x%02x, dot:%02x%02x%02x(#%02x%02x%02x), packed:0x%06x, cal:%s\n",
					row,
					DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[row + 5],
					pevent_item->color_id[0] ? pevent_item->color_id : "-",
					pevent_item->bg_color[0], pevent_item->bg_color[1], pevent_item->bg_color[2],
					watch_color[0], watch_color[1], watch_color[2],
					watch_color[0], watch_color[1], watch_color[2],
					DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).hour_value[row],
					today_event_cals[evt_idx] && today_event_cals[evt_idx]->id ? today_event_cals[evt_idx]->id : "-");
				if (pevent_item->start_utc <= now && pevent_item->end_utc > now)
					DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).hour_value[5] = row;
			}
			else
			{
				DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[row][0] = 0;
				DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[row + 5][0] = 0;
				DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).hour_value[row] = 0;
				divoom_print("today dot row%d: empty, packed:0x000000\n", row);
			}
		}

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
		{
			strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[10], "No More Events Today");
		}
		else
		{
			strcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[10], " ");
		}
		}
		else
		{
			DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[10][0] = 0;
		}
		divoom_calendar_format_today_activities(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[DIVOOM_CALENDAR_TODAY_ACTIVITIES_INDEX], today_event_num);

		divoom_print("today event watch: clock:%d rows:%d total:%d, start:%d, win_start:%d, show:%d, row_offset:%d, extra:%d, active_row:%d\n",
			clock_id,
			max_rows,
			today_event_num,
			start_idx < today_event_num ? start_idx : -1,
			win_start,
			show_num,
			row_offset,
			remaining > 0 ? remaining : 0,
			DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).hour_value[5]);
		return 1;
	}

	divoom_system_free_lock(pdivoom_google_calendar_cal_handle->lock_handle);
}

static void divoom_google_calendar_fill_next_event_watch_defaults(Pdivoom_clock_control_disp_data pdisp_data, int logged_in, int today_count)
{
	divoom_clock_control_disp_data_deinit(pdisp_data);
	divoom_clock_control_disp_data_init(pdisp_data);
	pdisp_data->num = 1;
	pdisp_data->update = 1;
	memset(divoom_clock_control_disp_data_get_item(pdisp_data, 0), 0, sizeof(divoom_clock_control_disp_array_item));
	DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).num = DIVOOM_GOOGLE_CALENDAR_NEXT_EVENT_WATCH_DATA_NUM;
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

int divoom_google_calendar_get_next_event_remain_sec(void)
{
	Pdivoom_google_calendar_cal_item pcal_item;
	Pdivoom_google_calendar_event_item pevent_item;
	static time_t last_ai_sync_req = 0;
	time_t now;
	time_t remain_time;
	int remain_sec;
	int best_remain;
	int i;
	int j;

	if (pdivoom_google_calendar_cal_handle == NULL ||
		divoom_google_calendar_check_login() == 0)
	{
		return DIVOOM_CALENDAR_EVENT_REMAIN_NONE;
	}
	now = time(0);
	if (now - last_ai_sync_req >= 30)
	{
		last_ai_sync_req = now;
		divoom_google_calendar_request_watch_refresh();
	}
	/* AI 调度不能在日历网络更新期间等待，下一次轮询会继续读取。 */
	if (divoom_system_try_lock(pdivoom_google_calendar_cal_handle->lock_handle))
	{
		return DIVOOM_CALENDAR_EVENT_REMAIN_BUSY;
	}
	best_remain = DIVOOM_CALENDAR_EVENT_REMAIN_NONE;
	if (pdivoom_google_calendar_cal_handle->calendar_list != NULL)
	{
		for (i = 0; i < pdivoom_google_calendar_cal_handle->calendar_num; i++)
		{
			pcal_item = &pdivoom_google_calendar_cal_handle->calendar_list[i];
			if (pcal_item->event_list == NULL)
			{
				continue;
			}
			for (j = 0; j < pcal_item->event_num; j++)
			{
				pevent_item = &pcal_item->event_list[j];
				if ((time_t)pevent_item->start_utc < now)
				{
					continue;
				}
				remain_time = (time_t)pevent_item->start_utc - now;
				remain_sec = remain_time > 0x7FFFFFFF ? 0x7FFFFFFF : (int)remain_time;
				if (best_remain < 0 || remain_sec < best_remain)
				{
					best_remain = remain_sec;
				}
			}
		}
	}
	divoom_system_free_lock(pdivoom_google_calendar_cal_handle->lock_handle);
	return best_remain;
}

/*********************************************************************
Function:         divoom_google_calendar_get_next_event_watch_data
Description:      表盘获取下一日程摘要（标题/时间/颜色/剩余活动数）
Input:            pdata
Return:           0:失败 1:成功
Author:           cursor
**********************************************************************/

int divoom_google_calendar_get_next_event_watch_data(void *pdata)
{
	Pdivoom_clock_control_disp_data pdisp_data = (Pdivoom_clock_control_disp_data)pdata;
	Pdivoom_google_calendar_event_item best = NULL;
	Pdivoom_google_calendar_cal_item pcal_item, best_cal = NULL;
	Pdivoom_google_calendar_event_item pevent_item;
	time_t now, day_start, day_end, start_local, end_local;
	struct tm start_tm, end_tm, cur_tm;
	int i, j, future_today = 0, remaining, total_today = 0;
	static time_t last_watch_sync_req = 0;

	pdisp_data->num = 0;
	pdisp_data->update = 0;

	if (pdivoom_google_calendar_cal_handle == NULL || !divoom_google_calendar_check_login())
	{
		divoom_google_calendar_fill_next_event_watch_defaults(pdisp_data, 0, 0);
		return 1;
	}

	now = time(0);
	if (now - last_watch_sync_req >= 30)
	{
		last_watch_sync_req = now;
		divoom_google_calendar_request_watch_refresh();
	}

	if (divoom_google_calendar_watch_try_lock())
	{
		return 0;
	}

	divoom_clock_control_disp_data_deinit(pdisp_data);
	divoom_clock_control_disp_data_init(pdisp_data);
	pdisp_data->num = 0;
	pdisp_data->update = 0;

	localtime_r(&now, &cur_tm);
	day_start = now - cur_tm.tm_hour * 3600 - cur_tm.tm_min * 60 - cur_tm.tm_sec;
	day_end = day_start + 24 * 3600;

	for (i = 0; i < pdivoom_google_calendar_cal_handle->calendar_num; i++)
	{
		pcal_item = &pdivoom_google_calendar_cal_handle->calendar_list[i];
		for (j = 0; j < pcal_item->event_num; j++)
		{
			pevent_item = &pcal_item->event_list[j];
			divoom_google_calendar_resolve_event_color(pevent_item, pcal_item);
			if (pevent_item->start_utc >= day_end || pevent_item->end_utc <= day_start)
				continue;
			total_today++;
			if (pevent_item->end_utc <= now)
				continue;
			if (pevent_item->start_utc > now)
			{
				future_today++;
				if (best == NULL || pevent_item->start_utc < best->start_utc)
				{
					best = pevent_item;
					best_cal = pcal_item;
				}
			}
		}
	}

	divoom_system_free_lock(pdivoom_google_calendar_cal_handle->lock_handle);

	pdisp_data->num = 1;
	pdisp_data->update = 1;
	memset(divoom_clock_control_disp_data_get_item(pdisp_data, 0), 0, sizeof(divoom_clock_control_disp_array_item));
	DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).num = DIVOOM_GOOGLE_CALENDAR_NEXT_EVENT_WATCH_DATA_NUM;

	if (best == NULL)
	{
		divoom_print("next event watch: no event today, cal:%d\n", pdivoom_google_calendar_cal_handle->calendar_num);
		divoom_google_calendar_fill_next_event_watch_defaults(pdisp_data, 1, total_today);
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

	memcpy(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).bg_color, best->bg_color, 3);
	DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).hour_value[0] = (int)best->start_utc;
	DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data_roll_flag[1] = 1;
	DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data_roll_flag[3] = 2;
	divoom_calendar_format_today_activities(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[DIVOOM_CALENDAR_TODAY_ACTIVITIES_INDEX], total_today);
	divoom_print("next event watch: %s, today:%d, future:%d, color:%02x%02x%02x, colorId:%s, calColor:%02x%02x%02x, cal:%s\n",
		DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[1], total_today, future_today,
		DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).bg_color[0], DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).bg_color[1], DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).bg_color[2],
		best->color_id[0] ? best->color_id : "-",
		best_cal ? best_cal->bg_color[0] : 0, best_cal ? best_cal->bg_color[1] : 0, best_cal ? best_cal->bg_color[2] : 0,
		best_cal && best_cal->id ? best_cal->id : "-");
	return 1;
}

static void divoom_google_calendar_request_watch_refresh_internal(int force)
{
	if (pdivoom_google_calendar_cal_handle == NULL)
		return;
	if (divoom_system_try_lock(pdivoom_google_calendar_cal_handle->lock_handle))
		return;
	#if 0
	int i;
	/* 原先 force/30s 就把 color/calendar/event 时间清 0，表盘刷新会把 600s/300s/120s 间隔打穿，后台一直打 Google。 */
	if (force || pdivoom_google_calendar_cal_handle->event_request_time + 30 <= time(0))
	{
		pdivoom_google_calendar_cal_handle->event_request_time = 0;
		pdivoom_google_calendar_cal_handle->calendar_request_time = 0;
		pdivoom_google_calendar_cal_handle->color_request_time = 0;
		for (i = 0; i < pdivoom_google_calendar_cal_handle->calendar_num; i++)
			pdivoom_google_calendar_cal_handle->calendar_list[i].last_update_time[0] = 0;
	}
	pdivoom_google_calendar_cal_handle->watch_refresh_flag = 1;
	#endif
	(void)force; /* force 不再清时间戳，间隔由主循环自己守。 */
	/* 通知全关时只在还没拉过事件时置位，已经拉过就睡觉；通知/菜单开着仍置位，由主循环按原间隔拉。 */
	if (pdivoom_google_calendar_cal_handle->pop_flag || pdivoom_google_calendar_cal_handle->event_flag || pdivoom_google_calendar_cal_handle->menu_flag || pdivoom_google_calendar_cal_handle->mail_pop_flag || pdivoom_google_calendar_cal_handle->event_request_time == 0)
		pdivoom_google_calendar_cal_handle->watch_refresh_flag = 1;
	divoom_system_free_lock(pdivoom_google_calendar_cal_handle->lock_handle);
	divoom_system_put_sem(pdivoom_google_calendar_cal_handle->run_sem);
}

void divoom_google_calendar_request_watch_refresh(void)
{
	divoom_google_calendar_request_watch_refresh_internal(0);
}

void divoom_google_calendar_request_watch_refresh_force(void)
{
	divoom_google_calendar_request_watch_refresh_internal(1);
}

/*********************************************************************
Function:         divoom_google_calendar_load_calenday_current_month
Description:      加载当前月的所有事件
Input:            data
Return:           
Author:           albert
**********************************************************************/

int divoom_google_calendar_load_calenday_current_month(void **queue_item)
{
    int index;
    time_t cur_time = 0, last_time, first_time, start_time; 
    struct tm cur_tm;
    Pdivoom_lvgl_calendar_event_info plvgl_event_item;
    Pdivoom_google_calendar_cal_item pcal_item;

    if(pdivoom_google_calendar_cal_handle == NULL)
    {
        return 0;
    }
    if(divoom_system_try_lock(pdivoom_google_calendar_cal_handle->lock_handle))
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
    localtime_r(&cur_time, &cur_tm);
    start_time = cur_time - (cur_tm.tm_mday - 1) * 24*3600 - cur_tm.tm_hour*3600 - cur_tm.tm_min*60 - cur_tm.tm_sec;
    localtime_r(&start_time, &cur_tm);
    start_time -= cur_tm.tm_wday * 24*3600;
    divoom_print("date:%d , %d, %d, %d\n", (int)start_time, cur_tm.tm_mon, cur_tm.tm_mday, cur_tm.tm_wday);
    

    for(int i = 0; i < pdivoom_google_calendar_cal_handle->calendar_num; i++)
    {
        pcal_item = &pdivoom_google_calendar_cal_handle->calendar_list[i];
        for(int j = 0; j < pcal_item->event_num; j++)
        {
            if(pcal_item->event_list[j].end_utc < start_time || pcal_item->event_list[j].start_utc >= start_time + (42*24*3600) || pcal_item->event_list[j].start_utc < start_time)
            {
                continue;
            }
            cur_time = pcal_item->event_list[j].start_utc;
            localtime_r(&cur_time, &cur_tm);
            index = (pcal_item->event_list[j].start_utc - start_time)/(24*3600);
            if(index >= 42)
            {
                continue;
            }
            
            plvgl_event_item = divoom_system_malloc(sizeof(divoom_lvgl_calendar_event_info));
            if(plvgl_event_item)
            {
                plvgl_event_item->title = divoom_app_com_copy_string(pcal_item->event_list[j].title, -1);
                plvgl_event_item->msg_info = divoom_app_com_copy_string(pcal_item->event_list[j].desc, -1);
                cur_time = pcal_item->event_list[j].start_utc;
                localtime_r(&cur_time, &cur_tm);
                plvgl_event_item->start_time = cur_tm.tm_hour * 60 + cur_tm.tm_min;
                cur_time = pcal_item->event_list[j].end_utc;
                localtime_r(&cur_time, &cur_tm);
                plvgl_event_item->end_time = cur_tm.tm_hour * 60 + cur_tm.tm_min;
                memcpy(plvgl_event_item->bg_color, pcal_item->event_list[j].bg_color, 3);
                memcpy(plvgl_event_item->fr_color, pcal_item->event_list[j].fr_color, 3);
                if(queue_item[index] == NULL)
                {
                    queue_item[index] = divoom_queue_init(1000, __func__, __LINE__);
                }
                divoom_queue_push(queue_item[index], plvgl_event_item);
            }
            
        }
    }
    pdivoom_google_calendar_cal_handle->update_flag = 0;
    divoom_system_free_lock(pdivoom_google_calendar_cal_handle->lock_handle);
    if(pdivoom_google_calendar_cal_handle->calendar_num > 1)
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
    
}

/*********************************************************************
Function:         divoom_google_calendar_get_update_event_flag
Description:      获取日历是否有更新
Input:            none
Return:           1 or 0
Author:           albert
**********************************************************************/

int divoom_google_calendar_get_update_event_flag(void)
{
    if(pdivoom_google_calendar_cal_handle == NULL)
    {
        return 0;
    }
    return pdivoom_google_calendar_cal_handle->update_flag;
}

/*********************************************************************
Function:         divoom_google_calendar_init
Description:      模块初始化
Input:            none
Return:           
Author:           albert
**********************************************************************/

void divoom_google_calendar_init(void *handle)
{
    pdivoom_google_calendar_cal_handle  = divoom_system_malloc(sizeof(divoom_google_calendar_cal_ctrl));
    if (pdivoom_google_calendar_cal_handle == NULL)
    {
        return;
    }
    pdivoom_google_calendar_cal_handle->color_list = divoom_queue_init(1000, __func__, __LINE__);
    pdivoom_google_calendar_cal_handle->event_msg_queue = divoom_queue_init(1000, __func__, __LINE__);
    pdivoom_google_calendar_cal_handle->phandle = handle;   
    pdivoom_google_calendar_cal_handle->calendar_list = NULL;
    pdivoom_google_calendar_cal_handle->calendar_num = 0; 
    pdivoom_google_calendar_cal_handle->token_generation = 0;
    pdivoom_google_calendar_cal_handle->token_refresh_tick = 0;
    pdivoom_google_calendar_cal_handle->token_refresh_pending = 0;
    pdivoom_google_calendar_cal_handle->watch_refresh_flag = 0; /* malloc 未清零，避免垃圾值把后台拉取误打开。 */
    pdivoom_google_calendar_cal_handle->run_sem = divoom_system_create_sem(1, 0);
    pdivoom_google_calendar_cal_handle->lock_handle = divoom_system_create_lock();
        
    divoom_google_calendar_load_config();
    divoom_system_create_task(divoom_google_calendar_main_task, "google_cal", pdivoom_google_calendar_cal_handle, DIVOOM_TASK_STACK_GOOGLE_CAL);
}


/*********************************************************************
Function:         divoom_outlook_calendar_update_menu_flag
Description:      进入与退出菜单
Input:            menu_flag
Return:           
Author:           albert
**********************************************************************/

void divoom_google_calendar_update_menu_flag(uint8 menu_flag)
{
    if(pdivoom_google_calendar_cal_handle == NULL)
    {
        return ;
    }
    pdivoom_google_calendar_cal_handle->menu_flag = menu_flag;
    if(menu_flag)
    {
        divoom_system_put_sem(pdivoom_google_calendar_cal_handle->run_sem);
    }
}

/*********************************************************************
Function:         divoom_google_calendar_update_switch
Description:      更新开关
Input:            purl_head_info
                　head_num
                  clock_id
Return:           
Author:           albert
**********************************************************************/

void divoom_google_calendar_update_switch(int pop_flag, int event_flag)
{
    divoom_system_get_lock(pdivoom_google_calendar_cal_handle->lock_handle);
    pdivoom_google_calendar_cal_handle->pop_flag = pop_flag;
    if(event_flag == 0 && pdivoom_google_calendar_cal_handle->event_flag == 1)
    {
        divoom_google_calendar_send_event_notice(NULL);
    }
    pdivoom_google_calendar_cal_handle->event_flag = event_flag;
    divoom_system_free_lock(pdivoom_google_calendar_cal_handle->lock_handle);
    divoom_google_calendar_save_config();
    divoom_system_put_sem(pdivoom_google_calendar_cal_handle->run_sem);
}

/*********************************************************************
Function:         divoom_google_calendar_check_login
Description:      是否有ＴＯＫＥＮ
Input:            purl_head_info
                　head_num
                  clock_id
Return:           
Author:           albert
**********************************************************************/

int divoom_google_calendar_check_login(void)
{
    if(pdivoom_google_calendar_cal_handle)
    {
        return pdivoom_google_calendar_cal_handle->google_token && strlen(pdivoom_google_calendar_cal_handle->google_token);
    }
    return 0;
    
}


/*********************************************************************
Function:         divoom_google_calendar_update
Description:      更新ＴＯＫＥＮ
Input:            pop_flag
                  event_flag
                  token
Return:           
Author:           albert
**********************************************************************/

void divoom_google_calendar_update_token(int pop_flag, int event_flag, char *token)
{
    #if 1
    if(token == NULL || strlen(token) == 0)
    {
        return;
    }
    if(pdivoom_google_calendar_cal_handle)
    {
        divoom_system_get_lock(pdivoom_google_calendar_cal_handle->lock_handle);
        if(strlen(token) &&(pdivoom_google_calendar_cal_handle->google_token == NULL  || strcmp(pdivoom_google_calendar_cal_handle->google_token, token)))
        {
            //如果TOKEN发生变化，将重新获取一次
            if(pdivoom_google_calendar_cal_handle->google_token)
            {
                divoom_system_free(pdivoom_google_calendar_cal_handle->google_token);
                pdivoom_google_calendar_cal_handle->google_token = NULL;
            }
            pdivoom_google_calendar_cal_handle->google_token = divoom_app_com_copy_string(token, -1);
            pdivoom_google_calendar_cal_handle->token_generation ++;
            pdivoom_google_calendar_cal_handle->color_request_time = 0;
            pdivoom_google_calendar_cal_handle->calendar_request_time = 0;
            pdivoom_google_calendar_cal_handle->event_request_time = 0;
        }
        if(pdivoom_google_calendar_cal_handle->google_token)
        {
            pdivoom_google_calendar_cal_handle->token_refresh_pending = 0;
            /* 开机 MQTT 回 token 后即使通知全关也拉一次，不必先切日历表盘。 */
            pdivoom_google_calendar_cal_handle->watch_refresh_flag = 1;
            pdivoom_google_calendar_cal_handle->color_request_time = 0;
            pdivoom_google_calendar_cal_handle->calendar_request_time = 0;
            pdivoom_google_calendar_cal_handle->event_request_time = 0;
        }
        if(pop_flag != -1)
        {
            pdivoom_google_calendar_cal_handle->pop_flag = pop_flag;
        }
        if(event_flag != -1)
        {
            if(event_flag == 0 && pdivoom_google_calendar_cal_handle->event_flag == 1)
            {
                divoom_google_calendar_send_event_notice(NULL);
            }
            pdivoom_google_calendar_cal_handle->event_flag = event_flag;
        }
        divoom_system_free_lock(pdivoom_google_calendar_cal_handle->lock_handle);
        divoom_google_calendar_save_config();
        divoom_system_put_sem(pdivoom_google_calendar_cal_handle->run_sem);
    }
    #endif
}

#endif

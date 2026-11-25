#ifndef _divoom_outlook_calendar_h_
#define _divoom_outlook_calendar_h_
/*********************************************************************
Function:         divoom_outlook_calendar_update_switch
Description:      更新开关
Input:            purl_head_info
                　head_num
                  clock_id
Return:           
Author:           albert
**********************************************************************/

void divoom_outlook_calendar_update_switch(int pop_flag, int event_flag);


/*********************************************************************
Function:         divoom_outlook_calendar_update
Description:      更新ＴＯＫＥＮ
Input:            purl_head_info
                　head_num
                  clock_id
Return:           
Author:           albert
**********************************************************************/

void divoom_outlook_calendar_update_token(int pop_flag, int event_flag, char *token);

/*********************************************************************
Function:         divoom_outlook_calendar_check_login
Description:      是否有ＴＯＫＥＮ
Input:            
Return:           
Author:           albert
**********************************************************************/

int divoom_outlook_calendar_check_login(void);


void divoom_outlook_calendar_init(void);


/*********************************************************************
Function:         divoom_outlook_calendar_get_update_flag
Description:      得到更新标志
Input:            
Return:           NONE
Author:           albert
**********************************************************************/

int divoom_outlook_calendar_get_update_flag(void);
/*********************************************************************
Function:         divoom_outlook_calendar_load_calenday_current_month
Description:      加载当前月的所有事件
Input:            data
Return:           
Author:           albert
**********************************************************************/

int divoom_outlook_calendar_load_calenday_current_month(void **queue_item);

/*********************************************************************
Function:         divoom_outlook_calendar_get_active_lvgl_event_info
Description:      得到LVGL显示的信息
Input:            data
Return:           
Author:           albert
**********************************************************************/

int divoom_outlook_calendar_get_active_lvgl_event_info(void *data);

/*********************************************************************
Function:         divoom_outlook_calendar_user_cancle_event
Description:      按键取消
Input:            
Return:           
Author:           albert
**********************************************************************/

void divoom_outlook_calendar_user_cancle_event(void);

/*********************************************************************
Function:         divoom_outlook_calendar_update_menu_flag
Description:      进入与退出菜单
Input:            purl_head_info
                　head_num
                  clock_id
Return:           
Author:           albert
**********************************************************************/

void divoom_outlook_calendar_update_menu_flag(uint8 menu_flag);

#define DIVOOM_OUTLOOK_CALENDAR_TODAY_WATCH_DATA_NUM 11
#define DIVOOM_OUTLOOK_CALENDAR_NEXT_EVENT_WATCH_DATA_NUM 4
#define DIVOOM_OUTLOOK_CALENDAR_WEEK_WATCH_DATA_NUM 7

int divoom_outlook_calendar_watch_cache_is_stale(const char *url_info, int cached_num, int clock_id);
int divoom_outlook_calendar_get_today_event_watch_data(void *pdata, int clock_id);
int divoom_outlook_calendar_get_next_event_watch_data(void *pdata);
#ifndef DIVOOM_CALENDAR_EVENT_REMAIN_NONE
#define DIVOOM_CALENDAR_EVENT_REMAIN_NONE (-1)
#define DIVOOM_CALENDAR_EVENT_REMAIN_BUSY (-2)
#endif
/* 非阻塞读取最近未来日程的剩余秒数；无数据返回 NONE，锁忙返回 BUSY。 */
int divoom_outlook_calendar_get_next_event_remain_sec(void);
int divoom_outlook_calendar_get_this_week_watch_data(void *pdata);
void divoom_outlook_calendar_request_watch_refresh(void);
void divoom_outlook_calendar_request_watch_refresh_force(void);

#endif




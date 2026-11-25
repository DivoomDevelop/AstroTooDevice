#ifndef _divoom_google_calendar_h_
#define _divoom_google_calendar_h_

/* Today 表盘：5 行时间(0-4) + 5 行标题(5-9) + footer(10)，cache 需完整保存 */
#define DIVOOM_GOOGLE_CALENDAR_TODAY_WATCH_DATA_NUM 11
/* Next Event 表盘：标题(1) + 时间(2) + 剩余数(3)，cache 需覆盖 pdisp_data[0..3] */
#define DIVOOM_GOOGLE_CALENDAR_NEXT_EVENT_WATCH_DATA_NUM 4
/* This Week 表盘：Mon-Sun 7 天事件数，存 hour_value[0..6] */
#define DIVOOM_GOOGLE_CALENDAR_WEEK_WATCH_DATA_NUM 7

typedef struct _divoom_google_gmail_item
{
    uint8 reserved;
}divoom_google_gmail_item, *Pdivoom_google_gmail_item;

typedef struct _divoom_google_calendar_event_item
{
    uint8 status;
    uint8 user_dismissed;   //用户主动退出 Active，本次活动不再自动进入
    void *lvgl_obj;
    char *event_id;
    char *title;        //事件标题
    char *updated_time;
    char *desc;         //事件描述
    int  start_utc;     //开始时间
    int  end_utc;       //结束时间
    int   next_notice_time; //下一次提醒时间
    uint8 bg_color[3];     //背景颜色
    uint8 fr_color[3];     //前景颜色
    char color_id[8];      //谷歌事件颜色ID，空则继承日历颜色
    int  reminder_num;       //提醒次数
    int  reminder_time[10]; //提前提醒时间
}divoom_google_calendar_event_item, *Pdivoom_google_calendar_event_item;


typedef struct _divoom_google_calendar_cal_item
{
    char *id;
    uint8 bg_color[3];     //背景颜色
    uint8 fr_color[3];     //前景颜色
    char last_update_time[32];
    int reminder_num;       //提醒次数
    int reminder_time[10];   //默认的提醒时间
    int event_num;      //有多少个事件
    Pdivoom_google_calendar_event_item event_list;   //事件数组
}divoom_google_calendar_cal_item, *Pdivoom_google_calendar_cal_item;

/*********************************************************************
Function:         divoom_google_calendar_init
Description:      模块初始化
Input:            none
Return:           
Author:           albert
**********************************************************************/

void divoom_google_calendar_init(void *handle);

/*********************************************************************
Function:         divoom_google_calendar_get_update_event_flag
Description:      获取日历是否有更新
Input:            none
Return:           1 or 0
Author:           albert
**********************************************************************/

int divoom_google_calendar_get_update_event_flag(void);

/*********************************************************************
Function:         divoom_google_calendar_update
Description:      更新ＴＯＫＥＮ
Input:            purl_head_info
                　head_num
                  clock_id
Return:           
Author:           albert
**********************************************************************/

void divoom_google_calendar_update(Pdivoom_channel_item_url_head_info purl_head_info, uint8 head_num, int clock_id);

/*********************************************************************
Function:         divoom_google_calendar_update_net_status
Description:      更新网络状态
Input:            status
                　
Return:           
Author:           albert
**********************************************************************/

void divoom_google_calendar_update_net_status(uint8 status);

/*********************************************************************
Function:         divoom_google_calendar_pack_calendar_item_to_json
Description:      将本地数据结构打包到JSON字符器，储存在数据库中
Input:            pcal_item
                  json_str
Return:           
Author:           albert
**********************************************************************/

char *divoom_google_calendar_pack_calendar_item_to_json(Pdivoom_google_calendar_cal_item pcal_item);

/*********************************************************************
Function:         divoom_google_calendar_unpack_calendar_item_to_json
Description:      将本地保存的JSON字串，解包到数据结构
Input:            pcal_item
                  json_str
Return:           
Author:           albert
**********************************************************************/

int divoom_google_calendar_unpack_calendar_item_to_json(Pdivoom_google_calendar_cal_item pcal_item, char *json_str);

/*********************************************************************
Function:         divoom_google_calendar_unpack_event_item_to_json
Description:      将本地保存的JSON字串，解包到数据结构
Input:            pevent_item
                  json_str
Return:           
Author:           albert
**********************************************************************/

int divoom_google_calendar_unpack_event_item_to_json(Pdivoom_google_calendar_event_item pevent_item, char *json_str);

/*********************************************************************
Function:         divoom_google_calendar_pack_event_item_to_json
Description:      将本地数据结构打包到JSON字符器，储存在数据库中
Input:            pevent_item
Return:           
Author:           albert
**********************************************************************/

char *divoom_google_calendar_pack_event_item_to_json(Pdivoom_google_calendar_event_item pevent_item);

/*********************************************************************
Function:         divoom_google_calendar_format_event_notice_time
Description:      计算事件的提醒时间
Input:            data
Return:           
Author:           albert
**********************************************************************/

int divoom_google_calendar_format_event_notice_time(void *data);

/*********************************************************************
Function:         divoom_google_calendar_get_current_day_to_do_list
Description:      表盘获取当天的待办事项
Input:            data
Return:           
Author:           wangft
**********************************************************************/

int divoom_google_calendar_get_current_day_to_do_list(void *pdata);

/*********************************************************************
Function:         divoom_google_calendar_watch_cache_is_stale
Description:      判断 Google 表盘缓存条目数是否不足（旧版 cache 仅保存 num=1）
Input:            url_info 表盘 URL；cached_num 缓存中的 pdisp_list[0].num
Return:           1:缓存不完整需忽略 0:可用
Author:           cursor
**********************************************************************/

int divoom_google_calendar_watch_cache_is_stale(const char *url_info, int cached_num, int clock_id);

/*********************************************************************
Function:         divoom_google_calendar_get_today_event_watch_data
Description:      表盘获取今日日程列表（最多5条+剩余活动数）
Input:            pdata
Return:           0:失败 1:成功
Author:           cursor
**********************************************************************/

int divoom_google_calendar_get_today_event_watch_data(void *pdata, int clock_id);

/*********************************************************************
Function:         divoom_google_calendar_get_next_event_watch_data
Description:      表盘获取下一日程摘要（标题/时间/颜色/剩余活动数）
Input:            pdata
Return:           0:失败 1:成功
Author:           cursor
**********************************************************************/

int divoom_google_calendar_get_next_event_watch_data(void *pdata);

#ifndef DIVOOM_CALENDAR_EVENT_REMAIN_NONE
#define DIVOOM_CALENDAR_EVENT_REMAIN_NONE (-1)
#define DIVOOM_CALENDAR_EVENT_REMAIN_BUSY (-2)
#endif

/*
 * 读取缓存中最近一个尚未开始的日程。
 * 返回距离开始的秒数；无有效日程或未登录返回 REMAIN_NONE，锁忙返回 REMAIN_BUSY。
 * 本接口不阻塞等待日历锁，供 AI 表盘高频轮询使用。
 */
int divoom_google_calendar_get_next_event_remain_sec(void);

/*********************************************************************
Function:         divoom_google_calendar_get_this_week_watch_data
Description:      表盘获取本周 Mon-Sun 每日事件数（热力图）
Input:            pdata
Return:           0:失败 1:成功
Author:           cursor
**********************************************************************/

int divoom_google_calendar_get_this_week_watch_data(void *pdata);

/*********************************************************************
Function:         divoom_google_calendar_request_watch_refresh
Description:      表盘读取前请求后台重新同步日历事件
Input:            none
Return:           none
Author:           cursor
**********************************************************************/

void divoom_google_calendar_request_watch_refresh(void);

/*********************************************************************
Function:         divoom_google_calendar_request_watch_refresh_force
Description:      切表盘时强制唤醒日历同步（不受30s节流限制）
Input:            none
Return:           none
Author:           cursor
**********************************************************************/

void divoom_google_calendar_request_watch_refresh_force(void);

/*********************************************************************
Function:         divoom_google_calendar_load_calenday_current_month
Description:      加载当前月的所有事件
Input:            data
Return:           
Author:           albert
**********************************************************************/

int divoom_google_calendar_load_calenday_current_month(void **queue_item);
/*********************************************************************
Function:         divoom_google_calendar_get_active_lvgl_event_info
Description:      得到LVGL显示的信息
Input:            data
Return:           
Author:           albert
**********************************************************************/

int divoom_google_calendar_get_active_lvgl_event_info(void *data);

/*********************************************************************
Function:         divoom_google_calendar_user_cancle_event
Description:      按键取消
Input:            
Return:           
Author:           albert
**********************************************************************/

void divoom_google_calendar_user_cancle_event(void);

/*********************************************************************
Function:         divoom_google_calendar_update_switch
Description:      更新开关
Input:            purl_head_info
                　head_num
                  clock_id
Return:           
Author:           albert
**********************************************************************/

void divoom_google_calendar_update_switch(int pop_flag, int event_flag);


/*********************************************************************
Function:         divoom_google_calendar_update
Description:      更新ＴＯＫＥＮ
Input:            purl_head_info
                　head_num
                  clock_id
Return:           
Author:           albert
**********************************************************************/

void divoom_google_calendar_update_token(int pop_flag, int event_flag, char *token);
/*********************************************************************
Function:         divoom_google_calendar_check_login
Description:      是否有ＴＯＫＥＮ
Input:            purl_head_info
                　head_num
                  clock_id
Return:           
Author:           albert
**********************************************************************/

int divoom_google_calendar_check_login(void);

/*********************************************************************
Function:         divoom_outlook_calendar_update_menu_flag
Description:      进入与退出菜单
Input:            menu_flag
Return:           
Author:           albert
**********************************************************************/

void divoom_google_calendar_update_menu_flag(uint8 menu_flag);

#endif

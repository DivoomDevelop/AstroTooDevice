#ifndef _divoom_time_plan_h_
#define _divoom_time_plan_h_

#include "divoom_screen_driver.h"


typedef struct _divoom_time_plan_item_info
{
    uint8  status;  //这个计划处于的状态
    uint8  enable;  //这个字项是关闭状态，还是开启状态
    uint8  cyc_attr;  //如果为0表示非循环，为其它为循环星期
    uint8  disp_mode; //0:表盘, 1:云图
    uint32 start_time;
    uint32 end_time;
    int    clock_id;
    uint8  cyc_flag[7];
    uint8  voice_switch;
    char   plan_name[64];
    char   file_id[64];
}divoom_time_plan_item_info, *Pdivoom_time_plan_item_info;


enum
{
    DIVOOM_TIME_PLAN_APP_EVENT_RUN,         
};


//内部事件宏，一般是由外部触发的
enum
{
    DIVOOM_TIME_PLAN_INIT,  //
    DIVOOM_TIME_PLAN_UPDATE_TIME,  //
    DIVOOM_TIME_PLAN_DELETE,  //
    DIVOOM_TIME_PLAN_MODIFY,  //
    DIVOOM_TIME_PLAN_CANCEL,  //开关时间管理
    DIVOOM_TIME_PLAN_OVER, 
};

int divoom_time_plan_get_plan_num(void);
char *divoom_time_plan_get_plan_name(int index);
void divoom_time_plan_init(void);
void divoom_time_plan_main(void);

uint8 divoom_time_plan_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type);

uint8 divoom_time_plan_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type);

uint8 divoom_time_plan_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type);

uint8 divoom_time_plan_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type);

/*********************************************************************
  Function:        divoom_time_plan_get_url_addr
  Description:     获取指定文件的网络地址
  Input:           model_id
                   image_id
                   url_addr
                    
  Return:          image_flag
**********************************************************************/

int divoom_time_plan_get_url_addr(int model_id, int image_id, char *url_addr);


/*********************************************************************
  Function:        divoom_time_plan_get_active_name
  Description:     获取激活时间计划名称
  Return:          激活时间计划名称
**********************************************************************/
char *divoom_time_plan_get_active_name(void);
/*********************************************************************
  Function:        divoom_time_plan_get_active_time_info
  Description:     获取激活时间计划起始与结束时间
  Return:          NONE
**********************************************************************/
void divoom_time_plan_get_active_time_info(char *time_info);

/*********************************************************************
  Function:        divoom_time_plan_get_active_count_down_info
  Description:     获取激活时间计划还有多长时间结束
  Return:          激活时间计划名称
**********************************************************************/
void divoom_time_plan_get_active_count_down_info(char *time_info);

#endif



#ifndef _divoom_user_define_schedule_h_

#define _divoom_user_define_schedule_h_
enum{
    DIVOOM_USER_DEFINE_SCHEDULE_ITEM_CUSTOM0,
    DIVOOM_USER_DEFINE_SCHEDULE_ITEM_CUSTOM1,
    DIVOOM_USER_DEFINE_SCHEDULE_ITEM_CUSTOM2,
    DIVOOM_USER_DEFINE_SCHEDULE_ITEM_EQ,
    DIVOOM_USER_DEFINE_SCHEDULE_ITEM_SUBSCRIBE,
    DIVOOM_USER_DEFINE_SCHEDULE_ITEM_NUMBER,
};

typedef struct _divoom_user_define_schedule_item
{
    uint32 start_utc_time;  //开始时间
    uint32 end_utc_time;    //结束时间
    uint32 pri;
    uint32 id;
    uint32 cyc_flag;
    uint8 active;       //0:过期的；1：有效的
    uint8 start_hour;
    uint8 start_min;
    uint8 end_hour;
    uint8 end_min;
    uint8 voice_flag;
}divoom_user_define_schedule_item, *Pdivoom_user_define_schedule_item;


typedef struct _divoom_user_define_schedule_play_info
{
    uint32 id;
    uint32 voice_flag;
}divoom_user_define_schedule_play_info, *Pdivoom_user_define_schedule_play_info;


/*
  Function:       divoom_user_define_schedule_init
  Description:    初始化
  Input:          none
  Return:         NONE
  Author:         aijun
*/

void divoom_user_define_schedule_init(void);


/*
  Function:       divoom_user_define_schedule_get_next_run_item
  Description:    从待执行的队列中取出最开始执行的那个，也就是start_utc_time最小的那个
  Input:          pclock_item
  Return:         NONE
  Author:         aijun
*/

void divoom_user_define_schedule_get_next_run_item(void);

/*
  Function:       divoom_user_define_schedule_update
  Description:    更新一个计划项，如果不存在，将添加
  Input:          pclock_item
  Return:         NONE
  Author:         aijun
*/

int divoom_user_define_schedule_update(Pdivoom_channel_user_define_schedule_info pitem);

/*
  Function:       divoom_user_define_schedule_update
  Description:    更新计划列表
  Input:          pclock_item
                  num
  Return:         NONE
  Author:         aijun
*/

void divoom_user_define_schedule_update_list(Pdivoom_channel_user_define_schedule_info plist, int num);
/*
  Function:       divoom_user_define_schedule_clear_all
  Description:    清除所有的
  Input:          puser_define_item
                  num
  Return:         NONE
  Author:         aijun
*/

void divoom_user_define_schedule_clear_all(void);


/*
  Function:       divoom_user_define_schedule_main
  Description:    对计划进行相关调度
  Input:          
  Return:         NONE
  Author:         aijun
*/

void divoom_user_define_schedule_main(void);



/*
  Function:       divoom_user_define_schedule_restore_run
  Description:    恢复当前运行的时间计划, 在事件重新被调度时调用
  Input:          
  Return:         NONE
  Author:         aijun
*/

void divoom_user_define_schedule_restore_run(void);





#endif



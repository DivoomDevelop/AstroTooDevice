#ifndef _divoom_clock_schedule_h_

#define _divoom_clock_schedule_h_

typedef struct _divoom_clock_schedule_item
{
    uint32 start_utc_time;  //开始时间
    uint32 end_utc_time;    //结束时间
    uint32 pri;
    uint32 clock_id;
	uint8 func_flag;	//0 表盘计划 1 纪念日提醒
    uint32 cyc_flag;
    uint8 status;
    uint8 active;       //0:过期的；1：有效的
    uint8 start_hour;
    uint8 start_min;
    uint8 end_hour;
    uint8 end_min;
    uint8 voice_flag;
}divoom_clock_schedule_item, *Pdivoom_clock_schedule_item;


typedef struct _divoom_clock_schedule_play_info
{
    uint32 clock_id;
    uint32 voice_flag;
}divoom_clock_schedule_play_info, *Pdivoom_clock_schedule_play_info;


/*
  Function:       divoom_clock_schedule_init
  Description:    初始化
  Input:          none
  Return:         NONE
  Author:         aijun
*/

void divoom_clock_schedule_init(void);


/*
  Function:       divoom_clock_schedule_clear_all
  Description:    清除所有配置
  Input:          NONE
  Return:         NONE
  Author:         aijun
*/

void divoom_clock_schedule_clear_all(void);

/*
  Function:       divoom_clock_schedule_get_next_run_item
  Description:    从待执行的队列中取出最开始执行的那个，也就是start_utc_time最小的那个
  Input:          pclock_item
  Return:         NONE
  Author:         aijun
*/

void divoom_clock_schedule_get_next_run_item(void);

/*
  Function:       divoom_clock_schedule_update
  Description:    更新一个计划项，如果不存在，将添加
  Input:          pclock_item
  Return:         NONE
  Author:         aijun
*/

int divoom_clock_schedule_update(Pdivoom_clock_schedule_item pclock_item);
/*
  Function:       divoom_clock_schedule_update_divoom_cloud
  Description:    更新计划列表
  Input:          pclock_item
                  num
  Return:         NONE
  Author:         aijun
*/

void divoom_clock_schedule_update_divoom_cloud(Pdivoom_channel_user_define_schedule_info pclock_item);

/*
  Function:       divoom_clock_schedule_update
  Description:    更新计划列表
  Input:          pclock_item
                  num
  Return:         NONE
  Author:         aijun
*/

void divoom_clock_schedule_update_list(Pdivoom_clock_schedule_item pclock_item, int num);


/*
  Function:       divoom_clock_schedule_main
  Description:    对计划进行相关调度
  Input:          
  Return:         NONE
  Author:         aijun
*/

void divoom_clock_schedule_main(void);



/*
  Function:       divoom_clock_schedule_restore_run
  Description:    恢复当前运行的时间计划, 在事件重新被调度时调用
  Input:          
  Return:         NONE
  Author:         aijun
*/

void divoom_clock_schedule_restore_run(void);

/*
  Function:       divoom_clock_schedule_get_active_play_info
  Description:    得到当前定时表盘播放信息，供播放计划兜底
  Input:          clock_id, voice_flag
  Return:         1 有活跃项
*/

uint8 divoom_clock_schedule_get_active_play_info(int *clock_id, uint32 *voice_flag);

#endif


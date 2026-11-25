#ifndef _divoom_power_control_h_
#define _divoom_power_control_h_



#define DIVOOM_POWER_CTRL_NUM    10


typedef struct _divoom_power_control_item
{
	T_U8    on_off;          // 1:on; 0:off
	//T_U8    voice;          // 1:on; 0:off
	T_U8    mode;          // 1:on; 0:off  0:开机   1:关机
    T_U8    hour;           /* 0-23 */
    T_U8    minute;         /* 0-59 */
    T_U8    week;           /* 与闹钟 cyc_flag 相同：bit0=周日, bit1=周一 … bit6=周六；0=不循环 */
	T_U32	pow_on_id;
    //divoom_screen_color_info color;
    //T_U32   utc_time;
}divoom_power_control_item, *Pdivoom_power_control_item;

/*********************************************************************
  Function:         divoom_power_control_init
  Description:     初始化开关机模块
  Input:             none
  Output            none
**********************************************************************/

void divoom_power_control_init(uint8 flag);

/*********************************************************************
  Function:         divoom_power_control_send
  Description:     向APP发送开关相应的数据包
  Input:             spp_data               APP数据包
  Output            none
**********************************************************************/
void divoom_power_control_send(Pdivoom_power_control_item alarm_info);


/*********************************************************************
  Function:         divoom_power_control_update
  Description:     返回下一次开关机的UTC时间
  Input:             data               所有的开关机信息
                       power_on_flag        1:开机；0:为关机
  Output            相应功能的UTC时间
**********************************************************************/

uint32 divoom_power_control_update(void *data, uint8 power_on_flag);

/*********************************************************************
  Function:         divoom_power_control_power_on_time
  Description:     返回下一次开机的UTC时间
  Input:             
  Output             NONE
**********************************************************************/

uint32 divoom_power_control_power_on_time(void);

/*********************************************************************
  Function:         divoom_power_control_main
  Description:     此函数系统会1S调用一次，查看是否到关机时间
  Input:             
  Output             NONE
**********************************************************************/

void divoom_power_control_main(void);

/** 所有已开启的「定时开机」(mode==0) 中，下一次触发时刻（Unix 秒），无则返回 0 */
uint32 divoom_power_control_get_next_power_on_unix(void);

/** 同上，且输出最早一项在数组中的下标（无则 *out_index=-1） */
uint32 divoom_power_control_get_next_power_on_unix_and_index(int *out_index);

/** DIVOOM_ONOFF_MODE_POWER_SCHEDULE 唤醒并完成初始化后调用：同步服务器、清除一次性定时开机项 */
void divoom_power_control_after_rtc_schedule_wakeup(void);

/** PowerOn/GetList 合并后：补发一次性定时开机的 PowerOn/Set（RTC 唤醒时 pow_on_id 可能尚未同步） */
void divoom_power_control_on_get_list_sync(void);

/** GetList 合并时：该 PowerOnId 是否应强制 EnableFlag=0（RTC 一次性定时刚执行） */
int divoom_power_control_should_force_disable_pow_on_id(uint32 pow_on_id);

/** 网络/NTP 校时后刷新下一次定时开关机时刻 */
void divoom_power_control_on_time_sync(void);

#endif



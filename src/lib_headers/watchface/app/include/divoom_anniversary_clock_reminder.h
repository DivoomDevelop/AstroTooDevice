#ifndef _divoom_anniversary_clock_reminder_
#define _divoom_anniversary_clock_reminder_

/* Shared by countdown display, element fallback and built-in reminder IDs.
 * Christmas uses its regular calendar date, December 25. */
#define DIVOOM_HOLIDAY_HALLOW_MONTH 11
#define DIVOOM_HOLIDAY_HALLOW_DAY 1
#ifndef DIVOOM_HOLIDAY_CHRISTMAS_MONTH
#define DIVOOM_HOLIDAY_CHRISTMAS_MONTH 12
#endif
#ifndef DIVOOM_HOLIDAY_CHRISTMAS_DAY
#define DIVOOM_HOLIDAY_CHRISTMAS_DAY 25
#endif
#define DIVOOM_HOLIDAY_NEW_YEAR_MONTH 1
#define DIVOOM_HOLIDAY_NEW_YEAR_DAY 1
#define DIVOOM_HOLIDAY_LABOR_MONTH 5
#define DIVOOM_HOLIDAY_LABOR_DAY 1
#define DIVOOM_HOLIDAY_DEAD_MONTH 11
#define DIVOOM_HOLIDAY_DEAD_DAY 1
#define DIVOOM_HOLIDAY_CHILDREN_MONTH 6
#define DIVOOM_HOLIDAY_CHILDREN_DAY 1
#define DIVOOM_HOLIDAY_VALENTINE_MONTH 2
#define DIVOOM_HOLIDAY_VALENTINE_DAY 14
int divoom_anniversary_clock_reminder_get_holiday_date(int flag, int *month, int *day);

typedef struct _divoom_anniversary_reminder_info
{
	int reminder_id;	//提醒id
	int remind_flag;	//0当天  1提前一天 2提前两天 3提前一周
	int remind_disp_type;  // 0提醒 1显示表盘
	int month;
	int day;
	int start_utc_time;
	int end_utc_time;
	int current_day_start_utc_time; //纪念日当天也要提醒
	int current_day_end_utc_time; 
}divoom_anniversary_reminder_info, *Pdivoom_anniversary_reminder_info;

/*
	Func: divoom_anniversary_clock_reminder_del 
	Description: 删除提醒
	Param: reminder_id 提醒id  
			save_flag: 1保存写文件 0不写文件
	Return: none
	Author: wangft
	Date: 2025.6.13

*/
void divoom_anniversary_clock_reminder_del(int reminder_id, int save_flag);

void divoom_anniversary_clock_reminder_save_cfg(int lock_flag);

/*
	Func: divoom_anniversary_clock_reminder_add 
	Description: 增加提醒
	Param: reminder_info 提醒参数  
			save_flag: 1保存写文件 0不写文件
	Return: none
	Author: wangft
	Date: 2025.6.13

*/
void divoom_anniversary_clock_reminder_add(Pdivoom_anniversary_reminder_info reminder_info, int save_flag); 

void divoom_anniversary_clock_reminder_check_list(int *clock_list, int clock_num);

void divoom_anniversary_clock_reminder_main(void);

int divoom_anniversary_clock_reminder_init(void);

#endif


#include "divoom.h"


#define DIVOOM_BASE_RTC_YEAR 1900
enum{
	DIVOOM_RRULE_REPEAT_TYPE_DAY = 1,
	DIVOOM_RRULE_REPEAT_TYPE_WEEK,
	DIVOOM_RRULE_REPEAT_TYPE_MONTH,
	DIVOOM_RRULE_REPEAT_TYPE_YEAR,
};  //FREQ:DAILY,WEEKLY,MONTHLY,YEARLY,

/*
RRULE:FREQ=MONTHLY;COUNT=12;INTERVAL=2;BYDAY=3TU

FREQ:DAILY,WEEKLY,MONTHLY,YEARLY,
COUNT:重复多少次
BYDAY=3FR,MO,SA,SU,TH,TU,WE,
在星期前面可以加数字，表示这个月或这年的第几周的星期几
UNTIL=20211220T155959Z,到什么时间结束
INTERVAL=2　　间隔多久的周期，是以FREQ为单位
WKST=SU;一周的第一天是以星期几开始
*/

/*********************************************************************
Function:         divoom_rrule_get_utc_time
Description:      将时间字串转换为ＵＴＣ时间
Input:            time_str
Return:           utc时间
Author:           albert
**********************************************************************/

//"dateTime": "2021-09-21T20:00:00+08:00",
//"date": "2021-09-21"
int divoom_rrule_get_utc_time(char * time_str)
{
	int year, month, day, hour = 0, min = 0;
	struct tm time_date;

	memset(&time_date, 0, sizeof(time_date));
	if (strlen(time_str) > strlen("2021-09-21") + 3)
	{
		//这个是带时间的
		sscanf(time_str, "%d-%d-%dT%d:%d", &year, &month, &day, &hour, &min);
	}
	else
	{
		sscanf(time_str, "%d-%d-%d", &year, &month, &day);
	}
	time_date.tm_year = year - DIVOOM_BASE_RTC_YEAR;
	time_date.tm_mon = month - 1;
	time_date.tm_mday = day;
	time_date.tm_hour = hour;
	time_date.tm_min = min;

	return (int)mktime(&time_date);
}

/*********************************************************************
Function:         divoom_rrule_get_days_in_month
Description:      获取当前月有多少天
Input:            time_date
Return:           天数
Author:           albert
**********************************************************************/

uint8 divoom_rrule_get_days_in_month(struct tm *time_date)
{
	uint8 std_month_days[13] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	uint8 leap_month_days[13] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	int year = time_date->tm_year + DIVOOM_BASE_RTC_YEAR;

	if (0 == (year % 4) && ((year % 100) != 0 || 0 == (year % 400)))
	{
		return leap_month_days[time_date->tm_mon];
	}
	else
	{
		return std_month_days[time_date->tm_mon];
	}
}
/*********************************************************************
Function:         divoom_rrule_get_year_repeat
Description:      得到下一次激活的时间信息
Input:            start_base    开始基础时间
                  end_base      结束基础时间
                  start_time　　将开始的事件时间
                  end_time      将结束的事件时间
                  count         重复次数
                  over_time     最后结束时间
                  intval        间隔周期
                  attr          0:得到下一个开始的时间，１：得到当前正在运行的
Return:           如果有合适的下一个激活时间，将返回1,否则返回０
Author:           albert
**********************************************************************/

uint8 divoom_rrule_get_year_repeat(time_t  start_base, time_t  end_base, time_t  *start_time, time_t  *end_time, int count, int over_time, int intval, 
int attr)
{
	int run_cnt;
	time_t  cur_time = time(0), check_time, area_time;
	struct tm time_now;

	area_time = end_base - start_base;
	localtime_r(&start_base, &time_now);

	run_cnt = 0;
	while (1)
	{
		check_time = mktime(&time_now);
		if (over_time)
		{
			//有结束时间的
			if (over_time < check_time)
			{
				break;
			}
		}
		else if (count)
		{
			//有总共计数个数的
			if (run_cnt >= count)
			{
				break;
			}
		}
		if (attr == 0 && check_time > cur_time)
		{
			*end_time = end_base + check_time - start_base;
			*start_time = check_time;
			//如果结束时间超过最终的结束时间，将取最终的
			if (over_time && *end_time > over_time)
			{
				*end_time = over_time;
			}
			return 1;
		}
		else if (attr == 1)
		{
			//检查正在运行的，如果起始时间大于当前时间，将直接返回不存在，
			if (check_time > cur_time)
			{
				return 0;
			}
			//结束时间大于当前时间表示正在运行的
			if (check_time + area_time > cur_time)
			{
				*end_time = area_time + check_time;
				*start_time = check_time;
				return 1;
			}
		}
		time_now.tm_year += intval;
		run_cnt++;
	}
	return 0;
}

/*********************************************************************
Function:         divoom_rrule_get_month_repeat
Description:      得到下一次激活的时间信息
Input:            start_base    开始基础时间
                  end_base      结束基础时间
                  start_time　　将开始的事件时间
                  end_time      将结束的事件时间
                  count         重复次数
                  over_time     最后结束时间
                  skip_week     这个月的第几个星期,如果为－１，将是按日期
                  month_week    星期几
                  intval        间隔周期
                  attr          0:得到下一个开始的时间，１：得到当前正在运行的
Return:           如果有合适的下一个激活时间，将返回1,否则返回０
Author:           albert
**********************************************************************/

uint8 divoom_rrule_get_month_repeat(time_t start_base, time_t end_base, time_t *start_time, time_t *end_time, int count, int over_time, int intval, 
int skip_week, int month_week, int attr, int arrow_flag)
{
	int run_cnt, last_week = arrow_flag;
	int mon_days;
	time_t  cur_time = time(0), check_time, area_time;
	struct tm time_now, tmp_date;

	area_time = end_base - start_base;
	localtime_r(&start_base, &time_now);

	run_cnt = 0;
	while (1)
	{
		check_time = mktime(&time_now);
		if (over_time)
		{
			//有结束时间的
			if (over_time < check_time)
			{
				break;
			}
		}
		else if (count)
		{
			//有总共计数个数的
			if (run_cnt >= count)
			{
				break;
			}
		}
		if (attr == 0 && check_time > cur_time)
		{
			*end_time = end_base + check_time - start_base;
			*start_time = check_time;
			//如果结束时间超过最终的结束时间，将取最终的
			if (over_time && *end_time > over_time)
			{
				*end_time = over_time;
			}
			return 1;
		}
		else if (attr == 1)
		{
			//检查正在运行的，如果起始时间大于当前时间，将直接返回不存在，
			if (check_time > cur_time)
			{
				return 0;
			}
			//结束时间大于当前时间表示正在运行的
			if (check_time + area_time > cur_time)
			{
				*end_time = area_time + check_time;
				*start_time = check_time;
				return 1;
			}
		}
		time_now.tm_mon += intval;
		if (time_now.tm_mon >= 12)
		{
			time_now.tm_mon -= 12;
			time_now.tm_year++;
		}
		if (skip_week != -1)
		{
			//取这个月的多少号,先看１号是星期几，再推出这个月第几个星期几是几号
			time_now.tm_mday = 1;
			check_time = mktime(&time_now);
			localtime_r(&check_time, &tmp_date);
			if (last_week)
			{
				//最后一个星期

				mon_days = divoom_rrule_get_days_in_month(&tmp_date);
				//先取出第一个星期是几号
				if (tmp_date.tm_wday > month_week)
				{
					time_now.tm_mday = 7 + 1 - (time_now.tm_wday - month_week);
				}
				else
				{
					time_now.tm_mday = 1 + (month_week - time_now.tm_wday);
				}
				time_now.tm_mday += 7 * ((mon_days - time_now.tm_mday) / 7);
			}
			else
			{
				//从前向后
				if (tmp_date.tm_wday > month_week)
				{
					time_now.tm_mday = skip_week * 7 + 1 - (time_now.tm_wday - month_week);
				}
				else
				{
					time_now.tm_mday = (skip_week - 1) * 7 + 1 + (month_week - time_now.tm_wday);
				}
			}
		}
		else
		{
			mon_days = divoom_rrule_get_days_in_month(&time_now);
			while (mon_days < time_now.tm_mday)
			{
				time_now.tm_mon += intval;
				if (time_now.tm_mon >= 12)
				{
					time_now.tm_mon -= 12;
					time_now.tm_year++;
				}
				mon_days = divoom_rrule_get_days_in_month(&time_now);
			}
		}
		run_cnt++;
	}
	return 0;
}

/*********************************************************************
Function:         divoom_rrule_get_week_repeat
Description:      得到下一次激活的时间信息
Input:            start_base    开始基础时间
                  end_base      结束基础时间
                  start_time　　将开始的事件时间
                  end_time      将结束的事件时间
                  count         重复次数
                  over_time     最后结束时间
                  intval        间隔周期
                  repeat_week   重复的天气
                  attr          0:得到下一个开始的时间，１：得到当前正在运行的
Return:           如果有合适的下一个激活时间，将返回1,否则返回０
Author:           albert
**********************************************************************/

uint8 divoom_rrule_get_week_repeat(time_t start_base, time_t end_base, time_t *start_time, time_t *end_time, int count, int over_time, int intval, int *
repeat_week, int attr)
{
	int i, run_cnt, start_week = 0;
	time_t  cur_time = time(0), check_time, area_time;
	struct tm time_now;

	area_time = end_base - start_base;
	localtime_r(&start_base, &time_now);
	check_time = start_base;

	run_cnt = 0;
	start_week = time_now.tm_wday;
	while (1)
	{
		if (count)
		{
			//有总共计数个数的
			if (run_cnt >= count)
			{
				return 0;
			}
		}
		for (i = start_week; i < 7; i++, check_time += 24 * 3600)
		{
			if (over_time)
			{
				//有结束时间的
				if (over_time < check_time)
				{
					return 0;
				}
			}

			if (repeat_week[i] == 0)
			{
				continue;
			}

			if (attr == 0 && check_time > cur_time)
			{
				*end_time = end_base + check_time - start_base;
				*start_time = check_time;
				//如果结束时间超过最终的结束时间，将取最终的
				if (over_time && *end_time > over_time)
				{
					*end_time = over_time;
				}
				return 1;
			}
			else if (attr == 1)
			{
				//检查正在运行的，如果起始时间大于当前时间，将直接返回不存在，
				if (check_time > cur_time)
				{
					return 0;
				}
				//结束时间大于当前时间表示正在运行的
				if (check_time + area_time > cur_time)
				{
					*end_time = area_time + check_time;
					*start_time = check_time;
					return 1;
				}
			}
		}
		start_week = 0;
		check_time += (intval - 1) * 7 * 24 * 3600;
		run_cnt++;
	}
	return 0;
}

/*********************************************************************
Function:         divoom_rrule_get_day_repeat
Description:      得到下一次激活的时间信息
Input:              start_base    开始基础时间
                    end_base      结束基础时间
                    start_time　　将开始的事件时间
                    end_time      将结束的事件时间
                    count         重复次数
                    over_time     最后结束时间
                    intval        间隔周期
Return:           如果有合适的下一个激活时间，将返回1,否则返回０
Author:           albert
**********************************************************************/

uint8 divoom_rrule_get_day_repeat(time_t start_base, time_t end_base, time_t *start_time, time_t *end_time, int count, int over_time, int intval, int attr)
{
	int run_cnt;
	time_t  cur_time = time(0), check_time = start_base, area_time;

	area_time = end_base - start_base;

	run_cnt = 0;
	while (1)
	{
		if (over_time)
		{
			//有结束时间的
			if (over_time < check_time)
			{
				break;
			}
		}
		else if (count)
		{
			//有总共计数个数的
			if (run_cnt >= count)
			{
				break;
			}
		}
		if (attr == 0 && check_time > cur_time)
		{
			*end_time = end_base + check_time - start_base;
			*start_time = check_time;
			//如果结束时间超过最终的结束时间，将取最终的
			if (over_time && *end_time > over_time)
			{
				*end_time = over_time;
			}
			return 1;
		}
		else if (attr == 1)
		{
			//检查正在运行的，如果起始时间大于当前时间，将直接返回不存在，
			if (check_time > cur_time)
			{
				return 0;
			}
			//结束时间大于当前时间表示正在运行的
			if (check_time + area_time > cur_time)
			{
				*end_time = area_time + check_time;
				*start_time = check_time;
				return 1;
			}
		}
		check_time += intval * 24 * 3600;
		run_cnt++;
	}

	return 0;
}

/*********************************************************************
Function:         divoom_rrule_get_week_info
Description:      从字串中得到星期几，星期天为０
Input:            week_str
Return:           返回星期ＩＤ
Author:           albert
**********************************************************************/

uint8 divoom_rrule_get_week_info(char *week_str)
{
	if (memcmp(week_str, "SU", 2) == 0)
	{
		return 0;
	}
	else if (memcmp(week_str, "MO", 2) == 0)
	{
		return 1;
	}
	else if (memcmp(week_str, "TU", 2) == 0)
	{
		return 2;
	}
	else if (memcmp(week_str, "WE", 2) == 0)
	{
		return 3;
	}
	else if (memcmp(week_str, "TH", 2) == 0)
	{
		return 4;
	}
	else if (memcmp(week_str, "FR", 2) == 0)
	{
		return 5;
	}
	else if (memcmp(week_str, "SA", 2) == 0)
	{
		return 6;
	}
	else
	{
		return 0xFF;
	}
}

/*********************************************************************
Function:         divoom_rrule_get_time
Description:      通过信息得到下次启动的ＵＴＣ时间
Input:              start    开始基础时间
                    end      结束基础时间
                    rule_info     规则,rrule
                    start_time　　将开始的事件时间
                    end_time      将结束的事件时间
                    attr          ０：当前正在运行的，１：下一个将运行的
Return:           如果有合适的时间，将返回1,否则返回０
Author:           albert
eg:
	char start_info[256], end_info[256];
	time_t start_time, end_time;
	divoom_rrule_get_time("2021-08-18T10:00:00+08:00", "2021-08-18T11:00:00+08:00", "RRULE:FREQ=MONTHLY;BYDAY=3WE", &start_time, &end_time, 0);
	ctime_s(start_info, 256, &start_time);
	ctime_s(end_info,256,&end_time);
	printf("start: %s, end: %s\n", start_info, end_info);
}
**********************************************************************/

uint8 divoom_rrule_get_time(int start, int end, char *rule_info, time_t *start_time, time_t *end_time, int attr)
{
	char *ptr;
	time_t cur_time;
	struct tm time_date;
	int repeat_flag = 0, repeat_count = 0, reqeat_week[7], over_time = 0, intval = 1, start_week = 0,  skip_week = -1, month_week = 0, arrow_flag = 0;

	cur_time = time(0);
	*start_time = start;
	*end_time = end;
	if (rule_info == NULL || strlen(rule_info) == 0)
	{
		return *end_time > cur_time;

	}
	//统计所有信息
	ptr = strstr(rule_info, "FREQ=");
	if (ptr)
	{
		ptr += strlen("FREQ=");
		//FREQ:DAILY,WEEKLY,MONTHLY,YEARLY,
		if (memcmp(ptr, "DAILY", strlen("DAILY")) == 0)
		{
			repeat_flag = DIVOOM_RRULE_REPEAT_TYPE_DAY;
		}
		else if (memcmp(ptr, "WEEKLY", strlen("WEEKLY")) == 0)
		{
			repeat_flag = DIVOOM_RRULE_REPEAT_TYPE_WEEK;
		}
		else if (memcmp(ptr, "MONTHLY", strlen("MONTHLY")) == 0)
		{
			repeat_flag = DIVOOM_RRULE_REPEAT_TYPE_MONTH;
		}
		else if (memcmp(ptr, "YEARLY", strlen("YEARLY")) == 0)
		{
			repeat_flag = DIVOOM_RRULE_REPEAT_TYPE_YEAR;
		}
	}

	ptr = strstr(rule_info, "COUNT=");
	if (ptr)
	{
		ptr += strlen("COUNT=");
		repeat_count = atoi(ptr);
	}

	ptr = strstr(rule_info, "INTERVAL=");
	if (ptr)
	{
		ptr += strlen("INTERVAL=");
		intval = atoi(ptr);
	}


	ptr = strstr(rule_info, "UNTIL=");
	if (ptr)
	{
		//UNTIL=20211220T155959Z
		int year, month, day, hour = 0, min = 0;
		sscanf(ptr, "UNTIL=%04d%02d%02dT%02d%02d", &year, &month, &day, &hour, &min);
		time_date.tm_year = year - DIVOOM_BASE_RTC_YEAR;
		time_date.tm_mon = month - 1;
		time_date.tm_mday = day;
		time_date.tm_hour = hour;
		time_date.tm_min = min;
		time_date.tm_sec = 0;
		over_time = (int)mktime(&time_date);
	}

	ptr = strstr(rule_info, "WKST=");
	if (ptr)
	{
		//FR,MO,SA,SU,TH,TU,WE
		ptr += strlen("WKST=");
		start_week = divoom_rrule_get_week_info(ptr);
	}

	memset(reqeat_week, 0, sizeof(reqeat_week));
	ptr = strstr(rule_info, "BYDAY=");
	if (ptr)
	{
		int id = 1, cur_week;
		ptr += strlen("BYDAY=");
		if (*ptr == '-')
		{
			arrow_flag = 1;
			ptr++;
		}
		if (*ptr >= '0' && *ptr <= '9')
		{
			skip_week = *ptr - '0';
			ptr++;
			month_week = divoom_rrule_get_week_info(ptr);
			month_week = (month_week + 7 - start_week) % 7;
		}
		else
		{
			while (*ptr)
			{
				cur_week = divoom_rrule_get_week_info(ptr);
				if (cur_week == 0xFF)
				{
					break;
				}
				cur_week = (cur_week + 7 - start_week) % 7;
				reqeat_week[cur_week] = id;
				if (ptr[2] != ',')
				{
					break;
				}
				ptr += 3;
			}
		}
	}

	switch (repeat_flag)
	{
	case DIVOOM_RRULE_REPEAT_TYPE_DAY:
		return divoom_rrule_get_day_repeat(*start_time, *end_time, start_time, end_time, repeat_count, over_time, intval, attr);
		break;

	case DIVOOM_RRULE_REPEAT_TYPE_WEEK:
		return divoom_rrule_get_week_repeat(*start_time, *end_time, start_time, end_time, repeat_count, over_time, intval, reqeat_week, attr);
		break;

	case DIVOOM_RRULE_REPEAT_TYPE_MONTH:
		return divoom_rrule_get_month_repeat(*start_time, *end_time, start_time, end_time, repeat_count, over_time, intval, skip_week, month_week, attr, arrow_flag);
		break;

	case DIVOOM_RRULE_REPEAT_TYPE_YEAR:
		return divoom_rrule_get_year_repeat(*start_time, *end_time, start_time, end_time, repeat_count, over_time, intval, attr);
		break;

	default:
		return 0;
	}


}


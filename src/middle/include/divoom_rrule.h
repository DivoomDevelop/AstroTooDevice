#ifndef _divoom_rrule_h_
#define _divoom_rrule_h_

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

uint8 divoom_rrule_get_time(int start, int end, char *rule_info, time_t *start_time, time_t *end_time, int attr);

#endif


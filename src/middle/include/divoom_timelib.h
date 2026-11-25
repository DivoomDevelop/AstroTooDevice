#ifndef __divoom_Timelib_h
#define __divoom_Timelib_h

//typedef struct divoom__tzrule_struct
//{
//    char ch;
//    int m;
//    int n;
//    int d;
//    int s;
//    time_t change;
//    long offset; /* Match type of _timezone. */
//} divoom_tzrule_type;

//typedef struct __divoom_tzinfo_struct
//{
//    int __tznorth;
//    int __tzyear;
//    int _daylight;
//    divoom_tzrule_type __tzrule[2];
//} divoom_tzinfo_type;

/*****************************************************************************
 Prototype    : divoom_timelib_parse_tzset
 Description  : 对时区代号进行解析
 Input        : char *tzenv        eg:  "GMT0BST,M3.5.0/1,M10.5.0"     
                divoom_tzinfo_type *tz  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2021/4/13
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_timelib_parse_tzset(char *tzenv,divoom_tzinfo_type *tz);

/*****************************************************************************
 Prototype    : divoom_timelib_convert_timezone_r
 Description  : 转换得到对应城市时间（含夏令时间）
 Input        : const time_t *tim_p           	UTC时间
                divoom_tzinfo_type *const tz  	由时区代号得到的规则信息
                struct tm *res                  得到的对应城市时间
 Output       : None
 Return Value : struct
 
  History        :
  1.Date         : 2021/4/13
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
struct tm *divoom_timelib_convert_timezone_r(const time_t *tim_p,divoom_tzinfo_type *const tz, struct tm *res);


int divoom_timelib_hourFormat12(int hour);

time_t divoom_timelib_std_mktime(struct tm *tim_p);

/* UTC struct tm → epoch；RTOS newlib 无 timegm，语义同 divoom_timelib_std_mktime */
#define divoom_timelib_timegm(tm)  divoom_timelib_std_mktime(tm)

#endif



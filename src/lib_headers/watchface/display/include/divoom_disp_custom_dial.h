#ifndef _divoom_disp_custom_dial_h_

#define _divoom_disp_custom_dial_h_

enum{
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_SECOND = 1,           //时间秒
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_MIN = 2,              //时间分
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_HOUR = 3,             //时间小时
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_TIME_AM_PM = 4,      //此标记只有在１２小时制才能显示,上下午标记
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_HOUR_MIN = 5,         //小时：分钟
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_HOUR_MIN_SEC = 6,     //小时:分钟:钞
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_YEAR = 7,             //年　（数字）
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_DAY = 8,              //日（数字）
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_MON = 9,              //月（数字）
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_MON_YEAR = 10,         //月－年（数字）
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_ENG_MONTH_DOT_DAY = 11,//英文月点数字日
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_DATE_WEEK_YEAR = 12,   //数字日期-三位英文月份-年份
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_ENG_WEEK = 13,         //星期二个子母英文
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_ENG_WEEK_THREE = 14,         //星期英文三个字母缩写)
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_ENG_WEEK_ALL = 15,         //星期英文(全字母)
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_ENG_MON = 16,         //英文    jan
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_TEMP_DIGIT = 17,         //当前温度数字显示
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_TODAY_MAX_TEMP = 18,         //当天最高气温
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_TODAY_MIN_TEMP = 19,         //当天最低气温
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_WEATHER_WORD = 20,         //天气文字
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_NOISE_DIGIT = 21,         //噪音数字显示
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_TEXT_MESSAGE = 22,   //文字提醒框           
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_NET_TEXT_MESSAGE = 23,   //网络文字提醒框    
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_APP_TEXT_MESSAGE = 127,   //APP文字提醒框           
    DIVOOM_DISP_CUSTOM_DIAL_SUPPORT_END_TYPE ,   //结束类型
};
    
void divoom_disp_custom_dial_update(void *data);
void divoom_disp_custom_dial_clear(void *data);    
void divoom_disp_custom_dial_add(void *data);
void divoom_disp_custom_dial_add_item_list(void *data);
void divoom_disp_custom_dial_exit(void);
void divoom_disp_custom_dial_init(void *data);

#endif



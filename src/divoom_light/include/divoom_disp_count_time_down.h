#ifndef _divoom_disp_count_time_down_h_
#define _divoom_disp_count_time_down_h_

enum
{
	DIVOOM_DISP_COUNT_TIME_DOWN_WAIT = 0,     //进入这个页面，还未开始
	DIVOOM_DISP_COUNT_TIME_DOWN_RUN,          //正在运行
	DIVOOM_DISP_COUNT_TIME_DOWN_TIME_OVER,    //时间到结束了，需要显示动画和提示音，按键触发或者播放完了进入DIVOOM_DISP_COUNT_TIME_DOWN_WAIT状态
	DIVOOM_DISP_COUNT_TIME_DOWN_PHONE_CANCEL, //手机主动取消，需要显示动画，按键触发或者播放完了进入DIVOOM_DISP_COUNT_TIME_DOWN_WAIT状态
};
    
//void  divoom_app_count_time_down_get_timer_info(Pdivoom_system_tool_json_timer_info timer_info);
void divoom_disp_count_time_down_main(void *data);
//void divoom_disp_count_time_down_init(Pdivoom_disp_count_time_down_struct info);
void divoom_disp_count_time_down_init(Pdivoom_disp_count_time_down_struct info, DIVOOM_LIGHT_GET_TOOL_INFO func);
//void divoom_disp_count_time_down_play(uint8 status);
//void divoom_disp_count_time_down_disp_flag(uint8 flag);
void divoom_disp_count_time_down_update_info(Pdivoom_disp_count_time_down_struct info);
void divoom_disp_count_time_down_close(void);
//void divoom_disp_watch_digit(uint8 x, uint8 y, uint8 width, uint8 height, uint8 digit,uint8 * screen_buf, Pdivoom_screen_color_info color);

#endif

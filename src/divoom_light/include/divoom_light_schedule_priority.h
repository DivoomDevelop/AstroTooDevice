#ifndef _divoom_light_schedule_priority_h_
#define _divoom_light_schedule_priority_h_


typedef struct{
	uint8    mode;       //模块ＩＤ
	uint8	light;      //相关灯效
	uint8    attr;
	uint8    flag;       //如果为１，将被压入队列，如果为0，将不考虑压入队列，直接丢弃
	uint16	pri;        //优先级
}divoom_light_app_msg_id_attr_pri, *Pdivoom_light_app_msg_id_attr_pri;

//model和DIVOOM_MODEL_TYPE一样，可以减少第一次从model搜索的次数
static const divoom_light_app_msg_id_attr_pri all_divoom_app_light_priority_info[] =
{
    {0},
};
#endif



#include <stdint.h>
#include <stdarg.h>
#include <time.h>
#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_power_control.h"
#include "divoom_onoff.h"
#include "divoom_main.h"
#include "divoom_alarm.h"
#include "divoom_net_app.h"
#include "divoom_net_mqtt_manage.h"

uint8  divoom_power_control_power_off_index = 0;
uint32 divoom_power_control_power_off_time = T_U32_MAX;

/** RTC 定时开机刚执行完、待 PowerOn/Set 关闭的一次性项；-1 表示无 */
static int s_power_rtc_wake_disable_slot = -1;
/** 待关闭项 PowerOnId，网络未就绪时保留直至发送成功 */
static uint32 s_power_rtc_disable_pow_on_id = 0;

static int divoom_power_control_is_onetime_power_on(Pdivoom_power_control_item item)
{
	return item && item->mode == 0 && item->week == 0;
}

static int divoom_power_control_net_ready(void)
{
	if (divoom_net_app_net_is_ok() == 0)
		return 0;
	if (divoom_net_mqtt_get_net_status() == 0)
		return 0;
	return 1;
}

static void divoom_power_control_mark_rtc_disable_slot(int idx, Pdivoom_power_control_item alarm_info)
{
	if (idx < 0 || idx >= DIVOOM_POWER_CTRL_NUM)
		return;
	s_power_rtc_wake_disable_slot = idx;
	if (alarm_info && alarm_info[idx].pow_on_id)
		s_power_rtc_disable_pow_on_id = alarm_info[idx].pow_on_id;
}

static int divoom_power_control_find_pending_index(Pdivoom_power_control_item alarm_info)
{
	int i;
	int idx = s_power_rtc_wake_disable_slot;

	if (!alarm_info)
		return -1;
	if (idx >= 0 && idx < DIVOOM_POWER_CTRL_NUM && alarm_info[idx].pow_on_id != 0 &&
		(s_power_rtc_disable_pow_on_id == 0 ||
		 alarm_info[idx].pow_on_id == s_power_rtc_disable_pow_on_id))
		return idx;
	if (s_power_rtc_disable_pow_on_id) {
		for (i = 0; i < DIVOOM_POWER_CTRL_NUM; i++) {
			if (alarm_info[i].pow_on_id == s_power_rtc_disable_pow_on_id)
				return i;
		}
	}
	if (idx >= 0 && idx < DIVOOM_POWER_CTRL_NUM)
		return idx;
	return -1;
}

static void divoom_power_control_disable_onetime_slot(Pdivoom_power_control_item alarm_info, int idx)
{
	if (!alarm_info || idx < 0 || idx >= DIVOOM_POWER_CTRL_NUM)
		return;
	if (!divoom_power_control_is_onetime_power_on(&alarm_info[idx]))
		return;

	alarm_info[idx].on_off = 0;
	if (alarm_info[idx].pow_on_id)
		s_power_rtc_disable_pow_on_id = alarm_info[idx].pow_on_id;
	divoom_print("%s: disable one-time power-on slot %d, pow_on_id=%u\n",
		__func__, idx, (unsigned)alarm_info[idx].pow_on_id);
}

/** 定时开机唤醒后：清除已错过的一次性定时关机（关机时可能未落盘） */
static void divoom_power_control_clear_missed_onetime_poweroff(Pdivoom_power_control_item alarm_info)
{
	int i;
	uint32 today_sec;

	if (!alarm_info)
		return;

	today_sec = divoom_alarm_get_today_sec() % (24u * 3600u);
	for (i = 0; i < DIVOOM_POWER_CTRL_NUM; i++) {
		uint32 sched_sec;

		if (!alarm_info[i].on_off || alarm_info[i].week != 0 || alarm_info[i].mode != 1)
			continue;
		sched_sec = (uint32)alarm_info[i].hour * 3600u + (uint32)alarm_info[i].minute * 60u;
		if (today_sec <= sched_sec)
			continue;

		divoom_print("%s: clear missed one-time power-off slot %d\n", __func__, i);
		alarm_info[i].on_off = 0;
	}
}

/** @return 1 已向服务器同步关闭；0 仍 pending（如 MQTT 未连接） */
static int divoom_power_control_sync_rtc_wake_disable(Pdivoom_power_control_item alarm_info)
{
	int idx;

	if (!alarm_info || (s_power_rtc_wake_disable_slot < 0 && s_power_rtc_disable_pow_on_id == 0))
		return 0;

	idx = divoom_power_control_find_pending_index(alarm_info);
	if (idx < 0)
		return 0;
	if (!divoom_power_control_is_onetime_power_on(&alarm_info[idx])) {
		s_power_rtc_wake_disable_slot = -1;
		s_power_rtc_disable_pow_on_id = 0;
		return 1;
	}

	alarm_info[idx].on_off = 0;
	if (alarm_info[idx].pow_on_id)
		s_power_rtc_disable_pow_on_id = alarm_info[idx].pow_on_id;
	if (alarm_info[idx].pow_on_id == 0 || !divoom_power_control_net_ready())
		return 0;

	divoom_power_control_send(&alarm_info[idx]);
	divoom_main_save_config();
	divoom_print("%s: PowerOn/Set disable pow_on_id=%u\n", __func__,
		(unsigned)alarm_info[idx].pow_on_id);
	s_power_rtc_wake_disable_slot = -1;
	s_power_rtc_disable_pow_on_id = 0;
	return 1;
}

int divoom_power_control_should_force_disable_pow_on_id(uint32 pow_on_id)
{
	return s_power_rtc_disable_pow_on_id != 0 && s_power_rtc_disable_pow_on_id == pow_on_id;
}

/*********************************************************************
  Function:         divoom_power_control_init
  Description:     初始化开关机模块
  Input:             none
  Output            none
**********************************************************************/

void divoom_power_control_init(uint8 flag)
{
    Pdivoom_power_control_item alarm_info = NULL;
    
    if(flag == 1)
    {
        //T_DIVOOM_TIME time_info;
#if 0
        divoom_os_com_get_system_time(&time_info);
#else
		//由于插着充电线时提前5秒开机了,给加上这个时间，避免不会清掉当前的定时开机
		time_t utc_time = time(NULL);
		struct tm p_now;

		//utc_time += 5;
		//Utl_Convert_SecondToDate(utc_time,&time_info);
		localtime_r(&utc_time, &p_now);
#endif

        alarm_info = divoom_main_model_get_power_on_item();
        if (alarm_info)
        {
            int i;
            int wake_slot = divoom_onoff_get_power_schedule_wake_slot_index();

            if (wake_slot >= 0 && wake_slot < DIVOOM_POWER_CTRL_NUM)
            {
                divoom_power_control_mark_rtc_disable_slot(wake_slot, alarm_info);
                divoom_power_control_disable_onetime_slot(alarm_info, wake_slot);
            }
            else
            {
                /* 兼容：无 RTC 槽位标记时仍按当前时分匹配一次性定时开机 */
                for (i = 0; i < DIVOOM_POWER_CTRL_NUM; i++)
                {
                    if (alarm_info[i].on_off && alarm_info[i].week == 0 && alarm_info[i].mode == 0
                        && alarm_info[i].hour == p_now.tm_hour && alarm_info[i].minute == p_now.tm_min)
                    {
                        divoom_print("%s:we will del index power on config: %d\n", __func__, i);
                        divoom_power_control_mark_rtc_disable_slot(i, alarm_info);
                        divoom_power_control_disable_onetime_slot(alarm_info, i);
                        break;
                    }
                }
            }
            divoom_power_control_clear_missed_onetime_poweroff(alarm_info);
        }

    }
    divoom_power_control_power_off_time = T_U32_MAX;

	if(time(0) > 1597112532)
    {
    	divoom_power_control_update(alarm_info, 0);
	}

	if (flag == 1 && alarm_info)
		divoom_power_control_sync_rtc_wake_disable(alarm_info);
}



/*********************************************************************
  Function:         divoom_power_control_send
  Description:     向APP发送开关相应的数据包
  Input:             spp_data               APP数据包
  Output            none
**********************************************************************/
void divoom_power_control_send(Pdivoom_power_control_item alarm_info)
{
	int i;
	divoom_device_power_control_item app_alarm_info;

	app_alarm_info.on_off = alarm_info->on_off;
	app_alarm_info.power_on_flag = alarm_info->mode;
	app_alarm_info.power_on_id = alarm_info->pow_on_id;
	app_alarm_info.time = alarm_info->hour * 3600 + alarm_info->minute * 60;

	memset(app_alarm_info.cyc_flag, 0, sizeof(app_alarm_info.cyc_flag));
	app_alarm_info.cyc_flag[6] = (alarm_info->week & 0x01) ? 1 : 0;
	for(i = 0; i < 6; i++)
	{
		if(alarm_info->week & (1 << (i + 1)))
			app_alarm_info.cyc_flag[i] = 1;
		else
			app_alarm_info.cyc_flag[i] = 0;
	}
	
	divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_SET_POWER_ON, DIVOOM_SERVER_TOPIC, &app_alarm_info);

	#if 0
	T_U8 *spp_data;
	T_U32 offset = 0;
	T_U8 i, index = 0;
    Pdivoom_power_control_item alarm_info = divoom_config_get_power_ctrl_cfg_info();

    spp_data = divoom_system_malloc(8*DIVOOM_POWER_CTRL_NUM);
    if(spp_data == NULL)
    {
        return;
    }
    if(alarm_info)
    {
    	for(index=0;index<DIVOOM_POWER_CTRL_NUM;index++)
    	{
            if(alarm_info[index].on_off > 2)
            {
                alarm_info[index].on_off = 0;
            }
            if(alarm_info[index].mode > 2)
            {
                alarm_info[index].mode = 0;
            }
    		spp_data[offset++] = alarm_info[index].on_off;
    		spp_data[offset++] = alarm_info[index].mode;
            if(alarm_info[index].hour < 24)
            {
    		    spp_data[offset++] = alarm_info[index].hour;
            }
            else
            {
    		    spp_data[offset++] = 12;
            }
            if(alarm_info[index].minute < 60)
            {
    		    spp_data[offset++] = alarm_info[index].minute;
            }
            else
            {
    		    spp_data[offset++] = 0;
            }
    		spp_data[offset++] = alarm_info[index].week;
    		spp_data[offset++] = alarm_info[index].color.r_c;
    		spp_data[offset++] = alarm_info[index].color.g_c;
    		spp_data[offset++] = alarm_info[index].color.b_c;
    	}
    }
    else
    {
        memset(spp_data, 0, 8*DIVOOM_POWER_CTRL_NUM);
    }
	divoom_app_protocol_OKCommandACK(NULL, SPP_SPP_POWER_ON_OFF_INFO, (T_U8 *)spp_data, 8*DIVOOM_POWER_CTRL_NUM);
    divoom_system_free(spp_data);
	#endif
}

/*********************************************************************
  Function:         divoom_power_control_set
  Description:     更新一个开机的信息
  Input:             spp_data               APP数据包
  Output            none
**********************************************************************/

void divoom_power_control_set(uint8 *spp_data)
{
	#if 0
	T_U8 index = 0;
	T_U32 offset = 0;
    divoom_sys_msg_info msg_info = {0};
    divoom_disp_event_disp_info notice_info = {0};
    Pdivoom_power_control_item alarm_info = divoom_config_get_power_ctrl_cfg_info();

    if(alarm_info == NULL)
    {
		divoom_print("%s,get cfg fail!\n",__func__);
		return;
    }

	index = spp_data[offset++];

	alarm_info[index].on_off = spp_data[offset++];
	alarm_info[index].mode = spp_data[offset++];
	alarm_info[index].hour = spp_data[offset++];
	alarm_info[index].minute = spp_data[offset++];
	alarm_info[index].week = spp_data[offset++];
	alarm_info[index].color.r_c = spp_data[offset++];
	alarm_info[index].color.g_c = spp_data[offset++];
	alarm_info[index].color.b_c = spp_data[offset++];

    divoom_power_control_update(alarm_info, 0);
    divoom_main_save_notice_dis();
	#endif
}


/*********************************************************************
  Function:         divoom_power_control_update
  Description:     返回下一次开关机的UTC时间
  Input:             data               所有的开关机信息
                       power_on_flag        1:开机；0:为关机
  Output            相应功能的UTC时间
**********************************************************************/

uint32 divoom_power_control_update(void *data, uint8 power_on_flag)
{
    int i;
    uint32 utc_time;
    time_t now_t;
    //T_DIVOOM_TIME cur_time, local_time;
    divoom_alarm_system_date cur_time, local_time;
    T_DIVOOM_ALARM_ACTIVE_DO_INFO tmp_info;
    Pdivoom_power_control_item alarm_info;

    now_t = time(NULL);
    //divoom_os_com_get_system_time(&cur_time);
    divoom_alarm_get_sys_time(&cur_time);
	//divoom_print("cur_time:%d:%d:%d:%d:%d\n", cur_time.year, cur_time.month, cur_time.day, cur_time.hour, cur_time.minute);

    if(data == NULL)
    {
        alarm_info = divoom_main_model_get_power_on_item();
    }
    else
    {
        alarm_info = (Pdivoom_power_control_item)data;
    }
    if(alarm_info == NULL)
    {
        divoom_power_control_power_off_time = T_U32_MAX;
        return T_U32_MAX;
    }

    divoom_power_control_power_off_time = T_U32_MAX;
    for(i = 0; i < DIVOOM_POWER_CTRL_NUM; i++)
    {
        if(alarm_info[i].on_off == 0)
        {
            continue;
        }
        //if(alarm_info[i].mode == power_on_flag)
        {
            //if(alarm_info[i].week == 0)
            //{
            //    divoom_power_control_power_off_index = i;
            //}
            //else
            //{
            //    divoom_power_control_power_off_index = U8_MAX;
            //}
            tmp_info.cyc_flag = alarm_info[i].week;
            tmp_info.hour = alarm_info[i].hour;
            tmp_info.minute = alarm_info[i].minute;
			
            divoom_alarm_lib_convert_alarm_time_to_local(&tmp_info, &local_time, &cur_time);
			//divoom_print("local_time:%d:%d:%d:%d:%d\n", local_time.year, local_time.month, local_time.day, local_time.hour, local_time.minute);
            //utc_time = Utl_Convert_DateToSecond(&local_time);
            utc_time = divoom_alarm_time_to_sec(&local_time);
            if (utc_time < (uint32)now_t)
                continue;
            if(divoom_power_control_power_off_time > utc_time)
            {
            	divoom_power_control_power_off_index = i;
                divoom_power_control_power_off_time = utc_time;
            }
        }
    }

	if(divoom_power_control_power_off_time == T_U32_MAX)
	{
		divoom_power_control_power_off_index = U8_MAX;
	}
	else if((power_on_flag == 3) && (divoom_light_app_get_screen_status(0xFF) == 0) && (divoom_power_control_power_off_index < DIVOOM_POWER_CTRL_NUM) && (alarm_info[divoom_power_control_power_off_index].mode == 1))
	{
		//关机时,重新设置关机时间先开屏
		divoom_sys_msg_info msg_info = {0};
		
		msg_info.dest_id = DIVOOM_MODEL_MAIN;
		msg_info.msg_id = DIVOOM_MAIN_EVENT_POWER_OFF_DISP_MENU_CANCLE;
		divoom_sys_model_crl_msg_push(&msg_info);
	}

    divoom_print("%s: find: %d! index:%d, power_off_time: %d\n",__func__, power_on_flag, divoom_power_control_power_off_index, divoom_power_control_power_off_time);
    return divoom_power_control_power_off_time;
}


/*********************************************************************
  Function:         divoom_power_control_power_on_time
  Description:     返回下一次开机的UTC时间
  Input:
  Output             NONE
**********************************************************************/

uint32 divoom_power_control_power_on_time(void)
{
    divoom_power_control_update(NULL, 1);
    return divoom_power_control_power_off_time;
}

/*********************************************************************
  Function:         divoom_power_control_main
  Description:     此函数系统会1S调用一次，查看是否到关机时间
  Input:
  Output             NONE
**********************************************************************/

void divoom_power_control_main(void)
{
	time_t utc_time = time(NULL);
	Pdivoom_power_control_item alarm_info = divoom_main_model_get_power_on_item();

	if (alarm_info)
		divoom_power_control_sync_rtc_wake_disable(alarm_info);

    if(divoom_power_control_power_off_time == T_U32_MAX)
    {
        return;
    }
    if(divoom_power_control_power_off_time <= utc_time)
    {
		alarm_info = divoom_main_model_get_power_on_item();
		
        if(divoom_power_control_power_off_index < DIVOOM_POWER_CTRL_NUM)
        {
            //关闭一次性的关机选项
            
            if(alarm_info && alarm_info[divoom_power_control_power_off_index].week == 0)
            {
                divoom_print("%s:we will del index power off config: %d\n", __func__, divoom_power_control_power_off_index);
                alarm_info[divoom_power_control_power_off_index].on_off = 0;

				divoom_power_control_send(&alarm_info[divoom_power_control_power_off_index]);
                divoom_main_save_config();
            }

			if(alarm_info[divoom_power_control_power_off_index].mode == 0)	//开机
			{
#ifndef DIVOOM_TIME_FRAME_SPEAKER
				divoom_sys_msg_info msg_info = {0};
				msg_info.dest_id = DIVOOM_MODEL_MAIN;
				msg_info.msg_id = DIVOOM_MAIN_EVENT_POWER_OFF_DISP_MENU_CANCLE;
				divoom_sys_model_crl_msg_push(&msg_info);
#endif
			}
			else if(alarm_info[divoom_power_control_power_off_index].mode == 1) //关机
			{
#ifdef DIVOOM_TIME_FRAME_SPEAKER
				divoom_onoff_poweroff_no_tone();
#else
				divoom_sys_msg_info msg_info = {0};
				msg_info.dest_id = DIVOOM_MODEL_MAIN;
				msg_info.msg_id = DIVOOM_MAIN_EVENT_POWER_OFF_DISP_MENU_ENTER;
				divoom_sys_model_crl_msg_push(&msg_info);
#endif
			}
			divoom_power_control_update(NULL, 2);
        }
//        divoom_print("%s: we will power off!%d\n",__func__, divoom_power_control_power_off_index);
//        divoom_os_com_poweroff();
		
    }
}

/*********************************************************************
  Function:        divoom_power_control_get_next_power_on_unix_and_index
  Description:     已开启的定时开机项(mode==0)中，下一次触发时刻（Unix 秒），无则返回 0
**********************************************************************/
uint32 divoom_power_control_get_next_power_on_unix_and_index(int *out_index)
{
	int i;
	uint32 min_u = T_U32_MAX;
	uint32 utc_time;
	int best_i = -1;
	divoom_alarm_system_date cur_time, local_time;
	T_DIVOOM_ALARM_ACTIVE_DO_INFO tmp_info;
	Pdivoom_power_control_item pitem = divoom_main_model_get_power_on_item();
	time_t now_t;

	if (out_index)
		*out_index = -1;
	if (pitem == NULL)
		return 0;

	divoom_alarm_get_sys_time(&cur_time);
	now_t = time(NULL);

	for (i = 0; i < DIVOOM_POWER_CTRL_NUM; i++) {
		if (pitem[i].on_off == 0 || pitem[i].mode != 0) /* 0=开机 */
			continue;

		tmp_info.cyc_flag = pitem[i].week;
		tmp_info.hour = pitem[i].hour;
		tmp_info.minute = pitem[i].minute;
		divoom_alarm_lib_convert_alarm_time_to_local(&tmp_info, &local_time, &cur_time);
		utc_time = divoom_alarm_time_to_sec(&local_time);
		if (utc_time > (uint32)now_t && utc_time < min_u) {
			min_u = utc_time;
			best_i = i;
		}
	}

	if (min_u == T_U32_MAX)
		return 0;
	if (out_index)
		*out_index = best_i;
	return min_u;
}

uint32 divoom_power_control_get_next_power_on_unix(void)
{
	return divoom_power_control_get_next_power_on_unix_and_index(NULL);
}

/*********************************************************************
  Function:        divoom_power_control_after_rtc_schedule_wakeup
  Description:     定时开机 RTC 唤醒后：拉服务器开关机列表；一次性项本地关闭并 SET_POWER_ON 同步
**********************************************************************/
void divoom_power_control_after_rtc_schedule_wakeup(void)
{
	int idx;
	Pdivoom_power_control_item p;

	idx = divoom_onoff_get_power_schedule_wake_slot_index();
	divoom_onoff_clear_power_schedule_wake_slot_index();

	if (idx >= 0 && idx < DIVOOM_POWER_CTRL_NUM)
		divoom_power_control_mark_rtc_disable_slot(idx, p);

	if (s_power_rtc_wake_disable_slot < 0 && s_power_rtc_disable_pow_on_id == 0)
		return;

	p = divoom_main_model_get_power_on_item();
	if (!p)
		return;

	divoom_power_control_disable_onetime_slot(p, s_power_rtc_wake_disable_slot);
	divoom_power_control_sync_rtc_wake_disable(p);
	divoom_power_control_update(NULL, 2);
	divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_POWER_ON_LIST, DIVOOM_SERVER_TOPIC, NULL);
}

void divoom_power_control_on_get_list_sync(void)
{
	Pdivoom_power_control_item p = divoom_main_model_get_power_on_item();

	if (!p)
		return;
	divoom_power_control_sync_rtc_wake_disable(p);
}

void divoom_power_control_on_time_sync(void)
{
	if (time(NULL) > 1597112532)
		divoom_power_control_update(NULL, 0);
}

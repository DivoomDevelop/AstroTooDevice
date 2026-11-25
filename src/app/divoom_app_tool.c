#include "divoom.h"
#include "divoom_menu_control.h"

enum{
	DIVOOM_TOOL_COUNT_TIME_DOWN_CANCEL,
	DIVOOM_TOOL_COUNT_TIME_DOWN_OVER,
	DIVOOM_TOOL_COUNT_TIME_DOWN_START,
	DIVOOM_TOOL_COUNT_TIME_DOWN_DISP,

	DIVOOM_TOOL_SCORE_SET,

	DIVOOM_TOOL_WATCH_SET,

	DIVOOM_TOOL_NOISE_SET,
};
/*
static uint8 mode_to_event[][3] = {
	{DIVOOM_DISP_WATCH_MODE, DIVOOM_APP_TIME_WATCH_SET, DIVOOM_APP_TIME_WATCH_GET},
	{DIVOOM_DISP_SCORE_MODE, DIVOOM_APP_SCORE_SET, DIVOOM_APP_SCORE_GET},
	{DIVOOM_DISP_NOISE_MODE, DIVOOM_APP_NOISE_SET, DIVOOM_APP_NOISE_GET},
	{DIVOOM_DISP_COUNT_TIME_DOWN_MODE, DIVOOM_APP_COUNT_TIME_DOWN_SET, DIVOOM_APP_COUNT_TIME_DOWN_GET},
};
*/
/*
   倒计时,噪音,秒表,计分板,互相被打断,就结束前一个事件,不再恢复
   工具类被倒计时时间到提醒打断,要恢复到前一个事件,如果前一个是倒计时,那就回到倒计时,显示1分0秒,不运行,如果是噪音,那就回到噪音,但是噪音再取消,就要结束优先级
   工具类被工具意外的打断,要不可以被打断,但是数据要保留,比如噪音的开关状态,分数,倒计时的时间值,秒表的时间
   如果是被可以自动取消的事件打断的,比如闹钟,只能提醒等,需要能恢复原来的显示
   */

typedef struct _divoom_app_tool_struct_
{
	//uint8		last_is_noise;		//记录最后一个效果是不是噪音,1是,0不是
	//uint8		pre_mode_set;		//记录前一个效果是计分板还是秒表
	uint8		cur_mode_set;

	//count down use
	uint8		status;
	uint8		count_paused;	/* 1=App/本机暂停（WAIT 且保留剩余时间） */
	uint8		music_index;
	uint8		hour;
	uint8		minute;
	uint8		second;
	//时间到结束要更新好minute和second
	//倒计时设置的时间,是个数字,比如一分钟,这个值就是60
	int			next_reach_time;
	//倒计时显示时间结束的提示该结束时候的时间,是个数字
	int			next_over_time;

	//score
	uint8		on_off;
	uint16		score_red_score;
	uint16		score_blue_score;

	//暂停时间是当前运行了多久,每次设置进入到秒表都要从新获取start_time,底层会根据start_time + pause_time来计算运行了多久
	//uint32		start_time;
	//uint32		pause_time;
	//改为在dtimerpre一秒+1
	uint32		watch_disp_value;
	uint32		watch_start_time;
	uint8		ctrl_flag;
	int		lap_num;
	uint32		lap_time[100];

	uint8		noise_flag;
}divoom_app_tool_struct, *Pdivoom_app_tool_struct;

static Pdivoom_app_tool_struct pdivoom_app_tool_handle = NULL;

uint8 divoom_app_tool_event(Pdivoom_sys_msg_info msg);
void divoom_app_tool_exit_cur_mode(uint8 flag);
uint8 divoom_app_tool_get_schdule(uint8 flag);
uint8 divoom_app_tool_set_disp_mode(uint8 flag);

/* 调度 play 恢复 UI：已在目标页则不叠栈；仅从闹钟激活页抢回，避免番茄/主菜单/表盘被秒表倒计时拽走 */
static int divoom_app_tool_can_restore_tool_ui(int target_menu)
{
	int cur;

	cur = divoom_menu_control_get_active_menu_id();
	if (cur == target_menu)
	{
		return 0;
	}
	if (cur == DIVOOM_MENU_CONTROL_ID_ALARM_ACTIVE_MENU)
	{
		return 1;
	}
	return 0;
}

static void divoom_app_tool_try_restore_menu(int target_menu)
{
	if (!divoom_app_tool_can_restore_tool_ui(target_menu))
	{
		return;
	}
	divoom_menu_control_load_next(target_menu, NULL, 0, 0);
}

/* AstroToo 倒计时走 COUNT_DOWN_MENU（divoom_astrootoo_tools），不再开旧 START_MENU */
static void divoom_app_tool_open_countdown_ui(uint8 clearall)
{
	if (!pdivoom_app_tool_handle)
	{
		return;
	}
	if (divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_ID_COUNT_DOWN_MENU)
	{
		if (clearall)
		{
			divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_COUNT_DOWN_MENU, NULL, 0, 0);
		}
		else
		{
			divoom_app_tool_try_restore_menu(DIVOOM_MENU_CONTROL_ID_COUNT_DOWN_MENU);
		}
	}
	pdivoom_app_tool_handle->cur_mode_set = DIVOOM_DISP_COUNT_TIME_DOWN_MODE;
}

/* App Tools/SetTimer Status=0：暂停并冻结剩余时间（不是旧屏的取消动画） */
static void divoom_app_tool_countdown_apply_pause(Pdivoom_app_tool_count_time_down_info info)
{
	uint32 remain;

	if (!pdivoom_app_tool_handle)
	{
		return;
	}

	remain = 0;
	if (pdivoom_app_tool_handle->status == DIVOOM_DISP_COUNT_TIME_DOWN_RUN
		&& pdivoom_app_tool_handle->next_reach_time > time(0))
	{
		remain = (uint32)(pdivoom_app_tool_handle->next_reach_time - time(0));
	}

	if (info && (info->hour || info->minute || info->second))
	{
		pdivoom_app_tool_handle->hour = info->hour;
		pdivoom_app_tool_handle->minute = info->minute;
		pdivoom_app_tool_handle->second = info->second;
	}
	else if (remain > 0)
	{
		pdivoom_app_tool_handle->hour = (uint8)(remain / 3600);
		pdivoom_app_tool_handle->minute = (uint8)((remain / 60) % 60);
		pdivoom_app_tool_handle->second = (uint8)(remain % 60);
	}

	pdivoom_app_tool_handle->status = DIVOOM_DISP_COUNT_TIME_DOWN_WAIT;
	pdivoom_app_tool_handle->count_paused = 1;
	pdivoom_app_tool_handle->next_reach_time = 0;
	pdivoom_app_tool_handle->next_over_time = 0;
}

/* 到点铃声：先停 tone，再取消 OVER 调度（须在改 status 之前调用） */
static void divoom_app_tool_countdown_stop_over_alert(void)
{
	divoom_app_schedule_item_info event_info = {0};

	divoom_app_tool_count_down_stop_voice();
	if (!pdivoom_app_tool_handle)
	{
		return;
	}

	event_info.model_id = DIVOOM_MODEL_APP_TOOL;
	event_info.event_id = DIVOOM_APP_COOUNT_TIME_DOWN_OVER_EVENT_PLAY;
	event_info.resource_type = (uint8)(DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE | DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE);
	divoom_app_schedule_cancel(&event_info);

	if (pdivoom_app_tool_handle->status == DIVOOM_DISP_COUNT_TIME_DOWN_TIME_OVER)
	{
		pdivoom_app_tool_handle->status = DIVOOM_DISP_COUNT_TIME_DOWN_WAIT;
		pdivoom_app_tool_handle->next_over_time = 0;
	}
}

//获取前一个效果是什么模式,只能是噪音,计分板和秒表
//0xFF退出当前优先级
//只有在被倒计时时间到提醒打断的时候才用得到
//被其他的时间打断就直接全部置0xFF
/*uint8 divoom_app_tool_get_pre_mode(void)
{
	if(pdivoom_app_tool_handle)
	{
		if(pdivoom_app_tool_handle->last_is_noise)
		{
			return DIVOOM_DISP_NOISE_MODE;
		}
		else if(pdivoom_app_tool_handle->pre_mode_set != 0xFF)
		{
			return pdivoom_app_tool_handle->pre_mode_set;
		}
		else
			return 0xFF;
	}
	else
		return 0xFF;
}*/
const T_U8 divoom_app_tool_count_down_voice_table[] = {
	DIVOOM_VOICE_ID_CLOCK_RAINSTORM, DIVOOM_VOICE_ID_CLOCK_OCEAN_WAVES,
	DIVOOM_VOICE_ID_CLOCK_URBAN, DIVOOM_VOICE_ID_CLOCK_FOREST,
	DIVOOM_VOICE_ID_CLOCK_WHALE, DIVOOM_VOICE_ID_CLOCK_ENERGIZE,
	DIVOOM_VOICE_ID_CLOCK_EAGLE, DIVOOM_VOICE_ID_CLOCK_CAMPANULA,
	DIVOOM_VOICE_ID_CLOCK_NORMAL1, DIVOOM_VOICE_ID_CLOCK_PIANO,
	DIVOOM_VOICE_ID_CLOCK_NORMAL3, DIVOOM_VOICE_ID_CLOCK_NORMAL2,
};


void divoom_app_tool_init(void)
{
	if(pdivoom_app_tool_handle != NULL)
	{
		divoom_print("Some Wrong in %d\n", __LINE__);
		return;
	}

	pdivoom_app_tool_handle = (Pdivoom_app_tool_struct)divoom_malloc(sizeof(divoom_app_tool_struct),DIVOOM_MODEL_APP_TOOL,__LINE__,__FILE__);
	if(pdivoom_app_tool_handle == NULL)
	{
		divoom_print("Some Wrong in %s:%d\n", __func__, __LINE__);
		return;
	}
	memset(pdivoom_app_tool_handle, 0, sizeof(divoom_app_tool_struct));

	//pdivoom_app_tool_handle->pre_mode_set = 0xFF;
	//pdivoom_app_tool_handle->last_is_noise = 0;

	pdivoom_app_tool_handle->status = DIVOOM_DISP_COUNT_TIME_DOWN_WAIT;
	pdivoom_app_tool_handle->count_paused = 0;
	pdivoom_app_tool_handle->hour = 0;
    pdivoom_app_tool_handle->minute = 1;
	pdivoom_app_tool_handle->second = 0;
	pdivoom_app_tool_handle->next_reach_time = 0;
	pdivoom_app_tool_handle->next_over_time = 0;
	pdivoom_app_tool_handle->music_index = 8; /* CLOCK_NORMAL1 */

	pdivoom_app_tool_handle->score_red_score = 0;
	pdivoom_app_tool_handle->score_blue_score = 0;

	pdivoom_app_tool_handle->noise_flag = 2;
	pdivoom_app_tool_handle->ctrl_flag = 2;
    divoom_sys_model_crl_register(DIVOOM_MODEL_APP_TOOL,divoom_app_tool_event);
	//pdivoom_app_tool_handle->pause_time = 0;
}

void divoom_app_tool_close(void)
{
	if(pdivoom_app_tool_handle != NULL)
	{
		divoom_free(pdivoom_app_tool_handle);
	}
}


void divoom_app_tool_set_time_watch(void *data)
{
    divoom_sys_msg_info msg_info = {0};
	divoom_app_tool_time_watch_info info;
	//Uint8 Ctrl_flag ：1;    0:暂停；1：开始；2：重置
	if(data)
	{
		memcpy(&info,data,sizeof(divoom_app_tool_time_watch_info));
		msg_info.data = (uint8 *)&info;
		msg_info.dest_id = DIVOOM_MODEL_APP_TOOL;
		msg_info.msg_id = DIVOOM_APP_TIME_WATCH_SET;
		divoom_sys_model_crl_msg_push(&msg_info);
	}
}


void divoom_app_tool_set_score(void *data)
{
    divoom_sys_msg_info msg_info = {0};
	divoom_app_tool_score_info info;
	if(data)
	{
		memcpy(&info,data,sizeof(divoom_app_tool_score_info));
		msg_info.data = (uint8 *)&info;
		msg_info.dest_id = DIVOOM_MODEL_APP_TOOL;
		msg_info.msg_id = DIVOOM_APP_SCORE_SET;
		divoom_sys_model_crl_msg_push(&msg_info);
	}
}

void divoom_app_tool_set_count_time_down(void *data)
{
    divoom_sys_msg_info msg_info = {0};
	divoom_app_tool_count_time_down_info info;
	if(data)
	{
		memcpy(&info,data,sizeof(divoom_app_tool_count_time_down_info));
		msg_info.data = (uint8 *)&info;
		msg_info.dest_id = DIVOOM_MODEL_APP_TOOL;
		msg_info.msg_id = DIVOOM_APP_COUNT_TIME_DOWN_SET;
		divoom_sys_model_crl_msg_push(&msg_info);
	}
}

void divoom_app_tool_set_noise(void *data)
{
    divoom_sys_msg_info msg_info = {0};
	divoom_app_tool_noise_info info;
	if(data)
	{
		memcpy(&info,data,sizeof(divoom_app_tool_noise_info));
		msg_info.data = (uint8 *)&info;
		msg_info.dest_id = DIVOOM_MODEL_APP_TOOL;
		msg_info.msg_id = DIVOOM_APP_NOISE_SET;
		divoom_sys_model_crl_msg_push(&msg_info);
	}
}



#if 0

//返回需要的数据长度
uint8 divoom_app_tool_get_event(uint8 model_id, uint8 *data)
{
	uint8 len = 0;
	divoom_sys_msg_info msg_info = {0};
	divoom_disp_ctrl_watch_ctrl watch_info;
	divoom_app_tool_score_info score_info;
	divoom_app_tool_count_time_down_info count_info;
	if(data)
	{
		uint8 light_mode = mode_to_event[model_id][0];

		msg_info.dest_id = DIVOOM_MODEL_APP_TOOL;
		msg_info.msg_id = mode_to_event[model_id][2];
		switch(light_mode)
		{
			//0 pause, 1, start, 2, exit
			case DIVOOM_DISP_WATCH_MODE:
				{
					msg_info.data = (uint8 *)&watch_info;
					divoom_sys_model_crl_msg_push(&msg_info);
					data[len++] = watch_info.ctrl_flag;
					//divoom_print("watch_info.ctrl_flag = %d\n", watch_info.ctrl_flag);
				}
				break;
			case DIVOOM_DISP_SCORE_MODE:
				{
					msg_info.data = (uint8 *)&score_info;
					divoom_sys_model_crl_msg_push(&msg_info);
					data[len++] = score_info.on_off;
					data[len++] = score_info.red_score & 0xFF;
					data[len++] = score_info.red_score >> 8;
					data[len++] = score_info.blue_score & 0xFF;
					data[len++] = score_info.blue_score >> 8;
				}
				break;
			case DIVOOM_DISP_NOISE_MODE:
				{
					msg_info.data = data;
					divoom_sys_model_crl_msg_push(&msg_info);
					len = 1;
				}
				break;
			case DIVOOM_DISP_COUNT_TIME_DOWN_MODE:
				{
					msg_info.data = (uint8 *)&count_info;
					divoom_sys_model_crl_msg_push(&msg_info);
					data[len++] = count_info.status;
					data[len++] = count_info.minute;
					data[len++] = count_info.second;
					//divoom_print("count_info.status = %d--%d--%d\n", count_info.status, count_info.minute, count_info.second);
				}
				break;
			default:
				divoom_print("No This Mode In Here:%s, %d, command:%d\n", __func__, __LINE__, model_id);
				break;
		}
	}
	return len;
}

//返回灯效模式
void divoom_app_tool_set_event(uint8 model_id, uint8 *data)
{
    divoom_sys_msg_info msg_info = {0};
	divoom_disp_ctrl_watch_ctrl watch_info;
	divoom_app_tool_score_info score_info;
	divoom_app_tool_count_time_down_info count_info;
	uint8 light_mode = mode_to_event[model_id][0];

	msg_info.dest_id = DIVOOM_MODEL_APP_TOOL;
	msg_info.msg_id = mode_to_event[model_id][1];
	switch(light_mode)
	{
		//0 pause, 1, start, 2, exit
		case DIVOOM_DISP_WATCH_MODE:
			{
				watch_info.ctrl_flag = data[0];
				//divoom_print("SET-->watch_info.ctrl_flag = %d\n", watch_info.ctrl_flag);
				//如果在运行状态变化为暂停状态,要得到pause_time
				/*if(pdivoom_app_tool_handle && pdivoom_app_tool_handle->ctrl_flag == 2)
				{
					pdivoom_app_tool_handle->pause_time = 0;
				}
				if(pdivoom_app_tool_handle && pdivoom_app_tool_handle->ctrl_flag == 1 && watch_info.ctrl_flag == 0)
				{
					pdivoom_app_tool_handle->pause_time = anyka_to_divoom_get_system_utc_time() - pdivoom_app_tool_handle->start_time;
				}
				//watch_info.start_time = Fwl_GetSystemUTCTime();
				watch_info.start_time = anyka_to_divoom_get_system_utc_time();*/
				watch_info.watch_disp_value = pdivoom_app_tool_handle->watch_disp_value;
				msg_info.data = (uint8 *)&watch_info;
			}
			break;
		case DIVOOM_DISP_SCORE_MODE:
			{
				score_info.on_off = data[0];
				score_info.red_score = data[1] + ((uint16)data[2] << 8);
				score_info.blue_score = data[3] + ((uint16)data[4] << 8);
				msg_info.data = (uint8 *)&score_info;
			}
			break;
		case DIVOOM_DISP_NOISE_MODE:
			{
				msg_info.data = data;
			}
			break;
		case DIVOOM_DISP_COUNT_TIME_DOWN_MODE:
			{
				count_info.status = ((Pdivoom_app_tool_count_time_down_info)data)->status;
				count_info.minute = ((Pdivoom_app_tool_count_time_down_info)data)->minute;
				count_info.second = ((Pdivoom_app_tool_count_time_down_info)data)->second;
				msg_info.data = (uint8 *)&count_info;
				//divoom_print("SET--->count_info.status = %d--%d--%d\n", count_info.status, count_info.minute, count_info.second);
			}
			break;
		default:
			divoom_print("No This Mode In Here:%s, %d, command:%d\n", __func__, __LINE__, model_id);
			break;
	}
	divoom_sys_model_crl_msg_push(&msg_info);
}
#endif


uint8 divoom_app_tool_event(Pdivoom_sys_msg_info msg)
{
	//uint32 cur_cnt;
	divoom_app_schedule_item_info event_info = {0};
	struct timeval t_start;
	divoom_print("event:%d\n", msg->msg_id);
	if(msg && pdivoom_app_tool_handle)
	{
		switch(msg->msg_id)
		{
			case DIVOOM_APP_COUNT_TIME_DOWN_SET:
				{
					Pdivoom_app_tool_count_time_down_info info = (Pdivoom_app_tool_count_time_down_info)msg->data;

                    //cur_cnt = anyka_to_divoom_get_system_utc_time();
					//发送的是停止命令
					if(info->status == 0)
					{
						/* 先停到点铃声，再暂停（避免改 status 后 pause 回调不再 stop_voice） */
						divoom_app_tool_countdown_stop_over_alert();
						divoom_app_tool_countdown_apply_pause(info);
						if (info->lvgl_flag != 1)
						{
							divoom_net_app_send_packet(DIVOOM_NET_COMM_IFFT_TRIGGER_DOWNCLOCK_STOP, DIVOOM_SERVER_TOPIC, NULL);
						}
						if(divoom_app_tool_get_schdule(DIVOOM_TOOL_COUNT_TIME_DOWN_DISP))
						{
							divoom_app_tool_set_disp_mode(DIVOOM_TOOL_COUNT_TIME_DOWN_DISP);
						}
					}
					else if(info->status == 1)
					{
						divoom_app_tool_countdown_stop_over_alert();
						if(pdivoom_app_tool_handle->status != DIVOOM_DISP_COUNT_TIME_DOWN_PHONE_CANCEL 
								|| pdivoom_app_tool_handle->status != DIVOOM_DISP_COUNT_TIME_DOWN_TIME_OVER)
						{
							if(divoom_app_tool_get_schdule(DIVOOM_TOOL_COUNT_TIME_DOWN_START))
							{
								//运行状态
								pdivoom_app_tool_handle->status = DIVOOM_DISP_COUNT_TIME_DOWN_RUN;
								pdivoom_app_tool_handle->count_paused = 0;
								pdivoom_app_tool_handle->hour = info->hour;
								pdivoom_app_tool_handle->minute = info->minute;
								pdivoom_app_tool_handle->second = info->second;
                                
                                //divoom_net_app_send_packet(DIVOOM_NET_COMM_IFFT_TRIGGER_DOWNCLOCK_START, DIVOOM_SERVER_TOPIC, NULL);
								pdivoom_app_tool_handle->next_reach_time = time(0) + pdivoom_app_tool_handle->hour * 3600 + pdivoom_app_tool_handle->minute * 60 + pdivoom_app_tool_handle->second;
								pdivoom_app_tool_handle->next_over_time = time(0) + pdivoom_app_tool_handle->hour * 3600 + pdivoom_app_tool_handle->minute * 60 + pdivoom_app_tool_handle->second + 4;
								divoom_print("pdivoom_app_tool_handle->next_reach_time = %d\n", (int)pdivoom_app_tool_handle->next_reach_time);
								divoom_app_tool_set_disp_mode(DIVOOM_TOOL_COUNT_TIME_DOWN_START);
							}
						}
					}
					else//删除倒计时时间
					{
						if (info->lvgl_flag == 1) 
						{
							if(pdivoom_app_tool_handle->status  == DIVOOM_DISP_COUNT_TIME_DOWN_TIME_OVER)
							{
								uint8 event_id = 0;
								event_info.model_id = divoom_app_schedule_get_active_model(&event_id,DIVOOM_APP_SCHEDULE_RESOURCCE_RGB);
		
								if((event_info.model_id == DIVOOM_MODEL_APP_TOOL) && (event_id == DIVOOM_APP_COOUNT_TIME_DOWN_OVER_EVENT_PLAY))
								{
									memset(&event_info, 0, sizeof(divoom_app_schedule_item_info));
        							event_info.model_id = DIVOOM_MODEL_APP_TOOL;
        							event_info.event_id = DIVOOM_APP_COOUNT_TIME_DOWN_OVER_EVENT_PLAY;
									event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
        							divoom_app_schedule_cancel(&event_info);
									break;
								}
							}
							pdivoom_app_tool_handle->next_over_time = 4;
							pdivoom_app_tool_handle->status = DIVOOM_DISP_COUNT_TIME_DOWN_WAIT;
							pdivoom_app_tool_handle->count_paused = 0;
							pdivoom_app_tool_handle->hour = 0;
							pdivoom_app_tool_handle->minute = 1;
							pdivoom_app_tool_handle->second = 0;
						}
					}
				}
				break;
			case DIVOOM_APP_COUNT_TIME_DOWN_GET:
				{
					if(pdivoom_app_tool_handle->status == DIVOOM_DISP_COUNT_TIME_DOWN_RUN)
						((Pdivoom_app_tool_count_time_down_info)msg->data)->status = 1;
					else
						((Pdivoom_app_tool_count_time_down_info)msg->data)->status = 0;

					//从 next_reach_time 获取分秒
					//if(pdivoom_app_tool_handle->next_reach_time == 0)
					//{
						((Pdivoom_app_tool_count_time_down_info)msg->data)->hour = pdivoom_app_tool_handle->hour;
						((Pdivoom_app_tool_count_time_down_info)msg->data)->minute = pdivoom_app_tool_handle->minute;
						((Pdivoom_app_tool_count_time_down_info)msg->data)->second = pdivoom_app_tool_handle->second;
					if(divoom_app_tool_get_schdule(DIVOOM_TOOL_COUNT_TIME_DOWN_START))
					{
						divoom_app_tool_set_disp_mode(DIVOOM_TOOL_COUNT_TIME_DOWN_START);
					}
					//}
					//else
					//{
						//cur_cnt = pdivoom_app_tool_handle->next_reach_time - anyka_to_divoom_get_system_utc_time();
						//((Pdivoom_app_tool_count_time_down_info)msg->data)->minute = cur_cnt/60;
						//((Pdivoom_app_tool_count_time_down_info)msg->data)->second = cur_cnt%60;
					//}
					//((Pdivoom_app_tool_count_time_down_info)msg->data)->minute = pdivoom_app_tool_handle->minute;
					//((Pdivoom_app_tool_count_time_down_info)msg->data)->second = pdivoom_app_tool_handle->second;
					//获取当前状态
					/*Pdivoom_app_tool_count_time_down_info info = (Pdivoom_app_tool_count_time_down_info)divoom_malloc(sizeof(divoom_app_tool_count_time_down_info), __LINE__, __func__);
					info->minute = pdivoom_app_tool_handle->minute;
					info->second = pdivoom_app_tool_handle->second;
					if(info->status == DIVOOM_DISP_COUNT_TIME_DOWN_RUN)
						pdivoom_app_tool_handle->status = 0;
					else
						pdivoom_app_tool_handle->status = 1;
					msg->data = (uint8 *)info;*/
				}
				break;

			case DIVOOM_APP_SCORE_SET:
				{
					if(divoom_app_tool_get_schdule(DIVOOM_TOOL_SCORE_SET))
					{
						pdivoom_app_tool_handle->on_off = ((Pdivoom_app_tool_score_info)msg->data)->on_off;
						pdivoom_app_tool_handle->score_red_score = ((Pdivoom_app_tool_score_info)msg->data)->red_score;
						pdivoom_app_tool_handle->score_blue_score = ((Pdivoom_app_tool_score_info)msg->data)->blue_score;
						/* 已在记分牌页只更新数值，由 UI 轮询刷新；否则 clearall 打开记分牌 */
						if(pdivoom_app_tool_handle->cur_mode_set != DIVOOM_DISP_SCORE_MODE
							|| divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_ID_SCORE_BOARD_MENU)
						{
							divoom_app_tool_set_disp_mode(DIVOOM_TOOL_SCORE_SET);
						}
						else
						{
							pdivoom_app_tool_handle->cur_mode_set = DIVOOM_DISP_SCORE_MODE;
						}
					}
				}
				break;
			case DIVOOM_APP_SCORE_GET:
				{
					((Pdivoom_app_tool_score_info)msg->data)->on_off = pdivoom_app_tool_handle->on_off;
					((Pdivoom_app_tool_score_info)msg->data)->red_score = pdivoom_app_tool_handle->score_red_score;
					((Pdivoom_app_tool_score_info)msg->data)->blue_score = pdivoom_app_tool_handle->score_blue_score;
					if(divoom_app_tool_get_schdule(DIVOOM_TOOL_SCORE_SET))
					{
						divoom_app_tool_set_disp_mode(DIVOOM_TOOL_SCORE_SET);
					}
				}
				break;
			case DIVOOM_APP_TIME_WATCH_SET:
				{
					if(divoom_app_tool_get_schdule(DIVOOM_TOOL_WATCH_SET))
					{
						gettimeofday(&t_start, NULL);
						pdivoom_app_tool_handle->ctrl_flag = ((Pdivoom_app_tool_time_watch_info)msg->data)->ctrl_flag;
						//处理2重置的问题
						if(pdivoom_app_tool_handle->ctrl_flag == 2)
						{
							pdivoom_app_tool_handle->watch_disp_value = 0;
							pdivoom_app_tool_handle->watch_start_time = 0;
						}
						else if(pdivoom_app_tool_handle->ctrl_flag == 1)
						{
                            //pdivoom_app_tool_handle->watch_start_time = time(0) - pdivoom_app_tool_handle->watch_disp_value;
							pdivoom_app_tool_handle->watch_start_time = (t_start.tv_sec - pdivoom_app_tool_handle->watch_disp_value/100)*100 + t_start.tv_usec/1000/10 - pdivoom_app_tool_handle->watch_disp_value%100;
						}
                        else
                        {
							//pdivoom_app_tool_handle->watch_disp_value = time(0) - pdivoom_app_tool_handle->watch_start_time;
							pdivoom_app_tool_handle->watch_disp_value = (t_start.tv_sec - pdivoom_app_tool_handle->watch_start_time/100)*100 + t_start.tv_usec/1000/10 - pdivoom_app_tool_handle->watch_start_time%100;
                        }

						/* 已在秒表页只更新后台，由 LVGL timer 轮询刷新；禁止在模型/APP 线程改 LVGL */
						if(divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_ID_STOP_WATCH_MENU)
						{
							divoom_app_tool_set_disp_mode(DIVOOM_TOOL_WATCH_SET);
						}
						else
						{
							pdivoom_app_tool_handle->cur_mode_set = DIVOOM_DISP_WATCH_MODE;
						}
					}
				}
				break;
			case DIVOOM_APP_TIME_WATCH_GET:
				{
					((Pdivoom_app_tool_time_watch_info)msg->data)->ctrl_flag = pdivoom_app_tool_handle->ctrl_flag;
					if(divoom_app_tool_get_schdule(DIVOOM_TOOL_WATCH_SET))
					{
						if(divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_ID_STOP_WATCH_MENU)
						{
							divoom_app_tool_set_disp_mode(DIVOOM_TOOL_WATCH_SET);
						}
						else
						{
							pdivoom_app_tool_handle->cur_mode_set = DIVOOM_DISP_WATCH_MODE;
						}
					}
				}
				break;
			case DIVOOM_APP_NOISE_SET:
				{
					if(divoom_app_tool_get_schdule(DIVOOM_TOOL_NOISE_SET))
					{
						divoom_print("DIVOOM_APP_NOISE_SET:%d, pdivoom_app_tool_handle->noise_flag = %d\n", *((uint8 *)msg->data), pdivoom_app_tool_handle->noise_flag);
						if(pdivoom_app_tool_handle->noise_flag == 1 && (*((uint8 *)msg->data) == 0))
						{
							//噪音在这里处理
							//divoom_app_schedule_item_info event_info = {0};
							//pdivoom_app_tool_handle->last_is_noise = 0;
							//pdivoom_app_tool_handle->pre_mode_set = 0xFF;
							pdivoom_app_tool_handle->noise_flag = *((uint8 *)msg->data);
							pdivoom_app_tool_handle->cur_mode_set = 0xFF;

							//从开到关的状态要直接取消当前优先级
							//噪音主动取消了,证明在这个界面取消了,退出工具优先级
							event_info.model_id = DIVOOM_MODEL_APP_TOOL;
							event_info.event_id = DIVOOM_APP_TOOL_EVENT_PLAY;
							event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
							divoom_app_schedule_cancel(&event_info);
							return 1;
						}
						else if((pdivoom_app_tool_handle->noise_flag == 1) && (*((uint8 *)msg->data) == 1))
						{
							//没有打开,那就不进行任何显示
							return 1;
						}
						pdivoom_app_tool_handle->noise_flag = *((uint8 *)msg->data);
						//在运行的时候,再设置失败,不进行清除
						divoom_app_tool_set_disp_mode(DIVOOM_TOOL_NOISE_SET);
					}
				}
				break;
			case DIVOOM_APP_NOISE_GET:
				{
					divoom_print("DIVOOM_APP_NOISE_GET:pdivoom_app_tool_handle->noise_flag = %d\n", pdivoom_app_tool_handle->noise_flag);
					*(msg->data) = pdivoom_app_tool_handle->noise_flag;
					if(divoom_app_tool_get_schdule(DIVOOM_TOOL_NOISE_SET))
					{
						divoom_app_tool_set_disp_mode(DIVOOM_TOOL_NOISE_SET);
					}
				}
				break;
			case DIVOOM_APP_LVGL_DISP_EXIT:
				divoom_app_tool_exit_cur_mode(1);
				break;
			default:
				divoom_print("No This Command In %s--%d, command = %d\n", __func__, __LINE__, msg->msg_id);
		}

		return 1;
	}

	return 0;
}

uint8 divoom_app_tool_schedule_play_callback(uint8 event_id, void *user_data,uint32 resouce_type)
{
	divoom_disp_high_pri_event_struct disp_info = {0};
	divoom_disp_count_time_down_struct run_info = {0};
	divoom_disp_ctrl_score_ctrl score_info = {0};
	divoom_disp_ctrl_watch_ctrl watch_info = {0};
	//如果还在这个显示的时间内,继续显示
	divoom_print("pdivoom_app_tool_handle->next_over_time = %d--status = %d\n", pdivoom_app_tool_handle->next_over_time, pdivoom_app_tool_handle->status);
	divoom_print("pdivoom_app_tool_handle->cur_mode_set = %d\n", pdivoom_app_tool_handle->cur_mode_set);
	//如果是倒计时提醒时间到事件,分开处理,不然会出错
	if(event_id == DIVOOM_APP_COOUNT_TIME_DOWN_OVER_EVENT_PLAY)
	{
		if(pdivoom_app_tool_handle->next_over_time > 10 && pdivoom_app_tool_handle->status == DIVOOM_DISP_COUNT_TIME_DOWN_TIME_OVER)
		{
			disp_info.on_time = 5;
			disp_info.period_time = 5;
			disp_info.tol_time = 50;
			disp_info.tol_pic = 2;
			disp_info.disp_id[0] = DIVOOM_DISP_TIMEROVER;
			disp_info.disp_id[1] = DIVOOM_DISP_TIMEROVER_NO;

			disp_info.extend_info.model_id = DIVOOM_MODEL_APP_TOOL;
			disp_info.extend_info.event_id = DIVOOM_APP_COOUNT_TIME_DOWN_OVER_EVENT_PLAY;

			//divoom_disp_ctrl_set_mode(DIVOOM_DISP_HIGH_PRI_EVENT_DIS_CTRL, (uint8 *)&disp_info);
			divoom_app_tool_open_countdown_ui(0);
			//divoom_print("disp_info.extend_info.model_id = %d--%d\n", disp_info.extend_info.model_id, disp_info.extend_info.event_id);
			//不能被打断,这里是遇到高优先的时候,要从新开始计算响应时间
			//pdivoom_app_tool_handle->next_over_time = anyka_to_divoom_get_system_utc_time() + 10;
			if(pdivoom_app_tool_handle->next_over_time != T_S32_MAX)
			{
			    divoom_app_tool_count_down_play_voice(0);
			}
			pdivoom_app_tool_handle->next_over_time = 5;
		}
	}
	else
	{
		switch(pdivoom_app_tool_handle->cur_mode_set)
		{
			case DIVOOM_DISP_COUNT_TIME_DOWN_MODE:
				{
					//如果倒计时在时间到了体型阶段
					//因为可能在响的阶段设置这个值
					if(pdivoom_app_tool_handle->next_over_time > 10 && pdivoom_app_tool_handle->status == DIVOOM_DISP_COUNT_TIME_DOWN_TIME_OVER)
					{
						disp_info.on_time = 5;
						disp_info.period_time = 5;
						disp_info.tol_time = 50;
						disp_info.tol_pic = 2;
						disp_info.disp_id[0] = DIVOOM_DISP_TIMEROVER;
						disp_info.disp_id[1] = DIVOOM_DISP_TIMEROVER_NO;

						disp_info.extend_info.model_id = DIVOOM_MODEL_APP_TOOL;
						disp_info.extend_info.event_id = DIVOOM_APP_COOUNT_TIME_DOWN_OVER_EVENT_PLAY;

						divoom_app_tool_open_countdown_ui(0);
						//divoom_disp_ctrl_set_mode(DIVOOM_DISP_HIGH_PRI_EVENT_DIS_CTRL, (uint8 *)&disp_info);
						//divoom_print("disp_info.extend_info.model_id = %d--%d\n", disp_info.extend_info.model_id, disp_info.extend_info.event_id);
						//不能被打断,这里是遇到高优先的时候,要从新开始计算响应时间
						//pdivoom_app_tool_handle->next_over_time = anyka_to_divoom_get_system_utc_time() + 10;
						divoom_app_tool_count_down_play_voice(0);
						pdivoom_app_tool_handle->next_over_time = 5;
					}
					//回到这里的时候倒计时已经结束了,那就显示1分0秒
					else if(pdivoom_app_tool_handle->status == DIVOOM_DISP_COUNT_TIME_DOWN_WAIT)
					{
						run_info.next_reach_time = pdivoom_app_tool_handle->minute*60 + pdivoom_app_tool_handle->second;
						run_info.status = DIVOOM_DISP_COUNT_TIME_DOWN_WAIT;
						//divoom_disp_ctrl_set_mode(DIVOOM_DISP_COUNT_TIME_DOWN_MODE, (uint8 *)&run_info);
						divoom_app_tool_open_countdown_ui(0);
					}
					else if(pdivoom_app_tool_handle->next_reach_time != 0)
					{
						//如果再运行阶段
						run_info.next_reach_time = pdivoom_app_tool_handle->next_reach_time - time(0);
						run_info.status = DIVOOM_DISP_COUNT_TIME_DOWN_RUN;
						//divoom_disp_ctrl_set_mode(DIVOOM_DISP_COUNT_TIME_DOWN_MODE, (uint8 *)&run_info);
						divoom_app_tool_open_countdown_ui(0);
					}
					//至于运行结束的状态不会出现,会在pause的时候处理好 cur_mode_set
					//如果是在倒计时被可自动取消的打断,在自动取消的事件结束之前,被倒计时时间到打断
				}
				break;
			case DIVOOM_DISP_SCORE_MODE:
				{
					score_info.red_score = pdivoom_app_tool_handle->score_red_score;
					score_info.blue_score = pdivoom_app_tool_handle->score_blue_score;
					divoom_app_tool_try_restore_menu(DIVOOM_MENU_CONTROL_ID_SCORE_BOARD_MENU);
					//divoom_disp_ctrl_set_mode(DIVOOM_DISP_SCORE_MODE, (uint8 *)&score_info);
				}
				break;
			case DIVOOM_DISP_WATCH_MODE:
				{
					watch_info.ctrl_flag = pdivoom_app_tool_handle->ctrl_flag;
					//pdivoom_app_tool_handle->start_time = anyka_to_divoom_get_system_utc_time();
					//watch_info.start_time = pdivoom_app_tool_handle->start_time;
					//watch_info.pause_time = pdivoom_app_tool_handle->pause_time;
					watch_info.watch_disp_value = pdivoom_app_tool_handle->watch_disp_value;
					//divoom_disp_ctrl_set_mode(DIVOOM_DISP_WATCH_MODE, (uint8 *)&watch_info);
					divoom_app_tool_try_restore_menu(DIVOOM_MENU_CONTROL_ID_STOP_WATCH_MENU);
				}
				break;
			case DIVOOM_DISP_NOISE_MODE:
				{
					if(pdivoom_app_tool_handle->noise_flag == 1)
					{
						//divoom_disp_ctrl_set_mode(DIVOOM_DISP_NOISE_MODE, &pdivoom_app_tool_handle->noise_flag);
						divoom_app_tool_try_restore_menu(DIVOOM_MENU_CONTROL_ID_NOISE_MEASURE_MENU);
					}
				}
				break;
			default:
				{
				}
				break;
		}
	}
	return 1;
}

/*switch(divoom_app_tool_get_pre_mode())
{
	case 0xFF:
		{
			return 0;
		}
		break;
	case DIVOOM_DISP_NOISE_MODE:
	case DIVOOM_DISP_SCORE_MODE:
	case DIVOOM_DISP_WATCH_MODE:
		{
			pdivoom_app_tool_handle->cur_mode_set = divoom_app_tool_get_pre_mode();
			return 1;
		}
		break;
}*/

uint8 divoom_app_tool_schedule_pause_callback(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type)
{
	//divoom_print("divoom_app_tool_get_pre_mode() = %d\n", divoom_app_tool_get_pre_mode());
	divoom_print("active_model = %d, active_event = %d\n", active_model, active_event);
	divoom_print("pdivoom_app_tool_handle->cur_mode_set = %d, pdivoom_app_tool_handle->status = %d\n", pdivoom_app_tool_handle->cur_mode_set, pdivoom_app_tool_handle->status);
	if(pdivoom_app_tool_handle->cur_mode_set == DIVOOM_DISP_COUNT_TIME_DOWN_MODE && pdivoom_app_tool_handle->status == DIVOOM_DISP_COUNT_TIME_DOWN_TIME_OVER)
	{
		//设置倒计时提醒被打断的标志
		divoom_app_tool_count_down_stop_voice();
		pdivoom_app_tool_handle->next_over_time = T_S32_MAX;
		return 1;
	}

	//如果是被倒计时时间到提醒打断的
	//工具类被倒计时时间到提醒打断,要恢复到前一个事件,如果前一个是倒计时,那就取消工具的优先级;如果是噪音,那就回到噪音,但是噪音再取消,就要结束优先级
	if(active_model == DIVOOM_MODEL_APP_TOOL && active_event == DIVOOM_APP_COOUNT_TIME_DOWN_OVER_EVENT_PLAY)
	{
		switch(pdivoom_app_tool_handle->cur_mode_set)
		{
			//如果实在倒计时被打断的,那就直接over掉当前事件,并且销毁掉之前的模式存档
			//倒计时被打断,要可以保存之前的信息,并在倒计时时间到提醒结束把倒计时的数据还原
			case DIVOOM_DISP_COUNT_TIME_DOWN_MODE:
				/*{
					//不改变当前所在的模式信息
					//pdivoom_app_tool_handle->last_is_noise = 0;
					//pdivoom_app_tool_handle->pre_mode_set = 0xFF;
					//pdivoom_app_tool_handle->cur_mode_set = 0xFF;
					return 0;
				}
				break;*/
			case DIVOOM_DISP_SCORE_MODE:
			case DIVOOM_DISP_WATCH_MODE:
			case DIVOOM_DISP_NOISE_MODE:
				return 1;
				break;
			default:
				{
					return 0;
				}
				break;
		}
	}
	else
	{

#if 0
		//区分为两类,可自动取消的事件和不能自动取消的事件,能自动取消的新效果就是可以被暂停,否侧就是不能被暂停,直接退出
		switch(active_model)
		{
			case DIVOOM_MODEL_APP_MAIN_MODEL:
				{
					if(active_event == DIVOOM_MAIN_SCHEDULE_POWER_ON_PIC)
					{
						return 1;
					}
					else
					{
						return 0;
					}
				}
				break;
			case DIVOOM_MODEL_APP_DOWNLOAD:
				if(active_event == DIVOOM_APP_DOWNLOAD_SCHEDULE_WAIT_DOWNLOAD)
				{
					return 0;
				}
				else
				{
					return 1;
				}
				break;
			case DIVOOM_MODEL_APP_ANCS_MODEL:
			case DIVOOM_MODEL_APP_ALARM:
			case DIVOOM_MODEL_APP_SLEEP:
			case DIVOOM_MODEL_APP_WEATHER_MODEL:
			case DIVOOM_MODEL_APP_GAME:
			case DIVOOM_MODEL_APP_TIME_MANAGE:
				return 1;
				break;
			case DIVOOM_MODEL_APP_DRAW:
				return 0;
				break;
			default:
				return 0;
				break;
		}
#endif
		return 1;
	}
	
	return 1;
}

uint8 divoom_app_tool_schedule_destroy_callback(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type)
{
	if(event_id == DIVOOM_APP_COOUNT_TIME_DOWN_OVER_EVENT_PLAY)
	{
		uint8 over_info[8];
		pdivoom_app_tool_handle->status = DIVOOM_DISP_COUNT_TIME_DOWN_WAIT;
		pdivoom_app_tool_handle->count_paused = 0;
		pdivoom_app_tool_handle->next_over_time = 0;
		pdivoom_app_tool_handle->next_reach_time = 0;
		pdivoom_app_tool_handle->hour = 0;
		pdivoom_app_tool_handle->minute = 1;
		pdivoom_app_tool_handle->second = 0;

		//仅仅是针对 DIVOOM_APP_COOUNT_TIME_DOWN_OVER_EVENT_PLAY 事件进行的反馈
		memset(over_info, 0, sizeof(over_info));
		over_info[0] = 3;
		over_info[1] = pdivoom_app_tool_handle->minute;
		over_info[2] = pdivoom_app_tool_handle->second;
		over_info[3] = 1;

		divoom_app_tool_count_down_stop_voice();
//		divoom_app_protocol_OKCommandACK(AK_NULL, SPP_GET_TOOL_INFO, (T_U8 *)&over_info, sizeof(over_info));
	}
	//不会再销毁下面几个事件了
	/*switch(pdivoom_app_tool_handle->cur_mode_set)
	{
		case DIVOOM_DISP_COUNT_TIME_DOWN_MODE:
            break;
            
		case DIVOOM_DISP_SCORE_MODE:
			{
				pdivoom_app_tool_handle->score_red_score = 0;
				pdivoom_app_tool_handle->score_blue_score = 0;
				pdivoom_app_tool_handle->on_off = 0;
				pdivoom_app_tool_handle->cur_mode_set = 0xFF;
			}
			break;
		case DIVOOM_DISP_WATCH_MODE:
			{
				pdivoom_app_tool_handle->start_time = 0;
				pdivoom_app_tool_handle->pause_time = 0;
				pdivoom_app_tool_handle->ctrl_flag = 2;
				pdivoom_app_tool_handle->cur_mode_set = 0xFF;
			}
			break;
		case DIVOOM_DISP_NOISE_MODE:
			{
				pdivoom_app_tool_handle->noise_flag = 2;
				pdivoom_app_tool_handle->cur_mode_set = 0xFF;
			}
			break;
		default:
			{
			}
			break;
	}*/
	return 1;
}

//不需要处理按键,发送到系统处理
uint8 divoom_app_tool_scdedule_key_callback(uint8 key, uint8 key_status, uint32 resouce_type)
{
	divoom_app_schedule_item_info event_info = {0};
	uint8 event_id = 0, event_model = 0;
	event_model = divoom_app_schedule_get_active_model(&event_id,DIVOOM_APP_SCHEDULE_RESOURCCE_RGB);
	divoom_print("%s-->event_model = %d, event_id = %d\n", __func__, event_model, event_id);
	//在倒计时时间到提醒阶段,要可以按键取消提醒
	//pdivoom_app_tool_handle->next_over_time = anyka_to_divoom_get_system_utc_time();
	if(event_model == DIVOOM_MODEL_APP_TOOL)
	{
		//查看当前有没开启计分板秒表噪音等
		switch(event_id)
		{
            case DIVOOM_APP_COOUNT_TIME_DOWN_OVER_EVENT_PLAY:
                
        		/*switch(divoom_app_tool_get_pre_mode())
        		{
        			case DIVOOM_DISP_NOISE_MODE:
        			case DIVOOM_DISP_SCORE_MODE:
        			case DIVOOM_DISP_WATCH_MODE:
        				{
        					pdivoom_app_tool_handle->cur_mode_set = divoom_app_tool_get_pre_mode();
        				}
        				break;
        			default:
        				{
        					pdivoom_app_tool_handle->last_is_noise = 0;
        					pdivoom_app_tool_handle->pre_mode_set = 0xFF;
        				}
        				break;
        		}*/
        		//取消当前优先级
        		memset(&event_info, 0, sizeof(divoom_app_schedule_item_info));
        		event_info.model_id = DIVOOM_MODEL_APP_TOOL;
        		event_info.event_id = DIVOOM_APP_COOUNT_TIME_DOWN_OVER_EVENT_PLAY;
				event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
        		divoom_app_schedule_cancel(&event_info);
        		return 2;
				break;

           case DIVOOM_APP_TOOL_EVENT_PLAY:
                if(key == DIVOOM_KEY_ESC)
                {
					//在运行阶段,那么一定是在这个模块的,直接运行取消画面
					if(pdivoom_app_tool_handle->status == DIVOOM_DISP_COUNT_TIME_DOWN_RUN)
					{
						//只存储一下结束时间就行了
						pdivoom_app_tool_handle->next_over_time = 4;

						pdivoom_app_tool_handle->status = DIVOOM_DISP_COUNT_TIME_DOWN_PHONE_CANCEL;
						pdivoom_app_tool_handle->hour = 0;
						pdivoom_app_tool_handle->minute = 1;
						pdivoom_app_tool_handle->second = 0;
						divoom_app_tool_set_disp_mode(DIVOOM_TOOL_COUNT_TIME_DOWN_CANCEL);
						return 2;
					}
                    pdivoom_app_tool_handle->noise_flag = 2;
            		memset(&event_info, 0, sizeof(divoom_app_schedule_item_info));
            		event_info.model_id = DIVOOM_MODEL_APP_TOOL;
            		event_info.event_id = DIVOOM_APP_TOOL_EVENT_PLAY;
					event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
            		divoom_app_schedule_cancel(&event_info);
            		return 2;
                }
				break;
		}
        
	}
	return 0;
}

void divoom_app_tool_set_watch_lap_nums(uint32 *lap_time, uint32 lap_num)
{
	if(lap_num >= 100)
	{
		return;
	}
	
	if(pdivoom_app_tool_handle)
	{
		int i;
		
		pdivoom_app_tool_handle->lap_num = lap_num;
		for(i = 0;i < lap_num; i++)
		{
			pdivoom_app_tool_handle->lap_time[i] = lap_time[i];
		}
	}
}

void divoom_app_tool_get_watch_lap_nums(int *lap_num, uint32 *lap_time)
{	
	if(pdivoom_app_tool_handle)
	{
		int i;
		
		(*lap_num) =  pdivoom_app_tool_handle->lap_num;
		for(i = 0;i < pdivoom_app_tool_handle->lap_num; i++)
		{
			lap_time[i] = pdivoom_app_tool_handle->lap_time[i];
		}
	}

}

void divoom_app_tool_get_watch_cnt(Pdivoom_disp_ctrl_watch_ctrl watch_info)
{
	if(pdivoom_app_tool_handle && watch_info)
	{
		struct timeval t_start;
		
		watch_info->ctrl_flag = pdivoom_app_tool_handle->ctrl_flag;
        if(pdivoom_app_tool_handle->ctrl_flag == 1)
        {
		    //watch_info->watch_disp_value = time(0) - pdivoom_app_tool_handle->watch_start_time;
		    gettimeofday(&t_start, NULL);
			watch_info->watch_disp_value = (t_start.tv_sec - pdivoom_app_tool_handle->watch_start_time/100)*100 + t_start.tv_usec/1000/10 - pdivoom_app_tool_handle->watch_start_time%100;
        }
        else if(pdivoom_app_tool_handle->ctrl_flag == 0)
        {
		    watch_info->watch_disp_value = pdivoom_app_tool_handle->watch_disp_value;
        }
        else
        {
            watch_info->watch_disp_value = 0;
        }
	}
	return;
}

void divoom_app_tool_count_down_menu_try_listen(uint8 voice_id, uint8 vol, uint8 on_off)
{
	char file_name[128];

	if(on_off == 0)
    {
		divoom_local_music_ctrl_stop_tone();
        return;
    }

	divoom_voice_read_get_filename(divoom_app_tool_count_down_voice_table[voice_id], file_name);
    divoom_local_music_ctrl_tone_play(file_name, vol, NULL, 60);
}

void divoom_app_tool_get_score(Pdivoom_app_tool_score_info score_info)
{
	if (pdivoom_app_tool_handle && score_info)
	{
		score_info->red_score = pdivoom_app_tool_handle->score_red_score;
		score_info->blue_score = pdivoom_app_tool_handle->score_blue_score;
	}
}


uint32 divoom_app_tool_get_count_down_next_reach_time(void)
{
	if(pdivoom_app_tool_handle)
	{
        return pdivoom_app_tool_handle->next_reach_time;
	}

	return 0;
}


uint32 divoom_app_tool_get_count_down_cnt(void)
{
    uint32 cur_time;
    
	if(pdivoom_app_tool_handle)
	{
		if(pdivoom_app_tool_handle->status == DIVOOM_DISP_COUNT_TIME_DOWN_RUN)
		{
			cur_time = time(0);
			if(pdivoom_app_tool_handle->next_reach_time > cur_time)
			{
				return pdivoom_app_tool_handle->next_reach_time - cur_time;
			}
			return 0;
		}
		return (uint32)pdivoom_app_tool_handle->hour * 3600u
			+ (uint32)pdivoom_app_tool_handle->minute * 60u
			+ (uint32)pdivoom_app_tool_handle->second;
	}

	return 0;
}

uint8 divoom_app_tool_get_count_down_status(void)
{
	if (pdivoom_app_tool_handle)
	{
		return pdivoom_app_tool_handle->status;
	}
	
	return DIVOOM_DISP_COUNT_TIME_DOWN_WAIT;
}

uint8 divoom_app_tool_get_count_down_paused(void)
{
	if (pdivoom_app_tool_handle)
	{
		return pdivoom_app_tool_handle->count_paused;
	}
	return 0;
}

uint8 divoom_app_tool_get_count_down_music_index(void)
{
	if (pdivoom_app_tool_handle)
	{
		return pdivoom_app_tool_handle->music_index;
	}

	return 0;
}


void divoom_app_tool_set_count_down_music_index(int music_index)
{
	if (pdivoom_app_tool_handle && pdivoom_app_tool_handle->music_index != music_index)
	{
		pdivoom_app_tool_handle->music_index = music_index;
	}
}

void divoom_app_tool_count_down_play_voice(int time)
{
	char file_name[128];

	divoom_voice_read_get_filename(DIVOOM_VOICE_ID_CLOCK_NORMAL1, file_name);
	divoom_local_music_ctrl_tone_play(file_name, divoom_main_get_tone_volume(), NULL, time);
}

void divoom_app_tool_count_down_stop_voice(void)
{
	divoom_local_music_ctrl_stop_tone();
}

void divoom_app_tool_count_cnt_update(void)
{
    uint32 cur_time, next_reach_time, next_over_time;
    
	if(pdivoom_app_tool_handle && pdivoom_app_tool_handle->status != DIVOOM_DISP_COUNT_TIME_DOWN_WAIT)
	{
        cur_time = time(0);
        if(pdivoom_app_tool_handle->next_reach_time > cur_time)
        {
            next_reach_time = pdivoom_app_tool_handle->next_reach_time - cur_time;
        }
        else
        {
            next_reach_time = 0;
        }
        
        if(pdivoom_app_tool_handle->next_over_time > cur_time)
        {
            next_over_time = pdivoom_app_tool_handle->next_over_time - cur_time;
        }
        else
        {
            next_over_time = 0;
        }
		
		switch(pdivoom_app_tool_handle->status)
		{
			case DIVOOM_DISP_COUNT_TIME_DOWN_RUN:
				{
					//时间到了，就去DIVOOM_DISP_tool_TIME_OVER
					if(next_reach_time == 0)
					{
        				divoom_system_tool_json_timer_info send_info = {0};

                        send_info.status = 0;
						send_info.min = pdivoom_app_tool_handle->minute;
						send_info.sec = pdivoom_app_tool_handle->second;
                        divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_TOOL_TIMER_STATUS, DIVOOM_APP_TOPIC, &send_info);
                        divoom_net_app_send_packet(DIVOOM_NET_COMM_IFFT_TRIGGER_DOWNCLOCK_STOP, DIVOOM_SERVER_TOPIC, NULL);
						if(divoom_app_tool_set_disp_mode(DIVOOM_TOOL_COUNT_TIME_DOWN_OVER) == 0)
						{
							pdivoom_app_tool_handle->status = DIVOOM_DISP_COUNT_TIME_DOWN_TIME_OVER;
							pdivoom_app_tool_handle->next_over_time = T_S32_MAX;
							break;
							/*pdivoom_app_tool_handle->status = DIVOOM_DISP_COUNT_TIME_DOWN_WAIT;
							  pdivoom_app_tool_handle->next_reach_time = 0;
							  pdivoom_app_tool_handle->next_over_time = 0;
							  pdivoom_app_tool_handle->minute = 1;
							  pdivoom_app_tool_handle->second = 0;
							  break;*/
						}
						pdivoom_app_tool_handle->status = DIVOOM_DISP_COUNT_TIME_DOWN_TIME_OVER;
						pdivoom_app_tool_handle->next_over_time = 4 + cur_time;
						{
							divoom_app_tool_count_down_play_voice(0);
						}
					}
				}
				break;
			case DIVOOM_DISP_COUNT_TIME_DOWN_TIME_OVER:
				{
					//超过时间回到初始状态
					if(next_over_time == 0)
					{
						divoom_print("%s-->Now To Exit DIVOOM_APP_COOUNT_TIME_DOWN_OVER_EVENT_PLAY!\n", __func__);
					}
				}
				break;
			case DIVOOM_DISP_COUNT_TIME_DOWN_PHONE_CANCEL:
				{
					//超过时间回到初始状态
					//通过取消,证明这里一定是点进来了倒计时页面,这个时候,取消了就完全退出优先级,跟噪音一样的处理
					if(next_over_time == 0)
					{
						//pdivoom_app_tool_handle->cur_mode_set = 0xFF;
						pdivoom_app_tool_handle->status = DIVOOM_DISP_COUNT_TIME_DOWN_WAIT;
						pdivoom_app_tool_handle->count_paused = 0;
						pdivoom_app_tool_handle->next_over_time = 0;
						pdivoom_app_tool_handle->next_reach_time = 0;
						pdivoom_app_tool_handle->cur_mode_set = 0xFF;
						
						divoom_app_tool_exit_cur_mode(1);
					}
				}
				break;
			default:
				break;
		}
	}
}

void divoom_app_tool_main(void)
{
	//divoom_app_schedule_item_info event_info = {0};
	//uint32 cur_cnt = anyka_to_divoom_get_system_utc_time();

	//更新秒表
	divoom_app_tool_count_cnt_update();
}

//取消当前模式显示
//需要发送schedule cancel
void divoom_app_tool_exit_cur_mode(uint8 flag)
{
	divoom_app_schedule_item_info event_info = {0};

	event_info.model_id = DIVOOM_MODEL_APP_TOOL;
	event_info.event_id = DIVOOM_APP_TOOL_EVENT_PLAY;
	event_info.attr = 1;
	event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
	divoom_app_schedule_cancel(&event_info);
}

//需要获取schedule 的优先状态 run
uint8 divoom_app_tool_get_schdule(uint8 flag)
{
	divoom_app_schedule_item_info event_info = {0};

	if(flag != DIVOOM_TOOL_COUNT_TIME_DOWN_OVER)
	{
		uint8 event_id = 0;
		event_info.model_id = divoom_app_schedule_get_active_model(&event_id,DIVOOM_APP_SCHEDULE_RESOURCCE_RGB);
		divoom_debug("flag = %d, event_info.model_id = %d, event_id = %d\n", flag, event_info.model_id, event_id);
		//if((divoom_app_schedule_get_active_model((uint16 *)&(event_info.event_id)) != DIVOOM_MODEL_APP_TOOL) && (event_info.event_id != DIVOOM_APP_TOOL_EVENT_PLAY))
		if((event_info.model_id != DIVOOM_MODEL_APP_TOOL) || (event_id != DIVOOM_APP_TOOL_EVENT_PLAY))
		{
			memset(&event_info, 0, sizeof(divoom_app_schedule_item_info));
			event_info.model_id = DIVOOM_MODEL_APP_TOOL;
			event_info.event_id = DIVOOM_APP_TOOL_EVENT_PLAY;
			event_info.attr = DIVOOM_APP_SCHEDULE_ATTR_CANCEL;
			event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;
			if(0 == divoom_app_schedule_push(&event_info))
			{
				divoom_print("MayBe have a high priority event!\n");
				return 0;
			}
			return 1;
		}
		else
			return 1;
	}
	return 0;
}

//直接set mode
uint8 divoom_app_tool_set_disp_mode(uint8 flag)
{
	divoom_app_schedule_item_info event_info = {0};
    divoom_disp_high_pri_event_struct disp_info = {0};
	divoom_disp_count_time_down_struct run_info = {0};
	divoom_disp_ctrl_score_ctrl score_info = {0};
	divoom_disp_ctrl_watch_ctrl watch_info = {0};

	disp_info.on_time = 5;
	disp_info.period_time = 5;
	switch(flag)
	{
		//进入这个模式显示空的
		case DIVOOM_TOOL_COUNT_TIME_DOWN_DISP:
			{
				run_info.next_reach_time = time(0) + pdivoom_app_tool_handle->minute*60 + pdivoom_app_tool_handle->second;
				run_info.status = DIVOOM_DISP_COUNT_TIME_DOWN_WAIT;
				
				divoom_app_tool_open_countdown_ui(1);
				//divoom_disp_ctrl_set_mode(DIVOOM_DISP_COUNT_TIME_DOWN_MODE, (uint8 *)&run_info);
				pdivoom_app_tool_handle->cur_mode_set = DIVOOM_DISP_COUNT_TIME_DOWN_MODE;
			}
			break;
		case DIVOOM_TOOL_COUNT_TIME_DOWN_CANCEL:
			{
				disp_info.tol_time = 60;
				disp_info.on_time = 5;
				disp_info.period_time = 10;
				disp_info.tol_pic = 1;
				disp_info.disp_id[0] = DIVOOM_DISP_TIMER_CANCEL;
				//disp_info.disp_id[1] = DIVOOM_DISP_TIMERCANCEL_NO;

				disp_info.extend_info.model_id = DIVOOM_MODEL_APP_TOOL;
				disp_info.extend_info.event_id = DIVOOM_APP_TOOL_EVENT_PLAY;

				divoom_app_tool_open_countdown_ui(1);
				//divoom_disp_ctrl_set_mode(DIVOOM_DISP_HIGH_PRI_EVENT_DIS_CTRL, (uint8 *)&disp_info);
				pdivoom_app_tool_handle->cur_mode_set = DIVOOM_DISP_COUNT_TIME_DOWN_MODE;
				//pdivoom_app_tool_handle->cur_mode_set = 0xFF;
			}
			break;
		case DIVOOM_TOOL_COUNT_TIME_DOWN_OVER:
			{
				//这个要求优先级要高一点
				event_info.model_id = DIVOOM_MODEL_APP_TOOL;
				event_info.event_id = DIVOOM_APP_COOUNT_TIME_DOWN_OVER_EVENT_PLAY;
				event_info.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
				event_info.resource_type = DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE;

				if(0 == divoom_app_schedule_push(&event_info))
				{
					divoom_print("MayBe have a high priority event!\n");
					return 0;
				}

				//disp_info.tol_time = 60;
				disp_info.tol_time = 30;
				disp_info.tol_pic = 9;
				disp_info.disp_id[0] = DIVOOM_DISP_TIMER_ALL_14;
				disp_info.disp_id[1] = DIVOOM_DISP_TIMER_ALL_15;
				disp_info.disp_id[2] = DIVOOM_DISP_TIMER_ALL_16;
				disp_info.disp_id[3] = DIVOOM_DISP_TIMER_ALL_17;
				disp_info.disp_id[4] = DIVOOM_DISP_TIMER_ALL_18;
				disp_info.disp_id[5] = DIVOOM_DISP_TIMER_ALL_19;
				disp_info.disp_id[6] = DIVOOM_DISP_TIMER_ALL_20;
				disp_info.disp_id[7] = DIVOOM_DISP_TIMER_ALL_21;
				disp_info.disp_id[8] = DIVOOM_DISP_TIMER_ALL_22;

				disp_info.extend_info.model_id = DIVOOM_MODEL_APP_TOOL;
				disp_info.extend_info.event_id = DIVOOM_APP_COOUNT_TIME_DOWN_OVER_EVENT_PLAY;

				divoom_app_tool_open_countdown_ui(1);
				//divoom_disp_ctrl_set_mode(DIVOOM_DISP_HIGH_PRI_EVENT_DIS_CTRL, (uint8 *)&disp_info);
				//pdivoom_app_tool_handle->cur_mode_set = DIVOOM_DISP_COUNT_TIME_DOWN_MODE;
			}
			break;
		case DIVOOM_TOOL_COUNT_TIME_DOWN_START:
			{
                if(pdivoom_app_tool_handle->next_reach_time > time(0))
                {
				    run_info.next_reach_time = pdivoom_app_tool_handle->next_reach_time - time(0);
                }
                else
                {
                    run_info.next_reach_time = 0;
                }
				run_info.status = DIVOOM_DISP_COUNT_TIME_DOWN_RUN;
				//divoom_disp_ctrl_set_mode(DIVOOM_DISP_COUNT_TIME_DOWN_MODE, (uint8 *)&run_info);
				divoom_app_tool_open_countdown_ui(1);
				pdivoom_app_tool_handle->cur_mode_set = DIVOOM_DISP_COUNT_TIME_DOWN_MODE;
			}
			break;
		case DIVOOM_TOOL_SCORE_SET:
			{
				score_info.red_score = pdivoom_app_tool_handle->score_red_score;
				score_info.blue_score = pdivoom_app_tool_handle->score_blue_score;
				//divoom_disp_ctrl_set_mode(DIVOOM_DISP_SCORE_MODE, (uint8 *)&score_info);
				divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_SCORE_BOARD_MENU, NULL, 0, 0);
				pdivoom_app_tool_handle->cur_mode_set = DIVOOM_DISP_SCORE_MODE;
				//pdivoom_app_tool_handle->pre_mode_set = DIVOOM_DISP_SCORE_MODE;
				//pdivoom_app_tool_handle->last_is_noise = 0;
			}
			break;
		case DIVOOM_TOOL_WATCH_SET:
			{
				watch_info.ctrl_flag = pdivoom_app_tool_handle->ctrl_flag;
				//watch_info.start_time = pdivoom_app_tool_handle->start_time;
				//watch_info.pause_time = pdivoom_app_tool_handle->pause_time;
				watch_info.watch_disp_value = pdivoom_app_tool_handle->watch_disp_value;
				//divoom_disp_ctrl_set_mode(DIVOOM_DISP_WATCH_MODE, (uint8 *)&watch_info);
				divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_STOP_WATCH_MENU, NULL, 0, 0);
				pdivoom_app_tool_handle->cur_mode_set = DIVOOM_DISP_WATCH_MODE;
				//pdivoom_app_tool_handle->pre_mode_set = DIVOOM_DISP_WATCH_MODE;
				//pdivoom_app_tool_handle->last_is_noise = 0;
			}
			break;
		case DIVOOM_TOOL_NOISE_SET:
			{
				if(pdivoom_app_tool_handle->noise_flag == 1)
				{
					pdivoom_app_tool_handle->cur_mode_set = DIVOOM_DISP_NOISE_MODE;
					divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_NOISE_MEASURE_MENU, NULL, 0, 0);
					//divoom_disp_ctrl_set_mode(DIVOOM_DISP_NOISE_MODE, NULL);
				}
			}
			break;
	}
	return 1;
}






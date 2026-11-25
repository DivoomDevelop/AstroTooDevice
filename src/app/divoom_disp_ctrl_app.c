#include "divoom.h"


#define MAX_TASK	10
#define DIVOOM_READ_FILE_SIZE 2048
typedef int divoom_disp_ctrl_app_get_config_func(void *buf, int size);
typedef int divoom_disp_ctrl_app_set_config_func(void *buf, int size);

typedef struct _divoom_disp_ctrl_app_task_info
{
	uint8 status;
	uint8 task_id;
	uint16 call_interval;
	uint32 ticks;
	void *task_mux;
	DIVOOM_LIGHT_CALLBACK *pcallback;
}divoom_disp_ctrl_app_task_info, Pdivoom_disp_ctrl_app_task_info;

typedef struct _divoom_disp_ctrl_app_info
{
	//存放命令的queue
	void *cmd_queue;
    uint8 rgb_buf[SCREEN_BUF_SIZE];
	divoom_disp_ctrl_app_task_info task_info[MAX_TASK];	//自定义图片显示有可能叠加文字显示
}divoom_disp_ctrl_app_info, *Pdivoom_disp_ctrl_app_info;

typedef struct _divoom_disp_ctrl_app_get_config
{
	int disp_mode;
	divoom_disp_ctrl_app_get_config_func *pget_config_func;
}divoom_disp_ctrl_app_get_config, *Pdivoom_disp_ctrl_app_get_config;

typedef struct _divoom_disp_ctrl_app_set_config
{
	int disp_mode;
	divoom_disp_ctrl_app_set_config_func *pset_config_func;
}divoom_disp_ctrl_app_set_config, *Pdivoom_disp_ctrl_app_set_config;

typedef struct _divoom_disp_ctrl_app_queue_struct_
{
	uint8 cmd;
	uint8 *data;
	uint16 data_len;
}divoom_disp_ctrl_app_queue_struct, *Pdivoom_disp_ctrl_app_queue_struct;

void divoom_disp_ctrl_app_free_queue(Pdivoom_disp_ctrl_app_queue_struct struct_data);

static Pdivoom_disp_ctrl_app_info pdivoom_disp_ctrl_app_handle = NULL;

int divoom_disp_rand(int x);

const divoom_disp_ctrl_app_get_config pdivoom_disp_ctrl_app_get_config_table_func[] =
{
	{DIVOOM_DISP_INFO_ALL, divoom_disp_file_manage_get_all_config},
};

const divoom_disp_ctrl_app_set_config pdivoom_disp_ctrl_app_set_config_table_func[] =
{
	{DIVOOM_DISP_INFO_ALL, divoom_disp_file_manage_set_all_config},
};

//--------------------------------------------------------------------------------------
//微任务
void divoom_disp_ctrl_app_resume_task(uint8 task_id)	
{
	int i = 0;

	for (i = 0; i < MAX_TASK; i++)
	{
		if(pdivoom_disp_ctrl_app_handle->task_info[i].task_id == task_id)
		{
    		divoom_system_get_lock(pdivoom_disp_ctrl_app_handle->task_info[i].task_mux);
			pdivoom_disp_ctrl_app_handle->task_info[i].status = 1;			
			divoom_system_free_lock(pdivoom_disp_ctrl_app_handle->task_info[i].task_mux);
			break;
		}
	}
}

void divoom_disp_ctrl_app_update_task(uint8 task_id, uint32 time)
{	
	int i = 0;

	for (i = 0; i < MAX_TASK; i++)
	{
		if(pdivoom_disp_ctrl_app_handle->task_info[i].task_id == task_id)
		{
			pdivoom_disp_ctrl_app_handle->task_info[i].call_interval = 10 * time;		
			divoom_print("pdivoom_disp_ctrl_app_handle->task_info[%d].call_interval = %d", i, pdivoom_disp_ctrl_app_handle->task_info[i].call_interval);
			pdivoom_disp_ctrl_app_handle->task_info[i].ticks = divoom_system_get_tickcount() + divoom_system_set_interva_ms(pdivoom_disp_ctrl_app_handle->task_info[i].call_interval);
			break;
		}
	}
}

void divoom_disp_ctrl_app_pause_task(uint8 task_id)
{
	int i = 0;
	
	for (i = 0; i < MAX_TASK; i++)
	{
		if(pdivoom_disp_ctrl_app_handle->task_info[i].task_id == task_id)
		{
            divoom_system_get_lock(pdivoom_disp_ctrl_app_handle->task_info[i].task_mux);
			pdivoom_disp_ctrl_app_handle->task_info[i].status = 0;
			divoom_system_free_lock(pdivoom_disp_ctrl_app_handle->task_info[i].task_mux);	
			break;
		}
	}
}


uint8 divoom_disp_ctrl_app_create_task(DIVOOM_LIGHT_CALLBACK *pcallback, uint32 time)
{
	uint8 task_id = 0xff;
	int i = 0;
	if(pdivoom_disp_ctrl_app_handle == NULL)
	{
		return task_id;
	}
	for (i = 0; i < MAX_TASK; i++)
	{
		if (pdivoom_disp_ctrl_app_handle->task_info[i].pcallback == NULL)
		{
            divoom_system_get_lock(pdivoom_disp_ctrl_app_handle->task_info[i].task_mux);
			pdivoom_disp_ctrl_app_handle->task_info[i].task_id = i+1;
			task_id = pdivoom_disp_ctrl_app_handle->task_info[i].task_id;
			pdivoom_disp_ctrl_app_handle->task_info[i].call_interval = 10 * time;
			pdivoom_disp_ctrl_app_handle->task_info[i].ticks = divoom_system_get_tickcount() + divoom_system_set_interva_ms(pdivoom_disp_ctrl_app_handle->task_info[i].call_interval);
			pdivoom_disp_ctrl_app_handle->task_info[i].pcallback = pcallback;
			pdivoom_disp_ctrl_app_handle->task_info[i].status = 0;				
			divoom_system_free_lock(pdivoom_disp_ctrl_app_handle->task_info[i].task_mux);
			break;
		}
	}	
	return task_id;
}

void divoom_disp_ctrl_app_destroy_task(uint8 task_id)
{	
	int i = 0;

	for (i = 0; i < MAX_TASK; i++)
	{	
		if(pdivoom_disp_ctrl_app_handle->task_info[i].task_id == task_id)
		{
		    divoom_system_get_lock(pdivoom_disp_ctrl_app_handle->task_info[i].task_mux);
			pdivoom_disp_ctrl_app_handle->task_info[i].status = 0;
			pdivoom_disp_ctrl_app_handle->task_info[i].task_id = 0;			
			pdivoom_disp_ctrl_app_handle->task_info[i].pcallback = NULL;
			divoom_system_free_lock(pdivoom_disp_ctrl_app_handle->task_info[i].task_mux);
			break;
		}	
	}
}

void divoom_disp_ctrl_app_task_main(void *arg)
{	
	//portTickType last_wake_time;
	//last_wake_time = divoom_system_get_tickcount();
	int i = 0;
//	Pdivoom_disp_ctrl_app_queue_struct struct_data = NULL;

	while (1)
	{
		//检查是否有命令需要发送,有的话就先延时50ms,发送命令,再延时50ms
		#if 0
		if(divoom_queue_not_empty(pdivoom_disp_ctrl_app_handle->cmd_queue))
		{
			struct_data = divoom_queue_pop(pdivoom_disp_ctrl_app_handle->cmd_queue);
			if(struct_data != NULL && struct_data->data != NULL)
			{
				vTaskDelay(50 / portTICK_PERIOD_MS);
				divoom_screen_handle_send(struct_data->cmd, struct_data->data, struct_data->data_len);
				vTaskDelay(50 / portTICK_PERIOD_MS);
				divoom_disp_ctrl_app_free_queue(struct_data);
			}
		}
		else
        #endif
		{
			for (i = 0; i < MAX_TASK; i++)
			{
				if (pdivoom_disp_ctrl_app_handle->task_info[i].status && (divoom_system_get_tickcount() >= pdivoom_disp_ctrl_app_handle->task_info[i].ticks))
				{					
					divoom_system_get_lock(pdivoom_disp_ctrl_app_handle->task_info[i].task_mux);
					if(pdivoom_disp_ctrl_app_handle->task_info[i].status && pdivoom_disp_ctrl_app_handle->task_info[i].pcallback != NULL)
					{
						pdivoom_disp_ctrl_app_handle->task_info[i].pcallback(NULL);
					}
					//放在这里是防止在.pcallback有延迟,这个时间就会有问题
					pdivoom_disp_ctrl_app_handle->task_info[i].ticks = divoom_system_get_tickcount() + divoom_system_set_interva_ms(pdivoom_disp_ctrl_app_handle->task_info[i].call_interval);
					divoom_system_free_lock(pdivoom_disp_ctrl_app_handle->task_info[i].task_mux);
				}		
			}
			//vTaskDelayUntil( &last_wake_time, divoom_system_set_interva_ms(10));
			//不再使用上面的函数,可能会造成问题,这里直接延时不精准的话其实问题不大
			divoom_system_delay_ms(10);
		}

	}
}
//--------------------------------------------------------------------------------------


void *divoom_disp_ctrl_app_malloc(uint32 size, uint32 model_line, const char *file_name)
{
	return divoom_malloc(size, DIVOOM_MODEL_LIGHT_APP, model_line, file_name);
}

void divoom_disp_ctrl_app_free(void *ptr, const char *file_name, int line)
{
	return divoom_free_line(ptr,file_name,line);
}

int divoom_disp_rand(int x) 
{
	//改为硬件随机数
	if (x > 0)
		return (rand()%x);
	else
		return 0;
}



int divoom_disp_ctrl_app_get_vol(void)
{
	return 5;
}
void divoom_disp_ctrl_app_play_voice(T_U16 voice_tip, T_U16 time, T_U8 vol, T_Voice_PalyEndCallBack callback)
{
    Pdivoom_sys_play_voice_info voice_info;
    divoom_sys_msg_info model_msg = {0};  

    model_msg.msg_type = 1;
    model_msg.src_id = DIVOOM_MODEL_LIGHT_APP;
    model_msg.dest_id = DIVOOM_MODEL_MAIN;          
    model_msg.msg_id = DIVOOM_MAIN_EVENT_PLAY_VOICE; 
    
    if(voice_tip == T_U16_MAX)
    {
        //divoom_voice_info_stop_all();
    }
    else
    {
        voice_info = divoom_system_malloc(sizeof(divoom_sys_play_voice_info));
        voice_info->cyc = time;
        voice_info->id = voice_tip;
        voice_info->vol = vol;
        model_msg.data = (T_U8 *)voice_info;
        divoom_sys_model_crl_msg_push(&model_msg);
    }
}

void divoom_disp_ctrl_app_free_queue(Pdivoom_disp_ctrl_app_queue_struct struct_data)
{
	if(struct_data != NULL)
	{
		if(struct_data->data != NULL)
		{
			divoom_free(struct_data->data);
		}
		divoom_free(struct_data);
	}
}

//给的数据可以是栈空间的,这个函数内部自动实现申请数据
//> 0 success, other fail
//这样接收数据的话,每次发送数据都会前后间隔50ms,确保cmd消息gd能正常处理
int divoom_disp_ctrl_app_send_cmd(uint8 cmd, uint8 *data, uint16 data_len)
{
    return 0;
	if(data_len > 0 && data != NULL)
	{
		Pdivoom_disp_ctrl_app_queue_struct queue_data = divoom_malloc(sizeof(divoom_disp_ctrl_app_queue_struct), DIVOOM_MODEL_LIGHT_APP, __LINE__, __FILE__);
		queue_data->data = divoom_malloc(data_len, DIVOOM_MODEL_LIGHT_APP, __LINE__, __FILE__);
		memcpy(queue_data->data, data, data_len);
		queue_data->cmd = cmd;
		queue_data->data_len = data_len;
		if(pdivoom_disp_ctrl_app_handle != NULL && pdivoom_disp_ctrl_app_handle->cmd_queue != NULL)
		{
			divoom_queue_push(pdivoom_disp_ctrl_app_handle->cmd_queue, queue_data);
		}
		return 1;
	}
	/*else
	{
		if(queue_data->data)
		{
			divoom_free(queue_data->data);
		}
		if(queue_data)
		{
			divoom_free(queue_data);
		}
		return 0;
	}*/
	return 0;
}




uint32 divoom_disp_ctrl_app_get_tool_info(uint8 mode, void *tool_data)
{
	switch(mode)
	{
		case DIVOOM_DISP_WATCH_MODE:
			{
				//*(uint32 *)tool_data = divoom_app_tool_get_watch_cnt();
				divoom_app_tool_get_watch_cnt((Pdivoom_disp_ctrl_watch_ctrl)tool_data);
			}
			return 1;
			break;
		case DIVOOM_DISP_COUNT_TIME_DOWN_MODE:
			{
				*(uint32 *)tool_data = divoom_app_tool_get_count_down_cnt();
			}
			return 1;
			break;
		default:
			return 0;
			break;
	}
	return 0;
}

#if defined SHACO_TEST
extern int divoom_weather_get_ceil_value_test(void);
#endif
int divoom_disp_ctrl_app_get_env_info(uint8 mode, void *env_data)
{
	//后期添加
	switch(mode)
	{
        case DIVOOM_DISP_ENV_TIME:
        {
			struct tm cur_tm;
			time_t cur_time;
			Pdivoom_light_env_time_info pinfo = (Pdivoom_light_env_time_info)env_data;
			if(pinfo)
			{
				cur_time = time(0);
				localtime_r((time_t *)&cur_time, &cur_tm);
				if(cur_tm.tm_hour != pinfo->hour || cur_tm.tm_min != pinfo->min)
				{
					pinfo->hour = cur_tm.tm_hour;
					pinfo->min = cur_tm.tm_min;
				}
			}
        }
            break;
            
        case DIVOOM_DISP_ENV_CELS:
#if defined SHACO_TEST
//            *(int *)env_data = divoom_weather_get_ceil_value_test();
#else
//            *(int *)env_data = divoom_weather_get_ceil_value();
#endif
            break;
            
        case DIVOOM_DISP_ENV_DATE:
			{
/*
				T_DIVOOM_TIME sys_time;
				Pdivoom_disp_env_date_info pdate_info = (Pdivoom_disp_env_date_info)env_data;

				anyka_to_divoom_get_system_time(&sys_time);
				pdate_info->month = sys_time.month;
				pdate_info->week = sys_time.week;
				if(pdate_info->week)
				{
					pdate_info->week= pdate_info->week-1;      //星期一从0算起
				}
				else
				{
					pdate_info->week=6;
				}

				pdate_info->day = sys_time.day;
*/
			}
            break;
            
        case DIVOOM_DISP_ENV_WEATHER:
//            *(T_U8 *)env_data = divoom_weather_get_weather_type();
            break;

        case DIVOOM_DISP_ENV_BAT:
        {
//            divoom_main_get_bat_info(env_data);
        }
	}
	return 0;
}


#if 0
extern uint8 divoom_main_get_charge_status(void);
int divoom_disp_ctrl_app_get_env_info(uint8 mode, void *env_data)
{
    int ret = 1;
	//更新时间，温度，湿度

    if(env_data == NULL)
    {
        return ret;
    }
	switch(mode)
	{
		case DIVOOM_DISP_ENV_TIME:
		{
			Pdivoom_disp_env_time_info ptime_info = (Pdivoom_disp_env_time_info)env_data;
			time_t cur_time;
            struct tm local_info;

            time(&cur_time);
            localtime_r(&cur_time, &local_info);
			ptime_info->hour = local_info.tm_hour; 							 
			ptime_info->min = local_info.tm_min;
		}
		break;
		
		case DIVOOM_DISP_ENV_DATE:
		{
			#if 1
			time_t cur_time;
            struct tm local_info;
            Pdivoom_disp_env_date_info pdate_info = (Pdivoom_disp_env_date_info)env_data;

            time(&cur_time);
            localtime_r(&cur_time, &local_info);
            
			pdate_info->day = local_info.tm_mday;
            if(local_info.tm_wday)
            {
                pdate_info->week --;
            }
            else
            {
                pdate_info->week = 6;
            }
			pdate_info->month = local_info.tm_mon + 1;
			#endif
			//ak_debug("date:%d,week:%d\n",date->day,date->week);
		}
		break;

		/*case DIVOOM_DISP_ENV_CELS:
			//没有温度了
			break;
			
		case DIVOOM_DISP_ENV_HUMI:
			//湿度也没有了
			break;
            
        case DIVOOM_DISP_ENV_WEATHER:
			//天气同样没有了
			break;*/
        
        case DIVOOM_DISP_ENV_BAT:
        {
            Pdivoom_disp_ctrl_bat_ctrl bat_info = (Pdivoom_disp_ctrl_bat_ctrl)env_data;

            bat_info->bat_charge = divoom_main_get_charge_status();
			bat_info->bat_level = divoom_battery_get_bat_level();
//            bat_info->bat_level = 9; 
        }
            break;
	}

    return ret;
}

void* divoom_disp_ctrl_app_get_net_weather_info(void *data)
{            
    //divoom_disp_ctrl_app_get_evn_info();
    return NULL;
}

#endif



//创建一个接收命令消息的queue,长度为10(足够了),显示task里面判断是否有cmd消息,有消息,就延时50ms发送消息,发送完消息,再延时50ms结束当前消息命令
void divoom_disp_ctrl_app_init(Pdivoom_sys_setting_info psys_setting)
{
	divoom_disp_ctrl_callback pcallback = {0};
	
	pcallback.print = NULL;
	pcallback.malloc = divoom_disp_ctrl_app_malloc;
	pcallback.free = divoom_disp_ctrl_app_free;
	pcallback.create = divoom_microtask_create;
	pcallback.destroy = divoom_microtask_destroy;
	pcallback.resume = divoom_microtask_resume;
	pcallback.pause = divoom_microtask_pause;
	pcallback.update = divoom_microtask_update;
	/*pcallback.write_light_muti_slave_data = divoom_disp_ctrl_app_write_multi_slave_light_data;
	pcallback.write_light_muti_gif_data = divoom_disp_ctrl_app_write_multi_gif_light_data;
	pcallback.send_light_muti_gif_data = divoom_disp_ctrl_app_send_multi_gif_light_data;*/
	pcallback.get_rand = divoom_disp_rand;
	pcallback.get_env_info = divoom_disp_ctrl_app_get_env_info;		
	pcallback.get_tool_iofo = divoom_disp_ctrl_app_get_tool_info;
	//pcallback.play_voice = divoom_disp_ctrl_app_play_voice;
	pcallback.get_vol = divoom_disp_ctrl_app_get_vol;
	
	pdivoom_disp_ctrl_app_handle = (Pdivoom_disp_ctrl_app_info)divoom_malloc(sizeof(divoom_disp_ctrl_app_info), DIVOOM_MODEL_LIGHT_APP, __LINE__, __FILE__);

	if(pdivoom_disp_ctrl_app_handle == NULL)
	{
		divoom_print("[%s,%d]it fails to malloc\n", __func__, __LINE__);
		return ;
	}
	memset(pdivoom_disp_ctrl_app_handle, 0, sizeof(divoom_disp_ctrl_app_info));
	pdivoom_disp_ctrl_app_handle->cmd_queue = divoom_queue_init(50, __func__, __LINE__);
	divoom_disp_ctrl_init(&pcallback, DIVOOM_SYSTEM_RTOS, DIVOOM_DISP_NO_DISP_MODE, psys_setting);	
    divoom_screen_set_direction(psys_setting->screen_dir);
}



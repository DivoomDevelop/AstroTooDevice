#include "divoom.h"

#define DIVOOM_MICROTASK_MAX_TASK	10
#define TIME_UNIT	1

typedef struct __divoom_microtask_info
{
	uint8 status;
	uint8 task_id;
	uint16 call_interval;
	uint32 ticks;
	void  *task_mux;
    void *para;
	DIVOOM_MICROTASK_CALLBACK *pcallback;
}divoom_microtask_info, Pdivoom_microtask_info;


typedef struct __divoom_microtask_ctrl
{
    uint8 actice_num;
    void  *wait_sem;
	divoom_microtask_info task_info[DIVOOM_MICROTASK_MAX_TASK];

}divoom_microtask_ctrl,*Pdivoom_microtask_ctrl;


Pdivoom_microtask_ctrl pdivoom_microtask_ctrl_handle = NULL;




void divoom_microtask_resume(uint8 task_id)	
{
	int i = 0;
	if(NULL == pdivoom_microtask_ctrl_handle)
		return;
	for (i = 0; i < DIVOOM_MICROTASK_MAX_TASK; i++)
	{
		if(pdivoom_microtask_ctrl_handle->task_info[i].task_id == task_id)
		{
    		divoom_system_get_lock(pdivoom_microtask_ctrl_handle->task_info[i].task_mux);
			pdivoom_microtask_ctrl_handle->task_info[i].status = 1;			
			divoom_system_free_lock(pdivoom_microtask_ctrl_handle->task_info[i].task_mux);
			break;
		}
	}
}

void divoom_microtask_update(uint8 task_id, uint32 time)
{	
	int i = 0;
	if(NULL == pdivoom_microtask_ctrl_handle)
		return;

	for (i = 0; i < DIVOOM_MICROTASK_MAX_TASK; i++)
	{
		if(pdivoom_microtask_ctrl_handle->task_info[i].task_id == task_id)
		{
			pdivoom_microtask_ctrl_handle->task_info[i].call_interval = TIME_UNIT * time;		
//			divoom_print("pdivoom_microtask_ctrl_handle->task_info[%d].call_interval = %d", i, pdivoom_microtask_ctrl_handle->task_info[i].call_interval);
			pdivoom_microtask_ctrl_handle->task_info[i].ticks = divoom_system_get_tickcount() + divoom_system_set_interva_ms(pdivoom_microtask_ctrl_handle->task_info[i].call_interval);
			break;
		}
	}
}

void divoom_microtask_pause(uint8 task_id)
{
	int i = 0;
	if(NULL == pdivoom_microtask_ctrl_handle)
		return;	
	for (i = 0; i < DIVOOM_MICROTASK_MAX_TASK; i++)
	{
		if(pdivoom_microtask_ctrl_handle->task_info[i].task_id == task_id)
		{
            divoom_system_get_lock(pdivoom_microtask_ctrl_handle->task_info[i].task_mux);
			pdivoom_microtask_ctrl_handle->task_info[i].status = 0;
			divoom_system_free_lock(pdivoom_microtask_ctrl_handle->task_info[i].task_mux);	
			break;
		}
	}
}


uint8 divoom_microtask_create(DIVOOM_MICROTASK_CALLBACK *pcallback, uint32 time)
{
	uint8 task_id = 0xff;
	int i = 0;
	if(pdivoom_microtask_ctrl_handle == NULL)
	{
		return task_id;
	}
	for (i = 0; i < DIVOOM_MICROTASK_MAX_TASK; i++)
	{
		if (pdivoom_microtask_ctrl_handle->task_info[i].pcallback == NULL)
		{
            divoom_system_get_lock(pdivoom_microtask_ctrl_handle->task_info[i].task_mux);
			pdivoom_microtask_ctrl_handle->task_info[i].task_id = i+1;
			task_id = pdivoom_microtask_ctrl_handle->task_info[i].task_id;
			pdivoom_microtask_ctrl_handle->task_info[i].call_interval = TIME_UNIT * time;
			pdivoom_microtask_ctrl_handle->task_info[i].ticks = divoom_system_get_tickcount() + divoom_system_set_interva_ms(pdivoom_microtask_ctrl_handle->task_info[i].call_interval);
			pdivoom_microtask_ctrl_handle->task_info[i].pcallback = pcallback;
			pdivoom_microtask_ctrl_handle->task_info[i].para = NULL;
			pdivoom_microtask_ctrl_handle->task_info[i].status = 0;		
            pdivoom_microtask_ctrl_handle->actice_num ++;		
			divoom_system_free_lock(pdivoom_microtask_ctrl_handle->task_info[i].task_mux);
            divoom_system_put_sem(pdivoom_microtask_ctrl_handle->wait_sem);
			break;
		}
	}	
	return task_id;
}

uint8 divoom_microtask_create_para(DIVOOM_MICROTASK_CALLBACK *pcallback, uint32 time, void *para)
{
	uint8 task_id = 0xff;
	int i = 0;
	if(pdivoom_microtask_ctrl_handle == NULL)
	{
		return task_id;
	}
	for (i = 0; i < DIVOOM_MICROTASK_MAX_TASK; i++)
	{
		if (pdivoom_microtask_ctrl_handle->task_info[i].pcallback == NULL)
		{
            divoom_system_get_lock(pdivoom_microtask_ctrl_handle->task_info[i].task_mux);
			pdivoom_microtask_ctrl_handle->task_info[i].task_id = i+1;
			task_id = pdivoom_microtask_ctrl_handle->task_info[i].task_id;
			pdivoom_microtask_ctrl_handle->task_info[i].call_interval = 10 * time;
			pdivoom_microtask_ctrl_handle->task_info[i].ticks = divoom_system_get_tickcount() + divoom_system_set_interva_ms(pdivoom_microtask_ctrl_handle->task_info[i].call_interval);
			pdivoom_microtask_ctrl_handle->task_info[i].pcallback = pcallback;
			pdivoom_microtask_ctrl_handle->task_info[i].para = para;
			pdivoom_microtask_ctrl_handle->task_info[i].status = 0;		
            pdivoom_microtask_ctrl_handle->actice_num ++;
			divoom_system_free_lock(pdivoom_microtask_ctrl_handle->task_info[i].task_mux);
            divoom_system_put_sem(pdivoom_microtask_ctrl_handle->wait_sem);
			break;
		}
	}	
	return task_id;
}

void divoom_microtask_destroy(uint8 task_id)
{	
	int i = 0;
	if(NULL == pdivoom_microtask_ctrl_handle)
		return;
	for (i = 0; i < DIVOOM_MICROTASK_MAX_TASK; i++)
	{	
		if(pdivoom_microtask_ctrl_handle->task_info[i].task_id == task_id)
		{
		    divoom_system_get_lock(pdivoom_microtask_ctrl_handle->task_info[i].task_mux);
			pdivoom_microtask_ctrl_handle->task_info[i].status = 0;
			pdivoom_microtask_ctrl_handle->task_info[i].task_id = 0;			
			pdivoom_microtask_ctrl_handle->task_info[i].pcallback = NULL;
            pdivoom_microtask_ctrl_handle->actice_num --;
			divoom_system_free_lock(pdivoom_microtask_ctrl_handle->task_info[i].task_mux);
			break;
		}	
	}
}

void *divoom_microtask_task_main(void *arg)
{	
	//portTickType last_wake_time;
	//last_wake_time = divoom_system_get_tickcount();
	int i = 0;
//	Pdivoom_microtask_queue_struct struct_data = NULL;

	while (1)
	{
		//检查是否有命令需要发送,有的话就先延时50ms,发送命令,再延时50ms
		#if 0
		if(divoom_queue_not_empty(pdivoom_microtask_ctrl_handle->cmd_queue))
		{
			struct_data = divoom_queue_pop(pdivoom_microtask_ctrl_handle->cmd_queue);
			if(struct_data != NULL && struct_data->data != NULL)
			{
				vTaskDelay(50 / portTICK_PERIOD_MS);
				divoom_screen_handle_send(struct_data->cmd, struct_data->data, struct_data->data_len);
				vTaskDelay(50 / portTICK_PERIOD_MS);
				divoom_microtask_free_queue(struct_data);
			}
		}
		else
        #endif
        if(pdivoom_microtask_ctrl_handle->actice_num == 0)
        {
            divoom_system_get_sem(pdivoom_microtask_ctrl_handle->wait_sem);
        }
		{
			for (i = 0; i < DIVOOM_MICROTASK_MAX_TASK; i++)
			{
				if (pdivoom_microtask_ctrl_handle->task_info[i].status && (divoom_system_get_tickcount() >= pdivoom_microtask_ctrl_handle->task_info[i].ticks))
				{					
					divoom_system_get_lock(pdivoom_microtask_ctrl_handle->task_info[i].task_mux);
					if(pdivoom_microtask_ctrl_handle->task_info[i].status && pdivoom_microtask_ctrl_handle->task_info[i].pcallback != NULL)
					{
						pdivoom_microtask_ctrl_handle->task_info[i].pcallback(pdivoom_microtask_ctrl_handle->task_info[i].para);
					}
					//放在这里是防止在.pcallback有延迟,这个时间就会有问题
					pdivoom_microtask_ctrl_handle->task_info[i].ticks = divoom_system_get_tickcount() + divoom_system_set_interva_ms(pdivoom_microtask_ctrl_handle->task_info[i].call_interval);
					divoom_system_free_lock(pdivoom_microtask_ctrl_handle->task_info[i].task_mux);
				}		
			}
			//vTaskDelayUntil( &last_wake_time, divoom_system_set_interva_ms(10));
			//不再使用上面的函数,可能会造成问题,这里直接延时不精准的话其实问题不大
			divoom_system_delay_ms(1);
		}

	}
    return NULL;
}

void divoom_microtask_init(void)
{
	int i = 0;
	if(pdivoom_microtask_ctrl_handle)
	{
		divoom_print("pdivoom_microtask_ctrl_handle exist\n");
	}
	pdivoom_microtask_ctrl_handle = divoom_system_malloc(sizeof(divoom_microtask_ctrl));
	if(NULL == pdivoom_microtask_ctrl_handle)
	{
		divoom_print("divoom microtask malloc fail\n");
		return;
	}
	memset(pdivoom_microtask_ctrl_handle,0,sizeof(divoom_microtask_ctrl));
    pdivoom_microtask_ctrl_handle->wait_sem = divoom_system_create_sem(1, 0);
	for (i = 0; i < DIVOOM_MICROTASK_MAX_TASK; i++)
		pdivoom_microtask_ctrl_handle->task_info[i].task_mux = divoom_system_create_lock();
	divoom_system_create_task(divoom_microtask_task_main, "microtask", NULL, DIVOOM_TASK_STACK_MICROTASK);
}



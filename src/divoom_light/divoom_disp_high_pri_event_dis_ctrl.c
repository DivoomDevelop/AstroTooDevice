#include "divoom_disp_com.h"

//完成图片定时显示，完成退出时的显示模式切换
//简化版的divoom_disp_event.c,只需要显示图片和在时间到了之后推出的时候，要不要divoom_disp_ctrl_clear_pre_mode

//这个作为全屏显示的提醒,在结束之后自动发命令退出当前schedule

#define DIVOOM_DISP_HIGH_PRI_EVENT_TICK 10

	//divoom_disp_high_pri_back_event event_info;
typedef struct{
	divoom_disp_high_pri_event_struct pic_info;

	uint8 pic_index;
    uint16 tick;
    uint16 tick_in_period;
	uint32 task_id;

	uint8  exit_flag;
	divoom_disp_event_extend_info extend_info;
}divoom_disp_ctrl_high_pri_struct, *Pdivoom_disp_ctrl_high_pri_struct;

Pdivoom_disp_ctrl_high_pri_struct pdivoom_disp_ctrl_high_pri_handle = NULL;

void divoom_disp_high_pri_event_dis_ctrl_main(void *data)
{
	uint16 pic_id;
	if(pdivoom_disp_ctrl_high_pri_handle->exit_flag == 1)
	{
		return;
	}
	pic_id = pdivoom_disp_ctrl_high_pri_handle->pic_info.disp_id[pdivoom_disp_ctrl_high_pri_handle->pic_index];
	pdivoom_disp_ctrl_high_pri_handle->tick++;
	if(pdivoom_disp_ctrl_high_pri_handle->tick_in_period == 0)
	{
		divoom_disp_picture_display(pic_id);
	}
	else if(pdivoom_disp_ctrl_high_pri_handle->tick_in_period == pdivoom_disp_ctrl_high_pri_handle->pic_info.on_time)
	{
		divoom_disp_ctrl_clr_screen();
	}
	pdivoom_disp_ctrl_high_pri_handle->tick_in_period++;
	if(pdivoom_disp_ctrl_high_pri_handle->tick_in_period == pdivoom_disp_ctrl_high_pri_handle->pic_info.period_time)
	{
		pdivoom_disp_ctrl_high_pri_handle->tick_in_period = 0;
		pdivoom_disp_ctrl_high_pri_handle->pic_index++;
		if(pdivoom_disp_ctrl_high_pri_handle->pic_index == pdivoom_disp_ctrl_high_pri_handle->pic_info.tol_pic)
		{
			pdivoom_disp_ctrl_high_pri_handle->pic_index = 0;
		}
	}
	//send exit
	if(pdivoom_disp_ctrl_high_pri_handle->tick >= pdivoom_disp_ctrl_high_pri_handle->pic_info.tol_time)
	{
		//divoom_disp_ctrl_clear_pre_mode();
		//divoom_disp_ctrl_app_exit_self_mode();
		divoom_disp_ctrl_app_exit_mode(DIVOOM_DISP_HIGH_PRI_EVENT_DIS_CTRL, 
				pdivoom_disp_ctrl_high_pri_handle->extend_info.model_id, pdivoom_disp_ctrl_high_pri_handle->extend_info.event_id);
		pdivoom_disp_ctrl_high_pri_handle->exit_flag = 1;
	}
}

//void divoom_disp_high_pri_event_dis_ctrl_init(Pdivoom_disp_high_pri_back_event event_info, Pdivoom_disp_high_pri_event_struct pic_info)
void divoom_disp_high_pri_event_dis_ctrl_init(Pdivoom_disp_high_pri_event_struct pic_info)
{
	divoom_malloc_check_all_ram_in();
	if(pdivoom_disp_ctrl_high_pri_handle)
		return;
	pdivoom_disp_ctrl_high_pri_handle = (Pdivoom_disp_ctrl_high_pri_struct)divoom_com_malloc(sizeof(divoom_disp_ctrl_high_pri_struct), __func__, __LINE__);
	if(pdivoom_disp_ctrl_high_pri_handle == NULL)
	{
		divoom_print("No mem to malloc!\n");
		return;
	}

	memset(pdivoom_disp_ctrl_high_pri_handle, 0, sizeof(divoom_disp_ctrl_high_pri_struct));

	//if(event_info != NULL && pic_info != NULL)
	if(pic_info != NULL)
	{
		//memcpy(&pdivoom_disp_ctrl_high_pri_handle->event_info, event_info, sizeof(divoom_disp_high_pri_back_event));
		memcpy(&pdivoom_disp_ctrl_high_pri_handle->pic_info, pic_info, sizeof(divoom_disp_high_pri_event_struct));
	}

	pdivoom_disp_ctrl_high_pri_handle->exit_flag = 0;
	pdivoom_disp_ctrl_high_pri_handle->pic_index = 0;
	pdivoom_disp_ctrl_high_pri_handle->tick = 0;
	pdivoom_disp_ctrl_high_pri_handle->tick_in_period = 0;
	pdivoom_disp_ctrl_high_pri_handle->extend_info.model_id = pic_info->extend_info.model_id;
	pdivoom_disp_ctrl_high_pri_handle->extend_info.event_id = pic_info->extend_info.event_id;
	//divoom_print("pic_info->extend_info.model_id = %d--%d\n", pic_info->extend_info.model_id, pic_info->extend_info.event_id);
	//divoom_print("pdivoom_disp_ctrl_high_pri_handle->extend_info.model_id = %d--%d\n", 
	//		pdivoom_disp_ctrl_high_pri_handle->extend_info.model_id, pdivoom_disp_ctrl_high_pri_handle->extend_info.event_id);

	pdivoom_disp_ctrl_high_pri_handle->task_id = divoom_com_create_task(divoom_disp_high_pri_event_dis_ctrl_main, DIVOOM_DISP_HIGH_PRI_EVENT_TICK);
	divoom_com_resume_task(pdivoom_disp_ctrl_high_pri_handle->task_id);
}


void divoom_disp_high_pri_event_dis_ctrl_close(void)
{
	if(pdivoom_disp_ctrl_high_pri_handle != NULL)
	{
		divoom_com_pause_task(pdivoom_disp_ctrl_high_pri_handle->task_id);
		divoom_com_destroy_task(pdivoom_disp_ctrl_high_pri_handle->task_id);
		divoom_com_free(pdivoom_disp_ctrl_high_pri_handle,__func__,__LINE__);
		pdivoom_disp_ctrl_high_pri_handle = NULL;
	}
	divoom_print("%s--%d\n", __func__, __LINE__);
	divoom_malloc_check_all_ram_out(__func__);
}




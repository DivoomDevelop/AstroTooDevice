#include "divoom_disp_com.h"

Pdivoom_disp_ctrl_callback pdivoom_disp_ctrl_callback = NULL;


/*********************************************************************
  Function:        divoom_com_init
  Description:    初始化所有的接口函数
  Input:            pcallback        接口回调
  Return:           NONE
  Author:           albert
**********************************************************************/

void divoom_com_init(Pdivoom_disp_ctrl_callback pcallback)
{
    if(pdivoom_disp_ctrl_callback == NULL)
    {
        pdivoom_disp_ctrl_callback = divoom_system_malloc(sizeof(divoom_disp_ctrl_callback));
    }
    memcpy(pdivoom_disp_ctrl_callback, pcallback, sizeof(divoom_disp_ctrl_callback));
}

/*********************************************************************
  Function:        divoom_com_malloc
  Description:    分配内存
  Input:            size
                      file_name     分配字串标识，方便调试
                      model_line    分配值标识
  Return:          返回分配的内存指针
  Author:           albert
**********************************************************************/

void * divoom_com_malloc(uint32 size, const char *file_name, uint32 model_line)
{
    return pdivoom_disp_ctrl_callback->malloc(size, model_line, file_name);
}

/*********************************************************************
  Function:        divoom_com_free
  Description:    释放内存资源
  Input:            ptr
  Return:          NONE
  Author:           albert
**********************************************************************/


void divoom_com_free(void *ptr,const char *file_name, uint32 model_line)
{
    if(ptr)
    {
        pdivoom_disp_ctrl_callback->free(ptr,file_name, model_line);
    }
}

uint8 divoom_com_create_task(DIVOOM_DISP_CTRL_CALLBACK cb, uint32 time)
{
	if(pdivoom_disp_ctrl_callback->create)
	{
		return pdivoom_disp_ctrl_callback->create(cb, time);
	}
	return 0xFF;
}

void divoom_com_destroy_task(uint8 task_id)
{
	if(pdivoom_disp_ctrl_callback->destroy)
	{
		pdivoom_disp_ctrl_callback->destroy(task_id);
	}
}

void divoom_com_pause_task(uint8 task_id)
{
	if(pdivoom_disp_ctrl_callback->pause)
	{
		pdivoom_disp_ctrl_callback->pause(task_id);
	}
}

void divoom_com_resume_task(uint8 task_id)
{
	if(pdivoom_disp_ctrl_callback->resume)
	{
		pdivoom_disp_ctrl_callback->resume(task_id);
	}
}

void divoom_com_update_task(uint8 task_id, uint32 time)
{
	if(pdivoom_disp_ctrl_callback->update)
	{
		pdivoom_disp_ctrl_callback->update(task_id, time);
	}
}

//获取工具的信息,比如秒表的显示时间
uint32 divoom_com_get_tool_info(uint8 mode, void *tool_data)
{
	if(pdivoom_disp_ctrl_callback->get_tool_iofo)
		return pdivoom_disp_ctrl_callback->get_tool_iofo(mode, tool_data);
	return 0xff;
}

/*********************************************************************
  Function:        divoom_com_get_env_info
  Description:    得到系统环境相关信息
  Input:            mode        类型
                       env_data    相关数据
  Return:           NONE
  Author:           albert
  Data:            2016-10-31
**********************************************************************/

int divoom_com_get_env_info(uint8 mode, void *env_data)
{
	if(pdivoom_disp_ctrl_callback->get_env_info)
	    return pdivoom_disp_ctrl_callback->get_env_info(mode, env_data);
	return 0xff;
}

/*********************************************************************
  Function:        divoom_com_get_system_time
  Description:    得到系统时间，单位为Ｓ
  Input:            NONE
  Return:           返回时间信息
  Author:           albert
**********************************************************************/

uint32 divoom_com_get_system_time(void)
{
    /*time_t cur_time;

    time(&cur_time);*/

//    return anyka_to_divoom_get_system_utc_time();

	return 1;
    
}

/*********************************************************************
  Function:        divoom_com_get_random
  Description:    得到系统随机数
  Input:            max        最大值
  Return:           返回随机数
  Author:           albert
  Data:            2016-10-31
**********************************************************************/

int divoom_com_get_random(int max)
{
	if(pdivoom_disp_ctrl_callback->get_rand)
	    return pdivoom_disp_ctrl_callback->get_rand(max);
	return 0xff;
}

/*********************************************************************
  Function:        divoom_com_get_vol
  Description:    得到系统音量
  Input:           
  Return:          系统音量
  Author:           albert
  Data:            2016-10-31
**********************************************************************/
int divoom_com_get_vol(void)
{
	if(pdivoom_disp_ctrl_callback->get_vol)
    	return pdivoom_disp_ctrl_callback->get_vol();
	return 0xff;
}

//从APP层获取数据,需要的话自己添加
uint8 divoom_com_get_data_from_app(uint8 event, uint8 *data)
{
	switch(event)
	{
		case DIVOOM_DISP_COM_GET_TIME_COLOUR:
			{
//				divoom_main_get_time_colour((Pdivoom_screen_color_info)data);
			}
			break;
		case DIVOOM_DISP_COM_GET_TIME_FLAG:
			{
//				return divoom_main_get_time_flag();
			}
			break;
	}
	return 1;
}



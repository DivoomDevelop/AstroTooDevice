#include "divoom_disp_com.h"

#if 0
#define DIVOOM_POWERON_SNOW
#define DIVOOM_DISP_POWER_ON_DISP_MIN_TIME 10000
#define DIVOOM_DISP_POWER_ON_DISP_DEFAULT_MIN_TIME 5000
#define DIVOOM_DISP_POWER_ON_REFRESH_TIME 10

#define DIVOOM_DISP_POWER_ON_SNOW_REFRESH_TIME  5
#define DIVOOM_DISP_POWER_ON_SNOW_REFRESH_END   (DIVOOM_DISP_POWER_ON_DISP_DEFAULT_MIN_TIME/(DIVOOM_DISP_POWER_ON_SNOW_REFRESH_TIME*10)) 

typedef struct _divoom_disp_power_on_ctrl
{    
    uint8 exit_flag;     
    int    image_id;        
    uint32 endtime;                                 //结束时间
    uint32 handle;    
    DIVOOM_DISP_CTRL_OVER_CALL_END *pcallback;
}divoom_disp_power_on_ctrl, *Pdivoom_disp_power_on_ctrl;

Pdivoom_disp_power_on_ctrl pdivoom_disp_power_on_handle = NULL;

int divoom_disp_power_on_play_over(T_U8 area_id)
{
    uint32 cur_time = divoom_system_get_tickms();
    
    if(pdivoom_disp_power_on_handle && cur_time >pdivoom_disp_power_on_handle->endtime)
    {
        pdivoom_disp_power_on_handle->pcallback(DIVOOM_MAIN_PLAY_POWER_ON_EVENT);
    }
    return 1;
}


void divoom_disp_power_on_gifctrl_callback(T_U8 area_id, Pdivoom_play_gif_image_info image_info)
{
    uint32 cur_time = divoom_system_get_tickms();    
    if(pdivoom_disp_power_on_handle && cur_time >pdivoom_disp_power_on_handle->endtime)
	{
	    if(pdivoom_disp_power_on_handle->exit_flag == 0)
	    {              
			if(pdivoom_disp_power_on_handle->pcallback)
		        pdivoom_disp_power_on_handle->pcallback(DIVOOM_MAIN_PLAY_POWER_ON_EVENT);
	        pdivoom_disp_power_on_handle->exit_flag = 2;
	    }
	}

    image_info->image_id = pdivoom_disp_power_on_handle->image_id;
    image_info->model_id = DIVOOM_MANAGE_IMAGE_PowerOnGif;
}



void divoom_disp_power_on_init(void * data)
{
    Pdivoom_disp_power_on_disp_info pdisp_info = (Pdivoom_disp_power_on_disp_info)data;
	divoom_malloc_check_all_ram_in();
    if(pdivoom_disp_power_on_handle) 
    {
        return;
    }
    pdivoom_disp_power_on_handle = (Pdivoom_disp_power_on_ctrl)divoom_com_malloc(sizeof(divoom_disp_power_on_ctrl), __func__, __LINE__);
    if(pdivoom_disp_power_on_handle == NULL)
    {
        divoom_print("%s:%d: it fails to malloc!\n",__FILE__, __LINE__);
        return ;
    }    
    memset(pdivoom_disp_power_on_handle, 0, sizeof(divoom_disp_power_on_ctrl));
	if(pdisp_info)
	{
    	pdivoom_disp_power_on_handle->pcallback = (DIVOOM_DISP_CTRL_OVER_CALL_END *)pdisp_info->pover_callback;
	}
    if(pdisp_info->power_down_flag)
    {
        pdivoom_disp_power_on_handle->image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + DIVOOM_DISP_DIVOOM_TF_ADAPTER_ERR;
    }
    else
    {
    	pdivoom_disp_power_on_handle->handle = divoom_manage_image_check_exist(DIVOOM_MANAGE_IMAGE_PowerOnGif,DIVOOM_MANAGE_IMAGE_ID_BASE_POWER_ON);
        
    	if(pdivoom_disp_power_on_handle->handle)
    	{
            pdivoom_disp_power_on_handle->image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_POWER_ON;
    	}else
        {
            pdivoom_disp_power_on_handle->image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + DIVOOM_DISP_DIVOOM_POWER_ON;
        
        }
    }
    pdivoom_disp_power_on_handle->endtime = divoom_system_get_tickms() + DIVOOM_DISP_POWER_ON_DISP_MIN_TIME;
    divoom_play_gif_com_init(0,0, -1);
    divoom_play_gif_com_create_image_area(0, 0, 0, SCREEN_WIDTH_64, 0, divoom_disp_power_on_gifctrl_callback, divoom_disp_power_on_play_over);
}

void divoom_disp_power_on_play(uint8 play)
{
}


void divoom_disp_power_on_close(void)
{
    if(pdivoom_disp_power_on_handle)
    {
		divoom_play_gif_com_exit_image();
        divoom_com_free(pdivoom_disp_power_on_handle, __func__, __LINE__);
        pdivoom_disp_power_on_handle = NULL;
    }
	divoom_malloc_check_all_ram_out(__func__);
}

#endif


#include "divoom_disp_com.h"

//#define DIVOOM_EVENT_DISP_ALATM_TIME
#ifdef DIVOOM_EVENT_DISP_ALATM_TIME
#define DIVOOM_ALARM_DISP_TIME_TIMES		10
//单位是10ms一次
#define DIVOOM_ALARM_DISP_TIME_DISP			10
#endif

#define DIVOOM_DISP_EVENT_REFRESH_TICK  1
typedef struct _divoom_disp_event_ctrl
{
    uint8  resume_mode;
    uint32  task_id;
    uint8  pic_index;
    uint8  exit_flag;
    uint8  light_tick;
    uint16  fake_out_start_time;
    uint16  fake_out_end_time;
    uint16 tick;
    uint16 tick_in_period;
#ifdef DIVOOM_EVENT_DISP_ALATM_TIME
	uint8  alarm_flag;
	uint8  time_disp_time;
#endif
    void  *user_define_image_handle;
	uint8 pic_data[DIVOOM_IMAGE_DECODE_RGB_SIZE_64];
    divoom_disp_event_disp_info disp_info;
    DIVOOM_LIGHT_DISP_OVER_CALLBACK *pcallback;
    uint32 notice_id;

	uint8 model_id;
	uint8 event_id;
}divoom_disp_event_ctrl, *Pdivoom_disp_event_ctrl;

static Pdivoom_disp_event_ctrl pdivoom_disp_event_handle = NULL;



#define DIVOOM_DISP_BATTERY_MAX_LEVEL       9
#define DIVOOM_DISP_BATTERY_START_X       6
#define DIVOOM_DISP_BATTERY_START_Y       4
#define DIVOOM_DISP_BATTERY_SIZE_X       4

void divoom_disp_event_display_battery(uint8 level, T_BOOL is_charge, uint8 *out_buf)
{
    uint16 i, j, start, end;
    divoom_screen_color_info f_color = {0,0xff,0}, b_color = {0,0,0x0};
    //uint8 disp_buf[SCREEN_BUF_SIZE_32];
	//充电改为黄色
	if(is_charge == AK_TRUE)
	{
		f_color.r_c = 0xff;
		f_color.g_c = 0xff;
	}

	memset(out_buf, 0, DIVOOM_IMAGE_SMALL_RGB_SIZE);
   // divoom_disp_picture_get_small_rgb_data(DIVOOM_DISP_BATTERY_PIC, out_buf);

    if(level >= DIVOOM_DISP_BATTERY_MAX_LEVEL)
    {
        level = DIVOOM_DISP_BATTERY_MAX_LEVEL;
    }
    for(i = 0; i < DIVOOM_DISP_BATTERY_MAX_LEVEL; i ++)
    {
        if(i == 0)
        {
            start = 1;
            end = DIVOOM_DISP_BATTERY_SIZE_X - 1;
        }
        else
        {
            start = 0;
            end = DIVOOM_DISP_BATTERY_SIZE_X;
        }
        for(j = start; j < end; j++)
        {
            if(level >= (DIVOOM_DISP_BATTERY_MAX_LEVEL - i))
            {
                divoom_disp_ctrl_set_data(DIVOOM_DISP_BATTERY_START_X + j + (DIVOOM_DISP_BATTERY_START_Y + i)*SCREEN_WIDTH_16, &f_color, out_buf);
            }
            else
            {
                divoom_disp_ctrl_set_data(DIVOOM_DISP_BATTERY_START_X + j+(DIVOOM_DISP_BATTERY_START_Y + i)*SCREEN_WIDTH_16, &b_color, out_buf);
            }
        }
    }
    //divoom_screen_disp_rgb_data(disp_buf);
}


void divoom_disp_event_main(void * data)
{
    uint32 level;
    uint16 pic_id;

    pdivoom_disp_event_handle->light_tick ++;
    if(pdivoom_disp_event_handle->fake_out_end_time > pdivoom_disp_event_handle->fake_out_start_time)
    {
        uint8 sys_light;

        pdivoom_disp_event_handle->fake_out_start_time ++;
        sys_light = divoom_main_get_sys_light_hw();
        sys_light = (pdivoom_disp_event_handle->fake_out_end_time - pdivoom_disp_event_handle->fake_out_start_time) * sys_light / pdivoom_disp_event_handle->fake_out_end_time;
        divoom_screen_bright(sys_light);
    }
    if(pdivoom_disp_event_handle->light_tick < 10)
    {
        return ;
    }
    pdivoom_disp_event_handle->light_tick = 0;
    pic_id = pdivoom_disp_event_handle->disp_info.disp_id[pdivoom_disp_event_handle->pic_index];
    pdivoom_disp_event_handle->tick ++;
    if(pdivoom_disp_event_handle->tick_in_period == 0)
    {
        //display picture
        if(pdivoom_disp_event_handle->disp_info.tot_pic == 0 && pdivoom_disp_event_handle->disp_info.pic_data != NULL)
        {    
            uint8 pic_len;
            
			memset(pdivoom_disp_event_handle->pic_data, 0, DIVOOM_IMAGE_SMALL_RGB_SIZE);
            pic_len = divoom_image_decode_decode_pic(pdivoom_disp_event_handle->user_define_image_handle, pdivoom_disp_event_handle->pic_data, NULL);
            if(pic_len == 0)
            {
                divoom_image_decode_reset(pdivoom_disp_event_handle->user_define_image_handle);
            }
            divoom_screen_disp_divoom_data(pdivoom_disp_event_handle->pic_data);
        }
        else
        {
            if(pic_id == 0)
            {
                //显示电池电量
                divoom_disp_ctrl_bat_ctrl bat_info;
                
                divoom_com_get_env_info(DIVOOM_DISP_ENV_BAT, &bat_info);                
                if(pdivoom_disp_event_handle->disp_info.tot_time > pdivoom_disp_event_handle->disp_info.period_time)
                {
                    level = (pdivoom_disp_event_handle->tick <bat_info.bat_level) ? pdivoom_disp_event_handle->tick:bat_info.bat_level;
                }
                else
                {
                    level = bat_info.bat_level;
                }
        		divoom_print("%s, level %d\n", __func__, level);
    			divoom_disp_event_display_battery(level, bat_info.bat_charge, pdivoom_disp_event_handle->pic_data);                
                divoom_screen_disp_rgb16_data(pdivoom_disp_event_handle->pic_data);

            }
            else
            {
                divoom_disp_picture_display(pic_id);
            }
        }
    }
    else if(pdivoom_disp_event_handle->tick_in_period == pdivoom_disp_event_handle->disp_info.on_time)
    {
        //close picture
        divoom_disp_ctrl_clr_screen();
		//取消当前的提醒显示
    }
    pdivoom_disp_event_handle->tick_in_period ++;
    if(pdivoom_disp_event_handle->tick_in_period == pdivoom_disp_event_handle->disp_info.period_time)
    {
        pdivoom_disp_event_handle->tick_in_period = 0;
        pdivoom_disp_event_handle->pic_index ++;
        if(pdivoom_disp_event_handle->pic_index == pdivoom_disp_event_handle->disp_info.tot_pic)
        {
            pdivoom_disp_event_handle->pic_index = 0;
        }
    }
    if((pdivoom_disp_event_handle->tick >= pdivoom_disp_event_handle->disp_info.tot_time) && (pdivoom_disp_event_handle->exit_flag == 0))
    {
        pdivoom_disp_event_handle->exit_flag = 1;
        divoom_print("divoom_disp_event_main: exit:%d, %d\n",(int)pdivoom_disp_event_handle->tick, (int)pdivoom_disp_event_handle->disp_info.tot_time);
        if(pdivoom_disp_event_handle->pcallback)
        {
            pdivoom_disp_event_handle->pcallback(pdivoom_disp_event_handle->notice_id);
        }
        else
        {
			divoom_disp_ctrl_app_exit_mode(DIVOOM_DISP_EVENT_DIS_CTRL, pdivoom_disp_event_handle->model_id, pdivoom_disp_event_handle->event_id);
        }
    }
}

/*********************************************************************
  Function:         divoom_disp_event_update
  Description:     更新显示的图片信息
  Input:            pctrl_info      是divoom_device_event_image这个结构体
  Author:           divoom
**********************************************************************/
void divoom_disp_event_update(Pdivoom_disp_event_disp_info pctrl_info)
{
    if(pdivoom_disp_event_handle == NULL)
    {
        return;
    }
    
    divoom_com_pause_task(pdivoom_disp_event_handle->task_id);
    pdivoom_disp_event_handle->pcallback = pctrl_info->pover_callback;
    pdivoom_disp_event_handle->notice_id = pctrl_info->noice_id;
    if(pdivoom_disp_event_handle->fake_out_end_time)
    {
        divoom_main_apply_sys_light();
        pdivoom_disp_event_handle->fake_out_end_time = 0;
    }
    if(pdivoom_disp_event_handle->disp_info.pic_data)
    {
        divoom_com_free(pdivoom_disp_event_handle->disp_info.pic_data,__func__,__LINE__);
    }
    else
    {
        if(pdivoom_disp_event_handle->user_define_image_handle)
        {
            divoom_image_decode_destoy(pdivoom_disp_event_handle->user_define_image_handle);
            pdivoom_disp_event_handle->user_define_image_handle = NULL;
        }
    }
    memcpy(&pdivoom_disp_event_handle->disp_info, pctrl_info, sizeof(divoom_disp_event_disp_info));
    pdivoom_disp_event_handle->disp_info.pic_data = NULL;
    if(pdivoom_disp_event_handle->disp_info.tot_pic == 0)
    {
        pdivoom_disp_event_handle->disp_info.pic_data = divoom_com_malloc(pdivoom_disp_event_handle->disp_info.pic_len, __func__, __LINE__);
        if(pdivoom_disp_event_handle->disp_info.pic_data == NULL)
        {
            divoom_print("%s:%d: it fails to malloc !\n",__FILE__, __LINE__);
            return ;
        }
        memcpy(pdivoom_disp_event_handle->disp_info.pic_data, pctrl_info->pic_data, pctrl_info->pic_len);
        if(pdivoom_disp_event_handle->user_define_image_handle == NULL)
        {
            pdivoom_disp_event_handle->user_define_image_handle = divoom_image_decode_creat_handle();
        }
        divoom_image_decode_add_data(pdivoom_disp_event_handle->user_define_image_handle, pdivoom_disp_event_handle->disp_info.pic_data, pctrl_info->pic_len, 0);
    }
    pdivoom_disp_event_handle->pic_index = 0;
    pdivoom_disp_event_handle->tick = 0;
    pdivoom_disp_event_handle->tick_in_period = 0;
    pdivoom_disp_event_handle->exit_flag = 0;

	pdivoom_disp_event_handle->model_id = pctrl_info->extend_info.model_id;
	pdivoom_disp_event_handle->event_id = pctrl_info->extend_info.event_id;
    divoom_com_resume_task(pdivoom_disp_event_handle->task_id);
  //  AkDebugOutput("divoom_disp_event_update: %d, %d\n", pdivoom_disp_event_handle->disp_info.tot_time, pdivoom_disp_event_handle->disp_info.tot_time);

}

/*********************************************************************
  Function:         divoom_disp_event_fakeout_time
  Description:     谈入谈出时间
  Input:            pctrl_info      是divoom_device_event_image这个结构体
  Author:           divoom
**********************************************************************/
void divoom_disp_event_fakeout_time(uint32 fake_time)
{
    if(pdivoom_disp_event_handle)
    {
        divoom_com_pause_task(pdivoom_disp_event_handle->task_id);
        pdivoom_disp_event_handle->fake_out_start_time = 0;
        pdivoom_disp_event_handle->fake_out_end_time = fake_time;
        divoom_com_resume_task(pdivoom_disp_event_handle->task_id);
    }
}

/*********************************************************************
  Function:         divoom_disp_event_exit_callback
  Description:     谈入谈出时间
  Input:            callback
  Author:           divoom
**********************************************************************/
void divoom_disp_event_exit_callback(DIVOOM_LIGHT_DISP_OVER_CALLBACK *pcallback)
{
    if(pdivoom_disp_event_handle)
    {
        divoom_com_pause_task(pdivoom_disp_event_handle->task_id);
        pdivoom_disp_event_handle->pcallback = pcallback;
        divoom_com_resume_task(pdivoom_disp_event_handle->task_id);
    }
}

/*********************************************************************
  Function:         divoom_disp_event_init
  Description:     显示提示图片，显示完成后，将还原之前的显示
  Input:            pctrl_info      是divoom_device_event_image这个结构体
                      cur_mode      前一模式
  Author:           divoom
**********************************************************************/
void divoom_disp_event_init(void *pctrl_info, uint8 cur_mode)
{
    if(pdivoom_disp_event_handle)
    {
        return;
    }
    pdivoom_disp_event_handle = (Pdivoom_disp_event_ctrl)divoom_com_malloc(sizeof(divoom_disp_event_ctrl), __func__, __LINE__);
    if(pdivoom_disp_event_handle == NULL)
    {
        return;
    }
    memset(pdivoom_disp_event_handle, 0, sizeof(divoom_disp_event_ctrl));
    pdivoom_disp_event_handle->resume_mode = cur_mode;
    pdivoom_disp_event_handle->task_id = divoom_com_create_task(divoom_disp_event_main, DIVOOM_DISP_EVENT_REFRESH_TICK);
    divoom_com_resume_task(pdivoom_disp_event_handle->task_id);
    divoom_disp_event_update((Pdivoom_disp_event_disp_info)pctrl_info);
    
}


/*********************************************************************
  Function:         divoom_disp_event_play
  Description:     状态控制
  Input:            status
  Author:           divoom
**********************************************************************/

void divoom_disp_event_play(uint8 status)
{
    if(pdivoom_disp_event_handle)
    {
        if(status)
        {
            divoom_com_resume_task(pdivoom_disp_event_handle->task_id);
            divoom_disp_picture_display(pdivoom_disp_event_handle->disp_info.disp_id[pdivoom_disp_event_handle->pic_index]);
        }
        else
        {
            divoom_com_pause_task(pdivoom_disp_event_handle->task_id);
        }
    }
}

/*********************************************************************
  Function:         divoom_disp_event_get_back_mode
  Description:     返回事件通知的前一个模式
  Input:            模式
  Author:           divoom
**********************************************************************/

uint8 divoom_disp_event_get_back_mode(void)
{
    if(pdivoom_disp_event_handle)
    {
        return pdivoom_disp_event_handle->resume_mode;
    }
    else
    {
        return 0;
    }
}


/*********************************************************************
  Function:         divoom_disp_event_close
  Description:     释放资源
  Input:            void
  Author:           divoom
**********************************************************************/

uint8 divoom_disp_event_close(void)
{
    uint8 ret = 0;

	if(pdivoom_disp_event_handle != NULL)
	{
        ret = pdivoom_disp_event_handle->resume_mode;
        if(pdivoom_disp_event_handle->fake_out_end_time)
        {
            divoom_main_apply_sys_light();
            pdivoom_disp_event_handle->fake_out_end_time = 0;
        }
		divoom_com_pause_task(pdivoom_disp_event_handle->task_id);
		divoom_com_destroy_task(pdivoom_disp_event_handle->task_id);
        if(pdivoom_disp_event_handle->disp_info.pic_data)
        {
            divoom_com_free(pdivoom_disp_event_handle->disp_info.pic_data,__func__,__LINE__);
        }
        if(pdivoom_disp_event_handle->user_define_image_handle)
        {
            divoom_image_decode_destoy(pdivoom_disp_event_handle->user_define_image_handle);
        }
		divoom_com_free(pdivoom_disp_event_handle,__func__,__LINE__);
		pdivoom_disp_event_handle = NULL;
        
	}

    return ret;
}


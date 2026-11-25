#include "divoom_disp_com.h"

typedef struct _divoom_disp_sleep_fun_ctrl
{
    uint8 task_id;
    divoom_light_env_time_info tmp_disp_time;
    divoom_light_env_time_info disp_time;
    divoom_disp_sleep_ctrl disp_config;
    uint8 disp_buf[SCREEN_BUF_SIZE_32];
}divoom_disp_sleep_fun_ctrl, *Pdivoom_disp_sleep_fun_ctrl;

static Pdivoom_disp_sleep_fun_ctrl pdivoom_disp_sleep_handle = NULL;
#define  DIVOOM_DISP_SLEEP_TIME_Y 10
#define  DIVOOM_DISP_SLEEP_TIME_HOUR_HIGH_X 1
#define  DIVOOM_DISP_SLEEP_TIME_HOUR_LOW_X  8
#define  DIVOOM_DISP_SLEEP_TIME_MIN_HIGH_X 19
#define  DIVOOM_DISP_SLEEP_TIME_MIN_LOW_X  26
#define  DIVOOM_DISP_SLEEP_TIME_DOT1_X  16
#define  DIVOOM_DISP_SLEEP_TIME_DOT1_Y  (DIVOOM_DISP_SLEEP_TIME_Y + 3)
#define  DIVOOM_DISP_SLEEP_TIME_DOT2_X  16
#define  DIVOOM_DISP_SLEEP_TIME_DOT2_Y  (DIVOOM_DISP_SLEEP_TIME_DOT1_Y + 4)
void divoom_disp_sleep_add_disp_time(void)
{
    divoom_light_env_time_info cur_info = {0};
    divoom_screen_color_info color_info;
    int32 h;
    uint8 s,v;

    divoom_disp_ctrl_app_get_env_info(DIVOOM_DISP_ENV_TIME, &cur_info);  
    memcpy(&pdivoom_disp_sleep_handle->disp_time, &cur_info, sizeof(divoom_light_env_time_info));
    if(divoom_main_get_time_flag() == 0)
    {
        if(cur_info.hour != 12)
        {
            cur_info.hour = cur_info.hour % 12;
            if(cur_info.hour == 0)
            {
                cur_info.hour = 12;
            }
        }
    }
    divoom_RgbToHsv(pdivoom_disp_sleep_handle->disp_config.color.r_c, pdivoom_disp_sleep_handle->disp_config.color.g_c, 
        pdivoom_disp_sleep_handle->disp_config.color.b_c, &h, &s, &v);
    if(h > 85)
    {
        h -= 85;
    }
    else
    {
        h += 85;
    }
    divoom_hsvToRGB(h, s, v, &color_info.r_c, &color_info.g_c, &color_info.b_c);
    divoom_disp_ctrl_fill(pdivoom_disp_sleep_handle->disp_buf, &pdivoom_disp_sleep_handle->disp_config.color);
    //小时
	divoom_disp_digit_fill_area(DIVOOM_DISP_SLEEP_TIME_HOUR_HIGH_X, DIVOOM_DISP_SLEEP_TIME_Y, cur_info.hour/10, DIVOOM_6X12_DIGIT, 
			pdivoom_disp_sleep_handle->disp_buf,  &color_info);
	divoom_disp_digit_fill_area(DIVOOM_DISP_SLEEP_TIME_HOUR_LOW_X, DIVOOM_DISP_SLEEP_TIME_Y, cur_info.hour%10, DIVOOM_6X12_DIGIT, 
			pdivoom_disp_sleep_handle->disp_buf, &color_info);
    //点
    divoom_disp_ctrl_set_data_xy(DIVOOM_DISP_SLEEP_TIME_DOT1_X, DIVOOM_DISP_SLEEP_TIME_DOT1_Y, &color_info, pdivoom_disp_sleep_handle->disp_buf);
    divoom_disp_ctrl_set_data_xy(DIVOOM_DISP_SLEEP_TIME_DOT1_X+1, DIVOOM_DISP_SLEEP_TIME_DOT1_Y, &color_info, pdivoom_disp_sleep_handle->disp_buf);
    divoom_disp_ctrl_set_data_xy(DIVOOM_DISP_SLEEP_TIME_DOT1_X, DIVOOM_DISP_SLEEP_TIME_DOT1_Y+1, &color_info, pdivoom_disp_sleep_handle->disp_buf);
    divoom_disp_ctrl_set_data_xy(DIVOOM_DISP_SLEEP_TIME_DOT1_X+1, DIVOOM_DISP_SLEEP_TIME_DOT1_Y+1, &color_info, pdivoom_disp_sleep_handle->disp_buf);
    divoom_disp_ctrl_set_data_xy(DIVOOM_DISP_SLEEP_TIME_DOT2_X, DIVOOM_DISP_SLEEP_TIME_DOT2_Y, &color_info, pdivoom_disp_sleep_handle->disp_buf);
    divoom_disp_ctrl_set_data_xy(DIVOOM_DISP_SLEEP_TIME_DOT2_X+1, DIVOOM_DISP_SLEEP_TIME_DOT2_Y, &color_info, pdivoom_disp_sleep_handle->disp_buf);
    divoom_disp_ctrl_set_data_xy(DIVOOM_DISP_SLEEP_TIME_DOT2_X, DIVOOM_DISP_SLEEP_TIME_DOT2_Y+1, &color_info, pdivoom_disp_sleep_handle->disp_buf);
    divoom_disp_ctrl_set_data_xy(DIVOOM_DISP_SLEEP_TIME_DOT2_X+1, DIVOOM_DISP_SLEEP_TIME_DOT2_Y+1, &color_info, pdivoom_disp_sleep_handle->disp_buf);
    //分钟
	divoom_disp_digit_fill_area(DIVOOM_DISP_SLEEP_TIME_MIN_HIGH_X, DIVOOM_DISP_SLEEP_TIME_Y, cur_info.min/10, DIVOOM_6X12_DIGIT, 
			pdivoom_disp_sleep_handle->disp_buf,  &color_info);
	divoom_disp_digit_fill_area(DIVOOM_DISP_SLEEP_TIME_MIN_LOW_X, DIVOOM_DISP_SLEEP_TIME_Y, cur_info.min%10, DIVOOM_6X12_DIGIT, 
			pdivoom_disp_sleep_handle->disp_buf, &color_info);
    
}

/*********************************************************************
  Function:         divoom_disp_sleep_update
  Description:     更新睡眠显示模式
   Input:            user_data   闹钟显示数据
  Author:           divoom
**********************************************************************/

void divoom_disp_sleep_update(void * user_data)
{
    Pdivoom_disp_sleep_ctrl sleep_info = (Pdivoom_disp_sleep_ctrl)user_data;

    if(pdivoom_disp_sleep_handle == NULL)
    {
        return;   
    }
    memcpy(&pdivoom_disp_sleep_handle->disp_config, sleep_info, sizeof(divoom_disp_sleep_ctrl));
    divoom_screen_bright(sleep_info->light);
    divoom_disp_sleep_add_disp_time();
    divoom_screen_disp_rgb32_data(pdivoom_disp_sleep_handle->disp_buf);
}


void divoom_disp_sleep_main(void * user_data)
{
    divoom_disp_ctrl_app_get_env_info(DIVOOM_DISP_ENV_TIME, &pdivoom_disp_sleep_handle->tmp_disp_time); 
    if(pdivoom_disp_sleep_handle->tmp_disp_time.hour == pdivoom_disp_sleep_handle->disp_time.hour && 
			pdivoom_disp_sleep_handle->tmp_disp_time.min == pdivoom_disp_sleep_handle->disp_time.min)
    {
        return;
    }
    divoom_disp_sleep_add_disp_time();
    divoom_screen_disp_rgb32_data(pdivoom_disp_sleep_handle->disp_buf);
    
}




/*********************************************************************
  Function:         divoom_disp_sleep_init
  Description:     
   Input:            user_data   显示数据
  Author:           divoom
**********************************************************************/

void divoom_disp_sleep_init(void * user_data)
{
	divoom_malloc_check_all_ram_in();
    pdivoom_disp_sleep_handle = divoom_com_malloc(sizeof(divoom_disp_sleep_fun_ctrl), __func__, __LINE__);
    if(pdivoom_disp_sleep_handle == NULL)
    {
        return;   
    }
    divoom_disp_sleep_update(user_data);
    pdivoom_disp_sleep_handle->task_id = divoom_com_create_task(divoom_disp_sleep_main,100);
    divoom_com_resume_task(pdivoom_disp_sleep_handle->task_id);
 
}


/*********************************************************************
  Function:         divoom_disp_sleep_close
  Description:     退出睡眠状态
   Input:            NONE
  Author:           divoom
**********************************************************************/


void divoom_disp_sleep_close(void)
{
    if(pdivoom_disp_sleep_handle == NULL)
    {
        return;   
    }
    if(pdivoom_disp_sleep_handle->task_id!= T_U8_MAX)
    {
		divoom_com_pause_task(pdivoom_disp_sleep_handle->task_id);
		divoom_com_destroy_task(pdivoom_disp_sleep_handle->task_id);
    }
    divoom_com_free(pdivoom_disp_sleep_handle, __func__, __LINE__);
    pdivoom_disp_sleep_handle = NULL;
    divoom_main_apply_sys_light();
	divoom_malloc_check_all_ram_out(__func__);
}



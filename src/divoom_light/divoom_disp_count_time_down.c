#include "divoom_disp_com.h"

#define DIVOOM_DISP_COUNT_TIME_DOWN_REFRESH_TIME      (30)
#define DIVOOM_DISP_COUNT_TIME_DOWN_REFRESH_OVER_TIME (10)
#define DIVOOM_DISP_COUNT_TIME_DOWN_OVER_DISP_TIME    (100)
/*
   ------->x
   |
   |
   |
   |↓y
*/

#if(DIVOOM_IMAGE_WIDTH == 16)
#define DIVOOM_DISP_COUNT_BIG_X				(8)
#define DIVOOM_DISP_COUNT_LESS_X			(12)
#define DIVOOM_DISP_COUNT_BIG_Y				(2)
#define DIVOOM_DISP_COUNT_LESS_Y			(9)
#define	DIVOOM_DISP_FONT					(DIVOOM_4X7_DIGIT)
#else
#define DIVOOM_DISP_COUNT_DOT_X				(16)
#define DIVOOM_DISP_COUNT_DOT_1_Y			(13)
#define DIVOOM_DISP_COUNT_DOT_2_Y			(17)
#define DIVOOM_DISP_COUNT_1_X				(4)
#define DIVOOM_DISP_COUNT_2_X				(10)
#define DIVOOM_DISP_COUNT_3_X				(18)
#define DIVOOM_DISP_COUNT_4_X				(24)
#define DIVOOM_DISP_COUNT_Y					(12)
#define	DIVOOM_DISP_FONT					(DIVOOM_5X7_DIGIT)
#endif

static const divoom_screen_color_info divoom_disp_count_down_min_color = {0xff,0xff,0xff};
static const divoom_screen_color_info divoom_disp_count_down_second_color = {0xff,0x00,0x00};
static const divoom_screen_color_info divoom_disp_count_down_dot_color = {0xff,0x00,0x00};

typedef struct _divoom_disp_count_time_down
{
	//0为正常显示  divoom_disp_count_time_down_pic_table, 1 显示divoom_disp_count_time_down_close_over , 时间变为 10
	uint8 disp_flag;
    uint8 pic_index;
	uint8 count_time_down_status;
    void *decode_handle;    //解压句柄	
    uint8 disp_buf[SCREEN_BUF_SIZE_32];
	//前一次的时间，对比确定是否需要更新
	//uint32 pre_count_cnt;
	//换算好的当前
	uint32 next_reach_time;
    uint32 task_id;
	DIVOOM_LIGHT_GET_TOOL_INFO get_count_down_func;
}divoom_disp_count_time_down_ctrl, *Pdivoom_disp_count_time_down_ctrl;

Pdivoom_disp_count_time_down_ctrl pdivoom_disp_count_time_down_handle = NULL;

static const uint16 divoom_disp_count_time_down_pic_table[] = 
{
	DIVOOM_DISP_TIMER_ALL_0,
	DIVOOM_DISP_TIMER_ALL_1,
	DIVOOM_DISP_TIMER_ALL_2,
	DIVOOM_DISP_TIMER_ALL_3,
};

//还剩一秒的时候,更新时间变为100ms,显示此表的图片
static const uint16 divoom_disp_count_time_down_close_over[] = 
{
	DIVOOM_DISP_TIMER_ALL_4,
	DIVOOM_DISP_TIMER_ALL_5,
	DIVOOM_DISP_TIMER_ALL_6,
	DIVOOM_DISP_TIMER_ALL_7,
	DIVOOM_DISP_TIMER_ALL_8,
	DIVOOM_DISP_TIMER_ALL_9,
	DIVOOM_DISP_TIMER_ALL_10,
	DIVOOM_DISP_TIMER_ALL_11,
	DIVOOM_DISP_TIMER_ALL_12,
	DIVOOM_DISP_TIMER_ALL_13,
};

/*void divoom_disp_count_time_down_disp_flag(uint8 flag)
{
	if(flag)
	{
		divoom_disp_high_pri_event_struct notice_info = {0};
		notice_info.disp_id[0] = DIVOOM_DISP_COUNT_DOWN_TIME_UP;
		notice_info.disp_id[1] = DIVOOM_DISP_STOPWATCH_15;
		notice_info.on_time = 5;
		notice_info.period_time = 5;
		notice_info.tol_pic = 2;
		notice_info.tol_time = 50;
		divoom_disp_ctrl_app_set_mode(DIVOOM_DISP_HIGH_PRI_EVENT_DIS_CTRL, (void *)&notice_info);
	}
	else
	{
		divoom_disp_event_disp_info notice_info = {0};
		notice_info.disp_id[0] = DIVOOM_DISP_COUNT_DOWN_CANCEL;
		notice_info.disp_id[1] = DIVOOM_DISP_STOPWATCH_15;
		notice_info.on_time = 5;
		notice_info.period_time = 5;
		notice_info.tot_pic = 2;
		notice_info.tot_time = 50;
		divoom_disp_ctrl_app_set_mode(DIVOOM_DISP_EVENT_DIS_CTRL, (void *)&notice_info);
	}
}*/

void divoom_disp_count_time_down_update_info(Pdivoom_disp_count_time_down_struct info)
{
	if(pdivoom_disp_count_time_down_handle)
	{
		divoom_com_pause_task(pdivoom_disp_count_time_down_handle->task_id);
		pdivoom_disp_count_time_down_handle->count_time_down_status = info->status;
		if(info->next_reach_time != pdivoom_disp_count_time_down_handle->next_reach_time)
		{
			pdivoom_disp_count_time_down_handle->next_reach_time = info->next_reach_time;
		}
		/*if(pdivoom_disp_count_time_down_handle->count_time_down_status == DIVOOM_DISP_COUNT_TIME_DOWN_OVER)
		{
			divoom_com_update_task(pdivoom_disp_count_time_down_handle->task_id, DIVOOM_DISP_COUNT_TIME_DOWN_OVER_DISP_TIME);
		}*/
		divoom_com_resume_task(pdivoom_disp_count_time_down_handle->task_id);
	}
	else
	{
		divoom_print("Must init divoom_disp_count_time_down_init first!\n");
		return;
	}
	/*if(status == DIVOOM_DISP_COUNT_TIME_DOWN_OVER)
	{
		//divoom_com_play_cyc_voice(DIVOOM_VOICE_ID_CLOCK_PIANO, 60, 8, NULL);
		divoom_com_play_cyc_voice(DIVOOM_VOICE_ID_CLOCK_PIANO, 0xFFFF, 8, NULL);
	}*/
}

void divoom_disp_count_time_down_main(void *data)
{
	uint8 min, second;
    //uint32 cur_time;
	//uint32 cur_count_cnt = 0;

    //cur_time = anyka_to_divoom_get_system_utc_time();
	//cur_count_cnt = (pdivoom_disp_count_time_down_handle->next_reach_time >= cur_time)?(pdivoom_disp_count_time_down_handle->next_reach_time - cur_time):(0);
	if(pdivoom_disp_count_time_down_handle->get_count_down_func(DIVOOM_DISP_COUNT_TIME_DOWN_MODE, &pdivoom_disp_count_time_down_handle->next_reach_time) == 0)
	{
		divoom_print("%d-->Can't get count down time!\n", __LINE__);
	}

	//min = cur_count_cnt / 60;
	//second = cur_count_cnt % 60;
	min = pdivoom_disp_count_time_down_handle->next_reach_time / 60;
	second = pdivoom_disp_count_time_down_handle->next_reach_time % 60;

	if(pdivoom_disp_count_time_down_handle->count_time_down_status == DIVOOM_DISP_COUNT_TIME_DOWN_WAIT)
	{
        pdivoom_disp_count_time_down_handle->pic_index = 0;
		min = 1;
		second = 0;
	}
	if(pdivoom_disp_count_time_down_handle->disp_flag == 0 && pdivoom_disp_count_time_down_handle->next_reach_time == 1)
	{
		pdivoom_disp_count_time_down_handle->disp_flag = 1;
	}

	/*if(cur_count_cnt == pdivoom_disp_count_time_down_handle->pre_count_cnt)
	{
		return;
	}
	pdivoom_disp_count_time_down_handle->pre_count_cnt = cur_count_cnt;*/

	if(pdivoom_disp_count_time_down_handle->disp_flag == 0)
	{
		divoom_disp_picture_get_rgb_data(divoom_disp_count_time_down_pic_table[pdivoom_disp_count_time_down_handle->pic_index], pdivoom_disp_count_time_down_handle->disp_buf);
	}
	else
	{
		divoom_disp_picture_get_rgb_data(divoom_disp_count_time_down_close_over[pdivoom_disp_count_time_down_handle->pic_index], pdivoom_disp_count_time_down_handle->disp_buf);
	}

	/*if(min == 0 && second == 0 && pdivoom_disp_count_time_down_handle->count_time_down_status == DIVOOM_DISP_COUNT_TIME_DOWN_OVER)
	{
		//时间到状态
		pdivoom_disp_count_time_down_handle->disp_flag++;
		if(pdivoom_disp_count_time_down_handle->disp_flag < 3)
		{
			divoom_screen_disp_rgb_data(pdivoom_disp_count_time_down_handle->disp_buf);
			return;
		}
		else if(pdivoom_disp_count_time_down_handle->disp_flag == 6)
		{
			pdivoom_disp_count_time_down_handle->disp_flag = 0;
		}
	}*/
    
	if(min >= 100)
	{
		min = min % 100;
	}
	divoom_disp_ctrl_set_dot(DIVOOM_DISP_COUNT_DOT_X, DIVOOM_DISP_COUNT_DOT_1_Y, &divoom_disp_count_down_min_color, pdivoom_disp_count_time_down_handle->disp_buf);
	divoom_disp_ctrl_set_dot(DIVOOM_DISP_COUNT_DOT_X, DIVOOM_DISP_COUNT_DOT_2_Y, &divoom_disp_count_down_min_color, pdivoom_disp_count_time_down_handle->disp_buf);
    divoom_disp_digit_fill_area(DIVOOM_DISP_COUNT_1_X, DIVOOM_DISP_COUNT_Y, min/10,
        DIVOOM_DISP_FONT, pdivoom_disp_count_time_down_handle->disp_buf, &divoom_disp_count_down_min_color);
    divoom_disp_digit_fill_area(DIVOOM_DISP_COUNT_2_X, DIVOOM_DISP_COUNT_Y, min%10,
        DIVOOM_DISP_FONT, pdivoom_disp_count_time_down_handle->disp_buf, &divoom_disp_count_down_min_color);
    divoom_disp_digit_fill_area(DIVOOM_DISP_COUNT_3_X, DIVOOM_DISP_COUNT_Y, second/10,
        DIVOOM_DISP_FONT, pdivoom_disp_count_time_down_handle->disp_buf, &divoom_disp_count_down_second_color);
    divoom_disp_digit_fill_area(DIVOOM_DISP_COUNT_4_X, DIVOOM_DISP_COUNT_Y, second%10,
        DIVOOM_DISP_FONT, pdivoom_disp_count_time_down_handle->disp_buf, &divoom_disp_count_down_second_color);
    
    divoom_screen_disp_rgb32_data(pdivoom_disp_count_time_down_handle->disp_buf);
 
    pdivoom_disp_count_time_down_handle->pic_index++;
	if(pdivoom_disp_count_time_down_handle->disp_flag == 0)
	{
		if(pdivoom_disp_count_time_down_handle->pic_index >= (sizeof(divoom_disp_count_time_down_pic_table)/sizeof(divoom_disp_count_time_down_pic_table[0])))
			pdivoom_disp_count_time_down_handle->pic_index = 0;
	}
	else
	{
		if(pdivoom_disp_count_time_down_handle->pic_index >= (sizeof(divoom_disp_count_time_down_close_over)/sizeof(divoom_disp_count_time_down_close_over[0])))
			pdivoom_disp_count_time_down_handle->pic_index = 0;
	}
}

/*********************************************************
  * CREATED TIME	: 2018年 08月 09日 星期四 14:39:06 CST
  * FUNCTION		: divoom_disp_count_time_down_init
  * IUPUT			: uint8 status, uint32 next_reach
  * OUTPUT			: void 
  * DESCRIPTION		: 时间如果是0就给成1分钟，主要还是看status
  *********************************************************/
void divoom_disp_count_time_down_init(Pdivoom_disp_count_time_down_struct info, DIVOOM_LIGHT_GET_TOOL_INFO func)
{
	divoom_malloc_check_all_ram_in();
    if(pdivoom_disp_count_time_down_handle)
    {
        return;
    }
    pdivoom_disp_count_time_down_handle = divoom_com_malloc(sizeof(divoom_disp_count_time_down_ctrl), __func__, __LINE__);
	if(pdivoom_disp_count_time_down_handle == NULL)
	{
        divoom_print("%s:%d: it fails to malloc!\n",__FILE__, __LINE__);
    	return;
	}
    memset(pdivoom_disp_count_time_down_handle, 0, sizeof(divoom_disp_count_time_down_ctrl));
    //pdivoom_disp_count_time_down_handle->disp_value = T_U32_MAX;
	pdivoom_disp_count_time_down_handle->get_count_down_func = func;
	pdivoom_disp_count_time_down_handle->count_time_down_status = info->status;
	pdivoom_disp_count_time_down_handle->next_reach_time = info->next_reach_time;
	pdivoom_disp_count_time_down_handle->disp_flag = 0;
    pdivoom_disp_count_time_down_handle->decode_handle = divoom_image_decode_creat_handle();
	//pdivoom_disp_count_time_down_handle->pre_count_cnt = 0;
    //pdivoom_disp_count_time_down_handle->task_id = divoom_com_create_task(divoom_disp_count_time_down_main, NULL, DIVOOM_DISP_COUNT_TIME_DOWN_REFRESH_TIME);
    pdivoom_disp_count_time_down_handle->task_id = divoom_com_create_task(divoom_disp_count_time_down_main, DIVOOM_DISP_COUNT_TIME_DOWN_REFRESH_OVER_TIME);
    if(pdivoom_disp_count_time_down_handle->task_id == 0xFF)
    {
        divoom_com_free(pdivoom_disp_count_time_down_handle,__func__,__LINE__);
        divoom_print("%s:it fails to create task for disp!\n", __func__);
        return ;
    }
	divoom_disp_count_time_down_main(NULL);
	divoom_com_resume_task(pdivoom_disp_count_time_down_handle->task_id);
}

/*void divoom_disp_count_time_down_play(uint8 status)
{
    if(pdivoom_disp_count_time_down_handle == NULL)
    {
        return;
    }
    if(status)
    {
        divoom_com_resume_task(pdivoom_disp_count_time_down_handle->task_id);
    }
    else
    {
        divoom_com_pause_task(pdivoom_disp_count_time_down_handle->task_id);
		divoom_disp_count_time_down_main(NULL);
    }
}*/

void divoom_disp_count_time_down_close(void)
{
    if(pdivoom_disp_count_time_down_handle == NULL)
    {
        return;
    }
    divoom_com_pause_task(pdivoom_disp_count_time_down_handle->task_id);
    divoom_com_destroy_task(pdivoom_disp_count_time_down_handle->task_id);
    divoom_image_decode_destoy(pdivoom_disp_count_time_down_handle->decode_handle);	
    divoom_com_free(pdivoom_disp_count_time_down_handle,__func__,__LINE__);
    pdivoom_disp_count_time_down_handle = NULL;
	divoom_malloc_check_all_ram_out(__func__);
}




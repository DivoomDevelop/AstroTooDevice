#include "divoom_disp_com.h"

#define DIVOOM_DISP_WATCH_REFRESH_TIME      10

/*
   ------->x
   |
   |
   |
   |↓y
*/

#if(DIVOOM_IMAGE_WIDTH == 16)
#define DIVOOM_DISP_WATCH_BIG_X				(8)
#define DIVOOM_DISP_WATCH_LESS_X			(12)
#define DIVOOM_DISP_WATCH_BIG_Y				(2)
#define DIVOOM_DISP_WATCH_LESS_Y			(9)
#define	DIVOOM_DISP_FONT					(DIVOOM_4X7_DIGIT)
#else
//新的秒表的位置,数字是12,图片最大应该是17,从19行开始,下面预留一行
#define DIVOOM_DISP_WATCH_MIN_BIG_X			(2)
#define DIVOOM_DISP_WATCH_MIN_LESS_X		(9)
#define DIVOOM_DISP_WATCH_SEC_BIG_X			(19)
#define DIVOOM_DISP_WATCH_SEC_LESS_X		(26)
#define DIVOOM_DISP_WATCH_Y					(19)
//先右移一位,在下移一位,再右移一位
#define DIVOOM_DISP_POINT_LESS_X			(16)
#define DIVOOM_DISP_POINT_Y_1				(22)

#define DIVOOM_DISP_POINT_Y_2				(26)

#define	DIVOOM_DISP_FONT					(DIVOOM_6X12_DIGIT)
#endif

static const divoom_screen_color_info divoom_disp_watch_min_color = {0xff,0xff,0xff};
static const divoom_screen_color_info divoom_disp_watch_second_color = {0xff,0x00,0x00};
static const divoom_screen_color_info divoom_disp_watch_dot_color = {0xff,0x00,0x00};
typedef struct _divoom_disp_watch_ctrl
{
	uint8 now_status;
    uint32 task_id;
    uint8 pic_index;
    //uint32 start_time;
    uint32 disp_value;
	DIVOOM_LIGHT_GET_TOOL_INFO get_info_func;
	//保存每次暂停时候已经运行的值
	//uint32 pause_time;
    void *decode_handle;    //解压句柄	
    uint8 disp_buf[SCREEN_BUF_SIZE_32];
}divoom_disp_watch_ctrl, *Pdivoom_disp_watch_ctrl;

Pdivoom_disp_watch_ctrl pdivoom_disp_watch_handle = NULL;


#if 0
static const uint16 divoom_disp_watch_pic_table[] = 
{
	DIVOOM_DISP_STOPWATCH_00,
	DIVOOM_DISP_STOPWATCH_01,
	DIVOOM_DISP_STOPWATCH_02,
	DIVOOM_DISP_STOPWATCH_03,
	DIVOOM_DISP_STOPWATCH_04,
	DIVOOM_DISP_STOPWATCH_05,
	DIVOOM_DISP_STOPWATCH_06,
	DIVOOM_DISP_STOPWATCH_07,
	DIVOOM_DISP_STOPWATCH_08,
	DIVOOM_DISP_STOPWATCH_09,
	DIVOOM_DISP_STOPWATCH_10,
	DIVOOM_DISP_STOPWATCH_11,
	DIVOOM_DISP_STOPWATCH_12,
	DIVOOM_DISP_STOPWATCH_13,
	DIVOOM_DISP_STOPWATCH_14,
	DIVOOM_DISP_STOPWATCH_15,
	DIVOOM_DISP_STOPWATCH_16,
	DIVOOM_DISP_STOPWATCH_17,
	DIVOOM_DISP_STOPWATCH_18,
};
#endif

//总共60张图片,每 DIVOOM_DISP_WATCH_REFRESH_TIME ms变化一张
//想了想,如果用大图load的话,原图的那些数字没有做处理,有点麻烦
static const uint16 divoom_disp_watch_pic_table[] = 
{
	DIVOOM_DISP_NEW_STOPWATCH_0,	DIVOOM_DISP_NEW_STOPWATCH_1,	DIVOOM_DISP_NEW_STOPWATCH_2,	DIVOOM_DISP_NEW_STOPWATCH_3,	DIVOOM_DISP_NEW_STOPWATCH_4,
	DIVOOM_DISP_NEW_STOPWATCH_5,	DIVOOM_DISP_NEW_STOPWATCH_6,	DIVOOM_DISP_NEW_STOPWATCH_7,	DIVOOM_DISP_NEW_STOPWATCH_8,	DIVOOM_DISP_NEW_STOPWATCH_9,
	DIVOOM_DISP_NEW_STOPWATCH_10,	DIVOOM_DISP_NEW_STOPWATCH_11,	DIVOOM_DISP_NEW_STOPWATCH_12,	DIVOOM_DISP_NEW_STOPWATCH_13,	DIVOOM_DISP_NEW_STOPWATCH_14,
	DIVOOM_DISP_NEW_STOPWATCH_15,	DIVOOM_DISP_NEW_STOPWATCH_16,	DIVOOM_DISP_NEW_STOPWATCH_17,	DIVOOM_DISP_NEW_STOPWATCH_18,	DIVOOM_DISP_NEW_STOPWATCH_19,
	DIVOOM_DISP_NEW_STOPWATCH_20,	DIVOOM_DISP_NEW_STOPWATCH_21,	DIVOOM_DISP_NEW_STOPWATCH_22,	DIVOOM_DISP_NEW_STOPWATCH_23,	DIVOOM_DISP_NEW_STOPWATCH_24,
	DIVOOM_DISP_NEW_STOPWATCH_25,	DIVOOM_DISP_NEW_STOPWATCH_26,	DIVOOM_DISP_NEW_STOPWATCH_27,	DIVOOM_DISP_NEW_STOPWATCH_28,	DIVOOM_DISP_NEW_STOPWATCH_29,
	DIVOOM_DISP_NEW_STOPWATCH_30,	DIVOOM_DISP_NEW_STOPWATCH_31,	DIVOOM_DISP_NEW_STOPWATCH_32,	DIVOOM_DISP_NEW_STOPWATCH_33,	DIVOOM_DISP_NEW_STOPWATCH_34,
	DIVOOM_DISP_NEW_STOPWATCH_35,	DIVOOM_DISP_NEW_STOPWATCH_36,	DIVOOM_DISP_NEW_STOPWATCH_37,	DIVOOM_DISP_NEW_STOPWATCH_38,	DIVOOM_DISP_NEW_STOPWATCH_39,
	DIVOOM_DISP_NEW_STOPWATCH_40,	DIVOOM_DISP_NEW_STOPWATCH_41,	DIVOOM_DISP_NEW_STOPWATCH_42,	DIVOOM_DISP_NEW_STOPWATCH_43,	DIVOOM_DISP_NEW_STOPWATCH_44,
	DIVOOM_DISP_NEW_STOPWATCH_45,	DIVOOM_DISP_NEW_STOPWATCH_46,	DIVOOM_DISP_NEW_STOPWATCH_47,	DIVOOM_DISP_NEW_STOPWATCH_48,	DIVOOM_DISP_NEW_STOPWATCH_49,
	DIVOOM_DISP_NEW_STOPWATCH_50,	DIVOOM_DISP_NEW_STOPWATCH_51,	DIVOOM_DISP_NEW_STOPWATCH_52,	DIVOOM_DISP_NEW_STOPWATCH_53,	DIVOOM_DISP_NEW_STOPWATCH_54,
	DIVOOM_DISP_NEW_STOPWATCH_55,	DIVOOM_DISP_NEW_STOPWATCH_56,	DIVOOM_DISP_NEW_STOPWATCH_57,	DIVOOM_DISP_NEW_STOPWATCH_58,	DIVOOM_DISP_NEW_STOPWATCH_59,
};


void divoom_disp_watch_main(void *data)
{   
	divoom_disp_ctrl_watch_ctrl watch_info;
    uint8 min, second;
    //time_t cur_time;
    //uint32 cur_time;

	/*if(pdivoom_disp_watch_handle->now_status != 2)
	{
		//time(&cur_time);
		cur_time = anyka_to_divoom_get_system_utc_time();

		//    if(pdivoom_disp_watch_handle->disp_value == watch_cnt)
		//    {
		//        return;
		//    }

		pdivoom_disp_watch_handle->disp_value  = cur_time - pdivoom_disp_watch_handle->start_time + pdivoom_disp_watch_handle->pause_time;
	}
	else
	{
		pdivoom_disp_watch_handle->disp_value = 0;
		pdivoom_disp_watch_handle->pause_time = 0;
	}*/
	//if(pdivoom_disp_watch_handle->get_info_func(DIVOOM_DISP_WATCH_MODE, &pdivoom_disp_watch_handle->disp_value) == 0)
	if(pdivoom_disp_watch_handle->get_info_func(DIVOOM_DISP_WATCH_MODE, &watch_info) == 0)
	{
		//获取失败
		divoom_print("Get Tool info Fail-->%d\n", DIVOOM_DISP_WATCH_MODE);
	}
	pdivoom_disp_watch_handle->disp_value = watch_info.watch_disp_value;
	min = pdivoom_disp_watch_handle->disp_value / 60;
	second = pdivoom_disp_watch_handle->disp_value % 60;
	//    memset(pdivoom_disp_watch_handle->disp_buf, 0, SCREEN_BUF_SIZE_32);
#if 0
	if(min == 0 && second == 0)
		pdivoom_disp_watch_handle->pic_index = 0;
	divoom_disp_picture_decode_rgb_data(pdivoom_disp_watch_handle->decode_handle,divoom_disp_watch_pic_table[pdivoom_disp_watch_handle->pic_index],pdivoom_disp_watch_handle->disp_buf);            
#endif
	if((min == 0 && second == 0) || (watch_info.ctrl_flag != 1))
		pdivoom_disp_watch_handle->pic_index = 0;
	divoom_disp_picture_get_rgb_data(divoom_disp_watch_pic_table[pdivoom_disp_watch_handle->pic_index], pdivoom_disp_watch_handle->disp_buf);

	if(min >= 100)
	{
		min = min % 100;
	}
	/*divoom_disp_digit_fill_area(DIVOOM_DISP_WATCH_BIG_X, DIVOOM_DISP_WATCH_BIG_Y, min/10,
	  DIVOOM_DISP_FONT, pdivoom_disp_watch_handle->disp_buf,&divoom_disp_watch_min_color);
	  divoom_disp_digit_fill_area(DIVOOM_DISP_WATCH_LESS_X, DIVOOM_DISP_WATCH_BIG_Y,min%10,
	  DIVOOM_DISP_FONT, pdivoom_disp_watch_handle->disp_buf,&divoom_disp_watch_min_color);
	  divoom_disp_digit_fill_area(DIVOOM_DISP_WATCH_BIG_X, DIVOOM_DISP_WATCH_LESS_Y,second/10,
	  DIVOOM_DISP_FONT, pdivoom_disp_watch_handle->disp_buf,&divoom_disp_watch_second_color);
	  divoom_disp_digit_fill_area(DIVOOM_DISP_WATCH_LESS_X, DIVOOM_DISP_WATCH_LESS_Y,second%10,
	  DIVOOM_DISP_FONT, pdivoom_disp_watch_handle->disp_buf,&divoom_disp_watch_second_color);*/
	divoom_disp_digit_fill_area(DIVOOM_DISP_WATCH_MIN_BIG_X, DIVOOM_DISP_WATCH_Y, min/10,
			DIVOOM_DISP_FONT, pdivoom_disp_watch_handle->disp_buf, &divoom_disp_watch_min_color);
	divoom_disp_digit_fill_area(DIVOOM_DISP_WATCH_MIN_LESS_X, DIVOOM_DISP_WATCH_Y, min%10,
			DIVOOM_DISP_FONT, pdivoom_disp_watch_handle->disp_buf, &divoom_disp_watch_min_color);

	divoom_disp_ctrl_set_dot(DIVOOM_DISP_POINT_LESS_X, DIVOOM_DISP_POINT_Y_1, &divoom_disp_watch_min_color, pdivoom_disp_watch_handle->disp_buf);
	divoom_disp_ctrl_set_dot(DIVOOM_DISP_POINT_LESS_X+1, DIVOOM_DISP_POINT_Y_1, &divoom_disp_watch_min_color, pdivoom_disp_watch_handle->disp_buf);
	divoom_disp_ctrl_set_dot(DIVOOM_DISP_POINT_LESS_X, DIVOOM_DISP_POINT_Y_1+1, &divoom_disp_watch_min_color, pdivoom_disp_watch_handle->disp_buf);
	divoom_disp_ctrl_set_dot(DIVOOM_DISP_POINT_LESS_X+1, DIVOOM_DISP_POINT_Y_1+1, &divoom_disp_watch_min_color, pdivoom_disp_watch_handle->disp_buf);

	divoom_disp_ctrl_set_dot(DIVOOM_DISP_POINT_LESS_X, DIVOOM_DISP_POINT_Y_2, &divoom_disp_watch_min_color, pdivoom_disp_watch_handle->disp_buf);
	divoom_disp_ctrl_set_dot(DIVOOM_DISP_POINT_LESS_X+1, DIVOOM_DISP_POINT_Y_2, &divoom_disp_watch_min_color, pdivoom_disp_watch_handle->disp_buf);
	divoom_disp_ctrl_set_dot(DIVOOM_DISP_POINT_LESS_X, DIVOOM_DISP_POINT_Y_2+1, &divoom_disp_watch_min_color, pdivoom_disp_watch_handle->disp_buf);
	divoom_disp_ctrl_set_dot(DIVOOM_DISP_POINT_LESS_X+1, DIVOOM_DISP_POINT_Y_2+1, &divoom_disp_watch_min_color, pdivoom_disp_watch_handle->disp_buf);

	divoom_disp_digit_fill_area(DIVOOM_DISP_WATCH_SEC_BIG_X, DIVOOM_DISP_WATCH_Y, second/10,
			DIVOOM_DISP_FONT, pdivoom_disp_watch_handle->disp_buf, &divoom_disp_watch_second_color);
	divoom_disp_digit_fill_area(DIVOOM_DISP_WATCH_SEC_LESS_X, DIVOOM_DISP_WATCH_Y, second%10,
			DIVOOM_DISP_FONT, pdivoom_disp_watch_handle->disp_buf, &divoom_disp_watch_second_color);

    divoom_screen_disp_rgb32_data(pdivoom_disp_watch_handle->disp_buf);
 
#if 1
    pdivoom_disp_watch_handle->pic_index++;    
    if(pdivoom_disp_watch_handle->pic_index >= (sizeof(divoom_disp_watch_pic_table)/sizeof(divoom_disp_watch_pic_table[0])))
        pdivoom_disp_watch_handle->pic_index = 0;
#endif
}



void divoom_disp_watch_init(void * data, DIVOOM_LIGHT_GET_TOOL_INFO get_info_func)
{
    //Pdivoom_disp_ctrl_watch_ctrl pwatch_info = (Pdivoom_disp_ctrl_watch_ctrl)data;
	
	divoom_malloc_check_all_ram_in();
    if(pdivoom_disp_watch_handle)
    {
        return;
    }
    pdivoom_disp_watch_handle = divoom_com_malloc(sizeof(divoom_disp_watch_ctrl), __func__, __LINE__);
	if(pdivoom_disp_watch_handle == NULL)
	{
        divoom_print("%s:%d: it fails to malloc!\n",__FILE__, __LINE__);
    	return;
	}
    memset(pdivoom_disp_watch_handle, 0, sizeof(divoom_disp_watch_ctrl));
    pdivoom_disp_watch_handle->disp_value = 0;
	pdivoom_disp_watch_handle->pic_index = 0;
    //pdivoom_disp_watch_handle->pause_time = pwatch_info->pause_time;
    //pdivoom_disp_watch_handle->start_time = pwatch_info->start_time;
    //pdivoom_disp_watch_handle->now_status = pwatch_info->ctrl_flag;
	//divoom_print("pdivoom_disp_watch_handle->now_status = %d\n", pdivoom_disp_watch_handle->now_status);
	pdivoom_disp_watch_handle->get_info_func = get_info_func;
    pdivoom_disp_watch_handle->decode_handle = divoom_image_decode_creat_handle();
    pdivoom_disp_watch_handle->task_id = divoom_com_create_task(divoom_disp_watch_main, DIVOOM_DISP_WATCH_REFRESH_TIME);
    if(pdivoom_disp_watch_handle->task_id == 0xFF)
    {
        divoom_com_free(pdivoom_disp_watch_handle,__func__,__LINE__);
        divoom_print("%s:it fails to create task for disp!\n", __func__);
        return ;
    }
    divoom_com_resume_task(pdivoom_disp_watch_handle->task_id);
    divoom_disp_watch_main(NULL);
}

//void divoom_disp_watch_play(uint8 status)
void divoom_disp_watch_play(Pdivoom_disp_ctrl_watch_ctrl info)
{
    if(pdivoom_disp_watch_handle == NULL || info == NULL)
    {
        return;
    }
	//divoom_com_pause_task(pdivoom_disp_watch_handle->task_id);
    //pdivoom_disp_watch_handle->start_time = info->start_time;
    //pdivoom_disp_watch_handle->now_status = info->ctrl_flag;
	//divoom_com_resume_task(pdivoom_disp_watch_handle->task_id);
    /*if(pdivoom_disp_watch_handle->now_status)
    {
        divoom_com_resume_task(pdivoom_disp_watch_handle->task_id);
    }
    else
    {
        divoom_com_pause_task(pdivoom_disp_watch_handle->task_id);
		//pdivoom_disp_watch_handle->pause_time = pdivoom_disp_watch_handle->disp_value;
        divoom_disp_watch_main(NULL);
    }*/
}

void divoom_disp_watch_close(void)
{
    if(pdivoom_disp_watch_handle == NULL)
    {
        return;
    }
    divoom_com_pause_task(pdivoom_disp_watch_handle->task_id);
    divoom_com_destroy_task(pdivoom_disp_watch_handle->task_id);
    divoom_image_decode_destoy(pdivoom_disp_watch_handle->decode_handle);	
    divoom_com_free(pdivoom_disp_watch_handle,__func__,__LINE__);
    pdivoom_disp_watch_handle = NULL;
	divoom_malloc_check_all_ram_out(__func__);
}




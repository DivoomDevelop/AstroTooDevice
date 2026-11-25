#include "divoom_disp_com.h"

#define MAX_ARRAY_NUM						(750)

//多彩数字点的位置
//x,y要set4次,x,y;x,y+1;x+1,y;x+1,y+1
#define ENV_TIME_COLOURFUL_NUM_P_X			(1)
#define ENV_TIME_COLOURFUL_NUM_P_BIG_Y		(12)
#define ENV_TIME_COLOURFUL_NUM_P_SMALL_Y	(18)
//多彩数字的位置
#define ENV_TIME_COLOURFUL_NUM_BIG_X		(7)
#define ENV_TIME_COLOURFUL_NUM_SMALL_X		(20)
#define ENV_TIME_COLOURFUL_NUM_BIG_Y		(4)
#define ENV_TIME_COLOURFUL_NUM_SMALL_Y		(19)

const static int need_offset[][2] = {
	{-1, 0},
	{-1, -1},
	{-1, 1},
	{0, -1},
	{0, 1},
	{1, -1},
	{1, 0},
	{1, 1},
};

//每一个字块有四个颜色,共有四个字块
//[总共几个][第几个圈的颜色][RGB]
const uint8 divoom_light_env_colourful_num[][4][3] = 
{
	{{255,13,35},  {255,98,102},  {158,7,21},   {80,23,24}},
	{{125,224,66}, {208,255,121}, {0,135,31},   {15,66,24}},
	{{255,189,54}, {255,224,129}, {155,107,31}, {78,59,27}},
	{{0,164,233},  {68,222,253},  {0,131,149},  {6,61,75}}
};

typedef struct _divoom_common_coulourful_num_xy_
{
	uint8 x;
	uint8 y:6;
	//记录当前点是第几个块的颜色,总共4个区域
	uint8 colour_type:2;
}divoom_common_coulourful_num_xy, *Pdivoom_common_coulourful_num_xy;

typedef struct _divoom_common_ctrl
{
	//uint16 Bigest_offset;
	Pdivoom_common_coulourful_num_xy cur_array;
	Pdivoom_common_coulourful_num_xy next_array;
}divoom_common_ctrl, *Pdivoom_common_ctrl;

#define COMMON_JUST_NUM_LESS_X					(8)
#define COMMON_JUST_NUM_LESS_Y					(2)
#define COMMON_JUST_NUM_BIG_X					(20)
#define COMMON_JUST_NUM_BIG_Y					(18)

Pdivoom_common_ctrl pdivoom_common_handle = NULL;

//申请空间,总共4k
//一直使用,不释放
void divoom_disp_time_common_time_init(void)
{
	if(pdivoom_common_handle != NULL)
	{
		return;
	}
	pdivoom_common_handle = divoom_com_malloc(sizeof(divoom_common_ctrl), __func__, __LINE__);
	if(pdivoom_common_handle == NULL)
	{
		goto ERROR_RETURN;
	}
	memset(pdivoom_common_handle, 0, sizeof(divoom_common_ctrl));
	pdivoom_common_handle->cur_array = divoom_com_malloc(sizeof(divoom_common_coulourful_num_xy)*MAX_ARRAY_NUM, __func__, __LINE__);
	if(pdivoom_common_handle->cur_array == NULL)
	{
		goto ERROR_RETURN1;
	}
	pdivoom_common_handle->next_array = divoom_com_malloc(sizeof(divoom_common_coulourful_num_xy)*MAX_ARRAY_NUM, __func__, __LINE__);
//	divoom_print("Fwl_GetUsedMem() = %d\n", Fwl_GetUsedMem());
	if(pdivoom_common_handle->next_array == NULL)
	{
		goto ERROR_RETURN2;
	}
	return;
ERROR_RETURN2:
	divoom_print("Some memory in here:%s--%d\n", __func__, __LINE__);
	if(pdivoom_common_handle->next_array != NULL)
	{
		divoom_com_free(pdivoom_common_handle->next_array, __func__, __LINE__);
	}
ERROR_RETURN1:
	divoom_print("Some memory in here:%s--%d\n", __func__, __LINE__);
	if(pdivoom_common_handle->cur_array != NULL)
	{
		divoom_com_free(pdivoom_common_handle->cur_array, __func__, __LINE__);
	}
ERROR_RETURN:
	divoom_print("Some memory in here:%s--%d\n", __func__, __LINE__);
	if(pdivoom_common_handle!= NULL)
	{
		divoom_com_free(pdivoom_common_handle, __func__, __LINE__);
		pdivoom_common_handle = NULL;
	}
	return;
}

void divoom_disp_time_common_time_colse(void)
{
	if(pdivoom_common_handle)
	{
		Pdivoom_common_ctrl handle = pdivoom_common_handle;
		Pdivoom_common_coulourful_num_xy tmp_arry;
		if(handle->cur_array)
		{
			tmp_arry = handle->cur_array;
			handle->cur_array = NULL;
			divoom_com_free(tmp_arry, __func__, __LINE__);
		}
		if(handle->next_array)
		{
			tmp_arry = handle->next_array;
			handle->next_array = NULL;
			divoom_com_free(tmp_arry, __func__, __LINE__);
		}
		pdivoom_common_handle = NULL;
		divoom_com_free(handle, __func__, __LINE__);
	}
}


//保留,其他模块有使用
//修改为新加的效果模式
//不再需要颜色
void divoom_disp_time_common_get_time_buf_ext(uint8 *time_buf, Pdivoom_light_env_time_info time_info_in, uint8 dot_flag)
{
	divoom_light_env_time_info time_info;

    uint32 seconds;
	if(time_info_in == NULL)
	{
		seconds = divoom_com_get_system_time();

		memset(time_buf, 0, SCREEN_BUF_SIZE_32);
		if(divoom_com_get_data_from_app(DIVOOM_DISP_COM_GET_TIME_FLAG, NULL))
		{
			time_info.hour = (seconds/3600)%24;
		}
		else
		{
			time_info.hour = (seconds/3600)%12;
			if(time_info.hour == 0)
				time_info.hour = 12;
		}
		time_info.min = (seconds%3600)/60;
	}
	else
	{
		time_info.hour = time_info_in->hour;
		time_info.min = time_info_in->min;
	}

	//pdivoom_common_handle->cur_array = divoom_com_malloc(sizeof(divoom_common_coulourful_num_xy)*1024, __func__, __LINE__);
	//pdivoom_common_handle->next_array = divoom_com_malloc(sizeof(divoom_common_coulourful_num_xy)*1024, __func__, __LINE__);
   // divoom_disp_time_disp_led_time(time_buf,&time_info, dot_flag);

}

//保留,其他模块有使用
//修改为新加的效果模式
//不再需要颜色
void divoom_disp_time_common_get_time_buf(uint8 *time_buf, Pdivoom_light_env_time_info time_info_in, uint8 dot_flag)

{
}
    /*void divoom_disp_time_common_get_time_buf(uint8 *time_buf, Pdivoom_screen_color_info colour)
{
    // uint8 time_str[10], hour_info;
    uint8 hour_info;
    uint32 seconds = divoom_com_get_system_time();
    
    memset(time_buf, 0, SCREEN_BUF_SIZE_32);
    if(divoom_com_get_data_from_app(DIVOOM_DISP_COM_GET_TIME_FLAG, NULL))
    {
        hour_info = (seconds/3600)%24;
    }
    else
    {
        hour_info = (seconds/3600)%12;
        if(hour_info == 0)
            hour_info = 12;
    }
    seconds = (seconds%3600)/60;
#if(SCREEN_WIDTH_32 == 32)
	divoom_light_env_disp_big_digit(COMMON_JUST_NUM_LESS_X, COMMON_JUST_NUM_LESS_Y, hour_info/10, 
			false, time_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM, colour);
	divoom_light_env_disp_big_digit(COMMON_JUST_NUM_BIG_X, COMMON_JUST_NUM_LESS_Y, hour_info%10, 
			false, time_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM, colour);
	divoom_light_env_disp_big_digit(COMMON_JUST_NUM_LESS_X, COMMON_JUST_NUM_BIG_Y, seconds/10, 
			false, time_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM, colour);
	divoom_light_env_disp_big_digit(COMMON_JUST_NUM_BIG_X, COMMON_JUST_NUM_BIG_Y, seconds%10, 
			false, time_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM, colour);
#else
    divoom_light_env_disp_big_digit(4, 0, hour_info/10, false, time_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM, colour);
    divoom_light_env_disp_big_digit(10, 0, hour_info%10, false, time_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM, colour);
    divoom_light_env_disp_big_digit(4, 9, seconds/10, false, time_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM, colour);
    divoom_light_env_disp_big_digit(10, 9, seconds%10, false, time_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM, colour);
#endif
}*/


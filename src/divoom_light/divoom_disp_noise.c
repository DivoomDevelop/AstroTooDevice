#include "divoom_disp_com.h"

#define DIVOOM_DISP_NOISE_REFRESH_TIME 20

typedef struct _divoom_disp_noise_ctrl
{
    uint32 task_id;
    uint8 disp_level;
    uint8 start_flag;
    uint16 pic_id;
    uint32 disp_value;
    uint32 cur_value;
    void *decode_handle;    //解压句柄	    
    uint8  disp_buf[SCREEN_BUF_SIZE_32];
}divoom_disp_noise_ctrl, *Pdivoom_disp_noise_ctrl;

Pdivoom_disp_noise_ctrl pdivoom_disp_noise_handle = NULL;

typedef struct _divoom_disp_noise_item
{
    uint8 value;
    uint8 pic_index;    
    divoom_screen_color_info color;
}divoom_disp_noise_item, *Pdivoom_disp_noise_item;




static const uint16 divoom_disp_noise_pic_table[] = 
{
    /*DIVOOM_DISP_NOISE_METER_00,
    DIVOOM_DISP_NOISE_METER_01,
    DIVOOM_DISP_NOISE_METER_02,
    DIVOOM_DISP_NOISE_METER_03,
    DIVOOM_DISP_NOISE_METER_04,
    DIVOOM_DISP_NOISE_METER_05,
    DIVOOM_DISP_NOISE_METER_06,
    DIVOOM_DISP_NOISE_METER_07,*/
	DIVOOM_DISP_NEW_NOISE_0, DIVOOM_DISP_NEW_NOISE_1, DIVOOM_DISP_NEW_NOISE_2, DIVOOM_DISP_NEW_NOISE_3, DIVOOM_DISP_NEW_NOISE_4, DIVOOM_DISP_NEW_NOISE_5,
	DIVOOM_DISP_NEW_NOISE_6, DIVOOM_DISP_NEW_NOISE_7, DIVOOM_DISP_NEW_NOISE_8, DIVOOM_DISP_NEW_NOISE_9, DIVOOM_DISP_NEW_NOISE_10, DIVOOM_DISP_NEW_NOISE_11,
	DIVOOM_DISP_NEW_NOISE_12, DIVOOM_DISP_NEW_NOISE_13, DIVOOM_DISP_NEW_NOISE_14, DIVOOM_DISP_NEW_NOISE_15, DIVOOM_DISP_NEW_NOISE_16, DIVOOM_DISP_NEW_NOISE_17,
};



const divoom_disp_noise_item divoom_disp_noise_color_table[] =
{
//    {15,{2, 118,44}},
//    {20,{42, 163, 66}},
//    {40, {205, 118,0}},
//    {60,{246, 118,1}},
//    {70,{249, 118,18}},
//    {90,{233, 118,0}},
//    {200,{227, 118,46}},

    {12,0,{0,0,255}},
    {25,1,{0,0,255}},
    {37,2,{0,255,0}},
    {50,3,{0,255,0}},
    {62,4,{255,255,0}},
    {75,5,{255,255,0}},
    {88,6,{255,0,0}},
    {200,7,{255,0,0}},    
};


/*********************************************************************
  Function:         divoom_light_env_disp_big_digit
  Description:     用三位显示大数据3*5
  Input:             x      起始位置
                       y      起始位置
                       value   显示的值
  Return:           NONE
  Author:           divoom
**********************************************************************/

uint8 divoom_disp_noise_disp_digit(uint8 x, uint8 y, uint16 value, uint8 disp_type, uint8 *buf, const divoom_screen_color_info *color)
{
    uint8  i, disp_info[3], start_x = x, start_y = y, flag = 0;

    disp_info[0] = value / 100;
    disp_info[1] = (value % 100) / 10;
    disp_info[2] = value % 10;
    
    for(i = 0; i < 3; i ++)
    {
        if(disp_info[i] || i == 2)
        {
            flag = 1;
        }
        
        if(flag)
        {
			divoom_disp_digit_fill_area(start_x, start_y, disp_info[i],
					DIVOOM_6X10_FANG_DIGIT, buf, color);
            //divoom_light_env_fill_area(start_x, start_y, 3, 5,disp_info[i],
            //    divoom_light_env_get_digit, buf, color);

//            divoom_disp_digit_fill_area(start_x, start_y,disp_info[i],DIVOOM_3X5_DIGIT, buf,divoom_disp_env_get_fill_color());
            
            start_x += 3 + 1;
        }
        else if(disp_type)
        {
            start_x += 3 + 1;
        }
    }
    
    return start_x;
}

void divoom_disp_noise_main(void * data)
{
    uint8 i, level, value;
    pdivoom_disp_noise_handle->cur_value = divoom_eq_noise_mic_fft_get_noise_value();
	value = pdivoom_disp_noise_handle->cur_value;

    for(i = 0; i < sizeof(divoom_disp_noise_color_table)/sizeof(divoom_disp_noise_item); i++)
    {
        level = i;
        if(pdivoom_disp_noise_handle->cur_value <= divoom_disp_noise_color_table[i].value)
        {
            break;
        }
    }   
    //pdivoom_disp_noise_handle->pic_id = divoom_disp_noise_pic_table[divoom_disp_noise_color_table[level].pic_index];
    pdivoom_disp_noise_handle->disp_level = level;

//    divoom_disp_ctrl_fill(pdivoom_disp_noise_handle->disp_buf, &color);

    divoom_disp_picture_get_rgb_data(divoom_disp_noise_pic_table[pdivoom_disp_noise_handle->pic_id], pdivoom_disp_noise_handle->disp_buf);

    
    if(value > 99)
    {
        value = 99;
    }
    //divoom_disp_noise_disp_digit(2, 20, value/10, false, pdivoom_disp_noise_handle->disp_buf, &divoom_disp_noise_color_table[level].color);
    //divoom_disp_noise_disp_digit(10, 20, value%10, false, pdivoom_disp_noise_handle->disp_buf, &divoom_disp_noise_color_table[level].color);   
    divoom_disp_noise_disp_digit(2, 20, value/10, false, pdivoom_disp_noise_handle->disp_buf, &divoom_disp_noise_color_table[level].color);
    divoom_disp_noise_disp_digit(10, 20, value%10, false, pdivoom_disp_noise_handle->disp_buf, &divoom_disp_noise_color_table[level].color);
	//18, 26
//    divoom_disp_env_fill_area(9, 10, 7, 5,0,divoom_disp_env_noise_sep_digit, pdivoom_disp_noise_handle->disp_buf, &divoom_disp_noise_color_table[level].color);

    divoom_screen_disp_rgb32_data(pdivoom_disp_noise_handle->disp_buf);

	pdivoom_disp_noise_handle->pic_id++;
	if(pdivoom_disp_noise_handle->pic_id >= sizeof(divoom_disp_noise_pic_table)/sizeof(divoom_disp_noise_pic_table[0]))
		pdivoom_disp_noise_handle->pic_id = 0;
   
}


void divoom_disp_noise_init(void)
{
	divoom_malloc_check_all_ram_in();
    if(pdivoom_disp_noise_handle)
    {
        return;
    }

    pdivoom_disp_noise_handle = divoom_com_malloc(sizeof(divoom_disp_noise_ctrl), __FILE__, __LINE__);
    if(pdivoom_disp_noise_handle == NULL)
    {
        divoom_print("%s:%d: it fails to malloc!\n",__FILE__, __LINE__);
        return ;
    }
	divoom_eq_noise_mic_fft_start(NULL, DIVOOM_FFT_MIC_NOISE);
    memset(pdivoom_disp_noise_handle, 0, sizeof(divoom_disp_noise_ctrl));
    pdivoom_disp_noise_handle->decode_handle = divoom_image_decode_creat_handle();  	    
    pdivoom_disp_noise_handle->task_id = divoom_com_create_task(divoom_disp_noise_main, DIVOOM_DISP_NOISE_REFRESH_TIME);
    pdivoom_disp_noise_handle->cur_value = 40;
    divoom_com_resume_task(pdivoom_disp_noise_handle->task_id);
}

void divoom_disp_noise_update(uint32 value)
{
    if(pdivoom_disp_noise_handle)
    {
        if(value)
        {
            pdivoom_disp_noise_handle->start_flag = 1;
        }
        if(pdivoom_disp_noise_handle->start_flag)
        {
            pdivoom_disp_noise_handle->cur_value = value;
        }
    }
}


void divoom_disp_noise_close(void)
{
    if(pdivoom_disp_noise_handle)
    {
        divoom_com_pause_task(pdivoom_disp_noise_handle->task_id);
        divoom_com_destroy_task(pdivoom_disp_noise_handle->task_id);
        divoom_image_decode_destoy(pdivoom_disp_noise_handle->decode_handle);	
 		divoom_eq_noise_mic_fft_stop();
        divoom_com_free(pdivoom_disp_noise_handle,__func__,__LINE__);
        pdivoom_disp_noise_handle = NULL;
    }
	divoom_malloc_check_all_ram_out(__func__);
}



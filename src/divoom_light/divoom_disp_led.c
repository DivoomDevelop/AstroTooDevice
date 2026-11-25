#include "divoom_disp_com.h"
#if 0
typedef struct _divoom_disp_led_ctrl
{
    uint8  task_run;
    uint32 next_time;
    uint32 start_disp_time;
    uint32 sep_disp_time;
    void *handle;
	uint8 pic_buf[DIVOOM_IMAGE_ENCODE_MAX_64_SIZE];
}divoom_disp_led_ctrl, *Pdivoom_disp_led_ctrl;


static Pdivoom_disp_led_ctrl pdivoom_disp_led_handle = NULL;

/*********************************************************
  Function:		divoom_disp_led_read_loop
  Description:		读数据模块
  Input:			data
  Return:			NONE
************************************************************/

void divoom_disp_led_read_loop(void *data)
{
    uint16 pic_len;
    uint16 time;
    uint32 cur_time = divoom_system_get_tickms();

    if(pdivoom_disp_led_handle && pdivoom_disp_led_handle->next_time <= cur_time)
    {
        if(pdivoom_disp_led_handle->start_disp_time < cur_time)
        {
            divoom_light_word_ctrl_task(AK_FALSE);
            //divoom_disp_time_common_get_time_buf_ext(pdivoom_disp_led_handle->pic_buf, NULL, 1);
            divoom_screen_disp_rgb32_data(pdivoom_disp_led_handle->pic_buf);
            pdivoom_disp_led_handle->next_time = cur_time + 5000;
            pdivoom_disp_led_handle->start_disp_time = cur_time + pdivoom_disp_led_handle->sep_disp_time;
            divoom_load_movie_reset(pdivoom_disp_led_handle->handle);
            pdivoom_disp_led_handle->task_run = 0;
            return;
        }
        else if(pdivoom_disp_led_handle->task_run == 0)
        {
            pdivoom_disp_led_handle->task_run = 1;
            divoom_light_word_ctrl_task(AK_TRUE);
        }
        pic_len = divoom_load_movie_load(pdivoom_disp_led_handle->handle, pdivoom_disp_led_handle->pic_buf, NULL);
        if(pic_len)
        {
            time = divoom_image_decode_get_time_info(pdivoom_disp_led_handle->pic_buf);
            if(time == 0)
            {
                time = 500;
            }
            pdivoom_disp_led_handle->next_time += time;
            divoom_light_word_update_base_color(pdivoom_disp_led_handle->pic_buf, pic_len, 0);            
        }
        else
        {
            pdivoom_disp_led_handle->next_time += 100;
        }
    }
}

/*********************************************************
  Function:		divoom_disp_led_init
  Description:		启动文字显示模块
  Input:			start_page
  Return:			NONE
************************************************************/

void divoom_disp_led_init(uint8 type,uint32 file_index, uint32 disp_time)
{
    uint16 pic_len, time;
    uint8 disp_buf[DIVOOM_PAGE_SIZE];
    
    if(pdivoom_disp_led_handle)
    {
        return;
    }
//    pic_type = divoom_save_led_get_type(type, file_index);
//    if(pic_type != DIVOOM_SAVE_LED_LED)
//    {
//        return ;
//    }
    pdivoom_disp_led_handle = divoom_com_malloc(sizeof(divoom_disp_led_ctrl), __func__, __LINE__);
    memset(pdivoom_disp_led_handle, 0, sizeof(divoom_disp_led_ctrl));  
        
    pdivoom_disp_led_handle->handle = divoom_load_movie_open_read(type, file_index, disp_buf, T_U32_MAX);
    if(pdivoom_disp_led_handle->handle == NULL)
    {
        divoom_print("pdivoom_disp_led_handle: fail!\n");
        return ;
    }

    if(disp_time)
    {
        pdivoom_disp_led_handle->sep_disp_time = disp_time * 1000;
        pdivoom_disp_led_handle->start_disp_time = pdivoom_disp_led_handle->sep_disp_time + divoom_system_get_tickms();
    }
    else
    {
        pdivoom_disp_led_handle->start_disp_time = T_U32_MAX;
    }
    pdivoom_disp_led_handle->task_run = 1;
    if(divoom_light_word_init(NULL, 0))
    {
        pic_len = divoom_load_movie_load(pdivoom_disp_led_handle->handle, pdivoom_disp_led_handle->pic_buf, NULL);
        divoom_light_word_update_speed(disp_buf[0]);
        divoom_light_word_update_word(disp_buf[1], (uint16 *)&disp_buf[2], 1);
        divoom_light_word_set_mul_flag(1);
    }
    else
    {
       return ;
    }
    time = divoom_image_decode_get_time_info(pdivoom_disp_led_handle->pic_buf);  
    
    if(time == 0)
    {
        time = 500;
    }      
    pdivoom_disp_led_handle->next_time = time + divoom_system_get_tickms();
    divoom_light_word_update_base_color(pdivoom_disp_led_handle->pic_buf, pic_len, 1);
//    divoom_main_regiest_loop(NULL,divoom_disp_led_read_loop);
}


/*********************************************************
  Function:		divoom_disp_led_close
  Description:		退出文字显示模块
  Input:			none
  Return:			NONE
************************************************************/

void divoom_disp_led_close(void)
{
    if(pdivoom_disp_led_handle->handle)
    {
        divoom_load_movie_close(pdivoom_disp_led_handle->handle);
    }
//    divoom_main_unregiest_loop(divoom_disp_led_read_loop);
    divoom_light_word_close();
    divoom_com_free(pdivoom_disp_led_handle, __func__, __LINE__);
    pdivoom_disp_led_handle = NULL;
}
#endif


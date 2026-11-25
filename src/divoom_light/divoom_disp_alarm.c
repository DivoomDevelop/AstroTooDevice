#include "divoom_disp_com.h"

#if 0
#define DIVOOM_DISP_ALARM_REFRESH_TICK 50
//#define DIVOOM_DISP_ALARM_SLEEP_LIGHT  30


enum
{
    DIVOOM_DISP_ALARM_DISP_DEFAULT_GIF_MODE,
    DIVOOM_DISP_ALARM_DISP_SNOOZE_MODE,
	DIVOOM_DISP_ALARM_DISP_GIF_CTRL_MODE,
};



typedef struct _divoom_disp_alarm_normal_pic_info
{
    uint8 mode;
    uint16 pic_id;
}divoom_disp_alarm_normal_pic_info, *Pdivoom_disp_alarm_normal_pic_info;
typedef struct _divoom_disp_alarm_ctrl
{
	uint8 disp_mode;
	uint8 task_id;
    uint8 refresh_time;
	uint32 handle;
	uint32 image_id;
    void *decode_handle;
    uint8 pic_buf[DIVOOM_IMAGE_ENCODE_MAX_64_SIZE];
	uint8 disp_tick;
}divoom_disp_alarm_ctrl, *Pdivoom_disp_alarm_ctrl;

Pdivoom_disp_alarm_ctrl pdivoom_disp_alarm_handle = NULL;

uint16 divoom_disp_alarm_get_local_gif_data(uint8 *pic_buf)
{
    uint8 *pic_raw_data;
    uint16 pic_len;
    
    pic_raw_data =  divoom_image_decode_get_pic_data(pdivoom_disp_alarm_handle->decode_handle,  &pic_len,  NULL);
    if(pic_raw_data == NULL)
    {
        divoom_image_decode_reset(pdivoom_disp_alarm_handle->decode_handle);
        pic_raw_data =  divoom_image_decode_get_pic_data(pdivoom_disp_alarm_handle->decode_handle,  &pic_len,  NULL);
    }
    memcpy(pic_buf, pic_raw_data, pic_len);
    return pic_len;
}


  
void divoom_disp_alarm_main(void * data)
{
	switch(pdivoom_disp_alarm_handle->disp_mode)
	{
		case DIVOOM_DISP_ALARM_DISP_DEFAULT_GIF_MODE:
			{
				if(pdivoom_disp_alarm_handle->decode_handle)
				{
					divoom_disp_alarm_get_local_gif_data(pdivoom_disp_alarm_handle->pic_buf);
					divoom_screen_disp_divoom_data(pdivoom_disp_alarm_handle->pic_buf);
				}
			}
			break;

		case DIVOOM_DISP_ALARM_DISP_SNOOZE_MODE:
			{
				if(0 == pdivoom_disp_alarm_handle->handle)
				{
					//在snooze状态,要时间和贪睡图标轮流显示
					if(pdivoom_disp_alarm_handle->disp_tick >= 10 && pdivoom_disp_alarm_handle->disp_tick < 16)
					{
						memset(pdivoom_disp_alarm_handle->pic_buf, 0, sizeof(pdivoom_disp_alarm_handle->pic_buf));
						//divoom_com_get_data_from_app(DIVOOM_DISP_COM_GET_TIME_COLOUR, (uint8*)&colour);
						//divoom_disp_time_common_get_time_buf(pdivoom_disp_alarm_handle->pic_buf, &colour);
						//divoom_disp_time_common_get_time_buf_ext(pdivoom_disp_alarm_handle->pic_buf, NULL, 1);
						divoom_screen_disp_rgb32_data(pdivoom_disp_alarm_handle->pic_buf);
					}
					else if(pdivoom_disp_alarm_handle->disp_tick == 16)
					{
						divoom_disp_picture_display(DIVOOM_DISP_ALARM_SNOOZ_PIC);
						pdivoom_disp_alarm_handle->disp_tick = 0;
					}
					pdivoom_disp_alarm_handle->disp_tick ++;
				}
			}
			break;

		default:
			break;
	}
	

}


void divoom_disp_alarm_gifctrl_callback(T_U8 area_id, Pdivoom_play_gif_image_info image_info)
{
	if(pdivoom_disp_alarm_handle)
	{
        image_info->image_id = pdivoom_disp_alarm_handle->image_id;
        image_info->model_id = DIVOOM_MANAGE_IMAGE_ALARM_GIF;
	}
    else
    {
        image_info->image_id = 0;
    }
}


void divoom_disp_alarm_update_disp(Pdivoom_disp_ctrl_alarm_info alarm_info)
{
	if(pdivoom_disp_alarm_handle)
	{
//		uint8 mode;
		uint32 pic_len;
		uint8 *pic_raw_data;
//		mode = alarm_info->mode;
//		image_id = alarm_info->image_id + DIVOOM_MANAGE_IMAGE_ID_BASE_ALARM_GIF;
		pdivoom_disp_alarm_handle->image_id = alarm_info->image_id;

        divoom_disp_alarm_play(AK_FALSE);

		if(pdivoom_disp_alarm_handle->handle)
		{
			pdivoom_disp_alarm_handle->handle = 0;
			divoom_play_gif_com_exit_image();
		}		
        if(pdivoom_disp_alarm_handle->decode_handle)
        {
            divoom_image_decode_destoy(pdivoom_disp_alarm_handle->decode_handle);
            pdivoom_disp_alarm_handle->decode_handle = NULL;
        }

		if(alarm_info == NULL || alarm_info->status != 0)
		{
			pdivoom_disp_alarm_handle->disp_mode = DIVOOM_DISP_ALARM_DISP_SNOOZE_MODE;

		}
		else
		{
			pdivoom_disp_alarm_handle->image_id = alarm_info->image_id;			
			pdivoom_disp_alarm_handle->handle = divoom_manage_image_check_exist(DIVOOM_MANAGE_IMAGE_ALARM_GIF,pdivoom_disp_alarm_handle->image_id);
			if(pdivoom_disp_alarm_handle->handle)
			{
				pdivoom_disp_alarm_handle->disp_mode = DIVOOM_DISP_ALARM_DISP_GIF_CTRL_MODE;
				divoom_play_gif_com_init(0,0, -1);
				divoom_play_gif_com_create_image_area(0, 0, 0, SCREEN_WIDTH_64, 0,divoom_disp_alarm_gifctrl_callback, NULL);
			}else
			{
				pic_raw_data = divoom_disp_picture_get_raw_data(DIVOOM_DISP_NEW_ALARM, &pic_len);
				if (pic_raw_data != NULL && pic_len > 0) {
					pdivoom_disp_alarm_handle->decode_handle = divoom_image_decode_creat_handle();
					divoom_image_decode_add_data(pdivoom_disp_alarm_handle->decode_handle, pic_raw_data, pic_len, AK_FALSE);
					divoom_com_update_task(pdivoom_disp_alarm_handle->task_id, pdivoom_disp_alarm_handle->refresh_time);
					divoom_screen_disp_divoom_data(pdivoom_disp_alarm_handle->pic_buf);
					pdivoom_disp_alarm_handle->disp_mode = DIVOOM_DISP_ALARM_DISP_DEFAULT_GIF_MODE;
				}
			}
		}  
		divoom_disp_alarm_play(AK_TRUE);
    }
}



void divoom_disp_alarm_init(Pdivoom_disp_ctrl_alarm_info alarm_info)
{

	divoom_malloc_check_all_ram_in();
    if(pdivoom_disp_alarm_handle)
    {
        return;
    }
    pdivoom_disp_alarm_handle = divoom_com_malloc(sizeof(divoom_disp_alarm_ctrl), __func__, __LINE__);
    if(pdivoom_disp_alarm_handle == NULL)
    {
        divoom_print("%s:%d: it fails to malloc !\n",__FILE__, __LINE__);
        return ;
    }
    memset(pdivoom_disp_alarm_handle, 0, sizeof(divoom_disp_alarm_ctrl));
    pdivoom_disp_alarm_handle->task_id = 0xFF;
	pdivoom_disp_alarm_handle->refresh_time = DIVOOM_DISP_ALARM_REFRESH_TICK;
    divoom_disp_alarm_update_disp(alarm_info);
    //divoom_disp_alarm_main( NULL);
    pdivoom_disp_alarm_handle->task_id = divoom_com_create_task(divoom_disp_alarm_main,pdivoom_disp_alarm_handle->refresh_time);
    divoom_com_resume_task(pdivoom_disp_alarm_handle->task_id);
}


void divoom_disp_alarm_play(uint8 status)
{
    if(pdivoom_disp_alarm_handle && pdivoom_disp_alarm_handle->task_id != 0xFF)
    {
        if(status)
        {
            divoom_com_resume_task(pdivoom_disp_alarm_handle->task_id);
        }
        else
        {
            divoom_com_pause_task(pdivoom_disp_alarm_handle->task_id);
        }
    }
}

void divoom_disp_alarm_close(void)
{
    
	if(pdivoom_disp_alarm_handle != NULL)
	{
        if(pdivoom_disp_alarm_handle->task_id != 0xFF)
        {
    		divoom_com_pause_task(pdivoom_disp_alarm_handle->task_id);
    		divoom_com_destroy_task(pdivoom_disp_alarm_handle->task_id);
        }
		if(pdivoom_disp_alarm_handle->handle)
		{
			pdivoom_disp_alarm_handle->handle = 0;
			divoom_play_gif_com_exit_image();
		}
        if(pdivoom_disp_alarm_handle->decode_handle)
        {
            divoom_image_decode_destoy(pdivoom_disp_alarm_handle->decode_handle);
        }

		divoom_com_free(pdivoom_disp_alarm_handle, __func__, __LINE__);
		pdivoom_disp_alarm_handle = NULL;
	}
	divoom_malloc_check_all_ram_out(__func__);
}

#endif


#include "divoom_disp_com.h"

#define DIVOOM_DISP_DRAW_REFRESH_TIME 7
typedef struct _divoom_disp_draw_ctrl
{    
    uint32 task_id;
    uint8 update_flag;
	uint8 pic_type;	// 1表示16*16 2表示32*32
    uint8 pic_buf[SCREEN_BUF_SIZE_64];
}divoom_disp_draw_ctrl, *Pdivoom_disp_draw_ctrl;

static Pdivoom_disp_draw_ctrl pdivoom_disp_draw_handle = NULL;

void divoom_disp_draw_update_drawing_pad(Pdivoom_screen_color_info color, uint8 num, uint16 *off_table)
{    
    if(pdivoom_disp_draw_handle)
    {
        uint8 i;

        divoom_com_pause_task(pdivoom_disp_draw_handle->task_id);
        for(i = 0; i < num; i++)
        {
            divoom_disp_ctrl_set_data(off_table[i],color,pdivoom_disp_draw_handle->pic_buf);
        }
        pdivoom_disp_draw_handle->update_flag = 1;
        divoom_com_resume_task(pdivoom_disp_draw_handle->task_id);
    }
}

void divoom_disp_draw_clear_drawing_pad(void)
{    
    if(pdivoom_disp_draw_handle)
    {
        divoom_com_pause_task(pdivoom_disp_draw_handle->task_id);
        memset(pdivoom_disp_draw_handle->pic_buf, 0, SCREEN_BUF_SIZE_64);
        pdivoom_disp_draw_handle->update_flag = 1;
        divoom_com_resume_task(pdivoom_disp_draw_handle->task_id);
    }
}


uint8  divoom_disp_draw_check_black(void)
{
    if(pdivoom_disp_draw_handle)
    {
        uint8 i;
        divoom_screen_color_info color_info;

        for(i = 0; i < 255; i++)
        {
            divoom_disp_ctrl_get_data(i, &color_info, pdivoom_disp_draw_handle->pic_buf);
            if(color_info.r_c || color_info.g_c || color_info.b_c)
            {
                return 0;
            }
        }
    }
    else
    {
        return 0;
    }
    return 1;
}

void divoom_disp_draw_update_drawing_buf(uint8 *pic_buf, uint8 pic_type)
{    
    if(pdivoom_disp_draw_handle)
    {
        divoom_com_pause_task(pdivoom_disp_draw_handle->task_id);
		pdivoom_disp_draw_handle->pic_type = pic_type;
        memcpy(pdivoom_disp_draw_handle->pic_buf, pic_buf, SCREEN_BUF_SIZE_64);
        pdivoom_disp_draw_handle->update_flag = 1;
        divoom_com_resume_task(pdivoom_disp_draw_handle->task_id);
    }
}


void divoom_disp_draw_ctrl_main(void *pic_buf)
{
    if(pdivoom_disp_draw_handle->update_flag)
    {
    	if (pdivoom_disp_draw_handle->pic_type == 1)
        	divoom_screen_disp_rgb_data(pdivoom_disp_draw_handle->pic_buf);
//VJ & CEILING不需要也不能有大于16的图了
//VJ和CEILING需要显示大于16的图
//#if (!defined DIVOOM_DEVICE_IS_CEILING && !defined DIVOOM_DEVICE_IS_VJ)
//       	else if (pdivoom_disp_draw_handle->pic_type == 2)
//        	divoom_screen_disp_rgb_data_32(pdivoom_disp_draw_handle->pic_buf, 0);
//		else if (pdivoom_disp_draw_handle->pic_type == 3)
//			divoom_screen_disp_rgb_data_64(pdivoom_disp_draw_handle->pic_buf, 0);
//#endif
        pdivoom_disp_draw_handle->update_flag = 0;
    }
}


void divoom_disp_draw_ctrl_init(uint8 *pic_buf, uint8 pic_type)
{
    if(pdivoom_disp_draw_handle)
    {
        memcpy(pdivoom_disp_draw_handle->pic_buf, pic_buf, SCREEN_BUF_SIZE_64);
        return;
    }
    pdivoom_disp_draw_handle = divoom_system_malloc(sizeof(divoom_disp_draw_ctrl));
    memcpy(pdivoom_disp_draw_handle->pic_buf, pic_buf, SCREEN_BUF_SIZE_64);
	pdivoom_disp_draw_handle->pic_type = pic_type;
    pdivoom_disp_draw_handle->task_id = divoom_com_create_task(divoom_disp_draw_ctrl_main, DIVOOM_DISP_DRAW_REFRESH_TIME);
	if (pic_type == 1)
	    divoom_screen_disp_rgb_data(pdivoom_disp_draw_handle->pic_buf);
//VJ & CEILING不需要也不能有大于16的图了
//VJ和CEILING需要显示大于16的图
//#if (!defined DIVOOM_DEVICE_IS_CEILING && !defined DIVOOM_DEVICE_IS_VJ)
//	else if (pic_type == 2)
//		divoom_screen_disp_rgb_data_32(pdivoom_disp_draw_handle->pic_buf, 0);
//	else if (pic_type == 3)
//		divoom_screen_disp_rgb_data_64(pdivoom_disp_draw_handle->pic_buf, 0);
//#endif
    divoom_com_resume_task(pdivoom_disp_draw_handle->task_id);
}


void divoom_disp_draw_ctrl_close(void)
{

    if(pdivoom_disp_draw_handle)
    {
        divoom_com_pause_task(pdivoom_disp_draw_handle->task_id);
        divoom_com_destroy_task(pdivoom_disp_draw_handle->task_id);
        divoom_system_free(pdivoom_disp_draw_handle);
        pdivoom_disp_draw_handle = NULL;
    }

}


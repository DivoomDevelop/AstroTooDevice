#include "divoom_disp_com.h"

typedef struct _divoom_light_sand_paint_ctrl
{
    uint8 task_id;
    uint8 dots_pre_time;
    uint8 dot_all_flag;
    uint32 tol_size;
    uint32 delay_time;
    uint16 refresh_time;
    uint16 cur_dot;
    uint16 dot_num;
    uint8 off_table[SCREEN_HEIGHT_32*SCREEN_WIDTH_32];
    divoom_screen_color_info color;
    uint32 start_disp_time;
    uint32 end_disp_time;
    uint32 sep_disp_time;
    uint8 pic_buf[SCREEN_BUF_SIZE_32];
    uint8 page_buf[DIVOOM_PAGE_SIZE];
	uint8 decode_pic_buf[DIVOOM_IMAGE_ENCODE_MAX_64_SIZE];
    void *file_handle;
}divoom_light_sand_paint_ctrl, *Pdivoom_light_sand_paint_ctrl;

#define DIVOOM_LIGHT_SAND_PAINT_MAX_DELAY  2000
#define DIVOOM_LIGHT_SAND_PAINT_MIN_REFRESH_TIME 70
Pdivoom_light_sand_paint_ctrl pdivoom_light_sand_paint_handle = NULL;
void divoom_light_sand_paint_read_pic(void);

T_BOOL divoom_light_sand_paint_read_dot(void)
{
    uint8 *page_buf = pdivoom_light_sand_paint_handle->page_buf;
    uint32 read_len;
    
    
    read_len = divoom_app_file_manage_read_data(pdivoom_light_sand_paint_handle->file_handle, 2, (uint8 *)&pdivoom_light_sand_paint_handle->dot_num);
    if(read_len != 2)
    {
        return AK_FALSE;
    }
    if(pdivoom_light_sand_paint_handle->dot_num & 0x8000)
    {
        pdivoom_light_sand_paint_handle->dot_all_flag = 1;
        pdivoom_light_sand_paint_handle->dot_num = pdivoom_light_sand_paint_handle->dot_num & 0x7FFF;
    }
    else
    {
        pdivoom_light_sand_paint_handle->dot_all_flag = 0;
    }
    divoom_app_file_manage_read_data(pdivoom_light_sand_paint_handle->file_handle, 3, pdivoom_light_sand_paint_handle->page_buf);
    
    pdivoom_light_sand_paint_handle->color.r_c = page_buf[0];
    pdivoom_light_sand_paint_handle->color.g_c = page_buf[1];
    pdivoom_light_sand_paint_handle->color.b_c = page_buf[2];
    if(pdivoom_light_sand_paint_handle->dot_num == 0)
    {
        divoom_disp_ctrl_fill(pdivoom_light_sand_paint_handle->pic_buf,&pdivoom_light_sand_paint_handle->color);
    }
    else
    {
        divoom_app_file_manage_read_data(pdivoom_light_sand_paint_handle->file_handle, pdivoom_light_sand_paint_handle->dot_num, pdivoom_light_sand_paint_handle->off_table);
    }
    return AK_TRUE;
}

void divoom_light_sand_paint_main(T_pVOID CbParam)
{
    uint32 cur_time = divoom_system_get_tickms();
    
    if(pdivoom_light_sand_paint_handle->start_disp_time < cur_time)
    {
        pdivoom_light_sand_paint_handle->end_disp_time = cur_time + 5000;
        //divoom_disp_time_common_get_time_buf_ext(pdivoom_light_sand_paint_handle->decode_pic_buf, NULL, 1);
        divoom_screen_disp_rgb32_data(pdivoom_light_sand_paint_handle->decode_pic_buf);
        pdivoom_light_sand_paint_handle->start_disp_time = cur_time + pdivoom_light_sand_paint_handle->sep_disp_time;
        return;
    }
    else if(pdivoom_light_sand_paint_handle->end_disp_time > cur_time)
    {
        return;
    }
    pdivoom_light_sand_paint_handle->end_disp_time = 0;
    if(pdivoom_light_sand_paint_handle->cur_dot >= pdivoom_light_sand_paint_handle->dot_num)
    {
        if(divoom_light_sand_paint_read_dot())
        {
            pdivoom_light_sand_paint_handle->cur_dot  = 0;
        }
        #if 0
        else if(pdivoom_light_sand_paint_handle->delay_time < DIVOOM_LIGHT_SAND_PAINT_MAX_DELAY)
        {
            pdivoom_light_sand_paint_handle->delay_time += pdivoom_light_sand_paint_handle->refresh_time;
            return;
        }
        #endif
        else
        {
            divoom_light_sand_paint_read_pic();
            divoom_light_sand_paint_read_dot();
            pdivoom_light_sand_paint_handle->cur_dot  = 0;
            pdivoom_light_sand_paint_handle->delay_time = 0;
            return ;
        }
    }
    if(pdivoom_light_sand_paint_handle->dot_all_flag)
    {
        #if 0
        uint16 i;

        for(i = 0; i < pdivoom_light_sand_paint_handle->dot_num; i++)
        {
            divoom_disp_ctrl_set_data(pdivoom_light_sand_paint_handle->off_table[pdivoom_light_sand_paint_handle->cur_dot],
                &pdivoom_light_sand_paint_handle->color,pdivoom_light_sand_paint_handle->pic_buf);
            pdivoom_light_sand_paint_handle->cur_dot ++;
        }
        #endif
//        divoom_draw_update_drawing_pad(&pdivoom_light_sand_paint_handle->color, pdivoom_light_sand_paint_handle->dot_num, 
//            &pdivoom_light_sand_paint_handle->off_table[pdivoom_light_sand_paint_handle->cur_dot], pdivoom_light_sand_paint_handle->pic_buf);
        pdivoom_light_sand_paint_handle->cur_dot += pdivoom_light_sand_paint_handle->dot_num;
    }
    else if(pdivoom_light_sand_paint_handle->dot_num)
    {
        #if 0
        uint16 i;

        for(i = 0; i < pdivoom_light_sand_paint_handle->dots_pre_time; i++)
        {
            divoom_disp_ctrl_set_data(pdivoom_light_sand_paint_handle->off_table[pdivoom_light_sand_paint_handle->cur_dot],
                &pdivoom_light_sand_paint_handle->color,pdivoom_light_sand_paint_handle->pic_buf);
            pdivoom_light_sand_paint_handle->cur_dot ++;
            if(pdivoom_light_sand_paint_handle->cur_dot >= pdivoom_light_sand_paint_handle->dot_num)
            {
                break;
            }
        }
        #endif

        uint8 dot_num;
        
        if(pdivoom_light_sand_paint_handle->cur_dot + pdivoom_light_sand_paint_handle->dots_pre_time >= pdivoom_light_sand_paint_handle->dot_num)
        {
            dot_num = pdivoom_light_sand_paint_handle->dot_num - pdivoom_light_sand_paint_handle->cur_dot;
        }
        else
        {
            dot_num = pdivoom_light_sand_paint_handle->dots_pre_time;
        }            
//        divoom_draw_update_drawing_pad(&pdivoom_light_sand_paint_handle->color, dot_num, 
//            &pdivoom_light_sand_paint_handle->off_table[pdivoom_light_sand_paint_handle->cur_dot], pdivoom_light_sand_paint_handle->pic_buf);
        pdivoom_light_sand_paint_handle->cur_dot += dot_num;
    }
    divoom_screen_disp_rgb32_data(pdivoom_light_sand_paint_handle->pic_buf);
}



void divoom_light_sand_paint_read_pic(void)
{
    //uint8 *pic_data;
    uint16 pic_len;
    
    divoom_app_file_manage_seek_off(pdivoom_light_sand_paint_handle->file_handle, DIVOOM_PAGE_SIZE + 2);
    divoom_app_file_manage_read_data(pdivoom_light_sand_paint_handle->file_handle, 2, (uint8 *)&pic_len);
    divoom_app_file_manage_read_data(pdivoom_light_sand_paint_handle->file_handle, pic_len, pdivoom_light_sand_paint_handle->decode_pic_buf);
    
    divoom_image_decode_Iframe(pdivoom_light_sand_paint_handle->decode_pic_buf, pdivoom_light_sand_paint_handle->pic_buf, DIVOOM_IMAGE_WIDTH);
    //divoom_com_free(pic_data, __func__, __LINE__);
    divoom_screen_disp_rgb32_data(pdivoom_light_sand_paint_handle->pic_buf);
}

void divoom_light_sand_paint_init(uint8 type,uint32 file_index, uint32 disp_time)
{
    uint16 refresh_time;
    
    pdivoom_light_sand_paint_handle = divoom_com_malloc(sizeof(divoom_light_sand_paint_ctrl) ,__func__, __LINE__);
    if(pdivoom_light_sand_paint_handle == NULL)
    {
        return;
    }
    memset(pdivoom_light_sand_paint_handle, 0, sizeof(divoom_light_sand_paint_ctrl));
    pdivoom_light_sand_paint_handle->file_handle = divoom_app_file_manage_open_read(type, file_index, 0);
    if(pdivoom_light_sand_paint_handle->file_handle == NULL)
    {
        divoom_com_free(pdivoom_light_sand_paint_handle, __func__, __LINE__);
        return ;
    }
    divoom_app_file_manage_read_data(pdivoom_light_sand_paint_handle->file_handle, 4, (uint8 *)&pdivoom_light_sand_paint_handle->tol_size);
    divoom_app_file_manage_seek_off(pdivoom_light_sand_paint_handle->file_handle, DIVOOM_PAGE_SIZE);
    divoom_app_file_manage_read_data(pdivoom_light_sand_paint_handle->file_handle, 2, (uint8 *)&refresh_time);
    if(refresh_time < DIVOOM_LIGHT_SAND_PAINT_MIN_REFRESH_TIME)
    {
        pdivoom_light_sand_paint_handle->dots_pre_time = (DIVOOM_LIGHT_SAND_PAINT_MIN_REFRESH_TIME + refresh_time - 1)/refresh_time;
        pdivoom_light_sand_paint_handle->refresh_time = pdivoom_light_sand_paint_handle->dots_pre_time * refresh_time;
    }
    else
    {
        pdivoom_light_sand_paint_handle->refresh_time = refresh_time;
        pdivoom_light_sand_paint_handle->dots_pre_time = 1;
    }
    
    if(disp_time)
    {
        pdivoom_light_sand_paint_handle->sep_disp_time = disp_time * 1000;
        pdivoom_light_sand_paint_handle->start_disp_time = pdivoom_light_sand_paint_handle->sep_disp_time + divoom_system_get_tickms();
    }
    else
    {
        pdivoom_light_sand_paint_handle->start_disp_time = T_U32_MAX;
    }
    pdivoom_light_sand_paint_handle->end_disp_time = 0;
    
    divoom_light_sand_paint_read_pic();
    divoom_light_sand_paint_read_dot();
    pdivoom_light_sand_paint_handle->task_id = divoom_com_create_task(divoom_light_sand_paint_main, pdivoom_light_sand_paint_handle->refresh_time / 10);
    divoom_com_resume_task(pdivoom_light_sand_paint_handle->task_id);
}


void divoom_light_sand_paint_play(uint8 play)
{
    if(pdivoom_light_sand_paint_handle)
    {
        if(play)
        {
            divoom_com_resume_task(pdivoom_light_sand_paint_handle->task_id);
        }
        else
        {
            divoom_com_pause_task(pdivoom_light_sand_paint_handle->task_id);
        }
    }
}


void divoom_light_sand_paint_exit(void)
{

    if(pdivoom_light_sand_paint_handle == NULL)
    {
        return;
    }
    divoom_app_file_manage_close_read(pdivoom_light_sand_paint_handle->file_handle);
    divoom_com_pause_task(pdivoom_light_sand_paint_handle->task_id);
    divoom_com_destroy_task(pdivoom_light_sand_paint_handle->task_id);
    divoom_com_free(pdivoom_light_sand_paint_handle, __func__, __LINE__);
    pdivoom_light_sand_paint_handle = NULL;
}



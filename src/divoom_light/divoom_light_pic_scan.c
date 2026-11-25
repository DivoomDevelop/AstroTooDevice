#include "divoom_disp_com.h"

typedef struct _divoom_light_pic_scan_ctrl
{
    uint8 task_id;
    uint8 pic_id;
    uint8 step;
    uint8 play;
    uint8 image_type;
    uint8 pic_num;
    uint8 active_pic_num;
    uint8 type;
    uint32 disp_line;
    uint32 max_line;
    uint32 load_id;
    uint32 file_index;
    divoom_light_pic_scan_para parm_info;
    uint8 encode_buf[DIVOOM_IMAGE_ENCODE_MAX_64_SIZE];
    void *handle;
    uint8 cur_pic_buf[SCREEN_BUF_SIZE_32];
    uint8 cache_pic_buf[2][SCREEN_BUF_SIZE_32];
}divoom_light_pic_scan_ctrl, *Pdivoom_light_pic_scan_ctrl;

#define DIVOOM_LIGHT_PIC_SCAN_MIN_TIME 100

Pdivoom_light_pic_scan_ctrl pdivoom_light_pic_scan_handle = NULL;

void divoom_light_pic_read_pic(uint8 id, uint8 *pic_buf);

T_BOOL divoom_light_pic_scan_check_row(uint8 cur_row, uint8 *pic_buf)
{
    uint16 i, off;
    divoom_screen_color_info color = {0, 0, 0};

    off = cur_row;

    for(i = 0; i < SCREEN_HEIGHT_32; i ++)
    {
        divoom_disp_ctrl_get_data(off, &color, pic_buf);
        if(color.r_c || color.g_c || color.b_c )
        {
            return AK_FALSE;
        }
        off += SCREEN_WIDTH_32;
    }
    return AK_TRUE;
}

T_BOOL divoom_light_pic_scan_check_line(uint8 cur_line, uint8 *pic_buf)
{
    uint16 i, off;
    divoom_screen_color_info color = {0, 0, 0};

    off = cur_line * SCREEN_WIDTH_32;

    for(i = 0; i < SCREEN_WIDTH_32; i ++)
    {
        divoom_disp_ctrl_get_data(off, &color, pic_buf);
        if(color.r_c || color.g_c || color.b_c )
        {
            return AK_FALSE;
        }
        off ++;
    }
    return AK_TRUE;
}

void divoom_light_pic_scan_init_max_line(void)
{
    uint16 i, j = SCREEN_HEIGHT_32;


    pdivoom_light_pic_scan_handle->max_line = SCREEN_HEIGHT_32 * pdivoom_light_pic_scan_handle->pic_num;    
    if(pdivoom_light_pic_scan_handle->parm_info.mode == DIVOOM_LIGHT_PIC_SCAN_MODE_1_4)
    {
        for(i = pdivoom_light_pic_scan_handle->pic_num - 1; i > 0; i --)
        {
            divoom_light_pic_read_pic(i, pdivoom_light_pic_scan_handle->cur_pic_buf);
            for(j = SCREEN_HEIGHT_32; j > 0; j --)
            {
                if(divoom_light_pic_scan_check_row(j - 1, pdivoom_light_pic_scan_handle->cur_pic_buf) == AK_FALSE)
                {
                    break;
                }
            }
            if(j != 0)
            {
                break;
            }
        }
        pdivoom_light_pic_scan_handle->max_line = (i + 1) * SCREEN_WIDTH_32;
        pdivoom_light_pic_scan_handle->active_pic_num = i + 1;
    }
    else if(pdivoom_light_pic_scan_handle->parm_info.mode == DIVOOM_LIGHT_PIC_SCAN_MODE_4_1)
    {
        for(i = pdivoom_light_pic_scan_handle->pic_num - 1; i > 0; i --)
        {
            divoom_light_pic_read_pic(i, pdivoom_light_pic_scan_handle->cur_pic_buf);
            for(j = SCREEN_HEIGHT_32; j > 0; j --)
            {
                if(divoom_light_pic_scan_check_line(j - 1, pdivoom_light_pic_scan_handle->cur_pic_buf) == AK_FALSE)
                {
                    break;
                }
            }
            if(j != 0)
            {
                break;
            }
        }
        pdivoom_light_pic_scan_handle->max_line = (i + 1) * SCREEN_WIDTH_32;
        pdivoom_light_pic_scan_handle->active_pic_num = i + 1;
    }
    else if(pdivoom_light_pic_scan_handle->parm_info.mode == DIVOOM_LIGHT_PIC_SCAN_MODE_2_2)
    {
        pdivoom_light_pic_scan_handle->max_line = pdivoom_light_pic_scan_handle->pic_num * SCREEN_WIDTH_32;
        pdivoom_light_pic_scan_handle->active_pic_num = pdivoom_light_pic_scan_handle->pic_num;
    }
}


void divoom_light_pic_scan_write_row(uint8 cur_row, uint8 cache_row)
{
    uint16 i, cur_base_off, cache_base_off, pic_id, pic_row;
    divoom_screen_color_info color = {0, 0, 0};

   
    pic_id = cache_row / SCREEN_WIDTH_32;
    pic_row = cache_row % SCREEN_WIDTH_32;
    cur_base_off = cur_row;
    cache_base_off = pic_row;
   // divoom_print("<%d, %d, %d>",cur_row, cache_row, pic_id);
    for(i = 0; i < SCREEN_HEIGHT_32; i ++)
    {
        divoom_disp_ctrl_get_data(cache_base_off, &color,pdivoom_light_pic_scan_handle->cache_pic_buf[pic_id]);
        divoom_disp_ctrl_set_data(cur_base_off, &color, pdivoom_light_pic_scan_handle->cur_pic_buf);
        cache_base_off += SCREEN_WIDTH_32;
        cur_base_off += SCREEN_WIDTH_32;
    }
}

void divoom_light_pic_scan_write_row_rev(uint8 cur_row, uint8 cache_row)
{
    uint16 i, cur_base_off, cache_base_off, pic_id, pic_row;
    divoom_screen_color_info color = {0, 0, 0};

   
    pic_id = cache_row / SCREEN_WIDTH_32;
    pic_row = cache_row % SCREEN_WIDTH_32;
    cur_base_off = SCREEN_WIDTH_32 - cur_row - 1;
    cache_base_off = SCREEN_WIDTH_32 - pic_row - 1;
   // divoom_print("<%d, %d, %d>",cur_row, cache_row, pic_id);
    for(i = 0; i < SCREEN_HEIGHT_32; i ++)
    {
        divoom_disp_ctrl_get_data(cache_base_off, &color,pdivoom_light_pic_scan_handle->cache_pic_buf[pic_id] );
        divoom_disp_ctrl_set_data(cur_base_off, &color, pdivoom_light_pic_scan_handle->cur_pic_buf);
        cache_base_off += SCREEN_WIDTH_32;
        cur_base_off += SCREEN_WIDTH_32;
    }
}

void divoom_light_pic_scan_write_line_rev(uint8 cur_line, uint8 cache_line)
{
    uint16 i, cur_base_off, cache_base_off, pic_id, pic_row;
    divoom_screen_color_info color = {0, 0, 0};

   
   // divoom_print("[%d, %d, %d]",cur_line, cache_line, pic_id);
    pic_id = cache_line / SCREEN_HEIGHT_32;
    pic_row = cache_line % SCREEN_HEIGHT_32;
    cur_base_off = (SCREEN_HEIGHT_32 - cur_line - 1) * SCREEN_WIDTH_32;
    cache_base_off = (SCREEN_HEIGHT_32 - pic_row - 1) * SCREEN_WIDTH_32;
    
    for(i = 0; i < SCREEN_WIDTH_32; i ++)
    {
        divoom_disp_ctrl_get_data(cache_base_off, &color,pdivoom_light_pic_scan_handle->cache_pic_buf[pic_id]);
        divoom_disp_ctrl_set_data(cur_base_off, &color, pdivoom_light_pic_scan_handle->cur_pic_buf);
        cache_base_off ++;
        cur_base_off ++;
    }
}

void divoom_light_pic_scan_write_line(uint8 cur_line, uint8 cache_line)
{
    uint16 i, cur_base_off, cache_base_off, pic_id, pic_row;
    divoom_screen_color_info color = {0, 0, 0};

   
   // divoom_print("[%d, %d, %d]",cur_line, cache_line, pic_id);
    pic_id = cache_line / SCREEN_HEIGHT_32;
    pic_row = cache_line % SCREEN_HEIGHT_32;
    cur_base_off = cur_line * SCREEN_WIDTH_32;
    cache_base_off = pic_row * SCREEN_WIDTH_32;
    for(i = 0; i < SCREEN_WIDTH_32; i ++)
    {
        divoom_disp_ctrl_get_data(cache_base_off,&color,pdivoom_light_pic_scan_handle->cache_pic_buf[pic_id]);
        divoom_disp_ctrl_set_data(cur_base_off, &color, pdivoom_light_pic_scan_handle->cur_pic_buf);
        cache_base_off ++;
        cur_base_off ++;
    }
}

void divoom_light_pic_read_pic(uint8 id, uint8 *pic_buf)
{
    uint16 pic_len;

    divoom_load_movie_get_pic_data(pdivoom_light_pic_scan_handle->handle, id, &pic_len, pdivoom_light_pic_scan_handle->encode_buf);
    divoom_image_decode_Iframe( pdivoom_light_pic_scan_handle->encode_buf, pic_buf,DIVOOM_IMAGE_HEIGHT);
    pdivoom_light_pic_scan_handle->image_type = divoom_image_decode_get_image_info( pdivoom_light_pic_scan_handle->encode_buf, NULL);
}


void divoom_light_pic_scan_load_pic(void)
{
    uint16 i, need_pic_id, start_line, base_line;
    
    base_line = pdivoom_light_pic_scan_handle->disp_line % pdivoom_light_pic_scan_handle->max_line;
    need_pic_id = base_line / SCREEN_HEIGHT_32;
    start_line = base_line % SCREEN_HEIGHT_32;

    if(pdivoom_light_pic_scan_handle->parm_info.mode == DIVOOM_LIGHT_PIC_SCAN_MODE_2_2)
    {
        uint8 pic_talbe[4] = {0,1,3,2};
        
        if(pdivoom_light_pic_scan_handle->pic_id == need_pic_id)
        {
            //不需要加载图片
        }
        else if((pdivoom_light_pic_scan_handle->pic_id != T_U8_MAX) && (pdivoom_light_pic_scan_handle->pic_id + 1 == need_pic_id))
        {
            //只需要加载一张图片
            memcpy(pdivoom_light_pic_scan_handle->cache_pic_buf[0], pdivoom_light_pic_scan_handle->cache_pic_buf[1], SCREEN_BUF_SIZE_32);
            divoom_light_pic_read_pic(pic_talbe[(need_pic_id + 1)%pdivoom_light_pic_scan_handle->active_pic_num], pdivoom_light_pic_scan_handle->cache_pic_buf[1]);
        }
        else
        {

            //需要加载二张图片
            divoom_light_pic_read_pic(pic_talbe[(need_pic_id)%pdivoom_light_pic_scan_handle->active_pic_num], pdivoom_light_pic_scan_handle->cache_pic_buf[0]);
            divoom_light_pic_read_pic(pic_talbe[(need_pic_id + 1)%pdivoom_light_pic_scan_handle->active_pic_num], pdivoom_light_pic_scan_handle->cache_pic_buf[1]);
        }
        pdivoom_light_pic_scan_handle->pic_id = need_pic_id;
        for(i = 0; i < SCREEN_HEIGHT_32; i ++)
        {
            if(need_pic_id == 0)
            {
                for(i = 0; i < SCREEN_HEIGHT_32; i ++)
                {
                    divoom_light_pic_scan_write_row(i, start_line + i);
                }
            }
            else if(need_pic_id == 1)
            {
                for(i = 0; i < SCREEN_WIDTH_32; i ++)
                {
                    divoom_light_pic_scan_write_line(i, start_line + i);
                }
            }
            else if(need_pic_id == 2)
            {
                for(i = 0; i < SCREEN_HEIGHT_32; i ++)
                {
                    divoom_light_pic_scan_write_row_rev(i, start_line + i);
                }
            }
            else if(need_pic_id == 3)
            {
                for(i = 0; i < SCREEN_WIDTH_32; i ++)
                {
                    divoom_light_pic_scan_write_line_rev(i, start_line + i);
                }
            }
            
        }
    }
    else
    {

        if(pdivoom_light_pic_scan_handle->pic_id == need_pic_id)
        {
            //不需要加载图片
        }
        else if((pdivoom_light_pic_scan_handle->pic_id != T_U8_MAX) && (pdivoom_light_pic_scan_handle->pic_id + 1 == need_pic_id))
        {
            //只需要加载一张图片
            memcpy(pdivoom_light_pic_scan_handle->cache_pic_buf[0], pdivoom_light_pic_scan_handle->cache_pic_buf[1], SCREEN_BUF_SIZE_32);
            divoom_light_pic_read_pic((need_pic_id + 1)%pdivoom_light_pic_scan_handle->active_pic_num, pdivoom_light_pic_scan_handle->cache_pic_buf[1]);
        }
        else
        {

            //需要加载二张图片
            divoom_light_pic_read_pic((need_pic_id)%pdivoom_light_pic_scan_handle->active_pic_num, pdivoom_light_pic_scan_handle->cache_pic_buf[0]);
            divoom_light_pic_read_pic((need_pic_id + 1)%pdivoom_light_pic_scan_handle->active_pic_num, pdivoom_light_pic_scan_handle->cache_pic_buf[1]);
        }
        pdivoom_light_pic_scan_handle->pic_id = need_pic_id;
        if(pdivoom_light_pic_scan_handle->parm_info.mode == DIVOOM_LIGHT_PIC_SCAN_MODE_1_4)
        {
            for(i = 0; i < SCREEN_HEIGHT_32; i ++)
            {
                divoom_light_pic_scan_write_row(i, start_line + i);
            }
        }
        else if(pdivoom_light_pic_scan_handle->parm_info.mode == DIVOOM_LIGHT_PIC_SCAN_MODE_4_1)
        {
            for(i = 0; i < SCREEN_WIDTH_32; i ++)
            {
                divoom_light_pic_scan_write_line(i, start_line + i);
            }
        }
    }
    pdivoom_light_pic_scan_handle->disp_line ++;
}


void divoom_light_pic_scan_main(void *para)
{
    uint8 i;

    if(pdivoom_light_pic_scan_handle->play)
    {
        for(i = 0; i < pdivoom_light_pic_scan_handle->step; i ++)
        {
            divoom_light_pic_scan_load_pic();
        }
        divoom_screen_disp_rgb32_data(pdivoom_light_pic_scan_handle->cur_pic_buf);
    }
    else
    {
        uint16 load_list[] = 
        {
            DIVOOM_DISP_LOADING_00,
            DIVOOM_DISP_LOADING_01,
            DIVOOM_DISP_LOADING_02,
            DIVOOM_DISP_LOADING_03,
            DIVOOM_DISP_LOADING_04,
            DIVOOM_DISP_LOADING_05,
            DIVOOM_DISP_LOADING_06,
            DIVOOM_DISP_LOADING_07,
        };
        divoom_disp_picture_display(load_list[pdivoom_light_pic_scan_handle->load_id%(sizeof(load_list)/sizeof(load_list[0]))]);
        pdivoom_light_pic_scan_handle->load_id ++;
    }
}


void divoom_light_pic_scan_init(Pdivoom_light_pic_scan_para para)
{
	divoom_malloc_check_all_ram_in();
    if(pdivoom_light_pic_scan_handle)
    {
        return ;
    }
    pdivoom_light_pic_scan_handle = (Pdivoom_light_pic_scan_ctrl)divoom_com_malloc(sizeof(divoom_light_pic_scan_ctrl) ,__func__, __LINE__);
    if(pdivoom_light_pic_scan_handle == NULL)
    {
        return;

    }
    memset(pdivoom_light_pic_scan_handle, 0, sizeof(divoom_light_pic_scan_ctrl));  
    memcpy(&pdivoom_light_pic_scan_handle->parm_info, para, sizeof(divoom_light_pic_scan_para));
    pdivoom_light_pic_scan_handle->pic_id = T_U8_MAX;
    pdivoom_light_pic_scan_handle->type = para->type;
    pdivoom_light_pic_scan_handle->file_index = para->file_index;
    pdivoom_light_pic_scan_handle->handle = divoom_load_movie_open_read(para->type, para->file_index, NULL, T_U32_MAX);
    pdivoom_light_pic_scan_handle->play = 1;
    if(pdivoom_light_pic_scan_handle->handle)
    {
        pdivoom_light_pic_scan_handle->pic_num = divoom_load_movie_get_pic_num(pdivoom_light_pic_scan_handle->handle);
        divoom_light_pic_scan_init_max_line();
        if(pdivoom_light_pic_scan_handle->parm_info.speed < DIVOOM_LIGHT_PIC_SCAN_MIN_TIME)
        {
            pdivoom_light_pic_scan_handle->step = (DIVOOM_LIGHT_PIC_SCAN_MIN_TIME + pdivoom_light_pic_scan_handle->parm_info.speed - 1)/pdivoom_light_pic_scan_handle->parm_info.speed;
            pdivoom_light_pic_scan_handle->parm_info.speed = pdivoom_light_pic_scan_handle->parm_info.speed * pdivoom_light_pic_scan_handle->step;
        }
        else
        {
            pdivoom_light_pic_scan_handle->step = 1;
        }
        if(pdivoom_light_pic_scan_handle->image_type == DIVOOM_IMAGE_SMALL_HEIGHT)
        {
            pdivoom_light_pic_scan_handle->parm_info.speed = pdivoom_light_pic_scan_handle->parm_info.speed /2 ;
        }
        pdivoom_light_pic_scan_handle->task_id = divoom_com_create_task(divoom_light_pic_scan_main, pdivoom_light_pic_scan_handle->parm_info.speed / 10);
        divoom_light_pic_scan_main(NULL);
        divoom_com_resume_task(pdivoom_light_pic_scan_handle->task_id);
    }
}



void divoom_light_pic_scan_play(uint8 play)
{
    if(pdivoom_light_pic_scan_handle == NULL)
    {
        return ;
    }
    pdivoom_light_pic_scan_handle->play = play;
    if(play)
    {
        divoom_com_update_task(pdivoom_light_pic_scan_handle->task_id, pdivoom_light_pic_scan_handle->parm_info.speed / 10);
    }
    else
    {
        pdivoom_light_pic_scan_handle->load_id = 0;
        divoom_com_update_task(pdivoom_light_pic_scan_handle->task_id, 10);
    }

    
}

void divoom_light_pic_scan_update(Pdivoom_light_pic_scan_para para)
{
    uint8 old_mode;

    if(pdivoom_light_pic_scan_handle == NULL)
    {
        return ;
    }
    divoom_com_pause_task(pdivoom_light_pic_scan_handle->task_id);
    old_mode = pdivoom_light_pic_scan_handle->parm_info.mode;
    memcpy(&pdivoom_light_pic_scan_handle->parm_info, para, sizeof(divoom_light_pic_scan_para));
    if(para->flag)
    {
        divoom_load_movie_close(pdivoom_light_pic_scan_handle->handle);
        pdivoom_light_pic_scan_handle->handle = divoom_load_movie_open_read(pdivoom_light_pic_scan_handle->type, pdivoom_light_pic_scan_handle->file_index, NULL, T_U32_MAX);
        pdivoom_light_pic_scan_handle->pic_num = divoom_load_movie_get_pic_num(pdivoom_light_pic_scan_handle->handle);
        divoom_light_pic_scan_init_max_line();
    }
    else if(old_mode != pdivoom_light_pic_scan_handle->parm_info.mode)
    {
        divoom_light_pic_scan_init_max_line();
    }
    pdivoom_light_pic_scan_handle->pic_id = T_U8_MAX;
    pdivoom_light_pic_scan_handle->disp_line = 0;
    if(pdivoom_light_pic_scan_handle->parm_info.speed < DIVOOM_LIGHT_PIC_SCAN_MIN_TIME)
    {
        pdivoom_light_pic_scan_handle->step = (DIVOOM_LIGHT_PIC_SCAN_MIN_TIME + pdivoom_light_pic_scan_handle->parm_info.speed - 1)/pdivoom_light_pic_scan_handle->parm_info.speed;
        pdivoom_light_pic_scan_handle->parm_info.speed = pdivoom_light_pic_scan_handle->parm_info.speed * pdivoom_light_pic_scan_handle->step;
    }
    else
    {
        pdivoom_light_pic_scan_handle->step = 1;
    }
    if(pdivoom_light_pic_scan_handle->image_type == DIVOOM_IMAGE_SMALL_HEIGHT)
    {
        pdivoom_light_pic_scan_handle->parm_info.speed = pdivoom_light_pic_scan_handle->parm_info.speed /2 ;
    }
    pdivoom_light_pic_scan_handle->play = 1;
    divoom_print("divoom_light_pic_scan_update: %d, %d\n", pdivoom_light_pic_scan_handle->step, pdivoom_light_pic_scan_handle->parm_info.speed);
    divoom_com_update_task(pdivoom_light_pic_scan_handle->task_id,pdivoom_light_pic_scan_handle->parm_info.speed / 10);
    divoom_com_resume_task(pdivoom_light_pic_scan_handle->task_id);
}

void divoom_light_pic_scan_close(void)
{
    if(pdivoom_light_pic_scan_handle == NULL)
    {
        return ;
    }
    
    if(pdivoom_light_pic_scan_handle->task_id != T_U8_MAX)
    {
        divoom_com_pause_task(pdivoom_light_pic_scan_handle->task_id);
        divoom_com_destroy_task(pdivoom_light_pic_scan_handle->task_id);
        divoom_load_movie_close(pdivoom_light_pic_scan_handle->handle);
    }
    divoom_com_free(pdivoom_light_pic_scan_handle, __func__, __LINE__);
    pdivoom_light_pic_scan_handle = NULL;
	divoom_malloc_check_all_ram_out(__func__);
}

void divoom_light_pic_scan_convert(uint8 type,uint32 file_index, Pdivoom_light_pic_scan_para para_info)
{
    uint8 page_buf[DIVOOM_PAGE_SIZE];
    void *file_handle;
    
    file_handle = divoom_app_file_manage_open_read(type, file_index, 0);
    if(file_handle == NULL)
    {
        return;
    }
    divoom_app_file_manage_read_data(file_handle, DIVOOM_PAGE_SIZE, page_buf);
    divoom_app_file_manage_close_read(file_handle);
    para_info->mode = page_buf[7];
    para_info->speed = page_buf[8] + ((uint16)page_buf[9] << 8);
    divoom_print("divoom_light_pic_scan_convert: %d, %d\n", para_info->mode, para_info->speed);
    
}


#include "divoom_disp_com.h"

#if 0
//#define DIVOOM_DISP_WORD_NO_SCROLL 
#define DIVOOM_DISP_WORD_REFRESH_TIME    1
#define DIVOOM_DISP_WORD_REFRESH_MS    (DIVOOM_DISP_WORD_REFRESH_TIME*10)
#define DIVOOM_DISP_WORD_SLIP_DOT       6
#define DIVOOM_DISP_WORD_SPACE_DOT      4
#define DIVOOM_DISP_WORD_START_Y      8
#define DIVOOM_DISP_WORD_START_X      0
typedef struct _divoom_disp_word_ctrl
{
    uint8 run_flag;
    uint8 exit_flag;
    uint8 no_scroll;
    uint8 device_id;
    uint8 device_num;
    uint8 back_mode;
    uint8 pass_led_time;
    uint32 cur_tick;
    uint32 speed;
    uint8 space_step;
    uint8 refresh_flag;
    uint8 base_pic_flag;
    uint8 end_tick;
    uint8 word_num;      //总共要显示多少个字
    uint8 word_id;       //正在显示的字的下标
    uint8 col_id;        //显示的字的第几行
    uint16 font_width;
    uint16 pic_len;
    uint32 cyc_flag;
    uint32 run_tick;
    uint32 pass_tick;
    uint32 task_id;
    void * parm;
    uint16 font_str[DIVOOM_DISP_WORD_MAX_NUM];
    divoom_light_word_disp_info font_info[DIVOOM_DISP_WORD_MAX_NUM];
    uint16 cur_screen_info[SCREEN_WIDTH_32];
    divoom_light_color_info back_color;
    divoom_light_color_info dis_color;
    uint8 disp_buf[SCREEN_BUF_SIZE_32];
    uint8 base_disp_buf[SCREEN_BUF_SIZE_32];
    DIVOOM_LIGHT_WORD_SEND_PIC *psend_call;
    DIVOOM_DISP_CTRL_OVER_CALL_END *pcallback;
}divoom_light_word_ctrl, *Pdivoom_light_word_ctrl;


Pdivoom_light_word_ctrl pdivoom_light_word_handle = NULL;




uint16 divoom_light_word_get_new_col_info_right_left(uint8 index, uint8 col_id)
{
    uint16 font_col = 0;
    
    if(index >= pdivoom_light_word_handle->word_num)
    {
        return 0;
    }
    font_col = pdivoom_light_word_handle->font_info[index].font_info[col_id];
    return font_col;
}

uint16 divoom_light_word_get_all_word_width(void)
{
    uint16 i, j, font_col = 0, new_col, font_data;

    pdivoom_light_word_handle->no_scroll = 0;
    for(j = 0; j < pdivoom_light_word_handle->word_num; j ++)
    {
        for(i = 0, new_col = 0; i < SCREEN_WIDTH_32; i++)
        {
            font_data = divoom_light_word_get_new_col_info_right_left(j, i);
            if(font_data != 0)
            {
                font_col ++;
                new_col = 1;
            }
        }
        if(new_col && j != pdivoom_light_word_handle->word_num - 1)
        {
            font_col ++;
        }
    }
    #ifdef DIVOOM_DISP_WORD_NO_SCROLL
    if(font_col < SCREEN_WIDTH_32 * pdivoom_light_word_handle->device_num)
    {
        pdivoom_light_word_handle->no_scroll = 1;
    }
    #endif
    pdivoom_light_word_handle->font_width = font_col;
    return font_col;
}

void divoom_light_word_get_all_word_info(void)
{
    uint16 i, j, font_col = 0, new_col, cur_col, offset = 0;

    if((pdivoom_light_word_handle->device_id == 0) && (pdivoom_light_word_handle->font_width != SCREEN_WIDTH_32 * pdivoom_light_word_handle->device_num))
    {
        offset = 1;
    }
    if(pdivoom_light_word_handle->device_num == 1 && pdivoom_light_word_handle->font_width < SCREEN_WIDTH_32)
    {
        offset = (SCREEN_WIDTH_32 - pdivoom_light_word_handle->font_width + 1) / 2;
    }
    memset(&pdivoom_light_word_handle->cur_screen_info, 0, sizeof(pdivoom_light_word_handle->cur_screen_info));
    for(j = 0; j < pdivoom_light_word_handle->word_num; j ++)
    {
        for(i = 0, new_col = 0; i < SCREEN_WIDTH_32; i++)
        {
            cur_col = divoom_light_word_get_new_col_info_right_left(j, i);
            if(cur_col != 0)
            {
                font_col ++;
                if(font_col >= pdivoom_light_word_handle->device_id * SCREEN_WIDTH_32)
                {
                    pdivoom_light_word_handle->cur_screen_info[offset] = cur_col;
                    offset ++;
                    if(offset >= SCREEN_WIDTH_32)
                    {
                        return;
                    }
                }
                new_col = 1;
            }
        }
        if(new_col && j != pdivoom_light_word_handle->word_num - 1)
        {
            font_col ++;
            if(font_col >= pdivoom_light_word_handle->device_id * SCREEN_WIDTH_32)
            {
                pdivoom_light_word_handle->cur_screen_info[offset] = 0;
                offset ++;
                if(offset >= SCREEN_WIDTH_32)
                {
                    return;
                }
            }
        }
    }

}

void divoom_light_word_update_disp_right_left(uint32 step)
{
    uint8 i, j, k;
    uint16 new_col = 0;
    divoom_screen_color_info color;

    if(pdivoom_light_word_handle->word_num == 0)
    {
        return;
    }

    #ifndef DIVOOM_DISP_WORD_NO_SCROLL
    if(pdivoom_light_word_handle->no_scroll == 1)
    {
        divoom_light_word_get_all_word_info();
    }
    else
    #endif
    {
        if((pdivoom_light_word_handle->word_id >= pdivoom_light_word_handle->word_num) && (pdivoom_light_word_handle->cyc_flag == 0))
        {
            if(pdivoom_light_word_handle->exit_flag == 0)
            {
                //divoom_disp_ctrl_exit_mode();
                pdivoom_light_word_handle->pcallback(0);
                pdivoom_light_word_handle->exit_flag = 1;
            }
            return ;
        }
        for(k = 0; k < step; k ++)
        {
            if(pdivoom_light_word_handle->word_id >= pdivoom_light_word_handle->word_num && pdivoom_light_word_handle->cyc_flag)
            {
                pdivoom_light_word_handle->end_tick ++;
                if(pdivoom_light_word_handle->end_tick >= DIVOOM_DISP_WORD_SLIP_DOT)
                {
                    pdivoom_light_word_handle->word_id = 0;
                    pdivoom_light_word_handle->col_id = 0;
                    if(pdivoom_light_word_handle->cyc_flag != T_U32_MAX)
                    {
                        pdivoom_light_word_handle->cyc_flag --;
                    }
                }
                new_col = 0;
            }
            else
            {
                if(pdivoom_light_word_handle->space_step)
                {
                    new_col = 0;
                    pdivoom_light_word_handle->space_step --;
                }
                else
                {
                    for(i = pdivoom_light_word_handle->col_id; i < SCREEN_HEIGHT_16; i++)
                    {
                        new_col = divoom_light_word_get_new_col_info_right_left(pdivoom_light_word_handle->word_id, i);
                        if(new_col != 0)
                        {
                            break;
                        }
                    }
                    if(pdivoom_light_word_handle->col_id == 0 && i == SCREEN_HEIGHT_16)
                    {
                        pdivoom_light_word_handle->space_step = DIVOOM_DISP_WORD_SPACE_DOT;
                    }
                    pdivoom_light_word_handle->col_id = i + 1;
                    if(i == SCREEN_HEIGHT_16)
                    {
                        pdivoom_light_word_handle->word_id ++;
                        pdivoom_light_word_handle->col_id = 0;
                        if(pdivoom_light_word_handle->word_id >= pdivoom_light_word_handle->word_num && pdivoom_light_word_handle->cyc_flag)
                        {
                            pdivoom_light_word_handle->end_tick = 1;
                        }
                    }
                }
            }
            memmove(&pdivoom_light_word_handle->cur_screen_info[0], &pdivoom_light_word_handle->cur_screen_info[1], (SCREEN_WIDTH_32 - 1) * sizeof(uint16));
            pdivoom_light_word_handle->cur_screen_info[SCREEN_WIDTH_32 - 1] = new_col;
        }

    }
    memcpy( pdivoom_light_word_handle->base_disp_buf, pdivoom_light_word_handle->disp_buf, SCREEN_BUF_SIZE_32);
    for(i = 0; i < SCREEN_HEIGHT_32; i++)
    {
        for(j = 0; j < SCREEN_WIDTH_16; j++)
        {
            if(pdivoom_light_word_handle->base_pic_flag)
            {
                if(pdivoom_light_word_handle->cur_screen_info[i] & ( 1 << j))
                {
                    divoom_disp_ctrl_get_dot(i + DIVOOM_DISP_WORD_START_X, j + DIVOOM_DISP_WORD_START_Y, &color, pdivoom_light_word_handle->disp_buf);
                    color.r_c = ~(color.r_c);
                    color.g_c = ~(color.g_c);
                    color.b_c = ~(color.b_c);
                    divoom_disp_ctrl_set_dot(i + DIVOOM_DISP_WORD_START_X, j + DIVOOM_DISP_WORD_START_Y, &color, pdivoom_light_word_handle->base_disp_buf);
                }
            }
            else
            {
                if(pdivoom_light_word_handle->cur_screen_info[i] & ( 1 << j))
                {
                    divoom_disp_ctrl_set_dot(i + DIVOOM_DISP_WORD_START_X, j + DIVOOM_DISP_WORD_START_Y, &pdivoom_light_word_handle->dis_color, pdivoom_light_word_handle->disp_buf);
                }
                else
                {
                    divoom_disp_ctrl_set_dot(i + DIVOOM_DISP_WORD_START_X, j + DIVOOM_DISP_WORD_START_Y, &pdivoom_light_word_handle->back_color, pdivoom_light_word_handle->disp_buf);
                }
            }
        }
    }
}

void divoom_light_word_main(void *para)
{
    uint32 cnt;
    
    if(!(pdivoom_light_word_handle->base_pic_flag == 1 || pdivoom_light_word_handle->back_color.r_c!= 0 || 
        pdivoom_light_word_handle->back_color.g_c!= 0 ||
        pdivoom_light_word_handle->back_color.b_c!= 0 ||
        pdivoom_light_word_handle->dis_color.r_c!= 0 ||
        pdivoom_light_word_handle->dis_color.g_c!= 0 ||
        pdivoom_light_word_handle->dis_color.b_c!= 0 ))
    {
        return;
    }
    pdivoom_light_word_handle->run_tick ++;
    if(pdivoom_light_word_handle->cur_tick == 0)
    {
        pdivoom_light_word_handle->cur_tick = divoom_system_get_tickms() + DIVOOM_DISP_WORD_REFRESH_MS * pdivoom_light_word_handle->speed;
    }
    
    if(pdivoom_light_word_handle->word_num)
    {
        if((pdivoom_light_word_handle->cur_tick <= divoom_system_get_tickms()))
        {    
            //文字在动的情况
            cnt = (divoom_system_get_tickms() - pdivoom_light_word_handle->cur_tick) / (DIVOOM_DISP_WORD_REFRESH_MS * pdivoom_light_word_handle->speed);
            cnt ++;
            pdivoom_light_word_handle->refresh_flag= 0;
            pdivoom_light_word_handle->cur_tick += cnt *DIVOOM_DISP_WORD_REFRESH_MS * pdivoom_light_word_handle->speed;
            memcpy(pdivoom_light_word_handle->base_disp_buf, pdivoom_light_word_handle->disp_buf, SCREEN_BUF_SIZE_32);
            divoom_light_word_update_disp_right_left(cnt);
            if(pdivoom_light_word_handle->psend_call)
            {
                pdivoom_light_word_handle->psend_call(pdivoom_light_word_handle->parm, pdivoom_light_word_handle->base_disp_buf, SCREEN_BUF_SIZE_32);
            }
            else
            {
                    divoom_screen_disp_rgb32_data(pdivoom_light_word_handle->base_disp_buf);
            }
        }
        else if(pdivoom_light_word_handle->refresh_flag > 0)
        {
            //只更新图片，文字没有动
            uint16 i, j;
            divoom_screen_color_info color;
            
            pdivoom_light_word_handle->refresh_flag = 0;
            memcpy(pdivoom_light_word_handle->base_disp_buf, pdivoom_light_word_handle->disp_buf, SCREEN_BUF_SIZE_32);
            for(i = 0; i < SCREEN_WIDTH_32; i++)
            {
                for(j = 0; j < SCREEN_WIDTH_16; j++)
                {
                    if(pdivoom_light_word_handle->cur_screen_info[i] & ( 1 << j))
                    {
                        divoom_disp_ctrl_get_dot(i + DIVOOM_DISP_WORD_START_X, j + DIVOOM_DISP_WORD_START_Y, &color, pdivoom_light_word_handle->base_disp_buf);
                        color.r_c = ~(color.r_c);
                        color.g_c = ~(color.g_c);
                        color.b_c = ~(color.b_c);
                        divoom_disp_ctrl_set_dot(i + DIVOOM_DISP_WORD_START_X, j + DIVOOM_DISP_WORD_START_Y, &color, pdivoom_light_word_handle->base_disp_buf);
                    }
                }
            }
            if(pdivoom_light_word_handle->psend_call)
            {
                pdivoom_light_word_handle->psend_call(pdivoom_light_word_handle->parm, pdivoom_light_word_handle->base_disp_buf, SCREEN_BUF_SIZE_32);
            }
            else
            {
                divoom_screen_disp_rgb32_data(pdivoom_light_word_handle->base_disp_buf);
            }
        }
    }
}


/*********************************************************************
  Function:         divoom_light_word_init
  Description:     开始文字显示
   Input:            data   显示相关数据
  Author:           divoom
**********************************************************************/

void *divoom_light_word_init(void *data, uint8 mode)
{
	divoom_malloc_check_all_ram_in();
    if(pdivoom_light_word_handle)
    {
        divoom_light_word_update(data);
        return pdivoom_light_word_handle;
    }

    pdivoom_light_word_handle = divoom_com_malloc(sizeof(divoom_light_word_ctrl) ,__func__, __LINE__);
    
    if(pdivoom_light_word_handle == NULL)
    {
        divoom_print("%s:%d: it fails to malloc!\n",__FILE__, __LINE__);
        return pdivoom_light_word_handle; 
    }
    memset(pdivoom_light_word_handle, 0, sizeof(divoom_light_word_ctrl));
    pdivoom_light_word_handle->device_id = 0;
    pdivoom_light_word_handle->device_num = 1;
    pdivoom_light_word_handle->cyc_flag = T_U32_MAX;
    pdivoom_light_word_handle->back_mode = mode;
    pdivoom_light_word_handle->task_id = divoom_com_create_task(divoom_light_word_main, DIVOOM_DISP_WORD_REFRESH_TIME);
    if(data)
    {
        pdivoom_light_word_handle->base_pic_flag = 1;
        divoom_light_word_update(data);
    }
    else
    {
        pdivoom_light_word_handle->word_num = 0;  
        pdivoom_light_word_handle->base_pic_flag = 0;
        divoom_com_resume_task(pdivoom_light_word_handle->task_id);
    }
    return pdivoom_light_word_handle; 
}


uint8 divoom_light_word_get_mul_run_flag(void)
{
    if(pdivoom_light_word_handle)
    {
    }
    return AK_FALSE;
}


/*********************************************************************
  Function:         divoom_light_word_set_mul_flag
  Description:     设置多屏的标记
   Input:            data  0:pause;1:main;2:sub
  Author:           divoom
**********************************************************************/

void divoom_light_word_set_mul_flag(uint8 data)
{
    #if 0
    if(pdivoom_light_word_handle == NULL)
    {
        return;
    }
    if(data == 0)
    {
        divoom_com_pause_task(pdivoom_light_word_handle->task_id);
        pdivoom_light_word_handle->run_flag = 0;
        pdivoom_light_word_handle->run_tick = 0;
    }
    else
    {
        uint16 id = divoom_mul_pic_get_id();
        if(data == 1 && divoom_nul_pic_word_mul_dis())
        {
            pdivoom_light_word_handle->run_flag = 1;
            #ifndef DIVOOM_DISP_WORD_NO_SCROLL        
            if(id != 0xFF && pdivoom_light_word_handle->no_scroll == 0)
            #else
            if(id != 0xFF)
            #endif
            {
                pdivoom_light_word_handle->pass_tick = divoom_system_get_tickms()+ (uint32)pdivoom_light_word_handle->speed * 160 * id * DIVOOM_DISP_WORD_REFRESH_TIME;
            }
            else
            {
                pdivoom_light_word_handle->pass_tick = 0;
            }
        }
        else
        {
            pdivoom_light_word_handle->run_flag = 1;
            pdivoom_light_word_handle->pass_tick = 0;
        }
        pdivoom_light_word_handle->cur_tick = divoom_system_get_tickms() + DIVOOM_DISP_WORD_REFRESH_MS * (uint32)pdivoom_light_word_handle->speed;
        divoom_com_resume_task(pdivoom_light_word_handle->task_id);
    }
    #endif
}


/*********************************************************************
  Function:         divoom_light_word_set_cyc_flag
  Description:     更新显示信息
   Input:            data  循环标记
  Author:           divoom
**********************************************************************/

void divoom_light_word_set_cyc_flag(uint8 data, DIVOOM_DISP_CTRL_OVER_CALL_END *pcallback)
{
    if(pdivoom_light_word_handle)
    {
        pdivoom_light_word_handle->cyc_flag = data;
        pdivoom_light_word_handle->pcallback = pcallback;
    }
    
}


/*********************************************************************
  Function:         divoom_light_word_update
  Description:     更新显示信息
   Input:            data  显示相关数据                
  Author:           divoom
**********************************************************************/

void divoom_light_word_update_all(void *data)
{
    Pdivoom_light_word_disp_font_info pword_info = (Pdivoom_light_word_disp_font_info)data;

    if(pdivoom_light_word_handle == NULL)
    {
        return;
    }
    
    divoom_com_pause_task(pdivoom_light_word_handle->task_id);
    //divoom_iclock_ctrl_set_bright(pword_info->light);
    memset(pdivoom_light_word_handle->disp_buf,0, SCREEN_BUF_SIZE_32);
    pdivoom_light_word_handle->word_num = pword_info->len;
    pdivoom_light_word_handle->word_id = 0;
    pdivoom_light_word_handle->col_id = 0;
    pdivoom_light_word_handle->exit_flag = 0;
    pdivoom_light_word_handle->base_pic_flag = 0;
	if(pword_info->speed > 100)
		pword_info->speed = 100;
	pword_info->speed = 100 - pword_info->speed;
    pdivoom_light_word_handle->speed = pword_info->speed / (DIVOOM_DISP_WORD_REFRESH_TIME * 10);
    //最小是30MS移动一格
    if(pdivoom_light_word_handle->speed < 3)
    {
        pdivoom_light_word_handle->speed = 3;
    }
    memcpy(&pdivoom_light_word_handle->back_color, &pword_info->back_color, sizeof(divoom_light_color_info));
    memcpy(&pdivoom_light_word_handle->dis_color, &pword_info->word_color, sizeof(divoom_light_color_info));
    divoom_disp_font_get_word_info(pword_info->word_info, pword_info->len, (uint8 *)pdivoom_light_word_handle->font_info, 0);
    memset(pdivoom_light_word_handle->cur_screen_info, 0, sizeof(pdivoom_light_word_handle->cur_screen_info));
    divoom_com_resume_task(pdivoom_light_word_handle->task_id);
}

void divoom_light_word_update_base_color(uint8 *buf, uint16 len, uint8 flag)
{
    if(pdivoom_light_word_handle)
    {
        divoom_com_pause_task(pdivoom_light_word_handle->task_id);
        pdivoom_light_word_handle->cyc_flag = T_U32_MAX;
        pdivoom_light_word_handle->pic_len = len;
        pdivoom_light_word_handle->refresh_flag = 1;
        pdivoom_light_word_handle->base_pic_flag = 1;
        divoom_decode_pic_get_rgb(buf, pdivoom_light_word_handle->disp_buf, NULL, DIVOOM_IMAGE_HEIGHT);
       // memcpy(pdivoom_light_word_handle->disp_buf, pdivoom_light_word_handle->base_disp_buf, SCREEN_BUF_SIZE_32);
        if(pdivoom_light_word_handle->psend_call)
        {
            pdivoom_light_word_handle->psend_call(pdivoom_light_word_handle->parm, buf, len);
        }
        else if(pdivoom_light_word_handle->word_num == 0)
        {
            divoom_screen_disp_rgb32_data(pdivoom_light_word_handle->disp_buf);
        }     
        divoom_com_resume_task(pdivoom_light_word_handle->task_id);
    }
}

void divoom_light_word_update_word(uint16 len, uint16 *word_info, uint8 flag)
{
    if(pdivoom_light_word_handle)
    {
        if(pdivoom_light_word_handle->word_num >= len &&
            memcmp(pdivoom_light_word_handle->font_str, word_info, len*2) == 0)
        {
            pdivoom_light_word_handle->word_num = len;
            return;
        }
        divoom_com_pause_task(pdivoom_light_word_handle->task_id);
        if(len > DIVOOM_DISP_WORD_MAX_NUM)
        {
            len = DIVOOM_DISP_WORD_MAX_NUM;
        }
        if(pdivoom_light_word_handle->word_num!= 0 && pdivoom_light_word_handle->word_num < len)
        {
            if(memcmp(pdivoom_light_word_handle->font_str, word_info, pdivoom_light_word_handle->word_num*2) == 0)
            {
                memcpy(pdivoom_light_word_handle->font_str, word_info, len*2);
                pdivoom_light_word_handle->word_num = len;
                divoom_disp_font_get_word_info(word_info, len, (uint8 *)pdivoom_light_word_handle->font_info, flag);
                divoom_com_resume_task(pdivoom_light_word_handle->task_id);
                return;
            }
        }
        
        memcpy(pdivoom_light_word_handle->font_str, word_info, len*2);
        pdivoom_light_word_handle->word_num = len;
        pdivoom_light_word_handle->word_id = 0;
        pdivoom_light_word_handle->col_id = 0;
        pdivoom_light_word_handle->exit_flag = 0;
        pdivoom_light_word_handle->cyc_flag = T_U32_MAX;
        if(pdivoom_light_word_handle->speed == 0)
        {
            divoom_light_word_update_speed(15);
        }
        memset(pdivoom_light_word_handle->cur_screen_info, 0, sizeof(pdivoom_light_word_handle->cur_screen_info));
        divoom_disp_font_get_word_info(word_info, len, (uint8 *)pdivoom_light_word_handle->font_info, flag);
        #if 0
        if(divoom_nul_pic_word_mul_dis())
        {
            pdivoom_light_word_handle->device_num = divoom_mul_pic_get_device_info(&pdivoom_light_word_handle->device_id);
        }
        else
        #endif
        {
            pdivoom_light_word_handle->device_id = 0;
            pdivoom_light_word_handle->device_num = 1;
            
        }
        divoom_light_word_get_all_word_width();   
        divoom_com_resume_task(pdivoom_light_word_handle->task_id);
    }
}

void divoom_light_word_update_speed(uint16 speed)
{
    if(pdivoom_light_word_handle)
    {
        if(speed == 0)
        {
            speed = 1;
        }
        pdivoom_light_word_handle->cyc_flag = T_U32_MAX;
        pdivoom_light_word_handle->speed = (100 + speed - 1) / speed;
    }
}


void divoom_light_word_update(Pdivoom_disp_ctrl_movie_ctrl data)
{
    if(pdivoom_light_word_handle == NULL)
    {
        return;
    }
    switch(data->attr_flag)
    {
        case DIVOOM_DISP_WORD_UPDATE_ALL:
            divoom_light_word_update_all(data->attr_data);
            break;
            
        case DIVOOM_DISP_WORD_FONT_INFO:
        {
            Pdivoom_light_word_word_info word_info = (Pdivoom_light_word_word_info)data->attr_data;

            divoom_light_word_update_word(word_info->len, word_info->word_info, word_info->flag);
        }
            break;
            
        case DIVOOM_DISP_WORD_FONT_SPEED:
            divoom_light_word_update_speed(*(uint16 *)data->attr_data);
            break;
            
        case DIVOOM_DISP_WORD_BACK_PIC:
        {
            Pdivoom_disp_ctrl_pic_info pic_info = (Pdivoom_disp_ctrl_pic_info)data->attr_data;
            
            divoom_light_word_update_base_color(pic_info->pic_data,pic_info->pic_len,pic_info->pic_type);
        }
            break;
            
    }
}


void divoom_light_word_update_callback(void *parm, DIVOOM_LIGHT_WORD_SEND_PIC *pcallback)
{
    if(pdivoom_light_word_handle)
    {
        pdivoom_light_word_handle->parm = parm;
        pdivoom_light_word_handle->psend_call = pcallback;
    }
}

uint8 divoom_light_word_get_mode(void)
{
    if(pdivoom_light_word_handle)
    {
        return pdivoom_light_word_handle->back_mode;
    }
    else
    {
        return 0;
    }
}

uint8 divoom_light_word_get_work_status(void)
{
    if(pdivoom_light_word_handle)
    {
        return pdivoom_light_word_handle->base_pic_flag;
    }
    else
    {
        return 0;
    }
}

/*********************************************************************
  Function:         divoom_light_word_ctrl_task
  Description:     控制文字的微任务
   Input:            on_off   开始还是暂停
  Author:           divoom
**********************************************************************/

void divoom_light_word_ctrl_task(uint8 on_off)
{
    if(pdivoom_light_word_handle)
    {
        if(on_off == 0)
        {
            divoom_com_pause_task(pdivoom_light_word_handle->task_id);
            pdivoom_light_word_handle->base_pic_flag = 2;
        }
        else
        {
            divoom_com_resume_task(pdivoom_light_word_handle->task_id);
        }
    }
}


/*********************************************************************
  Function:         divoom_light_word_close
  Description:     关闭显示的文字
   Input:            NONE
  Author:           divoom
**********************************************************************/

uint8 divoom_light_word_close(void)
{
    uint8 ret = 0;

    if(pdivoom_light_word_handle)
    {
        ret = pdivoom_light_word_handle->back_mode;
        divoom_com_pause_task(pdivoom_light_word_handle->task_id);
        divoom_com_destroy_task(pdivoom_light_word_handle->task_id);
        divoom_com_free(pdivoom_light_word_handle, __func__, __LINE__);
        pdivoom_light_word_handle = NULL;
    }
    
	divoom_malloc_check_all_ram_out(__func__);
    return ret;
}
#endif





#include "divoom_disp_com.h"



typedef struct _divoom_disp_light_ctrl
{
    uint8 on_off;
    uint8 tack_id;
    uint8 index;
    uint8 max;
    uint8 plant_mode;
    uint32 task_id;
    uint32 plant_index;
    uint32 disp_index;
    uint32 color_index;
    uint32 disp_tick;
    int16 rainbow_color_r_step;
    int16 rainbow_color_g_step;
    int16 rainbow_color_b_step;
    divoom_screen_color_info base_color;
    divoom_screen_color_info color;
    uint8 disp_buf[SCREEN_BUF_SIZE_32];
}divoom_disp_light_ctrl, *Pdivoom_disp_light_ctrl;

Pdivoom_disp_light_ctrl pdivoom_disp_light_handle = NULL; 

static const divoom_screen_color_info divoom_disp_light_color_table[] =
{
//    {0x0f,0x0f,0x0f},
//    {0x0f,0x03,0},
//    {0x0f,0x0d,0},
//    {0x05,0x0f,0},
//    {0,0x0f,0x0e},
//    {0, 0,0x0f},
//    {0x07,0,0x0f},
//    {0x0f,0,0x05},


    {255,255,255},  //白
    {255,144,0},    //橙
    {255,255,0},    //黄
    {0,255,0},     //绿
    {0,255,255},    //青
    {0, 0,255},     //蓝
    {168,0,255},    //紫
    {255,0,255},    //粉

};

const divoom_screen_color_info divoom_disp_light_rainbow_color_table[] =
{
    {255,0,74},
    {255,0,156},
    {247,0,255},
    {189,0,255},
    {123,0,255},
    {57,0,255},
    {0,125,255},
    {0,227,255},
    {0,255,206},
    {0,255,126},
    {123,255,0},
    {181,255,0},
    {255,255,0},
    {255,178,0},
    {255,72,0},
    {255,0,0},
};

void divoom_disp_light_get_rainbow_color_step(void)
{
    uint8 cur, next;

    cur = pdivoom_disp_light_handle->color_index %(sizeof(divoom_disp_light_rainbow_color_table) / sizeof(divoom_screen_color_info));
    next = (pdivoom_disp_light_handle->color_index + 1) %(sizeof(divoom_disp_light_rainbow_color_table) / sizeof(divoom_screen_color_info));
    pdivoom_disp_light_handle->rainbow_color_r_step = (int16)divoom_disp_light_rainbow_color_table[next].r_c - (int16)divoom_disp_light_rainbow_color_table[cur].r_c;
    pdivoom_disp_light_handle->rainbow_color_g_step = (int16)divoom_disp_light_rainbow_color_table[next].g_c - (int16)divoom_disp_light_rainbow_color_table[cur].g_c;
    pdivoom_disp_light_handle->rainbow_color_b_step = (int16)divoom_disp_light_rainbow_color_table[next].b_c - (int16)divoom_disp_light_rainbow_color_table[cur].b_c;
}

void divoom_disp_light_init_rainbow_color(void)
{
    
    pdivoom_disp_light_handle->disp_index = 0;
    pdivoom_disp_light_handle->color_index = 0;
    pdivoom_disp_light_handle->disp_tick = 0;
    memcpy(&pdivoom_disp_light_handle->color, &divoom_disp_light_rainbow_color_table[pdivoom_disp_light_handle->color_index], sizeof(divoom_screen_color_info));
    memcpy(&pdivoom_disp_light_handle->base_color, &pdivoom_disp_light_handle->color, sizeof(divoom_screen_color_info));
    divoom_disp_light_get_rainbow_color_step();
}

void divoom_disp_light_update_rainbow_color(void)
{
    pdivoom_disp_light_handle->color.r_c = pdivoom_disp_light_handle->base_color.r_c + pdivoom_disp_light_handle->rainbow_color_r_step * (int16)pdivoom_disp_light_handle->disp_index / 25;
    pdivoom_disp_light_handle->color.g_c = pdivoom_disp_light_handle->base_color.g_c + pdivoom_disp_light_handle->rainbow_color_g_step * (int16)pdivoom_disp_light_handle->disp_index / 25;
    pdivoom_disp_light_handle->color.b_c = pdivoom_disp_light_handle->base_color.b_c + pdivoom_disp_light_handle->rainbow_color_b_step * (int16)pdivoom_disp_light_handle->disp_index / 25;    
}

void divoom_disp_light_main_rainbow_color(void)
{
    pdivoom_disp_light_handle->disp_index ++;
    if(pdivoom_disp_light_handle->disp_index >= 25)
    {
        pdivoom_disp_light_handle->disp_index = 0;
        pdivoom_disp_light_handle->color_index ++;
        if(pdivoom_disp_light_handle->color_index >= sizeof(divoom_disp_light_rainbow_color_table) / sizeof(divoom_screen_color_info))
        {
            pdivoom_disp_light_handle->color_index = 0;
        }
        memcpy(&pdivoom_disp_light_handle->color, &divoom_disp_light_rainbow_color_table[pdivoom_disp_light_handle->color_index], sizeof(divoom_screen_color_info));
        memcpy(&pdivoom_disp_light_handle->base_color, &pdivoom_disp_light_handle->color, sizeof(divoom_screen_color_info));
        divoom_disp_light_get_rainbow_color_step();
    }
    divoom_disp_light_update_rainbow_color();
    divoom_disp_ctrl_fill(pdivoom_disp_light_handle->disp_buf,&pdivoom_disp_light_handle->color);
    divoom_screen_disp_rgb32_data(pdivoom_disp_light_handle->disp_buf);
}


void divoom_disp_light_main(void *data)
{
    #if 0
    if(pdivoom_disp_light_handle->plant_mode == 3)
    {
        if((pdivoom_disp_light_handle->plant_index % 5) == 0)
        {
            divoom_screen_color_info color = {0x00,0, 0xFF};
            divoom_disp_ctrl_fill(pdivoom_disp_light_handle->disp_buf, &color);
        }
        else if((pdivoom_disp_light_handle->plant_index % 5) == 1)
        {
            divoom_screen_color_info color = {0x0FF,0, 0x0};
            divoom_disp_ctrl_fill(pdivoom_disp_light_handle->disp_buf, &color);
        }
        pdivoom_disp_light_handle->plant_index ++;
        divoom_screen_disp_rgb_data(pdivoom_disp_light_handle->disp_buf);
    }
    #endif
    if(pdivoom_disp_light_handle->plant_mode == 1)
    {
        divoom_disp_light_main_rainbow_color();
    }
}

void divoom_disp_light_init(Pdivoom_disp_ctrl_light_color_info light_info)
{
//    divoom_screen_color_info color = {0};
	divoom_malloc_check_all_ram_in();
    if(pdivoom_disp_light_handle)
    {
        return;
    }
    //divoom_screen_clear(&color);
    pdivoom_disp_light_handle = divoom_com_malloc(sizeof(divoom_disp_light_ctrl), __func__, __LINE__);
    if(pdivoom_disp_light_handle == NULL)
    {
        divoom_print("%s:%d: it fails to malloc!\n",__FILE__, __LINE__);
        return ;
    }
    memset(pdivoom_disp_light_handle, 0, sizeof(divoom_disp_light_ctrl));
    pdivoom_disp_light_handle->max = sizeof(divoom_disp_light_color_table)/sizeof(divoom_screen_color_info);
    pdivoom_disp_light_handle->task_id = T_U32_MAX;
    //pdivoom_disp_light_handle->task_id = divoom_com_create_task(divoom_disp_light_main,500);
    //divoom_com_resume_task(pdivoom_disp_light_handle->task_id);
    //light_info->plant_mode = 4;
    divoom_disp_light_set_attr(light_info);
}

void divoom_disp_light_set_attr(Pdivoom_disp_ctrl_light_color_info light_info)
{
    if(pdivoom_disp_light_handle)
    {        
        divoom_screen_bright(light_info->light);
        if(light_info->plant_mode >=DIVOOM_DISP_LIGHT_END_MODE)
        {
            divoom_print("%s:plant_mode err: %d\n",__func__, light_info->plant_mode);
            light_info->plant_mode = DIVOOM_DISP_LIGHT_SUN;
            light_info->color.r_c = 0xFF;
            light_info->color.g_c = 0x50;
            light_info->color.b_c = 0;
        }
        if(light_info->plant_mode == 1)
        {
        }
        else
        {
            if(pdivoom_disp_light_handle->task_id != T_U32_MAX)
            {
                divoom_com_pause_task(pdivoom_disp_light_handle->task_id);
                divoom_com_destroy_task(pdivoom_disp_light_handle->task_id);
                pdivoom_disp_light_handle->task_id = T_U32_MAX;
            }
        }
        switch(light_info->plant_mode)
        {
            case DIVOOM_DISP_LIGHT_SUN:
                divoom_print("%s:%d, rgb: %d, %d, %d\n",__func__, __LINE__, light_info->color.r_c, light_info->color.g_c, light_info->color.b_c);
                memcpy(&pdivoom_disp_light_handle->color, &light_info->color, sizeof(divoom_screen_color_info));               
                divoom_disp_ctrl_fill(pdivoom_disp_light_handle->disp_buf, &pdivoom_disp_light_handle->color);
                divoom_screen_disp_rgb32_data(pdivoom_disp_light_handle->disp_buf);                
                break;
            case DIVOOM_DISP_LIGHT_NO_MOSQUITO:
                pdivoom_disp_light_handle->color.r_c = 0;
                pdivoom_disp_light_handle->color.g_c = 255;
                pdivoom_disp_light_handle->color.b_c = 0;                    
                divoom_disp_ctrl_fill(pdivoom_disp_light_handle->disp_buf, &pdivoom_disp_light_handle->color);
                divoom_screen_disp_rgb32_data(pdivoom_disp_light_handle->disp_buf);                 
                break;
            case DIVOOM_DISP_LIGHT_PLANT:
                {
                    uint8 i, j;
					uint16 off;
                    divoom_screen_color_info b_color = {0x00,0, 0xFF},r_color = {0xFF,0, 0x0};
                    pdivoom_disp_light_handle->plant_index = 0;
                    for(i = 0, off = 0; i < SCREEN_HEIGHT_32; i ++)
                    {
                        for(j = 0; j < SCREEN_WIDTH_32; j++, off++)
                        {
                            if((j % 5) == 0)
                            {
                                divoom_disp_ctrl_set_data(off,&b_color,pdivoom_disp_light_handle->disp_buf);
                            }
                            else
                            {
                                divoom_disp_ctrl_set_data(off,&r_color,pdivoom_disp_light_handle->disp_buf);
                            }
                        }
                    }
                    divoom_screen_disp_rgb32_data(pdivoom_disp_light_handle->disp_buf);                     
                }
                break;                
            case DIVOOM_DISP_LIGHT_SLEEP:
                pdivoom_disp_light_handle->color.r_c = 255;
                pdivoom_disp_light_handle->color.g_c = 127;
                pdivoom_disp_light_handle->color.b_c = 39;                    
                divoom_disp_ctrl_fill(pdivoom_disp_light_handle->disp_buf, &pdivoom_disp_light_handle->color);
                divoom_screen_disp_rgb32_data(pdivoom_disp_light_handle->disp_buf);                   
                break;
            case DIVOOM_DISP_LIGHT_RAINBOW:
                divoom_disp_light_init_rainbow_color();
                if(pdivoom_disp_light_handle->task_id== T_U32_MAX)
                {
                    pdivoom_disp_light_handle->task_id = divoom_com_create_task(divoom_disp_light_main,  7);
                    divoom_com_resume_task(pdivoom_disp_light_handle->task_id);
                }
                divoom_disp_light_main_rainbow_color();
                break;
            default:
                break;
        }
        pdivoom_disp_light_handle->on_off = light_info->on_off;
        pdivoom_disp_light_handle->plant_mode = light_info->plant_mode;
        if(pdivoom_disp_light_handle->on_off)
        {
            divoom_screen_switch(AK_TRUE);
        }
        else
        {
            divoom_print("%s:%d, we will close the light!\n", __func__, __LINE__);
            divoom_screen_switch(AK_FALSE);
        }
//        divoom_screen_disp_rgb_data(pdivoom_disp_light_handle->disp_buf);
    }
}

void divoom_disp_light_auto_attr(Pdivoom_disp_ctrl_light_color_info light_info, T_BOOL arrow)
{
    if(pdivoom_disp_light_handle)
    {
        pdivoom_disp_light_handle->on_off = 1;
        if(arrow)
        {
            pdivoom_disp_light_handle->index ++;
            pdivoom_disp_light_handle->plant_mode = 0;
#if 0
            if(pdivoom_disp_light_handle->index == pdivoom_disp_light_handle->max)
            {
                pdivoom_disp_light_handle->plant_mode = 1;
            }
            else
#endif
				if(pdivoom_disp_light_handle->index >= pdivoom_disp_light_handle->max)
            {
                pdivoom_disp_light_handle->index = 0;
            }
        }
        else
        {
            if(pdivoom_disp_light_handle->index > 0)
            {
                pdivoom_disp_light_handle->plant_mode = 0;
                pdivoom_disp_light_handle->index--;
            }
            else
            {
                pdivoom_disp_light_handle->plant_mode = 1;
                pdivoom_disp_light_handle->index = pdivoom_disp_light_handle->max-1;//数组中使用最大值-1
            }
        }
        if(pdivoom_disp_light_handle->plant_mode == 4)
        {
            divoom_screen_color_info color = {0x00,0, 0xFF};
            pdivoom_disp_light_handle->plant_index = 0;
            divoom_disp_ctrl_fill(pdivoom_disp_light_handle->disp_buf, &color);
        }
        else
        {
            memcpy(&pdivoom_disp_light_handle->color, &divoom_disp_light_color_table[pdivoom_disp_light_handle->index], sizeof(divoom_screen_color_info));
            light_info->color.r_c = pdivoom_disp_light_handle->color.r_c;
            light_info->color.g_c = pdivoom_disp_light_handle->color.g_c;
            light_info->color.b_c = pdivoom_disp_light_handle->color.b_c;
//            divoom_disp_ctrl_fill(pdivoom_disp_light_handle->disp_buf, &pdivoom_disp_light_handle->color);
        }
        light_info->plant_mode = pdivoom_disp_light_handle->plant_mode;
        //divoom_screen_disp_rgb_data(pdivoom_disp_light_handle->disp_buf);
    }
}
void divoom_disp_light_play(uint8 status)
{
    if(status)
    {
        divoom_screen_disp_rgb32_data(pdivoom_disp_light_handle->disp_buf);
    }
}
void divoom_disp_light_close(void)
{
    if(pdivoom_disp_light_handle == NULL)
    {
        return;
    }
    
    if(pdivoom_disp_light_handle->task_id != T_U32_MAX)
    {
        divoom_com_pause_task(pdivoom_disp_light_handle->task_id);
        divoom_com_destroy_task(pdivoom_disp_light_handle->task_id);
        pdivoom_disp_light_handle->task_id = T_U32_MAX;
    }
    divoom_com_free(pdivoom_disp_light_handle, __func__, __LINE__);
    divoom_disp_ctrl_clr_screen();
	divoom_screen_switch(AK_TRUE);
    pdivoom_disp_light_handle = NULL;
	divoom_malloc_check_all_ram_out(__func__);
}



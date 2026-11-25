
#include "divoom_disp_com.h"

typedef struct _divoom_disp_time_manange_ctrl
{
    divoom_disp_time_manage_disp_item pdisp_item;
}divoom_disp_time_manange_ctrl, *Pdivoom_disp_time_manange_ctrl;


Pdivoom_disp_time_manange_ctrl pdivoom_disp_time_manange_handle = NULL;

void divoom_disp_time_manange_get_image_id(T_U8 area_id, Pdivoom_play_gif_image_info image_info)
{
    image_info->model_id = DIVOOM_MANAGE_IMAGE_TIME_PLAN_GIF; 
    image_info->image_id =  DIVOOM_MANAGE_IMAGE_ID_BASE_TIME_PLAN_PIC + pdivoom_disp_time_manange_handle->pdisp_item.index;
}


void divoom_disp_time_manange_init(void *data)
{
    #if 0
    extern const int channel_clock_list[DIVOOM_CHANNEL_CLOCK_LIST_NUM];
    int i;
    
    pdivoom_disp_time_manange_handle = divoom_system_malloc(sizeof(divoom_disp_time_manange_ctrl));
    memcpy(&pdivoom_disp_time_manange_handle->pdisp_item, data, sizeof(divoom_disp_time_manage_disp_item));
    
    if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 0)
    {
        for(i = 0; i < sizeof(channel_clock_list)/sizeof(channel_clock_list[0]); i++)
        {
            if(channel_clock_list[i] == pdivoom_disp_time_manange_handle->pdisp_item.clock_id)
            {
                break;
            }
        }
        if(i < sizeof(channel_clock_list)/sizeof(channel_clock_list[0]))
        {
            pdivoom_disp_time_manange_handle->pdisp_item.disp_mode = i + 2;
        }
    }
    if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 0)
    {
        divoom_disp_base_clock_init((void *)&pdivoom_disp_time_manange_handle->pdisp_item.clock_id);
    }
    else if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 2)
    {
        divoom_disp_subscribe_init(pdivoom_disp_time_manange_handle->pdisp_item.psubscribe_info);
    }
    else if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 3)
    {
        Pdivoom_disp_ctrl_user_define_info puser_info = (Pdivoom_disp_ctrl_user_define_info)pdivoom_disp_time_manange_handle->pdisp_item.puser_define;
        puser_info->index = 0;
        divoom_disp_user_define_init((void *)pdivoom_disp_time_manange_handle->pdisp_item.puser_define);
    }
    else if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 4)
    {
        Pdivoom_disp_ctrl_user_define_info puser_info = (Pdivoom_disp_ctrl_user_define_info)pdivoom_disp_time_manange_handle->pdisp_item.puser_define;
        puser_info->index = 1;
        divoom_disp_user_define_init((void *)pdivoom_disp_time_manange_handle->pdisp_item.puser_define);
    }
    else if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 5)
    {
        Pdivoom_disp_ctrl_user_define_info puser_info = (Pdivoom_disp_ctrl_user_define_info)pdivoom_disp_time_manange_handle->pdisp_item.puser_define;
        puser_info->index = 2;
        divoom_disp_user_define_init((void *)pdivoom_disp_time_manange_handle->pdisp_item.puser_define);
    }
    else
    {
        divoom_play_gif_com_init(0, 0, -1);
        divoom_play_gif_com_create_image_area(0, 0, 0, SCREEN_WIDTH_64, 0, divoom_disp_time_manange_get_image_id, NULL);    
    }
    #endif
}

void divoom_disp_time_manange_close(void)
{
    #if 0
    if(pdivoom_disp_time_manange_handle == NULL)
    {
        return;
    }
    
    if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 0)
    {
        divoom_disp_base_clock_close(1);
    }
    else if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 2)
    {
        divoom_disp_subscribe_close();
    }
    else if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 3)
    {
        divoom_disp_user_define_close();
    }
    else if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 4)
    {
        divoom_disp_user_define_close();
    }
    else if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 5)
    {
        divoom_disp_user_define_close();
    }
    else
    {
        divoom_play_gif_com_exit_image();
    }
    divoom_system_free(pdivoom_disp_time_manange_handle);
    pdivoom_disp_time_manange_handle = NULL;
    #endif
}

void divoom_disp_time_manange_update(void *data)
{
    #if 0
    extern const int channel_clock_list[DIVOOM_CHANNEL_CLOCK_LIST_NUM];
    int i, disp_mode;
    Pdivoom_disp_time_manage_disp_item pdisp_item = (Pdivoom_disp_time_manage_disp_item)data;
    
    if(pdivoom_disp_time_manange_handle == NULL)
    {
        return;
    }
    disp_mode = pdisp_item->disp_mode;
    if(pdisp_item->disp_mode == 0)
    {
        for(i = 0; i < sizeof(channel_clock_list)/sizeof(channel_clock_list[0]); i++)
        {
            if(channel_clock_list[i] == pdisp_item->clock_id)
            {
                break;
            }
        }
        if(i < sizeof(channel_clock_list)/sizeof(channel_clock_list[0]))
        {
            disp_mode = i + 2;
        }
    }
    
    if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == disp_mode)
    {
        memcpy(&pdivoom_disp_time_manange_handle->pdisp_item, data, sizeof(divoom_disp_time_manage_disp_item));
        pdivoom_disp_time_manange_handle->pdisp_item.disp_mode = disp_mode;
        if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 0)
        {
            divoom_disp_base_clock_update((void *)&pdivoom_disp_time_manange_handle->pdisp_item.clock_id, 1);
        }
        else if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 2)
        {
            divoom_disp_subscribe_update_config(pdivoom_disp_time_manange_handle->pdisp_item.psubscribe_info);
        }
        else if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 3)
        {
            Pdivoom_disp_ctrl_user_define_info puser_info = (Pdivoom_disp_ctrl_user_define_info)pdivoom_disp_time_manange_handle->pdisp_item.puser_define;
            puser_info->index = 0;
            divoom_disp_user_define_update((void *)pdivoom_disp_time_manange_handle->pdisp_item.puser_define);
        }
        else if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 4)
        {
            Pdivoom_disp_ctrl_user_define_info puser_info = (Pdivoom_disp_ctrl_user_define_info)pdivoom_disp_time_manange_handle->pdisp_item.puser_define;
            puser_info->index = 1;
            divoom_disp_user_define_update((void *)pdivoom_disp_time_manange_handle->pdisp_item.puser_define);
        }
        else if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 5)
        {
            Pdivoom_disp_ctrl_user_define_info puser_info = (Pdivoom_disp_ctrl_user_define_info)pdivoom_disp_time_manange_handle->pdisp_item.puser_define;
            puser_info->index = 2;
            divoom_disp_user_define_update((void *)pdivoom_disp_time_manange_handle->pdisp_item.puser_define);
        }
        else
        {
            divoom_play_gif_com_clear_all_cache();
        }
    }
    else
    {
        if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 0)
        {
            divoom_disp_base_clock_close(1);
        }
        else if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 2)
        {
            divoom_disp_subscribe_close();
        }
        else if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 3)
        {
            divoom_disp_user_define_close();
        }
        else if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 4)
        {
            divoom_disp_user_define_close();
        }
        else if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 5)
        {
            divoom_disp_user_define_close();
        }
        else
        {
            divoom_play_gif_com_exit_image();
        }
        memcpy(&pdivoom_disp_time_manange_handle->pdisp_item, data, sizeof(divoom_disp_time_manage_disp_item));
        pdivoom_disp_time_manange_handle->pdisp_item.disp_mode = disp_mode;
        
        if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 0)
        {
            divoom_disp_base_clock_init((void *)&pdivoom_disp_time_manange_handle->pdisp_item.clock_id);
        }
        else if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 2)
        {
            divoom_disp_subscribe_init(pdivoom_disp_time_manange_handle->pdisp_item.psubscribe_info);
        }
        else if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 3)
        {
            Pdivoom_disp_ctrl_user_define_info puser_info = (Pdivoom_disp_ctrl_user_define_info)pdivoom_disp_time_manange_handle->pdisp_item.puser_define;
            puser_info->index = 0;
            divoom_disp_user_define_init((void *)pdivoom_disp_time_manange_handle->pdisp_item.puser_define);
        }
        else if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 4)
        {
            Pdivoom_disp_ctrl_user_define_info puser_info = (Pdivoom_disp_ctrl_user_define_info)pdivoom_disp_time_manange_handle->pdisp_item.puser_define;
            puser_info->index = 1;
            divoom_disp_user_define_init((void *)pdivoom_disp_time_manange_handle->pdisp_item.puser_define);
        }
        else if(pdivoom_disp_time_manange_handle->pdisp_item.disp_mode == 5)
        {
            Pdivoom_disp_ctrl_user_define_info puser_info = (Pdivoom_disp_ctrl_user_define_info)pdivoom_disp_time_manange_handle->pdisp_item.puser_define;
            puser_info->index = 2;
            divoom_disp_user_define_init((void *)pdivoom_disp_time_manange_handle->pdisp_item.puser_define);
        }
        else
        {
            divoom_play_gif_com_init(0, 0, -1);
            divoom_play_gif_com_create_image_area(0, 0, 0, SCREEN_WIDTH_64, 0, divoom_disp_time_manange_get_image_id, NULL);    
        }
        
    }
    #endif
}



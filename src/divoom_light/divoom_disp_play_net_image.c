#include "divoom.h"

typedef struct _divoom_disp_Play_net_image_ctrl
{
    uint8 task_id;
    int cur_pic_id;
    divoom_disp_Play_net_image_image_info ctrl_info;
    Pdivoom_http_download_image_file_info pimage_handle;
}divoom_disp_Play_net_image_ctrl, *Pdivoom_disp_Play_net_image_ctrl;

Pdivoom_disp_Play_net_image_ctrl pdivoom_disp_Play_net_image_handle = NULL;


void divoom_disp_Play_net_image_main(void *data)
{
    if(pdivoom_disp_Play_net_image_handle->pimage_handle)
    {
        
        divoom_screen_disp_send_rgb_data(pdivoom_disp_Play_net_image_handle->pimage_handle->pimage_table[pdivoom_disp_Play_net_image_handle->cur_pic_id]->image_data, pdivoom_disp_Play_net_image_handle->pimage_handle->width);
        pdivoom_disp_Play_net_image_handle->cur_pic_id ++;
        if(pdivoom_disp_Play_net_image_handle->cur_pic_id >= pdivoom_disp_Play_net_image_handle->pimage_handle->num)
        {
            pdivoom_disp_Play_net_image_handle->cur_pic_id = 0;
            
            if(pdivoom_disp_Play_net_image_handle->ctrl_info.play_time)
            {
               pdivoom_disp_Play_net_image_handle->ctrl_info.play_time --;
            } 
            
            if(pdivoom_disp_Play_net_image_handle->ctrl_info.play_time == 0 && pdivoom_disp_Play_net_image_handle->ctrl_info.pcallback)
            {
                pdivoom_disp_Play_net_image_handle->ctrl_info.pcallback(pdivoom_disp_Play_net_image_handle->ctrl_info.event_id);
            }
        }
    }
}


void divoom_disp_Play_net_image_init(void *data)
{
    Pdivoom_disp_Play_net_image_image_info pplay_info = (Pdivoom_disp_Play_net_image_image_info)data;
    if(pdivoom_disp_Play_net_image_handle != NULL)
    {
        return;
    }
    pdivoom_disp_Play_net_image_handle = divoom_system_malloc(sizeof(divoom_disp_Play_net_image_ctrl));
    pdivoom_disp_Play_net_image_handle->pimage_handle = divoom_manage_image_read(pplay_info->model_id, pplay_info->image_id, NULL, NULL);
    if(pdivoom_disp_Play_net_image_handle->pimage_handle == NULL)
    {
        pdivoom_disp_Play_net_image_handle->pimage_handle = divoom_manage_image_read(DIVOOM_MANAGE_IMAGE_NOTICE_NET_GIF, DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + DIVOOM_DISP_USER_DEFINE_TV_PIC, NULL, NULL);
    }
    
    if(pdivoom_disp_Play_net_image_handle->pimage_handle)
    {
        divoom_manage_image_format_gif_table(pdivoom_disp_Play_net_image_handle->pimage_handle, NULL);
        memcpy(&pdivoom_disp_Play_net_image_handle->ctrl_info, data, sizeof(divoom_disp_Play_net_image_image_info));
        pdivoom_disp_Play_net_image_handle->task_id = divoom_com_create_task(divoom_disp_Play_net_image_main, pdivoom_disp_Play_net_image_handle->pimage_handle->speed/10);
        divoom_com_resume_task(pdivoom_disp_Play_net_image_handle->task_id);
    }
}

void divoom_disp_Play_net_image_update(void *data)
{
    Pdivoom_disp_Play_net_image_image_info pplay_info = (Pdivoom_disp_Play_net_image_image_info)data;
    
    if(pdivoom_disp_Play_net_image_handle == NULL)
    {
        return;
    }
    if(memcmp(&pdivoom_disp_Play_net_image_handle->ctrl_info, data, sizeof(divoom_disp_Play_net_image_ctrl)))
    {
        divoom_com_pause_task(pdivoom_disp_Play_net_image_handle->task_id);
        memcpy(&pdivoom_disp_Play_net_image_handle->ctrl_info, data, sizeof(divoom_disp_Play_net_image_image_info));
        divoom_http_download_image_free_image_handle(pdivoom_disp_Play_net_image_handle->pimage_handle);
        pdivoom_disp_Play_net_image_handle->pimage_handle = divoom_manage_image_read(pplay_info->model_id, pplay_info->image_id, NULL, NULL);
        
        if(pdivoom_disp_Play_net_image_handle->pimage_handle == NULL)
        {
            pdivoom_disp_Play_net_image_handle->pimage_handle = divoom_manage_image_read(DIVOOM_MANAGE_IMAGE_NOTICE_NET_GIF, DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + DIVOOM_DISP_USER_DEFINE_TV_PIC, NULL, NULL);
        }
        if(pdivoom_disp_Play_net_image_handle->pimage_handle)
        {
            divoom_manage_image_format_gif_table(pdivoom_disp_Play_net_image_handle->pimage_handle, NULL);
            pdivoom_disp_Play_net_image_handle->cur_pic_id = 0;
            divoom_com_update_task(pdivoom_disp_Play_net_image_handle->task_id, pdivoom_disp_Play_net_image_handle->pimage_handle->speed/10);
        }
        divoom_com_resume_task(pdivoom_disp_Play_net_image_handle->task_id);
    }
}


void divoom_disp_Play_net_image_exit(void)
{
    if(pdivoom_disp_Play_net_image_handle == NULL)
    {
        return;
    }
    divoom_com_pause_task(pdivoom_disp_Play_net_image_handle->task_id);
    divoom_com_destroy_task(pdivoom_disp_Play_net_image_handle->task_id);
    divoom_http_download_image_free_image_handle(pdivoom_disp_Play_net_image_handle->pimage_handle);
    divoom_system_free(pdivoom_disp_Play_net_image_handle);
    pdivoom_disp_Play_net_image_handle = NULL;
}


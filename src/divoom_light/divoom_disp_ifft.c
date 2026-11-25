#include "divoom_disp_com.h"
#if 0
typedef struct _divoom_disp_ifft_ctrl
{
    char AppName[64];
    uint32 resource_type;
}divoom_disp_ifft_ctrl, *Pdivoom_disp_ifft_ctrl;


static Pdivoom_disp_ifft_ctrl pdivoom_disp_ifft_handle = NULL;

void divoom_disp_ifft_update(void *data)
{
    Pdivoom_disp_net_channel_para pchannel_info = (Pdivoom_disp_net_channel_para)data;
    
    divoom_app_com_get_str_from_config(pchannel_info->config, "Name", pdivoom_disp_ifft_handle->AppName);
    if(strlen(pdivoom_disp_ifft_handle->AppName) == 0)
    {
        strcpy(pdivoom_disp_ifft_handle->AppName, pchannel_info->name);
    }    
    pdivoom_disp_ifft_handle->resource_type = pchannel_info->resrouce_type;
    if(pdivoom_disp_ifft_handle->resource_type & DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE)
    {
        //如果已经启动，先暂停一下
//        divoom_image_disp_play(AK_FALSE);
    }
    
    if((pdivoom_disp_ifft_handle->resource_type & DIVOOM_SCHEDULE_WHITE_RESOUCE_TYPE))
    {
        divoom_main_out_disp_str(pdivoom_disp_ifft_handle->AppName, 0Xff, NULL);
    }
    
    if((pdivoom_disp_ifft_handle->resource_type & DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE))
    {
//        divoom_image_disp_update_file(pchannel_info->pic_addr);
    }
    
    if(pdivoom_disp_ifft_handle->resource_type & DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE)
    {
//        divoom_image_disp_play(AK_TRUE);
    }
}


void divoom_disp_ifft_init(void *data)
{
    if(pdivoom_disp_ifft_handle)
    {
        return;
    }
    
    pdivoom_disp_ifft_handle = (Pdivoom_disp_ifft_ctrl)divoom_system_malloc(sizeof(divoom_disp_ifft_ctrl));
    memset(pdivoom_disp_ifft_handle, 0, sizeof(divoom_disp_ifft_ctrl));
    divoom_disp_ifft_update(data);
}

void divoom_disp_ifft_close(void)
{
    divoom_system_free(pdivoom_disp_ifft_handle);
    pdivoom_disp_ifft_handle = NULL;
}
#endif



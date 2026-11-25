#include "divoom_disp_com.h"



void divoom_disp_download_file_get_pic_id(T_U8 area_id, Pdivoom_play_gif_image_info image_info)
{
    image_info->image_id =  DIVOOM_DISP_SD_WAIT_SD_WAIT + DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF;
    image_info->model_id = DIVOOM_MANAGE_IMAGE_LOCAL_PIC_MODE;
}

void divoom_disp_download_file_update(uint8 *data)
{
    
}

void divoom_disp_download_file_init(uint8 *data)
{
}
void divoom_disp_download_file_close(void)
{    
}



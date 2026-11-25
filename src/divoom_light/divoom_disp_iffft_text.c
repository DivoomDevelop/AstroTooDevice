#include "divoom.h"

void divoom_disp_iffft_text_get_image_id(T_U8 area_id, Pdivoom_play_gif_image_info image_info)
{
    if(area_id == 0)
    {
        image_info->image_id = DIVOOM_lOCAL_USE_PIC_NOTICE_IFFFT_TEXT_MSG + DIVOOM_MANAGE_IMAGE_ID_BASE_NOTICE_NET_PIC;;
        image_info->model_id = DIVOOM_MANAGE_IMAGE_NOTICE_NET_GIF;
    }
}

void divoom_disp_iffft_text_init(void *data)
{

}


void divoom_disp_iffft_text_update(void *data)
{
}


void divoom_disp_iffft_text_exit(void)
{
}


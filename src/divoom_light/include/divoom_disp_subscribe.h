#ifndef _divoom_disp_subscribe_h_
#define _divoom_disp_subscribe_h_
void divoom_disp_subscribe_init(Pdivoom_disp_subscribe_data psuscribe_info);
void divoom_disp_subscribe_update_config(Pdivoom_disp_subscribe_data psuscribe_info);
void divoom_disp_subscribe_close();
/*********************************************************************
  Function:        divoom_disp_subscribe_update_last_image
  Description:     更新最新的图片信息
  Input:           
  Return:          none
**********************************************************************/

void divoom_disp_subscribe_update_last_image(int mode);
#endif



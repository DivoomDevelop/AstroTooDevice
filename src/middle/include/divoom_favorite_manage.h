#ifndef _divoom_favorite_manage_h_
#define _divoom_favorite_manage_h_
void divoom_favorite_manage_init(uint8 *mac_addr);
/*********************************************************************
  Function:        divoom_favorite_manage_clear_all
  Description:     不使用收藏夹了，将清空相关信息
  Input:           none
  Return:          none
**********************************************************************/

void divoom_favorite_manage_clear_all(void);
void divoom_favorite_manage_start_clear_all(void);

/*********************************************************************
  Function:        divoom_favorite_manage_update_list
  Description:     更新最新的收藏列表
  Input:           plast_pic
                   
                    
  Return:          是否更新
**********************************************************************/

int divoom_favorite_manage_update_last(Pdivoom_devcice_image_file plast_pic);

/*********************************************************************
  Function:        divoom_favorite_manage_update_list
  Description:     新增加收藏列表
  Input:           pimage_list
                   
                    
  Return:          none
**********************************************************************/

void divoom_favorite_manage_update_list(Pdivoom_devcice_image_list pimage_list);

/*********************************************************************
  Function:        divoom_favorite_manage_update_item
  Description:     新增加收藏列表
  Input:           pimage_item
                   
                    
  Return:          none
**********************************************************************/

void divoom_favorite_manage_update_item(Pdivoom_devcice_image_file pimage_item);

/* 异步用户信息回包入口：按 USERID 更新收藏图片的作者名称。 */
void divoom_favorite_manage_update_author_list(
    Pdivoom_device_get_user_info_list_respone_info puser_info);

/*********************************************************************
  Function:        divoom_favorite_manage_update_config
  Description:     更新显示的配置
  Input:           flag  //０：不显示，1:显示最新的订阅的，　2:显示全部
                   
                    
  Return:          none
**********************************************************************/

void divoom_favorite_manage_update_config(uint8 flag);

/*********************************************************************
  Function:        divoom_favorite_manage_loop
  Description:     主循环不断向服务器请求相关数据
  Input:           image_size  DIVOOM_HOT_MANAGE_PIC_16之类
                   
                    
  Return:          none
**********************************************************************/

void divoom_favorite_manage_loop(void);

/*********************************************************************
  Function:        divoom_favorite_manage_get_url_addr
  Description:     获取指定文件的网络地址
  Input:           model_id
                   image_id
                   url_addr
                    
  Return:          image_flag
**********************************************************************/

int divoom_favorite_manage_get_url_addr(int model_id, uint32 image_id, char *url_addr);


/*********************************************************************
  Function:        divoom_favorite_manage_get_disp_image_id
  Description:     得到一个将要播放的图像
  Input:           none
  Return:          图像ＩＤ
**********************************************************************/

int divoom_favorite_manage_get_disp_image_id(uint32 *user_id, char *image_name);

/*********************************************************************
  Function:        divoom_favorite_manage_start_up_load_image
  Description:     连接上服务器加载相应的图片
  Input:           flag  
                   
                    
  Return:          none
**********************************************************************/

void divoom_favorite_manage_start_up_load_image(uint8 flag);

int divoom_favorite_manage_request_list(int update_time);
void divoom_favorite_manage_open(void);
void divoom_favorite_manage_close(void);
/*********************************************************************
  Function:        divoom_favorite_manage_get_url_addr
  Description:     获取指定文件的网络地址
  Input:           model_id
                   image_id
                   url_addr
                    
  Return:          image_flag
**********************************************************************/

int divoom_favorite_manage_get_url_addr(int model_id, uint32 image_id, char *url_addr);

#endif

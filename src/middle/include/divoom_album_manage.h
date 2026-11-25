#ifndef _divoom_album_manage_h_
#define _divoom_album_manage_h_


/*********************************************************************
  Function:        divoom_album_manage_init
  Description:     初始化模块，将相关的文件从spi中load出来
  Input:           file_path  SD路径 
                    
  Return:           none
**********************************************************************/

void divoom_album_manage_init(void);

/*********************************************************************
  Function:        divoom_album_manage_reset
  Description:     复位模块，全部删除相关数据
  Input:           none
                    
  Return:           none
**********************************************************************/

void divoom_album_manage_reset(void);


/*********************************************************************
  Function:        divoom_album_manage_get_random_pic_id
  Description:     得到某类型图片的ＩＤ
  Input:           
  Return:          返回图片ＩＤ
**********************************************************************/

uint32 divoom_album_manage_get_random_pic_id(int album_id, int *user_id, char *image_name);


/*********************************************************************
  Function:        divoom_album_manage_open_album_id
  Description:     打开专辑资源
  Input:           
  Return:          
**********************************************************************/


void *divoom_album_manage_open_album_id(int album_id);

/*********************************************************************
  Function:        divoom_album_manage_close_album_id
  Description:     关闭专辑资源
  Input:           
  Return:          
**********************************************************************/

void divoom_album_manage_close_album_id(int album_id);


/*********************************************************************
  Function:        divoom_album_manage_change_album_id
  Description:     某个表盘切换专辑，如果这个专辑没有被使用，将清理相关资源
  Input:           clock_id
                   clock_pos
                   parent_id
                   cur_album_id
                   new_album_id
                    
  Return:           none
**********************************************************************/

void divoom_album_manage_change_album_id(int clock_id, int clock_pos, int parent_id, int cur_album_id,int new_album_id);

/*********************************************************************
  Function:        divoom_album_manage_update_file_list
  Description:     更新图片列表
  Input:           
                    
  Return:          none
**********************************************************************/

void divoom_album_manage_update_file_list(Pdivoom_devcice_image_list pimage_list);

/* 异步用户信息回包入口：按 USERID 更新专辑图片的作者名称。 */
void divoom_album_manage_update_author_list(
    Pdivoom_device_get_user_info_list_respone_info puser_info);



/*********************************************************************
  Function:        divoom_album_manage_get_url_addr
  Description:     获取指定文件的网络地址
  Input:           model_id
                   image_id
                   url_addr
                    
  Return:          image_flag
**********************************************************************/

int divoom_album_manage_get_url_addr(int model_id, uint32 image_id, char *url_addr);

#endif



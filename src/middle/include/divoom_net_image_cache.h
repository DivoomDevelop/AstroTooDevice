#ifndef _divoom_net_image_cache_h_
#define _divoom_net_image_cache_h_

#include "divoom_http_download_image.h"

/*********************************************************************
  Function:        divoom_net_image_cache_init
  Description:     模块初始化操作
  Input:            
  Return:           NONE
**********************************************************************/

void divoom_net_image_cache_init(void);


/*********************************************************************
  Function:        divoom_net_image_cache_remove_image
  Description:     清除某个图像的数据
  Input:            id          
                    file_addr
  Return:           NONE
**********************************************************************/

void divoom_net_image_cache_remove_image(int id, char *file_addr);


/*********************************************************************
  Function:        divoom_net_image_cache_remove_mode
  Description:     清除这个模块的所有的数据
  Input:            mode_id          
                    
  Return:           NONE
**********************************************************************/

void divoom_net_image_cache_remove_mode(int mode_id);



/*********************************************************************
  Function:        divoom_net_image_cache_upate_end
  Description:     结束模块更新，将这个模块没有使用的数据全部清除
  Input:            mode_id          
                    
  Return:           NONE
**********************************************************************/

void divoom_net_image_cache_upate_end(int mode_id);


/*********************************************************************
  Function:        divoom_net_image_cache_upate_mode
  Description:     设置使用标记
  Input:            mode_id          
                    id              服务器的图像id,为-1时使用file_addr
                    file_addr       文件地址
  Return:           NONE
**********************************************************************/

void divoom_net_image_cache_upate_mode(int mode_id, int id, char *file_addr);


/*********************************************************************
  Function:        divoom_net_image_cache_clear_mode
  Description:     清除模块,只是设置清除标记
  Input:            mode_id          模块ＩＤ
  Return:           NONE
**********************************************************************/

void divoom_net_image_cache_clear_mode(int mode_id);


/*********************************************************************
  Function:        divoom_net_image_cache_add_addr_item
  Description:     向模块添加一个文件，启动下载
  Input:            mode_id          服务器的图像id,为-1时使用file_addr
                    id          方便查找
                    file_addr   服务器的图像网络地址
  Return:           NONE
**********************************************************************/


void divoom_net_image_cache_add_addr_item(int mode_id, int id, char *file_addr);


/*********************************************************************
  Function:        divoom_net_image_cache_get_image_handle
  Description:     获取图像的相关数据指针,此函数可能会造成一定的阻塞,外面只能使用这个结构，但是不能释放与写入
  Input:            id          服务器的图像id,为-1时使用file_addr
                    file_addr   服务器的图像网络地址
  Return:           返回图像缓冲区
**********************************************************************/


Pdivoom_http_download_image_file_info divoom_net_image_cache_get_image_handle(int id, char *file_addr);


/*********************************************************************
  Function:        divoom_net_image_cache_update_net_status
  Description:     更新网络状况
  Input:            
  Return:           NONE
**********************************************************************/


void divoom_net_image_cache_update_net_status(uint8 net_ok);


#endif


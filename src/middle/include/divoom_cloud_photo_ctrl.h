#ifndef _divoom_cloud_photo_ctrl_h_
#define _divoom_cloud_photo_ctrl_h_

/*********************************************************************
  Function:        divoom_cloud_photo_ctrl_init
  Description:     初始化
  Input:           
                   
  Return:          ＮＯＮＥ
**********************************************************************/

void divoom_cloud_photo_ctrl_init(void);

/*********************************************************************
  Function:        divoom_cloud_photo_ctrl_add_item
  Description:     增加一个元素
  Input:           vision_id
                   oss_name
  Return:          ＮＯＮＥ
**********************************************************************/

void divoom_cloud_photo_ctrl_request_more_info(int clock_id);


/*********************************************************************
  Function:        divoom_cloud_photo_ctrl_add_item
  Description:     增加一个元素
  Input:           vision_id
                   oss_name
  Return:          ＮＯＮＥ
**********************************************************************/

void divoom_cloud_photo_ctrl_request_more_info(int clock_id);

/*********************************************************************
  Function:        divoom_cloud_photo_ctrl_add_item
  Description:     增加一个元素
  Input:           vision_id
                   oss_name
  Return:          ＮＯＮＥ
**********************************************************************/

void divoom_cloud_photo_ctrl_add_item(Pdivoom_device_cloud_photo_item pitem_info);


/*********************************************************************
  Function:        divoom_cloud_photo_ctrl_del_item
  Description:     删除一个元素
  Input:           vision_id
  Return:          ＮＯＮＥ
**********************************************************************/

void divoom_cloud_photo_ctrl_del_item(int vision_id);

/*********************************************************************
  Function:        divoom_cloud_photo_ctrl_update_list
  Description:     更新播放资源列表，这个是增量更新
  Input:           pinfo
  Return:          ＮＯＮＥ
**********************************************************************/

void divoom_cloud_photo_ctrl_update_list(Pdivoom_device_cloud_photo_list_info pinfo);


/*********************************************************************
  Function:        divoom_cloud_photo_ctrl_open_handle
  Description:     打开播放资源
  Input:           ＮＯＮＥ
  Return:          ＮＯＮＥ
**********************************************************************/

void *divoom_cloud_photo_ctrl_open_handle(void);

/*********************************************************************
  Function:        divoom_cloud_photo_ctrl_close_handle
  Description:     关闭播放资源
  Input:           ＮＯＮＥ
  Return:          ＮＯＮＥ
**********************************************************************/

void divoom_cloud_photo_ctrl_close_handle(void);

/*********************************************************************
  Function:        divoom_cloud_photo_ctrl_get_total
  Description:     得到要播放的文件数目
  Input:           ＮＯＮＥ
  Return:          文件数目
**********************************************************************/

int divoom_cloud_photo_ctrl_get_total(void);


/*********************************************************************
  Function:        divoom_cloud_photo_ctrl_get_item
  Description:     得到要播放的文件名
  Input:           file_name
  Return:          1 or 0
**********************************************************************/

int divoom_cloud_photo_ctrl_get_item(char *file_name);


/*********************************************************************
  Function:        divoom_cloud_photo_redownload_pic_id
  Description:     重新下载这个文件
  Input:           pic_id
  Return:          1 or 0
**********************************************************************/

void divoom_cloud_photo_redownload_pic_id(int pic_id);

/*********************************************************************
  Function:        divoom_cloud_photo_get_last_pic_id
  Description:     得到最后一个播放的ＩＤ
  Input:           
  Return:          pic_id
**********************************************************************/

int divoom_cloud_photo_get_last_pic_id(void);

#endif




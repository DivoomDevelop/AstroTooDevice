#ifndef _divoom_app_pic_manage_h_
#define _divoom_app_pic_manage_h_

/*********************************************************************
  Function:        divoom_app_pic_manage_init
  Description:     初始化
  Input:           none
  Return:          none
**********************************************************************/

void divoom_app_pic_manage_init(void);


/*********************************************************************
  Function:        divoom_app_pic_manage_update
  Description:     更新app列表
  Input:           plist
                   num
  Return:          none
**********************************************************************/

void divoom_app_pic_manage_update(Pdivoom_app_pic_manage_item plist, int num);


/*********************************************************************
  Function:        divoom_app_pic_manage_use_app
  Description:     加载app图片
  Input:           app_id
  Return:          none
**********************************************************************/

void divoom_app_pic_manage_use_app(int app_id);

/*********************************************************************
  Function:        divoom_app_pic_manage_get_url_addr
  Description:     获取指定文件的网络地址
  Input:           model_id
                   image_id
                   url_addr
                    
  Return:          image_flag
**********************************************************************/

int divoom_app_pic_manage_get_url_addr(int model_id, int image_id, char *url_addr);

#endif




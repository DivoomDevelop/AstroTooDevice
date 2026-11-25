#ifndef _divoom_user_head_manage_h_
#define _divoom_user_head_manage_h_


/*********************************************************************
  Function:        divoom_user_head_manage_init
  Description:     初始化模块数据
  Input:           
                   	
                    
  Return:           none
**********************************************************************/

void divoom_user_head_manage_init(void);

/*********************************************************************
  Function:        divoom_user_head_manage_update_info
  Description:     更新用户数据，如果头像更新，将重新下载数据
  Input:           user_id
                   user_name
                   file_id	
                    
  Return:           none
**********************************************************************/

int divoom_user_head_manage_update_info(uint32 user_id, char *user_name, char *file_id);
/*********************************************************************
  Function:        divoom_user_head_manage_get_info
  Description:     获取用户的用户名
  Input:           user_id
                   user_name
                    
  Return:           none
**********************************************************************/

void divoom_user_head_manage_get_info(uint32 user_id, char *user_name, char *file_id);

/*********************************************************************
  Function:        divoom_user_head_manage_update_list
  Description:     更新用户列表信息
  Input:           plist
                    
  Return:           none
**********************************************************************/

void divoom_user_head_manage_update_list(Pdivoom_device_get_user_info_list_respone_info plist);

/*********************************************************************
  Function:        divoom_user_head_manage_get_url_addr
  Description:     获取用户头像的相关数据
  Input:           user_id
                   url_addr 
  Return:           none
**********************************************************************/

void divoom_user_head_manage_get_url_addr(uint32 user_id,char * url_addr);

#endif


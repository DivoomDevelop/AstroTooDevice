#ifndef _divoom_expert_manage_h_

#define _divoom_expert_manage_h_

/*********************************************************************
  Function:        divoom_expert_manage_init
  Description:     模块初始化
  Input:           
                    
  Return:          none
**********************************************************************/

void divoom_expert_manage_init(void);
/*********************************************************************
  Function:        divoom_expert_manage_reset
  Description:     模块复位
  Input:           
                    
  Return:          none
**********************************************************************/

void divoom_expert_manage_reset(void);

/*********************************************************************
  Function:        divoom_expert_manage_run_loop
  Description:     模块加载列表
  Input:           
                    
  Return:          none
**********************************************************************/

void divoom_expert_manage_run_loop(void);


/*********************************************************************
  Function:        divoom_expert_manage_update_mode
  Description:     设置达人订阅的模式
  Input:           mode 0:不显示，1 显示
                   num  订阅达人数目
                   user_list  达人列表
					disp_type  2：显示全部，1：显示最新
                    
  Return:          none
**********************************************************************/

void divoom_expert_manage_update_mode(uint8 mode, int num, int *user_list, int disp_type);

/*********************************************************************
  Function:        divoom_expert_manage_update_last_image
  Description:     更新达人最新的图
  Input:           mode
                    
  Return:          none
**********************************************************************/

void divoom_expert_manage_update_last_image(Pdivoom_devcice_image_list pimage_list);

/*********************************************************************
  Function:        divoom_expert_manage_update_list_image
  Description:     更新达人图的列表
  Input:           mode
                    
  Return:          none
**********************************************************************/

void divoom_expert_manage_update_list_image(Pdivoom_devcice_expert_list pimage_list);

/* 异步用户信息回包入口：按 USERID 更新达人作品的作者名称。 */
void divoom_expert_manage_update_author_list(
    Pdivoom_device_get_user_info_list_respone_info puser_info);

/*********************************************************************
  Function:        divoom_expert_manage_get_disp_image_id
  Description:     得到要显示的那张图的ＩＤ
  Input:           image_info
                    
  Return:          none
**********************************************************************/

void divoom_expert_manage_get_disp_image_id(int expert_id, Pdivoom_play_gif_image_info image_info, int last_flag, char *image_name);

/*********************************************************************
  Function:        divoom_expert_manage_update_list_image
  Description:     更新达人最新的图
  Input:           mode
                    
  Return:          none
**********************************************************************/

void divoom_expert_manage_close_expert_handle(int expert_id);

/*********************************************************************
  Function:        divoom_expert_manage_add_expert_info
  Description:     添加某个表盘的某个达人的信息
  Input:           mode
                    
  Return:          none
**********************************************************************/

void divoom_expert_manage_add_expert_info(int clock_id, int clock_pos, int parent_id, int expert_id);

/*********************************************************************
  Function:        divoom_expert_manage_delete_expert_info
  Description:     删除某个表盘的某个达人信息
  Input:           mode
                    
  Return:          none
**********************************************************************/

void divoom_expert_manage_delete_expert_info(int clock_id, int clock_pos, int parent_id, int expert_id);

/*********************************************************************
  Function:        divoom_expert_manage_update_list_image
  Description:     更新达人最新的图
  Input:           mode
                    
  Return:          none
**********************************************************************/

void *divoom_expert_manage_open_expert_handle(int expert_id);

/*********************************************************************
  Function:        divoom_expert_manage_get_url_addr
  Description:     获取指定文件的网络地址
  Input:           model_id
                   image_id
                   url_addr
                    
  Return:          image_flag
**********************************************************************/

int divoom_expert_manage_get_url_addr(int model_id, uint32 image_id, char *url_addr);



#endif


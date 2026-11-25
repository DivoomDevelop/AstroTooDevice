#ifndef _divoom_user_define_ctrl_h_
#define _divoom_user_define_ctrl_h_
/*********************************************************************
  Function:        divoom_user_define_ctrl_init
  Description:     模块初始化
  Input:           none
                    
  Return:           none
**********************************************************************/

void *divoom_user_define_ctrl_open(Pdivoom_clock_id_info clock_info);

/*********************************************************************
  Function:        divoom_user_define_ctrl_reset
  Description:     清空整个自定义
  Input:           pnew_item   网络数据结构
                    
  Return:           none
**********************************************************************/

void divoom_user_define_ctrl_reset(void);


/*********************************************************************
  Function:        divoom_user_define_ctrl_close
  Description:     模块释放相关资源
  Input:           none
                    
  Return:           none
**********************************************************************/

void divoom_user_define_ctrl_close(Pdivoom_clock_id_info clock_info);


/*********************************************************************
  Function:        divoom_user_define_ctrl_reload
  Description:     当网络正常了，要重新加载一下自定义
  Input:           type   第几个自定义
                   cur_id 正在播放的，可以为０
                    
  Return:           none
**********************************************************************/

void divoom_user_define_ctrl_reload(void);

/*********************************************************************
  Function:        divoom_user_define_ctrl_get_voice_flag
  Description:     得到自定义是否可以播放声音
  Input:           
                    
  Return:          true or flase
**********************************************************************/

int divoom_user_define_ctrl_get_voice_flag(void);

/*********************************************************************
  Function:        divoom_user_define_ctrl_get_show_time_flag
  Description:     得到自定义是否显示时间
  Input:           
                    
  Return:          true or flase
**********************************************************************/

int divoom_user_define_ctrl_get_show_time_flag(void);

/*********************************************************************
  Function:        divoom_user_define_ctrl_get_next_id
  Description:     得到将要播放的文件ＩＤ
  Input:           clock_info   
                    
  Return:           none
**********************************************************************/

int divoom_user_define_ctrl_get_next_id(Pdivoom_clock_id_info clock_info, int *user_id, char *image_name, int *voice_id);



/*********************************************************************
  Function:        divoom_user_define_ctrl_get_play_time
  Description:     得到将要播放的播放时间
  Input:           clock_info   
                    
  Return:           none
**********************************************************************/

int divoom_user_define_ctrl_get_play_time(Pdivoom_clock_id_info clock_info);

/*********************************************************************
  Function:        divoom_user_define_ctrl_update_play_time
  Description:     更新播放的播放时间
  Input:           clock_info   
                   play_time
                   show_time
                    
  Return:           none
**********************************************************************/

void divoom_user_define_ctrl_update_play_time(Pdivoom_clock_id_info clock_info, int play_time, int show_time);

/*********************************************************************
  Function:        divoom_user_define_ctrl_update
  Description:     模块更新
  Input:           plist   网络数据结构
                    
  Return:           none
**********************************************************************/

void divoom_user_define_ctrl_update(Pdivoom_devcice_user_define_respone plist);

/* 异步用户信息回包入口：按 USERID 更新自定义图片的作者名称。 */
void divoom_user_define_ctrl_update_author_list(
    Pdivoom_device_get_user_info_list_respone_info puser_info);

/*********************************************************************
  Function:        divoom_user_define_ctrl_init
  Description:     模块初始化
  Input:           none
                    
  Return:           none
**********************************************************************/

void divoom_user_define_ctrl_init(uint8 *mac_addr);



/*********************************************************************
  Function:        divoom_user_define_ctrl_set_item
  Description:     删除某个子项
  Input:           pdel_item   网络数据结构
                    
  Return:           none
**********************************************************************/

int divoom_user_define_ctrl_del_item(Pdivoom_devcice_user_define_item pdel_item);


/*********************************************************************
  Function:        divoom_user_define_ctrl_clean_item
  Description:     删除整个自定义
  Input:           pnew_item   网络数据结构
                    
  Return:           none
**********************************************************************/

void divoom_user_define_ctrl_clean_item(Pdivoom_devcice_user_define_item pclean_item);


/*********************************************************************
  Function:        divoom_user_define_ctrl_set_item
  Description:     更新某个子项
  Input:           pnew_item   网络数据结构
                    
  Return:           none
**********************************************************************/

void divoom_user_define_ctrl_set_item(Pdivoom_devcice_user_define_item pnew_item);


/*********************************************************************
  Function:        divoom_user_define_ctrl_check_exist_id
  Description:     检查这个文件是否存在
  Input:           image_id   
                    
  Return:           none
**********************************************************************/

int divoom_user_define_ctrl_check_exist_id(int image_id);

/*********************************************************************
  Function:        divoom_user_define_ctrl_get_url_addr
  Description:     获取指定文件的网络地址
  Input:           model_id
                   image_id
                   url_addr
                    
  Return:          image_flag
**********************************************************************/

int divoom_user_define_ctrl_get_url_addr(int model_id, int image_id, char *url_addr);

#endif


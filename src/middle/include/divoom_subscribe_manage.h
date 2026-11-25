#ifndef  _divoom_subscribe_manage_h_
#define _divoom_subscribe_manage_h_
/*********************************************************************
  Function:        divoom_subscribe_manage_init
  Description:     初始化
  Input:           none
                    
  Return:          none
**********************************************************************/

void divoom_subscribe_manage_init(void);

/* Reclaim unused modes in batches; update free_space after each batch. */
int divoom_subscribe_manage_clear_unused_data_if_needed(uint32 *free_space);

/*********************************************************************
  Function:        divoom_subscribe_manage_update
  Description:     更新表盘配置
  Input:           pclock_info
                   psubscribe_info 
  Return:          none
**********************************************************************/

void divoom_subscribe_manage_update(Pdivoom_clock_id_info pclock_info, Pdivoom_channel_set_subscribe_info psubscribe_info);


/*********************************************************************
  Function:        divoom_subscribe_manage_open
  Description:     打开读表盘配置句柄
  Input:           pclock_info
  Return:          handle
**********************************************************************/

void *divoom_subscribe_manage_open(Pdivoom_clock_id_info pclock_info);

/*********************************************************************
  Function:        divoom_subscribe_manage_get_play_time
  Description:     获取播放时间
  Input:           pclock_info
  Return:          时间
**********************************************************************/


int divoom_subscribe_manage_get_play_time(Pdivoom_clock_id_info pclock_info);

/*********************************************************************
  Function:        divoom_subscribe_manage_get_show_time_flag
  Description:     获取订阅表盘是否显示时间
  Input:           pclock_info
  Return:          1 显示，其他值不显示
**********************************************************************/

int divoom_subscribe_manage_get_show_time_flag(Pdivoom_clock_id_info pclock_info);


/*********************************************************************
  Function:        divoom_subscribe_manage_get_image_id
  Description:     获取播放文件相关信息
  Input:           pclock_info
                   image_info
  Return:          none
**********************************************************************/


void divoom_subscribe_manage_get_image_id(Pdivoom_clock_id_info pclock_info, Pdivoom_play_gif_image_info image_info);

/*********************************************************************
  Function:        divoom_subscribe_manage_get_change_flag
  Description:     获取播放文件是否发生变化
  Input:           pclock_info
                   image_info
  Return:          none
**********************************************************************/


int divoom_subscribe_manage_get_change_flag(Pdivoom_clock_id_info pclock_info);

/*********************************************************************
  Function:        divoom_subscribe_manage_close
  Description:     关闭相关句柄信息
  Input:           pclock_info
                   
  Return:          none
**********************************************************************/


void divoom_subscribe_manage_close(Pdivoom_clock_id_info pclock_info);


/*********************************************************************
  Function:        divoom_subscribe_manage_update_play_time
  Description:     更新播放时间
  Input:           pclock_info
                   play_time
                   show_time
  Return:          none
**********************************************************************/


void divoom_subscribe_manage_update_play_time(Pdivoom_clock_id_info pclock_info, int play_time, int show_time);

/*********************************************************************
  Function:        divoom_subscribe_manage_reload_more_gif
  Description:     每天为clock_id=250的多动画表盘更新一次订阅资源
  Input:           mday 当前日期中的日
  Return:          1 已发起当天更新，0 未发起更新
**********************************************************************/

int divoom_subscribe_manage_reload_more_gif(int mday);

/*
 * 当天更新的异步结果账本：网络回包按命令登记实际加入下载批次的文件数，
 * HOT 更新提示页只在全部回包完成后决定退出或进入下载进度页。
 */
void divoom_subscribe_manage_reload_result_add(int command, int32 add_file_num);
int divoom_subscribe_manage_reload_result_get(int32 *add_file_num);

/* 仅 divoom_subscribe_manage_update 发出的匹配回包允许显示下载进度。 */
void divoom_subscribe_manage_transfer_request_add(
    int command, uint32 packet_flag);
int divoom_subscribe_manage_transfer_response_begin(
    int command, uint32 packet_flag);
void divoom_subscribe_manage_transfer_response_defer(void);
void divoom_subscribe_manage_transfer_response_end(void);
int divoom_subscribe_manage_transfer_response_is_active(void);
int divoom_subscribe_manage_transfer_request_is_pending(void);

#endif


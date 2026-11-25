#ifndef _divoom_clock_manage_h_
#define _divoom_clock_manage_h_



/*********************************************************************
  Function:         divoom_clock_manage_init
  Description:      模块初始化
  Input:            
  Return:           none
  Author:           albert
**********************************************************************/

int divoom_clock_manage_init(void);

/*********************************************************************
  Function:         divoom_clock_manage_del_clock
  Description:      删除一个表盘信息
  Input:            pclock_list
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_clock_manage_del_clock(int clock_id);
void divoom_clock_manage_del_clock_app(int clock_id);

/* Channel/MyClockRemove 调用：内置只读表盘忽略，其余表盘按移除时间加入持久 FIFO。 */
int divoom_clock_manage_record_removed_clock(int clock_id);

/* Caller holds the clock-list lock; remove parent/child data without enqueueing. */
int divoom_clock_manage_clear_clock_data_direct(int clock_id);

/* Persist non-default IDs before deleting base config; also works before init. */
int divoom_clock_manage_record_removed_clock_list(const int *ids, int count);

/* 表盘重新加入“我的表盘”时取消延迟删除，以便直接复用尚未回收的数据。 */
void divoom_clock_manage_cancel_removed_clock(int clock_id);

/*
 * 下载前调用；/userdata 剩余空间低于 DIVOOM_ROOM_DEL_CLOCK_DATA（KB）时，
 * 按 FIFO 清理到空间达标或队列为空；失败项保留重试，活跃引用的资源不清理。
 * 返回 1 表示本轮已回收且记录保存成功，0 表示无需回收或未完整成功。
 */
int divoom_clock_manage_clear_removed_clock_data_if_needed(void);

/* Low-space explicit removal: drain the deferred queue, then update Full once. */
int divoom_clock_manage_clear_all_removed_clock_data(void);

/*********************************************************************
  Function:         divoom_clock_manage_update_clock_item
  Description:      更新一个表盘信息
  Input:            pclock_list
                    info_type   0:保存表盘基本信息，1：保存表盘APP配置; 2:全部
  Return:           none
  Author:           albert
**********************************************************************/

int divoom_clock_manage_update_clock_item(Pdivoom_clock_manage_item pclock_list, int info_type,uint8 lock_flag);
/* 与图片下载结果回调参数一致。调用方传入，表盘模块不点名页面。 */
typedef void divoom_clock_manage_nodownloaded_font_callback(int32 clock_id, uint32 model_id,
    uint32 image_id, int32 result);
/* 表盘列表缺预览图：拉最新 cfg，回包只登记这一张预览 jpg。ids 最多处理 8 个。done 在 jpg 结束时调用。
 * 返回实际发出的个数，并写入 sent_ids。没网或发不了时返回 -1，sent_ids 可为 NULL。 */
int divoom_clock_manage_request_review_thumbs(const int *ids, int n, int *sent_ids,
    divoom_clock_manage_nodownloaded_font_callback *done);


void divoom_clock_manage_free_clock(Pdivoom_clock_manage_item pitem_info);


/*********************************************************************
  Function:         divoom_clock_manage_get_clock_info
  Description:      返回表盘的相关配置信息
  Input:            clock_id
                    
  Return:           表盘结构
  Author:           albert
**********************************************************************/

Pdivoom_clock_manage_item divoom_clock_manage_get_clock_info(int clock_id, int clock_pos, int parent_id);

/*********************************************************************
  Function:         divoom_clock_manage_get_clock_info
  Description:      返回表盘的相关配置信息，如果本地没有，将向返回空
  Input:            clock_id
                    
  Return:           表盘结构
  Author:           albert
**********************************************************************/

Pdivoom_clock_manage_item divoom_clock_manage_get_raw_clock_info(int clock_id, int clock_pos, int parent_id);

/*********************************************************************
  Function:         divoom_clock_manage_move_clock
  Description:      更新所有表盘列表，如果有过期的表盘，将重新一个一个获取
  Input:            pclock_list
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_clock_manage_update_clock_list_time(Pdivoom_devcice_time_dial_clock_list_respone pclock_list);

/*********************************************************************
  Function:         divoom_clock_manage_get_all_default_clock_list_time
  Description:      获取所有的默认配置 
  Input:            pclock_list
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_clock_manage_get_all_default_clock_list_time(Pdivoom_devcice_time_dial_clock_list_respone pclock_list);

/*********************************************************************
  Function:         divoom_clock_manage_update_loop
  Description:      循环请求时钟表盘信息
  Input:            
                    
  Return:           NONE
  Author:           albert
**********************************************************************/

void divoom_clock_manage_update_loop(void);

/*********************************************************************
  Function:         divoom_clock_manage_get_device_image_id
  Description:      返回表盘对应的图片ＩＤ
  Input:            clock_id       
                    
  Return:           NONE
  Author:           albert
**********************************************************************/

int divoom_clock_manage_get_device_image_id(int clock_id);

/*********************************************************************
  Function:         divoom_clock_manage_get_disp_item_image
  Description:      通过表盘ＩＤ，得到表盘相关的显示图片ＩＤ
  Input:            clock_id
                    disp_type
  Return:           none
  Author:           albert
**********************************************************************/

int divoom_clock_manage_get_disp_item_image(int clock_id, int disp_type);

/*********************************************************************
  Function:         divoom_clock_manage_check_user_pic_config
  Description:      检查自定义图片种类是否扩充了
  Input:            none
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_clock_manage_check_user_pic_config(void);

/*********************************************************************
  Function:         divoom_clock_manage_refresh_token
  Description:      更新表盘的token信息
  Input:            clock_id
                    phead_info
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_clock_manage_refresh_token(int clock_id, int clock_pos, int parent_id,Pdivoom_http_client_url_head_info phead_info);


/*********************************************************************
  Function:         divoom_clock_manage_update_temp_clock_config
  Description:      用户发送临时表盘信息这类表盘信息可以不保存，
  Input:            
                    
  Return:           NONE
  Author:           albert
**********************************************************************/

int divoom_clock_manage_update_temp_clock_config(PDivoomSystemConfigJson pClockConfig);

/*********************************************************************
  Function:         divoom_clock_manage_get_divoom_simu_data
  Description:      获取用户模拟表盘数据，
  Input:            
                    
  Return:           模拟数据JSON
  Author:           albert
**********************************************************************/

char *divoom_clock_manage_get_divoom_simu_data(void);

/*********************************************************************
  Function:         divoom_clock_manage_get_divoom_simu_info
  Description:      获取用户模拟表盘数据，
  Input:            
                    
  Return:           模拟数据是否成功
  Author:           albert
**********************************************************************/

int divoom_clock_manage_get_divoom_simu_info(divoom_clock_control_disp_data *pdisp_data);
/*********************************************************************
  Function:         divoom_clock_manage_device_update_clock_config
  Description:      设备更新表盘配置，本地保存，并且通知服务器
  Input:            clock_id
                    item_id
                    item_value
                    item_name
                    
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_clock_manage_device_update_clock_config(int clock_id, char *item_id, char *item_value, char *item_name);

int divoom_clock_manage_update_clock_config(Pdivoom_clock_manage_item pclock_list, uint8 lock_flag);
/*********************************************************************
  Function:        divoom_clock_manage_get_url_addr
  Description:     获取指定文件的网络地址
  Input:           model_id
                   image_id
                   url_addr
                    
  Return:          image_flag
**********************************************************************/

int divoom_clock_manage_get_url_addr(int model_id, int image_id, char *url_addr, int *file_type);

/*********************************************************************
  Function:        divoom_clock_manage_get_clock_disp_item_url_addr
  Description:     获取表盘显示元素的网址
  Input:           model_id
                   image_id
                   url_addr
                    
  Return:          image_flag
**********************************************************************/

int divoom_clock_manage_get_clock_disp_item_url_addr(Pdivoom_clock_id_info pclock_id, int image_id, char *url_addr, int *file_type);

/*********************************************************************
  Function:         divoom_clock_manage_get_last_clock_update_time
  Description:      得到表盘最后更新时间
  Input:            none
  Return:           updatetime
  Author:           albert
**********************************************************************/

int divoom_clock_manage_get_last_clock_update_time(void);


/*********************************************************************
  Function:         divoom_clock_manage_get_last_sys_clock_update_time
  Description:      得到系统信息表盘最后更新时间
  Input:            none
  Return:           updatetime
  Author:           albert
**********************************************************************/

int divoom_clock_manage_get_last_sys_clock_update_time(void);


/*********************************************************************
  Function:         divoom_clock_manage_update_sys_clock_list_time
  Description:      更新基础信息表盘列表，如果有过期的表盘，将重新一个一个获取
  Input:            pclock_list
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_clock_manage_update_sys_clock_list_time(Pdivoom_devcice_time_dial_clock_list_respone pclock_list);

/*********************************************************************
  Function:        divoom_clock_manage_check_diy_clock_id
  Description:     检查这个表盘是否是DIY表盘
  Input:           clock_id
                    
  Return:          1 or 0
**********************************************************************/

int divoom_clock_manage_check_diy_clock_id(int clock_id);

/*********************************************************************
  Function:         divoom_clock_manage_get_diy_image_file_id
  Description:      获取某个DIY显示的图像地址
  Input:            config
                    disp_type   
                    file_addr
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_clock_manage_get_diy_image_file_id(char *config, int disp_type, char *file_addr);
/*********************************************************************
  Function:         divoom_clock_manage_reset_all_app_config
  Description:      用户表盘复位功能
  Input:            
  Return:           none
  Author:           albert
**********************************************************************/
void divoom_clock_manage_reset_all_app_config(void);

void divoom_clock_manage_add_download_image(int  model_id, int image_id, char *file_addr, int image_type);

/* 缺失资源先上报 PENDING，下载任务结束后再上报成功或失败终态。 */
#define DIVOOM_CLOCK_MANAGE_DOWNLOAD_PENDING (-1)
/*********************************************************************
  Function:         divoom_clock_manage_check_clock_all_file
  Description:      检查所有资源是否全部下载完，包括子表盘的资源
  Input:            clock_id
                    
  Return:           1：全部字体下载完；0：还有部分字体未下载
  Author:           albert
**********************************************************************/

int divoom_clock_manage_check_clock_all_file(int clock_id, divoom_clock_manage_nodownloaded_font_callback pcallback);
/* 回调 owner 与表盘 ID 分离，供下载会话隔离迟到的资源结果。 */
int divoom_clock_manage_check_clock_all_file_with_owner(int clock_id, int owner_id,
    divoom_clock_manage_nodownloaded_font_callback pcallback);
/* 歌词备用字库按表盘选择，资源检查和显示共用同一规则。 */
int32 divoom_clock_manage_get_lyric_fallback_font_id(int32 clock_id);
int divoom_clock_manage_local_clock_config_exists(int clock_id);
/* Read only Config's 480_480_DialVideo; output is decoded and bounded. */
int divoom_clock_manage_get_intro_video_addr(Pdivoom_clock_manage_item info,
    char *addr, uint32 capacity);
Pdivoom_clock_manage_item divoom_clock_manage_get_clock_info_no_download(int clock_id, int clock_pos, int parent_id);
void divoom_clock_manage_reset_local_sys_clock_disk(int clock_id);

#endif


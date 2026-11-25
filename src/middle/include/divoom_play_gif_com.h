#ifndef _divoom_play_gif_com_h_

#define _divoom_play_gif_com_h_



/*********************************************************************
  Function:        divoom_play_gif_com_init
  Description:     模块初始化
  Input:           insert_time   插入显示时间的间隔,为0将不显示时间
                   gif_time      动画显示最少时间
                   pic_id        开始播放的内置图片ＩＤ
  Return:          返回句柄
**********************************************************************/

void divoom_play_gif_com_init(int insert_time, int gif_time, int pic_id);

/*********************************************************************
  Function:        divoom_play_gif_com_update_disp_time
  Description:     更新插入显示时间的间隔
  Input:           insert_time   插入显示时间的间隔
  Return:          返回句柄
**********************************************************************/

void divoom_play_gif_com_update_disp_time(int insert_time);

/*********************************************************************
  Function:        divoom_play_gif_com_set_single_image_buffer
  Description:     标准的播放某个固定的ＢＵＦＦＥＲ的数据,像提醒什么的
  Input:           handle
                   image_size, 16,32,64, 
                   image_buf  图片的ＲＧＢ　ＢＵＦ
                   speed      播放速度，以ms为单位
                   image_num  图片数目
                   tol_time   播放总时间
                   pcallback  播放完成的回调
  Return:           NONE
**********************************************************************/

void divoom_play_gif_com_set_single_image_buffer(int image_size, uint8 *image_buf,
                                          int speed, int image_num, int tol_time,DIVOOM_DISP_CTRL_OVER_CALL_END *pcallback);


/*********************************************************************
  Function:        divoom_play_gif_com_update_text
  Description:     更新字休信息
  Input:           handle
                   ptext_info
  Return:           NONE
**********************************************************************/

int divoom_play_gif_com_update_text(Pdivoom_draw_set_info_json_image_info ptext_info);

/*********************************************************************
  Function:        divoom_play_gif_com_set_part_image_buffer
  Description:     设置局部图的相关信息
  Input:           handle
                   image_id   局部区域的标号
                   start_x    图片在大图中x的位置
                   start_y    图片在大图中y的位置
                   width      图片在大图中的宽度
                   height     图片在大图中的高度
                   image_buf  图片的缓冲区
                   speed      播放速度，以ms为单位
                   image_num  图片数目
                   layer      图层
  Return:           NONE
**********************************************************************/

void divoom_play_gif_com_set_part_image_buffer(int image_id, int start_x, int start_y, int width, int height, 
                                                uint8 *image_buf,int speed, int image_num, int layer);


/*********************************************************************
  Function:        divoom_play_gif_com_create_image_area
  Description:     创建局部图的相关信息
  Input:           handle
                   image_id   局部区域的标号
                   pic_id     图片的ＩＤ,这个ＩＤ要能在divoom_manage_image_read中得到相关数据才行
                   start_x    图片在大图中x的位置
                   start_y    图片在大图中y的位置
                   image_size 图片宽度
                   layer      图层
                   pcallback  获取下一个pic_id
  Return:           NONE
**********************************************************************/

void divoom_play_gif_com_create_image_area(int image_id, int start_x, int start_y, 
                                      int image_size, int layer, DIVOOM_PLAY_GIF_COM_GET_NEW_ID pcallback,DIVOOM_DISP_CTRL_OVER_CALL_END *povercallback);



/*********************************************************************
  Function:        divoom_play_gif_com_set_text
  Description:     设置局部文字播放相关信息
  Input:           ptext_info
  Return:           NONE
**********************************************************************/

void divoom_play_gif_com_set_text(Pdivoom_text_info ptext_info);



/*********************************************************************
  Function:        divoom_play_gif_com_exit_image
  Description:     模块退出，释放所有资源
  Input:           
  Return:          none
**********************************************************************/

Pdivoom_http_download_image_file_info divoom_play_gif_com_exit_image(void);


/*********************************************************************
  Function:        divoom_play_gif_com_get_play_image_id
  Description:     得到播放的ＩＤ
  Input:           
  Return:          none
**********************************************************************/

int divoom_play_gif_com_get_play_image_id(void);



/*********************************************************
  Function:		divoom_play_com_convert_rgb_net_to_local
  Description:	将网络上的ＲＧＢ排法转换为设备显示的大图
  Input:			width           图像宽度
                    src_pic_buf     网络图片的数据
                    rgb_buf         转换可用的ＢＵＦ
  Return:			none
************************************************************/

void divoom_play_com_convert_rgb_net_to_local(int width, uint8 *src_pic_buf, uint8 * rgb_buf);

/*********************************************************************
  Function:        divoom_play_gif_com_add_image_handle
  Description:     更新播放句柄
  Input:           pimage_handle
                   
  Return:           NONE
**********************************************************************/

void divoom_play_gif_com_add_image_handle(void *pimage_handle);

/*********************************************************************
  Function:        divoom_play_gif_com_clear_all_cache
  Description:     清除所有的缓冲区
  Input:           none
  Return:          none
**********************************************************************/

void divoom_play_gif_com_clear_all_cache(void);

typedef void DIVOOM_PLAY_GIF_COM_RESET_CALLBACK(void *data);
/*********************************************************************
  Function:        divoom_play_gif_com_reset_all_source
  Description:     将复位所有的资源
  Input:           none
  Return:          none
**********************************************************************/

int divoom_play_gif_com_reset_all_source(DIVOOM_PLAY_GIF_COM_RESET_CALLBACK *pcallback, void *data, int free_flag);
void divoom_play_gif_com_wait_notice_start(void);
void divoom_play_gif_com_wait_notice_end(void);

/*********************************************************************
  Function:        divoom_play_gif_com_ctrl_time_frame
  Description:     是否增加时间显示//只有动画才使用这个功能，默认ＩＤ为3;
  Input:           on_off   开关选项
  Return:          返回句柄
**********************************************************************/
void divoom_play_gif_com_ctrl_time_frame(int on_off);

/*********************************************************************
  Function:        divoom_play_gif_com_update_each_gif_time
  Description:     更新单个动画的显示时间
  Input:           gif_time
                   
  Return:          none
**********************************************************************/

void divoom_play_gif_com_update_each_gif_time(int gif_time);
/*********************************************************
  Function:		divoom_play_gif_com_send_part_image
  Description:	将网络上的ＲＧＢ部分图做为局部发送
  Input:			pimage_base           
                    pimage_handle     
                    flag         1要保持底色 0不保证保持底色 
                    start       起始帧
                    num         总帧数，如果是-１，就是全部
  Return:			none
************************************************************/

void divoom_play_gif_com_send_part_image(Pdivoom_image_info pimage_base, Pdivoom_http_download_image_file_info pimage_handle, uint8 flag, int start, int num);

/*********************************************************
  Function:		divoom_play_gif_com_send_part_image_list
  Description:	将多组网络上的ＲＧＢ部分图做为局部发送
  Input:			pimage_base           
                    pimage_handle     
                    flag        1要保持底色 0不保证保持底色
                    start_list       起始帧列表
                    num         总帧数，如果是-１，就是全部
                    list_num    总共有多少组
  Return:			none
************************************************************/

void divoom_play_gif_com_send_part_image_list(Pdivoom_image_info pimage_base, Pdivoom_http_download_image_file_info pimage_handle, uint8 flag, int *start_list, int num, int list_num);

/*********************************************************
  Function:		divoom_play_gif_com_set_part_image_callback
  Description:	设置局部图的回调函数
  Input:			id           
                    pimage_handle     
                    pover_callback
  Return:			none
************************************************************/

void divoom_play_gif_com_set_part_image_callback(int id, DIVOOM_DISP_CTRL_OVER_CALL_END *pover_callback);

#endif


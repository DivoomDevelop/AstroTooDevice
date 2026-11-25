#ifndef _divoom_load_movie_h_
#define _divoom_load_movie_h_
/*********************************************************
  Function:		divoom_load_movie_open_write
  Description:		打开文件句柄写
  Input:			start_page          在ＦＬＡＳＨ上的页地址?
  ????     
  Return:			返回文件句柄
************************************************************/

void *divoom_load_movie_open_write(uint32 start_page, uint32 file_size);

/*********************************************************
  Function:		divoom_load_movie_write
  Description:		打开文件句柄写
  Input:			handle?句柄
  ????     
  Return:			返回文件句柄
************************************************************/

uint32 divoom_load_movie_write(void *handle, uint8 *data, uint16 len);

/*********************************************************
  Function:		divoom_load_movie_open_write
  Description:		关闭写
  Input:			handle?句柄
  ????     
  Return:			返回文件句柄
************************************************************/

void divoom_load_movie_close_write(void *handle, uint8 *buf, uint16 len);

/*********************************************************
  Function:		divoom_load_movie_open_read
  Description:		打开文件句柄
  Input:			start_page          在ＦＬＡＳＨ上的页地址?
  ????     
  Return:			返回文件句柄
************************************************************/

void *divoom_load_movie_open_read(uint32 type, uint32 index,uint8 *buf, uint32 image_size);



/*********************************************************
  Function:		divoom_load_movie_get_next_frame_len
  Description:		得到下一帧数据的长度
  Input:			handle?句柄
  ????   buf          
                          len
  Return:			
************************************************************/

uint16 divoom_load_movie_get_next_frame_len(void *handle);

/*********************************************************
  Function:		divoom_load_movie_load
  Description:		读一张图片
  Input:			handle?句柄
  ????   buf          
                          len
  Return:			
************************************************************/

uint16 divoom_load_movie_load(void *handle, uint8 *buf, uint8 *end_flag);


/*********************************************************
  Function:		divoom_load_movie_read_part
  Description:		顺序读数据
  Input:			handle:句柄
                    buf    
                    read_len
                          len
  Return:			
************************************************************/

uint16 divoom_load_movie_read_part(void *handle, uint8 *buf, uint16 read_len);


/*********************************************************
  Function:		divoom_load_movie_get_pic_off
  Description:		得到当前的偏移
  Input:			handle?句柄
  Return:			
************************************************************/

uint32 divoom_load_movie_get_pic_off(void *handle);



/*********************************************************
  Function:		divoom_load_movie_set_image_size
  Description:		设置当前图像的空间大小
  Input:			handle?句柄
  Return:			
************************************************************/

void divoom_load_movie_set_image_size(void *handle, uint32 size);

/*********************************************************
  Function:		divoom_load_movie_set_image_range
  Description:		设置当前图像的空间范围
  Input:			handle?句柄
  Return:			
************************************************************/

void divoom_load_movie_set_image_range(void *handle, uint32 start, uint32 end);

/*********************************************************
  Function:		divoom_load_movie_set_pic_off
  Description:		设置当前的偏移
  Input:			handle?句柄
  Return:			
************************************************************/

void divoom_load_movie_set_pic_off(void *handle, uint32 off);

/*********************************************************
  Function:		divoom_load_movie_close
  Description:		使用完文件句柄，关闭句柄
  Input:			handle?句柄
  Return:			
************************************************************/

void divoom_load_movie_close(void *handle);

/*********************************************************
  Function:		divoom_load_movie_set_pos
  Description:		将读句柄指针回头
  Input:			handle?句柄
  Return:			
************************************************************/

void divoom_load_movie_reset(void *handle);


/*********************************************************
  Function:		divoom_load_movie_get_pic_num
  Description:		得到总共的图片张图
  Input:			handle?句柄
  Return:			
************************************************************/

uint8 divoom_load_movie_get_pic_num(void *handle);


/*********************************************************
  Function:		divoom_load_movie_get_pic_info
  Description:		读出第Ｎ帧数据
  Input:			handle?句柄
  Return:			
************************************************************/

uint8 divoom_load_movie_get_pic_info(void *handle, uint16 pic_id, uint16 *pic_len, uint8 *pic_data);


/*********************************************************
  Function:		divoom_load_movie_get_pic_data
  Description:		读出第Ｎ帧数据
  Input:			handle?句柄
  Return:			
************************************************************/

uint8 divoom_load_movie_get_pic_data(void *handle, uint16 pic_id, uint16 *pic_len, uint8  *pic_data);

/*********************************************************
  Function:		divoom_load_movie_check_single_pic
  Description:	检查数据是否为单张动画
  Input:			handle  句柄
  Return:		是否是单张图片	
************************************************************/

uint8 divoom_load_movie_check_single_pic(void *handle);

/*********************************************************
  Function:		divoom_load_movie_get_read_cache_pic_num
  Description:		得到已经加载的图片张图
  Input:			handle 句柄
  Return:			
************************************************************/

uint8 divoom_load_movie_get_read_cache_pic_num(void *handle);

/*********************************************************
  Function:		divoom_load_movie_set_read_cache_pic_num
  Description:		重置已经加载的图片张图
  Input:			handle 句柄
  Return:			
************************************************************/

void divoom_load_movie_set_read_cache_pic_num(void *handle);

/*********************************************************
  Function:		divoom_load_movie_load_first_pic
  Description:		读第一张图片
  Input:			handle 句柄
                    buf          
                    word_flag 是否有文字帧
                    image_size   图片的大小
  Return:		读数据的长度	,如果为0 读失败
************************************************************/

uint16 divoom_load_movie_load_first_pic(void *handle, uint8 *buf, uint8 *word_flag, uint16 *image_size);


void divoom_load_movie_anomaly_power_on(void);


/*********************************************************
  Function:		divoom_load_movie_get_pic_size
  Description:		得到当前的文件大小
  Input:			handle?句柄
  Return:			
************************************************************/

uint32 divoom_load_movie_get_pic_size(void *handle);

#endif


#ifndef _divoom_zstd_manage_h_

#define _divoom_zstd_manage_h_


/*********************************************************************
  Function:        divoom_zstd_manage_create_zstd_handle
  Description:     创建句柄
  Input:           file_handle      原文件句柄   
                   image_w     图像宽度
                   tol_len      数据总长度
  Return:          none
**********************************************************************/
typedef int divoom_zstd_manage_read_callback(void *handle, uint8 *buf, int read_size);
typedef int divoom_zstd_manage_read_at_callback(uint8 *buf, int offset, int read_size);

void *divoom_zstd_manage_create_zstd_handle(void *file_handle, uint8 *buf,int image_w, int tol_len, divoom_zstd_manage_read_callback *pread_call, void *read_handle);

/*********************************************************************
  Function:        divoom_zstd_manage_create_zstd_handle_at
  Description:     用可随机访问的数据源创建流式句柄；循环播放时从 source_offset 回卷
  Input:           pread_at_call  按绝对偏移读取，成功返回实际读取字节数
                   source_offset  ZSTD 数据在来源中的起始偏移
                   image_w        图像宽度
                   tol_len        ZSTD 数据长度
  Return:          成功返回句柄，失败返回 NULL
**********************************************************************/
void *divoom_zstd_manage_create_zstd_handle_at(int image_w, int tol_len,
    divoom_zstd_manage_read_at_callback *pread_at_call, int source_offset);


/*********************************************************************
  Function:        divoom_zstd_manage_decode_rgb_buf
  Description:     读出一帧数据,输出的数据是RGB565
  Input:           pzstd_handle   
  
  Return:          none
**********************************************************************/

uint8 *divoom_zstd_manage_decode_rgb_buf(void *handle, uint8 *rgb_buf);

/*********************************************************************
  Function:        divoom_zstd_manage_reset_stream
  Description:     将 ZSTD 流回卷到起始帧，供按需单帧解码时能量下降使用
  Input:           handle
  Return:          0 成功，-1 失败
**********************************************************************/

int divoom_zstd_manage_reset_stream(void *handle);

/*********************************************************************
  Function:        divoom_zstd_manage_set_sync_read_mode
  Description:     同步按需读帧：禁止 worker 预取、禁止压缩流 EOF 回卷
  Input:           handle, sync_read_mode
  Return:          none
**********************************************************************/

void divoom_zstd_manage_set_sync_read_mode(void *handle, int sync_read_mode);

/*********************************************************************
  Function:        divoom_zstd_manage_idle_read
  Description:     停止预取并清空 read 队列中未消费的帧
  Input:           handle
  Return:          none
**********************************************************************/

void divoom_zstd_manage_idle_read(void *handle);

/*********************************************************************
  Function:        divoom_zstd_manage_destroy
  Description:     释放所有资源
  Input:           handle   
  
  Return:          none
**********************************************************************/

void divoom_zstd_manage_destroy(void *handle );

/*********************************************************************
  Function:        divoom_zstd_manage_decode_one_frame
  Description:     将zstd的缓冲区解码RGB数据输出,输出的数据是RGB565
  Input:           zstd_buf   
                   buf_szie
                   rgb_buf
                   image_width 
  Return:          none
**********************************************************************/

void divoom_zstd_manage_decode_one_frame(uint8 *zstd_buf, int buf_szie, uint8 *rgb_buf, int image_width);

#endif


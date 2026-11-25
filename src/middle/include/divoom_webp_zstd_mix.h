#ifndef _divoom_webp_zstd_mix_h_
#define _divoom_webp_zstd_mix_h_


typedef int divoom_webp_zstd_mix_read_callback(void *handle, uint8 *buf, int read_size);
typedef int divoom_webp_zstd_mix_read_at_callback(uint8 *buf, int offset, int read_size);

/*********************************************************************
  Function:        divoom_webp_zstd_mix_create_zstd_handle
  Description:     创建句柄
  Input:           file_handle      原文件句柄   
                   buf              缓冲区BUF
                   image_w     图像宽度
                   tol_len      数据总长度
                   image_num        图片总数目
  Return:          none
**********************************************************************/

void *divoom_webp_zstd_mix_create_zstd_handle(void *file_handle, uint8 *buf, int image_w, int image_num, divoom_webp_zstd_mix_read_callback *pread_callback, void *read_handle);

/*********************************************************************
  Function:        divoom_webp_zstd_mix_create_zstd_handle_at
  Description:     用可随机访问来源创建混合动画句柄，仅缓存一帧压缩数据
  Input:           image_w        图像宽度
                   image_num      图片数目
                   pread_at_call  按绝对偏移读取，返回实际读取字节数
                   source_offset  第一帧类型字节的起始偏移
  Return:          成功返回句柄，失败返回 NULL
**********************************************************************/
void *divoom_webp_zstd_mix_create_zstd_handle_at(int image_w, int image_num,
    divoom_webp_zstd_mix_read_at_callback *pread_at_call, int source_offset);

/*********************************************************************
  Function:        divoom_zstd_manage_decode_rgb_buf
  Description:     读出一帧数据
  Input:           phandle   
  
  Return:          none
**********************************************************************/

void divoom_webp_zstd_mix_decode_rgb_buf(void *handle, uint8 *rgb_buf);


/*********************************************************************
  Function:        divoom_zstd_manage_destroy
  Description:     释放所有资源
  Input:           handle   
  
  Return:          none
**********************************************************************/

void divoom_webp_zstd_mix_destroy(void *handle );

#endif




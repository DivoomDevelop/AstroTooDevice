#ifndef _divoom_thumb_cache_h_
#define _divoom_thumb_cache_h_

#define DIVOOM_THUMB_PIC_WIDTH (140)
#define DIVOOM_THUMB_PIC_HIGH (140)
#define DIVOOM_THUMB_CACHE_RGB565_SIZE (DIVOOM_THUMB_PIC_WIDTH*DIVOOM_THUMB_PIC_HIGH*2)
#define DIVOOM_THUMB_CACHE_PIC_SIZE (DIVOOM_THUMB_PIC_WIDTH*DIVOOM_THUMB_PIC_HIGH*3)
/* v2 使用 BGRA 编码契约，隔离旧版本已生成的 R/B 互换缩略图。 */
#define DIVOOM_THUMB_BASE_PATH DIVOOM_THUMB_INFO_DIR


/*********************************************************************
  Function:        divoom_thumb_cache_init
  Description:     模块初始化
  Input:           max_item     缓冲区最大缓冲缩图的数目
  Return:          none
**********************************************************************/

void divoom_thumb_cache_init(int max_item);
/*********************************************************************
  Function:        divoom_thumb_cache_set_clock_info
  Description:     设置表盘信息
  Input:           pclock_info     
  Return:          none
**********************************************************************/

void divoom_thumb_cache_set_clock_info(Pdivoom_clock_id_info pclock_info);

/*********************************************************************
  Function:        divoom_thumb_cache_set_dest_pic_size
  Description:     设置目标缓冲区照片的大小
  Input:           pic_w     
                   pic_h
  Return:          none
**********************************************************************/

void divoom_thumb_cache_set_dest_pic_size(int pic_w, int pic_h);


/*********************************************************************
  Function:        divoom_thumb_cache_read_pic
  Description:     读取缩略图RGB565A8数据，此函数不能阻塞，如果数据未准备好，将直接返回，再由回调通知一下
  Input:           list_id   第几列
                   pic_id
                   rgb_buf
                   pcallback
                   para
  Return:          1:成功读取  0：未成功读取
**********************************************************************/

int divoom_thumb_cache_read_pic(int list_id, int pic_id, int asyn_flag, uint8 *rgb_buf, int image_w, int image_h, DIVOOM_PIC_ALBUM_MANAGE_READ_OVER *pcallback, void *para);

/*********************************************************************
  Function:        divoom_thumb_cache_clear_pic
  Description:     删除缩略图缓冲区，并删除缩图文件
  Input:           pic_id
                   list_id
  Return:          none
**********************************************************************/

void divoom_thumb_cache_clear_pic(int pic_id);

/*********************************************************************
  Function:        divoom_thumb_cache_clear_wait
  Description:     清除预加载图片的缩略图队列，这个一般在用户滑动方向变化时调用
  Input:           list_id
  Return:          none
**********************************************************************/

void divoom_thumb_cache_clear_wait(int list_id);

/*********************************************************************
  Function:        divoom_thumb_cache_pre_read
  Description:     预加载图片的缩略图，这个方便后面读取提速，由应用层提前调用
  Input:           pic_id
  Return:          none
**********************************************************************/

void divoom_thumb_cache_pre_read(int list_id, int pic_id);

/*********************************************************************
  Function:        divoom_thumb_cache_clear_all
  Description:     清除这个模块的所有申请的内存，让队列为空,在退出缩略图的使用场景情况下调用
  Input:           none
  Return:          none
**********************************************************************/

void divoom_thumb_cache_clear_all(void);
/*********************************************************************
  Function:        divoom_thumb_cache_get_thumb_name
  Description:     通过文件名得到缩略图的文件名
  Input:           pic_name
                   thumb_name
                   
  Return:          none
**********************************************************************/

void divoom_thumb_cache_get_thumb_name(char *pic_name, char *thumb_name);

#endif



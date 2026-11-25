#ifndef _divoom_gif_save_h_
#define _divoom_gif_save_h_

/*********************************************************
  Function:		divoom_gif_save_init
  Description:		针对发送固定数据大小，带偏移的数据接受
  Input:			start_page       
                           tol_size
                           flash_item
  Return:			
************************************************************/


void *divoom_gif_save_init(uint8 type, uint32 file_index, uint32 file_size);


/*********************************************************
  Function:		divoom_gif_save_write
  Description:		按规定位置写数据
  Input:			handle
                           buf
                           len
                           offset    当前偏移
  Return:			
************************************************************/

uint32 divoom_gif_save_write(void *handle, uint8 *buf, uint16 len, uint32 offset);



/*********************************************************
  Function:		divoom_gif_save_exit
  Description:		退出当前保存操作
  Input:			handle
  Return:			
************************************************************/


void divoom_gif_save_exit(void *handle, uint8 *attr_buf, uint16 attr_len);



#endif



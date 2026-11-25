#ifndef _divoom_mul_gif_ctrl_h_
#define _divoom_mul_gif_ctrl_h_

typedef uint32 DIVOOM_MUL_GIF_CTRL_OVER_CALL_BACK(uint8 type,uint32 file_index);

/*********************************************************
  Function:		divoom_mul_gif_ctrl_init
  Description:	启动多组动画的播放,
  Input:			type          
                    file_index
                    pic_id,             前面是否插入图片
                    insert_time
                    offset              起始播放偏移
                    image_size          图像起始大小
  Return:			返回句柄
************************************************************/

void *divoom_mul_gif_ctrl_init(uint8 type, uint32 file_index,uint16 pic_id, uint16 insert_time, uint32 offset, uint32 image_size,
            DIVOOM_MUL_GIF_CTRL_OVER_CALL_BACK *pcallback);


/*********************************************************
  Function:		divoom_mul_gif_ctrl_update_image_size
  Description:	       设置文件大小
  Input:			handle
                    image_size      图像文件的大小
  Return:			none
************************************************************/

void divoom_mul_gif_ctrl_update_image_size(void *handle, uint32 image_size);

/*********************************************************
  Function:		divoom_mul_gif_ctrl_update_disp_time
  Description:	        更新显示时间间隔
  Input:			handle
                        　time      时间
  Return:			none
************************************************************/

void divoom_mul_gif_ctrl_update_disp_time(void *handle, uint32 time);

/*********************************************************
  Function:		divoom_mul_gif_ctrl_exit
  Description:	退出播放逻辑
  Input:			none
  Return:			none
************************************************************/

void divoom_mul_gif_ctrl_exit(void *handle);

/*********************************************************
  Function:		divoom_mul_gif_ctrl_get_disp_offset
  Description:	       得到下一个将要显示的作品的首地址
  Input:			handle
  Return:			偏移
************************************************************/

uint32 divoom_mul_gif_ctrl_get_disp_offset(void *handle);

#endif



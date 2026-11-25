#ifndef _divoom_mul_gif_ctrl_h_
#define _divoom_mul_gif_ctrl_h_

typedef T_U32 DIVOOM_MUL_GIF_CTRL_OVER_CALL_BACK(T_U32 *model_id,T_U32 *file_id, T_U32 *file_off);

/*********************************************************
  Function:		divoom_mul_gif_ctrl_init
  Description:	启动多组动画的播放,
  Input:			start_page          动画在flash 的起始位置
                    pic_id,             前面是否插入图片
                    insert_time
                    offset              起始播放偏移
                    image_size          图像起始大小
  Return:			返回句柄
************************************************************/

T_VOID *divoom_mul_gif_ctrl_init(T_U16 model_id,T_U32 file_id,T_U16 pic_id, T_U16 insert_time, T_U32 offset, 
            DIVOOM_MUL_GIF_CTRL_OVER_CALL_BACK *pcallback, T_U32 gif_continue_time);


/*********************************************************
  Function:		divoom_mul_gif_ctrl_update_image_size
  Description:	       设置文件大小
  Input:			handle
                    image_size      图像文件的大小
  Return:			none
************************************************************/

T_VOID divoom_mul_gif_ctrl_update_image_size(T_VOID *handle, T_U32 image_size);

/*********************************************************
  Function:		divoom_mul_gif_ctrl_update_disp_time
  Description:	        更新显示时间间隔
  Input:			handle
                        　time      时间
  Return:			none
************************************************************/

T_VOID divoom_mul_gif_ctrl_update_disp_time(T_VOID *handle, T_U32 time);
/*********************************************************
  Function:		divoom_mul_gif_ctrl_update_play_time
  Description:	        更新播放动画时间间隔
  Input:			handle
                        　time      时间
  Return:			none
************************************************************/

T_VOID divoom_mul_gif_ctrl_update_play_time(T_VOID *handle, T_U32 time);

/*********************************************************
  Function:		divoom_mul_gif_ctrl_update_image_range
  Description:	        设置当前作品的空间范围
  Input:			handle
                            start
                            end
  Return:			none
************************************************************/

T_VOID divoom_mul_gif_ctrl_update_image_range(T_VOID *handle, T_U32 start, T_U32 end);

/*********************************************************
  Function:		divoom_mul_gif_ctrl_exit
  Description:	退出播放逻辑
  Input:			none
  Return:			none
************************************************************/

T_VOID divoom_mul_gif_ctrl_exit(T_VOID *handle);

/*********************************************************
  Function:		divoom_mul_gif_ctrl_get_disp_offset
  Description:	       得到下一个将要显示的作品的首地址
  Input:			handle
  Return:			偏移
************************************************************/

T_U32 divoom_mul_gif_ctrl_get_disp_offset(T_VOID *handle);

/*********************************************************
  Function:		divoom_mul_gif_ctrl_get_last_pic_offset
  Description:	        得到一组数据的最后一个作品的超始位置
  Input:			start_page          动画在flash 的起始位置
  Return:			返回偏移
************************************************************/

T_U32 divoom_mul_gif_ctrl_get_last_pic_offset(T_U32 start_page);

/*********************************************************
  Function:		divoom_mul_gif_ctrl_update_image_size
  Description:	退出播放逻辑
  Input:			handle
                    image_size      图像文件的大小
  Return:			none
************************************************************/

T_VOID divoom_mul_gif_ctrl_update_image_range(T_VOID *handle, T_U32 start, T_U32 end);

#endif



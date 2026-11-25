#ifndef _divoom_gif_ctrl_com_h_
#define _divoom_gif_ctrl_com_h_


/*********************************************************
  Function:		divoom_gif_ctrl_com_init
  Description:		启动动画的播放
  Input:			start_page          动画在flash 的起始位置
  Return:			NONE
************************************************************/

void *divoom_gif_ctrl_com_init(uint8 type, uint32 file_index,DIVOOM_DISP_CTRL_OVER_CALL_END *pcallback, uint32 init_size);


/*********************************************************
  Function:		divoom_gif_ctrl_com_update_gif_attr
  Description:		设置动画播放的相关属性
  Input:			size          目前动画的实际数量量
  Return:			NONE
************************************************************/

void divoom_gif_ctrl_com_update_gif_attr(void *handle, Pdivoom_disp_ctrl_gif_info pgif_info);


/*********************************************************
  Function:		divoom_gif_ctrl_com_reset_gif_num
  Description:	复位动画读到的张数
  Input:			handle
  Return:			NONE
************************************************************/

void divoom_gif_ctrl_com_reset_gif_num(void *handle);


/*********************************************************
  Function:		divoom_gif_ctrl_com_get_gif_num
  Description:	获取动画读到的张数
  Input:			handle
  Return:			NONE
************************************************************/

uint8 divoom_gif_ctrl_com_get_gif_num(void *handle);


void divoom_gif_ctrl_com_exit(void *handle);

/*********************************************************
  Function:		divoom_gif_ctrl_com_add_black_frame
  Description:	当数据读完后是否加个空帧
  Input:		handle
                speed 
  Return:		NONE
************************************************************/

void divoom_gif_ctrl_com_add_black_frame(void *handle, uint16 speed);

/*********************************************************
  Function:		divoom_gif_ctrl_com_set_read_quick
  Description:	       只有缓冲区为空的时候才读数据，避免阻塞，有其它的特殊要求时，调用这个，像有声音卡
  Input:		handle
  Return:		NONE
************************************************************/

void divoom_gif_ctrl_com_set_read_quick(void *handle);

#endif



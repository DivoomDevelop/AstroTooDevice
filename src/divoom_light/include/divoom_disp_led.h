#ifndef _divoom_disp_led_h_
#define _divoom_disp_led_h_


/*********************************************************
  Function:		divoom_disp_led_init
  Description:		启动文字显示模块
  Input:			start_page
  Return:			NONE
************************************************************/


void divoom_disp_led_init(uint8 type,uint32 file_index, uint32 disp_time);


/*********************************************************
  Function:		divoom_disp_led_read_loop
  Description:		读数据模块
  Input:			data
  Return:			NONE
************************************************************/

void divoom_disp_led_read_loop(void *data);


/*********************************************************
  Function:		divoom_disp_led_close
  Description:		退出文字显示模块
  Input:			none
  Return:			NONE
************************************************************/

void divoom_disp_led_close(void);

/*********************************************************
  Function:		divoom_disp_led_set_callback
  Description:		设置结束回调
  Input:			start_page
  Return:			NONE
************************************************************/

void divoom_disp_led_set_callback(DIVOOM_DISP_CTRL_OVER_CALL_END *pcallback);

#endif



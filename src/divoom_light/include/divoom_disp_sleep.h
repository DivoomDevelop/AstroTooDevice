#ifndef _divoom_disp_sleep_h_
#define _divoom_disp_sleep_h_


/*********************************************************************
  Function:         divoom_disp_sleep_update
  Description:     更新睡眠显示模式
   Input:            user_data   闹钟显示数据
  Author:           divoom
**********************************************************************/

void divoom_disp_sleep_update(void * user_data);




/*********************************************************************
  Function:         divoom_disp_sleep_init
  Description:     
   Input:            user_data   显示数据
  Author:           divoom
**********************************************************************/

void divoom_disp_sleep_init(void * user_data);


/*********************************************************************
  Function:         divoom_disp_sleep_close
  Description:     退出睡眠状态
   Input:            NONE
  Author:           divoom
**********************************************************************/


void divoom_disp_sleep_close(void);


#endif



#ifndef _divoom_disp_single_clock_h_
#define _divoom_disp_single_clock_h_

/*********************************************************************
  Function:         divoom_disp_single_clock_init
  Description:      只是用来显示只有一个GIF元素的表盘
  Input:            data
  Return:           句柄
  Author:           albert
**********************************************************************/

void *divoom_disp_single_clock_init(void *data, int font_id, uint8 lcd_id);
void divoom_disp_single_clock_update(void *handle, int font_id, void *data);
void divoom_disp_single_clock_exit(void *data);

#endif



#ifndef _divoom_disp_event_h_
#define _divoom_disp_event_h_

typedef void DIVOOM_DISP_EVENT_CALLBACK(uint8 mode);

/*********************************************************************
  Function:         divoom_disp_event_fakeout_time
  Description:     谈入谈出时间
  Input:            pctrl_info      是divoom_device_event_image这个结构体
  Author:           divoom
**********************************************************************/
void divoom_disp_event_fakeout_time(uint32 fake_time);

/*********************************************************************
  Function:         divoom_disp_event_exit_callback
  Description:     谈入谈出时间
  Input:            callback
  Author:           divoom
**********************************************************************/
void divoom_disp_event_exit_callback(DIVOOM_LIGHT_DISP_OVER_CALLBACK *pcallback);

/*********************************************************************
  Function:         divoom_disp_event_update
  Description:     更新显示的图片信息
  Input:            pctrl_info      是Pdivoom_disp_event_disp_info这个结构体
  Author:           divoom
**********************************************************************/
void divoom_disp_event_update(Pdivoom_disp_event_disp_info pctrl_info);


/*********************************************************************
  Function:         divoom_disp_event_init
  Description:     显示提示图片，显示完成后，将还原之前的显示
  Input:            pctrl_info      是divoom_device_event_image这个结构体
                      cur_mode      前一模式
  Author:           divoom
**********************************************************************/
void divoom_disp_event_init(void *pctrl_info, uint8 cur_mode);


/*********************************************************************
  Function:         divoom_disp_event_play
  Description:     状态控制
  Input:            status
  Author:           divoom
**********************************************************************/

void divoom_disp_event_play(uint8 status);


/*********************************************************************
  Function:         divoom_disp_event_close
  Description:     释放资源
  Input:            void
  Author:           divoom
**********************************************************************/

uint8 divoom_disp_event_close(void);


/*********************************************************************
  Function:         divoom_disp_event_get_back_mode
  Description:     返回事件通知的前一个模式
  Input:            模式
  Author:           divoom
**********************************************************************/

uint8 divoom_disp_event_get_back_mode(void);


void divoom_disp_event_display_battery(uint8 level, T_BOOL is_charge, uint8 *out_buf);
uint8 Eng_GetVolLevel(void);
#endif



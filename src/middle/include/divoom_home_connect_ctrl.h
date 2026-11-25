#ifndef _divoom_home_connect_ctrl_h_
#define _divoom_home_connect_ctrl_h_
typedef void DIVOOM_HOME_CONNECT_EVENT_NOTICE( int clock_id, int lcd_id,int indepence_index);

/*********************************************************************
  Function:        divoom_home_connect_get_disp_data
  Description:     获取表盘显示数据，如果
                    word        auth token
  Return:           none
  Author:           albert
**********************************************************************/

uint8 divoom_home_connect_get_disp_data(Pdivoom_clock_control_disp_data pdisp_data, int flag);


/*********************************************************************
  Function:        divoom_home_connect_exit_clock
  Description:     显示表盘将退出
                    word        auth token
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_home_connect_exit_clock(void);



/*********************************************************************
  Function:        divoom_home_connect_start_clock
  Description:     用户切换表盘时调用，后面将数据获取时，将获取所有在线的设备数据
                    word        auth token
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_home_connect_start_clock(char *word, int indepence_index, int lcd_id, int clock_id);


/*********************************************************************
  Function:        divoom_home_connect_stop_event
  Description:     用户关闭通知时调用
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_home_connect_stop_event(void);


/*********************************************************************
  Function:        divoom_home_connect_start_event
  Description:     用户启动通知时调用
                    word        auth token
                    pcallback
                    clock_id
                    indepence_index
                    lcd_id
  Return:           none
  Author:           albert
**********************************************************************/
void divoom_home_connect_start_event(char *word, DIVOOM_HOME_CONNECT_EVENT_NOTICE *pcallback, int clock_id, int indepence_index, int lcd_id);

/*********************************************************************
  Function:        divoom_home_connect_update_autho_token
  Description:     更新autho token
                    word        auth token
  Return:           none
  Author:           albert
**********************************************************************/
void divoom_home_connect_update_autho_token(char *word);
void divoom_home_connect_get_data_from_json(char *json_str);

/*********************************************************************
  Function:        divoom_home_connect_cancel_event
  Description:     取消通知事件
  Return:           none
  Author:           albert
**********************************************************************/
void divoom_home_connect_cancel_event(void);

#endif


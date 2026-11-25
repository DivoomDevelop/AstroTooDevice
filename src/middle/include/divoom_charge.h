#ifndef _divoom_charge_h_
#define _divoom_charge_h_
/*********************************************************************
  Function:        divoom_charge_init
  Description:     初始化
  Input:           none
  Return:          none
**********************************************************************/


void divoom_charge_init(void);


/*********************************************************************
  Function:        divoom_charge_get_status
  Description:     获取充电状态
  Input:           none
  Return:          1:充电；0：非充电
**********************************************************************/

int divoom_charge_get_status(void);

/*********************************************************************
  Function:        divoom_charge_get_voltage
  Description:     获取电池的电压
  Input:           none
  Return:          返回mv
**********************************************************************/

int divoom_charge_get_voltage(void);

/*********************************************************************
  Function:        divoom_charge_get_capacity
  Description:     获取电池的电量
  Input:           none
  Return:          返回百分比
**********************************************************************/

int divoom_charge_get_capacity(void);

#endif



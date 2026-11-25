#ifndef _divoom_cache_clock_data_h_
#define _divoom_cache_clock_data_h_

/*********************************************************************
  Function:         divoom_cache_clock_data_init
  Description:      初始化队列
  Input:            
  
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_cache_clock_data_init(void);


/*********************************************************************
  Function:         divoom_cache_clock_data_update
  Description:      更新表盘数据
  Input:            clock_id
                    pdisp_data
  
  Return:           1:数据有更新，　０：数据无更新
  Author:           albert
**********************************************************************/

uint8 divoom_cache_clock_data_update(int clock_id, int clock_pos, int parent_id, Pdivoom_clock_control_disp_data pdisp_data);


/*********************************************************************
  Function:         divoom_cache_clock_data_update
  Description:      更新表盘数据
  Input:            clock_id
                    pdisp_data
  
  Return:           none
  Author:           albert
**********************************************************************/


int divoom_cache_clock_data_get(int clock_id, int clock_pos, int parent_id, Pdivoom_clock_control_disp_data pdisp_data, uint8 flag);

/*********************************************************************
  Function:         divoom_cache_clock_data_remove
  Description:      表盘发生变化，移除表盘数据
  Input:            clock_id
  
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_cache_clock_data_remove(int clock_id, int clock_pos, int parent_id);

/*********************************************************************
  Function:         divoom_cache_clock_data_clear_all
  Description:      清除所有缓冲数据
  Input:            
  
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_cache_clock_data_clear_all(void);


/*********************************************************************
  Function:         divoom_cache_clock_data_work_ctrl
  Description:      缓冲队列是否工作的控制
  Input:            
  
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_cache_clock_data_work_ctrl(int on_off);

#endif



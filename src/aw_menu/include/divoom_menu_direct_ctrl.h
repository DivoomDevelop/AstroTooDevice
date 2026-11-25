#ifndef _divoom_menu_direct_ctrl_h_
#define _divoom_menu_direct_ctrl_h_

enum{
    DIVOOM_MENU_DIRECT_UP,
    DIVOOM_MENU_DIRECT_DOWN,
    DIVOOM_MENU_DIRECT_LEFT,
    DIVOOM_MENU_DIRECT_RIGHT,
};

/*********************************************************************
  Function:        divoom_menu_direct_ctrl_create
  Description:     创建方向控制句柄
  Input:           row      行最大数目
                   col      列最大数目
  Return:          句柄
**********************************************************************/

void *divoom_menu_direct_ctrl_create(int row, int col);

/*********************************************************************
  Function:        divoom_menu_direct_ctrl_update_row_col
  Description:     更新行列信息，这个只能减少，不能增加
  Input:           handle   句柄
                   row      
                   col
  Return:          none
**********************************************************************/

void divoom_menu_direct_ctrl_update_row_col(void *handle, int row, int col);

/*********************************************************************
  Function:        divoom_menu_direct_ctrl_update
  Description:     更新指定坐标的值
  Input:           handle   句柄
                   obj      需要保存的结构体
                   x
                   y
  Return:          none
**********************************************************************/

void divoom_menu_direct_ctrl_update(void *handle, void *obj, int x, int y);

/*********************************************************************
  Function:        divoom_menu_direct_ctrl_get
  Description:     获取某个结构的另一个方向的数据
  Input:           handle   句柄
                   obj      用户数据,divoom_menu_direct_ctrl_update给的数据
                   direct
                   
  Return:          需要的数据
**********************************************************************/

void *divoom_menu_direct_ctrl_get(void *handle, void *obj, int direct);

/*********************************************************************
  Function:        divoom_menu_direct_ctrl_destory
  Description:     释放相关资源
  Input:           handle   句柄
  Return:          none
**********************************************************************/

void divoom_menu_direct_ctrl_destory(void *handle);


/*********************************************************************
  Function:        divoom_menu_direct_ctrl_get_pos
  Description:     返回这个成员所在的位置偏移
  Input:           handle   句柄
                   obj      用户数据,divoom_menu_direct_ctrl_update给的数据
                   
  Return:          位置
**********************************************************************/

int divoom_menu_direct_ctrl_get_pos(void *handle, void *obj);

#endif



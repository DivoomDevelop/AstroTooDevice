#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_direct_ctrl.h"


typedef struct _divoom_menu_direct_ctrl_ctrl
{
    int row;
    int col;
    void **obj_tbl;
}divoom_menu_direct_ctrl_ctrl, *Pdivoom_menu_direct_ctrl_ctrl;


/*********************************************************************
  Function:        divoom_menu_direct_ctrl_update_row_col
  Description:     更新行列信息，这个只能减少，不能增加
  Input:           handle   句柄
                   row      
                   col
  Return:          none
**********************************************************************/

void divoom_menu_direct_ctrl_update_row_col(void *handle, int row, int col)
{
    Pdivoom_menu_direct_ctrl_ctrl phandle = (Pdivoom_menu_direct_ctrl_ctrl)handle;

    
    phandle->row = row;
    phandle->col = col;
}

/*********************************************************************
  Function:        divoom_menu_direct_ctrl_create
  Description:     创建方向控制句柄
  Input:           row      行最大数目
                   col      列最大数目
  Return:          句柄
**********************************************************************/

void *divoom_menu_direct_ctrl_create(int row, int col)
{
    Pdivoom_menu_direct_ctrl_ctrl phandle;

    phandle = divoom_system_malloc(sizeof(divoom_menu_direct_ctrl_ctrl) + row*col*sizeof(void *));
    if(phandle == NULL)
    {
        return NULL;
    }
    memset(phandle, 0, sizeof(divoom_menu_direct_ctrl_ctrl) + row*col*sizeof(void  *));
    phandle->obj_tbl = (void **)&phandle[1];
    phandle->row = row;
    phandle->col = col;
    return phandle;
}

/*********************************************************************
  Function:        divoom_menu_direct_ctrl_update
  Description:     更新指定坐标的值,对于Ｘ是顺序的，从０到开始，最后一个不一定是row-1
  Input:           handle   句柄
                   obj      需要保存的结构体
                   x
                   y
  Return:          none
**********************************************************************/

void divoom_menu_direct_ctrl_update(void *handle, void *obj, int x, int y)
{
    Pdivoom_menu_direct_ctrl_ctrl phandle = (Pdivoom_menu_direct_ctrl_ctrl)handle;

    if(handle == NULL)
    {
        return;
    }
    if(x >= phandle->row || y >= phandle->col)
    {
        divoom_print("err pos: x:%d, y: %d, row: %d, col: %d\n", x, y, phandle->row, phandle->col);
        return;
    }
    phandle->obj_tbl[y*phandle->row + x] = obj;
}

/*********************************************************************
  Function:        divoom_menu_direct_ctrl_get_last_item
  Description:     获取某一行的最后一个元素
  Input:           handle   句柄
                   col      第几行
                   direct
                   
  Return:          需要的数据
**********************************************************************/

void *divoom_menu_direct_ctrl_get_last_item(Pdivoom_menu_direct_ctrl_ctrl phandle, int col)
{
    int i;

    if(col >= phandle->col)
    {
        col = phandle->col -1;
    }
    for(i = phandle->row - 1; i >=0; i --)
    {
        if(phandle->obj_tbl[col * phandle->row + i] != NULL)
        {
            return phandle->obj_tbl[col * phandle->row + i];
        }
    }
    return phandle->obj_tbl[col * phandle->row - 1];
}

/*********************************************************************
  Function:        divoom_menu_direct_ctrl_get_next_item
  Description:     获取下一个元素
  Input:           handle   句柄
                   col      第几行
                   direct
                   
  Return:          需要的数据
**********************************************************************/

void *divoom_menu_direct_ctrl_get_next_item(Pdivoom_menu_direct_ctrl_ctrl phandle, int row, int col)
{
    int i, j;

    for(i = col; i < phandle->col; i ++)
    {
        for(j = (i == col?row+1:0); j < phandle->row; j ++)
        {
            if(phandle->obj_tbl[i * phandle->row + j] != NULL)
            {
                return phandle->obj_tbl[i * phandle->row + j];
            }
        }
    }
    return phandle->obj_tbl[0];
}

/*********************************************************************
  Function:        divoom_menu_direct_ctrl_get_pre_item
  Description:     获取前一个元素
  Input:           handle   句柄
                   col      第几行
                   direct
                   
  Return:          需要的数据
**********************************************************************/

void *divoom_menu_direct_ctrl_get_pre_item(Pdivoom_menu_direct_ctrl_ctrl phandle, int row, int col)
{
    int i, j;

    for(i = col; i >= 0; i--)
    {
        for(j = (i == col?(row-1):phandle->row-1); j >= 0 ; j--)
        {
            if(phandle->obj_tbl[i * phandle->row + j] != NULL)
            {
                return phandle->obj_tbl[i * phandle->row + j];
            }
        }
    }
    return divoom_menu_direct_ctrl_get_last_item(phandle, phandle->col - 1);
}

/*********************************************************************
  Function:        divoom_menu_direct_ctrl_get_last_item
  Description:     获取某一行的最后一个元素
  Input:           handle   句柄
                   col      第几行
                   direct
                   
  Return:          需要的数据
**********************************************************************/

void *divoom_menu_direct_ctrl_get_up_item(Pdivoom_menu_direct_ctrl_ctrl phandle, int x, int y)
{
    int cur_col;
    
    if(y == 0)
    {
        cur_col = phandle->col - 1;
    }
    else
    {
        cur_col = y - 1;
    }
    if(phandle->obj_tbl[cur_col * phandle->row + x] == NULL)
    {
        return divoom_menu_direct_ctrl_get_last_item(phandle, cur_col);
    }
    else
    {
        return phandle->obj_tbl[cur_col * phandle->row + x];
    }
}

/*********************************************************************
  Function:        divoom_menu_direct_ctrl_get_down_item
  Description:     获取某个元素的向下的一个元素
  Input:           handle   句柄
                   x      
                   y
                   
  Return:          需要的数据
**********************************************************************/

void *divoom_menu_direct_ctrl_get_down_item(Pdivoom_menu_direct_ctrl_ctrl phandle, int x, int y)
{
    int cur_col;
    
    if(y < phandle->col - 1)
    {
        cur_col = y + 1;
    }
    else
    {
        cur_col = 0;
    }
    if(phandle->obj_tbl[cur_col * phandle->row + x] == NULL)
    {
        return divoom_menu_direct_ctrl_get_last_item(phandle, cur_col);
    }
    else
    {
        return phandle->obj_tbl[cur_col * phandle->row + x];
    }
}

/*********************************************************************
  Function:        divoom_menu_direct_ctrl_get
  Description:     获取某个结构的另一个方向的数据
  Input:           handle   句柄
                   obj      用户数据,divoom_menu_direct_ctrl_update给的数据
                   direct
                   
  Return:          需要的数据
**********************************************************************/

void *divoom_menu_direct_ctrl_get(void *handle, void *obj, int direct)
{
    int i, j;
    Pdivoom_menu_direct_ctrl_ctrl phandle = (Pdivoom_menu_direct_ctrl_ctrl)handle;
    
    if(handle == NULL)
    {
        return NULL;
    }

    for(i = 0; i < phandle->col; i ++)
    {
        for(j = 0; j < phandle->row; j++)
        {
            if(phandle->obj_tbl[i * phandle->row + j] == NULL)
            {
                break;
            }
            if(phandle->obj_tbl[i * phandle->row + j] == obj)
            {
                //找到这个成员
                switch(direct)
                {
                    case DIVOOM_MENU_DIRECT_UP:
                        return divoom_menu_direct_ctrl_get_up_item(phandle, j, i);
                        break;
                        
                    case DIVOOM_MENU_DIRECT_DOWN:
                        return divoom_menu_direct_ctrl_get_down_item(phandle, j, i);
                        break;
                        
                    case DIVOOM_MENU_DIRECT_LEFT:
                        if(j == 0 && i == 0)
                        {           
                            return divoom_menu_direct_ctrl_get_last_item(phandle, phandle->col - 1);
                        }
                        else
                        {
                            return divoom_menu_direct_ctrl_get_pre_item(phandle, j, i);
                        }
                        break;
                        
                    case DIVOOM_MENU_DIRECT_RIGHT:
                        return divoom_menu_direct_ctrl_get_next_item(phandle, j, i);
                        break;
                        
                }
            }
            
        }
    }
    return NULL;
}


/*********************************************************************
  Function:        divoom_menu_direct_ctrl_get_pos
  Description:     返回这个成员所在的位置偏移
  Input:           handle   句柄
                   obj      用户数据,divoom_menu_direct_ctrl_update给的数据
                   
  Return:          位置
**********************************************************************/

int divoom_menu_direct_ctrl_get_pos(void *handle, void *obj)
{
    int i, j;
    Pdivoom_menu_direct_ctrl_ctrl phandle = (Pdivoom_menu_direct_ctrl_ctrl)handle;
    
    if(handle == NULL)
    {
        return 0;
    }

    for(i = 0; i < phandle->col; i ++)
    {
        for(j = 0; j < phandle->row; j++)
        {
            if(phandle->obj_tbl[i * phandle->row + j] == NULL)
            {
                break;
            }
            if(phandle->obj_tbl[i * phandle->row + j] == obj)
            {
                return i * phandle->row + j; 
            }
            
        }
    }
    return 0;
}


/*********************************************************************
  Function:        divoom_menu_direct_ctrl_get_obj
  Description:     返回某个位置的成员
  Input:           handle   句柄
                   x
                   y
                   
  Return:          obj
**********************************************************************/

void * divoom_menu_direct_ctrl_get_obj(void *handle, uint8 x, uint8 y)
{
    Pdivoom_menu_direct_ctrl_ctrl phandle = (Pdivoom_menu_direct_ctrl_ctrl)handle;
    
    if(handle == NULL)
    {
        return NULL;
    }

    return phandle->obj_tbl[y * phandle->row + x];
}

/*********************************************************************
  Function:        divoom_menu_direct_ctrl_destory
  Description:     释放相关资源
  Input:           handle   句柄
  Return:          none
**********************************************************************/

void divoom_menu_direct_ctrl_destory(void *handle)
{
    if(handle)
    {
        divoom_system_free(handle);
    }
}

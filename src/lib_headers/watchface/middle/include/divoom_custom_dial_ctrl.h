#ifndef _divoom_custom_dial_ctrl_h_
#define _divoom_custom_dial_ctrl_h_


/*********************************************************************
Function:         divoom_custom_dial_ctrl_create_handle
Description:      创建用户自定义表盘句柄
Input:            data
Return:           
Author:           albert
**********************************************************************/

void divoom_custom_dial_ctrl_create_handle(void *data);

/*********************************************************************
Function:         divoom_custom_dial_ctrl_change_bg_image
Description:      修改表盘底图
Input:            img_addr
Return:           
Author:           albert
**********************************************************************/

void divoom_custom_dial_ctrl_change_bg_image(char *img_addr, int local_flag, int flag);

/*********************************************************************
Function:         divoom_custom_dial_ctrl_add_disp_item
Description:      增加或修改显示元素
Input:            data
Return:           
Author:           albert
**********************************************************************/

int divoom_custom_dial_ctrl_add_disp_item(void *data);

/*********************************************************************
Function:         divoom_custom_dial_ctrl_destory_handle
Description:      退出显示模式，清除相关资源
Input:            data
Return:           
Author:           albert
**********************************************************************/

void divoom_custom_dial_ctrl_destory_handle(void);


/*********************************************************************
Function:         divoom_custom_dial_ctrl_update_disp_text_item
Description:      修改显示元素
Input:            data
Return:           
Author:           albert
**********************************************************************/

void divoom_custom_dial_ctrl_update_disp_text_item(void *data);


/*********************************************************************
Function:         divoom_custom_dial_ctrl_get_clock_info
Description:      获取表盘配置
Input:            data
Return:           
Author:           albert
**********************************************************************/

Pdivoom_clock_manage_item divoom_custom_dial_ctrl_get_clock_info(void);

#endif


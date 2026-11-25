#ifndef _divoom_disp_ctrl_app_
#define _divoom_disp_ctrl_app_

typedef struct{
	T_U8	mode;
    T_U8    cache_flag;
    T_U32   read_off;
    T_U32   buf_off;
    T_U32   total_size;
    T_U8    *file_buf;
	void	*handle;
}divoom_disp_ctrl_file_handle, *Pdivoom_disp_ctrl_file_handle;

/*********************************************************************
  Function:        divoom_disp_ctrl_app_play_task
  Description:    灯面板的启动一个事件
  Input:            task_id        事件的ID
  Return:           NONE
  Author:           albert
  Data:            2016-10-31
**********************************************************************/

void divoom_disp_ctrl_app_resume_task(uint8 task_id);
/*********************************************************************
  Function:        divoom_disp_ctrl_app_update_task
  Description:    灯面板的更新调试时间
  Input:            task_id        事件的ID
  Return:           NONE
  Author:           albert
  Data:            2016-10-31
**********************************************************************/

void divoom_disp_ctrl_app_update_task(uint8 task_id, uint32 time);
/*********************************************************************
  Function:        divoom_disp_ctrl_app_pause_task
  Description:    灯面板的暂停一个事件
  Input:            task_id        事件的ID
  Return:           NONE
  Author:           albert
  Data:            2016-10-31
**********************************************************************/

void divoom_disp_ctrl_app_pause_task(uint8 task_id);

/*********************************************************************
  Function:        divoom_disp_ctrl_app_add_task
  Description:    灯面板的添加一个事件，这个函数主要是方便AK向GD的转移
  Input:            time        运行的周期时间
                       pcallback    事件的回调函数
  Return:           此事件的ID
  Author:           albert
  Data:            2016-10-31
**********************************************************************/

uint8 divoom_disp_ctrl_app_create_task(DIVOOM_LIGHT_CALLBACK *pcallback, uint32 time);

/*********************************************************************
  Function:        divoom_disp_ctrl_app_destroy_task
  Description:    灯面板的取消一个事件
  Input:            task_id        事件的ID
  Return:           NONE
  Author:           albert
  Data:            2016-10-31
**********************************************************************/

void divoom_disp_ctrl_app_destroy_task(uint8 task_id);
void divoom_disp_ctrl_app_init(Pdivoom_sys_setting_info psys_setting);
int divoom_disp_ctrl_app_get_env_info(uint8 mode, void *env_data);
//给的数据可以是栈空间的,这个函数内部自动实现申请数据
//> 0 success, other fail
int divoom_disp_ctrl_app_send_cmd(uint8 cmd, uint8 *data, uint16 data_len);

#endif


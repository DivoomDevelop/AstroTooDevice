#ifndef _divoom_disp_com_h_
#define _divoom_disp_com_h_
//#include "divoom_disp_ctrl_app.h"


#include "divoom.h"
#include "divoom_disp_include.h"
#include "divoom_disp_ctrl.h"


//---------------------------------------------------------------------------------------------

typedef void DIVOOM_DISP_CTRL_CALLBACK(void *data);
















//---------------------------------------------------------------------------------------------

enum{
	DIVOOM_DISP_COM_GET_TIME_COLOUR,
	DIVOOM_DISP_COM_GET_TIME_FLAG,
};

void divoom_com_notice_to_screen(int flag);


/*********************************************************************
  Function:        divoom_com_init
  Description:    初始化所有的接口函数
  Input:            pcallback        接口回调
  Return:           NONE
  Author:           albert
**********************************************************************/

void divoom_com_init(Pdivoom_disp_ctrl_callback pcallback);
/*********************************************************************
  Function:        divoom_com_read_config
  Description:    读不同灯效果的配置文件
  Input:            mode 
                      buf
                      size
  Return:           是否读取成功
  Author:           albert
**********************************************************************/

int divoom_com_read_config(uint8 mode,  uint8 index, void *buf, int size);

/*********************************************************************
  Function:        divoom_com_write_config
  Description:    保存灯效果的配置文件
  Input:            mode 
                      buf
                      size
  Return:           是否更新成功
  Author:           albert
**********************************************************************/

int divoom_com_write_config(uint8 mode,  uint8 index, void *buf, int size);

uint32 divoom_disp_ctrl_simplde_write_data(void *buf, int size);

//写灯板的通知图像数据
uint32 divoom_disp_ctrl_notice_write_data(void *buf, int size);

#if 1
/*********************************************************************
  Function:        divoom_com_open_file
  Description:    打开灯效数据
  Input:            mode 
                      index　　　灯效内部索引
  Return:           返回打开句柄
  Author:           albert
**********************************************************************/

void * divoom_com_open_file(const char *file_name, int cache_flag);


//返回文件的长度,只能在open之后使用,handele为open返回的句柄
int divoom_com_return_file_len(void *handle);


/*********************************************************************
  Function:        divoom_com_read_file
  Description:    读取灯效数据
  Input:            handle 
                      buf
                      size
                      off
  Return:           返回读到的数据长度
  Author:           albert
**********************************************************************/

int divoom_com_read_file(void *handle, void *buf, int size, int off);


/*********************************************************************
  Function:        divoom_com_write_file
  Description:    写灯效数据
  Input:            handle 
                      buf
                      size
  Return:           返回写入的数据长度
  Author:           albert
**********************************************************************/

int divoom_com_write_file(void *handle, void *buf, int size, int off);
/*********************************************************************
  Function:        divoom_com_close_file
  Description:    关闭打开的灯效句柄
  Input:            handle 
  Return:          NONE
  Author:           albert
**********************************************************************/

void divoom_com_close_file(void *handle);
#endif

/*********************************************************************
  Function:        divoom_com_disp_write_cmd
  Description:    写灯控制的控制命令，要求实时写入
  Input:            buf
                      size
  Return:          NONE
  Author:           albert
**********************************************************************/

void divoom_com_disp_write_cmd(uint16 cmd, uint8 *data, int size);

#if 0
/*********************************************************************
  Function:        divoom_com_disp_write_data
  Description:    写灯的显示数据，要求按时间间隔写入
  Input:            buf
                      size
                      time   此图将持续的时间，如果为０不更新显示时间，
  Return:          NONE
  Author:           albert
**********************************************************************/

void divoom_com_disp_write_data(uint8 *buf, int size, int time);

/*********************************************************************
  Function:        divoom_com_disp_write_gif_data
  Description:    写灯的显示数据，要求按时间间隔写入
  Input:            buf
                      size
                      time   此图将持续的时间，如果为０不更新显示时间，
  Return:          NONE
  Author:           albert
**********************************************************************/

void divoom_com_disp_write_gif_data(uint8 *buf, int size, int time);

/*********************************************************************
  Function:        divoom_com_muti_gif_data_send
  Description:    	主机发送动画数据到从机
  Input:            gif_info 动画数据 gif_type 动画类型
  Return:          NONE
  Author:           albert
**********************************************************************/

void divoom_com_muti_gif_data_send(void *gif_info, uint8 gif_type);
#endif

/*********************************************************************
  Function:        divoom_com_malloc
  Description:    分配内存
  Input:            size
                      file_name     分配字串标识，方便调试
                      model_line    分配值标识
  Return:          返回分配的内存指针
  Author:           albert
**********************************************************************/

void * divoom_com_malloc(uint32 size,const char *file_name, uint32 model_line);

/*********************************************************************
  Function:        divoom_com_free
  Description:    释放内存资源
  Input:            ptr
  Return:          NONE
  Author:           albert
**********************************************************************/


void divoom_com_free(void *ptr,const char *file_name, uint32 model_line);
#if 0
/*********************************************************************
  Function:        divoom_com_update_task
  Description:    灯面板的更新调试时间
  Input:            task_id        事件的ID
  Return:           NONE
  Author:           albert
  Data:            2016-10-31
**********************************************************************/

void divoom_com_update_task(uint32 task_id, uint32 time);


/*********************************************************************
  Function:        divoom_com_pause_task
  Description:    灯面板的暂停一个事件
  Input:            task_id        事件的ID
  Return:           NONE
  Author:           albert
  Data:            2016-10-31
**********************************************************************/

void divoom_com_pause_task(uint32 task_id);

/*********************************************************************
  Function:        divoom_com_resume_task
  Description:    灯面板的启动一个事件
  Input:            task_id        事件的ID
  Return:           NONE
  Author:           albert
  Data:            2016-10-31
**********************************************************************/

void divoom_com_resume_task(uint32 task_id);

/*********************************************************************
  Function:        divoom_com_destroy_task
  Description:    灯面板的取消一个事件
  Input:            task_id        事件的ID
  Return:           NONE
  Author:           albert
  Data:            2016-10-31
**********************************************************************/

void divoom_com_destroy_task(uint32 task_id);

/*********************************************************************
  Function:        divoom_com_create_task
  Description:    灯面板的添加一个事件，这个函数主要是方便AK向GD的转移
  Input:            time        运行的周期时间
                       pcallback    事件的回调函数
  Return:           此事件的ID
  Author:           albert
  Data:            2016-10-31
**********************************************************************/

uint32 divoom_com_create_task(DIVOOM_DISP_CTRL_CALLBACK *pcallback, uint32 time, uint8 real_flag);


/*****************************************************************************
 Prototype    : divoom_com_resume_task
 Description  : 灯面板事件恢复运行
 Input        : uint32 task_id  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/8/4
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_com_resume_task(uint32 task_id);
#endif



uint8 divoom_com_create_task(DIVOOM_DISP_CTRL_CALLBACK cb, uint32 time);

void divoom_com_destroy_task(uint8 task_id);

void divoom_com_pause_task(uint8 task_id);

void divoom_com_resume_task(uint8 task_id);

void divoom_com_update_task(uint8 task_id, uint32 time);

//获取工具的信息,比如秒表的显示时间
uint32 divoom_com_get_tool_info(uint8 mode, void *tool_data);

/*********************************************************************
  Function:        divoom_com_get_env_info
  Description:    得到系统环境相关信息
  Input:            mode        类型
                       env_data    相关数据
  Return:           NONE
  Author:           albert
  Data:            2016-10-31
**********************************************************************/

int divoom_com_get_env_info(uint8 mode, void *env_data);



/*********************************************************************
  Function:        divoom_com_get_system_time
  Description:    得到系统时间，单位为Ｓ
  Input:            NONE
  Return:           返回时间信息
  Author:           albert
**********************************************************************/

uint32 divoom_com_get_system_time(void);


/*********************************************************************
  Function:        divoom_com_get_random
  Description:    得到系统随机数
  Input:            max        最大值
  Return:           返回随机数
  Author:           albert
  Data:            2016-10-31
**********************************************************************/

int divoom_com_get_random(int max);
typedef void (*T_Voice_PalyEndCallBack)(void);

/*********************************************************************
  Function:        divoom_com_play_voice
  Description:    播放相关提示音
  Input:            
  Author:           albert
  Data:            2016-10-31
**********************************************************************/

void  divoom_com_play_voice(uint16 voice_tip, uint16 time, uint8 vol, T_Voice_PalyEndCallBack callback);


/*********************************************************************
  Function:        divoom_com_get_vol
  Description:    得到系统音量
  Input:           
  Return:          系统音量
  Author:           albert
  Data:            2016-10-31
**********************************************************************/
int divoom_com_get_vol(void);

void *divoom_com_light_mid_init(void);

void divoom_com_light_mid_exit(void *handle);

void *divoom_com_light_start_read(const char *file_name, uint32 *start_page);

void divoom_com_light_end_read(void *handle);


uint8 divoom_com_get_data_from_app(uint8 event, uint8 *data);

#endif



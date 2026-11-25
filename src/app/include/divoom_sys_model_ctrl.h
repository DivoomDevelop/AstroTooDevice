#ifndef _divoom_sys_model_crl_h_
#define _divoom_sys_model_crl_h_

typedef void * DIVOOM_SYS_MODEL_USER_RAM(void *msg_data);
/*************************************************************************
此模块主要是负责不同模块之间的交互管理，所有的模块在使用前，
必须向这个模块进行注册，模块之间的通讯也是先向这个模块进行请求。
**************************************************************************/
typedef uint32 DIVOOM_SYS_MSG_FINISH_CALL_BACK(void *data);
typedef struct _divoom_sys_msg_info
{
    uint8 msg_type;     //消息类型，0:同步消息，1:异步消息
    uint8 src_id;       //发出消息源模块ID
    uint8 dest_id;      //目的地模块ID
    uint8 attr;         //发出模块的一些特殊信息
    uint8 msg_id;       //模块命令ID
    int   result;       //返回值
    uint16 data_len;    //数据长度
    uint8 *data;        //数据
    DIVOOM_SYS_MSG_FINISH_CALL_BACK *pcallback;  //当消息处理完后给出的回调，如果不需要通知，可以调协NULL
    DIVOOM_SYS_MODEL_USER_RAM *pmsg_data_malloc;
    DIVOOM_SYS_MODEL_USER_RAM *pmsg_data_free;
}divoom_sys_msg_info, *Pdivoom_sys_msg_info;

/*********************************************************************
  Function:        divoom_sys_model_crl_send_all_model
  Description:    向所有模块发出某一事件
  Input:            msg_id
  Return:           void
  Author:           albert
  Data:            2015-7-15
**********************************************************************/

void divoom_sys_model_crl_send_all_model(int msg_id);
void divoom_sys_model_crl_send_asyn_all_model(int msg_id);

void divoom_sys_model_crl_send_all_model_with_data(int msg_id, void *data, uint32 data_len);


/*********************************************************************
  Function:        divoom_sys_model_crl_init
  Description:      系统控制模块的初始化,此模块要首先被加载，才能加载其它模块，
                        其它模块要有初始时调用注册，对能正常被使用。
  Input:            void
  Return:           void
  Author:           albert
  Data:            2015-7-15
**********************************************************************/

void divoom_sys_model_crl_init(void);


/*********************************************************************
  Function:        divoom_sys_model_crl_init
  Description:     检查MODEL是否被加载
  Input:            model_id   模块类型
  Return:           1:为加载;0:未加载
  Author:           albert
  Data:            2015-7-15
**********************************************************************/

uint8 divoom_sys_model_is_loaded(uint8 model_id);


/*********************************************************************
  Function:        divoom_sys_model_crl_msg_push
  Description:    当一个模块要请求与另一个模块进行交互时，将使用这个函数，
                      必须先填充相应的数据结构，而且要与之通讯的模块被成功初始化和注册
                      对于msg->data,如果不为空，必须是申请的独立内存，当消息传递给相应模块后，
                      内存将被自动释放。
  Input:            msg     模块通讯的数据结构。
  Return:           void
  Author:           albert
  Data:            2015-7-15
**********************************************************************/
uint8 divoom_sys_model_crl_msg_push(Pdivoom_sys_msg_info msg);


/*********************************************************************
  Function:        divoom_sys_model_crl_save_config
  Description:    关机或者异常时调用，向所有的模块发送保存配置的命令
  Input:            NONE
  Return:           void
  Author:           albert
  Data:            2015-7-15
**********************************************************************/
void divoom_sys_model_crl_save_config(void);


/*********************************************************************
  Function:        divoom_sys_model_crl_register
  Description:    当一模块在成功初始化后，应该调用这个函数将自己注册给系统进行管理
                      ，当其它模块有消息送过来时，将调用pModelCall进行通知。
                      
  Input:            model_id     模块的ID           
                      pModelCall   模块的消息处理函数
  Return:           void
  Author:           albert
  Data:            2015-7-15
**********************************************************************/


typedef uint8 DIVOOM_SYS_MODEL_CALLBACK(Pdivoom_sys_msg_info msg);
void divoom_sys_model_crl_register(uint8 model_id, DIVOOM_SYS_MODEL_CALLBACK *pModelCall);

/*********************************************************************
  Function:        divoom_sys_model_crl_unregister
  Description:    当一个模块被释放后，也要调用这个函数将其在系统解绑，此后系统将不
                      会再调用通知函数。
                      
  Input:            model_id     模块的ID           
  Return:           void
  Author:           albert
  Data:            2015-7-15
**********************************************************************/

void divoom_sys_model_crl_unregister(uint8 model_id);

/*********************************************************************
  Function:        divoom_sys_model_crl_deinit
  Description:    模块的释放资源，但是这个模块是基本模块，只会加载，不会释放，所以
                      这个函数没有功能
                      
  Input:            void
  Return:           void
  Author:           albert
  Data:            2015-7-15
**********************************************************************/

void divoom_sys_model_crl_deinit(void);

/*********************************************************************
  Function:        divoom_sys_model_crl_asyn_main
  Description:     异步消息处理函数
                      
  Input:            void
  Return:           void
  Author:           albert
  Data:            2015-7-15
**********************************************************************/
void *divoom_sys_model_crl_asyn_main(void *data);

#endif



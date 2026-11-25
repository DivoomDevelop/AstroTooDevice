#include "divoom.h"

#define DIVOOM_SYS_MODEL_CTRL_MAX_THREAD_NUM 2
#define DIVOOM_SYS_MODEL_CTRL_MAX_REGISTERED 64

typedef struct _divoom_model_info *Pdivoom_model_info;
typedef struct _divoom_model_info
{
    uint8 model_id;
    DIVOOM_SYS_MODEL_CALLBACK *pcallback;
    Pdivoom_model_info next;
}divoom_model_info;
typedef struct _divoom_sys_model_ctrl_handle
{
    uint32 model_init_flag;
    void *run_sem;
    void *lock;
    void *msg_queue;
    void *task_table[DIVOOM_SYS_MODEL_CTRL_MAX_THREAD_NUM];
    Pdivoom_model_info pmodel_link;
}divoom_sys_model_ctrl_handle, *Pdivoom_sys_model_ctrl_handle;

static Pdivoom_sys_model_ctrl_handle pdivoom_model_ctrl_handle = NULL;


/*********************************************************************
  Function:        divoom_sys_model_crl_send_all_model
  Description:    向所有模块发出某一事件
  Input:            msg_id
  Return:           void
  Author:           albert
  Data:            2015-7-15
**********************************************************************/
void divoom_sys_model_crl_send_all_model(int msg_id)
{
	Pdivoom_model_info pmodel_link = NULL;
	divoom_sys_msg_info  msg = {0};

	if(pdivoom_model_ctrl_handle == NULL )
	{
		return ;
	}
	msg.msg_id = msg_id;
	msg.src_id = DIVOOM_MODEL_NULL;
	msg.dest_id = DIVOOM_MODEL_MAIN;
    divoom_sys_model_crl_msg_push(&msg);
	//divoom_system_get_lock(pdivoom_model_ctrl_handle->msg_mutex);
	pmodel_link = pdivoom_model_ctrl_handle->pmodel_link;
	while(pmodel_link)
	{
		msg.dest_id = pmodel_link->model_id;
        if(pmodel_link->model_id != DIVOOM_MODEL_MAIN)
        {
		    pmodel_link->pcallback(&msg);
        }
		pmodel_link = pmodel_link->next;
	}
	//divoom_system_free_lock(pdivoom_model_ctrl_handle->msg_mutex);
}

void divoom_sys_model_crl_send_all_model_with_data(int msg_id, void *data, uint32 data_len)
{
	Pdivoom_model_info pmodel_link = NULL;
	divoom_sys_msg_info  msg = {0};

	if(pdivoom_model_ctrl_handle == NULL )
	{
		return ;
	}
	msg.msg_id = msg_id;
	msg.data = data;
	msg.data_len = data_len;
	msg.src_id = DIVOOM_MODEL_NULL;
	pmodel_link = pdivoom_model_ctrl_handle->pmodel_link;
	while(pmodel_link)
	{
		msg.dest_id = pmodel_link->model_id;
		pmodel_link->pcallback(&msg);
		pmodel_link = pmodel_link->next;
	}
}

/*********************************************************************
  Function:        divoom_sys_model_crl_send_all_model
  Description:    向所有模块发出某一事件
  Input:            msg_id
  Return:           void
  Author:           albert
  Data:            2015-7-15
**********************************************************************/
void divoom_sys_model_crl_send_asyn_all_model(int msg_id)
{
	Pdivoom_model_info pmodel_link = NULL;
	divoom_sys_msg_info  msg = {0};
	uint8 dest_ids[DIVOOM_SYS_MODEL_CTRL_MAX_REGISTERED];
	int dest_count = 0;
	int i;

	if(pdivoom_model_ctrl_handle == NULL )
	{
		return ;
	}
	msg.msg_id = msg_id;
	msg.src_id = DIVOOM_MODEL_NULL;
	msg.dest_id = DIVOOM_MODEL_MAIN;
    divoom_sys_model_crl_msg_push(&msg);
    msg.msg_type = 1;

    /* 先快照 dest_id，再释放锁后 msg_push；持锁时 put_sem 会唤醒 asyn worker 抢同一把锁 → 死锁 */
    divoom_system_get_lock(pdivoom_model_ctrl_handle->lock);
	pmodel_link = pdivoom_model_ctrl_handle->pmodel_link;
	while(pmodel_link && dest_count < DIVOOM_SYS_MODEL_CTRL_MAX_REGISTERED)
	{
        if(pmodel_link->model_id != DIVOOM_MODEL_MAIN && pmodel_link->model_id != DIVOOM_MODEL_DINGDANG_VOICE)
        {
            dest_ids[dest_count++] = pmodel_link->model_id;
        }
		pmodel_link = pmodel_link->next;
	}
    divoom_system_free_lock(pdivoom_model_ctrl_handle->lock);

    for(i = 0; i < dest_count; i++)
    {
        msg.dest_id = dest_ids[i];
        divoom_sys_model_crl_msg_push(&msg);
    }
}


void *divoom_sys_model_crl_asyn_main(void * data)
{
    Pdivoom_sys_msg_info pmsg_info;
    Pdivoom_model_info pmodel_link = NULL;
    while(1)
    {
        divoom_system_get_sem(pdivoom_model_ctrl_handle->run_sem);
        while(divoom_queue_not_empty(pdivoom_model_ctrl_handle->msg_queue))
        {
            pmsg_info = (Pdivoom_sys_msg_info)divoom_queue_pop(pdivoom_model_ctrl_handle->msg_queue);
            if(pmsg_info == NULL)
            {
                break;
            }
            divoom_system_get_lock(pdivoom_model_ctrl_handle->lock);
    	    pmodel_link = pdivoom_model_ctrl_handle->pmodel_link;
    	    while(pmodel_link)
    	    {
    	        if(pmodel_link->model_id == pmsg_info->dest_id)
    	        {
    	            break; 
    	        }
    	        pmodel_link = pmodel_link->next;
    	    }
            divoom_system_free_lock(pdivoom_model_ctrl_handle->lock);
            if(pmodel_link)
            {
                divoom_print("start msg: %d, %d\n", pmsg_info->dest_id, pmsg_info->msg_id);
                pmodel_link->pcallback(pmsg_info);
                divoom_print("end msg: %d, %d\n", pmsg_info->dest_id, pmsg_info->msg_id);
            }
            if(pmsg_info->pmsg_data_free)
            {
                pmsg_info->pmsg_data_free(pmsg_info);
            }
            else
            {
                divoom_net_app_do_comm_free(pmsg_info);
            }
        }
    }
    return NULL;
}

#if defined(DIVOOM_SIMULATOR) && DIVOOM_SIMULATOR && !SIM_THREAD_REAL_DEVICE
void divoom_sys_model_crl_poll(void)
{
    Pdivoom_sys_msg_info pmsg_info;
    Pdivoom_model_info pmodel_link = NULL;

    if (pdivoom_model_ctrl_handle == NULL)
    {
        return;
    }

    while (divoom_queue_not_empty(pdivoom_model_ctrl_handle->msg_queue))
    {
        pmsg_info = (Pdivoom_sys_msg_info)divoom_queue_pop(pdivoom_model_ctrl_handle->msg_queue);
        if (pmsg_info == NULL)
        {
            break;
        }
        divoom_system_get_lock(pdivoom_model_ctrl_handle->lock);
        pmodel_link = pdivoom_model_ctrl_handle->pmodel_link;
        while (pmodel_link)
        {
            if (pmodel_link->model_id == pmsg_info->dest_id)
            {
                break;
            }
            pmodel_link = pmodel_link->next;
        }
        divoom_system_free_lock(pdivoom_model_ctrl_handle->lock);
        if (pmodel_link)
        {
            divoom_print("start msg: %d, %d\n", pmsg_info->dest_id, pmsg_info->msg_id);
            pmodel_link->pcallback(pmsg_info);
            divoom_print("end msg: %d, %d\n", pmsg_info->dest_id, pmsg_info->msg_id);
        }
        if (pmsg_info->pmsg_data_free)
        {
            pmsg_info->pmsg_data_free(pmsg_info);
        }
        else
        {
            divoom_net_app_do_comm_free(pmsg_info);
        }
    }
}
#endif

/*********************************************************************
  Function:        divoom_sys_model_crl_init
  Description:      系统控制模块的初始化,此模块要首先被加载，才能加载其它模块，
                        其它模块要有初始时调用注册，对能正常被使用。
  Input:            void
  Return:           void
  Author:           albert
  Data:            2015-7-15
**********************************************************************/

void divoom_sys_model_crl_init(void)
{
    int i;
    if(pdivoom_model_ctrl_handle)
    {
        divoom_print("[%s:%d] the model had been init\n",__func__, __LINE__);
        return;
    }

    pdivoom_model_ctrl_handle = (Pdivoom_sys_model_ctrl_handle)divoom_system_malloc(sizeof(divoom_sys_model_ctrl_handle));
    if(pdivoom_model_ctrl_handle == NULL)
    {
        divoom_print("[%s:%d] it fails to malloc\n",__func__, __LINE__);    
        return;
    }
    memset(pdivoom_model_ctrl_handle, 0, sizeof(divoom_sys_model_ctrl_handle));
    pdivoom_model_ctrl_handle->msg_queue = divoom_queue_init(200, __func__, __LINE__);
    pdivoom_model_ctrl_handle->run_sem = divoom_system_create_sem(DIVOOM_SYS_MODEL_CTRL_MAX_THREAD_NUM, 0);
    pdivoom_model_ctrl_handle->lock = divoom_system_create_lock();
    #if 1
    for(i = 0; i < DIVOOM_SYS_MODEL_CTRL_MAX_THREAD_NUM; i++)
    {
        pdivoom_model_ctrl_handle->task_table[i] = divoom_system_create_task(divoom_sys_model_crl_asyn_main, "sys_model", NULL, DIVOOM_TASK_STACK_SYS_MODEL_ASYNC); 
    }
    #endif
}



/*********************************************************************
  Function:        divoom_sys_model_crl_init
  Description:     检查MODEL是否被加载
  Input:            model_id   模块类型
  Return:           1:为加载;0:未加载
  Author:           albert
  Data:            2015-7-15
**********************************************************************/

uint8 divoom_sys_model_is_loaded(uint8 model_id)
{
    uint32 flag = 0;
    
	if(pdivoom_model_ctrl_handle)
	{
		flag = pdivoom_model_ctrl_handle->model_init_flag & (1 << model_id);
	}

	return flag!=0 ;
}


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
uint8 divoom_sys_model_crl_msg_push(Pdivoom_sys_msg_info msg)
{
    Pdivoom_model_info pmodel_link = NULL;
	//uint8 ret = DIVOOM_LIGHT_APP_NULL;
	uint8 ret = 0xFF;

    if(pdivoom_model_ctrl_handle == NULL || msg == NULL)
    {
        return 0xFF;
    }

	if (divoom_onoff_is_poweroff_active())
	{
		return 0xFF;
	}
    
    if(msg->msg_type)
    {
        Pdivoom_sys_msg_info pnew_msg;

        if(msg->pmsg_data_malloc == NULL)
        {
            pnew_msg = (Pdivoom_sys_msg_info)divoom_net_app_do_comm_malloc(msg);
        }
        else
        {
            pnew_msg = (Pdivoom_sys_msg_info)msg->pmsg_data_malloc(msg);
        }
        if(divoom_queue_push(pdivoom_model_ctrl_handle->msg_queue, pnew_msg) == 0)
        {
            divoom_print_err("push err,please check:!\n");
        }
        pmodel_link = NULL;
        divoom_system_put_sem(pdivoom_model_ctrl_handle->run_sem);
    }
    else
    {
	    pmodel_link = pdivoom_model_ctrl_handle->pmodel_link;
	    while(pmodel_link)
	    {
	        if(pmodel_link->model_id == msg->dest_id)
	        {
	            break;
	        }
	        pmodel_link = pmodel_link->next;
	    }
    }
	
	if(pmodel_link)
	{
        divoom_print("start msg: %d, %d\n", msg->dest_id, msg->msg_id);
	    ret = pmodel_link->pcallback(msg);
        divoom_print("end msg: %d, %d\n", msg->dest_id, msg->msg_id);
    }

	return pmodel_link?ret:0xFF;
}


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


void divoom_sys_model_crl_register(uint8 model_id, DIVOOM_SYS_MODEL_CALLBACK *pModelCall)
{
    Pdivoom_model_info pmodel_link, new_model;
    
    if(pdivoom_model_ctrl_handle == NULL)
    {
        return ;
    }
    divoom_system_get_lock(pdivoom_model_ctrl_handle->lock);
    pmodel_link = pdivoom_model_ctrl_handle->pmodel_link;
    new_model = (Pdivoom_model_info)divoom_system_malloc(sizeof(divoom_model_info));
    if(new_model)
    {
        new_model->model_id = model_id;
        new_model->pcallback = pModelCall;
        new_model->next = NULL;
        if(pmodel_link)
        {
            while(pmodel_link->next)
            {
                pmodel_link = pmodel_link->next;
            }
            pmodel_link->next = new_model;
        }
        else
        {
            pdivoom_model_ctrl_handle->pmodel_link = new_model;
        }
    }
    divoom_system_free_lock(pdivoom_model_ctrl_handle->lock);
}


/*********************************************************************
  Function:        divoom_sys_model_crl_unregister
  Description:    当一个模块被释放后，也要调用这个函数将其在系统解绑，此后系统将不
                      会再调用通知函数。
                      
  Input:            model_id     模块的ID           
  Return:           void
  Author:           albert
  Data:            2015-7-15
**********************************************************************/

void divoom_sys_model_crl_unregister(uint8 model_id)
{
    Pdivoom_model_info pre_item =NULL,pmodel_link;
    
    if(pdivoom_model_ctrl_handle == NULL)
    {
        return ;
    }
    
    divoom_system_get_lock(pdivoom_model_ctrl_handle->lock);
    pmodel_link = pdivoom_model_ctrl_handle->pmodel_link;
    while(pmodel_link)
    {
        if(pmodel_link->model_id == model_id)
        {
            if(pre_item == NULL)
            {
	            pdivoom_model_ctrl_handle->pmodel_link = pmodel_link->next;
            }
            else
            {
	            pre_item->next = pmodel_link->next;
            }

			//zhangyu fix 
			divoom_system_free(pmodel_link);
			//zhangyu end fix
			
            break;
        }
        pre_item = pmodel_link;
        pmodel_link = pmodel_link->next;
    }
    divoom_system_free_lock(pdivoom_model_ctrl_handle->lock);
}


/*********************************************************************
  Function:        divoom_sys_model_crl_deinit
  Description:    模块的释放资源，但是这个模块是基本模块，只会加载，不会释放，所以
                      这个函数没有功能
                      
  Input:            void
  Return:           void
  Author:           albert
  Data:            2015-7-15
**********************************************************************/

void divoom_sys_model_crl_deinit(void)
{
}

/*********************************************************************
Function:        divoom_sys_model_crl_save_config
Description:    关机或者异常时调用，向所有的模块发送保存配置的命令
Input:            NONE        
Return:           void
Author:           albert
Data:            2015-7-15   
 **********************************************************************/
void divoom_sys_model_crl_save_config(void)
{   
	divoom_sys_model_crl_send_all_model(DIVOOM_APP_CONFIG_SAVE_EVENT_ID);
}



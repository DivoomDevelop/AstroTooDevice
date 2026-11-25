#include "divoom.h"

#if 0
typedef struct _divoom_user_define_schedule_ctrl
{
    uint32 update_time;
    void *lock;
    Pdivoom_user_define_schedule_item  prun_first_end;  //run_queue里央最早结束的UTC时间
    Pdivoom_user_define_schedule_item  pcur_run_item;  //正在激活的运行的
    Pdivoom_user_define_schedule_item  pwait_first_run;  //wait_queue里面最开始运行的UTC时间
    void *run_queue;    //已经激活的
    void *wait_queue;   //还未激活的
}divoom_user_define_schedule_ctrl, *Pdivoom_user_define_schedule_ctrl;

Pdivoom_user_define_schedule_ctrl pdivoom_user_define_schedule_handle = NULL;
void divoom_user_define_schedule_save_config(void);
void divoom_user_define_schedule_get_next_run_item(void);
Pdivoom_user_define_schedule_item divoom_user_define_schedule_get_cur_run_item(uint32 cur_time);
void divoom_user_define_schedule_request_run(Pdivoom_user_define_schedule_item pactive_item);

/*
  Function:       divoom_user_define_schedule_format_run_time
  Description:    得到这个计划项的start_utc_time， end_utc_time
  Input:          pitem
                  flag 0:新添加，1：平时运行的; 2:开机位置
  Return:         NONE
  Author:         aijun
*/

void divoom_user_define_schedule_format_run_time(Pdivoom_user_define_schedule_item pitem, uint8 flag)
{
    time_t cur_time;
	struct tm time_now;  
    
    if(pitem == NULL)
    {
        return;
    }
    if(pitem->active == 0)
    {
        pitem->start_utc_time = U32_MAX;
        pitem->end_utc_time = U32_MAX;
        return;
    }
    cur_time = time(0);
    if(flag == 1)
    {
        //如果一次性的运行完了，将取消
        if(pitem->cyc_flag == 0)
        {
            pitem->active = 0;
            pitem->start_utc_time = U32_MAX;
            pitem->end_utc_time = U32_MAX;
            return;
        }
    }
    
	localtime_r(&cur_time, &time_now);
    time_now.tm_sec = 0;
    if(pitem->cyc_flag == 0)
    {
        if(pitem->end_hour > pitem->start_hour ||
            (pitem->end_hour == pitem->start_hour && pitem->end_min > pitem->start_min))
        {
            //结束时间大于起始时间
            if(time_now.tm_hour > pitem->end_hour ||
                (time_now.tm_hour == pitem->end_hour && time_now.tm_min >= pitem->end_min))
            {
                //明天的时间
                time_now.tm_hour = pitem->end_hour;
                time_now.tm_min = pitem->end_min;
                pitem->end_utc_time = mktime(&time_now) + 24*3600;
                time_now.tm_hour = pitem->start_hour;
                time_now.tm_min = pitem->start_min;
                pitem->start_utc_time = mktime(&time_now) + 24*3600;
            }
            else
            {
                //今天的时间
                time_now.tm_hour = pitem->end_hour;
                time_now.tm_min = pitem->end_min;
                pitem->end_utc_time = mktime(&time_now);
                time_now.tm_hour = pitem->start_hour;
                time_now.tm_min = pitem->start_min;
                pitem->start_utc_time = mktime(&time_now);
            }
        }
        else
        {
            divoom_print("aj test big time!\n");
            //只要比较START TIME,确认起始位置，看是今天还是明天
            if((time_now.tm_hour < pitem->start_hour ||
                (time_now.tm_hour == pitem->start_hour && time_now.tm_min <= pitem->start_min)))
            {
                //今天的时间
                divoom_print("aj test today time!\n");
                time_now.tm_hour = pitem->end_hour;
                time_now.tm_min = pitem->end_min;
                pitem->end_utc_time = mktime(&time_now) + 24*3600;
                time_now.tm_hour = pitem->start_hour;
                time_now.tm_min = pitem->start_min;
                pitem->start_utc_time = mktime(&time_now);
            }
            else
            {
                //明天的时间
                divoom_print("aj test tomorrow time!\n");
                time_now.tm_hour = pitem->end_hour;
                time_now.tm_min = pitem->end_min;
                pitem->end_utc_time = mktime(&time_now) + 24*3600 * 2;
                time_now.tm_hour = pitem->start_hour;
                time_now.tm_min = pitem->start_min;
                pitem->start_utc_time = mktime(&time_now) + 24*3600;
            }
        }
    }
    else
    {
        //循环模式
        int i, days = 1;
        
        if(pitem->end_hour > pitem->start_hour ||
            (pitem->end_hour == pitem->start_hour && pitem->end_min > pitem->start_min))
        {
            //结束时间大于起始时间
            if((pitem->cyc_flag & ( 1 << time_now.tm_wday)) &&
                (time_now.tm_hour < pitem->end_hour ||
                (time_now.tm_hour == pitem->end_hour && time_now.tm_min < pitem->end_min)))
            {
                
                time_now.tm_hour = pitem->end_hour;
                time_now.tm_min = pitem->end_min;
                pitem->end_utc_time = mktime(&time_now);
                time_now.tm_hour = pitem->start_hour;
                time_now.tm_min = pitem->start_min;
                pitem->start_utc_time = mktime(&time_now);
            }
            else
            {
            
                for(i = time_now.tm_wday + 1; i < time_now.tm_wday + 7; i ++)
                {
                    if(pitem->cyc_flag & ( 1 << (i % 7)))
                    {
                        break;
                    }
                    days ++;
                }
                time_now.tm_hour = pitem->end_hour;
                time_now.tm_min = pitem->end_min;
                pitem->end_utc_time = mktime(&time_now) + days * 24*3600;
                time_now.tm_hour = pitem->start_hour;
                time_now.tm_min = pitem->start_min;
                pitem->start_utc_time = mktime(&time_now) + days * 24*3600;
            }
        }
        else
        {
            //结束时间小于起始时间
            divoom_print("aj test big time!\n");
            if((pitem->cyc_flag & ( 1 << time_now.tm_wday)) &&
                (time_now.tm_hour < pitem->start_hour ||
                (time_now.tm_hour == pitem->start_hour && time_now.tm_min < pitem->start_min)))
            {
                
                divoom_print("aj test today time!\n");
                time_now.tm_hour = pitem->end_hour;
                time_now.tm_min = pitem->end_min;
                pitem->end_utc_time = mktime(&time_now) + 24*3600;
                time_now.tm_hour = pitem->start_hour;
                time_now.tm_min = pitem->start_min;
                pitem->start_utc_time = mktime(&time_now);
            }
            else
            {            
                divoom_print("aj test tomorrow time!\n");
                for(i = time_now.tm_wday + 1; i < time_now.tm_wday + 7; i ++)
                {
                    if(pitem->cyc_flag & ( 1 << (i % 7)))
                    {
                        break;
                    }
                    days ++;
                }
                time_now.tm_hour = pitem->end_hour;
                time_now.tm_min = pitem->end_min;
                pitem->end_utc_time = mktime(&time_now) + (days + 1) * 24*3600;
                time_now.tm_hour = pitem->start_hour;
                time_now.tm_min = pitem->start_min;
                pitem->start_utc_time = mktime(&time_now) + days * 24*3600;
            }
        }
    }
    pitem->pri = pitem->start_utc_time;
    
}


/*
  Function:       divoom_user_define_schedule_init
  Description:    初始化
  Input:          none
  Return:         NONE
  Author:         aijun
*/

void divoom_user_define_schedule_init(void)
{
    if(pdivoom_user_define_schedule_handle)
    {
        return;
    }
    pdivoom_user_define_schedule_handle = divoom_system_malloc(sizeof(divoom_user_define_schedule_ctrl));
    if(pdivoom_user_define_schedule_handle == NULL)
    {
        return;
    }
    pdivoom_user_define_schedule_handle->run_queue = divoom_queue_init(1000);
    pdivoom_user_define_schedule_handle->wait_queue = divoom_queue_init(1000);
    pdivoom_user_define_schedule_handle->lock = divoom_system_create_lock();
}
int divoom_user_define_schedule_compare_item(void *item1, void *item2)
{
    Pdivoom_user_define_schedule_item pitem1 = (Pdivoom_user_define_schedule_item)item1, pitem2 = (Pdivoom_user_define_schedule_item)item2;

    return (pitem1->id == pitem2->id) ? 0:1;
}


void divoom_user_define_schedule_get_sys_time(Pdivoom_channel_user_define_schedule_info pitem, Pdivoom_user_define_schedule_item plocal)
{
    uint32 min_time;
    memset(plocal, 0, sizeof(divoom_user_define_schedule_item));
    plocal->id = pitem->id;
    plocal->cyc_flag = pitem->cyc_flag;
    plocal->active = pitem->status;       //0:过期的；1：有效的
    min_time = pitem->start_time / 60;
    plocal->start_hour = min_time / 60;
    plocal->start_min = min_time % 60;
    min_time = pitem->end_time / 60;
    plocal->end_hour = min_time / 60;
    plocal->end_min = min_time % 60;
    plocal->voice_flag = pitem->voice_flag;
}

/*
  Function:       divoom_user_define_schedule_update
  Description:    更新一个计划项，如果不存在，将添加
  Input:          puser_define_item
  Return:         NONE
  Author:         aijun
*/

int divoom_user_define_schedule_update(Pdivoom_channel_user_define_schedule_info pnet_item)
{
    int ret = 0;
    Pdivoom_user_define_schedule_item pfind_item;
    divoom_user_define_schedule_item *puser_define_item, local_item;

    if(pdivoom_user_define_schedule_handle == NULL)
    {
        return 0;
    }
    divoom_print("update item: %d, %d, %d, %d, %d, %d\n", pnet_item->id, pnet_item->status, pnet_item->start_time
    , pnet_item->end_time, pnet_item->cyc_flag, pnet_item->voice_flag);
    puser_define_item = &local_item;
    divoom_user_define_schedule_get_sys_time(pnet_item, puser_define_item);
    divoom_system_get_lock(pdivoom_user_define_schedule_handle->lock);
    pfind_item = divoom_queue_pop_cmp(pdivoom_user_define_schedule_handle->wait_queue, puser_define_item, divoom_user_define_schedule_compare_item);    
    if(pfind_item)
    {
        //比较一下是否发生变化
        if(pfind_item->active != puser_define_item->active ||
            pfind_item->cyc_flag != puser_define_item->cyc_flag ||
            pfind_item->start_hour != puser_define_item->start_hour ||
            pfind_item->start_min != puser_define_item->start_min ||
            pfind_item->end_hour != puser_define_item->end_hour ||
            pfind_item->end_min != puser_define_item->end_min  ||
            pfind_item->voice_flag != puser_define_item->voice_flag)
        {
            memcpy(pfind_item, puser_define_item, sizeof(divoom_user_define_schedule_item));
            divoom_user_define_schedule_format_run_time(pfind_item, 0);
            divoom_queue_push(pdivoom_user_define_schedule_handle->wait_queue, pfind_item);
            ret = 1;
        }
        else
        {
            divoom_queue_push(pdivoom_user_define_schedule_handle->wait_queue, pfind_item);
        }
            
    }
    else
    {
        pfind_item = divoom_queue_pop_cmp(pdivoom_user_define_schedule_handle->run_queue, puser_define_item, divoom_user_define_schedule_compare_item);    
        if(pfind_item)
        {
            if(pfind_item->active != puser_define_item->active ||
                pfind_item->cyc_flag != puser_define_item->cyc_flag ||
                pfind_item->start_hour != puser_define_item->start_hour ||
                pfind_item->start_min != puser_define_item->start_min ||
                pfind_item->end_hour != puser_define_item->end_hour ||
                pfind_item->end_min != puser_define_item->end_min  ||
                pfind_item->voice_flag != puser_define_item->voice_flag)
            {
                memcpy(pfind_item, puser_define_item, sizeof(divoom_user_define_schedule_item));
                divoom_user_define_schedule_format_run_time(pfind_item, 0);
                divoom_queue_push(pdivoom_user_define_schedule_handle->wait_queue, pfind_item);
                if(pfind_item == pdivoom_user_define_schedule_handle->pcur_run_item)
                {
                    divoom_user_define_schedule_request_run(NULL);
                }
                ret = 1;
            }
            else
            {
                divoom_queue_push(pdivoom_user_define_schedule_handle->run_queue, pfind_item);
            }
        }
        else
        {
            pfind_item = divoom_system_malloc(sizeof(divoom_user_define_schedule_item));
            if(pfind_item)
            {
                memcpy(pfind_item, puser_define_item, sizeof(divoom_user_define_schedule_item));
                divoom_user_define_schedule_format_run_time(pfind_item, 0);
                divoom_queue_push(pdivoom_user_define_schedule_handle->wait_queue, pfind_item);
            }
            ret = 1;
        }
    }
	//divoom_user_define_schedule_get_cur_run_item();
	//divoom_user_define_schedule_request_run(pdivoom_user_define_schedule_handle->pcur_run_item);
    divoom_system_free_lock(pdivoom_user_define_schedule_handle->lock);
    return ret;
}

/*
  Function:       divoom_user_define_schedule_clear_all
  Description:    清除所有的
  Input:          puser_define_item
                  num
  Return:         NONE
  Author:         aijun
*/

void divoom_user_define_schedule_clear_all(void)
{
    divoom_system_get_lock(pdivoom_user_define_schedule_handle->lock);
    divoom_queue_pop_all(pdivoom_user_define_schedule_handle->run_queue, NULL);
    divoom_queue_pop_all(pdivoom_user_define_schedule_handle->wait_queue, NULL);
    pdivoom_user_define_schedule_handle->pcur_run_item = NULL;
    pdivoom_user_define_schedule_handle->prun_first_end = NULL;
    pdivoom_user_define_schedule_handle->pcur_run_item = NULL;
    pdivoom_user_define_schedule_handle->pwait_first_run = NULL;
    divoom_system_free_lock(pdivoom_user_define_schedule_handle->lock);
}

/*
  Function:       divoom_user_define_schedule_update
  Description:    更新计划列表
  Input:          puser_define_item
                  num
  Return:         NONE
  Author:         aijun
*/

void divoom_user_define_schedule_update_list(Pdivoom_channel_user_define_schedule_info puser_define_item, int num)
{
    int i, ret = 0;

    if(pdivoom_user_define_schedule_handle == NULL || puser_define_item == NULL)
    {
        return;
    }
    if(divoom_queue_items(pdivoom_user_define_schedule_handle->wait_queue) + divoom_queue_items(pdivoom_user_define_schedule_handle->run_queue) == DIVOOM_USER_DEFINE_SCHEDULE_ITEM_NUMBER)
    {
        //divoom_main_save_notice_dis();
    }

    for(i = 0; i < num; i ++)
    {
        ret += divoom_user_define_schedule_update(&puser_define_item[i]);
    }
    if(ret)
    {
        divoom_system_get_lock(pdivoom_user_define_schedule_handle->lock);
        divoom_user_define_schedule_get_next_run_item();
        divoom_system_free_lock(pdivoom_user_define_schedule_handle->lock);
    }
}


/*
  Function:       divoom_user_define_schedule_get_next_run_item
  Description:    从待执行的队列中取出最开始执行的那个，也就是start_utc_time最小的那个
  Input:          puser_define_item
  Return:         NONE
  Author:         aijun
*/

void divoom_user_define_schedule_get_next_run_item(void)
{
    int i;
    Pdivoom_user_define_schedule_item pitem;

    pdivoom_user_define_schedule_handle->pwait_first_run = NULL;
    for(i = 0; i < divoom_queue_items(pdivoom_user_define_schedule_handle->wait_queue); i++)
    {
        pitem = divoom_queue_get_index_item(pdivoom_user_define_schedule_handle->wait_queue, i);
        if(pitem->active == 0 || pitem->start_utc_time == U32_MAX)
        {
            continue;
        }
        if(pdivoom_user_define_schedule_handle->pwait_first_run == NULL)
        {
            pdivoom_user_define_schedule_handle->pwait_first_run = pitem;
        }
        else if(pdivoom_user_define_schedule_handle->pwait_first_run->start_utc_time > pitem->start_utc_time)
        {
            pdivoom_user_define_schedule_handle->pwait_first_run = pitem;
        }
        else if(pdivoom_user_define_schedule_handle->pwait_first_run->start_utc_time == pitem->start_utc_time &&
            pdivoom_user_define_schedule_handle->pwait_first_run->pri <= pitem->pri)
        {
            pdivoom_user_define_schedule_handle->pwait_first_run = pitem;
        }
    }
    if(pdivoom_user_define_schedule_handle->pwait_first_run)
    {
        divoom_print("next user_define is; %d, s: %d, e:%d\n", pdivoom_user_define_schedule_handle->pwait_first_run->id, pdivoom_user_define_schedule_handle->pwait_first_run->start_utc_time, pdivoom_user_define_schedule_handle->pwait_first_run->end_utc_time);
    }
    else
    {
        divoom_print("no user_define info!\n");
    }
}

/*
  Function:       divoom_user_define_schedule_get_next_run_item
  Description:    从待执行的队列中取出最开始执行的那个，也就是start_utc_time最小的那个
  Input:          puser_define_item
  Return:         NONE
  Author:         aijun
*/

Pdivoom_user_define_schedule_item divoom_user_define_schedule_get_cur_run_item(uint32 cur_time)
{
    int i;
    Pdivoom_user_define_schedule_item pitem, pfind = NULL;

    for(i = 0; i < divoom_queue_items(pdivoom_user_define_schedule_handle->run_queue); i++)
    {
		pitem = divoom_queue_get_index_item(pdivoom_user_define_schedule_handle->run_queue, i);
        if(pitem->start_utc_time < cur_time && pitem->end_utc_time > cur_time)
        {
            if(pfind == NULL)
            {
                pfind = pitem;
            }
            else if(pfind->start_utc_time < pitem->start_utc_time &&
                pfind->pri == pitem->pri)
            {
                pfind = pitem;
            }
            else if(pfind->pri < pitem->pri)
            {
                pfind= pitem;
            }
        }
    }
    return pfind;
}

/*
  Function:       divoom_user_define_schedule_del_time_out_item
  Description:    将过期的计划移出
  Input:          puser_define_item
  Return:         NONE
  Author:         aijun
*/

void divoom_user_define_schedule_del_time_out_item(int cur_time)
{
    int i, run_flag = 1, del_num = 0;
    Pdivoom_user_define_schedule_item pitem;

    while(run_flag && divoom_queue_items(pdivoom_user_define_schedule_handle->run_queue) > 1)
    {
        run_flag = 0;
        for(i = 0; i < divoom_queue_items(pdivoom_user_define_schedule_handle->run_queue); i++)
        {
            pitem = divoom_queue_get_index_item(pdivoom_user_define_schedule_handle->run_queue, i);
            if(pdivoom_user_define_schedule_handle->pcur_run_item == pitem)
            {
                //当前正在处理的不管，后面处理
                continue;
            }
            if(pitem->end_utc_time <= cur_time)
            {
                divoom_queue_pop_item(pdivoom_user_define_schedule_handle->run_queue, pitem);
                divoom_user_define_schedule_format_run_time(pitem, 1);
                divoom_queue_push(pdivoom_user_define_schedule_handle->wait_queue, pitem);
                del_num ++;
                run_flag = 1;
				break;
            }
        }
    }

    if(del_num)
    {
        divoom_user_define_schedule_get_next_run_item();
    }

}


/*
  Function:       divoom_user_define_schedule_request_run
  Description:    请求运行某个计划
  Input:          pactive_item      为空的话，关闭计划运行，否则将运行某个计划
  Return:         NONE
  Author:         aijun
*/

void divoom_user_define_schedule_request_run(Pdivoom_user_define_schedule_item pactive_item)
{
	divoom_sys_msg_info model_msg = {0};
    
    if(pactive_item == NULL)
    {
        //将要取消运行
        if(pdivoom_user_define_schedule_handle->pcur_run_item != NULL)
        {
            //之前有运行的，要取消调度
            divoom_print("end alarm user_define: %d\n", pdivoom_user_define_schedule_handle->pcur_run_item->id);
			model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
			model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_START_SCHEDULE_USER_DEFINE;
            model_msg.data = NULL;
			divoom_sys_model_crl_msg_push(&model_msg);
            pdivoom_user_define_schedule_handle->pcur_run_item = NULL;
            
        }
    }
    else
    {
        //将要运行
        divoom_user_define_schedule_play_info user_define_info;
        divoom_print("run alarm user_define: %d\n", pactive_item->id);
        pdivoom_user_define_schedule_handle->pcur_run_item = pactive_item;
		model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
		model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_START_SCHEDULE_USER_DEFINE;
        user_define_info.id = pactive_item->id;
        user_define_info.voice_flag = pactive_item->voice_flag;
        model_msg.data = (uint8 *)&user_define_info;
		divoom_sys_model_crl_msg_push(&model_msg);
    }
}


/*
  Function:       divoom_user_define_schedule_restore_run
  Description:    恢复当前运行的时间计划, 在事件重新被调度时调用
  Input:          
  Return:         NONE
  Author:         aijun
*/

void divoom_user_define_schedule_restore_run(void)
{
}

/*
  Function:       divoom_user_define_schedule_main
  Description:    对计划进行相关调度
  Input:          
  Return:         NONE
  Author:         aijun
*/

void divoom_user_define_schedule_main(void)
{
    int cur_time = time(0);
    
    if(pdivoom_user_define_schedule_handle == NULL)
    {
        return;
    }
    
    divoom_system_get_lock(pdivoom_user_define_schedule_handle->lock);
    //先检查将要运行的是否到时间，如果到了，将它添加到运行队列
    if(pdivoom_user_define_schedule_handle->pwait_first_run)
    {
        divoom_print("wait:id: %d, time: %d, %d, %d\n",pdivoom_user_define_schedule_handle->pwait_first_run->id,pdivoom_user_define_schedule_handle->pwait_first_run->start_utc_time, cur_time,  pdivoom_user_define_schedule_handle->pwait_first_run->start_utc_time - cur_time);
        if(pdivoom_user_define_schedule_handle->pwait_first_run->start_utc_time <= cur_time)
        {
            if(pdivoom_user_define_schedule_handle->pwait_first_run->cyc_flag == 0)
            {
                divoom_channel_user_define_schedule_info info[3] = {0};
                
                pdivoom_user_define_schedule_handle->pwait_first_run->active = 0;   
                if(pdivoom_user_define_schedule_handle->pwait_first_run->id <= DIVOOM_USER_DEFINE_SCHEDULE_ITEM_CUSTOM2)
                {
                    int i;                    
                    Pdivoom_user_define_schedule_item pfind_item;
                    divoom_user_define_schedule_item puser_define_item ;
                    
                    for(i = 0; i < 3; i++)
                    {
                        puser_define_item.id = i;
                        pfind_item = divoom_queue_get_cmp(pdivoom_user_define_schedule_handle->wait_queue, &puser_define_item, divoom_user_define_schedule_compare_item);    
                        if(pfind_item == NULL)
                        {
                            pfind_item = divoom_queue_get_cmp(pdivoom_user_define_schedule_handle->run_queue, &puser_define_item, divoom_user_define_schedule_compare_item);    
                        }
                        if(pfind_item)
                        {
                            info[i].start_time = 60*(pfind_item->start_hour * 60 + pfind_item->start_min);
                            info[i].end_time = 60*(pfind_item->end_hour * 60 + pfind_item->end_min);    //结束时间
                            info[i].cyc_flag = pfind_item->cyc_flag;
                            info[i].id = i;
                            if(pfind_item->id == pdivoom_user_define_schedule_handle->pwait_first_run->id)
                            {
                                info[i].status = 0;
                            }
                            else
                            {
                                info[i].status = pfind_item->active;
                            }
                            info[i].voice_flag = pfind_item->voice_flag;
                        }
                        else
                        {
                            
                        }
                    }
                    divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_SET_CUSTOM_SCHEDULE_TIME, DIVOOM_SERVER_TOPIC,info);
                }
                else if(pdivoom_user_define_schedule_handle->pwait_first_run->id == DIVOOM_USER_DEFINE_SCHEDULE_ITEM_EQ)
                {
                    info[0].start_time = 60*(pdivoom_user_define_schedule_handle->pwait_first_run->start_hour * 60 + pdivoom_user_define_schedule_handle->pwait_first_run->start_min);
                    info[0].end_time = 60*(pdivoom_user_define_schedule_handle->pwait_first_run->end_hour * 60 + pdivoom_user_define_schedule_handle->pwait_first_run->end_min);    //结束时间
                    info[0].cyc_flag = 0;
                    info[0].id = pdivoom_user_define_schedule_handle->pwait_first_run->id;
                    info[0].status = 0;
                    info[0].voice_flag = pdivoom_user_define_schedule_handle->pwait_first_run->voice_flag;
                    divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_SET_EQ_SCHEDULE_TIME, DIVOOM_SERVER_TOPIC,info);
                }
                else
                {
                    info[0].start_time = 60*(pdivoom_user_define_schedule_handle->pwait_first_run->start_hour * 60 + pdivoom_user_define_schedule_handle->pwait_first_run->start_min);
                    info[0].end_time = 60*(pdivoom_user_define_schedule_handle->pwait_first_run->end_hour * 60 + pdivoom_user_define_schedule_handle->pwait_first_run->end_min);    //结束时间
                    info[0].cyc_flag = 0;
                    info[0].id = pdivoom_user_define_schedule_handle->pwait_first_run->id;
                    info[0].status = 0;
                    info[0].voice_flag = pdivoom_user_define_schedule_handle->pwait_first_run->voice_flag;
                    divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_SET_SUBSCRIBE_SCHEDULE_TIME, DIVOOM_SERVER_TOPIC,info);
                }
            }
            divoom_queue_pop_item(pdivoom_user_define_schedule_handle->wait_queue, pdivoom_user_define_schedule_handle->pwait_first_run);
            divoom_queue_push(pdivoom_user_define_schedule_handle->run_queue, pdivoom_user_define_schedule_handle->pwait_first_run);
            //如果没有正在运行的，或者当前运行的优先级小于刚添加的，将运行新的任务，
            if(pdivoom_user_define_schedule_handle->pcur_run_item == NULL ||
                pdivoom_user_define_schedule_handle->pcur_run_item->pri <= pdivoom_user_define_schedule_handle->pwait_first_run->pri)
            {
                divoom_user_define_schedule_request_run(pdivoom_user_define_schedule_handle->pwait_first_run);
            }
            pdivoom_user_define_schedule_handle->pwait_first_run = NULL;
            divoom_user_define_schedule_get_next_run_item();
        }
    }

    //检查正在运行的是否到结束时间了
    if(pdivoom_user_define_schedule_handle->pcur_run_item)
    {
        divoom_user_define_schedule_del_time_out_item(cur_time);
        divoom_print("end: %d, %d, %d\n",pdivoom_user_define_schedule_handle->pcur_run_item->end_utc_time, cur_time,  pdivoom_user_define_schedule_handle->pcur_run_item->end_utc_time - cur_time);
        if(pdivoom_user_define_schedule_handle->pcur_run_item->end_utc_time < cur_time)
        {
            Pdivoom_user_define_schedule_item pnew_item;
            //当前运行的到期，将它移出运行队列
            if(pdivoom_user_define_schedule_handle->pcur_run_item->cyc_flag == 0)
            {
                pdivoom_user_define_schedule_handle->pcur_run_item->active = 0;         
            }
            divoom_queue_pop_item(pdivoom_user_define_schedule_handle->run_queue, pdivoom_user_define_schedule_handle->pcur_run_item);
            divoom_user_define_schedule_format_run_time(pdivoom_user_define_schedule_handle->pcur_run_item, 1);
            divoom_queue_push(pdivoom_user_define_schedule_handle->wait_queue, pdivoom_user_define_schedule_handle->pcur_run_item);
            divoom_user_define_schedule_get_next_run_item();
            pnew_item = divoom_user_define_schedule_get_cur_run_item(cur_time);
            divoom_user_define_schedule_request_run(pnew_item);
        }
    }
    divoom_system_free_lock(pdivoom_user_define_schedule_handle->lock);
}
#endif


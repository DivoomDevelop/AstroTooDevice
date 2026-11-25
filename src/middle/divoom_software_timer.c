#include "divoom.h"

#define DIVOOM_SOFTWARE_TIMER_MAX_NUM  100

typedef struct _divoom_software_timer_item
{
    void *para;
    DIVOOM_SOFTWARE_TIMER_CALLBACK *pcallback;
#ifdef DIVOOM_SIMULATOR
    int time_fd;
    uint64_t expire_at_ms;
#else
    int run_time;
    int cancelled;
#endif
}divoom_software_timer_item, *Pdivoom_software_timer_item;

typedef struct _divoom_software_timer_ctrl
{
    int user_num;
    void *lock_handle;
    Pdivoom_software_timer_item time_queue[DIVOOM_SOFTWARE_TIMER_MAX_NUM];
    
}divoom_software_timer_ctrl, *Pdivoom_software_timer_ctrl;

Pdivoom_software_timer_ctrl pdivoom_software_timer_handle = NULL;

#ifdef DIVOOM_SIMULATOR
#include <sys/timerfd.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

static void divoom_software_timer_close_fd(int fd)
{
    if (sim_timerfd_is_managed(fd))
    {
        sim_timerfd_release(fd);
    }
}

void *divoom_software_timer_main_loop(void *param)
{
    (void)param;
    while(1)
    {
        if(pdivoom_software_timer_handle->user_num)
        {
            int find = 0, max_fd = -1;
            fd_set read_fds;
            struct timeval timeout;
            
            FD_ZERO(&read_fds);
            
            divoom_system_get_lock(pdivoom_software_timer_handle->lock_handle);
            for(int i= 0; i < DIVOOM_SOFTWARE_TIMER_MAX_NUM && find < pdivoom_software_timer_handle->user_num; i++)
            {
                if(pdivoom_software_timer_handle->time_queue[i])
                {
                    FD_SET(pdivoom_software_timer_handle->time_queue[i]->time_fd, &read_fds);
                    if(pdivoom_software_timer_handle->time_queue[i]->time_fd > max_fd)
                    {
                        max_fd = pdivoom_software_timer_handle->time_queue[i]->time_fd;
                    }
                    find ++;
                }                
            }
            if(find)
            {
                timeout.tv_sec = 0;
                timeout.tv_usec = 100*1000;
                divoom_system_free_lock(pdivoom_software_timer_handle->lock_handle);
                int ret = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
                if (ret > 0)
                {             
                    divoom_print("time is ready!\n");
                    divoom_system_get_lock(pdivoom_software_timer_handle->lock_handle);
                    for(int i= 0; i < DIVOOM_SOFTWARE_TIMER_MAX_NUM; i++)
                    {
                        if(pdivoom_software_timer_handle->time_queue[i])
                        {
                            if (FD_ISSET(pdivoom_software_timer_handle->time_queue[i]->time_fd, &read_fds)) 
                            {
                                divoom_print("find : %d\n", pdivoom_software_timer_handle->time_queue[i]->time_fd);
                                if(pdivoom_software_timer_handle->time_queue[i]->pcallback)
                                {
                                    pdivoom_software_timer_handle->time_queue[i]->pcallback(pdivoom_software_timer_handle->time_queue[i]->para);
                                }
                                divoom_software_timer_close_fd(pdivoom_software_timer_handle->time_queue[i]->time_fd);
                                divoom_system_free(pdivoom_software_timer_handle->time_queue[i]);
                                pdivoom_software_timer_handle->time_queue[i] = NULL;
                                pdivoom_software_timer_handle->user_num --;
                            }
                        }                
                    }
                    divoom_system_free_lock(pdivoom_software_timer_handle->lock_handle);
                }
            }
            else
            {
                divoom_system_free_lock(pdivoom_software_timer_handle->lock_handle);
            }
        }
        divoom_system_delay_ms(100);
    }
}

#if defined(_WIN32)
#include <windows.h>
#endif

void divoom_software_timer_poll(void)
{
    uint64_t now_ms;
    int i;

    if (pdivoom_software_timer_handle == NULL || pdivoom_software_timer_handle->user_num == 0) {
        return;
    }

#if defined(_WIN32)
    now_ms = (uint64_t)GetTickCount64();
#else
    now_ms = (uint64_t)divoom_system_get_tickms();
#endif
    divoom_system_get_lock(pdivoom_software_timer_handle->lock_handle);
    for (i = 0; i < DIVOOM_SOFTWARE_TIMER_MAX_NUM; i++)
    {
        Pdivoom_software_timer_item item = pdivoom_software_timer_handle->time_queue[i];
        if (item == NULL || now_ms < item->expire_at_ms) {
            continue;
        }
        DIVOOM_SOFTWARE_TIMER_CALLBACK *cb = item->pcallback;
        void *para = item->para;
        int fd = item->time_fd;
        pdivoom_software_timer_handle->time_queue[i] = NULL;
        pdivoom_software_timer_handle->user_num--;
        divoom_system_free(item);
        divoom_system_free_lock(pdivoom_software_timer_handle->lock_handle);
        if (cb) {
            cb(para);
        }
        divoom_software_timer_close_fd(fd);
        divoom_system_get_lock(pdivoom_software_timer_handle->lock_handle);
    }
    divoom_system_free_lock(pdivoom_software_timer_handle->lock_handle);
}

#else /* F101 FreeRTOS：无 timerfd，用一次性任务延时 */

static void divoom_software_timer_remove_item(Pdivoom_software_timer_item ptimer_item)
{
    int i;

    if (pdivoom_software_timer_handle == NULL || ptimer_item == NULL)
    {
        return;
    }

    divoom_system_get_lock(pdivoom_software_timer_handle->lock_handle);
    for (i = 0; i < DIVOOM_SOFTWARE_TIMER_MAX_NUM; i++)
    {
        if (pdivoom_software_timer_handle->time_queue[i] == ptimer_item)
        {
            pdivoom_software_timer_handle->time_queue[i] = NULL;
            pdivoom_software_timer_handle->user_num--;
            break;
        }
    }
    divoom_system_free_lock(pdivoom_software_timer_handle->lock_handle);
    divoom_system_free(ptimer_item);
}

static void *divoom_software_timer_rtos_task(void *param)
{
    Pdivoom_software_timer_item ptimer_item = (Pdivoom_software_timer_item)param;
    DIVOOM_SOFTWARE_TIMER_CALLBACK *pcallback;
    void *para;

    if (ptimer_item == NULL)
    {
        return NULL;
    }

    divoom_system_delay_ms((uint32)ptimer_item->run_time * 1000u);
    if (ptimer_item->cancelled == 0 && ptimer_item->pcallback)
    {
        pcallback = ptimer_item->pcallback;
        para = ptimer_item->para;
        divoom_software_timer_remove_item(ptimer_item);
        pcallback(para);
        return NULL;
    }

    divoom_software_timer_remove_item(ptimer_item);
    return NULL;
}

void divoom_software_timer_poll(void)
{
}

#endif

void divoom_software_timer_init(void)
{
    pdivoom_software_timer_handle = divoom_system_malloc(sizeof(divoom_software_timer_ctrl));
    pdivoom_software_timer_handle->lock_handle = divoom_system_create_lock();
#ifdef DIVOOM_SIMULATOR
#if SIM_USE_REAL_THREADS
    divoom_system_create_task(divoom_software_timer_main_loop, "sw_timer", pdivoom_software_timer_handle, DIVOOM_TASK_STACK_SW_TIMER_MAIN);
#endif
#endif
}

void *divoom_software_timer_create_timer(int run_time, DIVOOM_SOFTWARE_TIMER_CALLBACK pcallback,void *para)
{
    Pdivoom_software_timer_item ptimer_item;
    int i;

    if(pdivoom_software_timer_handle == NULL || run_time <= 0)
    {
        divoom_print("err at : run_time:%d\n", run_time);
        return NULL;
    }
    if(pdivoom_software_timer_handle->user_num >= DIVOOM_SOFTWARE_TIMER_MAX_NUM)
    {
        divoom_print("err at : full and exit\n");
        return NULL;
    }
    ptimer_item = divoom_system_malloc(sizeof(divoom_software_timer_item));
    if(ptimer_item == NULL)
    {
        return NULL;
    }
    ptimer_item->pcallback = pcallback;
    ptimer_item->para = para;

#ifdef DIVOOM_SIMULATOR
    {
        struct itimerspec new_value;

#if defined(_WIN32)
        ptimer_item->expire_at_ms = (uint64_t)GetTickCount64() + (uint64_t)run_time * 1000u;
#else
        ptimer_item->expire_at_ms = (uint64_t)divoom_system_get_tickms() + (uint64_t)run_time * 1000u;
#endif
        ptimer_item->time_fd = timerfd_create(CLOCK_REALTIME, 0);
        if (ptimer_item->time_fd == -1) {
            divoom_print("timerfd_create");
            divoom_system_free(ptimer_item);
            return NULL;
        }

        new_value.it_value.tv_sec = run_time;
        new_value.it_value.tv_nsec = 0;
        new_value.it_interval.tv_sec = 0;
        new_value.it_interval.tv_nsec = 0;

        if (timerfd_settime(ptimer_item->time_fd, 0, &new_value, NULL) == -1) {
            divoom_print("timerfd_settime");
            divoom_software_timer_close_fd(ptimer_item->time_fd);
            divoom_system_free(ptimer_item);
            return NULL;
        }
    }
#else
    ptimer_item->run_time = run_time;
    ptimer_item->cancelled = 0;
    divoom_system_create_task(divoom_software_timer_rtos_task, "sw_timer_rt", ptimer_item, DIVOOM_TASK_STACK_SW_TIMER_RTOS);
#endif

    divoom_system_get_lock(pdivoom_software_timer_handle->lock_handle);
    for(i = 0; i < DIVOOM_SOFTWARE_TIMER_MAX_NUM; i ++)
    {
        if(pdivoom_software_timer_handle->time_queue[i] == NULL)
        {
            pdivoom_software_timer_handle->time_queue[i] = ptimer_item;
            break;
        }
    }
    divoom_system_free_lock(pdivoom_software_timer_handle->lock_handle);
    pdivoom_software_timer_handle->user_num ++;
    divoom_print("create software timer: %d %d\n", pdivoom_software_timer_handle->user_num, run_time);
    return ptimer_item;
}

void divoom_software_timer_delete_timer(void *handle)
{
    Pdivoom_software_timer_item ptimer_item = (Pdivoom_software_timer_item)handle;
    int i;
    
    if (ptimer_item == NULL)
    {
        return;
    }

    divoom_system_get_lock(pdivoom_software_timer_handle->lock_handle);
    for(i = 0; i < DIVOOM_SOFTWARE_TIMER_MAX_NUM; i ++)
    {
        if(pdivoom_software_timer_handle->time_queue[i] == handle)
        {
#ifdef DIVOOM_SIMULATOR
            divoom_software_timer_close_fd(pdivoom_software_timer_handle->time_queue[i]->time_fd);
            divoom_system_free(handle);
#else
            ptimer_item->cancelled = 1;
#endif
            pdivoom_software_timer_handle->time_queue[i] = NULL;
            pdivoom_software_timer_handle->user_num --;
            break;
        }
    }
    
    divoom_system_free_lock(pdivoom_software_timer_handle->lock_handle);

#ifdef DIVOOM_SIMULATOR
#else
    if (ptimer_item->cancelled)
    {
        /* RTOS 任务醒来后自行释放；此处仅标记取消 */
    }
#endif
}

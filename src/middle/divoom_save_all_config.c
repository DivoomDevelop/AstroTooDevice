#include "divoom.h"

#define DIVOOM_SAVE_ALL_CONFIG_DELAY_TIME   5
typedef struct _divoom_save_all_config_item
{
    int run_time;
    DIVOOM_SAVE_ALL_CONFIG_CALLBACK *pcallback;
}divoom_save_all_config_item, *Pdivoom_save_all_config_item;

typedef struct _divoom_save_all_config_ctrl
{
    void *wait_sem;
    void *task_handle;
    void *config_queue;
}divoom_save_all_config_ctrl, *Pdivoom_save_all_config_ctrl;


Pdivoom_save_all_config_ctrl pdivoom_save_all_config_handle = NULL;

int divoom_save_all_config_find(void *item1, void *item2)
{
    int *cur_time = (int *)item1;
    Pdivoom_save_all_config_item check_item = (Pdivoom_save_all_config_item)item2;

    return !(check_item->run_time >= *cur_time);
}


void *divoom_save_all_config_main(void *data)
{
    int cur_time;
    Pdivoom_save_all_config_item pcallback;
    
    while(1)
    {
        divoom_system_get_sem(pdivoom_save_all_config_handle->wait_sem);
        while(divoom_queue_items(pdivoom_save_all_config_handle->config_queue))
        {
            cur_time = time(0);
            pcallback = divoom_queue_pop_cmp(pdivoom_save_all_config_handle->config_queue, &cur_time, divoom_save_all_config_find);
            if(pcallback)
            {
                pcallback->pcallback();
                divoom_system_free(pcallback);
            }
            divoom_system_delay_ms(1000);
        }
    }
    return NULL;
}
void divoom_save_all_config_init(void)
{
    pdivoom_save_all_config_handle = divoom_system_malloc(sizeof(divoom_save_all_config_ctrl));
    pdivoom_save_all_config_handle->wait_sem = divoom_system_create_sem(1, 0);
    pdivoom_save_all_config_handle->config_queue = divoom_queue_init(1000, __func__, __LINE__);
    //divoom_system_create_task(divoom_save_all_config_main, "save_config", NULL, DIVOOM_TASK_STACK_SAVE_ALL_CONFIG);
}

int divoom_save_all_config_add_compare(void *item1, void *item2)
{

    DIVOOM_SAVE_ALL_CONFIG_CALLBACK *pcallback = (DIVOOM_SAVE_ALL_CONFIG_CALLBACK *)item1;
    Pdivoom_save_all_config_item check_item = (Pdivoom_save_all_config_item)item2;

    return pcallback != check_item->pcallback;
}

void divoom_save_all_config_add(DIVOOM_SAVE_ALL_CONFIG_CALLBACK *pcallback)
{
    Pdivoom_save_all_config_item pfind_item;
    
    if(pdivoom_save_all_config_handle)
    {
        pfind_item = divoom_queue_get_cmp(pdivoom_save_all_config_handle->config_queue, (void *)pcallback, divoom_save_all_config_add_compare);
        if(pfind_item == NULL)
        {
            pfind_item = divoom_system_malloc(sizeof(divoom_save_all_config_item));
            if(pfind_item)
            {
                pfind_item->pcallback = pcallback;
                pfind_item->run_time = time(0) + DIVOOM_SAVE_ALL_CONFIG_DELAY_TIME;
                divoom_queue_push(pdivoom_save_all_config_handle->config_queue, (void *)pfind_item);
                divoom_system_put_sem(pdivoom_save_all_config_handle->wait_sem);
            }
            else
            {
                pcallback();
            }
        }
        else
        {
            pfind_item->run_time = time(0) + DIVOOM_SAVE_ALL_CONFIG_DELAY_TIME;
        }
    }
}

void divoom_save_all_config_add_sync_run(DIVOOM_SAVE_ALL_CONFIG_CALLBACK *pcallback)
{
    Pdivoom_save_all_config_item pfind_item;
    
    if(pdivoom_save_all_config_handle)
    {
        pfind_item = divoom_queue_get_cmp(pdivoom_save_all_config_handle->config_queue, (void *)pcallback, divoom_save_all_config_add_compare);
        if(pfind_item == NULL)
        {
            pfind_item = divoom_system_malloc(sizeof(divoom_save_all_config_item));
            if(pfind_item)
            {
                pfind_item->pcallback = pcallback;
                pfind_item->run_time = time(0);
                divoom_queue_push(pdivoom_save_all_config_handle->config_queue, (void *)pfind_item);
                divoom_system_put_sem(pdivoom_save_all_config_handle->wait_sem);
            }
            else
            {
                pcallback();
            }
        }
        else
        {
            pfind_item->run_time = time(0);
        }
    }
}



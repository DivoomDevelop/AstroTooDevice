#include "divoom.h"



typedef struct _divoom_system_lock_item
{
    void *lock_handle;
    char *func_name;
    int   func_line;
}divoom_system_lock_item, *Pdivoom_system_lock_item;

static void *lock_handle_queue = NULL;

void divoom_check_system_lock_init(void)
{
    lock_handle_queue = divoom_queue_init(10000, __func__, __LINE__);
}


void divoom_check_system_lock_add(void *lock_handle, char *func_name, int func_line)
{
    Pdivoom_system_lock_item lock_item;

    if(lock_handle_queue == NULL)
    {
        return;
    }
    lock_item = divoom_system_malloc(sizeof(divoom_system_lock_item));
    if(lock_item)
    {
        lock_item->lock_handle = lock_handle;
        lock_item->func_name = func_name;
        lock_item->func_line = func_line;
        divoom_queue_push(lock_handle_queue, lock_item);
    }
}

int divoom_check_system_lock_remove_compare(void *item1, void *item2)
{
    Pdivoom_system_lock_item lock_item = (Pdivoom_system_lock_item)item2;

    return item1 == lock_item->lock_handle?0:1;
}

void divoom_check_system_lock_remove(void *lock_handle)
{
    Pdivoom_system_lock_item lock_item;

    if(lock_handle_queue == NULL)
    {
        return;
    }

    lock_item = divoom_queue_pop_cmp(lock_handle_queue, lock_handle, divoom_check_system_lock_remove_compare);

    if(lock_item)
    {
        divoom_system_free(lock_item);
    }
}


void divoom_check_system_lock_do_dead_lock(void *lock_handle, char *func_name, int func_line)
{
    Pdivoom_system_lock_item lock_item1;
    
    if(lock_handle_queue == NULL)
    {
        return;
    }
    
    lock_item1 = divoom_queue_pop_cmp(lock_handle_queue, lock_handle, divoom_check_system_lock_remove_compare);

    if(lock_item1)
    {
        char *lock_string = divoom_system_malloc(1024);
        
        divoom_print("system err: lock dead: %s: %d\n", lock_item1->func_name, lock_item1->func_line);
        divoom_print("system err: request line: %s: %d\n", func_name, func_line);
        sprintf(lock_string, "dead lock:%s:%d; request:%s:%d\n", lock_item1->func_name, lock_item1->func_line, func_name, func_line);
        divoom_main_update_dead_lock_file(lock_string);
        divoom_system_free(lock_string);
    }
    else
    {
        divoom_print("system err: not find: %s: %d\n", func_name, func_line);
    }
}




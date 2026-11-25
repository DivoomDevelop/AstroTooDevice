
#include "divoom.h"



struct _divoom_queue_note_info;

typedef struct _divoom_queue_note_info
{
    void *data;
    struct _divoom_queue_note_info *next;
}divoom_queue_note_info, *p_divoom_queue_note_info;

typedef struct _divoom_queue_con_info
{
    char *file_name;
    int  file_line;
    unsigned short total_item;      //队列中有的当前保存的数据项
    unsigned short max_item;        //队列最大储存项
    void *queue_mutex;
    p_divoom_queue_note_info head;
    p_divoom_queue_note_info find;
    p_divoom_queue_note_info tail;
}divoom_queue_con_info, *p_divoom_queue_con_info;




/**
 * NAME         divoom_queue_init
 * @BRIEF	队列初始化
 * @PARAM	max_item : 队列中最大储存项
 
 * @RETURN	队列句柄
 * @RETVAL	
 */



void * divoom_queue_init(int max_item, const char *file_name,int file_line)
{
    p_divoom_queue_con_info new_queue;

    new_queue = divoom_system_malloc(sizeof(divoom_queue_con_info));
    if(NULL == new_queue)
    {
        divoom_print("[%s:%s] it fails to malloc!\n", __FILE__, __func__);
        return NULL;
    }
    new_queue->file_name = file_name;
    new_queue->file_line = file_line;
    new_queue->head = NULL;
    new_queue->tail = NULL;
    new_queue->find = NULL;
    new_queue->total_item = 0;
    new_queue->max_item = max_item;
    new_queue->queue_mutex = divoom_system_create_lock();
    if (new_queue->queue_mutex == NULL)
    {
        divoom_system_free(new_queue);
        return NULL;
    }
    //divoom_print("[divoom_queue_init:], the queue has been created!(max item:%d)\n", max_item);
    return new_queue;
}



/**
 * NAME         divoom_queue_destroy
 * @BRIEF	队列释放，将释放队列中所有未处理的数据项，只释放相应内存
                    系统将丢失所有数据
 * @PARAM	queue_handle : 队列句柄
                    queue_free     :队列释放数据项的内存回调
 
 * @RETURN	NONE
 * @RETVAL	
 */

void divoom_queue_destroy(void * queue_handle, divoom_queue_free *queue_free)
{
    p_divoom_queue_con_info queue_info = (p_divoom_queue_con_info)queue_handle;
    p_divoom_queue_note_info head, next;

    if(queue_handle == NULL)
    {
        return;
    }
    divoom_system_get_lock_direct(queue_info->queue_mutex);
    head = queue_info->head;
    while(head)
    {
        next = head->next;
        if(queue_free)
        {
            queue_free(head->data);
        }
        else
        {
            divoom_system_free(head->data);
        }
        divoom_system_free(head);
        head = next;        
    }
    divoom_system_free_lock_direct(queue_info->queue_mutex);
    
	divoom_system_destroy_lock(queue_info->queue_mutex);
    divoom_system_free(queue_info);
    //divoom_print("[divoom_queue_destroy:], the queue has been destroy!\n");
    
}


/**
 * NAME         divoom_queue_pop_all
 * @BRIEF	将释放队列中所有未处理的数据项，只释放相应内存
                    系统将丢失所有数据
 * @PARAM	queue_handle : 队列句柄
                    queue_free     :队列释放数据项的内存回调
 
 * @RETURN	NONE
 * @RETVAL	
 */

void divoom_queue_pop_all(void * queue_handle, divoom_queue_free *queue_free)
{
    p_divoom_queue_con_info queue_info = (p_divoom_queue_con_info)queue_handle;
    p_divoom_queue_note_info head, next;

    if(queue_handle == NULL)
    {
        return;
    }
    divoom_system_get_lock_direct(queue_info->queue_mutex);
    head = queue_info->head;
    while(head)
    {
        next = head->next;
        if(queue_free)
        {
            queue_free(head->data);
        }
        else
        {
            divoom_system_free(head->data);
        }
        divoom_system_free(head);
        head = next;        
    }
    queue_info->head = NULL;
    queue_info->tail = NULL;
    queue_info->find = NULL;
    queue_info->total_item = 0;
    divoom_system_free_lock_direct(queue_info->queue_mutex);
        
}

/**
 * NAME         divoom_queue_get_tail_note
 * @BRIEF	返回队列尾的数据项，并不解除在队列的位置
 * @PARAM	queue_handle : 队列句柄
 
 * @RETURN	队列尾数据项
 * @RETVAL	
 */

void* divoom_queue_get_tail_note(void * queue_handle)
{
    p_divoom_queue_con_info queue_info = (p_divoom_queue_con_info)queue_handle;
    
    if(queue_handle == NULL)
    {
        return NULL;
    }
    if(queue_info->tail)
    {
        return queue_info->tail->data;
    }
    else
    {
        return NULL;
    }
}


/**
 * NAME         divoom_queue_get_head_note
 * @BRIEF	返回队列头的数据项，并不解除在队列的位置
 * @PARAM	queue_handle : 队列句柄
 
 * @RETURN	队列头数据项
 * @RETVAL	
 */

void* divoom_queue_get_head_note(void * queue_handle)
{
    p_divoom_queue_con_info queue_info = (p_divoom_queue_con_info)queue_handle;
    
    if(queue_handle == NULL)
    {
        return NULL;
    }
    if(queue_info->head)
    {
        return queue_info->head->data;
    }
    else
    {
        return NULL;
    }
}

/**
 * NAME         divoom_queue_push_head
 * @BRIEF	将用户数据插入队列的头部
 * @PARAM	queue_handle : 队列句柄
                    data  用户数据指针
 * @RETURN	队列尾数据项
 * @RETVAL	0-->fail; 1-->ok
 */

uint8 divoom_queue_push_head(void * queue_handle, void *data)
{
	p_divoom_queue_con_info queue_info = (p_divoom_queue_con_info)queue_handle;

	p_divoom_queue_note_info new_note;

	if(queue_handle == NULL)
	{
		return 0;
	}

	divoom_system_get_lock_direct(queue_info->queue_mutex);

	if(queue_info->total_item >= queue_info->max_item)
	{
		divoom_print("[%s] the queue items is too many(max:%d) at: %s:%d!\n", __func__, queue_info->max_item, queue_info->file_name, queue_info->file_line);
		divoom_system_free_lock_direct(queue_info->queue_mutex);
		return 0;
	}
	
	new_note= divoom_system_malloc(sizeof(divoom_queue_note_info));
	if(new_note == NULL)
	{
		divoom_print("[%s] it fails to malloc!: %s:%d\n", __func__, queue_info->file_name, queue_info->file_line);
		divoom_system_free_lock_direct(queue_info->queue_mutex);
		return 0;
	}
	queue_info->total_item++;
	new_note->data = data;
	new_note->next = queue_info->head;
	
	queue_info->head = new_note;

	if (queue_info->tail == NULL)
	{
		queue_info->tail = new_note;
	}

	divoom_system_free_lock_direct(queue_info->queue_mutex);
	return 1;
}

/**
 * NAME         divoom_queue_push
 * @BRIEF	将用户数据放入队列中
 * @PARAM	queue_handle : 队列句柄
                    data  用户数据指针
 * @RETURN	队列尾数据项
 * @RETVAL	0-->fail; 1-->ok
 */

uint8 divoom_queue_push(void * queue_handle, void *data)
{
    
    p_divoom_queue_con_info queue_info = (p_divoom_queue_con_info)queue_handle;

    p_divoom_queue_note_info new_note;

    
    if(queue_handle == NULL)
    {
        return 0;
    }
    
    divoom_system_get_lock_direct(queue_info->queue_mutex);

    if(queue_info->total_item >= queue_info->max_item)
    {
        divoom_print("[%s] the queue items is too many(max:%d)!at: %s:%d\n", __func__, queue_info->max_item, queue_info->file_name, queue_info->file_line);
        
        divoom_system_free_lock_direct(queue_info->queue_mutex);
        return 0;
    }
    queue_info->total_item ++;
    new_note= divoom_system_malloc(sizeof(divoom_queue_note_info));
    if(new_note == NULL)
    {
        divoom_print("[%s] it fails to malloc!: %s:%d\n", __func__, queue_info->file_name, queue_info->file_line);
        queue_info->total_item --;
        divoom_system_free_lock_direct(queue_info->queue_mutex);
        return 0;
    }
    new_note->data = data;
    new_note->next = NULL;

    if(queue_info->tail)
    {
        queue_info->tail->next = new_note;
        queue_info->tail = new_note;
    }
    else
    {
        queue_info->tail = new_note;        
        queue_info->head = new_note;
    } 

    divoom_system_free_lock_direct(queue_info->queue_mutex);
    return 1;
    
}

//根据规则进行 push 
uint8 divoom_queue_push_cmp(void * queue_handle, void *data, divoom_queue_compare *pcmp_chk)
{
	int ret = 0;
    p_divoom_queue_con_info queue_info = (p_divoom_queue_con_info)queue_handle;

    p_divoom_queue_note_info new_note, cur_note, pre_item = NULL;

    
    if(queue_handle == NULL)
    {
        return 0;
    }
    
    divoom_system_get_lock_direct(queue_info->queue_mutex);

    if(queue_info->total_item >= queue_info->max_item)
    {
        divoom_print("[%s] the queue items is too many(max:%d)!at: %s:%d\n", __func__, queue_info->max_item, queue_info->file_name, queue_info->file_line);
        
        divoom_system_free_lock_direct(queue_info->queue_mutex);
        return 0;
    }
    queue_info->total_item ++;
    new_note= divoom_system_malloc(sizeof(divoom_queue_note_info));
    if(new_note == NULL)
    {
        divoom_print("[%s] it fails to malloc!: %s:%d\n", __func__, queue_info->file_name, queue_info->file_line);
        divoom_system_free_lock_direct(queue_info->queue_mutex);
        return 0;
    }
    new_note->data = data;
    new_note->next = NULL;

	cur_note = queue_info->head;
	while(cur_note)
	{
		ret = pcmp_chk(data, cur_note->data);
		if(ret == 1)
		{
			//头部
			if(pre_item != NULL)
			{
				pre_item->next = new_note;
				new_note->next = cur_note;
			}
			else
			{
				new_note->next = cur_note;
				queue_info->head = new_note;
			}
			break;
		}
		pre_item = cur_note;
		cur_note = cur_note->next;
	}
	//在前面所有的都没找到,就放在结尾部分
	if(ret == 0)
	{
		queue_info->tail->next = new_note;
		queue_info->tail = new_note;
	}

    divoom_system_free_lock_direct(queue_info->queue_mutex);
    return 1;
}

/**
 * NAME         divoom_queue_pop
 * @BRIEF	将用户数据返回给用户，队列中弹出此数据项
 * @PARAM	queue_handle : 队列句柄
                    
 * @RETURN	返回队列头数据
 * @RETVAL	
 */

void * divoom_queue_pop(void * queue_handle)
{
    p_divoom_queue_con_info queue_info = (p_divoom_queue_con_info)queue_handle;
    void *data;
    p_divoom_queue_note_info head;

    
    if(queue_handle == NULL)
    {
        return NULL;
    }
    

    divoom_system_get_lock_direct(queue_info->queue_mutex);
    if(NULL == queue_info->head)
    {        
        divoom_system_free_lock_direct(queue_info->queue_mutex);
        return NULL;
    }

    queue_info->total_item --;
    
    data = queue_info->head->data;
    head = queue_info->head;
    queue_info->head = queue_info->head->next;

    if(queue_info->head == NULL)
    {
        queue_info->head = NULL;
        queue_info->tail = NULL;
    }
    if(head == queue_info->find)
    {
        queue_info->find = queue_info->head;
    }

    divoom_system_free_lock_direct(queue_info->queue_mutex);

    divoom_system_free(head);
    return data;

}



/**
 * NAME         divoom_queue_pop_tail
 * @BRIEF	将用户数据返回给用户，队列中弹出此数据项
 * @PARAM	queue_handle : 队列句柄
                    
 * @RETURN	返回队列尾数据
 * @RETVAL	
 */

void * divoom_queue_pop_tail(void * queue_handle)
{
    p_divoom_queue_con_info queue_info = (p_divoom_queue_con_info)queue_handle;
    void *data;
    p_divoom_queue_note_info head, find = NULL;

    
    if(queue_handle == NULL)
    {
        return NULL;
    }
    

    divoom_system_get_lock_direct(queue_info->queue_mutex);
    if(NULL == queue_info->head)
    {        
        divoom_system_free_lock_direct(queue_info->queue_mutex);
        return NULL;
    }

    queue_info->total_item --;
    
    data = queue_info->tail->data;
    head = queue_info->head;
    while(head->next)
    {
        find = head;
        head = head->next;
    }
    if(find)
    {
        queue_info->tail = find;
        find->next = NULL;
    }
    if(find == NULL)
    {
        queue_info->find = NULL;
        queue_info->head = NULL;
        queue_info->tail = NULL;
    }
    else if(head == queue_info->find)
    {
        queue_info->find = find;
    }

    divoom_system_free_lock_direct(queue_info->queue_mutex);

    divoom_system_free(head);
    return data;

}

/**
 * NAME         divoom_queue_sort
 * @BRIEF	对队列中的数据项进行排序
 * @PARAM	queue_handle : 队列句柄
                    compare         :数据项比较回调函数
 * @RETURN	NONE
 * @RETVAL	
 */

void divoom_queue_sort(void * queue_handle, divoom_queue_compare *compare)
{
    int i, total = 0;
    p_divoom_queue_con_info queue_info = (p_divoom_queue_con_info)queue_handle;
    p_divoom_queue_note_info *sorted_queue, head;

    if(queue_handle == NULL)
    {
        return ;
    }
    
    if(queue_info->total_item == 0)
    {
        return;
    }
    
    sorted_queue = divoom_system_malloc(sizeof(p_divoom_queue_note_info) * queue_info->total_item);
    if(sorted_queue == NULL)
    {
        return;
    }
    
    memset(sorted_queue, 0, sizeof(p_divoom_queue_note_info) * queue_info->total_item);
    divoom_system_get_lock_direct(queue_info->queue_mutex);
    head = queue_info->head; 
    while(head)
    {
        for(i = 0; i < total; i++)
        {
            if(compare(head->data, sorted_queue[i]->data) < 0)
            {
                memmove((void *)((unsigned long)sorted_queue + (i + 1)* sizeof(p_divoom_queue_note_info)), 
                    (void *)((unsigned long)sorted_queue + i * sizeof(p_divoom_queue_note_info)), (total - i) * sizeof(p_divoom_queue_note_info));
                break;
            }
        }
        sorted_queue[i] = head;
        total ++;
        head = head->next;
    }
    queue_info->head = sorted_queue[0];
    queue_info->tail = sorted_queue[queue_info->total_item-1];
    queue_info->tail->next = NULL;
    for(i = 1; i < queue_info->total_item; i ++)
    {
        sorted_queue[i-1]->next = sorted_queue[i];
    }
    divoom_system_free(sorted_queue);
    divoom_system_free_lock_direct(queue_info->queue_mutex);
}



/**
 * NAME         divoom_queue_not_empty
 * @BRIEF	判断队列是否为空
 * @PARAM	queue_handle : 队列句柄
 
 * @RETURN	0-->empty; 1-->have
 * @RETVAL	
 */

uint8 divoom_queue_not_empty(void * queue_handle)
{
    p_divoom_queue_con_info queue_info = (p_divoom_queue_con_info)queue_handle;
    uint8 ret; 
    
    if(queue_handle == NULL)
    {
        return 0;
    }

    divoom_system_get_lock_direct(queue_info->queue_mutex);
    ret = queue_info && queue_info->head;
    divoom_system_free_lock_direct(queue_info->queue_mutex);

    return ret;
}


/**
 * NAME         divoom_queue_get_reset
 * @BRIEF	复位查找链表，将其指向头结点
 * @PARAM	queue_handle : 队列句柄
                    
 * @RETURN	
 * @RETVAL	
 */

uint8 divoom_queue_get_reset(void * queue_handle)
{
    uint8 items;
    p_divoom_queue_con_info queue_info = (p_divoom_queue_con_info)queue_handle;
    
    if(queue_handle == NULL)
    {
        return 0;
    }

    divoom_system_get_lock_direct(queue_info->queue_mutex);
    queue_info->find = queue_info->head;
    items = queue_info->total_item;
    divoom_system_free_lock_direct(queue_info->queue_mutex);
    return items;
}




/**
 * NAME         divoom_queue_lock
 * @BRIEF	对队列加锁，以便配合divoom_queue_get_next使用
 * @PARAM	queue_handle : 队列句柄
 * @RETURN	NONE
 * @RETVAL	
 */

void divoom_queue_lock(void * queue_handle)
{
    p_divoom_queue_con_info queue_info = (p_divoom_queue_con_info)queue_handle;
    if(queue_handle == NULL)
    {
        return ;
    }
    divoom_system_get_lock_direct(queue_info->queue_mutex);
}

/**
 * NAME         divoom_queue_unlock
 * @BRIEF	对队列解锁，以便配合divoom_queue_get_next使用
 * @PARAM	queue_handle : 队列句柄
 * @RETURN	NONE
 * @RETVAL	
 */

void divoom_queue_unlock(void * queue_handle)
{
    p_divoom_queue_con_info queue_info = (p_divoom_queue_con_info)queue_handle;
    if(queue_handle == NULL)
    {
        return ;
    }
    divoom_system_free_lock_direct(queue_info->queue_mutex);
}


/**
 * NAME         divoom_queue_get_next
 * @BRIEF	得到队列的相关数据，并不会对队列进行操作
 * @PARAM	queue_handle : 队列句柄
                    item        :用户要使用的比较数据
                    pcmp_chk:用户使用的比较函数,如果为空的话，将不判断条件
                    
 * @RETURN	返回队列查找到的数据
 * @RETVAL	
 */

void *divoom_queue_get_item(void * queue_handle, void *item, divoom_queue_compare *pcmp_chk)
{
    p_divoom_queue_con_info queue_info = (p_divoom_queue_con_info)queue_handle;
    p_divoom_queue_note_info find_item;
    
    if(queue_handle == NULL)
    {
        return NULL;
    }

    while(queue_info->find)
    {
        if(pcmp_chk == NULL || (pcmp_chk(item, queue_info->find->data) == 0))
        {
            find_item = queue_info->find;
            queue_info->find = queue_info->find->next;
            return find_item->data;
        }
        queue_info->find = queue_info->find->next;
    }
    return NULL;
}



/**
 * NAME         divoom_queue_pop_item
 * @BRIEF	将指定项的用户数据返回给用户，队列中弹出此数据项
 * @PARAM	queue_handle : 队列句柄
                    item:使用push传入的用户数据
                    
 * @RETURN	返回队指定的数据项
 * @RETVAL	
 */

void * divoom_queue_pop_item(void * queue_handle, void *item)
{

    p_divoom_queue_con_info queue_info = (p_divoom_queue_con_info)queue_handle;
    p_divoom_queue_note_info find_item, pre_item = NULL;
    void *data;
    
    if(queue_handle == NULL)
    {
        return 0;
    }

    divoom_system_get_lock_direct(queue_info->queue_mutex);
    find_item = queue_info->head;
    while(find_item)
    {
        if((void *)item == (void *)find_item->data)
        {
            break;
        }
        pre_item = find_item;
        find_item = find_item->next;
    }
    
    if(find_item)
    {
        if(find_item == queue_info->find)
        {
            queue_info->find = find_item->next;
        }
        
        if(pre_item == NULL)
        {
            queue_info->head = queue_info->head->next;
        }
        else
        {
            pre_item->next = find_item->next;
        }
        if(find_item == queue_info->tail)
        {
            queue_info->tail = pre_item;
        }
		queue_info->total_item--;
    }
    
    divoom_system_free_lock_direct(queue_info->queue_mutex);
    if(find_item == NULL)
    {
        return NULL;
    }
    data = find_item->data;
    divoom_system_free(find_item);
    return data;
}


/**
 * NAME         divoom_queue_pop
 * @BRIEF	将用户数据返回给用户，队列中弹出此数据项
 * @PARAM	queue_handle : 队列句柄
                    item        :用户要使用的比较数据
                    pcmp_chk:用户使用的比较函数
                    
 * @RETURN	返回队指定的数据项
 * @RETVAL	
 */

void * divoom_queue_pop_cmp(void * queue_handle, void *item, divoom_queue_compare *pcmp_chk)
{

    p_divoom_queue_con_info queue_info = (p_divoom_queue_con_info)queue_handle;
    p_divoom_queue_note_info find_item, pre_item = NULL;
    void *data;
    
    if(queue_handle == NULL)
    {
        return 0;
    }

    divoom_system_get_lock_direct(queue_info->queue_mutex);
    find_item = queue_info->head;
    while(find_item)
    {
        if(pcmp_chk(item ,find_item->data) == 0)
        {
            break;
        }
        pre_item = find_item;
        find_item = find_item->next;
    }
    
    if(find_item)
    {
        if(find_item == queue_info->find)
        {
            queue_info->find = find_item->next;
        }
        
        if(pre_item == NULL)
        {
            queue_info->head = queue_info->head->next;
        }
        else
        {
            pre_item->next = find_item->next;
        }
        if(find_item == queue_info->tail)
        {
            queue_info->tail = pre_item;
        }
		queue_info->total_item--;
    }
    
    divoom_system_free_lock_direct(queue_info->queue_mutex);
    if(find_item == NULL)
    {
        return NULL;
    }
    data = find_item->data;
    divoom_system_free(find_item);
    return data;
}


/**
 * NAME         divoom_queue_get_cmp
 * @BRIEF	将用户数据返回给用户，队列中取出此数据项
 * @PARAM	queue_handle : 队列句柄
                    item        :用户要使用的比较数据
                    pcmp_chk:用户使用的比较函数
                    
 * @RETURN	返回队指定的数据项
 * @RETVAL	
 */

void * divoom_queue_get_cmp(void * queue_handle, void *item, divoom_queue_compare *pcmp_chk)
{

    p_divoom_queue_con_info queue_info = (p_divoom_queue_con_info)queue_handle;
    p_divoom_queue_note_info find_item;
    void *data;
    
    if(queue_handle == NULL)
    {
        return 0;
    }

    divoom_system_get_lock_direct(queue_info->queue_mutex);
    find_item = queue_info->head;
    while(find_item)
    {
        if(pcmp_chk(item ,find_item->data) == 0)
        {
            break;
        }
        find_item = find_item->next;
    }
    
    data = find_item ? find_item->data : NULL;
    divoom_system_free_lock_direct(queue_info->queue_mutex);
    return data;
}

/**
 * NAME         divoom_queue_is_full
 * @BRIEF	判断队列是否为满
 * @PARAM	queue_handle : 队列句柄
 
 * @RETURN	0-->not full; 1-->full
 * @RETVAL	
 */

uint16 divoom_queue_is_full(void * queue_handle)
{
    p_divoom_queue_con_info queue_info = (p_divoom_queue_con_info)queue_handle;
    uint8 ret; 
    
    if(queue_handle == NULL)
    {
        return 0;
    }

    divoom_system_get_lock_direct(queue_info->queue_mutex);
    ret = (queue_info->total_item >= queue_info->max_item);
    divoom_system_free_lock_direct(queue_info->queue_mutex);

    return ret;
}


/**
 * NAME         divoom_queue_is_full
 * @BRIEF	返回队列数据项数目
 * @PARAM	queue_handle : 队列句柄
 
 * @RETURN	数据项数目
 * @RETVAL	
 */

uint16 divoom_queue_items(void * queue_handle)
{
    p_divoom_queue_con_info queue_info = (p_divoom_queue_con_info)queue_handle;
    uint16 ret; 
    
    if(queue_handle == NULL)
    {
        return 0;
    }

    divoom_system_get_lock_direct(queue_info->queue_mutex);
    ret = queue_info->total_item;
    divoom_system_free_lock_direct(queue_info->queue_mutex);

    return ret;
}



/**
 * NAME         divoom_queue_get_index_item
 * @BRIEF	返回指定数据项数据
 * @PARAM	queue_handle : 队列句柄
                    index              :数据项序号
 * @RETURN	用户数据项
 * @RETVAL	
 */

void* divoom_queue_get_index_item(void * queue_handle, int index)
{
    p_divoom_queue_con_info queue_info = (p_divoom_queue_con_info)queue_handle;
    p_divoom_queue_note_info head;
    void *data = NULL;
    
    if(queue_handle == NULL)
    {
        return NULL;
    }

   
   	divoom_system_get_lock_direct(queue_info->queue_mutex);

	if(index < 0 || index >= queue_info->total_item)
    {
		divoom_system_free_lock_direct(queue_info->queue_mutex);
        return NULL;
    }
	
    head = queue_info->head;
	if(head == NULL){
		divoom_system_free_lock_direct(queue_info->queue_mutex);
		return NULL;
	}
		
    while(index && head)
    {
        head = head->next;
        index --;
    }
	
    if(head)
    {
        data = head->data;
    }
    divoom_system_free_lock_direct(queue_info->queue_mutex);
    return data;
}

/**
 * NAME         divoom_queue_get_all_items
 * @BRIEF	返回所有指定数据项数据
 * @PARAM	queue_handle : 队列句柄
                    data              :数据缓存器
                    psave_call          
 * @RETURN	总共多少项
 * @RETVAL	
 */

int divoom_queue_get_all_items(void * queue_handle, void *data, divoom_queue_save *psave_call)
{
    int find_num = 0;
    p_divoom_queue_con_info queue_info = (p_divoom_queue_con_info)queue_handle;
    p_divoom_queue_note_info head;
    
    if(queue_handle == NULL)
    {
        return 0;
    }

   
   	divoom_system_get_lock_direct(queue_info->queue_mutex);

	
    head = queue_info->head;
	if(head == NULL){
		divoom_system_free_lock_direct(queue_info->queue_mutex);
		return 0;
	}
		
    while(head)
    {
        psave_call(data, head->data, find_num);
        head = head->next;
        find_num ++;
    }
	
    divoom_system_free_lock_direct(queue_info->queue_mutex);

    return find_num;
}


/**
 * NAME         divoom_queue_get_index_item_no_lock
 * @BRIEF	返回指定数据项数据
 * @PARAM	queue_handle : 队列句柄
                    index              :数据项序号
 * @RETURN	用户数据项
 * @RETVAL	
 */

void* divoom_queue_get_index_item_no_lock(void * queue_handle, int index)
{
    p_divoom_queue_con_info queue_info = (p_divoom_queue_con_info)queue_handle;
    p_divoom_queue_note_info head;
    
    if(queue_handle == NULL)
    {
        return NULL;
    }

   

	if(index >= queue_info->total_item)
    {
        return NULL;
    }
	
    head = queue_info->head;
	if(head == NULL){
		return NULL;
	}
		
    while(index && head)
    {
        head = head->next;
        index --;
    }
	
	
	if(head)
	{
    	return head->data;
	}
	else
	{
		return NULL;
	}
}


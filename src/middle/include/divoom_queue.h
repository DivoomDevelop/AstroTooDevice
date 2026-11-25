#ifndef _DIVOOM_QUEUE_H_
#define _DIVOOM_QUEUE_H_

typedef void divoom_queue_free(void *item);
typedef int divoom_queue_compare(void *item1, void *item2);
typedef void divoom_queue_save(void *item1, void *item2, int pos);

/**
 * NAME         divoom_queue_init
 * @BRIEF	队列初始化
 * @PARAM	max_item : 队列中最大储存项
 
 * @RETURN	队列句柄
 * @RETVAL	
 */

void * divoom_queue_init(int max_item, const char *file_name,int file_line);


/**
 * NAME         divoom_queue_destroy
 * @BRIEF	队列释放，将释放队列中所有未处理的数据项，只释放相应内存
                    系统将丢失所有数据
 * @PARAM	queue_handle : 队列句柄
                    queue_free     :队列释放数据项的内存回调
 
 * @RETURN	NONE
 * @RETVAL	
 */

void divoom_queue_destroy(void * queue_handle, divoom_queue_free *queue_free);

/**
 * NAME         divoom_queue_pop_all
 * @BRIEF	将释放队列中所有未处理的数据项，只释放相应内存
                    系统将丢失所有数据
 * @PARAM	queue_handle : 队列句柄
                    queue_free     :队列释放数据项的内存回调
 
 * @RETURN	NONE
 * @RETVAL	
 */

void divoom_queue_pop_all(void * queue_handle, divoom_queue_free *queue_free);

/**
 * NAME         divoom_queue_push_head
 * @BRIEF	将用户数据插入队列的头部
 * @PARAM	queue_handle : 队列句柄
                    data  用户数据指针
 * @RETURN	队列尾数据项
 * @RETVAL	0-->fail; 1-->ok
 */

uint8 divoom_queue_push_head(void * queue_handle, void *data);

/**
 * NAME         divoom_queue_push
 * @BRIEF	将用户数据放入队列中
 * @PARAM	queue_handle : 队列句柄
                    data  用户数据指针
 * @RETURN	队列尾数据项
 * @RETVAL	0-->fail; 1-->ok
 */

uint8 divoom_queue_push(void * queue_handle, void *data);

uint8 divoom_queue_push_cmp(void * queue_handle, void *data, divoom_queue_compare *pcmp_chk);

/**
* NAME         divoom_queue_pop_tail
* @BRIEF   将用户数据返回给用户，队列中弹出此数据项
* @PARAM   queue_handle : 队列句柄
                  
* @RETURN  返回队列尾数据
* @RETVAL  
*/

void * divoom_queue_pop_tail(void * queue_handle);


/**
 * NAME         divoom_queue_pop
 * @BRIEF	将用户数据返回给用户，队列中弹出此数据项
 * @PARAM	queue_handle : 队列句柄
                    
 * @RETURN	返回队列头数据
 * @RETVAL	
 */
  

void * divoom_queue_pop(void * queue_handle);

/**
 * NAME         divoom_queue_sort
 * @BRIEF	对队列中的数据项进行排序
 * @PARAM	queue_handle : 队列句柄
                    compare         :数据项比较回调函数
 * @RETURN	NONE
 * @RETVAL	
 */

void divoom_queue_sort(void * queue_handle, divoom_queue_compare *compare);

/**
 * NAME         divoom_queue_not_empty
 * @BRIEF	判断队列是否为空
 * @PARAM	queue_handle : 队列句柄
 
 * @RETURN	0-->empty; 1-->have
 * @RETVAL	
 */

uint8 divoom_queue_not_empty(void * queue_handle);

/**
 * NAME         divoom_queue_is_full
 * @BRIEF	判断队列是否为满
 * @PARAM	queue_handle : 队列句柄
 
 * @RETURN	0-->not full; 1-->full
 * @RETVAL	
 */

uint16 divoom_queue_is_full(void * queue_handle);


/**
 * NAME         divoom_queue_get_tail_note
 * @BRIEF	返回队列尾的数据项，并不解除在队列的位置
 * @PARAM	queue_handle : 队列句柄
 
 * @RETURN	队列尾数据项
 * @RETVAL	
 */

void* divoom_queue_get_tail_note(void * queue_handle);

/**
 * NAME         divoom_queue_get_head_note
 * @BRIEF	返回队列头的数据项，并不解除在队列的位置
 * @PARAM	queue_handle : 队列句柄
 
 * @RETURN	队列头数据项
 * @RETVAL	
 */

void* divoom_queue_get_head_note(void * queue_handle);

/**
 * NAME         divoom_queue_is_full
 * @BRIEF	返回队列数据项数目
 * @PARAM	queue_handle : 队列句柄
 
 * @RETURN	数据项数目
 * @RETVAL	
 */

uint16 divoom_queue_items(void * queue_handle);


/**
 * NAME         divoom_queue_is_full
 * @BRIEF	返回指定数据项数据
 * @PARAM	queue_handle : 队列句柄
                    index              :数据项序号
 * @RETURN	用户数据项
 * @RETVAL	
 */

void* divoom_queue_get_index_item(void * queue_handle, int index);

/**
 * NAME         divoom_queue_get_all_items
 * @BRIEF	返回所有指定数据项数据
 * @PARAM	queue_handle : 队列句柄
                    data              :数据缓存器
                    psave_call          
 * @RETURN	总共多少项
 * @RETVAL	
 */

int divoom_queue_get_all_items(void * queue_handle, void *data, divoom_queue_save *psave_call);

/**
 * NAME         divoom_queue_get_index_item_no_lock
 * @BRIEF	返回指定数据项数据
 * @PARAM	queue_handle : 队列句柄
                    index              :数据项序号
 * @RETURN	用户数据项
 * @RETVAL	
 */

void* divoom_queue_get_index_item_no_lock(void * queue_handle, int index);


/**
 * NAME         divoom_queue_get_reset
 * @BRIEF	复位查找链表，将其指向头结点
 * @PARAM	queue_handle : 队列句柄
                    
 * @RETURN	当前队列中元素数目
 * @RETVAL	
 */

uint8 divoom_queue_get_reset(void * queue_handle);


/**
 * NAME         divoom_queue_lock
 * @BRIEF	对队列加锁，以便配合divoom_queue_get_next使用
 * @PARAM	queue_handle : 队列句柄
 * @RETURN	NONE
 * @RETVAL	
 */

void divoom_queue_lock(void * queue_handle);


/**
 * NAME         divoom_queue_unlock
 * @BRIEF	对队列解锁，以便配合divoom_queue_get_next使用
 * @PARAM	queue_handle : 队列句柄
 * @RETURN	NONE
 * @RETVAL	
 */

void divoom_queue_unlock(void * queue_handle);


/**
 * NAME         divoom_queue_get_next
 * @BRIEF	得到队列的相关数据，并不会对队列进行操作
 * @PARAM	queue_handle : 队列句柄
                    item        :用户要使用的比较数据
                    pcmp_chk:用户使用的比较函数,如果为空的话，将不判断条件
                    
 * @RETURN	返回队列查找到的数据
 * @RETVAL	
 */

void *divoom_queue_get_item(void * queue_handle, void *item, divoom_queue_compare *pcmp_chk);



/**
 * NAME         divoom_queue_pop_item
 * @BRIEF	将指定项的用户数据返回给用户，队列中弹出此数据项
 * @PARAM	queue_handle : 队列句柄
                    item:使用push传入的用户数据
                    
 * @RETURN	返回队指定的数据项
 * @RETVAL	
 */

void * divoom_queue_pop_item(void * queue_handle, void *item);


/**
 * NAME         divoom_queue_pop
 * @BRIEF	将用户数据返回给用户，队列中弹出此数据项
 * @PARAM	queue_handle : 队列句柄
                    item        :用户要使用的比较数据
                    pcmp_chk:用户使用的比较函数
                    
 * @RETURN	返回队指定的数据项
 * @RETVAL	
 */

void * divoom_queue_pop_cmp(void * queue_handle, void *item, divoom_queue_compare *pcmp_chk);

/**
 * NAME         divoom_queue_get_cmp
 * @BRIEF	将用户数据返回给用户，队列中取出此数据项
 * @PARAM	queue_handle : 队列句柄
                    item        :用户要使用的比较数据
                    pcmp_chk:用户使用的比较函数
                    
 * @RETURN	返回队指定的数据项
 * @RETVAL	
 */

void * divoom_queue_get_cmp(void * queue_handle, void *item, divoom_queue_compare *pcmp_chk);

#endif

